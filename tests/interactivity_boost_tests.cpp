#include "coreflow/interactivity_boost.hpp"
#include "coreflow/config.hpp"
#include "coreflow/mutation_journal.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", \
                     __FILE__, __LINE__, #condition); \
        return __LINE__; \
    } \
} while (false)

namespace fs = std::filesystem;
using namespace coreflow;

namespace {

// Journal double with switchable failures. It keeps the last committed state so
// tests can check what would survive a restart.
class FaultyJournal final : public MutationJournal {
public:
    bool fail_commit{false};
    bool fail_clear{false};
    bool report_corrupt{false};
    std::size_t commits{0};
    Entries last;

    bool commit(const Entries& entries) noexcept override {
        if (fail_commit) return false;
        last = entries;
        ++commits;
        return true;
    }
    bool clear() noexcept override {
        if (fail_clear || fail_commit) return false;  // a journal fault covers every write
        last.clear();
        return true;
    }
    LoadState load(Entries& out) noexcept override {
        if (report_corrupt) return LoadState::Corrupt;
        out = last;
        return last.empty() ? LoadState::Absent : LoadState::Pending;
    }
};

void writeText(const fs::path& path, const std::string& body) {
    std::ofstream out(path);
    out << body;
}

std::string readText(const fs::path& path) {
    std::ifstream in(path);
    std::string body;
    std::getline(in, body);
    return body;
}

EngineConfig armedConfig() {
    EngineConfig config;
    config.setInteractivityMode(InteractivityMode::Adaptive);
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    config.setGrantCgroup(true);       // uclamp boost domain
    config.setGrantBlockQueue(true);   // read-ahead boost domain
    config.setGrantGpu(true);
    return config;
}

BoostIntent uclampIntent(long value = 20, std::uint64_t window_ms = 1000U) {
    BoostIntent intent;
    intent.kind = BoostKind::TopAppUclampMin;
    intent.window_ms = window_ms;
    intent.value = value;
    intent.reason = "test";
    return intent;
}

struct Fixture {
    fs::path root;
    fs::path uclamp;
    FaultyJournal journal;
    InteractivityBoostController boost;

    explicit Fixture(const std::string& name) : root(fs::temp_directory_path() / name) {
        fs::remove_all(root);
        fs::create_directories(root);
        uclamp = root / "uclamp_min";
        writeText(uclamp, "5");
        boost.setJournal(&journal);
        boost.setTopAppUclampPath(uclamp.string());
    }
    ~Fixture() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
};

// ---- positive path -----------------------------------------------------------

int testBoostAppliesJournalsAndRestores() {
    Fixture f("coreflow_boost_positive");
    const EngineConfig config = armedConfig();
    CHECK(f.boost.apply({uclampIntent()}, config, 10'000U) == 1U);
    CHECK(readText(f.uclamp) == "20");
    CHECK(f.boost.activeCount() == 1U);
    CHECK(f.journal.last.size() == 1U);
    CHECK(f.journal.last[0].second == "5");  // the journal holds the true baseline

    f.boost.expire(11'000U);                  // past the 1000 ms window
    CHECK(readText(f.uclamp) == "5");
    CHECK(f.boost.activeCount() == 0U);
    CHECK(f.journal.last.empty());
    CHECK(!f.boost.recoveryRequired());
    return 0;
}

// ---- gates -------------------------------------------------------------------

int testDisarmedWritesNothing() {
    Fixture f("coreflow_boost_disarmed");
    EngineConfig config = armedConfig();
    config.setMutationArmed(false);
    CHECK(f.boost.apply({uclampIntent()}, config, 10'000U) == 0U);
    CHECK(readText(f.uclamp) == "5");
    return 0;
}

int testUngrantedDomainWritesNothing() {
    Fixture f("coreflow_boost_ungranted");
    EngineConfig config = armedConfig();
    config.setGrantCgroup(false);  // capability exists, grant does not
    CHECK(f.boost.apply({uclampIntent()}, config, 10'000U) == 0U);
    CHECK(readText(f.uclamp) == "5");
    CHECK(f.boost.activeCount() == 0U);
    return 0;
}

int testGrantForOneDomainDoesNotAllowAnother() {
    Fixture f("coreflow_boost_cross_domain");
    EngineConfig config = armedConfig();
    config.setGrantCgroup(false);
    config.setGrantBlockQueue(true);  // granted, but for a different domain
    CHECK(f.boost.apply({uclampIntent()}, config, 10'000U) == 0U);
    CHECK(readText(f.uclamp) == "5");
    return 0;
}

// ---- journal fault injection -------------------------------------------------

int testFailedCommitBeforeWriteStopsAndRecovers() {
    Fixture f("coreflow_boost_commit_fail");
    const EngineConfig config = armedConfig();
    f.journal.fail_commit = true;
    CHECK(f.boost.apply({uclampIntent()}, config, 10'000U) == 0U);
    CHECK(readText(f.uclamp) == "5");           // no write without a durable journal
    CHECK(f.boost.recoveryRequired());
    CHECK(f.boost.activeCount() == 0U);

    // While the journal fault persists, recovery cannot finish and nothing is applied.
    CHECK(f.boost.apply({uclampIntent()}, config, 10'000U) == 0U);
    CHECK(readText(f.uclamp) == "5");
    CHECK(f.boost.recoveryRequired());

    // Once the journal works again, apply first retries recovery, then applies.
    f.journal.fail_commit = false;
    CHECK(f.boost.apply({uclampIntent()}, config, 10'000U) == 1U);
    CHECK(!f.boost.recoveryRequired());
    CHECK(readText(f.uclamp) == "20");
    return 0;
}

int testRestoreFailureKeepsEntryAndBlocksPath() {
    Fixture f("coreflow_boost_restore_fail");
    const EngineConfig config = armedConfig();
    CHECK(f.boost.apply({uclampIntent()}, config, 10'000U) == 1U);

    // Make the restore fail: the actuator refuses a final-component symlink.
    const fs::path target = f.root / "elsewhere";
    writeText(target, "20");
    fs::remove(f.uclamp);
    fs::create_directory_symlink(target, f.uclamp);

    f.boost.expire(11'000U);
    CHECK(f.boost.activeCount() == 1U);          // kept: restore is pending
    CHECK(!f.boost.recoveryRequired());          // the journal itself is fine
    CHECK(f.journal.last.size() == 1U);          // still journaled as pending

    // The pending path is never boosted again, so the original is not lost.
    CHECK(f.boost.apply({uclampIntent(30)}, config, 11'000U) == 0U);

    // Repair the path and retry: restore succeeds, and retrying is idempotent.
    fs::remove(f.uclamp);
    writeText(f.uclamp, "20");
    f.boost.expire(12'000U);
    CHECK(readText(f.uclamp) == "5");
    CHECK(f.boost.activeCount() == 0U);
    f.boost.expire(12'000U);
    CHECK(readText(f.uclamp) == "5");
    CHECK(f.boost.activeCount() == 0U);
    return 0;
}

int testRestoredButUncommittedIsRetriedNotForgotten() {
    Fixture f("coreflow_boost_commit_after_restore");
    const EngineConfig config = armedConfig();
    CHECK(f.boost.apply({uclampIntent()}, config, 10'000U) == 1U);

    f.journal.fail_commit = true;
    f.boost.expire(11'000U);                     // value restored, journal write fails
    CHECK(readText(f.uclamp) == "5");
    CHECK(f.boost.recoveryRequired());
    CHECK(f.boost.activeCount() == 1U);          // kept until the journal agrees
    CHECK(f.boost.apply({uclampIntent()}, config, 11'000U) == 0U);

    f.journal.fail_commit = false;
    f.boost.expire(11'000U);                     // retry: commit succeeds, entry drops
    CHECK(!f.boost.recoveryRequired());
    CHECK(f.boost.activeCount() == 0U);
    CHECK(f.journal.last.empty());
    return 0;
}

int testRestoreAllIsPartialAndJournalled() {
    Fixture f("coreflow_boost_restore_all");
    const fs::path readahead = f.root / "read_ahead_kb";
    writeText(readahead, "128");
    f.boost.addReadaheadPath(readahead.string());

    const EngineConfig config = armedConfig();
    BoostIntent readahead_intent;
    readahead_intent.kind = BoostKind::IoReadaheadKb;
    readahead_intent.window_ms = 1000U;
    readahead_intent.value = 512;
    CHECK(f.boost.apply({uclampIntent(), readahead_intent}, config, 10'000U) == 2U);
    CHECK(f.boost.activeCount() == 2U);

    // The read-ahead restore fails; the uclamp restore succeeds.
    const fs::path target = f.root / "elsewhere_ra";
    writeText(target, "512");
    fs::remove(readahead);
    fs::create_directory_symlink(target, readahead);

    f.boost.restoreAll();
    CHECK(readText(f.uclamp) == "5");
    CHECK(f.boost.activeCount() == 1U);          // only the failed restore remains
    CHECK(f.journal.last.size() == 1U);
    CHECK(f.journal.last[0].first == readahead.string());
    CHECK(!f.boost.recoveryRequired());
    return 0;
}

int testRestoreAllCommitFailureIsReported() {
    Fixture f("coreflow_boost_restore_all_commit");
    const EngineConfig config = armedConfig();
    CHECK(f.boost.apply({uclampIntent()}, config, 10'000U) == 1U);
    f.journal.fail_commit = true;
    f.boost.restoreAll();
    CHECK(readText(f.uclamp) == "5");
    CHECK(f.boost.recoveryRequired());
    f.journal.fail_commit = false;
    f.boost.restoreAll();
    CHECK(!f.boost.recoveryRequired());
    CHECK(f.boost.activeCount() == 0U);
    return 0;
}

int testRollbackFailureIsNotSilent() {
    Fixture f("coreflow_boost_rollback");
    // Make the write itself fail after the journal is durable: the path becomes a
    // symlink before apply, so the actuator refuses it, and the boost is not counted.
    const fs::path target = f.root / "elsewhere_w";
    writeText(target, "5");
    fs::remove(f.uclamp);
    fs::create_directory_symlink(target, f.uclamp);
    const EngineConfig config = armedConfig();
    CHECK(f.boost.apply({uclampIntent()}, config, 10'000U) == 0U);
    CHECK(f.boost.activeCount() == 0U);
    CHECK(!f.boost.recoveryRequired());
    return 0;
}

int testRecoveryAdoptsPendingEntriesAndRestoresBaseline() {
    // Simulates a crash while a boost was active: the file holds the boosted
    // value, and the journal holds the true baseline.
    Fixture f("coreflow_boost_recover");
    writeText(f.uclamp, "20");
    f.journal.last = {{f.uclamp.string(), "5"}};
    CHECK(f.boost.recoverFromJournal() == 1U);
    CHECK(f.boost.activeCount() == 1U);
    f.boost.expire(1U);                                              // due at once
    CHECK(readText(f.uclamp) == "5");
    CHECK(f.boost.activeCount() == 0U);
    CHECK(f.journal.last.empty());
    return 0;
}

int testCorruptJournalBlocksBoostsAndIsNotOverwritten() {
    Fixture f("coreflow_boost_corrupt");
    f.journal.report_corrupt = true;
    const std::size_t commits_before = f.journal.commits;
    CHECK(f.boost.recoverFromJournal() == 0U);
    CHECK(f.boost.recoveryRequired());
    CHECK(f.boost.apply({uclampIntent()}, armedConfig(), 10'000U) == 0U);
    f.boost.expire(20'000U);                                         // must not clear the corrupt file
    CHECK(f.journal.commits == commits_before);
    CHECK(readText(f.uclamp) == "5");
    return 0;
}

} // namespace

int main() {
    int failures = 0;
    failures += testBoostAppliesJournalsAndRestores() != 0;
    failures += testDisarmedWritesNothing() != 0;
    failures += testUngrantedDomainWritesNothing() != 0;
    failures += testGrantForOneDomainDoesNotAllowAnother() != 0;
    failures += testFailedCommitBeforeWriteStopsAndRecovers() != 0;
    failures += testRestoreFailureKeepsEntryAndBlocksPath() != 0;
    failures += testRestoredButUncommittedIsRetriedNotForgotten() != 0;
    failures += testRestoreAllIsPartialAndJournalled() != 0;
    failures += testRestoreAllCommitFailureIsReported() != 0;
    failures += testRecoveryAdoptsPendingEntriesAndRestoresBaseline() != 0;
    failures += testCorruptJournalBlocksBoostsAndIsNotOverwritten() != 0;
    failures += testRollbackFailureIsNotSilent() != 0;
    if (failures != 0) {
        std::fprintf(stderr, "CoreFlow interactivity boost tests: FAIL (%d)\n", failures);
        return 1;
    }
    std::puts("CoreFlow interactivity boost tests: PASS");
    return 0;
}
