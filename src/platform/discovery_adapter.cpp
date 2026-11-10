#include "coreflow/platform/discovery_adapter.hpp"

namespace coreflow::platform {

bool isValidRawEndpoint(const RawDiscoveredEndpoint& endpoint,
                        std::uint64_t active_generation) noexcept {
    if (!endpoint.probe_id.isValid() || endpoint.opaque_locator.empty() ||
        endpoint.observed_generation != active_generation) return false;

    constexpr auto known_access =
        static_cast<std::uint8_t>(semantic::AccessCapability::Read) |
        static_cast<std::uint8_t>(semantic::AccessCapability::Write) |
        static_cast<std::uint8_t>(semantic::AccessCapability::Subscribe);
    const auto raw_access = static_cast<std::uint8_t>(endpoint.raw_access);
    return (raw_access & static_cast<std::uint8_t>(~known_access)) == 0U;
}

} // namespace coreflow::platform
