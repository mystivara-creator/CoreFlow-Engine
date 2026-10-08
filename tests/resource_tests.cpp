#include "coreflow/resource_actuator.hpp"
#include "coreflow/resource_mutation.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

using namespace coreflow;
namespace fs = std::filesystem;

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { ++failures; std::fprintf(stderr, "CHECK FAILED: %s\n", #x); } } while (0)

std::string readFile(const fs::path& p) {
    std::ifstream f(p);
    std::string s;
    std::getline(f, s);
    return s;
}

void writeFile(const fs::path& p, const std::string& s) {
    std::ofstream f(p, std::ios::trunc);
    f << s << '\n';
}

class Journal final : public MutationJournal {
public:
    Entries entries;
    bool pending{false};
    bool commit(const Entries& e) noexcept override { entries = e; pending = true; return true; }
    bool clear() noexcept override { entries.clear(); pending = false; return true; }
    LoadState load(Entries& out) noexcept override { out = entries; return pending ? LoadState::Pending : LoadState::Absent; }
};

void test_swappiness_adaptive_path() {
    const fs::path root = fs::temp_directory_path() / "coreflow_resource_mutation_test";
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    profile.vm_swappiness.path = swappiness.string();
    profile.vm_swappiness.readable = true;
    profile.vm_swappiness.writable = true;

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 100;
    sample.mem_available_ratio = 0.10;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.2;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);
    CHECK(controller.captureBaseline(profile));
    CHECK(controller.apply(RuntimeState::Elevated, sample, profile, config) == MutationResult::Verified);
    CHECK(readFile(swappiness) == "70");
    CHECK(controller.mutated());
    CHECK(controller.restoreAll());
    CHECK(readFile(swappiness) == "60");
    CHECK(!controller.mutated());
    fs::remove_all(root);
}

void test_resource_restore_only_owned_paths() {
    const fs::path root = fs::temp_directory_path() / "coreflow_resource_owned_test";
    fs::remove_all(root);
    fs::create_directories(root / "queue");
    const fs::path swappiness = root / "swappiness";
    const fs::path readAhead = root / "queue" / "read_ahead_kb";
    writeFile(swappiness, "60");
    writeFile(readAhead, "128");

    DeviceProfile profile;
    profile.vm_swappiness.path = swappiness.string();
    profile.vm_swappiness.readable = true;
    profile.vm_swappiness.writable = true;
    IoDevice device;
    device.path = root.string();
    device.name = "testblk";
    device.read_ahead_readable = true;
    device.read_ahead_writable = true;
    profile.io_devices.push_back(device);

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 100;
    sample.mem_available_ratio = 0.20;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.8;
    sample.cpu_utilization_available = true;
    sample.load1 = 2.0;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);
    CHECK(controller.captureBaseline(profile));
    CHECK(controller.apply(RuntimeState::Elevated, sample, profile, config) == MutationResult::Verified);
    CHECK(readFile(swappiness) == "65");
    writeFile(readAhead, "2048");

    CHECK(controller.restoreAll());
    CHECK(readFile(swappiness) == "60");
    CHECK(readFile(readAhead) == "2048");
    CHECK(!journal.pending);
    fs::remove_all(root);
}

void test_policy_block_without_mutation_is_skipped() {
    const fs::path root =
        fs::temp_directory_path() / "coreflow_resource_policy_skip_test";

    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    profile.vm_swappiness.path = swappiness.string();
    profile.vm_swappiness.readable = true;
    profile.vm_swappiness.writable = true;

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 500;
    sample.mem_available_ratio = 0.50;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.20;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);

    PolicyPlan blocked;
    blocked.action = PolicyAction::ReduceIntervention;
    blocked.mutation_eligible = false;
    blocked.reason = "thermal safety guard";

    const MutationAuthority authority;
    const MutationPermit permit = authority.authorize(
        blocked,
        config,
        RuntimeState::ThermalGuard,
        sample.confidence,
        MutationPermit::Scope::Resource);

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);

    CHECK(controller.captureBaseline(profile));

    const MutationResult result = controller.apply(
        RuntimeState::ThermalGuard,
        sample,
        profile,
        config,
        blocked,
        permit);

    CHECK(result == MutationResult::Skipped);
    CHECK(!controller.mutated());
    CHECK(readFile(swappiness) == "60");

    fs::remove_all(root);
}

} // namespace

int main() {
    test_swappiness_adaptive_path();
    test_resource_restore_only_owned_paths();
    test_policy_block_without_mutation_is_skipped();

    std::printf(
        "CoreFlow resource autonomy: %s\n",
        failures == 0 ? "PASS" : "FAIL");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
