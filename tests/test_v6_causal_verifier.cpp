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

} // namespace

int main() {
    test_chassis_temp_fuser_zero_dc_bias_and_interpolation();
    test_causal_verifier_learning_bypass_persistence_and_ota_reset();
    test_actuator_rollback_and_thermal_guard_immunity();
    std::cout << "ALL V6 CAUSAL VERIFIER & CHASSIS FUSER TESTS PASSED!\n";
    return 0;
}
