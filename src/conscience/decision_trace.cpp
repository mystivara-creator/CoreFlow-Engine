#include "coreflow/decision_trace.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>
#include <string>
#include <system_error>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace coreflow {
namespace {

constexpr const char* kDecisionTracePath = "/data/adb/coreflow/decision_trace.jsonl";
constexpr const char* kDecisionTraceLockPath = "/data/adb/coreflow/decision_trace.lock";
constexpr std::uintmax_t kDecisionTraceMaxBytes = 512U * 1024U;
std::mutex g_decision_trace_mutex;
std::atomic<std::uint64_t> g_decision_trace_sequence{0U};
std::atomic<std::uint64_t> g_decision_trace_dropped{0U};

class TraceFileLock final {
public:
    TraceFileLock() noexcept {
        fd_ = ::open(kDecisionTraceLockPath,
                      O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (fd_ < 0) return;
        while (::flock(fd_, LOCK_EX) != 0) {
            if (errno == EINTR) continue;
            (void)::close(fd_);
            fd_ = -1;
            return;
        }
        (void)::fchmod(fd_, 0600);
    }

    ~TraceFileLock() {
        if (fd_ >= 0) {
            (void)::flock(fd_, LOCK_UN);
            (void)::close(fd_);
        }
    }

    TraceFileLock(const TraceFileLock&) = delete;
    TraceFileLock& operator=(const TraceFileLock&) = delete;

    bool valid() const noexcept { return fd_ >= 0; }

private:
    int fd_{-1};
};

std::string jsonEscape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8U);
    for (const char raw : value) {
        const auto ch = static_cast<unsigned char>(raw);
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20U) {
                    constexpr char hex[] = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(ch >> 4U) & 0x0fU];
                    out += hex[ch & 0x0fU];
                } else {
                    out += static_cast<char>(ch);
                }
        }
    }
    return out;
}

const char* safeText(const char* value, const char* fallback = "") noexcept {
    return value != nullptr ? value : fallback;
}

const char* mutationResultTraceName(MutationResult result) noexcept {
    switch (result) {
        case MutationResult::Skipped: return "SKIPPED";
        case MutationResult::Applied: return "APPLIED";
        case MutationResult::Verified: return "VERIFIED";
        case MutationResult::Failed: return "FAILED";
        case MutationResult::RolledBack: return "ROLLED_BACK";
    }
    return "UNKNOWN";
}

const std::string& traceSessionId() {
    static const std::string value = [] {
        const auto ticks = std::chrono::steady_clock::now().time_since_epoch();
        const auto ticks_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(ticks).count();
        return std::to_string(static_cast<long long>(::getpid())) + "-" +
               std::to_string(static_cast<long long>(std::time(nullptr))) + "-" +
               std::to_string(static_cast<long long>(ticks_ns));
    }();
    return value;
}

void writeJsonNumber(std::ostream& out, double value) {
    if (std::isfinite(value)) {
        out << std::setprecision(8) << value;
    } else {
        out << "null";
    }
}

void rotateDecisionTraceIfNeeded() noexcept {
    try {
        std::error_code ec;
        const auto size = std::filesystem::file_size(kDecisionTracePath, ec);
        if (ec || size < kDecisionTraceMaxBytes) return;
        const std::string previous = std::string(kDecisionTracePath) + ".1";
        (void)std::remove(previous.c_str());
        (void)std::rename(kDecisionTracePath, previous.c_str());
    } catch (...) {
        // Diagnostics must never interrupt the control loop.
    }
}

// Keep filesystem latency out of the control loop. The bounded queue is
// intentionally lossy under pressure; missing event_seq values plus
// trace_dropped_before make that loss visible without blocking actuation.
class AsyncDecisionTraceWriter final {
public:
    AsyncDecisionTraceWriter() noexcept {
        try {
            worker_ = std::thread([this] { run(); });
        } catch (...) {
            // A missing diagnostic worker must never prevent daemon startup.
        }
    }

    ~AsyncDecisionTraceWriter() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        ready_.notify_one();
        if (worker_.joinable()) worker_.join();
    }

    AsyncDecisionTraceWriter(const AsyncDecisionTraceWriter&) = delete;
    AsyncDecisionTraceWriter& operator=(const AsyncDecisionTraceWriter&) = delete;

    bool enqueue(std::string line) noexcept {
        try {
            std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
            if (!lock.owns_lock() || stopping_ || !worker_.joinable() ||
                queue_.size() >= kQueueCapacity) {
                return false;
            }
            queue_.push_back(std::move(line));
            lock.unlock();
            ready_.notify_one();
            return true;
        } catch (...) {
            return false;
        }
    }

private:
    static constexpr std::size_t kQueueCapacity = 1024U;
    static constexpr std::size_t kBatchCapacity = 64U;

    void run() noexcept {
        for (;;) {
            std::vector<std::string> batch;
            try {
                std::unique_lock<std::mutex> lock(mutex_);
                ready_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
                // Briefly coalesce records from one control cycle to reduce opens,
                // lock traffic, flushes, and flash wakeups.
                if (!stopping_ && queue_.size() < kBatchCapacity) {
                    (void)ready_.wait_for(lock, std::chrono::milliseconds(8), [this] {
                        return stopping_ || queue_.size() >= kBatchCapacity;
                    });
                }
                const std::size_t count = std::min(queue_.size(), kBatchCapacity);
                batch.reserve(count);
                for (std::size_t i = 0; i < count; ++i) {
                    batch.push_back(std::move(queue_.front()));
                    queue_.pop_front();
                }
                const bool done = stopping_ && queue_.empty() && batch.empty();
                lock.unlock();
                if (done) return;
            } catch (...) {
                // Allocation failed before queue entries were removed. Back off
                // briefly so a low-memory device does not spin in this worker.
                std::this_thread::sleep_for(std::chrono::milliseconds(8));
                continue;
            }
            if (!batch.empty()) writeBatch(batch);
        }
    }

    void writeBatch(const std::vector<std::string>& batch) noexcept {
        if (batch.empty()) return;
        try {
            TraceFileLock file_lock;
            if (!file_lock.valid()) {
                g_decision_trace_dropped.fetch_add(
                    static_cast<std::uint64_t>(batch.size()), std::memory_order_relaxed);
                return;
            }
            rotateDecisionTraceIfNeeded();
            const int fd = ::open(kDecisionTracePath,
                O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0600);
            if (fd < 0) {
                g_decision_trace_dropped.fetch_add(
                    static_cast<std::uint64_t>(batch.size()), std::memory_order_relaxed);
                return;
            }

            struct stat st{};
            if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || ::fchmod(fd, 0600) != 0) {
                (void)::close(fd);
                g_decision_trace_dropped.fetch_add(
                    static_cast<std::uint64_t>(batch.size()), std::memory_order_relaxed);
                return;
            }

            std::size_t required = 0U;
            constexpr std::size_t kMaxSize = std::numeric_limits<std::size_t>::max();
            for (const auto& line : batch) {
                // Include the trailing newline in the overflow check.
                if (required == kMaxSize || line.size() >= kMaxSize - required) {
                    (void)::close(fd);
                    g_decision_trace_dropped.fetch_add(
                        static_cast<std::uint64_t>(batch.size()), std::memory_order_relaxed);
                    return;
                }
                required += line.size() + 1U;
            }
            std::string payload;
            payload.reserve(required);
            for (const auto& line : batch) {
                payload += line;
                payload.push_back('\n');
            }

            const off_t start_offset = ::lseek(fd, 0, SEEK_END);
            bool write_ok = start_offset >= 0;
            std::size_t written = 0U;
            while (write_ok && written < payload.size()) {
                const ssize_t count = ::write(
                    fd, payload.data() + written, payload.size() - written);
                if (count < 0 && errno == EINTR) continue;
                if (count <= 0) {
                    write_ok = false;
                    break;
                }
                written += static_cast<std::size_t>(count);
            }
            if (!write_ok && start_offset >= 0) {
                // The sidecar lock serializes this append. Remove a partial batch
                // on ordinary write failure so it cannot corrupt following JSONL.
                (void)::ftruncate(fd, start_offset);
            }
            const int close_status = ::close(fd);
            write_ok = write_ok && close_status == 0;
            if (!write_ok) {
                g_decision_trace_dropped.fetch_add(
                    static_cast<std::uint64_t>(batch.size()), std::memory_order_relaxed);
            }
        } catch (...) {
            g_decision_trace_dropped.fetch_add(
                static_cast<std::uint64_t>(batch.size()), std::memory_order_relaxed);
        }
    }

    std::mutex mutex_;
    std::condition_variable ready_;
    std::deque<std::string> queue_;
    std::thread worker_;
    bool stopping_{false};
};

AsyncDecisionTraceWriter& decisionTraceWriter() {
    static AsyncDecisionTraceWriter writer;
    return writer;
}

} // namespace

void appendDecisionTraceEvent(const DecisionTraceContext& context,
                              const DecisionTraceEvent& event) noexcept {
    try {
        if (context.context == nullptr || context.plan == nullptr) return;

        // This mutex keeps per-process sequence assignment and enqueue order
        // consistent. Filesystem work occurs only on the bounded background writer.
        std::unique_lock<std::mutex> process_lock(
            g_decision_trace_mutex, std::try_to_lock);
        if (!process_lock.owns_lock()) {
            g_decision_trace_dropped.fetch_add(1U, std::memory_order_relaxed);
            return;
        }

        const std::time_t now = std::time(nullptr);
        std::tm tm{};
        char timestamp[32] = "unknown";
        if (now != static_cast<std::time_t>(-1) && gmtime_r(&now, &tm) != nullptr) {
            (void)std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &tm);
        }

        const char* decision_reason = decisionTraceReason(
            context.hold, context.mode, context.armed,
            context.plan->mutation_eligible,
            context.cpu_mutation, context.resource_mutation);
        const std::uint64_t sequence =
            g_decision_trace_sequence.fetch_add(1U, std::memory_order_relaxed) + 1U;
        const std::string session_id = traceSessionId();

        const std::uint64_t dropped_before =
            g_decision_trace_dropped.load(std::memory_order_relaxed);
        std::ostringstream out;
        out << "{\"schema\":\"coreflow.event.v2\""
            << ",\"session_id\":\"" << jsonEscape(session_id) << "\""
            << ",\"event_seq\":" << sequence
            << ",\"trace_dropped_before\":" << dropped_before
            << ",\"timestamp\":\"" << timestamp << "\""
            << ",\"cycle\":" << context.cycle
            << ",\"event\":\"" << jsonEscape(safeText(event.event, "event")) << "\""
            << ",\"scope\":\"" << jsonEscape(safeText(event.scope, "engine")) << "\""
            << ",\"result\":\"" << jsonEscape(safeText(event.result, "UNKNOWN")) << "\""
            << ",\"reason\":\"" << jsonEscape(safeText(event.reason)) << "\""
            << ",\"decision_reason\":";
        if (context.cpu_mutation_known && context.resource_mutation_known) {
            out << "\"" << decision_reason << "\"";
        } else {
            out << "null";
        }
        out << ",\"previous_state\":\"" << jsonEscape(stateName(context.previous_state)) << "\""
            << ",\"state\":\"" << jsonEscape(stateName(context.state)) << "\""
            << ",\"decision\":\"" << jsonEscape(decisionName(context.decision)) << "\""
            << ",\"confidence\":";
        writeJsonNumber(out, context.confidence);
        out << ",\"workload\":\""
            << jsonEscape(workloadClassName(context.context->workload)) << "\""
            << ",\"context_confidence\":";
        writeJsonNumber(out, context.context->confidence);
        out << ",\"mutation_eligible\":"
            << (context.plan->mutation_eligible ? "true" : "false")
            << ",\"stabilizing_only\":"
            << (context.plan->stabilizing_only ? "true" : "false")
            << ",\"candidate_count\":" << context.plan->candidates.size()
            << ",\"plan_action\":\"" << jsonEscape(policyActionName(context.plan->action)) << "\""
            << ",\"intervention\":" << static_cast<unsigned>(context.plan->intervention)
            << ",\"plan_reason\":\"" << jsonEscape(context.plan->reason) << "\""
            << ",\"safety_hold\":\"" << jsonEscape(holdReasonName(context.hold)) << "\""
            << ",\"mode\":\"" << (context.mode == MutationMode::Adaptive ? "adaptive" :
                                       context.hold != HoldReason::None ? "disabled" : "observe") << "\""
            << ",\"mutation_armed\":" << (context.armed ? "true" : "false")
            << ",\"allow_cpu_governor\":" << (context.cpu_governor_allowed ? "true" : "false")
            << ",\"permit_valid\":";
        if (event.permit_known) out << (event.permit_valid ? "true" : "false");
        else out << "null";
        out << ",\"failure_stage\":\"" << jsonEscape(safeText(event.failure_stage)) << "\""
            << ",\"actuator_status\":\"" << jsonEscape(safeText(event.actuator_status)) << "\""
            << ",\"operation_id\":";
        if (event.target != nullptr && event.target[0] != '\0') {
            const std::string operation_id = session_id + ":" +
                std::to_string(context.cycle) + ":" + safeText(event.scope, "engine") + ":" +
                safeText(event.target);
            out << "\"" << jsonEscape(operation_id) << "\"";
        } else {
            out << "null";
        }
        out << ",\"target\":";
        if (event.target != nullptr) out << "\"" << jsonEscape(event.target) << "\"";
        else out << "null";
        out << ",\"requested\":";
        if (event.requested != nullptr) out << "\"" << jsonEscape(event.requested) << "\"";
        else out << "null";
        out << ",\"current_value\":";
        if (event.current_value != nullptr) out << "\"" << jsonEscape(event.current_value) << "\"";
        else out << "null";
        out << ",\"baseline_value\":";
        if (event.baseline_value != nullptr) out << "\"" << jsonEscape(event.baseline_value) << "\"";
        else out << "null";
        out << ",\"writes_attempted\":";
        if (event.write_counts_known) out << event.writes_attempted;
        else out << "null";
        out << ",\"writes_verified\":";
        if (event.write_counts_known) out << event.writes_verified;
        else out << "null";
        out << ",\"cpu_mutation\":";
        if (context.cpu_mutation_known) {
            out << "\"" << mutationResultTraceName(context.cpu_mutation) << "\"";
        } else {
            out << "null";
        }
        out << ",\"resource_mutation\":";
        if (context.resource_mutation_known) {
            out << "\"" << mutationResultTraceName(context.resource_mutation) << "\"";
        } else {
            out << "null";
        }
        out << "}";
        if (!out.good() || !decisionTraceWriter().enqueue(out.str())) {
            g_decision_trace_dropped.fetch_add(1U, std::memory_order_relaxed);
        }
    } catch (...) {
        // Serialization/allocation failure loses this diagnostic event, but
        // must remain visible to subsequent records and must never affect permits.
        g_decision_trace_dropped.fetch_add(1U, std::memory_order_relaxed);
    }
}

void appendDecisionTrace(
    std::uint64_t cycle,
    RuntimeState previous,
    RuntimeState current,
    Decision decision,
    double confidence,
    const SystemContext& context,
    const PolicyPlan& plan,
    HoldReason hold,
    MutationMode mode,
    bool armed,
    bool cpuGovernorAllowed,
    MutationResult cpuMutation,
    MutationResult resourceMutation
) noexcept {
    DecisionTraceContext trace_context{
        cycle, previous, current, decision, confidence, &context, &plan,
        hold, mode, armed, cpuGovernorAllowed, cpuMutation, resourceMutation};
    trace_context.cpu_mutation_known = true;
    trace_context.resource_mutation_known = true;
    const char* reason = decisionTraceReason(
        hold, mode, armed, plan.mutation_eligible, cpuMutation, resourceMutation);
    appendDecisionTraceEvent(trace_context,
        {"cycle_completed", "engine", "COMPLETED", reason});
}

} // namespace coreflow
