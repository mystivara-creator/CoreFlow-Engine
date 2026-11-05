/**
 * @file autonomous_tuner.hpp
 * @brief Self-Directed Multi-Domain Transaction Planner & Empirical Outcome Evaluator
 * @license MIT
 *
 * Extends CFAE beyond UClamp to autonomously discover, plan, apply, and empirically
 * evaluate coordinated kernel transactions across:
 *   1. Block I/O Schedulers (mq-deadline, kyber, bfq, none) & iosched/<tunable> queue tunables
 *   2. VM / Memory Subsystem (/proc/sys/vm/page-cluster, vfs_cache_pressure, watermark_scale_factor)
 *   3. CPUFreq Governor & rate-limit tunables (schedutil/walt up_rate_limit_us / down_rate_limit_us)
 *
 * Every mutation requires a measured system bottleneck (PSI/thermal evidence),
 * respects dependency provenance (Required-before / Coupled-with), executes via
 * BaselineStore write-ahead journal + readback verification, and is evaluated over
 * a Before/After observation window. Beneficial changes (IMPROVED) are retained;
 * NEUTRAL or REGRESSED trials are rolled back in reverse dependency order and
 * learned via an online Upper Confidence Bound (UCB1) bandit.
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace coreflow {

enum class TunerVerdict : uint8_t {
    Pending = 0,
    Hold = 1,
    Improved = 2,
    Neutral = 3,
    Regressed = 4,
    Aborted = 5,
    OwnershipLost = 6,
};

struct BlockQueueSurface {
    std::string device_name;            // e.g. "sda", "mmcblk0"
    std::string queue_dir;              // e.g. "/sys/block/sda/queue"
    std::string baseline_scheduler;     // e.g. "mq-deadline" or "none"
    std::string active_scheduler;       // current active token
    std::vector<std::string> available_schedulers;
    uint64_t baseline_read_ahead_kb{128};
    uint64_t active_read_ahead_kb{128};
    uint64_t read_ahead_kb{128};
    uint64_t baseline_nr_requests{128};
    uint64_t active_nr_requests{128};
    uint64_t nr_requests{128};
    uint64_t rq_affinity{1};
    uint64_t read_expire_ms{500};
    uint64_t writes_starved{2};
    uint64_t fifo_batch{16};
    bool has_iosched_tunables{false};
};

struct CandidateStats {
    std::string id;
    std::string domain;                 // "io", "vm", "cpufreq"
    std::string label;
    double mean_utility{0.5};           // Empirical utility score Q_k
    double mean_utility_delta{0.0};     // Empirical mean ΔU
    double ucb_score{0.65};             // Upper Confidence Bound (UCB1) score
    uint32_t trials{0};
    uint32_t improved_count{0};
    uint32_t successes{0};
    uint32_t regressed_count{0};
    uint32_t rollbacks{0};
    uint32_t cooldown_ticks{0};
};

struct AutonomousDecisionSnapshot {
    std::string stage{"RETAINED"};      // PLANNED, OBSERVING, RETAINED, ROLLED_BACK
    std::string active_intent{"HoldEquilibrium"};
    std::string intent{"HoldEquilibrium"};
    std::string active_candidate_id{"baseline"};
    std::string active_candidate_label{"OEM Verified Baseline"};
    std::string decision_rationale{"System in low-pressure equilibrium; no mutation required."};
    std::string rationale{"System in low-pressure equilibrium; no mutation required."};
    std::string last_verdict{"IMPROVED"}; // IMPROVED, NEUTRAL, REGRESSED, OWNERSHIP_LOST, HOLD, ABORTED
    TunerVerdict verdict{TunerVerdict::Improved};
    double last_utility_delta{0.0};
    double utility_delta{0.0};
    double before_io_psi{0.0};
    double after_io_psi{0.0};
    double before_mem_psi{0.0};
    double after_mem_psi{0.0};
    double before_cpu_psi{0.0};
    double after_cpu_psi{0.0};
    uint32_t observation_tick{0};
    uint32_t observation_window{3};
    uint32_t total_trials{0};
    uint32_t total_improved{0};
    uint32_t improved_count{0};
    uint32_t total_rolled_back{0};
    uint32_t rolled_back_count{0};
    std::vector<BlockQueueSurface> block_queues;
    std::vector<CandidateStats> candidates;
    std::string vm_page_cluster;
    std::string vm_vfs_cache_pressure;
};

struct TunerTelemetryInput {
    double cpu_psi{0.0};
    double memory_psi{0.0};
    double io_psi{0.0};
    double thermal_pressure{45.0};
    bool thermal_guard_active{false};
    bool thermal_emergency{false};
};

class AutonomousTuner {
public:
    static AutonomousTuner& instance();

    explicit AutonomousTuner(std::string sys_root = "/sys", std::string proc_root = "/proc");

    void discover(std::string_view sys_root = "/sys", std::string_view proc_root = "/proc");

    // Advances the autonomous reasoning, transaction coordinator, and before/after outcome evaluator.
    AutonomousDecisionSnapshot step(const TunerTelemetryInput& telemetry);

    // Forces an immediate, dependency-ordered rollback of any active tuner transaction.
    void rollback_active(std::string_view reason = "Operator or thermal rollback");

    // Resets all active mutations, bandit statistics, and observation state.
    void reset();

    [[nodiscard]] AutonomousDecisionSnapshot snapshot() const;

    static constexpr std::string_view stage_name(std::string_view stage) noexcept {
        return stage.empty() ? "PLANNED" : stage;
    }

    static constexpr std::string_view intent_name(std::string_view intent) noexcept {
        return intent.empty() ? "HoldEquilibrium" : intent;
    }

    static constexpr std::string_view verdict_name(std::string_view verdict) noexcept {
        return verdict.empty() ? "HOLD" : verdict;
    }

    static constexpr std::string_view verdict_name(TunerVerdict verdict) noexcept {
        switch (verdict) {
            case TunerVerdict::Pending: return "PENDING";
            case TunerVerdict::Hold: return "HOLD";
            case TunerVerdict::Improved: return "IMPROVED";
            case TunerVerdict::Neutral: return "NEUTRAL";
            case TunerVerdict::Regressed: return "REGRESSED";
            case TunerVerdict::Aborted: return "ABORTED";
            case TunerVerdict::OwnershipLost: return "OWNERSHIP_LOST";
        }
        return "HOLD";
    }

private:
    struct MutationStep {
        std::string path;
        std::string target_value;
        std::string relationship; // "Anchor" (Required-before) or "Coupled-with"
    };

    std::string sys_root_;
    std::string proc_root_;
    std::vector<BlockQueueSurface> block_queues_;
    std::vector<std::string> cpufreq_governor_dirs_;
    std::vector<CandidateStats> candidates_;

    bool transaction_active_{false};
    bool observing_{false};
    std::string current_candidate_id_{"baseline"};
    std::string current_candidate_label_{"OEM Verified Baseline"};
    std::string current_intent_{"HoldEquilibrium"};
    std::string current_rationale_{"System in low-pressure equilibrium; no mutation required."};
    std::string last_verdict_{"HOLD"};
    TunerVerdict last_verdict_enum_{TunerVerdict::Hold};
    double last_utility_delta_{0.0};

    // Before/After evaluation window state
    double before_io_psi_{0.0};
    double before_mem_psi_{0.0};
    double before_cpu_psi_{0.0};
    double before_thermal_{0.0};
    double sum_after_io_psi_{0.0};
    double sum_after_mem_psi_{0.0};
    double sum_after_cpu_psi_{0.0};
    double sum_after_thermal_{0.0};
    uint32_t obs_ticks_{0};
    uint32_t obs_window_{3};
    uint32_t idle_streak_{0};

    uint32_t total_trials_{0};
    uint32_t total_improved_{0};
    uint32_t total_rolled_back_{0};

    std::vector<MutationStep> applied_steps_;

    void init_candidates();
    void update_ucb_scores();
    void refresh_surface_readings();
    bool apply_transaction(
        std::string_view candidate_id,
        std::string_view candidate_label,
        std::string_view intent,
        std::string_view rationale,
        const std::vector<MutationStep>& steps,
        const TunerTelemetryInput& telemetry);
    void evaluate_observation_window();
    CandidateStats* find_candidate(std::string_view id);
};

} // namespace coreflow
