#include "coreflow/frequency_target_planner.hpp"
#include "coreflow/tiered_policy.hpp"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <unistd.h>

using namespace coreflow;
namespace fs = std::filesystem;

namespace {

void write_file(const fs::path& path, const char* value) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << value;
    assert(output.good());
}

PolicyEvidence cpu_bound_evidence(
    double cpu_psi,
    float confidence,
    uint32_t consecutive_windows,
    uint64_t window_end_ns) {
    PolicyEvidence evidence{};
    evidence.telemetry.window_start_ns = window_end_ns - 1'000'000'000ULL;
    evidence.telemetry.window_end_ns = window_end_ns;
    evidence.telemetry.cpu_pressure_some_pct = cpu_psi;
    evidence.telemetry.valid_features = FeatureCpuPressure | FeatureWakeupRate;
    evidence.telemetry.source = TelemetryBackend::Ebpf;
    evidence.telemetry.event_count = 1200U;
    evidence.telemetry.event_drain_saturated = false;

    evidence.classification.workload = DetectedWorkloadClass::CpuBound;
    evidence.classification.confidence = confidence;
    evidence.classification.evidence_mask = FeatureCpuPressure;
    evidence.classification.consecutive_windows = consecutive_windows;
    evidence.classification.classified_at_ns = window_end_ns;
    evidence.classification.stable = true;

    evidence.evaluated_at_ns = window_end_ns + 100'000'000ULL;
    evidence.max_telemetry_age_ns = 1'000'000'000ULL;
    evidence.telemetry_fresh = true;
    return evidence;
}

PlatformContextSnapshot hyperos_context() {
    PlatformContextSnapshot platform{};
    platform.identity.vendor = PlatformVendor::Xiaomi;
    platform.identity.os_family = OsFamily::HyperOS;
    platform.identity.confidence = IdentityConfidence::High;
    platform.execution_tier = ExecutionTier::XiaomiHyperOS;
    platform.policy_family = PolicyFamily::HyperOS4Inspired;
    platform.tier1_eligible = true;
    return platform;
}

void test_policy_router() {
    const CoreFlowBase base{};
    const HyperOS4Inspired hyperos{};
    const TieredPolicyRouter router(base, hyperos);

    auto evidence = cpu_bound_evidence(18.27, 0.74F, 2U, 2'000'000'000ULL);

    PlatformContextSnapshot universal{};
    const auto universal_selection = router.select(universal);
    assert(universal_selection.policy == &base);
    assert(!universal_selection.platform_policy_eligible);
    const auto base_decision = router.evaluate(universal, evidence);
    assert(base_decision.tier == OptimizationTier::Tier0Equilibrium);
    assert(base_decision.action == PolicyAction::ObserveOnly);

    const auto platform = hyperos_context();
    const auto hyperos_selection = router.select(platform);
    assert(hyperos_selection.policy == &hyperos);
    assert(hyperos_selection.platform_policy_eligible);

    const auto tier1 = router.evaluate(platform, evidence);
    assert(tier1.tier == OptimizationTier::Tier1InteractiveResponse);
    assert(tier1.action == PolicyAction::CpuInteractiveResponse);
    assert(!tier1.requires_mutation_admission);
    assert(std::abs(tier1.confidence - 0.74F) < 0.0001F);

    evidence.thermal_headroom_valid = true;
    evidence.thermal_headroom_c = 8.0;
    evidence.cpu_frequency_capability_valid = true;
    evidence.mutation_runtime_ready = true;
    const auto tier1_step = router.evaluate(platform, evidence);
    assert(tier1_step.tier == OptimizationTier::Tier1InteractiveResponse);
    assert(tier1_step.action == PolicyAction::CpuFrequencyStep);
    assert(tier1_step.requires_mutation_admission);
    assert(std::abs(tier1_step.requested_capacity_scale - 1.03) < 0.0001);

    auto sustained = cpu_bound_evidence(35.0, 0.80F, 5U, 4'000'000'000ULL);
    sustained.thermal_headroom_valid = true;
    sustained.thermal_headroom_c = 12.0;
    sustained.cpu_frequency_capability_valid = true;
    sustained.mutation_runtime_ready = true;
    const auto tier2 = router.evaluate(platform, sustained);
    assert(tier2.tier == OptimizationTier::Tier2SustainedOptimization);
    assert(tier2.action == PolicyAction::CpuFrequencyStep);
    assert(tier2.requires_mutation_admission);
    assert(std::abs(tier2.requested_capacity_scale - 1.08) < 0.0001);

    // Sustained CPU pressure without independent thermal admission cannot
    // authorize an OPP step; it remains a Tier-1 response candidate.
    sustained.thermal_headroom_valid = false;
    const auto withheld = router.evaluate(platform, sustained);
    assert(withheld.tier == OptimizationTier::Tier1InteractiveResponse);
    assert(withheld.action == PolicyAction::CpuInteractiveResponse);
    assert(!withheld.requires_mutation_admission);

    // Universal policy has higher thresholds and a longer evidence dwell.
    auto base_sustained = cpu_bound_evidence(42.0, 0.85F, 5U, 5'000'000'000ULL);
    base_sustained.thermal_headroom_valid = true;
    base_sustained.thermal_headroom_c = 15.0;
    base_sustained.cpu_frequency_capability_valid = true;
    base_sustained.mutation_runtime_ready = true;
    const auto base_tier2 = router.evaluate(universal, base_sustained);
    assert(base_tier2.tier == OptimizationTier::Tier2SustainedOptimization);
    assert(std::abs(base_tier2.requested_capacity_scale - 1.05) < 0.0001);

    // Safety recovery preempts performance evidence, even if telemetry is stale.
    PolicyEvidence thermal{};
    thermal.thermal_recovery_required = true;
    thermal.mutation_runtime_ready = true;
    const auto tier3 = router.evaluate(platform, thermal);
    assert(tier3.tier == OptimizationTier::Tier3ThermalRecovery);
    assert(tier3.action == PolicyAction::ThermalRecovery);
    assert(tier3.requires_mutation_admission);

    auto invalid = cpu_bound_evidence(90.0, 0.99F, 8U, 6'000'000'000ULL);
    invalid.event_stream_saturated = true;
    const auto saturated = router.evaluate(platform, invalid);
    assert(saturated.tier == OptimizationTier::Tier0Equilibrium);
    assert(saturated.action == PolicyAction::ObserveOnly);

    invalid.event_stream_saturated = false;
    invalid.telemetry_fresh = false;
    const auto stale = router.evaluate(platform, invalid);
    assert(stale.tier == OptimizationTier::Tier0Equilibrium);

    invalid.telemetry_fresh = true;
    invalid.telemetry.cpu_pressure_some_pct =
        std::numeric_limits<double>::quiet_NaN();
    const auto not_finite = router.evaluate(platform, invalid);
    assert(not_finite.tier == OptimizationTier::Tier0Equilibrium);
}

FrequencyDiscoveryRequest valid_discovery_request() {
    FrequencyDiscoveryRequest request{};
    request.policy_index = 0U;
    request.active_constraint_min_hz = 300'000'000ULL;
    request.active_constraint_max_hz = 1'804'800'000ULL;
    request.active_constraints_valid = true;
    request.thermal_admission_valid = true;
    request.thermal_admission_granted = true;
    request.capability_fingerprint_valid = true;
    return request;
}

void test_opp_discovery_and_planning() {
    char root_template[] = "/tmp/coreflow-p2d-opp-XXXXXX";
    assert(::mkdtemp(root_template) != nullptr);
    const fs::path sys_root = fs::path(root_template) / "sys";
    const fs::path policy = sys_root / "devices/system/cpu/cpufreq/policy0";

    write_file(policy / "cpuinfo_min_freq", "300000\n");
    write_file(policy / "cpuinfo_max_freq", "1804800\n");
    write_file(policy / "scaling_min_freq", "300000\n");
    write_file(policy / "scaling_max_freq", "1401600\n");
    write_file(policy / "scaling_available_frequencies",
               "300000 576000 806400 1113600 1401600 1708800 1804800\n");

    const FrequencyOppDiscovery discovery(sys_root.string());
    const auto discovered = discovery.discover_cpu_policy(valid_discovery_request());
    assert(discovered.has_value());
    assert(discovered->hardware_max_hz == 1'804'800'000ULL);
    assert(discovered->observed_policy_max_hz == 1'401'600'000ULL);
    assert(discovered->operating_points.size() == 7U);
    assert(FrequencyTargetPlanner::validate(*discovered));

    const FrequencyTargetPlanner planner{};
    const auto upward = planner.select_target(
        *discovered, 1'401'600'000ULL, 1.10);
    assert(upward.has_value());
    assert(*upward == 1'708'800'000ULL);
    assert(*upward > discovered->observed_policy_max_hz);
    assert(*upward <= discovered->hardware_max_hz);

    const auto at_ceiling = planner.select_target(
        *discovered, 1'804'800'000ULL, 1.25);
    assert(at_ceiling.has_value());
    assert(*at_ceiling == 1'804'800'000ULL);

    assert(!planner.select_target(*discovered, 1'000'000'000ULL, 0.10).has_value());
    assert(!planner.select_target(*discovered, 1'000'000'000ULL, 1.50).has_value());
    assert(!planner.select_target(
        *discovered, 1'000'000'000ULL,
        std::numeric_limits<double>::quiet_NaN()).has_value());

    auto no_thermal = *discovered;
    no_thermal.thermal_admission_valid = false;
    assert(!FrequencyTargetPlanner::validate(no_thermal));
    assert(!planner.select_target(no_thermal, 1'401'600'000ULL, 1.10).has_value());

    auto invalid_constraints = *discovered;
    invalid_constraints.active_constraint_max_hz =
        invalid_constraints.hardware_max_hz + 1U;
    assert(!FrequencyTargetPlanner::validate(invalid_constraints));

    // If the active safety ceiling is below an OPP, that OPP is not selectable.
    auto constrained_request = valid_discovery_request();
    constrained_request.active_constraint_max_hz = 1'600'000'000ULL;
    const auto constrained =
        discovery.discover_cpu_policy(constrained_request);
    assert(constrained.has_value());
    const auto constrained_target = planner.select_target(
        *constrained, 1'401'600'000ULL, 1.10);
    assert(constrained_target.has_value());
    assert(*constrained_target == 1'401'600'000ULL);
    assert(*constrained_target <= constrained->active_constraint_max_hz);

    // OPP fallback reads the first column of stats/time_in_state, not an
    // interpolated range, when scaling_available_frequencies is absent.
    const fs::path policy1 = sys_root / "devices/system/cpu/cpufreq/policy1";
    write_file(policy1 / "cpuinfo_min_freq", "300000\n");
    write_file(policy1 / "cpuinfo_max_freq", "1804800\n");
    write_file(policy1 / "scaling_min_freq", "300000\n");
    write_file(policy1 / "scaling_max_freq", "1401600\n");
    write_file(policy1 / "stats/time_in_state",
               "300000 12\n576000 20\n1401600 30\n1708800 2\n");
    auto fallback_request = valid_discovery_request();
    fallback_request.policy_index = 1U;
    const auto fallback = discovery.discover_cpu_policy(fallback_request);
    assert(fallback.has_value());
    assert(fallback->operating_points.size() == 4U);
    assert(FrequencyTargetPlanner::validate(*fallback));

    // Without discrete kernel OPP evidence, cpuinfo min/max alone is not
    // converted into invented intermediate operating points.
    const fs::path policy2 = sys_root / "devices/system/cpu/cpufreq/policy2";
    write_file(policy2 / "cpuinfo_min_freq", "300000\n");
    write_file(policy2 / "cpuinfo_max_freq", "1804800\n");
    write_file(policy2 / "scaling_min_freq", "300000\n");
    write_file(policy2 / "scaling_max_freq", "1401600\n");
    auto missing_table = valid_discovery_request();
    missing_table.policy_index = 2U;
    assert(!discovery.discover_cpu_policy(missing_table).has_value());

    // Reject a symlinked OPP table (O_NOFOLLOW) instead of following it.
    const fs::path policy3 = sys_root / "devices/system/cpu/cpufreq/policy3";
    write_file(policy3 / "cpuinfo_min_freq", "300000\n");
    write_file(policy3 / "cpuinfo_max_freq", "1804800\n");
    write_file(policy3 / "scaling_min_freq", "300000\n");
    write_file(policy3 / "scaling_max_freq", "1401600\n");
    const fs::path external_table = fs::path(root_template) / "external_opp_table";
    write_file(external_table, "300000 576000 1401600 1708800\n");
    fs::create_symlink(external_table, policy3 / "scaling_available_frequencies");
    auto symlink_request = valid_discovery_request();
    symlink_request.policy_index = 3U;
    assert(!discovery.discover_cpu_policy(symlink_request).has_value());

    fs::remove_all(root_template);
}

} // namespace

int main() {
    test_policy_router();
    test_opp_discovery_and_planning();
    std::cout << "P2-D.1 policy engine and P2-D.2 validated OPP planner PASS\n";
    return 0;
}
