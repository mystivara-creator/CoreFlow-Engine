#include "coreflow/thermal_monitor.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace coreflow;

static void create_zone(const std::string& root, int index, const char* type, const char* temp) {
    const std::string dir = root + "/thermal_zone" + std::to_string(index);
    std::filesystem::create_directories(dir);
    {
        std::ofstream t(dir + "/type");
        t << type << "\n";
    }
    {
        std::ofstream v(dir + "/temp");
        v << temp << "\n";
    }
}

int main() {
    char tmp[] = "/tmp/coreflow-thermal-XXXXXX";
    assert(::mkdtemp(tmp) != nullptr);
    const std::string root(tmp);
    assert(!assessment_from_android_severity(-1).available);
    assert(!assessment_from_android_severity(7).available);
    assert(assessment_from_android_severity(0).pressure == 0);
    assert(assessment_from_android_severity(2).pressure > 70);
    assert(!assessment_from_android_severity(4).emergency);
    assert(assessment_from_android_severity(5).emergency);
    assert(assessment_from_android_severity(6).emergency);

    // Pixel Tensor BIG/MID/LITTLE clusters + non-compute zones that must be ignored.
    create_zone(root, 0, "socd", "95000");
    create_zone(root, 1, "battery", "49000");
    create_zone(root, 2, "LITTLE", "36250");
    create_zone(root, 3, "MID", "39500");
    create_zone(root, 4, "BIG", "42750");

    ThermalMonitor tensor_monitor(root);
    assert(std::fabs(tensor_monitor.hottest_celsius() - 42.75f) < 0.01f);

    // Sub-1C millidegree sensor scaling (500 mC == 0.5 C, not 500 C).
    const std::string cold_root = root + "/cold";
    create_zone(cold_root, 0, "cpu-0-0", "500");
    ThermalMonitor cold_monitor(cold_root);
    assert(std::fabs(cold_monitor.hottest_celsius() - 0.5f) < 0.01f);

    // No per-model tuning: hotter CPU junction and cooler battery use their own trips.
    const auto oem = root + "/oem";
    create_zone(oem, 150, "cpuss-0", "78000");
    create_zone(oem, 151, "battery", "40000");
    create_zone(oem, 152, "skin", "41000");
    create_zone(oem, 153, "socd", "70000");
    auto trip = [](const std::string& r, int zone, int index, const char* type, const char* value) {
        const auto base = r + "/thermal_zone" + std::to_string(zone) + "/trip_point_" + std::to_string(index);
        std::ofstream(base + "_type") << type;
        std::ofstream(base + "_temp") << value;
    };
    trip(oem, 150, 0, "passive", "95000"); trip(oem, 150, 1, "critical", "110000");
    trip(oem, 151, 0, "passive", "45000"); trip(oem, 151, 1, "critical", "60000");
    trip(oem, 152, 0, "passive", "43000");
    ThermalMonitor adaptive(oem);
    assert(adaptive.hottest_celsius() == 78);
    assert(adaptive.battery_celsius() == 40);
    assert(adaptive.skin_celsius() == 41);
    const auto decision = adaptive.assess();
    assert(decision.available && !decision.emergency);
    assert(decision.sensor == "skin"); // 41/43C is nearer its own limit than CPU 78/95C.
    assert(!tensor_monitor.assess().available); // A name/temperature alone is no thermal policy.
    create_zone(oem, 150, "cpuss-0", "111000");
    assert(adaptive.assess().emergency);
    create_zone(oem, 150, "cpuss-0", "78000");
    create_zone(oem, 151, "battery", "unavailable");
    assert(!adaptive.assess().available); // Do not silently omit a broken, known-policy sensor.
    const auto invalid = root + "/invalid";
    create_zone(invalid, 0, "cpu", "-40000");
    create_zone(invalid, 1, "skin", "-273000");
    create_zone(invalid, 2, "battery", "garbage");
    ThermalMonitor bad(invalid);
    assert(bad.hottest_celsius() < 0 && !bad.assess().available);
    const auto units = root + "/units";
    create_zone(units, 0, "cpu", "100");
    assert(std::fabs(ThermalMonitor(units).hottest_celsius() - 0.1f) < 0.001f);
    std::filesystem::remove_all(root);
    std::cout << "ThermalMonitor cluster discovery and millidegree scaling PASS\n";
    return 0;
}
