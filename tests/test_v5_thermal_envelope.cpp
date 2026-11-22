#include "coreflow/baseline_store.hpp"
#include "coreflow/v5/thermal_envelope_actuator.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace coreflow;
using namespace coreflow::v5;
namespace fs = std::filesystem;

static void write_file(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p);
    assert(out.good());
    out << content;
}

static std::string read_file(const fs::path& p) {
    std::ifstream in(p);
    std::string line;
    std::getline(in, line);
    return line;
}

int main() {
    char tpl[] = "/tmp/coreflow-v5-env-XXXXXX";
    const char* created = ::mkdtemp(tpl);
    assert(created != nullptr);
    const fs::path root(created);
    assert(::setenv("COREFLOW_TEST_STATE_DIR", root.c_str(), 1) == 0);

    auto& baseline = BaselineStore::instance();
    baseline.reinitialize();
    assert(baseline.recover_pending());

    HardwareTopology topo{};
    topo.refresh_hz = 120U;

    const fs::path hispeed = root / "cpu/hispeed_freq";
    const fs::path hispeed_load = root / "cpu/hispeed_load";
    const fs::path up_rate = root / "cpu/up_rate_limit_us";
    const fs::path down_rate = root / "cpu/down_rate_limit_us";
    const fs::path gpu_idle = root / "gpu/idle_timer";
    const fs::path kgsl_max = root / "gpu/max_pwrlevel";
    const fs::path kgsl_min = root / "gpu/min_pwrlevel";
    write_file(hispeed, "1800000\n");
    write_file(hispeed_load, "90\n");
    write_file(up_rate, "0\n");
    write_file(down_rate, "0\n");
    write_file(gpu_idle, "80\n");
    write_file(kgsl_max, "0\n");
    write_file(kgsl_min, "5\n");

    CpuClusterInfo big{};
    big.role = ClusterRole::Big;
    big.governor = "walt";
    big.opps_khz = {600000, 1000000, 1400000, 1800000, 2200000, 2600000};
    big.hispeed_freq_path = hispeed.string();
    big.hispeed_load_path = hispeed_load.string();
    big.up_rate_limit_path = up_rate.string();
    big.down_rate_limit_path = down_rate.string();
    big.baseline_up_rate_limit_us = 0U;
    big.has_baseline_up_rate_limit = true;
    big.baseline_down_rate_limit_us = 0U;
    big.has_baseline_down_rate_limit = true;
    topo.cpu_clusters.push_back(big);

    topo.gpu.available = true;
    topo.gpu.is_kgsl = true;
    topo.gpu.kgsl_num_pwrlevels = 7U;
    topo.gpu.idle_timer_path = gpu_idle.string();
    topo.gpu.kgsl_max_pwrlevel_path = kgsl_max.string();
    topo.gpu.kgsl_min_pwrlevel_path = kgsl_min.string();

    ThermalEnvelopeActuator act(baseline);

    // 1. Gaming at full capacity scale -> high hispeed (below max OPP), responsive hispeed_load, fast KGSL max_pwrlevel (0), WALT 0 us preserved
    act.apply(topo, ContextMode::Gaming, PerformanceProfile::Gaming, 1.0, false, true, {20.0, 70.0});
    const uint64_t gaming_hispeed = std::stoull(read_file(hispeed));
    const uint64_t gaming_load = std::stoull(read_file(hispeed_load));
    const uint64_t gaming_kgsl_max = std::stoull(read_file(kgsl_max));
    assert(gaming_hispeed >= 2200000U && gaming_hispeed < 2600000U);
    assert(gaming_load >= 70U && gaming_load <= 80U);
    assert(gaming_kgsl_max == 0U);
    assert(read_file(up_rate) == "0");
    assert(read_file(down_rate) == "0");

    // 1b. Release-on-Calm: cool Efficiency restores all CPU/GPU nodes cleanly to OEM baseline (zero lingering mutations)
    act.apply(topo, ContextMode::Efficiency, PerformanceProfile::Balanced, 1.0, false, true, {5.0, 15.0});
    assert(act.stats().managed_nodes == 0U);
    assert(read_file(hispeed) == "1800000");
    assert(read_file(kgsl_max) == "0");
    assert(read_file(kgsl_min) == "5");

    // 1c. Unconditional Release-on-Calm in EFFICIENCY even while cooling (scale = 0.91 < 0.95, !thermal_guard)
    act.apply(topo, ContextMode::Gaming, PerformanceProfile::Gaming, 1.0, false, true, {20.0, 70.0});
    assert(std::stoull(read_file(hispeed)) >= 2200000U);
    act.apply(topo, ContextMode::Efficiency, PerformanceProfile::Balanced, 0.91, false, true, {8.0, 0.0});
    assert(act.stats().managed_nodes == 0U);
    assert(read_file(hispeed) == "1800000");
    assert(read_file(hispeed_load) == "90");
    assert(read_file(up_rate) == "0");
    assert(read_file(kgsl_min) == "5");

    // 2. PID capacity_scale drop + thermal_guard smoothly lowers hispeed, raises hispeed_load, and raises KGSL ceiling index (slower/cooler)
    act.apply(topo, ContextMode::Gaming, PerformanceProfile::Gaming, 0.55, true, true, {10.0, 50.0});
    const uint64_t hot_hispeed = std::stoull(read_file(hispeed));
    const uint64_t hot_load = std::stoull(read_file(hispeed_load));
    const uint64_t hot_kgsl_max = std::stoull(read_file(kgsl_max));
    const uint64_t hot_kgsl_min = std::stoull(read_file(kgsl_min));
    assert(hot_hispeed < gaming_hispeed);
    assert(hot_load > gaming_load);
    assert(hot_kgsl_max > gaming_kgsl_max);
    assert(hot_kgsl_max <= hot_kgsl_min);

    // 3. OEM contention on hispeed_freq yields during 30s cooldown, then recovers once OEM settles
    write_file(hispeed, "600000\n");
    act.apply(topo, ContextMode::Gaming, PerformanceProfile::Gaming, 1.0, false, true, {20.0, 70.0}, 1000);
    assert(read_file(hispeed) == "600000");
    assert(act.stats().contended_nodes == 1U);
    assert(act.contended_paths().size() == 1U);
    act.apply(topo, ContextMode::Gaming, PerformanceProfile::Gaming, 1.0, false, true, {20.0, 70.0}, 15000);
    assert(read_file(hispeed) == "600000");
    act.apply(topo, ContextMode::Gaming, PerformanceProfile::Gaming, 1.0, false, true, {20.0, 70.0}, 35000);
    assert(std::stoull(read_file(hispeed)) >= 2200000U);
    assert(act.stats().contended_nodes == 0U);

    // 4. Excluded app restores OEM baseline cleanly (hispeed restores to the OEM-adopted 600000)
    act.apply(topo, ContextMode::Excluded, PerformanceProfile::Gaming, 1.0, false, true, {}, 36000);
    assert(read_file(hispeed) == "600000");
    assert(read_file(hispeed_load) == "90");
    assert(read_file(kgsl_max) == "0");
    assert(read_file(kgsl_min) == "5");

    assert(!baseline.has_active_mutations());
    assert(baseline.verify_and_persist_clear_state());
    (void)::unsetenv("COREFLOW_TEST_STATE_DIR");
    fs::remove_all(root);
    std::cout << "v5 ThermalEnvelopeActuator tests passed\n";
    return 0;
}
