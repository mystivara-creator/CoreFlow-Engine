#include "coreflow/v6/schedstat_sensor.hpp"
#include "coreflow/v6/vmstat_thrashing_sensor.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace coreflow::v6;
namespace fs = std::filesystem;

static void write_file(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p);
    assert(out.good());
    out << content;
}

int main() {
    char tpl[] = "/tmp/coreflow-v6-universal-XXXXXX";
    const char* created = ::mkdtemp(tpl);
    assert(created != nullptr);
    const fs::path root(created);
    const fs::path proc = root / "proc";
    fs::create_directories(proc);

    // -------------------------------------------------------------------------
    // 1. SchedstatSensor: universal runqueue delay detection, Hz normalization,
    //    unknown-engine main-PID tracking, SurfaceFlinger tracking, fail-closed
    // -------------------------------------------------------------------------
    {
        // 1a. Fail-closed when CONFIG_SCHEDSTATS=n (no schedstat files)
        SchedstatSensor missing_sensor((root / "proc_missing").string());
        const auto m = missing_sensor.sample(4000, {4001}, {900}, 120U, 1.0);
        assert(!m.available);
        assert(m.starvation_pressure < 0.0);
        assert(!missing_sensor.supported());

        // 1b. Live schedstat tracking across foreground main PID (even with empty fg_tids!) + SF
        const fs::path fg_main = proc / "4000/task/4000/schedstat";
        const fs::path sf_main = proc / "900/task/900/schedstat";
        write_file(fg_main, "100000000 0 100\n");
        write_file(sf_main, "50000000 0 50\n");

        SchedstatSensor sensor(proc.string());
        // Tick 0 establishes baseline -> available=true, starvation_pressure=-1.0
        const auto s0 = sensor.sample(4000, {}, {900}, 120U, 1.0);
        assert(s0.available);
        assert(s0.starvation_pressure < 0.0);
        assert(sensor.supported());

        // Tick 1: smooth execution (+400 ms exec, 0 ns run_delay, +100 pcount) -> 0.0 starvation
        write_file(fg_main, "500000000 0 200\n");
        write_file(sf_main, "150000000 0 100\n");
        const auto s_smooth = sensor.sample(4000, {}, {900}, 120U, 1.0);
        assert(s_smooth.available);
        assert(std::abs(s_smooth.starvation_pressure) < 1e-6);
        assert(std::abs(s_smooth.peak_wait_ms) < 1e-6);
        assert(s_smooth.exec_ratio >= 0.39 && s_smooth.exec_ratio <= 0.41);

        // Tick 2: Foreground main thread experiences heavy runqueue starvation
        // (+180 ms run_delay over 50 wakeups = 3.6 ms/wakeup, 18% wall wait ratio on 120Hz)
        write_file(fg_main, "750000000 180000000 250\n");
        write_file(sf_main, "250000000 1000000 150\n");
        const auto s_starved = sensor.sample(4000, {}, {900}, 120U, 1.0);
        assert(s_starved.available);
        assert(s_starved.starvation_pressure >= 0.25);
        assert(s_starved.peak_wait_ms >= 1.2);
        assert(s_starved.wait_ratio >= 0.06);

        // Tick 3: Sustained starvation climbs higher in EWMA
        write_file(fg_main, "1000000000 380000000 300\n");
        const auto s_starved2 = sensor.sample(4000, {}, {900}, 120U, 1.0);
        assert(s_starved2.starvation_pressure > s_starved.starvation_pressure);
        assert(s_starved2.starvation_pressure >= 0.45);

        // 1c. Refresh-rate sensitivity: identical 2.5 ms/wakeup wait triggers stronger starvation at 120Hz than 60Hz
        SchedstatSensor s60(proc.string());
        SchedstatSensor s120(proc.string());
        write_file(fg_main, "1000000000 400000000 400\n");
        (void)s60.sample(4000, {}, {}, 60U, 1.0);
        (void)s120.sample(4000, {}, {}, 120U, 1.0);
        // +25 ms run_delay across 10 wakeups = 2.5 ms/wakeup (wait_ratio = 2.5%, below ratio threshold)
        write_file(fg_main, "1100000000 425000000 410\n");
        const auto r60 = s60.sample(4000, {}, {}, 60U, 1.0);
        const auto r120 = s120.sample(4000, {}, {}, 120U, 1.0);
        assert(r120.starvation_pressure > r60.starvation_pressure + 0.20);

        // 1d. SurfaceFlinger-only starvation (foreground smooth, SF delayed) still rescues!
        SchedstatSensor sf_only(proc.string());
        write_file(fg_main, "2000000000 500000000 500\n");
        write_file(sf_main, "500000000 10000000 200\n");
        (void)sf_only.sample(4000, {}, {900}, 120U, 1.0);
        write_file(fg_main, "2100000000 500000000 550\n");
        write_file(sf_main, "600000000 210000000 240\n"); // SF waited 200 ms / 40 = 5 ms/wakeup!
        const auto r_sf = sf_only.sample(4000, {}, {900}, 120U, 1.0);
        assert(r_sf.available && r_sf.starvation_pressure >= 0.70);

        // 1e. reset() clears baselines cleanly (ScreenOff)
        sf_only.reset();
        const auto after_reset = sf_only.sample(4000, {}, {900}, 120U, 1.0);
        assert(after_reset.available && after_reset.starvation_pressure < 0.0);
    }

    // -------------------------------------------------------------------------
    // 2. VmstatThrashingSensor: Linux 5.9+ refault_file, legacy fallback,
    //    direct reclaim storm, pswpin, wrap guard, ScreenOff zero-syscall reset
    // -------------------------------------------------------------------------
    {
        // 2a. Fail-closed when /proc/vmstat missing
        VmstatThrashingSensor missing_vm((root / "proc_missing").string());
        const auto mv = missing_vm.sample(1000ULL, true);
        assert(!mv.available && !missing_vm.supported());

        // 2b. Modern GKI /proc/vmstat (workingset_refault_file + pgsteal_direct + pswpin)
        const fs::path vm_path = proc / "vmstat";
        write_file(vm_path,
                   "nr_free_pages 50000\n"
                   "workingset_refault_file 1000\n"
                   "workingset_refault_anon 200\n"
                   "pgsteal_direct 10\n"
                   "pswpin 100\n");
        VmstatThrashingSensor vm(proc.string());
        const auto v0 = vm.sample(1000ULL, true);
        assert(v0.available && vm.supported());
        assert(std::abs(v0.thrashing_pressure) < 1e-6);
        assert(!v0.direct_reclaim_storm);

        // Calm window (+30 refaults/s, 0 direct reclaim) -> 0 pressure
        write_file(vm_path,
                   "workingset_refault_file 1030\n"
                   "workingset_refault_anon 200\n"
                   "pgsteal_direct 10\n"
                   "pswpin 105\n");
        const auto v_calm = vm.sample(2000ULL, true);
        assert(v_calm.available);
        assert(std::abs(v_calm.thrashing_pressure) < 1e-6);
        assert(!v_calm.direct_reclaim_storm);

        // Regime A: heavy file-refault thrashing (+1800 pages/s), NO direct reclaim storm
        write_file(vm_path,
                   "workingset_refault_file 2830\n"
                   "workingset_refault_anon 220\n"
                   "pgsteal_direct 10\n"
                   "pswpin 120\n");
        const auto v_refault = vm.sample(3000ULL, true);
        assert(v_refault.available);
        assert(std::abs(v_refault.refault_file_per_sec - 1800.0) < 5.0);
        assert(!v_refault.direct_reclaim_storm);
        assert(v_refault.thrashing_pressure >= 0.25);

        // Regime B: direct reclaim storm (+180 pgsteal_direct/s) -> storm flag + high pressure
        write_file(vm_path,
                   "workingset_refault_file 4630\n"
                   "workingset_refault_anon 250\n"
                   "pgsteal_direct 190\n"
                   "pswpin 320\n");
        const auto v_storm = vm.sample(4000ULL, true);
        assert(v_storm.available);
        assert(v_storm.direct_reclaim_storm);
        assert(std::abs(v_storm.direct_reclaim_per_sec - 180.0) < 2.0);
        assert(v_storm.thrashing_pressure > v_refault.thrashing_pressure);

        // Counter wrap / non-monotonic jump resets cleanly without bogus spike
        write_file(vm_path,
                   "workingset_refault_file 100\n"
                   "pgsteal_direct 0\n"
                   "pswpin 0\n");
        const auto v_wrap = vm.sample(5000ULL, true);
        assert(v_wrap.available && std::abs(v_wrap.thrashing_pressure) < 1e-6);

        // ScreenOff (active=false) resets state cleanly
        const auto v_off = vm.sample(6000ULL, false);
        assert(!v_off.available && std::abs(v_off.thrashing_pressure) < 1e-6);

        // 2c. Legacy kernel < 5.9 fallback (workingset_refault + split pgsteal_direct_*)
        const fs::path legacy_proc = root / "proc_legacy";
        const fs::path legacy_vm = legacy_proc / "vmstat";
        write_file(legacy_vm,
                   "workingset_refault 500\n"
                   "pgsteal_direct_normal 20\n"
                   "pgsteal_direct_movable 10\n"
                   "pswpin 50\n");
        VmstatThrashingSensor vm_legacy(legacy_proc.string());
        (void)vm_legacy.sample(1000ULL, true);
        write_file(legacy_vm,
                   "workingset_refault 2000\n"
                   "pgsteal_direct_normal 70\n"
                   "pgsteal_direct_movable 50\n"
                   "pswpin 50\n");
        const auto vl = vm_legacy.sample(2000ULL, true);
        assert(vl.available);
        assert(std::abs(vl.refault_file_per_sec - 1500.0) < 5.0);
        assert(std::abs(vl.direct_reclaim_per_sec - 90.0) < 2.0);
        assert(vl.direct_reclaim_storm);
        assert(vl.thrashing_pressure >= 0.20);
    }

    std::error_code ec;
    fs::remove_all(root, ec);
    std::cout << "v6.2 Universal Zero-eBPF Sensors tests passed (SchedstatSensor + VmstatThrashingSensor)\n";
    return 0;
}
