#include "coreflow/observation/observation.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <sys/utsname.h>
#include <unistd.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include "coreflow/observation/thermal_policy.hpp"
#include "coreflow/observation/pressure.hpp"

namespace fs = std::filesystem;
namespace coreflow {
namespace {
bool readFirst(const std::string& path, std::string& value) {
    std::ifstream f(path);
    return static_cast<bool>(f >> value);
}
std::string canonicalValue(std::string raw, const std::vector<std::string>& choices) {
    if (choices.empty()) return raw;
    const auto left = raw.find('[');
    if (left != std::string::npos) {
        const auto right = raw.find(']', left + 1);
        if (right != std::string::npos && right > left + 1) {
            const auto active = raw.substr(left + 1, right - left - 1);
            if (std::find(choices.begin(), choices.end(), active) != choices.end()) return active;
        }
    }
    std::istringstream input(raw);
    std::string first;
    input >> first;
    return std::find(choices.begin(), choices.end(), first) != choices.end() ? first : raw;
}

std::string trim(std::string value) {
    value.erase(std::remove(value.begin(), value.end(), '\0'), value.end());
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}
std::string readText(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};

    constexpr std::size_t kMaximumBytes = 4096;
    std::string result;
    result.reserve(256);
    char chunk[256];
    while (in) {
        in.read(chunk, sizeof(chunk));
        const auto count = in.gcount();
        if (count > 0) {
            const auto amount = static_cast<std::size_t>(count);
            if (amount > kMaximumBytes - result.size()) return {};
            result.append(chunk, amount);
        }
    }
    if (in.bad()) return {};
    return trim(std::move(result));
}
void loadAndroidProperties(const fs::path& path, std::map<std::string, std::string>& properties) {
    std::ifstream input(path);
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line.front() == '#') continue;
        const auto equal = line.find('=');
        if (equal == std::string::npos || equal == 0) continue;
        auto key = trim(line.substr(0, equal));
        auto value = trim(line.substr(equal + 1));
        if (!key.empty() && !value.empty() && !properties.count(key))
            properties.emplace(std::move(key), std::move(value));
    }
}
std::string property(const std::map<std::string, std::string>& props,
                     const std::string& key) {
    const auto it = props.find(key);
    return it == props.end() ? std::string{} : it->second;
}

std::string detectStorageType() {
    // Do not infer UFS/eMMC from model names. Prefer controller identity and
    // kernel-exported block metadata; retain an explicit unknown/fallback label.
    std::error_code ec;
    const fs::path hosts("/sys/class/scsi_host");
    for (fs::directory_iterator it(hosts, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end; it.increment(ec)) {
        const auto driver = readText(it->path() / "proc_name");
        if (driver.find("ufshcd") != std::string::npos || driver.find("ufs") != std::string::npos)
            return "UFS";
    }

    bool nonrotational = false;
    bool rotational = false;
    bool emmc = false;
    const fs::path blocks("/sys/block");
    ec.clear();
    for (fs::directory_iterator it(blocks, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end; it.increment(ec)) {
        const auto name = it->path().filename().string();
        if (name.rfind("loop", 0) == 0 || name.rfind("ram", 0) == 0 ||
            name.rfind("zram", 0) == 0 || name.rfind("dm-", 0) == 0) continue;
        const auto resolved = fs::canonical(it->path(), ec).string();
        ec.clear();
        if (resolved.find("ufshc") != std::string::npos || resolved.find("ufshcd") != std::string::npos)
            return "UFS";
        const auto device_type = readText(it->path() / "device" / "type");
        if (name.rfind("mmcblk", 0) == 0 && (device_type == "MMC" || device_type == "mmc")) emmc = true;
        const auto rotational_value = readText(it->path() / "queue" / "rotational");
        if (rotational_value == "0") nonrotational = true;
        else if (rotational_value == "1") rotational = true;
    }
    if (emmc) return "eMMC";
    if (nonrotational) return "non-rotational flash (controller unknown)";
    if (rotational) return "rotational block storage";
    return "unknown";
}
DeviceContext discoverDeviceContext() {
    static const DeviceContext cached = [] {
        DeviceContext context;
        std::map<std::string, std::string> props;
        loadAndroidProperties("/system/build.prop", props);
        loadAndroidProperties("/system/etc/prop.default", props);
        loadAndroidProperties("/system_ext/build.prop", props);
        loadAndroidProperties("/vendor/build.prop", props);
        loadAndroidProperties("/vendor/default.prop", props);
        loadAndroidProperties("/product/build.prop", props);
        loadAndroidProperties("/product/etc/build.prop", props);
        loadAndroidProperties("/odm/build.prop", props);
        context.manufacturer = property(props, "ro.product.manufacturer");
        context.model = property(props, "ro.product.model");
        context.device_codename = property(props, "ro.product.device");
        context.soc_model = property(props, "ro.soc.model");
        if (context.soc_model.empty()) context.soc_model = property(props, "ro.board.platform");
        context.android_release = property(props, "ro.build.version.release");
        context.android_api = property(props, "ro.build.version.sdk");
        if (context.soc_model.empty()) context.soc_model = readText("/sys/devices/soc0/machine");
        if (context.soc_model.empty()) context.soc_model = readText("/sys/devices/soc0/soc_id");
        if (context.model.empty()) context.model = readText("/proc/device-tree/model");
        struct utsname kernel{};
        if (::uname(&kernel) == 0) context.kernel_release = kernel.release;
        const long cpus = ::sysconf(_SC_NPROCESSORS_ONLN);
        if (cpus > 0) context.online_cpu_count = static_cast<std::size_t>(cpus);
        std::ifstream mem("/proc/meminfo");
        std::string key, unit;
        std::uint64_t amount{};
        while (mem >> key >> amount >> unit) {
            if (key == "MemTotal:") {
                context.total_memory_kb = amount;
                break;
            }
        }
        context.storage_type = detectStorageType();
        return context;
    }();
    return cached;
}

// Reads trip_point_N_type/temp pairs for one compute zone. Trip indices are
// contiguous in the kernel, so the first missing type file ends the scan.
bool passiveHeadroomForZone(const fs::path& zone, double celsius, double& headroom) {
    constexpr int kMaximumTripPoints = 32;
    bool found = false;
    double best = 0.0;
    for (int index = 0; index < kMaximumTripPoints; ++index) {
        const std::string base = "trip_point_" + std::to_string(index);
        std::string type;
        if (!readFirst((zone / (base + "_type")).string(), type)) break;
        if (!observation::thermal_policy::isPassiveTripType(type)) continue;
        std::string raw;
        double trip{};
        if (!readFirst((zone / (base + "_temp")).string(), raw) ||
            !observation::thermal_policy::parseTripCelsius(raw, trip)) continue;
        const double room = trip - celsius;
        if (!found || room < best) {
            best = room;
            found = true;
        }
    }
    if (found) headroom = best;
    return found;
}
bool readWhole(const std::string& path, std::string& value) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    constexpr std::size_t kMaximumBytes = 2048;
    std::string buffer;
    buffer.reserve(256);
    char chunk[256];
    while (in) {
        in.read(chunk, sizeof(chunk));
        const auto count = in.gcount();
        if (count > 0) {
            const auto amount = static_cast<std::size_t>(count);
            if (amount > kMaximumBytes - buffer.size()) return false;
            buffer.append(chunk, amount);
        }
    }
    if (in.bad()) return false;

    const auto first = buffer.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return false;
    const auto last = buffer.find_last_not_of(" \t\r\n");
    value = buffer.substr(first, last - first + 1);
    return true;
}
}

SystemObservation RuntimeObserver::observeSystem() const {
    SystemObservation o;
    {
        // Serialize the read and baseline update together. Otherwise concurrent
        // callers can apply an older /proc/stat sample after a newer one.
        std::lock_guard<std::mutex> lock(cpu_sample_mutex_);

        std::ifstream stat("/proc/stat");
        std::string label;
        unsigned long long user = 0, nice = 0, system = 0, idle = 0;
        unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0;

        if (stat >> label >> user >> nice >> system >> idle >>
            iowait >> irq >> softirq >> steal) {
            constexpr auto max_value =
                std::numeric_limits<unsigned long long>::max();
            const unsigned long long counters[] = {
                user, nice, system, idle, iowait, irq, softirq, steal
            };

            unsigned long long total = 0;
            bool valid_total = true;
            for (const auto counter : counters) {
                if (counter > max_value - total) {
                    valid_total = false;
                    break;
                }
                total += counter;
            }

            if (valid_total && iowait <= max_value - idle) {
                const auto idle_all = idle + iowait;

                // Invalid samples do not poison the previous valid baseline.
                if (previous_cpu_total_ != 0 &&
                    total > previous_cpu_total_ &&
                    idle_all >= previous_cpu_idle_) {
                    const auto total_delta = total - previous_cpu_total_;
                    const auto idle_delta = idle_all - previous_cpu_idle_;

                    if (total_delta > 0 && idle_delta <= total_delta) {
                        o.cpu_busy_percent = 100.0 *
                            static_cast<double>(total_delta - idle_delta) /
                            static_cast<double>(total_delta);
                        o.cpu_valid = std::isfinite(o.cpu_busy_percent) &&
                            o.cpu_busy_percent >= 0.0 &&
                            o.cpu_busy_percent <= 100.0;
                    }
                }

                previous_cpu_total_ = total;
                previous_cpu_idle_ = idle_all;
            }
        }
    }
    std::ifstream mem("/proc/meminfo");
    std::string key, unit;
    double total_kb = 0.0;
    double available_kb = 0.0;
    double value = 0.0;
    bool found_total = false;
    bool found_available = false;

    while (mem >> key >> value >> unit) {
        if (!std::isfinite(value) || value < 0.0) continue;

        if (key == "MemTotal:") {
            total_kb = value;
            found_total = true;
        } else if (key == "MemAvailable:") {
            available_kb = value;
            found_available = true;
        }
    }

    if (found_total && found_available &&
        total_kb > 0.0 && available_kb >= 0.0) {
        o.memory_available_ratio =
            std::clamp(available_kb / total_kb, 0.0, 1.0);
        o.memory_valid = true;
    }
    // Only consume recognized compute-related zones with plausible
    // millidegree-Celsius values. Ignore battery, modem, BCL, level indicators,
    // sentinels, and unknown zone types rather than guessing their semantics.
    const fs::path thermal_root("/sys/class/thermal");
    std::error_code ec;
    if (fs::exists(thermal_root, ec)) {
        for (fs::directory_iterator it(
                 thermal_root, fs::directory_options::skip_permission_denied, ec),
             end; it != end; it.increment(ec)) {
            if (ec) {
                ec.clear();
                continue;
            }
            if (!it->is_directory(ec)) continue;

            std::string type;
            std::string raw;
            double celsius{};

            if (!readFirst((it->path() / "type").string(), type) ||
                !readFirst((it->path() / "temp").string(), raw) ||
                !observation::thermal_policy::parseTrustedThermalCelsius(type, raw, celsius)) {
                continue;
            }

            if (!o.temperature_valid ||
                celsius > o.maximum_temperature_c) {
                o.maximum_temperature_c = celsius;
            }
            o.temperature_valid = true;

            // The hottest zone with a published passive trip defines headroom.
            // Zones without one contribute nothing rather than a guessed limit.
            double zone_headroom{};
            if (passiveHeadroomForZone(it->path(), celsius, zone_headroom) &&
                (!o.thermal_headroom_valid || zone_headroom < o.thermal_headroom_c)) {
                o.thermal_headroom_c = zone_headroom;
                o.thermal_headroom_valid = true;
            }
        }
    }
    const auto readPressure = [](const char* path, double& avg10) {
        std::ifstream input(path, std::ios::binary);
        if (!input) return false;

        constexpr std::size_t kMaximumBytes = observation::pressure::kMaximumPressureTextBytes;
        std::string snapshot;
        snapshot.reserve(512);
        char chunk[256];
        while (input) {
            input.read(chunk, sizeof(chunk));
            const auto count = input.gcount();
            if (count > 0) {
                const auto amount = static_cast<std::size_t>(count);
                if (amount > kMaximumBytes - snapshot.size()) return false;
                snapshot.append(chunk, amount);
            }
        }
        if (input.bad()) return false;
        return observation::pressure::parseSomeAvg10(snapshot, avg10);
    };
    o.cpu_pressure_valid = readPressure("/proc/pressure/cpu", o.cpu_pressure_some_avg10);
    o.memory_pressure_valid = readPressure("/proc/pressure/memory", o.memory_pressure_some_avg10);
    o.io_pressure_valid = readPressure("/proc/pressure/io", o.io_pressure_some_avg10);

    // SurfaceFlinger metrics are opportunistic and strictly read-only. When the
    // foreground package cannot be correlated with available timestats, the
    // frame signal remains invalid and cannot authorize a tuning mutation.
    const auto frames = surface_flinger_.observe();
    if (frames.valid) {
        o.frame_average_fps = frames.average_fps;
        o.frame_p95_interval_ms = frames.p95_frame_interval_ms;
        o.frame_p99_interval_ms = frames.p99_frame_interval_ms;
        o.slow_frame_ratio = frames.slow_frame_ratio;
        o.frame_package = frames.package_name;
        o.frame_timing_valid = true;
    }
    return o;
}
ObservationFrame RuntimeObserver::observe(const std::vector<Capability>& capabilities) const {
    ObservationFrame f;
    f.system=observeSystem();
    f.device=discoverDeviceContext();
    f.capability_values.reserve(capabilities.size());
    for (const auto& c: capabilities) {
        if (!c.optimization_supported || !c.writable) continue;
        if (c.numeric && !c.range.valid) continue;
        if (!c.numeric && c.choices.empty()) continue;
        std::string raw;
        if (c.choices.empty()) {
            if (readFirst(c.path, raw)) f.capability_values.emplace_back(c.id, raw);
        } else if (readWhole(c.path, raw)) {
            f.capability_values.emplace_back(c.id, canonicalValue(raw, c.choices));
        }
    }
    return f;
}
}
