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
    if (v == "INCONCLUSIVE") return TunerVerdict::Inconclusive;
    if (v == "RECOVERY_REQUIRED") return TunerVerdict::RecoveryRequired;
    if (v == "PENDING") return TunerVerdict::Pending;
    return TunerVerdict::Hold;
}

std::string resolve_state_dir() {
    const char* test = std::getenv("COREFLOW_TEST_STATE_DIR");
    return (test && *test) ? std::string(test) : "/data/adb/coreflow";
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

uint64_t compute_relative_u64(
    const std::string& path,
    uint64_t fallback_baseline,
    double ratio,
    uint64_t min_bound,
    uint64_t max_bound) {
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
    if (baseline == 0) baseline = fallback_baseline;
    const double calc = static_cast<double>(baseline) * ratio;
    const uint64_t rounded = std::max<uint64_t>(1U, static_cast<uint64_t>(std::round(calc)));
    return std::clamp(rounded, min_bound, max_bound);
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
            .label = "mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)",
            .mean_utility = 0.65,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.86,
            .trials = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
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
            .consecutive_non_improved = 0,
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
            .consecutive_non_improved = 0,
        },
        CandidateStats{
            .id = "io.queue_readahead_pacing",
            .domain = "io",
            .label = "Adaptive Block Queue Read-Ahead & Request Depth (2.0x OEM baseline)",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.71,
            .trials = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
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
            .consecutive_non_improved = 0,
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
            .consecutive_non_improved = 0,
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
    (void)rollback_active("Autonomous tuner reset to verified OEM baseline.");
    recovery_required_ = false;
    applied_steps_.clear();
    init_candidates();
    total_trials_ = 0;
    total_improved_ = 0;
    total_rolled_back_ = 0;
    last_verdict_ = "HOLD";
    last_verdict_enum_ = TunerVerdict::Hold;
    last_utility_delta_ = 0.0;
    idle_streak_ = 0;
    std::error_code ec;
    fs::remove(fs::path(resolve_state_dir()) / "tuner_stats.state", ec);
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

bool AutonomousTuner::is_candidate_eligible(std::string_view id) const noexcept {
    const auto* c = find_candidate(id);
    if (c == nullptr || c->cooldown_ticks > 0) return false;
    if (id == "vm.zram_latency_relief" && !zram_active_) return false;
    if (id == "cpufreq.schedutil_interactive_response" && cpufreq_governor_dirs_.empty()) return false;
    return true;
}

std::string AutonomousTuner::compute_fingerprint() const {
    std::string fp = "v1|zram=" + std::string(zram_active_ ? "1" : "0");
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
    const fs::path state_dir = resolve_state_dir();
    std::error_code ec;
    fs::create_directories(state_dir, ec);
    if (ec) return;
    const fs::path tmp_path = state_dir / "tuner_stats.state.tmp";
    const fs::path final_path = state_dir / "tuner_stats.state";
    std::ofstream out(tmp_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return;
    out << "CFTS1\t" << compute_fingerprint() << "\t"
        << total_trials_ << "\t" << total_improved_ << "\t" << total_rolled_back_ << "\n";
    for (const auto& c : candidates_) {
        out << c.id << "\t"
            << c.mean_utility << "\t"
            << c.mean_utility_delta << "\t"
            << c.trials << "\t"
            << c.improved_count << "\t"
            << c.regressed_count << "\t"
            << c.cooldown_ticks << "\t"
            << c.consecutive_non_improved << "\n";
    }
    out.flush();
    out.close();
    fs::rename(tmp_path, final_path, ec);
}

void AutonomousTuner::load_persisted_stats() {
    const fs::path final_path = fs::path(resolve_state_dir()) / "tuner_stats.state";
    std::ifstream in(final_path);
    if (!in.is_open()) return;
    std::string header;
    if (!std::getline(in, header)) return;
    std::istringstream hs(header);
    std::string magic, fp;
    uint32_t trials = 0, improved = 0, rolled_back = 0;
    if (!(hs >> magic >> fp >> trials >> improved >> rolled_back)) return;
    if (magic != "CFTS1" || fp != compute_fingerprint()) return;

    total_trials_ = trials;
    total_improved_ = improved;
    total_rolled_back_ = rolled_back;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ls(line);
        std::string id;
        double mean_u = 0.5, mean_du = 0.0;
        uint32_t c_trials = 0, c_imp = 0, c_reg = 0, c_cool = 0, c_non_imp = 0;
        if (!(ls >> id >> mean_u >> mean_du >> c_trials >> c_imp >> c_reg >> c_cool >> c_non_imp)) {
            continue;
        }
        if (auto* c = find_candidate(id)) {
            c->mean_utility = std::clamp(mean_u, 0.05, 2.50);
            c->mean_utility_delta = mean_du;
            c->trials = c_trials;
            c->improved_count = c_imp;
            c->successes = c_imp;
            c->regressed_count = c_reg;
            c->rollbacks = c_reg;
            c->cooldown_ticks = std::min<uint32_t>(c_cool, 64U);
            c->consecutive_non_improved = std::min<uint32_t>(c_non_imp, 16U);
        }
    }
    update_ucb_scores();
}

void AutonomousTuner::discover(std::string_view sys_root, std::string_view proc_root) {
    sys_root_ = std::string(sys_root);
    proc_root_ = std::string(proc_root);
    block_queues_.clear();
    cpufreq_governor_dirs_.clear();
    zram_active_ = check_zram_swap_active(proc_root_, sys_root_);

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
    // Verify policyX/scaling_governor matches the governor directory if scaling_governor exists.
    auto& store = BaselineStore::instance();
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
    std::sort(cpufreq_governor_dirs_.begin(), cpufreq_governor_dirs_.end());

    // Capture VM baseline nodes if accessible
    (void)store.capture(proc_root_ + "/sys/vm/page-cluster");
    (void)store.capture(proc_root_ + "/sys/vm/vfs_cache_pressure");

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

bool AutonomousTuner::rollback_active(std::string_view reason) {
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

    if (!failed_steps.empty()) {
        std::reverse(failed_steps.begin(), failed_steps.end());
        applied_steps_ = std::move(failed_steps);
        transaction_active_ = false;
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) + " [ROLLBACK FAILED: RECOVERY_REQUIRED]";
        refresh_surface_readings();
        return false;
    }

    applied_steps_.clear();
    transaction_active_ = false;
    recovery_required_ = false;
    current_candidate_id_ = "baseline";
    current_candidate_label_ = "OEM Verified Baseline";
    current_intent_ = "HoldEquilibrium";
    current_rationale_ = std::string(reason);
    refresh_surface_readings();
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

    // First unwind any previous transaction cleanly
    if (!applied_steps_.empty()) {
        if (!rollback_active("Switching coordinated candidate")) {
            return false;
        }
    }

    auto& store = BaselineStore::instance();
    if (store.identity_unavailable()) return false;

    // Phase 1: Full Preflight (All-or-Nothing for RequiredAnchor & RequiredDependent steps)
    std::vector<MutationStep> planned;
    planned.reserve(steps.size());
    bool has_anchor = false;

    for (const auto& step : steps) {
        const bool is_required = (step.relation == StepRelation::RequiredAnchor ||
                                  step.relation == StepRelation::RequiredDependent);
        const bool accessible = (::access(step.path.c_str(), R_OK | W_OK) == 0) &&
                                store.capture(step.path);
        if (is_required && !accessible) {
            // Abort entire transaction before any write occurs
            return false;
        }
        if (accessible) {
            if (step.relation == StepRelation::RequiredAnchor) {
                has_anchor = true;
            }
            planned.push_back(step);
        }
    }

    if (!has_anchor || planned.empty()) return false;

    // Phase 2: Ordered Execution (RequiredAnchor before RequiredDependent)
    std::vector<MutationStep> committed;
    committed.reserve(planned.size());

    for (const auto& step : planned) {
        if (!store.apply_mutation(step.path, step.target_value)) {
            const bool is_required = (step.relation == StepRelation::RequiredAnchor ||
                                      step.relation == StepRelation::RequiredDependent);
            if (!is_required) {
                continue;
            }
            // Dependency or readback failure on required step: unwind committed steps in reverse order
            applied_steps_ = std::move(committed);
            (void)rollback_active("Transaction aborted on failed required step; rolled back to baseline.");
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
    min_after_io_psi_ = 1e9;
    max_after_io_psi_ = 0.0;
    ++total_trials_;

    if (auto* stats = find_candidate(candidate_id)) {
        ++stats->trials;
    }
    save_persisted_stats();
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

    // Verify both readback value and inode ownership across all applied nodes
    if (!verify_active_ownership()) {
        last_verdict_ = "OWNERSHIP_LOST";
        last_verdict_enum_ = TunerVerdict::OwnershipLost;
        ++total_rolled_back_;
        (void)rollback_active("External node ownership lost during observation window.");
        save_persisted_stats();
        return;
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
            (stats->mean_utility_delta * prev_trials + utility_delta) / static_cast<double>(std::max(1u, stats->trials));
    }

    // Check for high-variance / thermal-contaminated inconclusive observation window
    const double io_spread = (max_after_io_psi_ >= min_after_io_psi_) ? (max_after_io_psi_ - min_after_io_psi_) : 0.0;
    const bool high_variance_inconclusive =
        (io_spread > std::max(6.0, before_io_psi_ * 1.2) && std::abs(utility_delta) < 0.50);

    if (high_variance_inconclusive) {
        last_verdict_ = "INCONCLUSIVE";
        last_verdict_enum_ = TunerVerdict::Inconclusive;
        ++total_rolled_back_;
        if (stats != nullptr) {
            ++stats->consecutive_non_improved;
            const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
            stats->cooldown_ticks = std::min<uint32_t>(32U, 4U * (1U << shift));
        }
        const std::string msg =
            "Trial " + current_candidate_id_ + " inconclusive due to high sample variance; reverted to OEM baseline.";
        (void)rollback_active(msg);
        save_persisted_stats();
        return;
    }

    if (utility_delta >= 0.25) {
        // IMPROVED: Measurable benefit verified -> Retain configuration!
        last_verdict_ = "IMPROVED";
        last_verdict_enum_ = TunerVerdict::Improved;
        ++total_improved_;
        if (stats != nullptr) {
            ++stats->improved_count;
            stats->successes = stats->improved_count;
            stats->consecutive_non_improved = 0;
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
        // REGRESSED: Stalls or thermals worsened -> Rollback & penalize bandit score with exponential backoff!
        last_verdict_ = "REGRESSED";
        last_verdict_enum_ = TunerVerdict::Regressed;
        ++total_rolled_back_;
        if (stats != nullptr) {
            ++stats->regressed_count;
            stats->rollbacks = stats->regressed_count;
            ++stats->consecutive_non_improved;
            stats->mean_utility = std::max(0.05, stats->mean_utility - 0.35);
            const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
            stats->cooldown_ticks = std::min<uint32_t>(64U, 8U * (1U << shift));
        }
        const std::string msg =
            "Trial " + current_candidate_id_ + " regressed (ΔU=" +
            std::to_string(utility_delta).substr(0, 5) +
            ", io_psi " + std::to_string(before_io_psi_).substr(0, 4) +
            "% -> " + std::to_string(after_io).substr(0, 4) +
            "%); rolled back to OEM baseline.";
        (void)rollback_active(msg);
    } else {
        // NEUTRAL: No statistically meaningful benefit -> Rollback per Invariant 8 with exponential backoff
        last_verdict_ = "NEUTRAL";
        last_verdict_enum_ = TunerVerdict::Neutral;
        ++total_rolled_back_;
        if (stats != nullptr) {
            ++stats->consecutive_non_improved;
            const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
            stats->cooldown_ticks = std::min<uint32_t>(32U, 4U * (1U << shift));
        }
        const std::string msg =
            "Trial " + current_candidate_id_ + " neutral (ΔU=" +
            std::to_string(utility_delta).substr(0, 4) +
            "); reverted to OEM baseline per Invariant 8 (no unverified retention).";
        (void)rollback_active(msg);
    }
    save_persisted_stats();
}

AutonomousDecisionSnapshot AutonomousTuner::step(const TunerTelemetryInput& telemetry) {
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
    }
    if (cooldown_changed) {
        save_persisted_stats();
    }

    // Safety Gate: ThermalGuard immediately rolls back non-stabilizing tuning
    if (telemetry.thermal_guard_active || telemetry.thermal_emergency || telemetry.thermal_pressure >= 88.0) {
        if (transaction_active_) {
            last_verdict_ = "ABORTED";
            last_verdict_enum_ = TunerVerdict::Aborted;
            ++total_rolled_back_;
            (void)rollback_active("Thermal safety gate active; restored OEM block/VM baseline.");
            save_persisted_stats();
        }
        return snapshot();
    }

    // Stage 1: If currently observing an applied transaction, collect After-Window samples
    if (observing_) {
        if (!verify_active_ownership()) {
            last_verdict_ = "OWNERSHIP_LOST";
            last_verdict_enum_ = TunerVerdict::OwnershipLost;
            ++total_rolled_back_;
            (void)rollback_active("External node ownership lost during observation window.");
            save_persisted_stats();
            return snapshot();
        }
        sum_after_io_psi_ += telemetry.io_psi;
        sum_after_mem_psi_ += telemetry.memory_psi;
        sum_after_cpu_psi_ += telemetry.cpu_psi;
        sum_after_thermal_ += telemetry.thermal_pressure;
        min_after_io_psi_ = std::min(min_after_io_psi_, telemetry.io_psi);
        max_after_io_psi_ = std::max(max_after_io_psi_, telemetry.io_psi);
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

    // Stage 2: If a transaction is currently RETAINED, verify ownership and cross-domain guards
    if (transaction_active_) {
        if (!verify_active_ownership()) {
            last_verdict_ = "OWNERSHIP_LOST";
            last_verdict_enum_ = TunerVerdict::OwnershipLost;
            ++total_rolled_back_;
            (void)rollback_active("External node ownership lost while transaction was retained.");
            save_persisted_stats();
            return snapshot();
        }
        if (!io_bottleneck && !mem_bottleneck && telemetry.cpu_psi < 4.0) {
            if (++idle_streak_ >= 6) {
                idle_streak_ = 0;
                last_verdict_ = "HOLD";
                last_verdict_enum_ = TunerVerdict::Hold;
                (void)rollback_active("Workload returned to sustained idle; restored OEM baseline.");
            }
        } else {
            idle_streak_ = 0;
            // Monitor domain-specific target and guard metrics while retained
            const bool domain_regressed =
                (telemetry.io_psi > before_io_psi_ + 4.0) ||
                (current_intent_ == "MemoryStallRelief" && telemetry.memory_psi > before_mem_psi_ + 3.5) ||
                (current_intent_ == "InteractiveCpuPacing" &&
                 (telemetry.cpu_psi > before_cpu_psi_ + 6.0 || telemetry.thermal_pressure > before_thermal_ + 8.0));
            if (domain_regressed) {
                if (auto* stats = find_candidate(current_candidate_id_)) {
                    stats->cooldown_ticks = 6;
                    save_persisted_stats();
                }
                (void)rollback_active("Workload regime shifted; releasing retained candidate to re-plan.");
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

        auto score_candidate = [&](std::string_view id, double domain_bonus = 0.0) -> double {
            // Strict admission gate BEFORE any UCB1 or domain bonus calculation (fixes Kyber cooldown bypass)
            if (!is_candidate_eligible(id)) return -1.0;
            const auto* s = find_candidate(id);
            if (s == nullptr) return -1.0;
            const double explore = 0.25 * std::sqrt(std::log(total_n + 1.0) / static_cast<double>(s->trials + 1));
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
            std::vector<MutationStep> steps;
            for (const auto& q : block_queues_) {
                if (!contains_token(q.available_schedulers, "mq-deadline")) continue;
                steps.push_back({q.queue_dir + "/scheduler", "mq-deadline", "Anchor (Required-before)", StepRelation::RequiredAnchor});
                if (q.has_iosched_tunables) {
                    const std::string read_exp_path = q.queue_dir + "/iosched/read_expire";
                    const uint64_t rel_read_expire = compute_relative_u64(read_exp_path, q.read_expire_ms, 0.50, 100, 2000);
                    steps.push_back({read_exp_path, std::to_string(rel_read_expire), "Coupled-with", StepRelation::RequiredDependent});

                    const std::string starved_path = q.queue_dir + "/iosched/writes_starved";
                    const uint64_t rel_starved = compute_relative_u64(starved_path, q.writes_starved, 1.50, 1, 32);
                    steps.push_back({starved_path, std::to_string(rel_starved), "Coupled-with", StepRelation::RequiredDependent});

                    const std::string fifo_path = q.queue_dir + "/iosched/fifo_batch";
                    const uint64_t rel_fifo = compute_relative_u64(fifo_path, q.fifo_batch, 0.50, 1, 64);
                    steps.push_back({fifo_path, std::to_string(rel_fifo), "Coupled-with", StepRelation::RequiredDependent});
                }
                steps.push_back({q.queue_dir + "/rq_affinity", "2", "Coupled-with", StepRelation::RequiredDependent});

                const std::string readahead_path = q.queue_dir + "/read_ahead_kb";
                const uint64_t rel_readahead = compute_relative_u64(readahead_path, q.baseline_read_ahead_kb, 2.00, 64, 2048);
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
            std::vector<MutationStep> steps;
            for (const auto& q : block_queues_) {
                if (!contains_token(q.available_schedulers, "kyber")) continue;
                steps.push_back({q.queue_dir + "/scheduler", "kyber", "Anchor (Required-before)", StepRelation::RequiredAnchor});
                const std::string nr_req_path = q.queue_dir + "/nr_requests";
                const uint64_t rel_nr_requests = compute_relative_u64(nr_req_path, q.baseline_nr_requests, 2.00, 32, 512);
                steps.push_back({nr_req_path, std::to_string(rel_nr_requests), "Coupled-with", StepRelation::RequiredDependent});

                const std::string readahead_path = q.queue_dir + "/read_ahead_kb";
                const uint64_t rel_readahead = compute_relative_u64(readahead_path, q.baseline_read_ahead_kb, 1.00, 64, 2048);
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
            std::vector<MutationStep> steps;
            for (const auto& q : block_queues_) {
                if (!contains_token(q.available_schedulers, "bfq")) continue;
                steps.push_back({q.queue_dir + "/scheduler", "bfq", "Anchor (Required-before)", StepRelation::RequiredAnchor});
                const std::string readahead_path = q.queue_dir + "/read_ahead_kb";
                const uint64_t rel_readahead = compute_relative_u64(readahead_path, q.baseline_read_ahead_kb, 2.00, 64, 2048);
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
            std::vector<MutationStep> steps;
            for (const auto& q : block_queues_) {
                const std::string readahead_path = q.queue_dir + "/read_ahead_kb";
                const uint64_t rel_readahead = compute_relative_u64(readahead_path, q.baseline_read_ahead_kb, 2.00, 64, 2048);
                steps.push_back({readahead_path, std::to_string(rel_readahead), "Anchor (Required-before)", StepRelation::RequiredAnchor});

                const std::string nr_req_path = q.queue_dir + "/nr_requests";
                const uint64_t rel_nr_requests = compute_relative_u64(nr_req_path, q.baseline_nr_requests, 2.00, 32, 512);
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
    if (mem_bottleneck && is_candidate_eligible("vm.zram_latency_relief")) {
        std::vector<MutationStep> steps{
            {proc_root_ + "/sys/vm/page-cluster", "0", "Anchor (Required-before)", StepRelation::RequiredAnchor},
            {proc_root_ + "/sys/vm/vfs_cache_pressure", "80", "Coupled-with", StepRelation::RequiredDependent},
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
    if (cpu_bottleneck && is_candidate_eligible("cpufreq.schedutil_interactive_response")) {
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
    AutonomousDecisionSnapshot out{};
    if (recovery_required_) {
        out.stage = "RECOVERY_REQUIRED";
    } else if (observing_) {
        out.stage = "OBSERVING";
    } else if (transaction_active_) {
        out.stage = "RETAINED";
    } else {
        out.stage = "PLANNED";
    }
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
    out.recovery_required = recovery_required_;
    out.block_queues = block_queues_;
    out.candidates = candidates_;
    out.vm_page_cluster = read_first_line(proc_root_ + "/sys/vm/page-cluster");
    out.vm_vfs_cache_pressure = read_first_line(proc_root_ + "/sys/vm/vfs_cache_pressure");
    return out;
}

} // namespace coreflow
