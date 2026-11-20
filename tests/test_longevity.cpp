#include "coreflow/hardware_longevity.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/mutation_coordinator.hpp"
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
    coreflow::CentralMutationCoordinator coordinator;
    coordinator.reset();
    const auto battery = root / "class/power_supply/battery";
    const auto node = battery / "constant_charge_current_max";
    put(battery / "type", "Battery"); put(node, "900000"); put(battery / "temp", "420"); put(battery / "status", "Charging");
    const auto proc = root / "proc";
    coreflow::HardwareLongevityManager guard(coordinator, root.string(), proc.string());
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
    put(other_node.parent_path() / "type", "Battery"); put(other_node, "2000000"); put(other_node.parent_path() / "temp", "420"); put(other_node.parent_path() / "status", "Charging");
    coreflow::HardwareLongevityManager stepped(coordinator, other.string()); stepped.initialize(config);
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
    put(other_node.parent_path() / "current_now", "2500000");
    put(other_node.parent_path() / "voltage_now", "4100000");
    const auto batt_telemetry = stepped.sample_battery_telemetry();
    assert(batt_telemetry.available);
    assert(batt_telemetry.charging);
    assert(batt_telemetry.fast_charging);
    assert(batt_telemetry.thermal_step_active);
    assert(batt_telemetry.current_ma >= 2499.0 && batt_telemetry.current_ma <= 2501.0);
    assert(batt_telemetry.voltage_mv >= 4099.0 && batt_telemetry.voltage_mv <= 4101.0);
    assert(batt_telemetry.charge_limit_ua == 1500000U);
    assert(batt_telemetry.oem_baseline_ua == 2000000U);
    put(other_node, "700000"); // A newer OEM thermal limit wins.
    assert(!stepped.monitor_battery_thermals(44));
    assert(get(other_node) == "700000");
    assert(coordinator.active_leases().empty());
    const auto foreground_group = root / "fs/cgroup/interactive-foreground";
    const auto top_app_weight = foreground_group / "io.weight";
    put(proc / "self/mountinfo", "55 1 0:55 / " + (root / "fs/cgroup").string() +
        " rw,nosuid,nodev - cgroup2 cgroup rw\n");
    put(proc / "441/cgroup", "0::/interactive-foreground\n");
    put(proc / "441/status", "Name:\ttest-app\nUid:\t10000\t10000\t10000\t10000\n");
    put(proc / "441/oom_score_adj", "0\n");
    put(foreground_group / "cpu.uclamp.min", "0.00\n");
    put(foreground_group / "cpu.uclamp.max", "max\n");
    put(foreground_group / "cgroup.threads", "441\n");
    put(foreground_group / "cgroup.procs", "441\n");
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
    assert(get(top_app_weight) == "default 100");
    assert(coordinator.active_leases().empty());

    assert(guard.apply_storage_pacing());
    put(top_app_weight, "default 200"); // OEM takes ownership.
    assert(!guard.apply_storage_pacing());
    assert(get(top_app_weight) == "default 200");
    assert(coordinator.active_leases().empty());
    assert(!baseline.has_active_mutations());

    // Real Linux/Android kernels expose /sys/class/power_supply/battery as a symlink
    // to /sys/devices/.../power_supply/battery. Verify in-tree symlinks work, escaping
    // symlinks are rejected, and low idle microampere currents (e.g. 15000 uA = 15 mA)
    // are never misreported as multi-amp fast charging.
    {
        const auto real_sys = root / "android_sys";
        const auto dev_batt = real_sys / "devices/platform/pmic/power_supply/battery";
        put(dev_batt / "type", "Battery");
        put(dev_batt / "temp", "345");
        put(dev_batt / "status", "Charging");
        put(dev_batt / "current_now", "15000");   // 15 mA in standard uA
        put(dev_batt / "voltage_now", "4150000"); // 4150 mV in standard uV
        put(dev_batt / "constant_charge_current_max", "3000000");
        fs::create_directories(real_sys / "class/power_supply");
        fs::create_directory_symlink(dev_batt, real_sys / "class/power_supply/battery");

        // Escaping symlink outside sys_root must be ignored.
        const auto outside = root / "outside_evil_batt";
        put(outside / "type", "Battery");
        put(outside / "temp", "400");
        put(outside / "status", "Charging");
        fs::create_directory_symlink(outside, real_sys / "class/power_supply/evil");

        const auto ro_probe = coreflow::HardwareLongevityManager::probe_battery_telemetry(real_sys.string());
        assert(ro_probe.available);
        assert(ro_probe.charging);
        assert(!ro_probe.fast_charging); // 15 mA is trickle/idle, NOT 15A fast charge!
        assert(ro_probe.current_ma >= 14.5 && ro_probe.current_ma <= 15.5);
        assert(ro_probe.voltage_mv >= 4149.0 && ro_probe.voltage_mv <= 4151.0);
        assert(ro_probe.temperature_celsius >= 34.4 && ro_probe.temperature_celsius <= 34.6);
        assert(ro_probe.charge_limit_ua == 3000000U);

        // Full status must not report fast_charging even if current_now is momentarily high.
        put(dev_batt / "status", "Full");
        put(dev_batt / "current_now", "2600000");
        const auto full_probe = coreflow::HardwareLongevityManager::probe_battery_telemetry(real_sys.string());
        assert(full_probe.available);
        assert(full_probe.charging);
        assert(!full_probe.fast_charging);

        // Dual-cell fast charging layout (main "battery" with charge control + secondary "bms" fuel gauge)
        const auto dev_bms = real_sys / "devices/platform/pmic/power_supply/bms";
        put(dev_bms / "type", "Battery");
        put(dev_bms / "temp", "340");
        put(dev_bms / "status", "Charging");
        fs::create_directory_symlink(dev_bms, real_sys / "class/power_supply/bms");
        const auto dual_probe = coreflow::HardwareLongevityManager::probe_battery_telemetry(real_sys.string());
        assert(dual_probe.available);
        assert(dual_probe.charge_limit_ua == 3000000U);

        // Genuinely ambiguous twin anonymous batteries must still fail closed.
        const auto amb_sys = root / "ambiguous_sys";
        put(amb_sys / "class/power_supply/cell0/type", "Battery");
        put(amb_sys / "class/power_supply/cell0/temp", "350");
        put(amb_sys / "class/power_supply/cell0/status", "Charging");
        put(amb_sys / "class/power_supply/cell1/type", "Battery");
        put(amb_sys / "class/power_supply/cell1/temp", "360");
        put(amb_sys / "class/power_supply/cell1/status", "Charging");
        const auto amb_probe = coreflow::HardwareLongevityManager::probe_battery_telemetry(amb_sys.string());
        assert(!amb_probe.available);
    }

    fs::remove_all(root);
}

