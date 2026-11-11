#pragma once

#include "coreflow/platform/discovery_adapter.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace coreflow::platform {

// Read-only Linux semantic discovery. Roots are copied at construction; evidence
// collection uses bounded stack buffers and POSIX file descriptors only.
class LinuxSysfsDiscoveryAdapter final : public DiscoveryAdapter {
public:
    explicit LinuxSysfsDiscoveryAdapter(
        std::string_view sysfs_root = "/sys",
        std::string_view procfs_root = "/proc") noexcept;

    [[nodiscard]] std::vector<RawDiscoveredEndpoint> discover() noexcept override;
    [[nodiscard]] std::optional<std::string> readPassive(
        const RawDiscoveredEndpoint& endpoint) noexcept override;
    [[nodiscard]] NumericFormatStream readNumericFormat(
        const RawDiscoveredEndpoint& endpoint) noexcept override;

    [[nodiscard]] std::size_t collectDomainRelationshipEvidence(
        const RawDiscoveredEndpoint& endpoint,
        std::span<semantic::DomainRelationshipEvidence> output) noexcept override;

private:
    static constexpr std::size_t kRootCapacity = 256;
    std::array<char, kRootCapacity> sysfs_root_{};
    std::array<char, kRootCapacity> procfs_root_{};
    bool roots_valid_{false};
};

} // namespace coreflow::platform
