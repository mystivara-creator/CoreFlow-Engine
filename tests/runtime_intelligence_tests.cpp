#include "coreflow/runtime_intelligence.hpp"
#include "coreflow/presentation_window.hpp"
#include "coreflow/baseline_intelligence.hpp"
#include "coreflow/interactivity_model.hpp"
#include "coreflow/outcome_diagnostics.hpp"
#include "coreflow/effect_model.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while(false)
using namespace coreflow;
namespace {
RuntimeSample measured() {
    RuntimeSample s;
    s.confidence=.9; s.thermal_available=true;
    s.thermal_millidegrees=40000; s.hottest_thermal_millidegrees=40000;
    s.mem_total_kb=1000; s.mem_available_ratio=.5;
    s.cpu_utilization_available=true; s.cpu_utilization=.5;
    s.load_available=true; s.load1=1.;
    s.frame_jank_available=true; s.frame_jank_rate=.01; s.frame_period_ns=16666667;
    return s;
}
struct Directory {
    std::filesystem::path path;
    Directory() { char temp[]="/tmp/coreflow-intelligence-XXXXXX"; if (auto* p=::mkdtemp(temp)) path=p; }
    ~Directory() { if (!path.empty()) { std::error_code ec; std::filesystem::remove_all(path,ec); } }
};
}
int main() {
    const auto evaluate=[](const RuntimeSample& before,const RuntimeSample& after) {
        BaselineIntelligence b;
        for (std::size_t i=0;i<BaselineIntelligence::kBaselineSamples;++i) (void)b.captureBaseline(before);
        (void)b.beginObservation();
        for (std::size_t i=0;i<BaselineIntelligence::kObservationSamples;++i) (void)b.observe(after);
        return b.result();
    };
    {
        auto before=measured(), after=before;
        before.psi_available=after.psi_available=true;
        before.io_latency_available=after.io_latency_available=true;
        after.psi_io_some_avg10=.30; after.io_avg_latency_ms=50.;
        const auto e=evaluate(before,after);
        CHECK(e.valid && e.outcome==BaselineIntelligence::Outcome::Regression);
        CHECK(e.psi_score<.5 && e.io_latency_score<.5);
        CHECK(e.metrics[4].before==0. && e.metrics[4].after==.30);
        CHECK(e.metrics[5].before_samples==4U && e.metrics[5].after_samples==4U);
        CHECK(outcomeEvaluationJson(e).find("weighted_metrics_regressed")!=std::string::npos);
        after=before; after.cpu_utilization+=.005; after.load1+=.05;
        const auto noisy=evaluate(before,after);
        CHECK(noisy.valid && noisy.cpu_score==.5 && noisy.load_score==.5);
        CHECK(noisy.outcome==BaselineIntelligence::Outcome::Neutral);
        before.charging_telemetry_available=after.charging_telemetry_available=true;
        after.charging=true;
        const auto changed=evaluate(before,after);
        CHECK(!changed.valid && !changed.context_compatible);
        CHECK(changed.outcome==BaselineIntelligence::Outcome::Inconclusive);
        CHECK(changed.reason==BaselineIntelligence::EvaluationReason::ContextChanged);
        BaselineIntelligence sparse;
        for (unsigned i=0;i<4U;++i) {
            RuntimeSample sample; sample.psi_available=sample.io_latency_available=i==0U;
            (void)sparse.captureBaseline(sample);
        }
        CHECK(sparse.beginObservation());
        for (unsigned i=0;i<4U;++i) {
            RuntimeSample sample; sample.psi_available=sample.io_latency_available=i==0U;
            sample.psi_io_some_avg10=.5; sample.io_avg_latency_ms=100.;
            (void)sparse.observe(sample);
        }
        CHECK(!sparse.result().valid && sparse.result().comparable_metrics==0U);
        CHECK(sparse.result().outcome==BaselineIntelligence::Outcome::Inconclusive);
        CHECK(outcomeEvaluationJson(sparse.result()).find("\"score\":null")!=std::string::npos);
    }
    {
        ResourceCapability cap; cap.domain=ResourceDomain::Scheduler;
        cap.name="walt.walt_low_latency_task_threshold"; cap.path="/proc/sys/walt/walt_low_latency_task_threshold";
        cap.exists=cap.readable=cap.writable=cap.permission_granted=cap.policy_authorized=cap.runtime_verified=cap.mutation_ready=true;
        cap.vendor_values.push_back({"325",VendorProfile::Performance,"fixture:powerhint.json"});
        EffectContext ctx; ctx.state=RuntimeState::Normal; ctx.intervention=InterventionLevel::Low;
        ctx.sample_confidence=1.; ctx.cpu_utilization_available=true; ctx.cpu_utilization=.8;
        ctx.thermal_available=ctx.memory_available=true; ctx.thermal_c=35.; ctx.mem_available_ratio=.5;
        CHECK(EffectModel{}.proposals(cap,"100",ctx).empty());
        cap.name="cpufreq.governor.up_rate_limit_us";
        cap.path="/sys/devices/system/cpu/cpufreq/policy0/schedutil/up_rate_limit_us";
        const auto options=EffectModel{}.proposals(cap,"100",ctx);
        CHECK(!options.empty());
        CHECK(options[0].source==CandidateSource::VendorPerformance);
        CHECK(options[0].requested=="325");
        CHECK(options[0].reason.find("fixture:powerhint.json")!=std::string::npos);
    }
    auto s=measured();
    CHECK(diagnoseRuntime(RuntimeSample{}).dominant==RuntimePressure::Unknown);
    s.gpu_busy_available=true; s.gpu_busy_ratio=.95;
    CHECK(diagnoseRuntime(s).dominant!=RuntimePressure::Gpu); // utilization is not bottleneck proof
    s.frame_jank_rate=.2;
    CHECK(diagnoseRuntime(s).dominant==RuntimePressure::Gpu);
    s.frame_jank_available=false;
    CHECK(diagnoseRuntime(s).dominant!=RuntimePressure::Gpu);
    s.psi_available=true; s.psi_memory_some_avg10=.4; s.psi_io_some_avg10=.39;
    auto d=diagnoseRuntime(s);
    CHECK(d.conflicting_signals);
    CHECK(unrelatedResourcePenalty(d,ResourceDomain::Gpu)==0.);
    s.psi_io_some_avg10=0.; d=diagnoseRuntime(s);
    CHECK(d.dominant==RuntimePressure::Memory);
    CHECK(unrelatedResourcePenalty(d,ResourceDomain::Memory)==0.);
    CHECK(unrelatedResourcePenalty(d,ResourceDomain::Gpu)>0.);
    s.psi_memory_some_avg10=std::numeric_limits<double>::quiet_NaN();
    CHECK(diagnoseRuntime(s).severity==0.);
    CHECK(!parseStorageHealth("", "0x00", "garbage").available);
    CHECK(parseStorageHealth("0x0a\n","1","0x02").caution);
    CHECK(!parseStorageHealth("12","-1","4").available);
    CHECK(!parseStorageHealth("1 trailing","","0x").available);
    CHECK(discoverStorageHealthDirectories("").empty());
    Directory temp; CHECK(!temp.path.empty());
    const auto health=temp.path/"bus/platform/devices/soc.ufs/health_descriptor";
    std::filesystem::create_directories(health);
    std::ofstream(health/"life_time_estimation_a")<<"0x0b\n";
    std::ofstream(health/"eol_info")<<"0x03\n";
    const auto dirs=discoverStorageHealthDirectories(temp.path.string());
    CHECK(dirs.size()==1U);
    CHECK(readStorageHealth(dirs[0]).caution);
    const auto tunable=(temp.path/"knob").string();
    std::ofstream(tunable)<<"42\n";
    CHECK(inspectOwnedSettings({{tunable,"42"}})==ControlContinuity::Intact);
    CHECK(inspectOwnedSettings({{tunable,"43"}})==ControlContinuity::Changed);
    CHECK(inspectOwnedSettings({{tunable+"missing","42"}})==ControlContinuity::Unavailable);
    observe::PresentationWindow frames;
    CHECK(frames.consume("16666667\n1 100000000 99000000\n2 116666667 115000000\n").size()==1U);
    CHECK(frames.consume("1 100000000 99000000\n2 116666667 115000000\n").empty());
    const auto newer=frames.consume("3 133333334 132000000\n");
    CHECK(newer.size()==1U && newer[0]==16666667U);
    CHECK(frames.consume("4 9223372036854775807 132000000\n").empty());
    CHECK(frames.consume("bad\n1 2 3 4\n-1 123 5\n").empty());
    frames.reset();
    CHECK(frames.consume("1 100 90\n1 100 90\n2 200 180\n").size()==1U);
    understand::InteractivityModel interaction;
    for (unsigned i=0;i<120;++i) interaction.ingestFrame(25000000U,16666667U);
    understand::InteractivitySample input;
    (void)interaction.evaluate(input);
    (void)interaction.evaluate(input);
    input.frame_fresh=false;
    const auto absent=interaction.evaluate(input);
    CHECK(!absent.frame.valid);
    CHECK(absent.state!=understand::InteractionState::Stuttering);
    BaselineIntelligence baseline;
    s=measured();
    for (std::size_t i=0;i<4;++i) (void)baseline.captureBaseline(s);
    CHECK(baseline.beginObservation());
    s.frame_jank_rate=.3;
    for (std::size_t i=0;i<4;++i) (void)baseline.observe(s);
    CHECK(baseline.result().valid);
    CHECK(baseline.result().rendering_regression);
    CHECK(baseline.result().outcome==BaselineIntelligence::Outcome::Regression);
    CHECK(baseline.result().overall_score<.5);
    baseline.reset(); s=measured();
    for (std::size_t i=0;i<4;++i) (void)baseline.captureBaseline(s);
    CHECK(baseline.beginObservation());
    s.frame_jank_available=false; s.frame_jank_rate=.9;
    for (std::size_t i=0;i<4;++i) (void)baseline.observe(s);
    CHECK(!baseline.result().rendering_regression);
    baseline.reset(); s=measured();
    for (std::size_t i=0;i<4;++i) (void)baseline.captureBaseline(s);
    CHECK(baseline.beginObservation());
    s.frame_period_ns=8333333;
    for (std::size_t i=0;i<4;++i) (void)baseline.observe(s);
    CHECK(baseline.result().outcome==BaselineIntelligence::Outcome::Inconclusive);
    std::puts("CoreFlow runtime intelligence: PASS");
}
