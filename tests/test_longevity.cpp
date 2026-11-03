#include "coreflow/hardware_longevity.hpp"
#include "coreflow/baseline_store.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>
namespace fs = std::filesystem;
static void put(const fs::path& p, const std::string& v) { fs::create_directories(p.parent_path()); std::ofstream(p) << v; }
static std::string get(const fs::path& p) { std::ifstream f(p); std::string v; std::getline(f,v); return v; }
int main() {
    char name[] = "/tmp/coreflow-longevity-XXXXXX";
    const char* dir = ::mkdtemp(name); assert(dir);
    fs::path root(dir); ::setenv("COREFLOW_TEST_STATE_DIR", dir, 1);
    auto& baseline = coreflow::BaselineStore::instance(); assert(baseline.recover_pending());
    const auto battery = root / "class/power_supply/battery";
    const auto node = battery / "constant_charge_current_max";
    put(node, "900000"); put(battery / "temp", "420"); put(battery / "status", "Charging");
    coreflow::HardwareLongevityManager guard(root.string());
    coreflow::LongevityConfig config; guard.initialize(config);
    assert(!guard.monitor_battery_thermals(44)); assert(get(node) == "900000");
    // Start a new fixture with a higher valid OEM ceiling; never increase a low one.
    const auto other = root / "other";
    const auto other_node = other / "class/power_supply/battery/constant_charge_current_max";
    put(other_node, "2000000"); put(other_node.parent_path() / "temp", "420"); put(other_node.parent_path() / "status", "Charging");
    coreflow::HardwareLongevityManager stepped(other.string()); stepped.initialize(config);
    assert(stepped.monitor_battery_thermals(44)); assert(get(other_node) == "1500000");
    put(other_node, "700000"); // A newer OEM thermal limit wins.
    assert(!stepped.monitor_battery_thermals(44)); baseline.rollback_all(); assert(get(other_node) == "700000");
    put(root / "fs/cgroup/top-app/io.weight", "default 100");
    assert(guard.apply_storage_pacing()); assert(get(root / "fs/cgroup/top-app/io.weight") == "default 800");
    baseline.rollback_all(); assert(get(root / "fs/cgroup/top-app/io.weight") == "default 100");
    fs::remove_all(root);
}
