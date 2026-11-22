#include "coreflow/v5/context_engine.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace coreflow::v5;
namespace fs = std::filesystem;

static void write_file(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p);
    assert(out.good());
    out << content;
}

int main() {
    char tpl[] = "/tmp/coreflow-v5-ctx-XXXXXX";
    const char* created = ::mkdtemp(tpl);
    assert(created != nullptr);
    const fs::path root(created);
    const fs::path proc = root / "proc";

    // Synthetic foreground app + critical threads
    write_file(root / "top_app_procs", "4200\n4321\n");
    write_file(proc / "4200/cmdline", std::string("com.android.systemui\0", 21));
    write_file(proc / "4321/cmdline", std::string("com.example.game\0", 17));
    write_file(proc / "4321/task/4321/comm", "com.example.game\n");
    write_file(proc / "4321/task/4322/comm", "RenderThread\n");
    write_file(proc / "4321/task/4323/comm", "UnityMain\n");
    write_file(proc / "4321/task/4324/comm", "AsyncLogWorker\n");

    // Synthetic surfaceflinger
    write_file(proc / "900/cmdline", std::string("/system/bin/surfaceflinger\0", 27));
    write_file(proc / "900/task/900/comm", "surfaceflinger\n");
    write_file(proc / "900/task/901/comm", "RenderEngine\n");

    ContextEngine engine(proc.string());
    engine.set_profile(PerformanceProfile::Balanced);
    engine.set_app_rules({"com.example.forcedgame"}, {"com.android.camera"});

    const auto fg = engine.resolve_foreground_app((root / "top_app_procs").string());
    assert(fg.pid == 4321);
    assert(fg.package_name == "com.example.game");
    assert(fg.critical_tids.size() == 3U); // 4321 (UI), 4322 (RenderThread), 4323 (UnityMain)

    // HyperOS / Android 15 regression: top-app contains persistent daemons (adj < 0),
    // perceptible IME keyboard (adj = 200), bound Play Services (adj = 0 without RenderThread),
    // helper shell binaries (/.../bash), and the real fg app (adj = 0 with RenderThread).
    write_file(root / "hyperos_top_app", "1312\n2523\n5393\n13821\n18000\n22235\n27597\n");
    write_file(proc / "1312/cmdline", std::string("zygote64\0", 9));
    write_file(proc / "1312/oom_score_adj", "-1000\n");
    write_file(proc / "2523/cmdline", std::string("system_server\0", 14));
    write_file(proc / "2523/oom_score_adj", "-900\n");
    write_file(proc / "5393/cmdline", std::string("com.google.android.gms\0", 23));
    write_file(proc / "5393/oom_score_adj", "0\n");
    write_file(proc / "5393/task/5393/comm", "oid.gms\n");
    write_file(proc / "5393/task/5394/comm", "Binder:5393_1\n");
    write_file(proc / "13821/cmdline", std::string("com.preff.kb.xm\0", 16));
    write_file(proc / "13821/oom_score_adj", "200\n");
    write_file(proc / "18000/cmdline", std::string("com.termux\0", 11));
    write_file(proc / "18000/oom_score_adj", "0\n");
    write_file(proc / "18000/task/18000/comm", "com.termux\n");
    write_file(proc / "18000/task/18005/comm", "RenderThread\n");
    write_file(proc / "22235/cmdline", std::string("/data/data/com.termux/files/usr/bin/bash\0", 41));
    write_file(proc / "22235/oom_score_adj", "0\n");
    write_file(proc / "27597/cmdline", std::string("com.qti.qcc\0", 12));
    write_file(proc / "27597/oom_score_adj", "-800\n");

    const auto hyperos_fg = engine.resolve_foreground_app((root / "hyperos_top_app").string());
    assert(hyperos_fg.pid == 18000);
    assert(hyperos_fg.package_name == "com.termux");
    assert(hyperos_fg.critical_tids.size() == 2U);

    const auto sf_tids = engine.resolve_surfaceflinger_tids();
    assert(sf_tids.size() == 2U); // 900, 901

    // 1. Launching a new app enters LaunchBurst immediately
    ContextSignalInput sig{};
    sig.now_ms = 1000;
    sig.foreground_pid = 4321;
    sig.foreground_package = "com.example.browser";
    auto dec = engine.evaluate(sig);
    assert(dec.mode == ContextMode::LaunchBurst);
    assert(dec.newly_launched_app);

    // 2. After launch burst expires without touch/load -> Efficiency
    sig.now_ms = 4000;
    dec = engine.evaluate(sig);
    assert(dec.mode == ContextMode::Efficiency);
    assert(!dec.newly_launched_app);

    // 3. Touch notification -> Interactive immediately
    engine.notify_touch(4100);
    sig.now_ms = 4150;
    dec = engine.evaluate(sig);
    assert(dec.mode == ContextMode::Interactive);

    // 4. Sustained GPU load >= 48% for 3 ticks -> Gaming (hysteretic)
    sig.now_ms = 5000;
    sig.gpu_busy_pct = 60.0;
    assert(engine.evaluate(sig).mode == ContextMode::Efficiency);
    sig.now_ms = 6000;
    assert(engine.evaluate(sig).mode == ContextMode::Efficiency);
    sig.now_ms = 7000;
    assert(engine.evaluate(sig).mode == ContextMode::Gaming);

    // 5. Excluded app immediately yields stock OEM behavior
    sig.now_ms = 8000;
    sig.foreground_pid = 5555;
    sig.foreground_package = "com.android.camera";
    dec = engine.evaluate(sig);
    assert(dec.mode == ContextMode::Excluded);
    assert(dec.app_excluded);

    // 6. Forced gaming app enters LaunchBurst then Gaming even without GPU sensor
    sig.now_ms = 9000;
    sig.gpu_busy_pct = std::nullopt;
    sig.foreground_pid = 6666;
    sig.foreground_package = "com.example.forcedgame";
    assert(engine.evaluate(sig).mode == ContextMode::LaunchBurst);
    sig.now_ms = 12000;
    dec = engine.evaluate(sig);
    assert(dec.mode == ContextMode::Gaming && dec.app_forced_gaming);

    // 7. ScreenOff overrides everything
    sig.now_ms = 13000;
    sig.screen_off = true;
    assert(engine.evaluate(sig).mode == ContextMode::ScreenOff);

    // 8. 4-profile per-app rules (smoothness_apps & battery_apps set effective_profile cleanly)
    engine.set_profile(PerformanceProfile::Balanced);
    engine.set_app_rules(
        {"com.example.forcedgame"},
        {"com.android.camera"},
        {"com.instagram.android"},
        {"com.whatsapp"});
    sig.screen_off = false;
    sig.now_ms = 14000;
    sig.foreground_pid = 7001;
    sig.foreground_package = "com.instagram.android";
    dec = engine.evaluate(sig);
    assert(dec.effective_profile == PerformanceProfile::Smoothness);

    sig.now_ms = 17000;
    sig.foreground_pid = 7002;
    sig.foreground_package = "com.whatsapp";
    dec = engine.evaluate(sig);
    assert(dec.effective_profile == PerformanceProfile::Battery);

    // 9. GPU-less MediaTek/emulator auto-gaming via game engine thread + sustained CPU load
    assert(ContextEngine::is_game_engine_thread_name("EmuThread"));
    assert(ContextEngine::is_critical_render_thread_name("CrRendererMain"));
    sig.gpu_busy_pct = std::nullopt;
    sig.has_game_thread = true;
    sig.cpu_util_pct = 62.0;
    sig.foreground_pid = 7003;
    sig.foreground_package = "org.ppsspp.ppsspp";
    sig.now_ms = 20000;
    assert(engine.evaluate(sig).mode == ContextMode::LaunchBurst);
    sig.now_ms = 22500;
    (void)engine.evaluate(sig);
    sig.now_ms = 23500;
    assert(engine.evaluate(sig).mode == ContextMode::Gaming);

    // 10. HyperOS/Android background services without RenderThread must never hijack foreground or trigger LaunchBurst
    assert(ContextEngine::is_background_system_service("com.xiaomi.xmsf"));
    assert(ContextEngine::is_background_system_service("com.milink.service"));
    assert(ContextEngine::is_background_system_service("com.android.externalstorage"));
    assert(!ContextEngine::is_background_system_service("com.miHoYo.GenshinImpact"));
    write_file(root / "bg_only_top_app", "31001\n31002\n31003\n");
    write_file(proc / "31001/cmdline", std::string("com.xiaomi.xmsf\0", 16));
    write_file(proc / "31001/oom_score_adj", "0\n");
    write_file(proc / "31001/task/31001/comm", "com.xiaomi.xmsf\n");
    write_file(proc / "31002/cmdline", std::string("com.milink.service\0", 19));
    write_file(proc / "31002/oom_score_adj", "0\n");
    write_file(proc / "31002/task/31002/comm", "milink.service\n");
    write_file(proc / "31003/cmdline", std::string("com.android.externalstorage\0", 28));
    write_file(proc / "31003/oom_score_adj", "0\n");
    write_file(proc / "31003/task/31003/comm", "externalstorage\n");
    const auto bg_fg = engine.resolve_foreground_app((root / "bg_only_top_app").string());
    assert(bg_fg.pid == 0);
    assert(bg_fg.package_name.empty());

    // 11. HyperFluid Camera fast-launch & MediaCodec full-chain detection
    assert(ContextEngine::is_camera_package("com.android.camera"));
    assert(ContextEngine::is_camera_package("com.miui.camera"));
    assert(!ContextEngine::is_camera_package("com.whatsapp"));
    assert(ContextEngine::is_media_pipeline_thread_name("MediaCodec_loop"));
    assert(ContextEngine::is_media_pipeline_thread_name("ExoPlayer:Playback"));

    write_file(proc / "4100/cmdline", std::string("/system/bin/cameraserver\0", 25));
    write_file(proc / "4100/task/4100/comm", "cameraserver\n");
    write_file(proc / "4100/task/4101/comm", "Binder:4100_1\n");
    const auto cam_tids = engine.resolve_camera_hal_tids();
    assert(cam_tids.size() >= 2U);

    // Media playback stays smoothly Interactive without touch
    ContextSignalInput media_sig{};
    media_sig.now_ms = 40000;
    media_sig.foreground_pid = 8001;
    media_sig.foreground_package = "com.google.android.youtube";
    media_sig.has_media_thread = true;
    (void)engine.evaluate(media_sig); // LaunchBurst
    media_sig.now_ms = 45000;
    const auto media_dec = engine.evaluate(media_sig);
    assert(media_dec.mode == ContextMode::Interactive && media_dec.media_pipeline_active);

    // 12. Play Protect / HyperOS verifier & MSA background services are filtered, and rapid 200ms ticks
    // with one stale GPU spike cannot falsely latch Gaming mode within < 1.6s.
    assert(ContextEngine::is_background_system_service("com.google.android.verifier"));
    assert(ContextEngine::is_background_system_service("com.miui.guardprovider"));
    assert(ContextEngine::is_background_system_service("com.miui.msa.global"));
    ContextSignalInput fast_sig{};
    fast_sig.foreground_pid = 8002;
    fast_sig.foreground_package = "com.example.notes";
    fast_sig.now_ms = 50000;
    (void)engine.evaluate(fast_sig); // LaunchBurst
    fast_sig.gpu_busy_pct = 62.0;
    fast_sig.now_ms = 52000;
    assert(engine.evaluate(fast_sig).mode == ContextMode::Efficiency); // streak 1
    fast_sig.now_ms = 52200;
    assert(engine.evaluate(fast_sig).mode == ContextMode::Efficiency); // ignored (< 800ms)
    fast_sig.now_ms = 52400;
    assert(engine.evaluate(fast_sig).mode == ContextMode::Efficiency); // ignored (< 800ms)
    fast_sig.now_ms = 53000;
    assert(engine.evaluate(fast_sig).mode == ContextMode::Efficiency); // streak 2
    fast_sig.now_ms = 54000;
    assert(engine.evaluate(fast_sig).mode == ContextMode::Gaming);     // streak 3 (>= 2s sustained)

    // 13. Gaming profile uses hysteresis and does NOT flap EFFICIENCY <-> GAMING on a single 32% CPU tick
    engine.set_profile(PerformanceProfile::Gaming);
    ContextSignalInput gprof{};
    gprof.foreground_pid = 8003;
    gprof.foreground_package = "com.example.reader";
    gprof.now_ms = 60000;
    (void)engine.evaluate(gprof); // LaunchBurst
    gprof.now_ms = 63000;
    gprof.gpu_busy_pct = 10.0;
    gprof.cpu_util_pct = 33.0;
    assert(engine.evaluate(gprof).mode == ContextMode::Efficiency);
    gprof.now_ms = 64000;
    gprof.cpu_util_pct = 45.0;
    assert(engine.evaluate(gprof).mode == ContextMode::Efficiency); // streak 1 (needs 2 ticks)
    gprof.now_ms = 65000;
    assert(engine.evaluate(gprof).mode == ContextMode::Gaming);     // streak 2 -> latched cleanly

    // 14. Bayesian Engine Signature Detection: UnityMain thread and /proc/<pid>/maps libUE4.so -> HeavyGame3D vs StandardUi
    assert(fg.engine_class == AppEngineClass::HeavyGame3D);
    assert(hyperos_fg.engine_class == AppEngineClass::StandardUi);
    write_file(root / "ue4_top_app", "44500\n");
    write_file(proc / "44500/cmdline", std::string("com.tencent.ig\0", 15));
    write_file(proc / "44500/oom_score_adj", "0\n");
    write_file(proc / "44500/task/44500/comm", "com.tencent.ig\n");
    write_file(proc / "44500/task/44501/comm", "RenderThread\n");
    write_file(proc / "44500/maps",
               "70000000-70010000 r-xp 00000000 fd:00 1234 /data/app/com.tencent.ig/lib/arm64/libUE4.so\n");
    const auto ue4_fg = engine.resolve_foreground_app((root / "ue4_top_app").string());
    assert(ue4_fg.pid == 44500);
    assert(ue4_fg.engine_class == AppEngineClass::HeavyGame3D);

    // 15. System Launcher detection excludes launchers from LaunchBurst preread & BOOT_SETTLE soft bypass
    assert(ContextEngine::is_system_launcher_package("com.miui.home"));
    assert(ContextEngine::is_system_launcher_package("com.android.launcher3"));
    assert(ContextEngine::is_system_launcher_package("com.google.android.apps.nexuslauncher"));
    assert(!ContextEngine::is_system_launcher_package("com.whatsapp"));
    assert(!ContextEngine::is_system_launcher_package("com.miHoYo.GenshinImpact"));

    fs::remove_all(root);
    std::cout << "v5 ContextEngine tests passed\n";
    return 0;
}
