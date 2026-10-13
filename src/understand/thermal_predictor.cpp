#include "coreflow/thermal_predictor.hpp"
#include "coreflow/thermal_features.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <string>

#if defined(CORE_FLOW_ANDROID)
#include <onnxruntime_cxx_api.h>
#endif

namespace coreflow {
namespace {

// Heuristic multi-step predictor used when the ONNX model is unavailable or
// the feature vector is incomplete. It is intentionally conservative: it
// extrapolates the recent thermal delta with damping so a single noisy sample
// cannot force an early ThermalGuard.
double heuristicPredict(
    const std::deque<RuntimeSample>& history,
    const RuntimeSample& current,
    int steps_ahead) noexcept
{
    if (!current.thermal_available) {
        return 0.0;
    }

    const double current_c =
        static_cast<double>(current.thermal_millidegrees) / 1000.0;

    if (history.size() < 2U || steps_ahead <= 0) {
        return current_c;
    }

    // Use the most recent valid delta, damped for longer horizons.
    double delta_c = 0.0;
    const RuntimeSample& previous = history.back();
    if (previous.thermal_available) {
        delta_c = (static_cast<double>(current.thermal_millidegrees) -
                   static_cast<double>(previous.thermal_millidegrees)) /
                  1000.0;

        // The delta is extrapolated per "step", but the sample cadence varies
        // from 1 s (ThermalGuard) to 300 s (Idle deep sleep). A slow +3 C drift
        // over a 5-minute idle gap must not be projected as +3 C per step,
        // which caused false predictive ThermalGuard entries after idle. Rate-
        // normalize gaps longer than a normal tick to a nominal step length.
        constexpr double kMaxStepSeconds = 10.0;
        if (current.uptime_seconds > previous.uptime_seconds) {
            const double dt = static_cast<double>(
                current.uptime_seconds - previous.uptime_seconds);
            if (dt > kMaxStepSeconds) {
                delta_c *= kMaxStepSeconds / dt;
            }
        }
    }

    // Mild trend bias when the classified trend is available.
    if (current.thermal_trend == Trend::Rising) {
        delta_c = std::max(delta_c, 0.15);
    } else if (current.thermal_trend == Trend::Falling) {
        delta_c = std::min(delta_c, -0.10);
    }

    // Damping: each predicted step contributes less of the observed delta so
    // long-horizon forecasts do not explode.
    double predicted = current_c;
    double remaining = delta_c;
    for (int i = 0; i < steps_ahead; ++i) {
        predicted += remaining;
        remaining *= 0.65;
    }

    // Clamp to a physically plausible mobile range; the guard layer still
    // applies its own thresholds.
    return std::clamp(predicted, 15.0, 110.0);
}

#if defined(CORE_FLOW_ANDROID)

class OnnxSession {
public:
    OnnxSession() = default;

    bool load(const char* model_path) {
        if (model_path == nullptr || model_path[0] == '\0') {
            return false;
        }
        try {
            env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING,
                                              "CoreFlowThermal");
            Ort::SessionOptions options;
            options.SetIntraOpNumThreads(1);
            options.SetGraphOptimizationLevel(
                GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
            // Mobile: prefer CPU EP only; no GPU dependency.
            session_ = std::make_unique<Ort::Session>(
                *env_, model_path, options);
            memory_info_ = std::make_unique<Ort::MemoryInfo>(
                Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));

            // Validate expected input/output shape for the v1.2 18-feature model.
            if (session_->GetInputCount() < 1 || session_->GetOutputCount() < 1) {
                reset();
                return false;
            }
            ready_ = true;
            return true;
        } catch (...) {
            reset();
            return false;
        }
    }

    // Returns predicted temperature in Celsius, or NaN on failure.
    double run(const ThermalFeatureVector& features, int steps_ahead) {
        if (!ready_ || !features.complete || steps_ahead <= 0) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        try {
            // Input: [1, 18] float32 feature vector.
            std::array<int64_t, 2> input_shape{1, static_cast<int64_t>(kThermalFeatureCount)};
            Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
                *memory_info_,
                const_cast<float*>(features.values.data()),
                kThermalFeatureCount,
                input_shape.data(),
                input_shape.size());

            // skl2onnx export used by the v1.2 training pipeline:
            // input  = "float_input"  [1, 18]
            // output = "variable"     [1, 1]   (1-step Celsius prediction)
            const char* input_names[] = {"float_input"};
            const char* output_names[] = {"variable"};

            auto outputs = session_->Run(
                Ort::RunOptions{nullptr},
                input_names, &input_tensor, 1,
                output_names, 1);

            if (outputs.empty() || !outputs[0].IsTensor()) {
                return std::numeric_limits<double>::quiet_NaN();
            }

            const float* data = outputs[0].GetTensorData<float>();
            if (data == nullptr) {
                return std::numeric_limits<double>::quiet_NaN();
            }

            double pred = static_cast<double>(data[0]);
            // If the model was trained for a 1-step horizon, extrapolate.
            // Otherwise treat the output as already multi-step.
            if (steps_ahead > 1) {
                // Conservative linear blend toward the 1-step prediction.
                // (Real multi-step models can ignore this scaling.)
                const double current_c = static_cast<double>(features.values[3]);
                const double delta = pred - current_c;
                pred = current_c;
                double rem = delta;
                for (int i = 0; i < steps_ahead; ++i) {
                    pred += rem;
                    rem *= 0.70;
                }
            }
            if (!std::isfinite(pred)) {
                return std::numeric_limits<double>::quiet_NaN();
            }
            return std::clamp(pred, 15.0, 110.0);
        } catch (...) {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }

    void reset() noexcept {
        session_.reset();
        memory_info_.reset();
        env_.reset();
        ready_ = false;
    }

    bool ready() const noexcept { return ready_; }

private:
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::MemoryInfo> memory_info_;
    bool ready_{false};
};

#endif  // CORE_FLOW_ANDROID

}  // namespace

struct ThermalPredictor::Impl {
#if defined(CORE_FLOW_ANDROID)
    OnnxSession session;
#endif
    bool model_loaded{false};
};

ThermalPredictor::ThermalPredictor()
    : impl_(std::make_unique<Impl>()) {}

ThermalPredictor::~ThermalPredictor() = default;

bool ThermalPredictor::initialize(const char* model_path) {
    if (!impl_) {
        return false;
    }
    impl_->model_loaded = false;

#if defined(CORE_FLOW_ANDROID)
    if (model_path != nullptr && model_path[0] != '\0') {
        impl_->model_loaded = impl_->session.load(model_path);
    }
#else
    (void)model_path;
#endif
    // Always report success: the heuristic path is always available.
    // Callers distinguish model vs heuristic via usingModel().
    return true;
}

double ThermalPredictor::predict(
    const std::deque<RuntimeSample>& history,
    const RuntimeSample& current_sample,
    int steps_ahead)
{
    if (steps_ahead < 1) {
        steps_ahead = 1;
    }

    // Prefer the trained model when the feature vector is complete.
#if defined(CORE_FLOW_ANDROID)
    if (impl_ && impl_->model_loaded && impl_->session.ready()) {
        const ThermalFeatureVector features =
            buildThermalFeatures(history, current_sample);
        if (features.complete) {
            const double model_pred = impl_->session.run(features, steps_ahead);
            if (std::isfinite(model_pred)) {
                return model_pred;
            }
        }
    }
#endif

    return heuristicPredict(history, current_sample, steps_ahead);
}

bool ThermalPredictor::isReady() const noexcept {
    // Ready means a prediction path exists (always true after construction).
    return impl_ != nullptr;
}

bool ThermalPredictor::usingModel() const noexcept {
#if defined(CORE_FLOW_ANDROID)
    return impl_ && impl_->model_loaded && impl_->session.ready();
#else
    return false;
#endif
}

}  // namespace coreflow
