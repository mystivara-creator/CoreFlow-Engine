#include "coreflow/uclamp.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace coreflow;

static std::string read_token(const std::string& path) {
    std::ifstream file(path);
    std::string value;
    file >> value;
    return value;
}

static void write_token(const std::string& path, const std::string& value) {
    std::ofstream file(path);
    file << value;
}

int main() {
    char state_dir[] = "/tmp/coreflow-uclamp-XXXXXX";
    assert(::mkdtemp(state_dir));
    assert(::setenv("COREFLOW_TEST_STATE_DIR", state_dir, 1) == 0);
    const std::string root(state_dir);
    assert(BaselineStore::instance().recover_pending());
    auto& coordinator = CentralMutationCoordinator::instance();
    coordinator.reset();

    // cgroup v2-style percentage interface.
    const std::string percent_root = root + "/unified";
    const std::string percent_top = percent_root + "/top-app";
    std::filesystem::create_directories(percent_top);
    write_token(percent_top + "/cpu.uclamp.min", "0.00");
    write_token(percent_top + "/cpu.uclamp.max", "max");

    UclampGovernor percentage(percent_root);
    assert(percentage.is_supported());
    assert(percentage.encoding_name() == "percentage");
    assert(percentage.target_path() == percent_top);
    assert(percentage.apply_state(WorkloadState::Balanced, 320, 1024));
    assert(read_token(percent_top + "/cpu.uclamp.min") == "31.25");
    assert(read_token(percent_top + "/cpu.uclamp.max") == "max");
    assert(percentage.get_top_app_min() == 320u);
    assert(percentage.get_top_app_max() == 1024u);
    assert(coordinator.active_leases().size() == 1);
    assert(percentage.apply_state(WorkloadState::ThermalGuard, 320, 1024));
    const auto thermal_lease = coordinator.active_leases();
    assert(thermal_lease.size() == 1);
    assert(thermal_lease.front().priority == ControllerPriority::ThermalStabilization);
    coordinator.emergency_preempt_all("test-uclamp-emergency");
    assert(read_token(percent_top + "/cpu.uclamp.min") == "0.00");
    assert(read_token(percent_top + "/cpu.uclamp.max") == "max");
    assert(coordinator.active_leases().empty());
    assert(percentage.apply_state(WorkloadState::Balanced, 320, 1024));
    assert(coordinator.active_leases().size() == 1);
    // OEM changes must not be overwritten during subsequent policy transitions or stop.
    write_token(percent_top + "/cpu.uclamp.min", "10.00");
    assert(!percentage.owned_values_intact());
    assert(coordinator.active_leases().empty());
    (void)BaselineStore::instance().rollback_all();
    assert(read_token(percent_top + "/cpu.uclamp.min") == "10.00");

    // Android/vendor controller layouts can expose 0..1024 capacity values.
    // Put it one directory deeper to exercise bounded semantic discovery.
    const std::string capacity_root = root + "/legacy";
    const std::string capacity_top = capacity_root + "/cpu-controller/top-app";
    std::filesystem::create_directories(capacity_top);
    write_token(capacity_top + "/cpu.uclamp.min", "0");
    write_token(capacity_top + "/cpu.uclamp.max", "1024");

    UclampGovernor capacity(capacity_root);
    assert(capacity.is_supported());
    assert(capacity.encoding_name() == "capacity1024");
    assert(capacity.target_path() == capacity_top);
    assert(capacity.apply_state(WorkloadState::SustainedLoad, 320, 1024));
    assert(read_token(capacity_top + "/cpu.uclamp.min") == "480");
    assert(read_token(capacity_top + "/cpu.uclamp.max") == "1024");
    assert(capacity.get_top_app_min() == 480u);
    assert(capacity.get_top_app_max() == 1024u);
    assert(capacity.resync());
    assert(coordinator.active_leases().empty());
    assert(!BaselineStore::instance().has_active_mutations());

    // Vendor thermal throttling may leave integer 0..1024 nodes capped <= 100 at startup;
    // ensure plain integers without cgroup.controllers remain capacity1024,
    // while preserving a low OEM ceiling rather than forcing 1024.
    const std::string throttled_root = root + "/throttled-v1";
    const std::string throttled_top = throttled_root + "/top-app";
    std::filesystem::create_directories(throttled_top);
    write_token(throttled_top + "/cpu.uclamp.min", "0");
    write_token(throttled_top + "/cpu.uclamp.max", "100");

    UclampGovernor throttled(throttled_root);
    assert(throttled.is_supported());
    assert(throttled.encoding_name() == "capacity1024");
    assert(throttled.apply_state(WorkloadState::Balanced, 320, 1024));
    assert(read_token(throttled_top + "/cpu.uclamp.min") == "0");
    assert(read_token(throttled_top + "/cpu.uclamp.max") == "100");
    assert(throttled.apply_state(WorkloadState::ThermalGuard, 320, 1024));
    assert(read_token(throttled_top + "/cpu.uclamp.max") == "87");
    write_token(throttled_top + "/cpu.uclamp.max", "70");
    assert(!throttled.owned_values_intact());
    (void)BaselineStore::instance().rollback_all();
    assert(read_token(throttled_top + "/cpu.uclamp.max") == "70");
    assert(!throttled.apply_state(WorkloadState::Balanced, 320, 1024));

    const std::string capped_root = root + "/capped-v2";
    const std::string capped_top = capped_root + "/top-app";
    std::filesystem::create_directories(capped_top);
    write_token(capped_root + "/cgroup.controllers", "cpu");
    write_token(capped_top + "/cpu.uclamp.min", "0.00");
    write_token(capped_top + "/cpu.uclamp.max", "50.00");
    UclampGovernor capped(capped_root);
    assert(capped.is_supported());
    assert(capped.apply_state(WorkloadState::Balanced, 320, 1024));
    assert(read_token(capped_top + "/cpu.uclamp.max") == "50.00");
    assert(capped.apply_state(WorkloadState::ThermalGuard, 320, 1024));
    assert(capped.get_top_app_max() < 512u);
    assert(capped.resync());
    assert(read_token(capped_top + "/cpu.uclamp.max") == "50.00");
    assert(coordinator.active_leases().empty());

    // Unified cgroup v2 hierarchy identified by cgroup.controllers uses percentage.
    const std::string v2_ctrl_root = root + "/unified-controllers";
    const std::string v2_ctrl_top = v2_ctrl_root + "/top-app";
    std::filesystem::create_directories(v2_ctrl_top);
    write_token(v2_ctrl_root + "/cgroup.controllers", "cpu memory io");
    write_token(v2_ctrl_top + "/cpu.uclamp.min", "0");
    write_token(v2_ctrl_top + "/cpu.uclamp.max", "100");

    UclampGovernor v2_ctrl(v2_ctrl_root);
    assert(v2_ctrl.is_supported());
    assert(v2_ctrl.encoding_name() == "percentage");

    std::filesystem::remove_all(root);
    std::cout << "Semantic UClamp discovery, encoding and restoration PASS\n";
    return 0;
}
