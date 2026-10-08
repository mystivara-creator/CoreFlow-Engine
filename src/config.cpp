#include "coreflow/config.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>

namespace coreflow {
namespace {

std::string trim(std::string value) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

bool parseBool(const std::string& value, bool fallback) {
    if (value == "1" || value == "true" || value == "yes" || value == "on") return true;
    if (value == "0" || value == "false" || value == "no" || value == "off") return false;
    return fallback;
}

} // namespace

bool EngineConfig::load(const std::string& path) noexcept {
    try {
        std::ifstream file(path);
        if (!file) return false;

        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;

            const std::size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            const std::string key = trim(line.substr(0, eq));
            const std::string value = trim(line.substr(eq + 1));

            if (key == "monitor_interval") {
                try { setMonitorIntervalSeconds(std::stoi(value)); } catch (...) {}
            } else if (key == "min_confidence") {
                try { setMinConfidence(std::stod(value)); } catch (...) {}
            } else if (key == "mutation_mode") {
                // Unknown values, including the removed experimental "trial"
                // mode, keep the fail-closed default (Disabled).
                if (value == "adaptive") mutation_mode_ = MutationMode::Adaptive;
                else if (value == "disabled" || value == "observe") mutation_mode_ = MutationMode::Disabled;
            } else if (key == "allow_cpu_governor") {
                allow_cpu_governor_ = parseBool(value, allow_cpu_governor_);
            } else if (key == "runtime_refresh") {
                runtime_refresh_enabled_ = parseBool(value, runtime_refresh_enabled_);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

void EngineConfig::setMonitorIntervalSeconds(int seconds) noexcept {
    monitor_interval_seconds_ = std::clamp(seconds, 1, 60);
}

void EngineConfig::setMinConfidence(double confidence) noexcept {
    min_confidence_ = std::clamp(confidence, 0.50, 1.0);
}

const char* mutationModeName(MutationMode mode) noexcept {
    switch (mode) {
        case MutationMode::Disabled: return "DISABLED";
        case MutationMode::Adaptive: return "ADAPTIVE";
    }
    return "UNKNOWN";
}

} // namespace coreflow
