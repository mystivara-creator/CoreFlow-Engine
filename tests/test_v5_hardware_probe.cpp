#include "coreflow/v5/hardware_probe.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace coreflow::v5;
namespace fs = std::filesystem;

static void write_file(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p);
    assert(out.good());
    out << content;
}

int main() {
    char tpl[] = "/tmp/coreflow-v5-probe-XXXXXX";
    const char* created = ::mkdtemp(tpl);
    assert(created != nullptr);
    const fs::path root(created);
    const fs::path sys = root / "sys";
    const fs::path proc = root / "proc";

    write_file(proc / "sys/kernel/osrelease", "6.1.75-android14\n");
    write_file(sys / "devices/soc0/machine", "Snapdragon 8 Gen 3\n");
    write_file(proc / "meminfo", "MemTotal:       12288000 kB\nMemFree:        4096000 kB\n");
    write_file(sys / "block/zram0/disksize", "4294967296\n");

    // 3-cluster CPU topology
    write_file(sys / "devices/system/cpu/cpu0/cpu_capacity", "300\n");
    write_file(sys / "devices/system/cpu/cpu4/cpu_capacity", "700\n");
    write_file(sys / "devices/system/cpu/cpu7/cpu_capacity", "1024\n");

    const fs::path p0 = sys / "devices/system/cpu/cpufreq/policy0";
    write_file(p0 / "scaling_available_frequencies", "300000 800000 1500000 2000000\n");
    write_file(p0 / "scaling_governor", "walt\n");
    write_file(p0 / "related_cpus", "0-3\n");
    write_file(p0 / "walt/hispeed_freq", "1500000\n");
    write_file(p0 / "walt/hispeed_load", "90\n");

    const fs::path p4 = sys / "devices/system/cpu/cpufreq/policy4";
    write_file(p4 / "scaling_available_frequencies", "500000 1400000 2400000 2800000\n");
    write_file(p4 / "scaling_governor", "walt\n");
    write_file(p4 / "related_cpus", "4-6\n");
    write_file(p4 / "walt/hispeed_freq", "1400000\n");

    const fs::path p7 = sys / "devices/system/cpu/cpufreq/policy7";
    write_file(p7 / "scaling_available_frequencies", "600000 1800000 3000000 3300000\n");
    write_file(p7 / "scaling_governor", "walt\n");
    write_file(p7 / "related_cpus", "7\n");
    write_file(p7 / "walt/hispeed_freq", "1800000\n");

    // KGSL GPU
    const fs::path kgsl = sys / "class/kgsl/kgsl-3d0";
    write_file(kgsl / "gpu_model", "Adreno750\n");
    write_file(kgsl / "gpu_available_frequencies", "903000000 770000000 650000000 440000000 220000000\n");
    write_file(kgsl / "num_pwrlevels", "5\n");
    write_file(kgsl / "max_pwrlevel", "0\n");
    write_file(kgsl / "min_pwrlevel", "4\n");
    write_file(kgsl / "idle_timer", "80\n");
    write_file(kgsl / "gpu_busy_percentage", " 62 %\n");

    // Physical UFS block queue (must never touch iostats)
    const fs::path ufs_real = sys / "devices/platform/1d84000.ufshc/host0/block/sda";
    write_file(ufs_real / "queue/rotational", "0\n");
    write_file(ufs_real / "queue/read_ahead_kb", "512\n");
    write_file(ufs_real / "queue/nr_requests", "128\n");
    write_file(ufs_real / "queue/scheduler", "[mq-deadline] none\n");
    write_file(ufs_real / "queue/iostats", "1\n");
    fs::create_directories(sys / "class/block");
    fs::create_symlink(ufs_real, sys / "class/block/sda");

    // VM + MGLRU
    write_file(proc / "sys/vm/swappiness", "100\n");
    write_file(proc / "sys/vm/page-cluster", "3\n");
    write_file(sys / "kernel/mm/lru_gen/min_ttl_ms", "0\n");

    // Backlight & display refresh
    const fs::path bl = sys / "class/backlight/panel0";
    write_file(bl / "brightness", "500\n");
    write_file(bl / "max_brightness", "4095\n");
    write_file(sys / "class/drm/card0-DSI-1/modes", "1440x3200@120\n");

    // PSI
    write_file(proc / "pressure/cpu", "some avg10=12.50 avg60=8.00 avg300=4.00 total=100\n");

    HardwareProbe probe(sys.string(), proc.string());
    const HardwareTopology topo = probe.discover();

    assert(topo.soc_model == "Snapdragon 8 Gen 3");
    assert(topo.kernel_version == "6.1.75-android14");
    assert(topo.total_ram_bytes == 12288000ULL * 1024ULL);
    assert(topo.zram_active && topo.zram_bytes == 4294967296ULL);
    assert(topo.refresh_hz == 120U);
    assert(topo.cpu_clusters.size() == 3U);
    assert(topo.cpu_clusters[0].role == ClusterRole::Little);
    assert(topo.cpu_clusters[1].role == ClusterRole::Mid);
    assert(topo.cpu_clusters[2].role == ClusterRole::Big);
    assert(topo.gpu.available && topo.gpu.is_kgsl && topo.gpu.model == "Adreno750");
    assert(topo.gpu.kgsl_num_pwrlevels == 5U);
    assert(topo.primary_storage == StorageType::UFS);
    assert(topo.storage_queues.size() == 1U);
    assert(topo.storage_queues[0].baseline_read_ahead_kb == 512U);
    assert(!topo.vm.mglru_min_ttl_path.empty());

    assert(!probe.is_screen_off(topo));
    write_file(bl / "brightness", "0\n");
    assert(probe.is_screen_off(topo));

    const auto gpu_busy = probe.read_gpu_busy_pct(topo);
    assert(gpu_busy.has_value() && *gpu_busy == 62.0);
    const auto cpu_psi = probe.read_psi_avg10("cpu");
    assert(cpu_psi.has_value() && *cpu_psi == 12.5);

    // MediaTek GED GPU + OLED DRM DPMS / bl_power + sugov_ext custom governor probe
    {
        const fs::path mtk_sys = root / "mtk_sys";
        const fs::path mtk_proc = root / "mtk_proc";
        const fs::path mp0 = mtk_sys / "devices/system/cpu/cpufreq/policy0";
        write_file(mp0 / "scaling_available_frequencies", "400000 1200000 2000000\n");
        write_file(mp0 / "scaling_governor", "sugov_ext\n");
        write_file(mp0 / "sugov_ext/up_rate_limit_us", "500\n");
        write_file(mtk_sys / "module/ged/parameters/gpu_loading", "54 0 0\n");
        const fs::path oled_bl = mtk_sys / "class/backlight/panel0-backlight";
        write_file(oled_bl / "brightness", "120\n");
        write_file(oled_bl / "max_brightness", "4095\n");
        write_file(oled_bl / "bl_power", "0\n");
        const fs::path drm = mtk_sys / "class/drm/card0-DSI-1";
        write_file(drm / "status", "connected\n");
        write_file(drm / "dpms", "On\n");

        HardwareProbe mtk_probe(mtk_sys.string(), mtk_proc.string());
        const HardwareTopology mtk_topo = mtk_probe.discover();
        assert(mtk_topo.cpu_clusters.size() == 1U);
        assert(!mtk_topo.cpu_clusters[0].up_rate_limit_path.empty());
        assert(mtk_topo.gpu.available && mtk_topo.gpu.is_mtk_ged);
        const auto mtk_busy = mtk_probe.read_gpu_busy_pct(mtk_topo);
        assert(mtk_busy.has_value() && *mtk_busy == 54.0);
        assert(!mtk_probe.is_screen_off(mtk_topo));
        // OLED AOD / DPMS Off triggers ScreenOff even when brightness is non-zero
        write_file(drm / "dpms", "Off\n");
        assert(mtk_probe.is_screen_off(mtk_topo));
        write_file(drm / "dpms", "On\n");
        write_file(oled_bl / "bl_power", "4\n");
        assert(mtk_probe.is_screen_off(mtk_topo));
    }

    // Android cpuctl: the resolver's "most app processes" heuristic lands on `background`
    // (3 cached apps vs 1 visible app). The EAS cap target must still be the AOSP `top-app`
    // group and must never alias another managed cgroup node.
    {
        const fs::path cg_sys = root / "cg_sys";
        const fs::path cg_proc = root / "cg_proc";
        const fs::path cpuctl = root / "cpuctl";
        write_file(cg_proc / "self/mountinfo",
                   "41 30 0:42 / " + cpuctl.string() +
                       " rw,nosuid,nodev - cgroup cgroup rw,cpu,cpuacct\n");
        const auto make_group = [&](const char* name, const std::string& pids) {
            const fs::path g = cpuctl / name;
            write_file(g / "cpu.uclamp.min", "0.00\n");
            write_file(g / "cpu.uclamp.max", "max\n");
            write_file(g / "tasks", pids);
            write_file(g / "cgroup.procs", pids);
        };
        make_group("top-app", "501\n");
        make_group("foreground", "");
        make_group("system-background", "");
        make_group("background", "601\n602\n603\n");
        for (const int pid : {501, 601, 602, 603}) {
            const std::string id = std::to_string(pid);
            const char* grp = pid == 501 ? "top-app" : "background";
            write_file(cg_proc / id / "status",
                       "Name:\tapp\nUid:\t10200\t10200\t10200\t10200\nTgid:\t" + id + "\n");
            write_file(cg_proc / id / "oom_score_adj", pid == 501 ? "0\n" : "900\n");
            write_file(cg_proc / id / "cgroup", std::string("7:cpu,cpuacct:/") + grp + "\n");
        }
        HardwareProbe cg_probe(cg_sys.string(), cg_proc.string());
        const HardwareTopology cg_topo = cg_probe.discover();
        assert(fs::path(cg_topo.cgroups.top_app_uclamp_max_path).parent_path().filename() ==
               "top-app");
        assert(fs::path(cg_topo.cgroups.foreground_uclamp_max_path).parent_path().filename() ==
               "foreground");
        assert(fs::path(cg_topo.cgroups.system_background_uclamp_max_path).parent_path().filename() ==
               "system-background");
        assert(fs::path(cg_topo.cgroups.background_uclamp_max_path).parent_path().filename() ==
               "background");
        assert(cg_topo.cgroups.top_app_uclamp_max_path != cg_topo.cgroups.background_uclamp_max_path);
    }

    // 4. Display refresh_hz discovery for Qualcomm/HyperOS DRM formats (p120, p-120, 120Hz)
    //    and live dynamic_fps leaf that starts at 0 at boot then transitions to 120Hz.
    {
        const fs::path disp_root = root / "disp_test";
        const fs::path dsys = disp_root / "sys";
        const fs::path dproc = disp_root / "proc";
        fs::create_directories(dproc);
        write_file(dsys / "class/drm/card0-DSI-1/status", "connected\n");
        write_file(dsys / "class/drm/card0-DSI-1/modes", "1220x2712p120\n1220x2712p60\n");
        write_file(dsys / "class/mi_display/disp-DSI-0/dynamic_fps", "0\n");
        HardwareProbe dprobe(dsys.string(), dproc.string());
        const HardwareTopology dtopo = dprobe.discover();
        assert(dtopo.refresh_hz == 120U);
        assert(!dtopo.refresh_rate_paths.empty());
        // Live dynamic_fps updates from 0 -> 120 after display wakes up
        write_file(dsys / "class/mi_display/disp-DSI-0/dynamic_fps", "120\n");
        const auto live_hz = dprobe.read_live_refresh_hz(dtopo);
        assert(live_hz.has_value() && *live_hz == 120U);
    }

    fs::remove_all(root);
    std::cout << "v5 HardwareProbe tests passed\n";
    return 0;
}
