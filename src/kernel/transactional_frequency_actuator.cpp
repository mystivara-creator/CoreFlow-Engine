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
#include <filesystem>
#include <vector>

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

bool read_small_text(const std::string& path, std::string& value) noexcept {
    value.clear();
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        (void)::close(fd);
        return false;
    }
    char buffer[257]{};
    ssize_t count = 0;
    do { count = ::read(fd, buffer, sizeof(buffer) - 1U); }
    while (count < 0 && errno == EINTR);
    (void)::close(fd);
    if (count <= 0 || count >= static_cast<ssize_t>(sizeof(buffer) - 1U)) return false;
    std::string_view view(buffer, static_cast<size_t>(count));
    const size_t first = view.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return false;
    const size_t last = view.find_last_not_of(" \t\r\n");
    value.assign(view.substr(first, last - first + 1U));
    return !value.empty();
}

bool parse_u64_text(std::string_view text, uint64_t& value, bool allow_zero) noexcept {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t' ||
           text.front() == '\r' || text.front() == '\n')) text.remove_prefix(1U);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' ||
           text.back() == '\r' || text.back() == '\n')) text.remove_suffix(1U);
    if (text.empty()) return false;
    uint64_t parsed = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed, 10);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
        (!allow_zero && parsed == 0)) return false;
    value = parsed;
    return true;
}

bool parse_cpu_list(std::string_view text, std::vector<uint64_t>& cpus) noexcept {
    cpus.clear();
    size_t offset = 0;
    while (offset < text.size()) {
        while (offset < text.size() &&
               (text[offset] == ' ' || text[offset] == '\t' ||
                text[offset] == '\r' || text[offset] == '\n' || text[offset] == ',')) ++offset;
        if (offset >= text.size()) break;
        const size_t start = offset;
        while (offset < text.size() &&
               text[offset] != ' ' && text[offset] != '\t' &&
               text[offset] != '\r' && text[offset] != '\n' && text[offset] != ',') ++offset;
        uint64_t cpu = 0;
        if (!parse_u64_text(text.substr(start, offset - start), cpu, true) || cpu > 4095U) {
            cpus.clear();
            return false;
        }
        cpus.push_back(cpu);
        if (cpus.size() > 4096U) {
            cpus.clear();
            return false;
        }
    }
    std::sort(cpus.begin(), cpus.end());
    cpus.erase(std::unique(cpus.begin(), cpus.end()), cpus.end());
    return !cpus.empty();
}

bool cdev_binds_to_policy(
    const std::filesystem::path& cdev_path,
    const std::string& policy_path,
    const std::vector<uint64_t>& related_cpus,
    uint64_t policy_index) noexcept {
    std::error_code ec;
    const auto target = std::filesystem::canonical(cdev_path / "device", ec);
    if (ec) return false;
    const std::string canonical_target = target.string();
    const auto canonical_policy = std::filesystem::canonical(policy_path, ec);
    if (!ec) {
        const std::string policy = canonical_policy.string();
        if (canonical_target == policy ||
            canonical_target.rfind(policy + "/", 0) == 0) return true;
    }

    for (const auto& component : target) {
        const std::string name = component.string();
        if (name.rfind("policy", 0) == 0) {
            uint64_t index = 0;
            if (parse_u64_text(std::string_view(name).substr(6), index, true) &&
                index == policy_index) return true;
        }
        if (name.rfind("cpu", 0) == 0) {
            uint64_t cpu = 0;
            if (parse_u64_text(std::string_view(name).substr(3), cpu, true) &&
                std::binary_search(related_cpus.begin(), related_cpus.end(), cpu)) return true;
        }
    }
    return false;
}

bool is_cpu_cooling_device(std::string_view type) noexcept {
    return contains_case_insensitive(type, "processor") ||
           contains_case_insensitive(type, "cpufreq") ||
           contains_case_insensitive(type, "cpu");
}

bool read_lmh_kernel_contract(
    const std::string& directory,
    uint64_t& maximum_khz,
    uint64_t& sample_timestamp_ns,
    std::string& fingerprint) noexcept {
    std::string version;
    std::string raw_frequency;
    std::string raw_timestamp;
    if (!read_small_text(directory + "/qcom_lmh_abi_version", version) || version != "1" ||
        !read_small_text(directory + "/qcom_lmh_max_freq", raw_frequency) ||
        !parse_u64_text(raw_frequency, maximum_khz, false) ||
        !read_small_text(directory + "/qcom_lmh_sample_timestamp_ns", raw_timestamp) ||
        !parse_u64_text(raw_timestamp, sample_timestamp_ns, false) ||
        !read_small_text(directory + "/qcom_lmh_capability_fingerprint", fingerprint) ||
        fingerprint.size() < 8U || fingerprint.size() > 128U) return false;
    for (const unsigned char ch : fingerprint) {
        if (!(std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == ':')) return false;
    }
    return true;
}

} // namespace

QualcommLmhFrequencyCeilingProvider::QualcommLmhFrequencyCeilingProvider(
    std::string sys_root, uint64_t max_sample_age_ns) noexcept
    : sys_root_(std::move(sys_root)), max_sample_age_ns_(max_sample_age_ns) {}

std::optional<IndependentFrequencyCeiling>
QualcommLmhFrequencyCeilingProvider::current(
    std::string_view policy_path_view, uint64_t now_ns) const noexcept {
    namespace fs = std::filesystem;
    if (sys_root_.empty() || sys_root_.size() > 1024U ||
        max_sample_age_ns_ == 0 || now_ns == 0 ||
        policy_path_view.empty() || policy_path_view.size() > 512U) return std::nullopt;

    const std::string policy_path(policy_path_view);
    const std::string policy_prefix =
        sys_root_ + "/devices/system/cpu/cpufreq/policy";
    if (policy_path.rfind(policy_prefix, 0) != 0 ||
        policy_path.size() == policy_prefix.size()) return std::nullopt;
    const std::string index_text = policy_path.substr(policy_prefix.size());
    uint64_t policy_index = 0;
    if (!parse_u64_text(index_text, policy_index, true) || policy_index > 255U) return std::nullopt;

    // This is deliberately not scaling_max_freq. The qcom driver identity and
    // explicit versioned LMh contract are required before a ceiling is trusted.
    std::string scaling_driver;
    if (!read_small_text(policy_path + "/scaling_driver", scaling_driver) ||
        scaling_driver != "qcom-cpufreq-hw") return std::nullopt;

    // read_frequency_khz returns the raw cpufreq ABI value in kHz.
    uint64_t hardware_min_khz = 0;
    uint64_t hardware_max_khz = 0;
    if (!read_frequency_khz(policy_path + "/cpuinfo_min_freq", hardware_min_khz) ||
        !read_frequency_khz(policy_path + "/cpuinfo_max_freq", hardware_max_khz) ||
        hardware_min_khz > hardware_max_khz) return std::nullopt;

    std::string cpu_list_text;
    std::vector<uint64_t> related_cpus;
    if (!read_small_text(policy_path + "/related_cpus", cpu_list_text) ||
        !parse_cpu_list(cpu_list_text, related_cpus)) return std::nullopt;

    const fs::path thermal_root(sys_root_ + "/class/thermal");
    std::error_code ec;
    if (!fs::is_directory(thermal_root, ec) || ec) return std::nullopt;

    bool found_contract = false;
    uint64_t selected_max_khz = 0;
    uint64_t selected_timestamp_ns = 0;
    std::string selected_fingerprint;
    size_t visited = 0;
    for (fs::directory_iterator it(thermal_root, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && visited++ < 512U; it.increment(ec)) {
        const std::string name = it->path().filename().string();
        if (name.rfind("cooling_device", 0) != 0) continue;
        std::error_code type_ec;
        if (!it->is_directory(type_ec) && !fs::is_symlink(it->path(), type_ec)) continue;

        const fs::path cdev_path = it->path();
        std::string type;
        std::string current_state_text;
        std::string maximum_state_text;
        uint64_t current_state = 0;
        uint64_t maximum_state = 0;
        if (!read_small_text((cdev_path / "type").string(), type) ||
            !is_cpu_cooling_device(type) ||
            !read_small_text((cdev_path / "cur_state").string(), current_state_text) ||
            !read_small_text((cdev_path / "max_state").string(), maximum_state_text) ||
            !parse_u64_text(current_state_text, current_state, true) ||
            !parse_u64_text(maximum_state_text, maximum_state, true) ||
            maximum_state == 0 || current_state > maximum_state) continue;

        // Cooling state is used only to validate that the thermal cooling device
        // is live and bound to this CPU policy. No state-to-frequency inference.
        if (!cdev_binds_to_policy(cdev_path, policy_path, related_cpus, policy_index)) continue;

        uint64_t candidate_max_khz = 0;
        uint64_t candidate_timestamp_ns = 0;
        std::string candidate_fingerprint;
        if (!read_lmh_kernel_contract(
                policy_path, candidate_max_khz, candidate_timestamp_ns, candidate_fingerprint) &&
            !read_lmh_kernel_contract(
                cdev_path.string(), candidate_max_khz, candidate_timestamp_ns, candidate_fingerprint)) {
            continue;
        }
        if (candidate_max_khz < hardware_min_khz ||
            candidate_max_khz > hardware_max_khz ||
            candidate_timestamp_ns > now_ns ||
            now_ns - candidate_timestamp_ns > max_sample_age_ns_) return std::nullopt;

        if (!found_contract) {
            found_contract = true;
            selected_max_khz = candidate_max_khz;
            selected_fingerprint = std::move(candidate_fingerprint);
            selected_timestamp_ns = candidate_timestamp_ns;
        } else if (candidate_max_khz != selected_max_khz ||
                   candidate_timestamp_ns != selected_timestamp_ns ||
                   candidate_fingerprint != selected_fingerprint) {
            // Conflicting cooling-device authorities are ambiguous; fail closed.
            return std::nullopt;
        }
    }

    if (!found_contract || selected_max_khz == 0 ||
        selected_max_khz > std::numeric_limits<uint64_t>::max() / 1000ULL ||
        selected_timestamp_ns > std::numeric_limits<uint64_t>::max() - max_sample_age_ns_) {
        return std::nullopt;
    }

    return IndependentFrequencyCeiling{
        selected_max_khz * 1000ULL,
        selected_timestamp_ns + max_sample_age_ns_,
        selected_fingerprint,
        true
    };
}

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
    if (assessment.severity >= 4 || assessment.pressure >= 110.0) {
        admission.reason = "OEM severe/hot/critical thermal trip reached";
        return admission;
    }

    for (const auto& token : required_sensor_tokens_) {
        bool found = false;
        for (const auto& reading : assessment.readings) {
            if (!contains_case_insensitive(reading.type, token) &&
                !contains_case_insensitive(reading.id, token)) continue;
            if (!reading.valid || !reading.freshness_valid || !reading.policy_available ||
                !std::isfinite(reading.celsius) || reading.celsius < 0.0F ||
                reading.sample_timestamp_ns == 0 ||
                reading.sample_timestamp_ns > sampled_at ||
                sampled_at - reading.sample_timestamp_ns > max_age_ns_) {
                admission.reason =
                    "required thermal sensor invalid, stale, warming up, or lacks OEM trip policy: " + token;
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

std::optional<FrequencyCapabilitySnapshot>
TransactionalFrequencyActuator::discover_capability(
    uint32_t policy_index, uint64_t now_ns) const noexcept {
    if (policy_index > 255U || sys_root_.empty()) return std::nullopt;
    const uint64_t admission_now = monotonic_now_ns();
    if (admission_now == 0U || now_ns == 0U || admission_now < now_ns ||
        admission_now - now_ns > 250'000'000ULL) return std::nullopt;

    const auto thermal = thermal_.evaluate(admission_now);
    if (!thermal.granted || thermal.observed_at_ns == 0U ||
        thermal.observed_at_ns > admission_now ||
        admission_now - thermal.observed_at_ns > 250'000'000ULL) return std::nullopt;

    const std::string policy_path = sys_root_ +
        "/devices/system/cpu/cpufreq/policy" + std::to_string(policy_index);
    const auto ceiling = ceiling_provider_.current(policy_path, admission_now);
    if (!ceiling.has_value() || !ceiling->signature_or_kernel_contract_verified ||
        ceiling->max_hz == 0U || ceiling->valid_until_ns < admission_now ||
        ceiling->capability_fingerprint.empty()) return std::nullopt;

    uint64_t hardware_min_khz = 0U;
    if (!read_frequency_khz(policy_path + "/cpuinfo_min_freq", hardware_min_khz) ||
        hardware_min_khz > std::numeric_limits<uint64_t>::max() / 1000ULL) {
        return std::nullopt;
    }
    FrequencyDiscoveryRequest request{};
    request.policy_index = policy_index;
    request.active_constraint_min_hz = hardware_min_khz * 1000ULL;
    request.active_constraint_max_hz = ceiling->max_hz;
    request.active_constraints_valid = true;
    request.thermal_admission_valid = true;
    request.thermal_admission_granted = true;
    request.capability_fingerprint_valid = true;
    request.capability_fingerprint = ceiling->capability_fingerprint;

    FrequencyOppDiscovery discovery(sys_root_);
    auto capability = discovery.discover_cpu_policy(request);
    if (!capability.has_value() || !FrequencyTargetPlanner::validate(*capability) ||
        ceiling->max_hz > capability->hardware_max_hz) return std::nullopt;
    return capability;
}

bool TransactionalFrequencyActuator::can_apply_policy(
    uint32_t policy_index, double requested_capacity_scale,
    uint64_t now_ns) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_ || recovery_required_) return false;
    const auto capability = discover_capability(policy_index, now_ns);
    if (!capability.has_value()) return false;
    const std::string policy_path = sys_root_ +
        "/devices/system/cpu/cpufreq/policy" + std::to_string(policy_index);
    uint64_t current_khz = 0U;
    if (!read_frequency_khz(policy_path + "/scaling_cur_freq", current_khz) ||
        current_khz > std::numeric_limits<uint64_t>::max() / 1000ULL) return false;
    const auto target = planner_.select_target(
        *capability, current_khz * 1000ULL, requested_capacity_scale);
    return target.has_value() && *target > capability->observed_policy_max_hz;
}

bool TransactionalFrequencyActuator::apply_policy(
    uint32_t policy_index, double requested_capacity_scale,
    const OutcomeSample& baseline_sample, uint64_t now_ns) noexcept {
    const auto capability = discover_capability(policy_index, now_ns);
    if (!capability.has_value()) return false;
    const std::string policy_path = sys_root_ +
        "/devices/system/cpu/cpufreq/policy" + std::to_string(policy_index);
    uint64_t current_khz = 0U;
    if (!read_frequency_khz(policy_path + "/scaling_cur_freq", current_khz) ||
        current_khz > std::numeric_limits<uint64_t>::max() / 1000ULL) return false;
    return apply(policy_path, *capability, current_khz * 1000ULL,
                 requested_capacity_scale, baseline_sample, now_ns);
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
    if (!thermal.granted || thermal.observed_at_ns == 0U ||
        thermal.observed_at_ns > admission_now ||
        admission_now - thermal.observed_at_ns > 250'000'000ULL ||
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

    capability_ = capability;
    evaluation_complete_ = false;
    last_outcome_ = {};
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
    if (!active_ || recovery_required_) {
        return {OutcomeVerdict::Inconclusive, 0, "no active verified transaction"};
    }

    const uint64_t now = monotonic_now_ns();
    const auto thermal = thermal_.evaluate(now);
    const std::size_t separator = target_path_.rfind('/');
    const std::string policy_path = separator == std::string::npos
        ? std::string{} : target_path_.substr(0U, separator);
    const auto ceiling = policy_path.empty()
        ? std::optional<IndependentFrequencyCeiling>{}
        : ceiling_provider_.current(policy_path, now);
    const bool admission_live =
        thermal.granted && thermal.observed_at_ns != 0U &&
        thermal.observed_at_ns <= now && now - thermal.observed_at_ns <= 250'000'000ULL &&
        ceiling.has_value() && ceiling->signature_or_kernel_contract_verified &&
        ceiling->valid_until_ns >= now &&
        ceiling->capability_fingerprint == capability_.capability_fingerprint &&
        ceiling->max_hz >= target_hz_;
    if (!admission_live) {
        recovery_required_ = true;
        OutcomeResult denied{OutcomeVerdict::Inconclusive, 0,
            "thermal or independent LMh admission expired during active transaction"};
        if (!rollback_locked()) denied.reason += "; rollback verification failed, lease retained";
        else denied.reason += "; OEM baseline restored and verified";
        return denied;
    }

    if (evaluation_complete_) return last_outcome_;
    OutcomeResult result = evaluator_.observe(sample);
    if (result.verdict == OutcomeVerdict::Regressed ||
        result.verdict == OutcomeVerdict::Inconclusive ||
        result.verdict == OutcomeVerdict::Neutral) {
        recovery_required_ = true;
        if (!rollback_locked()) result.reason += "; rollback verification failed, lease retained";
        else result.reason += "; OEM baseline restored and verified";
    } else if (result.verdict == OutcomeVerdict::Improved) {
        evaluation_complete_ = true;
        last_outcome_ = result;
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
    capability_ = {};
    evaluation_complete_ = false;
    last_outcome_ = {};
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
