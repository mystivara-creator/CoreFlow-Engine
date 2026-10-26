#pragma once

#include <string>
#include <utility>
#include <vector>

namespace coreflow {

enum class RetainedStateFailureStage : unsigned char {
    None = 0,
    Precondition,
    BaselineInvalid,
    LiveReadFailed,
    LiveTargetMismatch,
    AllocationFailed,
    StoreUpsertFailed,
    RecoveryCommitFailed
};

inline const char* retainedStateFailureStageName(
    RetainedStateFailureStage stage
) noexcept {
    switch (stage) {
        case RetainedStateFailureStage::None: return "NONE";
        case RetainedStateFailureStage::Precondition: return "PRECONDITION_FAILED";
        case RetainedStateFailureStage::BaselineInvalid: return "BASELINE_INVALID";
        case RetainedStateFailureStage::LiveReadFailed: return "LIVE_READ_FAILED";
        case RetainedStateFailureStage::LiveTargetMismatch: return "LIVE_TARGET_MISMATCH";
        case RetainedStateFailureStage::AllocationFailed: return "ALLOCATION_FAILED";
        case RetainedStateFailureStage::StoreUpsertFailed: return "STORE_UPSERT_FAILED";
        case RetainedStateFailureStage::RecoveryCommitFailed: return "RECOVERY_COMMIT_FAILED";
    }
    return "UNKNOWN";
}

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
