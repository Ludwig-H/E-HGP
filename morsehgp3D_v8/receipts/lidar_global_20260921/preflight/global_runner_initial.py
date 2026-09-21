#!/usr/bin/env python3
"""Global LiDAR q3/q4 CPU measurements; no catalogue, FULL, or GPU claim.

Reuse the tranche31 edge collector's input/pinning and interruption primitives
explicitly. Each invocation writes a fresh capture, never overwrites a result.
The small exhaustive C++ gate is the geometric judge; large runs check stream
structure, complete ledgers and paired digests, not an exhaustive oracle.
"""
from __future__ import annotations
import argparse
from copy import deepcopy
import json
import math
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

import run_q34_lidar_checks as edge
from run_p0_matrix import invoke, on_signal, parse_result, require, uint, utc_stamp, write_json
from run_q4_family_checks import digest, pins, read_json

ROOT = edge.ROOT
SOURCES = edge.SOURCES | {"morsehgp3D_v8/bench/run_wspd_q34_lidar.py"}
SCHEMA = "mhgp8_global_lidar_capture_v1"


def flatten(value, prefix=""):
    result = {}
    for key, item in value.items():
        if type(item) is dict:
            result.update(flatten(item, prefix+key+"."))
        elif type(item) is list:
            for i, count in enumerate(item):
                require(type(count) is int, "noninteger array counter")
                result[prefix+key+f"[{i}]"] = count
        else:
            result[prefix+key] = item
    return result


def validate(row, command):
    require(len(command) == 10 and Path(command[0]).name == "mhgp8_wspd_q34_probe", "global command")
    n, k, s, mask, backend, workers = map(int, command[2:8])
    require(row["schema"] == "mhgp8_wspd_q34_probe_v1" and row["status"] == "completed" and
            row["scope"] == "global_q3_q4_candidate_stream_not_catalogue_or_full" and
            row["backend"] == "cpu_reference" and row["profile"] == "quantized_u16_input_only" and
            row["public_status"] == "not_claimed", "global scope/status")
    require([row[key] for key in ("n", "kmax", "s", "mask", "q4_backend", "workers")] ==
            [n, k, s, mask, backend, workers] and row["front_mode"] == command[8] and
            row["output_mode"] == command[9], "global command/result mismatch")
    require(0 < n <= row["source_n"] and 1 <= k <= 10 and s > 0 and mask == 6 and
            backend in (28, 30) and workers > 0, "unsupported global measurement")
    for group in ("front", "work", "parallel", "cloud_work", "index_work", "memory"):
        for name, value in flatten(row[group]).items():
            uint(value, group+"."+name)
    for part in row["workers_work"]:
        for name, value in part.items():
            uint(value, "worker."+name)
    for name, value in row["timings_ms"].items():
        require(type(value) in (int, float) and math.isfinite(value) and value >= 0, "invalid time "+name)
    front, work, par, output = row["front"], row["work"], row["parallel"], row["output"]
    fw, q3 = front["work"], work["q3"]
    active = 6 & ((1 << min(k, 3))-1)
    require(front["total_unordered_pairs"] == n*(n-1)//2 and front["active_lane_mask"] == active,
            "front metadata differs")
    for lane in range(3):
        require(fw["rejected_pair_mass"][lane]+fw["residual_pair_mass"][lane] ==
                (n*(n-1)//2 if active & (1 << lane) else 0), "front lane partition")
    require(work["input_rectangles"] == fw["emitted_rectangles"] and
            work["q3_edges"] == fw["residual_pair_mass"][1] and
            work["q4_edges"] == fw["residual_pair_mass"][2] and
            work["expanded_pairs"] == work["q3_edges"]+work["q4_edges"]-work["both_edges"] == work["cover_builds"],
            "global expansion ledger")
    require(work["cover_sites"] == work["cover"]["admitted_sites"] <= n*work["cover_builds"] and
            work["max_cover_sites"] <= n and
            work["cover"]["admitted_sites"]+work["cover"]["rejected_sites"] == n*work["cover_builds"],
            "cover preparation ledger")
    require(q3["edge_queries"] == work["q3_edges"] and
            q3["census_point_tests"] == q3["census_inside_sites"]+q3["census_outside_sites"]+q3["census_shell_sites"] and
            q3["ball_builds"] == q3["seeds"] == q3["depth_rejections"]+q3["emitted"] and
            q3["emitted"] == work["q3_emitted"] == output["q3"] and
            q3["shell_ids"] <= work["payload_shell_ids"], "q3 census ledger")
    lane = work["local28"] if backend == 28 else work["window30"]
    unused = work["window30"] if backend == 28 else work["local28"]
    require(all(value == 0 for value in flatten(unused).values()), "unselected q4 engine did work")
    require(lane["sweep"]["emitted"] == work["q4_emitted"] == output["q4"] and
            lane["edge"]["seeds"] == lane["sweep"]["seed_queries"], "q4 output ledger")
    require(output["callbacks"] == output["q3"]+output["q4"] and
            output["support_ids"] == 3*output["q3"]+4*output["q4"] and
            output["shell_ids"] == work["payload_shell_ids"], "stream payload ledger")
    for key in ("callbacks", "q3", "q4", "support_ids", "shell_ids"):
        uint(output[key], "output."+key)
    for key in ("xor", "sum"):
        require(type(output[key]) is str and output[key] == format(int(output[key], 16), "x") and
                0 <= int(output[key], 16) < 1 << 64, "noncanonical stream digest")
    require(par["requested_workers"] == workers and par["target_jobs"] == workers*16 and
            par["started_workers"] == len(row["workers_work"]) == min(workers, par["jobs"]) and
            par["completed_jobs"] == par["jobs"] == sum(w["jobs"] for w in row["workers_work"]), "parallel jobs ledger")
    for field in ("input_rectangles", "expanded_pairs", "q3_emitted", "q4_emitted"):
        require(sum(w[field] for w in row["workers_work"]) == work[field], "worker reduction "+field)
    require(sum(w["front_products"] for w in row["workers_work"])+par["prefix_product_visits"] == fw["product_visits"] and
            sum(w["peak_edge_buffer_bytes"] for w in row["workers_work"]) == par["edge_buffer_bytes_sum"], "parallel sums")
    times = row["timings_ms"]
    require(abs(times["cloud"]+times["index"]+times["front_edges_collect"]-
                times["pipeline_including_shared_preparation"]) <= 0.000003 and
            abs(times["load_prefix_hash"]+times["pipeline_including_shared_preparation"]+
                times["record_normalization"]-times["total_before_serialization_and_release"]) <= 0.000003,
            "global timing partition")
    if command[9] == "records":
        require(len(row["records"]) == output["callbacks"], "records missing")
        hashes = []
        for record in row["records"]:
            arity, depth, support, shell = [record[key] for key in ("arity", "depth", "support", "shell")]
            require(arity in (3, 4) and 0 <= depth < k+2-arity and len(support) == arity and
                    support == sorted(set(support)) and shell == sorted(set(shell)) and set(support) <= set(shell) and
                    all(type(i) is int and 0 <= i < n for i in shell), "invalid full record")
            h = edge.word(edge.word(14695981039346656037, arity), depth)
            for i in support: h = edge.word(h, i)
            for value in record["coefficients"]:
                coefficient = int(value) % (1 << 128)
                h = edge.word(edge.word(h, coefficient & edge.MASK64), coefficient >> 64)
            for i in shell: h = edge.word(h, i)
            hashes.append(edge.word(h, len(shell)))
        x = 0
        for h in hashes: x ^= h
        require(format(x, "x") == output["xor"] and format(sum(hashes) & edge.MASK64, "x") == output["sum"],
                "records/digest differ")


def paired(rows):
    by_output, by_work = {}, {}
    for row in rows:
        key = (row["input_hash"], row["n"], row["kmax"], row["mask"])
        require(key not in by_output or by_output[key] == row["output"], "paired outputs differ")
        by_output[key] = row["output"]
        discrete = deepcopy(row["work"])
        discrete["q3"]["peak_shell_bytes"] = 0
        discrete["peak_edge_buffer_bytes"] = 0
        key += (row["s"], row["front_mode"], row["q4_backend"])
        value = (row["front"], discrete)
        require(key not in by_work or by_work[key] == value, "paired geometry differs")
        by_work[key] = value


def summary(rows):
    paired(rows)
    growth = []
    groups = {}
    for row in rows:
        key = (row["scan"], row["kmax"], row["s"], row["q4_backend"], row["workers"])
        groups.setdefault(key, []).append(row)
    for key, values in sorted(groups.items()):
        values.sort(key=lambda row: row["n"])
        for a, b in zip(values, values[1:]):
            fields = {"front": a["front"]["work"], "work": a["work"], "timings_ms": a["timings_ms"]}
            before = flatten(fields)
            after = flatten({"front": b["front"]["work"], "work": b["work"], "timings_ms": b["timings_ms"]})
            ratios = {field: after[field]/value if value else None for field, value in before.items()}
            growth.append(dict(scan=key[0], kmax=key[1], s=key[2], backend=key[3], workers=key[4],
                               n=[a["n"], b["n"]], ratios=ratios))
    return dict(status="passed", measurements=len(rows), growth=growth,
                full_contract_qualified=False, universal_subquadratic_claim=False)


def run(args):
    build = args.build.resolve()
    require(build.is_relative_to(ROOT/"build") and build.is_dir(), "fresh local build required")
    require(len(set(args.sizes)) == len(args.sizes) and all(0 < n <= 50000 for n in args.sizes), "prefix sizes")
    require(all(k in (5, 10) for k in args.kmax) and all(s in (8, 10, 12) for s in args.s) and
            all(w > 0 for w in args.workers) and all(b in (28, 30) for b in args.backends) and
            all(scan in (0, 100, 200) for scan in args.scans), "unsupported matrix")
    args.output.mkdir(parents=True, exist_ok=True)
    path = Path(tempfile.mkdtemp(prefix="global_", dir=args.output)).resolve()
    files, commands, datasets = set(), [], []
    for scan in args.scans:
        directory = ROOT/edge.PREPARED/f"single_{scan:06}"
        metadata = read_json(directory/"METADATA.json")
        require(metadata["frames"] == [scan] and metadata["frame"] == "LiDAR_scan_000000", "wrong scan frame")
        files.add(str((directory/"METADATA.json").relative_to(ROOT)))
        for n in args.sizes:
            source_n = next(size for size in (8000, 16000, 32000, 50000) if size >= n)
            source = directory/f"n{source_n}.u16le"
            raw = source.read_bytes()
            entry = next(row for row in metadata["samples"] if row["n"] == source_n)
            require(len(raw) == 6*source_n and digest(source) == entry["sha256"], "LiDAR bytes mismatch")
            points = list(edge.struct.iter_unpack("<HHH", raw[:6*n]))
            datasets.append(dict(scan=scan, n=n, source=str(source), input_hash=edge.input_hash(points)))
            files.add(str(source.relative_to(ROOT)))
            for k in args.kmax:
                for s in args.s:
                    for backend in args.backends:
                        for workers in args.workers:
                            commands.append([str(build/"mhgp8_wspd_q34_probe"), str(source), str(n), str(k), str(s),
                                             "6", str(backend), str(workers), "samples", args.payload])
    artifacts = {str((build/name).relative_to(ROOT)) for name in ("mhgp8_wspd_q34_probe", "libmhgp8_p0.a", "CMakeCache.txt")}
    manifest = dict(schema=SCHEMA, started_utc=utc_stamp(), commands=commands, datasets=datasets,
                    source_sha256=pins(SOURCES), artifact_sha256=pins(artifacts), input_sha256=pins(files),
                    environment=edge.environment_record(dict(os.environ)), affinity=sorted(os.sched_getaffinity(0)),
                    command=[sys.executable, *sys.argv], git_commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                    gcp_used=False, public_status="not_claimed", full_contract_qualified=False)
    write_json(path/"MANIFEST.json", manifest)
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, rows = [], []
    status, error = "failed", None
    try:
        for i, command in enumerate(commands):
            print(json.dumps(dict(path=str(path), index=i, command=command)), flush=True)
            record = dict(command=command, started_utc=utc_stamp(), status="failed", exit_code=None,
                          stdout="", stderr="", stdout_base64="", stderr_base64="")
            try:
                invoke(command, dict(os.environ), ROOT, record, new_session=True)
                require(record["exit_code"] == 0 and not record["stderr"], "global command failed")
                row = parse_result(record["stdout"].encode())
                validate(row, command)
                dataset = next(d for d in datasets if d["source"] == command[1] and d["n"] == row["n"])
                require(row["input_hash"] == dataset["input_hash"], "constructor input hash differs")
                row["scan"] = dataset["scan"]
                record["result"] = row
                record["status"] = "passed"
                rows.append(row)
                print(json.dumps(dict(index=i, n=row["n"], ms=row["timings_ms"]["pipeline_including_shared_preparation"],
                                      edges=row["work"]["expanded_pairs"], outputs=row["output"]["callbacks"])), flush=True)
            finally:
                record["finished_utc"] = utc_stamp()
                file = path/f"record_{i:04}.json"
                write_json(file, record)
                records.append(dict(path=file.name, sha256=digest(file)))
        summary(rows)
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in handlers: signal.signal(sig, signal.SIG_IGN)
        completion = dict(status=status, error=error, finished_utc=utc_stamp(), records=records,
                          manifest_sha256=digest(path/"MANIFEST.json"), source_sha256_after=pins(SOURCES),
                          artifact_sha256_after=pins(artifacts), input_sha256_after=pins(files))
        for key in ("source_sha256", "artifact_sha256", "input_sha256"):
            if manifest[key] != completion[key+"_after"]:
                completion.update(status="failed", error=error or "capture closure changed")
        write_json(path/"COMPLETION.json", completion)
        for sig, handler in handlers.items(): signal.signal(sig, handler)
        print(json.dumps(dict(path=str(path), status=completion["status"], error=completion["error"])), flush=True)
    require(completion["status"] == "passed", "capture closure failed")


def read(path, live=False):
    m, c = read_json(path/"MANIFEST.json"), read_json(path/"COMPLETION.json")
    require(m["schema"] == SCHEMA and m["gcp_used"] is False and m["full_contract_qualified"] is False and
            m["public_status"] == "not_claimed" and c["status"] == "passed" and c["error"] is None and
            digest(path/"MANIFEST.json") == c["manifest_sha256"], "capture identity/closure")
    require(set(m["source_sha256"]) == SOURCES, "source inventory")
    for key in ("source_sha256", "artifact_sha256", "input_sha256"):
        require(m[key] == c[key+"_after"], "closure hash differs")
        if live: require(m[key] == pins(m[key]), "live hash differs")
    require(len(c["records"]) == len(m["commands"]), "incomplete command inventory")
    rows = []
    for i, (item, command) in enumerate(zip(c["records"], m["commands"], strict=True)):
        require(item["path"] == f"record_{i:04}.json" and digest(path/item["path"]) == item["sha256"], "record closure")
        record = read_json(path/item["path"])
        require(record["status"] == "passed" and record["exit_code"] == 0 and record["command"] == command and
                not record["stderr"] and edge.base64.b64decode(record["stdout_base64"], validate=True).decode() == record["stdout"], "command/raw mismatch")
        row = parse_result(record["stdout"].encode())
        validate(row, command)
        dataset = next(d for d in m["datasets"] if d["source"] == command[1] and d["n"] == row["n"])
        require(row["input_hash"] == dataset["input_hash"], "input hash mismatch")
        row["scan"] = dataset["scan"]
        require(row == record["result"], "parsed/raw mismatch")
        rows.append(row)
    return summary(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    runner = sub.add_parser("run")
    runner.add_argument("--build", type=Path, required=True)
    runner.add_argument("--output", type=Path, required=True)
    runner.add_argument("--scans", type=int, nargs="+", default=[0])
    runner.add_argument("--sizes", type=int, nargs="+", required=True)
    runner.add_argument("--kmax", type=int, nargs="+", default=[10])
    runner.add_argument("--s", type=int, nargs="+", default=[8])
    runner.add_argument("--workers", type=int, nargs="+", default=[1, 4])
    runner.add_argument("--backends", type=int, nargs="+", default=[28, 30])
    runner.add_argument("--payload", choices=("digest", "records"), default="digest")
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    reader.add_argument("--check-live", action="store_true")
    args = parser.parse_args()
    if args.operation == "run": run(args)
    else: print(json.dumps(read(args.path, args.check_live), sort_keys=True, allow_nan=False))


if __name__ == "__main__": main()
