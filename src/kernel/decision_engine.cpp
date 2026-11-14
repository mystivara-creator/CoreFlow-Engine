/**
 * @file decision_engine.cpp
 * @brief Global Evidence-Driven Autonomous Optimization Engine Implementation.
 * @license MIT
 */
#include "coreflow/decision_engine.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/config.hpp"
#include "coreflow/kernel_node_inventory.hpp"
#include "coreflow/worker_pool.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <unordered_set>

namespace coreflow {
namespace fs = std::filesystem;

namespace {

[[nodiscard]] bool contains_token(const std::vector<std::string>& list, std::string_view target) noexcept {
    return std::any_of(list.begin(), list.end(), [&](const std::string& s) noexcept {
        return s == target;
    });
}

[[nodiscard]] const CandidateStats* find_stats(
    const std::vector<CandidateStats>& stats,
    std::string_view id) noexcept {
    for (const auto& s : stats) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

[[nodiscard]] uint64_t compute_relative_u64_for_env(
    const EnvironmentCapabilities& env,
    const std::string& path,
    uint64_t fallback_baseline,
    double ratio,
    uint64_t min_bound,
    uint64_t max_bound,
    double adaptive_scale) noexcept {
    const uint64_t baseline = env.lookup_factory_u64(path, fallback_baseline);
    const double clamped_scale = std::clamp(adaptive_scale, 0.25, 1.0);
    const double effective_ratio = 1.0 + (ratio - 1.0) * clamped_scale;
    const double calc = static_cast<double>(baseline) * effective_ratio;
    const uint64_t rounded = std::max<uint64_t>(1U, static_cast<uint64_t>(std::round(calc)));
    return std::clamp(rounded, min_bound, max_bound);
}

[[nodiscard]] std::string read_trimmed_line(const std::string& path) noexcept {
    const auto res = KernelIo::read_first_line(path);
    if (!res.ok()) return {};
    return DaemonConfig::trim(res.value);
}

[[nodiscard]] std::string format_metric(double value) {
    std::string s = std::to_string(value);
    if (s.size() > 4U) s.resize(4U);
    return s;
}

[[nodiscard]] bool shares_resource_key(
    const std::vector<std::string>& lhs,
    const std::vector<std::string>& rhs) noexcept {
    for (const auto& a : lhs) {
        if (a.empty()) continue;
        for (const auto& b : rhs) {
            if (a == b) return true;
        }
    }
    return false;
}

void collect_step_resource_keys(CandidateProposal& proposal) {
    proposal.resource_keys.reserve(proposal.steps.size());
    for (const auto& s : proposal.steps) {
        if (s.path.empty()) continue;
        if (std::find(proposal.resource_keys.begin(), proposal.resource_keys.end(), s.path) ==
            proposal.resource_keys.end()) {
            proposal.resource_keys.push_back(s.path);
        }
    }
}

[[nodiscard]] double normalize_ucb_empirical(
    const CandidateStats* stats,
    DynamicWorkloadRegime regime,
    uint32_t total_reward_samples) noexcept {
    if (stats == nullptr) return 0.50;
    const auto idx = static_cast<std::size_t>(regime);
    const uint32_t regime_samples = (idx < stats->regime_reward_samples.size())
        ? stats->regime_reward_samples[idx]
        : 0U;
    const double context_mean = regime_samples > 0U
        ? stats->regime_mean_utility[idx]
        : stats->mean_utility;
    const double blended_prior = regime_samples > 0U
        ? (0.35 * stats->mean_utility + 0.65 * context_mean)
        : stats->mean_utility;

    // Normalize Q_k [0.05 .. 2.50] into [0.0 .. 1.0] so all domains share one scale.
    const double normalized_mean = std::clamp((blended_prior - 0.05) / 2.45, 0.0, 1.0);
    // Shrink unvisited/low-sample empirical prior smoothly toward 0.50 neutral baseline.
    const double maturity = std::min(1.0, static_cast<double>(regime_samples + stats->reward_samples) / 4.0);
    const double calibrated_mean = (0.45 * (1.0 - maturity) * 0.50) +
                                   (1.0 - 0.45 * (1.0 - maturity)) * normalized_mean;

    const double total_n = static_cast<double>(std::max<uint32_t>(1U, total_reward_samples));
    const double explore = std::min(
        0.22,
        0.16 * std::sqrt(std::log(total_n + 1.0) / static_cast<double>(regime_samples + 1U)));

    const uint32_t regime_regs = (idx < stats->regime_regressions.size())
        ? stats->regime_regressions[idx]
        : 0U;
    const double rollback_penalty = std::min(0.45, 0.12 * static_cast<double>(regime_regs));
    return std::clamp(calibrated_mean + explore - rollback_penalty, 0.0, 1.0);
}

} // namespace

bool EnvironmentCapabilities::has_runtime_node(std::string_view path) const noexcept {
    return CandidateSelector::has_runtime_path(runtime_tunable_nodes, path);
}

uint64_t EnvironmentCapabilities::lookup_factory_u64(
    std::string_view path,
    uint64_t fallback_baseline) const noexcept {
    uint64_t baseline = fallback_baseline;
    const auto audit = BaselineStore::instance().get_audit_trail();
    for (const auto& item : audit) {
        if (item.path == path) {
            uint64_t parsed = 0U;
            if (KernelIo::parse_u64_strict(item.factory_value, parsed) && parsed > 0U) {
                baseline = parsed;
                break;
            }
        }
    }
    if (baseline == 0U) {
        for (const auto& cap : runtime_tunable_nodes) {
            if (cap.path == path) {
                uint64_t parsed = 0U;
                if (KernelIo::parse_u64_strict(cap.current_value, parsed) && parsed > 0U) {
                    baseline = parsed;
                    break;
                }
            }
        }
    }
    return baseline == 0U ? fallback_baseline : baseline;
}

EnvironmentCapabilities EnvironmentDiscovery::discover(const EnvironmentDiscoveryInput& input) {
    EnvironmentCapabilities env{};
    env.zram_active = input.zram_active;
    env.vm_page_cluster_path = input.vm_page_cluster_path;
    env.vm_vfs_cache_pressure_path = input.vm_vfs_cache_pressure_path;
    if (input.block_queues != nullptr) {
        env.block_queues = *input.block_queues;
    }
    if (input.cpufreq_governor_dirs != nullptr) {
        env.cpufreq_governor_dirs = *input.cpufreq_governor_dirs;
    }
    if (input.kernel_node_capabilities != nullptr) {
        env.runtime_tunable_nodes.reserve(input.kernel_node_capabilities->size());
        for (const auto& cap : *input.kernel_node_capabilities) {
            if (CandidateSelector::is_mutation_candidate(cap)) {
                env.runtime_tunable_nodes.push_back(cap);
            }
        }
    }
    if (input.device_profile) {
        env.gpu_domain_count = input.device_profile->gpu_domains.size();
        env.thermal_sensor_count = input.device_profile->thermal_sensors.size();
    }
    env.frequency_actuator_ready = input.frequency_actuator_configured;
    env.frequency_policy_index = input.frequency_policy_index;
    env.tiered_policy_decision = input.tiered_policy_decision;
    env.tiered_policy_ready = input.tiered_policy_ready;
    env.tiered_cpu_policy_owned = input.tiered_cpu_policy_owned;
    env.uclamp_ready = input.uclamp_ready;
    return env;
}

CorrelatedEvidence EvidenceCorrelator::correlate(
    const TunerTelemetryInput& telemetry,
    const EnvironmentCapabilities& env,
    bool in_thermal_recovery) noexcept {
    CorrelatedEvidence ev{};
    ev.telemetry = telemetry;
    ev.workload = AutonomousTuner::classify_workload(telemetry);
    ev.regime = AutonomousTuner::classify_regime(telemetry, ev.workload);
    ev.thermal_bucket = AutonomousTuner::compute_thermal_bucket(telemetry);
    ev.in_thermal_recovery = in_thermal_recovery;

    ev.psi_complete = telemetry.cpu_psi_valid && telemetry.memory_psi_valid && telemetry.io_psi_valid;
    ev.thermal_emergency = telemetry.thermal_emergency ||
        (telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure) &&
         telemetry.thermal_pressure >= 88.0);
    ev.thermal_stabilization_required =
        !ev.thermal_emergency &&
        (ev.workload == WorkloadClass::ThermalConstrained ||
         telemetry.thermal_guard_active ||
         (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 65.0) ||
         (telemetry.thermal_headroom >= 0.0 && telemetry.thermal_headroom <= 0.20));

    if (telemetry.thermal_headroom >= 0.0 && std::isfinite(telemetry.thermal_headroom)) {
        ev.remaining_thermal_headroom = std::clamp(telemetry.thermal_headroom, 0.0, 1.0);
    } else if (telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure)) {
        ev.remaining_thermal_headroom = std::clamp((88.0 - telemetry.thermal_pressure) / 46.0, 0.0, 1.0);
    } else {
        ev.remaining_thermal_headroom = 0.5;
    }

    if (telemetry.cpu_psi_valid && std::isfinite(telemetry.cpu_psi) && telemetry.cpu_psi >= 0.0) {
        ev.cpu_severity = std::clamp((telemetry.cpu_psi - 2.0) / 26.0, 0.0, 1.0);
    }
    if (telemetry.frequency_policy_evidence_valid &&
        telemetry.frequency_policy_evidence.classification.workload == DetectedWorkloadClass::CpuBound) {
        const double cls_conf = std::clamp(
            static_cast<double>(telemetry.frequency_policy_evidence.classification.confidence), 0.0, 1.0);
        ev.cpu_severity = std::max(ev.cpu_severity, cls_conf * 0.85);
    }
    if (telemetry.memory_psi_valid && std::isfinite(telemetry.memory_psi) && telemetry.memory_psi >= 0.0) {
        ev.memory_severity = std::clamp((telemetry.memory_psi - 0.8) / 14.0, 0.0, 1.0);
    }
    if (telemetry.io_psi_valid && std::isfinite(telemetry.io_psi) && telemetry.io_psi >= 0.0) {
        ev.io_severity = std::clamp((telemetry.io_psi - 0.8) / 14.0, 0.0, 1.0);
    }
    if (telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure) && telemetry.thermal_pressure >= 0.0) {
        ev.thermal_severity = std::clamp((telemetry.thermal_pressure - 42.0) / 46.0, 0.0, 1.0);
    }

    if (telemetry.cpu_psi_valid && telemetry.io_psi_valid &&
        telemetry.cpu_psi >= 8.0 && telemetry.io_psi >= 3.5) {
        ev.concurrent_cpu_io = std::clamp(0.45 * ev.cpu_severity + 0.45 * ev.io_severity + 0.15, 0.0, 1.0);
    } else {
        ev.concurrent_cpu_io = std::clamp(std::min(ev.cpu_severity, ev.io_severity), 0.0, 1.0);
    }

    if (env.zram_active && telemetry.memory_psi_valid && telemetry.io_psi_valid &&
        telemetry.memory_psi >= 2.0 && telemetry.io_psi >= 1.5) {
        ev.swap_io_thrash = std::clamp(0.55 * ev.memory_severity + 0.45 * ev.io_severity, 0.0, 1.0);
    }

    const bool interactive_workload =
        ev.workload == WorkloadClass::GeneralInteraction ||
        ev.workload == WorkloadClass::ScrollingAndAnimation ||
        ev.workload == WorkloadClass::Gaming ||
        ev.workload == WorkloadClass::CpuBound;
    if (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 8.0 &&
        (!telemetry.thermal_pressure_valid || telemetry.thermal_pressure < 72.0)) {
        ev.interactive_cpu_stall = std::clamp(
            ev.cpu_severity * (interactive_workload ? 1.15 : 0.90), 0.0, 1.0);
    }

    double vel_contrib = 0.0;
    if (telemetry.temperature_velocity_valid && std::isfinite(telemetry.temperature_velocity_celsius_per_sec) &&
        telemetry.temperature_velocity_celsius_per_sec > 0.05) {
        vel_contrib = std::clamp(telemetry.temperature_velocity_celsius_per_sec / 1.5, 0.0, 1.0);
    }
    ev.thermal_ramp_pressure = std::clamp(
        0.65 * ev.thermal_severity + 0.35 * vel_contrib + (telemetry.thermal_guard_active ? 0.25 : 0.0),
        0.0, 1.0);

    uint32_t valid_signals = 0U;
    if (telemetry.cpu_psi_valid) ++valid_signals;
    if (telemetry.memory_psi_valid) ++valid_signals;
    if (telemetry.io_psi_valid) ++valid_signals;
    if (telemetry.thermal_pressure_valid) ++valid_signals;
    if (telemetry.temperature_velocity_valid && telemetry.temperature_samples_count >= 2U) ++valid_signals;
    ev.evidence_confidence = std::clamp(static_cast<double>(valid_signals) / 5.0, 0.10, 1.0);

    // Global, non-prioritized bottleneck diagnosis via normalized severity comparison
    if (ev.thermal_emergency) {
        ev.diagnosed_bottleneck = BottleneckKind::ThermalEmergency;
    } else if (ev.thermal_stabilization_required) {
        ev.diagnosed_bottleneck = BottleneckKind::ThermalHeadroomDepletion;
    } else if (telemetry.io_psi_valid && telemetry.cpu_psi_valid &&
               telemetry.io_psi >= 4.5 && telemetry.cpu_psi >= 9.0) {
        ev.diagnosed_bottleneck = BottleneckKind::ConcurrentCpuIoContention;
    } else {
        const double io_active = (telemetry.io_psi_valid && telemetry.io_psi >= 2.2)
            ? std::max(0.05, ev.io_severity) : -1.0;
        const double mem_active = (telemetry.memory_psi_valid && telemetry.memory_psi >= 2.0)
            ? std::max(0.05, ev.memory_severity) : -1.0;
        const double cpu_active = (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 8.0)
            ? std::max(0.05, ev.cpu_severity) : -1.0;
        if (mem_active >= 0.0 && mem_active >= io_active && mem_active >= cpu_active) {
            ev.diagnosed_bottleneck = BottleneckKind::MemorySwapThrashing;
        } else if (cpu_active >= 0.0 && cpu_active >= io_active) {
            ev.diagnosed_bottleneck = BottleneckKind::CpuSchedulingLatency;
        } else if (io_active >= 0.0) {
            ev.diagnosed_bottleneck = BottleneckKind::IoQueueContention;
        } else {
            ev.diagnosed_bottleneck = BottleneckKind::None;
        }
    }

    return ev;
}

void BlockIoCandidateProvider::generate(
    const EnvironmentCapabilities& env,
    const CorrelatedEvidence& evidence,
    const std::vector<CandidateStats>& stats,
    std::vector<CandidateProposal>& out) const {
    if (evidence.thermal_emergency || evidence.thermal_stabilization_required) return;
    if (!evidence.telemetry.io_psi_valid || evidence.telemetry.io_psi < 2.2 ||
        env.block_queues.empty()) {
        return;
    }

    const auto& primary_q = env.block_queues.front();
    const double base_rel = std::clamp(0.42 + 0.50 * evidence.io_severity, 0.0, 0.94);

    const bool any_mq = std::any_of(env.block_queues.begin(), env.block_queues.end(),
        [](const auto& q) noexcept { return contains_token(q.available_schedulers, "mq-deadline"); });
    const bool any_kyber = std::any_of(env.block_queues.begin(), env.block_queues.end(),
        [](const auto& q) noexcept { return contains_token(q.available_schedulers, "kyber"); });
    const bool any_bfq = std::any_of(env.block_queues.begin(), env.block_queues.end(),
        [](const auto& q) noexcept { return contains_token(q.available_schedulers, "bfq"); });

    auto build_scheduler_steps = [&](std::string_view sched, double ra_ratio, double nr_ratio, bool include_nr) {
        std::vector<PlannedMutationStep> steps;
        const auto* st = find_stats(stats, sched == "mq-deadline" ? "io.mq_deadline_low_latency"
            : sched == "kyber" ? "io.kyber_token_bucket" : "io.bfq_interactive_fairness");
        const double scale = st ? st->adaptive_scale : 1.0;
        for (const auto& q : env.block_queues) {
            if (!contains_token(q.available_schedulers, sched)) continue;
            const std::string sched_path = q.queue_dir + "/scheduler";
            const std::string ra_path = q.queue_dir + "/read_ahead_kb";
            if (!env.has_runtime_node(sched_path) || !env.has_runtime_node(ra_path)) continue;
            steps.push_back({sched_path, std::string(sched), "Anchor (Required-before)", StepRelation::RequiredAnchor});
            if (include_nr) {
                const std::string nr_path = q.queue_dir + "/nr_requests";
                if (env.has_runtime_node(nr_path)) {
                    const uint64_t rel_nr = compute_relative_u64_for_env(
                        env, nr_path, q.baseline_nr_requests, nr_ratio, 32U, 512U, scale);
                    steps.push_back({nr_path, std::to_string(rel_nr), "Coupled-with", StepRelation::RequiredDependent});
                }
            }
            const uint64_t rel_ra = compute_relative_u64_for_env(
                env, ra_path, q.baseline_read_ahead_kb, ra_ratio, 32U, 512U, scale);
            steps.push_back({ra_path, std::to_string(rel_ra), "Coupled-with", StepRelation::RequiredDependent});
            if (sched != "bfq") {
                const std::string aff_path = q.queue_dir + "/rq_affinity";
                if (env.has_runtime_node(aff_path)) {
                    steps.push_back({aff_path, "2", "Coupled-with", StepRelation::RequiredDependent});
                }
            }
        }
        return steps;
    };

    const bool concurrent_cpu_io =
        evidence.telemetry.io_psi >= 5.0 && evidence.telemetry.cpu_psi_valid &&
        evidence.telemetry.cpu_psi >= 10.0;

    if (any_mq) {
        CandidateProposal p{};
        p.id = "io.mq_deadline_low_latency";
        p.label = "mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)";
        p.domain_label = "io";
        p.mutation_domain = MutationDomain::BlockIo;
        p.evaluation_domain = EvaluationDomain::Io;
        p.priority = ControllerPriority::PressureGuard;
        p.action_kind = CandidateActionKind::SysfsTransaction;
        p.intent = "IoLatencyRelief";
        p.rationale = "Measured I/O PSI stall (" + format_metric(evidence.telemetry.io_psi) +
            "%); switching scheduler from " + primary_q.active_scheduler +
            " to mq-deadline with relative read_ahead scaling from OEM baseline.";
        p.steps = build_scheduler_steps("mq-deadline", 2.00, 1.00, false);
        collect_step_resource_keys(p);
        p.relevance = std::clamp(base_rel + (concurrent_cpu_io ? 0.0 : 0.05), 0.0, 1.0);
        p.inherent_risk = 0.22;
        p.thermal_cost = 0.08;
        p.stabilizing = false;
        out.push_back(std::move(p));
    }

    if (any_kyber) {
        CandidateProposal p{};
        p.id = "io.kyber_token_bucket";
        p.label = "kyber Token-Bucket Low-Latency I/O Scheduler";
        p.domain_label = "io";
        p.mutation_domain = MutationDomain::BlockIo;
        p.evaluation_domain = EvaluationDomain::Io;
        p.priority = ControllerPriority::PressureGuard;
        p.action_kind = CandidateActionKind::SysfsTransaction;
        p.intent = "IoLatencyRelief";
        p.rationale = "Concurrent I/O (" + format_metric(evidence.telemetry.io_psi) +
            "%) & CPU load correlated globally; switching scheduler from " +
            primary_q.active_scheduler + " to kyber token-bucket arbitration.";
        p.steps = build_scheduler_steps("kyber", 1.00, 2.00, true);
        collect_step_resource_keys(p);
        p.relevance = std::clamp(base_rel + (concurrent_cpu_io ? 0.14 : -0.02), 0.0, 1.0);
        p.inherent_risk = 0.22;
        p.thermal_cost = 0.08;
        p.stabilizing = false;
        out.push_back(std::move(p));
    }

    if (any_bfq) {
        CandidateProposal p{};
        p.id = "io.bfq_interactive_fairness";
        p.label = "bfq Proportional-Share Interactive I/O Scheduler";
        p.domain_label = "io";
        p.mutation_domain = MutationDomain::BlockIo;
        p.evaluation_domain = EvaluationDomain::Io;
        p.priority = ControllerPriority::PressureGuard;
        p.action_kind = CandidateActionKind::SysfsTransaction;
        p.intent = "IoLatencyRelief";
        p.rationale = "Sustained I/O contention (" + format_metric(evidence.telemetry.io_psi) +
            "%); evaluating bfq proportional-share scheduler against OEM baseline.";
        p.steps = build_scheduler_steps("bfq", 2.00, 1.00, false);
        collect_step_resource_keys(p);
        const bool interactive =
            evidence.workload == WorkloadClass::ScrollingAndAnimation ||
            evidence.workload == WorkloadClass::GeneralInteraction;
        p.relevance = std::clamp(base_rel + (interactive ? 0.06 : -0.03), 0.0, 1.0);
        p.inherent_risk = 0.22;
        p.thermal_cost = 0.06;
        p.stabilizing = false;
        out.push_back(std::move(p));
    }

    if (!any_mq && !any_kyber && !any_bfq) {
        const auto* st = find_stats(stats, "io.queue_readahead_pacing");
        const double scale = st ? st->adaptive_scale : 1.0;
        CandidateProposal p{};
        p.id = "io.queue_readahead_pacing";
        p.label = "Adaptive Block Queue Read-Ahead & Request Depth (2.0x OEM baseline)";
        p.domain_label = "io";
        p.mutation_domain = MutationDomain::BlockIo;
        p.evaluation_domain = EvaluationDomain::Io;
        p.priority = ControllerPriority::PressureGuard;
        p.action_kind = CandidateActionKind::SysfsTransaction;
        p.intent = "IoLatencyRelief";
        p.rationale = "I/O stall pressure elevated; relative scaling of verified block queue read_ahead_kb and nr_requests from OEM baseline.";
        for (const auto& q : env.block_queues) {
            const std::string ra_path = q.queue_dir + "/read_ahead_kb";
            if (!env.has_runtime_node(ra_path)) continue;
            const uint64_t rel_ra = compute_relative_u64_for_env(
                env, ra_path, q.baseline_read_ahead_kb, 2.00, 32U, 512U, scale);
            p.steps.push_back({ra_path, std::to_string(rel_ra), "Anchor (Required-before)", StepRelation::RequiredAnchor});
            const std::string nr_path = q.queue_dir + "/nr_requests";
            if (env.has_runtime_node(nr_path)) {
                const uint64_t rel_nr = compute_relative_u64_for_env(
                    env, nr_path, q.baseline_nr_requests, 2.00, 32U, 512U, scale);
                p.steps.push_back({nr_path, std::to_string(rel_nr), "Coupled-with", StepRelation::RequiredDependent});
            }
        }
        collect_step_resource_keys(p);
        p.relevance = std::clamp(base_rel - 0.06, 0.0, 1.0);
        p.inherent_risk = 0.14;
        p.thermal_cost = 0.04;
        p.stabilizing = false;
        out.push_back(std::move(p));
    }
}

void MemoryVmCandidateProvider::generate(
    const EnvironmentCapabilities& env,
    const CorrelatedEvidence& evidence,
    const std::vector<CandidateStats>&,
    std::vector<CandidateProposal>& out) const {
    if (evidence.thermal_emergency || evidence.thermal_stabilization_required) return;
    if (!env.zram_active || !evidence.telemetry.memory_psi_valid ||
        evidence.telemetry.memory_psi < 2.0) {
        return;
    }
    if (env.vm_page_cluster_path.empty() || env.vm_vfs_cache_pressure_path.empty()) return;

    CandidateProposal p{};
    p.id = "vm.zram_latency_relief";
    p.label = "VM Page-Cluster Zero-Latency & VFS Dentry Preservation";
    p.domain_label = "vm";
    p.mutation_domain = MutationDomain::VmMemory;
    p.evaluation_domain = EvaluationDomain::Memory;
    p.priority = ControllerPriority::PressureGuard;
    p.action_kind = CandidateActionKind::SysfsTransaction;
    p.intent = "MemoryStallRelief";
    p.rationale = "Memory PSI stall (" + format_metric(evidence.telemetry.memory_psi) +
        "%) detected with active zram swap; setting vm.page-cluster=0 and vfs_cache_pressure=80.";
    p.steps = {
        {env.vm_page_cluster_path, "0", "Anchor (Required-before)", StepRelation::RequiredAnchor},
        {env.vm_vfs_cache_pressure_path, "80", "Coupled-with", StepRelation::RequiredDependent},
    };
    collect_step_resource_keys(p);
    p.relevance = std::clamp(
        0.44 + 0.54 * evidence.memory_severity + 0.12 * evidence.swap_io_thrash,
        0.0, 1.0);
    p.inherent_risk = 0.16;
    p.thermal_cost = 0.05;
    p.stabilizing = false;
    out.push_back(std::move(p));
}

void CpuGovernorCandidateProvider::generate(
    const EnvironmentCapabilities& env,
    const CorrelatedEvidence& evidence,
    const std::vector<CandidateStats>&,
    std::vector<CandidateProposal>& out) const {
    if (evidence.thermal_emergency || evidence.thermal_stabilization_required ||
        evidence.in_thermal_recovery || env.tiered_cpu_policy_owned) {
        return;
    }
    if (!evidence.telemetry.cpu_psi_valid || evidence.telemetry.cpu_psi < 8.0) return;
    if (!evidence.telemetry.thermal_pressure_valid || evidence.telemetry.thermal_pressure >= 72.0) return;
    if (env.cpufreq_governor_dirs.empty()) return;

    CandidateProposal p{};
    p.id = "cpufreq.schedutil_interactive_response";
    p.label = "EAS Governor Rate-Limit Pacing (500us up / 10000us down)";
    p.domain_label = "cpufreq";
    p.mutation_domain = MutationDomain::CpuPolicy;
    p.evaluation_domain = EvaluationDomain::Cpu;
    p.priority = ControllerPriority::WorkloadOptimization;
    p.action_kind = CandidateActionKind::SysfsTransaction;
    p.intent = "InteractiveCpuPacing";
    p.rationale = "CPU PSI stall (" + format_metric(evidence.telemetry.cpu_psi) +
        "%) with thermal headroom; pacing EAS governor rate limits (up=500us, down=10000us).";

    for (const auto& gdir : env.cpufreq_governor_dirs) {
        const fs::path gpath(gdir);
        const std::string gov_name = gpath.filename().string();
        const fs::path scaling_gov_path = gpath.parent_path() / "scaling_governor";
        const std::string active_gov = read_trimmed_line(scaling_gov_path.string());
        if (!active_gov.empty() && active_gov != gov_name) {
            continue;
        }
        p.steps.push_back({gdir + "/up_rate_limit_us", "500", "Anchor (Required-before)", StepRelation::RequiredAnchor});
        p.steps.push_back({gdir + "/down_rate_limit_us", "10000", "Coupled-with", StepRelation::RequiredDependent});
    }
    collect_step_resource_keys(p);
    p.relevance = std::clamp(
        0.45 + 0.50 * evidence.cpu_severity + 0.14 * evidence.interactive_cpu_stall,
        0.0, 1.0);
    p.inherent_risk = 0.24;
    p.thermal_cost = 0.26;
    p.stabilizing = false;
    out.push_back(std::move(p));
}

void TieredFrequencyCandidateProvider::generate(
    const EnvironmentCapabilities& env,
    const CorrelatedEvidence& evidence,
    const std::vector<CandidateStats>&,
    std::vector<CandidateProposal>& out) const {
    if (evidence.thermal_emergency) return;
    if (!env.frequency_actuator_ready || !env.frequency_policy_index.has_value() ||
        !env.tiered_policy_ready || !env.tiered_policy_decision.requires_mutation_admission) {
        return;
    }

    if (env.tiered_policy_decision.action != PolicyAction::CpuFrequencyStep ||
        (env.tiered_policy_decision.tier != OptimizationTier::Tier1InteractiveResponse &&
         env.tiered_policy_decision.tier != OptimizationTier::Tier2SustainedOptimization)) {
        return;
    }
    if (evidence.thermal_stabilization_required) {
        return;
    }
    const bool is_thermal_tier = false;

    CandidateProposal p{};
    p.id = "cpu.frequency.tiered";
    p.label = "Tiered CPU Frequency OPP";
    p.domain_label = "cpufreq";
    p.mutation_domain = MutationDomain::CpuPolicy;
    p.evaluation_domain = EvaluationDomain::Cpu;
    p.priority = is_thermal_tier
        ? ControllerPriority::ThermalStabilization
        : ControllerPriority::WorkloadOptimization;
    p.action_kind = CandidateActionKind::TieredFrequencyPolicy;
    p.intent = "TieredCpuFrequency";
    p.rationale = std::string(env.tiered_policy_decision.rationale);
    p.requested_capacity_scale = env.tiered_policy_decision.requested_capacity_scale;
    p.frequency_policy_index = *env.frequency_policy_index;
    p.resource_keys.push_back("cpufreq:policy" + std::to_string(*env.frequency_policy_index));

    const double policy_conf = std::clamp(
        static_cast<double>(env.tiered_policy_decision.confidence), 0.25, 1.0);
    p.relevance = is_thermal_tier
        ? std::clamp(0.55 + 0.40 * evidence.thermal_severity, 0.0, 1.0)
        : std::clamp(0.45 + 0.35 * evidence.cpu_severity + 0.20 * policy_conf, 0.0, 1.0);
    p.inherent_risk = 0.20;
    p.thermal_cost = is_thermal_tier ? 0.0 : 0.22;
    p.stabilizing = is_thermal_tier;
    out.push_back(std::move(p));
}

void ThermalStabilizationCandidateProvider::generate(
    const EnvironmentCapabilities& env,
    const CorrelatedEvidence& evidence,
    const std::vector<CandidateStats>& stats,
    std::vector<CandidateProposal>& out) const {
    if (evidence.thermal_emergency || !evidence.thermal_stabilization_required) return;

    if (!env.cpufreq_governor_dirs.empty()) {
        const auto* st = find_stats(stats, "thermal.schedutil_damped_pacing");
        const double scale = st ? st->adaptive_scale : 1.0;
        CandidateProposal p{};
        p.id = "thermal.schedutil_damped_pacing";
        p.label = "Thermal Stabilization Governor Damping (2.0x up / 0.5x down rate-limit)";
        p.domain_label = "thermal";
        p.mutation_domain = MutationDomain::CpuPolicy;
        p.evaluation_domain = EvaluationDomain::Thermal;
        p.priority = ControllerPriority::ThermalStabilization;
        p.action_kind = CandidateActionKind::SysfsTransaction;
        p.intent = "ThermalStabilization";
        p.rationale = "Active thermal stabilization: damping EAS governor frequency ramp rate to reduce transient thermal spikes.";

        for (const auto& gdir : env.cpufreq_governor_dirs) {
            const fs::path gpath(gdir);
            const std::string gov_name = gpath.filename().string();
            const fs::path scaling_gov_path = gpath.parent_path() / "scaling_governor";
            const std::string active_gov = read_trimmed_line(scaling_gov_path.string());
            if (!active_gov.empty() && active_gov != gov_name) continue;

            const std::string up_path = gdir + "/up_rate_limit_us";
            const std::string down_path = gdir + "/down_rate_limit_us";
            const uint64_t damped_up = compute_relative_u64_for_env(env, up_path, 2000U, 2.00, 2000U, 10000U, scale);
            const uint64_t damped_down = compute_relative_u64_for_env(env, down_path, 1000U, 0.50, 500U, 4000U, scale);
            p.steps.push_back({up_path, std::to_string(damped_up), "Anchor (Required-before)", StepRelation::RequiredAnchor});
            p.steps.push_back({down_path, std::to_string(damped_down), "Coupled-with", StepRelation::RequiredDependent});
        }
        collect_step_resource_keys(p);
        p.relevance = std::clamp(
            0.48 + 0.34 * evidence.thermal_ramp_pressure + 0.18 * evidence.cpu_severity,
            0.0, 1.0);
        p.inherent_risk = 0.18;
        p.thermal_cost = 0.0;
        p.stabilizing = true;
        out.push_back(std::move(p));
    }

    if (evidence.telemetry.io_psi_valid && evidence.telemetry.io_psi >= 1.5 &&
        !env.block_queues.empty()) {
        const auto* st = find_stats(stats, "thermal.io_burst_pacing");
        const double scale = st ? st->adaptive_scale : 1.0;
        CandidateProposal p{};
        p.id = "thermal.io_burst_pacing";
        p.label = "Thermal Stabilization Block Queue Burst Pacing (0.5x read-ahead / 0.75x depth)";
        p.domain_label = "thermal";
        p.mutation_domain = MutationDomain::BlockIo;
        p.evaluation_domain = EvaluationDomain::Thermal;
        p.priority = ControllerPriority::ThermalStabilization;
        p.action_kind = CandidateActionKind::SysfsTransaction;
        p.intent = "ThermalStabilization";
        p.rationale = "Active thermal stabilization: moderating block queue burst depth under concurrent thermal and I/O load.";

        for (const auto& q : env.block_queues) {
            const std::string ra_path = q.queue_dir + "/read_ahead_kb";
            const std::string nr_path = q.queue_dir + "/nr_requests";
            if (!env.has_runtime_node(ra_path) || !env.has_runtime_node(nr_path)) continue;
            const uint64_t damped_ra = compute_relative_u64_for_env(
                env, ra_path, q.baseline_read_ahead_kb, 0.50, 32U, 512U, scale);
            const uint64_t damped_nr = compute_relative_u64_for_env(
                env, nr_path, q.baseline_nr_requests, 0.75, 32U, 256U, scale);
            p.steps.push_back({ra_path, std::to_string(damped_ra), "Anchor (Required-before)", StepRelation::RequiredAnchor});
            p.steps.push_back({nr_path, std::to_string(damped_nr), "Coupled-with", StepRelation::RequiredDependent});
        }
        collect_step_resource_keys(p);
        p.relevance = std::clamp(
            0.48 + 0.30 * evidence.thermal_ramp_pressure + 0.22 * evidence.io_severity,
            0.0, 1.0);
        p.inherent_risk = 0.15;
        p.thermal_cost = 0.0;
        p.stabilizing = true;
        out.push_back(std::move(p));
    }
}

std::vector<EvaluatedCandidate> GlobalCandidateEvaluator::evaluate_all(
    std::vector<CandidateProposal> proposals,
    const CorrelatedEvidence& evidence,
    const std::vector<CandidateStats>& stats,
    uint32_t total_reward_samples,
    const CandidateEligibilityFn& eligibility_check) {
    std::vector<EvaluatedCandidate> evaluated;
    evaluated.reserve(proposals.size());
    (void)total_reward_samples; // Global count is intentionally not used for domain UCB.

    for (auto& proposal : proposals) {
        EvaluatedCandidate item{};
        std::string reason;
        bool is_eligible = true;
        if (eligibility_check) {
            is_eligible = eligibility_check(proposal, reason);
        }
        if (proposal.action_kind == CandidateActionKind::SysfsTransaction && proposal.steps.empty()) {
            is_eligible = false;
            if (reason.empty()) reason = "No verified mutation steps available";
        }
        if (proposal.relevance <= 0.0) {
            is_eligible = false;
            if (reason.empty()) reason = "Zero relevance under current workload evidence";
        }

        const CandidateStats* st = find_stats(stats, proposal.id);
        // UCB exploration must be causal within the candidate's own subsystem.
        // CPU/IO samples must not suppress exploration of another domain.
        uint32_t domain_reward_samples = 0U;
        for (const auto& domain_stats : stats) {
            if (domain_stats.domain == proposal.domain_label) {
                const uint32_t remaining =
                    std::numeric_limits<uint32_t>::max() - domain_reward_samples;
                domain_reward_samples += std::min(remaining, domain_stats.reward_samples);
            }
        }
        const double empirical = normalize_ucb_empirical(
            st, evidence.regime, domain_reward_samples);
        const double sample_maturity = (st == nullptr)
            ? 0.20
            : std::min(1.0, static_cast<double>(st->reward_samples + 1U) / 6.0);
        const double confidence = std::clamp(
            0.65 * evidence.evidence_confidence + 0.35 * sample_maturity,
            0.10, 1.0);

        const double rollback_hist = (st == nullptr)
            ? 0.0
            : std::min(0.35, 0.08 * static_cast<double>(st->regressed_count) +
                             0.10 * static_cast<double>(st->ownership_loss_count));
        const double risk_penalty = std::clamp(0.50 * proposal.inherent_risk + rollback_hist, 0.0, 0.60);
        const double thermal_penalty = proposal.stabilizing
            ? 0.0
            : std::clamp(proposal.thermal_cost * (0.25 + 0.75 * evidence.thermal_severity), 0.0, 0.40);

        const double raw_utility =
            0.48 * proposal.relevance +
            0.32 * empirical +
            0.20 * confidence -
            0.35 * risk_penalty -
            0.30 * thermal_penalty;
        const double norm_utility = is_eligible ? std::clamp(raw_utility, 0.0, 1.0) : 0.0;

        item.proposal = std::move(proposal);
        item.empirical_score = empirical;
        item.confidence = confidence;
        item.risk_penalty = risk_penalty;
        item.thermal_penalty = thermal_penalty;
        item.normalized_utility = norm_utility;
        item.utility_score = norm_utility * 100.0;
        item.eligible = is_eligible;
        item.rejection_reason = std::move(reason);
        evaluated.push_back(std::move(item));
    }

    // Strict global ranking: utility -> confidence -> lowest risk -> deterministic ID.
    // Never rank by domain identity!
    std::sort(evaluated.begin(), evaluated.end(),
        [](const EvaluatedCandidate& a, const EvaluatedCandidate& b) noexcept {
            if (a.eligible != b.eligible) return a.eligible > b.eligible;
            if (std::abs(a.utility_score - b.utility_score) > 1e-6) {
                return a.utility_score > b.utility_score;
            }
            if (std::abs(a.confidence - b.confidence) > 1e-6) {
                return a.confidence > b.confidence;
            }
            if (std::abs(a.risk_penalty - b.risk_penalty) > 1e-6) {
                return a.risk_penalty < b.risk_penalty;
            }
            return a.proposal.id < b.proposal.id;
        });

    return evaluated;
}

InterventionPlan InterventionPlanner::plan(
    std::vector<EvaluatedCandidate> evaluated,
    const CorrelatedEvidence& evidence,
    const std::vector<ActiveDomainReservation>& active_reservations,
    const CentralMutationCoordinator& coordinator,
    std::string_view controller_name) {
    InterventionPlan plan{};
    plan.ranked_all = std::move(evaluated);

    if (evidence.thermal_emergency) {
        plan.planning_rationale = "Thermal emergency veto active; all performance and tuning mutations blocked.";
        return plan;
    }

    const size_t max_total_active = evidence.thermal_stabilization_required
        ? kMaxConcurrentActiveStabilizing
        : (evidence.remaining_thermal_headroom >= 0.35
               ? kMaxConcurrentActiveNormal
               : kMaxConcurrentActiveTightThermal);

    if (active_reservations.size() >= max_total_active) {
        plan.planning_rationale = "Active intervention budget saturated for current thermal headroom.";
        return plan;
    }

    std::unordered_set<uint8_t> occupied_domains;
    std::vector<std::string> occupied_resources;
    for (const auto& res : active_reservations) {
        occupied_domains.insert(static_cast<uint8_t>(res.domain));
        for (const auto& key : res.resource_keys) {
            occupied_resources.push_back(key);
        }
    }

    for (const auto& cand : plan.ranked_all) {
        if (active_reservations.size() + plan.selected.size() >= max_total_active) break;
        if (!cand.eligible || cand.normalized_utility < kMinimumUtilityThreshold) continue;
        if (evidence.thermal_stabilization_required && !cand.proposal.stabilizing) continue;

        const uint8_t dom_key = static_cast<uint8_t>(cand.proposal.mutation_domain);
        if (occupied_domains.count(dom_key) > 0U) continue;
        if (shares_resource_key(cand.proposal.resource_keys, occupied_resources)) continue;

        if (coordinator.is_domain_locked_by_other(
                cand.proposal.mutation_domain, controller_name, cand.proposal.priority)) {
            continue;
        }

        occupied_domains.insert(dom_key);
        for (const auto& key : cand.proposal.resource_keys) {
            occupied_resources.push_back(key);
        }
        plan.selected.push_back(cand);
    }

    if (plan.selected.empty()) {
        plan.planning_rationale = "No eligible candidate exceeded utility & safety admission thresholds.";
    } else if (plan.selected.size() == 1U) {
        plan.planning_rationale = "Global evaluation selected " + plan.selected.front().proposal.id +
            " (utility=" + format_metric(plan.selected.front().utility_score) + "/100).";
    } else {
        plan.planning_rationale = "Coordinated multi-domain plan selected " +
            std::to_string(plan.selected.size()) + " compatible interventions led by " +
            plan.selected.front().proposal.id + ".";
    }

    return plan;
}

GlobalDecisionEngine::GlobalDecisionEngine() {
    providers_.push_back(std::make_unique<BlockIoCandidateProvider>());
    providers_.push_back(std::make_unique<MemoryVmCandidateProvider>());
    providers_.push_back(std::make_unique<CpuGovernorCandidateProvider>());
    providers_.push_back(std::make_unique<TieredFrequencyCandidateProvider>());
    providers_.push_back(std::make_unique<ThermalStabilizationCandidateProvider>());
}

InterventionPlan GlobalDecisionEngine::run_cycle(
    const EnvironmentCapabilities& env,
    const CorrelatedEvidence& evidence,
    const std::vector<CandidateStats>& stats,
    uint32_t total_reward_samples,
    const std::vector<ActiveDomainReservation>& active_reservations,
    const CentralMutationCoordinator& coordinator,
    std::string_view controller_name,
    const CandidateEligibilityFn& eligibility_check,
    BoundedWorkerPool* pool) const {
    const size_t provider_count = providers_.size();
    std::vector<std::vector<CandidateProposal>> per_provider(provider_count);

    if (pool != nullptr && pool->active() && provider_count > 1U) {
        pool->parallel_for(provider_count, [&](size_t idx) noexcept {
            providers_[idx]->generate(env, evidence, stats, per_provider[idx]);
        });
    } else {
        for (size_t idx = 0U; idx < provider_count; ++idx) {
            providers_[idx]->generate(env, evidence, stats, per_provider[idx]);
        }
    }

    std::vector<CandidateProposal> all_proposals;
    size_t total = 0U;
    for (const auto& vec : per_provider) total += vec.size();
    all_proposals.reserve(total);
    for (auto& vec : per_provider) {
        for (auto& p : vec) {
            all_proposals.push_back(std::move(p));
        }
    }

    auto evaluated = GlobalCandidateEvaluator::evaluate_all(
        std::move(all_proposals),
        evidence,
        stats,
        total_reward_samples,
        eligibility_check);

    return InterventionPlanner::plan(
        std::move(evaluated),
        evidence,
        active_reservations,
        coordinator,
        controller_name);
}

} // namespace coreflow
