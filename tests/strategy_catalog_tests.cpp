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

    // OEM ZRAM swappiness=120 (Linux 5.8+ 0..200 scale) is recognized as an
    // already ZRAM-tuned factory baseline so blind upward inflation is penalized
    // while vfs_cache_pressure relief is rewarded under MemoryRelief.
    EnvironmentCapabilityMatrix mem_matrix{};
    ResourceCapability swap_cap{};
    swap_cap.exists = swap_cap.mutation_ready = true;
    swap_cap.domain = ResourceDomain::Memory;
    swap_cap.name = "vm.swappiness";
    swap_cap.path = "/proc/sys/vm/swappiness";
    mem_matrix.resources.push_back(swap_cap);

    ResourceCapability vfs_cap = swap_cap;
    vfs_cap.name = "vm.vfs_cache_pressure";
    vfs_cap.path = "/proc/sys/vm/vfs_cache_pressure";
    mem_matrix.resources.push_back(vfs_cap);

    ResourceCapability bg_uclamp{};
    bg_uclamp.exists = bg_uclamp.mutation_ready = true;
    bg_uclamp.domain = ResourceDomain::UClamp;
    bg_uclamp.name = "background:cpu.uclamp.max";
    bg_uclamp.path = "/dev/cpuctl/background/cpu.uclamp.max";
    mem_matrix.resources.push_back(bg_uclamp);

    EffectContext mem_ctx{};
    mem_ctx.memory_available = true;
    mem_ctx.mem_available_ratio = 0.14;
    mem_ctx.memory_stall_detected = true;

    const std::unordered_map<std::string, std::string> baselines = {
        {"/proc/sys/vm/swappiness", "120"},
        {"/proc/sys/vm/vfs_cache_pressure", "100"},
        {"/dev/cpuctl/background/cpu.uclamp.max", "80"},
    };
    const CouplingSnapshot mem_snap = StrategyCatalog::analyze(mem_matrix, baselines, mem_ctx);
    CHECK(mem_snap.profile == StrategyProfile::MemoryRelief);
    CHECK(mem_snap.zram_high_swappiness);

    const StrategyAdjustment swap_up_adj = StrategyCatalog::evaluateCandidate(
        swap_cap, "120", "135", EffectDirection::Increase, mem_snap, mem_ctx);
    CHECK(swap_up_adj.benefit_delta < 0.0 && swap_up_adj.risk_delta > 0.0);
    CHECK(swap_up_adj.coupling_tag != nullptr &&
          std::string(swap_up_adj.coupling_tag) == "coupled:zram_oem_swappiness_saturated");

    const StrategyAdjustment vfs_up_adj = StrategyCatalog::evaluateCandidate(
        vfs_cap, "100", "112", EffectDirection::Increase, mem_snap, mem_ctx);
    CHECK(vfs_up_adj.benefit_delta > 0.0 && vfs_up_adj.risk_delta < 0.0);

    if (failures != 0) {
        std::cerr << failures << " strategy catalog test(s) failed\n";
        return 1;
    }
    std::cout << "Strategy catalog tests passed\n";
    return 0;
}
