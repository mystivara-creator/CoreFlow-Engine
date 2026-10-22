#include "coreflow/optimization/optimization.hpp"
#include <cassert>
#include <algorithm>
#include <cmath>
#include <vector>
int main() {
    using namespace coreflow;
    Capability a;
    a.id = "a"; a.path = "/virtual/a"; a.raw_value = "10";
    a.numeric_value = 10.0; a.numeric = true; a.writable = true;
    a.range = {0.0, 20.0, true};
    Capability b;
    b.id = "b"; b.path = "/virtual/b"; b.raw_value = "5";
    b.numeric_value = 5.0; b.numeric = true; b.writable = true;
    b.range = {0.0, 10.0, true};
    a.optimization_supported = true;
    b.optimization_supported = true;
    OptimizationEngine blocked_engine;
    ObservationFrame frame;
    Capability unknown;
    unknown.id = "unknown"; unknown.path = "/virtual/unknown";
    unknown.raw_value = "3"; unknown.numeric_value = 3.0;
    unknown.numeric = true; unknown.writable = true;
    unknown.range = {0.0, 8.0, true}; unknown.optimization_supported = false;
    assert(!blocked_engine.propose(std::vector<Capability>{unknown}, frame));
    OptimizationEngine engine;
    const std::vector<Capability> caps{a, b};
    auto first = engine.propose(caps, frame);
    assert(first && first->capability_id == "a");
    assert(std::stod(first->target_value) > a.numeric_value);
    engine.recordOutcome("a", false);
    auto second = engine.propose(caps, frame);
    assert(second && second->capability_id == "b");
    assert(std::stod(second->target_value) > b.numeric_value);
    engine.recordOutcome("b", false);
    auto third = engine.propose(caps, frame);
    assert(third && third->capability_id == "a");
    assert(std::stod(third->target_value) < a.numeric_value);

    // Repeated negative/neutral feedback expands the next bounded search step,
    // rather than endlessly retrying the same tiny delta.
    Capability fractional = a;
    fractional.id = "fractional";
    fractional.raw_value = "10.00";
    fractional.numeric_value = 10.0;
    OptimizationEngine adaptive_step_engine;
    auto small_step = adaptive_step_engine.propose(std::vector<Capability>{fractional}, frame);
    assert(small_step);
    const double initial_delta = std::abs(std::stod(small_step->target_value) - fractional.numeric_value);
    adaptive_step_engine.recordOutcome(fractional.id, false);
    auto expanded_step = adaptive_step_engine.propose(std::vector<Capability>{fractional}, frame);
    assert(expanded_step);
    const double next_delta = std::abs(std::stod(expanded_step->target_value) - fractional.numeric_value);
    assert(next_delta > initial_delta);
    engine.recordOutcome("a", true);
    auto fourth = engine.propose(caps, frame);
    assert(fourth && fourth->capability_id == "b");

    Capability selector;
    selector.id = "block-queue-selector";
    selector.path = "/virtual/queue/scheduler";
    selector.raw_value = "mq-deadline";
    selector.writable = true;
    selector.optimization_supported = true;
    selector.domain = "sysfs.block.queue";
    selector.attribute = "scheduler";
    selector.choices = {"mq-deadline", "bfq", "kyber"};
    OptimizationEngine discrete_engine;
    auto discrete = discrete_engine.propose(std::vector<Capability>{selector}, frame);
    assert(discrete);
    assert(discrete->previous_value == selector.raw_value);
    assert(discrete->target_value != selector.raw_value);
    assert(std::find(discrete->allowed_values.begin(), discrete->allowed_values.end(),
                     discrete->target_value) != discrete->allowed_values.end());
    assert(discrete->allowed_values.size() == selector.choices.size());

    Capability governor = selector;
    governor.id = "cpu-policy-governor";
    governor.path = "/virtual/cpu/scaling_governor";
    governor.domain = "sysfs.cpu.cpufreq";
    governor.attribute = "scaling_governor";
    governor.raw_value = "schedutil";
    governor.choices = {"schedutil", "performance", "powersave"};
    Capability minimum_frequency;
    minimum_frequency.id = "cpu-policy-min-frequency";
    minimum_frequency.path = "/virtual/cpu/scaling_min_freq";
    minimum_frequency.raw_value = "600000";
    minimum_frequency.numeric_value = 600000;
    minimum_frequency.numeric = true;
    minimum_frequency.writable = true;
    minimum_frequency.optimization_supported = true;
    minimum_frequency.range = {300000, 1800000, true};
    minimum_frequency.domain = "sysfs.cpu.cpufreq";
    minimum_frequency.attribute = "scaling_min_freq";
    OptimizationEngine linked_engine;
    const std::vector<Capability> linked_caps{governor, minimum_frequency};
    auto governor_change = linked_engine.propose(linked_caps, frame);
    assert(governor_change && governor_change->capability_id == governor.id);
    linked_engine.recordOutcome(governor.id, governor.domain, governor.attribute, true, true);
    auto followup = linked_engine.propose(linked_caps, frame);
    assert(followup && followup->capability_id == minimum_frequency.id);
    return 0;
}
