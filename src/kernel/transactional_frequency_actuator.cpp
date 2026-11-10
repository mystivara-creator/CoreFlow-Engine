#include "coreflow/transactional_frequency_actuator.hpp"

#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"

#include <algorithm>
#include <charconv>
#include <cerrno>
#include <time.h>
#include <cctype>
#include <cmath>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace {

uint64_t monotonic_now_ns() noexcept {
    timespec ts{};
    if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0 || ts.tv_sec < 0 || ts.tv_nsec < 0) return 0;
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL +
           static_cast<uint64_t>(ts.tv_nsec);
}

bool read_frequency_khz(const std::string& path, uint64_t& value) noexcept {
    value = 0;
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        (void)::close(fd);
        return false;
    }
    char buffer[64]{};
    ssize_t count = 0;
    do { count = ::read(fd, buffer, sizeof(buffer) - 1U); }
    while (count < 0 && errno == EINTR);
    (void)::close(fd);
    if (count <= 0) return false;
    const char* begin = buffer;
    const char* end = buffer + count;
    while (begin < end && (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n')) ++begin;
    while (end > begin && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) --end;
    const auto parsed = std::from_chars(begin, end, value, 10);
    return parsed.ec == std::errc{} && parsed.ptr == end && value > 0;
}

bool contains_case_insensitive(std::string_view value, std::string_view token) noexcept {
    if (token.empty() || value.size() < token.size()) return false;
    for (size_t i = 0; i + token.size() <= value.size(); ++i) {
        size_t j = 0;
        for (; j < token.size(); ++j) {
            const auto a = static_cast<unsigned char>(value[i + j]);
            const auto b = static_cast<unsigned char>(token[j]);
            if (std::tolower(a) != std::tolower(b)) break;
        }
        if (j == token.size()) return true;
    }
    return false;
}

} // namespace

ThermalSafetyAuthority::ThermalSafetyAuthority(
    const ThermalMonitor& monitor,
    std::vector<std::string> required_sensor_tokens,
    uint64_t max_age_ns) noexcept
    : monitor_(monitor),
      required_sensor_tokens_(std::move(required_sensor_tokens)),
      max_age_ns_(max_age_ns) {}

ThermalAdmission ThermalSafetyAuthority::evaluate(uint64_t now_ns) const noexcept {
    ThermalAdmission admission{};
    admission.observed_at_ns = now_ns;
    if (now_ns == 0 || max_age_ns_ == 0 || required_sensor_tokens_.empty()) {
        admission.reason = "invalid thermal authority configuration";
        return admission;
    }

    const ThermalAssessment assessment = monitor_.assess(false);
    const uint64_t sampled_at = monotonic_now_ns();
    if (sampled_at == 0 || sampled_at < now_ns || sampled_at - now_ns > max_age_ns_) {
        admission.reason = "thermal sample timestamp invalid or stale";
        return admission;
    }
    admission.observed_at_ns = sampled_at;
    if (!assessment.available || assessment.emergency || assessment.incomplete ||
        assessment.degraded || !assessment.sensor_readings_complete) {
        admission.reason = "thermal assessment unavailable, incomplete, degraded, or emergency";
        return admission;
    }
    if (assessment.severity >= 5) {
        admission.reason = "OEM Android thermal severity at emergency threshold";
        return admission;
    }

    for (const auto& token : required_sensor_tokens_) {
        bool found = false;
        for (const auto& reading : assessment.readings) {
            if (!contains_case_insensitive(reading.type, token) &&
                !contains_case_insensitive(reading.id, token)) continue;
            if (!reading.valid || !reading.policy_available ||
                !std::isfinite(reading.celsius) || reading.celsius < 0.0F) {
                admission.reason = "required thermal sensor invalid or lacks OEM trip policy: " + token;
                return admission;
            }
            found = true;
            break;
        }
        if (!found) {
            admission.reason = "required thermal sensor missing: " + token;
            return admission;
        }
    }

    admission.granted = true;
    admission.reason = "fresh required sensors and OEM trip policy validated; no emergency";
    return admission;
}

bool FrequencyOutcomeEvaluator::begin(const OutcomeSample& baseline) noexcept {
    reset();
    if (baseline.timestamp_ns == 0 || !baseline.cpu_psi_valid ||
        !std::isfinite(baseline.cpu_psi_some_pct) ||
        baseline.cpu_psi_some_pct < 0.0 || baseline.cpu_psi_some_pct > 100.0) return false;
    if ((baseline.memory_psi_valid && (!std::isfinite(baseline.memory_psi_some_pct) || baseline.memory_psi_some_pct < 0.0 || baseline.memory_psi_some_pct > 100.0)) ||
        (baseline.io_psi_valid && (!std::isfinite(baseline.io_psi_some_pct) || baseline.io_psi_some_pct < 0.0 || baseline.io_psi_some_pct > 100.0)) ||
        (baseline.ebpf_wakeup_valid && (!std::isfinite(baseline.ebpf_wakeup_rate_per_sec) || baseline.ebpf_wakeup_rate_per_sec < 0.0))) return false;
    baseline_ = baseline;
    started_at_ns_ = baseline.timestamp_ns;
    active_ = true;
    return true;
}

OutcomeResult FrequencyOutcomeEvaluator::observe(const OutcomeSample& sample) noexcept {
    OutcomeResult result{};
    result.samples = samples_;
    if (!active_ || sample.timestamp_ns <= started_at_ns_ ||
        sample.timestamp_ns <= baseline_.timestamp_ns) {
        result.reason = "invalid or non-monotonic observation";
        return result;
    }
    if (sample.cpu_psi_valid &&
        (!std::isfinite(sample.cpu_psi_some_pct) || sample.cpu_psi_some_pct < 0.0 ||
         sample.cpu_psi_some_pct > 100.0)) {
        result.reason = "invalid CPU PSI sample";
        return result;
    }
    if (sample.memory_psi_valid &&
        (!std::isfinite(sample.memory_psi_some_pct) || sample.memory_psi_some_pct < 0.0 ||
         sample.memory_psi_some_pct > 100.0)) {
        result.reason = "invalid memory PSI sample";
        return result;
    }
    if (sample.io_psi_valid &&
        (!std::isfinite(sample.io_psi_some_pct) || sample.io_psi_some_pct < 0.0 ||
         sample.io_psi_some_pct > 100.0)) {
        result.reason = "invalid I/O PSI sample";
        return result;
    }
    if (sample.ebpf_wakeup_valid &&
        (!std::isfinite(sample.ebpf_wakeup_rate_per_sec) ||
         sample.ebpf_wakeup_rate_per_sec < 0.0)) {
        result.reason = "invalid eBPF wakeup sample";
        return result;
    }

    if (!sample.cpu_psi_valid ||
        (baseline_.memory_psi_valid && !sample.memory_psi_valid) ||
        (baseline_.io_psi_valid && !sample.io_psi_valid) ||
        (baseline_.ebpf_wakeup_valid && baseline_.workload_stable && !sample.ebpf_wakeup_valid)) {
        result.verdict = OutcomeVerdict::Inconclusive;
        result.reason = "required outcome telemetry became unavailable";
        active_ = false;
        return result;
    }

    ++samples_;
    const uint64_t elapsed = sample.timestamp_ns - started_at_ns_;
    const bool cpu_spike = sample.cpu_psi_valid &&
        (sample.cpu_psi_some_pct >= 35.0 ||
         (baseline_.cpu_psi_valid && sample.cpu_psi_some_pct - baseline_.cpu_psi_some_pct >= 10.0));
    const bool memory_spike = sample.memory_psi_valid &&
        (sample.memory_psi_some_pct >= 35.0 ||
         (baseline_.memory_psi_valid && sample.memory_psi_some_pct - baseline_.memory_psi_some_pct >= 10.0));
    const bool io_spike = sample.io_psi_valid &&
        (sample.io_psi_some_pct >= 35.0 ||
         (baseline_.io_psi_valid && sample.io_psi_some_pct - baseline_.io_psi_some_pct >= 10.0));
    const bool wakeup_collapse = baseline_.ebpf_wakeup_valid && sample.ebpf_wakeup_valid &&
        baseline_.workload_stable && sample.workload_stable &&
        baseline_.ebpf_wakeup_rate_per_sec > 0.0 &&
        sample.ebpf_wakeup_rate_per_sec < baseline_.ebpf_wakeup_rate_per_sec * 0.65;

    result.samples = samples_;
    if (cpu_spike || memory_spike || io_spike || wakeup_collapse) {
        result.verdict = OutcomeVerdict::Regressed;
        result.reason = cpu_spike ? "CPU PSI regression" :
            memory_spike ? "memory PSI regression" :
            io_spike ? "I/O PSI regression" : "eBPF wakeup rate collapsed under stable workload";
        active_ = false;
        return result;
    }

    if (elapsed > 5'000'000'000ULL) {
        result.verdict = OutcomeVerdict::Inconclusive;
        result.reason = "five-second outcome window expired";
        active_ = false;
        return result;
    }
    if (elapsed < 3'000'000'000ULL || samples_ < 3U) {
        result.verdict = OutcomeVerdict::Pending;
        result.reason = "collecting three-to-five-second observation window";
        return result;
    }

    const bool cpu_improved = sample.cpu_psi_valid && baseline_.cpu_psi_valid &&
        sample.cpu_psi_some_pct <= baseline_.cpu_psi_some_pct - 3.0;
    const bool wakeup_improved = sample.ebpf_wakeup_valid && baseline_.ebpf_wakeup_valid &&
        sample.workload_stable && baseline_.workload_stable &&
        sample.ebpf_wakeup_rate_per_sec >= baseline_.ebpf_wakeup_rate_per_sec * 1.15;
    result.verdict = (cpu_improved || wakeup_improved)
        ? OutcomeVerdict::Improved : OutcomeVerdict::Neutral;
    result.reason = result.verdict == OutcomeVerdict::Improved
        ? "stable observation window with measurable improvement"
        : "no material regression in valid observation window";
    active_ = false;
    return result;
}

void FrequencyOutcomeEvaluator::reset() noexcept {
    baseline_ = {};
    started_at_ns_ = 0;
    samples_ = 0;
    active_ = false;
}

TransactionalFrequencyActuator::TransactionalFrequencyActuator(
    CentralMutationCoordinator& coordinator,
    BaselineStore& baseline_store,
    const FrequencyTargetPlanner& planner,
    const ThermalSafetyAuthority& thermal,
    const IndependentFrequencyCeilingProvider& ceiling_provider,
    std::string sys_root)
    : coordinator_(coordinator),
      baseline_store_(baseline_store),
      planner_(planner),
      thermal_(thermal),
      ceiling_provider_(ceiling_provider),
      sys_root_(std::move(sys_root)),
      controller_id_(CentralMutationCoordinator::make_controller_identity("TransactionalFrequencyActuator")) {
    (void)coordinator_.register_preemption_handler(
        controller_id_,
        [this](MutationDomain domain, std::string_view id, std::string_view reason) noexcept {
            return handle_preemption(domain, id, reason);
        });
}

TransactionalFrequencyActuator::~TransactionalFrequencyActuator() {
    (void)rollback();
    coordinator_.unregister_preemption_handler(controller_id_);
}

bool TransactionalFrequencyActuator::apply(
    const std::string& policy_path,
    const FrequencyCapabilitySnapshot& capability,
    uint64_t current_hz,
    double requested_capacity_scale,
    const OutcomeSample& baseline_sample,
    uint64_t now_ns) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string policy_prefix = sys_root_ + "/devices/system/cpu/cpufreq/policy";
    if (active_ || recovery_required_ || sys_root_.empty() ||
        policy_path.empty() || policy_path.size() > 512U ||
        policy_path.find("..") != std::string::npos ||
        policy_path.rfind(policy_prefix, 0) != 0 ||
        policy_path.size() == policy_prefix.size() ||
        !std::all_of(policy_path.begin() + static_cast<std::ptrdiff_t>(policy_prefix.size()),
                     policy_path.end(), [](unsigned char ch) { return ch >= '0' && ch <= '9'; }) ||
        capability.domain != FrequencyDomain::CpuPolicy ||
        !FrequencyTargetPlanner::validate(capability)) return false;

    const uint64_t admission_now = monotonic_now_ns();
    if (admission_now == 0 || admission_now < now_ns || admission_now - now_ns > 250'000'000ULL) return false;
    if (baseline_sample.timestamp_ns == 0 || baseline_sample.timestamp_ns > admission_now ||
        admission_now - baseline_sample.timestamp_ns > 1'000'000'000ULL) return false;
    const auto thermal = thermal_.evaluate(admission_now);
    if (!thermal.granted || thermal.observed_at_ns < admission_now ||
        thermal.observed_at_ns - admission_now > 250'000'000ULL ||
        admission_now - now_ns > 250'000'000ULL) return false;

    const auto ceiling = ceiling_provider_.current(policy_path, admission_now);
    if (!ceiling.has_value() || !ceiling->signature_or_kernel_contract_verified ||
        ceiling->max_hz == 0 || ceiling->valid_until_ns < admission_now ||
        ceiling->capability_fingerprint.empty() ||
        ceiling->capability_fingerprint != capability.capability_fingerprint) return false;

    if (!capability.capability_fingerprint_valid ||
        ceiling->max_hz > capability.hardware_max_hz ||
        ceiling->max_hz < capability.active_constraint_min_hz ||
        capability.active_constraint_max_hz != ceiling->max_hz) return false;

    const auto target = planner_.select_target(capability, current_hz, requested_capacity_scale);
    if (!target.has_value() || *target > ceiling->max_hz ||
        *target <= capability.observed_policy_max_hz) return false;

    const std::string path = policy_path + "/scaling_max_freq";
    uint64_t current_khz = 0;
    if (!read_frequency_khz(path, current_khz) ||
        current_khz > std::numeric_limits<uint64_t>::max() / 1000ULL ||
        current_khz * 1000ULL != capability.observed_policy_max_hz ||
        *target % 1000ULL != 0ULL) return false;

    transaction_id_ = "p2d-frequency-" + std::to_string(admission_now);
    if (!coordinator_.acquire_lease(
            MutationDomain::CpuPolicy, controller_id_, transaction_id_,
            ControllerPriority::WorkloadOptimization)) {
        transaction_id_.clear();
        return false;
    }

    if (!baseline_store_.capture(path)) {
        coordinator_.release_lease(MutationDomain::CpuPolicy, controller_id_, transaction_id_);
        transaction_id_.clear();
        return false;
    }
    const auto audit = baseline_store_.get_audit_trail();
    const auto snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
        return node.path == path;
    });
    if (snapshot == audit.end() || snapshot->is_modified ||
        snapshot->factory_value != std::to_string(current_khz)) {
        coordinator_.release_lease(MutationDomain::CpuPolicy, controller_id_, transaction_id_);
        transaction_id_.clear();
        return false;
    }

    baseline_value_ = snapshot->factory_value;
    target_path_ = path;
    target_hz_ = *target;
    const uint64_t target_khz = *target / 1000ULL;
    const bool applied = baseline_store_.apply_mutation(path, std::to_string(target_khz));
    uint64_t readback = 0;
    if (!applied || !read_frequency_khz(path, readback) || readback != target_khz) {
        recovery_required_ = true;
        if (rollback_locked()) return false;
        return false;
    }

    active_ = true;
    if (!evaluator_.begin(baseline_sample)) {
        recovery_required_ = true;
        (void)rollback_locked();
        return false;
    }
    return true;
}

OutcomeResult TransactionalFrequencyActuator::observe(const OutcomeSample& sample) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_ || recovery_required_) return {OutcomeVerdict::Inconclusive, 0, "no active verified transaction"};
    OutcomeResult result = evaluator_.observe(sample);
    if (result.verdict == OutcomeVerdict::Regressed ||
        result.verdict == OutcomeVerdict::Inconclusive) {
        recovery_required_ = true;
        if (!rollback_locked()) result.reason += "; rollback verification failed, lease retained";
        else result.reason += "; OEM baseline restored and verified";
    }
    return result;
}

bool TransactionalFrequencyActuator::rollback_locked() noexcept {
    if (transaction_id_.empty()) return !recovery_required_;
    if (target_path_.empty() || baseline_value_.empty()) {
        recovery_required_ = true;
        return false;
    }
    if (!baseline_store_.rollback(target_path_)) {
        recovery_required_ = true;
        return false;
    }
    uint64_t readback = 0;
    uint64_t expected = 0;
    const auto parsed = std::from_chars(
        baseline_value_.data(), baseline_value_.data() + baseline_value_.size(), expected, 10);
    if (parsed.ec != std::errc{} || parsed.ptr != baseline_value_.data() + baseline_value_.size() ||
        !read_frequency_khz(target_path_, readback) || readback != expected) {
        const auto audit = baseline_store_.get_audit_trail();
        const auto node = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& item) {
            return item.path == target_path_;
        });
        if (node == audit.end() || node->is_modified || node->factory_value != baseline_value_) {
            // An external owner changed the node; never overwrite or silently release.
            recovery_required_ = true;
            return false;
        }
        recovery_required_ = true;
        return false;
    }

    const auto audit = baseline_store_.get_audit_trail();
    const auto node = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& item) {
        return item.path == target_path_;
    });
    if (node == audit.end() || node->is_modified || node->factory_value != baseline_value_) {
        recovery_required_ = true;
        return false;
    }

    coordinator_.release_lease(MutationDomain::CpuPolicy, controller_id_, transaction_id_);
    transaction_id_.clear();
    target_path_.clear();
    baseline_value_.clear();
    target_hz_ = 0;
    active_ = false;
    recovery_required_ = false;
    evaluator_.reset();
    return true;
}

bool TransactionalFrequencyActuator::rollback() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return rollback_locked();
}

bool TransactionalFrequencyActuator::mutation_active() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_;
}

bool TransactionalFrequencyActuator::recovery_required() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return recovery_required_;
}

bool TransactionalFrequencyActuator::handle_preemption(
    MutationDomain domain, std::string_view id, std::string_view) noexcept {
    if (domain != MutationDomain::CpuPolicy) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (transaction_id_.empty() || transaction_id_ != id) return false;
    return rollback_locked();
}

} // namespace coreflow
