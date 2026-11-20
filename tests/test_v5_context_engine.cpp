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

    fs::remove_all(root);
    std::cout << "v5 ContextEngine tests passed\n";
    return 0;
}
