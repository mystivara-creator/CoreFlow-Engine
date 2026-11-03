/**
 * @file pid_governor.hpp
 * @brief Predictive PID Thermal & Frame Equilibrium Governor
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include <chrono>
#include <algorithm>

namespace coreflow {

struct PidParameters {
    float kp{2.5f};        ///< Proportional gain
    float ki{0.15f};       ///< Integral gain (accumulated thermal drift)
    float kd{1.2f};        ///< Derivative gain (temperature velocity)
    float target_temp{42.5f}; ///< Equilibrium target temperature in Celsius
    float max_integral{50.0f};///< Anti-windup clamping
};

class PidEquilibriumGovernor {
public:
    explicit PidEquilibriumGovernor(PidParameters params = {})
        : params_(params), last_error_(0.0f), integral_(0.0f) {}

    /// Compute capacity adjustment ratio [0.0f, 1.0f] based on thermal velocity
    float compute_thermal_budget(float current_temp_celsius, std::chrono::milliseconds dt) {
        float dt_sec = std::max(0.001f, static_cast<float>(dt.count()) / 1000.0f);
        float error = params_.target_temp - current_temp_celsius;

        // Anti-windup integral accumulation
        integral_ += error * dt_sec;
        integral_ = std::clamp(integral_, -params_.max_integral, params_.max_integral);

        // Derivative: rate of temperature change (thermal velocity)
        float derivative = (error - last_error_) / dt_sec;
        last_error_ = error;

        // PID output: positive means thermal headroom available; negative means cooling needed
        float pid_output = (params_.kp * error) + (params_.ki * integral_) + (params_.kd * derivative);

        // Map PID output to a smooth continuous uclamp scaling factor between 0.40 (guarded) and 1.0 (full burst)
        // Equilibrium baseline is at pid_output == 0
        float capacity_scale = 1.0f + (pid_output / 50.0f);
        return std::clamp(capacity_scale, 0.40f, 1.0f);
    }

    void reset() noexcept {
        last_error_ = 0.0f;
        integral_ = 0.0f;
    }

    void set_parameters(const PidParameters& p) noexcept {
        params_ = p;
    }

    [[nodiscard]] const PidParameters& parameters() const noexcept {
        return params_;
    }

private:
    PidParameters params_;
    float last_error_{0.0f};
    float integral_{0.0f};
};

} // namespace coreflow
