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

    std::filesystem::remove_all(root);
    std::cout << "ThermalMonitor cluster discovery and millidegree scaling PASS\n";
    return 0;
}
