/**
 * @file thermal_envelope_actuator.cpp
 * @brief CoreFlow v5 Pillar 3C: Smooth CPU/GPU governor & thermal envelope actuator driven by Rust PID + Context.
 * @license MIT
 */
#include "coreflow/v5/thermal_envelope_actuator.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>

namespace coreflow::v5 {

namespace {

std::string read_line_trimmed(const std::string& path) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    char buf[256];
    const ssize_t n = ::read(fd, buf, sizeof(buf));
    (void)::close(fd);
    if (n <= 0) return {};
    std::string out(buf, static_cast<size_t>(n));
    const auto nl = out.find('\n');
    if (nl != std::string::npos) out.resize(nl);
    while (!out.empty() && (out.back() == '\r' || out.back() == ' ' || out.back() == '\t')) out.pop_back();
    size_t start = 0;
    while (start < out.size() && (out[start] == ' ' || out[start] == '\t')) ++start;
    return out.substr(start);
}

bool parse_u64_exact(std::string_view text, uint64_t& out) noexcept {
    if (text.empty() || text.size() > 24U) return false;
    const auto r = std::from_chars(text.data(), text.data() + text.size(), out);
    return r.ec == std::errc{} && r.ptr == text.data() + text.size();
}

} // namespace

ThermalEnvelopeActuator::ThermalEnvelopeActuator(BaselineStore& baseline) noexcept
    : baseline_(baseline) {}

void ThermalEnvelopeActuator::reset_contention() noexcept {
    contended_paths_.clear();
    last_written_.clear();
    stats_.contended_nodes = 0U;
    stats_.managed_nodes = 0U;
}

uint64_t ThermalEnvelopeActuator::select_opp(
    const std::vector<uint64_t>& sorted_opps, double fraction) noexcept {
    if (sorted_opps.empty()) return 0U;
    const double clamped = std::isfinite(fraction) ? std::clamp(fraction, 0.0, 1.0) : 0.5;
    const size_t idx = static_cast<size_t>(
        std::llround(static_cast<double>(sorted_opps.size() - 1U) * clamped));
    return sorted_opps[std::min(idx, sorted_opps.size() - 1U)];
}

std::pair<uint64_t, uint64_t> ThermalEnvelopeActuator::compute_kgsl_levels(
    uint64_t num_pwrlevels,
    ContextMode mode,
    PerformanceProfile profile,
    double capacity_scale,
    bool thermal_guard) noexcept {
    // KGSL: level 0 is fastest OPP; num_pwrlevels - 1 is slowest (some older drivers reserve last level).
    if (num_pwrlevels < 2U) return {0U, 0U};
    const uint64_t max_safe_idx = num_pwrlevels >= 3U ? (num_pwrlevels - 2U) : (num_pwrlevels - 1U);
    const double scale = std::isfinite(capacity_scale) ? std::clamp(capacity_scale, 0.25, 1.0) : 1.0;

    double base_max_frac = 0.25; // lower index = faster ceiling
    double base_min_frac = 0.75; // higher index = lower floor (power saving)
    if (thermal_guard) {
        base_max_frac = 0.50;
        base_min_frac = 0.88;
    } else if (mode == ContextMode::ScreenOff) {
        base_max_frac = 0.65;
        base_min_frac = 1.00;
    } else if (mode == ContextMode::Gaming || mode == ContextMode::LaunchBurst) {
        base_max_frac = (profile == PerformanceProfile::Battery) ? 0.20 : 0.0;
        base_min_frac = (mode == ContextMode::Gaming) ? 0.45 : 0.60;
    } else if (mode == ContextMode::Interactive) {
        base_max_frac = (profile == PerformanceProfile::Smoothness) ? 0.10 : 0.20;
        base_min_frac = 0.70;
    } else { // Efficiency
        base_max_frac = (profile == PerformanceProfile::Battery) ? 0.45 : 0.35;
        base_min_frac = 0.90;
    }

    // Proportional PID thermal pacing shifts ceiling smoothly downward as capacity_scale drops.
    const double throttle_shift = (1.0 - scale) * 0.55;
    const double max_frac = std::clamp(base_max_frac + throttle_shift, 0.0, 0.90);
    const double min_frac = std::clamp(std::max(max_frac, base_min_frac + throttle_shift * 0.30), 0.0, 1.0);

    const uint64_t target_max = static_cast<uint64_t>(std::llround(static_cast<double>(max_safe_idx) * max_frac));
    const uint64_t target_min = std::max(
        target_max,
        static_cast<uint64_t>(std::llround(static_cast<double>(max_safe_idx) * min_frac)));
    return {std::min(target_max, max_safe_idx), std::min(target_min, max_safe_idx)};
}

bool ThermalEnvelopeActuator::write_managed_node(
    const std::string& path, const std::string& target) noexcept {
    if (path.empty() || target.empty() || contended_paths_.contains(path)) return false;

    const std::string live = read_line_trimmed(path);
    if (live.empty()) return false;
    const auto prev = last_written_.find(path);
    if (prev != last_written_.end() && live != prev->second) {
        contended_paths_.insert(path);
        stats_.contended_nodes = static_cast<uint32_t>(contended_paths_.size());
        return false;
    }
    if (live == target) {
        last_written_[path] = target;
        return true;
    }

    if (!baseline_.capture_durable(path)) return false;
    if (!baseline_.apply_mutation(path, target)) return false;
    const std::string verify = read_line_trimmed(path);
    if (verify != target) {
        contended_paths_.insert(path);
        stats_.contended_nodes = static_cast<uint32_t>(contended_paths_.size());
        return false;
    }
    last_written_[path] = target;
    return true;
}

void ThermalEnvelopeActuator::rollback_all_managed() noexcept {
    for (const auto& [path, _] : last_written_) {
        (void)baseline_.rollback(path);
    }
    last_written_.clear();
    stats_.managed_nodes = 0U;
}

void ThermalEnvelopeActuator::apply(
    const HardwareTopology& topo,
    ContextMode mode,
    PerformanceProfile profile,
    double capacity_scale,
    bool thermal_guard,
    bool enabled) noexcept {
    const double scale = std::isfinite(capacity_scale) ? std::clamp(capacity_scale, 0.25, 1.0) : 1.0;
    stats_.last_capacity_scale = scale;

    if (!enabled || mode == ContextMode::Excluded) {
        if (!last_written_.empty()) rollback_all_managed();
        return;
    }

    const double frame_us = topo.refresh_hz > 0U ? (1000000.0 / static_cast<double>(topo.refresh_hz)) : 16666.0;
    const double profile_bias =
        profile == PerformanceProfile::Smoothness ? 1.08 :
        profile == PerformanceProfile::Gaming ? 1.10 :
        profile == PerformanceProfile::Battery ? 0.85 : 1.00;

    // 1. CPU Governor Envelopes (single-owner, PID capacity_scale modulated)
    for (const auto& cluster : topo.cpu_clusters) {
        const bool is_big = cluster.role == ClusterRole::Big;
        const bool is_mid = cluster.role == ClusterRole::Mid;

        if (!cluster.hispeed_freq_path.empty() && !cluster.opps_khz.empty()) {
            double base_tier = 0.65;
            if (thermal_guard) {
                base_tier = is_big ? 0.50 : is_mid ? 0.56 : 0.60;
            } else if (mode == ContextMode::ScreenOff) {
                base_tier = 0.30;
            } else if (mode == ContextMode::LaunchBurst || mode == ContextMode::Gaming) {
                base_tier = is_big ? 0.85 : 0.80;
            } else if (mode == ContextMode::Interactive) {
                base_tier = is_big ? 0.72 : is_mid ? 0.74 : 0.76;
            } else { // Efficiency
                base_tier = is_big ? 0.48 : is_mid ? 0.54 : 0.60;
            }
            const double effective_tier = std::clamp(base_tier * profile_bias * scale, 0.25, 1.0);
            const uint64_t freq = select_opp(cluster.opps_khz, effective_tier);
            if (freq > 0U) (void)write_managed_node(cluster.hispeed_freq_path, std::to_string(freq));
        }

        if (!cluster.hispeed_load_path.empty()) {
            uint32_t load = 88U;
            if (thermal_guard || mode == ContextMode::ScreenOff) load = 95U;
            else if (mode == ContextMode::LaunchBurst || mode == ContextMode::Gaming) load = 72U;
            else if (mode == ContextMode::Interactive) load = (profile == PerformanceProfile::Smoothness) ? 76U : 82U;
            else load = (profile == PerformanceProfile::Battery) ? 92U : 88U;
            (void)write_managed_node(cluster.hispeed_load_path, std::to_string(load));
        }

        if (!cluster.up_rate_limit_path.empty()) {
            const uint64_t up_us = thermal_guard ? 4000U :
                (mode == ContextMode::LaunchBurst || mode == ContextMode::Interactive)
                    ? static_cast<uint64_t>(std::clamp(frame_us * 0.05, 400.0, 1000.0))
                    : mode == ContextMode::Gaming ? 1000U : 2500U;
            (void)write_managed_node(cluster.up_rate_limit_path, std::to_string(up_us));
        }

        if (!cluster.down_rate_limit_path.empty()) {
            const uint64_t down_us = thermal_guard ? 5000U :
                mode == ContextMode::ScreenOff ? 5000U :
                (mode == ContextMode::LaunchBurst || mode == ContextMode::Gaming)
                    ? static_cast<uint64_t>(std::clamp(frame_us * 1.5, 15000.0, 25000.0))
                    : mode == ContextMode::Interactive
                        ? static_cast<uint64_t>(std::clamp(frame_us * 1.1, 10000.0, 18000.0))
                        : 8000U;
            (void)write_managed_node(cluster.down_rate_limit_path, std::to_string(down_us));
        }
    }

    // 2. GPU Idle Timer & Smooth Thermal Frequency Envelope
    if (topo.gpu.available) {
        if (!topo.gpu.idle_timer_path.empty()) {
            const double frames = (thermal_guard || mode == ContextMode::ScreenOff) ? 6.0 :
                mode == ContextMode::Gaming ? 22.0 :
                (mode == ContextMode::LaunchBurst || mode == ContextMode::Interactive) ? 14.0 : 8.0;
            const uint64_t idle_ms = static_cast<uint64_t>(
                std::clamp((frame_us / 1000.0) * frames, 40.0, 400.0));
            (void)write_managed_node(topo.gpu.idle_timer_path, std::to_string(idle_ms));
        }

        if (topo.gpu.is_kgsl &&
            !topo.gpu.kgsl_max_pwrlevel_path.empty() &&
            !topo.gpu.kgsl_min_pwrlevel_path.empty()) {
            const auto [target_max, target_min] = compute_kgsl_levels(
                topo.gpu.kgsl_num_pwrlevels, mode, profile, scale, thermal_guard);
            uint64_t live_min = target_min;
            (void)parse_u64_exact(read_line_trimmed(topo.gpu.kgsl_min_pwrlevel_path), live_min);
            // Order writes from the live interval so KGSL never clamps one against the stale peer.
            if (target_max <= live_min) {
                (void)write_managed_node(topo.gpu.kgsl_max_pwrlevel_path, std::to_string(target_max));
                (void)write_managed_node(topo.gpu.kgsl_min_pwrlevel_path, std::to_string(target_min));
            } else {
                (void)write_managed_node(topo.gpu.kgsl_min_pwrlevel_path, std::to_string(target_min));
                (void)write_managed_node(topo.gpu.kgsl_max_pwrlevel_path, std::to_string(target_max));
            }
        } else if (!topo.gpu.devfreq_max_freq_path.empty() && !topo.gpu.devfreq_opps_hz.empty()) {
            double base_tier = 0.80;
            if (thermal_guard) base_tier = 0.62;
            else if (mode == ContextMode::ScreenOff) base_tier = 0.35;
            else if (mode == ContextMode::Gaming || mode == ContextMode::LaunchBurst) base_tier = 1.00;
            else if (mode == ContextMode::Interactive) base_tier = 0.85;
            else base_tier = 0.65;
            const double tier = std::clamp(base_tier * profile_bias * scale, 0.30, 1.0);
            const uint64_t freq = select_opp(topo.gpu.devfreq_opps_hz, tier);
            if (freq > 0U) (void)write_managed_node(topo.gpu.devfreq_max_freq_path, std::to_string(freq));
        }
    }

    stats_.managed_nodes = static_cast<uint32_t>(last_written_.size());
}

} // namespace coreflow::v5
