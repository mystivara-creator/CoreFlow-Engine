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
static std::string read(const std::string& path) {
    std::ifstream f(path);
    std::string value;
    f >> value;
    return value;
}
int main() {
    char dir[] = "/tmp/coreflow-cgroup-XXXXXX";
    assert(::mkdtemp(dir));
    assert(::setenv("COREFLOW_TEST_STATE_DIR", dir, 1) == 0);
    const std::string root(dir);
    for (const auto* group : {"top-app", "foreground", "background"}) {
        std::filesystem::create_directory(root + "/" + group);
        { std::ofstream f(root + "/" + group + "/cpu.uclamp.min"); f << "0.00"; }
        { std::ofstream f(root + "/" + group + "/cpu.uclamp.max"); f << "max"; }
    }
    assert(BaselineStore::instance().recover_pending());
    UclampGovernor governor(root);
    assert(governor.is_supported());
    assert(governor.apply_state(WorkloadState::Interactive, DeviceTier::MidRange, 320, 1024));
    assert(read(root + "/top-app/cpu.uclamp.min") == "31.25");
    assert(read(root + "/top-app/cpu.uclamp.max") == "max");
    assert(governor.get_top_app_min() == 320u);
    assert(governor.get_top_app_max() == 1024u);
    assert(BaselineStore::instance().rollback_all() >= 1);
    assert(!BaselineStore::instance().has_active_mutations());
    for (const auto* group : {"top-app", "foreground", "background"}) {
        assert(read(root + "/" + group + "/cpu.uclamp.min") == "0.00");
        assert(read(root + "/" + group + "/cpu.uclamp.max") == "max");
    }
    std::filesystem::remove_all(root);
    std::cout << "Kernel cgroup v2 percentage and restoration PASS\n";
}
