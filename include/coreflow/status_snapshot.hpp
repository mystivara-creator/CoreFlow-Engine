#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace coreflow {

// Live engine status exported for the WebUI. The engine is the only writer; the
// WebUI is a read-only consumer. Nothing here carries authority: it describes
// what the engine saw (eyes), decided (brain) and changed (hands).
//
// The snapshot is a single small JSON file replaced atomically every cycle, so a
// reader never sees a partial document. Writing it can fail without affecting
// control: callers must treat every function here as best-effort.
inline constexpr int kStatusSchemaVersion = 1;

struct EngineStatus {
    // identity / liveness
    std::string version;
    long pid{0};
    std::uint64_t sample{0};
    std::int64_t updated_epoch{0};
    int interval_s{0};
    bool running{true};

    // Device & ecosystem intelligence. These fields are observed identity and
    // discovered-interface facts, not inferred mutation permission.
    std::string manufacturer;
    std::string model;
    std::string device;
    std::string product;
    std::string board;
    std::string hardware;
    std::string soc_manufacturer;
    std::string soc_model;
    std::string android_release;
    std::string sdk_level;
    std::string kernel_release;
    std::string abi;
    bool proc_available{false};
    bool sys_available{false};
    bool cgroup_v2{false};
    bool cpuset_available{false};
    bool uclamp_available{false};
    bool scheduler_controls_available{false};
    bool devfreq_available{false};
    std::size_t discovered_resources{0};
    std::size_t mutation_ready_resources{0};
    bool block_queue_mutations_quarantined{true};

    // eyes: what the engine measured this cycle
    bool thermal_available{false};
    double thermal_c{0.0};
    double hottest_c{0.0};
    std::string thermal_trend;
    double mem_available_ratio{0.0};
    std::string memory_trend;
    bool cpu_available{false};
    double cpu_utilization{0.0};
    double load1{0.0};
    std::string load_trend;
    bool io_available{false};
    double io_read_kbs{0.0};
    double io_write_kbs{0.0};
    int battery_percent{-1};
    bool charging{false};
    double confidence{0.0};

    // brain: how the engine judged it
    std::string state;
    std::string previous_state;
    std::string decision;
    std::string workload;
    bool thermal_headroom{false};
    bool memory_headroom{false};
    bool power_headroom{false};
    bool proactive_allowed{false};
    bool stabilizing_allowed{false};
    std::string plan_action;
    std::string intervention;
    bool mutation_eligible{false};
    bool stabilizing_only{false};
    std::size_t candidates{0};
    std::string plan_reason;
    bool hold_active{false};
    std::string safety_hold;
    std::string mode;
    bool armed{false};
    bool allow_cpu_governor{false};
    // What MutationAuthority would grant right now (informational re-evaluation).
    bool permit_resource{false};
    bool permit_cpu{false};
    bool baseline_ready{false};
    std::size_t baseline_samples{0};
    std::size_t baseline_target{0};
    bool observing{false};
    std::uint64_t cooldown_remaining{0};

    // hands: what the engine changed and still holds
    bool cpu_mutated{false};
    bool resource_mutated{false};
    bool stabilizing_epoch{false};
    std::vector<std::pair<std::string, std::string>> cpu_applied;       // key, value
    std::vector<std::pair<std::string, std::string>> resource_applied;  // path, value
    std::string last_cpu_result;
    std::string last_resource_result;
    std::string last_change_kind;     // "cpu" | "resource" | ""
    std::string last_change_result;
    std::uint64_t last_change_sample{0};

    // cumulative since daemon start
    std::uint64_t verified{0};
    std::uint64_t rolled_back{0};
    std::uint64_t failed{0};
};

// Every code primaryBlocker() can return, in evaluation order.
const std::vector<std::string>& blockerCodes();

// One stable code answering "why is the engine not changing anything right now?".
// Evaluated in a fixed order so the answer is the first thing that stops it.
std::string primaryBlocker(const EngineStatus& status);

// Strict JSON for the status. Non-finite numbers become null; strings are escaped.
std::string statusToJson(const EngineStatus& status);

// Replace `path` with `json` atomically (temp file in the same directory, then
// rename). Returns false on any failure and never throws.
bool writeStatusFile(const std::string& path, const std::string& json) noexcept;

} // namespace coreflow
