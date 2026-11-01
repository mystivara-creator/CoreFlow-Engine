#include "coreflow/trial_history.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
using namespace coreflow;
namespace fs=std::filesystem;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while(false)
RuntimeSample sample() {
    RuntimeSample s; s.thermal_available=true; s.hottest_thermal_millidegrees=40000;
    s.cpu_utilization_available=true; s.cpu_utilization=.8; s.mem_total_kb=1000; s.mem_available_ratio=.5;
    s.psi_available=true; s.psi_cpu_some_avg10=.2; s.charging_telemetry_available=true;
    s.load_available=true; s.load1=2.; return s;
}
int main() {
    char name[]="/tmp/coreflow-trial-history-XXXXXX"; const char* dir=::mkdtemp(name); CHECK(dir);
    const fs::path root=dir;
    struct Cleanup { fs::path root; ~Cleanup() { std::error_code e; fs::remove_all(root,e); } } cleanup{root};
    TrialHistory history; CHECK(history.initialize((root/"history").string(),"device:kernel"));
    history.setForeground("app.a"); auto s=sample(); const auto initial=history.conditions(s,RuntimeState::Normal);
    const std::vector<TrialMember> scalar{{ResourceDomain::Scheduler,"/proc/sys/walt/threshold","100","325"}};
    CHECK(history.remember(scalar,initial,BaselineIntelligence::Outcome::Regression,false,"cohort:test"));
    CHECK(!history.ready() && history.dirty()); CHECK(history.flush());
    CHECK(history.blockReason(scalar[0].path,"100",scalar[0].domain,initial,"325"));
    CHECK(history.blockReason(scalar[0].path,"100",scalar[0].domain,initial,"326"));
    for (unsigned i=0;i<1000U;++i) CHECK(history.blockReason(scalar[0].path,"100",scalar[0].domain,initial,"325"));
    s.cpu_utilization=.79; s.hottest_thermal_millidegrees=40500; s.thermal_trend=Trend::Rising;
    auto jitter=history.conditions(s,RuntimeState::Elevated);
    CHECK(history.blockReason(scalar[0].path,"100",scalar[0].domain,jitter,"325"));
    s.mem_available_ratio=.25; auto unrelated=history.conditions(s,RuntimeState::Normal);
    CHECK(history.blockReason(scalar[0].path,"100",scalar[0].domain,unrelated,"325"));
    s=sample(); s.cpu_utilization=.3;
    CHECK(!history.blockReason(scalar[0].path,"100",scalar[0].domain,history.conditions(s,RuntimeState::Normal),"325"));
    CHECK(!history.blockReason(scalar[0].path,"150",scalar[0].domain,initial,"325")); // external baseline changed
    s=sample(); s.charging=true;
    CHECK(!history.blockReason(scalar[0].path,"100",scalar[0].domain,history.conditions(s,RuntimeState::Normal),"325"));
    s=sample(); s.cpu_utilization_available=false; s.psi_available=false;
    CHECK(history.blockReason(scalar[0].path,"100",scalar[0].domain,history.conditions(s,RuntimeState::Normal),"325"));
    history.setForeground("app.b");
    CHECK(!history.blockReason(scalar[0].path,"100",scalar[0].domain,history.conditions(sample(),RuntimeState::Normal),"325"));
    history.setForeground("app.a");
    CHECK(history.blockReason(scalar[0].path,"100",scalar[0].domain,history.conditions(sample(),RuntimeState::Normal),"325"));
    history.setForeground(""); // an unavailable foreground is not fresh causal evidence
    CHECK(history.blockReason(scalar[0].path,"100",scalar[0].domain,history.conditions(sample(),RuntimeState::Normal),"325"));
    auto quiet=initial; quiet.values[1]=.1; quiet.values[9]=.1;
    unsigned confirmations=0;
    CHECK(!TrialHistory::updateQuietRelease(initial,quiet,false,confirmations));
    CHECK(!TrialHistory::updateQuietRelease(initial,quiet,false,confirmations));
    CHECK(TrialHistory::updateQuietRelease(initial,quiet,false,confirmations));
    CHECK(!TrialHistory::updateQuietRelease(initial,quiet,true,confirmations) && confirmations==0U);
    CHECK(!TrialHistory::updateQuietRelease(initial,initial,false,confirmations) && confirmations==0U);
    auto display_before=initial, display_after=initial;
    display_before.frame_period_ns=16666667; display_after.frame_period_ns=8333333;
    CHECK(TrialHistory::materiallyChanged(display_before,display_after,ResourceDomain::Scheduler));
    CHECK(!TrialHistory::materiallyChanged(display_before,display_after,ResourceDomain::Memory));
    TrialHistory restarted; CHECK(restarted.initialize((root/"history").string(),"device:kernel"));
    CHECK(restarted.size()==1U && restarted.blockReason(scalar[0].path,"100",scalar[0].domain,initial,"325"));
    struct stat st{}; CHECK(::stat((root/"history").c_str(),&st)==0 && (st.st_mode&0777)==0600);
    const std::vector<TrialMember> governor{{ResourceDomain::CpuFreq,"/sys/cpufreq/policy0/scaling_governor","powersave","walt"}};
    CHECK(restarted.remember(governor,initial,BaselineIntelligence::Outcome::Inconclusive,false,"cohort:uncertain")); CHECK(restarted.flush());
    CHECK(restarted.blockReason(governor[0].path,"powersave",ResourceDomain::CpuFreq,initial,"walt"));
    CHECK(!restarted.blockReason(governor[0].path,"powersave",ResourceDomain::CpuFreq,initial,"schedutil"));
    const std::vector<TrialMember> other_governor{{ResourceDomain::CpuFreq,governor[0].path,"powersave","schedutil"}};
    CHECK(restarted.remember(other_governor,initial,BaselineIntelligence::Outcome::Regression,false,"cohort:other")); CHECK(restarted.flush());
    CHECK(restarted.blockReason(governor[0].path,"powersave",ResourceDomain::CpuFreq,initial,"walt"));
    CHECK(restarted.blockReason(governor[0].path,"powersave",ResourceDomain::CpuFreq,initial,"schedutil"));
    const std::vector<TrialMember> accepted{{ResourceDomain::UClamp,"/dev/cpuctl/top-app/cpu.uclamp.min","0.00","10.00"}};
    CHECK(restarted.remember(accepted,initial,BaselineIntelligence::Outcome::Beneficial,true,"cohort:accepted")); CHECK(restarted.flush());
    CHECK(restarted.blockReason(accepted[0].path,"10.00",ResourceDomain::UClamp,initial,"12.00"));
    CHECK(!restarted.blockReason(accepted[0].path,"0.00",ResourceDomain::UClamp,initial,"10.00"));
    s=sample(); s.hottest_thermal_millidegrees=56000;
    CHECK(!restarted.blockReason(accepted[0].path,"10.00",ResourceDomain::UClamp,restarted.conditions(s,RuntimeState::ThermalGuard),"0.00"));
    TrialHistory new_kernel; CHECK(new_kernel.initialize((root/"history").string(),"device:new-kernel")); CHECK(new_kernel.size()==0U);
    TrialHistory io; CHECK(io.initialize((root/"missing/history").string(),"scope"));
    CHECK(io.remember(scalar,initial,BaselineIntelligence::Outcome::Neutral,false,"cohort:neutral"));
    CHECK(!io.flush() && !io.ready());
    CHECK(io.blockReason("/another/path","1",ResourceDomain::Memory,initial,"2"));
    fs::create_directories(root/"missing"); CHECK(io.flush() && io.ready());
    TrialHistory budget; CHECK(budget.initialize((root/"budget").string(),"scope"));
    for (unsigned i=0;i<124U;++i) {
        const std::vector<TrialMember> member{{ResourceDomain::Memory,"/node/"+std::to_string(i),"100","110"}};
        CHECK(budget.remember(member,initial,BaselineIntelligence::Outcome::Inconclusive,false,"cohort:budget"));
    }
    CHECK(budget.flush()); CHECK(budget.blockReason("/new/node","100",ResourceDomain::Memory,initial,"110"));
    auto changed=initial; changed.values[2]=.2;
    CHECK(!budget.blockReason("/new/node","100",ResourceDomain::Memory,changed,"110"));
    { std::ofstream file(root/"budget",std::ios::app); file<<"unexpected trailing data\n"; }
    TrialHistory corrupt; CHECK(!corrupt.initialize((root/"budget").string(),"scope"));
    CHECK(corrupt.blockReason("/node/1","100",ResourceDomain::Memory,initial,"110"));
    std::puts("CoreFlow condition-aware trial memory: PASS"); return 0;
}
