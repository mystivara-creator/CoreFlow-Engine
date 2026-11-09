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
}
int main() {
    using namespace coreflow;
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
    DeviceProfiler pa(a.sys.string(), a.proc.string());
    const auto profile_a = pa.refresh();
    assert(profile_a && profile_a->cpu_topology_valid);
    assert(profile_a->cpu_policies.size() == 1 && profile_a->cpu_policies.front().related_cpus_valid);
    assert(profile_a->memory_telemetry_available && profile_a->cgroup_v2_available);
    assert(!profile_a->thermal_sensors.empty());
    const auto* cpu_max = find_cap(*profile_a, "cpufreq.scaling_max_freq");
    assert(cpu_max && cpu_max->mutation_eligible());
    const auto profile_a_refresh = pa.refresh();
    assert(profile_a_refresh->generation > profile_a->generation);
    assert(profile_a_refresh->fingerprint == profile_a->fingerprint);

    const std::string state_dir = (a.root / "state").string();
    (void)::setenv("COREFLOW_TEST_STATE_DIR", state_dir.c_str(), 1);
    auto& baseline_store = BaselineStore::instance();
    const bool baseline_recovered = baseline_store.recover_pending();
    assert(baseline_recovered);
    (void)baseline_recovered;
    {
        FrequencyController controller(a.sys.string(), profile_a);
        controller.discover(true, false);
        assert(controller.cpu_count() == 1 && controller.gpu_count() == 0);
        assert(controller.apply(0.75));
        assert(read_value(a.sys / "devices/system/cpu/cpufreq/policy0/scaling_max_freq") == "1500000");
        assert(controller.apply(1.0));
        assert(read_value(a.sys / "devices/system/cpu/cpufreq/policy0/scaling_max_freq") == "2000000");
    }
    {
        AutonomousTuner tuner(a.sys.string(), a.proc.string(), profile_a);
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
