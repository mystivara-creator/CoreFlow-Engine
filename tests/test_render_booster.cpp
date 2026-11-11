#include "coreflow/baseline_store.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/reactor.hpp"
#include "coreflow/render_booster.hpp"
#include "coreflow/uclamp.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

using namespace coreflow;
namespace fs = std::filesystem;

static void put(const fs::path& path, const char* value) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::trunc) << value;
}
static std::string get(const fs::path& path) {
    std::ifstream input(path);
    std::string value;
    input >> value;
    return value;
}
static bool wait_for(const fs::path& path, const char* expected,
                     std::chrono::milliseconds timeout) {
    const auto until = std::chrono::steady_clock::now() + timeout;
    do {
        if (get(path) == expected) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    } while (std::chrono::steady_clock::now() < until);
    return get(path) == expected;
}

int main() {
    char state_dir[] = "/tmp/coreflow-render-XXXXXX";
    assert(::mkdtemp(state_dir) != nullptr);
    assert(::setenv("COREFLOW_TEST_STATE_DIR", state_dir, 1) == 0);
    assert(BaselineStore::instance().recover_pending());

    const fs::path root = fs::path(state_dir) / "cgroup";
    const fs::path top = root / "top-app";
    put(root / "cgroup.controllers", "cpu");
    put(top / "cpu.uclamp.min", "0.00");
    put(top / "cpu.uclamp.max", "100.00");

    CentralMutationCoordinator coordinator;
    coordinator.reset();
    UclampGovernor governor(coordinator, root.string());
    assert(governor.is_supported());
    EpollReactor reactor;
    {
        RenderBooster booster(governor, reactor);
        std::thread loop([&] { reactor.run(); });

        booster.on_touch_down();
        assert(wait_for(top / "cpu.uclamp.min", "37.11",
                        std::chrono::milliseconds(100)));
        assert(wait_for(top / "cpu.uclamp.min", "0.00",
                        std::chrono::milliseconds(500)));
        assert(wait_for(top / "cpu.uclamp.max", "100.00",
                        std::chrono::milliseconds(500)));
        assert(coordinator.active_leases().empty());

        // External ownership loss during the burst must never be overwritten
        // by the expiry callback.
        booster.on_touch_down();
        assert(wait_for(top / "cpu.uclamp.min", "37.11",
                        std::chrono::milliseconds(100)));
        put(top / "cpu.uclamp.min", "75.00");
        std::this_thread::sleep_for(std::chrono::milliseconds(180));
        assert(get(top / "cpu.uclamp.min") == "75.00");

        reactor.stop();
        loop.join();
    }
    fs::remove_all(state_dir);
    return 0;
}
