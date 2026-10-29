#pragma once

#include <string>
#include <utility>
#include <vector>

#ifdef COREFLOW_HOST_TEST_FIXTURES
namespace coreflow::testing {
void failNextJournalDirectorySync(bool fail) noexcept;
}
#endif

namespace coreflow {

/**
 * Durable record of the factory values that must be restored if the daemon
 * dies while a mutation is active.
 *
 * Contract:
 *   - commit() must be durable (fsync'd) before the first sysfs write.
 *   - clear() follows verified restores or explicit release of externally
 *     replaced paths; other owned paths must remain journaled.
 *   - load() reports whether a previous run left an unrestored journal.
 */
class MutationJournal {
public:
    using Entries = std::vector<std::pair<std::string, std::string>>;

    enum class LoadState {
        Absent,   // no pending journal
        Pending,  // valid journal with factory values to restore
        Corrupt   // journal exists but cannot be trusted
    };

    virtual ~MutationJournal() = default;
    // Caller must first establish that another controller replaced its target.
    // Release only that path durably; preserve every other recovery obligation.
    // Idempotent after a successful release, including retry after a sync error.
    bool releaseExternallyControlledPath(const std::string& path) noexcept;

    virtual bool commit(const Entries& factory_values) noexcept = 0;
    virtual bool clear() noexcept = 0;
    virtual LoadState load(Entries& out) noexcept = 0;
};

/**
 * File-backed journal. New writes use bounded, checksummed v2 records; the
 * loader still accepts strict v1 records for recovery across upgrades. Writes
 * are atomic: temp file -> fsync -> rename -> fsync(directory). A malformed,
 * oversized, symlinked, or checksum-invalid record is Corrupt, never Absent.
 */
class FileMutationJournal final : public MutationJournal {
public:
    explicit FileMutationJournal(std::string path) : path_(std::move(path)) {}

    bool commit(const Entries& factory_values) noexcept override;
    bool clear() noexcept override;
    LoadState load(Entries& out) noexcept override;

    const std::string& path() const noexcept { return path_; }

private:
    std::string path_;
};

} // namespace coreflow
