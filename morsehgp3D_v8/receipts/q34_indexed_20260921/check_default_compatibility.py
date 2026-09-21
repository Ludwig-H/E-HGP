#!/usr/bin/env python3
"""Sixteen bounded native commands compare the unchanged legacy v1 entry.

Old/new pinned executables are only read/executed, never rebuilt. Every actual
JSON difference is retained, including capacities and worker assignment; no
memory field is silently omitted. Reuses constructor31 validation explicitly.
"""
import argparse
import base64
from copy import deepcopy
import json
import os
from pathlib import Path
import signal
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_q34_indexed_checks as current
import run_wspd_q34_lidar as legacy
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import invoke, on_signal, parse_result, require, utc_stamp, write_json

OUTPUT = HERE / "DEFAULT_COMPATIBILITY.json"
BUILDS = (ROOT / "build/v8_lidar_global_r2_20260921", ROOT / "build/v8_q34_indexed_20260921")
AUTHORITIES = (ROOT / "morsehgp3D_v8/receipts/lidar_global_20260921/regression_0cd1l_3e",
               HERE / "qualifications/smoke_2x8dvljr")
MATRIX = dict(scans=[0], sizes=[64,128], kmax=[5,10], s=[8], workers=[1,4], backends=[28], payload="records")


def plan():
    dataset, old, files = legacy.prepare_matrix(BUILDS[0], MATRIX)
    other, new, more_files = legacy.prepare_matrix(BUILDS[1], MATRIX)
    require(dataset == other and files == more_files and len(old) == len(new) == 8, "paired inputs differ")
    return dataset, files, [(arm, command) for pair in zip(old, new, strict=True)
                             for arm, command in zip(("old", "new"), pair, strict=True)]


def differences(a, b, path=""):
    if type(a) is not type(b):
        return [dict(path=path, old=a, new=b)]
    if type(a) is dict:
        require(a.keys() == b.keys(), "legacy JSON field inventory changed")
        return [item for key in a for item in differences(a[key], b[key], path + ("." if path else "") + key)]
    if type(a) is list:
        require(len(a) == len(b), "legacy JSON array length changed")
        return [item for i, (x,y) in enumerate(zip(a,b,strict=True))
                for item in differences(x,y,path+f"[{i}]" )]
    return [] if a == b else [dict(path=path, old=a, new=b)]


def compare(old, new):
    observed = differences(old, new)
    # Per-worker capacity maxima are scheduling-dependent by the public API;
    # keep their numerical differences, while comparing every geometric count.
    capacity = {"work.peak_edge_buffer_bytes", "work.q3.peak_shell_bytes", "parallel.edge_buffer_bytes_sum",
                "memory.worker_record_capacity_bytes_before_merge"}
    unknown = [item for item in observed if not (item["path"].startswith(("timings_ms.", "workers_work[")) or
               item["path"] in capacity or item["path"] == "parallel.worker_state_bytes")]
    require(not unknown, "default logical work/front/payload changed: " + str(unknown[:3]))
    workers = old["parallel"]["started_workers"]
    delta = new["parallel"]["worker_state_bytes"] - old["parallel"]["worker_state_bytes"]
    require(workers > 0 and delta == 704*workers, "unexpected explicitly paid worker-state growth")
    for item in observed:
        item["category"] = ("wall_time_not_performance_claim" if item["path"].startswith("timings_ms.") else
            "worker_assignment" if item["path"].startswith("workers_work[") else
            "scheduling_dependent_capacity" if item["path"] in capacity else "paid_worker_state_capacity")
    return dict(n=old["n"], kmax=old["kmax"], workers=old["workers"], s=old["s"],
        logical_work_front_full_records_equal=True, observed_differences=observed,
        worker_state_bytes=dict(old=old["parallel"]["worker_state_bytes"], new=new["parallel"]["worker_state_bytes"],
                                delta=delta, delta_per_started_worker=delta//workers), output=old["output"])


def verify_authorities(artifact_hashes):
    result = []
    for build, authority in zip(BUILDS, AUTHORITIES, strict=True):
        m, c = read_json(authority / "MANIFEST.json"), read_json(authority / "COMPLETION.json")
        require(c["status"] == "passed" and c["error"] is None and c["closing_errors"] == [] and
                c["manifest_sha256"] == digest(authority / "MANIFEST.json") and
                m["artifact_sha256"] == c["artifact_sha256_after"] and
                m["source_sha256"] == c["source_sha256_after"], "baseline authority not closed")
        for name in legacy.artifacts_for(build):
            require(m["artifact_sha256"][name] == artifact_hashes[name], "probe/library/cache no longer matches authority")
        result.append(dict(path=str(authority.relative_to(ROOT)), manifest_sha256=digest(authority / "MANIFEST.json"),
                           completion_sha256=digest(authority / "COMPLETION.json")))
    return result


def run():
    require(not OUTPUT.exists(), "refuse overwriting a compatibility capture")
    datasets, inputs, commands = plan()
    inputs.add(str(Path(__file__).resolve().relative_to(ROOT)))
    inputs.update(str((path/name).relative_to(ROOT)) for path in AUTHORITIES for name in ("MANIFEST.json", "COMPLETION.json"))
    sources, files = pins(current.SOURCES), pins(inputs)
    artifacts = pins(legacy.artifacts_for(BUILDS[0]) | legacy.artifacts_for(BUILDS[1]))
    authorities = verify_authorities(artifacts)
    environment = dict(os.environ)
    started = utc_stamp()
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, pairs, status, error = [], [], "failed", None
    try:
        for number, (arm, command) in enumerate(commands):
            record = dict(arm=arm, command=command, cwd=str(ROOT), started_utc=utc_stamp(), status="failed",
                          exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
            try:
                invoke(command, environment, ROOT, record, new_session=True)
                require(record["exit_code"] == 0 and not record["stderr"], "native compatibility command failed")
                row = parse_result(record["stdout"].encode())
                legacy.validate(row, command)
                dataset = next(d for d in datasets if d["n"] == row["n"])
                require(row["input_hash"] == dataset["input_hash"], "prepared input differs")
                record["row"] = row
                if arm == "new":
                    pairs.append(compare(records[-1]["row"], row))
                record["status"] = "passed"
            finally:
                record["finished_utc"] = utc_stamp()
                records.append(record)
        require(len(records) == 16 and len(pairs) == 8, "incomplete compatibility plan")
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        errors = []
        def close(label, function):
            try:
                return function()
            except Exception as cause:
                errors.append(f"{label}: {type(cause).__name__}: {cause}")
                return None
        result = dict(schema="mhgp8_default31_32_compatibility_v1", status=status, error=error,
            started_utc=started, finished_utc=utc_stamp(), commands=[list(item) for item in commands],
            matrix=MATRIX, datasets=datasets, records=records, comparisons=pairs, authorities=authorities,
            environment=current.edge.environment_record(environment),
            source_sha256=sources, source_sha256_after=close("sources", lambda: pins(current.SOURCES)),
            input_sha256=files, input_sha256_after=close("inputs", lambda: pins(inputs)),
            artifact_sha256=artifacts, artifact_sha256_after=close("artifacts", lambda: pins(artifacts)),
            closing_errors=errors, full_contract_qualified=False, gcp_used=False,
            scope="small actual legacy-v1 default compatibility; reported memory/scheduling differences, not speed")
        if errors or any(result[key] != result[key + "_after"]
                         for key in ("source_sha256", "input_sha256", "artifact_sha256")):
            result["status"] = "failed"
            result["error"] = error or "compatibility closure changed"
        write_json(OUTPUT, result)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(status=result["status"], error=result["error"], commands=len(records), pairs=len(pairs))), flush=True)
    require(result["status"] == "passed", "default compatibility failed")


def read(check_live=False):
    capture = read_json(OUTPUT)
    datasets, files, commands = plan()
    files.add(str(Path(__file__).resolve().relative_to(ROOT)))
    files.update(str((path/name).relative_to(ROOT)) for path in AUTHORITIES for name in ("MANIFEST.json", "COMPLETION.json"))
    require(capture["schema"] == "mhgp8_default31_32_compatibility_v1" and capture["status"] == "passed" and
            capture["error"] is None and capture["closing_errors"] == [] and capture["gcp_used"] is False and
            capture["full_contract_qualified"] is False and capture["matrix"] == MATRIX and
            capture["datasets"] == datasets and capture["commands"] == [list(item) for item in commands] and
            len(capture["records"]) == 16, "capture plan/closure differs")
    inventories = dict(source_sha256=current.SOURCES, input_sha256=files,
                       artifact_sha256=legacy.artifacts_for(BUILDS[0]) | legacy.artifacts_for(BUILDS[1]))
    for key, names in inventories.items():
        require(set(capture[key]) == names and capture[key] == capture[key + "_after"], "pin inventory/closure differs")
        if check_live:
            require(capture[key] == pins(names), "live pins changed")
    require(capture["authorities"] == verify_authorities(capture["artifact_sha256"]), "authority differs")
    pairs = []
    for number, ((arm, command), record) in enumerate(zip(commands,capture["records"],strict=True)):
        require(record["arm"] == arm and record["command"] == command and record["cwd"] == str(ROOT) and
                record["status"] == "passed" and type(record["exit_code"]) is int and record["exit_code"] == 0 and
                not record["stderr"], "native command/result differs")
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(record[stream + "_base64"],validate=True).decode(errors="replace") == record[stream],
                    "raw/decoded output differs")
        row = parse_result(record["stdout"].encode())
        legacy.validate(row, command)
        require(row == record["row"] and row["input_hash"] == next(d for d in datasets if d["n"] == row["n"])["input_hash"],
                "row/input differs")
        if arm == "new":
            pairs.append(compare(capture["records"][number-1]["row"],row))
    require(pairs == capture["comparisons"], "recorded differences differ")
    return dict(status="passed", commands=16, pairs=8, full_records_equal=True,
        worker_state_bytes_delta_per_started_worker=704, full_contract_qualified=False,
        scheduling_capacity_differences=[dict(n=p["n"],kmax=p["kmax"],workers=p["workers"],difference=item)
            for p in pairs for item in p["observed_differences"] if item["category"] == "scheduling_dependent_capacity"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=("run", "read"))
    parser.add_argument("--check-live", action="store_true")
    args = parser.parse_args()
    if args.operation == "run":
        run()
    else:
        print(json.dumps(read(args.check_live), sort_keys=True, allow_nan=False))


if __name__ == "__main__":
    main()
