#include "coreflow/effect_model.hpp"
#include "coreflow/ecosystem_knowledge.hpp"
#include "coreflow/fatal_surface.hpp"
#include "coreflow/pressure_limits.hpp"

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
    try {
        const std::string cleaned = trim(value);
        if (cleaned.empty()) return false;
        std::size_t used = 0;
        const long long parsed = std::stoll(cleaned, &used, 10);
        if (used != cleaned.size()) return false;
        out = parsed;
        return true;
    } catch (...) {
        return false;
    }
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
    s.thermal = ctx.thermal_available &&
                (ctx.state == RuntimeState::ThermalGuard || ctx.state == RuntimeState::Warming ||
                 ctx.thermal_c >= 48.0);
    s.memory = ctx.state == RuntimeState::Pressure || ctx.mem_available_ratio < 0.15 ||
               ctx.memory_stall_detected;
    s.io_pressure =
        (ctx.psi_available &&
         ctx.psi_io >= pressure_limits::kIoPsiSomeAvg10Enter) ||
        (ctx.io_latency_available &&
         ctx.io_avg_latency_ms >= pressure_limits::kIoLatencyEnterMs);
    const bool cpu_or_load_elevated =
        (ctx.cpu_utilization_available && ctx.cpu_utilization >= 0.70) ||
        ctx.load1 >= 1.50;
    // AdaptivePolicy can enter Elevated due to I/O alone. Do not interpret that
    // state as a general CPU-load signal unless CPU/load telemetry agrees.
    s.elevated = cpu_or_load_elevated ||
                 (ctx.state == RuntimeState::Elevated && !s.io_pressure);
    s.calm_explore = (ctx.state == RuntimeState::Normal) &&
                     ctx.intervention >= InterventionLevel::Low &&
                     ctx.sample_confidence >= 0.75;
    s.throttling = ctx.throttling_detected;

    double sev = 0.0;
    if (ctx.psi_available) {
        sev = std::max({ctx.psi_cpu, ctx.psi_memory, ctx.psi_io * 0.85});
    }
    if (ctx.throttle_ratio > sev) sev = ctx.throttle_ratio;
    if (ctx.memory_stall_score > sev) sev = ctx.memory_stall_score;
    if (s.thermal && ctx.thermal_available) {
        const double t = std::clamp((ctx.thermal_c - 48.0) / 12.0, 0.0, 1.0);
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
        const std::string base_text = trim(baseline_value);
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
            !(block_scheduler && situation.io_pressure)) return out;

        const double base_confidence = clamp01(ctx.sample_confidence * 0.92);
        long long base_num = 0;
        const bool base_numeric = parseInteger(base_text, base_num);

        auto emit = [&](const std::string& requested, CandidateSource source,
                        EffectDirection direction, bool stabilizing, const char* reason) {
            if (requested.empty() || requested == base_text) return;
            if (isFatalValue(base_text, requested)) return;
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
            const double risk = outcomeRisk(situation, evidence_stabilizing, source, ctx, direction);
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
            score.reason = reason;
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
                         "vendor efficiency profile value");
                    break;
                case VendorProfile::Performance:
                    emit(value, CandidateSource::VendorPerformance, dir, false,
                         "vendor performance profile value");
                    break;
                case VendorProfile::Default:
                case VendorProfile::Boot:
                    emit(value, CandidateSource::VendorDefault, dir, false,
                         "vendor factory configuration value");
                    break;
            }
        }

        // ---- 2/3. Kernel-advertised options ----
        if (!capability.options.empty()) {
            if (capability.ordered_scale && base_numeric) {
                // Neighbour steps along the hardware's own ordered scale.
                long long below = 0;
                long long above = 0;
                bool has_below = false;
                bool has_above = false;
                for (const auto& option : capability.options) {
                    long long v = 0;
                    if (!parseInteger(option, v)) continue;
                    if (v < base_num && (!has_below || v > below)) { below = v; has_below = true; }
                    if (v > base_num && (!has_above || v < above)) { above = v; has_above = true; }
                }
                if (has_below) {
                    emit(std::to_string(below), CandidateSource::OrderedScale,
                         EffectDirection::Decrease, situation.thermal || situation.throttling,
                         "one step down the advertised hardware scale");
                }
                if (has_above) {
                    emit(std::to_string(above), CandidateSource::OrderedScale,
                         EffectDirection::Increase, false,
                         "one step up the advertised hardware scale");
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
        if (numeric && capability.options.empty()) {
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
                const long long lower_bound = scaled > 0 ? std::max<long long>(1, scaled / 2)
                    : (scaled < min / 2 ? min : scaled * 2);
                const long long upper_bound = scaled > 0
                    ? (scaled > max / 2 ? max : scaled * 2) : scaled / 2;
                const long long raw_down = scaled < min + step ? min : scaled - step;
                const long long raw_up = scaled > max - step ? max : scaled + step;
                const long long down = std::max(lower_bound, raw_down);
                const long long up = std::min(upper_bound, raw_up);
                const bool clamp = isUtilizationClamp(capability) &&
                                    (situation.thermal || situation.throttling);
                if (down != scaled) {
                    emit(format(down), CandidateSource::BoundedStep,
                         EffectDirection::Decrease, clamp,
                         clamp ? "lower utilization clamp" : "bounded exploration step down");
                }
                if (up != scaled) {
                    emit(format(up), CandidateSource::BoundedStep,
                         EffectDirection::Increase, false,
                         "bounded exploration step up");
                }
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
    best.reason = "no candidate derivable from capability evidence";
    try {
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
) const noexcept {
    std::vector<EffectScore> ranked;
    ranked.reserve(matrix.resources.size());

    for (const auto& cap : matrix.resources) {
        if (!cap.exists || !cap.mutation_ready || !cap.permission_granted ||
            !cap.policy_authorized || !cap.runtime_verified ||
            !cap.readable || !cap.writable ||
            cap.path.empty()) continue;
        const auto baseline = baselines.find(cap.path);
        if (baseline == baselines.end()) continue;

        std::vector<EffectScore> local = proposals(cap, baseline->second, ctx);
        for (auto& score : local) {
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
            if (!std::isfinite(score.utility) || score.utility < 0.02) continue;
            ranked.push_back(std::move(score));
            ++kept;
        }
    }

    std::stable_sort(ranked.begin(), ranked.end(),
                     [](const EffectScore& lhs, const EffectScore& rhs) {
                         if (lhs.utility != rhs.utility) return lhs.utility > rhs.utility;
                         if (lhs.risk != rhs.risk) return lhs.risk < rhs.risk;
                         if (lhs.confidence != rhs.confidence)
                             return lhs.confidence > rhs.confidence;
                         if (lhs.path != rhs.path) return lhs.path < rhs.path;
                         return lhs.requested < rhs.requested;
                     });

    if (ranked.size() > max_results) ranked.resize(max_results);
    return ranked;
}

} // namespace coreflow
