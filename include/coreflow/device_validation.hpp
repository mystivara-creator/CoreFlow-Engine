#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace coreflow {

struct DeviceValidationSample {
    std::optional<double> cpu_psi;
    std::optional<double> memory_psi;
    std::optional<double> io_psi;
    std::optional<double> thermal_pressure;
    std::optional<double> temperature_celsius;
    std::optional<double> temperature_velocity_celsius_per_sec;
    std::optional<double> thermal_headroom;
    std::optional<double> ai_prediction_10s;
    std::optional<double> ai_confidence;
    std::string thermal_source{"unavailable"};
    std::string workload{"unknown"};
    std::string tuner_stage{"unknown"};
    std::string tuner_verdict{"unknown"};
    std::string candidate{"unknown"};
    double capacity_scale{-1.0};
    uint32_t valid_thermal_sensors{0};
    bool recovery_pending{false};
    bool journal_clear{false};
    bool rollback_verified{false};
};

class DeviceValidationRecorder {
public:
    explicit DeviceValidationRecorder(
        std::string root = "/data/adb/coreflow/cf-dvs") noexcept;
    ~DeviceValidationRecorder() noexcept;

    DeviceValidationRecorder(const DeviceValidationRecorder&) = delete;
    DeviceValidationRecorder& operator=(const DeviceValidationRecorder&) = delete;

    [[nodiscard]] bool start(std::string_view profile, std::string* error = nullptr) noexcept;
    [[nodiscard]] bool record_sample(const DeviceValidationSample& sample) noexcept;
    [[nodiscard]] bool stop(std::string* bundle_path = nullptr,
                            std::string* error = nullptr) noexcept;
    [[nodiscard]] bool active() const noexcept { return active_; }
    [[nodiscard]] std::string session_id() const { return session_id_; }

private:
    std::string root_;
    std::string session_id_;
    std::string session_dir_;
    std::string profile_;
    int events_fd_{-1};
    int metrics_fd_{-1};
    uint64_t sequence_{0};
    uint64_t started_ns_{0};
    std::string started_at_;
    uint64_t event_bytes_{0};
    uint32_t samples_{0};
    bool active_{false};
    bool failed_{false};
    DeviceValidationSample latest_{};

    [[nodiscard]] bool append_event(std::string_view type,
                                    std::string_view correlation_id,
                                    std::string_view data_json) noexcept;
    void close_files() noexcept;
};

} // namespace coreflow
