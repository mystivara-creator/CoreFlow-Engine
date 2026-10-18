#include "coreflow/render_collector.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", \
                     __FILE__, __LINE__, #condition); \
        return __LINE__; \
    } \
} while (false)

namespace fs = std::filesystem;
using coreflow::observe::countCgroupProcs;
using coreflow::observe::kMaxCgroupPids;
using coreflow::observe::readLaunchObservation;

namespace {

void writeText(const fs::path& path, const std::string& body) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path);
    out << body;
}

int testCountsOnlyNumericLines() {
    CHECK(countCgroupProcs("123\n456\n\n789") == 3U);
    CHECK(countCgroupProcs("12a\n-4\n7\n") == 1U);
    CHECK(countCgroupProcs("") == 0U);
    return 0;
}

int testCountIsBounded() {
    std::string many;
    for (std::size_t i = 0; i < kMaxCgroupPids + 100U; ++i) many += "1\n";
    CHECK(countCgroupProcs(many) == kMaxCgroupPids);
    return 0;
}

int testReadsMembershipAndPressure() {
    const fs::path root = fs::temp_directory_path() / "coreflow_render_collector";
    fs::remove_all(root);
    writeText(root / "procs", "1001\n1002\n1003\n");
    writeText(root / "io_pressure", "some avg10=7.25 avg60=1.00 avg300=0.50 total=1234\n"
                                    "full avg10=0.00 avg60=0.00 avg300=0.00 total=0\n");
    coreflow::observe::LaunchObservation obs;
    CHECK(readLaunchObservation((root / "procs").string(), (root / "io_pressure").string(), 42U, obs));
    CHECK(obs.timestamp_monotonic_ns == 42U);
    CHECK(obs.foreground_pids == 3U);
    CHECK(obs.io_pressure_some > 7.2 && obs.io_pressure_some < 7.3);
    fs::remove_all(root);
    return 0;
}

int testMissingPressureIsUnavailableNotFatal() {
    const fs::path root = fs::temp_directory_path() / "coreflow_render_collector_nopsi";
    fs::remove_all(root);
    writeText(root / "procs", "5\n");
    coreflow::observe::LaunchObservation obs;
    CHECK(readLaunchObservation((root / "procs").string(), (root / "absent").string(), 7U, obs));
    CHECK(obs.io_pressure_some < 0.0);
    fs::remove_all(root);
    return 0;
}

int testOversizedMembershipFailsClosed() {
    const fs::path root = fs::temp_directory_path() / "coreflow_render_collector_oversized";
    fs::remove_all(root);
    writeText(root / "procs", std::string((1U << 20U) + 1U, '1'));
    coreflow::observe::LaunchObservation obs;
    CHECK(!readLaunchObservation((root / "procs").string(), (root / "absent").string(), 8U, obs));
    fs::remove_all(root);
    return 0;
}

int testMissingMembershipFailsClosed() {
    coreflow::observe::LaunchObservation obs;
    CHECK(!readLaunchObservation("/nonexistent/coreflow/procs", "/nonexistent/io", 1U, obs));
    return 0;
}

} // namespace

int main() {
    int failures = 0;
    failures += testCountsOnlyNumericLines() != 0;
    failures += testCountIsBounded() != 0;
    failures += testReadsMembershipAndPressure() != 0;
    failures += testMissingPressureIsUnavailableNotFatal() != 0;
    failures += testOversizedMembershipFailsClosed() != 0;
    failures += testMissingMembershipFailsClosed() != 0;
    if (failures != 0) {
        std::fprintf(stderr, "CoreFlow render collector tests: FAIL (%d)\n", failures);
        return 1;
    }
    std::puts("CoreFlow render collector tests: PASS");
    return 0;
}
