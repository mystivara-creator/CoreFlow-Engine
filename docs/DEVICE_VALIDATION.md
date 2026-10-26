# On-device validation checklist

Operator checklist. Not a certificate of universal compatibility.

## Before Adaptive

1. Install the **CI release ZIP** for the target ABI (arm64).
2. Confirm `coreflowd` starts after reboot (logcat / module status).
3. Leave `mutation_mode=observe` and `mutation_armed=false`.
4. Confirm WebUI / `status.json` updates.
5. Confirm journal directory is writable under the module data path.

## Optional thermal model

- ONNX ships with the module as a **best-effort** thermal aid.
- No claim that the model is calibrated for every SoC.
- Daemon logic does not require a validated model to observe or to apply bounded, permitted mutations when Adaptive is deliberately armed.

## Adaptive (only if you accept risk)

1. Arm only after observe soak looks healthy on **this** device.
2. Prefer one domain at a time if debugging.
3. Verify journal restore after a forced process stop.
4. Keep a known-good recovery path (module disable / last good config).

## Evidence worth keeping

- Commit SHA / module version
- Device model + kernel
- Log excerpts for start, first mutation, restore
- Whether thermal model loaded or fell back
