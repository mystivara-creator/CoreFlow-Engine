#include "coreflow/frame_bridge.hpp"
#include "coreflow/v5/hardware_probe.hpp"
#include "coreflow/v6/app_dna_store.hpp"
#include "coreflow/v6/boot_calm_sanctuary.hpp"
#include "coreflow/v6/frame_pacing_sensor.hpp"

#include <cassert>
#include <cmath>
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

int main() {
    char tpl[] = "/tmp/coreflow-v62-intel-XXXXXX";
    const char* created = ::mkdtemp(tpl);
    assert(created != nullptr);
    const fs::path root(created);
    const fs::path sys = root / "sys";
    const fs::path proc = root / "proc";
    const fs::path state = root / "state";
    fs::create_directories(state);

    // 1. FramePacingSensor: zero-fork discovery, natural cadence anchoring, static guard, jank emission
    const fs::path sde_fps = sys / "class/drm/sde-crtc-0/measured_fps";
    write_file(sde_fps, "fps: 119.6 duration: 1000.0 frame_count: 120\n");

    FramePacingSensor sensor;
    sensor.discover(sys.string(), proc.string());
    assert(sensor.supported());

    AndroidFrameEventBridge bridge;
    uint32_t timeline_events = 0;
    uint32_t jank_windows = 0;
    assert(bridge.start([&](const FrameEvent& ev) {
        if (ev.kind == FrameEventKind::FrameTimelineWindow) {
            ++timeline_events;
            if (ev.missed_frames > 0U) ++jank_windows;
        }
    }));

    // Smooth 120fps on 120Hz panel -> jank_ratio ~ 0.0, FrameTimelineWindow emitted with 0 missed
    const auto s_smooth = sensor.sample(120U, 1000ULL, true, &bridge);
    assert(s_smooth.available);
    assert(std::abs(s_smooth.measured_fps - 119.6) < 0.2);
    assert(s_smooth.jank_ratio >= 0.0 && s_smooth.jank_ratio < 0.02);
    assert(timeline_events == 1U && jank_windows == 0U);

    // Natural 60fps video/game on 120Hz panel -> anchored to 60fps cadence (no false jank!)
    write_file(sde_fps, "fps: 59.8 duration: 1000.0 frame_count: 60\n");
    const auto s_60 = sensor.sample(120U, 2000ULL, true, &bridge);
    assert(s_60.available);
    assert(s_60.target_fps <= 60.5);
    assert(s_60.jank_ratio >= 0.0 && s_60.jank_ratio < 0.03);
    assert(jank_windows == 0U);

    // Real stutter: 47fps during active 120Hz workload -> jank detected & missed_frames > 0
    write_file(sde_fps, "fps: 47.0 duration: 1000.0 frame_count: 47\n");
    (void)sensor.sample(120U, 3000ULL, true, &bridge);
    const auto s_jank = sensor.sample(120U, 4000ULL, true, &bridge);
    assert(s_jank.available);
    assert(s_jank.jank_ratio >= 0.08);
    assert(jank_windows >= 1U);

    // Static reading screen (< 12fps even while active) -> neutral jank_ratio (-1.0), never false jank
    write_file(sde_fps, "fps: 2.0 duration: 1000.0 frame_count: 2\n");
    const auto s_static = sensor.sample(120U, 5000ULL, true, &bridge);
    assert(s_static.available);
    assert(s_static.jank_ratio < 0.0);

    // Stale/inconsistent SDE payload (non-zero fps with frame_count: 0) -> treated as static (neutral -1.0)
    write_file(sde_fps, "fps: 120.0 duration: 1000.0 frame_count: 0\n");
    const auto s_stale = sensor.sample(120U, 6000ULL, true, &bridge);
    assert(s_stale.available && s_stale.jank_ratio < 0.0);
    bridge.stop();

    // 2a. Non-LTPO Fixed-Hz Panel Quarantine (strict fail-closed probation on VBlank IRQ & generic panel/fps)
    {
        const fs::path nl_sys = root / "sys_nonltpo";
        const fs::path nl_proc = root / "proc_nonltpo";
        fs::create_directories(nl_sys);
        const fs::path irq_file = nl_proc / "interrupts";
        write_file(irq_file,
                   "           CPU0       CPU1       CPU2       CPU3\n"
                   "187:       1000        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");

        HardwareProbe probe(nl_sys.string(), nl_proc.string());
        const auto topo = probe.discover();
        assert(topo.vblank_irq_token == "187:");

        FramePacingSensor fixed_sensor;
        fixed_sensor.discover(nl_sys.string(), nl_proc.string(), topo.vblank_irq_token);
        assert(fixed_sensor.supported());
        assert(!fixed_sensor.verified()); // Tier B starts unproven!

        // While unproven, active workload at constant 120 Hz MUST stay fail-closed (never emit 0% jank!)
        const auto p0 = fixed_sensor.sample(120U, 1000ULL, true, nullptr, false);
        assert(!p0.available && p0.jank_ratio < 0.0);
        write_file(irq_file, "187:       1120        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        const auto p1 = fixed_sensor.sample(120U, 2000ULL, true, nullptr, false);
        assert(!p1.available && p1.jank_ratio < 0.0 && !p1.verified_commit_source);

        // 3 consecutive static-idle probes where VBlank remains pinned at 120 Hz -> permanent per-boot quarantine!
        write_file(irq_file, "187:       1240        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        const auto i1 = fixed_sensor.sample(120U, 3000ULL, false, nullptr, true);
        assert(!i1.available && !i1.quarantined_fixed_hz);
        write_file(irq_file, "187:       1360        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        const auto i2 = fixed_sensor.sample(120U, 4000ULL, false, nullptr, true);
        assert(!i2.available && !i2.quarantined_fixed_hz);
        write_file(irq_file, "187:       1480        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        const auto i3 = fixed_sensor.sample(120U, 5000ULL, false, nullptr, true);
        assert(!i3.available && i3.quarantined_fixed_hz && i3.jank_ratio < 0.0);
        assert(!fixed_sensor.supported() && fixed_sensor.quarantined_fixed_hz());
        assert(fixed_sensor.active_source() == "fail_closed:fixed_hz_panel");

        // Quarantine persists across reset() (ScreenOff/ScreenOn) for the entire boot session!
        fixed_sensor.reset();
        assert(fixed_sensor.quarantined_fixed_hz() && !fixed_sensor.supported());
        write_file(irq_file, "187:       1600        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        const auto after_q = fixed_sensor.sample(120U, 7000ULL, true, nullptr, false);
        assert(!after_q.available && after_q.quarantined_fixed_hz && after_q.jank_ratio < 0.0);

        // Generic Tier-B panel/panel/fps stuck at "120" is also quarantined fail-closed after 3 idle probes
        const fs::path gp_sys = root / "sys_panel_fps";
        const fs::path gp_proc = root / "proc_panel_fps";
        fs::create_directories(gp_proc);
        write_file(gp_sys / "class/panel/panel/fps", "120\n");
        FramePacingSensor panel_sensor;
        panel_sensor.discover(gp_sys.string(), gp_proc.string());
        assert(!panel_sensor.verified());
        assert(!panel_sensor.sample(120U, 1000ULL, true, nullptr, false).available);
        assert(!panel_sensor.sample(120U, 2000ULL, false, nullptr, true).quarantined_fixed_hz);
        assert(!panel_sensor.sample(120U, 3000ULL, false, nullptr, true).quarantined_fixed_hz);
        const auto gp_q = panel_sensor.sample(120U, 4000ULL, false, nullptr, true);
        assert(gp_q.quarantined_fixed_hz && !gp_q.available && gp_q.jank_ratio < 0.0);
    }

    // 2b. Proven Commit-Driven / Dynamic LTPO VBlank IRQ Fallback (drops on idle -> promoted to ProvenCommit)
    {
        const fs::path fb_sys = root / "sys_novendor";
        const fs::path fb_proc = root / "proc_novendor";
        fs::create_directories(fb_sys);
        const fs::path irq_file = fb_proc / "interrupts";
        write_file(irq_file,
                   "           CPU0       CPU1       CPU2       CPU3\n"
                   " 42:        100        200          0          0   GICv3  120 Level     arch_timer\n"
                   "187:       1000        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");

        HardwareProbe probe(fb_sys.string(), fb_proc.string());
        const auto topo = probe.discover();
        assert(topo.vblank_irq_token == "187:");

        FramePacingSensor irq_sensor;
        irq_sensor.discover(fb_sys.string(), fb_proc.string(), topo.vblank_irq_token);
        assert(irq_sensor.supported() && !irq_sensor.verified());

        // Static-idle probation probe: baseline + drop to 4 Hz (< 18 Hz) proves commit-driven/LTPO panel!
        const auto idle0 = irq_sensor.sample(120U, 1000ULL, false, nullptr, true);
        assert(!idle0.available && !irq_sensor.verified());
        write_file(irq_file, "187:       1004        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        const auto idle1 = irq_sensor.sample(120U, 2000ULL, false, nullptr, true);
        assert(!idle1.available && idle1.jank_ratio < 0.0);
        assert(irq_sensor.verified());
        assert(irq_sensor.active_source() == "vblank_irq:187:");

        // Now active workload (+120 IRQs over 1.0s) -> verified smooth 120 Hz commit rate (jank ~ 0.0)
        write_file(irq_file,
                   "           CPU0       CPU1       CPU2       CPU3\n"
                   " 42:        200        200          0          0   GICv3  120 Level     arch_timer\n"
                   "187:       1124        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        const auto v1 = irq_sensor.sample(120U, 3000ULL, true, nullptr, false);
        assert(v1.available && v1.verified_commit_source);
        assert(std::abs(v1.measured_fps - 120.0) < 0.5);
        assert(v1.jank_ratio >= 0.0 && v1.jank_ratio < 0.02);

        // Oscillating VBlank jitter (120 -> 78 -> 114 -> 76 Hz) raises jitter-penalized jank_ratio
        write_file(irq_file,
                   "187:       1202        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        (void)irq_sensor.sample(120U, 4000ULL, true, nullptr, false);
        write_file(irq_file,
                   "187:       1316        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        (void)irq_sensor.sample(120U, 5000ULL, true, nullptr, false);
        write_file(irq_file,
                   "187:       1392        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        const auto v_jit = irq_sensor.sample(120U, 6000ULL, true, nullptr, false);
        assert(v_jit.available && v_jit.jank_ratio > 0.08);

        // Static screen (< 12 Hz commit rate: +4 IRQs in 1s) -> neutral (-1.0), never false jank
        write_file(irq_file,
                   "187:       1396        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        const auto v_idle = irq_sensor.sample(120U, 7000ULL, true, nullptr, false);
        assert(v_idle.available && v_idle.jank_ratio < 0.0);
    }

    // 3. AppDnaStore: Bayesian Engine Signature Seeding, EWMA convergence, LRU eviction, atomic CFDNA1 persistence
    {
        AppDnaStore store;
        store.initialize(state.string());
        assert(store.learned_count() == 0U);
        assert(std::abs(store.uclamp_bias_for("com.whatsapp") - 1.0) < 1e-9);

        // Bayesian Engine Signature Seeding: cold-start 3D game immediately receives 1.10 prior on frame 1!
        store.record_launch("com.kurogame.wutheringwaves", 500ULL, EngineSignature::HeavyGame3D);
        assert(std::abs(store.uclamp_bias_for("com.kurogame.wutheringwaves") - 1.10) < 1e-6);

        // Standard UI/media app starts at neutral 1.00 prior
        store.record_launch("com.whatsapp", 1000ULL, EngineSignature::StandardUi);
        assert(std::abs(store.uclamp_bias_for("com.whatsapp") - 1.0) < 1e-9);

        // Organic EWMA convergence trims a smoothly running game gently down from its 1.10 Bayesian prior
        for (uint64_t i = 0; i < 6; ++i) {
            store.observe("com.kurogame.wutheringwaves", 0.002, 0.2, 1200ULL + i * 1000ULL, EngineSignature::HeavyGame3D);
        }
        const double game_converged = store.uclamp_bias_for("com.kurogame.wutheringwaves");
        assert(game_converged < 1.10 && game_converged >= 1.00);

        // Smooth lightweight app trims its UClamp bias below 1.0
        for (uint64_t i = 0; i < 20; ++i) {
            store.observe("com.whatsapp", 0.002, 0.1, 2000ULL + i * 1000ULL);
        }
        const double smooth_bias = store.uclamp_bias_for("com.whatsapp");
        assert(smooth_bias < 0.96 && smooth_bias >= 0.78);

        // Heavy janky app raises its UClamp bias above 1.0
        store.record_launch("com.miHoYo.GenshinImpact", 25000ULL, EngineSignature::HeavyGame3D);
        for (uint64_t i = 0; i < 15; ++i) {
            store.observe("com.miHoYo.GenshinImpact", 0.12, 0.8, 26000ULL + i * 1000ULL, EngineSignature::HeavyGame3D);
        }
        const double heavy_bias = store.uclamp_bias_for("com.miHoYo.GenshinImpact");
        assert(heavy_bias > 1.10 && heavy_bias <= 1.18);

        // Flush atomically and reload into fresh AppDnaStore instance (simulating reboot + CFDNA1 compatibility)
        assert(store.flush_if_dirty());
        AppDnaStore reloaded;
        reloaded.initialize(state.string());
        assert(reloaded.learned_count() == 3U);
        assert(reloaded.uclamp_bias_for("com.whatsapp") < 0.96);
        assert(reloaded.uclamp_bias_for("com.miHoYo.GenshinImpact") > 1.10);
        assert(std::abs(reloaded.uclamp_bias_for("com.kurogame.wutheringwaves") - game_converged) < 1e-3);

        // LRU cap at 64 entries evicts oldest entries cleanly and persists bounded size
        for (size_t i = 0; i < 70U; ++i) {
            reloaded.record_launch("com.lru.pkg" + std::to_string(i), 50000ULL + i * 10ULL);
        }
        assert(reloaded.learned_count() == AppDnaStore::kMaxEntries);
        assert(reloaded.flush_if_dirty());
        AppDnaStore reloaded_lru;
        reloaded_lru.initialize(state.string());
        assert(reloaded_lru.learned_count() == AppDnaStore::kMaxEntries);
    }

    // 4. BootCalmSanctuary: 3-phase kernel quiescence gate, Screen-On Gate, Touch Soft Bypass & alpha(t) ramp
    {
        write_file(proc / "uptime", "28.45 190.12\n");
        const double parsed_uptime = BootCalmSanctuary::read_kernel_uptime_sec(proc.string());
        assert(std::abs(parsed_uptime - 28.45) < 1e-3);

        // 4a. Warm daemon restart (uptime >= 120s) skips BOOT_SETTLE immediately
        BootCalmSanctuary warm;
        warm.initialize(320.0, 10000ULL);
        assert(!warm.is_settling());
        assert(warm.exit_reason() == "warm_daemon_restart");
        const auto warm_dec = warm.evaluate({321.0, 11000ULL, false, false, 15.0, 1.0, 38.0, 0.0, false, true});
        assert(!warm_dec.boot_settle_active && warm_dec.allow_learning && warm_dec.allow_storage_and_symbiotic);
        assert(std::abs(warm_dec.ramp_alpha - 1.0) < 1e-9);
        assert(warm_dec.exit_reason == "warm_daemon_restart");

        // 4b. Cold boot: Phase 1 sanctuary, Touch-Aware Soft Bypass, Screen-On Gate, Quiescence Gate, Phase 3 Ramp
        BootCalmSanctuary cold;
        cold.initialize(12.0, 1000ULL);
        assert(cold.is_settling());

        // At uptime=20s (below 35s min): stays in BOOT_SETTLE, learning frozen; real-app touch allows soft UClamp ONLY
        const auto d_launcher_touch = cold.evaluate({20.0, 9000ULL, false, false, 18.0, 2.0, 42.0, 0.1, true, false});
        assert(d_launcher_touch.boot_settle_active && !d_launcher_touch.allow_learning);
        assert(!d_launcher_touch.allow_touch_soft_uclamp);
        assert(!d_launcher_touch.allow_envelope_and_semantic && !d_launcher_touch.allow_storage_and_symbiotic);

        const auto d_real_touch = cold.evaluate({21.0, 10000ULL, false, false, 18.0, 2.0, 42.0, 0.1, true, true});
        assert(d_real_touch.boot_settle_active && !d_real_touch.allow_learning);
        assert(d_real_touch.allow_touch_soft_uclamp);

        // Post-boot CPU storm (75%) resets quiescence streak; hot SoC (56C) or rising slope (+2.5C/m) blocks exit
        assert(cold.evaluate({36.0, 25000ULL, false, false, 75.0, 2.0, 44.0, 0.0, false, false}).boot_settle_active);
        assert(cold.evaluate({37.0, 26000ULL, false, false, 20.0, 2.0, 56.0, 0.0, false, false}).boot_settle_active);
        assert(cold.evaluate({42.0, 31000ULL, false, false, 20.0, 2.0, 46.0, 2.2, false, false}).boot_settle_active);

        // Screen-On Gate: even when uptime >= 35s + calm >= 5s + temp < 52C + slope <= 1.0, ScreenOff MUST defer exit!
        const auto d_screen_off = cold.evaluate({44.0, 33000ULL, true, false, 15.0, 1.0, 42.0, 0.2, false, false});
        assert(d_screen_off.boot_settle_active && !d_screen_off.needs_late_baseline_snapshot);

        // Screen turns ON with quiescence satisfied -> exits with quiescence_met + one-shot Late-Baseline Snapshot!
        const auto d_exit = cold.evaluate({45.0, 34000ULL, false, false, 15.0, 1.0, 42.0, 0.2, false, false});
        assert(!d_exit.boot_settle_active);
        assert(d_exit.needs_late_baseline_snapshot);
        assert(d_exit.allow_learning);
        assert(d_exit.exit_reason == "quiescence_met");
        assert(d_exit.allow_uclamp && !d_exit.allow_envelope_and_semantic && !d_exit.allow_storage_and_symbiotic);
        assert(std::abs(d_exit.settle_duration_sec - 33.0) < 0.05);

        // Phase 3 Exponential Soft-Start Ramp: alpha(t) = 1 - exp(-t / 15s)
        // t = 5s: UClamp only
        const auto r5 = cold.evaluate({50.0, 39000ULL, false, false, 20.0, 1.0, 42.0, 0.0, false, true});
        assert(!r5.needs_late_baseline_snapshot);
        assert(r5.allow_uclamp && !r5.allow_envelope_and_semantic && !r5.allow_storage_and_symbiotic);
        assert(std::abs(r5.ramp_alpha - (1.0 - std::exp(-5.0 / 15.0))) < 1e-3);

        // t = 15s: +ThermalEnvelope & SemanticKnobs
        const auto r15 = cold.evaluate({60.0, 49000ULL, false, false, 20.0, 1.0, 42.0, 0.0, false, true});
        assert(r15.allow_uclamp && r15.allow_envelope_and_semantic && !r15.allow_storage_and_symbiotic);
        assert(std::abs(r15.ramp_alpha - (1.0 - std::exp(-1.0))) < 1e-3);

        // t = 25s: +StorageMemory & SymbioticOrchestrator
        const auto r25 = cold.evaluate({70.0, 59000ULL, false, false, 20.0, 1.0, 42.0, 0.0, false, true});
        assert(r25.allow_uclamp && r25.allow_envelope_and_semantic && r25.allow_storage_and_symbiotic);
        assert(r25.ramp_alpha > r15.ramp_alpha && r25.ramp_alpha < 1.0);

        // t = 30s: ramp complete -> alpha = 1.0, full 1.0x authority
        const auto r30 = cold.evaluate({75.0, 64000ULL, false, false, 20.0, 1.0, 42.0, 0.0, false, true});
        assert(std::abs(r30.ramp_alpha - 1.0) < 1e-9);
        assert(std::abs(r30.ramp_uclamp_scale - 1.0) < 1e-9);
        assert(std::abs(cold.last_ramp_alpha() - 1.0) < 1e-9);
        assert(cold.settle_duration_sec() > 30.0);

        // 4c. Max uptime timeout (120s) exits cleanly once screen is ON even under persistent storm
        BootCalmSanctuary stormy;
        stormy.initialize(10.0, 1000ULL);
        assert(stormy.evaluate({125.0, 116000ULL, true, false, 92.0, 20.0, 60.0, 3.0, false, false}).boot_settle_active);
        const auto d_timeout = stormy.evaluate({126.0, 117000ULL, false, false, 92.0, 20.0, 60.0, 3.0, false, false});
        assert(!d_timeout.boot_settle_active && d_timeout.needs_late_baseline_snapshot);
        assert(d_timeout.exit_reason == "max_uptime_timeout");
    }

    // 5. Pillar 3: Zero-Wear Context Learning (CFDNA2 cost-to-gain RAM modulation, significant-drift gating, CFDNA1 compat)
    {
        const fs::path zw_state = root / "state_zerowear";
        fs::create_directories(zw_state);
        // Seed legacy CFDNA1 file and verify seamless backward-compatible load
        write_file(zw_state / "app_dna.map",
                   "CFDNA1\ncom.legacy.app 0.9400 0.5000 0.0100 4 12 9000\n");
        AppDnaStore zw;
        zw.initialize(zw_state.string());
        assert(zw.learned_count() == 1U);
        assert(std::abs(zw.uclamp_bias_for("com.legacy.app") - 0.94) < 1e-3);
        assert(std::abs(zw.efficiency_weight_for("com.legacy.app") - 1.0) < 1e-3);
        assert(!zw.has_significant_drift());
        assert(zw.flush_if_significant());
        assert(zw.flush_count() == 0U); // Zero flash wear when no significant drift occurred!

        // Wake ramp samples are ignored completely so cold transitions never pollute DNA
        ContextLearningSample wake_sample{};
        wake_sample.jank_ratio = 0.20;
        wake_sample.in_wake_ramp = true;
        zw.observe_context("com.legacy.app", wake_sample, 10000ULL);
        assert(!zw.has_significant_drift());
        assert(zw.flush_if_significant());
        assert(zw.flush_count() == 0U);

        // Single tiny smooth sample (< 0.025 drift) stays in RAM — zero disk flush!
        ContextLearningSample tiny_sample{};
        tiny_sample.jank_ratio = 0.002;
        tiny_sample.measured_fps = 60.0;
        tiny_sample.target_fps = 60.0;
        tiny_sample.power_mw = 1200.0;
        zw.observe_context("com.legacy.app", tiny_sample, 11000ULL);
        assert(!zw.has_significant_drift());
        assert(zw.flush_if_significant());
        assert(zw.flush_count() == 0U);

        // Sustained high-cost low-battery samples shift bias & efficiency_weight significantly -> triggers 1 atomic CFDNA2 flush!
        ContextLearningSample costly_sample{};
        costly_sample.jank_ratio = 0.004;
        costly_sample.measured_fps = 59.5;
        costly_sample.target_fps = 60.0;
        costly_sample.thermal_velocity_cpm = 3.2;
        costly_sample.power_mw = 5200.0;
        costly_sample.low_battery = true;
        for (uint64_t i = 0; i < 6; ++i) {
            zw.observe_context("com.legacy.app", costly_sample, 12000ULL + i * 1000ULL);
        }
        assert(zw.has_significant_drift());
        assert(zw.flush_if_significant());
        assert(zw.flush_count() == 1U);
        assert(!zw.has_significant_drift()); // Clean after flush!

        // Verify CFDNA2 header written and reloads efficiency_weight accurately
        std::ifstream in(zw_state / "app_dna.map");
        std::string hdr;
        std::getline(in, hdr);
        assert(hdr == "CFDNA2");
        AppDnaStore zw2;
        zw2.initialize(zw_state.string());
        assert(zw2.uclamp_bias_for("com.legacy.app") < 0.90);
        assert(zw2.efficiency_weight_for("com.legacy.app") < 0.98);
    }

    fs::remove_all(root);
    std::cout << "v6.0.0-R Closed-Loop Frame Pacing, Zero-Wear CFDNA2 & Boot-Calm Sanctuary tests passed\n";
    return 0;
}
