// Deterministic host tests for the CoreFlow mutation-safety invariants.
// The CPUFreq sysfs tree is simulated with a temporary directory, so these
// tests never touch the real kernel interface.

#include "coreflow/config.hpp"
#include "coreflow/experience.hpp"
#include "coreflow/mutation.hpp"
#include "coreflow/mutation_journal.hpp"
#include "coreflow/policy.hpp"
#include "coreflow/safety_hold.hpp"
#include "coreflow/types.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

using namespace coreflow;
namespace fs = std::filesystem;

namespace {

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failures;                                                        \
            std::fprintf(stderr, "  CHECK FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                        \
    } while (0)

std::string readLine(const std::string& path) {
    std::ifstream f(path);
    std::string s;
    std::getline(f, s);
    return s;
}

void writeLine(const std::string& path, const std::string& value) {
    std::ofstream f(path, std::ios::trunc);
    f << value << "\n";
}

struct Sandbox {
    fs::path root;
    std::string policy;
    std::string governor_file;
    std::string journal_file;

    Sandbox() {
        char tmpl[] = "/tmp/coreflow_test_XXXXXX";
        char* made = mkdtemp(tmpl);
        if (made == nullptr) std::abort();
        root = made;
        policy = (root / "policy0").string();
        governor_file = policy + "/scaling_governor";
        journal_file = (root / "mutation_journal.txt").string();
        fs::create_directories(policy);
        writeLine(governor_file, "powersave");
    }

    ~Sandbox() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }

    DeviceProfile profile(const std::string& live_governor = "powersave") const {
        writeLine(governor_file, live_governor);
        DeviceProfile profile;
        CpuPolicy p;
        p.id = 0;
        p.path = policy;
        p.governor = live_governor;
        p.available_governors = {"powersave", "conservative", "schedutil", "performance"};
        p.readable = true;
        p.governor_writable = true;
        profile.cpu_policies.push_back(p);
        return profile;
    }
};

RuntimeSample sample(double util, double thermalC) {
    RuntimeSample s;
    s.mem_total_kb = 1000;
    s.mem_available_kb = 500;
    s.mem_available_ratio = 0.5;
    s.load1 = util * 8.0;
    s.cpu_utilization = util;
    s.cpu_utilization_available = true;
    s.thermal_available = true;
    s.thermal_millidegrees = static_cast<long>(thermalC * 1000.0);
    s.confidence = 1.0;
    return s;
}

EngineConfig adaptiveConfig() {
    EngineConfig cfg;
    cfg.setMutationMode(MutationMode::Adaptive);
    cfg.setMutationArmed(true);
    cfg.setAllowCpuGovernor(true);
    return cfg;
}

// Journal test double with controllable failure.
class FakeJournal final : public MutationJournal {
public:
    bool fail_commit{false};
    bool fail_clear{false};
    int commits{0};
    int clears{0};
    Entries stored;
    bool has_pending{false};

    bool commit(const Entries& values) noexcept override {
        if (fail_commit) return false;
        ++commits;
        stored = values;
        has_pending = true;
        return true;
    }
    bool clear() noexcept override {
        if (fail_clear) return false;
        ++clears;
        has_pending = false;
        stored.clear();
        return true;
    }
    LoadState load(Entries& out) noexcept override {
        out = stored;
        return has_pending ? LoadState::Pending : LoadState::Absent;
    }
};

// ---------------------------------------------------------------------------

void test_restore_returns_to_factory_and_clears_journal() {
    Sandbox box;
    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();

    CHECK(mc.captureBaseline(profile));
    const MutationResult applied = mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig());
    CHECK(applied == MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "schedutil");
    CHECK(mc.mutated());
    CHECK(journal.commits == 1);

    CHECK(mc.restoreAll());
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(!mc.mutated());
    CHECK(journal.clears == 1);
    CHECK(!journal.has_pending);
}

void test_live_governor_drives_decisions_not_stale_snapshot() {
    // Regression test for the stale-governor bug: after switching to schedutil,
    // a hot ThermalGuard decision must move the live governor to powersave.
    Sandbox box;
    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();

    CHECK(mc.captureBaseline(profile));
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "schedutil");

    const MutationResult hot = mc.apply(RuntimeState::ThermalGuard, sample(0.0, 45.0), profile, adaptiveConfig());
    CHECK(hot == MutationResult::RolledBack);
    CHECK(readLine(box.governor_file) == "powersave");
}

void test_rejected_candidate_is_not_reselected_during_cooldown() {
    // Regression test for the dead rejection bug: a rejected governor must not
    // be re-applied during its cooldown, and must become eligible afterwards.
    Sandbox box;
    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();

    CHECK(mc.captureBaseline(profile));
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "schedutil");

    mc.rejectLastMutation();
    CHECK(mc.restoreAll());
    CHECK(readLine(box.governor_file) == "powersave");

    for (int i = 0; i < 50; ++i) {
        (void)mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig());
        CHECK(readLine(box.governor_file) != "schedutil");
    }

    // Advance past the cooldown with idle cycles (each restores factory state).
    for (std::uint64_t i = 0; i <= MutationController::kRejectionCooldownCycles; ++i) {
        (void)mc.apply(RuntimeState::Idle, sample(0.0, 30.0), profile, adaptiveConfig());
    }
    CHECK(readLine(box.governor_file) == "powersave");

    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "schedutil");
}

void test_refresh_cannot_rebaseline_a_mutated_system() {
    // Regression test for baseline poisoning: while the restore is failing,
    // capturing a new baseline must be refused, not adopt the mutated value.
    Sandbox box;
    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();

    CHECK(mc.captureBaseline(profile));
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);

    // Make the governor unreadable so the restore fails.
    fs::remove(box.governor_file);
    fs::create_directories(box.governor_file);
    CHECK(!mc.restoreAll());
    CHECK(mc.mutated());
    CHECK(!mc.captureBaseline(profile));

    // Restore the file and confirm recovery to the true factory value.
    fs::remove_all(box.governor_file);
    writeLine(box.governor_file, "schedutil");
    CHECK(mc.restoreAll());
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(mc.captureBaseline(profile));
}

void test_crash_recovery_restores_factory_before_capture() {
    // Simulates a daemon killed mid-mutation: the journal holds the factory
    // value and discovery reads the mutated one. A new controller must restore
    // the factory value and must not adopt the mutated value as baseline.
    Sandbox box;
    FakeJournal journal;
    {
        MutationController crashed;
        crashed.setJournal(&journal);
        const DeviceProfile profile = box.profile();
        CHECK(crashed.captureBaseline(profile));
        CHECK(crashed.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);
        // Process dies here: no restore, journal still pending.
    }
    CHECK(journal.has_pending);
    CHECK(readLine(box.governor_file) == "schedutil");

    MutationController restarted;
    restarted.setJournal(&journal);
    const DeviceProfile rediscovered = box.profile("schedutil");  // discovery reads live state
    CHECK(restarted.captureBaseline(rediscovered));
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(!journal.has_pending);

    CHECK(restarted.apply(RuntimeState::Idle, sample(0.0, 30.0), rediscovered, adaptiveConfig()) != MutationResult::Failed);
    CHECK(restarted.restoreAll());
    CHECK(readLine(box.governor_file) == "powersave");
}

void test_failed_journal_commit_prevents_any_write() {
    Sandbox box;
    FakeJournal journal;
    journal.fail_commit = true;
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();

    CHECK(mc.captureBaseline(profile));
    const MutationResult r = mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig());
    CHECK(r == MutationResult::Failed);
    CHECK(readLine(box.governor_file) == "powersave");
}

void test_corrupt_journal_blocks_mutation() {
    Sandbox box;
    writeLine(box.journal_file, "garbage without header");
    FileMutationJournal journal(box.journal_file);
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();

    CHECK(!mc.captureBaseline(profile));
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Failed);
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(fs::exists(box.journal_file));
}

void test_journal_with_unavailable_value_fails_closed() {
    // A stale journal value the policy no longer advertises must never be
    // written; recovery reports failure and mutation stays blocked.
    Sandbox box;
    FileMutationJournal journal(box.journal_file);
    CHECK(journal.commit({{box.governor_file, "turbo"}}));
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile("schedutil");

    CHECK(!mc.captureBaseline(profile));
    CHECK(readLine(box.governor_file) == "schedutil");
    CHECK(fs::exists(box.journal_file));
}

void test_file_journal_roundtrip_and_truncation_detection() {
    Sandbox box;
    FileMutationJournal journal(box.journal_file);
    const MutationJournal::Entries values = {{"/sys/a/scaling_governor", "powersave"},
                                             {"/sys/b/scaling_governor", "conservative"}};
    CHECK(journal.commit(values));

    MutationJournal::Entries loaded;
    CHECK(journal.load(loaded) == MutationJournal::LoadState::Pending);
    CHECK(loaded == values);

    // Truncate the file: the missing "end" marker must be detected.
    std::ofstream(box.journal_file, std::ios::trunc)
        << "coreflow-mutation-journal v1\n/sys/a/scaling_governor\tpowersave\n";
    CHECK(journal.load(loaded) == MutationJournal::LoadState::Corrupt);

    CHECK(journal.clear());
    CHECK(journal.load(loaded) == MutationJournal::LoadState::Absent);
}


void test_disabled_mode_is_a_hard_no_mutation_gate() {
    Sandbox box;
    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();

    CHECK(mc.captureBaseline(profile));
    EngineConfig cfg;
    cfg.setMutationMode(MutationMode::Disabled);
    cfg.setAllowCpuGovernor(true);

    const MutationResult r = mc.apply(
        RuntimeState::Normal, sample(0.5, 30.0), profile, cfg);
    CHECK(r != MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(journal.commits == 0);
    CHECK(!mc.mutated());
}

void test_mutation_requires_durable_journal() {
    Sandbox box;
    MutationController mc;
    const DeviceProfile profile = box.profile();

    CHECK(mc.captureBaseline(profile));
    const MutationResult r = mc.apply(
        RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig());
    CHECK(r == MutationResult::Failed);
    CHECK(readLine(box.governor_file) == "powersave");
}

void test_empty_governor_advertisement_fails_closed() {
    Sandbox box;
    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    DeviceProfile profile = box.profile();
    profile.cpu_policies.front().available_governors.clear();

    CHECK(!mc.captureBaseline(profile));
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig())
          != MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "powersave");
}

void test_low_confidence_restores_instead_of_mutating() {
    Sandbox box;
    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();
    CHECK(mc.captureBaseline(profile));

    RuntimeSample low = sample(0.5, 30.0);
    low.confidence = 0.10;
    CHECK(mc.apply(RuntimeState::Normal, low, profile, adaptiveConfig()) != MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(journal.commits == 0);
}

void test_mutation_requires_explicit_arm() {
    Sandbox box;
    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();
    CHECK(mc.captureBaseline(profile));

    EngineConfig cfg;
    cfg.setMutationMode(MutationMode::Adaptive);
    cfg.setAllowCpuGovernor(true);
    cfg.setMutationArmed(false);
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, cfg) != MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(journal.commits == 0);
}

void test_thermal_thresholds_are_consistent() {
    // The guard exit threshold must be the shared constant, not a second value.
    AdaptivePolicy policy;
    RuntimeSample inside;
    inside.thermal_available = true;
    inside.thermal_millidegrees = 41'600;  // above exit (41.5C), below enter (43C)
    inside.mem_total_kb = 1000;
    inside.mem_available_ratio = 0.5;
    inside.cpu_utilization_available = true;
    inside.cpu_utilization = 0.1;
    inside.load1 = 0.1;
    CHECK(policy.evaluate(inside, RuntimeState::ThermalGuard) == RuntimeState::ThermalGuard);

    RuntimeSample outside = inside;
    outside.thermal_millidegrees = static_cast<long>(kThermalGuardExitC * 1000.0) - 100;
    CHECK(policy.evaluate(outside, RuntimeState::ThermalGuard) != RuntimeState::ThermalGuard);

    RuntimeSample hot = inside;
    hot.thermal_millidegrees = static_cast<long>(kThermalGuardEnterC * 1000.0);
    CHECK(policy.evaluate(hot, RuntimeState::Normal) == RuntimeState::ThermalGuard);
}

void test_production_defaults_are_autonomous_but_bounded_by_explicit_gates() {
    EngineConfig cfg;
    CHECK(cfg.mutationMode() == MutationMode::Adaptive);
    CHECK(cfg.mutationArmed());
    CHECK(cfg.allowCpuGovernor());

    cfg.setMutationMode(MutationMode::Disabled);
    CHECK(cfg.mutationMode() == MutationMode::Disabled);
}

void test_experimental_trial_mode_is_not_enabled_by_config() {
    const std::string path = "/tmp/coreflow_test_config_trial.ini";
    writeLine(path, "mutation_mode=trial");
    EngineConfig cfg;
    CHECK(cfg.load(path));
    CHECK(cfg.mutationMode() == MutationMode::Disabled);

    writeLine(path, "mutation_mode=adaptive");
    CHECK(cfg.load(path));
    CHECK(cfg.mutationMode() == MutationMode::Adaptive);
    CHECK(!cfg.mutationArmed());
    std::remove(path.c_str());
}


// --- Tier-2 safety ----------------------------------------------------------

void test_observe_mode_never_writes_to_kernel_when_not_mutated() {
    // Regression for the observe-mode fight: a governor changed by the platform
    // must not be "corrected" by a controller that never mutated anything.
    Sandbox box;
    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();
    CHECK(mc.captureBaseline(profile));

    writeLine(box.governor_file, "performance");  // platform change
    EngineConfig observe;
    observe.setMutationMode(MutationMode::Disabled);
    observe.setMutationArmed(false);
    observe.setAllowCpuGovernor(false);
    const MutationResult r = mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, observe);
    CHECK(r == MutationResult::Skipped);
    CHECK(readLine(box.governor_file) == "performance");
    CHECK(journal.commits == 0);

    CHECK(mc.restoreAll());
    CHECK(readLine(box.governor_file) == "performance");
}

void test_unarmed_adaptive_mode_never_writes() {
    Sandbox box;
    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();
    CHECK(mc.captureBaseline(profile));

    EngineConfig cfg;
    cfg.setMutationMode(MutationMode::Adaptive);  // adaptive but NOT armed
    cfg.setMutationArmed(false);
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, cfg) == MutationResult::Skipped);
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(journal.commits == 0);
}

void test_disabling_mode_restores_only_own_mutation() {
    Sandbox box;
    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();
    CHECK(mc.captureBaseline(profile));

    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "schedutil");

    EngineConfig held;  // operator / guard disables mutation
    held.setMutationMode(MutationMode::Disabled);
    held.setMutationArmed(false);
    held.setAllowCpuGovernor(false);
    const MutationResult r = mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, held);
    CHECK(r == MutationResult::RolledBack);
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(!mc.mutated());
}

void test_max_hold_forces_restore_then_cooldown() {
    Sandbox box;
    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    const DeviceProfile profile = box.profile();
    CHECK(mc.captureBaseline(profile));

    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);
    // Stay mutated (schedutil is already the best choice) until the hold limit.
    for (std::uint64_t i = 1; i < MutationController::kMaxMutationHoldCycles; ++i) {
        (void)mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig());
    }
    CHECK(readLine(box.governor_file) == "schedutil");
    CHECK(mc.mutated());

    // The next cycle reaches the hold limit and must restore the factory value.
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::RolledBack);
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(!mc.mutated());

    // Cooldown: no new mutation until it elapses.
    for (std::uint64_t i = 0; i < MutationController::kHoldCooldownCycles - 1; ++i) {
        (void)mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig());
        CHECK(readLine(box.governor_file) == "powersave");
    }
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "schedutil");
}

void test_safety_hold_markers() {
    char tmpl[] = "/tmp/coreflow_hold_XXXXXX";
    const std::string dir = mkdtemp(tmpl);
    const SafetyHoldPaths paths{dir + "/DISABLE", dir + "/SAFE_MODE"};

    CHECK(evaluateSafetyHold(paths) == HoldReason::None);

    writeLine(paths.safe_mode, "x");
    CHECK(evaluateSafetyHold(paths) == HoldReason::SafeMode);

    writeLine(paths.kill_switch, "x");
    CHECK(evaluateSafetyHold(paths) == HoldReason::KillSwitch);  // kill switch wins

    std::remove(paths.kill_switch.c_str());
    std::remove(paths.safe_mode.c_str());
    std::remove(dir.c_str());
}

void test_environment_capability_matrix_is_fail_closed() {
    EnvironmentCapabilityMatrix matrix;
    ResourceCapability uclamp;
    uclamp.domain = ResourceDomain::UClamp;
    uclamp.name = "cpu.uclamp.min";
    uclamp.exists = true;
    uclamp.readable = true;
    uclamp.writable = true;
    uclamp.mutation_ready = false;
    matrix.resources.push_back(uclamp);

    CHECK(matrix.count(ResourceDomain::UClamp) == 1);
    CHECK(matrix.has(ResourceDomain::UClamp, "cpu.uclamp.min"));
    CHECK(matrix.mutationReadyCount() == 0);
}

void test_resource_domain_names_are_stable() {
    CHECK(std::string(resourceDomainName(ResourceDomain::CpuFreq)) == "CPUFREQ");
    CHECK(std::string(resourceDomainName(ResourceDomain::UClamp)) == "UCLAMP");
    CHECK(std::string(resourceDomainName(ResourceDomain::CpuSet)) == "CPUSET");
    CHECK(std::string(resourceDomainName(ResourceDomain::Scheduler)) == "SCHEDULER");
    CHECK(std::string(resourceDomainName(ResourceDomain::Memory)) == "MEMORY");
    CHECK(std::string(resourceDomainName(ResourceDomain::Io)) == "IO");
    CHECK(std::string(resourceDomainName(ResourceDomain::Gpu)) == "GPU");
    CHECK(std::string(resourceDomainName(ResourceDomain::Thermal)) == "THERMAL");
    CHECK(std::string(resourceDomainName(ResourceDomain::Charging)) == "CHARGING");
    CHECK(std::string(resourceDomainName(ResourceDomain::Power)) == "POWER");
    CHECK(std::string(resourceDomainName(ResourceDomain::CGroup)) == "CGROUP");
    CHECK(std::string(resourceDomainName(ResourceDomain::AndroidRuntime)) == "ANDROID_RUNTIME");
}


void test_restore_only_owned_policies() {
    // An external actor may legitimately change an untouched policy while
    // CoreFlow owns another one. Restore must never overwrite that change.
    Sandbox box;
    const std::string external = (box.root / "external_policy").string();
    fs::create_directories(external);
    writeLine(external + "/scaling_governor", "powersave");

    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    DeviceProfile profile = box.profile();
    CpuPolicy second;
    second.id = 1;
    second.path = external;
    second.governor = "powersave";
    second.available_governors = {"powersave", "schedutil", "performance"};
    second.readable = true;
    second.governor_writable = true;
    profile.cpu_policies.push_back(second);

    CHECK(mc.captureBaseline(profile));
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);
    writeLine(external + "/scaling_governor", "performance");

    CHECK(mc.restoreAll());
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(readLine(external + "/scaling_governor") == "performance");
    CHECK(!journal.has_pending);
}

void test_mutation_authority_is_single_gate() {
    EngineConfig config = adaptiveConfig();
    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.mutation_eligible = true;
    MutationAuthority authority;
    const MutationPermit normal = authority.authorize(
        plan, config, RuntimeState::Normal, 1.0, MutationPermit::Scope::CpuFreq);
    CHECK(normal.validFor(MutationPermit::Scope::CpuFreq));
    CHECK(!normal.validFor(MutationPermit::Scope::Resource));

    const MutationPermit hot = authority.authorize(
        plan, config, RuntimeState::ThermalGuard, 1.0, MutationPermit::Scope::CpuFreq);
    CHECK(!hot.valid());

    config.setMutationArmed(false);
    const MutationPermit unarmed = authority.authorize(
        plan, config, RuntimeState::Normal, 1.0, MutationPermit::Scope::CpuFreq);
    CHECK(!unarmed.valid());
}

void test_multi_policy_mutations_all_restored() {
    // Two policies changed in separate cycles must both return to factory, and
    // the journal must clear only after both are restored.
    Sandbox box;
    const std::string policy1 = (box.root / "policy1").string();
    fs::create_directories(policy1);
    writeLine(policy1 + "/scaling_governor", "powersave");

    FakeJournal journal;
    MutationController mc;
    mc.setJournal(&journal);
    DeviceProfile profile = box.profile();
    CpuPolicy second;
    second.id = 1;
    second.path = policy1;
    second.governor = "powersave";
    second.available_governors = {"powersave", "conservative", "schedutil", "performance"};
    second.readable = true;
    second.governor_writable = true;
    profile.cpu_policies.push_back(second);

    CHECK(mc.captureBaseline(profile));
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "schedutil");
    CHECK(readLine(policy1 + "/scaling_governor") == "schedutil");

    CHECK(mc.restoreAll());
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(readLine(policy1 + "/scaling_governor") == "powersave");
    CHECK(!mc.mutated());
    CHECK(journal.clears >= 1);
    CHECK(!journal.has_pending);
}


void test_experience_persists_and_is_bounded() {
    const fs::path path = fs::temp_directory_path() / "coreflow_experience_test.db";
    fs::remove(path);

    ExperienceMemory memory;
    memory.setScope("test-device:kernel");
    ExperienceMemory::Record record;
    record.candidate.key = "cpufreq:/policy0:scaling_governor:schedutil";
    record.context.state = RuntimeState::Normal;
    record.context.workload = WorkloadClass::CpuBound;
    record.context.charging = false;
    record.context.thermal_trend = Trend::Stable;
    record.context.memory_trend = Trend::Stable;
    record.context.load_trend = Trend::Rising;
    record.outcome = BaselineIntelligence::Outcome::Beneficial;
    record.score = 0.82;
    record.confidence = 0.91;
    record.observations = 5;
    CHECK(memory.record(record));
    CHECK(memory.flush(path.string()));

    ExperienceMemory restored;
    restored.setScope("test-device:kernel");
    CHECK(restored.load(path.string()));
    const auto* found = restored.find(record.candidate, record.context);
    CHECK(found != nullptr);
    CHECK(found != nullptr && found->outcome == BaselineIntelligence::Outcome::Beneficial);
    CHECK(found != nullptr && found->score > 0.80);

    // A different device/kernel scope must not inherit another environment's experience.
    ExperienceMemory isolated;
    isolated.setScope("different-device:kernel");
    CHECK(isolated.load(path.string()));
    CHECK(isolated.find(record.candidate, record.context) == nullptr);
    fs::remove(path);
}

} // namespace

int main() {
    struct Case {
        const char* name;
        std::function<void()> fn;
    };
    const std::vector<Case> cases = {
        {"restore_returns_to_factory_and_clears_journal", test_restore_returns_to_factory_and_clears_journal},
        {"live_governor_drives_decisions", test_live_governor_drives_decisions_not_stale_snapshot},
        {"rejected_candidate_cooldown", test_rejected_candidate_is_not_reselected_during_cooldown},
        {"refresh_cannot_rebaseline_mutated", test_refresh_cannot_rebaseline_a_mutated_system},
        {"crash_recovery_before_capture", test_crash_recovery_restores_factory_before_capture},
        {"failed_commit_prevents_write", test_failed_journal_commit_prevents_any_write},
        {"corrupt_journal_blocks_mutation", test_corrupt_journal_blocks_mutation},
        {"unavailable_journal_value_fails_closed", test_journal_with_unavailable_value_fails_closed},
        {"journal_roundtrip_and_truncation", test_file_journal_roundtrip_and_truncation_detection},
        {"disabled_mode_hard_gate", test_disabled_mode_is_a_hard_no_mutation_gate},
        {"production_defaults_autonomous", test_production_defaults_are_autonomous_but_bounded_by_explicit_gates},
        {"mutation_requires_durable_journal", test_mutation_requires_durable_journal},
        {"empty_governor_advertisement_fails_closed", test_empty_governor_advertisement_fails_closed},
        {"low_confidence_restores", test_low_confidence_restores_instead_of_mutating},
        {"mutation_requires_explicit_arm", test_mutation_requires_explicit_arm},
        {"thermal_thresholds_consistent", test_thermal_thresholds_are_consistent},
        {"trial_mode_removed_from_config", test_experimental_trial_mode_is_not_enabled_by_config},
        {"observe_mode_no_kernel_write", test_observe_mode_never_writes_to_kernel_when_not_mutated},
        {"unarmed_adaptive_no_write", test_unarmed_adaptive_mode_never_writes},
        {"disabling_restores_only_own", test_disabling_mode_restores_only_own_mutation},
        {"max_hold_restore_and_cooldown", test_max_hold_forces_restore_then_cooldown},
        {"safety_hold_markers", test_safety_hold_markers},
        {"multi_policy_all_restored", test_multi_policy_mutations_all_restored},
        {"restore_only_owned_policies", test_restore_only_owned_policies},
        {"mutation_authority_single_gate", test_mutation_authority_is_single_gate},
        {"environment_capability_matrix_fail_closed", test_environment_capability_matrix_is_fail_closed},
        {"resource_domain_names_stable", test_resource_domain_names_are_stable},
        {"experience_persistence_and_scope", test_experience_persists_and_is_bounded},
    };

    for (const Case& c : cases) {
        const int before = g_failures;
        c.fn();
        std::printf("[%s] %s\n", g_failures == before ? " OK " : "FAIL", c.name);
    }

    std::printf("%d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
