"""Synthetic F scale parser/lifecycle tests: no CMake, CTest, engine or Git."""
from __future__ import annotations

import contextlib
import copy
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

import runner as S

R = S.R
COMMON, INCIDENCE = R.helpers()
FIXTURE_PATH = S.ROOT / "morsehgp3D_v7/tests/incidence_campaign_gate.py"
R.require(R.digest(FIXTURE_PATH) == "d74bca5257bd307b5956a946ca188a217f519ec2dc49e6b0de3ebf33a0ccca0a",
          "synthetic incidence fixture pin changed")
with patch.dict(sys.modules, {"runner": R}):
    F = S.load_pinned(S.ROOT / "build/v7_meb_paired/selftest.py",
        "f192a95f013e0b1df56a9324d8f18034949cd51410fcc6d41e8ee87af9048348", "scale_synthetic_fixture")


def text(n: int) -> str:
    # Synthetic fixture construction only, never a transformation of engine raw.
    value = F.complete_text()
    replacements = {"n=8000 coord=": f"n={n} coord=", "facettes=8063 ": f"facettes={n + 63} ",
                    "cardinalites K=1 evenements=10 facettes=8000 ": f"cardinalites K=1 evenements=10 facettes={n} "}
    for before, after in replacements.items():
        R.require(value.count(before) == 1, "unique synthetic fixture anchor changed")
        value = value.replace(before, after)
    return value


def parsed(n: int, mode: str = "completed", raw: str | None = None, code: int | None = None) -> dict:
    with tempfile.TemporaryDirectory(prefix="mhgp7-scale-parser-") as temporary:
        out, err, usage = (Path(temporary) / name for name in ("out", "err", "time"))
        out.write_text((text(n) if raw is None else raw) if mode == "completed" else "partial" if mode == "censored" else "")
        err.write_text(F.FIXTURE.silent_refusal() if mode == "refused" else "")
        usage.write_text(F.FIXTURE.USAGE)
        record = {"role": "F", "n": n, "status": "invalid", "wall_seconds": 1.0,
                  "returncode": code if code is not None else 0 if mode == "completed" else 2 if mode == "refused" else -9,
                  "timed_out": mode == "censored"}
        S.classify(record, out, err, usage, n, COMMON, INCIDENCE)
        return record


class ScaleGate(unittest.TestCase):
    def test_inert_preview_and_exact_argv_substitution(self):
        for n in S.SIZES:
            with patch.object(S, "execute", side_effect=RuntimeError("no execution")), \
                    patch.object(R, "helpers", side_effect=RuntimeError("preview must not load helpers")), \
                    contextlib.redirect_stdout(io.StringIO()) as stdout:
                self.assertEqual(S.main(["--n", str(n)]), 0)
            value = json.loads(stdout.getvalue())
            self.assertEqual(value["status"], "prepared_not_executed")
            self.assertFalse(value["writes"])
            self.assertEqual(value["engine_runs"], 0)
            self.assertEqual(value["classification_source_adaptations"], 0)
            self.assertEqual((value["timeout_seconds"], value["rlimit_as_gib"], value["cpu"]), (600, 26, 6))
            expected = [f"--n={n}" if arg == "--n=8000" else arg for arg in R.command(S.CANDIDATE)]
            self.assertEqual(value["command"], expected)
            for arg in ("--s=8", "--smax=11", "--threads=1", "--fold-inflight=1", "--fold-join=1",
                        "--complete-incidences", "--digest", "--coord=65536", "--seed=3", "--layout=csr"):
                self.assertEqual(expected.count(arg), 1)
        for n in (0, 8000, 16001, 50000):
            with self.assertRaisesRegex(RuntimeError, "unreviewed"):
                S.command(n)
        with patch.object(R, "command", return_value=["--n=8000", "--n=8000"]), self.assertRaisesRegex(RuntimeError, "unique"):
            S.command(16000)

    def test_generic_n_parser_and_full_digest_inventory(self):
        for n in S.SIZES:
            result = parsed(n)
            self.assertEqual(result["status"], "engine_completed")
            self.assertEqual(result["counts"]["n"], n)
            self.assertEqual(result["cardinalities"][1]["facettes"], n)
            self.assertEqual(set(result["digests"]), {"digest_all"} | {f"digest_forest_K{k}" for k in range(1, 11)})
            self.assertEqual(set(result["silent"]), set(range(2, 11)))
            self.assertEqual(result["silent_limits"], R.CAPS)
            self.assertEqual(result["max_rss_kb"], 1000)
            self.assertTrue(result["serialized_stage_flags_verified"])
            other = 32000 if n == 16000 else 16000
            for raw in (text(other), text(n).replace(f"n={n} coord=", f"n={n}.0 coord="),
                        text(n).replace(" s=8 ", " s=10 "), text(n).replace("threads=1", "threads=2"),
                        text(n).replace("coord=65536", "coord=4096"), text(n).replace("ordres_publies=10", "ordres_publies=9"),
                        text(n).replace(f"facettes={n + 63} ", f"facettes={n + 64} ")):
                with self.subTest(n=n, raw=raw[:30]), self.assertRaises(RuntimeError):
                    parsed(n, raw=raw)

    def test_mono_caps_and_late_completion_rejections(self):
        for before, after in (("fold_inflight=1", "fold_inflight=2"), ("fold_join=1", "fold_join=2"),
                              ("ouvriers wspd=1", "ouvriers wspd=2"), ("core_records=8000000", "core_records=8000001"),
                              ("archive_committed=false", "archive_committed=true"),
                              ("silent_incidence_ms=0.4", "silent_incidence_ms=nan")):
            with self.subTest(before=before), self.assertRaises(RuntimeError):
                parsed(16000, raw=text(16000).replace(before, after))
        with self.assertRaises(RuntimeError):
            parsed(16000, raw=text(16000) + "digest_all=" + "0" * 64 + "\n")
        with self.assertRaises(ValueError):
            parsed(16000, code=139)

    def test_refusals_and_censures_never_publish_completion(self):
        for n in S.SIZES:
            for mode in ("refused", "censored"):
                result = parsed(n, mode)
                self.assertEqual(result["status"], "engine_refused" if mode == "refused" else "censored")
                self.assertNotIn("digests", result)
                self.assertNotIn("pipeline_ms", result)
                self.assertNotIn("max_rss_kb", result)
                verdict = S.decision([result], n, True, None, INCIDENCE)
                self.assertEqual(verdict["status"], "observations_completed")
                self.assertEqual(verdict["engine_successes"], 0)
                self.assertEqual(verdict["comparison"], "none_F_only")
                self.assertNotIn("ratio_E_over_F", verdict)

    def test_no_false_green_identity_drift_or_missing_attempt(self):
        good = parsed(16000)
        self.assertEqual(S.decision([good], 16000, True, None, INCIDENCE)["status"], "observations_completed")
        bad_role, bad_n = copy.deepcopy(good), copy.deepcopy(good)
        bad_role["role"], bad_n["n"] = "E", 32000
        for records, stable, error in (([], True, None), ([good, good], True, None), ([bad_role], True, None),
                                      ([bad_n], True, None), ([good], False, None), ([good], True, "late failure")):
            self.assertEqual(S.decision(records, 16000, stable, error, INCIDENCE)["status"], "invalid")

    def test_fixed_F_pins_rejected_before_candidate_binding(self):
        for expected, damaged in ((S.CANDIDATE, "binary"), (S.RECEIPT, "receipt")):
            def digest(path):
                if path == expected:
                    return "0" * 64
                return S.CANDIDATE_SHA if path == S.CANDIDATE else S.RECEIPT_SHA
            with patch.object(R, "digest", side_effect=digest), patch.object(R, "candidate_binding") as binding, \
                    self.assertRaisesRegex(RuntimeError, "fixed F"):
                S.admission({"protected_binaries": {}}, COMMON)
            binding.assert_not_called()

    def test_loader_injection_refuses_before_any_attempt(self):
        for name in R.INJECTION_VARIABLES:
            clean = {key: "" for key in R.INJECTION_VARIABLES}
            clean[name] = "private fixture value"
            with patch.dict(os.environ, clean), patch.object(S.N, "baseline_binding") as baseline, \
                    patch.object(COMMON, "run_process") as process, self.assertRaises(RuntimeError) as error:
                S.execute(16000, COMMON, INCIDENCE)
            self.assertNotIn("private fixture value", str(error.exception))
            baseline.assert_not_called()
            process.assert_not_called()

    def test_one_attempt_lifecycle_closed_even_on_failure(self):
        for n in S.SIZES:
            for mode in ("completed", "refused", "censored", "interrupted", "malformed", "drift", "late_repository", "late_drift"):
                with self.subTest(n=n, mode=mode), tempfile.TemporaryDirectory(prefix="mhgp7-scale-lifecycle-") as temporary:
                    base = Path(temporary)
                    calls, snapshots, repository_calls, late = [], [0], [0], [False]
                    def current(*_args):
                        snapshots[0] += 1
                        return {"pin": "changed" if late[0] or (mode == "drift" and snapshots[0] >= 3) else "fixed"}
                    def repository():
                        repository_calls[0] += 1
                        if repository_calls[0] == 2:
                            if mode == "late_repository":
                                raise RuntimeError("late repository fixture")
                            late[0] = mode == "late_drift"
                        return {"head": "synthetic only", "worktree_porcelain": ""}
                    def run(argv, out, err, **kwargs):
                        calls.append(argv)
                        self.assertEqual(kwargs, {"timeout": 600, "address_limit_gib": 26})
                        self.assertIn(f"--n={n}", argv)
                        self.assertIn("--smax=11", argv)
                        self.assertIn("--complete-incidences", argv)
                        out.write_text("" if mode == "refused" else "bad" if mode == "malformed" else text(n))
                        err.write_text(F.FIXTURE.silent_refusal() if mode == "refused" else "")
                        out.with_suffix(".time").write_text(F.FIXTURE.USAGE)
                        if mode == "interrupted":
                            raise KeyboardInterrupt("synthetic interruption")
                        return (2 if mode == "refused" else -9 if mode == "censored" else 0, mode == "censored", 1.0)
                    with patch.object(S, "BASE", base), patch.object(S.N, "baseline_binding", return_value={}), \
                            patch.object(S, "admission", return_value={"binding": {}, "before": {"pin": "fixed"}}), \
                            patch.object(S, "snapshot", side_effect=current), patch.object(os, "sched_getaffinity", return_value={6}), \
                            patch.object(R, "repository_state", side_effect=repository), \
                            patch.object(R, "host_observation", return_value={"host_shared": True}), \
                            patch.object(COMMON, "run_process", side_effect=run), contextlib.redirect_stdout(io.StringIO()):
                        rc = S.execute(n, COMMON, INCIDENCE)
                        with self.assertRaises(FileExistsError):
                            S.execute(n, COMMON, INCIDENCE)
                    receipt = base / f"n{n}_receipts"
                    summary = json.loads((receipt / "summary.json").read_text())
                    records = json.loads((receipt / "runs.json").read_text())
                    hashes = json.loads((receipt / "hashes.json").read_text())
                    self.assertEqual(len(calls), 1)
                    self.assertEqual(len(records), 1)
                    self.assertIsNotNone(records[0]["ended_utc"])
                    self.assertEqual(rc, 0 if mode == "completed" else 2 if mode in ("refused", "censored") else 1)
                    self.assertEqual(summary["status"], "observations_completed" if mode in ("completed", "refused", "censored") else "invalid")
                    self.assertEqual(hashes["F.out"], COMMON.sha256(receipt / "F.out"))
                    self.assertEqual(hashes["F.err"], COMMON.sha256(receipt / "F.err"))
                    self.assertEqual(hashes["summary.json"], COMMON.sha256(receipt / "summary.json"))


if __name__ == "__main__":
    unittest.main()
