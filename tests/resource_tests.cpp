#include "coreflow/effect_model.hpp"
#include "coreflow/ecosystem_knowledge.hpp"
#include "coreflow/resource_actuator.hpp"
#include "coreflow/resource_mutation.hpp"
#include "coreflow/tunable_discovery.hpp"
#include "coreflow/retained_state.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

using namespace coreflow;
namespace fs = std::filesystem;

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { ++failures; std::fprintf(stderr, "CHECK FAILED: %s\n", #x); } } while (0)

std::string readFile(const fs::path& p) {
    std::ifstream f(p);
    std::string s;
    std::getline(f, s);
    return s;
}

void writeFile(const fs::path& p, const std::string& s) {
    std::ofstream f(p, std::ios::trunc);
    f << s << '\n';
}

class Journal final : public MutationJournal {
public:
    Entries entries;
    bool pending{false};
    bool commit(const Entries& e) noexcept override { entries = e; pending = true; return true; }
    bool clear() noexcept override { entries.clear(); pending = false; return true; }
    LoadState load(Entries& out) noexcept override { out = entries; return pending ? LoadState::Pending : LoadState::Absent; }
};

MutationResult applyResourcePolicy(
    ResourceMutationController& controller,
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const EngineConfig& config,
    InterventionLevel intervention) {

    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.intervention = intervention;
    plan.mutation_eligible = true;

    plan.candidates.push_back({
        ResourceDomain::Memory,
        "vm.swappiness",
        {},
        sample.confidence,
        1.0,
        PolicyAction::Candidate
    });

    for (const auto& device : profile.io_devices) {
        plan.candidates.push_back({
            ResourceDomain::Io,
            device.name + ":read_ahead_kb",
            {},
            sample.confidence,
            1.0,
            PolicyAction::Candidate
        });

        plan.candidates.push_back({
            ResourceDomain::Io,
            device.name + ":nr_requests",
            {},
            sample.confidence,
            1.0,
            PolicyAction::Candidate
        });

        plan.candidates.push_back({
            ResourceDomain::Io,
            device.name + ":scheduler",
            {},
            sample.confidence,
            1.0,
            PolicyAction::Candidate
        });
    }

    const MutationAuthority authority;
    const MutationPermit permit = authority.authorize(
        plan,
        config,
        state,
        sample.confidence,
        MutationPermit::Scope::Resource);

    return controller.apply(
        state,
        sample,
        profile,
        config,
        plan,
        permit);
}

// Safety property: with mutation armed and adaptive, but without the user's
// grant, a device under real pressure must still leave the kernel untouched.
// The brain defers, the hands never run, and the ledger says why.
void test_no_write_without_user_grant() {
    const fs::path root = fs::temp_directory_path() / "coreflow_resource_no_grant_test";
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    profile.vm_swappiness.path = swappiness.string();
    profile.vm_swappiness.readable = true;
    profile.vm_swappiness.writable = true;

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 100;
    sample.mem_available_ratio = 0.10;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.2;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    // Grants are opt-out; this user explicitly opts every namespace out.
    config.setGrantMemory(false);
    config.setGrantBlockQueue(false);
    config.setGrantScheduler(false);
    config.setGrantGpu(false);
    config.setGrantCgroup(false);
    config.setGrantCpuFreq(false);
    config.setGrantKernel(false);

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);
    CHECK(controller.captureBaseline(profile));
    const MutationResult result = applyResourcePolicy(
        controller, RuntimeState::Elevated, sample, profile, config,
        InterventionLevel::Low);

    CHECK(result != MutationResult::Verified);
    CHECK(readFile(swappiness) == "60");
    CHECK(!controller.mutated());
    const auto& ledger = controller.consultationLedger();
    CHECK(ledger.by_reason[static_cast<std::size_t>(ConsultReason::AwaitingUserGrant)] >= 1U);
    CHECK(ledger.by_reason[static_cast<std::size_t>(ConsultReason::Allowed)] == 0U);
}

void test_swappiness_adaptive_path() {
    const fs::path root = fs::temp_directory_path() / "coreflow_resource_mutation_test";
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    profile.vm_swappiness.path = swappiness.string();
    profile.vm_swappiness.readable = true;
    profile.vm_swappiness.writable = true;

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 100;
    sample.mem_available_ratio = 0.10;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.2;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    // The user has granted these namespaces; a device need alone never does.
    config.setGrantMemory(true);
    config.setGrantBlockQueue(true);
    config.setGrantScheduler(true);

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);
    CHECK(controller.captureBaseline(profile));
    CHECK(applyResourcePolicy(
          controller,
          RuntimeState::Elevated,
          sample,
          profile,
          config,
          InterventionLevel::Low) == MutationResult::Verified);
    CHECK(readFile(swappiness) == "53");  // bounded step, conservative probe first
    CHECK(controller.mutated());
    CHECK(controller.restoreAll());
    CHECK(readFile(swappiness) == "60");
    CHECK(!controller.mutated());
    fs::remove_all(root);
}

void test_retained_resource_state_survives_restart_and_revalidates() {
    const fs::path root = fs::temp_directory_path() / "coreflow_resource_retained_test";
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path swappiness = root / "swappiness";
    const fs::path retainedPath = root / "retained.db";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    profile.vm_swappiness.path = swappiness.string();
    profile.vm_swappiness.readable = true;
    profile.vm_swappiness.writable = true;

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 100;
    sample.mem_available_ratio = 0.10;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.2;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    // The user has granted these namespaces; a device need alone never does.
    config.setGrantMemory(true);
    config.setGrantBlockQueue(true);
    config.setGrantScheduler(true);

    Journal firstJournal;
    FileRetainedStateStore retained(retainedPath.string());
    ResourceMutationController first;
    first.setJournal(&firstJournal);
    first.setRetainedStateStore(&retained);
    CHECK(first.captureBaseline(profile));
    CHECK(applyResourcePolicy(first, RuntimeState::Elevated, sample, profile, config,
                              InterventionLevel::Low) == MutationResult::Verified);
    CHECK(first.retainLastMutation(profile));
    CHECK(readFile(swappiness) == "53");  // bounded step, conservative probe first
    CHECK(first.activeResourceSettings().size() == 1U);
    CHECK(first.restoreAll());
    CHECK(readFile(swappiness) == "60");
    CHECK(first.activeResourceSettings().empty());

    // Re-open the store as a new daemon would. The retained value is a
    // preference only; normal capability/policy/actuator validation still runs.
    Journal secondJournal;
    FileRetainedStateStore reopened(retainedPath.string());
    ResourceMutationController restarted;
    restarted.setJournal(&secondJournal);
    restarted.setRetainedStateStore(&reopened);
    CHECK(restarted.captureBaseline(profile));
    CHECK(applyResourcePolicy(restarted, RuntimeState::Elevated, sample, profile, config,
                              InterventionLevel::Low) == MutationResult::Verified);
    CHECK(readFile(swappiness) == "53");  // bounded step, conservative probe first
    CHECK(restarted.hasPendingOutcome());
    CHECK(restarted.rollbackLastMutation());
    CHECK(readFile(swappiness) == "60");

    RetainedStateStore::Entries entries;
    CHECK(reopened.load(entries) == RetainedStateStore::LoadState::Absent);
    fs::remove_all(root);
}

void test_resource_restore_only_owned_paths() {
    const fs::path root = fs::temp_directory_path() / "coreflow_resource_owned_test";
    fs::remove_all(root);
    fs::create_directories(root / "queue");
    const fs::path swappiness = root / "swappiness";
    const fs::path readAhead = root / "queue" / "read_ahead_kb";
    writeFile(swappiness, "60");
    writeFile(readAhead, "128");

    DeviceProfile profile;
    profile.vm_swappiness.path = swappiness.string();
    profile.vm_swappiness.readable = true;
    profile.vm_swappiness.writable = true;
    // The block device stays in the profile so restore isolation is exercised,
    // but read_ahead is not writable here: block-queue writes are quarantined,
    // and this test is about owning only the swappiness write it performed.
    IoDevice device;
    device.path = root.string();
    device.name = "testblk";
    device.read_ahead_readable = true;
    device.read_ahead_writable = false;
    profile.io_devices.push_back(device);

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 100;
    sample.mem_available_ratio = 0.10;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.8;
    sample.cpu_utilization_available = true;
    sample.load1 = 2.0;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    // The user has granted these namespaces; a device need alone never does.
    config.setGrantMemory(true);
    config.setGrantBlockQueue(true);
    config.setGrantScheduler(true);

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);
    CHECK(controller.captureBaseline(profile));
    CHECK(applyResourcePolicy(
          controller,
          RuntimeState::Elevated,
          sample,
          profile,
          config,
          InterventionLevel::Low) == MutationResult::Verified);
    CHECK(readFile(swappiness) == "53");  // bounded step, conservative probe first
    writeFile(readAhead, "2048");

    CHECK(controller.restoreAll());
    CHECK(readFile(swappiness) == "60");
    CHECK(readFile(readAhead) == "2048");
    CHECK(!journal.pending);
    fs::remove_all(root);
}


void test_policy_block_without_mutation_is_skipped() {
    const fs::path root =
        fs::temp_directory_path() /
        "coreflow_resource_policy_skip_test";

    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path swappiness =
        root / "swappiness";

    writeFile(swappiness, "60");

    DeviceProfile profile;
    profile.vm_swappiness.path = swappiness.string();
    profile.vm_swappiness.readable = true;
    profile.vm_swappiness.writable = true;

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 500;
    sample.mem_available_ratio = 0.50;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.20;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    // The user has granted these namespaces; a device need alone never does.
    config.setGrantMemory(true);
    config.setGrantBlockQueue(true);
    config.setGrantScheduler(true);

    PolicyPlan blocked;
    blocked.action = PolicyAction::ReduceIntervention;
    blocked.intervention = InterventionLevel::ObserveOnly;
    blocked.mutation_eligible = false;
    blocked.reason = "thermal safety guard";

    const MutationAuthority authority;
    const MutationPermit permit = authority.authorize(
        blocked,
        config,
        RuntimeState::ThermalGuard,
        sample.confidence,
        MutationPermit::Scope::Resource);

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);

    CHECK(controller.captureBaseline(profile));

    const MutationResult result = controller.apply(
        RuntimeState::ThermalGuard,
        sample,
        profile,
        config,
        blocked,
        permit);

    CHECK(result == MutationResult::Skipped);
    CHECK(!controller.mutated());
    CHECK(readFile(swappiness) == "60");

    fs::remove_all(root);
}


void test_intervention_level_boundaries() {
    auto run_case = [](InterventionLevel level,
                       bool read_ahead,
                       bool nr_requests,
                       bool scheduler,
                       const std::string& expected_path,
                       const std::string& expected_value) {
        const fs::path root =
            fs::temp_directory_path() /
            ("coreflow_intervention_" +
             std::to_string(static_cast<int>(level)));

        fs::remove_all(root);
        fs::create_directories(root / "queue");

        const fs::path readAhead =
            root / "queue" / "read_ahead_kb";
        const fs::path requests =
            root / "queue" / "nr_requests";
        const fs::path schedulerPath =
            root / "queue" / "scheduler";
        const fs::path rotational =
            root / "rotational";

        writeFile(readAhead, "128");
        writeFile(requests, "128");
        writeFile(schedulerPath, "[mq-deadline] none");
        writeFile(rotational, "0");

        DeviceProfile profile;

        IoDevice device;
        device.path = root.string();
        device.name = "testblk";

        device.read_ahead_readable = read_ahead;
        device.read_ahead_writable = read_ahead;

        device.nr_requests_readable = nr_requests;
        device.nr_requests_writable = nr_requests;

        device.scheduler_readable = scheduler;
        device.scheduler_writable = scheduler;

        profile.io_devices.push_back(device);

        RuntimeSample sample;
        sample.mem_total_kb = 1000;
        sample.mem_available_kb = 500;
        sample.mem_available_ratio = 0.50;
        sample.confidence = 1.0;
        sample.cpu_utilization = 0.80;
        sample.cpu_utilization_available = true;
        sample.load1 = 2.0;
        sample.io_activity_available = true;
        sample.io_read_kb_per_sec = 2048.0;
        sample.io_write_kb_per_sec = 1024.0;

        EngineConfig config;
        config.setMutationMode(MutationMode::Adaptive);
        config.setMutationArmed(true);
        // The user has granted these namespaces; a device need alone never does.
        config.setGrantMemory(true);
        config.setGrantBlockQueue(true);
        config.setGrantScheduler(true);

        Journal journal;
        ResourceMutationController controller;
        controller.setJournal(&journal);

        CHECK(controller.captureBaseline(profile));

        CHECK(applyResourcePolicy(
            controller,
            RuntimeState::Elevated,
            sample,
            profile,
            config,
            level) == MutationResult::Verified);

        CHECK(readFile(expected_path) == expected_value);
        CHECK(controller.mutated());

        CHECK(controller.restoreAll());

        fs::remove_all(root);
    };

    {
        const fs::path root =
            fs::temp_directory_path() /
            "coreflow_intervention_1";

        run_case(
            InterventionLevel::Low,
            true,
            false,
            false,
            root / "queue" / "read_ahead_kb",
            "112");
    }

    {
        const fs::path root =
            fs::temp_directory_path() /
            "coreflow_intervention_2";

        run_case(
            InterventionLevel::Moderate,
            false,
            true,
            false,
            root / "queue" / "nr_requests",
            "112");
    }

    {
        const fs::path root =
            fs::temp_directory_path() /
            "coreflow_intervention_3";

        run_case(
            InterventionLevel::High,
            false,
            false,
            true,
            root / "queue" / "scheduler",
            "none");
    }
}


void test_disarmed_policy_performs_no_write() {
    const fs::path root =
        fs::temp_directory_path() /
        "coreflow_resource_disarmed_test";

    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    DeviceProfile profile;
    profile.vm_swappiness.path = swappiness.string();
    profile.vm_swappiness.readable = true;
    profile.vm_swappiness.writable = true;

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 500;
    sample.mem_available_ratio = 0.50;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.80;
    sample.cpu_utilization_available = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(false);

    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.intervention = InterventionLevel::Low;
    plan.mutation_eligible = true;
    plan.candidates.push_back({
        ResourceDomain::Memory,
        "vm.swappiness",
        {},
        sample.confidence,
        1.0,
        PolicyAction::Candidate
    });

    const MutationAuthority authority;
    const MutationPermit permit = authority.authorize(
        plan,
        config,
        RuntimeState::Elevated,
        sample.confidence,
        MutationPermit::Scope::Resource);

    CHECK(!permit.validFor(MutationPermit::Scope::Resource));

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);

    CHECK(controller.captureBaseline(profile));

    const MutationResult result = controller.apply(
        RuntimeState::Elevated,
        sample,
        profile,
        config,
        plan,
        permit);

    CHECK(result == MutationResult::Skipped);
    CHECK(!controller.mutated());
    CHECK(readFile(swappiness) == "60");
    CHECK(!journal.pending);

    fs::remove_all(root);
}



void test_effect_model_structured_rank_and_unknown_semantics() {
    EffectModel model;
    EnvironmentCapabilityMatrix matrix;

    ResourceCapability known;
    known.domain = ResourceDomain::Memory;
    known.name = "vm.swappiness";
    known.path = "/proc/sys/vm/swappiness";
    known.exists = true;
    known.readable = true;
    known.writable = true;
    known.permission_granted = true;
    known.policy_authorized = true;
    known.runtime_verified = true;
    known.mutation_ready = true;
    matrix.resources.push_back(known);

    ResourceCapability unknown = known;
    unknown.name = "experimental_numeric_knob";
    unknown.path = "/proc/sys/vm/experimental_numeric_knob";
    matrix.resources.push_back(unknown);

    std::unordered_map<std::string, std::string> baselines{
        {known.path, "60"}, {unknown.path, "100"}};
    EffectContext context;
    context.state = RuntimeState::Pressure;
    context.mem_available_ratio = 0.08;
    context.sample_confidence = 0.95;
    context.intervention = InterventionLevel::Low;

    const auto ranked = model.rank(matrix, baselines, context, 4);
    CHECK(!ranked.empty());
    bool swappiness_ranked = false;
    for (const auto& score : ranked) {
        if (score.path == known.path) {
            swappiness_ranked = true;
            CHECK(score.domain == ResourceDomain::Memory);
            CHECK(score.utility > 0.0);
            CHECK(score.requested == "53" || score.requested == "67");
            CHECK(score.source == CandidateSource::BoundedStep);
        }
    }
    CHECK(swappiness_ranked);

    // Capability-driven: a node with no name-table entry is still a candidate.
    const EffectScore unknown_score = model.evaluate(unknown, "100", context);
    CHECK(unknown_score.eligible);
    CHECK(unknown_score.requested == "88" || unknown_score.requested == "112");

    ResourceCapability notAuthorized = known;
    notAuthorized.policy_authorized = false;
    const EffectScore denied = model.evaluate(notAuthorized, "60", context);
    CHECK(!denied.eligible);
    CHECK(denied.requested.empty());

    // Catalog lookup must distinguish dynamic per-device queue nodes.
    const auto* scheduler = lookupEcosystemKnowledgeByPath(
        "/sys/block/dm-0/queue/scheduler");
    const auto* readAhead = lookupEcosystemKnowledgeByPath(
        "/sys/block/mmcblk0/queue/read_ahead_kb");
    const auto* requests = lookupEcosystemKnowledgeByPath(
        "/sys/devices/platform/soc/1d84000.ufshc/block/sda/queue/nr_requests");
    CHECK(scheduler != nullptr && std::string(scheduler->key) == "block.queue.scheduler");
    CHECK(readAhead != nullptr && std::string(readAhead->key) == "block.queue.read_ahead_kb");
    CHECK(requests != nullptr && std::string(requests->key) == "block.queue.nr_requests");

    // Fatal surfaces are never candidates, even if marked mutation_ready.
    ResourceCapability fatal = known;
    fatal.name = "vm.drop_caches";
    fatal.path = "/proc/sys/vm/drop_caches";
    CHECK(!model.evaluate(fatal, "3", context).eligible);

    // Vendor knowledge outranks blind exploration.
    ResourceCapability vendor = known;
    vendor.name = "vm.vendor_knob";
    vendor.path = "/proc/sys/vm/vendor_knob";
    vendor.vendor_values.push_back(VendorValue{"40", VendorProfile::Efficiency, "test.rc"});
    const EffectScore vendor_score = model.evaluate(vendor, "100", context);
    CHECK(vendor_score.eligible);
    CHECK(vendor_score.requested == "40");
    CHECK(vendor_score.source == CandidateSource::VendorEfficiency);
}

void test_block_queue_mutation_targets_are_admitted_for_physical() {
    // Capability-driven: every kernel tunable is admitted, including scheduler
    // tokens, virtual block devices, sched sysctls, GPU and uclamp nodes.
    CHECK(ResourceActuator::mutationTargetAllowed("/sys/block/loop0/queue/scheduler"));
    CHECK(ResourceActuator::mutationTargetAllowed("/sys/block/dm-0/queue/nr_requests"));
    CHECK(ResourceActuator::mutationTargetAllowed("/sys/block/zram0/queue/read_ahead_kb"));
    CHECK(ResourceActuator::mutationTargetAllowed("/sys/block/mmcblk0/queue/scheduler"));
    CHECK(ResourceActuator::mutationTargetAllowed("/proc/sys/kernel/sched_latency_ns"));
    CHECK(ResourceActuator::mutationTargetAllowed("/proc/sys/vm/swappiness"));
    CHECK(ResourceActuator::mutationTargetAllowed("/proc/sys/vm/page-cluster"));
    CHECK(ResourceActuator::mutationTargetAllowed("/sys/class/kgsl/kgsl-3d0/max_gpuclk"));
    CHECK(ResourceActuator::mutationTargetAllowed("/sys/class/devfreq/soc:qcom,gpubw/governor"));
    CHECK(ResourceActuator::mutationTargetAllowed("/dev/cpuctl/top-app/cpu.uclamp.min"));
    CHECK(ResourceActuator::mutationTargetAllowed(
        "/sys/devices/system/cpu/cpufreq/policy4/schedutil/up_rate_limit_us"));
    CHECK(ResourceActuator::mutationTargetAllowed("/tmp/coreflow-test/queue/scheduler"));

    // Only universally fatal classes are refused.
    CHECK(!ResourceActuator::mutationTargetAllowed("/proc/sys/vm/drop_caches"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/proc/sys/kernel/panic"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/proc/sys/vm/overcommit_memory"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/sys/class/thermal/thermal_zone0/mode"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/sys/class/power_supply/battery/charge_control_limit"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/sys/devices/system/cpu/cpu1/online"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/sys/block/sda/queue/write_cache"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/sys/fs/selinux/enforce"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/dev/cpuctl/top-app/cgroup.procs"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/sys/kernel/debug/sched_features"));
    CHECK(!ResourceActuator::mutationTargetAllowed("/data/local/tmp/anything"));
    // The CPU governor belongs to MutationController.
    CHECK(!ResourceActuator::mutationTargetAllowed(
        "/sys/devices/system/cpu/cpufreq/policy0/scaling_governor"));

    // The central actuator guard rejects fatal paths before any I/O.
    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.mutation_eligible = true;
    plan.intervention = InterventionLevel::Low;
    const MutationAuthority authority;
    const MutationPermit permit = authority.authorize(
        plan, config, RuntimeState::Elevated, 1.0,
        MutationPermit::Scope::Resource);
    CHECK(permit.validFor(MutationPermit::Scope::Resource));
    ResourceActuator actuator;
    const ActuatorMutation mutation{
        {ActuatorDomain::Vm, 0}, "/proc/sys/vm/drop_caches", "3"};
    const ActuatorResult result = actuator.apply(mutation, permit);
    CHECK(result.status == ActuatorStatus::SafetyRejected);
    CHECK(result.writes_attempted == 0);
}

} // namespace

// Regression: sysfs selector nodes such as queue/scheduler read back as
// "[mq-deadline] kyber none". The kernel accepts only the bare token on write,
// so a baseline captured as the bracketed line could never be restored.
void test_selector_value_comparable_form() {
    CHECK(ResourceActuator::comparableValue("[mq-deadline] kyber none") == "mq-deadline");
    CHECK(ResourceActuator::comparableValue("mq-deadline [none]") == "none");
    CHECK(ResourceActuator::comparableValue("none") == "none");
    CHECK(ResourceActuator::comparableValue("  128 \n") == "128");
    CHECK(ResourceActuator::comparableValue("performance") == "performance");
    CHECK(ResourceActuator::comparableValue("") == "");
}

void test_selector_scheduler_restore_writes_bare_token() {
    const fs::path root =
        fs::temp_directory_path() / "coreflow_selector_scheduler_regress";
    fs::remove_all(root);
    fs::create_directories(root / "queue");
    const fs::path schedulerPath = root / "queue" / "scheduler";
    writeFile(schedulerPath, "[mq-deadline] none");

    DeviceProfile profile;
    IoDevice device;
    device.path = root.string();
    device.name = "testblk";
    device.scheduler_readable = true;
    device.scheduler_writable = true;
    profile.io_devices.push_back(device);

    RuntimeSample sample;
    sample.mem_total_kb = 1000;
    sample.mem_available_kb = 500;
    sample.mem_available_ratio = 0.50;
    sample.confidence = 1.0;
    sample.cpu_utilization = 0.80;
    sample.cpu_utilization_available = true;
    sample.load1 = 2.0;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    // The user has granted these namespaces; a device need alone never does.
    config.setGrantMemory(true);
    config.setGrantBlockQueue(true);
    config.setGrantScheduler(true);

    Journal journal;
    ResourceMutationController controller;
    controller.setJournal(&journal);
    CHECK(controller.captureBaseline(profile));

    CHECK(applyResourcePolicy(controller, RuntimeState::Elevated, sample,
                              profile, config, InterventionLevel::High)
          == MutationResult::Verified);
    CHECK(readFile(schedulerPath) == "none");
    CHECK(controller.mutated());

    // Rollback must write the bare factory token the kernel accepts.
    CHECK(controller.restoreAll());
    CHECK(readFile(schedulerPath) == "mq-deadline");
    CHECK(!controller.mutated());

    fs::remove_all(root);
}

// A journal written by an earlier build may hold the bracketed line. Recovery
// and restore must normalize it instead of writing an unacceptable value.
void test_legacy_bracketed_journal_value_is_normalized() {
    const fs::path path = fs::temp_directory_path() / "coreflow_legacy_journal_sched";
    writeFile(path, "none");

    ResourceActuator actuator;
    const ActuatorResult result = actuator.restore(path.string(), "[mq-deadline] none");
    CHECK(result.status == ActuatorStatus::RolledBack);
    CHECK(readFile(path) == "mq-deadline");

    fs::remove(path);
}

// The CPU governor permit must not be issued without the separate opt-in.
void test_cpu_permit_requires_governor_opt_in() {
    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.mutation_eligible = true;

    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    // The user has granted these namespaces; a device need alone never does.
    config.setGrantMemory(true);
    config.setGrantBlockQueue(true);
    config.setGrantScheduler(true);
    config.setAllowCpuGovernor(false);

    const MutationAuthority authority;
    CHECK(!authority.authorize(plan, config, RuntimeState::Elevated, 1.0,
                               MutationPermit::Scope::CpuFreq)
              .validFor(MutationPermit::Scope::CpuFreq));

    config.setAllowCpuGovernor(true);
    CHECK(authority.authorize(plan, config, RuntimeState::Elevated, 1.0,
                              MutationPermit::Scope::CpuFreq)
              .validFor(MutationPermit::Scope::CpuFreq));
}

// ---- v2.1.1: graded safety. Stress allows small load-reducing writes only. ----
namespace stab {

struct Fixture {
    fs::path root;
    fs::path swappiness;
    fs::path gpu_max;
    DeviceProfile profile;
    RuntimeSample sample;
    EngineConfig config;
    Journal journal;
    ResourceMutationController controller;

    explicit Fixture(const std::string& name) {
        root = fs::temp_directory_path() / name;
        fs::remove_all(root);
        fs::create_directories(root);
        swappiness = root / "swappiness";
        writeFile(swappiness, "60");
        // An ordered hardware scale (GPU-style max_freq + available list).
        fs::create_directories(root / "gpu");
        gpu_max = root / "gpu" / "max_freq";
        writeFile(gpu_max, "600");
        writeFile(root / "gpu" / "available_frequencies", "300 450 600");
        {
            ResourceCapability cap;
            cap.domain = ResourceDomain::Gpu;
            cap.name = "gpu.max_freq";
            cap.path = gpu_max.string();
            cap.exists = cap.readable = cap.writable = true;
            cap.permission_granted = cap.policy_authorized = true;
            cap.runtime_verified = cap.mutation_ready = true;
            profile.capabilities.resources.push_back(cap);
            TunableDiscovery{}.enrich(profile.capabilities, VendorKnowledge{});
        }
        profile.vm_swappiness.path = swappiness.string();
        profile.vm_swappiness.readable = true;
        profile.vm_swappiness.writable = true;
        sample.mem_total_kb = 1000;
        sample.mem_available_kb = 500;
        sample.mem_available_ratio = 0.50;
        sample.confidence = 1.0;
        sample.cpu_utilization = 0.50;
        sample.cpu_utilization_available = true;
        sample.thermal_available = true;
        sample.thermal_millidegrees = 30000;
        config.setMutationMode(MutationMode::Adaptive);
        config.setMutationArmed(true);
        // The user has granted these namespaces; a device need alone never does.
        config.setGrantMemory(true);
        config.setGrantBlockQueue(true);
        config.setGrantScheduler(true);
        controller.setJournal(&journal);
    }
    ~Fixture() { fs::remove_all(root); }

    static PolicyPlan stabilizingPlan() {
        PolicyPlan plan;
        plan.action = PolicyAction::Candidate;
        plan.intervention = InterventionLevel::Low;
        plan.mutation_eligible = true;
        plan.stabilizing_only = true;
        plan.candidates.push_back({ResourceDomain::Memory, "vm.swappiness", {}, 1.0, 0.3,
                                   PolicyAction::Candidate});
        plan.candidates.push_back({ResourceDomain::Gpu, "gpu.max_freq", {}, 1.0, 0.3,
                                   PolicyAction::Candidate});
        return plan;
    }

    MutationResult run(RuntimeState state, const PolicyPlan& plan) {
        const MutationAuthority authority;
        const MutationPermit permit = authority.authorize(
            plan, config, state, sample.confidence, MutationPermit::Scope::Resource);
        return controller.apply(state, sample, profile, config, plan, permit);
    }
};

} // namespace stab

void test_thermal_guard_allows_stabilizing_write_and_restores() {
    stab::Fixture f("coreflow_stab_hot");
    f.sample.thermal_millidegrees = 55000;
    CHECK(f.controller.captureBaseline(f.profile));

    CHECK(f.run(RuntimeState::ThermalGuard, stab::Fixture::stabilizingPlan()) ==
          MutationResult::Verified);
    // Thermal stress: one step down the hardware's own advertised scale. The
    // memory knob has no load-reducing evidence and is left untouched.
    CHECK(readFile(f.gpu_max) == "450");
    CHECK(readFile(f.swappiness) == "60");
    CHECK(f.controller.mutated());
    CHECK(f.controller.stabilizingEpoch());
    CHECK(f.journal.pending);

    CHECK(f.controller.restoreAll());
    CHECK(readFile(f.gpu_max) == "600");
    CHECK(!f.controller.stabilizingEpoch());
    CHECK(!f.journal.pending);
}

void test_pressure_allows_stabilizing_write() {
    stab::Fixture f("coreflow_stab_pressure");
    f.sample.mem_available_ratio = 0.08;
    f.sample.mem_available_kb = 80;
    CHECK(f.controller.captureBaseline(f.profile));

    CHECK(f.run(RuntimeState::Pressure, stab::Fixture::stabilizingPlan()) ==
          MutationResult::Verified);
    // Memory pressure: the pressured domain itself is explored (measured and
    // rolled back if it does not help); the GPU scale is not a memory response.
    CHECK(readFile(f.swappiness) == "53");
    CHECK(readFile(f.gpu_max) == "600");
    CHECK(f.controller.stabilizingEpoch());
    CHECK(f.controller.restoreAll());
    CHECK(readFile(f.swappiness) == "60");
}

void test_stabilizing_plan_rejects_non_stabilizing_candidates() {
    // A calm system under a stabilizing-only plan: the EffectModel would offer a
    // mild efficiency tweak, but that is not a response to stress, so nothing is
    // written.
    stab::Fixture f("coreflow_stab_calm");
    CHECK(f.controller.captureBaseline(f.profile));
    CHECK(f.run(RuntimeState::Normal, stab::Fixture::stabilizingPlan()) ==
          MutationResult::Skipped);
    CHECK(readFile(f.swappiness) == "60");
    CHECK(!f.controller.mutated());
}

void test_safety_state_without_stabilizing_plan_never_writes() {
    stab::Fixture f("coreflow_stab_plain");
    f.sample.thermal_millidegrees = 55000;
    CHECK(f.controller.captureBaseline(f.profile));
    PolicyPlan plain = stab::Fixture::stabilizingPlan();
    plain.stabilizing_only = false;   // an ordinary candidate plan
    CHECK(f.run(RuntimeState::ThermalGuard, plain) == MutationResult::Skipped);
    CHECK(readFile(f.swappiness) == "60");
    CHECK(readFile(f.gpu_max) == "600");
    CHECK(!f.controller.mutated());
}

void test_regressed_candidate_is_suppressed() {
    stab::Fixture f("coreflow_stab_reject");
    f.sample.thermal_millidegrees = 55000;
    CHECK(f.controller.captureBaseline(f.profile));
    const PolicyPlan plan = stab::Fixture::stabilizingPlan();

    CHECK(f.run(RuntimeState::ThermalGuard, plan) == MutationResult::Verified);
    // The daemon rejects the candidate when its measured outcome regressed, then
    // restores.
    CHECK(readFile(f.gpu_max) == "450");
    f.controller.rejectLastMutation();
    CHECK(f.controller.restoreAll());
    CHECK(readFile(f.gpu_max) == "600");

    // The same candidate is not selected again within the cooldown.
    CHECK(f.run(RuntimeState::ThermalGuard, plan) == MutationResult::Skipped);
    CHECK(readFile(f.gpu_max) == "600");
    CHECK(!f.controller.mutated());
}


void test_resource_actuator_rejects_scope_only_permit() {
    const fs::path root = fs::temp_directory_path() / "coreflow_resource_scope_permit_test";
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path swappiness = root / "swappiness";
    writeFile(swappiness, "60");

    ActuatorMutation mutation;
    mutation.id = {ActuatorDomain::Vm, 0};
    mutation.target = swappiness.string();
    mutation.requested = "67";

    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.mutation_eligible = true;
    EngineConfig config;
    config.setMutationMode(MutationMode::Adaptive);
    config.setMutationArmed(true);
    config.setGrantMemory(true);

    const MutationAuthority authority;
    const MutationPermit scope = authority.authorize(
        plan, config, RuntimeState::Elevated, 1.0, MutationPermit::Scope::Resource);
    ResourceActuator actuator;
    const ActuatorResult result = actuator.apply(mutation, scope);
    CHECK(result.status == ActuatorStatus::SafetyRejected);
    CHECK(readFile(swappiness) == "60");
    fs::remove_all(root);
}

int main() {
    test_resource_actuator_rejects_scope_only_permit();
    test_no_write_without_user_grant();
    test_swappiness_adaptive_path();
    test_retained_resource_state_survives_restart_and_revalidates();
    test_resource_restore_only_owned_paths();
    test_policy_block_without_mutation_is_skipped();
    test_intervention_level_boundaries();
    test_disarmed_policy_performs_no_write();
    test_effect_model_structured_rank_and_unknown_semantics();
    test_block_queue_mutation_targets_are_admitted_for_physical();
    test_selector_value_comparable_form();
    test_selector_scheduler_restore_writes_bare_token();
    test_legacy_bracketed_journal_value_is_normalized();
    test_cpu_permit_requires_governor_opt_in();
    test_thermal_guard_allows_stabilizing_write_and_restores();
    test_pressure_allows_stabilizing_write();
    test_stabilizing_plan_rejects_non_stabilizing_candidates();
    test_safety_state_without_stabilizing_plan_never_writes();
    test_regressed_candidate_is_suppressed();
    std::printf("CoreFlow resource autonomy: %s\n", failures == 0 ? "PASS" : "FAIL");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
