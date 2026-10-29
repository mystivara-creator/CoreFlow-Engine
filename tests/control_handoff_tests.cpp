#include "coreflow/config.hpp"
#include "coreflow/interactivity_boost.hpp"
#include "coreflow/mutation.hpp"
#include "coreflow/resource_mutation.hpp"
#include "coreflow/retained_state.hpp"
#include "coreflow/tunable_discovery.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unordered_set>
using namespace coreflow;
namespace fs = std::filesystem;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while(false)
namespace {
struct Journal final : MutationJournal {
    Entries entries;
    bool fail{false};
    bool corrupt{false};
    bool commit(const Entries& e) noexcept override { if(fail)return false; entries=e; return true; }
    bool clear() noexcept override { if(fail)return false; entries.clear(); return true; }
    LoadState load(Entries& e) noexcept override { e=entries; return corrupt ? LoadState::Corrupt : entries.empty() ? LoadState::Absent : LoadState::Pending; }
};
struct Fixture {
    fs::path root;
    Fixture() { char tmp[]="/tmp/coreflow-handoff-XXXXXX"; if(auto* p=::mkdtemp(tmp))root=p; else std::abort(); }
    ~Fixture() { std::error_code ec; fs::remove_all(root,ec); }
};
void write(const fs::path& p,const std::string& s) { fs::create_directories(p.parent_path()); std::ofstream(p)<<s<<'\n'; }
std::string read(const fs::path& p) { std::string s; std::ifstream f(p); std::getline(f,s);return s; }
EngineConfig config() {
    EngineConfig c; c.setMutationMode(MutationMode::Adaptive);c.setMutationArmed(true);
    c.setGrantCpuFreq(true);c.setAllowCpuGovernor(true);c.setGrantMemory(true);
    c.setInteractivityMode(InteractivityMode::Adaptive);c.setGrantCgroup(true);
    return c;
}
RuntimeSample sample() {
    RuntimeSample s;s.confidence=1.;s.mem_total_kb=1000;s.mem_available_ratio=.5;
    s.cpu_utilization_available=true;s.cpu_utilization=.5;s.load_available=true;s.load1=4.;
    s.thermal_available=true;s.thermal_millidegrees=35000;s.hottest_thermal_millidegrees=35000;
    return s;
}
int journalHandoff() {
    Journal j; j.entries={{"/a","1"},{"/b","2"}};
    j.fail=true;CHECK(!j.releaseExternallyControlledPath("/a"));CHECK(j.entries.size()==2U);
    j.fail=false;CHECK(j.releaseExternallyControlledPath("/a"));CHECK(j.entries==MutationJournal::Entries({{"/b","2"}}));
    CHECK(j.releaseExternallyControlledPath("/a"));CHECK(j.entries.size()==1U);
    j.corrupt=true;CHECK(!j.releaseExternallyControlledPath("/b"));CHECK(j.entries.size()==1U);
    return 0;
}
int cpuHandoff() {
    for (bool retained : {false,true}) {
        Fixture f;Journal j;FileRetainedStateStore store((f.root/"retained").string());
        const auto policy=(f.root/"policy0").string();const auto knob=policy+"/scaling_governor";
        write(knob,"powersave");DeviceProfile p;CpuPolicy cpu;
        cpu.id=0;cpu.path=policy;cpu.governor="powersave";cpu.available_governors={"powersave","schedutil","performance"};
        cpu.readable=true;cpu.governor_writable=true;p.cpu_policies.push_back(cpu);
        std::unordered_set<std::string> blocked;MutationController controller;
        controller.setExternalControlPaths(&blocked);controller.setJournal(&j);controller.setRetainedStateStore(&store);
        const auto c=config();CHECK(controller.captureBaseline(p));
        CHECK(controller.apply(RuntimeState::Normal,sample(),p,c)==MutationResult::Verified);
        if(retained)CHECK(controller.retainLastMutation(p));
        write(knob,"performance");j.fail=true;
        CHECK(!controller.reconcileExternalChanges(blocked));CHECK(blocked.contains(knob));
        CHECK(!controller.restoreAll());CHECK(read(knob)=="performance");CHECK(!j.entries.empty());
        j.fail=false;CHECK(controller.reconcileExternalChanges(blocked));
        CHECK(read(knob)=="performance");CHECK(j.entries.empty());CHECK(!controller.hasPendingOutcome());
        CHECK(controller.restoreAll());CHECK(read(knob)=="performance");
        CHECK(controller.apply(RuntimeState::Normal,sample(),p,c)==MutationResult::Skipped);
        CHECK(c.mutationMode()==MutationMode::Adaptive && c.mutationArmed());
        RetainedStateStore::Entries saved;CHECK(store.load(saved)!=RetainedStateStore::LoadState::Corrupt);CHECK(saved.empty());
    }
    return 0;
}
int resourceHandoff() {
    Fixture f;Journal j;FileRetainedStateStore store((f.root/"retained").string());
    DeviceProfile p;
    for(const char* part : {"first","second"}) {
        const auto knob=(f.root/part/"swappiness").string();write(knob,"60");
        ResourceCapability cap;cap.domain=ResourceDomain::Memory;cap.name="vm.swappiness";cap.path=knob;
        cap.exists=cap.readable=cap.writable=cap.permission_granted=cap.policy_authorized=cap.runtime_verified=cap.mutation_ready=true;
        p.capabilities.resources.push_back(cap);
    }
    TunableDiscovery{}.enrich(p.capabilities,VendorKnowledge{});
    std::unordered_set<std::string> blocked;ResourceMutationController controller;
    controller.setExternalControlPaths(&blocked);controller.setJournal(&j);controller.setRetainedStateStore(&store);
    CHECK(controller.captureBaseline(p));auto s=sample();s.cpu_utilization=.2;
    const auto c=config();
    CHECK(controller.apply(RuntimeState::Elevated,s,p,c)==MutationResult::Verified);
    const auto changed=controller.lastAppliedResources().front().first;
    CHECK(controller.retainLastMutation(p));write(changed,"77");
    j.fail=true;CHECK(!controller.reconcileExternalChanges(blocked));CHECK(!controller.restoreAll());CHECK(read(changed)=="77");
    j.fail=false;CHECK(controller.reconcileExternalChanges(blocked));CHECK(read(changed)=="77");CHECK(j.entries.empty());
    CHECK(controller.apply(RuntimeState::Elevated,s,p,c)==MutationResult::Verified);
    CHECK(controller.lastAppliedResources().front().first!=changed); // other resource remains eligible
    CHECK(read(changed)=="77");CHECK(controller.restoreAll());CHECK(read(changed)=="77");
    CHECK(c.mutationMode()==MutationMode::Adaptive && c.mutationArmed());
    return 0;
}
int boostHandoff() {
    Fixture f;Journal j;InteractivityBoostController boost;std::unordered_set<std::string> blocked;
    const auto knob=(f.root/"uclamp_min").string();write(knob,"5");
    boost.setJournal(&j);boost.setTopAppUclampPath(knob);boost.setExternalControlPaths(&blocked);
    BoostIntent intent;intent.kind=BoostKind::TopAppUclampMin;intent.value=20;intent.window_ms=1000;
    const auto c=config();CHECK(boost.apply({intent},c,100U)==1U);write(knob,"35");
    j.fail=true;CHECK(!boost.reconcileExternalChanges(blocked));boost.restoreAll();CHECK(read(knob)=="35");CHECK(!j.entries.empty());
    j.fail=false;CHECK(boost.reconcileExternalChanges(blocked));CHECK(j.entries.empty());CHECK(boost.activeCount()==0U);
    boost.expire(2000U);boost.restoreAll();CHECK(read(knob)=="35");
    CHECK(boost.apply({intent},c,3000U)==0U);CHECK(read(knob)=="35");
    CHECK(c.mutationMode()==MutationMode::Adaptive && c.mutationArmed());
    return 0;
}
}
int main() { return journalHandoff() || cpuHandoff() || resourceHandoff() || boostHandoff(); }
