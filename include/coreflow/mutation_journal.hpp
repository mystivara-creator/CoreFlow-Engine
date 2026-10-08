#pragma once

#include <string>
#include <utility>
#include <vector>

namespace coreflow {

/**
 * Durable record of the factory values that must be restored if the daemon
 * dies while a mutation is active.
 *
 * Contract:
 *   - commit() must be durable (fsync'd) before the first sysfs write.
 *   - clear() is called only after a fully verified restore.
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
    virtual bool commit(const Entries& factory_values) noexcept = 0;
    virtual bool clear() noexcept = 0;
    virtual LoadState load(Entries& out) noexcept = 0;
};

/**
 * File-backed journal. Writes are atomic: temp file -> fsync -> rename ->
 * fsync(directory). Entries are validated on load; any malformed content is
 * reported as Corrupt so mutation stays blocked until an operator intervenes.
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
