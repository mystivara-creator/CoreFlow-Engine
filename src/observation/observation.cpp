#include "coreflow/observation/observation.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <sys/utsname.h>
#include <unistd.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
namespace coreflow {
namespace {
bool readFirst(const std::string& path, std::string& value) {
    std::ifstream f(path);
    return static_cast<bool>(f >> value);
}
bool number(const std::string& s, double& v) {
    try { std::size_t n=0; v=std::stod(s,&n); return n==s.size() && std::isfinite(v); }
    catch (...) { return false; }
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
    std::ostringstream out;
    out << in.rdbuf();
    auto result = out.str();
    if (result.size() > 4096) return {};
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

bool readWhole(const std::string& path, std::string& value) {
    std::ifstream in(path);
    if (!in) return false;
    std::ostringstream out;
    out << in.rdbuf();
    value = out.str();
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return false;
    const auto last = value.find_last_not_of(" \t\r\n");
    value = value.substr(first, last - first + 1);
    return value.size() <= 2048;
}
}

SystemObservation RuntimeObserver::observeSystem() const {
    SystemObservation o;
    std::ifstream stat("/proc/stat");
    std::string label;
    unsigned long long user=0,nice=0,system=0,idle=0,iowait=0,irq=0,softirq=0,steal=0;
    if (stat >> label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal) {
        static unsigned long long prev_total=0, prev_idle=0;
        const auto total=user+nice+system+idle+iowait+irq+softirq+steal;
        const auto idle_all=idle+iowait;
        if (prev_total && total > prev_total && idle_all >= prev_idle) {
            const auto td=total-prev_total, id=idle_all-prev_idle;
            o.cpu_busy_percent=100.0*static_cast<double>(td-id)/static_cast<double>(td);
            o.cpu_valid=true;
        }
        prev_total=total; prev_idle=idle_all;
    }
    std::ifstream mem("/proc/meminfo");
    std::string key, unit;
    double total_kb=0, available_kb=0, value=0;
    while (mem >> key >> value >> unit) {
        if (key=="MemTotal:") total_kb=value;
        else if (key=="MemAvailable:") available_kb=value;
    }
    if (total_kb > 0 && available_kb >= 0) {
        o.memory_available_ratio=std::clamp(available_kb/total_kb, 0.0, 1.0);
        o.memory_valid=true;
    }
    // Discover every thermal zone rather than assuming a particular zone index.
    const fs::path thermal_root("/sys/class/thermal");
    std::error_code ec;
    if (fs::exists(thermal_root, ec)) {
        for (fs::directory_iterator it(thermal_root, fs::directory_options::skip_permission_denied, ec), end;
             it!=end; it.increment(ec)) {
            if (ec) { ec.clear(); continue; }
            if (!it->is_directory(ec)) continue;
            const auto temp_path=it->path()/"temp";
            std::string raw;
            double t{};
            if (readFirst(temp_path.string(), raw) && number(raw,t)) {
                if (t > 1000.0) t/=1000.0;
                else if (t > 100.0) t/=10.0;
                if (!o.temperature_valid || t > o.maximum_temperature_c) o.maximum_temperature_c=t;
                o.temperature_valid=true;
            }
        }
    }
    const auto readPressure = [](const char* path, double& avg10) {
        std::ifstream input(path);
        std::string line;
        while (std::getline(input, line)) {
            if (line.rfind("some ", 0) != 0) continue;
            std::istringstream row(line);
            std::string kind, field;
            row >> kind;
            while (row >> field) {
                const auto equal = field.find('=');
                if (equal == std::string::npos || field.substr(0, equal) != "avg10") continue;
                return number(field.substr(equal + 1), avg10);
            }
        }
        return false;
    };
    o.cpu_pressure_valid = readPressure("/proc/pressure/cpu", o.cpu_pressure_some_avg10);
    o.memory_pressure_valid = readPressure("/proc/pressure/memory", o.memory_pressure_some_avg10);
    o.io_pressure_valid = readPressure("/proc/pressure/io", o.io_pressure_some_avg10);
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
