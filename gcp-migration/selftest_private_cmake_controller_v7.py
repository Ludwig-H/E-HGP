#!/usr/bin/env python3
"""Controller overlay branch tests: fake builds/GPU/network, real short watchdog."""
from __future__ import annotations

from contextlib import ExitStack, redirect_stdout
import copy
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import time
from types import SimpleNamespace
import unittest
from unittest import mock


HERE = Path(__file__).resolve().parent


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None: raise RuntimeError("module unavailable")
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


C = module("controller_cmake_overlay", HERE / "v7_g4_session.py")
M = module("private_cmake_controller_test", HERE / "private_cmake_v7.py")


def installation():
    return {"schema": "ehgp.v7.private-cmake.v1", "status": "completed", "public_status": "not_claimed",
            "version": M.VERSION, "url": M.WHEEL_URL, "source_json": M.SOURCE_JSON,
            "wheel_sha256": M.WHEEL_SHA256, "wheel_bytes": M.WHEEL_SIZE,
            "helper_sha256": M.digest(Path(M.__file__).read_bytes()), "elapsed_seconds": 0.1,
            "paths": M.BINARIES, "extraction": {"member_count": 3, "extracted_files": 3, "uncompressed_bytes": 100},
            "probes": {name: {"argv": [path, "--version"], "exit_code": 0, "stderr": "",
                               "stdout": f"{name} version {M.VERSION}\n", "binary_sha256": "0" * 64}
                       for name, path in M.BINARIES.items()}}


def run_fake_worker(root: Path, scenario: str):
    base = time.time()
    clock = [base]
    events = []
    gpu = scenario != "not_requested"
    def guard(_):
        events.append("guest_guard")
        return base + 3600
    def logged(out, name, argv, timeout, **_kwargs):
        if name == "gpu_cmake_install":
            if events[-1] != "guest_guard": raise RuntimeError("installation not preceded by live guest guard")
            if timeout != 120: raise RuntimeError("installer lacks the total process-group watchdog")
        events.append(name)
        stdout, stderr, rc = "fixture only\n", "", 0
        if name == "cpu_cmake_version": stdout = "cmake version " + ("3.31.6\n" if scenario == "modern" else "3.22.1\n")
        if name == "cpu_build":
            for line in (6, 7):
                path = root / f"build-v{line}/mhgp{line}"
                path.parent.mkdir()
                path.write_bytes(b"fake CPU binary, never executed")
        if name.startswith("cpu_v") or name.startswith("candidate_"):
            (out / (name + ".usage")).write_text("fixture only")
        if name == "candidate_uniform": clock[0] = base + 2100 - (850 if scenario == "budget" else 1100)
        if name == "gpu_cmake_install":
            receipt = installation()
            if scenario == "bad_receipt": receipt["helper_sha256"] = "f" * 64
            stdout = json.dumps(receipt)
            rc = 124 if scenario == "timeout" else 2 if scenario == "install_failed" else 0
            clock[0] += 1
        if name == "gpu_build":
            for binary in ("mhgp7_device_witness", "mhgp7_census_device_gate"):
                path = root / "build-v7-cuda" / binary
                path.parent.mkdir(exist_ok=True)
                path.write_bytes(b"fake GPU binary, never executed")
        (out / (name + ".stdout")).write_text(stdout)
        (out / (name + ".stderr")).write_text(stderr)
        C.write_json(out / (name + ".json"), {"argv": argv, "exit_code": rc,
                     "status": "censored" if rc == 124 else "failed" if rc else "completed",
                     "timeout_seconds": timeout, "elapsed_seconds": 0.1})
        return rc
    with ExitStack() as stack:
        stack.enter_context(mock.patch.object(C, "verify_source"))
        stack.enter_context(mock.patch.object(C, "guest_deadline", side_effect=guard))
        stack.enter_context(mock.patch.object(C, "time", SimpleNamespace(time=lambda: clock[0])))
        stack.enter_context(mock.patch.object(C, "run_logged", side_effect=logged))
        stack.enter_context(mock.patch.object(C, "cpu_toolchain_receipt", return_value={"fake_toolchain": True}))
        stack.enter_context(mock.patch.object(C, "compare_module", return_value=SimpleNamespace(
            parse_success=lambda *a, **k: {"digests": {"fake": "0" * 64}, "cardinalities": {"fake": 1}})))
        stack.enter_context(mock.patch.object(C, "candidate_observation", return_value={"status": "engine_refused", "fixture": True}))
        stack.enter_context(mock.patch.object(C, "private_cmake_module", return_value=M))
        stack.enter_context(mock.patch.object(C.shutil, "which", side_effect=lambda name: "/fake/" + name))
        stack.enter_context(redirect_stdout(io.StringIO()))
        rc = C.worker(root, "0" * 64, gpu)
    return rc, events


class ControllerTests(unittest.TestCase):
    def test_selection_branches_and_raw_receipt_mutants(self):
        for scenario in ("private", "modern", "budget", "timeout", "install_failed", "not_requested", "bad_receipt"):
            with self.subTest(scenario=scenario), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                rc, events = run_fake_worker(root, scenario)
                self.assertEqual(rc, 1 if scenario == "bad_receipt" else 0)
                if scenario == "bad_receipt":
                    self.assertNotIn("gpu_build", events)
                    continue
                out = root / "out"
                M.validate_selection(out)
                gpu = json.loads((out / "gpu_status.json").read_text())
                self.assertEqual(gpu["status"], "completed" if scenario in ("private", "modern") else
                                 "not_requested" if scenario == "not_requested" else "unavailable")
                self.assertEqual("gpu_cmake_install" in events, scenario in ("private", "timeout", "install_failed"))
                self.assertEqual("gpu_build" in events, scenario in ("private", "modern"))
                if scenario == "private":
                    cases = [("gpu_cmake_install.json", lambda d: d["argv"].__setitem__(8, "1")),
                             ("gpu_cmake_install.json", lambda d: d.__setitem__("timeout_seconds", 121)),
                             ("gpu_cmake_install.json", lambda d: d.__setitem__("exit_code", 124)),
                             ("gpu_cmake_toolchain.json", lambda d: d.__setitem__("cmake", "cmake")),
                             ("gpu_cmake_toolchain.json", lambda d: d.__setitem__("ctest", "ctest")),
                             ("gpu_primitives.json", lambda d: d["argv"].__setitem__(0, "ctest")),
                             ("gpu_build.json", lambda d: d["argv"].__setitem__(3, "cmake -S morsehgp3D_v7"))]
                    for name, mutate in cases:
                        path = out / name
                        original = path.read_text()
                        data = json.loads(original)
                        mutate(data)
                        path.write_text(json.dumps(data))
                        with self.assertRaises(ValueError): M.validate_selection(out)
                        path.write_text(original)
                if scenario == "budget":
                    gpu["status"] = "completed"
                    (out / "gpu_status.json").write_text(json.dumps(gpu))
                    with self.assertRaisesRegex(ValueError, "old system CMake"):
                        M.validate_selection(out)

    def test_total_watchdog_is_real(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory)
            began = time.monotonic()
            rc = C.run_logged(out, "gpu_cmake_install", [sys.executable, "-c", "import time; time.sleep(60)"], 1)
            self.assertEqual(rc, 124)
            self.assertLess(time.monotonic() - began, 4)
            self.assertEqual(json.loads((out / "gpu_cmake_install.json").read_text())["status"], "censored")


if __name__ == "__main__":
    unittest.main()
