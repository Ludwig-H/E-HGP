#!/usr/bin/env python3
"""Capture and validate a fixed serial front-only partition experiment."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import platform
import subprocess
import sys
import zipfile

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
COMMIT = "ba11e3abfe686d8078c13bacfc066e372d6bfc10"
INPUTS = [("single_000000", 8000), ("single_000100", 8000), ("single_000200", 8000),
          ("single_000000", 16000), ("single_000000", 32000), ("single_000000", 50000)]
MATRIX = [(dataset, n, width) for dataset, n in INPUTS for width in (0, 1, 64, 256)]


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def stamp():
    return datetime.now(timezone.utc).isoformat()


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n")


def read(path):
    value = json.loads(path.read_text())
    json.dumps(value, allow_nan=False)
    return value


def input_path(dataset, n):
    return BASE.parent / "lidar08_20260914/prepared" / dataset / f"n{n}.u16le"


def fnv(path):
    value = 14695981039346656037
    for byte in path.read_bytes():
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return format(value, "x")


def command_for(binary, key):
    dataset, n, width = key
    return [str(binary), str(input_path(dataset, n)), str(n), "10", "8", str(width)]


def check(row):
    require(row["status"] == "completed" and row["returncode"] == 0 and not row["stderr"] and
            row["timed_out"] is False and row["deferred_signals"] == [], "unfinished or failed invocation")
    result = json.loads(row["stdout"])
    json.dumps(result, allow_nan=False)
    dataset, n, width = row["key"]
    require((dataset, n) in INPUTS and width in (0, 1, 64, 256), "undeclared configuration")
    require(result["status"] == "completed" and result["schema"] == "mhgp8_front_tasks_probe_v1" and
            result["scope"] == "serial_front_only_no_census_no_parallel_speedup" and
            result["public_status"] == "not_claimed" and result["gcp_used"] is False and
            result["threads"] == 1 and result["n"] == n and result["width"] == width and
            result["kmax"] == 10 and result["s"] == 8 and result["input_fnv64"] == row["input_fnv64"],
            "wrong result configuration")
    front, prefix, jobs = result["front_work"], result["prefix_work"], result["jobs"]

    def counter(value):
        if isinstance(value, dict):
            for child in value.values():
                counter(child)
        elif isinstance(value, list):
            for child in value:
                counter(child)
        else:
            require(type(value) is int and 0 <= value < 2**64, "invalid counter")

    counter(front)
    counter(prefix)
    for key, value in result.items():
        if key.endswith("_ms"):
            require(type(value) in (float, int) and math.isfinite(value) and value >= 0, "invalid time")
    total = n * (n - 1) // 2
    require(front["rejected_pair_mass"][0] + front["residual_pair_mass"][0] == total and
            front["residual_pair_mass"][0] == result["residual_mass"] and
            front["emitted_rectangles"] == result["rectangles"] == sum(front["size_class_rectangles"]) and
            sum(front["size_class_pair_mass"]) == result["residual_mass"] and
            front["xi_bound_tests"] == 0 and front["max_product_depth"] <= 96,
            "front ledger mismatch")
    for field in ("rejected_pair_mass", "residual_pair_mass", "lane_rectangles"):
        require(front[field][1:] == [0, 0] and prefix[field][1:] == [0, 0], "revived inactive lane")
    require(result["cloud_coordinate_copies"] == n and result["cloud_validation_points"] == n,
            "cloud preparation repeated")
    if width == 0:
        require(not jobs and result["maximum_ready"] == 0 and result["task_bytes"] == 0 and
                all((value == 0 if type(value) is int else value == [0] * len(value)) for value in prefix.values()),
                "public front unexpectedly used partition adapter")
    else:
        require(0 < result["maximum_ready"] <= width + 1 and len(jobs) <= result["maximum_ready"] and
                (not jobs or len(jobs) >= width) and 0 < result["task_bytes"] <= 64 and
                front["max_stack_size"] <= 97, "partition width/stack mismatch")
        for job in jobs:
            counter({k: v for k, v in job.items() if k != "elapsed_ms"})
            require(job["mask"] == 1 and job["depth"] <= 96 and job["a"] < 2*n and job["b"] < 2*n and
                    math.isfinite(job["elapsed_ms"]) and job["elapsed_ms"] >= 0, "invalid initial job")
        for field, job_field in (("product_visits", "product_visits"),
                                 ("witness_descent_steps", "witness_descent_steps"),
                                 ("emitted_rectangles", "rectangles")):
            require(prefix[field] + sum(job[job_field] for job in jobs) == front[field],
                    "lost/doubled prefix or job: " + field)
        require(prefix["residual_pair_mass"][0] + sum(job["pair_mass"] for job in jobs) == result["residual_mass"],
                "lost prefix output")
        require(prefix["residual_pair_mass"][0] + prefix["rejected_pair_mass"][0] +
                sum(job["seed_mass"] for job in jobs) == total, "initial seeds do not close the prefix mass")
        require(result["adapter_ms"] + 1e-5 >= result["prefix_ms"] + sum(job["elapsed_ms"] for job in jobs),
                "job times exceed enclosing interval")
    require(result["total_ms"] + 1e-5 >= result["front_ms"] and
            result["front_ms"] + 1e-5 >= result["adapter_ms"], "invalid enclosing time")
    return result


def build_record(name, passed=True):
    path = BASE / (name + "_BUILD.json")
    record = read(path)
    require(record["schema"] == "mhgp8_audit_front_tasks_build_v1" and record["source_commit"] == COMMIT and
            record["status"] == ("passed" if passed else "failed"), "wrong build")
    archive = ROOT / record["archive"]
    require(sha(archive) == record["archive_sha256"], "changed source archive")
    with zipfile.ZipFile(archive) as zipped:
        require(len(zipped.namelist()) == len(set(zipped.namelist())) and
                set(zipped.namelist()) == set(record["source_sha256"]), "changed archive entries")
        for source, pin in record["source_sha256"].items():
            require(hashlib.sha256(zipped.read(source)).hexdigest() == pin, "archived source differs")
    for source, pin in record["audit_inputs"].items():
        require(sha(ROOT / source) == pin, "changed audit input: " + source)
    if passed:
        require(all(step["returncode"] == step["expected_returncode"] for step in record["steps"]),
                "unclosed build/gate steps")
        gate = BASE / ".build" / name / "gate"
        require(sha(gate) == record["gate_sha256"], "changed gate binary")
    return record


def pairing(rows):
    groups = {}
    summaries = []
    for row in rows:
        result = check(row)
        groups.setdefault(tuple(row["key"][:2]), {})[row["key"][2]] = result
    require(set(groups) == set(INPUTS), "missing paired input")
    for key, modes in groups.items():
        require(set(modes) == {0, 1, 64, 256}, "missing paired width")
        reference = modes[0]
        expected = {k: v for k, v in reference["front_work"].items() if k != "max_stack_size"}
        for width, result in modes.items():
            require(expected == {k: v for k, v in result["front_work"].items() if k != "max_stack_size"} and
                    result["digest"] == reference["digest"] and result["rectangles"] == reference["rectangles"],
                    "restart changed geometric work or rectangles")
            jobs = result["jobs"]
            summary = dict(input=key, width=width, jobs=len(jobs), front_ms=result["front_ms"],
                           prefix_ms=result["prefix_ms"])
            for field in ("product_visits", "witness_descent_steps", "rectangles", "pair_mass", "seed_mass", "elapsed_ms"):
                values = sorted(job[field] for job in jobs)
                if values and sum(values) > 0:
                    summary[field] = dict(total=sum(values), maximum=values[-1],
                                          maximum_fraction=values[-1]/sum(values),
                                          imbalance_to_mean=values[-1]*len(values)/sum(values))
            summaries.append(summary)
    return summaries


def validate():
    directory = BASE / "campaign"
    manifest, done = read(directory / "MANIFEST.json"), read(directory / "COMPLETION.json")
    require(done["status"] == "completed" and done["manifest_sha256"] == sha(directory / "MANIFEST.json") and
            done["measures_sha256"] == sha(directory / "MEASURES.jsonl"), "campaign not closed")
    require(manifest["matrix"] == [list(key) for key in MATRIX] and manifest["runner_sha256"] == sha(Path(__file__)),
            "changed matrix/runner")
    for source, pin in manifest["pins"].items():
        require(sha(ROOT / source) == pin, "changed campaign source")
    build = build_record("r1")
    binary = ROOT / build["binary"]
    require(sha(binary) == build["binary_sha256"], "changed probe binary")
    rows = [json.loads(line) for line in (directory / "MEASURES.jsonl").read_text().splitlines()]
    require([row["key"] for row in rows] == manifest["matrix"], "incomplete or reordered campaign")
    for row in rows:
        require(row["command"] == command_for(binary, row["key"]), "command/result mismatch")
        path = input_path(*row["key"][:2])
        require(row["input_sha256"] == sha(path) and row["input_fnv64"] == fnv(path), "wrong input")
    return dict(status="passed", rows=len(rows), summaries=pairing(rows))


def measure():
    directory = BASE / "campaign"
    require(not directory.exists(), "refuse overwrite")
    # Explicitly reuse the previously reviewed kill/drain/signal deferral helper.
    helper = BASE.parent / "q2_small_roots_20260914/measure.py"
    require(sha(helper) == "b3f9e39592004d12ff37d25edd2ca4a5f136f250fb6cb271cdb65c5e7a996eb7", "changed invocation helper")
    spec = importlib.util.spec_from_file_location("small_roots_measure", helper)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    build = build_record("r1")
    binary = ROOT / build["binary"]
    require(sha(binary) == build["binary_sha256"], "changed probe binary")
    paths = {Path(__file__), helper, BASE / "r1_BUILD.json", ROOT / build["archive"], binary,
             *(ROOT / p for p in build["audit_inputs"]), *(input_path(d, n) for d, n in INPUTS),
             BASE.parent / "q2_pool_bridge_20260914/measure.py"}
    pins = {str(path.relative_to(ROOT)): sha(path) for path in sorted(paths)}
    cpus = sorted(os.sched_getaffinity(0))
    manifest = dict(schema="mhgp8_front_tasks_campaign_v1", matrix=MATRIX, pins=pins,
                    runner_sha256=sha(Path(__file__)), selected_cpu=cpus[-1], allowed_cpus=cpus,
                    started_utc=stamp(), platform=platform.platform(), python=sys.version,
                    shared_host=True, warmups=0, timeout_seconds=180, gcp_used=False,
                    public_status="not_claimed", scope="serial_front_only", command=sys.argv)
    directory.mkdir()
    write(directory / "MANIFEST.json", manifest)
    rows, status = [], "failed"
    try:
        with (directory / "MEASURES.jsonl").open("x") as stream:
            for key in MATRIX:
                path = input_path(*key[:2])
                row = dict(key=list(key), command=command_for(binary, key), status="failed", started_utc=stamp(),
                           input_sha256=sha(path), input_fnv64=fnv(path), loadavg_before=os.getloadavg())
                try:
                    row.update(module.invoke(row["command"], cpus[-1]))
                    row["status"] = "completed"
                    result = check(row)
                except BaseException as error:
                    row.update(status="failed", error_type=type(error).__name__, error=str(error))
                    raise
                finally:
                    row["finished_utc"] = stamp()
                    stream.write(json.dumps(row, allow_nan=False) + "\n")
                    stream.flush()
                rows.append(row)
                print(json.dumps(dict(key=key, front_ms=result["front_ms"], jobs=len(result["jobs"]),
                                      prefix_ms=result["prefix_ms"])), flush=True)
        pairing(rows)
        require(all(sha(ROOT / p) == h for p, h in pins.items()), "source changed during measurement")
        status = "completed"
    finally:
        write(directory / "COMPLETION.json", dict(status=status, finished_utc=stamp(),
              manifest_sha256=sha(directory / "MANIFEST.json"), measures_sha256=sha(directory / "MEASURES.jsonl")))
    print(json.dumps(dict(status=validate()["status"], rows=len(rows))), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validate", action="store_true")
    args = parser.parse_args()
    if args.validate:
        print(json.dumps(validate(), sort_keys=True, allow_nan=False))
    else:
        measure()
