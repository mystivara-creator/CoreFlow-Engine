#pragma once

#include "coreflow/semantic/semantic_types.hpp"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace coreflow::semantic::platform {

enum class LocatorKind : std::uint8_t { None, Filesystem, KernelInterface, DriverInterface, RuntimeHandle };

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
        const std::lock_guard lock(mutex_);
        for (auto& entry : entries_) {
            if (entry.capability == capability) {
                if (locator.generation < entry.locator.generation) return false;
                entry.locator = std::move(locator);
                return true;
            }
        }
        if (entries_.size() >= maximum_entries_) return false;
        entries_.push_back(Entry{capability, std::move(locator)});
        return true;
    }

    [[nodiscard]] std::optional<EndpointLocator> resolve(CapabilityId capability,
                                                          std::uint64_t active_generation) const {
        const std::lock_guard lock(mutex_);
        for (const auto& entry : entries_) {
            if (entry.capability == capability && entry.locator.generation == active_generation)
                return entry.locator;
        }
        return std::nullopt;
    }

    // A delayed stale cycle must not invalidate a newer successful binding.
    void revoke(CapabilityId capability, std::uint64_t observed_generation) noexcept {
        const std::lock_guard lock(mutex_);
        for (auto& entry : entries_) {
            if (entry.capability == capability && entry.locator.generation <= observed_generation)
                entry.locator = {};
        }
    }

private:
    static constexpr std::size_t maximum_entries_{4096};
    struct Entry final { CapabilityId capability{}; EndpointLocator locator{}; };
    mutable std::mutex mutex_{};
    std::vector<Entry> entries_{};
};

} // namespace coreflow::semantic::platform
