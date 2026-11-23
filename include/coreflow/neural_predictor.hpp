/**
 * @file neural_predictor.hpp
 * @brief Hybrid ONNX Runtime C-API Bridge, CFW2 Micro-MLP & Online RLS Predictor
 * @license Apache-2.0
 *
 * Implements a three-tier deterministic inference engine for coreflowd:
 * 1. Tier 1 (Optional Add-on): Dynamic ONNX Runtime C-API session (dlopen of
 *    libonnxruntime.so + /data/adb/coreflow/models/equilibrium.onnx) configured
 *    single-threaded, arena-free, and non-spinning. 0 bytes bundled in standard ZIP.
 * 2. Tier 2 (Primary Native Engine): Compact binary CFW2 micro-MLP (10->16->12->3,
 *    ~1.7 KB on disk, < 16 KB RAM, CRC32-verified, O_NOFOLLOW) compiled from offline
 *    PyTorch/ONNX models via `tools/onnx_to_cfw2.py`, fused with Online RLS thermal
 *    slope + NLMS residual adaptation.
 * 3. Tier 3 (Built-in Fallback): Embedded physics-calibrated 10->16->12->3 RLS-MLP
 *    (plus backward-compatible legacy CFW1 6->8->2 loader).
 *
 * Safety Invariant: Predictions are strictly finite, clamped advisory signals and
 * can never override OEM thermal ceilings, CFJ4 allowlists, or Safety Admission Gates.
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
    // v6.1 CFW2 / ONNX 10-feature extensions (backward-compatible defaults):
    double gpu_busy_pct{0.0};       // [0.0 .. 100.0]
    double render_pressure{0.0};    // [0.0 .. 1.0]
    double jank_ratio{0.0};         // [0.0 .. 1.0]
    double charging_power_mw{0.0};  // [0.0 .. 25000.0]
};

struct NeuralPrediction {
    bool valid{false};
    bool onnx_active{false};
    bool cfw2_active{false};
    uint32_t cfw2_crc32{0};
    double predicted_pressure_10s{0.0}; // Forecast thermal pressure at t + 10s
    double jank_risk{0.0};              // Advisory frame-drop risk [0.0 .. 1.0]
    double recommended_scale{1.0};      // Advisory capacity ceiling [0.4 .. 1.0]
    double confidence{0.0};             // Statistical confidence [0.0 .. 1.0]
    double prediction_mae{0.0};         // Running mean absolute error of pressure forecast
    double predicted_velocity{0.0};     // Learned thermal pressure rate of change (/s)
    uint64_t samples_observed{0};
    std::string_view workload_class{"balanced"};
    std::string_view backend{"embedded-rls-mlp"};
};

class NeuralPredictor {
public:
    static constexpr size_t kCfw2InDim = 10U;
    static constexpr size_t kCfw2H1Dim = 16U;
    static constexpr size_t kCfw2H2Dim = 12U;
    static constexpr size_t kCfw2OutDim = 3U;

    explicit NeuralPredictor(std::string model_dir = "/data/adb/coreflow/models");
    ~NeuralPredictor();

    NeuralPredictor(const NeuralPredictor&) = delete;
    NeuralPredictor& operator=(const NeuralPredictor&) = delete;

    // Attempts to load Tier 1 ONNX Runtime + equilibrium.onnx, Tier 2 equilibrium.cfw2,
    // or legacy equilibrium.cfw. Always succeeds with Tier 3 embedded RLS-MLP fallback.
    void initialize(std::string_view model_dir = "/data/adb/coreflow/models") noexcept;

    // Performs one online learning update (against the previous tick's realization)
    // followed by a forward inference pass for the 10-second horizon.
    [[nodiscard]] NeuralPrediction infer_and_learn(const NeuralInput& input) noexcept;

    // Read-only snapshot of the most recent prediction for --status telemetry.
    [[nodiscard]] NeuralPrediction last_prediction() const noexcept;

    void reset() noexcept;

private:
    struct OrtRuntimeState;

    std::string model_dir_;
    OrtRuntimeState* ort_state_{nullptr};
    bool onnx_loaded_{false};
    bool cfw2_loaded_{false};
    bool legacy_cfw1_loaded_{false};
    uint32_t cfw2_crc32_{0};
    uint16_t cfw2_flags_{0};
    uint32_t ort_consecutive_failures_{0};

    // Online Recursive Least Squares (RLS) 4-parameter thermal state-space filter:
    // Predicts delta_pressure over a 1-second normalized horizon:
    // x = [1.0, norm_pressure, norm_cpu_psi + 0.5 * norm_io_psi, current_scale]
    std::array<double, 4> rls_weights_{0.0, 0.02, 0.14, -0.02};
    std::array<double, 16> rls_cov_{};
    std::array<double, 4> prev_features_{0.0, 0.0, 0.0, 0.0};
    std::array<double, kCfw2H2Dim> prev_h2_{};
    std::array<double, 8> prev_legacy_hidden_{};
    bool prev_used_mlp_{false};
    double prev_out0_{0.0};
    double prev_pressure_{-1.0};
    double prev_thermal_for_vel_{-1.0};
    double ema_abs_error_{1.5};
    uint64_t samples_{0};

    // Primary 10 -> 16 -> 12 -> 3 Micro-MLP weights (built-in or loaded from CFW2 binary)
    std::array<float, kCfw2H1Dim * kCfw2InDim> cfw2_w1_{};
    std::array<float, kCfw2H1Dim> cfw2_b1_{};
    std::array<float, kCfw2H2Dim * kCfw2H1Dim> cfw2_w2_{};
    std::array<float, kCfw2H2Dim> cfw2_b2_{};
    std::array<float, kCfw2OutDim * kCfw2H2Dim> cfw2_w3_{};
    std::array<float, kCfw2OutDim> cfw2_b3_{};
    // Immutable snapshot of output-0 head for O(1) lossless reset() without corrupting loaded model
    std::array<float, kCfw2H2Dim> base_cfw2_w3_out0_{};
    float base_cfw2_b3_out0_{0.05f};

    // Legacy 6 -> 8 -> 2 Micro-MLP weights (used only when equilibrium.cfw CFW1 is loaded)
    std::array<double, 48> legacy_w1_{};
    std::array<double, 8> legacy_b1_{};
    std::array<double, 16> legacy_w2_{};
    std::array<double, 2> legacy_b2_{};
    std::array<double, 8> base_legacy_w2_out0_{};
    double base_legacy_b2_out0_{0.05};

    NeuralPrediction last_{};

    void init_default_network() noexcept;
    void reset_online_state() noexcept;
    bool try_load_cfw2(const std::string& path) noexcept;
    bool try_load_legacy_cfw1(const std::string& path) noexcept;
    bool try_load_onnx_runtime(const std::string& onnx_path) noexcept;
    bool run_onnx_inference(const std::array<float, kCfw2InDim>& features,
                            std::array<double, kCfw2OutDim>& out_logits) noexcept;
    void close_onnx_runtime() noexcept;
    [[nodiscard]] std::string_view active_backend_name() const noexcept;
};

} // namespace coreflow
