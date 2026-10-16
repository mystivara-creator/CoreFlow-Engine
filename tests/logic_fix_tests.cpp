#include "coreflow/decision_trace.hpp"
// Regression tests for the Autonomous-Engine logic review fixes.
#include "coreflow/baseline_intelligence.hpp"
#include "coreflow/policy.hpp"
#include "coreflow/thermal_predictor.hpp"
#include "coreflow/observer_metrics.hpp"
#include "coreflow/experience.hpp"

#include <deque>
#include <iostream>
#include <string>
#include <limits>
#include <cmath>

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

// PSI parser: valid percentages, malformed tokens, and boundaries.
    {
        using coreflow::observer_metrics::parsePsiLine;
        using coreflow::observer_metrics::normalizePsiPercent;

        double some = -1.0;
        double full = -1.0;

        CHECK(parsePsiLine(
            "some avg10=12.50 avg60=8.00 avg300=3.00 total=10",
            some, full));
        CHECK(std::abs(some - 12.50) < 1e-9);

        CHECK(parsePsiLine(
            "full avg10=0.00 avg60=0.00 avg300=0.00 total=0",
            some, full));
        CHECK(std::abs(full) < 1e-9);

        CHECK(!parsePsiLine("some avg10=invalid total=0", some, full));
        CHECK(!parsePsiLine("some avg10=12.5oops total=0", some, full));
        CHECK(!parsePsiLine("some avg10=-1 total=0", some, full));
        CHECK(!parsePsiLine("some avg10=101 total=0", some, full));
        CHECK(!parsePsiLine("other avg10=12 total=0", some, full));

        CHECK(std::abs(normalizePsiPercent(0.0)) < 1e-9);
        CHECK(std::abs(normalizePsiPercent(12.5) - 0.125) < 1e-9);
        CHECK(std::abs(normalizePsiPercent(100.0) - 1.0) < 1e-9);
        CHECK(std::abs(normalizePsiPercent(-1.0)) < 1e-9);
        CHECK(std::abs(normalizePsiPercent(
            std::numeric_limits<double>::quiet_NaN())) < 1e-9);
    }

    // Decision trace reason precedence: report a real failure even when the
    // policy plan is ineligible, but preserve safety and mode gates.
    {
        using coreflow::HoldReason;
        using coreflow::MutationMode;
        using coreflow::MutationResult;
        using coreflow::decisionTraceReason;

        CHECK(std::string(decisionTraceReason(
            HoldReason::None, MutationMode::Adaptive, true, false,
            MutationResult::Skipped, MutationResult::Failed)) == "MUTATION_FAILED");

        CHECK(std::string(decisionTraceReason(
            HoldReason::None, MutationMode::Adaptive, true, false,
            MutationResult::Skipped, MutationResult::Skipped)) == "POLICY_BLOCKED");

        CHECK(std::string(decisionTraceReason(
            HoldReason::SafeMode, MutationMode::Adaptive, true, false,
            MutationResult::Failed, MutationResult::Failed)) == "SAFETY_HOLD");

        CHECK(std::string(decisionTraceReason(
            HoldReason::None, MutationMode::Disabled, true, true,
            MutationResult::Failed, MutationResult::Failed)) == "OBSERVE_OR_NOT_ARMED");

        CHECK(std::string(decisionTraceReason(
            HoldReason::None, MutationMode::Adaptive, true, true,
            MutationResult::Verified, MutationResult::Skipped)) == "MUTATION_VERIFIED");

        CHECK(std::string(decisionTraceReason(
            HoldReason::None, MutationMode::Adaptive, true, true,
            MutationResult::RolledBack, MutationResult::Skipped)) == "MUTATION_ROLLED_BACK");
    }

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

    // A zero available-memory baseline is valid telemetry. A subsequent
    // positive ratio is a real recovery signal, not an undefined ratio.
    {
        BaselineIntelligence bi;
        for (std::size_t i = 0; i < BaselineIntelligence::kBaselineSamples; ++i)
            (void)bi.captureBaseline(base(40000, 0.5, 0.2, 0.0));
        CHECK(bi.baselineReady());
        CHECK(bi.beginObservation());
        bool done = false;
        for (std::size_t i = 0; i < BaselineIntelligence::kObservationSamples; ++i)
            done = bi.observe(base(40000, 0.5, 0.2, 0.1));
        CHECK(done);
        CHECK(bi.result().memory_score > 0.5);
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



    // Shared empirical scoring: confidence blends measured results with prior.
    {
        using coreflow::BaselineIntelligence;
        using coreflow::ExperienceMemory;
        using coreflow::blendExperienceScore;

        ExperienceMemory::Record record;
        record.candidate.key = "test-candidate";
        record.outcome = BaselineIntelligence::Outcome::Beneficial;
        record.score = 0.90;
        record.confidence = 0.80;
        record.observations = 5;

        CHECK(std::abs(blendExperienceScore(0.50, &record) - 0.82) < 1e-9);

        record.outcome = BaselineIntelligence::Outcome::Regression;
        record.score = 0.10;
        record.confidence = 0.90;
        CHECK(std::abs(blendExperienceScore(0.50, &record) - 0.14) < 1e-9);

        record.outcome = BaselineIntelligence::Outcome::Unknown;
        CHECK(std::abs(blendExperienceScore(0.40, &record) - 0.40) < 1e-9);

        record.outcome = BaselineIntelligence::Outcome::Beneficial;
        record.confidence = 0.20;
        CHECK(std::abs(blendExperienceScore(0.40, &record) - 0.40) < 1e-9);
    }

    // Per-device I/O delta regression tests.
    {
        using namespace coreflow::observer_metrics;

        DiskCounterMap previous;
        DiskCounterMap current;

        current["8:0:sda"] = {
            100U, 200U, 10U, 20U, 50U, 100U
        };

        auto delta = calculateIoDelta(current, previous, 5.0);
        CHECK(!delta.activity_available);
        CHECK(!delta.latency_available);
        CHECK(previous.size() == 1U);

        current["8:0:sda"] = {
            140U, 240U, 12U, 22U, 70U, 130U
        };

        delta = calculateIoDelta(current, previous, 5.0);
        CHECK(delta.activity_available);
        CHECK(delta.latency_available);
        CHECK(std::abs(delta.read_kb_per_sec - 4.0) < 1e-9);
        CHECK(std::abs(delta.write_kb_per_sec - 4.0) < 1e-9);
        CHECK(std::abs(delta.read_latency_ms - 10.0) < 1e-9);
        CHECK(std::abs(delta.write_latency_ms - 15.0) < 1e-9);
        CHECK(std::abs(delta.avg_latency_ms - 12.5) < 1e-9);

        // Read counters reset; unaffected write counters remain usable.
        current["8:0:sda"] = {
            5U, 260U, 1U, 24U, 3U, 160U
        };

        delta = calculateIoDelta(current, previous, 5.0);
        CHECK(delta.activity_available);
        CHECK(std::abs(delta.read_kb_per_sec) < 1e-9);
        CHECK(std::abs(delta.write_kb_per_sec - 2.0) < 1e-9);
        CHECK(delta.latency_available);
        CHECK(std::abs(delta.read_latency_ms) < 1e-9);
        CHECK(std::abs(delta.write_latency_ms - 15.0) < 1e-9);

        // A newly detected device establishes a baseline, not a fake delta.
        current["8:0:sda"] = {
            15U, 270U, 3U, 26U, 13U, 190U
        };
        current["259:0:nvme0"] = {
            10000U, 20000U, 100U, 200U, 500U, 600U
        };

        delta = calculateIoDelta(current, previous, 5.0);
        CHECK(delta.activity_available);
        CHECK(std::abs(delta.read_kb_per_sec - 1.0) < 1e-9);
        CHECK(std::abs(delta.write_kb_per_sec - 1.0) < 1e-9);

        // A removed device must not contribute a stale or negative delta.
        current.clear();
        current["259:0:nvme0"] = {
            10050U, 20030U, 101U, 202U, 505U, 630U
        };

        delta = calculateIoDelta(current, previous, 5.0);
        CHECK(delta.activity_available);
        CHECK(std::abs(delta.read_kb_per_sec - 5.0) < 1e-9);
        CHECK(std::abs(delta.write_kb_per_sec - 3.0) < 1e-9);
        CHECK(previous.size() == 1U);
    }

    if (failures == 0) std::cout << "logic fix tests passed\n";
    return failures == 0 ? 0 : 1;
}
