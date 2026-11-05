#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

static void put(const fs::path& p, const std::string& value) {
    fs::create_directories(p.parent_path());
    std::ofstream(p) << value;
}

static std::string get(const fs::path& p) {
    std::string line;
    std::ifstream file(p);
    std::getline(file, line);
    return line;
}

int main() {
    char pattern[] = "/tmp/coreflow-autotuner-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    assert(dir != nullptr);
    const fs::path root(dir);
    ::setenv("COREFLOW_TEST_STATE_DIR", dir, 1);

    auto& baseline = coreflow::BaselineStore::instance();
    assert(baseline.recover_pending());

    const fs::path sys_root = root / "sys";
    const fs::path proc_root = root / "proc";
    const fs::path sda_q = sys_root / "block/sda/queue";
    const fs::path zram_q = sys_root / "block/zram0/queue";

    put(sda_q / "scheduler", "[none] mq-deadline kyber bfq");
    put(sda_q / "read_ahead_kb", "128");
    put(sda_q / "nr_requests", "64");
    put(sda_q / "rq_affinity", "1");
    put(sda_q / "iosched/read_expire", "500");
    put(sda_q / "iosched/writes_starved", "2");
    put(sda_q / "iosched/fifo_batch", "16");

    // Virtual zram device must be ignored by block queue discovery
    put(zram_q / "scheduler", "[none]");

    put(proc_root / "sys/vm/page-cluster", "3");
    put(proc_root / "sys/vm/vfs_cache_pressure", "100");

    coreflow::AutonomousTuner tuner(sys_root.string(), proc_root.string());
    const auto initial = tuner.snapshot();
    assert(initial.block_queues.size() == 1);
    assert(initial.block_queues[0].device_name == "sda");
    assert(initial.block_queues[0].active_scheduler == "none");
    assert(initial.block_queues[0].available_schedulers.size() == 4);
    assert(initial.block_queues[0].read_ahead_kb == 128);
    assert(initial.block_queues[0].nr_requests == 64);
    assert(initial.block_queues[0].rq_affinity == 1);

    // 1. Idle equilibrium: no need -> must hold baseline without mutating
    tuner.step({.cpu_psi = 1.0, .memory_psi = 0.2, .io_psi = 0.3, .thermal_pressure = 42.0});
    assert(tuner.snapshot().stage == "PLANNED");
    assert(get(sda_q / "scheduler") == "[none] mq-deadline kyber bfq");

    // 1b. TUNER-01 All-or-Nothing Preflight: if a required dependent node is unwritable,
    // no partial mutation of the anchor (scheduler) may occur!
    if (::geteuid() != 0) {
        assert(::chmod((sda_q / "iosched/read_expire").c_str(), 0444) == 0);
        // Temporarily restrict available schedulers to mq-deadline so it tests mq-deadline preflight directly
        put(sda_q / "scheduler", "[none] mq-deadline");
        tuner.discover(sys_root.string(), proc_root.string());
        tuner.step({.cpu_psi = 6.0, .memory_psi = 0.5, .io_psi = 7.5, .thermal_pressure = 48.0});
        assert(tuner.snapshot().stage == "PLANNED");
        assert(get(sda_q / "scheduler") == "[none] mq-deadline");
        assert(::chmod((sda_q / "iosched/read_expire").c_str(), 0644) == 0);
        put(sda_q / "scheduler", "[none] mq-deadline kyber bfq");
        tuner.discover(sys_root.string(), proc_root.string());
    }

    // 2. Elevated I/O PSI -> Autonomously selects mq-deadline + coupled iosched tunables
    tuner.step({.cpu_psi = 6.0, .memory_psi = 0.5, .io_psi = 7.5, .thermal_pressure = 48.0});
    const auto observing1 = tuner.snapshot();
    assert(observing1.stage == "OBSERVING");
    assert(observing1.active_candidate_id == "io.mq_deadline_low_latency");
    assert(get(sda_q / "scheduler") == "mq-deadline");
    assert(get(sda_q / "iosched/read_expire") == "250");
    assert(get(sda_q / "iosched/fifo_batch") == "8");
    assert(get(sda_q / "read_ahead_kb") == "256");

    // 3. Simulate regression during 3-tick observation window (io_psi worsens from 7.5 to 11.0)
    for (int i = 0; i < 3; ++i) {
        tuner.step({.cpu_psi = 8.0, .memory_psi = 0.5, .io_psi = 11.0, .thermal_pressure = 50.0});
    }
    const auto after_regress = tuner.snapshot();
    assert(after_regress.last_verdict == "REGRESSED");
    assert(after_regress.verdict == coreflow::TunerVerdict::Regressed);
    assert(after_regress.stage == "PLANNED");
    // BaselineStore normalized "[none] mq-deadline kyber bfq" -> bare token "none" on rollback
    assert(get(sda_q / "scheduler") == "none");
    assert(get(sda_q / "iosched/read_expire") == "500");
    assert(get(sda_q / "read_ahead_kb") == "128");

    // Re-populate advertised scheduler list on mock sysfs file before next selection
    put(sda_q / "scheduler", "[none] mq-deadline kyber bfq");
    tuner.discover(sys_root.string(), proc_root.string());

    // 4. Next step under high I/O + CPU load: mq-deadline is on cooldown (persisted across discover!),
    // so bandit selects kyber!
    tuner.step({.cpu_psi = 14.0, .memory_psi = 0.5, .io_psi = 8.5, .thermal_pressure = 49.0});
    const auto observing2 = tuner.snapshot();
    assert(observing2.stage == "OBSERVING");
    assert(observing2.active_candidate_id == "io.kyber_token_bucket");
    assert(get(sda_q / "scheduler") == "kyber");
    assert(get(sda_q / "nr_requests") == "128");

    // 5. Simulate measurable benefit during 3-tick observation window (io_psi drops from 8.5 to 1.8)
    for (int i = 0; i < 3; ++i) {
        tuner.step({.cpu_psi = 5.0, .memory_psi = 0.4, .io_psi = 1.8, .thermal_pressure = 47.0});
    }
    const auto retained = tuner.snapshot();
    assert(retained.last_verdict == "IMPROVED");
    assert(retained.verdict == coreflow::TunerVerdict::Improved);
    assert(retained.stage == "RETAINED");
    assert(retained.total_improved == 1);
    assert(retained.improved_count == 1);
    assert(get(sda_q / "scheduler") == "kyber");

    // 6. Sustained idle for 6 ticks -> cleanly releases retained mutation back to OEM baseline
    for (int i = 0; i < 6; ++i) {
        tuner.step({.cpu_psi = 0.8, .memory_psi = 0.1, .io_psi = 0.1, .thermal_pressure = 40.0});
    }
    assert(tuner.snapshot().stage == "PLANNED");
    assert(get(sda_q / "scheduler") == "none");

    // 6b. TUNER-02 Kyber Cooldown Fix: verify that when kyber regresses and is on cooldown,
    // high concurrent CPU + I/O PSI does NOT bypass kyber's cooldown via the +0.15 bonus!
    tuner.reset();
    put(sda_q / "scheduler", "[none] kyber bfq");
    tuner.discover(sys_root.string(), proc_root.string());
    tuner.step({.cpu_psi = 14.0, .memory_psi = 0.5, .io_psi = 8.5, .thermal_pressure = 48.0});
    assert(tuner.snapshot().active_candidate_id == "io.kyber_token_bucket");
    for (int i = 0; i < 3; ++i) {
        tuner.step({.cpu_psi = 14.0, .memory_psi = 0.5, .io_psi = 12.0, .thermal_pressure = 50.0});
    }
    assert(tuner.snapshot().verdict == coreflow::TunerVerdict::Regressed);
    assert(!tuner.is_candidate_eligible("io.kyber_token_bucket"));
    put(sda_q / "scheduler", "[none] kyber bfq");
    tuner.discover(sys_root.string(), proc_root.string());
    // Must NOT re-select kyber while on cooldown; must select bfq instead!
    tuner.step({.cpu_psi = 14.0, .memory_psi = 0.5, .io_psi = 8.5, .thermal_pressure = 48.0});
    assert(tuner.snapshot().active_candidate_id == "io.bfq_interactive_fairness");

    // 6c. TUNER-01 External Value Modification during observation -> OWNERSHIP_LOST
    // While bfq is being observed (read_ahead_kb set to 256), an external writer sets read_ahead_kb to 512.
    put(sda_q / "read_ahead_kb", "512");
    tuner.step({.cpu_psi = 4.0, .memory_psi = 0.2, .io_psi = 2.0, .thermal_pressure = 45.0});
    assert(tuner.snapshot().verdict == coreflow::TunerVerdict::OwnershipLost);
    assert(tuner.snapshot().stage == "PLANNED");
    // External writer's value (512) is preserved, while our other node (scheduler) is rolled back to "none"
    assert(get(sda_q / "read_ahead_kb") == "512");
    assert(get(sda_q / "scheduler") == "none");
    put(sda_q / "read_ahead_kb", "128");
    tuner.reset();

    // 7. Smart CPUFreq schedutil governor rate-limit tuning under CPU PSI bottleneck
    // First verify scaling_governor mismatch blocks schedutil rate-limit mutation (TUNER-02)
    const fs::path policy0_dir = sys_root / "devices/system/cpu/cpufreq/policy0";
    const fs::path schedutil_dir = policy0_dir / "schedutil";
    put(policy0_dir / "scaling_governor", "performance");
    put(schedutil_dir / "up_rate_limit_us", "2000");
    put(schedutil_dir / "down_rate_limit_us", "1000");
    tuner.discover(sys_root.string(), proc_root.string());
    tuner.step({.cpu_psi = 12.5, .memory_psi = 0.2, .io_psi = 0.4, .thermal_pressure = 44.0});
    assert(tuner.snapshot().stage == "PLANNED");
    assert(get(schedutil_dir / "up_rate_limit_us") == "2000");

    // Now set scaling_governor to schedutil -> eligible and applied!
    put(policy0_dir / "scaling_governor", "schedutil");
    tuner.discover(sys_root.string(), proc_root.string());
    const auto cpu_obs = tuner.step({.cpu_psi = 12.5, .memory_psi = 0.2, .io_psi = 0.4, .thermal_pressure = 44.0});
    assert(cpu_obs.stage == "OBSERVING");
    assert(cpu_obs.active_candidate_id == "cpufreq.schedutil_interactive_response");
    assert(get(schedutil_dir / "up_rate_limit_us") == "500");
    assert(get(schedutil_dir / "down_rate_limit_us") == "10000");

    tuner.reset();
    assert(tuner.snapshot().stage == "PLANNED");
    assert(get(schedutil_dir / "up_rate_limit_us") == "2000");
    assert(get(schedutil_dir / "down_rate_limit_us") == "1000");

    // 8. TUNER-02: /proc/swaps zram precondition for vm.zram_latency_relief
    put(proc_root / "swaps", "Filename\tType\tSize\tUsed\tPriority\n");
    tuner.discover(sys_root.string(), proc_root.string());
    assert(!tuner.is_candidate_eligible("vm.zram_latency_relief"));
    tuner.step({.cpu_psi = 1.0, .memory_psi = 4.5, .io_psi = 0.2, .thermal_pressure = 42.0});
    assert(tuner.snapshot().stage == "PLANNED");
    assert(get(proc_root / "sys/vm/page-cluster") == "3");

    put(proc_root / "swaps",
        "Filename\tType\tSize\tUsed\tPriority\n/dev/block/zram0\tpartition\t2097148\t1024\t-2\n");
    tuner.discover(sys_root.string(), proc_root.string());
    assert(tuner.is_candidate_eligible("vm.zram_latency_relief"));
    tuner.step({.cpu_psi = 1.0, .memory_psi = 4.5, .io_psi = 0.2, .thermal_pressure = 42.0});
    assert(tuner.snapshot().stage == "OBSERVING");
    assert(tuner.snapshot().active_candidate_id == "vm.zram_latency_relief");
    assert(get(proc_root / "sys/vm/page-cluster") == "0");
    assert(get(proc_root / "sys/vm/vfs_cache_pressure") == "80");

    tuner.reset();
    fs::remove_all(root);
    std::cout << "[PASS] AutonomousTuner atomic transactions, cooldowns, and domain preconditions succeeded.\n";
    return 0;
}
