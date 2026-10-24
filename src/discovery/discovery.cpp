#include "coreflow/discovery/discovery.hpp"
#include "coreflow/discovery/classification.hpp"
#include "coreflow/discovery/strategy_catalog.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace coreflow {
namespace {
bool parseNumber(const std::string& text, double& out) {
    if (text.empty()) return false;
    try {
        std::size_t used = 0;
        out = std::stod(text, &used);
        return used == text.size() && std::isfinite(out);
    } catch (...) { return false; }
}

std::string firstToken(const std::string& text) {
    std::istringstream in(text);
    std::string value;
    in >> value;
    return value;
}

std::vector<std::string> tokens(const std::string& text) {
    std::istringstream in(text);
    std::vector<std::string> result;
    std::string value;
    while (in >> value) {
        if (!value.empty() && value.front() == '[' && value.back() == ']')
            value = value.substr(1, value.size() - 2);
        if (!value.empty() && std::find(result.begin(), result.end(), value) == result.end())
            result.push_back(value);
    }
    return result;
}

bool readResource(const fs::path& path, std::string& raw) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;

    // Enforce the size limit while reading; checking raw.size() after assigning
    // an unbounded stream would not protect discovery from oversized attributes.
    constexpr std::size_t kMaximumBytes = 2048;
    std::string buffer;
    buffer.reserve(256);
    char chunk[256];
    while (input) {
        input.read(chunk, sizeof(chunk));
        const auto count = input.gcount();
        if (count > 0) {
            const auto amount = static_cast<std::size_t>(count);
            if (amount > kMaximumBytes - buffer.size()) return false;
            buffer.append(chunk, amount);
        }
    }
    if (input.bad()) return false;

    const auto first = buffer.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return false;
    const auto last = buffer.find_last_not_of(" \t\r\n");
    raw = buffer.substr(first, last - first + 1);
    return true;
}

// The active governor of a cpufreq policy. Governor tunables live in a directory
// named after the governor the policy is running, so classification needs it.
std::string activeGovernorFor(const fs::path& path) {
    const auto dir = path.parent_path();
    const auto policy = dir.parent_path();
    if (policy.filename().string().rfind("policy", 0) != 0) return {};
    std::string governor;
    if (!readResource(policy / "scaling_governor", governor)) return {};
    return governor;
}

std::optional<double> readNumber(const fs::path& path) {
    std::string raw;
    if (!readResource(path, raw)) return std::nullopt;
    double value{};
    if (!parseNumber(firstToken(raw), value)) return std::nullopt;
    return value;
}

NumericRange discoverRange(const fs::path& path) {
    const auto parent = path.parent_path();
    const auto name = path.filename().string();
    const std::vector<std::pair<std::string, std::string>> companions = {
        {name + ".min", name + ".max"},
        {name + "_min", name + "_max"},
        {"min_" + name, "max_" + name}
    };
    for (const auto& pair : companions) {
        const auto lo = readNumber(parent / pair.first);
        const auto hi = readNumber(parent / pair.second);
        if (lo && hi && *lo < *hi) return {*lo, *hi, true};
    }

    // cpufreq exposes hardware limits as sibling metadata rather than per-file
    // <attribute>_min/<attribute>_max companions. The range is read from the
    // running policy, so it follows the active SoC/kernel instead of a device list.
    if (name == "scaling_min_freq" || name == "scaling_max_freq") {
        const auto hardware_min = readNumber(parent / "cpuinfo_min_freq");
        const auto hardware_max = readNumber(parent / "cpuinfo_max_freq");
        const auto active_min = readNumber(parent / "scaling_min_freq");
        const auto active_max = readNumber(parent / "scaling_max_freq");
        if (hardware_min && hardware_max && active_min && active_max && *hardware_min < *hardware_max) {
            const double lo = name == "scaling_min_freq" ? *hardware_min : *active_min;
            const double hi = name == "scaling_max_freq" ? *hardware_max : *active_max;
            if (lo < hi) return {lo, hi, true};
        }
    }
    if (name == "min_freq" || name == "max_freq") {
        const auto raw = [&]() -> std::string {
            std::string value;
            if (readResource(parent / "available_frequencies", value)) return value;
            return {};
        }();
        const auto values = tokens(raw);
        double lo = 0.0, hi = 0.0;
        bool valid = false;
        for (const auto& entry : values) {
            double frequency{};
            if (parseNumber(entry, frequency)) {
                if (!valid || frequency < lo) lo = frequency;
                if (!valid || frequency > hi) hi = frequency;
                valid = true;
            }
        }
        if (valid && lo < hi) return {lo, hi, true};
    }
    return {};
}

std::vector<fs::path> choicesFiles(const fs::path& path) {
    const auto name = path.filename().string();
    const auto parent = path.parent_path();
    if (name == "scaling_governor") return {parent / "scaling_available_governors"};
    if (name == "governor") return {parent / "available_governors"};
    if (name == "energy_performance_preference")
        return {parent / "energy_performance_available_preferences"};
    if (name == "scaling_min_freq" || name == "scaling_max_freq")
        return {parent / "scaling_available_frequencies"};
    if (name == "min_freq" || name == "max_freq")
        return {parent / "available_frequencies"};
    // Generic Linux drivers commonly expose one of these companion naming forms.
    return {parent / ("available_" + name), parent / (name + "_available"),
            parent / ("available_" + name + "s")};
}

std::vector<std::string> discoverChoices(const fs::path& path, const std::string& raw,
                                         std::string& active) {
    // Scheduler files publish both available choices and active selection using
    // [selected] token notation. Parse all values without hardcoding scheduler names.
    if (raw.find('[') != std::string::npos && raw.find(']') != std::string::npos) {
        auto values = tokens(raw);
        const auto left = raw.find('[');
        const auto right = raw.find(']', left == std::string::npos ? 0 : left + 1);
        if (left != std::string::npos && right != std::string::npos && right > left + 1)
            active = raw.substr(left + 1, right - left - 1);
        return values;
    }

    active = firstToken(raw);
    for (const auto& metadata_path : choicesFiles(path)) {
        std::string available_raw;
        if (!readResource(metadata_path, available_raw)) continue;
        auto values = tokens(available_raw);
        if (values.size() > 1 &&
            std::find(values.begin(), values.end(), active) != values.end()) return values;
    }
    return {};
}

bool supportsDiscreteStrategy(const std::string& domain, const std::string& attribute) {
    if (domain == "sysfs.block.queue" && attribute == "scheduler") return true;
    if (domain == "sysfs.cpu.cpufreq" &&
        (attribute == "scaling_governor" || attribute == "energy_performance_preference")) return true;
    if (domain == "sysfs.devfreq" && attribute == "governor") return true;
    return false;
}

void scanRoot(const fs::path& root, std::vector<Capability>& out) {
    std::error_code ec;
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) return;

    fs::recursive_directory_iterator it(
        root, fs::directory_options::skip_permission_denied, ec), end;
    for (; it != end; it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        const auto entry = *it;
        const auto link_status = entry.symlink_status(ec);
        if (ec) { ec.clear(); continue; }
        if (fs::is_directory(link_status)) {
            if (isPrunedDiscoverySubtree(entry.path().string())) it.disable_recursion_pending();
            continue;
        }
        if (fs::is_symlink(link_status) || !fs::is_regular_file(link_status)) continue;

        const auto path = entry.path();
        struct stat st{};
        if (::lstat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) continue;
        // Mode bits decide writability for the daemon's own permission class.
        // access() alone is insufficient because root passes it for read-only files.
        if (!hasWritePermissionBits(st.st_mode)) continue;
        if (::access(path.c_str(), R_OK | W_OK) != 0) continue;

        const auto domain = classifyKernelDomain(path.string(), activeGovernorFor(path));
        // Keep discovery broad across kernel namespaces while avoiding unrelated
        // regular files exposed by sysfs/procfs.
        if (domain.empty()) continue;

        std::string raw;
        if (!readResource(path, raw)) continue;

        Capability capability;
        capability.path = path.string();
        capability.id = capability.path;
        capability.attribute = path.filename().string();
        capability.domain = domain;
        capability.writable = true;  // Verified above from mode bits and access().
        capability.raw_value = firstToken(raw);
        capability.numeric = parseNumber(capability.raw_value, capability.numeric_value);

        std::string active;
        if (!capability.numeric) {
            capability.choices = discoverChoices(path, raw, active);
            if (capability.choices.size() > 1 &&
                std::find(capability.choices.begin(), capability.choices.end(), active) != capability.choices.end() &&
                supportsDiscreteStrategy(domain, capability.attribute)) {
                capability.raw_value = active;
                capability.strategy_id = "discrete." + domain + "." + capability.attribute;
                capability.optimization_supported = true;
            }
        }
        if (capability.numeric) {
            capability.range = discoverRange(path);
            capability.optimization_supported = capability.range.valid;
            // Frequency controls are discrete on most Android cpufreq/devfreq
            // drivers. Use the kernel's actual frequency set rather than writing
            // interpolated values which the driver may reject or round silently.
            if (capability.attribute == "scaling_min_freq" ||
                capability.attribute == "scaling_max_freq" ||
                capability.attribute == "min_freq" || capability.attribute == "max_freq") {
                std::string active;
                auto choices = discoverChoices(path, capability.raw_value, active);
                const auto active_min = readNumber(path.parent_path() /
                    (capability.attribute == "scaling_min_freq" || capability.attribute == "scaling_max_freq"
                        ? "scaling_min_freq" : "min_freq"));
                const auto active_max = readNumber(path.parent_path() /
                    (capability.attribute == "scaling_min_freq" || capability.attribute == "scaling_max_freq"
                        ? "scaling_max_freq" : "max_freq"));
                if (active_min && active_max && *active_min <= *active_max) {
                    choices.erase(std::remove_if(choices.begin(), choices.end(), [&](const std::string& item) {
                        double frequency{};
                        if (!parseNumber(item, frequency)) return true;
                        if (capability.attribute == "scaling_min_freq" || capability.attribute == "min_freq")
                            return frequency > *active_max;
                        return frequency < *active_min;
                    }), choices.end());
                }
                if (choices.size() > 1 &&
                    std::find(choices.begin(), choices.end(), capability.raw_value) != choices.end()) {
                    capability.choices = std::move(choices);
                    capability.strategy_id = "discrete-frequency." + domain + "." + capability.attribute;
                }
            }
        }
        out.push_back(std::move(capability));
    }
}

std::size_t applyStrategyCatalog(std::vector<Capability>& capabilities,
                                 const StrategyCatalog& catalog) {
    std::size_t eligible = 0;
    for (auto& capability : capabilities) {
        if (capability.domain == "sysfs.protected") {
            capability.optimization_supported = false;
            continue;
        }
        if (!capability.numeric || !capability.writable || capability.domain.empty() ||
            capability.attribute.empty()) {
            if (capability.optimization_supported) ++eligible;
            continue;
        }

        if (capability.choices.size() > 1 &&
            std::find(capability.choices.begin(), capability.choices.end(), capability.raw_value) !=
                capability.choices.end()) {
            capability.strategy_id = "kernel-choice." + capability.domain + "." + capability.attribute;
            capability.optimization_supported = true;
            ++eligible;
            continue;
        }

        const auto* rule = catalog.find(capability.domain, capability.attribute);
        if (rule && capability.numeric_value >= rule->minimum &&
            capability.numeric_value <= rule->maximum) {
            capability.range = {rule->minimum, rule->maximum, true};
            capability.strategy_id = rule->id;
            capability.optimization_supported = true;
        } else if ((capability.domain == "sysfs.block.queue" ||
                    capability.domain == "sysfs.cpu.cpufreq" ||
                    capability.domain == "sysfs.devfreq") &&
                   capability.range.valid &&
                   capability.range.minimum <= capability.numeric_value &&
                   capability.numeric_value <= capability.range.maximum) {
            // Only recognized performance subsystems may use metadata-derived
            // numeric ranges. A range on an unrelated sysfs file is not semantic
            // authorization; those resources remain observable but read-only.
            capability.strategy_id = "range-derived." + capability.domain + "." + capability.attribute;
            capability.optimization_supported = true;
        } else {
            capability.optimization_supported = false;
        }
        if (capability.optimization_supported) ++eligible;
    }
    return eligible;
}
}

std::vector<Capability> CapabilityDiscovery::discover() const {
    std::vector<Capability> result;
    scanRoot("/sys", result);
    scanRoot("/proc/sys", result);
    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) { return a.path < b.path; });
    result.erase(std::unique(result.begin(), result.end(),
        [](const auto& a, const auto& b) { return a.path == b.path; }), result.end());

    StrategyCatalog catalog;
    const char* configured_path = std::getenv("COREFLOW_STRATEGY_CATALOG");
    std::string error;
    if (configured_path && *configured_path && !catalog.load(configured_path, error)) {
        std::clog << "[CoreFlowEngine] Strategy extension rejected: " << error
                  << "; metadata-derived policies remain available\n";
    }

    const auto eligible = applyStrategyCatalog(result, catalog);
    std::clog << "[CoreFlowEngine] Discovered " << result.size()
              << " kernel controls; policy rules=" << catalog.size()
              << "; dynamically eligible mutation candidates=" << eligible << '\n';
    return result;
}
}
