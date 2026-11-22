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
#include <sys/stat.h>
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

    // Symbiotic Hints during Interactive: cluster spread lowers WALT upmigrate further and GPU-bound lifts bus_split
    SymbioticHints hints{};
    hints.boost_gpu_memory_supply = true;
    hints.encourage_cluster_spread = true;
    engine.apply(ContextMode::Interactive, PerformanceProfile::Balanced, 1.0, false, true, 0.50, 0.5, 2700, false, hints);
    assert(read_file(gpu_bus_split) == "0");
    assert(read_file(gpu_adrenoboost) == "2");
    assert(read_file(proc_walt_upmig) == "77 86"); // -10% (-6 gaming -4 spread): stricter spread than 78 87 & 85 95

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

    // 4. Cross-SoC MediaTek GED/FPSGO & Samsung/Exynos /sys/class/panel discovery
    const fs::path mtk_ged = sys / "module/ged/parameters/ged_boost_enable";
    const fs::path mtk_fpsgo = sys / "module/mtk_fpsgo/parameters/boost_affinity";
    const fs::path exynos_panel = sys / "class/panel/panel0/refresh_rate";
    write_file(mtk_ged, "0\n");
    write_file(mtk_fpsgo, "0\n");
    write_file(exynos_panel, "90\n");

    HardwareTopology mtk_topo{};
    SemanticKnobEngine mtk_engine(baseline, oem_map);
    mtk_engine.discover(mtk_topo, sys.string(), proc.string());
    assert(mtk_engine.stats().gpu_knobs >= 1U);
    assert(mtk_engine.stats().cpu_knobs >= 1U);
    mtk_engine.apply(ContextMode::Gaming, PerformanceProfile::Gaming, 1.0, false, true, 0.50, 0.5, 4000);
    assert(read_file(mtk_ged) == "1");
    assert(read_file(mtk_fpsgo) == "1");
    mtk_engine.apply(ContextMode::Efficiency, PerformanceProfile::Balanced, 1.0, false, true, 0.50, 0.5, 5000);
    assert(read_file(mtk_ged) == "0");
    assert(read_file(mtk_fpsgo) == "0");
    assert(!baseline.has_active_mutations());

    // Remove mi_display so probe falls back to /sys/class/panel (Samsung/OnePlus/Oppo)
    fs::remove_all(sys / "class/mi_display");
    HardwareProbe exynos_probe(sys.string(), proc.string());
    const auto exynos_topo = exynos_probe.discover();
    assert(exynos_topo.refresh_hz == 90U);

    // 5. Pillar 1 & 2: Sovereign Tamper Guard (0444 lock + Reclaim-First + 3x/5s circuit breaker + force_resync)
    //    AND Coupled Devfreq Memory Bus (<= 1.25x OEM OPP floor scaling + Release-on-Calm permission restore)
    {
        const fs::path ddr_dir = sys / "class/devfreq/soc:qcom,cpu-cpu-ddr-bw";
        const fs::path ddr_min = ddr_dir / "min_freq";
        const fs::path ddr_opps = ddr_dir / "available_frequencies";
        write_file(ddr_min, "1000000\n");
        write_file(ddr_opps, "547000 768000 1000000 1144000 1248000 1555000 2092000\n");
        ::chmod(ddr_min.c_str(), 0644);
        ::chmod(gpu_adrenoboost.c_str(), 0644);

        HardwareProbe bus_probe(sys.string(), proc.string());
        HardwareTopology bus_topo = bus_probe.discover();
        assert(bus_topo.memory_bus_nodes.size() == 1U);
        bus_topo.gpu.available = true;
        bus_topo.gpu.device_path = gpu_dir.string();

        CentralMutationCoordinator coord;
        std::vector<TamperEvent> captured_events;
        coord.set_tamper_listener([&](const TamperEvent& ev) { captured_events.push_back(ev); });

        SemanticKnobEngine sov_engine(baseline, oem_map, &coord);
        sov_engine.discover(bus_topo, sys.string(), proc.string());
        assert(sov_engine.stats().memory_bus_knobs == 1U);

        // Coupled memory_bus_scale = 1.25x -> selects 1248000 (<= 1.25x of 1000000, never 1555000) and locks 0444!
        SymbioticHints bus_hints{};
        bus_hints.memory_bus_scale = 1.25;
        sov_engine.apply(ContextMode::Gaming, PerformanceProfile::Gaming, 1.0, false, true, 0.50, 0.5, 10000, false, bus_hints);
        assert(read_file(ddr_min) == "1248000");
        assert(read_file(gpu_adrenoboost) == "2");

        struct stat st_locked{};
        assert(::lstat(gpu_adrenoboost.c_str(), &st_locked) == 0);
        assert((st_locked.st_mode & 0777) == 0444);
        assert(baseline.locked_node_count() >= 2U);

        // Simulate external root tool (e.g. FKM) chmod'ing & tampering adrenoboost to "0" -> Reclaim-First!
        auto simulate_fkm_tamper = [&](uint64_t ts_ms) {
            assert(::chmod(gpu_adrenoboost.c_str(), 0644) == 0);
            write_file(gpu_adrenoboost, "0\n");
            sov_engine.apply(ContextMode::Gaming, PerformanceProfile::Gaming, 1.0, false, true, 0.50, 0.5, ts_ms, false, bus_hints);
        };

        // Attempt 1 & 2 within 5s window -> reclaimed to "2" and re-locked 0444!
        simulate_fkm_tamper(11000);
        assert(read_file(gpu_adrenoboost) == "2");
        assert(::lstat(gpu_adrenoboost.c_str(), &st_locked) == 0 && (st_locked.st_mode & 0777) == 0444);
        assert(captured_events.size() == 1U && captured_events.back().reclaimed && !captured_events.back().circuit_open);

        simulate_fkm_tamper(12000);
        assert(read_file(gpu_adrenoboost) == "2");
        assert(captured_events.size() == 2U && captured_events.back().reclaimed && !captured_events.back().circuit_open);

        // Attempt 3 within 5s window -> trips circuit breaker (30s quarantine, zero write-loop, re-locks 0444)
        simulate_fkm_tamper(13000);
        assert(captured_events.size() == 3U && captured_events.back().circuit_open);
        assert(coord.is_node_quarantined(gpu_adrenoboost.string(), 14000));
        assert(coord.quarantined_node_count(14000) == 1U);

        // Inline Reply RESYNC / ENFORCE clears quarantine and immediately reclaims & re-locks 0444!
        assert(sov_engine.force_resync_node(gpu_adrenoboost.string(), 14500));
        assert(!coord.is_node_quarantined(gpu_adrenoboost.string(), 14500));
        assert(read_file(gpu_adrenoboost) == "2");
        assert(::lstat(gpu_adrenoboost.c_str(), &st_locked) == 0 && (st_locked.st_mode & 0777) == 0444);

        // Release-on-Calm (Efficiency) unlocks 0444 back to 0644 and restores factory values!
        sov_engine.apply(ContextMode::Efficiency, PerformanceProfile::Balanced, 1.0, false, true, 0.50, 0.5, 16000);
        assert(read_file(ddr_min) == "1000000");
        assert(read_file(gpu_adrenoboost) == "0");
        struct stat st_restored{};
        assert(::lstat(gpu_adrenoboost.c_str(), &st_restored) == 0);
        assert((st_restored.st_mode & 0777) == 0644);
        assert(baseline.locked_node_count() == 0U);
        assert(!baseline.has_active_mutations());

        // 6. Thermal sibling max_freq cap on memory bus devfreq:
        //    If OEM thermal daemon lowers ddr/max_freq to 1144000, CFAE must NEVER write min_freq > 1144000!
        const fs::path ddr_max = ddr_dir / "max_freq";
        write_file(ddr_max, "1144000\n");
        sov_engine.apply(ContextMode::Gaming, PerformanceProfile::Gaming, 1.0, false, true, 0.50, 0.5, 17000, false, bus_hints);
        assert(read_file(ddr_min) == "1144000");

        // 7. SIGKILL crash-recovery permission safety:
        //    Simulate abrupt daemon crash while gpu_adrenoboost is locked 0444, then reinitialize BaselineStore
        //    from CFJ4 journal and verify recover_if_needed() restores BOTH factory value AND writable 0644 mode!
        assert(::lstat(gpu_adrenoboost.c_str(), &st_locked) == 0 && (st_locked.st_mode & 0777) == 0444);
        baseline.reinitialize();
        assert(baseline.recover_pending());
        assert(read_file(gpu_adrenoboost) == "0");
        assert(read_file(ddr_min) == "1000000");
        assert(::lstat(gpu_adrenoboost.c_str(), &st_restored) == 0);
        assert((st_restored.st_mode & 0777) == 0644);

        // 8. Persistent multi-window root OEM daemon escalation (e.g. mi_thermald / perf-hal):
        //    After 2 circuit-breaker trips across quarantine windows, CFAE must cleanly unlock 0444,
        //    roll back, and hand over to OemCollaborationMap instead of fighting forever.
        CentralMutationCoordinator oem_coord;
        OemCollaborationMap oem_escalate_map;
        SemanticKnobEngine oem_engine(baseline, oem_escalate_map, &oem_coord);
        oem_engine.discover(bus_topo, sys.string(), proc.string());
        oem_engine.apply(ContextMode::Gaming, PerformanceProfile::Gaming, 1.0, false, true, 0.50, 0.5, 20000, false, bus_hints);
        assert(read_file(gpu_adrenoboost) == "2");
        auto simulate_oem_overwrite = [&](uint64_t ts_ms) {
            assert(::chmod(gpu_adrenoboost.c_str(), 0644) == 0);
            write_file(gpu_adrenoboost, "0\n");
            oem_engine.apply(ContextMode::Gaming, PerformanceProfile::Gaming, 1.0, false, true, 0.50, 0.5, ts_ms, false, bus_hints);
        };
        // Window 1: 3 overwrites -> 1st circuit breaker trip (30s quarantine)
        simulate_oem_overwrite(21000);
        simulate_oem_overwrite(22000);
        simulate_oem_overwrite(23000);
        assert(oem_coord.breaker_trip_count(gpu_adrenoboost.string()) == 1U);
        assert(!oem_escalate_map.is_blocked(gpu_adrenoboost.string(), 24000));
        // Window 2 (after 30s quarantine expires at 53000): 3 more overwrites -> 2nd breaker trip -> yields to OEM map & unlocks!
        simulate_oem_overwrite(54000);
        simulate_oem_overwrite(55000);
        simulate_oem_overwrite(56000);
        assert(oem_coord.breaker_trip_count(gpu_adrenoboost.string()) == 2U);
        assert(oem_escalate_map.is_blocked(gpu_adrenoboost.string(), 57000));
        assert(::lstat(gpu_adrenoboost.c_str(), &st_restored) == 0 && (st_restored.st_mode & 0777) == 0644);
        oem_engine.rollback_all();
    }

    (void)::unsetenv("COREFLOW_TEST_STATE_DIR");
    fs::remove_all(root);
    std::cout << "v6.0.0-R Sovereign Tamper Guard, Coupled Memory Bus & Cross-SoC Semantic Co-Pilot tests passed\n";
    return 0;
}
