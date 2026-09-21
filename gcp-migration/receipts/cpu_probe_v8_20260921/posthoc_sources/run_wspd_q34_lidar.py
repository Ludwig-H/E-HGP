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
SCHEMA = "mhgp8_global_lidar_capture_v2"
HISTORICAL_SCHEMA = "mhgp8_global_lidar_capture_v1"


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



GLOBAL_FIELDS = "input_rectangles expanded_pairs q3_edges q4_edges both_edges cover_builds cover_sites max_cover_sites peak_cover_bytes q3_emitted q4_emitted payload_shell_ids peak_edge_buffer_bytes".split()
Q3_FIELDS = "edge_queries seed_node_visits seed_bound_tests seed_point_tests seed_rejected_nodes seed_split_nodes seed_rejected_sites acute_seeds owner_tests owner_rejections seeds ball_builds census_range_visits census_point_tests census_inside_sites census_outside_sites census_shell_sites depth_rejections early_unread_sites shell_sort_comparisons shell_ids emitted peak_shell_bytes".split()
FRONT_FIELDS = "product_visits diagonal_splits diagonal_leaves disjoint_splits separation_tests witness_searches witness_descent_steps witness_box_distance_tests proposed_sites proposals_in_factors h_bound_tests xi_bound_tests witness_lane_credits fully_rejected_products emitted_rectangles emitted_factor_sites max_factor_size leaf_pair_rectangles max_stack_size max_product_depth extended_products extended_proposals extended_proposals_in_factors extended_credits extended_rejections inherited_credits inherited_duplicates extended_inherited_duplicates inherited_rejections emitted_witness_credits".split()
FRONT_ARRAYS = dict(rejected_pair_mass=3,residual_pair_mass=3,lane_rectangles=3,size_class_rectangles=5,size_class_pair_mass=5)
PARALLEL_FIELDS = "requested_workers started_workers target_jobs jobs completed_jobs terminal_jobs prefix_product_visits job_storage_bytes worker_state_bytes callback_storage_bytes edge_buffer_bytes_sum".split()
WORKER_FIELDS = "jobs front_products input_rectangles expanded_pairs q3_emitted q4_emitted peak_edge_buffer_bytes".split()
MEMORY_FIELDS = "id_bytes input_capacity_bytes cloud_retained_bytes index_retained_bytes worker_record_capacity_bytes_before_merge merged_record_capacity_bytes".split()
TIME_FIELDS = "load_prefix_hash cloud index front_edges_collect record_normalization pipeline_including_shared_preparation total_before_serialization_and_release".split()
OUTPUT_FIELDS = "callbacks q3 q4 support_ids shell_ids".split()
FIXED = dict(schema="mhgp8_wspd_q34_probe_v1",status="completed",
    scope="global_q3_q4_candidate_stream_not_catalogue_or_full",backend="cpu_reference",
    profile="quantized_u16_input_only",public_status="not_claimed",
    validation="payload_structure_and_ledger_only_small_exhaustive_gate_separate")

def exact_fields(value,fields,label):
    require(type(value) is dict and set(value)==set(fields),label+": field inventory differs")

def strict_shape(row,command):
    require(type(command) is list and len(command)==10 and all(type(v) is str for v in command),
            "global command arity/types")
    require(command[8]=="samples" and command[9] in ("digest","records"),"unsupported global mode")
    scalars="n source_n kmax s mask q4_backend workers input_hash".split()
    nested="front work parallel workers_work cloud_work index_work memory output timings_ms front_mode output_mode".split()
    exact_fields(row,[*FIXED,*scalars,*nested,*(["records"] if command[9]=="records" else [])],"row")
    require(all(type(row[key]) is type(value) and row[key]==value for key,value in FIXED.items()),"fixed scope differs")
    for key in scalars:uint(row[key],key)
    require(row["source_n"] in (8000,16000,32000,50000) and
        Path(command[1]).name==f'n{row["source_n"]}.u16le',"source count/path differs")
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

def auxiliary_ledgers(row):
    n=row["n"];w=row["work"];q=w["q3"];c=row["cloud_work"];ix=row["index_work"];m=row["memory"]
    require(c["coordinate_copies"]==c["validation_points"]==c["range_tree_leaf_visits"]==n and
        c["uniqueness_adjacent_tests"]==c["range_tree_merges"]==n-1 and c["range_tree_nodes"]==2*n-1,"cloud preparation")
    require(ix["nodes"]==ix["escape_links"]==2*n-1 and ix["point_visits"]>=n,"global index preparation")
    require(m["id_bytes"] in (4,8) and min(m["input_capacity_bytes"],m["cloud_retained_bytes"])>=6*n and
        m["index_retained_bytes"]>=n*m["id_bytes"],"shared capacities")
    if row["output_mode"]=="digest":
        require(m["worker_record_capacity_bytes_before_merge"]==m["merged_record_capacity_bytes"]==0,"digest allocated full records")
    else:
        require(m["worker_record_capacity_bytes_before_merge"]>=row["output"]["shell_ids"]*m["id_bytes"] and
            m["merged_record_capacity_bytes"]>=row["output"]["shell_ids"]*m["id_bytes"],"complete record capacity")
    cover=w["cover"]
    require(cover["node_visits"]==cover["bound_tests"]+cover["point_tests"]==
        cover["admitted_nodes"]+cover["rejected_nodes"]+cover["split_nodes"] and
        cover["retained_ranges"]+cover["merged_ranges"]==cover["admitted_nodes"],"cover node/range ledger")
    require(q["seed_node_visits"]==q["seed_bound_tests"]+q["seed_point_tests"] and
        q["seed_bound_tests"]==q["seed_rejected_nodes"]+q["seed_split_nodes"] and
        q["seed_rejected_sites"]+q["seed_point_tests"]==n*q["edge_queries"] and
        q["acute_seeds"]==q["seeds"]+q["owner_rejections"] and
        q["acute_seeds"]<=q["owner_tests"]<=2*q["acute_seeds"],"q3 seed generation")
    require(q["census_shell_sites"]>=q["shell_ids"]>=3*q["emitted"] and
        q["census_inside_sites"]>=(row["kmax"]-1)*q["depth_rejections"] and
        q["peak_shell_bytes"]>=3*m["id_bytes"]*(q["emitted"]>0),"q3 accepted shell/rejected depth")
    par=row["parallel"]
    require(par["terminal_jobs"]<=par["jobs"] and
        w["peak_edge_buffer_bytes"]==max((x["peak_edge_buffer_bytes"] for x in row["workers_work"]),default=0),
        "parallel peak or terminal job ledger")

def validate(row, command):
    strict_shape(row, command)
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
    auxiliary_ledgers(row)


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
            def metrics(row):
                return {"front":row["front"]["work"],"work":row["work"],"timings_ms":row["timings_ms"],
                    "cloud_work":row["cloud_work"],"index_work":row["index_work"],"memory":row["memory"],
                    "parallel":row["parallel"],"output":{key:row["output"][key] for key in OUTPUT_FIELDS}}
            fields = metrics(a)
            before = flatten(fields)
            after = flatten(metrics(b))
            ratios = {field: after[field]/value if value else None for field, value in before.items()}
            growth.append(dict(scan=key[0], kmax=key[1], s=key[2], backend=key[3], workers=key[4],
                               n=[a["n"], b["n"]], ratios=ratios,
                               quadratic_step=(b["n"]/a["n"])**2,
                               above_quadratic_step=[field for field,value in ratios.items()
                                   if value is not None and value>(b["n"]/a["n"])**2]))
    return dict(status="passed", measurements=len(rows), growth=growth,
                full_contract_qualified=False, universal_subquadratic_claim=False)



def matrix_from_args(args):
    matrix={key:list(getattr(args,key)) for key in ("scans","sizes","kmax","s","workers","backends")}
    matrix["payload"]=args.payload
    validate_matrix(matrix)
    return matrix

def validate_matrix(matrix):
    exact_fields(matrix,("scans","sizes","kmax","s","workers","backends","payload"),"matrix")
    for key in ("scans","sizes","kmax","s","workers","backends"):
        values=matrix[key]
        require(type(values) is list and values and all(type(v) is int for v in values) and
            len(values)==len(set(values)),"empty/duplicate/noninteger matrix axis")
    require(all(v in (0,100,200) for v in matrix["scans"]) and
        all(0<v<=50000 for v in matrix["sizes"]) and all(v in (5,10) for v in matrix["kmax"]) and
        all(v in (8,10,12) for v in matrix["s"]) and all(v>0 for v in matrix["workers"]) and
        all(v in (28,30) for v in matrix["backends"]) and matrix["payload"] in ("digest","records"),
        "unsupported global matrix")

def prepare_matrix(build,matrix):
    validate_matrix(matrix)
    files=set();datasets=[];commands=[];raw_cache={}
    for scan in matrix["scans"]:
        directory=ROOT/edge.PREPARED/f"single_{scan:06}"
        meta_path=directory/"METADATA.json";metadata=read_json(meta_path)
        require(metadata["frames"]==[scan] and metadata["frame"]=="LiDAR_scan_000000" and
            metadata["sampling"]==dict(hash="blake2b_128(seed_bytes + packed_u16le_xyz)",order="ascending_priority",
                seed_u32_le=3,targets=[8000,16000,32000,50000],tie_break="lexicographic_xyz") and
            metadata["quantization"]==dict(all_axes_same_grid=True,clipping_jitter_adaptation=False,
                formula="floor(50*x + 32768 + 0.5)",invalid_input_policy="reject_entire_preparation",step_m=0.02),
            "prepared scan/frame/quantization differs")
        files.add(str(meta_path.relative_to(ROOT)))
        for n in matrix["sizes"]:
            source_n=next(size for size in (8000,16000,32000,50000) if size>=n)
            source=directory/f"n{source_n}.u16le"
            if source not in raw_cache:
                raw=source.read_bytes()
                entry=next((e for e in metadata["samples"] if e["n"]==source_n),None)
                require(entry is not None and entry["requested_n"]==source_n and entry["status"]=="prepared" and
                    entry["path"]==f"single_{scan:06}/n{source_n}.u16le" and entry["bytes"]==len(raw)==6*source_n and
                    edge.hashlib.sha256(raw).hexdigest()==entry["sha256"],"prepared input bytes/hash differ")
                raw_cache[source]=raw
            points=list(edge.struct.iter_unpack("<HHH",raw_cache[source][:6*n]))
            require(len(set(points))==n,"duplicate prepared prefix coordinates")
            datasets.append(dict(scan=scan,n=n,source=str(source),input_hash=edge.input_hash(points)))
            files.add(str(source.relative_to(ROOT)))
            for k in matrix["kmax"]:
                for s in matrix["s"]:
                    for backend in matrix["backends"]:
                        for workers in matrix["workers"]:
                            commands.append([str(build/"mhgp8_wspd_q34_probe"),str(source),str(n),str(k),str(s),
                                "6",str(backend),str(workers),"samples",matrix["payload"]])
    return datasets,commands,files

def artifacts_for(build):
    return {str((build/name).relative_to(ROOT)) for name in ("mhgp8_wspd_q34_probe","libmhgp8_p0.a","CMakeCache.txt")}

def configuration_from_launch(command):
    require(type(command) is list and len(command)>=3 and all(type(v) is str for v in command) and
        Path(command[1]).name=="run_wspd_q34_lidar.py" and command[2]=="run","invalid launch command")
    try:args=make_parser().parse_args(command[2:])
    except SystemExit:raise edge.InvalidReceipt("unreadable launch configuration")
    require(args.operation=="run","launch is not a run")
    return args.build.resolve(),matrix_from_args(args)

def run(args):
    build=args.build.resolve();matrix=matrix_from_args(args)
    require(build.is_relative_to(ROOT/"build") and build.is_dir(),"fresh local build required")
    require(all((ROOT/p).resolve()==ROOT/p for p in artifacts_for(build)),"linked build artefact")
    args.output.mkdir(parents=True,exist_ok=True)
    path=Path(tempfile.mkdtemp(prefix="global_",dir=args.output)).resolve()
    source_before=pins(SOURCES);artifact_before=pins(artifacts_for(build))
    datasets,commands,files=prepare_matrix(build,matrix)
    environment=dict(os.environ)
    cache=(build/"CMakeCache.txt").read_text()
    def git(*options):return subprocess.check_output(["git",*options],cwd=ROOT,text=True).strip()
    manifest=dict(schema=SCHEMA,started_utc=utc_stamp(),build=str(build),matrix=matrix,commands=commands,datasets=datasets,
        source_sha256=source_before,artifact_sha256=artifact_before,input_sha256=pins(files),
        environment=edge.environment_record(environment),affinity=sorted(os.sched_getaffinity(0)),
        command=[sys.executable,*sys.argv],git_commit=git("rev-parse","HEAD"),branch=git("branch","--show-current"),
        worktree=git("status","--short"),compiler_cache="\n".join(line for line in cache.splitlines() if
            line.startswith(("CMAKE_CXX_COMPILER","CMAKE_CXX_FLAGS","CMAKE_BUILD_TYPE:","CMAKE_GENERATOR:","MHGP8_SANITIZE:"))),
        gcp_used=False,public_status="not_claimed",full_contract_qualified=False)
    write_json(path/"MANIFEST.json",manifest)
    handlers={sig:signal.signal(sig,on_signal) for sig in (signal.SIGINT,signal.SIGTERM)}
    records=[];rows=[];status="failed";error=None
    try:
        for i,command in enumerate(commands):
            print(json.dumps(dict(path=str(path),index=i,command=command)),flush=True)
            record=dict(command=command,cwd=str(ROOT),environment=manifest["environment"],started_utc=utc_stamp(),
                status="failed",exit_code=None,stdout="",stderr="",stdout_base64="",stderr_base64="")
            try:
                invoke(command,environment,ROOT,record,new_session=True)
                require(record["exit_code"]==0 and not record["stderr"],"global command failed")
                row=parse_result(record["stdout"].encode());validate(row,command)
                dataset=next(d for d in datasets if d["source"]==command[1] and d["n"]==row["n"])
                require(row["input_hash"]==dataset["input_hash"],"constructor input hash differs")
                row["scan"]=dataset["scan"];record["result"]=row;record["status"]="passed";rows.append(row)
                print(json.dumps(dict(index=i,n=row["n"],ms=row["timings_ms"]["pipeline_including_shared_preparation"],
                    edges=row["work"]["expanded_pairs"],outputs=row["output"]["callbacks"])),flush=True)
            finally:
                record["finished_utc"]=utc_stamp();file=path/f"record_{i:04}.json"
                write_json(file,record);records.append(dict(path=file.name,sha256=digest(file)))
        summary(rows);status="passed"
    except BaseException as cause:
        error=f"{type(cause).__name__}: {cause}";raise
    finally:
        for sig in handlers:signal.signal(sig,signal.SIG_IGN)
        errors=[]
        def close(label,fn):
            try:return fn()
            except BaseException as cause:
                errors.append(f"{label}: {type(cause).__name__}: {cause}");return None
        completion=dict(status=status,error=error,finished_utc=utc_stamp(),records=records,
            manifest_sha256=close("manifest",lambda:digest(path/"MANIFEST.json")),
            source_sha256_after=close("sources",lambda:pins(SOURCES)),
            artifact_sha256_after=close("artifacts",lambda:pins(artifacts_for(build))),
            input_sha256_after=close("inputs",lambda:pins(files)),closing_errors=errors)
        for key in ("source_sha256","artifact_sha256","input_sha256"):
            if errors or manifest[key]!=completion[key+"_after"]:
                completion.update(status="failed",error=error or "capture closure changed or unreadable")
        write_json(path/"COMPLETION.json",completion)
        for sig,handler in handlers.items():signal.signal(sig,handler)
        print(json.dumps(dict(path=str(path),status=completion["status"],error=completion["error"])),flush=True)
    require(completion["status"]=="passed","capture closure failed")

def manifest_plan(m,c,path):
    require(type(m) is dict and type(c) is dict and m["schema"] in (SCHEMA,HISTORICAL_SCHEMA) and
        m["gcp_used"] is False and m["full_contract_qualified"] is False and m["public_status"]=="not_claimed" and
        c["status"]=="passed" and c["error"] is None and digest(path/"MANIFEST.json")==c["manifest_sha256"],
        "capture identity/closure")
    legacy=m["schema"]==HISTORICAL_SCHEMA
    if not legacy:
        require(c.get("closing_errors")==[] and m["branch"]=="main" and type(m["worktree"]) is str and
            type(m["compiler_cache"]) is str and "CMAKE_CXX_COMPILER:" in m["compiler_cache"],"provenance/closure missing")
    require(type(m["git_commit"]) is str and len(m["git_commit"])==40 and
        all(v in "0123456789abcdef" for v in m["git_commit"]) and
        m["environment"]["scope"]=="selected_values_full_environment_fingerprint_no_secrets","missing commit/environment")
    build,matrix=configuration_from_launch(m["command"])
    require(build.is_absolute() and build.is_relative_to(ROOT/"build") and ".." not in build.parts,"invalid build")
    if not legacy:require(m["build"]==str(build) and m["matrix"]==matrix,"explicit launch/matrix differs")
    datasets,commands,files=prepare_matrix(build,matrix)
    require(m["datasets"]==datasets and m["commands"]==commands and commands and
        len(c["records"])==len(commands),"input or complete reconstructed plan differs")
    require(set(m["source_sha256"])==SOURCES and set(m["artifact_sha256"])==artifacts_for(build) and
        set(m["input_sha256"])==files,"exact source/artifact/input inventory differs")
    for key in ("source_sha256","artifact_sha256","input_sha256"):
        require(m[key]==c[key+"_after"] and all(type(h) is str and len(h)==64 and
            all(v in "0123456789abcdef" for v in h) for h in m[key].values()),"closure hash differs")
    require(pins(files)==m["input_sha256"],"original prepared inputs changed")
    require({p.name for p in path.glob("record_*.json")}=={f"record_{i:04}.json" for i in range(len(commands))},
            "record inventory missing or has extras")
    return legacy,commands,datasets

def read(path,live=False):
    path=path.resolve();m=read_json(path/"MANIFEST.json");c=read_json(path/"COMPLETION.json")
    legacy,commands,datasets=manifest_plan(m,c,path)
    if live:
        for key in ("source_sha256","artifact_sha256","input_sha256"):require(m[key]==pins(m[key]),"live hash differs")
    rows=[]
    for i,(item,command) in enumerate(zip(c["records"],commands,strict=True)):
        require(item["path"]==f"record_{i:04}.json" and digest(path/item["path"])==item["sha256"],"record closure")
        record=read_json(path/item["path"])
        require(record["status"]=="passed" and type(record["exit_code"]) is int and record["exit_code"]==0 and
            record["command"]==command and not record["stderr"],"command/result status")
        if not legacy:require(record["cwd"]==str(ROOT) and record["environment"]==m["environment"],"command provenance")
        for stream in ("stdout","stderr"):
            require(edge.base64.b64decode(record[stream+"_base64"],validate=True).decode("utf-8",errors="replace")==record[stream],
                    "raw/decoded log differs")
        row=parse_result(record["stdout"].encode());validate(row,command)
        dataset=next(d for d in datasets if d["source"]==command[1] and d["n"]==row["n"])
        require(row["input_hash"]==dataset["input_hash"],"input hash mismatch")
        row["scan"]=dataset["scan"];require(row==record["result"],"parsed/raw mismatch");rows.append(row)
    result=summary(rows)
    result.update(capture_schema=m["schema"],historical_capture=legacy,
        reader_scope="current_strict_reader_does_not_reexecute_or_promote_historical_capture",
        sources=len(SOURCES),input_files=len(m["input_sha256"]))
    return result

def selftest(path):
    summary_result=read(path)
    m=read_json(path/"MANIFEST.json");c=read_json(path/"COMPLETION.json")
    records=[read_json(p) for p in sorted(path.glob("record_*.json"))]
    base=next((r for r in records if r["result"]["output"]["callbacks"]>0),None)
    require(base is not None,"selftest requires a real nonempty result")
    row=deepcopy(base["result"]);row.pop("scan");validate(row,base["command"])
    tests=0
    def reject(action):
        nonlocal tests
        try:action()
        except edge.InvalidReceipt:tests+=1
        else:raise edge.InvalidReceipt("global reader mutation survived")
    def alter(route,value):
        changed=deepcopy(row);target=changed
        for key in route[:-1]:target=target[key]
        target[route[-1]]=value
        reject(lambda:validate(changed,base["command"]))
    for key in ("n","source_n","kmax","s","mask","q4_backend","workers","input_hash"):alter([key],True)
    if "records" in row:
        alter(["output","sum"],format(int(row["output"]["sum"],16)^1,"x"))
    else:
        changed=deepcopy(base["result"])
        changed["output"]["sum"]=format(int(changed["output"]["sum"],16)^1,"x")
        reject(lambda:paired([base["result"],changed]))
    # Digest corruption is linked to full records only; paired digest-only data
    # needs another engine/run or an external oracle to refute changed content.
    for route,value in ((["work","expanded_pairs"],row["work"]["expanded_pairs"]+1),
        (["parallel","completed_jobs"],row["parallel"]["completed_jobs"]+1),
        (["cloud_work","coordinate_copies"],0),(["timings_ms","cloud"],float("nan")),
        (["front","work","rejected_pair_mass"],[0,0]),(["output","callbacks"],row["output"]["callbacks"]+1)):
        alter(route,value)
    for section,field in (("work","peak_cover_bytes"),("memory","input_capacity_bytes"),("parallel","callback_storage_bytes")):
        changed=deepcopy(row);del changed[section][field];reject(lambda:validate(changed,base["command"]))
    if "records" in row and row["records"]:
        for route,value in ((["records",0,"depth"],True),(["records",0,"support"],[False,1,2]),
            (["records",0,"coefficients"],["1"]),(["records",0,"coefficients"],["01","0","0","0","0"]),
            (["records",0,"shell"],[])):
            alter(route,value)
        changed=deepcopy(row);changed["records"].append(deepcopy(changed["records"][0]))
        reject(lambda:validate(changed,base["command"]))
    changed_command=base["command"].copy();changed_command[2]=str(int(changed_command[2])+1)
    reject(lambda:validate(row,changed_command))
    for target,field in (("source_sha256","invented.cpp"),("artifact_sha256","invented.bin"),("input_sha256","invented.u16le")):
        mm=deepcopy(m);mm[target][field]="0"*64
        reject(lambda:manifest_plan(mm,c,path))
    cc=deepcopy(c);cc["records"]=cc["records"][:-1]
    reject(lambda:manifest_plan(m,cc,path))
    reject(lambda:parse_result(b'{"x":NaN}'));reject(lambda:parse_result(b'{"x":1,"x":2}'))
    return dict(status="passed",mutants=tests,real_measurements=summary_result["measurements"],
                scope="receipt_reader_not_geometry")

def make_parser():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest="operation",required=True)
    runner=sub.add_parser("run")
    runner.add_argument("--build",type=Path,required=True);runner.add_argument("--output",type=Path,required=True)
    runner.add_argument("--scans",type=int,nargs="+",default=[0]);runner.add_argument("--sizes",type=int,nargs="+",required=True)
    runner.add_argument("--kmax",type=int,nargs="+",default=[10]);runner.add_argument("--s",type=int,nargs="+",default=[8])
    runner.add_argument("--workers",type=int,nargs="+",default=[1,4]);runner.add_argument("--backends",type=int,nargs="+",default=[28,30])
    runner.add_argument("--payload",choices=("digest","records"),default="digest")
    reader=sub.add_parser("read");reader.add_argument("path",type=Path);reader.add_argument("--check-live",action="store_true")
    reader.add_argument("--compact",action="store_true")
    unit=sub.add_parser("selftest");unit.add_argument("path",type=Path)
    return parser

def main():
    args=make_parser().parse_args()
    if args.operation=="run":run(args)
    else:
        result=read(args.path,args.check_live) if args.operation=="read" else selftest(args.path)
        if getattr(args,"compact",False):result={key:value for key,value in result.items() if key!="growth"}
        print(json.dumps(result,sort_keys=True,allow_nan=False))

if __name__=="__main__":main()
