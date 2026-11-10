#pragma once

#include <cstdint>
#include <optional>
#include <span>
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
    uint64_t hardware_max_hz{0};
    uint64_t active_constraint_max_hz{0};
    uint64_t observed_policy_max_hz{0};
    bool hardware_limit_valid{false};
    bool active_constraints_valid{false};
    bool kernel_frequency_table_valid{false};
    bool thermal_admission_valid{false};
    bool thermal_admission_granted{false};
    bool capability_fingerprint_valid{false};
};

// The OEM scaling_max_freq/devfreq max is an observed policy setting, not a
// hardware ceiling. This planner may select above it only when the kernel
// exposes the OPP and independent hardware + active safety constraints allow it.
class FrequencyTargetPlanner final {
public:
    [[nodiscard]] std::optional<uint64_t> select_target(
        const FrequencyCapabilitySnapshot& capability,
        uint64_t current_hz,
        double requested_capacity_scale) const noexcept;

    [[nodiscard]] static bool validate(const FrequencyCapabilitySnapshot&) noexcept;
};

} // namespace coreflow
