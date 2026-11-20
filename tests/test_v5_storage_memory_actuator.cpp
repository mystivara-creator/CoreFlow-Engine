#include "coreflow/baseline_store.hpp"
#include "coreflow/v5/storage_memory_actuator.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace coreflow;
using namespace coreflow::v5;
namespace fs = std::filesystem;

static void write_file(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p);
    assert(out.good());
    out << content;
}

static std::string read_file(const fs::path& p) {
    std::ifstream in(p);
    std::string line;
    std::getline(in, line);
    return line;
}

int main() {
    char tpl[] = "/tmp/coreflow-v5-storage-XXXXXX";
    const char* created = ::mkdtemp(tpl);
    assert(created != nullptr);
    const fs::path root(created);
    assert(::setenv("COREFLOW_TEST_STATE_DIR", root.c_str(), 1) == 0);

    auto& baseline = BaselineStore::instance();
    baseline.reinitialize();
    assert(baseline.recover_pending());

    HardwareTopology topo{};
    topo.total_ram_bytes = 8ULL * 1024ULL * 1024ULL * 1024ULL;
    topo.zram_active = true;
    topo.primary_storage = StorageType::UFS;

    const fs::path ra_path = root / "sys/block/sda/queue/read_ahead_kb";
    const fs::path bg_io = root / "cg/background/io.weight";
    const fs::path bg_uclamp = root / "cg/background/cpu.uclamp.max";
    const fs::path mglru = root / "sys/kernel/mm/lru_gen/min_ttl_ms";
    const fs::path page_cluster = root / "proc/sys/vm/page-cluster";
    const fs::path swappiness = root / "proc/sys/vm/swappiness";
    const fs::path compaction = root / "proc/sys/vm/compaction_proactiveness";

    write_file(ra_path, "128\n");
    write_file(bg_io, "default 100\n");
    write_file(bg_uclamp, "max\n");
    write_file(mglru, "0\n");
    write_file(page_cluster, "3\n");
    write_file(swappiness, "100\n");
    write_file(compaction, "20\n");

    topo.storage_queues.push_back({"sda", StorageType::UFS, ra_path.string(), "", "", 128});
    topo.cgroups.background_io_weight_path = bg_io.string();
    topo.cgroups.background_uclamp_max_path = bg_uclamp.string();
    topo.cgroups.uclamp_uses_percent = true;
    topo.vm.mglru_min_ttl_path = mglru.string();
    topo.vm.page_cluster_path = page_cluster.string();
    topo.vm.swappiness_path = swappiness.string();
    topo.vm.compaction_proactiveness_path = compaction.string();

    StorageMemoryActuator act(baseline, (root / "proc").string());

    // 1. LaunchBurst -> UFS burst 512 KB, background isolated, MGLRU=1000, compaction paused
    ContextDecision launch_ctx{};
    launch_ctx.mode = ContextMode::LaunchBurst;
    launch_ctx.newly_launched_app = true;
    launch_ctx.foreground_pid = 9999;
    launch_ctx.foreground_package = "com.example.biggame";
    act.apply(topo, launch_ctx, PerformanceProfile::Balanced, true, true, 1000);

    assert(read_file(ra_path) == "512");
    assert(read_file(bg_io) == "default 15");
    assert(read_file(bg_uclamp) == "35.00");
    assert(read_file(mglru) == "1000");
    assert(read_file(page_cluster) == "0");
    assert(read_file(swappiness) == "60");
    assert(read_file(compaction) == "0");
    assert(act.stats().burst_active);
    assert(act.stats().background_isolated);

    // 2. Steady-state ScreenOff -> UFS drops back to 128 KB, background limits restored, MGLRU=0
    ContextDecision off_ctx{};
    off_ctx.mode = ContextMode::ScreenOff;
    act.apply(topo, off_ctx, PerformanceProfile::Balanced, true, true, 5000);

    assert(read_file(ra_path) == "128");
    assert(read_file(bg_io) == "default 100");
    assert(read_file(bg_uclamp) == "max");
    assert(read_file(mglru) == "0");
    assert(read_file(compaction) == "20");
    assert(!act.stats().background_isolated);

    // 3. Smart pre-read via posix_fadvise on mapped app assets with cooldown
    const fs::path apk_file = root / "app/base.apk";
    write_file(apk_file, std::string(4096, 'A'));
    write_file(root / "proc/9999/maps", "70000000-70001000 r--p 00000000 00:00 1 " + apk_file.string() + "\n");
    const uint64_t advised = act.prefetch_app_assets(9999, "com.example.test", topo.total_ram_bytes, 10000, true);
    assert(advised == 4096U);
    // Cooldown prevents redundant prefetch within 30s
    assert(act.prefetch_app_assets(9999, "com.example.test", topo.total_ram_bytes, 15000, true) == 0U);

    // 4. OEM contention is isolated instead of fighting external writers
    ContextDecision game_ctx{};
    game_ctx.mode = ContextMode::Gaming;
    act.apply(topo, game_ctx, PerformanceProfile::Gaming, true, true, 20000);
    write_file(mglru, "250\n"); // Simulate OEM daemon overwrite
    act.apply(topo, launch_ctx, PerformanceProfile::Gaming, true, true, 25000);
    assert(read_file(mglru) == "250"); // Respected OEM takeover
    assert(act.stats().contended_nodes >= 1U);

    assert(baseline.rollback_all());
    (void)::unsetenv("COREFLOW_TEST_STATE_DIR");
    fs::remove_all(root);
    std::cout << "v5 StorageMemoryActuator tests passed\n";
    return 0;
}
