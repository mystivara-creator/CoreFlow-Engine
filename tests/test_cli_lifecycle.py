#!/usr/bin/env python3
"""Verify CFAE System Framework / CoreFlow Engine v6.0.0-R Sovereign Rebuild daemon CLI & safety contracts."""
import json
import subprocess
import sys
from pathlib import Path

repo = Path(__file__).resolve().parents[1]
source = (repo / "src/daemon/main.cpp").read_text()
config = (repo / "include/coreflow/config.hpp").read_text()

# Core v6.1.0-R Unified Zero-Profile Autonomous architecture & safety invariants
assert 'constexpr char kVersion[] = "6.1.0-R";' in source
assert 'std::string version{"6.1.0-R"};' in config
assert "VulkanController" not in source
assert "DeviceValidationRecorder" not in source
assert "ThreadSchedulerActuator" in source
assert "StorageMemoryActuator" in source
assert "ThermalEnvelopeActuator" in source
assert "ContextEngine" in source
assert "HardwareProbe" in source
assert "RenderLoadSampler" in source
assert "DecisionJournal" in source
assert "FramePacingSensor" in source
assert "AppDnaStore" in source
assert "compute_thermal_equilibrium" in source
assert "compute_symbiotic_plan" in source
assert "feedforward_freeze_until_ms" in source
assert "vblank_irq_token" in source
assert "to_dna_signature" in source
assert "BootCalmSanctuary" in source
assert "dispatch_tamper_broadcast" in source
assert "handle_resolve_tamper_cli" in source
assert "observe_context" in source
assert "flush_if_significant" in source
assert "O_EXCL" in source and "renameat" in source
assert "baseline.recover_pending()" in source
assert "baseline.rollback_all()" in source
assert "thread_scheduler.reset_all()" in source
assert "build_sysfs_ctx(" in source
assert "apply_storage_pacing()" not in source
assert "monitor_battery_thermals" in source
assert "apply_power_control" in source
assert "service_launcher_authorized()" in source

# Live CLI execution against compiled coreflowd binary
if len(sys.argv) >= 2:
    binary = Path(sys.argv[1])
    assert binary.is_file(), f"Missing coreflowd binary: {binary}"
    for flag in ("--status", "--capabilities", "--atlas", "--episodes", "--journal", "--diag"):
        proc = subprocess.run(
            [str(binary), flag],
            capture_output=True,
            text=True,
            timeout=15,
            check=False,
        )
        assert proc.returncode == 0, f"{flag} exited with {proc.returncode}: {proc.stderr}"
        lines = [line.strip() for line in proc.stdout.splitlines() if line.strip()]
        assert lines, f"{flag} produced empty stdout"
        payload = json.loads(lines[-1])
        assert isinstance(payload, dict), f"{flag} JSON root is not an object"
        if flag == "--status":
            assert payload.get("version") == "6.1.0-R"
            assert "profile" in payload
            assert "effectiveProfile" in payload
            assert "autonomousRegime" in payload and isinstance(payload["autonomousRegime"], str)
            assert "lowBatteryEco" in payload and isinstance(payload["lowBatteryEco"], bool)
            assert "appDnaEntries" in payload and isinstance(payload["appDnaEntries"], list)
            assert "mode" in payload
            for bs_key in (
                "bootSettleActive",
                "bootSettleDurationSec",
                "bootSettleExitReason",
                "bootRampProgress",
            ):
                assert bs_key in payload, f"Missing {bs_key}"
            assert "features" in payload and isinstance(payload["features"], dict)
            assert "hyperFluid" in payload and isinstance(payload["hyperFluid"], dict)
            for hf_key in (
                "framePacingAvailable",
                "measuredFps",
                "jankRatio",
                "appDnaLearned",
                "activeAppBias",
                "activeAppEfficiencyWeight",
                "appDnaFlushCount",
                "chargingDamped",
            ):
                assert hf_key in payload["hyperFluid"], f"Missing hyperFluid.{hf_key}"
            assert "tamperGuard" in payload and isinstance(payload["tamperGuard"], dict)
            for tg_key in ("enabled", "lockedNodes", "quarantinedNodes", "totalEvents"):
                assert tg_key in payload["tamperGuard"], f"Missing tamperGuard.{tg_key}"
            assert isinstance(payload.get("tamperEvents"), list)
            assert "coupledOrchestration" in payload and isinstance(payload["coupledOrchestration"], dict)
            for co_key in (
                "regime",
                "memoryBusScale",
                "schedulerResponsivenessScale",
                "thermalStepdownLead",
                "couplingMask",
                "memoryBusNodes",
            ):
                assert co_key in payload["coupledOrchestration"], f"Missing coupledOrchestration.{co_key}"
            assert isinstance(payload.get("conflictingModules"), list)
            assert "powerConfig" in payload and isinstance(payload["powerConfig"], dict)
            assert "apps" in payload and isinstance(payload["apps"], dict)
            for bucket in ("gaming", "smoothness", "battery", "excluded"):
                assert isinstance(payload["apps"].get(bucket), list)
            assert "actuators" in payload and isinstance(payload["actuators"], dict)
            assert isinstance(payload["actuators"].get("contendedPaths"), list)
            assert "easTopAppCapPct" in payload["actuators"]
            assert "easCappedGroups" in payload["actuators"]
            model = payload.get("thermalModel")
            assert isinstance(model, dict) and model["valid"] is False
            assert model["rThetaCPerW"] is None and model["sustainablePowerMw"] is None
            assert model["targetCelsius"] == 38.0 and model["confidence"] == 0
            assert "memoryBusKnobs" in payload["actuators"]
            assert "hardware" in payload and isinstance(payload["hardware"], dict)
            assert "memory" in payload and isinstance(payload["memory"], dict)
            assert isinstance(payload.get("cpuClusters"), list)
            assert isinstance(payload.get("thermalZones"), list)
            assert isinstance(payload.get("episodes"), list)
            assert "batteryPower" in payload and isinstance(payload["batteryPower"], dict)
            assert "bypassSupported" in payload["batteryPower"]
            assert "session" in payload and isinstance(payload["session"], dict)
            assert "history" in payload and isinstance(payload["history"], list)
            assert "aiJankRisk" in payload and isinstance(payload["aiJankRisk"], (int, float))
            assert 0.0 <= float(payload["aiJankRisk"]) <= 1.0
            assert "aiWorkloadClass" in payload and isinstance(payload["aiWorkloadClass"], str)
            assert payload["aiWorkloadClass"] in (
                "idle", "balanced", "interactive", "io_bound", "gpu_heavy", "thermal_surge"
            )
            assert "aiBackend" in payload and isinstance(payload["aiBackend"], str)
            assert payload["aiBackend"] in (
                "embedded-rls-mlp", "cfw2-micro-mlp", "custom-cfw-rls-mlp", "onnx-runtime-live"
            )
            assert "vulkan" not in payload
        elif flag == "--capabilities":
            assert payload.get("version") == "6.1.0-R"
            assert payload.get("rustPidEngine") is True
            assert "perThreadUclampSupported" in payload
            assert "bypassChargingSupported" in payload
            assert "storageType" in payload
        elif flag == "--atlas":
            assert payload.get("atlasFormat") == "coreflow-atlas-v6"
            assert payload.get("version") == "6.1.0-R"
            assert isinstance(payload.get("cpuClusters"), list)
            assert isinstance(payload.get("gpu"), dict)
            assert isinstance(payload.get("powerControl"), dict)
            assert isinstance(payload.get("thermalZones"), list)
        elif flag == "--episodes":
            assert payload.get("version") == "6.1.0-R"
            assert isinstance(payload.get("episodes"), list)
        elif flag == "--journal":
            assert payload.get("format") == "CFJ4"
            assert isinstance(payload.get("entries"), list)
        elif flag == "--diag":
            assert payload.get("reportFormat") == "coreflow-diag-v6"
            assert payload.get("version") == "6.1.0-R"
            assert isinstance(payload.get("capabilities"), dict)
            assert isinstance(payload.get("atlas"), dict)
            assert isinstance(payload.get("status"), dict)
            assert isinstance(payload.get("journal"), dict)

    # Verify --recover-only is supported cleanly in CLI contract (T4)
    assert '"--recover-only"' in source

    # Verify --resolve-tamper strictly rejects YIELD/bypass compromise actions!
    bad_tamper = subprocess.run(
        [str(binary), "--resolve-tamper", "/sys/test/node", "YIELD"],
        capture_output=True,
        text=True,
        timeout=10,
        check=False,
    )
    assert bad_tamper.returncode == 2, "Sovereign CLI must reject YIELD action!"

# Verify CFW2 offline toolchain round-trip (zero third-party pip deps)
import tempfile
with tempfile.TemporaryDirectory(prefix="coreflow-cfw2-cli-") as tmpdir:
    default_cfw2 = Path(tmpdir) / "default.cfw2"
    trained_cfw2 = Path(tmpdir) / "trained.cfw2"
    exp = subprocess.run(
        [sys.executable, str(repo / "tools/onnx_to_cfw2.py"), "--export-default", str(default_cfw2)],
        capture_output=True, text=True, timeout=15, check=False,
    )
    assert exp.returncode == 0, f"onnx_to_cfw2 --export-default failed: {exp.stderr}"
    assert default_cfw2.stat().st_size == 1700
    insp = subprocess.run(
        [sys.executable, str(repo / "tools/onnx_to_cfw2.py"), "--inspect", str(default_cfw2)],
        capture_output=True, text=True, timeout=15, check=False,
    )
    assert insp.returncode == 0 and "CFW2 VALID" in insp.stdout
    trn = subprocess.run(
        [sys.executable, str(repo / "tools/train_cfw2_from_diag.py"), "--epochs", "10", "-o", str(trained_cfw2)],
        capture_output=True, text=True, timeout=20, check=False,
    )
    assert trn.returncode == 0, f"train_cfw2_from_diag failed: {trn.stderr}"
    assert trained_cfw2.stat().st_size == 1700
    insp2 = subprocess.run(
        [sys.executable, str(repo / "tools/onnx_to_cfw2.py"), "--inspect", str(trained_cfw2)],
        capture_output=True, text=True, timeout=15, check=False,
    )
    assert insp2.returncode == 0 and "CFW2 VALID" in insp2.stdout

    # Verify trainer safely survives null/corrupt fields and aggressive learning rate via best-checkpoint rollback
    corrupt_diag = Path(tmpdir) / "corrupt_diag.json"
    corrupt_diag.write_text(json.dumps({
        "status": {
            "memPsi": None,
            "ioPsi": "bad",
            "gpuBusyPct": None,
            "history": [
                {"ts": 1000 * i, "pressure": None if i == 3 else 45.0 + 0.5 * i, "cpu": None if i == 5 else 20.0, "scale": 1.0}
                for i in range(16)
            ],
        }
    }), encoding="utf-8")
    trained_corrupt = Path(tmpdir) / "trained_corrupt.cfw2"
    trn2 = subprocess.run(
        [
            sys.executable, str(repo / "tools/train_cfw2_from_diag.py"),
            "--diag", str(corrupt_diag), "--epochs", "6", "--lr", "0.5",
            "-o", str(trained_corrupt),
        ],
        capture_output=True, text=True, timeout=20, check=False,
    )
    assert trn2.returncode == 0, f"train_cfw2_from_diag failed on corrupt diag: {trn2.stderr}"
    assert trained_corrupt.stat().st_size == 1700

print("CoreFlow v6.0.0-R Sovereign Rebuild CLI lifecycle contract PASS")
