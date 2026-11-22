#include "coreflow/frame_bridge.hpp"
#include "coreflow/v5/hardware_probe.hpp"
#include "coreflow/v6/app_dna_store.hpp"
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
    bridge.stop();

    // 2. Universal VBlank IRQ Counter Fallback (custom kernels without vendor FPS sysfs)
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
        assert(irq_sensor.supported());

        // First read establishes baseline -> neutral jank (-1.0)
        const auto v0 = irq_sensor.sample(120U, 1000ULL, true, nullptr);
        assert(v0.available && v0.jank_ratio < 0.0);
        assert(irq_sensor.active_source() == "vblank_irq:187:");

        // +120 IRQs over 1.0s -> smooth 120 Hz commit rate (jank ~ 0.0)
        write_file(irq_file,
                   "           CPU0       CPU1       CPU2       CPU3\n"
                   " 42:        200        200          0          0   GICv3  120 Level     arch_timer\n"
                   "187:       1120        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        const auto v1 = irq_sensor.sample(120U, 2000ULL, true, nullptr);
        assert(v1.available);
        assert(std::abs(v1.measured_fps - 120.0) < 0.5);
        assert(v1.jank_ratio >= 0.0 && v1.jank_ratio < 0.02);

        // Oscillating VBlank jitter (120 -> 78 -> 114 -> 76 Hz) raises jitter-penalized jank_ratio
        write_file(irq_file,
                   "187:       1198        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        (void)irq_sensor.sample(120U, 3000ULL, true, nullptr);
        write_file(irq_file,
                   "187:       1312        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        (void)irq_sensor.sample(120U, 4000ULL, true, nullptr);
        write_file(irq_file,
                   "187:       1388        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        const auto v_jit = irq_sensor.sample(120U, 5000ULL, true, nullptr);
        assert(v_jit.available && v_jit.jank_ratio > 0.08);

        // Static screen (< 12 Hz commit rate: +4 IRQs in 1s) -> neutral (-1.0), never false jank
        write_file(irq_file,
                   "187:       1392        200          0          0   GICv3  245 Edge      mdss_vblank_dsi0\n");
        const auto v_idle = irq_sensor.sample(120U, 6000ULL, true, nullptr);
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

    fs::remove_all(root);
    std::cout << "v6.2 Closed-Loop Frame Pacing, VBlank IRQ Fallback & Bayesian App DNA tests passed\n";
    return 0;
}
