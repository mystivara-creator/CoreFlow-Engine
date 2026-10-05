#include "coreflow/mutation.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace coreflow;

static bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

static std::string readValue(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::string value;
    std::getline(file, value);
    return value;
}

int main() {
    namespace fs = std::filesystem;

    const fs::path root = fs::path("/tmp/coreflow_mutation_test");
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path governor = root / "scaling_governor";
    {
        std::ofstream file(governor);
        if (!expect(file.is_open(), "unable to create governor test node"))
            return 1;
        file << "conservative\n";
    }

    CpuPolicy policy;
    policy.path = root.string();
    policy.readable = true;
    policy.governor = "conservative";
    policy.governor_writable = true;
    policy.available_governors = {
        "walt", "performance", "schedutil", "conservative", "powersave"
    };

    DeviceProfile profile;
    profile.cpu_policies.push_back(policy);

    EngineConfig cfg;
    cfg.setMutationMode(MutationMode::Adaptive);
    cfg.setMinConfidence(0.70);
    cfg.setAllowCpuGovernor(true);

    RuntimeSample active;
    active.confidence = 1.0;
    active.thermal_available = true;
    active.thermal_millidegrees = 36000;
    active.cpu_utilization_available = true;
    active.cpu_utilization = 0.90;
    active.load1 = 4.0;

    MutationController controller;
    controller.captureBaseline(profile);

    if (!expect(controller.baselineSize() == 1,
                "baseline should contain one governor node"))
        return 1;

    if (!expect(controller.candidateCount() == 5,
                "all available governors should be discovered"))
        return 1;

    if (!expect(controller.validatedCandidateCount() == 0,
                "discovered governors must not be trusted automatically"))
        return 1;

    // Adaptive selection must work without Trial/evidence registration.
    const auto elevated =
        controller.apply(RuntimeState::Elevated, active, profile, cfg);
    if (!expect(elevated == MutationResult::Verified,
                "adaptive governor selection should verify without trial evidence"))
        return 1;

    const std::string elevatedGovernor = readValue(governor);
    if (!expect(elevatedGovernor == "performance" || elevatedGovernor == "walt",
                "elevated workload should select a responsive governor"))
        return 1;

    // Repeating the same state/telemetry should not rewrite the same choice.
    const auto hysteresis =
        controller.apply(RuntimeState::Elevated, active, profile, cfg);
    if (!expect(hysteresis == MutationResult::Skipped,
                "hysteresis should suppress a redundant governor switch"))
        return 1;

    // Thermal protection must move away from performance-biased operation.
    active.cpu_utilization = 0.20;
    active.thermal_millidegrees = 44000;
    const auto thermal =
        controller.apply(RuntimeState::ThermalGuard, active, profile, cfg);
    if (!expect(thermal == MutationResult::Verified,
                "thermal guard should apply a safer adaptive governor"))
        return 1;

    const std::string thermalGovernor = readValue(governor);
    if (!expect(thermalGovernor == "powersave" ||
                thermalGovernor == "conservative" ||
                thermalGovernor == "schedutil",
                "thermal guard should avoid performance-biased governors"))
        return 1;

    // Pressure is not a CPU-governor tuning target; restore the baseline.
    const auto pressure =
        controller.apply(RuntimeState::Pressure, active, profile, cfg);
    if (!expect(pressure == MutationResult::Verified ||
                pressure == MutationResult::Skipped,
                "pressure should restore or already hold the captured baseline"))
        return 1;
    if (!expect(readValue(governor) == "conservative",
                "pressure state must restore the original governor"))
        return 1;

    // Explicit validation remains optional prior evidence.
    if (!expect(controller.registerValidatedGovernor(root.string(), "schedutil"),
                "available governor should be registrable after validation"))
        return 1;
    if (!expect(controller.validatedCandidateCount() == 1,
                "validated candidate count should increase"))
        return 1;

    // Low confidence must fail closed and restore baseline.
    active.confidence = 0.40;
    const auto blockedByConfidence =
        controller.apply(RuntimeState::Elevated, active, profile, cfg);
    if (!expect(blockedByConfidence == MutationResult::Verified ||
                blockedByConfidence == MutationResult::Skipped,
                "low confidence must not perform an unverified mutation"))
        return 1;
    if (!expect(readValue(governor) == "conservative",
                "low confidence must leave the baseline governor active"))
        return 1;

    // Permission gate must prevent mutation even when a candidate is known.
    cfg.setAllowCpuGovernor(false);
    active.confidence = 1.0;
    const auto blocked =
        controller.apply(RuntimeState::Elevated, active, profile, cfg);
    if (!expect(blocked == MutationResult::Skipped,
                "CPU governor permission must block mutation"))
        return 1;
    if (!expect(readValue(governor) == "conservative",
                "blocked mutation must not change the governor"))
        return 1;

    // Controlled Governor Trial Engine remains an explicit experimental path.
    cfg.setAllowCpuGovernor(true);
    cfg.setMutationMode(MutationMode::Trial);

    controller.captureBaseline(profile);
    RuntimeSample trialSample;
    trialSample.confidence = 1.0;
    trialSample.thermal_available = true;
    trialSample.thermal_millidegrees = 35000;
    trialSample.load1 = 0.5;
    trialSample.cpu_utilization = 0.20;

    if (!expect(controller.validatedCandidateCount() == 0,
                "captureBaseline should reset validated candidate evidence"))
        return 1;

    if (!expect(controller.apply(RuntimeState::Normal, trialSample, profile, cfg) ==
                    MutationResult::TrialObserving,
                "trial should begin with baseline observation"))
        return 1;
    if (!expect(controller.apply(RuntimeState::Normal, trialSample, profile, cfg) ==
                    MutationResult::TrialObserving,
                "trial baseline should require multiple samples"))
        return 1;
    if (!expect(controller.apply(RuntimeState::Normal, trialSample, profile, cfg) ==
                    MutationResult::TrialApplied,
                "trial should apply candidate after baseline window"))
        return 1;
    if (!expect(readValue(governor) != "conservative",
                "trial candidate should be written only after baseline window"))
        return 1;
    if (!expect(controller.apply(RuntimeState::Normal, trialSample, profile, cfg) ==
                    MutationResult::TrialObserving,
                "trial candidate window should collect evidence"))
        return 1;
    if (!expect(controller.apply(RuntimeState::Normal, trialSample, profile, cfg) ==
                    MutationResult::TrialObserving,
                "trial candidate window should collect multiple samples"))
        return 1;
    if (!expect(controller.apply(RuntimeState::Normal, trialSample, profile, cfg) ==
                    MutationResult::TrialCompleted,
                "safe trial should complete with evidence"))
        return 1;
    if (!expect(readValue(governor) == "conservative",
                "completed trial must restore the baseline governor"))
        return 1;
    if (!expect(controller.validatedCandidateCount() == 1,
                "safe trial should validate exactly one candidate"))
        return 1;

    // A protected state must abort an active trial and restore the baseline.
    controller.captureBaseline(profile);
    (void)controller.apply(RuntimeState::Normal, trialSample, profile, cfg);
    (void)controller.apply(RuntimeState::Normal, trialSample, profile, cfg);
    (void)controller.apply(RuntimeState::Normal, trialSample, profile, cfg);
    if (!expect(controller.trialActive(), "trial should be active after candidate write"))
        return 1;
    const auto aborted =
        controller.apply(RuntimeState::ThermalGuard, trialSample, profile, cfg);
    if (!expect(aborted == MutationResult::TrialAborted,
                "thermal guard must abort an active trial"))
        return 1;
    if (!expect(readValue(governor) == "conservative",
                "aborted trial must restore the baseline governor"))
        return 1;

    fs::remove_all(root);
    return 0;
}
