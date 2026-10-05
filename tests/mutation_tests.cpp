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

int main() {
    namespace fs = std::filesystem;

    const fs::path root = fs::path("/tmp/coreflow_mutation_test");
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path governor = root / "scaling_governor";
    {
        std::ofstream file(governor);
        if (!expect(file.is_open(), "unable to create governor test node")) {
            return 1;
        }
        file << "performance\n";
    }

    CpuPolicy policy;
    policy.path = root.string();
    policy.readable = true;
    policy.governor = "performance";
    policy.governor_writable = true;
    policy.available_governors = {"performance", "schedutil"};

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

    const auto applied =
        controller.apply(RuntimeState::ThermalGuard, sample, profile, cfg);

    if (!expect(applied == MutationResult::Verified,
                "mutation should be verified")) {
        fs::remove_all(root);
        return 1;
    }

    {
        std::ifstream file(governor);
        std::string value;
        std::getline(file, value);
        if (!expect(value == "schedutil",
                    "governor should be changed to schedutil")) {
            fs::remove_all(root);
            return 1;
        }
    }

    if (!expect(controller.restoreAll(), "baseline restore should succeed")) {
        fs::remove_all(root);
        return 1;
    }

    {
        std::ifstream file(governor);
        std::string value;
        std::getline(file, value);
        if (!expect(value == "performance",
                    "governor should be restored to performance")) {
            fs::remove_all(root);
            return 1;
        }
    }

    fs::remove_all(root);
    return 0;
}
