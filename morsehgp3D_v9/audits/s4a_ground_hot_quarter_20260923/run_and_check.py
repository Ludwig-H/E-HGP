#!/usr/bin/env python3
"""Prepare and, when the host is quiet, run paired S3/S4a CPU probes.

All writes stay beside this script. Run with ``python3 -B``. The source
generator and v12 outputs are read only; no historical receipt is rewritten.
"""

import argparse
import datetime as dt
import hashlib
import importlib.util
import json
import math
import os
import resource
import subprocess
import time
from pathlib import Path


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
WORKTREE = REPO / "build/v9-open-worktree"
BINARY = WORKTREE / "build/v9-exp/mhgp9_tower_probe"
SOURCE_SCRIPT = REPO / "morsehgp3D_v9/audits/lidar_ground_hot_quarter_multiseed_20260923/run_and_check.py"
SOURCE_MANIFEST = SOURCE_SCRIPT.parent / "MANIFEST.json"
BASELINE = {
    "s1_quarter": SOURCE_SCRIPT.parent / "s1_quarter.stdout",
    "s1_half": SOURCE_SCRIPT.parent / "s1_half.stdout",
    "physical_full": REPO / "morsehgp3D_v9/audits/lidar_scene02_physical_cut_20260923/quarter_x_nonneg_y_neg.stdout",
}
BASELINE_SHA = {
    "s1_quarter": "4abdf59405aeec4ab39648613021bb9222e80b579d673b1671f9f15bd3e106f4",
    "s1_half": "ad4c666a92f4a8c18880391888d7affc51f3b7647acf27db2b019e46e318e1a7",
    "physical_full": "7493d0fa054886198c73b760be41ff29809ad4badda3bc94363be447cf25d374",
}
SOURCE_SCRIPT_SHA = "139e1f0715810bc8b959eda16705b21e33c4eb599eb2ba474dee112835ab304b"
SOURCE_MANIFEST_SHA = "1cd693b57cbd35d2f2f9b76cdaf14334c575cb7340cc88c5c869bf8c889bfaa8"
BINARY_SHA = "eea3040cf4150599ca7ab5c71a4f0e3738f2975483584527f339d3d2600d08e9"
SNAPSHOT_HEAD = "7ceadffad1de860e30325ae357ba3243f269d48b"
SOURCE_OBJECTS = {
    "morsehgp3D_v9/src": "064e30629574e057bf699ba7a2bbe911240a99aa",
    "morsehgp3D_v9/bench/tower_probe.cpp": "2231b1964f434255cd655090bb06f77ec8ebd683",
    "morsehgp3D_v9/CMakeLists.txt": "143b7b0b31bee62f551ee21c999ee46a253e8a51",
}
SIZES = ("s1_quarter", "s1_half", "physical_full")
ARMS = ("s3", "s4a")
LEGACY_LEVERS = ("atlas_saturate_deep", "q3_leaf_census", "q34_dead_lanes",
                 "q34_witness_cache", "q34_dead_core", "tower_meb_proposal")
SHARED_LEVERS = ("q34_jobs_by_mass", "q34_fine_jobs", "tower_overlap_static",
                 "q2_jobs_by_mass", "q34_batch_filter", "q34_batch_certificates")
LEDGER_EQUAL = ("expanded_pairs", "cover_builds", "cover_sites", "cover_node_visits",
                "q3_edges", "q4_edges", "both_edges")
WORK_FIELDS = ("core_sites", "expanded_pairs", "q3_edges", "q4_edges",
               "lanes_edges", "lanes_cover_sites", "lanes_seed_tests",
               "lanes_seeds", "lanes_census_point_tests", "lanes_emitted")


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(*args):
    result = subprocess.run(("git", "-C", str(WORKTREE), *args),
                            capture_output=True, text=True)
    need(result.returncode == 0, f"git {args}: {result.stderr.strip()}")
    return result.stdout.strip()


def live_source_objects():
    objects = {}
    for path, expected in SOURCE_OBJECTS.items():
        actual = git("rev-parse", f"{SNAPSHOT_HEAD}:{path}")
        need(actual == expected, f"pinned source object drifted: {path}")
        objects[path] = actual
    return objects


def live_identity():
    # The executable is pinned by bytes; the original source objects are
    # looked up in the immutable snapshot commit, not mutable HEAD.
    live_source_objects()
    need(sha(BINARY) == BINARY_SHA, "S4a CPU binary drifted")
    need(sha(SOURCE_SCRIPT) == SOURCE_SCRIPT_SHA and
         sha(SOURCE_MANIFEST) == SOURCE_MANIFEST_SHA, "input generator/manifest drifted")
    for name, path in BASELINE.items():
        need(sha(path) == BASELINE_SHA[name], f"v12 baseline drifted: {name}")


def generator():
    spec = importlib.util.spec_from_file_location("ground_hot_inputs", SOURCE_SCRIPT)
    need(spec is not None and spec.loader is not None, "cannot import pinned generator")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def generated():
    helper = generator()
    points, ids, raw, physical, full = helper.source()
    selected = helper.select(points, ids, raw, physical, helper.SEEDS["s1"])
    blobs = {"s1_quarter": selected["quarter"]["points"],
             "s1_half": selected["half"]["points"],
             "physical_full": full}
    published = json.loads(SOURCE_MANIFEST.read_text())
    for name in SIZES:
        entry = (published["cases"][name] if name != "physical_full" else
                 {"sites": published["physical_full_sites"],
                  "points_sha256": published["physical_full_points_sha256"],
                  "input_fnv64": helper.fnv_points(full)})
        need(len(blobs[name]) == 12 * entry["sites"] and
             hashlib.sha256(blobs[name]).hexdigest() == entry["points_sha256"] and
             helper.fnv_points(blobs[name]) == entry["input_fnv64"],
             f"regenerated input differs: {name}")
    return helper, blobs


def expected():
    published = json.loads(SOURCE_MANIFEST.read_text())
    helper, blobs = generated()
    entries = {}
    for name in SIZES:
        data = blobs[name]
        old = json.loads(BASELINE[name].read_text())
        need(old["schema"] == "mhgp9_tower_probe_v12" and
             old["status"] == "complete_relative", f"legacy status: {name}")
        need(old["input"]["sites"] == len(data) // 12 and
             old["input"]["hash"] == helper.fnv_points(data),
             f"legacy input differs: {name}")
        entries[name] = {
            "file": f"inputs/{name}.u32le", "sites": len(data) // 12,
            "sha256": hashlib.sha256(data).hexdigest(),
            "fnv64": helper.fnv_points(data),
            "baseline_stdout_sha256": BASELINE_SHA[name],
            "baseline_tower_digest": old["tower_digest"],
            "baseline_catalogue_balls": old["catalogue"]["balls"],
            "baseline_core_sites": old["ledger"]["core_sites"],
            "baseline_expanded_pairs": old["ledger"]["expanded_pairs"],
        }
    need(entries["s1_quarter"]["sites"] < entries["s1_half"]["sites"] <
         entries["physical_full"]["sites"] == 14828, "density nesting sizes")
    need(published["cases"]["s1_quarter"]["seed"] ==
         published["cases"]["s1_half"]["seed"] == "7d1c9a5eb3f24680",
         "density seed")
    return entries, blobs


def prepare():
    live_identity()
    entries, blobs = expected()
    folder = HERE / "inputs"
    folder.mkdir(exist_ok=True)
    for name, data in blobs.items():
        path = folder / f"{name}.u32le"
        if path.exists():
            need(path.read_bytes() == data, f"input already exists but differs: {name}")
        else:
            path.write_bytes(data)
    record = {
        "schema": "mhgp9_s4a_hot_quarter_prepared_v1",
        "status": "prepared_no_hgp_measurements",
        "source_head": SNAPSHOT_HEAD, "source_objects": SOURCE_OBJECTS,
        "binary": str(BINARY), "binary_sha256": BINARY_SHA,
        "generator_sha256": SOURCE_SCRIPT_SHA,
        "source_manifest_sha256": SOURCE_MANIFEST_SHA,
        "sector": "08/000200 sans sol, physical x>=0,y<0",
        "seed": "7d1c9a5eb3f24680", "grid": "1mm/u18",
        "K": 10, "s": 8, "workers": 8, "static_threads": 8,
        "cases": entries,
    }
    path = HERE / "PREPARED.json"
    data = (json.dumps(record, indent=2, sort_keys=True) + "\n").encode()
    if path.exists():
        need(path.read_bytes() == data, "prepared record drifted")
    else:
        path.write_bytes(data)
    print("prepared", *(f"{name}={entries[name]['sites']}:{entries[name]['sha256']}"
                        for name in SIZES))
    return record


def archived_prepared():
    """Check regenerated inputs against the archived record, without live v9.

    This path intentionally never opens BINARY, calls git, or reads the v12
    baseline outputs. Their identities and stable facts are in PREPARED.json.
    """
    path = HERE / "PREPARED.json"
    record = json.loads(path.read_text())
    need(record["schema"] == "mhgp9_s4a_hot_quarter_prepared_v1" and
         record["source_head"] == SNAPSHOT_HEAD and
         record["binary_sha256"] == BINARY_SHA and
         record["source_objects"] == SOURCE_OBJECTS and
         record["generator_sha256"] == SOURCE_SCRIPT_SHA and
         record["source_manifest_sha256"] == SOURCE_MANIFEST_SHA and
         record["K"] == 10 and record["s"] == 8 and
         record["workers"] == record["static_threads"] == 8 and
         set(record["cases"]) == set(SIZES), "archived preparation metadata")
    need(sha(SOURCE_SCRIPT) == record["generator_sha256"] and
         sha(SOURCE_MANIFEST) == record["source_manifest_sha256"],
         "archived input generator/manifest drifted")
    helper, blobs = generated()
    for name in SIZES:
        entry = record["cases"][name]
        data = blobs[name]
        need(entry["file"] == f"inputs/{name}.u32le" and
             entry["sites"] == len(data) // 12 and
             entry["sha256"] == hashlib.sha256(data).hexdigest() and
             entry["fnv64"] == helper.fnv_points(data) and
             entry["baseline_stdout_sha256"] == BASELINE_SHA[name],
             f"archived input differs: {name}")
        file = HERE / entry["file"]
        if file.exists():
            need(file.read_bytes() == data, f"stored input differs: {name}")
        else:
            file.parent.mkdir(exist_ok=True)
            file.write_bytes(data)
    return record


def command(name, arm, binary=None):
    levers = [*LEGACY_LEVERS, *SHARED_LEVERS]
    argv = ["nice", "-n", "19", str(BINARY if binary is None else binary),
            str(HERE / "inputs" / f"{name}.u32le"),
            "10", "8", "--s=8", "--static=8", "--grid=1mm", "--catalogue-digest"]
    argv.extend(f"--lever={lever}=1" for lever in levers)
    argv.extend(("--lever=q34_gpu_filter=0", "--lever=q34_gpu_certificates=0",
                 f"--lever=q34_batch_q3={int(arm == 's4a')}", "--lever=q34_gpu_q3=0"))
    return argv


def rows():
    path = HERE / "CASES.jsonl"
    return [json.loads(line) for line in path.read_text().splitlines() if line] if path.exists() else []


def output_validated(row):
    # The first three attempts used `validated` before timing quality was
    # separated. Keep those lines unchanged and interpret them explicitly.
    if "output_validated" in row:
        need(row["output_validated"] is row["validated"], "output validation aliases differ")
    return row["validated"] is True


def timing_accepted(row):
    threshold = row.get("max_load", 8.0)
    accepted = row["load_before"][0] <= threshold and row["load_after"][0] <= threshold
    if "timing_accepted" in row:
        need(row["timing_accepted"] is accepted, "timing acceptance differs from load record")
    return accepted


def run(max_load, timeout):
    prepared = prepare()
    done = {(row["name"], row["arm"]) for row in rows()
            if output_validated(row) and timing_accepted(row)}
    for name in SIZES:
        for arm in ARMS:
            if (name, arm) in done:
                continue
            live_identity()
            source_before = live_source_objects()
            head_before = git("rev-parse", "HEAD")
            binary_before = sha(BINARY)
            entry = prepared["cases"][name]
            path = HERE / entry["file"]
            need(sha(path) == entry["sha256"], f"input drifted: {name}")
            load_before = os.getloadavg()
            need(load_before[0] <= max_load,
                 f"host load {load_before[0]:.2f} exceeds {max_load}; no timing started")
            argv = command(name, arm)
            previous = rows()
            attempt = len(previous) + 1
            stem = f"attempt_{attempt:04d}_{name}_{arm}"
            stdout_file, stderr_file = (HERE / f"{stem}.{suffix}"
                                        for suffix in ("stdout", "stderr"))
            need(not stdout_file.exists() and not stderr_file.exists(),
                 f"attempt output already exists: {stem}")
            before = resource.getrusage(resource.RUSAGE_CHILDREN)
            started = dt.datetime.now(dt.timezone.utc).isoformat()
            tick = time.monotonic()
            try:
                proc = subprocess.run(argv, capture_output=True, timeout=timeout)
                stdout, stderr, code, timed_out = proc.stdout, proc.stderr, proc.returncode, False
            except subprocess.TimeoutExpired as exc:
                stdout, stderr, code, timed_out = exc.stdout or b"", exc.stderr or b"", None, True
            wall = time.monotonic() - tick
            after = resource.getrusage(resource.RUSAGE_CHILDREN)
            ended = dt.datetime.now(dt.timezone.utc).isoformat()
            stdout_file.write_bytes(stdout)
            stderr_file.write_bytes(stderr)
            post_errors = []
            try:
                binary_after = sha(BINARY)
            except Exception as exc:
                binary_after = None
                post_errors.append(f"binary: {exc}")
            try:
                source_after = live_source_objects()
                head_after = git("rev-parse", "HEAD")
            except Exception as exc:
                source_after, head_after = None, None
                post_errors.append(f"source: {exc}")
            try:
                input_after = sha(path)
            except Exception as exc:
                input_after = None
                post_errors.append(f"input: {exc}")
            row = {"schema": "mhgp9_s4a_hot_quarter_case_v1", "name": name, "arm": arm,
                   "attempt": attempt, "stdout_file": stdout_file.name,
                   "stderr_file": stderr_file.name, "argv": argv,
                   "binary_sha256_before": binary_before,
                   "binary_sha256_after": binary_after,
                   "source_objects_before": source_before,
                   "source_objects_after": source_after,
                   "current_head_before": head_before,
                   "current_head_after": head_after,
                   "post_identity_error": "; ".join(post_errors) if post_errors else None,
                   "input_sha256_before": entry["sha256"], "input_sha256_after": input_after,
                   "started_utc": started, "ended_utc": ended,
                   "load_before": load_before, "load_after": os.getloadavg(),
                   "max_load": max_load,
                   "wall_s": wall,
                   "cpu_user_s": after.ru_utime - before.ru_utime,
                   "cpu_system_s": after.ru_stime - before.ru_stime,
                   "exit_code": code, "timed_out": timed_out,
                   "stdout_sha256": hashlib.sha256(stdout).hexdigest(),
                   "stderr_sha256": hashlib.sha256(stderr).hexdigest()}
            try:
                need(not timed_out and code == 0 and row["binary_sha256_after"] == BINARY_SHA and
                     row["input_sha256_after"] == entry["sha256"] and
                     row["source_objects_after"] == SOURCE_OBJECTS,
                     "failed or drifted process")
                check_probe(json.loads(stdout), name, arm, entry)
                row["validated"], row["validation_error"] = True, None
            except Exception as exc:
                row["validated"], row["validation_error"] = False, str(exc)
            row["output_validated"] = row["validated"]
            row["timing_accepted"] = timing_accepted(row)
            row["timing_reason"] = (None if row["timing_accepted"] else
                                    f"host load exceeded {max_load} during the case")
            with (HERE / "CASES.jsonl").open("a") as file:
                file.write(json.dumps(row, sort_keys=True) + "\n")
            print(name, arm, "attempt", attempt, "exit", code, "wall_s", round(wall, 3),
                  "load", round(load_before[0], 2), round(row["load_after"][0], 2),
                  "timing_accepted", row["timing_accepted"], flush=True)
            need(output_validated(row), f"failed case retained: {stem}: {row['validation_error']}")
            need(timing_accepted(row), f"contended timing retained and excluded: {stem}")
            live_identity()
    verify()


def check_probe(probe, name, arm, entry):
    need(probe["schema"] == "mhgp9_tower_probe_v20" and
         probe["status"] == "complete_relative" and
         probe["reason"] == "complete_relative_to_cross_checked_catalogue",
         f"status {name}/{arm}")
    need(probe["input"] == {"format": "u32le", "grid": "1mm",
                             "sites": entry["sites"], "hash": entry["fnv64"]},
         f"input {name}/{arm}")
    opts = probe["options"]
    need(opts["K"] == opts["K_effective"] == 10 and opts["s"] == 8 and
         opts["workers"] == opts["tower_static_threads"] == 8 and
         opts["run_tower"] and not opts["certificate_judge"] and
         not opts["lanes_judge"], f"options {name}/{arm}")
    lev = opts["levers"]
    need(all(lev[key] for key in (*LEGACY_LEVERS, *SHARED_LEVERS)) and
         not lev["q34_gpu_filter"] and not lev["q34_gpu_certificates"] and
         not lev["q34_gpu_q3"] and lev["q34_batch_q3"] == (arm == "s4a"),
         f"levers {name}/{arm}")
    need([row["K"] for row in probe["orders"]] == list(range(1, 11)),
         f"orders {name}/{arm}")
    need(probe["tower_digest"] == entry["baseline_tower_digest"] and
         probe["catalogue"]["balls"] == entry["baseline_catalogue_balls"] and
         probe["ledger"]["core_sites"] == entry["baseline_core_sites"] and
         probe["ledger"]["expanded_pairs"] == entry["baseline_expanded_pairs"],
         f"v12 stable baseline {name}/{arm}")
    need(probe["ledger"]["core_sites"] == probe["ledger"]["dead_core_form_sites"] +
         2 * probe["ledger"]["dead_core_loads"], f"core identity {name}/{arm}")
    batch = probe["q34_batch"]
    need(batch["used"] and batch["backend"] == "cpu" and
         batch["certificate_backend"] == "cpu", f"batch backend {name}/{arm}")
    if arm == "s4a":
        need(batch["lanes_backend"] == "cpu" and batch["lanes_asked"] > 0 and
             batch["lanes_asked"] == batch["lanes_decided"] + batch["lanes_deferred"] and
             probe["ledger"]["lanes_edges"] > 0, f"S4a work {name}")
    else:
        need(batch["lanes_asked"] == 0 and probe["ledger"]["lanes_edges"] == 0,
             f"S3 lane work {name}")


def slope(values, sizes):
    need(all(v > 0 for v in values), "nonpositive slope work")
    return [math.log(values[i + 1] / values[i]) /
            math.log(sizes[i + 1] / sizes[i]) for i in (0, 1)]


def verify():
    prepared = archived_prepared()
    attempts = rows()
    wanted = {(name, arm) for name in SIZES for arm in ARMS}
    need(all((r.get("name"), r.get("arm")) in wanted for r in attempts),
         "unknown attempt in receipt")
    need([row.get("attempt") for row in attempts] == list(range(1, len(attempts) + 1)),
         "attempt numbers must follow the append order")
    saved = {}
    for row in attempts:
        name, arm, attempt = row["name"], row["arm"], row["attempt"]
        entry = prepared["cases"][name]
        stem = f"attempt_{attempt:04d}_{name}_{arm}"
        need(row["schema"] == "mhgp9_s4a_hot_quarter_case_v1" and
             row["argv"] == command(name, arm, prepared["binary"]) and
             row["stdout_file"] == f"{stem}.stdout" and
             row["stderr_file"] == f"{stem}.stderr" and
             row["binary_sha256_before"] == BINARY_SHA and
             row["source_objects_before"] == SOURCE_OBJECTS and
             row["input_sha256_before"] == entry["sha256"],
             f"attempt metadata: {stem}")
        stdout_file = HERE / row["stdout_file"]
        stderr_file = HERE / row["stderr_file"]
        need(sha(stdout_file) == row["stdout_sha256"] and
             sha(stderr_file) == row["stderr_sha256"],
             f"attempt output hash: {stem}")
        valid = (not row["timed_out"] and row["exit_code"] == 0 and
                 row["binary_sha256_after"] == BINARY_SHA and
                 row["source_objects_after"] == SOURCE_OBJECTS and
                 row["input_sha256_after"] == entry["sha256"] and
                 row["post_identity_error"] is None)
        probe = None
        if valid:
            try:
                probe = json.loads(stdout_file.read_text())
                check_probe(probe, name, arm, entry)
            except Exception:
                valid = False
        need(output_validated(row) is valid and
             ((row["validation_error"] is None) if valid else
              (isinstance(row["validation_error"], str) and bool(row["validation_error"]))),
             f"attempt validation flag: {stem}")
        accepted = timing_accepted(row)
        if "timing_reason" in row:
            need((row["timing_reason"] is None) if accepted else
                 (isinstance(row["timing_reason"], str) and bool(row["timing_reason"])),
                 f"attempt timing reason: {stem}")
        if valid and accepted:
            key = (name, arm)
            need(key not in saved, f"duplicate successful case: {key}")
            saved[key] = (row, probe)
    need(set(saved) == wanted, "six successful cases required")
    expected_files = {row[key] for row in attempts for key in ("stdout_file", "stderr_file")}
    existing_files = {path.name for pattern in ("attempt_*.stdout", "attempt_*.stderr")
                      for path in HERE.glob(pattern)}
    need(existing_files == expected_files, "unreferenced or missing attempt output")
    probes = {}
    for key, (_, probe) in saved.items():
        probes[key] = probe
    for name in SIZES:
        a, b = probes[(name, "s3")], probes[(name, "s4a")]
        need(a["tower_digest"] == b["tower_digest"] and
             a["catalogue_digest"] == b["catalogue_digest"] and
             a["orders"] == b["orders"] and a["generator"] == b["generator"] and
             a["catalogue"] == b["catalogue"] and
             all(a["ledger"][key] == b["ledger"][key] for key in LEDGER_EQUAL),
             f"S3/S4a object or structural work differs: {name}")
    sizes = [prepared["cases"][name]["sites"] for name in SIZES]
    summary = {"schema": "mhgp9_s4a_hot_quarter_summary_v1",
               "status": "complete_relative", "sites": sizes,
               "attempts": len(attempts), "successful_cases": len(saved),
               "timing_rejected_attempts": [r["attempt"] for r in attempts
                                            if not timing_accepted(r)],
               "output_rejected_attempts": [r["attempt"] for r in attempts
                                            if not output_validated(r)],
               "cases": {}, "slopes": {}}
    for arm in ARMS:
        arm_probes = [probes[(name, arm)] for name in SIZES]
        summary["slopes"][arm] = {}
        for field in WORK_FIELDS:
            values = [p["ledger"][field] for p in arm_probes]
            summary["slopes"][arm][field] = {"values": values,
                                              "p": slope(values, sizes) if all(values) else None}
        for field, source in (("chain_cpu_s", lambda p: p["chain_cpu_s"]),
                              ("chain_total_ms", lambda p: p["times_ms"]["chain_total"]),
                              ("catalogue_balls", lambda p: p["catalogue"]["balls"])):
            values = [source(p) for p in arm_probes]
            summary["slopes"][arm][field] = {"values": values, "p": slope(values, sizes)}
    for name in SIZES:
        summary["cases"][name] = {}
        for arm in ARMS:
            p = probes[(name, arm)]
            summary["cases"][name][arm] = {
                "tower_digest": p["tower_digest"],
                "catalogue_digest": p["catalogue_digest"],
                "chain_total_ms": p["times_ms"]["chain_total"],
                "chain_cpu_s": p["chain_cpu_s"],
                "external_wall_s": saved[(name, arm)][0]["wall_s"],
                "q34_batch": p["q34_batch"],
                "ledger": {key: p["ledger"][key] for key in WORK_FIELDS},
            }
    path = HERE / "SUMMARY.json"
    blob = (json.dumps(summary, indent=2, sort_keys=True) + "\n").encode()
    if path.exists():
        need(path.read_bytes() == blob, "summary differs")
    else:
        path.write_bytes(blob)
    files = [HERE / "run_and_check.py", HERE / "PREPARED.json", HERE / "CASES.jsonl",
             HERE / "SUMMARY.json", *(HERE / "inputs" / f"{name}.u32le" for name in SIZES),
             *(HERE / row[key] for row in attempts for key in ("stdout_file", "stderr_file"))]
    checksums = "".join(f"{sha(file)}  {file.relative_to(HERE)}\n" for file in files).encode()
    seal = HERE / "SHA256SUMS"
    if seal.exists():
        need(seal.read_bytes() == checksums, "receipt checksums differ")
    else:
        seal.write_bytes(checksums)
    print("verified six paired cases and two adjacent density links")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("prepare", "run", "verify"))
    parser.add_argument("--max-load", type=float, default=8.0,
                        help="refuse a new HGP case when one-minute host load exceeds this")
    parser.add_argument("--timeout", type=int, default=900)
    args = parser.parse_args()
    if args.mode == "prepare":
        prepare()
    elif args.mode == "run":
        run(args.max_load, args.timeout)
    else:
        verify()


if __name__ == "__main__":
    main()
