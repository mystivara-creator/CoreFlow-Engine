# CDDP Synthetic Fixtures A-L

Fixtures are materialized into isolated temporary sysfs/procfs trees by `tests/cddp_fixture_factory.hpp`; they require neither root nor a real Android device.

| Fixture | Surface | Expected outcome |
|---|---|---|
| A | Valid CPUFreq, memory, PSI, cgroup v2, thermal | Eligibility and apply/readback/restore path |
| B | Non-contiguous CPU membership | Exact CPU IDs retained |
| C | Malformed online CPU set | Topology invalid; mutation rejected |
| D | Writable CPUFreq node but no advertised table | Semantics known; constraints unverified |
| E | Read-only CPUFreq max node | Remains READABLE even under uid 0 |
| F | Contradictory CPUFreq bounds | Constraints verification rejected |
| G | Generic devfreq bus | Not presumed to be GPU |
| H | Recognized Mali devfreq | GPU max capability verified in Hz |
| I | Qualcomm KGSL-like nodes | Observation-only |
| J | Sparse system tree | Missing capabilities remain unavailable |
| K | Whitespace-delimited `related_cpus` | CPU policy membership parsed cleanly |
| L | Malformed `related_cpus` with valid `affected_cpus` | Falls back to valid `affected_cpus` |

The E2E test claims verified actuation and restoration only. It does not claim performance improvement without comparable runtime telemetry.

## CF-Sim Virtual SoC Farm (`tests/cf_sim_plant.hpp` + `tests/cf_sim_farm.cpp`)

Extending the isolated sysfs/procfs pattern above (using per-worker `/dev/shm` tmpfs roots with automatic `/tmp` fallback), `CFSimVirtualSoCFarmTest` executes the real `coreflowd_sim` control loop (`--sim-loop`, built only when `CORE_FLOW_BUILD_TESTS=ON`; never present in production `coreflowd` or Android ARM64 NDK binaries) in deterministic lockstep against a coupled 2-node RC thermal + convex $C \cdot V^2 \cdot f$ physics plant across 6 cross-OEM device archetypes:

1. `snapdragon_hyperos` — 3-cluster WALT + Adreno KGSL + Tier-A SDE + cold-boot `BootCalmSanctuary` + adversarial `mi_thermald`
2. `mediatek_dimensity` — 3-cluster schedutil + MediaTek Mali/GED + Tier-A `mtk_fpsgo` + `schedstat` starvation rescue + adversarial `mtk_fpsgo`
3. `samsung_exynos_sdhms` — 3-cluster schedutil + Mali/Xclipse devfreq + fixed-Hz 120 Hz non-LTPO quarantine + aggressive SDHMS/GOS
4. `google_tensor_ltpo` — 3-cluster schedutil + Mali devfreq + dynamic LTPO 1–120 Hz VBlank probation/promotion + 60 FPS cadence anchor + micro-stutter rescue
5. `budget_4gb_emmc_legacy` — 2-cluster + 4 GB RAM + eMMC + legacy kernel without PSI/MGLRU/`schedstat` + fixed 60 Hz non-LTPO quarantine + legacy `/proc/vmstat` 2-regime thrashing & direct-reclaim storm
6. `gaming_bypass_ambient_cooler` — Flagship 144 Hz + tropical ambient jump (26 °C → 34.5 °C) + sudden Peltier cooler attach (16 °C, low $R_\theta$) + bypass charging + UVLO floor (≤ 20%) & resume hysteresis (≥ 25%)

Every archetype enforces 0 controller oscillation (`<= 2` scale reversals/min, slew-rate compliant), 0 post-yield OEM write-wars, 0 false `THERMAL_GUARD`, closed-loop thermal convergence, non-LTPO fail-closed quarantine where applicable, and 100% clean `CFJ4` rollback.