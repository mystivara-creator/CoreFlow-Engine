#include "coreflow/foreground_tracker.hpp"
#include "coreflow/frame_telemetry.hpp"
#include "coreflow/gpu_busy_source.hpp"
#include "coreflow/render_target.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
int failures = 0;
unsigned calls = 0;
bool failDump = false;
std::string layerDump = "com.example.game/Main#1\n";
std::string frameDump = "16666667\n1 100000000 99000000\n2 116666667 115000000\n";
coreflow::process::BoundedResult fakeDump(const std::vector<std::string>& argv,
    std::uint32_t, std::size_t) noexcept {
    ++calls;
    coreflow::process::BoundedResult r;
    r.started = true; r.exit_status = failDump ? 1 : 0;
    r.output = argv.size() == 3U && argv[2] == "--list" ? layerDump : frameDump;
    return r;
}
#define CHECK(expr) do { if (!(expr)) { ++failures; std::cerr << "FAIL: " #expr " line " << __LINE__ << '\n'; } } while (false)
}

int main() {
    using namespace coreflow::observe;

    CHECK(selectRenderLayer("com.example.game/Main#1\n", "com.example.game") == "com.example.game/Main#1");
    CHECK(selectRenderLayer("com.example.game.other/Main\n", "com.example.game").empty());
    CHECK(selectRenderLayer("com.example.game/A\ncom.example.game/B\n", "com.example.game").empty());
    CHECK(selectRenderLayer("com.example.game/Main\n", "bad;command").empty());
    CHECK(parseLatencyRefreshPeriodNs("ERROR\n16666667\n", false) == 0U);
    CHECK(parseLatencyRefreshPeriodNs("8333333\n", false) == 8333333U);
    CHECK(parseLatencyRefreshPeriodNs("", false) == 0U);
    FrameTelemetrySource telemetry(fakeDump);
    CHECK(telemetry.poll(1000, false).empty() && calls == 0U);
    telemetry.setForeground("com.example.game:worker");
    CHECK(telemetry.poll(1000, false).empty() && calls == 1U); // discovery only
    CHECK(telemetry.poll(1100, false).empty() && calls == 1U); // rate cap
    CHECK(telemetry.poll(2000, false).empty()); // history is warmup
    frameDump += "3 133333334 132000000\n";
    CHECK(telemetry.poll(3000, false).size() == 1U);
    CHECK(std::string(telemetry.availability()) == "available");
    CHECK(telemetry.poll(4000, false).empty()); // duplicate history is not evidence
    failDump = true;
    CHECK(telemetry.poll(5000, false).empty());
    CHECK(telemetry.layer().empty());
    telemetry.setForeground("");
    CHECK(telemetry.poll(6000, false).empty());
    failDump = false;
    telemetry.setForeground("com.example.game");
    CHECK(telemetry.poll(7000, false).empty()); // new epoch discovery
    frameDump = "ERROR\n16666667\n1 100000000 99000000\n";
    CHECK(telemetry.poll(8000, false).empty());
    CHECK(std::string(telemetry.availability()) == "invalid_refresh_period");

    // devfreq load parsing
    CHECK(parseDevfreqLoadPercent("12@1000000") == 12.0);
    CHECK(parseDevfreqLoadPercent("0@0") == 0.0);
    CHECK(parseDevfreqLoadPercent(" 8.5@1000 ") == 8.5);
    CHECK(parseDevfreqLoadPercent(" 8.5@abc") < 0.0);  // invalid frequency rejects the reading
    CHECK(parseDevfreqLoadPercent("150") < 0.0);
    CHECK(parseDevfreqLoadPercent("abc") < 0.0);
    CHECK(parseDevfreqLoadPercent("") < 0.0);

    // cgroup.procs first-PID parsing
    std::uint32_t pid = 0;
    CHECK(parseFirstPid("1234\n", pid) && pid == 1234U);
    CHECK(parseFirstPid(" 77\n", pid) && pid == 77U);
    CHECK(!parseFirstPid("abc\n", pid));
    CHECK(!parseFirstPid("", pid));

    // SurfaceFlinger latency parsing
    const auto good = parseLatencyDurations("16666667 16666667\n100 200 300\n500 700 900\n");
    CHECK(good.size() == 1U);
    if (good.size() == 1U) CHECK(good[0] == 500U);
    CHECK(parseLatencyDurations("0 900 100\n").empty());  // reversed
    CHECK(parseLatencyDurations("ERROR\nERROR\n").empty());

    // GpuBusySource against fixture directories
    const std::filesystem::path dir =
        std::filesystem::temp_directory_path() / "coreflow_interactivity_test";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir / "gpu0");
    {
        std::ofstream(dir / "gpu0" / "load") << "25@600000000";
    }
    GpuBusySource source;
    source.addRoot((dir / "gpu0").string());
    double ratio = -1.0;
    CHECK(source.poll(ratio));
    CHECK(std::abs(ratio - 0.25) < 1e-9);
    CHECK(source.activeRoot() == (dir / "gpu0").string());

    GpuBusySource missing;
    missing.addRoot((dir / "nope").string());
    CHECK(!missing.poll(ratio));

    // ForegroundTracker against fixture files
    std::filesystem::create_directories(dir / "proc" / "4321");
    {
        std::ofstream(dir / "top-app.procs") << "4321\n";
        std::ofstream(dir / "proc" / "4321" / "cmdline")
            << "com.android.chrome" << '\0' << "--flag" << '\0';
    }
    ForegroundTracker tracker;
    tracker.setProcsPath((dir / "top-app.procs").string());
    tracker.setProcRoot((dir / "proc").string());
    std::string package;
    CHECK(tracker.poll(package));
    CHECK(package == "com.android.chrome");
    CHECK(!tracker.poll(package)); // same PID: no repeated event

    std::ofstream(dir / "top-app.procs") << "4322\n";
    CHECK(!tracker.poll(package)); // process not yet readable
    std::filesystem::create_directories(dir / "proc" / "4322");
    std::ofstream(dir / "proc" / "4322" / "cmdline") << '\0';
    CHECK(!tracker.poll(package)); // empty identity is not a launch
    std::ofstream(dir / "proc" / "4322" / "cmdline") << std::string(300, 'a');
    CHECK(!tracker.poll(package)); // truncated identity remains retryable
    std::ofstream(dir / "proc" / "4322" / "cmdline") << "com.example.game" << '\0';
    CHECK(tracker.poll(package)); // same PID resolves on retry
    CHECK(package == "com.example.game");
    CHECK(!tracker.poll(package));

    std::ofstream(dir / "top-app.procs") << "9999\n4322\n";
    CHECK(!tracker.poll(package)); // same member, reordered cgroup
    CHECK(tracker.available());
    std::ofstream(dir / "proc" / "4322" / "cmdline") << "com.example.reused" << '\0';
    CHECK(tracker.poll(package)); // same PID, new identity
    CHECK(package == "com.example.reused");
    std::ofstream(dir / "top-app.procs").close();
    CHECK(!tracker.poll(package) && !tracker.available());
    std::ofstream(dir / "top-app.procs") << "4322\n";
    CHECK(tracker.poll(package)); // membership disappeared and returned

    std::filesystem::remove_all(dir);
    if (failures != 0) {
        std::cerr << failures << " interactivity source test(s) failed\n";
        return 1;
    }
    std::cout << "Interactivity source tests passed\n";
    return 0;
}
