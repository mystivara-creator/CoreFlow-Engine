#include "coreflow/discovery/strategy_catalog.hpp"

#include <cmath>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

namespace coreflow {
namespace {
std::vector<std::string> splitTsv(const std::string& line) {
    std::vector<std::string> fields;
    std::istringstream input(line);
    std::string field;
    while (std::getline(input, field, '\t')) fields.push_back(field);
    if (!line.empty() && line.back() == '\t') fields.emplace_back();
    return fields;
}

bool parseFinite(const std::string& text, double& out) {
    if (text.empty()) return false;
    try {
        std::size_t used = 0;
        out = std::stod(text, &used);
        return used == text.size() && std::isfinite(out);
    } catch (...) {
        return false;
    }
}

bool token(const std::string& value) {
    if (value.empty() || value == "." || value == "..") return false;
    for (const unsigned char c : value) {
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
            !(c >= '0' && c <= '9') && c != '_' && c != '-' && c != '.') return false;
    }
    return true;
}

std::string keyFor(const std::string& domain, const std::string& attribute) {
    return domain + '\n' + attribute;
}
}

bool StrategyCatalog::load(const std::string& file, std::string& error) {
    std::ifstream input(file);
    if (!input) {
        error = "STRATEGY_CATALOG_OPEN_FAILED";
        return false;
    }

    std::unordered_map<std::string, StrategyRule> parsed;
    std::string line;
    std::size_t line_number = 0;
    bool schema_seen = false;
    while (std::getline(input, line)) {
        ++line_number;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const auto fields = splitTsv(line);
        if (!schema_seen) {
            if (fields.size() != 2 || fields[0] != "schema_version" || fields[1] != "2") {
                error = "STRATEGY_CATALOG_SCHEMA_UNSUPPORTED at line " + std::to_string(line_number);
                return false;
            }
            schema_seen = true;
            continue;
        }

        double minimum{}, maximum{};
        if (fields.size() != 6 || fields[0] != "strategy" || !token(fields[1]) ||
            !token(fields[2]) || !token(fields[3]) ||
            fields[2].find('/') != std::string::npos ||
            fields[3].find('/') != std::string::npos ||
            !parseFinite(fields[4], minimum) || !parseFinite(fields[5], maximum) ||
            !(minimum < maximum)) {
            error = "INVALID_STRATEGY_RULE at line " + std::to_string(line_number);
            return false;
        }
        const auto key = keyFor(fields[2], fields[3]);
        StrategyRule rule{fields[1], fields[2], fields[3], minimum, maximum};
        if (!parsed.emplace(key, std::move(rule)).second) {
            error = "DUPLICATE_STRATEGY_SELECTOR at line " + std::to_string(line_number);
            return false;
        }
    }
    if (!schema_seen) {
        error = "STRATEGY_CATALOG_SCHEMA_MISSING";
        return false;
    }
    rules_ = std::move(parsed);
    error.clear();
    return true;
}

const StrategyRule* StrategyCatalog::find(const std::string& domain,
                                          const std::string& attribute) const {
    const auto it = rules_.find(keyFor(domain, attribute));
    return it == rules_.end() ? nullptr : &it->second;
}

} // namespace coreflow
