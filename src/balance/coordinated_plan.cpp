#include "coreflow/coordinated_plan.hpp"
#include "coreflow/decision_trace.hpp"
#include "coreflow/tunable_discovery.hpp"
#include <algorithm>

namespace coreflow {
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
        result.identity.clear(); result.members = 0;
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
        if (!measuredTuningNeed(sample,state)) {
            result.winner="KEEP_CURRENT";
            result.decision_reason="no_measured_tuning_need_keep_current";
            return result;
        }
        const MutationAuthority authority;
        const auto resource_permit=authority.authorize(plan,config,state,sample.confidence,MutationPermit::Scope::Resource);
        const auto cpu_permit=authority.authorize(plan,config,state,sample.confidence,MutationPermit::Scope::CpuFreq);
        if (!resource_permit.valid() && !cpu_permit.valid()) {
            result.decision_reason="authority_or_confidence_blocks_tuning"; return result;
        }
        // Both options are read and scored before either controller may write.
        resources.setCoupledOnly(false);
        auto resource_option=resources.preview(state,sample,profile,config,plan);
        auto cpu_option=cpu.preview(state,sample,profile,config,plan);
        result.funnel=resources.candidateFunnel();
        const bool owns_governor_dependencies=std::any_of(resources.activeResourceSettings().begin(),
            resources.activeResourceSettings().end(),[](const auto& setting) {
                return setting.first.find("/cpufreq/")!=std::string::npos;
            });
        if (owns_governor_dependencies) cpu_option.valid=false;
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
        // Incompatible anchor/dependent alternatives compete; independent
        // profitable options can participate in the same measured configuration.
        if (cpu_option.valid && resource_option.valid) {
            const auto slash=cpu_option.path.find_last_of('/');
            const auto prefix=cpu_option.path.substr(0,slash)+"/";
            if (resource_option.path.starts_with(prefix)) {
                if (cpu_wins) resource_option.valid=false;
                else cpu_option.valid=false;
            }
        }
        if (trace) {
            const auto& selected=cpu_wins ? cpu_option : resource_option;
            const std::string reason="cpu_utility="+std::to_string(result.cpu_utility)+
                " resource_utility="+std::to_string(result.resource_utility)+" keep_current=0";
            DecisionTraceEvent event{"global_plan_selected","planner","SELECTED",reason.c_str()};
            event.target=selected.path.c_str(); event.requested=selected.requested.c_str();
            appendDecisionTraceEvent(*trace,event);
        }
        resources.setConditionDriven(true);
        PolicyPlan execution_plan=plan;
        // A chosen governor is an anchor, so its writes precede dependent files.
        // This execution order never grants it priority in the score comparison.
        if (cpu_option.valid) {
            result.cpu=cpu.applySelected(cpu_option,state,sample,profile,config,plan,cpu_permit,trace);
            if (result.cpu!=MutationResult::Verified) {
                result.decision_reason="selected_cpu_proposal_changed_or_failed"; return result;
            }
            for (const auto& setting:cpu.lastAppliedGovernors()) {
                (void)TunableDiscovery{}.discoverWithin(profile.capabilities,VendorKnowledge{},setting.first+"/"+setting.second);
                const auto slash=setting.first.find_last_of('/');
                if (slash!=std::string::npos)
                    (void)TunableDiscovery{}.discoverWithin(profile.capabilities,VendorKnowledge{},setting.first.substr(0,slash)+"/"+setting.second);
            }
            (void)resources.admitNewBaselines(profile);
            for (const auto& cap:profile.capabilities.resources)
                if (cap.mutation_ready && cap.readable && cap.writable && cap.runtime_verified &&
                    !execution_plan.allows(cap.domain,cap.name))
                    execution_plan.candidates.push_back({cap.domain,cap.name,{},sample.confidence,1.,PolicyAction::Candidate});
        }
        if (resource_option.valid) {
            result.resource=resources.applySelected(resource_option,state,sample,profile,config,
                execution_plan,resource_permit,trace);
            if (result.resource!=MutationResult::Verified) {
                result.decision_reason="selected_resource_proposal_changed_or_failed";
                if (result.cpu==MutationResult::Verified) recover();
                return result;
            }
        } else if (result.cpu==MutationResult::Verified) {
            resources.setCoupledOnly(true);
            result.resource=resources.apply(state,sample,profile,config,execution_plan,resource_permit,trace);
            if (result.resource!=MutationResult::Verified && result.resource!=MutationResult::Skipped) {
                result.decision_reason="dependent_plan_failed"; recover(); return result;
            }
        }
        if (result.resource==MutationResult::Verified)
            for (const auto& cap:resources.discoveredCapabilities())
                if (std::none_of(profile.capabilities.resources.begin(),profile.capabilities.resources.end(),
                    [&](const auto& existing) { return existing.path==cap.path; })) profile.capabilities.resources.push_back(cap);
        if (!result.verified()) return result;
        std::vector<std::string> identities;
        if (result.cpu==MutationResult::Verified) for (const auto& setting : cpu.lastAppliedGovernors())
            identities.push_back("cpufreq:" + setting.first + ":" + setting.second);
        if (result.resource==MutationResult::Verified) for (const auto& setting : resources.lastAppliedResources())
            identities.push_back("resource:" + setting.first + ":" + setting.second);
        std::sort(identities.begin(), identities.end());
        result.members = identities.size();
        if (identities.empty()) { recover(); return result; }
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
