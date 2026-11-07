#!/usr/bin/env python3
"""Validate a CoreFlow Device Validation Suite bundle (stdlib only)."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import sys
from datetime import datetime
from pathlib import Path, PurePosixPath
from typing import Any

MAX_FILES = 32
MAX_FILE_BYTES = 8 * 1024 * 1024
MAX_TOTAL_BYTES = 24 * 1024 * 1024
REQUIRED_FILES = {"report.json", "events.jsonl", "summary.txt"}
PROFILES = {"BOOT_STABILITY", "IDLE_EQUILIBRIUM", "INTERACTIVE_CPU", "GAMING_LOAD", "MULTITASKING", "THERMAL_RECOVERY", "OWNERSHIP_CONFLICT", "ROLLBACK_RECOVERY"}
VALIDITY = {"valid", "unavailable", "unsupported", "permission_denied", "stale", "invalid"}
CHECK_STATUS = {"pass", "fail", "warning", "inconclusive", "not_applicable"}
VERDICTS = {"PASS", "PASS_WITH_WARNINGS", "INCONCLUSIVE", "FAIL", "RECOVERY_REQUIRED"}


class ValidationError(Exception):
    pass


def _safe_file(root: Path, rel: str) -> Path:
    if not isinstance(rel, str) or not rel or "\\" in rel:
        raise ValidationError("manifest contains an invalid path")
    posix = PurePosixPath(rel)
    if posix.is_absolute() or any(part in ("", ".", "..") for part in posix.parts):
        raise ValidationError(f"unsafe manifest path: {rel!r}")
    path = root.joinpath(*posix.parts)
    current = root
    for part in posix.parts:
        current = current / part
        if current.is_symlink():
            raise ValidationError(f"symlink is not allowed: {rel}")
    if not path.is_file():
        raise ValidationError(f"manifest file missing: {rel}")
    return path


def _computed_verdict(report: dict[str, Any]) -> str:
    recovery = report["recovery"]
    if recovery["pending"] or not recovery["journal_clear"]:
        return "RECOVERY_REQUIRED"
    checks = report["checks"]
    required = [c for c in checks if c["required"]]
    if any(c["required"] and c["status"] == "fail" for c in checks):
        return "FAIL"
    if not required or any(c["status"] in {"inconclusive", "not_applicable"} for c in required):
        return "INCONCLUSIVE"
    if any(c["status"] != "pass" for c in required):
        return "INCONCLUSIVE"
    if any(c["status"] in {"warning", "fail"} for c in checks):
        return "PASS_WITH_WARNINGS"
    return "PASS"


def _validate_report(report: Any) -> list[str]:
    errors: list[str] = []
    if not isinstance(report, dict):
        return ["report.json must contain an object"]
    required_top = {"schema_version", "session", "profile", "build", "metrics", "checks", "recovery", "verdict"}
    missing = sorted(required_top - report.keys())
    if missing:
        errors.append("missing report fields: " + ", ".join(missing))
        return errors
    if report["schema_version"] != "1.0": errors.append("unsupported schema_version")
    if not isinstance(report["profile"], str) or report["profile"] not in PROFILES: errors.append("unknown profile")
    if not isinstance(report["verdict"], str) or report["verdict"] not in VERDICTS: errors.append("unknown verdict")
    session, build, metrics, checks, recovery = (report[k] for k in ("session", "build", "metrics", "checks", "recovery"))
    if not isinstance(session, dict) or not re.fullmatch(r"[0-9a-fA-F-]{36}", str(session.get("id", ""))):
        errors.append("session.id must be a UUID")
    else:
        for field in ("started_at", "ended_at"):
            try:
                stamp = str(session[field])
                parsed = datetime.fromisoformat(stamp.replace("Z", "+00:00"))
                if not stamp.endswith("Z") or parsed.utcoffset() is None: raise ValueError("timestamp must be UTC")
            except (KeyError, ValueError):
                errors.append(f"session.{field} must be an RFC 3339 UTC timestamp ending in Z")
        device = session.get("device")
        if not isinstance(device, dict) or not isinstance(device.get("manufacturer"), str) or not isinstance(device.get("model"), str) or not isinstance(device.get("android_api"), int) or isinstance(device.get("android_api"), bool):
            errors.append("session.device requires manufacturer, model, and android_api")
    if not isinstance(build, dict) or not re.fullmatch(r"(?:[a-f0-9]{7,40}|unknown)", str(build.get("commit", ""))):
        errors.append("build.commit must be a hexadecimal commit id")
    if not isinstance(metrics, dict):
        errors.append("metrics must be an object")
    else:
        for name, metric in metrics.items():
            if not isinstance(metric, dict):
                errors.append(f"metric {name}: expected object")
                continue
            for key in ("value", "unit", "validity", "source", "observed_at"):
                if key not in metric: errors.append(f"metric {name}: missing {key}")
            if not isinstance(metric.get("validity"), str) or metric.get("validity") not in VALIDITY: errors.append(f"metric {name}: invalid validity")
            if metric.get("value") is None and not metric.get("reason"):
                errors.append(f"metric {name}: null value requires reason")
            value = metric.get("value")
            if value is not None and not isinstance(value, (int, float, str, bool)): errors.append(f"metric {name}: value must be scalar or null")
            if isinstance(value, (int, float)) and not isinstance(value, bool) and not math.isfinite(value): errors.append(f"metric {name}: non-finite value")
            if not isinstance(metric.get("source"), str) or not metric.get("source"): errors.append(f"metric {name}: source must be non-empty")
    if not isinstance(checks, list):
        errors.append("checks must be an array")
    else:
        seen = set()
        for check in checks:
            if not isinstance(check, dict):
                errors.append("check entry must be an object")
                continue
            ident = check.get("id")
            if not isinstance(ident, str) or not re.fullmatch(r"[a-z][a-z0-9_.-]{1,63}", ident):
                errors.append("check has invalid id")
            elif ident in seen:
                errors.append(f"duplicate check id: {ident}")
            else:
                seen.add(ident)
            if not isinstance(check.get("required"), bool): errors.append(f"check {ident}: required must be boolean")
            if check.get("status") not in CHECK_STATUS: errors.append(f"check {ident}: invalid status")
    if not isinstance(recovery, dict) or any(not isinstance(recovery.get(k), bool) for k in ("pending", "journal_clear", "rollback_verified")):
        errors.append("recovery must contain boolean pending, journal_clear, rollback_verified")
    if not errors:
        computed = _computed_verdict(report)
        if report["verdict"] != computed:
            errors.append(f"verdict mismatch: report says {report['verdict']}, evidence computes {computed}")
    return errors


def validate_bundle(bundle: Path) -> tuple[bool, list[str]]:
    errors: list[str] = []
    try:
        root = bundle.resolve(strict=True)
        if not root.is_dir(): raise ValidationError("bundle path is not a directory")
        if bundle.is_symlink(): raise ValidationError("bundle root must not be a symlink")
        manifest_path = _safe_file(root, "manifest.json")
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        if not isinstance(manifest, dict) or manifest.get("manifest_version") != "1.0":
            raise ValidationError("unsupported manifest_version")
        entries = manifest.get("files")
        if not isinstance(entries, list): raise ValidationError("manifest.files must be an array")
        if len(entries) > MAX_FILES: raise ValidationError("bundle contains too many files")
        seen: set[str] = set()
        total = 0
        for entry in entries:
            if not isinstance(entry, dict): raise ValidationError("manifest entry must be an object")
            rel = entry.get("path")
            if not isinstance(rel, str): raise ValidationError("manifest path must be a string")
            if rel in seen: raise ValidationError(f"duplicate manifest path: {rel}")
            seen.add(rel)
            path = _safe_file(root, rel)
            size = path.stat().st_size
            total += size
            if size > MAX_FILE_BYTES: raise ValidationError(f"file too large: {rel}")
            if entry.get("size_bytes") != size: raise ValidationError(f"size mismatch: {rel}")
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            if entry.get("sha256") != digest: raise ValidationError(f"SHA-256 mismatch: {rel}")
        if total > MAX_TOTAL_BYTES: raise ValidationError("bundle exceeds total size limit")
        if not REQUIRED_FILES.issubset(seen):
            raise ValidationError("manifest is missing required files: " + ", ".join(sorted(REQUIRED_FILES - seen)))
        report = json.loads(_safe_file(root, "report.json").read_text(encoding="utf-8"))
        errors.extend(_validate_report(report))
        last_sequence = 0
        expected_session = report.get("session", {}).get("id") if isinstance(report, dict) else None
        for line_no, line in enumerate(_safe_file(root, "events.jsonl").read_text(encoding="utf-8").splitlines(), 1):
            if not line.strip(): continue
            try: event = json.loads(line)
            except json.JSONDecodeError as exc: raise ValidationError(f"events.jsonl:{line_no}: invalid JSON: {exc}")
            if not isinstance(event, dict): raise ValidationError(f"events.jsonl:{line_no}: event must be an object")
            for key in ("session_id", "sequence", "timestamp", "event_type"):
                if key not in event: raise ValidationError(f"events.jsonl:{line_no}: missing {key}")
            if event["session_id"] != expected_session:
                raise ValidationError(f"events.jsonl:{line_no}: session_id mismatch")
            if not isinstance(event["sequence"], int) or isinstance(event["sequence"], bool) or event["sequence"] <= last_sequence:
                raise ValidationError(f"events.jsonl:{line_no}: sequence must increase monotonically")
            last_sequence = event["sequence"]
            if not isinstance(event["event_type"], str) or not event["event_type"]:
                raise ValidationError(f"events.jsonl:{line_no}: event_type must be non-empty")
            try:
                stamp = event["timestamp"]
                parsed = datetime.fromisoformat(stamp.replace("Z", "+00:00"))
                if not stamp.endswith("Z") or parsed.utcoffset() is None: raise ValueError("timestamp must be UTC")
            except (AttributeError, ValueError):
                raise ValidationError(f"events.jsonl:{line_no}: invalid UTC timestamp")
    except (OSError, UnicodeError, json.JSONDecodeError, ValidationError) as exc:
        errors.append(str(exc))
    return not errors, errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=Path, help="directory containing a CF-DVS bundle")
    args = parser.parse_args()
    if not args.bundle.exists():
        print(f"CF-DVS: input does not exist: {args.bundle}", file=sys.stderr)
        return 2
    valid, errors = validate_bundle(args.bundle)
    if valid:
        print("CF-DVS: bundle integrity and report contract PASS")
        return 0
    print("CF-DVS: bundle INVALID")
    for error in errors: print(f"- {error}")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
