#!/usr/bin/env python3
"""Prepare exact, unique u16 sites; never qualify HGP of raw LiDAR returns."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

BASE = Path(__file__).resolve().parent
FRAMES = (0, 1, 2, 3, 4, 100, 200)
GROUPS = {"single_000000": (0,), "single_000100": (100,),
          "single_000200": (200,), "overlap_5": (0, 1, 2, 3, 4)}
SIZES = (8000, 16000, 32000, 50000)
TOL = 1e-4
SCOPE = "exact_u16_unique_sites_not_raw_returns_or_full_hgp_qualification"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1048576), b""):
            digest.update(block)
    return digest.hexdigest()


def identity():
    return [[float(i == j) for j in range(4)] for i in range(4)]


def multiply(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4))
             for j in range(4)] for i in range(4)]


def inverse(a):
    require(len(a) == 4 and all(len(row) == 4 for row in a), "invalid matrix shape")
    require(all(math.isfinite(x) for row in a for x in row), "nonfinite matrix")
    rows = [list(row) + identity()[i] for i, row in enumerate(a)]
    for col in range(4):
        pivot = max(range(col, 4), key=lambda i: abs(rows[i][col]))
        require(abs(rows[pivot][col]) > 1e-12, "singular matrix")
        rows[col], rows[pivot] = rows[pivot], rows[col]
        scale = rows[col][col]
        rows[col] = [x / scale for x in rows[col]]
        for i in range(4):
            if i != col:
                factor = rows[i][col]
                rows[i] = [x - factor * y for x, y in zip(rows[i], rows[col])]
    return [row[4:] for row in rows]


def rigid(values):
    require(len(values) == 12, "expected exactly twelve matrix coefficients")
    require(all(math.isfinite(x) for x in values), "nonfinite calibration/pose")
    matrix = [values[i:i + 4] for i in range(0, 12, 4)] + [[0., 0., 0., 1.]]
    r = [row[:3] for row in matrix[:3]]
    error = max(abs(sum(r[i][k] * r[j][k] for k in range(3)) - (i == j))
                for i in range(3) for j in range(3))
    determinant = sum(r[0][i] * (r[1][(i + 1) % 3] * r[2][(i + 2) % 3]
                       - r[1][(i + 2) % 3] * r[2][(i + 1) % 3]) for i in range(3))
    require(error <= TOL and abs(determinant - 1.) <= TOL, "non-rigid calibration/pose")
    return matrix


def transform(matrix, point):
    return tuple(sum(matrix[i][j] * point[j] for j in range(3)) + matrix[i][3]
                 for i in range(3))


def quantize(point):
    require(len(point) == 3 and all(math.isfinite(x) for x in point), "nonfinite xyz")
    scaled = [50. * x + 32768. for x in point]
    require(all(math.isfinite(x) for x in scaled), "quantizer overflow")
    result = tuple(math.floor(x + .5) for x in scaled)
    require(all(0 <= x <= 65535 for x in result), "coordinate outside fixed u16 grid")
    return result


def priority(point):
    packed = struct.pack("<HHH", *point)
    return hashlib.blake2b(struct.pack("<I", 3) + packed, digest_size=16).digest(), point


def locate(root, relatives):
    hits = sorted({(root / name).resolve() for name in relatives if (root / name).is_file()})
    require(len(hits) == 1, "expected one input, found " + str(hits) + " for " + str(relatives))
    require(hits[0].is_relative_to(BASE), "input resolves outside audit data directory")
    return hits[0]


def collect(frames, raw, matrices):
    per_frame, records, all_sites = {}, {}, set()
    low, high = [math.inf] * 3, [-math.inf] * 3
    for frame in frames:
        payload = raw[frame]
        require(len(payload) > 0 and len(payload) % 16 == 0, "empty/truncated xyzi file")
        sensor_sites, scan_sites, mapped = set(), set(), []
        for return_id, xyzi in enumerate(struct.iter_unpack("<ffff", payload)):
            require(all(math.isfinite(x) for x in xyzi),
                    f"nonfinite xyzi: frame={frame}, return={return_id}")
            sensor_sites.add(xyzi[:3])
            point = transform(matrices[frame], xyzi)
            q = quantize(point)
            for axis in range(3):
                low[axis], high[axis] = min(low[axis], point[axis]), max(high[axis], point[axis])
            mapped.append(q)
            scan_sites.add(q)
        per_frame[str(frame)] = {"raw_points": len(mapped), "raw_sensor_unique_xyz": len(sensor_sites),
                                 "quantized_sites": len(scan_sites),
                                 "within_scan_quant_duplicates": len(mapped) - len(scan_sites),
                                 "cross_scan_matches_previous": len(scan_sites & all_sites)}
        records[frame] = mapped
        all_sites.update(scan_sites)
    raw_count = sum(x["raw_points"] for x in per_frame.values())
    within = sum(x["within_scan_quant_duplicates"] for x in per_frame.values())
    cross = sum(x["cross_scan_matches_previous"] for x in per_frame.values())
    require(raw_count - len(all_sites) == within + cross, "collision accounting mismatch")
    return sorted(all_sites), records, {"per_frame": per_frame, "raw_points": raw_count,
        "raw_sensor_unique_xyz_sum_per_frame": sum(x["raw_sensor_unique_xyz"] for x in per_frame.values()),
        "unique_sites": len(all_sites), "within_scan_quant_duplicates": within,
        "cross_scan_matches": cross, "raw_minus_unique": raw_count - len(all_sites),
        "bbox_transformed_m": {"low": low, "high": high}}


def file_record(path, root, count=None):
    record = {"path": str(path.relative_to(root)), "bytes": path.stat().st_size, "sha256": sha256(path)}
    if count is not None:
        record["n"] = count
    return record


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n", encoding="utf-8")


def write_points(path, points):
    with path.open("xb") as stream:
        for point in points:
            stream.write(struct.pack("<HHH", *point))


def prepare(data, out):
    require(data.is_relative_to(BASE) and out.is_relative_to(BASE), "data/out must be inside audit directory")
    require(data.is_dir(), "missing data directory")
    require(not out.exists() or (out.is_dir() and not any(out.iterdir())), "output must be new or empty")
    sequence_roots = ("dataset/sequences/08", "sequences/08", "08", ".")
    calibration = locate(data, [f"{root}/calib.txt" for root in sequence_roots])
    poses_path = locate(data, ("dataset/poses/08.txt", "poses/08.txt", "posesodom08.txt", "08.txt"))
    calibration_payload, poses_payload = calibration.read_bytes(), poses_path.read_bytes()
    entries = [line.partition(":")[2].split() for line in calibration_payload.decode("utf-8").splitlines()
               if line.partition(":")[0].strip() == "Tr"]
    require(len(entries) == 1, "calibration requires exactly one Tr entry")
    c = rigid([float(x) for x in entries[0]])
    lines = poses_payload.decode("utf-8").splitlines()
    require(len(lines) > max(FRAMES), "poses file lacks a requested frame")
    poses = [rigid([float(x) for x in line.split()]) for line in lines]
    matrices = {i: multiply(multiply(multiply(inverse(c), inverse(poses[0])), poses[i]), c)
                for i in FRAMES}
    paths = {i: locate(data, [f"{root}/velodyne/{i:06d}.bin" for root in sequence_roots]
                            + [f"{i:06d}.bin"]) for i in FRAMES}
    raw = {i: path.read_bytes() for i, path in paths.items()}
    payloads = [(calibration, calibration_payload), (poses_path, poses_payload)]
    payloads.extend((paths[i], raw[i]) for i in FRAMES)
    pins = [{"path": str(path.relative_to(BASE)), "bytes": len(payload),
             "sha256": hashlib.sha256(payload).hexdigest()} for path, payload in payloads]
    source = {"path": str(Path(__file__).resolve().relative_to(BASE)), "sha256": sha256(Path(__file__))}
    common = {"scope": SCOPE, "generator": source, "input_files": pins,
        "input_paths_relative_to": "directory_containing_prepare_inputs.py",
        "output_paths_relative_to": str(out.relative_to(BASE)),
        "frame": "LiDAR_scan_000000", "matrix_formula": "inverse(Tr) * inverse(P0) * Pi * Tr",
        "calibration_Tr": c, "poses": {str(i): poses[i] for i in FRAMES},
        "transforms": {str(i): matrices[i] for i in FRAMES}, "rotation_tolerance": TOL,
        "matrix_inverse": "4x4_Gauss_Jordan_partial_pivot_no_transpose_approximation",
        "quantization": {"formula": "floor(50*x + 32768 + 0.5)", "step_m": .02,
                         "all_axes_same_grid": True, "clipping_jitter_adaptation": False,
                         "invalid_input_policy": "reject_entire_preparation"},
        "raw_geometry_policy": "source float32 xyzi retained unchanged; unique quantized sites define a different object",
        "sampling": {"seed_u32_le": 3, "hash": "blake2b_128(seed_bytes + packed_u16le_xyz)",
                     "tie_break": "lexicographic_xyz", "order": "ascending_priority", "targets": SIZES}}
    # Validate every input geometrically before publishing any successful dataset.
    prepared = {name: collect(frames, raw, matrices) for name, frames in GROUPS.items()}
    require(all(sha256(BASE / item["path"]) == item["sha256"] for item in pins), "input changed while reading")
    out.mkdir(parents=True, exist_ok=True)
    manifest = dict(common, status="prepared", datasets={})
    for name, (sites, records, counts) in prepared.items():
        directory = out / name
        directory.mkdir()
        require(len(sites) <= 2 ** 32 and counts["raw_points"] <= 2 ** 32, "mapping ID overflow")
        ids = {point: i for i, point in enumerate(sites)}
        full = directory / "full.u16le"
        write_points(full, sites)
        mapping = directory / "raw_to_site.u32le"
        with mapping.open("xb") as stream:
            for frame, points in records.items():
                for return_id, point in enumerate(points):
                    stream.write(struct.pack("<III", frame, return_id, ids[point]))
        report = dict(common, name=name, frames=GROUPS[name], counts=counts,
                      full=dict(file_record(full, out, len(sites)), order="lexicographic_xyz_site_id"),
                      mapping=dict(file_record(mapping, out, counts["raw_points"]),
                                   record="<III frame_id, original_return_id, lexicographic_site_id"), samples=[])
        ordered = sorted(sites, key=priority)
        for size in SIZES:
            if len(sites) < size:
                report["samples"].append({"requested_n": size, "available_n": len(sites),
                                          "status": "rejected_insufficient_unique_sites", "file": None})
                continue
            sample = directory / f"n{size}.u16le"
            write_points(sample, ordered[:size])
            report["samples"].append(dict(file_record(sample, out, size), requested_n=size, status="prepared"))
        write_json(directory / "METADATA.json", report)
        manifest["datasets"][name] = file_record(directory / "METADATA.json", out)
    require(all(sha256(BASE / item["path"]) == item["sha256"] for item in pins), "input changed during preparation")
    write_json(out / "MANIFEST.json", manifest)
    return {"status": "prepared", "scope": SCOPE, "manifest": file_record(out / "MANIFEST.json", BASE)}


def selftest():
    checks = 0
    def rejected(call):
        nonlocal checks
        try:
            call()
        except (ValueError, FileNotFoundError):
            checks += 1
            return
        raise ValueError("selftest: expected rejection")
    c = rigid([0., -1., 0., 1., 1., 0., 0., 2., 0., 0., 1., 3.])
    p0 = rigid([1., 0., 0., 10., 0., 1., 0., 0., 0., 0., 1., 0.])
    pi = rigid([0., -1., 0., 10., 1., 0., 0., 5., 0., 0., 1., 0.])
    conjugated = multiply(multiply(multiply(inverse(c), inverse(p0)), pi), c)
    require(transform(conjugated, (2., 3., 4.)) == (1., 5., 4.), "pose conjugation")
    general = [[2., 1., 0., 3.], [0., 3., 0., 5.], [0., 0., 4., 7.], [0., 0., 0., 1.]]
    product = multiply(general, inverse(general))
    require(max(abs(product[i][j] - (i == j)) for i in range(4) for j in range(4)) < 1e-12,
            "general inverse was approximated with a transpose")
    checks += 2
    for call in (lambda: locate(BASE, ("__missing_input_selftest__.bin",)), lambda: rigid([]),
                 lambda: rigid([math.nan] * 12), lambda: rigid([math.inf] * 12),
                 lambda: rigid([0.] * 12), lambda: inverse([[0.] * 4 for _ in range(4)]),
                 lambda: quantize((math.nan, 0., 0.)), lambda: quantize((math.inf, 0., 0.)),
                 lambda: quantize((1000., 0., 0.)), lambda: quantize((-1000., 0., 0.)),
                 lambda: collect((0,), {0: b"bad"}, {0: identity()})):
        rejected(call)
    raw = {0: b"".join(struct.pack("<ffff", x, 0., 0., 1.) for x in (0., .001, .04)),
           1: struct.pack("<ffff", 0., 0., 0., 1.)}
    sites, records, counts = collect((0, 1), raw, {0: identity(), 1: identity()})
    ids = {point: i for i, point in enumerate(sites)}
    mapping = [(frame, index, ids[point]) for frame, points in records.items() for index, point in enumerate(points)]
    require(mapping == [(0, 0, 0), (0, 1, 0), (0, 2, 1), (1, 0, 0)], "raw mapping/collision")
    require(counts["within_scan_quant_duplicates"] == counts["cross_scan_matches"] == 1,
            "collision decomposition")
    ordered = sorted([(i, 0, 0) for i in range(32)], key=priority)
    prefixes = [ordered[:n] for n in (8, 16, 32)]
    require(prefixes[0] == prefixes[1][:8] == prefixes[2][:8] and prefixes[1] == prefixes[2][:16], "nested samples")
    neighbors = ((3, 0, 0), (0, 2, 0))
    def nearest(scales):
        return min(range(2), key=lambda j: sum((neighbors[j][i] * scales[i]) ** 2 for i in range(3)))
    require(nearest((1, 1, 1)) == 1 and nearest((1, 2, 1)) == 0,
            "anisotropic scaling changes nearest neighbor")
    a, b, returns = (0, 0, 0), (4, 0, 0), [(2, 0, 0), (2, 0, 0)]
    def depth(points):
        return sum(sum((z[i] - a[i]) * (b[i] - z[i]) for i in range(3)) > 0 for z in points)
    require(depth(returns) == 2 and depth(set(returns)) == 1, "duplicate multiplicity changes depth at K=2")
    checks += 5
    return {"status": "passed", "checks": checks, "scope": SCOPE}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", type=Path)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    require((args.selftest and args.data is None and args.out is None)
            or (not args.selftest and args.data is not None and args.out is not None),
            "use --selftest alone, or --data DIR --out DIR")
    result = selftest() if args.selftest else prepare(args.data.resolve(), args.out.resolve())
    print(json.dumps(result, sort_keys=True, allow_nan=False))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, struct.error) as error:
        print(json.dumps({"status": "rejected", "scope": SCOPE, "error": str(error)}), file=sys.stderr)
        sys.exit(2)
