#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>

namespace coreflow {

struct StrategyRule {
    std::string id;
    std::string domain;
    std::string attribute;
    double minimum{};
    double maximum{};
};

// External, extensible policy catalogue. Rules describe resource semantics by
// domain and attribute rather than binding policy to a device-specific path.
// The catalogue deliberately has no resource-count limit.
class StrategyCatalog {
public:
    bool load(const std::string& file, std::string& error);
    const StrategyRule* find(const std::string& domain,
                             const std::string& attribute) const;
    std::size_t size() const noexcept { return rules_.size(); }

private:
    std::unordered_map<std::string, StrategyRule> rules_;
};

} // namespace coreflow
