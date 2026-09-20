#!/usr/bin/env python3
"""Explicit adaptation of tranche25 auxiliary receipts, with new product assessments.

Builds only a temporary standalone probe linked to the frozen Release archive.
All commands, failures, raw output and before/after input hashes are retained.
"""
from __future__ import annotations
import argparse
import base64
from copy import deepcopy
import json
import math
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_q34_collective_checks as product
from run_p0_matrix import InvalidReceipt, invoke, on_signal, parse_result, require, uint, utc_stamp, write_json
from run_q4_family_checks import counts, digest, pins, read_json
from run_q34_seed_checks import environment_record

SCHEMA = "mhgp8_dense_collective_forecast_attempt_v1"
AUXILIARY = frozenset(str(path.relative_to(ROOT)) for path in (Path(__file__), HERE / "dense_forecast.cpp"))
SOURCES = product.SOURCES | AUXILIARY
FIXED = dict(schema="mhgp8_dense_collective_forecast_v1", status="completed",
    scope="executed_product_pool_assessments_known_seeds_then_unexecuted_scan_lower_bound", public_status="not_claimed",
    backend="cpu_reference", profile="quantized_u16_input_only", recipe="dense_grid_prefix_41x21x38_v1", threads=1, edge_ids=[0,1],
    timing_scope="auxiliary_generation_validation_preparation_filter_release_excludes_json",
    family_sweeps_executed=0, edge_generator_executed=False, candidates_computed=False)
WORK = tuple(product.FILTER_FIELDS)
PART_TIMES = ("generation_ms", "fixture_validation_ms", "cloud_ms", "index_ms", "cover_ms", "pool_ms", "pool_validation_ms",
              "filter_only_ms", "result_validation_ms", "release_ms")
MEMORY = ("id_bytes", "input_capacity_bytes", "cloud_retained_bytes", "index_retained_bytes", "cover_retained_bytes", "pool_retained_bytes", "workspace_retained_bytes")


def validate_row(row, command):
    require(type(row) is dict and len(command) == 5, "forecast command arity")
    n, k, budget = map(int, command[1:4]); mode = command[4]
    require(n in (8000,16000,32000) and k in (5,10) and budget in (32,64) and mode in product.MODES, "forecast fixture domain")
    require(set(row) == set(FIXED) | {"n","kmax","budget","mode","input_hash","pool_hash","seeds","cover_sites","validation","work","forecast",
                                    "cloud_work","index_work","cover_work","pool_work","memory","timings"}, "forecast fields differ")
    require(all(type(row[key]) is type(value) and row[key] == value for key,value in FIXED.items()) and
            all(type(value) is int for value in row["edge_ids"]), "forecast scope mismatch")
    require(all(type(row[key]) is int and row[key] == value for key,value in dict(n=n,kmax=k,budget=budget,seeds=n-2,cover_sites=n).items()),
            "forecast command/result mismatch")
    require(row["mode"]==mode, "forecast mode differs")
    uint(row["input_hash"],"input_hash");uint(row["pool_hash"],"pool_hash")
    counts(row["validation"], ("fixture_point_checks","owned_seed_checks","pool_ids_checked"), "validation")
    require(row["validation"] == dict(fixture_point_checks=n,owned_seed_checks=n-2,pool_ids_checked=budget), "fixture validation work differs")
    w=row["work"];counts(w,WORK,"work")
    s=n-2
    product.validate_filter(w,s,budget,k,mode,row["memory"]["id_bytes"])
    f=row["forecast"]
    require(type(f) is dict and set(f)=={"q4_survivors","future_scan_lower_bound","formula","includes_sorting","measured_scan_work"} and
            f["formula"]=="cover_sites_times_q4_survivors" and f["includes_sorting"] is False and f["measured_scan_work"] is False and
            uint(f["q4_survivors"],"q4_survivors")==s-w["q4_rejected"] and
            uint(f["future_scan_lower_bound"],"future_scan_lower_bound")==n*f["q4_survivors"], "forecast mistaken for measured/full work")
    counts(row["cloud_work"],product.previous.CLOUD_FIELDS,"cloud")
    c=row["cloud_work"]
    require(c["coordinate_copies"]==c["validation_points"]==c["range_tree_leaf_visits"]==n and
            c["uniqueness_adjacent_tests"]==c["range_tree_merges"]==n-1 and c["range_tree_nodes"]==2*n-1,"owner work differs")
    counts(row["index_work"],product.previous.INDEX_FIELDS,"index")
    ix=row["index_work"]
    require(ix["nodes"]==ix["escape_links"]==2*n-1 and ix["point_visits"]>=n and ix["max_depth"]>0,"index preparation missing")
    counts(row["cover_work"],product.previous.COVER_FIELDS,"cover")
    require(row["cover_work"]==dict(node_visits=1,bound_tests=1,point_tests=0,admitted_nodes=1,rejected_nodes=0,split_nodes=0,
                                  admitted_sites=n,rejected_sites=0,retained_ranges=1,merged_ranges=0),"dense cover should admit its complete root")
    counts(row["pool_work"],product.POOL_FIELDS,"pool")
    require(row["pool_work"]==dict(range_visits=1,selected_sites=budget),"pool work differs")
    m=row["memory"]
    require(type(m) is dict and set(m)==set(MEMORY)|{"scope"} and m["scope"]=="retained_capacities_not_RSS_shared_event_workspace","memory scope differs")
    for key in MEMORY:uint(m[key],"memory."+key)
    require(m["id_bytes"] in (4,8) and min(m["input_capacity_bytes"],m["cloud_retained_bytes"])>=6*n and
            m["index_retained_bytes"]>=m["id_bytes"]*n and m["cover_retained_bytes"]>=2*m["id_bytes"] and
            m["pool_retained_bytes"]>=budget*m["id_bytes"] and m["workspace_retained_bytes"]==w["peak_event_bytes"],"retained capacities missing")
    t=row["timings"]
    require(type(t) is dict and set(t)==set(PART_TIMES)|{"auxiliary_total_ms"} and
            all(type(v) in (int,float) and math.isfinite(v) and v>=0 for v in t.values()) and
            math.isclose(sum(t[key] for key in PART_TIMES),t["auxiliary_total_ms"],rel_tol=1e-12,abs_tol=1e-6),"auxiliary timing partition differs")


def artifact_paths(build):
    return {str((build/name).relative_to(ROOT)) for name in ("CMakeCache.txt","libmhgp8_p0.a")}


def plan(build, temporary, compiler):
    binary=temporary/"dense_forecast"
    return [("compile",[compiler,"-std=c++20","-O3","-DNDEBUG","-Wall","-Wextra","-Wconversion","-Wshadow","-Wpedantic","-Werror",
                         "-I"+str(ROOT/"morsehgp3D_v8/src"),str(HERE/"dense_forecast.cpp"),str(build/"libmhgp8_p0.a"),"-pthread","-o",str(binary)])] + [
        ("pool_assessment_forecast",[str(binary),str(n),str(k),str(budget),mode]) for budget in (32,64) for k in (5,10) for mode in product.MODES for n in (8000,16000,32000)]


def validate_closure(manifest, completion):
    require(completion["status"]=="passed" and completion["error"] is None and completion["closing_errors"]==[],"auxiliary capture not successful")
    for key in ("source_sha256","artifact_sha256","compiler_sha256"):
        require(manifest[key]==completion[key+"_after"],"closure hashes differ: "+key)
    require(type(completion["binary_sha256"]) is str and len(completion["binary_sha256"])==64 and
            completion["binary_sha256"]==completion["binary_sha256_after"],"compiled binary changed after compilation")


def run(args):
    build=args.build.resolve()
    require(build.is_dir() and build.is_relative_to(ROOT/"build") and len(product.SOURCES)==163,"frozen 163-source local build required")
    cache=(build/"CMakeCache.txt").read_text()
    require("CMAKE_BUILD_TYPE:STRING=Release\n" in cache and "MHGP8_SANITIZE:BOOL=OFF\n" in cache,"requires nonsanitized Release archive")
    compilers=[line.split("=",1)[1] for line in cache.splitlines() if line.startswith(("CMAKE_CXX_COMPILER:FILEPATH=","CMAKE_CXX_COMPILER:STRING="))]
    require(len(compilers)==1,"ambiguous compiler")
    compiler=str(Path(compilers[0]).resolve())
    args.output.mkdir(parents=True,exist_ok=True)
    capture=Path(tempfile.mkdtemp(prefix="forecast_",dir=args.output)).resolve()
    temporary=Path(tempfile.mkdtemp(prefix="mhgp8_dense_forecast_")).resolve()
    binary=temporary/"dense_forecast"
    env=dict(os.environ)
    def git(*command):return subprocess.check_output(["git",*command],cwd=ROOT,text=True).strip()
    manifest=dict(schema=SCHEMA,started_utc=utc_stamp(),build=str(build),temporary=str(temporary),compiler=compiler,
        source_sha256=pins(SOURCES),artifact_sha256=pins(artifact_paths(build)),compiler_sha256=digest(Path(compiler)),
        compiler_cache="\n".join(line for line in cache.splitlines() if line.startswith(("CMAKE_CXX_COMPILER","CMAKE_CXX_FLAGS","CMAKE_BUILD_TYPE:","MHGP8_SANITIZE:"))),
        commit=git("rev-parse","HEAD"),branch=git("branch","--show-current"),worktree=git("status","--short"),
        launch_command=[sys.executable,*sys.argv],environment=environment_record(env),affinity=sorted(os.sched_getaffinity(0)),
        scope=FIXED["scope"],public_status="not_claimed",gcp_used=False,full_contract_qualified=False,
        product_source_count=163,auxiliary_source_count=2,planned_commands=plan(build,temporary,compiler))
    write_json(capture/"MANIFEST.json",manifest)
    records=[];status="failed";error=None;binary_hash=None
    handlers={sig:signal.signal(sig,on_signal) for sig in (signal.SIGINT,signal.SIGTERM)}
    try:
        for number,(kind,command) in enumerate(manifest["planned_commands"]):
            record=dict(kind=kind,command=command,cwd=str(ROOT),environment=manifest["environment"],started_utc=utc_stamp(),
                status="failed",exit_code=None,stdout="",stderr="",stdout_base64="",stderr_base64="",binary_sha256_before=digest(binary) if kind!="compile" else None)
            try:
                if kind!="compile":require(record["binary_sha256_before"]==binary_hash,"binary changed before execution")
                invoke(command,env,ROOT,record,new_session=True)
                require(type(record["exit_code"]) is int and record["exit_code"]==0,"auxiliary command failed")
                record["binary_sha256_after"]=digest(binary)
                if kind=="compile":binary_hash=record["binary_sha256_after"]
                else:
                    require(record["binary_sha256_after"]==binary_hash,"binary changed during execution")
                    record["row"]=parse_result(record["stdout"].encode());validate_row(record["row"],command)
                record["status"]="passed"
            finally:
                record["finished_utc"]=utc_stamp()
                file=capture/f"record_{number:02}.json";write_json(file,record)
                records.append(dict(path=file.name,sha256=digest(file)))
        status="passed"
    except BaseException as cause:
        error=f"{type(cause).__name__}: {cause}";raise
    finally:
        for sig in handlers:signal.signal(sig,signal.SIG_IGN)
        errors=[]
        def close(label,function):
            try:return function()
            except Exception as cause:errors.append(f"{label}: {type(cause).__name__}: {cause}");return None
        completion=dict(status=status,error=error,finished_utc=utc_stamp(),records=records,manifest_sha256=digest(capture/"MANIFEST.json"),
            source_sha256_after=close("sources",lambda:pins(SOURCES)),artifact_sha256_after=close("artifacts",lambda:pins(artifact_paths(build))),
            compiler_sha256_after=close("compiler",lambda:digest(Path(compiler))),binary_sha256=binary_hash,
            binary_sha256_after=close("binary",lambda:digest(binary)),closing_errors=errors)
        if errors or any(completion[key+"_after"]!=manifest[key] for key in ("source_sha256","artifact_sha256","compiler_sha256")) or binary_hash!=completion["binary_sha256_after"]:
            completion["status"]="failed";completion["error"]=error or "closure changed or unreadable"
        write_json(capture/"COMPLETION.json",completion)
        for sig,handler in handlers.items():signal.signal(sig,handler)
        print(json.dumps(dict(path=str(capture),status=completion["status"],error=completion["error"])),flush=True)
    require(completion["status"]=="passed","auxiliary capture failed closure")


def read(capture,check_live=False):
    capture=capture.resolve();m=read_json(capture/"MANIFEST.json");c=read_json(capture/"COMPLETION.json")
    require(m["schema"]==SCHEMA and c["manifest_sha256"]==digest(capture/"MANIFEST.json"),"manifest schema/hash differs")
    validate_closure(m,c)
    require(m["scope"]==FIXED["scope"] and m["public_status"]=="not_claimed" and m["gcp_used"] is False and
            m["full_contract_qualified"] is False and m["branch"]=="main" and m["product_source_count"]==163 and m["auxiliary_source_count"]==2,
            "scope or source count differs")
    require(type(m["commit"]) is str and len(m["commit"])==40 and all(v in "0123456789abcdef" for v in m["commit"]) and
            type(m["worktree"]) is str and m["environment"]["scope"]=="selected_values_full_environment_fingerprint_no_secrets" and
            "CMAKE_BUILD_TYPE:STRING=Release" in m["compiler_cache"] and "MHGP8_SANITIZE:BOOL=OFF" in m["compiler_cache"],"provenance missing")
    build,temporary=Path(m["build"]),Path(m["temporary"])
    require(build.is_absolute() and build.is_relative_to(ROOT/"build") and ".." not in build.parts and temporary.is_absolute() and
            temporary.parent==Path(tempfile.gettempdir()) and temporary.name.startswith("mhgp8_dense_forecast_"),"build/temp scope differs")
    require(set(m["source_sha256"])==SOURCES and set(m["artifact_sha256"])==artifact_paths(build),"input inventory differs")
    commands=[[kind,command] for kind,command in plan(build,temporary,m["compiler"])]
    require(m["planned_commands"]==commands and len(c["records"])==49 and
            {file.name for file in capture.glob("record_*.json")}=={f"record_{i:02}.json" for i in range(49)},"command plan differs")
    if check_live:
        require(pins(SOURCES)==m["source_sha256"] and pins(artifact_paths(build))==m["artifact_sha256"] and
                digest(Path(m["compiler"]))==m["compiler_sha256"] and digest(temporary/"dense_forecast")==c["binary_sha256"],"live source/artifact differs")
    rows=[]
    for i,((kind,command),info) in enumerate(zip(commands,c["records"],strict=True)):
        require(info["path"]==f"record_{i:02}.json" and digest(capture/info["path"])==info["sha256"],"record hash/path differs")
        rec=read_json(capture/info["path"])
        require(rec["kind"]==kind and rec["command"]==command and rec["cwd"]==str(ROOT) and rec["environment"]==m["environment"] and
                rec["status"]=="passed" and type(rec["exit_code"]) is int and rec["exit_code"]==0 and
                rec["binary_sha256_after"]==c["binary_sha256"] and rec["binary_sha256_before"]==(None if kind=="compile" else c["binary_sha256"]),
                "command/result/binary mismatch")
        for stream in ("stdout","stderr"):
            require(base64.b64decode(rec[stream+"_base64"],validate=True).decode("utf-8",errors="replace")==rec[stream],"raw/decoded output differs")
        if kind!="compile":
            row=parse_result(rec["stdout"].encode());require(row==rec["row"],"parsed output changed")
            validate_row(row,command);rows.append(row)
    for n in (8000,16000,32000):
        group=[r for r in rows if r["n"]==n]
        require(len(group)==16 and all(all(r[key]==group[0][key] for key in ("input_hash","cloud_work","index_work","cover_work")) for r in group),
                "same-size dense input/preparation changed")
        for budget in (32,64):
            pair=[r for r in group if r["budget"]==budget]
            require(len(pair)==8 and all(r["pool_hash"]==pair[0]["pool_hash"] for r in pair),"K changed spatial pool")
    growth={}
    for budget in (32,64):
        for k in (5,10):
            for mode in product.MODES:
                series=sorted((r for r in rows if (r["budget"],r["kmax"],r["mode"])==(budget,k,mode)),key=lambda r:r["n"])
                require(len(series)==3, "forecast growth triplet missing")
                values=[r["forecast"]["future_scan_lower_bound"] for r in series]
                growth[f"C{budget}_K{k}_{mode}"]=dict(q4_survivors=[r["forecast"]["q4_survivors"] for r in series],lower_bounds=values,
                    ratios=[b/a if a else None for a,b in zip(values,values[1:])],above_quadrupling=[b>4*a for a,b in zip(values,values[1:])],
                    paired_predicate_tests=[r["work"]["paired_predicate_tests"] for r in series],
                    filter_sort_comparisons=[r["work"]["sort_comparisons"] for r in series],
                    filter_event_side_tests=[r["work"]["event_side_tests"] for r in series],
                    filter_only_ms=[r["timings"]["filter_only_ms"] for r in series])
    for n in (8000,16000,32000):
        for budget in (32,64):
            for k in (5,10):
                by_mode={r["mode"]:r for r in rows if (r["n"],r["budget"],r["kmax"])==(n,budget,k)}
                base=by_mode["jung_universal"]["work"]
                for mode in product.MODES:
                    require(by_mode[mode]["work"]["q3_rejected"]==base["q3_rejected"], "q4 options changed q3 decision")
                for chord in ("jung","variance"):
                    require(by_mode[chord+"_collective"]["work"]["q4_rejected"] >= by_mode[chord+"_universal"]["work"]["q4_rejected"],
                            "collective mode lost a universal rejection")
                for reduction in ("universal","collective"):
                    require(by_mode["variance_"+reduction]["work"]["q4_rejected"] >= by_mode["jung_"+reduction]["work"]["q4_rejected"],
                            "narrower chord lost a rejection")
    return dict(status="passed",path=str(capture),records=49,configurations=48,product_sources=163,auxiliary_sources=2,
        family_sweeps_executed=0,full_contract_qualified=False,general_asymptotic_bound=False,
        timings_scope="pool_assessment_auxiliary_only_not_full_family_runtime",growth=growth)



def selftest(capture):
    summary=read(capture)
    base=read_json(capture/"record_01.json");mutants=0
    def reject(action):
        nonlocal mutants
        try:action()
        except InvalidReceipt:mutants+=1
        else:raise InvalidReceipt("forecast reader mutant survived")
    for path,value in ((["n"],True),(["edge_ids"],[False,1]),(["family_sweeps_executed"],1),(["candidates_computed"],True),
        (["forecast","future_scan_lower_bound"],0),(["forecast","measured_scan_work"],True),(["work","both_rejected"],999999),
        (["work","paired_predicate_tests"],False),(["mode"],"unknown"),(["work","collective_queries"],999999),(["validation","owned_seed_checks"],0),(["timings","auxiliary_total_ms"],float("nan"))):
        row=deepcopy(base["row"]);target=row
        for key in path[:-1]:target=target[key]
        target[path[-1]]=value;reject(lambda:validate_row(row,base["command"]))
    command=base["command"].copy();command[2]="10"
    reject(lambda:validate_row(base["row"],command))
    reject(lambda:parse_result(b'{"x":NaN}'));reject(lambda:parse_result(b'{"x":1,"x":2}'))
    m=read_json(capture/"MANIFEST.json");c=read_json(capture/"COMPLETION.json")
    c["source_sha256_after"][next(iter(m["source_sha256"]))]="0"*64
    reject(lambda:validate_closure(m,c))
    return dict(status="passed",mutants=mutants,configurations=summary["configurations"],scope="reader_not_geometry")


def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest="operation",required=True)
    runner=sub.add_parser("run");runner.add_argument("--build",type=Path,required=True);runner.add_argument("--output",type=Path,required=True)
    reader=sub.add_parser("read");reader.add_argument("capture",type=Path);reader.add_argument("--check-live",action="store_true")
    unit=sub.add_parser("selftest");unit.add_argument("capture",type=Path)
    args=parser.parse_args()
    if args.operation=="run":run(args)
    else:print(json.dumps(read(args.capture,args.check_live) if args.operation=="read" else selftest(args.capture),sort_keys=True,allow_nan=False))
if __name__=="__main__":main()

