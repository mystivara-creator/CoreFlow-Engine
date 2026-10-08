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
    CHECK(plan.action == PolicyAction::Observe);
    CHECK(plan.mutation_eligible);
    CHECK(plan.candidates.size() == 1);

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
