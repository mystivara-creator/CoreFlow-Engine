#include "coreflow/thermal_features.hpp"

#include <algorithm>
#include <cmath>

namespace coreflow {
namespace {

// Trend encoding used by the training script:
// -1 falling, 0 unknown, 0.5 stable, 1 rising.
double trendEncoding(Trend trend) noexcept {
    switch (trend) {
        case Trend::Falling: return -1.0;
        case Trend::Stable: return 0.5;
        case Trend::Rising: return 1.0;
        case Trend::Unknown:
        default: return 0.0;
    }
}

double thermalDeltaCelsius(
    const std::deque<RuntimeSample>& history,
    const RuntimeSample& current) noexcept {
    if (history.empty()) return 0.0;
    const RuntimeSample& previous = history.back();
    if (!previous.thermal_available || !current.thermal_available) return 0.0;
    return static_cast<double>(current.thermal_millidegrees -
                               previous.thermal_millidegrees) / 1000.0;
}

double uptimeDeltaSeconds(
    const std::deque<RuntimeSample>& history,
    const RuntimeSample& current) noexcept {
    if (history.empty()) return 0.0;
    const RuntimeSample& previous = history.back();
    if (current.uptime_seconds < previous.uptime_seconds) return 0.0;
    return static_cast<double>(current.uptime_seconds - previous.uptime_seconds);
}

bool allFinite(const std::array<float, kThermalFeatureCount>& values) noexcept {
    for (const float v : values) {
        if (!std::isfinite(static_cast<double>(v))) return false;
    }
    return true;
}

}  // namespace

ThermalFeatureVector buildThermalFeatures(
    const std::deque<RuntimeSample>& history,
    const RuntimeSample& current) noexcept {
    ThermalFeatureVector out;

    const double thermal_current =
        static_cast<double>(current.thermal_millidegrees) / 1000.0;
    const double hottest_c =
        static_cast<double>(current.hottest_thermal_millidegrees) / 1000.0;
    const double spread_c = std::max(0.0, hottest_c - thermal_current);

    const double battery_temperature =
        static_cast<double>(current.battery_temperature_millidegrees) / 1000.0;
    const double battery_current =
        static_cast<double>(current.battery_current_microamps) / 1000000.0;
    const double battery_voltage =
        static_cast<double>(current.battery_voltage_microvolts) / 1000000.0;
    const double io_total =
        current.io_read_kb_per_sec + current.io_write_kb_per_sec;

    auto& v = out.values;
    v[0] = static_cast<float>(current.cpu_utilization);
    v[1] = static_cast<float>(current.load1);
    v[2] = static_cast<float>(current.mem_available_ratio);
    v[3] = static_cast<float>(thermal_current);
    v[4] = static_cast<float>(thermalDeltaCelsius(history, current));
    v[5] = current.charging ? 1.0F : 0.0F;
    v[6] = static_cast<float>(battery_temperature);
    v[7] = static_cast<float>(battery_current);
    v[8] = static_cast<float>(battery_voltage);
    v[9] = static_cast<float>(uptimeDeltaSeconds(history, current));
    v[10] = static_cast<float>(trendEncoding(current.thermal_trend));
    v[11] = static_cast<float>(trendEncoding(current.memory_trend));
    v[12] = static_cast<float>(trendEncoding(current.load_trend));
    v[13] = static_cast<float>(current.confidence);
    v[14] = static_cast<float>(hottest_c);
    v[15] = static_cast<float>(spread_c);
    v[16] = static_cast<float>(io_total);
    v[17] = static_cast<float>(current.battery_level_percent);

    // Completeness: every source the model depends on must be real telemetry.
    // Thermal trend 0 (Unknown) never occurs in training data, so an unknown
    // thermal or load trend also disqualifies the model. Memory trend 0 is a
    // valid training value and is accepted.
    const bool telemetry_ok =
        current.thermal_available &&
        current.cpu_utilization_available &&
        current.charging_telemetry_available &&
        current.io_activity_available &&
        current.battery_level_percent >= 0 &&
        current.battery_level_percent <= 100;
    const bool trends_ok =
        current.thermal_trend != Trend::Unknown &&
        current.load_trend != Trend::Unknown;
    const bool history_ok = history.size() >= kThermalMinimumHistorySamples;

    out.complete = telemetry_ok && trends_ok && history_ok && allFinite(v);
    return out;
}

}  // namespace coreflow
