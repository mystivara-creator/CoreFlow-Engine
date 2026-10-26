#pragma once

#include <array>
#include <cstddef>
#include <deque>

#include "coreflow/types.hpp"

namespace coreflow {

// v1.2 18-feature contract. The order below is FROZEN and must match
// tools/train_coreflow_thermal_predictor_v1_2_18f.py (FEATURE_NAMES).
//
//   0  cpu_utilization          1  load1
//   2  mem_available_ratio      3  thermal_current_c
//   4  thermal_delta_c          5  charging
//   6  battery_temperature_c    7  battery_current_a
//   8  battery_voltage_v        9  uptime_delta_s
//  10  thermal_trend           11  memory_trend
//  12  load_trend              13  runtime_confidence
//  14  hottest_thermal_c       15  thermal_spread_c
//  16  io_total_kb_s           17  battery_level_percent
inline constexpr std::size_t kThermalFeatureCount = 18U;
inline constexpr std::size_t kThermalMinimumHistorySamples = 3U;

struct ThermalFeatureVector {
    std::array<float, kThermalFeatureCount> values{};
    // True only when every input is real telemetry and the vector is inside
    // the range the model was trained on. The model must not be evaluated
    // when this is false; the heuristic fallback is used instead.
    bool complete{false};
};

// Builds the 18-feature vector for the current sample. Missing telemetry is
// never encoded as 0: it marks the vector incomplete instead.
ThermalFeatureVector buildThermalFeatures(
    const std::deque<RuntimeSample>& history,
    const RuntimeSample& current) noexcept;

}  // namespace coreflow
