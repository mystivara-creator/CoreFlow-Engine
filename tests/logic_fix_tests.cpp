// Regression tests for the Autonomous-Engine logic review fixes.
#include "coreflow/baseline_intelligence.hpp"
#include "coreflow/policy.hpp"
#include "coreflow/thermal_predictor.hpp"

#include <deque>
#include <iostream>

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { ++failures; std::cerr << "FAIL: " << #x << " (line " << __LINE__ << ")\n"; } } while (0)

coreflow::RuntimeSample base(int milli_c, double load, double cpu, double mem) {
    coreflow::RuntimeSample s;
    s.thermal_available = true;
    s.thermal_millidegrees = milli_c;
    s.hottest_thermal_millidegrees = milli_c;
    s.load1 = load;
    s.cpu_utilization_available = true;
    s.cpu_utilization = cpu;
    s.mem_total_kb = 8u * 1024u * 1024u;
    s.mem_available_ratio = mem;
    return s;
}
} // namespace

int main() {
    using namespace coreflow;
    AdaptivePolicy policy;

    // Memory pressure must win over Warming (48 C <= t < 55 C).
    CHECK(policy.evaluate(base(50000, 1.0, 0.4, 0.05), RuntimeState::Normal) ==
          RuntimeState::Pressure);
    // Pressure exit hysteresis must survive a warm device.
    CHECK(policy.evaluate(base(50000, 1.0, 0.4, 0.12), RuntimeState::Pressure) ==
          RuntimeState::Pressure);
    // ThermalGuard still has top precedence.
    CHECK(policy.evaluate(base(56000, 1.0, 0.4, 0.05), RuntimeState::Normal) ==
          RuntimeState::ThermalGuard);

    // Idle requires low measured CPU too, not only a lagging load1.
    CHECK(policy.evaluate(base(40000, 0.10, 0.50, 0.5), RuntimeState::Normal) !=
          RuntimeState::Idle);
    CHECK(policy.evaluate(base(40000, 0.10, 0.05, 0.5), RuntimeState::Normal) ==
          RuntimeState::Idle);

    // Rolling baseline: it follows the most recent pre-mutation workload.
    {
        BaselineIntelligence bi;
        for (int i = 0; i < 6; ++i) (void)bi.captureBaseline(base(40000, 0.5, 0.2, 0.5));
        CHECK(bi.baselineReady());
        CHECK(bi.baselineSampleCount() == BaselineIntelligence::kBaselineSamples);
        // Workload changes; after kBaselineSamples new samples the old ones are gone.
        for (std::size_t i = 0; i < BaselineIntelligence::kBaselineSamples; ++i)
            (void)bi.captureBaseline(base(45000, 3.0, 0.8, 0.4));
        CHECK(bi.beginObservation());
        CHECK(!bi.captureBaseline(base(45000, 3.0, 0.8, 0.4)));  // frozen
        bool done = false;
        for (std::size_t i = 0; i < BaselineIntelligence::kObservationSamples; ++i)
            done = bi.observe(base(45000, 3.0, 0.8, 0.4));
        CHECK(done);
        // Same workload before/after: must not be judged a regression, and the
        // context must be compatible (it was the stale-baseline failure mode).
        CHECK(bi.result().context_compatible);
        CHECK(bi.outcome() != BaselineIntelligence::Outcome::Regression);
        // A fresh window is required before the next candidate.
        CHECK(!bi.baselineReady());
    }

    // Stability floor: flat baseline + one 1 C quantization step is not a collapse.
    {
        BaselineIntelligence bi;
        for (std::size_t i = 0; i < BaselineIntelligence::kBaselineSamples; ++i)
            (void)bi.captureBaseline(base(45000, 1.0, 0.4, 0.5));
        CHECK(bi.beginObservation());
        (void)bi.observe(base(45000, 1.0, 0.4, 0.5));
        (void)bi.observe(base(46000, 1.0, 0.4, 0.5));
        (void)bi.observe(base(45000, 1.0, 0.4, 0.5));
        (void)bi.observe(base(46000, 1.0, 0.4, 0.5));
        CHECK(bi.result().stability_score > 0.2);
    }

    // Predictor: a slow drift across a 300 s idle gap is rate-normalized.
    {
        ThermalPredictor predictor;
        std::deque<RuntimeSample> history;
        RuntimeSample a = base(45000, 0.1, 0.05, 0.5);
        a.uptime_seconds = 1000;
        RuntimeSample b = base(48000, 0.1, 0.05, 0.5);
        b.uptime_seconds = 1300;  // +3 C over 5 minutes
        history.push_back(a);
        history.push_back(a);
        const double slow = predictor.predict(history, b, 3);
        CHECK(slow < 49.0);  // was ~54.2 C before normalization

        RuntimeSample c = base(48000, 1.0, 0.5, 0.5);
        c.uptime_seconds = 1005;  // +3 C in 5 s is a genuine fast rise
        const double fast = predictor.predict(history, c, 3);
        CHECK(fast > 52.0);
    }

    if (failures == 0) std::cout << "logic fix tests passed\n";
    return failures == 0 ? 0 : 1;
}
