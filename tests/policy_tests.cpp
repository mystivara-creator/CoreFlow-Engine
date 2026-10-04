#include "coreflow/policy.hpp"

#include <cassert>

using namespace coreflow;

int main() {
    AdaptivePolicy policy;
    RuntimeSample sample;

    sample.thermal_available = true;
    sample.thermal_millidegrees = 44000;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 500;
    sample.mem_available_ratio = 0.50;
    sample.load1 = 0.5;
    assert(policy.evaluate(sample, RuntimeState::Normal) == RuntimeState::ThermalGuard);

    sample.thermal_millidegrees = 41000;
    assert(policy.evaluate(sample, RuntimeState::ThermalGuard) == RuntimeState::ThermalGuard);

    sample.thermal_millidegrees = 40000;
    assert(policy.evaluate(sample, RuntimeState::ThermalGuard) != RuntimeState::ThermalGuard);

    sample.thermal_available = false;
    sample.mem_total_kb = 1000;
    sample.mem_available_ratio = 0.05;
    assert(policy.evaluate(sample, RuntimeState::Normal) == RuntimeState::Pressure);

    sample.mem_available_ratio = 0.50;
    sample.load1 = 2.0;
    assert(policy.evaluate(sample, RuntimeState::Normal) == RuntimeState::Elevated);

    return 0;
}
