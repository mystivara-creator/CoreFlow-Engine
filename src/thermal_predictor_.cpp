#include "coreflow/thermal_predictor.hpp"

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

constexpr std::size_t kFeatureCount = 14U;
constexpr int kDefaultHorizonTicks = 3;
constexpr std::size_t kMinimumHistorySamples = 3U;

constexpr double kTrendUnknown = 0.0;
constexpr double kTrendFalling = -1.0;
constexpr double kTrendStable = 0.5;
constexpr double kTrendRising = 1.0;

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

double batteryTemperatureCelsius(const RuntimeSample& sample) noexcept {
    return static_cast<double>(sample.battery_temperature_millidegrees) / 1000.0;
}

double batteryCurrentAmps(const RuntimeSample& sample) noexcept {
    return static_cast<double>(sample.battery_current_microamps) / 1000000.0;
}

double batteryVoltageVolts(const RuntimeSample& sample) noexcept {
    return static_cast<double>(sample.battery_voltage_microvolts) / 1000000.0;
}

double thermalDeltaCelsius(
    const std::deque<RuntimeSample>& history,
    const RuntimeSample& current) noexcept {
    if (history.empty()) {
        return 0.0;
    }

    const RuntimeSample& previous = history.back();
    if (!previous.thermal_available || !current.thermal_available) {
        return 0.0;
    }

    return static_cast<double>(
               current.thermal_millidegrees -
               previous.thermal_millidegrees) /
           1000.0;
}

double trendEncoding(Trend trend) noexcept {
    switch (trend) {
        case Trend::Falling:
            return kTrendFalling;
        case Trend::Stable:
            return kTrendStable;
        case Trend::Rising:
            return kTrendRising;
        case Trend::Unknown:
        default:
            return kTrendUnknown;
    }
}

double uptimeDeltaSeconds(
    const std::deque<RuntimeSample>& history,
    const RuntimeSample& current) noexcept {
    if (history.empty()) {
        return 0.0;
    }

    const RuntimeSample& previous = history.back();
    if (current.uptime_seconds < previous.uptime_seconds) {
        return 0.0;
    }

    return static_cast<double>(
        current.uptime_seconds - previous.uptime_seconds);
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
                "unexpected ONNX 14-feature input contract");
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

    const double cpu_utilization =
        current_sample.cpu_utilization_available
            ? current_sample.cpu_utilization
            : 0.0;

    const double load1 = current_sample.load1;
    const double mem_ratio =
        current_sample.mem_available_ratio;
    const double thermal_current =
        thermalCelsius(current_sample);
    const double thermal_delta =
        thermalDeltaCelsius(history, current_sample);

    const double charging =
        current_sample.charging ? 1.0 : 0.0;

    const double battery_temperature =
        current_sample.charging_telemetry_available
            ? batteryTemperatureCelsius(current_sample)
            : 0.0;

    const double battery_current =
        current_sample.charging_telemetry_available
            ? batteryCurrentAmps(current_sample)
            : 0.0;

    const double battery_voltage =
        current_sample.charging_telemetry_available
            ? batteryVoltageVolts(current_sample)
            : 0.0;

    const double uptime_delta =
        uptimeDeltaSeconds(history, current_sample);

    /*
     * Feature order is frozen and MUST match
     * train_coreflow_autonomous_thermal_predictor_v1_1_14f.py:
     *
     *  0  cpu_utilization
     *  1  load1
     *  2  mem_available_ratio
     *  3  thermal_current_c
     *  4  thermal_delta_c
     *  5  charging
     *  6  battery_temperature_c
     *  7  battery_current_a
     *  8  battery_voltage_v
     *  9  uptime_delta_s
     * 10  thermal_trend
     * 11  memory_trend
     * 12  load_trend
     * 13  runtime_confidence
     */
    const std::array<float, kFeatureCount> features = {
        static_cast<float>(cpu_utilization),
        static_cast<float>(load1),
        static_cast<float>(mem_ratio),
        static_cast<float>(thermal_current),
        static_cast<float>(thermal_delta),
        static_cast<float>(charging),
        static_cast<float>(battery_temperature),
        static_cast<float>(battery_current),
        static_cast<float>(battery_voltage),
        static_cast<float>(uptime_delta),
        static_cast<float>(trendEncoding(current_sample.thermal_trend)),
        static_cast<float>(trendEncoding(current_sample.memory_trend)),
        static_cast<float>(trendEncoding(current_sample.load_trend)),
        static_cast<float>(current_sample.confidence),
    };

    for (const float value : features) {
        if (!std::isfinite(static_cast<double>(value))) {
            logWarn(
                "ONNX_INFERENCE_SKIPPED reason=nonfinite_input");
            return fallback;
        }
    }

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

        logInfo(
            "ONNX_THERMAL_PREDICTION current=%.2fC "
            "predicted=%.2fC delta=%.3f cpu=%.3f "
            "load=%.2f charging=%s confidence=%.2f",
            thermal_current,
            prediction,
            thermal_delta,
            cpu_utilization,
            load1,
            charging > 0.5 ? "YES" : "NO",
            current_sample.confidence);

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
