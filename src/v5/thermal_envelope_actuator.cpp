/**
 * @file thermal_envelope_actuator.cpp
 * @brief CoreFlow v5 Pillar 3C: Smooth CPU/GPU governor & thermal envelope actuator driven by Rust PID + Context.
 * @license Apache-2.0
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

ThermalEnvelopeActuator::ThermalEnvelopeActuator(
    BaselineStore& baseline,
    v6::OemCollaborationMap* oem_map) noexcept
    : baseline_(baseline), oem_map_(oem_map) {}

void ThermalEnvelopeActuator::reset_contention() noexcept {
    contended_paths_.clear();
    last_written_.clear();
    baseline_hispeed_khz_.clear();
    stats_.contended_nodes = oem_map_ ? oem_map_->active_count() : 0U;
    stats_.managed_nodes = 0U;
}

std::vector<std::string> ThermalEnvelopeActuator::contended_paths() const {
    if (oem_map_) return oem_map_->active_contended_paths();
    std::vector<std::string> out;
    out.reserve(contended_paths_.size());
    for (const auto& [path, _] : contended_paths_) {
        out.push_back(path);
    }
    std::sort(out.begin(), out.end());
    return out;
}

void ThermalEnvelopeActuator::record_contention(
    const std::string& path,
    const std::string& observed,
    uint64_t now_ms) noexcept {
    // Adopt OEM's external value in CFJ4 without overwriting it, so journal stays clean
    // and persistent OEM collaboration escalates cleanly without repeated tug-of-war.
    (void)baseline_.rollback(path);
    last_written_.erase(path);
    baseline_hispeed_khz_.erase(path);
    if (oem_map_) {
        oem_map_->record_contention(path, observed, now_ms);
        stats_.contended_nodes = oem_map_->active_count(now_ms);
        return;
    }
    auto& entry = contended_paths_[path];
    ++entry.strikes;
    const uint32_t shift = std::min<uint32_t>(entry.strikes - 1U, 4U);
    const uint64_t backoff = std::min<uint64_t>(
        kMaxContentionCooldownMs, kInitialContentionCooldownMs << shift);
    entry.retry_after_ms = now_ms + backoff;
    entry.observed_value = observed;
    stats_.contended_nodes = static_cast<uint32_t>(contended_paths_.size());
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

    double base_max_frac = 0.0;  // lower index = faster ceiling (0 = unrestricted on-demand burst)
    double base_min_frac = 0.80; // higher index = lower floor (power saving when idle)
    if (thermal_guard) {
        base_max_frac = 0.45;
        base_min_frac = 0.88;
    } else if (mode == ContextMode::ScreenOff) {
        base_max_frac = 0.65;
        base_min_frac = 1.00;
    } else if (mode == ContextMode::Gaming || mode == ContextMode::LaunchBurst) {
        base_max_frac = (profile == PerformanceProfile::Battery) ? 0.15 : 0.0;
        base_min_frac = (mode == ContextMode::Gaming) ? 0.50 : 0.65;
    } else if (mode == ContextMode::Interactive) {
        base_max_frac = 0.0;
        base_min_frac = 0.75;
    } else { // Efficiency: let GPU idle at low floor while keeping burst ceiling ready for UI blurs
        base_max_frac = (profile == PerformanceProfile::Battery) ? 0.20 : 0.0;
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
    const std::string& path, const std::string& target, uint64_t now_ms, uint64_t deadband) noexcept {
    if (path.empty() || target.empty()) return false;

    const std::string live = read_line_trimmed(path);
    if (live.empty()) return false;

    if (oem_map_) {
        if (oem_map_->is_blocked(path, now_ms)) {
            stats_.contended_nodes = oem_map_->active_count(now_ms);
            return false;
        }
        if (!oem_map_->check_cooldown_expired(path, live, now_ms)) {
            stats_.contended_nodes = oem_map_->active_count(now_ms);
            return false;
        }
        stats_.contended_nodes = oem_map_->active_count(now_ms);
    } else {
        const auto cit = contended_paths_.find(path);
        if (cit != contended_paths_.end()) {
            if (now_ms < cit->second.retry_after_ms) return false;
            if (live != cit->second.observed_value) {
                // OEM daemon is still mutating this node; extend cooldown without writing.
                record_contention(path, live, now_ms);
                return false;
            }
            contended_paths_.erase(cit);
            stats_.contended_nodes = static_cast<uint32_t>(contended_paths_.size());
        }
    }

    const auto prev = last_written_.find(path);
    if (prev != last_written_.end() && live != prev->second) {
        record_contention(path, live, now_ms);
        return false;
    }
    if (live == target) {
        last_written_[path] = target;
        return true;
    }
    // Suppress minor PSI/load jitter writes so CFJ4 journal is not fsynced for negligible deltas.
    if (deadband > 0U && prev != last_written_.end()) {
        uint64_t cur_num = 0U, tgt_num = 0U;
        if (parse_u64_exact(live, cur_num) && parse_u64_exact(target, tgt_num)) {
            const uint64_t diff = cur_num > tgt_num ? (cur_num - tgt_num) : (tgt_num - cur_num);
            if (diff <= deadband) return true;
        }
    }

    // BaselineStore::apply_mutation already performs durable write-ahead journaling before writing.
    if (!baseline_.capture(path)) return false;
    if (!baseline_.apply_mutation(path, target)) return false;
    const std::string verify = read_line_trimmed(path);
    if (verify != target) {
        record_contention(path, verify, now_ms);
        return false;
    }
    last_written_[path] = target;
    return true;
}

void ThermalEnvelopeActuator::rollback_managed_node(const std::string& path) noexcept {
    if (path.empty() || !last_written_.contains(path)) return;
    (void)baseline_.rollback(path);
    last_written_.erase(path);
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
    bool enabled,
    const EnvelopeTelemetry& telemetry,
    uint64_t now_ms) noexcept {
    const double scale = std::isfinite(capacity_scale) ? std::clamp(capacity_scale, 0.25, 1.0) : 1.0;
    stats_.last_capacity_scale = scale;

    // Release-on-Calm & Passive Video Playback:
    // When disabled, excluded, calm (Efficiency/ScreenOff without active thermal_guard), or hands-off
    // hardware video playback, unconditionally restore OEM CPU/GPU governor baselines (hispeed_freq,
    // hispeed_load, up_rate_limit_us, kgsl/min_pwrlevel) so cooling idle silicon never holds elevated nodes.
    const bool calm_cool =
        !thermal_guard &&
        (mode == ContextMode::Efficiency || mode == ContextMode::ScreenOff ||
         (mode == ContextMode::Interactive && telemetry.passive_media_only && scale >= 0.95));
    if (!enabled || mode == ContextMode::Excluded || calm_cool) {
        if (!last_written_.empty()) rollback_all_managed();
        return;
    }

    const double frame_us = topo.refresh_hz > 0U ? (1000000.0 / static_cast<double>(topo.refresh_hz)) : 16666.0;
    const double cpu_psi_norm = std::isfinite(telemetry.cpu_psi) ? std::clamp(telemetry.cpu_psi / 100.0, 0.0, 1.0) : 0.0;
    const double gpu_norm = std::isfinite(telemetry.gpu_busy_pct) ? std::clamp(telemetry.gpu_busy_pct / 100.0, 0.0, 1.0) : 0.0;
    const double sched_resp_scale = std::isfinite(telemetry.scheduler_responsiveness_scale)
        ? std::clamp(telemetry.scheduler_responsiveness_scale, 0.70, 1.0)
        : 1.0;
    const double stepdown_lead = std::isfinite(telemetry.thermal_stepdown_lead)
        ? std::clamp(telemetry.thermal_stepdown_lead, 0.0, 0.15)
        : 0.0;
    // Headroom <-> Thermal Envelope coupling:
    // Whenever wider performance headroom is open, shift thermal step-down curve earlier
    // proportionally to thermal pressure so silicon glides smoothly instead of cliff-throttling.
    const double thermal_pressure = thermal_guard ? 1.0 : std::clamp((1.0 - scale) * 4.0, 0.0, 1.0);
    const double effective_scale = std::clamp(scale - (stepdown_lead * (0.25 + 0.75 * thermal_pressure)), 0.25, 1.0);
    const double profile_bias =
        profile == PerformanceProfile::Smoothness ? 1.08 :
        profile == PerformanceProfile::Gaming ? 1.10 :
        profile == PerformanceProfile::Battery ? 0.85 : 1.00;
    const double max_cluster_cap = static_cast<double>(std::max<uint32_t>(topo.big_capacity, 512U));

    // 1. CPU Governor Envelopes (single-owner, silicon-capacity + frame_us + PSI + PID modulated)
    for (const auto& cluster : topo.cpu_clusters) {
        const double cap_ratio = cluster.capacity > 0U
            ? std::clamp(static_cast<double>(cluster.capacity) / max_cluster_cap, 0.25, 1.0)
            : (cluster.role == ClusterRole::Big ? 1.0 : cluster.role == ClusterRole::Mid ? 0.65 : 0.40);

        if (!cluster.hispeed_freq_path.empty() && !cluster.opps_khz.empty()) {
            // Capture OEM factory hispeed_freq once while untouched so warm Interactive
            // scrolling never wastes C*V^2*f power boosting Big cores above factory baseline.
            if (!last_written_.contains(cluster.hispeed_freq_path) &&
                !baseline_hispeed_khz_.contains(cluster.hispeed_freq_path)) {
                uint64_t oem_khz = 0U;
                if (parse_u64_exact(read_line_trimmed(cluster.hispeed_freq_path), oem_khz) && oem_khz > 0U) {
                    baseline_hispeed_khz_[cluster.hispeed_freq_path] = oem_khz;
                }
            }
            // Higher-capacity clusters scale deeper in Efficiency to save power and higher in Burst/Gaming.
            double mode_demand = 0.55;
            if (thermal_guard) {
                mode_demand = 0.56 - (cap_ratio * 0.08);
            } else if (mode == ContextMode::ScreenOff) {
                mode_demand = 0.30;
            } else if (mode == ContextMode::LaunchBurst || mode == ContextMode::Gaming) {
                mode_demand = 0.78 + (cap_ratio * 0.10) + (cpu_psi_norm * 0.08);
            } else if (mode == ContextMode::Interactive) {
                const double warm_big_trim = (effective_scale < 0.94 && cap_ratio >= 0.80)
                    ? std::clamp((0.94 - effective_scale) * 0.75, 0.0, 0.22)
                    : 0.0;
                mode_demand = 0.64 - warm_big_trim + (cpu_psi_norm * 0.10);
            } else { // Efficiency under thermal pacing
                mode_demand = 0.60 - (cap_ratio * 0.12) + (cpu_psi_norm * 0.10);
            }
            const double effective_tier = std::clamp(mode_demand * profile_bias * effective_scale, 0.25, 0.82);
            uint64_t freq = select_opp(cluster.opps_khz, effective_tier);
            if (mode == ContextMode::Interactive &&
                effective_scale < 0.94 &&
                profile != PerformanceProfile::Smoothness) {
                const auto bit = baseline_hispeed_khz_.find(cluster.hispeed_freq_path);
                if (bit != baseline_hispeed_khz_.end() && bit->second > 0U && freq > bit->second) {
                    freq = bit->second;
                }
            }
            if (freq > 0U) (void)write_managed_node(cluster.hispeed_freq_path, std::to_string(freq), now_ms);
        }

        if (!cluster.hispeed_load_path.empty()) {
            // Lower load threshold when latency-sensitive or PSI rises; higher when thermal-guarded or saving power.
            double target_load = 88.0;
            if (thermal_guard || mode == ContextMode::ScreenOff) {
                target_load = 95.0;
            } else if (mode == ContextMode::LaunchBurst || mode == ContextMode::Gaming) {
                target_load = 78.0 - (cpu_psi_norm * 8.0);
            } else if (mode == ContextMode::Interactive) {
                target_load = 84.0 - (cpu_psi_norm * 8.0);
            } else {
                target_load = 90.0 - (cpu_psi_norm * 6.0);
            }
            target_load = target_load / std::clamp(profile_bias, 0.90, 1.08);
            // Smooth thermal backoff raises hispeed_load gradually as PID scale drops.
            target_load += (1.0 - effective_scale) * 14.0;
            const uint32_t load = static_cast<uint32_t>(std::clamp(std::lround(target_load), 70L, 96L));
            (void)write_managed_node(cluster.hispeed_load_path, std::to_string(load), now_ms, 3U);
        }

        const bool walt_zero_up = (cluster.governor == "walt" && cluster.has_baseline_up_rate_limit &&
                                   cluster.baseline_up_rate_limit_us == 0U);
        if (!cluster.up_rate_limit_path.empty() && (!walt_zero_up || thermal_guard)) {
            // Derived continuously from frame_us (e.g. 0.04..0.25 frames), PID scale, and
            // Rendering <-> Scheduler responsiveness coupling (tighter when render_pressure rises).
            // Never slow down Qualcomm WALT's instant 0 us ramp-up while cool.
            const double up_frames = thermal_guard ? 0.25 :
                (mode == ContextMode::LaunchBurst || mode == ContextMode::Interactive) ? 0.04 :
                mode == ContextMode::Gaming ? 0.06 : 0.16;
            const double coupled_up_frames = thermal_guard ? up_frames : (up_frames * sched_resp_scale);
            const uint64_t up_us = static_cast<uint64_t>(
                std::clamp(std::lround((frame_us * coupled_up_frames) / std::clamp(scale, 0.50, 1.0)), 250L, 6000L));
            (void)write_managed_node(cluster.up_rate_limit_path, std::to_string(up_us), now_ms, 100U);
        }

        const bool walt_zero_down = (cluster.governor == "walt" && cluster.has_baseline_down_rate_limit &&
                                     cluster.baseline_down_rate_limit_us == 0U);
        if (!cluster.down_rate_limit_path.empty() && !walt_zero_down) {
            // Hold frequencies cleanly across frame boundaries without overriding WALT's internal 0 us decay.
            const double down_frames = (thermal_guard || mode == ContextMode::ScreenOff) ? 0.35 :
                (mode == ContextMode::LaunchBurst || mode == ContextMode::Gaming) ? (1.10 + gpu_norm * 0.30) :
                mode == ContextMode::Interactive ? (0.85 + cpu_psi_norm * 0.25) : 0.50;
            const uint64_t down_us = static_cast<uint64_t>(
                std::clamp(std::lround(frame_us * down_frames * effective_scale), 3000L, 18000L));
            (void)write_managed_node(cluster.down_rate_limit_path, std::to_string(down_us), now_ms, 1500U);
        }
    }

    // 2. GPU Idle Timer & Smooth Thermal Frequency Envelope (OEM-Collaborative)
    if (topo.gpu.available) {
        if (!topo.gpu.idle_timer_path.empty()) {
            const double frames = (thermal_guard || mode == ContextMode::ScreenOff) ? 4.0 :
                mode == ContextMode::Gaming ? (9.0 + gpu_norm * 4.0) :
                (mode == ContextMode::LaunchBurst || mode == ContextMode::Interactive) ? (7.0 + gpu_norm * 3.0) : 5.0;
            const uint64_t idle_ms = static_cast<uint64_t>(
                std::clamp(std::lround((frame_us / 1000.0) * frames * effective_scale), 40L, 180L));
            (void)write_managed_node(topo.gpu.idle_timer_path, std::to_string(idle_ms), now_ms, 15U);
        }

        if (topo.gpu.is_kgsl &&
            !topo.gpu.kgsl_max_pwrlevel_path.empty() &&
            !topo.gpu.kgsl_min_pwrlevel_path.empty()) {
            const auto [target_max, target_min] = compute_kgsl_levels(
                topo.gpu.kgsl_num_pwrlevels, mode, profile, effective_scale, thermal_guard);
            // Collaborate with OEM GPU daemon (perfd): only touch min_pwrlevel during active
            // Gaming or thermal pacing; otherwise leave OEM min_pwrlevel untouched to avoid contention.
            const bool manage_min = (mode == ContextMode::Gaming || thermal_guard || effective_scale < 0.95);
            if (!manage_min) {
                rollback_managed_node(topo.gpu.kgsl_min_pwrlevel_path);
                if (target_max == 0U) {
                    rollback_managed_node(topo.gpu.kgsl_max_pwrlevel_path);
                } else {
                    (void)write_managed_node(topo.gpu.kgsl_max_pwrlevel_path, std::to_string(target_max), now_ms);
                }
            } else {
                uint64_t live_min = target_min;
                (void)parse_u64_exact(read_line_trimmed(topo.gpu.kgsl_min_pwrlevel_path), live_min);
                // Order writes from the live interval so KGSL never clamps one against the stale peer.
                if (target_max <= live_min) {
                    (void)write_managed_node(topo.gpu.kgsl_max_pwrlevel_path, std::to_string(target_max), now_ms);
                    (void)write_managed_node(topo.gpu.kgsl_min_pwrlevel_path, std::to_string(target_min), now_ms);
                } else {
                    (void)write_managed_node(topo.gpu.kgsl_min_pwrlevel_path, std::to_string(target_min), now_ms);
                    (void)write_managed_node(topo.gpu.kgsl_max_pwrlevel_path, std::to_string(target_max), now_ms);
                }
            }
        } else if (!topo.gpu.devfreq_max_freq_path.empty() && !topo.gpu.devfreq_opps_hz.empty()) {
            if (!thermal_guard && effective_scale >= 0.95 &&
                !(mode == ContextMode::Efficiency && profile == PerformanceProfile::Battery)) {
                rollback_managed_node(topo.gpu.devfreq_max_freq_path);
            } else {
                double base_tier = 1.00;
                if (thermal_guard) base_tier = 0.65;
                else if (mode == ContextMode::ScreenOff) base_tier = 0.35;
                else if (mode == ContextMode::Efficiency && profile == PerformanceProfile::Battery) base_tier = 0.85;
                const double tier = std::clamp(base_tier * effective_scale, 0.30, 1.0);
                const uint64_t freq = select_opp(topo.gpu.devfreq_opps_hz, tier);
                if (freq > 0U) (void)write_managed_node(topo.gpu.devfreq_max_freq_path, std::to_string(freq), now_ms);
            }
        }
    }

    stats_.managed_nodes = static_cast<uint32_t>(last_written_.size());
}

} // namespace coreflow::v5
