#include "coreflow/uclamp.hpp"
#include "coreflow/baseline_store.hpp"
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
    assert(percentage.apply_state(WorkloadState::Interactive, DeviceTier::MidRange, 320, 1024));
    assert(read_token(percent_top + "/cpu.uclamp.min") == "31.25");
    assert(read_token(percent_top + "/cpu.uclamp.max") == "max");
    assert(percentage.get_top_app_min() == 320u);
    assert(percentage.get_top_app_max() == 1024u);
    assert(BaselineStore::instance().rollback_all() >= 1);
    assert(read_token(percent_top + "/cpu.uclamp.min") == "0.00");

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
    assert(capacity.apply_state(WorkloadState::SustainedGame, DeviceTier::Flagship));
    assert(read_token(capacity_top + "/cpu.uclamp.min") == "448");
    assert(read_token(capacity_top + "/cpu.uclamp.max") == "1024");
    assert(capacity.get_top_app_min() == 448u);
    assert(capacity.get_top_app_max() == 1024u);
    assert(BaselineStore::instance().rollback_all() >= 1);
    assert(!BaselineStore::instance().has_active_mutations());

    std::filesystem::remove_all(root);
    std::cout << "Semantic UClamp discovery, encoding and restoration PASS\n";
    return 0;
}
