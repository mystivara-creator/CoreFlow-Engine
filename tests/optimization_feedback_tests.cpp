#include "coreflow/optimization/optimization.hpp"

#include <cassert>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

int main() {
    using namespace coreflow;
    ObservationFrame io_frame;
    io_frame.system.io_pressure_valid = true;
    io_frame.system.io_pressure_some_avg10 = 5.0;

    Capability a;
    a.id = "a"; a.path = "/virtual/queue/a"; a.raw_value = "10";
    a.numeric_value = 10.0; a.numeric = true; a.writable = true;
    a.range = {0.0, 20.0, true}; a.optimization_supported = true;
    a.domain = "sysfs.block.queue"; a.attribute = "read_ahead_kb";
    Capability b = a;
    b.id = "b"; b.path = "/virtual/queue/b"; b.raw_value = "5";
    b.numeric_value = 5.0; b.range = {0.0, 10.0, true}; b.attribute = "nr_requests";

    OptimizationEngine blocked_engine;
    Capability unknown = a;
    unknown.id = "unknown"; unknown.optimization_supported = false;
    assert(!blocked_engine.propose(std::vector<Capability>{unknown}, io_frame));

    // The outcome-aware API must not train direction/step size when telemetry
    // is inconclusive. Compare against an engine that received no feedback.
    {
        OptimizationEngine inconclusive_engine;
        OptimizationEngine untouched_engine;
        const std::vector<Capability> single_capability{a};
        const auto uncertain = inconclusive_engine.propose(single_capability, io_frame);
        const auto control = untouched_engine.propose(single_capability, io_frame);
        assert(uncertain && control);
        Outcome no_measurement;
        no_measurement.measurable = false;
        no_measurement.improved = false;
        inconclusive_engine.recordOutcome(
            *uncertain, no_measurement, Disposition::Unresolved, io_frame);
        const auto next_uncertain = inconclusive_engine.propose(single_capability, io_frame);
        const auto next_control = untouched_engine.propose(single_capability, io_frame);
        assert(next_uncertain && next_control);
        assert(next_uncertain->target_value == next_control->target_value);
    }

    OptimizationEngine engine;
    const std::vector<Capability> caps{a, b};
    auto first = engine.propose(caps, io_frame);
    assert(first && first->capability_id == "a");
    assert(std::stod(first->target_value) > a.numeric_value);
    engine.recordOutcome(*first, false, false, io_frame);
    auto second = engine.propose(caps, io_frame);
    assert(second && second->capability_id == "b");
    assert(std::stod(second->target_value) > b.numeric_value);
    engine.recordOutcome(*second, false, false, io_frame);
    auto third = engine.propose(caps, io_frame);
    assert(third && third->capability_id == "a");
    assert(std::stod(third->target_value) < a.numeric_value);

    Capability fractional = a;
    fractional.id = "fractional";
    fractional.raw_value = "10.00";
    fractional.numeric_value = 10.0;
    OptimizationEngine adaptive_step_engine;
    auto small_step = adaptive_step_engine.propose(std::vector<Capability>{fractional}, io_frame);
    assert(small_step);
    const double initial_delta = std::abs(std::stod(small_step->target_value) - fractional.numeric_value);
    adaptive_step_engine.recordOutcome(*small_step, false, false, io_frame);
    auto expanded_step = adaptive_step_engine.propose(std::vector<Capability>{fractional}, io_frame);
    assert(expanded_step);
    const double next_delta = std::abs(std::stod(expanded_step->target_value) - fractional.numeric_value);
    assert(next_delta > initial_delta);

    // A discrete choice cannot be cycled without relevant pressure evidence and
    // an untested kernel-advertised alternative.
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
    auto discrete = discrete_engine.propose(std::vector<Capability>{selector}, io_frame);
    assert(discrete);
    assert(discrete->previous_value == selector.raw_value);
    assert(discrete->target_value != selector.raw_value);
    assert(std::find(discrete->allowed_values.begin(), discrete->allowed_values.end(),
                     discrete->target_value) != discrete->allowed_values.end());

    // Scoring multiple selectors must not consume alternatives belonging to the
    // selector that was not actually returned. The second queue should receive
    // its first untested kernel-advertised alternative on the next round.
    Capability second_selector = selector;
    second_selector.id = "second-block-queue-selector";
    second_selector.path = "/virtual/queue/second/scheduler";
    OptimizationEngine independent_selector_engine;
    const std::vector<Capability> selectors{selector, second_selector};
    auto first_selector = independent_selector_engine.propose(selectors, io_frame);
    assert(first_selector && first_selector->capability_id == selector.id);
    auto second_selector_proposal = independent_selector_engine.propose(selectors, io_frame);
    assert(second_selector_proposal && second_selector_proposal->capability_id == second_selector.id);
    assert(second_selector_proposal->target_value == "bfq");

    // Once a kernel choice is retained for the current workload, it is no longer
    // churned back and forth on every daemon cycle.
    ObservationFrame after_io = io_frame;
    discrete_engine.recordOutcome(*discrete, true, true, after_io);
    assert(!discrete_engine.propose(std::vector<Capability>{selector}, io_frame));

    Capability governor = selector;
    governor.id = "cpu-policy-governor";
    governor.path = "/virtual/cpu/policy0/scaling_governor";
    governor.domain = "sysfs.cpu.cpufreq";
    governor.attribute = "scaling_governor";
    governor.raw_value = "schedutil";
    governor.choices = {"schedutil", "performance", "powersave"};
    Capability minimum_frequency;
    minimum_frequency.id = "cpu-policy-min-frequency";
    minimum_frequency.path = "/virtual/cpu/policy0/scaling_min_freq";
    minimum_frequency.raw_value = "600000";
    minimum_frequency.numeric_value = 600000;
    minimum_frequency.numeric = true;
    minimum_frequency.writable = true;
    minimum_frequency.optimization_supported = true;
    minimum_frequency.range = {300000, 1800000, true};
    minimum_frequency.domain = "sysfs.cpu.cpufreq";
    minimum_frequency.attribute = "scaling_min_freq";
    ObservationFrame cpu_frame;
    cpu_frame.system.cpu_pressure_valid = true;
    cpu_frame.system.cpu_pressure_some_avg10 = 5.0;
    OptimizationEngine linked_engine;
    Capability maximum_frequency = minimum_frequency;
    maximum_frequency.id = "cpu-policy-max-frequency";
    maximum_frequency.path = "/virtual/cpu/policy0/scaling_max_freq";
    maximum_frequency.attribute = "scaling_max_freq";
    maximum_frequency.raw_value = "1800000";
    maximum_frequency.numeric_value = 1800000.0;
    const std::vector<Capability> linked_caps{governor, minimum_frequency, maximum_frequency};
    auto governor_change = linked_engine.propose(linked_caps, cpu_frame);
    assert(governor_change && governor_change->capability_id == governor.id);
    linked_engine.recordOutcome(*governor_change, true, true, cpu_frame);
    auto followup = linked_engine.propose(linked_caps, cpu_frame);
    assert(followup && followup->capability_id == minimum_frequency.id);


    // Frequencies exposed by the kernel's available-frequency list are discrete
    // controls too; a retained governor change can trigger a same-policy follow-up.
    Capability governor2 = governor;
    governor2.raw_value = "schedutil";
    governor2.choices = {"schedutil", "performance", "powersave"};
    Capability minimum_frequency_choices = minimum_frequency;
    minimum_frequency_choices.choices = {"300000", "600000", "900000", "1200000"};
    minimum_frequency_choices.raw_value = "600000";
    OptimizationEngine frequency_link_engine;
    const std::vector<Capability> linked_discrete_caps{governor2, minimum_frequency_choices};
    auto governor_choice = frequency_link_engine.propose(linked_discrete_caps, cpu_frame);
    assert(governor_choice && governor_choice->capability_id == governor2.id);
    frequency_link_engine.recordOutcome(*governor_choice, true, true, cpu_frame);
    auto frequency_followup = frequency_link_engine.propose(linked_discrete_caps, cpu_frame);
    assert(frequency_followup && frequency_followup->capability_id == minimum_frequency_choices.id);
    assert(std::find(frequency_followup->allowed_values.begin(), frequency_followup->allowed_values.end(),
                     frequency_followup->target_value) != frequency_followup->allowed_values.end());

    // An unmeasurable numeric proposal is not retried every daemon cycle for the
    // same workload. A materially different pressure class may be tested separately.
    OptimizationEngine deferred_engine;
    auto deferred = deferred_engine.propose(std::vector<Capability>{a}, io_frame);
    assert(deferred);
    deferred_engine.defer(*deferred, io_frame);
    assert(!deferred_engine.propose(std::vector<Capability>{a}, io_frame));

    // Two consecutive conclusive non-improvements stop further probes for the
    // same capability/workload instead of creating unbounded mutation churn.
    OptimizationEngine bounded_probe_engine;
    auto probe_one = bounded_probe_engine.propose(std::vector<Capability>{a}, io_frame);
    assert(probe_one && std::stod(probe_one->target_value) > a.numeric_value);
    bounded_probe_engine.recordOutcome(*probe_one, false, false, io_frame);
    auto probe_two = bounded_probe_engine.propose(std::vector<Capability>{a}, io_frame);
    assert(probe_two && std::stod(probe_two->target_value) < a.numeric_value);
    bounded_probe_engine.recordOutcome(*probe_two, false, false, io_frame);
    assert(!bounded_probe_engine.propose(std::vector<Capability>{a}, io_frame));

    // Unmeasurable contexts are bounded. Once older contexts are evicted, a
    // previously deferred context can be evaluated again instead of retaining
    // a permanent per-workload tombstone.
    {
        OptimizationEngine bounded_context_engine;
        auto first_context = bounded_context_engine.propose(std::vector<Capability>{a}, io_frame);
        assert(first_context);
        bounded_context_engine.defer(*first_context, io_frame);
        assert(!bounded_context_engine.propose(std::vector<Capability>{a}, io_frame));

        for (std::size_t i = 0; i < 512; ++i) {
            Capability other = a;
            other.id = "bounded-context-" + std::to_string(i);
            other.path = "/virtual/queue/bounded-" + std::to_string(i);
            auto candidate = bounded_context_engine.propose(std::vector<Capability>{other}, io_frame);
            assert(candidate);
            bounded_context_engine.defer(*candidate, io_frame);
        }
        assert(bounded_context_engine.propose(std::vector<Capability>{a}, io_frame));
    }

    // Capability-level legacy feedback and discrete-choice rotation share a
    // bounded history. Eviction resets only the old round-robin cursor, without
    // affecting choice exclusions scoped to a different workload signature.
    {
        OptimizationEngine bounded_capability_engine;
        auto first_choice = bounded_capability_engine.propose(
            std::vector<Capability>{selector}, io_frame);
        assert(first_choice && first_choice->target_value == "bfq");

        ObservationFrame mixed_frame = io_frame;
        mixed_frame.system.cpu_pressure_valid = true;
        mixed_frame.system.cpu_pressure_some_avg10 = 2.0;
        for (std::size_t i = 0; i < 512; ++i) {
            bounded_capability_engine.recordOutcome(
                "bounded-capability-" + std::to_string(i), true);
        }

        auto after_eviction = bounded_capability_engine.propose(
            std::vector<Capability>{selector}, mixed_frame);
        assert(after_eviction && after_eviction->target_value == "bfq");
    }

    // Coupled frequency bounds must never be proposed across the sibling bound.
    Capability min_freq = minimum_frequency;
    min_freq.raw_value = "600000"; min_freq.numeric_value = 600000.0;
    Capability max_freq = min_freq;
    max_freq.id = "cpu-policy-max-frequency";
    max_freq.path = "/virtual/cpu/policy0/scaling_max_freq";
    max_freq.attribute = "scaling_max_freq";
    max_freq.raw_value = "900000"; max_freq.numeric_value = 900000.0;
    max_freq.range = {300000, 1800000, true};
    OptimizationEngine bounds_engine;
    auto min_proposal = bounds_engine.propose(std::vector<Capability>{min_freq, max_freq}, cpu_frame);
    if (min_proposal && min_proposal->capability_id == min_freq.id)
        assert(std::stod(min_proposal->target_value) <= max_freq.numeric_value);
    if (min_proposal && min_proposal->capability_id == max_freq.id)
        assert(std::stod(min_proposal->target_value) >= min_freq.numeric_value);

    // Learning for one workload must not change the initial direction for another.
    ObservationFrame memory_frame;
    memory_frame.system.memory_pressure_valid = true;
    memory_frame.system.memory_pressure_some_avg10 = 5.0;
    OptimizationEngine isolated_learning_engine;
    auto io_probe = isolated_learning_engine.propose(std::vector<Capability>{a}, io_frame);
    assert(io_probe);
    isolated_learning_engine.recordOutcome(*io_probe, false, false, io_frame);
    auto memory_probe = isolated_learning_engine.propose(std::vector<Capability>{a}, memory_frame);
    // The capability is I/O-domain scoped, so memory pressure cannot make it eligible.
    assert(!memory_probe);

    // Workload relevance is mandatory: a generic sysfs resource cannot be changed
    // merely because the kernel exposes a numerical range.
    Capability generic = a;
    generic.domain = "sysfs.generic";
    OptimizationEngine generic_engine;
    assert(!generic_engine.propose(std::vector<Capability>{generic}, io_frame));

    ObservationFrame idle;
    assert(!generic_engine.propose(caps, idle));
    return 0;
}
