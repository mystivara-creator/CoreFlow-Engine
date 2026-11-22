use crate::bridge::ffi::{SymbioticInput, SymbioticPlan};

/// Orthogonal Symbiotic Orchestrator regime encoding shared across C++ <-> Rust FFI:
/// 0 = Balanced
/// 1 = GpuBound (shift power & bus supply toward GPU, ease excess CPU floor)
/// 2 = CpuDrawBound (boost worker parallelism & draw-call headroom so GPU does not stall)
/// 3 = MemoryIoStarved (elevate asset/bus supply so CPU/GPU pipelines stay fed)
/// 4 = SingleHotspotSpread (relieve saturated Prime/Big cluster by engaging companion clusters)
pub const REGIME_BALANCED: u8 = 0;
pub const REGIME_GPU_BOUND: u8 = 1;
pub const REGIME_CPU_DRAW_BOUND: u8 = 2;
pub const REGIME_MEMORY_IO_STARVED: u8 = 3;
pub const REGIME_SINGLE_HOTSPOT_SPREAD: u8 = 4;

/// Active coupled-domain bitmask flags exported via `SymbioticPlan::coupling_mask`:
pub const COUPLING_COMPUTE_MEMORY_BUS: u8 = 1 << 0;
pub const COUPLING_RENDERING_SCHEDULER: u8 = 1 << 1;
pub const COUPLING_HEADROOM_THERMAL: u8 = 1 << 2;

fn sanitize_unit(v: f64) -> f64 {
    if v.is_finite() {
        v.clamp(0.0, 1.0)
    } else {
        0.0
    }
}

/// Orthogonal Symbiotic Orchestrator:
/// Uses Little's Law queue balance (L = lambda * W) across CPU, GPU, and Memory/I/O
/// plus cross-cluster saturation gradient detection and coupled multi-domain physics
/// (Compute <-> Memory Bus, Rendering <-> Scheduler, Headroom <-> Thermal Envelope).
pub fn compute_symbiotic_plan(input: SymbioticInput) -> SymbioticPlan {
    let cap_scale = if input.capacity_scale.is_finite() {
        input.capacity_scale.clamp(0.25, 1.0)
    } else {
        1.0
    };

    // Under thermal guard or heavy PID throttling, keep all symbiotic overrides neutral.
    if input.thermal_guard || cap_scale < 0.78 {
        return SymbioticPlan {
            regime: REGIME_BALANCED,
            cluster_imbalance: 0.0,
            cpu_uclamp_scale: 1.0,
            helper_uclamp_ratio: 0.65,
            boost_gpu_memory_supply: false,
            io_supply_boost: false,
            encourage_cluster_spread: false,
            memory_bus_scale: 1.0,
            scheduler_responsiveness_scale: 1.0,
            thermal_stepdown_lead: 0.0,
            coupling_mask: 0,
        };
    }

    let little_r = sanitize_unit(input.little_freq_ratio);
    let mid_r = sanitize_unit(input.mid_freq_ratio);
    let big_r = sanitize_unit(input.big_freq_ratio);
    let gpu_queue = if input.gpu_busy_pct.is_finite() {
        (input.gpu_busy_pct / 100.0).clamp(0.0, 1.0)
    } else {
        0.0
    };
    let render_q = sanitize_unit(input.render_pressure);
    let cpu_psi_q = if input.cpu_psi.is_finite() {
        (input.cpu_psi / 35.0).clamp(0.0, 1.0)
    } else {
        0.0
    };
    let cpu_util_q = if input.cpu_util_pct.is_finite() {
        (input.cpu_util_pct / 100.0).clamp(0.0, 1.0)
    } else {
        0.0
    };
    let mem_psi_q = if input.mem_psi.is_finite() {
        (input.mem_psi / 25.0).clamp(0.0, 1.0)
    } else {
        0.0
    };
    let io_psi_q = if input.io_psi.is_finite() {
        (input.io_psi / 25.0).clamp(0.0, 1.0)
    } else {
        0.0
    };
    let avail_ram = if input.avail_ram_ratio.is_finite() {
        input.avail_ram_ratio.clamp(0.0, 1.0)
    } else {
        0.45
    };

    // Little's Law normalized queue pressure per orthogonal domain (0.0..1.0).
    let l_gpu = gpu_queue;
    let l_cpu = render_q.max(cpu_psi_q).max(cpu_util_q * 0.85);
    let l_mem_io = io_psi_q.max(mem_psi_q);

    // 1. Anti Single-Core/Prime Hotspot & Cross-Cluster Co-Sharing:
    // Detect Prime/Big cluster saturation (> 85%) while companion Mid/Little clusters have ample headroom.
    let companion_r = if mid_r > 0.0 {
        mid_r.min(little_r.max(mid_r * 0.85))
    } else {
        little_r
    };
    let cluster_imbalance = if big_r > 0.85 && companion_r <= 0.58 {
        (big_r - companion_r).clamp(0.0, 1.0)
    } else {
        0.0
    };

    let mut regime = REGIME_BALANCED;
    let mut cpu_uclamp_scale: f64 = 1.0;
    let mut helper_uclamp_ratio: f64 = 0.65;
    let mut boost_gpu_memory_supply = false;
    let mut io_supply_boost = false;
    let mut encourage_cluster_spread = false;

    if cluster_imbalance > 0.25 {
        // Relieve single-cluster hotspot: ease Tier-1 Prime pinning slightly while raising Tier-2
        // helper/worker floor so physics/hwui/audio/IO threads share work across Mid/Big companions.
        let spread = ((cluster_imbalance - 0.25) / 0.55).clamp(0.0, 1.0);
        regime = REGIME_SINGLE_HOTSPOT_SPREAD;
        cpu_uclamp_scale = (1.0 - spread * 0.07).clamp(0.92, 1.0);
        helper_uclamp_ratio = (0.68 + spread * 0.14).clamp(0.68, 0.82);
        encourage_cluster_spread = true;
    }

    // 2. CPU-GPU-Memory Symbiotic Co-Pacing (orthogonal axes):
    if l_gpu >= 0.72 && l_cpu <= 0.45 {
        // GPU-bound scene: CPU has spare headroom -> redirect silicon power/thermal budget to GPU
        // and keep GPU/memory supply knobs engaged.
        let gpu_excess = ((l_gpu - 0.72) / 0.28).clamp(0.0, 1.0);
        let gpu_cpu_scale = (0.95 - gpu_excess * 0.05).clamp(0.90, 0.95);
        cpu_uclamp_scale = cpu_uclamp_scale.min(gpu_cpu_scale);
        boost_gpu_memory_supply = true;
        if regime == REGIME_BALANCED {
            regime = REGIME_GPU_BOUND;
        }
    } else if l_cpu >= 0.65 && (0.08..=0.55).contains(&l_gpu) && render_q >= 0.35 {
        // CPU draw-call bound: GPU is actively rendering (>= 8%) and waiting on foreground CPU
        // draw-calls (render_pressure >= 0.35), NOT a 0%-GPU background dex2oat/sync spike.
        let cpu_excess = ((l_cpu - 0.65) / 0.35).clamp(0.0, 1.0);
        if !encourage_cluster_spread {
            cpu_uclamp_scale = (1.04 + cpu_excess * 0.04).clamp(1.04, 1.08);
        }
        helper_uclamp_ratio = helper_uclamp_ratio.max((0.75 + cpu_excess * 0.06).clamp(0.75, 0.81));
        encourage_cluster_spread = true;
        if regime == REGIME_BALANCED {
            regime = REGIME_CPU_DRAW_BOUND;
        }
    }

    if l_mem_io >= 0.40 && avail_ram >= 0.25 {
        // Storage/Memory queue pressure: boost read-ahead & asset supply so compute pipelines don't stall.
        io_supply_boost = true;
        if regime == REGIME_BALANCED {
            regime = REGIME_MEMORY_IO_STARVED;
        }
    }

    // 3. Coupled Multi-Domain Orchestration (Pillar 2):
    //    (a) Compute <-> Memory Bus: scale memory bus floor up to 1.25x OEM baseline
    //        whenever sustained compute demand (CPU/GPU/mem-IO) rises while thermal headroom holds.
    let mut coupling_mask: u8 = 0;
    let compute_demand = l_gpu.max(l_cpu * 0.90).max(l_mem_io);
    let mut memory_bus_scale = 1.0;
    if compute_demand >= 0.55 || boost_gpu_memory_supply || io_supply_boost {
        let excess = ((compute_demand - 0.50) / 0.50).clamp(0.0, 1.0);
        let boost = (0.06 + excess * 0.19) * cap_scale;
        memory_bus_scale = (1.0 + boost).clamp(1.0, 1.25);
        if memory_bus_scale > 1.02 {
            coupling_mask |= COUPLING_COMPUTE_MEMORY_BUS;
        }
    }

    //    (b) Rendering <-> Scheduler Responsiveness: tighten up_rate_limit_us (0.70..1.00)
    //        proportionally as foreground render_pressure climbs.
    let mut scheduler_responsiveness_scale = 1.0;
    if render_q >= 0.35 {
        let render_excess = ((render_q - 0.35) / 0.65).clamp(0.0, 1.0);
        scheduler_responsiveness_scale = (1.0 - render_excess * 0.30).clamp(0.70, 1.0);
        if scheduler_responsiveness_scale < 0.98 {
            coupling_mask |= COUPLING_RENDERING_SCHEDULER;
        }
    }

    //    (c) Performance Headroom <-> Thermal Envelope Step-Down Lead:
    //        Whenever wider headroom is opened (memory bus raised, CPU UClamp boosted,
    //        or GPU/IO supply boosted), shift thermal step-down earlier (0.00..0.15)
    //        so silicon transitions smoothly instead of slamming into thermal throttle.
    let headroom_open = (memory_bus_scale - 1.0) * 0.32
        + (cpu_uclamp_scale - 1.0).max(0.0) * 0.55
        + if boost_gpu_memory_supply { 0.03 } else { 0.0 }
        + if io_supply_boost { 0.02 } else { 0.0 };
    let thermal_stepdown_lead = headroom_open.clamp(0.0, 0.15);
    if thermal_stepdown_lead >= 0.02 {
        coupling_mask |= COUPLING_HEADROOM_THERMAL;
    }

    SymbioticPlan {
        regime,
        cluster_imbalance,
        cpu_uclamp_scale: cpu_uclamp_scale.clamp(0.88, 1.10),
        helper_uclamp_ratio: helper_uclamp_ratio.clamp(0.60, 0.84),
        boost_gpu_memory_supply,
        io_supply_boost,
        encourage_cluster_spread,
        memory_bus_scale,
        scheduler_responsiveness_scale,
        thermal_stepdown_lead,
        coupling_mask,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn neutral_input() -> SymbioticInput {
        SymbioticInput {
            little_freq_ratio: 0.45,
            mid_freq_ratio: 0.50,
            big_freq_ratio: 0.55,
            cpu_util_pct: 30.0,
            render_pressure: 0.30,
            gpu_busy_pct: 30.0,
            cpu_psi: 2.0,
            mem_psi: 1.0,
            io_psi: 1.0,
            avail_ram_ratio: 0.50,
            capacity_scale: 1.0,
            thermal_guard: false,
        }
    }

    #[test]
    fn single_cluster_hotspot_relieves_prime_and_lifts_helpers() {
        let mut inp = neutral_input();
        inp.big_freq_ratio = 0.96;
        inp.mid_freq_ratio = 0.35;
        inp.little_freq_ratio = 0.30;
        let plan = compute_symbiotic_plan(inp);
        assert_eq!(plan.regime, REGIME_SINGLE_HOTSPOT_SPREAD);
        assert!(plan.cluster_imbalance > 0.5);
        assert!(plan.encourage_cluster_spread);
        assert!(plan.cpu_uclamp_scale < 1.0);
        assert!(plan.helper_uclamp_ratio >= 0.76);
    }

    #[test]
    fn gpu_bound_eases_cpu_and_boosts_gpu_supply() {
        let mut inp = neutral_input();
        inp.gpu_busy_pct = 92.0;
        inp.render_pressure = 0.25;
        inp.cpu_psi = 1.0;
        let plan = compute_symbiotic_plan(inp);
        assert_eq!(plan.regime, REGIME_GPU_BOUND);
        assert!(plan.cpu_uclamp_scale <= 0.94);
        assert!(plan.boost_gpu_memory_supply);
    }

    #[test]
    fn cpu_draw_bound_lifts_parallel_workers_to_feed_gpu() {
        let mut inp = neutral_input();
        inp.render_pressure = 0.85;
        inp.cpu_psi = 28.0;
        inp.gpu_busy_pct = 25.0;
        let plan = compute_symbiotic_plan(inp);
        assert_eq!(plan.regime, REGIME_CPU_DRAW_BOUND);
        assert!(plan.cpu_uclamp_scale > 1.04);
        assert!(plan.helper_uclamp_ratio >= 0.76);
        assert!(plan.encourage_cluster_spread);
    }

    #[test]
    fn thermal_guard_returns_strict_orthogonal_neutral() {
        let mut inp = neutral_input();
        inp.big_freq_ratio = 0.98;
        inp.mid_freq_ratio = 0.25;
        inp.gpu_busy_pct = 95.0;
        inp.io_psi = 40.0;
        inp.thermal_guard = true;
        let plan = compute_symbiotic_plan(inp);
        assert_eq!(plan.regime, REGIME_BALANCED);
        assert_eq!(plan.cpu_uclamp_scale, 1.0);
        assert_eq!(plan.helper_uclamp_ratio, 0.65);
        assert!(!plan.boost_gpu_memory_supply);
        assert!(!plan.io_supply_boost);
        assert!(!plan.encourage_cluster_spread);
        assert_eq!(plan.memory_bus_scale, 1.0);
        assert_eq!(plan.scheduler_responsiveness_scale, 1.0);
        assert_eq!(plan.thermal_stepdown_lead, 0.0);
        assert_eq!(plan.coupling_mask, 0);
    }

    #[test]
    fn coupled_domains_scale_bus_scheduler_and_thermal_lead_together() {
        let mut inp = neutral_input();
        inp.render_pressure = 0.88;
        inp.cpu_psi = 26.0;
        inp.gpu_busy_pct = 42.0;
        inp.mem_psi = 14.0;
        let plan = compute_symbiotic_plan(inp);
        assert!(plan.memory_bus_scale > 1.10 && plan.memory_bus_scale <= 1.25);
        assert!(plan.scheduler_responsiveness_scale >= 0.70 && plan.scheduler_responsiveness_scale < 0.85);
        assert!(plan.thermal_stepdown_lead >= 0.05 && plan.thermal_stepdown_lead <= 0.15);
        assert_ne!(plan.coupling_mask & COUPLING_COMPUTE_MEMORY_BUS, 0);
        assert_ne!(plan.coupling_mask & COUPLING_RENDERING_SCHEDULER, 0);
        assert_ne!(plan.coupling_mask & COUPLING_HEADROOM_THERMAL, 0);
    }

    #[test]
    fn zero_gpu_background_cpu_spike_never_triggers_cpu_draw_bound() {
        let mut inp = neutral_input();
        inp.cpu_util_pct = 92.0;
        inp.cpu_psi = 24.0;
        inp.render_pressure = 0.22;
        inp.gpu_busy_pct = 0.0;
        let plan = compute_symbiotic_plan(inp);
        assert_eq!(plan.regime, REGIME_BALANCED);
        assert_eq!(plan.cpu_uclamp_scale, 1.0);
        assert!(!plan.encourage_cluster_spread);

        // Even with higher render_pressure, GPU < 8% cannot be draw-call bound
        let mut inp_low_gpu = neutral_input();
        inp_low_gpu.cpu_util_pct = 92.0;
        inp_low_gpu.cpu_psi = 24.0;
        inp_low_gpu.render_pressure = 0.70;
        inp_low_gpu.gpu_busy_pct = 4.0;
        let plan_low_gpu = compute_symbiotic_plan(inp_low_gpu);
        assert_eq!(plan_low_gpu.regime, REGIME_BALANCED);
    }
}
