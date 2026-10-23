#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace coreflow {
struct NumericRange {
    double minimum{};
    double maximum{};
    bool valid{false};
};
struct Capability {
    std::string id;
    std::string path;
    std::string raw_value;
    double numeric_value{};
    bool numeric{false};
    bool writable{false};
    // True only when a provider has established actionable semantics for this resource.
    // Numeric formatting and write permission alone are not optimization semantics.
    NumericRange range;
    bool optimization_supported{false};
    // Semantic classification is derived from the discovered resource location.
    // Strategy policy matches domain + attribute, never a device-specific absolute path.
    std::string domain;
    std::string attribute;
    std::string strategy_id;
    // Discrete values are discovered from the live kernel (for example, the
    // bracketed active scheduler plus the scheduler choices it exposes).
    // There is deliberately no fixed number of resources or choices.
    std::vector<std::string> choices;
};
struct SystemObservation {
    double cpu_busy_percent{};
    double memory_available_ratio{};
    double maximum_temperature_c{};
    double cpu_pressure_some_avg10{};
    double memory_pressure_some_avg10{};
    double io_pressure_some_avg10{};
    bool cpu_valid{false};
    bool memory_valid{false};
    bool temperature_valid{false};
    bool cpu_pressure_valid{false};
    bool memory_pressure_valid{false};
    bool io_pressure_valid{false};
    // Available only when the foreground package can be matched to existing
    // SurfaceFlinger timestats. No diagnostics are enabled or reset by CoreFlow.
    double frame_average_fps{};
    double frame_p95_interval_ms{};
    double frame_p99_interval_ms{};
    double slow_frame_ratio{};
    std::string frame_package;
    bool frame_timing_valid{false};
};
struct DeviceContext {
    std::string manufacturer;
    std::string model;
    std::string device_codename;
    std::string soc_model;
    std::string android_release;
    std::string android_api;
    std::string kernel_release;
    std::string storage_type{"unknown"};
    std::uint64_t total_memory_kb{};
    std::size_t online_cpu_count{};
};
struct ObservationFrame {
    SystemObservation system;
    DeviceContext device;
    std::vector<std::pair<std::string, std::string>> capability_values;
};
struct ChangeProposal {
    std::string capability_id;
    std::string path;
    std::string previous_value;
    std::string target_value;
    double minimum{};
    double maximum{};
    std::string rationale;
    // Empty for bounded numeric controls; populated from the live kernel for
    // discrete controls. Validation and read-back must stay inside this set.
    std::vector<std::string> allowed_values;
    // Semantic identity is carried with a proposal so verification can score
    // the changed subsystem instead of unrelated global metrics.
    std::string domain;
    std::string attribute;
};
struct AppliedChange {
    ChangeProposal proposal;
    bool write_attempted{false};
    bool write_succeeded{false};
    std::string readback_value;
};
struct Outcome {
    bool measurable{false};
    bool improved{false};
    double before_score{};
    double after_score{};
    std::string explanation;
};
enum class Disposition { Retain, Rollback, Unresolved };
}
