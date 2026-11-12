#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <sys/stat.h>
#include <unistd.h>
namespace cddp_fixture {
namespace fs = std::filesystem;
struct Fixture {
    fs::path root, sys, proc;
    Fixture() = default;
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&& other) noexcept
        : root(std::move(other.root)), sys(std::move(other.sys)), proc(std::move(other.proc)) { other.root.clear(); }
    Fixture& operator=(Fixture&& other) noexcept {
        if (this != &other) { cleanup(); root = std::move(other.root); sys = std::move(other.sys);
            proc = std::move(other.proc); other.root.clear(); }
        return *this;
    }
    ~Fixture() { cleanup(); }
    void cleanup() noexcept {
        if (!root.empty()) { std::error_code ec; fs::remove_all(root, ec); root.clear(); }
    }
};
inline bool put(const fs::path& path, const std::string& value, mode_t mode = 0644) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    if (ec) return false;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(value.data(), static_cast<std::streamsize>(value.size()));
    out.close();
    return out.good() && ::chmod(path.c_str(), mode) == 0;
}
inline Fixture create(const std::string& id) {
    Fixture f;
    f.root = fs::path("/tmp") / ("coreflow-cddp-" + std::to_string(::getpid()) + "-" + id);
    f.sys = f.root / "sys"; f.proc = f.root / "proc";
    std::error_code ec; fs::remove_all(f.root, ec); ec.clear();
    fs::create_directories(f.sys, ec); if (ec) return f;
    if (id != "J") {
        put(f.sys / "devices/system/cpu/possible", "0-3\n");
        put(f.sys / "devices/system/cpu/present", "0-3\n");
        put(f.sys / "devices/system/cpu/online", "0-3\n");
        const fs::path p = f.sys / "devices/system/cpu/cpufreq/policy0";
        put(p / "related_cpus", "0-3\n"); put(p / "affected_cpus", "0-3\n");
        put(p / "cpuinfo_min_freq", "300000\n"); put(p / "cpuinfo_max_freq", "2000000\n");
        put(p / "scaling_min_freq", "300000\n"); put(p / "scaling_max_freq", "2000000\n");
        put(p / "scaling_cur_freq", "1200000\n");
        put(p / "scaling_available_frequencies", "300000 600000 1000000 1500000 2000000\n");
        put(p / "scaling_governor", "schedutil\n");
        put(p / "schedutil/up_rate_limit_us", "500\n"); put(p / "schedutil/down_rate_limit_us", "10000\n");
        put(f.proc / "sys/kernel/osrelease", "6.1.0-cddp-fixture\n");
        put(f.proc / "meminfo", "MemTotal: 8192000 kB\nMemAvailable: 4096000 kB\n");
        put(f.proc / "pressure/cpu", "some avg10=1.00 avg60=1.00 avg300=1.00 total=10\n");
        const auto cgroup_mount = f.sys / "fs/cgroup";
        put(f.proc / "self/mountinfo", "31 20 0:28 / " + cgroup_mount.string() + " rw - cgroup2 cgroup rw\n");
        put(cgroup_mount / "cgroup.controllers", "cpu cpuset memory io\n");
        const auto foreground_group = cgroup_mount / "interactive-session";
        put(foreground_group / "cpu.uclamp.min", "0.00\n");
        put(foreground_group / "cpu.uclamp.max", "max\n");
        put(foreground_group / "cgroup.threads", "");
        put(foreground_group / "cgroup.procs", "");
        put(f.sys / "class/thermal/thermal_zone0/type", "soc-thermal\n");
        put(f.sys / "class/thermal/thermal_zone0/temp", "42000\n");
        put(f.sys / "devices/soc0/machine", "Synthetic SoC A\n");
        put(f.sys / "devices/soc0/family", "synthetic\n");
    }
    if (id == "B") {
        put(f.sys / "devices/system/cpu/possible", "0-5\n");
        put(f.sys / "devices/system/cpu/present", "0-5\n");
        put(f.sys / "devices/system/cpu/online", "0-1,3-5\n");
        put(f.sys / "devices/system/cpu/cpufreq/policy0/related_cpus", "0,2-3\n");
    } else if (id == "C") {
        put(f.sys / "devices/system/cpu/online", "0-2,,4\n");
    } else if (id == "D") {
        std::error_code remove_ec;
        fs::remove(f.sys / "devices/system/cpu/cpufreq/policy0/scaling_available_frequencies", remove_ec);
    } else if (id == "E") {
        (void)::chmod((f.sys / "devices/system/cpu/cpufreq/policy0/scaling_max_freq").c_str(), 0444);
    } else if (id == "F") {
        put(f.sys / "devices/system/cpu/cpufreq/policy0/cpuinfo_min_freq", "2100000\n");
    } else if (id == "G") {
        const fs::path bus = f.sys / "class/devfreq/soc-bus";
        put(bus / "name", "soc-bus\n"); put(bus / "min_freq", "100000000\n");
        put(bus / "max_freq", "400000000\n"); put(bus / "cur_freq", "200000000\n");
        put(bus / "available_frequencies", "100000000 200000000 400000000\n");
    } else if (id == "H") {
        const fs::path gpu = f.sys / "class/devfreq/mali.0";
        put(gpu / "name", "mali\n"); put(gpu / "min_freq", "200000000\n");
        put(gpu / "max_freq", "800000000\n"); put(gpu / "cur_freq", "400000000\n");
        put(gpu / "available_frequencies", "200000000 400000000 600000000 800000000\n");
        put(gpu / "governor", "simple_ondemand\n");
    } else if (id == "I") {
        const fs::path kgsl = f.sys / "class/kgsl/kgsl-3d0";
        put(kgsl / "gpuclk", "300000000\n"); put(kgsl / "max_gpuclk", "800000000\n");
        put(kgsl / "min_gpuclk", "200000000\n");
    } else if (id == "K") {
        put(f.sys / "devices/system/cpu/cpufreq/policy0/related_cpus", "0 1 2 3\n");
    } else if (id == "L") {
        put(f.sys / "devices/system/cpu/cpufreq/policy0/related_cpus", "0,,3\n");
        put(f.sys / "devices/system/cpu/cpufreq/policy0/affected_cpus", "0-3\n");
    }
    return f;
}
} // namespace cddp_fixture
