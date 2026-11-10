#pragma once

#include "coreflow/platform/discovery_adapter.hpp"
#include "coreflow/semantic/dynamic_semantic_descriptor.hpp"
#include "coreflow/semantic/endpoint_locator.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace coreflow::semantic {

class CapabilityResolver final {
public:
    [[nodiscard]] std::vector<DynamicSemanticDescriptor> resolveAll(
        platform::DiscoveryAdapter& adapter, std::uint64_t active_generation,
        std::uint64_t now_ns) noexcept;

    [[nodiscard]] std::optional<DynamicSemanticDescriptor> resolveOne(
        const platform::RawDiscoveredEndpoint& endpoint, platform::DiscoveryAdapter& adapter,
        std::uint64_t active_generation, std::uint64_t now_ns) noexcept;

    [[nodiscard]] std::optional<platform::EndpointLocator> resolveLocator(
        CapabilityId capability, std::uint64_t active_generation) const;

private:
    [[nodiscard]] std::optional<DynamicSemanticDescriptor> buildDescriptor(
        const platform::RawDiscoveredEndpoint& endpoint, platform::DiscoveryAdapter& adapter,
        std::uint64_t active_generation, std::uint64_t now_ns) noexcept;

    platform::EndpointRegistry endpoint_registry_{};
};

} // namespace coreflow::semantic
