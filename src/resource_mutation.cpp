#include "coreflow/resource_mutation.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>

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
        if (!cap.mutation_ready || !cap.readable || !cap.writable) continue;
        if (cap.path.empty()) continue;

        std::string value;
        if (!ResourceActuator::read(cap.path, value)) continue;
        value = trim(value);
        if (value.empty()) continue;

        baseline_[cap.path] = {cap.domain, cap.name, cap.path, value, true};
    }

    auto captureField = [this](ResourceDomain domain, const std::string& name,
                               const std::string& path, bool readable,
                               bool writable) {
        if (path.empty() || !readable || !writable) return;
        if (baseline_.count(path)) return;
        std::string value;
        if (!ResourceActuator::read(path, value)) return;
        baseline_[path] = {domain, name, path, trim(value), true};
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
    if (state == RuntimeState::ThermalGuard || state == RuntimeState::Pressure) {
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
        ResourceCapability capability;
        capability.domain = item.second.domain;
        capability.name = item.second.name;
        capability.path = item.second.path;
        capability.exists = true;
        capability.readable = true;
        capability.writable = true;
        capability.permission_granted = true;
        capability.runtime_verified = true;
        capability.mutation_ready = true;
        effective_matrix.resources.push_back(std::move(capability));
    }

    const auto ranked =
        effect_model_.rank(effective_matrix, baseline_values, ctx, 8);

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
        if (bit == baseline_.end() || !bit->second.valid) continue;

        if (!policy.allows(bit->second.domain, name)) continue;

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

bool ResourceMutationController::applyCandidate(
    const Candidate& candidate,
    const MutationPermit& permit
) noexcept {
    if (journal_ == nullptr) return false;
    if (!permit.validFor(MutationPermit::Scope::Resource)) return false;

    MutationJournal::Entries entries = factorySnapshot();
    if (dirty_resources_.find(candidate.path) == dirty_resources_.end()) {
        const auto it = baseline_.find(candidate.path);
        if (it != baseline_.end() && it->second.valid) {
            entries.emplace_back(candidate.path, it->second.value);
        }
    }
    if (entries.empty() || !journal_->commit(entries)) return false;
    journal_committed_ = true;

    ActuatorMutation mutation;
    mutation.id = {ActuatorDomain::Io, 0};
    if (candidate.domain == ResourceDomain::Memory) {
        mutation.id.domain = ActuatorDomain::Vm;
    }
    mutation.target = candidate.path;
    mutation.requested = candidate.requested;

    const ActuatorResult result = actuator_.apply(mutation, permit);
    if (result.writes_attempted != 0 &&
        result.status != ActuatorStatus::RolledBack) {
        mutated_ = true;
        dirty_resources_.insert(candidate.path);
    } else if (!result.succeeded() && result.writes_attempted == 0) {
        // No kernel write was attempted; do not leave a stale pending journal.
        if (journal_->clear()) {
            journal_committed_ = false;
        } else {
            restore_failed_ = true;
        }
    }
    if (result.changed()) {
        last_applied_resources_.clear();
        last_applied_resources_.emplace_back(candidate.path, candidate.requested);
    }
    return result.succeeded() &&
           (result.status == ActuatorStatus::NoChange || result.changed());
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

    if (!applyCandidate(candidate, permit)) {
        if (mutated_ || restore_failed_) {
            return restoreAll() ? MutationResult::RolledBack : MutationResult::Failed;
        }
        return MutationResult::Failed;
    }
    return MutationResult::Verified;
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
    return true;
}

void ResourceMutationController::rejectLastMutation() noexcept {
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
