#include "coreflow/mutation_journal.hpp"
#include "coreflow/effect_model.hpp"
#include "coreflow/ecosystem_knowledge.hpp"
#include "coreflow/resource_actuator.hpp"
#include "coreflow/resource_mutation.hpp"
#include "coreflow/tunable_discovery.hpp"
#include "coreflow/retained_state.hpp"

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

// Mirrors what discovery produces for a writable, policy-admitted tunable.
void addSwappinessCapability(DeviceProfile& profile, const std::string& path) {
    ResourceCapability cap;
    cap.domain = ResourceDomain::Memory;
    cap.name = "vm.swappiness";
    cap.path = path;
    cap.exists = true;
    cap.readable = true;
    cap.writable = true;
    cap.permission_granted = true;
    cap.runtime_verified = true;
    cap.policy_authorized = true;
    cap.mutation_ready = true;
    profile.capabilities.resources.push_back(std::move(cap));
}

// Block-queue nodes as discovery admits them: one capability per queue node,
// named like the walker names it, with mutation readiness from its own flags.
void addBlockQueueCapability(DeviceProfile& profile, const IoDevice& device,
                             const std::string& leaf, bool readable, bool writable) {
    if (!readable && !writable) return;
    ResourceCapability cap;
    cap.domain = ResourceDomain::Io;
    cap.name = device.name + ":" + leaf;
    cap.path = device.path + "/queue/" + leaf;
    cap.exists = true;
    cap.readable = readable;
    cap.writable = writable;
    cap.permission_granted = writable;
    cap.runtime_verified = readable;
    cap.policy_authorized = true;
    cap.mutation_ready = readable && writable;
    profile.capabilities.resources.push_back(std::move(cap));
}

void addBlockQueueCapabilities(DeviceProfile& profile, const IoDevice& device) {
    addBlockQueueCapability(profile, device, "read_ahead_kb",
                            device.read_ahead_readable, device.read_ahead_writable);
    addBlockQueueCapability(profile, device, "nr_requests",
                            device.nr_requests_readable, device.nr_requests_writable);
    addBlockQueueCapability(profile, device, "scheduler",
                            device.scheduler_readable, device.scheduler_writable);
    // Discovery attaches kernel-advertised options after admission; do the same.
    TunableDiscovery{}.enrich(profile.capabilities, VendorKnowledge{});
}

void writeFile(const fs::path& p, const std::string& s) {
    std::ofstream f(p, std::ios::trunc);
    f << s << '\n';
}

class Journal final : public MutationJournal {
public:
    Entries entries;
    bool pending{false};
    bool fail_commit{false};
    bool fail_clear{false};
    bool corrupt{false};

    bool commit(const Entries& e) noexcept override {
        if (fail_commit) return false;
        entries = e;
        pending = true;
        corrupt = false;
        return true;
    }

    bool clear() noexcept override {
        if (fail_clear) return false;
        entries.clear();
        pending = false;
        corrupt = false;
        return true;
    }

    LoadState load(Entries& out) noexcept override {
        out = entries;
        if (corrupt) return LoadState::Corrupt;
        return pending ? LoadState::Pending : LoadState::Absent;
    }
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

// Safety property: with mutation armed and adaptive, but without the user's
// grant, an otherwise eligible headroom candidate must leave the kernel untouched.
// The brain defers, the hands never run, and the ledger says why.
void test_no_write_without_user_grant() {
    const fs::path root = fs::temp_directory_path() / "coreflow_resource_no_grant_test";
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    addSwappinessCapability(profile, swappiness.string());

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 100;
    sample.mem_available_ratio = 0.50;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.2;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    // Grants are opt-out; this user explicitly opts every namespace out.
    config.setGrantMemory(false);
    config.setGrantBlockQueue(false);
    config.setGrantScheduler(false);
    config.setGrantGpu(false);
    config.setGrantCgroup(false);
    config.setGrantCpuFreq(false);
    config.setGrantKernel(false);

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);
    CHECK(controller.captureBaseline(profile));
    const MutationResult result = applyResourcePolicy(
        controller, RuntimeState::Elevated, sample, profile, config,
        InterventionLevel::Low);

    CHECK(result != MutationResult::Verified);
    CHECK(readFile(swappiness) == "60");
    CHECK(!controller.mutated());
    const auto& ledger = controller.consultationLedger();
    CHECK(ledger.by_reason[static_cast<std::size_t>(ConsultReason::AwaitingUserGrant)] >= 1U);
    CHECK(ledger.by_reason[static_cast<std::size_t>(ConsultReason::Allowed)] == 0U);
}

void test_journal_commit_failure_is_diagnosed() {
    const fs::path root =
        fs::temp_directory_path() / "coreflow_resource_journal_failure_test";
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    addSwappinessCapability(profile, swappiness.string());

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 100;
    sample.mem_available_ratio = 0.50;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.2;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    config.setGrantMemory(true);

    Journal journal;
    journal.fail_commit = true;

    ResourceMutationController controller;
    controller.setJournal(&journal);
    CHECK(controller.captureBaseline(profile));

    const MutationResult result = applyResourcePolicy(
        controller, RuntimeState::Elevated, sample, profile, config,
        InterventionLevel::Low);

    CHECK(result == MutationResult::Failed);
    CHECK(controller.lastFailureStage() ==
          ResourceMutationFailureStage::JournalCommit);
    CHECK(readFile(swappiness) == "60");
    CHECK(!controller.mutated());
    CHECK(!journal.pending);

    fs::remove_all(root);
}

void test_file_journal_clear_reports_directory_sync_failure() {
    const fs::path root =
        fs::temp_directory_path() / "coreflow_journal_fsync_failure_test";
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path journal_path = root / "journal";
    FileMutationJournal journal(journal_path.string());
    const MutationJournal::Entries entries{{(root / "resource").string(), "60"}};

    CHECK(journal.commit(entries));
    CHECK(fs::exists(journal_path));

    coreflow::testing::failNextJournalDirectorySync(true);
    const bool cleared = journal.clear();

    CHECK(!cleared);
    CHECK(!fs::exists(journal_path));
    MutationJournal::Entries loaded;
    CHECK(journal.load(loaded) == MutationJournal::LoadState::Absent);
    CHECK(loaded.empty());

    fs::remove_all(root);
}

void test_file_journal_commit_reports_directory_sync_failure() {
    const fs::path root =
        fs::temp_directory_path() / "coreflow_journal_commit_fsync_failure_test";
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path journal_path = root / "journal";
    FileMutationJournal journal(journal_path.string());
    const MutationJournal::Entries entries{
        {(root / "resource").string(), "60"}
    };

    coreflow::testing::failNextJournalDirectorySync(true);
    CHECK(!journal.commit(entries));

    // Rename may already have succeeded before directory fsync fails.
    CHECK(fs::exists(journal_path));

    MutationJournal::Entries loaded;
    CHECK(journal.load(loaded) == MutationJournal::LoadState::Pending);
    CHECK(loaded == entries);

    CHECK(journal.clear());
    fs::remove_all(root);
}

void test_file_journal_rejects_ambiguous_records() {
    const fs::path root =
        fs::temp_directory_path() / "coreflow_file_journal_ambiguous_test";
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path journal_path = root / "journal";
    const std::string resource = (root / "resource").string();
    FileMutationJournal journal(journal_path.string());
    MutationJournal::Entries loaded;

    const MutationJournal::Entries duplicate_entries{
        {resource, "60"},
        {resource, "70"}
    };

    CHECK(!journal.commit(duplicate_entries));
    CHECK(journal.load(loaded) == MutationJournal::LoadState::Absent);
    CHECK(loaded.empty());

    const auto writeRaw = [&](const std::string& content) {
        std::ofstream file(
            journal_path, std::ios::binary | std::ios::trunc);
        file << content;
        file.close();
        CHECK(static_cast<bool>(file));
    };

    const std::string header = "coreflow-mutation-journal v1\n";

    writeRaw(header + resource + "\t60\nend\nunexpected-data\n");
    CHECK(journal.load(loaded) == MutationJournal::LoadState::Corrupt);
    CHECK(loaded.empty());

    writeRaw(header + resource + "\t60\n" +
             resource + "\t70\nend\n");
    CHECK(journal.load(loaded) == MutationJournal::LoadState::Corrupt);
    CHECK(loaded.empty());

    fs::remove_all(root);
}

void test_corrupt_journal_blocks_mutation() {
    Journal journal;
    journal.corrupt = true;

    ResourceMutationController controller;
    controller.setJournal(&journal);

    DeviceProfile profile;
    CHECK(!controller.captureBaseline(profile));
    CHECK(controller.lastFailureStage() ==
          ResourceMutationFailureStage::JournalCorrupt);
    CHECK(journal.corrupt);
    CHECK(!controller.mutated());
}

void test_recovery_requires_a_journal() {
    ResourceMutationController controller;
    const DeviceProfile profile;

    CHECK(!controller.captureBaseline(profile));
    CHECK(controller.lastFailureStage() ==
          ResourceMutationFailureStage::JournalUnavailable);
    CHECK(!controller.mutated());
}

void test_journal_clear_failure_blocks_recovery() {
    const fs::path root =
        fs::temp_directory_path() / "coreflow_journal_clear_failure_test";
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    addSwappinessCapability(profile, swappiness.string());

    Journal journal;
    journal.entries.emplace_back(swappiness.string(), "60");
    journal.pending = true;
    journal.fail_clear = true;

    ResourceMutationController controller;
    controller.setJournal(&journal);

    // Recovery must not proceed while the durable journal cannot be cleared.
    CHECK(!controller.captureBaseline(profile));
    CHECK(controller.lastFailureStage() ==
          ResourceMutationFailureStage::JournalCleanup);
    CHECK(journal.pending);
    CHECK(readFile(swappiness) == "60");
    CHECK(!controller.mutated());

    fs::remove_all(root);
}


void test_recovery_actuator_failure_is_diagnosed() {
    const fs::path root =
        fs::temp_directory_path() / "coreflow_recovery_actuator_failure_test";
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path target = root / "resource";
    const fs::path secondary = root / "secondary";
    writeFile(target, "90");
    writeFile(secondary, "90");

    DeviceProfile profile;
    Journal journal;
    journal.entries.emplace_back(target.string(), "60");
    journal.entries.emplace_back(secondary.string(), "60");
    journal.pending = true;

    // The actuator's writable() check inspects mode bits, even when tests run
    // as root. The existing file remains readable but is not writable.
    fs::permissions(target, fs::perms::owner_read,
                    fs::perm_options::replace);

    ResourceMutationController controller;
    controller.setJournal(&journal);

    CHECK(!controller.captureBaseline(profile));
    CHECK(controller.lastFailureStage() ==
          ResourceMutationFailureStage::RecoveryActuator);
    CHECK(controller.lastActuatorStatus() == ActuatorStatus::SafetyRejected);
    CHECK(journal.pending);
    CHECK(readFile(target) == "90");
    CHECK(readFile(secondary) == "60");

    fs::permissions(target, fs::perms::owner_all,
                    fs::perm_options::replace);
    fs::remove_all(root);
}


void test_missing_recovery_path_keeps_journal() {
    const fs::path root =
        fs::temp_directory_path() / "coreflow_missing_recovery_path_test";
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path missing = root / "resource-that-does-not-exist";

    DeviceProfile profile;
    Journal journal;
    journal.entries.emplace_back(missing.string(), "60");
    journal.pending = true;

    ResourceMutationController controller;
    controller.setJournal(&journal);

    CHECK(!controller.captureBaseline(profile));
    CHECK(controller.lastFailureStage() ==
          ResourceMutationFailureStage::RecoveryActuator);
    CHECK(controller.lastActuatorStatus() == ActuatorStatus::ReadFailed);
    CHECK(journal.pending);
    CHECK(!fs::exists(missing));

    fs::remove_all(root);
}

void test_swappiness_adaptive_path() {
    const fs::path root = fs::temp_directory_path() / "coreflow_resource_mutation_test";
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    addSwappinessCapability(profile, swappiness.string());

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 100;
    sample.mem_available_ratio = 0.50;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.2;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    // The user has granted these namespaces; a device need alone never does.
    config.setGrantMemory(true);
    config.setGrantBlockQueue(true);
    config.setGrantScheduler(true);

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
    CHECK(readFile(swappiness) == "53");  // bounded step, conservative probe first
    CHECK(controller.mutated());
    CHECK(controller.restoreAll());
    CHECK(readFile(swappiness) == "60");
    CHECK(!controller.mutated());
    fs::remove_all(root);
}

void test_retained_resource_state_survives_restart_and_revalidates() {
    const fs::path root = fs::temp_directory_path() / "coreflow_resource_retained_test";
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path swappiness = root / "swappiness";
    const fs::path retainedPath = root / "retained.db";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    addSwappinessCapability(profile, swappiness.string());

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 100;
    sample.mem_available_ratio = 0.50;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.2;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    // The user has granted these namespaces; a device need alone never does.
    config.setGrantMemory(true);
    config.setGrantBlockQueue(true);
    config.setGrantScheduler(true);

    Journal firstJournal;
    FileRetainedStateStore retained(retainedPath.string());
    ResourceMutationController first;
    first.setJournal(&firstJournal);
    first.setRetainedStateStore(&retained);
    CHECK(first.captureBaseline(profile));
    CHECK(applyResourcePolicy(first, RuntimeState::Elevated, sample, profile, config,
                              InterventionLevel::Low) == MutationResult::Verified);
    CHECK(first.retainLastMutation(profile));
    CHECK(readFile(swappiness) == "53");  // bounded step, conservative probe first
    CHECK(first.activeResourceSettings().size() == 1U);
    CHECK(first.restoreAll());
    CHECK(readFile(swappiness) == "60");
    CHECK(first.activeResourceSettings().empty());

    // Re-open the store as a new daemon would. The retained value is a
    // preference only; normal capability/policy/actuator validation still runs.
    Journal secondJournal;
    FileRetainedStateStore reopened(retainedPath.string());
    ResourceMutationController restarted;
    restarted.setJournal(&secondJournal);
    restarted.setRetainedStateStore(&reopened);
    CHECK(restarted.captureBaseline(profile));
    CHECK(applyResourcePolicy(restarted, RuntimeState::Elevated, sample, profile, config,
                              InterventionLevel::Low) == MutationResult::Verified);
    CHECK(readFile(swappiness) == "53");  // bounded step, conservative probe first
    CHECK(restarted.hasPendingOutcome());
    CHECK(restarted.rollbackLastMutation());
    CHECK(readFile(swappiness) == "60");

    RetainedStateStore::Entries entries;
    CHECK(reopened.load(entries) == RetainedStateStore::LoadState::Absent);
    fs::remove_all(root);
}

void test_restore_all_keeps_journal_when_dirty_path_disappears() {
    const fs::path root =
        fs::temp_directory_path() / "coreflow_restore_missing_dirty_test";
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    addSwappinessCapability(profile, swappiness.string());

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 100;
    sample.mem_available_ratio = 0.50;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.2;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    config.setGrantMemory(true);

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);

    CHECK(controller.captureBaseline(profile));
    CHECK(applyResourcePolicy(
        controller, RuntimeState::Elevated, sample, profile, config,
        InterventionLevel::Low) == MutationResult::Verified);
    CHECK(controller.mutated());
    CHECK(journal.pending);

    // Simulasikan node resource menghilang setelah mutasi.
    fs::remove(swappiness);

    CHECK(!controller.restoreAll());
    CHECK(controller.mutated());
    CHECK(journal.pending);
    CHECK(controller.lastFailureStage() ==
          ResourceMutationFailureStage::Rollback);

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
    addSwappinessCapability(profile, swappiness.string());
    // The block device stays in the profile so restore isolation is exercised,
    // but read_ahead is not writable here: block-queue writes are quarantined,
    // and this test is about owning only the swappiness write it performed.
    IoDevice device;
    device.path = root.string();
    device.name = "testblk";
    device.read_ahead_readable = true;
    device.read_ahead_writable = false;
    profile.io_devices.push_back(device);
    addBlockQueueCapabilities(profile, device);

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 100;
    sample.mem_available_ratio = 0.50;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.8;
    sample.cpu_utilization_available = true;
    sample.load1 = 2.0;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    // The user has granted these namespaces; a device need alone never does.
    config.setGrantMemory(true);
    config.setGrantBlockQueue(true);
    config.setGrantScheduler(true);

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
    CHECK(readFile(swappiness) == "53");  // bounded step, conservative probe first
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
    addSwappinessCapability(profile, swappiness.string());

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
    // The user has granted these namespaces; a device need alone never does.
    config.setGrantMemory(true);
    config.setGrantBlockQueue(true);
    config.setGrantScheduler(true);

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
    addBlockQueueCapabilities(profile, device);

        RuntimeSample sample;
        sample.mem_total_kb = 1000;
        sample.mem_available_kb = 500;
        sample.mem_available_ratio = 0.50;
        sample.confidence = 1.0;
        sample.cpu_utilization = 0.80;
        sample.cpu_utilization_available = true;
        sample.load1 = 2.0;
        sample.io_activity_available = true;
        sample.io_read_kb_per_sec = 2048.0;
        sample.io_write_kb_per_sec = 1024.0;

        EngineConfig config;
        config.setMutationMode(MutationMode::Adaptive);
        config.setMutationArmed(true);
        // The user has granted these namespaces; a device need alone never does.
        config.setGrantMemory(true);
        config.setGrantBlockQueue(true);
        config.setGrantScheduler(true);

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
            "112");
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
            "112");
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


void test_disarmed_policy_performs_no_write() {
    const fs::path root =
        fs::temp_directory_path() /
        "coreflow_resource_disarmed_test";

    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    addSwappinessCapability(profile, swappiness.string());

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 500;
    sample.mem_available_ratio = 0.50;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.80;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(false);

    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.intervention = InterventionLevel::Low;
    plan.mutation_eligible = true;
    plan.candidates.push_back({
        ResourceDomain::Memory,
        "vm.swappiness",
        {},
        sample.confidence,
        1.0,
        PolicyAction::Candidate
    });

    const MutationAuthority authority;
    const MutationPermit permit = authority.authorize(
        plan,
        config,
        RuntimeState::Elevated,
        sample.confidence,
        MutationPermit::Scope::Resource);

    CHECK(!permit.validFor(MutationPermit::Scope::Resource));

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);

    CHECK(controller.captureBaseline(profile));

    const MutationResult result = controller.apply(
        RuntimeState::Elevated,
        sample,
        profile,
        config,
        plan,
        permit);

    CHECK(result == MutationResult::Skipped);
    CHECK(!controller.mutated());
    CHECK(readFile(swappiness) == "60");
    CHECK(!journal.pending);

    fs::remove_all(root);
}



void test_effect_model_structured_rank_and_unknown_semantics() {
    EffectModel model;
    EnvironmentCapabilityMatrix matrix;

    ResourceCapability known;
    known.domain = ResourceDomain::Memory;
    known.name = "vm.swappiness";
    known.path = "/proc/sys/vm/swappiness";
    known.exists = true;
    known.readable = true;
    known.writable = true;
    known.permission_granted = true;
    known.policy_authorized = true;
    known.runtime_verified = true;
    known.mutation_ready = true;
    matrix.resources.push_back(known);

    ResourceCapability unknown = known;
    unknown.name = "experimental_numeric_knob";
    unknown.path = "/proc/sys/vm/experimental_numeric_knob";
    matrix.resources.push_back(unknown);

    std::unordered_map<std::string, std::string> baselines{
        {known.path, "60"}, {unknown.path, "100"}};
    EffectContext context;
    context.state = RuntimeState::Pressure;
    context.mem_available_ratio = 0.08;
    context.sample_confidence = 0.95;
    context.intervention = InterventionLevel::Low;

    const auto ranked = model.rank(matrix, baselines, context, 4);
    // A memory-pressure label is not enough to justify arbitrary numeric steps.
    CHECK(ranked.empty());
    const EffectScore pressured_swappiness = model.evaluate(known, "60", context);
    CHECK(!pressured_swappiness.eligible);
    CHECK(pressured_swappiness.requested.empty());

    // Unknown numeric nodes remain capability-driven, but exploration waits for
    // headroom rather than guessing that a direction relieves memory pressure.
    const EffectScore pressured_unknown = model.evaluate(unknown, "100", context);
    CHECK(!pressured_unknown.eligible);
    CHECK(pressured_unknown.requested.empty());

    EffectContext headroom = context;
    headroom.state = RuntimeState::Elevated;
    headroom.mem_available_ratio = 0.50;
    const EffectScore unknown_score = model.evaluate(unknown, "100", headroom);
    CHECK(unknown_score.eligible);
    CHECK(unknown_score.requested == "88" || unknown_score.requested == "112");

    ResourceCapability notAuthorized = known;
    notAuthorized.policy_authorized = false;
    const EffectScore denied = model.evaluate(notAuthorized, "60", context);
    CHECK(!denied.eligible);
    CHECK(denied.requested.empty());

    // Catalog risk levels are inputs to proposal eligibility, not just UI text.
    ResourceCapability observeOnly = known;
    observeOnly.name = "vm.overcommit_memory";
    observeOnly.path = "/proc/sys/vm/overcommit_memory";
    CHECK(!model.evaluate(observeOnly, "2", headroom).eligible);

    // Catalog lookup must distinguish dynamic per-device queue nodes.
    const auto* scheduler = lookupEcosystemKnowledgeByPath(
        "/sys/block/dm-0/queue/scheduler");
    const auto* readAhead = lookupEcosystemKnowledgeByPath(
        "/sys/block/mmcblk0/queue/read_ahead_kb");
    const auto* requests = lookupEcosystemKnowledgeByPath(
        "/sys/devices/platform/soc/1d84000.ufshc/block/sda/queue/nr_requests");
    CHECK(scheduler != nullptr && std::string(scheduler->key) == "block.queue.scheduler");
    CHECK(readAhead != nullptr && std::string(readAhead->key) == "block.queue.read_ahead_kb");
    CHECK(requests != nullptr && std::string(requests->key) == "block.queue.nr_requests");


    // Regression: physical block queue tunables remain explorable with
    // headroom, but are not presumed to stabilize memory pressure.
    ResourceCapability readAheadCap = known;
    readAheadCap.domain = ResourceDomain::Io;
    readAheadCap.name = "sda:read_ahead_kb";
    readAheadCap.path = "/sys/block/sda/queue/read_ahead_kb";

    ResourceCapability requestsCap = known;
    requestsCap.domain = ResourceDomain::Io;
    requestsCap.name = "sda:nr_requests";
    requestsCap.path = "/sys/block/sda/queue/nr_requests";

    const EffectScore readAheadHeadroom =
        model.evaluate(readAheadCap, "128", headroom);
    CHECK(readAheadHeadroom.eligible);
    CHECK(readAheadHeadroom.requested == "112" ||
          readAheadHeadroom.requested == "144");

    const EffectScore requestsHeadroom =
        model.evaluate(requestsCap, "128", headroom);
    CHECK(requestsHeadroom.eligible);
    CHECK(requestsHeadroom.requested == "112" ||
          requestsHeadroom.requested == "144");

    const EffectScore readAheadPressure =
        model.evaluate(readAheadCap, "128", context);
    CHECK(!readAheadPressure.eligible);
    CHECK(readAheadPressure.requested.empty());

    const EffectScore requestsPressure =
        model.evaluate(requestsCap, "128", context);
    CHECK(!requestsPressure.eligible);
    CHECK(requestsPressure.requested.empty());

    // The canonical path must take precedence over a misleading capability
    // name. A scheduler path must not inherit read_ahead_kb's BoundedTune risk.
    ResourceCapability pathWins = readAheadCap;
    pathWins.name = "block.queue.read_ahead_kb";
    pathWins.path = "/sys/block/sda/queue/scheduler";
    const EffectScore schedulerPressure =
        model.evaluate(pathWins, "[mq-deadline] kyber none", context);
    CHECK(!schedulerPressure.eligible);

    // Fatal surfaces are never candidates, even if marked mutation_ready.
    ResourceCapability fatal = known;
    fatal.name = "vm.drop_caches";
    fatal.path = "/proc/sys/vm/drop_caches";
    CHECK(!model.evaluate(fatal, "3", context).eligible);

    // Vendor knowledge outranks blind exploration.
    ResourceCapability vendor = known;
    vendor.name = "vm.vendor_knob";
    vendor.path = "/proc/sys/vm/vendor_knob";
    vendor.vendor_values.push_back(VendorValue{"40", VendorProfile::Efficiency, "test.rc"});
    const EffectScore vendor_score = model.evaluate(vendor, "100", context);
    CHECK(vendor_score.eligible && vendor_score.stabilizing);
    CHECK(vendor_score.requested == "40");
    CHECK(vendor_score.source == CandidateSource::VendorEfficiency);
}

void test_block_queue_mutation_targets_are_admitted_for_physical() {
    // Capability-driven: every kernel tunable is admitted, including scheduler
    // tokens, virtual block devices, sched sysctls, GPU and uclamp nodes.
    CHECK(ResourceActuator::mutationTargetAllowed("/sys/block/loop0/queue/scheduler"));
    CHECK(ResourceActuator::mutationTargetAllowed("/sys/block/dm-0/queue/nr_requests"));
    CHECK(ResourceActuator::mutationTargetAllowed("/sys/block/zram0/queue/read_ahead_kb"));
    CHECK(ResourceActuator::mutationTargetAllowed("/sys/block/mmcblk0/queue/scheduler"));
    CHECK(ResourceActuator::mutationTargetAllowed("/proc/sys/kernel/sched_latency_ns"));
    CHECK(ResourceActuator::mutationTargetAllowed("/proc/sys/vm/swappiness"));
    CHECK(ResourceActuator::mutationTargetAllowed("/proc/sys/vm/page-cluster"));
    CHECK(ResourceActuator::mutationTargetAllowed("/sys/class/kgsl/kgsl-3d0/max_gpuclk"));
    CHECK(ResourceActuator::mutationTargetAllowed("/sys/class/devfreq/soc:qcom,gpubw/governor"));
    CHECK(ResourceActuator::mutationTargetAllowed("/dev/cpuctl/top-app/cpu.uclamp.min"));
    CHECK(ResourceActuator::mutationTargetAllowed(
        "/sys/devices/system/cpu/cpufreq/policy4/schedutil/up_rate_limit_us"));
    CHECK(ResourceActuator::mutationTargetAllowed("/tmp/coreflow-test/queue/scheduler"));

    // Only universally fatal classes are refused.
    CHECK(!ResourceActuator::mutationTargetAllowed("/proc/sys/vm/drop_caches"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/proc/sys/kernel/panic"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/proc/sys/vm/overcommit_memory"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/sys/class/thermal/thermal_zone0/mode"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/sys/class/power_supply/battery/charge_control_limit"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/sys/devices/system/cpu/cpu1/online"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/sys/block/sda/queue/write_cache"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/sys/fs/selinux/enforce"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/dev/cpuctl/top-app/cgroup.procs"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/sys/kernel/debug/sched_features"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/data/local/tmp/anything"));
    // The CPU governor belongs to MutationController.
    CHECK(!ResourceActuator::mutationTargetAllowed(
        "/sys/devices/system/cpu/cpufreq/policy0/scaling_governor"));

    // The central actuator guard rejects fatal paths before any I/O.
    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.mutation_eligible = true;
    plan.intervention = InterventionLevel::Low;
    const MutationAuthority authority;
    const MutationPermit permit = authority.authorize(
        plan, config, RuntimeState::Elevated, 1.0,
        MutationPermit::Scope::Resource);
    CHECK(permit.validFor(MutationPermit::Scope::Resource));
    ResourceActuator actuator;
    const ActuatorMutation mutation{
        {ActuatorDomain::Vm, 0}, "/proc/sys/vm/drop_caches", "3"};
    const ActuatorResult result = actuator.apply(mutation, permit);
    CHECK(result.status == ActuatorStatus::SafetyRejected);
    CHECK(result.writes_attempted == 0);
}

} // namespace

// Regression: sysfs selector nodes such as queue/scheduler read back as
// "[mq-deadline] kyber none". The kernel accepts only the bare token on write,
// so a baseline captured as the bracketed line could never be restored.
void test_selector_value_comparable_form() {
    CHECK(ResourceActuator::comparableValue("[mq-deadline] kyber none") == "mq-deadline");
    CHECK(ResourceActuator::comparableValue("mq-deadline [none]") == "none");
    CHECK(ResourceActuator::comparableValue("none") == "none");
    CHECK(ResourceActuator::comparableValue("  128 \n") == "128");
    CHECK(ResourceActuator::comparableValue("performance") == "performance");
    CHECK(ResourceActuator::comparableValue("") == "");
}

void test_selector_scheduler_restore_writes_bare_token() {
    const fs::path root =
        fs::temp_directory_path() / "coreflow_selector_scheduler_regress";
    fs::remove_all(root);
    fs::create_directories(root / "queue");
    const fs::path schedulerPath = root / "queue" / "scheduler";
    writeFile(schedulerPath, "[mq-deadline] none");

    DeviceProfile profile;
    IoDevice device;
    device.path = root.string();
    device.name = "testblk";
    device.scheduler_readable = true;
    device.scheduler_writable = true;
    profile.io_devices.push_back(device);
    addBlockQueueCapabilities(profile, device);

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
    // The user has granted these namespaces; a device need alone never does.
    config.setGrantMemory(true);
    config.setGrantBlockQueue(true);
    config.setGrantScheduler(true);

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);
    CHECK(controller.captureBaseline(profile));

    CHECK(applyResourcePolicy(controller, RuntimeState::Elevated, sample,
                              profile, config, InterventionLevel::High)
          == MutationResult::Verified);
    CHECK(readFile(schedulerPath) == "none");
    CHECK(controller.mutated());

    // Rollback must write the bare factory token the kernel accepts.
    CHECK(controller.restoreAll());
    CHECK(readFile(schedulerPath) == "mq-deadline");
    CHECK(!controller.mutated());

    fs::remove_all(root);
}

// A journal written by an earlier build may hold the bracketed line. Recovery
// and restore must normalize it instead of writing an unacceptable value.
void test_restore_enforces_mutation_surface_guard() {
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / "coreflow_restore_surface_guard";
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path fatal = root / "drop_caches";
    writeFile(fatal, "0");
    ResourceActuator actuator;
    const ActuatorResult fatal_result = actuator.restore(fatal.string(), "3");
    CHECK(fatal_result.status == ActuatorStatus::SafetyRejected);
    CHECK(readFile(fatal) == "0");

    // CPU governor writes belong exclusively to CpuFreqActuator, including
    // during crash recovery from a resource journal.
    const fs::path governor = root / "scaling_governor";
    writeFile(governor, "schedutil");
    const ActuatorResult governor_result = actuator.restore(governor.string(), "performance");
    CHECK(governor_result.status == ActuatorStatus::SafetyRejected);
    CHECK(readFile(governor) == "schedutil");

    const fs::path ordinary = root / "ordinary_tunable";
    const fs::path symlink = root / "redirected_tunable";
    writeFile(ordinary, "10");
    std::error_code symlink_error;
    fs::create_symlink(ordinary, symlink, symlink_error);
    if (!symlink_error) {
        CHECK(!ResourceActuator::writable(symlink.string()));
        const ActuatorResult symlink_result = actuator.restore(symlink.string(), "20");
        CHECK(symlink_result.status == ActuatorStatus::SafetyRejected);
        CHECK(readFile(ordinary) == "10");
    }

    fs::remove_all(root);
}

void test_legacy_bracketed_journal_value_is_normalized() {
    const fs::path path = fs::temp_directory_path() / "coreflow_legacy_journal_sched";
    writeFile(path, "none");

    ResourceActuator actuator;
    const ActuatorResult result = actuator.restore(path.string(), "[mq-deadline] none");
    CHECK(result.status == ActuatorStatus::RolledBack);
    CHECK(readFile(path) == "mq-deadline");

    fs::remove(path);
}

// The CPU governor permit must not be issued without the separate opt-in.
void test_cpu_permit_requires_governor_opt_in() {
    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.mutation_eligible = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    // The user has granted these namespaces; a device need alone never does.
    config.setGrantMemory(true);
    config.setGrantBlockQueue(true);
    config.setGrantScheduler(true);
    config.setAllowCpuGovernor(false);

    const MutationAuthority authority;
    CHECK(!authority.authorize(plan, config, RuntimeState::Elevated, 1.0,
                               MutationPermit::Scope::CpuFreq)
              .validFor(MutationPermit::Scope::CpuFreq));

    config.setAllowCpuGovernor(true);
    CHECK(authority.authorize(plan, config, RuntimeState::Elevated, 1.0,
                              MutationPermit::Scope::CpuFreq)
              .validFor(MutationPermit::Scope::CpuFreq));
}

// ---- v2.1.1: graded safety. Stress allows small load-reducing writes only. ----
namespace stab {

struct Fixture {
    fs::path root;
    fs::path swappiness;
    fs::path gpu_max;
    DeviceProfile profile;
    RuntimeSample sample;
    EngineConfig config;
    Journal journal;
    ResourceMutationController controller;

    explicit Fixture(const std::string& name) {
        root = fs::temp_directory_path() / name;
        fs::remove_all(root);
        fs::create_directories(root);
        swappiness = root / "swappiness";
        writeFile(swappiness, "60");
        // An ordered hardware scale (GPU-style max_freq + available list).
        fs::create_directories(root / "gpu");
        gpu_max = root / "gpu" / "max_freq";
        writeFile(gpu_max, "600");
        writeFile(root / "gpu" / "available_frequencies", "300 450 600");
        {
            ResourceCapability cap;
            cap.domain = ResourceDomain::Gpu;
            cap.name = "gpu.max_freq";
            cap.path = gpu_max.string();
            cap.exists = cap.readable = cap.writable = true;
            cap.permission_granted = cap.policy_authorized = true;
            cap.runtime_verified = cap.mutation_ready = true;
            profile.capabilities.resources.push_back(cap);
            TunableDiscovery{}.enrich(profile.capabilities, VendorKnowledge{});
        }
        addSwappinessCapability(profile, swappiness.string());
        sample.mem_total_kb = 1000;
        sample.mem_available_kb = 500;
        sample.mem_available_ratio = 0.50;
        sample.confidence = 1.0;
        sample.cpu_utilization = 0.50;
        sample.cpu_utilization_available = true;
        sample.thermal_available = true;
        sample.thermal_millidegrees = 30000;
        config.setMutationMode(MutationMode::Adaptive);
        config.setMutationArmed(true);
        // The user has granted these namespaces; a device need alone never does.
        config.setGrantMemory(true);
        config.setGrantBlockQueue(true);
        config.setGrantScheduler(true);
        controller.setJournal(&journal);
    }
    ~Fixture() { fs::remove_all(root); }

    static PolicyPlan stabilizingPlan() {
        PolicyPlan plan;
        plan.action = PolicyAction::Candidate;
        plan.intervention = InterventionLevel::Low;
        plan.mutation_eligible = true;
        plan.stabilizing_only = true;
        plan.candidates.push_back({ResourceDomain::Memory, "vm.swappiness", {}, 1.0, 0.3,
                                   PolicyAction::Candidate});
        plan.candidates.push_back({ResourceDomain::Gpu, "gpu.max_freq", {}, 1.0, 0.3,
                                   PolicyAction::Candidate});
        return plan;
    }

    MutationResult run(RuntimeState state, const PolicyPlan& plan) {
        const MutationAuthority authority;
        const MutationPermit permit = authority.authorize(
            plan, config, state, sample.confidence, MutationPermit::Scope::Resource);
        return controller.apply(state, sample, profile, config, plan, permit);
    }
};

} // namespace stab

void test_thermal_guard_allows_stabilizing_write_and_restores() {
    stab::Fixture f("coreflow_stab_hot");
    f.sample.thermal_millidegrees = 55000;
    CHECK(f.controller.captureBaseline(f.profile));

    CHECK(f.run(RuntimeState::ThermalGuard, stab::Fixture::stabilizingPlan()) ==
          MutationResult::Verified);
    // Thermal stress: one step down the hardware's own advertised scale. The
    // memory knob has no load-reducing evidence and is left untouched.
    CHECK(readFile(f.gpu_max) == "450");
    CHECK(readFile(f.swappiness) == "60");
    CHECK(f.controller.mutated());
    CHECK(f.controller.stabilizingEpoch());
    CHECK(f.journal.pending);

    CHECK(f.controller.restoreAll());
    CHECK(readFile(f.gpu_max) == "600");
    CHECK(!f.controller.stabilizingEpoch());
    CHECK(!f.journal.pending);
}

void test_pressure_defers_without_memory_evidence() {
    stab::Fixture f("coreflow_stab_pressure");
    f.sample.mem_available_ratio = 0.08;
    f.sample.mem_available_kb = 80;
    CHECK(f.controller.captureBaseline(f.profile));

    CHECK(f.run(RuntimeState::Pressure, stab::Fixture::stabilizingPlan()) ==
          MutationResult::Skipped);
    // Neither swappiness nor an unrelated GPU scale has memory-specific
    // stabilizing evidence, so pressure triggers a hold instead of exploration.
    CHECK(readFile(f.swappiness) == "60");
    CHECK(readFile(f.gpu_max) == "600");
    CHECK(!f.controller.stabilizingEpoch());
    CHECK(!f.controller.mutated());
    CHECK(!f.journal.pending);
}

void test_stabilizing_plan_rejects_non_stabilizing_candidates() {
    // A calm system under a stabilizing-only plan: the EffectModel would offer a
    // mild efficiency tweak, but that is not a response to stress, so nothing is
    // written.
    stab::Fixture f("coreflow_stab_calm");
    CHECK(f.controller.captureBaseline(f.profile));
    CHECK(f.run(RuntimeState::Normal, stab::Fixture::stabilizingPlan()) ==
          MutationResult::Skipped);
    CHECK(readFile(f.swappiness) == "60");
    CHECK(!f.controller.mutated());
}

void test_safety_state_without_stabilizing_plan_never_writes() {
    stab::Fixture f("coreflow_stab_plain");
    f.sample.thermal_millidegrees = 55000;
    CHECK(f.controller.captureBaseline(f.profile));
    PolicyPlan plain = stab::Fixture::stabilizingPlan();
    plain.stabilizing_only = false;   // an ordinary candidate plan
    CHECK(f.run(RuntimeState::ThermalGuard, plain) == MutationResult::Skipped);
    CHECK(readFile(f.swappiness) == "60");
    CHECK(readFile(f.gpu_max) == "600");
    CHECK(!f.controller.mutated());
}

void test_regressed_candidate_is_suppressed() {
    stab::Fixture f("coreflow_stab_reject");
    f.sample.thermal_millidegrees = 55000;
    CHECK(f.controller.captureBaseline(f.profile));
    const PolicyPlan plan = stab::Fixture::stabilizingPlan();

    CHECK(f.run(RuntimeState::ThermalGuard, plan) == MutationResult::Verified);
    // The daemon rejects the candidate when its measured outcome regressed, then
    // restores.
    CHECK(readFile(f.gpu_max) == "450");
    f.controller.rejectLastMutation();
    CHECK(f.controller.restoreAll());
    CHECK(readFile(f.gpu_max) == "600");

    // The same candidate is not selected again within the cooldown.
    CHECK(f.run(RuntimeState::ThermalGuard, plan) == MutationResult::Skipped);
    CHECK(readFile(f.gpu_max) == "600");
    CHECK(!f.controller.mutated());
}


void test_resource_actuator_rejects_scope_only_permit() {
    const fs::path root = fs::temp_directory_path() / "coreflow_resource_scope_permit_test";
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    ActuatorMutation mutation;
    mutation.id = {ActuatorDomain::Vm, 0};
    const std::string swappiness_target = swappiness.string();
    mutation.target = swappiness_target;
    mutation.requested = "67";

    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.mutation_eligible = true;
    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    config.setGrantMemory(true);

    const MutationAuthority authority;
    const MutationPermit scope = authority.authorize(
        plan, config, RuntimeState::Elevated, 1.0, MutationPermit::Scope::Resource);
    ResourceActuator actuator;
    const ActuatorResult result = actuator.apply(mutation, scope);
    CHECK(result.status == ActuatorStatus::SafetyRejected);
    CHECK(readFile(swappiness) == "60");
    fs::remove_all(root);
}

int main() {
    test_resource_actuator_rejects_scope_only_permit();
    test_journal_commit_failure_is_diagnosed();
    test_file_journal_clear_reports_directory_sync_failure();
    test_file_journal_commit_reports_directory_sync_failure();
    test_file_journal_rejects_ambiguous_records();
    test_corrupt_journal_blocks_mutation();
    test_recovery_requires_a_journal();
    test_journal_clear_failure_blocks_recovery();
    test_recovery_actuator_failure_is_diagnosed();
    test_missing_recovery_path_keeps_journal();
    test_no_write_without_user_grant();
    test_swappiness_adaptive_path();
    test_retained_resource_state_survives_restart_and_revalidates();
    test_restore_all_keeps_journal_when_dirty_path_disappears();
    test_resource_restore_only_owned_paths();
    test_policy_block_without_mutation_is_skipped();
    test_intervention_level_boundaries();
    test_disarmed_policy_performs_no_write();
    test_effect_model_structured_rank_and_unknown_semantics();
    test_block_queue_mutation_targets_are_admitted_for_physical();
    test_selector_value_comparable_form();
    test_selector_scheduler_restore_writes_bare_token();
    test_restore_enforces_mutation_surface_guard();
    test_legacy_bracketed_journal_value_is_normalized();
    test_cpu_permit_requires_governor_opt_in();
    test_thermal_guard_allows_stabilizing_write_and_restores();
    test_pressure_defers_without_memory_evidence();
    test_stabilizing_plan_rejects_non_stabilizing_candidates();
    test_safety_state_without_stabilizing_plan_never_writes();
    test_regressed_candidate_is_suppressed();
    std::printf("CoreFlow resource autonomy: %s\n", failures == 0 ? "PASS" : "FAIL");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
