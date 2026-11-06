#include "coreflow/hardware_longevity.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/autonomous_tuner.hpp"
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
    auto& coordinator = coreflow::CentralMutationCoordinator::instance();
    coordinator.reset();
    const auto battery = root / "class/power_supply/battery";
    const auto node = battery / "constant_charge_current_max";
    put(node, "900000"); put(battery / "temp", "420"); put(battery / "status", "Charging");
    coreflow::HardwareLongevityManager guard(root.string());
    coreflow::LongevityConfig config; guard.initialize(config);
    assert(!guard.monitor_battery_thermals(44)); assert(get(node) == "900000");

    // A journaled battery limit created after discovery is not this manager's
    // baseline and must not be treated as an OEM limit by the thermal stepper.
    assert(baseline.apply_mutation(node.string(), "850000"));
    assert(!guard.monitor_battery_thermals(44));
    assert(get(node) == "850000");
    assert(coordinator.active_leases().empty());
    assert(baseline.rollback(node.string()));
    assert(get(node) == "900000");
    // Start a new fixture with a higher valid OEM ceiling; never increase a low one.
    const auto other = root / "other";
    const auto other_node = other / "class/power_supply/battery/constant_charge_current_max";
    put(other_node, "2000000"); put(other_node.parent_path() / "temp", "420"); put(other_node.parent_path() / "status", "Charging");
    coreflow::HardwareLongevityManager stepped(other.string()); stepped.initialize(config);
    put(other_node.parent_path() / "temp", "350");
    assert(!stepped.monitor_battery_thermals(95)); // CPU junction heat is not battery temperature.
    assert(get(other_node) == "2000000");
    put(other_node.parent_path() / "temp", "420");
    assert(stepped.monitor_battery_thermals(44)); assert(get(other_node) == "1500000");
    auto battery_lease = coordinator.active_leases();
    assert(battery_lease.size() == 1);
    assert(battery_lease.front().domain == coreflow::MutationDomain::PowerLongevity);
    coordinator.emergency_preempt_all("test-battery-thermal-preemption");
    assert(get(other_node) == "2000000");
    assert(coordinator.active_leases().empty());

    assert(stepped.monitor_battery_thermals(44));
    put(other_node, "700000"); // A newer OEM thermal limit wins.
    assert(!stepped.monitor_battery_thermals(44));
    assert(get(other_node) == "700000");
    assert(coordinator.active_leases().empty());
    const auto top_app_weight = root / "fs/cgroup/top-app/io.weight";
    put(top_app_weight, "default 100");
    assert(baseline.capture(top_app_weight.string()));
    assert(baseline.apply_mutation(top_app_weight.string(), "default 300"));
    assert(guard.apply_storage_pacing());
    assert(get(top_app_weight) == "default 300");
    assert(coordinator.active_leases().empty());
    assert(baseline.rollback(top_app_weight.string()));
    assert(get(top_app_weight) == "default 100");

    assert(guard.apply_storage_pacing());
    assert(get(top_app_weight) == "default 800");
    const auto io_lease = coordinator.active_leases();
    assert(io_lease.size() == 1);
    assert(io_lease.front().domain == coreflow::MutationDomain::BlockIo);
    coordinator.emergency_preempt_all("test-storage-preemption");
    assert(get(root / "fs/cgroup/top-app/io.weight") == "default 100");
    assert(coordinator.active_leases().empty());

    assert(guard.apply_storage_pacing());
    put(root / "fs/cgroup/top-app/io.weight", "default 200"); // OEM takes ownership.
    assert(!guard.apply_storage_pacing());
    assert(get(root / "fs/cgroup/top-app/io.weight") == "default 200");
    assert(coordinator.active_leases().empty());
    assert(!baseline.has_active_mutations());
    fs::remove_all(root);
}
