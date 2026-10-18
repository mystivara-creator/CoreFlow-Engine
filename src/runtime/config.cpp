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

int parseInt(const std::string& value, int fallback) noexcept {
    try {
        std::size_t parsed = 0;
        const int result = std::stoi(value, &parsed);
        if (parsed == value.size()) return result;
    } catch (...) {
        // Fall through to the fallback.
    }
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
        bool invalid_interactivity_mode = false;
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
            } else if (key == "runtime_refresh") {
                candidate.runtime_refresh_enabled_ =
                    parseBool(value, candidate.runtime_refresh_enabled_);
            } else if (key == "allow_cpu_governor") {
                candidate.allow_cpu_governor_ = parseBool(value, false);
            } else if (key == "grant_memory") {
                candidate.grants_.memory = parseBool(value, false);
            } else if (key == "grant_block_queue") {
                candidate.grants_.block_queue = parseBool(value, false);
            } else if (key == "grant_scheduler") {
                candidate.grants_.scheduler = parseBool(value, false);
            } else if (key == "grant_gpu") {
                candidate.grants_.gpu = parseBool(value, false);
            } else if (key == "grant_cgroup") {
                candidate.grants_.cgroup = parseBool(value, false);
            } else if (key == "grant_cpufreq") {
                candidate.grants_.cpufreq = parseBool(value, false);
            } else if (key == "grant_kernel") {
                candidate.grants_.kernel = parseBool(value, false);
            } else if (key == "mutation_armed") {
                candidate.mutation_armed_ = parseBool(value, false);
            } else if (key == "interactivity_mode") {
                if (value == "adaptive") {
                    candidate.interactivity_mode_ = InteractivityMode::Adaptive;
                } else if (value == "observe") {
                    candidate.interactivity_mode_ = InteractivityMode::Observe;
                } else {
                    invalid_interactivity_mode = true;
                    candidate.interactivity_mode_ = InteractivityMode::Observe;
                }
            } else if (key == "launch_window_ms") {
                candidate.setLaunchWindowMs(parseInt(value, candidate.launch_window_ms_));
            } else if (key == "jank_miss_rate") {
                try { std::size_t parsed = 0; const double rate = std::stod(value, &parsed); if (parsed == value.size() && std::isfinite(rate)) candidate.setJankMissRate(rate); } catch (...) {}
            } else if (key == "boost_readahead_kb") {
                candidate.setBoostReadaheadKb(parseInt(value, static_cast<int>(candidate.boost_readahead_kb_)));
            } else if (key == "boost_uclamp_launch") {
                candidate.setBoostUclampLaunch(parseInt(value, static_cast<int>(candidate.boost_uclamp_launch_)));
            } else if (key == "boost_uclamp_jank") {
                candidate.setBoostUclampJank(parseInt(value, static_cast<int>(candidate.boost_uclamp_jank_)));
            } else if (key == "boost_gpu_steps") {
                candidate.setBoostGpuSteps(parseInt(value, candidate.boost_gpu_steps_));
            } else if (key == "boost_window_ms") {
                candidate.setBoostWindowMs(parseInt(value, candidate.boost_window_ms_));
            }
        }

        // getline may stop because of an I/O error, not just EOF.
        if (file.bad()) return false;

        if (invalid_mode) {
            candidate.mutation_mode_ = MutationMode::Disabled;
            candidate.mutation_armed_ = false;
            candidate.interactivity_mode_ = InteractivityMode::Observe;
            // Fail safe: a rejected mode also withdraws every grant.
            candidate.allow_cpu_governor_ = false;
            candidate.grants_ = GrantFlags{};
        }
        if (invalid_interactivity_mode) {
            // An invalid boost mode disables interactivity mutation without
            // unexpectedly changing unrelated resource-mutation policy.
            candidate.interactivity_mode_ = InteractivityMode::Observe;
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

void EngineConfig::setLaunchWindowMs(int ms) noexcept {
    launch_window_ms_ = std::max(ms, 1);
}

void EngineConfig::setJankMissRate(double rate) noexcept {
    if (!std::isfinite(rate)) {
        jank_miss_rate_ = 0.05;
        return;
    }
    jank_miss_rate_ = std::clamp(rate, 0.01, 0.90);
}

void EngineConfig::setBoostReadaheadKb(long kb) noexcept {
    boost_readahead_kb_ = std::max(kb, 0L);
}

void EngineConfig::setBoostUclampLaunch(long value) noexcept {
    boost_uclamp_launch_ = std::clamp(value, 0L, 100L);
}

void EngineConfig::setBoostUclampJank(long value) noexcept {
    boost_uclamp_jank_ = std::clamp(value, 0L, 100L);
}

void EngineConfig::setBoostGpuSteps(int steps) noexcept {
    boost_gpu_steps_ = std::max(steps, 0);
}

void EngineConfig::setBoostWindowMs(int ms) noexcept {
    boost_window_ms_ = std::clamp(ms, 500, 15000);
}

const char* mutationModeName(MutationMode mode) noexcept {
    switch (mode) {
        case MutationMode::Disabled: return "DISABLED";
        case MutationMode::Adaptive: return "ADAPTIVE";
    }
    return "UNKNOWN";
}

} // namespace coreflow
