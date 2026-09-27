#!/usr/bin/env python3
"""Predeclared whole-instance, fixed-K clustering experiment; CPU only.

Raw benchmark inputs and full native exports stay outside the repository.
No parameter search using labels, no cloud calls, no subsampling.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import subprocess
import sys
import time
import traceback

import numpy as np
from sklearn.metrics import adjusted_rand_score, normalized_mutual_info_score
from threadpoolctl import threadpool_limits

from eom import (condense_eom, equivalent_labels, fit_hdbscan, sklearn_provenance,
                 _validated_tree, _atomize)
from projection import SourceTree

HERE = Path(__file__).resolve().parent
KS, SIZES, EXPONENTS = (2, 5, 10), (20, 50), (1, 2)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def clean(value):
    if isinstance(value, float) and not math.isfinite(value):
        if math.isnan(value):
            raise ValueError("NaN result")
        return "+infinity" if value > 0 else "-infinity"
    if isinstance(value, dict):
        return {str(k): clean(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [clean(v) for v in value]
    return value


def save(path, value):
    Path(path).write_text(json.dumps(clean(value), indent=2, sort_keys=True, allow_nan=False) + "\n")


def metrics(truth, labels):
    truth, labels = np.asarray(truth, dtype=int), np.asarray(labels, dtype=int)
    if truth.ndim != 1 or labels.shape != truth.shape or len(truth) == 0:
        raise ValueError("invalid evaluation arrays")
    mask, retained = truth >= 0, labels >= 0
    split_noise = labels[mask].copy()
    next_label = int(max(split_noise.max(initial=-1), -1)) + 1
    for i in np.flatnonzero(split_noise < 0):
        split_noise[i] = next_label
        next_label += 1
    result = dict(ari_all=float(adjusted_rand_score(truth, labels)),
                  nmi_all=float(normalized_mutual_info_score(truth, labels)),
                  clusters=len(set(labels[retained].tolist())),
                  coverage=float(np.mean(retained)), noise_count=int(np.sum(~retained)),
                  ari_true_inliers=float(adjusted_rand_score(truth[mask], labels[mask]))
                  if np.sum(mask) >= 2 else None,
                  ari_inliers_noise_singletons=float(adjusted_rand_score(truth[mask], split_noise))
                  if np.sum(mask) >= 2 else None,
                  ari_classified=float(adjusted_rand_score(truth[retained], labels[retained]))
                  if np.sum(retained) >= 2 else None)
    if np.any(~mask):
        tp, fp, fn = int(np.sum(~mask & ~retained)), int(np.sum(mask & ~retained)), int(np.sum(~mask & retained))
        result.update(noise_precision=tp / (tp + fp) if tp + fp else 0.0,
                      noise_recall=tp / (tp + fn),
                      noise_f1=2 * tp / (2 * tp + fp + fn) if 2 * tp + fp + fn else 0.0)
    else:
        result.update(noise_precision=None, noise_recall=None, noise_f1=None)
    return result


def dendrogram_purity(tree, truth):
    """Exact combinatorial LCA purity after equal-height atomization, noise excluded.

    Mean over all pairs in a common true class of that class's proportion among
    inlier leaves below their LCA. Label permutations and EOM do not affect it.
    O(number_of_classes * number_of_nodes), intended for these small benchmarks.
    """
    n, original, height, root, _, _ = _validated_tree(tree["n"], tree["children"], tree["heights"])
    atomic, _ = _atomize(original, height, root)
    truth = np.asarray(truth, dtype=int)
    if len(truth) != n:
        raise ValueError("truth size")
    classes = sorted(set(truth[truth >= 0].tolist()))
    index = {label: i for i, label in enumerate(classes)}
    order, stack = [], [root]
    while stack:
        node = stack.pop()
        order.append(node)
        stack.extend(atomic.get(node, []))
    counts, terms, denominator = {}, [], 0
    for node in reversed(order):
        if node < n:
            counts[node] = np.zeros(len(classes), dtype=np.int64)
            if truth[node] >= 0:
                counts[node][index[int(truth[node])]] = 1
        else:
            count = sum((counts[c] for c in atomic[node]), start=np.zeros(len(classes), dtype=np.int64))
            pairs = count * (count - 1) // 2
            for c in atomic[node]:
                pairs -= counts[c] * (counts[c] - 1) // 2
            if int(count.sum()):
                terms.extend((pairs * count / int(count.sum())).tolist())
            denominator += int(pairs.sum())
            counts[node] = count
    return math.fsum(terms) / denominator if denominator else None


def validate_case(case):
    points = np.load(case["points_npy"], allow_pickle=False)
    binary = np.fromfile(case["points_u32le"], dtype="<u4").reshape((-1, 3))
    truth = np.asarray(json.loads(Path(case["labels_json"]).read_text()), dtype=int)
    if not np.array_equal(points, binary) or points.shape != (case["n"], 3) or truth.shape != (case["n"],):
        raise ValueError("same-coordinate/label/count gate")
    if len(np.unique(binary, axis=0)) != len(binary) or binary.max() >= 2**18:
        raise ValueError("collisions or coordinates outside u18")
    return binary, truth


def experiment(args):
    output = args.output.resolve()
    if output.exists():
        raise ValueError("new output directory required; failures must not be overwritten")
    output.mkdir(parents=True)
    args.run_created = True
    manifest = json.loads(args.manifest.read_text())
    cases = manifest["cases"]
    source_paths = sorted(HERE.glob("*.py")) + [HERE / "native_export.cpp", HERE / "README.md",
                                               HERE / "FAIRNESS.md", HERE / "DATASETS.md"]
    sources = {str(p): sha(p) for p in source_paths}
    receipt = dict(schema="mhgp9_fixed_k_clustering_benchmark_v1", status="running",
                   input_manifest=str(args.manifest.resolve()), input_manifest_sha256=sha(args.manifest),
                   native_binary=str(args.native.resolve()), native_binary_sha256=sha(args.native),
                   sources_before=sources, GCP_used=False, GPU_used=False, geometry_engine_modified=False,
                   fixed_k_only=True, primary=dict(k=5, min_cluster_size=20, exp_z=1, method="first_coverage"),
                   parameter_grid=dict(k=list(KS), min_cluster_size=list(SIZES), exp_z=list(EXPONENTS)),
                   host=dict(platform=platform.platform(), python=sys.version, affinity=sorted(os.sched_getaffinity(0))),
                   sklearn=sklearn_provenance(), commands=[], failures=[], rows=[])
    save(output / "receipt.json", receipt)
    began = time.perf_counter()
    for case in cases:
        points, truth = validate_case(case)
        input_hashes = {key: sha(case[key]) for key in ("points_u32le", "points_npy", "labels_json")}
        for k in KS:
            name = f"{case['id']}_k{k}"
            print(f"START {name} n={len(points)}", flush=True)
            path = output / name
            path.mkdir()
            command = [str(args.native.resolve()), "--input", case["points_u32le"], "--k", str(k), "--workers", str(args.workers)]
            start = time.perf_counter()
            with (path / "native.json").open("wb") as out, (path / "native.stderr").open("wb") as err:
                completed = subprocess.run(command, stdout=out, stderr=err, check=False)
            wall = (time.perf_counter() - start) * 1000
            command_receipt = dict(argv=command, returncode=completed.returncode, wall_ms=wall,
                                   stdout_sha256=sha(path / "native.json"), stderr_sha256=sha(path / "native.stderr"),
                                   input_sha256=input_hashes)
            receipt["commands"].append(command_receipt)
            save(path / "command.json", command_receipt)
            try:
                if completed.returncode:
                    raise RuntimeError("native exporter failed")
                start = time.perf_counter()
                native = json.loads((path / "native.json").read_text())
                if native["k"] != k or native["point_count"] != len(points) or not np.array_equal(native["points"], points):
                    raise ValueError("native command/input mismatch")
                source = SourceTree(native)
                read_ms = (time.perf_counter() - start) * 1000
                projections = {}
                for method in ("first_coverage", "entry_vote"):
                    for z in EXPONENTS:
                        start = time.perf_counter()
                        projected = source.project(method, z)
                        project_ms = (time.perf_counter() - start) * 1000
                        projections[(method, z)] = projected, project_ms
                        save(path / f"{method}_z{z}_tree.json", projected)
                # Every geometry and projection is complete BEFORE truth is used below.
                for size in SIZES:
                    start = time.perf_counter()
                    fitted = fit_hdbscan(points, k=k, min_cluster_size=size, exp_z=1)
                    fit_ms = (time.perf_counter() - start) * 1000
                    save(path / f"hdbscan_m{size}.json", fitted)
                    hb_purity = dendrogram_purity(fitted["tree"], truth)
                    for z in EXPONENTS:
                        methods = []
                        for method in ("first_coverage", "entry_vote"):
                            projected, project_ms = projections[(method, z)]
                            tree = projected["tree"]
                            start = time.perf_counter()
                            selection = condense_eom(tree["n"], tree["children"], tree["heights"],
                                                     min_cluster_size=size, exp_z=z, atomize_ties=True)
                            select_ms = (time.perf_counter() - start) * 1000
                            methods.append((method, selection, dict(
                                native_process_wall_ms=wall, native_chain_ms=native["times_ms"]["native_chain_wall"],
                                source_read_validate_ms=read_ms, projection_ms=project_ms, selection_ms=select_ms,
                                dendrogram_purity=dendrogram_purity(tree, truth),
                                projection_statistics=projected["statistics"])))
                        start = time.perf_counter()
                        hb = fitted["common"] if z == 1 else condense_eom(
                            **fitted["tree"], min_cluster_size=size, exp_z=z, atomize_ties=True)
                        hb_ms = (time.perf_counter() - start) * 1000
                        methods.append(("hdbscan_common", hb, dict(hdbscan_fit_and_checks_ms=fit_ms,
                            additional_z2_selection_ms=hb_ms if z == 2 else 0,
                            common_z1_matches_standard=fitted["common_z1_matches_standard"],
                            preserved_tree_z1_matches_standard=fitted["preserved_tree_z1_matches_standard"],
                            dendrogram_purity=hb_purity)))
                        if z == 1:
                            methods.append(("hdbscan_standard", dict(labels=fitted["standard_labels_z1"], warnings=[]),
                                            dict(hdbscan_fit_and_checks_ms=fit_ms, dendrogram_purity=hb_purity)))
                        for method, selected, extra in methods:
                            row = dict(case=case["id"], dimension=case["dimension"], split=case["split"],
                                       n=len(points), k=k, min_cluster_size=size, exp_z=z, method=method,
                                       metrics=metrics(truth, selected["labels"]), warnings=selected["warnings"], **extra)
                            save(path / f"{method}_m{size}_z{z}_eom.json", selected)
                            receipt["rows"].append(row)
                if any(sha(case[key]) != value for key, value in input_hashes.items()):
                    raise ValueError("dataset mutated during run")
                print(f"DONE {name} native={wall:.1f}ms source_nodes={len(source.children)}", flush=True)
            except Exception as error:
                receipt["failures"].append(dict(case=case["id"], k=k, error=repr(error), traceback=traceback.format_exc()))
                print(f"FAIL {name}: {error}", flush=True)
            save(output / "receipt.json", receipt)
    receipt["sources_after"] = {str(p): sha(p) for p in source_paths}
    receipt["native_binary_sha256_after"] = sha(args.native)
    receipt["elapsed_seconds"] = time.perf_counter() - began
    stable = (receipt["sources_before"] == receipt["sources_after"] and
              receipt["native_binary_sha256"] == receipt["native_binary_sha256_after"] and
              receipt["input_manifest_sha256"] == sha(args.manifest))
    receipt["status"] = "completed" if stable and not receipt["failures"] else "failed"
    save(output / "receipt.json", receipt)
    flat = []
    for row in receipt["rows"]:
        flat.append({**{key: row[key] for key in ("case", "dimension", "split", "n", "k", "min_cluster_size", "exp_z", "method")},
                     **row["metrics"], "dendrogram_purity": row["dendrogram_purity"]})
    if flat:
        with (output / "scores.csv").open("w", newline="") as handle:
            writer = csv.DictWriter(handle, fieldnames=list(flat[0]))
            writer.writeheader()
            writer.writerows(flat)
    print(json.dumps(dict(status=receipt["status"], rows=len(receipt["rows"]), failures=len(receipt["failures"]),
                          output=str(output)), sort_keys=True), flush=True)
    return 0 if receipt["status"] == "completed" else 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--native", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--workers", type=int, default=4)
    args = parser.parse_args()
    if args.workers < 1:
        parser.error("positive workers required")
    try:
        with threadpool_limits(limits=1):
            result_code = experiment(args)
    except BaseException as error:
        # Preserve interruption or I/O failure too, never leave a successful or
        # apparently live receipt after an exceptional exit. Do not touch an
        # existing directory rejected before this invocation created its run.
        receipt_path = args.output.resolve() / "receipt.json"
        if getattr(args, "run_created", False) and receipt_path.exists():
            receipt = json.loads(receipt_path.read_text())
            if receipt.get("status") == "running":
                receipt["status"] = "failed"
                receipt["fatal_error"] = dict(error=repr(error), traceback=traceback.format_exc())
                save(receipt_path, receipt)
        raise
    sys.exit(result_code)
