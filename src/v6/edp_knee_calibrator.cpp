/**
 * @file edp_knee_calibrator.cpp
 * @brief CoreFlow v6.2+ Phase 3: Online Silicon EDP Knee Discovery (`silicon_edp.map`, CFEDP1).
 * @license Apache-2.0
 */
#include "coreflow/v6/edp_knee_calibrator.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow::v6 {

namespace {

constexpr double kMinLogDeltaMw = 50.0;
constexpr double kMinPowerSpanMw = 250.0;
constexpr double kMinScaleSpan = 0.28;
constexpr double kMaxMeanRelNoise = 0.22;
constexpr double kMinFitR2 = 0.70;
constexpr double kMinMonotonicRatio = 0.75;

std::string sanitize_token(std::string_view raw) {
    std::string out;
    out.reserve(std::min<size_t>(raw.size(), 128U));
    for (unsigned char c : raw) {
        if (out.size() >= 128U) break;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_' ||
            c == ':' || c == '/') {
            out.push_back(static_cast<char>(c));
        } else if (c == ' ') {
            out.push_back('_');
        }
    }
    return out;
}

bool write_all_fd(int fd, std::string_view data) noexcept {
    size_t off = 0U;
    while (off < data.size()) {
        const ssize_t n = ::write(fd, data.data() + off, data.size() - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        off += static_cast<size_t>(n);
    }
    return true;
}

bool sync_fd_if_needed(int fd, bool allow_einval = false) noexcept {
#ifdef COREFLOW_ENABLE_TEST_HOOKS
    const char* skip = std::getenv("COREFLOW_TEST_SKIP_FSYNC");
    if (skip != nullptr && skip[0] == '1' && skip[1] == '\0') {
        return true;
    }
#endif
    for (;;) {
        if (::fsync(fd) == 0) return true;
        if (errno == EINTR) continue;
        if (allow_einval && (errno == EINVAL || errno == EROFS)) return true;
        return false;
    }
}

std::string read_small_raw(const std::string& path, size_t max_bytes = 8192U) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        (void)::close(fd);
        return {};
    }
    char buf[1024];
    std::string out;
    while (out.size() < max_bytes) {
        const ssize_t n = ::read(fd, buf, std::min(sizeof(buf), max_bytes - out.size()));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        out.append(buf, static_cast<size_t>(n));
    }
    (void)::close(fd);
    return out;
}

} // namespace

void EdpKneeCalibrator::initialize(
    const std::string& state_dir,
    const std::string& kernel_release,
    const std::string& rom_fingerprint) noexcept {
    state_dir_ = state_dir;
    map_path_ = state_dir.empty() ? std::string{} : (state_dir + "/silicon_edp.map");
    kernel_release_ = sanitize_token(kernel_release);
    if (kernel_release_.empty()) kernel_release_ = "unknown-kernel";
    rom_fingerprint_ = sanitize_token(rom_fingerprint);
    if (rom_fingerprint_.empty()) rom_fingerprint_ = "unknown-rom";

    bins_ = {};
    total_samples_ = 0U;
    min_idle_power_mw_ = -1.0;
    last_obs_ms_ = 0U;
    last_obs_scale_ = -1.0;
    calibrated_ = false;
    confidence_ = 0U;
    knee_scale_ = 0.78;
    power_exponent_ = kDefaultExponent;
    populated_bins_ = 0U;
    persisted_calibrated_ = false;
    persisted_confidence_ = 0U;
    persisted_knee_scale_ = 0.78;
    persisted_exponent_ = kDefaultExponent;
    dirty_ = false;
    significant_dirty_ = false;
    flush_count_ = 0U;

    load_locked();
}

void EdpKneeCalibrator::clear_all() noexcept {
    bins_ = {};
    total_samples_ = 0U;
    min_idle_power_mw_ = -1.0;
    last_obs_ms_ = 0U;
    last_obs_scale_ = -1.0;
    calibrated_ = false;
    confidence_ = 0U;
    knee_scale_ = 0.78;
    power_exponent_ = kDefaultExponent;
    populated_bins_ = 0U;
    dirty_ = false;
    significant_dirty_ = false;
    if (!map_path_.empty()) {
        (void)::unlink(map_path_.c_str());
    }
}

void EdpKneeCalibrator::observe(const EdpObservation& obs) noexcept {
    if (!obs.power_available || obs.charging || obs.screen_off) {
        last_obs_scale_ = -1.0;
        return;
    }
    if (!std::isfinite(obs.power_mw) || obs.power_mw < 150.0 || obs.power_mw > 25000.0) {
        return;
    }
    if (!std::isfinite(obs.effective_scale) || obs.effective_scale < kMinScale || obs.effective_scale > 1.05) {
        return;
    }

    const double s = std::clamp(obs.effective_scale, kMinScale, kMaxScale);
    const double p_mw = obs.power_mw;

    // Reject single-tick PMIC step-transient lag when scale jumps abruptly (> 0.25 within 5s)
    if (last_obs_scale_ > 0.0 && obs.now_ms >= last_obs_ms_ && (obs.now_ms - last_obs_ms_) <= 5000ULL &&
        std::abs(s - last_obs_scale_) > 0.25) {
        last_obs_scale_ = s;
        last_obs_ms_ = obs.now_ms;
        return;
    }
    last_obs_scale_ = s;
    last_obs_ms_ = obs.now_ms;

    // Track low-load static/idle power anchor for safe P_base subtraction.
    // On real ARM64 silicon with deep C-states (e.g. Qualcomm WALT), idle cores can briefly
    // park at hispeed_freq (scale > 0.40) while clock-gated (drawing ~idle Watts). Reject
    // idle C-state samples from mid/high bins so they never invert the power-vs-scale curve!
    const bool util_known = (std::isfinite(obs.cpu_util_pct) && obs.cpu_util_pct >= 0.0) ||
                            (std::isfinite(obs.gpu_busy_pct) && obs.gpu_busy_pct >= 0.0);
    const bool idle_cstate = util_known &&
                             (obs.cpu_util_pct < 15.0) &&
                             (obs.gpu_busy_pct < 15.0);
    if (s <= 0.38 || idle_cstate) {
        if (min_idle_power_mw_ < 0.0) {
            min_idle_power_mw_ = p_mw;
        } else if (p_mw < min_idle_power_mw_) {
            min_idle_power_mw_ = min_idle_power_mw_ * 0.70 + p_mw * 0.30;
        } else {
            min_idle_power_mw_ = min_idle_power_mw_ * 0.98 + p_mw * 0.02;
        }
        if (idle_cstate && s > 0.40) {
            return;
        }
    }

    // Steady-State Settling Gate: skip unsettled transient bursts before fuel-gauge catches up.
    if (obs.steady_state_checked && !obs.steady_state_ok) {
        return;
    }

    // High-scale near-idle-floor anomaly guard: reject brief CPU-only bursts (scale > 0.55)
    // while battery fuel-gauge power is still sitting near the static idle floor (< 1.25x idle).
    if (min_idle_power_mw_ > 0.0 && s > 0.55 && p_mw < min_idle_power_mw_ * 1.25) {
        return;
    }

    const double norm = (s - kMinScale) / (kMaxScale - kMinScale);
    const size_t idx = std::min<size_t>(
        kNumBins - 1U,
        static_cast<size_t>(std::floor(std::clamp(norm, 0.0, 0.999999) * static_cast<double>(kNumBins))));
    auto& bin = bins_[idx];
    if (bin.count == 0U) {
        bin.mean_scale = s;
        bin.mean_power_mw = p_mw;
        bin.ewma_abs_dev_mw = 0.0;
        bin.count = 1U;
    } else {
        // Bounded rolling EWMA (floor 0.025): stays smooth early on yet remains adaptive
        // across multi-hour sessions (thousands of samples) so old outliers never freeze a bin.
        const double alpha = std::max(1.0 / static_cast<double>(bin.count + 1U), kMinBinEwmaAlpha);
        const double dev_alpha = std::clamp(alpha * 1.5, 0.04, 0.20);
        const double dev = std::abs(p_mw - bin.mean_power_mw);
        bin.ewma_abs_dev_mw += dev_alpha * (dev - bin.ewma_abs_dev_mw);
        bin.mean_scale += alpha * (s - bin.mean_scale);
        bin.mean_power_mw += alpha * (p_mw - bin.mean_power_mw);
        if (bin.count < 100000U) ++bin.count;
    }
    if (total_samples_ < 1000000U) ++total_samples_;
    dirty_ = true;

    recompute_locked();
}

void EdpKneeCalibrator::recompute_locked() noexcept {
    // Collect active bins (count >= 4) onto the stack — strictly 0 heap allocations.
    // Requiring >= 4 samples per bin prevents isolated 1-2 tick PMIC/display transients
    // from inverting bin order on real Android devices right after boot.
    size_t active_idx[kNumBins];
    size_t n_active = 0U;
    for (size_t i = 0U; i < kNumBins; ++i) {
        if (bins_[i].count >= 4U && std::isfinite(bins_[i].mean_scale) &&
            std::isfinite(bins_[i].mean_power_mw) && bins_[i].mean_power_mw > 0.0) {
            active_idx[n_active++] = i;
        }
    }
    populated_bins_ = static_cast<uint32_t>(n_active);

    auto fail_closed = [&](uint32_t partial_conf) {
        calibrated_ = false;
        confidence_ = std::min(partial_conf, kMinConfidence - 1U);
        knee_scale_ = 0.78;
        power_exponent_ = kDefaultExponent;
    };

    if (n_active < kMinPopulatedBins || total_samples_ < kMinTotalSamples) {
        const uint32_t partial = static_cast<uint32_t>(
            std::min<double>(45.0, (static_cast<double>(n_active) / 4.0) * 20.0 +
                                   (static_cast<double>(total_samples_) / 24.0) * 25.0));
        fail_closed(partial);
        return;
    }

    const auto& first_bin = bins_[active_idx[0]];
    const auto& last_bin = bins_[active_idx[n_active - 1U]];
    const double s_span = last_bin.mean_scale - first_bin.mean_scale;
    double p_min = first_bin.mean_power_mw;
    double p_max = first_bin.mean_power_mw;
    double noise_sum = 0.0;
    for (size_t k = 0U; k < n_active; ++k) {
        const auto& b = bins_[active_idx[k]];
        p_min = std::min(p_min, b.mean_power_mw);
        p_max = std::max(p_max, b.mean_power_mw);
        noise_sum += b.ewma_abs_dev_mw / std::max(b.mean_power_mw, 300.0);
    }
    const double p_span = p_max - p_min;
    const double mean_rel_noise = noise_sum / static_cast<double>(n_active);

    // Reject flat curves, insufficient excitation span, or wildly noisy telemetry
    if (s_span < kMinScaleSpan || p_span < kMinPowerSpanMw || mean_rel_noise > kMaxMeanRelNoise ||
        (last_bin.mean_power_mw - first_bin.mean_power_mw) < 200.0) {
        fail_closed(20U);
        return;
    }

    // Check monotonic power rise across scale bins (6% tolerance for real fuel-gauge/display jitter;
    // still strictly rejects inverted or erratic non-monotonic curves).
    size_t mono_ok = 0U;
    for (size_t k = 0U; k + 1U < n_active; ++k) {
        const auto& lo = bins_[active_idx[k]];
        const auto& hi = bins_[active_idx[k + 1U]];
        if (hi.mean_power_mw >= lo.mean_power_mw - 0.06 * std::max(lo.mean_power_mw, 500.0)) {
            ++mono_ok;
        }
    }
    const double mono_ratio = static_cast<double>(mono_ok) / static_cast<double>(n_active - 1U);
    if (mono_ratio < kMinMonotonicRatio) {
        fail_closed(15U);
        return;
    }

    // Safe 1D profile-likelihood search over P_base in [0, max_safe_base]:
    // Jointly recovers (P_base, alpha, k) by minimizing linear-space weighted SSE while
    // strictly guaranteeing (P_k - P_base) >= kMinLogDeltaMw > 0 before std::log().
    const double max_safe_base = std::max(0.0, std::min(p_min * 0.90, p_min - 60.0));

    struct FitCandidate {
        bool valid{false};
        double p_base{0.0};
        double alpha{kDefaultExponent};
        double intercept{0.0};
        double r2_log{0.0};
        double sse_mw2{1e30};
    };

    auto eval_candidate = [&](double cand_base) noexcept -> FitCandidate {
        FitCandidate out{};
        out.p_base = cand_base;
        double sum_w = 0.0;
        double sum_wx = 0.0;
        double sum_wy = 0.0;
        double xs[kNumBins];
        double ys[kNumBins];
        double ws[kNumBins];
        for (size_t k = 0U; k < n_active; ++k) {
            const auto& b = bins_[active_idx[k]];
            const double s_clamped = std::clamp(b.mean_scale, kMinScale, kMaxScale);
            const double delta_p = std::max(b.mean_power_mw - cand_base, kMinLogDeltaMw);
            xs[k] = std::log(s_clamped);
            ys[k] = std::log(delta_p);
            ws[k] = std::min<double>(static_cast<double>(b.count), 20.0);
            sum_w += ws[k];
            sum_wx += ws[k] * xs[k];
            sum_wy += ws[k] * ys[k];
        }
        if (sum_w <= 0.0) return out;
        const double mean_x = sum_wx / sum_w;
        const double mean_y = sum_wy / sum_w;
        double sxx = 0.0;
        double sxy = 0.0;
        double syy = 0.0;
        for (size_t k = 0U; k < n_active; ++k) {
            const double dx = xs[k] - mean_x;
            const double dy = ys[k] - mean_y;
            sxx += ws[k] * dx * dx;
            sxy += ws[k] * dx * dy;
            syy += ws[k] * dy * dy;
        }
        if (sxx <= 1e-6 || syy <= 1e-6) return out;
        const double alpha_hat = sxy / sxx;
        const double intercept = mean_y - alpha_hat * mean_x;
        if (!std::isfinite(alpha_hat) || !std::isfinite(intercept) ||
            alpha_hat < 1.05 || alpha_hat > 3.50) {
            return out;
        }
        double sse_log = 0.0;
        double sse_lin = 0.0;
        const double k_mw = std::exp(std::clamp(intercept, 0.0, 11.0));
        for (size_t k = 0U; k < n_active; ++k) {
            const auto& b = bins_[active_idx[k]];
            const double pred_log = intercept + alpha_hat * xs[k];
            const double err_log = ys[k] - pred_log;
            sse_log += ws[k] * err_log * err_log;

            const double s_clamped = std::clamp(b.mean_scale, kMinScale, kMaxScale);
            const double pred_mw = cand_base + k_mw * std::pow(s_clamped, alpha_hat);
            const double err_mw = b.mean_power_mw - pred_mw;
            sse_lin += ws[k] * err_mw * err_mw;
        }
        const double r2 = 1.0 - (sse_log / syy);
        if (!std::isfinite(r2) || !std::isfinite(sse_lin)) return out;
        out.valid = true;
        out.alpha = alpha_hat;
        out.intercept = intercept;
        out.r2_log = r2;
        out.sse_mw2 = sse_lin;
        return out;
    };

    FitCandidate best{};
    static constexpr int kCoarseSteps = 12;
    for (int i = 0; i <= kCoarseSteps; ++i) {
        const double frac = static_cast<double>(i) / static_cast<double>(kCoarseSteps);
        const auto c = eval_candidate(max_safe_base * frac);
        if (c.valid && (!best.valid || c.sse_mw2 < best.sse_mw2)) {
            best = c;
        }
    }
    if (best.valid && max_safe_base > 10.0) {
        const double step = max_safe_base / static_cast<double>(kCoarseSteps);
        const double lo_b = std::max(0.0, best.p_base - step);
        const double hi_b = std::min(max_safe_base, best.p_base + step);
        for (int i = 0; i <= 6; ++i) {
            const double frac = static_cast<double>(i) / 6.0;
            const auto c = eval_candidate(lo_b + (hi_b - lo_b) * frac);
            if (c.valid && c.sse_mw2 < best.sse_mw2) {
                best = c;
            }
        }
    }

    if (!best.valid || best.r2_log < kMinFitR2) {
        fail_closed(25U);
        return;
    }

    const double p_base = best.p_base;
    const double alpha_hat = best.alpha;
    const double intercept = best.intercept;
    const double r2 = best.r2_log;

    const double clamped_alpha = std::clamp(alpha_hat, kMinExponent, kMaxExponent);
    const double k_dyn_mw = std::exp(std::clamp(intercept, 0.0, 11.0));

    // Analytical EDP knee: solve marginal power-to-performance elasticity E(s*) = 1.65
    // where E(s) = (dP/P) / (ds/s) = alpha * (k * s^alpha) / (P_base + k * s^alpha).
    double knee_analytic = kMaxKneeScale;
    if (clamped_alpha > kKneeSlopeThreshold + 0.02 && k_dyn_mw > 50.0 && p_base > 50.0) {
        const double rhs = (kKneeSlopeThreshold * p_base) / (k_dyn_mw * (clamped_alpha - kKneeSlopeThreshold));
        if (rhs > 0.0 && std::isfinite(rhs)) {
            knee_analytic = std::pow(rhs, 1.0 / clamped_alpha);
        }
    }

    // Empirical cross-check across consecutive bins (first segment crossing slope > 1.65)
    double knee_empirical = -1.0;
    for (size_t k = 0U; k + 1U < n_active; ++k) {
        const auto& lo = bins_[active_idx[k]];
        const auto& hi = bins_[active_idx[k + 1U]];
        const double ds = hi.mean_scale - lo.mean_scale;
        if (ds < 0.04) continue;
        const double dp = hi.mean_power_mw - lo.mean_power_mw;
        const double rel_p = dp / std::max(lo.mean_power_mw, 100.0);
        const double rel_s = ds / std::max(lo.mean_scale, kMinScale);
        const double slope = rel_p / rel_s;
        if (slope > kKneeSlopeThreshold) {
            knee_empirical = lo.mean_scale;
            break;
        }
    }

    const double raw_knee = (knee_empirical > 0.0)
        ? (0.65 * knee_analytic + 0.35 * knee_empirical)
        : knee_analytic;
    const double final_knee = std::clamp(raw_knee, kMinKneeScale, kMaxKneeScale);

    // Confidence score combining bin coverage, sample depth, fit R^2, and low noise
    const double bin_factor = std::clamp((static_cast<double>(n_active) - 2.0) / 4.0, 0.0, 1.0);
    const double sample_factor = std::clamp(static_cast<double>(total_samples_) / 48.0, 0.0, 1.0);
    const double r2_factor = std::clamp((r2 - 0.65) / 0.30, 0.0, 1.0);
    const double noise_factor = 1.0 - std::clamp(mean_rel_noise / 0.25, 0.0, 1.0);
    const double fit_factor = r2_factor * noise_factor * mono_ratio;
    const uint32_t conf = static_cast<uint32_t>(std::clamp(
        std::llround(100.0 * (0.35 * bin_factor + 0.30 * sample_factor + 0.35 * fit_factor)),
        0LL, 100LL));

    confidence_ = conf;
    if (conf >= kMinConfidence) {
        calibrated_ = true;
        power_exponent_ = clamped_alpha;
        knee_scale_ = final_knee;
    } else {
        calibrated_ = false;
        power_exponent_ = kDefaultExponent;
        knee_scale_ = 0.78;
    }

    if (calibrated_ != persisted_calibrated_ ||
        std::abs(knee_scale_ - persisted_knee_scale_) >= 0.02 ||
        std::abs(power_exponent_ - persisted_exponent_) >= 0.08 ||
        (confidence_ >= persisted_confidence_ + 10U) ||
        (persisted_confidence_ >= confidence_ + 10U)) {
        significant_dirty_ = true;
    }
}

EdpSnapshot EdpKneeCalibrator::snapshot() const noexcept {
    EdpSnapshot snap{};
    snap.calibrated = calibrated_;
    snap.confidence = confidence_;
    snap.populated_bins = populated_bins_;
    snap.total_samples = total_samples_;
    snap.flush_count = flush_count_;
    if (calibrated_ && confidence_ >= kMinConfidence) {
        const double denom = static_cast<double>(kFullBlendConfidence - kMinConfidence);
        snap.blend = std::clamp(
            static_cast<double>(confidence_ - kMinConfidence) / std::max(denom, 1.0),
            0.0, 1.0);
        snap.knee_scale = std::clamp(knee_scale_, kMinKneeScale, kMaxKneeScale);
        snap.power_exponent = std::clamp(power_exponent_, kMinExponent, kMaxExponent);
        snap.effective_exponent = kDefaultExponent + snap.blend * (snap.power_exponent - kDefaultExponent);
    } else {
        snap.blend = 0.0;
        snap.knee_scale = 0.78;
        snap.power_exponent = kDefaultExponent;
        snap.effective_exponent = kDefaultExponent;
    }
    return snap;
}

bool EdpKneeCalibrator::flush_if_significant() noexcept {
    if (!dirty_ || !significant_dirty_) return false;
    return persist_locked();
}

bool EdpKneeCalibrator::flush_now() noexcept {
    if (!dirty_) return false;
    return persist_locked();
}

void EdpKneeCalibrator::load_locked() noexcept {
    if (map_path_.empty()) return;
    const std::string raw = read_small_raw(map_path_);
    if (raw.empty()) return;

    std::istringstream ss(raw);
    std::string header;
    if (!std::getline(ss, header)) return;
    if (!header.empty() && header.back() == '\r') header.pop_back();

    const std::string expected_header =
        "CFEDP1\t" + kernel_release_ + "\t" + rom_fingerprint_;
    if (header != expected_header) {
        // Kernel or ROM changed via OTA -> auto-reset silicon EDP map to recalibrate cleanly.
        (void)::unlink(map_path_.c_str());
        return;
    }

    std::string line;
    size_t lines_read = 0U;
    while (std::getline(ss, line) && lines_read < 32U) {
        ++lines_read;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::istringstream ls(line);
        std::string tag;
        if (!std::getline(ls, tag, '\t')) continue;

        if (tag == "META") {
            std::string samples_tok, idle_tok;
            if (!std::getline(ls, samples_tok, '\t') || !std::getline(ls, idle_tok, '\t')) continue;
            char* end_u = nullptr;
            const unsigned long s_ul = std::strtoul(samples_tok.c_str(), &end_u, 10);
            char* end_d = nullptr;
            const double idle_d = std::strtod(idle_tok.c_str(), &end_d);
            if (end_u != samples_tok.c_str() && *end_u == '\0') {
                total_samples_ = static_cast<uint32_t>(std::min<unsigned long>(s_ul, 1000000UL));
            }
            if (end_d != idle_tok.c_str() && *end_d == '\0' && std::isfinite(idle_d)) {
                min_idle_power_mw_ = idle_d;
            }
        } else if (tag == "BIN") {
            std::string idx_tok, cnt_tok, scale_tok, pwr_tok, dev_tok;
            if (!std::getline(ls, idx_tok, '\t') ||
                !std::getline(ls, cnt_tok, '\t') ||
                !std::getline(ls, scale_tok, '\t') ||
                !std::getline(ls, pwr_tok, '\t') ||
                !std::getline(ls, dev_tok, '\t')) {
                continue;
            }
            char* e0 = nullptr;
            const unsigned long idx_ul = std::strtoul(idx_tok.c_str(), &e0, 10);
            if (e0 == idx_tok.c_str() || *e0 != '\0' || idx_ul >= kNumBins) continue;

            char* e1 = nullptr;
            const unsigned long cnt_ul = std::strtoul(cnt_tok.c_str(), &e1, 10);
            if (e1 == cnt_tok.c_str() || *e1 != '\0') continue;

            char* e2 = nullptr;
            const double scale_d = std::strtod(scale_tok.c_str(), &e2);
            if (e2 == scale_tok.c_str() || *e2 != '\0' || !std::isfinite(scale_d)) continue;

            char* e3 = nullptr;
            const double pwr_d = std::strtod(pwr_tok.c_str(), &e3);
            if (e3 == pwr_tok.c_str() || *e3 != '\0' || !std::isfinite(pwr_d)) continue;

            char* e4 = nullptr;
            const double dev_d = std::strtod(dev_tok.c_str(), &e4);
            if (e4 == dev_tok.c_str() || *e4 != '\0' || !std::isfinite(dev_d)) continue;

            auto& b = bins_[idx_ul];
            b.count = static_cast<uint32_t>(std::min<unsigned long>(cnt_ul, 100000UL));
            b.mean_scale = std::clamp(scale_d, kMinScale, kMaxScale);
            b.mean_power_mw = std::clamp(pwr_d, 0.0, 25000.0);
            b.ewma_abs_dev_mw = std::clamp(dev_d, 0.0, 10000.0);
        }
    }

    recompute_locked();
    persisted_calibrated_ = calibrated_;
    persisted_confidence_ = confidence_;
    persisted_knee_scale_ = knee_scale_;
    persisted_exponent_ = power_exponent_;
    dirty_ = false;
    significant_dirty_ = false;
}

bool EdpKneeCalibrator::persist_locked() noexcept {
    if (state_dir_.empty() || map_path_.empty()) return false;
    const int dfd = ::open(state_dir_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (dfd < 0) return false;
    struct stat st{};
    if (::fstat(dfd, &st) != 0 || !S_ISDIR(st.st_mode)) {
        (void)::close(dfd);
        return false;
    }

    std::string payload = "CFEDP1\t" + kernel_release_ + "\t" + rom_fingerprint_ + "\n";
    char line_buf[192];
    const int mn = std::snprintf(
        line_buf, sizeof(line_buf), "META\t%u\t%.2f\n",
        static_cast<unsigned>(total_samples_),
        std::isfinite(min_idle_power_mw_) ? min_idle_power_mw_ : -1.0);
    if (mn > 0 && static_cast<size_t>(mn) < sizeof(line_buf)) {
        payload.append(line_buf, static_cast<size_t>(mn));
    }
    for (size_t i = 0U; i < kNumBins; ++i) {
        const auto& b = bins_[i];
        if (b.count == 0U) continue;
        const int n = std::snprintf(
            line_buf, sizeof(line_buf), "BIN\t%zu\t%u\t%.4f\t%.2f\t%.2f\n",
            i, static_cast<unsigned>(b.count),
            std::isfinite(b.mean_scale) ? b.mean_scale : 0.0,
            std::isfinite(b.mean_power_mw) ? b.mean_power_mw : 0.0,
            std::isfinite(b.ewma_abs_dev_mw) ? b.ewma_abs_dev_mw : 0.0);
        if (n > 0 && static_cast<size_t>(n) < sizeof(line_buf)) {
            payload.append(line_buf, static_cast<size_t>(n));
        }
    }

    static constexpr const char* kMapFilename = "silicon_edp.map";
    const std::string tmp_name = std::string(kMapFilename) + ".tmp." + std::to_string(::getpid());
    (void)::unlinkat(dfd, tmp_name.c_str(), 0);
    const int fd = ::openat(dfd, tmp_name.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) {
        (void)::close(dfd);
        return false;
    }
    const bool write_sync_ok = write_all_fd(fd, payload) && sync_fd_if_needed(fd, false);
    const bool close_ok = (::close(fd) == 0);
    if (!write_sync_ok || !close_ok) {
        (void)::unlinkat(dfd, tmp_name.c_str(), 0);
        (void)::close(dfd);
        return false;
    }
    if (::renameat(dfd, tmp_name.c_str(), dfd, kMapFilename) != 0) {
        (void)::unlinkat(dfd, tmp_name.c_str(), 0);
        (void)::close(dfd);
        return false;
    }
    // POSIX crash durability: flush parent directory entry immediately after renameat().
    const bool dir_sync_ok = sync_fd_if_needed(dfd, true);
    (void)::close(dfd);
    if (!dir_sync_ok) {
        return false;
    }

    persisted_calibrated_ = calibrated_;
    persisted_confidence_ = confidence_;
    persisted_knee_scale_ = knee_scale_;
    persisted_exponent_ = power_exponent_;
    dirty_ = false;
    significant_dirty_ = false;
    ++flush_count_;
    return true;
}

} // namespace coreflow::v6
