#include "coreflow/strategy_catalog.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace {
int failures = 0;

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            ++failures;                                                         \
            std::cerr << "FAIL: " << #condition << " (line " << __LINE__       \
                      << ")\n";                                               \
        }                                                                       \
    } while (false)
} // namespace

int main() {
    using namespace coreflow;

    // Strategy matching is ASCII case-insensitive and preserves leading UTF-8
    // bytes instead of converting signed char values implicitly.
    ResourceCapability gpu{};
    gpu.domain = ResourceDomain::Gpu;
    gpu.path = "/sys/class/kgsl/kgsl-3d0/devfreq/governor";

    CouplingSnapshot snapshot{};
    snapshot.profile = StrategyProfile::SustainedGaming;

    EffectContext context{};
    context.gaming_active = true;

    std::string candidate = "\xC3\x89";
    candidate += "MSM-ADRENO-TZ";

    const StrategyAdjustment adjustment = StrategyCatalog::evaluateCandidate(
        gpu,
        "simple_ondemand",
        candidate,
        EffectDirection::Select,
        snapshot,
        context);

    CHECK(std::abs(adjustment.benefit_delta - 0.14) < 1e-9);
    CHECK(adjustment.coupling_tag != nullptr);
    if (adjustment.coupling_tag != nullptr) {
        CHECK(std::string(adjustment.coupling_tag) == "coupled:gpu_render_headroom");
    }

    if (failures != 0) {
        std::cerr << failures << " strategy catalog test(s) failed\n";
        return 1;
    }
    std::cout << "Strategy catalog tests passed\n";
    return 0;
}
