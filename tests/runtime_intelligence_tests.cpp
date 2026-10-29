#include "coreflow/runtime_intelligence.hpp"
#include "coreflow/presentation_window.hpp"
#include "coreflow/baseline_intelligence.hpp"
#include "coreflow/interactivity_model.hpp"
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
