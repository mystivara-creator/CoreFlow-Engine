#include "coreflow/baseline_store.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/reactor.hpp"
#include "coreflow/render_booster.hpp"
#include "coreflow/frame_bridge.hpp"
#include "coreflow/uclamp.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <sys/timerfd.h>

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
namespace coreflow {
struct RenderBoosterTestAccess {
    static void dispatch_request(RenderBooster& booster) {
        booster.handle_request(EPOLLIN);
    }
    static void restore(RenderBooster& booster) {
        booster.restore_burst();
    }
    static int64_t expiry_remaining_ns(const RenderBooster& booster) {
        itimerspec current{};
        if (!booster.expiry_fd_.valid() ||
            ::timerfd_gettime(booster.expiry_fd_.get(), &current) != 0) return -1;
        return static_cast<int64_t>(current.it_value.tv_sec) * 1'000'000'000LL +
            static_cast<int64_t>(current.it_value.tv_nsec);
    }
};
} // namespace coreflow

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
        RenderBooster booster(governor, reactor, 512U, 200U);
        AndroidFrameEventBridge frame_bridge;
        assert(booster.connect_frame_source(frame_bridge));
        const FrameEvent touch_event{
            .kind = FrameEventKind::TouchBurst,
            .vsync_timestamp_ns = 1000000ULL,
        };
        assert(frame_bridge.submit(touch_event));

        // Producer callbacks and metrics snapshots may execute concurrently.
        // Both paths must use the same metrics synchronization boundary.
        std::thread producer([&] {
            for (uint64_t i = 1; i <= 2000; ++i) {
                const FrameEvent frame_event{
                    .kind = FrameEventKind::FramePresented,
                    .vsync_timestamp_ns = i,
                    .frame_duration_ns = 8'000'000ULL,
                    .jank_threshold_ns = 16'000'000ULL,
                };
                assert(frame_bridge.submit(frame_event));
            }
        });
        std::thread reader([&] {
            for (int i = 0; i < 2000; ++i) {
                (void)booster.get_metrics();
            }
        });
        producer.join();
        reader.join();

        booster.disconnect_frame_source();
        assert(!frame_bridge.submit(touch_event));

        // Verify listener callbacks may re-enter the bridge without deadlock.
        AndroidFrameEventBridge reentrant_bridge;
        int callback_count = 0;
        bool nested_submit_succeeded = false;
        assert(reentrant_bridge.start([&](const FrameEvent& event) {
            ++callback_count;
            if (callback_count == 1) {
                nested_submit_succeeded = reentrant_bridge.submit(event);
            } else {
                reentrant_bridge.stop();
            }
        }));
        assert(reentrant_bridge.submit(touch_event));
        assert(nested_submit_succeeded);
        assert(callback_count == 2);
        assert(!reentrant_bridge.submit(touch_event));

        // Drive the exact reactor callbacks synchronously. This validates
        // mutation and expiry/restore semantics without scheduler timing,
        // sleeps, or dependence on CI runner CPU availability.
        booster.on_touch_down();
        RenderBoosterTestAccess::dispatch_request(booster);
        assert(get(top / "cpu.uclamp.min") == "50.00");
        const int64_t remaining_ns = RenderBoosterTestAccess::expiry_remaining_ns(booster);
        assert(remaining_ns > 0 && remaining_ns <= 200'000'000LL);
        assert(governor.get_top_app_max() == 1024u);

        RenderBoosterTestAccess::restore(booster);
        assert(get(top / "cpu.uclamp.min") == "0.00");
        // Restore the exact OEM representation, not merely equivalent capacity.
        assert(get(top / "cpu.uclamp.max") == "100.00");
        assert(governor.get_top_app_max() == 1024u);
        assert(coordinator.active_leases().empty());

        // A subsequent burst must be admitted after restoration and journal cleanup.
        booster.on_touch_down();
        RenderBoosterTestAccess::dispatch_request(booster);
        assert(get(top / "cpu.uclamp.min") == "50.00");
        RenderBoosterTestAccess::restore(booster);
        assert(get(top / "cpu.uclamp.max") == "100.00");
        assert(coordinator.active_leases().empty());

        // External ownership loss during a burst must never be overwritten
        // by the synchronous expiry/restore path.
        booster.on_touch_down();
        RenderBoosterTestAccess::dispatch_request(booster);
        assert(get(top / "cpu.uclamp.min") == "50.00");
        put(top / "cpu.uclamp.min", "75.00");
        RenderBoosterTestAccess::restore(booster);
        assert(get(top / "cpu.uclamp.min") == "75.00");
    }
    fs::remove_all(state_dir);
    return 0;
}
