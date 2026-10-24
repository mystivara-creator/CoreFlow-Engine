#include "coreflow/optimization/workload.hpp"

#include <cassert>
#include <string>

namespace {
coreflow::NoiseFloor learned(coreflow::Signal signal, double median, double deadband) {
    coreflow::NoiseFloor n;
    n.signals[static_cast<std::size_t>(signal)] = {true, median, deadband};
    return n;
}
} // namespace

int main() {
    using namespace coreflow;

    SystemObservation idle;
    NoiseFloor none;
    assert(workloadSignature(idle, none) == "idle");
    assert(workloadScoreForDomain("sysfs.block.queue", idle, none) == 0.0);
    assert(workloadScoreForDomain("sysfs.generic", idle, none) == 0.0);

    // CPU busy percentage remains an adaptive fallback when kernel PSI is absent.
    SystemObservation cpu_busy;
    cpu_busy.cpu_valid = true;
    cpu_busy.cpu_busy_percent = 90.0;
    NoiseFloor cpu_busy_floor = learned(Signal::CpuBusy, 20.0, 5.0);
    assert(workloadScoreForDomain("sysfs.cpu.cpufreq", cpu_busy, cpu_busy_floor) == 14.0);
    assert(workloadSignature(cpu_busy, cpu_busy_floor) == "cpu");
    cpu_busy.cpu_busy_percent = 24.0;
    assert(workloadScoreForDomain("sysfs.cpu.cpufreq", cpu_busy, cpu_busy_floor) == 0.0);

    // Without a learned floor nothing is elevated: decisions fail closed.
    SystemObservation io;
    io.io_pressure_valid = true;
    io.io_pressure_some_avg10 = 5.0;
    assert(workloadScoreForDomain("sysfs.block.queue", io, none) == 0.0);

    // Elevation is measured against this device's own median and variation.
    const auto io_floor = learned(Signal::IoPressure, 0.0, 1.0);
    assert(workloadScoreForDomain("sysfs.block.queue", io, io_floor) == 5.0);
    assert(workloadScoreForDomain("sysfs.block.iosched", io, io_floor) == 5.0);
    assert(workloadScoreForDomain("sysfs.cpu.cpufreq", io, io_floor) == 0.0);
    assert(workloadSignature(io, io_floor) == "io");

    // A value within the device's own variation is not an elevation.
    io.io_pressure_some_avg10 = 0.9;
    assert(workloadScoreForDomain("sysfs.block.queue", io, io_floor) == 0.0);

    // MemAvailable remains a supported fallback on kernels without memory PSI.
    SystemObservation memory;
    memory.memory_valid = true;
    memory.memory_available_ratio = 0.10;
    const auto memory_floor = learned(Signal::MemoryAvailable, 0.30, 0.02);
    assert(workloadScoreForDomain("proc_sys.vm", memory, memory_floor) > 0.0);
    assert(workloadSignature(memory, memory_floor) == "memory");
    memory.memory_available_ratio = 0.29;
    assert(workloadScoreForDomain("proc_sys.vm", memory, memory_floor) == 0.0);

    // PSI and MemAvailable describe different memory failure modes. The domain
    // takes the stronger dimensionless score, never sums or compares raw units.
    memory.memory_pressure_valid = true;
    memory.memory_pressure_some_avg10 = 0.1;
    auto combined_floor = memory_floor;
    combined_floor.signals[static_cast<std::size_t>(Signal::MemoryPressure)] = {true, 0.0, 1.0};
    memory.memory_available_ratio = 0.10;
    assert(workloadScoreForDomain("proc_sys.vm", memory, combined_floor) > 0.0);
    memory.memory_available_ratio = 0.29;
    memory.memory_pressure_some_avg10 = 8.0;
    assert(workloadScoreForDomain("proc_sys.vm", memory, combined_floor) == 8.0);

    // The same signal is judged against the device's typical level, not a constant.
    const auto busy_floor = learned(Signal::IoPressure, 4.0, 1.0);
    io.io_pressure_some_avg10 = 4.5;
    assert(workloadScoreForDomain("sysfs.block.queue", io, busy_floor) == 0.0);
    io.io_pressure_some_avg10 = 8.0;
    assert(workloadScoreForDomain("sysfs.block.queue", io, busy_floor) == 4.0);

    SystemObservation render;
    render.frame_timing_valid = true;
    render.frame_average_fps = 60.0;
    render.frame_p95_interval_ms = 16.7;
    render.frame_p99_interval_ms = 45.0;
    render.slow_frame_ratio = 0.20;
    NoiseFloor frames = learned(Signal::SlowFrameRatio, 0.0, 0.02);
    assert(workloadScoreForDomain("sysfs.devfreq", render, frames) > 0.0);
    assert(workloadSignature(render, frames) == "frame-pacing");

    render.slow_frame_ratio = 0.01;
    assert(workloadScoreForDomain("sysfs.devfreq", render, frames) == 0.0);
    return 0;
}
