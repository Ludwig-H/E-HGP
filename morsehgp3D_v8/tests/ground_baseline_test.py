#!/usr/bin/env python3
"""Bounded receipt tests: fake native outputs, no LiDAR benchmark or build.

The process-group cancellation implementation is explicitly reused from
run_p0_matrix; this suite tests the caller's persisted failure, not a second
implementation of that collector or a geometric qualification.
"""
from __future__ import annotations

import argparse
import base64
from contextlib import ExitStack, redirect_stdout
import copy
import io
import json
from pathlib import Path
import signal
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

BENCH = Path(__file__).resolve().parents[1] / "bench"
sys.path.insert(0, str(BENCH))
import run_ground_baseline as subject

TIME = """User time (seconds): 0.01
System time (seconds): 0.02
Percent of CPU this job got: 30%
Elapsed (wall clock) time (h:mm:ss or m:ss): 0:00.10
Maximum resident set size (kbytes): 1234
Exit status: 0
"""


class BaselineTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="mhgp8_ground_receipt_test_")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.stack = ExitStack()
        self.addCleanup(self.stack.close)
        self.stack.enter_context(patch.object(subject, "ROOT", self.root))
        self.manifest = self.root / "INPUT.json"
        self.manifest.write_text('{}\n')
        self.stack.enter_context(patch.object(subject, "MANIFEST", self.manifest))
        self.source = self.root / "source.hpp"
        self.source.write_text("// test fixture, no native execution\n")
        self.probe = self.root / "probe"
        self.probe.write_bytes(b"fixture never executed")
        self.inputs = {}
        for scene in ("00", "01", "02"):
            path = self.root / f"input_{scene}.u16le"
            path.write_bytes(struct.pack("<12H", 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2))
            self.inputs[scene] = dict(path=path.name, n=4, sha256=subject.sha256(path), frame=f"frame_{scene}")
        self.stack.enter_context(patch.object(subject, "inputs_2cm", side_effect=lambda: (copy.deepcopy(self.inputs), subject.sha256(self.manifest))))
        self.stack.enter_context(patch.object(subject, "PIPELINE_SOURCES", ()))
        self.stack.enter_context(patch.object(subject, "source_paths", return_value=[str(self.source)]))
        self.stack.enter_context(patch.object(subject, "git", side_effect=lambda *args: "a" * 40 if args[0] == "rev-parse" else ""))
        self.stack.enter_context(patch.object(subject.collector, "invoke", side_effect=self.invoke))
        self.counter = 0
        self.mutation = None

    def invoke(self, command, environment, root, record, **kwargs):
        self.assertTrue(kwargs["new_session"])
        self.assertEqual(environment["LC_ALL"], "C")
        native = command[4:]
        entry = next(e for e in self.inputs.values() if str(self.root / e["path"]) == native[1])
        fingerprint, profile = subject.input_fingerprint(entry, "2cm")
        result = dict(schema="mhgp8_wspd_q34_probe_v5", status="completed",
                      scope="global_q3_q4_candidate_stream_not_catalogue_or_full", backend="cpu_reference",
                      profile=profile, public_status="not_claimed", n=4, source_n=4, kmax=int(native[3]), s=8,
                      mask=6, q4_backend=28, workers=int(native[7]), front_mode="samples", output_mode="digest",
                      witness_mode="rectangle-pair", q3_census_mode="boxes", witness_bounds_mode="affine",
                      q4_seed_mode="live", q4_seed_block_size=64, q3_atlas_mode="atlas", input_hash=fingerprint,
                      output=dict(callbacks=2, q3=1, q4=1, support_ids=7, shell_ids=7, xor="a", sum="b"),
                      front=dict(product_visits=5), work=dict(q3_emitted=1, q4_emitted=1, terminal_pairs=9, peak_buffer_bytes=32),
                      parallel={}, workers_work=[], cloud_work={}, index_work={}, memory={}, timings_ms=dict(total=10.0))
        if self.mutation:
            self.mutation(result)
        raw = json.dumps(result) + "\n"
        record.update(exit_code=0, stdout=raw, stdout_base64=base64.b64encode(raw.encode()).decode(), stderr="", stderr_base64="")
        Path(command[3]).write_text(TIME)

    def capture(self, *, only=None, reference=None):
        self.counter += 1
        output = self.root / f"capture_{self.counter}"
        args = argparse.Namespace(probe=self.probe, output=output, grid="2cm", extra=["atlas"], only=only, reference=reference or [])
        with redirect_stdout(io.StringIO()):
            subject.run(args)
        return output

    def reclose(self, path, receipt):
        """Forge enclosing hashes to test semantics rather than just hashing."""
        filename = "BASELINE.only.json" if (path / "BASELINE.only.json").exists() else "BASELINE.json"
        subject.write(path / filename, receipt)
        manifest = {k: v for k, v in receipt.items() if k not in ("rows", "finished_utc")}
        manifest["status"] = "running"
        subject.write(path / "MANIFEST.json", manifest)
        completion = subject.load(path / "COMPLETION.json")
        completion["receipt_sha256"] = subject.sha256(path / filename)
        completion["manifest_sha256"] = subject.sha256(path / "MANIFEST.json")
        subject.write(path / "COMPLETION.json", completion)

    def test_closed_roundtrip_and_live(self):
        path = self.capture()
        receipt, pins = subject.read_receipt(path, check_live=True)
        self.assertEqual(receipt["schema"], subject.SCHEMA_CLOSED)
        self.assertEqual(len(receipt["rows"]), 9)
        self.assertGreater(len(pins), 27)

    def test_reference_is_reopened_and_same_inputs(self):
        reference = self.capture()
        path = self.capture(only=[("00", 5, 8)], reference=[str(reference)])
        receipt, _ = subject.read_receipt(path, "only", check_live=True)
        self.assertEqual(subject.check(receipt)["reference_identity_pairs"], 2)
        row = subject.load(reference / "BASELINE.json")["rows"][0]
        (reference / row["probe_json"]).write_text("{}\n")
        with self.assertRaises(subject.Failure):
            subject.read_receipt(path, "only")

    def test_reference_input_mismatch_before_output(self):
        reference = self.capture()
        self.inputs["00"]["frame"] = "another_frame"
        with self.assertRaisesRegex(subject.Failure, "autres points/IDs"):
            self.capture(only=[("00", 5, 8)], reference=[str(reference)])
        self.assertFalse((self.root / "capture_2").exists())

    def test_embedded_reference_not_self_authority(self):
        reference = self.capture()
        path = self.capture(only=[("00", 5, 8)], reference=[str(reference)])
        receipt = subject.load(path / "BASELINE.only.json")
        receipt["reference"]["rows"][0]["logical_sha256"] = "0" * 64
        self.reclose(path, receipt)
        with self.assertRaisesRegex(subject.Failure, "référence embarquée"):
            subject.read_receipt(path, "only")

    def test_native_errors_preserve_stdout_stderr_and_failed_completion(self):
        def fail(command, env, root, record, **kwargs):
            record.update(exit_code=7, stdout="partial", stdout_base64="cGFydGlhbA==", stderr="failed", stderr_base64="ZmFpbGVk")
        with patch.object(subject.collector, "invoke", side_effect=fail), self.assertRaises(subject.Failure):
            self.capture()
        path = self.root / "capture_1"
        self.assertEqual(subject.load(path / "COMPLETION.json")["status"], "failed")
        self.assertEqual(subject.load(path / "record_00.json")["stderr"], "failed")
        with self.assertRaises(subject.Failure):
            subject.read_receipt(path)

    def test_interrupted_record_is_persisted(self):
        def interrupt(command, env, root, record, **kwargs):
            record.update(exit_code=-15, stdout="partial", stdout_base64="cGFydGlhbA==")
            raise subject.collector.CampaignInterrupted(signal.SIGTERM)
        with patch.object(subject.collector, "invoke", side_effect=interrupt), self.assertRaises(subject.Failure):
            self.capture()
        self.assertIn("CampaignInterrupted", subject.load(self.root / "capture_1/COMPLETION.json")["error"])

    def test_source_change_or_disappearance_closes_failed(self):
        for remove in (False, True):
            with self.subTest(remove=remove):
                self.source.write_text("initial")
                def change(result):
                    if remove:
                        self.source.unlink(missing_ok=True)
                    else:
                        self.source.write_text("changed")
                self.mutation = change
                with self.assertRaises(subject.Failure):
                    self.capture()
                completion = subject.load(self.root / f"capture_{self.counter}/COMPLETION.json")
                self.assertEqual(completion["status"], "failed")
                self.assertTrue(completion["closing_errors"])

    def test_invalid_native_fields_fail(self):
        for field, value in (("n", 3), ("source_n", 3), ("workers", True), ("input_hash", 0), ("q3_atlas_mode", "no-atlas")):
            with self.subTest(field=field):
                self.mutation = lambda result: result.__setitem__(field, value)
                with self.assertRaises(subject.Failure):
                    self.capture()

    def test_boolean_work_counter_and_nan_fail(self):
        for field in ("work", "timings_ms"):
            with self.subTest(field=field):
                self.mutation = lambda result: result[field].update(corruption=True if field == "work" else float("nan"))
                with self.assertRaises(subject.Failure):
                    self.capture()

    def test_partial_has_no_invented_identity(self):
        path = self.capture(only=[("00", 5, 8)])
        receipt, _ = subject.read_receipt(path, "only")
        self.assertEqual(receipt["status"], "partial")
        self.assertEqual(subject.check(receipt), dict(worker_identity_pairs=0, reference_identity_pairs=0))

    def test_existing_output_not_replaced(self):
        path = self.capture()
        pin = subject.sha256(path / "BASELINE.json")
        self.counter -= 1
        with self.assertRaisesRegex(subject.Failure, "jamais recouvert"):
            self.capture()
        self.assertEqual(subject.sha256(path / "BASELINE.json"), pin)

    def test_invalid_only_rejected_before_output(self):
        for selection in ([('00', 5, 8), ('00', 5, 8)], [('00', 6, 8)]):
            with self.subTest(selection=selection), self.assertRaises(subject.Failure):
                self.capture(only=selection)

    def test_raw_time_and_raw_output_and_record_are_bound(self):
        for field in ("time_file", "probe_json", "record"):
            with self.subTest(field=field):
                path = self.capture()
                row = subject.load(path / "BASELINE.json")["rows"][0]
                with (path / row[field]).open("a") as stream:
                    stream.write(" ")
                with self.assertRaises(subject.Failure):
                    subject.read_receipt(path)

    def test_forged_receipt_still_checks_command(self):
        path = self.capture()
        receipt = subject.load(path / "BASELINE.json")
        receipt["rows"][0]["command"][3] = "10"
        self.reclose(path, receipt)
        with self.assertRaisesRegex(subject.Failure, "commande différente"):
            subject.read_receipt(path)

    def test_forged_receipt_still_checks_row_and_dates(self):
        for field, value in (("q4_emitted", 2), ("finished_utc", "2000-01-01T00:00:00Z"), ("probe_json", "../outside.json")):
            with self.subTest(field=field):
                path = self.capture()
                receipt = subject.load(path / "BASELINE.json")
                receipt["rows"][0][field] = value
                self.reclose(path, receipt)
                with self.assertRaises(subject.Failure):
                    subject.read_receipt(path)

    def test_forged_closing_inventory_is_rejected(self):
        path = self.capture()
        completion = subject.load(path / "COMPLETION.json")
        completion["artifact_sha256"].pop(next(iter(completion["artifact_sha256"])))
        subject.write(path / "COMPLETION.json", completion)
        with self.assertRaisesRegex(subject.Failure, "inventaire"):
            subject.read_receipt(path)

    def test_live_input_changed_and_historical_not_upgraded(self):
        path = self.capture()
        input_path = self.root / self.inputs["00"]["path"]
        input_path.write_bytes(bytes(24))
        subject.read_receipt(path)
        with self.assertRaises(subject.Failure):
            subject.read_receipt(path, check_live=True)

    def test_fingerprint_format_independent_and_coordinate_range(self):
        entry = self.inputs["00"]
        first, profile = subject.input_fingerprint(entry, "2cm")
        path = self.root / "same.u32le"
        coords = struct.unpack("<12H", (self.root / entry["path"]).read_bytes())
        path.write_bytes(struct.pack("<12I", *coords))
        second, profile2 = subject.input_fingerprint(dict(path=path.name, n=4, sha256=subject.sha256(path)), "1mm")
        self.assertEqual((first, profile), (second, profile2))
        path.write_bytes(struct.pack("<12I", 262144, *coords[1:]))
        with self.assertRaisesRegex(subject.Failure, "hors 18 bits"):
            subject.input_fingerprint(dict(path=path.name, n=4, sha256=subject.sha256(path)), "1mm")

    def test_gnu_time_and_json_strict(self):
        self.assertEqual(subject.gnu_time_fields(TIME)["wall_s"], .1)
        for text in (TIME.replace("Exit status: 0", "Exit status: 7"), TIME + "User time (seconds): 0.01\n", TIME.replace("0:00.10", "nan")):
            with self.subTest(text=text), self.assertRaises((subject.Failure, ValueError)):
                subject.gnu_time_fields(text)
        path = self.root / "bad.json"
        for text in ('{"a":1,"a":2}', '{"a":NaN}', '{"a":1e999}'):
            path.write_text(text)
            with self.assertRaises(ValueError):
                subject.load(path)

    def test_v2_retains_terminal_geometric_counts(self):
        work = dict(terminal_pairs=7, terminal_refinements=3, peak_bytes=99)
        self.assertEqual(subject.logical(work), {})
        self.assertEqual(subject.logical(work, version=2), dict(terminal_pairs=7, terminal_refinements=3))


if __name__ == "__main__":
    unittest.main()
