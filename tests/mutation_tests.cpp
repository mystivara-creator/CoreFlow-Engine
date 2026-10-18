#include "coreflow/cpufreq_actuator.hpp"
#include "coreflow/control.hpp"
#include "coreflow/actuator.hpp"
// Deterministic host tests for the CoreFlow mutation-safety invariants.
// The CPUFreq sysfs tree is simulated with a temporary directory, so these
// tests never touch the real kernel interface.

#include "coreflow/config.hpp"
#include "coreflow/experience.hpp"
#include "coreflow/mutation.hpp"
#include "coreflow/mutation_journal.hpp"
#include "coreflow/retained_state.hpp"
#include "coreflow/policy.hpp"
#include "coreflow/safety_hold.hpp"
#include "coreflow/types.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <iterator>
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


fs::path createTempDirectory(const std::string& prefix) {
    std::vector<fs::path> candidates;
    const char* configured_tmp = std::getenv("TMPDIR");
    if (configured_tmp != nullptr && configured_tmp[0] != '\0') {
        candidates.emplace_back(configured_tmp);
    }
    candidates.emplace_back("/data/data/com.termux/files/usr/tmp");

    std::error_code ec;
    const fs::path system_tmp = fs::temp_directory_path(ec);
    if (!ec) candidates.push_back(system_tmp);

    ec.clear();
    const fs::path working_dir = fs::current_path(ec);
    if (!ec) candidates.push_back(working_dir);

    std::string last_pattern;
    for (const auto& base : candidates) {
        ec.clear();
        if (!fs::is_directory(base, ec) || ec) continue;

        const std::string pattern =
            (base / (prefix + "XXXXXX")).string();
        last_pattern = pattern;
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');

        char* made = mkdtemp(buffer.data());
        if (made != nullptr) return fs::path(made);
    }

    std::fprintf(stderr, "Cannot create temp directory: %s\n",
                 last_pattern.c_str());
    return {};
}

struct Sandbox {
    fs::path root;
    std::string policy;
    std::string governor_file;
    std::string journal_file;

    Sandbox() {
        const char* configured_tmp = std::getenv("TMPDIR");
        fs::path temp_dir;
        if (configured_tmp != nullptr && configured_tmp[0] != '\0' &&
            fs::is_directory(configured_tmp)) {
            temp_dir = configured_tmp;
        } else if (fs::is_directory("/data/data/com.termux/files/usr/tmp")) {
            temp_dir = "/data/data/com.termux/files/usr/tmp";
        } else {
            temp_dir = fs::temp_directory_path();
        }
        const std::string pattern =
            (temp_dir / "coreflow_test_XXXXXX").string();
        std::vector<char> tmpl(pattern.begin(), pattern.end());
        tmpl.push_back('\0');
        char* made = mkdtemp(tmpl.data());
        if (made == nullptr) {
            std::fprintf(stderr, "mkdtemp failed; temp dir: %s\n", pattern.c_str());
            std::abort();
        }
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

class FakeRetainedStateStore final : public RetainedStateStore {
public:
    bool fail_commit{false};
    bool corrupt{false};
    Entries stored;

    LoadState load(Entries& out) noexcept override {
        out = stored;
        if (corrupt) return LoadState::Corrupt;
        return stored.empty() ? LoadState::Absent : LoadState::Valid;
    }
    bool commit(const Entries& entries) noexcept override {
        if (fail_commit) return false;
        stored = entries;
        corrupt = false;
        return true;
    }
    bool clear() noexcept override {
        if (fail_commit) return false;
        stored.clear();
        corrupt = false;
        return true;
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
    FakeRetainedStateStore retained;
    MutationController mc;
    mc.setJournal(&journal);
    mc.setRetainedStateStore(&retained);
    const DeviceProfile profile = box.profile();

    CHECK(mc.captureBaseline(profile));
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);
    CHECK(mc.retainLastMutation(profile));
    CHECK(readLine(box.governor_file) == "schedutil");

    const MutationResult hot = mc.apply(RuntimeState::ThermalGuard, sample(0.0, 45.0), profile, adaptiveConfig());
    CHECK(hot == MutationResult::RolledBack);
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(!mc.mutated());
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

void test_recovery_clear_failure_keeps_journal_and_blocks_mutation() {
    Sandbox box;
    FakeJournal journal;

    {
        MutationController original;
        original.setJournal(&journal);
        const DeviceProfile profile = box.profile();

        CHECK(original.captureBaseline(profile));
        CHECK(original.apply(
            RuntimeState::Normal,
            sample(0.5, 30.0),
            profile,
            adaptiveConfig()) == MutationResult::Verified);

        CHECK(journal.has_pending);
        CHECK(readLine(box.governor_file) == "schedutil");
    }

    // Simulate a restart where recovery restores the factory value,
    // but journal cleanup fails.
    journal.fail_clear = true;

    MutationController restarted;
    restarted.setJournal(&journal);
    const DeviceProfile profile = box.profile("schedutil");

    CHECK(!restarted.captureBaseline(profile));
    CHECK(journal.has_pending);
    CHECK(readLine(box.governor_file) == "powersave");

    // A subsequent attempt must not treat the failed recovery as complete.
    CHECK(restarted.apply(
        RuntimeState::Normal,
        sample(0.5, 30.0),
        profile,
        adaptiveConfig()) == MutationResult::Failed);

    CHECK(journal.has_pending);
    CHECK(readLine(box.governor_file) == "powersave");
}

void test_recovery_missing_policy_keeps_journal() {
    Sandbox box;
    FakeJournal journal;

    {
        MutationController original;
        original.setJournal(&journal);

        const DeviceProfile profile = box.profile();
        CHECK(original.captureBaseline(profile));

        CHECK(original.apply(
            RuntimeState::Normal,
            sample(0.5, 30.0),
            profile,
            adaptiveConfig()) == MutationResult::Verified);

        CHECK(journal.has_pending);
    }

    // The journal references the original governor path, but discovery
    // currently provides no CPU policies.
    MutationController restarted;
    restarted.setJournal(&journal);

    DeviceProfile incomplete = box.profile("schedutil");
    incomplete.cpu_policies.clear();

    CHECK(!restarted.captureBaseline(incomplete));
    CHECK(journal.has_pending);
    CHECK(readLine(box.governor_file) == "schedutil");

    // Rediscovery makes the original policy available again.
    const DeviceProfile rediscovered = box.profile("schedutil");

    CHECK(restarted.captureBaseline(rediscovered));
    CHECK(!journal.has_pending);
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

    // A pre-created hard link at the historical fixed temp name must never be
    // opened with O_TRUNC or damage the unrelated inode.
    {
        const fs::path sentinel = box.root / "journal_temp_sentinel";
        const fs::path old_temp_name = box.journal_file + ".tmp";
        std::ofstream(sentinel, std::ios::binary | std::ios::trunc) << "keep-me";
        std::error_code link_error;
        fs::create_hard_link(sentinel, old_temp_name, link_error);
        if (!link_error) {
            CHECK(journal.commit(values));
            std::ifstream sentinel_input(sentinel, std::ios::binary);
            const std::string sentinel_contents((std::istreambuf_iterator<char>(sentinel_input)),
                                                std::istreambuf_iterator<char>());
            CHECK(sentinel_contents == "keep-me");
            std::error_code cleanup_error;
            fs::remove(old_temp_name, cleanup_error);
        }
    }

    // V2 includes a checksum: same-length payload corruption must fail closed.
    {
        std::ifstream input(box.journal_file, std::ios::binary);
        const std::string contents((std::istreambuf_iterator<char>(input)),
                                   std::istreambuf_iterator<char>());
        std::string corrupted = contents;
        const std::size_t value_offset = corrupted.find("powersave");
        CHECK(value_offset != std::string::npos);
        if (value_offset != std::string::npos) {
            corrupted.replace(value_offset, std::string("powersave").size(), "powerxxve");
            std::ofstream output(box.journal_file, std::ios::binary | std::ios::trunc);
            output.write(corrupted.data(), static_cast<std::streamsize>(corrupted.size()));
        }
    }
    CHECK(journal.load(loaded) == MutationJournal::LoadState::Corrupt);
    CHECK(loaded.empty());

    // A v1 journal may survive an upgrade. Continue to support it for recovery.
    std::ofstream(box.journal_file, std::ios::trunc)
        << "coreflow-mutation-journal v1\n/sys/a/scaling_governor\tpowersave\n"
        << "/sys/b/scaling_governor\tconservative\nend\n";
    CHECK(journal.load(loaded) == MutationJournal::LoadState::Pending);
    CHECK(loaded == values);

    // Truncate the legacy file: the missing "end" marker must be detected.
    std::ofstream(box.journal_file, std::ios::trunc)
        << "coreflow-mutation-journal v1\n/sys/a/scaling_governor\tpowersave\n";
    CHECK(journal.load(loaded) == MutationJournal::LoadState::Corrupt);

    // Embedded NUL bytes cannot enter a journal and become truncated C paths.
    CHECK(!journal.commit({{std::string("/sys/a\0evil", 11U), "powersave"}}));

    // A symlink or oversized journal is untrusted and must fail closed.
    const fs::path journal_target = box.root / "journal_target";
    std::ofstream(journal_target, std::ios::trunc)
        << "coreflow-mutation-journal v1\n/sys/a/scaling_governor\tpowersave\nend\n";
    CHECK(journal.clear());
    std::error_code symlink_error;
    fs::create_symlink(journal_target, box.journal_file, symlink_error);
    if (!symlink_error) {
        CHECK(journal.load(loaded) == MutationJournal::LoadState::Corrupt);
        CHECK(journal.clear());
    }

    {
        std::ofstream oversized(box.journal_file, std::ios::binary | std::ios::trunc);
        const std::string data(1024U * 1024U + 1U, 'x');
        oversized.write(data.data(), static_cast<std::streamsize>(data.size()));
    }
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
    // Midway between exit and enter (hysteresis band).
    inside.thermal_millidegrees = static_cast<long>(
        ((kThermalGuardExitC + kThermalGuardEnterC) / 2.0) * 1000.0);
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

void test_min_confidence_rejects_non_finite_and_malformed_values() {
    EngineConfig direct;
    direct.setMinConfidence(0.80);
    CHECK(direct.minConfidence() == 0.80);

    direct.setMinConfidence(std::numeric_limits<double>::quiet_NaN());
    CHECK(direct.minConfidence() == 0.60);

    direct.setMinConfidence(std::numeric_limits<double>::infinity());
    CHECK(direct.minConfidence() == 0.60);

    const fs::path config_dir = createTempDirectory("coreflow_confidence_");
    CHECK(!config_dir.empty());
    if (config_dir.empty()) return;

    const std::string path = (config_dir / "config.ini").string();
    EngineConfig loaded;
    writeLine(path, "min_confidence=0.8xyz");
    CHECK(loaded.load(path));
    CHECK(loaded.minConfidence() == 0.60);

    writeLine(path, "min_confidence=0.8");
    CHECK(loaded.load(path));
    CHECK(loaded.minConfidence() == 0.80);

    writeLine(path, "min_confidence=nan");
    CHECK(loaded.load(path));
    CHECK(loaded.minConfidence() == 0.80);

    writeLine(path, "min_confidence=inf");
    CHECK(loaded.load(path));
    CHECK(loaded.minConfidence() == 0.80);

    std::error_code cleanup_ec;
    fs::remove_all(config_dir, cleanup_ec);
}

void test_release_defaults_are_observe_only_until_explicit_opt_in() {
    EngineConfig cfg;
    CHECK(cfg.mutationMode() == MutationMode::Disabled);
    CHECK(!cfg.mutationArmed());
    CHECK(!cfg.allowCpuGovernor()); // default-deny: governor writes need allow_cpu_governor=yes

    // Adaptive mode still does not arm mutations; the global arm gate remains.
    cfg.setMutationMode(MutationMode::Adaptive);
    CHECK(cfg.mutationMode() == MutationMode::Adaptive);
    CHECK(!cfg.mutationArmed());
    CHECK(!cfg.allowCpuGovernor());
}

void test_experimental_trial_mode_is_not_enabled_by_config() {
    const fs::path config_dir = createTempDirectory("coreflow_config_");
    CHECK(!config_dir.empty());
    if (config_dir.empty()) return;
    const std::string path = (config_dir / "config_trial.ini").string();
    writeLine(path, "mutation_mode=trial");
    EngineConfig cfg;
    CHECK(cfg.load(path));
    CHECK(cfg.mutationMode() == MutationMode::Disabled);

    writeLine(path, "mutation_mode=adaptive");
    CHECK(cfg.load(path));
    CHECK(cfg.mutationMode() == MutationMode::Adaptive);
    CHECK(!cfg.mutationArmed());
    std::error_code cleanup_ec;
    fs::remove_all(config_dir, cleanup_ec);
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

void test_beneficial_mutation_is_retained_without_time_expiry() {
    Sandbox box;
    FakeJournal journal;
    FakeRetainedStateStore retained;
    MutationController mc;
    mc.setJournal(&journal);
    mc.setRetainedStateStore(&retained);
    const DeviceProfile profile = box.profile();
    CHECK(mc.captureBaseline(profile));

    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile,
                   adaptiveConfig()) == MutationResult::Verified);
    CHECK(mc.hasPendingOutcome());
    CHECK(mc.retainLastMutation(profile));
    CHECK(!mc.hasPendingOutcome());
    CHECK(retained.stored.size() == 1U);
    CHECK(retained.stored.front().baseline == "powersave");
    CHECK(retained.stored.front().target == "schedutil");
    CHECK(mc.activeGovernorSettings().size() == 1U);

    // The elapsed time alone must never expire an accepted optimization.
    for (std::uint64_t i = 0; i < 900U; ++i) {
        (void)mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile,
                       adaptiveConfig());
    }
    CHECK(readLine(box.governor_file) == "schedutil");
    CHECK(mc.mutated());
    CHECK(retained.stored.size() == 1U);

    // A deliberate disarm still restores the original value.
    EngineConfig held;
    held.setMutationMode(MutationMode::Disabled);
    held.setMutationArmed(false);
    held.setAllowCpuGovernor(false);
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, held) ==
          MutationResult::RolledBack);
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(!mc.mutated());
    CHECK(mc.activeGovernorSettings().empty());
}

void test_rollback_only_latest_candidate_and_preserve_retained_value() {
    Sandbox box;
    FakeJournal journal;
    FakeRetainedStateStore retained;
    MutationController mc;
    mc.setJournal(&journal);
    mc.setRetainedStateStore(&retained);
    const std::string second_policy = (box.root / "policy1").string();
    fs::create_directories(second_policy);
    writeLine(second_policy + "/scaling_governor", "powersave");
    DeviceProfile profile = box.profile();
    CpuPolicy second;
    second.id = 1;
    second.path = second_policy;
    second.governor = "powersave";
    second.available_governors = {"powersave", "conservative", "schedutil", "performance"};
    second.readable = true;
    second.governor_writable = true;
    profile.cpu_policies.push_back(second);
    CHECK(mc.captureBaseline(profile));

    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile,
                   adaptiveConfig()) == MutationResult::Verified);
    CHECK(mc.retainLastMutation(profile));
    CHECK(readLine(box.governor_file) == "schedutil");

    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile,
                   adaptiveConfig()) == MutationResult::Verified);
    CHECK(readLine(second_policy + "/scaling_governor") == "schedutil");
    CHECK(mc.hasPendingOutcome());
    CHECK(mc.rollbackLastMutation());
    CHECK(readLine(second_policy + "/scaling_governor") == "powersave");
    CHECK(readLine(box.governor_file) == "schedutil");
    CHECK(mc.mutated());
    CHECK(!mc.hasPendingOutcome());
    CHECK(retained.stored.size() == 1U);
    CHECK(retained.stored.front().target == "schedutil");
}

void test_external_override_is_preserved_during_candidate_rollback() {
    Sandbox box;
    FakeJournal journal;
    FakeRetainedStateStore retained;
    MutationController mc;
    mc.setJournal(&journal);
    mc.setRetainedStateStore(&retained);
    const DeviceProfile profile = box.profile();
    CHECK(mc.captureBaseline(profile));

    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile,
                   adaptiveConfig()) == MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "schedutil");
    // Emulate another actor changing the governor while CoreFlow evaluates.
    writeLine(box.governor_file, "conservative");
    CHECK(mc.rollbackLastMutation());
    CHECK(readLine(box.governor_file) == "conservative");
    CHECK(!mc.mutated());
    CHECK(!journal.has_pending);
    CHECK(retained.stored.empty());
}

void test_corrupt_retained_state_fails_closed() {
    Sandbox box;
    FakeJournal journal;
    FakeRetainedStateStore retained;
    retained.corrupt = true;
    MutationController mc;
    mc.setJournal(&journal);
    mc.setRetainedStateStore(&retained);
    const DeviceProfile profile = box.profile();

    CHECK(!mc.captureBaseline(profile));
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile,
                   adaptiveConfig()) != MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(!journal.has_pending);
}

void test_retained_cpu_state_restores_saved_baseline_after_restart() {
    Sandbox box;
    FakeJournal first_journal;
    FakeRetainedStateStore retained;
    MutationController first;
    first.setJournal(&first_journal);
    first.setRetainedStateStore(&retained);
    const DeviceProfile first_profile = box.profile();
    CHECK(first.captureBaseline(first_profile));
    CHECK(first.apply(RuntimeState::Normal, sample(0.5, 30.0), first_profile,
                      adaptiveConfig()) == MutationResult::Verified);
    CHECK(first.retainLastMutation(first_profile));
    CHECK(readLine(box.governor_file) == "schedutil");

    // Model a daemon restart while the accepted setting is still live.
    FakeJournal restarted_journal;
    MutationController restarted;
    restarted.setJournal(&restarted_journal);
    restarted.setRetainedStateStore(&retained);
    const DeviceProfile rediscovered = box.profile("schedutil");
    CHECK(restarted.captureBaseline(rediscovered));
    CHECK(restarted.mutated());
    CHECK(restarted.restoreAll());
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(retained.stored.size() == 1U);
    CHECK(retained.stored.front().baseline == "powersave");
    CHECK(retained.stored.front().target == "schedutil");
}

void test_retained_state_store_round_trip_and_corruption() {
    const fs::path root = createTempDirectory("coreflow_retained_");
    CHECK(!root.empty());
    if (root.empty()) return;
    const fs::path storePath = root / "retained.db";
    FileRetainedStateStore store(storePath.string());
    RetainedStateEntry entry;
    entry.scope = "vendor|device|soc|android|kernel|arm64";
    entry.path = "/sys/devices/system/cpu/cpufreq/policy0/scaling_governor";
    entry.baseline = "powersave";
    entry.target = "schedutil";
    CHECK(store.upsert(entry));

    FileRetainedStateStore restarted(storePath.string());
    RetainedStateStore::Entries loaded;
    CHECK(restarted.load(loaded) == RetainedStateStore::LoadState::Valid);
    CHECK(loaded.size() == 1U);
    if (!loaded.empty()) {
        CHECK(loaded.front().scope == entry.scope);
        CHECK(loaded.front().baseline == "powersave");
        CHECK(loaded.front().target == "schedutil");
    }

    std::ifstream originalFile(storePath);
    std::string contents((std::istreambuf_iterator<char>(originalFile)),
                         std::istreambuf_iterator<char>());
    const std::size_t baselinePos = contents.find("powersave");
    CHECK(baselinePos != std::string::npos);
    if (baselinePos != std::string::npos) contents.replace(baselinePos, 9U, "performance");
    std::ofstream corruptedFile(storePath, std::ios::trunc);
    corruptedFile << contents;
    corruptedFile.close();
    CHECK(restarted.load(loaded) == RetainedStateStore::LoadState::Corrupt);
    std::error_code ec;
    fs::remove_all(root, ec);
}

void test_safety_hold_markers() {
    const fs::path root = createTempDirectory("coreflow_hold_");
    CHECK(!root.empty());
    if (root.empty()) return;
    const std::string dir = root.string();
    const SafetyHoldPaths paths{dir + "/DISABLE", dir + "/SAFE_MODE"};

    CHECK(evaluateSafetyHold(paths) == HoldReason::None);

    writeLine(paths.safe_mode, "x");
    CHECK(evaluateSafetyHold(paths) == HoldReason::SafeMode);

    writeLine(paths.kill_switch, "x");
    CHECK(evaluateSafetyHold(paths) == HoldReason::KillSwitch);  // kill switch wins

    std::remove(paths.kill_switch.c_str());
    std::remove(paths.safe_mode.c_str());
    std::error_code cleanup_ec;
    fs::remove_all(root, cleanup_ec);
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
    FakeRetainedStateStore retained;
    MutationController mc;
    mc.setJournal(&journal);
    mc.setRetainedStateStore(&retained);
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
    CHECK(mc.retainLastMutation(profile));
    CHECK(mc.apply(RuntimeState::Normal, sample(0.5, 30.0), profile, adaptiveConfig()) == MutationResult::Verified);
    CHECK(mc.retainLastMutation(profile));
    CHECK(readLine(box.governor_file) == "schedutil");
    CHECK(readLine(policy1 + "/scaling_governor") == "schedutil");

    CHECK(mc.restoreAll());
    CHECK(readLine(box.governor_file) == "powersave");
    CHECK(readLine(policy1 + "/scaling_governor") == "powersave");
    CHECK(!mc.mutated());
    CHECK(journal.clears >= 1);
    CHECK(!journal.has_pending);
}



void test_experience_score_ranks_beneficial_governor() {
    Sandbox box;
    FakeJournal journal;
    ExperienceMemory memory;
    MutationController mc;
    mc.setJournal(&journal);
    mc.setExperienceMemory(&memory);

    const DeviceProfile profile = box.profile();
    const RuntimeSample runtime = sample(0.5, 30.0);

    ExperienceMemory::Record record;
    record.candidate.key = "cpufreq:" + box.policy + ":performance";
    record.context.state = RuntimeState::Normal;
    record.context.workload = WorkloadClass::Interactive;
    record.context.charging = false;
    record.outcome = BaselineIntelligence::Outcome::Beneficial;
    record.score = 0.95;
    record.confidence = 0.95;
    record.observations = 8;
    CHECK(memory.record(record));

    CHECK(mc.captureBaseline(profile));
    CHECK(mc.apply(RuntimeState::Normal, runtime, profile,
                   adaptiveConfig()) == MutationResult::Verified);
    CHECK(readLine(box.governor_file) == "performance");
    CHECK(mc.restoreAll());
    CHECK(readLine(box.governor_file) == "powersave");
}

void test_regressed_governor_is_not_bootstrapped() {
    Sandbox box;
    FakeJournal journal;
    ExperienceMemory memory;
    MutationController mc;
    mc.setJournal(&journal);
    mc.setExperienceMemory(&memory);

    const DeviceProfile profile = box.profile();
    const RuntimeSample runtime = sample(0.5, 30.0);

    ExperienceMemory::Record record;
    record.candidate.key = "cpufreq:" + box.policy + ":schedutil";
    record.context.state = RuntimeState::Normal;
    record.context.workload = WorkloadClass::Interactive;
    record.context.charging = false;
    record.outcome = BaselineIntelligence::Outcome::Regression;
    record.score = 0.05;
    record.confidence = 0.95;
    record.observations = 8;
    CHECK(memory.record(record));

    CHECK(mc.captureBaseline(profile));
    CHECK(mc.apply(RuntimeState::Normal, runtime, profile,
                   adaptiveConfig()) == MutationResult::Verified);
    CHECK(readLine(box.governor_file) != "schedutil");
    CHECK(mc.restoreAll());
    CHECK(readLine(box.governor_file) == "powersave");
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


void test_capability_bound_permit_rejects_scope_and_mismatches() {
    // Scope-only permit must never authorize an actuator write.
    MutationAuthority authority;
    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.mutation_eligible = true;
    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    config.setAllowCpuGovernor(true);
    config.setMinConfidence(0.5);

    const MutationPermit scope_only = authority.authorize(
        plan, config, RuntimeState::Normal, 1.0, MutationPermit::Scope::CpuFreq);
    CHECK(scope_only.valid());
    CHECK(scope_only.validFor(MutationPermit::Scope::CpuFreq));
    CHECK(!scope_only.isCapabilityBound());

    ActuatorMutation mut;
    mut.id = {ActuatorDomain::CpuFreq, 0};
    mut.target = "/sys/devices/system/cpu/cpufreq/policy0/scaling_governor";
    mut.requested = "schedutil";
    CHECK(!scope_only.validFor(mut));

    // Capability-bound permit for exact target succeeds.
    ActuatorCapability cap;
    cap.id = mut.id;
    cap.mode = ActuatorMode::Autonomous;
    cap.readable = true;
    cap.writable = true;
    cap.verifiable = true;
    cap.rollbackable = true;
    cap.max_writes_per_cycle = 1;

    const MutationPermit bound = authority.authorize(
        plan, config, RuntimeState::Normal, 1.0, cap, mut.target);
    CHECK(bound.valid());
    CHECK(bound.isCapabilityBound());
    CHECK(bound.validFor(mut));

    // Wrong actuator id rejected.
    ActuatorMutation wrong_id = mut;
    wrong_id.id.instance = 99;
    CHECK(!bound.validFor(wrong_id));

    // Wrong target rejected.
    ActuatorMutation wrong_target = mut;
    wrong_target.target = "/sys/devices/system/cpu/cpufreq/policy1/scaling_governor";
    CHECK(!bound.validFor(wrong_target));

    // Empty / unsafe capability yields pure invalid.
    ActuatorCapability unsafe;
    unsafe.id = mut.id;
    const MutationPermit bad_cap = authority.authorize(
        plan, config, RuntimeState::Normal, 1.0, unsafe, mut.target);
    CHECK(!bad_cap.valid());
    CHECK(!bad_cap.isCapabilityBound());
    CHECK(!bad_cap.validFor(mut));
}

void test_cpufreq_actuator_rejects_scope_only_permit() {
    // Host fixture: temporary governor file.
    const fs::path root = fs::temp_directory_path() / "coreflow_permit_test";
    fs::create_directories(root / "policy0");
    const std::string gov = (root / "policy0" / "scaling_governor").string();
    writeLine(gov, "powersave");

    DeviceProfile profile;
    CpuPolicy policy;
    policy.path = (root / "policy0").string();
    policy.readable = true;
    policy.governor_writable = true;
    policy.governor = "powersave";
    policy.available_governors = {"powersave", "schedutil", "performance"};
    profile.cpu_policies.push_back(policy);

    CpuFreqActuator actuator;
    actuator.setProfile(&profile);
    CHECK(actuator.discover() == ActuatorStatus::Ok);
    CHECK(actuator.capability().mutationSafe());

    ActuatorMutation mutation;
    mutation.id = actuator.id();
    mutation.target = gov;
    mutation.requested = "schedutil";

    MutationAuthority authority;
    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.mutation_eligible = true;
    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    config.setAllowCpuGovernor(true);
    config.setMinConfidence(0.5);

    // Scope-only → SafetyRejected, no write.
    const MutationPermit scope = authority.authorize(
        plan, config, RuntimeState::Normal, 1.0, MutationPermit::Scope::CpuFreq);
    const ActuatorResult rejected = actuator.apply(mutation, scope);
    CHECK(rejected.status == ActuatorStatus::SafetyRejected);
    CHECK(readLine(gov) == "powersave");

    // Capability-bound exact match → may write (or NoChange if already set).
    const MutationPermit bound = authority.authorize(
        plan, config, RuntimeState::Normal, 1.0, actuator.capability(), gov);
    const ActuatorResult ok = actuator.apply(mutation, bound);
    CHECK(ok.status == ActuatorStatus::Ok || ok.status == ActuatorStatus::NoChange);
    if (ok.changed()) {
        CHECK(readLine(gov) == "schedutil");
        // Restore to leave fixture clean.
        (void)actuator.restore(gov);
    }

    fs::remove_all(root);
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
        {"recovery_clear_failure_keeps_journal", test_recovery_clear_failure_keeps_journal_and_blocks_mutation},
        {"failed_commit_prevents_write", test_failed_journal_commit_prevents_any_write},
        {"corrupt_journal_blocks_mutation", test_corrupt_journal_blocks_mutation},
        {"unavailable_journal_value_fails_closed", test_journal_with_unavailable_value_fails_closed},
        {"journal_roundtrip_and_truncation", test_file_journal_roundtrip_and_truncation_detection},
        {"disabled_mode_hard_gate", test_disabled_mode_is_a_hard_no_mutation_gate},
        {"min_confidence_rejects_non_finite_and_malformed", test_min_confidence_rejects_non_finite_and_malformed_values},
        {"release_defaults_observe_only", test_release_defaults_are_observe_only_until_explicit_opt_in},
        {"mutation_requires_durable_journal", test_mutation_requires_durable_journal},
        {"empty_governor_advertisement_fails_closed", test_empty_governor_advertisement_fails_closed},
        {"low_confidence_restores", test_low_confidence_restores_instead_of_mutating},
        {"mutation_requires_explicit_arm", test_mutation_requires_explicit_arm},
        {"thermal_thresholds_consistent", test_thermal_thresholds_are_consistent},
        {"trial_mode_removed_from_config", test_experimental_trial_mode_is_not_enabled_by_config},
        {"observe_mode_no_kernel_write", test_observe_mode_never_writes_to_kernel_when_not_mutated},
        {"unarmed_adaptive_no_write", test_unarmed_adaptive_mode_never_writes},
        {"disabling_restores_only_own", test_disabling_mode_restores_only_own_mutation},
        {"beneficial_mutation_retained_without_time_expiry", test_beneficial_mutation_is_retained_without_time_expiry},
        {"rollback_latest_candidate_preserves_retained_value", test_rollback_only_latest_candidate_and_preserve_retained_value},
        {"external_override_preserved", test_external_override_is_preserved_during_candidate_rollback},
        {"corrupt_retained_state_fail_closed", test_corrupt_retained_state_fails_closed},
        {"retained_cpu_state_recovery_baseline", test_retained_cpu_state_restores_saved_baseline_after_restart},
        {"retained_state_store_roundtrip_and_corruption", test_retained_state_store_round_trip_and_corruption},
        {"safety_hold_markers", test_safety_hold_markers},
        {"multi_policy_all_restored", test_multi_policy_mutations_all_restored},
        {"restore_only_owned_policies", test_restore_only_owned_policies},
        {"mutation_authority_single_gate", test_mutation_authority_is_single_gate},
        {"environment_capability_matrix_fail_closed", test_environment_capability_matrix_is_fail_closed},
        {"resource_domain_names_stable", test_resource_domain_names_are_stable},
        {"experience_score_ranks_beneficial_governor", test_experience_score_ranks_beneficial_governor},
        {"regressed_governor_is_not_bootstrapped", test_regressed_governor_is_not_bootstrapped},
        {"experience_persistence_and_scope", test_experience_persists_and_is_bounded},
        {"capability_bound_permit_rejects_scope_and_mismatches",
         test_capability_bound_permit_rejects_scope_and_mismatches},
        {"cpufreq_actuator_rejects_scope_only_permit",
         test_cpufreq_actuator_rejects_scope_only_permit},
        {"recovery_missing_policy_keeps_journal",
         test_recovery_missing_policy_keeps_journal},
};

    for (const Case& c : cases) {
        const int before = g_failures;
        c.fn();
        std::printf("[%s] %s\n", g_failures == before ? " OK " : "FAIL", c.name);
    }

    std::printf("%d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
