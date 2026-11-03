#include "coreflow/config.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
int main() {
    const auto path = std::filesystem::temp_directory_path() / "coreflow-v2-test-config.ini";
    {
        std::ofstream f(path);
        f << "[thermal_guardian]\n"
             "thermal_trip_celsius=garbage\n"
             "thermal_recover_celsius=nan\n"
             "temp_monitor_interval_ms=-1\n"
             "[uclamp_tuning]\n"
             "top_app_interactive_min=99999999\n"
             "top_app_interactive_max=1\n";
    }
    auto c = coreflow::DaemonConfig::load_from_file(path.string());
    assert(c.thermal_trip_celsius == 44.0f);
    assert(c.thermal_recover_celsius == 40.0f);
    assert(c.temp_monitor_interval_ms == 1000u);
    assert(c.top_app_interactive_min == 1024u);
    assert(c.top_app_interactive_max == 1024u);
    std::filesystem::remove(path);
    std::cout << "Configuration bounds and malformed values PASS\n";
}
