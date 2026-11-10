#include "coreflow/memory_warmup_controller.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace {
constexpr uint64_t kDefaultHardBudget = 32ULL * 1024ULL * 1024ULL;
constexpr double kMaximumMemoryPsiPct = 10.0;
constexpr double kMaximumIoPsiPct = 10.0;

bool valid_admission(const MemoryWarmupAdmission& a) noexcept {
    if (!a.memory_pressure_valid || !a.io_pressure_valid ||
        !a.thermal_admission_valid || !a.thermal_admission_granted ||
        !a.working_set_prediction_valid || !a.foreground_identity_valid ||
        a.max_telemetry_age_ns == 0U || a.telemetry_timestamp_ns == 0U ||
        a.now_ns < a.telemetry_timestamp_ns ||
        a.now_ns - a.telemetry_timestamp_ns > a.max_telemetry_age_ns ||
        !std::isfinite(a.memory_pressure_pct) ||
        !std::isfinite(a.io_pressure_pct) ||
        a.memory_pressure_pct < 0.0 || a.memory_pressure_pct > kMaximumMemoryPsiPct ||
        a.io_pressure_pct < 0.0 || a.io_pressure_pct > kMaximumIoPsiPct ||
        a.available_headroom_bytes == 0U || a.configured_budget_bytes == 0U ||
        a.max_single_file_bytes == 0U) {
        return false;
    }
    return true;
}
} // namespace

MemoryWarmupController::MemoryWarmupController(uint64_t hard_budget_bytes) noexcept
    : hard_budget_bytes_(std::min(hard_budget_bytes, kDefaultHardBudget)) {}

MemoryWarmupController::~MemoryWarmupController() noexcept {
    cancel();
}

void MemoryWarmupController::cancel() noexcept {
    cancelled_.store(true, std::memory_order_release);
}

MemoryWarmupResult MemoryWarmupController::warmup(
    const std::vector<WarmupFileRange>& ranges,
    const MemoryWarmupAdmission& admission) noexcept {
    std::lock_guard<std::mutex> lock(warmup_mutex_);
    MemoryWarmupResult result{};
    for (const auto& range : ranges) {
        if (range.length <= std::numeric_limits<uint64_t>::max() - result.requested_bytes) {
            result.requested_bytes += range.length;
        } else {
            result.requested_bytes = std::numeric_limits<uint64_t>::max();
        }
    }

    if (cancelled_.load(std::memory_order_acquire)) {
        result.cancelled = true;
        return result;
    }
    if (!valid_admission(admission) || hard_budget_bytes_ == 0U) {
        result.files_rejected = static_cast<uint32_t>(
            std::min<std::size_t>(ranges.size(), std::numeric_limits<uint32_t>::max()));
        return result;
    }

    // Reserve at least 75% of currently reported headroom for the rest of the
    // system. The caller's configured budget can only reduce our hard ceiling.
    const uint64_t headroom_budget = admission.available_headroom_bytes / 4U;
    uint64_t remaining = std::min({hard_budget_bytes_,
                                   admission.configured_budget_bytes,
                                   headroom_budget});
    if (remaining == 0U) {
        result.files_rejected = static_cast<uint32_t>(
            std::min<std::size_t>(ranges.size(), std::numeric_limits<uint32_t>::max()));
        return result;
    }

    for (const auto& range : ranges) {
        if (cancelled_.load(std::memory_order_acquire)) {
            result.cancelled = true;
            break;
        }
        if (range.path.empty() || range.path.find('\0') != std::string::npos ||
            range.length == 0U || range.offset >
                static_cast<uint64_t>(std::numeric_limits<off_t>::max()) ||
            range.length > static_cast<uint64_t>(std::numeric_limits<off_t>::max()) - range.offset) {
            ++result.files_rejected;
            continue;
        }

        const uint64_t advised = std::min({range.length, remaining,
                                           admission.max_single_file_bytes});
        if (advised == 0U) {
            ++result.files_rejected;
            continue;
        }

        const int fd = ::open(range.path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
        if (fd < 0) {
            ++result.files_rejected;
            continue;
        }
        struct stat metadata {};
        const bool valid_file = ::fstat(fd, &metadata) == 0 &&
                                S_ISREG(metadata.st_mode) &&
                                metadata.st_size >= 0 &&
                                range.offset <= static_cast<uint64_t>(metadata.st_size) &&
                                advised <= static_cast<uint64_t>(metadata.st_size) - range.offset;
        if (!valid_file) {
            (void)::close(fd);
            ++result.files_rejected;
            continue;
        }

        if (cancelled_.load(std::memory_order_acquire)) {
            (void)::close(fd);
            result.cancelled = true;
            break;
        }

        const int advice = ::posix_fadvise(
            fd, static_cast<off_t>(range.offset), static_cast<off_t>(advised),
            POSIX_FADV_WILLNEED);
        (void)::close(fd);
        if (advice != 0) {
            ++result.files_rejected;
            continue;
        }

        result.admitted_bytes += advised;
        remaining -= advised;
        ++result.files_advised;
        if (remaining == 0U) break;
    }
    result.cancelled = result.cancelled ||
                       cancelled_.load(std::memory_order_acquire);
    return result;
}

} // namespace coreflow
