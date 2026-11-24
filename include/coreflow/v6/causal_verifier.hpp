/**
 * @file causal_verifier.hpp
 * @brief CoreFlow v6.1 Online Causal Verification Engine (`device_effectiveness.map`, CFEM1).
 *
 * Measures actual physical causal effect of CoreFlow's 6 actuator domains via
 * natural-experiment matched pre/post windows (zero artificial stutter injection,
 * zero heap allocations in the 1 Hz hot path). If an actuator domain produces
 * negligible effect (score <= 0.02) across >= 5 independent matched trials,
 * CoreFlow transitions it to BypassedIneffective, rolls back its sysfs nodes via
 * BaselineStore, and skips future futile writes — while keeping THERMAL_GUARD
 * and THERMAL_EMERGENCY safety paths unconditionally immune.
 *
 * Zero-Wear persistence: 100% RAM-first while screen is ON; atomic CFEM1 write
 * only on ScreenOff (when state changed or score drifted significantly) and on
 * clean daemon shutdown; automatically resets on kernel/ROM OTA updates.
 *
 * @license Apache-2.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace coreflow::v6 {

enum class CausalDomain : uint8_t {
    ThermalUclampCap = 0,
    ThermalGpuCap = 1,
    CpuGovTuning = 2,
    SemanticCpuGpuBoost = 3,
    MemoryBusFloor = 4,
    StorageMemoryTuning = 5,
    Count = 6,
};

enum class CausalState : uint8_t {
    Probing = 0,
    VerifiedEffective = 1,
    BypassedIneffective = 2,
};

struct CausalDomainSnapshot {
    CausalDomain domain{CausalDomain::ThermalUclampCap};
    CausalState state{CausalState::Probing};
    uint32_t samples{0U};
    double score{0.0};
    uint32_t confidence{0U};
};

struct CausalObservation {
    uint64_t now_ms{0U};
    double dt_sec{1.0};
    bool screen_off{false};
    bool boot_settle{false};
    bool thermal_guard{false};
    bool inspection_mode{false};

    // Actuator domain engagement states for the current tick
    bool uclamp_cap_active{false};
    uint32_t uclamp_cap_pct{0U};       // 0 = unspecified; 1..100 = active top-app uclamp.max cap %
    bool gpu_cap_active{false};
    bool cpu_gov_active{false};
    bool semantic_boost_active{false};
    bool memory_bus_active{false};
    bool storage_memory_active{false};

    // Physical telemetry signals
    double cpu_util_pct{0.0};
    double gpu_busy_pct{0.0};
    bool power_available{false};
    bool charging{false};
    double power_mw{0.0};
    double thermal_velocity_cpm{0.0};
    double mean_cluster_khz{0.0};
    double jank_ratio{-1.0};           // < 0 when unavailable/unverified
    double starvation_pressure{-1.0};  // < 0 when schedstat unavailable
    double cpu_psi{-1.0};              // < 0 when PSI unavailable
    double io_psi{-1.0};               // < 0 when PSI unavailable
    double mem_psi{-1.0};              // < 0 when PSI unavailable
    double refault_file_per_sec{0.0};
    double direct_reclaim_per_sec{0.0};
};

struct CausalObserveResult {
    uint32_t newly_bypassed_mask{0U};
    uint32_t newly_verified_mask{0U};
    uint32_t completed_trials_mask{0U};
};

class CausalVerifier {
public:
    static constexpr size_t kDomainCount = static_cast<size_t>(CausalDomain::Count);
    static constexpr uint32_t kMinSamplesForVerify = 3U;
    static constexpr uint32_t kMinSamplesForBypass = 5U;
    static constexpr double kVerifyScoreThreshold = 0.08;
    static constexpr double kBypassScoreThreshold = 0.02;
    static constexpr double kSignificantDriftDelta = 0.10;
    static constexpr uint32_t kMinPreSamples = 3U;
    static constexpr double kSettleDurationSec = 2.0;
    static constexpr double kPostWindowTargetSec = 10.0;
    static constexpr uint32_t kMinPostSamplesShort = 4U;
    static constexpr double kMaxMatchedLoadDeltaPct = 24.0;
    static constexpr double kMaxGapResetSec = 15.0;
    static constexpr double kMinUclampTrialPowerMw = 2400.0;
    static constexpr double kMinUclampTrialCpuUtilPct = 50.0;
    static constexpr uint32_t kMaxUclampTrialCapPct = 85U;

    CausalVerifier() noexcept = default;

    // Loads persisted CFEM1 records if kernel_release + rom_fingerprint match;
    // automatically unlinks and resets all domains on kernel/ROM update.
    void initialize(const std::string& state_dir,
                    const std::string& kernel_release,
                    const std::string& rom_fingerprint) noexcept;

    // Zero-heap 1 Hz natural-experiment matched-window observer.
    [[nodiscard]] CausalObserveResult observe(const CausalObservation& obs) noexcept;

    // Returns true if the domain has been proven ineffective on this device/kernel.
    // Safety invariant: ALWAYS returns false when thermal_guard is true.
    [[nodiscard]] bool is_bypassed(CausalDomain domain,
                                   bool thermal_guard = false) const noexcept;

    [[nodiscard]] CausalDomainSnapshot domain_snapshot(CausalDomain domain) const noexcept;
    [[nodiscard]] std::array<CausalDomainSnapshot, kDomainCount> snapshot_all() const noexcept;

    [[nodiscard]] uint32_t effective_count() const noexcept;
    [[nodiscard]] uint32_t bypassed_count() const noexcept;
    [[nodiscard]] uint32_t probing_count() const noexcept;
    [[nodiscard]] uint64_t flush_count() const noexcept { return flush_count_; }

    // Zero-Wear persistence:
    //  - flush_if_significant(): called ONLY on ScreenOff when a state transition
    //    or significant score drift (>= 0.10) occurred since last flush.
    //  - flush_if_dirty(): called ONLY on clean daemon shutdown.
    bool flush_if_significant() noexcept;
    bool flush_if_dirty() noexcept;

    // Clears in-memory state and removes persisted CFEM1 file (for tests/reset).
    void clear_all() noexcept;

    [[nodiscard]] static const char* domain_key(CausalDomain domain) noexcept;
    [[nodiscard]] static const char* state_name(CausalState state) noexcept;
    [[nodiscard]] static bool parse_domain_key(std::string_view key, CausalDomain& out) noexcept;
    [[nodiscard]] static bool parse_state_name(std::string_view name, CausalState& out) noexcept;

private:
    struct MetricsSample {
        double cpu_util_pct{0.0};
        double gpu_busy_pct{0.0};
        bool power_ok{false};
        double power_mw{0.0};
        double thermal_vel_cpm{0.0};
        double mean_khz{0.0};
        bool jank_ok{false};
        double jank_ratio{0.0};
        bool starv_ok{false};
        double starvation{0.0};
        bool cpu_psi_ok{false};
        double cpu_psi{0.0};
        bool io_psi_ok{false};
        double io_psi{0.0};
        bool mem_psi_ok{false};
        double mem_psi{0.0};
        double refault_ps{0.0};
        double reclaim_ps{0.0};
        uint32_t uclamp_cap_pct{0U};
    };

    struct DomainTracker {
        CausalState state{CausalState::Probing};
        uint32_t samples{0U};
        double score{0.0};
        uint32_t confidence{0U};

        // Last flushed snapshot for Zero-Wear drift gating
        CausalState persisted_state{CausalState::Probing};
        double persisted_score{0.0};

        // Natural-experiment pre-window rolling baseline (zero heap)
        uint32_t pre_samples{0U};
        MetricsSample pre_ewma{};

        // Active trial post-window accumulators
        bool in_trial{false};
        bool latched_until_inactive{false};
        MetricsSample trial_pre{};
        double settle_elapsed_sec{0.0};
        double post_elapsed_sec{0.0};
        uint32_t post_samples{0U};
        double sum_cpu{0.0};
        double sum_gpu{0.0};
        uint32_t power_samples{0U};
        double sum_power{0.0};
        double sum_vel{0.0};
        uint32_t khz_samples{0U};
        double sum_khz{0.0};
        uint32_t jank_samples{0U};
        double sum_jank{0.0};
        uint32_t starv_samples{0U};
        double sum_starv{0.0};
        uint32_t cpu_psi_samples{0U};
        double sum_cpu_psi{0.0};
        uint32_t io_psi_samples{0U};
        double sum_io_psi{0.0};
        uint32_t mem_psi_samples{0U};
        double sum_mem_psi{0.0};
        double sum_refault{0.0};
        double sum_reclaim{0.0};
        uint32_t min_uclamp_cap_pct{0U};
    };

    static void update_pre_window(DomainTracker& tr, const CausalObservation& obs) noexcept;
    static void begin_trial(DomainTracker& tr) noexcept;
    static void accumulate_post(DomainTracker& tr, const CausalObservation& obs) noexcept;
    static bool evaluate_domain_effect(CausalDomain domain,
                                       const MetricsSample& pre,
                                       const MetricsSample& post,
                                       double& out_effect) noexcept;
    void finalize_trial(CausalDomain domain,
                        DomainTracker& tr,
                        CausalObserveResult& result) noexcept;
    void load_locked() noexcept;
    bool persist_locked() noexcept;

    std::string state_dir_;
    std::string map_path_;
    std::string kernel_release_;
    std::string rom_fingerprint_;
    std::array<DomainTracker, kDomainCount> trackers_{};
    bool dirty_{false};
    bool significant_dirty_{false};
    uint64_t flush_count_{0U};
};

} // namespace coreflow::v6
