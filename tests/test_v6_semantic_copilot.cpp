#include "coreflow/baseline_store.hpp"
#include "coreflow/v5/hardware_probe.hpp"
#include "coreflow/v6/oem_collaboration_map.hpp"
#include "coreflow/v6/semantic_knob_engine.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace coreflow;
using namespace coreflow::v5;
using namespace coreflow::v6;
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
    char tpl[] = "/tmp/coreflow-v6-copilot-XXXXXX";
    const char* created = ::mkdtemp(tpl);
    assert(created != nullptr);
    const fs::path root(created);
    const fs::path state = root / "state";
    const fs::path sys = root / "sys";
    const fs::path proc = root / "proc";
    fs::create_directories(state);
    assert(::setenv("COREFLOW_TEST_STATE_DIR", state.c_str(), 1) == 0);

    auto& baseline = BaselineStore::instance();
    baseline.reinitialize();
    assert(baseline.recover_pending());

    // 1. Xiaomi mi_display 120Hz live discovery + live refresh rate update
    write_file(sys / "class/mi_display/disp-DSI-0/dynamic_fps", "120\n");
    write_file(sys / "kernel/mm/lru_gen/enabled", "0x0000\n");
    write_file(sys / "kernel/mm/lru_gen/min_ttl_ms", "0\n");
    HardwareProbe probe(sys.string(), proc.string());
    HardwareTopology topo = probe.discover();
    assert(topo.refresh_hz == 120U);
    assert(!topo.refresh_rate_paths.empty());
    assert(!topo.vm.mglru_oem_enabled);

    // Switching panel refresh rate dynamically (e.g. 60Hz <-> 144Hz) is reflected live
    write_file(sys / "class/mi_display/disp-DSI-0/dynamic_fps", "144\n");
    const auto live_hz = probe.read_live_refresh_hz(topo);
    assert(live_hz.has_value() && *live_hz == 144U);

    // 2. Persistent OEM-Collaboration Map: 30s -> 2m -> 10m -> permanent yield & auto-reset on ROM/kernel change
    {
        OemCollaborationMap map1;
        map1.initialize(state.string(), "6.6.118-android15", "HyperOS-4.0-Beta");
        const std::string node = (sys / "kgsl/min_pwrlevel").string();
        map1.record_contention(node, "7", 1000);   // strike 1 -> 30s
        assert(map1.is_blocked(node, 20000));
        assert(!map1.is_blocked(node, 32000));
        assert(map1.check_cooldown_expired(node, "7", 32000));
        map1.record_contention(node, "7", 33000);  // strike 2 -> 2m (preserved strikes!)
        assert(map1.is_blocked(node, 100000));
        assert(!map1.is_blocked(node, 155000));
        map1.record_contention(node, "7", 156000); // strike 3 -> 10m
        assert(map1.is_blocked(node, 500000));
        assert(!map1.is_permanently_yielded(node));
        map1.record_contention(node, "7", 800000); // strike 4 -> permanent yield + persisted
        assert(map1.is_permanently_yielded(node));
        assert(map1.yielded_count() == 1U);

        // Reload on same kernel+ROM -> permanent yield preserved immediately at boot!
        OemCollaborationMap map2;
        map2.initialize(state.string(), "6.6.118-android15", "HyperOS-4.0-Beta");
        assert(map2.is_permanently_yielded(node));

        // Transient reset on SIGHUP preserves permanently yielded nodes
        map2.reset_transient_cooldowns();
        assert(map2.is_permanently_yielded(node));

        // Reload after OTA ROM update -> auto-resets map cleanly!
        OemCollaborationMap map3;
        map3.initialize(state.string(), "6.6.118-android15", "HyperOS-4.1-Stable");
        assert(!map3.is_permanently_yielded(node));
        assert(map3.yielded_count() == 0U);
    }

    // 3. Dynamic Semantic Knob Discovery + Safety Denylist + Instant Reflex / Release-on-Calm
    const fs::path pol4 = sys / "devices/system/cpu/cpufreq/policy4";
    const fs::path walt_boost = pol4 / "walt/boost";
    const fs::path walt_pl = pol4 / "walt/pl";
    const fs::path walt_rtg = pol4 / "walt/rtg_boost_freq";
    const fs::path bad_volt = pol4 / "walt/voltage_limit";
    const fs::path proc_walt_upmig = proc / "sys/walt/sched_upmigrate";
    const fs::path proc_ctx_burst = proc / "sys/kernel/context_burst_count";
    const fs::path gpu_dir = sys / "class/kgsl/kgsl-3d0";
    const fs::path gpu_adrenoboost = gpu_dir / "adrenoboost";
    const fs::path gpu_bus_split = gpu_dir / "bus_split";
    const fs::path bad_thermal = gpu_dir / "throttling";
    const fs::path ufshc = sys / "devices/platform/soc/1d84000.ufshc";
    const fs::path ufs_ll = ufshc / "inflight_low_latency";
    const fs::path ufs_q = ufshc / "host0/block/sda/queue";
    const fs::path ufs_ra = ufs_q / "read_ahead_kb";
    const fs::path ufs_iostats = ufs_q / "iostats";
    const fs::path ufs_rq = ufs_q / "rq_affinity";
    const fs::path vfs_cp = proc / "sys/vm/vfs_cache_pressure";
    const fs::path proc_sched_boost = proc / "sys/walt/sched_boost";
    const fs::path pol0 = sys / "devices/system/cpu/cpufreq/policy0";
    const fs::path pol0_rtg_zero = pol0 / "walt/rtg_boost_freq";

    write_file(walt_boost, "0\n");
    write_file(walt_pl, "0\n");
    write_file(walt_rtg, "1000000\n");
    write_file(pol0_rtg_zero, "0\n");
    write_file(bad_volt, "900\n");
    write_file(proc_walt_upmig, "85 95\n");
    write_file(proc_ctx_burst, "2\n");
    write_file(proc_sched_boost, "0\n");
    write_file(gpu_adrenoboost, "0\n");
    write_file(gpu_bus_split, "1\n");
    write_file(bad_thermal, "1\n");
    write_file(ufs_ll, "0\n");
    write_file(ufs_ra, "512\n");
    write_file(ufs_iostats, "1\n");
    write_file(ufs_rq, "1\n");
    write_file(vfs_cp, "100\n");

    HardwareTopology synth{};
    CpuClusterInfo little{};
    little.policy_path = pol0.string();
    little.governor = "walt";
    little.opps_khz = {691200, 1000000, 1248000, 1497600, 1804800};
    synth.cpu_clusters.push_back(little);
    CpuClusterInfo big{};
    big.policy_path = pol4.string();
    big.governor = "walt";
    big.opps_khz = {691200, 1000000, 1248000, 1497600, 1804800, 2112000, 2400000};
    synth.cpu_clusters.push_back(big);
    synth.gpu.available = true;
    synth.gpu.device_path = gpu_dir.string();
    StorageQueueInfo sq{};
    sq.device_name = "sda";
    sq.is_mapper = false;
    sq.read_ahead_kb_path = ufs_ra.string();
    synth.storage_queues.push_back(sq);
    synth.vm.vfs_cache_pressure_path = vfs_cp.string();

    OemCollaborationMap oem_map;
    oem_map.initialize(state.string(), "6.6.118-android15", "HyperOS-4.1-Stable");
    SemanticKnobEngine engine(baseline, oem_map);
    engine.discover(synth, sys.string(), proc.string());

    // Must discover all safe semantic knobs while rejecting voltage_limit, throttling,
    // Qualcomm's refcounted sched_boost, and OEM-disabled rtg_boost_freq == 0!
    assert(engine.stats().discovered_total >= 11U);
    assert(engine.stats().cpu_knobs >= 5U);
    assert(engine.stats().gpu_knobs >= 2U);
    assert(engine.stats().storage_knobs >= 3U);
    assert(engine.stats().memory_knobs == 1U);
    for (const auto& k : engine.knobs()) {
        assert(k.leaf != "voltage_limit");
        assert(k.leaf != "throttling");
        assert(k.leaf != "sched_boost");
        assert(k.path != pol0_rtg_zero.string());
    }

    // LaunchBurst -> instant reflex across CPU boost, WALT pl, upmigrate, context_burst_count, UFS latency, GPU
    engine.apply(ContextMode::LaunchBurst, PerformanceProfile::Balanced, 1.0, false, true, 0.50, 0.5, 1000);
    assert(read_file(walt_boost) == "1");
    assert(read_file(walt_pl) == "1");
    assert(read_file(walt_rtg) == "1248000"); // OEM-relative <= 1.30x baseline (1.0 MHz -> 1.248 MHz)
    assert(read_file(pol0_rtg_zero) == "0");  // OEM-disabled RTG boost never touched
    assert(read_file(proc_sched_boost) == "0");
    assert(read_file(proc_walt_upmig) == "78 87");
    assert(read_file(proc_ctx_burst) == "4");
    assert(read_file(ufs_ll) == "1");
    assert(read_file(ufs_iostats) == "0");
    assert(read_file(ufs_rq) == "2");
    assert(read_file(gpu_adrenoboost) == "1");
    assert(read_file(vfs_cp) == "80");
    assert(read_file(bad_volt) == "900");
    assert(read_file(bad_thermal) == "1");

    // Gaming -> bus_split unified (0) & adrenoboost=2, while transient LaunchBurst-only walt/boost releases to 0!
    engine.apply(ContextMode::Gaming, PerformanceProfile::Gaming, 1.0, false, true, 0.50, 0.5, 2000);
    assert(read_file(walt_boost) == "0");
    assert(read_file(gpu_bus_split) == "0");
    assert(read_file(gpu_adrenoboost) == "2");

    // Passive hands-off video playback -> releases ALL semantic knobs cleanly to OEM so VPU stays cool!
    engine.apply(ContextMode::Interactive, PerformanceProfile::Balanced, 1.0, false, true, 0.50, 0.5, 2500, true);
    assert(engine.stats().active_managed == 0U);
    assert(read_file(gpu_adrenoboost) == "0");
    assert(read_file(walt_pl) == "0");

    // Re-activate then test Release-on-Calm (Efficiency) -> ALL semantic knobs cleanly restored to OEM baselines!
    engine.apply(ContextMode::LaunchBurst, PerformanceProfile::Balanced, 1.0, false, true, 0.50, 0.5, 2800);
    engine.apply(ContextMode::Efficiency, PerformanceProfile::Balanced, 1.0, false, true, 0.50, 0.5, 3000);
    assert(engine.stats().active_managed == 0U);
    assert(read_file(walt_boost) == "0");
    assert(read_file(walt_pl) == "0");
    assert(read_file(walt_rtg) == "1000000");
    assert(read_file(proc_walt_upmig) == "85 95");
    assert(read_file(proc_ctx_burst) == "2");
    assert(read_file(ufs_ll) == "0");
    assert(read_file(ufs_iostats) == "1");
    assert(read_file(ufs_rq) == "1");
    assert(read_file(gpu_adrenoboost) == "0");
    assert(read_file(gpu_bus_split) == "1");
    assert(read_file(vfs_cp) == "100");
    assert(!baseline.has_active_mutations());

    (void)::unsetenv("COREFLOW_TEST_STATE_DIR");
    fs::remove_all(root);
    std::cout << "v6.1 Semantic Co-Pilot & OEM Collaboration tests passed\n";
    return 0;
}
