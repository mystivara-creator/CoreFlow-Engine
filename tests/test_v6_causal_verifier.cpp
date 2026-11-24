/**
 * @file test_v6_causal_verifier.cpp
 * @brief Unit tests for CoreFlow v6.1 ChassisTempFuser (1°C quantization fix)
 *        and Online CausalVerifier (`device_effectiveness.map`, CFEM1).
 * @license Apache-2.0
 */
#include "coreflow/baseline_store.hpp"
#include "coreflow/v5/storage_memory_actuator.hpp"
#include "coreflow/v5/thermal_envelope_actuator.hpp"
#include "coreflow/v6/causal_verifier.hpp"
#include "coreflow/v6/chassis_temp_fuser.hpp"
#include "coreflow/v6/oem_collaboration_map.hpp"
#include "coreflow/v6/semantic_knob_engine.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace coreflow;
using namespace coreflow::v5;
using namespace coreflow::v6;

namespace {

void write_text(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream ofs(p, std::ios::trunc);
    assert(ofs.is_open());
    ofs << content;
    ofs.close();
}

std::string read_trimmed(const fs::path& p) {
    std::ifstream ifs(p);
    std::string line;
    std::getline(ifs, line);
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
    return line;
}

void test_chassis_temp_fuser_zero_dc_bias_and_interpolation() {
    ChassisTempFuser fuser;

    // Warm up with 1°C quantized battery (38.0°C) + +4.0°C warmer millidegree xo-therm (42.0°C)
    double fused = 0.0;
    for (int t = 0; t < 12; ++t) {
        fused = fuser.update(38.0, -1.0, 42.0 + t * 0.002, 1.0);
    }
    assert(fuser.battery_coarse_1c());
    assert(fuser.fused_active());
    assert(std::abs(fused - 38.0) < 0.05);

    // Ramp xo-therm smoothly by +0.8°C over 40s while battery stays stuck at 38.0°C
    double prev = fused;
    int rises = 0;
    for (int t = 0; t < 40; ++t) {
        const double xo = 42.024 + t * 0.020;
        fused = fuser.update(38.0, -1.0, xo, 1.0);
        if (fused > prev + 0.001) ++rises;
        assert(fused >= 38.0 - 0.61 && fused <= 38.0 + 0.61);
        prev = fused;
    }
    assert(rises >= 25);
    assert(fused > 38.15 && fused <= 38.60);

    // Hold steady at battery=41.0°C and xo-therm=45.0°C (+4.0°C offset) -> exact zero DC bias!
    for (int t = 0; t < 400; ++t) {
        fused = fuser.update(41.0, -1.0, 45.001, 1.0);
    }
    assert(fuser.fused_active());
    assert(std::abs(fused - 41.0) < 0.02);

    // Fine 0.05°C battery sensor must bypass fusion and return raw battery temperature verbatim
    ChassisTempFuser fine_fuser;
    for (int t = 0; t < 20; ++t) {
        const double b = 36.15 + t * 0.05;
        const double out = fine_fuser.update(b, -1.0, b + 3.005, 1.0);
        assert(std::abs(out - b) < 1e-6);
    }
    assert(!fine_fuser.battery_coarse_1c());
    assert(!fine_fuser.fused_active());

    // Pure 1°C battery sensor without skin/board fallback returns raw battery safely
    ChassisTempFuser pure_1c_fuser;
    for (int t = 0; t < 15; ++t) {
        const double out = pure_1c_fuser.update(40.0, -1.0, -1.0, 1.0);
        assert(std::abs(out - 40.0) < 1e-6);
    }
    assert(pure_1c_fuser.battery_coarse_1c());
    assert(!pure_1c_fuser.fused_active());

    std::cout << "[PASS] ChassisTempFuser zero-DC-bias, 1C interpolation, and fallback\n";
}

void run_matched_trial(CausalVerifier& verifier,
                       uint64_t& now_ms,
                       CausalObservation pre_obs,
                       CausalObservation post_obs,
                       CausalObserveResult& last_res) {
    // 4s pre-window
    for (int i = 0; i < 4; ++i) {
        now_ms += 1000ULL;
        pre_obs.now_ms = now_ms;
        pre_obs.dt_sec = 1.0;
        last_res = verifier.observe(pre_obs);
    }
    // 2s settle + 10s post-window = 13s active
    for (int i = 0; i < 13; ++i) {
        now_ms += 1000ULL;
        post_obs.now_ms = now_ms;
        post_obs.dt_sec = 1.0;
        const auto r = verifier.observe(post_obs);
        if (r.completed_trials_mask != 0U || r.newly_bypassed_mask != 0U || r.newly_verified_mask != 0U) {
            last_res = r;
        }
    }
    // 2s release before next trial
    for (int i = 0; i < 2; ++i) {
        now_ms += 1000ULL;
        pre_obs.now_ms = now_ms;
        pre_obs.dt_sec = 1.0;
        (void)verifier.observe(pre_obs);
    }
}

void test_causal_verifier_learning_bypass_persistence_and_ota_reset() {
    const fs::path tmp = fs::temp_directory_path() / ("cf_test_causal_" + std::to_string(::getpid()));
    std::error_code ec;
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp);

    CausalVerifier verifier;
    verifier.initialize(tmp.string(), "6.6.30-gki", "HYPEROS2");
    assert(verifier.probing_count() == CausalVerifier::kDomainCount);
    assert(verifier.effective_count() == 0U);
    assert(verifier.bypassed_count() == 0U);

    uint64_t now_ms = 10000ULL;
    CausalObserveResult res{};

    // 1. Ceiling-effect protection: 0% jank & 0.02 starvation must NOT count as ineffective trials
    {
        CausalObservation pre{};
        pre.cpu_util_pct = 60.0;
        pre.gpu_busy_pct = 60.0;
        pre.jank_ratio = 0.0;
        pre.starvation_pressure = 0.02;
        pre.cpu_psi = 1.0;
        pre.semantic_boost_active = false;
        CausalObservation post = pre;
        post.semantic_boost_active = true;
        for (int k = 0; k < 6; ++k) {
            run_matched_trial(verifier, now_ms, pre, post, res);
        }
        const auto snap = verifier.domain_snapshot(CausalDomain::SemanticCpuGpuBoost);
        assert(snap.samples == 0U);
        assert(snap.state == CausalState::Probing);
    }

    // 2. Unmatched-load protection: Idle (10%) -> Gaming (75%) must NOT pollute trials
    {
        CausalObservation pre{};
        pre.cpu_util_pct = 10.0;
        pre.gpu_busy_pct = 5.0;
        pre.jank_ratio = 0.08;
        pre.starvation_pressure = 0.35;
        pre.semantic_boost_active = false;
        CausalObservation post = pre;
        post.cpu_util_pct = 75.0;
        post.gpu_busy_pct = 75.0;
        post.semantic_boost_active = true;
        for (int k = 0; k < 6; ++k) {
            run_matched_trial(verifier, now_ms, pre, post, res);
        }
        assert(verifier.domain_snapshot(CausalDomain::SemanticCpuGpuBoost).samples == 0U);
    }

    // 3. Genuine effective ThermalUclampCap: 3 trials reducing power 3800mW -> 3100mW
    {
        CausalObservation pre{};
        pre.cpu_util_pct = 68.0;
        pre.gpu_busy_pct = 72.0;
        pre.power_available = true;
        pre.charging = false;
        pre.power_mw = 3800.0;
        pre.thermal_velocity_cpm = 0.7;
        pre.uclamp_cap_active = false;

        CausalObservation post = pre;
        post.power_mw = 3100.0;
        post.thermal_velocity_cpm = 0.1;
        post.uclamp_cap_active = true;

        for (int k = 0; k < 3; ++k) {
            run_matched_trial(verifier, now_ms, pre, post, res);
        }
        const auto snap = verifier.domain_snapshot(CausalDomain::ThermalUclampCap);
        assert(snap.samples == 3U);
        assert(snap.score >= 0.15);
        assert(snap.state == CausalState::VerifiedEffective);
        assert(!verifier.is_bypassed(CausalDomain::ThermalUclampCap, false));
    }

    // 4. Stubbed no-op SemanticCpuGpuBoost: 5 matched trials with real headroom & 0% lift -> BypassedIneffective
    {
        CausalObservation pre{};
        pre.cpu_util_pct = 65.0;
        pre.gpu_busy_pct = 68.0;
        pre.power_available = true;
        pre.charging = false;
        pre.power_mw = 3200.0;
        pre.jank_ratio = 0.08;
        pre.starvation_pressure = 0.35;
        pre.cpu_psi = 12.0;
        pre.semantic_boost_active = false;

        CausalObservation post = pre;
        post.semantic_boost_active = true; // Stubbed: metrics stay identical!

        bool got_bypass_event = false;
        for (int k = 0; k < 5; ++k) {
            res = {};
            run_matched_trial(verifier, now_ms, pre, post, res);
            if (k < 4) {
                assert(!verifier.is_bypassed(CausalDomain::SemanticCpuGpuBoost, false));
            } else {
                got_bypass_event =
                    (res.newly_bypassed_mask & (1U << static_cast<uint32_t>(CausalDomain::SemanticCpuGpuBoost))) != 0U;
            }
        }
        assert(got_bypass_event);
        const auto snap = verifier.domain_snapshot(CausalDomain::SemanticCpuGpuBoost);
        assert(snap.samples == 5U);
        assert(snap.score <= 0.02);
        assert(snap.state == CausalState::BypassedIneffective);
        assert(verifier.is_bypassed(CausalDomain::SemanticCpuGpuBoost, false));
        // Absolute safety invariant: THERMAL_GUARD is ALWAYS immune!
        assert(!verifier.is_bypassed(CausalDomain::SemanticCpuGpuBoost, true));
    }

    // 5. Zero-Wear check: 0 disk writes while screen ON!
    const fs::path map_file = tmp / "device_effectiveness.map";
    assert(!fs::exists(map_file));
    assert(verifier.flush_count() == 0U);

    // ScreenOff significant flush writes CFEM1 atomically
    assert(verifier.flush_if_significant());
    assert(fs::exists(map_file));
    assert(verifier.flush_count() == 1U);
    // Second call with no new drift does NOT rewrite flash
    assert(!verifier.flush_if_significant());
    assert(verifier.flush_count() == 1U);

    // 6. Reload with identical kernel+ROM preserves effective & bypassed decisions
    {
        CausalVerifier reloaded;
        reloaded.initialize(tmp.string(), "6.6.30-gki", "HYPEROS2");
        assert(reloaded.domain_snapshot(CausalDomain::ThermalUclampCap).state == CausalState::VerifiedEffective);
        assert(reloaded.domain_snapshot(CausalDomain::ThermalUclampCap).samples == 3U);
        assert(reloaded.domain_snapshot(CausalDomain::SemanticCpuGpuBoost).state == CausalState::BypassedIneffective);
        assert(reloaded.is_bypassed(CausalDomain::SemanticCpuGpuBoost, false));
        assert(!reloaded.is_bypassed(CausalDomain::SemanticCpuGpuBoost, true));
    }

    // 7. Kernel/ROM OTA update auto-resets CFEM1 cleanly
    {
        CausalVerifier ota_reloaded;
        ota_reloaded.initialize(tmp.string(), "6.6.45-gki", "HYPEROS2");
        assert(ota_reloaded.probing_count() == CausalVerifier::kDomainCount);
        assert(!ota_reloaded.is_bypassed(CausalDomain::SemanticCpuGpuBoost, false));
        assert(!fs::exists(map_file));
    }

    fs::remove_all(tmp, ec);
    std::cout << "[PASS] CausalVerifier natural trials, bypass, Zero-Wear CFEM1, and OTA reset\n";
}

void test_actuator_rollback_and_thermal_guard_immunity() {
    const fs::path tmp = fs::temp_directory_path() / ("cf_test_causal_act_" + std::to_string(::getpid()));
    std::error_code ec;
    fs::remove_all(tmp, ec);
    const fs::path sys_root = tmp / "sys";
    const fs::path proc_root = tmp / "proc";
    const fs::path state_dir = tmp / "state";
    fs::create_directories(state_dir);

    const fs::path gpu_dir = sys_root / "class/kgsl/kgsl-3d0";
    const fs::path adrenoboost = gpu_dir / "adrenoboost";
    const fs::path top_uclamp = sys_root / "fs/cgroup/top-app/cpu.uclamp.max";
    write_text(adrenoboost, "0\n");
    write_text(top_uclamp, "max\n");

    assert(::setenv("COREFLOW_TEST_STATE_DIR", state_dir.c_str(), 1) == 0);
    auto& baseline = BaselineStore::instance();
    baseline.reinitialize();
    assert(baseline.recover_pending());
    OemCollaborationMap oem_map;
    oem_map.initialize(state_dir.string(), "6.6.30", "ROM1");
    CausalVerifier verifier;
    verifier.initialize(state_dir.string(), "6.6.30", "ROM1");

    HardwareTopology topo{};
    topo.refresh_hz = 120;
    topo.gpu.available = true;
    topo.gpu.is_kgsl = true;
    topo.gpu.device_path = gpu_dir.string();
    topo.cgroups.top_app_uclamp_max_path = top_uclamp.string();
    topo.cgroups.uclamp_uses_percent = true;

    SemanticKnobEngine semantic(baseline, oem_map, nullptr, &verifier);
    semantic.discover(topo, sys_root.string(), proc_root.string());
    assert(semantic.stats().gpu_knobs >= 1U);

    // Probing phase: adrenoboost activates during Gaming
    semantic.apply(ContextMode::Gaming, PerformanceProfile::Balanced, 1.0, false, true, 0.50, 1.0, 10000ULL);
    assert(read_trimmed(adrenoboost) == "2");
    assert(semantic.stats().active_cpu_gpu_boost == 1U);

    // Prove SemanticCpuGpuBoost AND ThermalUclampCap ineffective via 5 natural no-op trials
    uint64_t now_ms = 20000ULL;
    CausalObserveResult res{};
    {
        CausalObservation pre{};
        pre.cpu_util_pct = 70.0;
        pre.gpu_busy_pct = 75.0;
        pre.power_available = true;
        pre.charging = false;
        pre.power_mw = 3500.0;
        pre.jank_ratio = 0.08;
        pre.starvation_pressure = 0.35;
        pre.cpu_psi = 12.0;

        CausalObservation post = pre;
        post.semantic_boost_active = true;
        post.uclamp_cap_active = true;

        for (int k = 0; k < 5; ++k) {
            run_matched_trial(verifier, now_ms, pre, post, res);
        }
    }
    assert(verifier.is_bypassed(CausalDomain::SemanticCpuGpuBoost, false));
    assert(verifier.is_bypassed(CausalDomain::ThermalUclampCap, false));

    // Next SemanticKnobEngine::apply immediately rolls back adrenoboost to OEM "0"
    semantic.apply(ContextMode::Gaming, PerformanceProfile::Balanced, 1.0, false, true, 0.50, 1.0, 200000ULL);
    assert(read_trimmed(adrenoboost) == "0");
    assert(semantic.stats().active_cpu_gpu_boost == 0U);

    // ThermalEnvelopeActuator: normal warm pacing (thermal_guard=false) respects bypass
    ThermalEnvelopeActuator envelope(baseline, &oem_map, &verifier);
    EnvelopeTelemetry tel{};
    tel.gpu_busy_pct = 70.0;
    envelope.apply(topo, ContextMode::Gaming, PerformanceProfile::Balanced, 0.80, false, true, tel, 201000ULL);
    assert(read_trimmed(top_uclamp) == "max");
    assert(envelope.stats().eas_top_app_cap_pct == 100U);

    // Safety invariant: under thermal_guard=true, ThermalUclampCap STILL engages immediately!
    envelope.apply(topo, ContextMode::Gaming, PerformanceProfile::Balanced, 0.55, true, true, tel, 202000ULL);
    assert(read_trimmed(top_uclamp) != "max");
    assert(envelope.stats().eas_top_app_cap_pct < 100U);

    (void)baseline.rollback_all();
    assert(read_trimmed(top_uclamp) == "max");
    fs::remove_all(tmp, ec);
    std::cout << "[PASS] Actuator bypass rollback and THERMAL_GUARD immunity\n";
}

void test_chassis_temp_fuser_extreme_edge_cases() {
    // 1. Flatlined battery sensor: stuck at 38.0 C for >= 180s while xo-therm ramps +5 C
    //    must fail closed (fused_active == false, returns raw 38.0 C) and recover once battery moves.
    {
        ChassisTempFuser fuser;
        double out = 0.0;
        for (int t = 0; t < 200; ++t) {
            const double xo = 41.0 + t * 0.025;
            out = fuser.update(38.0, -1.0, xo, 1.0);
        }
        assert(fuser.battery_coarse_1c());
        assert(fuser.battery_flatlined());
        assert(!fuser.fused_active());
        assert(std::abs(out - 38.0) < 1e-6);

        // Sensor resumes moving (38 -> 39 C) -> clears flatline and re-enables smooth fusion
        out = fuser.update(39.0, -1.0, 46.025, 1.0);
        assert(!fuser.battery_flatlined());
        assert(fuser.fused_active());
        assert(std::abs(out - 39.0) <= 0.61);
    }

    // 2. 1 C integer boundary jitter: battery toggles 39.0 <-> 40.0 every second while xo-therm is steady.
    //    Fuser must suppress the 1.0 C chatter to a smooth narrow band around 39.5 C.
    {
        ChassisTempFuser fuser;
        for (int t = 0; t < 60; ++t) {
            (void)fuser.update((t % 2 == 0) ? 39.0 : 40.0, -1.0, 42.305 + (t % 3) * 0.002, 1.0);
        }
        double min_v = 100.0, max_v = 0.0;
        for (int t = 0; t < 40; ++t) {
            const double b = (t % 2 == 0) ? 39.0 : 40.0;
            const double out = fuser.update(b, -1.0, 42.305 + (t % 3) * 0.002, 1.0);
            assert(fuser.fused_active());
            min_v = std::min(min_v, out);
            max_v = std::max(max_v, out);
        }
        assert((max_v - min_v) < 0.25);
        assert(min_v >= 39.2 && max_v <= 39.8);
    }

    // 3. Bidirectional precision transitions: coarse 1 C -> fine 0.1 C -> coarse 1 C
    {
        ChassisTempFuser fuser;
        for (int t = 0; t < 12; ++t) {
            (void)fuser.update(38.0, -1.0, 41.205 + t * 0.005, 1.0);
        }
        assert(fuser.battery_coarse_1c());
        assert(fuser.fused_active());

        // Switch to fine 0.1 C battery readings -> fusion immediately yields to raw fine battery
        for (int t = 0; t < 6; ++t) {
            const double b = 38.1 + t * 0.1;
            const double out = fuser.update(b, -1.0, 41.305 + t * 0.05, 1.0);
            if (t >= 2) {
                assert(!fuser.battery_coarse_1c());
                assert(!fuser.fused_active());
                assert(std::abs(out - b) < 1e-6);
            }
        }

        // Transition back to coarse 1 C integer steps -> recovers coarse detection & fusion
        for (int t = 0; t < 60; ++t) {
            const double b = (t < 20) ? 39.0 : ((t < 40) ? 40.0 : 41.0);
            (void)fuser.update(b, -1.0, b + 3.005 + (t % 5) * 0.003, 1.0);
        }
        assert(fuser.battery_coarse_1c());
        assert(fuser.fused_active());
    }

    // 4. Extreme spikes & invalid inputs (NaN, Inf, out-of-range, +30 C single-tick aux glitch, bad dt)
    {
        ChassisTempFuser fuser;
        for (int t = 0; t < 15; ++t) {
            (void)fuser.update(39.0, -1.0, 42.005 + t * 0.002, 1.0);
        }
        assert(fuser.fused_active());
        const double spiked = fuser.update(39.0, -1.0, 72.005, 1.0);
        assert(std::isfinite(spiked));
        assert(spiked >= 39.0 - 0.61 && spiked <= 39.0 + 0.61);

        // Invalid battery / dt never produce NaN/Inf
        const double nan_out = fuser.update(std::nan(""), -1.0, 42.5, 1.0);
        assert(nan_out == -1.0 && !fuser.fused_active());
        const double high_out = fuser.update(120.0, 40.25, 42.5, 1.0);
        assert(std::abs(high_out - 40.25) < 1e-6 && !fuser.fused_active());
        const double gap_out = fuser.update(39.0, -1.0, 42.505, 120.0);
        assert(std::isfinite(gap_out) && std::abs(gap_out - 39.0) <= 0.61);
    }

    std::cout << "[PASS] ChassisTempFuser flatline, 1C jitter, coarse<->fine transition, and extreme spikes\n";
}

void test_all_six_causal_domains_positive_negative_and_tradeoffs() {
    uint64_t now_ms = 10000ULL;
    CausalObserveResult res{};

    // A. ThermalGpuCap: positive (power drops under GPU load) & negative (no-op stub)
    {
        CausalVerifier pos_v, neg_v;
        pos_v.initialize("", "6.6.30", "R1");
        neg_v.initialize("", "6.6.30", "R1");

        CausalObservation pre{};
        pre.cpu_util_pct = 55.0;
        pre.gpu_busy_pct = 75.0;
        pre.power_available = true;
        pre.charging = false;
        pre.power_mw = 4200.0;
        pre.thermal_velocity_cpm = 0.8;

        CausalObservation post_eff = pre;
        post_eff.gpu_cap_active = true;
        post_eff.power_mw = 3450.0;
        post_eff.thermal_velocity_cpm = 0.15;

        CausalObservation post_noop = pre;
        post_noop.gpu_cap_active = true;

        for (int k = 0; k < 3; ++k) run_matched_trial(pos_v, now_ms, pre, post_eff, res);
        const auto ps = pos_v.domain_snapshot(CausalDomain::ThermalGpuCap);
        assert(ps.state == CausalState::VerifiedEffective && ps.score >= 0.15);

        for (int k = 0; k < 5; ++k) run_matched_trial(neg_v, now_ms, pre, post_noop, res);
        const auto ns = neg_v.domain_snapshot(CausalDomain::ThermalGpuCap);
        assert(ns.state == CausalState::BypassedIneffective && ns.score <= 0.02);
        assert(neg_v.is_bypassed(CausalDomain::ThermalGpuCap, false));
        assert(!neg_v.is_bypassed(CausalDomain::ThermalGpuCap, true));
    }

    // B. CpuGovTuning:
    //    1) Positive UX benefit (starvation/jank drops even with mild +5% power cost) -> VerifiedEffective
    //    2) Positive efficiency benefit (matched load power drops without UX degradation) -> VerifiedEffective
    //    3) Negative actuator-motion-only + power-burn trade-off: cluster_cur_khz jumps +83% and
    //       power burns +20% with ZERO latency/jank/starvation gain -> score <= 0, BypassedIneffective!
    //    4) Negative smooth-load power burn (no UX headroom, burns +10% power) -> BypassedIneffective!
    {
        CausalVerifier ux_v, eff_v, khz_only_burn_v, smooth_burn_v;
        ux_v.initialize("", "6.6.30", "R1");
        eff_v.initialize("", "6.6.30", "R1");
        khz_only_burn_v.initialize("", "6.6.30", "R1");
        smooth_burn_v.initialize("", "6.6.30", "R1");

        // B1: UX gain outweighs modest +5% power cost
        {
            CausalObservation pre{};
            pre.cpu_util_pct = 62.0;
            pre.power_available = true;
            pre.power_mw = 2800.0;
            pre.mean_cluster_khz = 1200000.0;
            pre.starvation_pressure = 0.42;
            pre.jank_ratio = 0.10;
            pre.cpu_psi = 14.0;

            CausalObservation post = pre;
            post.cpu_gov_active = true;
            post.mean_cluster_khz = 1800000.0;
            post.power_mw = 2940.0; // +5% power
            post.starvation_pressure = 0.14;
            post.jank_ratio = 0.02;
            post.cpu_psi = 5.0;

            for (int k = 0; k < 3; ++k) run_matched_trial(ux_v, now_ms, pre, post, res);
            const auto s = ux_v.domain_snapshot(CausalDomain::CpuGovTuning);
            assert(s.state == CausalState::VerifiedEffective && s.score >= 0.15);
        }

        // B2: Power efficiency gain without UX regression
        {
            CausalObservation pre{};
            pre.cpu_util_pct = 50.0;
            pre.power_available = true;
            pre.power_mw = 3200.0;
            pre.mean_cluster_khz = 1900000.0;
            pre.starvation_pressure = 0.03;
            pre.jank_ratio = 0.01;

            CausalObservation post = pre;
            post.cpu_gov_active = true;
            post.mean_cluster_khz = 1450000.0;
            post.power_mw = 2650.0; // -17% power, UX intact

            for (int k = 0; k < 3; ++k) run_matched_trial(eff_v, now_ms, pre, post, res);
            const auto s = eff_v.domain_snapshot(CausalDomain::CpuGovTuning);
            assert(s.state == CausalState::VerifiedEffective && s.score >= 0.12);
        }

        // B3: Actuator motion only (mean_cluster_khz jumps +83%!) + +20% power burn, ZERO UX improvement
        {
            CausalObservation pre{};
            pre.cpu_util_pct = 62.0;
            pre.power_available = true;
            pre.power_mw = 2800.0;
            pre.mean_cluster_khz = 1200000.0;
            pre.starvation_pressure = 0.35;
            pre.jank_ratio = 0.08;
            pre.cpu_psi = 12.0;

            CausalObservation post = pre;
            post.cpu_gov_active = true;
            post.mean_cluster_khz = 2200000.0; // Actuator moved sharply!
            post.power_mw = 3360.0;            // +20% power burn!
            // starvation / jank / cpu_psi unchanged -> ZERO real system benefit!

            for (int k = 0; k < 5; ++k) run_matched_trial(khz_only_burn_v, now_ms, pre, post, res);
            const auto s = khz_only_burn_v.domain_snapshot(CausalDomain::CpuGovTuning);
            assert(s.score < 0.0);
            assert(s.state == CausalState::BypassedIneffective);
        }

        // B4: Smooth workload (no UX headroom) where governor tuning still burns +10% power
        {
            CausalObservation pre{};
            pre.cpu_util_pct = 48.0;
            pre.power_available = true;
            pre.power_mw = 2400.0;
            pre.mean_cluster_khz = 1300000.0;
            pre.starvation_pressure = 0.01;
            pre.jank_ratio = 0.0;

            CausalObservation post = pre;
            post.cpu_gov_active = true;
            post.mean_cluster_khz = 2100000.0;
            post.power_mw = 2640.0; // +10% wasted power

            for (int k = 0; k < 5; ++k) run_matched_trial(smooth_burn_v, now_ms, pre, post, res);
            const auto s = smooth_burn_v.domain_snapshot(CausalDomain::CpuGovTuning);
            assert(s.score < 0.0);
            assert(s.state == CausalState::BypassedIneffective);
        }
    }

    // C. SemanticCpuGpuBoost: positive UX rescue & negative wasteful power-burn trade-off
    {
        CausalVerifier pos_v, burn_v;
        pos_v.initialize("", "6.6.30", "R1");
        burn_v.initialize("", "6.6.30", "R1");

        CausalObservation pre{};
        pre.cpu_util_pct = 68.0;
        pre.gpu_busy_pct = 74.0;
        pre.power_available = true;
        pre.power_mw = 3200.0;
        pre.jank_ratio = 0.10;
        pre.starvation_pressure = 0.38;
        pre.cpu_psi = 13.0;

        CausalObservation post_good = pre;
        post_good.semantic_boost_active = true;
        post_good.power_mw = 3360.0; // +5% power cost justified by real UX rescue
        post_good.jank_ratio = 0.02;
        post_good.starvation_pressure = 0.12;
        post_good.cpu_psi = 4.0;

        CausalObservation post_burn = pre;
        post_burn.semantic_boost_active = true;
        post_burn.power_mw = 3800.0; // +18.75% power burn with zero UX gain

        for (int k = 0; k < 3; ++k) run_matched_trial(pos_v, now_ms, pre, post_good, res);
        assert(pos_v.domain_snapshot(CausalDomain::SemanticCpuGpuBoost).state == CausalState::VerifiedEffective);

        for (int k = 0; k < 5; ++k) run_matched_trial(burn_v, now_ms, pre, post_burn, res);
        const auto bs = burn_v.domain_snapshot(CausalDomain::SemanticCpuGpuBoost);
        assert(bs.score < 0.0 && bs.state == CausalState::BypassedIneffective);
    }

    // D. MemoryBusFloor: positive (GPU/IO relief) & negative (no-op + power burn trade-off)
    {
        CausalVerifier pos_v, neg_v;
        pos_v.initialize("", "6.6.30", "R1");
        neg_v.initialize("", "6.6.30", "R1");

        CausalObservation pre{};
        pre.cpu_util_pct = 55.0;
        pre.gpu_busy_pct = 78.0;
        pre.power_available = true;
        pre.power_mw = 3400.0;
        pre.io_psi = 14.0;
        pre.jank_ratio = 0.09;

        CausalObservation post_good = pre;
        post_good.memory_bus_active = true;
        post_good.power_mw = 3520.0;
        post_good.io_psi = 5.0;
        post_good.jank_ratio = 0.02;

        CausalObservation post_burn = pre;
        post_burn.memory_bus_active = true;
        post_burn.power_mw = 3900.0; // +14.7% power, zero relief

        for (int k = 0; k < 3; ++k) run_matched_trial(pos_v, now_ms, pre, post_good, res);
        assert(pos_v.domain_snapshot(CausalDomain::MemoryBusFloor).state == CausalState::VerifiedEffective);

        for (int k = 0; k < 5; ++k) run_matched_trial(neg_v, now_ms, pre, post_burn, res);
        const auto ns = neg_v.domain_snapshot(CausalDomain::MemoryBusFloor);
        assert(ns.score < 0.0 && ns.state == CausalState::BypassedIneffective);
    }

    // E. StorageMemoryTuning: positive (refault/reclaim drop) & negative (no-op)
    {
        CausalVerifier pos_v, neg_v;
        pos_v.initialize("", "6.6.30", "R1");
        neg_v.initialize("", "6.6.30", "R1");

        CausalObservation pre{};
        pre.cpu_util_pct = 45.0;
        pre.refault_file_per_sec = 900.0;
        pre.direct_reclaim_per_sec = 45.0;
        pre.io_psi = 12.0;
        pre.mem_psi = 10.0;

        CausalObservation post_good = pre;
        post_good.storage_memory_active = true;
        post_good.refault_file_per_sec = 320.0;
        post_good.direct_reclaim_per_sec = 10.0;
        post_good.io_psi = 4.0;
        post_good.mem_psi = 3.0;

        CausalObservation post_noop = pre;
        post_noop.storage_memory_active = true;

        for (int k = 0; k < 3; ++k) run_matched_trial(pos_v, now_ms, pre, post_good, res);
        assert(pos_v.domain_snapshot(CausalDomain::StorageMemoryTuning).state == CausalState::VerifiedEffective);

        for (int k = 0; k < 5; ++k) run_matched_trial(neg_v, now_ms, pre, post_noop, res);
        const auto ns = neg_v.domain_snapshot(CausalDomain::StorageMemoryTuning);
        assert(ns.score <= 0.02 && ns.state == CausalState::BypassedIneffective);
        assert(neg_v.is_bypassed(CausalDomain::StorageMemoryTuning, false));
        assert(!neg_v.is_bypassed(CausalDomain::StorageMemoryTuning, true));
    }

    std::cout << "[PASS] All 6 causal domains positive, negative, and power-vs-UX trade-off verification\n";
}

void test_all_domain_actuator_rollbacks_and_emergency_immunity() {
    const fs::path tmp = fs::temp_directory_path() / ("cf_test_causal_all6_" + std::to_string(::getpid()));
    std::error_code ec;
    fs::remove_all(tmp, ec);
    const fs::path sys_root = tmp / "sys";
    const fs::path proc_root = tmp / "proc";
    const fs::path state_dir = tmp / "state";
    fs::create_directories(state_dir);

    // Prepare mock sysfs/procfs nodes for ThermalGpuCap, CpuGovTuning, MemoryBusFloor, StorageMemoryTuning
    const fs::path gov_dir = sys_root / "devices/system/cpu/cpufreq/policy7/schedutil";
    const fs::path hispeed_freq = gov_dir / "hispeed_freq";
    const fs::path hispeed_load = gov_dir / "hispeed_load";
    const fs::path up_rate = gov_dir / "up_rate_limit_us";
    const fs::path down_rate = gov_dir / "down_rate_limit_us";
    write_text(hispeed_freq, "1700000\n");
    write_text(hispeed_load, "90\n");
    write_text(up_rate, "1000\n");
    write_text(down_rate, "10000\n");

    const fs::path gpu_dir = sys_root / "class/kgsl/kgsl-3d0";
    const fs::path gpu_max_lvl = gpu_dir / "max_pwrlevel";
    const fs::path gpu_min_lvl = gpu_dir / "min_pwrlevel";
    const fs::path gpu_idle = gpu_dir / "idle_timer";
    write_text(gpu_max_lvl, "0\n");
    write_text(gpu_min_lvl, "7\n");
    write_text(gpu_idle, "80\n");

    const fs::path bus_dir = sys_root / "class/devfreq/soc-ddr";
    const fs::path bus_min = bus_dir / "min_freq";
    const fs::path bus_max = bus_dir / "max_freq";
    write_text(bus_min, "547000\n");
    write_text(bus_max, "3196000\n");

    const fs::path ra_node = sys_root / "block/sda/queue/read_ahead_kb";
    const fs::path swap_node = proc_root / "sys/vm/swappiness";
    const fs::path ttl_node = sys_root / "kernel/mm/lru_gen/min_ttl_ms";
    write_text(ra_node, "128\n");
    write_text(swap_node, "60\n");
    write_text(ttl_node, "600\n");

    assert(::setenv("COREFLOW_TEST_STATE_DIR", state_dir.c_str(), 1) == 0);
    auto& baseline = BaselineStore::instance();
    baseline.reinitialize();
    assert(baseline.recover_pending());

    OemCollaborationMap oem_map;
    oem_map.initialize(state_dir.string(), "6.6.30", "ROM_ALL6");
    CausalVerifier verifier;
    verifier.initialize(state_dir.string(), "6.6.30", "ROM_ALL6");

    HardwareTopology topo{};
    topo.refresh_hz = 120;
    topo.total_ram_bytes = 8ULL * 1024ULL * 1024ULL * 1024ULL;
    topo.zram_active = true;
    topo.zram_bytes = 4ULL * 1024ULL * 1024ULL * 1024ULL;
    topo.big_capacity = 1024U;
    topo.little_capacity = 384U;

    CpuClusterInfo big{};
    big.role = ClusterRole::Big;
    big.capacity = 1024U;
    big.governor = "schedutil";
    big.opps_khz = {700000, 1200000, 1700000, 2200000, 2700000, 3200000};
    big.hispeed_freq_path = hispeed_freq.string();
    big.hispeed_load_path = hispeed_load.string();
    big.up_rate_limit_path = up_rate.string();
    big.down_rate_limit_path = down_rate.string();
    topo.cpu_clusters.push_back(big);

    topo.gpu.available = true;
    topo.gpu.is_kgsl = true;
    topo.gpu.kgsl_num_pwrlevels = 8U;
    topo.gpu.device_path = gpu_dir.string();
    topo.gpu.kgsl_max_pwrlevel_path = gpu_max_lvl.string();
    topo.gpu.kgsl_min_pwrlevel_path = gpu_min_lvl.string();
    topo.gpu.idle_timer_path = gpu_idle.string();

    MemoryBusNodeInfo bus{};
    bus.name = "soc-ddr";
    bus.min_freq_path = bus_min.string();
    bus.opps_hz = {547000, 680000, 1017000, 1555000, 2092000, 2736000, 3196000};
    topo.memory_bus_nodes.push_back(bus);

    StorageQueueInfo sq{};
    sq.type = StorageType::UFS;
    sq.read_ahead_kb_path = ra_node.string();
    sq.baseline_read_ahead_kb = 128U;
    sq.max_sectors_kb = 1024U;
    sq.nr_requests = 64U;
    topo.storage_queues.push_back(sq);

    topo.vm.swappiness_path = swap_node.string();
    topo.vm.mglru_oem_enabled = true;
    topo.vm.mglru_min_ttl_path = ttl_node.string();

    ThermalEnvelopeActuator envelope(baseline, &oem_map, &verifier);
    SemanticKnobEngine semantic(baseline, oem_map, nullptr, &verifier);
    semantic.discover(topo, sys_root.string(), proc_root.string());
    StorageMemoryActuator storage(baseline, proc_root.string(), &oem_map, &verifier);

    // 1. Probing phase: all 4 domains actively mutate their managed sysfs/procfs nodes
    EnvelopeTelemetry etel{};
    etel.gpu_busy_pct = 75.0;
    etel.cpu_psi = 10.0;
    envelope.apply(topo, ContextMode::Gaming, PerformanceProfile::Balanced, 0.72, false, true, etel, 10000ULL);
    assert(envelope.stats().cpu_gov_active);
    assert(envelope.stats().gpu_cap_active);
    assert(read_trimmed(gpu_max_lvl) != "0");
    assert(read_trimmed(hispeed_load) != "90");

    SymbioticHints hints{};
    hints.memory_bus_scale = 1.25;
    semantic.apply(ContextMode::Gaming, PerformanceProfile::Balanced, 1.0, false, true, 0.50, 1.0, 11000ULL, false, hints);
    assert(semantic.stats().active_memory_bus == 1U);
    assert(read_trimmed(bus_min) != "547000");

    ContextDecision launch_ctx{};
    launch_ctx.mode = ContextMode::LaunchBurst;
    StorageMemoryTelemetry stel{};
    stel.io_psi = 12.0;
    stel.mem_psi = 4.0;
    stel.refault_file_per_sec = 400.0;
    stel.memory.valid = true;
    stel.memory.total_bytes = topo.total_ram_bytes;
    stel.memory.available_bytes = topo.total_ram_bytes / 2U;
    storage.apply(topo, launch_ctx, PerformanceProfile::Balanced, true, true, 12000ULL, stel);
    assert(storage.stats().vm_or_storage_active);
    assert(read_trimmed(ra_node) != "128");
    assert(read_trimmed(ttl_node) != "600");

    // 2. Drive all 4 domains to BypassedIneffective via 5 natural no-op / wasteful trials
    uint64_t now_ms = 20000ULL;
    CausalObserveResult res{};
    {
        CausalObservation pre{};
        pre.cpu_util_pct = 65.0;
        pre.gpu_busy_pct = 75.0;
        pre.power_available = true;
        pre.power_mw = 3600.0;
        pre.mean_cluster_khz = 1200000.0;
        pre.jank_ratio = 0.08;
        pre.starvation_pressure = 0.35;
        pre.cpu_psi = 12.0;
        pre.io_psi = 12.0;
        pre.mem_psi = 10.0;
        pre.refault_file_per_sec = 800.0;
        pre.direct_reclaim_per_sec = 40.0;

        CausalObservation post = pre;
        post.gpu_cap_active = true;
        post.cpu_gov_active = true;
        post.memory_bus_active = true;
        post.storage_memory_active = true;
        post.mean_cluster_khz = 2200000.0; // clock moved with 0 benefit

        for (int k = 0; k < 5; ++k) run_matched_trial(verifier, now_ms, pre, post, res);
    }
    assert(verifier.is_bypassed(CausalDomain::ThermalGpuCap, false));
    assert(verifier.is_bypassed(CausalDomain::CpuGovTuning, false));
    assert(verifier.is_bypassed(CausalDomain::MemoryBusFloor, false));
    assert(verifier.is_bypassed(CausalDomain::StorageMemoryTuning, false));

    // 3. Re-apply under normal conditions: EVERY bypassed domain rolls back cleanly to OEM factory values!
    envelope.apply(topo, ContextMode::Gaming, PerformanceProfile::Balanced, 0.72, false, true, etel, 200000ULL);
    assert(!envelope.stats().cpu_gov_active);
    assert(!envelope.stats().gpu_cap_active);
    assert(read_trimmed(hispeed_freq) == "1700000");
    assert(read_trimmed(hispeed_load) == "90");
    assert(read_trimmed(up_rate) == "1000");
    assert(read_trimmed(down_rate) == "10000");
    assert(read_trimmed(gpu_max_lvl) == "0");
    assert(read_trimmed(gpu_min_lvl) == "7");
    assert(read_trimmed(gpu_idle) == "80");

    semantic.apply(ContextMode::Gaming, PerformanceProfile::Balanced, 1.0, false, true, 0.50, 1.0, 201000ULL, false, hints);
    assert(semantic.stats().active_memory_bus == 0U);
    assert(read_trimmed(bus_min) == "547000");

    storage.apply(topo, launch_ctx, PerformanceProfile::Balanced, true, true, 202000ULL, stel);
    assert(!storage.stats().vm_or_storage_active);
    assert(read_trimmed(ra_node) == "128");
    assert(read_trimmed(swap_node) == "60");
    assert(read_trimmed(ttl_node) == "600");

    // 4. Safety immunity:
    //    - Under thermal_guard=true, ThermalGpuCap & CpuGovTuning STILL engage immediately!
    //    - Under direct_reclaim_storm=true, StorageMemoryTuning STILL overrides mglru_min_ttl_ms to "0"!
    envelope.apply(topo, ContextMode::Gaming, PerformanceProfile::Balanced, 0.55, true, true, etel, 203000ULL);
    assert(envelope.stats().cpu_gov_active);
    assert(envelope.stats().gpu_cap_active);
    assert(read_trimmed(gpu_max_lvl) != "0");

    StorageMemoryTelemetry storm_tel = stel;
    storm_tel.direct_reclaim_storm = true;
    storm_tel.direct_reclaim_per_sec = 180.0;
    storm_tel.mem_psi = 22.0;
    storm_tel.thrashing_pressure = 0.85;
    storage.apply(topo, launch_ctx, PerformanceProfile::Balanced, true, true, 204000ULL, storm_tel);
    assert(read_trimmed(ttl_node) == "0");

    // 5. Persist CFEM1 with parent-dir fsync and verify durability on disk
    assert(verifier.flush_if_significant());
    assert(fs::exists(state_dir / "device_effectiveness.map"));

    (void)baseline.rollback_all();
    fs::remove_all(tmp, ec);
    std::cout << "[PASS] All 4 remaining domain rollbacks and THERMAL_GUARD/reclaim-storm immunity\n";
}

void test_thermal_uclamp_cap_light_load_and_shallow_cap_precondition() {
    CausalVerifier verifier;
    verifier.initialize("", "6.6.118-android15-8", "PARROTPRO_HYPEROS3");
    uint64_t now_ms = 10000ULL;
    CausalObserveResult res{};

    // 1. Light daily usage (Instagram/Chrome: ~1679 mW, CPU 28%, GPU 8%, uclamp_cap_pct = 80%)
    //    Display backlight dominates fuel-gauge; 6 repeated trials with slight noise MUST NOT
    //    record samples or falsely bypass ThermalUclampCap!
    {
        CausalObservation pre{};
        pre.cpu_util_pct = 28.0;
        pre.gpu_busy_pct = 8.0;
        pre.power_available = true;
        pre.charging = false;
        pre.power_mw = 1679.0;
        pre.uclamp_cap_active = false;
        pre.uclamp_cap_pct = 100U;

        CausalObservation post = pre;
        post.power_mw = 1699.0; // -1.2% noise buried in 120Hz display draw
        post.uclamp_cap_active = true;
        post.uclamp_cap_pct = 80U;

        for (int k = 0; k < 6; ++k) {
            run_matched_trial(verifier, now_ms, pre, post, res);
        }
        const auto snap = verifier.domain_snapshot(CausalDomain::ThermalUclampCap);
        assert(snap.samples == 0U);
        assert(snap.state == CausalState::Probing);
        assert(!verifier.is_bypassed(CausalDomain::ThermalUclampCap, false));
    }

    // 2. Heavy load (3400 mW, CPU 68%) with SHALLOW cap (uclamp_cap_pct = 90% > 85%)
    //    MUST ALSO be rejected so tiny caps never cause false bypass!
    {
        CausalObservation pre{};
        pre.cpu_util_pct = 68.0;
        pre.gpu_busy_pct = 60.0;
        pre.power_available = true;
        pre.charging = false;
        pre.power_mw = 3400.0;
        pre.uclamp_cap_active = false;
        pre.uclamp_cap_pct = 100U;

        CausalObservation post = pre;
        post.power_mw = 3390.0;
        post.uclamp_cap_active = true;
        post.uclamp_cap_pct = 90U; // > 85% shallow cap

        for (int k = 0; k < 6; ++k) {
            run_matched_trial(verifier, now_ms, pre, post, res);
        }
        const auto snap = verifier.domain_snapshot(CausalDomain::ThermalUclampCap);
        assert(snap.samples == 0U);
        assert(snap.state == CausalState::Probing);
    }

    // 3. Real heavy load (>= 2400 mW) AND meaningful cap (<= 85%) -> evaluates cleanly & proves effective!
    {
        CausalObservation pre{};
        pre.cpu_util_pct = 64.0;
        pre.gpu_busy_pct = 70.0;
        pre.power_available = true;
        pre.charging = false;
        pre.power_mw = 3600.0;
        pre.thermal_velocity_cpm = 0.6;
        pre.uclamp_cap_active = false;
        pre.uclamp_cap_pct = 100U;

        CausalObservation post = pre;
        post.power_mw = 2950.0;
        post.thermal_velocity_cpm = 0.05;
        post.uclamp_cap_active = true;
        post.uclamp_cap_pct = 78U;

        for (int k = 0; k < 3; ++k) {
            run_matched_trial(verifier, now_ms, pre, post, res);
        }
        const auto snap = verifier.domain_snapshot(CausalDomain::ThermalUclampCap);
        assert(snap.samples == 3U);
        assert(snap.state == CausalState::VerifiedEffective);
    }

    // 4. High-CPU compute load (CPU >= 50% even at 2100 mW) with <= 85% cap on a genuinely no-op
    //    kernel STILL bypasses cleanly after 5 trials.
    {
        CausalVerifier noop_v;
        noop_v.initialize("", "6.6.118-android15-8", "NOOP_UCLAMP");
        uint64_t t_ms = 10000ULL;
        CausalObserveResult r{};

        CausalObservation pre{};
        pre.cpu_util_pct = 58.0;
        pre.gpu_busy_pct = 15.0;
        pre.power_available = true;
        pre.charging = false;
        pre.power_mw = 2100.0;
        pre.uclamp_cap_active = false;
        pre.uclamp_cap_pct = 100U;

        CausalObservation post = pre;
        post.power_mw = 2105.0;
        post.uclamp_cap_active = true;
        post.uclamp_cap_pct = 75U;

        for (int k = 0; k < 5; ++k) {
            run_matched_trial(noop_v, t_ms, pre, post, r);
        }
        assert(noop_v.is_bypassed(CausalDomain::ThermalUclampCap, false));
    }

    std::cout << "[PASS] ThermalUclampCap rejects light daily loads (~1.6W) & shallow caps (>85%), verifies/bypasses real loads\n";
}

} // namespace

int main() {
    test_chassis_temp_fuser_zero_dc_bias_and_interpolation();
    test_chassis_temp_fuser_extreme_edge_cases();
    test_causal_verifier_learning_bypass_persistence_and_ota_reset();
    test_all_six_causal_domains_positive_negative_and_tradeoffs();
    test_actuator_rollback_and_thermal_guard_immunity();
    test_all_domain_actuator_rollbacks_and_emergency_immunity();
    test_thermal_uclamp_cap_light_load_and_shallow_cap_precondition();
    std::cout << "ALL V6 CAUSAL VERIFIER & CHASSIS FUSER TESTS PASSED!\n";
    return 0;
}
