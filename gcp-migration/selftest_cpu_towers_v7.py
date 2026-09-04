#!/usr/bin/env python3
"""Fake 50k CPU observations, real strict parsers; never execute a cloud command."""
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
ROOT = HERE.parent


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError("module unavailable")
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


C = module("g4_cpu_towers_test", HERE / "v7_g4_session.py")
F = module("paired_cpu_fixture", ROOT / "morsehgp3D_v7/tests/compare_campaign_gate.py")
P = C.compare_module(ROOT)
TOOL = C.private_cmake_module(ROOT)
USAGE = "\tMaximum resident set size (kbytes): 100\n\tExit status: 0\n"
REFUSAL = ("REFUS unsupported_degeneracy : rank-relevant extra-shell\n"
           "refus_etage=census rss_mb apres_generation=1 apres_rle=1 apres_prefiltre=1 apres_census=1 "
           "max_fold=0 (frontiere de completion : dernier etage atteint)\n")


def fixture(line, family, kmax, seed="fixture"):
    # Test data only. Adjust every affected cardinality total, not merely n.
    return F.fixture(f"v{line}", seed, kmax=kmax, threads=48).replace(
        "famille=uniform n=200", f"famille={family} n=50000").replace(
        f"facettes={198 + 2 * kmax} ", f"facettes={49998 + 2 * kmax} ").replace(
        f"noeuds={198 + 4 * kmax}\n", f"noeuds={49998 + 4 * kmax}\n").replace(
        "facettes=200 deltas", "facettes=50000 deltas").replace("noeuds=202\n", "noeuds=50002\n")


def write_case(out, line=7, family="uniform", kmax=5, *, status="completed", rc=0, elapsed=2.0,
               output=None, error="", gpu=True, remaining=2100):
    name = C.cpu_name(line, family, kmax)
    C.write_json(out / "identity.json", {"worker_root": str(out.parent)})
    C.write_json(out / "gpu_tools.json", {"requested": gpu})
    C.write_json(out / f"{name}_budget.json", C.diagnostic_plan(remaining, 120, 920 if gpu and kmax == 5 else 0))
    command = ["/usr/bin/time", "-v", "-o", str(out / f"{name}.usage"), *C.cpu_command(line, family, kmax)]
    record = {"argv": command, "exit_code": rc, "status": status, "elapsed_seconds": elapsed,
              "timeout_seconds": 120}
    C.write_json(out / f"{name}.json", record)
    (out / f"{name}.stdout").write_text(fixture(line, family, kmax) if output is None else output)
    (out / f"{name}.stderr").write_text(error)
    (out / f"{name}.usage").write_text(USAGE)
    return name, record


def fake_worker(root, scenario):
    base = time.time()
    clock, events = [base], []
    gpu = True

    def guard(_):
        events.append("guard")
        return base + 3600

    def logged(out, name, argv, timeout, **_kwargs):
        events.append(name)
        rc, elapsed, output, error = 0, 0.1, "fixture only\n", ""
        if name == "cpu_cmake_version":
            output = "cmake version 3.31.6\n"
        if name == "cpu_build":
            for line in (6, 7):
                path = root / f"build-v{line}/mhgp{line}"
                path.parent.mkdir()
                path.write_bytes(b"fake CPU binary never executed")
        if name.startswith("cpu_v") and name != "cpu_version":
            if events[-2] != "guard" or timeout != 120:
                raise RuntimeError("CPU run did not retain its guest guard and watchdog")
            line = int(name[5])
            family = "uniform" if "uniform" in name else "terrain"
            kmax = 5 if name.endswith("_k5") else 10
            output = fixture(line, family, kmax)
            if family == "uniform" and line == 7 and kmax == 10 and scenario != "fast":
                elapsed = 2.0
                if scenario == "censored":
                    rc, elapsed, output = 124, 120.1, "partial non-authoritative output"
                elif scenario == "refused":
                    rc, output, error = 2, "", REFUSAL
                elif scenario == "failed":
                    rc, output, error = 3, "", "fixture process failure\n"
                elif scenario == "invalid":
                    output = output.replace("digest_all=", "missing_digest_all=")
            if family == "uniform" and line == 7 and kmax == 5:
                if scenario == "k5_censored":
                    rc, elapsed, output = 124, 120.1, "partial"
                if scenario == "k5_diverged":
                    output = fixture(line, family, kmax, "different")
            (out / f"{name}.usage").write_text(USAGE)
            if name == "cpu_v7_terrain":
                if scenario == "k5_budget":
                    clock[0] = base + 1000  # 1100 left, less than paired 1200 reservation.
                if scenario == "candidate_budget":
                    clock[0] = base + 1100  # 1000 left: retain GPU, omit both candidates.
            if name == "cpu_v6_uniform_k5" and scenario == "second_leg_budget":
                clock[0] = base + 1100
        if name.startswith("candidate_"):
            if events[-2] != "guard":
                raise RuntimeError("candidate lacks a live guest guard")
            (out / f"{name}.usage").write_text(USAGE)
        if name == "gpu_build":
            for binary in ("mhgp7_device_witness", "mhgp7_census_device_gate"):
                path = root / "build-v7-cuda" / binary
                path.parent.mkdir(exist_ok=True)
                path.write_bytes(b"fake GPU binary never executed")
        clock[0] += elapsed
        (out / f"{name}.stdout").write_text(output)
        (out / f"{name}.stderr").write_text(error)
        C.write_json(out / f"{name}.json", {"argv": argv, "exit_code": rc,
            "status": "censored" if rc == 124 else "failed" if rc else "completed",
            "timeout_seconds": timeout, "elapsed_seconds": elapsed})
        return rc

    with ExitStack() as stack:
        stack.enter_context(mock.patch.object(C, "verify_source"))
        stack.enter_context(mock.patch.object(C, "guest_deadline", side_effect=guard))
        stack.enter_context(mock.patch.object(C, "time", SimpleNamespace(time=lambda: clock[0])))
        stack.enter_context(mock.patch.object(C, "run_logged", side_effect=logged))
        stack.enter_context(mock.patch.object(C, "cpu_toolchain_receipt", return_value={"fake_toolchain": True}))
        stack.enter_context(mock.patch.object(C, "compare_module", return_value=P))
        stack.enter_context(mock.patch.object(C, "candidate_observation", return_value={"status": "engine_refused", "fixture": True}))
        stack.enter_context(mock.patch.object(C, "private_cmake_module", return_value=TOOL))
        stack.enter_context(mock.patch.object(C.shutil, "which", side_effect=lambda name: "/fake/" + name))
        stack.enter_context(redirect_stdout(io.StringIO()))
        rc = C.worker(root, "0" * 64, gpu)
        C.validate_receipt(root / "out", ROOT, "0" * 64, rc)
    return rc, events


class CpuTowerTests(unittest.TestCase):
    def test_real_parsers_and_no_objects_on_refusal_or_censure(self):
        for line in (6, 7):
            for kmax in (5, 10):
                with self.subTest(line=line, kmax=kmax), tempfile.TemporaryDirectory() as temp:
                    out = Path(temp) / "out"
                    out.mkdir()
                    name, record = write_case(out, line=line, kmax=kmax)
                    parsed = C.cpu_observation(out, ROOT, line, "uniform", kmax)
                    self.assertEqual(parsed["status"], "engine_completed")
                    self.assertEqual(len(parsed["digests"]), kmax + 1)
                    self.assertEqual(len(parsed["cardinalities"]), kmax)
                    valid = (out / f"{name}.stdout").read_text()
                    for bad in (valid.replace("n=50000", "n=49999"), valid.replace("digest_all=", "missing_digest_all="),
                                valid + "digest_forest_K11=" + "f" * 64 + "\n",
                                valid.replace("K=1 evenements", "K=2 evenements"),
                                fixture(line, "uniform", 10 if kmax == 5 else 5)):
                        (out / f"{name}.stdout").write_text(bad)
                        invalid = C.cpu_observation(out, ROOT, line, "uniform", kmax)
                        self.assertEqual(invalid["status"], "invalid")
                        self.assertNotIn("digests", invalid)
                    for status, rc, elapsed, output, error, expected in (
                        ("censored", 124, 120.1, valid, "", "censored"),
                        ("failed", 2, 0.1, "", REFUSAL, "engine_refused"),
                        ("failed", 2, 0.1, "", REFUSAL.replace("unsupported_degeneracy", "resource_exhausted"), "engine_refused"),
                        ("failed", 2, 0.1, valid, REFUSAL, "invalid"),
                        ("failed", 3, 0.1, "", "fixture\n", "failed")):
                        write_case(out, line=line, kmax=kmax, status=status, rc=rc, elapsed=elapsed, output=output, error=error)
                        observed = C.cpu_observation(out, ROOT, line, "uniform", kmax)
                        self.assertEqual(observed["status"], expected)
                        self.assertNotIn("digests", observed)
                        self.assertNotIn("equal", C.cpu_pair(parsed, observed))

    def test_threshold_and_reservation_boundaries(self):
        for seconds, requested in ((1.0, False), (1.000001, True)):
            state = {"status": "engine_completed", "process_wall_seconds": seconds}
            self.assertEqual(C.fallback_plan(state, 1200, True)["requested"], requested)
        for status in ("censored", "engine_refused"):
            self.assertEqual(C.fallback_plan({"status": status}, 1200, True)["status"], "planned")
            self.assertEqual(C.fallback_plan({"status": status}, 1199.9, True)["reason"], "budget_insufficient")
            self.assertEqual(C.fallback_plan({"status": status}, 280, False)["status"], "planned")
        for limit in (120, 240):
            self.assertEqual(C.diagnostic_plan(limit + 940, limit, 920)["status"], "planned")
            self.assertEqual(C.diagnostic_plan(limit + 939.9, limit, 920)["status"], "not_attempted")

    def test_raw_command_censure_and_budget_mutants(self):
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / "out"
            out.mkdir()
            name, original = write_case(out)
            for mutation in ("n", "order", "wrapper", "usage", "timeout", "short_censure", "not_attempted"):
                record = copy.deepcopy(original)
                if mutation == "n":
                    record["argv"][6] = "--n=49999"
                elif mutation == "order":
                    record["argv"][9] = "--smax=11"
                elif mutation == "wrapper":
                    record["argv"][0] = "/fake/time"
                elif mutation == "usage":
                    record["argv"][3] = "/other/case.usage"
                elif mutation == "timeout":
                    record["timeout_seconds"] = 121
                elif mutation == "short_censure":
                    record.update(status="censored", exit_code=124, elapsed_seconds=1.0)
                else:
                    record.update(status="not_attempted", exit_code=None, timeout_seconds=0,
                                  elapsed_seconds=0, reason="budget_insufficient")
                C.write_json(out / f"{name}.json", record)
                with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                    C.cpu_observation(out, ROOT, 7, "uniform", 5)
            C.write_json(out / f"{name}.json", original)
            budget_path = out / f"{name}_budget.json"
            budget = json.loads(budget_path.read_text())
            for key, value in (("reserved_seconds", 0), ("remaining_seconds", 100),
                               ("drain_margin_seconds", 0), ("run_limit_seconds", 121)):
                C.write_json(budget_path, dict(budget, **{key: value}))
                with self.subTest(key=key), self.assertRaises(ValueError):
                    C.cpu_observation(out, ROOT, 7, "uniform", 5)

    def test_worker_branches_and_receiver(self):
        scenarios = ("fast", "slow", "censored", "refused", "failed", "invalid", "k5_budget",
                     "candidate_budget", "k5_censored", "k5_diverged", "second_leg_budget")
        for scenario in scenarios:
            with self.subTest(scenario=scenario), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                rc, events = fake_worker(root, scenario)
                out = root / "out"
                self.assertEqual(rc, 0 if scenario in ("fast", "slow") else 1)
                self.assertIn("gpu_primitives", events)
                summary = json.loads((out / "cpu_campaign.json").read_text())
                if scenario in ("slow", "censored", "refused", "k5_censored", "k5_diverged", "second_leg_budget"):
                    self.assertIn("cpu_v6_uniform_k5", events)
                    if scenario != "second_leg_budget":
                        self.assertIn("cpu_v7_uniform_k5", events)
                else:
                    self.assertNotIn("cpu_v6_uniform_k5", events)
                if scenario in ("censored", "refused", "failed", "invalid"):
                    self.assertNotIn("equal", summary["pairs"]["uniform_k10"])
                if scenario in ("k5_censored", "second_leg_budget"):
                    self.assertNotIn("equal", summary["pairs"]["uniform_k5"])
                if scenario == "candidate_budget":
                    self.assertNotIn("candidate_uniform", events)
                    self.assertNotIn("candidate_wide", events)
                terminal = json.loads((out / "worker_terminal.json").read_text())
                self.assertTrue(terminal["diagnostics_completed"])
                # A nonzero CPU diagnostic still revalidates independent GPU evidence.
                with mock.patch.object(C, "cpu_toolchain_receipt", return_value={"fake_toolchain": True}), \
                        mock.patch.object(C, "candidate_observation", return_value={"status": "engine_refused", "fixture": True}):
                    before = (out / "gpu_cmake_toolchain.json").read_bytes()
                    selection = json.loads(before)
                    selection["ctest"] = "wrong-ctest"
                    C.write_json(out / "gpu_cmake_toolchain.json", selection)
                    with self.assertRaises(ValueError):
                        C.validate_receipt(out, ROOT, "0" * 64, rc)
                    (out / "gpu_cmake_toolchain.json").write_bytes(before)
                    pair = out / "pair_uniform.json"
                    pair_data = json.loads(pair.read_text())
                    pair_data["equal"] = True
                    pair_data["invented"] = True
                    C.write_json(pair, pair_data)
                    with self.assertRaises(ValueError):
                        C.validate_receipt(out, ROOT, "0" * 64, rc)

    def test_terminal_contradictions_and_fail_closed_except(self):
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp)
            for completed in (False, None):
                C.write_json(out / "worker_terminal.json", {"exit_code": 0, "public_status": "not_claimed",
                                                            "diagnostics_completed": completed})
                with self.assertRaises(ValueError):
                    C.validate_receipt(out, ROOT, "0" * 64, 0)
            C.write_json(out / "worker_terminal.json", {"exit_code": 0, "public_status": "not_claimed",
                                                        "diagnostics_completed": True})
            C.write_json(out / "failure.json", {"error": "fixture"})
            with self.assertRaises(ValueError):
                C.validate_receipt(out, ROOT, "0" * 64, 0)


if __name__ == "__main__":
    unittest.main()
