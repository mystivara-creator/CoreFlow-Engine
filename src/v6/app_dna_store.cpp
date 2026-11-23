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
    const int fd = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600);
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
    flush_count_ = 0U;
    if (state_dir.empty()) {
        store_path_.clear();
        return;
    }
    store_path_ = (fs::path(state_dir) / "app_dna.map").string();
    std::ifstream in(store_path_);
    if (!in.is_open()) return;

    std::string header;
    if (!std::getline(in, header)) return;
    const bool is_v2 = (header.rfind("CFDNA2", 0) == 0);
    const bool is_v1 = (header.rfind("CFDNA1", 0) == 0);
    if (!is_v2 && !is_v1) return;

    std::string line;
    while (std::getline(in, line) && entries_.size() < kMaxEntries) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        AppDnaEntry e{};
        if (is_v2) {
            if (!(ss >> e.package_name >> e.uclamp_bias >> e.avg_thermal_slope >> e.avg_jank_ratio >>
                  e.avg_power_mw >> e.efficiency_weight >> e.launch_count >> e.sample_count >> e.last_seen_ms)) {
                continue;
            }
        } else {
            if (!(ss >> e.package_name >> e.uclamp_bias >> e.avg_thermal_slope >> e.avg_jank_ratio >>
                  e.launch_count >> e.sample_count >> e.last_seen_ms)) {
                continue;
            }
            e.avg_power_mw = 0.0;
            e.efficiency_weight = 1.0;
        }
        if (!is_valid_pkg_token(e.package_name)) continue;
        if (!std::isfinite(e.uclamp_bias)) e.uclamp_bias = 1.0;
        e.uclamp_bias = std::clamp(e.uclamp_bias, 0.78, 1.18);
        if (!std::isfinite(e.avg_thermal_slope)) e.avg_thermal_slope = 0.0;
        e.avg_thermal_slope = std::clamp(e.avg_thermal_slope, -20.0, 20.0);
        if (!std::isfinite(e.avg_jank_ratio)) e.avg_jank_ratio = 0.0;
        e.avg_jank_ratio = std::clamp(e.avg_jank_ratio, 0.0, 1.0);
        if (!std::isfinite(e.avg_power_mw) || e.avg_power_mw < 0.0) e.avg_power_mw = 0.0;
        e.avg_power_mw = std::clamp(e.avg_power_mw, 0.0, 30000.0);
        if (!std::isfinite(e.efficiency_weight)) e.efficiency_weight = 1.0;
        e.efficiency_weight = std::clamp(e.efficiency_weight, 0.80, 1.15);
        e.persisted_uclamp_bias = e.uclamp_bias;
        e.persisted_efficiency_weight = e.efficiency_weight;
        e.persisted_on_disk = true;
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

void AppDnaStore::record_launch(
    const std::string& package_name,
    uint64_t now_ms,
    EngineSignature signature) noexcept {
    if (!is_valid_pkg_token(package_name)) return;
    auto it = entries_.find(package_name);
    if (it == entries_.end()) {
        evict_lru_if_needed();
        AppDnaEntry e{};
        e.package_name = package_name;
        e.launch_count = 1U;
        e.last_seen_ms = now_ms;
        if (signature == EngineSignature::HeavyGame3D) {
            e.uclamp_bias = kBayesianHeavyGamePrior;
            e.sample_count = kBayesianPriorSamples;
        }
        entries_.emplace(package_name, std::move(e));
    } else {
        it->second.launch_count = std::min<uint32_t>(it->second.launch_count + 1U, 100000U);
        it->second.last_seen_ms = now_ms;
        if (signature == EngineSignature::HeavyGame3D && it->second.sample_count < kBayesianPriorSamples) {
            it->second.uclamp_bias = kBayesianHeavyGamePrior;
            it->second.sample_count = kBayesianPriorSamples;
        }
    }
    dirty_ = true;
}

void AppDnaStore::observe_context(
    const std::string& package_name,
    const ContextLearningSample& sample,
    uint64_t now_ms,
    EngineSignature signature) noexcept {
    if (!is_valid_pkg_token(package_name)) return;
    // Skip learning noise during Boot-Calm / wake stabilization ramp so cold transients never pollute DNA.
    if (sample.in_wake_ramp) return;

    auto it = entries_.find(package_name);
    if (it == entries_.end()) {
        evict_lru_if_needed();
        AppDnaEntry e{};
        e.package_name = package_name;
        e.last_seen_ms = now_ms;
        if (signature == EngineSignature::HeavyGame3D) {
            e.uclamp_bias = kBayesianHeavyGamePrior;
            e.sample_count = kBayesianPriorSamples;
        }
        it = entries_.emplace(package_name, std::move(e)).first;
    } else if (signature == EngineSignature::HeavyGame3D && it->second.sample_count < kBayesianPriorSamples) {
        it->second.uclamp_bias = kBayesianHeavyGamePrior;
        it->second.sample_count = kBayesianPriorSamples;
    }

    AppDnaEntry& e = it->second;
    e.last_seen_ms = now_ms;
    e.sample_count = std::min<uint32_t>(e.sample_count + 1U, 1000000U);

    const double vel = std::isfinite(sample.thermal_velocity_cpm)
        ? std::clamp(sample.thermal_velocity_cpm, -20.0, 20.0)
        : 0.0;
    e.avg_thermal_slope = e.sample_count <= 1U
        ? vel
        : (e.avg_thermal_slope * 0.85 + vel * 0.15);

    const double pwr = (std::isfinite(sample.power_mw) && sample.power_mw > 0.0)
        ? std::clamp(sample.power_mw, 0.0, 30000.0)
        : 0.0;
    if (pwr > 0.0) {
        e.avg_power_mw = (e.avg_power_mw <= 1.0)
            ? pwr
            : (e.avg_power_mw * 0.85 + pwr * 0.15);
    }

    // Real-time Cost-to-Gain modulation in RAM:
    //   Cost = thermal rise velocity + instantaneous power draw + battery/thermal pressure
    //   Gain = frame pacing smoothness (jank_ratio + measured_fps vs target_fps)
    const double cap_scale = std::isfinite(sample.capacity_scale)
        ? std::clamp(sample.capacity_scale, 0.25, 1.0)
        : 1.0;
    const double thermal_cost = std::max(0.0, vel / 4.0);
    const double power_cost = pwr > 0.0 ? std::clamp(pwr / 4500.0, 0.0, 1.5) : 0.0;
    const double context_penalty =
        (sample.low_battery ? 0.35 : 0.0) +
        (sample.charging ? 0.18 : 0.0) +
        (cap_scale < 0.90 ? (0.90 - cap_scale) * 1.2 : 0.0);
    const double total_cost = thermal_cost + power_cost * 0.45 + context_penalty;

    if (std::isfinite(sample.jank_ratio) && sample.jank_ratio >= 0.0) {
        const double clamped_jank = std::clamp(sample.jank_ratio, 0.0, 1.0);
        e.avg_jank_ratio = e.sample_count <= 1U
            ? clamped_jank
            : (e.avg_jank_ratio * 0.80 + clamped_jank * 0.20);

        const double fps_deficit = (sample.measured_fps > 0.0 && sample.target_fps >= 30.0)
            ? std::clamp((sample.target_fps - sample.measured_fps) / sample.target_fps, 0.0, 1.0)
            : 0.0;

        // Cost-to-gain efficiency weight:
        // - Smooth & cool/low-power -> efficiency_weight trends toward 1.05..1.15 (efficient)
        // - High cost with poor frame pacing -> efficiency_weight trims toward 0.80..0.95 (diminishing return)
        if (clamped_jank < 0.015 && total_cost < 0.65) {
            e.efficiency_weight = std::min(1.15, e.efficiency_weight + 0.008);
        } else if (total_cost >= 1.0 && (clamped_jank < 0.02 || fps_deficit < 0.05)) {
            e.efficiency_weight = std::max(0.80, e.efficiency_weight - 0.015);
        } else if (total_cost >= 1.25) {
            e.efficiency_weight = std::max(0.80, e.efficiency_weight - 0.010);
        }

        // Closed-loop UClamp bias convergence modulated by cost-to-gain:
        if (clamped_jank < 0.015) {
            const double trim = 0.008 * (1.0 + std::min(total_cost, 1.0) * 0.5);
            e.uclamp_bias = std::max(0.78, e.uclamp_bias - trim);
        } else if (clamped_jank >= 0.05) {
            // Dampen upward boost when low battery, charging heat, or high cost-to-gain is active.
            const double damp = (sample.low_battery || cap_scale < 0.85) ? 0.45 :
                                (sample.charging || total_cost > 1.1) ? 0.70 : 1.0;
            const double step = std::min(0.025, (0.01 + (clamped_jank - 0.05) * 0.05) * damp);
            e.uclamp_bias = std::min(1.18, e.uclamp_bias + step);
        }
    } else if (vel >= 3.0 || total_cost >= 1.2) {
        // Thermally aggressive / costly app without frame feedback: ease bias & efficiency gently
        e.uclamp_bias = std::max(0.84, e.uclamp_bias - 0.004);
        e.efficiency_weight = std::max(0.82, e.efficiency_weight - 0.006);
    }

    dirty_ = true;
}

void AppDnaStore::observe(
    const std::string& package_name,
    double jank_ratio,
    double thermal_velocity_cpm,
    uint64_t now_ms,
    EngineSignature signature) noexcept {
    ContextLearningSample sample{};
    sample.jank_ratio = jank_ratio;
    sample.thermal_velocity_cpm = thermal_velocity_cpm;
    observe_context(package_name, sample, now_ms, signature);
}

double AppDnaStore::uclamp_bias_for(const std::string& package_name) const noexcept {
    const auto it = entries_.find(package_name);
    if (it == entries_.end() || it->second.sample_count < kBayesianPriorSamples) {
        return 1.0;
    }
    return std::clamp(it->second.uclamp_bias, 0.78, 1.18);
}

double AppDnaStore::efficiency_weight_for(const std::string& package_name) const noexcept {
    const auto it = entries_.find(package_name);
    if (it == entries_.end() || it->second.sample_count < kBayesianPriorSamples) {
        return 1.0;
    }
    return std::clamp(it->second.efficiency_weight, 0.80, 1.15);
}

bool AppDnaStore::has_significant_drift() const noexcept {
    if (!dirty_) return false;
    for (const auto& [_, e] : entries_) {
        if (!e.persisted_on_disk) return true;
        if (std::abs(e.uclamp_bias - e.persisted_uclamp_bias) >= kMinBiasDriftForFlush) return true;
        if (std::abs(e.efficiency_weight - e.persisted_efficiency_weight) >= kMinEfficiencyDriftForFlush) return true;
    }
    return false;
}

bool AppDnaStore::flush_atomic_now() noexcept {
    if (store_path_.empty()) return true;
    std::error_code ec;
    const fs::path parent = fs::path(store_path_).parent_path();
    if (!parent.empty()) fs::create_directories(parent, ec);

    std::ostringstream out;
    out << "CFDNA2\n";
    out << std::fixed << std::setprecision(4);
    for (const auto& [_, e] : entries_) {
        out << e.package_name << ' '
            << e.uclamp_bias << ' '
            << e.avg_thermal_slope << ' '
            << e.avg_jank_ratio << ' '
            << e.avg_power_mw << ' '
            << e.efficiency_weight << ' '
            << e.launch_count << ' '
            << e.sample_count << ' '
            << e.last_seen_ms << '\n';
    }
    if (!write_file_atomic(store_path_, out.str())) return false;
    for (auto& [_, e] : entries_) {
        e.persisted_uclamp_bias = e.uclamp_bias;
        e.persisted_efficiency_weight = e.efficiency_weight;
        e.persisted_on_disk = true;
    }
    dirty_ = false;
    ++flush_count_;
    return true;
}

bool AppDnaStore::flush_if_significant() noexcept {
    if (!has_significant_drift()) return true;
    return flush_atomic_now();
}

bool AppDnaStore::flush_if_dirty() noexcept {
    if (!dirty_ || store_path_.empty()) return true;
    return flush_atomic_now();
}

std::vector<AppDnaEntry> AppDnaStore::snapshot_entries() const {
    std::vector<AppDnaEntry> out;
    out.reserve(entries_.size());
    for (const auto& [_, entry] : entries_) {
        out.push_back(entry);
    }
    std::sort(out.begin(), out.end(), [](const AppDnaEntry& a, const AppDnaEntry& b) {
        if (a.last_seen_ms != b.last_seen_ms) return a.last_seen_ms > b.last_seen_ms;
        return a.sample_count > b.sample_count;
    });
    return out;
}

} // namespace coreflow::v6
