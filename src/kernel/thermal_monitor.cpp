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
    const double scaled = std::fabs(raw) >= 200.0 ? raw / 1000.0 : raw;
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
    if (!sensor_paths_.empty()) return;

    for (int i = 0; i < 128; ++i) {
        const std::string base = sysfs_root_ + "/thermal_zone" + std::to_string(i);

        std::string type;
        if (!read_sysfs_line(base + "/type", type) || !is_trusted_compute_zone(type)) {
            continue;
        }

        std::string temp_raw;
        float celsius = 0.0f;
        if (read_sysfs_line(base + "/temp", temp_raw) && parse_celsius(temp_raw, celsius)) {
            sensor_paths_.push_back(base + "/temp");
        }
    }
}

float ThermalMonitor::hottest_celsius() const {
    discover_sensors();
    float hottest = -1.0f;
    for (const auto& path : sensor_paths_) {
        std::string temp_raw;
        float celsius = 0.0f;
        if (read_sysfs_line(path, temp_raw) && parse_celsius(temp_raw, celsius)) {
            hottest = std::max(hottest, celsius);
        }
    }
    return hottest;
}

} // namespace coreflow
