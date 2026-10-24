#include "coreflow/observation/noise_floor.hpp"
#include "coreflow/optimization/workload.hpp"

#include <cassert>
#include <cmath>
#include <deque>
#include <limits>

namespace {
coreflow::SystemObservation cpuPressure(double value) {
    coreflow::SystemObservation sample;
    sample.cpu_pressure_valid = true;
    sample.cpu_pressure_some_avg10 = value;
    sample.cpu_pressure_resolution = 0.01;
    return sample;
}
} // namespace

int main() {
    using namespace coreflow;

    const auto unready = baselineFromHistory({});
    assert(!unready.ready);

    // Non-finite samples are ignored rather than reaching nth_element, whose
    // ordering assumptions are not valid for NaN.
    const auto filtered = baselineFromHistory(
        {1.0, 1.0, std::numeric_limits<double>::quiet_NaN(),
         std::numeric_limits<double>::infinity(), 1.0}, 0.01);
    assert(filtered.ready);
    assert(filtered.median == 1.0);
    assert(std::abs(filtered.deadband - 0.01) < 1e-12);

    // A single transition must not define the sensor's noise floor. Without a
    // source precision or repeated variation the estimator fails closed.
    const auto unknown_quantum = baselineFromHistory({0.0, 0.0, 5.0});
    assert(!unknown_quantum.ready);

    const auto varying = baselineFromHistory({0.0, 0.1, 0.2, 0.2});
    assert(varying.ready);
    assert(std::abs(varying.median - 0.1) < 1e-12);
    assert(std::abs(varying.deadband - 0.1) < 1e-12);

    // PSI's published decimal precision makes a constant idle history usable.
    // A subsequent pressure step is scored against that prior history and is not
    // inserted back into the baseline that it would otherwise quickly shift.
    NoiseFloorTracker tracker;
    for (int i = 0; i < 6; ++i) tracker.observe(cpuPressure(0.0));
    auto floor = tracker.floor();
    assert(floor[Signal::CpuPressure].ready);
    assert(floor[Signal::CpuPressure].median == 0.0);
    assert(std::abs(floor[Signal::CpuPressure].deadband - 0.01) < 1e-12);

    const auto burst = cpuPressure(5.0);
    tracker.observe(burst);
    floor = tracker.floor();
    assert(floor[Signal::CpuPressure].median == 0.0);
    assert(workloadScoreForDomain("proc_sys.kernel", burst, floor) > 0.0);
    for (int i = 0; i < 48; ++i) tracker.observe(burst);
    floor = tracker.floor();
    assert(floor[Signal::CpuPressure].median == 0.0);
    assert(workloadSignature(burst, floor) == "cpu");

    // A gradual rising workload must not drag the rolling baseline along with it.
    // PSI has two-decimal representation, so each step is a source-observable quantum.
    NoiseFloorTracker ramp_tracker;
    for (int i = 0; i < 6; ++i) ramp_tracker.observe(cpuPressure(0.0));
    for (int step = 1; step <= 40; ++step) {
        ramp_tracker.observe(cpuPressure(static_cast<double>(step) / 100.0));
    }
    const auto ramp_floor = ramp_tracker.floor();
    assert(ramp_floor[Signal::CpuPressure].ready);
    assert(ramp_floor[Signal::CpuPressure].median == 0.0);
    assert(workloadScoreForDomain("proc_sys.kernel", cpuPressure(0.40), ramp_floor) > 0.0);

    // The same protection applies to the adaptive CPU-busy fallback.
    NoiseFloorTracker busy_tracker;
    SystemObservation busy;
    busy.cpu_valid = true;
    busy.cpu_busy_percent = 20.0;
    busy.cpu_busy_resolution = 0.1;
    for (int i = 0; i < 6; ++i) busy_tracker.observe(busy);
    busy.cpu_busy_percent = 90.0;
    busy_tracker.observe(busy);
    const auto busy_floor = busy_tracker.floor();
    assert(busy_floor[Signal::CpuBusy].ready);
    assert(std::abs(busy_floor[Signal::CpuBusy].median - 20.0) < 1e-12);
    assert(workloadScoreForDomain("sysfs.cpu.cpufreq", busy, busy_floor) > 0.0);

    // A falling MemAvailable ratio is adverse too, and it must not be absorbed
    // into its own baseline while the system remains memory constrained.
    NoiseFloorTracker memory_tracker;
    SystemObservation memory;
    memory.memory_valid = true;
    memory.memory_available_ratio = 0.30;
    memory.memory_available_resolution = 0.0001;
    for (int i = 0; i < 6; ++i) memory_tracker.observe(memory);
    memory.memory_available_ratio = 0.10;
    memory_tracker.observe(memory);
    const auto memory_floor = memory_tracker.floor();
    assert(memory_floor[Signal::MemoryAvailable].ready);
    assert(std::abs(memory_floor[Signal::MemoryAvailable].median - 0.30) < 1e-12);
    assert(workloadScoreForDomain("proc_sys.vm", memory, memory_floor) > 0.0);

    // Invalid readings neither enter history nor turn into fake zero pressure.
    memory.memory_available_ratio = std::numeric_limits<double>::quiet_NaN();
    memory_tracker.observe(memory);
    assert(std::abs(memory_tracker.floor()[Signal::MemoryAvailable].median - 0.30) < 1e-12);

    // Constant SurfaceFlinger FPS remains learnable from its published precision.
    NoiseFloorTracker fps_tracker;
    SystemObservation fps;
    fps.frame_timing_valid = true;
    fps.frame_average_fps = 60.0;
    fps.frame_average_fps_resolution = 0.001;
    for (int i = 0; i < 5; ++i) fps_tracker.observe(fps);
    const auto fps_floor = fps_tracker.floor();
    assert(fps_floor[Signal::FrameFps].ready);
    assert(fps_floor[Signal::FrameFps].median == 60.0);
    assert(std::abs(fps_floor[Signal::FrameFps].deadband - 0.001) < 1e-12);

    return 0;
}
