#include "coreflow/config.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
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

        // Parse into a candidate so failed reads cannot partially mutate
        // the active configuration.
        EngineConfig candidate = *this;

        // An invalid mutation_mode must fail safe regardless of key order.
        bool invalid_mode = false;
        std::string line;

        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;

            const std::size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            const std::string key = trim(line.substr(0, eq));
            const std::string value = trim(line.substr(eq + 1));

            if (key == "monitor_interval") {
                try {
                    candidate.setMonitorIntervalSeconds(std::stoi(value));
                } catch (...) {
                    // Preserve the existing behavior: ignore invalid values.
                }
            } else if (key == "min_confidence") {
                try {
                    std::size_t parsed = 0;
                    const double confidence = std::stod(value, &parsed);
                    if (parsed == value.size() && std::isfinite(confidence)) {
                        candidate.setMinConfidence(confidence);
                    }
                } catch (...) {
                    // Preserve the existing behavior: ignore invalid values.
                }
            } else if (key == "mutation_mode") {
                if (value == "adaptive") {
                    candidate.mutation_mode_ = MutationMode::Adaptive;
                } else if (value == "disabled" || value == "observe") {
                    candidate.mutation_mode_ = MutationMode::Disabled;
                } else {
                    invalid_mode = true;
                    candidate.mutation_mode_ = MutationMode::Disabled;
                }
            } else if (key == "allow_cpu_governor") {
                candidate.allow_cpu_governor_ = parseBool(value, false);
            } else if (key == "runtime_refresh") {
                candidate.runtime_refresh_enabled_ =
                    parseBool(value, candidate.runtime_refresh_enabled_);
            } else if (key == "mutation_armed") {
                candidate.mutation_armed_ = parseBool(value, false);
            } else if (key == "grant_memory") {
                candidate.grant_memory_ = parseBool(value, true);
            } else if (key == "grant_block_queue") {
                candidate.grant_block_queue_ = parseBool(value, true);
            } else if (key == "grant_scheduler") {
                candidate.grant_scheduler_ = parseBool(value, true);
            } else if (key == "grant_gpu") {
                candidate.grant_gpu_ = parseBool(value, true);
            } else if (key == "grant_cgroup") {
                candidate.grant_cgroup_ = parseBool(value, true);
            } else if (key == "grant_cpufreq") {
                candidate.grant_cpufreq_ = parseBool(value, true);
            } else if (key == "grant_kernel") {
                candidate.grant_kernel_ = parseBool(value, true);
            }
        }

        // getline may stop because of an I/O error, not just EOF.
        if (file.bad()) return false;

        if (invalid_mode) {
            candidate.mutation_mode_ = MutationMode::Disabled;
            candidate.mutation_armed_ = false;
            candidate.allow_cpu_governor_ = false;
            candidate.grant_memory_ = false;
            candidate.grant_block_queue_ = false;
            candidate.grant_scheduler_ = false;
            candidate.grant_gpu_ = false;
            candidate.grant_cgroup_ = false;
            candidate.grant_cpufreq_ = false;
            candidate.grant_kernel_ = false;
        }

        // Commit only after the entire file has been read successfully.
        *this = candidate;
        return true;
    } catch (...) {
        return false;
    }
}

void EngineConfig::setMonitorIntervalSeconds(int seconds) noexcept {
    monitor_interval_seconds_ = std::clamp(seconds, 1, 60);
}

void EngineConfig::setMinConfidence(double confidence) noexcept {
    if (!std::isfinite(confidence)) {
        min_confidence_ = 0.60;
        return;
    }
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
