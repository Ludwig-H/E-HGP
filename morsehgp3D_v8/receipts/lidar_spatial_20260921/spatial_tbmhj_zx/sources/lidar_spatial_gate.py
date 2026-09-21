#!/usr/bin/env python3
"""Independent fixtures for the sensor-frame spatial LiDAR preparation."""
from __future__ import annotations

from fractions import Fraction
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

PREPARER = Path(__file__).resolve().parents[1] / "bench/prepare_lidar_spatial.py"
SPEC = importlib.util.spec_from_file_location("lidar_spatial", PREPARER)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("cannot import spatial preparer")
spatial = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(spatial)
NAMES = (
    "full", "half_x_neg", "half_x_nonneg", "quarter_x_neg_y_neg",
    "quarter_x_neg_y_nonneg", "quarter_x_nonneg_y_neg",
    "quarter_x_nonneg_y_nonneg",
)


def packed(rows):
    return b"".join(struct.pack("<ffff", *row) for row in rows)


def coordinates(path):
    return list(struct.iter_unpack("<HHH", path.read_bytes()))


def ids(path):
    return [record[0] for record in struct.iter_unpack("<I", path.read_bytes())]


class SpatialPreparationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="mhgp8_lidar_spatial_")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.source = self.directory / "scan.bin"
        self.destination = self.directory / "prepared"

    def run_cli(self, *arguments, success=True):
        command = [sys.executable]
        if sys.flags.optimize:
            command.append("-O")
        command.extend((str(PREPARER), *map(str, arguments)))
        result = subprocess.run(command, text=True, capture_output=True, check=False)
        if success:
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        else:
            self.assertNotEqual(result.returncode, 0, result.stderr + result.stdout)
        return result

    def prepare_fixture(self, rows):
        self.source.write_bytes(packed(rows))
        self.run_cli("prepare", "--input", self.source, "--output", self.destination)

    def test_exact_rounding_against_fraction(self):
        cases = [-655.37, -655.36, -655.35, -0.03, -0.01, -0.005, -0.0,
                 0.0, 0.005, 0.01, 0.03, 655.33, 655.34, 655.35]
        # Include exact half-grid ties and both adjacent binary values.
        for tie in (-1.25, -0.25, 0.25, 1.25):
            cases.extend((math.nextafter(tie, -math.inf), tie,
                          math.nextafter(tie, math.inf)))
        for value in cases:
            rational = Fraction.from_float(value) * 50 + Fraction(65537, 2)
            expected = rational.numerator // rational.denominator
            with self.subTest(value=value):
                if 0 <= expected <= 65535:
                    self.assertEqual(spatial.quantize_coordinate(value), expected)
                else:
                    with self.assertRaises((ValueError, RuntimeError)):
                        spatial.quantize_coordinate(value)

    def test_disjoint_partitions_and_all_return_mappings(self):
        rows = [(-1, -1, 0, 0.1), (-1, 1, 0, 0.2), (1, -1, 0, 0.3),
                (1, 1, 0, 0.4), (-0.005, -0.005, 0, 0.5),
                (0.005, 0.005, 0, 0.6), (0, 0, 0, 0.7),
                (-1, -1, 0, 0.8), (0, -1, 0, 0.9)]
        self.prepare_fixture(rows)
        full = [(32718, 32718, 32768), (32718, 32818, 32768),
                (32768, 32718, 32768), (32768, 32768, 32768),
                (32818, 32718, 32768), (32818, 32818, 32768)]
        self.assertEqual(coordinates(self.destination / "full.u16le"), full)
        self.assertEqual(ids(self.destination / "raw_to_full.u32le"),
                         [0, 1, 4, 5, 3, 3, 3, 0, 2])
        expected = [[0, 1, 2, 3, 4, 5], [0, 1], [2, 3, 4, 5],
                    [0], [1], [2, 4], [3, 5]]
        for name, indices in zip(NAMES, expected, strict=True):
            with self.subTest(dataset=name):
                self.assertEqual(ids(self.destination / f"{name}.site_ids.u32le"), indices)
                self.assertEqual(coordinates(self.destination / f"{name}.u16le"),
                                 [full[i] for i in indices])
        metadata = json.loads((self.destination / "MANIFEST.json").read_bytes())
        self.assertEqual(metadata["counts"]["merged_returns"], 3)
        self.assertEqual(metadata["boundaries"]["raw_to_quantized_quadrant_changes"], 1)
        self.assertEqual(metadata["boundaries"]["sites_mixing_raw_quadrants"], 1)
        self.run_cli("read", "--path", self.destination)

    def test_empty_pieces_and_negative_zero_have_unique_ownership(self):
        self.prepare_fixture([(-0.0, -0.0, 0, 2.0)])
        for name in NAMES:
            expected = [(32768, 32768, 32768)] if name in (
                "full", "half_x_nonneg", "quarter_x_nonneg_y_nonneg") else []
            self.assertEqual(coordinates(self.destination / f"{name}.u16le"), expected)
        self.run_cli("read", "--path", self.destination)

    def test_invalid_raw_rejected_before_creating_output(self):
        invalid = [b"", b"\0", b"\0" * 15, b"\0" * 17]
        for value in (math.nan, math.inf, -math.inf):
            for axis in range(4):
                row = [0.0] * 4
                row[axis] = value
                invalid.append(packed([row]))
        invalid.extend(packed([row]) for row in ((700, 0, 0, 0),
                       (-700, 0, 0, 0), (0, 0, 700, 0)))
        for number, raw in enumerate(invalid):
            with self.subTest(invalid=number):
                self.source.write_bytes(raw)
                self.run_cli("prepare", "--input", self.source, "--output",
                             self.destination, success=False)
                self.assertFalse(self.destination.exists())

    def test_existing_directory_is_not_overwritten(self):
        self.prepare_fixture([(1, 1, 1, 1)])
        before = {p.name: p.read_bytes() for p in self.destination.iterdir()}
        self.source.write_bytes(packed([(-1, -1, -1, 0)]))
        self.run_cli("prepare", "--input", self.source, "--output",
                     self.destination, success=False)
        self.assertEqual(before, {p.name: p.read_bytes() for p in self.destination.iterdir()})

    def test_reader_rejects_each_artifact_corruption(self):
        self.prepare_fixture([(-1, -1, 0, 0), (1, 1, 0, 0)])
        binary_files = sorted(self.destination.glob("*.u16le"))
        binary_files += sorted(self.destination.glob("*.u32le"))
        self.assertEqual(len(binary_files), 15)
        for path in binary_files:
            original = path.read_bytes()
            with self.subTest(artifact=path.name):
                changed = bytes([original[0] ^ 1]) + original[1:] if original else b"\0"
                path.write_bytes(changed)
                self.run_cli("read", "--path", self.destination, success=False)
                path.write_bytes(original)
        self.run_cli("read", "--path", self.destination)

    def test_reader_rejects_changed_source_and_manifest(self):
        self.prepare_fixture([(1, 1, 0, 0)])
        original_source = self.source.read_bytes()
        self.source.write_bytes(packed([(-1, 1, 0, 0)]))
        self.run_cli("read", "--path", self.destination, success=False)
        self.source.write_bytes(original_source)
        manifest = self.destination / "MANIFEST.json"
        original_manifest = manifest.read_bytes()
        document = json.loads(original_manifest)
        document["unexpected_unchecked_metadata"] = True
        manifest.write_text(json.dumps(document), encoding="utf-8")
        self.run_cli("read", "--path", self.destination, success=False)
        manifest.write_bytes(original_manifest)
        self.run_cli("read", "--path", self.destination)

    def test_reader_reconstructs_geometry_even_with_forged_hashes(self):
        self.prepare_fixture([(1, 1, 0, 0)])
        manifest_path = self.destination / "MANIFEST.json"
        completion_path = self.destination / "COMPLETION.json"
        manifest = json.loads(manifest_path.read_bytes())
        completion = json.loads(completion_path.read_bytes())
        binary = self.destination / "quarter_x_nonneg_y_nonneg.u16le"
        binary.write_bytes(struct.pack("<HHH", 32718, 32718, 32768))
        bad_hash = hashlib.sha256(binary.read_bytes()).hexdigest()
        manifest["datasets"]["quarter_x_nonneg_y_nonneg"]["points_sha256"] = bad_hash
        manifest_path.write_text(json.dumps(manifest, sort_keys=True, indent=2,
                                           ensure_ascii=False) + "\n", encoding="utf-8")
        completion["manifest_sha256"] = hashlib.sha256(manifest_path.read_bytes()).hexdigest()
        completion["output_sha256"][binary.name] = bad_hash
        completion_path.write_text(json.dumps(completion, sort_keys=True, indent=2,
                                             ensure_ascii=False) + "\n", encoding="utf-8")
        self.run_cli("read", "--path", self.destination, success=False)

    def test_late_write_failure_keeps_failed_capture(self):
        self.source.write_bytes(packed([(1, 1, 0, 0)]))
        original_write = spatial._write_bytes

        def fail_payload(path, data):
            if path.name == "raw_to_full.u32le":
                raise OSError("injected payload write failure")
            return original_write(path, data)

        with patch.object(spatial, "_write_bytes", side_effect=fail_payload):
            with self.assertRaises(OSError):
                spatial.prepare(self.source, self.destination)
        completion = json.loads((self.destination / "COMPLETION.json").read_bytes())
        self.assertEqual(completion["status"], "failed")
        self.assertIn("injected payload write failure", completion["error"])
        self.assertTrue((self.destination / "MANIFEST.json").is_file())
        self.run_cli("read", "--path", self.destination, success=False)

    def test_preparation_hash_closure_cannot_persist_success(self):
        self.source.write_bytes(packed([(1, 1, 0, 0)]))
        original_hash = spatial._current_hash

        def changed_payload_hash(path):
            return "0" * 64 if path.name == "full.u16le" else original_hash(path)

        with patch.object(spatial, "_current_hash", side_effect=changed_payload_hash):
            with self.assertRaises(ValueError):
                spatial.prepare(self.source, self.destination)
        completion = json.loads((self.destination / "COMPLETION.json").read_bytes())
        self.assertEqual(completion["status"], "failed")
        self.run_cli("read", "--path", self.destination, success=False)

    def test_reader_checks_final_payload_hashes(self):
        self.prepare_fixture([(1, 1, 0, 0)])
        original_hash = spatial._current_hash

        def changed_payload_hash(path):
            return "0" * 64 if path.name == "full.u16le" else original_hash(path)

        with patch.object(spatial, "_current_hash", side_effect=changed_payload_hash):
            with self.assertRaises(ValueError):
                spatial.read(self.destination)


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(SpatialPreparationTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    print(json.dumps({"schema": "mhgp8_lidar_spatial_gate_v1",
                      "status": "passed" if result.wasSuccessful() else "failed",
                      "tests": result.testsRun, "failures": len(result.failures),
                      "errors": len(result.errors), "optimized": bool(sys.flags.optimize),
                      "source_sha256": hashlib.sha256(PREPARER.read_bytes()).hexdigest(),
                      "gate_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()},
                     sort_keys=True))
    sys.exit(0 if result.wasSuccessful() else 1)
