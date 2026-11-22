#!/usr/bin/env python3
"""Verify CFAE System Framework / CoreFlow Engine v6.0.0-R Sovereign Rebuild daemon CLI & safety contracts."""
import json
import subprocess
import sys
from pathlib import Path

repo = Path(__file__).resolve().parents[1]
source = (repo / "src/daemon/main.cpp").read_text()
config = (repo / "include/coreflow/config.hpp").read_text()

# Core v6.0.0-R Sovereign Rebuild architecture & safety invariants
assert 'constexpr char kVersion[] = "6.0.0-R";' in source
assert 'std::string version{"6.0.0-R"};' in config
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
            assert payload.get("version") == "6.0.0-R"
            assert "profile" in payload
            assert "effectiveProfile" in payload
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
            assert "vulkan" not in payload
        elif flag == "--capabilities":
            assert payload.get("version") == "6.0.0-R"
            assert payload.get("rustPidEngine") is True
            assert "perThreadUclampSupported" in payload
            assert "bypassChargingSupported" in payload
            assert "storageType" in payload
        elif flag == "--atlas":
            assert payload.get("atlasFormat") == "coreflow-atlas-v6"
            assert payload.get("version") == "6.0.0-R"
            assert isinstance(payload.get("cpuClusters"), list)
            assert isinstance(payload.get("gpu"), dict)
            assert isinstance(payload.get("powerControl"), dict)
            assert isinstance(payload.get("thermalZones"), list)
        elif flag == "--episodes":
            assert payload.get("version") == "6.0.0-R"
            assert isinstance(payload.get("episodes"), list)
        elif flag == "--journal":
            assert payload.get("format") == "CFJ4"
            assert isinstance(payload.get("entries"), list)
        elif flag == "--diag":
            assert payload.get("reportFormat") == "coreflow-diag-v6"
            assert payload.get("version") == "6.0.0-R"
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

print("CoreFlow v6.0.0-R Sovereign Rebuild CLI lifecycle contract PASS")
