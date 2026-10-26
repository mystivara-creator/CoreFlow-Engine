#include "coreflow/kernel_roots.hpp"
#include "coreflow/observer.hpp"
#include "coreflow/observer_metrics.hpp"
#include "coreflow/thermal_limits.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <unistd.h>

namespace coreflow {
namespace {

struct DirCloser {
    DIR* dir;
    ~DirCloser() { if (dir) ::closedir(dir); }
};

std::string lowerCopy(const std::string& input) {
    std::string out = input;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool isExcludedThermalType(const std::string& type) {
    const std::string t = lowerCopy(type);
    if (t.find("bcl") != std::string::npos) return true;
    if (t.find("mmw") != std::string::npos) return true;
    if (t.find("sub1") != std::string::npos) return true;
    if (t.find("sdr") != std::string::npos) return true;
    if (t.find("ific") != std::string::npos) return true;
    if (t.find("zeroc") != std::string::npos) return true;
    if (t.find("modem") != std::string::npos) return true;
    if (t.rfind("lvl", 0) != std::string::npos) return true;
    if (t.find("lvl") != std::string::npos && t.find("pm") != std::string::npos) return true;
    return false;
}

int thermalTypeRank(const std::string& type) {
    const std::string t = lowerCopy(type);
    if (isExcludedThermalType(type)) return -1;
    if (t == "quiet_therm" || t.find("quiet") != std::string::npos) return 100;
    if (t == "xo-therm" || t.find("xo_therm") != std::string::npos ||
        t.find("xo-therm") != std::string::npos) return 95;
    if (t.find("skin") != std::string::npos || t.find("shell") != std::string::npos) return 90;
    if (t.find("cpuss") != std::string::npos) return 80;
    if (t.find("cpu-") != std::string::npos || t.find("cpu_") != std::string::npos) return 70;
    if (t.find("tsens") != std::string::npos) return 65;
    if (t.find("gpu") != std::string::npos) return 55;
    if (t.find("soc") != std::string::npos && t.find("socd") == std::string::npos) return 50;
    if (t == "battery") return 40;
    if (t.find("aoss") != std::string::npos) return 35;
    if (t.find("ddr") != std::string::npos) return 30;
    return 10;
}

bool isPrimaryThermalType(const std::string& type) {
    return thermalTypeRank(type) >= 50;
}

template <std::size_t N>
std::size_t readBoundedFd(const std::string& path, std::array<char, N>& buffer,
                          int extra_flags = 0) noexcept {
    static_assert(N >= 2U, "Buffer must hold at least 1 byte plus NUL");
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | extra_flags);
    if (fd < 0) return 0U;

    ssize_t count = 0;
    do {
        count = ::read(fd, buffer.data(), N - 1U);
    } while (count < 0 && errno == EINTR);
    ::close(fd);

    if (count <= 0) return 0U;
    const auto len = static_cast<std::size_t>(count);
    buffer[len] = '\0';
    return len;
}

long readLongFd(const std::string& path, bool& ok) noexcept {
    ok = false;
    std::array<char, 64> buffer{};
    if (readBoundedFd(path, buffer, O_NOFOLLOW) == 0U) return 0;

    char* end = nullptr;
    errno = 0;
    const long val = std::strtol(buffer.data(), &end, 10);
    if (errno == 0 && end != buffer.data()) {
        ok = true;
        return val;
    }
    return 0;
}

bool readTrimmedLineFd(const std::string& path, std::string& out) noexcept {
    std::array<char, 64> buffer{};
    const std::size_t len = readBoundedFd(path, buffer);
    if (len == 0U) return false;
    std::string_view view(buffer.data(), len);
    const std::size_t eol = view.find('\n');
    if (eol != std::string_view::npos) view = view.substr(0U, eol);
    if (!view.empty() && view.back() == '\r') view.remove_suffix(1U);
    out.assign(view);
    return true;
}

bool parseLeadingNonNegativeDouble(std::string_view text, double& out) noexcept {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) {
        text.remove_prefix(1U);
    }
    if (text.empty() || text.front() < '0' || text.front() > '9') return false;

    double integer_part = 0.0;
    std::size_t pos = 0U;
    while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
        integer_part = integer_part * 10.0 + static_cast<double>(text[pos] - '0');
        ++pos;
    }

    double fraction_part = 0.0;
    double scale = 0.1;
    if (pos < text.size() && text[pos] == '.') {
        ++pos;
        if (pos >= text.size() || text[pos] < '0' || text[pos] > '9') return false;
        while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
            fraction_part += static_cast<double>(text[pos] - '0') * scale;
            scale *= 0.1;
            ++pos;
        }
    }

    const double result = integer_part + fraction_part;
    if (!std::isfinite(result) || result < 0.0) return false;
    out = result;
    return true;
}

bool nextUint64Token(const char*& cur, const char* end, std::uint64_t& value) noexcept {
    while (cur < end && (*cur == ' ' || *cur == '\t')) ++cur;
    if (cur >= end || *cur < '0' || *cur > '9') return false;
    const auto res = std::from_chars(cur, end, value);
    if (res.ec != std::errc{}) return false;
    cur = res.ptr;
    return true;
}

bool skipToken(const char*& cur, const char* end) noexcept {
    while (cur < end && (*cur == ' ' || *cur == '\t')) ++cur;
    if (cur >= end || *cur == '\n' || *cur == '\r') return false;
    while (cur < end && *cur != ' ' && *cur != '\t' && *cur != '\n' && *cur != '\r') ++cur;
    return true;
}

long cachedPageKb() noexcept {
    static const long kPageKb = [] {
        const long kb = ::sysconf(_SC_PAGESIZE) / 1024L;
        return kb > 0 ? kb : 4L;
    }();
    return kPageKb;
}

long cachedClockTicks() noexcept {
    static const long kTicks = ::sysconf(_SC_CLK_TCK);
    return kTicks;
}

long normalizeBatteryTemperature(long raw) {
    if (raw > -1000 && raw < 1000) return raw * 100;
    return raw;
}

bool parseProcessStat(const std::string& path, std::string& name,
                      std::uint64_t& ticks, std::uint64_t& memory_kb) {
    std::array<char, 512> buffer{};
    const std::size_t len = readBoundedFd(path, buffer);
    if (len == 0U) return false;

    std::string_view line(buffer.data(), len);
    const std::size_t eol = line.find('\n');
    if (eol != std::string_view::npos) line = line.substr(0U, eol);
    if (line.empty()) return false;

    const std::size_t open = line.find('(');
    const std::size_t close = line.rfind(')');
    if (open == std::string_view::npos || close == std::string_view::npos ||
        close <= open || close + 2U >= line.size()) {
        return false;
    }
    name.assign(line.substr(open + 1U, close - open - 1U));

    const char* cur = line.data() + close + 2U;
    const char* const end = line.data() + line.size();

    // Field 3: state token, Fields 4..13: 10 scalar tokens before utime/stime.
    if (!skipToken(cur, end)) return false;
    for (int field = 4; field <= 13; ++field) {
        if (!skipToken(cur, end)) return false;
    }

    std::uint64_t utime = 0;
    std::uint64_t stime = 0;
    if (!nextUint64Token(cur, end, utime) || !nextUint64Token(cur, end, stime)) {
        return false;
    }

    // Fields 16..23: 8 tokens before rss (field 24).
    for (int field = 16; field <= 23; ++field) {
        if (!skipToken(cur, end)) return false;
    }

    while (cur < end && (*cur == ' ' || *cur == '\t')) ++cur;
    if (cur >= end) return false;
    long long rss_pages = 0;
    const auto rss_res = std::from_chars(cur, end, rss_pages);
    if (rss_res.ec != std::errc{} || rss_pages < 0) return false;

    ticks = utime + stime;
    memory_kb = static_cast<std::uint64_t>(rss_pages) *
                static_cast<std::uint64_t>(cachedPageKb());
    return true;
}

} // namespace

void RuntimeObserver::readMemory(RuntimeSample& sample) const {
    std::array<char, 2048> buffer{};
    const std::size_t len = readBoundedFd(
        kernelFilesystemPath(KernelFsKind::Procfs, "meminfo"), buffer);
    if (len == 0U) return;

    bool got_total = false;
    bool got_avail = false;
    std::string_view text(buffer.data(), len);
    while (!text.empty() && !(got_total && got_avail)) {
        const std::size_t eol = text.find('\n');
        const std::string_view line =
            eol == std::string_view::npos ? text : text.substr(0U, eol);
        text = eol == std::string_view::npos ? std::string_view{} : text.substr(eol + 1U);

        if (line.rfind("MemTotal:", 0U) == 0U) {
            const char* cur = line.data() + 9U;
            const char* const end = line.data() + line.size();
            std::uint64_t val = 0;
            if (nextUint64Token(cur, end, val)) {
                sample.mem_total_kb = val;
                got_total = true;
            }
        } else if (line.rfind("MemAvailable:", 0U) == 0U) {
            const char* cur = line.data() + 13U;
            const char* const end = line.data() + line.size();
            std::uint64_t val = 0;
            if (nextUint64Token(cur, end, val)) {
                sample.mem_available_kb = val;
                got_avail = true;
            }
        }
    }

    if (sample.mem_total_kb > 0) {
        sample.mem_available_ratio =
            static_cast<double>(sample.mem_available_kb) /
            static_cast<double>(sample.mem_total_kb);
    }
}

void RuntimeObserver::readLoad(RuntimeSample& sample) const {
    std::array<char, 128> buffer{};
    const std::size_t len = readBoundedFd(
        kernelFilesystemPath(KernelFsKind::Procfs, "loadavg"), buffer);
    if (len == 0U) return;

    double value = 0.0;
    if (parseLeadingNonNegativeDouble(std::string_view(buffer.data(), len), value)) {
        sample.load1 = value;
        sample.load_available = true;
    }
}

void RuntimeObserver::readCpuUtilization(RuntimeSample& sample) const {
    std::array<char, 256> buffer{};
    const std::size_t len = readBoundedFd(
        kernelFilesystemPath(KernelFsKind::Procfs, "stat"), buffer);
    if (len < 5U) return;

    std::string_view line(buffer.data(), len);
    const std::size_t eol = line.find('\n');
    if (eol != std::string_view::npos) line = line.substr(0U, eol);
    if (line.size() < 4U || line.rfind("cpu", 0U) != 0U ||
        (line[3] != ' ' && line[3] != '\t')) {
        return;
    }

    const char* cur = line.data() + 4U;
    const char* const end = line.data() + line.size();
    std::uint64_t user = 0, nice = 0, system = 0, idle = 0;
    std::uint64_t iowait = 0, irq = 0, softirq = 0, steal = 0;
    if (!nextUint64Token(cur, end, user) ||
        !nextUint64Token(cur, end, nice) ||
        !nextUint64Token(cur, end, system) ||
        !nextUint64Token(cur, end, idle) ||
        !nextUint64Token(cur, end, iowait) ||
        !nextUint64Token(cur, end, irq) ||
        !nextUint64Token(cur, end, softirq) ||
        !nextUint64Token(cur, end, steal)) {
        return;
    }

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
    long preferred = 0;
    int preferred_rank = -1;
    bool any = false;
    bool primary = false;

    sample.thermal_readings.clear();
    sample.thermal_readings.reserve(profile.thermal_zones.size());
    sample.thermal_sensor_count = static_cast<std::uint32_t>(profile.thermal_zones.size());
    sample.thermal_valid_sensor_count = 0;

    for (const ThermalZone& zone : profile.thermal_zones) {
        ThermalReading reading;
        const std::size_t separator = zone.path.find_last_of('/');
        reading.zone = separator == std::string::npos
                           ? zone.path : zone.path.substr(separator + 1);
        reading.type = zone.type;

        bool ok = false;
        const long value = readLongFd(zone.path + "/temp", ok);
        reading.readable = ok;
        reading.temperature_millidegrees = ok ? value : 0;

        if (!ok) {
            reading.status = ThermalReadingStatus::Unreadable;
            sample.thermal_readings.push_back(std::move(reading));
            continue;
        }
        if (!isPlausibleThermalMilli(value)) {
            reading.status = ThermalReadingStatus::InvalidRange;
            sample.thermal_readings.push_back(std::move(reading));
            continue;
        }

        reading.valid = true;
        reading.status = ThermalReadingStatus::Valid;
        ++sample.thermal_valid_sensor_count;
        const bool excluded = isExcludedThermalType(zone.type);
        reading.policy_eligible = !excluded;
        if (excluded) reading.status = ThermalReadingStatus::ExcludedFromPolicy;
        sample.thermal_readings.push_back(reading);
        if (excluded) continue;

        any = true;
        hottest = std::max(hottest, value);

        const int rank = thermalTypeRank(zone.type);
        if (rank > preferred_rank) {
            preferred_rank = rank;
            preferred = value;
        } else if (rank == preferred_rank && rank >= 0) {
            preferred = std::max(preferred, value);
        }
        if (isPrimaryThermalType(zone.type)) {
            primary = true;
            primary_hottest = std::max(primary_hottest, value);
        }
    }

    if (!any) return;
    sample.thermal_available = true;
    sample.hottest_thermal_millidegrees = hottest;

    if (preferred_rank >= 50) {
        sample.thermal_millidegrees = preferred;
        sample.thermal_source = ThermalSource::Primary;
    } else if (primary) {
        sample.thermal_millidegrees = primary_hottest;
        sample.thermal_source = ThermalSource::Primary;
    } else {
        sample.thermal_millidegrees = hottest;
        sample.thermal_source = ThermalSource::Fallback;
    }
}

void RuntimeObserver::readCharging(RuntimeSample& sample) const {
    const std::string base = kernelFilesystemPath(KernelFsKind::Sysfs, "class/power_supply");
    if (base.empty()) return;

    if (cached_battery_base_ != base) {
        cached_battery_base_ = base;
        cached_battery_path_.clear();
    }

    auto sampleBatteryNode = [&](const std::string& path, bool& status_found) -> bool {
        std::string type;
        if (!readTrimmedLineFd(path + "/type", type) || type != "Battery") return false;

        std::string status;
        if (readTrimmedLineFd(path + "/status", status)) {
            status_found = true;
            sample.battery_status = status;
            sample.charging = (status == "Charging");
        }

        bool level_ok = false;
        const long level = readLongFd(path + "/capacity", level_ok);
        if (level_ok && level >= 0 && level <= 100) sample.battery_level_percent = static_cast<int>(level);

        bool ok = false;
        const long temp = readLongFd(path + "/temp", ok);
        if (ok) sample.battery_temperature_millidegrees = normalizeBatteryTemperature(temp);
        const long current = readLongFd(path + "/current_now", ok);
        if (ok) sample.battery_current_microamps = current;
        const long voltage = readLongFd(path + "/voltage_now", ok);
        if (ok) sample.battery_voltage_microvolts = voltage;
        return true;
    };

    bool status_found = false;
    bool battery_found = false;

    if (!cached_battery_path_.empty()) {
        battery_found = sampleBatteryNode(cached_battery_path_, status_found);
        if (!battery_found) cached_battery_path_.clear();
    }

    if (!battery_found) {
        DIR* raw_dir = opendir(base.c_str());
        if (!raw_dir) return;
        DirCloser dir_guard{raw_dir};

        while (dirent* entry = readdir(raw_dir)) {
            const std::string name(entry->d_name);
            if (name == "." || name == "..") continue;
            const std::string path = base + "/" + name;
            if (!sampleBatteryNode(path, status_found)) continue;
            battery_found = true;
            cached_battery_path_ = path;
            break;
        }
    }

    sample.charging_telemetry_available =
        battery_found && (status_found ||
                          sample.battery_temperature_millidegrees != 0 ||
                          sample.battery_current_microamps != 0 ||
                          sample.battery_voltage_microvolts != 0);
}

void RuntimeObserver::readIoActivity(RuntimeSample& sample) const {
    std::ifstream file(kernelFilesystemPath(KernelFsKind::Procfs, "diskstats"));
    if (!file) return;

    observer_metrics::DiskCounterMap current;
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream in(line);
        int major = 0;
        int minor = 0;
        std::string name;

        std::uint64_t rd_ios = 0;
        std::uint64_t rd_merged = 0;
        std::uint64_t rd_sectors = 0;
        std::uint64_t rd_ms = 0;
        std::uint64_t wr_ios = 0;
        std::uint64_t wr_merged = 0;
        std::uint64_t wr_sectors = 0;
        std::uint64_t wr_ms = 0;

        if (!(in >> major >> minor >> name
                 >> rd_ios >> rd_merged >> rd_sectors >> rd_ms
                 >> wr_ios >> wr_merged >> wr_sectors >> wr_ms)) {
            continue;
        }

        if (name.rfind("loop", 0) == 0 ||
            name.rfind("ram", 0) == 0 ||
            name.rfind("zram", 0) == 0 ||
            name.rfind("dm-", 0) == 0) {
            continue;
        }

        const std::string key =
            std::to_string(major) + ":" +
            std::to_string(minor) + ":" + name;

        current[key] = observer_metrics::DiskCounters{
            rd_sectors, wr_sectors, rd_ios, wr_ios, rd_ms, wr_ms
        };
    }

    const auto delta = observer_metrics::calculateIoDelta(
        current, previous_io_devices_, last_interval_seconds_);

    sample.io_read_kb_per_sec = delta.read_kb_per_sec;
    sample.io_write_kb_per_sec = delta.write_kb_per_sec;
    sample.io_activity_available = delta.activity_available;

    sample.io_read_latency_ms = delta.read_latency_ms;
    sample.io_write_latency_ms = delta.write_latency_ms;
    sample.io_avg_latency_ms = delta.avg_latency_ms;
    sample.io_latency_available = delta.latency_available;
}

void RuntimeObserver::readProcessProfile(RuntimeSample& sample) const {
    if ((sample_counter_ % 6U) != 0U) return;
    const std::string proc_root = kernelFilesystemPath(KernelFsKind::Procfs);
    if (proc_root.empty()) return;
    DIR* raw_dir = opendir(proc_root.c_str());
    if (!raw_dir) return;
    DirCloser dir_guard{raw_dir};

    const long ticks_per_second = cachedClockTicks();
    std::unordered_map<int, std::uint64_t> current_ticks;
    current_ticks.reserve(previous_process_ticks_.size());
    std::string best_name;
    std::string name;
    std::string stat_path;
    stat_path.reserve(proc_root.size() + 24U);
    std::uint64_t best_delta = 0;
    std::uint64_t best_memory = 0;
    std::uint32_t count = 0;
    while (dirent* entry = readdir(raw_dir)) {
        const char* text = entry->d_name;
        if (!std::isdigit(static_cast<unsigned char>(text[0]))) continue;
        const int pid = std::atoi(text);
        if (pid <= 0) continue;
        stat_path.assign(proc_root);
        stat_path.push_back('/');
        stat_path.append(text);
        stat_path.append("/stat");
        std::uint64_t ticks = 0;
        std::uint64_t memory_kb = 0;
        if (!parseProcessStat(stat_path, name, ticks, memory_kb)) continue;
        ++count;
        current_ticks[pid] = ticks;
        const auto old = previous_process_ticks_.find(pid);
        if (old != previous_process_ticks_.end() && ticks >= old->second) {
            const std::uint64_t delta = ticks - old->second;
            if (delta > best_delta) {
                best_delta = delta;
                best_name = name;
                best_memory = memory_kb;
            }
        }
    }

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

void RuntimeObserver::readPsi(RuntimeSample& sample) const {
    std::string line_buf;
    line_buf.reserve(128U);
    auto readResource = [&](const std::string& path, double& some, double& full) -> bool {
        std::array<char, 512> buffer{};
        const std::size_t len = readBoundedFd(path, buffer);
        if (len == 0U) return false;

        bool got = false;
        std::string_view text(buffer.data(), len);
        while (!text.empty()) {
            const std::size_t eol = text.find('\n');
            const std::string_view line =
                eol == std::string_view::npos ? text : text.substr(0U, eol);
            text = eol == std::string_view::npos ? std::string_view{} : text.substr(eol + 1U);
            if (line.empty()) continue;
            line_buf.assign(line);
            if (observer_metrics::parsePsiLine(line_buf, some, full)) got = true;
        }
        return got;
    };

    bool any = false;
    any |= readResource(kernelFilesystemPath(KernelFsKind::Procfs, "pressure/cpu"),
                        sample.psi_cpu_some_avg10, sample.psi_cpu_full_avg10);
    any |= readResource(kernelFilesystemPath(KernelFsKind::Procfs, "pressure/memory"),
                        sample.psi_memory_some_avg10, sample.psi_memory_full_avg10);
    any |= readResource(kernelFilesystemPath(KernelFsKind::Procfs, "pressure/io"),
                        sample.psi_io_some_avg10, sample.psi_io_full_avg10);
    if (!any) return;

    sample.psi_available = true;
    auto norm = [](double& value) {
        value = observer_metrics::normalizePsiPercent(value);
    };
    norm(sample.psi_cpu_some_avg10);
    norm(sample.psi_memory_some_avg10);
    norm(sample.psi_io_some_avg10);
    norm(sample.psi_cpu_full_avg10);
    norm(sample.psi_memory_full_avg10);
    norm(sample.psi_io_full_avg10);
}

void RuntimeObserver::diagnoseStalls(RuntimeSample& sample) const {
    sample.throttle_ratio = 0.0;
    sample.throttling_detected = false;

    double mem_score = 0.0;
    if (sample.psi_available) {
        mem_score = std::max(sample.psi_memory_some_avg10, sample.psi_memory_full_avg10 * 1.2);
    }
    if (sample.mem_total_kb > 0 && sample.mem_available_ratio < 0.12) {
        mem_score = std::max(mem_score, 1.0 - sample.mem_available_ratio);
    }
    sample.memory_stall_score = std::clamp(mem_score, 0.0, 1.0);
    sample.memory_stall_detected = sample.memory_stall_score >= 0.10;
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

    std::array<char, 128> uptime_buf{};
    const std::size_t uptime_len = readBoundedFd(
        kernelFilesystemPath(KernelFsKind::Procfs, "uptime"), uptime_buf);
    double seconds = 0.0;
    if (uptime_len > 0U &&
        parseLeadingNonNegativeDouble(std::string_view(uptime_buf.data(), uptime_len), seconds)) {
        sample.uptime_seconds = static_cast<std::uint64_t>(seconds);
    }

    readMemory(sample);
    readLoad(sample);
    readCpuUtilization(sample);
    readThermal(sample, profile);
    readCharging(sample);
    readIoActivity(sample);
    readPsi(sample);
    diagnoseStalls(sample);
    readProcessProfile(sample);
    return sample;
}

} // namespace coreflow
