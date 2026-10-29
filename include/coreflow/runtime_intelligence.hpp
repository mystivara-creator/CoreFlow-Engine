#pragma once

#include "coreflow/types.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace coreflow {
// Evidence describes pressure, never proof of the underlying application's bug.
enum class RuntimePressure : std::uint8_t { Unknown, None, Thermal, Memory, Io, Cpu, Gpu };
struct RuntimeDiagnosis {
    RuntimePressure dominant{RuntimePressure::Unknown};
    bool rendering_available{false};
    bool rendering_disturbed{false};
    bool conflicting_signals{false};
    double severity{0.0};
    double confidence{0.0};
};
RuntimeDiagnosis diagnoseRuntime(const RuntimeSample&) noexcept;
const char* runtimePressureName(RuntimePressure) noexcept;
// Penalties only: diagnosis cannot grant authority or manufacture confidence.
double unrelatedResourcePenalty(const RuntimeDiagnosis&, ResourceDomain) noexcept;

struct StorageHealth {
    // UFS descriptor codes, not percentages. Zero means unavailable.
    unsigned life_a{0};
    unsigned life_b{0};
    unsigned pre_eol{0};
    bool available{false};
    bool caution{false};
};
StorageHealth parseStorageHealth(std::string_view a, std::string_view b,
                                 std::string_view pre_eol) noexcept;
// Reads a previously discovered descriptor directory; never probes devices or writes.
StorageHealth readStorageHealth(const std::string& descriptor_directory) noexcept;
StorageHealth summarizeStorageHealth(const std::vector<std::string>& directories) noexcept;
std::vector<std::string> discoverStorageHealthDirectories(const std::string& sysfs_root);

enum class ControlContinuity : std::uint8_t { Intact, Unavailable, Changed };
ControlContinuity inspectOwnedSettings(
    const std::vector<std::pair<std::string, std::string>>& expected,
    bool cpu_policy_directories = false) noexcept;
} // namespace coreflow
