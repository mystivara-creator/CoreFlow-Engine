#include "coreflow/coordinated_plan.hpp"
#include "coreflow/decision_trace.hpp"
#include "coreflow/tunable_discovery.hpp"
#include <algorithm>

namespace coreflow {
static_assert(1U+ResourceMutationController::kMaxCohortWrites<=4U,"Trial history reservation must cover every cohort member");
bool rollbackCoordinatedPlan(MutationController& cpu, ResourceMutationController& resources) noexcept {
    if (resources.hasPendingOutcome() && !resources.rollbackLastMutation()) return false;
    return !cpu.hasPendingOutcome() || cpu.rollbackLastMutation();
}
bool restoreCoordinatedPlan(MutationController& cpu, ResourceMutationController& resources) noexcept {
    return resources.restoreAll() && cpu.restoreAll();
}
CoordinatedResult executeCoordinatedPlan(MutationController& cpu,
    ResourceMutationController& resources, RuntimeState state, const RuntimeSample& sample,
    DeviceProfile& profile, const EngineConfig& config, const PolicyPlan& plan,
    const DecisionTraceContext* trace) noexcept {
    CoordinatedResult result;
    const auto recover = [&]() noexcept {
        const bool rolled = rollbackCoordinatedPlan(cpu,resources) || restoreCoordinatedPlan(cpu,resources);
        result.resource = result.cpu = rolled ? MutationResult::RolledBack : MutationResult::Failed;
        result.identity.clear(); result.members = 0; result.trials.clear();
        resources.setCoupledOnly(false);
    };
    struct SelectionScope {
        ResourceMutationController& resources;
        ~SelectionScope() { resources.setCoupledOnly(false); resources.setConditionDriven(false); }
    } scope{resources};
    try {
        if (cpu.hasPendingOutcome() || resources.hasPendingOutcome()) {
            result.decision_reason="outcome_measurement_pending"; return result;
        }
        // Recovery is a prerequisite, never a competing tuning proposal.
        if (!resources.recoveryReady()) (void)resources.captureBaseline(profile);
        if (!resources.recoveryReady()) { result.decision_reason="resource_recovery_pending"; return result; }
        if (!cpu.recoveryReady()) (void)cpu.captureBaseline(profile);
        if (!cpu.recoveryReady()) { result.decision_reason="cpu_recovery_pending"; return result; }
        result.funnel.discovered=profile.capabilities.resources.size();
        const MutationAuthority authority;
        const auto resource_permit=authority.authorize(plan,config,state,sample.confidence,MutationPermit::Scope::Resource);
        const auto cpu_permit=authority.authorize(plan,config,state,sample.confidence,MutationPermit::Scope::CpuFreq);
        // Both options are read and scored before either controller may write.
        resources.setCoupledOnly(false);
        auto resource_option=resources.preview(state,sample,profile,config,plan);
        auto cpu_option=cpu.preview(state,sample,profile,config,plan);
        result.funnel=resources.candidateFunnel();
        result.cpu_reason=cpu_option.reason; result.resource_reason=resource_option.reason;
        if (!measuredTuningNeed(sample,state)) {
            result.winner="KEEP_CURRENT";
            result.decision_reason="no_measured_tuning_need_keep_current"; return result;
        }
        if (!resource_permit.valid() && !cpu_permit.valid()) {
            result.decision_reason="authority_or_confidence_blocks_tuning"; return result;
        }
        const bool owns_governor_dependencies=std::any_of(resources.activeResourceSettings().begin(),
            resources.activeResourceSettings().end(),[](const auto& setting) {
                return setting.first.find("/cpufreq/")!=std::string::npos;
            });
        if (owns_governor_dependencies) {
            cpu_option.valid=false; result.cpu_reason="owned_governor_dependencies_must_settle_before_switch";
        }
        result.cpu_utility=cpu_option.valid ? cpu_option.utility : 0.;
        result.resource_utility=resource_option.valid ? resource_option.utility : 0.;
        if (!cpu_option.valid && !resource_option.valid) {
            result.winner="KEEP_CURRENT";
            result.decision_reason=owns_governor_dependencies ? "no_better_safe_plan_dependencies_held"
                : "measured_need_but_no_admissible_gain";
            return result;
        }
        const bool cpu_wins=cpu_option.valid && (!resource_option.valid ||
            cpu_option.utility>resource_option.utility ||
            (cpu_option.utility==resource_option.utility &&
             (cpu_option.risk<resource_option.risk ||
              (cpu_option.risk==resource_option.risk && cpu_option.path<resource_option.path))));
        result.winner=cpu_wins ? "CPU_GOVERNOR" : resourceDomainName(resource_option.domain);
        result.decision_reason="predicted_gain_selected_across_domains";
        if (trace) {
            const auto& selected=cpu_wins ? cpu_option : resource_option;
            const std::string reason="cpu_utility="+std::to_string(result.cpu_utility)+
                " resource_utility="+std::to_string(result.resource_utility)+" keep_current=0";
            DecisionTraceEvent event{"global_plan_selected","planner","SELECTED",reason.c_str()};
            event.target=selected.path.c_str(); event.requested=selected.requested.c_str();
            appendDecisionTraceEvent(*trace,event);
        }
        // Single-variable causal trial: never write a governor and a resource
        // in the same outcome window. A read-back verifies the value, not its
        // performance benefit. Re-observe and measure the winner in the next
        // epoch before admitting any dependent mutation.
        if (cpu_option.valid && resource_option.valid) {
            if (cpu_wins) resource_option.valid = false;
            else cpu_option.valid = false;
        }
        resources.setConditionDriven(true);
        if (cpu_option.valid) {
            result.cpu=cpu.applySelected(cpu_option,state,sample,profile,config,plan,cpu_permit,trace);
            if (result.cpu!=MutationResult::Verified) {
                result.decision_reason="selected_cpu_proposal_changed_or_failed";
                if (result.cpu!=MutationResult::Skipped)
                    result.trials.push_back({ResourceDomain::CpuFreq,cpu_option.path,cpu_option.current,cpu_option.requested});
                return result;
            }
            // Refresh governor-dependent inventory only after measured acceptance.
        }
        if (resource_option.valid) {
            result.resource=resources.applySelected(resource_option,state,sample,profile,config,
                plan,resource_permit,trace);
            if (result.resource!=MutationResult::Verified) {
                result.decision_reason="selected_resource_proposal_changed_or_failed";
                if (result.resource!=MutationResult::Skipped)
                    result.trials.push_back({resource_option.domain,resource_option.path,resource_option.current,resource_option.requested});
                return result;
            }
        }
        // Governor-only trial. Dependent resource tuning must wait for
        // verified measured outcome and a fresh device sample.
        if (result.resource==MutationResult::Verified)
            for (const auto& cap:resources.discoveredCapabilities())
                if (std::none_of(profile.capabilities.resources.begin(),profile.capabilities.resources.end(),
                    [&](const auto& existing) { return existing.path==cap.path; })) profile.capabilities.resources.push_back(cap);
        if (!result.verified()) return result;
        result.trials.reserve(1U+ResourceMutationController::kMaxCohortWrites);
        if (!cpu.appendPendingTrials(result.trials) || !resources.appendPendingTrials(result.trials)) {
            result.decision_reason="trial_metadata_capture_failed"; recover(); return result;
        }
        std::vector<std::string> identities;
        if (result.cpu==MutationResult::Verified) for (const auto& setting : cpu.lastAppliedGovernors())
            identities.push_back("cpufreq:" + setting.first + ":" + setting.second);
        if (result.resource==MutationResult::Verified) for (const auto& setting : resources.lastAppliedResources())
            identities.push_back("resource:" + setting.first + ":" + setting.second);
        std::sort(identities.begin(), identities.end());
        result.members = identities.size();
        if (identities.size()!=1U || identities.size()!=result.trials.size()) { recover(); return result; }
        if (identities.size() == 1U) result.identity = identities.front();
        else {
            result.identity = "cohort";
            for (const auto& identity : identities)
                result.identity += ":" + std::to_string(identity.size()) + ":" + identity;
        }
        // Fits the advisory memory record bound; no anonymous experiment.
        if (result.identity.size() > 4096U) { recover(); return result; }
        if (trace) appendDecisionTraceEvent(*trace, {"coordinated_plan_verified", "cohort", "VERIFIED",
            result.identity.c_str()});
        return result;
    } catch (...) {
        recover(); return result;
    }
}
} // namespace coreflow
