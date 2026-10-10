# CoreFlow Autonomous — Device validation checklist

This package is **host-tested**. Device behavior is **not certified** until the steps below pass on real hardware.

## 0. Build & install

- [ ] CI/NDK produces `coreflowd` ARM64 for the target min API
- [ ] Module installs on Magisk / KernelSU / APatch without service errors
- [ ] `/data/adb/coreflow/status.json` updates each monitor interval

## 1. Observe-only (default)

- [ ] After boot, config remains observe / unarmed
- [ ] Live WebUI shows blocker other than a false Observe when Adaptive is *not* requested
- [ ] No sysfs/proc writes from CoreFlow (journal empty or restore-only)

## 2. Adaptive path

Config minimum:

```
mutation_mode=adaptive
mutation_armed=true
allow_cpu_governor=no
```

- [ ] `status.json` → `"mode":"adaptive"`, `"armed":true`
- [ ] Live banner is **not** stuck on Observe solely because of mode string case
- [ ] Blocker progresses: BASELINE_CAPTURE → (optional COOLDOWN/CONTEXT_*) → READY or a real gate
- [ ] Brain shows Decision agent reason when candidates exist
- [ ] At most one resource mutation per epoch; journal commits before write
- [ ] Idle / disarm restores factory values

## 3. Stress / safety

- [ ] ThermalGuard / Pressure only allows stabilizing resource writes
- [ ] Kill switch `DISABLE` and `SAFE_MODE` stop mutation and restore
- [ ] CPU governor remains off unless `allow_cpu_governor=yes`

## 4. Multi-device notes (fill per device)

| Device | SoC | Kernel | Root | Result |
|--------|-----|--------|------|--------|
|        |     |        |      |        |

## Pass criteria for community RC

Observe soak ≥ 24h without unintended writes; Adaptive resource path shows ≥1 verified mutation and clean restore on one primary device; no boot-loop; status/WebUI agree with config.
