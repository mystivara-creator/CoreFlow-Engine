#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/decision_engine.hpp"
#include "coreflow/thermal_monitor.hpp"
#include "coreflow/worker_pool.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
using namespace coreflow;

namespace {

void put(const fs::path& path, const std::string& content) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    assert(!ec);
    std::ofstream out(path);
    assert(out.good());
    out << content << '\n';
}

std::string get(const fs::path& path) {
    std::ifstream in(path);
    std::string line;
    std::getline(in, line);
    return line;
}

NodeCapability make_runtime_node(const fs::path& path, std::string_view value) {
    NodeCapability cap{};
    cap.path = path.string();
    cap.current_value = std::string(value);
    cap.state = NodeCapabilityState::MutationEligible;
    cap.classification = KernelNodeClassification::RuntimeTunable;
    cap.semantic_id = "test.runtime." + path.filename().string();
    cap.unit = "scalar";
    cap.constraints = "0..65535";
    return cap;
}

} // namespace

int main() {
    char tmpl[] = "/tmp/coreflow-decision-engine-test-XXXXXX";
    const char* raw_dir = ::mkdtemp(tmpl);
    assert(raw_dir != nullptr);
    const fs::path root(raw_dir);
    ::setenv("COREFLOW_TEST_STATE_DIR", (root / "state").c_str(), 1);

    const fs::path sys_root = root / "sys";
    const fs::path proc_root = root / "proc";
    const fs::path sda_q = sys_root / "class/block/sda/queue";

    put(sda_q / "scheduler", "[none] mq-deadline kyber bfq");
    put(sda_q / "read_ahead_kb", "128");
    put(sda_q / "nr_requests", "64");
    put(sda_q / "rq_affinity", "1");
    put(sda_q / "iosched/read_expire", "500");
    put(sda_q / "iosched/fifo_batch", "16");

    const fs::path vm_dir = proc_root / "sys/vm";
    put(vm_dir / "page-cluster", "3");
    put(vm_dir / "vfs_cache_pressure", "100");
    put(proc_root / "swaps",
        "Filename\tType\tSize\tUsed\tPriority\n/dev/block/zram0\tpartition\t2097148\t1024\t-2\n");

    auto& baseline = BaselineStore::instance();
    baseline.reinitialize();
    assert(baseline.recover_pending());

    CentralMutationCoordinator coordinator;
    coordinator.reset();

    // =========================================================================
    // 1. Global non-waterfall arbitration: Memory > I/O when Memory dominates
    //    (Even though I/O PSI >= 2.2% would have hijacked legacy Priority A)
    // =========================================================================
    {
        AutonomousTuner tuner(coordinator, sys_root.string(), proc_root.string());
        const auto snap = tuner.step({
            .cpu_psi = 1.0,
            .memory_psi = 8.5,
            .io_psi = 2.5,
            .thermal_pressure = 44.0,
        });
        assert(snap.bottleneck == BottleneckKind::MemorySwapThrashing);
        assert(snap.stage == "OBSERVING");
        assert(!snap.evaluated_candidates.empty());
        assert(snap.evaluated_candidates.front().id == "vm.zram_latency_relief");
        assert(snap.evaluated_candidates.front().domain == "vm");
        assert(snap.active_interventions.size() == 2U);
        tuner.reset();
        assert(get(vm_dir / "page-cluster") == "3");
        assert(get(sda_q / "scheduler") == "none");
        put(sda_q / "scheduler", "[none] mq-deadline kyber bfq");
    }

    // =========================================================================
    // 2. CPU-bound Tiered Frequency dominates global evaluation over low-severity
    //    Memory & I/O candidates when TieredPolicyRouter requests a step
    // =========================================================================
    {
        GlobalDecisionEngine engine;
        std::vector<BlockQueueSurface> queues(1);
        queues[0].device_name = "sda";
        queues[0].queue_dir = sda_q.string();
        queues[0].active_scheduler = "none";
        queues[0].baseline_scheduler = "none";
        queues[0].available_schedulers = {"none", "mq-deadline", "kyber", "bfq"};
        queues[0].baseline_read_ahead_kb = 128;
        queues[0].baseline_nr_requests = 64;

        std::vector<NodeCapability> caps{
            make_runtime_node(sda_q / "scheduler", "none"),
            make_runtime_node(sda_q / "read_ahead_kb", "128"),
            make_runtime_node(sda_q / "nr_requests", "64"),
            make_runtime_node(sda_q / "rq_affinity", "1"),
            make_runtime_node(vm_dir / "page-cluster", "3"),
            make_runtime_node(vm_dir / "vfs_cache_pressure", "100"),
        };

        EnvironmentDiscoveryInput env_in{};
        env_in.sys_root = sys_root.string();
        env_in.proc_root = proc_root.string();
        env_in.zram_active = true;
        env_in.vm_page_cluster_path = (vm_dir / "page-cluster").string();
        env_in.vm_vfs_cache_pressure_path = (vm_dir / "vfs_cache_pressure").string();
        env_in.block_queues = &queues;
        env_in.kernel_node_capabilities = &caps;
        env_in.frequency_actuator_configured = true;
        env_in.frequency_policy_index = 0U;
        env_in.tiered_policy_ready = true;
        env_in.tiered_cpu_policy_owned = true;
        env_in.tiered_policy_decision.tier = OptimizationTier::Tier1InteractiveResponse;
        env_in.tiered_policy_decision.action = PolicyAction::CpuFrequencyStep;
        env_in.tiered_policy_decision.requires_mutation_admission = true;
        env_in.tiered_policy_decision.requested_capacity_scale = 1.15;
        env_in.tiered_policy_decision.confidence = 0.90;
        env_in.tiered_policy_decision.rationale = "Interactive CPU-bound spike";

        const EnvironmentCapabilities env = EnvironmentDiscovery::discover(env_in);
        const TunerTelemetryInput tel{
            .cpu_psi = 18.0,
            .memory_psi = 2.4,
            .io_psi = 2.5,
            .thermal_pressure = 46.0,
            .thermal_headroom = 0.65,
        };
        const CorrelatedEvidence ev = EvidenceCorrelator::correlate(tel, env, false);
        assert(ev.diagnosed_bottleneck == BottleneckKind::CpuSchedulingLatency);

        std::vector<CandidateStats> stats;
        for (const char* id : {
                 "io.mq_deadline_low_latency",
                 "io.kyber_token_bucket",
                 "io.bfq_interactive_fairness",
                 "vm.zram_latency_relief",
                 "cpu.frequency.tiered"}) {
            CandidateStats s{};
            s.id = id;
            s.mean_utility = 0.60;
            stats.push_back(s);
        }

        const InterventionPlan plan = engine.run_cycle(
            env, ev, stats, 0U, {}, coordinator, "TestController",
            [](const CandidateProposal&, std::string&) { return true; }, nullptr);

        assert(!plan.ranked_all.empty());
        assert(plan.ranked_all.front().proposal.id == "cpu.frequency.tiered");
        assert(plan.ranked_all.front().eligible);
        assert(plan.selected.size() == 2U);
        assert(plan.selected[0].proposal.id == "cpu.frequency.tiered");
        assert(plan.selected[0].proposal.mutation_domain == MutationDomain::CpuPolicy);
        assert(plan.selected[1].proposal.mutation_domain != MutationDomain::CpuPolicy);
    }

    // =========================================================================
    // 3. Compatible multi-domain co-execution (BlockIo + VmMemory) & isolated
    //    per-domain regression rollback (BlockIo rolls back while VmMemory stays RETAINED)
    // =========================================================================
    {
        baseline.reinitialize();
        assert(baseline.recover_pending());
        coordinator.reset();
        put(sda_q / "scheduler", "[none] mq-deadline");
        put(sda_q / "read_ahead_kb", "128");
        put(vm_dir / "page-cluster", "3");
        put(vm_dir / "vfs_cache_pressure", "100");

        AutonomousTuner tuner(coordinator, sys_root.string(), proc_root.string());
        const auto start = tuner.step({
            .cpu_psi = 3.0,
            .memory_psi = 8.0,
            .io_psi = 9.0,
            .thermal_pressure = 46.0,
        });
        assert(start.stage == "OBSERVING");
        assert(start.active_interventions.size() == 2U);
        assert(get(vm_dir / "page-cluster") == "0");
        assert(get(sda_q / "scheduler") == "mq-deadline");

        const auto leases = coordinator.active_leases();
        assert(leases.size() == 2U);

        // Both domains observe cleanly and transition to RETAINED together
        for (int i = 0; i < 3; ++i) {
            tuner.step({
                .cpu_psi = 2.0,
                .memory_psi = 2.2,
                .io_psi = 2.4,
                .thermal_pressure = 45.0,
            });
        }
        const auto retained = tuner.snapshot();
        assert(retained.stage == "RETAINED");
        assert(retained.verdict == TunerVerdict::Improved);
        assert(retained.active_interventions.size() == 2U);
        assert(retained.active_interventions[0].stage == "RETAINED");
        assert(retained.active_interventions[1].stage == "RETAINED");

        // BlockIo regresses (> before_io_psi + 4.0) while VmMemory remains healthy:
        // only BlockIo must roll back and release its lease, while VmMemory stays active.
        const auto partial_rb = tuner.step({
            .cpu_psi = 3.0,
            .memory_psi = 2.2,
            .io_psi = 14.5,
            .thermal_pressure = 45.0,
        });
        assert(partial_rb.stage == "RETAINED");
        assert(partial_rb.active_interventions.size() == 1U);
        assert(partial_rb.active_interventions.front().candidate_id == "vm.zram_latency_relief");
        assert(get(sda_q / "scheduler") == "none");
        assert(get(vm_dir / "page-cluster") == "0");
        const auto remaining_leases = coordinator.active_leases();
        assert(remaining_leases.size() == 1U);
        assert(remaining_leases.front().domain == MutationDomain::VmMemory);

        tuner.reset();
        assert(coordinator.active_leases().empty());
        assert(get(vm_dir / "page-cluster") == "3");
        assert(get(sda_q / "scheduler") == "none");
        put(sda_q / "scheduler", "[none] mq-deadline kyber bfq");
    }

    // =========================================================================
    // 4. Resource conflict exclusion & Thermal emergency veto in Planner
    // =========================================================================
    {
        CandidateProposal p_a{};
        p_a.id = "io.mq_deadline_low_latency";
        p_a.mutation_domain = MutationDomain::BlockIo;
        p_a.evaluation_domain = EvaluationDomain::Io;
        p_a.relevance = 0.80;
        p_a.resource_keys = {(sda_q / "read_ahead_kb").string()};
        p_a.steps.push_back({(sda_q / "read_ahead_kb").string(), "256", "Anchor", StepRelation::RequiredAnchor});

        CandidateProposal p_b{};
        p_b.id = "thermal.io_burst_pacing";
        p_b.mutation_domain = MutationDomain::CpuPolicy; // Different domain, SAME shared resource key!
        p_b.evaluation_domain = EvaluationDomain::Thermal;
        p_b.relevance = 0.70;
        p_b.stabilizing = true;
        p_b.resource_keys = {(sda_q / "read_ahead_kb").string()};
        p_b.steps.push_back({(sda_q / "read_ahead_kb").string(), "96", "Anchor", StepRelation::RequiredAnchor});

        CorrelatedEvidence ev{};
        ev.evidence_confidence = 0.9;
        ev.remaining_thermal_headroom = 0.6;
        ev.thermal_emergency = false;

        auto evaluated = GlobalCandidateEvaluator::evaluate_all(
            {p_a, p_b}, ev, {}, 0U, {});
        const InterventionPlan plan = InterventionPlanner::plan(
            evaluated, ev, {}, coordinator, "PlannerTest");
        // p_b shares resource_key with higher-utility p_a, so p_b must NOT co-execute
        assert(plan.selected.size() == 1U);
        assert(plan.selected.front().proposal.id == "io.mq_deadline_low_latency");

        // Thermal emergency vetoes ALL candidates unconditionally
        ev.thermal_emergency = true;
        const InterventionPlan vetoed = InterventionPlanner::plan(
            evaluated, ev, {}, coordinator, "PlannerTest");
        assert(vetoed.selected.empty());
    }

    // =========================================================================
    // 5. BoundedWorkerPool, TaskGroup, AsyncSnapshotWriter, and KernelIo
    // =========================================================================
    {
        BoundedWorkerPool pool(2U, 16U, "coreflow-test");
        assert(pool.active());
        assert(pool.worker_count() == 2U);

        std::atomic<uint64_t> counter{0U};
        {
            TaskGroup group;
            for (int i = 0; i < 64; ++i) {
                pool.submit(group, [&counter]() noexcept {
                    counter.fetch_add(1U, std::memory_order_relaxed);
                });
            }
            group.wait();
        }
        assert(counter.load(std::memory_order_relaxed) == 64U);

        pool.parallel_for(32U, [&counter](size_t) noexcept {
            counter.fetch_add(1U, std::memory_order_relaxed);
        });
        assert(counter.load(std::memory_order_relaxed) == 96U);
        pool.shutdown();
        assert(!pool.active());

        // AsyncSnapshotWriter coalescing & flush on stop()
        std::string last_written;
        std::atomic<uint32_t> write_calls{0U};
        {
            AsyncSnapshotWriter writer([&](const std::string& payload) noexcept {
                write_calls.fetch_add(1U, std::memory_order_relaxed);
                last_written = payload;
                return true;
            });
            for (int i = 0; i < 10; ++i) {
                writer.enqueue("snapshot-" + std::to_string(i));
            }
            writer.stop();
            assert(writer.completed_writes() >= 1U);
            assert(writer.failed_writes() == 0U);
        }
        assert(last_written == "snapshot-9");
        assert(write_calls.load(std::memory_order_relaxed) >= 1U);

        // KernelIo hardened sysfs/procfs reads & symlink rejection
        const fs::path missing_res_path = root / "does_not_exist";
        const auto missing_res = KernelIo::read_first_line(missing_res_path.string());
        assert(missing_res.status == KernelIoStatus::NotFound);
        assert(!missing_res.ok());

        const fs::path target_file = root / "real_node";
        put(target_file, "4096");
        const fs::path link_file = root / "link_node";
        std::error_code sym_ec;
        fs::create_symlink(target_file, link_file, sym_ec);
        if (!sym_ec) {
            const auto link_res = KernelIo::read_first_line(link_file.string());
            assert(link_res.status == KernelIoStatus::SymlinkRejected);
            assert(!link_res.ok());
        }

        const auto real_res = KernelIo::read_first_line(target_file.string());
        assert(real_res.ok());
        assert(real_res.value == "4096");

        uint64_t parsed = 0U;
        assert(KernelIo::parse_u64_strict(" 4096 \n", parsed) && parsed == 4096U);
        assert(!KernelIo::parse_u64_strict("4096kb", parsed));
        assert(!KernelIo::parse_u64_strict("-1", parsed));
    }

    // =========================================================================
    // 6. ThermalMonitor thread safety & single-pass sample_bundle() equivalence
    // =========================================================================
    {
        const fs::path therm_root = root / "sys_thermal/thermal";
        const auto create_tz = [&](int idx, const char* type, const char* temp) {
            const fs::path z = therm_root / ("thermal_zone" + std::to_string(idx));
            put(z / "type", type);
            put(z / "temp", temp);
            put(z / "trip_point_0_type", "passive");
            put(z / "trip_point_0_temp", "80000");
            put(z / "trip_point_1_type", "critical");
            put(z / "trip_point_1_temp", "105000");
        };
        create_tz(0, "cpu-0-0", "48000");
        create_tz(1, "gpu-0", "44000");
        create_tz(2, "battery", "34000");
        create_tz(3, "skin", "36000");

        ThermalMonitor mon(therm_root.string());
        const ThermalBundle b = mon.sample_bundle(false);
        const auto prof = mon.multi_sensor_profile();
        assert(std::fabs(b.profile.composite_soc - prof.composite_soc) < 0.01f);
        assert(std::fabs(b.battery_celsius - 34.0f) < 0.01f);
        assert(std::fabs(b.skin_celsius - 36.0f) < 0.01f);
        assert(b.assessment.readings.size() == 4U);

        std::vector<std::thread> readers;
        for (int t = 0; t < 4; ++t) {
            readers.emplace_back([&mon]() {
                for (int k = 0; k < 15; ++k) {
                    const ThermalBundle tb = mon.sample_bundle(false);
                    assert(tb.composite_celsius >= 0.0f);
                    (void)mon.hottest_celsius();
                    (void)mon.multi_sensor_profile();
                }
            });
        }
        for (auto& th : readers) th.join();
    }

    std::error_code rm_ec;
    fs::remove_all(root, rm_ec);
    return 0;
}
