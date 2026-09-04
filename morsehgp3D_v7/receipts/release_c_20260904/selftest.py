#!/usr/bin/env python3
"""Lightweight runner-contract tests; never compile or start CTest."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("qualification_c", HERE / "run_qualification.py")
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("runner unavailable")
M = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M)


class ContractTests(unittest.TestCase):
    def test_default_is_inert(self):
        before_build = M.BUILD.exists()
        before_files = sorted(p.name for p in HERE.iterdir())
        result = subprocess.run([sys.executable, "-B", str(HERE / "run_qualification.py")],
                                capture_output=True, text=True, timeout=10, check=True)
        self.assertEqual(json.loads(result.stdout)["status"], "prepared_not_executed")
        self.assertEqual(M.BUILD.exists(), before_build)
        self.assertEqual(sorted(p.name for p in HERE.iterdir()), before_files)

    def test_campaign_terminal_and_mutants(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            binary = root / "candidate"
            binary.write_bytes(b"fake executable identity, never run")
            campaign = root / "campaign"
            campaign.mkdir()
            snapshot = {"source": "a" * 64}
            good = {"metadata.json": {"source_sha256": snapshot,
                    "binary_roles": {"candidate": {"path": str(binary), "wire_version": "v7"}},
                    "binaries": {str(binary): M.H.digest(binary)}, "serial_stages_requested": True},
                    "summary.json": {"status": "invalid", "source_stable": True, "runs": 1},
                    "runs.json": [{"status": "failed", "timed_out": True}]}
            def publish(values):
                for name, value in values.items():
                    (campaign / name).write_text(json.dumps(value))
                (campaign / "runner.py").write_text("fake runner, never run")
                hashes = {p.name: M.H.digest(p) for p in campaign.iterdir() if p.name != "hashes.json"}
                (campaign / "hashes.json").write_text(json.dumps(hashes))
            publish(good)
            self.assertEqual(M.validate_terminal_campaign(campaign, snapshot, binary)["status"], "invalid")
            for branch in ("unstable", "count", "source", "wire", "binary", "parallel", "running"):
                values = json.loads(json.dumps(good))
                if branch == "unstable": values["summary.json"]["source_stable"] = False
                if branch == "count": values["summary.json"]["runs"] = 2
                if branch == "source": values["metadata.json"]["source_sha256"] = {"other": "b" * 64}
                if branch == "wire": values["metadata.json"]["binary_roles"]["candidate"]["wire_version"] = "v6"
                if branch == "binary": values["metadata.json"]["binaries"][str(binary)] = "0" * 64
                if branch == "parallel": values["metadata.json"]["serial_stages_requested"] = False
                if branch == "running": values["summary.json"]["status"] = "running"
                publish(values)
                with self.subTest(branch=branch), self.assertRaises(RuntimeError):
                    M.validate_terminal_campaign(campaign, snapshot, binary)
            publish(good)
            (campaign / "runs.json").write_text("[]")
            with self.assertRaisesRegex(RuntimeError, "seal mismatch"):
                M.validate_terminal_campaign(campaign, snapshot, binary)

    def test_process_guard_and_snapshot_exclusion(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            proc = root / "proc"
            proc.mkdir()
            for pid, argv in ((101, [str(M.LIVE_CLI), "--n=8000"]),
                              (102, ["python3", "-B", str(M.BENCH)]),
                              (103, ["python3", str(HERE / "selftest.py")])):
                (proc / str(pid)).mkdir()
                (proc / str(pid) / "cmdline").write_bytes(b"\0".join(arg.encode() for arg in argv) + b"\0")
            self.assertEqual({entry["pid"] for entry in M.active_measurements(proc)}, {101, 102})
            for version in ("morsehgp3D_v6", "morsehgp3D_v7"):
                (root / version / "src").mkdir(parents=True)
                (root / version / "CMakeLists.txt").write_text("fixture")
                (root / version / "src/test.hpp").write_text("fixture")
            (root / "morsehgp3D_v7/bench").mkdir()
            (root / "morsehgp3D_v7/bench/compare_v6_v7.py").write_text("fixture")
            before = M.PAIR.source_snapshot(root)
            outside = root / "morsehgp3D_v7/receipts/release_c_20260904"
            outside.mkdir(parents=True)
            for name in ("run_qualification.py", "README.md", "receipt.json"):
                (outside / name).write_text("outside campaign scope")
            self.assertEqual(M.PAIR.source_snapshot(root), before)
            (root / "morsehgp3D_v7/src/test.hpp").write_text("actual source changed")
            self.assertNotEqual(M.PAIR.source_snapshot(root), before)


if __name__ == "__main__":
    unittest.main()
