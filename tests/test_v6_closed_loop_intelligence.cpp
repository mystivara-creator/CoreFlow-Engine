#include "coreflow/frame_bridge.hpp"
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

    // 2. AppDnaStore: EWMA closed-loop convergence, LRU eviction, atomic cross-reboot persistence
    {
        AppDnaStore store;
        store.initialize(state.string());
        assert(store.learned_count() == 0U);
        assert(std::abs(store.uclamp_bias_for("com.whatsapp") - 1.0) < 1e-9);

        // Smooth lightweight app trims its UClamp bias below 1.0
        store.record_launch("com.whatsapp", 1000ULL);
        for (uint64_t i = 0; i < 20; ++i) {
            store.observe("com.whatsapp", 0.002, 0.1, 2000ULL + i * 1000ULL);
        }
        const double smooth_bias = store.uclamp_bias_for("com.whatsapp");
        assert(smooth_bias < 0.96 && smooth_bias >= 0.78);

        // Heavy janky app raises its UClamp bias above 1.0
        store.record_launch("com.miHoYo.GenshinImpact", 25000ULL);
        for (uint64_t i = 0; i < 15; ++i) {
            store.observe("com.miHoYo.GenshinImpact", 0.12, 0.8, 26000ULL + i * 1000ULL);
        }
        const double heavy_bias = store.uclamp_bias_for("com.miHoYo.GenshinImpact");
        assert(heavy_bias > 1.04 && heavy_bias <= 1.18);

        // Flush atomically and reload into fresh AppDnaStore instance (simulating reboot)
        assert(store.flush_if_dirty());
        AppDnaStore reloaded;
        reloaded.initialize(state.string());
        assert(reloaded.learned_count() == 2U);
        assert(reloaded.uclamp_bias_for("com.whatsapp") < 0.96);
        assert(reloaded.uclamp_bias_for("com.miHoYo.GenshinImpact") > 1.04);

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
    std::cout << "v6.2 Closed-Loop Frame Pacing & App DNA Store tests passed\n";
    return 0;
}
