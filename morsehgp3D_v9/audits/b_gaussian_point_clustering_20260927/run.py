#!/usr/bin/env python3
"""Fixed Gaussian experiment; one source tree, many minimum-size condensations."""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
import traceback

HERE = Path(__file__).resolve().parent
OLD = HERE.parent / "b_point_hierarchy_k_20260927"
sys.path.insert(1, str(OLD))

import numpy as np
from threadpoolctl import threadpool_limits
from benchmark import clean, dendrogram_purity, metrics
from eom import equivalent_labels, fit_hdbscan, sklearn_provenance
from projection import SourceTree
from condensed import point_clusterer_from_tree
from evaluation import evaluate_labels, tree_recoverability, condensed_recoverability

KS = (5, 10)
SIZES = (10, 20, 50, 100)
EXPONENTS = (1, 2)
EXPECTED_BINARY = "897a715a5b40b7fa298aaf66696def3a5cf636c98118db69da2684277cd11dd8"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    Path(path).write_text(json.dumps(clean(value), sort_keys=True, indent=2, allow_nan=False) + "\n")


def tree_digest(tree):
    return hashlib.sha256(json.dumps(tree, sort_keys=True, allow_nan=False).encode()).hexdigest()


def read_case(case):
    for key, expected in case["prepared_sha256"].items():
        if sha(case[key]) != expected:
            raise ValueError("changed prepared data " + key)
    for key, expected in case["diagnostic_sha256"].items():
        if sha(case[key]) != expected:
            raise ValueError("changed diagnostic data " + key)
    points = np.load(case["points_npy"], allow_pickle=False)
    binary = np.fromfile(case["points_u32le"], dtype="<u4").reshape((-1, 3))
    truth = json.loads(Path(case["labels_json"]).read_text())
    if not np.array_equal(points, binary) or points.shape != (case["n"], 3) or len(truth) != len(points):
        raise ValueError("whole shared geometry mismatch")
    if any(type(x) is not int or x < 1 for x in truth) or set(truth) != set(range(1, case["communities"] + 1)):
        raise ValueError("invalid Gaussian truth")
    if binary.max() >= 2**18 or len(np.unique(binary, axis=0)) != len(points):
        raise ValueError("invalid distinct u18 input")
    return points, binary, truth


def checked_inputs(case):
    paths = {key: case[key] for key in case["prepared_sha256"]}
    paths.update(parameters_json=case["parameters_json"], bayes_map_json=case["bayes_map_json"])
    return {str(Path(path).resolve()): sha(path) for path in paths.values()}


def experiment(args):
    if args.output.exists():
        raise ValueError("fresh output required; do not overwrite a capture")
    args.output.mkdir(parents=True)
    args.run_created = True
    manifest = json.loads(args.manifest.read_text())
    if len(manifest["cases"]) != 48 or not manifest["complete"]:
        raise ValueError("complete predeclared 48-scene corpus required")
    if sha(args.native) != EXPECTED_BINARY:
        raise ValueError("native exporter differs from explicitly inherited build")
    sources = sorted(HERE.glob("*.py")) + sorted(HERE.glob("*.md")) + [
        OLD / name for name in ("eom.py", "projection.py", "benchmark.py", "datasets.py", "native_export.cpp")]
    receipt = dict(schema="mhgp9_gaussian_condensation_benchmark_v1", status="running", rows=[], failures=[], commands=[],
                   manifest_path=str(args.manifest.resolve()), manifest_sha256=sha(args.manifest),
                   native_binary=str(args.native.resolve()), native_binary_sha256=sha(args.native),
                   sources_before={str(p): sha(p) for p in sources}, sklearn=sklearn_provenance(),
                   GCP_used=False, GPU_used=False, engine_modified=False, fixed_k_only=True,
                   primary=dict(k=5, min_cluster_size=20, exp_z=1),
                   grid=dict(k=list(KS), min_cluster_size=list(SIZES), exp_z=list(EXPONENTS)),
                   input_hashes={}, case_diagnostics=[])
    save(args.output / "receipt.json", receipt)
    start_run = time.perf_counter()
    for case in manifest["cases"]:
        points, binary, truth = read_case(case)
        pins = checked_inputs(case)
        receipt["input_hashes"].update(pins)
        bayes = json.loads(Path(case["bayes_map_json"]).read_text())
        map_labels = bayes["predictions_reconstructed_grid"]
        receipt["case_diagnostics"].append(dict(case=case["id"], scope="known_parameter_MAP_not_clustering_baseline",
            metrics=metrics(truth, map_labels), extra=evaluate_labels(truth, map_labels),
            empirical_accuracy=bayes["empirical_accuracy_reconstructed_grid"],
            quantization_MAP_changes=bayes["grid_prediction_changes"]))
        for k in KS:
            name = case["id"] + "_k" + str(k)
            directory = args.output / name
            directory.mkdir()
            command = [str(args.native.resolve()), "--input", case["points_u32le"], "--k", str(k), "--workers", str(args.workers)]
            print("START", name, flush=True)
            start = time.perf_counter()
            with (directory / "native.json").open("wb") as out, (directory / "native.stderr").open("wb") as err:
                completed = subprocess.run(command, stdout=out, stderr=err, check=False)
            native_wall = (time.perf_counter() - start) * 1000
            command_receipt = dict(case=case["id"], k=k, argv=command, returncode=completed.returncode,
                wall_ms=native_wall, stdout_sha256=sha(directory / "native.json"),
                stderr_sha256=sha(directory / "native.stderr"), input_sha256=sha(case["points_u32le"]))
            receipt["commands"].append(command_receipt)
            save(directory / "command.json", command_receipt)
            try:
                if completed.returncode:
                    raise RuntimeError("native exporter failed")
                start = time.perf_counter()
                native = json.loads((directory / "native.json").read_text())
                if native["k"] != k or native["point_count"] != case["n"] or not np.array_equal(native["points"], binary):
                    raise ValueError("native command/geometry mismatch")
                source = SourceTree(native)
                source_ms = (time.perf_counter() - start) * 1000
                start = time.perf_counter()
                projection = source.project("first_coverage", 1)
                projection_ms = (time.perf_counter() - start) * 1000
                hg_tree = projection["tree"]
                save(directory / "hgp_tree.json", projection)
                hg_purity = dendrogram_purity(hg_tree, truth)
                hg_recover = tree_recoverability(truth, hg_tree)
                hb_digest = None
                for size in SIZES:
                    start = time.perf_counter()
                    fitted = fit_hdbscan(points, k=k, min_cluster_size=size, exp_z=1)
                    hb_ms = (time.perf_counter() - start) * 1000
                    digest = tree_digest(fitted["tree"])
                    if hb_digest is not None and hb_digest != digest:
                        raise ValueError("HDBSCAN source tree changed when only min_cluster_size changed")
                    hb_digest = digest
                    save(directory / f"hdbscan_m{size}.json", fitted)
                    hb_purity = dendrogram_purity(fitted["tree"], truth)
                    hb_recover = tree_recoverability(truth, fitted["tree"])
                    for z in EXPONENTS:
                        for method, tree, purity, recover in (
                            ("hgp_first_coverage", hg_tree, hg_purity, hg_recover),
                            ("hdbscan_common", fitted["tree"], hb_purity, hb_recover)):
                            start = time.perf_counter()
                            clustered = point_clusterer_from_tree(tree, min_cluster_size=size, exp_z=z)
                            cluster_ms = (time.perf_counter() - start) * 1000
                            if method == "hdbscan_common" and z == 1 and not equivalent_labels(
                                clustered["selection"]["labels"], fitted["common_z1_labels"]):
                                raise ValueError("new condensation wrapper changed common EOM")
                            save(directory / f"{method}_m{size}_z{z}.json", clustered)
                            labels = clustered["selection"]["labels"]
                            row = dict(case=case["id"], n=case["n"], communities=case["communities"],
                                regime=case["regime"], separation=case["separation"], seed=case["seed"],
                                k=k, min_cluster_size=size, exp_z=z, method=method,
                                metrics=metrics(truth, labels), extra=evaluate_labels(truth, labels, min_cluster_size=size),
                                dendrogram_purity=purity, raw_recoverability=recover,
                                raw_size_filtered_recoverability=tree_recoverability(truth, tree, min_cluster_size=size),
                                condensed_recoverability=condensed_recoverability(truth, clustered["condensed_tree"]),
                                condensation_stats=clustered["stats"], condensation_and_selection_ms=cluster_ms,
                                warnings=clustered["selection"].get("warnings", []))
                            if method == "hgp_first_coverage":
                                row.update(native_process_wall_ms=native_wall,
                                    native_chain_ms=native["times_ms"]["native_chain_wall"],
                                    source_read_validate_ms=source_ms, projection_ms=projection_ms,
                                    projection_stats=projection["statistics"])
                            else:
                                row.update(hdbscan_fit_and_checks_ms=hb_ms,
                                    common_z1_matches_standard=fitted["common_z1_matches_standard"],
                                    preserved_tree_z1_matches_standard=fitted["preserved_tree_z1_matches_standard"])
                            receipt["rows"].append(row)
                    standard = fitted["standard_labels_z1"]
                    receipt["rows"].append(dict(case=case["id"], n=case["n"], communities=case["communities"],
                        regime=case["regime"], separation=case["separation"], seed=case["seed"],
                        k=k, min_cluster_size=size, exp_z=1, method="hdbscan_standard",
                        metrics=metrics(truth, standard), extra=evaluate_labels(truth, standard, min_cluster_size=size),
                        dendrogram_purity=hb_purity, warnings=[]))
                if any(sha(path) != expected for path, expected in pins.items()):
                    raise ValueError("case inputs changed")
                print(f"DONE {name} native={native_wall:.1f}ms source_nodes={len(source.children)}", flush=True)
            except Exception as error:
                receipt["failures"].append(dict(case=case["id"], k=k, error=repr(error), traceback=traceback.format_exc()))
                print("FAIL", name, repr(error), flush=True)
            save(args.output / "receipt.json", receipt)
    receipt["sources_after"] = {str(p): sha(p) for p in sources}
    receipt["native_binary_sha256_after"] = sha(args.native)
    receipt["elapsed_seconds"] = time.perf_counter() - start_run
    stable = (receipt["sources_before"] == receipt["sources_after"] and
              receipt["native_binary_sha256"] == receipt["native_binary_sha256_after"] and
              sha(args.manifest) == receipt["manifest_sha256"] and
              all(sha(path) == expected for path, expected in receipt["input_hashes"].items()))
    receipt["status"] = "completed" if stable and not receipt["failures"] and len(receipt["rows"]) == 1920 else "failed"
    save(args.output / "receipt.json", receipt)
    # Scalar table for independent analysis; the full hierarchies remain separate.
    rows = []
    for row in receipt["rows"]:
        flat = {key: row[key] for key in ("case", "n", "communities", "regime", "separation", "seed", "k", "min_cluster_size", "exp_z", "method")}
        flat.update(row["metrics"], dendrogram_purity=row["dendrogram_purity"])
        rows.append(flat)
    if rows:
        with (args.output / "scores.csv").open("w", newline="") as handle:
            writer = csv.DictWriter(handle, fieldnames=list(rows[0]), lineterminator="\n")
            writer.writeheader()
            writer.writerows(rows)
    print(json.dumps(dict(status=receipt["status"], rows=len(receipt["rows"]), failures=len(receipt["failures"])), sort_keys=True), flush=True)
    return 0 if receipt["status"] == "completed" else 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--native", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--workers", type=int, default=4)
    args = parser.parse_args()
    if args.workers < 1:
        parser.error("workers must be positive")
    try:
        with threadpool_limits(limits=1):
            result = experiment(args)
    except BaseException as error:
        path = args.output / "receipt.json"
        if getattr(args, "run_created", False) and path.exists():
            receipt = json.loads(path.read_text())
            receipt.update(status="failed", fatal_error=repr(error), fatal_traceback=traceback.format_exc())
            save(path, receipt)
        raise
    sys.exit(result)
