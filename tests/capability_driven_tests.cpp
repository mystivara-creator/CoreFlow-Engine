// Capability-driven contract: vendor knowledge, fatal-only guard, generic
// tunable discovery and evidence-derived candidates.
#include "coreflow/consultation.hpp"
#include "coreflow/effect_model.hpp"
#include "coreflow/experience.hpp"
#include "coreflow/fatal_surface.hpp"
#include "coreflow/tunable_discovery.hpp"
#include "coreflow/vendor_knowledge.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sys/stat.h>

namespace fs = std::filesystem;
using namespace coreflow;

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { ++failures; std::cerr << "FAIL: " << #x << " (line " << __LINE__ << ")\n"; } } while (0)

void writeFile(const fs::path& p, const std::string& v, mode_t mode = 0644) {
    fs::create_directories(p.parent_path());
    std::ofstream(p) << v << "\n";
    ::chmod(p.c_str(), mode);
}

const ResourceCapability* findCap(const EnvironmentCapabilityMatrix& m, const std::string& path) {
    for (const auto& c : m.resources) if (c.path == path) return &c;
    return nullptr;
}

void testVendorParsers() {
    VendorKnowledge vk;
    // init.rc writes + trigger-derived profile.
    const char* rc =
        "on boot\n"
        "    write /proc/sys/vm/swappiness 100\n"
        "    write /proc/sys/kernel/sched_util_clamp_min 512\n"
        "on property:vendor.power.lowpower=1\n"
        "    write /sys/class/kgsl/kgsl-3d0/max_pwrlevel 6\n"
        "    write /proc/sys/vm/${unknown} 1\n";  // non-literal: ignored
    CHECK(vk.parseInitRc(rc, "init.vendor.rc") == 3U);
    const VendorKnob* swap = vk.find("/proc/sys/vm/swappiness");
    CHECK(swap != nullptr && swap->declaresWithProfile("100", VendorProfile::Boot));
    const VendorKnob* lvl = vk.find("/sys/class/kgsl/kgsl-3d0/max_pwrlevel");
    CHECK(lvl != nullptr && lvl->declaresWithProfile("6", VendorProfile::Efficiency));

    // Vendor shell (post_boot style).
    const char* sh =
        "#!/vendor/bin/sh\n"
        "echo 20000 > /sys/devices/system/cpu/cpufreq/policy0/schedutil/up_rate_limit_us\n"
        "echo \"0 1 2 3\" > /dev/cpuset/background/cpus; echo 1 > /proc/sys/kernel/sched_x\n"
        "configure_sustained_mode() {\n"
        "    echo 1497600 > /sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq\n"
        "}\n"
        "echo $VAR > /proc/sys/vm/dirty_ratio\n";
    CHECK(vk.parseShell(sh, "init.post_boot.sh") == 4U);
    const VendorKnob* cpus = vk.find("/dev/cpuset/background/cpus");
    CHECK(cpus != nullptr && cpus->declares("0 1 2 3"));
    const VendorKnob* maxf = vk.find("/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq");
    CHECK(maxf != nullptr && maxf->declaresWithProfile("1497600", VendorProfile::Efficiency));
    CHECK(vk.find("/proc/sys/vm/dirty_ratio") == nullptr);

    // Power HAL JSON: node values + per-hint actions.
    const char* json = R"({
      "Nodes": [
        {"Name": "GPUMaxFreq", "Path": "/sys/class/devfreq/gpu/max_freq",
         "Values": ["900000000", "600000000", "400000000"], "DefaultIndex": 0},
        {"Name": "TAUclampMin", "Path": "/dev/cpuctl/top-app/cpu.uclamp.min",
         "Values": ["20", "0"], "DefaultIndex": 1}
      ],
      "Actions": [
        {"PowerHint": "SUSTAINED_PERFORMANCE", "Node": "GPUMaxFreq", "Value": "400000000"},
        {"PowerHint": "LAUNCH", "Node": "TAUclampMin", "Value": "20"}
      ]
    })";
    CHECK(vk.parsePowerHintJson(json, "powerhint.json") >= 5U);
    const VendorKnob* gpu = vk.find("/sys/class/devfreq/gpu/max_freq");
    CHECK(gpu != nullptr && gpu->declaresWithProfile("900000000", VendorProfile::Default));
    CHECK(gpu != nullptr && gpu->declaresWithProfile("400000000", VendorProfile::Efficiency));
    const VendorKnob* clamp = vk.find("/dev/cpuctl/top-app/cpu.uclamp.min");
    CHECK(clamp != nullptr && clamp->declaresWithProfile("20", VendorProfile::Performance));
    CHECK(clamp != nullptr && clamp->declaresWithProfile("0", VendorProfile::Default));

    // Malformed JSON never throws or adds anything.
    VendorKnowledge broken;
    CHECK(broken.parsePowerHintJson("{\"Nodes\": [ {", "bad.json") == 0U);
    CHECK(broken.parsePowerHintJson(std::string(json) + "garbage", "trailing.json") == 0U);
    CHECK(broken.knobs().empty());
    CHECK(broken.parsePowerHintJson(R"({"Nodes":[],"Nodes":[]})", "duplicate.json") == 0U);
    CHECK(broken.parsePowerHintJson(R"({"Nodes":[],"unused":01})", "number.json") == 0U);
    CHECK(broken.parsePowerHintJson(R"({"Nodes":[],"unused":"bad\q"})", "escape.json") == 0U);
    CHECK(broken.parsePowerHintJson(R"({"Nodes":[],"unused":"\uD800"})", "surrogate.json") == 0U);
    VendorKnowledge escaped;
    CHECK(escaped.parsePowerHintJson(R"({"Nodes":[{"Name":"vm","Path":"\u002fproc/sys/vm/swappiness","Values":["60"],"DefaultIndex":0}],"Actions":[{"PowerHint":"LAUNCH","Node":"vm","Value":"999"}]})", "unicode.json") == 1U);
    const auto* escaped_knob = escaped.find("/proc/sys/vm/swappiness");
    CHECK(escaped_knob && escaped_knob->declaresWithProfile("60", VendorProfile::Default));
    CHECK(escaped_knob && !escaped_knob->declares("999"));
    VendorKnowledge nul;
    CHECK(nul.parsePowerHintJson(R"({"Nodes":[{"Name":"vm","Path":"/proc/sys/vm/swappiness\u0000ignored","Values":["60"]}]})", "nul.json") == 0U);
    CHECK(nul.knobs().empty());
    CHECK(broken.parsePowerHintJson(std::string(VendorKnowledge::kMaxFileBytes + 1U, ' '), "large.json") == 0U);

}

void testFatalGuard() {
    // Fatal classes on every device.
    for (const char* p : {
             "/proc/sys/vm/drop_caches", "/proc/sys/kernel/panic", "/proc/sys/kernel/panic_on_oops",
             "/proc/sys/kernel/sysrq", "/proc/sys/kernel/hung_task_timeout_secs",
             "/proc/sys/kernel/kptr_restrict", "/proc/sys/kernel/randomize_va_space",
             "/proc/sys/kernel/sched_rt_runtime_us", "/proc/sys/vm/overcommit_memory",
             "/proc/sys/vm/nr_hugepages", "/proc/sys/vm/mmap_min_addr", "/proc/sys/kernel/pid_max",
             "/sys/class/thermal/thermal_zone0/mode", "/sys/class/thermal/cooling_device0/cur_state",
             "/sys/class/kgsl/kgsl-3d0/throttling", "/sys/class/power_supply/battery/input_suspend",
             "/sys/power/state", "/sys/devices/system/cpu/cpu7/online",
             "/sys/block/sda/queue/write_cache", "/sys/block/sda/device/delete",
             "/sys/fs/selinux/enforce", "/sys/kernel/debug/x", "/dev/cpuctl/top-app/cgroup.procs",
             "/dev/cpuctl/cpu.rt_runtime_us", "/dev/cpuset/top-app/mems",
             "/sys/fs/cgroup/memory.max", "/sys/bus/platform/drivers/x/unbind",
             "/dev/block/sda", "/data/adb/x",
             // Incident regression: these must NEVER be considered tunables.
             "/sys/devices/platform/soc/ae00000.qcom,mdss_mdp/drm/card0/card0-DSI-1/status",
             "/sys/class/drm/card0-DSI-1/status",
             "/sys/devices/platform/soc/1d84000.ufshc/auto_hibern8",
             "/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable",
             "/sys/devices/platform/soc/1d84000.ufshc/block/sda/queue/nr_requests",
             "/sys/devices/system/cpu/cpu0/core_ctl/not_preferred",
             "/sys/devices/system/cpu/cpu0/core_ctl/enable",
             "/sys/class/backlight/panel0-backlight/brightness",
             "/sys/class/graphics/fb0/blank"}) {
        if (!isFatalSurface(p)) { ++failures; std::cerr << "FAIL: not fatal " << p << "\n"; }
    }
    // Ordinary tunables across every capability domain.
    for (const char* p : {
             "/proc/sys/vm/swappiness", "/proc/sys/vm/watermark_scale_factor",
             "/proc/sys/kernel/sched_util_clamp_min", "/proc/sys/walt/sched_boost",
             "/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq",
             "/sys/devices/system/cpu/cpufreq/policy0/schedutil/rate_limit_us",
             "/sys/block/sda/queue/scheduler",
             "/sys/block/sda/queue/iosched/fifo_batch", "/sys/class/devfreq/gpu/governor",
             "/sys/class/kgsl/kgsl-3d0/devfreq/max_freq", "/dev/cpuctl/top-app/cpu.uclamp.max",
             "/dev/stune/top-app/schedtune.boost", "/dev/cpuset/background/cpus"}) {
        if (isFatalSurface(p)) { ++failures; std::cerr << "FAIL: fatal " << p << "\n"; }
    }
    // Value invariants.
    CHECK(isFatalValue("60", "-1"));        // sign flip to "unlimited/disabled"
    CHECK(isFatalValue("60", ""));
    CHECK(isFatalValue("60", "1\n2"));
    CHECK(!isFatalValue("60", "53"));
    CHECK(!isFatalValue("-1", "100"));
    CHECK(!isFatalValue("[mq-deadline] none", "none"));
}

void testGenericDiscovery() {
    const fs::path root = fs::temp_directory_path() / "coreflow_capdrv_tree";
    fs::remove_all(root);
    const std::string r = root.string();

    writeFile(root / "proc/sys/vm/swappiness", "60");
    writeFile(root / "proc/sys/vm/drop_caches", "0", 0200);     // fatal
    writeFile(root / "proc/sys/vm/stat_interval", "1", 0444);   // read-only
    writeFile(root / "proc/sys/kernel/sched_wakeup_granularity_ns", "1000000");
    writeFile(root / "proc/sys/kernel/panic", "5");             // fatal
    writeFile(root / "sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq", "1804800");
    writeFile(root / "sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies",
              "300000 1036800 1804800", 0444);
    writeFile(root / "sys/devices/system/cpu/cpufreq/policy0/scaling_governor", "schedutil");
    writeFile(root / "sys/devices/system/cpu/cpufreq/policy0/schedutil/rate_limit_us", "500");
    writeFile(root / "sys/devices/system/cpu/cpu1/online", "1");         // fatal
    writeFile(root / "sys/block/sda/queue/scheduler", "[mq-deadline] kyber none bfq");
    writeFile(root / "sys/block/sda/queue/write_cache", "write back");   // fatal
    writeFile(root / "sys/block/sda/queue/iosched/fifo_batch", "16");
    // Real sysfs device-model path: /sys/block entries are commonly symlinks,
    // so discovery must also classify the canonical /sys/devices/.../block tree.
    writeFile(root / "sys/devices/platform/storage/block/mmcblk0/queue/iosched/fifo_batch", "24");
    writeFile(root / "sys/devices/platform/storage/block/mmcblk0/queue/extra_queue_knob", "8");
    writeFile(root / "sys/class/devfreq/gpu/governor", "msm-adreno-tz");
    writeFile(root / "sys/class/devfreq/gpu/available_governors", "msm-adreno-tz performance powersave", 0444);
    writeFile(root / "sys/class/devfreq/gpu/max_freq", "900000000");
    writeFile(root / "sys/class/devfreq/gpu/available_frequencies", "300000000 600000000 900000000", 0444);
    writeFile(root / "dev/cpuctl/top-app/cpu.uclamp.min", "10");
    writeFile(root / "dev/cpuctl/top-app/cgroup.procs", "1", 0644);      // fatal
    writeFile(root / "dev/cpuset/background/cpus", "0-3");
    writeFile(root / "vendor_only/custom_knob", "7");                    // vendor-declared only
    // The kernel mount table is the only source of tunable roots.
    writeFile(root / "proc/mounts",
              "none " + r + "/proc proc rw 0 0\n"
              "none " + r + "/sys sysfs rw 0 0\n"
              "none " + r + "/dev/cpuctl cgroup rw 0 0\n"
              "none " + r + "/dev/cpuset cgroup rw 0 0\n");

    VendorKnowledge vendor;
    const std::string rc =
        "on boot\n    write " + r + "/vendor_only/custom_knob 4\n"
        "on property:vendor.low_power=1\n    write " + r + "/dev/cpuset/background/cpus 0-1\n";
    // Fixture paths are under /tmp, which is a host-test tunable root.
    (void)vendor.parseInitRc(rc, "fixture.rc");

    EnvironmentCapabilityMatrix matrix;
    const TunableDiscovery discovery(r);
    CHECK(discovery.nameFor(r + "/proc/sys/vm/swappiness") ==
          "vm.swappiness");
    CHECK(discovery.nameFor("/alternate/proc/sys/kernel/sched_latency_ns") ==
          "kernel.sched_latency_ns");
    const TunableDiscoveryStats stats = discovery.discover(matrix, vendor);
    CHECK(stats.added >= 10U);
    CHECK(stats.fatal_skipped >= 4U);
    CHECK(stats.visited_entries <= TunableDiscovery::kMaxVisits);

    const auto* swap = findCap(matrix, r + "/proc/sys/vm/swappiness");
    CHECK(swap && swap->domain == ResourceDomain::Memory && swap->name == "vm.swappiness" &&
          swap->mutation_ready);
    CHECK(!findCap(matrix, r + "/proc/sys/vm/drop_caches"));
    CHECK(!findCap(matrix, r + "/proc/sys/vm/stat_interval"));  // no write bit
    CHECK(!findCap(matrix, r + "/proc/sys/kernel/panic"));
    CHECK(!findCap(matrix, r + "/sys/devices/system/cpu/cpu1/online"));
    CHECK(!findCap(matrix, r + "/sys/block/sda/queue/write_cache"));
    CHECK(!findCap(matrix, r + "/dev/cpuctl/top-app/cgroup.procs"));

    const auto* sched = findCap(matrix, r + "/proc/sys/kernel/sched_wakeup_granularity_ns");
    CHECK(sched && sched->domain == ResourceDomain::Scheduler && sched->mutation_ready);

    const auto* maxf = findCap(matrix, r + "/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq");
    CHECK(maxf && maxf->domain == ResourceDomain::CpuFreq && !maxf->mutation_ready);
    const auto* gov = findCap(matrix, r + "/sys/devices/system/cpu/cpufreq/policy0/scaling_governor");
    CHECK(gov && !gov->mutation_ready);  // governor authority is separate
    const auto* rate = findCap(matrix, r + "/sys/devices/system/cpu/cpufreq/policy0/schedutil/rate_limit_us");
    CHECK(rate && rate->domain == ResourceDomain::Scheduler && rate->mutation_ready);

    const auto* iosched = findCap(matrix, r + "/sys/block/sda/queue/scheduler");
    CHECK(iosched && iosched->domain == ResourceDomain::Io && iosched->options.size() == 4U &&
          !iosched->ordered_scale && iosched->name == "sda:scheduler");
    const auto* fifo = findCap(matrix, r + "/sys/block/sda/queue/iosched/fifo_batch");
    CHECK(fifo && fifo->name == "sda:iosched.fifo_batch");
    const auto* canonical_fifo = findCap(
        matrix, r + "/sys/devices/platform/storage/block/mmcblk0/queue/iosched/fifo_batch");
    CHECK(canonical_fifo && canonical_fifo->domain == ResourceDomain::Io &&
          canonical_fifo->name == "mmcblk0:iosched.fifo_batch" && canonical_fifo->mutation_ready);
    const auto* unknown_queue_knob = findCap(
        matrix, r + "/sys/devices/platform/storage/block/mmcblk0/queue/extra_queue_knob");
    CHECK(unknown_queue_knob && unknown_queue_knob->domain == ResourceDomain::Io &&
          unknown_queue_knob->name == "mmcblk0:extra_queue_knob" && unknown_queue_knob->mutation_ready);

    const auto* gpu_gov = findCap(matrix, r + "/sys/class/devfreq/gpu/governor");
    CHECK(gpu_gov && gpu_gov->domain == ResourceDomain::Gpu && gpu_gov->options.size() == 3U);
    const auto* gpu_max = findCap(matrix, r + "/sys/class/devfreq/gpu/max_freq");
    CHECK(gpu_max && gpu_max->ordered_scale && gpu_max->options.front() == "300000000");

    const auto* clamp = findCap(matrix, r + "/dev/cpuctl/top-app/cpu.uclamp.min");
    CHECK(clamp && clamp->domain == ResourceDomain::UClamp && clamp->mutation_ready);
    const auto* cpus = findCap(matrix, r + "/dev/cpuset/background/cpus");
    CHECK(cpus && cpus->domain == ResourceDomain::CpuSet && !cpus->vendor_values.empty());

    // A node no walker covers is discovered because the vendor configures it.
    const auto* custom = findCap(matrix, r + "/vendor_only/custom_knob");
    CHECK(custom && custom->mutation_ready && custom->domain == ResourceDomain::Vendor &&
          !custom->vendor_values.empty());
    CHECK(stats.vendor_paths >= 1U);

    // ---- Candidates derived purely from that evidence ----
    EffectModel model;
    EffectContext hot;
    hot.state = RuntimeState::ThermalGuard;
    hot.thermal_available = true;
    hot.thermal_c = 56.0;
    hot.mem_available_ratio = 0.5;
    hot.sample_confidence = 0.95;
    hot.intervention = InterventionLevel::Low;

    // Thermal: a step down the advertised GPU scale; CPU policy frequency
    // bounds remain PowerHAL-owned and ineligible for generic actuation.
    CHECK(gpu_max != nullptr && maxf != nullptr);
    if (gpu_max == nullptr || maxf == nullptr) return;
    const EffectScore g = model.evaluate(*gpu_max, "900000000", hot);
    CHECK(g.eligible && g.requested == "600000000" && g.stabilizing &&
          g.source == CandidateSource::OrderedScale);
    const EffectScore c = model.evaluate(*maxf, "1804800", hot);
    CHECK(!c.eligible && c.requested.empty());
    // Thermal: a lower utilization clamp.
    const EffectScore u = model.evaluate(*clamp, "10", hot);
    CHECK(u.eligible && u.stabilizing && u.requested == "9");
    // Fixed-point uclamp values keep their precision.
    const EffectScore ud = model.evaluate(*clamp, "10.00", hot);
    CHECK(ud.eligible && ud.stabilizing && ud.requested == "8.75");
    // A zero clamp cannot go lower: nothing to propose under thermal stress.
    CHECK(!model.evaluate(*clamp, "0.00", hot).eligible);
    // A vendor scalar with "uclamp" in its name does not inherit the standard
    // cgroup percentage range unless it is the cpu.uclamp.min/max interface.
    ResourceCapability custom_uclamp = *clamp;
    custom_uclamp.name = "vendor.uclamp_profile";
    custom_uclamp.path = r + "/dev/cpuctl/top-app/vendor.uclamp_profile";
    custom_uclamp.options.clear();
    custom_uclamp.vendor_values.clear();
    CHECK(model.proposals(custom_uclamp, "10", hot).empty());
    // Thermal: the vendor's low-power cpuset is the stabilizing choice.
    const EffectScore cs = model.evaluate(*cpus, "0-3", hot);
    CHECK(cs.eligible && cs.requested == "0-1" && cs.source == CandidateSource::VendorEfficiency);
    // Thermal stress alone is not an I/O scheduler signal; advertised names
    // must not be treated as universally stabilizing.
    CHECK(!model.evaluate(*iosched, "mq-deadline", hot).eligible);
    EffectContext thermal_and_io = hot;
    thermal_and_io.psi_available = true;
    thermal_and_io.psi_io = 0.12;
    CHECK(model.proposals(*iosched, "mq-deadline", thermal_and_io).empty());
    // A strong, candidate-specific positive outcome prior can justify a
    // scheduler trial during matched I/O pressure; it never bypasses authority.
    thermal_and_io.has_experience = true;
    thermal_and_io.experience_confidence = 0.80;
    thermal_and_io.experience_score = 0.82;
    CHECK(!model.proposals(*iosched, "mq-deadline", thermal_and_io).empty());

    // CPU placement values are evidence-derived. Never synthesize or tune the
    // separate memory-node set through the generic CPU-set actuator.
    ResourceCapability cpuset_mems = *cpus;
    cpuset_mems.name = "background:cpuset.mems";
    cpuset_mems.path = r + "/dev/cpuset/background/mems";
    cpuset_mems.options = {"0"};
    CHECK(model.proposals(cpuset_mems, "0", hot).empty());

    EffectContext busy;
    busy.state = RuntimeState::Elevated;
    busy.cpu_utilization_available = true;
    busy.cpu_utilization = 0.85;
    busy.load1 = 3.0;
    busy.mem_available_ratio = 0.5;
    busy.sample_confidence = 0.95;
    busy.intervention = InterventionLevel::Moderate;

    // Headroom: vendor factory value outranks blind exploration.
    const EffectScore v = model.evaluate(*custom, "7", busy);
    CHECK(v.eligible && v.requested == "4" && v.source == CandidateSource::VendorDefault);
    // Headroom: kernel-advertised tokens are candidates.
    const EffectScore t = model.evaluate(*iosched, "mq-deadline", busy);
    CHECK(t.eligible && (t.requested == "kyber" || t.requested == "none" ||
                         t.requested == "bfq"));
    // Non-whitelisted/vendor scheduler names stay eligible when the kernel
    // advertises them; common-token ranking is only a weak prior under I/O PSI.
    EffectContext io_busy = busy;
    io_busy.psi_available = true;
    io_busy.psi_io = 0.15;
    const auto scheduler_proposals = model.proposals(*iosched, "mq-deadline", io_busy);
    bool saw_bfq = false;
    for (const auto& proposal : scheduler_proposals) {
        if (proposal.requested == "bfq") saw_bfq = true;
    }
    CHECK(saw_bfq);
    EffectContext io_only = io_busy;
    io_only.state = RuntimeState::Elevated;
    io_only.cpu_utilization = 0.10;
    io_only.load1 = 0.10;
    io_only.mem_available_ratio = 0.50;
    CHECK(!model.proposals(*iosched, "mq-deadline", io_only).empty());
    CHECK(model.proposals(*custom, "7", io_only).empty());
    // Layered/mapper paths are not categorically blocked by name. They remain
    // candidates only when discovery, policy, grants, journaling and the
    // actuator accept the exact live node.
    ResourceCapability mapped_scheduler = *iosched;
    mapped_scheduler.path = "/sys/block/dm-0/queue/scheduler";
    mapped_scheduler.name = "dm-0:scheduler";
    CHECK(model.evaluate(mapped_scheduler, "mq-deadline", busy).eligible);

    EnvironmentCapabilityMatrix ranking_matrix;
    ranking_matrix.resources = {*custom, *iosched};
    const std::unordered_map<std::string, std::string> ranking_baselines = {
        {custom->path, "7"}, {iosched->path, "mq-deadline"}};
    const auto full = model.rank(ranking_matrix, ranking_baselines, busy, 100U);
    const auto top = model.rank(ranking_matrix, ranking_baselines, busy, 1U);
    CHECK(full.size() > 1U && top.size() == 1U);
    if (!full.empty() && !top.empty())
        CHECK(top[0].path == full[0].path && top[0].requested == full[0].requested && top[0].utility == full[0].utility);
    CHECK(model.rank(ranking_matrix, ranking_baselines, busy, 0U).empty());

    // Bounded numeric exploration must not overflow at signed integer limits.
    ResourceCapability edge_numeric = *custom;
    edge_numeric.name = "vendor.edge_numeric";
    edge_numeric.path = "/proc/sys/kernel/vendor_edge_numeric";
    edge_numeric.options.clear();
    edge_numeric.vendor_values.clear();
    const auto edge_candidates = model.proposals(
        edge_numeric, std::to_string(std::numeric_limits<long long>::min()), busy);
    CHECK(edge_candidates.size() <= 2U);
    // Opaque multi-field value without vendor data is never synthesized.
    ResourceCapability opaque = *cpus;
    opaque.vendor_values.clear();
    CHECK(!model.evaluate(opaque, "0-3", busy).eligible);
    // 0/1 toggles are never flipped without vendor evidence.
    ResourceCapability toggle = *swap;
    CHECK(!model.evaluate(toggle, "1", busy).eligible);
    CHECK(!model.evaluate(toggle, "0", busy).eligible);

    // Modern Android ZRAM OEM swappiness (e.g. 120 on Linux 5.8+ 0..200 scale)
    // is a valid baseline and never steps past the kernel's 200 ceiling.
    const auto swap120 = model.proposals(*swap, "120", busy);
    CHECK(!swap120.empty());
    for (const auto& cand : swap120) {
        const long long requested_value = std::stoll(cand.requested);
        CHECK(requested_value >= 0LL && requested_value <= 200LL);
    }
    const auto swap195 = model.proposals(*swap, "195", busy);
    for (const auto& cand : swap195) {
        const long long requested_value = std::stoll(cand.requested);
        CHECK(requested_value >= 0LL && requested_value <= 200LL);
    }

    // Block queue rq_affinity is bounded to {0, 1, 2}; baseline 2 never proposes 3.
    ResourceCapability rq_aff{};
    rq_aff.exists = rq_aff.readable = rq_aff.writable = rq_aff.mutation_ready = true;
    rq_aff.permission_granted = rq_aff.policy_authorized = rq_aff.runtime_verified = true;
    rq_aff.domain = ResourceDomain::Io;
    rq_aff.name = "sda:rq_affinity";
    rq_aff.path = r + "/sys/block/sda/queue/rq_affinity";
    const auto rq_cands = model.proposals(rq_aff, "2", busy);
    CHECK(rq_cands.size() == 1U && rq_cands[0].requested == "1");

    fs::remove_all(root);
}


// Roots are the mount table, not a list in the source. Without a sysfs mount,
// nothing under sys/ is searched even though the node exists and is writable.
// Without a cgroup mount, the uclamp group is not searched either.
void testRootsComeFromMountTable() {
    const fs::path root = fs::temp_directory_path() / "coreflow_capdrv_mounts";
    fs::remove_all(root);
    const std::string r = root.string();

    writeFile(root / "proc/sys/vm/swappiness", "60");
    writeFile(root / "sys/block/sda/queue/read_ahead_kb", "128");
    writeFile(root / "dev/cpuctl/top-app/cpu.uclamp.min", "10");
    writeFile(root / "proc/mounts", "none " + r + "/proc proc rw 0 0\n");

    EnvironmentCapabilityMatrix matrix;
    VendorKnowledge vendor;
    const TunableDiscoveryStats stats = TunableDiscovery(r).discover(matrix, vendor);
    CHECK(findCap(matrix, r + "/proc/sys/vm/swappiness") != nullptr);
    CHECK(findCap(matrix, r + "/sys/block/sda/queue/read_ahead_kb") == nullptr);
    CHECK(findCap(matrix, r + "/dev/cpuctl/top-app/cpu.uclamp.min") == nullptr);
    CHECK(stats.added == 1U);
    fs::remove_all(root);
}

// A writable node in a recognised tree is admitted only when its subsystem is
// recognised or the vendor declares it. Unknown writable nodes remain present
// in the generic domain and are counted as unclaimed. Symlinks are never followed.
void testDiscoveryCapacityReportsTruncation() {
    EnvironmentCapabilityMatrix saturated;
    saturated.resources.resize(TunableDiscovery::kMaxTunables);
    const TunableDiscoveryStats stats = TunableDiscovery{}.discover(saturated, VendorKnowledge{});
    CHECK(stats.truncated);
    CHECK(stats.visited_entries == 0U);
    CHECK(stats.added == 0U);
    CHECK(saturated.resources.size() == TunableDiscovery::kMaxTunables);
}

void testDiscoveryInputAndDirectoryBounds() {
    const fs::path root = fs::temp_directory_path() / "coreflow_capdrv_bounds";
    fs::remove_all(root);
    const std::string r = root.string();

    // An overlong scalar cannot be silently truncated into a plausible value.
    writeFile(root / "sys/class/devfreq/gpu/odd_attr", std::string(5000U, '7'));
    // A wide, unclaimed directory exercises the per-directory memory/work cap.
    for (std::size_t i = 0; i < TunableDiscovery::kMaxVisits / 9U + 1U; ++i) {
        const std::string leaf = "entry" + std::to_string(i);
        writeFile(root / "sys/unmapped" / leaf, "1");
    }
    writeFile(root / "proc/mounts", "none " + r + "/sys sysfs rw 0 0\n");

    EnvironmentCapabilityMatrix matrix;
    VendorKnowledge vendor;
    const TunableDiscoveryStats stats = TunableDiscovery(r).discover(matrix, vendor);
    CHECK(stats.truncated);
    CHECK(stats.visited_entries <= TunableDiscovery::kMaxDirectoryEntries + 16U);
    CHECK(findCap(matrix, r + "/sys/class/devfreq/gpu/odd_attr") == nullptr);
    CHECK(matrix.resources.size() < TunableDiscovery::kMaxTunables);
    fs::remove_all(root);
}

void testUnclaimedAndSymlinks() {
    const fs::path root = fs::temp_directory_path() / "coreflow_capdrv_unclaimed";
    fs::remove_all(root);
    const std::string r = root.string();

    writeFile(root / "sys/class/net/wlan0/tx_queue_len", "1000");  // unrecognised
    writeFile(root / "sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq", "1804800");
    fs::create_directories(root / "sys/bus/cpu/devices");
    fs::create_directory_symlink(root / "sys/devices/system/cpu/cpufreq/policy0",
                                 root / "sys/bus/cpu/devices/policy0");
    writeFile(root / "proc/mounts",
              "none " + r + "/sys sysfs rw 0 0\n");

    EnvironmentCapabilityMatrix matrix;
    VendorKnowledge vendor;
    const TunableDiscoveryStats stats = TunableDiscovery(r).discover(matrix, vendor);
    const auto* unknown = findCap(matrix, r + "/sys/class/net/wlan0/tx_queue_len");
    CHECK(unknown && unknown->domain == ResourceDomain::Vendor && unknown->mutation_ready);
    CHECK(stats.unclaimed >= 1U);
    const auto* maxf = findCap(matrix, r + "/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq");
    CHECK(maxf != nullptr && maxf->domain == ResourceDomain::CpuFreq);
    std::size_t copies = 0;
    for (const auto& c : matrix.resources) {
        if (c.path.find("/sys/bus/") != std::string::npos) ++copies;
    }
    CHECK(copies == 0U);
    fs::remove_all(root);
}

void testSiliconHeadroomAndBandit() {
    VendorKnowledge vk;
    vk.parseVendorPerfXml(R"(
        <ResourceConfigs>
            <PerfResources>
                <Config Target="parrot"
                        SysfsNode="/sys/devices/system/cpu/cpufreq/policy4/walt/up_rate_limit_us"
                        DefaultValue="5000"
                        BoostValue="500" />
                <Entry path="/sys/class/kgsl/kgsl-3d0/min_pwrlevel" value="2" profile="gaming" />
            </PerfResources>
        </ResourceConfigs>
    )", "targetresourceconfigs.xml");

    const auto* walt_knob = vk.find("/sys/devices/system/cpu/cpufreq/policy4/walt/up_rate_limit_us");
    CHECK(walt_knob != nullptr);
    if (walt_knob != nullptr) {
        CHECK(walt_knob->hasPerformanceHeadroom("5000"));
        CHECK(!walt_knob->hasPerformanceHeadroom("500"));
    }
    const auto* gpu_knob = vk.find("/sys/class/kgsl/kgsl-3d0/min_pwrlevel");
    CHECK(gpu_knob != nullptr && gpu_knob->hasPerformanceHeadroom("5"));

    // Sibling tcp_available_congestion_control & cluster OPP inheritance.
    const fs::path root = fs::temp_directory_path() / "coreflow_capdrv_silicon";
    fs::remove_all(root);
    const std::string r = root.string();
    writeFile(root / "proc/sys/net/ipv4/tcp_congestion_control", "cubic");
    writeFile(root / "proc/sys/net/ipv4/tcp_available_congestion_control", "reno cubic bbr");
    writeFile(root / "sys/devices/system/cpu/cpufreq/policy4/scaling_available_frequencies",
              "691200 1401600 2016000 2400000");
    writeFile(root / "sys/devices/system/cpu/cpufreq/policy4/walt/hispeed_freq", "1401600");
    writeFile(root / "sys/class/devfreq/gpu/min_freq", "300000000");
    writeFile(root / "sys/class/devfreq/gpu/freq_table_mhz", "300 600 9223372036854775807 9999999999999999999999");
    writeFile(root / "proc/mounts",
              "none " + r + "/proc proc rw 0 0\n"
              "none " + r + "/sys sysfs rw 0 0\n");

    EnvironmentCapabilityMatrix matrix;
    TunableDiscovery(r).discover(matrix, vk);
    const auto* tcp = findCap(matrix, r + "/proc/sys/net/ipv4/tcp_congestion_control");
    CHECK(tcp != nullptr && !tcp->ordered_scale && tcp->options.size() == 3U);
    const auto* hispeed = findCap(matrix, r + "/sys/devices/system/cpu/cpufreq/policy4/walt/hispeed_freq");
    CHECK(hispeed != nullptr && hispeed->ordered_scale && hispeed->options.size() == 4U);
    const auto* scaled_gpu=findCap(matrix,r+"/sys/class/devfreq/gpu/min_freq");
    CHECK(scaled_gpu!=nullptr && scaled_gpu->ordered_scale && scaled_gpu->options.size()==2U);
    if (scaled_gpu && scaled_gpu->options.size()==2U) {
        CHECK(scaled_gpu->options[0]=="300000000" && scaled_gpu->options[1]=="600000000");
    }
    fs::remove_all(root);

    // Contextual UCB1-Tuned Bandit rewards novel/beneficial arms and suppresses regressions.
    const double novel_ucb = contextualBanditScore(0.60, nullptr, 12U, 0.08);
    CHECK(novel_ucb > 0.60 && novel_ucb <= 1.0);

    ExperienceMemory::Record reg_rec{};
    reg_rec.candidate.key = "resource:/test:bad";
    reg_rec.outcome = BaselineIntelligence::Outcome::Regression;
    reg_rec.score = 0.20;
    reg_rec.confidence = 0.85;
    reg_rec.variance = 0.02;
    reg_rec.observations = 4U;
    const double reg_ucb = contextualBanditScore(0.60, &reg_rec, 16U, 0.08);
    CHECK(reg_ucb < 0.40);
}

} // namespace

int main() {
    testVendorParsers();
    testFatalGuard();
    testGenericDiscovery();
    testRootsComeFromMountTable();
    testDiscoveryCapacityReportsTruncation();
    testDiscoveryInputAndDirectoryBounds();
    testUnclaimedAndSymlinks();
    testSiliconHeadroomAndBandit();
    if (failures == 0) std::cout << "CoreFlow capability-driven contract: PASS\n";
    return failures == 0 ? 0 : 1;
}
