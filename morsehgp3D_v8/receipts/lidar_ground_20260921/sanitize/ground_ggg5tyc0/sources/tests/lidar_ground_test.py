#!/usr/bin/env python3
"""Independent mask/ID tests for complete-frame ground removal preparation.

No segmentation-quality or HGP timing claim is made here. Fixtures use an
explicit synthetic producer, not semantic labels or simulated Patchwork++
success. Expected sites, rounding, masks and sensor-frame partitions are
reconstructed independently with Fraction; no product mapping helper is a
geometric oracle. Historical precision sources remain unchanged.
"""
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


PREPARER = Path(__file__).resolve().parents[1]/"bench/prepare_lidar_ground.py"
BASE_PREPARER = PREPARER.with_name("prepare_lidar_precision.py")
SPEC = importlib.util.spec_from_file_location("lidar_ground_under_test", PREPARER)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("cannot import ground preparer")
ground = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ground)
NAMES = ("full", "half_x_neg", "half_x_nonneg", "quarter_x_neg_y_neg",
         "quarter_x_neg_y_nonneg", "quarter_x_nonneg_y_neg", "quarter_x_nonneg_y_nonneg")


def sha(data):
    return hashlib.sha256(data).hexdigest()


def canonical(value):
    return (json.dumps(value, sort_keys=True, indent=2, ensure_ascii=False, allow_nan=False)+"\n").encode()


def packed(rows):
    return b"".join(struct.pack("<ffff", *row) for row in rows)


def words(rows):
    return b"".join(struct.pack("<IIII", *row) for row in rows)


def ids(data):
    return [item[0] for item in struct.iter_unpack("<I", data)]


def descriptor(raw, mask):
    return dict(schema="mhgp8_lidar_ground_mask_v1", input_sha256=sha(raw), mask_sha256=sha(mask),
                raw_returns=len(raw)//16, encoding="u8_0_unknown_1_ground_2_nonground",
                producer=dict(kind="synthetic_test_fixture", segmentation_performed=False,
                              labels_used=False, temporal_state="not_applicable"))


def expected(raw, mask, profile="float32", precision_mm=None):
    coordinates = [tuple(0.0 if c == 0 else c for c in row[:3]) for row in struct.iter_unpack("<ffff", raw)]
    step = None if profile == "float32" else Fraction(precision_mm or "1")/1000
    def quantize(c):
        value = Fraction.from_float(c)/step+Fraction(1, 2)
        return value.numerator//value.denominator
    represented = coordinates if step is None else [tuple(quantize(c) for c in p) for p in coordinates]
    originals = sorted(set(represented))
    original_ids = {p: i for i, p in enumerate(originals)}
    raw_to_original = [original_ids[p] for p in represented]
    translation = [0.0]*3 if step is None else [-min(p[axis] for p in originals) for axis in range(3)]
    stored = originals if step is None else [tuple(p[axis]+translation[axis] for axis in range(3)) for p in originals]
    decisions = {i: set() for i in range(len(originals))}
    for original, state in zip(raw_to_original, mask, strict=True):
        decisions[original].add(state != 1)
    retained = [i for i in range(len(originals)) if True in decisions[i]]
    groups = {"full": list(range(len(retained)))}
    for positive_x, xname in ((False, "neg"), (True, "nonneg")):
        half = [i for i, original in enumerate(retained) if (originals[original][0] >= 0) == positive_x]
        groups["half_x_"+xname] = half
        for positive_y, yname in ((False, "neg"), (True, "nonneg")):
            groups["quarter_x_"+xname+"_y_"+yname] = [i for i in half if (originals[retained[i]][1] >= 0) == positive_y]
    return dict(coordinates=coordinates, originals=originals, stored=stored, translation=translation,
                raw_to_original=raw_to_original, retained=retained, groups=groups,
                original_kept=bytes(int(True in decisions[i]) for i in range(len(originals))),
                kept_returns=[i for i, state in enumerate(mask) if state != 1],
                removed_returns=[i for i, state in enumerate(mask) if state == 1],
                mixed_decision_sites=sum(values == {False, True} for values in decisions.values()))


class GroundPreparationTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="mhgp8_ground_test_")
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        self.raw_path = self.directory/"scan.bin"
        self.mask_path = self.directory/"mask.u8"
        self.receipt_path = self.directory/"mask.json"
        self.output = self.directory/"prepared"

    def write_inputs(self, raw, mask, receipt=None):
        self.raw_path.write_bytes(raw)
        self.mask_path.write_bytes(mask)
        self.receipt_path.write_bytes(canonical(descriptor(raw, mask) if receipt is None else receipt))

    def run_cli(self, *args, success=True):
        command = [sys.executable, "-B", *(["-O"] if sys.flags.optimize else []), str(PREPARER), *map(str, args)]
        result = subprocess.run(command, text=True, capture_output=True, check=False)
        self.assertEqual(result.returncode == 0, success, result.stdout+result.stderr)
        return result

    def prepare(self, raw=None, mask=None, profile="float32", precision_mm=None):
        if raw is None:
            raw = packed([(-1, -1, 0, 0), (1, 1, 0, 0), (0, 0, 0, 0)])
            mask = bytes((1, 0, 2))
        self.write_inputs(raw, mask)
        result = ground.prepare(self.raw_path, self.mask_path, self.receipt_path, self.output, profile, precision_mm)
        self.assertEqual(result["status"], "passed")
        return raw, mask

    def check_reconstruction(self, raw, mask, profile="float32", precision_mm=None):
        metadata, payloads = ground.reconstruct(raw, mask, descriptor(raw, mask), profile, precision_mm)
        oracle = expected(raw, mask, profile, precision_mm)
        suffix, code = (".f32le", "<fff") if profile == "float32" else (".u32le", "<III")
        fixed = {"raw_to_original.u32le", "retained_to_original.u32le", "original_kept.u8", "mask.u8",
                 "kept_return_ids.u32le", "removed_return_ids.u32le"}
        names = fixed | {name+extension for name in NAMES for extension in (suffix, ".site_ids.u32le", ".original_site_ids.u32le")}
        self.assertEqual(set(payloads), names)
        self.assertEqual(len(payloads), 27)
        self.assertEqual(ids(payloads["raw_to_original.u32le"]), oracle["raw_to_original"])
        self.assertEqual(ids(payloads["retained_to_original.u32le"]), oracle["retained"])
        self.assertEqual(payloads["original_kept.u8"], oracle["original_kept"])
        self.assertEqual(payloads["mask.u8"], mask)
        self.assertEqual(ids(payloads["kept_return_ids.u32le"]), oracle["kept_returns"])
        self.assertEqual(ids(payloads["removed_return_ids.u32le"]), oracle["removed_returns"])
        self.assertEqual(metadata["raw_preparation"]["translation"]["vector"], oracle["translation"])
        self.assertEqual(metadata["raw_preparation"]["partition"]["encoded_sensor_origin"], oracle["translation"])
        self.assertEqual(metadata["raw_preparation"]["counts"]["raw_returns"], len(mask))
        self.assertEqual(metadata["raw_preparation"]["counts"]["unique_sites"], len(oracle["originals"]))
        for name in NAMES:
            local_to_retained = oracle["groups"][name]
            local_to_original = [oracle["retained"][i] for i in local_to_retained]
            self.assertEqual(ids(payloads[name+".site_ids.u32le"]), local_to_retained)
            self.assertEqual(ids(payloads[name+".original_site_ids.u32le"]), local_to_original)
            expected_bytes = b"".join(struct.pack(code, *oracle["stored"][i]) for i in local_to_original)
            self.assertEqual(payloads[name+suffix], expected_bytes)
            self.assertEqual(metadata["datasets"][name]["sites"], len(local_to_retained))
        counts = metadata["ground"]
        for field, value in dict(retained_returns=len(oracle["kept_returns"]), removed_returns=len(oracle["removed_returns"]),
                                 original_sites=len(oracle["originals"]), retained_sites=len(oracle["retained"]),
                                 removed_sites=len(oracle["originals"])-len(oracle["retained"]),
                                 mixed_decision_sites=oracle["mixed_decision_sites"]).items():
            self.assertEqual(counts[field], value, field)
        # The six parent/child relations use retained global IDs, not original
        # IDs with gaps, prefixes, equal-sized sampling, or separately cut masks.
        union_halves = sorted(ids(payloads[NAMES[1]+".site_ids.u32le"])+ids(payloads[NAMES[2]+".site_ids.u32le"]))
        self.assertEqual(union_halves, ids(payloads["full.site_ids.u32le"]))
        for half, quarters in ((NAMES[1], NAMES[3:5]), (NAMES[2], NAMES[5:7])):
            self.assertEqual(sorted(i for name in quarters for i in ids(payloads[name+".site_ids.u32le"])),
                             ids(payloads[half+".site_ids.u32le"]))
        return metadata, payloads, oracle

    def test_unknown_noise_outside_domain_are_conservatively_kept(self):
        raw = packed([(-200, -1, 0, 0), (200, 1, 0, 0), (0, 0, -100, 0), (1, -1, 2, 0)])
        mask = bytes((0, 0, 0, 2))
        # The three unknown decisions stand for omitted/noise/out-of-domain
        # upstream returns. This preparer never guesses that they are ground.
        metadata, _, oracle = self.check_reconstruction(raw, mask)
        self.assertEqual(oracle["kept_returns"], [0, 1, 2, 3])
        self.assertEqual(metadata["ground"]["removed_sites"], 0)

    def test_float32_bits_extremes_subnormals_signed_zero_reflectance(self):
        raw = words([(0xff7fffff, 0x80000001, 0x7f7fffff, 0x7fa12345),
                     (1, 2, 0x80000002, 0xff800000),
                     (0x80000000, 0, 0x80000000, 0x7f800000),
                     (0, 0x80000000, 0, 0xffc12345),
                     (0x3f800000, 0x3f800001, 0, 0x3f000000)])
        metadata, payloads, _ = self.check_reconstruction(raw, bytes((0, 2, 1, 0, 2)))
        self.assertEqual(metadata["raw_preparation"]["counts"]["nonfinite_reflectance_returns"], 4)
        self.assertEqual(metadata["ground"]["mixed_decision_sites"], 1)
        self.assertNotIn((0x80000000,), list(struct.iter_unpack("<I", payloads["full.f32le"])))

    def test_mixed_duplicate_decisions_keep_site_and_all_raw_mappings(self):
        raw = packed([(2, 0, 0, 1), (-1, 0, 0, 2), (2, 0, 0, 3), (-1, 0, 0, 4),
                      (0, 1, 0, 5), (0, 1, 0, 6), (3, 3, 3, 7)])
        mask = bytes((1, 1, 0, 1, 2, 1, 2))
        metadata, payloads, oracle = self.check_reconstruction(raw, mask)
        self.assertEqual(metadata["ground"]["mixed_decision_sites"], 2)
        self.assertEqual(oracle["retained"], [1, 2, 3])
        self.assertEqual(ids(payloads["raw_to_original.u32le"]), [2, 0, 2, 0, 1, 1, 3])
        self.assertEqual(ids(payloads["removed_return_ids.u32le"]), [0, 1, 3, 5])
        self.assertEqual(ids(payloads["kept_return_ids.u32le"]), [2, 4, 6])
        # A removed return can still address a RETAINED site through another
        # return. Return removal and geometric site removal are not identical.
        self.assertIn(oracle["raw_to_original"][0], oracle["retained"])

    def test_sensor_planes_and_seven_partitions_after_one_global_mask(self):
        raw = packed([(-2, -2, 0, 0), (-2, 2, 0, 0), (2, -2, 0, 0), (2, 2, 0, 0),
                      (-0.0, -1, 0, 0), (-1, -0.0, 0, 0), (0, 0, 0, 0), (0, 1, 0, 0), (1, 0, 0, 0)])
        _, payloads, oracle = self.check_reconstruction(raw, bytes((0, 2, 1, 0, 1, 0, 2, 2, 0)))
        origin = oracle["originals"].index((0.0, 0.0, 0.0))
        self.assertIn(origin, ids(payloads["quarter_x_nonneg_y_nonneg.original_site_ids.u32le"]))
        self.assertNotEqual(len(ids(payloads["half_x_neg.site_ids.u32le"])), len(ids(payloads["half_x_nonneg.site_ids.u32le"])))

    def test_grid_reuses_raw_translation_even_when_minima_are_removed(self):
        raw = packed([(-80, -90, -5, 0), (80, 2, 1, 0), (2, 1, 0, 0)])
        metadata, payloads, oracle = self.check_reconstruction(raw, bytes((1, 0, 2)), "grid")
        self.assertEqual(oracle["translation"], [80000, 90000, 5000])
        self.assertEqual(list(struct.iter_unpack("<III", payloads["full.u32le"])), [(82000, 91000, 5000), (160000, 92000, 6000)])
        self.assertEqual(metadata["datasets"]["half_x_neg"]["sites"], 0)

    def test_grid_fusion_across_raw_sides_keeps_mixed_site(self):
        raw = packed([(-.0002, -.0002, 0, 0), (.0002, .0002, 0, 0), (1, -1, 0, 0), (-2, 2, 0, 0)])
        mask = bytes((1, 0, 2, 1))
        for mm in (None, "1", "2.5", "0.1", "1e-1", "+1.00"):
            with self.subTest(precision_mm=mm):
                metadata, _, _ = self.check_reconstruction(raw, mask, "grid", mm)
                if mm in (None, "1", "2.5", "+1.00"):
                    self.assertEqual(metadata["ground"]["mixed_decision_sites"], 1)
                    self.assertGreater(metadata["raw_preparation"]["boundaries"]["raw_to_represented_quadrant_changes"], 0)

    def test_all_ground_keeps_empty_seven_pieces_and_complete_original_maps(self):
        raw = packed([(-3, -2, -1, 0), (3, 2, 1, 0), (-3, -2, -1, 1)])
        for profile in ("float32", "grid"):
            with self.subTest(profile=profile):
                metadata, payloads, _ = self.check_reconstruction(raw, bytes((1, 1, 1)), profile)
                self.assertEqual(metadata["ground"]["retained_sites"], 0)
                self.assertEqual(payloads["retained_to_original.u32le"], b"")
                self.assertEqual(payloads["original_kept.u8"], b"\0\0")
                suffix = ".f32le" if profile == "float32" else ".u32le"
                for name in NAMES:
                    self.assertEqual(payloads[name+suffix], b"")
                    self.assertEqual(payloads[name+".site_ids.u32le"], b"")
                    self.assertEqual(payloads[name+".original_site_ids.u32le"], b"")
        self.prepare(raw, bytes((1, 1, 1)))
        self.assertEqual(ground.read(self.output)["status"], "passed")

    def test_no_ground_and_exact_binary_half_grid_ties(self):
        raw = packed([(-.0625, .0625, 0, 0), (.03125, -.03125, 0, 1), (-.0625, .0625, 0, 2)])
        for profile, mm in (("float32", None), ("grid", None), ("grid", "2.5"), ("grid", "0.1")):
            metadata, _, oracle = self.check_reconstruction(raw, bytes((0, 2, 2)), profile, mm)
            self.assertEqual(oracle["retained"], list(range(len(oracle["originals"]))))
            self.assertEqual(metadata["ground"]["removed_returns"], 0)

    def test_invalid_masks_and_empty_or_incomplete_raw_rejected_before_output(self):
        raw = packed([(1, 2, 3, 0), (-1, -2, -3, 0)])
        for invalid in (b"", b"\0", b"\0\0\0", b"\3\0", b"\xff\2"):
            with self.subTest(mask=invalid):
                self.write_inputs(raw, invalid)
                with self.assertRaises(ValueError):
                    ground.prepare(self.raw_path, self.mask_path, self.receipt_path, self.output)
                self.assertFalse(self.output.exists())
        for invalid in (b"", b"\0", b"\0"*15, b"\0"*17):
            self.write_inputs(invalid, b"")
            with self.assertRaises(ValueError):
                ground.prepare(self.raw_path, self.mask_path, self.receipt_path, self.output)
            self.assertFalse(self.output.exists())
        with self.assertRaises(ValueError):
            ground.reconstruct(raw, bytearray((0, 2)), descriptor(raw, b"\0\2"))

    def test_descriptor_schema_strict_types_hashes_and_no_ambiguous_lists(self):
        raw, mask = packed([(1, 2, 3, 0)]), b"\0"
        valid = descriptor(raw, mask)
        invalid = []
        for field, value in (("schema", "other"), ("encoding", "ground=0"), ("input_sha256", "0"*64),
                             ("mask_sha256", "0"*64), ("raw_returns", True), ("raw_returns", 0),
                             ("raw_returns", 1.0), ("producer", []), ("producer", None), ("producer", {"x": math.nan})):
            invalid.append(dict(valid, **{field: value}))
        invalid.extend((dict(valid, unexpected=1), {k: v for k, v in valid.items() if k != "producer"},
                        dict(valid, ground_ids=[0], nonground_ids=[0])))
        for receipt in invalid:
            with self.subTest(receipt=receipt):
                with self.assertRaises(ValueError):
                    ground.reconstruct(raw, mask, receipt)

    def test_nonfinite_xyz_are_not_hidden_by_ground_mask(self):
        for value in (math.nan, math.inf, -math.inf):
            for axis in range(3):
                point = [0.0]*4
                point[axis] = value
                raw, mask = packed([point]), b"\1"
                self.write_inputs(raw, mask)
                with self.assertRaises(ValueError):
                    ground.prepare(self.raw_path, self.mask_path, self.receipt_path, self.output)
                self.assertFalse(self.output.exists())

    def test_grid_span_and_parameters_cannot_be_changed_by_removal(self):
        raw, mask = packed([(0, 0, 0, 0), (1, 0, 0, 0)]), b"\1\0"
        # Even if the offending extreme would be removed, the common original
        # grid must be representable. No new adaptive scale or per-piece shift.
        with self.assertRaises(ValueError):
            ground.reconstruct(raw, mask, descriptor(raw, mask), "grid", "0.0000001")
        for profile, mm in (("float32", "1"), ("FLOAT32", None), ("grid", 1), ("grid", "0"), ("grid", "nan"), ("grid", "-1")):
            with self.subTest(profile=profile, mm=mm):
                with self.assertRaises(ValueError):
                    ground.reconstruct(raw, mask, descriptor(raw, mask), profile, mm)

    def test_prepare_cli_read_preserves_raw_reflectance_and_provenance(self):
        raw = words([(0x80000000, 0, 0, 0x7fa12345), (0x3f800000, 0x40000000, 0x40400000, 0xff800000)])
        mask = b"\0\2"
        self.write_inputs(raw, mask)
        self.run_cli("prepare", "--input", self.raw_path, "--mask", self.mask_path, "--mask-receipt", self.receipt_path, "--output", self.output)
        result = json.loads(self.run_cli("read", "--path", self.output).stdout)
        self.assertEqual(result["status"], "passed")
        self.assertEqual(self.raw_path.read_bytes(), raw)
        self.assertEqual(self.mask_path.read_bytes(), mask)
        self.assertEqual(json.loads(self.receipt_path.read_bytes()), descriptor(raw, mask))
        manifest = (self.output/"MANIFEST.json").read_bytes()
        self.assertIn(sha(raw).encode(), manifest)
        self.assertIn(sha(mask).encode(), manifest)
        self.assertIn(sha(BASE_PREPARER.read_bytes()).encode(), manifest)
        self.assertNotIn(b"NaN", manifest)

    def test_reader_rejects_each_payload_corruption_including_empty_pieces(self):
        for profile in ("float32", "grid"):
            with self.subTest(profile=profile):
                self.output = self.directory/profile
                self.prepare(profile=profile)
                payloads = [p for p in self.output.iterdir() if p.name not in ("MANIFEST.json", "COMPLETION.json")]
                self.assertEqual(len(payloads), 27)
                for path in payloads:
                    original = path.read_bytes()
                    path.write_bytes(bytes((original[0] ^ 1,))+original[1:] if original else b"\0")
                    with self.assertRaises(ValueError):
                        ground.read(self.output)
                    path.write_bytes(original)
                ground.read(self.output)

    def test_reader_reconstructs_even_if_payload_hashes_are_forged(self):
        self.prepare()
        target = self.output/"full.f32le"
        old_digest = sha(target.read_bytes())
        target.write_bytes(struct.pack("<fff", 100, 100, 100)+target.read_bytes()[12:])
        new_digest = sha(target.read_bytes())
        manifest_path, completion_path = self.output/"MANIFEST.json", self.output/"COMPLETION.json"
        old_manifest = manifest_path.read_bytes()
        changed_manifest = old_manifest.replace(old_digest.encode(), new_digest.encode())
        self.assertNotEqual(changed_manifest, old_manifest)
        manifest_path.write_bytes(changed_manifest)
        completion = completion_path.read_bytes().replace(old_digest.encode(), new_digest.encode())
        completion = completion.replace(sha(old_manifest).encode(), sha(changed_manifest).encode())
        completion_path.write_bytes(completion)
        with self.assertRaises(ValueError):
            ground.read(self.output)

    def test_stale_raw_mask_and_descriptor_are_rejected(self):
        self.prepare()
        for path in (self.raw_path, self.mask_path, self.receipt_path):
            with self.subTest(path=path.name):
                original = path.read_bytes()
                if path == self.raw_path:
                    changed = original[:12]+struct.pack("<I", 0x7fa98765)+original[16:]
                elif path == self.mask_path:
                    changed = bytes((0 if original[0] == 1 else 1,))+original[1:]
                else:
                    receipt = json.loads(original)
                    receipt["producer"]["extra_provenance"] = "changed_after_preparation"
                    changed = canonical(receipt)
                path.write_bytes(changed)
                with self.assertRaises(ValueError):
                    ground.read(self.output)
                path.write_bytes(original)
        self.assertEqual(ground.read(self.output)["status"], "passed")

    def test_existing_output_is_not_overwritten_and_stale_receipt_precedes_mkdir(self):
        self.prepare()
        before = {p.name: p.read_bytes() for p in self.output.iterdir()}
        with self.assertRaises(ValueError):
            ground.prepare(self.raw_path, self.mask_path, self.receipt_path, self.output)
        self.assertEqual(before, {p.name: p.read_bytes() for p in self.output.iterdir()})
        receipt = json.loads(self.receipt_path.read_bytes())
        receipt["input_sha256"] = "0"*64
        self.receipt_path.write_bytes(canonical(receipt))
        new_output = self.directory/"absent_parent"/"new"
        with self.assertRaises(ValueError):
            ground.prepare(self.raw_path, self.mask_path, self.receipt_path, new_output)
        self.assertFalse(new_output.parent.exists())

    def test_late_write_failure_keeps_failed_capture(self):
        self.write_inputs(packed([(1, 2, 3, 0)]), b"\0")
        original = ground._write_bytes
        def failing(path, data):
            if path.name == "retained_to_original.u32le":
                raise OSError("injected ground mapping write failure")
            return original(path, data)
        with patch.object(ground, "_write_bytes", side_effect=failing):
            with self.assertRaises(OSError):
                ground.prepare(self.raw_path, self.mask_path, self.receipt_path, self.output)
        completion = json.loads((self.output/"COMPLETION.json").read_bytes())
        self.assertEqual(completion["status"], "failed")
        self.assertIn("injected ground mapping", completion["error"])
        with self.assertRaises(ValueError):
            ground.read(self.output)

    def test_write_closure_never_publishes_success_on_input_change(self):
        raw, mask = packed([(1, 2, 3, 0), (-1, -2, -3, 0)]), b"\0\2"
        self.write_inputs(raw, mask)
        original = ground._write_bytes
        def change_source(path, data):
            result = original(path, data)
            if path.name == "full.f32le":
                self.mask_path.write_bytes(b"\1\2")
            return result
        with patch.object(ground, "_write_bytes", side_effect=change_source):
            with self.assertRaises(ValueError):
                ground.prepare(self.raw_path, self.mask_path, self.receipt_path, self.output)
        self.assertEqual(json.loads((self.output/"COMPLETION.json").read_bytes())["status"], "failed")

    def test_final_reader_closure_extra_symlink_and_strict_json(self):
        self.prepare()
        original_hash = ground._current_hash
        visits = 0
        def changed_final_mask_hash(path):
            nonlocal visits
            if path == self.mask_path:
                visits += 1
                # Initial pins and expected manifest read the same valid mask;
                # only the final source closure reports its simulated change.
                if visits >= 3:
                    return "0"*64
            return original_hash(path)
        with patch.object(ground, "_current_hash", side_effect=changed_final_mask_hash):
            with self.assertRaises(ValueError):
                ground.read(self.output)
        self.assertGreaterEqual(visits, 3)
        extra = self.output/"unaccounted_output"
        extra.write_bytes(b"")
        with self.assertRaises(ValueError):
            ground.read(self.output)
        extra.unlink()
        artifact = self.output/"half_x_neg.f32le"
        original = artifact.read_bytes()
        artifact.unlink()
        artifact.symlink_to(self.output/"full.f32le")
        with self.assertRaises(ValueError):
            ground.read(self.output)
        artifact.unlink()
        artifact.write_bytes(original)
        self.assertEqual(ground.read(self.output)["status"], "passed")
        for text in (b'{"x":1,"x":2}', b'{"x":NaN}'):
            with self.assertRaises(ValueError):
                ground.load_json(text)


if __name__ == "__main__":
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(GroundPreparationTests))
    print(json.dumps(dict(schema="mhgp8_lidar_ground_test_v1", status="passed" if result.wasSuccessful() else "failed",
        tests=result.testsRun, failures=len(result.failures), errors=len(result.errors), optimized=bool(sys.flags.optimize),
        source_sha256=sha(PREPARER.read_bytes()), precision_source_sha256=sha(BASE_PREPARER.read_bytes()),
        test_sha256=sha(Path(__file__).read_bytes())), sort_keys=True))
    sys.exit(0 if result.wasSuccessful() else 1)
