/**
 * @file neural_predictor.hpp
 * @brief Hybrid ONNX Runtime C-API Bridge & Online RLS-MLP Thermal/Workload Predictor
 * @license MIT
 *
 * Implements a two-tier real inference engine for coreflowd:
 * 1. Dynamic ONNX Runtime C-API session (via dlopen of libonnxruntime.so + .onnx model)
 *    configured with arena-free, single-threaded non-spinning execution.
 * 2. Deterministic Embedded Online Recursive Least Squares (RLS) + 6x8x2 Micro-MLP
 *    predictor with online NLMS output-head adaptation that learns the device's physical
 *    thermal time constant and nonlinear saturation curve in real time without requiring
 *    external shared libraries.
 *
 * Safety Invariant: Predictions are strictly advisory within the Reasoning Engine
 * and can never override OEM thermal ceilings, Safety Admission Gates, or readback verification.
 */
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace coreflow {

struct NeuralInput {
    double thermal_pressure{0.0};   // [0.0 .. 130.0]
    double cpu_psi{0.0};            // [0.0 .. 100.0]
    double memory_psi{0.0};         // [0.0 .. 100.0]
    double io_psi{0.0};             // [0.0 .. 100.0]
    bool cpu_psi_valid{true};
    bool memory_psi_valid{true};
    bool io_psi_valid{true};
    double current_scale{1.0};      // [0.4 .. 1.0]
    double elapsed_seconds{1.0};    // [0.001 .. 60.0]
};

struct NeuralPrediction {
    bool valid{false};
    bool onnx_active{false};
    double predicted_pressure_10s{0.0}; // Forecast thermal pressure at t + 10s
    double recommended_scale{1.0};      // Advisory capacity ceiling [0.4 .. 1.0]
    double confidence{0.0};             // Statistical confidence [0.0 .. 1.0]
    double prediction_mae{0.0};         // Running mean absolute error of pressure forecast
    double predicted_velocity{0.0};     // Learned thermal pressure rate of change (/s)
    uint64_t samples_observed{0};
    std::string_view backend{"embedded-rls-mlp"};
};

class NeuralPredictor {
public:

    explicit NeuralPredictor(std::string model_dir = "/data/adb/coreflow/models");
    ~NeuralPredictor();

    NeuralPredictor(const NeuralPredictor&) = delete;
    NeuralPredictor& operator=(const NeuralPredictor&) = delete;

    // Attempts to load an external ONNX model via libonnxruntime.so or custom weights.
    // Always succeeds in initializing the fallback online RLS-MLP engine if ONNX is absent.
    void initialize(std::string_view model_dir = "/data/adb/coreflow/models") noexcept;

    // Performs one online learning update (against the previous tick's realization)
    // followed by a forward inference pass for the 10-second horizon.
    [[nodiscard]] NeuralPrediction infer_and_learn(const NeuralInput& input) noexcept;

    // Read-only snapshot of the most recent prediction for --status telemetry.
    [[nodiscard]] NeuralPrediction last_prediction() const noexcept;

    void reset() noexcept;

private:
    std::string model_dir_;
    void* ort_lib_handle_{nullptr};
    bool onnx_loaded_{false};
    bool custom_weights_loaded_{false};

    // Online Recursive Least Squares (RLS) 4-parameter thermal state-space filter:
    // Predicts delta_pressure over a 1-second normalized horizon:
    // x = [1.0, norm_pressure, norm_cpu_psi + 0.5 * norm_io_psi, current_scale]
    std::array<double, 4> rls_weights_{0.0, 0.02, 0.12, -0.02};
    std::array<double, 16> rls_cov_{};
    std::array<double, 4> prev_features_{0.0, 0.0, 0.0, 0.0};
    std::array<double, 8> prev_hidden_{};
    double prev_out0_{0.0};
    double prev_pressure_{-1.0};
    double ema_abs_error_{1.5};
    uint64_t samples_{0};

    // Embedded 6 -> 8 -> 2 Micro-MLP weights (pre-calibrated for equilibrium pre-cooling,
    // overrideable via /data/adb/coreflow/models/equilibrium.cfw)
    std::array<double, 48> w1_{};
    std::array<double, 8> b1_{};
    std::array<double, 16> w2_{};
    std::array<double, 2> b2_{};

    NeuralPrediction last_{};

    void init_default_network() noexcept;
    bool try_load_custom_weights(const std::string& path) noexcept;
    bool try_load_onnx_runtime(const std::string& onnx_path) noexcept;
    void close_onnx_runtime() noexcept;
};

} // namespace coreflow
