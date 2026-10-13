#include "coreflow/effect_model.hpp"
#include "coreflow/fatal_surface.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

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
    bool stressed() const noexcept { return thermal || memory; }
};

Situation classify(const EffectContext& ctx) noexcept {
    Situation s;
    s.thermal = ctx.thermal_available &&
                (ctx.state == RuntimeState::ThermalGuard || ctx.state == RuntimeState::Warming ||
                 ctx.thermal_c >= 48.0);
    s.memory = ctx.state == RuntimeState::Pressure || ctx.mem_available_ratio < 0.15;
    s.elevated = ctx.state == RuntimeState::Elevated ||
                 (ctx.cpu_utilization_available && ctx.cpu_utilization >= 0.70) ||
                 ctx.load1 >= 1.50;
    s.calm_explore = (ctx.state == RuntimeState::Normal) &&
                     ctx.intervention >= InterventionLevel::Low &&
                     ctx.sample_confidence >= 0.75;
    return s;
}

bool isUtilizationClamp(const ResourceCapability& cap) noexcept {
    // uclamp / schedtune semantics are a property of the cgroup controller
    // class: a lower clamp or boost always lowers the DVFS request.
    return (cap.domain == ResourceDomain::UClamp) &&
           (cap.path.find("uclamp") != std::string::npos ||
            cap.path.find("schedtune.boost") != std::string::npos);
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
        const std::string base_text = trim(baseline_value);
        if (base_text.empty() || ctx.intervention == InterventionLevel::ObserveOnly) return out;

        const Situation situation = classify(ctx);
        if (!situation.stressed() && !situation.elevated && !situation.calm_explore) return out;

        const double base_confidence = clamp01(ctx.sample_confidence * 0.92);
        long long base_num = 0;
        const bool base_numeric = parseInteger(base_text, base_num);

        auto emit = [&](const std::string& requested, CandidateSource source,
                        EffectDirection direction, bool stabilizing, double benefit,
                        double risk, const char* reason) {
            if (requested.empty() || requested == base_text) return;
            if (isFatalValue(base_text, requested)) return;
            // Memory pressure without thermal stress: experimenting on the
            // pressured resource's own domain is the stabilizing action; the
            // measured outcome (and rollback) decides which direction helps.
            if (situation.memory && !situation.thermal &&
                capability.domain == ResourceDomain::Memory) {
                stabilizing = true;
                benefit = std::max(benefit, 0.24);
            }
            // Stressed contexts accept only load-reducing changes.
            if (situation.stressed() && !stabilizing) return;
            for (const auto& existing : out) {
                if (existing.requested == requested) return;  // keep first (strongest) source
            }
            EffectScore score;
            score.requested = requested;
            score.source = source;
            score.direction = direction;
            score.stabilizing = stabilizing;
            score.confidence = clamp01(base_confidence +
                                       (source == CandidateSource::VendorEfficiency ||
                                                source == CandidateSource::VendorPerformance ||
                                                source == CandidateSource::VendorDefault
                                            ? 0.05
                                            : 0.0));
            if (ctx.intervention == InterventionLevel::Low) risk += 0.05;
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
                         situation.stressed() ? 0.44 : 0.20, 0.18,
                         "vendor efficiency profile value");
                    break;
                case VendorProfile::Performance:
                    emit(value, CandidateSource::VendorPerformance, dir, false,
                         situation.elevated ? 0.34 : 0.16, 0.22,
                         "vendor performance profile value");
                    break;
                case VendorProfile::Default:
                case VendorProfile::Boot:
                    emit(value, CandidateSource::VendorDefault, dir, false, 0.28, 0.20,
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
                         EffectDirection::Decrease, situation.thermal,
                         situation.thermal ? 0.36 : 0.16, 0.24,
                         "one step down the advertised hardware scale");
                }
                if (has_above) {
                    emit(std::to_string(above), CandidateSource::OrderedScale,
                         EffectDirection::Increase, false,
                         situation.elevated ? 0.24 : 0.12, 0.30,
                         "one step up the advertised hardware scale");
                }
            } else if (!capability.ordered_scale) {
                for (const auto& option : capability.options) {
                    const bool vendor_low_power = [&] {
                        for (const auto& v : capability.vendor_values) {
                            if (v.value == option && v.profile == VendorProfile::Efficiency) return true;
                        }
                        return false;
                    }();
                    emit(option, CandidateSource::KernelOption, EffectDirection::Select,
                         vendor_low_power, situation.elevated ? 0.20 : 0.16, 0.30,
                         "kernel-advertised option");
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
            const long long a = negative ? -v : v;
            std::string frac = std::to_string(a % div);
            while (static_cast<int>(frac.size()) < decimals) frac.insert(0, "0");
            return std::string(negative ? "-" : "") + std::to_string(a / div) + "." + frac;
        };
        if (numeric && capability.options.empty()) {
            const long long magnitude = scaled < 0 ? -scaled : scaled;
            // 0/1 are feature toggles: only vendor-declared values may flip them.
            // A fixed-point 0.00 / 1.00 is a real quantity, not a toggle.
            if (magnitude >= 2 || (decimals > 0 && magnitude > 0)) {
                const long long step = std::max<long long>(1, magnitude / 8);
                const long long lower_bound = scaled > 0 ? std::max<long long>(1, scaled / 2)
                                                           : scaled * 2;
                const long long upper_bound = scaled > 0 ? scaled * 2 : scaled / 2;
                const long long down = std::max(lower_bound, scaled - step);
                const long long up = std::min(upper_bound, scaled + step);
                const bool clamp = isUtilizationClamp(capability);
                if (down != scaled) {
                    emit(format(down), CandidateSource::BoundedStep,
                         EffectDirection::Decrease, clamp,
                         clamp && situation.stressed() ? 0.30 : 0.16, 0.34,
                         clamp ? "lower utilization clamp" : "bounded exploration step down");
                }
                if (up != scaled) {
                    emit(format(up), CandidateSource::BoundedStep,
                         EffectDirection::Increase, false, 0.16, 0.34,
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
