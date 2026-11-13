/**
 * @file worker_pool.hpp
 * @brief Bounded -fno-exceptions worker pool, coalescing async snapshot writer,
 *        and hardened Linux kernel sysfs/procfs I/O primitives.
 * @license MIT
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace coreflow {

enum class KernelIoStatus : uint8_t {
    Ok = 0,
    InvalidPath,
    SymlinkRejected,
    NotRegularFile,
    NotFound,
    PermissionDenied,
    Busy,
    ReadOnlyFs,
    DeviceGone,
    TooLarge,
    IncompleteRead,
    WriteShort,
    IoError
};

[[nodiscard]] std::string_view kernel_io_status_name(KernelIoStatus status) noexcept;
[[nodiscard]] KernelIoStatus classify_kernel_errno(int err) noexcept;

struct KernelIoReadResult final {
    KernelIoStatus status{KernelIoStatus::IoError};
    int errno_value{0};
    std::string value;

    [[nodiscard]] bool ok() const noexcept { return status == KernelIoStatus::Ok; }
};

class KernelIo final {
public:
    static constexpr size_t kDefaultMaxLineBytes = 4096U;
    static constexpr size_t kMaxPathBytes = 512U;

    [[nodiscard]] static KernelIoReadResult read_first_line(
        std::string_view path,
        size_t max_bytes = kDefaultMaxLineBytes) noexcept;

    [[nodiscard]] static KernelIoReadResult read_bounded_file(
        std::string_view path,
        size_t max_bytes) noexcept;

    [[nodiscard]] static bool read_bounded_fd(
        int fd,
        std::string& output,
        size_t max_bytes,
        KernelIoStatus* status_out = nullptr,
        int* errno_out = nullptr) noexcept;

    [[nodiscard]] static bool write_all_fd(
        int fd,
        std::string_view data,
        KernelIoStatus* status_out = nullptr,
        int* errno_out = nullptr) noexcept;

    [[nodiscard]] static bool parse_u64_strict(std::string_view text, uint64_t& out) noexcept;
    [[nodiscard]] static bool parse_double_strict(std::string_view text, double& out) noexcept;
};

// Lightweight barrier for -fno-exceptions parallel fan-out.
class TaskGroup final {
public:
    TaskGroup() noexcept = default;
    ~TaskGroup() noexcept { wait(); }

    TaskGroup(const TaskGroup&) = delete;
    TaskGroup& operator=(const TaskGroup&) = delete;

    void add(size_t count = 1U) noexcept;
    void done() noexcept;
    void wait() noexcept;

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    size_t pending_{0U};
};

// Bounded thread pool designed for -fno-exceptions / -fno-rtti native Linux/Android
// runtimes. If the queue is full or the pool is shutting down, tasks execute inline
// on the caller thread so work is never silently dropped and deadlocks cannot occur.
class BoundedWorkerPool final {
public:
    static constexpr size_t kDefaultQueueCapacity = 64U;

    explicit BoundedWorkerPool(
        size_t num_threads = 0U,
        size_t max_queue_size = kDefaultQueueCapacity,
        std::string_view thread_prefix = "coreflow-w") noexcept;
    ~BoundedWorkerPool() noexcept;

    BoundedWorkerPool(const BoundedWorkerPool&) = delete;
    BoundedWorkerPool& operator=(const BoundedWorkerPool&) = delete;

    void submit(std::function<void()> task) noexcept;
    void submit(TaskGroup& group, std::function<void()> task) noexcept;
    void parallel_for(size_t count, const std::function<void(size_t)>& fn) noexcept;

    void shutdown() noexcept;

    [[nodiscard]] size_t worker_count() const noexcept { return workers_.size(); }
    [[nodiscard]] bool active() const noexcept {
        return !stopping_.load(std::memory_order_acquire) && !workers_.empty();
    }

private:
    void worker_loop(size_t index) noexcept;

    std::string thread_prefix_;
    size_t max_queue_size_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> queue_;
    std::vector<std::thread> workers_;
    std::atomic<bool> stopping_{false};
};

// Coalescing single-worker asynchronous writer that removes flash fsync() stalls
// from the EpollReactor event loop. Intermediate payloads are superseded by newer ones.
class AsyncSnapshotWriter final {
public:
    using WriteFn = std::function<bool(const std::string&)>;

    explicit AsyncSnapshotWriter(WriteFn writer) noexcept;
    ~AsyncSnapshotWriter() noexcept;

    AsyncSnapshotWriter(const AsyncSnapshotWriter&) = delete;
    AsyncSnapshotWriter& operator=(const AsyncSnapshotWriter&) = delete;

    void enqueue(std::string payload) noexcept;
    void stop() noexcept;

    [[nodiscard]] uint64_t completed_writes() const noexcept {
        return completed_writes_.load(std::memory_order_acquire);
    }
    [[nodiscard]] uint64_t failed_writes() const noexcept {
        return failed_writes_.load(std::memory_order_acquire);
    }

private:
    void writer_loop() noexcept;

    WriteFn writer_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::string pending_payload_;
    bool has_pending_{false};
    bool stopping_{false};
    std::atomic<uint64_t> completed_writes_{0U};
    std::atomic<uint64_t> failed_writes_{0U};
    std::thread worker_;
};

} // namespace coreflow
