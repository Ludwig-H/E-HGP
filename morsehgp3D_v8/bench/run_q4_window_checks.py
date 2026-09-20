#!/usr/bin/env python3
"""Exact two-pass q4 root windows compared with freshly executed shallow29.

Explicit frozen-collector/work-validator adapter. Both preparations are paid
once per edge per engine, both candidate streams are normalized and checked,
and all first-pass, heap, second-pass, sorting and output counters are exposed.
No old capture or benchmark runtime is transferred to the new engine.
"""
from __future__ import annotations
import argparse
from copy import deepcopy
import json
import math
import os
from pathlib import Path
import xml.etree.ElementTree as ET

import run_q4_shallow_checks as reference
shared = reference.reference
previous = reference.previous
collector = previous.collector
ROOT = previous.ROOT
from run_p0_matrix import InvalidReceipt, parse_result, require, uint
from run_q4_family_checks import counts, read_json, validate_closure

SOURCES = reference.SOURCES | frozenset({
    "morsehgp3D_v8/src/lanes/q4_window.hpp", "morsehgp3D_v8/src/lanes/q4_window.cpp",
    "morsehgp3D_v8/tests/q4_window_gate.cpp",
    "morsehgp3D_v8/bench/q4_window_probe.cpp", "morsehgp3D_v8/bench/run_q4_window_checks.py",
})
SCHEMA = "mhgp8_q4_window_attempt_v1"
GATES = (*reference.GATES, "mhgp8_q4_window_gate")
REGIMES = reference.REGIMES
WINDOW_FIELDS = ["seed_queries","entry_heap_insertions","exit_heap_insertions","entry_heap_replacements","exit_heap_replacements","heap_comparisons","heap_sort_comparisons","constant_rejected_seeds","disjoint_rejected_seeds","fixed_depth_rejected_seeds","lower_bounds","upper_bounds","point_windows","second_pass_sites","window_comparisons","lower_ids","upper_ids","inner_ids","outside_ids","fixed_inside_sites","rejected_event_ids","max_inner_ids","max_endpoint_ids","peak_heap_bytes","peak_buffer_bytes"]
GATE_FLOORS = dict(checks=1,families=1,root_groups=1,root_ids=1,constant_rejections=1,
    empty_windows=1,fixed_rejections=1,point_windows=1,point_windows_with_constants=1,
    point_window_candidates=1,lower_infinite=1,upper_infinite=1,both_infinite=1,finite_intervals=1,
    mixed_groups=1,constant_inside=1,constant_shells=1,endpoint_ids=1,inner_ids=1,fixed_inside=1,
    outside_ids=1,depth_rejected_groups=1,removed_seed_rejections=1,interior_bound_checks=1,
    strict_shell_checks=1,extreme_calls=1,allocation_failures=4,second_pass_sites=1,
    heap_comparisons=1,heap_replacements=1,huge_k_calls=1,edge_calls=1,seed_calls=1,reference_calls=1,
    oracle_completions=1,oracle_sites=1,candidates=1,q4=1,max_shell=30,exhaustive_edges=10,
    permutations=1,invalid_inputs=1,callback_failures=1,parallel_calls=4,isolated_point_fixtures=1)
FIXED = dict(schema="mhgp8_q4_window_probe_v1",status="completed",
    scope="one_edge_q4_exact_root_window_not_global_producer",public_status="not_claimed",
    backend="cpu_reference",profile="quantized_u16_input_only",threads=1,edge_ids=[0,1],
    timing_scope="enclosing_wall_including_validation_release_pipeline_sums_share_preparation",
    baseline="q4_shallow29_really_executed_q4_only")
PART_TIMES = reference.PART_TIMES
structural = reference.structural
dense = reference.dense

def validate_work(row,n,initial_seeds,population,id_bytes):
    w=row["work"];b=row["baseline_work"]
    structural(w,reference.EDGE_FIELDS,("geometry","selection","sweep"),"work")
    structural(w["geometry"],shared.GEOMETRY_FIELDS,("domain",),"geometry")
    counts(w["geometry"]["domain"],shared.DOMAIN_FIELDS,"geometry.domain")
    counts(w["selection"],reference.SELECTION_FIELDS,"selection")
    require(w["geometry"]==b["geometry"] and w["selection"]==b["selection"] and
        all(w[key]==b[key] for key in reference.EDGE_FIELDS if key!="peak_live_buffer_bytes"),
        "window changed shared exact geometry, selection or retained seeds")
    bundle=w["sweep"];structural(bundle,(),("sweep","window"),"window bundle")
    s,p=bundle["sweep"],bundle["window"]
    structural(s,reference.SWEEP_FIELDS,("family",),"sweep")
    counts(p,WINDOW_FIELDS,"window");f=s["family"];counts(f,previous.FAMILY_FIELDS,"family")
    old=b["sweep"];old_family=old["family"]
    r=w["selection"]["retained_ids"];seeds=w["seeds"];threshold=row["kmax"]-2
    first_pass=("sites","entries","exits","constant_inside","constant_on","constant_outside","event_count")
    require(all(f[key]==old_family[key] for key in first_pass) and f["sites"]==seeds*r,
        "first complete retained-site pass differs from29")
    unchanged=("seed_queries","seed_owner_tests","seed_owner_rejections","removed_seed_rejections",
        "membership_comparisons","presentations","owner_tests","owner_rejections","positive_tests",
        "positive_rejections","canonical_tests","canonical_rejections","groups_without_support",
        "unexamined_after_emit","emitted","shell_ids")
    require(all(s[key]==old[key] for key in unchanged),"window changed non-depth rejection or output work")
    require(p["seed_queries"]==seeds and
        p["constant_rejected_seeds"]+p["disjoint_rejected_seeds"]+p["fixed_depth_rejected_seeds"]<=seeds and
        threshold*p["constant_rejected_seeds"]<=f["constant_inside"] and
        threshold*p["fixed_depth_rejected_seeds"]<=f["constant_inside"]+p["fixed_inside_sites"],
        "window seed rejection partition differs")
    second_seeds=seeds-p["constant_rejected_seeds"]-p["disjoint_rejected_seeds"]
    require(p["second_pass_sites"]==r*second_seeds and
        p["fixed_inside_sites"]<=p["outside_ids"] and
        p["lower_ids"]+p["upper_ids"]+p["inner_ids"]+p["outside_ids"]<=p["second_pass_sites"],
        "second COMPLETE retained-site pass or fixed contributions differ")
    for side in ("entry","exit"):
        events=f["entries" if side=="entry" else "exits"]
        insertions=p[side+"_heap_insertions"];replacements=p[side+"_heap_replacements"]
        require(insertions<=min(events,seeds*threshold) and replacements<=events-insertions and
            (insertions>0)==(events>0),"bounded heap insertion/replacement ledger differs")
    require(p["heap_comparisons"]>=f["event_count"]-p["entry_heap_insertions"]-p["exit_heap_insertions"] and
        p["lower_bounds"]<=seeds-p["constant_rejected_seeds"] and
        p["upper_bounds"]<=seeds-p["constant_rejected_seeds"] and
        p["point_windows"]<=min(p["lower_bounds"],p["upper_bounds"],second_seeds) and
        p["point_windows"]+p["disjoint_rejected_seeds"]<=p["window_comparisons"]<=
        2*p["second_pass_sites"]+seeds-p["constant_rejected_seeds"],
        "heap comparisons or closed-window bound ledger differs")
    require(p["inner_ids"]<=2*(threshold-1)*second_seeds and
        p["max_inner_ids"]<=min(2*(threshold-1),r,p["inner_ids"]) and
        p["max_endpoint_ids"]<=min(r,p["lower_ids"]+p["upper_ids"]),
        "strict-interior rank bound or whole endpoint capacity differs")
    processed=s["presentations"]+s["depth_skipped_ids"]+s["unexamined_after_emit"]
    window_ids=p["lower_ids"]+p["upper_ids"]+p["inner_ids"]
    require(f["event_count"]==p["outside_ids"]+p["rejected_event_ids"]+processed and
        0<=window_ids-processed<=p["rejected_event_ids"] and
        f["groups"]==f["callbacks"]<=processed and f["max_group"]*f["groups"]>=processed and
        f["max_group"]<=old_family["max_group"] and f["group_comparisons"]<=p["inner_ids"],
        "outside/rejected/processed roots partition differs")
    if p["fixed_depth_rejected_seeds"]==0:
        require(0<=p["inner_ids"]-f["group_comparisons"]<=second_seeds,
            "processed strict-interior grouping differs")
    if p["max_inner_ids"]<=1:
        require(f["sort_comparisons"]==f["group_comparisons"]==0,
            "zero/singleton strict interiors performed root sorting")
    require(f["groups"]==s["depth_rejected_groups"]+s["groups_without_support"]+s["emitted"] and
        s["presentations"]==s["owner_rejections"]+s["positive_tests"] and
        s["positive_tests"]==s["positive_rejections"]+s["canonical_tests"] and
        s["canonical_tests"]==s["canonical_rejections"]+s["emitted"] and
        s["emitted"]==row["digest"]["q4"] and s["shell_ids"]==row["digest"]["shell_ids_visited"],
        "window presentation/depth/output partition differs")
    require(p["peak_buffer_bytes"]==s["peak_buffer_bytes"]==f["retained_capacity_bytes"] and
        p["peak_buffer_bytes"]>=p["peak_heap_bytes"] and
        p["peak_buffer_bytes"]>=id_bytes*max(f["max_group"],p["max_inner_ids"],p["max_endpoint_ids"]) and
        w["peak_live_buffer_bytes"]==w["geometry"]["peak_retained_bytes"]+
        max(w["selection"]["peak_live_bytes"],w["selection"]["retained_bytes"]+p["peak_buffer_bytes"]),
        "six simultaneous vector/selection/geometry capacities differ")

def validate_row(row,command):
    require(type(row) is dict and len(command)==4,"window command arity")
    n,regime,k=int(command[1]),command[2],int(command[3])
    require(n>=8 and regime in REGIMES and k in (5,10) and
        (regime!="cap" or n<=35307) and (regime!="adversarial" or n<=514) and
        (not dense(regime) or n<=32720),"fixture/option domain differs")
    extra={"n","regime","kmax","input_hash","expected_initial_seeds","cover_sites",
        "generation","validation","baseline_work","baseline_digest","work","digest",
        "cloud_work","index_work","cover_work","memory","timings"}
    require(set(row)==set(FIXED)|extra and
        all(type(row[key]) is type(value) and row[key]==value for key,value in FIXED.items()) and
        all(type(x) is int for x in row["edge_ids"]),"window schema/scope differs")
    require(type(row["n"]) is int and row["n"]==n and type(row["kmax"]) is int and row["kmax"]==k and row["regime"]==regime,
        "command/result differs")
    uint(row["input_hash"],"input_hash")
    seeds=n-2 if regime=="adversarial" or dense(regime) else 2
    population=6 if regime=="far" else n
    require(uint(row["expected_initial_seeds"],"expected_initial_seeds")==seeds and
        uint(row["cover_sites"],"cover_sites")==population,"fixture initial ownership/population differs")
    g=row["generation"];counts(g,shared.GENERATION_FIELDS,"generation")
    require(g["proposals"]==n-(2 if regime=="adversarial" or dense(regime) else 6)+g["duplicates"] and
        g["random_calls"]==(3*g["proposals"] if regime=="far" else 0) and
        (regime=="far" or g["duplicates"]==0),"generation ledger differs")
    if dense(regime):
        require(g["grid_size"]==32718 and g["permutation_keys"]==(32718 if regime=="dense_permuted" else 0) and
            (g["permutation_sort_comparisons"]>0)==(regime=="dense_permuted") and
            (g["auxiliary_capacity_bytes"]>0)==(regime=="dense_permuted"),"dense generation/permutation differs")
    else:require(all(g[key]==0 for key in ("grid_size","permutation_keys","permutation_sort_comparisons","auxiliary_capacity_bytes")),
        "non-dense fixture performed dense generation")
    d=row["digest"];counts(d,previous.DIGEST_FIELDS,"digest")
    counts(row["baseline_digest"],previous.DIGEST_FIELDS,"baseline_digest")
    require(row["baseline_digest"]==d and d["callbacks"]==d["q4"] and d["q3"]==0 and
        d["support_ids_visited"]==4*d["q4"] and d["shell_ids_visited"]>=d["support_ids_visited"],
        "actual shallow29/window30 output differential differs")
    if regime in ("far","cap"):require(d==shared.expected_digest(),"closed-form complete q4 fixture differs")
    if regime=="adversarial":require(d["q4"]>0,"bounded adverse output is vacuous")
    m=row["memory"];structural(m,shared.MEMORY_FIELDS,("scope",),"memory")
    require(m["scope"]=="dynamic_capacities_not_RSS_fixed_objects_or_judge_map_nodes" and m["id_bytes"] in (4,8) and
        min(m["input_capacity_bytes"],m["cloud_retained_bytes"])>=6*n and m["index_retained_bytes"]>=n*m["id_bytes"] and
        min(m["output_retained_bytes"],m["baseline_output_retained_bytes"])>=d["shell_ids_visited"]*m["id_bytes"],
        "memory scope/capacity differs")
    if regime=="dense_permuted":require(g["auxiliary_capacity_bytes"]>=32718*(8+m["id_bytes"]),"permutation capacity omitted")
    v=row["validation"];structural(v,shared.VALIDATION_FIELDS,("method",),"validation")
    require(v["method"]=="independent_rational_support_and_full_census_per_published_ball_not_large_completeness" and
        v["fixture_point_tests"]==n and v["supports"]==d["q4"] and v["owner_distance_tests"]==6*v["supports"] and
        v["distinct_balls"]<=v["supports"] and (v["distinct_balls"]>0)==(v["supports"]>0) and
        v["point_tests"]==n*v["distinct_balls"] and v["shell_ids"]>=4*v["distinct_balls"] and
        v["strict_interiors"]<=(k-3)*v["distinct_balls"] and v["shell_capacity_bytes"]>=v["shell_ids"]*m["id_bytes"],
        "independent published-ball census differs")
    c=row["cloud_work"];counts(c,previous.CLOUD_FIELDS,"cloud")
    require(c["coordinate_copies"]==c["validation_points"]==c["range_tree_leaf_visits"]==n and
        c["uniqueness_adjacent_tests"]==c["range_tree_merges"]==n-1 and c["range_tree_nodes"]==2*n-1,"shared cloud work differs")
    ix=row["index_work"];counts(ix,previous.INDEX_FIELDS,"index")
    require(ix["nodes"]==ix["escape_links"]==2*n-1 and ix["point_visits"]>=n and ix["max_depth"]>0,"shared index work differs")
    c=row["cover_work"];counts(c,previous.COVER_FIELDS,"cover")
    require(c["node_visits"]==c["bound_tests"]+c["point_tests"]<=2*n-1 and
        c["node_visits"]==c["admitted_nodes"]+c["rejected_nodes"]+c["split_nodes"] and
        c["admitted_sites"]==population and c["admitted_sites"]+c["rejected_sites"]==n and
        c["retained_ranges"]+c["merged_ranges"]==c["admitted_nodes"] and c["retained_ranges"]>0 and
        m["cover_retained_bytes"]>=2*m["id_bytes"]*c["retained_ranges"],"shared cover work differs")
    # Validate ONLY the freshly executed shallow29 work, never a fabricated
    # local28 execution or an inherited historical receipt.
    baseline_view={**row,"work":row["baseline_work"],"digest":row["baseline_digest"]}
    reference.validate_work(baseline_view,n,seeds,population,m["id_bytes"])
    validate_work(row,n,seeds,population,m["id_bytes"])
    t=row["timings"]
    require(type(t) is dict and set(t)==set(PART_TIMES)|{"baseline_prepare_run_sum_ms","window_prepare_run_sum_ms","total_ms"} and
        all(type(value) in (int,float) and math.isfinite(value) and value>=0 for value in t.values()),"invalid timings")
    common=("generation_ms","cloud_ms","index_ms","cover_ms")
    for total,parts in (("total_ms",PART_TIMES),("window_prepare_run_sum_ms",(*common,"run_callback_ms")),
                        ("baseline_prepare_run_sum_ms",(*common,"baseline_run_callback_ms"))):
        require(math.isclose(t[total],sum(t[key] for key in parts),rel_tol=1e-12,abs_tol=1e-6),"inclusive timing partition differs")

def validate_gate(row,executable):
    if executable in reference.GATES:return reference.validate_gate(row,executable)
    require(GATE_FLOORS and executable==GATES[-1] and type(row) is dict and
        set(row)==set(GATE_FLOORS)|{"schema","status"} and row["schema"]==executable+"_v1" and row["status"]=="passed",
        "window gate schema/fields differ")
    for key,floor in GATE_FLOORS.items():require(uint(row[key],"gate."+key)>=floor,"window gate floor failed")
    require(row["candidates"]==row["q4"] and row["exhaustive_edges"]==10 and
        row["parallel_calls"]==4 and row["callback_failures"]==1 and row["allocation_failures"]==4 and
        row["huge_k_calls"]==1 and row["isolated_point_fixtures"]==1,
        "window gate exact output/lifecycle ledger differs")

def validate_xml(path):
    tree=ET.parse(path).getroot();cases=tree.findall("testcase");names=[c.get("name") for c in cases]
    require(tree.tag=="testsuite" and tree.get("tests")=="91" and all(tree.get(key)=="0" for key in ("failures","disabled","skipped")) and
        len(cases)==len(set(names))==91 and all(type(name) is str and name.startswith("mhgp8_") for name in names) and
        set(GATES)|{"mhgp8_q4_family_gate"}<=set(names),"regression91 inventory/status differs")
    require(all(c.get("status")=="run" and all(c.find(tag) is None for tag in ("failure","error","skipped")) for c in cases),
        "regression testcase failed/skipped")
    return sorted(names)

def plan(build,campaign,output,ctest=None):
    if campaign=="regression":return [("ctest",[ctest,"--test-dir",str(build),"--output-on-failure","--parallel","2","--output-junit",str(output/"result.xml")])]
    result=[("gate",[str(build/name),"--selftest"]) for name in GATES]
    for regime in REGIMES:
        sizes=() if campaign=="gate" else (32,) if campaign=="smoke" else ((32,64,128,256) if regime=="adversarial" else (8000,16000,32000))
        for k in (5,10):
            for n in sizes:
                result.append(("measure",[str(build/"mhgp8_q4_window_probe"),str(n),regime,str(k)]))
    return result

def executable_names(build,campaign):
    if campaign=="regression":
        names=sorted(p.name for p in build.glob("mhgp8_*") if p.is_file() and os.access(p,os.X_OK))
        require(names and all((build/name).resolve()==build/name for name in names),"missing or linked executable")
        return names
    return sorted([*GATES,*(["mhgp8_q4_window_probe"] if campaign!="gate" else [])])

def install_adapters():
    collector.SOURCES,collector.SCHEMA,collector.GATES=SOURCES,SCHEMA,GATES
    collector.FAMILIES=()
    collector.plan,collector.executable_names=plan,executable_names
    collector.validate_row,collector.validate_gate,collector.validate_xml=validate_row,validate_gate,validate_xml

def read(path,check_live=False):
    install_adapters();summary=previous.BASE_READ(path,check_live)
    rows=[r["row"] for file in sorted(path.glob("record_*.json")) if (r:=read_json(file))["kind"]=="measure"]
    for regime in REGIMES:
        for n in sorted({r["n"] for r in rows if r["regime"]==regime}):
            pair=[r for r in rows if (r["regime"],r["n"])==(regime,n)]
            require(len(pair)==2 and {r["kmax"] for r in pair}=={5,10},"missing K comparison")
            require(all(all(r[key]==pair[0][key] for key in ("input_hash","generation","cloud_work","index_work","cover_work")) and
                r["work"]["geometry"]==pair[0]["work"]["geometry"] and
                r["baseline_work"]["geometry"]==pair[0]["baseline_work"]["geometry"] for r in pair),"immutable preparation changed")
            lower,upper=sorted(pair,key=lambda r:r["kmax"])
            require(lower["work"]["selection"]["retained_ids"]<=upper["work"]["selection"]["retained_ids"] and
                lower["work"]["seeds"]<=upper["work"]["seeds"],"nested dual layers lost retained sites/seeds")
    growth={}
    if summary["campaign"]=="scale":
        def flatten(value,prefix=""):
            out={}
            for key,item in value.items():
                if type(item) is dict:out.update(flatten(item,prefix+key+"."))
                elif type(item) in (int,float) and key not in ("hash","id_bytes"):out[prefix+key]=item
            return out
        for regime in REGIMES:
            for k in (5,10):
                series=sorted((r for r in rows if (r["regime"],r["kmax"])==(regime,k)),key=lambda r:r["n"])
                metrics=[]
                for r in series:
                    metrics.append(flatten({key:r[key] for key in ("work","baseline_work","generation","validation","cloud_work","index_work","cover_work","digest","memory","timings")}))
                    s=r["work"]["sweep"]["sweep"];p=r["work"]["sweep"]["window"];f=s["family"]
                    processed=s["presentations"]+s["depth_skipped_ids"]+s["unexamined_after_emit"]
                    old=r["baseline_work"]["sweep"]["family"]
                    metrics[-1].update({
                        "derived.total_site_visits":f["sites"]+p["second_pass_sites"],
                        "derived.processed_event_ids":processed,
                        "derived.window_side_tests":f["sites"]+p["second_pass_sites"]+processed,
                        "derived.baseline_side_tests":old["sites"]+old["event_count"],
                        "derived.window_root_order_comparisons":p["heap_comparisons"]+p["heap_sort_comparisons"]+
                            p["window_comparisons"]+f["sort_comparisons"]+f["group_comparisons"],
                        "derived.baseline_root_order_comparisons":old["sort_comparisons"]+old["group_comparisons"]})
                growth[f"{regime}_K{k}"]={}
                for metric in metrics[0]:
                    values=[m[metric] for m in metrics]
                    growth[f"{regime}_K{k}"][metric]=dict(values=values,ratios=[b/a if a else None for a,b in zip(values,values[1:])],
                        above_quadrupling=[a>0 and b>4*a for a,b in zip(values,values[1:])])
    summary.update(scope=FIXED["scope"],growth=growth,both_engines_really_executed=True,
        dense_large_whole_cover_baseline_executed=False,validation_proves_emission_validity_not_dense_completeness=True,
        reference_runtime_is_q4_only=True,pipeline_sums_are_not_independent_wall_measurements=True)
    return summary

def selftest(path):
    summary=read(path);records=[read_json(file) for file in sorted(path.glob("record_*.json"))]
    base=next(r for r in records if r["kind"]=="measure" and r["row"]["regime"]=="far")
    mutations=0
    def reject(action):
        nonlocal mutations
        try:action()
        except InvalidReceipt:mutations+=1
        else:raise InvalidReceipt("window reader corruption survived")
    for route,value in ((["n"],True),(["edge_ids"],[False,1]),
        (["digest","hash"],0),(["baseline_digest","hash"],0),(["validation","point_tests"],0),
        (["validation","distinct_balls"],True),(["work","geometry","cover_sites"],0),
        (["work","selection","input_sites"],0),(["work","selection","retained_ids"],True),
        (["work","selection","discarded_ids"],999999),(["work","selection","layer_input_ids"],999999),
        (["work","seed_candidates"],0),(["work","sweep","sweep","family","sites"],0),
        (["work","sweep","sweep","depth_skipped_ids"],999999),(["work","peak_live_buffer_bytes"],0),
        (["baseline_work","sweep","family","sites"],0),(["cover_work","admitted_sites"],0),
        (["index_work","nodes"],0),(["memory","id_bytes"],True),(["timings","window_prepare_run_sum_ms"],float("nan")),
        (["work","sweep","window","second_pass_sites"],999999),
        (["work","sweep","window","rejected_event_ids"],999999),
        (["work","sweep","window","inner_ids"],999999),
        (["work","sweep","window","peak_buffer_bytes"],0)):
        row=deepcopy(base["row"]);target=row
        for key in route[:-1]:target=target[key]
        target[route[-1]]=value;reject(lambda:validate_row(row,base["command"]))
    command=base["command"].copy();command[3]="10";reject(lambda:validate_row(base["row"],command))
    reject(lambda:parse_result(b'{"x":NaN}'));reject(lambda:parse_result(b'{"x":1,"x":2}'))
    for rec in records:
        if rec["kind"]!="gate":continue
        gate=deepcopy(rec["row"]);gate["checks"]=True
        reject(lambda:validate_gate(gate,Path(rec["command"][0]).name))
        gate["checks"]=0;reject(lambda:validate_gate(gate,Path(rec["command"][0]).name))
    m,c=read_json(path/"MANIFEST.json"),read_json(path/"COMPLETION.json")
    c["source_sha256_after"][next(iter(m["source_sha256"]))]="0"*64
    reject(lambda:validate_closure(m,c))
    return dict(status="passed",mutants=mutations,real_measurements=summary["measurements"],scope="receipt_reader_not_geometry")

def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest="operation",required=True)
    run=sub.add_parser("run");run.add_argument("--build",type=Path,required=True);run.add_argument("--output",type=Path,required=True)
    run.add_argument("--campaign",choices=("gate","gates","smoke","scale","regression"),required=True)
    reader=sub.add_parser("read");reader.add_argument("path",type=Path);reader.add_argument("--check-live",action="store_true")
    unit=sub.add_parser("selftest");unit.add_argument("path",type=Path)
    args=parser.parse_args();install_adapters()
    if args.operation=="run":
        if args.campaign=="gates":args.campaign="gate"
        collector.run(args)
    else:print(json.dumps(read(args.path,args.check_live) if args.operation=="read" else selftest(args.path),sort_keys=True,allow_nan=False))

if __name__=="__main__":main()
