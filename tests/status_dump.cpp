// Writes real status snapshots produced by the C++ engine code so the WebUI parser
// can be tested against actual output (tools/test_webui_status.sh).
#include "coreflow/status_snapshot.hpp"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <string>

using namespace coreflow;

namespace {

EngineStatus base() {
    EngineStatus s;
    s.version = "2.1.0";
    s.pid = 4242;
    s.sample = 120;
    s.updated_epoch = 1760000000;
    s.interval_s = 5;
    s.thermal_available = true;
    s.thermal_c = 41.5;
    s.hottest_c = 43.0;
    s.thermal_trend = "STABLE";
    s.mem_available_ratio = 0.52;
    s.memory_trend = "STABLE";
    s.cpu_available = true;
    s.cpu_utilization = 0.31;
    s.load1 = 1.25;
    s.load_trend = "RISING";
    s.io_available = true;
    s.io_read_kbs = 120.0;
    s.io_write_kbs = 40.0;
    s.battery_percent = 76;
    s.confidence = 0.93;
    s.state = "NORMAL";
    s.previous_state = "NORMAL";
    s.decision = "NO_ACTION";
    s.workload = "INTERACTIVE";
    s.thermal_headroom = s.memory_headroom = s.power_headroom = true;
    s.proactive_allowed = true;
    s.plan_action = "CANDIDATE";
    s.intervention = "LOW";
    s.mutation_eligible = true;
    s.candidates = 4;
    s.plan_reason = "bounded candidates available";
    s.safety_hold = "NONE";
    s.mode = "adaptive";
    s.armed = true;
    s.permit_resource = true;
    s.baseline_ready = true;
    s.baseline_samples = 5;
    s.baseline_target = 5;
    return s;
}

void emit(const std::string& dir, const char* name, const EngineStatus& s) {
    std::ofstream out(dir + "/" + name);
    out << statusToJson(s);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    const std::string dir = argv[1];

    emit(dir, "ready.json", base());

    EngineStatus observe = base();
    observe.mode = "observe";
    observe.armed = false;
    observe.permit_resource = false;
    emit(dir, "observe.json", observe);

    EngineStatus stressed = base();
    stressed.state = "THERMAL_GUARD";
    stressed.previous_state = "ELEVATED";
    stressed.thermal_c = 52.0;
    stressed.hottest_c = 55.5;
    stressed.proactive_allowed = false;
    stressed.stabilizing_allowed = true;
    stressed.stabilizing_only = true;
    stressed.resource_mutated = true;
    stressed.stabilizing_epoch = true;
    stressed.resource_applied.emplace_back("/proc/sys/vm/swappiness", "53");
    stressed.verified = 3;
    stressed.rolled_back = 1;
    stressed.last_change_kind = "resource";
    stressed.last_change_result = "VERIFIED";
    stressed.last_change_sample = 118;
    emit(dir, "stressed_holding.json", stressed);

    EngineStatus hostile = base();
    hostile.thermal_available = true;
    hostile.thermal_c = std::numeric_limits<double>::quiet_NaN();
    hostile.plan_reason = "<img src=x onerror=alert(1)> \"q\" \\ \n end";
    hostile.resource_applied.emplace_back("/sys/<script>x</script>", "<b>1</b>");
    emit(dir, "hostile.json", hostile);

    EngineStatus stopped = base();
    stopped.running = false;
    emit(dir, "stopped.json", stopped);

    EngineStatus idle = base();
    idle.state = "IDLE";
    idle.proactive_allowed = false;
    idle.mutation_eligible = false;
    idle.candidates = 0;
    emit(dir, "idle.json", idle);
    return 0;
}
