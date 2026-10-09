// Release hardening regression tests (v2.1.0).
//
// Covers: observe-only release defaults, shared thermal plausibility bounds,
// the frozen 18-feature mapping and completeness rules for the thermal model,
// and the thermal guard entry/hold logic including hottest-only confirmation.

#include "coreflow/config.hpp"
#include "coreflow/thermal_features.hpp"
#include "coreflow/thermal_guard.hpp"
#include "coreflow/thermal_limits.hpp"
#include "coreflow/types.hpp"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

using namespace coreflow;
namespace fs = std::filesystem;

namespace {

int failures = 0;
int checks = 0;

#define CHECK(x)                                                          \
    do {                                                                  \
        ++checks;                                                         \
        if (!(x)) {                                                       \
            ++failures;                                                   \
            std::fprintf(stderr, "CHECK FAILED %s:%d: %s\n", __FILE__,    \
                         __LINE__, #x);                                   \
        }                                                                 \
    } while (0)

bool near(double a, double b, double tol = 1e-4) {
    return std::fabs(a - b) <= tol;
}

// ---------------------------------------------------------------------------
// Configuration defaults
// ---------------------------------------------------------------------------

void testEngineDefaultsAreObserveOnly() {
    EngineConfig cfg;
    CHECK(cfg.mutationMode() == MutationMode::Disabled);
    CHECK(!cfg.mutationArmed());
    CHECK(!cfg.allowCpuGovernor());
}

void testShippedDefaultConfIsObserveOnly() {
    const fs::path conf =
        fs::path(COREFLOW_SOURCE_DIR) / "module/system/etc/coreflow/default.conf";
    EngineConfig cfg;
    CHECK(cfg.load(conf.string()));
    CHECK(cfg.mutationMode() == MutationMode::Disabled);
    CHECK(!cfg.mutationArmed());
    CHECK(!cfg.allowCpuGovernor());
}

void testExplicitOptInEnablesAdaptiveMutation() {
    const fs::path tmp = fs::temp_directory_path() / "coreflow_optin_test.ini";
    {
        std::ofstream f(tmp);
        f << "mutation_mode=adaptive\nmutation_armed=true\n"
             "allow_cpu_governor=yes\n";
    }
    EngineConfig cfg;
    CHECK(cfg.load(tmp.string()));
    CHECK(cfg.mutationMode() == MutationMode::Adaptive);
    CHECK(cfg.mutationArmed());
    CHECK(cfg.allowCpuGovernor());
    fs::remove(tmp);
}

void testMalformedModeFailsSafe() {
    const fs::path tmp = fs::temp_directory_path() / "coreflow_bad_mode_test.ini";
    {
        std::ofstream f(tmp);
        f << "mutation_mode=turbo\nmutation_armed=true\nallow_cpu_governor=yes\n";
    }
    EngineConfig cfg;
    CHECK(cfg.load(tmp.string()));
    CHECK(cfg.mutationMode() == MutationMode::Disabled);
    CHECK(!cfg.mutationArmed());
    CHECK(!cfg.allowCpuGovernor());
    fs::remove(tmp);
}

// ---------------------------------------------------------------------------
// Thermal plausibility (single source of truth)
// ---------------------------------------------------------------------------

void testThermalPlausibilityBounds() {
    CHECK(!isPlausibleThermalMilli(0));
    CHECK(!isPlausibleThermalMilli(-273000));
    CHECK(!isPlausibleThermalMilli(-40000));
    CHECK(!isPlausibleThermalMilli(9999));
    CHECK(isPlausibleThermalMilli(10000));
    CHECK(isPlausibleThermalMilli(45000));
    CHECK(isPlausibleThermalMilli(120000));
    CHECK(!isPlausibleThermalMilli(120001));
}

// ---------------------------------------------------------------------------
// 18-feature contract
// ---------------------------------------------------------------------------

RuntimeSample completeSample(std::uint64_t uptime, long thermal_milli) {
    RuntimeSample s;
    s.cpu_utilization = 0.25;
    s.cpu_utilization_available = true;
    s.load1 = 1.5;
    s.mem_available_ratio = 0.4;
    s.uptime_seconds = uptime;
    s.thermal_available = true;
    s.thermal_millidegrees = thermal_milli;
    s.hottest_thermal_millidegrees = thermal_milli;
    s.charging = true;
    s.charging_telemetry_available = true;
    s.battery_temperature_millidegrees = 36000;
    s.battery_current_microamps = -1200000;
    s.battery_voltage_microvolts = 4000000;
    s.battery_level_percent = 80;
    s.io_activity_available = true;
    s.io_read_kb_per_sec = 100.0;
    s.io_write_kb_per_sec = 50.0;
    s.thermal_trend = Trend::Rising;
    s.memory_trend = Trend::Stable;
    s.load_trend = Trend::Falling;
    s.confidence = 0.9;
    return s;
}

void testFeatureVectorMapsEveryIndexInFrozenOrder() {
    std::deque<RuntimeSample> history;
    history.push_back(completeSample(100, 44000));
    history.push_back(completeSample(102, 44500));
    history.push_back(completeSample(104, 44800));

    RuntimeSample current = completeSample(105, 45000);
    current.hottest_thermal_millidegrees = 47000;

    const ThermalFeatureVector f = buildThermalFeatures(history, current);
    CHECK(kThermalFeatureCount == 18U);
    CHECK(f.complete);
    CHECK(f.values.size() == 18U);

    CHECK(near(f.values[0], 0.25));    // cpu_utilization
    CHECK(near(f.values[1], 1.5));     // load1
    CHECK(near(f.values[2], 0.4));     // mem_available_ratio
    CHECK(near(f.values[3], 45.0));    // thermal_current_c
    CHECK(near(f.values[4], 0.2));     // thermal_delta_c (45.0 - 44.8)
    CHECK(near(f.values[5], 1.0));     // charging
    CHECK(near(f.values[6], 36.0));    // battery_temperature_c
    CHECK(near(f.values[7], -1.2));    // battery_current_a
    CHECK(near(f.values[8], 4.0));     // battery_voltage_v
    CHECK(near(f.values[9], 1.0));     // uptime_delta_s (105 - 104)
    CHECK(near(f.values[10], 1.0));    // thermal_trend Rising
    CHECK(near(f.values[11], 0.5));    // memory_trend Stable
    CHECK(near(f.values[12], -1.0));   // load_trend Falling
    CHECK(near(f.values[13], 0.9));    // runtime_confidence
    CHECK(near(f.values[14], 47.0));   // hottest_thermal_c
    CHECK(near(f.values[15], 2.0));    // thermal_spread_c
    CHECK(near(f.values[16], 150.0));  // io_total_kb_s
    CHECK(near(f.values[17], 80.0));   // battery_level_percent
}

void testMissingTelemetryIsNeverEncodedAsZero() {
    std::deque<RuntimeSample> history = {
        completeSample(100, 44000), completeSample(102, 44500),
        completeSample(104, 44800)};

    RuntimeSample no_battery = completeSample(105, 45000);
    no_battery.charging_telemetry_available = false;
    CHECK(!buildThermalFeatures(history, no_battery).complete);

    RuntimeSample no_io = completeSample(105, 45000);
    no_io.io_activity_available = false;
    CHECK(!buildThermalFeatures(history, no_io).complete);

    RuntimeSample no_cpu = completeSample(105, 45000);
    no_cpu.cpu_utilization_available = false;
    CHECK(!buildThermalFeatures(history, no_cpu).complete);

    RuntimeSample no_level = completeSample(105, 45000);
    no_level.battery_level_percent = -1;
    CHECK(!buildThermalFeatures(history, no_level).complete);

    RuntimeSample no_thermal = completeSample(105, 45000);
    no_thermal.thermal_available = false;
    CHECK(!buildThermalFeatures(history, no_thermal).complete);
}

void testUnknownTrendAndShortHistoryAreIncomplete() {
    std::deque<RuntimeSample> history = {
        completeSample(100, 44000), completeSample(102, 44500),
        completeSample(104, 44800)};

    RuntimeSample unknown_thermal_trend = completeSample(105, 45000);
    unknown_thermal_trend.thermal_trend = Trend::Unknown;
    CHECK(!buildThermalFeatures(history, unknown_thermal_trend).complete);

    RuntimeSample unknown_load_trend = completeSample(105, 45000);
    unknown_load_trend.load_trend = Trend::Unknown;
    CHECK(!buildThermalFeatures(history, unknown_load_trend).complete);

    // Memory trend 0 occurs in training data, so Unknown is accepted.
    RuntimeSample unknown_memory_trend = completeSample(105, 45000);
    unknown_memory_trend.memory_trend = Trend::Unknown;
    CHECK(buildThermalFeatures(history, unknown_memory_trend).complete);

    std::deque<RuntimeSample> short_history = {completeSample(100, 44000),
                                               completeSample(102, 44500)};
    CHECK(!buildThermalFeatures(short_history, completeSample(105, 45000)).complete);
}

void testNonFiniteConfidenceIsIncomplete() {
    std::deque<RuntimeSample> history = {
        completeSample(100, 44000), completeSample(102, 44500),
        completeSample(104, 44800)};
    RuntimeSample bad = completeSample(105, 45000);
    bad.confidence = std::numeric_limits<double>::quiet_NaN();
    CHECK(!buildThermalFeatures(history, bad).complete);
}

// ---------------------------------------------------------------------------
// Thermal guard
// ---------------------------------------------------------------------------

constexpr double kEnter = 55.0;
constexpr double kExit = 52.0;

ThermalGuardInputs guardIn(double selected, double hottest,
                           double predicted = 40.0, bool rising = false) {
    ThermalGuardInputs in;
    in.selected_c = selected;
    in.hottest_c = hottest;
    in.predicted_c = predicted;
    in.rising = rising;
    return in;
}

void testRepresentativeAndPredictedEnterImmediately() {
    ThermalGuardState st;
    CHECK(thermalGuardShouldEnter(guardIn(56.0, 56.0), kEnter, st));

    ThermalGuardState st2;
    CHECK(thermalGuardShouldEnter(guardIn(50.0, 50.0, 55.5), kEnter, st2));
}

void testRisingNearThresholdEntersEarly() {
    ThermalGuardState st;
    CHECK(thermalGuardShouldEnter(guardIn(54.6, 54.6, 40.0, true), kEnter, st));

    ThermalGuardState st2;
    CHECK(!thermalGuardShouldEnter(guardIn(54.6, 54.6, 40.0, false), kEnter, st2));
}

void testSingleHottestOnlySpikeDoesNotTriggerGuard() {
    // A one-sample spike on a per-core zone must not force protection.
    ThermalGuardState st;
    CHECK(!thermalGuardShouldEnter(guardIn(45.0, 58.0), kEnter, st));
    CHECK(st.hottest_only_streak == 1U);
    // Spike ends: streak resets.
    CHECK(!thermalGuardShouldEnter(guardIn(45.0, 46.0), kEnter, st));
    CHECK(st.hottest_only_streak == 0U);
}

void testSustainedHottestOnlyTriggersGuard() {
    ThermalGuardState st;
    CHECK(!thermalGuardShouldEnter(guardIn(45.0, 58.0), kEnter, st));
    CHECK(thermalGuardShouldEnter(guardIn(45.0, 58.0), kEnter, st));
    // Once confirmed, the streak stays saturated.
    CHECK(thermalGuardShouldEnter(guardIn(45.0, 58.0), kEnter, st));
}

void testStreakRequiresConsecutiveSamples() {
    ThermalGuardState st;
    CHECK(!thermalGuardShouldEnter(guardIn(45.0, 58.0), kEnter, st));
    CHECK(!thermalGuardShouldEnter(guardIn(45.0, 46.0), kEnter, st));  // breaks run
    CHECK(!thermalGuardShouldEnter(guardIn(45.0, 58.0), kEnter, st));
}

void testGuardHoldsUntilExitThreshold() {
    CHECK(thermalGuardShouldHold(guardIn(53.0, 53.0), kExit));
    CHECK(thermalGuardShouldHold(guardIn(45.0, 52.5), kExit));  // hottest keeps it
    CHECK(!thermalGuardShouldHold(guardIn(51.0, 51.5), kExit));
}

// ---------------------------------------------------------------------------

}  // namespace

int main() {
    testEngineDefaultsAreObserveOnly();
    testShippedDefaultConfIsObserveOnly();
    testExplicitOptInEnablesAdaptiveMutation();
    testMalformedModeFailsSafe();
    testThermalPlausibilityBounds();
    testFeatureVectorMapsEveryIndexInFrozenOrder();
    testMissingTelemetryIsNeverEncodedAsZero();
    testUnknownTrendAndShortHistoryAreIncomplete();
    testNonFiniteConfidenceIsIncomplete();
    testRepresentativeAndPredictedEnterImmediately();
    testRisingNearThresholdEntersEarly();
    testSingleHottestOnlySpikeDoesNotTriggerGuard();
    testSustainedHottestOnlyTriggersGuard();
    testStreakRequiresConsecutiveSamples();
    testGuardHoldsUntilExitThreshold();

    if (failures != 0) {
        std::fprintf(stderr, "CoreFlow release hardening: %d/%d checks FAILED\n",
                     failures, checks);
        return 1;
    }
    std::printf("CoreFlow release hardening: %d/%d checks passed\n", checks, checks);
    return 0;
}
