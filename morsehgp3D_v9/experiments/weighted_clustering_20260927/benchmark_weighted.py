#!/usr/bin/env python3
"""Predeclared paired Gaussian pilot of the weighted Gabriel reference.

Inputs and comparator labels are inherited by hash, not fitted or chosen
after inspecting new scores. Large native/condensed arrays stay private.
"""
from __future__ import annotations

import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
import traceback

HERE = Path(__file__).resolve().parent
V9 = HERE.parents[1]
PREVIOUS = V9 / "audits/b_gaussian_point_clustering_20260927"
FIRST = V9 / "audits/b_point_hierarchy_k_20260927"
sys.path.extend((str(PREVIOUS), str(FIRST)))
from benchmark import clean, metrics
from evaluation import evaluate_labels
from weighted_model import build_facet_model, vote_points
from weighted_eom import weighted_condense_eom


CASES = tuple(f"spherical_g{g}_d{delta}_s{seed}"
              for g, delta in ((2, 8), (8, 4), (16, 2)) for seed in (1, 2, 3)) + tuple(
    f"{regime}_g8_d4_s{seed}" for regime in ("anisotropic", "unbalanced") for seed in (1, 2))
KS, SIZES, EXPONENTS = (5, 10), (20, 50), (1, 2)
PREVIOUS_RECEIPT_SHA = "8f9aba99396b6341f7a6872956e66f50a7776cc1535d9a3f9137907d2945157a"
MANIFEST_SHA = "1cab6404055aebf0dfa31b76f16f8a4c84e4acf9678b2b910dabd9f367f46c67"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    path = Path(path)
    with path.open("x") as out:
        json.dump(clean(value), out, sort_keys=True, indent=2, allow_nan=False)
        out.write("\n")


def save_gzip(path, value):
    encoder = json.JSONEncoder(sort_keys=True, allow_nan=False, separators=(",", ":"))
    with Path(path).open("xb") as raw:
        with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=0) as zipped:
            for fragment in encoder.iterencode(clean(value)):
                zipped.write(fragment.encode())


def need(ok, why):
    if not ok:
        raise ValueError(why)


def run(args):
    need(not args.output.exists(), "fresh output directory required")
    args.output.mkdir(parents=True)
    receipt = dict(schema="mhgp9_weighted_gaussian_pilot_v1", status="running", rows=[], commands=[],
                   artifacts={}, GCP_used=False, GPU_used=False, engine_modified=False,
                   scope="weighted_Gabriel_reference_fixed_K_not_point_hierarchy_decoder",
                   plan=dict(cases=list(CASES), k=list(KS), sizes=list(SIZES), exp_z=list(EXPONENTS)),
                   root_policy="excluded_for_both", approximate_postprocessing=True,
                   selected_subset_after_previous_scores=True,
                   note="Diagnostic pilot on known regimes, not a held-out superiority test")
    start_run = time.monotonic()
    try:
        baseline_path = args.baseline / "receipt.json"
        need(sha(baseline_path) == PREVIOUS_RECEIPT_SHA, "baseline receipt pin")
        baseline = json.loads(baseline_path.read_text())
        need(baseline["sources_before"] == baseline["sources_after"], "baseline source closure")
        for path, expected in baseline["sources_after"].items():
            need(sha(path) == expected, "changed inherited source " + path)
        baseline_commands = {(c["case"], c["k"]): c for c in baseline["commands"]}
        manifest_path = Path(baseline["manifest_path"])
        need(sha(manifest_path) == MANIFEST_SHA, "manifest pin")
        manifest = json.loads(manifest_path.read_text())
        by_case = {case["id"]: case for case in manifest["cases"]}
        need(len(CASES) == len(set(CASES)) == 13 and all(c in by_case for c in CASES), "fixed 13-case grid")
        qualification = json.loads(args.qualification.read_text())
        need(qualification["status"] == "passed", "geometry qualification required")
        need(sha(args.native) == qualification["native_binary_sha256"], "qualified native binary pin")
        need(qualification["sources_before"] == qualification["sources_after"], "qualification source closure")
        for path, expected in qualification["sources_after"].items():
            need(sha(path) == expected, "qualified source changed " + path)
        dependencies = sorted(HERE.glob("*.py")) + [HERE / "native_weighted_export.cpp", HERE / "README.md"]
        dependencies += [PREVIOUS / name for name in ("evaluation.py", "condensed.py")]
        dependencies += [FIRST / name for name in ("benchmark.py", "eom.py", "projection.py", "datasets.py", "native_export.cpp")]
        receipt.update(sources_before={str(p.resolve()): sha(p) for p in dependencies},
                       native_binary=str(args.native.resolve()), native_binary_sha256=sha(args.native),
                       qualification=str(args.qualification.resolve()), qualification_sha256=sha(args.qualification),
                       baseline_receipt=str(baseline_path.resolve()), baseline_receipt_sha256=sha(baseline_path),
                       manifest=str(manifest_path), manifest_sha256=sha(manifest_path), input_hashes={})
        save(args.output / "intent.json", receipt)
        for case_id in CASES:
            case = by_case[case_id]
            for key, expected in case["prepared_sha256"].items():
                need(sha(case[key]) == expected, "changed input " + key)
                receipt["input_hashes"][case[key]] = expected
            truth = json.loads(Path(case["labels_json"]).read_text())
            need(len(truth) == case["n"] == 1200, "whole Gaussian scene")
            for k in KS:
                directory = args.output / f"{case_id}_k{k}"
                directory.mkdir()
                argv = [str(args.native.resolve()), "--input", case["points_u32le"], "--k", str(k), "--workers", "1"]
                save(directory / "intent.json", dict(argv=argv, binary_sha256=sha(args.native), input_sha256=sha(case["points_u32le"])))
                print("START", case_id, k, flush=True)
                start = time.monotonic()
                with (directory / "native.json").open("xb") as out, (directory / "native.stderr").open("xb") as err:
                    command = subprocess.run(argv, stdout=out, stderr=err, check=False)
                receipt["commands"].append(dict(case=case_id, k=k, argv=argv, returncode=command.returncode,
                    elapsed_seconds=time.monotonic()-start, stdout_sha256=sha(directory/"native.json"),
                    stderr_sha256=sha(directory/"native.stderr")))
                save(directory / "command.json", receipt["commands"][-1])
                need(command.returncode == 0, "native export failed")
                payload = json.loads((directory / "native.json").read_text())
                need(payload["schema"] == "mhgp9_weighted_catalogue_export_v1" and payload["status"] == "completed", "native export status/schema")
                native = payload["native"]
                need(native["status"] == "completed", "native FULL status")
                need(payload["catalog_universe"] == "gabriel_complete_boundary", "declared catalogue")
                need(native["point_count"] == 1200 and native["k"] == k, "native request binding")
                # Point order and geometric object must match the frozen common
                # comparator experiment, not just n and K.
                old_native_path = args.baseline / f"{case_id}_k{k}" / "native.json"
                need(sha(old_native_path) == baseline_commands[(case_id, k)]["stdout_sha256"], "frozen native payload hash")
                old_native = json.loads(old_native_path.read_text())
                for key in ("points", "nodes", "roots", "populations", "contributions", "tower_digest", "catalogue_digest"):
                    need(native[key] == old_native[key], "changed fixed-K object: " + key)
                receipt["artifacts"][str(old_native_path)] = sha(old_native_path)
                rows = [dict(vertices=row["vertices"], beta=row["beta"]) for row in payload["cofaces"]]
                for z in EXPONENTS:
                    start = time.monotonic()
                    model = build_facet_model(1200, k, rows, exp_z=z)
                    model_ms = 1000*(time.monotonic()-start)
                    need(len(model["roots"]) == 1, "connected complete facet catalogue")
                    measure_path = directory / f"measure_z{z}.json.gz"
                    save_gzip(measure_path, dict(facets=model["facets"], scores=model["scores"],
                        point_totals=model["point_totals"], masses=model["masses"], children=model["children"],
                        squared_levels={node: str(beta) for node, beta in model["squared_levels"].items()}))
                    receipt["artifacts"][str(measure_path)] = sha(measure_path)
                    for minimum in SIZES:
                        start = time.monotonic()
                        selected = weighted_condense_eom(len(model["facets"]), model["children"], model["heights"],
                            model["masses"], min_cluster_size=minimum, exp_z=z)
                        selection_ms = 1000*(time.monotonic()-start)
                        start = time.monotonic()
                        vote = vote_points(model, selected["labels"])
                        vote_ms = 1000*(time.monotonic()-start)
                        point_counts = Counter(x for x in vote["labels"] if x >= 0)
                        result = dict(case=case_id, regime=case["regime"], communities=case["communities"],
                            separation=case["separation"], seed=case["seed"], n=1200, k=k,
                            min_cluster_size=minimum, exp_z=z, method="hgp_weighted_Gabriel_vote",
                            metrics=metrics(truth, vote["labels"]), extra=evaluate_labels(truth, vote["labels"]),
                            model_statistics=model["statistics"], native_statistics=payload["stats"],
                            times_ms=dict(model=model_ms, selection=selection_ms, vote=vote_ms),
                            final_point_sizes=sorted(point_counts.values()),
                            final_point_clusters_below_mass_threshold=sum(v < minimum for v in point_counts.values()),
                            near_threshold_nodes=len(selected["near_threshold_nodes"]),
                            vote_ties=sum(x == 0 and label >= 0 for x, label in zip(vote["raw_margins"], vote["labels"])))
                        receipt["rows"].append(result)
                        path = directory / f"weighted_m{minimum}_z{z}.json.gz"
                        save_gzip(path, dict(selection=selected, vote=vote))
                        receipt["artifacts"][str(path)] = sha(path)
                # Reuse immutable comparator rows. No new favorable retuning.
                for row in baseline["rows"]:
                    if row["case"] == case_id and row["k"] == k and row["min_cluster_size"] in SIZES:
                        receipt["rows"].append({key: row[key] for key in (
                            "case", "regime", "communities", "separation", "seed", "n", "k",
                            "min_cluster_size", "exp_z", "method", "metrics", "extra")})
                print("DONE", case_id, k, "cofaces", len(rows), "facets", len(model["facets"]), flush=True)
                del payload, native, old_native, rows, model, selected, vote
        receipt["sources_after"] = {path: sha(path) for path in receipt["sources_before"]}
        need(receipt["sources_after"] == receipt["sources_before"], "source closure")
        need(sha(args.native) == receipt["native_binary_sha256"], "binary closure")
        for path, expected in receipt["input_hashes"].items():
            need(sha(path) == expected, "input closure")
        for path, expected in receipt["artifacts"].items():
            need(sha(path) == expected, "artifact closure")
        for command in receipt["commands"]:
            directory = args.output / f"{command['case']}_k{command['k']}"
            for suffix in ("stdout", "stderr"):
                path = directory / ("native.json" if suffix == "stdout" else "native.stderr")
                need(sha(path) == command[suffix + "_sha256"], "native capture closure")
        need(sha(baseline_path) == PREVIOUS_RECEIPT_SHA, "baseline closure")
        need(sha(manifest_path) == MANIFEST_SHA, "manifest closure")
        need(sha(args.qualification) == receipt["qualification_sha256"], "qualification receipt closure")
        need(len(receipt["rows"]) == 364 and len(receipt["commands"]) == 26, "complete planned grid")
        receipt["status"] = "completed"
    except BaseException as error:
        receipt.update(status="failed", error=repr(error), traceback=traceback.format_exc())
        raise
    finally:
        receipt["elapsed_seconds"] = time.monotonic()-start_run
        save(args.output / "receipt.json", receipt)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path, required=True)
    parser.add_argument("--qualification", type=Path, required=True)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    run(parser.parse_args())
