/**
 * @file autonomous_tuner.cpp
 * @brief Self-Directed Multi-Domain Transaction Planner, Central Mutation Coordinator & Empirical Outcome Evaluator
 * @license MIT
 */
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/foreground_task_resolver.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/config.hpp"
#include "coreflow/unique_fd.hpp"
#include "coreflow/transactional_frequency_actuator.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cmath>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
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

bool is_virtual_block_device(std::string_view name) {
    return name.rfind("loop", 0) == 0 || name.rfind("ram", 0) == 0 ||
           name.rfind("zram", 0) == 0 || name.rfind("dm-", 0) == 0 ||
           name.rfind("md", 0) == 0 || name.rfind("nbd", 0) == 0;
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
    size_t offset = 0;
    while (offset < data.size()) {
        const ssize_t written = ::write(fd, data.data() + offset, data.size() - offset);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) return false;
        offset += static_cast<size_t>(written);
    }
    return true;
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
    std::error_code ec;
    if (fs::exists(swaps_path, ec) && !ec) {
        std::ifstream in(swaps_path);
        std::string line;
        while (std::getline(in, line)) {
            if (line.find("zram") != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    return fs::exists(fs::path(sys_root) / "block/zram0", ec) && !ec;
}

uint64_t lookup_factory_u64(const std::string& path, uint64_t fallback_baseline) {
    uint64_t baseline = fallback_baseline;
    const auto audit = BaselineStore::instance().get_audit_trail();
    for (const auto& item : audit) {
        if (item.path == path) {
            uint64_t parsed = 0;
            if (parse_u64(item.factory_value, parsed) && parsed > 0) {
                baseline = parsed;
                break;
            }
        }
    }
    return baseline == 0 ? fallback_baseline : baseline;
}

uint64_t compute_relative_u64(
    const std::string& path,
    uint64_t fallback_baseline,
    double ratio,
    uint64_t min_bound,
    uint64_t max_bound,
    double adaptive_scale = 1.0) {
    const uint64_t baseline = lookup_factory_u64(path, fallback_baseline);
    const double clamped_scale = std::clamp(adaptive_scale, 0.25, 1.0);
    // Blend ratio toward 1.0 when adaptive_scale < 1.0 after prior rollbacks
    const double effective_ratio = 1.0 + (ratio - 1.0) * clamped_scale;
    const double calc = static_cast<double>(baseline) * effective_ratio;
    const uint64_t rounded = std::max<uint64_t>(1U, static_cast<uint64_t>(std::round(calc)));
    return std::clamp(rounded, min_bound, max_bound);
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
      sys_root_(std::move(sys_root)), proc_root_(std::move(proc_root)),
      device_profile_(std::move(profile)) {
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
        if (!applied_steps_.empty() ||
            (frequency_actuator_ &&
             (frequency_actuator_->mutation_active() || frequency_actuator_->recovery_required()))) {
            (void)rollback_active("Autonomous tuner teardown.");
        }
    }
    coordinator_.unregister_preemption_handler(controller_name_);
}

void AutonomousTuner::set_device_profile(DeviceProfileHandle profile) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    if (!applied_steps_.empty() || recovery_required_) return;
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
    // A failed apply may leave a journaled partial transaction before the
    // normal OBSERVING state is entered. The candidate identity and applied
    // steps are installed immediately after lease acquisition, so preemption
    // can still retry that exact rollback even when transaction_active_ is false.
    if (current_candidate_id_ != transaction_id ||
        domain_for_candidate(current_candidate_id_) != domain) {
        return false;
    }

    // A request can arrive at a safe point before the first write. rollback_active()
    // also acknowledges this no-write case and retires only this exact lease.
    const bool had_applied_steps = !applied_steps_.empty();
    last_verdict_ = "ABORTED";
    last_verdict_enum_ = TunerVerdict::Aborted;
    if (auto* stats = find_candidate(current_candidate_id_)) ++stats->preempted_count;
    if (had_applied_steps) ++total_rolled_back_;
    const bool restored = rollback_active(reason);
    if (!restored) {
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
    }
    save_persisted_stats();
    return restored;
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
    if (candidate_id.rfind("cpufreq.", 0) == 0 || candidate_id == "thermal.schedutil_damped_pacing") {
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
    if (id == "vm.zram_latency_relief" && !zram_active_) return false;
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
    if (telemetry.io_psi_valid && telemetry.io_psi >= 2.2) {
        return BottleneckKind::IoQueueContention;
    }
    if (telemetry.memory_psi_valid && telemetry.memory_psi >= 2.0) {
        return BottleneckKind::MemorySwapThrashing;
    }
    if (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 8.0) {
        return BottleneckKind::CpuSchedulingLatency;
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
    out << "CFTS7\t" << compute_fingerprint() << "\t"
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
            << sanitize_tsv_field(c.last_rollback_reason) << "\n";
    }
    std::string payload = out.str();
    if (!out.good() || payload.size() > 256U * 1024U - 64U) return;
    payload += "CFTS7_END\t" + std::to_string(tuner_state_checksum(payload)) + "\n";

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
    const size_t footer_pos7 = payload.rfind("CFTS7_END\t");
    const size_t footer_pos6 = payload.rfind("CFTS6_END\t");
    if (footer_pos7 != std::string::npos && footer_pos6 != std::string::npos) return;
    const size_t footer_pos = footer_pos7 != std::string::npos ? footer_pos7 : footer_pos6;
    if (footer_pos != std::string::npos) {
        if (footer_pos > 0 && payload[footer_pos - 1] != '\n') return;
        const std::string footer = payload.substr(footer_pos);
        std::istringstream footer_stream(footer);
        std::string extra;
        uint64_t stored_checksum = 0;
        if (!(footer_stream >> checksum_footer_magic >> stored_checksum) ||
            (checksum_footer_magic != "CFTS7_END" && checksum_footer_magic != "CFTS6_END") ||
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
    if ((magic == "CFTS7" && (!has_checksum || checksum_footer_magic != "CFTS7_END")) ||
        (magic == "CFTS6" && (!has_checksum || checksum_footer_magic != "CFTS6_END")) ||
        (magic != "CFTS7" && magic != "CFTS6" && has_checksum)) return;

    const std::string current_fp = compute_fingerprint();
    uint32_t persisted_reward_samples = 0;
    uint32_t loaded_inconclusive = 0;
    uint32_t loaded_rejected = 0;
    double loaded_noise_floor = 0.05;
    std::array<double, 4> loaded_domain_floors{{0.05, 0.05, 0.05, 0.05}};

    if (magic == "CFTS7" || magic == "CFTS6" || magic == "CFTS5" || magic == "CFTS4") {
        if (fp != current_fp ||
            !(hs >> persisted_reward_samples >> loaded_inconclusive >> loaded_rejected >> loaded_noise_floor) ||
            persisted_reward_samples > trials ||
            !std::isfinite(loaded_noise_floor) || loaded_noise_floor < 0.02 || loaded_noise_floor > 0.45) return;
        if (magic == "CFTS7" || magic == "CFTS6" || magic == "CFTS5") {
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

        const bool current_schema = magic == "CFTS7" || magic == "CFTS6" || magic == "CFTS5" || magic == "CFTS4";
        if (current_schema) {
            if (!(ls >> saved.reward_samples >> saved.improved >> saved.regressed >> saved.neutral >> saved.aborted)) return;
            if (magic == "CFTS7" && !(ls >> saved.preempted)) return;
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

        if (magic == "CFTS7" || magic == "CFTS6" || magic == "CFTS5" || magic == "CFTS4" || magic == "CFTS3") {
            if (!(ls >> saved.ownership_losses >> saved.quarantine_ticks)) return;
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
                                                : ResourcePermissionClass::VerifiedConstraints,
            .unit = "scheduler_token",
            .range_provenance = "sysfs:queue/scheduler",
            .baseline_value = active,
            .reason = writable ? "Verified kernel scheduler token list" : "Read-only scheduler node",
        });

        for (const auto& [leaf, unit_str, range_str] : {
                 std::tuple{"read_ahead_kb", "KiB", "[64..2048] relative to OEM baseline"},
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
                                               : ResourcePermissionClass::VerifiedConstraints,
                .unit = unit_str,
                .range_provenance = range_str,
                .baseline_value = raw,
                .reason = !valid_num ? "Non-numeric sysfs content"
                          : !leaf_w  ? "Read-only queue node"
                                     : "Verified numeric queue tunable",
            });
        }
    }

    // 2. Classify CPUFreq policy & governor nodes.
    if (device_profile_) {
        for (const auto& cap : device_profile_->node_capabilities) {
            if (cap.domain != HardwareDomain::Cpu ||
                cap.semantic_id != "cpufreq.governor_tunable.unverified") continue;
            ResourcePermissionClass permission = ResourcePermissionClass::IncompleteMetadata;
            if (cap.state == NodeCapabilityState::MutationEligible) {
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

        const std::string active_gov = DaemonConfig::trim(read_first_line((it->path() / "scaling_governor").string()));
        for (const char* gov : {"schedutil", "walt"}) {
            const fs::path gdir = it->path() / gov;
            if (!fs::is_directory(gdir, cpu_ec) || cpu_ec) continue;
            const std::string up_path = (gdir / "up_rate_limit_us").string();
            const std::string down_path = (gdir / "down_rate_limit_us").string();
            const bool gov_matches = active_gov.empty() || active_gov == gov;
            const bool both_exist = fs::exists(up_path, cpu_ec) && fs::exists(down_path, cpu_ec);
            const bool both_w = (::access(up_path.c_str(), R_OK | W_OK) == 0) &&
                                (::access(down_path.c_str(), R_OK | W_OK) == 0);

            ResourcePermissionClass perm = ResourcePermissionClass::VerifiedConstraints;
            std::string reason = "Verified EAS governor rate-limit pair";
            if (!both_exist) {
                perm = ResourcePermissionClass::IncompleteMetadata;
                reason = "Missing companion rate-limit node";
            } else if (!gov_matches) {
                perm = ResourcePermissionClass::WritableButUnsafe;
                reason = "scaling_governor (" + active_gov + ") does not match " + gov;
            } else if (!both_w) {
                perm = ResourcePermissionClass::ReadOnly;
                reason = "Governor rate-limit node is read-only";
            }
            discovered_resources_.push_back(DiscoveredResourceMetadata{
                .path = up_path,
                .domain = MutationDomain::CpuPolicy,
                .permission_class = perm,
                .unit = "microseconds",
                .range_provenance = "[500..10000] relative/bounded",
                .baseline_value = read_first_line(up_path),
                .reason = reason,
            });
        }
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
        ResourcePermissionClass perm = ResourcePermissionClass::VerifiedConstraints;
        std::string reason = "Verified VM tunable with active zram swap";
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

        for (const char* gov : {"schedutil", "walt"}) {
            if (!active_gov.empty() && active_gov != gov) {
                continue;
            }
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
    }
    std::sort(cpufreq_governor_dirs_.begin(), cpufreq_governor_dirs_.end());

    // Capture VM baseline nodes if accessible
    const auto page_cluster_path = resolve_vm_node_path("vm.page_cluster");
    const auto cache_pressure_path = resolve_vm_node_path("vm.vfs_cache_pressure");
    if (!page_cluster_path.empty()) (void)store.capture(page_cluster_path);
    if (!cache_pressure_path.empty()) (void)store.capture(cache_pressure_path);

    classify_discovered_resources();
    load_persisted_stats();
}

void AutonomousTuner::refresh_surface_readings() {
    zram_active_ = check_zram_swap_active(proc_root_, sys_root_);
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

bool AutonomousTuner::verify_active_ownership() const {
    auto& store = BaselineStore::instance();
    for (const auto& step : applied_steps_) {
        if (::access(step.path.c_str(), R_OK) != 0 ||
            !store.owns_expected_state(step.path, step.target_value)) {
            return false;
        }
    }
    return true;
}

bool AutonomousTuner::retry_contended_ownership() {
    if (applied_steps_.empty() || current_candidate_id_.empty() ||
        current_candidate_id_ == "baseline" || recovery_required_) return false;
    auto& coordinator = coordinator_;
    uint32_t attempt = 0;
    constexpr uint32_t kMaximumContentionRetries = 3;
    if (!coordinator.reserve_contention_retry(
            domain_for_candidate(current_candidate_id_), controller_name_,
            current_candidate_id_, kMaximumContentionRetries, &attempt)) return false;
    if (total_contention_retries_ < std::numeric_limits<uint32_t>::max()) ++total_contention_retries_;

    auto& store = BaselineStore::instance();
    for (const auto& step : applied_steps_) {
        if (!store.reassert_mutation_from_oem_baseline(step.path, step.target_value)) return false;
    }
    contention_backoff_ticks_ = 1U << (attempt - 1U);
    obs_ticks_ = 0;
    sum_after_io_psi_ = sum_after_mem_psi_ = sum_after_cpu_psi_ = sum_after_thermal_ = 0.0;
    sum_after_temp_vel_ = 0.0;
    valid_thermal_samples_in_window_ = 0;
    valid_io_samples_in_window_ = valid_mem_samples_in_window_ = 0;
    valid_cpu_samples_in_window_ = valid_pressure_samples_in_window_ = 0;
    min_after_io_psi_ = min_after_mem_psi_ = min_after_cpu_psi_ = min_after_thermal_ = 1e9;
    max_after_io_psi_ = max_after_mem_psi_ = max_after_cpu_psi_ = max_after_thermal_ = -1e9;
    if (const auto* stats = find_candidate(current_candidate_id_)) {
        settling_ticks_remaining_ = stats->settling_ticks;
    }
    last_verdict_ = "PENDING";
    last_verdict_enum_ = TunerVerdict::Pending;
    last_utility_delta_ = 0.0;
    current_rationale_ = "OEM contention detected; exact captured OEM baseline reasserted " +
        std::to_string(attempt) + "/3; observation restarted after bounded tick backoff.";
    refresh_surface_readings();
    return true;
}

bool AutonomousTuner::rollback_active(std::string_view reason) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    contention_backoff_ticks_ = 0;

    // Frequency mutations have their own CpuPolicy lease and BaselineStore
    // transaction. The tuner coordinates lifecycle, but never duplicates the
    // actuator's rollback or releases its lease independently.
    if (frequency_actuator_ &&
        (frequency_mutation_active_ || frequency_actuator_->mutation_active() ||
         frequency_actuator_->recovery_required())) {
        if (!frequency_actuator_->rollback()) {
            frequency_mutation_active_ = true;
            recovery_required_ = true;
            last_verdict_ = "RECOVERY_REQUIRED";
            last_verdict_enum_ = TunerVerdict::RecoveryRequired;
            current_rationale_ = std::string(reason) +
                " [frequency actuator rollback unresolved; CpuPolicy lease retained]";
            return false;
        }
        frequency_mutation_active_ = false;
    }

    auto& store = BaselineStore::instance();
    std::vector<MutationStep> failed_steps;
    // Unwind in reverse dependency order: Coupled-with dependents first, then Anchor!
    for (auto it = applied_steps_.rbegin(); it != applied_steps_.rend(); ++it) {
        if (!store.rollback(it->path)) {
            failed_steps.push_back(*it);
        }
    }
    observing_ = false;
    obs_ticks_ = 0;
    settling_ticks_remaining_ = 0;
    if (!failed_steps.empty()) {
        std::reverse(failed_steps.begin(), failed_steps.end());
        applied_steps_ = std::move(failed_steps);
        transaction_active_ = false;
        if (!recovery_required_) {
            if (auto* stats = find_candidate(current_candidate_id_)) ++stats->recovery_required_count;
        }
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) + " [ROLLBACK FAILED: RECOVERY_REQUIRED]";
        refresh_surface_readings();
        return false;
    }

    // Release only the candidate's exact domain/transaction lease after every
    // tracked path has passed BaselineStore rollback and readback verification.
    // AutonomousTuner owns one candidate transaction at a time; never bulk-release
    // any unexpected lease that may belong to a separate recovery path.
    if (!current_candidate_id_.empty() && current_candidate_id_ != "baseline") {
        coordinator_.release_lease(
            domain_for_candidate(current_candidate_id_), controller_name_, current_candidate_id_);
    }
    applied_steps_.clear();
    transaction_active_ = false;
    recovery_required_ = false;
    thermal_stabilization_active_ = false;
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
    const bool had_active_transaction = transaction_active_;
    current_objective_ = OptimizationObjective::EmergencyMitigation;
    if (had_active_transaction) {
        last_verdict_ = "ABORTED";
        last_verdict_enum_ = TunerVerdict::Aborted;
    }

    // This public entry point must provide the same admission guarantee as the
    // daemon's coordinated recovery path. Closing admission first prevents a
    // new controller from acquiring a domain between owner rollback and the
    // global recovery decision.
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
    const TunerTelemetryInput& telemetry) {
    if (steps.empty() || recovery_required_) return false;

    EvaluationDomain evaluation_domain = EvaluationDomain::Unavailable;
    if (intent == "IoLatencyRelief") evaluation_domain = EvaluationDomain::Io;
    else if (intent == "MemoryStallRelief") evaluation_domain = EvaluationDomain::Memory;
    else if (intent == "InteractiveCpuPacing") evaluation_domain = EvaluationDomain::Cpu;
    else if (intent == "ThermalStabilization") evaluation_domain = EvaluationDomain::Thermal;
    const bool target_baseline_valid =
        (evaluation_domain == EvaluationDomain::Io && telemetry.io_psi_valid && std::isfinite(telemetry.io_psi) && telemetry.io_psi >= 0.0) ||
        (evaluation_domain == EvaluationDomain::Memory && telemetry.memory_psi_valid && std::isfinite(telemetry.memory_psi) && telemetry.memory_psi >= 0.0) ||
        (evaluation_domain == EvaluationDomain::Cpu && telemetry.cpu_psi_valid && std::isfinite(telemetry.cpu_psi) && telemetry.cpu_psi >= 0.0) ||
        (evaluation_domain == EvaluationDomain::Thermal && telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure) && telemetry.thermal_pressure >= 0.0);
    if (!target_baseline_valid) return false;

    // First unwind any previous transaction cleanly
    if (!applied_steps_.empty()) {
        if (!rollback_active("Switching coordinated candidate")) {
            return false;
        }
    }

    auto& store = BaselineStore::instance();
    if (store.identity_unavailable()) return false;

    // Central Mutation Coordinator domain ownership & conflict check
    const MutationDomain dom = domain_for_candidate(candidate_id);
    const ControllerPriority prio = priority_for_candidate(candidate_id);
    std::string conflict_reason;
    if (!coordinator_.acquire_lease(
            dom, controller_name_, candidate_id, prio, &conflict_reason)) {
        last_verdict_ = "REJECTED";
        last_verdict_enum_ = TunerVerdict::Rejected;
        ++total_rejected_;
        if (auto* stats = find_candidate(candidate_id)) {
            ++stats->rejected_count;
            stats->last_rollback_reason = conflict_reason;
        }
        current_rationale_ = "Candidate " + std::string(candidate_id) + " rejected: " + conflict_reason;
        save_persisted_stats();
        return false;
    }

    // Publish the exact lease identity before preflight. rollback_active() and
    // the preemption callback must be able to resolve failures that occur before
    // the transaction reaches OBSERVING.
    current_candidate_id_ = std::string(candidate_id);
    contention_backoff_ticks_ = 0;
    auto& coordinator = coordinator_;

    const auto reject_preflight = [&](std::string_view reason) {
        coordinator.release_lease(dom, controller_name_, candidate_id);
        current_candidate_id_ = "baseline";
        current_candidate_label_ = "OEM Verified Baseline";
        current_intent_ = "HoldEquilibrium";
        last_verdict_ = "REJECTED";
        last_verdict_enum_ = TunerVerdict::Rejected;
        ++total_rejected_;
        if (auto* stats = find_candidate(candidate_id)) {
            ++stats->rejected_count;
            stats->last_rollback_reason = std::string(reason);
        }
        current_rationale_ = "Candidate " + std::string(candidate_id) +
            " rejected during preflight: " + std::string(reason);
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
        bool accessible = (::access(step.path.c_str(), R_OK | W_OK) == 0) &&
                          store.capture(step.path);
        if (accessible) {
            const auto audit = store.get_audit_trail();
            const auto snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
                return node.path == step.path;
            });
            // apply_transaction has already resolved this tuner's previous trial.
            // Any remaining journaled mutation belongs to another or unresolved owner.
            if (snapshot == audit.end() || snapshot->is_modified) accessible = false;
        }
        if (is_required && !accessible) {
            // No write has started. Classify this as REJECTED, never as an
            // empirical trial or an aborted mutation transaction.
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
    // Preflight is read-only. If a preemption arrived during discovery, do not
    // start the first mutation; leave this exact lease for callback acknowledgement.
    if (coordinator.is_preemption_requested(dom, controller_name_, candidate_id)) {
        return false;
    }

    // Phase 2: Ordered Execution (RequiredAnchor before RequiredDependent)
    std::vector<MutationStep> committed;
    committed.reserve(planned.size());
    std::string after_summary;

    for (const auto& step : planned) {
        // Never start a subsequent write after a priority/emergency request.
        // The registered callback rolls back the committed prefix at this safe point.
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
                // BaselineStore now performs path-scoped failure recovery. If the
                // optional write remains journaled as active, explicitly restore
                // that path before continuing with the independent candidate steps.
                if (unresolved_path_mutation && !store.rollback(step.path)) {
                    committed.push_back(step);
                    applied_steps_ = std::move(committed);
                    const std::string reason =
                        "Optional step failed and its path-scoped rollback is unresolved.";
                    record_apply_abort(reason);
                    (void)rollback_active(reason);
                    save_persisted_stats();
                    return false;
                }
                continue;
            }

            // Include a partially committed required path in reverse-order unwind.
            // rollback(path) is safe for a path already restored by BaselineStore.
            if (unresolved_path_mutation) committed.push_back(step);
            applied_steps_ = std::move(committed);
            const std::string reason =
                "Transaction aborted on failed required step; rollback attempted.";
            record_apply_abort(reason);
            (void)rollback_active(reason);
            // rollback_active can upgrade the verdict to RECOVERY_REQUIRED and
            // increment recovery_required_count when a path remains unresolved.
            save_persisted_stats();
            return false;
        }
        if (!after_summary.empty()) after_summary += ",";
        after_summary += fs::path(step.path).filename().string() + "=" + step.target_value;
        committed.push_back(step);
    }

    // Check after the final in-flight write too; there may be no following loop
    // iteration in which to observe a request that arrived during that write.
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

    applied_steps_ = std::move(committed);
    transaction_active_ = true;
    observing_ = true;
    obs_ticks_ = 0;
    thermal_stabilization_active_ = (intent == "ThermalStabilization");
    current_evaluation_domain_ = evaluation_domain;
    noise_floor_ = evaluation_noise_floors_[static_cast<size_t>(current_evaluation_domain_)];
    current_candidate_id_ = std::string(candidate_id);
    current_candidate_label_ = std::string(candidate_label);
    current_intent_ = std::string(intent);
    current_rationale_ = std::string(rationale);

    // The verdict and utility shown while OBSERVING belong to this new trial.
    // Never carry the previous candidate's verdict/delta onto a different candidate.
    last_verdict_ = "PENDING";
    last_verdict_enum_ = TunerVerdict::Pending;
    last_utility_delta_ = 0.0;

    before_io_valid_ = telemetry.io_psi_valid && std::isfinite(telemetry.io_psi) && telemetry.io_psi >= 0.0;
    before_mem_valid_ = telemetry.memory_psi_valid && std::isfinite(telemetry.memory_psi) && telemetry.memory_psi >= 0.0;
    before_cpu_valid_ = telemetry.cpu_psi_valid && std::isfinite(telemetry.cpu_psi) && telemetry.cpu_psi >= 0.0;
    before_thermal_valid_ = telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure) && telemetry.thermal_pressure >= 0.0;
    before_io_psi_ = before_io_valid_ ? telemetry.io_psi : 0.0;
    before_mem_psi_ = before_mem_valid_ ? telemetry.memory_psi : 0.0;
    before_cpu_psi_ = before_cpu_valid_ ? telemetry.cpu_psi : 0.0;
    before_thermal_ = before_thermal_valid_ ? telemetry.thermal_pressure : 0.0;
    before_temperature_celsius_ = telemetry.current_temperature_celsius;

    sum_after_io_psi_ = 0.0;
    sum_after_mem_psi_ = 0.0;
    sum_after_cpu_psi_ = 0.0;
    sum_after_thermal_ = 0.0;
    sum_after_temp_vel_ = 0.0;
    valid_thermal_samples_in_window_ = 0;
    valid_io_samples_in_window_ = valid_mem_samples_in_window_ = 0;
    valid_cpu_samples_in_window_ = valid_pressure_samples_in_window_ = 0;
    min_after_io_psi_ = min_after_mem_psi_ = min_after_cpu_psi_ = min_after_thermal_ = 1e9;
    max_after_io_psi_ = max_after_mem_psi_ = max_after_cpu_psi_ = max_after_thermal_ = -1e9;
    ++total_trials_;

    if (auto* stats = find_candidate(candidate_id)) {
        ++stats->trials;
        settling_ticks_remaining_ = stats->settling_ticks;
        stats->last_workload = current_workload_;
        stats->last_thermal_bucket = compute_thermal_bucket(telemetry);
        stats->last_before_summary = std::move(before_summary);
        stats->last_after_summary = std::move(after_summary);
    } else {
        settling_ticks_remaining_ = 1;
    }
    save_persisted_stats();
    refresh_surface_readings();
    return true;
}

void AutonomousTuner::evaluate_observation_window(const TunerTelemetryInput& latest_telemetry) {
    if (!observing_ || obs_ticks_ == 0) return;

    const auto mean = [](double sum, uint32_t count) noexcept { return count ? sum / static_cast<double>(count) : 0.0; };
    const double after_io = mean(sum_after_io_psi_, valid_io_samples_in_window_);
    const double after_mem = mean(sum_after_mem_psi_, valid_mem_samples_in_window_);
    const double after_cpu = mean(sum_after_cpu_psi_, valid_cpu_samples_in_window_);
    const double after_therm = mean(sum_after_thermal_, valid_pressure_samples_in_window_);

    // Verify both readback value and inode ownership across all applied nodes
    if (!verify_active_ownership()) {
        if (retry_contended_ownership()) {
            save_persisted_stats();
            return;
        }
        last_verdict_ = "OWNERSHIP_LOST";
        last_verdict_enum_ = TunerVerdict::OwnershipLost;
        ++total_rolled_back_;
        record_ownership_loss(find_candidate(current_candidate_id_));
        (void)rollback_active("External node ownership lost during observation window.");
        save_persisted_stats();
        return;
    }

    const uint32_t target_samples = current_evaluation_domain_ == EvaluationDomain::Io ? valid_io_samples_in_window_ :
        current_evaluation_domain_ == EvaluationDomain::Memory ? valid_mem_samples_in_window_ :
        current_evaluation_domain_ == EvaluationDomain::Cpu ? valid_cpu_samples_in_window_ : valid_pressure_samples_in_window_;
    if (target_samples < 2) {
        last_verdict_ = "INCONCLUSIVE"; last_verdict_enum_ = TunerVerdict::Inconclusive; last_utility_delta_ = 0.0;
        observing_ = false; settling_ticks_remaining_ = 0; ++total_rolled_back_; ++total_inconclusive_;
        if (auto* stats = find_candidate(current_candidate_id_)) { ++stats->inconclusive_count; ++stats->consecutive_non_improved; stats->last_rollback_reason = "Insufficient valid target-domain samples"; }
        (void)rollback_active("Target-domain evidence incomplete; reverted to OEM baseline.");
        update_ucb_scores(); save_persisted_stats(); return;
    }
    const bool io_valid = before_io_valid_ && valid_io_samples_in_window_ >= 2;
    const bool mem_valid = before_mem_valid_ && valid_mem_samples_in_window_ >= 2;
    const bool cpu_valid = before_cpu_valid_ && valid_cpu_samples_in_window_ >= 2;
    const bool thermal_valid = before_thermal_valid_ && valid_pressure_samples_in_window_ >= 2;
    const double io_gain = io_valid ? before_io_psi_ - after_io : 0.0;
    const double mem_gain = mem_valid ? before_mem_psi_ - after_mem : 0.0;
    const double cpu_gain = cpu_valid ? before_cpu_psi_ - after_cpu : 0.0;
    const double therm_gain = thermal_valid ? before_thermal_ - after_therm : 0.0;
    const double therm_penalty = thermal_valid ? std::max(0.0, after_therm - before_thermal_ - 2.0) : 0.0;

    double utility_delta = 0.0;
    if (current_intent_ == "ThermalStabilization") {
        // Requirements 6 & 7: Check if physical thermal evidence is available and adequate.
        // A candidate under ThermalStabilization can ONLY be retained if valid physical sensor telemetry is available.
        const bool has_thermal_evidence = valid_thermal_samples_in_window_ >= 2 && valid_pressure_samples_in_window_ >= 2;

        if (!has_thermal_evidence || (latest_telemetry.current_temperature_celsius < 0.0 && !latest_telemetry.temperature_velocity_valid)) {
            // Sensor unavailable or insufficient samples -> INCONCLUSIVE per Requirement 6 & 7
            last_verdict_ = "INCONCLUSIVE";
            last_verdict_enum_ = TunerVerdict::Inconclusive;
            last_utility_delta_ = 0.0;
            observing_ = false;
            settling_ticks_remaining_ = 0;
            ++total_rolled_back_;
            ++total_inconclusive_;
            if (auto* stats = find_candidate(current_candidate_id_)) {
                ++stats->inconclusive_count;
                ++stats->consecutive_non_improved;
                stats->last_rollback_reason = "Inconclusive: thermal sensor telemetry unavailable or insufficient samples";
                const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
                stats->cooldown_ticks = std::min<uint32_t>(32U, 4U * (1U << shift));
            }
            const std::string msg =
                "Trial " + current_candidate_id_ + " inconclusive due to missing thermal sensor evidence; reverted to OEM baseline.";
            (void)rollback_active(msg);
            save_persisted_stats();
            return;
        }

        const double avg_temp_vel = (valid_thermal_samples_in_window_ > 0)
            ? (sum_after_temp_vel_ / static_cast<double>(valid_thermal_samples_in_window_))
            : latest_telemetry.temperature_velocity_celsius_per_sec;

        // Requirements 4, 5, 8:
        // Measurement noise margin: |avg_temp_vel| <= 0.05 °C/s is within the thermistor sensor noise floor.
        // A zero or near-zero temperature trend is NOT evidence of cooling (zero bonus!).
        // Genuine cooling requires avg_temp_vel < -0.05 °C/s.
        // If temperature is rising (avg_temp_vel > +0.05 °C/s), it is warming and cannot be considered improved.

        if (avg_temp_vel > 0.05) {
            // Warming trend: device is getting hotter
            const double warming_penalty = std::min(1.5, avg_temp_vel * 1.5);
            utility_delta = 0.40 * therm_gain - warming_penalty + 0.20 * io_gain + 0.15 * cpu_gain;
            if (utility_delta >= 0.25) {
                utility_delta = 0.10; // Cap below IMPROVED threshold
            }
        } else if (avg_temp_vel < -0.05) {
            // Genuine physical cooling evidence beyond sensor noise
            const double cooling_gain = std::min(1.2, (-avg_temp_vel) * 1.5);
            utility_delta = 0.45 * therm_gain + cooling_gain + 0.20 * io_gain + 0.15 * cpu_gain;
        } else {
            // Flat / zero temperature trend (|avg_temp_vel| <= 0.05):
            // Zero is NOT evidence of cooling. Zero trend bonus!
            // Thermal pressure reduction must significantly exceed noise floor (>= 2.5 index points).
            const double effective_therm_gain = (therm_gain >= 2.5) ? (therm_gain * 0.40) : 0.0;
            utility_delta = effective_therm_gain + 0.20 * io_gain + 0.15 * cpu_gain;
        }

        // Requirement 9: Thermal pressure reduction must not mask workload or stability regression.
        if ((io_valid && io_gain < -1.5) || (cpu_valid && cpu_gain < -3.0) || (mem_valid && mem_gain < -1.5)) {
            if (utility_delta >= 0.25) {
                utility_delta = -0.25; // Force regressed verdict
            }
        }
    } else if (current_intent_ == "IoLatencyRelief") {
        utility_delta = 1.40 * io_gain + (cpu_valid ? 0.35 * cpu_gain : 0.0) - 0.60 * therm_penalty;
    } else if (current_intent_ == "MemoryStallRelief") {
        utility_delta = 1.60 * mem_gain + (io_valid ? 0.45 * io_gain : 0.0) - 0.60 * therm_penalty;
    } else if (current_intent_ == "InteractiveCpuPacing") {
        utility_delta = 1.50 * cpu_gain + (io_valid ? 0.35 * io_gain : 0.0) - 0.65 * therm_penalty;
    } else utility_delta = 0.0;

    last_utility_delta_ = utility_delta;
    observing_ = false;
    settling_ticks_remaining_ = 0;

    auto* stats = find_candidate(current_candidate_id_);

    double target_spread = 0.0, target_baseline = 0.0, target_after = 0.0;
    std::string_view target_metric = "unavailable";
    size_t noise_index = 0;
    switch (current_evaluation_domain_) {
        case EvaluationDomain::Io: target_spread=max_after_io_psi_-min_after_io_psi_; target_baseline=before_io_psi_; target_after=after_io; target_metric="io_psi"; noise_index=0; break;
        case EvaluationDomain::Memory: target_spread=max_after_mem_psi_-min_after_mem_psi_; target_baseline=before_mem_psi_; target_after=after_mem; target_metric="memory_psi"; noise_index=1; break;
        case EvaluationDomain::Cpu: target_spread=max_after_cpu_psi_-min_after_cpu_psi_; target_baseline=before_cpu_psi_; target_after=after_cpu; target_metric="cpu_psi"; noise_index=2; break;
        case EvaluationDomain::Thermal: target_spread=max_after_thermal_-min_after_thermal_; target_baseline=before_thermal_; target_after=after_therm; target_metric="thermal_pressure"; noise_index=3; break;
        default: break;
    }
    const double domain_weight = current_evaluation_domain_ == EvaluationDomain::Io ? 0.07 : current_evaluation_domain_ == EvaluationDomain::Memory ? 0.08 : current_evaluation_domain_ == EvaluationDomain::Cpu ? 0.075 : 0.0225;
    const double measured_noise = std::clamp(target_spread * domain_weight, 0.02, 0.45);
    evaluation_noise_floors_[noise_index] = std::clamp(0.85 * evaluation_noise_floors_[noise_index] + 0.15 * measured_noise, 0.02, 0.45);
    noise_floor_ = evaluation_noise_floors_[noise_index];
    const double improved_threshold = std::max(0.25, 2.0 * noise_floor_);
    const double regressed_threshold = -(0.20 + noise_floor_);
    const bool high_variance_inconclusive = target_spread > std::max(6.0, std::abs(target_baseline) * 1.2) && std::abs(utility_delta) < 0.50;

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
            "Trial " + current_candidate_id_ + " inconclusive due to high sample variance; reverted to OEM baseline.";
        (void)rollback_active(msg);
        save_persisted_stats();
        return;
    }

    // Only an ownership-verified, complete, low-variance window is eligible reward.
    ++total_reward_samples_;
    if (stats != nullptr) {
        const double previous = static_cast<double>(stats->reward_samples);
        ++stats->reward_samples;
        stats->mean_utility_delta = (stats->mean_utility_delta * previous + utility_delta) /
                                    static_cast<double>(stats->reward_samples);
    }

    if (utility_delta >= improved_threshold) {
        // IMPROVED: Measurable benefit verified above noise floor -> Retain configuration!
        last_verdict_ = "IMPROVED";
        last_verdict_enum_ = TunerVerdict::Improved;
        ++total_improved_;
        if (stats != nullptr) {
            ++stats->improved_count;
            stats->successes = stats->improved_count;
            stats->consecutive_non_improved = 0;
            stats->adaptive_scale = std::min(1.0, stats->adaptive_scale + 0.15);
            stats->mean_utility = std::clamp(
                stats->mean_utility + 0.30 * (std::min(2.5, utility_delta) - stats->mean_utility),
                0.05,
                2.50);
        }
        current_rationale_ += " [Verified IMPROVED: ΔU=+" +
                              std::to_string(utility_delta).substr(0, 4) +
                              ", " + std::string(target_metric) + " " + std::to_string(target_baseline).substr(0, 5) +
                              " -> " + std::to_string(target_after).substr(0, 5) + "% -> RETAINED]";
        update_ucb_scores();
    } else if (utility_delta < regressed_threshold) {
        // REGRESSED: Stalls or thermals worsened -> Rollback, step down adaptive scale & apply context-aware cooldown!
        last_verdict_ = "REGRESSED";
        last_verdict_enum_ = TunerVerdict::Regressed;
        ++total_rolled_back_;
        if (stats != nullptr) {
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
            "Trial " + current_candidate_id_ + " regressed (ΔU=" +
            std::to_string(utility_delta).substr(0, 5) +
            ", " + std::string(target_metric) + " " + std::to_string(target_baseline).substr(0, 5) +
            " -> " + std::to_string(target_after).substr(0, 5) +
            "%); rolled back to OEM baseline.";
        (void)rollback_active(msg);
    } else {
        // NEUTRAL (within noise floor [-0.20, +0.25)): Revert to OEM baseline per Invariant 8
        last_verdict_ = "NEUTRAL";
        last_verdict_enum_ = TunerVerdict::Neutral;
        ++total_rolled_back_;
        if (stats != nullptr) {
            ++stats->neutral_count;
            ++stats->consecutive_non_improved;
            stats->adaptive_scale = std::max(0.50, stats->adaptive_scale * 0.90);
            stats->mean_utility = std::clamp(stats->mean_utility + 0.30 * (0.50 - stats->mean_utility), 0.05, 2.50);
            stats->last_rollback_reason = "Within noise floor; reverted per Invariant 8";
            const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
            stats->cooldown_ticks = std::min<uint32_t>(32U, 4U * (1U << shift));
        }
        const std::string msg =
            "Trial " + current_candidate_id_ + " neutral (ΔU=" +
            std::to_string(utility_delta).substr(0, 4) +
            "); reverted to OEM baseline per Invariant 8 (no unverified retention).";
        (void)rollback_active(msg);
    }
    update_ucb_scores();
    save_persisted_stats();
}

AutonomousDecisionSnapshot AutonomousTuner::step(const TunerTelemetryInput& raw_telemetry) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    TunerTelemetryInput telemetry = raw_telemetry;
    latest_current_temperature_ = telemetry.current_temperature_celsius;
    latest_temperature_velocity_ = telemetry.temperature_velocity_celsius_per_sec;
    latest_temperature_velocity_valid_ = telemetry.temperature_velocity_valid;
    latest_pressure_velocity_ = telemetry.thermal_pressure_velocity;

    // Startup observation is a hard admission gate: collect telemetry and preserve
    // state, but do not plan or execute candidate mutations before auto-promotion.
    if (!operational_) {
        return snapshot();
    }

    // Backward compatibility: if caller set thermal_trend_celsius without setting temperature_velocity_valid,
    // adopt it only if non-zero, but keep them distinct.
    if (!telemetry.temperature_velocity_valid && std::abs(telemetry.thermal_trend_celsius) > 1e-6) {
        telemetry.temperature_velocity_celsius_per_sec = telemetry.thermal_trend_celsius;
        telemetry.temperature_velocity_valid = true;
        telemetry.temperature_samples_count = std::max(telemetry.temperature_samples_count, 2U);
        latest_temperature_velocity_ = telemetry.temperature_velocity_celsius_per_sec;
        latest_temperature_velocity_valid_ = true;
    }

    // Pressure deltas are meaningful only across valid thermal-pressure samples.
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

    // Emergency arbitration must run before recovery retries: a failed rollback can
    // return early, but it must never suppress the emergency preemption request.
    const bool emergency_boundary =
        telemetry.thermal_emergency || (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 88.0);
    if (emergency_boundary) {
        current_objective_ = OptimizationObjective::EmergencyMitigation;
        coordinator_.emergency_preempt_all("ThermalEmergency");
    }

    if (recovery_required_) {
        // Attempt recovery again; do not allow new mutations while recovery_required_ is true
        if (!rollback_active("Retrying pending rollback recovery")) {
            return snapshot();
        }
    }

    // Decrement candidate cooldowns
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
    }
    if (cooldown_changed) {
        save_persisted_stats();
    }

    // Update workload classification, bottleneck diagnosis, and optimization objective
    current_workload_ = classify_workload(telemetry);
    if (foreground_task_resolver_) {
        const bool foreground_render_workload =
            current_workload_ == WorkloadClass::Gaming ||
            current_workload_ == WorkloadClass::ScrollingAndAnimation ||
            current_workload_ == WorkloadClass::GeneralInteraction;
        const bool thermal_admitted = !telemetry.thermal_guard_active &&
            !telemetry.thermal_emergency && telemetry.thermal_pressure_valid &&
            std::isfinite(telemetry.thermal_pressure) && telemetry.thermal_pressure < 78.0;
        if (foreground_render_workload && thermal_admitted) {
            (void)foreground_task_resolver_->refresh();
        } else {
            (void)foreground_task_resolver_->restore_all();
        }
    }
    current_bottleneck_ = diagnose_bottleneck(telemetry, current_workload_);
    const uint8_t thermal_bucket = compute_thermal_bucket(telemetry);

    // Emergency Safety Gate: Catastrophic thermal emergency (>= 88.0 pressure or emergency flag)
    // immediately rolls back all mutations and preempts domain leases.
    if (telemetry.thermal_emergency || (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 88.0)) {
        current_objective_ = OptimizationObjective::EmergencyMitigation;
        if (transaction_active_ || frequency_mutation_active_ ||
            (frequency_actuator_ && frequency_actuator_->mutation_active())) {
            last_verdict_ = "ABORTED";
            last_verdict_enum_ = TunerVerdict::Aborted;
            if (auto* stats = find_candidate(current_candidate_id_)) ++stats->aborted_count;
            ++total_rolled_back_;
            (void)rollback_active("Thermal emergency gate active; restored OEM baseline.");
            save_persisted_stats();
        }
        current_intent_ = "ThermalHoldAndProtect";
        current_rationale_ = "Emergency thermal boundary reached; all domains locked to OEM baseline.";
        return snapshot();
    }

    // Active Thermal Stabilization Mode:
    // When thermal guard is active, headroom is low (<= 0.20), or thermal pressure is elevated (>= 65.0),
    // switch optimization objective to ThermalStabilization instead of blindly halting!
    const bool needs_thermal_stabilization =
        (current_workload_ == WorkloadClass::ThermalConstrained ||
         current_bottleneck_ == BottleneckKind::ThermalHeadroomDepletion);

    if (needs_thermal_stabilization) {
        current_objective_ = OptimizationObjective::ThermalStabilization;
        thermal_recovery_ticks_ = 2; // Arm hysteresis recovery window for when thermal load subsides

        // If a non-thermal performance transaction is currently active, unwind it first
        if ((transaction_active_ || frequency_mutation_active_ ||
             (frequency_actuator_ && frequency_actuator_->mutation_active())) &&
            current_intent_ != "ThermalStabilization") {
            last_verdict_ = "ABORTED";
            last_verdict_enum_ = TunerVerdict::Aborted;
            if (auto* stats = find_candidate(current_candidate_id_)) ++stats->aborted_count;
            ++total_rolled_back_;
            (void)rollback_active("Preempting performance mutation for active ThermalStabilization.");
            save_persisted_stats();
        }

        // If we are already observing or retaining a ThermalStabilization transaction, continue below;
        // otherwise select the best eligible ThermalStabilization candidate.
        if (!transaction_active_) {
            current_intent_ = "ThermalStabilization";
            current_rationale_ = "Thermal headroom constrained; switching objective to ThermalStabilization.";

            if (!cpufreq_governor_dirs_.empty() &&
                is_candidate_eligible_for_context("thermal.schedutil_damped_pacing", current_workload_, thermal_bucket)) {
                const auto* stats = find_candidate("thermal.schedutil_damped_pacing");
                const double scale = stats ? stats->adaptive_scale : 1.0;
                std::vector<MutationStep> steps;
                for (const auto& gdir : cpufreq_governor_dirs_) {
                    const fs::path gpath(gdir);
                    const std::string gov_name = gpath.filename().string();
                    const fs::path scaling_gov_path = gpath.parent_path() / "scaling_governor";
                    const std::string active_gov = DaemonConfig::trim(read_first_line(scaling_gov_path.string()));
                    if (!active_gov.empty() && active_gov != gov_name) continue;

                    const std::string up_path = gdir + "/up_rate_limit_us";
                    const std::string down_path = gdir + "/down_rate_limit_us";
                    const uint64_t damped_up = compute_relative_u64(up_path, 2000, 2.00, 2000, 10000, scale);
                    const uint64_t damped_down = compute_relative_u64(down_path, 1000, 0.50, 500, 4000, scale);
                    steps.push_back({up_path, std::to_string(damped_up), "Anchor (Required-before)", StepRelation::RequiredAnchor});
                    steps.push_back({down_path, std::to_string(damped_down), "Coupled-with", StepRelation::RequiredDependent});
                }
                if (!steps.empty()) {
                    (void)apply_transaction(
                        "thermal.schedutil_damped_pacing",
                        "Thermal Stabilization Governor Damping (2.0x up / 0.5x down rate-limit)",
                        "ThermalStabilization",
                        "Active thermal stabilization: damping EAS governor frequency ramp rate to reduce transient thermal spikes.",
                        steps,
                        telemetry);
                    return snapshot();
                }
            }

            if (telemetry.io_psi_valid && telemetry.io_psi >= 1.5 && !block_queues_.empty() &&
                is_candidate_eligible_for_context("thermal.io_burst_pacing", current_workload_, thermal_bucket)) {
                const auto* stats = find_candidate("thermal.io_burst_pacing");
                const double scale = stats ? stats->adaptive_scale : 1.0;
                std::vector<MutationStep> steps;
                for (const auto& q : block_queues_) {
                    const std::string ra_path = q.queue_dir + "/read_ahead_kb";
                    const std::string nr_path = q.queue_dir + "/nr_requests";
                    const uint64_t damped_ra = compute_relative_u64(ra_path, q.baseline_read_ahead_kb, 0.50, 64, 512, scale);
                    const uint64_t damped_nr = compute_relative_u64(nr_path, q.baseline_nr_requests, 0.75, 32, 256, scale);
                    steps.push_back({ra_path, std::to_string(damped_ra), "Anchor (Required-before)", StepRelation::RequiredAnchor});
                    steps.push_back({nr_path, std::to_string(damped_nr), "Coupled-with", StepRelation::RequiredDependent});
                }
                if (!steps.empty()) {
                    (void)apply_transaction(
                        "thermal.io_burst_pacing",
                        "Thermal Stabilization Block Queue Burst Pacing (0.5x read-ahead / 0.75x depth)",
                        "ThermalStabilization",
                        "Active thermal stabilization: moderating block queue burst depth under concurrent thermal and I/O load.",
                        steps,
                        telemetry);
                    return snapshot();
                }
            }

            return snapshot();
        }
    } else if (thermal_stabilization_active_ && telemetry.thermal_pressure_valid && !telemetry.thermal_guard_active) {
        // Exit thermal stabilization only with valid pressure evidence that the guard subsided.
        (void)rollback_active("Thermal pressure subsided below hysteresis threshold; entering ThermalRecovery.");
        thermal_recovery_ticks_ = 2;
    }

    // Workload optimization currently evaluates a three-domain before/after utility vector.
    // If any PSI source is unavailable, do not treat its placeholder as zero or mutate based on
    // incomplete evidence. Thermal stabilization above remains independently actionable.
    const bool psi_evidence_complete =
        telemetry.cpu_psi_valid && telemetry.memory_psi_valid && telemetry.io_psi_valid;
    const bool active_domain_evidence_complete = frequency_mutation_active_
        ? (telemetry.cpu_psi_valid && telemetry.memory_psi_valid &&
           telemetry.io_psi_valid && telemetry.thermal_pressure_valid)
        : ((current_evaluation_domain_ == EvaluationDomain::Io && telemetry.io_psi_valid) ||
           (current_evaluation_domain_ == EvaluationDomain::Memory && telemetry.memory_psi_valid) ||
           (current_evaluation_domain_ == EvaluationDomain::Cpu && telemetry.cpu_psi_valid) ||
           (current_evaluation_domain_ == EvaluationDomain::Thermal && telemetry.thermal_pressure_valid));
    const bool required_evidence_complete =
        (observing_ || transaction_active_ || frequency_mutation_active_)
            ? active_domain_evidence_complete : psi_evidence_complete;
    if (!required_evidence_complete) {
        if (observing_ || transaction_active_ || frequency_mutation_active_) {
            last_verdict_ = "INCONCLUSIVE";
            last_verdict_enum_ = TunerVerdict::Inconclusive;
            ++total_inconclusive_;
            ++total_rolled_back_;
            if (auto* stats = find_candidate(current_candidate_id_)) {
                ++stats->inconclusive_count;
                ++stats->consecutive_non_improved;
                stats->last_rollback_reason = "Incomplete PSI evidence during observation/retention";
            }
            const bool restored = rollback_active(
                "Incomplete PSI evidence; releasing candidate and restoring OEM baseline.");
            if (!restored) {
                last_verdict_ = "RECOVERY_REQUIRED";
                last_verdict_enum_ = TunerVerdict::RecoveryRequired;
            }
            save_persisted_stats();
        } else {
            current_objective_ = OptimizationObjective::HoldBaseline;
            current_intent_ = "HoldEquilibrium";
            current_rationale_ = "PSI evidence incomplete; workload mutation withheld while thermal policy remains active.";
            last_verdict_ = "INCONCLUSIVE";
            last_verdict_enum_ = TunerVerdict::Inconclusive;
        }
        return snapshot();
    }

    const bool in_thermal_recovery = (thermal_recovery_ticks_ > 0);
    if (thermal_recovery_ticks_ > 0) {
        const bool temp_stable_or_falling =
            (!telemetry.temperature_velocity_valid) ||
            (telemetry.temperature_velocity_celsius_per_sec <= 0.05);
        if (telemetry.thermal_pressure_valid && telemetry.thermal_pressure <= 46.0 && temp_stable_or_falling) {
            thermal_recovery_ticks_ = 0;
        } else {
            --thermal_recovery_ticks_;
        }
    }
    current_objective_ = select_objective(telemetry, current_workload_, current_bottleneck_, in_thermal_recovery);

    // Backoff is timer-tick based; emergency, thermal, and evidence gates have already run.
    if (contention_backoff_ticks_ > 0) {
        --contention_backoff_ticks_;
        return snapshot();
    }

    // Stage 1: If currently observing an applied transaction, collect After-Window samples
    if (observing_) {
        if (!verify_active_ownership()) {
            if (retry_contended_ownership()) {
                save_persisted_stats();
                return snapshot();
            }
            last_verdict_ = "OWNERSHIP_LOST";
            last_verdict_enum_ = TunerVerdict::OwnershipLost;
            ++total_rolled_back_;
            record_ownership_loss(find_candidate(current_candidate_id_));
            (void)rollback_active("External node ownership lost during observation window.");
            save_persisted_stats();
            return snapshot();
        }
        if (settling_ticks_remaining_ > 0) {
            --settling_ticks_remaining_;
        }
        if (telemetry.io_psi_valid && std::isfinite(telemetry.io_psi) && telemetry.io_psi >= 0.0) {
            sum_after_io_psi_ += telemetry.io_psi; min_after_io_psi_ = std::min(min_after_io_psi_, telemetry.io_psi);
            max_after_io_psi_ = std::max(max_after_io_psi_, telemetry.io_psi); ++valid_io_samples_in_window_;
        }
        if (telemetry.memory_psi_valid && std::isfinite(telemetry.memory_psi) && telemetry.memory_psi >= 0.0) {
            sum_after_mem_psi_ += telemetry.memory_psi; min_after_mem_psi_ = std::min(min_after_mem_psi_, telemetry.memory_psi);
            max_after_mem_psi_ = std::max(max_after_mem_psi_, telemetry.memory_psi); ++valid_mem_samples_in_window_;
        }
        if (telemetry.cpu_psi_valid && std::isfinite(telemetry.cpu_psi) && telemetry.cpu_psi >= 0.0) {
            sum_after_cpu_psi_ += telemetry.cpu_psi; min_after_cpu_psi_ = std::min(min_after_cpu_psi_, telemetry.cpu_psi);
            max_after_cpu_psi_ = std::max(max_after_cpu_psi_, telemetry.cpu_psi); ++valid_cpu_samples_in_window_;
        }
        if (telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure) && telemetry.thermal_pressure >= 0.0) {
            sum_after_thermal_ += telemetry.thermal_pressure; min_after_thermal_ = std::min(min_after_thermal_, telemetry.thermal_pressure);
            max_after_thermal_ = std::max(max_after_thermal_, telemetry.thermal_pressure); ++valid_pressure_samples_in_window_;
        }
        if (telemetry.temperature_velocity_valid && std::isfinite(telemetry.temperature_velocity_celsius_per_sec) && telemetry.temperature_samples_count >= 2) {
            sum_after_temp_vel_ += telemetry.temperature_velocity_celsius_per_sec; ++valid_thermal_samples_in_window_;
        }
        ++obs_ticks_;
        if (obs_ticks_ >= obs_window_) {
            evaluate_observation_window(telemetry);
        }
        refresh_surface_readings();
        return snapshot();
    }

    const bool io_bottleneck = telemetry.io_psi_valid && telemetry.io_psi >= 2.2;
    const bool mem_bottleneck = telemetry.memory_psi_valid && telemetry.memory_psi >= 2.0;
    // Once the tiered runtime identifies a CPU-bound workload, it owns the
    // decision for CPU frequency behavior. Do not fall back to legacy schedutil
    // pacing when Tier 1/Tier 2 or LMh admission is denied; I/O and memory
    // optimizations remain independently eligible.
    const bool tiered_cpu_policy_owned =
        frequency_actuator_ != nullptr && tiered_policy_router_ != nullptr &&
        telemetry.frequency_policy_evidence_valid &&
        telemetry.frequency_policy_evidence.classification.workload == DetectedWorkloadClass::CpuBound;
    // Gate aggressive CPU interactive boost during post-thermal hysteresis recovery.
    const bool cpu_bottleneck =
        !tiered_cpu_policy_owned && !in_thermal_recovery &&
        telemetry.cpu_psi_valid && telemetry.cpu_psi >= 8.0 &&
        telemetry.thermal_pressure_valid && telemetry.thermal_pressure < 72.0 &&
        !cpufreq_governor_dirs_.empty();

    // Stage 2: If a transaction is currently RETAINED, verify ownership and cross-domain guards
    if (transaction_active_) {
        if (!verify_active_ownership()) {
            if (retry_contended_ownership()) {
                save_persisted_stats();
                return snapshot();
            }
            last_verdict_ = "OWNERSHIP_LOST";
            last_verdict_enum_ = TunerVerdict::OwnershipLost;
            ++total_rolled_back_;
            record_ownership_loss(find_candidate(current_candidate_id_));
            (void)rollback_active("External node ownership lost while transaction was retained.");
            save_persisted_stats();
            return snapshot();
        }
        if (!io_bottleneck && !mem_bottleneck && telemetry.cpu_psi_valid && telemetry.cpu_psi < 4.0 && !needs_thermal_stabilization) {
            if (++idle_streak_ >= 6) {
                idle_streak_ = 0;
                last_verdict_ = "HOLD";
                last_verdict_enum_ = TunerVerdict::Hold;
                (void)rollback_active("Workload returned to sustained idle; restored OEM baseline.");
            }
        } else {
            idle_streak_ = 0;
            // Retained-state checks follow the same evaluation domain as trial scoring.
            bool domain_regressed = false;
            switch (current_evaluation_domain_) {
                case EvaluationDomain::Io:
                    domain_regressed = telemetry.io_psi_valid && before_io_valid_ && telemetry.io_psi > before_io_psi_ + 4.0;
                    break;
                case EvaluationDomain::Memory:
                    domain_regressed = telemetry.memory_psi_valid && before_mem_valid_ && telemetry.memory_psi > before_mem_psi_ + 3.5;
                    break;
                case EvaluationDomain::Cpu:
                    domain_regressed = telemetry.cpu_psi_valid && before_cpu_valid_ && telemetry.cpu_psi > before_cpu_psi_ + 6.0;
                    if (!domain_regressed && telemetry.thermal_pressure_valid && before_thermal_valid_)
                        domain_regressed = telemetry.thermal_pressure > before_thermal_ + 8.0;
                    break;
                case EvaluationDomain::Thermal:
                    domain_regressed = telemetry.thermal_pressure_valid && before_thermal_valid_ && telemetry.thermal_pressure > before_thermal_ + 8.0;
                    if (!domain_regressed && telemetry.temperature_velocity_valid)
                        domain_regressed = telemetry.temperature_velocity_celsius_per_sec > 0.05;
                    break;
                default: break;
            }
            if (domain_regressed) {
                if (auto* stats = find_candidate(current_candidate_id_)) {
                    stats->cooldown_ticks = 6;
                    stats->last_rollback_reason = "Retained metric guard exceeded on workload shift";
                    save_persisted_stats();
                }
                (void)rollback_active("Workload regime shifted; releasing retained candidate to re-plan.");
            }
        }
        refresh_surface_readings();
        return snapshot();
    }

    // Tiered CPU frequency policy is evaluated only after generic transactions
    // have completed. The actuator owns its own CpuPolicy lease and rollback journal.
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
        PolicyEvidence evidence = telemetry.frequency_policy_evidence;
        evidence.evaluated_at_ns = policy_now_ns;
        evidence.mutation_runtime_ready = true;
        evidence.cpu_frequency_capability_valid =
            frequency_mutation_active_ ||
            frequency_actuator_->can_apply_policy(*policy_index, 1.02, policy_now_ns);
        frequency_decision = tiered_policy_router_->evaluate(platform_context_, evidence);
        frequency_policy_evidence_ready = true;
    }
    frequency_policy_decision_ = frequency_decision;

    const bool frequency_step_requested =
        frequency_policy_evidence_ready &&
        frequency_decision.action == PolicyAction::CpuFrequencyStep &&
        frequency_decision.requires_mutation_admission &&
        (frequency_decision.tier == OptimizationTier::Tier1InteractiveResponse ||
         frequency_decision.tier == OptimizationTier::Tier2SustainedOptimization);

    if (frequency_mutation_active_ || (frequency_actuator_ && frequency_actuator_->mutation_active())) {
        if (!frequency_step_requested || !policy_index.has_value() ||
            *policy_index != frequency_policy_index_) {
            if (!rollback_active("Tiered CPU-bound evidence or independent admission no longer qualifies.")) {
                return snapshot();
            }
            last_verdict_ = "HOLD";
            last_verdict_enum_ = TunerVerdict::Hold;
            current_intent_ = "HoldEquilibrium";
            current_rationale_ = "CPU frequency step released because Tier 1/Tier 2 evidence or admission expired.";
            return snapshot();
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
            last_verdict_ = "PENDING";
            last_verdict_enum_ = TunerVerdict::Pending;
            current_intent_ = "TieredCpuFrequency";
            current_rationale_ = std::string(frequency_decision.rationale);
        } else if (outcome.verdict == OutcomeVerdict::Improved && frequency_mutation_active_) {
            if (last_verdict_ != "IMPROVED") {
                ++total_reward_samples_;
                ++total_improved_;
            }
            last_verdict_ = "IMPROVED";
            last_verdict_enum_ = TunerVerdict::Improved;
            current_intent_ = "TieredCpuFrequency";
            current_rationale_ = std::string(outcome.reason);
        } else {
            if (outcome.verdict == OutcomeVerdict::Regressed) {
                last_verdict_ = "REGRESSED";
                last_verdict_enum_ = TunerVerdict::Regressed;
            } else if (outcome.verdict == OutcomeVerdict::Neutral) {
                last_verdict_ = "NEUTRAL";
                last_verdict_enum_ = TunerVerdict::Neutral;
            } else {
                last_verdict_ = "INCONCLUSIVE";
                last_verdict_enum_ = TunerVerdict::Inconclusive;
                ++total_inconclusive_;
            }
            if (!frequency_mutation_active_) ++total_rolled_back_;
            current_intent_ = "HoldEquilibrium";
            current_rationale_ = outcome.reason;
            if (frequency_actuator_->recovery_required()) {
                recovery_required_ = true;
                last_verdict_ = "RECOVERY_REQUIRED";
                last_verdict_enum_ = TunerVerdict::RecoveryRequired;
            }
        }
        refresh_surface_readings();
        return snapshot();
    }

    if (frequency_step_requested && !transaction_active_ && !observing_ &&
        frequency_actuator_ != nullptr && policy_index.has_value()) {
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
        if (frequency_actuator_->apply_policy(
                *policy_index, frequency_decision.requested_capacity_scale,
                baseline_sample, policy_now_ns)) {
            frequency_mutation_active_ = true;
            frequency_policy_index_ = *policy_index;
            current_intent_ = "TieredCpuFrequency";
            current_rationale_ = std::string(frequency_decision.rationale);
            last_verdict_ = "PENDING";
            last_verdict_enum_ = TunerVerdict::Pending;
        } else {
            ++total_rejected_;
            last_verdict_ = "REJECTED";
            last_verdict_enum_ = TunerVerdict::Rejected;
            current_intent_ = "HoldEquilibrium";
            current_rationale_ = "Tiered CPU frequency request denied by live thermal, LMh, OPP, or ownership admission.";
        }
        refresh_surface_readings();
        return snapshot();
    }

    // Stage 3: No active transaction — diagnose if the device genuinely needs a mutation
    if (!io_bottleneck && !mem_bottleneck && !cpu_bottleneck) {
        if (in_thermal_recovery) {
            current_intent_ = "ThermalRecovery";
            current_rationale_ = "Hysteresis thermal recovery active; holding OEM baseline until thermal equilibrium stabilizes.";
        }
        refresh_surface_readings();
        return snapshot();
    }

    // Priority A: I/O Latency & Queue Bottleneck -> Select highest-UCB1 eligible I/O candidate
    if (io_bottleneck && !block_queues_.empty()) {
        const auto& primary_q = block_queues_.front();
        const double total_n = std::max(1.0, static_cast<double>(total_reward_samples_));

        auto score_candidate = [&](std::string_view id, double domain_bonus = 0.0) -> double {
            // Strict admission gate BEFORE any UCB1 or domain bonus calculation
            if (!is_candidate_eligible_for_context(id, current_workload_, thermal_bucket)) return -1.0;
            const auto* s = find_candidate(id);
            if (s == nullptr) return -1.0;
            const double explore = 0.25 * std::sqrt(std::log(total_n + 1.0) / static_cast<double>(s->reward_samples + 1));
            return s->mean_utility + explore + domain_bonus;
        };

        const bool any_mq_deadline = std::any_of(block_queues_.begin(), block_queues_.end(), [](const auto& q) {
            return contains_token(q.available_schedulers, "mq-deadline");
        });
        const bool any_kyber = std::any_of(block_queues_.begin(), block_queues_.end(), [](const auto& q) {
            return contains_token(q.available_schedulers, "kyber");
        });
        const bool any_bfq = std::any_of(block_queues_.begin(), block_queues_.end(), [](const auto& q) {
            return contains_token(q.available_schedulers, "bfq");
        });

        std::string best_id;
        double best_score = -1.0;

        if (any_mq_deadline) {
            const double sc = score_candidate("io.mq_deadline_low_latency");
            if (sc > best_score) {
                best_score = sc;
                best_id = "io.mq_deadline_low_latency";
            }
        }
        if (any_kyber) {
            const double bonus = (telemetry.io_psi >= 5.0 && telemetry.cpu_psi >= 10.0) ? 0.15 : 0.0;
            const double sc = score_candidate("io.kyber_token_bucket", bonus);
            if (sc > best_score) {
                best_score = sc;
                best_id = "io.kyber_token_bucket";
            }
        }
        if (any_bfq) {
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
            const auto* stats = find_candidate(best_id);
            const double scale = stats ? stats->adaptive_scale : 1.0;
            std::vector<MutationStep> steps;
            for (const auto& q : block_queues_) {
                if (!contains_token(q.available_schedulers, "mq-deadline")) continue;
                steps.push_back({q.queue_dir + "/scheduler", "mq-deadline", "Anchor (Required-before)", StepRelation::RequiredAnchor});
                if (q.has_iosched_tunables) {
                    const std::string read_exp_path = q.queue_dir + "/iosched/read_expire";
                    const uint64_t rel_read_expire = compute_relative_u64(read_exp_path, q.read_expire_ms, 0.50, 100, 2000, scale);
                    steps.push_back({read_exp_path, std::to_string(rel_read_expire), "Coupled-with", StepRelation::RequiredDependent});

                    const std::string starved_path = q.queue_dir + "/iosched/writes_starved";
                    const uint64_t rel_starved = compute_relative_u64(starved_path, q.writes_starved, 1.50, 1, 32, scale);
                    steps.push_back({starved_path, std::to_string(rel_starved), "Coupled-with", StepRelation::RequiredDependent});

                    const std::string fifo_path = q.queue_dir + "/iosched/fifo_batch";
                    const uint64_t rel_fifo = compute_relative_u64(fifo_path, q.fifo_batch, 0.50, 1, 64, scale);
                    steps.push_back({fifo_path, std::to_string(rel_fifo), "Coupled-with", StepRelation::RequiredDependent});
                }
                steps.push_back({q.queue_dir + "/rq_affinity", "2", "Coupled-with", StepRelation::RequiredDependent});

                const std::string readahead_path = q.queue_dir + "/read_ahead_kb";
                const uint64_t rel_readahead = compute_relative_u64(readahead_path, q.baseline_read_ahead_kb, 2.00, 64, 2048, scale);
                steps.push_back({readahead_path, std::to_string(rel_readahead), "Coupled-with", StepRelation::RequiredDependent});
            }
            (void)apply_transaction(
                "io.mq_deadline_low_latency",
                "mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)",
                "IoLatencyRelief",
                "Measured I/O PSI stall (" + std::to_string(telemetry.io_psi).substr(0, 4) +
                    "%); switching scheduler from " + primary_q.active_scheduler +
                    " to mq-deadline with relative 0.5x read_expire & fifo_batch scaling from OEM baseline.",
                steps,
                telemetry);
            return snapshot();
        }

        if (best_id == "io.kyber_token_bucket") {
            const auto* stats = find_candidate(best_id);
            const double scale = stats ? stats->adaptive_scale : 1.0;
            std::vector<MutationStep> steps;
            for (const auto& q : block_queues_) {
                if (!contains_token(q.available_schedulers, "kyber")) continue;
                steps.push_back({q.queue_dir + "/scheduler", "kyber", "Anchor (Required-before)", StepRelation::RequiredAnchor});
                const std::string nr_req_path = q.queue_dir + "/nr_requests";
                const uint64_t rel_nr_requests = compute_relative_u64(nr_req_path, q.baseline_nr_requests, 2.00, 32, 512, scale);
                steps.push_back({nr_req_path, std::to_string(rel_nr_requests), "Coupled-with", StepRelation::RequiredDependent});

                const std::string readahead_path = q.queue_dir + "/read_ahead_kb";
                const uint64_t rel_readahead = compute_relative_u64(readahead_path, q.baseline_read_ahead_kb, 1.00, 64, 2048, scale);
                steps.push_back({readahead_path, std::to_string(rel_readahead), "Coupled-with", StepRelation::RequiredDependent});
                steps.push_back({q.queue_dir + "/rq_affinity", "2", "Coupled-with", StepRelation::RequiredDependent});
            }
            (void)apply_transaction(
                "io.kyber_token_bucket",
                "kyber Token-Bucket Low-Latency I/O Scheduler",
                "IoLatencyRelief",
                "Concurrent I/O (" + std::to_string(telemetry.io_psi).substr(0, 4) +
                    "%) & CPU load selected via UCB1 bandit; switching scheduler from " +
                    primary_q.active_scheduler + " to kyber token-bucket arbitration with relative baseline scaling.",
                steps,
                telemetry);
            return snapshot();
        }

        if (best_id == "io.bfq_interactive_fairness") {
            const auto* stats = find_candidate(best_id);
            const double scale = stats ? stats->adaptive_scale : 1.0;
            std::vector<MutationStep> steps;
            for (const auto& q : block_queues_) {
                if (!contains_token(q.available_schedulers, "bfq")) continue;
                steps.push_back({q.queue_dir + "/scheduler", "bfq", "Anchor (Required-before)", StepRelation::RequiredAnchor});
                const std::string readahead_path = q.queue_dir + "/read_ahead_kb";
                const uint64_t rel_readahead = compute_relative_u64(readahead_path, q.baseline_read_ahead_kb, 2.00, 64, 2048, scale);
                steps.push_back({readahead_path, std::to_string(rel_readahead), "Coupled-with", StepRelation::RequiredDependent});
            }
            (void)apply_transaction(
                "io.bfq_interactive_fairness",
                "bfq Proportional-Share Interactive I/O Scheduler",
                "IoLatencyRelief",
                "Sustained I/O contention (" + std::to_string(telemetry.io_psi).substr(0, 4) +
                    "%); evaluating bfq proportional-share scheduler against OEM baseline with relative read-ahead scaling.",
                steps,
                telemetry);
            return snapshot();
        }

        if (best_id == "io.queue_readahead_pacing") {
            const auto* stats = find_candidate(best_id);
            const double scale = stats ? stats->adaptive_scale : 1.0;
            std::vector<MutationStep> steps;
            for (const auto& q : block_queues_) {
                const std::string readahead_path = q.queue_dir + "/read_ahead_kb";
                const uint64_t rel_readahead = compute_relative_u64(readahead_path, q.baseline_read_ahead_kb, 2.00, 64, 2048, scale);
                steps.push_back({readahead_path, std::to_string(rel_readahead), "Anchor (Required-before)", StepRelation::RequiredAnchor});

                const std::string nr_req_path = q.queue_dir + "/nr_requests";
                const uint64_t rel_nr_requests = compute_relative_u64(nr_req_path, q.baseline_nr_requests, 2.00, 32, 512, scale);
                steps.push_back({nr_req_path, std::to_string(rel_nr_requests), "Coupled-with", StepRelation::RequiredDependent});
            }
            (void)apply_transaction(
                "io.queue_readahead_pacing",
                "Adaptive Block Queue Read-Ahead & Request Depth (2.0x OEM baseline)",
                "IoLatencyRelief",
                "I/O stall pressure elevated; relative scaling of block queue read_ahead_kb and nr_requests from OEM baseline.",
                steps,
                telemetry);
            return snapshot();
        }
    }

    // Priority B: Memory PSI Contention -> VM Subsystem Relief Transaction (requires active zram swap)
    if (mem_bottleneck && is_candidate_eligible_for_context("vm.zram_latency_relief", current_workload_, thermal_bucket)) {
        const auto page_cluster_path = resolve_vm_node_path("vm.page_cluster");
        const auto cache_pressure_path = resolve_vm_node_path("vm.vfs_cache_pressure");
        if (page_cluster_path.empty() || cache_pressure_path.empty()) return snapshot();
        std::vector<MutationStep> steps{
            {page_cluster_path, "0", "Anchor (Required-before)", StepRelation::RequiredAnchor},
            {cache_pressure_path, "80", "Coupled-with", StepRelation::RequiredDependent},
        };
        (void)apply_transaction(
            "vm.zram_latency_relief",
            "VM Page-Cluster Zero-Latency & VFS Dentry Preservation",
            "MemoryStallRelief",
            "Memory PSI stall (" + std::to_string(telemetry.memory_psi).substr(0, 4) +
                "%) detected with active zram swap; setting vm.page-cluster=0 and vfs_cache_pressure=80.",
            steps,
            telemetry);
        return snapshot();
    }

    // Priority C: Interactive CPU PSI Bottleneck -> Smart CPUFreq Governor Rate-Limit Pacing
    if (cpu_bottleneck && is_candidate_eligible_for_context("cpufreq.schedutil_interactive_response", current_workload_, thermal_bucket)) {
        std::vector<MutationStep> steps;
        for (const auto& gdir : cpufreq_governor_dirs_) {
            const fs::path gpath(gdir);
            const std::string gov_name = gpath.filename().string();
            const fs::path scaling_gov_path = gpath.parent_path() / "scaling_governor";
            const std::string active_gov = DaemonConfig::trim(read_first_line(scaling_gov_path.string()));
            if (!active_gov.empty() && active_gov != gov_name) {
                continue;
            }
            steps.push_back({gdir + "/up_rate_limit_us", "500", "Anchor (Required-before)", StepRelation::RequiredAnchor});
            steps.push_back({gdir + "/down_rate_limit_us", "10000", "Coupled-with", StepRelation::RequiredDependent});
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
    out.workload_name = std::string(workload_class_name(current_workload_));
    out.bottleneck = current_bottleneck_;
    out.bottleneck_name = std::string(bottleneck_kind_name(current_bottleneck_));
    out.objective = current_objective_;
    out.objective_name = std::string(objective_name(current_objective_));
    out.capability_fingerprint = compute_fingerprint();
    out.active_candidate_id = current_candidate_id_;
    out.active_candidate_label = current_candidate_label_;
    if (frequency_mutation_active_ && frequency_actuator_ &&
        frequency_actuator_->mutation_active()) {
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
    out.after_io_psi = valid_io_samples_in_window_ > 0 ? (sum_after_io_psi_ / static_cast<double>(valid_io_samples_in_window_)) : before_io_psi_;
    out.before_mem_psi = before_mem_psi_;
    out.after_mem_psi = valid_mem_samples_in_window_ > 0 ? (sum_after_mem_psi_ / static_cast<double>(valid_mem_samples_in_window_)) : before_mem_psi_;
    out.before_cpu_psi = before_cpu_psi_;
    out.after_cpu_psi = valid_cpu_samples_in_window_ > 0 ? (sum_after_cpu_psi_ / static_cast<double>(valid_cpu_samples_in_window_)) : before_cpu_psi_;
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
    const auto page_cluster_path = resolve_vm_node_path("vm.page_cluster");
    const auto cache_pressure_path = resolve_vm_node_path("vm.vfs_cache_pressure");
    if (!page_cluster_path.empty()) out.vm_page_cluster = read_first_line(page_cluster_path);
    if (!cache_pressure_path.empty()) out.vm_vfs_cache_pressure = read_first_line(cache_pressure_path);
    return out;
}

} // namespace coreflow

