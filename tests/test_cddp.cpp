#include "coreflow/device_profiler.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/frequency_controller.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include "cddp_fixture_factory.hpp"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
namespace {
std::string read_value(const std::filesystem::path& path) {
    std::ifstream in(path); std::string value; in >> value; return value;
}
const coreflow::NodeCapability* find_cap(
    const coreflow::DeviceProfileSnapshot& profile, const std::string& semantic) {
    const auto it = std::find_if(profile.node_capabilities.begin(), profile.node_capabilities.end(),
        [&](const auto& cap) { return cap.semantic_id == semantic; });
    return it == profile.node_capabilities.end() ? nullptr : &*it;
}
const coreflow::NodeCapability* find_cap_path(
    const coreflow::DeviceProfileSnapshot& profile, const std::filesystem::path& path) {
    const auto it = std::find_if(profile.node_capabilities.begin(), profile.node_capabilities.end(),
        [&](const auto& cap) { return cap.path == path.string(); });
    return it == profile.node_capabilities.end() ? nullptr : &*it;
}
}
int main() {
    using namespace coreflow;
    TunerTelemetryInput idle{};
    idle.cpu_psi_valid = idle.memory_psi_valid = idle.io_psi_valid = true;
    assert(AutonomousTuner::classify_regime(idle, WorkloadClass::Idle) == DynamicWorkloadRegime::IdleDeep);
    auto light = idle; light.cpu_psi = 0.7;
    assert(AutonomousTuner::classify_regime(light, WorkloadClass::BackgroundWorkload) == DynamicWorkloadRegime::LightLoad);
    auto interactive = idle; interactive.cpu_psi = 3.0;
    assert(AutonomousTuner::classify_regime(interactive, WorkloadClass::GeneralInteraction) == DynamicWorkloadRegime::Interactive);
    auto moderate = idle; moderate.memory_psi = 2.0;
    assert(AutonomousTuner::classify_regime(moderate, WorkloadClass::MemoryPressure) == DynamicWorkloadRegime::ModerateLoad);
    auto burst = idle; burst.io_psi = 4.2;
    assert(AutonomousTuner::classify_regime(burst, WorkloadClass::StorageIoPressure) == DynamicWorkloadRegime::BurstLoading);
    assert(AutonomousTuner::classify_regime(idle, WorkloadClass::Gaming) == DynamicWorkloadRegime::HeavySustained);
    std::vector<uint32_t> cpus;
    assert(DeviceProfiler::parse_cpu_list("0-3,6,8-9\n", cpus));
    assert((cpus == std::vector<uint32_t>{0,1,2,3,6,8,9}));
    assert(DeviceProfiler::parse_cpu_list("0,2-3,5\n", cpus));
    assert((cpus == std::vector<uint32_t>{0,2,3,5}));
    // cpufreq related_cpus/affected_cpus are commonly whitespace-delimited.
    assert(DeviceProfiler::parse_cpu_list("0 1 2 3\n", cpus));
    assert((cpus == std::vector<uint32_t>{0,1,2,3}));
    assert(DeviceProfiler::parse_cpu_list("0-3 6 8-9\r\n", cpus));
    assert((cpus == std::vector<uint32_t>{0,1,2,3,6,8,9}));
    assert(DeviceProfiler::parse_cpu_list("0-1, 3 5-6\t7", cpus));
    assert((cpus == std::vector<uint32_t>{0,1,3,5,6,7}));
    assert(!DeviceProfiler::parse_cpu_list("0-3,,5", cpus));
    assert(!DeviceProfiler::parse_cpu_list("0, ,1", cpus));
    assert(!DeviceProfiler::parse_cpu_list("0-3,  \n", cpus));
    assert(!DeviceProfiler::parse_cpu_list("3-1", cpus));
    assert(!DeviceProfiler::parse_cpu_list("0,0", cpus));

    // A: profile -> classify -> baseline -> act -> readback -> restore.
    // No IMPROVED verdict is fabricated without comparable workload telemetry.
    auto a = cddp_fixture::create("A");
    const auto policy_a = a.sys / "devices/system/cpu/cpufreq/policy0";
    cddp_fixture::put(policy_a / "scaling_available_governors", "schedutil vendor_governor\n");
    cddp_fixture::put(policy_a / "schedutil/vendor_adaptive_level", "2\n");
    // Model a known physical ABI device. Virtual devices remain present in the
    // profile but never receive runtime mutation authority.
    const auto storage_a = a.sys / "class/block/mmcblk0";
    cddp_fixture::put(storage_a / "size", "2097152\n");
    cddp_fixture::put(storage_a / "queue/nr_hw_queues", "4\n");
    cddp_fixture::put(storage_a / "queue/rotational", "0\n");
    cddp_fixture::put(storage_a / "queue/scheduler", "none [mq-deadline] kyber\n");
    cddp_fixture::put(storage_a / "queue/read_ahead_kb", "128\n");
    cddp_fixture::put(storage_a / "queue/nr_requests", "128\n");
    cddp_fixture::put(storage_a / "queue/rq_affinity", "1\n");
    cddp_fixture::put(storage_a / "queue/max_segment_size", "65536\n", 0444);
    cddp_fixture::put(storage_a / "queue/virt_boundary_mask", "4095\n", 0444);
    cddp_fixture::put(storage_a / "queue/zone_append_max_bytes", "0\n", 0444);
    cddp_fixture::put(storage_a / "queue/stable_writes", "0\n");
    cddp_fixture::put(storage_a / "queue/write_cache", "write back\n");
    cddp_fixture::put(storage_a / "queue/iosched/read_expire", "500\n");
    // A real app PID in the uniquely-resolved CPU foreground group proves the
    // UClamp contract without relying on cgroup-directory names alone.
    cddp_fixture::put(a.sys / "fs/cgroup/interactive-session/cgroup.procs", "123\n");
    cddp_fixture::put(a.proc / "123/status", "Name:\tgame\nUid:\t10123\t10123\t10123\t10123\nTgid:\t123\n");
    cddp_fixture::put(a.proc / "123/cgroup", "0::/interactive-session\n");
    cddp_fixture::put(a.sys / "block/zram0/disksize", "1073741824\n");
    cddp_fixture::put(a.sys / "block/zram0/comp_algorithm", "[lz4] lzo lzo-rle zstd\n");
    cddp_fixture::put(a.proc / "swaps", "Filename Type Size Used Priority\n/dev/block/zram0 partition 1048576 128 0\n");
    const std::string state_dir = (a.root / "state").string();
    (void)::setenv("COREFLOW_TEST_STATE_DIR", state_dir.c_str(), 1);
    auto& baseline_store = BaselineStore::instance();
    assert(baseline_store.recover_pending());
    DeviceProfiler pa(a.sys.string(), a.proc.string());
    const auto profile_a = pa.refresh();
    assert(profile_a && profile_a->cpu_topology_valid);
    assert(profile_a->cpu_policies.size() == 1 && profile_a->cpu_policies.front().related_cpus_valid);
    assert(profile_a->cpu_policies.front().cluster_role == "unified");
    assert((profile_a->cpu_policies.front().available_governors ==
            std::vector<std::string>{"schedutil", "vendor_governor"}));
    assert(std::find(profile_a->cpu_policies.front().governor_tunables.begin(),
                     profile_a->cpu_policies.front().governor_tunables.end(),
                     "vendor_adaptive_level") != profile_a->cpu_policies.front().governor_tunables.end());
    assert(profile_a->block_devices.size() == 1U);
    assert(profile_a->block_devices.front().hardware_queues == 4U);
    assert(profile_a->block_devices.front().capacity_bytes == 2097152ULL * 512ULL);
    assert(profile_a->block_devices.front().rotational_valid && !profile_a->block_devices.front().rotational);
    assert(profile_a->block_devices.front().active_scheduler == "mq-deadline");
    const auto* read_ahead_cap = find_cap_path(*profile_a, storage_a / "queue/read_ahead_kb");
    const auto* segment_cap = find_cap_path(*profile_a, storage_a / "queue/max_segment_size");
    const auto* boundary_cap = find_cap_path(*profile_a, storage_a / "queue/virt_boundary_mask");
    const auto* zone_cap = find_cap_path(*profile_a, storage_a / "queue/zone_append_max_bytes");
    const auto* stable_cap = find_cap_path(*profile_a, storage_a / "queue/stable_writes");
    const auto* cache_cap = find_cap_path(*profile_a, storage_a / "queue/write_cache");
    const auto* scheduler_cap = find_cap_path(*profile_a, storage_a / "queue/scheduler");
    assert(read_ahead_cap && read_ahead_cap->classification == KernelNodeClassification::RuntimeTunable);
    assert(segment_cap && segment_cap->classification == KernelNodeClassification::ObservationOnly);
    assert(boundary_cap && boundary_cap->classification == KernelNodeClassification::ObservationOnly);
    assert(zone_cap && zone_cap->classification == KernelNodeClassification::ObservationOnly);
    assert(stable_cap && stable_cap->classification == KernelNodeClassification::TestOnly);
    assert(cache_cap && cache_cap->classification == KernelNodeClassification::Protected);
    assert(scheduler_cap && scheduler_cap->classification == KernelNodeClassification::RuntimeTunable);
    const auto* uclamp_min = find_cap_path(*profile_a,
        a.sys / "fs/cgroup/interactive-session/cpu.uclamp.min");
    const auto* uclamp_max = find_cap_path(*profile_a,
        a.sys / "fs/cgroup/interactive-session/cpu.uclamp.max");
    assert(uclamp_min && uclamp_min->classification == KernelNodeClassification::RuntimeTunable);
    assert(uclamp_max && uclamp_max->classification == KernelNodeClassification::RuntimeTunable);
    assert(uclamp_min->mutation_eligible() && uclamp_max->mutation_eligible());
    assert(std::find(profile_a->block_devices.front().iosched_tunables.begin(),
                     profile_a->block_devices.front().iosched_tunables.end(), "read_expire") !=
           profile_a->block_devices.front().iosched_tunables.end());
    assert(profile_a->memory_hardware.total_ram_bytes == 8192000ULL * 1024ULL);
    assert(profile_a->memory_hardware.zram_devices.size() == 1U);
    assert(profile_a->memory_hardware.zram_devices.front().swap_active);
    assert(profile_a->memory_hardware.zram_devices.front().active_algorithm == "lz4");
    assert(profile_a->memory_telemetry_available && profile_a->cgroup_v2_available);
    assert(!profile_a->thermal_sensors.empty());
    const auto* cpu_max = find_cap(*profile_a, "cpufreq.scaling_max_freq");
    assert(cpu_max && cpu_max->mutation_eligible());
    const auto profile_a_refresh = pa.refresh();
    assert(profile_a_refresh->generation > profile_a->generation);
    assert(profile_a_refresh->fingerprint == profile_a->fingerprint);
    // Observation-only physical queue constraints are part of the hardware
    // fingerprint, while mutable tuner values are intentionally excluded.
    cddp_fixture::put(storage_a / "queue/max_segment_size", "131072\n", 0444);
    const auto hardware_changed = pa.refresh();
    assert(hardware_changed->fingerprint != profile_a_refresh->fingerprint);
    const auto* refreshed_segment = find_cap_path(*hardware_changed,
        storage_a / "queue/max_segment_size");
    assert(refreshed_segment && refreshed_segment->current_value == "131072");

    CentralMutationCoordinator coordinator;
    {
        FrequencyController controller(coordinator, a.sys.string(), profile_a);
        controller.discover(true, false);
        assert(controller.cpu_count() == 1 && controller.gpu_count() == 0);
        assert(controller.apply(0.75));
        assert(read_value(a.sys / "devices/system/cpu/cpufreq/policy0/scaling_max_freq") == "1500000");
        assert(controller.apply(1.0));
        assert(read_value(a.sys / "devices/system/cpu/cpufreq/policy0/scaling_max_freq") == "2000000");
    }
    {
        AutonomousTuner tuner(coordinator, a.sys.string(), a.proc.string(), profile_a);
        assert(!tuner.is_candidate_eligible("cpufreq.schedutil_interactive_response"));
        assert(tuner.snapshot().capability_fingerprint.find(profile_a->fingerprint) != std::string::npos);
        const auto invalid_evidence = tuner.step({
            .cpu_psi = 0.0, .memory_psi = 0.0, .io_psi = 0.0,
            .cpu_psi_valid = false, .memory_psi_valid = false, .io_psi_valid = false,
            .thermal_pressure_valid = false,
        });
        assert(invalid_evidence.verdict == TunerVerdict::Inconclusive);
        assert(invalid_evidence.last_verdict == "INCONCLUSIVE");
        assert(read_value(a.sys / "devices/system/cpu/cpufreq/policy0/scaling_max_freq") == "2000000");
    }

    // B: exact non-contiguous CPU membership.
    auto b = cddp_fixture::create("B");
    DeviceProfiler pb(b.sys.string(), b.proc.string());
    const auto profile_b = pb.refresh();
    assert(profile_b->cpu_topology_valid);
    assert((profile_b->cpu_policies.front().related_cpus == std::vector<uint32_t>{0,2,3}));
    assert((profile_b->online_cpus == std::vector<uint32_t>{0,1,3,4,5}));

    // C: malformed topology fails closed.
    auto c = cddp_fixture::create("C");
    DeviceProfiler pc(c.sys.string(), c.proc.string());
    const auto profile_c = pc.refresh();
    assert(!profile_c->cpu_topology_valid);
    const auto* cap_c = find_cap(*profile_c, "cpufreq.scaling_max_freq");
    assert(cap_c && !cap_c->mutation_eligible());

    // D: missing frequency table prevents constraints verification.
    auto d = cddp_fixture::create("D");
    DeviceProfiler pd(d.sys.string(), d.proc.string());
    const auto profile_d = pd.refresh();
    const auto* cap_d = find_cap(*profile_d, "cpufreq.scaling_max_freq");
    assert(cap_d && !cap_d->mutation_eligible() && cap_d->state == NodeCapabilityState::SemanticsVerified);

    // E: write mode denied even when CI executes as uid 0.
    auto e = cddp_fixture::create("E");
    DeviceProfiler pe(e.sys.string(), e.proc.string());
    const auto profile_e = pe.refresh();
    const auto* cap_e = find_cap(*profile_e, "cpufreq.scaling_max_freq");
    assert(cap_e && !cap_e->mutation_eligible() && cap_e->state == NodeCapabilityState::Readable);

    // F: contradictory min/max bounds fail constraints verification.
    auto f = cddp_fixture::create("F");
    DeviceProfiler pf(f.sys.string(), f.proc.string());
    const auto profile_f = pf.refresh();
    const auto* cap_f = find_cap(*profile_f, "cpufreq.scaling_max_freq");
    assert(cap_f && !cap_f->mutation_eligible() && cap_f->state == NodeCapabilityState::SemanticsVerified);

    // G: generic devfreq is not presumed to be a GPU.
    auto g = cddp_fixture::create("G");
    DeviceProfiler pg(g.sys.string(), g.proc.string());
    assert(pg.refresh()->gpu_domains.empty());

    // H: Mali devfreq uses Hz and an explicit advertised table.
    auto h = cddp_fixture::create("H");
    DeviceProfiler ph(h.sys.string(), h.proc.string());
    const auto profile_h = ph.refresh();
    assert(profile_h->gpu_domains.size() == 1 && profile_h->gpu_domains.front().frequency_table_valid);
    const auto* gpu_max = find_cap(*profile_h, "devfreq.max_freq");
    assert(gpu_max && gpu_max->mutation_eligible() && gpu_max->unit == "Hz");

    // I: KGSL controls are observation-only.
    auto i = cddp_fixture::create("I");
    DeviceProfiler pi(i.sys.string(), i.proc.string());
    const auto profile_i = pi.refresh();
    assert(!profile_i->gpu_domains.empty());
    for (const auto& cap : profile_i->node_capabilities)
        if (cap.semantic_id == "kgsl.vendor_control.unverified") assert(!cap.mutation_eligible());

    // K: whitespace-separated CPU policy membership is parsed.
    auto k = cddp_fixture::create("K");
    DeviceProfiler pk(k.sys.string(), k.proc.string());
    const auto profile_k = pk.refresh();
    assert(profile_k->cpu_policies.size() == 1);
    assert(profile_k->cpu_policies.front().related_cpus_valid);
    assert((profile_k->cpu_policies.front().related_cpus == std::vector<uint32_t>{0,1,2,3}));

    // L: malformed related_cpus falls back to valid affected_cpus.
    auto l = cddp_fixture::create("L");
    DeviceProfiler pl(l.sys.string(), l.proc.string());
    const auto profile_l = pl.refresh();
    assert(profile_l->cpu_policies.size() == 1);
    assert(profile_l->cpu_policies.front().related_cpus_valid);
    assert((profile_l->cpu_policies.front().related_cpus == std::vector<uint32_t>{0,1,2,3}));

    // J: absent subsystems stay absent; no synthesized defaults.
    auto j = cddp_fixture::create("J");
    DeviceProfiler pj(j.sys.string(), j.proc.string());
    const auto profile_j = pj.refresh();
    assert(!profile_j->cpu_topology_valid && profile_j->cpu_policies.empty());
    assert(profile_j->gpu_domains.empty() && profile_j->thermal_sensors.empty());
    assert(!profile_j->memory_telemetry_available && !profile_j->cgroup_v2_available);

    (void)::unsetenv("COREFLOW_TEST_STATE_DIR");
    return 0;
}
