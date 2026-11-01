#include "coreflow/coordinated_plan.hpp"
#include "coreflow/context.hpp"
#include "coreflow/tunable_discovery.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
using namespace coreflow;
namespace fs = std::filesystem;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n", __LINE__, #x); return 1; } } while(false)
namespace {
struct Journal final : MutationJournal {
    Entries entries;
    unsigned commits{0}, fail_on{0};
    bool fail_clear{false};
    bool commit(const Entries& value) noexcept override {
        ++commits;
        if (commits == fail_on) return false;
        entries = value; return true;
    }
    bool clear() noexcept override { if (fail_clear) return false; entries.clear(); return true; }
    LoadState load(Entries& value) noexcept override {
        value = entries; return value.empty() ? LoadState::Absent : LoadState::Pending;
    }
};
struct Fixture {
    fs::path root;
    Journal cpu_journal, resource_journal;
    DeviceProfile profile;
    MutationController cpu;
    ResourceMutationController resources;
    Fixture() {
        char name[] = "/tmp/coreflow-cohort-XXXXXX";
        if (const auto* p = ::mkdtemp(name)) root = p; else std::abort();
        cpu.setJournal(&cpu_journal); resources.setJournal(&resource_journal);
    }
    ~Fixture() { std::error_code ec; fs::remove_all(root, ec); }
    void write(const fs::path& path, const std::string& text) {
        fs::create_directories(path.parent_path()); std::ofstream(path) << text << '\n';
    }
    std::string read(const fs::path& path) { std::string value; std::ifstream file(path); std::getline(file,value); return value; }
    std::string governor(const std::string& initial = "powersave") {
        const auto path = (root/"cpufreq/policy0").string();
        write(path+"/scaling_governor", initial);
        CpuPolicy policy;
        policy.id = 0; policy.path = path; policy.governor = initial;
        policy.available_governors = {"powersave", "performance", "walt"};
        policy.readable = policy.governor_writable = true;
        profile.cpu_policies.push_back(policy);
        return path;
    }
    std::string add(ResourceDomain domain, const std::string& name,
                    const std::string& relative, const std::string& initial,
                    std::vector<std::string> options = {}) {
        const auto path = (root/relative).string(); write(path,initial);
        ResourceCapability cap;
        cap.domain = domain; cap.name = name; cap.path = path;
        cap.exists = cap.readable = cap.writable = cap.permission_granted =
            cap.runtime_verified = cap.policy_authorized = cap.mutation_ready = true;
        cap.options = std::move(options); cap.ordered_scale = !cap.options.empty();
        profile.capabilities.resources.push_back(std::move(cap));
        return path;
    }
};
EngineConfig config() {
    EngineConfig c; c.setMutationMode(MutationMode::Adaptive); c.setMutationArmed(true);
    c.setAllowCpuGovernor(true); c.setGrantCpuFreq(true); c.setGrantScheduler(true);
    c.setGrantCgroup(true); c.setGrantGpu(true); c.setGrantMemory(true); c.setGrantBlockQueue(true);
    return c;
}
RuntimeSample sample(bool hot) {
    RuntimeSample s;
    s.thermal_available = true; s.thermal_millidegrees = hot ? 56000 : 35000;
    s.hottest_thermal_millidegrees = s.thermal_millidegrees;
    s.confidence = 1.; s.cpu_utilization_available = true; s.cpu_utilization = .8;
    s.load_available = true; s.load1 = 2.; s.mem_total_kb = 100000;
    s.mem_available_ratio = .5; s.charging_telemetry_available = true; s.battery_level_percent = 80;
    s.gpu_busy_available = true; s.gpu_busy_ratio = .7;
    return s;
}
PolicyPlan plan(const DeviceProfile& p, const RuntimeSample& s, RuntimeState state) {
    ResourceStateModel model; model.reset(p.capabilities);
    return PolicyEngine{}.evaluate(ContextEngine{}.evaluate(s,state), model);
}
void thermalNodes(Fixture& f, std::vector<std::string>& paths) {
    paths.push_back(f.add(ResourceDomain::Scheduler,"cpufreq.governor.hispeed_freq",
        "cpufreq/policy0/walt/hispeed_freq","1200",{"400","800","1200"}));
    paths.push_back(f.add(ResourceDomain::Gpu,"gpu.minimum",
        "devfreq/gpu/min_freq","1200",{"400","800","1200"}));
    // A real cgroup exposes paired uclamp controls. Live sibling checks
    // intentionally fail closed when the fixture omits the other half.
    f.write(f.root/"dev/cpuctl/background/cpu.uclamp.min","0");
    paths.push_back(f.add(ResourceDomain::UClamp,"background:cpu.uclamp.max",
        "dev/cpuctl/background/cpu.uclamp.max","max"));
}
int thermalCohort() {
    Fixture f; const auto cpu = f.governor("walt");
    std::vector<std::string> paths; thermalNodes(f,paths);
    CHECK(f.cpu.captureBaseline(f.profile)); CHECK(f.resources.captureBaseline(f.profile));
    const auto s = sample(true); const auto p = plan(f.profile,s,RuntimeState::ThermalGuard);
    CHECK(p.stabilizing_only && p.mutation_eligible);
    const auto result = executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::ThermalGuard,s,f.profile,config(),p);
    CHECK(result.resource == MutationResult::Verified && result.cpu == MutationResult::Skipped);
    CHECK(result.members == 3U && result.identity.starts_with("cohort:"));
    CHECK(f.read(paths[0]) == "800" && f.read(paths[1]) == "800");
    CHECK(f.read(paths[2]) != "max" && f.read(cpu+"/scaling_governor") == "walt");
    CHECK(f.resources.stabilizingEpoch() && f.resource_journal.entries.size() == 3U);
    const auto before = f.read(paths[0]);
    CHECK(f.resources.apply(RuntimeState::ThermalGuard,s,f.profile,config(),p,
        MutationAuthority{}.authorize(p,config(),RuntimeState::ThermalGuard,1.,MutationPermit::Scope::Resource)) == MutationResult::Skipped);
    CHECK(f.read(paths[0]) == before); // own journal is not a crash recovery
    CHECK(!f.resources.captureBaseline(f.profile)); CHECK(f.read(paths[0]) == before);
    CHECK(f.resources.acceptCoordinatedMutation());
    CHECK(!f.resources.hasPendingOutcome() && f.resources.activeResourceSettings().size() == 3U);
    EpochRestoreInputs in;
    in.state = RuntimeState::ThermalGuard; in.mode_adaptive = in.armed = true;
    in.resource_epoch_stabilizing = true; // confidence dip blocks new writes, not cooling retention
    CHECK(!shouldRestoreEpochNow(in));
    in.armed = false; CHECK(shouldRestoreEpochNow(in));
    for (unsigned i=0; i<10U; ++i) {
        const auto next=executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::ThermalGuard,s,f.profile,config(),p);
        if (!next.verified()) break;
        CHECK(f.resources.acceptCoordinatedMutation());
    }
    CHECK(std::stod(f.read(paths[2])) >= 50.); // factory-relative budget, no runaway capping
    CHECK(f.resources.restoreAll());
    CHECK(f.read(paths[0]) == "1200" && f.read(paths[1]) == "1200" && f.read(paths[2]) == "max");
    return 0;
}
int failedCompanionRollsBack() {
    Fixture f; f.governor("walt"); std::vector<std::string> paths; thermalNodes(f,paths);
    CHECK(f.cpu.captureBaseline(f.profile)); CHECK(f.resources.captureBaseline(f.profile));
    f.resource_journal.fail_on = 2U;
    const auto s = sample(true); const auto p = plan(f.profile,s,RuntimeState::ThermalGuard);
    const auto result = executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::ThermalGuard,s,f.profile,config(),p);
    CHECK(!result.verified()); CHECK(!f.resources.hasPendingOutcome());
    CHECK(f.read(paths[0]) == "1200" && f.read(paths[1]) == "1200" && f.read(paths[2]) == "max");
    CHECK(f.resource_journal.entries.empty());
    return 0;
}
int governorAndDiscoveredTunables() {
    Fixture f; const auto cpu = f.governor();
    f.write(cpu+"/walt/up_rate_limit_us","2000");
    f.write(cpu+"/walt/down_rate_limit_us","4000");
    CHECK(f.cpu.captureBaseline(f.profile));
    const auto s = sample(false);
    PolicyPlan p; p.action = PolicyAction::Candidate; p.intervention = InterventionLevel::Moderate;
    p.mutation_eligible = true; p.candidates.push_back({ResourceDomain::CpuFreq,"governor",{},1.,.2,PolicyAction::Candidate});
    const auto result = executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Elevated,s,f.profile,config(),p);
    CHECK(result.cpu == MutationResult::Verified); CHECK(f.read(cpu+"/scaling_governor") == "walt");
    CHECK(f.profile.capabilities.resources.size() == 2U);
    CHECK(result.resource == MutationResult::Verified && result.members >= 2U);
    CHECK(f.resources.lastAppliedResources().size() <= ResourceMutationController::kMaxCohortWrites);
    CHECK(f.resources.rollbackLastMutation()); CHECK(f.cpu.rollbackLastMutation());
    CHECK(f.read(cpu+"/scaling_governor") == "powersave");
    CHECK(f.read(cpu+"/walt/up_rate_limit_us") == "2000" && f.read(cpu+"/walt/down_rate_limit_us") == "4000");
    return 0;
}
int dependentRollbackFailurePreservesGovernor() {
    Fixture f; const auto cpu = f.governor();
    f.write(cpu+"/walt/up_rate_limit_us","2000"); f.write(cpu+"/walt/down_rate_limit_us","4000");
    CHECK(f.cpu.captureBaseline(f.profile)); const auto s=sample(false);
    PolicyPlan p; p.action=PolicyAction::Candidate; p.intervention=InterventionLevel::Moderate; p.mutation_eligible=true;
    const auto result=executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Elevated,s,f.profile,config(),p);
    CHECK(result.cpu==MutationResult::Verified && result.resource==MutationResult::Verified);
    f.resource_journal.fail_clear=true;
    CHECK(!rollbackCoordinatedPlan(f.cpu,f.resources));
    CHECK(f.read(cpu+"/scaling_governor")=="walt");
    CHECK(f.cpu.hasPendingOutcome() && !f.resource_journal.entries.empty());
    f.resource_journal.fail_clear=false;
    CHECK(rollbackCoordinatedPlan(f.cpu,f.resources));
    CHECK(f.read(cpu+"/scaling_governor")=="powersave");
    return 0;
}
int externalMemberWithdrawn() {
    Fixture f; std::vector<std::string> paths; thermalNodes(f,paths);
    CHECK(f.resources.captureBaseline(f.profile)); const auto s=sample(true);
    const auto p=plan(f.profile,s,RuntimeState::ThermalGuard);
    CHECK(executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::ThermalGuard,s,f.profile,config(),p).members==3U);
    f.write(paths[0],"700");
    std::unordered_set<std::string> external;
    CHECK(f.resources.reconcileExternalChanges(external)); CHECK(external.contains(paths[0]));
    CHECK(f.resources.rollbackLastMutation());
    CHECK(f.read(paths[0])=="700" && f.read(paths[1])=="1200" && f.read(paths[2])=="max");
    return 0;
}
int ioSchedulerCompanions() {
    Fixture f; const auto cpu=f.governor("walt");
    const auto scheduler=f.add(ResourceDomain::Io,"disk:scheduler","block/disk/queue/scheduler","[cfq] mq-deadline",{"cfq","mq-deadline"});
    f.profile.capabilities.resources.back().ordered_scale=false;
    f.profile.capabilities.resources.back().vendor_values.push_back({"mq-deadline",VendorProfile::Default,"firmware"});
    const auto requests=f.add(ResourceDomain::Io,"disk:nr_requests","block/disk/queue/nr_requests","128");
    f.write(f.root/"block/disk/queue/iosched/fifo_batch","16");
    CHECK(f.cpu.captureBaseline(f.profile)); CHECK(f.resources.captureBaseline(f.profile));
    auto s=sample(false); s.cpu_utilization=.1; s.load1=.1; s.gpu_busy_available=false;
    s.psi_available=true; s.psi_io_some_avg10=.3; s.io_activity_available=true;
    const auto p=plan(f.profile,s,RuntimeState::Elevated);
    const auto result=executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Elevated,s,f.profile,config(),p);
    CHECK(result.resource==MutationResult::Verified && result.cpu==MutationResult::Skipped);
    CHECK(f.read(scheduler)=="mq-deadline");
    CHECK(result.members>=2U && f.resources.activeResourceSettings().size()==result.members);
    CHECK(f.read(requests)!="128" || f.read(f.root/"block/disk/queue/iosched/fifo_batch")!="16");
    CHECK(f.read(cpu+"/scaling_governor")=="walt");
    CHECK(f.resources.rollbackLastMutation()); CHECK(f.read(scheduler)=="cfq");
    CHECK(f.read(requests)=="128" && f.read(f.root/"block/disk/queue/iosched/fifo_batch")=="16");
    return 0;
}
int incompleteCrashRecoveryKeepsJournal() {
    Fixture f; std::vector<std::string> paths; thermalNodes(f,paths);
    CHECK(f.resources.captureBaseline(f.profile)); const auto s=sample(true);
    const auto p=plan(f.profile,s,RuntimeState::ThermalGuard);
    CHECK(executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::ThermalGuard,s,f.profile,config(),p).members==3U);
    const auto absent=f.resource_journal.entries.back();
    fs::remove(absent.first);
    ResourceMutationController restarted; restarted.setJournal(&f.resource_journal);
    CHECK(!restarted.captureBaseline(f.profile));
    CHECK(!restarted.restoreAll()); CHECK(!f.resource_journal.entries.empty());
    f.write(absent.first,absent.second);
    CHECK(restarted.restoreAll()); CHECK(f.resource_journal.entries.empty());
    return 0;
}
int crashRestoresWholeCohort() {
    Fixture f; std::vector<std::string> paths; thermalNodes(f,paths);
    CHECK(f.resources.captureBaseline(f.profile));
    const auto s = sample(true); const auto p = plan(f.profile,s,RuntimeState::ThermalGuard);
    const auto result = executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::ThermalGuard,s,f.profile,config(),p);
    CHECK(result.resource == MutationResult::Verified);
    CHECK(f.resources.acceptCoordinatedMutation());
    ResourceMutationController restarted; restarted.setJournal(&f.resource_journal);
    CHECK(restarted.captureBaseline(f.profile));
    CHECK(f.read(paths[0]) == "1200" && f.read(paths[1]) == "1200" && f.read(paths[2]) == "max");
    CHECK(f.resource_journal.entries.empty());
    return 0;
}
int conditionDrivenSelection() {
    for (unsigned scenario=0; scenario<5U; ++scenario) {
        Fixture f; const auto cpu=f.governor();
        const auto scheduler=f.add(ResourceDomain::Io,"disk:scheduler","block/disk/queue/scheduler",
            "[cfq] mq-deadline",{"cfq","mq-deadline"});
        f.profile.capabilities.resources.back().ordered_scale=false;
        f.profile.capabilities.resources.back().vendor_values.push_back({"mq-deadline",VendorProfile::Default,"firmware"});
        CHECK(f.cpu.captureBaseline(f.profile)); CHECK(f.resources.captureBaseline(f.profile));
        auto s=sample(false); s.cpu_utilization=(scenario==0U || scenario==2U || scenario==4U) ? .8 : .1;
        s.load1=.1; s.gpu_busy_available=false; s.psi_available=true;
        s.psi_io_some_avg10=(scenario==1U || scenario==2U || scenario==4U) ? .3 : 0.; s.io_activity_available=true;
        auto c=config(); if (scenario==4U) c.setGrantCpuFreq(false);
        const auto p=plan(f.profile,s,RuntimeState::Elevated);
        const auto cpu_option=f.cpu.preview(RuntimeState::Elevated,s,f.profile,c,p);
        const auto io_option=f.resources.preview(RuntimeState::Elevated,s,f.profile,c,p);
        CHECK(f.cpu_journal.commits==0U && f.resource_journal.commits==0U);
        CHECK(f.read(cpu+"/scaling_governor")=="powersave" && f.read(scheduler)=="[cfq] mq-deadline");
        CHECK(cpu_option.valid==(scenario==0U || scenario==2U));
        CHECK(io_option.valid==(scenario==1U || scenario==2U || scenario==4U));
        const auto r=executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Elevated,s,f.profile,c,p);
        if (scenario==3U) {
            CHECK(!r.verified() && r.winner=="KEEP_CURRENT");
            CHECK(r.decision_reason=="no_measured_tuning_need_keep_current");
            CHECK(f.cpu_journal.commits==0U && f.resource_journal.commits==0U);
            continue;
        }
        CHECK(r.verified());
        CHECK((r.cpu==MutationResult::Verified)==cpu_option.valid);
        CHECK((r.resource==MutationResult::Verified)==io_option.valid);
        if (scenario==0U) CHECK(r.winner=="CPU_GOVERNOR");
        if (scenario==1U || scenario==4U) CHECK(r.winner=="IO");
        if (scenario==2U) {
            CHECK(r.members==2U);
            CHECK(r.winner==(cpu_option.utility>io_option.utility ? "CPU_GOVERNOR" : "IO"));
            CHECK(f.cpu.acceptCoordinatedMutation()); CHECK(f.resources.acceptCoordinatedMutation());
            EpochRestoreInputs idle;
            idle.mode_adaptive=idle.armed=true; idle.state=RuntimeState::Idle;
            idle.pending_outcome=false;
            CHECK(!shouldRestoreEpochNow(idle));
            idle.armed=false; CHECK(shouldRestoreEpochNow(idle)); idle.armed=true;
            idle.pending_outcome=true; CHECK(shouldRestoreEpochNow(idle));
            const auto held=f.cpu.apply(RuntimeState::Idle,s,f.profile,config(),p,MutationAuthority{}.authorize(p,config(),RuntimeState::Idle,1.,MutationPermit::Scope::CpuFreq));
            CHECK(held==MutationResult::Skipped && f.read(cpu+"/scaling_governor")=="walt");
            f.add(ResourceDomain::Io,"disk:nr_requests","block/disk/queue/nr_requests","128");
            CHECK(f.resources.admitNewBaselines(f.profile));
            s.cpu_utilization=.1;
            const auto next_plan=plan(f.profile,s,RuntimeState::Elevated);
            const auto next=executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Elevated,s,f.profile,config(),next_plan);
            CHECK(next.resource==MutationResult::Verified && next.cpu==MutationResult::Skipped);
            CHECK(next.members==f.resources.lastAppliedResources().size());
            CHECK(next.identity.find("cpufreq:")==std::string::npos); // no credit to an earlier accepted governor

        }
        CHECK(restoreCoordinatedPlan(f.cpu,f.resources));
    }
    return 0;
}
int needWithoutAuthorityOrUsefulSurface() {
    Fixture f;
    f.add(ResourceDomain::Memory,"unclassified","proc/sys/vm/unclassified","10");
    CHECK(f.resources.captureBaseline(f.profile));
    auto s=sample(false); s.cpu_utilization=.1; s.load1=.1; s.gpu_busy_available=false;
    s.mem_available_ratio=.1;
    PolicyPlan p; p.action=PolicyAction::Candidate; p.intervention=InterventionLevel::Moderate; p.mutation_eligible=true; p.stabilizing_only=true;
    p.candidates.push_back({ResourceDomain::Memory,"unclassified",{},1.,.2,PolicyAction::Candidate});
    const auto r=executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Pressure,s,f.profile,config(),p);
    CHECK(!r.verified() && r.decision_reason=="measured_need_but_no_admissible_gain");
    CHECK(r.funnel.assessed && r.funnel.discovered==1U);
    CHECK(f.resource_journal.commits==0U);
    return 0;
}
int stalePreviewsCannotOverwrite() {
    Fixture f; const auto cpu=f.governor();
    const auto scheduler=f.add(ResourceDomain::Io,"disk:scheduler","block/disk/queue/scheduler",
        "[cfq] mq-deadline",{"cfq","mq-deadline"});
    f.profile.capabilities.resources.back().ordered_scale=false;
    f.profile.capabilities.resources.back().vendor_values.push_back({"mq-deadline",VendorProfile::Default,"firmware"});
    CHECK(f.cpu.captureBaseline(f.profile)); CHECK(f.resources.captureBaseline(f.profile));
    auto s=sample(false); s.psi_available=true; s.psi_io_some_avg10=.3;
    const auto p=plan(f.profile,s,RuntimeState::Elevated); const auto c=config();
    const auto a=f.cpu.preview(RuntimeState::Elevated,s,f.profile,c,p);
    const auto b=f.resources.preview(RuntimeState::Elevated,s,f.profile,c,p);
    CHECK(a.valid && b.valid);
    f.write(cpu+"/scaling_governor","performance"); f.write(scheduler,"none");
    const MutationAuthority authority;
    CHECK(f.cpu.applySelected(a,RuntimeState::Elevated,s,f.profile,c,p,
        authority.authorize(p,c,RuntimeState::Elevated,1.,MutationPermit::Scope::CpuFreq))==MutationResult::Skipped);
    CHECK(f.resources.applySelected(b,RuntimeState::Elevated,s,f.profile,c,p,
        authority.authorize(p,c,RuntimeState::Elevated,1.,MutationPermit::Scope::Resource))==MutationResult::Skipped);
    CHECK(f.cpu_journal.commits==0U && f.resource_journal.commits==0U);
    CHECK(f.read(cpu+"/scaling_governor")=="performance" && f.read(scheduler)=="none");
    return 0;
}

int cpuReviewExplainsNoWrite() {
    Fixture f; const auto policy=f.governor("walt");
    f.profile.cpu_policies[0].available_governors={"walt","schedutil"};
    CHECK(f.cpu.captureBaseline(f.profile));
    auto s=sample(false); PolicyPlan p; p.action=PolicyAction::Candidate; p.mutation_eligible=true;
    p.intervention=InterventionLevel::Moderate;
    const auto review=f.cpu.preview(RuntimeState::Normal,s,f.profile,config(),p);
    CHECK(!review.valid && review.reason=="no_higher_scoring_governor_than_current");
    CHECK(f.read(policy+"/scaling_governor")=="walt" && f.cpu_journal.commits==0U);
    s.cpu_utilization=.2;
    const auto calm=f.cpu.preview(RuntimeState::Normal,s,f.profile,config(),p);
    CHECK(!calm.valid && calm.reason=="no_measured_cpu_or_render_demand");
    s.cpu_utilization=.8; f.profile.cpu_policies[0].available_governors={"walt"};
    CHECK(f.cpu.preview(RuntimeState::Normal,s,f.profile,config(),p).reason=="no_advertised_governor_alternative");
    return 0;
}

int contextualHistoryPreventsRepeatAndDrift() {
    Fixture f; f.governor("walt");
    f.write(f.root/"dev/cpuctl/top-app/cpu.uclamp.max","max");
    const auto clamp=f.add(ResourceDomain::UClamp,"top-app:cpu.uclamp.min","dev/cpuctl/top-app/cpu.uclamp.min","0.00");
    TrialHistory history; CHECK(history.initialize((f.root/"trial-history").string(),"device:kernel"));
    f.cpu.setTrialHistory(&history); f.resources.setTrialHistory(&history);
    CHECK(f.cpu.captureBaseline(f.profile)); CHECK(f.resources.captureBaseline(f.profile));
    auto s=sample(false); const auto p=plan(f.profile,s,RuntimeState::Elevated);
    const auto first=executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Elevated,s,f.profile,config(),p);
    CHECK(first.resource==MutationResult::Verified && first.trials.size()==1U);
    CHECK(first.trials[0].previous=="0.00" && first.trials[0].path==clamp);
    CHECK(history.remember(first.trials,history.conditions(s,RuntimeState::Elevated),BaselineIntelligence::Outcome::Neutral,false,first.identity));
    CHECK(history.flush()); CHECK(rollbackCoordinatedPlan(f.cpu,f.resources));
    const auto commits=f.resource_journal.commits;
    for (unsigned i=0;i<250U;++i) {
        const auto retry=executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Elevated,s,f.profile,config(),p);
        CHECK(!retry.verified() && retry.funnel.history_held>0U);
    }
    CHECK(f.resource_journal.commits==commits && f.read(clamp)=="0.00");
    s.charging=true; // a relevant power/workload condition changed, not an expired timer
    const auto next=executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Elevated,s,f.profile,config(),p);
    CHECK(next.resource==MutationResult::Verified);
    CHECK(history.remember(next.trials,history.conditions(s,RuntimeState::Elevated),BaselineIntelligence::Outcome::Beneficial,true,next.identity));
    CHECK(history.flush()); CHECK(f.resources.acceptCoordinatedMutation());
    const auto accepted=f.read(clamp); const auto held_commits=f.resource_journal.commits;
    for (unsigned i=0;i<150U;++i) CHECK(!executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Elevated,s,f.profile,config(),p).verified());
    CHECK(f.read(clamp)==accepted && f.resource_journal.commits==held_commits);
    CHECK(restoreCoordinatedPlan(f.cpu,f.resources)); return 0;
}
int distinctGovernorMayCompeteAfterFailedTrial() {
    Fixture f; const auto policy=f.governor(); f.profile.cpu_policies[0].available_governors={"powersave","walt","schedutil"};
    TrialHistory history; CHECK(history.initialize((f.root/"trial-history").string(),"device:kernel"));
    f.cpu.setTrialHistory(&history); f.resources.setTrialHistory(&history);
    CHECK(f.cpu.captureBaseline(f.profile)); auto s=sample(false);
    PolicyPlan p; p.action=PolicyAction::Candidate; p.intervention=InterventionLevel::Moderate; p.mutation_eligible=true;
    const auto first=executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Elevated,s,f.profile,config(),p);
    CHECK(first.cpu==MutationResult::Verified && first.trials.size()==1U);
    CHECK(f.read(policy+"/scaling_governor")=="walt");
    CHECK(history.remember(first.trials,history.conditions(s,RuntimeState::Elevated),BaselineIntelligence::Outcome::Regression,false,first.identity));
    CHECK(history.flush()); CHECK(rollbackCoordinatedPlan(f.cpu,f.resources));
    const auto next=executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Elevated,s,f.profile,config(),p);
    CHECK(next.cpu==MutationResult::Verified && f.read(policy+"/scaling_governor")=="schedutil");
    CHECK(history.remember(next.trials,history.conditions(s,RuntimeState::Elevated),BaselineIntelligence::Outcome::Regression,false,next.identity));
    CHECK(history.flush()); CHECK(rollbackCoordinatedPlan(f.cpu,f.resources));
    const auto commits=f.cpu_journal.commits;
    for (unsigned i=0;i<250U;++i) CHECK(!executeCoordinatedPlan(f.cpu,f.resources,RuntimeState::Elevated,s,f.profile,config(),p).verified());
    CHECK(f.cpu_journal.commits==commits && f.read(policy+"/scaling_governor")=="powersave");
    CHECK(restoreCoordinatedPlan(f.cpu,f.resources)); return 0;
}

}
int main() {
    const int result=contextualHistoryPreventsRepeatAndDrift() || distinctGovernorMayCompeteAfterFailedTrial() || cpuReviewExplainsNoWrite() || conditionDrivenSelection() || needWithoutAuthorityOrUsefulSurface() || stalePreviewsCannotOverwrite() || thermalCohort() || failedCompanionRollsBack() || governorAndDiscoveredTunables() ||
        dependentRollbackFailurePreservesGovernor() || externalMemberWithdrawn() || ioSchedulerCompanions() || incompleteCrashRecoveryKeepsJournal() || crashRestoresWholeCohort();
    if (!result) std::puts("CoreFlow coordinated and thermal plans: PASS");
    return result;
}
