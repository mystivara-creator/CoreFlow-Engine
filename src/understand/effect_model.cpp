#include "coreflow/effect_model.hpp"
#include "coreflow/ecosystem_knowledge.hpp"
#include "coreflow/fatal_surface.hpp"
#include "coreflow/pressure_limits.hpp"
#include "coreflow/strategy_catalog.hpp"
#include "coreflow/integer_parse.hpp"
#include <charconv>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string_view>

namespace coreflow {
namespace {

// Gates applied to every derived candidate.
constexpr double kMinBenefit = 0.10;
constexpr double kMaxRisk = 0.58;
constexpr double kMinConfidence = 0.50;

} // namespace

std::string EffectModel::trim(std::string value) noexcept {
    while (!value.empty() &&
           (value.back() == ' ' || value.back() == '\t' || value.back() == '\r' ||
            value.back() == '\n')) {
        value.pop_back();
    }
    std::size_t first = 0;
    while (first < value.size() &&
           (value[first] == ' ' || value[first] == '\t')) {
        ++first;
    }
    if (first != 0) value.erase(0, first);
    return value;
}

bool EffectModel::parseInteger(const std::string& value, long long& out) noexcept {
    return integer_parse::parseSignedDecimal(value, out);
}

double EffectModel::clamp01(double v) noexcept {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

const char* candidateSourceName(CandidateSource source) noexcept {
    switch (source) {
        case CandidateSource::None:              return "none";
        case CandidateSource::VendorEfficiency:  return "vendor-efficiency";
        case CandidateSource::VendorPerformance: return "vendor-performance";
        case CandidateSource::VendorDefault:     return "vendor-default";
        case CandidateSource::KernelOption:      return "kernel-option";
        case CandidateSource::OrderedScale:      return "ordered-scale";
        case CandidateSource::BoundedStep:       return "bounded-step";
    }
    return "none";
}

namespace {

struct Situation {
    bool thermal{false};
    bool warm_equilibrium{false};
    bool memory{false};
    bool elevated{false};
    bool calm_explore{false};
    bool io_pressure{false};
    bool throttling{false};
    // 0..1 continuous severity derived from PSI / stalls / thermal — used to
    // scale benefit instead of fixed score tables.
    double severity{0.0};
    bool stressed() const noexcept { return thermal || memory || throttling; }
};

Situation classify(const EffectContext& ctx) noexcept {
    Situation s;
    // Align thermal stress with ContextEngine thermal_headroom (< 52°C) so warm
    // sustained gaming / multitasking (48..51.9°C) stays eligible for coupled
    // fine-tuning without triggering a stress veto.
    s.thermal = ctx.thermal_available &&
                (ctx.state == RuntimeState::ThermalGuard || ctx.thermal_c >= 52.0);
    s.warm_equilibrium = !s.thermal &&
                         ((ctx.thermal_available && ctx.thermal_c >= 48.0) ||
                          ctx.state == RuntimeState::Warming);
    s.memory = ctx.state == RuntimeState::Pressure ||
               (ctx.memory_available && ctx.mem_available_ratio < 0.15) ||
               ctx.memory_stall_detected;
    s.io_pressure =
        (ctx.psi_available &&
         ctx.psi_io >= pressure_limits::kIoPsiSomeAvg10Enter) ||
        (ctx.io_latency_available &&
         ctx.io_avg_latency_ms >= pressure_limits::kIoLatencyEnterMs);
    const bool cpu_or_load_elevated =
        (ctx.cpu_utilization_available && ctx.cpu_utilization >= 0.70) ||
        ctx.load1 >= 1.50 ||
        ctx.gaming_active ||
        (ctx.gpu_busy_available && ctx.gpu_busy_ratio >= 0.45);
    // AdaptivePolicy can enter Elevated due to I/O alone. Do not interpret that
    // state as a general CPU-load signal unless CPU/load/GPU telemetry agrees.
    s.elevated = cpu_or_load_elevated ||
                 (ctx.state == RuntimeState::Elevated && !s.io_pressure);
    s.calm_explore = (ctx.state == RuntimeState::Normal ||
                      (ctx.state == RuntimeState::Warming && !s.thermal)) &&
                     ctx.intervention >= InterventionLevel::Low &&
                     ctx.sample_confidence >= 0.60;
    s.throttling = ctx.throttling_detected;

    double sev = 0.0;
    if (ctx.psi_available) {
        sev = std::max({ctx.psi_cpu, ctx.psi_memory, ctx.psi_io * 0.85});
    }
    if (ctx.throttle_ratio > sev) sev = ctx.throttle_ratio;
    if (ctx.memory_stall_score > sev) sev = ctx.memory_stall_score;
    if (s.thermal && ctx.thermal_available) {
        const double t = std::clamp((ctx.thermal_c - 50.0) / 10.0, 0.0, 1.0);
        sev = std::max(sev, t);
    } else if (s.warm_equilibrium && ctx.thermal_available) {
        const double t = std::clamp((ctx.thermal_c - 48.0) / 16.0, 0.0, 0.25);
        sev = std::max(sev, t);
    }
    if (s.memory && ctx.mem_available_ratio < 0.20) {
        sev = std::max(sev, 1.0 - ctx.mem_available_ratio);
    }
    s.severity = std::clamp(sev, 0.0, 1.0);
    return s;
}

// Outcome-driven base benefit: severity need + experience prior.
// No fixed "0.44 for efficiency" table — device situation and measured
// history set the numbers.
double outcomeBenefit(const Situation& s, bool stabilizing, CandidateSource source,
                      const EffectContext& ctx) noexcept {
    double need = s.severity;
    if (s.elevated && !s.stressed()) need = std::max(need, 0.25);
    if (s.calm_explore) need = std::max(need, 0.12);
    if (s.io_pressure) need = std::max(need, 0.18);

    double source_bias = 0.0;
    switch (source) {
        case CandidateSource::VendorEfficiency:  source_bias = stabilizing ? 0.08 : 0.02; break;
        case CandidateSource::VendorPerformance: source_bias = s.elevated ? 0.06 : 0.0; break;
        case CandidateSource::VendorDefault:     source_bias = 0.04; break;
        case CandidateSource::OrderedScale:      source_bias = 0.03; break;
        case CandidateSource::KernelOption:      source_bias = 0.02; break;
        case CandidateSource::BoundedStep:       source_bias = 0.01; break;
        default: break;
    }

    double benefit = 0.10 + need * 0.35 + source_bias;
    if (stabilizing && s.stressed()) benefit += 0.08;

    if (ctx.has_experience && ctx.experience_confidence >= 0.40) {
        const double exp_delta = (ctx.experience_score - 0.5) * 2.0;
        benefit += exp_delta * 0.25 * ctx.experience_confidence;
    }
    return std::clamp(benefit, 0.0, 1.0);
}

double outcomeRisk(const Situation& s, bool stabilizing, CandidateSource source,
                   const EffectContext& ctx, EffectDirection direction) noexcept {
    double risk = 0.22;
    if (s.stressed() && !stabilizing) risk += 0.25;
    if (direction == EffectDirection::Increase && s.thermal) risk += 0.12;
    if (direction == EffectDirection::Increase && s.memory) risk += 0.10;
    if (source == CandidateSource::BoundedStep) risk += 0.08;
    if (source == CandidateSource::KernelOption) risk += 0.05;
    if (ctx.intervention == InterventionLevel::Low) risk += 0.04;
    // Missing telemetry is uncertainty, not proof of available headroom. Apply
    // a graded per-candidate penalty instead of a global veto so known, bounded
    // candidates can still compete when the rest of their evidence is strong.
    if (!ctx.thermal_available) risk += 0.05;
    if (!ctx.memory_available) risk += 0.05;
    if (!ctx.charging_telemetry_available) risk += 0.03;
    if (ctx.has_experience && ctx.experience_confidence >= 0.40) {
        if (ctx.experience_score < 0.45) risk += (0.45 - ctx.experience_score) * 0.40;
        else if (ctx.experience_score > 0.55) risk -= (ctx.experience_score - 0.55) * 0.20;
    }
    return std::clamp(risk, 0.05, 0.95);
}

bool isUtilizationClamp(const ResourceCapability& cap) noexcept {
    // uclamp / schedtune semantics are a property of the cgroup controller
    // class: a lower clamp or boost always lowers the DVFS request.
    return (cap.domain == ResourceDomain::UClamp) &&
           (cap.path.find("uclamp") != std::string::npos ||
            cap.path.find("schedtune.boost") != std::string::npos);
}

bool endsWith(std::string_view value, std::string_view suffix) noexcept {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Match a real queue selector, not any unrelated tunable whose name happens
// to contain "scheduler". Device topology is not inferred from a device name:
// the discovery matrix, security authority and actuator verify the target.
bool isBlockQueueScheduler(const ResourceCapability& cap) noexcept {
    return cap.domain == ResourceDomain::Io &&
           endsWith(cap.path, "/queue/scheduler");
}

bool isCpuSetCpusLeaf(const ResourceCapability& cap) noexcept {
    if (cap.domain != ResourceDomain::CpuSet) return false;
    const std::string_view path(cap.path);
    const auto slash = path.rfind('/');
    const std::string_view leaf = slash == std::string_view::npos
                                      ? path : path.substr(slash + 1U);
    return leaf == "cpus" || leaf == "cpuset.cpus";
}

bool isPreferredForegroundPath(const std::string& path) noexcept {
    // Match path components rather than broad substrings such as "perf" which
    // can occur in unrelated vendor paths and are not evidence of foreground.
    return path.find("/top-app/") != std::string::npos ||
           path.find("/foreground/") != std::string::npos;
}

bool isBackgroundPath(const std::string& path) noexcept {
    return path.find("/system-background/") != std::string::npos ||
           path.find("/background/") != std::string::npos;
}

bool isConventionalIoSchedulerToken(std::string_view token) noexcept {
    // A small ranking prior only; these tokens are not declared universally
    // safe or optimal. The kernel-advertised option list remains authoritative.
    return token == "none" || token == "mq-deadline" || token == "kyber";
}

bool hasPositiveExperience(const EffectContext& ctx) noexcept {
    return ctx.has_experience && ctx.experience_confidence >= 0.60 &&
           ctx.experience_score >= 0.60;
}

// Kernel-enforced numeric domain bounds for reviewed Linux/Android sysctl and
// queue interfaces. Prevents proposing out-of-range scalars (such as
// vm.swappiness > 200 on ZRAM kernels where OEM baseline is 120..180,
// dirty_ratio > 100, or rq_affinity/nomerges > 2) that fail with EINVAL.
bool kernelScalarBounds(const EcosystemKnowledgeEntry* knowledge,
                        long long scale,
                        long long& min_out,
                        long long& max_out) noexcept {
    if (knowledge == nullptr || knowledge->key == nullptr || scale <= 0LL) {
        return false;
    }
    const std::string_view key(knowledge->key);
    if (key == "vm.swappiness") {
        min_out = 0LL;
        max_out = 200LL * scale;
        return true;
    }
    if (key == "vm.dirty_ratio" || key == "vm.dirty_background_ratio" ||
        key == "cpufreq.governor.hispeed_load" || key == "walt.target_load") {
        min_out = 1LL * scale;
        max_out = 100LL * scale;
        return true;
    }
    if (key == "walt.sched_upmigrate" || key == "walt.sched_downmigrate") {
        min_out = 1LL * scale;
        max_out = 1024LL * scale;
        return true;
    }
    if (key == "vm.vfs_cache_pressure") {
        min_out = 10LL * scale;
        max_out = 500LL * scale;
        return true;
    }
    if (key == "vm.watermark_scale_factor") {
        min_out = 1LL * scale;
        max_out = 1000LL * scale;
        return true;
    }
    if (key == "vm.page-cluster") {
        min_out = 0LL;
        max_out = 8LL * scale;
        return true;
    }
    if (key == "block.queue.rq_affinity" || key == "block.queue.nomerges" ||
        key == "net.ipv4.tcp_ecn") {
        min_out = 0LL;
        max_out = 2LL * scale;
        return true;
    }
    if (key == "net.ipv4.tcp_fastopen") {
        min_out = 0LL;
        max_out = 3LL * scale;
        return true;
    }
    return false;
}

} // namespace

std::vector<EffectScore> EffectModel::proposals(
    const ResourceCapability& capability,
    const std::string& baseline_value,
    const EffectContext& ctx
) const noexcept {
    std::vector<EffectScore> out;
    try {
        if (!capability.exists || !capability.mutation_ready ||
            !capability.permission_granted || !capability.policy_authorized ||
            !capability.runtime_verified || !capability.writable || !capability.readable ||
            !isTunableResourceDomain(capability.domain) ||
            isFatalSurface(capability.path)) {
            return out;
        }
        // cpuset.mems and partition/control files have different migration and
        // ownership semantics. This generic tuner only proposes CPU placement;
        // other cpuset controls need their own semantic actuator/model.
        if (capability.domain == ResourceDomain::CpuSet &&
            !isCpuSetCpusLeaf(capability)) {
            return out;
        }
        std::string base_text = trim(baseline_value);
        if (base_text.empty() || ctx.intervention == InterventionLevel::ObserveOnly) return out;

        const Situation situation = classify(ctx);
        const bool block_scheduler = isBlockQueueScheduler(capability);

        // Apply the embedded semantic catalog before deriving values. It is a
        // risk prior, not mutation authority: the controller still enforces the
        // user's grants, permit, journal, actuator checks and read-back.
        const EcosystemKnowledgeEntry* knowledge =
            lookupEcosystemKnowledgeByPath(capability.path);
        if (knowledge == nullptr) {
            knowledge = lookupEcosystemKnowledgeByKey(capability.name);
        }
        if (knowledge != nullptr) {
            switch (knowledge->risk) {
                case KnowledgeRisk::ObserveOnly:
                case KnowledgeRisk::FrameworkOwned:
                    return out;
                case KnowledgeRisk::Quarantined:
                    // Keep quarantined surfaces discoverable and usable when
                    // there is headroom; never probe them as an emergency fix.
                    if (situation.stressed()) return out;
                    break;
                case KnowledgeRisk::BoundedTune:
                case KnowledgeRisk::VendorVolatile:
                    break;
            }
        }
        if (!situation.stressed() && !situation.elevated && !situation.calm_explore &&
            !(capability.domain == ResourceDomain::Io && situation.io_pressure)) return out;

        // The standard cgroup cpu.uclamp.{min,max} interface is expressed as a
        // percentage (0..100). Do not extend that range to arbitrary vendor paths
        // merely because their names contain "uclamp"; those require their own
        // declared semantics or kernel-advertised options.
        const bool cgroup_uclamp_percent =
            capability.domain == ResourceDomain::UClamp &&
            (capability.path.find("/dev/cpuctl/") != std::string::npos ||
             capability.path.rfind("/sys/fs/cgroup/", 0U) == 0U) &&
            (endsWith(capability.path, "/cpu.uclamp.min") ||
             endsWith(capability.path, "/cpu.uclamp.max"));
        if (cgroup_uclamp_percent && endsWith(capability.path, "/cpu.uclamp.max") && base_text == "max")
            base_text = "100";
        const bool semantic_numeric_bounds = cgroup_uclamp_percent ||
            (knowledge != nullptr &&
             (knowledge->risk == KnowledgeRisk::BoundedTune ||
              (knowledge->risk == KnowledgeRisk::VendorVolatile &&
               knowledge->path != nullptr && capability.path == knowledge->path)));

        const double base_confidence = clamp01(ctx.sample_confidence * 0.92);
        long long base_num = 0;
        const bool base_numeric = parseInteger(base_text, base_num);

        auto emit = [&](const std::string& requested, CandidateSource source,
                        EffectDirection direction, bool stabilizing, const char* reason, const std::string* origin=nullptr) {
            if (requested.empty() || requested == base_text) return;
            if (isFatalValue(base_text, requested)) return;
            // Enforce reviewed ABI bounds for EVERY proposal origin, including
            // vendor profiles and kernel-advertised choices. Bounded-step
            // generation is not the only source of candidate numbers.
            if (semantic_numeric_bounds) {
                if (cgroup_uclamp_percent) {
                    if (requested == "max") {
                        if (!endsWith(capability.path, "/cpu.uclamp.max")) return;
                    } else {
                        const std::size_t dot = requested.find('.');
                        if (dot != std::string::npos &&
                            (dot == 0U || dot + 1U == requested.size() ||
                             requested.find('.', dot + 1U) != std::string::npos ||
                             requested.size() - dot - 1U > 6U)) return;
                        const std::string digits = dot == std::string::npos
                            ? requested : requested.substr(0, dot) + requested.substr(dot + 1U);
                        if (digits.empty() || !std::all_of(digits.begin(), digits.end(),
                            [](char ch) { return ch >= '0' && ch <= '9'; })) return;
                        long long value = 0;
                        if (!parseInteger(digits, value)) return;
                        long long scale = 1LL;
                        if (dot != std::string::npos)
                            for (std::size_t i = dot + 1U; i < requested.size(); ++i) scale *= 10LL;
                        if (value < 0LL || value > 100LL * scale) return;
                    }
                } else {
                    long long domain_min = 0, domain_max = 0;
                    if (kernelScalarBounds(knowledge, 1LL, domain_min, domain_max)) {
                        long long numeric = 0;
                        if (!parseInteger(requested, numeric) ||
                            numeric < domain_min || numeric > domain_max) return;
                    }
                }
            }
            // An ordered hardware scale is discrete. Vendor hints cannot
            // authorize an OPP absent from the active kernel's own table.
            if (capability.ordered_scale && !capability.options.empty() &&
                std::find(capability.options.begin(), capability.options.end(), requested) ==
                    capability.options.end()) return;
            // A CPU governor frequency without the live OPP list cannot be
            // validated against this policy; never guess a frequency or overclock.
            if (knowledge != nullptr && knowledge->key != nullptr &&
                std::string_view(knowledge->key) == "cpufreq.governor.hispeed_freq" &&
                (!capability.ordered_scale || capability.options.empty())) return;
            bool evidence_stabilizing = stabilizing;
            if (block_scheduler && situation.stressed()) {
                // Scheduler changes are relevant to an I/O bottleneck, not a
                // generic remedy for thermal/memory stress. During stress,
                // require matching I/O pressure plus vendor efficiency evidence
                // or a strong candidate-specific measured prior. Mere token
                // availability never makes a scheduler choice stabilizing.
                if (!situation.io_pressure) return;
                const bool vendor_evidence = source == CandidateSource::VendorEfficiency;
                if (!vendor_evidence && !hasPositiveExperience(ctx)) return;
                evidence_stabilizing = true;
            }
            // Domain membership alone is not evidence that a change will
            // stabilize the device. Under memory-only pressure, allow only a
            // vendor-declared efficiency value on a Memory resource; generic
            // numeric steps (including swappiness) must wait for headroom or
            // candidate-specific outcome evidence.
            const bool memory_only_pressure = situation.memory &&
                !situation.thermal && !situation.throttling;
            if (memory_only_pressure &&
                (capability.domain != ResourceDomain::Memory ||
                 source != CandidateSource::VendorEfficiency)) {
                return;
            }
            if (situation.stressed() && !evidence_stabilizing) return;
            for (const auto& existing : out) {
                if (existing.requested == requested) return;
            }
            const double benefit = outcomeBenefit(situation, evidence_stabilizing, source, ctx);
            double risk = outcomeRisk(situation, evidence_stabilizing, source, ctx, direction);
            if (knowledge == nullptr) risk += 0.04;
            else if (knowledge->risk == KnowledgeRisk::VendorVolatile) risk += 0.06;
            EffectScore score;
            score.requested = requested;
            score.source = source;
            score.direction = direction;
            score.stabilizing = evidence_stabilizing;
            score.confidence = clamp01(base_confidence +
                                       (source == CandidateSource::VendorEfficiency ||
                                                source == CandidateSource::VendorPerformance ||
                                                source == CandidateSource::VendorDefault
                                            ? 0.05
                                            : 0.0) +
                                       (ctx.has_experience ? ctx.experience_confidence * 0.08 : 0.0));
            score.benefit = clamp01(benefit);
            score.risk = clamp01(risk);
            score.eligible = score.benefit >= kMinBenefit && score.risk <= kMaxRisk &&
                             score.confidence >= kMinConfidence;
            score.reason = std::string(reason)+" [source:"+candidateSourceName(source);
            if (origin && !origin->empty()) score.reason+=" origin:"+origin->substr(0,512);
            score.reason+="]";
            score.resource_name = capability.name;
            score.path = capability.path;
            score.domain = capability.domain;
            if (score.eligible) out.push_back(std::move(score));
        };

        auto directionOf = [&](const std::string& requested) {
            long long next = 0;
            if (base_numeric && parseInteger(requested, next)) {
                return next > base_num ? EffectDirection::Increase : EffectDirection::Decrease;
            }
            return EffectDirection::Select;
        };

        // ---- 1. Vendor configuration (device-validated operating points) ----
        for (const auto& vendor : capability.vendor_values) {
            const std::string value = trim(vendor.value);
            const EffectDirection dir = directionOf(value);
            switch (vendor.profile) {
                case VendorProfile::Efficiency:
                    emit(value, CandidateSource::VendorEfficiency, dir, true,
                         "vendor efficiency profile value", &vendor.source);
                    break;
                case VendorProfile::Performance:
                    emit(value, CandidateSource::VendorPerformance, dir, false,
                         "vendor performance profile value", &vendor.source);
                    break;
                case VendorProfile::Default:
                case VendorProfile::Boot:
                    emit(value, CandidateSource::VendorDefault, dir, false,
                         "vendor factory configuration value", &vendor.source);
                    break;
            }
        }

        // ---- 2/3. Kernel-advertised options ----
        if (!capability.options.empty()) {
            if (capability.ordered_scale && base_numeric) {
                // Neighbour steps along the hardware's own ordered scale.
                // Note: Qualcomm KGSL pwrlevels index 0 = highest clock and
                // N-1 = lowest clock, so increasing min_pwrlevel lowers the
                // minimum GPU clock (stabilizing) while decreasing toward 0
                // raises the minimum GPU floor for rendering consistency.
                const bool kgsl_inverted_pwrlevel =
                    capability.domain == ResourceDomain::Gpu &&
                    endsWith(capability.path, "/min_pwrlevel");
                long long below = 0;
                long long above = 0;
                bool has_below = false;
                bool has_above = false;
                std::vector<long long> sorted_opps;
                sorted_opps.reserve(capability.options.size());
                for (const auto& option : capability.options) {
                    long long v = 0;
                    if (!parseInteger(option, v)) continue;
                    sorted_opps.push_back(v);
                    if (v < base_num && (!has_below || v > below)) { below = v; has_below = true; }
                    if (v > base_num && (!has_above || v < above)) { above = v; has_above = true; }
                }
                if (has_below) {
                    emit(std::to_string(below), CandidateSource::OrderedScale,
                         EffectDirection::Decrease,
                         !kgsl_inverted_pwrlevel && (situation.thermal || situation.throttling),
                         kgsl_inverted_pwrlevel
                             ? "one step toward higher KGSL GPU power level"
                             : "one step down the advertised hardware scale");
                }
                if (has_above) {
                    emit(std::to_string(above), CandidateSource::OrderedScale,
                         EffectDirection::Increase,
                         kgsl_inverted_pwrlevel && (situation.thermal || situation.throttling),
                         kgsl_inverted_pwrlevel
                             ? "one step toward lower KGSL GPU power level"
                             : "one step up the advertised hardware scale");
                }
                // Demand-proportional raw silicon OPP target: when OEM stock baseline
                // parks a capable chip at its lowest OPP (e.g. KGSL min_pwrlevel=N-1
                // or conservative hispeed_freq) and real-time render/compute pressure
                // is elevated with clean thermal headroom, index proportionally into
                // the hardware's own advertised OPP vector instead of taking 1-OPP microsteps.
                const bool high_interactive_demand =
                    !situation.stressed() && !situation.warm_equilibrium &&
                    sorted_opps.size() >= 4U &&
                    (ctx.gaming_active ||
                     (ctx.gpu_busy_available && ctx.gpu_busy_ratio >= 0.50) ||
                     (ctx.frame_jank_available && ctx.frame_jank_rate >= 0.04) ||
                     (ctx.cpu_utilization_available && ctx.cpu_utilization >= 0.75));
                if (high_interactive_demand) {
                    std::sort(sorted_opps.begin(), sorted_opps.end());
                    sorted_opps.erase(std::unique(sorted_opps.begin(), sorted_opps.end()),
                                      sorted_opps.end());
                    if (sorted_opps.size() >= 4U) {
                        const double demand_intensity = std::clamp(
                            std::max({
                                ctx.gaming_active ? 0.62 : 0.0,
                                ctx.gpu_busy_available ? ctx.gpu_busy_ratio * 0.85 : 0.0,
                                ctx.cpu_utilization_available ? ctx.cpu_utilization * 0.80 : 0.0,
                                ctx.frame_jank_available ? std::min(0.75, ctx.frame_jank_rate * 8.0) : 0.0
                            }),
                            0.35, 0.75);
                        const std::size_t max_idx = sorted_opps.size() - 1U;
                        const std::size_t target_idx = kgsl_inverted_pwrlevel
                            ? static_cast<std::size_t>(
                                  std::llround(static_cast<double>(max_idx) * (1.0 - demand_intensity)))
                            : static_cast<std::size_t>(
                                  std::llround(static_cast<double>(max_idx) * demand_intensity));
                        const long long opp_target = sorted_opps[std::min(target_idx, max_idx)];
                        if ((kgsl_inverted_pwrlevel && opp_target < base_num && opp_target != below) ||
                            (!kgsl_inverted_pwrlevel && opp_target > base_num && opp_target != above)) {
                            emit(std::to_string(opp_target), CandidateSource::OrderedScale,
                                 directionOf(std::to_string(opp_target)), false,
                                 "demand-proportional silicon OPP target from hardware scale");
                        }
                    }
                }
            } else if (!capability.ordered_scale) {
                for (const auto& option : capability.options) {
                    const std::string token = trim(option);
                    if (token.empty() || token == base_text) continue;
                    const bool vendor_low_power = [&] {
                        for (const auto& v : capability.vendor_values) {
                            if (trim(v.value) == token &&
                                v.profile == VendorProfile::Efficiency) return true;
                        }
                        return false;
                    }();
                    const bool clamp_down = isUtilizationClamp(capability) &&
                                            (situation.thermal || situation.throttling);
                    // Every scheduler alternative must be kernel-advertised.
                    // No name is treated as universally safe. Common tokens get
                    // only a small context-specific ranking prior below; other
                    // options (including vendor schedulers) remain eligible.
                    const char* option_reason = "kernel-advertised option";
                    if (block_scheduler) {
                        option_reason = situation.stressed()
                            ? "I/O scheduler candidate with matching pressure and evidence"
                            : "kernel-advertised I/O scheduler option";
                    } else if (clamp_down) {
                        option_reason = "lower utilization clamp option";
                    }
                    emit(token, CandidateSource::KernelOption, EffectDirection::Select,
                         vendor_low_power || clamp_down, option_reason);
                }
            }
        }

        // ---- 4. Bounded exploration for plain integers and fixed decimals ----
        // Fixed-point values (e.g. cpu.uclamp.min "10.00") keep their precision:
        // the step is computed on the scaled integer and formatted back.
        int decimals = 0;
        long long scaled = base_num;
        bool numeric = base_numeric;
        if (!numeric) {
            const auto dot = base_text.find('.');
            if (dot != std::string::npos && dot > 0 && dot + 1 < base_text.size() &&
                base_text.size() - dot - 1 <= 6U) {
                const std::string digits = base_text.substr(0, dot) + base_text.substr(dot + 1);
                long long parsed = 0;
                if (parseInteger(digits, parsed)) {
                    decimals = static_cast<int>(base_text.size() - dot - 1);
                    scaled = parsed;
                    numeric = true;
                }
            }
        }
        auto format = [decimals](long long v) {
            if (decimals == 0) return std::to_string(v);
            long long div = 1;
            for (int i = 0; i < decimals; ++i) div *= 10;
            const bool negative = v < 0;
            // Avoid negating LLONG_MIN, which is undefined behavior.
            const unsigned long long magnitude = negative
                ? static_cast<unsigned long long>(-(v + 1LL)) + 1ULL
                : static_cast<unsigned long long>(v);
            const auto unsigned_div = static_cast<unsigned long long>(div);
            std::string frac = std::to_string(magnitude % unsigned_div);
            while (static_cast<int>(frac.size()) < decimals) frac.insert(0, "0");
            return std::string(negative ? "-" : "") +
                   std::to_string(magnitude / unsigned_div) + "." + frac;
        };
        // An arbitrary scalar is not evidence of its valid range or effect. Only
        // generate +/- steps for a catalogued, bounded interface (or an exact
        // VendorVolatile leaf); unfamiliar nodes need kernel-advertised options,
        // vendor operating points, or measured evidence instead of guessed bounds.
        const bool frequency_needs_scale = knowledge != nullptr && knowledge->key != nullptr &&
            std::string_view(knowledge->key) == "cpufreq.governor.hispeed_freq";
        if (numeric && capability.options.empty() && semantic_numeric_bounds && !frequency_needs_scale) {
            const unsigned long long magnitude = scaled < 0
                ? static_cast<unsigned long long>(-(scaled + 1LL)) + 1ULL
                : static_cast<unsigned long long>(scaled);
            // 0/1 are feature toggles: only vendor-declared values may flip them.
            // A fixed-point 0.00 / 1.00 is a real quantity, not a toggle.
            if (magnitude >= 2ULL || (decimals > 0 && magnitude > 0ULL)) {
                const long long signed_magnitude = magnitude >
                    static_cast<unsigned long long>(std::numeric_limits<long long>::max())
                        ? std::numeric_limits<long long>::max()
                        : static_cast<long long>(magnitude);
                const long long step = std::max<long long>(1, signed_magnitude / 8);
                const long long min = std::numeric_limits<long long>::min();
                const long long max = std::numeric_limits<long long>::max();
                const long long scale = decimals > 0
                    ? static_cast<long long>(std::pow(10.0, decimals)) : 1LL;
                long long lower_bound = cgroup_uclamp_percent ? 0LL :
                    (scaled > 0 ? std::max<long long>(1, scaled / 2)
                                : (scaled < min / 2 ? min : scaled * 2));
                long long upper_bound = cgroup_uclamp_percent ? 100LL * scale :
                    (scaled > 0 ? (scaled > max / 2 ? max : scaled * 2) : scaled / 2);
                long long domain_min = 0LL;
                long long domain_max = 0LL;
                if (!cgroup_uclamp_percent &&
                    kernelScalarBounds(knowledge, scale, domain_min, domain_max)) {
                    lower_bound = std::max(lower_bound, domain_min);
                    upper_bound = std::min(upper_bound, domain_max);
                }
                if (lower_bound > upper_bound) {
                    lower_bound = std::min(scaled, domain_max);
                    upper_bound = std::max(scaled, domain_min);
                }
                const long long raw_down = scaled < min + step ? min : scaled - step;
                const long long raw_up = scaled > max - step ? max : scaled + step;
                const long long down = std::clamp(raw_down, lower_bound, upper_bound);
                const long long up = std::clamp(raw_up, lower_bound, upper_bound);
                const bool thermal_relief = situation.thermal || situation.throttling;
                const std::string_view key = knowledge != nullptr && knowledge->key != nullptr
                    ? std::string_view(knowledge->key) : std::string_view{};
                const bool down_relief = thermal_relief &&
                    (isUtilizationClamp(capability) || key == "cpufreq.governor.hispeed_freq" ||
                     key == "cpufreq.governor.down_rate_limit_us");
                const bool up_relief = thermal_relief &&
                    (key == "cpufreq.governor.up_rate_limit_us" ||
                     key == "cpufreq.governor.hispeed_load" || key == "walt.target_load");
                if (down != scaled) {
                    emit(format(down), CandidateSource::BoundedStep,
                         EffectDirection::Decrease, down_relief,
                         down_relief ? "reduce live DVFS demand" : "bounded exploration step down");
                }
                if (up != scaled) {
                    emit(format(up), CandidateSource::BoundedStep,
                         EffectDirection::Increase, up_relief,
                         up_relief ? "reduce live DVFS boost aggressiveness" : "bounded exploration step up");
                }
                // Demand-scaled continuous fine-tuning candidate: when workload
                // demand is elevated with clean thermal headroom, scale step size
                // continuously from live pressure telemetry within verified bounds
                // so conservative OEM factory baselines (e.g. slow governor ramp or
                // high WALT target_load) converge proportionally without hardcoded constants.
                if (!situation.stressed() && !situation.warm_equilibrium && situation.elevated) {
                    const double demand_ratio = std::clamp(
                        std::max({
                            situation.severity,
                            ctx.gaming_active ? 0.65 : 0.0,
                            ctx.cpu_utilization_available ? ctx.cpu_utilization * 0.85 : 0.0,
                            ctx.gpu_busy_available ? ctx.gpu_busy_ratio * 0.85 : 0.0
                        }),
                        0.25, 0.90);
                    const long long adaptive_delta = std::max<long long>(
                        step + 1LL,
                        static_cast<long long>(
                            std::llround(static_cast<double>(signed_magnitude) * (0.16 + 0.20 * demand_ratio))));
                    const std::string_view kkey =
                        (knowledge != nullptr && knowledge->key != nullptr)
                            ? std::string_view(knowledge->key)
                            : std::string_view{};
                    if (kkey == "cpufreq.governor.up_rate_limit_us" ||
                        kkey == "cpufreq.governor.hispeed_load" ||
                        kkey == "walt.target_load" ||
                        kkey == "walt.sched_upmigrate") {
                        const long long adaptive_down = std::clamp(
                            scaled < min + adaptive_delta ? min : scaled - adaptive_delta,
                            lower_bound, upper_bound);
                        if (adaptive_down != scaled && adaptive_down != down) {
                            emit(format(adaptive_down), CandidateSource::BoundedStep,
                                 EffectDirection::Decrease, false,
                                 "demand-scaled silicon responsiveness fine-tune down");
                        }
                    } else if (kkey == "cpufreq.governor.down_rate_limit_us" ||
                               kkey == "block.queue.read_ahead_kb" ||
                               (cgroup_uclamp_percent && isPreferredForegroundPath(capability.path) &&
                                endsWith(capability.path, "/cpu.uclamp.min"))) {
                        const long long uclamp_cap = cgroup_uclamp_percent ? (35LL * scale) : upper_bound;
                        const long long adaptive_up = std::clamp(
                            scaled > max - adaptive_delta ? max : scaled + adaptive_delta,
                            lower_bound, std::min(upper_bound, uclamp_cap));
                        if (adaptive_up != scaled && adaptive_up != up) {
                            emit(format(adaptive_up), CandidateSource::BoundedStep,
                                 EffectDirection::Increase, false,
                                 "demand-scaled silicon responsiveness fine-tune up");
                        }
                    }
                }
            } else if (cgroup_uclamp_percent && scaled == 0 && !situation.stressed() &&
                       endsWith(capability.path, "/cpu.uclamp.min") &&
                       isPreferredForegroundPath(capability.path)) {
                // OEM factory configs commonly ship top-app / foreground
                // cpu.uclamp.min at 0 or 0.00. Allow a conservative 10% floor
                // step when headroom is available while preserving 0/1 feature
                // toggle protection for all non-uclamp scalar nodes.
                const long long scale = decimals > 0
                    ? static_cast<long long>(std::pow(10.0, decimals)) : 1LL;
                emit(format(10LL * scale), CandidateSource::BoundedStep,
                     EffectDirection::Increase, false,
                     "foreground utilization clamp baseline floor step");
            }
        }
    } catch (...) {
        out.clear();
    }
    return out;
}

EffectScore EffectModel::evaluate(
    const ResourceCapability& capability,
    const std::string& baseline_value,
    const EffectContext& ctx
) const noexcept {
    EffectScore best;
    try {
        best.reason = "no candidate derivable from capability evidence";
        for (auto& score : proposals(capability, baseline_value, ctx)) {
            score.utility = score.benefit * 0.65 - score.risk * 0.25 + score.confidence * 0.10;
            if (score.domain == ResourceDomain::Io &&
                endsWith(score.path, "/queue/scheduler") &&
                ((ctx.psi_available && ctx.psi_io >= pressure_limits::kIoPsiSomeAvg10Enter) ||
                 (ctx.io_latency_available && ctx.io_avg_latency_ms >= pressure_limits::kIoLatencyEnterMs)) &&
                isConventionalIoSchedulerToken(score.requested)) {
                score.utility += 0.015; // weak prior, never a safety/risk override
            }
            if (score.stabilizing) score.utility += 0.01;
            if (!best.eligible || score.utility > best.utility) best = std::move(score);
        }
    } catch (...) {
    }
    return best;
}

std::vector<EffectScore> EffectModel::rank(
    const EnvironmentCapabilityMatrix& matrix,
    const std::unordered_map<std::string, std::string>& baselines,
    const EffectContext& ctx,
    std::size_t max_results
) const noexcept try {
    if (max_results == 0U) return {};
    std::vector<EffectScore> ranked;
    ranked.reserve(matrix.resources.size());
    const CouplingSnapshot coupling = StrategyCatalog::analyze(matrix, baselines, ctx);

    for (const auto& cap : matrix.resources) {
        if (!cap.exists || !cap.mutation_ready || !cap.permission_granted ||
            !cap.policy_authorized || !cap.runtime_verified ||
            !cap.readable || !cap.writable ||
            cap.path.empty()) continue;
        const auto baseline = baselines.find(cap.path);
        if (baseline == baselines.end()) continue;

        std::vector<EffectScore> local = proposals(cap, baseline->second, ctx);
        for (auto& score : local) {
            // Enforce Linux VM writeback watermark invariant:
            // vm.dirty_background_ratio must never exceed vm.dirty_ratio.
            if (cap.domain == ResourceDomain::Memory) {
                long long cand_val = 0;
                if (endsWith(cap.path, "/vm/dirty_background_ratio") &&
                    parseInteger(score.requested, cand_val)) {
                    const std::string sibling =
                        cap.path.substr(0, cap.path.size() - 22U) + "dirty_ratio";
                    const auto sib_it = baselines.find(sibling);
                    long long dirty_ratio_val = 0;
                    if (sib_it != baselines.end() &&
                        parseInteger(sib_it->second, dirty_ratio_val) &&
                        cand_val > dirty_ratio_val) {
                        score.eligible = false;
                        score.utility = -1.0;
                        continue;
                    }
                } else if (endsWith(cap.path, "/vm/dirty_ratio") &&
                           parseInteger(score.requested, cand_val)) {
                    const std::string sibling =
                        cap.path.substr(0, cap.path.size() - 11U) + "dirty_background_ratio";
                    const auto sib_it = baselines.find(sibling);
                    long long bg_ratio_val = 0;
                    if (sib_it != baselines.end() &&
                        parseInteger(sib_it->second, bg_ratio_val) &&
                        cand_val < bg_ratio_val) {
                        score.eligible = false;
                        score.utility = -1.0;
                        continue;
                    }
                }
            }

            const StrategyAdjustment adj = StrategyCatalog::evaluateCandidate(
                cap, baseline->second, score.requested, score.direction, coupling, ctx);
            score.benefit = clamp01(score.benefit + adj.benefit_delta);
            score.risk = clamp01(score.risk + adj.risk_delta);
            score.eligible = score.benefit >= kMinBenefit && score.risk <= kMaxRisk &&
                             score.confidence >= kMinConfidence;
            if (!score.eligible) {
                score.utility = -1.0;
                continue;
            }
            if (adj.coupling_tag != nullptr && adj.coupling_tag[0] != '\0') {
                score.reason += " [";
                score.reason += adj.coupling_tag;
                score.reason += "]";
            }
            score.utility = score.benefit * 0.65 - score.risk * 0.25 + score.confidence * 0.10;
            // Foreground placement is a small tie-breaker, not a claim that the
            // write is less risky. The preference remains a small utility prior.
            if (isPreferredForegroundPath(score.path)) score.utility += 0.015;
            if (isBackgroundPath(score.path)) score.utility -= 0.015;
            if (score.domain == ResourceDomain::Io &&
                endsWith(score.path, "/queue/scheduler") &&
                ((ctx.psi_available && ctx.psi_io >= pressure_limits::kIoPsiSomeAvg10Enter) ||
                 (ctx.io_latency_available && ctx.io_avg_latency_ms >= pressure_limits::kIoLatencyEnterMs)) &&
                isConventionalIoSchedulerToken(score.requested)) {
                score.utility += 0.015;
            }
            if (score.stabilizing) score.utility += 0.01;
        }
        std::stable_sort(local.begin(), local.end(),
                         [](const EffectScore& a, const EffectScore& b) { return a.utility > b.utility; });
        std::size_t kept = 0;
        for (auto& score : local) {
            if (kept >= kProposalsPerResource) break;
            if (!score.eligible || !std::isfinite(score.utility) || score.utility < 0.02) continue;
            ranked.push_back(std::move(score));
            ++kept;
        }
    }

    const auto order = [](const EffectScore& lhs, const EffectScore& rhs) {
        if (lhs.utility != rhs.utility) return lhs.utility > rhs.utility;
        if (lhs.risk != rhs.risk) return lhs.risk < rhs.risk;
        if (lhs.confidence != rhs.confidence) return lhs.confidence > rhs.confidence;
        if (lhs.path != rhs.path) return lhs.path < rhs.path;
        return lhs.requested < rhs.requested;
    };

    if (ranked.size() > max_results) {
        const auto middle = ranked.begin() + static_cast<std::ptrdiff_t>(max_results);
        std::partial_sort(ranked.begin(), middle, ranked.end(), order);
        ranked.resize(max_results);
    } else {
        std::sort(ranked.begin(), ranked.end(), order);
    }
    return ranked;
} catch (...) { return {}; }

} // namespace coreflow
