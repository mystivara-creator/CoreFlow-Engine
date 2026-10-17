#include "coreflow/foreground_tracker.hpp"
#include "coreflow/frame_telemetry.hpp"
#include "coreflow/gpu_busy_source.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; std::cerr << "FAIL: " #expr " line " << __LINE__ << '\n'; } } while (false)
}

int main() {
    using namespace coreflow::observe;

    // devfreq load parsing
    CHECK(parseDevfreqLoadPercent("12@1000000") == 12.0);
    CHECK(parseDevfreqLoadPercent("0@0") == 0.0);
    CHECK(parseDevfreqLoadPercent(" 8.5@abc") == 8.5);
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
    CHECK(good.size() == 2U);
    if (good.size() == 2U) {
        CHECK(good[0] == 100U);
        CHECK(good[1] == 200U);
    }
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

    std::filesystem::remove_all(dir);
    if (failures != 0) {
        std::cerr << failures << " interactivity source test(s) failed\n";
        return 1;
    }
    std::cout << "Interactivity source tests passed\n";
    return 0;
}
