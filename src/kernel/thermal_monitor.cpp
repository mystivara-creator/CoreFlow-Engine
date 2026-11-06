#include "coreflow/thermal_monitor.hpp"
#include "coreflow/config.hpp"
#include "coreflow/unique_fd.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <fcntl.h>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>
#include <chrono>
#ifdef __ANDROID__
#include <dlfcn.h>
#endif

namespace coreflow {
namespace {

bool read_sysfs_line(const std::string& path, std::string& out) {
    out.clear();
    if (path.empty() || path.find('\0') != std::string::npos) return false;
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;

    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) return false;

    char buf[128];
    ssize_t n = 0;
    do {
        n = ::read(fd.get(), buf, sizeof(buf));
    } while (n < 0 && errno == EINTR);
    if (n <= 0) return false;

    std::string_view view(buf, static_cast<size_t>(n));
    const size_t first = view.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return false;
    const size_t last = view.find_last_not_of(" \t\r\n");
    out.assign(view.substr(first, last - first + 1));
    return !out.empty();
}

bool is_trusted_compute_zone(std::string_view raw_type) {
    const size_t first = raw_type.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return false;
    const size_t last = raw_type.find_last_not_of(" \t\r\n");

    std::string type(raw_type.substr(first, last - first + 1));
    std::transform(type.begin(), type.end(), type.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    // Exclude battery state-of-charge droop ("socd") and non-compute peripherals/counters.
    if (type.find("socd") != std::string::npos ||
        type.find("batt") != std::string::npos ||
        type.find("chg") != std::string::npos ||
        type.find("usb") != std::string::npos ||
        type.find("bcl") != std::string::npos ||
        type.find("ibat") != std::string::npos ||
        type.find("sub1") != std::string::npos ||
        type.find("sdr") != std::string::npos ||
        type.find("mmw") != std::string::npos ||
        type.find("zeroc") != std::string::npos ||
        type.find("wireless") != std::string::npos) {
        return false;
    }

    if (type.find("cpu") != std::string::npos ||
        type.find("cpuss") != std::string::npos ||
        type.find("soc") != std::string::npos ||
        type.find("tsens") != std::string::npos ||
        type.find("apss") != std::string::npos ||
        type.find("cluster") != std::string::npos ||
        type.find("nspss") != std::string::npos ||
        type.find("npu") != std::string::npos ||
        type.rfind("aoss", 0) == 0) {
        return true;
    }

    const auto has_cluster_prefix = [&type](std::string_view prefix) {
        if (type.rfind(prefix, 0) != 0) return false;
        if (type.size() == prefix.size()) return true;
        const char next = type[prefix.size()];
        return next == '-' || next == '_' || (next >= '0' && next <= '9');
    };

    return has_cluster_prefix("big") ||
           has_cluster_prefix("mid") ||
           has_cluster_prefix("little");
}

bool parse_celsius(std::string_view text, float& celsius) noexcept {
    double raw = 0.0;
    if (!parse_ascii_double(text, raw)) return false;
    const double scaled = raw / 1000.0; // Linux thermal-zone ABI is millidegrees Celsius.
    if (!std::isfinite(scaled) || scaled < 0.0 || scaled > 130.0) return false;
    celsius = static_cast<float>(scaled);
    return true;
}

} // namespace

ThermalMonitor::ThermalMonitor(std::string_view sysfs_root)
    : sysfs_root_(sysfs_root.empty() ? "/sys/class/thermal" : std::string(sysfs_root)) {}

ThermalMonitor& ThermalMonitor::instance() {
    static ThermalMonitor monitor;
    return monitor;
}

int classify_thermal_type(std::string_view type) {
    std::string lower(type);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    const bool is_gpu_zone =
        lower.find("gpu") != std::string::npos || lower.find("gpuss") != std::string::npos ||
        lower.find("kgsl") != std::string::npos || lower.find("adreno") != std::string::npos ||
        lower.find("mali") != std::string::npos || lower.find("xclipse") != std::string::npos ||
        lower.find("sgpu") != std::string::npos || lower.find("g3d") != std::string::npos;
    if (is_trusted_compute_zone(type) || is_gpu_zone) {
        return 1;
    }
    if (lower == "battery" || lower == "batt" || lower == "bms" ||
        lower.rfind("battery-", 0) == 0 || lower.rfind("battery_", 0) == 0 ||
        lower.find("batt_therm") != std::string::npos) {
        return 2;
    }
    if (lower == "skin" || lower.rfind("skin-", 0) == 0 || lower.rfind("skin_", 0) == 0 ||
        lower.find("quiet_therm") != std::string::npos || lower.find("case_therm") != std::string::npos ||
        lower.find("shell_therm") != std::string::npos || lower.find("chg_skin") != std::string::npos ||
        lower.find(" surface") != std::string::npos || lower.find("virtual-skin") != std::string::npos) {
        return 3;
    }
    if (lower.find("ap_therm") != std::string::npos || lower.find("xo-therm") != std::string::npos ||
        lower.find("xo_therm") != std::string::npos || lower.rfind("aoss", 0) == 0 ||
        lower == "ddr" || lower == "video" || lower.find("pmic") != std::string::npos ||
        lower.find("pm6150") != std::string::npos || lower.find("pm7250") != std::string::npos) {
        return 4;
    }
    return 0;
}

bool is_excluded_non_thermal_peripheral(std::string_view raw_type) {
    std::string lower(raw_type);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lower.find("socd") != std::string::npos ||
           lower.find("bcl") != std::string::npos ||
           lower.find("ibat") != std::string::npos ||
           lower.find("zeroc") != std::string::npos;
}

void ThermalMonitor::discover_sensors() const {
    const auto now = std::chrono::steady_clock::now();
    if (!sensors_.empty() && now - discovery_at_ < std::chrono::seconds(30)) return;
    discovery_at_ = now;
    sensors_.clear();
    size_t visited = 0;
    std::error_code ec;
    namespace fs = std::filesystem;

    for (fs::directory_iterator it(sysfs_root_, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && sensors_.size() < 512 && visited++ < 2048; it.increment(ec)) {
        if (it->path().filename().string().rfind("thermal_zone", 0) != 0) continue;
        std::string type;
        if (!read_sysfs_line(it->path().string() + "/type", type)) continue;
        const int category = classify_thermal_type(type);
        if (category > 0) {
            sensors_.push_back({
                it->path().string(),
                it->path().string() + "/temp",
                type,
                category,
                false,
            });
        }
    }

    // Secondary discovery of /sys/class/hwmon/hwmon* (temp*_input + temp*_label / name)
    // when not already represented in thermal_zone*.
    std::vector<fs::path> hwmon_roots;
    const fs::path base_path(sysfs_root_);
    if (fs::is_directory(base_path / "hwmon", ec) && !ec) {
        hwmon_roots.push_back(base_path / "hwmon");
    }
    if (base_path.filename() == "thermal" &&
        fs::is_directory(base_path.parent_path() / "hwmon", ec) && !ec) {
        hwmon_roots.push_back(base_path.parent_path() / "hwmon");
    }

    for (const auto& hw_root : hwmon_roots) {
        std::error_code hw_ec;
        for (fs::directory_iterator hit(hw_root, fs::directory_options::skip_permission_denied, hw_ec), hend;
             !hw_ec && hit != hend && sensors_.size() < 512; hit.increment(hw_ec)) {
            const std::string dname = hit->path().filename().string();
            if (dname.rfind("hwmon", 0) != 0) continue;

            std::string chip_name;
            (void)read_sysfs_line((hit->path() / "name").string(), chip_name);

            std::error_code f_ec;
            for (fs::directory_iterator fit(hit->path(), fs::directory_options::skip_permission_denied, f_ec), fend;
                 !f_ec && fit != fend && sensors_.size() < 512; fit.increment(f_ec)) {
                const std::string fname = fit->path().filename().string();
                if (fname.rfind("temp", 0) != 0 || !fname.ends_with("_input")) continue;
                const std::string prefix = fname.substr(0, fname.size() - 6); // "tempN"
                std::string label;
                if (!read_sysfs_line((hit->path() / (prefix + "_label")).string(), label)) {
                    label = chip_name.empty() ? (dname + "_" + prefix) : chip_name;
                }
                if (is_excluded_non_thermal_peripheral(label)) continue;

                // Skip if an identical sensor type is already present from thermal_zone*
                const bool duplicate = std::any_of(sensors_.begin(), sensors_.end(), [&](const Sensor& s) {
                    return !s.is_hwmon && s.type == label;
                });
                if (duplicate) continue;

                const int category = classify_thermal_type(label);
                // Keep classified sensors (category 1..4) and diagnostic hwmon sensors (category 0)
                sensors_.push_back({
                    hit->path().string(),
                    fit->path().string(),
                    label,
                    category,
                    true,
                });
            }
        }
    }
}

float ThermalMonitor::hottest_category(int category) const {
    discover_sensors();
    float hottest = -1;
    for (const auto& sensor : sensors_) {
        if (sensor.category != category) continue;
        std::string raw; float value;
        if (read_sysfs_line(sensor.temp_path, raw) && parse_celsius(raw, value)) hottest = std::max(hottest, value);
    }
    return hottest;
}

MultiSensorProfile ThermalMonitor::multi_sensor_profile() const {
    discover_sensors();
    MultiSensorProfile profile;
    std::vector<float> cpu_readings;
    std::vector<float> gpu_readings;
    std::vector<float> board_readings;
    float max_cpu = -1.0f;
    float max_temp = -1.0f;

    for (const auto& sensor : sensors_) {
        // Category 0 (unknown/diagnostic) is never mixed into compute/board composite temperatures
        if (sensor.category <= 0) continue;
        std::string raw;
        float value = -1.0f;
        if (!read_sysfs_line(sensor.temp_path, raw) || !parse_celsius(raw, value)) continue;
        if (value <= 0.0f) continue;

        max_temp = std::max(max_temp, value);
        std::string lower = sensor.type;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (sensor.category == 1) { // Compute
            if (lower.find("gpu") != std::string::npos || lower.find("gpuss") != std::string::npos ||
                lower.find("kgsl") != std::string::npos || lower.find("adreno") != std::string::npos ||
                lower.find("mali") != std::string::npos || lower.find("xclipse") != std::string::npos ||
                lower.find("sgpu") != std::string::npos || lower.find("g3d") != std::string::npos) {
                gpu_readings.push_back(value);
            } else if (lower.find("cpu") != std::string::npos || lower.find("apss") != std::string::npos ||
                       lower.find("cluster") != std::string::npos || lower.find("tsens") != std::string::npos ||
                       lower == "big" || lower == "mid" || lower == "little") {
                cpu_readings.push_back(value);
                max_cpu = std::max(max_cpu, value);
            } else {
                board_readings.push_back(value);
            }
        } else if (sensor.category == 2) { // Battery
            if (profile.battery < 0 || value > profile.battery) profile.battery = value;
        } else if (sensor.category == 3) { // Skin
            if (profile.skin < 0 || value > profile.skin) profile.skin = value;
        } else if (sensor.category == 4) { // Board / AP
            board_readings.push_back(value);
        }
    }

    if (profile.battery < 0.0f) {
        profile.battery = battery_celsius();
    }

    profile.total_valid_sensors = cpu_readings.size() + gpu_readings.size() + board_readings.size();
    profile.valid_cpu_count = cpu_readings.size();
    profile.valid_gpu_count = gpu_readings.size();

    if (!cpu_readings.empty()) {
        double sum = 0.0;
        for (float v : cpu_readings) sum += v;
        profile.cpu_average = static_cast<float>(sum / cpu_readings.size());
        profile.cpu_peak = max_cpu;
    }
    if (!gpu_readings.empty()) {
        double sum = 0.0;
        for (float v : gpu_readings) sum += v;
        profile.gpu_average = static_cast<float>(sum / gpu_readings.size());
    }
    if (!board_readings.empty()) {
        double sum = 0.0;
        for (float v : board_readings) sum += v;
        profile.board_average = static_cast<float>(sum / board_readings.size());
    }

    // Accurate multi-sensor composite calculation
    if (profile.cpu_average >= 0.0f) {
        if (profile.gpu_average >= 0.0f && profile.board_average >= 0.0f) {
            profile.composite_soc = 0.50f * profile.cpu_average + 0.25f * profile.cpu_peak +
                                    0.15f * profile.gpu_average + 0.10f * profile.board_average;
        } else if (profile.gpu_average >= 0.0f) {
            profile.composite_soc = 0.60f * profile.cpu_average + 0.25f * profile.cpu_peak +
                                    0.15f * profile.gpu_average;
        } else {
            profile.composite_soc = 0.70f * profile.cpu_average + 0.30f * profile.cpu_peak;
        }
    } else if (max_temp >= 0.0f) {
        profile.composite_soc = max_temp;
    }

    return profile;
}

float ThermalMonitor::composite_celsius() const {
    const auto prof = multi_sensor_profile();
    return prof.composite_soc >= 0.0f ? prof.composite_soc : hottest_celsius();
}

float ThermalMonitor::hottest_celsius() const { return hottest_category(1); }
float ThermalMonitor::battery_celsius() const {
    const float tz_batt = hottest_category(2);
    if (tz_batt >= 0.0f) return tz_batt;
    if (sysfs_root_ == "/sys/class/thermal") {
        std::string raw;
        double tenths = 0.0;
        if (read_sysfs_line("/sys/class/power_supply/battery/temp", raw) &&
            parse_ascii_double(raw, tenths) && tenths > 0.0 && tenths <= 800.0) {
            return static_cast<float>(tenths / 10.0);
        }
    }
    return -1.0f;
}
float ThermalMonitor::skin_celsius() const { return hottest_category(3); }

ThermalAssessment assessment_from_android_severity(int severity) {
    if (severity < 0 || severity > 6) return {};
    constexpr double indices[] = {0, 40, 75, 95, 110, 120, 130};
    ThermalAssessment out{};
    out.available = true;
    out.emergency = (severity >= 5);
    out.pressure = indices[severity];
    out.severity = severity;
    out.source = "android-thermal";
    out.sensor = "OEM severity";
    out.platform_status_valid = true;
    out.sensor_readings_complete = true;
    return out;
}

ThermalAssessment apply_android_headroom(ThermalAssessment base, float headroom) {
    if (!std::isfinite(headroom) || headroom < 0.0f || headroom > 10.0f) {
        return base;
    }
    base.headroom_valid = true;
    base.headroom_value = headroom;
    // Android NDK: 1.0 corresponds to THERMAL_STATUS_SEVERE (pressure 95.0).
    // Values > 1.0 have no defined linear mapping to CRITICAL/EMERGENCY/SHUTDOWN,
    // so headroom-derived pressure is capped at SEVERE (95.0) and never sets emergency on its own.
    const double clamped_ratio = std::clamp(static_cast<double>(headroom), 0.0, 1.0);
    const double headroom_pressure = (headroom >= 1.0f) ? 95.0 : (clamped_ratio * 85.0);
    base.pressure = std::max(base.pressure, headroom_pressure);
    if (!base.available) {
        base.available = true;
        base.source = "android-headroom";
        base.sensor = "OEM headroom";
    }
    return base;
}

ThermalAssessment fuse_thermal_assessments(ThermalAssessment platform, ThermalAssessment sensors) {
    auto result = platform;
    result.platform_status_valid = platform.available || platform.platform_status_valid;
    result.sysfs_trip_policy_valid = sensors.available || sensors.sysfs_trip_policy_valid;
    result.sensor_readings_complete = !sensors.incomplete && sensors.sensor_readings_complete;
    result.incomplete = sensors.incomplete;
    result.degraded = sensors.incomplete || sensors.degraded;

    if ((sensors.available || sensors.sysfs_trip_policy_valid) &&
        (!result.available || sensors.pressure > result.pressure)) {
        result.pressure = sensors.pressure;
        if (!sensors.sensor.empty()) {
            result.sensor = sensors.sensor;
        }
    }
    result.emergency = platform.emergency || sensors.emergency;
    // Never discard a valid Android platform thermal status when a sysfs sensor is incomplete;
    // if only sysfs policy is present and incomplete, mark degraded while preserving available
    // when either platform status or emergency or valid complete sysfs trip policy is present.
    result.available = result.platform_status_valid || result.emergency ||
                       (sensors.available && !sensors.incomplete);
    if (result.platform_status_valid && (result.sysfs_trip_policy_valid || result.degraded)) {
        result.source = result.degraded ? "android+sysfs-degraded" : "android+sysfs-oem-trips";
    } else if (result.platform_status_valid) {
        result.source = platform.source;
    } else {
        result.source = sensors.source;
    }
    result.readings = std::move(sensors.readings);
    return result;
}

ThermalAssessment ThermalMonitor::assess(bool use_headroom) const {
    ThermalAssessment platform;

#ifdef __ANDROID__
    if (sysfs_root_ == "/sys/class/thermal") {
        // Public NDK thermal API, resolved optionally for standalone root daemons.
        // A severity is OEM policy, not a universal absolute temperature.
        struct AThermalManager;
        using Acquire = AThermalManager* (*)();
        using Status = int (*)(AThermalManager*);
        using Headroom = float (*)(AThermalManager*, int);
        static void* library = ::dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
        static auto acquire = library ? reinterpret_cast<Acquire>(::dlsym(library, "AThermal_acquireManager")) : nullptr;
        static auto status = library ? reinterpret_cast<Status>(::dlsym(library, "AThermal_getCurrentThermalStatus")) : nullptr;
        static auto headroom = library ? reinterpret_cast<Headroom>(::dlsym(library, "AThermal_getThermalHeadroom")) : nullptr;
        static auto manager = acquire && status ? acquire() : nullptr;
        if (manager) {
            const int severity = status(manager);
            if (severity >= 0 && severity <= 6) {
                auto result = assessment_from_android_severity(severity);
                if (use_headroom && headroom) {
                    static auto last = std::chrono::steady_clock::time_point::min();
                    static float cached = -1;
                    const auto now = std::chrono::steady_clock::now();
                    if (last == std::chrono::steady_clock::time_point::min() || now - last >= std::chrono::seconds(10)) {
                        cached = headroom(manager, 0); last = now;
                    }
                    result = apply_android_headroom(std::move(result), cached);
                }
                platform = result;
            }
        }
    }
#else
    (void)use_headroom;
#endif
    discover_sensors();
    ThermalAssessment result;
    for (const auto& sensor : sensors_) {
        std::string raw; float temp;
        const bool valid_temperature = read_sysfs_line(sensor.temp_path, raw) && parse_celsius(raw, temp);
        float passive = -1, critical = -1, hot = -1, hyst = 0;
        // Never assume a global degree threshold or reinterpret active cooling trips.
        if (!sensor.is_hwmon) {
            std::error_code ec;
            size_t visited = 0;
            for (std::filesystem::directory_iterator it(sensor.path, ec), end;
                 !ec && it != end && visited++ < 256; it.increment(ec)) {
                const auto name = it->path().filename().string();
                if (name.rfind("trip_point_", 0) != 0 || !name.ends_with("_type")) continue;
                const auto prefix = sensor.path + "/" + name.substr(0, name.size() - 5);
                std::string type; float value;
                if (!read_sysfs_line(prefix + "_type", type) || !read_sysfs_line(prefix + "_temp", raw) ||
                    !parse_celsius(raw, value) || value <= 0) continue;
                if (type == "passive" && (passive < 0 || value < passive)) {
                    passive = value; float h = 0;
                    hyst = read_sysfs_line(prefix + "_hyst", raw) && parse_celsius(raw, h) ? h : 0;
                }
                if (type == "hot" && (hot < 0 || value < hot)) hot = value;
                if (type == "critical" && (critical < 0 || value < critical)) critical = value;
            }
        }
        ThermalSensorReading reading{sensor.path, sensor.type, sensor.category,
            valid_temperature ? temp : -1, valid_temperature, passive > 0 || hot > 0 || critical > 0, 0};
        result.readings.push_back(reading);
        auto& observed = result.readings.back();
        if (!valid_temperature) {
            if (observed.policy_available) {
                result.incomplete = true;
                result.degraded = true;
                result.sensor_readings_complete = false;
            }
            continue;
        }
        if (sensor.category <= 0) {
            // Diagnostic-only sensor (category 0): record reading without driving compute policy
            continue;
        }
        if (critical > 0 && temp >= critical) {
            result.emergency = true; result.available = true; result.sysfs_trip_policy_valid = true;
            result.pressure = 130;
            result.source = "sysfs-oem-trips"; result.sensor = sensor.type;
            observed.pressure = 130;
            continue; // Continue reading the remaining sensors, even during emergency.
        }
        if (hot > 0 && temp >= hot) {
            observed.pressure = 110;
            if (!result.available || result.pressure < 110) {
                result.available = true; result.sysfs_trip_policy_valid = true;
                result.pressure = 110;
                result.source = "sysfs-oem-trips"; result.sensor = sensor.type;
            }
        }
        if (passive <= 0 || (critical > 0 && passive >= critical)) continue;
        const double margin = std::clamp(static_cast<double>(passive) * 0.05, 1.0, 5.0);
        // Normalize each sensor against its own OEM trip. Different sensor classes
        // are compared only as policy-relative pressure, never as raw Celsius.
        const double recovery = passive - std::max(margin, static_cast<double>(hyst));
        if (recovery < 0 || recovery >= passive) continue;
        const double pressure = std::clamp(65.0 + (temp - recovery) / (passive - recovery) * 20.0, 0.0, 130.0);
        observed.pressure = std::max(observed.pressure, pressure);
        if (!result.available || pressure > result.pressure) {
            result.available = true; result.sysfs_trip_policy_valid = true;
            result.pressure = pressure;
            result.source = "sysfs-oem-trips"; result.sensor = sensor.type;
        }
    }
    auto fused = fuse_thermal_assessments(std::move(platform), std::move(result));
    const float comp = composite_celsius();
    if (comp >= 0.0f) {
        const auto now_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        record_temperature_sample(comp, now_ns);
    }
    fused.temperature_velocity = current_temperature_velocity();
    return fused;
}

TemperatureTrendTracker::TemperatureTrendTracker(size_t capacity) noexcept
    : capacity_(std::clamp<size_t>(capacity, 3, kMaxSamples)) {}

void TemperatureTrendTracker::record_sample(float celsius, uint64_t timestamp_ns) noexcept {
    if (!std::isfinite(celsius) || celsius < 0.0f || celsius > 130.0f || timestamp_ns == 0) {
        return;
    }

    if (count_ > 0) {
        const uint64_t prev_ts = samples_[count_ - 1].timestamp_ns;
        if (timestamp_ns <= prev_ts) {
            return;
        }
        // If elapsed time since last sample exceeds 10 seconds (deep sleep, pause, or stall),
        // reset history so stale measurements are never bridged.
        if (timestamp_ns - prev_ts > 10000000000ULL) {
            reset();
        }
    }

    if (count_ >= capacity_) {
        for (size_t i = 1; i < count_; ++i) {
            samples_[i - 1] = samples_[i];
        }
        --count_;
    }

    samples_[count_++] = TemperatureTrendSample{celsius, timestamp_ns};
}

TemperatureVelocityReading TemperatureTrendTracker::evaluate_velocity() const noexcept {
    TemperatureVelocityReading out{};
    if (count_ < 2) {
        out.valid = false;
        out.valid_samples_count = static_cast<uint32_t>(count_);
        if (count_ == 1) {
            out.current_temperature_celsius = samples_[0].celsius;
            out.timestamp_ns = samples_[0].timestamp_ns;
        }
        return out;
    }

    const uint64_t t0 = samples_[0].timestamp_ns;
    const uint64_t t_last = samples_[count_ - 1].timestamp_ns;
    const double duration_sec = static_cast<double>(t_last - t0) / 1e9;

    out.sample_duration_sec = duration_sec;
    out.valid_samples_count = static_cast<uint32_t>(count_);
    out.current_temperature_celsius = samples_[count_ - 1].celsius;
    out.timestamp_ns = t_last;

    if (duration_sec < 0.8 || count_ < 2) {
        out.valid = false;
        return out;
    }

    double sum_t = 0.0;
    double sum_temp = 0.0;
    for (size_t i = 0; i < count_; ++i) {
        const double t_sec = static_cast<double>(samples_[i].timestamp_ns - t0) / 1e9;
        sum_t += t_sec;
        sum_temp += static_cast<double>(samples_[i].celsius);
    }
    const double mean_t = sum_t / static_cast<double>(count_);
    const double mean_temp = sum_temp / static_cast<double>(count_);

    double s_tt = 0.0;
    double s_t_temp = 0.0;
    for (size_t i = 0; i < count_; ++i) {
        const double t_sec = static_cast<double>(samples_[i].timestamp_ns - t0) / 1e9;
        const double dt = t_sec - mean_t;
        const double d_temp = static_cast<double>(samples_[i].celsius) - mean_temp;
        s_tt += dt * dt;
        s_t_temp += dt * d_temp;
    }

    if (s_tt < 1e-7) {
        out.valid = false;
        return out;
    }

    out.velocity_celsius_per_sec = s_t_temp / s_tt;
    out.valid = true;
    return out;
}

void TemperatureTrendTracker::reset() noexcept {
    count_ = 0;
}

TemperatureVelocityReading ThermalMonitor::current_temperature_velocity() const {
    return trend_tracker_.evaluate_velocity();
}

void ThermalMonitor::record_temperature_sample(float celsius, uint64_t timestamp_ns) const {
    trend_tracker_.record_sample(celsius, timestamp_ns);
}

} // namespace coreflow

