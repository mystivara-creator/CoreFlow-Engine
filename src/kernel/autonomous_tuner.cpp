/**
 * @file autonomous_tuner.cpp
 * @brief Self-Directed Multi-Domain Transaction Planner & Empirical Outcome Evaluator
 * @license MIT
 */
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/config.hpp"
#include "coreflow/unique_fd.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;

namespace {

std::string read_first_line(const std::string& path) {
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return {};
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) return {};
    char buf[1024];
    const ssize_t n = ::read(fd.get(), buf, sizeof(buf));
    if (n <= 0) return {};
    std::string raw(buf, static_cast<size_t>(n));
    const size_t nl = raw.find('\n');
    if (nl != std::string::npos) raw.resize(nl);
    if (!raw.empty() && raw.back() == '\r') raw.pop_back();
    return raw;
}

bool parse_u64(const std::string& text, uint64_t& out) {
    const std::string trimmed = DaemonConfig::trim(text);
    if (trimmed.empty()) return false;
    const auto res = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), out);
    return res.ec == std::errc{} && res.ptr == trimmed.data() + trimmed.size();
}

bool is_physical_block_device(std::string_view name) {
    if (name.empty()) return false;
    // Exclude virtual block devices: loop*, ram*, zram*, dm-*, md*, nbd*
    if (name.rfind("loop", 0) == 0 || name.rfind("ram", 0) == 0 ||
        name.rfind("zram", 0) == 0 || name.rfind("dm-", 0) == 0 ||
        name.rfind("md", 0) == 0 || name.rfind("nbd", 0) == 0) {
        return false;
    }
    return name.rfind("sd", 0) == 0 ||
           name.rfind("mmcblk", 0) == 0 ||
           name.rfind("nvme", 0) == 0;
}

void parse_scheduler_line(
    const std::string& line,
    std::string& active_out,
    std::vector<std::string>& available_out) {
    active_out.clear();
    available_out.clear();
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) {
        if (token.size() > 2 && token.front() == '[' && token.back() == ']') {
            const std::string clean = token.substr(1, token.size() - 2);
            active_out = clean;
            available_out.push_back(clean);
        } else {
            available_out.push_back(token);
        }
    }
    if (active_out.empty() && available_out.size() == 1) {
        active_out = available_out.front();
    }
}

bool contains_token(const std::vector<std::string>& list, std::string_view target) {
    return std::any_of(list.begin(), list.end(), [&](const std::string& s) { return s == target; });
}

TunerVerdict verdict_from_string(std::string_view v) noexcept {
    if (v == "IMPROVED") return TunerVerdict::Improved;
    if (v == "NEUTRAL") return TunerVerdict::Neutral;
    if (v == "REGRESSED") return TunerVerdict::Regressed;
    if (v == "ABORTED") return TunerVerdict::Aborted;
    if (v == "OWNERSHIP_LOST") return TunerVerdict::OwnershipLost;
    if (v == "PENDING") return TunerVerdict::Pending;
    return TunerVerdict::Hold;
}

} // namespace

AutonomousTuner& AutonomousTuner::instance() {
    static AutonomousTuner tuner;
    return tuner;
}

AutonomousTuner::AutonomousTuner(std::string sys_root, std::string proc_root)
    : sys_root_(std::move(sys_root)), proc_root_(std::move(proc_root)) {
    init_candidates();
    discover(sys_root_, proc_root_);
}

void AutonomousTuner::init_candidates() {
    candidates_ = {
        CandidateStats{
            .id = "io.mq_deadline_low_latency",
            .domain = "io",
            .label = "mq-deadline + Low-Latency Read Expiry (250ms / batch 8)",
            .mean_utility = 0.65,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.86,
            .trials = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .cooldown_ticks = 0,
        },
        CandidateStats{
            .id = "io.kyber_token_bucket",
            .domain = "io",
            .label = "kyber Token-Bucket Low-Latency I/O Scheduler",
            .mean_utility = 0.62,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.83,
            .trials = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .cooldown_ticks = 0,
        },
        CandidateStats{
            .id = "io.bfq_interactive_fairness",
            .domain = "io",
            .label = "bfq Proportional-Share Interactive I/O Scheduler",
            .mean_utility = 0.55,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.76,
            .trials = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .cooldown_ticks = 0,
        },
        CandidateStats{
            .id = "io.queue_readahead_pacing",
            .domain = "io",
            .label = "Adaptive Block Queue Read-Ahead & Request Depth (256KB / 128 req)",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.71,
            .trials = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .cooldown_ticks = 0,
        },
        CandidateStats{
            .id = "vm.zram_latency_relief",
            .domain = "vm",
            .label = "VM Page-Cluster Zero-Latency & VFS Dentry Preservation",
            .mean_utility = 0.60,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.81,
            .trials = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .cooldown_ticks = 0,
        },
        CandidateStats{
            .id = "cpufreq.schedutil_interactive_response",
            .domain = "cpufreq",
            .label = "EAS Governor Rate-Limit Pacing (500us up / 10000us down)",
            .mean_utility = 0.58,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.79,
            .trials = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .cooldown_ticks = 0,
        },
    };
    update_ucb_scores();
}

void AutonomousTuner::update_ucb_scores() {
    const double total_n = std::max(1.0, static_cast<double>(total_trials_));
    for (auto& c : candidates_) {
        c.successes = c.improved_count;
        c.rollbacks = c.regressed_count;
        const double explore = 0.25 * std::sqrt(std::log(total_n + 1.0) / static_cast<double>(c.trials + 1));
        c.ucb_score = c.mean_utility + explore;
    }
}

void AutonomousTuner::reset() {
    rollback_active("Autonomous tuner reset to verified OEM baseline.");
    init_candidates();
    total_trials_ = 0;
    total_improved_ = 0;
    total_rolled_back_ = 0;
    last_verdict_ = "HOLD";
    last_verdict_enum_ = TunerVerdict::Hold;
    last_utility_delta_ = 0.0;
    idle_streak_ = 0;
}

CandidateStats* AutonomousTuner::find_candidate(std::string_view id) {
    for (auto& c : candidates_) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

void AutonomousTuner::discover(std::string_view sys_root, std::string_view proc_root) {
    sys_root_ = std::string(sys_root);
    proc_root_ = std::string(proc_root);
    block_queues_.clear();
    cpufreq_governor_dirs_.clear();

    const fs::path block_root = fs::path(sys_root_) / "block";
    std::error_code ec;
    for (fs::directory_iterator it(block_root, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && block_queues_.size() < 8;
         it.increment(ec)) {
        const std::string dev_name = it->path().filename().string();
        if (!is_physical_block_device(dev_name)) continue;

        const fs::path qdir = it->path() / "queue";
        const std::string sched_path = (qdir / "scheduler").string();
        if (::access(sched_path.c_str(), R_OK) != 0) continue;

        const std::string sched_raw = read_first_line(sched_path);
        if (sched_raw.empty()) continue;

        BlockQueueSurface surface{};
        surface.device_name = dev_name;
        surface.queue_dir = qdir.string();
        parse_scheduler_line(sched_raw, surface.active_scheduler, surface.available_schedulers);
        if (surface.active_scheduler.empty()) continue;
        surface.baseline_scheduler = surface.active_scheduler;

        uint64_t val = 0;
        if (parse_u64(read_first_line((qdir / "read_ahead_kb").string()), val)) {
            surface.baseline_read_ahead_kb = val;
            surface.active_read_ahead_kb = val;
            surface.read_ahead_kb = val;
        }
        if (parse_u64(read_first_line((qdir / "nr_requests").string()), val)) {
            surface.baseline_nr_requests = val;
            surface.active_nr_requests = val;
            surface.nr_requests = val;
        }
        if (parse_u64(read_first_line((qdir / "rq_affinity").string()), val)) {
            surface.rq_affinity = val;
        }

        const fs::path iosched = qdir / "iosched";
        if (fs::is_directory(iosched, ec) && !ec) {
            surface.has_iosched_tunables = true;
            if (parse_u64(read_first_line((iosched / "read_expire").string()), val)) {
                surface.read_expire_ms = val;
            }
            if (parse_u64(read_first_line((iosched / "writes_starved").string()), val)) {
                surface.writes_starved = val;
            }
            if (parse_u64(read_first_line((iosched / "fifo_batch").string()), val)) {
                surface.fifo_batch = val;
            }
        }

        // Pre-capture baseline values in BaselineStore for safe restoration
        auto& store = BaselineStore::instance();
        (void)store.capture(sched_path);
        (void)store.capture((qdir / "read_ahead_kb").string());
        (void)store.capture((qdir / "nr_requests").string());
        (void)store.capture((qdir / "rq_affinity").string());
        if (surface.has_iosched_tunables) {
            (void)store.capture((iosched / "read_expire").string());
            (void)store.capture((iosched / "writes_starved").string());
            (void)store.capture((iosched / "fifo_batch").string());
        }

        block_queues_.push_back(std::move(surface));
    }

    // Sort deterministically by device name (e.g. sda before sdb)
    std::sort(block_queues_.begin(), block_queues_.end(), [](const auto& a, const auto& b) {
        return a.device_name < b.device_name;
    });

    // Discover CPUFreq governor interactive rate-limit directories (schedutil / walt)
    auto& store = BaselineStore::instance();
    const fs::path cpufreq_root = fs::path(sys_root_) / "devices/system/cpu/cpufreq";
    std::error_code cpu_ec;
    for (fs::directory_iterator it(cpufreq_root, fs::directory_options::skip_permission_denied, cpu_ec), end;
         !cpu_ec && it != end && cpufreq_governor_dirs_.size() < 8;
         it.increment(cpu_ec)) {
        const std::string pol_name = it->path().filename().string();
        if (pol_name.rfind("policy", 0) != 0) continue;
        for (const char* gov : {"schedutil", "walt"}) {
            const fs::path gdir = it->path() / gov;
            const std::string up_path = (gdir / "up_rate_limit_us").string();
            const std::string down_path = (gdir / "down_rate_limit_us").string();
            if (::access(up_path.c_str(), R_OK | W_OK) == 0 &&
                ::access(down_path.c_str(), R_OK | W_OK) == 0) {
                (void)store.capture(up_path);
                (void)store.capture(down_path);
                cpufreq_governor_dirs_.push_back(gdir.string());
                break;
            }
        }
    }
    std::sort(cpufreq_governor_dirs_.begin(), cpufreq_governor_dirs_.end());

    // Capture VM baseline nodes if accessible
    (void)store.capture(proc_root_ + "/sys/vm/page-cluster");
    (void)store.capture(proc_root_ + "/sys/vm/vfs_cache_pressure");
}

void AutonomousTuner::refresh_surface_readings() {
    for (auto& q : block_queues_) {
        std::string active;
        std::vector<std::string> avail;
        parse_scheduler_line(read_first_line(q.queue_dir + "/scheduler"), active, avail);
        if (!active.empty()) q.active_scheduler = active;
        if (!avail.empty()) q.available_schedulers = std::move(avail);

        uint64_t val = 0;
        if (parse_u64(read_first_line(q.queue_dir + "/read_ahead_kb"), val)) {
            q.active_read_ahead_kb = val;
            q.read_ahead_kb = val;
        }
        if (parse_u64(read_first_line(q.queue_dir + "/nr_requests"), val)) {
            q.active_nr_requests = val;
            q.nr_requests = val;
        }
        if (parse_u64(read_first_line(q.queue_dir + "/rq_affinity"), val)) {
            q.rq_affinity = val;
        }
        if (parse_u64(read_first_line(q.queue_dir + "/iosched/read_expire"), val)) {
            q.read_expire_ms = val;
            q.has_iosched_tunables = true;
        }
        if (parse_u64(read_first_line(q.queue_dir + "/iosched/writes_starved"), val)) {
            q.writes_starved = val;
        }
        if (parse_u64(read_first_line(q.queue_dir + "/iosched/fifo_batch"), val)) {
            q.fifo_batch = val;
        }
    }
    update_ucb_scores();
}

void AutonomousTuner::rollback_active(std::string_view reason) {
    auto& store = BaselineStore::instance();
    // Unwind in reverse dependency order: Coupled-with dependents first, then Anchor!
    for (auto it = applied_steps_.rbegin(); it != applied_steps_.rend(); ++it) {
        (void)store.rollback(it->path);
    }
    applied_steps_.clear();
    transaction_active_ = false;
    observing_ = false;
    obs_ticks_ = 0;
    current_candidate_id_ = "baseline";
    current_candidate_label_ = "OEM Verified Baseline";
    current_intent_ = "HoldEquilibrium";
    current_rationale_ = std::string(reason);
    refresh_surface_readings();
}

bool AutonomousTuner::apply_transaction(
    std::string_view candidate_id,
    std::string_view candidate_label,
    std::string_view intent,
    std::string_view rationale,
    const std::vector<MutationStep>& steps,
    const TunerTelemetryInput& telemetry) {
    if (steps.empty()) return false;

    // First unwind any previous transaction cleanly
    if (!applied_steps_.empty()) {
        rollback_active("Switching coordinated candidate");
    }

    auto& store = BaselineStore::instance();
    std::vector<MutationStep> committed;
    committed.reserve(steps.size());

    for (const auto& step : steps) {
        if (::access(step.path.c_str(), W_OK) != 0) continue;
        (void)store.capture(step.path);
        if (!store.apply_mutation(step.path, step.target_value)) {
            // Dependency or readback failure: unwind already-applied steps in reverse order
            for (auto it = committed.rbegin(); it != committed.rend(); ++it) {
                (void)store.rollback(it->path);
            }
            refresh_surface_readings();
            return false;
        }
        committed.push_back(step);
    }

    if (committed.empty()) return false;

    applied_steps_ = std::move(committed);
    transaction_active_ = true;
    observing_ = true;
    obs_ticks_ = 0;
    current_candidate_id_ = std::string(candidate_id);
    current_candidate_label_ = std::string(candidate_label);
    current_intent_ = std::string(intent);
    current_rationale_ = std::string(rationale);

    before_io_psi_ = telemetry.io_psi;
    before_mem_psi_ = telemetry.memory_psi;
    before_cpu_psi_ = telemetry.cpu_psi;
    before_thermal_ = telemetry.thermal_pressure;

    sum_after_io_psi_ = 0.0;
    sum_after_mem_psi_ = 0.0;
    sum_after_cpu_psi_ = 0.0;
    sum_after_thermal_ = 0.0;
    ++total_trials_;

    if (auto* stats = find_candidate(candidate_id)) {
        ++stats->trials;
    }
    refresh_surface_readings();
    return true;
}

void AutonomousTuner::evaluate_observation_window() {
    if (!observing_ || obs_ticks_ == 0) return;

    const double n = static_cast<double>(obs_ticks_);
    const double after_io = sum_after_io_psi_ / n;
    const double after_mem = sum_after_mem_psi_ / n;
    const double after_cpu = sum_after_cpu_psi_ / n;
    const double after_therm = sum_after_thermal_ / n;

    // Verify ownership is still intact across all applied nodes
    auto& store = BaselineStore::instance();
    for (const auto& step : applied_steps_) {
        (void)store;
        if (::access(step.path.c_str(), R_OK) != 0) {
            last_verdict_ = "OWNERSHIP_LOST";
            last_verdict_enum_ = TunerVerdict::OwnershipLost;
            ++total_rolled_back_;
            rollback_active("External node ownership lost during observation window.");
            return;
        }
    }

    // Calculate domain-weighted empirical utility delta:
    // Positive delta means system pressure/stalls decreased without thermal regression.
    const double io_gain = before_io_psi_ - after_io;
    const double mem_gain = before_mem_psi_ - after_mem;
    const double cpu_gain = before_cpu_psi_ - after_cpu;
    const double therm_penalty = std::max(0.0, after_therm - before_thermal_ - 2.0);

    double utility_delta = 0.0;
    if (current_intent_ == "IoLatencyRelief") {
        utility_delta = 1.40 * io_gain + 0.35 * cpu_gain - 0.60 * therm_penalty;
    } else if (current_intent_ == "MemoryStallRelief") {
        utility_delta = 1.60 * mem_gain + 0.45 * io_gain - 0.60 * therm_penalty;
    } else if (current_intent_ == "InteractiveCpuPacing") {
        utility_delta = 1.50 * cpu_gain + 0.35 * io_gain - 0.65 * therm_penalty;
    } else {
        utility_delta = 1.10 * io_gain + 1.10 * mem_gain + 0.50 * cpu_gain - 0.60 * therm_penalty;
    }

    last_utility_delta_ = utility_delta;
    observing_ = false;

    auto* stats = find_candidate(current_candidate_id_);
    if (stats != nullptr) {
        const double prev_trials = static_cast<double>(std::max(1u, stats->trials) - 1u);
        stats->mean_utility_delta =
            (stats->mean_utility_delta * prev_trials + utility_delta) / static_cast<double>( std::max(1u, stats->trials));
    }

    if (utility_delta >= 0.25) {
        // IMPROVED: Measurable benefit verified -> Retain configuration!
        last_verdict_ = "IMPROVED";
        last_verdict_enum_ = TunerVerdict::Improved;
        ++total_improved_;
        if (stats != nullptr) {
            ++stats->improved_count;
            stats->successes = stats->improved_count;
            stats->mean_utility = std::clamp(
                stats->mean_utility + 0.30 * (std::min(2.5, utility_delta) - stats->mean_utility),
                0.05,
                2.50);
        }
        current_rationale_ += " [Verified IMPROVED: ΔU=+" +
                              std::to_string(utility_delta).substr(0, 4) +
                              ", io_psi " + std::to_string(before_io_psi_).substr(0, 4) +
                              "% -> " + std::to_string(after_io).substr(0, 4) + "% -> RETAINED]";
        update_ucb_scores();
    } else if (utility_delta < -0.20) {
        // REGRESSED: Stalls or thermals worsened -> Rollback & penalize bandit score!
        last_verdict_ = "REGRESSED";
        last_verdict_enum_ = TunerVerdict::Regressed;
        ++total_rolled_back_;
        if (stats != nullptr) {
            ++stats->regressed_count;
            stats->rollbacks = stats->regressed_count;
            stats->mean_utility = std::max(0.05, stats->mean_utility - 0.35);
            stats->cooldown_ticks = 8;
        }
        const std::string msg =
            "Trial " + current_candidate_id_ + " regressed (ΔU=" +
            std::to_string(utility_delta).substr(0, 5) +
            ", io_psi " + std::to_string(before_io_psi_).substr(0, 4) +
            "% -> " + std::to_string(after_io).substr(0, 4) +
            "%); rolled back to OEM baseline.";
        rollback_active(msg);
    } else {
        // NEUTRAL: No statistically meaningful benefit -> Rollback per Invariant 8
        last_verdict_ = "NEUTRAL";
        last_verdict_enum_ = TunerVerdict::Neutral;
        ++total_rolled_back_;
        if (stats != nullptr) {
            stats->cooldown_ticks = 4;
        }
        const std::string msg =
            "Trial " + current_candidate_id_ + " neutral (ΔU=" +
            std::to_string(utility_delta).substr(0, 4) +
            "); reverted to OEM baseline per Invariant 8 (no unverified retention).";
        rollback_active(msg);
    }
}

AutonomousDecisionSnapshot AutonomousTuner::step(const TunerTelemetryInput& telemetry) {
    // Decrement candidate cooldowns
    for (auto& c : candidates_) {
        if (c.cooldown_ticks > 0) --c.cooldown_ticks;
    }

    // Safety Gate: ThermalGuard immediately rolls back non-stabilizing tuning
    if (telemetry.thermal_guard_active || telemetry.thermal_emergency || telemetry.thermal_pressure >= 88.0) {
        if (transaction_active_) {
            last_verdict_ = "ABORTED";
            last_verdict_enum_ = TunerVerdict::Aborted;
            ++total_rolled_back_;
            rollback_active("Thermal safety gate active; restored OEM block/VM baseline.");
        }
        return snapshot();
    }

    // Stage 1: If currently observing an applied transaction, collect After-Window samples
    if (observing_) {
        sum_after_io_psi_ += telemetry.io_psi;
        sum_after_mem_psi_ += telemetry.memory_psi;
        sum_after_cpu_psi_ += telemetry.cpu_psi;
        sum_after_thermal_ += telemetry.thermal_pressure;
        ++obs_ticks_;
        if (obs_ticks_ >= obs_window_) {
            evaluate_observation_window();
        }
        refresh_surface_readings();
        return snapshot();
    }

    const bool io_bottleneck = telemetry.io_psi >= 2.2;
    const bool mem_bottleneck = telemetry.memory_psi >= 2.0;
    const bool cpu_bottleneck =
        telemetry.cpu_psi >= 8.0 && telemetry.thermal_pressure < 72.0 && !cpufreq_governor_dirs_.empty();

    // Stage 2: If a transaction is currently RETAINED, verify if it is still needed
    if (transaction_active_) {
        if (!io_bottleneck && !mem_bottleneck && telemetry.cpu_psi < 4.0) {
            if (++idle_streak_ >= 6) {
                idle_streak_ = 0;
                last_verdict_ = "HOLD";
                last_verdict_enum_ = TunerVerdict::Hold;
                rollback_active("Workload returned to sustained idle; restored OEM baseline.");
            }
        } else {
            idle_streak_ = 0;
            // If pressure severely worsened while retained, re-evaluate
            if (telemetry.io_psi > before_io_psi_ + 4.0) {
                if (auto* stats = find_candidate(current_candidate_id_)) {
                    stats->cooldown_ticks = 6;
                }
                rollback_active("Workload regime shifted; releasing retained candidate to re-plan.");
            }
        }
        refresh_surface_readings();
        return snapshot();
    }

    // Stage 3: No active transaction — diagnose if the device genuinely needs a mutation
    if (!io_bottleneck && !mem_bottleneck && !cpu_bottleneck) {
        refresh_surface_readings();
        return snapshot();
    }

    // Priority A: I/O Latency & Queue Bottleneck -> Select highest-UCB1 eligible I/O candidate
    if (io_bottleneck && !block_queues_.empty()) {
        const auto& primary_q = block_queues_.front();
        const double total_n = std::max(1.0, static_cast<double>(total_trials_));

        auto score_candidate = [&](std::string_view id) -> double {
            const auto* s = find_candidate(id);
            if (s == nullptr || s->cooldown_ticks > 0) return -1.0;
            // Upper Confidence Bound (UCB1): balances empirical benefit with exploration
            const double explore = 0.25 * std::sqrt(std::log(total_n + 1.0) / static_cast<double>(s->trials + 1));
            return s->mean_utility + explore;
        };

        std::string best_id;
        double best_score = -1.0;

        if (contains_token(primary_q.available_schedulers, "mq-deadline")) {
            const double sc = score_candidate("io.mq_deadline_low_latency");
            if (sc > best_score) {
                best_score = sc;
                best_id = "io.mq_deadline_low_latency";
            }
        }
        if (contains_token(primary_q.available_schedulers, "kyber")) {
            // Prefer kyber when both CPU and I/O concurrency are high or when UCB1 selects it
            const double bonus = (telemetry.io_psi >= 5.0 && telemetry.cpu_psi >= 10.0) ? 0.15 : 0.0;
            const double sc = score_candidate("io.kyber_token_bucket") + bonus;
            if (sc > best_score) {
                best_score = sc;
                best_id = "io.kyber_token_bucket";
            }
        }
        if (contains_token(primary_q.available_schedulers, "bfq")) {
            const double sc = score_candidate("io.bfq_interactive_fairness");
            if (sc > best_score) {
                best_score = sc;
                best_id = "io.bfq_interactive_fairness";
            }
        }
        if (best_id.empty()) {
            const double sc = score_candidate("io.queue_readahead_pacing");
            if (sc > best_score) {
                best_score = sc;
                best_id = "io.queue_readahead_pacing";
            }
        }

        if (best_id == "io.mq_deadline_low_latency") {
            std::vector<MutationStep> steps;
            for (const auto& q : block_queues_) {
                if (!contains_token(q.available_schedulers, "mq-deadline")) continue;
                steps.push_back({q.queue_dir + "/scheduler", "mq-deadline", "Anchor (Required-before)"});
                steps.push_back({q.queue_dir + "/iosched/read_expire", "250", "Coupled-with"});
                steps.push_back({q.queue_dir + "/iosched/writes_starved", "3", "Coupled-with"});
                steps.push_back({q.queue_dir + "/iosched/fifo_batch", "8", "Coupled-with"});
                steps.push_back({q.queue_dir + "/rq_affinity", "2", "Coupled-with"});
                steps.push_back({q.queue_dir + "/read_ahead_kb", "256", "Coupled-with"});
            }
            (void)apply_transaction(
                "io.mq_deadline_low_latency",
                "mq-deadline + Low-Latency Read Expiry (250ms / batch 8)",
                "IoLatencyRelief",
                "Measured I/O PSI stall (" + std::to_string(telemetry.io_psi).substr(0, 4) +
                    "%); switching scheduler from " + primary_q.active_scheduler +
                    " to mq-deadline with 250ms read expiry & fifo_batch=8 to prevent write starvation.",
                steps,
                telemetry);
            return snapshot();
        }

        if (best_id == "io.kyber_token_bucket") {
            std::vector<MutationStep> steps;
            for (const auto& q : block_queues_) {
                if (!contains_token(q.available_schedulers, "kyber")) continue;
                steps.push_back({q.queue_dir + "/scheduler", "kyber", "Anchor (Required-before)"});
                steps.push_back({q.queue_dir + "/nr_requests", "128", "Coupled-with"});
                steps.push_back({q.queue_dir + "/read_ahead_kb", "128", "Coupled-with"});
                steps.push_back({q.queue_dir + "/rq_affinity", "2", "Coupled-with"});
            }
            (void)apply_transaction(
                "io.kyber_token_bucket",
                "kyber Token-Bucket Low-Latency I/O Scheduler",
                "IoLatencyRelief",
                "Concurrent I/O (" + std::to_string(telemetry.io_psi).substr(0, 4) +
                    "%) & CPU load selected via UCB1 bandit; switching scheduler from " +
                    primary_q.active_scheduler + " to kyber token-bucket arbitration.",
                steps,
                telemetry);
            return snapshot();
        }

        if (best_id == "io.bfq_interactive_fairness") {
            std::vector<MutationStep> steps;
            for (const auto& q : block_queues_) {
                if (!contains_token(q.available_schedulers, "bfq")) continue;
                steps.push_back({q.queue_dir + "/scheduler", "bfq", "Anchor (Required-before)"});
                steps.push_back({q.queue_dir + "/read_ahead_kb", "256", "Coupled-with"});
            }
            (void)apply_transaction(
                "io.bfq_interactive_fairness",
                "bfq Proportional-Share Interactive I/O Scheduler",
                "IoLatencyRelief",
                "Sustained I/O contention (" + std::to_string(telemetry.io_psi).substr(0, 4) +
                    "%); evaluating bfq proportional-share scheduler against OEM baseline.",
                steps,
                telemetry);
            return snapshot();
        }

        if (best_id == "io.queue_readahead_pacing") {
            std::vector<MutationStep> steps;
            for (const auto& q : block_queues_) {
                steps.push_back({q.queue_dir + "/read_ahead_kb", "256", "Anchor (Required-before)"});
                steps.push_back({q.queue_dir + "/nr_requests", "128", "Coupled-with"});
            }
            (void)apply_transaction(
                "io.queue_readahead_pacing",
                "Adaptive Block Queue Read-Ahead & Request Depth (256KB / 128 req)",
                "IoLatencyRelief",
                "I/O stall pressure elevated; tuning block queue read_ahead_kb=256 and nr_requests=128.",
                steps,
                telemetry);
            return snapshot();
        }
    }

    // Priority B: Memory PSI Contention -> VM Subsystem Relief Transaction
    if (mem_bottleneck) {
        const auto* vm_cand = find_candidate("vm.zram_latency_relief");
        if (vm_cand != nullptr && vm_cand->cooldown_ticks == 0) {
            std::vector<MutationStep> steps{
                {proc_root_ + "/sys/vm/page-cluster", "0", "Anchor (Required-before)"},
                {proc_root_ + "/sys/vm/vfs_cache_pressure", "80", "Coupled-with"},
            };
            (void)apply_transaction(
                "vm.zram_latency_relief",
                "VM Page-Cluster Zero-Latency & VFS Dentry Preservation",
                "MemoryStallRelief",
                "Memory PSI stall (" + std::to_string(telemetry.memory_psi).substr(0, 4) +
                    "%) detected; setting vm.page-cluster=0 and vfs_cache_pressure=80 to reduce swap latency.",
                steps,
                telemetry);
            return snapshot();
        }
    }

    // Priority C: Interactive CPU PSI Bottleneck -> Smart CPUFreq Governor Rate-Limit Pacing
    if (cpu_bottleneck) {
        const auto* cpu_cand = find_candidate("cpufreq.schedutil_interactive_response");
        if (cpu_cand != nullptr && cpu_cand->cooldown_ticks == 0) {
            std::vector<MutationStep> steps;
            for (const auto& gdir : cpufreq_governor_dirs_) {
                steps.push_back({gdir + "/up_rate_limit_us", "500", "Anchor (Required-before)"});
                steps.push_back({gdir + "/down_rate_limit_us", "10000", "Coupled-with"});
            }
            (void)apply_transaction(
                "cpufreq.schedutil_interactive_response",
                "EAS Governor Rate-Limit Pacing (500us up / 10000us down)",
                "InteractiveCpuPacing",
                "CPU PSI stall (" + std::to_string(telemetry.cpu_psi).substr(0, 4) +
                    "%) with thermal headroom; pacing EAS governor rate limits (up=500us, down=10000us).",
                steps,
                telemetry);
        }
    }

    return snapshot();
}

AutonomousDecisionSnapshot AutonomousTuner::snapshot() const {
    AutonomousDecisionSnapshot out{};
    out.stage = observing_ ? "OBSERVING" : (transaction_active_ ? "RETAINED" : "PLANNED");
    out.active_intent = current_intent_;
    out.intent = current_intent_;
    out.active_candidate_id = current_candidate_id_;
    out.active_candidate_label = current_candidate_label_;
    out.decision_rationale = current_rationale_;
    out.rationale = current_rationale_;
    out.last_verdict = last_verdict_;
    out.verdict = verdict_from_string(last_verdict_);
    out.last_utility_delta = last_utility_delta_;
    out.utility_delta = last_utility_delta_;
    out.before_io_psi = before_io_psi_;
    out.after_io_psi = obs_ticks_ > 0 ? (sum_after_io_psi_ / static_cast<double>(obs_ticks_)) : before_io_psi_;
    out.before_mem_psi = before_mem_psi_;
    out.after_mem_psi = obs_ticks_ > 0 ? (sum_after_mem_psi_ / static_cast<double>(obs_ticks_)) : before_mem_psi_;
    out.before_cpu_psi = before_cpu_psi_;
    out.after_cpu_psi = obs_ticks_ > 0 ? (sum_after_cpu_psi_ / static_cast<double>(obs_ticks_)) : before_cpu_psi_;
    out.observation_tick = obs_ticks_;
    out.observation_window = obs_window_;
    out.total_trials = total_trials_;
    out.total_improved = total_improved_;
    out.improved_count = total_improved_;
    out.total_rolled_back = total_rolled_back_;
    out.rolled_back_count = total_rolled_back_;
    out.block_queues = block_queues_;
    out.candidates = candidates_;
    out.vm_page_cluster = read_first_line(proc_root_ + "/sys/vm/page-cluster");
    out.vm_vfs_cache_pressure = read_first_line(proc_root_ + "/sys/vm/vfs_cache_pressure");
    return out;
}

} // namespace coreflow
