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
        file << "walt\n";
    }

    CpuPolicy policy;
    policy.path = root.string();
    policy.readable = true;
    policy.governor = "walt";
    policy.governor_writable = true;
    policy.available_governors = {"walt", "performance", "schedutil", "conservative", "powersave"};

    DeviceProfile profile;
    profile.cpu_policies.push_back(policy);

    RuntimeSample sample;
    sample.confidence = 1.0;

    EngineConfig cfg;
    cfg.setMutationMode(MutationMode::Adaptive);
    cfg.setMinConfidence(0.70);
    cfg.setAllowCpuGovernor(true);

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

    // Adaptive mode must work from discovered capabilities and telemetry;
    // it must not depend on Trial/evidence registration. With a low workload
    // and comfortable temperature, the bounded scorer should prefer a
    // conservative governor over the walt baseline.
    sample.thermal_available = true;
    sample.thermal_millidegrees = 36000;
    sample.cpu_utilization_available = true;
    sample.cpu_utilization = 0.10;
    sample.load1 = 0.50;

    const auto applied =
        controller.apply(RuntimeState::Normal, sample, profile, cfg);
    if (!expect(applied == MutationResult::Verified,
                "adaptive scorer should select and verify a valid candidate"))
        return 1;

    if (!expect(readValue(governor) != "walt",
                "adaptive selection should not require trial evidence"))
        return 1;

    if (!expect(controller.validatedCandidateCount() == 0,
                "adaptive mode must not create trial evidence implicitly"))
        return 1;

    // ThermalGuard is a safety override, not a Trial phase: the adaptive
    // scorer may move to a more protective advertised governor.
    const auto guarded =
        controller.apply(RuntimeState::ThermalGuard, sample, profile, cfg);

    if (!expect(guarded == MutationResult::Verified ||
                    guarded == MutationResult::Skipped,
                "thermal guard should remain within bounded adaptive selection"))
        return 1;

    if (!expect(readValue(governor) != "performance",
                "thermal guard must never select performance"))
        return 1;

    if (!expect(controller.restoreAll(),
                "explicit baseline restore should succeed after thermal guard"))
        return 1;

    if (!expect(readValue(governor) == "walt",
                "baseline restore must recover the original governor"))
        return 1;

    // Unknown governors are ignored by the bounded automatic selector.
    policy.available_governors.push_back("vendor_magic");
    controller.captureBaseline(profile);
    sample.thermal_millidegrees = 36000;
    sample.cpu_utilization = 0.10;
    const auto unknownSafe =
        controller.apply(RuntimeState::Normal, sample, profile, cfg);
    if (!expect(unknownSafe == MutationResult::Verified ||
                    unknownSafe == MutationResult::Skipped,
                "unknown governor must not break adaptive selection"))
        return 1;

    if (!expect(controller.restoreAll(),
                "baseline restore should succeed"))
        return 1;

    // Permission gate must prevent mutation even when a candidate is known.
    cfg.setAllowCpuGovernor(false);
    const auto blocked =
        controller.apply(RuntimeState::Normal, sample, profile, cfg);
    if (!expect(blocked == MutationResult::Skipped,
                "CPU governor permission must block mutation"))
        return 1;

    if (!expect(readValue(governor) == "walt",
                "blocked mutation must not change the governor"))
        return 1;


    // Controlled Governor Trial Engine: baseline observation -> candidate
    // write -> candidate observation -> verified rollback/evidence.
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
                "trial reset should clear validated candidate evidence"))
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
    if (!expect(readValue(governor) != "walt",
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
    if (!expect(readValue(governor) == "walt",
                "completed trial must restore the baseline governor"))
        return 1;
    if (!expect(controller.validatedCandidateCount() == 1,
                "safe trial should validate exactly one candidate"))
        return 1;

    // A protected state must abort an active trial and restore the baseline.
    cfg.setMutationMode(MutationMode::Trial);
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
    if (!expect(readValue(governor) == "walt",
                "aborted trial must restore the baseline governor"))
        return 1;

    fs::remove_all(root);
    return 0;
}
