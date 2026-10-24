#pragma once

#include "coreflow/model/types.hpp"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace coreflow {

// Operator commands accepted from the WebUI through the state directory. The
// WebUI can only send these fixed tokens; the daemon never evaluates text as a
// shell command or as a path.
enum class Command { None, Pause, Resume, Rescan, ResetLearning };

// Accepts only the exact lowercase tokens, ignoring surrounding whitespace.
[[nodiscard]] Command parseCommand(std::string_view text) noexcept;
[[nodiscard]] std::string_view commandToken(Command command) noexcept;

// RFC 8259 string escaping for quotes, backslashes and control characters.
[[nodiscard]] std::string jsonEscape(std::string_view text);

struct CapabilityRecord {
    std::string path;
    std::string domain;
    std::string attribute;
    std::string strategy_id;
    std::string current_value;
    bool eligible{false};
};

struct JournalRecord {
    std::string kind;          // "decision" or "mutation"
    std::int64_t unix_time{0};
    std::string resource;
    std::string previous_value;
    std::string target_value;
    std::string disposition;   // retain | rollback | unresolved | deferred | rejected
    std::string detail;
};

struct RuntimeStatus {
    std::string engine_state{"starting"};  // starting | running | paused
    std::string stage{"startup"};          // name of the C++ pipeline stage in progress
    std::int64_t generated_unix{0};
    std::uint64_t uptime_s{0};
    // Age after which the console must treat this status as stale. Derived by the
    // daemon from its own loop bounds, so the console never guesses it.
    std::uint64_t stale_after_s{0};
    std::uint64_t cycles{0};
    std::size_t discovered{0};
    std::size_t eligible{0};
    std::uint64_t retained{0};
    std::uint64_t rolled_back{0};
    std::uint64_t deferred{0};
    std::uint64_t unresolved{0};
    std::string workload_signature{"idle"};
    std::string last_error;
    std::string model;
    std::string soc;
    std::string android_release;
    std::string kernel_release;
    std::string storage_type;
    std::string catalog_status{"not-configured"};
    SystemObservation system;
};

[[nodiscard]] std::string formatStatusJson(const RuntimeStatus& status);
[[nodiscard]] std::string formatCapabilitiesJson(const std::vector<CapabilityRecord>& records,
                                                 std::size_t limit);
[[nodiscard]] std::string formatJournalLine(const JournalRecord& record);

// COREFLOW_STATE_DIR when set, otherwise /data/adb/coreflow. Kept outside the
// module directory so it survives module updates.
[[nodiscard]] std::string defaultStateDirectory();

// File-based bridge between the native daemon and the WebUI:
//   status.json        atomically replaced every cycle
//   capabilities.json  atomically replaced after each inventory refresh
//   journal.jsonl      append-only decision and mutation trace (rotated)
//   command            one token written by the WebUI, consumed by the daemon
class StateStore {
public:
    explicit StateStore(std::string directory);

    [[nodiscard]] bool ready() const noexcept { return ready_; }
    [[nodiscard]] const std::string& directory() const noexcept { return directory_; }

    bool writeStatus(const RuntimeStatus& status) const;
    bool writeCapabilities(const std::vector<CapabilityRecord>& records) const;
    bool appendJournal(const JournalRecord& record) const;
    [[nodiscard]] Command takeCommand() const;

private:
    bool atomicWrite(const std::string& name, const std::string& content) const;

    std::string directory_;
    bool ready_{false};
    // Public store operations can be called from multiple native threads. This
    // serializes atomic replacement, journal rotation/appends and command claims.
    mutable std::mutex io_mutex_;
};

} // namespace coreflow
