#include "coreflow/intent_actuator.hpp"
#include "coreflow/mutation/intent_actuator.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace {
bool parse_number(std::string_view text, double& value) noexcept {
    if (text.empty() || text.size() > 64U) return false;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size() &&
           std::isfinite(value);
}
bool parse_bounds(std::string_view contract, double& minimum, double& maximum) noexcept {
    constexpr std::string_view separator = "<=value<=";
    const size_t at = contract.find(separator);
    return at != std::string_view::npos &&
           parse_number(contract.substr(0, at), minimum) &&
           parse_number(contract.substr(at + separator.size()), maximum) &&
           minimum <= maximum;
}
bool read_scalar(const std::string& path, std::string& value) noexcept {
    value.clear();
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) { (void)::close(fd); return false; }
    std::array<char, 128> buffer{};
    const ssize_t n = ::read(fd, buffer.data(), buffer.size());
    (void)::close(fd);
    if (n <= 0 || static_cast<size_t>(n) == buffer.size()) return false;
    value.assign(buffer.data(), static_cast<size_t>(n));
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r' || value.back() == ' '))
        value.pop_back();
    return !value.empty();
}
} // namespace

IntentActuator::IntentActuator(CentralMutationCoordinator& coordinator,
                               BaselineStore& baseline_store,
                               std::string proc_root)
    : coordinator_(coordinator), baseline_store_(baseline_store),
      proc_root_(std::move(proc_root)),
      controller_id_(CentralMutationCoordinator::make_controller_identity("IntentActuator")) {}

IntentActuator::~IntentActuator() {
    std::lock_guard<std::mutex> lock(mutex_);
    // Retained is terminal: destructor must never roll back a committed result.
    // Only an actively observed transaction is eligible for destructor rollback.
    if (state_.state == IntentActuatorState::Observing) {
        (void)finish_rollback_locked("IntentActuator teardown");
    }
    coordinator_.unregister_preemption_handler(controller_id_);
}

uint64_t IntentActuator::monotonic_now_ns() noexcept {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

double IntentActuator::selected_psi(const InfrastructurePsiSample& sample) const noexcept {
    return memory_domain_ ? sample.memory_some_avg10 : sample.io_some_avg10;
}

bool IntentActuator::begin(DeviceProfileHandle profile, std::string_view semantic_id,
                            std::string_view target_value,
                            const InfrastructurePsiSample& admission) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!profile || state_.state == IntentActuatorState::Observing ||
        state_.state == IntentActuatorState::OwnershipQuarantined ||
        state_.state == IntentActuatorState::RecoveryRequired) return false;

    // A retained/rolled-back/rejected transaction is complete. Reuse this same
    // actuator instance by clearing only transaction-local state before admission.
    state_ = {};
    baseline_value_.clear();
    target_value_.clear();
    observations_.clear();
    started_ns_ = 0U;
    before_psi_ = 0.0;
    const uint64_t now = monotonic_now_ns();
    if (profile->captured_monotonic_ns == 0U || now < profile->captured_monotonic_ns ||
        now - profile->captured_monotonic_ns > 5'000'000'000ULL ||
        admission.monotonic_ns == 0U || now < admission.monotonic_ns ||
        now - admission.monotonic_ns > 2'000'000'000ULL) {
        state_ = {.state=IntentActuatorState::Rejected,.reason="stale profile or PSI admission"};
        return false;
    }
    const auto it = std::find_if(profile->node_capabilities.begin(), profile->node_capabilities.end(),
        [&](const NodeCapability& cap) { return cap.semantic_id == semantic_id; });
    if (it == profile->node_capabilities.end() || !it->mutation_eligible()) {
        state_ = {.state=IntentActuatorState::Rejected,.semantic_id=std::string(semantic_id),
                  .reason="node lacks verified writable semantic/range contract"};
        return false;
    }
    memory_domain_ = it->domain == HardwareDomain::Memory;
    if (!memory_domain_ && it->domain != HardwareDomain::Storage) {
        state_ = {.state=IntentActuatorState::Rejected,.semantic_id=std::string(semantic_id),
                  .reason="IntentActuator accepts only memory and storage domains"};
        return false;
    }
    if ((memory_domain_ && !admission.memory_valid) || (!memory_domain_ && !admission.io_valid)) {
        state_ = {.state=IntentActuatorState::Rejected,.semantic_id=std::string(semantic_id),
                  .reason="required PSI domain is unavailable"};
        return false;
    }
    const double psi = selected_psi(admission);
    if (!std::isfinite(psi) || psi < 1.0 || psi > 100.0) {
        state_ = {.state=IntentActuatorState::Rejected,.semantic_id=std::string(semantic_id),
                  .reason="PSI bottleneck admission below 1.0% some avg10"};
        return false;
    }
    double minimum=0.0, maximum=0.0, target=0.0, baseline=0.0;
    if (!parse_bounds(it->constraints, minimum, maximum) ||
        !parse_number(target_value, target) || target < minimum || target > maximum ||
        !parse_number(it->current_value, baseline) || baseline < minimum || baseline > maximum ||
        std::abs(target - baseline) < 0.000001) {
        state_ = {.state=IntentActuatorState::Rejected,.semantic_id=std::string(semantic_id),
                  .reason="target or baseline violates the profiled numeric contract"};
        return false;
    }
    domain_ = memory_domain_ ? MutationDomain::VmMemory : MutationDomain::BlockIo;
    const uint64_t sequence = now;
    const std::string tx = std::string(semantic_id) + ":" + std::to_string(sequence);
    if (!coordinator_.acquire_lease(domain_, controller_id_, tx,
                                    ControllerPriority::PressureGuard)) {
        state_ = {.state=IntentActuatorState::Rejected,.semantic_id=std::string(semantic_id),
                  .reason="mutation domain lease unavailable"};
        return false;
    }
    state_.path = it->path;
    state_.semantic_id = std::string(semantic_id);
    state_.transaction_id = tx;
    if (!baseline_store_.capture(it->path) ||
        !baseline_store_.apply_mutation(it->path, std::string(target_value))) {
        const bool restored = baseline_store_.rollback(it->path);
        coordinator_.release_lease(domain_, controller_id_, tx);
        state_ = {.state=restored ? IntentActuatorState::RolledBack : IntentActuatorState::RecoveryRequired,
                  .semantic_id=std::string(semantic_id),.path=it->path,.transaction_id=tx,
                  .reason="baseline capture or transactional write failed"};
        return false;
    }
    std::string readback;
    if (!read_value_locked(readback) || readback != target_value ||
        !coordinator_.lease_is_live(domain_, controller_id_, tx)) {
        const bool restored = baseline_store_.rollback(it->path);
        coordinator_.release_lease(domain_, controller_id_, tx);
        state_ = {.state=restored ? IntentActuatorState::RolledBack : IntentActuatorState::RecoveryRequired,
                  .semantic_id=std::string(semantic_id),.path=it->path,.transaction_id=tx,
                  .reason="read-back verification or lease check failed"};
        return false;
    }
    state_ = {.state=IntentActuatorState::Observing,.semantic_id=std::string(semantic_id),
              .path=it->path,.transaction_id=tx,.reason="mutation applied; awaiting empirical PSI outcome",
              .before_psi=psi};
    baseline_value_ = it->current_value;
    target_value_ = std::string(target_value);
    before_psi_ = psi;
    started_ns_ = now;
    observations_.clear();
    return true;
}

bool IntentActuator::read_value_locked(std::string& value) const noexcept {
    return !state_.path.empty() && read_scalar(state_.path, value);
}

IntentActuatorSnapshot IntentActuator::observe(const InfrastructurePsiSample& sample) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_.state != IntentActuatorState::Observing) return state_;
    if (!coordinator_.lease_is_live(domain_, controller_id_, state_.transaction_id)) {
        (void)coordinator_.mark_external_ownership_lost(domain_, controller_id_, state_.transaction_id);
        state_.state = IntentActuatorState::OwnershipQuarantined;
        state_.reason = "lease ownership lost; transaction quarantined pending verified recovery";
        return state_;
    }
    std::string actual;
    if (!read_value_locked(actual) || actual != target_value_) {
        (void)coordinator_.mark_external_ownership_lost(domain_, controller_id_, state_.transaction_id);
        state_.state = IntentActuatorState::OwnershipQuarantined;
        state_.reason = "out-of-band node change detected; no blind overwrite attempted";
        return state_;
    }
    const bool valid = memory_domain_ ? sample.memory_valid : sample.io_valid;
    const double psi = selected_psi(sample);
    if (!valid || !std::isfinite(psi) || psi < 0.0 || psi > 100.0 ||
        sample.monotonic_ns <= started_ns_ ||
        (!observations_.empty() && sample.monotonic_ns <= state_.elapsed_ns + started_ns_)) {
        state_.reason = "invalid, out-of-range, or non-monotonic PSI sample";
        return state_;
    }
    observations_.push_back(psi);
    const uint64_t elapsed = sample.monotonic_ns - started_ns_;
    state_.observation_samples = static_cast<uint32_t>(observations_.size());
    state_.elapsed_ns = elapsed;
    if (elapsed > 5'000'000'000ULL) {
        (void)finish_rollback_locked("outcome window exceeded 5 seconds without sufficient evidence");
        return state_;
    }
    if (elapsed < 3'000'000'000ULL || observations_.size() < 3U) return state_;
    double total = 0.0;
    for (const double value : observations_) total += value;
    const double average = total / static_cast<double>(observations_.size());
    state_.after_psi = average;
    const double required_gain = std::max(0.25, before_psi_ * 0.05);
    if (average <= before_psi_ - required_gain) {
        const auto retained = mutation::RetainedLifecycle::commit(
            baseline_store_, coordinator_, domain_, controller_id_,
            state_.transaction_id, state_.path, target_value_);
        if (retained != mutation::RetentionResult::Retained) {
            state_.state = IntentActuatorState::RecoveryRequired;
            state_.reason = retained == mutation::RetentionResult::PersistenceFailed
                ? "performance baseline persistence failed; lease retained for fail-safe recovery"
                : "retained record persisted but lease release requires recovery";
            return state_;
        }
        state_.state = IntentActuatorState::Retained;
        state_.reason = "verified performance baseline persisted; transaction lease released";
        return state_;
    }
    (void)finish_rollback_locked("PSI did not improve or regressed; rollback-by-default");
    return state_;
}

bool IntentActuator::finish_rollback_locked(std::string_view reason) {
    if (state_.path.empty() || state_.transaction_id.empty()) return false;
    std::string current;
    if (!read_value_locked(current) || current != target_value_) {
        (void)coordinator_.mark_external_ownership_lost(domain_, controller_id_, state_.transaction_id);
        state_.state = IntentActuatorState::OwnershipQuarantined;
        state_.reason = "rollback withheld because external ownership changed";
        return false;
    }
    const bool restored = baseline_store_.rollback(state_.path);
    if (restored) {
        coordinator_.release_lease(domain_, controller_id_, state_.transaction_id);
        state_.state = IntentActuatorState::RolledBack;
        state_.reason = std::string(reason);
        return true;
    }
    state_.state = IntentActuatorState::RecoveryRequired;
    state_.reason = "rollback failed; journal and lease require recovery";
    return false;
}

bool IntentActuator::rollback(std::string_view reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_.state != IntentActuatorState::Observing) return false;
    return finish_rollback_locked(reason);
}

IntentActuatorSnapshot IntentActuator::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

} // namespace coreflow
