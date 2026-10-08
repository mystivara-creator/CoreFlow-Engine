#include "coreflow/context.hpp"
#include "coreflow/control.hpp"
#include "coreflow/environment.hpp"
#include "coreflow/outcome.hpp"
#include "coreflow/resource_model.hpp"

#include <cstdlib>
#include <iostream>

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { ++failures; std::cerr << "FAIL: " << #x << "\n"; } } while (0)
}

int main() {
    using namespace coreflow;

    EnvironmentCapabilityMatrix matrix;
    matrix.resources.push_back(ResourceCapability{
        ResourceDomain::UClamp, "cpu.uclamp.min", "/sys/fs/cgroup/cpu.uclamp.min",
        true, true, true, true, true, false});
    matrix.resources.push_back(ResourceCapability{
        ResourceDomain::Thermal, "thermal-zones", "/sys/class/thermal",
        true, true, false, true, true, false});

    ResourceStateModel resources;
    resources.reset(matrix);
    CHECK(resources.size() == 2);
    CHECK(resources.observe(ResourceDomain::UClamp, "cpu.uclamp.min", "0"));
    CHECK(resources.setBaseline(ResourceDomain::UClamp, "cpu.uclamp.min", "0"));
    CHECK(resources.setDesired(ResourceDomain::UClamp, "cpu.uclamp.min", "20"));
    CHECK(resources.setVerified(ResourceDomain::UClamp, "cpu.uclamp.min", "0"));
    CHECK(resources.addConstraint(ResourceDomain::UClamp, "cpu.uclamp.min",
                                  {"test", "explicit architecture test", false}));
    CHECK(resources.setMutationPermitted(ResourceDomain::UClamp, "cpu.uclamp.min", true));
    const auto* uclamp = resources.find(ResourceDomain::UClamp, "cpu.uclamp.min");
    CHECK(uclamp != nullptr);
    CHECK(uclamp != nullptr && uclamp->safeForMutation());

    RuntimeSample sample;
    sample.load1 = 0.30;
    sample.cpu_utilization = 0.20;
    sample.cpu_utilization_available = true;
    sample.mem_total_kb = 100000;
    sample.mem_available_kb = 60000;
    sample.mem_available_ratio = 0.60;
    sample.thermal_available = true;
    sample.thermal_millidegrees = 39000;
    sample.confidence = 0.90;

    ContextEngine context_engine;
    const SystemContext context = context_engine.evaluate(sample, RuntimeState::Normal);
    CHECK(context.workload == WorkloadClass::Interactive);
    CHECK(context.mutation_allowed_by_context);

    PolicyEngine policy;
    const PolicyPlan plan = policy.evaluate(context, resources);
    CHECK(plan.action == PolicyAction::Candidate);
    CHECK(plan.mutation_eligible);
    CHECK(plan.candidates.size() == 1);
    CHECK(plan.allows(ResourceDomain::UClamp, "cpu.uclamp.min"));
    CHECK(!plan.allows(ResourceDomain::Thermal, "thermal-zones"));

    const auto* thermal = resources.find(ResourceDomain::Thermal, "thermal-zones");
    CHECK(thermal != nullptr);
    CHECK(!thermal->safeForMutation());

    sample.thermal_millidegrees = 45000;
    const SystemContext hot = context_engine.evaluate(sample, RuntimeState::ThermalGuard);
    CHECK(hot.workload == WorkloadClass::ThermalLimited);
    CHECK(!hot.mutation_allowed_by_context);
    const PolicyPlan held = policy.evaluate(hot, resources);
    CHECK(held.action == PolicyAction::ReduceIntervention);
    CHECK(!held.mutation_eligible);


    // Commit 5: adaptive intervention policy coverage.
    {
        RuntimeSample safe_sample;
        safe_sample.load1 = 0.30;
        safe_sample.cpu_utilization = 0.82;
        safe_sample.cpu_utilization_available = true;
        safe_sample.mem_total_kb = 100000;
        safe_sample.mem_available_kb = 60000;
        safe_sample.mem_available_ratio = 0.60;
        safe_sample.thermal_available = true;
        safe_sample.thermal_millidegrees = 39000;
        safe_sample.confidence = 0.95;

        const SystemContext safe_context =
            context_engine.evaluate(safe_sample, RuntimeState::Normal);

        CHECK(safe_context.mutation_allowed_by_context);

        const PolicyPlan safe_plan =
            policy.evaluate(safe_context, resources);

        CHECK(safe_plan.mutation_eligible);
        CHECK(safe_plan.intervention == InterventionLevel::Moderate ||
              safe_plan.intervention == InterventionLevel::Low);

        RuntimeSample thermal_sample = safe_sample;
        thermal_sample.thermal_millidegrees = 45000;

        const SystemContext thermal_context =
            context_engine.evaluate(
                thermal_sample,
                RuntimeState::ThermalGuard);

        CHECK(!thermal_context.mutation_allowed_by_context);

        const PolicyPlan thermal_plan =
            policy.evaluate(thermal_context, resources);

        CHECK(thermal_plan.action == PolicyAction::ReduceIntervention);
        CHECK(thermal_plan.intervention == InterventionLevel::ObserveOnly);
        CHECK(!thermal_plan.mutation_eligible);

        RuntimeSample unknown_sample = safe_sample;
        unknown_sample.confidence = 0.0;

        SystemContext blocked_context;
        blocked_context.state = RuntimeState::Normal;
        blocked_context.workload = WorkloadClass::Unknown;
        blocked_context.mutation_allowed_by_context = false;
        blocked_context.confidence = 0.0;

        const PolicyPlan blocked_plan =
            policy.evaluate(blocked_context, resources);

        CHECK(blocked_plan.intervention == InterventionLevel::ObserveOnly);
        CHECK(!blocked_plan.mutation_eligible);
    }


    // Commit 7: verify the complete Context -> Policy -> Authority path.
    {
        const MutationAuthority authority;

        const MutationPermit allowed = authority.authorize(
            safe_plan,
            EngineConfig{},
            RuntimeState::Normal,
            safe_context.confidence,
            MutationPermit::Scope::Resource);

        CHECK(allowed.validFor(MutationPermit::Scope::Resource));

        const MutationPermit blocked = authority.authorize(
            held,
            EngineConfig{},
            RuntimeState::ThermalGuard,
            hot.confidence,
            MutationPermit::Scope::Resource);

        CHECK(!blocked.validFor(MutationPermit::Scope::Resource));

        EngineConfig disarmed;
        disarmed.setMutationMode(MutationMode::Adaptive);
        disarmed.setMutationArmed(false);

        const MutationPermit disarmed_permit = authority.authorize(
            safe_plan,
            disarmed,
            RuntimeState::Normal,
            safe_context.confidence,
            MutationPermit::Scope::Resource);

        CHECK(!disarmed_permit.validFor(MutationPermit::Scope::Resource));
    }

    ActuatorResult no_change;
    no_change.status = ActuatorStatus::NoChange;
    CHECK(classifyOutcome(no_change) == OutcomeClass::Neutral);

    ActuatorResult rejected;
    rejected.status = ActuatorStatus::SafetyRejected;
    CHECK(classifyOutcome(rejected) == OutcomeClass::SafetyBlocked);

    std::cout << (failures == 0 ? "CoreFlow architecture foundation: PASS\n"
                                : "CoreFlow architecture foundation: FAIL\n");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
