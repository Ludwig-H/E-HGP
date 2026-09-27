#!/usr/bin/env python3
"""Publish small benchmark receipts and tables; never copy raw input coordinates."""
import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import statistics


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def checked(receipt):
    if receipt["status"] != "completed" or receipt["failures"]:
        raise ValueError("incomplete benchmark")
    if receipt["sources_before"] != receipt["sources_after"]:
        raise ValueError("changed experiment sources")
    if receipt["native_binary_sha256"] != receipt["native_binary_sha256_after"]:
        raise ValueError("changed native binary")
    rows = receipt["rows"]
    keys = [(r["case"], r["k"], r["min_cluster_size"], r["exp_z"], r["method"]) for r in rows]
    if len(set(keys)) != len(keys):
        raise ValueError("duplicate score row")
    case_ids = sorted({r["case"] for r in rows})
    expected = {(case, k, size, z, method) for case in case_ids for k in (2, 5, 10)
                for size in (20, 50) for z in (1, 2)
                for method in (("first_coverage", "entry_vote", "hdbscan_common", "hdbscan_standard")
                               if z == 1 else ("first_coverage", "entry_vote", "hdbscan_common"))}
    if set(keys) != expected or len(receipt["commands"]) != len(case_ids) * 3:
        raise ValueError("incomplete grid or native command count")
    return rows, case_ids


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--checks", type=Path, required=True)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    run_path = args.run / "receipt.json"
    receipt = json.loads(run_path.read_text())
    rows, cases = checked(receipt)
    manifest_path = Path(receipt["input_manifest"])
    if sha(manifest_path) != receipt["input_manifest_sha256"]:
        raise ValueError("manifest changed")
    manifest = json.loads(manifest_path.read_text())
    if not manifest["complete"] or set(cases) != {c["id"] for c in manifest["cases"]}:
        raise ValueError("whole predeclared corpus missing")
    checks = json.loads((args.checks / "receipt.json").read_text())
    if checks["status"] != "passed" or checks["sources_before"] != checks["sources_after"]:
        raise ValueError("local gates failed or sources changed")
    if any(checks["sources_before"].get(p) != h for p, h in receipt["sources_before"].items() if p.endswith(".py")):
        raise ValueError("benchmark and tested Python sources differ")
    for p, expected_hash in receipt["sources_before"].items():
        if sha(p) != expected_hash:
            raise ValueError("current source differs: " + p)
    for command in receipt["commands"]:
        name = Path(command["argv"][2]).parent.name + "_k" + command["argv"][4]
        directory = args.run / name
        if command["returncode"] or sha(directory / "native.json") != command["stdout_sha256"]:
            raise ValueError("native output changed or failed: " + name)
        if sha(directory / "native.stderr") != command["stderr_sha256"]:
            raise ValueError("native stderr changed")
    args.output.mkdir(parents=True, exist_ok=False)
    # These small archival receipts contain result metrics, commands and hashes,
    # not input point coordinates or the massive native population catalogue.
    archive = dict(schema="mhgp9_point_clustering_public_v1", status="completed",
                   run_receipt_sha256=sha(run_path), checks_receipt_sha256=sha(args.checks / "receipt.json"),
                   build_receipt_sha256=sha(args.build / "receipt.json"),
                   private_run=str(args.run.resolve()), private_checks=str(args.checks.resolve()),
                   source_scope="fixed_K_point_projection_experiment_not_GPU_contract",
                   GCP_used=False, GPU_used=False, rows=len(rows), cases=cases)
    for name, value in (("archive.json", archive), ("benchmark_receipt.json", receipt),
                        ("datasets_manifest.json", manifest), ("checks_receipt.json", checks),
                        ("native_build_receipt.json", json.loads((args.build / "receipt.json").read_text()))):
        (args.output / name).write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n")
    for command in checks["commands"]:
        for suffix in ("stdout", "stderr"):
            source = args.checks / (command["name"] + "." + suffix)
            if sha(source) != command[suffix + "_sha256"]:
                raise ValueError("gate output changed")
            (args.output / source.name).write_bytes(source.read_bytes())
    (args.output / "scores.csv").write_bytes((args.run / "scores.csv").read_bytes())
    summaries = []
    for dimension in (3, 2):
        for split in ("all", "development", "evaluation"):
            for k in (2, 5, 10):
                for size in (20, 50):
                    for z in (1, 2):
                        for method in ("first_coverage", "entry_vote", "hdbscan_common", "hdbscan_standard"):
                            group = [r for r in rows if r["dimension"] == dimension and (split == "all" or r["split"] == split)
                                     and r["k"] == k and r["min_cluster_size"] == size and r["exp_z"] == z and r["method"] == method]
                            if not group:
                                continue
                            averages = {metric: statistics.mean(r["metrics"][metric] for r in group)
                                        for metric in ("ari_all", "ari_inliers_noise_singletons", "coverage")}
                            averages["dendrogram_purity"] = statistics.mean(r["dendrogram_purity"] for r in group)
                            summaries.append(dict(dimension=dimension, split=split, k=k, min_cluster_size=size,
                                exp_z=z, method=method, case_count=len(group), **averages))
    stream = io.StringIO()
    writer = csv.DictWriter(stream, fieldnames=list(summaries[0]))
    writer.writeheader()
    writer.writerows(summaries)
    (args.output / "macro_scores.csv").write_text(stream.getvalue())
    print(json.dumps(archive, sort_keys=True))


if __name__ == "__main__":
    main()
