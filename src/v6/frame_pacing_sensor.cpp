/**
 * @file frame_pacing_sensor.cpp
 * @brief CoreFlow v6.2: Zero-fork Closed-Loop Display Frame Pacing Sensor.
 * @license Apache-2.0
 */
#include "coreflow/v6/frame_pacing_sensor.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>

namespace fs = std::filesystem;

namespace coreflow::v6 {

namespace {

std::string to_lower_copy(std::string_view text) {
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

std::string read_small_text(const std::string& path) noexcept {
    std::ifstream in(path);
    if (!in.is_open()) return {};
    std::string buf(256, '\0');
    in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
    buf.resize(static_cast<size_t>(in.gcount()));
    return buf;
}

bool parse_u64_exact(std::string_view token, uint64_t& out) noexcept {
    if (token.empty() || token.size() > 20U) return false;
    const auto r = std::from_chars(token.data(), token.data() + token.size(), out);
    return r.ec == std::errc{} && r.ptr == token.data() + token.size();
}

std::string find_vblank_irq_token_once(const std::string& interrupts_path) noexcept {
    std::ifstream in(interrupts_path);
    if (!in.is_open()) return {};
    std::string line;
    uint64_t best_score = 0U;
    std::string best_token;
    size_t lines_read = 0U;
    while (lines_read++ < 1024U && std::getline(in, line)) {
        const std::string low = to_lower_copy(line);
        uint64_t weight = 0U;
        if (low.find("vblank") != std::string::npos || low.find("te_irq") != std::string::npos) {
            weight = 4U;
        } else if (low.find("mdss") != std::string::npos || low.find("sde") != std::string::npos ||
                   low.find("dpu") != std::string::npos || low.find("exynos-drm") != std::string::npos ||
                   low.find("decon") != std::string::npos || low.find("mtk_disp") != std::string::npos) {
            weight = 3U;
        } else if (low.find("dsi") != std::string::npos || low.find("disp") != std::string::npos ||
                   low.find("ovl") != std::string::npos) {
            weight = 2U;
        }
        if (weight == 0U) continue;
        std::istringstream ls(line);
        std::string tok;
        if (!(ls >> tok) || tok.empty() || tok.back() != ':') continue;
        uint64_t sum = 0U;
        std::string col;
        while (ls >> col) {
            uint64_t n = 0U;
            if (!parse_u64_exact(col, n)) break;
            sum += n;
        }
        const uint64_t score = weight * 1'000'000'000ULL + std::min<uint64_t>(sum, 999'999'999ULL);
        if (score > best_score) {
            best_score = score;
            best_token = tok;
        }
    }
    return best_token;
}

bool read_vblank_irq_total(const std::string& interrupts_path,
                           const std::string& irq_token,
                           uint64_t& out_total) noexcept {
    if (interrupts_path.empty() || irq_token.empty()) return false;
    std::ifstream in(interrupts_path);
    if (!in.is_open()) return false;
    std::string line;
    size_t lines_read = 0U;
    while (lines_read++ < 1024U && std::getline(in, line)) {
        size_t pos = 0U;
        while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
        if (line.compare(pos, irq_token.size(), irq_token) != 0) continue;
        const size_t after = pos + irq_token.size();
        if (after < line.size() && line[after] != ' ' && line[after] != '\t') continue;

        std::istringstream ls(line.substr(after));
        uint64_t sum = 0U;
        bool found_any = false;
        std::string col;
        while (ls >> col) {
            uint64_t n = 0U;
            if (!parse_u64_exact(col, n)) break;
            sum += n;
            found_any = true;
        }
        if (found_any) {
            out_total = sum;
            return true;
        }
        return false;
    }
    return false;
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
    const std::string& procfs_root,
    const std::string& cached_vblank_irq_token) noexcept {
    fps_paths_.clear();
    proc_interrupts_path_.clear();
    vblank_irq_token_.clear();
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

    // 4. Universal Hardware Fallback: cached /proc/interrupts DSI/DRM VBlank IRQ counter
    //    (used when vendor FPS sysfs nodes are absent on custom kernels)
    if (fps_paths_.empty()) {
        const fs::path irq_path = fs::path(procfs_root) / "interrupts";
        if (is_readable_file(irq_path)) {
            const std::string token = cached_vblank_irq_token.empty()
                ? find_vblank_irq_token_once(irq_path.string())
                : cached_vblank_irq_token;
            if (!token.empty()) {
                proc_interrupts_path_ = irq_path.string();
                vblank_irq_token_ = token;
            }
        }
    }
}

void FramePacingSensor::reset() noexcept {
    ewma_jank_ = -1.0;
    prev_measured_fps_ = -1.0;
    ewma_jitter_ = 0.0;
    prev_vblank_count_ = 0U;
    prev_vblank_ms_ = 0U;
    has_vblank_baseline_ = false;
    last_sample_ms_ = 0U;
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

    if (!active_workload || !supported()) {
        ewma_jank_ = -1.0;
        prev_measured_fps_ = -1.0;
        ewma_jitter_ = 0.0;
        has_vblank_baseline_ = false;
        last_sample_ = out;
        return out;
    }

    // Rate-limit reads to at most once every 800 ms.
    if (last_sample_ms_ > 0U && now_ms >= last_sample_ms_ && (now_ms - last_sample_ms_) < 800ULL) {
        return last_sample_;
    }
    last_sample_ms_ = now_ms;

    double measured = -1.0;
    if (!fps_paths_.empty()) {
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
    } else if (!vblank_irq_token_.empty()) {
        uint64_t cur_irqs = 0U;
        if (read_vblank_irq_total(proc_interrupts_path_, vblank_irq_token_, cur_irqs)) {
            if (!has_vblank_baseline_ || now_ms <= prev_vblank_ms_ || cur_irqs < prev_vblank_count_) {
                prev_vblank_count_ = cur_irqs;
                prev_vblank_ms_ = now_ms;
                has_vblank_baseline_ = true;
                active_source_ = "vblank_irq:" + vblank_irq_token_;
                out.available = true;
                out.jank_ratio = -1.0;
                last_sample_ = out;
                return out;
            }
            const double dt_sec = static_cast<double>(now_ms - prev_vblank_ms_) / 1000.0;
            const uint64_t delta_irqs = cur_irqs - prev_vblank_count_;
            prev_vblank_count_ = cur_irqs;
            prev_vblank_ms_ = now_ms;
            if (dt_sec >= 0.25 && dt_sec <= 10.0) {
                const double rate = static_cast<double>(delta_irqs) / dt_sec;
                if (std::isfinite(rate) && rate >= 0.0 && rate <= 360.0) {
                    measured = rate;
                    active_source_ = "vblank_irq:" + vblank_irq_token_;
                }
            }
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
        prev_measured_fps_ = -1.0;
        ewma_jitter_ = 0.0;
        out.jank_ratio = -1.0;
        last_sample_ = out;
        return out;
    }

    // Infer the workload's natural cadence anchor (30, 60, 90, 120, 144 Hz <= target_hz)
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
    // Cadence jitter variance against prior sample detects micro-stutter oscillation even when mean FPS looks high.
    double jitter_penalty = 0.0;
    if (prev_measured_fps_ >= 12.0 && std::abs(prev_measured_fps_ - cadence_target) <= cadence_target * 0.45) {
        const double delta_norm = std::abs(measured - prev_measured_fps_) / std::max(cadence_target, 1.0);
        ewma_jitter_ = ewma_jitter_ * 0.65 + delta_norm * 0.35;
        if (ewma_jitter_ > 0.06) {
            jitter_penalty = std::clamp((ewma_jitter_ - 0.06) * 0.70, 0.0, 0.25);
        }
    }
    prev_measured_fps_ = measured;

    // Tolerance band: <= 3% jitter around vsync (e.g. 58.2 fps on 60 Hz) counts as 0% jank.
    const double base_jank = deficit <= 0.03 ? 0.0 : std::clamp((deficit - 0.03) / 0.45, 0.0, 1.0);
    const double instant_jank = std::clamp(base_jank + jitter_penalty, 0.0, 1.0);
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
