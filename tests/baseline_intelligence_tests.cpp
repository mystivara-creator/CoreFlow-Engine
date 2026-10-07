#include "coreflow/baseline_intelligence.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using coreflow::BaselineIntelligence;
using coreflow::RuntimeSample;

namespace {
RuntimeSample sample(double thermal_c, double load, double cpu, double memory) {
    RuntimeSample s;
    s.thermal_available = true;
    s.hottest_thermal_millidegrees = static_cast<long>(thermal_c * 1000.0);
    s.thermal_millidegrees = s.hottest_thermal_millidegrees;
    s.load1 = load;
    s.cpu_utilization_available = true;
    s.cpu_utilization = cpu;
    s.mem_total_kb = 100000;
    s.mem_available_ratio = memory;
    s.confidence = 1.0;
    return s;
}

void capture(BaselineIntelligence& b, const RuntimeSample& s) {
    for (std::size_t i = 0; i < BaselineIntelligence::kBaselineSamples; ++i) {
        const bool ready = b.captureBaseline(s);
        assert(ready == (i + 1U == BaselineIntelligence::kBaselineSamples));
    }
}

void observe(BaselineIntelligence& b, const RuntimeSample& s) {
    assert(b.beginObservation());
    for (std::size_t i = 0; i < BaselineIntelligence::kObservationSamples; ++i) {
        const bool complete = b.observe(s);
        assert(complete == (i + 1U == BaselineIntelligence::kObservationSamples));
    }
}
}

int main() {
    {
        BaselineIntelligence b;
        capture(b, sample(40.0, 6.0, 0.30, 0.50));
        observe(b, sample(38.0, 5.0, 0.25, 0.55));
        const auto& r = b.result();
        assert(r.valid);
        assert(r.context_compatible);
        assert(r.outcome == BaselineIntelligence::Outcome::Beneficial);
        assert(std::isfinite(r.overall_score));
        assert(r.stability_score >= 0.0 && r.stability_score <= 1.0);
    }

    {
        BaselineIntelligence b;
        capture(b, sample(40.0, 6.0, 0.30, 0.50));
        observe(b, sample(45.0, 8.0, 0.45, 0.40));
        assert(b.result().outcome == BaselineIntelligence::Outcome::Regression);
    }

    {
        BaselineIntelligence b;
        capture(b, sample(40.0, 6.0, 0.30, 0.50));
        observe(b, sample(40.0, 6.0, 0.30, 0.50));
        assert(b.result().outcome == BaselineIntelligence::Outcome::Neutral);
    }

    {
        BaselineIntelligence b;
        RuntimeSample base = sample(40.0, 6.0, 0.30, 0.50);
        base.charging_telemetry_available = true;
        base.charging = false;
        capture(b, base);

        RuntimeSample candidate = sample(38.0, 5.0, 0.25, 0.55);
        candidate.charging_telemetry_available = true;
        candidate.charging = true;
        observe(b, candidate);

        const auto& r = b.result();
        assert(!r.context_compatible);
        assert(r.thermal_available == false);
        assert(std::isfinite(r.confidence));
    }

    std::cout << "baseline_intelligence_tests: PASS\n";
    return 0;
}
