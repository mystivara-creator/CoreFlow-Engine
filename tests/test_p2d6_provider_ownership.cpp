#include "coreflow/transactional_frequency_actuator.hpp"
#include "coreflow/autonomous_tuner.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace coreflow;

namespace fs = std::filesystem;

static void put(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path);
    out << value << '\n';
    assert(out.good());
}

int main() {
    const fs::path root = fs::temp_directory_path() /
        ("coreflow-p2d6-" + std::to_string(::getpid()));
    fs::remove_all(root);
    const fs::path policy = root / "sys/devices/system/cpu/cpufreq/policy0";
    put(policy / "scaling_driver", "cpufreq-hw");
    put(policy / "related_cpus", "0-3");
    put(policy / "cpuinfo_min_freq", "300000");
    put(policy / "cpuinfo_max_freq", "2000000");
    put(root / "sys/class/thermal/thermal_zone0/temp", "45000");
    put(root / "sys/class/thermal/thermal_zone0/trip_point_0_type", "passive");
    put(root / "sys/class/thermal/thermal_zone0/trip_point_0_temp", "80000");

    const uint64_t now = 10'000'000'000ULL;
    GenericLinuxFrequencyCeilingProvider generic((root / "sys").string());
    const auto ceiling = generic.current(policy.string(), now);
    assert(ceiling.has_value());
    assert(ceiling->max_hz == 2'000'000'000ULL);
    assert(ceiling->valid_until_ns > now);
    assert(ceiling->signature_or_kernel_contract_verified);
    assert(ceiling->capability_fingerprint.find("generic-cpufreq-v1:") == 0);

    // A generic hardware maximum must never bypass a Qualcomm LMh contract.
    put(policy / "scaling_driver", "qcom-cpufreq-hw");
    assert(!generic.current(policy.string(), now).has_value());
    QualcommLmhFrequencyCeilingProvider qcom((root / "sys").string());
    FallbackFrequencyCeilingProvider fallback(qcom, generic);
    assert(!fallback.current(policy.string(), now).has_value());

    CentralMutationCoordinator coordinator;
    const std::string owner = CentralMutationCoordinator::make_controller_identity("test");
    assert(coordinator.acquire_lease(MutationDomain::CpuPolicy, owner, "tx-1",
                                     ControllerPriority::WorkloadOptimization));
    assert(coordinator.lease_is_live(MutationDomain::CpuPolicy, owner, "tx-1"));
    assert(coordinator.mark_external_ownership_lost(MutationDomain::CpuPolicy, owner, "tx-1"));
    assert(!coordinator.lease_is_live(MutationDomain::CpuPolicy, owner, "tx-1"));
    assert(!coordinator.reserve_contention_retry(MutationDomain::CpuPolicy, owner, "tx-1", 2));
    assert(!coordinator.acquire_lease(MutationDomain::CpuPolicy, owner, "tx-1",
                                      ControllerPriority::WorkloadOptimization));
    coordinator.release_lease(MutationDomain::CpuPolicy, owner, "tx-1");
    const auto active = coordinator.active_leases();
    assert(active.size() == 1);
    assert(active.front().external_ownership_lost);

    fs::remove_all(root);
    return 0;
}
