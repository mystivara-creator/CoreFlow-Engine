#include "coreflow/resource_mutation.hpp"
#include "coreflow/context.hpp"
#include "coreflow/tunable_discovery.hpp"

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

bool isMutableDomain(ResourceDomain domain) noexcept {
    return isTunableResourceDomain(domain);
}

bool fallbackPolicyAuthorized(ResourceDomain domain, const std::string& name,
                              const std::string& path) noexcept {
    // Capability-driven: the same rule discovery applies. Only fatal surfaces
    // (actuator gate) and non-tunable/governor-owned nodes are refused.
    if (!ResourceActuator::mutationTargetAllowed(path)) return false;
    return classifySurface(domain, name, path) == SurfaceClass::Admissible;
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

void ResourceMutationController::setRetainedStateStore(RetainedStateStore* store) noexcept {
    retained_store_ = store;
    retained_entries_.clear();
    retained_active_resources_.clear();
    active_resources_cache_.clear();
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
            last_failure_stage_ = ResourceMutationFailureStage::JournalCorrupt;
            return false;
        case MutationJournal::LoadState::Pending:
            break;
    }

    bool restored = true;
    for (const auto& entry : entries) {
        // Do not silently skip missing paths: restore() reports the read
        // failure, and the pending journal remains available for diagnosis.
        const ActuatorResult result =
            actuator_.restore(entry.first, entry.second);
        const bool entry_restored =
            result.status == ActuatorStatus::RolledBack ||
            result.status == ActuatorStatus::NoChange ||
            result.status == ActuatorStatus::Ok;

        // Preserve the first failure status while still attempting the
        // remaining entries, so one failed resource cannot be masked.
        if (restored || !entry_restored) {
            last_actuator_status_ = result.status;
        }
        if (!entry_restored) {
            restored = false;
        }
    }
    if (!restored) {
        mutated_ = true;
        restore_failed_ = true;
        last_failure_stage_ = ResourceMutationFailureStage::RecoveryActuator;
        return false;
    }
    if (!journal_->clear()) {
        last_failure_stage_ = ResourceMutationFailureStage::JournalCleanup;
        return false;
    }
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

    baseline_captured_ = false;
    retained_entries_.clear();
    retained_active_resources_.clear();
    active_resources_cache_.clear();
    pending_outcome_ = false;
    baseline_.clear();
    dirty_resources_.clear();
    retained_active_resources_.clear();
    active_resources_cache_.clear();
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

    if (baseline_.empty()) return false;

    if (retained_store_ != nullptr) {
        RetainedStateStore::Entries entries;
        const RetainedStateStore::LoadState load_state = retained_store_->load(entries);
        if (load_state == RetainedStateStore::LoadState::Corrupt) return false;
        retained_entries_ = std::move(entries);
        const std::string scope = retainedScopeFor(profile);
        std::vector<std::string> stale_paths;
        for (const auto& retained : retained_entries_) {
            if (retained.scope != scope) {
                stale_paths.push_back(retained.path);
                continue;
            }
            const auto baseline_it = baseline_.find(retained.path);
            if (baseline_it == baseline_.end() ||
                !fallbackPolicyAuthorized(baseline_it->second.domain,
                                          baseline_it->second.name, retained.path)) {
                stale_paths.push_back(retained.path);
                continue;
            }
            std::string live;
            if (!ResourceActuator::readComparable(retained.path, live)) return false;
            if (live == retained.target) {
                baseline_it->second.value = retained.baseline;
                dirty_resources_.insert(retained.path);
                mutated_ = true;
            } else if (live == retained.baseline) {
                baseline_it->second.value = retained.baseline;
            } else {
                // The node was changed by another actor; a retained record must
                // never turn into authority to overwrite that external value.
                baseline_it->second.value = live;
                stale_paths.push_back(retained.path);
            }
        }
        for (const std::string& path : stale_paths) {
            if (!retained_store_->erasePath(path)) return false;
            retained_entries_.erase(std::remove_if(retained_entries_.begin(), retained_entries_.end(),
                [&path](const RetainedStateEntry& entry) { return entry.path == path; }),
                retained_entries_.end());
        }
    }

    baseline_captured_ = !baseline_.empty();
    if (baseline_captured_) refreshRetainedActiveCache();
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
    const UserGrants& grants,
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
    // Outcome-driven pressure signals — machine measures, model scales.
    ctx.psi_available = sample.psi_available;
    ctx.psi_cpu = sample.psi_cpu_some_avg10;
    ctx.psi_memory = sample.psi_memory_some_avg10;
    ctx.psi_io = sample.psi_io_some_avg10;
    ctx.throttling_detected = sample.throttling_detected;
    ctx.throttle_ratio = sample.throttle_ratio;
    ctx.memory_stall_detected = sample.memory_stall_detected;
    ctx.memory_stall_score = sample.memory_stall_score;
    ctx.io_latency_available = sample.io_latency_available;
    ctx.io_avg_latency_ms = sample.io_avg_latency_ms;

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
    EnvironmentCapabilityMatrix legacy_matrix;
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
        legacy_matrix.resources.push_back(std::move(capability));
    }
    // Legacy adapters get the same capability evidence as discovered nodes
    // (kernel-advertised options); there is no vendor data for them here.
    if (!legacy_matrix.resources.empty()) {
        TunableDiscovery{}.enrich(legacy_matrix, VendorKnowledge{});
        for (auto& capability : legacy_matrix.resources) {
            effective_matrix.resources.push_back(std::move(capability));
        }
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
    agent_observation.max_candidates = 16;
    const AgentProposal proposal = decision_agent_.reason(agent_observation);
    last_agent_reason_ = proposal.reasoning_summary;
    if (!proposal.valid) return false;
    const auto& ranked = proposal.ranked;

    ExperienceMemory::Context experience_context;
    experience_context.state = state;
    // Same classifier that produced the recorded experience context; a local
    // copy with different thresholds (0.80/0.65 vs 0.72/0.60) made lookups miss.
    experience_context.workload = ContextEngine{}.evaluate(sample, state).workload;
    experience_context.charging = sample.charging;
    experience_context.thermal_trend = sample.thermal_trend;
    experience_context.memory_trend = sample.memory_trend;
    experience_context.load_trend = sample.load_trend;

    double best_utility = -1.0;
    Candidate best_candidate;
    bool found_candidate = false;
    bool best_candidate_retained = false;

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

        // Consultation: the eye reports facts, the brain decides. A Defer or
        // Deny drops only this candidate. The cycle continues, and nothing is
        // written without an Allow and the permit that follows it.
        {
            ControlFact fact;
            fact.ns = namespaceOf(bit->second.domain, name);
            // Read the same effective view that ranking used, so the eye reports
            // exactly what the brain is asked to judge.
            for (const auto& cap : effective_matrix.resources) {
                if (cap.path != path) continue;
                fact.exists = cap.exists;
                fact.readable = cap.readable;
                fact.writable = cap.writable;
                fact.verified = cap.runtime_verified;
                break;
            }
            fact.baseline_captured = bit->second.valid;
            // Same guard the write path enforces, so the brain never approves a
            // surface the actuator would refuse.
            fact.surface_admissible = fallbackPolicyAuthorized(bit->second.domain, name, path);
            fact.semantic_known = scored.eligible && scored.direction != EffectDirection::None;
            fact.stabilizing = scored.stabilizing;
            fact.benefit_justified = scored.eligible;
            const Consultation verdict = consult(fact, state, grants);
            consultation_ledger_.record(fact.ns, verdict);
            if (verdict.verdict != ConsultVerdict::Allow) continue;
        }

        if (!interventionAllows(policy.intervention,
                                requiredIntervention(bit->second.domain, name))) {
            continue;
        }

        // The candidate value came from capability evidence (vendor config,
        // kernel-advertised options, ordered scale or a bounded step). Skip it
        // when it is already live; the actuator verifies the write by read-back.
        const std::string requested = scored.requested;
        if (requested.empty() || requested == bit->second.value) continue;
        {
            std::string live;
            if (!ResourceActuator::readComparable(path, live)) continue;
            if (live == requested) continue;
        }

        if (isRejected(path, requested)) continue;

        // EffectModel utility ranges from -0.25 to 0.75. DecisionAgent
        // may subtract up to 0.04 for evidence uncertainty, so normalize
        // [-0.29, 0.75] to [0,1] before blending empirical experience.
        double utility = std::clamp((scored.utility + 0.29) / 1.04, 0.0, 1.0);
        if (experience_memory_ != nullptr) {
            ExperienceMemory::CandidateIdentity identity;
            identity.key = "resource:" + path + ":" + requested;
            utility = blendExperienceScore(
                utility, experience_memory_->find(identity, experience_context));
        }

        const bool candidate_retained = std::any_of(
            retained_entries_.begin(), retained_entries_.end(),
            [&path, &requested](const RetainedStateEntry& entry) {
                return entry.path == path && entry.target == requested;
            });

        // Preserve the original raw-utility admission threshold of 0.05
        // after the affine normalization above.
        constexpr double normalized_min_utility = (0.05 + 0.29) / 1.04;
        if (utility < normalized_min_utility ||
            utility < best_utility ||
            (utility == best_utility &&
             (!candidate_retained || best_candidate_retained))) {
            continue;
        }


        best_utility = utility;
        best_candidate_retained = candidate_retained;
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
    RuntimeState state,
    double confidence,
    const EngineConfig& config,
    const PolicyPlan& policy
) noexcept {
    if (journal_ == nullptr) {
        last_failure_stage_ = ResourceMutationFailureStage::JournalUnavailable;
        return MutationResult::Failed;
    }
    if (!policy.allows(candidate.domain, candidate.name)) return MutationResult::Skipped;
    if (!fallbackPolicyAuthorized(candidate.domain, candidate.name, candidate.path)) {
        return MutationResult::Skipped;
    }

    ActuatorMutation mutation;
    // Resource writes never use the CpuFreq actuator domain: that domain is the
    // governor authority and needs allow_cpu_governor.
    mutation.id = {ActuatorDomain::Io, 0};
    switch (candidate.domain) {
        case ResourceDomain::Memory: mutation.id.domain = ActuatorDomain::Vm; break;
        case ResourceDomain::UClamp: mutation.id.domain = ActuatorDomain::Uclamp; break;
        case ResourceDomain::CpuSet: mutation.id.domain = ActuatorDomain::CpuSet; break;
        default: break;
    }
    mutation.target = candidate.path;
    mutation.requested = candidate.requested;

    // Issue a capability-bound permit for this exact candidate, not the
    // coarse scope permit used by the outer runtime gate.
    const ActuatorCapability capability = actuator_.capability(
        mutation.id, mutation.target);
    const MutationAuthority authority;
    const MutationPermit permit = authority.authorize(
        policy, config, state, confidence, capability, mutation.target);
    if (!permit.validFor(mutation)) return MutationResult::Skipped;

    std::string previous_value;
    if (!ResourceActuator::readComparable(candidate.path, previous_value)) {
        last_failure_stage_ = ResourceMutationFailureStage::BaselineRead;
        return MutationResult::Failed;
    }
    const bool was_dirty_before = dirty_resources_.find(candidate.path) != dirty_resources_.end();
    const bool from_retained = std::any_of(retained_entries_.begin(), retained_entries_.end(),
        [&candidate](const RetainedStateEntry& retained) {
            return retained.path == candidate.path && retained.target == candidate.requested;
        });

    // Durability before the write. The journal holds factory values for every
    // resource this controller has changed, plus the candidate about to change.
    MutationJournal::Entries entries = factorySnapshot();
    if (dirty_resources_.find(candidate.path) == dirty_resources_.end()) {
        const auto it = baseline_.find(candidate.path);
        if (it != baseline_.end() && it->second.valid) {
            entries.emplace_back(candidate.path, it->second.value);
        }
    }
    if (entries.empty() || !journal_->commit(entries)) {
        last_failure_stage_ = ResourceMutationFailureStage::JournalCommit;
        return MutationResult::Failed;
    }
    journal_committed_ = true;

    const ActuatorResult result = actuator_.apply(mutation, permit);
    last_actuator_status_ = result.status;
    if (result.status == ActuatorStatus::ReadFailed) {
        last_failure_stage_ = ResourceMutationFailureStage::ActuatorRead;
    } else if (result.status == ActuatorStatus::SafetyRejected) {
        last_failure_stage_ = ResourceMutationFailureStage::ActuatorSafety;
    } else if (result.status == ActuatorStatus::VerifyFailed ||
               result.status == ActuatorStatus::WriteFailed) {
        last_failure_stage_ = ResourceMutationFailureStage::ActuatorWriteOrVerify;
    } else if (result.status != ActuatorStatus::Ok &&
               result.status != ActuatorStatus::NoChange &&
               result.status != ActuatorStatus::RolledBack) {
        last_failure_stage_ = ResourceMutationFailureStage::ActuatorOther;
    }

    // Any attempted write may have landed, even if verification failed, so the
    // path is tracked as dirty until an explicit rollback clears it.
    if (result.writes_attempted != 0 && result.status != ActuatorStatus::RolledBack) {
        mutated_ = true;
        dirty_resources_.insert(candidate.path);
    }

    if (result.status == ActuatorStatus::RolledBack) {
        if (!was_dirty_before) dirty_resources_.erase(candidate.path);
        mutated_ = !dirty_resources_.empty();
        if (journal_committed_ && journal_ != nullptr) {
            const MutationJournal::Entries snapshot = factorySnapshot();
            const bool journal_ok = snapshot.empty() ? journal_->clear()
                                                     : journal_->commit(snapshot);
            if (!journal_ok) {
                restore_failed_ = true;
                mutated_ = true;
                last_failure_stage_ = ResourceMutationFailureStage::JournalCleanup;
                return MutationResult::Failed;
            }
            journal_committed_ = !snapshot.empty();
        }
        return MutationResult::RolledBack;
    }

    if (result.changed()) {
        last_applied_resources_.clear();
        last_applied_resources_.emplace_back(candidate.path, candidate.requested);
        pending_outcome_ = true;
        last_mutation_path_ = candidate.path;
        last_mutation_target_ = candidate.requested;
        last_previous_value_ = previous_value;
        last_path_was_dirty_before_ = was_dirty_before;
        last_candidate_from_retained_ = from_retained;
        refreshActiveSettingsCache();
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
            last_failure_stage_ = ResourceMutationFailureStage::JournalCleanup;
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
    last_failure_stage_ = ResourceMutationFailureStage::None;
    last_actuator_status_ = ActuatorStatus::Invalid;
    if (!recoverIfNeeded()) {
        if (last_failure_stage_ == ResourceMutationFailureStage::None) {
            last_failure_stage_ = ResourceMutationFailureStage::Recovery;
        }
        return MutationResult::Failed;
    }
    if (!baseline_captured_ && !captureBaseline(profile)) {
        return MutationResult::Skipped;
    }
    if (restore_failed_) {
        if (restoreAll()) return MutationResult::RolledBack;
        last_failure_stage_ = ResourceMutationFailureStage::Rollback;
        return MutationResult::Failed;
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
        // Only an authority loss restores here. A merely ineligible plan or a
        // transient confidence dip means "no new candidate"; accepted settings
        // persist until shouldRestoreEpochNow() reports a safety/authority
        // transition (Idle, stress with a non-stabilizing epoch, mode/arm off).
        const bool authority_lost =
            config.mutationMode() != MutationMode::Adaptive || !config.mutationArmed();
        if (!authority_lost) return MutationResult::Skipped;
        return restoreAll() ? MutationResult::RolledBack : MutationResult::Failed;
    }
    if (pending_outcome_) return MutationResult::Skipped;

    Candidate candidate;
    UserGrants grants;
    grants.memory = config.grantMemory();
    grants.block_queue = config.grantBlockQueue();
    grants.scheduler = config.grantScheduler();
    grants.gpu = config.grantGpu();
    grants.cgroup = config.grantCgroup();
    grants.cpufreq = config.grantCpuFreq();
    grants.kernel = config.grantKernel();
    if (!selectCandidate(state, sample, profile, policy, grants, candidate)) {
        return MutationResult::Skipped;
    }

    const bool was_clean = !mutated_;
    const MutationResult applied = applyCandidate(
        candidate, state, sample.confidence, config, policy);
    if (applied == MutationResult::Verified) {
        stabilizing_epoch_ = was_clean ? policy.stabilizing_only
                                       : (stabilizing_epoch_ && policy.stabilizing_only);
        return MutationResult::Verified;
    }
    if (applied == MutationResult::Skipped) return MutationResult::Skipped;
    if (applied == MutationResult::RolledBack) return MutationResult::RolledBack;
    if (mutated_ || restore_failed_) {
        if (restoreAll()) return MutationResult::RolledBack;
        last_failure_stage_ = ResourceMutationFailureStage::Rollback;
        return MutationResult::Failed;
    }
    if (last_failure_stage_ == ResourceMutationFailureStage::None) {
        last_failure_stage_ = ResourceMutationFailureStage::ActuatorOther;
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
        std::error_code path_error;
        const bool path_exists = std::filesystem::exists(path, path_error);
        if (item == baseline_.end() || !item->second.valid ||
            path_error || !path_exists) {
            ok = false;
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
        last_failure_stage_ = ResourceMutationFailureStage::Rollback;
        return false;
    }
    if (journal_committed_ && journal_ != nullptr && !journal_->clear()) {
        restore_failed_ = true;
        mutated_ = true;
        last_failure_stage_ = ResourceMutationFailureStage::JournalCleanup;
        return false;
    }
    journal_committed_ = false;
    restore_failed_ = false;
    mutated_ = false;
    dirty_resources_.clear();
    retained_active_resources_.clear();
    active_resources_cache_.clear();
    last_applied_resources_.clear();
    stabilizing_epoch_ = false;
    pending_outcome_ = false;
    last_mutation_path_.clear();
    last_mutation_target_.clear();
    last_previous_value_.clear();
    last_path_was_dirty_before_ = false;
    last_candidate_from_retained_ = false;
    return true;
}

bool ResourceMutationController::retainLastMutation(const DeviceProfile& profile) noexcept {
    if (!pending_outcome_ || retained_store_ == nullptr ||
        last_mutation_path_.empty() || last_mutation_target_.empty()) return false;
    const auto baseline = baseline_.find(last_mutation_path_);
    if (baseline == baseline_.end() || !baseline->second.valid ||
        baseline->second.value == last_mutation_target_) return false;
    std::string live;
    if (!ResourceActuator::readComparable(last_mutation_path_, live) ||
        live != last_mutation_target_) return false;

    RetainedStateEntry entry;
    entry.scope = retainedScopeFor(profile);
    entry.path = last_mutation_path_;
    entry.baseline = baseline->second.value;
    entry.target = last_mutation_target_;
    RetainedStateStore::Entries updated_entries;
    try {
        updated_entries = retained_entries_;
        updated_entries.erase(std::remove_if(updated_entries.begin(), updated_entries.end(),
            [&entry](const RetainedStateEntry& current) { return current.path == entry.path; }),
            updated_entries.end());
        updated_entries.push_back(entry);
    } catch (...) {
        return false;
    }
    if (!retained_store_->upsert(entry)) {
        // A rename may have succeeded even if the final directory sync failed.
        // Best-effort restore the last known-good ledger so an unaccepted
        // candidate cannot become a retained preference after restart.
        (void)retained_store_->commit(retained_entries_);
        return false;
    }
    retained_entries_.swap(updated_entries);
    pending_outcome_ = false;
    last_applied_resources_.clear();
    refreshRetainedActiveCache();
    last_mutation_path_.clear();
    last_mutation_target_.clear();
    last_previous_value_.clear();
    last_path_was_dirty_before_ = false;
    last_candidate_from_retained_ = false;
    return true;
}

bool ResourceMutationController::rollbackLastMutation() noexcept {
    if (!pending_outcome_ || last_mutation_path_.empty() || last_previous_value_.empty()) {
        return false;
    }

    std::string live;
    if (!ResourceActuator::readComparable(last_mutation_path_, live)) {
        restore_failed_ = true;
        mutated_ = true;
        return false;
    }

    bool external_override = false;
    bool storage_ok = true;
    if (live == last_mutation_target_) {
        const ActuatorResult result = actuator_.restore(last_mutation_path_, last_previous_value_);
        if (!(result.succeeded() || result.status == ActuatorStatus::RolledBack)) {
            restore_failed_ = true;
            mutated_ = true;
            return false;
        }
    } else if (live != last_previous_value_) {
        // Do not overwrite a value changed externally after the candidate write.
        external_override = true;
        const auto baseline = baseline_.find(last_mutation_path_);
        if (baseline != baseline_.end()) baseline->second.value = live;
        dirty_resources_.erase(last_mutation_path_);
        if (retained_store_ != nullptr) {
            storage_ok = retained_store_->erasePath(last_mutation_path_);
        }
        retained_entries_.erase(std::remove_if(retained_entries_.begin(), retained_entries_.end(),
            [this](const RetainedStateEntry& entry) {
                return entry.path == last_mutation_path_;
            }), retained_entries_.end());
    }

    if (!external_override && !last_path_was_dirty_before_) {
        dirty_resources_.erase(last_mutation_path_);
    }
    mutated_ = !dirty_resources_.empty();

    if (journal_ != nullptr && journal_committed_) {
        MutationJournal::Entries snapshot = factorySnapshot();
        const bool journal_ok = snapshot.empty() ? journal_->clear()
                                                 : journal_->commit(snapshot);
        if (!journal_ok) {
            restore_failed_ = true;
            mutated_ = true;
            return false;
        }
        journal_committed_ = !snapshot.empty();
    }

    if (last_candidate_from_retained_ && !last_path_was_dirty_before_ &&
        !external_override && retained_store_ != nullptr) {
        if (!retained_store_->erasePath(last_mutation_path_)) storage_ok = false;
        retained_entries_.erase(std::remove_if(retained_entries_.begin(), retained_entries_.end(),
            [this](const RetainedStateEntry& entry) {
                return entry.path == last_mutation_path_;
            }), retained_entries_.end());
    }

    pending_outcome_ = false;
    last_applied_resources_.clear();
    refreshRetainedActiveCache();
    last_mutation_path_.clear();
    last_mutation_target_.clear();
    last_previous_value_.clear();
    last_path_was_dirty_before_ = false;
    last_candidate_from_retained_ = false;
    if (!storage_ok) {
        restore_failed_ = true;
        mutated_ = true;
        return false;
    }
    return true;
}

void ResourceMutationController::refreshRetainedActiveCache() noexcept {
    try {
        retained_active_resources_.clear();
        for (const auto& retained : retained_entries_) {
            std::string live;
            if (!ResourceActuator::readComparable(retained.path, live) ||
                live != retained.target) continue;
            retained_active_resources_.emplace_back(retained.path, retained.target);
        }
        refreshActiveSettingsCache();
    } catch (...) {
        retained_active_resources_.clear();
        active_resources_cache_.clear();
    }
}

void ResourceMutationController::refreshActiveSettingsCache() noexcept {
    try {
        active_resources_cache_ = retained_active_resources_;
        if (!pending_outcome_) return;
        for (const auto& candidate : last_applied_resources_) {
            const auto same_path = std::find_if(active_resources_cache_.begin(),
                active_resources_cache_.end(), [&candidate](const auto& current) {
                    return current.first == candidate.first;
                });
            if (same_path == active_resources_cache_.end()) {
                active_resources_cache_.push_back(candidate);
            } else {
                *same_path = candidate;
            }
        }
    } catch (...) {
        active_resources_cache_.clear();
    }
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
