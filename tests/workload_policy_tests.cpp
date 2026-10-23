#include "coreflow/optimization/workload.hpp"

#include <cassert>
#include <string>

int main() {
    using namespace coreflow;

    SystemObservation idle;
    assert(workloadSignature(idle) == "idle");
    assert(workloadScoreForDomain("sysfs.block.queue", idle) == 0.0);
    assert(workloadScoreForDomain("sysfs.generic", idle) == 0.0);

    SystemObservation memory;
    memory.memory_valid = true;
    memory.memory_available_ratio = 0.12;
    assert(workloadScoreForDomain("proc_sys.vm", memory) > 0.0);
    assert(workloadSignature(memory).find("memory") != std::string::npos);

    SystemObservation io;
    io.io_pressure_valid = true;
    io.io_pressure_some_avg10 = 5.0;
    assert(workloadScoreForDomain("sysfs.block.queue", io) == 5.0);
    assert(workloadScoreForDomain("sysfs.cpu.cpufreq", io) == 0.0);
    assert(workloadSignature(io) == "io");

    SystemObservation render;
    render.frame_timing_valid = true;
    render.frame_average_fps = 60.0;
    render.frame_p95_interval_ms = 16.7;
    render.frame_p99_interval_ms = 45.0;
    render.slow_frame_ratio = 0.20;
    assert(workloadScoreForDomain("sysfs.devfreq", render) > 0.0);
    assert(workloadSignature(render) == "frame-pacing");
    render.frame_p95_interval_ms = render.frame_p99_interval_ms = 35.0;
    assert(workloadScoreForDomain("sysfs.devfreq", render) > 0.0);

    render.slow_frame_ratio = 0.01;
    render.frame_p99_interval_ms = 17.0;
    assert(workloadScoreForDomain("sysfs.devfreq", render) == 0.0);
    return 0;
}
