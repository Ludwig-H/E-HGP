#!/usr/bin/env python3
"""Small, predeclared point-clustering corpus; no tuning and no hidden merges.

Raw downloads stay outside the repository. ``prepare(root)`` is local-only.
FCPS conversion needs rdata==1.0.0; everything else needs only numpy.
"""
from __future__ import annotations

import argparse
from fractions import Fraction
import hashlib
import json
import math
from pathlib import Path

import numpy as np

LIMIT = (1 << 18) - 1
FCPS_COMMIT = "c9c55f4a6162b32d9d34de8b39011a8c254dda6a"
FCPS_BASE = f"https://raw.githubusercontent.com/Mthrun/FCPS/{FCPS_COMMIT}/data"
PUBLIC = (
    ("fcps_hepta", "Hepta", 212, 3, "development", "dc1eb7a1da3de0a28003cae69355be112758d050ec44c3d76aaa2b340c765456"),
    ("fcps_tetra", "Tetra", 400, 3, "development", "496ec6a0a43c21ef5283729120660bc8bd46c7f6e1409f47a5e754c9b7206c1a"),
    ("fcps_atom", "Atom", 800, 3, "evaluation", "8270f8a7f20ab7d4794d05471661d7519945c65c2fd50ded4b928c731f2131e9"),
    ("fcps_chainlink", "Chainlink", 1000, 3, "evaluation", "a180d1e0b087308bf46e04d2c16372ee52852c9757de5e13e79ae59fe1e95564"),
)
# SIPU files are x,y,label, NOT three-dimensional coordinates.
SIPU = (
    ("sipu_flame", "flame", 240, "development", "2523942f59388e428580e98974189fcdc29ecac71bb861b41ec76d422669ed93"),
    ("sipu_spiral", "spiral", 312, "evaluation", "5f0ae012e6c25d469e9b4485fb5cb0bc3f5c1c29f3163be2bc66e2a562a0bbe6"),
)
FAMILIES = ("varied_density", "linked_rings", "bridge_noise")
SEEDS = {"development": 2026092701, "evaluation": 2026092702}
PLAN = {
    "schema": "point-clustering-plan-v1",
    "cases": [x[0] for x in PUBLIC] + [x[0] for x in SIPU]
    + [f"synthetic_{family}_{split}" for family in FAMILIES for split in SEEDS],
    "k": [2, 5, 10], "min_cluster_size": [20, 50], "z": [1, 2],
    "k1": "sanity only on small cases",
    "seeds": SEEDS, "synthetic_n": 900,
    "selection": "fixed before clustering scores; no selection using truth labels",
    "split_unit": "whole scene, never subsampled points",
}


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def json_bytes(obj):
    return (json.dumps(obj, sort_keys=True, indent=2, allow_nan=False) + "\n").encode()


def write_once(path, payload):
    """Idempotent only for identical bytes; refuse any overwrite."""
    path = Path(path)
    if path.exists():
        if path.read_bytes() != payload:
            raise ValueError(f"existing file differs: {path}")
        return
    with path.open("xb") as out:
        out.write(payload)


def normalize_labels(values):
    labels = np.asarray(values)
    if labels.ndim != 1 or not np.all(np.isfinite(labels)):
        raise ValueError("labels must be a finite vector")
    if np.any(labels != np.floor(labels)) or np.any(labels < -1):
        raise ValueError("labels must be integers >= -1")
    # In these named sources/generators only, zero denotes noise/unassigned.
    return np.where(labels == 0, -1, labels).astype(np.int64)


def quantize(points, labels):
    """One isotropic u18 grid, exact rational rounding of input binary64.

    No label enters scale/origin/rounding. Every duplicate is refused, even
    same-label exact duplicates: there is no implicit dropping or fusion.
    """
    points = np.asarray(points, dtype=np.float64)
    labels = normalize_labels(labels)
    if points.ndim != 2 or points.shape[1] not in (2, 3) or len(points) < 2:
        raise ValueError("expected n>=2, dimension 2 or 3")
    if not np.all(np.isfinite(points)) or len(labels) != len(points):
        raise ValueError("nonfinite coordinates or label length mismatch")
    dimension = points.shape[1]
    xyz = np.zeros((len(points), 3), dtype=np.float64)
    xyz[:, :dimension] = points
    exact = [[Fraction(float(x)) for x in row] for row in xyz]
    origin = [min(row[d] for row in exact) for d in range(3)]
    spans = [max(row[d] for row in exact) - origin[d] for d in range(3)]
    span = max(spans)
    if span == 0:
        raise ValueError("zero-diameter cloud")
    step = span / LIMIT
    q = np.asarray([[round((x - origin[d]) / step) for d, x in enumerate(row)]
                    for row in exact], dtype="<u4")
    raw_seen, grid_seen = {}, {}
    raw_duplicate_rows = grid_duplicate_rows = contradictory = 0
    for i, (raw, grid) in enumerate(zip(map(tuple, xyz), map(tuple, q))):
        raw_duplicate_rows += raw in raw_seen
        raw_seen[raw] = i
        if grid in grid_seen:
            grid_duplicate_rows += 1
            contradictory += int(labels[i] != labels[grid_seen[grid]])
        grid_seen[grid] = i
    if grid_duplicate_rows:
        raise ValueError(f"duplicate sites refused: raw={raw_duplicate_rows}, "
                         f"grid={grid_duplicate_rows}, contradictory_labels={contradictory}")
    errors = [[abs(origin[d] + int(q[i, d]) * step - row[d])
               for d in range(3)] for i, row in enumerate(exact)]
    maximum = max(x for row in errors for x in row)
    if maximum > step / 2 or np.any(q > LIMIT):
        raise ValueError("quantization bound violated")
    metadata = {
        "scheme": "isotropic-u18-exact-rational-nearest-ties-to-even-v1",
        "limit": LIMIT, "origin_exact": [str(x) for x in origin],
        "step_exact": str(step), "step": float(step),
        "max_abs_error_exact": str(maximum), "max_abs_error": float(maximum),
        "max_euclidean_error": max(math.sqrt(sum(float(x)**2 for x in row)) for row in errors),
        "euclidean_error_bound": math.sqrt(3) * float(step) / 2,
        "raw_duplicate_rows": raw_duplicate_rows,
        "grid_duplicate_rows": grid_duplicate_rows,
        "new_grid_collision_rows": grid_duplicate_rows - raw_duplicate_rows,
        "contradictory_labels": contradictory, "merged_rows": 0,
        "dimension": dimension, "embedding": "z=0" if dimension == 2 else "native-3d",
        "method_coordinates": "integer grid coordinates, float64 exactly identical to u32",
    }
    return q, labels, metadata


def synthetic(family, seed):
    """900 points; generator parameters fixed, no rejection using truth/metrics."""
    if family not in FAMILIES:
        raise ValueError(f"unknown synthetic family: {family}")
    rng = np.random.Generator(np.random.PCG64(seed))
    if family == "varied_density":
        centers = np.asarray([[-2., 0., 0.], [0., 2., 1.], [2., 0., -1.]])
        points = np.concatenate([rng.normal(c, sigma, (300, 3))
                                 for c, sigma in zip(centers, (.08, .22, .5))])
        labels = np.repeat([1, 2, 3], 300)
    elif family == "linked_rings":
        a, b = rng.uniform(0, 2 * np.pi, (2, 450))
        first = np.column_stack((np.cos(a), np.sin(a), np.zeros(450)))
        second = np.column_stack((1 + np.cos(b), np.zeros(450), np.sin(b)))
        points = np.concatenate((first, second)) + rng.normal(0, .04, (900, 3))
        labels = np.repeat([1, 2], 450)
    else:
        left = rng.normal([-2., 0., 0.], .25, (350, 3))
        right = rng.normal([2., 0., 0.], .25, (350, 3))
        bridge = np.column_stack((rng.uniform(-1.5, 1.5, 80), rng.normal(0, .06, (80, 2))))
        noise = rng.uniform([-3., -1.5, -1.5], [3., 1.5, 1.5], (120, 3))
        points = np.concatenate((left, right, bridge, noise))
        labels = np.concatenate((np.repeat([1, 2], 350), np.zeros(200, dtype=int)))
    return points, labels


def load_public(root, case_id):
    raw = Path(root) / "raw"
    for ident, name, n, dim, split, expected in PUBLIC:
        if ident != case_id:
            continue
        path = raw / f"{name}.rda"
        if digest(path) != expected:
            raise ValueError(f"source SHA256 mismatch: {path}")
        import rdata
        obj = rdata.read_rda(path)[name]
        points, labels = np.asarray(obj["Data"]), np.asarray(obj["Cls"])
        if points.shape != (n, dim) or labels.shape != (n,):
            raise ValueError(f"FCPS shape mismatch: {name}")
        return points, labels, dict(name=name, split=split, source_url=f"{FCPS_BASE}/{name}.rda",
                                   source_sha256=expected, source_file=str(path.resolve()),
                                   collection="FCPS", source_commit=FCPS_COMMIT)
    for ident, name, n, split, expected in SIPU:
        if ident != case_id:
            continue
        path = raw / f"sipu_{name}_browser.txt"
        provenance = raw / f"sipu_{name}_browser_provenance.txt"
        if digest(path) != expected:
            raise ValueError(f"normalized SIPU source SHA256 mismatch: {path}")
        provenance_text = provenance.read_text()
        if f"https://cs.uef.fi/sipu/datasets/{name}.txt" not in provenance_text:
            raise ValueError("SIPU official browser provenance missing")
        data = np.loadtxt(path)
        if data.shape != (n, 3):
            raise ValueError(f"SIPU shape mismatch: {name}; expected x,y,label")
        return data[:, :2], data[:, 2], dict(name=name, split=split, collection="SIPU",
            source_url=f"https://cs.uef.fi/sipu/datasets/{name}.txt",
            source_sha256=expected, source_file=str(path.resolve()),
            source_encoding="official browser numerical rows, LF normalized; not byte-identical HTTP archive",
            source_provenance=str(provenance.resolve()), source_provenance_sha256=digest(provenance))
    raise ValueError(f"unknown public case: {case_id}")


def prepare(root, case_ids=None):
    """Prepare locally; lock plan before loading labels; reuse only exact bytes.

    Optional explicit subsets use manifest_<ids hash>.json, never masquerade as
    the full 12-case manifest. No download, metric, fitting or parameter search.
    """
    import io
    root = Path(root).resolve()
    root.mkdir(parents=True, exist_ok=True)
    write_once(root / "plan.json", json_bytes(PLAN))
    selected = list(PLAN["cases"] if case_ids is None else case_ids)
    if not selected or len(set(selected)) != len(selected) or not set(selected) <= set(PLAN["cases"]):
        raise ValueError("empty, duplicate or unknown selected cases")
    manifest = {"schema": "point-clustering-datasets-v1", "plan": PLAN,
                "plan_sha256": digest(root / "plan.json"), "complete": selected == PLAN["cases"],
                "numpy_version": np.__version__, "cases": []}
    for ident in selected:
        if ident.startswith("synthetic_"):
            family, split = ident[len("synthetic_"):].rsplit("_", 1)
            seed = SEEDS[split]
            points, labels = synthetic(family, seed)
            source_bytes = np.asarray(points, dtype="<f8").tobytes() + np.asarray(labels, dtype="<i8").tobytes()
            source = dict(name=ident, split=split, collection="synthetic", source_url=None,
                          family=family, seed=seed,
                          source_sha256=hashlib.sha256(source_bytes).hexdigest(),
                          source_hash_encoding="C-order little-endian float64 XYZ then int64 raw labels")
        else:
            points, labels, source = load_public(root, ident)
        q, truth, grid = quantize(points, labels)
        destination = root / "prepared" / ident
        destination.mkdir(parents=True, exist_ok=True)
        paths = {"points_u32le": destination / "points.u32le",
                 "points_npy": destination / "points.npy", "labels_json": destination / "labels.json"}
        stream = io.BytesIO()
        np.save(stream, q.astype(np.float64), allow_pickle=False)
        write_once(paths["points_u32le"], q.tobytes(order="C"))
        write_once(paths["points_npy"], stream.getvalue())
        write_once(paths["labels_json"], json_bytes(truth.tolist()))
        case = dict(id=ident, n=len(q), dimension=points.shape[1], quantization=grid, **source)
        case.update({key: str(path) for key, path in paths.items()})
        case["prepared_sha256"] = {key: digest(path) for key, path in paths.items()}
        case["noise_label"] = -1
        case["noise_count"] = int(np.count_nonzero(truth == -1))
        write_once(destination / "case.json", json_bytes(case))
        manifest["cases"].append(case)
    suffix = "" if manifest["complete"] else "_" + hashlib.sha256(json_bytes(selected)).hexdigest()[:12]
    manifest_path = root / f"manifest{suffix}.json"
    manifest["manifest_path"] = str(manifest_path)
    write_once(manifest_path, json_bytes(manifest))
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    parser.add_argument("--cases", nargs="+", choices=PLAN["cases"])
    args = parser.parse_args()
    result = prepare(args.root, args.cases)
    print(json.dumps({"manifest_path": result["manifest_path"], "complete": result["complete"],
                      "cases": [{key: c[key] for key in ("id", "n", "dimension", "split")}
                                for c in result["cases"]]}, sort_keys=True))


if __name__ == "__main__":
    main()
