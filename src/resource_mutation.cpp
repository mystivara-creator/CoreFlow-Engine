#include "coreflow/resource_mutation.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <utility>

namespace coreflow {
namespace {

std::string trim(std::string value) {
    while (!value.empty() &&
           (value.back() == ' ' || value.back() == '\t' || value.back() == '\r' ||
            value.back() == '\n')) {
        value.pop_back();
    }
    std::size_t first = 0;
    while (first < value.size() &&
           (value[first] == ' ' || value[first] == '\t')) {
        ++first;
    }
    if (first != 0) value.erase(0, first);
    return value;
}

bool endsWith(const std::string& value, const char* suffix) noexcept {
    const std::size_t suffixLength = std::char_traits<char>::length(suffix);
    return value.size() >= suffixLength &&
           value.compare(value.size() - suffixLength, suffixLength, suffix) == 0;
}

bool isSchedulerResource(const std::string& name) noexcept {
    return name == "scheduler" || endsWith(name, ":scheduler") ||
           endsWith(name, ".scheduler");
}

bool activeSchedulerIs(const std::string& live, const std::string& token) {
    std::istringstream stream(live);
    std::string part;
    while (stream >> part) {
        const bool selected = part.size() >= 2 && part.front() == '[' &&
                              part.back() == ']';
        if (selected && part.substr(1, part.size() - 2) == token) return true;
    }
    return false;
}

bool isMutableDomain(ResourceDomain domain) noexcept {
    return domain == ResourceDomain::Memory ||
           domain == ResourceDomain::Io ||
           domain == ResourceDomain::Scheduler;
}

bool fallbackPolicyAuthorized(ResourceDomain domain, const std::string& name,
                              const std::string& path) noexcept {
    if (!ResourceActuator::mutationTargetAllowed(path)) return false;
    if (domain == ResourceDomain::Memory) {
        // Exact production name/path pairs. The fallback is only for legacy
        // DeviceProfile adapters; it must not re-authorize another sysctl.
        static constexpr std::pair<const char*, const char*> approved[] = {
            {"vm.swappiness", "/proc/sys/vm/swappiness"},
            {"vm.dirty_ratio", "/proc/sys/vm/dirty_ratio"},
            {"vm.dirty_background_ratio", "/proc/sys/vm/dirty_background_ratio"},
            {"vm.vfs_cache_pressure", "/proc/sys/vm/vfs_cache_pressure"},
            {"vm.min_free_kbytes", "/proc/sys/vm/min_free_kbytes"},
            {"vm.dirty_expire_centisecs", "/proc/sys/vm/dirty_expire_centisecs"},
            {"vm.dirty_writeback_centisecs", "/proc/sys/vm/dirty_writeback_centisecs"},
        };
        for (const auto& candidate : approved) {
            if (name == candidate.first && path == candidate.second) return true;
        }
#ifdef COREFLOW_HOST_TEST_FIXTURES
        // Synthetic profiles backed by /tmp are confined to host tests.
        static constexpr const char* names[] = {
            "vm.swappiness", "vm.dirty_ratio", "vm.dirty_background_ratio",
            "vm.vfs_cache_pressure", "vm.min_free_kbytes",
            "vm.dirty_expire_centisecs", "vm.dirty_writeback_centisecs"};
        if (path.rfind("/tmp/", 0) == 0) {
            for (const char* candidate : names) {
                if (name == candidate) return true;
            }
        }
#endif
        return false;
    }
#ifdef COREFLOW_HOST_TEST_FIXTURES
    // Synthetic I/O/scheduler fixtures model legacy tests only. No equivalent
    // production sysfs/procfs target is authorized by this compatibility path.
    return (domain == ResourceDomain::Io || domain == ResourceDomain::Scheduler) &&
           path.rfind("/tmp/", 0) == 0;
#else
    return false;
#endif
}

InterventionLevel requiredIntervention(ResourceDomain /*domain*/,
                                       const std::string& /*name*/) noexcept {
    return InterventionLevel::Low;
}

bool interventionAllows(InterventionLevel have, InterventionLevel need) noexcept {
    return static_cast<std::uint8_t>(have) >= static_cast<std::uint8_t>(need);
}

} // namespace

void ResourceMutationController::setJournal(MutationJournal* journal) noexcept {
    journal_ = journal;
}

void ResourceMutationController::setExperienceMemory(
    const ExperienceMemory* memory) noexcept {
    experience_memory_ = memory;
}

bool ResourceMutationController::recoverIfNeeded() noexcept {
    if (journal_ == nullptr) return true;
    MutationJournal::Entries entries;
    const MutationJournal::LoadState state = journal_->load(entries);
    switch (state) {
        case MutationJournal::LoadState::Absent:
            recovery_pending_ = false;
            return true;
        case MutationJournal::LoadState::Corrupt:
            return false;
        case MutationJournal::LoadState::Pending:
            break;
    }

    bool restored = true;
    for (const auto& entry : entries) {
        if (!std::filesystem::exists(entry.first)) continue;
        const ActuatorResult result =
            actuator_.restore(entry.first, entry.second);
        if (!(result.status == ActuatorStatus::RolledBack ||
              result.status == ActuatorStatus::NoChange ||
              result.status == ActuatorStatus::Ok)) {
            restored = false;
        }
    }
    if (!restored) {
        mutated_ = true;
        restore_failed_ = true;
        return false;
    }
    if (!journal_->clear()) return false;
    recovery_pending_ = false;
    journal_committed_ = false;
    mutated_ = false;
    restore_failed_ = false;
    return true;
}

bool ResourceMutationController::captureBaseline(
    const DeviceProfile& profile
) noexcept {
    if (!recoverIfNeeded() || mutated_) return false;

    baseline_.clear();
    dirty_resources_.clear();
    last_applied_resources_.clear();

    for (const auto& cap : profile.capabilities.resources) {
        if (!isMutableDomain(cap.domain)) continue;
        if (!cap.mutation_ready || !cap.policy_authorized ||
            !cap.readable || !cap.writable) continue;
        if (cap.path.empty() ||
            !fallbackPolicyAuthorized(cap.domain, cap.name, cap.path)) continue;

        std::string value;
        if (!ResourceActuator::read(cap.path, value)) continue;
        value = ResourceActuator::comparableValue(value);
        if (value.empty()) continue;

        baseline_[cap.path] = {cap.domain, cap.name, cap.path, value, true};
    }

    auto captureField = [this](ResourceDomain domain, const std::string& name,
                               const std::string& path, bool readable,
                               bool writable) {
        if (path.empty() || !readable || !writable ||
            !ResourceActuator::mutationTargetAllowed(path) ||
            !fallbackPolicyAuthorized(domain, name, path)) return;
        if (baseline_.count(path)) return;
        std::string value;
        if (!ResourceActuator::read(path, value)) return;
        baseline_[path] = {domain, name, path,
                           ResourceActuator::comparableValue(value), true};
    };

    captureField(ResourceDomain::Memory, "vm.swappiness",
                 profile.vm_swappiness.path, profile.vm_swappiness.readable,
                 profile.vm_swappiness.writable);
    for (const auto& device : profile.io_devices) {
        captureField(ResourceDomain::Io, device.name + ":read_ahead_kb",
                     device.path + "/queue/read_ahead_kb",
                     device.read_ahead_readable, device.read_ahead_writable);
        captureField(ResourceDomain::Io, device.name + ":nr_requests",
                     device.path + "/queue/nr_requests",
                     device.nr_requests_readable, device.nr_requests_writable);
        captureField(ResourceDomain::Io, device.name + ":scheduler",
                     device.path + "/queue/scheduler",
                     device.scheduler_readable, device.scheduler_writable);
    }

    baseline_captured_ = !baseline_.empty();
    return baseline_captured_;
}

bool ResourceMutationController::verifySchedulerToken(
    const std::string& path,
    const std::string& token,
    std::string& resolved
) const noexcept {
    std::string live;
    if (!ResourceActuator::read(path, live)) return false;
    live = trim(live);
    std::istringstream stream(live);
    std::string part;
    bool found = false;
    while (stream >> part) {
        if (!part.empty() && part.front() == '[' && part.back() == ']') {
            part = part.substr(1, part.size() - 2);
        }
        if (part == token) {
            found = true;
            break;
        }
    }
    if (!found) return false;
    resolved = token;
    return true;
}

bool ResourceMutationController::selectCandidate(
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const PolicyPlan& policy,
    Candidate& out
) const noexcept {
    if (!baseline_captured_ || baseline_.empty()) return false;
    if (!policy.mutation_eligible || policy.candidates.empty()) return false;
    if (policy.action != PolicyAction::Candidate) {
        // ReduceIntervention means stop/recover, never start a new resource write.
        return false;
    }
    // Thermal/memory safety states accept only a stabilizing plan; anything else
    // is hold/recovery-only.
    if ((state == RuntimeState::ThermalGuard || state == RuntimeState::Pressure) &&
        !policy.stabilizing_only) {
        return false;
    }

    EffectContext ctx;
    ctx.state = state;
    ctx.thermal_c = sample.thermal_available
                        ? static_cast<double>(sample.thermal_millidegrees) / 1000.0
                        : 0.0;
    ctx.thermal_available = sample.thermal_available;
    ctx.mem_available_ratio = sample.mem_available_ratio;
    ctx.cpu_utilization = sample.cpu_utilization;
    ctx.cpu_utilization_available = sample.cpu_utilization_available;
    ctx.load1 = sample.load1;
    ctx.io_activity_available = sample.io_activity_available;
    ctx.io_read_kb_per_sec = sample.io_read_kb_per_sec;
    ctx.io_write_kb_per_sec = sample.io_write_kb_per_sec;
    ctx.charging = sample.charging;
    ctx.sample_confidence = sample.confidence;
    ctx.intervention = policy.intervention;

    if (policy.action == PolicyAction::ReduceIntervention) {
        ctx.intervention = InterventionLevel::Low;
    }

    std::unordered_map<std::string, std::string> baseline_values;
    baseline_values.reserve(baseline_.size());
    for (const auto& item : baseline_) {
        if (item.second.valid) {
            baseline_values[item.first] = item.second.value;
        }
    }

    // Discovery remains authoritative. The baseline map also contains legacy
    // DeviceProfile adapters (VM and block-queue fields) which predate the
    // unified matrix; expose those as capabilities only when Discovery omitted
    // the same path, and only because captureBaseline already verified both
    // readability and writability for that adapter.
    EnvironmentCapabilityMatrix effective_matrix = profile.capabilities;
    for (const auto& item : baseline_) {
        if (!item.second.valid) continue;
        const bool present = std::any_of(
            effective_matrix.resources.begin(), effective_matrix.resources.end(),
            [&item](const ResourceCapability& capability) {
                return capability.path == item.first;
            });
        if (present) continue;
        // Legacy/manual profiles may not carry the discovery matrix. Never
        // synthesize authorization for a production sysfs/procfs target.
        if (!fallbackPolicyAuthorized(item.second.domain, item.second.name, item.first)) {
            continue;
        }
        ResourceCapability capability;
        capability.domain = item.second.domain;
        capability.name = item.second.name;
        capability.path = item.second.path;
        capability.exists = true;
        capability.readable = true;
        capability.writable = true;
        capability.permission_granted = true;
        capability.policy_authorized = true;
        capability.runtime_verified = true;
        capability.mutation_ready = true;
        effective_matrix.resources.push_back(std::move(capability));
    }

    // The DecisionAgent is the proposal layer for resource candidates. It only
    // ranks observed capabilities; this controller still independently enforces
    // policy, intervention level, experience confidence, rejection cooldown,
    // MutationAuthority, journaling, verification and rollback before any write.
    AgentObservation agent_observation;
    agent_observation.effect_ctx = ctx;
    agent_observation.matrix = &effective_matrix;
    agent_observation.baselines = &baseline_values;
    agent_observation.state = state;
    agent_observation.sample_confidence = sample.confidence;
    agent_observation.mutation_allowed_by_context = !policy.stabilizing_only;
    agent_observation.stabilizing_allowed_by_context = policy.stabilizing_only;
    agent_observation.max_candidates = 8;
    const AgentProposal proposal = decision_agent_.reason(agent_observation);
    if (!proposal.valid) return false;
    const auto& ranked = proposal.ranked;

    ExperienceMemory::Context experience_context;
    experience_context.state = state;
    const bool power_headroom =
        !sample.charging_telemetry_available || sample.battery_level_percent < 0 ||
        sample.battery_level_percent >= 20;
    const double io_rate = sample.io_read_kb_per_sec + sample.io_write_kb_per_sec;
    if (state == RuntimeState::ThermalGuard) {
        experience_context.workload = WorkloadClass::ThermalLimited;
    } else if (!power_headroom) {
        experience_context.workload = WorkloadClass::PowerConstrained;
    } else if (state == RuntimeState::Pressure) {
        experience_context.workload = WorkloadClass::MemoryBound;
    } else if (sample.io_activity_available && io_rate >= 4096.0 &&
               (!sample.cpu_utilization_available || sample.cpu_utilization < 0.65)) {
        experience_context.workload = WorkloadClass::IoBound;
    } else if (sample.mem_available_ratio > 0.0 && sample.mem_available_ratio < 0.20) {
        experience_context.workload = WorkloadClass::MemoryBound;
    } else if (sample.load1 < 0.20 &&
               (!sample.cpu_utilization_available || sample.cpu_utilization < 0.15)) {
        experience_context.workload = WorkloadClass::Idle;
    } else if (sample.cpu_utilization_available && sample.cpu_utilization >= 0.80) {
        experience_context.workload = WorkloadClass::CpuBound;
    } else if (sample.thermal_trend == Trend::Rising && sample.cpu_utilization >= 0.65) {
        experience_context.workload = WorkloadClass::Sustained;
    } else {
        experience_context.workload = WorkloadClass::Interactive;
    }
    experience_context.charging = sample.charging;
    experience_context.thermal_trend = sample.thermal_trend;
    experience_context.memory_trend = sample.memory_trend;
    experience_context.load_trend = sample.load_trend;

    double best_utility = -1.0;
    Candidate best_candidate;
    bool found_candidate = false;

    for (const auto& scored : ranked) {
        const std::string& name = scored.resource_name;
        const std::string& path = scored.path;
        if (name.empty() || path.empty()) continue;

        auto bit = baseline_.find(path);
        if (bit == baseline_.end() || !bit->second.valid ||
            !fallbackPolicyAuthorized(bit->second.domain, name, path)) continue;

        if (!policy.allows(bit->second.domain, name)) continue;
        // A stressed context accepts only changes that respond to the stress by
        // reducing load. Everything else waits for headroom.
        if (policy.stabilizing_only && !scored.stabilizing) continue;

        if (!interventionAllows(policy.intervention,
                                requiredIntervention(bit->second.domain, name))) {
            continue;
        }

        std::string requested = scored.requested;
        if (isSchedulerResource(name)) {
            std::string resolved;
            if (!verifySchedulerToken(path, requested, resolved)) continue;

            std::string live;
            if (!ResourceActuator::read(path, live)) continue;
            live = trim(live);
            if (activeSchedulerIs(live, resolved)) {
                // A preference is not a mandate. If it is already active,
                // consider a known, advertised alternative; never guess one.
                std::string alternative;
                if (!verifySchedulerToken(path, "none", alternative) ||
                    activeSchedulerIs(live, alternative)) {
                    continue;
                }
                resolved = alternative;
            }
            requested = resolved;
        } else {
            if (requested.empty() || requested == bit->second.value) continue;
        }

        if (isRejected(path, requested)) continue;

        double utility = scored.utility;
        if (experience_memory_ != nullptr) {
            ExperienceMemory::CandidateIdentity identity;
            identity.key = "resource:" + path + ":" + requested;
            if (const auto* record = experience_memory_->find(identity, experience_context);
                record != nullptr && record->observations > 0 &&
                record->confidence >= 0.60) {
                switch (record->outcome) {
                    case BaselineIntelligence::Outcome::Beneficial:
                        utility += 0.08 * record->confidence;
                        break;
                    case BaselineIntelligence::Outcome::Neutral:
                        utility -= 0.05 * record->confidence;
                        break;
                    case BaselineIntelligence::Outcome::Regression:
                        utility -= 0.30 * record->confidence;
                        break;
                    case BaselineIntelligence::Outcome::Unknown:
                    default:
                        break;
                }
            }
        }
        if (utility < 0.05 || utility <= best_utility) continue;

        best_utility = utility;
        best_candidate.domain = bit->second.domain;
        best_candidate.name = name;
        best_candidate.path = path;
        best_candidate.requested = requested;
        best_candidate.baseline = bit->second.value;
        best_candidate.benefit = scored.benefit;
        best_candidate.risk = scored.risk;
        best_candidate.reason = scored.reason;
        found_candidate = true;
    }

    if (found_candidate) {
        out = std::move(best_candidate);
        return true;
    }
    return false;
}

MutationJournal::Entries
ResourceMutationController::factorySnapshot() const {
    MutationJournal::Entries entries;
    entries.reserve(dirty_resources_.size() + 1);
    for (const auto& path : dirty_resources_) {
        const auto it = baseline_.find(path);
        if (it != baseline_.end() && it->second.valid) {
            entries.emplace_back(path, it->second.value);
        }
    }
    return entries;
}

MutationResult ResourceMutationController::applyCandidate(
    const Candidate& candidate,
    const MutationPermit& permit
) noexcept {
    if (journal_ == nullptr) return MutationResult::Failed;
    if (!permit.validFor(MutationPermit::Scope::Resource)) return MutationResult::Failed;
    if (!fallbackPolicyAuthorized(candidate.domain, candidate.name, candidate.path)) {
        return MutationResult::Skipped;
    }

    // Durability before the write. The journal holds factory values for every
    // resource this controller has changed, plus the candidate about to change.
    MutationJournal::Entries entries = factorySnapshot();
    if (dirty_resources_.find(candidate.path) == dirty_resources_.end()) {
        const auto it = baseline_.find(candidate.path);
        if (it != baseline_.end() && it->second.valid) {
            entries.emplace_back(candidate.path, it->second.value);
        }
    }
    if (entries.empty() || !journal_->commit(entries)) return MutationResult::Failed;
    journal_committed_ = true;

    ActuatorMutation mutation;
    mutation.id = {ActuatorDomain::Io, 0};
    if (candidate.domain == ResourceDomain::Memory) {
        mutation.id.domain = ActuatorDomain::Vm;
    }
    mutation.target = candidate.path;
    mutation.requested = candidate.requested;

    const ActuatorResult result = actuator_.apply(mutation, permit);

    // Any attempted write may have landed, even if verification failed, so the
    // path is tracked as dirty until an explicit rollback clears it.
    if (result.writes_attempted != 0 && result.status != ActuatorStatus::RolledBack) {
        mutated_ = true;
        dirty_resources_.insert(candidate.path);
    }

    if (result.changed()) {
        last_applied_resources_.clear();
        last_applied_resources_.emplace_back(candidate.path, candidate.requested);
        return MutationResult::Verified;
    }

    // No write landed. Drop a journal epoch that this attempt opened, but only
    // when no other resource is still changed: the journal must keep the factory
    // values of anything that is still dirty.
    if (result.writes_attempted == 0 && !mutated_ && journal_committed_) {
        if (journal_->clear()) {
            journal_committed_ = false;
        } else {
            restore_failed_ = true;
            return MutationResult::Failed;
        }
    }

    // NoChange means the kernel already holds the requested value. Nothing was
    // applied, so report Skipped rather than a verified mutation.
    if (result.status == ActuatorStatus::NoChange) return MutationResult::Skipped;
    return MutationResult::Failed;
}

MutationResult ResourceMutationController::apply(
    const RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const EngineConfig& config,
    const PolicyPlan& policy,
    const MutationPermit& permit
) noexcept {
    ++cycle_;
    if (!recoverIfNeeded()) return MutationResult::Failed;
    if (!baseline_captured_ && !captureBaseline(profile)) {
        return MutationResult::Skipped;
    }
    if (restore_failed_) {
        return restoreAll() ? MutationResult::RolledBack : MutationResult::Failed;
    }

    const bool mutation_gate_open =
        permit.validFor(MutationPermit::Scope::Resource) &&
        config.mutationMode() == MutationMode::Adaptive &&
        config.mutationArmed() &&
        policy.mutation_eligible &&
        policy.intervention != InterventionLevel::ObserveOnly &&
        sample.confidence >= config.minConfidence();

    if (!mutation_gate_open) {
        if (!mutated_) return MutationResult::Skipped;
        return restoreAll() ? MutationResult::RolledBack : MutationResult::Failed;
    }

    Candidate candidate;
    if (!selectCandidate(state, sample, profile, policy, candidate)) {
        return MutationResult::Skipped;
    }

    const bool was_clean = !mutated_;
    const MutationResult applied = applyCandidate(candidate, permit);
    if (applied == MutationResult::Verified) {
        stabilizing_epoch_ = was_clean ? policy.stabilizing_only
                                       : (stabilizing_epoch_ && policy.stabilizing_only);
        return MutationResult::Verified;
    }
    if (applied == MutationResult::Skipped) return MutationResult::Skipped;
    if (mutated_ || restore_failed_) {
        return restoreAll() ? MutationResult::RolledBack : MutationResult::Failed;
    }
    return MutationResult::Failed;
}

MutationResult ResourceMutationController::apply(
    const RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const EngineConfig& config
) noexcept {
    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.mutation_eligible = true;
    plan.intervention = InterventionLevel::Low;
    plan.reason = "permissive test plan";

    for (const auto& cap : profile.capabilities.resources) {
        if (!cap.mutation_ready) continue;
        if (!isMutableDomain(cap.domain)) continue;
        plan.candidates.push_back({cap.domain, cap.name, {},
                                   sample.confidence, 0.5,
                                   PolicyAction::Candidate});
    }

    const MutationAuthority authority;
    const MutationPermit permit = authority.authorize(
        plan, config, state, sample.confidence, MutationPermit::Scope::Resource);
    return apply(state, sample, profile, config, plan, permit);
}

bool ResourceMutationController::restoreAll() noexcept {
    if (!mutated_ && !restore_failed_) return true;

    bool ok = true;
    for (const auto& path : dirty_resources_) {
        const auto item = baseline_.find(path);
        if (item == baseline_.end() || !item->second.valid ||
            !std::filesystem::exists(path)) {
            continue;
        }
        const ActuatorResult result =
            actuator_.restore(path, item->second.value);
        if (!(result.status == ActuatorStatus::RolledBack ||
              result.status == ActuatorStatus::NoChange ||
              result.status == ActuatorStatus::Ok)) {
            ok = false;
        }
    }
    if (!ok) {
        restore_failed_ = true;
        mutated_ = true;
        return false;
    }
    if (journal_committed_ && journal_ != nullptr && !journal_->clear()) {
        restore_failed_ = true;
        mutated_ = true;
        return false;
    }
    journal_committed_ = false;
    restore_failed_ = false;
    mutated_ = false;
    dirty_resources_.clear();
    last_applied_resources_.clear();
    stabilizing_epoch_ = false;
    return true;
}

bool ResourceMutationController::isRejected(
    const std::string& path,
    const std::string& requested
) const noexcept {
    try {
        const auto it = rejected_.find(path + "\n" + requested);
        return it != rejected_.end() &&
               cycle_ - it->second < kRejectionCooldownCycles;
    } catch (...) {
        return true;  // fail closed: treat an unreadable record as rejected
    }
}

void ResourceMutationController::rejectLastMutation() noexcept {
    try {
        for (const auto& applied : last_applied_resources_) {
            rejected_[applied.first + "\n" + applied.second] = cycle_;
        }
    } catch (...) {
    }
    last_applied_resources_.clear();
}

bool ResourceMutationController::syncPolicyModel(
    ResourceStateModel& model
) const noexcept {
    bool any = false;
    for (const auto& item : baseline_) {
        if (!item.second.valid) continue;
        ResourceState* state =
            model.find(item.second.domain, item.second.name);
        if (state == nullptr) continue;
        if (!model.setBaseline(item.second.domain, item.second.name,
                               item.second.value)) {
            continue;
        }
        (void)model.setDesired(item.second.domain, item.second.name,
                               item.second.value);
        any = model.setMutationPermitted(item.second.domain, item.second.name,
                                         true) ||
              any;
    }
    return any;
}

} // namespace coreflow
