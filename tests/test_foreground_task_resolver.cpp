#include "coreflow/foreground_task_resolver.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace coreflow;

static void put(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary | std::ios::trunc) << value;
}
static std::string get(const fs::path& path) {
    std::ifstream input(path);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}
static std::string proc_stat(pid_t pid, uint64_t start) {
    std::string result = std::to_string(pid) + " (com.example.game) S";
    for (unsigned field = 4; field <= 21; ++field) result += " 0";
    result += " " + std::to_string(start) + " 0 0\n";
    return result;
}
static void make_task(const fs::path& proc, pid_t pid, pid_t tid,
                      const std::string& comm, uint64_t start) {
    put(proc / std::to_string(pid) / "task" / std::to_string(tid) / "comm", comm + "\n");
    put(proc / std::to_string(pid) / "task" / std::to_string(tid) / "stat", proc_stat(tid, start));
    put(proc / std::to_string(tid) / "status",
        "Name:\t" + comm + "\nTgid:\t" + std::to_string(pid) +
        "\nUid:\t10000\t10000\t10000\t10000\n");
    put(proc / std::to_string(pid) / "task" / std::to_string(tid) / "cgroup",
        "0::/top-app/render\n");
}

int main() {
    assert(ForegroundTaskResolver::is_excluded_process("system_server"));
    assert(ForegroundTaskResolver::is_excluded_process("surfaceflinger"));
    assert(ForegroundTaskResolver::is_excluded_thread("Jit code cache"));
    assert(ForegroundTaskResolver::is_excluded_thread("Signal Catcher"));
    assert(ForegroundTaskResolver::is_excluded_thread("HeapTaskDaemon"));
    assert(ForegroundTaskResolver::is_excluded_thread("Binder:321_1"));
    assert(ForegroundTaskResolver::is_whitelisted_thread(321, 322, "RenderThread"));
    assert(ForegroundTaskResolver::is_whitelisted_thread(321, 323, "UnityMain"));
    assert(ForegroundTaskResolver::is_whitelisted_thread(321, 324, "UnityChoreograp"));
    assert(ForegroundTaskResolver::is_whitelisted_thread(321, 325, "GLThread 0"));
    assert(ForegroundTaskResolver::is_whitelisted_thread(321, 326, "VkQueue"));
    assert(ForegroundTaskResolver::is_whitelisted_thread(321, 321, "main"));
    assert(!ForegroundTaskResolver::is_whitelisted_thread(321, 327, "Binder:321_1"));

    char pattern[] = "/tmp/coreflow-foreground-XXXXXX";
    const char* root_text = ::mkdtemp(pattern);
    assert(root_text != nullptr);
    const fs::path root(root_text);
    const fs::path cgroup = root / "cgroup";
    const fs::path proc = root / "proc";
    const fs::path top = cgroup / "top-app";
    const fs::path render = top / "render";

    put(cgroup / "cgroup.controllers", "cpu\n");
    put(cgroup / "cgroup.type", "domain threaded\n");
    put(cgroup / "cgroup.threads", "");
    put(top / "cgroup.type", "threaded\n");
    put(top / "cgroup.procs", "321\n");
    put(top / "cgroup.threads", "321\n");
    put(render / "cgroup.type", "threaded\n");
    put(render / "cgroup.threads", "322\n");

    put(proc / "321/cmdline", std::string("com.example.game\0", 17));
    put(proc / "321/status", "Name:\tcom.example.game\nTgid:\t321\nUid:\t10000\t10000\t10000\t10000\n");
    make_task(proc, 321, 321, "main", 1001);
    make_task(proc, 321, 322, "RenderThread", 1002);
    make_task(proc, 321, 323, "Binder:321_1", 1003);

    ::setenv("COREFLOW_TEST_STATE_DIR", (root / "state").c_str(), 1);
    assert(BaselineStore::instance().recover_pending());
    CentralMutationCoordinator coordinator;
    coordinator.reset();
    {
        CgroupV2Controller controller(coordinator, cgroup.string(), proc.string());
        assert(controller.is_supported());
        ForegroundTaskResolver resolver(controller, top.string(), proc.string());
        assert(resolver.refresh(true));
        auto snapshot = resolver.snapshot();
        assert(snapshot.source_available && !snapshot.ambiguous);
        assert(snapshot.package_name == "com.example.game");
        assert(snapshot.process_count == 1);
        assert(snapshot.discovered_thread_count == 2);
        assert(snapshot.migrated_thread_count == 2);
        assert(resolver.tracked_threads() == 2);
        assert(get(top / "cgroup.threads") == "322\n");

        // Simulate kernel membership readback and per-thread proc membership after migration.
        put(render / "cgroup.threads", "");
        put(top / "cgroup.threads", "321\n322\n");
        put(proc / "321/task/322/cgroup", "0::/top-app\n");
        assert(resolver.restore_all());
        assert(resolver.tracked_threads() == 0);
        assert(get(render / "cgroup.threads") == "322");
        assert(controller.pending_thread_migrations() == 0);
        assert(coordinator.active_leases().empty());

        // Re-admit the same app, then an empty cgroup.procs file represents a real
        // foreground transition and must trigger restoration rather than a stale-read hold.
        put(top / "cgroup.procs", "321\n");
        put(top / "cgroup.threads", "321\n");
        put(render / "cgroup.threads", "322\n");
        put(proc / "321/task/322/cgroup", "0::/top-app/render\n");
        assert(resolver.refresh(true));
        assert(resolver.tracked_threads() == 2);
        put(render / "cgroup.threads", "");
        put(top / "cgroup.threads", "321\n322\n");
        put(proc / "321/task/322/cgroup", "0::/top-app\n");
        put(top / "cgroup.procs", "");
        assert(resolver.refresh(true));
        assert(resolver.tracked_threads() == 0);
        assert(controller.pending_thread_migrations() == 0);

        // Two distinct app package roots are ambiguous; resolver does not pick one.
        put(top / "cgroup.procs", "321\n331\n");
        put(proc / "331/cmdline", std::string("com.other.app\0", 15));
        put(proc / "331/status", "Name:\tcom.other.app\nTgid:\t331\nUid:\t10001\t10001\t10001\t10001\n");
        make_task(proc, 331, 331, "main", 2001);
        put(proc / "331/task/331/cgroup", "0::/top-app\n");
        const auto before = resolver.refresh(true);
        assert(!before);
        snapshot = resolver.snapshot();
        assert(snapshot.source_available && snapshot.ambiguous);
        assert(snapshot.package_count == 2);
        assert(resolver.tracked_threads() == 0);
        assert(coordinator.active_leases().empty());
    }
    // Android GKI legacy cpuctl layout: process discovery comes from
    // top-app/cgroup.procs while per-thread placement uses the v1 tasks ABI.
    {
        const fs::path legacy = root / "cpuctl-layout";
        const fs::path legacy_proc = root / "legacy-proc";
        const fs::path legacy_top = legacy / "top-app";
        const fs::path legacy_foreground = legacy / "foreground";
        constexpr pid_t legacy_pid = 441;
        put(legacy_top / "cgroup.procs", "441\n");
        put(legacy_top / "tasks", "");
        put(legacy_top / "cpu.uclamp.min", "0.00");
        put(legacy_foreground / "tasks", "441\n");
        put(legacy_proc / "441/cmdline", std::string("com.example.androidgame\0", 25));
        put(legacy_proc / "441/status",
            "Name:\tcom.example.androidgame\nTgid:\t441\nUid:\t10000\t10000\t10000\t10000\n");
        make_task(legacy_proc, legacy_pid, legacy_pid, "main", 4401);
        put(legacy_proc / "441/task/441/cgroup", "2:cpu,cpuacct:/foreground\n");

        CentralMutationCoordinator legacy_coordinator;
        legacy_coordinator.reset();
        {
            CgroupV2Controller legacy_controller(
                legacy_coordinator, legacy.string(), legacy_proc.string());
            assert(legacy_controller.is_supported());
            ForegroundTaskResolver legacy_resolver(
                legacy_controller, legacy_top.string(), legacy_proc.string());
            assert(legacy_resolver.refresh(true));
            const auto legacy_snapshot = legacy_resolver.snapshot();
            assert(legacy_snapshot.source_available);
            assert(legacy_snapshot.process_count == 1U);
            assert(legacy_snapshot.package_name == "com.example.androidgame");
            assert(legacy_snapshot.discovered_thread_count == 1U);
            assert(legacy_snapshot.migrated_thread_count == 1U);
            assert(get(legacy_top / "tasks") == "441");
            put(legacy_proc / "441/task/441/cgroup", "2:cpu,cpuacct:/top-app\n");
            // Regular files do not emulate the kernel tasks ABI: simulate the
            // kernel removing the TID from its source cgroup after migration.
            put(legacy_foreground / "tasks", "");
            assert(legacy_resolver.restore_all());
            assert(get(legacy_foreground / "tasks") == "441");
            assert(legacy_controller.pending_thread_migrations() == 0U);
            assert(legacy_coordinator.active_leases().empty());
        }
    }

    fs::remove_all(root);
    return 0;
}
