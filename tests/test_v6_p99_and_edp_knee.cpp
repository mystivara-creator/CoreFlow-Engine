/**
 * @file test_v6_p99_and_edp_knee.cpp
 * @brief Phase 3 Unit Tests: Per-Frame p99 Deadline Control (anti-false-boost static/LTPO idle
 *        + fail-closed non-LTPO) & Online Silicon EDP Knee Discovery (CFEDP1 numerical safety,
 *        C0 blend, Zero-Wear POSIX parent-dir fsync persistence, and OTA reset).
 * @license Apache-2.0
 */
#include "coreflow/v5/thread_scheduler.hpp"
#include "coreflow/v6/edp_knee_calibrator.hpp"
#include "coreflow/v6/frame_pacing_sensor.hpp"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace coreflow::v5;
using namespace coreflow::v6;

namespace {

fs::path make_temp_dir(const std::string& tag) {
    const fs::path dir = fs::temp_directory_path() /
                         ("coreflow_p99_edp_" + tag + "_" + std::to_string(::getpid()));
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    assert(!ec);
    return dir;
}

void write_text(const fs::path& p, const std::string& content) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream out(p, std::ios::trunc);
    assert(out.is_open());
    out << content;
}

void test_p99_deadline_estimation_trim_rescue_and_cadence_switch() {
    const fs::path tmp = make_temp_dir("p99_ctrl");
    const fs::path sys = tmp / "sys";
    const fs::path proc = tmp / "proc";
    const fs::path sde_fps = sys / "class/drm/card0/sde-crtc-0/measured_fps";
    write_text(sde_fps, "fps: 120.0 duration: 1000.0 frame_count: 120\n");

    FramePacingSensor sensor;
    sensor.discover(sys.string(), proc.string());
    assert(sensor.verified());

    // 1. Smooth 120 FPS with light render load (0.45) & tiny runqueue delay (0.2ms) -> p99 <= 0.80 -> smooth trim
    uint64_t now_ms = 1000ULL;
    FramePacingSample s{};
    for (int i = 0; i < 6; ++i) {
        now_ms += 1000ULL;
        s = sensor.sample(120U, now_ms, true, nullptr, false, 0.2, 0.45);
    }
    assert(s.available && s.p99_available);
    assert(std::abs(s.frame_budget_ms - (1000.0 / 120.0)) < 0.05);
    assert(s.p99_deadline_util > 0.35 && s.p99_deadline_util <= 0.70);

    ThreadDemandContext base_demand{};
    base_demand.little_capacity = 384;
    base_demand.mid_capacity = 768;
    base_demand.big_capacity = 1024;
    base_demand.refresh_hz = 120;
    base_demand.cpu_psi = 5.0;
    base_demand.gpu_busy_pct = 35.0;
    base_demand.render_pressure = 0.35;
    base_demand.jank_ratio = -1.0;
    base_demand.p99_deadline_util = -1.0; // fail-closed / neutral reference

    const auto t_neutral = ThreadSchedulerActuator::compute_target(
        ContextMode::Interactive, PerformanceProfile::Balanced, 1.0, false, base_demand);

    ThreadDemandContext comfy_demand = base_demand;
    comfy_demand.p99_deadline_util = s.p99_deadline_util;
    const auto t_comfy = ThreadSchedulerActuator::compute_target(
        ContextMode::Interactive, PerformanceProfile::Balanced, 1.0, false, comfy_demand);
    assert(t_comfy.util_min < t_neutral.util_min);

    // 2. Tail spike via runqueue starvation (4.5ms delay even while FPS is still ~119!) -> p99 >= 0.92 -> rescue
    write_text(sde_fps, "fps: 118.5 duration: 1000.0 frame_count: 118\n");
    now_ms += 1000ULL;
    s = sensor.sample(120U, now_ms, true, nullptr, false, 4.5, 0.72);
    assert(s.p99_available && s.p99_deadline_util >= 1.15);

    ThreadDemandContext rescue_demand = base_demand;
    rescue_demand.jank_ratio = 0.0; // average FPS still looks smooth!
    rescue_demand.p99_deadline_util = s.p99_deadline_util;
    const auto t_rescue = ThreadSchedulerActuator::compute_target(
        ContextMode::Interactive, PerformanceProfile::Balanced, 1.0, false, rescue_demand);
    assert(t_rescue.util_min > t_neutral.util_min + 30U);

    // 3. Cadence switch 120 Hz UI -> smooth 60 FPS game resets p99 ring cleanly (no stale 120 Hz pollution)
    write_text(sde_fps, "fps: 60.0 duration: 1000.0 frame_count: 60\n");
    for (int i = 0; i < 4; ++i) {
        now_ms += 1000ULL;
        s = sensor.sample(120U, now_ms, true, nullptr, false, 0.4, 0.50);
    }
    assert(s.p99_available);
    assert(std::abs(s.frame_budget_ms - (1000.0 / 60.0)) < 0.05);
    assert(s.p99_deadline_util < 0.70);

    std::error_code ec;
    fs::remove_all(tmp, ec);
    std::cout << "[PASS] Per-frame p99 deadline estimation, proportional trim/rescue, and cadence shift\n";
}

void test_p99_anti_false_boost_static_ltpo_and_non_ltpo_fail_closed() {
    // Part A: Tier-A / LTPO static screen drop (120 FPS -> 1..8 FPS) MUST NOT trigger false +22% boost!
    {
        const fs::path tmp = make_temp_dir("p99_ltpo_idle");
        const fs::path sys = tmp / "sys";
        const fs::path proc = tmp / "proc";
        const fs::path sde_fps = sys / "class/drm/card0/sde-crtc-0/measured_fps";
        write_text(sde_fps, "fps: 120.0 duration: 1000.0 frame_count: 120\n");

        FramePacingSensor sensor;
        sensor.discover(sys.string(), proc.string());
        uint64_t now_ms = 1000ULL;
        for (int i = 0; i < 5; ++i) {
            now_ms += 1000ULL;
            (void)sensor.sample(120U, now_ms, true, nullptr, false, 0.2, 0.50);
        }

        // 1) Screen becomes static with screen_idle_hint = true and LTPO drops to 10 FPS:
        write_text(sde_fps, "fps: 10.0 duration: 1000.0 frame_count: 10\n");
        now_ms += 1000ULL;
        auto s_idle = sensor.sample(120U, now_ms, true, nullptr, true, 0.1, 0.02);
        assert(!s_idle.p99_available && s_idle.p99_deadline_util < 0.0);

        // 2) Even if active_workload=true & screen_idle_hint=false while LTPO sits at 1.0 FPS (static reading):
        write_text(sde_fps, "fps: 1.0 duration: 1000.0 frame_count: 1\n");
        now_ms += 1000ULL;
        auto s_ltpo1 = sensor.sample(120U, now_ms, true, nullptr, false, 0.1, 0.02);
        assert(!s_ltpo1.p99_available && s_ltpo1.p99_deadline_util < 0.0 && s_ltpo1.jank_ratio < 0.0);

        // 3) Returning to active 120 FPS starts fresh without any 1000ms/100ms stale spike in ring!
        write_text(sde_fps, "fps: 120.0 duration: 1000.0 frame_count: 120\n");
        FramePacingSample s_resume{};
        for (int i = 0; i < 4; ++i) {
            now_ms += 1000ULL;
            s_resume = sensor.sample(120U, now_ms, true, nullptr, false, 0.2, 0.50);
        }
        assert(s_resume.p99_available && s_resume.p99_deadline_util < 0.70);

        std::error_code ec;
        fs::remove_all(tmp, ec);
    }

    // Part B: Fixed-Hz non-LTPO Tier-B panel MUST stay strictly fail-closed on p99 before & after quarantine
    {
        const fs::path tmp = make_temp_dir("p99_non_ltpo");
        const fs::path sys = tmp / "sys";
        const fs::path proc = tmp / "proc";
        const fs::path panel_fps = sys / "class/panel/panel/fps";
        write_text(panel_fps, "120\n");

        FramePacingSensor sensor;
        sensor.discover(sys.string(), proc.string());
        assert(!sensor.verified());

        uint64_t now_ms = 1000ULL;
        // During active probation: strictly fail-closed
        for (int i = 0; i < 4; ++i) {
            now_ms += 1000ULL;
            const auto s = sensor.sample(120U, now_ms, true, nullptr, false, 0.2, 0.50);
            assert(!s.p99_available && s.p99_deadline_util < 0.0 && s.jank_ratio < 0.0);
        }
        // 3 static-idle probes at 120 Hz -> permanently quarantined fail-closed
        for (int i = 0; i < 3; ++i) {
            now_ms += 1000ULL;
            (void)sensor.sample(120U, now_ms, false, nullptr, true, 0.0, 0.0);
        }
        assert(sensor.quarantined_fixed_hz());
        now_ms += 1000ULL;
        const auto s_post = sensor.sample(120U, now_ms, true, nullptr, false, 0.2, 0.50);
        assert(!s_post.p99_available && s_post.p99_deadline_util < 0.0 && s_post.quarantined_fixed_hz);

        std::error_code ec;
        fs::remove_all(tmp, ec);
    }

    std::cout << "[PASS] Anti-false-boost on static screen / LTPO idle and strict fail-closed non-LTPO\n";
}

void feed_silicon_curve(EdpKneeCalibrator& cal,
                        double p_idle_mw,
                        double k_dyn_mw,
                        double alpha,
                        int reps_per_level) {
    static constexpr double kScales[7] = {0.30, 0.42, 0.54, 0.66, 0.78, 0.88, 0.96};
    uint64_t now_ms = 10000ULL;
    for (int r = 0; r < reps_per_level; ++r) {
        for (double s : kScales) {
            now_ms += 6000ULL; // steady-state spacing (> 5s transient gate)
            EdpObservation obs{};
            obs.now_ms = now_ms;
            obs.effective_scale = s;
            obs.power_mw = p_idle_mw + k_dyn_mw * std::pow(s, alpha);
            obs.power_available = true;
            obs.charging = false;
            obs.screen_off = false;
            cal.observe(obs);
        }
    }
}

void test_edp_knee_discovery_across_silicon_archetypes_and_c0_blend() {
    // 1. Steep-voltage silicon (Exynos-like, alpha = 2.40) -> lower EDP knee
    EdpKneeCalibrator steep;
    steep.initialize("", "6.6.30", "EXYNOS");
    feed_silicon_curve(steep, 650.0, 4500.0, 2.40, 8);
    const auto s_steep = steep.snapshot();
    assert(s_steep.calibrated && s_steep.confidence >= 70U && s_steep.blend >= 0.99);
    assert(s_steep.power_exponent >= 2.15 && s_steep.power_exponent <= 2.65);
    assert(s_steep.knee_scale >= 0.55 && s_steep.knee_scale <= 0.75);

    // 2. Flagship silicon (Snapdragon-like, alpha = 2.10) -> mid-upper EDP knee
    EdpKneeCalibrator flagship;
    flagship.initialize("", "6.6.30", "SNAPDRAGON");
    feed_silicon_curve(flagship, 650.0, 4200.0, 2.10, 8);
    const auto s_flag = flagship.snapshot();
    assert(s_flag.calibrated && s_flag.confidence >= 70U);
    assert(s_flag.power_exponent >= 1.90 && s_flag.power_exponent <= 2.35);
    assert(s_flag.knee_scale >= 0.65 && s_flag.knee_scale <= 0.85);
    assert(s_flag.knee_scale > s_steep.knee_scale);

    // 3. Flat low-voltage silicon (Budget 4GB, alpha = 1.55) -> high knee clamped <= 0.88
    EdpKneeCalibrator budget;
    budget.initialize("", "5.4.210", "BUDGET4G");
    feed_silicon_curve(budget, 550.0, 2200.0, 1.55, 8);
    const auto s_bud = budget.snapshot();
    assert(s_bud.calibrated && s_bud.confidence >= 70U);
    assert(s_bud.power_exponent >= 1.35 && s_bud.power_exponent <= 1.75);
    assert(s_bud.knee_scale >= 0.84 && s_bud.knee_scale <= 0.88);

    // 4. Charging & screen-off samples NEVER pollute an uncalibrated calibrator
    EdpKneeCalibrator chg_only;
    chg_only.initialize("", "6.6.30", "CHG");
    uint64_t now_ms = 10000ULL;
    for (int i = 0; i < 40; ++i) {
        now_ms += 6000ULL;
        EdpObservation obs{};
        obs.now_ms = now_ms;
        obs.effective_scale = 0.30 + (i % 7) * 0.11;
        obs.power_mw = 6000.0;
        obs.power_available = true;
        obs.charging = (i % 2 == 0);
        obs.screen_off = (i % 2 == 1);
        chg_only.observe(obs);
    }
    assert(!chg_only.snapshot().calibrated && chg_only.snapshot().total_samples == 0U);

    std::cout << "[PASS] Online silicon EDP knee discovery across steep/flagship/budget silicon & C0 blend\n";
}

void test_edp_numerical_safety_and_fail_closed_degenerate_curves() {
    static constexpr double kScales[7] = {0.30, 0.42, 0.54, 0.66, 0.78, 0.88, 0.96};

    // 1. Flat constant power curve (P = 2000 mW everywhere) -> MUST fail closed (calibrated = false)
    {
        EdpKneeCalibrator flat;
        flat.initialize("", "6.6.30", "FLAT");
        uint64_t now_ms = 10000ULL;
        for (int r = 0; r < 6; ++r) {
            for (double s : kScales) {
                now_ms += 6000ULL;
                flat.observe({now_ms, s, 2000.0, true, false, false});
            }
        }
        const auto snap = flat.snapshot();
        assert(!snap.calibrated && snap.confidence < 50U);
        assert(std::isfinite(snap.effective_exponent) && std::abs(snap.effective_exponent - 1.8) < 1e-9);
    }

    // 2. Inverted / non-monotonic curve (power drops as scale increases) -> MUST fail closed
    {
        EdpKneeCalibrator inv;
        inv.initialize("", "6.6.30", "INVERTED");
        uint64_t now_ms = 10000ULL;
        for (int r = 0; r < 6; ++r) {
            for (double s : kScales) {
                now_ms += 6000ULL;
                inv.observe({now_ms, s, 4500.0 - s * 2800.0, true, false, false});
            }
        }
        const auto snap = inv.snapshot();
        assert(!snap.calibrated && snap.confidence < 50U);
    }

    // 3. Wildly noisy curve (±65% alternating swings in every bin) -> MUST fail closed
    {
        EdpKneeCalibrator noisy;
        noisy.initialize("", "6.6.30", "NOISY");
        uint64_t now_ms = 10000ULL;
        for (int r = 0; r < 10; ++r) {
            for (double s : kScales) {
                now_ms += 6000ULL;
                const double base_p = 650.0 + 3500.0 * std::pow(s, 2.0);
                const double p = (r % 2 == 0) ? (base_p * 1.65) : (base_p * 0.35);
                noisy.observe({now_ms, s, std::max(200.0, p), true, false, false});
            }
        }
        const auto snap = noisy.snapshot();
        assert(!snap.calibrated && snap.confidence < 50U);
    }

    // 4. Garbage / NaN / Inf / tiny-power-near-P_base inputs -> zero NaN/Inf, strictly bounded
    {
        EdpKneeCalibrator edge;
        edge.initialize("", "6.6.30", "EDGE");
        edge.observe({1000ULL, NAN, 2000.0, true, false, false});
        edge.observe({7000ULL, 0.5, INFINITY, true, false, false});
        edge.observe({13000ULL, -1.0, -500.0, true, false, false});
        edge.observe({19000ULL, 0.30, 160.0, true, false, false});
        edge.observe({25000ULL, 0.30, 165.0, true, false, false});
        const auto snap = edge.snapshot();
        assert(!snap.calibrated);
        assert(std::isfinite(snap.knee_scale) && std::isfinite(snap.power_exponent));
    }

    std::cout << "[PASS] EdpKneeCalibrator numerical safety & fail-closed on flat/inverted/noisy/garbage curves\n";
}

void test_edp_zero_wear_cfedp1_persistence_and_ota_reset() {
    const fs::path tmp = make_temp_dir("cfedp1_persist");
    const fs::path state = tmp / "state";
    fs::create_directories(state);

    EdpKneeCalibrator cal;
    cal.initialize(state.string(), "6.6.30-android15", "BUILD_A");
    feed_silicon_curve(cal, 650.0, 4400.0, 2.25, 8);
    const auto orig = cal.snapshot();
    assert(orig.calibrated && orig.confidence >= 70U);

    // First ScreenOff flush writes CFEDP1 atomically with parent-dir fsync
    assert(cal.flush_if_significant());
    assert(cal.flush_count() == 1U);
    assert(fs::exists(state / "silicon_edp.map"));
    // Second call with no new drift -> Zero-Wear skip!
    assert(!cal.flush_if_significant());
    assert(cal.flush_count() == 1U);

    // Reload on SAME kernel & ROM -> restores calibrated knee & exponent immediately
    EdpKneeCalibrator reloaded;
    reloaded.initialize(state.string(), "6.6.30-android15", "BUILD_A");
    const auto rest = reloaded.snapshot();
    assert(rest.calibrated);
    assert(rest.confidence == orig.confidence);
    assert(std::abs(rest.knee_scale - orig.knee_scale) < 0.005);
    assert(std::abs(rest.power_exponent - orig.power_exponent) < 0.005);

    // Reload after OTA ROM update -> auto-resets & unlinks stale CFEDP1 map!
    EdpKneeCalibrator ota;
    ota.initialize(state.string(), "6.6.30-android15", "BUILD_B_OTA");
    assert(!ota.snapshot().calibrated);
    assert(ota.snapshot().total_samples == 0U);
    assert(!fs::exists(state / "silicon_edp.map"));

    std::error_code ec;
    fs::remove_all(tmp, ec);
    std::cout << "[PASS] CFEDP1 Zero-Wear atomic persistence (parent-dir fsync) and OTA reset\n";
}

} // namespace

int main() {
    test_p99_deadline_estimation_trim_rescue_and_cadence_switch();
    test_p99_anti_false_boost_static_ltpo_and_non_ltpo_fail_closed();
    test_edp_knee_discovery_across_silicon_archetypes_and_c0_blend();
    test_edp_numerical_safety_and_fail_closed_degenerate_curves();
    test_edp_zero_wear_cfedp1_persistence_and_ota_reset();
    std::cout << "ALL V6 PHASE 3 P99 DEADLINE & SILICON EDP KNEE TESTS PASSED!\n";
    return 0;
}
