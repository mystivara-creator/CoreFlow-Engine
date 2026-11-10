#pragma once

#include "coreflow/semantic/semantic_types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace coreflow::platform {

struct RawDiscoveredEndpoint final {
    semantic::Identifier128 probe_id{};
    std::string opaque_locator{};
    semantic::AccessCapability raw_access{semantic::AccessCapability::None};
    std::uint64_t observed_generation{0};
};

struct NumericFormatStream final {
    bool has_minimum{false};
    bool has_maximum{false};
    bool has_resolution{false};
    std::int64_t minimum{0};
    std::int64_t maximum{0};
    std::int64_t resolution{0};
};

class DiscoveryAdapter {
public:
    virtual ~DiscoveryAdapter() = default;
    [[nodiscard]] virtual std::vector<RawDiscoveredEndpoint> discover() noexcept = 0;
    [[nodiscard]] virtual std::optional<std::string> readPassive(
        const RawDiscoveredEndpoint& endpoint) noexcept = 0;
    [[nodiscard]] virtual NumericFormatStream readNumericFormat(
        const RawDiscoveredEndpoint& endpoint) noexcept = 0;
};

[[nodiscard]] bool isValidRawEndpoint(const RawDiscoveredEndpoint& endpoint,
                                      std::uint64_t active_generation) noexcept;

} // namespace coreflow::platform
