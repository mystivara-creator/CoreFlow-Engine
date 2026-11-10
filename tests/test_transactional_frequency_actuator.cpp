#include "coreflow/transactional_frequency_actuator.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <time.h>
#include <unistd.h>

using namespace coreflow;
namespace fs = std::filesystem;

static void put(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream(path) << value << "\n";
}

static uint64_t now_ns() {
    timespec ts{};
    assert(::clock_gettime(CLOCK_MONOTONIC, &ts) == 0);
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL +
           static_cast<uint64_t>(ts.tv_nsec);
}

class TestCeiling final : public IndependentFrequencyCeilingProvider {
public:
    std::optional<IndependentFrequencyCeiling> current(
        std::string_view, uint64_t) const noexcept override {
        return IndependentFrequencyCeiling{2'500'000'000ULL, UINT64_MAX, "test-kernel-contract", true};
    }
};

int main() {
    char pattern[] = "/tmp/coreflow-p2d-actuator-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    assert(dir != nullptr);
    const fs::path root(dir);
    ::setenv("COREFLOW_TEST_STATE_DIR", dir, 1);

    const fs::path thermal_root = root / "thermal";
    const auto create_zone = [&](int index, const char* type, const char* temp,
                                 const char* passive, const char* critical) {
        const auto zone = thermal_root / ("thermal_zone" + std::to_string(index));
        put(zone / "type", type);
        put(zone / "temp", temp);
        put(zone / "trip_point_0_type", "passive");
        put(zone / "trip_point_0_temp", passive);
        put(zone / "trip_point_1_type", "critical");
        put(zone / "trip_point_1_temp", critical);
    };
    create_zone(0, "xo-therm", "50000", "80000", "100000");
    create_zone(1, "quiet_therm", "35000", "50000", "60000");
    ThermalMonitor monitor(thermal_root.string());
    ThermalSafetyAuthority thermal(monitor);
    const uint64_t now = now_ns();
    const auto admission = thermal.evaluate(now);
    assert(admission.granted);

    const fs::path policy = root / "sys/devices/system/cpu/cpufreq/policy0";
    put(policy / "scaling_max_freq", "2000000");
    put(policy / "scaling_min_freq", "500000");

    FrequencyCapabilitySnapshot capability{};
    capability.domain = FrequencyDomain::CpuPolicy;
    capability.hardware_min_hz = 500'000'000ULL;
    capability.hardware_max_hz = 2'800'000'000ULL;
    capability.active_constraint_min_hz = 500'000'000ULL;
    capability.active_constraint_max_hz = 2'500'000'000ULL;
    capability.observed_policy_min_hz = 500'000'000ULL;
    capability.observed_policy_max_hz = 2'000'000'000ULL;
    capability.hardware_limit_valid = true;
    capability.active_constraints_valid = true;
    capability.kernel_frequency_table_valid = true;
    capability.thermal_admission_valid = true;
    capability.thermal_admission_granted = true;
    capability.capability_fingerprint_valid = true;
    capability.capability_fingerprint = "test-kernel-contract";
    capability.operating_points = {
        {500'000'000ULL, true, true, true},
        {1'000'000'000ULL, true, true, true},
        {1'500'000'000ULL, true, true, true},
        {2'000'000'000ULL, true, true, true},
        {2'500'000'000ULL, true, true, true}
    };
    assert(FrequencyTargetPlanner::validate(capability));

    CentralMutationCoordinator coordinator;
    coordinator.reset();
    auto& store = BaselineStore::instance();
    assert(store.recover_pending());
    FrequencyTargetPlanner planner;
    TestCeiling ceiling;
    TransactionalFrequencyActuator actuator(
        coordinator, store, planner, thermal, ceiling, (root / "sys").string());

    OutcomeSample baseline{};
    baseline.timestamp_ns = now;
    baseline.cpu_psi_valid = true;
    baseline.cpu_psi_some_pct = 10.0;
    baseline.ebpf_wakeup_valid = true;
    baseline.ebpf_wakeup_rate_per_sec = 100.0;
    baseline.workload_stable = true;

    assert(actuator.apply(policy.string(), capability, 2'000'000'000ULL, 1.25,
                          baseline, now_ns()));
    assert(actuator.mutation_active());
    assert(coordinator.active_leases().size() == 1);
    assert(actuator.rollback());
    std::string restored;
    std::ifstream(policy / "scaling_max_freq") >> restored;
    assert(restored == "2000000");
    assert(coordinator.active_leases().empty());

    // Outcome regression is wired to automatic rollback, not just evaluator reporting.
    baseline.timestamp_ns = now_ns();
    assert(actuator.apply(policy.string(), capability, 2'000'000'000ULL, 1.25,
                          baseline, now_ns()));
    OutcomeSample regressed = baseline;
    regressed.timestamp_ns += 3'100'000'000ULL;
    regressed.cpu_psi_some_pct = 25.0;
    const auto rollback_result = actuator.observe(regressed);
    assert(rollback_result.verdict == OutcomeVerdict::Regressed);
    assert(!actuator.mutation_active() && !actuator.recovery_required());
    assert(coordinator.active_leases().empty());
    std::ifstream(policy / "scaling_max_freq") >> restored;
    assert(restored == "2000000");

    // OEM hot trip is a hard denial before the critical trip is reached.
    put(thermal_root / "thermal_zone0/trip_point_2_type", "hot");
    put(thermal_root / "thermal_zone0/trip_point_2_temp", "98000");
    put(thermal_root / "thermal_zone0/temp", "99000");
    assert(!actuator.apply(policy.string(), capability, 2'000'000'000ULL, 1.25,
                           baseline, now_ns()));
    assert(coordinator.active_leases().empty());

    // Emergency OEM critical trip must fail closed before a lease or write.
    put(thermal_root / "thermal_zone0/temp", "101000");
    assert(!actuator.apply(policy.string(), capability, 2'000'000'000ULL, 1.25,
                           baseline, now_ns()));
    assert(coordinator.active_leases().empty());

    // Missing required quiet_therm sensor is also a hard denial.
    put(thermal_root / "thermal_zone0/temp", "50000");
    fs::remove_all(thermal_root / "thermal_zone1");
    ThermalMonitor missing_monitor(thermal_root.string());
    ThermalSafetyAuthority missing_thermal(missing_monitor);
    assert(!missing_thermal.evaluate(now_ns()).granted);

    // Outcome evaluator requires a complete 3s window and flags PSI/wakeup regression.
    FrequencyOutcomeEvaluator evaluator;
    assert(evaluator.begin(baseline));
    OutcomeSample sample = baseline;
    sample.timestamp_ns += 3'100'000'000ULL;
    sample.cpu_psi_some_pct = 25.0;
    sample.ebpf_wakeup_rate_per_sec = 50.0;
    auto outcome = evaluator.observe(sample);
    assert(outcome.verdict == OutcomeVerdict::Regressed);
    assert(outcome.reason.find("CPU PSI") != std::string::npos);

    assert(evaluator.begin(baseline));
    sample = baseline;
    sample.timestamp_ns += 3'100'000'000ULL;
    sample.cpu_psi_some_pct = 6.0;
    sample.ebpf_wakeup_rate_per_sec = 110.0;
    outcome = evaluator.observe(sample);
    assert(outcome.verdict == OutcomeVerdict::Improved);

    fs::remove_all(root);
    return 0;
}
