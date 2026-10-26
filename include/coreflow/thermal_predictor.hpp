#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>

#include "coreflow/types.hpp"

namespace coreflow {

class ThermalPredictor {
public:
    ThermalPredictor();
    ~ThermalPredictor();

    ThermalPredictor(const ThermalPredictor&) = delete;
    ThermalPredictor& operator=(const ThermalPredictor&) = delete;

    bool initialize(const char* model_path);

    double predict(
        const std::deque<RuntimeSample>& history,
        const RuntimeSample& current_sample,
        int steps_ahead);

    bool isReady() const noexcept;
    bool usingModel() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace coreflow
