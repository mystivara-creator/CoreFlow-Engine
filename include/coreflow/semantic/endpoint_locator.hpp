#pragma once

#include "coreflow/semantic/semantic_types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace coreflow::semantic::platform {

enum class LocatorKind : std::uint8_t { None, Filesystem, KernelInterface, DriverInterface, RuntimeHandle };

// Platform-adapter-only locator. The opaque payload is never parsed by Core
// or used to infer semantic identity.
struct EndpointLocator final {
    Identifier128 locator_id{};
    LocatorKind kind{LocatorKind::None};
    std::string opaque_locator{};
    std::uint64_t generation{0};

    [[nodiscard]] bool isValid() const noexcept {
        return locator_id.isValid() && kind != LocatorKind::None && !opaque_locator.empty();
    }
};

class EndpointRegistry final {
public:
    [[nodiscard]] bool bind(CapabilityId capability, EndpointLocator locator) {
        if (!capability.isValid() || !locator.isValid()) return false;
        for (auto& entry : entries_) {
            if (entry.capability == capability) {
                if (locator.generation < entry.locator.generation) return false;
                entry.locator = std::move(locator);
                return true;
            }
        }
        entries_.push_back(Entry{capability, std::move(locator)});
        return true;
    }

    [[nodiscard]] std::optional<EndpointLocator> resolve(CapabilityId capability,
                                                          std::uint64_t active_generation) const {
        for (const auto& entry : entries_) {
            if (entry.capability == capability && entry.locator.generation == active_generation)
                return entry.locator;
        }
        return std::nullopt;
    }

    void revoke(CapabilityId capability) noexcept {
        for (auto& entry : entries_) if (entry.capability == capability) entry.locator = {};
    }

private:
    struct Entry final { CapabilityId capability{}; EndpointLocator locator{}; };
    std::vector<Entry> entries_{};
};

} // namespace coreflow::semantic::platform
