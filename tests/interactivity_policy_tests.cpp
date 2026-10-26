#include "coreflow/interactivity_policy.hpp"

#include <iostream>

namespace {
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; std::cerr << "FAIL: " #expr " line " << __LINE__ << '\n'; } } while (false)
}

int main() {
    using namespace coreflow;
    using namespace coreflow::understand;

    InteractivityPolicy policy;
    InteractivityPolicy::BoostConfig config;
    config.boost_window_ms = 3000;
    config.uclamp_launch = 20;
    config.uclamp_jank = 10;
    config.readahead_kb = 512;
    config.gpu_steps = 1;
    policy.setConfig(config);

    InteractivitySnapshot snap; // defaults to Idle
    CHECK(policy.evaluate(snap, 0).empty());
    snap.state = InteractionState::Interactive;
    CHECK(policy.evaluate(snap, 0).empty());

    snap.state = InteractionState::Launching;
    snap.launch_deadline_ms = 10'000;
    const auto launch = policy.evaluate(snap, 8'000);
    CHECK(launch.size() == 3U);
    if (launch.size() == 3U) {
        CHECK(launch[0].kind == BoostKind::TopAppUclampMin);
        CHECK(launch[0].window_ms == 2'000U);
        CHECK(launch[0].value == 20L);
        CHECK(launch[1].kind == BoostKind::GpuMinFreqStep);
        CHECK(launch[1].value == 1L);
        CHECK(launch[2].kind == BoostKind::IoReadaheadKb);
        CHECK(launch[2].value == 512L);
        CHECK(launch[2].window_ms == 2'000U);
    }

    snap.state = InteractionState::Stuttering;
    snap.gpu_busy_available = true;
    snap.gpu_busy_ratio = 0.9;
    const auto gpu_bound = policy.evaluate(snap, 0);
    CHECK(gpu_bound.size() == 2U);
    if (gpu_bound.size() == 2U) {
        CHECK(gpu_bound[0].kind == BoostKind::TopAppUclampMin);
        CHECK(gpu_bound[0].value == 10L);
        CHECK(gpu_bound[0].window_ms == 3'000U);
        CHECK(gpu_bound[1].kind == BoostKind::GpuMinFreqStep);
    }

    snap.gpu_busy_ratio = 0.2; // not GPU-bound: no GPU intent
    const auto not_gpu = policy.evaluate(snap, 0);
    CHECK(not_gpu.size() == 1U);
    if (not_gpu.size() == 1U) CHECK(not_gpu[0].kind == BoostKind::TopAppUclampMin);

    snap.gpu_busy_available = false;
    CHECK(policy.evaluate(snap, 0).size() == 1U);

    if (failures != 0) {
        std::cerr << failures << " interactivity policy test(s) failed\n";
        return 1;
    }
    std::cout << "Interactivity policy tests passed\n";
    return 0;
}
