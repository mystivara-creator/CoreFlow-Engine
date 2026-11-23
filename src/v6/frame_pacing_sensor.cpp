/**
 * @file frame_pacing_sensor.cpp
 * @brief CoreFlow v6.2+: Zero-fork, Zero-heap Closed-Loop Display Frame Pacing Sensor
 *        with Non-LTPO Fixed-Hz Panel-Clock Discriminator (Strict Fail-Closed Probation).
 * @license Apache-2.0
 */
#include "coreflow/v6/frame_pacing_sensor.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

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

bool parse_u64_exact(std::string_view token, uint64_t& out) noexcept {
    if (token.empty() || token.size() > 20U) return false;
    const auto r = std::from_chars(token.data(), token.data() + token.size(), out);
    return r.ec == std::errc{} && r.ptr == token.data() + token.size();
}

// Zero-heap bounded read of small sysfs/procfs text files into caller's stack buffer.
size_t read_small_buf(const char* path, char* buf, size_t cap) noexcept {
    if (path == nullptr || path[0] == '\0' || buf == nullptr || cap < 2U) return 0U;
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return 0U;
    size_t total = 0U;
    while (total + 1U < cap) {
        const ssize_t n = ::read(fd, buf + total, cap - 1U - total);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        total += static_cast<size_t>(n);
    }
    (void)::close(fd);
    buf[total] = '\0';
    return total;
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

bool match_and_sum_irq_line(std::string_view line,
                            std::string_view irq_token,
                            uint64_t& out_total) noexcept {
    size_t pos = 0U;
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
    if (line.size() - pos < irq_token.size()) return false;
    if (line.compare(pos, irq_token.size(), irq_token) != 0) return false;
    size_t cur = pos + irq_token.size();
    if (cur < line.size() && line[cur] != ' ' && line[cur] != '\t') return false;

    uint64_t sum = 0U;
    bool found_any = false;
    while (cur < line.size()) {
        while (cur < line.size() && (line[cur] == ' ' || line[cur] == '\t' || line[cur] == '\r')) {
            ++cur;
        }
        if (cur >= line.size()) break;
        size_t end = cur;
        while (end < line.size() && line[end] != ' ' && line[end] != '\t' && line[end] != '\r') {
            ++end;
        }
        uint64_t n = 0U;
        if (!parse_u64_exact(line.substr(cur, end - cur), n)) break;
        sum += n;
        found_any = true;
        cur = end;
    }
    if (!found_any) return false;
    out_total = sum;
    return true;
}

// Zero-heap streaming reader for /proc/interrupts matching a cached IRQ token.
bool read_vblank_irq_total(const std::string& interrupts_path,
                           const std::string& irq_token,
                           uint64_t& out_total) noexcept {
    if (interrupts_path.empty() || irq_token.empty()) return false;
    const int fd = ::open(interrupts_path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;

    char chunk[4096];
    char line_buf[512];
    size_t line_len = 0U;
    bool line_overflow = false;
    size_t lines_read = 0U;
    bool matched = false;

    while (!matched && lines_read < 1024U) {
        const ssize_t n = ::read(fd, chunk, sizeof(chunk));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        for (ssize_t i = 0; i < n; ++i) {
            const char ch = chunk[i];
            if (ch == '\n') {
                ++lines_read;
                if (!line_overflow && line_len > 0U &&
                    match_and_sum_irq_line(std::string_view(line_buf, line_len), irq_token, out_total)) {
                    matched = true;
                    break;
                }
                line_len = 0U;
                line_overflow = false;
                if (lines_read >= 1024U) break;
            } else if (!line_overflow) {
                if (line_len < sizeof(line_buf)) {
                    line_buf[line_len++] = ch;
                } else {
                    line_overflow = true;
                }
            }
        }
    }
    if (!matched && !line_overflow && line_len > 0U && lines_read < 1024U) {
        matched = match_and_sum_irq_line(std::string_view(line_buf, line_len), irq_token, out_total);
    }
    (void)::close(fd);
    return matched;
}

double parse_fps_token(std::string_view text) noexcept {
    // Supports formats like:
    //   "fps: 59.8 duration: 1000.0 frame_count: 60" / "measured_fps: 119.4" / "120" / "59.94" / "fps=60"
    size_t pos = 0U;
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
                // Guard against inconsistent stale Qualcomm SDE payloads reporting non-zero fps with frame_count: 0.
                const size_t fc_pos = text.find("frame_count:");
                if (fc_pos != std::string_view::npos && val >= 12.0) {
                    size_t p = fc_pos + 12U;
                    while (p < text.size() && (text[p] == ' ' || text[p] == '\t')) ++p;
                    size_t e = p;
                    while (e < text.size() && text[e] >= '0' && text[e] <= '9') ++e;
                    uint64_t fc = 0U;
                    if (e > p && parse_u64_exact(text.substr(p, e - p), fc) && fc == 0U) {
                        return 0.0;
                    }
                }
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
    struct stat st{};
    return ::lstat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode) && ::access(p.c_str(), R_OK) == 0;
}

} // namespace

void FramePacingSensor::discover(
    const std::string& sysfs_root,
    const std::string& procfs_root,
    const std::string& cached_vblank_irq_token) noexcept {
    verified_fps_paths_.clear();
    unverified_fps_paths_.clear();
    proc_interrupts_path_.clear();
    vblank_irq_token_.clear();
    active_source_.clear();
    active_source_.reserve(128U);
    source_tier_ = FrameSourceTier::None;
    tier_b_state_ = TierBVerificationState::Unproven;
    static_pinned_streak_ = 0U;
    reset();

    auto add_unique = [](std::vector<std::string>& dst, const fs::path& p) {
        if (is_readable_file(p)) {
            const std::string s = p.string();
            if (std::find(dst.begin(), dst.end(), s) == dst.end()) {
                dst.push_back(s);
            }
        }
    };

    std::error_code ec;
    // 1. Qualcomm DRM SDE measured_fps (Tier A) vs generic crtc fps (Tier B)
    const fs::path drm_dir = fs::path(sysfs_root) / "class/drm";
    if (fs::is_directory(drm_dir, ec) && !ec) {
        for (fs::directory_iterator card_it(drm_dir, fs::directory_options::skip_permission_denied, ec), end;
             !ec && card_it != end; card_it.increment(ec)) {
            const std::string cname = card_it->path().filename().string();
            if (cname.rfind("sde-crtc-", 0) == 0 || cname.rfind("crtc-", 0) == 0) {
                add_unique(verified_fps_paths_, card_it->path() / "measured_fps");
                add_unique(unverified_fps_paths_, card_it->path() / "fps");
                continue;
            }
            if (cname.rfind("card", 0) != 0) continue;
            std::error_code sub_ec;
            for (fs::directory_iterator crtc_it(card_it->path(), fs::directory_options::skip_permission_denied, sub_ec), sub_end;
                 !sub_ec && crtc_it != sub_end; crtc_it.increment(sub_ec)) {
                const std::string crtc = crtc_it->path().filename().string();
                if (crtc.rfind("sde-crtc-", 0) == 0 || crtc.rfind("crtc-", 0) == 0) {
                    add_unique(verified_fps_paths_, crtc_it->path() / "measured_fps");
                    add_unique(unverified_fps_paths_, crtc_it->path() / "fps");
                }
            }
        }
    }

    // 2. Xiaomi mi_display: measured_fps (Tier A) vs panel fps (Tier B)
    const fs::path mi_disp = fs::path(sysfs_root) / "class/mi_display";
    if (fs::is_directory(mi_disp, ec) && !ec) {
        for (fs::directory_iterator it(mi_disp, fs::directory_options::skip_permission_denied, ec), end;
             !ec && it != end; it.increment(ec)) {
            add_unique(verified_fps_paths_, it->path() / "measured_fps");
            add_unique(unverified_fps_paths_, it->path() / "fps");
        }
    }

    // 3. Qualcomm MDP / MediaTek FPSGO (Tier A) vs generic panel/fb fps (Tier B)
    add_unique(verified_fps_paths_, fs::path(sysfs_root) / "class/graphics/fb0/measured_fps");
    add_unique(verified_fps_paths_, fs::path(sysfs_root) / "kernel/fpsgo/fstb/fpsgo_status");
    add_unique(verified_fps_paths_, fs::path(procfs_root) / "mtk_fpsgo/fps");

    add_unique(unverified_fps_paths_, fs::path(sysfs_root) / "class/graphics/fb0/fps");
    add_unique(unverified_fps_paths_, fs::path(sysfs_root) / "class/drm/card0-DSI-1/fps");
    add_unique(unverified_fps_paths_, fs::path(sysfs_root) / "class/panel/panel/fps");

    if (!verified_fps_paths_.empty()) {
        source_tier_ = FrameSourceTier::VerifiedCommit;
        active_source_ = verified_fps_paths_.front();
        return;
    }

    if (!unverified_fps_paths_.empty()) {
        source_tier_ = FrameSourceTier::UnverifiedScanout;
        active_source_ = unverified_fps_paths_.front();
        return;
    }

    // 4. Universal Hardware Fallback (Tier B, strict fail-closed probation):
    //    cached /proc/interrupts DSI/DRM VBlank IRQ counter.
    const fs::path irq_path = fs::path(procfs_root) / "interrupts";
    if (is_readable_file(irq_path)) {
        const std::string token = cached_vblank_irq_token.empty()
            ? find_vblank_irq_token_once(irq_path.string())
            : cached_vblank_irq_token;
        if (!token.empty()) {
            proc_interrupts_path_ = irq_path.string();
            vblank_irq_token_ = token;
            source_tier_ = FrameSourceTier::UnverifiedScanout;
            active_source_ = "vblank_irq:" + vblank_irq_token_;
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
    AndroidFrameEventBridge* bridge,
    bool screen_idle_hint) noexcept {
    const double target_hz = static_cast<double>(std::clamp(target_refresh_hz, 30U, 240U));
    FramePacingSample out{};
    out.target_fps = target_hz;
    out.verified_commit_source = verified();
    out.quarantined_fixed_hz = quarantined_fixed_hz();

    if (source_tier_ == FrameSourceTier::None ||
        tier_b_state_ == TierBVerificationState::QuarantinedFixedHz) {
        last_sample_ = out;
        return out;
    }

    const bool probing_tier_b =
        (source_tier_ == FrameSourceTier::UnverifiedScanout) &&
        (tier_b_state_ == TierBVerificationState::Unproven) &&
        screen_idle_hint;

    if (!active_workload && !probing_tier_b) {
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
    char text_buf[256];

    auto read_first_from = [&](const std::vector<std::string>& paths) -> double {
        for (const auto& path : paths) {
            const size_t n = read_small_buf(path.c_str(), text_buf, sizeof(text_buf));
            if (n == 0U) continue;
            const double parsed = parse_fps_token(std::string_view(text_buf, n));
            if (parsed >= 0.0) {
                if (active_source_ != path) active_source_ = path;
                return parsed;
            }
        }
        return -1.0;
    };

    if (source_tier_ == FrameSourceTier::VerifiedCommit) {
        measured = read_first_from(verified_fps_paths_);
    } else if (!unverified_fps_paths_.empty()) {
        measured = read_first_from(unverified_fps_paths_);
    } else if (!vblank_irq_token_.empty()) {
        uint64_t cur_irqs = 0U;
        if (read_vblank_irq_total(proc_interrupts_path_, vblank_irq_token_, cur_irqs)) {
            if (!has_vblank_baseline_ || now_ms <= prev_vblank_ms_ || cur_irqs < prev_vblank_count_) {
                prev_vblank_count_ = cur_irqs;
                prev_vblank_ms_ = now_ms;
                has_vblank_baseline_ = true;
                out.available = verified() && active_workload;
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
                }
            }
        }
    }

    if (measured < 0.0) {
        last_sample_ = out;
        return out;
    }

    // Strict fail-closed probation & non-LTPO fixed-Hz panel-clock discriminator for Tier B sources.
    if (source_tier_ == FrameSourceTier::UnverifiedScanout &&
        tier_b_state_ == TierBVerificationState::Unproven) {
        if (screen_idle_hint) {
            if (measured < 18.0 || measured < target_hz * 0.50) {
                // Static idle screen caused presentation rate to drop -> proven commit-driven / LTPO!
                tier_b_state_ = TierBVerificationState::ProvenCommit;
                static_pinned_streak_ = 0U;
                out.verified_commit_source = true;
            } else if (measured >= target_hz * 0.92) {
                ++static_pinned_streak_;
                if (static_pinned_streak_ >= kFixedHzQuarantineStrikes) {
                    // Panel refresh clock stayed locked at target_hz across 3 static-idle probes:
                    // quarantine fail-closed for this boot so fixed-Hz panels never fabricate 0% jank.
                    tier_b_state_ = TierBVerificationState::QuarantinedFixedHz;
                    active_source_ = "fail_closed:fixed_hz_panel";
                    out.available = false;
                    out.verified_commit_source = false;
                    out.quarantined_fixed_hz = true;
                    out.jank_ratio = -1.0;
                    last_sample_ = out;
                    return out;
                }
            } else {
                static_pinned_streak_ = 0U;
            }
        }
        if (tier_b_state_ != TierBVerificationState::ProvenCommit || !active_workload) {
            // Remain strictly fail-closed until proven AND actively rendering.
            out.available = false;
            out.jank_ratio = -1.0;
            last_sample_ = out;
            return out;
        }
    }

    if (!active_workload) {
        out.available = false;
        out.jank_ratio = -1.0;
        last_sample_ = out;
        return out;
    }

    out.available = true;
    out.verified_commit_source = true;
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
