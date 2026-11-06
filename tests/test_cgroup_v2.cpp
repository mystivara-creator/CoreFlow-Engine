#include "coreflow/cgroup_v2.hpp"
#include "coreflow/frequency_controller.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
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

        // FrequencyController and CgroupV2Controller share the conservative
        // CpuPolicy ownership domain. The second controller must not write while
        // the first owns it; emergency preemption must roll frequency back before
        // cgroup policy can acquire the domain.
        const auto cpu_policy = root / "devices/system/cpu/cpufreq/policy0";
        put(cpu_policy / "scaling_max_freq", "2000000");
        put(cpu_policy / "scaling_min_freq", "500000");
        put(cpu_policy / "scaling_available_frequencies", "500000 1000000 1500000 2000000");
        const auto gpu_policy = root / "class/devfreq/gpu";
        put(gpu_policy / "max_freq", "800000000");
        put(gpu_policy / "min_freq", "200000000");
        put(gpu_policy / "available_frequencies", "200000000 400000000 600000000 800000000");
        coreflow::FrequencyController frequencies(root.string());
        frequencies.discover(true, true);
        assert(frequencies.apply(0.75));
        assert(get(cpu_policy / "scaling_max_freq") == "1500000");
        assert(!controller.set_cpu_weight(coreflow::CgroupTarget::TopApp, 500));
        assert(get(weight) == "100");
        assert(coordinator.active_leases().size() == 2);
        coordinator.emergency_preempt_all("test-frequency-to-cgroup-domain-handoff");
        assert(get(cpu_policy / "scaling_max_freq") == "2000000");
        assert(get(gpu_policy / "max_freq") == "800000000");
        assert(coordinator.active_leases().empty());

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

        // If a later node in the same controller transaction is unwritable,
        // the controller must restore its already-mutated node and release the
        // lease. It must not strand a CPU-policy lease with no active mutation.
        assert(controller.set_cpu_weight(coreflow::CgroupTarget::TopApp, 500));
        assert(get(weight) == "500");
        assert(::chmod(cpuset.c_str(), 0444) == 0);
        assert(!controller.set_cpuset(coreflow::CgroupTarget::TopApp, "0-1"));
        assert(get(weight) == "100");
        assert(get(cpuset) == "0-3");
        assert(coordinator.active_leases().empty());
        assert(!baseline.has_active_mutations());
        assert(::chmod(cpuset.c_str(), 0644) == 0);

        assert(controller.set_cpu_weight(coreflow::CgroupTarget::TopApp, 400));
        put(weight, "600"); // OEM changes the active node while the lease is held.
        assert(!controller.set_cpuset(coreflow::CgroupTarget::TopApp, "0-2"));
        assert(get(weight) == "600");
        assert(get(cpuset) == "0-3");
        assert(coordinator.active_leases().empty());
        assert(!baseline.has_active_mutations());
        assert(!controller.migrate_task(::getpid(), coreflow::CgroupTarget::Background));
        assert(!controller.unlock_vendor_restrictions());

        // A controller that already owns TopApp must still reject a different
        // path whose BaselineStore entry is modified by an unrelated legacy owner.
        assert(controller.set_cpu_weight(coreflow::CgroupTarget::TopApp, 300));
        assert(get(weight) == "300");
        const fs::path foreign_weight = root / "foreground/cpu.weight";
        put(foreign_weight, "100");
        assert(baseline.capture(foreign_weight.string()));
        assert(baseline.apply_mutation(foreign_weight.string(), "900"));
        assert(!controller.set_cpu_weight(coreflow::CgroupTarget::Foreground, 500));
        assert(get(foreign_weight) == "900");
        assert(get(weight) == "300");
        const auto retained_lease = coordinator.active_leases();
        assert(retained_lease.size() == 1);
        assert(retained_lease.front().owner_controller.rfind("CgroupV2Controller@", 0) == 0);
        assert(baseline.rollback(foreign_weight.string()));
        assert(get(foreign_weight) == "100");
        coordinator.emergency_preempt_all("test-cgroup-foreign-node-isolation");
        assert(get(weight) == "600");
        assert(coordinator.active_leases().empty());

        // Destruction must synchronously restore any still-owned transaction.
        assert(controller.set_cpu_weight(coreflow::CgroupTarget::TopApp, 300));
        assert(coordinator.active_leases().size() == 1);
    }
    assert(get(weight) == "600");
    assert(coordinator.active_leases().empty());
    assert(!baseline.has_active_mutations());

    fs::remove_all(root);
    return 0;
}
