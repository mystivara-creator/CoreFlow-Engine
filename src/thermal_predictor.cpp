#include "coreflow/thermal_predictor.hpp"
#include "coreflow/thermal_features.hpp"

#include <android/log.h>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <onnxruntime_cxx_api.h>

namespace coreflow {
namespace {

constexpr const char* kLogTag = "CoreFlowThermalML";

// v1.2 18-feature contract. Feature order is frozen and must match
// tools/train_coreflow_thermal_predictor_v1_2_18f.py.
constexpr std::size_t kFeatureCount = kThermalFeatureCount;
constexpr int kDefaultHorizonTicks = 3;
constexpr std::size_t kMinimumHistorySamples = 3U;
// A one-step-ahead model prediction should never be far from the live sensor.
// A larger gap indicates model drift or bad input; reject it in favour of the
// bounded heuristic.
constexpr double kMaxModelDeviationC = 12.0;
// Throttle per-inference INFO logs (one inference per tick otherwise).
constexpr std::uint64_t kInferenceLogEvery = 60U;


#if defined(__clang__) || defined(__GNUC__)
#define COREFLOW_PRINTF_FORMAT(format_index, argument_index) \
    __attribute__((format(printf, format_index, argument_index)))
#else
#define COREFLOW_PRINTF_FORMAT(format_index, argument_index)
#endif

void logMessage(int priority, const char* fmt, va_list args) {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
    __android_log_vprint(priority, kLogTag, fmt, args);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}

void logInfo(const char* fmt, ...) COREFLOW_PRINTF_FORMAT(1, 2);
void logWarn(const char* fmt, ...) COREFLOW_PRINTF_FORMAT(1, 2);

void logInfo(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logMessage(ANDROID_LOG_INFO, fmt, args);
    va_end(args);
}

void logWarn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logMessage(ANDROID_LOG_WARN, fmt, args);
    va_end(args);
}

#undef COREFLOW_PRINTF_FORMAT

double thermalCelsius(const RuntimeSample& sample) noexcept {
    return static_cast<double>(sample.thermal_millidegrees) / 1000.0;
}

double heuristicPrediction(
    const std::deque<RuntimeSample>& history,
    const RuntimeSample& current,
    int steps_ahead) noexcept {
    const double current_temperature = thermalCelsius(current);

    if (history.size() < kMinimumHistorySamples) {
        return current_temperature;
    }

    double sum_delta = 0.0;
    std::size_t count = 0U;

    auto previous = history.begin();
    for (auto it = std::next(history.begin()); it != history.end(); ++it) {
        if (previous->thermal_available && it->thermal_available) {
            const double delta = static_cast<double>(
                                     it->thermal_millidegrees -
                                     previous->thermal_millidegrees) /
                                 1000.0;
            if (std::isfinite(delta)) {
                sum_delta += delta;
                ++count;
            }
        }
        previous = it;
    }

    if (count == 0U) {
        return current_temperature;
    }

    const double average_delta =
        sum_delta / static_cast<double>(count);

    return current_temperature +
           average_delta * static_cast<double>(steps_ahead);
}

}  // namespace

struct ThermalPredictor::Impl {
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "CoreFlowThermalML"};
    std::unique_ptr<Ort::Session> session;
    Ort::SessionOptions options;
    Ort::AllocatorWithDefaultOptions allocator;
    std::string input_name;
    std::string output_name;
    bool ready{false};
    bool model_ready{false};
    std::uint64_t inference_count{0};
};

ThermalPredictor::ThermalPredictor()
    : impl_(std::make_unique<Impl>()) {
    impl_->options.SetIntraOpNumThreads(1);
    impl_->options.SetInterOpNumThreads(1);
    impl_->options.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_BASIC);
}

ThermalPredictor::~ThermalPredictor() = default;

bool ThermalPredictor::initialize(const char* model_path) {
    impl_->ready = false;
    impl_->model_ready = false;
    impl_->session.reset();
    impl_->input_name.clear();
    impl_->output_name.clear();

    if (model_path == nullptr || model_path[0] == '\0') {
        logWarn("ONNX disabled: model path is empty");
        return false;
    }

    try {
        impl_->session = std::make_unique<Ort::Session>(
            impl_->env,
            model_path,
            impl_->options);

        if (impl_->session->GetInputCount() != 1U ||
            impl_->session->GetOutputCount() != 1U) {
            throw std::runtime_error(
                "unexpected ONNX input/output count");
        }

        auto input_name = impl_->session->GetInputNameAllocated(
            0, impl_->allocator);
        auto output_name = impl_->session->GetOutputNameAllocated(
            0, impl_->allocator);

        if (input_name == nullptr || output_name == nullptr) {
            throw std::runtime_error("ONNX tensor name lookup failed");
        }

        impl_->input_name = input_name.get();
        impl_->output_name = output_name.get();

        if (impl_->input_name != "float_input") {
            throw std::runtime_error("unexpected ONNX input name");
        }

        const auto input_info =
            impl_->session->GetInputTypeInfo(0)
                .GetTensorTypeAndShapeInfo();
        const auto input_shape = input_info.GetShape();

        if (input_info.GetElementType() !=
                ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT ||
            input_shape.size() != 2U ||
            input_shape[1] != static_cast<int64_t>(kFeatureCount)) {
            throw std::runtime_error(
                "unexpected ONNX 18-feature input contract");
        }

        const auto output_info =
            impl_->session->GetOutputTypeInfo(0)
                .GetTensorTypeAndShapeInfo();

        if (output_info.GetElementType() !=
            ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
            throw std::runtime_error(
                "unexpected ONNX output tensor type");
        }

        impl_->model_ready = true;
        impl_->ready = true;

        logInfo(
            "ONNX_READY model=%s input=%s output=%s features=%u",
            model_path,
            impl_->input_name.c_str(),
            impl_->output_name.c_str(),
            static_cast<unsigned>(kFeatureCount));

        return true;
    } catch (const Ort::Exception& e) {
        logWarn(
            "ONNX_INIT_FAILED error=%s fallback=HEURISTIC",
            e.what());
    } catch (const std::exception& e) {
        logWarn(
            "ONNX_INIT_FAILED error=%s fallback=HEURISTIC",
            e.what());
    } catch (...) {
        logWarn(
            "ONNX_INIT_FAILED error=UNKNOWN fallback=HEURISTIC");
    }

    impl_->session.reset();
    impl_->model_ready = false;
    impl_->ready = true;
    return false;
}

double ThermalPredictor::predict(
    const std::deque<RuntimeSample>& history,
    const RuntimeSample& current_sample,
    int steps_ahead) {
    const int horizon =
        steps_ahead > 0 ? steps_ahead : kDefaultHorizonTicks;

    const double fallback =
        heuristicPrediction(history, current_sample, horizon);

    /*
     * ThermalPredictor remains a predictor only.
     * ThermalGuard / policy remains the final safety authority.
     */
    if (!impl_->model_ready || !current_sample.thermal_available) {
        return fallback;
    }

    // Missing telemetry never reaches the model as 0. Incomplete or
    // out-of-range inputs use the bounded heuristic instead.
    const ThermalFeatureVector built =
        buildThermalFeatures(history, current_sample);
    if (!built.complete) {
        return fallback;
    }
    const std::array<float, kThermalFeatureCount>& features = built.values;

    const double thermal_current = thermalCelsius(current_sample);

    const std::array<int64_t, 2> input_shape = {
        1,
        static_cast<int64_t>(kFeatureCount),
    };

    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
        OrtArenaAllocator,
        OrtMemTypeDefault);

    try {
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info,
            const_cast<float*>(features.data()),
            features.size(),
            input_shape.data(),
            input_shape.size());

        const char* input_names[] = {
            impl_->input_name.c_str()
        };
        const char* output_names[] = {
            impl_->output_name.c_str()
        };

        auto outputs = impl_->session->Run(
            Ort::RunOptions{nullptr},
            input_names,
            &input_tensor,
            1,
            output_names,
            1);

        if (outputs.empty() || !outputs[0].IsTensor()) {
            throw std::runtime_error(
                "ONNX output is not a tensor");
        }

        const auto output_info =
            outputs[0].GetTensorTypeAndShapeInfo();

        if (output_info.GetElementType() !=
                ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT ||
            output_info.GetElementCount() != 1U) {
            throw std::runtime_error(
                "unexpected ONNX output contract");
        }

        const float* output =
            outputs[0].GetTensorData<float>();

        const double prediction =
            static_cast<double>(output[0]);

        if (!std::isfinite(prediction) ||
            prediction < 0.0 ||
            prediction > 150.0) {
            throw std::runtime_error(
                "ONNX prediction outside valid range");
        }

        if (std::fabs(prediction - thermal_current) > kMaxModelDeviationC) {
            logWarn(
                "ONNX_PREDICTION_REJECTED current=%.2fC predicted=%.2fC "
                "fallback=HEURISTIC",
                thermal_current,
                prediction);
            return fallback;
        }

        if ((impl_->inference_count++ % kInferenceLogEvery) == 0U) {
            logInfo(
                "ONNX_THERMAL_PREDICTION current=%.2fC "
                "predicted=%.2fC delta=%.3f confidence=%.2f "
                "sampled_every=%llu",
                thermal_current,
                prediction,
                static_cast<double>(features[4]),  // thermal_delta_c
                current_sample.confidence,
                static_cast<unsigned long long>(kInferenceLogEvery));
        }

        return prediction;
    } catch (const Ort::Exception& e) {
        logWarn(
            "ONNX_INFERENCE_FAILED error=%s fallback=HEURISTIC",
            e.what());
    } catch (const std::exception& e) {
        logWarn(
            "ONNX_INFERENCE_FAILED error=%s fallback=HEURISTIC",
            e.what());
    } catch (...) {
        logWarn(
            "ONNX_INFERENCE_FAILED error=UNKNOWN fallback=HEURISTIC");
    }

    return fallback;
}

bool ThermalPredictor::isReady() const noexcept {
    return impl_ != nullptr && impl_->ready;
}

bool ThermalPredictor::usingModel() const noexcept {
    return impl_ != nullptr && impl_->model_ready;
}

}  // namespace coreflow
