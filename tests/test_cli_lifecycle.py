#!/usr/bin/env python3
"""Verify CoreFlow v5.0.1 lean 4-pillar daemon CLI & safety contracts."""
import json
import subprocess
import sys
from pathlib import Path

repo = Path(__file__).resolve().parents[1]
source = (repo / "src/daemon/main.cpp").read_text()
config = (repo / "include/coreflow/config.hpp").read_text()

# Core v5 architecture & safety invariants
assert 'constexpr char kVersion[] = "5.0.1";' in source
assert 'std::string version{"5.0.1"};' in config
assert "VulkanController" not in source
assert "DeviceValidationRecorder" not in source
assert "ThreadSchedulerActuator" in source
assert "StorageMemoryActuator" in source
assert "ThermalEnvelopeActuator" in source
assert "ContextEngine" in source
assert "HardwareProbe" in source
assert "O_EXCL" in source and "renameat" in source
assert "baseline.recover_pending()" in source
assert "baseline.rollback_all()" in source
assert "thread_scheduler.reset_all()" in source
assert "apply_storage_pacing()" in source
assert "monitor_battery_thermals" in source
assert "service_launcher_authorized()" in source

# Live CLI execution against compiled coreflowd binary
if len(sys.argv) >= 2:
    binary = Path(sys.argv[1])
    assert binary.is_file(), f"Missing coreflowd binary: {binary}"
    for flag in ("--status", "--capabilities", "--journal"):
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
            assert payload.get("version") == "5.0.1"
            assert "profile" in payload
            assert "mode" in payload
            assert "features" in payload and isinstance(payload["features"], dict)
            assert "apps" in payload and isinstance(payload["apps"], dict)
            assert "actuators" in payload and isinstance(payload["actuators"], dict)
            assert "hardware" in payload and isinstance(payload["hardware"], dict)
            assert "batteryPower" in payload and isinstance(payload["batteryPower"], dict)
            assert "history" in payload and isinstance(payload["history"], list)
            assert "vulkan" not in payload
        elif flag == "--capabilities":
            assert payload.get("version") == "5.0.1"
            assert payload.get("rustPidEngine") is True
            assert "perThreadUclampSupported" in payload
            assert "storageType" in payload
        elif flag == "--journal":
            assert payload.get("format") == "CFJ4"
            assert isinstance(payload.get("entries"), list)

print("CoreFlow v5 CLI lifecycle contract PASS")
