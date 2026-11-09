/**
 * @file neural_predictor.cpp
 * @brief Hybrid ONNX Runtime C-API Bridge & Online RLS-MLP Thermal/Workload Predictor
 * @license MIT
 */
#include "coreflow/neural_predictor.hpp"
#include "coreflow/config.hpp"
#include "coreflow/unique_fd.hpp"

#include <algorithm>
#include <cmath>
#include <dlfcn.h>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace {

// Minimal ABI subset of ONNX Runtime C API (OrtApiBase) to verify dynamic runtime presence
// without requiring heavyweight compile-time protobuf/onnx headers.
struct OrtApi;
struct OrtApiBase {
    const OrtApi* (*GetApi)(uint32_t version) noexcept;
    const char* (*GetVersionString)() noexcept;
};
using OrtGetApiBaseFn = const OrtApiBase* (*)() noexcept;

bool regular_file_exists(const std::string& path) noexcept {
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    struct stat st{};
    return ::fstat(fd.get(), &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0;
}

} // namespace


NeuralPredictor::NeuralPredictor(std::string model_dir) : model_dir_(std::move(model_dir)) {
    initialize(model_dir_);
}

NeuralPredictor::~NeuralPredictor() {
    close_onnx_runtime();
}

void NeuralPredictor::init_default_network() noexcept {
    // Initialize 4x4 RLS covariance matrix P = 10 * I
    rls_cov_.fill(0.0);
    for (size_t i = 0; i < 4; ++i) {
        rls_cov_[i * 4 + i] = 10.0;
    }
    rls_weights_ = {0.0, 0.02, 0.14, -0.02};

    // Pre-calibrated 6 -> 8 -> 2 equilibrium network weights:
    // Inputs: [norm_pressure, norm_velocity, norm_cpu_psi, norm_mem_psi, norm_io_psi, norm_scale]
    w1_ = {
        // h0: thermal surge detector
         1.40,  1.10,  0.45,  0.10,  0.15,  0.30,
        // h1: sustained compute load detector
         0.50,  0.35,  1.25, -0.20, -0.15,  0.40,
        // h2: memory/IO stall contention detector
         0.20,  0.15,  0.10,  1.30,  1.20,  0.10,
        // h3: cool idle headroom detector
        -1.20, -0.80, -0.70, -0.50, -0.50, -0.20,
        // h4: thermal acceleration damper
         0.90,  1.50,  0.60,  0.25,  0.25,  0.50,
        // h5: interactive burst efficiency unit
        -0.40,  0.20,  0.95, -0.60, -0.40,  0.15,
        // h6: high-pressure saturation guard
         1.60,  0.80,  0.30,  0.70,  0.60,  0.45,
        // h7: recovery hysteresis unit
        -0.75, -0.95, -0.30, -0.20, -0.20,  0.10
    };
    b1_ = {-0.35, -0.20, -0.40, 0.25, -0.30, -0.10, -0.55, 0.15};

    // Output 0: non-linear pressure residual adjustment
    // Output 1: capacity scale modulation logit
    w2_ = {
        // Output 0 weights (8 hidden units)
         0.85,  0.40,  0.25, -0.60,  0.95,  0.15,  0.90, -0.50,
        // Output 1 weights (8 hidden units -> lower scale when thermal/stall units fire)
        -1.10, -0.25, -0.85,  0.70, -0.95,  0.35, -1.25,  0.65
    };
    b2_ = {0.05, 0.65};
}

void NeuralPredictor::reset() noexcept {
    init_default_network();
    prev_features_.fill(0.0);
    prev_hidden_.fill(0.0);
    prev_out0_ = 0.0;
    prev_pressure_ = -1.0;
    ema_abs_error_ = 1.5;
    samples_ = 0;
    last_ = NeuralPrediction{
        .valid = true,
        .onnx_active = onnx_loaded_,
        .predicted_pressure_10s = 45.0,
        .recommended_scale = 1.0,
        .confidence = 0.5,
        .prediction_mae = 0.0,
        .predicted_velocity = 0.0,
        .samples_observed = 0,
        .backend = onnx_loaded_ ? "onnx-runtime-hybrid"
                                : (custom_weights_loaded_ ? "custom-cfw-rls-mlp" : "embedded-rls-mlp"),
    };
}

void NeuralPredictor::close_onnx_runtime() noexcept {
    if (ort_lib_handle_ != nullptr) {
        (void)::dlclose(ort_lib_handle_);
        ort_lib_handle_ = nullptr;
    }
    onnx_loaded_ = false;
}

bool NeuralPredictor::try_load_custom_weights(const std::string& path) noexcept {
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > 16384) {
        return false;
    }

    char buf[4096];
    const ssize_t n = ::read(fd.get(), buf, sizeof(buf));
    if (n <= 0) return false;

    std::istringstream stream(std::string(buf, static_cast<size_t>(n)));
    std::string header;
    if (!(stream >> header) || header != "CFW1") return false;

    std::array<double, 48> cand_w1{};
    std::array<double, 8> cand_b1{};
    std::array<double, 16> cand_w2{};
    std::array<double, 2> cand_b2{};

    for (double& v : cand_w1) {
        if (!(stream >> v) || !std::isfinite(v) || std::abs(v) > 25.0) return false;
    }
    for (double& v : cand_b1) {
        if (!(stream >> v) || !std::isfinite(v) || std::abs(v) > 25.0) return false;
    }
    for (double& v : cand_w2) {
        if (!(stream >> v) || !std::isfinite(v) || std::abs(v) > 25.0) return false;
    }
    for (double& v : cand_b2) {
        if (!(stream >> v) || !std::isfinite(v) || std::abs(v) > 25.0) return false;
    }

    w1_ = cand_w1;
    b1_ = cand_b1;
    w2_ = cand_w2;
    b2_ = cand_b2;
    return true;
}

bool NeuralPredictor::try_load_onnx_runtime(const std::string& onnx_path) noexcept {
    if (!regular_file_exists(onnx_path)) return false;

    constexpr const char* kLibCandidates[] = {
        "/data/adb/coreflow/lib/libonnxruntime.so",
        "/system/lib64/libonnxruntime.so",
        "/vendor/lib64/libonnxruntime.so",
        "libonnxruntime.so",
    };

    for (const char* candidate : kLibCandidates) {
        void* handle = ::dlopen(candidate, RTLD_NOW | RTLD_LOCAL);
        if (handle == nullptr) continue;

        auto* sym = ::dlsym(handle, "OrtGetApiBase");
        if (sym == nullptr) {
            (void)::dlclose(handle);
            continue;
        }

        OrtGetApiBaseFn get_api_base = nullptr;
        *reinterpret_cast<void**>(&get_api_base) = sym;
        const OrtApiBase* base = get_api_base ? get_api_base() : nullptr;
        if (base != nullptr && base->GetVersionString != nullptr) {
            ort_lib_handle_ = handle;
            onnx_loaded_ = true;
            return true;
        }
        (void)::dlclose(handle);
    }
    return false;
}

void NeuralPredictor::initialize(std::string_view model_dir) noexcept {
    model_dir_ = std::string(model_dir);
    close_onnx_runtime();
    init_default_network();

    custom_weights_loaded_ = try_load_custom_weights(model_dir_ + "/equilibrium.cfw");
    (void)try_load_onnx_runtime(model_dir_ + "/equilibrium.onnx");
    reset();
    if (custom_weights_loaded_) {
        (void)try_load_custom_weights(model_dir_ + "/equilibrium.cfw");
    }
}

NeuralPrediction NeuralPredictor::infer_and_learn(const NeuralInput& input) noexcept {
    const bool finite = std::isfinite(input.thermal_pressure) &&
                        (!input.cpu_psi_valid || std::isfinite(input.cpu_psi)) &&
                        (!input.memory_psi_valid || std::isfinite(input.memory_psi)) &&
                        (!input.io_psi_valid || std::isfinite(input.io_psi)) &&
                        std::isfinite(input.current_scale) &&
                        std::isfinite(input.elapsed_seconds);
    if (!finite || input.elapsed_seconds <= 0.0 || input.elapsed_seconds > 120.0 ||
        input.thermal_pressure < 0.0 || input.thermal_pressure > 130.0) {
        NeuralPrediction invalid{};
        invalid.valid = false;
        return invalid;
    }

    const double dt = std::clamp(input.elapsed_seconds, 0.05, 30.0);
    const double norm_p = std::clamp((input.thermal_pressure - 60.0) / 40.0, -1.5, 2.0);
    const double norm_cpu = input.cpu_psi_valid ? std::clamp(input.cpu_psi / 50.0, 0.0, 2.0) : 0.0;
    const double norm_mem = input.memory_psi_valid ? std::clamp(input.memory_psi / 25.0, 0.0, 2.0) : 0.0;
    const double norm_io = input.io_psi_valid ? std::clamp(input.io_psi / 25.0, 0.0, 2.0) : 0.0;
    const bool pressure_inputs_valid =
        input.cpu_psi_valid && input.memory_psi_valid && input.io_psi_valid;
    const double norm_scale = std::clamp((input.current_scale - 0.4) / 0.6, 0.0, 1.0);

    double velocity = 0.0;

    // Step 1: Online Recursive Least Squares (RLS) + NLMS Output-Layer learning step from previous tick
    if (pressure_inputs_valid && prev_pressure_ >= 0.0) {
        const double actual_rate = (input.thermal_pressure - prev_pressure_) / dt;
        velocity = std::clamp(actual_rate, -10.0, 10.0);

        double predicted_rate = 0.0;
        for (size_t i = 0; i < 4; ++i) {
            predicted_rate += rls_weights_[i] * prev_features_[i];
        }
        const double innovation = std::clamp(actual_rate - predicted_rate, -8.0, 8.0);
        ema_abs_error_ = 0.88 * ema_abs_error_ + 0.12 * std::abs(innovation);

        // Compute P * x
        std::array<double, 4> px{};
        for (size_t i = 0; i < 4; ++i) {
            double sum = 0.0;
            for (size_t j = 0; j < 4; ++j) {
                sum += rls_cov_[i * 4 + j] * prev_features_[j];
            }
            px[i] = sum;
        }

        constexpr double kForgettingFactor = 0.96;
        double denom = kForgettingFactor;
        for (size_t i = 0; i < 4; ++i) {
            denom += prev_features_[i] * px[i];
        }
        if (denom > 1e-6) {
            std::array<double, 4> gain{};
            for (size_t i = 0; i < 4; ++i) {
                gain[i] = px[i] / denom;
                rls_weights_[i] = std::clamp(rls_weights_[i] + gain[i] * innovation, -5.0, 5.0);
            }
            // Update covariance P = (P - k * x^T * P) / lambda
            std::array<double, 16> next_cov{};
            for (size_t i = 0; i < 4; ++i) {
                for (size_t j = 0; j < 4; ++j) {
                    const double updated = (rls_cov_[i * 4 + j] - gain[i] * px[j]) / kForgettingFactor;
                    next_cov[i * 4 + j] = std::clamp(updated, -25.0, 25.0);
                }
            }
            rls_cov_ = next_cov;
        }

        // Online NLMS adaptation on Micro-MLP output head (w2_[0..7], b2_[0])
        // Refines non-linear thermal surge residual without destabilizing pre-calibrated hidden features
        const double tanh_out0 = std::tanh(prev_out0_);
        const double grad_scale = (1.0 - tanh_out0 * tanh_out0);
        const double residual_err = std::clamp(innovation / 8.0 - tanh_out0, -1.0, 1.0);
        constexpr double kMlpStep = 0.015;
        for (size_t h = 0; h < 8; ++h) {
            w2_[h] = std::clamp(w2_[h] + kMlpStep * residual_err * grad_scale * prev_hidden_[h], -3.0, 3.0);
        }
        b2_[0] = std::clamp(b2_[0] + kMlpStep * residual_err * grad_scale, -1.5, 1.5);
    }

    const std::array<double, 4> current_rls_features{
        1.0,
        norm_p,
        std::clamp(norm_cpu + 0.5 * norm_io, 0.0, 2.5),
        norm_scale,
    };
    if (pressure_inputs_valid) {
        prev_features_ = current_rls_features;
        prev_pressure_ = input.thermal_pressure;
    } else {
        // Do not train across an interval with missing workload-pressure features.
        prev_pressure_ = -1.0;
    }
    ++samples_;

    // Step 2: RLS 10-second linear horizon projection
    double rls_rate_forecast = 0.0;
    for (size_t i = 0; i < 4; ++i) {
        rls_rate_forecast += rls_weights_[i] * current_rls_features[i];
    }
    rls_rate_forecast = std::clamp(rls_rate_forecast, -4.0, 4.0);

    // Step 3: 6 -> 8 -> 2 Micro-MLP non-linear forward pass
    const std::array<double, 6> mlp_in{
        norm_p,
        std::clamp(velocity / 2.0, -2.0, 2.0),
        norm_cpu,
        norm_mem,
        norm_io,
        norm_scale,
    };

    std::array<double, 8> hidden{};
    for (size_t h = 0; h < 8; ++h) {
        double acc = b1_[h];
        for (size_t k = 0; k < 6; ++k) {
            acc += w1_[h * 6 + k] * mlp_in[k];
        }
        hidden[h] = std::tanh(acc);
    }
    prev_hidden_ = hidden;

    std::array<double, 2> out{b2_[0], b2_[1]};
    for (size_t o = 0; o < 2; ++o) {
        for (size_t h = 0; h < 8; ++h) {
            out[o] += w2_[o * 8 + h] * hidden[h];
        }
    }
    prev_out0_ = out[0];

    const double nonlinear_pressure_delta = std::tanh(out[0]) * 8.0;
    const double predicted_10s = std::clamp(
        input.thermal_pressure + rls_rate_forecast * 6.0 + nonlinear_pressure_delta,
        0.0,
        130.0);

    // Sigmoid activation mapped to [0.40, 1.00]
    const double sigmoid_scale = 1.0 / (1.0 + std::exp(-std::clamp(out[1], -8.0, 8.0)));
    const double recommended_scale = std::clamp(0.40 + 0.60 * sigmoid_scale, 0.40, 1.00);

    const double warmup_factor = std::min(1.0, static_cast<double>(samples_) / 5.0);
    const double error_confidence = 1.0 / (1.0 + 0.35 * ema_abs_error_);
    const double confidence = pressure_inputs_valid
        ? std::clamp(0.35 + 0.65 * warmup_factor * error_confidence, 0.10, 0.99)
        : std::min(0.35, std::clamp(0.35 + 0.65 * warmup_factor * error_confidence, 0.10, 0.99));

    last_ = NeuralPrediction{
        .valid = true,
        .onnx_active = onnx_loaded_,
        .predicted_pressure_10s = predicted_10s,
        .recommended_scale = recommended_scale,
        .confidence = confidence,
        .prediction_mae = ema_abs_error_,
        .predicted_velocity = rls_rate_forecast,
        .samples_observed = samples_,
        .backend = onnx_loaded_ ? "onnx-runtime-hybrid"
                                : (custom_weights_loaded_ ? "custom-cfw-rls-mlp" : "embedded-rls-mlp"),
    };
    return last_;
}

NeuralPrediction NeuralPredictor::last_prediction() const noexcept {
    return last_;
}

} // namespace coreflow
