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
// It contains no parameter tables. Every candidate value is derived from
// evidence the device itself provides:
//
//   1. Vendor configuration   values the vendor's init / power HAL writes to
//                             this node, tagged boot/default/efficiency/
//                             performance (VendorKnowledge)
//   2. Kernel advertisement   tokens from a "[a] b c" selector line or a
//                             sibling available_* list
//   3. Ordered hardware scale one step along an advertised frequency/level list
//   4. Bounded exploration    a +/-12.5% step from the factory value for plain
//                             integers (never for 0/1 toggles, never sign flips)
//
// Stressed contexts (thermal/memory) accept only stabilizing candidates:
// vendor efficiency-profile values, a step down an ordered hardware scale, or
// a lower utilization clamp. Everything else is explored only with headroom,
// and BaselineIntelligence measures each outcome; experience then steers later
// choices. With no other evidence, a step down is tried before a step up (the
// lower resource commitment is the conservative probe). The only hard
// restriction lives in FatalSurfaceGuard.

enum class EffectDirection : std::uint8_t {
    None = 0,
    Increase,
    Decrease,
    Select,   // choose one of several advertised / vendor-declared values
};

enum class CandidateSource : std::uint8_t {
    None = 0,
    VendorEfficiency,
    VendorPerformance,
    VendorDefault,
    KernelOption,
    OrderedScale,
    BoundedStep,
};

const char* candidateSourceName(CandidateSource source) noexcept;

struct EffectScore {
    bool eligible{false};
    double benefit{0.0};       // 0..1 predicted stability gain
    double risk{1.0};          // 0..1 risk of harm
    double confidence{0.0};    // 0..1 model confidence for this resource
    EffectDirection direction{EffectDirection::None};
    // True only when the change responds to thermal or memory stress by moving a
    // knob in its load-reducing direction. Stressed contexts accept nothing else.
    bool stabilizing{false};
    std::string requested;     // concrete value to write (empty if ineligible)
    CandidateSource source{CandidateSource::None};
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

    // Every candidate value derivable for one capability, scored. Ineligible
    // entries are omitted.
    std::vector<EffectScore> proposals(
        const ResourceCapability& capability,
        const std::string& baseline_value,
        const EffectContext& ctx
    ) const noexcept;

    // Rank all mutation-ready capabilities with one benefit/risk/confidence
    // utility function. At most kProposalsPerResource candidates per node are
    // kept so one rich node cannot crowd out the rest of the device.
    static constexpr std::size_t kProposalsPerResource = 2;
    std::vector<EffectScore> rank(
        const EnvironmentCapabilityMatrix& matrix,
        const std::unordered_map<std::string, std::string>& baselines,
        const EffectContext& ctx,
        std::size_t max_results = 3
    ) const noexcept;

private:
    static bool parseInteger(const std::string& value, long long& out) noexcept;
    static std::string trim(std::string value) noexcept;
    static double clamp01(double v) noexcept;
};

} // namespace coreflow
