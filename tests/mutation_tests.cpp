#include "coreflow/mutation.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    using namespace coreflow;
    namespace fs = std::filesystem;

    const fs::path root = fs::path("/tmp/coreflow_mutation_test");
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path governor = root / "scaling_governor";
    {
        std::ofstream file(governor);
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

    const auto applied = controller.apply(RuntimeState::ThermalGuard, sample, profile, cfg);
    assert(applied == MutationResult::Verified);

    {
        std::ifstream file(governor);
        std::string value;
        std::getline(file, value);
        assert(value == "schedutil");
    }

    assert(controller.restoreAll());
    {
        std::ifstream file(governor);
        std::string value;
        std::getline(file, value);
        assert(value == "performance");
    }

    fs::remove_all(root);
    return 0;
}
