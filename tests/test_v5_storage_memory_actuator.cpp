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
    topo.zram_bytes = 8ULL * 1024ULL * 1024ULL * 1024ULL;
    topo.zram_active = true;
    topo.primary_storage = StorageType::UFS;

    const fs::path ra_path = root / "sys/block/sda/queue/read_ahead_kb";
    const fs::path bg_io = root / "cg/background/io.weight";
    const fs::path bg_uclamp = root / "cg/background/cpu.uclamp.max";
    const fs::path mglru_en = root / "sys/kernel/mm/lru_gen/enabled";
    const fs::path mglru = root / "sys/kernel/mm/lru_gen/min_ttl_ms";
    const fs::path page_cluster = root / "proc/sys/vm/page-cluster";
    const fs::path swappiness = root / "proc/sys/vm/swappiness";
    const fs::path compaction = root / "proc/sys/vm/compaction_proactiveness";

    write_file(ra_path, "128\n");
    write_file(bg_io, "default 100\n");
    write_file(bg_uclamp, "max\n");
    write_file(mglru_en, "0x0000\n");
    write_file(mglru, "0\n");
    write_file(page_cluster, "3\n");
    write_file(swappiness, "100\n");
    write_file(compaction, "20\n");

    StorageQueueInfo ufs_q{};
    ufs_q.device_name = "sda";
    ufs_q.type = StorageType::UFS;
    ufs_q.read_ahead_kb_path = ra_path.string();
    ufs_q.baseline_read_ahead_kb = 128;
    ufs_q.max_sectors_kb = 512;
    ufs_q.nr_requests = 64;
    topo.storage_queues.push_back(ufs_q);

    topo.cgroups.background_io_weight_path = bg_io.string();
    topo.cgroups.background_uclamp_max_path = bg_uclamp.string();
    topo.cgroups.uclamp_uses_percent = true;
    topo.vm.mglru_enabled_path = mglru_en.string();
    topo.vm.mglru_min_ttl_path = mglru.string();
    topo.vm.page_cluster_path = page_cluster.string();
    topo.vm.swappiness_path = swappiness.string();
    topo.vm.compaction_proactiveness_path = compaction.string();

    StorageMemoryActuator act(baseline, (root / "proc").string());

    StorageMemoryTelemetry calm_tel{};
    calm_tel.memory.total_bytes = topo.total_ram_bytes;
    calm_tel.memory.available_bytes = 4ULL * 1024ULL * 1024ULL * 1024ULL; // 50% free
    calm_tel.memory.valid = true;

    // 1. LaunchBurst -> UFS burst widens dynamically above baseline, background isolated, MGLRU enabled, compaction paused
    ContextDecision launch_ctx{};
    launch_ctx.mode = ContextMode::LaunchBurst;
    launch_ctx.newly_launched_app = true;
    launch_ctx.foreground_pid = 9999;
    launch_ctx.foreground_package = "com.example.biggame";
    act.apply(topo, launch_ctx, PerformanceProfile::Balanced, true, true, 1000, calm_tel);

    const uint64_t burst_ra = std::stoull(read_file(ra_path));
    assert(burst_ra > 128U && burst_ra <= 512U);
    assert(read_file(bg_io).rfind("default ", 0) == 0 && read_file(bg_io) != "default 100");
    assert(read_file(bg_uclamp) != "max");
    assert(read_file(mglru_en) == "y");
    const uint64_t calm_ttl = std::stoull(read_file(mglru));
    assert(calm_ttl >= 800U);
    assert(read_file(page_cluster) == "0");
    const uint64_t calm_swap = std::stoull(read_file(swappiness));
    assert(calm_swap >= 30U && calm_swap <= 90U);
    assert(read_file(compaction) == "0");
    assert(act.stats().burst_active);
    assert(act.stats().background_isolated);
    assert(act.stats().contended_nodes == 0U);

    // 2. Dynamic memory pressure adaptation: tighter available RAM + higher mem_psi raises swappiness and lowers MGLRU TTL
    StorageMemoryTelemetry tight_tel = calm_tel;
    tight_tel.memory.available_bytes = 1ULL * 1024ULL * 1024ULL * 1024ULL; // 12.5% free
    tight_tel.mem_psi = 30.0;
    ContextDecision inter_ctx{};
    inter_ctx.mode = ContextMode::Interactive;
    act.apply(topo, inter_ctx, PerformanceProfile::Balanced, true, true, 2000, tight_tel);
    const uint64_t tight_swap = std::stoull(read_file(swappiness));
    const uint64_t tight_ttl = std::stoull(read_file(mglru));
    assert(tight_swap > calm_swap);
    assert(tight_ttl < calm_ttl);

    // 3. Steady-state ScreenOff -> UFS drops down to low-latency steady read_ahead, background limits restored, MGLRU TTL=0
    ContextDecision off_ctx{};
    off_ctx.mode = ContextMode::ScreenOff;
    act.apply(topo, off_ctx, PerformanceProfile::Balanced, true, true, 5000, calm_tel);

    const uint64_t off_ra = std::stoull(read_file(ra_path));
    assert(off_ra <= 128U && off_ra >= 64U);
    assert(read_file(bg_io) == "default 100");
    assert(read_file(bg_uclamp) == "max");
    assert(read_file(mglru) == "0");
    assert(std::stoull(read_file(compaction)) > 0U);
    assert(!act.stats().background_isolated);

    // 4. Smart pre-read via posix_fadvise on mapped app assets with 60s cooldown
    const fs::path apk_file = root / "app/base.apk";
    write_file(apk_file, std::string(4096, 'A'));
    write_file(root / "proc/9999/maps", "70000000-70001000 r--p 00000000 00:00 1 " + apk_file.string() + "\n");
    const uint64_t advised = act.prefetch_app_assets(9999, "com.example.test", calm_tel.memory.available_bytes, 10000, true);
    assert(advised == 4096U);
    assert(act.prefetch_app_assets(9999, "com.example.test", calm_tel.memory.available_bytes, 30000, true) == 0U);

    // 5. Kernel quantization tolerance on cpu.uclamp.max does NOT trigger false contention
    ContextDecision game_ctx{};
    game_ctx.mode = ContextMode::Gaming;
    act.apply(topo, game_ctx, PerformanceProfile::Gaming, true, true, 20000, calm_tel);
    const double cur_pct = std::stod(read_file(bg_uclamp));
    char quantized[32];
    std::snprintf(quantized, sizeof(quantized), "%.2f\n", cur_pct - 0.12);
    write_file(bg_uclamp, quantized); // Simulate minor kernel 1/1024 bucket rounding
    act.apply(topo, game_ctx, PerformanceProfile::Gaming, true, true, 21000, calm_tel);
    assert(act.stats().contended_nodes == 0U);

    // 6. Real OEM contention on MGLRU is isolated instead of fighting external writers
    write_file(mglru, "250\n"); // Simulate OEM daemon overwrite
    act.apply(topo, launch_ctx, PerformanceProfile::Gaming, true, true, 25000, calm_tel);
    assert(read_file(mglru) == "250");
    assert(act.stats().contended_nodes >= 1U);

    // 7. Hardware-calibrated read_ahead reacts dynamically to queue geometry and io_psi
    StorageQueueInfo deep_ufs = ufs_q;
    deep_ufs.max_sectors_kb = 1024;
    deep_ufs.nr_requests = 128;
    const uint64_t deep_burst = StorageMemoryActuator::compute_read_ahead_kb(
        deep_ufs, ContextMode::LaunchBurst, PerformanceProfile::Balanced, 25.0);
    const uint64_t calm_steady = StorageMemoryActuator::compute_read_ahead_kb(
        deep_ufs, ContextMode::Interactive, PerformanceProfile::Balanced, 0.0);
    assert(deep_burst > calm_steady);

    assert(baseline.rollback_all() > 0U);
    assert(read_file(mglru_en) == "0x0000");
    (void)::unsetenv("COREFLOW_TEST_STATE_DIR");
    fs::remove_all(root);
    std::cout << "v5 StorageMemoryActuator tests passed\n";
    return 0;
}
