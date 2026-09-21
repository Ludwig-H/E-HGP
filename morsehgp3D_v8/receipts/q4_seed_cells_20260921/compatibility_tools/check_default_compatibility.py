#!/usr/bin/env python3
"""Explicit port of constructor33 DEFAULT_COMPATIBILITY, now schemas 1/2/3.

Eight paired old CLIs, actual full records, no projection to schema4. The source
closure covers compiler-reported C++ prerequisites and the imported readers,
not the concurrent constructor34 gates. No build or cloud operation is run.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_q34_affine_checks as affine
import run_q34_indexed_checks as indexed
import run_wspd_q34_lidar as legacy
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import invoke, on_signal, parse_result, require, utc_stamp, write_json

BUILDS = (ROOT / "build/v8_q34_affine_20260921", ROOT / "build/v8_q4_seed_cells_20260921")
AUTHORITY = ROOT / "morsehgp3D_v8/receipts/q34_affine_20260921/candidate/smoke_jeqq5w9w"
# K, q4 backend, workers, old optional arguments. All use n64/s8/scan0/records.
CASES = ((5, 28, 1, ()), (10, 30, 4, ()),
    (5, 30, 1, ("pair", "scalar")), (10, 28, 4, ("rectangle-pair", "boxes")),
    (5, 28, 4, ("pair", "boxes", "legacy")),
    (10, 30, 1, ("rectangle-pair", "scalar", "exclude")),
    (5, 30, 4, ("rectangle-pair", "boxes", "affine")),
    (10, 28, 1, ("pair", "boxes", "affine")))
CAPACITIES = {"work.peak_edge_buffer_bytes", "work.q3.peak_shell_bytes",
    "parallel.edge_buffer_bytes_sum", "memory.worker_record_capacity_bytes_before_merge"}
WORKER_FIELDS = {"jobs", "front_products", "input_rectangles", "expanded_pairs",
    "q3_emitted", "q4_emitted", "peak_edge_buffer_bytes"}


def relative(path):
    return str(Path(path).resolve().relative_to(ROOT))


def python_sources():
    return {relative(Path(module.__file__)) for module in tuple(sys.modules.values())
        if getattr(module, "__file__", None) and Path(module.__file__).resolve().is_relative_to(ROOT)
        and Path(module.__file__).suffix == ".py"}


def plan():
    matrix = dict(scans=[0], sizes=[64], kmax=[5], s=[8], workers=[1], backends=[28], payload="records")
    datasets, unused, inputs = legacy.prepare_matrix(BUILDS[0], matrix)
    require(len(datasets) == 1 and len(unused) == 1, "input preparation differs")
    source = datasets[0]["source"]
    commands = [(arm, [str(build / "mhgp8_wspd_q34_probe"), source, "64", str(k), "8", "6",
        str(backend), str(workers), "samples", "records", *options])
        for k, backend, workers, options in CASES
        for arm, build in zip(("old", "new"), BUILDS, strict=True)]
    return datasets, inputs, commands


def dependency_sources(dependencies):
    result = set()
    for content in dependencies.values():
        words = shlex.split(content.replace("\\\n", " "))
        require(words and words[0].endswith(":"), "unexpected compiler dependency syntax")
        for name in words[1:]:
            path = Path(name)
            if path.is_absolute() and path.is_relative_to(ROOT):
                require(path.suffix in (".cpp", ".hpp", ".h"), "unexpected project prerequisite")
                result.add(relative(path))
    require("morsehgp3D_v8/src/lanes/q4_seed_cells.hpp" in result and
        "morsehgp3D_v8/bench/wspd_q34_probe.cpp" in result, "new prerequisite graph incomplete")
    return result


def artifacts(dependencies):
    result = legacy.artifacts_for(BUILDS[0]) | legacy.artifacts_for(BUILDS[1])
    for target in ("mhgp8_p0", "mhgp8_wspd_q34_probe"):
        for name in ("flags.make", "link.txt"):
            result.add(relative(BUILDS[1] / "CMakeFiles" / (target + ".dir") / name))
    for name in dependencies:
        result.update((name, name[:-2]))  # .o.d and the corresponding .o
    return result


def verify_authority(artifact_hashes):
    m, c = read_json(AUTHORITY / "MANIFEST.json"), read_json(AUTHORITY / "COMPLETION.json")
    require(c["status"] == "passed" and c["error"] is None and c["closing_errors"] == [] and
        c["manifest_sha256"] == digest(AUTHORITY / "MANIFEST.json") and
        m["artifact_sha256"] == c["artifact_sha256_after"] and
        m["source_sha256"] == c["source_sha256_after"], "old authority not closed")
    require(all(m["artifact_sha256"][name] == artifact_hashes[name]
        for name in legacy.artifacts_for(BUILDS[0])), "old build differs from pinned33 authority")
    return {name: digest(AUTHORITY / name) for name in ("MANIFEST.json", "COMPLETION.json")}


def validate(row, command):
    {10: legacy.validate, 12: indexed.validate_row, 13: affine.validate_row}[len(command)](row, command)
    require(row["output_mode"] == "records" and len(row["records"]) == row["output"]["callbacks"],
        "full records missing")


def differences(a, b, path=""):
    if type(a) is not type(b):
        return [dict(path=path, old=a, new=b)]
    if type(a) is dict:
        require(a.keys() == b.keys(), "old schema field inventory changed at " + path)
        return [item for key in a for item in differences(a[key], b[key], path + ("." if path else "") + key)]
    if type(a) is list:
        require(len(a) == len(b), "old schema array length changed at " + path)
        return [item for i, (x, y) in enumerate(zip(a, b, strict=True))
            for item in differences(x, y, path + f"[{i}]")]
    return [] if a == b else [dict(path=path, old=a, new=b)]


def compare(old, new):
    # The worker decomposition may differ, never its sums. Validate its complete
    # field inventory before admitting per-slot assignment differences.
    for row in (old, new):
        require(all(set(w) == WORKER_FIELDS for w in row["workers_work"]), "worker shape changed")
    for field in WORKER_FIELDS - {"peak_edge_buffer_bytes"}:
        require(sum(w[field] for w in old["workers_work"]) ==
            sum(w[field] for w in new["workers_work"]), "worker total changed: " + field)
    observed = differences(old, new)
    for item in observed:
        path = item["path"]
        if path.startswith("timings_ms."):
            item["category"] = "timing_not_a_performance_claim"
        elif path.startswith("workers_work["):
            item["category"] = "worker_assignment_with_equal_totals"
        elif path in CAPACITIES:
            item["category"] = "scheduling_dependent_capacity"
        else:
            require(path == "parallel.worker_state_bytes", "logical work/payload changed: " + str(item))
            item["category"] = "measured_worker_state_capacity"
    workers = old["parallel"]["started_workers"]
    delta = new["parallel"]["worker_state_bytes"] - old["parallel"]["worker_state_bytes"]
    require(workers > 0 and delta >= 0 and delta % workers == 0, "worker-state capacity not integral per worker")
    return dict(schema=old["schema"], n=old["n"], kmax=old["kmax"], backend=old["q4_backend"],
        workers=old["workers"], full_records_front_logical_work_equal=True,
        worker_state_bytes=dict(old=old["parallel"]["worker_state_bytes"], new=new["parallel"]["worker_state_bytes"],
            delta=delta, delta_per_started_worker=delta // workers), observed_differences=observed,
        output=old["output"])


def fresh_record(command):
    return dict(command=command, cwd=str(ROOT), started_utc=utc_stamp(), status="failed", exit_code=None,
        stdout="", stderr="", stdout_base64="", stderr_base64="")


def execute(command, environment, record):
    invoke(command, environment, ROOT, record, new_session=True)
    require(record["exit_code"] == 0 and not record["stderr"], "command failed")
    return parse_result(record["stdout"].encode())


def close_pins(result, before):
    errors = []
    for key, hashes in before.items():
        result[key] = hashes
        try:
            result[key + "_after"] = pins(hashes)
        except Exception as cause:
            errors.append(f"{key}: {type(cause).__name__}: {cause}")
            result[key + "_after"] = None
    result["closing_errors"] = errors
    if errors or any(result[key] != result[key + "_after"] for key in before):
        result.update(status="failed", error=result["error"] or "pin closure changed")


def run():
    capture = Path(tempfile.mkdtemp(prefix="compatibility_", dir=HERE.parent)).resolve()
    print(json.dumps(dict(capture=str(capture))), flush=True)
    environment = dict(os.environ)
    records, comparisons, before = [], [], {}
    result = dict(schema="mhgp8_default33_34_compatibility_v1", status="failed", error=None,
        started_utc=utc_stamp(), environment=affine.edge.environment_record(environment),
        affinity=sorted(os.sched_getaffinity(0)), command=[sys.executable, *sys.argv],
        git_commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        source_scope="new build C++ dependency graph + CMake + imported Python, not full216 gate inventory",
        gcp_used=False, full_contract_qualified=False, records=records, comparisons=comparisons)
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    try:
        datasets, inputs, commands = plan()
        dependency_files = sorted(path for target in ("mhgp8_p0", "mhgp8_wspd_q34_probe")
            for path in (BUILDS[1] / "CMakeFiles" / (target + ".dir")).rglob("*.o.d"))
        require(len(dependency_files) == 25, "expected24 library objects and one probe dependency file")
        dependencies = {relative(p): p.read_text() for p in dependency_files}
        sources = dependency_sources(dependencies) | python_sources() | {"morsehgp3D_v8/CMakeLists.txt"}
        inputs.update(relative(AUTHORITY / name) for name in ("MANIFEST.json", "COMPLETION.json"))
        before = dict(source_sha256=pins(sources), input_sha256=pins(inputs), artifact_sha256=pins(artifacts(dependencies)))
        result.update(datasets=datasets, commands=commands, dependencies=dependencies,
            authority=verify_authority(before["artifact_sha256"]), python_sources=sorted(python_sources()))
        result.update(before)
        write_json(capture / "LAUNCH.json", result)
        for number, (arm, command) in enumerate(commands):
            record = fresh_record(command)
            record["arm"] = arm
            try:
                row = execute(command, environment, record)
                validate(row, command)
                require(row["input_hash"] == datasets[0]["input_hash"], "input FNV mismatch")
                record["row"] = row
                if arm == "new":
                    comparisons.append(compare(records[-1]["row"], row))
                record["status"] = "passed"
            finally:
                record["finished_utc"] = utc_stamp()
                records.append(record)
                write_json(capture / f"record_{number:04}.json", record)
            print(json.dumps(dict(index=number, arm=arm, schema=row["schema"], outputs=row["output"]["callbacks"])), flush=True)
        require(len(records) == 16 and len(comparisons) == 8 and
            len({p["worker_state_bytes"]["delta_per_started_worker"] for p in comparisons}) == 1,
            "incomplete pairs or inconsistent per-worker state growth")
        result["status"] = "passed"
    except BaseException as cause:
        result["error"] = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        result["finished_utc"] = utc_stamp()
        close_pins(result, before)
        result["record_sha256"] = {f"record_{i:04}.json": digest(capture / f"record_{i:04}.json") for i in range(len(records))}
        write_json(capture / "DEFAULT_COMPATIBILITY.json", result)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(capture=str(capture), status=result["status"], error=result["error"], pairs=len(comparisons))), flush=True)
    require(result["status"] == "passed", "compatibility capture failed")


def read(capture, check_live):
    result = read_json(capture / "DEFAULT_COMPATIBILITY.json")
    datasets, inputs, commands = plan()
    require(result["schema"] == "mhgp8_default33_34_compatibility_v1" and result["status"] == "passed" and
        result["error"] is None and result["closing_errors"] == [] and not result["gcp_used"] and
        not result["full_contract_qualified"] and result["datasets"] == datasets and
        result["commands"] == [[arm, command] for arm, command in commands], "capture identity/plan/closure differs")
    inputs.update(relative(AUTHORITY / name) for name in ("MANIFEST.json", "COMPLETION.json"))
    sources = dependency_sources(result["dependencies"]) | python_sources() | {"morsehgp3D_v8/CMakeLists.txt"}
    inventories = dict(source_sha256=sources, input_sha256=inputs, artifact_sha256=artifacts(result["dependencies"]))
    for key, names in inventories.items():
        require(set(result[key]) == names and result[key] == result[key + "_after"], "pin inventory/closure differs")
        if check_live:
            require(result[key] == pins(names), "live hashes differ")
    require(result["python_sources"] == sorted(python_sources()) and
        all(result["source_sha256"][name] == digest(ROOT / name) for name in python_sources()), "reader source changed")
    for name, raw in result["dependencies"].items():
        require(hashlib.sha256(raw.encode()).hexdigest() == result["artifact_sha256"][name], "dependency bytes/hash differ")
    require(result["authority"] == verify_authority(result["artifact_sha256"]), "old authority differs")
    require(len(result["records"]) == 16 and set(result["record_sha256"]) ==
        {f"record_{i:04}.json" for i in range(16)}, "native record inventory differs")
    comparisons = []
    for i, ((arm, command), record) in enumerate(zip(commands, result["records"], strict=True)):
        path = capture / f"record_{i:04}.json"
        require(digest(path) == result["record_sha256"][path.name] and read_json(path) == record and
            record["command"] == command and record["arm"] == arm and record["cwd"] == str(ROOT) and
            record["status"] == "passed" and type(record["exit_code"]) is int and
            record["exit_code"] == 0 and not record["stderr"], "native command/raw binding differs")
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(record[stream + "_base64"], validate=True).decode(errors="replace") == record[stream],
                "raw/decoded bytes differ")
        row = parse_result(record["stdout"].encode())
        validate(row, command)
        require(row == record["row"] and row["input_hash"] == datasets[0]["input_hash"], "row/input mismatch")
        if arm == "new":
            comparisons.append(compare(result["records"][i-1]["row"], row))
    require(comparisons == result["comparisons"], "recorded differences changed")
    deltas = {p["worker_state_bytes"]["delta_per_started_worker"] for p in comparisons}
    require(len(deltas) == 1, "per-worker state delta inconsistent")
    return dict(status="passed", commands=16, pairs=8, schemas=[1, 2, 3], full_records_equal=True,
        logical_work_front_equal=True, worker_state_bytes_delta_per_started_worker=deltas.pop(),
        full_contract_qualified=False, sources=len(sources), artifacts=len(result["artifact_sha256"]))


def readback(capture):
    output = capture / "DEFAULT_COMPATIBILITY_READBACK.json"
    require(not output.exists(), "refuse overwriting readback")
    original = read_json(capture / "DEFAULT_COMPATIBILITY.json")
    files = set(original["input_sha256"]) | {relative(p) for p in capture.glob("*.json")}
    before = dict(source_sha256=pins(original["source_sha256"]), input_sha256=pins(files),
        artifact_sha256=pins(original["artifact_sha256"]))
    records, values = [], []
    result = dict(schema="mhgp8_default33_34_readback_v1", status="failed", error=None,
        started_utc=utc_stamp(), records=records, results=values, native_reexecutions=0,
        environment=affine.edge.environment_record(dict(os.environ)))
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    try:
        for optimized in (False, True):
            for live in (False, True):
                command = [sys.executable, "-B", *(["-O"] if optimized else []), str(Path(__file__).resolve()),
                    "read", str(capture), *(["--check-live"] if live else [])]
                record = fresh_record(command)
                try:
                    value = execute(command, dict(os.environ), record)
                    require(value["status"] == "passed", "reader verdict failed")
                    values.append(value)
                    record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp()
                    records.append(record)
        require(len(values) == 4 and all(x == values[0] for x in values), "normal/optimized/live mismatch")
        result["status"] = "passed"
    except BaseException as cause:
        result["error"] = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        result["finished_utc"] = utc_stamp()
        close_pins(result, before)
        write_json(output, result)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(status=result["status"], error=result["error"], reads=len(values),
            result=values[0] if values else None)), flush=True)
    require(result["status"] == "passed", "readback closure failed")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=("run", "read", "readback"))
    parser.add_argument("capture", nargs="?", type=Path)
    parser.add_argument("--check-live", action="store_true")
    args = parser.parse_args()
    require((args.operation == "run") == (args.capture is None), "capture argument mismatch")
    if args.operation == "run":
        run()
    elif args.operation == "read":
        print(json.dumps(read(args.capture.resolve(), args.check_live), sort_keys=True))
    else:
        readback(args.capture.resolve())


if __name__ == "__main__":
    main()
