#include "coreflow/policy.hpp"

using namespace coreflow;

static bool expect(bool condition) {
    return condition;
}

int main() {
    AdaptivePolicy policy;
    RuntimeSample sample;

    sample.thermal_available = true;
    sample.thermal_millidegrees = 44000;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 500;
    sample.mem_available_ratio = 0.50;
    sample.load1 = 0.5;
    if (!expect(policy.evaluate(sample, RuntimeState::Normal) == RuntimeState::ThermalGuard)) {
        return 1;
    }

    sample.thermal_millidegrees = 41000;
    if (!expect(policy.evaluate(sample, RuntimeState::ThermalGuard) == RuntimeState::ThermalGuard)) {
        return 1;
    }

    sample.thermal_millidegrees = 40000;
    if (!expect(policy.evaluate(sample, RuntimeState::ThermalGuard) != RuntimeState::ThermalGuard)) {
        return 1;
    }

    sample.thermal_available = false;
    sample.mem_total_kb = 1000;
    sample.mem_available_ratio = 0.05;
    if (!expect(policy.evaluate(sample, RuntimeState::Normal) == RuntimeState::Pressure)) {
        return 1;
    }

    sample.mem_available_ratio = 0.50;
    sample.load1 = 2.0;
    sample.cpu_utilization_available = false;
    if (!expect(policy.evaluate(sample, RuntimeState::Normal) == RuntimeState::Elevated)) {
        return 1;
    }

    // Multicore Android: a high absolute load average must not force
    // ELEVATED when reliable CPU utilization says the CPU is not busy.
    // This is important for making the safety-gated TRIAL window reachable.
    sample.cpu_utilization_available = true;
    sample.cpu_utilization = 0.20;
    sample.load1 = 6.0;
    if (!expect(policy.evaluate(sample, RuntimeState::Normal) == RuntimeState::Normal)) {
        return 1;
    }

    // A genuinely high CPU utilization must still enter ELEVATED even when
    // load average is low.
    sample.cpu_utilization = 0.90;
    sample.load1 = 0.5;
    if (!expect(policy.evaluate(sample, RuntimeState::Normal) == RuntimeState::Elevated)) {
        return 1;
    }

    return 0;
}
