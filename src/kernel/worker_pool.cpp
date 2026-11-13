/**
 * @file worker_pool.cpp
 * @brief Bounded -fno-exceptions worker pool, coalescing async snapshot writer,
 *        and hardened Linux kernel sysfs/procfs I/O primitives.
 * @license MIT
 */
#include "coreflow/worker_pool.hpp"
#include "coreflow/types.hpp"
#include "coreflow/unique_fd.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace coreflow {
namespace {

constexpr unsigned kMaxTransientIoRetries = 3U;

bool is_transient_retryable(int err) noexcept {
    return err == EINTR || err == EAGAIN || err == EWOULDBLOCK;
}

void tiny_io_backoff(int err, unsigned attempt) noexcept {
    if (err == EINTR) return;
    if (attempt > 0U) {
        ::sched_yield();
    }
}

void set_current_thread_name(std::string_view prefix, size_t index) noexcept {
    std::string name(prefix);
    name += std::to_string(index);
    if (name.size() > 15U) {
        name.resize(15U);
    }
    (void)::pthread_setname_np(::pthread_self(), name.c_str());
}

} // namespace

std::string_view kernel_io_status_name(KernelIoStatus status) noexcept {
    switch (status) {
        case KernelIoStatus::Ok: return "Ok";
        case KernelIoStatus::InvalidPath: return "InvalidPath";
        case KernelIoStatus::SymlinkRejected: return "SymlinkRejected";
        case KernelIoStatus::NotRegularFile: return "NotRegularFile";
        case KernelIoStatus::NotFound: return "NotFound";
        case KernelIoStatus::PermissionDenied: return "PermissionDenied";
        case KernelIoStatus::Busy: return "Busy";
        case KernelIoStatus::ReadOnlyFs: return "ReadOnlyFs";
        case KernelIoStatus::DeviceGone: return "DeviceGone";
        case KernelIoStatus::TooLarge: return "TooLarge";
        case KernelIoStatus::IncompleteRead: return "IncompleteRead";
        case KernelIoStatus::WriteShort: return "WriteShort";
        case KernelIoStatus::IoError: return "IoError";
    }
    return "IoError";
}

KernelIoStatus classify_kernel_errno(int err) noexcept {
    switch (err) {
        case 0: return KernelIoStatus::Ok;
        case ENOENT:
        case ENOTDIR:
            return KernelIoStatus::NotFound;
        case EACCES:
        case EPERM:
            return KernelIoStatus::PermissionDenied;
        case ELOOP:
            return KernelIoStatus::SymlinkRejected;
        case EBUSY:
        case EAGAIN:
#if EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
            return KernelIoStatus::Busy;
        case EROFS:
            return KernelIoStatus::ReadOnlyFs;
        case ENODEV:
        case ENXIO:
        case ESHUTDOWN:
            return KernelIoStatus::DeviceGone;
        case EINVAL:
        case ENAMETOOLONG:
            return KernelIoStatus::InvalidPath;
        default:
            return KernelIoStatus::IoError;
    }
}

KernelIoReadResult KernelIo::read_first_line(
    std::string_view path,
    size_t max_bytes) noexcept {
    KernelIoReadResult result{};
    if (path.empty() || path.size() > kMaxPathBytes ||
        path.find('\0') != std::string_view::npos || max_bytes == 0U) {
        result.status = KernelIoStatus::InvalidPath;
        result.errno_value = EINVAL;
        return result;
    }

    const std::string path_str(path);
    int raw_fd = -1;
    int last_err = 0;
    for (unsigned attempt = 0U; attempt <= kMaxTransientIoRetries; ++attempt) {
        raw_fd = ::open(path_str.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
        if (raw_fd >= 0) break;
        last_err = errno;
        if (!is_transient_retryable(last_err) || attempt == kMaxTransientIoRetries) {
            result.status = classify_kernel_errno(last_err);
            result.errno_value = last_err;
            return result;
        }
        tiny_io_backoff(last_err, attempt);
    }

    UniqueFd fd(raw_fd);
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0) {
        last_err = errno;
        result.status = classify_kernel_errno(last_err);
        result.errno_value = last_err;
        return result;
    }
    if (!S_ISREG(st.st_mode)) {
        result.status = S_ISLNK(st.st_mode)
            ? KernelIoStatus::SymlinkRejected
            : KernelIoStatus::NotRegularFile;
        result.errno_value = EINVAL;
        return result;
    }

    std::string raw;
    raw.reserve(std::min<size_t>(max_bytes, 256U));
    char buf[1024];
    unsigned transient_retries = 0U;
    while (raw.size() < max_bytes) {
        const size_t chunk = std::min(sizeof(buf), max_bytes - raw.size());
        const ssize_t n = ::read(fd.get(), buf, chunk);
        if (n < 0) {
            const int err = errno;
            if (is_transient_retryable(err) && transient_retries < kMaxTransientIoRetries) {
                ++transient_retries;
                tiny_io_backoff(err, transient_retries);
                continue;
            }
            if (!raw.empty() && err == EAGAIN) break;
            result.status = classify_kernel_errno(err);
            result.errno_value = err;
            return result;
        }
        if (n == 0) break;
        raw.append(buf, static_cast<size_t>(n));
        if (raw.find('\n') != std::string::npos) break;
    }

    const size_t nl = raw.find('\n');
    if (nl != std::string::npos) raw.resize(nl);
    if (!raw.empty() && raw.back() == '\r') raw.pop_back();
    result.status = KernelIoStatus::Ok;
    result.errno_value = 0;
    result.value = std::move(raw);
    return result;
}

KernelIoReadResult KernelIo::read_bounded_file(
    std::string_view path, size_t max_bytes) noexcept {
    KernelIoReadResult result{};
    if (path.empty() || path.size() > kMaxPathBytes ||
        path.find('\0') != std::string_view::npos || max_bytes == 0U) {
        result.status = KernelIoStatus::InvalidPath;
        result.errno_value = EINVAL;
        return result;
    }
    const std::string path_str(path);
    int fd = -1;
    for (unsigned attempt = 0U; attempt <= kMaxTransientIoRetries; ++attempt) {
        fd = ::open(path_str.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
        if (fd >= 0) break;
        const int err = errno;
        if (!is_transient_retryable(err) || attempt == kMaxTransientIoRetries) {
            result.status = classify_kernel_errno(err);
            result.errno_value = err;
            return result;
        }
        tiny_io_backoff(err, attempt);
    }
    UniqueFd owned(fd);
    KernelIoStatus status = KernelIoStatus::IoError;
    int error = 0;
    if (!read_bounded_fd(owned.get(), result.value, max_bytes, &status, &error)) {
        result.status = status;
        result.errno_value = error;
        result.value.clear();
        return result;
    }
    result.status = KernelIoStatus::Ok;
    result.errno_value = 0;
    return result;
}

bool KernelIo::read_bounded_fd(
    int fd,
    std::string& output,
    size_t max_bytes,
    KernelIoStatus* status_out,
    int* errno_out) noexcept {
    const auto finish = [&](KernelIoStatus s, int e, bool ok) noexcept {
        if (status_out != nullptr) *status_out = s;
        if (errno_out != nullptr) *errno_out = e;
        return ok;
    };
    if (fd < 0) return finish(KernelIoStatus::InvalidPath, EBADF, false);

    struct stat st{};
    if (::fstat(fd, &st) != 0) {
        return finish(classify_kernel_errno(errno), errno, false);
    }
    if (!S_ISREG(st.st_mode)) {
        return finish(KernelIoStatus::NotRegularFile, EINVAL, false);
    }
    if (st.st_size < 0 || static_cast<uint64_t>(st.st_size) > max_bytes) {
        return finish(KernelIoStatus::TooLarge, EFBIG, false);
    }

    output.clear();
    output.reserve(static_cast<size_t>(st.st_size));
    char buf[4096];
    unsigned transient_retries = 0U;
    for (;;) {
        const ssize_t n = ::read(fd, buf, sizeof(buf));
        if (n < 0) {
            const int err = errno;
            if (is_transient_retryable(err) && transient_retries < kMaxTransientIoRetries) {
                ++transient_retries;
                tiny_io_backoff(err, transient_retries);
                continue;
            }
            return finish(classify_kernel_errno(err), err, false);
        }
        if (n == 0) break;
        const size_t count = static_cast<size_t>(n);
        if (count > max_bytes - output.size()) {
            output.clear();
            return finish(KernelIoStatus::TooLarge, EFBIG, false);
        }
        output.append(buf, count);
    }
    return finish(KernelIoStatus::Ok, 0, true);
}

bool KernelIo::write_all_fd(
    int fd,
    std::string_view data,
    KernelIoStatus* status_out,
    int* errno_out) noexcept {
    const auto finish = [&](KernelIoStatus s, int e, bool ok) noexcept {
        if (status_out != nullptr) *status_out = s;
        if (errno_out != nullptr) *errno_out = e;
        return ok;
    };
    if (fd < 0) return finish(KernelIoStatus::InvalidPath, EBADF, false);

    size_t offset = 0U;
    unsigned transient_retries = 0U;
    while (offset < data.size()) {
        const ssize_t written = ::write(fd, data.data() + offset, data.size() - offset);
        if (written < 0) {
            const int err = errno;
            if (is_transient_retryable(err) && transient_retries < kMaxTransientIoRetries) {
                ++transient_retries;
                tiny_io_backoff(err, transient_retries);
                continue;
            }
            return finish(classify_kernel_errno(err), err, false);
        }
        if (written == 0) {
            return finish(KernelIoStatus::WriteShort, EIO, false);
        }
        offset += static_cast<size_t>(written);
        transient_retries = 0U;
    }
    return finish(KernelIoStatus::Ok, 0, true);
}

bool KernelIo::parse_u64_strict(std::string_view text, uint64_t& out) noexcept {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        text.remove_prefix(1U);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) {
        text.remove_suffix(1U);
    }
    if (text.empty()) return false;
    uint64_t parsed = 0U;
    const auto res = std::from_chars(text.data(), text.data() + text.size(), parsed, 10);
    if (res.ec != std::errc{} || res.ptr != text.data() + text.size()) return false;
    out = parsed;
    return true;
}

bool KernelIo::parse_double_strict(std::string_view text, double& out) noexcept {
    return parse_ascii_double(text, out);
}

void TaskGroup::add(size_t count) noexcept {
    if (count == 0U) return;
    std::lock_guard<std::mutex> lock(mutex_);
    pending_ += count;
}

void TaskGroup::done() noexcept {
    bool notify = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_ > 0U) {
            --pending_;
            notify = (pending_ == 0U);
        }
    }
    if (notify) {
        cv_.notify_all();
    }
}

void TaskGroup::wait() noexcept {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this]() noexcept { return pending_ == 0U; });
}

BoundedWorkerPool::BoundedWorkerPool(
    size_t num_threads,
    size_t max_queue_size,
    std::string_view thread_prefix) noexcept
    : thread_prefix_(thread_prefix.empty() ? "coreflow-w" : std::string(thread_prefix)),
      max_queue_size_(std::max<size_t>(4U, max_queue_size)) {
    const long online_cpus = ::sysconf(_SC_NPROCESSORS_ONLN);
    const size_t hw = online_cpus > 0 ? static_cast<size_t>(online_cpus) : 0U;
    const size_t count = (num_threads == 0U)
        ? std::clamp<size_t>(hw == 0U ? 2U : hw, 2U, 4U)
        : std::clamp<size_t>(num_threads, 1U, 8U);
    workers_.reserve(count);
    worker_contexts_.reserve(count);
    for (size_t i = 0U; i < count; ++i) {
        worker_contexts_.push_back(WorkerContext{.pool = this, .index = i});
        pthread_t worker{};
        const int result = ::pthread_create(
            &worker, nullptr, &BoundedWorkerPool::worker_entry, &worker_contexts_.back());
        if (result != 0) {
            worker_contexts_.pop_back();
            break; // Partial or zero-worker mode remains operational via inline execution.
        }
        workers_.push_back(worker);
    }
}

BoundedWorkerPool::~BoundedWorkerPool() noexcept {
    shutdown();
}

void* BoundedWorkerPool::worker_entry(void* context) noexcept {
    auto* worker = static_cast<WorkerContext*>(context);
    if (worker != nullptr && worker->pool != nullptr) {
        worker->pool->worker_loop(worker->index);
    }
    return nullptr;
}

size_t BoundedWorkerPool::worker_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return workers_.size();
}

bool BoundedWorkerPool::active() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return !stopping_.load(std::memory_order_acquire) && !workers_.empty();
}

void BoundedWorkerPool::worker_loop(size_t index) noexcept {
    set_current_thread_name(thread_prefix_, index);
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() noexcept {
                return stopping_.load(std::memory_order_acquire) || !queue_.empty();
            });
            if (stopping_.load(std::memory_order_acquire) && queue_.empty()) {
                return;
            }
            task = std::move(queue_.front());
            queue_.pop_front();
        }
        if (task) {
            task();
        }
    }
}

void BoundedWorkerPool::submit(std::function<void()> task) noexcept {
    if (!task) return;
    bool run_inline = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_.load(std::memory_order_acquire) || workers_.empty() ||
            queue_.size() >= max_queue_size_) {
            run_inline = true;
        } else {
            queue_.push_back(std::move(task));
        }
    }
    if (run_inline) {
        task();
    } else {
        cv_.notify_one();
    }
}

void BoundedWorkerPool::submit(TaskGroup& group, std::function<void()> task) noexcept {
    if (!task) return;
    group.add(1U);
    submit([&group, fn = std::move(task)]() mutable noexcept {
        fn();
        group.done();
    });
}

void BoundedWorkerPool::parallel_for(
    size_t count,
    const std::function<void(size_t)>& fn) noexcept {
    if (count == 0U || !fn) return;
    if (count == 1U || !active()) {
        for (size_t i = 0U; i < count; ++i) {
            fn(i);
        }
        return;
    }
    TaskGroup group;
    // Execute index 0 on the calling thread while workers process [1..count-1]
    // to minimize thread wakeup overhead on small batches.
    for (size_t i = 1U; i < count; ++i) {
        submit(group, [&fn, i]() noexcept { fn(i); });
    }
    fn(0U);
    group.wait();
}

void BoundedWorkerPool::shutdown() noexcept {
    std::lock_guard<std::mutex> shutdown_lock(shutdown_mutex_);
    if (shutdown_complete_) return;

    // The predicate and transition share mutex_: notify cannot race between
    // a worker's predicate check and its condition-variable wait.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_.store(true, std::memory_order_release);
    }
    cv_.notify_all();

    for (const pthread_t worker : workers_) {
        (void)::pthread_join(worker, nullptr);
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        workers_.clear();
        worker_contexts_.clear();
    }
    shutdown_complete_ = true;
}

AsyncSnapshotWriter::AsyncSnapshotWriter(WriteFn writer) noexcept
    : writer_(std::move(writer)) {
    if (writer_) {
        worker_started_ = (::pthread_create(
            &worker_, nullptr, &AsyncSnapshotWriter::writer_entry, this) == 0);
    }
}

AsyncSnapshotWriter::~AsyncSnapshotWriter() noexcept {
    stop();
}

void AsyncSnapshotWriter::enqueue(std::string payload) noexcept {
    if (!writer_) return;
    bool run_inline = false;
    bool run_async = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) return;
        pending_payload_ = std::move(payload);
        has_pending_ = true;
        if (worker_started_) {
            run_async = true;
        } else if (!inline_writing_) {
            inline_writing_ = true;
            run_inline = true;
        }
    }
    if (run_async) {
        cv_.notify_one();
    } else if (run_inline) {
        drain_inline();
    }
}

void* AsyncSnapshotWriter::writer_entry(void* context) noexcept {
    auto* writer = static_cast<AsyncSnapshotWriter*>(context);
    if (writer != nullptr) writer->writer_loop();
    return nullptr;
}

void AsyncSnapshotWriter::drain_inline() noexcept {
    for (;;) {
        std::string payload;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!has_pending_) {
                inline_writing_ = false;
                cv_.notify_all();
                return;
            }
            payload = std::move(pending_payload_);
            has_pending_ = false;
        }
        if (writer_(payload)) {
            completed_writes_.fetch_add(1U, std::memory_order_release);
        } else {
            failed_writes_.fetch_add(1U, std::memory_order_release);
        }
    }
}

void AsyncSnapshotWriter::writer_loop() noexcept {
    (void)::pthread_setname_np(::pthread_self(), "coreflow-io");
    for (;;) {
        std::string payload;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() noexcept { return stopping_ || has_pending_; });
            if (stopping_ && !has_pending_) return;
            payload = std::move(pending_payload_);
            has_pending_ = false;
        }
        if (writer_(payload)) {
            completed_writes_.fetch_add(1U, std::memory_order_release);
        } else {
            failed_writes_.fetch_add(1U, std::memory_order_release);
        }
    }
}

void AsyncSnapshotWriter::stop() noexcept {
    std::lock_guard<std::mutex> stop_lock(stop_mutex_);
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (stopping_ && (!worker_started_ || worker_joined_)) return;
        stopping_ = true;
        cv_.notify_all();
        if (!worker_started_) {
            cv_.wait(lock, [this]() noexcept { return !inline_writing_; });
            return;
        }
    }

    (void)::pthread_join(worker_, nullptr);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        worker_joined_ = true;
        worker_started_ = false;
    }
}
} // namespace coreflow
