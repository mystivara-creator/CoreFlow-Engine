/**
 * @file frame_pacing_sensor.cpp
 * @brief CoreFlow v6.2: Zero-fork Closed-Loop Display Frame Pacing Sensor.
 * @license Apache-2.0
 */
#include "coreflow/v6/frame_pacing_sensor.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string_view>

namespace fs = std::filesystem;

namespace coreflow::v6 {

namespace {

std::string read_small_text(const std::string& path) noexcept {
    std::ifstream in(path);
    if (!in.is_open()) return {};
    std::string buf(256, '\0');
    in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
    buf.resize(static_cast<size_t>(in.gcount()));
    return buf;
}

double parse_fps_token(std::string_view text) noexcept {
    // Supports formats like:
    //   "fps: 59.8" / "measured_fps: 119.4" / "120" / "59.94" / "fps=60"
    size_t pos = 0;
    while (pos < text.size()) {
        const char ch = text[pos];
        if (ch >= '0' && ch <= '9') {
            size_t end = pos;
            bool seen_dot = false;
            while (end < text.size()) {
                const char c = text[end];
                if (c >= '0' && c <= '9') {
                    ++end;
                } else if (c == '.' && !seen_dot) {
                    seen_dot = true;
                    ++end;
                } else {
                    break;
                }
            }
            double val = 0.0;
            const auto r = std::from_chars(text.data() + pos, text.data() + end, val);
            if (r.ec == std::errc{} && std::isfinite(val) && val >= 0.0 && val <= 360.0) {
                return val;
            }
            pos = end;
        } else {
            ++pos;
        }
    }
    return -1.0;
}

bool is_readable_file(const fs::path& p) noexcept {
    std::error_code ec;
    if (!fs::is_regular_file(p, ec) || ec) return false;
    std::ifstream in(p);
    return in.is_open();
}

} // namespace

void FramePacingSensor::discover(
    const std::string& sysfs_root,
    const std::string& procfs_root) noexcept {
    fps_paths_.clear();
    active_source_.clear();
    reset();

    auto add_if_readable = [&](const fs::path& p) {
        if (is_readable_file(p)) {
            const std::string s = p.string();
            if (std::find(fps_paths_.begin(), fps_paths_.end(), s) == fps_paths_.end()) {
                fps_paths_.push_back(s);
            }
        }
    };

    std::error_code ec;
    // 1. Qualcomm DRM SDE measured_fps (most accurate live hardware presentation cadence)
    const fs::path drm_dir = fs::path(sysfs_root) / "class/drm";
    if (fs::is_directory(drm_dir, ec) && !ec) {
        for (fs::directory_iterator card_it(drm_dir, fs::directory_options::skip_permission_denied, ec), end;
             !ec && card_it != end; card_it.increment(ec)) {
            const std::string cname = card_it->path().filename().string();
            if (cname.rfind("sde-crtc-", 0) == 0 || cname.rfind("crtc-", 0) == 0) {
                add_if_readable(card_it->path() / "measured_fps");
                add_if_readable(card_it->path() / "fps");
                continue;
            }
            if (cname.rfind("card", 0) != 0) continue;
            std::error_code sub_ec;
            for (fs::directory_iterator crtc_it(card_it->path(), fs::directory_options::skip_permission_denied, sub_ec), sub_end;
                 !sub_ec && crtc_it != sub_end; crtc_it.increment(sub_ec)) {
                const std::string crtc = crtc_it->path().filename().string();
                if (crtc.rfind("sde-crtc-", 0) == 0 || crtc.rfind("crtc-", 0) == 0) {
                    add_if_readable(crtc_it->path() / "measured_fps");
                    add_if_readable(crtc_it->path() / "fps");
                }
            }
        }
    }

    // 2. Xiaomi mi_display live fps
    const fs::path mi_disp = fs::path(sysfs_root) / "class/mi_display";
    if (fs::is_directory(mi_disp, ec) && !ec) {
        for (fs::directory_iterator it(mi_disp, fs::directory_options::skip_permission_denied, ec), end;
             !ec && it != end; it.increment(ec)) {
            add_if_readable(it->path() / "measured_fps");
            add_if_readable(it->path() / "fps");
        }
    }

    // 3. Qualcomm MDP / MediaTek / Exynos / Pixel display fps nodes
    add_if_readable(fs::path(sysfs_root) / "class/graphics/fb0/measured_fps");
    add_if_readable(fs::path(sysfs_root) / "class/graphics/fb0/fps");
    add_if_readable(fs::path(sysfs_root) / "class/drm/card0-DSI-1/fps");
    add_if_readable(fs::path(sysfs_root) / "class/panel/panel/fps");
    add_if_readable(fs::path(sysfs_root) / "kernel/fpsgo/fstb/fpsgo_status");
    add_if_readable(fs::path(procfs_root) / "mtk_fpsgo/fps");
}

void FramePacingSensor::reset() noexcept {
    ewma_jank_ = -1.0;
    last_sample_ms_ = 0;
    last_sample_ = {};
}

FramePacingSample FramePacingSensor::sample(
    uint32_t target_refresh_hz,
    uint64_t now_ms,
    bool active_workload,
    AndroidFrameEventBridge* bridge) noexcept {
    const double target_hz = static_cast<double>(std::clamp(target_refresh_hz, 30U, 240U));
    FramePacingSample out{};
    out.target_fps = target_hz;

    if (!active_workload || fps_paths_.empty()) {
        ewma_jank_ = -1.0;
        last_sample_ = out;
        return out;
    }

    // Rate-limit sysfs reads to at most once every 800 ms.
    if (last_sample_ms_ > 0U && now_ms >= last_sample_ms_ && (now_ms - last_sample_ms_) < 800ULL) {
        return last_sample_;
    }
    last_sample_ms_ = now_ms;

    double measured = -1.0;
    for (const auto& path : fps_paths_) {
        const std::string raw = read_small_text(path);
        if (raw.empty()) continue;
        const double parsed = parse_fps_token(raw);
        if (parsed >= 0.0) {
            measured = parsed;
            active_source_ = path;
            break;
        }
    }

    if (measured < 0.0) {
        last_sample_ = out;
        return out;
    }

    out.available = true;
    out.measured_fps = measured;

    // Below 12 FPS on Android indicates a static/paused UI (no frames being submitted),
    // NOT a janking animation — keep jank_ratio neutral (-1.0) to avoid false boosts.
    if (measured < 12.0) {
        ewma_jank_ = -1.0;
        out.jank_ratio = -1.0;
        last_sample_ = out;
        return out;
    }

    // Infer the workload's natural cadence anchor (30, 45, 60, 90, 120, 144 Hz <= target_hz)
    // so a rock-solid 60 FPS game on a 120 Hz panel is evaluated against 60 FPS, NOT penalized as 50% jank!
    double cadence_target = target_hz;
    if (target_hz >= 115.0 && measured >= 54.0 && measured <= 63.0) {
        cadence_target = 60.0;
    } else if (target_hz >= 85.0 && measured >= 54.0 && measured <= 63.0) {
        cadence_target = 60.0;
    } else if (measured >= 27.5 && measured <= 32.0) {
        cadence_target = 30.0;
    } else if (target_hz >= 115.0 && measured >= 84.0 && measured <= 93.0) {
        cadence_target = 90.0;
    }
    out.target_fps = cadence_target;

    const double deficit = std::max(0.0, cadence_target - measured) / std::max(cadence_target, 1.0);
    // Tolerance band: <= 3% jitter around vsync (e.g. 58.2 fps on 60 Hz) counts as 0% jank.
    const double instant_jank = deficit <= 0.03 ? 0.0 : std::clamp((deficit - 0.03) / 0.45, 0.0, 1.0);
    ewma_jank_ = (ewma_jank_ < 0.0) ? instant_jank : (ewma_jank_ * 0.65 + instant_jank * 0.35);
    out.jank_ratio = ewma_jank_;
    last_sample_ = out;

    if (bridge != nullptr && now_ms > 0U) {
        FrameEvent ev{};
        ev.kind = FrameEventKind::FrameTimelineWindow;
        ev.vsync_timestamp_ns = now_ms * 1000000ULL;
        const uint32_t frame_cnt = static_cast<uint32_t>(std::max(1.0, std::round(cadence_target)));
        ev.frame_count = frame_cnt;
        ev.missed_frames = static_cast<uint32_t>(std::round(static_cast<double>(frame_cnt) * out.jank_ratio));
        ev.jank_threshold_ns = static_cast<uint64_t>(1000000000.0 / std::max(cadence_target, 1.0));
        (void)bridge->submit(ev);
    }

    return out;
}

} // namespace coreflow::v6
