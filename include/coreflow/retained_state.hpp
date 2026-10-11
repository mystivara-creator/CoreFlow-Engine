#pragma once

#include <string>
#include <utility>
#include <vector>

namespace coreflow {

struct DeviceProfile;

/** A previously measured-good setting and the value CoreFlow must restore. */
struct RetainedStateEntry {
    std::string scope;
    std::string path;
    std::string baseline;
    std::string target;
};

/**
 * Durable ledger of accepted settings. This is intentionally separate from
 * MutationJournal: that journal describes an in-flight write that must be
 * recovered, while this store describes a beneficial setting that may be
 * reconsidered after restart.
 */
class RetainedStateStore {
public:
    using Entries = std::vector<RetainedStateEntry>;

    enum class LoadState { Absent, Valid, Corrupt };
    virtual ~RetainedStateStore() = default;
    virtual LoadState load(Entries& out) noexcept = 0;
    virtual bool commit(const Entries& entries) noexcept = 0;
    virtual bool clear() noexcept = 0;

    bool upsert(const RetainedStateEntry& entry) noexcept;
    bool erasePath(const std::string& path) noexcept;
};

/** Atomic file-backed implementation (temporary file + fsync + rename). */
class FileRetainedStateStore final : public RetainedStateStore {
public:
    explicit FileRetainedStateStore(std::string path) : path_(std::move(path)) {}

    LoadState load(Entries& out) noexcept override;
    bool commit(const Entries& entries) noexcept override;
    bool clear() noexcept override;
    const std::string& path() const noexcept { return path_; }

private:
    std::string path_;
};

/** Stable device/kernel scope; a setting must not migrate across incompatible devices. */
std::string retainedScopeFor(const DeviceProfile& profile);

} // namespace coreflow
