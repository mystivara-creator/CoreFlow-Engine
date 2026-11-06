#include "coreflow/cgroup_v2.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

static void put(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary | std::ios::trunc) << value;
}

static std::string get(const fs::path& path) {
    std::ifstream input(path);
    std::string value;
    input >> value;
    return value;
}

int main() {
    char pattern[] = "/tmp/coreflow-cgroup-XXXXXX";
    const char* directory = ::mkdtemp(pattern);
    assert(directory != nullptr);
    const fs::path root(directory);
    const fs::path top_app = root / "top-app";
    const fs::path cpuset = top_app / "cpuset.cpus";
    const fs::path weight = top_app / "cpu.weight";
    put(root / "cgroup.controllers", "cpu cpuset\n");
    put(cpuset, "0-3\n");
    put(weight, "100\n");
    ::setenv("COREFLOW_TEST_STATE_DIR", (root / "state").c_str(), 1);

    auto& baseline = coreflow::BaselineStore::instance();
    assert(baseline.recover_pending());
    auto& coordinator = coreflow::CentralMutationCoordinator::instance();
    coordinator.reset();

    {
        coreflow::CgroupV2Controller controller(root.string());
        assert(controller.is_supported());
        assert(controller.set_cpu_weight(coreflow::CgroupTarget::TopApp, 500));
        assert(controller.set_cpuset(coreflow::CgroupTarget::TopApp, "0-1"));
        assert(get(weight) == "500");
        assert(get(cpuset) == "0-1");

        const auto leases = coordinator.active_leases();
        assert(leases.size() == 1);
        assert(leases.front().domain == coreflow::MutationDomain::CpuPolicy);
        coordinator.emergency_preempt_all("test-cgroup-cpu-preemption");
        assert(get(weight) == "100");
        assert(get(cpuset) == "0-3");
        assert(coordinator.active_leases().empty());

        assert(controller.set_cpu_weight(coreflow::CgroupTarget::TopApp, 400));
        put(weight, "600"); // OEM changes the active node while the lease is held.
        assert(!controller.set_cpuset(coreflow::CgroupTarget::TopApp, "0-2"));
        assert(get(weight) == "600");
        assert(get(cpuset) == "0-3");
        assert(coordinator.active_leases().empty());
        assert(!baseline.has_active_mutations());
        assert(!controller.migrate_task(::getpid(), coreflow::CgroupTarget::Background));
        assert(!controller.unlock_vendor_restrictions());
    }

    fs::remove_all(root);
    return 0;
}
