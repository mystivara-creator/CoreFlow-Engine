/**
 * @file app_dna_store.cpp
 * @brief CoreFlow v6.2: Persistent Per-App Behavioral DNA Store.
 * @license Apache-2.0
 */
#include "coreflow/v6/app_dna_store.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unistd.h>

namespace fs = std::filesystem;

namespace coreflow::v6 {

namespace {

bool is_valid_pkg_token(const std::string& pkg) noexcept {
    if (pkg.empty() || pkg.size() > 160U) return false;
    for (const unsigned char c : pkg) {
        if (c <= 0x20 || c >= 0x7F || c == '\t' || c == '|' || c == ';') return false;
    }
    return pkg.find('.') != std::string::npos;
}

bool write_file_atomic(const std::string& target, const std::string& payload) noexcept {
    const std::string tmp = target + ".tmp";
    const int fd = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (fd < 0) return false;
    size_t written = 0;
    while (written < payload.size()) {
        const ssize_t n = ::write(fd, payload.data() + written, payload.size() - written);
        if (n <= 0) {
            ::close(fd);
            std::error_code ec;
            fs::remove(tmp, ec);
            return false;
        }
        written += static_cast<size_t>(n);
    }
    (void)::fsync(fd);
    ::close(fd);
    if (::rename(tmp.c_str(), target.c_str()) != 0) {
        std::error_code ec;
        fs::remove(tmp, ec);
        return false;
    }
    return true;
}

} // namespace

void AppDnaStore::initialize(const std::string& state_dir) noexcept {
    entries_.clear();
    dirty_ = false;
    if (state_dir.empty()) {
        store_path_.clear();
        return;
    }
    store_path_ = (fs::path(state_dir) / "app_dna.map").string();
    std::ifstream in(store_path_);
    if (!in.is_open()) return;

    std::string header;
    if (!std::getline(in, header) || header.rfind("CFDNA1", 0) != 0) return;

    std::string line;
    while (std::getline(in, line) && entries_.size() < kMaxEntries) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        AppDnaEntry e{};
        if (!(ss >> e.package_name >> e.uclamp_bias >> e.avg_thermal_slope >> e.avg_jank_ratio >>
              e.launch_count >> e.sample_count >> e.last_seen_ms)) {
            continue;
        }
        if (!is_valid_pkg_token(e.package_name)) continue;
        if (!std::isfinite(e.uclamp_bias)) e.uclamp_bias = 1.0;
        e.uclamp_bias = std::clamp(e.uclamp_bias, 0.78, 1.18);
        if (!std::isfinite(e.avg_thermal_slope)) e.avg_thermal_slope = 0.0;
        e.avg_thermal_slope = std::clamp(e.avg_thermal_slope, -20.0, 20.0);
        if (!std::isfinite(e.avg_jank_ratio)) e.avg_jank_ratio = 0.0;
        e.avg_jank_ratio = std::clamp(e.avg_jank_ratio, 0.0, 1.0);
        entries_[e.package_name] = std::move(e);
    }
}

void AppDnaStore::evict_lru_if_needed() noexcept {
    while (entries_.size() >= kMaxEntries) {
        auto victim = entries_.begin();
        for (auto it = entries_.begin(); it != entries_.end(); ++it) {
            if (it->second.last_seen_ms < victim->second.last_seen_ms) {
                victim = it;
            }
        }
        entries_.erase(victim);
    }
}

void AppDnaStore::record_launch(const std::string& package_name, uint64_t now_ms) noexcept {
    if (!is_valid_pkg_token(package_name)) return;
    auto it = entries_.find(package_name);
    if (it == entries_.end()) {
        evict_lru_if_needed();
        AppDnaEntry e{};
        e.package_name = package_name;
        e.launch_count = 1U;
        e.last_seen_ms = now_ms;
        entries_.emplace(package_name, std::move(e));
    } else {
        it->second.launch_count = std::min<uint32_t>(it->second.launch_count + 1U, 100000U);
        it->second.last_seen_ms = now_ms;
    }
    dirty_ = true;
}

void AppDnaStore::observe(
    const std::string& package_name,
    double jank_ratio,
    double thermal_velocity_cpm,
    uint64_t now_ms) noexcept {
    if (!is_valid_pkg_token(package_name)) return;
    auto it = entries_.find(package_name);
    if (it == entries_.end()) {
        evict_lru_if_needed();
        AppDnaEntry e{};
        e.package_name = package_name;
        e.last_seen_ms = now_ms;
        it = entries_.emplace(package_name, std::move(e)).first;
    }

    AppDnaEntry& e = it->second;
    e.last_seen_ms = now_ms;
    e.sample_count = std::min<uint32_t>(e.sample_count + 1U, 1000000U);

    if (std::isfinite(thermal_velocity_cpm)) {
        const double clamped_vel = std::clamp(thermal_velocity_cpm, -20.0, 20.0);
        e.avg_thermal_slope = e.sample_count <= 1U
            ? clamped_vel
            : (e.avg_thermal_slope * 0.85 + clamped_vel * 0.15);
    }

    if (std::isfinite(jank_ratio) && jank_ratio >= 0.0) {
        const double clamped_jank = std::clamp(jank_ratio, 0.0, 1.0);
        e.avg_jank_ratio = e.sample_count <= 1U
            ? clamped_jank
            : (e.avg_jank_ratio * 0.80 + clamped_jank * 0.20);

        // Closed-loop DNA convergence:
        // - Consistently smooth (< 1.5% jank) -> gently nudge floor bias down (saves power/heat)
        // - Consistently janky (>= 5% jank)   -> nudge floor bias up (within safe 1.18 ceiling)
        if (clamped_jank < 0.015) {
            e.uclamp_bias = std::max(0.78, e.uclamp_bias - 0.008);
        } else if (clamped_jank >= 0.05) {
            const double step = std::min(0.025, 0.01 + (clamped_jank - 0.05) * 0.05);
            e.uclamp_bias = std::min(1.18, e.uclamp_bias + step);
        }
    } else if (std::isfinite(thermal_velocity_cpm) && thermal_velocity_cpm >= 3.0) {
        // Thermally aggressive app without frame feedback: ease bias gently to prevent cliff throttle
        e.uclamp_bias = std::max(0.84, e.uclamp_bias - 0.004);
    }

    dirty_ = true;
}

double AppDnaStore::uclamp_bias_for(const std::string& package_name) const noexcept {
    const auto it = entries_.find(package_name);
    if (it == entries_.end() || it->second.sample_count < 3U) {
        return 1.0;
    }
    return std::clamp(it->second.uclamp_bias, 0.78, 1.18);
}

bool AppDnaStore::flush_if_dirty() noexcept {
    if (!dirty_ || store_path_.empty()) return true;
    std::error_code ec;
    const fs::path parent = fs::path(store_path_).parent_path();
    if (!parent.empty()) fs::create_directories(parent, ec);

    std::ostringstream out;
    out << "CFDNA1\n";
    out << std::fixed << std::setprecision(4);
    for (const auto& [_, e] : entries_) {
        out << e.package_name << ' '
            << e.uclamp_bias << ' '
            << e.avg_thermal_slope << ' '
            << e.avg_jank_ratio << ' '
            << e.launch_count << ' '
            << e.sample_count << ' '
            << e.last_seen_ms << '\n';
    }
    if (!write_file_atomic(store_path_, out.str())) return false;
    dirty_ = false;
    return true;
}

std::vector<AppDnaEntry> AppDnaStore::top_entries(size_t max_count) const {
    std::vector<AppDnaEntry> list;
    list.reserve(entries_.size());
    for (const auto& [_, e] : entries_) {
        list.push_back(e);
    }
    std::sort(list.begin(), list.end(), [](const AppDnaEntry& a, const AppDnaEntry& b) {
        if (a.launch_count != b.launch_count) return a.launch_count > b.launch_count;
        return a.last_seen_ms > b.last_seen_ms;
    });
    if (list.size() > max_count) list.resize(max_count);
    return list;
}

} // namespace coreflow::v6
