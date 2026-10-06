#include "coreflow/thermal_predictor.hpp"

#include <android/log.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <onnxruntime_cxx_api.h>

namespace coreflow {
namespace {

constexpr const char* kLogTag = "CoreFlowThermalML";
constexpr std::size_t kFeatureCount = 5;
constexpr int kDefaultHorizonTicks = 3;
constexpr std::size_t kMinimumHistorySamples = 3;

void logInfo(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, kLogTag, fmt, args);
    va_end(args);
}

void logWarn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_WARN, kLogTag, fmt, args);
    va_end(args);
}

double thermalCelsius(const RuntimeSample& sample) noexcept {
    return static_cast<double>(sample.thermal_millidegrees) / 1000.0;
}

double thermalDeltaCelsius(
    const std::deque<RuntimeSample>& history,
    const RuntimeSample& current) noexcept {
    if (!history.empty()) {
        const RuntimeSample& previous = history.back();
        if (previous.thermal_available && current.thermal_available) {
            return static_cast<double>(
                       current.thermal_millidegrees -
                       previous.thermal_millidegrees) /
                   1000.0;
        }
    }

    return 0.0;
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
    std::size_t count = 0;

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

    if (count == 0) {
        return current_temperature;
    }

    const double average_delta =
        sum_delta / static_cast<double>(count);
    return current_temperature +
           average_delta * static_cast<double>(steps_ahead);
}

} // namespace

struct ThermalPredictor::Impl {
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "CoreFlowThermalML"};
    std::unique_ptr<Ort::Session> session;
    Ort::SessionOptions options;
    Ort::AllocatorWithDefaultOptions allocator;
    std::string input_name;
    std::string output_name;
    bool ready{false};
    bool model_ready{false};
};

ThermalPredictor::ThermalPredictor() : impl_(std::make_unique<Impl>()) {
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

        const std::size_t input_count = impl_->session->GetInputCount();
        const std::size_t output_count = impl_->session->GetOutputCount();

        if (input_count != 1 || output_count != 1) {
            throw std::runtime_error("unexpected ONNX input/output count");
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

        const auto input_info = impl_->session->GetInputTypeInfo(0)
                                    .GetTensorTypeAndShapeInfo();
        const auto input_shape = input_info.GetShape();

        if (input_info.GetElementType() !=
                ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT ||
            input_shape.size() != 2 ||
            input_shape[1] != static_cast<int64_t>(kFeatureCount)) {
            throw std::runtime_error("unexpected ONNX input tensor contract");
        }

        const auto output_info = impl_->session->GetOutputTypeInfo(0)
                                     .GetTensorTypeAndShapeInfo();
        if (output_info.GetElementType() !=
            ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
            throw std::runtime_error("unexpected ONNX output tensor type");
        }

        impl_->model_ready = true;
        impl_->ready = true;

        logInfo(
            "ONNX_READY model=%s input=%s output=%s",
            model_path,
            impl_->input_name.c_str(),
            impl_->output_name.c_str());
        return true;
    } catch (const Ort::Exception& e) {
        logWarn("ONNX_INIT_FAILED error=%s fallback=HEURISTIC", e.what());
    } catch (const std::exception& e) {
        logWarn("ONNX_INIT_FAILED error=%s fallback=HEURISTIC", e.what());
    } catch (...) {
        logWarn("ONNX_INIT_FAILED error=UNKNOWN fallback=HEURISTIC");
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
    const int horizon = steps_ahead > 0 ? steps_ahead
                                        : kDefaultHorizonTicks;
    const double fallback =
        heuristicPrediction(history, current_sample, horizon);

    if (!impl_->model_ready || !current_sample.thermal_available) {
        return fallback;
    }

    const double cpu_utilization =
        current_sample.cpu_utilization_available
            ? current_sample.cpu_utilization
            : 0.0;
    const double load1 = current_sample.load1;
    const double mem_ratio = current_sample.mem_available_ratio;
    const double thermal_current = thermalCelsius(current_sample);
    const double thermal_delta =
        thermalDeltaCelsius(history, current_sample);

    std::array<float, kFeatureCount> features = {
        static_cast<float>(cpu_utilization),
        static_cast<float>(load1),
        static_cast<float>(mem_ratio),
        static_cast<float>(thermal_current),
        static_cast<float>(thermal_delta),
    };

    for (const float value : features) {
        if (!std::isfinite(static_cast<double>(value))) {
            logWarn("ONNX_INFERENCE_SKIPPED reason=nonfinite_input");
            return fallback;
        }
    }

    const std::array<int64_t, 2> input_shape = {1, 5};
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
        OrtArenaAllocator,
        OrtMemTypeDefault);

    try {
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info,
            features.data(),
            features.size(),
            input_shape.data(),
            input_shape.size());

        const char* input_names[] = {impl_->input_name.c_str()};
        const char* output_names[] = {impl_->output_name.c_str()};

        auto outputs = impl_->session->Run(
            Ort::RunOptions{nullptr},
            input_names,
            &input_tensor,
            1,
            output_names,
            1);

        if (outputs.empty() || !outputs[0].IsTensor()) {
            throw std::runtime_error("ONNX output is not a tensor");
        }

        const auto output_info = outputs[0].GetTensorTypeAndShapeInfo();
        if (output_info.GetElementType() !=
                ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT ||
            output_info.GetElementCount() != 1) {
            throw std::runtime_error("unexpected ONNX output contract");
        }

        const float* output = outputs[0].GetTensorData<float>();
        const double prediction = static_cast<double>(output[0]);

        if (!std::isfinite(prediction) || prediction < 0.0 ||
            prediction > 150.0) {
            throw std::runtime_error("ONNX prediction outside valid range");
        }

        logInfo(
            "ONNX_THERMAL_PREDICTION current=%.2fC predicted=%.2fC "
            "delta=%.3f cpu=%.3f load=%.2f",
            thermal_current,
            prediction,
            thermal_delta,
            cpu_utilization,
            load1);

        return prediction;
    } catch (const Ort::Exception& e) {
        logWarn("ONNX_INFERENCE_FAILED error=%s fallback=HEURISTIC", e.what());
    } catch (const std::exception& e) {
        logWarn("ONNX_INFERENCE_FAILED error=%s fallback=HEURISTIC", e.what());
    } catch (...) {
        logWarn("ONNX_INFERENCE_FAILED error=UNKNOWN fallback=HEURISTIC");
    }

    return fallback;
}

bool ThermalPredictor::isReady() const noexcept {
    return impl_ != nullptr && impl_->ready;
}

bool ThermalPredictor::usingModel() const noexcept {
    return impl_ != nullptr && impl_->model_ready;
}

} // namespace coreflow
