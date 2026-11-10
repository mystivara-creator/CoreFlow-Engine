#include "coreflow/device_profiler.hpp"
#include "coreflow/intent_actuator.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace coreflow;

static void put(const fs::path& path, const std::string& value, bool writable = false) {
    fs::create_directories(path.parent_path());
    { std::ofstream out(path); out << value << '\n'; assert(out.good()); }
    (void)::chmod(path.c_str(), writable ? 0666 : 0444);
}
static uint64_t now_ns() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
int main() {
    const fs::path root = fs::temp_directory_path() / ("coreflow-p3-" + std::to_string(::getpid()));
    fs::remove_all(root);
    const fs::path proc = root / "proc";
    const fs::path sys = root / "sys";
    const fs::path state = root / "state";
    const fs::path vm = proc / "sys/vm";
    const fs::path cache_pressure = vm / "vfs_cache_pressure";
    const fs::path page_cluster = vm / "page-cluster";
    const fs::path read_ahead = sys / "block/mmcblk0/queue/read_ahead_kb";
    put(proc / "sys/kernel/osrelease", "test-kernel");
    put(proc / "meminfo", "MemTotal: 4096 kB\nMemAvailable: 2048 kB");
    put(proc / "self/mountinfo", "1 0 0:1 / / rw - rootfs rootfs rw");
    put(cache_pressure, "100", true);
    put(page_cluster, "3", true);
    put(read_ahead, "128", true);

    DeviceProfiler profiler(sys.string(), proc.string());
    const auto profile = profiler.refresh();
    assert(profile && profile->infrastructure_nodes_scanned >= 5U);
    assert(profile->infrastructure_nodes_writable >= 3U);
    const NodeCapability* cache_cap = nullptr;
    const NodeCapability* io_cap = nullptr;
    for (const auto& cap : profile->node_capabilities) {
        if (cap.semantic_id == "vm.vfs_cache_pressure") cache_cap = &cap;
        if (cap.semantic_id == "block.queue.read_ahead_kb") io_cap = &cap;
    }
    assert(cache_cap && cache_cap->mutation_eligible());
    assert(io_cap && io_cap->mutation_eligible());

    (void)::setenv("COREFLOW_TEST_STATE_DIR", state.c_str(), 1);
    BaselineStore& store = BaselineStore::instance();
    store.reinitialize();
    assert(store.recover_pending());
    CentralMutationCoordinator coordinator;
    {
        IntentActuator actuator(coordinator, store, proc.string());
        const uint64_t start = now_ns();
        InfrastructurePsiSample admission{
            .monotonic_ns=start,.memory_some_avg10=8.0,.io_some_avg10=0.0,
            .memory_valid=true,.io_valid=true
        };
        assert(actuator.begin(profile, "vm.vfs_cache_pressure", "120", admission));
        std::string current;
        { std::ifstream in(cache_pressure); in >> current; }
        assert(current == "120");

        auto outcome = actuator.observe({.monotonic_ns=start+1'000'000'000ULL,
            .memory_some_avg10=3.0,.memory_valid=true});
        assert(outcome.state == IntentActuatorState::Observing);
        outcome = actuator.observe({.monotonic_ns=start+2'000'000'000ULL,
            .memory_some_avg10=3.0,.memory_valid=true});
        assert(outcome.state == IntentActuatorState::Observing);
        outcome = actuator.observe({.monotonic_ns=start+3'100'000'000ULL,
            .memory_some_avg10=3.0,.memory_valid=true});
        assert(outcome.state == IntentActuatorState::Retained);
        assert(outcome.observation_samples == 3U);
        assert(actuator.rollback("test cleanup"));
        { std::ifstream in(cache_pressure); in >> current; }
        assert(current == "100");
    }
    {
        IntentActuator actuator(coordinator, store, proc.string());
        const uint64_t start = now_ns();
        InfrastructurePsiSample no_pressure{
            .monotonic_ns=start,.memory_some_avg10=0.0,.io_some_avg10=0.0,
            .memory_valid=true,.io_valid=true
        };
        assert(!actuator.begin(profile, "vm.vfs_cache_pressure", "120", no_pressure));
        assert(actuator.snapshot().state == IntentActuatorState::Rejected);
    }
    (void)::unsetenv("COREFLOW_TEST_STATE_DIR");
    fs::remove_all(root);
    return 0;
}
