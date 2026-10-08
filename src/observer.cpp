#include "coreflow/observer.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <unistd.h>

namespace coreflow {
namespace {

std::string lowerCopy(const std::string& input) {
    std::string out = input;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool isPrimaryThermalType(const std::string& type) {
    const std::string t = lowerCopy(type);
    return t.find("cpu") != std::string::npos ||
           t.find("soc") != std::string::npos ||
           t.find("gpu") != std::string::npos ||
           t.find("tsens") != std::string::npos;
}

long readLong(const std::string& path, bool& ok) {
    std::ifstream file(path);
    long value = 0;
    ok = static_cast<bool>(file >> value);
    return value;
}

long normalizeBatteryTemperature(long raw) {
    // Linux power_supply battery temp is commonly tenths of a degree C.
    // Millidegree values are much larger. Keep this conservative and read-only.
    if (raw > -1000 && raw < 1000) return raw * 100;
    return raw;
}

bool parseProcessStat(const std::string& path, std::string& name,
                      std::uint64_t& ticks, std::uint64_t& memory_kb) {
    std::ifstream file(path);
    if (!file) return false;
    std::string line;
    std::getline(file, line);
    if (line.empty()) return false;
    const std::size_t open = line.find('(');
    const std::size_t close = line.rfind(')');
    if (open == std::string::npos || close == std::string::npos || close <= open) return false;
    name = line.substr(open + 1, close - open - 1);
    std::istringstream in(line.substr(close + 2));
    char state = 0;
    std::uint64_t ignored = 0;
    // fields after comm: state=3, utime=14, stime=15, rss=24.
    if (!(in >> state)) return false;
    for (int field = 4; field <= 13; ++field) if (!(in >> ignored)) return false;
    std::uint64_t utime = 0;
    std::uint64_t stime = 0;
    if (!(in >> utime >> stime)) return false;
    for (int field = 16; field <= 23; ++field) if (!(in >> ignored)) return false;
    long long rss_pages = 0;
    if (!(in >> rss_pages) || rss_pages < 0) return false;
    ticks = utime + stime;
    const long page_kb = sysconf(_SC_PAGESIZE) / 1024L;
    memory_kb = static_cast<std::uint64_t>(rss_pages) *
                static_cast<std::uint64_t>(page_kb > 0 ? page_kb : 4);
    return true;
}


} // namespace

void RuntimeObserver::readMemory(RuntimeSample& sample) const {
    std::ifstream file("/proc/meminfo");
    if (!file) return;

    std::string key;
    std::uint64_t value = 0;
    std::string unit;
    while (file >> key >> value >> unit) {
        if (key == "MemTotal:") sample.mem_total_kb = value;
        else if (key == "MemAvailable:") sample.mem_available_kb = value;
    }

    if (sample.mem_total_kb > 0) {
        sample.mem_available_ratio =
            static_cast<double>(sample.mem_available_kb) /
            static_cast<double>(sample.mem_total_kb);
    }
}

void RuntimeObserver::readLoad(RuntimeSample& sample) const {
    std::ifstream file("/proc/loadavg");
    if (file) file >> sample.load1;
}

void RuntimeObserver::readCpuUtilization(RuntimeSample& sample) const {
    std::ifstream file("/proc/stat");
    if (!file) return;

    std::string cpu;
    std::uint64_t user = 0, nice = 0, system = 0, idle = 0;
    std::uint64_t iowait = 0, irq = 0, softirq = 0, steal = 0;
    if (!(file >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal)) return;
    if (cpu != "cpu") return;

    const std::uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
    const std::uint64_t idle_total = idle + iowait;

    if (!have_cpu_baseline_) {
        previous_cpu_total_ = total;
        previous_cpu_idle_ = idle_total;
        have_cpu_baseline_ = true;
        return;
    }

    if (total <= previous_cpu_total_ || idle_total < previous_cpu_idle_) return;

    const std::uint64_t total_delta = total - previous_cpu_total_;
    const std::uint64_t idle_delta = idle_total - previous_cpu_idle_;
    previous_cpu_total_ = total;
    previous_cpu_idle_ = idle_total;

    if (total_delta == 0 || idle_delta > total_delta) return;
    sample.cpu_utilization =
        1.0 - static_cast<double>(idle_delta) / static_cast<double>(total_delta);
    sample.cpu_utilization = std::clamp(sample.cpu_utilization, 0.0, 1.0);
    sample.cpu_utilization_available = true;
}

void RuntimeObserver::readThermal(RuntimeSample& sample, const DeviceProfile& profile) const {
    long hottest = 0;
    long primary_hottest = 0;
    bool any = false;
    bool primary = false;

    for (const ThermalZone& zone : profile.thermal_zones) {
        bool ok = false;
        const long value = readLong(zone.path + "/temp", ok);
        if (!ok) continue;
        any = true;
        hottest = std::max(hottest, value);
        if (isPrimaryThermalType(zone.type)) {
            primary = true;
            primary_hottest = std::max(primary_hottest, value);
        }
    }

    if (!any) return;
    sample.thermal_available = true;
    sample.hottest_thermal_millidegrees = hottest;
    if (primary) {
        sample.thermal_millidegrees = primary_hottest;
        sample.thermal_source = ThermalSource::Primary;
    } else {
        sample.thermal_millidegrees = hottest;
        sample.thermal_source = ThermalSource::Fallback;
    }
}

void RuntimeObserver::readCharging(RuntimeSample& sample) const {
    constexpr const char* base = "/sys/class/power_supply";
    DIR* dir = opendir(base);
    if (!dir) return;

    bool status_found = false;
    bool battery_found = false;
    while (dirent* entry = readdir(dir)) {
        const std::string name(entry->d_name);
        if (name == "." || name == "..") continue;
        const std::string path = std::string(base) + "/" + name;

        std::ifstream type_file(path + "/type");
        std::string type;
        if (!type_file || !std::getline(type_file, type) || type != "Battery") continue;
        battery_found = true;

        std::ifstream status_file(path + "/status");
        std::string status;
        if (status_file && std::getline(status_file, status)) {
            status_found = true;
            sample.battery_status = status;
            sample.charging = (status == "Charging");
        }

        bool level_ok = false;
        const long level = readLong(path + "/capacity", level_ok);
        if (level_ok && level >= 0 && level <= 100) sample.battery_level_percent = static_cast<int>(level);

        bool ok = false;
        const long temp = readLong(path + "/temp", ok);
        if (ok) sample.battery_temperature_millidegrees = normalizeBatteryTemperature(temp);
        const long current = readLong(path + "/current_now", ok);
        if (ok) sample.battery_current_microamps = current;
        const long voltage = readLong(path + "/voltage_now", ok);
        if (ok) sample.battery_voltage_microvolts = voltage;
        break;
    }
    closedir(dir);

    sample.charging_telemetry_available =
        battery_found && (status_found ||
                          sample.battery_temperature_millidegrees != 0 ||
                          sample.battery_current_microamps != 0 ||
                          sample.battery_voltage_microvolts != 0);
}


void RuntimeObserver::readIoActivity(RuntimeSample& sample) const {
    std::ifstream file("/proc/diskstats");
    if (!file) return;
    std::uint64_t reads = 0;
    std::uint64_t writes = 0;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream in(line);
        int major = 0;
        int minor = 0;
        std::string name;
        std::uint64_t rd_completed = 0;
        std::uint64_t rd_merged = 0;
        std::uint64_t rd_sectors = 0;
        std::uint64_t read_ms = 0;
        std::uint64_t read_weighted_ms = 0;
        std::uint64_t wr_completed = 0;
        std::uint64_t wr_merged = 0;
        std::uint64_t wr_sectors = 0;
        if (!(in >> major >> minor >> name >> rd_completed >> rd_merged >> rd_sectors
              >> read_ms >> read_weighted_ms >> wr_completed >> wr_merged >> wr_sectors)) continue;
        // Avoid counting loop/ram devices and prefer physical/block-backed activity.
        if (name.rfind("loop", 0) == 0 || name.rfind("ram", 0) == 0) continue;
        reads += rd_sectors;
        writes += wr_sectors;
    }
    if (!have_io_baseline_) {
        previous_io_read_sectors_ = reads;
        previous_io_write_sectors_ = writes;
        have_io_baseline_ = true;
        return;
    }
    const std::uint64_t rd_delta = reads >= previous_io_read_sectors_ ? reads - previous_io_read_sectors_ : 0;
    const std::uint64_t wr_delta = writes >= previous_io_write_sectors_ ? writes - previous_io_write_sectors_ : 0;
    previous_io_read_sectors_ = reads;
    previous_io_write_sectors_ = writes;
    // Linux diskstats sectors are normally 512-byte sectors. Runtime interval is
    // intentionally estimated from the engine's five-second default cadence.
    const double interval = std::clamp(last_interval_seconds_, 0.25, 60.0);
    sample.io_read_kb_per_sec = static_cast<double>(rd_delta) * 0.5 / interval;
    sample.io_write_kb_per_sec = static_cast<double>(wr_delta) * 0.5 / interval;
    sample.io_activity_available = true;
}

void RuntimeObserver::readProcessProfile(RuntimeSample& sample) const {
    // Process scanning is intentionally throttled: it is a profiling signal,
    // not a hot-path actuator. Every sixth sample is ~30s at the default cadence.
    if ((sample_counter_ % 6U) != 0U) return;
    DIR* dir = opendir("/proc");
    if (!dir) return;
    const long ticks_per_second = sysconf(_SC_CLK_TCK);
    std::unordered_map<int, std::uint64_t> current_ticks;
    std::string best_name;
    std::uint64_t best_delta = 0;
    std::uint64_t best_memory = 0;
    std::uint32_t count = 0;
    while (dirent* entry = readdir(dir)) {
        const char* text = entry->d_name;
        if (!std::isdigit(static_cast<unsigned char>(text[0]))) continue;
        const int pid = std::atoi(text);
        if (pid <= 0) continue;
        std::string name;
        std::uint64_t ticks = 0;
        std::uint64_t memory_kb = 0;
        if (!parseProcessStat(std::string("/proc/") + text + "/stat", name, ticks, memory_kb)) continue;
        ++count;
        current_ticks[pid] = ticks;
        const auto old = previous_process_ticks_.find(pid);
        if (old != previous_process_ticks_.end() && ticks >= old->second) {
            const std::uint64_t delta = ticks - old->second;
            if (delta > best_delta) {
                best_delta = delta;
                best_name = std::move(name);
                best_memory = memory_kb;
            }
        }
    }
    closedir(dir);
    previous_process_ticks_.swap(current_ticks);
    sample.process_count = count;
    double profile_interval = 30.0;
    const auto now = std::chrono::steady_clock::now();
    if (have_process_profile_time_) {
        profile_interval = std::chrono::duration<double>(now - previous_process_profile_time_).count();
    }
    previous_process_profile_time_ = now;
    have_process_profile_time_ = true;
    if (ticks_per_second > 0 && best_delta > 0) {
        sample.top_process_name = best_name;
        sample.top_process_memory_kb = best_memory;
        sample.top_process_cpu_ratio = std::clamp(
            static_cast<double>(best_delta) /
                (static_cast<double>(ticks_per_second) * std::clamp(profile_interval, 1.0, 120.0)),
            0.0, 1.0);
        sample.process_profile_available = true;
    }
}

RuntimeSample RuntimeObserver::sample(const DeviceProfile& profile) const {
    RuntimeSample sample;
    ++sample_counter_;
    const auto now = std::chrono::steady_clock::now();
    if (have_sample_time_) {
        last_interval_seconds_ = std::chrono::duration<double>(now - previous_sample_time_).count();
    }
    previous_sample_time_ = now;
    have_sample_time_ = true;

    std::ifstream uptime("/proc/uptime");
    double seconds = 0.0;
    if (uptime && (uptime >> seconds) && seconds >= 0.0)
        sample.uptime_seconds = static_cast<std::uint64_t>(seconds);

    readMemory(sample);
    readLoad(sample);
    readCpuUtilization(sample);
    readThermal(sample, profile);
    readCharging(sample);
    readIoActivity(sample);
    readProcessProfile(sample);
    return sample;
}

} // namespace coreflow
