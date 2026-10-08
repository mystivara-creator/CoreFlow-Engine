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

MutationResult applyResourcePolicy(
    ResourceMutationController& controller,
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const EngineConfig& config,
    InterventionLevel intervention) {

    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.intervention = intervention;
    plan.mutation_eligible = true;

    plan.candidates.push_back({
        ResourceDomain::Memory,
        "vm.swappiness",
        {},
        sample.confidence,
        1.0,
        PolicyAction::Candidate
    });

    for (const auto& device : profile.io_devices) {
        plan.candidates.push_back({
            ResourceDomain::Io,
            device.name + ":read_ahead_kb",
            {},
            sample.confidence,
            1.0,
            PolicyAction::Candidate
        });

        plan.candidates.push_back({
            ResourceDomain::Io,
            device.name + ":nr_requests",
            {},
            sample.confidence,
            1.0,
            PolicyAction::Candidate
        });

        plan.candidates.push_back({
            ResourceDomain::Io,
            device.name + ":scheduler",
            {},
            sample.confidence,
            1.0,
            PolicyAction::Candidate
        });
    }

    const MutationAuthority authority;
    const MutationPermit permit = authority.authorize(
        plan,
        config,
        state,
        sample.confidence,
        MutationPermit::Scope::Resource);

    return controller.apply(
        state,
        sample,
        profile,
        config,
        plan,
        permit);
}

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
    CHECK(applyResourcePolicy(
          controller,
          RuntimeState::Elevated,
          sample,
          profile,
          config,
          InterventionLevel::Low) == MutationResult::Verified);
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
    CHECK(applyResourcePolicy(
          controller,
          RuntimeState::Elevated,
          sample,
          profile,
          config,
          InterventionLevel::Low) == MutationResult::Verified);
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
        fs::temp_directory_path() /
        "coreflow_resource_policy_skip_test";

    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path swappiness =
        root / "swappiness";

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
    blocked.intervention = InterventionLevel::ObserveOnly;
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


void test_intervention_level_boundaries() {
    auto run_case = [](InterventionLevel level,
                       bool read_ahead,
                       bool nr_requests,
                       bool scheduler,
                       const std::string& expected_path,
                       const std::string& expected_value) {
        const fs::path root =
            fs::temp_directory_path() /
            ("coreflow_intervention_" +
             std::to_string(static_cast<int>(level)));

        fs::remove_all(root);
        fs::create_directories(root / "queue");

        const fs::path readAhead =
            root / "queue" / "read_ahead_kb";
        const fs::path requests =
            root / "queue" / "nr_requests";
        const fs::path schedulerPath =
            root / "queue" / "scheduler";
        const fs::path rotational =
            root / "rotational";

        writeFile(readAhead, "128");
        writeFile(requests, "128");
        writeFile(schedulerPath, "[mq-deadline] none");
        writeFile(rotational, "0");

        DeviceProfile profile;

        IoDevice device;
        device.path = root.string();
        device.name = "testblk";

        device.read_ahead_readable = read_ahead;
        device.read_ahead_writable = read_ahead;

        device.nr_requests_readable = nr_requests;
        device.nr_requests_writable = nr_requests;

        device.scheduler_readable = scheduler;
        device.scheduler_writable = scheduler;

        profile.io_devices.push_back(device);

        RuntimeSample sample;
        sample.mem_total_kb = 1000;
        sample.mem_available_kb = 500;
        sample.mem_available_ratio = 0.50;
        sample.confidence = 1.0;
        sample.cpu_utilization = 0.80;
        sample.cpu_utilization_available = true;
        sample.load1 = 2.0;

        EngineConfig config;
        config.setMutationMode(MutationMode::Adaptive);
        config.setMutationArmed(true);

        Journal journal;
        ResourceMutationController controller;
        controller.setJournal(&journal);

        CHECK(controller.captureBaseline(profile));

        CHECK(applyResourcePolicy(
            controller,
            RuntimeState::Elevated,
            sample,
            profile,
            config,
            level) == MutationResult::Verified);

        CHECK(readFile(expected_path) == expected_value);
        CHECK(controller.mutated());

        CHECK(controller.restoreAll());

        fs::remove_all(root);
    };

    {
        const fs::path root =
            fs::temp_directory_path() /
            "coreflow_intervention_1";

        run_case(
            InterventionLevel::Low,
            true,
            false,
            false,
            root / "queue" / "read_ahead_kb",
            "160");
    }

    {
        const fs::path root =
            fs::temp_directory_path() /
            "coreflow_intervention_2";

        run_case(
            InterventionLevel::Moderate,
            false,
            true,
            false,
            root / "queue" / "nr_requests",
            "154");
    }

    {
        const fs::path root =
            fs::temp_directory_path() /
            "coreflow_intervention_3";

        run_case(
            InterventionLevel::High,
            false,
            false,
            true,
            root / "queue" / "scheduler",
            "none");
    }
}

} // namespace

int main() {
    test_swappiness_adaptive_path();
    test_resource_restore_only_owned_paths();
    test_policy_block_without_mutation_is_skipped();
    test_intervention_level_boundaries();
    std::printf("CoreFlow resource autonomy: %s\n", failures == 0 ? "PASS" : "FAIL");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
