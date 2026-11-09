import hashlib
import json
import tempfile
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.cf_dvs_validate import validate_bundle


class CfDvsValidatorTest(unittest.TestCase):
    def make_bundle(self, root: Path, verdict="PASS", recovery=None, checks=None):
        recovery = recovery or {"pending": False, "journal_clear": True, "rollback_verified": True}
        checks = checks or [{"id": "daemon.running", "required": True, "status": "pass"}]
        report = {
            "schema_version": "1.0",
            "session": {
                "id": "123e4567-e89b-12d3-a456-426614174000",
                "started_at": "2026-11-08T00:00:00Z",
                "ended_at": "2026-11-08T00:05:00Z",
                "device": {"manufacturer": "Example", "model": "Test Device", "android_api": 35},
            },
            "profile": "INTERACTIVE_CPU",
            "build": {"version": "3.1.0", "commit": "e30174c630c77790dd7fb285e23a70a2035a3c63"},
            "metrics": {
                "cpu_psi_some_avg10": {"value": 0.0, "unit": "%", "validity": "valid", "source": "procfs.psi", "observed_at": "2026-11-08T00:01:00Z"},
                "thermal_headroom": {"value": None, "unit": "ratio", "validity": "unsupported", "source": "thermal.headroom", "observed_at": "2026-11-08T00:01:00Z", "reason": "No supported headroom source"},
            },
            "checks": checks,
            "recovery": recovery,
            "verdict": verdict,
        }
        files = {
            "report.json": json.dumps(report, separators=(",", ":")),
            "events.jsonl": json.dumps({"session_id": report["session"]["id"], "sequence": 1, "timestamp": "2026-11-08T00:01:00Z", "event_type": "session.started"}) + "\n",
            "summary.txt": "CF-DVS test bundle\n",
        }
        for name, content in files.items():
            (root / name).write_text(content, encoding="utf-8")
        entries = []
        for name in sorted(files):
            data = (root / name).read_bytes()
            entries.append({"path": name, "size_bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()})
        (root / "manifest.json").write_text(json.dumps({"manifest_version": "1.0", "files": entries}), encoding="utf-8")

    def test_valid_bundle(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_bundle(root)
            self.assertEqual(validate_bundle(root), (True, []))

    def test_recovery_required_takes_precedence(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_bundle(root, verdict="RECOVERY_REQUIRED", recovery={"pending": True, "journal_clear": False, "rollback_verified": False}, checks=[{"id": "daemon.running", "required": True, "status": "fail"}])
            self.assertEqual(validate_bundle(root), (True, []))

    def test_required_missing_evidence_is_inconclusive(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_bundle(root, verdict="INCONCLUSIVE", checks=[{"id": "thermal.headroom", "required": True, "status": "inconclusive"}])
            self.assertEqual(validate_bundle(root), (True, []))

    def test_schema_11_validates_pinned_cddp_profile(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_bundle(root)
            report = json.loads((root / "report.json").read_text())
            report["schema_version"] = "1.1"
            report["device_profile"] = {
                "generation": 7, "fingerprint": "0123456789abcdef", "captured_monotonic_ns": 123456,
                "kernel_release": "6.1-test", "android_release": "", "android_api_level": None,
                "device_model": "Synthetic Device", "soc_model": "Synthetic SoC", "hardware_platform": "",
                "cpu_topology": {"valid": True, "possible": [0, 1, 2, 3], "present": [0, 1, 2, 3], "online": [0, 1, 2, 3]},
                "cpu_policies": [{
                    "path": "/sys/devices/system/cpu/cpufreq/policy0", "related_cpus": [0, 1, 2, 3],
                    "frequency_table_khz": [300000, 600000, 1200000, 1800000],
                    "cpuinfo_min_freq_khz": 300000, "cpuinfo_max_freq_khz": 1800000,
                    "scaling_min_freq_khz": 300000, "scaling_max_freq_khz": 1800000,
                    "scaling_cur_freq_khz": 1200000, "governor": "schedutil",
                    "related_cpus_valid": True, "frequency_table_valid": True
                }],
                "gpu_domains": [], "thermal_sensors": [],
                "capabilities": [{
                    "path": "/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq",
                    "domain": "cpu", "source": "sysfs", "state": "MUTATION_ELIGIBLE",
                    "semantic_id": "cpufreq.scaling_max_freq", "unit": "kHz",
                    "current_value": "1800000", "accepted_values": "",
                    "constraints": "verified bounds and table", "evidence": "synthetic",
                    "rejection_reason": ""
                }],
                "memory_telemetry_available": True, "cgroup_v2_available": False,
                "uclamp_interface_available": False
            }
            (root / "report.json").write_text(json.dumps(report, separators=(",", ":")), encoding="utf-8")
            files = {name: (root / name).read_bytes() for name in ("report.json", "events.jsonl", "summary.txt")}
            entries = [
                {"path": name, "size_bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                for name, data in sorted(files.items())
            ]
            (root / "manifest.json").write_text(json.dumps({"manifest_version": "1.0", "files": entries}), encoding="utf-8")
            self.assertEqual(validate_bundle(root), (True, []))

    def test_schema_11_requires_device_profile(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_bundle(root)
            report = json.loads((root / "report.json").read_text())
            report["schema_version"] = "1.1"
            (root / "report.json").write_text(json.dumps(report), encoding="utf-8")
            files = {name: (root / name).read_bytes() for name in ("report.json", "events.jsonl", "summary.txt")}
            entries = [
                {"path": name, "size_bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                for name, data in sorted(files.items())
            ]
            (root / "manifest.json").write_text(json.dumps({"manifest_version": "1.0", "files": entries}), encoding="utf-8")
            valid, errors = validate_bundle(root)
            self.assertFalse(valid)
            self.assertTrue(any("requires device_profile" in error for error in errors))

    def test_active_mutation_is_not_misclassified_as_recovery_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_bundle(
                root, verdict="INCONCLUSIVE",
                recovery={"pending": False, "journal_clear": False, "active_mutations": True, "rollback_verified": False},
                checks=[{"id": "device.acceptance", "required": True, "status": "inconclusive"}],
            )
            self.assertEqual(validate_bundle(root), (True, []))

    def test_detached_nonempty_journal_requires_recovery(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_bundle(
                root, verdict="RECOVERY_REQUIRED",
                recovery={"pending": False, "journal_clear": False, "active_mutations": False, "rollback_verified": False},
                checks=[{"id": "recovery.state", "required": True, "status": "inconclusive"}],
            )
            self.assertEqual(validate_bundle(root), (True, []))

    def test_checksum_mismatch_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_bundle(root)
            (root / "summary.txt").write_text("XF-DVS test bundle\n", encoding="utf-8")
            valid, errors = validate_bundle(root)
            self.assertFalse(valid)
            self.assertTrue(any("SHA-256 mismatch" in e for e in errors))

    def test_path_traversal_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_bundle(root)
            manifest = json.loads((root / "manifest.json").read_text())
            manifest["files"][0]["path"] = "../outside.txt"
            (root / "manifest.json").write_text(json.dumps(manifest))
            valid, errors = validate_bundle(root)
            self.assertFalse(valid)
            self.assertTrue(any("unsafe manifest path" in e for e in errors))

    def test_malformed_untrusted_report_is_rejected_without_crashing(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_bundle(root)
            report = json.loads((root / "report.json").read_text())
            report["profile"] = ["unexpected", "array"]
            report["metrics"]["cpu_psi_some_avg10"]["validity"] = {"unexpected": "object"}
            (root / "report.json").write_text(json.dumps(report), encoding="utf-8")
            valid, errors = validate_bundle(root)
            self.assertFalse(valid)
            self.assertTrue(errors)

    def test_verdict_cannot_claim_pass_with_missing_required_check(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_bundle(root, verdict="PASS", checks=[{"id": "thermal.headroom", "required": True, "status": "inconclusive"}])
            valid, errors = validate_bundle(root)
            self.assertFalse(valid)
            self.assertTrue(any("verdict mismatch" in e for e in errors))


if __name__ == "__main__":
    unittest.main()
