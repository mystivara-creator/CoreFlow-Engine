#include "coreflow/input_event_source.hpp"
#include "coreflow/foreground_task_resolver.hpp"
#include "coreflow/baseline_store.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <fcntl.h>
#include <linux/input.h>
#include <mutex>
#include <string>
#include <thread>
#include <sys/stat.h>
#include <unistd.h>

using namespace coreflow;
namespace fs = std::filesystem;

namespace coreflow {
struct NativeInputEventSourceTestAccess {
    static void scan(NativeInputEventSource& source) {
        source.scan_devices();
    }
};
} // namespace coreflow

static bool accept_test_fifo(int) noexcept {
    return true;
}

static void run_decoder_tests() {
    assert(is_touchscreen_capability(true, false, true, true, true, true));
    assert(is_touchscreen_capability(false, false, true, true, true, false));
    assert(!is_touchscreen_capability(false, true, true, true, true, false));
    assert(!is_touchscreen_capability(false, false, true, false, true, false));
    assert(!is_touchscreen_capability(true, false, true, true, false, false));

    TouchInputState state{};
    FrameEvent output{};
    const auto event = [](uint16_t type, uint16_t code, int32_t value) {
        input_event result{};
        result.type = type;
        result.code = code;
        result.value = value;
        return result;
    };

    assert(!consume_touch_input_event(state, event(EV_KEY, BTN_TOUCH, 1), 1'000'000ULL, output));
    assert(consume_touch_input_event(state, event(EV_SYN, SYN_REPORT, 0), 2'000'000ULL, output));
    assert(output.kind == FrameEventKind::TouchBurst && output.burst_duration_ms == 80U);

    assert(!consume_touch_input_event(state, event(EV_ABS, ABS_X, 100), 10'000'000ULL, output));
    assert(!consume_touch_input_event(state, event(EV_SYN, SYN_REPORT, 0), 20'000'000ULL, output));
    assert(consume_touch_input_event(state, event(EV_SYN, SYN_REPORT, 0), 60'000'000ULL, output));

    assert(!consume_touch_input_event(state, event(EV_KEY, BTN_TOUCH, 0), 61'000'000ULL, output));
    assert(!consume_touch_input_event(state, event(EV_ABS, ABS_X, 101), 120'000'000ULL, output));
    assert(!consume_touch_input_event(state, event(EV_SYN, SYN_REPORT, 0), 121'000'000ULL, output));

    TouchInputState mt{};
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_SLOT, 0), 1, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_TRACKING_ID, 17), 2, output));
    assert(consume_touch_input_event(mt, event(EV_SYN, SYN_REPORT, 0), 3, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_SLOT, 1), 4, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_TRACKING_ID, 18), 5, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_SLOT, 0), 6, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_TRACKING_ID, -1), 7, output));
    assert(!consume_touch_input_event(mt, event(EV_SYN, SYN_REPORT, 0), 8, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_SLOT, 1), 9, output));
    assert(!consume_touch_input_event(mt, event(EV_ABS, ABS_MT_TRACKING_ID, -1), 10, output));
    assert(!consume_touch_input_event(mt, event(EV_SYN, SYN_REPORT, 0), 11, output));

    TouchInputState dropped{};
    assert(!consume_touch_input_event(dropped, event(EV_KEY, BTN_TOUCH, 1), 1, output));
    assert(!consume_touch_input_event(dropped, event(EV_SYN, SYN_DROPPED, 0), 2, output));
    assert(!consume_touch_input_event(dropped, event(EV_SYN, SYN_REPORT, 0), 3, output));
    assert(!consume_touch_input_event(dropped, event(EV_KEY, BTN_TOUCH, 1), 4, output));
    assert(consume_touch_input_event(dropped, event(EV_SYN, SYN_REPORT, 0), 5, output));
}

static bool wait_until(const std::function<bool()>& predicate,
                       std::chrono::milliseconds timeout = std::chrono::seconds(3)) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) return true;
        std::this_thread::yield();
    }
    return predicate();
}

static void write_touch_burst(int fd) {
    const std::array<input_event, 2> events{{
        input_event{.type = EV_KEY, .code = BTN_TOUCH, .value = 1},
        input_event{.type = EV_SYN, .code = SYN_REPORT, .value = 0},
    }};
    const ssize_t written = ::write(fd, events.data(), sizeof(events));
    assert(written == static_cast<ssize_t>(sizeof(events)));
}

static void run_inotify_hotplug_tests() {
    char pattern[] = "/tmp/coreflow-input-hotplug-XXXXXX";
    const char* root_text = ::mkdtemp(pattern);
    assert(root_text != nullptr);
    const fs::path root(root_text);
    const fs::path input_dir = root / "input";
    fs::create_directories(input_dir);
    const fs::path event_path = input_dir / "event0";

    EpollReactor reactor;
    NativeInputEventSource source(reactor, input_dir.string(), &accept_test_fifo);
    std::mutex mutex;
    std::condition_variable cv;
    unsigned touch_bursts = 0;
    assert(source.start([&](const FrameEvent& event) {
        if (event.kind != FrameEventKind::TouchBurst) return;
        {
            std::lock_guard<std::mutex> lock(mutex);
            ++touch_bursts;
        }
        cv.notify_all();
    }));
    // Empty startup must preserve the active inotify watch.
    assert(!source.active());

    std::thread reactor_thread([&] { reactor.run(); });
    assert(wait_until([&] { return reactor.is_running(); }));

    auto create_device_and_emit = [&](unsigned expected_bursts) {
        assert(::mkfifo(event_path.c_str(), 0600) == 0);
        UniqueFd writer(::open(event_path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC));
        assert(writer.valid());
        write_touch_burst(writer.get());
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_for(lock, std::chrono::seconds(3),
            [&] { return touch_bursts >= expected_bursts; });
    };

    // Late driver probe: an event node appears after an empty start().
    assert(create_device_and_emit(1));
    assert(source.active());

    // Remove every device, then wait until inotify has processed the deletion.
    assert(::unlink(event_path.c_str()) == 0);
    assert(wait_until([&] { return !source.active(); }));

    // Re-create the node without restarting NativeInputEventSource or epoll.
    assert(create_device_and_emit(2));
    assert(source.active());

    assert(::unlink(event_path.c_str()) == 0);
    assert(wait_until([&] { return !source.active(); }));
    reactor.stop();
    reactor_thread.join();
    source.stop();
    fs::remove_all(root);
}

static void put_file(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    assert(output.good());
    output << value;
    assert(output.good());
}

static std::string proc_stat(pid_t pid, uint64_t start) {
    std::string result = std::to_string(pid) + " (com.example.game) S";
    for (unsigned field = 4; field <= 21; ++field) result += " 0";
    result += " " + std::to_string(start) + " 0 0\n";
    return result;
}

static void make_task(const fs::path& proc, pid_t pid, pid_t tid,
                      const std::string& comm, uint64_t start) {
    put_file(proc / std::to_string(pid) / "task" / std::to_string(tid) / "comm",
             comm + "\n");
    put_file(proc / std::to_string(pid) / "task" / std::to_string(tid) / "stat",
             proc_stat(tid, start));
    put_file(proc / std::to_string(tid) / "status",
             "Name:\t" + comm + "\nTgid:\t" + std::to_string(pid) +
             "\nUid:\t10000\t10000\t10000\t10000\n");
    put_file(proc / std::to_string(pid) / "task" / std::to_string(tid) / "cgroup",
             "0::/top-app/render\n");
}

static void prepare_foreground_fixture(const fs::path& cgroup, const fs::path& proc) {
    const fs::path top = cgroup / "top-app";
    const fs::path render = top / "render";
    put_file(cgroup / "cgroup.controllers", "cpu\n");
    put_file(cgroup / "cgroup.type", "domain threaded\n");
    put_file(cgroup / "cgroup.threads", "");
    put_file(top / "cgroup.type", "threaded\n");
    put_file(top / "cgroup.procs", "321\n");
    put_file(top / "cgroup.threads", "321\n");
    put_file(render / "cgroup.type", "threaded\n");
    put_file(render / "cgroup.threads", "322\n");

    put_file(proc / "321/cmdline", std::string("com.example.game\0", 17));
    put_file(proc / "321/status",
             "Name:\tcom.example.game\nTgid:\t321\nUid:\t10000\t10000\t10000\t10000\n");
    make_task(proc, 321, 321, "main", 1001);
    make_task(proc, 321, 322, "RenderThread", 1002);
}

static size_t count_open_fds() {
    DIR* directory = ::opendir("/proc/self/fd");
    assert(directory != nullptr);
    size_t count = 0;
    while (dirent* entry = ::readdir(directory)) {
        if (entry->d_name[0] == '.') continue;
        bool numeric = true;
        for (const char* p = entry->d_name; *p != '\0'; ++p) {
            if (*p < '0' || *p > '9') { numeric = false; break; }
        }
        if (numeric) ++count;
    }
    assert(::closedir(directory) == 0);
    return count;
}

static void run_fd_leak_regression() {
    char pattern[] = "/tmp/coreflow-fd-regression-XXXXXX";
    const char* root_text = ::mkdtemp(pattern);
    assert(root_text != nullptr);
    const fs::path root(root_text);
    const fs::path input_dir = root / "input";
    const fs::path cgroup = root / "cgroup";
    const fs::path proc = root / "proc";
    fs::create_directories(input_dir);

    assert(::setenv("COREFLOW_TEST_STATE_DIR", (root / "state").c_str(), 1) == 0);
    assert(BaselineStore::instance().recover_pending());
    CentralMutationCoordinator coordinator;
    coordinator.reset();
    EpollReactor reactor;
    CgroupV2Controller controller(coordinator, cgroup.string(), proc.string());
    assert(controller.is_supported());
    ForegroundTaskResolver resolver(controller, (cgroup / "top-app").string(), proc.string());
    NativeInputEventSource source(reactor, input_dir.string(), &accept_test_fifo);

    const size_t fd_count_before = count_open_fds();
    for (unsigned iteration = 0; iteration < 50; ++iteration) {
        prepare_foreground_fixture(cgroup, proc);
        const fs::path event_path = input_dir / "event0";
        assert(::mkfifo(event_path.c_str(), 0600) == 0);

        assert(source.start([](const FrameEvent&) {}));
        assert(source.active());
        NativeInputEventSourceTestAccess::scan(source);

        assert(::unlink(event_path.c_str()) == 0);
        NativeInputEventSourceTestAccess::scan(source);
        assert(!source.active());

        assert(resolver.refresh(true));
        assert(resolver.tracked_threads() == 2);

        // Model kernel thread-membership readback after the migration.
        put_file(cgroup / "top-app/render/cgroup.threads", "");
        put_file(cgroup / "top-app/cgroup.threads", "321\n322\n");
        put_file(proc / "321/task/322/cgroup", "0::/top-app\n");
        assert(resolver.restore_all());
        assert(resolver.tracked_threads() == 0);
        assert(controller.pending_thread_migrations() == 0);
        assert(coordinator.active_leases().empty());

        source.stop();
        assert(!source.active());
    }

    const size_t fd_count_after = count_open_fds();
    assert(fd_count_after == fd_count_before);
    fs::remove_all(root);
}

int main() {
    run_decoder_tests();
    run_inotify_hotplug_tests();
    run_fd_leak_regression();
    return 0;
}
