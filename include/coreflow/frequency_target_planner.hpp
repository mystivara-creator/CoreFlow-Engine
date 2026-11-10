#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace coreflow {

enum class FrequencyDomain : uint8_t { CpuPolicy, GpuDevfreq };

struct ValidatedOperatingPoint final {
    uint64_t hz{0};
    bool exposed_by_kernel{false};
    bool within_hardware_limit{false};
    bool permitted_by_active_constraints{false};
};

struct FrequencyCapabilitySnapshot final {
    FrequencyDomain domain{FrequencyDomain::CpuPolicy};
    std::vector<ValidatedOperatingPoint> operating_points;
    uint64_t hardware_min_hz{0};
    uint64_t hardware_max_hz{0};
    uint64_t active_constraint_min_hz{0};
    uint64_t active_constraint_max_hz{0};
    uint64_t observed_policy_min_hz{0};
    uint64_t observed_policy_max_hz{0};
    bool hardware_limit_valid{false};
    bool active_constraints_valid{false};
    bool kernel_frequency_table_valid{false};
    bool thermal_admission_valid{false};
    bool thermal_admission_granted{false};
    bool capability_fingerprint_valid{false};
};

// These two values must come from an independent, current safety authority
// (e.g. a validated thermal/firmware constraint snapshot), never from
// scaling_max_freq alone. An invalid request still permits discovery, but the
// resulting snapshot cannot authorize target selection.
struct FrequencyDiscoveryRequest final {
    uint32_t policy_index{0};
    uint64_t active_constraint_min_hz{0};
    uint64_t active_constraint_max_hz{0};
    bool active_constraints_valid{false};
    bool thermal_admission_valid{false};
    bool thermal_admission_granted{false};
    bool capability_fingerprint_valid{false};
};

class FrequencyOppDiscovery final {
public:
    explicit FrequencyOppDiscovery(std::string sys_root = "/sys") noexcept
        : sys_root_(std::move(sys_root)) {}

    // Reads policyN/cpuinfo_{min,max}_freq for hardware bounds, then obtains
    // discrete OPPs from scaling_available_frequencies or stats/time_in_state.
    // Values from cpufreq sysfs are kHz and are normalized to Hz.
    [[nodiscard]] std::optional<FrequencyCapabilitySnapshot> discover_cpu_policy(
        const FrequencyDiscoveryRequest& request) const noexcept;

private:
    std::string sys_root_;
};

// The OEM scaling_max_freq is an observed policy setting, not the hardware
// ceiling. An OPP above it is eligible only when the OPP is exposed by kernel,
// within cpuinfo hardware bounds, and permitted by an independently validated
// active safety ceiling and thermal admission.
class FrequencyTargetPlanner final {
public:
    [[nodiscard]] std::optional<uint64_t> select_target(
        const FrequencyCapabilitySnapshot& capability,
        uint64_t current_hz,
        double requested_capacity_scale) const noexcept;

    [[nodiscard]] static bool validate(const FrequencyCapabilitySnapshot&) noexcept;
};

} // namespace coreflow
