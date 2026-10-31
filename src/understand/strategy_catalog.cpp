// @coreflow-platform: any
#include "coreflow/strategy_catalog.hpp"
#include "coreflow/ascii.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include "coreflow/integer_parse.hpp"

namespace coreflow {
namespace {

bool containsToken(std::string_view haystack, std::string_view needle) noexcept {
    return haystack.find(needle) != std::string_view::npos;
}

bool endsWithToken(std::string_view value, std::string_view suffix) noexcept {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

using ascii::equalsIgnoreCase;
using ascii::containsIgnoreCase;

bool isBackgroundCgroupPath(std::string_view path) noexcept {
    return containsToken(path, "/background/") ||
           containsToken(path, "/system-background/") ||
           endsWithToken(path, "/background") ||
           endsWithToken(path, "/system-background");
}

bool isForegroundCgroupPath(std::string_view path) noexcept {
    return containsToken(path, "/top-app/") ||
           containsToken(path, "/foreground/") ||
           endsWithToken(path, "/top-app") ||
           endsWithToken(path, "/foreground");
}

std::size_t countCpuListSpan(std::string_view raw) noexcept {
    raw = integer_parse::trim(raw);
    std::size_t count = 0;
    std::uint64_t previous = 0;
    bool have_previous = false;
    while (!raw.empty()) {
        const auto comma = raw.find(',');
        const auto range = integer_parse::trim(raw.substr(0, comma));
        const auto dash = range.find('-');
        std::uint64_t first = 0, last = 0;
        if (!integer_parse::parseUint64(range.substr(0, dash), first)) return 0;
        last = first;
        if (dash != std::string_view::npos &&
            !integer_parse::parseUint64(range.substr(dash + 1U), last)) return 0;
        if (last < first || (have_previous && first <= previous)) return 0;
        const auto width = last - first;
        if (width >= std::numeric_limits<std::size_t>::max() - count) return 0;
        count += static_cast<std::size_t>(width) + 1U;
        previous = last;
        have_previous = true;
        if (comma == std::string_view::npos) break;
        raw = integer_parse::trim(raw.substr(comma + 1U));
        if (raw.empty()) return 0;
    }
    return count;
}

} // namespace

const char* strategyProfileName(StrategyProfile profile) noexcept {
    switch (profile) {
        case StrategyProfile::IdleConservation: return "IDLE_CONSERVATION";
        case StrategyProfile::DailyMultitasking: return "DAILY_MULTITASKING";
        case StrategyProfile::SustainedGaming: return "SUSTAINED_GAMING";
        case StrategyProfile::MemoryRelief: return "MEMORY_RELIEF";
        case StrategyProfile::ThermalStabilization: return "THERMAL_STABILIZATION";
    }
    return "IDLE_CONSERVATION";
}

std::string_view StrategyCatalog::extractActiveToken(std::string_view raw) noexcept {
    const auto open_bracket = raw.find('[');
    if (open_bracket != std::string_view::npos) {
        const auto close_bracket = raw.find(']', open_bracket + 1U);
        if (close_bracket != std::string_view::npos && close_bracket > open_bracket + 1U) {
            return raw.substr(open_bracket + 1U, close_bracket - open_bracket - 1U);
        }
    }
    std::size_t start = 0;
    while (start < raw.size() && std::isspace(static_cast<unsigned char>(raw[start]))) {
        ++start;
    }
    std::size_t end = start;
    while (end < raw.size() && !std::isspace(static_cast<unsigned char>(raw[end]))) {
        ++end;
    }
    return raw.substr(start, end - start);
}

IoSchedulerPosture StrategyCatalog::classifyIoScheduler(std::string_view token) noexcept {
    if (token.empty()) return IoSchedulerPosture::Unknown;
    if (equalsIgnoreCase(token, "none") || equalsIgnoreCase(token, "noop")) {
        return IoSchedulerPosture::LowOverheadDirect;
    }
    if (containsIgnoreCase(token, "deadline") || containsIgnoreCase(token, "kyber")) {
        return IoSchedulerPosture::DeadlineLatency;
    }
    return IoSchedulerPosture::FairThroughput;
}

CpuGovernorPosture StrategyCatalog::classifyCpuGovernor(std::string_view token) noexcept {
    if (token.empty()) return CpuGovernorPosture::Unknown;
    if (containsIgnoreCase(token, "schedutil") || containsIgnoreCase(token, "walt") ||
        containsIgnoreCase(token, "sugov") || containsIgnoreCase(token, "interactive")) {
        return CpuGovernorPosture::EnergyAwareDynamic;
    }
    if (equalsIgnoreCase(token, "performance")) {
        return CpuGovernorPosture::PerformancePinned;
    }
    if (equalsIgnoreCase(token, "powersave") || equalsIgnoreCase(token, "conservative")) {
        return CpuGovernorPosture::PowerSavePinned;
    }
    return CpuGovernorPosture::Unknown;
}

CouplingSnapshot StrategyCatalog::analyze(
    const EnvironmentCapabilityMatrix& matrix,
    const std::unordered_map<std::string, std::string>& baselines,
    const EffectContext& ctx
) noexcept {
    CouplingSnapshot snap{};

    const bool thermal_guard =
        ctx.state == RuntimeState::ThermalGuard ||
        (ctx.thermal_available && ctx.thermal_c >= 52.0);
    snap.warm_equilibrium =
        !thermal_guard &&
        ((ctx.thermal_available && ctx.thermal_c >= 48.0 && ctx.thermal_c < 52.0) ||
         ctx.state == RuntimeState::Warming);

    const bool gaming_signal =
        ctx.gaming_active ||
        (ctx.gpu_busy_available && ctx.gpu_busy_ratio >= 0.45) ||
        (ctx.frame_jank_available && ctx.frame_jank_rate >= 0.04 &&
         ctx.cpu_utilization_available && ctx.cpu_utilization >= 0.35);

    const bool memory_pressure =
        ctx.memory_stall_detected ||
        (ctx.memory_available && ctx.mem_available_ratio < 0.16) ||
        (ctx.psi_available && ctx.psi_memory >= 0.12);

    if (thermal_guard) {
        snap.profile = StrategyProfile::ThermalStabilization;
    } else if (gaming_signal) {
        snap.profile = StrategyProfile::SustainedGaming;
    } else if (memory_pressure) {
        snap.profile = StrategyProfile::MemoryRelief;
    } else if (ctx.state == RuntimeState::Idle &&
               (!ctx.cpu_utilization_available || ctx.cpu_utilization < 0.15)) {
        snap.profile = StrategyProfile::IdleConservation;
    } else {
        snap.profile = StrategyProfile::DailyMultitasking;
    }

    for (const auto& res : matrix.resources) {
        if (!isTunableResourceDomain(res.domain)) continue;
        if (res.domain == ResourceDomain::Io &&
            snap.io_posture == IoSchedulerPosture::Unknown &&
            endsWithToken(res.path, "/queue/scheduler")) {
            const auto it = baselines.find(res.path);
            if (it != baselines.end()) {
                snap.io_posture = classifyIoScheduler(extractActiveToken(it->second));
            }
        } else if (res.domain == ResourceDomain::CpuFreq &&
                   snap.cpu_posture == CpuGovernorPosture::Unknown &&
                   endsWithToken(res.path, "/scaling_governor")) {
            const auto it = baselines.find(res.path);
            if (it != baselines.end()) {
                snap.cpu_posture = classifyCpuGovernor(extractActiveToken(it->second));
            }
        }
        if (res.domain == ResourceDomain::Memory) {
            if (endsWithToken(res.path, "/vm/swappiness")) {
                const auto it = baselines.find(res.path);
                if (it != baselines.end()) {
                    std::uint64_t value = 0;
                    if (integer_parse::parseUint64(it->second, value) && value >= 100U && value <= 200U)
                        snap.zram_high_swappiness = true;
                }
            } else if (endsWithToken(res.path, "/vm/page-cluster")) {
                const auto it = baselines.find(res.path);
                if (it != baselines.end() && integer_parse::trim(it->second) == "0") {
                    snap.zram_zero_page_cluster = true;
                }
            }
        }
        const auto base_it = baselines.find(res.path);
        if (base_it != baselines.end() && !snap.oem_performance_capped) {
            const std::string_view base_tok = extractActiveToken(base_it->second);
            for (const auto& vv : res.vendor_values) {
                if (vv.profile == VendorProfile::Performance &&
                    !equalsIgnoreCase(extractActiveToken(vv.value), base_tok)) {
                    snap.oem_performance_capped = true;
                    break;
                }
            }
            if (!snap.oem_performance_capped &&
                (endsWithToken(res.path, "/walt_target_load") ||
                 endsWithToken(res.path, "/hispeed_load"))) {
                std::uint64_t value = 0;
                if (integer_parse::parseUint64(base_it->second, value) && value >= 85U && value <= 100U)
                    snap.oem_performance_capped = true;
            }
        }
        if (!res.mutation_ready && !(res.domain == ResourceDomain::CpuFreq &&
                                     endsWithToken(res.path, "/scaling_governor"))) {
            continue;
        }
        if (containsToken(res.path, "/net/ipv4/tcp_")) {
            snap.has_network_domain = true;
        }
        switch (res.domain) {
            case ResourceDomain::Io:
                snap.has_io_domain = true;
                break;
            case ResourceDomain::CpuFreq:
            case ResourceDomain::Scheduler:
                snap.has_cpufreq_domain = true;
                break;
            case ResourceDomain::UClamp:
                snap.has_uclamp_domain = true;
                break;
            case ResourceDomain::CpuSet:
                snap.has_cpuset_domain = true;
                break;
            case ResourceDomain::Gpu:
                snap.has_gpu_domain = true;
                break;
            case ResourceDomain::Memory:
                snap.has_memory_domain = true;
                break;
            default:
                break;
        }
    }

    snap.active_domain_count =
        (snap.has_io_domain ? 1U : 0U) +
        (snap.has_cpufreq_domain ? 1U : 0U) +
        (snap.has_uclamp_domain ? 1U : 0U) +
        (snap.has_cpuset_domain ? 1U : 0U) +
        (snap.has_gpu_domain ? 1U : 0U) +
        (snap.has_memory_domain ? 1U : 0U) +
        (snap.has_network_domain ? 1U : 0U);

    return snap;
}

StrategyAdjustment StrategyCatalog::evaluateCandidate(
    const ResourceCapability& capability,
    std::string_view baseline_value,
    std::string_view candidate_value,
    EffectDirection direction,
    const CouplingSnapshot& snapshot,
    const EffectContext& ctx
) noexcept {
    StrategyAdjustment adj{};

    // Preserve deterministic single-node unit behavior when no cross-domain
    // anchors or explicit gaming/GPU/frame signals exist.
    const bool has_live_render_signal =
        ctx.gaming_active ||
        (ctx.gpu_busy_available && ctx.gpu_busy_ratio > 0.0) ||
        (ctx.frame_jank_available && ctx.frame_jank_rate > 0.0);
    const bool has_coupled_anchors =
        snapshot.active_domain_count >= 2U &&
        (snapshot.io_posture != IoSchedulerPosture::Unknown ||
          snapshot.cpu_posture != CpuGovernorPosture::Unknown ||
          snapshot.warm_equilibrium ||
          snapshot.zram_high_swappiness ||
          snapshot.oem_performance_capped ||
          snapshot.profile == StrategyProfile::MemoryRelief);
    if (!has_coupled_anchors && !has_live_render_signal) {
        return adj;
    }

    const std::string_view path = capability.path;
    const std::string_view cand_lower = candidate_value;

    // 1. Warm Equilibrium (48-52C) & Sustained Gaming background containment:
    //    Trim background uclamp.max / cpuset span first so foreground clocks and
    //    frame pacing stay smooth without triggering Power HAL thermal throttling.
    if (snapshot.warm_equilibrium || snapshot.profile == StrategyProfile::SustainedGaming) {
        if (capability.domain == ResourceDomain::UClamp &&
            isBackgroundCgroupPath(path) &&
            endsWithToken(path, "uclamp.max") &&
            direction == EffectDirection::Decrease) {
            adj.benefit_delta += snapshot.warm_equilibrium ? 0.14 : 0.09;
            adj.risk_delta -= 0.05;
            adj.coupling_tag = "coupled:bg_uclamp_thermal_headroom";
            return adj;
        }
        if (capability.domain == ResourceDomain::CpuSet &&
            isBackgroundCgroupPath(path) &&
            endsWithToken(path, "/cpus")) {
            const std::size_t base_span = countCpuListSpan(baseline_value);
            const std::size_t cand_span = countCpuListSpan(candidate_value);
            if (cand_span > 0U && base_span > cand_span) {
                adj.benefit_delta += snapshot.warm_equilibrium ? 0.12 : 0.07;
                adj.risk_delta -= 0.04;
                adj.coupling_tag = "coupled:bg_cpuset_containment";
                return adj;
            }
        }
    }

    // 2. Sustained Gaming GPU / Vulkan / Frame-Pacing pipeline unlock:
    //    Maximize GPU devfreq / KGSL / Mali driver headroom and foreground
    //    scheduler responsiveness while preventing inter-frame clock drops.
    if (snapshot.profile == StrategyProfile::SustainedGaming) {
        if (capability.domain == ResourceDomain::Gpu) {
            const bool kgsl_pwrlevel_step =
                endsWithToken(path, "/min_pwrlevel") && direction == EffectDirection::Decrease;
            const bool freq_floor_step =
                !endsWithToken(path, "/min_pwrlevel") && direction == EffectDirection::Increase;
            const bool perf_governor_select =
                direction == EffectDirection::Select &&
                (containsIgnoreCase(cand_lower, "msm-adreno-tz") ||
                 containsIgnoreCase(cand_lower, "simple_ondemand") ||
                 containsIgnoreCase(cand_lower, "bw_hwmon") ||
                 containsIgnoreCase(cand_lower, "performance"));
            if ((kgsl_pwrlevel_step || freq_floor_step || perf_governor_select) &&
                !snapshot.warm_equilibrium) {
                adj.benefit_delta += 0.14;
                adj.risk_delta -= 0.04;
                adj.coupling_tag = "coupled:gpu_render_headroom";
                return adj;
            }
        }

        if (capability.domain == ResourceDomain::UClamp &&
            isForegroundCgroupPath(path) &&
            endsWithToken(path, "uclamp.min") &&
            direction == EffectDirection::Increase &&
            snapshot.cpu_posture == CpuGovernorPosture::EnergyAwareDynamic) {
            adj.benefit_delta += 0.12;
            adj.risk_delta -= 0.04;
            adj.coupling_tag = "coupled:schedutil_fg_uclamp_frame_lock";
            return adj;
        }

        if (capability.domain == ResourceDomain::CpuFreq ||
            capability.domain == ResourceDomain::Scheduler) {
            if (endsWithToken(path, "/down_rate_limit_us") &&
                direction == EffectDirection::Increase) {
                adj.benefit_delta += 0.10;
                adj.risk_delta -= 0.03;
                adj.coupling_tag = "coupled:cpufreq_anti_microstutter_hold";
                return adj;
            }
            if (endsWithToken(path, "/up_rate_limit_us") &&
                direction == EffectDirection::Decrease) {
                adj.benefit_delta += 0.09;
                adj.risk_delta -= 0.03;
                adj.coupling_tag = "coupled:cpufreq_fast_frame_ramp";
                return adj;
            }
            if ((endsWithToken(path, "/walt_target_load") ||
                 endsWithToken(path, "/target_loads") ||
                 endsWithToken(path, "/hispeed_load") ||
                 endsWithToken(path, "/sched_upmigrate")) &&
                direction == EffectDirection::Decrease &&
                !snapshot.warm_equilibrium) {
                adj.benefit_delta += 0.11;
                adj.risk_delta -= 0.03;
                adj.coupling_tag = "coupled:silicon_walt_headroom_unlock";
                return adj;
            }
            if (endsWithToken(path, "/hispeed_freq") &&
                direction == EffectDirection::Increase &&
                !snapshot.warm_equilibrium) {
                adj.benefit_delta += 0.10;
                adj.risk_delta -= 0.03;
                adj.coupling_tag = "coupled:silicon_hispeed_opp_unlock";
                return adj;
            }
        }
    }

    // 3. I/O Scheduler <-> Block Queue & CPU/Memory Cross-Domain Coupling:
    //    When the active I/O scheduler changes (e.g., vendor default -> none,
    //    mq-deadline, kyber, or bfq), align companion queue and governor knobs.
    if (capability.domain == ResourceDomain::Io) {
        if (endsWithToken(path, "/queue/scheduler") && direction == EffectDirection::Select) {
            const IoSchedulerPosture target_posture = classifyIoScheduler(cand_lower);
            if (snapshot.profile == StrategyProfile::SustainedGaming &&
                (target_posture == IoSchedulerPosture::LowOverheadDirect ||
                 target_posture == IoSchedulerPosture::DeadlineLatency)) {
                adj.benefit_delta += 0.08;
                adj.risk_delta -= 0.02;
                adj.coupling_tag = "coupled:io_scheduler_low_latency";
                return adj;
            }
            if (snapshot.profile == StrategyProfile::DailyMultitasking &&
                target_posture == IoSchedulerPosture::DeadlineLatency) {
                adj.benefit_delta += 0.07;
                adj.risk_delta -= 0.02;
                adj.coupling_tag = "coupled:io_scheduler_daily_deadline";
                return adj;
            }
        }

        if (snapshot.io_posture == IoSchedulerPosture::LowOverheadDirect) {
            if ((endsWithToken(path, "/rq_affinity") && direction == EffectDirection::Increase) ||
                (endsWithToken(path, "/wbt_lat_usec") && direction == EffectDirection::Decrease) ||
                (endsWithToken(path, "/nr_requests") && direction == EffectDirection::Decrease)) {
                adj.benefit_delta += 0.08;
                adj.risk_delta -= 0.02;
                adj.coupling_tag = "coupled:direct_io_queue_alignment";
                return adj;
            }
        } else if (snapshot.io_posture == IoSchedulerPosture::DeadlineLatency) {
            if ((endsWithToken(path, "/read_ahead_kb") && direction == EffectDirection::Increase &&
                 ctx.io_activity_available && ctx.io_read_kb_per_sec >= 256.0) ||
                (endsWithToken(path, "/fifo_batch") && direction == EffectDirection::Increase) ||
                (endsWithToken(path, "/read_expire") && direction == EffectDirection::Decrease)) {
                adj.benefit_delta += 0.08;
                adj.risk_delta -= 0.02;
                adj.coupling_tag = "coupled:deadline_io_companion";
                return adj;
            }
        } else if (snapshot.io_posture == IoSchedulerPosture::FairThroughput) {
            if (endsWithToken(path, "/read_ahead_kb") && direction == EffectDirection::Increase) {
                adj.benefit_delta += 0.07;
                adj.risk_delta -= 0.02;
                adj.coupling_tag = "coupled:fair_io_readahead";
                return adj;
            }
        }
    }

    // 4. MemoryRelief & ZRAM-Aware VM Coupling (Linux 5.8+ 0..200 swappiness):
    //    Modern Android kernels with in-RAM ZRAM ship OEM vm.swappiness at
    //    100..160 (commonly 120) and vm.page-cluster at 0. Respect that factory
    //    ZRAM posture instead of assuming legacy desktop Linux swappiness=60:
    //    avoid blindly inflating an already high ZRAM swappiness (>=100) or
    //    enabling multi-page swap readahead on random compressed RAM pages.
    if (capability.domain == ResourceDomain::Memory) {
        if (endsWithToken(path, "/vm/swappiness")) {
            if (snapshot.zram_high_swappiness && direction == EffectDirection::Increase) {
                adj.benefit_delta -= 0.04;
                adj.risk_delta += 0.06;
                adj.coupling_tag = "coupled:zram_oem_swappiness_saturated";
                return adj;
            }
            if (!snapshot.zram_high_swappiness &&
                snapshot.profile == StrategyProfile::MemoryRelief &&
                direction == EffectDirection::Increase) {
                adj.benefit_delta += 0.07;
                adj.risk_delta -= 0.02;
                adj.coupling_tag = "coupled:zram_anon_reclaim_relief";
                return adj;
            }
        }
        if (endsWithToken(path, "/vm/page-cluster")) {
            if (snapshot.zram_high_swappiness && direction == EffectDirection::Increase) {
                adj.benefit_delta -= 0.05;
                adj.risk_delta += 0.08;
                adj.coupling_tag = "coupled:zram_random_swap_no_readahead";
                return adj;
            }
            if (snapshot.zram_high_swappiness && direction == EffectDirection::Decrease) {
                adj.benefit_delta += 0.08;
                adj.risk_delta -= 0.03;
                adj.coupling_tag = "coupled:zram_single_page_latency";
                return adj;
            }
        }
        if (snapshot.profile == StrategyProfile::MemoryRelief) {
            if ((endsWithToken(path, "/vm/vfs_cache_pressure") ||
                 endsWithToken(path, "/vm/watermark_scale_factor")) &&
                direction == EffectDirection::Increase) {
                adj.benefit_delta += 0.08;
                adj.risk_delta -= 0.02;
                adj.coupling_tag = "coupled:memory_relief_reclaim_headroom";
                return adj;
            }
        }
    }

    if (snapshot.profile == StrategyProfile::MemoryRelief &&
        capability.domain == ResourceDomain::UClamp &&
        isBackgroundCgroupPath(path) &&
        endsWithToken(path, "uclamp.max") &&
        direction == EffectDirection::Decrease) {
        adj.benefit_delta += 0.08;
        adj.risk_delta -= 0.03;
        adj.coupling_tag = "coupled:memory_relief_bg_containment";
        return adj;
    }

    // 5. Daily Multitasking CPU Governor <-> UClamp & Memory coordination:
    if (snapshot.profile == StrategyProfile::DailyMultitasking) {
        if (capability.domain == ResourceDomain::CpuFreq &&
            endsWithToken(path, "/scaling_governor") &&
            direction == EffectDirection::Select &&
            classifyCpuGovernor(cand_lower) == CpuGovernorPosture::EnergyAwareDynamic) {
            adj.benefit_delta += 0.09;
            adj.risk_delta -= 0.03;
            adj.coupling_tag = "coupled:powerhal_schedutil_alignment";
            return adj;
        }
        if (capability.domain == ResourceDomain::UClamp &&
            isForegroundCgroupPath(path) &&
            endsWithToken(path, "uclamp.min") &&
            direction == EffectDirection::Increase &&
            snapshot.cpu_posture == CpuGovernorPosture::EnergyAwareDynamic) {
            adj.benefit_delta += 0.07;
            adj.risk_delta -= 0.02;
            adj.coupling_tag = "coupled:daily_fg_uclamp_responsiveness";
            return adj;
        }
        if ((capability.domain == ResourceDomain::CpuFreq ||
             capability.domain == ResourceDomain::Scheduler) &&
            endsWithToken(path, "/up_rate_limit_us") &&
            direction == EffectDirection::Decrease &&
            snapshot.cpu_posture == CpuGovernorPosture::EnergyAwareDynamic) {
            adj.benefit_delta += 0.06;
            adj.risk_delta -= 0.02;
            adj.coupling_tag = "coupled:daily_governor_ramp";
            return adj;
        }
        if ((capability.domain == ResourceDomain::CpuFreq ||
             capability.domain == ResourceDomain::Scheduler) &&
            (endsWithToken(path, "/walt_target_load") ||
             endsWithToken(path, "/hispeed_load")) &&
            direction == EffectDirection::Decrease &&
            snapshot.oem_performance_capped &&
            !snapshot.warm_equilibrium) {
            adj.benefit_delta += 0.07;
            adj.risk_delta -= 0.02;
            adj.coupling_tag = "coupled:oem_walt_cap_relief";
            return adj;
        }
    }

    // 6. Low-Latency Network & OEM Performance Cap Unlock (Encore / Silicon Envelope):
    if ((snapshot.profile == StrategyProfile::SustainedGaming ||
         snapshot.profile == StrategyProfile::DailyMultitasking) &&
        !snapshot.warm_equilibrium) {
        if (endsWithToken(path, "/net/ipv4/tcp_congestion_control") &&
            direction == EffectDirection::Select &&
            (equalsIgnoreCase(cand_lower, "bbr") || equalsIgnoreCase(cand_lower, "bbr2") || equalsIgnoreCase(cand_lower, "westwood"))) {
            adj.benefit_delta += 0.08;
            adj.risk_delta -= 0.02;
            adj.coupling_tag = "coupled:tcp_low_latency_congestion";
            return adj;
        }
        if (snapshot.oem_performance_capped) {
            for (const auto& vv : capability.vendor_values) {
                if (vv.profile == VendorProfile::Performance &&
                    equalsIgnoreCase(vv.value, cand_lower) &&
                    !equalsIgnoreCase(baseline_value, cand_lower)) {
                    adj.benefit_delta += 0.09;
                    adj.risk_delta -= 0.03;
                    adj.coupling_tag = "coupled:oem_perf_cap_unlock";
                    return adj;
                }
            }
        }
    }

    return adj;
}

} // namespace coreflow
