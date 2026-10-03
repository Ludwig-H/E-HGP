"""G4 audit campaign: unchanged FULL, qualified covers and official HDBSCAN.

Inputs/dumps remain in the private build/data directories. Only hashes, work
counts and object diagnostics are written to the collected result directory.
"""
import argparse
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import subprocess
import time

import numpy as np
import qualified
from hdbscan_compare import analyze_hdbscan


HERE = Path(__file__).resolve().parent
CONFIG = HERE / "campaign.json"


def need(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    value = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            value.update(block)
    return value.hexdigest()


def save(path, value):
    path = Path(path)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True,
                                    allow_nan=False) + "\n")
    temporary.replace(path)


def compact(result):
    # The complete tree is available during analysis/jitter checks. Its digest
    # attests the derived hierarchy without exporting LiDAR coordinates/labels.
    answer = dict(result)
    for field in ("tree", "entry_dates"):
        raw = json.dumps(answer.pop(field), sort_keys=True,
                         separators=(",", ":"), allow_nan=False).encode()
        answer[field + "_sha256"] = hashlib.sha256(raw).hexdigest()
    return answer


def methods(projection):
    for k, order in projection["orders"].items():
        for name, value in order.items():
            if name == "qualified":
                for m, result in value.items():
                    yield k, "qualified_m" + m, result
            else:
                yield k, name, value


def project(args, name, xyz, ids, labels, config, retain_trees=False):
    directory = args.work / name
    directory.mkdir(parents=True, exist_ok=False)
    xyz_path, ids_path = directory / "sites.u32le", directory / "ids.u32le"
    np.asarray(xyz, dtype="<u4").tofile(xyz_path)
    np.asarray(ids, dtype="<u4").tofile(ids_path)
    dump_path = directory / "points.bin"
    command = [str(args.probe), str(xyz_path), str(ids_path), str(dump_path),
               str(config["kmax"]), str(config["workers"]), str(config["budget_bytes"])]
    began = time.monotonic()
    try:
        result = subprocess.run(command, capture_output=True, timeout=config["native_timeout_seconds"])
    except subprocess.TimeoutExpired as error:
        (args.out / (name + ".native.stdout")).write_bytes(error.stdout or b"")
        (args.out / (name + ".native.stderr")).write_bytes(error.stderr or b"")
        save(args.out / (name + ".native.timeout.json"),
             dict(argv=command, timeout_seconds=config["native_timeout_seconds"],
                  status="timeout", partial_final_dump=dump_path.exists()))
        raise
    (args.out / (name + ".native.stdout")).write_bytes(result.stdout)
    (args.out / (name + ".native.stderr")).write_bytes(result.stderr)
    native_seconds = time.monotonic() - began
    need(result.returncode == 0, "native refusal " + name + ": " + str(result.returncode))
    stats = json.loads(result.stdout)
    need(stats["status"] == "ok" and stats["sites"] == len(ids), "native status/site count")
    payload_digest, payload_bytes = digest(dump_path), dump_path.stat().st_size
    data = qualified.load(dump_path)
    try:
        need(data.bits == 21 and data.kmax == config["kmax"] and data.sites == len(ids), "dump header")
        rows = {int(point_id): i for i, point_id in enumerate(ids)}
        need(len(rows) == len(ids), "input ID uniqueness")
        native_ids = list(data.points[3::4])
        need(set(native_ids) == set(rows), "native original ID set")
        mapped = [rows[point_id] for point_id in native_ids]
        for site, row in enumerate(mapped):
            need(tuple(data.points[4*site:4*site+3]) == tuple(int(x) for x in xyz[row]), "native XYZ/ID match")
        truth = [int(labels[row]) for row in mapped]
        began = time.monotonic()
        projection = qualified.analyse(data, truth, orders=config["orders"])
        elapsed = time.monotonic() - began
        trees = dict(orders={k: dict(qualified={m: projection["orders"][k]["qualified"][m]
                                              for m in config["jitter_thresholds"]})
                            for k in config["jitter_orders"]}) if retain_trees else None
        projection["orders"] = {k: {name: ({m: compact(value) for m, value in result.items()}
                                          if name == "qualified" else compact(result))
                                    for name, result in order.items()}
                                for k, order in projection["orders"].items()}
        # Retain only the qualified compact trees in memory for optional matched
        # perturbation checks. Normal outputs carry their hashes, not the arrays.
        return dict(native=stats, native_wall_seconds=native_seconds,
                    dump_sha256=payload_digest, dump_bytes=payload_bytes,
                    analysis_seconds=elapsed, projection=projection), trees, native_ids
    finally:
        data.close()
        dump_path.unlink()
        xyz_path.unlink()
        ids_path.unlink()


def jitter_check(before, after, before_ids, after_ids, seed):
    rng = np.random.default_rng(seed)
    pairs = rng.integers(0, len(before_ids), size=(256, 2))
    amap = {value: i for i, value in enumerate(after_ids)}
    count = 0
    for k in before["orders"]:
        for m, first in before["orders"][k]["qualified"].items():
            second = after["orders"][k]["qualified"][m]
            left, right = first["tree"], second["tree"]
            for a, b in pairs:
                if a == b:
                    continue
                na = qualified.tree_lca(left, int(a), int(b))
                nb = qualified.tree_lca(right, amap[before_ids[a]], amap[before_ids[b]])
                need(na is not None and nb is not None, "jitter disconnected final projection")
                x, y = sorted((Fraction(left["height"][na]), Fraction(right["height"][nb])), reverse=True)
                # |sqrt(x)-sqrt(y)|<=sqrt(3) is checked without floating roots.
                delta = x - y - 3
                need(delta <= 0 or delta*delta <= 12*y, "qualified radius stability violated")
                count += 1
    return dict(status="ok", pairs_requested=256, pair_checks=count,
                epsilon_squared_grid_units=3, id_matching="original point IDs",
                scope="sampled pair merge heights; finite matched perturbation only")


def one_case(args, name, xyz, ids, labels, metadata, config):
    need(xyz.shape == (len(ids), 3) and labels.shape == (len(ids),), "input shape")
    need(len(np.unique(xyz, axis=0)) == len(ids), "duplicate positions")
    need(bool((xyz >= 0).all()) and bool((xyz < 2**21).all()), "u21 domain")
    initial = dict(schema="ehgp.audit.full_points.case.v1", name=name, status="started",
                   metadata=metadata, config_sha256=digest(CONFIG), sites=len(ids),
                   xyz_sha256=hashlib.sha256(np.asarray(xyz, dtype="<u4").tobytes()).hexdigest(),
                   ids_sha256=hashlib.sha256(np.asarray(ids, dtype="<u4").tobytes()).hexdigest(),
                   label_sha256=hashlib.sha256(np.asarray(labels, dtype="<i4").tobytes()).hexdigest())
    path = args.out / (name + ".json")
    save(path, initial)
    try:
        paired = config["jitter"] and name == config["jitter_case"]
        answer, trees, native_ids = project(args, name, xyz, ids, labels, config, paired)
        initial.update(answer, hdbscan={})
        save(path, initial)
        for k in config["orders"]:
            initial["hdbscan"][str(k)] = compact(analyze_hdbscan(xyz, labels, k))
            save(path, initial)
        if trees is not None:
            rng = np.random.default_rng(config["jitter_seed"])
            perturbed = xyz.astype(np.int64) + rng.integers(-1, 2, size=xyz.shape)
            # Common translation of both clouds preserves every distance while
            # leaving room for the +/-1 displacement at the lower grid boundary.
            need(bool((perturbed >= 0).all()), "jitter input must have a positive margin")
            need(bool((perturbed < 2**21).all()), "jitter u21 upper domain")
            if len(np.unique(perturbed, axis=0)) != len(ids):
                initial["jitter"] = dict(status="outside_unit_site_domain",
                                         reason="matched perturbation created duplicate sites")
            else:
                answer2, trees2, ids2 = project(args, name + "_jitter", perturbed, ids, labels, config, True)
                initial["jitter"] = dict(check=jitter_check(trees, trees2, native_ids, ids2,
                                                           config["jitter_pair_seed"]),
                                         alternate=answer2)
        initial["status"] = "ok"
        save(path, initial)
        print(json.dumps(dict(name=name, status="ok", sites=len(ids)), sort_keys=True), flush=True)
    except Exception as error:
        initial.update(status="failed", error=type(error).__name__ + ": " + str(error))
        save(path, initial)
        raise


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--data", type=Path)
    parser.add_argument("--mode", choices=("synthetic", "zoltan"), required=True)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=False)
    args.out.mkdir(parents=True, exist_ok=True)
    config = json.loads(CONFIG.read_text())
    save(args.out / "campaign.json", config)
    if args.mode == "synthetic":
        from vendor_scenes import generate, quantize
        for spec in config["synthetic"]:
            points, labels, meta = generate(spec)
            xyz, grid = quantize(points, millimetre=0.001, bits=21)
            xyz = xyz + 2  # Same isometric translation for every method, jitter margin.
            name = spec["family"] + "_" + str(spec["seed"])
            meta.update(grid_m=grid, generator_sha256=digest(HERE / "vendor_scenes.py"),
                        generator_source=config["generator_source"], subsampling="none")
            one_case(args, name, xyz, np.arange(len(xyz), dtype="<u4"), labels, meta, config)
    else:
        need(args.data is not None, "Zoltan requires external data")
        manifest = json.loads((args.data / "manifest.json").read_text())
        need(digest(args.data / "manifest.json") == config["zoltan_manifest_sha256"], "input manifest pin")
        for scene in manifest["scenes"]:
            paths = {}
            for entry in scene["files"]:
                path = args.data / entry["name"]
                need(path.stat().st_size == entry["bytes"] and digest(path) == entry["sha256"], "input payload pin")
                paths[entry["role"]] = path
            n = scene["n"]
            xyz = np.fromfile(paths["sites"], dtype="<u4").reshape(n, 3)
            ids = np.fromfile(paths["ids"], dtype="<u4")
            truth = np.fromfile(paths["truth"], dtype="<i4").reshape(2, n)
            labels = truth[1].copy()
            labels[truth[0] == -2] = -2
            need([int((labels == i).sum()) for i in range(3)] ==
                 [entry["sites"] for entry in scene["targets"]], "target size mapping")
            one_case(args, "zoltan_" + scene["name"], xyz, ids, labels, scene, config)


if __name__ == "__main__":
    main()
