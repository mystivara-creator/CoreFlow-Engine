#!/usr/bin/env python3
"""Validate a bundle emitted by the native CF-DVS recorder."""
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from cf_dvs_validate import validate_bundle


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: test_cf_dvs_runtime_bundle.py <native-test-binary>", file=sys.stderr)
        return 2
    with tempfile.TemporaryDirectory(prefix="cf-dvs-contract-") as tmp:
        result = subprocess.run([sys.argv[1], tmp], check=False, text=True, capture_output=True, timeout=10)
        if result.returncode != 0:
            print(result.stdout)
            print(result.stderr, file=sys.stderr)
            return result.returncode or 1
        marker = next((line for line in result.stdout.splitlines() if line.startswith("CF_DVS_BUNDLE=")), None)
        if marker is None:
            print("native recorder did not report its finalized bundle", file=sys.stderr)
            return 1
        bundle = Path(marker.split("=", 1)[1])
        valid, errors = validate_bundle(bundle)
        if not valid:
            print("native bundle failed the shared report contract:", file=sys.stderr)
            for error in errors:
                print("- " + error, file=sys.stderr)
            return 1
        print("Native CF-DVS bundle passes the offline Python contract validator")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
