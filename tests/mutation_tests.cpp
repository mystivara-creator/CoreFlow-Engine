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

    // Simulate another component having changed the governor.
    {
        std::ofstream file(governor);
        file << "schedutil\n";
    }

    const auto restored =
        controller.apply(RuntimeState::ThermalGuard, sample, profile, cfg);

    if (!expect(restored == MutationResult::Verified,
                "thermal guard should restore the captured baseline"))
        return 1;

    if (!expect(readValue(governor) == "walt",
                "thermal guard must not force schedutil"))
        return 1;

    if (!expect(controller.restoreAll(),
                "baseline restore should succeed"))
        return 1;

    fs::remove_all(root);
    return 0;
}
