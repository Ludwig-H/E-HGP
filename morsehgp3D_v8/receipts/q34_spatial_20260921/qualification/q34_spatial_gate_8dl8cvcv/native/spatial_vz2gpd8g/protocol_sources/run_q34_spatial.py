#!/usr/bin/env python3
"""Closed measurements of all seven WHOLE quantized spatial LiDAR datasets.

No prefix, quota, implicit benchmark timeout, native build or cloud action.
The unchanged r2 probe measures q3/q4 candidate streams, NOT the full HGP tower.
The small native rational gates remain the geometry authority; this reader
checks exact stream structure, paid work, input identity and receipt closure.

Explicit validator port:
- run_wspd_q34_lidar.py strict_shape, SHA ad76a103dac4baeb16cf9be1d9a6d9ddea6770692e803d6720fb97312521676c
- run_q4_seed_cells_checks.py validate_row, SHA 35ce2c1bc056048ae323e5fee2302ba8c4d471be6324a1a5b32f64a898f5a14d
Only filename/prefix assumptions are replaced by whole spatial-dataset identity.
The actual row/command are never rewritten. The historical shape-only common
view is not an old execution. No monkeypatch of an old module is performed.
"""
from __future__ import annotations
import argparse
import base64
from copy import deepcopy
from datetime import datetime, timezone
import json
import math
import os
from pathlib import Path
import resource
import signal
import struct
import subprocess
import sys
import tempfile
import time

import prepare_lidar_spatial as preparation
import run_q4_seed_cells_checks as checks
from run_p0_matrix import InvalidReceipt, invoke, on_signal, parse_result, require, uint, utc_stamp, write_json
from run_q4_family_checks import digest, pins, read_json
from run_wspd_q34_lidar import (FIXED, FRONT_FIELDS, FRONT_ARRAYS, GLOBAL_FIELDS, Q3_FIELDS,
    PARALLEL_FIELDS, WORKER_FIELDS, MEMORY_FIELDS, TIME_FIELDS, OUTPUT_FIELDS, exact_fields)

ROOT, previous, edge = checks.ROOT, checks.previous, checks.edge
MODES, BOUNDS_MODES, SEED_MODES = checks.MODES, checks.BOUNDS_MODES, checks.SEED_MODES
WITNESS_FIELDS, Q3_BLOCK_FIELDS = checks.WITNESS_FIELDS, checks.Q3_BLOCK_FIELDS
validate_search, validate_auxiliary = checks.validate_search, checks.validate_auxiliary
validate_seed_cells = checks.validate_seed_cells
SCHEMA = "mhgp8_q34_spatial_capture_v1"
SOURCES = checks.SOURCES
PROTOCOL_SOURCES = frozenset({
    "morsehgp3D_v8/bench/run_q34_spatial.py",
    "morsehgp3D_v8/bench/prepare_lidar_spatial.py",
    "morsehgp3D_v8/tests/q34_spatial_gate.py",
})
ORDER = (*preparation.QUARTER_NAMES, *preparation.DATASET_NAMES[1:3], "full")
AUTHORITY = ROOT / "morsehgp3D_v8/receipts/q4_seed_cells_20260921/qualification_r2/smoke_ewedfs4y"
AUTHORITY_HASHES = {
    str(AUTHORITY / "MANIFEST.json"): "b4682fd3f59c9cb22266f8135c0f8ce08d256fc77c7c986eb8d3c580a1d1b0f0",
    str(AUTHORITY / "COMPLETION.json"): "282f048be8a8d0025f456a70b551e04dadcacb9411a0914ca7d1d104bcfae80f",
}


def strict_shape(row,command):
    require(type(command) is list and len(command)==10 and all(type(v) is str for v in command),
            "global command arity/types")
    require(command[8]=="samples" and command[9] in ("digest","records"),"unsupported global mode")
    scalars="n source_n kmax s mask q4_backend workers input_hash".split()
    nested="front work parallel workers_work cloud_work index_work memory output timings_ms front_mode output_mode".split()
    exact_fields(row,[*FIXED,*scalars,*nested,*(["records"] if command[9]=="records" else [])],"row")
    require(all(type(row[key]) is type(value) and row[key]==value for key,value in FIXED.items()),"fixed scope differs")
    for key in scalars:uint(row[key],key)
    require(row["n"] == row["source_n"] and row["n"] > 0,
            "spatial datasets must be measured whole, never by prefix")
    edge.structural(row["front"],("total_unordered_pairs","active_lane_mask"),("work",),"front")
    fw=row["front"]["work"];edge.structural(fw,FRONT_FIELDS,FRONT_ARRAYS,"front.work")
    for field,size in FRONT_ARRAYS.items():
        require(type(fw[field]) is list and len(fw[field])==size,"front array dimensions differ")
        for value in fw[field]:uint(value,"front."+field)
    work=row["work"];edge.structural(work,GLOBAL_FIELDS,("cover","q3","local28","window30"),"work")
    edge.counts(work["cover"],edge.previous.COVER_FIELDS,"cover")
    edge.counts(work["q3"],Q3_FIELDS,"q3")
    for name in ("local28","window30"):
        w=work[name]
        exact_fields(w,("edge","geometry","atlas","sweep") if name=="local28" else
            ("edge","geometry","selection","sweep","window"),name)
        edge.counts(w["edge"],edge.local.LOCAL_EDGE_FIELDS if name=="local28" else edge.EDGE_FIELDS,name+".edge")
        edge.structural(w["geometry"],edge.local.GEOMETRY_FIELDS,("domain",),name+".geometry")
        edge.counts(w["geometry"]["domain"],edge.local.DOMAIN_FIELDS,name+".domain")
        if name=="local28":
            a=w["atlas"];edge.structural(a,edge.local.ATLAS_FIELDS,("partition","domain"),"atlas")
            edge.counts(a["partition"],edge.local.PARTITION_FIELDS,"partition")
            edge.counts(a["domain"],edge.local.DOMAIN_QUERY_FIELDS,"atlas.domain")
            edge.counts(w["sweep"],edge.local.SWEEP_FIELDS,"local sweep")
        else:
            edge.counts(w["selection"],edge.SELECTION_FIELDS,"selection")
            edge.structural(w["sweep"],edge.SWEEP_FIELDS,("family",),"window.sweep")
            edge.counts(w["sweep"]["family"],edge.previous.FAMILY_FIELDS,"family")
            edge.counts(w["window"],edge.window.WINDOW_FIELDS,"window")
    edge.counts(row["parallel"],PARALLEL_FIELDS,"parallel")
    require(type(row["workers_work"]) is list,"workers_work must be a list")
    for worker in row["workers_work"]:edge.counts(worker,WORKER_FIELDS,"worker")
    edge.counts(row["cloud_work"],edge.previous.CLOUD_FIELDS,"cloud")
    edge.counts(row["index_work"],edge.previous.INDEX_FIELDS,"index")
    edge.counts(row["memory"],MEMORY_FIELDS,"memory")
    edge.structural(row["output"],OUTPUT_FIELDS,("xor","sum"),"output")
    exact_fields(row["timings_ms"],TIME_FIELDS,"timings")
    for key in ("xor","sum"):
        value=row["output"][key]
        require(type(value) is str and 1<=len(value)<=16 and all(c in "0123456789abcdef" for c in value)
            and value==format(int(value,16),"x"),"noncanonical digest")
    require(type(row["front_mode"]) is str and type(row["output_mode"]) is str,"invalid mode type")
    if command[9]=="records":
        require(type(row["records"]) is list,"records must be a list")
        last=None
        for record in row["records"]:
            exact_fields(record,("arity","depth","support","shell","coefficients"),"record")
            arity=uint(record["arity"],"record.arity");depth=uint(record["depth"],"record.depth")
            require(arity in (3,4) and depth<row["kmax"]+2-arity,"record arity/depth")
            for key in ("support","shell"):
                ids=record[key]
                require(type(ids) is list and all(type(v) is int and 0<=v<row["n"] for v in ids) and
                    ids==sorted(set(ids)),"invalid sorted IDs")
            require(len(record["support"])==arity and set(record["support"])<=set(record["shell"]),"support/shell")
            values=record["coefficients"]
            require(type(values) is list and len(values)==5 and all(type(v) is str for v in values),"coefficient fields")
            coefficients=[]
            for value in values:
                require(value and len(value)<=40 and (value.isascii() and
                    (value.isdecimal() or (value.startswith("-") and value[1:].isdecimal()))),"coefficient token")
                number=int(value)
                require(str(number)==value and -(1<<127)<=number<(1<<127),"noncanonical i128")
                coefficients.append(number)
            require(coefficients[0]>0 and math.gcd(*coefficients)==1,"nonprimitive exact ball")
            ordering=(arity,tuple(record["support"]),tuple(coefficients),depth,tuple(record["shell"]))
            require(last is None or last<ordering,"duplicate/unsorted normalized records")
            last=ordering



def validate_row(row, command, dataset):
    require(type(dataset) is dict and set(dataset) == {"name", "source", "n", "input_hash"} and
            dataset["name"] in ORDER and type(dataset["n"]) is int and dataset["n"] > 0,
            "invalid nonempty spatial dataset")
    require(type(command) is list and len(command) == 15 and command[1] == dataset["source"] and
            command[2] == str(dataset["n"]) and command[10:13] == ["rectangle-pair", "boxes", "affine"] and
            command[5:7] == ["6", "28"], "spatial command/dataset/profile mismatch")
    require(type(row) is dict and row.get("n") == row.get("source_n") == dataset["n"] and
            row.get("input_hash") == dataset["input_hash"], "whole spatial input/hash mismatch")
    require(type(command) is list and len(command) == 15 and all(type(v) is str for v in command) and
            command[10] in MODES and type(row) is dict and row.get("schema") == "mhgp8_wspd_q34_probe_v4" and
            type(row.get("witness_mode")) is str and row["witness_mode"] == command[10] and
            type(row.get("q3_census_mode")) is str and row["q3_census_mode"] == command[11] and
            command[11] in ("scalar", "boxes") and command[12] in BOUNDS_MODES and
            type(row.get("witness_bounds_mode")) is str and row["witness_bounds_mode"] == command[12],
            "affine command/schema/mode")
    require(command[13] in SEED_MODES and type(row.get("q4_seed_mode")) is str and
            row["q4_seed_mode"] == command[13] and type(row.get("q4_seed_block_size")) is int and
            row["q4_seed_block_size"] == int(command[14]) and 0 < row["q4_seed_block_size"] < (1 << 64),
            "seed/cell command/schema/mode/grain")
    # Shape-only view of unchanged fields, not a synthetic old execution. The
    # changed expansion/lane partitions are validated below on the actual row.
    require(type(row.get("work")) is dict and {"witness", "q3_blocks", "q4_seed_cells"} <= set(row["work"]), "missing indexed work")
    common = deepcopy(row)
    common.pop("witness_mode")
    common.pop("q3_census_mode")
    common.pop("witness_bounds_mode")
    common.pop("q4_seed_mode")
    common.pop("q4_seed_block_size")
    common["work"].pop("q4_seed_cells")
    common["schema"] = previous.FIXED["schema"]
    witness = common["work"].pop("witness")
    blocks = common["work"].pop("q3_blocks")
    strict_shape(common, command[:10])
    edge.structural(witness, WITNESS_FIELDS, ("rectangles", "pairs", "rectangles_bounds", "pairs_bounds"), "witness")
    edge.counts(blocks, Q3_BLOCK_FIELDS, "q3 blocks")
    n, k, s, mask, backend, workers = map(int, command[2:8])
    require(Path(command[0]).name == "mhgp8_wspd_q34_probe" and
            [row[key] for key in ("n", "kmax", "s", "mask", "q4_backend", "workers")] ==
            [n, k, s, mask, backend, workers] and row["front_mode"] == command[8] and
            row["output_mode"] == command[9] and 0 < n <= row["source_n"] and k in (5,10) and
            s in (8,10,12) and mask == 6 and backend in (28,30) and workers > 0,
            "indexed command/result mismatch")
    for value in row["timings_ms"].values():
        require(type(value) in (int, float) and math.isfinite(value) and value >= 0, "invalid time")
    front, w, par, out = row["front"], row["work"], row["parallel"], row["output"]
    fw, q3, z = front["work"], w["q3"], witness
    require(front["total_unordered_pairs"] == n*(n-1)//2 and front["active_lane_mask"] == 6, "front metadata")
    for lane in range(3):
        require(fw["rejected_pair_mass"][lane] + fw["residual_pair_mass"][lane] ==
                (n*(n-1)//2 if lane else 0), "front mass partition")
    require(w["input_rectangles"] == fw["emitted_rectangles"] == sum(fw["size_class_rectangles"]) and
            z["input_pair_mass"] == sum(fw["size_class_pair_mass"]) and
            max(fw["residual_pair_mass"]) <= z["input_pair_mass"] <= sum(fw["residual_pair_mass"]) and
            w["expanded_pairs"] + z["rectangle_pair_mass"] == z["input_pair_mass"] and
            w["cover_builds"] + z["rejected_pairs"] == w["expanded_pairs"] and
            w["q3_edges"] + w["q4_edges"] - w["both_edges"] == w["cover_builds"] and
            w["both_edges"] <= min(w["q3_edges"], w["q4_edges"]), "new expansion union partition")
    require(z["rejected_rectangles"] <= w["input_rectangles"] and
            z["rejected_rectangles"] <= z["rectangle_pair_mass"] <=
            z["rectangle_q3_pairs"] + z["rectangle_q4_pairs"] and
            z["rejected_pairs"] <= z["pair_q3_pairs"] + z["pair_q4_pairs"], "union rejections")
    for lane, number in (("q3", 1), ("q4", 2)):
        require(w[lane + "_edges"] + z["rectangle_" + lane + "_pairs"] + z["pair_" + lane + "_pairs"] ==
                fw["residual_pair_mass"][number], "independent before/after lane partition")
    for section in ("rectangles", "pairs"):
        validate_search(z[section], z[section + "_bounds"], n, k, row["memory"]["id_bytes"], command[12], section)
    if row["witness_mode"] == "disabled":
        require(all(value == 0 for name, value in previous.flatten(z).items() if name != "input_pair_mass"),
                "disabled witness mode did work")
        require(False, "spatial profile requires indexed witnesses and boxed census")
    else:
        require(z["pairs"]["queries"] == w["expanded_pairs"], "pair search count")
        for lane, number in (("q3",1), ("q4",2)):
            require(z["pairs"][lane + "_queries"] == fw["residual_pair_mass"][number] -
                    z["rectangle_" + lane + "_pairs"] and
                    z["pairs"][lane + "_rejected"] == z["pair_" + lane + "_pairs"], "pair lane search/rejections")
        if row["witness_mode"] == "pair":
            require(all(value == 0 for value in z["rectangles"].values()) and
                    all(z[name] == 0 for name in ("rejected_rectangles", "rectangle_pair_mass",
                                                 "rectangle_q3_pairs", "rectangle_q4_pairs")), "pair-only rectangle work")
        else:
            require(z["rectangles"]["queries"] == w["input_rectangles"], "rectangle search count")
            for lane, number in (("q3",1), ("q4",2)):
                require(z["rectangles"][lane + "_queries"] == fw["lane_rectangles"][number] and
                        z["rectangles"][lane + "_rejected"] <= z["rectangle_" + lane + "_pairs"],
                        "rectangle lane mass/query count")
    require(w["cover_sites"] == w["cover"]["admitted_sites"] <= n*w["cover_builds"] and
            w["cover"]["admitted_sites"] + w["cover"]["rejected_sites"] == n*w["cover_builds"] and
            w["max_cover_sites"] <= n, "cover ledger")
    require(q3["edge_queries"] == w["q3_edges"] and
            q3["census_point_tests"] == q3["census_inside_sites"] + q3["census_outside_sites"] + q3["census_shell_sites"] and
            q3["ball_builds"] == q3["seeds"] == q3["depth_rejections"] + q3["emitted"] and
            q3["emitted"] == w["q3_emitted"] == out["q3"] and q3["shell_ids"] <= w["payload_shell_ids"], "q3 census")
    selected = w["local28"] if backend == 28 else w["window30"]
    unused = w["window30"] if backend == 28 else w["local28"]
    require(all(value == 0 for value in previous.flatten(unused).values()), "unused q4 backend did work")
    require(selected["sweep"]["emitted"] == w["q4_emitted"] == out["q4"] and
            (row["q4_seed_mode"] == "joined" or selected["edge"]["seeds"] == selected["sweep"]["seed_queries"]) and
            out["callbacks"] == out["q3"] + out["q4"] and
            out["support_ids"] == 3*out["q3"] + 4*out["q4"] and out["shell_ids"] == w["payload_shell_ids"],
            "emitted payload ledger")
    require(par["requested_workers"] == workers and par["target_jobs"] == workers*16 and
            par["started_workers"] == len(row["workers_work"]) == min(workers, par["jobs"]) and
            par["completed_jobs"] == par["jobs"] == sum(item["jobs"] for item in row["workers_work"]), "parallel jobs")
    for field in ("input_rectangles", "expanded_pairs", "q3_emitted", "q4_emitted"):
        require(sum(item[field] for item in row["workers_work"]) == w[field], "worker reduction")
    require(sum(item["front_products"] for item in row["workers_work"]) + par["prefix_product_visits"] == fw["product_visits"] and
            sum(item["peak_edge_buffer_bytes"] for item in row["workers_work"]) == par["edge_buffer_bytes_sum"], "parallel sums")
    validate_auxiliary(row)
    validate_seed_cells(row)
    times = row["timings_ms"]
    require(abs(times["cloud"] + times["index"] + times["front_edges_collect"] -
                times["pipeline_including_shared_preparation"]) <= 0.000003 and
            abs(times["load_prefix_hash"] + times["pipeline_including_shared_preparation"] +
                times["record_normalization"] - times["total_before_serialization_and_release"]) <= 0.000003,
            "timing partition")
    if command[9] == "records":
        require(len(row["records"]) == out["callbacks"], "full records missing")
        hashes = []
        for record in row["records"]:
            h = edge.word(edge.word(14695981039346656037, record["arity"]), record["depth"])
            for identifier in record["support"]:
                h = edge.word(h, identifier)
            for value in record["coefficients"]:
                coefficient = int(value) % (1 << 128)
                h = edge.word(edge.word(h, coefficient & edge.MASK64), coefficient >> 64)
            for identifier in record["shell"]:
                h = edge.word(h, identifier)
            hashes.append(edge.word(h, len(record["shell"])))
        xor = 0
        for value in hashes:
            xor ^= value
        require(format(xor, "x") == out["xor"] and format(sum(hashes) & edge.MASK64, "x") == out["sum"],
                "full records/digest differ")


def make_parser():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    run_parser = sub.add_parser("run")
    for name in ("prepared", "build", "output"):
        run_parser.add_argument("--" + name, type=Path, required=True)
    run_parser.add_argument("--kmax", type=int, choices=(5, 10), default=5)
    run_parser.add_argument("--s", type=int, choices=(8, 10, 12), default=8)
    run_parser.add_argument("--workers", type=int, default=4)
    run_parser.add_argument("--q4-seed-mode", choices=SEED_MODES, default="live")
    run_parser.add_argument("--q4-seed-block-size", type=int, default=64)
    run_parser.add_argument("--repeats", type=int, default=1)
    run_parser.add_argument("--payload", choices=("digest", "records"), default="digest")
    for name in ("read", "selftest"):
        reader = sub.add_parser(name)
        reader.add_argument("--path", type=Path, required=True)
        reader.add_argument("--compact", action="store_true")
        if name == "read":
            reader.add_argument("--check-live", action="store_true")
    return parser


def configuration(args):
    result = {name: getattr(args, name) for name in
        ("kmax", "s", "workers", "q4_seed_mode", "q4_seed_block_size", "repeats", "payload")}
    validate_configuration(result)
    return result


def validate_configuration(config):
    exact_fields(config, ("kmax", "s", "workers", "q4_seed_mode", "q4_seed_block_size", "repeats", "payload"),
                 "spatial configuration")
    for field in ("kmax", "s", "workers", "q4_seed_block_size", "repeats"):
        require(0 < uint(config[field], field) < (1 << 64), "nonpositive/oversized spatial option")
    require(config["kmax"] in (5,10) and config["s"] in (8,10,12) and
            type(config["q4_seed_mode"]) is str and config["q4_seed_mode"] in SEED_MODES and
            type(config["payload"]) is str and config["payload"] in ("digest", "records"), "spatial option contract")


def authority(build, live):
    """Bind this run to the closed native r2 source/build, not new Python code."""
    require(pins(AUTHORITY_HASHES) == AUTHORITY_HASHES, "fixed native authority receipts changed")
    m, c = (read_json(AUTHORITY / name) for name in ("MANIFEST.json", "COMPLETION.json"))
    require(c["status"] == "passed" and c["error"] is None and c["closing_errors"] == [] and
            c["manifest_sha256"] == AUTHORITY_HASHES[str(AUTHORITY / "MANIFEST.json")] and
            m["schema"] == checks.SCHEMA and m["build"] == str(build) and
            m["config"]["qualification"] == "candidate" and m["config"]["campaign"] == "smoke",
            "native r2 authority/build mismatch")
    require(len(SOURCES) == 216 and set(m["source_sha256"]) == SOURCES and
            m["source_sha256"] == c["source_sha256_after"] and
            m["artifact_sha256"] == c["artifact_sha256_after"], "native authority closure differs")
    for item in c["records"]:
        require(Path(item["path"]).name == item["path"] and
                digest(AUTHORITY / item["path"]) == item["sha256"], "native authority record changed")
    if live:
        require(pins(SOURCES) == m["source_sha256"] and pins(m["artifact_sha256"]) == m["artifact_sha256"],
                "native source/build no longer matches its r2 authority")
    return m


def prepared_datasets(prepared):
    result = preparation.read(prepared)  # One complete raw reconstruction per capture/read.
    m = read_json(prepared / "MANIFEST.json")
    datasets = []
    for name in ORDER:
        d = m["datasets"][name]
        source = prepared / d["points_file"]
        points = list(struct.iter_unpack("<HHH", source.read_bytes()))
        require(len(points) == d["sites"], "whole prepared dataset length differs")
        datasets.append(dict(name=name, source=str(source), n=len(points), input_hash=edge.input_hash(points)))
    inputs = {str(p) for p in prepared.iterdir()} | {m["raw"]["path"]}
    expected = {str(prepared / "MANIFEST.json"): result["manifest_sha256"],
                str(prepared / "COMPLETION.json"): result["completion_sha256"], m["raw"]["path"]: m["raw"]["sha256"],
                str(prepared / m["raw_to_full"]["file"]): m["raw_to_full"]["sha256"]}
    for d in m["datasets"].values():
        expected[str(prepared / d["points_file"])] = d["points_sha256"]
        expected[str(prepared / d["site_ids_file"])] = d["site_ids_sha256"]
    require(set(expected) == inputs and pins(inputs) == expected, "prepared artifacts changed after reconstruction")
    return result, datasets, expected


def plan(build, datasets, config):
    validate_configuration(config)
    require([d["name"] for d in datasets] == list(ORDER), "spatial dataset inventory/order differs")
    entries = []
    for repeat in range(config["repeats"]):
        for d in datasets:
            command = [str(build / "mhgp8_wspd_q34_probe"), d["source"], str(d["n"]),
                str(config["kmax"]), str(config["s"]), "6", "28", str(config["workers"]), "samples", config["payload"],
                "rectangle-pair", "boxes", "affine", config["q4_seed_mode"], str(config["q4_seed_block_size"])]
            entries.append(dict(dataset=d["name"], repeat=repeat, command=command if d["n"] else None))
    return entries


def host_state():
    files = ("/sys/fs/cgroup/cpu.max", "/sys/fs/cgroup/cpu.stat", "/sys/fs/cgroup/cpuset.cpus.effective")
    result = dict(affinity=sorted(os.sched_getaffinity(0)), cpu_count=os.cpu_count(), load_average=list(os.getloadavg()))
    result["cgroup"] = {name: Path(name).read_text() if Path(name).is_file() else None for name in files}
    return result


def repeated_outputs(entries):
    known = {}
    for entry in entries:
        if entry["status"] == "empty":
            continue
        row = entry["row"]
        value = (row["output"], row.get("records"))
        require(entry["dataset"] not in known or known[entry["dataset"]] == value, "repeated spatial output differs")
        known[entry["dataset"]] = value


def run(args):
    build, prepared = args.build.resolve(), args.prepared.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    target = Path(tempfile.mkdtemp(prefix="spatial_", dir=args.output.resolve()))
    print(json.dumps(dict(path=str(target), status="starting")), flush=True)
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    try:
        config = configuration(args)
        native = authority(build, True)
        source_before, protocol_before = pins(SOURCES), pins(PROTOCOL_SOURCES)
        prepared_result, datasets, input_before = prepared_datasets(prepared)
        require(protocol_before == pins(PROTOCOL_SOURCES), "protocol source changed during setup")
        planned = plan(build, datasets, config)
        environment = dict(os.environ)
        cache = (build / "CMakeCache.txt").read_text()
        require("CMAKE_BUILD_TYPE:STRING=Release\n" in cache and "MHGP8_SANITIZE:BOOL=OFF\n" in cache,
                "spatial performance requires the pinned nonsanitized Release build")
        def git(*options):
            return subprocess.check_output(["git", *options], cwd=ROOT, text=True).strip()
        snapshot = target / "protocol_sources"
        snapshot.mkdir()
        for name in sorted(PROTOCOL_SOURCES):
            (snapshot / Path(name).name).write_bytes((ROOT / name).read_bytes())
        require({name: digest(snapshot / Path(name).name) for name in PROTOCOL_SOURCES} == protocol_before,
                "protocol snapshot differs")
        manifest = dict(schema=SCHEMA, started_utc=utc_stamp(), build=str(build), prepared=str(prepared),
            config=config, datasets=datasets, planned_entries=planned, preparation=prepared_result,
            launch_command=[sys.executable, *sys.argv], python_optimized=sys.flags.optimize, source_sha256=source_before,
            protocol_source_sha256=protocol_before, artifact_sha256=native["artifact_sha256"],
            input_sha256=input_before, authority_sha256=AUTHORITY_HASHES,
            environment=edge.environment_record(environment), host_before=host_state(),
            git_commit=git("rev-parse", "HEAD"), branch=git("branch", "--show-current"), worktree=git("status", "--short"),
            compiler_cache=native["compiler_cache"], gcp_used=False, full_contract_qualified=False,
            universal_subquadratic_claim=False, public_status="not_claimed",
            timing_scope="exploratory_wall_times_shared_host_not_stable_speedup_claim")
        require(manifest["branch"] == "main", "spatial capture requires main")
        write_json(target / "MANIFEST.json", manifest)
        manifest_pin = digest(target / "MANIFEST.json")
    except BaseException as error:
        failure = dict(status="failed", error=f"{type(error).__name__}: {error}", finished_utc=utc_stamp(),
                       launch_command=[sys.executable, *sys.argv])
        write_json(target / "SETUP_FAILURE.json", failure)
        write_json(target / "COMPLETION.json", failure)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
        raise
    records, entries, error, status = [], [], None, "failed"
    try:
        for number, item in enumerate(planned):
            record = dict(item, cwd=str(ROOT), environment=manifest["environment"], started_utc=utc_stamp(),
                status="failed", exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="", row=None,
                child_cpu_seconds=None, collector_wall_seconds=None)
            print(json.dumps(dict(path=str(target), index=number, **item)), flush=True)
            try:
                if item["command"] is None:
                    record["status"] = "empty"
                else:
                    cpu_before, wall_before = resource.getrusage(resource.RUSAGE_CHILDREN), time.perf_counter()
                    try:
                        invoke(item["command"], environment, ROOT, record, new_session=True)
                    finally:
                        cpu_after = resource.getrusage(resource.RUSAGE_CHILDREN)
                        record["collector_wall_seconds"] = time.perf_counter() - wall_before
                        record["child_cpu_seconds"] = dict(user=cpu_after.ru_utime-cpu_before.ru_utime,
                            system=cpu_after.ru_stime-cpu_before.ru_stime,
                            scope="RUSAGE_CHILDREN_delta_joined_command_not_rss")
                    require(record["exit_code"] == 0 and not record["stderr"], "spatial native command failed")
                    row = parse_result(record["stdout"].encode())
                    dataset = next(d for d in datasets if d["name"] == item["dataset"])
                    validate_row(row, item["command"], dataset)
                    record.update(row=row, status="completed")
                entries.append({key: record[key] for key in ("dataset", "repeat", "status", "row", "command")})
            finally:
                record["finished_utc"] = utc_stamp()
                file = target / f"record_{number:04}.json"
                write_json(file, record)
                records.append(dict(path=file.name, sha256=digest(file)))
        repeated_outputs(entries)
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        errors = []
        def close(label, action):
            try:
                return action()
            except BaseException as cause:
                errors.append(f"{label}: {type(cause).__name__}: {cause}")
                return None
        completion = dict(status=status, error=error, finished_utc=utc_stamp(), records=records,
            manifest_sha256=close("manifest", lambda: digest(target / "MANIFEST.json")), closing_errors=errors,
            host_after=close("host", host_state))
        for name in ("source", "protocol_source", "artifact", "input", "authority"):
            key = name + "_sha256"
            completion[key + "_after"] = close(name, lambda key=key: pins(manifest[key]))
            if completion[key + "_after"] != manifest[key]:
                errors.append(name + " hashes changed")
        completion["protocol_snapshot_sha256"] = close("snapshot", lambda:
            {name: digest(snapshot / Path(name).name) for name in PROTOCOL_SOURCES})
        if completion["protocol_snapshot_sha256"] != protocol_before:
            errors.append("protocol snapshot changed")
        completion["record_sha256_after"] = close("records", lambda:
            {item["path"]: digest(target / item["path"]) for item in records})
        if completion["record_sha256_after"] != {item["path"]: item["sha256"] for item in records}:
            errors.append("record hashes changed")
        if completion["manifest_sha256"] != manifest_pin:
            errors.append("manifest changed during closure")
        if errors:
            completion.update(status="failed", error=error or "spatial closure failed")
        write_json(target / "COMPLETION.json", completion)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(target), status=completion["status"], error=completion["error"])), flush=True)
    require(completion["status"] == "passed", "spatial capture failed")
    return target


def validate_closure(path, manifest, completion):
    exact_fields(manifest, ("schema", "started_utc", "build", "prepared", "config", "datasets", "planned_entries",
        "preparation", "launch_command", "python_optimized", "source_sha256", "protocol_source_sha256", "artifact_sha256", "input_sha256",
        "authority_sha256", "environment", "host_before", "git_commit", "branch", "worktree", "compiler_cache", "gcp_used",
        "full_contract_qualified", "universal_subquadratic_claim", "public_status", "timing_scope"), "manifest")
    exact_fields(completion, ("status", "error", "finished_utc", "records", "manifest_sha256", "closing_errors",
        "host_after", "source_sha256_after", "protocol_source_sha256_after", "artifact_sha256_after", "input_sha256_after",
        "authority_sha256_after", "protocol_snapshot_sha256", "record_sha256_after"), "completion")
    require(manifest["schema"] == SCHEMA and completion["status"] == "passed" and completion["error"] is None and
            completion["closing_errors"] == [] and completion["manifest_sha256"] == digest(path / "MANIFEST.json"),
            "spatial capture not successfully closed")
    require(type(manifest["python_optimized"]) is int and manifest["python_optimized"] in (0,1,2),
            "invalid launching Python optimization mode")
    require(utc_time(manifest["started_utc"]) <= utc_time(completion["finished_utc"]), "capture timestamps reversed")
    require(manifest["gcp_used"] is False and manifest["full_contract_qualified"] is False and
            manifest["universal_subquadratic_claim"] is False and manifest["public_status"] == "not_claimed" and
            manifest["branch"] == "main" and
            manifest["timing_scope"] == "exploratory_wall_times_shared_host_not_stable_speedup_claim", "spatial scope changed")
    for name in ("source", "protocol_source", "artifact", "input", "authority"):
        key = name + "_sha256"
        require(type(manifest[key]) is dict and manifest[key] == completion[key + "_after"], "spatial pin closure differs")
        require(all(type(v) is str and len(v) == 64 and all(c in "0123456789abcdef" for c in v)
                    for v in manifest[key].values()), "invalid SHA256")
    require(set(manifest["source_sha256"]) == SOURCES and set(manifest["protocol_source_sha256"]) == PROTOCOL_SOURCES and
            manifest["authority_sha256"] == AUTHORITY_HASHES and
            completion["protocol_snapshot_sha256"] == manifest["protocol_source_sha256"], "spatial pin inventory differs")
    require({p.name for p in (path / "protocol_sources").iterdir()} == {Path(n).name for n in PROTOCOL_SOURCES} and
            {n: digest(path / "protocol_sources" / Path(n).name) for n in PROTOCOL_SOURCES} ==
            manifest["protocol_source_sha256"], "protocol snapshots differ")
    require(type(completion["records"]) is list and completion["record_sha256_after"] ==
            {item["path"]: item["sha256"] for item in completion["records"]}, "record closure differs")


def utc_time(value):
    require(type(value) is str, "timestamp must be text")
    try:
        parsed = datetime.fromisoformat(value)
    except ValueError as cause:
        raise InvalidReceipt("invalid timestamp") from cause
    require(parsed.utcoffset() == timezone.utc.utcoffset(None), "timestamp must be UTC")
    return parsed


def validate_record(record, planned, dataset, manifest):
    exact_fields(record, (*planned, "cwd", "environment", "started_utc", "finished_utc", "status", "exit_code",
                         "stdout", "stderr", "stdout_base64", "stderr_base64", "row", "child_cpu_seconds",
                         "collector_wall_seconds"), "record")
    require(all(type(record[k]) is type(v) and record[k] == v for k,v in planned.items()) and
            record["cwd"] == str(ROOT) and record["environment"] == manifest["environment"], "spatial command record differs")
    require(type(record["repeat"]) is int, "boolean repetition")
    require(utc_time(manifest["started_utc"]) <= utc_time(record["started_utc"]) <= utc_time(record["finished_utc"]),
            "record timestamps reversed or before capture")
    for stream in ("stdout", "stderr"):
        require(type(record[stream]) is str and type(record[stream + "_base64"]) is str and
                base64.b64decode(record[stream + "_base64"], validate=True).decode(errors="replace") == record[stream],
                "raw/decoded stream differs")
    if planned["command"] is None:
        require(dataset["n"] == 0 and record["status"] == "empty" and record["row"] is None and
                record["exit_code"] is None and record["stdout"] == record["stderr"] == "" and
                record["child_cpu_seconds"] is None and record["collector_wall_seconds"] is None, "fabricated empty execution")
    else:
        require(record["status"] == "completed" and type(record["exit_code"]) is int and record["exit_code"] == 0 and
                record["stderr"] == "", "native command did not complete cleanly")
        exact_fields(record["child_cpu_seconds"], ("user", "system", "scope"), "child CPU")
        require(record["child_cpu_seconds"]["scope"] == "RUSAGE_CHILDREN_delta_joined_command_not_rss" and
                all(type(v) in (int,float) and math.isfinite(v) and v >= 0 for v in
                    (record["child_cpu_seconds"]["user"], record["child_cpu_seconds"]["system"],
                     record["collector_wall_seconds"])), "invalid collected CPU/wall time")
        row = parse_result(record["stdout"].encode())
        validate_row(row, planned["command"], dataset)
        require(preparation.canonical_json(row) == preparation.canonical_json(record["row"]), "raw/parsed row differs")
    return {key: record[key] for key in ("dataset", "repeat", "status", "row", "command")}


def read(path, check_live=False):
    path = path.resolve()
    manifest, completion = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    validate_closure(path, manifest, completion)
    before = pins({str(p) for p in path.glob("record_*.json")} | {str(p) for p in (path / "protocol_sources").iterdir()} |
                  {str(path / "MANIFEST.json"), str(path / "COMPLETION.json"), *AUTHORITY_HASHES})
    build, prepared = Path(manifest["build"]), Path(manifest["prepared"])
    require(build.is_absolute() and build.resolve() == build and prepared.is_absolute() and prepared.resolve() == prepared,
            "noncanonical capture paths")
    native = authority(build, check_live)
    require(manifest["source_sha256"] == native["source_sha256"] and
            manifest["artifact_sha256"] == native["artifact_sha256"] and
            manifest["compiler_cache"] == native["compiler_cache"], "spatial capture/native authority mismatch")
    launch = manifest["launch_command"]
    require(type(launch) is list and len(launch) > 3 and all(type(v) is str for v in launch) and
            Path(launch[1]).name == Path(__file__).name and launch[2] == "run", "spatial launch command")
    try:
        args = make_parser().parse_args(launch[2:])
    except SystemExit as cause:
        raise InvalidReceipt("unreadable spatial launch") from cause
    require(configuration(args) == manifest["config"] and args.build.resolve() == build and
            args.prepared.resolve() == prepared and args.output.resolve() == path.parent, "launch/configuration mismatch")
    prepared_result, datasets, input_pins = prepared_datasets(prepared)
    require(preparation.canonical_json(datasets) == preparation.canonical_json(manifest["datasets"]) and
            preparation.canonical_json(prepared_result) == preparation.canonical_json(manifest["preparation"]) and
            input_pins == manifest["input_sha256"], "spatial preparation differs")
    planned = plan(build, datasets, manifest["config"])
    require(preparation.canonical_json(planned) == preparation.canonical_json(manifest["planned_entries"]) and
            len(completion["records"]) == len(planned), "spatial command plan not complete")
    require({p.name for p in path.glob("record_*.json")} == {f"record_{i:04}.json" for i in range(len(planned))},
            "spatial record inventory differs")
    if check_live:
        for key in ("source_sha256", "protocol_source_sha256", "artifact_sha256", "input_sha256", "authority_sha256"):
            require(pins(manifest[key]) == manifest[key], "live spatial pins changed")
    entries, usage = [], []
    for i, (item, recorded) in enumerate(zip(planned, completion["records"], strict=True)):
        exact_fields(recorded, ("path", "sha256"), "record reference")
        require(recorded["path"] == f"record_{i:04}.json" and digest(path / recorded["path"]) == recorded["sha256"],
                "record path/hash differs")
        dataset = next(d for d in datasets if d["name"] == item["dataset"])
        record = read_json(path / recorded["path"])
        entries.append(validate_record(record, item, dataset, manifest))
        usage.append({key: record[key] for key in ("dataset", "repeat", "child_cpu_seconds", "collector_wall_seconds")})
        require(utc_time(record["finished_utc"]) <= utc_time(completion["finished_utc"]), "record finished after closure")
    repeated_outputs(entries)
    require(pins(before) == before and pins(input_pins) == input_pins, "receipt/preparation changed during reading")
    if check_live:
        for key in ("source_sha256", "protocol_source_sha256", "artifact_sha256", "authority_sha256"):
            require(pins(manifest[key]) == manifest[key], "live pin changed during reading")
    return dict(status="passed", path=str(path), sources=216, protocol_sources=len(PROTOCOL_SOURCES),
        datasets=7, entries=len(entries), measurements=sum(e["status"] == "completed" for e in entries),
        empty_entries=sum(e["status"] == "empty" for e in entries), records=entries, resource_usage=usage,
        full_contract_qualified=False, universal_subquadratic_claim=False)


def selftest(path):
    result = read(path)
    m, c = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    original = next((e for e in result["records"] if e["status"] == "completed"), None)
    require(original is not None, "reader selftest needs one actual nonempty execution")
    dataset = next(d for d in m["datasets"] if d["name"] == original["dataset"])
    row, command = original["row"], original["command"]
    mutants = 0
    def reject(action):
        nonlocal mutants
        try:
            action()
        except InvalidReceipt:
            mutants += 1
        else:
            raise InvalidReceipt("spatial reader corruption survived")
    def alter(route, value):
        changed = deepcopy(row)
        target = changed
        for key in route[:-1]:
            target = target[key]
        require(type(target[route[-1]]) is not type(value) or target[route[-1]] != value, "no-op row mutation")
        target[route[-1]] = value
        reject(lambda: validate_row(changed, command, dataset))
    for field in ("n", "source_n", "kmax", "s", "mask", "q4_backend", "workers", "input_hash", "q4_seed_block_size"):
        alter([field], True)
    alter(["source_n"], row["source_n"] + 1)
    alter(["timings_ms", "cloud"], float("nan"))
    for field in checks.SEED_FIELDS:
        alter(["work", "q4_seed_cells", field], True)
    for route in (("front", "total_unordered_pairs"), ("work", "cover_sites"),
                  ("work", "witness", "input_pair_mass"), ("work", "q3_blocks", "queries"),
                  ("work", "q4_seed_cells", "live_child_reads"), ("output", "shell_ids")):
        current = row
        for key in route:
            current = current[key]
        alter(route, current + 1)
    for index, value in ((1, command[1] + ".wrong"), (2, str(dataset["n"] + 1)), (13, "unknown")):
        changed = command.copy()
        changed[index] = value
        reject(lambda changed=changed: validate_row(row, changed, dataset))
    changed = deepcopy(c)
    changed["input_sha256_after"][next(iter(changed["input_sha256_after"]))] = "0" * 64
    reject(lambda: validate_closure(path, m, changed))
    changed = deepcopy(original)
    changed["row"]["output"]["sum"] = format(int(row["output"]["sum"], 16) ^ 1, "x")
    reject(lambda: repeated_outputs([original, changed]))
    return dict(status="passed", path=str(path), mutants=mutants, native_runs=0,
                scope="receipt_corruptions_only_not_new_geometric_qualification")


def main():
    args = make_parser().parse_args()
    try:
        if args.operation == "run":
            run(args)
        else:
            result = read(args.path, args.check_live) if args.operation == "read" else selftest(args.path)
            if args.compact:
                result = {k: v for k,v in result.items() if k != "records"}
            print(json.dumps(result, sort_keys=True, allow_nan=False))
        return 0
    except (Exception, KeyboardInterrupt) as error:
        print(json.dumps(dict(status="failed", error=f"{type(error).__name__}: {error}")), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
