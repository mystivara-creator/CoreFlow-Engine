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
    try {
        if (cpu.hasPendingOutcome() || resources.hasPendingOutcome()) return result;
        const MutationAuthority authority;
        const auto resource_permit = authority.authorize(plan, config, state,
            sample.confidence, MutationPermit::Scope::Resource);
        resources.setCoupledOnly(false);
        result.resource = resources.apply(state, sample, profile, config, plan, resource_permit, trace);
        if (trace) appendDecisionTraceEvent(*trace, {"mutation_controller_returned", "resource",
            mutationResultName(result.resource), resources.lastAgentReason().c_str()});
        // A resource failure consumes the cycle; do not cover it with a CPU write.
        const bool owns_governor_dependencies = std::any_of(resources.activeResourceSettings().begin(),
            resources.activeResourceSettings().end(), [](const auto& setting) {
                return setting.first.find("/cpufreq/") != std::string::npos;
            });
        if (result.resource == MutationResult::Verified) {
            for (const auto& cap : resources.discoveredCapabilities())
                if (std::none_of(profile.capabilities.resources.begin(),profile.capabilities.resources.end(),
                    [&](const auto& existing) { return existing.path == cap.path; })) profile.capabilities.resources.push_back(cap);
        }
        if (result.resource == MutationResult::Skipped && !plan.stabilizing_only && !owns_governor_dependencies) {
            const auto cpu_permit = authority.authorize(plan, config, state,
                sample.confidence, MutationPermit::Scope::CpuFreq);
            result.cpu = cpu.apply(state, sample, profile, config, plan, cpu_permit, trace);
            if (trace) appendDecisionTraceEvent(*trace, {"mutation_controller_returned", "cpu_freq",
                mutationResultName(result.cpu), mutationTraceOutcomeReason(result.cpu)});
            if (result.cpu == MutationResult::Verified) {
                // Governor-specific files can appear only after the switch.
                // Read that small subtree, never reset live factory baselines.
                PolicyPlan coupled_plan = plan;
                for (const auto& setting : cpu.lastAppliedGovernors()) {
                    const auto stats = TunableDiscovery{}.discoverWithin(profile.capabilities,
                        VendorKnowledge{}, setting.first + "/" + setting.second);
                    (void)stats;
                    const auto slash = setting.first.find_last_of('/');
                    if (slash != std::string::npos)
                        (void)TunableDiscovery{}.discoverWithin(profile.capabilities, VendorKnowledge{},
                            setting.first.substr(0,slash) + "/" + setting.second);
                }
                (void)resources.admitNewBaselines(profile);
                for (const auto& cap : profile.capabilities.resources) {
                    if (!cap.mutation_ready || !cap.readable || !cap.writable || !cap.runtime_verified ||
                        coupled_plan.allows(cap.domain, cap.name)) continue;
                    coupled_plan.candidates.push_back({cap.domain, cap.name, {}, sample.confidence,
                        1.0, PolicyAction::Candidate});
                }
                resources.setCoupledOnly(true);
                result.resource = resources.apply(state, sample, profile, config, coupled_plan,
                    resource_permit, trace);
                resources.setCoupledOnly(false);
                if (result.resource != MutationResult::Verified && result.resource != MutationResult::Skipped) {
                    recover(); return result;
                }
            }
        }
        if (!result.verified()) return result;
        std::vector<std::string> identities;
        for (const auto& setting : cpu.lastAppliedGovernors())
            identities.push_back("cpufreq:" + setting.first + ":" + setting.second);
        for (const auto& setting : resources.lastAppliedResources())
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
