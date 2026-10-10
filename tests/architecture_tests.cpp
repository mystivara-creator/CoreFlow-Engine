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
        ResourceDomain::Memory, "vm.swappiness", "/proc/sys/vm/swappiness",
        true, true, true, true, true, false});
    matrix.resources.push_back(ResourceCapability{
        ResourceDomain::Thermal, "thermal-zones", "/sys/class/thermal",
        true, true, false, true, true, false});

    ResourceStateModel resources;
    resources.reset(matrix);
    CHECK(resources.size() == 2);
    CHECK(resources.observe(ResourceDomain::Memory, "vm.swappiness", "0"));
    CHECK(resources.setBaseline(ResourceDomain::Memory, "vm.swappiness", "0"));
    CHECK(resources.setDesired(ResourceDomain::Memory, "vm.swappiness", "20"));
    CHECK(resources.setVerified(ResourceDomain::Memory, "vm.swappiness", "0"));
    CHECK(resources.addConstraint(ResourceDomain::Memory, "vm.swappiness",
                                  {"test", "explicit architecture test", false}));
    CHECK(resources.setMutationPermitted(ResourceDomain::Memory, "vm.swappiness", true));
    const auto* uclamp = resources.find(ResourceDomain::Memory, "vm.swappiness");
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
    CHECK(plan.allows(ResourceDomain::Memory, "vm.swappiness"));
    CHECK(!plan.allows(ResourceDomain::Thermal, "thermal-zones"));

    ResourceStateModel no_resources;
    const PolicyPlan no_candidate_plan = policy.evaluate(context, no_resources);
    CHECK(no_candidate_plan.action == PolicyAction::Hold);
    CHECK(!no_candidate_plan.mutation_eligible);
    CHECK(no_candidate_plan.candidates.empty());

    const auto* thermal = resources.find(ResourceDomain::Thermal, "thermal-zones");
    CHECK(thermal != nullptr);
    CHECK(!thermal->safeForMutation());

    sample.thermal_millidegrees = 45000;
    const SystemContext hot = context_engine.evaluate(sample, RuntimeState::ThermalGuard);
    CHECK(hot.workload == WorkloadClass::ThermalLimited);
    CHECK(!hot.mutation_allowed_by_context);
    // Stress is graded, not binary. With trustworthy telemetry the policy allows
    // small load-reducing writes (stabilizing_only); it never allows more.
    CHECK(hot.stabilizing_allowed_by_context);
    const PolicyPlan held = policy.evaluate(hot, resources);
    CHECK(held.action == PolicyAction::Candidate);
    CHECK(held.mutation_eligible);
    CHECK(held.stabilizing_only);
    CHECK(held.intervention == InterventionLevel::Low);

    // Untrustworthy telemetry under stress remains a hard hold.
    RuntimeSample unsure = sample;
    unsure.confidence = 0.20;
    const SystemContext unsure_hot =
        context_engine.evaluate(unsure, RuntimeState::ThermalGuard);
    CHECK(!unsure_hot.stabilizing_allowed_by_context);
    const PolicyPlan unsure_plan = policy.evaluate(unsure_hot, resources);
    CHECK(unsure_plan.action == PolicyAction::ReduceIntervention);
    CHECK(!unsure_plan.mutation_eligible);
    CHECK(!unsure_plan.stabilizing_only);


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

        CHECK(thermal_plan.action == PolicyAction::Candidate);
        CHECK(thermal_plan.intervention == InterventionLevel::Low);
        CHECK(thermal_plan.mutation_eligible);
        CHECK(thermal_plan.stabilizing_only);

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

        EngineConfig enabled;
        enabled.setMutationMode(MutationMode::Adaptive);
        enabled.setMutationArmed(true);

        const MutationPermit allowed = authority.authorize(
            plan,
            enabled,
            RuntimeState::Normal,
            context.confidence,
            MutationPermit::Scope::Resource);

        CHECK(allowed.validFor(MutationPermit::Scope::Resource));

        // A stabilizing plan may start a resource write in ThermalGuard...
        const MutationPermit stabilizing = authority.authorize(
            held,
            enabled,
            RuntimeState::ThermalGuard,
            hot.confidence,
            MutationPermit::Scope::Resource);
        CHECK(stabilizing.validFor(MutationPermit::Scope::Resource));

        // ...but never the CPU governor, even when the operator allowed it.
        EngineConfig cpu_enabled = enabled;
        cpu_enabled.setAllowCpuGovernor(true);
        CHECK(!authority.authorize(held, cpu_enabled, RuntimeState::ThermalGuard,
                                   hot.confidence, MutationPermit::Scope::CpuFreq)
                   .valid());
        CHECK(!authority.authorize(held, cpu_enabled, RuntimeState::Normal,
                                   hot.confidence, MutationPermit::Scope::CpuFreq)
                   .valid());

        // A plan that is not stabilizing gets no permit in a safety state.
        PolicyPlan plain;
        plain.action = PolicyAction::Candidate;
        plain.mutation_eligible = true;
        CHECK(!authority.authorize(plain, enabled, RuntimeState::ThermalGuard, 1.0,
                                   MutationPermit::Scope::Resource).valid());
        CHECK(!authority.authorize(plain, enabled, RuntimeState::Pressure, 1.0,
                                   MutationPermit::Scope::Resource).valid());
        CHECK(!authority.authorize(unsure_plan, enabled, RuntimeState::ThermalGuard,
                                   unsure.confidence,
                                   MutationPermit::Scope::Resource).valid());

        EngineConfig disarmed;
        disarmed.setMutationMode(MutationMode::Adaptive);
        disarmed.setMutationArmed(false);

        const MutationPermit disarmed_permit = authority.authorize(
            plan,
            disarmed,
            RuntimeState::Normal,
            context.confidence,
            MutationPermit::Scope::Resource);

        CHECK(!disarmed_permit.validFor(MutationPermit::Scope::Resource));
    }

    ActuatorResult no_change;
    no_change.status = ActuatorStatus::NoChange;
    CHECK(classifyOutcome(no_change) == OutcomeClass::Neutral);

    ActuatorResult rejected;
    rejected.status = ActuatorStatus::SafetyRejected;
    CHECK(classifyOutcome(rejected) == OutcomeClass::SafetyBlocked);

    // v2.1.1 daemon epoch rule: when must an active mutation be restored now?
    {
        EpochRestoreInputs in;
        in.state = RuntimeState::ThermalGuard;
        in.mode_adaptive = true;
        in.armed = true;
        in.resource_epoch_stabilizing = true;
        in.plan_stabilizing_only = true;
        in.plan_mutation_eligible = true;
        // The one case that is held through stress.
        CHECK(!shouldRestoreEpochNow(in));
        in.state = RuntimeState::Pressure;
        CHECK(!shouldRestoreEpochNow(in));

        // Every deviation restores.
        EpochRestoreInputs e = in;  e.cpu_mutated = true;
        CHECK(shouldRestoreEpochNow(e));
        e = in;  e.resource_epoch_stabilizing = false;
        CHECK(shouldRestoreEpochNow(e));
        e = in;  e.plan_stabilizing_only = false;
        CHECK(shouldRestoreEpochNow(e));
        e = in;  e.plan_mutation_eligible = false;   // e.g. confidence dropped
        CHECK(shouldRestoreEpochNow(e));
        e = in;  e.armed = false;                    // disarmed / kill switch
        CHECK(shouldRestoreEpochNow(e));
        e = in;  e.mode_adaptive = false;            // SAFE_MODE disables the config
        CHECK(shouldRestoreEpochNow(e));
        e = in;  e.state = RuntimeState::Idle;
        CHECK(shouldRestoreEpochNow(e));

        // Outside stress the epoch runs its normal window; disarm still restores.
        e = in;  e.state = RuntimeState::Normal;
        CHECK(!shouldRestoreEpochNow(e));
        e.armed = false;
        CHECK(shouldRestoreEpochNow(e));
    }

    // v2.1.1 graded context gate.
    {
        RuntimeSample base;
        base.load1 = 1.0;
        base.cpu_utilization = 0.50;
        base.cpu_utilization_available = true;
        base.mem_total_kb = 100000;
        base.mem_available_kb = 60000;
        base.mem_available_ratio = 0.60;
        base.thermal_available = true;
        base.thermal_millidegrees = 39000;
        base.confidence = 0.95;

        // Heavy I/O no longer blocks tuning.
        RuntimeSample heavy_io = base;
        heavy_io.io_activity_available = true;
        heavy_io.io_read_kb_per_sec = 150000.0;
        const SystemContext io_ctx = context_engine.evaluate(heavy_io, RuntimeState::Normal);
        CHECK(!io_ctx.io_headroom);
        CHECK(io_ctx.mutation_allowed_by_context);

        // Low memory headroom: proactive tuning waits, stabilization is allowed.
        RuntimeSample low_mem = base;
        low_mem.mem_available_ratio = 0.10;
        low_mem.mem_available_kb = 10000;
        const SystemContext mem_ctx = context_engine.evaluate(low_mem, RuntimeState::Normal);
        CHECK(!mem_ctx.mutation_allowed_by_context);
        CHECK(mem_ctx.stabilizing_allowed_by_context);

        // Pressure is stressed too.
        const SystemContext pressure_ctx =
            context_engine.evaluate(low_mem, RuntimeState::Pressure);
        CHECK(!pressure_ctx.mutation_allowed_by_context);
        CHECK(pressure_ctx.stabilizing_allowed_by_context);

        // Idle never tunes; low confidence never tunes, stressed or not.
        const SystemContext idle_ctx = context_engine.evaluate(base, RuntimeState::Idle);
        CHECK(!idle_ctx.mutation_allowed_by_context);
        CHECK(!idle_ctx.stabilizing_allowed_by_context);
        const SystemContext idle_hot = context_engine.evaluate(low_mem, RuntimeState::Idle);
        CHECK(!idle_hot.stabilizing_allowed_by_context);

        RuntimeSample weak = low_mem;
        weak.confidence = 0.40;
        const SystemContext weak_ctx = context_engine.evaluate(weak, RuntimeState::Pressure);
        CHECK(!weak_ctx.mutation_allowed_by_context);
        CHECK(!weak_ctx.stabilizing_allowed_by_context);

        // A comfortable system is not "stressed": no stabilization path.
        const SystemContext calm = context_engine.evaluate(base, RuntimeState::Normal);
        CHECK(calm.mutation_allowed_by_context);
        CHECK(!calm.stabilizing_allowed_by_context);
    }

    std::cout << (failures == 0 ? "CoreFlow architecture foundation: PASS\n"
                                : "CoreFlow architecture foundation: FAIL\n");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
