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
    policy.available_governors = {"walt", "performance", "schedutil"};

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

    if (!expect(controller.candidateCount() == 3,
                "all available governors should be discovered"))
        return 1;

    if (!expect(controller.validatedCandidateCount() == 0,
                "discovered governors must not be trusted automatically"))
        return 1;

    // Adaptive mode without validated evidence must remain observation-only.
    const auto noEvidence =
        controller.apply(RuntimeState::Normal, sample, profile, cfg);
    if (!expect(noEvidence == MutationResult::Skipped,
                "unvalidated candidate must not be applied"))
        return 1;

    if (!expect(readValue(governor) == "walt",
                "baseline governor must remain unchanged without evidence"))
        return 1;

    if (!expect(controller.registerValidatedGovernor(root.string(), "schedutil"),
                "available governor should be registrable after validation"))
        return 1;

    if (!expect(controller.validatedCandidateCount() == 1,
                "validated candidate count should increase"))
        return 1;

    // The controller can now perform a real, verified write in a controlled
    // test environment. This is the same write/read-back contract used by
    // the Android sysfs path.
    const auto applied =
        controller.apply(RuntimeState::Normal, sample, profile, cfg);
    if (!expect(applied == MutationResult::Verified,
                "validated governor mutation should verify"))
        return 1;

    if (!expect(readValue(governor) == "schedutil",
                "verified mutation should change the governor"))
        return 1;

    // Protected state must immediately restore the captured baseline rather
    // than trying another governor experiment.
    const auto restored =
        controller.apply(RuntimeState::ThermalGuard, sample, profile, cfg);

    if (!expect(restored == MutationResult::Verified,
                "thermal guard should restore the captured baseline"))
        return 1;

    if (!expect(readValue(governor) == "walt",
                "thermal guard must restore the original governor"))
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

    fs::remove_all(root);
    return 0;
}
