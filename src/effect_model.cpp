#include "coreflow/effect_model.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace coreflow {
namespace {

constexpr double kMinBenefit = 0.12;
constexpr double kMaxRisk = 0.55;
constexpr double kMinConfidence = 0.55;

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

const EffectModel::NumericPrior* EffectModel::findPrior(
    const std::string& name
) noexcept {
    // Soft knowledge only. Discovery still owns existence/writability.
    static constexpr NumericPrior kPriors[] = {
        // name_suffix, abs_min, abs_max, max_delta, thermal_prefer, memory_prefer
        {"vm.swappiness", 10, 100, 15,
         EffectDirection::Decrease, EffectDirection::Increase},
        {"vm.dirty_ratio", 5, 40, 5,
         EffectDirection::Decrease, EffectDirection::Increase},
        {"vm.dirty_background_ratio", 3, 20, 3,
         EffectDirection::Decrease, EffectDirection::Increase},
        {"vm.vfs_cache_pressure", 50, 200, 25,
         EffectDirection::Decrease, EffectDirection::Increase},
        {"vm.min_free_kbytes", 4096, 262144, 8192,
         EffectDirection::Increase, EffectDirection::Increase},
        {"vm.dirty_expire_centisecs", 500, 6000, 500,
         EffectDirection::Decrease, EffectDirection::Increase},
        {"vm.dirty_writeback_centisecs", 100, 2000, 200,
         EffectDirection::Decrease, EffectDirection::None},
        {"read_ahead_kb", 16, 2048, 256,
         EffectDirection::Decrease, EffectDirection::None},
        {"nr_requests", 32, 512, 64,
         EffectDirection::Decrease, EffectDirection::None},
    };
    for (const auto& prior : kPriors) {
        if (name == prior.name_suffix) return &prior;
        // Allow "sda:read_ahead_kb" style names.
        if (name.size() > std::char_traits<char>::length(prior.name_suffix)) {
            const auto pos = name.rfind(prior.name_suffix);
            if (pos != std::string::npos &&
                pos + std::char_traits<char>::length(prior.name_suffix) == name.size() &&
                (pos == 0 || name[pos - 1] == ':' || name[pos - 1] == '.')) {
                return &prior;
            }
        }
    }
    return nullptr;
}

EffectScore EffectModel::scoreNumeric(
    const ResourceCapability& capability,
    const std::string& baseline_value,
    const EffectContext& ctx
) const noexcept {
    EffectScore score;
    score.confidence = clamp01(ctx.sample_confidence * 0.85);

    long long base = 0;
    if (!parseInteger(baseline_value, base)) {
        score.reason = "baseline not integer";
        return score;
    }

    const NumericPrior* prior = findPrior(capability.name);
    if (prior == nullptr) {
        score.reason = "unknown numeric semantics; mutation withheld";
        return score;
    }

    const bool thermal_pressure =
        ctx.thermal_available &&
        (ctx.state == RuntimeState::ThermalGuard ||
         ctx.state == RuntimeState::Warming ||
         ctx.thermal_c >= 48.0);

    const bool memory_pressure =
        ctx.state == RuntimeState::Pressure ||
        ctx.mem_available_ratio < 0.15;

    const bool elevated =
        ctx.state == RuntimeState::Elevated ||
        (ctx.cpu_utilization_available && ctx.cpu_utilization >= 0.70) ||
        ctx.load1 >= 1.50;
    const bool io_pressure = ctx.io_activity_available &&
        (ctx.io_read_kb_per_sec + ctx.io_write_kb_per_sec) >= 1024.0;
    const bool read_ahead_resource =
        capability.name == "read_ahead_kb" ||
        (capability.name.size() >= 13 &&
         capability.name.compare(capability.name.size() - 13, 13, "read_ahead_kb") == 0);
    const bool request_depth_resource =
        capability.name == "nr_requests" ||
        (capability.name.size() > 11 &&
         capability.name.compare(capability.name.size() - 11, 11, "nr_requests") == 0);

    EffectDirection prefer = EffectDirection::None;
    double benefit = 0.0;
    double risk = 0.45;

    if (thermal_pressure) {
        prefer = prior ? prior->thermal_prefer : EffectDirection::Decrease;
        benefit = 0.35 + (ctx.thermal_c >= 55.0 ? 0.25 : 0.10);
        risk = 0.30;
        score.confidence = clamp01(score.confidence + 0.08);
    } else if (memory_pressure) {
        prefer = prior ? prior->memory_prefer : EffectDirection::Increase;
        benefit = 0.40 + (ctx.mem_available_ratio < 0.10 ? 0.20 : 0.05);
        risk = 0.28;
        score.confidence = clamp01(score.confidence + 0.06);
    } else if (io_pressure && ctx.intervention >= InterventionLevel::Low &&
               capability.domain == ResourceDomain::Io) {
        // I/O controls require I/O evidence; CPU/load pressure alone is not a
        // sufficient reason to tune queue depth or read-ahead.
        if (read_ahead_resource) {
            prefer = EffectDirection::Increase;
            benefit = 0.22;
            risk = 0.34;
        } else if (request_depth_resource) {
            prefer = EffectDirection::Increase;
            benefit = 0.16;
            risk = 0.42;
        }
    } else if (elevated && ctx.intervention >= InterventionLevel::Low) {
        // CPU/load pressure alone cannot justify an arbitrary numeric knob.
        score.reason = "no workload-specific numeric effect evidence";
        return score;
    } else if (ctx.state == RuntimeState::Idle || ctx.state == RuntimeState::Normal) {
        // Very mild optimization only at Low+ intervention with high confidence.
        if (ctx.intervention >= InterventionLevel::Low &&
            ctx.sample_confidence >= 0.80 &&
            prior != nullptr) {
            prefer = EffectDirection::Decrease;
            benefit = 0.14;
            risk = 0.40;
        }
    }

    if (prefer == EffectDirection::None || prefer == EffectDirection::PreferLowOverhead ||
        prefer == EffectDirection::PreferDeadline) {
        score.reason = "no numeric direction justified";
        return score;
    }

    const long long abs_min = prior->absolute_min;
    const long long abs_max = prior->absolute_max;
    const long long max_delta = prior->max_delta;
    if (base < abs_min || base > abs_max) {
        score.reason = "baseline outside semantic bounds";
        return score;
    }

    long long delta = 0;
    if (prefer == EffectDirection::Increase) {
        delta = std::min(max_delta, memory_pressure ? max_delta : max_delta / 2);
        if (delta < 1) delta = 1;
    } else if (prefer == EffectDirection::Decrease) {
        delta = -std::min(max_delta, thermal_pressure ? max_delta : max_delta / 2);
        if (delta > -1) delta = -1;
    }

    // Compute the bounded target without signed overflow.
    long long requested = base;
    if (delta > 0 && base > abs_max - delta) {
        requested = abs_max;
    } else if (delta < 0 && base < abs_min - delta) {
        requested = abs_min;
    } else {
        requested = std::clamp(base + delta, abs_min, abs_max);
    }
    if (requested == base) {
        score.reason = "requested equals baseline";
        return score;
    }

    // Intervention level clamps aggressiveness.
    if (ctx.intervention == InterventionLevel::ObserveOnly) {
        score.reason = "observe-only intervention";
        return score;
    }
    if (ctx.intervention == InterventionLevel::Low) {
        // Allow only half step.
        const long long mid = base + (requested - base) / 2;
        if (mid == base) {
            // Keep at least 1 unit move if possible.
            if (requested > base && base < abs_max) requested = base + 1;
            else if (requested < base && base > abs_min) requested = base - 1;
            requested = std::clamp(requested, abs_min, abs_max);
            if (requested == base) {
                score.reason = "low intervention cannot move";
                return score;
            }
        } else {
            requested = mid;
        }
        risk = std::min(1.0, risk + 0.05);
    }

    score.eligible = benefit >= kMinBenefit && risk <= kMaxRisk &&
                     score.confidence >= kMinConfidence;
    score.benefit = clamp01(benefit);
    score.risk = clamp01(risk);
    score.direction = prefer;
    score.requested = std::to_string(requested);
    if (score.reason.empty()) {
        score.reason = thermal_pressure
                           ? "thermal stability"
                           : (memory_pressure ? "memory stability" : "efficiency");
    }
    return score;
}

EffectScore EffectModel::scoreScheduler(
    const ResourceCapability& capability,
    const std::string& baseline_value,
    const EffectContext& ctx
) const noexcept {
    EffectScore score;
    score.confidence = clamp01(ctx.sample_confidence * 0.75);

    // Only consider under sustained load or thermal/memory pressure.
    const bool justify =
        ctx.state == RuntimeState::Elevated ||
        ctx.state == RuntimeState::Warming ||
        ctx.state == RuntimeState::ThermalGuard ||
        ctx.state == RuntimeState::Pressure ||
        (ctx.cpu_utilization_available && ctx.cpu_utilization >= 0.70) ||
        ctx.load1 >= 1.50;

    if (!justify || ctx.intervention == InterventionLevel::ObserveOnly) {
        score.reason = "scheduler change not justified";
        return score;
    }

    // baseline_value for scheduler is the full kernel string e.g.
    // "[mq-deadline] kyber none" or current selection. We need available list.
    // Resource mutation layer re-reads the live scheduler file for candidates;
    // here we only signal preferred direction.
    const bool thermal =
        ctx.thermal_available &&
        (ctx.state == RuntimeState::ThermalGuard || ctx.thermal_c >= 50.0);

    if (thermal || ctx.state == RuntimeState::Pressure) {
        score.direction = EffectDirection::PreferLowOverhead;
        score.benefit = 0.22;
        score.risk = 0.38;
        score.eligible = score.benefit >= kMinBenefit && score.risk <= kMaxRisk &&
                         score.confidence >= kMinConfidence;
        score.requested = "none";  // preferred token; applicator verifies availability
        score.reason = "prefer low-overhead scheduler under pressure";
        return score;
    }

    score.direction = EffectDirection::PreferDeadline;
    score.benefit = 0.16;
    score.risk = 0.42;
    score.eligible = score.benefit >= kMinBenefit && score.risk <= kMaxRisk &&
                     score.confidence >= kMinConfidence;
    score.requested = "mq-deadline";
    score.reason = "prefer deadline scheduler under sustained load";
    (void)capability;
    (void)baseline_value;
    return score;
}

EffectScore EffectModel::evaluate(
    const ResourceCapability& capability,
    const std::string& baseline_value,
    const EffectContext& ctx
) const noexcept {
    EffectScore score;
    if (!capability.exists || !capability.mutation_ready ||
        !capability.permission_granted || !capability.runtime_verified ||
        !capability.writable || !capability.readable) {
        score.reason = "not mutation_ready";
        return score;
    }
    if (baseline_value.empty()) {
        score.reason = "no baseline";
        return score;
    }

    // Domain gate: only Memory / Io / Scheduler numeric-or-scheduler knobs.
    // Charging, Thermal sensors, AndroidRuntime, etc. stay telemetry-only.
    switch (capability.domain) {
        case ResourceDomain::Memory:
        case ResourceDomain::Io:
        case ResourceDomain::Scheduler:
            break;
        default:
            score.reason = "domain not autonomously mutable";
            return score;
    }

    const std::string& name = capability.name;
    if (name.find("scheduler") != std::string::npos) {
        return scoreScheduler(capability, baseline_value, ctx);
    }
    return scoreNumeric(capability, baseline_value, ctx);
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
            !cap.runtime_verified || !cap.readable || !cap.writable ||
            cap.path.empty()) continue;
        const auto baseline = baselines.find(cap.path);
        if (baseline == baselines.end()) continue;

        EffectScore score = evaluate(cap, baseline->second, ctx);
        if (!score.eligible) continue;

        score.utility = score.benefit * 0.65 - score.risk * 0.25 + score.confidence * 0.10;
        if (!std::isfinite(score.utility) || score.utility < 0.05) continue;
        score.resource_name = cap.name;
        score.path = cap.path;
        score.domain = cap.domain;
        ranked.push_back(std::move(score));
    }

    std::stable_sort(ranked.begin(), ranked.end(),
                     [](const EffectScore& lhs, const EffectScore& rhs) {
                         if (lhs.utility != rhs.utility) return lhs.utility > rhs.utility;
                         if (lhs.risk != rhs.risk) return lhs.risk < rhs.risk;
                         if (lhs.confidence != rhs.confidence)
                             return lhs.confidence > rhs.confidence;
                         if (lhs.path != rhs.path) return lhs.path < rhs.path;
                         return lhs.resource_name < rhs.resource_name;
                     });

    if (ranked.size() > max_results) ranked.resize(max_results);
    return ranked;
}

} // namespace coreflow
