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

    // Exclude battery state-of-charge droop ("socd") and non-SoC peripherals.
    if (type.find("socd") != std::string::npos ||
        type.find("batt") != std::string::npos ||
        type.find("chg") != std::string::npos ||
        type.find("usb") != std::string::npos) {
        return false;
    }

    if (type.find("cpu") != std::string::npos ||
        type.find("soc") != std::string::npos ||
        type.find("tsens") != std::string::npos ||
        type.find("apss") != std::string::npos ||
        type.find("cluster") != std::string::npos) {
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

void ThermalMonitor::discover_sensors() const {
    if (!sensors_.empty()) return;
    std::error_code ec;
    namespace fs = std::filesystem;
    for (fs::directory_iterator it(sysfs_root_, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && sensors_.size() < 512; it.increment(ec)) {
        if (it->path().filename().string().rfind("thermal_zone", 0) != 0) continue;
        std::string type;
        if (!read_sysfs_line(it->path().string() + "/type", type)) continue;
        std::string lower = type;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        int category = 0;
        if (is_trusted_compute_zone(type) || lower.find("gpu") != std::string::npos) category = 1;
        else if (lower == "battery" || lower == "batt" || lower.rfind("battery-", 0) == 0) category = 2;
        else if (lower == "skin" || lower.rfind("skin-", 0) == 0 || lower.rfind("skin_", 0) == 0) category = 3;
        if (category) sensors_.push_back({it->path().string(), type, category});
    }
}
float ThermalMonitor::hottest_category(int category) const {
    discover_sensors();
    float hottest = -1;
    for (const auto& sensor : sensors_) {
        if (sensor.category != category) continue;
        std::string raw; float value;
        if (read_sysfs_line(sensor.path + "/temp", raw) && parse_celsius(raw, value)) hottest = std::max(hottest, value);
    }
    return hottest;
}
float ThermalMonitor::hottest_celsius() const { return hottest_category(1); }
float ThermalMonitor::battery_celsius() const { return hottest_category(2); }
float ThermalMonitor::skin_celsius() const { return hottest_category(3); }

ThermalAssessment assessment_from_android_severity(int severity) {
    if (severity < 0 || severity > 6) return {};
    constexpr double indices[] = {0, 40, 75, 95, 110, 120, 130};
    return {true, severity >= 5, indices[severity], severity, "android-thermal", "OEM severity"};
}
ThermalAssessment ThermalMonitor::assess(bool use_headroom) const {
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
                    if (std::isfinite(cached) && cached >= 0 && cached <= 10)
                        result.pressure = std::max(result.pressure, std::min(130.0, static_cast<double>(cached) * 85.0));
                }
                return result;
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
        const bool valid_temperature = read_sysfs_line(sensor.path + "/temp", raw) && parse_celsius(raw, temp);
        float passive = -1, emergency = -1, hyst = 0;
        // Never assume a global degree threshold or reinterpret active cooling trips.
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
            if ((type == "hot" || type == "critical") && (emergency < 0 || value < emergency)) emergency = value;
        }
        if (!valid_temperature) {
            if (passive > 0 || emergency > 0) return {}; // A known policy sensor cannot be silently dropped.
            continue;
        }
        if (emergency > 0 && temp >= emergency) {
            return {true, true, 120, -1, "sysfs-oem-trips", sensor.type};
        }
        if (passive <= 0 || (emergency > 0 && passive >= emergency)) continue;
        const double margin = std::clamp(static_cast<double>(passive) * 0.05, 1.0, 5.0);
        // Normalize each sensor against its own OEM trip. Different sensor classes
        // are compared only as policy-relative pressure, never as raw Celsius.
        const double recovery = passive - std::max(margin, static_cast<double>(hyst));
        if (recovery < 0 || recovery >= passive) continue;
        const double pressure = std::clamp(65.0 + (temp - recovery) / (passive - recovery) * 20.0, 0.0, 130.0);
        if (!result.available || pressure > result.pressure)
            result = {true, false, pressure, -1, "sysfs-oem-trips", sensor.type};
    }
    return result;
}
} // namespace coreflow
