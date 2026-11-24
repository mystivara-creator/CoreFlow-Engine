/**
 * @file causal_verifier.cpp
 * @brief CoreFlow v6.1 Online Causal Verification Engine (`device_effectiveness.map`, CFEM1).
 * @license Apache-2.0
 */
#include "coreflow/v6/causal_verifier.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow::v6 {

namespace {

constexpr double kPreEwmaAlpha = 0.35;

std::string sanitize_token(std::string_view raw) {
    std::string out;
    out.reserve(std::min<size_t>(raw.size(), 128U));
    for (unsigned char c : raw) {
        if (out.size() >= 128U) break;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_' ||
            c == ':' || c == '/') {
            out.push_back(static_cast<char>(c));
        } else if (c == ' ') {
            out.push_back('_');
        }
    }
    return out;
}

bool write_all_fd(int fd, std::string_view data) noexcept {
    size_t off = 0U;
    while (off < data.size()) {
        const ssize_t n = ::write(fd, data.data() + off, data.size() - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        off += static_cast<size_t>(n);
    }
    return true;
}

std::string read_small_raw(const std::string& path, size_t max_bytes = 8192U) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        (void)::close(fd);
        return {};
    }
    char buf[1024];
    std::string out;
    while (out.size() < max_bytes) {
        const ssize_t n = ::read(fd, buf, std::min(sizeof(buf), max_bytes - out.size()));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        out.append(buf, static_cast<size_t>(n));
    }
    (void)::close(fd);
    return out;
}

inline double finite_or(double v, double fallback = 0.0) noexcept {
    return std::isfinite(v) ? v : fallback;
}

inline void ewma_step(double& acc, double val, bool first) noexcept {
    if (first) {
        acc = val;
    } else {
        acc += kPreEwmaAlpha * (val - acc);
    }
}

inline bool domain_is_active(CausalDomain d, const CausalObservation& obs) noexcept {
    switch (d) {
        case CausalDomain::ThermalUclampCap: return obs.uclamp_cap_active;
        case CausalDomain::ThermalGpuCap: return obs.gpu_cap_active;
        case CausalDomain::CpuGovTuning: return obs.cpu_gov_active;
        case CausalDomain::SemanticCpuGpuBoost: return obs.semantic_boost_active;
        case CausalDomain::MemoryBusFloor: return obs.memory_bus_active;
        case CausalDomain::StorageMemoryTuning: return obs.storage_memory_active;
        case CausalDomain::Count: break;
    }
    return false;
}

} // namespace

const char* CausalVerifier::domain_key(CausalDomain domain) noexcept {
    switch (domain) {
        case CausalDomain::ThermalUclampCap: return "thermal_uclamp_cap";
        case CausalDomain::ThermalGpuCap: return "thermal_gpu_cap";
        case CausalDomain::CpuGovTuning: return "cpu_gov_tuning";
        case CausalDomain::SemanticCpuGpuBoost: return "semantic_cpu_gpu_boost";
        case CausalDomain::MemoryBusFloor: return "memory_bus_floor";
        case CausalDomain::StorageMemoryTuning: return "storage_memory_tuning";
        case CausalDomain::Count: break;
    }
    return "unknown";
}

const char* CausalVerifier::state_name(CausalState state) noexcept {
    switch (state) {
        case CausalState::Probing: return "probing";
        case CausalState::VerifiedEffective: return "effective";
        case CausalState::BypassedIneffective: return "bypassed";
    }
    return "probing";
}

bool CausalVerifier::parse_domain_key(std::string_view key, CausalDomain& out) noexcept {
    for (size_t i = 0U; i < kDomainCount; ++i) {
        const auto d = static_cast<CausalDomain>(i);
        if (key == domain_key(d)) {
            out = d;
            return true;
        }
    }
    return false;
}

bool CausalVerifier::parse_state_name(std::string_view name, CausalState& out) noexcept {
    if (name == "probing") {
        out = CausalState::Probing;
        return true;
    }
    if (name == "effective") {
        out = CausalState::VerifiedEffective;
        return true;
    }
    if (name == "bypassed") {
        out = CausalState::BypassedIneffective;
        return true;
    }
    return false;
}

void CausalVerifier::initialize(
    const std::string& state_dir,
    const std::string& kernel_release,
    const std::string& rom_fingerprint) noexcept {
    state_dir_ = state_dir;
    map_path_ = state_dir.empty() ? std::string{} : (state_dir + "/device_effectiveness.map");
    kernel_release_ = sanitize_token(kernel_release);
    if (kernel_release_.empty()) kernel_release_ = "unknown-kernel";
    rom_fingerprint_ = sanitize_token(rom_fingerprint);
    if (rom_fingerprint_.empty()) rom_fingerprint_ = "unknown-rom";
    trackers_ = {};
    dirty_ = false;
    significant_dirty_ = false;
    flush_count_ = 0U;
    load_locked();
}

void CausalVerifier::clear_all() noexcept {
    trackers_ = {};
    dirty_ = false;
    significant_dirty_ = false;
    if (!map_path_.empty()) {
        (void)::unlink(map_path_.c_str());
    }
}

bool CausalVerifier::is_bypassed(CausalDomain domain, bool thermal_guard) const noexcept {
    if (thermal_guard) return false; // Absolute safety invariant
    const size_t idx = static_cast<size_t>(domain);
    if (idx >= kDomainCount) return false;
    return trackers_[idx].state == CausalState::BypassedIneffective;
}

CausalDomainSnapshot CausalVerifier::domain_snapshot(CausalDomain domain) const noexcept {
    CausalDomainSnapshot snap{};
    snap.domain = domain;
    const size_t idx = static_cast<size_t>(domain);
    if (idx >= kDomainCount) return snap;
    const auto& tr = trackers_[idx];
    snap.state = tr.state;
    snap.samples = tr.samples;
    snap.score = tr.score;
    snap.confidence = tr.confidence;
    return snap;
}

std::array<CausalDomainSnapshot, CausalVerifier::kDomainCount>
CausalVerifier::snapshot_all() const noexcept {
    std::array<CausalDomainSnapshot, kDomainCount> out{};
    for (size_t i = 0U; i < kDomainCount; ++i) {
        out[i] = domain_snapshot(static_cast<CausalDomain>(i));
    }
    return out;
}

uint32_t CausalVerifier::effective_count() const noexcept {
    uint32_t n = 0U;
    for (const auto& tr : trackers_) {
        if (tr.state == CausalState::VerifiedEffective) ++n;
    }
    return n;
}

uint32_t CausalVerifier::bypassed_count() const noexcept {
    uint32_t n = 0U;
    for (const auto& tr : trackers_) {
        if (tr.state == CausalState::BypassedIneffective) ++n;
    }
    return n;
}

uint32_t CausalVerifier::probing_count() const noexcept {
    uint32_t n = 0U;
    for (const auto& tr : trackers_) {
        if (tr.state == CausalState::Probing) ++n;
    }
    return n;
}

void CausalVerifier::update_pre_window(DomainTracker& tr, const CausalObservation& obs) noexcept {
    const bool first = (tr.pre_samples == 0U);
    auto& p = tr.pre_ewma;

    ewma_step(p.cpu_util_pct, std::clamp(finite_or(obs.cpu_util_pct), 0.0, 100.0), first);
    ewma_step(p.gpu_busy_pct, std::clamp(finite_or(obs.gpu_busy_pct), 0.0, 100.0), first);
    ewma_step(p.thermal_vel_cpm, std::clamp(finite_or(obs.thermal_velocity_cpm), -10.0, 10.0), first);
    ewma_step(p.refault_ps, std::max(0.0, finite_or(obs.refault_file_per_sec)), first);
    ewma_step(p.reclaim_ps, std::max(0.0, finite_or(obs.direct_reclaim_per_sec)), first);

    const bool pwr_ok = obs.power_available && !obs.charging &&
                        std::isfinite(obs.power_mw) && obs.power_mw > 50.0;
    if (pwr_ok) {
        ewma_step(p.power_mw, obs.power_mw, first || !p.power_ok);
        p.power_ok = true;
    } else if (first || obs.charging) {
        p.power_ok = false;
        p.power_mw = 0.0;
    }

    if (std::isfinite(obs.mean_cluster_khz) && obs.mean_cluster_khz > 0.0) {
        ewma_step(p.mean_khz, obs.mean_cluster_khz, first || p.mean_khz <= 0.0);
    }

    if (std::isfinite(obs.jank_ratio) && obs.jank_ratio >= 0.0) {
        ewma_step(p.jank_ratio, std::clamp(obs.jank_ratio, 0.0, 1.0), first || !p.jank_ok);
        p.jank_ok = true;
    }
    if (std::isfinite(obs.starvation_pressure) && obs.starvation_pressure >= 0.0) {
        ewma_step(p.starvation, std::clamp(obs.starvation_pressure, 0.0, 1.0), first || !p.starv_ok);
        p.starv_ok = true;
    }
    if (std::isfinite(obs.cpu_psi) && obs.cpu_psi >= 0.0) {
        ewma_step(p.cpu_psi, std::clamp(obs.cpu_psi, 0.0, 100.0), first || !p.cpu_psi_ok);
        p.cpu_psi_ok = true;
    }
    if (std::isfinite(obs.io_psi) && obs.io_psi >= 0.0) {
        ewma_step(p.io_psi, std::clamp(obs.io_psi, 0.0, 100.0), first || !p.io_psi_ok);
        p.io_psi_ok = true;
    }
    if (std::isfinite(obs.mem_psi) && obs.mem_psi >= 0.0) {
        ewma_step(p.mem_psi, std::clamp(obs.mem_psi, 0.0, 100.0), first || !p.mem_psi_ok);
        p.mem_psi_ok = true;
    }

    if (tr.pre_samples < 10000U) ++tr.pre_samples;
}

void CausalVerifier::begin_trial(DomainTracker& tr) noexcept {
    tr.in_trial = true;
    tr.trial_pre = tr.pre_ewma;
    tr.settle_elapsed_sec = 0.0;
    tr.post_elapsed_sec = 0.0;
    tr.post_samples = 0U;
    tr.sum_cpu = 0.0;
    tr.sum_gpu = 0.0;
    tr.power_samples = 0U;
    tr.sum_power = 0.0;
    tr.sum_vel = 0.0;
    tr.khz_samples = 0U;
    tr.sum_khz = 0.0;
    tr.jank_samples = 0U;
    tr.sum_jank = 0.0;
    tr.starv_samples = 0U;
    tr.sum_starv = 0.0;
    tr.cpu_psi_samples = 0U;
    tr.sum_cpu_psi = 0.0;
    tr.io_psi_samples = 0U;
    tr.sum_io_psi = 0.0;
    tr.mem_psi_samples = 0U;
    tr.sum_mem_psi = 0.0;
    tr.sum_refault = 0.0;
    tr.sum_reclaim = 0.0;
}

void CausalVerifier::accumulate_post(DomainTracker& tr, const CausalObservation& obs) noexcept {
    ++tr.post_samples;
    tr.sum_cpu += std::clamp(finite_or(obs.cpu_util_pct), 0.0, 100.0);
    tr.sum_gpu += std::clamp(finite_or(obs.gpu_busy_pct), 0.0, 100.0);
    tr.sum_vel += std::clamp(finite_or(obs.thermal_velocity_cpm), -10.0, 10.0);
    tr.sum_refault += std::max(0.0, finite_or(obs.refault_file_per_sec));
    tr.sum_reclaim += std::max(0.0, finite_or(obs.direct_reclaim_per_sec));

    if (obs.power_available && !obs.charging && std::isfinite(obs.power_mw) && obs.power_mw > 50.0) {
        ++tr.power_samples;
        tr.sum_power += obs.power_mw;
    }
    if (std::isfinite(obs.mean_cluster_khz) && obs.mean_cluster_khz > 0.0) {
        ++tr.khz_samples;
        tr.sum_khz += obs.mean_cluster_khz;
    }
    if (std::isfinite(obs.jank_ratio) && obs.jank_ratio >= 0.0) {
        ++tr.jank_samples;
        tr.sum_jank += std::clamp(obs.jank_ratio, 0.0, 1.0);
    }
    if (std::isfinite(obs.starvation_pressure) && obs.starvation_pressure >= 0.0) {
        ++tr.starv_samples;
        tr.sum_starv += std::clamp(obs.starvation_pressure, 0.0, 1.0);
    }
    if (std::isfinite(obs.cpu_psi) && obs.cpu_psi >= 0.0) {
        ++tr.cpu_psi_samples;
        tr.sum_cpu_psi += std::clamp(obs.cpu_psi, 0.0, 100.0);
    }
    if (std::isfinite(obs.io_psi) && obs.io_psi >= 0.0) {
        ++tr.io_psi_samples;
        tr.sum_io_psi += std::clamp(obs.io_psi, 0.0, 100.0);
    }
    if (std::isfinite(obs.mem_psi) && obs.mem_psi >= 0.0) {
        ++tr.mem_psi_samples;
        tr.sum_mem_psi += std::clamp(obs.mem_psi, 0.0, 100.0);
    }
}

bool CausalVerifier::evaluate_domain_effect(
    CausalDomain domain,
    const MetricsSample& pre,
    const MetricsSample& post,
    double& out_effect) noexcept {
    const double d_cpu = std::abs(post.cpu_util_pct - pre.cpu_util_pct);
    const double d_gpu = std::abs(post.gpu_busy_pct - pre.gpu_busy_pct);

    switch (domain) {
        case CausalDomain::ThermalUclampCap: {
            if (!pre.power_ok || !post.power_ok) return false;
            if (pre.cpu_util_pct < 20.0 && pre.gpu_busy_pct < 20.0) return false;
            if (d_cpu > kMaxMatchedLoadDeltaPct || d_gpu > kMaxMatchedLoadDeltaPct) return false;
            const double pwr_drop = (pre.power_mw - post.power_mw) / std::max(pre.power_mw, 500.0);
            const double vel_drop = (pre.thermal_vel_cpm > 0.15)
                ? std::clamp((pre.thermal_vel_cpm - post.thermal_vel_cpm) * 0.25, -0.25, 0.45)
                : pwr_drop;
            out_effect = std::clamp(std::max(pwr_drop, 0.55 * pwr_drop + 0.45 * vel_drop), -0.30, 0.60);
            return true;
        }
        case CausalDomain::ThermalGpuCap: {
            if (!pre.power_ok || !post.power_ok) return false;
            if (pre.gpu_busy_pct < 30.0) return false;
            if (d_cpu > kMaxMatchedLoadDeltaPct || d_gpu > kMaxMatchedLoadDeltaPct) return false;
            const double pwr_drop = (pre.power_mw - post.power_mw) / std::max(pre.power_mw, 500.0);
            const double vel_drop = (pre.thermal_vel_cpm > 0.15)
                ? std::clamp((pre.thermal_vel_cpm - post.thermal_vel_cpm) * 0.25, -0.25, 0.45)
                : pwr_drop;
            out_effect = std::clamp(std::max(pwr_drop, 0.55 * pwr_drop + 0.45 * vel_drop), -0.30, 0.60);
            return true;
        }
        case CausalDomain::CpuGovTuning: {
            if (pre.cpu_util_pct < 20.0) return false;
            if (d_cpu > kMaxMatchedLoadDeltaPct) return false;
            const bool has_khz = (pre.mean_khz > 100000.0 && post.mean_khz > 100000.0);
            const bool has_starv = (pre.starv_ok && post.starv_ok && pre.starvation >= 0.12);
            const bool has_jank = (pre.jank_ok && post.jank_ok && pre.jank_ratio >= 0.025);
            if (!has_khz && !has_starv && !has_jank) return false;
            double best = -1.0;
            if (has_khz) {
                const double khz_resp = std::abs(post.mean_khz - pre.mean_khz) / std::max(pre.mean_khz, 300000.0);
                best = std::max(best, std::clamp(khz_resp * 0.8, 0.0, 0.40));
            }
            if (has_starv) {
                best = std::max(best, std::clamp((pre.starvation - post.starvation) * 1.5, -0.30, 0.50));
            }
            if (has_jank) {
                best = std::max(best, std::clamp((pre.jank_ratio - post.jank_ratio) * 2.5, -0.30, 0.50));
            }
            out_effect = std::clamp(best, -0.30, 0.60);
            return true;
        }
        case CausalDomain::SemanticCpuGpuBoost: {
            if (pre.cpu_util_pct < 20.0 && pre.gpu_busy_pct < 20.0) return false;
            if (d_cpu > kMaxMatchedLoadDeltaPct || d_gpu > kMaxMatchedLoadDeltaPct) return false;
            // Precondition against ceiling effect: only evaluate when pre-window actually had
            // measurable latency/contention headroom to improve.
            const bool jank_hr = (pre.jank_ok && post.jank_ok && pre.jank_ratio >= 0.025);
            const bool starv_hr = (pre.starv_ok && post.starv_ok && pre.starvation >= 0.12);
            const bool psi_hr = (pre.cpu_psi_ok && post.cpu_psi_ok && pre.cpu_psi >= 5.0);
            if (!jank_hr && !starv_hr && !psi_hr) return false;

            double raw_gain = -1.0;
            if (jank_hr) {
                raw_gain = std::max(raw_gain, std::clamp((pre.jank_ratio - post.jank_ratio) * 2.5, -0.30, 0.50));
            }
            if (starv_hr) {
                raw_gain = std::max(raw_gain, std::clamp((pre.starvation - post.starvation) * 1.5, -0.30, 0.50));
            }
            if (psi_hr) {
                raw_gain = std::max(raw_gain, std::clamp((pre.cpu_psi - post.cpu_psi) / 25.0, -0.25, 0.45));
            }
            double power_penalty = 0.0;
            if (pre.power_ok && post.power_ok && raw_gain <= 0.015) {
                const double extra_w_frac = (post.power_mw - pre.power_mw) / std::max(pre.power_mw, 500.0);
                if (extra_w_frac > 0.03) {
                    power_penalty = std::clamp(extra_w_frac * 0.5, 0.0, 0.25);
                }
            }
            out_effect = std::clamp(raw_gain - power_penalty, -0.35, 0.60);
            return true;
        }
        case CausalDomain::MemoryBusFloor: {
            if (pre.gpu_busy_pct < 30.0 && !(pre.io_psi_ok && pre.io_psi >= 4.0)) return false;
            if (d_gpu > kMaxMatchedLoadDeltaPct) return false;
            const bool jank_hr = (pre.jank_ok && post.jank_ok && pre.jank_ratio >= 0.025);
            const bool io_hr = (pre.io_psi_ok && post.io_psi_ok && pre.io_psi >= 4.0);
            const bool starv_hr = (pre.starv_ok && post.starv_ok && pre.starvation >= 0.12);
            if (!jank_hr && !io_hr && !starv_hr) return false;
            double best = -1.0;
            if (jank_hr) best = std::max(best, std::clamp((pre.jank_ratio - post.jank_ratio) * 2.5, -0.30, 0.50));
            if (io_hr) best = std::max(best, std::clamp((pre.io_psi - post.io_psi) / std::max(pre.io_psi, 5.0), -0.30, 0.50));
            if (starv_hr) best = std::max(best, std::clamp((pre.starvation - post.starvation) * 1.5, -0.30, 0.50));
            out_effect = std::clamp(best, -0.30, 0.60);
            return true;
        }
        case CausalDomain::StorageMemoryTuning: {
            const bool refault_hr = (pre.refault_ps >= 120.0);
            const bool reclaim_hr = (pre.reclaim_ps >= 20.0);
            const bool io_hr = (pre.io_psi_ok && post.io_psi_ok && pre.io_psi >= 5.0);
            const bool mem_hr = (pre.mem_psi_ok && post.mem_psi_ok && pre.mem_psi >= 5.0);
            if (!refault_hr && !reclaim_hr && !io_hr && !mem_hr) return false;
            double best = -1.0;
            if (refault_hr) {
                best = std::max(best, std::clamp((pre.refault_ps - post.refault_ps) / pre.refault_ps, -0.30, 0.60));
            }
            if (reclaim_hr) {
                best = std::max(best, std::clamp((pre.reclaim_ps - post.reclaim_ps) / pre.reclaim_ps, -0.30, 0.60));
            }
            if (io_hr) {
                best = std::max(best, std::clamp((pre.io_psi - post.io_psi) / pre.io_psi, -0.30, 0.60));
            }
            if (mem_hr) {
                best = std::max(best, std::clamp((pre.mem_psi - post.mem_psi) / pre.mem_psi, -0.30, 0.60));
            }
            out_effect = std::clamp(best, -0.30, 0.60);
            return true;
        }
        case CausalDomain::Count:
            break;
    }
    return false;
}

void CausalVerifier::finalize_trial(
    CausalDomain domain,
    DomainTracker& tr,
    CausalObserveResult& result) noexcept {
    if (tr.post_samples == 0U) return;
    const double inv = 1.0 / static_cast<double>(tr.post_samples);

    MetricsSample post{};
    post.cpu_util_pct = tr.sum_cpu * inv;
    post.gpu_busy_pct = tr.sum_gpu * inv;
    post.thermal_vel_cpm = tr.sum_vel * inv;
    post.refault_ps = tr.sum_refault * inv;
    post.reclaim_ps = tr.sum_reclaim * inv;
    if (tr.power_samples > 0U) {
        post.power_ok = true;
        post.power_mw = tr.sum_power / static_cast<double>(tr.power_samples);
    }
    if (tr.khz_samples > 0U) {
        post.mean_khz = tr.sum_khz / static_cast<double>(tr.khz_samples);
    }
    if (tr.jank_samples > 0U) {
        post.jank_ok = true;
        post.jank_ratio = tr.sum_jank / static_cast<double>(tr.jank_samples);
    }
    if (tr.starv_samples > 0U) {
        post.starv_ok = true;
        post.starvation = tr.sum_starv / static_cast<double>(tr.starv_samples);
    }
    if (tr.cpu_psi_samples > 0U) {
        post.cpu_psi_ok = true;
        post.cpu_psi = tr.sum_cpu_psi / static_cast<double>(tr.cpu_psi_samples);
    }
    if (tr.io_psi_samples > 0U) {
        post.io_psi_ok = true;
        post.io_psi = tr.sum_io_psi / static_cast<double>(tr.io_psi_samples);
    }
    if (tr.mem_psi_samples > 0U) {
        post.mem_psi_ok = true;
        post.mem_psi = tr.sum_mem_psi / static_cast<double>(tr.mem_psi_samples);
    }

    double effect = 0.0;
    if (!evaluate_domain_effect(domain, tr.trial_pre, post, effect)) {
        return;
    }

    const uint32_t bit = 1U << static_cast<uint32_t>(domain);
    result.completed_trials_mask |= bit;

    ++tr.samples;
    if (tr.samples == 1U) {
        tr.score = effect;
    } else {
        const double alpha = std::max(0.25, 1.0 / static_cast<double>(tr.samples));
        tr.score += alpha * (effect - tr.score);
    }
    tr.score = std::clamp(tr.score, -1.0, 1.0);
    tr.confidence = std::min<uint32_t>(100U, tr.samples * 20U);
    dirty_ = true;

    const CausalState prev_state = tr.state;
    if (tr.samples >= kMinSamplesForBypass && tr.score <= kBypassScoreThreshold) {
        tr.state = CausalState::BypassedIneffective;
        if (prev_state != CausalState::BypassedIneffective) {
            result.newly_bypassed_mask |= bit;
            significant_dirty_ = true;
        }
    } else if (tr.samples >= kMinSamplesForVerify && tr.score >= kVerifyScoreThreshold) {
        tr.state = CausalState::VerifiedEffective;
        if (prev_state != CausalState::VerifiedEffective) {
            result.newly_verified_mask |= bit;
            significant_dirty_ = true;
        }
    }

    if (tr.state != tr.persisted_state ||
        std::abs(tr.score - tr.persisted_score) >= kSignificantDriftDelta) {
        significant_dirty_ = true;
    }
}

CausalObserveResult CausalVerifier::observe(const CausalObservation& obs) noexcept {
    CausalObserveResult result{};

    const bool bad_dt = !std::isfinite(obs.dt_sec) || obs.dt_sec <= 0.0 || obs.dt_sec > kMaxGapResetSec;
    if (bad_dt || obs.screen_off || obs.boot_settle || obs.inspection_mode || obs.thermal_guard) {
        for (auto& tr : trackers_) {
            tr.in_trial = false;
            tr.pre_samples = 0U;
            if (obs.screen_off || obs.boot_settle || bad_dt) {
                tr.latched_until_inactive = false;
            }
        }
        return result;
    }

    for (size_t i = 0U; i < kDomainCount; ++i) {
        const auto domain = static_cast<CausalDomain>(i);
        auto& tr = trackers_[i];
        const bool active = domain_is_active(domain, obs);

        if (active) {
            if (!tr.in_trial) {
                if (!tr.latched_until_inactive &&
                    tr.pre_samples >= kMinPreSamples &&
                    tr.state != CausalState::BypassedIneffective) {
                    begin_trial(tr);
                }
            }
            if (tr.in_trial) {
                tr.settle_elapsed_sec += obs.dt_sec;
                if (tr.settle_elapsed_sec > kSettleDurationSec) {
                    tr.post_elapsed_sec += obs.dt_sec;
                    accumulate_post(tr, obs);
                    if (tr.post_elapsed_sec >= kPostWindowTargetSec && tr.post_samples >= 8U) {
                        finalize_trial(domain, tr, result);
                        tr.in_trial = false;
                        tr.latched_until_inactive = true;
                    }
                }
            }
        } else {
            if (tr.in_trial) {
                if (tr.post_samples >= kMinPostSamplesShort) {
                    finalize_trial(domain, tr, result);
                }
                tr.in_trial = false;
            }
            tr.latched_until_inactive = false;
            update_pre_window(tr, obs);
        }
    }

    return result;
}

bool CausalVerifier::flush_if_significant() noexcept {
    if (!significant_dirty_) return false;
    return persist_locked();
}

bool CausalVerifier::flush_if_dirty() noexcept {
    if (!dirty_) return false;
    return persist_locked();
}

void CausalVerifier::load_locked() noexcept {
    if (map_path_.empty()) return;
    const std::string raw = read_small_raw(map_path_, 8192U);
    if (raw.empty()) return;

    std::istringstream ss(raw);
    std::string header;
    if (!std::getline(ss, header)) return;
    if (!header.empty() && header.back() == '\r') header.pop_back();

    const std::string expected_header =
        "CFEM1\t" + kernel_release_ + "\t" + rom_fingerprint_;
    if (header != expected_header) {
        // Kernel or ROM updated -> auto-reset effectiveness map to re-verify on new OS build.
        (void)::unlink(map_path_.c_str());
        return;
    }

    std::string line;
    size_t lines_read = 0U;
    while (std::getline(ss, line) && lines_read < 32U) {
        ++lines_read;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::istringstream ls(line);
        std::string key_tok, state_tok, samples_tok, score_tok, conf_tok;
        if (!std::getline(ls, key_tok, '\t') ||
            !std::getline(ls, state_tok, '\t') ||
            !std::getline(ls, samples_tok, '\t') ||
            !std::getline(ls, score_tok, '\t') ||
            !std::getline(ls, conf_tok, '\t')) {
            continue;
        }

        CausalDomain dom{};
        CausalState st{};
        if (!parse_domain_key(key_tok, dom) || !parse_state_name(state_tok, st)) continue;

        char* end_u = nullptr;
        const unsigned long samples_ul = std::strtoul(samples_tok.c_str(), &end_u, 10);
        if (end_u == samples_tok.c_str() || *end_u != '\0') continue;

        char* end_d = nullptr;
        const double score_d = std::strtod(score_tok.c_str(), &end_d);
        if (end_d == score_tok.c_str() || *end_d != '\0' || !std::isfinite(score_d)) continue;

        char* end_c = nullptr;
        const unsigned long conf_ul = std::strtoul(conf_tok.c_str(), &end_c, 10);
        if (end_c == conf_tok.c_str() || *end_c != '\0') continue;

        auto& tr = trackers_[static_cast<size_t>(dom)];
        tr.state = st;
        tr.samples = static_cast<uint32_t>(std::min<unsigned long>(samples_ul, 100000UL));
        tr.score = std::clamp(score_d, -1.0, 1.0);
        tr.confidence = static_cast<uint32_t>(std::min<unsigned long>(conf_ul, 100UL));
        tr.persisted_state = tr.state;
        tr.persisted_score = tr.score;
    }
}

bool CausalVerifier::persist_locked() noexcept {
    if (state_dir_.empty() || map_path_.empty()) return false;
    struct stat st{};
    if (::lstat(state_dir_.c_str(), &st) != 0 || !S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode)) {
        return false;
    }

    std::string payload = "CFEM1\t" + kernel_release_ + "\t" + rom_fingerprint_ + "\n";
    char line_buf[160];
    for (size_t i = 0U; i < kDomainCount; ++i) {
        const auto dom = static_cast<CausalDomain>(i);
        const auto& tr = trackers_[i];
        const int n = std::snprintf(
            line_buf, sizeof(line_buf), "%s\t%s\t%u\t%.4f\t%u\n",
            domain_key(dom), state_name(tr.state),
            static_cast<unsigned>(tr.samples),
            std::isfinite(tr.score) ? tr.score : 0.0,
            static_cast<unsigned>(tr.confidence));
        if (n > 0 && static_cast<size_t>(n) < sizeof(line_buf)) {
            payload.append(line_buf, static_cast<size_t>(n));
        }
    }

    const std::string tmp_path = map_path_ + ".tmp." + std::to_string(::getpid());
    (void)::unlink(tmp_path.c_str());
    const int fd = ::open(tmp_path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) return false;
    const bool ok = write_all_fd(fd, payload) && (::fsync(fd) == 0);
    (void)::close(fd);
    if (!ok) {
        (void)::unlink(tmp_path.c_str());
        return false;
    }
    if (::rename(tmp_path.c_str(), map_path_.c_str()) != 0) {
        (void)::unlink(tmp_path.c_str());
        return false;
    }
    const int dfd = ::open(state_dir_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (dfd >= 0) {
        (void)::fsync(dfd);
        (void)::close(dfd);
    }

    for (auto& tr : trackers_) {
        tr.persisted_state = tr.state;
        tr.persisted_score = tr.score;
    }
    dirty_ = false;
    significant_dirty_ = false;
    ++flush_count_;
    return true;
}

} // namespace coreflow::v6
