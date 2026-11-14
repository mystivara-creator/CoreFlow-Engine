/**
 * @file autonomous_tuner.cpp
 * @brief Self-Directed Multi-Domain Transaction Planner, Central Mutation Coordinator & Empirical Outcome Evaluator
 * @license MIT
 */
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/config.hpp"
#include "coreflow/decision_engine.hpp"
#include "coreflow/foreground_task_resolver.hpp"
#include "coreflow/kernel_node_inventory.hpp"
#include "coreflow/transactional_frequency_actuator.hpp"
#include "coreflow/unique_fd.hpp"
#include "coreflow/worker_pool.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cmath>
#include <fcntl.h>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;

namespace {

std::string read_first_line(const std::string& path) {
    const auto res = KernelIo::read_first_line(path);
    return res.ok() ? res.value : std::string{};
}

bool parse_u64(const std::string& text, uint64_t& out) {
    return KernelIo::parse_u64_strict(DaemonConfig::trim(text), out);
}

std::vector<std::string> enumerate_attribute_names(const fs::path& directory,
                                                   std::size_t maximum = 128U) {
    std::vector<std::string> names;
    std::error_code ec;
    fs::directory_iterator it(directory, fs::directory_options::skip_permission_denied, ec);
    const fs::directory_iterator end;
    for (; !ec && it != end && names.size() < maximum; it.increment(ec)) {
        std::error_code status_error;
        const auto status = it->symlink_status(status_error);
        if (status_error || fs::is_symlink(status) || !fs::is_regular_file(status)) continue;
        names.push_back(it->path().filename().string());
    }
    std::sort(names.begin(), names.end());
    return names;
}

bool is_physical_block_device(std::string_view name) {
    if (name.empty()) return false;
    // Exclude virtual block devices: loop*, ram*, zram*, dm-*, md*, nbd*, ublk*, z*
    if (name.rfind("loop", 0) == 0 || name.rfind("ram", 0) == 0 ||
        name.rfind("zram", 0) == 0 || name.rfind("dm-", 0) == 0 ||
        name.rfind("md", 0) == 0 || name.rfind("nbd", 0) == 0 ||
        name.rfind("ublkb", 0) == 0 || name.rfind("zd", 0) == 0) {
        return false;
    }
    return name.rfind("sd", 0) == 0 ||
           name.rfind("mmcblk", 0) == 0 ||
           name.rfind("nvme", 0) == 0;
}

bool is_virtual_block_device(std::string_view name) {
    return name.rfind("loop", 0) == 0 || name.rfind("ram", 0) == 0 ||
           name.rfind("zram", 0) == 0 || name.rfind("dm-", 0) == 0 ||
           name.rfind("md", 0) == 0 || name.rfind("nbd", 0) == 0 ||
           name.rfind("ublkb", 0) == 0 || name.rfind("zd", 0) == 0;
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

TunerVerdict verdict_from_string(std::string_view v) noexcept {
    if (v == "IMPROVED") return TunerVerdict::Improved;
    if (v == "NEUTRAL") return TunerVerdict::Neutral;
    if (v == "REGRESSED") return TunerVerdict::Regressed;
    if (v == "ABORTED") return TunerVerdict::Aborted;
    if (v == "OWNERSHIP_LOST") return TunerVerdict::OwnershipLost;
    if (v == "INCONCLUSIVE") return TunerVerdict::Inconclusive;
    if (v == "RECOVERY_REQUIRED") return TunerVerdict::RecoveryRequired;
    if (v == "REJECTED") return TunerVerdict::Rejected;
    if (v == "PENDING") return TunerVerdict::Pending;
    return TunerVerdict::Hold;
}

std::string resolve_state_dir() {
    const char* test = std::getenv("COREFLOW_TEST_STATE_DIR");
    return (test && *test) ? std::string(test) : "/data/adb/coreflow";
}

// Open every directory component relative to the already verified parent. This
// rejects symlinked ancestors as well as a symlink at the final state directory.
UniqueFd open_state_directory(std::string_view path, bool create) noexcept {
    if (path.empty() || path.front() != '/') return {};
    UniqueFd current(::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC));
    if (!current.valid()) return {};
    size_t pos = 1;
    while (pos < path.size()) {
        while (pos < path.size() && path[pos] == '/') ++pos;
        if (pos >= path.size()) break;
        const size_t end = path.find('/', pos);
        const size_t count = (end == std::string_view::npos ? path.size() : end) - pos;
        const std::string component(path.substr(pos, count));
        pos = (end == std::string_view::npos) ? path.size() : end + 1;
        if (component.empty() || component == ".") continue;
        if (component == "..") return {};
        int next = ::openat(current.get(), component.c_str(),
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (next < 0 && errno == ENOENT && create) {
            if (::mkdirat(current.get(), component.c_str(), 0700) != 0 && errno != EEXIST) return {};
            next = ::openat(current.get(), component.c_str(),
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        }
        if (next < 0) return {};
        struct stat st{};
        if (::fstat(next, &st) != 0 || !S_ISDIR(st.st_mode)) {
            (void)::close(next);
            return {};
        }
        current.reset(next);
    }
    return current;
}

bool write_all(int fd, std::string_view data) noexcept {
    return KernelIo::write_all_fd(fd, data);
}

bool read_bounded_regular_file(int fd, std::string& output, size_t max_bytes) {
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 ||
        static_cast<uint64_t>(st.st_size) > max_bytes) return false;
    output.clear();
    output.reserve(static_cast<size_t>(st.st_size));
    char buffer[4096];
    for (;;) {
        const ssize_t n = ::read(fd, buffer, sizeof(buffer));
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) return false;
        if (n == 0) return true;
        const size_t count = static_cast<size_t>(n);
        if (output.size() > max_bytes - count) return false;
        output.append(buffer, count);
    }
}

void remove_persisted_tuner_state() noexcept {
    UniqueFd dir = open_state_directory(resolve_state_dir(), false);
    if (!dir.valid()) return;
    if (::unlinkat(dir.get(), "tuner_stats.state", 0) == 0) (void)::fsync(dir.get());
}

bool check_zram_swap_active(const std::string& proc_root, const std::string& sys_root) {
    const fs::path swaps_path = fs::path(proc_root) / "swaps";
    const auto read_res = KernelIo::read_bounded_file(swaps_path.string(), 64U * 1024U);
    if (read_res.ok()) {
        return read_res.value.find("zram") != std::string::npos;
    }
    if (read_res.status != KernelIoStatus::NotFound) {
        return false;
    }
    std::error_code ec;
    return fs::exists(fs::path(sys_root) / "block/zram0", ec) && !ec;
}

std::string sanitize_tsv_field(std::string_view input) {
    if (input.empty()) return "-";
    std::string out;
    out.reserve(input.size());
    for (char ch : input) {
        if (ch == '\t' || ch == '\n' || ch == '\r' || ch == ' ') {
            out.push_back('_');
        } else {
            out.push_back(ch);
        }
    }
    return out.empty() ? "-" : out;
}


uint64_t tuner_state_checksum(std::string_view data) noexcept {
    uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char byte : data) {
        hash ^= static_cast<uint64_t>(byte);
        hash *= 1099511628211ULL;
    }
    return hash;
}

void record_ownership_loss(CandidateStats* stats) {
    if (stats == nullptr) return;

    // Escalate quarantine after repeated OEM/external takeovers: 8, 16, 32, then
    // 64 monitor ticks (bounded to avoid permanent suppression from stale state).
    stats->ownership_loss_count = std::min<uint32_t>(stats->ownership_loss_count, 15U) + 1U;
    const uint32_t exponent = std::min<uint32_t>(stats->ownership_loss_count - 1U, 3U);
    const uint32_t backoff_ticks = std::min<uint32_t>(64U, 8U << exponent);
    stats->ownership_quarantine_ticks = std::max(stats->ownership_quarantine_ticks, backoff_ticks);
    stats->cooldown_ticks = std::max(stats->cooldown_ticks, backoff_ticks);
    stats->last_rollback_reason =
        "External ownership lost; candidate quarantined for " + std::to_string(backoff_ticks) + " ticks";
}

} // namespace

// ============================================================================
// CentralMutationCoordinator Implementation
// ============================================================================

// AutonomousTuner Implementation
// ============================================================================

AutonomousTuner::AutonomousTuner(CentralMutationCoordinator& coordinator,
                                 std::string sys_root,
                                 std::string proc_root,
                                 DeviceProfileHandle profile)
    : coordinator_(coordinator),
      controller_name_(CentralMutationCoordinator::make_controller_identity("AutonomousTuner")),
      sys_root_(std::move(sys_root)),
      proc_root_(std::move(proc_root)),
      device_profile_(std::move(profile)),
      worker_pool_(std::make_unique<BoundedWorkerPool>(2U, 32U, "coreflow-tuner")),
      decision_engine_(std::make_unique<GlobalDecisionEngine>()) {
    (void)coordinator_.register_preemption_handler(
        controller_name_,
        [this](MutationDomain domain, std::string_view transaction_id,
               std::string_view reason) noexcept {
            return handle_preemption(domain, transaction_id, reason);
        });
    init_candidates();
    discover(sys_root_, proc_root_);
}

AutonomousTuner::~AutonomousTuner() {
    {
        std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
        if (foreground_task_resolver_) {
            (void)foreground_task_resolver_->restore_all();
            foreground_task_resolver_ = nullptr;
        }
        if (!active_interventions_.empty() || !applied_steps_.empty() ||
            !unresolved_recoveries_.empty() ||
            (frequency_actuator_ &&
             (frequency_actuator_->mutation_active() || frequency_actuator_->recovery_required()))) {
            (void)rollback_active("Autonomous tuner teardown.");
        }
    }
    coordinator_.unregister_preemption_handler(controller_name_);
    if (worker_pool_) {
        worker_pool_->shutdown();
    }
}

void AutonomousTuner::set_device_profile(DeviceProfileHandle profile) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    if (!active_interventions_.empty() || !applied_steps_.empty() ||
        !unresolved_recoveries_.empty() || recovery_required_) {
        return;
    }
    device_profile_ = std::move(profile);
    discover(sys_root_, proc_root_);
}

void AutonomousTuner::configure_frequency_actuator(
    TransactionalFrequencyActuator* actuator,
    const TieredPolicyRouter* router,
    const PlatformContextSnapshot& platform) noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    if (frequency_mutation_active_ ||
        (frequency_actuator_ != nullptr && frequency_actuator_->mutation_active())) return;
    frequency_actuator_ = actuator;
    tiered_policy_router_ = router;
    platform_context_ = platform;
}

bool AutonomousTuner::frequency_mutation_active() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    return frequency_mutation_active_ && frequency_actuator_ != nullptr &&
           frequency_actuator_->mutation_active();
}

bool AutonomousTuner::frequency_actuator_configured() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    return frequency_actuator_ != nullptr && tiered_policy_router_ != nullptr;
}

std::optional<uint32_t> AutonomousTuner::select_frequency_policy_index() const noexcept {
    if (!device_profile_ || device_profile_->cpu_policies.empty()) return std::nullopt;
    uint64_t best_capacity = 0U;
    std::optional<uint32_t> selected;
    for (const auto& policy : device_profile_->cpu_policies) {
        if (policy.policy_path.empty() || !policy.related_cpus_valid ||
            policy.related_cpus.empty() || !policy.frequency_table_valid ||
            policy.cpuinfo_max_freq_khz == 0U) continue;
        const std::string name = fs::path(policy.policy_path).filename().string();
        if (name.rfind("policy", 0) != 0 || name.size() <= 6U) continue;
        uint32_t index = 0U;
        const std::string_view suffix(name.data() + 6U, name.size() - 6U);
        const auto parsed = std::from_chars(
            suffix.data(), suffix.data() + suffix.size(), index, 10);
        if (parsed.ec != std::errc{} || parsed.ptr != suffix.data() + suffix.size() ||
            index > 255U) continue;
        if (!selected.has_value() || policy.cpuinfo_max_freq_khz > best_capacity) {
            best_capacity = policy.cpuinfo_max_freq_khz;
            selected = index;
        }
    }
    return selected;
}

bool AutonomousTuner::handle_preemption(
    MutationDomain domain, std::string_view transaction_id, std::string_view reason) noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    // 1. Retry any unresolved recovery record matching this exact domain & lease.
    for (size_t idx = 0; idx < unresolved_recoveries_.size(); ++idx) {
        if (unresolved_recoveries_[idx].domain == domain &&
            unresolved_recoveries_[idx].candidate_id == transaction_id) {
            const bool restored = retry_unresolved_recoveries();
            if (!restored) {
                last_verdict_ = "RECOVERY_REQUIRED";
                last_verdict_enum_ = TunerVerdict::RecoveryRequired;
            }
            sync_primary_state_from_active();
            save_persisted_stats();
            return restored && std::none_of(
                unresolved_recoveries_.begin(), unresolved_recoveries_.end(),
                [&](const FailedRecoveryRecord& rec) {
                    return rec.domain == domain && rec.candidate_id == transaction_id;
                });
        }
    }

    // 2. Locate exact active intervention owning (domain, transaction_id).
    for (size_t idx = 0; idx < active_interventions_.size(); ++idx) {
        auto& iv = active_interventions_[idx];
        if (iv.mutation_domain != domain || iv.candidate_id != transaction_id) {
            continue;
        }
        const bool had_applied_steps = !iv.applied_steps.empty() || iv.is_frequency_policy;
        last_verdict_ = "ABORTED";
        last_verdict_enum_ = TunerVerdict::Aborted;
        if (auto* stats = find_candidate(iv.candidate_id)) ++stats->preempted_count;
        if (had_applied_steps) ++total_rolled_back_;
        const bool restored = rollback_intervention(iv, reason);
        if (restored) {
            active_interventions_.erase(active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx));
        } else {
            active_interventions_.erase(active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx));
            last_verdict_ = "RECOVERY_REQUIRED";
            last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        }
        sync_primary_state_from_active();
        save_persisted_stats();
        return restored;
    }

    // 3. Handle preemption during an in-flight apply_transaction() before OBSERVING starts.
    if (current_candidate_id_ == transaction_id &&
        domain_for_candidate(current_candidate_id_) == domain) {
        const bool had_applied_steps = !applied_steps_.empty();
        last_verdict_ = "ABORTED";
        last_verdict_enum_ = TunerVerdict::Aborted;
        if (auto* stats = find_candidate(current_candidate_id_)) ++stats->preempted_count;
        if (had_applied_steps) ++total_rolled_back_;
        bool restored = true;
        if (active_interventions_.empty()) {
            restored = rollback_active(reason);
        } else {
            restored = rollback_partial_steps(domain, transaction_id, applied_steps_, reason);
            sync_primary_state_from_active();
        }
        if (!restored) {
            last_verdict_ = "RECOVERY_REQUIRED";
            last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        }
        save_persisted_stats();
        return restored;
    }

    return false;
}

void AutonomousTuner::init_candidates() {
    candidates_ = {
        CandidateStats{
            .id = "io.mq_deadline_low_latency",
            .domain = "io",
            .label = "mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)",
            .mean_utility = 0.65,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.86,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "io.kyber_token_bucket",
            .domain = "io",
            .label = "kyber Token-Bucket Low-Latency I/O Scheduler",
            .mean_utility = 0.62,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.83,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "io.bfq_interactive_fairness",
            .domain = "io",
            .label = "bfq Proportional-Share Interactive I/O Scheduler",
            .mean_utility = 0.55,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.76,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "io.queue_readahead_pacing",
            .domain = "io",
            .label = "Adaptive Block Queue Read-Ahead & Request Depth (2.0x OEM baseline)",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.71,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "vm.zram_latency_relief",
            .domain = "vm",
            .label = "VM Page-Cluster Zero-Latency & VFS Dentry Preservation",
            .mean_utility = 0.60,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.81,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "cpufreq.schedutil_interactive_response",
            .domain = "cpufreq",
            .label = "EAS Governor Rate-Limit Pacing (500us up / 10000us down)",
            .mean_utility = 0.58,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.79,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "thermal.schedutil_damped_pacing",
            .domain = "thermal",
            .label = "Thermal Stabilization Governor Damping (2.0x up / 0.5x down rate-limit)",
            .mean_utility = 0.64,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.85,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "thermal.io_burst_pacing",
            .domain = "thermal",
            .label = "Thermal Stabilization Block Queue Burst Pacing (0.5x read-ahead / 0.75x depth)",
            .mean_utility = 0.59,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.80,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "cpu.frequency.tiered",
            .domain = "cpufreq",
            .label = "Tiered CPU Frequency OPP",
            .mean_utility = 0.62,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.84,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
    };
    update_ucb_scores();
}

void AutonomousTuner::update_ucb_scores() {
    const double total_n = std::max(1.0, static_cast<double>(total_reward_samples_));
    for (auto& c : candidates_) {
        c.successes = c.improved_count;
        c.rollbacks = c.regressed_count;
        const double explore = 0.25 * std::sqrt(std::log(total_n + 1.0) / static_cast<double>(c.reward_samples + 1));
        c.ucb_score = c.mean_utility + explore;
    }
}

void AutonomousTuner::reset() {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    // rollback_active releases the lease only after verified recovery. Do not
    // clear ownership here when rollback failed; recovery must remain fail-closed.
    if (!rollback_active("Autonomous tuner reset to verified OEM baseline.")) {
        return; // Preserve failed steps, recovery state, and the owning lease.
    }
    thermal_stabilization_active_ = false;
    thermal_recovery_ticks_ = 0;
    has_prev_thermal_ = false;
    noise_floor_ = 0.05;
    evaluation_noise_floors_.fill(0.05);
    current_evaluation_domain_ = EvaluationDomain::Unavailable;
    settling_ticks_remaining_ = 0;
    contention_backoff_ticks_ = 0;
    total_contention_retries_ = 0;
    applied_steps_.clear();
    active_interventions_.clear();
    unresolved_recoveries_.clear();
    last_evaluated_candidates_.clear();
    last_decision_plan_rationale_.clear();
    total_trials_ = 0;
    total_reward_samples_ = 0;
    total_improved_ = 0;
    init_candidates();
    total_rolled_back_ = 0;
    total_inconclusive_ = 0;
    total_rejected_ = 0;
    last_verdict_ = "HOLD";
    last_verdict_enum_ = TunerVerdict::Hold;
    last_utility_delta_ = 0.0;
    idle_streak_ = 0;
    remove_persisted_tuner_state();
}

CandidateStats* AutonomousTuner::find_candidate(std::string_view id) {
    for (auto& c : candidates_) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

const CandidateStats* AutonomousTuner::find_candidate(std::string_view id) const {
    for (const auto& c : candidates_) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

MutationDomain AutonomousTuner::domain_for_candidate(std::string_view candidate_id) const noexcept {
    if (candidate_id.rfind("io.", 0) == 0 || candidate_id == "thermal.io_burst_pacing") {
        return MutationDomain::BlockIo;
    }
    if (candidate_id.rfind("vm.", 0) == 0) {
        return MutationDomain::VmMemory;
    }
    if (candidate_id.rfind("cpufreq.", 0) == 0 ||
        candidate_id == "cpu.frequency.tiered" ||
        candidate_id == "thermal.schedutil_damped_pacing") {
        return MutationDomain::CpuPolicy;
    }
    return MutationDomain::ThermalCore;
}

ControllerPriority AutonomousTuner::priority_for_candidate(std::string_view candidate_id) const noexcept {
    if (candidate_id.rfind("thermal.", 0) == 0) {
        return ControllerPriority::ThermalStabilization;
    }
    if (candidate_id.rfind("vm.", 0) == 0 || candidate_id.rfind("io.", 0) == 0) {
        return ControllerPriority::PressureGuard;
    }
    return ControllerPriority::WorkloadOptimization;
}

bool AutonomousTuner::is_candidate_eligible(std::string_view id) const noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    const auto* c = find_candidate(id);
    if (c == nullptr || c->ownership_quarantine_ticks > 0 || c->cooldown_ticks > 0) return false;
    if (id == "vm.zram_latency_relief" && !zram_active_) return false;
    if (!candidate_contract_eligible(id)) return false;
    if ((id == "cpufreq.schedutil_interactive_response" || id == "thermal.schedutil_damped_pacing") &&
        cpufreq_governor_dirs_.empty()) {
        return false;
    }
    if ((id.rfind("io.", 0) == 0 || id == "thermal.io_burst_pacing") && block_queues_.empty()) {
        return false;
    }
    const MutationDomain dom = domain_for_candidate(id);
    const ControllerPriority prio = priority_for_candidate(id);
    if (coordinator_.is_domain_locked_by_other(dom, controller_name_, prio)) {
        return false;
    }
    return true;
}

bool AutonomousTuner::is_candidate_eligible_for_context(
    std::string_view id,
    WorkloadClass workload,
    uint8_t thermal_bucket) const noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    const auto* c = find_candidate(id);
    if (c == nullptr || c->ownership_quarantine_ticks > 0) return false;
    const auto regime_index = static_cast<std::size_t>(current_regime_);
    if (regime_index < c->regime_cooldown_ticks.size() &&
        c->regime_cooldown_ticks[regime_index] > 0U) return false;
    if (id == "vm.zram_latency_relief" && !zram_active_) return false;
    if (!candidate_contract_eligible(id)) return false;
    if ((id == "cpufreq.schedutil_interactive_response" || id == "thermal.schedutil_damped_pacing") &&
        cpufreq_governor_dirs_.empty()) {
        return false;
    }
    if ((id.rfind("io.", 0) == 0 || id == "thermal.io_burst_pacing") && block_queues_.empty()) {
        return false;
    }
    const MutationDomain dom = domain_for_candidate(id);
    const ControllerPriority prio = priority_for_candidate(id);
    if (coordinator_.is_domain_locked_by_other(dom, controller_name_, prio)) {
        return false;
    }
    if (c->cooldown_ticks > 0) {
        // Context-aware anti-repeat: keep candidate on cooldown in the same or adjacent context;
        // only allow early re-evaluation if cooldown is nearly expired (<= 2 ticks) AND both
        // workload class and thermal regime shifted significantly (>= 2 thermal buckets).
        const int bucket_diff = std::abs(static_cast<int>(thermal_bucket) - static_cast<int>(c->last_thermal_bucket));
        if (c->cooldown_ticks > 2 || workload == c->last_workload || bucket_diff < 2) {
            return false;
        }
    }
    return true;
}

uint8_t AutonomousTuner::compute_thermal_bucket(const TunerTelemetryInput& telemetry) noexcept {
    if (telemetry.thermal_emergency || (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 88.0)) return 4;
    if (telemetry.thermal_guard_active || (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 65.0)) return 3;
    if (!telemetry.thermal_pressure_valid) return 0;
    if (telemetry.thermal_pressure >= 52.0) return 2;
    if (telemetry.thermal_pressure >= 42.0) return 1;
    return 0;
}

WorkloadClass AutonomousTuner::classify_workload(const TunerTelemetryInput& telemetry) noexcept {
    const bool rapid_temp_rise =
        (telemetry.temperature_velocity_valid && telemetry.temperature_velocity_celsius_per_sec >= 0.8) ||
        (telemetry.thermal_trend_celsius >= 0.8);
    if (telemetry.thermal_emergency || telemetry.thermal_guard_active ||
        (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 65.0) ||
        (telemetry.thermal_headroom >= 0.0 && telemetry.thermal_headroom <= 0.20) ||
        (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 56.0 && rapid_temp_rise)) {
        return WorkloadClass::ThermalConstrained;
    }
    if (telemetry.memory_psi_valid && telemetry.io_psi_valid && telemetry.memory_psi >= 3.5 && telemetry.io_psi >= 3.0) {
        return WorkloadClass::HeavyMultitasking;
    }
    if (telemetry.memory_psi_valid && telemetry.memory_psi >= 2.0) {
        return WorkloadClass::MemoryPressure;
    }
    if (telemetry.io_psi_valid && telemetry.cpu_psi_valid && telemetry.io_psi >= 5.0 && telemetry.cpu_psi >= 10.0) {
        return WorkloadClass::HeavyMultitasking;
    }
    if (telemetry.io_psi_valid && telemetry.io_psi >= 2.2) {
        return WorkloadClass::StorageIoPressure;
    }
    if (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 16.0 && telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 50.0) {
        return WorkloadClass::Gaming;
    }
    if (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 10.0) {
        return WorkloadClass::CpuBound;
    }
    if (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 6.0) {
        return WorkloadClass::ScrollingAndAnimation;
    }
    if ((telemetry.cpu_psi_valid && telemetry.cpu_psi >= 2.5) || (telemetry.io_psi_valid && telemetry.io_psi >= 1.0)) {
        return WorkloadClass::GeneralInteraction;
    }
    if (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 1.2) {
        return WorkloadClass::BackgroundWorkload;
    }
    return WorkloadClass::Idle;
}

DynamicWorkloadRegime AutonomousTuner::classify_regime(
    const TunerTelemetryInput& telemetry, WorkloadClass workload) noexcept {
    const bool thermal_constrained = telemetry.thermal_emergency || telemetry.thermal_guard_active ||
        (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 65.0) ||
        (telemetry.thermal_headroom >= 0.0 && telemetry.thermal_headroom <= 0.20);
    if (thermal_constrained || workload == WorkloadClass::Gaming ||
        workload == WorkloadClass::CpuBound || workload == WorkloadClass::GpuBound ||
        workload == WorkloadClass::ThermalConstrained)
        return DynamicWorkloadRegime::HeavySustained;

    const double io = telemetry.io_psi_valid ? telemetry.io_psi : 0.0;
    const double memory = telemetry.memory_psi_valid ? telemetry.memory_psi : 0.0;
    if (io >= 4.0 || memory >= 3.0 || workload == WorkloadClass::StorageIoPressure)
        return DynamicWorkloadRegime::BurstLoading;
    if (io >= 1.5 || memory >= 1.5 || workload == WorkloadClass::MemoryPressure ||
        workload == WorkloadClass::HeavyMultitasking)
        return DynamicWorkloadRegime::ModerateLoad;
    if (workload == WorkloadClass::GeneralInteraction ||
        workload == WorkloadClass::ScrollingAndAnimation ||
        (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 2.5))
        return DynamicWorkloadRegime::Interactive;
    if (workload == WorkloadClass::BackgroundWorkload ||
        (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 0.5))
        return DynamicWorkloadRegime::LightLoad;
    return DynamicWorkloadRegime::IdleDeep;
}

std::string_view AutonomousTuner::regime_name(DynamicWorkloadRegime regime) noexcept {
    switch (regime) {
        case DynamicWorkloadRegime::IdleDeep: return "IDLE_DEEP";
        case DynamicWorkloadRegime::LightLoad: return "LIGHT_LOAD";
        case DynamicWorkloadRegime::Interactive: return "INTERACTIVE";
        case DynamicWorkloadRegime::ModerateLoad: return "MODERATE_LOAD";
        case DynamicWorkloadRegime::BurstLoading: return "BURST_LOADING";
        case DynamicWorkloadRegime::HeavySustained: return "HEAVY_SUSTAINED";
    }
    return "IDLE_DEEP";
}

BottleneckKind AutonomousTuner::diagnose_bottleneck(
    const TunerTelemetryInput& telemetry,
    WorkloadClass workload) noexcept {
    if (telemetry.thermal_emergency || (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 88.0)) {
        return BottleneckKind::ThermalEmergency;
    }
    if (workload == WorkloadClass::ThermalConstrained) {
        return BottleneckKind::ThermalHeadroomDepletion;
    }
    if (telemetry.io_psi_valid && telemetry.cpu_psi_valid && telemetry.io_psi >= 4.5 && telemetry.cpu_psi >= 9.0) {
        return BottleneckKind::ConcurrentCpuIoContention;
    }
    const double io_sev = (telemetry.io_psi_valid && telemetry.io_psi >= 2.2)
        ? std::max(0.05, std::clamp((telemetry.io_psi - 0.8) / 14.0, 0.0, 1.0))
        : -1.0;
    const double mem_sev = (telemetry.memory_psi_valid && telemetry.memory_psi >= 2.0)
        ? std::max(0.05, std::clamp((telemetry.memory_psi - 0.8) / 14.0, 0.0, 1.0))
        : -1.0;
    const double cpu_sev = (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 8.0)
        ? std::max(0.05, std::clamp((telemetry.cpu_psi - 2.0) / 26.0, 0.0, 1.0))
        : -1.0;
    if (mem_sev >= 0.0 && mem_sev >= io_sev && mem_sev >= cpu_sev) {
        return BottleneckKind::MemorySwapThrashing;
    }
    if (cpu_sev >= 0.0 && cpu_sev >= io_sev) {
        return BottleneckKind::CpuSchedulingLatency;
    }
    if (io_sev >= 0.0) {
        return BottleneckKind::IoQueueContention;
    }
    return BottleneckKind::None;
}

OptimizationObjective AutonomousTuner::select_objective(
    const TunerTelemetryInput& telemetry,
    WorkloadClass workload,
    BottleneckKind bottleneck,
    bool in_thermal_recovery) noexcept {
    if (bottleneck == BottleneckKind::ThermalEmergency) {
        return OptimizationObjective::EmergencyMitigation;
    }
    if (bottleneck == BottleneckKind::ThermalHeadroomDepletion || workload == WorkloadClass::ThermalConstrained) {
        return OptimizationObjective::ThermalStabilization;
    }
    if (in_thermal_recovery) {
        return OptimizationObjective::ThermalRecovery;
    }
    switch (bottleneck) {
        case BottleneckKind::ConcurrentCpuIoContention:
            return OptimizationObjective::ThroughputBalance;
        case BottleneckKind::IoQueueContention:
            return OptimizationObjective::IoLatencyRelief;
        case BottleneckKind::MemorySwapThrashing:
            return OptimizationObjective::MemoryRelief;
        case BottleneckKind::CpuSchedulingLatency:
            return (workload == WorkloadClass::Gaming)
                       ? OptimizationObjective::ThroughputBalance
                       : OptimizationObjective::InteractiveLatency;
        case BottleneckKind::None:
        case BottleneckKind::ThermalHeadroomDepletion:
        case BottleneckKind::ThermalEmergency:
            break;
    }
    (void)telemetry;
    return OptimizationObjective::HoldBaseline;
}

std::string AutonomousTuner::compute_fingerprint() const {
    std::string fp = "v2|zram=" + std::string(zram_active_ ? "1" : "0");
    if (device_profile_) fp += "|cddp=" + device_profile_->fingerprint;
    for (const auto& q : block_queues_) {
        fp += "|dev=" + q.device_name + ":";
        for (const auto& s : q.available_schedulers) {
            fp += s + ",";
        }
    }
    for (const auto& g : cpufreq_governor_dirs_) {
        fp += "|gov=" + fs::path(g).filename().string();
    }
    return fp;
}

void AutonomousTuner::save_persisted_stats() const {
    std::ostringstream out;
    out << "CFTS8\t" << compute_fingerprint() << "\t"
        << total_trials_ << "\t" << total_improved_ << "\t" << total_rolled_back_ << "\t"
        << total_reward_samples_ << "\t" << total_inconclusive_ << "\t" << total_rejected_ << "\t" << noise_floor_;
    for (const double floor : evaluation_noise_floors_) out << "\t" << floor;
    out << "\n";
    for (const auto& c : candidates_) {
        out << c.id << "\t" << c.mean_utility << "\t" << c.mean_utility_delta << "\t"
            << c.trials << "\t" << c.reward_samples << "\t" << c.improved_count << "\t"
            << c.regressed_count << "\t" << c.neutral_count << "\t" << c.aborted_count << "\t"
            << c.preempted_count << "\t" << c.recovery_required_count << "\t" << c.cooldown_ticks << "\t"
            << c.consecutive_non_improved << "\t" << c.adaptive_scale << "\t"
            << c.inconclusive_count << "\t" << c.rejected_count << "\t"
            << static_cast<uint32_t>(c.last_workload) << "\t"
            << static_cast<uint32_t>(c.last_thermal_bucket) << "\t"
            << c.ownership_loss_count << "\t" << c.ownership_quarantine_ticks << "\t"
            << static_cast<uint32_t>(c.last_regime);
        for (std::size_t i = 0; i < c.regime_reward_samples.size(); ++i) {
            out << "\t" << c.regime_reward_samples[i]
                << "\t" << c.regime_mean_utility[i]
                << "\t" << c.regime_regressions[i]
                << "\t" << c.regime_cooldown_ticks[i];
        }
        out << "\t" << sanitize_tsv_field(c.last_rollback_reason) << "\n";
    }
    std::string payload = out.str();
    if (!out.good() || payload.size() > 256U * 1024U - 64U) return;
    payload += "CFTS8_END\t" + std::to_string(tuner_state_checksum(payload)) + "\n";

    UniqueFd dir = open_state_directory(resolve_state_dir(), true);
    if (!dir.valid()) return;

    static std::atomic<uint64_t> temp_nonce{0};
    std::string temp_name;
    UniqueFd temp_fd;
    for (unsigned attempt = 0; attempt < 8; ++attempt) {
        temp_name = "tuner_stats.state.tmp." + std::to_string(::getpid()) + "." +
                    std::to_string(temp_nonce.fetch_add(1, std::memory_order_relaxed));
        const int fd = ::openat(dir.get(), temp_name.c_str(),
                                O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (fd >= 0) { temp_fd.reset(fd); break; }
        if (errno != EEXIST) return;
    }
    if (!temp_fd.valid()) return;

    struct stat temp_stat{};
    if (::fstat(temp_fd.get(), &temp_stat) != 0 || !S_ISREG(temp_stat.st_mode) ||
        ::fchmod(temp_fd.get(), 0600) != 0 || !write_all(temp_fd.get(), payload) ||
        ::fsync(temp_fd.get()) != 0) {
        (void)::unlinkat(dir.get(), temp_name.c_str(), 0);
        return;
    }
    if (::renameat(dir.get(), temp_name.c_str(), dir.get(), "tuner_stats.state") != 0) {
        (void)::unlinkat(dir.get(), temp_name.c_str(), 0);
        return;
    }
    (void)::fsync(dir.get());
}

void AutonomousTuner::load_persisted_stats() {
    UniqueFd dir = open_state_directory(resolve_state_dir(), false);
    if (!dir.valid()) return;
    UniqueFd state_fd(::openat(dir.get(), "tuner_stats.state",
                               O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!state_fd.valid()) return;

    std::string payload;
    if (!read_bounded_regular_file(state_fd.get(), payload, 256U * 1024U)) return;

    bool has_checksum = false;
    std::string checksum_footer_magic;
    const size_t footer_pos8 = payload.rfind("CFTS8_END\t");
    const size_t footer_pos7 = payload.rfind("CFTS7_END\t");
    const size_t footer_pos6 = payload.rfind("CFTS6_END\t");
    if ((footer_pos8 != std::string::npos) + (footer_pos7 != std::string::npos) +
        (footer_pos6 != std::string::npos) > 1) return;
    const size_t footer_pos = footer_pos8 != std::string::npos ? footer_pos8 :
        (footer_pos7 != std::string::npos ? footer_pos7 : footer_pos6);
    if (footer_pos != std::string::npos) {
        if (footer_pos > 0 && payload[footer_pos - 1] != '\n') return;
        const std::string footer = payload.substr(footer_pos);
        std::istringstream footer_stream(footer);
        std::string extra;
        uint64_t stored_checksum = 0;
        if (!(footer_stream >> checksum_footer_magic >> stored_checksum) ||
            (checksum_footer_magic != "CFTS8_END" && checksum_footer_magic != "CFTS7_END" &&
             checksum_footer_magic != "CFTS6_END") ||
            (footer_stream >> extra)) return;
        payload.resize(footer_pos);
        if (tuner_state_checksum(payload) != stored_checksum) return;
        has_checksum = true;
    }

    std::istringstream in(payload);
    std::string header;
    if (!std::getline(in, header)) return;
    std::istringstream hs(header);
    std::string magic, fp, extra;
    uint32_t trials = 0, improved = 0, rolled_back = 0;
    if (!(hs >> magic >> fp >> trials >> improved >> rolled_back)) return;
    if ((magic == "CFTS8" && (!has_checksum || checksum_footer_magic != "CFTS8_END")) ||
        (magic == "CFTS7" && (!has_checksum || checksum_footer_magic != "CFTS7_END")) ||
        (magic == "CFTS6" && (!has_checksum || checksum_footer_magic != "CFTS6_END")) ||
        (magic != "CFTS8" && magic != "CFTS7" && magic != "CFTS6" && has_checksum)) return;

    const std::string current_fp = compute_fingerprint();
    uint32_t persisted_reward_samples = 0;
    uint32_t loaded_inconclusive = 0;
    uint32_t loaded_rejected = 0;
    double loaded_noise_floor = 0.05;
    std::array<double, 4> loaded_domain_floors{{0.05, 0.05, 0.05, 0.05}};

    if (magic == "CFTS8" || magic == "CFTS7" || magic == "CFTS6" || magic == "CFTS5" || magic == "CFTS4") {
        if (fp != current_fp ||
            !(hs >> persisted_reward_samples >> loaded_inconclusive >> loaded_rejected >> loaded_noise_floor) ||
            persisted_reward_samples > trials ||
            !std::isfinite(loaded_noise_floor) || loaded_noise_floor < 0.02 || loaded_noise_floor > 0.45) return;
        if (magic == "CFTS8" || magic == "CFTS7" || magic == "CFTS6" || magic == "CFTS5") {
            for (auto& floor : loaded_domain_floors) {
                if (!(hs >> floor) || !std::isfinite(floor) || floor < 0.02 || floor > 0.45) return;
            }
        } else {
            loaded_domain_floors.fill(loaded_noise_floor);
        }
        if (hs >> extra) return;
    } else if (magic == "CFTS3" || magic == "CFTS2") {
        if (fp != current_fp) return;
        double legacy_floor = 0.05;
        if (hs >> loaded_inconclusive >> loaded_rejected >> legacy_floor) {
            if (!std::isfinite(legacy_floor) || legacy_floor < 0.02 || legacy_floor > 0.45) return;
            loaded_noise_floor = legacy_floor;
            loaded_domain_floors.fill(legacy_floor);
        } else {
            hs.clear();
        }
        if (hs >> extra) return;
    } else if (magic == "CFTS1") {
        std::string v1_fp = current_fp;
        if (v1_fp.rfind("v2|", 0) == 0) v1_fp.replace(0, 3, "v1|");
        if (fp != v1_fp || (hs >> extra)) return;
    } else {
        return;
    }

    struct ParsedCandidate {
        std::string id;
        double mean_utility{0.5};
        double mean_utility_delta{0.0};
        double adaptive_scale{1.0};
        uint32_t trials{0}, reward_samples{0}, improved{0}, regressed{0}, neutral{0};
        uint32_t aborted{0}, preempted{0}, recovery_required{0}, cooldown{0}, consecutive_non_improved{0};
        uint32_t inconclusive{0}, rejected{0}, workload{0}, thermal_bucket{0};
        uint32_t ownership_losses{0}, quarantine_ticks{0};
        uint32_t last_regime{0};
        std::array<double, 6> regime_means{};
        std::array<uint32_t, 6> regime_samples{};
        std::array<uint32_t, 6> regime_regressions{};
        std::array<uint32_t, 6> regime_cooldowns{};
        std::string rollback_reason;
    };
    std::vector<ParsedCandidate> parsed;
    std::unordered_set<std::string> seen_ids;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line.size() > 4096 || parsed.size() >= 256) return;
        std::istringstream ls(line);
        ParsedCandidate saved;
        if (!(ls >> saved.id >> saved.mean_utility >> saved.mean_utility_delta >> saved.trials)) return;
        if (saved.id.empty() || saved.id.size() > 128 || !seen_ids.insert(saved.id).second ||
            !std::isfinite(saved.mean_utility) || saved.mean_utility < 0.05 || saved.mean_utility > 2.50 ||
            !std::isfinite(saved.mean_utility_delta)) return;

        const bool current_schema = magic == "CFTS8" || magic == "CFTS7" || magic == "CFTS6" || magic == "CFTS5" || magic == "CFTS4";
        if (current_schema) {
            if (!(ls >> saved.reward_samples >> saved.improved >> saved.regressed >> saved.neutral >> saved.aborted)) return;
            if ((magic == "CFTS8" || magic == "CFTS7") && !(ls >> saved.preempted)) return;
            if (!(ls >> saved.recovery_required >> saved.cooldown >> saved.consecutive_non_improved)) return;
            // Legacy CFTS3 counters were retained during the R1 migration while
            // reward_samples intentionally started at zero. Do not reinterpret those
            // historical outcome totals as eligible modern reward observations.
            if (saved.reward_samples > saved.trials) return;
        } else {
            if (!(ls >> saved.improved >> saved.regressed >> saved.cooldown >> saved.consecutive_non_improved)) return;
            // Pre-R1 trial totals mixed attempts with outcomes, so their historical
            // utility estimate has no defensible sample denominator. Preserve the
            // counters for diagnostics, but restart learning from an uninformative
            // prior until new ownership-verified reward samples are collected.
            saved.mean_utility = 0.5;
            saved.mean_utility_delta = 0.0;
            saved.reward_samples = 0;
            saved.neutral = 0;
            saved.aborted = 0;
            saved.recovery_required = 0;
        }

        if (!(ls >> saved.adaptive_scale >> saved.inconclusive >> saved.rejected >>
              saved.workload >> saved.thermal_bucket)) return;
        if (!std::isfinite(saved.adaptive_scale)) return;
        saved.adaptive_scale = std::clamp(saved.adaptive_scale, 0.25, 1.0);
        saved.cooldown = std::min<uint32_t>(saved.cooldown, 64U);
        saved.consecutive_non_improved = std::min<uint32_t>(saved.consecutive_non_improved, 16U);
        saved.workload = std::min<uint32_t>(saved.workload, 10U);
        saved.thermal_bucket = std::min<uint32_t>(saved.thermal_bucket, 4U);

        if (magic == "CFTS8" || magic == "CFTS7" || magic == "CFTS6" || magic == "CFTS5" || magic == "CFTS4" || magic == "CFTS3") {
            if (!(ls >> saved.ownership_losses >> saved.quarantine_ticks)) return;
        }
        if (magic == "CFTS8") {
            if (!(ls >> saved.last_regime) || saved.last_regime >= 6U) return;
            for (std::size_t i = 0; i < saved.regime_samples.size(); ++i) {
                if (!(ls >> saved.regime_samples[i] >> saved.regime_means[i] >>
                      saved.regime_regressions[i] >> saved.regime_cooldowns[i])) return;
                if (saved.regime_samples[i] > saved.reward_samples ||
                    !std::isfinite(saved.regime_means[i]) ||
                    (saved.regime_samples[i] > 0U &&
                     (saved.regime_means[i] < 0.05 || saved.regime_means[i] > 2.50)) ||
                    (saved.regime_samples[i] == 0U && saved.regime_means[i] != 0.0) ||
                    saved.regime_regressions[i] > saved.trials ||
                    saved.regime_cooldowns[i] > 64U) return;
            }
        }
        if (!std::getline(ls, saved.rollback_reason)) return;
        if (!saved.rollback_reason.empty() && saved.rollback_reason.front() == '\t')
            saved.rollback_reason.erase(saved.rollback_reason.begin());
        if (saved.rollback_reason.empty()) saved.rollback_reason = "-";
        if (saved.rollback_reason.size() > 512 ||
            saved.rollback_reason.find_first_of("\t\r\n") != std::string::npos) return;
        saved.ownership_losses = std::min<uint32_t>(saved.ownership_losses, 16U);
        saved.quarantine_ticks = std::min<uint32_t>(saved.quarantine_ticks, 64U);
        std::string trailing;
        if (ls >> trailing) return;
        parsed.push_back(std::move(saved));
    }
    if (in.bad()) return;

    // Commit parsed state only after the complete file, checksum, header, and every
    // candidate record have passed validation. Learning-state corruption must never
    // partially overwrite in-memory controller state or BaselineStore recovery state.
    total_trials_ = trials;
    total_reward_samples_ = persisted_reward_samples;
    total_improved_ = improved;
    total_rolled_back_ = rolled_back;
    total_inconclusive_ = loaded_inconclusive;
    total_rejected_ = loaded_rejected;
    noise_floor_ = std::clamp(loaded_noise_floor, 0.02, 0.45);
    evaluation_noise_floors_ = loaded_domain_floors;

    for (const auto& saved : parsed) {
        if (auto* candidate = find_candidate(saved.id)) {
            candidate->mean_utility = saved.mean_utility;
            candidate->mean_utility_delta = saved.mean_utility_delta;
            candidate->adaptive_scale = saved.adaptive_scale;
            candidate->trials = saved.trials;
            candidate->reward_samples = saved.reward_samples;
            candidate->improved_count = saved.improved;
            candidate->successes = saved.improved;
            candidate->regressed_count = saved.regressed;
            candidate->rollbacks = saved.regressed;
            candidate->neutral_count = saved.neutral;
            candidate->aborted_count = saved.aborted;
            candidate->preempted_count = saved.preempted;
            candidate->recovery_required_count = saved.recovery_required;
            candidate->cooldown_ticks = saved.cooldown;
            candidate->consecutive_non_improved = saved.consecutive_non_improved;
            candidate->inconclusive_count = saved.inconclusive;
            candidate->rejected_count = saved.rejected;
            candidate->last_workload = static_cast<WorkloadClass>(saved.workload);
            candidate->last_thermal_bucket = static_cast<uint8_t>(saved.thermal_bucket);
            if (magic == "CFTS8") {
                candidate->last_regime = static_cast<DynamicWorkloadRegime>(saved.last_regime);
                candidate->regime_reward_samples = saved.regime_samples;
                candidate->regime_mean_utility = saved.regime_means;
                candidate->regime_regressions = saved.regime_regressions;
                candidate->regime_cooldown_ticks = saved.regime_cooldowns;
            }
            candidate->ownership_loss_count = saved.ownership_losses;
            candidate->ownership_quarantine_ticks = saved.quarantine_ticks;
            candidate->last_rollback_reason = saved.rollback_reason == "-" ? "" : saved.rollback_reason;
        }
    }
    update_ucb_scores();
}

std::string AutonomousTuner::resolve_vm_node_path(std::string_view semantic_id) const {
    if (device_profile_) {
        const auto found = std::find_if(device_profile_->node_capabilities.begin(),
            device_profile_->node_capabilities.end(), [&](const NodeCapability& node) {
                return node.semantic_id == semantic_id && node.domain == HardwareDomain::Memory &&
                       node.source == ProfileSource::Procfs;
            });
        if (found != device_profile_->node_capabilities.end()) return found->path;
    }

    // Compatibility path for standalone/tests without a pinned CDDP profile:
    // enumerate the VM virtual table and match the standardized ABI name.
    const std::string leaf = semantic_id == "vm.page_cluster" ? "page-cluster" :
        semantic_id == "vm.vfs_cache_pressure" ? "vfs_cache_pressure" : std::string{};
    if (leaf.empty()) return {};
    const fs::path root = fs::path(proc_root_) / "sys/vm";
    std::error_code ec;
    for (fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end; it.increment(ec)) {
        if (it->path().filename() == leaf) {
            std::error_code type_ec;
            const auto status = it->symlink_status(type_ec);
            if (!type_ec && !fs::is_symlink(status) && fs::is_regular_file(status))
                return it->path().string();
        }
    }
    return {};
}

bool AutonomousTuner::has_runtime_node(std::string_view path) const noexcept {
    return CandidateSelector::has_runtime_path(kernel_node_capabilities_, path);
}

bool AutonomousTuner::candidate_contract_eligible(std::string_view candidate_id) const noexcept {
    const bool storage_candidate = candidate_id.rfind("io.", 0) == 0 ||
        candidate_id == "thermal.io_burst_pacing";
    if (storage_candidate) {
        for (const auto& queue : block_queues_) {
            const std::string scheduler = queue.queue_dir + "/scheduler";
            const std::string read_ahead = queue.queue_dir + "/read_ahead_kb";
            const std::string requests = queue.queue_dir + "/nr_requests";
            if (candidate_id == "io.queue_readahead_pacing") {
                if (has_runtime_node(read_ahead)) return true;
                continue;
            }
            if (candidate_id == "thermal.io_burst_pacing") {
                if (has_runtime_node(read_ahead) && has_runtime_node(requests)) return true;
                continue;
            }
            if (has_runtime_node(scheduler) && has_runtime_node(read_ahead)) return true;
        }
        return false;
    }

    if (candidate_id == "vm.zram_latency_relief") {
        const auto page_cluster = resolve_vm_node_path("vm.page_cluster");
        const auto cache_pressure = resolve_vm_node_path("vm.vfs_cache_pressure");
        return !page_cluster.empty() && !cache_pressure.empty() &&
               has_runtime_node(page_cluster) && has_runtime_node(cache_pressure);
    }

    if (candidate_id == "cpufreq.schedutil_interactive_response" ||
        candidate_id == "thermal.schedutil_damped_pacing") {
        return std::any_of(cpufreq_governor_dirs_.begin(), cpufreq_governor_dirs_.end(),
            [&](const std::string& governor_dir) {
                return has_runtime_node(governor_dir + "/up_rate_limit_us") &&
                       has_runtime_node(governor_dir + "/down_rate_limit_us");
            });
    }
    if (candidate_id == "cpu.frequency.tiered") {
        return frequency_actuator_ != nullptr && tiered_policy_router_ != nullptr &&
               select_frequency_policy_index().has_value();
    }
    return true;
}

void AutonomousTuner::classify_discovered_resources() {
    discovered_resources_.clear();
    std::error_code ec;

    // 1. Classify all block devices (both physical and virtual/unsafe)
    const fs::path block_root = fs::path(sys_root_) / "class/block";
    for (fs::directory_iterator it(block_root, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && discovered_resources_.size() < 64;
         it.increment(ec)) {
        const std::string dev_name = it->path().filename().string();
        const fs::path qdir = it->path() / "queue";
        const std::string sched_path = (qdir / "scheduler").string();
        if (!fs::exists(sched_path, ec) || ec) continue;

        if (is_virtual_block_device(dev_name)) {
            discovered_resources_.push_back(DiscoveredResourceMetadata{
                .path = sched_path,
                .domain = MutationDomain::BlockIo,
                .permission_class = ResourcePermissionClass::WritableButUnsafe,
                .unit = "token",
                .range_provenance = "virtual_block_queue",
                .baseline_value = read_first_line(sched_path),
                .reason = "Virtual block device (" + dev_name + ") excluded from scheduler mutation",
            });
            continue;
        }
        if (!is_physical_block_device(dev_name)) {
            discovered_resources_.push_back(DiscoveredResourceMetadata{
                .path = sched_path,
                .domain = MutationDomain::BlockIo,
                .permission_class = ResourcePermissionClass::UnknownSemantic,
                .unit = "unknown",
                .range_provenance = "unrecognized_block_device",
                .baseline_value = read_first_line(sched_path),
                .reason = "Unrecognized block device prefix; semantics unverified",
            });
            continue;
        }

        const bool writable = (::access(sched_path.c_str(), R_OK | W_OK) == 0);
        std::string active;
        std::vector<std::string> avail;
        parse_scheduler_line(read_first_line(sched_path), active, avail);
        discovered_resources_.push_back(DiscoveredResourceMetadata{
            .path = sched_path,
            .domain = MutationDomain::BlockIo,
            .permission_class = !writable ? ResourcePermissionClass::ReadOnly
                                : avail.empty() ? ResourcePermissionClass::IncompleteMetadata
                                : has_runtime_node(sched_path) ? ResourcePermissionClass::VerifiedConstraints
                                                              : ResourcePermissionClass::UnknownSemantic,
            .unit = "scheduler_token",
            .range_provenance = "sysfs:queue/scheduler",
            .baseline_value = active,
            .reason = !writable ? "Read-only scheduler node"
                : has_runtime_node(sched_path) ? "Verified scheduler ABI and immutable OEM baseline"
                                               : "Scheduler is observable; mutation contract/baseline not verified",
        });

        for (const auto& [leaf, unit_str, range_str] : {
                 std::tuple{"read_ahead_kb", "KiB", "[32..512] relative to OEM baseline"},
                 std::tuple{"nr_requests", "requests", "[32..512] relative to OEM baseline"},
                 std::tuple{"rq_affinity", "mode", "{0,1,2}"},
             }) {
            const std::string p = (qdir / leaf).string();
            if (!fs::exists(p, ec) || ec) continue;
            uint64_t parsed = 0;
            const std::string raw = read_first_line(p);
            const bool valid_num = parse_u64(raw, parsed);
            const bool leaf_w = (::access(p.c_str(), R_OK | W_OK) == 0);
            discovered_resources_.push_back(DiscoveredResourceMetadata{
                .path = p,
                .domain = MutationDomain::BlockIo,
                .permission_class = !valid_num ? ResourcePermissionClass::UnknownSemantic
                                    : !leaf_w  ? ResourcePermissionClass::ReadOnly
                                    : has_runtime_node(p) ? ResourcePermissionClass::VerifiedConstraints
                                                          : ResourcePermissionClass::UnknownSemantic,
                .unit = unit_str,
                .range_provenance = range_str,
                .baseline_value = raw,
                .reason = !valid_num ? "Non-numeric sysfs content"
                          : !leaf_w  ? "Read-only queue node"
                          : has_runtime_node(p) ? "Verified numeric queue ABI and immutable OEM baseline"
                                                : "Numeric queue attribute remains observation-only",
            });
        }
    }

    // 2. Classify CPUFreq policy & governor nodes.
    if (device_profile_) {
        for (const auto& cap : device_profile_->node_capabilities) {
            if (cap.domain != HardwareDomain::Cpu ||
                cap.semantic_id != "cpufreq.governor_tunable.unverified") continue;
            ResourcePermissionClass permission = ResourcePermissionClass::IncompleteMetadata;
            if (CandidateSelector::is_mutation_candidate(cap)) {
                permission = ResourcePermissionClass::VerifiedConstraints;
            } else if (cap.state == NodeCapabilityState::Writable) {
                permission = ResourcePermissionClass::WritableButUnsafe;
            } else if (cap.state == NodeCapabilityState::Readable) {
                permission = ResourcePermissionClass::ReadOnly;
            }
            discovered_resources_.push_back(DiscoveredResourceMetadata{
                .path = cap.path,
                .domain = MutationDomain::CpuPolicy,
                .permission_class = permission,
                .unit = cap.unit,
                .range_provenance = cap.constraints,
                .baseline_value = cap.current_value,
                .reason = cap.rejection_reason.empty() ? cap.evidence : cap.rejection_reason,
            });
        }
    } else {
    const fs::path cpufreq_root = fs::path(sys_root_) / "devices/system/cpu/cpufreq";
    std::error_code cpu_ec;
    for (fs::directory_iterator it(cpufreq_root, fs::directory_options::skip_permission_denied, cpu_ec), end;
         !cpu_ec && it != end && discovered_resources_.size() < 96;
         it.increment(cpu_ec)) {
        const std::string pol_name = it->path().filename().string();
        if (pol_name.rfind("policy", 0) != 0) continue;

        const std::string active_gov = DaemonConfig::trim(
            read_first_line((it->path() / "scaling_governor").string()));
        if (active_gov.empty()) continue;
        const fs::path governor_dir = it->path() / active_gov;
        std::error_code governor_ec;
        if (!fs::is_directory(governor_dir, governor_ec) || governor_ec) continue;
        const std::string up_path = (governor_dir / "up_rate_limit_us").string();
        const std::string down_path = (governor_dir / "down_rate_limit_us").string();
        const bool both_exist = fs::is_regular_file(up_path, governor_ec) && !governor_ec &&
                                fs::is_regular_file(down_path, governor_ec) && !governor_ec;
        const bool both_w = both_exist && (::access(up_path.c_str(), R_OK | W_OK) == 0) &&
                            (::access(down_path.c_str(), R_OK | W_OK) == 0);
        discovered_resources_.push_back(DiscoveredResourceMetadata{
            .path = up_path,
            .domain = MutationDomain::CpuPolicy,
            .permission_class = !both_exist ? ResourcePermissionClass::IncompleteMetadata :
                both_w ? ResourcePermissionClass::WritableButUnsafe : ResourcePermissionClass::ReadOnly,
            .unit = "microseconds",
            .range_provenance = "Active governor pair; ABI-specific limits require verified contract",
            .baseline_value = read_first_line(up_path),
            .reason = both_w ? "Active governor pair discovered; no policy value changed during discovery" :
                "Active governor pair incomplete or not writable",
        });
    }

    }
    // 3. Classify VM nodes
    for (const auto& [leaf, unit_str, range_str] : {
             std::tuple{"page-cluster", "log2_pages", "[0..3] requires active zram"},
             std::tuple{"vfs_cache_pressure", "percentage", "[50..150] requires active zram"},
         }) {
        const std::string semantic = std::string_view(leaf) == "page-cluster" ? "vm.page_cluster" : "vm.vfs_cache_pressure";
        const std::string p = resolve_vm_node_path(semantic);
        if (p.empty() || !fs::exists(p, ec) || ec) continue;
        uint64_t parsed = 0;
        const std::string raw = read_first_line(p);
        const bool valid_num = parse_u64(raw, parsed);
        const bool writable = (::access(p.c_str(), R_OK | W_OK) == 0);
        ResourcePermissionClass perm = has_runtime_node(p)
            ? ResourcePermissionClass::VerifiedConstraints : ResourcePermissionClass::UnknownSemantic;
        std::string reason = has_runtime_node(p)
            ? "Verified VM tunable ABI and immutable OEM baseline"
            : "VM node is observed; runtime contract/baseline not verified";
        if (!valid_num) {
            perm = ResourcePermissionClass::UnknownSemantic;
            reason = "Unparseable VM tunable value";
        } else if (!writable) {
            perm = ResourcePermissionClass::ReadOnly;
            reason = "Read-only procfs VM tunable";
        } else if (!zram_active_) {
            perm = ResourcePermissionClass::WritableButUnsafe;
            reason = "Writable VM node gated because zram swap is inactive";
        }
        discovered_resources_.push_back(DiscoveredResourceMetadata{
            .path = p,
            .domain = MutationDomain::VmMemory,
            .permission_class = perm,
            .unit = unit_str,
            .range_provenance = range_str,
            .baseline_value = raw,
            .reason = reason,
        });
    }
}

void AutonomousTuner::set_operational(bool enabled) noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    if (!enabled && foreground_task_resolver_) {
        (void)foreground_task_resolver_->restore_all();
    }
    operational_ = enabled;
}

void AutonomousTuner::configure_foreground_task_resolver(
    ForegroundTaskResolver* resolver) noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    if (foreground_task_resolver_ == resolver) return;
    if (foreground_task_resolver_ && !foreground_task_resolver_->restore_all()) return;
    foreground_task_resolver_ = resolver;
}

void AutonomousTuner::discover(std::string_view sys_root, std::string_view proc_root) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    sys_root_ = std::string(sys_root);
    proc_root_ = std::string(proc_root);
    block_queues_.clear();
    cpufreq_governor_dirs_.clear();
    zram_active_ = check_zram_swap_active(proc_root_, sys_root_);

    const fs::path block_root = fs::path(sys_root_) / "class/block";
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
        surface.queue_tunables = enumerate_attribute_names(qdir);
        surface.iosched_tunables = enumerate_attribute_names(qdir / "iosched");
        surface.scheduler_generation = 1U;

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
        if (fs::is_directory(iosched, ec) && !ec && !surface.iosched_tunables.empty()) {
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

    auto& store = BaselineStore::instance();
    // Discover CPUFreq governor rate-limit pairs only from the injected profile.
    // No profile means legacy compatibility; an injected profile suppresses all
    // independent CPUFreq enumeration, even when it rejects every candidate.
    if (device_profile_) {
        const auto eligible_capability = [&](const std::string& path) {
            return std::any_of(device_profile_->node_capabilities.begin(),
                device_profile_->node_capabilities.end(), [&](const NodeCapability& cap) {
                    return cap.path == path && cap.mutation_eligible();
                });
        };
        for (const auto& policy : device_profile_->cpu_policies) {
            const fs::path policy_path(policy.policy_path);
            const fs::path governor_dir = policy_path / policy.governor;
            const std::string up = (governor_dir / "up_rate_limit_us").string();
            const std::string down = (governor_dir / "down_rate_limit_us").string();
            if (!policy.governor.empty() && eligible_capability(up) && eligible_capability(down)) {
                (void)store.capture(up);
                (void)store.capture(down);
                cpufreq_governor_dirs_.push_back(governor_dir.string());
            }
        }
    } else {
    // Legacy compatibility discovery for non-CDDP callers.
    // Verify policyX/scaling_governor matches the governor directory if scaling_governor exists.
    const fs::path cpufreq_root = fs::path(sys_root_) / "devices/system/cpu/cpufreq";
    std::error_code cpu_ec;
    for (fs::directory_iterator it(cpufreq_root, fs::directory_options::skip_permission_denied, cpu_ec), end;
         !cpu_ec && it != end && cpufreq_governor_dirs_.size() < 8;
         it.increment(cpu_ec)) {
        const std::string pol_name = it->path().filename().string();
        if (pol_name.rfind("policy", 0) != 0) continue;
        const fs::path scaling_gov_path = it->path() / "scaling_governor";
        const std::string active_gov = DaemonConfig::trim(read_first_line(scaling_gov_path.string()));
        if (active_gov.empty()) continue;
        const fs::path governor_dir = it->path() / active_gov;
        const std::string up_path = (governor_dir / "up_rate_limit_us").string();
        const std::string down_path = (governor_dir / "down_rate_limit_us").string();
        if (::access(up_path.c_str(), R_OK | W_OK) == 0 &&
            ::access(down_path.c_str(), R_OK | W_OK) == 0) {
            (void)store.capture(up_path);
            (void)store.capture(down_path);
            cpufreq_governor_dirs_.push_back(governor_dir.string());
        }
    }
    }
    std::sort(cpufreq_governor_dirs_.begin(), cpufreq_governor_dirs_.end());

    // Capture VM baseline nodes if accessible
    const auto page_cluster_path = resolve_vm_node_path("vm.page_cluster");
    const auto cache_pressure_path = resolve_vm_node_path("vm.vfs_cache_pressure");
    if (!page_cluster_path.empty()) (void)store.capture(page_cluster_path);
    if (!cache_pressure_path.empty()) (void)store.capture(cache_pressure_path);

    // Pin the semantic inventory used by every candidate admission and
    // transaction preflight.  A profile is authoritative when supplied;
    // standalone/test construction gets the same bounded scanner after OEM
    // baselines have been captured above.
    kernel_node_capabilities_.clear();
    if (device_profile_) {
        kernel_node_capabilities_ = device_profile_->node_capabilities;
    } else {
        KernelNodeInventory inventory(sys_root_, proc_root_);
        kernel_node_capabilities_ = inventory.scan(512U);
    }

    classify_discovered_resources();
    load_persisted_stats();
}

void AutonomousTuner::refresh_surface_readings() {
    zram_active_ = check_zram_swap_active(proc_root_, sys_root_);
    auto refresh_queue = [](BlockQueueSurface& q) {
        std::string active;
        std::vector<std::string> avail;
        parse_scheduler_line(read_first_line(q.queue_dir + "/scheduler"), active, avail);
        const bool parent_policy_changed = !active.empty() && active != q.active_scheduler;
        if (!active.empty()) q.active_scheduler = active;
        if (!avail.empty()) q.available_schedulers = std::move(avail);
        if (parent_policy_changed || q.scheduler_generation == 0U) {
            // Scheduler-specific ABI children are invalidated by a parent policy
            // transition. Re-enumerate the currently active iosched directory now.
            q.queue_tunables = enumerate_attribute_names(q.queue_dir);
            q.iosched_tunables = enumerate_attribute_names(fs::path(q.queue_dir) / "iosched");
            q.has_iosched_tunables = !q.iosched_tunables.empty();
            ++q.scheduler_generation;
        }

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
    };

    if (block_queues_.size() > 1U && worker_pool_ && worker_pool_->active()) {
        worker_pool_->parallel_for(block_queues_.size(), [&](size_t i) noexcept {
            refresh_queue(block_queues_[i]);
        });
    } else {
        for (auto& q : block_queues_) {
            refresh_queue(q);
        }
    }
    update_ucb_scores();
}

bool AutonomousTuner::verify_intervention_ownership(const ActiveIntervention& iv) const {
    if (iv.is_frequency_policy) {
        return frequency_actuator_ != nullptr && frequency_actuator_->mutation_active();
    }
    auto& store = BaselineStore::instance();
    for (const auto& step : iv.applied_steps) {
        if (::access(step.path.c_str(), R_OK) != 0 ||
            !store.owns_expected_state(step.path, step.target_value)) {
            return false;
        }
    }
    return true;
}

bool AutonomousTuner::verify_active_ownership() const {
    for (const auto& iv : active_interventions_) {
        if (!verify_intervention_ownership(iv)) {
            return false;
        }
    }
    auto& store = BaselineStore::instance();
    for (const auto& step : applied_steps_) {
        if (::access(step.path.c_str(), R_OK) != 0 ||
            !store.owns_expected_state(step.path, step.target_value)) {
            return false;
        }
    }
    return true;
}

bool AutonomousTuner::retry_contended_intervention_ownership(ActiveIntervention& iv) {
    if (iv.is_frequency_policy || iv.applied_steps.empty() ||
        iv.candidate_id.empty() || iv.candidate_id == "baseline" || recovery_required_) {
        return false;
    }
    uint32_t attempt = 0;
    constexpr uint32_t kMaximumContentionRetries = 3;
    if (!coordinator_.reserve_contention_retry(
            iv.mutation_domain, controller_name_,
            iv.candidate_id, kMaximumContentionRetries, &attempt)) {
        return false;
    }
    if (total_contention_retries_ < std::numeric_limits<uint32_t>::max()) {
        ++total_contention_retries_;
    }

    auto& store = BaselineStore::instance();
    for (const auto& step : iv.applied_steps) {
        if (!store.reassert_mutation_from_oem_baseline(step.path, step.target_value)) {
            return false;
        }
    }

    const uint32_t backoff = 1U << (attempt - 1U);
    iv.contention_backoff_ticks = backoff;
    contention_backoff_ticks_ = std::max(contention_backoff_ticks_, backoff);
    iv.obs_ticks = 0;
    iv.observing = true;
    iv.retained = false;
    iv.sum_after_io_psi = 0.0;
    iv.sum_after_mem_psi = 0.0;
    iv.sum_after_cpu_psi = 0.0;
    iv.sum_after_thermal = 0.0;
    iv.sum_after_temp_vel = 0.0;
    iv.valid_thermal_samples_in_window = 0;
    iv.valid_io_samples_in_window = 0;
    iv.valid_mem_samples_in_window = 0;
    iv.valid_cpu_samples_in_window = 0;
    iv.valid_pressure_samples_in_window = 0;
    iv.min_after_io_psi = 1e9;
    iv.min_after_mem_psi = 1e9;
    iv.min_after_cpu_psi = 1e9;
    iv.min_after_thermal = 1e9;
    iv.max_after_io_psi = -1e9;
    iv.max_after_mem_psi = -1e9;
    iv.max_after_cpu_psi = -1e9;
    iv.max_after_thermal = -1e9;
    if (const auto* stats = find_candidate(iv.candidate_id)) {
        iv.settling_ticks_remaining = stats->settling_ticks;
    }
    iv.verdict = "PENDING";
    iv.verdict_enum = TunerVerdict::Pending;
    iv.last_utility_delta = 0.0;
    iv.rationale = "OEM contention detected; exact captured OEM baseline reasserted " +
        std::to_string(attempt) + "/3; observation restarted after bounded tick backoff.";
    last_verdict_ = "PENDING";
    last_verdict_enum_ = TunerVerdict::Pending;
    last_utility_delta_ = 0.0;
    current_rationale_ = iv.rationale;
    sync_primary_state_from_active();
    refresh_surface_readings();
    return true;
}

bool AutonomousTuner::retry_contended_ownership() {
    if (active_interventions_.empty()) return false;
    return retry_contended_intervention_ownership(active_interventions_.front());
}

void AutonomousTuner::sync_primary_state_from_active() noexcept {
    if (active_interventions_.empty()) {
        transaction_active_ = false;
        observing_ = false;
        thermal_stabilization_active_ = false;
        current_evaluation_domain_ = EvaluationDomain::Unavailable;
        settling_ticks_remaining_ = 0;
        obs_ticks_ = 0;
        applied_steps_.clear();
        current_candidate_id_ = "baseline";
        current_candidate_label_ = "OEM Verified Baseline";
        current_intent_ = "HoldEquilibrium";
        return;
    }

    const ActiveIntervention* primary = &active_interventions_.front();
    bool any_observing = false;
    bool any_thermal_stab = false;
    for (const auto& iv : active_interventions_) {
        if (iv.observing && !any_observing) {
            primary = &iv;
        }
        if (iv.observing) any_observing = true;
        if (iv.thermal_stabilization) any_thermal_stab = true;
    }

    transaction_active_ = true;
    observing_ = any_observing;
    thermal_stabilization_active_ = any_thermal_stab;
    current_candidate_id_ = primary->candidate_id;
    current_candidate_label_ = primary->candidate_label;
    current_intent_ = primary->intent;
    current_rationale_ = primary->rationale;
    current_evaluation_domain_ = primary->evaluation_domain;
    settling_ticks_remaining_ = primary->settling_ticks_remaining;
    obs_ticks_ = primary->obs_ticks;
    obs_window_ = primary->obs_window;
    idle_streak_ = primary->idle_streak;
    before_io_psi_ = primary->before_io_psi;
    before_mem_psi_ = primary->before_mem_psi;
    before_cpu_psi_ = primary->before_cpu_psi;
    before_thermal_ = primary->before_thermal;
    before_temperature_celsius_ = primary->before_temperature_celsius;
    before_io_valid_ = primary->before_io_valid;
    before_mem_valid_ = primary->before_mem_valid;
    before_cpu_valid_ = primary->before_cpu_valid;
    before_thermal_valid_ = primary->before_thermal_valid;
    sum_after_io_psi_ = primary->sum_after_io_psi;
    sum_after_mem_psi_ = primary->sum_after_mem_psi;
    sum_after_cpu_psi_ = primary->sum_after_cpu_psi;
    sum_after_thermal_ = primary->sum_after_thermal;
    sum_after_temp_vel_ = primary->sum_after_temp_vel;
    valid_thermal_samples_in_window_ = primary->valid_thermal_samples_in_window;
    valid_io_samples_in_window_ = primary->valid_io_samples_in_window;
    valid_mem_samples_in_window_ = primary->valid_mem_samples_in_window;
    valid_cpu_samples_in_window_ = primary->valid_cpu_samples_in_window;
    valid_pressure_samples_in_window_ = primary->valid_pressure_samples_in_window;
    min_after_io_psi_ = primary->min_after_io_psi;
    max_after_io_psi_ = primary->max_after_io_psi;
    min_after_mem_psi_ = primary->min_after_mem_psi;
    max_after_mem_psi_ = primary->max_after_mem_psi;
    min_after_cpu_psi_ = primary->min_after_cpu_psi;
    max_after_cpu_psi_ = primary->max_after_cpu_psi;
    min_after_thermal_ = primary->min_after_thermal;
    max_after_thermal_ = primary->max_after_thermal;
    applied_steps_ = primary->applied_steps;
}

bool AutonomousTuner::rollback_partial_steps(
    MutationDomain domain,
    std::string_view candidate_id,
    std::vector<MutationStep>& steps,
    std::string_view reason) {
    auto& store = BaselineStore::instance();
    std::vector<MutationStep> failed_steps;
    for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
        if (!store.rollback(it->path)) {
            failed_steps.push_back(*it);
        }
    }
    if (!failed_steps.empty()) {
        std::reverse(failed_steps.begin(), failed_steps.end());
        steps = failed_steps;
        if (!recovery_required_) {
            if (auto* stats = find_candidate(candidate_id)) {
                ++stats->recovery_required_count;
            }
        }
        unresolved_recoveries_.push_back(FailedRecoveryRecord{
            .domain = domain,
            .candidate_id = std::string(candidate_id),
            .failed_steps = std::move(failed_steps),
            .is_frequency_policy = false,
        });
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) + " [ROLLBACK FAILED: RECOVERY_REQUIRED]";
        refresh_surface_readings();
        return false;
    }
    steps.clear();
    if (!candidate_id.empty() && candidate_id != "baseline") {
        coordinator_.release_lease(domain, controller_name_, candidate_id);
    }
    refresh_surface_readings();
    return true;
}

bool AutonomousTuner::rollback_intervention(ActiveIntervention& iv, std::string_view reason) {
    if (iv.is_frequency_policy) {
        if (frequency_actuator_ &&
            (frequency_mutation_active_ || frequency_actuator_->mutation_active() ||
             frequency_actuator_->recovery_required())) {
            if (!frequency_actuator_->rollback()) {
                frequency_mutation_active_ = true;
                const bool already_recorded = std::any_of(
                    unresolved_recoveries_.begin(), unresolved_recoveries_.end(),
                    [&](const FailedRecoveryRecord& rec) {
                        return rec.is_frequency_policy;
                    });
                if (!already_recorded) {
                    unresolved_recoveries_.push_back(FailedRecoveryRecord{
                        .domain = MutationDomain::CpuPolicy,
                        .candidate_id = iv.candidate_id,
                        .failed_steps = {},
                        .is_frequency_policy = true,
                    });
                }
                recovery_required_ = true;
                last_verdict_ = "RECOVERY_REQUIRED";
                last_verdict_enum_ = TunerVerdict::RecoveryRequired;
                current_rationale_ = std::string(reason) +
                    " [frequency actuator rollback unresolved; CpuPolicy lease retained]";
                return false;
            }
            frequency_mutation_active_ = false;
        }
        return true;
    }

    auto& store = BaselineStore::instance();
    std::vector<MutationStep> failed_steps;
    // Unwind in reverse dependency order: Coupled-with dependents first, then Anchor.
    for (auto it = iv.applied_steps.rbegin(); it != iv.applied_steps.rend(); ++it) {
        if (!store.rollback(it->path)) {
            failed_steps.push_back(*it);
        }
    }

    if (!failed_steps.empty()) {
        std::reverse(failed_steps.begin(), failed_steps.end());
        iv.applied_steps = failed_steps;
        if (!recovery_required_) {
            if (auto* stats = find_candidate(iv.candidate_id)) {
                ++stats->recovery_required_count;
            }
        }
        auto existing = std::find_if(
            unresolved_recoveries_.begin(), unresolved_recoveries_.end(),
            [&](const FailedRecoveryRecord& rec) {
                return !rec.is_frequency_policy &&
                       rec.domain == iv.mutation_domain &&
                       rec.candidate_id == iv.candidate_id;
            });
        if (existing != unresolved_recoveries_.end()) {
            existing->failed_steps = std::move(failed_steps);
        } else {
            unresolved_recoveries_.push_back(FailedRecoveryRecord{
                .domain = iv.mutation_domain,
                .candidate_id = iv.candidate_id,
                .failed_steps = std::move(failed_steps),
                .is_frequency_policy = false,
            });
        }
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) + " [ROLLBACK FAILED: RECOVERY_REQUIRED]";
        refresh_surface_readings();
        return false;
    }

    if (!iv.candidate_id.empty() && iv.candidate_id != "baseline") {
        coordinator_.release_lease(iv.mutation_domain, controller_name_, iv.candidate_id);
    }
    iv.applied_steps.clear();
    // Rebuild dynamic queue metadata after restoring the prior scheduler before
    // save_persisted_stats() recomputes the capability fingerprint.
    refresh_surface_readings();
    return true;
}

bool AutonomousTuner::retry_unresolved_recoveries() {
    auto& store = BaselineStore::instance();
    bool all_ok = true;
    for (auto it = unresolved_recoveries_.begin(); it != unresolved_recoveries_.end();) {
        if (it->is_frequency_policy) {
            if (frequency_actuator_ &&
                (frequency_actuator_->mutation_active() ||
                 frequency_actuator_->recovery_required())) {
                if (!frequency_actuator_->rollback()) {
                    all_ok = false;
                    ++it;
                    continue;
                }
            }
            frequency_mutation_active_ = false;
            it = unresolved_recoveries_.erase(it);
            continue;
        }

        std::vector<MutationStep> still_failed;
        for (auto step_it = it->failed_steps.rbegin(); step_it != it->failed_steps.rend(); ++step_it) {
            if (!store.rollback(step_it->path)) {
                still_failed.push_back(*step_it);
            }
        }
        if (!still_failed.empty()) {
            std::reverse(still_failed.begin(), still_failed.end());
            it->failed_steps = std::move(still_failed);
            all_ok = false;
            ++it;
            continue;
        }
        if (!it->candidate_id.empty() && it->candidate_id != "baseline") {
            coordinator_.release_lease(it->domain, controller_name_, it->candidate_id);
        }
        it = unresolved_recoveries_.erase(it);
    }

    if (frequency_actuator_ &&
        (frequency_mutation_active_ || frequency_actuator_->mutation_active() ||
         frequency_actuator_->recovery_required())) {
        if (!frequency_actuator_->rollback()) {
            frequency_mutation_active_ = true;
            all_ok = false;
        } else {
            frequency_mutation_active_ = false;
        }
    }
    return all_ok;
}

bool AutonomousTuner::rollback_active(std::string_view reason) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    contention_backoff_ticks_ = 0;

    bool all_restored = retry_unresolved_recoveries();

    // Also unwind any in-flight partial applied_steps_ left from an interrupted apply_transaction().
    if (!applied_steps_.empty() && active_interventions_.empty()) {
        auto& store = BaselineStore::instance();
        std::vector<MutationStep> failed_steps;
        for (auto it = applied_steps_.rbegin(); it != applied_steps_.rend(); ++it) {
            if (!store.rollback(it->path)) {
                failed_steps.push_back(*it);
            }
        }
        if (!failed_steps.empty()) {
            std::reverse(failed_steps.begin(), failed_steps.end());
            applied_steps_ = std::move(failed_steps);
            unresolved_recoveries_.push_back(FailedRecoveryRecord{
                .domain = domain_for_candidate(current_candidate_id_),
                .candidate_id = current_candidate_id_,
                .failed_steps = applied_steps_,
                .is_frequency_policy = false,
            });
            all_restored = false;
        } else {
            if (!current_candidate_id_.empty() && current_candidate_id_ != "baseline") {
                coordinator_.release_lease(
                    domain_for_candidate(current_candidate_id_),
                    controller_name_,
                    current_candidate_id_);
            }
            applied_steps_.clear();
        }
    }

    // Unwind all active multi-domain interventions in reverse order of acquisition.
    while (!active_interventions_.empty()) {
        auto iv = std::move(active_interventions_.back());
        active_interventions_.pop_back();
        if (!rollback_intervention(iv, reason)) {
            all_restored = false;
        }
    }

    observing_ = false;
    obs_ticks_ = 0;
    settling_ticks_remaining_ = 0;

    if (!all_restored || !unresolved_recoveries_.empty()) {
        transaction_active_ = false;
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) + " [ROLLBACK FAILED: RECOVERY_REQUIRED]";
        if (!unresolved_recoveries_.empty()) {
            current_candidate_id_ = unresolved_recoveries_.front().candidate_id;
            applied_steps_ = unresolved_recoveries_.front().failed_steps;
        }
        refresh_surface_readings();
        return false;
    }

    applied_steps_.clear();
    transaction_active_ = false;
    recovery_required_ = false;
    thermal_stabilization_active_ = false;
    current_evaluation_domain_ = EvaluationDomain::Unavailable;
    current_candidate_id_ = "baseline";
    current_candidate_label_ = "OEM Verified Baseline";
    current_intent_ = "HoldEquilibrium";
    current_rationale_ = std::string(reason);
    refresh_surface_readings();
    return true;
}

bool AutonomousTuner::emergency_rollback(std::string_view reason) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    auto& coordinator = coordinator_;
    auto& store = BaselineStore::instance();
    const bool had_active_transaction =
        !active_interventions_.empty() || transaction_active_;
    current_objective_ = OptimizationObjective::EmergencyMitigation;
    if (had_active_transaction) {
        last_verdict_ = "ABORTED";
        last_verdict_enum_ = TunerVerdict::Aborted;
    }

    // Close admission before dispatching emergency preemption callbacks.
    coordinator.begin_emergency_recovery();
    coordinator.emergency_preempt_all(reason);
    if (frequency_actuator_ &&
        !frequency_actuator_->mutation_active() &&
        !frequency_actuator_->recovery_required()) {
        frequency_mutation_active_ = false;
    }
    const auto remaining = coordinator.active_leases();
    const bool live_owner_pending = std::any_of(
        remaining.begin(), remaining.end(),
        [](const DomainLease& lease) { return !lease.owner_detached; });
    if (live_owner_pending) {
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) +
            " [LIVE OWNER PREEMPTION ACKNOWLEDGEMENT PENDING]";
        save_persisted_stats();
        return false;
    }

    if (store.has_active_mutations()) (void)store.rollback_all();
    if (store.has_active_mutations() || !store.verify_and_persist_clear_state() ||
        (!remaining.empty() &&
         !coordinator.release_orphaned_leases_after_verified_recovery(store))) {
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) +
            " [ORPHANED OWNER RECOVERY INCOMPLETE; MUTATION ADMISSION REMAINS CLOSED]";
        save_persisted_stats();
        return false;
    }

    const bool restored = rollback_active(reason);
    if (!restored || store.has_active_mutations() ||
        !coordinator.complete_emergency_recovery()) {
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) +
            " [GLOBAL RECOVERY INCOMPLETE; MUTATION ADMISSION REMAINS CLOSED]";
        save_persisted_stats();
        return false;
    }

    if (had_active_transaction) ++total_rolled_back_;
    current_intent_ = "ThermalHoldAndProtect";
    save_persisted_stats();
    return true;
}

bool AutonomousTuner::apply_transaction(
    std::string_view candidate_id,
    std::string_view candidate_label,
    std::string_view intent,
    std::string_view rationale,
    const std::vector<MutationStep>& steps,
    const TunerTelemetryInput& telemetry,
    EvaluationDomain eval_domain_override,
    const std::vector<std::string>& resource_keys,
    bool replace_existing_if_any) {
    if (steps.empty() || recovery_required_) return false;

    EvaluationDomain evaluation_domain = eval_domain_override;
    if (evaluation_domain == EvaluationDomain::Unavailable) {
        if (intent == "IoLatencyRelief") evaluation_domain = EvaluationDomain::Io;
        else if (intent == "MemoryStallRelief") evaluation_domain = EvaluationDomain::Memory;
        else if (intent == "InteractiveCpuPacing") evaluation_domain = EvaluationDomain::Cpu;
        else if (intent == "ThermalStabilization") evaluation_domain = EvaluationDomain::Thermal;
    }
    const bool target_baseline_valid =
        (evaluation_domain == EvaluationDomain::Io && telemetry.io_psi_valid && std::isfinite(telemetry.io_psi) && telemetry.io_psi >= 0.0) ||
        (evaluation_domain == EvaluationDomain::Memory && telemetry.memory_psi_valid && std::isfinite(telemetry.memory_psi) && telemetry.memory_psi >= 0.0) ||
        (evaluation_domain == EvaluationDomain::Cpu && telemetry.cpu_psi_valid && std::isfinite(telemetry.cpu_psi) && telemetry.cpu_psi >= 0.0) ||
        (evaluation_domain == EvaluationDomain::Thermal && telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure) && telemetry.thermal_pressure >= 0.0);
    if (!target_baseline_valid) return false;

    const MutationDomain dom = domain_for_candidate(candidate_id);
    if (replace_existing_if_any) {
        if (!rollback_active("Switching coordinated candidate")) {
            return false;
        }
    } else {
        // Ensure no same-domain intervention is active inside this tuner; rollback any same-domain stale entry.
        for (size_t idx = 0; idx < active_interventions_.size(); ++idx) {
            if (active_interventions_[idx].mutation_domain == dom) {
                if (!rollback_intervention(active_interventions_[idx], "Replacing same-domain candidate")) {
                    active_interventions_.erase(active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx));
                    sync_primary_state_from_active();
                    return false;
                }
                active_interventions_.erase(active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx));
                break;
            }
        }
    }

    auto& store = BaselineStore::instance();
    if (store.identity_unavailable()) return false;

    const ControllerPriority prio = priority_for_candidate(candidate_id);
    std::string conflict_reason;
    if (!coordinator_.acquire_lease(
            dom, controller_name_, candidate_id, prio, &conflict_reason)) {
        ++total_rejected_;
        if (auto* stats = find_candidate(candidate_id)) {
            ++stats->rejected_count;
            stats->last_rollback_reason = conflict_reason;
        }
        if (active_interventions_.empty()) {
            last_verdict_ = "REJECTED";
            last_verdict_enum_ = TunerVerdict::Rejected;
            current_rationale_ = "Candidate " + std::string(candidate_id) + " rejected: " + conflict_reason;
        }
        sync_primary_state_from_active();
        save_persisted_stats();
        return false;
    }

    // Publish the exact lease identity before preflight so preemption or failure unwinds cleanly.
    current_candidate_id_ = std::string(candidate_id);
    contention_backoff_ticks_ = 0;
    auto& coordinator = coordinator_;

    const auto reject_preflight = [&](std::string_view reason) {
        coordinator.release_lease(dom, controller_name_, candidate_id);
        ++total_rejected_;
        if (auto* stats = find_candidate(candidate_id)) {
            ++stats->rejected_count;
            stats->last_rollback_reason = std::string(reason);
        }
        if (active_interventions_.empty()) {
            last_verdict_ = "REJECTED";
            last_verdict_enum_ = TunerVerdict::Rejected;
            current_rationale_ = "Candidate " + std::string(candidate_id) +
                " rejected during preflight: " + std::string(reason);
        }
        sync_primary_state_from_active();
        save_persisted_stats();
    };

    const auto record_apply_abort = [&](std::string_view reason) {
        last_verdict_ = "ABORTED";
        last_verdict_enum_ = TunerVerdict::Aborted;
        ++total_rolled_back_;
        if (auto* stats = find_candidate(candidate_id)) {
            ++stats->aborted_count;
            stats->last_rollback_reason = std::string(reason);
        }
        current_rationale_ = std::string(reason);
    };

    // Phase 1: Full Preflight (All-or-Nothing for RequiredAnchor & RequiredDependent steps)
    std::vector<MutationStep> planned;
    planned.reserve(steps.size());
    bool has_anchor = false;
    std::string before_summary;

    for (const auto& step : steps) {
        const bool is_required = (step.relation == StepRelation::RequiredAnchor ||
                                  step.relation == StepRelation::RequiredDependent);
        if (!has_runtime_node(step.path)) {
            if (is_required) {
                reject_preflight("Node has no RUNTIME_TUNABLE contract: " + step.path);
                return false;
            }
            continue;
        }
        bool accessible = (::access(step.path.c_str(), R_OK | W_OK) == 0) &&
                          store.capture(step.path);
        if (accessible) {
            const auto audit = store.get_audit_trail();
            const auto snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
                return node.path == step.path;
            });
            if (snapshot == audit.end() || snapshot->is_modified) accessible = false;
        }
        if (is_required && !accessible) {
            reject_preflight("Required resource unavailable, unsafe, or no longer writable: " + step.path);
            return false;
        }
        if (accessible) {
            if (step.relation == StepRelation::RequiredAnchor) {
                has_anchor = true;
            }
            if (!before_summary.empty()) before_summary += ",";
            before_summary += fs::path(step.path).filename().string() + "=" + read_first_line(step.path);
            planned.push_back(step);
        }
    }

    if (!has_anchor || planned.empty()) {
        reject_preflight("No valid required anchor or mutation steps were available");
        return false;
    }
    if (coordinator.is_preemption_requested(dom, controller_name_, candidate_id)) {
        return false;
    }

    // Phase 2: Ordered Execution (RequiredAnchor before RequiredDependent)
    std::vector<MutationStep> committed;
    committed.reserve(planned.size());
    std::string after_summary;

    for (const auto& step : planned) {
        if (coordinator.is_preemption_requested(dom, controller_name_, candidate_id)) {
            applied_steps_ = std::move(committed);
            observing_ = false;
            transaction_active_ = false;
            return false;
        }
        if (!store.apply_mutation(step.path, step.target_value)) {
            const bool is_required = (step.relation == StepRelation::RequiredAnchor ||
                                      step.relation == StepRelation::RequiredDependent);
            const auto audit = store.get_audit_trail();
            const auto snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
                return node.path == step.path;
            });
            const bool unresolved_path_mutation =
                snapshot != audit.end() && snapshot->is_modified;

            if (!is_required) {
                if (unresolved_path_mutation && !store.rollback(step.path)) {
                    committed.push_back(step);
                    applied_steps_ = std::move(committed);
                    const std::string reason =
                        "Optional step failed and its path-scoped rollback is unresolved.";
                    record_apply_abort(reason);
                    if (active_interventions_.empty()) {
                        (void)rollback_active(reason);
                    } else {
                        (void)rollback_partial_steps(dom, candidate_id, applied_steps_, reason);
                        sync_primary_state_from_active();
                    }
                    save_persisted_stats();
                    return false;
                }
                continue;
            }

            if (unresolved_path_mutation) committed.push_back(step);
            applied_steps_ = std::move(committed);
            const std::string reason =
                "Transaction aborted on failed required step; rollback attempted.";
            record_apply_abort(reason);
            if (active_interventions_.empty()) {
                (void)rollback_active(reason);
            } else {
                (void)rollback_partial_steps(dom, candidate_id, applied_steps_, reason);
                sync_primary_state_from_active();
            }
            save_persisted_stats();
            return false;
        }
        if (!after_summary.empty()) after_summary += ",";
        after_summary += fs::path(step.path).filename().string() + "=" + step.target_value;
        committed.push_back(step);
    }

    if (coordinator.is_preemption_requested(dom, controller_name_, candidate_id)) {
        applied_steps_ = std::move(committed);
        observing_ = false;
        transaction_active_ = false;
        return false;
    }

    if (committed.empty()) {
        reject_preflight("No mutation step was committed");
        return false;
    }

    ActiveIntervention iv{};
    iv.candidate_id = std::string(candidate_id);
    iv.candidate_label = std::string(candidate_label);
    iv.mutation_domain = dom;
    iv.evaluation_domain = evaluation_domain;
    iv.intent = std::string(intent);
    iv.rationale = std::string(rationale);
    iv.is_frequency_policy = false;
    iv.thermal_stabilization = (intent == "ThermalStabilization");
    iv.observing = true;
    iv.retained = false;
    iv.obs_ticks = 0;
    iv.obs_window = obs_window_;
    iv.idle_streak = 0;
    iv.before_io_valid = telemetry.io_psi_valid && std::isfinite(telemetry.io_psi) && telemetry.io_psi >= 0.0;
    iv.before_mem_valid = telemetry.memory_psi_valid && std::isfinite(telemetry.memory_psi) && telemetry.memory_psi >= 0.0;
    iv.before_cpu_valid = telemetry.cpu_psi_valid && std::isfinite(telemetry.cpu_psi) && telemetry.cpu_psi >= 0.0;
    iv.before_thermal_valid = telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure) && telemetry.thermal_pressure >= 0.0;
    iv.before_io_psi = iv.before_io_valid ? telemetry.io_psi : 0.0;
    iv.before_mem_psi = iv.before_mem_valid ? telemetry.memory_psi : 0.0;
    iv.before_cpu_psi = iv.before_cpu_valid ? telemetry.cpu_psi : 0.0;
    iv.before_thermal = iv.before_thermal_valid ? telemetry.thermal_pressure : 0.0;
    iv.before_temperature_celsius = telemetry.current_temperature_celsius;
    iv.verdict = "PENDING";
    iv.verdict_enum = TunerVerdict::Pending;
    iv.last_utility_delta = 0.0;
    iv.applied_steps = std::move(committed);
    if (!resource_keys.empty()) {
        iv.resource_keys = resource_keys;
    } else {
        for (const auto& s : iv.applied_steps) {
            iv.resource_keys.push_back(s.path);
        }
    }

    noise_floor_ = evaluation_noise_floors_[static_cast<size_t>(evaluation_domain)];
    last_verdict_ = "PENDING";
    last_verdict_enum_ = TunerVerdict::Pending;
    last_utility_delta_ = 0.0;
    ++total_trials_;

    if (auto* stats = find_candidate(candidate_id)) {
        ++stats->trials;
        iv.settling_ticks_remaining = stats->settling_ticks;
        stats->last_workload = current_workload_;
        stats->last_thermal_bucket = compute_thermal_bucket(telemetry);
        stats->last_regime = current_regime_;
        stats->last_before_summary = std::move(before_summary);
        stats->last_after_summary = std::move(after_summary);
    } else {
        iv.settling_ticks_remaining = 1;
    }

    active_interventions_.push_back(std::move(iv));
    sync_primary_state_from_active();
    save_persisted_stats();
    refresh_surface_readings();
    return true;
}

bool AutonomousTuner::apply_frequency_proposal(
    double requested_capacity_scale,
    uint32_t policy_index,
    std::string_view rationale,
    const TunerTelemetryInput& telemetry,
    uint64_t policy_now_ns,
    const std::vector<std::string>& resource_keys) {
    if (frequency_actuator_ == nullptr || recovery_required_) return false;

    OutcomeSample baseline_sample{};
    baseline_sample.timestamp_ns = policy_now_ns;
    baseline_sample.cpu_psi_some_pct = telemetry.cpu_psi;
    baseline_sample.memory_psi_some_pct = telemetry.memory_psi;
    baseline_sample.io_psi_some_pct = telemetry.io_psi;
    baseline_sample.cpu_psi_valid = telemetry.cpu_psi_valid;
    baseline_sample.memory_psi_valid = telemetry.memory_psi_valid;
    baseline_sample.io_psi_valid = telemetry.io_psi_valid;
    const auto& features = telemetry.frequency_policy_evidence.telemetry;
    baseline_sample.ebpf_wakeup_rate_per_sec = features.wakeup_rate_per_sec;
    baseline_sample.ebpf_wakeup_valid = (features.valid_features & FeatureWakeupRate) != 0U;
    baseline_sample.workload_stable =
        telemetry.frequency_policy_evidence.classification.stable &&
        telemetry.frequency_policy_evidence.classification.workload == DetectedWorkloadClass::CpuBound;

    ++total_trials_;
    if (auto* stats = find_candidate("cpu.frequency.tiered")) {
        ++stats->trials;
        stats->last_workload = current_workload_;
        stats->last_thermal_bucket = compute_thermal_bucket(telemetry);
        stats->last_regime = current_regime_;
    }

    if (!frequency_actuator_->apply_policy(
            policy_index, requested_capacity_scale, baseline_sample, policy_now_ns)) {
        ++total_rejected_;
        if (auto* stats = find_candidate("cpu.frequency.tiered")) {
            ++stats->rejected_count;
        }
        last_verdict_ = "REJECTED";
        last_verdict_enum_ = TunerVerdict::Rejected;
        current_rationale_ =
            "Tiered CPU frequency request denied by live thermal, LMh, OPP, or ownership admission.";
        sync_primary_state_from_active();
        save_persisted_stats();
        return false;
    }

    frequency_mutation_active_ = true;
    frequency_policy_index_ = policy_index;
    ActiveIntervention iv{};
    iv.candidate_id = "cpu.frequency.tiered";
    iv.candidate_label = "Tiered CPU Frequency OPP";
    iv.mutation_domain = MutationDomain::CpuPolicy;
    iv.evaluation_domain = EvaluationDomain::Cpu;
    iv.intent = "TieredCpuFrequency";
    iv.rationale = std::string(rationale);
    iv.is_frequency_policy = true;
    iv.thermal_stabilization = false;
    iv.observing = true;
    iv.retained = false;
    iv.obs_ticks = 0;
    iv.obs_window = obs_window_;
    iv.before_io_psi = telemetry.io_psi_valid ? telemetry.io_psi : 0.0;
    iv.before_mem_psi = telemetry.memory_psi_valid ? telemetry.memory_psi : 0.0;
    iv.before_cpu_psi = telemetry.cpu_psi_valid ? telemetry.cpu_psi : 0.0;
    iv.before_thermal = telemetry.thermal_pressure_valid ? telemetry.thermal_pressure : 0.0;
    iv.before_temperature_celsius = telemetry.current_temperature_celsius;
    iv.before_io_valid = telemetry.io_psi_valid;
    iv.before_mem_valid = telemetry.memory_psi_valid;
    iv.before_cpu_valid = telemetry.cpu_psi_valid;
    iv.before_thermal_valid = telemetry.thermal_pressure_valid;
    iv.verdict = "PENDING";
    iv.verdict_enum = TunerVerdict::Pending;
    iv.resource_keys = resource_keys;

    last_verdict_ = "PENDING";
    last_verdict_enum_ = TunerVerdict::Pending;
    last_utility_delta_ = 0.0;
    active_interventions_.push_back(std::move(iv));
    sync_primary_state_from_active();
    save_persisted_stats();
    return true;
}

bool AutonomousTuner::evaluate_intervention_window(
    ActiveIntervention& iv,
    const TunerTelemetryInput& latest_telemetry) {
    if (!iv.observing || iv.obs_ticks == 0) return true;

    const auto mean = [](double sum, uint32_t count) noexcept {
        return count ? sum / static_cast<double>(count) : 0.0;
    };
    const double after_io = mean(iv.sum_after_io_psi, iv.valid_io_samples_in_window);
    const double after_mem = mean(iv.sum_after_mem_psi, iv.valid_mem_samples_in_window);
    const double after_cpu = mean(iv.sum_after_cpu_psi, iv.valid_cpu_samples_in_window);
    const double after_therm = mean(iv.sum_after_thermal, iv.valid_pressure_samples_in_window);

    if (!verify_intervention_ownership(iv)) {
        if (retry_contended_intervention_ownership(iv)) {
            save_persisted_stats();
            return true;
        }
        last_verdict_ = "OWNERSHIP_LOST";
        last_verdict_enum_ = TunerVerdict::OwnershipLost;
        ++total_rolled_back_;
        record_ownership_loss(find_candidate(iv.candidate_id));
        (void)rollback_intervention(iv, "External node ownership lost during observation window.");
        save_persisted_stats();
        return false;
    }

    const uint32_t target_samples =
        iv.evaluation_domain == EvaluationDomain::Io ? iv.valid_io_samples_in_window :
        iv.evaluation_domain == EvaluationDomain::Memory ? iv.valid_mem_samples_in_window :
        iv.evaluation_domain == EvaluationDomain::Cpu ? iv.valid_cpu_samples_in_window :
        iv.valid_pressure_samples_in_window;

    if (target_samples < 2) {
        last_verdict_ = "INCONCLUSIVE";
        last_verdict_enum_ = TunerVerdict::Inconclusive;
        last_utility_delta_ = 0.0;
        iv.observing = false;
        iv.settling_ticks_remaining = 0;
        ++total_rolled_back_;
        ++total_inconclusive_;
        if (auto* stats = find_candidate(iv.candidate_id)) {
            ++stats->inconclusive_count;
            ++stats->consecutive_non_improved;
            stats->last_rollback_reason = "Insufficient valid target-domain samples";
        }
        (void)rollback_intervention(iv, "Target-domain evidence incomplete; reverted to OEM baseline.");
        update_ucb_scores();
        save_persisted_stats();
        return false;
    }

    const bool io_valid = iv.before_io_valid && iv.valid_io_samples_in_window >= 2;
    const bool mem_valid = iv.before_mem_valid && iv.valid_mem_samples_in_window >= 2;
    const bool cpu_valid = iv.before_cpu_valid && iv.valid_cpu_samples_in_window >= 2;
    const bool thermal_valid = iv.before_thermal_valid && iv.valid_pressure_samples_in_window >= 2;
    const double io_gain = io_valid ? iv.before_io_psi - after_io : 0.0;
    const double mem_gain = mem_valid ? iv.before_mem_psi - after_mem : 0.0;
    const double cpu_gain = cpu_valid ? iv.before_cpu_psi - after_cpu : 0.0;
    const double therm_gain = thermal_valid ? iv.before_thermal - after_therm : 0.0;
    const double therm_penalty = thermal_valid ? std::max(0.0, after_therm - iv.before_thermal - 2.0) : 0.0;

    double utility_delta = 0.0;
    if (iv.intent == "ThermalStabilization") {
        const bool has_thermal_evidence =
            iv.valid_thermal_samples_in_window >= 2 && iv.valid_pressure_samples_in_window >= 2;
        if (!has_thermal_evidence ||
            (latest_telemetry.current_temperature_celsius < 0.0 && !latest_telemetry.temperature_velocity_valid)) {
            last_verdict_ = "INCONCLUSIVE";
            last_verdict_enum_ = TunerVerdict::Inconclusive;
            last_utility_delta_ = 0.0;
            iv.observing = false;
            iv.settling_ticks_remaining = 0;
            ++total_rolled_back_;
            ++total_inconclusive_;
            if (auto* stats = find_candidate(iv.candidate_id)) {
                ++stats->inconclusive_count;
                ++stats->consecutive_non_improved;
                stats->last_rollback_reason =
                    "Inconclusive: thermal sensor telemetry unavailable or insufficient samples";
                const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
                stats->cooldown_ticks = std::min<uint32_t>(32U, 4U * (1U << shift));
            }
            const std::string msg =
                "Trial " + iv.candidate_id +
                " inconclusive due to missing thermal sensor evidence; reverted to OEM baseline.";
            (void)rollback_intervention(iv, msg);
            update_ucb_scores();
            save_persisted_stats();
            return false;
        }

        const double avg_temp_vel = (iv.valid_thermal_samples_in_window > 0)
            ? (iv.sum_after_temp_vel / static_cast<double>(iv.valid_thermal_samples_in_window))
            : latest_telemetry.temperature_velocity_celsius_per_sec;

        // Thermal outcomes are judged against both independent safety signals.
        // Neither a lower pressure reading nor a cooling trend may mask regression
        // in the other signal.
        const bool temperature_cooling = avg_temp_vel < -0.05;
        const bool temperature_not_warming = avg_temp_vel <= 0.05;
        const bool pressure_nonworsening = thermal_valid &&
            (after_therm <= iv.before_thermal + 0.50) &&
            ((iv.max_after_thermal - iv.min_after_thermal) <= 2.0);
        const bool thermal_objective_met =
            temperature_not_warming && pressure_nonworsening;

        // Preserve workload responsiveness: reject a thermal win that materially
        // worsens CPU/I/O contention or memory stalls during the same window.
        const bool workload_regressed =
            (io_valid && io_gain < -1.5) ||
            (cpu_valid && cpu_gain < -3.0) ||
            (mem_valid && mem_gain < -1.5);

        if (temperature_cooling && pressure_nonworsening) {
            const double cooling_gain = std::min(1.2, (-avg_temp_vel) * 1.5);
            utility_delta = 0.45 * therm_gain + cooling_gain +
                0.20 * io_gain + 0.15 * cpu_gain;
        } else if (thermal_objective_met) {
            utility_delta = std::max(0.0, therm_gain) * 0.40 +
                0.20 * io_gain + 0.15 * cpu_gain;
        } else {
            utility_delta = -0.50 - std::max(0.0, avg_temp_vel) * 1.5;
        }
        if (!thermal_objective_met || workload_regressed) {
            utility_delta = std::min(utility_delta, -0.25);
        }
    } else if (iv.intent == "IoLatencyRelief") {
        utility_delta = 1.40 * io_gain + (cpu_valid ? 0.35 * cpu_gain : 0.0) - 0.60 * therm_penalty;
    } else if (iv.intent == "MemoryStallRelief") {
        utility_delta = 1.60 * mem_gain + (io_valid ? 0.45 * io_gain : 0.0) - 0.60 * therm_penalty;
    } else if (iv.intent == "InteractiveCpuPacing") {
        utility_delta = 1.50 * cpu_gain + (io_valid ? 0.35 * io_gain : 0.0) - 0.65 * therm_penalty;
    } else {
        utility_delta = 0.0;
    }

    iv.last_utility_delta = utility_delta;
    last_utility_delta_ = utility_delta;
    iv.observing = false;
    iv.settling_ticks_remaining = 0;

    auto* stats = find_candidate(iv.candidate_id);

    double target_spread = 0.0, target_baseline = 0.0, target_after = 0.0;
    std::string_view target_metric = "unavailable";
    size_t noise_index = 0;
    switch (iv.evaluation_domain) {
        case EvaluationDomain::Io:
            target_spread = iv.max_after_io_psi - iv.min_after_io_psi;
            target_baseline = iv.before_io_psi;
            target_after = after_io;
            target_metric = "io_psi";
            noise_index = 0;
            break;
        case EvaluationDomain::Memory:
            target_spread = iv.max_after_mem_psi - iv.min_after_mem_psi;
            target_baseline = iv.before_mem_psi;
            target_after = after_mem;
            target_metric = "memory_psi";
            noise_index = 1;
            break;
        case EvaluationDomain::Cpu:
            target_spread = iv.max_after_cpu_psi - iv.min_after_cpu_psi;
            target_baseline = iv.before_cpu_psi;
            target_after = after_cpu;
            target_metric = "cpu_psi";
            noise_index = 2;
            break;
        case EvaluationDomain::Thermal:
            target_spread = iv.max_after_thermal - iv.min_after_thermal;
            target_baseline = iv.before_thermal;
            target_after = after_therm;
            target_metric = "thermal_pressure";
            noise_index = 3;
            break;
        default:
            break;
    }

    const double domain_weight =
        iv.evaluation_domain == EvaluationDomain::Io ? 0.07 :
        iv.evaluation_domain == EvaluationDomain::Memory ? 0.08 :
        iv.evaluation_domain == EvaluationDomain::Cpu ? 0.075 : 0.0225;
    const double measured_noise = std::clamp(target_spread * domain_weight, 0.02, 0.45);
    evaluation_noise_floors_[noise_index] = std::clamp(
        0.85 * evaluation_noise_floors_[noise_index] + 0.15 * measured_noise, 0.02, 0.45);
    noise_floor_ = evaluation_noise_floors_[noise_index];
    const double improved_threshold = std::max(0.25, 2.0 * noise_floor_);
    const double regressed_threshold = -(0.20 + noise_floor_);
    const bool high_variance_inconclusive =
        target_spread > std::max(6.0, std::abs(target_baseline) * 1.2) && std::abs(utility_delta) < 0.50;

    if (high_variance_inconclusive) {
        last_verdict_ = "INCONCLUSIVE";
        last_verdict_enum_ = TunerVerdict::Inconclusive;
        ++total_rolled_back_;
        ++total_inconclusive_;
        if (stats != nullptr) {
            ++stats->inconclusive_count;
            ++stats->consecutive_non_improved;
            stats->last_rollback_reason = "Inconclusive: sample variance exceeded noise floor";
            const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
            stats->cooldown_ticks = std::min<uint32_t>(32U, 4U * (1U << shift));
        }
        const std::string msg =
            "Trial " + iv.candidate_id +
            " inconclusive due to high sample variance; reverted to OEM baseline.";
        (void)rollback_intervention(iv, msg);
        current_rationale_ = msg;
        update_ucb_scores();
        save_persisted_stats();
        return false;
    }

    ++total_reward_samples_;
    if (stats != nullptr) {
        const double previous = static_cast<double>(stats->reward_samples);
        ++stats->reward_samples;
        stats->mean_utility_delta = (stats->mean_utility_delta * previous + utility_delta) /
                                    static_cast<double>(stats->reward_samples);
    }

    bool keep_active = false;
    if (utility_delta >= improved_threshold) {
        iv.retained = true;
        iv.verdict = "IMPROVED";
        iv.verdict_enum = TunerVerdict::Improved;
        last_verdict_ = "IMPROVED";
        last_verdict_enum_ = TunerVerdict::Improved;
        ++total_improved_;
        if (stats != nullptr) {
            const auto regime_index = static_cast<std::size_t>(stats->last_regime);
            if (regime_index < stats->regime_reward_samples.size()) {
                const uint32_t samples = stats->regime_reward_samples[regime_index];
                const double reward = std::clamp(0.5 + utility_delta, 0.05, 2.50);
                ++stats->regime_reward_samples[regime_index];
                stats->regime_mean_utility[regime_index] = samples == 0U ? reward :
                    (stats->regime_mean_utility[regime_index] * static_cast<double>(samples) + reward) /
                    static_cast<double>(samples + 1U);
                if (stats->regime_cooldown_ticks[regime_index] > 0U) {
                    stats->regime_cooldown_ticks[regime_index] = 0U;
                }
            }
            ++stats->improved_count;
            stats->successes = stats->improved_count;
            stats->consecutive_non_improved = 0;
            stats->adaptive_scale = std::min(1.0, stats->adaptive_scale + 0.15);
            stats->mean_utility = std::clamp(
                stats->mean_utility + 0.30 * (std::min(2.5, utility_delta) - stats->mean_utility),
                0.05,
                2.50);
        }
        iv.rationale += " [Verified IMPROVED: ΔU=+" +
            std::to_string(utility_delta).substr(0, 4) +
            ", " + std::string(target_metric) + " " + std::to_string(target_baseline).substr(0, 5) +
            " -> " + std::to_string(target_after).substr(0, 5) + "% -> RETAINED]";
        current_rationale_ = iv.rationale;
        keep_active = true;
    } else if (utility_delta < regressed_threshold) {
        last_verdict_ = "REGRESSED";
        last_verdict_enum_ = TunerVerdict::Regressed;
        ++total_rolled_back_;
        if (stats != nullptr) {
            const auto regime_index = static_cast<std::size_t>(stats->last_regime);
            if (regime_index < stats->regime_reward_samples.size()) {
                const uint32_t samples = stats->regime_reward_samples[regime_index];
                const double reward = std::clamp(0.5 + utility_delta, 0.05, 2.50);
                ++stats->regime_reward_samples[regime_index];
                stats->regime_mean_utility[regime_index] = samples == 0U ? reward :
                    (stats->regime_mean_utility[regime_index] * static_cast<double>(samples) + reward) /
                    static_cast<double>(samples + 1U);
                ++stats->regime_regressions[regime_index];
                const uint32_t shift = std::min<uint32_t>(3U, stats->regime_regressions[regime_index] - 1U);
                stats->regime_cooldown_ticks[regime_index] = std::min<uint32_t>(64U, 8U * (1U << shift));
            }
            ++stats->regressed_count;
            stats->rollbacks = stats->regressed_count;
            ++stats->consecutive_non_improved;
            stats->adaptive_scale = std::max(0.25, stats->adaptive_scale * 0.75);
            stats->mean_utility = std::max(0.05, stats->mean_utility - 0.35);
            stats->last_rollback_reason = "Regressed: delta_U=" + std::to_string(utility_delta).substr(0, 5);
            const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
            stats->cooldown_ticks = std::min<uint32_t>(64U, 8U * (1U << shift));
        }
        const std::string msg =
            "Trial " + iv.candidate_id + " regressed (ΔU=" +
            std::to_string(utility_delta).substr(0, 5) +
            ", " + std::string(target_metric) + " " + std::to_string(target_baseline).substr(0, 5) +
            " -> " + std::to_string(target_after).substr(0, 5) +
            "%); rolled back to OEM baseline.";
        (void)rollback_intervention(iv, msg);
        current_rationale_ = msg;
        keep_active = false;
    } else {
        last_verdict_ = "NEUTRAL";
        last_verdict_enum_ = TunerVerdict::Neutral;
        ++total_rolled_back_;
        if (stats != nullptr) {
            const auto regime_index = static_cast<std::size_t>(stats->last_regime);
            if (regime_index < stats->regime_reward_samples.size()) {
                const uint32_t samples = stats->regime_reward_samples[regime_index];
                const double reward = std::clamp(0.5 + utility_delta, 0.05, 2.50);
                ++stats->regime_reward_samples[regime_index];
                stats->regime_mean_utility[regime_index] = samples == 0U ? reward :
                    (stats->regime_mean_utility[regime_index] * static_cast<double>(samples) + reward) /
                    static_cast<double>(samples + 1U);
                const uint32_t shift = std::min<uint32_t>(3U, stats->regime_regressions[regime_index]);
                stats->regime_cooldown_ticks[regime_index] = std::max<uint32_t>(
                    stats->regime_cooldown_ticks[regime_index], 4U * (1U << shift));
            }
            ++stats->neutral_count;
            ++stats->consecutive_non_improved;
            stats->adaptive_scale = std::max(0.50, stats->adaptive_scale * 0.90);
            stats->mean_utility = std::clamp(stats->mean_utility + 0.30 * (0.50 - stats->mean_utility), 0.05, 2.50);
            stats->last_rollback_reason = "Within noise floor; reverted per Invariant 8";
            const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
            stats->cooldown_ticks = std::min<uint32_t>(32U, 4U * (1U << shift));
        }
        const std::string msg =
            "Trial " + iv.candidate_id + " neutral (ΔU=" +
            std::to_string(utility_delta).substr(0, 4) +
            "); reverted to OEM baseline per Invariant 8 (no unverified retention).";
        (void)rollback_intervention(iv, msg);
        current_rationale_ = msg;
        keep_active = false;
    }

    update_ucb_scores();
    save_persisted_stats();
    return keep_active;
}

void AutonomousTuner::evaluate_observation_window(const TunerTelemetryInput& latest_telemetry) {
    if (active_interventions_.empty()) return;
    if (!evaluate_intervention_window(active_interventions_.front(), latest_telemetry)) {
        active_interventions_.erase(active_interventions_.begin());
    }
    sync_primary_state_from_active();
}

AutonomousDecisionSnapshot AutonomousTuner::step(const TunerTelemetryInput& raw_telemetry) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    TunerTelemetryInput telemetry = raw_telemetry;
    latest_current_temperature_ = telemetry.current_temperature_celsius;
    latest_temperature_velocity_ = telemetry.temperature_velocity_celsius_per_sec;
    latest_temperature_velocity_valid_ = telemetry.temperature_velocity_valid;
    latest_pressure_velocity_ = telemetry.thermal_pressure_velocity;

    if (!operational_) {
        return snapshot();
    }

    if (!telemetry.temperature_velocity_valid && std::abs(telemetry.thermal_trend_celsius) > 1e-6) {
        telemetry.temperature_velocity_celsius_per_sec = telemetry.thermal_trend_celsius;
        telemetry.temperature_velocity_valid = true;
        telemetry.temperature_samples_count = std::max(telemetry.temperature_samples_count, 2U);
        latest_temperature_velocity_ = telemetry.temperature_velocity_celsius_per_sec;
        latest_temperature_velocity_valid_ = true;
    }

    if (telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure)) {
        if (has_prev_thermal_ && std::abs(telemetry.thermal_pressure_velocity) < 1e-6) {
            telemetry.thermal_pressure_velocity = telemetry.thermal_pressure - prev_thermal_pressure_;
            latest_pressure_velocity_ = telemetry.thermal_pressure_velocity;
        }
        prev_thermal_pressure_ = telemetry.thermal_pressure;
        has_prev_thermal_ = true;
    } else {
        telemetry.thermal_pressure_velocity = 0.0;
        latest_pressure_velocity_ = 0.0;
    }

    const bool emergency_boundary =
        telemetry.thermal_emergency ||
        (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 88.0);
    if (emergency_boundary) {
        current_objective_ = OptimizationObjective::EmergencyMitigation;
        coordinator_.emergency_preempt_all("ThermalEmergency");
    }

    if (recovery_required_) {
        if (!rollback_active("Retrying pending rollback recovery")) {
            return snapshot();
        }
    }

    bool cooldown_changed = false;
    for (auto& c : candidates_) {
        if (c.cooldown_ticks > 0) {
            --c.cooldown_ticks;
            cooldown_changed = true;
        }
        if (c.ownership_quarantine_ticks > 0) {
            --c.ownership_quarantine_ticks;
            cooldown_changed = true;
        }
        for (auto& regime_cooldown : c.regime_cooldown_ticks) {
            if (regime_cooldown > 0U) {
                --regime_cooldown;
                cooldown_changed = true;
            }
        }
    }
    if (cooldown_changed) {
        save_persisted_stats();
    }

    current_workload_ = classify_workload(telemetry);
    current_regime_ = classify_regime(telemetry, current_workload_);
    // Foreground render-thread migration is scoped to RenderBooster's reactor-owned
    // burst lease. timerfd expiry and thermal admission perform deterministic restore;
    // workload classification alone must not keep app threads migrated.

    if (emergency_boundary) {
        current_objective_ = OptimizationObjective::EmergencyMitigation;
        current_bottleneck_ = BottleneckKind::ThermalEmergency;
        if (!active_interventions_.empty() || frequency_mutation_active_ ||
            (frequency_actuator_ && frequency_actuator_->mutation_active())) {
            last_verdict_ = "ABORTED";
            last_verdict_enum_ = TunerVerdict::Aborted;
            for (const auto& iv : active_interventions_) {
                if (auto* stats = find_candidate(iv.candidate_id)) ++stats->aborted_count;
                ++total_rolled_back_;
            }
            if (active_interventions_.empty()) {
                ++total_rolled_back_;
            }
            (void)rollback_active("Thermal emergency gate active; restored OEM baseline.");
            save_persisted_stats();
        }
        current_intent_ = "ThermalHoldAndProtect";
        current_rationale_ = "Emergency thermal boundary reached; all domains locked to OEM baseline.";
        last_evaluated_candidates_.clear();
        last_decision_plan_rationale_ = current_rationale_;
        return snapshot();
    }

    // Evaluate optional Tiered CPU Frequency Policy input so it competes globally on utility.
    const uint64_t policy_now_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    const auto policy_index = select_frequency_policy_index();
    bool frequency_policy_evidence_ready = false;
    PolicyDecision frequency_decision{};
    if (frequency_actuator_ != nullptr && tiered_policy_router_ != nullptr &&
        telemetry.frequency_policy_evidence_valid && policy_index.has_value() &&
        policy_now_ns != 0U && !recovery_required_ &&
        !coordinator_.is_emergency_recovery_active()) {
        PolicyEvidence policy_ev = telemetry.frequency_policy_evidence;
        policy_ev.evaluated_at_ns = policy_now_ns;
        policy_ev.mutation_runtime_ready = true;
        policy_ev.cpu_frequency_capability_valid =
            frequency_mutation_active_ ||
            frequency_actuator_->can_apply_policy(*policy_index, 1.02, policy_now_ns);
        frequency_decision = tiered_policy_router_->evaluate(platform_context_, policy_ev);
        frequency_policy_evidence_ready = true;
    }
    frequency_policy_decision_ = frequency_decision;
    const bool tiered_cpu_policy_owned =
        frequency_actuator_ != nullptr && tiered_policy_router_ != nullptr &&
        telemetry.frequency_policy_evidence_valid &&
        telemetry.frequency_policy_evidence.classification.workload == DetectedWorkloadClass::CpuBound;
    const bool frequency_step_requested =
        frequency_policy_evidence_ready &&
        frequency_decision.action == PolicyAction::CpuFrequencyStep &&
        frequency_decision.requires_mutation_admission &&
        (frequency_decision.tier == OptimizationTier::Tier1InteractiveResponse ||
         frequency_decision.tier == OptimizationTier::Tier2SustainedOptimization);

    // 1. Global Environment Discovery & 2. Global Evidence Correlation
    const std::string vm_page_cluster_path = resolve_vm_node_path("vm.page_cluster");
    const std::string vm_cache_pressure_path = resolve_vm_node_path("vm.vfs_cache_pressure");
    EnvironmentDiscoveryInput env_input{};
    env_input.sys_root = sys_root_;
    env_input.proc_root = proc_root_;
    env_input.zram_active = zram_active_;
    env_input.vm_page_cluster_path = vm_page_cluster_path;
    env_input.vm_vfs_cache_pressure_path = vm_cache_pressure_path;
    env_input.block_queues = &block_queues_;
    env_input.cpufreq_governor_dirs = &cpufreq_governor_dirs_;
    env_input.kernel_node_capabilities = &kernel_node_capabilities_;
    env_input.device_profile = device_profile_;
    env_input.frequency_actuator_configured =
        frequency_actuator_ != nullptr && tiered_policy_router_ != nullptr;
    env_input.frequency_policy_index = policy_index;
    env_input.tiered_policy_decision = frequency_decision;
    env_input.tiered_policy_ready = frequency_policy_evidence_ready;
    env_input.tiered_cpu_policy_owned = tiered_cpu_policy_owned;
    env_input.uclamp_ready = (foreground_task_resolver_ != nullptr);

    const EnvironmentCapabilities env = EnvironmentDiscovery::discover(env_input);
    const bool in_thermal_recovery_before_step = (thermal_recovery_ticks_ > 0);
    const CorrelatedEvidence evidence =
        EvidenceCorrelator::correlate(telemetry, env, in_thermal_recovery_before_step);
    current_bottleneck_ = evidence.diagnosed_bottleneck;

    // Thermal Stabilization Arbitration (preempts non-stabilizing performance mutations)
    if (evidence.thermal_stabilization_required) {
        current_objective_ = OptimizationObjective::ThermalStabilization;
        thermal_recovery_ticks_ = 2;

        bool rolled_non_thermal = false;
        for (size_t idx = active_interventions_.size(); idx > 0; --idx) {
            auto& iv = active_interventions_[idx - 1U];
            if (!iv.thermal_stabilization) {
                last_verdict_ = "ABORTED";
                last_verdict_enum_ = TunerVerdict::Aborted;
                if (auto* stats = find_candidate(iv.candidate_id)) ++stats->aborted_count;
                ++total_rolled_back_;
                (void)rollback_intervention(
                    iv, "Preempting performance mutation for active ThermalStabilization.");
                active_interventions_.erase(
                    active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                rolled_non_thermal = true;
            }
        }
        if (rolled_non_thermal) {
            sync_primary_state_from_active();
            save_persisted_stats();
        }
        if (active_interventions_.empty()) {
            current_intent_ = "ThermalStabilization";
            current_rationale_ =
                "Thermal headroom constrained; switching objective to ThermalStabilization.";
        }
    } else if (telemetry.thermal_pressure_valid && !telemetry.thermal_guard_active) {
        bool rolled_thermal = false;
        for (size_t idx = active_interventions_.size(); idx > 0; --idx) {
            auto& iv = active_interventions_[idx - 1U];
            if (iv.thermal_stabilization) {
                (void)rollback_intervention(
                    iv, "Thermal pressure subsided below hysteresis threshold; entering ThermalRecovery.");
                active_interventions_.erase(
                    active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                rolled_thermal = true;
            }
        }
        if (rolled_thermal) {
            thermal_recovery_ticks_ = 2;
            sync_primary_state_from_active();
        }
    }

    const bool in_thermal_recovery = (thermal_recovery_ticks_ > 0);
    if (thermal_recovery_ticks_ > 0) {
        const bool temp_stable_or_falling =
            (!telemetry.temperature_velocity_valid) ||
            (telemetry.temperature_velocity_celsius_per_sec <= 0.05);
        if (telemetry.thermal_pressure_valid && telemetry.thermal_pressure <= 46.0 &&
            temp_stable_or_falling) {
            thermal_recovery_ticks_ = 0;
        } else {
            --thermal_recovery_ticks_;
        }
    }

    // Required-Evidence Completeness Gate (fail-closed per domain or globally when idle)
    if (!active_interventions_.empty()) {
        bool any_evidence_dropped = false;
        for (size_t idx = active_interventions_.size(); idx > 0; --idx) {
            auto& iv = active_interventions_[idx - 1U];
            const bool domain_evidence_ok = iv.is_frequency_policy
                ? (telemetry.cpu_psi_valid && telemetry.memory_psi_valid &&
                   telemetry.io_psi_valid && telemetry.thermal_pressure_valid)
                : ((iv.evaluation_domain == EvaluationDomain::Io && telemetry.io_psi_valid) ||
                   (iv.evaluation_domain == EvaluationDomain::Memory && telemetry.memory_psi_valid) ||
                   (iv.evaluation_domain == EvaluationDomain::Cpu && telemetry.cpu_psi_valid) ||
                   (iv.evaluation_domain == EvaluationDomain::Thermal && telemetry.thermal_pressure_valid));
            if (!domain_evidence_ok) {
                any_evidence_dropped = true;
                last_verdict_ = "INCONCLUSIVE";
                last_verdict_enum_ = TunerVerdict::Inconclusive;
                ++total_inconclusive_;
                ++total_rolled_back_;
                if (auto* stats = find_candidate(iv.candidate_id)) {
                    ++stats->inconclusive_count;
                    ++stats->consecutive_non_improved;
                    stats->last_rollback_reason = "Incomplete PSI evidence during observation/retention";
                }
                const bool restored = rollback_intervention(
                    iv, "Incomplete PSI evidence; releasing candidate and restoring OEM baseline.");
                active_interventions_.erase(
                    active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                if (!restored) {
                    last_verdict_ = "RECOVERY_REQUIRED";
                    last_verdict_enum_ = TunerVerdict::RecoveryRequired;
                }
                save_persisted_stats();
            }
        }
        if (any_evidence_dropped && active_interventions_.empty()) {
            sync_primary_state_from_active();
            return snapshot();
        }
    } else if (!evidence.psi_complete && !evidence.thermal_stabilization_required) {
        current_objective_ = OptimizationObjective::HoldBaseline;
        current_intent_ = "HoldEquilibrium";
        current_rationale_ =
            "PSI evidence incomplete; workload mutation withheld while thermal policy remains active.";
        last_verdict_ = "INCONCLUSIVE";
        last_verdict_enum_ = TunerVerdict::Inconclusive;
        last_evaluated_candidates_.clear();
        last_decision_plan_rationale_ = current_rationale_;
        return snapshot();
    }

    current_objective_ = select_objective(
        telemetry, current_workload_, current_bottleneck_, in_thermal_recovery);

    if (contention_backoff_ticks_ > 0) {
        --contention_backoff_ticks_;
        return snapshot();
    }

    const bool io_bottleneck = telemetry.io_psi_valid && telemetry.io_psi >= 2.2;
    const bool mem_bottleneck = telemetry.memory_psi_valid && telemetry.memory_psi >= 2.0;

    // Stage 1 & 2: Advance all active interventions independently (OBSERVING / RETAINED / TieredFreq)
    bool ownership_lost_this_tick = false;
    bool handled_observation_tick = false;

    for (size_t idx = active_interventions_.size(); idx > 0; --idx) {
        auto& iv = active_interventions_[idx - 1U];

        if (iv.is_frequency_policy) {
            if (!frequency_step_requested || !policy_index.has_value() ||
                *policy_index != frequency_policy_index_) {
                (void)rollback_intervention(
                    iv, "Tiered CPU-bound evidence or independent admission no longer qualifies.");
                active_interventions_.erase(
                    active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                if (!recovery_required_) {
                    last_verdict_ = "HOLD";
                    last_verdict_enum_ = TunerVerdict::Hold;
                    current_rationale_ =
                        "CPU frequency step released because Tier 1/Tier 2 evidence or admission expired.";
                }
                continue;
            }

            OutcomeSample sample{};
            sample.timestamp_ns = policy_now_ns;
            sample.cpu_psi_some_pct = telemetry.cpu_psi;
            sample.memory_psi_some_pct = telemetry.memory_psi;
            sample.io_psi_some_pct = telemetry.io_psi;
            sample.cpu_psi_valid = telemetry.cpu_psi_valid;
            sample.memory_psi_valid = telemetry.memory_psi_valid;
            sample.io_psi_valid = telemetry.io_psi_valid;
            const auto& features = telemetry.frequency_policy_evidence.telemetry;
            sample.ebpf_wakeup_rate_per_sec = features.wakeup_rate_per_sec;
            sample.ebpf_wakeup_valid = (features.valid_features & FeatureWakeupRate) != 0U;
            sample.workload_stable =
                telemetry.frequency_policy_evidence.classification.stable &&
                telemetry.frequency_policy_evidence.classification.workload == DetectedWorkloadClass::CpuBound;

            const OutcomeResult outcome = frequency_actuator_->observe(sample);
            frequency_mutation_active_ = frequency_actuator_->mutation_active();
            if (outcome.verdict == OutcomeVerdict::Pending) {
                iv.observing = true;
                iv.retained = false;
                iv.verdict = "PENDING";
                iv.verdict_enum = TunerVerdict::Pending;
                iv.rationale = std::string(frequency_decision.rationale);
                last_verdict_ = "PENDING";
                last_verdict_enum_ = TunerVerdict::Pending;
                handled_observation_tick = true;
            } else if (outcome.verdict == OutcomeVerdict::Improved && frequency_mutation_active_) {
                if (iv.verdict != "IMPROVED") {
                    ++total_reward_samples_;
                    ++total_improved_;
                    if (auto* stats = find_candidate(iv.candidate_id)) {
                        ++stats->reward_samples;
                        ++stats->improved_count;
                        stats->successes = stats->improved_count;
                    }
                }
                iv.observing = false;
                iv.retained = true;
                iv.verdict = "IMPROVED";
                iv.verdict_enum = TunerVerdict::Improved;
                iv.rationale = std::string(outcome.reason);
                last_verdict_ = "IMPROVED";
                last_verdict_enum_ = TunerVerdict::Improved;
            } else {
                if (outcome.verdict == OutcomeVerdict::Regressed) {
                    last_verdict_ = "REGRESSED";
                    last_verdict_enum_ = TunerVerdict::Regressed;
                    if (auto* stats = find_candidate(iv.candidate_id)) {
                        ++stats->reward_samples;
                        ++stats->regressed_count;
                        stats->rollbacks = stats->regressed_count;
                    }
                } else if (outcome.verdict == OutcomeVerdict::Neutral) {
                    last_verdict_ = "NEUTRAL";
                    last_verdict_enum_ = TunerVerdict::Neutral;
                    if (auto* stats = find_candidate(iv.candidate_id)) {
                        ++stats->reward_samples;
                        ++stats->neutral_count;
                    }
                } else {
                    last_verdict_ = "INCONCLUSIVE";
                    last_verdict_enum_ = TunerVerdict::Inconclusive;
                    ++total_inconclusive_;
                    if (auto* stats = find_candidate(iv.candidate_id)) {
                        ++stats->inconclusive_count;
                    }
                }
                if (!frequency_mutation_active_) ++total_rolled_back_;
                current_rationale_ = outcome.reason;
                if (frequency_actuator_->recovery_required()) {
                    (void)rollback_intervention(iv, outcome.reason);
                    recovery_required_ = true;
                    last_verdict_ = "RECOVERY_REQUIRED";
                    last_verdict_enum_ = TunerVerdict::RecoveryRequired;
                }
                if (!frequency_mutation_active_) {
                    active_interventions_.erase(
                        active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                }
            }
            continue;
        }

        if (iv.observing) {
            handled_observation_tick = true;
            if (!verify_intervention_ownership(iv)) {
                if (retry_contended_intervention_ownership(iv)) {
                    save_persisted_stats();
                    continue;
                }
                last_verdict_ = "OWNERSHIP_LOST";
                last_verdict_enum_ = TunerVerdict::OwnershipLost;
                ++total_rolled_back_;
                record_ownership_loss(find_candidate(iv.candidate_id));
                (void)rollback_intervention(
                    iv, "External node ownership lost during observation window.");
                active_interventions_.erase(
                    active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                save_persisted_stats();
                ownership_lost_this_tick = true;
                continue;
            }

            if (iv.settling_ticks_remaining > 0) {
                --iv.settling_ticks_remaining;
            }
            if (telemetry.io_psi_valid && std::isfinite(telemetry.io_psi) && telemetry.io_psi >= 0.0) {
                iv.sum_after_io_psi += telemetry.io_psi;
                iv.min_after_io_psi = std::min(iv.min_after_io_psi, telemetry.io_psi);
                iv.max_after_io_psi = std::max(iv.max_after_io_psi, telemetry.io_psi);
                ++iv.valid_io_samples_in_window;
            }
            if (telemetry.memory_psi_valid && std::isfinite(telemetry.memory_psi) && telemetry.memory_psi >= 0.0) {
                iv.sum_after_mem_psi += telemetry.memory_psi;
                iv.min_after_mem_psi = std::min(iv.min_after_mem_psi, telemetry.memory_psi);
                iv.max_after_mem_psi = std::max(iv.max_after_mem_psi, telemetry.memory_psi);
                ++iv.valid_mem_samples_in_window;
            }
            if (telemetry.cpu_psi_valid && std::isfinite(telemetry.cpu_psi) && telemetry.cpu_psi >= 0.0) {
                iv.sum_after_cpu_psi += telemetry.cpu_psi;
                iv.min_after_cpu_psi = std::min(iv.min_after_cpu_psi, telemetry.cpu_psi);
                iv.max_after_cpu_psi = std::max(iv.max_after_cpu_psi, telemetry.cpu_psi);
                ++iv.valid_cpu_samples_in_window;
            }
            if (telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure) && telemetry.thermal_pressure >= 0.0) {
                iv.sum_after_thermal += telemetry.thermal_pressure;
                iv.min_after_thermal = std::min(iv.min_after_thermal, telemetry.thermal_pressure);
                iv.max_after_thermal = std::max(iv.max_after_thermal, telemetry.thermal_pressure);
                ++iv.valid_pressure_samples_in_window;
            }
            if (telemetry.temperature_velocity_valid &&
                std::isfinite(telemetry.temperature_velocity_celsius_per_sec) &&
                telemetry.temperature_samples_count >= 2) {
                iv.sum_after_temp_vel += telemetry.temperature_velocity_celsius_per_sec;
                ++iv.valid_thermal_samples_in_window;
            }
            ++iv.obs_ticks;
            if (iv.obs_ticks >= iv.obs_window) {
                if (!evaluate_intervention_window(iv, telemetry)) {
                    active_interventions_.erase(
                        active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                }
            }
            continue;
        }

        if (iv.retained) {
            if (!verify_intervention_ownership(iv)) {
                if (retry_contended_intervention_ownership(iv)) {
                    save_persisted_stats();
                    continue;
                }
                last_verdict_ = "OWNERSHIP_LOST";
                last_verdict_enum_ = TunerVerdict::OwnershipLost;
                ++total_rolled_back_;
                record_ownership_loss(find_candidate(iv.candidate_id));
                (void)rollback_intervention(
                    iv, "External node ownership lost while transaction was retained.");
                active_interventions_.erase(
                    active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                save_persisted_stats();
                ownership_lost_this_tick = true;
                continue;
            }

            if (!io_bottleneck && !mem_bottleneck && telemetry.cpu_psi_valid &&
                telemetry.cpu_psi < 4.0 && !evidence.thermal_stabilization_required) {
                if (++iv.idle_streak >= 6) {
                    iv.idle_streak = 0;
                    last_verdict_ = "HOLD";
                    last_verdict_enum_ = TunerVerdict::Hold;
                    (void)rollback_intervention(
                        iv, "Workload returned to sustained idle; restored OEM baseline.");
                    active_interventions_.erase(
                        active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                }
            } else {
                iv.idle_streak = 0;
                bool domain_regressed = false;
                switch (iv.evaluation_domain) {
                    case EvaluationDomain::Io:
                        domain_regressed = telemetry.io_psi_valid && iv.before_io_valid &&
                            telemetry.io_psi > iv.before_io_psi + 4.0;
                        break;
                    case EvaluationDomain::Memory:
                        domain_regressed = telemetry.memory_psi_valid && iv.before_mem_valid &&
                            telemetry.memory_psi > iv.before_mem_psi + 3.5;
                        break;
                    case EvaluationDomain::Cpu:
                        domain_regressed = telemetry.cpu_psi_valid && iv.before_cpu_valid &&
                            telemetry.cpu_psi > iv.before_cpu_psi + 6.0;
                        if (!domain_regressed && telemetry.thermal_pressure_valid && iv.before_thermal_valid) {
                            domain_regressed = telemetry.thermal_pressure > iv.before_thermal + 8.0;
                        }
                        break;
                    case EvaluationDomain::Thermal:
                        domain_regressed = telemetry.thermal_pressure_valid && iv.before_thermal_valid &&
                            telemetry.thermal_pressure > iv.before_thermal + 8.0;
                        if (!domain_regressed && telemetry.temperature_velocity_valid) {
                            domain_regressed = telemetry.temperature_velocity_celsius_per_sec > 0.05;
                        }
                        break;
                    default:
                        break;
                }
                if (domain_regressed) {
                    if (auto* stats = find_candidate(iv.candidate_id)) {
                        stats->cooldown_ticks = 6;
                        stats->last_rollback_reason = "Retained metric guard exceeded on workload shift";
                        save_persisted_stats();
                    }
                    (void)rollback_intervention(
                        iv, "Workload regime shifted; releasing retained candidate to re-plan.");
                    active_interventions_.erase(
                        active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                }
            }
        }
    }

    const bool any_still_observing = std::any_of(
        active_interventions_.begin(), active_interventions_.end(),
        [](const ActiveIntervention& iv) noexcept { return iv.observing; });

    // 3 & 4 & 5: Global Candidate Evaluation & Compatible Intervention Planning
    // Wait for any active observation window to complete before starting new trials
    // so causal before/after attribution across domains stays clean and unconfounded.
    if (!handled_observation_tick && !any_still_observing &&
        !ownership_lost_this_tick && !recovery_required_ && decision_engine_) {
        std::vector<ActiveDomainReservation> active_reservations;
        active_reservations.reserve(active_interventions_.size());
        for (const auto& iv : active_interventions_) {
            active_reservations.push_back(ActiveDomainReservation{
                .domain = iv.mutation_domain,
                .candidate_id = iv.candidate_id,
                .resource_keys = iv.resource_keys,
            });
        }

        const auto eligibility_fn = [&](const CandidateProposal& proposal, std::string& reason_out) -> bool {
            const CandidateStats* st = find_candidate(proposal.id);
            if (st == nullptr) {
                reason_out = "Unregistered candidate";
                return false;
            }
            if (st->ownership_quarantine_ticks > 0U) {
                reason_out = "Candidate quarantined after external ownership loss";
                return false;
            }
            if (!is_candidate_eligible_for_context(
                    proposal.id, evidence.workload, evidence.thermal_bucket)) {
                reason_out = "Cooldown, missing contract, hardware precondition, or coordinator lock";
                return false;
            }
            return true;
        };

        const InterventionPlan plan = decision_engine_->run_cycle(
            env,
            evidence,
            candidates_,
            total_reward_samples_,
            active_reservations,
            coordinator_,
            controller_name_,
            eligibility_fn,
            worker_pool_.get());

        last_evaluated_candidates_.clear();
        last_evaluated_candidates_.reserve(plan.ranked_all.size());
        for (const auto& item : plan.ranked_all) {
            const bool was_selected = std::any_of(
                plan.selected.begin(), plan.selected.end(),
                [&](const EvaluatedCandidate& sel) noexcept {
                    return sel.proposal.id == item.proposal.id;
                });
            last_evaluated_candidates_.push_back(EvaluatedCandidateSummary{
                .id = item.proposal.id,
                .domain = item.proposal.domain_label,
                .intent = item.proposal.intent,
                .utility_score = item.utility_score,
                .empirical_score = item.empirical_score,
                .confidence = item.confidence,
                .risk_penalty = item.risk_penalty,
                .thermal_penalty = item.thermal_penalty,
                .eligible = item.eligible,
                .selected = was_selected,
                .rejection_reason = item.rejection_reason,
            });
        }
        last_decision_plan_rationale_ = plan.planning_rationale;

        if (!plan.selected.empty()) {
            for (const auto& chosen : plan.selected) {
                if (chosen.proposal.action_kind == CandidateActionKind::TieredFrequencyPolicy) {
                    (void)apply_frequency_proposal(
                        chosen.proposal.requested_capacity_scale,
                        chosen.proposal.frequency_policy_index,
                        chosen.proposal.rationale,
                        telemetry,
                        policy_now_ns,
                        chosen.proposal.resource_keys);
                } else {
                    std::vector<MutationStep> steps;
                    steps.reserve(chosen.proposal.steps.size());
                    for (const auto& s : chosen.proposal.steps) {
                        steps.push_back(MutationStep{
                            .path = s.path,
                            .target_value = s.target_value,
                            .relationship = s.relationship,
                            .relation = s.relation,
                        });
                    }
                    (void)apply_transaction(
                        chosen.proposal.id,
                        chosen.proposal.label,
                        chosen.proposal.intent,
                        chosen.proposal.rationale,
                        steps,
                        telemetry,
                        chosen.proposal.evaluation_domain,
                        chosen.proposal.resource_keys,
                        false);
                }
            }
        } else if (active_interventions_.empty() && in_thermal_recovery &&
                   !evidence.thermal_stabilization_required) {
            current_intent_ = "ThermalRecovery";
            current_rationale_ =
                "Hysteresis thermal recovery active; holding OEM baseline until thermal equilibrium stabilizes.";
        }
    }

    sync_primary_state_from_active();
    refresh_surface_readings();
    return snapshot();
}

AutonomousDecisionSnapshot AutonomousTuner::snapshot() const {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    AutonomousDecisionSnapshot out{};
    if (recovery_required_) {
        out.stage = "RECOVERY_REQUIRED";
    } else if (observing_ ||
               (frequency_mutation_active_ && last_verdict_ == "PENDING")) {
        out.stage = "OBSERVING";
    } else if (transaction_active_ ||
               (frequency_mutation_active_ && frequency_actuator_ &&
                frequency_actuator_->mutation_active())) {
        out.stage = "RETAINED";
    } else {
        out.stage = "PLANNED";
    }
    out.runtime_stage = operational_ ? "STAGE_ACTIVE_TUNING" : "STAGE_OBSERVING";
    out.active_intent = operational_ ? current_intent_ : "StartupBaselineObservation";
    out.intent = out.active_intent;
    out.workload_class = current_workload_;
    out.workload_regime = current_regime_;
    out.regime_name = std::string(regime_name(current_regime_));
    out.workload_name = std::string(workload_class_name(current_workload_));
    out.bottleneck = current_bottleneck_;
    out.bottleneck_name = std::string(bottleneck_kind_name(current_bottleneck_));
    out.objective = current_objective_;
    out.objective_name = std::string(objective_name(current_objective_));
    out.capability_fingerprint = compute_fingerprint();
    out.active_candidate_id = current_candidate_id_;
    out.active_candidate_label = current_candidate_label_;
    if (frequency_mutation_active_ && frequency_actuator_ &&
        frequency_actuator_->mutation_active() &&
        out.active_candidate_id == "baseline") {
        out.active_candidate_id = "cpu.frequency.tiered";
        out.active_candidate_label = "Tiered CPU Frequency OPP";
        out.active_intent = "TieredCpuFrequency";
        out.intent = "TieredCpuFrequency";
    }
    out.decision_rationale = current_rationale_;
    out.rationale = current_rationale_;
    out.last_verdict = last_verdict_;
    out.verdict = verdict_from_string(last_verdict_);
    out.last_utility_delta = last_utility_delta_;
    out.utility_delta = last_utility_delta_;
    out.noise_floor = noise_floor_;
    out.before_io_psi = before_io_psi_;
    out.after_io_psi = valid_io_samples_in_window_ > 0
        ? (sum_after_io_psi_ / static_cast<double>(valid_io_samples_in_window_))
        : before_io_psi_;
    out.before_mem_psi = before_mem_psi_;
    out.after_mem_psi = valid_mem_samples_in_window_ > 0
        ? (sum_after_mem_psi_ / static_cast<double>(valid_mem_samples_in_window_))
        : before_mem_psi_;
    out.before_cpu_psi = before_cpu_psi_;
    out.after_cpu_psi = valid_cpu_samples_in_window_ > 0
        ? (sum_after_cpu_psi_ / static_cast<double>(valid_cpu_samples_in_window_))
        : before_cpu_psi_;
    out.settling_ticks_remaining = settling_ticks_remaining_;
    out.observation_tick = obs_ticks_;
    out.observation_window = obs_window_;
    out.total_trials = total_trials_;
    out.total_improved = total_improved_;
    out.improved_count = total_improved_;
    out.total_rolled_back = total_rolled_back_;
    out.rolled_back_count = total_rolled_back_;
    out.total_inconclusive = total_inconclusive_;
    out.total_rejected = total_rejected_;
    out.ownership_contention_retries = total_contention_retries_;
    out.recovery_required = recovery_required_;
    out.thermal_stabilization_active = thermal_stabilization_active_;
    out.thermal_recovery_active = (thermal_recovery_ticks_ > 0);
    out.current_temperature_celsius = latest_current_temperature_;
    out.temperature_velocity_celsius_per_sec = latest_temperature_velocity_;
    out.temperature_velocity_valid = latest_temperature_velocity_valid_;
    out.thermal_pressure_velocity = latest_pressure_velocity_;
    out.block_queues = block_queues_;
    out.candidates = candidates_;
    out.discovered_resources = discovered_resources_;
    out.evaluated_candidates = last_evaluated_candidates_;
    out.decision_plan_rationale = last_decision_plan_rationale_;
    out.active_interventions.reserve(active_interventions_.size());
    for (const auto& iv : active_interventions_) {
        out.active_interventions.push_back(ActiveInterventionSummary{
            .candidate_id = iv.candidate_id,
            .candidate_label = iv.candidate_label,
            .domain = iv.mutation_domain,
            .evaluation_domain = iv.evaluation_domain,
            .intent = iv.intent,
            .stage = iv.observing ? "OBSERVING" : "RETAINED",
            .verdict = iv.verdict,
            .utility_delta = iv.last_utility_delta,
            .observation_tick = iv.obs_ticks,
            .observation_window = iv.obs_window,
            .settling_ticks_remaining = iv.settling_ticks_remaining,
        });
    }
    const auto page_cluster_path = resolve_vm_node_path("vm.page_cluster");
    const auto cache_pressure_path = resolve_vm_node_path("vm.vfs_cache_pressure");
    if (!page_cluster_path.empty()) out.vm_page_cluster = read_first_line(page_cluster_path);
    if (!cache_pressure_path.empty()) out.vm_vfs_cache_pressure = read_first_line(cache_pressure_path);
    return out;
}

} // namespace coreflow
