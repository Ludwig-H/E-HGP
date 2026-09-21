#!/usr/bin/env python3
"""Independent tests: lossless finite float32 and configurable exact decimal grid.

The failure-preserving I/O test pattern is explicitly reused from
lidar_spatial_gate.py SHA256
f0576c9c48932154859a224998a1730a5d5a747812eef8f47f0f5e8755577e18.
Geometric expectations below use Fraction independently, never a product
quantization helper. No native engine or cloud execution is involved.
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

PREPARER = Path(__file__).resolve().parents[1]/"bench/prepare_lidar_precision.py"
SPEC = importlib.util.spec_from_file_location("lidar_precision_under_test", PREPARER)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("cannot import precision preparer")
precision = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(precision)
NAMES = ("full", "half_x_neg", "half_x_nonneg", "quarter_x_neg_y_neg",
         "quarter_x_neg_y_nonneg", "quarter_x_nonneg_y_neg", "quarter_x_nonneg_y_nonneg")


def packed(rows):
    return b"".join(struct.pack("<ffff", *row) for row in rows)


def f32(bits):
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def ids(raw):
    return [r[0] for r in struct.iter_unpack("<I", raw)]


def expected(raw, profile="float32", mm=None):
    rows = [tuple(0.0 if c == 0 else c for c in row[:3]) for row in struct.iter_unpack("<ffff", raw)]
    step = None if profile == "float32" else Fraction(mm or "1")/1000
    def q(c):
        rational = Fraction.from_float(c)/step + Fraction(1, 2)
        return rational.numerator//rational.denominator
    signed = rows if step is None else [tuple(q(c) for c in row) for row in rows]
    full = sorted(set(signed))
    offset = [0.0]*3 if step is None else [-min(p[i] for p in full) for i in range(3)]
    stored = full if step is None else [tuple(p[i]+offset[i] for i in range(3)) for p in full]
    index = {p:i for i,p in enumerate(full)}
    groups = {"full":list(range(len(full)))}
    for xpos,xname in ((False,"neg"),(True,"nonneg")):
        half = [i for i,p in enumerate(full) if (p[0] >= 0) == xpos]
        groups["half_x_"+xname] = half
        for ypos,yname in ((False,"neg"),(True,"nonneg")):
            groups["quarter_x_"+xname+"_y_"+yname] = [i for i in half if (full[i][1] >= 0) == ypos]
    return rows, full, stored, offset, [index[p] for p in signed], groups


class PrecisionPreparationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="mhgp8_precision_gate_")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.source = self.directory/"scan.bin"
        self.destination = self.directory/"prepared"

    def run_cli(self, *args, success=True):
        command = [sys.executable, "-B", *(["-O"] if sys.flags.optimize else []), str(PREPARER), *map(str,args)]
        result = subprocess.run(command, text=True, capture_output=True, check=False)
        self.assertEqual(result.returncode == 0, success, result.stderr+result.stdout)
        return result

    def check_reconstruction(self, raw, profile="float32", mm=None):
        metadata, payloads = precision.reconstruct(raw, profile, mm)
        rows, full, stored, offset, raw_map, groups = expected(raw, profile, mm)
        code, suffix = ("<fff",".f32le") if profile == "float32" else ("<III",".u32le")
        self.assertEqual(len(payloads),15)
        self.assertEqual(ids(payloads["raw_to_full.u32le"]),raw_map)
        self.assertEqual(metadata["translation"]["vector"],offset)
        self.assertEqual(metadata["partition"]["encoded_sensor_origin"],offset)
        for name in NAMES:
            indices=ids(payloads[name+".site_ids.u32le"])
            self.assertEqual(indices,groups[name])
            self.assertEqual(list(struct.iter_unpack(code,payloads[name+suffix])),[stored[i] for i in indices])
            self.assertEqual(metadata["datasets"][name]["sites"],len(indices))
        self.assertEqual(metadata["counts"]["raw_returns"],len(rows))
        self.assertEqual(metadata["counts"]["exact_float32_unique_xyz"],len(set(rows)))
        self.assertEqual(metadata["counts"]["unique_sites"],len(full))
        return metadata,payloads

    def test_default_float32_subnormals_extremes_neighbours_and_signed_zero(self):
        low, tiny, next_tiny = f32(0xff7fffff), f32(1), f32(2)
        high, one_next = f32(0x7f7fffff), f32(0x3f800001)
        raw = packed([(low,-tiny,high,math.nan),(tiny,tiny,next_tiny,math.inf),
                      (0.0,-0.0,0.0,-math.inf),(-0.0,0.0,-0.0,0.0),
                      (1.0,one_next,-next_tiny,0.5)])
        metadata,payloads=self.check_reconstruction(raw)
        self.assertEqual(metadata["counts"]["unique_sites"],4)
        self.assertEqual(metadata["counts"]["repeated_raw_xyz_returns"],1)
        self.assertEqual(metadata["counts"]["distinct_raw_sites_merged_by_quantization"],0)
        self.assertEqual(metadata["counts"]["nonfinite_reflectance_returns"],3)
        self.assertEqual(metadata["counts"]["negative_zero_xyz_components"],3)
        self.assertEqual(metadata["boundaries"]["raw_to_represented_quadrant_changes"],0)
        self.assertEqual(metadata["quantization_error_proof"]["bound_metres"],dict(numerator=0,denominator=1))
        self.assertEqual(metadata["quantization_error_proof"]["float32_roundtrip_coordinates_verified"],15)
        self.assertTrue(metadata["representation"]["raw_coordinate_values_preserved"])
        self.assertFalse(metadata["representation"]["current_u16_engine_input_compatible"])
        # Every zero in the represented point stream has positive-zero bits.
        words=list(struct.iter_unpack("<I",payloads["full.f32le"]))
        self.assertNotIn((0x80000000,),words)

    def test_default_float32_prepare_read_preserves_raw_reflectance_bits(self):
        raw=packed([(1.0,2.0,3.0,0.0),(-1.0,-2.0,-3.0,0.0)])
        # Include arbitrary NaN payload bits, not merely math.nan roundtrips.
        raw=raw[:12]+struct.pack("<I",0x7fa12345)+raw[16:28]+struct.pack("<I",0xff800000)
        self.source.write_bytes(raw)
        self.run_cli("prepare","--input",self.source,"--output",self.destination)
        result=json.loads(self.run_cli("read","--path",self.destination).stdout)
        self.assertEqual(result["parameters"],dict(profile="float32",precision_mm=None))
        self.assertEqual(self.source.read_bytes(),raw)
        manifest=json.loads((self.destination/"MANIFEST.json").read_bytes())
        self.assertEqual(manifest["raw"]["sha256"],hashlib.sha256(raw).hexdigest())
        self.assertEqual(manifest["raw"]["finite_validation"],"XYZ_only")
        self.assertNotIn(b"NaN",(self.destination/"MANIFEST.json").read_bytes())

    def test_grid_default_1mm_2500um_01mm_fraction_partition_and_maps(self):
        raw=packed([(-80.0,-2.0,-1.0,0.1),(80.0,2.0,1.0,0.2),
                    (-0.0002,-0.0002,0.0,0.3),(0.0002,0.0002,0.0,0.4),
                    (0.0,-0.0,0.0,0.5),(-80.0,-2.0,-1.0,0.6)])
        for mm in (None,"1","2.5","0.1","1e-1","+1.00"):
            with self.subTest(mm=mm):
                metadata,_=self.check_reconstruction(raw,"grid",mm)
                self.assertEqual(metadata["parameters"]["precision_mm"],"1" if mm is None else mm)
                self.assertTrue(metadata["translation"]["common_to_all_seven_datasets"])
                self.assertFalse(metadata["quantization"]["adaptive_scale"])
                if mm in (None,"1","+1.00"):
                    self.assertEqual(metadata["coordinate_bounds"]["encoded_max"][0],160000)
                    self.assertEqual(metadata["counts"]["repeated_raw_xyz_returns"],1)
                    self.assertEqual(metadata["counts"]["distinct_raw_sites_merged_by_quantization"],2)
                    self.assertEqual(metadata["counts"]["merged_returns"],3)

    def test_grid_error_proof_against_fraction_and_half_ties(self):
        raw=packed([(-0.0625,0.0625,0.123456,0.0),(0.03125,-0.03125,-0.3,0.0)])
        for mm in ("1","2.5","0.1"):
            metadata,payloads=self.check_reconstruction(raw,"grid",mm)
            stored=list(struct.iter_unpack("<III",payloads["full.u32le"]))
            offset=metadata["translation"]["vector"]
            maxima=[Fraction(0)]*3
            for index,row in zip(ids(payloads["raw_to_full.u32le"]),struct.iter_unpack("<ffff",raw),strict=True):
                for axis,c in enumerate(row[:3]):
                    error=abs(Fraction(stored[index][axis]-offset[axis])*Fraction(mm)/1000-Fraction.from_float(c))
                    self.assertLessEqual(error,Fraction(mm)/2000)
                    maxima[axis]=max(maxima[axis],error)
            proof=metadata["quantization_error_proof"]
            self.assertEqual(proof["coordinates_verified"],6)
            self.assertEqual(proof["max_abs_error_metres_by_axis"],
                             [dict(numerator=v.numerator,denominator=v.denominator) for v in maxima])
            self.assertEqual(proof["bound_metres"],precision.ratio(Fraction(mm)/2000))
            self.assertFalse(proof["exact_raw_coordinate_values_preserved"])
        # Exact binary half ties and immediate binary64 neighbours.
        for value in (-0.0625,0.0625):
            for c in (math.nextafter(value,-math.inf),value,math.nextafter(value,math.inf)):
                rational=Fraction.from_float(c)*1000+Fraction(1,2)
                self.assertEqual(precision.quantize_coordinate(c),rational.numerator//rational.denominator)

    def test_grid_origin_outside_scene_and_common_translation(self):
        raw=packed([(100.0,200.0,300.0,0.0),(100.001,200.0,300.0,0.0)])
        metadata,_=self.check_reconstruction(raw,"grid")
        self.assertEqual(metadata["translation"]["vector"],[-100000,-200000,-300000])
        self.assertEqual(metadata["datasets"]["half_x_neg"]["sites"],0)
        self.assertEqual(metadata["datasets"]["quarter_x_nonneg_y_nonneg"]["sites"],2)
        self.assertTrue(metadata["partition"]["origin_may_lie_outside_u32_storage"])

    def test_grid_u32_limit_without_clipping_or_adaptive_scale(self):
        # Exact binary grid 2^-32 metres, expressed as a finite decimal in mm.
        step = "0.00000023283064365386962890625"
        exact_limit = packed([(2.0**-32,0.0,0.0,0.0),(1.0,0.0,0.0,0.0)])
        metadata,_ = self.check_reconstruction(exact_limit,"grid",step)
        self.assertEqual(metadata["coordinate_bounds"]["encoded_max"][0],2**32-1)
        self.assertEqual(metadata["translation"]["vector"][0],-1)
        with self.assertRaises(ValueError):
            precision.reconstruct(packed([(0.0,0.0,0.0,0.0),(1.0,0.0,0.0,0.0)]),"grid",step)
        # Decimal-expressible step 0.1 nm: no scientific quota below u32 width.
        accepted=packed([(0.0,0.0,0.0,0.0),(0.25,0.0,0.0,0.0)])
        metadata,_=self.check_reconstruction(accepted,"grid","0.0000001")
        self.assertEqual(metadata["coordinate_bounds"]["encoded_max"][0],2500000000)
        self.source.write_bytes(packed([(0.0,0.0,0.0,0.0),(0.5,0.0,0.0,0.0)]))
        with self.assertRaises(ValueError):
            precision.prepare(self.source,self.destination,"grid","0.0000001")
        self.assertFalse(self.destination.exists())

    def test_invalid_xyz_and_profiles_rejected_before_output(self):
        invalid=[b"",b"\0",b"\0"*15,b"\0"*17]
        for value in (math.nan,math.inf,-math.inf):
            for axis in range(3):
                row=[0.0]*4
                row[axis]=value
                invalid.append(packed([row]))
        for raw in invalid:
            self.source.write_bytes(raw)
            with self.assertRaises(ValueError):
                precision.prepare(self.source,self.destination)
            self.assertFalse(self.destination.exists())
        self.source.write_bytes(packed([(1.0,2.0,3.0,0.0)]))
        options=[("float32","1"),("FLOAT32",None),(True,None),("grid",1),("grid",True)]
        options += [("grid",text) for text in ("0","-1","nan","NaN","Inf","Infinity","1/2","1x0",""," 1","1 ","1e309x")]
        for profile,mm in options:
            with self.subTest(profile=profile,mm=mm):
                with self.assertRaises(ValueError):
                    precision.prepare(self.source,self.destination,profile,mm)
                self.assertFalse(self.destination.exists())

    def test_empty_pieces_and_nonnegative_contact_owner(self):
        for profile,mm in (("float32",None),("grid",None)):
            metadata,_=self.check_reconstruction(packed([(-0.0,-0.0,0.0,0.0)]),profile,mm)
            for name in NAMES:
                self.assertEqual(metadata["datasets"][name]["sites"],
                                 int(name in ("full","half_x_nonneg","quarter_x_nonneg_y_nonneg")))

    def prepare_fixture(self,profile="float32",mm=None):
        raw=packed([(-1.0,-1.0,0.0,0.0),(1.0,1.0,0.0,math.nan)])
        self.source.write_bytes(raw)
        precision.prepare(self.source,self.destination,profile,mm)
        return raw

    def test_reader_rejects_each_binary_corruption_both_profiles(self):
        for profile in ("float32","grid"):
            with self.subTest(profile=profile):
                destination=self.directory/profile
                self.source.write_bytes(packed([(-1.0,-1.0,0.0,0.0),(1.0,1.0,0.0,0.0)]))
                precision.prepare(self.source,destination,profile)
                payloads=[p for p in destination.iterdir() if p.suffix not in (".json",)]
                self.assertEqual(len(payloads),15)
                for path in payloads:
                    original=path.read_bytes()
                    path.write_bytes(bytes([original[0]^1])+original[1:] if original else b"\0")
                    with self.assertRaises(ValueError):
                        precision.read(destination)
                    path.write_bytes(original)
                precision.read(destination)

    def test_reader_reconstructs_even_with_forged_hashes(self):
        self.prepare_fixture("grid","1")
        file=self.destination/"quarter_x_nonneg_y_nonneg.u32le"
        file.write_bytes(struct.pack("<III",0,0,0))
        mpath,cpath=self.destination/"MANIFEST.json",self.destination/"COMPLETION.json"
        m,c=json.loads(mpath.read_bytes()),json.loads(cpath.read_bytes())
        digest=hashlib.sha256(file.read_bytes()).hexdigest()
        m["datasets"]["quarter_x_nonneg_y_nonneg"]["points_sha256"]=digest
        mpath.write_bytes(precision.canonical_json(m))
        c["manifest_sha256"]=hashlib.sha256(mpath.read_bytes()).hexdigest()
        c["output_sha256"][file.name]=digest
        cpath.write_bytes(precision.canonical_json(c))
        with self.assertRaises(ValueError):
            precision.read(self.destination)

    def test_manifest_parameter_types_and_unknown_fields_rejected(self):
        self.prepare_fixture("grid","1")
        mpath=self.destination/"MANIFEST.json"
        original=mpath.read_bytes()
        for parameters in (dict(profile="grid",precision_mm=1),dict(profile="grid",precision_mm=True),
                           dict(profile="float32",precision_mm="1"),dict(profile="grid",precision_mm="1",extra=0)):
            m=json.loads(original)
            m["parameters"]=parameters
            mpath.write_bytes(precision.canonical_json(m))
            with self.assertRaises(ValueError):
                precision.read(self.destination)
        mpath.write_bytes(original)
        precision.read(self.destination)

    def test_existing_output_not_overwritten_and_raw_change_rejected(self):
        raw=self.prepare_fixture()
        before={p.name:p.read_bytes() for p in self.destination.iterdir()}
        with self.assertRaises(ValueError):
            precision.prepare(self.source,self.destination)
        self.assertEqual(before,{p.name:p.read_bytes() for p in self.destination.iterdir()})
        self.source.write_bytes(packed([(2.0,2.0,0.0,0.0)]))
        with self.assertRaises(ValueError):
            precision.read(self.destination)
        self.source.write_bytes(raw)
        precision.read(self.destination)

    def test_late_failure_preserves_failed_capture(self):
        self.source.write_bytes(packed([(1.0,1.0,0.0,0.0)]))
        original=precision._write_bytes
        def fail(path,data):
            if path.name=="raw_to_full.u32le":
                raise OSError("injected payload failure")
            return original(path,data)
        with patch.object(precision,"_write_bytes",side_effect=fail):
            with self.assertRaises(OSError):
                precision.prepare(self.source,self.destination)
        completion=json.loads((self.destination/"COMPLETION.json").read_bytes())
        self.assertEqual(completion["status"],"failed")
        self.assertIn("injected payload failure",completion["error"])
        with self.assertRaises(ValueError):
            precision.read(self.destination)

    def test_closure_failure_never_publishes_success(self):
        self.source.write_bytes(packed([(1.0,1.0,0.0,0.0)]))
        original=precision._current_hash
        def fail(path):
            return "0"*64 if path.name=="full.f32le" else original(path)
        with patch.object(precision,"_current_hash",side_effect=fail):
            with self.assertRaises(ValueError):
                precision.prepare(self.source,self.destination)
        self.assertEqual(json.loads((self.destination/"COMPLETION.json").read_bytes())["status"],"failed")

    def test_final_read_closure_and_extra_symlink_json_artifacts(self):
        self.prepare_fixture()
        original=precision._current_hash
        with patch.object(precision,"_current_hash",
                          side_effect=lambda path:"0"*64 if path.name=="full.f32le" else original(path)):
            with self.assertRaises(ValueError):
                precision.read(self.destination)
        (self.destination/"extra").write_bytes(b"")
        with self.assertRaises(ValueError):
            precision.read(self.destination)
        (self.destination/"extra").unlink()
        link=self.destination/"half_x_neg.f32le"
        original_bytes=link.read_bytes()
        link.unlink()
        link.symlink_to(self.destination/"full.f32le")
        with self.assertRaises(ValueError):
            precision.read(self.destination)
        link.unlink()
        link.write_bytes(original_bytes)
        precision.read(self.destination)
        for text in (b'{"x":1,"x":2}',b'{"x":NaN}'):
            with self.assertRaises(ValueError):
                precision.load_json(text)


if __name__ == "__main__":
    result=unittest.TextTestRunner(verbosity=2).run(
        unittest.defaultTestLoader.loadTestsFromTestCase(PrecisionPreparationTests))
    print(json.dumps(dict(schema="mhgp8_lidar_precision_gate_v2",
        status="passed" if result.wasSuccessful() else "failed",
        tests=result.testsRun,failures=len(result.failures),errors=len(result.errors),
        optimized=bool(sys.flags.optimize),source_sha256=hashlib.sha256(PREPARER.read_bytes()).hexdigest(),
        gate_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),sort_keys=True))
    sys.exit(0 if result.wasSuccessful() else 1)
