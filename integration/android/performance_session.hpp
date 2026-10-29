#pragma once
#include <android/performance_hint.h>
#include <cerrno>
#include <cstdint>
#include <mutex>
#include <span>
#include <sys/types.h>

namespace coreflow::android {
// For cooperating apps, not a cross-process daemon boost. API 33+.
// Serialize session methods as required by the NDK. Owners must finish all
// callers before destruction, as for any C++ object containing a mutex.
class PerformanceSession final {
public:
    PerformanceSession() = default;
    ~PerformanceSession() { if (session_) APerformanceHint_closeSession(session_); }
    PerformanceSession(const PerformanceSession&) = delete;
    PerformanceSession& operator=(const PerformanceSession&) = delete;

    // TIDs must belong to the calling app. The platform validates membership.
    // A failed replacement preserves the existing session.
    bool start(std::span<const pid_t> tids, std::int64_t target_ns) {
        std::lock_guard lock(mutex_);
        if (tids.empty() || tids.size() > 1024U || target_ns <= 0) return false;
        for (const pid_t tid : tids) if (tid <= 0) return false;
        auto* manager = APerformanceHint_getManager();
        if (!manager) return false;
        auto* replacement = APerformanceHint_createSession(manager, tids.data(), tids.size(), target_ns);
        if (!replacement) return false;
        if (session_) APerformanceHint_closeSession(session_);
        session_ = replacement;
        return true;
    }
    int report(std::int64_t actual_ns) {
        std::lock_guard lock(mutex_);
        if (actual_ns <= 0) return EINVAL;
        return session_ ? APerformanceHint_reportActualWorkDuration(session_, actual_ns) : ENOTSUP;
    }
    int setTarget(std::int64_t target_ns) {
        std::lock_guard lock(mutex_);
        if (target_ns <= 0) return EINVAL;
        return session_ ? APerformanceHint_updateTargetWorkDuration(session_, target_ns) : ENOTSUP;
    }
    void close() {
        std::lock_guard lock(mutex_);
        if (session_) APerformanceHint_closeSession(session_);
        session_ = nullptr;
    }
private:
    std::mutex mutex_;
    APerformanceHintSession* session_{nullptr};
};
} // namespace coreflow::android
