#include "coreflow/frequency_controller.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include <cassert>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
namespace fs = std::filesystem;
static void put(const fs::path& p, const std::string& value) { fs::create_directories(p.parent_path()); std::ofstream(p) << value; }
static std::string get(const fs::path& p) { std::string value; std::ifstream(p) >> value; return value; }
int main() {
    char pattern[] = "/tmp/coreflow-frequency-XXXXXX";
    const char* dir = ::mkdtemp(pattern); assert(dir);
    const fs::path root(dir);
    ::setenv("COREFLOW_TEST_STATE_DIR", dir, 1);
    auto& baseline = coreflow::BaselineStore::instance(); assert(baseline.recover_pending());
    coreflow::CentralMutationCoordinator coordinator;
    coordinator.reset();
    const auto cpu = root / "devices/system/cpu/cpufreq/policy0";
    put(cpu / "scaling_max_freq", "2000000"); put(cpu / "scaling_min_freq", "500000");
    put(cpu / "scaling_available_frequencies", "500000 1000000 1500000 2000000");
    const auto gpu = root / "class/devfreq/gpu";
    put(gpu / "max_freq", "800000000"); put(gpu / "min_freq", "200000000");
    put(gpu / "available_frequencies", "200000000 400000000 600000000 800000000");
    // A second controller instance owns different mock nodes but competes for
    // the same global CPU/GPU mutation domains. Its callback identity must not
    // replace the first instance's preemption handler.
    const fs::path second_root = root / "second";
    const auto second_cpu = second_root / "devices/system/cpu/cpufreq/policy0";
    put(second_cpu / "scaling_max_freq", "1800000");
    put(second_cpu / "scaling_min_freq", "400000");
    put(second_cpu / "scaling_available_frequencies", "400000 900000 1300000 1800000");
    const auto second_gpu = second_root / "class/devfreq/gpu";
    put(second_gpu / "max_freq", "700000000");
    put(second_gpu / "min_freq", "100000000");
    put(second_gpu / "available_frequencies", "100000000 300000000 500000000 700000000");

    coreflow::FrequencyController controller(coordinator, root.string());
    controller.discover(true, true);
    coreflow::FrequencyController second_controller(coordinator, second_root.string());
    second_controller.discover(true, true);
    assert(controller.cpu_count() == 1 && controller.gpu_count() == 1);
    assert(second_controller.cpu_count() == 1 && second_controller.gpu_count() == 1);
    assert(controller.apply(0.75));
    assert(!second_controller.apply(0.75));
    assert(get(cpu / "scaling_max_freq") == "1500000");
    assert(get(gpu / "max_freq") == "600000000");
    auto leases = coordinator.active_leases();
    assert(leases.size() == 2);
    assert(std::any_of(leases.begin(), leases.end(), [](const auto& lease) {
        return lease.domain == coreflow::MutationDomain::CpuPolicy &&
               lease.owner_controller.rfind("FrequencyController@", 0) == 0;
    }));
    assert(std::any_of(leases.begin(), leases.end(), [](const auto& lease) {
        return lease.domain == coreflow::MutationDomain::GpuDevfreq &&
               lease.owner_controller.rfind("FrequencyController@", 0) == 0;
    }));

    // Emergency preemption must synchronously restore and read back every frequency
    // node before the coordinator releases either domain lease.
    coordinator.emergency_preempt_all("test-thermal-emergency");
    assert(get(cpu / "scaling_max_freq") == "2000000");
    assert(get(gpu / "max_freq") == "800000000");
    assert(get(second_cpu / "scaling_max_freq") == "1800000");
    assert(get(second_gpu / "max_freq") == "700000000");
    assert(coordinator.active_leases().empty());

    // A CPU frequency write rejected by filesystem permissions must not strand
    // a CPU lease, while an independently successful GPU-domain mutation remains
    // leased and is restored through the normal emergency callback.
    assert(::chmod((cpu / "scaling_max_freq").c_str(), 0444) == 0);
    assert(!controller.apply(0.75));
    assert(get(cpu / "scaling_max_freq") == "2000000");
    assert(get(gpu / "max_freq") == "600000000");
    const auto failed_write_leases = coordinator.active_leases();
    assert(failed_write_leases.size() == 1);
    assert(failed_write_leases.front().domain == coreflow::MutationDomain::GpuDevfreq);
    coordinator.emergency_preempt_all("test-frequency-partial-domain-failure");
    assert(get(gpu / "max_freq") == "800000000");
    assert(coordinator.active_leases().empty());
    assert(::chmod((cpu / "scaling_max_freq").c_str(), 0644) == 0);

    // A CPUFreq path that became journaled by another/legacy owner after
    // discovery must be rejected without overwriting its value. The independent
    // GPU domain may still apply its own mutation even though the aggregate result
    // is false because the CPU part of the requested policy was rejected.
    assert(baseline.apply_mutation((cpu / "scaling_max_freq").string(), "1700000"));
    assert(!controller.apply(0.75));
    assert(get(cpu / "scaling_max_freq") == "1700000");
    assert(get(gpu / "max_freq") == "600000000");
    const auto foreign_cpu_leases = coordinator.active_leases();
    assert(foreign_cpu_leases.size() == 1);
    assert(foreign_cpu_leases.front().domain == coreflow::MutationDomain::GpuDevfreq);
    coordinator.emergency_preempt_all("test-frequency-foreign-baseline");
    assert(get(gpu / "max_freq") == "800000000");
    assert(get(cpu / "scaling_max_freq") == "1700000");
    assert(coordinator.active_leases().empty());
    assert(baseline.rollback((cpu / "scaling_max_freq").string()));
    assert(get(cpu / "scaling_max_freq") == "2000000");
    controller.discover(true, true);
    assert(controller.cpu_count() == 1 && controller.gpu_count() == 1);

    // A subsequent authorized policy cycle may acquire fresh transaction identities.
    assert(controller.apply(0.75));
    assert(get(cpu / "scaling_max_freq") == "1500000");
    assert(get(gpu / "max_freq") == "600000000");
    assert(controller.apply(1.0));
    assert(get(cpu / "scaling_max_freq") == "2000000");
    assert(get(gpu / "max_freq") == "800000000");
    assert(coordinator.active_leases().empty());

    // Emergency thermal floor is accepted, but physical per-domain minimums remain authoritative.
    assert(!controller.apply(0.19));
    assert(controller.apply(0.2));
    assert(get(cpu / "scaling_max_freq") == "500000");
    assert(get(gpu / "max_freq") == "200000000");
    assert(controller.apply(1.0));
    assert(get(cpu / "scaling_max_freq") == "2000000");
    assert(get(gpu / "max_freq") == "800000000");
    assert(coordinator.active_leases().empty());

    assert(controller.apply(0.75));
    assert(get(cpu / "scaling_max_freq") == "1500000");
    assert(get(gpu / "max_freq") == "600000000");
    put(cpu / "scaling_max_freq", "1000000"); // OEM changed its cap.
    assert(controller.apply(1.0));
    assert(get(cpu / "scaling_max_freq") == "1000000");
    assert(controller.cpu_count() == 0);
    assert(get(gpu / "max_freq") == "800000000");
    assert(coordinator.active_leases().empty());
    baseline.rollback_all(); assert(get(cpu / "scaling_max_freq") == "1000000");
    assert(!controller.apply(1.1));
    fs::remove_all(root);
}
