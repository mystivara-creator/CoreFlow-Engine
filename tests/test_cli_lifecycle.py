#!/usr/bin/env python3
"""Verify CoreFlow v6.1.0 NextGen daemon CLI & safety contracts."""
import json
import subprocess
import sys
from pathlib import Path

repo = Path(__file__).resolve().parents[1]
source = (repo / "src/daemon/main.cpp").read_text()
config = (repo / "include/coreflow/config.hpp").read_text()

# Core v6 architecture & safety invariants
assert 'constexpr char kVersion[] = "6.1.0";' in source
assert 'std::string version{"6.1.0"};' in config
assert "VulkanController" not in source
assert "DeviceValidationRecorder" not in source
assert "ThreadSchedulerActuator" in source
assert "StorageMemoryActuator" in source
assert "ThermalEnvelopeActuator" in source
assert "ContextEngine" in source
assert "HardwareProbe" in source
assert "RenderLoadSampler" in source
assert "DecisionJournal" in source
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
            assert payload.get("version") == "6.1.0"
            assert "profile" in payload
            assert "effectiveProfile" in payload
            assert "mode" in payload
            assert "features" in payload and isinstance(payload["features"], dict)
            assert "powerConfig" in payload and isinstance(payload["powerConfig"], dict)
            assert "apps" in payload and isinstance(payload["apps"], dict)
            for bucket in ("gaming", "smoothness", "battery", "excluded"):
                assert isinstance(payload["apps"].get(bucket), list)
            assert "actuators" in payload and isinstance(payload["actuators"], dict)
            assert isinstance(payload["actuators"].get("contendedPaths"), list)
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
            assert payload.get("version") == "6.1.0"
            assert payload.get("rustPidEngine") is True
            assert "perThreadUclampSupported" in payload
            assert "bypassChargingSupported" in payload
            assert "storageType" in payload
        elif flag == "--atlas":
            assert payload.get("atlasFormat") == "coreflow-atlas-v6"
            assert payload.get("version") == "6.1.0"
            assert isinstance(payload.get("cpuClusters"), list)
            assert isinstance(payload.get("gpu"), dict)
            assert isinstance(payload.get("powerControl"), dict)
            assert isinstance(payload.get("thermalZones"), list)
        elif flag == "--episodes":
            assert payload.get("version") == "6.1.0"
            assert isinstance(payload.get("episodes"), list)
        elif flag == "--journal":
            assert payload.get("format") == "CFJ4"
            assert isinstance(payload.get("entries"), list)
        elif flag == "--diag":
            assert payload.get("reportFormat") == "coreflow-diag-v6"
            assert payload.get("version") == "6.1.0"
            assert isinstance(payload.get("capabilities"), dict)
            assert isinstance(payload.get("atlas"), dict)
            assert isinstance(payload.get("status"), dict)
            assert isinstance(payload.get("journal"), dict)

print("CoreFlow v6.1.0 CLI lifecycle contract PASS")
