// Release hardening regression tests (v2.1.0).
//
// Covers: observe-only release defaults, shared thermal plausibility bounds,
// the frozen 18-feature mapping and completeness rules for the thermal model,
// and the thermal guard entry/hold logic including hottest-only confirmation.

#include "coreflow/config.hpp"
#include "coreflow/status_snapshot.hpp"
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
#include <iterator>
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

// ---- v2.1.1: live status snapshot consumed by the WebUI. ----

// Minimal strict JSON validator (RFC 8259 grammar), so the test proves the
// snapshot is parseable by any JSON.parse without depending on a library.
struct JsonCheck {
    const std::string& t;
    std::size_t i{0};
    explicit JsonCheck(const std::string& text) : t(text) {}

    bool at(char c) const { return i < t.size() && t[i] == c; }
    bool digit() const { return i < t.size() && t[i] >= '0' && t[i] <= '9'; }

    void ws() {
        while (i < t.size() &&
               (t[i] == ' ' || t[i] == '\n' || t[i] == '\r' || t[i] == '\t')) {
            ++i;
        }
    }

    bool lit(const std::string& word) {
        if (t.compare(i, word.size(), word) != 0) return false;
        i += word.size();
        return true;
    }

    bool hex4() {
        for (int k = 0; k < 4; ++k) {
            ++i;
            if (i >= t.size()) return false;
            const char h = t[i];
            const bool ok = (h >= '0' && h <= '9') || (h >= 'a' && h <= 'f') ||
                            (h >= 'A' && h <= 'F');
            if (!ok) return false;
        }
        return true;
    }

    bool str() {
        if (!at('"')) return false;
        ++i;
        while (i < t.size()) {
            const unsigned char c = static_cast<unsigned char>(t[i]);
            if (c == '"') {
                ++i;
                return true;
            }
            if (c < 0x20) return false;
            if (c == '\\') {
                ++i;
                if (i >= t.size()) return false;
                const char e = t[i];
                if (e == 'u') {
                    if (!hex4()) return false;
                } else if (std::string("\"\\/bfnrt").find(e) == std::string::npos) {
                    return false;
                }
            }
            ++i;
        }
        return false;
    }

    bool num() {
        const std::size_t start = i;
        if (at('-')) ++i;
        if (at('0')) {
            ++i;
        } else if (digit()) {
            while (digit()) ++i;
        } else {
            return false;
        }
        if (at('.')) {
            ++i;
            const std::size_t d = i;
            while (digit()) ++i;
            if (i == d) return false;
        }
        if (at('e') || at('E')) {
            ++i;
            if (at('+') || at('-')) ++i;
            const std::size_t d = i;
            while (digit()) ++i;
            if (i == d) return false;
        }
        return i > start;
    }

    bool object() {
        ++i;
        ws();
        if (at('}')) {
            ++i;
            return true;
        }
        for (;;) {
            ws();
            if (!str()) return false;
            ws();
            if (!at(':')) return false;
            ++i;
            if (!value()) return false;
            ws();
            if (at(',')) {
                ++i;
                continue;
            }
            if (at('}')) {
                ++i;
                return true;
            }
            return false;
        }
    }

    bool array() {
        ++i;
        ws();
        if (at(']')) {
            ++i;
            return true;
        }
        for (;;) {
            if (!value()) return false;
            ws();
            if (at(',')) {
                ++i;
                continue;
            }
            if (at(']')) {
                ++i;
                return true;
            }
            return false;
        }
    }

    bool value() {
        ws();
        if (i >= t.size()) return false;
        switch (t[i]) {
            case '{': return object();
            case '[': return array();
            case '"': return str();
            case 't': return lit("true");
            case 'f': return lit("false");
            case 'n': return lit("null");
            default: return num();
        }
    }

    bool document() {
        if (!value()) return false;
        ws();
        return i == t.size();
    }
};

bool validJson(const std::string& text) { JsonCheck c(text); return c.document(); }
bool has(const std::string& hay, const char* needle) { return hay.find(needle) != std::string::npos; }

EngineStatus adaptiveReady() {
    EngineStatus s;
    s.version = "2.1.0";
    s.mode = "adaptive";
    s.armed = true;
    s.baseline_ready = true;
    s.proactive_allowed = true;
    s.mutation_eligible = true;
    s.candidates = 3;
    s.confidence = 0.9;
    s.state = "NORMAL";
    return s;
}

void testStatusJsonIsStrictAndHostile() {
    EngineStatus s = adaptiveReady();
    s.thermal_available = true;
    s.thermal_c = std::numeric_limits<double>::quiet_NaN();
    s.hottest_c = std::numeric_limits<double>::infinity();
    s.load1 = -std::numeric_limits<double>::infinity();
    s.mem_available_ratio = 0.4217;
    s.plan_reason = std::string("quote\" back\\slash\nnewline\ttab\x01ctl </script> \xc3\xa9");
    s.resource_applied.emplace_back("/proc/sys/vm/swappiness\"x", "45\n");
    const std::string json = statusToJson(s);
    CHECK(validJson(json));
    CHECK(has(json, "\"thermal_c\":null"));      // NaN never leaks as a bare token
    CHECK(has(json, "\"hottest_c\":null"));
    CHECK(has(json, "\"load1\":null"));
    CHECK(has(json, "\"mem_available_ratio\":0.4217"));
    CHECK(!has(json, "nan"));
    CHECK(!has(json, "inf"));
    CHECK(!has(json, "\x01"));
}

void testStatusJsonContractShape() {
    const std::string json = statusToJson(adaptiveReady());
    CHECK(validJson(json));
    CHECK(has(json, "\"schema\":1"));
    CHECK(has(json, "\"eyes\":{"));
    CHECK(has(json, "\"brain\":{"));
    CHECK(has(json, "\"hands\":{"));
    CHECK(has(json, "\"blocker\":\"READY\""));
    CHECK(has(json, "\"stabilizing_only\":false"));
    CHECK(has(json, "\"permit_cpu\":false"));
    // Unavailable sensors are null, not zero.
    EngineStatus off = adaptiveReady();
    off.thermal_available = false;
    off.cpu_available = false;
    off.io_available = false;
    off.battery_percent = -1;
    const std::string off_json = statusToJson(off);
    CHECK(has(off_json, "\"thermal_c\":null"));
    CHECK(has(off_json, "\"cpu_utilization\":null"));
    CHECK(has(off_json, "\"io_read_kbs\":null"));
    CHECK(has(off_json, "\"battery_percent\":null"));
}

void testStatusJsonIsBounded() {
    EngineStatus s = adaptiveReady();
    s.plan_reason = std::string(100000, 'x');
    for (int i = 0; i < 100; ++i) {
        s.resource_applied.emplace_back("/p/" + std::to_string(i), "1");
    }
    const std::string json = statusToJson(s);
    CHECK(validJson(json));
    CHECK(json.size() < 16 * 1024);              // the WebUI refuses larger documents
    CHECK(has(json, "/p/15"));
    CHECK(!has(json, "/p/16"));                  // list capped at 16 entries
}

void testPrimaryBlockerOrder() {
    const EngineStatus ready = adaptiveReady();
    CHECK(primaryBlocker(ready) == "READY");

    struct Case {
        const char* expected;
        void (*mutate)(EngineStatus&);
    };
    const Case cases[] = {
        {"STOPPED", [](EngineStatus& s) { s.running = false; }},
        {"SAFETY_HOLD", [](EngineStatus& s) { s.hold_active = true; }},
        {"OBSERVE_MODE", [](EngineStatus& s) { s.mode = "observe"; }},
        {"NOT_ARMED", [](EngineStatus& s) { s.armed = false; }},
        {"BASELINE_CAPTURE", [](EngineStatus& s) { s.baseline_ready = false; }},
        {"OBSERVING_OUTCOME", [](EngineStatus& s) { s.observing = true; }},
        {"CHANGE_HELD", [](EngineStatus& s) { s.resource_mutated = true; }},
        {"CHANGE_HELD", [](EngineStatus& s) { s.cpu_mutated = true; }},
        {"COOLDOWN", [](EngineStatus& s) { s.cooldown_remaining = 3; }},
        {"CONTEXT_IDLE", [](EngineStatus& s) {
             s.proactive_allowed = false;
             s.state = "IDLE";
         }},
        {"CONTEXT_LOW_CONFIDENCE", [](EngineStatus& s) {
             s.proactive_allowed = false;
             s.confidence = 0.40;
         }},
        {"CONTEXT_NO_HEADROOM", [](EngineStatus& s) { s.proactive_allowed = false; }},
        {"READY", [](EngineStatus& s) {
             s.proactive_allowed = false;
             s.stabilizing_allowed = true;
         }},
        {"NO_CANDIDATES", [](EngineStatus& s) { s.candidates = 0; }},
        {"NO_CANDIDATES", [](EngineStatus& s) { s.mutation_eligible = false; }},
        // The first thing that stops the engine wins.
        {"SAFETY_HOLD", [](EngineStatus& s) {
             s.mode = "observe";
             s.baseline_ready = false;
             s.hold_active = true;
         }},
        {"STOPPED", [](EngineStatus& s) {
             s.running = false;
             s.hold_active = true;
         }},
    };
    for (const Case& c : cases) {
        EngineStatus s = ready;
        c.mutate(s);
        CHECK(primaryBlocker(s) == c.expected);
    }
}

void testEveryBlockerIsListed() {
    const auto& codes = blockerCodes();
    CHECK(codes.size() == 13);
    EngineStatus s = adaptiveReady();
    auto listed = [&](const std::string& c) {
        for (const auto& k : codes) if (k == c) return true;
        return false;
    };
    CHECK(listed(primaryBlocker(s)));
    s.running = false;           CHECK(listed(primaryBlocker(s)));
    s.running = true; s.hold_active = true;  CHECK(listed(primaryBlocker(s)));
    s.hold_active = false; s.mode = "observe"; CHECK(listed(primaryBlocker(s)));
}

void testWriteStatusFileIsAtomic() {
    const fs::path dir = fs::temp_directory_path() / "coreflow_status_write_test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = (dir / "status.json").string();

    const std::string first = statusToJson(adaptiveReady());
    CHECK(writeStatusFile(path, first));
    {
        std::ifstream in(path); std::string body((std::istreambuf_iterator<char>(in)), {});
        CHECK(body == first);
    }
    EngineStatus next = adaptiveReady();
    next.sample = 99;
    const std::string second = statusToJson(next);
    CHECK(writeStatusFile(path, second));
    {
        std::ifstream in(path); std::string body((std::istreambuf_iterator<char>(in)), {});
        CHECK(body == second);
        CHECK(validJson(body));
    }
    CHECK(!fs::exists(path + ".tmp"));            // no leftover temp file

    // Best-effort: failure is reported, never thrown, and leaves nothing behind.
    CHECK(!writeStatusFile((dir / "missing" / "status.json").string(), first));
    CHECK(!writeStatusFile("", first));
    CHECK(!writeStatusFile(path, ""));
    fs::remove_all(dir);
}

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
    testStatusJsonIsStrictAndHostile();
    testStatusJsonContractShape();
    testStatusJsonIsBounded();
    testPrimaryBlockerOrder();
    testEveryBlockerIsListed();
    testWriteStatusFileIsAtomic();

    if (failures != 0) {
        std::fprintf(stderr, "CoreFlow release hardening: %d/%d checks FAILED\n",
                     failures, checks);
        return 1;
    }
    std::printf("CoreFlow release hardening: %d/%d checks passed\n", checks, checks);
    return 0;
}
