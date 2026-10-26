#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "coreflow/environment.hpp"

namespace coreflow {

// Capability expansion is descriptive evidence, never mutation authority.
// A domain can be recognized while remaining observation-only.
enum class ExpandedDomain : std::uint8_t {
    CpuIdle,
    Devfreq,
    GpuFrequency,
    ThermalIntelligence,
    PressureStall,
    Zram,
    MemoryReclaim,
    SchedulerTelemetry,
    CpuTopology,
    FilesystemIo,
    Writeback,
    TaskProfiles,
    PowerState,
    NetworkTelemetry,
    BatteryEnergy,
};

enum class CapabilityEvidence : std::uint8_t {
    Unavailable,
    Detected,
    Observable,
};

struct ExpandedDomainDescriptor {
    ExpandedDomain id;
    std::string_view name;
    std::string_view purpose;
    bool observation_only;
};

struct ExpandedDomainStatus {
    ExpandedDomain id;
    CapabilityEvidence evidence{CapabilityEvidence::Unavailable};
    std::size_t matched_resources{0};
    std::size_t readable_resources{0};
    // Intentionally always false in this catalog: discovery must not grant
    // actuator authority. Mutation policy remains in the existing safety path.
    bool mutation_authorized{false};
};

class HighCapabilityExpansion final {
public:
    static const std::array<ExpandedDomainDescriptor, 15>& catalog() noexcept;
    static std::vector<ExpandedDomainStatus> assess(
        const EnvironmentCapabilityMatrix& matrix);
    static const char* evidenceName(CapabilityEvidence evidence) noexcept;
};

} // namespace coreflow
