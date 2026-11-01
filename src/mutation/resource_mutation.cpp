#include "coreflow/resource_mutation.hpp"
#include "coreflow/context.hpp"
#include "coreflow/decision_trace.hpp"
#include "coreflow/tunable_discovery.hpp"
#include "coreflow/integer_parse.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <utility>
#include <optional>

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


bool liveDependentBoundsAllow(ResourceDomain domain, const std::string& path,
                              const std::string& requested) noexcept {
    // Recheck siblings against the LIVE configuration immediately before the
    // first privileged write. Earlier ranking may have observed older values,
    // including after another member of a coordinated cohort changed them.
    if (domain == ResourceDomain::Memory &&
        (path.ends_with("/dirty_ratio") || path.ends_with("/dirty_background_ratio"))) {
        const bool background = path.ends_with("/dirty_background_ratio");
        const std::string_view leaf = background ? "/dirty_background_ratio" : "/dirty_ratio";
        const std::string peer_path = path.substr(0, path.size() - leaf.size()) +
            (background ? "/dirty_ratio" : "/dirty_background_ratio");
        std::string peer;
        long long target_value = 0, peer_value = 0;
        if (!ResourceActuator::readComparable(peer_path, peer) ||
            !integer_parse::parseSignedDecimal(requested, target_value) ||
            !integer_parse::parseSignedDecimal(peer, peer_value)) return false;
        return background ? target_value <= peer_value : target_value >= peer_value;
    }
    if (domain == ResourceDomain::UClamp &&
        (path.ends_with("/cpu.uclamp.min") || path.ends_with("/cpu.uclamp.max"))) {
        const bool minimum = path.ends_with("/cpu.uclamp.min");
        const auto slash = path.find_last_of('/');
        if (slash == std::string::npos) return false;
        std::string peer;
        if (!ResourceActuator::readComparable(path.substr(0, slash) +
            (minimum ? "/cpu.uclamp.max" : "/cpu.uclamp.min"), peer)) return false;
        const auto parse_percent = [](const std::string& text, double& value) noexcept {
            if (text == "max") { value = 100.0; return true; }
            try {
                std::size_t consumed = 0;
                value = std::stod(text, &consumed);
                return consumed == text.size() && std::isfinite(value) &&
                       value >= 0.0 && value <= 100.0;
            } catch (...) { return false; }
        };
        double target = 0.0, other = 0.0;
        if ((minimum && requested == "max") ||
            !parse_percent(requested, target) || !parse_percent(peer, other)) return false;
        return minimum ? target <= other : target >= other;
    }
    return true;
}

bool isMutableDomain(ResourceDomain domain) noexcept {
    return isTunableResourceDomain(domain);
}

bool fallbackPolicyAuthorized(ResourceDomain domain, const std::string& name,
                              const std::string& path) noexcept {
    // Capability-driven: this is the path/semantic gate; the actuator adds an
    // independent opened-filesystem check immediately before every write.
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
    recovery_pending_ = true;
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
    // Our own in-flight journal is not a crash from a previous process.
    if (!recovery_pending_) return journal_ != nullptr;
    // Without a journal we cannot prove that a previous process left no
    // in-flight mutation. Failing closed here prevents baseline capture from
    // silently adopting an unknown post-crash value as the factory baseline.
    if (journal_ == nullptr) {
        last_failure_stage_ = ResourceMutationFailureStage::JournalUnavailable;
        return false;
    }
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
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
        const auto& entry = *it;
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
        // Only overwrite last_actuator_status_ on success while every prior
        // entry succeeded, or on the first failure encountered.
        if (!entry_restored) {
            if (restored) {
                last_actuator_status_ = result.status;
            }
            restored = false;
            break; // keep governor/scheduler anchors until dependents restore
        } else if (restored) {
            last_actuator_status_ = result.status;
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
    pending_changes_.clear();
    retained_entries_.clear();
    retained_active_resources_.clear();
    active_resources_cache_.clear();
    pending_outcome_ = false;
    baseline_.clear();
    dirty_resources_.clear();
    write_order_.clear();
    accepted_resources_.clear();
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

bool ResourceMutationController::admitNewBaselines(const DeviceProfile& profile) noexcept {
    try {
        for (const auto& cap : profile.capabilities.resources) {
            if (baseline_.contains(cap.path) || !cap.mutation_ready || !cap.policy_authorized ||
                !cap.permission_granted || !cap.runtime_verified || !cap.readable || !cap.writable ||
                !fallbackPolicyAuthorized(cap.domain, cap.name, cap.path)) continue;
            std::string live;
            if (!ResourceActuator::readComparable(cap.path, live) || !ResourceActuator::writable(cap.path)) continue;
            baseline_.emplace(cap.path, Baseline{cap.domain, cap.name, cap.path, live, true});
        }
        baseline_captured_ = !baseline_.empty();
        return baseline_captured_;
    } catch (...) { return false; }
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
    Candidate& out, bool companion, bool condition_driven
) const noexcept try {
    if (!baseline_captured_ || baseline_.empty()) return false;
    const auto trial_conditions=trial_history_ ? trial_history_->conditions(sample,state) : TrialConditions{};
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
    ctx.memory_available = sample.mem_total_kb > 0U;
    ctx.charging_telemetry_available = sample.charging_telemetry_available &&
                                       sample.battery_level_percent >= 0;
    ctx.mem_available_ratio = sample.mem_available_ratio;
    ctx.cpu_utilization = sample.cpu_utilization;
    ctx.cpu_utilization_available = sample.cpu_utilization_available;
    // EffectModel uses a negative sentinel for an unavailable load average so
    // its elevated-load prior cannot mistake the observer's default zero for data.
    ctx.load1 = sample.load_available ? sample.load1 : -1.0;
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
    ctx.gpu_busy_available = sample.gpu_busy_available;
    ctx.gpu_busy_ratio = sample.gpu_busy_ratio;
    ctx.frame_jank_rate = sample.frame_jank_rate;
    ctx.gaming_active = sample.gaming_active;
    ctx.frame_jank_available = sample.frame_jank_available;
    ctx.storage_health_caution = sample.storage_health_caution;

    if (policy.action == PolicyAction::ReduceIntervention) {
        ctx.intervention = InterventionLevel::Low;
    }

    std::unordered_map<std::string, std::string> baseline_values;
    baseline_values.reserve(baseline_.size() + profile.cpu_policies.size());
    for (const auto& item : baseline_) {
        if (item.second.valid) {
            baseline_values[item.first] = item.second.value;
        }
    }
    // Rank against the actual accepted/current cohort configuration. Factory
    // baselines remain separate and are used only for durable restoration.
    for (const auto& path : dirty_resources_) {
        std::string live;
        if (!ResourceActuator::readComparable(path, live)) return false;
        baseline_values[path] = std::move(live);
    }
    // Expose active CPUFreq scaling_governor posture so StrategyCatalog can
    // couple governor rate_limit_us / uclamp / queue parameters to the live governor.
    for (const CpuPolicy& cpu_pol : profile.cpu_policies) {
        if (cpu_pol.path.empty()) continue;
        const std::string gov_path = cpu_pol.path + "/scaling_governor";
        std::string live_gov;
        if (ResourceActuator::readComparable(gov_path, live_gov) && !live_gov.empty()) {
            baseline_values[gov_path] = live_gov;
        } else if (!cpu_pol.governor.empty()) {
            baseline_values[gov_path] = cpu_pol.governor;
        }
    }

    // Discovery remains authoritative. The baseline map also contains legacy
    // DeviceProfile adapters (VM and block-queue fields) which predate the
    // unified matrix; expose those as capabilities only when Discovery omitted
    // the same path, and only because captureBaseline already verified both
    // readability and writability for that adapter.
    EnvironmentCapabilityMatrix effective_matrix = profile.capabilities;
    // A read-only governor anchor is useful even when its write interface was
    // excluded from the generic resource inventory.
    for (const auto& cpu : profile.cpu_policies) {
        const std::string path = cpu.path + "/scaling_governor";
        if (baseline_values.contains(path) && std::none_of(effective_matrix.resources.begin(), effective_matrix.resources.end(),
            [&](const auto& cap) { return cap.path == path; })) {
            ResourceCapability anchor;
            anchor.domain = ResourceDomain::CpuFreq; anchor.path = path; anchor.name = path;
            anchor.exists = anchor.readable = true;
            effective_matrix.resources.push_back(std::move(anchor));
        }
        const auto current = baseline_values.find(path);
        if (current == baseline_values.end()) continue;
        const std::string prefix = cpu.path + "/";
        for (auto& cap : effective_matrix.resources) {
            if (!cap.path.starts_with(prefix)) continue;
            const std::string_view suffix(cap.path.data() + prefix.size(), cap.path.size() - prefix.size());
            const auto slash = suffix.find('/');
            if (slash == std::string_view::npos) continue;
            const auto owner = suffix.substr(0, slash);
            if (owner != current->second && std::find(cpu.available_governors.begin(), cpu.available_governors.end(), owner) != cpu.available_governors.end())
                cap.mutation_ready = false;
        }
    }
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

    // Exclude shared externally controlled paths before top-N ranking, so
    // contested candidates cannot crowd out otherwise eligible resources.
    candidate_funnel_ = {};
    candidate_funnel_.assessed = true;
    candidate_funnel_.discovered = effective_matrix.resources.size();
    for (auto& cap : effective_matrix.resources) {
        const bool structural = cap.mutation_ready && cap.exists && cap.readable && cap.writable && cap.runtime_verified;
        if (structural) ++candidate_funnel_.structural;
        const auto base = baseline_.find(cap.path);
        const bool baseline_ok = structural && base != baseline_.end() && base->second.valid;
        if (baseline_ok) ++candidate_funnel_.baseline;
        const bool granted = grants.allows(namespaceOf(cap.domain,cap.name));
        if (baseline_ok && granted) ++candidate_funnel_.granted;
        const bool relevant = !condition_driven || domainNeedsTuning(sample,state,cap.domain);
        if (baseline_ok && granted && relevant) ++candidate_funnel_.relevant;
        if (trial_history_) {
            const auto live=baseline_values.find(cap.path);
            if (baseline_ok && live!=baseline_values.end() &&
                trial_history_->blockReason(cap.path,live->second,cap.domain,trial_conditions)) {
                cap.mutation_ready=false; ++candidate_funnel_.history_held;
            }
        }
        if (condition_driven && (!baseline_ok || !granted || !relevant ||
            !policy.allows(cap.domain,cap.name) || !fallbackPolicyAuthorized(cap.domain,cap.name,cap.path)))
            cap.mutation_ready = false;
        if ((external_control_paths_ != nullptr && external_control_paths_->contains(cap.path)) ||
            std::any_of(pending_changes_.begin(), pending_changes_.end(),
                [&](const PendingChange& change) { return change.path == cap.path; })) cap.mutation_ready = false;
    }

    ExperienceMemory::Context experience_context;
    experience_context.state = state;
    // Same classifier that produced the recorded experience context; a local
    // copy with different thresholds (0.80/0.65 vs 0.72/0.60) made lookups miss.
    experience_context.workload = ContextEngine{}.evaluate(sample, state).workload;
    experience_context.charging = sample.charging;
    experience_context.thermal_trend = sample.thermal_trend;
    experience_context.memory_trend = sample.memory_trend;
    experience_context.load_trend = sample.load_trend;

    // The DecisionAgent is the proposal layer for resource candidates. It only
    // ranks observed capabilities; this controller still independently enforces
    // policy, intervention level, experience confidence, rejection cooldown,
    // MutationAuthority, journaling, verification and rollback before any write.
    AgentObservation agent_observation;
    agent_observation.effect_ctx = ctx;
    agent_observation.matrix = &effective_matrix;
    agent_observation.baselines = &baseline_values;
    agent_observation.experience = experience_memory_;
    agent_observation.experience_ctx = experience_context;
    agent_observation.state = state;
    agent_observation.sample_confidence = sample.confidence;
    agent_observation.mutation_allowed_by_context = !policy.stabilizing_only;
    agent_observation.stabilizing_allowed_by_context = policy.stabilizing_only;
    agent_observation.max_candidates = 16;
    const AgentProposal proposal = decision_agent_.reason(agent_observation);
    last_agent_reason_ = proposal.reasoning_summary;
    if (!proposal.valid) return false;
    const auto& ranked = proposal.ranked;
    candidate_funnel_.ranked = ranked.size();

    double best_utility = -1.0;
    Candidate best_candidate;
    bool found_candidate = false;
    bool best_candidate_retained = false;

    for (const auto& scored : ranked) {
        const std::string& name = scored.resource_name;
        const std::string& path = scored.path;
        if (name.empty() || path.empty()) continue;
        if (!companion && selected_primary_ != nullptr &&
            (path != selected_primary_->path || scored.requested != selected_primary_->requested)) continue;
        if (external_control_paths_ != nullptr && external_control_paths_->contains(path)) continue;

        auto bit = baseline_.find(path);
        if (bit == baseline_.end() || !bit->second.valid ||
            !fallbackPolicyAuthorized(bit->second.domain, name, path)) continue;

        if (!policy.allows(bit->second.domain, name)) continue;
        // A stressed context accepts only changes that respond to the stress by
        // reducing load. Everything else waits for headroom.
        if (policy.stabilizing_only && !scored.stabilizing) continue;
        if ((companion || coupled_only_) && !policy.stabilizing_only &&
            scored.reason.find("[coupled:") == std::string::npos &&
            !(condition_driven && !coupled_only_ && domainNeedsTuning(sample,state,scored.domain))) continue;

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
        std::string current;
        if (!ResourceActuator::readComparable(path,current) || current == requested) continue;
        if (trial_history_ && trial_history_->blockReason(path,current,scored.domain,trial_conditions,requested)) {
            ++candidate_funnel_.history_held; continue;
        }

        if (bit->second.domain == ResourceDomain::UClamp &&
            (path.ends_with("/cpu.uclamp.min") || path.ends_with("/cpu.uclamp.max"))) {
            const bool is_min = path.ends_with("/cpu.uclamp.min");
            const auto slash = path.find_last_of('/');
            const auto percentage = [](const std::string& value, double& ratio) noexcept {
                try {
                    if (value == "max") { ratio = 100.; return true; }
                    std::size_t used = 0;
                    ratio = std::stod(value, &used);
                    return used == value.size() && std::isfinite(ratio) && ratio >= 0. && ratio <= 100.;
                } catch (...) { return false; }
            };
            double target = 0.;
            if (!percentage(requested,target)) continue;
            // Bound cumulative capping against this device's factory ceiling,
            // not against an ever-decreasing last step.
            if (!is_min && policy.stabilizing_only) {
                double factory = 0.;
                if (!percentage(bit->second.value,factory) || target < factory / 2.) continue;
            }
            std::string sibling;
            if (ResourceActuator::readComparable(path.substr(0, slash) +
                    (is_min ? "/cpu.uclamp.max" : "/cpu.uclamp.min"), sibling)) {
                double peer = 0.;
                if (!percentage(sibling,peer) || (is_min ? target > peer : target < peer)) continue;
            }
        }

        if (path.ends_with("/queue/scheduler")) {
            const auto prefix = path.substr(0,path.find_last_of('/')) + "/iosched/";
            if (std::any_of(dirty_resources_.begin(),dirty_resources_.end(),
                    [&](const auto& owned) { return owned.starts_with(prefix); })) continue;
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
        if (utility >= normalized_min_utility) ++candidate_funnel_.selectable;
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
        best_candidate.current = current;
        best_candidate.confidence = scored.confidence;
        best_candidate.utility = utility * 1.04 - .29;
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
} catch (...) { return false; }

MutationJournal::Entries
ResourceMutationController::factorySnapshot() const {
    MutationJournal::Entries entries;
    entries.reserve(dirty_resources_.size() + 1);
    for (const auto& path : write_order_) {
        if (!dirty_resources_.contains(path)) continue;
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
    const PolicyPlan& policy,
    const DecisionTraceContext* trace_context
) noexcept try {
    if (journal_ == nullptr) {
        last_failure_stage_ = ResourceMutationFailureStage::JournalUnavailable;
        if (trace_context != nullptr) {
            DecisionTraceEvent event{"mutation_preflight_failed", "resource", "FAILED",
                                     "mutation_journal_unavailable"};
            event.target = candidate.path.c_str();
            event.requested = candidate.requested.c_str();
            event.baseline_value = candidate.baseline.c_str();
            appendDecisionTraceEvent(*trace_context, event);
        }
        return MutationResult::Failed;
    }
    if (!policy.allows(candidate.domain, candidate.name)) return MutationResult::Skipped;
    if (!fallbackPolicyAuthorized(candidate.domain, candidate.name, candidate.path)) {
        return MutationResult::Skipped;
    }

    ActuatorMutation mutation;
    // Resource writes never use the CpuFreq actuator domain; CPUFreq governor
    // writes are handled by the dedicated governor controller.
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
    const bool permit_valid = permit.validFor(mutation);
    if (trace_context != nullptr) {
        const char* permit_reason = nullptr;
        if (permit_valid) {
            permit_reason = "target_bound_permit_issued";
        } else if (!capability.mutationSafe()) {
            permit_reason = "resource_actuator_capability_not_mutation_safe";
        } else {
            permit_reason = mutationPermitBlockReason(
                policy, config, state, confidence, MutationPermit::Scope::Resource);
            if (permit_reason == nullptr) permit_reason = "target_bound_permit_invalid";
        }
        DecisionTraceEvent event{"target_permit_checked", "resource",
                                 permit_valid ? "ISSUED" : "DENIED", permit_reason};
        event.permit_known = true;
        event.permit_valid = permit_valid;
        event.target = candidate.path.c_str();
        event.requested = candidate.requested.c_str();
        event.baseline_value = candidate.baseline.c_str();
        appendDecisionTraceEvent(*trace_context, event);
    }
    if (!permit_valid) return MutationResult::Skipped;

    std::string previous_value;
    const bool prewrite_read_ok = ResourceActuator::readComparable(
        candidate.path, previous_value);
    if (trace_context != nullptr) {
        const std::string rationale=candidate.reason+" benefit="+std::to_string(candidate.benefit)+
            " risk="+std::to_string(candidate.risk)+" confidence="+std::to_string(candidate.confidence);
        DecisionTraceEvent selected{"resource_candidate_selected","resource","SELECTED",rationale.c_str()};
        selected.target=candidate.path.c_str(); selected.requested=candidate.requested.c_str();
        selected.current_value=prewrite_read_ok ? previous_value.c_str() : nullptr;
        selected.baseline_value=candidate.baseline.c_str(); appendDecisionTraceEvent(*trace_context,selected);
        DecisionTraceEvent event{"prewrite_read_completed", "resource",
                                 prewrite_read_ok ? "READ" : "FAILED",
                                 prewrite_read_ok ? "live_resource_value_read"
                                                  : "live_resource_value_read_failed"};
        event.target = candidate.path.c_str();
        event.requested = candidate.requested.c_str();
        event.current_value = prewrite_read_ok ? previous_value.c_str() : nullptr;
        event.baseline_value = candidate.baseline.c_str();
        appendDecisionTraceEvent(*trace_context, event);
    }
    if (!prewrite_read_ok) {
        last_failure_stage_ = ResourceMutationFailureStage::BaselineRead;
        return MutationResult::Failed;
    }
    if (!liveDependentBoundsAllow(candidate.domain, candidate.path,
                                  candidate.requested)) {
        if (trace_context != nullptr) {
            DecisionTraceEvent event{"mutation_preflight_failed", "resource", "SKIPPED",
                                     "live_sibling_configuration_constraint"};
            event.target = candidate.path.c_str();
            event.requested = candidate.requested.c_str();
            event.current_value = previous_value.c_str();
            appendDecisionTraceEvent(*trace_context, event);
        }
        return MutationResult::Skipped;
    }
    const bool was_dirty_before = dirty_resources_.find(candidate.path) != dirty_resources_.end();
    const bool from_retained = std::any_of(retained_entries_.begin(), retained_entries_.end(),
        [&candidate](const RetainedStateEntry& retained) {
            return retained.path == candidate.path && retained.target == candidate.requested;
        });

    PendingChange change{candidate.path, candidate.requested, previous_value,
                         was_dirty_before, from_retained};
    std::pair<std::string, std::string> applied_entry{candidate.path, candidate.requested};
    std::string ordered_path = candidate.path;
    write_order_.reserve(write_order_.size() + 1U);

    // Durability before the write. The journal holds factory values for every
    // resource this controller has changed, plus the candidate about to change.
    MutationJournal::Entries entries = factorySnapshot();
    if (dirty_resources_.find(candidate.path) == dirty_resources_.end()) {
        const auto it = baseline_.find(candidate.path);
        if (it != baseline_.end() && it->second.valid) {
            entries.emplace_back(candidate.path, it->second.value);
        }
    }
    const bool journal_entries_ready = !entries.empty();
    const bool journal_commit_ok = journal_entries_ready && journal_->commit(entries);
    if (trace_context != nullptr) {
        const char* journal_reason = journal_commit_ok
            ? "factory_snapshot_committed_before_actuator_write"
            : journal_entries_ready ? "journal_commit_failed_no_actuator_write"
                                    : "journal_snapshot_empty_no_actuator_write";
        DecisionTraceEvent event{"mutation_journal_commit", "resource",
                                 journal_commit_ok ? "COMMITTED" : "FAILED",
                                 journal_reason};
        event.target = candidate.path.c_str();
        event.requested = candidate.requested.c_str();
        event.current_value = previous_value.c_str();
        event.baseline_value = candidate.baseline.c_str();
        appendDecisionTraceEvent(*trace_context, event);
    }
    if (!journal_commit_ok) {
        last_failure_stage_ = ResourceMutationFailureStage::JournalCommit;
        return MutationResult::Failed;
    }
    journal_committed_ = true;

    if (trace_context != nullptr) {
        DecisionTraceEvent event{"actuator_apply_started", "resource", "STARTED",
                                 "target_bound_permit_and_prewrite_journal_confirmed"};
        event.permit_known = true;
        event.permit_valid = permit.validFor(mutation);
        event.target = candidate.path.c_str();
        event.requested = candidate.requested.c_str();
        event.current_value = previous_value.c_str();
        event.baseline_value = candidate.baseline.c_str();
        appendDecisionTraceEvent(*trace_context, event);
    }
    // Allocate ownership bookkeeping before a privileged write can land.
    if (!dirty_resources_.contains(candidate.path)) {
        dirty_resources_.insert(candidate.path);
        write_order_.erase(std::remove(write_order_.begin(), write_order_.end(), candidate.path), write_order_.end());
        write_order_.push_back(std::move(ordered_path));
    }
    mutated_ = true;
    const ActuatorResult result = actuator_.apply(mutation, permit);
    last_actuator_status_ = result.status;
    if (trace_context != nullptr) {
        const std::string_view status = actuatorStatusName(result.status);
        DecisionTraceEvent event{"actuator_apply_completed", "resource", status.data(),
                                 "actuator_return_reports_write_and_readback_counts"};
        event.permit_known = true;
        event.permit_valid = permit.validFor(mutation);
        event.actuator_status = status.data();
        event.target = candidate.path.c_str();
        event.requested = candidate.requested.c_str();
        event.current_value = previous_value.c_str();
        event.baseline_value = candidate.baseline.c_str();
        event.write_counts_known = true;
        event.writes_attempted = result.writes_attempted;
        event.writes_verified = result.writes_verified;
        appendDecisionTraceEvent(*trace_context, event);
    }
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

    if (result.status != ActuatorStatus::Ok && result.status != ActuatorStatus::NoChange) {
        try {
            if (rejected_.size() >= 128U) {
                for (auto it = rejected_.begin(); it != rejected_.end(); ) {
                    if (cycle_ - it->second >= kRejectionCooldownCycles) {
                        it = rejected_.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            rejected_[candidate.path + "\n" + candidate.requested] = cycle_;
        } catch (...) {
        }
    }

    // Any attempted write may have landed, even if verification failed, so the
    // path is tracked as dirty until an explicit rollback clears it.
    if (result.writes_attempted == 0U && !was_dirty_before) dirty_resources_.erase(candidate.path);
    mutated_ = !dirty_resources_.empty();

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
        pending_changes_.push_back(std::move(change));
        last_applied_resources_.push_back(std::move(applied_entry));
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
} catch (...) {
    last_failure_stage_ = ResourceMutationFailureStage::ActuatorOther;
    return MutationResult::Failed;
}

PlanOption ResourceMutationController::preview(RuntimeState state, const RuntimeSample& sample,
    const DeviceProfile& profile, const EngineConfig& config, const PolicyPlan& policy) noexcept {
    ++cycle_; // rejecting every proposal must not freeze rejection cooldowns
    candidate_funnel_={}; candidate_funnel_.discovered=profile.capabilities.resources.size();
    try {
        if (trial_history_ && !trial_history_->ready())
            return {false,ResourceDomain::Vendor,{},{},{},{},"trial_history_checkpoint_unavailable"};
        if (!recoveryReady() || pending_outcome_ || !baseline_captured_)
            return {false,ResourceDomain::Vendor,{},{},{},{},"resource_baseline_recovery_or_measurement_pending"};
        if (const char* gate=mutationPermitBlockReason(policy,config,state,sample.confidence,MutationPermit::Scope::Resource))
            return {false,ResourceDomain::Vendor,{},{},{},{},gate};
        if (!measuredTuningNeed(sample,state))
            return {false,ResourceDomain::Vendor,{},{},{},{},"no_measured_resource_demand"};
        UserGrants grants;
        grants.memory=config.grantMemory(); grants.block_queue=config.grantBlockQueue(); grants.scheduler=config.grantScheduler();
        grants.gpu=config.grantGpu(); grants.cgroup=config.grantCgroup(); grants.cpufreq=config.grantCpuFreq(); grants.kernel=config.grantKernel();
        Candidate candidate;
        if (!selectCandidate(state,sample,profile,policy,grants,candidate,false,true))
            return {false,ResourceDomain::Vendor,{},{},{},{},"no_admissible_resource_gain_after_grants_and_ranking"};
        if (candidate.utility <= .05) return {};
        return {true,candidate.domain,candidate.name,candidate.path,candidate.current,candidate.requested,candidate.reason,
            candidate.benefit,candidate.risk,candidate.confidence,candidate.utility - .05};
    } catch (...) { return {}; }
}
MutationResult ResourceMutationController::applySelected(const PlanOption& selected, RuntimeState state,
    const RuntimeSample& sample, const DeviceProfile& profile, const EngineConfig& config,
    const PolicyPlan& policy, const MutationPermit& permit, const DecisionTraceContext* trace) noexcept {
    std::string live;
    if (!selected.valid || !ResourceActuator::readComparable(selected.path,live) || live != selected.current) return MutationResult::Skipped;
    selected_primary_ = &selected;
    const auto result = apply(state,sample,profile,config,policy,permit,trace);
    selected_primary_ = nullptr;
    return result;
}

MutationResult ResourceMutationController::apply(
    const RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const EngineConfig& config,
    const PolicyPlan& policy,
    const MutationPermit& permit,
    const DecisionTraceContext* trace_context
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
        // transition (pending trial canceled, unsafe stress, mode/arm off).
        const bool authority_lost =
            config.mutationMode() != MutationMode::Adaptive || !config.mutationArmed();
        if (!authority_lost) return MutationResult::Skipped;
        return restoreAll() ? MutationResult::RolledBack : MutationResult::Failed;
    }
    if (pending_outcome_) return MutationResult::Skipped;

    UserGrants grants;
    grants.memory = config.grantMemory();
    grants.block_queue = config.grantBlockQueue();
    grants.scheduler = config.grantScheduler();
    grants.gpu = config.grantGpu();
    grants.cgroup = config.grantCgroup();
    grants.cpufreq = config.grantCpuFreq();
    grants.kernel = config.grantKernel();
    try {
        pending_changes_.reserve(kMaxCohortWrites);
        supplemental_.clear();
        last_applied_resources_.clear();
        last_applied_resources_.reserve(kMaxCohortWrites);
        const bool was_clean = !mutated_;
        std::optional<DeviceProfile> refreshed;
        PolicyPlan working_policy = policy;
        const DeviceProfile* view = &profile;
        for (std::size_t i = 0; i < kMaxCohortWrites; ++i) {
            Candidate candidate;
            if (!selectCandidate(state, sample, *view, working_policy, grants, candidate, i != 0U, condition_driven_)) break;
            const MutationResult applied = applyCandidate(
                candidate, state, sample.confidence, config, working_policy, trace_context);
            if (applied == MutationResult::Verified) {
                if (candidate.domain == ResourceDomain::Io && candidate.path.ends_with("/queue/scheduler")) {
                    refreshed.emplace(profile);
                    (void)TunableDiscovery{}.discoverWithin(refreshed->capabilities, VendorKnowledge{},
                        candidate.path.substr(0,candidate.path.find_last_of('/')) + "/iosched");
                    if (!admitNewBaselines(*refreshed))
                        return rollbackLastMutation() ? MutationResult::RolledBack : MutationResult::Failed;
                    for (const auto& cap : refreshed->capabilities.resources) {
                        if (std::none_of(profile.capabilities.resources.begin(),profile.capabilities.resources.end(),
                                [&](const auto& previous) { return previous.path == cap.path; })) supplemental_.push_back(cap);
                        if (cap.mutation_ready && cap.readable && cap.writable &&
                            !working_policy.allows(cap.domain,cap.name))
                            working_policy.candidates.push_back({cap.domain,cap.name,{},sample.confidence,1.,PolicyAction::Candidate});
                    }
                    view = &*refreshed;
                }
                continue;
            }
            if (applied == MutationResult::Skipped && i == 0U) return applied;
            if (!pending_outcome_ && !mutated_ && !restore_failed_) return applied;
            // Never measure a partially failed cohort as successful.
            const bool rolled = pending_outcome_ && rollbackLastMutation();
            if (!rolled && (mutated_ || restore_failed_)) {
                if (!restoreAll()) return MutationResult::Failed;
            }
            return MutationResult::RolledBack;
        }
        if (pending_changes_.empty()) return MutationResult::Skipped;
        for (const auto& change : pending_changes_) {
            std::string live;
            if (!ResourceActuator::readComparable(change.path, live) || live != change.target) {
                rejectLastMutation();
                return rollbackLastMutation() ? MutationResult::RolledBack : MutationResult::Failed;
            }
        }
        stabilizing_epoch_ = was_clean ? policy.stabilizing_only
                                      : (stabilizing_epoch_ && policy.stabilizing_only);
        return MutationResult::Verified;
    } catch (...) {
        if (pending_outcome_ && rollbackLastMutation()) return MutationResult::RolledBack;
        if (mutated_ || restore_failed_) return restoreAll() ? MutationResult::RolledBack : MutationResult::Failed;
        return MutationResult::Failed;
    }

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

bool ResourceMutationController::reconcileExternalChanges(std::unordered_set<std::string>& paths, std::string* failure) noexcept {
    try {
        if (failure != nullptr) failure->clear();
        const auto owned = active_resources_cache_;
        for (const auto& setting : owned) {
            const std::string path = setting.first;
            std::string live;
            if (!ResourceActuator::readComparable(path, live)) {
                if (failure != nullptr) *failure = "read_live: " + path;
                return false;
            }
            if (live == ResourceActuator::comparableValue(setting.second) && !paths.contains(path)) continue;
            paths.insert(path); // block all actuator classes before any durable handoff
            if (retained_store_ != nullptr && !retained_store_->erasePath(path)) {
                if (failure != nullptr) *failure = "erase_retained: " + path;
                return false;
            }
            if (journal_committed_ && (journal_ == nullptr ||
                !journal_->releaseExternallyControlledPath(path))) {
                if (failure != nullptr) *failure = "release_journal: " + path;
                return false;
            }
            dirty_resources_.erase(path);
            accepted_resources_.erase(std::remove_if(accepted_resources_.begin(), accepted_resources_.end(),
                [&](const auto& current) { return current.first == path; }), accepted_resources_.end());
            const auto baseline = baseline_.find(path);
            if (baseline != baseline_.end()) baseline->second.valid = false;
            retained_entries_.erase(std::remove_if(retained_entries_.begin(), retained_entries_.end(),
                [&path](const auto& entry) { return entry.path == path; }), retained_entries_.end());
            retained_active_resources_.erase(std::remove_if(retained_active_resources_.begin(), retained_active_resources_.end(),
                [&setting](const auto& entry) { return entry.first == setting.first; }), retained_active_resources_.end());
            active_resources_cache_.erase(std::remove_if(active_resources_cache_.begin(), active_resources_cache_.end(),
                [&setting](const auto& entry) { return entry.first == setting.first; }), active_resources_cache_.end());
            last_applied_resources_.erase(std::remove_if(last_applied_resources_.begin(), last_applied_resources_.end(),
                [&setting](const auto& entry) { return entry.first == setting.first; }), last_applied_resources_.end());
            pending_changes_.erase(std::remove_if(pending_changes_.begin(), pending_changes_.end(),
                [&](const auto& change) { return change.path == path; }), pending_changes_.end());
            pending_outcome_ = !pending_changes_.empty();
            if (last_mutation_path_ == path) {
                last_mutation_path_.clear(); last_mutation_target_.clear(); last_previous_value_.clear();
                last_path_was_dirty_before_ = false; last_candidate_from_retained_ = false;
            }
            mutated_ = !dirty_resources_.empty();
            journal_committed_ = mutated_;
            if (!mutated_) restore_failed_ = false;
        }
        return true;
    } catch (...) { return false; }
}

bool ResourceMutationController::restoreOneRetained() noexcept {
    if (pending_outcome_ || !recoveryReady()) return false;
    if (!mutated_) return true;
    try {
        if (active_resources_cache_.empty()) return false;
        auto setting = active_resources_cache_.back();
        // Restore iosched children before their scheduler anchor, regardless of
        // cache insertion order (retained and session settings have separate stores).
        if (setting.first.ends_with("/queue/scheduler")) {
            const auto prefix=setting.first.substr(0,setting.first.find_last_of('/'))+"/iosched/";
            for (const auto& active:active_resources_cache_)
                if (active.first.starts_with(prefix)) { setting=active; break; }
        }
        const auto base=baseline_.find(setting.first);
        if (base==baseline_.end() || !base->second.valid || journal_==nullptr) return false;
        if (external_control_paths_ && external_control_paths_->contains(setting.first)) return false;
        pending_changes_.push_back({setting.first,setting.second,base->second.value,false,true});
        pending_outcome_=true;
        if (!rollbackLastMutation()) return false;
        accepted_resources_.erase(std::remove_if(accepted_resources_.begin(),accepted_resources_.end(),
            [&](const auto& entry) { return entry.first==setting.first; }),accepted_resources_.end());
        write_order_.erase(std::remove(write_order_.begin(),write_order_.end(),setting.first),write_order_.end());
        refreshRetainedActiveCache();
        return true;
    } catch (...) { restore_failed_=true; return false; }
}

bool ResourceMutationController::restoreAll() noexcept {
    // A failed startup recovery owns a durable journal, not an initialized RAM
    // dirty set. Never clear that journal by "restoring" an empty RAM inventory.
    if (recovery_pending_ && journal_ != nullptr) return recoverIfNeeded();
    if (!mutated_ && !restore_failed_) return true;

    bool ok = true;
    for (auto it = write_order_.rbegin(); it != write_order_.rend(); ++it) {
        const auto& path = *it;
        if (!dirty_resources_.contains(path)) continue;
        if (external_control_paths_ != nullptr && external_control_paths_->contains(path)) {
            ok = false; break;
        }
        const auto item = baseline_.find(path);
        std::error_code path_error;
        const bool path_exists = std::filesystem::exists(path, path_error);
        if (item == baseline_.end() || !item->second.valid ||
            path_error || !path_exists) {
            ok = false;
            break;
        }
        const ActuatorResult result =
            actuator_.restore(path, item->second.value);
        if (!(result.status == ActuatorStatus::RolledBack ||
              result.status == ActuatorStatus::NoChange ||
              result.status == ActuatorStatus::Ok)) {
            ok = false;
            break;
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
    write_order_.clear();
    accepted_resources_.clear();
    retained_active_resources_.clear();
    active_resources_cache_.clear();
    last_applied_resources_.clear();
    stabilizing_epoch_ = false;
    pending_changes_.clear();
    pending_outcome_ = false;
    last_mutation_path_.clear();
    last_mutation_target_.clear();
    last_previous_value_.clear();
    last_path_was_dirty_before_ = false;
    last_candidate_from_retained_ = false;
    return true;
}

bool ResourceMutationController::acceptCoordinatedMutation() noexcept {
    if (!pending_outcome_ || last_applied_resources_.empty()) return false;
    try {
        auto accepted = accepted_resources_;
        for (const auto& applied : last_applied_resources_) {
            std::string live;
            if (!ResourceActuator::readComparable(applied.first, live) || live != applied.second) return false;
            accepted.erase(std::remove_if(accepted.begin(), accepted.end(),
                [&](const auto& current) { return current.first == applied.first; }), accepted.end());
            accepted.push_back(applied);
        }
        accepted_resources_.swap(accepted);
        pending_outcome_ = false;
        pending_changes_.clear();
        last_applied_resources_.clear();
        last_mutation_path_.clear(); last_mutation_target_.clear(); last_previous_value_.clear();
        last_path_was_dirty_before_ = false; last_candidate_from_retained_ = false;
        refreshRetainedActiveCache();
        return true;
    } catch (...) { return false; }
}

bool ResourceMutationController::retainLastMutation(const DeviceProfile& profile) noexcept {
    if (pending_changes_.size() > 1U) return acceptCoordinatedMutation();
    last_retained_state_failure_stage_ = RetainedStateFailureStage::None;
    try {
        if (!pending_outcome_ || pending_changes_.empty() || retained_store_ == nullptr) {
            last_retained_state_failure_stage_ = RetainedStateFailureStage::Precondition;
            return false;
        }
        auto updated = retained_entries_;
        const std::string scope = retainedScopeFor(profile);
        for (const auto& change : pending_changes_) {
            const auto base = baseline_.find(change.path);
            if (base == baseline_.end() || !base->second.valid || base->second.value == change.target) {
                last_retained_state_failure_stage_ = RetainedStateFailureStage::BaselineInvalid;
                return false;
            }
            std::string live;
            if (!ResourceActuator::readComparable(change.path, live)) {
                last_retained_state_failure_stage_ = RetainedStateFailureStage::LiveReadFailed;
                return false;
            }
            if (live != change.target) {
                last_retained_state_failure_stage_ = RetainedStateFailureStage::LiveTargetMismatch;
                return false;
            }
            updated.erase(std::remove_if(updated.begin(), updated.end(),
                [&](const auto& entry) { return entry.path == change.path; }), updated.end());
            updated.push_back({scope, change.path, base->second.value, change.target});
        }
        if (!retained_store_->commit(updated)) {
            last_retained_state_failure_stage_ = RetainedStateFailureStage::StoreUpsertFailed;
            if (!retained_store_->commit(retained_entries_))
                last_retained_state_failure_stage_ = RetainedStateFailureStage::RecoveryCommitFailed;
            return false;
        }
        retained_entries_.swap(updated);
        for (const auto& change : pending_changes_)
            accepted_resources_.erase(std::remove_if(accepted_resources_.begin(), accepted_resources_.end(),
                [&](const auto& current) { return current.first == change.path; }), accepted_resources_.end());
        pending_changes_.clear();
        pending_outcome_ = false;
        last_applied_resources_.clear();
        last_mutation_path_.clear(); last_mutation_target_.clear(); last_previous_value_.clear();
        last_path_was_dirty_before_ = false; last_candidate_from_retained_ = false;
        refreshRetainedActiveCache();
        return true;
    } catch (...) {
        last_retained_state_failure_stage_ = RetainedStateFailureStage::AllocationFailed;
        return false;
    }
}

bool ResourceMutationController::rollbackLastMutation() noexcept {
    if (!pending_outcome_ || pending_changes_.empty()) return false;
    try {
        // Reverse dependency order. Keep failed/unprocessed members retryable.
        while (!pending_changes_.empty()) {
            const auto& change = pending_changes_.back();
            last_mutation_path_ = change.path;
            last_mutation_target_ = change.target;
            last_previous_value_ = change.previous;
            last_path_was_dirty_before_ = change.was_dirty;
            last_candidate_from_retained_ = change.from_retained;
            pending_outcome_ = true;
            if (!rollbackSingleMutation()) { pending_outcome_ = true; return false; }
            pending_changes_.pop_back();
        }
        pending_outcome_ = false;
        refreshRetainedActiveCache();
        return true;
    } catch (...) { restore_failed_ = true; return false; }
}


bool ResourceMutationController::rollbackSingleMutation() noexcept {
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
        if (baseline != baseline_.end()) baseline->second.valid = false;
        dirty_resources_.erase(last_mutation_path_);
        accepted_resources_.erase(std::remove_if(accepted_resources_.begin(), accepted_resources_.end(),
            [&](const auto& current) { return current.first == last_mutation_path_; }), accepted_resources_.end());
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
        for (const auto& accepted : accepted_resources_) {
            active_resources_cache_.erase(std::remove_if(active_resources_cache_.begin(), active_resources_cache_.end(),
                [&](const auto& current) { return current.first == accepted.first; }), active_resources_cache_.end());
            active_resources_cache_.push_back(accepted);
        }
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
    if (rejected_.empty()) return false;
    try {
        const auto it = rejected_.find(path + "\n" + requested);
        return it != rejected_.end() &&
               cycle_ - it->second < kRejectionCooldownCycles;
    } catch (...) {
        return true;  // fail closed: treat an unreadable record as rejected
    }
}

bool ResourceMutationController::appendPendingTrials(std::vector<TrialMember>& out) const noexcept {
    try {
        for (const auto& change:pending_changes_) {
            const auto base=baseline_.find(change.path);
            if (base==baseline_.end()) return false;
            out.push_back({base->second.domain,change.path,change.previous,change.target});
        }
        return true;
    } catch (...) { return false; }
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
