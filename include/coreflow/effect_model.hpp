#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "coreflow/environment.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

// EffectModel is the capability-driven brain of autonomous resource mutation.
// It does NOT hard-code which resources may be touched. Discovery marks what
// is readable/writable; this model scores whether a bounded change is likely
// to improve stability under the current runtime context.
//
// Soft knowledge (safe ranges, preferred direction) exists as semantic priors
// for known kernel knobs. Unknown writable numerics remain ineligible until a
// semantic prior or adapter defines safe direction and bounds. Text controls
// (schedulers) use kernel-exposed candidates only.

enum class EffectDirection : std::uint8_t {
    None = 0,
    Increase,
    Decrease,
    PreferLowOverhead,   // e.g. scheduler "none" on non-rotational
    PreferDeadline       // e.g. mq-deadline on rotational
};

struct EffectScore {
    bool eligible{false};
    double benefit{0.0};       // 0..1 predicted stability gain
    double risk{1.0};          // 0..1 risk of harm
    double confidence{0.0};    // 0..1 model confidence for this resource
    EffectDirection direction{EffectDirection::None};
    std::string requested;     // concrete value to write (empty if ineligible)
    std::string reason;
    // Structured identity/utility; never encode identity into diagnostic text.
    std::string resource_name;
    std::string path;
    ResourceDomain domain{ResourceDomain::AndroidRuntime};
    double utility{0.0};
};

struct EffectContext {
    RuntimeState state{RuntimeState::Idle};
    double thermal_c{0.0};
    bool thermal_available{false};
    double mem_available_ratio{1.0};
    double cpu_utilization{0.0};
    bool cpu_utilization_available{false};
    double load1{0.0};
    bool io_activity_available{false};
    double io_read_kb_per_sec{0.0};
    double io_write_kb_per_sec{0.0};
    bool charging{false};
    double sample_confidence{0.0};
    InterventionLevel intervention{InterventionLevel::ObserveOnly};
};

class EffectModel final {
public:
    // Score a single discovered capability against the current context and
    // its factory baseline value. Returns ineligible when the change is not
    // justified or cannot be bounded safely.
    EffectScore evaluate(
        const ResourceCapability& capability,
        const std::string& baseline_value,
        const EffectContext& ctx
    ) const noexcept;

    // Rank all mutation-ready capabilities with one benefit/risk/confidence
    // utility function. Structured identity travels with each score; diagnostic
    // text is never parsed to recover the target. Empty vector
    // means no safe intervention this cycle.
    std::vector<EffectScore> rank(
        const EnvironmentCapabilityMatrix& matrix,
        const std::unordered_map<std::string, std::string>& baselines,
        const EffectContext& ctx,
        std::size_t max_results = 3
    ) const noexcept;

private:
    EffectScore scoreNumeric(
        const ResourceCapability& capability,
        const std::string& baseline_value,
        const EffectContext& ctx
    ) const noexcept;

    EffectScore scoreScheduler(
        const ResourceCapability& capability,
        const std::string& baseline_value,
        const EffectContext& ctx
    ) const noexcept;

    static bool parseInteger(const std::string& value, long long& out) noexcept;
    static std::string trim(std::string value) noexcept;
    static double clamp01(double v) noexcept;

    // Semantic priors for well-known knobs. These describe meaning, ranges,
    // and bounded direction; Discovery still decides existence/writability.
    struct NumericPrior {
        const char* name_suffix;   // matched against capability.name
        long long absolute_min;
        long long absolute_max;
        long long max_delta;       // max absolute step from baseline per cycle
        // Preferred direction under thermal pressure / memory pressure.
        EffectDirection thermal_prefer;
        EffectDirection memory_prefer;
    };

    static const NumericPrior* findPrior(const std::string& name) noexcept;
};

} // namespace coreflow
