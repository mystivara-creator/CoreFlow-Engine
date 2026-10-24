#include "coreflow/outcome/outcome.hpp"
#include <cassert>

// Test fixture: the learned noise floor of a device whose signals vary by these
// deadbands. Production code learns these values at runtime from observations.
namespace {
coreflow::NoiseFloor testNoise() {
    coreflow::NoiseFloor n;
    auto set = [&n](coreflow::Signal s, double deadband) {
        n.signals[static_cast<std::size_t>(s)] = {true, 0.0, deadband};
    };
    set(coreflow::Signal::CpuPressure, 0.1);
    set(coreflow::Signal::MemoryPressure, 0.1);
    set(coreflow::Signal::IoPressure, 0.1);
    set(coreflow::Signal::MemoryAvailable, 0.01);
    set(coreflow::Signal::Temperature, 0.5);
    set(coreflow::Signal::ThermalHeadroom, 0.5);
    set(coreflow::Signal::FrameFps, 1.0);
    set(coreflow::Signal::FrameP95, 0.5);
    set(coreflow::Signal::FrameP99, 1.0);
    set(coreflow::Signal::SlowFrameRatio, 0.02);
    set(coreflow::Signal::CpuBusy, 2.0);
    return n;
}
} // namespace


int main() {
    using namespace coreflow;
    OutcomeVerifier verifier;

    SystemObservation before;
    before.memory_valid = true;
    before.memory_available_ratio = 0.30;
    before.temperature_valid = true;
    before.maximum_temperature_c = 43.0;
    before.cpu_pressure_valid = true;
    before.cpu_pressure_some_avg10 = 4.0;
    before.memory_pressure_valid = true;
    before.memory_pressure_some_avg10 = 2.0;
    before.io_pressure_valid = true;
    before.io_pressure_some_avg10 = 5.0;

    auto after = before;
    after.memory_available_ratio = 0.32;
    after.maximum_temperature_c = 42.5;
    after.cpu_pressure_some_avg10 = 3.0;
    after.memory_pressure_some_avg10 = 1.2;
    after.io_pressure_some_avg10 = 3.0;
    auto outcome = verifier.verify(before, after, testNoise());
    assert(outcome.measurable && outcome.improved);
    assert(OutcomePolicy{}.decide(outcome) == Disposition::Retain);

    // A measurable regression in any comparable pressure signal prevents retention.
    after.io_pressure_some_avg10 = 8.0;
    outcome = verifier.verify(before, after, testNoise());
    assert(outcome.measurable && !outcome.improved);
    assert(OutcomePolicy{}.decide(outcome) == Disposition::Rollback);

    SystemObservation absent;
    outcome = verifier.verify(absent, SystemObservation{}, testNoise());
    assert(!outcome.measurable && !outcome.improved);
    assert(OutcomePolicy{}.decide(outcome) == Disposition::Unresolved);

    // Memory-domain verification uses MemAvailable when PSI is not measurable,
    // and independently vetoes retention when available-memory headroom worsens.
    ChangeProposal memory_change;
    memory_change.capability_id = "vm-pressure-control";
    memory_change.domain = "proc_sys.vm";
    memory_change.attribute = "swappiness";
    auto memory_before = SystemObservation{};
    auto memory_after = SystemObservation{};
    memory_before.memory_valid = memory_after.memory_valid = true;
    memory_before.memory_available_ratio = 0.20;
    memory_after.memory_available_ratio = 0.25;
    memory_before.memory_pressure_valid = memory_after.memory_pressure_valid = true;
    memory_before.memory_pressure_some_avg10 = 5.0;
    memory_after.memory_pressure_some_avg10 = 6.0;
    auto memory_noise = testNoise();
    memory_noise.signals[static_cast<std::size_t>(Signal::MemoryPressure)].ready = false;
    auto memory_outcome = verifier.verify(memory_before, memory_after, memory_change, memory_noise);
    assert(memory_outcome.measurable && memory_outcome.improved);
    memory_noise = testNoise();
    memory_before.memory_pressure_some_avg10 = 5.0;
    memory_after.memory_pressure_some_avg10 = 4.0;
    memory_before.memory_available_ratio = 0.30;
    memory_after.memory_available_ratio = 0.10;
    memory_outcome = verifier.verify(memory_before, memory_after, memory_change, memory_noise);
    assert(memory_outcome.measurable && !memory_outcome.improved);

    // CPU-busy fallback permits CPU-domain measurement when kernel PSI is absent.
    ChangeProposal busy_only_cpu_change;
    busy_only_cpu_change.capability_id = "cpu-policy-without-psi";
    busy_only_cpu_change.domain = "sysfs.cpu.cpufreq";
    auto busy_before = SystemObservation{};
    auto busy_after = SystemObservation{};
    busy_before.cpu_valid = busy_after.cpu_valid = true;
    busy_before.cpu_busy_percent = 80.0;
    busy_after.cpu_busy_percent = 60.0;
    auto busy_noise = testNoise();
    busy_noise.signals[static_cast<std::size_t>(Signal::CpuPressure)].ready = false;
    auto busy_outcome = verifier.verify(busy_before, busy_after, busy_only_cpu_change, busy_noise);
    assert(busy_outcome.measurable && busy_outcome.improved);

    // When PSI improves but CPU-busy regresses outside its own noise floor, the
    // independent fallback signal vetoes retention rather than hiding the regression.
    auto mixed_before = busy_before;
    auto mixed_after = busy_after;
    mixed_before.cpu_pressure_valid = mixed_after.cpu_pressure_valid = true;
    mixed_before.cpu_pressure_some_avg10 = 8.0;
    mixed_after.cpu_pressure_some_avg10 = 5.0;
    mixed_before.cpu_busy_percent = 40.0;
    mixed_after.cpu_busy_percent = 80.0;
    busy_noise.signals[static_cast<std::size_t>(Signal::CpuPressure)].ready = true;
    busy_outcome = verifier.verify(mixed_before, mixed_after, busy_only_cpu_change, busy_noise);
    assert(busy_outcome.measurable && !busy_outcome.improved);

    // The candidate-aware verifier must not let unrelated I/O metrics justify a
    // CPU governor change, and a measured thermal regression vetoes retention.
    ChangeProposal cpu_change;
    cpu_change.capability_id = "cpu-policy";
    cpu_change.domain = "sysfs.cpu.cpufreq";
    cpu_change.attribute = "scaling_governor";
    auto cpu_before = before;
    auto cpu_after = after;
    cpu_before.cpu_pressure_valid = true;
    cpu_before.cpu_pressure_some_avg10 = 6.0;
    cpu_after.cpu_pressure_valid = true;
    cpu_after.cpu_pressure_some_avg10 = 4.0;
    cpu_before.maximum_temperature_c = 39.0;
    cpu_after.maximum_temperature_c = 39.2;
    auto cpu_outcome = verifier.verify(cpu_before, cpu_after, cpu_change, testNoise());
    assert(cpu_outcome.measurable && cpu_outcome.improved);
    cpu_after.maximum_temperature_c = 40.0;
    cpu_outcome = verifier.verify(cpu_before, cpu_after, cpu_change, testNoise());
    assert(cpu_outcome.measurable && !cpu_outcome.improved);

    // A meaningful measured thermal rise vetoes retention even when CPU pressure improves.
    cpu_after.maximum_temperature_c = 45.0;
    cpu_outcome = verifier.verify(cpu_before, cpu_after, cpu_change, testNoise());
    assert(cpu_outcome.measurable && !cpu_outcome.improved);

    // A high but stable temperature is not treated as a universal device-independent ceiling.
    cpu_before.maximum_temperature_c = 49.0;
    cpu_after.maximum_temperature_c = 49.2;
    cpu_outcome = verifier.verify(cpu_before, cpu_after, cpu_change, testNoise());
    assert(cpu_outcome.measurable && cpu_outcome.improved);

    cpu_before.frame_timing_valid = cpu_after.frame_timing_valid = true;
    cpu_before.frame_package = "com.example.game";
    cpu_after.frame_package = "com.example.launcher";
    cpu_after.maximum_temperature_c = 39.2;
    cpu_outcome = verifier.verify(cpu_before, cpu_after, cpu_change, testNoise());
    assert(!cpu_outcome.measurable && !cpu_outcome.improved);

    ChangeProposal io_change;
    io_change.capability_id = "block-scheduler";
    io_change.domain = "sysfs.block.queue";
    io_change.attribute = "scheduler";
    auto io_before = before;
    auto io_after = after;
    io_before.io_pressure_valid = true;
    io_before.io_pressure_some_avg10 = 5.0;
    io_after.io_pressure_valid = true;
    io_after.io_pressure_some_avg10 = 7.0;
    io_after.cpu_pressure_valid = true;
    io_after.cpu_pressure_some_avg10 = 0.1; // unrelated CPU improvement
    io_before.cpu_pressure_valid = true;
    io_before.cpu_pressure_some_avg10 = 10.0;
    const auto io_outcome = verifier.verify(io_before, io_after, io_change, testNoise());
    assert(io_outcome.measurable && !io_outcome.improved);

    ChangeProposal render_change;
    render_change.capability_id = "devfreq-governor";
    render_change.domain = "sysfs.devfreq";
    auto render_before = SystemObservation{};
    auto render_after = SystemObservation{};
    render_before.frame_timing_valid = render_after.frame_timing_valid = true;
    render_before.frame_package = render_after.frame_package = "com.example.game";
    render_before.frame_average_fps = render_after.frame_average_fps = 60.0;
    render_before.frame_p95_interval_ms = 17.0;
    render_before.frame_p99_interval_ms = 45.0;
    render_before.slow_frame_ratio = 0.20;
    render_after.frame_p95_interval_ms = 16.5;
    render_after.frame_p99_interval_ms = 20.0;
    render_after.slow_frame_ratio = 0.02;
    const auto render_outcome = verifier.verify(render_before, render_after, render_change, testNoise());
    assert(render_outcome.measurable && render_outcome.improved);
    render_after.frame_package = "com.example.other";
    const auto switched_app_outcome = verifier.verify(render_before, render_after, render_change, testNoise());
    assert(!switched_app_outcome.measurable && !switched_app_outcome.improved);
    return 0;
}
