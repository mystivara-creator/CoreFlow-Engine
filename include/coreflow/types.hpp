/**
 * @file types.hpp
 * @brief Minimal CoreFlow Modern v2 runtime types
 * @author mystivara-creator
 * @license MIT
 */
#pragma once
#include <cstdint>

namespace coreflow {

enum class WorkloadState : uint8_t {
    Balanced,       ///< Normal daily use: configured responsive floor, open ceiling
    SustainedLoad,  ///< Sustained CPU pressure with thermal headroom
    PressureGuard,  ///< Memory/I/O contention: reduce artificial boosting
    ThermalGuard    ///< Thermal pressure: gently lower scheduler ceiling
};

struct UclampValues {
    uint32_t uclamp_min{0};
    uint32_t uclamp_max{1024};
};

} // namespace coreflow
