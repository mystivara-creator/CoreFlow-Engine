#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"
#include <algorithm>
#include <cassert>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
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

    // Concurrent requests for one domain must have a single lease winner.
    auto& coordinator = coreflow::CentralMutationCoordinator::instance();
    coordinator.reset();
    const auto first_frequency_identity =
        coreflow::CentralMutationCoordinator::make_controller_identity("FrequencyController");
    const auto second_frequency_identity =
        coreflow::CentralMutationCoordinator::make_controller_identity("FrequencyController");
    assert(first_frequency_identity.rfind("FrequencyController@", 0) == 0);
    assert(second_frequency_identity.rfind("FrequencyController@", 0) == 0);
    assert(first_frequency_identity != second_frequency_identity);
    std::atomic<uint32_t> acquired{0};
    std::vector<std::thread> contenders;
    for (uint32_t i = 0; i < 12; ++i) {
        contenders.emplace_back([&, i] {
            const std::string owner = "concurrent-controller-" + std::to_string(i);
            if (coordinator.acquire_lease(
                    coreflow::MutationDomain::BlockIo, owner, "tx-" + std::to_string(i),
                    coreflow::ControllerPriority::WorkloadOptimization)) {
                acquired.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& contender : contenders) contender.join();
    assert(acquired.load(std::memory_order_relaxed) == 1);
    assert(coordinator.active_leases().size() == 1);
    assert(coordinator.conflict_rejections() == 11);
    std::string invalid_lease_reason;
    assert(!coordinator.acquire_lease(
        coreflow::MutationDomain::BlockIo, "", "invalid-owner-tx",
        coreflow::ControllerPriority::WorkloadOptimization, &invalid_lease_reason));
    assert(!invalid_lease_reason.empty());
    assert(!coordinator.acquire_lease(
        coreflow::MutationDomain::BlockIo, "InvalidTransactionController", "",
        coreflow::ControllerPriority::WorkloadOptimization));
    assert(coordinator.active_leases().size() == 1);
    const auto winning_lease = coordinator.active_leases().front();
    coordinator.release_lease(winning_lease.domain, winning_lease.owner_controller, winning_lease.transaction_id);
    assert(coordinator.active_leases().empty());
    assert(coordinator.acquire_lease(coreflow::MutationDomain::BlockIo, "replacement", "replacement-tx", coreflow::ControllerPriority::WorkloadOptimization));
    assert(coordinator.active_leases().size() == 1);
    // A controller must release the previous transaction before acquiring a new one.
    coordinator.release_lease(coreflow::MutationDomain::BlockIo, "replacement", "replacement-tx");
    assert(coordinator.acquire_lease(coreflow::MutationDomain::BlockIo, "replacement", "replacement-tx-2", coreflow::ControllerPriority::WorkloadOptimization));
    // A stale completion from the earlier transaction must not release the newer lease.
    coordinator.release_lease(coreflow::MutationDomain::BlockIo, "replacement", "replacement-tx");
    const auto protected_lease = coordinator.active_leases();
    assert(protected_lease.size() == 1);
    assert(protected_lease.front().transaction_id == "replacement-tx-2");
    coordinator.release_lease(coreflow::MutationDomain::BlockIo, "replacement", "replacement-tx-2");

    // A controller that owns more than one domain must release each exact
    // transaction independently; domain scope alone is not sufficient.
    coordinator.reset();
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::CpuPolicy, "ScopedReleaseController", "scoped-cpu-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::GpuDevfreq, "ScopedReleaseController", "scoped-gpu-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    // An empty transaction identifier is never a wildcard release.
    coordinator.release_lease(
        coreflow::MutationDomain::CpuPolicy, "ScopedReleaseController", "");
    assert(coordinator.active_leases().size() == 2);
    coordinator.release_lease(
        coreflow::MutationDomain::CpuPolicy, "ScopedReleaseController", "scoped-cpu-tx");
    const auto scoped_remaining = coordinator.active_leases();
    assert(scoped_remaining.size() == 1);
    assert(scoped_remaining.front().domain == coreflow::MutationDomain::GpuDevfreq);
    assert(scoped_remaining.front().transaction_id == "scoped-gpu-tx");
    coordinator.release_lease(
        coreflow::MutationDomain::GpuDevfreq, "ScopedReleaseController", "scoped-gpu-tx");

    // Controller callbacks must run outside the coordinator lock and only release
    // the exact transaction after the controller acknowledges successful rollback.
    coordinator.reset();
    std::atomic<uint32_t> acknowledged_callbacks{0};
    assert(coordinator.register_preemption_handler("AckController",
        [&](coreflow::MutationDomain domain, std::string_view tx, std::string_view reason) {
            assert(domain == coreflow::MutationDomain::GpuDevfreq);
            assert(tx == "tx-ack");
            assert(reason == "thermal-emergency");
            const auto active = coordinator.active_leases(); // Would deadlock if callback held the lock.
            assert(active.size() == 1 && active.front().preemption_requested);
            acknowledged_callbacks.fetch_add(1, std::memory_order_relaxed);
            return true;
        }));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::GpuDevfreq, "AckController", "tx-ack",
        coreflow::ControllerPriority::WorkloadOptimization));
    coordinator.emergency_preempt_all("thermal-emergency");
    assert(acknowledged_callbacks.load(std::memory_order_relaxed) == 1);
    assert(coordinator.active_leases().empty());
    coordinator.unregister_preemption_handler("AckController");

    std::atomic<uint32_t> refused_callbacks{0};
    assert(coordinator.register_preemption_handler("RefusingController",
        [&](coreflow::MutationDomain domain, std::string_view tx, std::string_view) {
            assert(domain == coreflow::MutationDomain::CpuPolicy);
            assert(tx == "tx-refuse");
            refused_callbacks.fetch_add(1, std::memory_order_relaxed);
            return false;
        }));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::CpuPolicy, "RefusingController", "tx-refuse",
        coreflow::ControllerPriority::BackgroundEfficiency));
    assert(!coordinator.acquire_lease(
        coreflow::MutationDomain::CpuPolicy, "EmergencyRequester", "tx-emergency",
        coreflow::ControllerPriority::EmergencySafety));
    assert(refused_callbacks.load(std::memory_order_relaxed) == 1);
    const auto refused_lease = coordinator.active_leases();
    assert(refused_lease.size() == 1);
    assert(refused_lease.front().owner_controller == "RefusingController");
    assert(refused_lease.front().preemption_requested);
    // A failed callback clears its dispatch marker; a later recovery pass may retry.
    coordinator.emergency_preempt_all("retry-failed-preemption");
    assert(refused_callbacks.load(std::memory_order_relaxed) == 2);
    assert(coordinator.active_leases().size() == 1);
    assert(!coordinator.acquire_lease(
        coreflow::MutationDomain::CpuPolicy, "RefusingController", "tx-refuse",
        coreflow::ControllerPriority::BackgroundEfficiency));
    coordinator.unregister_preemption_handler("RefusingController");
    coordinator.release_lease(
        coreflow::MutationDomain::CpuPolicy, "RefusingController", "tx-refuse");

    // A delayed successful acknowledgement must not release a replacement transaction.
    assert(coordinator.register_preemption_handler("RotatingController",
        [&](coreflow::MutationDomain domain, std::string_view tx, std::string_view) {
            assert(tx == "tx-old");
            coordinator.release_lease(domain, "RotatingController", "tx-old");
            assert(coordinator.acquire_lease(
                domain, "RotatingController", "tx-new",
                coreflow::ControllerPriority::ThermalStabilization));
            return true;
        }));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::GpuDevfreq, "RotatingController", "tx-old",
        coreflow::ControllerPriority::WorkloadOptimization));
    coordinator.emergency_preempt_all("rotate-transaction");
    const auto rotated_lease = coordinator.active_leases();
    assert(rotated_lease.size() == 1);
    assert(rotated_lease.front().transaction_id == "tx-new");
    assert(rotated_lease.front().active);
    coordinator.unregister_preemption_handler("RotatingController");
    coordinator.release_lease(
        coreflow::MutationDomain::GpuDevfreq, "RotatingController", "tx-new");

    // 1. Missing PSI is unavailable evidence, not a measured zero. No workload mutation is allowed.
    const auto incomplete_psi = tuner.step({
        .cpu_psi = 0.0, .memory_psi = 0.0, .io_psi = 0.0,
        .cpu_psi_valid = false, .memory_psi_valid = false, .io_psi_valid = false,
        .thermal_pressure = 42.0,
    });
    assert(incomplete_psi.last_verdict == "INCONCLUSIVE");
    assert(incomplete_psi.objective == coreflow::OptimizationObjective::HoldBaseline);
    assert(incomplete_psi.stage == "PLANNED");
    assert(get(sda_q / "scheduler") == "[none] mq-deadline kyber bfq");

    // Independent thermal evidence must still take precedence when PSI is unavailable.
    const auto thermal_without_psi = tuner.step({
        .cpu_psi = 0.0, .memory_psi = 0.0, .io_psi = 0.0,
        .cpu_psi_valid = false, .memory_psi_valid = false, .io_psi_valid = false,
        .thermal_pressure = 70.0,
    });
    assert(thermal_without_psi.objective == coreflow::OptimizationObjective::ThermalStabilization);
    assert(get(sda_q / "scheduler") == "[none] mq-deadline kyber bfq");

    // 1a. Idle equilibrium: no need -> must hold baseline without mutating
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
    // A new trial must not inherit the preceding mq-deadline REGRESSED verdict or delta.
    assert(observing2.last_verdict == "PENDING");
    assert(observing2.verdict == coreflow::TunerVerdict::Pending);
    assert(observing2.last_utility_delta == 0.0);
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
    const auto ownership_lost_snapshot = tuner.snapshot();
    const auto quarantined_bfq = std::find_if(
        ownership_lost_snapshot.candidates.begin(), ownership_lost_snapshot.candidates.end(),
        [](const coreflow::CandidateStats& candidate) { return candidate.id == "io.bfq_interactive_fairness"; });
    assert(quarantined_bfq != ownership_lost_snapshot.candidates.end());
    assert(quarantined_bfq->ownership_loss_count == 1);
    assert(quarantined_bfq->ownership_quarantine_ticks == 8);
    assert(quarantined_bfq->cooldown_ticks >= 8);
    assert(!tuner.is_candidate_eligible("io.bfq_interactive_fairness"));
    // Even a completely different workload/thermal context cannot bypass ownership quarantine.
    assert(!tuner.is_candidate_eligible_for_context(
        "io.bfq_interactive_fairness", coreflow::WorkloadClass::Gaming, 4));
    // Verify durable serialization by constructing a fresh tuner instance against
    // the same state directory and capability fingerprint (not merely rereading
    // into already-populated in-memory CandidateStats).
    {
        coreflow::AutonomousTuner reloaded_tuner(sys_root.string(), proc_root.string());
        const auto reloaded = reloaded_tuner.snapshot();
        const auto persisted_bfq = std::find_if(
            reloaded.candidates.begin(), reloaded.candidates.end(),
            [](const coreflow::CandidateStats& candidate) { return candidate.id == "io.bfq_interactive_fairness"; });
        assert(persisted_bfq != reloaded.candidates.end());
        assert(persisted_bfq->ownership_loss_count == 1);
        assert(persisted_bfq->ownership_quarantine_ticks == 8);
        assert(!reloaded_tuner.is_candidate_eligible("io.bfq_interactive_fairness"));
        assert(!reloaded_tuner.is_candidate_eligible_for_context(
            "io.bfq_interactive_fairness", coreflow::WorkloadClass::Gaming, 4));
    }
    // External writer's value (512) is preserved, while our other node (scheduler) is rolled back to "none"
    assert(get(sda_q / "read_ahead_kb") == "512");
    assert(get(sda_q / "scheduler") == "none");
    put(sda_q / "read_ahead_kb", "128");
    tuner.reset();

    // The ownership-loss scenario deliberately adopts the external value (512) as
    // the new factory baseline. This test restores the mock node to 128 to start a
    // fresh fixture, so reinitialize the store before later cases recapture baselines.
    baseline.reinitialize();
    assert(baseline.recover_pending());
    tuner.discover(sys_root.string(), proc_root.string());

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

    // 9. Section 13.1: Resource Permission Classification (VerifiedConstraints vs WritableButUnsafe vs UnknownSemantic)
    put(sys_root / "block/custom0/queue/scheduler", "mystery_token");
    tuner.discover(sys_root.string(), proc_root.string());
    bool found_sda_verified = false;
    bool found_zram_unsafe = false;
    bool found_custom_unknown = false;
    for (const auto& res : tuner.snapshot().discovered_resources) {
        if (res.path.find("block/sda/queue/scheduler") != std::string::npos) {
            assert(res.permission_class == coreflow::ResourcePermissionClass::VerifiedConstraints);
            found_sda_verified = true;
        } else if (res.path.find("block/zram0/queue/scheduler") != std::string::npos) {
            assert(res.permission_class == coreflow::ResourcePermissionClass::WritableButUnsafe);
            found_zram_unsafe = true;
        } else if (res.path.find("block/custom0/queue/scheduler") != std::string::npos) {
            assert(res.permission_class == coreflow::ResourcePermissionClass::UnknownSemantic);
            found_custom_unknown = true;
        }
    }
    assert(found_sda_verified && found_zram_unsafe && found_custom_unknown);

    // 10. Section 13.2: Adaptive Step Sizing, Context-Aware Anti-Repeat & CFTS2 Persistence
    put(sda_q / "scheduler", "[none] mq-deadline");
    put(sda_q / "read_ahead_kb", "128");
    put(sda_q / "iosched/read_expire", "500");
    tuner.discover(sys_root.string(), proc_root.string());
    tuner.step({.cpu_psi = 6.0, .memory_psi = 0.5, .io_psi = 7.0, .thermal_pressure = 46.0});
    assert(tuner.snapshot().active_candidate_id == "io.mq_deadline_low_latency");
    assert(get(sda_q / "read_ahead_kb") == "256"); // 1.0 adaptive_scale -> full 2.0x = 256
    for (int i = 0; i < 3; ++i) {
        tuner.step({.cpu_psi = 7.0, .memory_psi = 0.5, .io_psi = 12.0, .thermal_pressure = 48.0});
    }
    assert(tuner.snapshot().verdict == coreflow::TunerVerdict::Regressed);
    // Verify CFTS2 persisted adaptive_scale < 1.0 and rollback reason across discover()
    put(sda_q / "scheduler", "[none] mq-deadline");
    tuner.discover(sys_root.string(), proc_root.string());
    double mq_scale = 1.0;
    for (const auto& c : tuner.snapshot().candidates) {
        if (c.id == "io.mq_deadline_low_latency") {
            mq_scale = c.adaptive_scale;
            assert(!c.last_rollback_reason.empty());
        }
    }
    assert(mq_scale < 1.0);
    tuner.reset();

    // 11. Section 13.4: Active Thermal Stabilization Mode & Hysteresis Recovery
    put(policy0_dir / "scaling_governor", "schedutil");
    put(schedutil_dir / "up_rate_limit_us", "2000");
    put(schedutil_dir / "down_rate_limit_us", "1000");
    tuner.discover(sys_root.string(), proc_root.string());
    const auto therm_stab = tuner.step({
        .cpu_psi = 11.0,
        .memory_psi = 0.5,
        .io_psi = 1.0,
        .thermal_pressure = 68.0,
        .thermal_guard_active = true,
        .thermal_emergency = false,
        .thermal_headroom = 0.15,
        .thermal_trend_celsius = 0.9,
    });
    assert(therm_stab.workload_class == coreflow::WorkloadClass::ThermalConstrained);
    assert(therm_stab.objective == coreflow::OptimizationObjective::ThermalStabilization);
    assert(therm_stab.intent == "ThermalStabilization");
    assert(therm_stab.active_candidate_id == "thermal.schedutil_damped_pacing");
    assert(therm_stab.thermal_stabilization_active);
    assert(get(schedutil_dir / "up_rate_limit_us") == "4000");
    assert(get(schedutil_dir / "down_rate_limit_us") == "500");

    // Emergency thermal spike must request preemption from every live domain owner
    // before the tuner attempts to roll back its own transaction.
    auto& emergency_coord = coreflow::CentralMutationCoordinator::instance();
    assert(emergency_coord.acquire_lease(
        coreflow::MutationDomain::UclampCgroup,
        "ExternalUclampController",
        "tx-uclamp-before-emergency",
        coreflow::ControllerPriority::WorkloadOptimization));
    const auto therm_emerg = tuner.step({
        .cpu_psi = 11.0,
        .memory_psi = 0.5,
        .io_psi = 1.0,
        .thermal_pressure = 92.0,
        .thermal_guard_active = true,
        .thermal_emergency = true,
    });
    assert(therm_emerg.objective == coreflow::OptimizationObjective::EmergencyMitigation);
    assert(therm_emerg.stage == "PLANNED");
    assert(get(schedutil_dir / "up_rate_limit_us") == "2000");
    assert(get(schedutil_dir / "down_rate_limit_us") == "1000");
    const auto emergency_leases = emergency_coord.active_leases();
    assert(emergency_leases.size() == 1);
    assert(emergency_leases[0].owner_controller == "ExternalUclampController");
    assert(emergency_leases[0].transaction_id == "tx-uclamp-before-emergency");
    assert(emergency_leases[0].preemption_requested);
    emergency_coord.release_lease(
        coreflow::MutationDomain::UclampCgroup, "ExternalUclampController", "tx-uclamp-before-emergency");

    // P0 regression: failed rollback must preserve recovery state and domain ownership.
    const auto recovery_start = tuner.step({
        .cpu_psi = 11.0,
        .memory_psi = 0.5,
        .io_psi = 1.0,
        .thermal_pressure = 68.0,
        .thermal_guard_active = true,
    });
    assert(recovery_start.thermal_stabilization_active);
    const fs::path up_rate_path = schedutil_dir / "up_rate_limit_us";
    assert(get(up_rate_path) == "4000");
    assert(std::filesystem::remove(up_rate_path));
    tuner.reset(); // rollback of the missing node must fail closed
    const auto recovery_snapshot = tuner.snapshot();
    assert(recovery_snapshot.recovery_required);
    const auto recovery_leases = coreflow::CentralMutationCoordinator::instance().active_leases();
    assert(recovery_leases.size() == 1);
    assert(recovery_leases[0].owner_controller.rfind("AutonomousTuner@", 0) == 0);
    assert(recovery_leases[0].domain == coreflow::MutationDomain::CpuPolicy);

    // Emergency must still preempt other owners even when the recovery retry fails early.
    auto& recovery_coord = coreflow::CentralMutationCoordinator::instance();
    assert(recovery_coord.acquire_lease(
        coreflow::MutationDomain::GpuDevfreq,
        "ExternalGpuController",
        "tx-gpu-during-recovery",
        coreflow::ControllerPriority::WorkloadOptimization));
    const auto emergency_during_recovery = tuner.step({
        .thermal_pressure = 92.0,
        .thermal_emergency = true,
    });
    assert(emergency_during_recovery.recovery_required);
    const auto leases_during_recovery = recovery_coord.active_leases();
    bool gpu_preemption_requested = false;
    for (const auto& lease : leases_during_recovery) {
        if (lease.owner_controller == "ExternalGpuController" &&
            lease.transaction_id == "tx-gpu-during-recovery") {
            gpu_preemption_requested = lease.preemption_requested;
        }
    }
    assert(gpu_preemption_requested);
    recovery_coord.release_lease(
        coreflow::MutationDomain::GpuDevfreq, "ExternalGpuController", "tx-gpu-during-recovery");

    // Recreated path has a new inode and already contains OEM baseline; adopt it safely.
    put(up_rate_path, "2000");
    tuner.reset();
    assert(!tuner.snapshot().recovery_required);
    assert(coreflow::CentralMutationCoordinator::instance().active_leases().empty());
    assert(get(up_rate_path) == "2000");

    // 12. Section 13.5: Central Mutation Coordinator Conflict Detection & Priority Arbitration
    tuner.reset();
    auto& coord = coreflow::CentralMutationCoordinator::instance();
    coord.reset();
    std::string conflict_msg;
    assert(coord.acquire_lease(
        coreflow::MutationDomain::BlockIo,
        "ExternalSafetyGuard",
        "tx-lock-io",
        coreflow::ControllerPriority::EmergencySafety,
        &conflict_msg));
    assert(!tuner.is_candidate_eligible("io.mq_deadline_low_latency"));
    coord.release_lease(coreflow::MutationDomain::BlockIo, "ExternalSafetyGuard", "tx-lock-io");
    assert(tuner.is_candidate_eligible("io.mq_deadline_low_latency"));

    // A higher-priority request must not steal a lease from a live transaction.
    coord.reset();
    assert(coord.acquire_lease(
        coreflow::MutationDomain::CpuPolicy,
        "BackgroundController",
        "tx-background-1",
        coreflow::ControllerPriority::BackgroundEfficiency));
    assert(!coord.acquire_lease(
        coreflow::MutationDomain::CpuPolicy,
        "EmergencyController",
        "tx-emergency-1",
        coreflow::ControllerPriority::EmergencySafety,
        &conflict_msg));
    auto pending_preemption = coord.active_leases();
    assert(pending_preemption.size() == 1);
    assert(pending_preemption[0].active);
    assert(pending_preemption[0].owner_controller == "BackgroundController");
    assert(pending_preemption[0].transaction_id == "tx-background-1");
    assert(pending_preemption[0].preemption_requested);

    // Emergency arbitration requests owner resolution; it must not erase the lease.
    coord.emergency_preempt_all("thermal-emergency");
    pending_preemption = coord.active_leases();
    assert(pending_preemption.size() == 1);
    assert(pending_preemption[0].active);
    assert(pending_preemption[0].owner_controller == "BackgroundController");
    assert(pending_preemption[0].transaction_id == "tx-background-1");
    coord.release_lease(coreflow::MutationDomain::CpuPolicy, "BackgroundController", "tx-background-1");
    assert(coord.active_leases().empty());

    // 13. Thermal Decision Correctness & False-Positive Elimination (TASK 01)
    // 13.1 Constant thermal pressure must NOT yield false-positive IMPROVED
    tuner.reset();
    tuner.discover(sys_root.string(), proc_root.string());
    const auto t1_start = tuner.step({
        .cpu_psi = 10.0,
        .memory_psi = 0.5,
        .io_psi = 1.0,
        .thermal_pressure = 68.0,
        .current_temperature_celsius = 45.0,
        .temperature_velocity_celsius_per_sec = 0.9,
        .temperature_velocity_valid = true,
        .temperature_samples_count = 3,
        .thermal_guard_active = true,
    });
    assert(t1_start.thermal_stabilization_active);
    assert(t1_start.stage == "OBSERVING");
    // Feed 3 observation ticks with constant pressure (68.0) and zero temp trend
    for (int i = 0; i < 3; ++i) {
        tuner.step({
            .cpu_psi = 10.0,
            .memory_psi = 0.5,
            .io_psi = 1.0,
            .thermal_pressure = 68.0,
            .current_temperature_celsius = 45.0,
            .temperature_velocity_celsius_per_sec = 0.0,
            .temperature_velocity_valid = true,
            .temperature_samples_count = 3,
            .thermal_guard_active = true,
        });
    }
    const auto t1_eval = tuner.snapshot();
    assert(t1_eval.last_verdict != "IMPROVED");
    assert(t1_eval.last_verdict == "NEUTRAL");
    assert(t1_eval.stage == "PLANNED");

    // 13.2 Zero temperature trend is NOT evidence of cooling
    // Evaluated in 13.1 above: utility delta was 0.0, receiving 0 trend bonus and resulting in NEUTRAL rollback.
    assert(t1_eval.utility_delta < 0.25);

    // 13.3 Negative temperature trend indicates genuine cooling when samples are valid
    tuner.reset();
    tuner.discover(sys_root.string(), proc_root.string());
    tuner.step({
        .cpu_psi = 10.0,
        .memory_psi = 0.5,
        .io_psi = 1.0,
        .thermal_pressure = 68.0,
        .current_temperature_celsius = 46.0,
        .temperature_velocity_celsius_per_sec = 0.9,
        .temperature_velocity_valid = true,
        .temperature_samples_count = 3,
        .thermal_guard_active = true,
    });
    for (int i = 0; i < 3; ++i) {
        tuner.step({
            .cpu_psi = 10.0,
            .memory_psi = 0.5,
            .io_psi = 1.0,
            .thermal_pressure = 67.0,
            .current_temperature_celsius = 45.0,
            .temperature_velocity_celsius_per_sec = -0.35,
            .temperature_velocity_valid = true,
            .temperature_samples_count = 3,
            .thermal_guard_active = true,
        });
    }
    const auto t3_eval = tuner.snapshot();
    assert(t3_eval.last_verdict == "IMPROVED");
    assert(t3_eval.stage == "RETAINED");

    // 13.4 Positive temperature trend (warming) must NEVER be considered improved
    tuner.reset();
    tuner.discover(sys_root.string(), proc_root.string());
    tuner.step({
        .cpu_psi = 10.0,
        .memory_psi = 0.5,
        .io_psi = 1.0,
        .thermal_pressure = 68.0,
        .current_temperature_celsius = 44.0,
        .temperature_velocity_celsius_per_sec = 0.9,
        .temperature_velocity_valid = true,
        .temperature_samples_count = 3,
        .thermal_guard_active = true,
    });
    // Even if thermal_pressure decreases by 4.0 points, temperature is warming by +0.40 °C/s
    for (int i = 0; i < 3; ++i) {
        tuner.step({
            .cpu_psi = 10.0,
            .memory_psi = 0.5,
            .io_psi = 1.0,
            .thermal_pressure = 64.0,
            .current_temperature_celsius = 45.5,
            .temperature_velocity_celsius_per_sec = 0.40,
            .temperature_velocity_valid = true,
            .temperature_samples_count = 3,
            .thermal_guard_active = true,
        });
    }
    const auto t4_eval = tuner.snapshot();
    assert(t4_eval.last_verdict != "IMPROVED");
    assert(t4_eval.stage == "PLANNED");

    // 13.5 Sensor unavailable yields INCONCLUSIVE
    tuner.reset();
    tuner.discover(sys_root.string(), proc_root.string());
    tuner.step({
        .cpu_psi = 10.0,
        .memory_psi = 0.5,
        .io_psi = 1.0,
        .thermal_pressure = 68.0,
        .current_temperature_celsius = 45.0,
        .temperature_velocity_celsius_per_sec = 0.9,
        .temperature_velocity_valid = true,
        .temperature_samples_count = 3,
        .thermal_guard_active = true,
    });
    for (int i = 0; i < 3; ++i) {
        tuner.step({
            .cpu_psi = 10.0,
            .memory_psi = 0.5,
            .io_psi = 1.0,
            .thermal_pressure = 67.0,
            .current_temperature_celsius = -1.0, // Sensor unavailable
            .temperature_velocity_celsius_per_sec = 0.0,
            .temperature_velocity_valid = false,
            .temperature_samples_count = 0,
            .thermal_guard_active = true,
        });
    }
    const auto t5_eval = tuner.snapshot();
    assert(t5_eval.last_verdict == "INCONCLUSIVE");
    assert(t5_eval.stage == "PLANNED");

    // 13.6 Insufficient sample count yields INCONCLUSIVE
    tuner.reset();
    tuner.discover(sys_root.string(), proc_root.string());
    tuner.step({
        .cpu_psi = 10.0,
        .memory_psi = 0.5,
        .io_psi = 1.0,
        .thermal_pressure = 68.0,
        .current_temperature_celsius = 45.0,
        .temperature_velocity_celsius_per_sec = 0.9,
        .temperature_velocity_valid = true,
        .temperature_samples_count = 3,
        .thermal_guard_active = true,
    });
    for (int i = 0; i < 3; ++i) {
        tuner.step({
            .cpu_psi = 10.0,
            .memory_psi = 0.5,
            .io_psi = 1.0,
            .thermal_pressure = 67.0,
            .current_temperature_celsius = 45.0,
            .temperature_velocity_celsius_per_sec = -0.20,
            .temperature_velocity_valid = false, // Not enough samples
            .temperature_samples_count = 1,
            .thermal_guard_active = true,
        });
    }
    const auto t6_eval = tuner.snapshot();
    assert(t6_eval.last_verdict == "INCONCLUSIVE");
    assert(t6_eval.stage == "PLANNED");

    // 13.7 Pressure velocity and temperature velocity must NOT be swapped
    tuner.reset();
    tuner.discover(sys_root.string(), proc_root.string());
    tuner.step({
        .cpu_psi = 10.0,
        .memory_psi = 0.5,
        .io_psi = 1.0,
        .thermal_pressure = 68.0,
        .current_temperature_celsius = 45.0,
        .temperature_velocity_celsius_per_sec = 0.9,
        .temperature_velocity_valid = true,
        .temperature_samples_count = 3,
        .thermal_guard_active = true,
    });
    // Negative pressure velocity (-2.5 /s) but positive temperature velocity (+0.45 °C/s)
    for (int i = 0; i < 3; ++i) {
        tuner.step({
            .cpu_psi = 10.0,
            .memory_psi = 0.5,
            .io_psi = 1.0,
            .thermal_pressure = 68.0,
            .thermal_pressure_velocity = -2.5,
            .current_temperature_celsius = 45.5,
            .temperature_velocity_celsius_per_sec = 0.45,
            .temperature_velocity_valid = true,
            .temperature_samples_count = 3,
            .thermal_guard_active = true,
        });
    }
    const auto t7_eval = tuner.snapshot();
    assert(t7_eval.last_verdict != "IMPROVED");
    assert(t7_eval.stage == "PLANNED");

    // 13.8 Sub-noise sensor jitter does NOT yield false-positive IMPROVED
    tuner.reset();
    tuner.discover(sys_root.string(), proc_root.string());
    tuner.step({
        .cpu_psi = 10.0,
        .memory_psi = 0.5,
        .io_psi = 1.0,
        .thermal_pressure = 68.0,
        .current_temperature_celsius = 45.0,
        .temperature_velocity_celsius_per_sec = 0.9,
        .temperature_velocity_valid = true,
        .temperature_samples_count = 3,
        .thermal_guard_active = true,
    });
    // Trend within noise floor (-0.02 °C/s) and pressure variation within noise
    for (int i = 0; i < 3; ++i) {
        tuner.step({
            .cpu_psi = 10.0,
            .memory_psi = 0.5,
            .io_psi = 1.0,
            .thermal_pressure = 67.5,
            .current_temperature_celsius = 45.0,
            .temperature_velocity_celsius_per_sec = -0.02,
            .temperature_velocity_valid = true,
            .temperature_samples_count = 3,
            .thermal_guard_active = true,
        });
    }
    const auto t8_eval = tuner.snapshot();
    assert(t8_eval.last_verdict != "IMPROVED");
    assert(t8_eval.last_verdict == "NEUTRAL");

    // 13.9 Thermal pressure drop must NOT mask workload regression
    tuner.reset();
    tuner.discover(sys_root.string(), proc_root.string());
    tuner.step({
        .cpu_psi = 10.0,
        .memory_psi = 0.5,
        .io_psi = 1.0,
        .thermal_pressure = 68.0,
        .current_temperature_celsius = 46.0,
        .temperature_velocity_celsius_per_sec = 0.9,
        .temperature_velocity_valid = true,
        .temperature_samples_count = 3,
        .thermal_guard_active = true,
    });
    // Thermal pressure dropped significantly (60.0) and cooling (-0.30 °C/s),
    // but io_psi spiked from 1.0 to 12.0 (massive stall)
    for (int i = 0; i < 3; ++i) {
        tuner.step({
            .cpu_psi = 10.0,
            .memory_psi = 0.5,
            .io_psi = 12.0,
            .thermal_pressure = 60.0,
            .current_temperature_celsius = 45.0,
            .temperature_velocity_celsius_per_sec = -0.30,
            .temperature_velocity_valid = true,
            .temperature_samples_count = 3,
            .thermal_guard_active = true,
        });
    }
    const auto t9_eval = tuner.snapshot();
    assert(t9_eval.last_verdict == "REGRESSED");
    assert(t9_eval.stage == "PLANNED");

    tuner.reset();

    // A separate tuner instance verifies that coordinator preemption invokes
    // its registered callback and restores the active candidate before releasing.
    {
        put(sda_q / "scheduler", "[none] mq-deadline kyber bfq");
        coreflow::AutonomousTuner preemption_tuner(sys_root.string(), proc_root.string());
        preemption_tuner.reset();
        const auto active = preemption_tuner.step({
            .cpu_psi = 6.0, .memory_psi = 0.5, .io_psi = 7.5, .thermal_pressure = 48.0});
        assert(active.stage == "OBSERVING");
        assert(active.active_candidate_id.rfind("io.", 0) == 0);
        const auto tuner_lease = coordinator.active_leases();
        assert(tuner_lease.size() == 1);
        assert(tuner_lease.front().domain == coreflow::MutationDomain::BlockIo);
        assert(tuner_lease.front().owner_controller.rfind("AutonomousTuner@", 0) == 0);
        coordinator.begin_emergency_recovery();
        coordinator.emergency_preempt_all("test-autonomous-tuner-preemption");
        const auto preempted = preemption_tuner.snapshot();
        assert(preempted.stage == "PLANNED");
        assert(preempted.last_verdict == "ABORTED");
        assert(preempted.verdict == coreflow::TunerVerdict::Aborted);
        assert(coordinator.active_leases().empty());
        assert(coordinator.complete_emergency_recovery());
        assert(get(sda_q / "scheduler") == "none");
        assert(get(sda_q / "iosched/read_expire") == "500");
        assert(get(sda_q / "read_ahead_kb") == "128");
        assert(coordinator.active_leases().empty());
        // The public emergency API must reopen admission only after the tuner
        // and every other registered owner have acknowledged verified rollback.
        assert(coordinator.acquire_lease(
            coreflow::MutationDomain::ThermalCore, "PostEmergencyAdmission",
            "post-emergency-tx", coreflow::ControllerPriority::EmergencySafety));
        coordinator.release_lease(
            coreflow::MutationDomain::ThermalCore, "PostEmergencyAdmission", "post-emergency-tx");
    }

    // One controller identity cannot silently replace an already-registered callback.
    coordinator.reset();
    std::atomic<uint32_t> original_handler_calls{0};
    assert(coordinator.register_preemption_handler("SingleHandlerController",
        [&](coreflow::MutationDomain, std::string_view, std::string_view) {
            original_handler_calls.fetch_add(1, std::memory_order_relaxed);
            return false;
        }));
    assert(!coordinator.register_preemption_handler("SingleHandlerController",
        [](coreflow::MutationDomain, std::string_view, std::string_view) {
            return true;
        }));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::CpuPolicy, "SingleHandlerController", "single-handler-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    coordinator.emergency_preempt_all("single-handler-registration-test");
    assert(original_handler_calls.load(std::memory_order_relaxed) == 1);
    const auto single_handler_lease = coordinator.active_leases();
    assert(single_handler_lease.size() == 1);
    assert(single_handler_lease.front().preemption_requested);
    coordinator.release_lease(
        coreflow::MutationDomain::CpuPolicy, "SingleHandlerController", "single-handler-tx");
    coordinator.unregister_preemption_handler("SingleHandlerController");

    // Concurrent emergency dispatchers must not execute the same owner's rollback
    // callback twice for one lease. A failed callback is retryable (covered above).
    coordinator.reset();
    std::mutex dispatch_mutex;
    std::condition_variable dispatch_cv;
    bool dispatch_callback_entered = false;
    bool dispatch_callback_release = false;
    std::atomic<uint32_t> concurrent_callback_calls{0};
    assert(coordinator.register_preemption_handler("ConcurrentDispatchController",
        [&](coreflow::MutationDomain, std::string_view, std::string_view) {
            concurrent_callback_calls.fetch_add(1, std::memory_order_relaxed);
            std::unique_lock<std::mutex> lock(dispatch_mutex);
            dispatch_callback_entered = true;
            dispatch_cv.notify_all();
            dispatch_cv.wait(lock, [&] { return dispatch_callback_release; });
            return true;
        }));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::CpuPolicy, "ConcurrentDispatchController", "concurrent-dispatch-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    std::thread first_dispatcher([&] {
        coordinator.emergency_preempt_all("concurrent-dispatch-first");
    });
    {
        std::unique_lock<std::mutex> lock(dispatch_mutex);
        dispatch_cv.wait(lock, [&] { return dispatch_callback_entered; });
    }
    std::thread second_dispatcher([&] {
        coordinator.emergency_preempt_all("concurrent-dispatch-second");
    });
    second_dispatcher.join();
    assert(concurrent_callback_calls.load(std::memory_order_relaxed) == 1);
    const auto in_flight_lease = coordinator.active_leases();
    assert(in_flight_lease.size() == 1);
    assert(in_flight_lease.front().preemption_dispatch_in_flight);
    {
        std::lock_guard<std::mutex> lock(dispatch_mutex);
        dispatch_callback_release = true;
    }
    dispatch_cv.notify_all();
    first_dispatcher.join();
    assert(coordinator.active_leases().empty());
    assert(concurrent_callback_calls.load(std::memory_order_relaxed) == 1);
    coordinator.unregister_preemption_handler("ConcurrentDispatchController");

    // One callback object may own several domains, but the coordinator must not
    // invoke that same object concurrently for different transactions either.
    coordinator.reset();
    std::mutex multi_domain_mutex;
    std::condition_variable multi_domain_cv;
    bool cpu_callback_entered = false;
    bool release_cpu_callback = false;
    std::atomic<uint32_t> multi_domain_callback_calls{0};
    assert(coordinator.register_preemption_handler("MultiDomainController",
        [&](coreflow::MutationDomain domain, std::string_view, std::string_view) {
            multi_domain_callback_calls.fetch_add(1, std::memory_order_relaxed);
            if (domain == coreflow::MutationDomain::CpuPolicy) {
                std::unique_lock<std::mutex> lock(multi_domain_mutex);
                cpu_callback_entered = true;
                multi_domain_cv.notify_all();
                multi_domain_cv.wait(lock, [&] { return release_cpu_callback; });
            }
            return true;
        }));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::CpuPolicy, "MultiDomainController", "multi-cpu-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::GpuDevfreq, "MultiDomainController", "multi-gpu-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    std::thread multi_domain_first([&] {
        coordinator.emergency_preempt_all("multi-domain-first-dispatch");
    });
    {
        std::unique_lock<std::mutex> lock(multi_domain_mutex);
        multi_domain_cv.wait(lock, [&] { return cpu_callback_entered; });
    }
    std::thread multi_domain_second([&] {
        coordinator.emergency_preempt_all("multi-domain-second-dispatch");
    });
    multi_domain_second.join();
    assert(multi_domain_callback_calls.load(std::memory_order_relaxed) == 1);
    const auto multi_domain_pending = coordinator.active_leases();
    assert(multi_domain_pending.size() == 2);
    assert(multi_domain_pending[0].preemption_dispatch_in_flight);
    assert(!multi_domain_pending[1].preemption_dispatch_in_flight);
    {
        std::lock_guard<std::mutex> lock(multi_domain_mutex);
        release_cpu_callback = true;
    }
    multi_domain_cv.notify_all();
    multi_domain_first.join();
    assert(coordinator.active_leases().empty());
    assert(multi_domain_callback_calls.load(std::memory_order_relaxed) == 2);
    coordinator.unregister_preemption_handler("MultiDomainController");

    // Repeat the multi-domain overlap under competing emergency dispatchers.
    // Condition variables force the critical interleaving; no scheduling sleep
    // is used to make the expected callback count deterministic.
    for (uint32_t round = 0; round < 24; ++round) {
        coordinator.reset();
        const std::string owner = "RepeatedDispatchController@" + std::to_string(round);
        std::mutex stress_mutex;
        std::condition_variable stress_cv;
        bool first_callback_entered = false;
        bool release_first_callback = false;
        std::atomic<uint32_t> stress_callback_calls{0};
        assert(coordinator.register_preemption_handler(owner,
            [&](coreflow::MutationDomain, std::string_view, std::string_view) {
                const uint32_t call = stress_callback_calls.fetch_add(1, std::memory_order_relaxed);
                if (call == 0) {
                    std::unique_lock<std::mutex> lock(stress_mutex);
                    first_callback_entered = true;
                    stress_cv.notify_all();
                    stress_cv.wait(lock, [&] { return release_first_callback; });
                }
                return true;
            }));
        assert(coordinator.acquire_lease(
            coreflow::MutationDomain::CpuPolicy, owner, "stress-cpu-" + std::to_string(round),
            coreflow::ControllerPriority::WorkloadOptimization));
        assert(coordinator.acquire_lease(
            coreflow::MutationDomain::GpuDevfreq, owner, "stress-gpu-" + std::to_string(round),
            coreflow::ControllerPriority::WorkloadOptimization));
        std::thread primary_dispatcher([&] {
            coordinator.emergency_preempt_all("repeated-multi-domain-primary");
        });
        {
            std::unique_lock<std::mutex> lock(stress_mutex);
            stress_cv.wait(lock, [&] { return first_callback_entered; });
        }
        std::vector<std::thread> competing_dispatchers;
        for (uint32_t contender = 0; contender < 4; ++contender) {
            competing_dispatchers.emplace_back([&] {
                coordinator.emergency_preempt_all("repeated-multi-domain-contender");
            });
        }
        for (auto& dispatcher : competing_dispatchers) dispatcher.join();
        assert(stress_callback_calls.load(std::memory_order_relaxed) == 1);
        const auto stress_pending = coordinator.active_leases();
        assert(stress_pending.size() == 2);
        {
            std::lock_guard<std::mutex> lock(stress_mutex);
            release_first_callback = true;
        }
        stress_cv.notify_all();
        primary_dispatcher.join();
        assert(stress_callback_calls.load(std::memory_order_relaxed) == 2);
        assert(coordinator.active_leases().empty());
        coordinator.unregister_preemption_handler(owner);
    }

    // Admission cannot reopen merely because a callback released its exact
    // lease before returning its acknowledgement. The callback itself must drain.
    coordinator.reset();
    std::mutex barrier_callback_mutex;
    std::condition_variable barrier_callback_cv;
    bool barrier_callback_entered = false;
    bool barrier_callback_release = false;
    assert(coordinator.register_preemption_handler("EmergencyCompletionController",
        [&](coreflow::MutationDomain domain, std::string_view tx, std::string_view) {
            coordinator.release_lease(
                domain, "EmergencyCompletionController", tx);
            std::unique_lock<std::mutex> lock(barrier_callback_mutex);
            barrier_callback_entered = true;
            barrier_callback_cv.notify_all();
            barrier_callback_cv.wait(lock, [&] { return barrier_callback_release; });
            return true;
        }));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::CpuPolicy, "EmergencyCompletionController", "completion-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    coordinator.begin_emergency_recovery();
    std::thread barrier_dispatcher([&] {
        coordinator.emergency_preempt_all("callback-drain-barrier-test");
    });
    {
        std::unique_lock<std::mutex> lock(barrier_callback_mutex);
        barrier_callback_cv.wait(lock, [&] { return barrier_callback_entered; });
    }
    assert(coordinator.active_leases().empty());
    assert(coordinator.is_emergency_recovery_active());
    assert(!coordinator.complete_emergency_recovery());
    {
        std::lock_guard<std::mutex> lock(barrier_callback_mutex);
        barrier_callback_release = true;
    }
    barrier_callback_cv.notify_all();
    barrier_dispatcher.join();
    assert(coordinator.complete_emergency_recovery());
    coordinator.unregister_preemption_handler("EmergencyCompletionController");

    // Handler teardown must wait for callbacks already copied by dispatch.
    coordinator.reset();
    std::mutex callback_mutex;
    std::condition_variable callback_cv;
    bool callback_entered = false;
    bool callback_release = false;
    std::atomic<bool> unregister_started{false};
    std::atomic<bool> unregister_finished{false};
    assert(coordinator.register_preemption_handler("LifetimeController",
        [&](coreflow::MutationDomain, std::string_view, std::string_view) {
            std::unique_lock<std::mutex> lock(callback_mutex);
            callback_entered = true;
            callback_cv.notify_all();
            callback_cv.wait(lock, [&] { return callback_release; });
            return true;
        }));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::CpuPolicy, "LifetimeController", "lifetime-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    std::thread dispatcher([&] { coordinator.emergency_preempt_all("lifetime-test"); });
    {
        std::unique_lock<std::mutex> lock(callback_mutex);
        callback_cv.wait(lock, [&] { return callback_entered; });
    }
    std::thread unregisterer([&] {
        unregister_started.store(true, std::memory_order_release);
        coordinator.unregister_preemption_handler("LifetimeController");
        unregister_finished.store(true, std::memory_order_release);
    });
    while (!unregister_started.load(std::memory_order_acquire)) std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    assert(!unregister_finished.load(std::memory_order_acquire));
    {
        std::lock_guard<std::mutex> lock(callback_mutex);
        callback_release = true;
    }
    callback_cv.notify_all();
    dispatcher.join();
    unregisterer.join();
    assert(unregister_finished.load(std::memory_order_acquire));
    assert(coordinator.active_leases().empty());

    // Emergency recovery closes admission before asking active owners to roll back.
    // A new controller must not acquire a free domain until recovery completes.
    coordinator.reset();
    bool gate_acknowledged = false;
    assert(coordinator.register_preemption_handler("EmergencyGateController",
        [&](coreflow::MutationDomain, std::string_view, std::string_view) {
            gate_acknowledged = true;
            return true;
        }));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::CpuPolicy, "EmergencyGateController", "gate-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    coordinator.begin_emergency_recovery();
    assert(coordinator.is_emergency_recovery_active());
    std::string gate_conflict;
    assert(!coordinator.acquire_lease(
        coreflow::MutationDomain::GpuDevfreq, "LateController", "late-tx",
        coreflow::ControllerPriority::EmergencySafety, &gate_conflict));
    assert(gate_conflict.find("admission is closed") != std::string::npos);
    assert(!coordinator.complete_emergency_recovery()); // Active owner has not acknowledged yet.
    coordinator.emergency_preempt_all("test-emergency-admission-barrier");
    assert(gate_acknowledged);
    assert(coordinator.active_leases().empty());
    assert(coordinator.complete_emergency_recovery());
    assert(!coordinator.is_emergency_recovery_active());
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::GpuDevfreq, "LateController", "late-tx",
        coreflow::ControllerPriority::EmergencySafety));
    coordinator.release_lease(
        coreflow::MutationDomain::GpuDevfreq, "LateController", "late-tx");
    coordinator.unregister_preemption_handler("EmergencyGateController");
    assert(coordinator.active_leases().empty());

    // A controller that cannot safely restore must return a failed
    // acknowledgement. Ownership remains active and admission stays closed
    // until an explicit recovery path resolves the retained lease.
    coordinator.reset();
    assert(coordinator.register_preemption_handler("UnresolvedController",
        [](coreflow::MutationDomain, std::string_view, std::string_view) noexcept {
            return false;
        }));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::ThermalCore, "UnresolvedController", "unresolved-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    coordinator.begin_emergency_recovery();
    coordinator.emergency_preempt_all("test-failed-preemption-acknowledgement");
    const auto unresolved_leases = coordinator.active_leases();
    assert(unresolved_leases.size() == 1);
    assert(unresolved_leases.front().preemption_requested);
    assert(!coordinator.complete_emergency_recovery());
    // A failed acknowledgement from a still-registered owner must never be
    // bypassed by the detached-owner recovery API.
    assert(!coordinator.release_orphaned_leases_after_verified_recovery(baseline));
    coordinator.release_lease(
        coreflow::MutationDomain::ThermalCore, "UnresolvedController", "unresolved-tx");
    assert(coordinator.complete_emergency_recovery());
    coordinator.unregister_preemption_handler("UnresolvedController");
    assert(coordinator.active_leases().empty());

    // Controller disappearance is different from a live owner refusing
    // preemption. Its destructor unregisters the callback but deliberately
    // leaves the lease active; only a verified BaselineStore sweep may retire it.
    coordinator.reset();
    const fs::path orphan_node = sda_q / "read_ahead_kb";
    put(orphan_node, "128");
    assert(!baseline.has_active_mutations());
    assert(baseline.capture(orphan_node.string()));
    assert(baseline.apply_mutation(orphan_node.string(), "256"));
    assert(get(orphan_node) == "256");
    assert(coordinator.register_preemption_handler(
        "VanishedController",
        [](coreflow::MutationDomain, std::string_view, std::string_view) noexcept {
            return false;
        }));
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::BlockIo, "VanishedController", "vanished-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    coordinator.unregister_preemption_handler("VanishedController");
    auto orphaned = coordinator.active_leases();
    assert(orphaned.size() == 1);
    assert(orphaned.front().owner_detached);
    assert(orphaned.front().owner_controller == "VanishedController");
    // A detached identity cannot be rebound to a new callback while its old
    // lease remains unresolved; process-lifetime IDs must stay one-owner-only.
    assert(!coordinator.register_preemption_handler(
        "VanishedController",
        [](coreflow::MutationDomain, std::string_view, std::string_view) noexcept {
            return true;
        }));

    coordinator.begin_emergency_recovery();
    coordinator.emergency_preempt_all("test-detached-owner-recovery");
    orphaned = coordinator.active_leases();
    assert(orphaned.size() == 1 && orphaned.front().owner_detached);
    assert(orphaned.front().preemption_requested);
    assert(!coordinator.release_orphaned_leases_after_verified_recovery(baseline));
    assert(coordinator.active_leases().size() == 1);
    assert(baseline.has_active_mutations());
    assert(baseline.rollback_all() > 0);
    assert(!baseline.has_active_mutations());
    assert(get(orphan_node) == "128");
    assert(coordinator.release_orphaned_leases_after_verified_recovery(baseline));
    assert(coordinator.active_leases().empty());
    assert(coordinator.complete_emergency_recovery());
    assert(!coordinator.acquire_lease(
        coreflow::MutationDomain::BlockIo, "VanishedController", "reused-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    assert(!coordinator.register_preemption_handler(
        "VanishedController",
        [](coreflow::MutationDomain, std::string_view, std::string_view) noexcept {
            return true;
        }));

    // Admission is reopened only after the baseline and orphan lease are both
    // reconciled; a fresh controller with a new identity can acquire the domain.
    assert(coordinator.acquire_lease(
        coreflow::MutationDomain::BlockIo, "RecoveredController", "recovered-tx",
        coreflow::ControllerPriority::WorkloadOptimization));
    coordinator.release_lease(
        coreflow::MutationDomain::BlockIo, "RecoveredController", "recovered-tx");
    assert(coordinator.active_leases().empty());

    // Exercise detached-owner recovery through a real controller lifetime rather
    // than a synthetic callback identity. The controller commits a real tuner
    // candidate, one owned mock node disappears, and its destructor must retain
    // the unresolved lease instead of silently releasing ownership.
    coordinator.reset();
    const fs::path teardown_sys = root / "teardown_sys";
    const fs::path teardown_proc = root / "teardown_proc";
    const fs::path teardown_queue = teardown_sys / "block/sdb/queue";
    put(teardown_queue / "scheduler", "[none] mq-deadline kyber bfq");
    put(teardown_queue / "read_ahead_kb", "128");
    put(teardown_queue / "nr_requests", "64");
    put(teardown_queue / "rq_affinity", "1");
    put(teardown_queue / "iosched/read_expire", "500");
    put(teardown_queue / "iosched/writes_starved", "2");
    put(teardown_queue / "iosched/fifo_batch", "16");

    std::string disappeared_path;
    std::string disappeared_active_value;
    std::string disappeared_factory_value;
    std::string temporarily_hidden_path;
    {
        coreflow::AutonomousTuner doomed(teardown_sys.string(), teardown_proc.string());
        const auto applied = doomed.step({
            .cpu_psi = 6.0,
            .memory_psi = 0.5,
            .io_psi = 12.0,
            .thermal_pressure = 48.0,
        });
        assert(applied.stage == "OBSERVING");
        assert(applied.active_candidate_id != "baseline");

        const auto teardown_audit = baseline.get_audit_trail();
        const auto changed_node = std::find_if(
            teardown_audit.begin(), teardown_audit.end(),
            [&](const coreflow::NodeSnapshot& node) {
                return node.path.rfind(teardown_sys.string() + "/", 0) == 0 && node.is_modified;
            });
        assert(changed_node != teardown_audit.end());
        disappeared_path = changed_node->path;
        disappeared_active_value = changed_node->active_value;
        disappeared_factory_value = changed_node->factory_value;
        assert(!disappeared_active_value.empty());
        assert(!disappeared_factory_value.empty());
        // Move the same inode out of the discovered path. Reusing the original
        // inode lets BaselineStore distinguish temporary absence from an OEM
        // replacement when the path is restored after controller teardown.
        temporarily_hidden_path = disappeared_path + ".temporarily-hidden";
        fs::rename(disappeared_path, temporarily_hidden_path);
        assert(!fs::exists(disappeared_path));
        assert(fs::exists(temporarily_hidden_path));
    } // Actual AutonomousTuner destructor attempts rollback, then retires its handler.

    auto teardown_orphans = coordinator.active_leases();
    assert(teardown_orphans.size() == 1);
    assert(teardown_orphans.front().owner_detached);
    assert(teardown_orphans.front().owner_controller.rfind("AutonomousTuner@", 0) == 0);
    assert(teardown_orphans.front().preemption_requested == false);

    // Restore the exact mock node (same inode) before monitor recovery. No
    // detached controller callback exists; BaselineStore must restore and verify
    // the factory value rather than misclassifying the unavailable path as OEM
    // takeover or adopting the transiently hidden active value.
    fs::rename(temporarily_hidden_path, disappeared_path);
    assert(fs::exists(disappeared_path));
    assert(!fs::exists(temporarily_hidden_path));
    assert(get(disappeared_path) == disappeared_active_value);
    coordinator.begin_emergency_recovery();
    coordinator.emergency_preempt_all("actual-controller-teardown-recovery");
    teardown_orphans = coordinator.active_leases();
    assert(teardown_orphans.size() == 1 && teardown_orphans.front().owner_detached);
    assert(teardown_orphans.front().preemption_requested);
    assert(!coordinator.release_orphaned_leases_after_verified_recovery(baseline));
    assert(baseline.has_active_mutations());
    assert(baseline.rollback_all() > 0);
    assert(!baseline.has_active_mutations());
    assert(get(disappeared_path) == disappeared_factory_value);
    assert(baseline.verify_and_persist_clear_state());
    assert(coordinator.release_orphaned_leases_after_verified_recovery(baseline));
    assert(coordinator.active_leases().empty());
    assert(coordinator.complete_emergency_recovery());

    fs::remove_all(root);
    std::cout << "[PASS] AutonomousTuner discovery classification, coordinator arbitration, thermal stabilization, and false-positive elimination succeeded.\n";
    return 0;
}
