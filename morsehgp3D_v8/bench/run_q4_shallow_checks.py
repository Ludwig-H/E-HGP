#!/usr/bin/env python3
"""Dual shallow layers plus REAL q4 sweeps, compared with real local28 sweeps.

Explicit adapter of the frozen collector and local28 work validator. No old
receipt or source is changed. Preparations and every retained-seed sweep are
paid. The 32 scale rows compare both engines even on the large dense inputs;
there is no old whole-cover quadratic reference on those inputs.
"""
from __future__ import annotations
import argparse
from copy import deepcopy
import json
import math
import os
from pathlib import Path
import xml.etree.ElementTree as ET

import run_q4_local_checks as reference
previous = reference.previous
collector = previous.collector
ROOT = previous.ROOT
from run_p0_matrix import InvalidReceipt, parse_result, require, uint
from run_q4_family_checks import counts, read_json, validate_closure

SOURCES = reference.SOURCES | frozenset({
    "morsehgp3D_v8/src/lanes/q4_shallow_set.hpp", "morsehgp3D_v8/src/lanes/q4_shallow_set.cpp",
    "morsehgp3D_v8/src/lanes/q4_shallow.hpp", "morsehgp3D_v8/src/lanes/q4_shallow.cpp",
    "morsehgp3D_v8/tests/q4_shallow_gate.cpp",
    "morsehgp3D_v8/bench/q4_shallow_probe.cpp", "morsehgp3D_v8/bench/run_q4_shallow_checks.py",
})
SCHEMA = "mhgp8_q4_shallow_attempt_v1"
GATES = (*reference.GATES, "mhgp8_q4_shallow_gate")
REGIMES = reference.REGIMES
SELECTION_FIELDS = ["preparations","input_sites","form_tests","zero_sites","positive_sites","negative_sites","lex_comparisons","orientation_tests","coordinate_groups","duplicate_ids","positive_layers","negative_layers","layer_input_groups","layer_input_ids","boundary_groups","degenerate_groups","retained_ids","discarded_ids","retained_id_sort_comparisons","record_insertions","group_insertions","hull_index_copies","compaction_moves","peak_live_bytes","retained_bytes"]
SWEEP_FIELDS = ["seed_queries","seed_owner_tests","seed_owner_rejections","removed_seed_rejections","membership_comparisons","depth_rejected_groups","depth_skipped_ids","presentations","owner_tests","owner_rejections","positive_tests","positive_rejections","canonical_tests","canonical_rejections","groups_without_support","unexamined_after_emit","emitted","shell_ids","peak_buffer_bytes"]
EDGE_FIELDS = ["seed_candidates","acute_tests","acute_seeds","owner_tests","owner_rejections","seeds","peak_live_buffer_bytes"]
GATE_FLOORS = dict(checks=1,sets=1,oracle_layers=1,boundary_groups=1,degenerate_groups=1,
    duplicate_ids=1,retained_ids=1,discarded_ids=1,zero_sites=1,positive_sites=1,negative_sites=1,
    query_centres=1,shallow_centres=1,removed_nonpositive=1,strict_witness_checks=1,
    removed_shell_contacts=1,t0_queries=1,collinear_cases=1,extreme_calls=1,boundary_nonvertices=1,
    coincident_duals=1,saved_seed_sites=1,allocation_failures=4,removed_seed_rejections=1,
    retained_seed_queries=1,constant_shells=1,mixed_sign_cases=1,edge_calls=1,seed_calls=1,
    reference_calls=1,oracle_completions=1,oracle_sites=1,candidates=1,q4=1,max_shell=30,
    exhaustive_edges=10,permutations=1,invalid_inputs=1,callback_failures=1,parallel_calls=4)
FIXED = dict(schema="mhgp8_q4_shallow_probe_v1",status="completed",
    scope="one_edge_q4_dual_layers_and_retained_sweeps_not_global_producer",public_status="not_claimed",
    backend="cpu_reference",profile="quantized_u16_input_only",threads=1,edge_ids=[0,1],
    timing_scope="enclosing_wall_including_validation_release_pipeline_sums_share_preparation",
    baseline="q4_local28_really_executed_q4_only")
PART_TIMES = reference.PART_TIMES
structural = reference.structural
dense = reference.dense

def validate_work(row,n,initial_seeds,population,id_bytes):
    w=row["work"];structural(w,EDGE_FIELDS,("geometry","selection","sweep"),"work")
    g,p,s=w["geometry"],w["selection"],w["sweep"]
    structural(g,reference.GEOMETRY_FIELDS,("domain",),"geometry")
    counts(p,SELECTION_FIELDS,"selection")
    structural(s,SWEEP_FIELDS,("family",),"sweep")
    f=s["family"];counts(f,previous.FAMILY_FIELDS,"family")
    reference.validate_domain(g["domain"],n,row["regime"],"disk",g)
    require(g["preparations"]==1 and g["cover_node_visits"]==g["cover_disjoint_nodes"]+g["cover_splits"]+g["cover_blocks"]<=2*n-1 and
        g["cover_sites"]==population and g["cover_sites"]+g["cover_excluded_sites"]==n and
        g["cover_blocks"]==g["cover_node_ids_copied"]>0 and g["cover_range_advances"]<=row["cover_work"]["retained_ranges"] and
        g["peak_retained_bytes"]>=g["cover_blocks"]*id_bytes,"shallow shared geometry ledger differs")
    r=p["retained_ids"];t=row["kmax"]-2
    require(p["preparations"]==1 and p["input_sites"]==p["form_tests"]==population and
        population==p["zero_sites"]+p["positive_sites"]+p["negative_sites"]==r+p["discarded_ids"] and
        2<=p["zero_sites"]<=r and p["record_insertions"]==p["positive_sites"]+p["negative_sites"] and
        p["coordinate_groups"]+p["duplicate_ids"]==p["record_insertions"] and
        p["group_insertions"]==p["coordinate_groups"],"dual sign/group/population partition differs")
    for sign in ("positive","negative"):
        size=p[sign+"_sites"];layers=p[sign+"_layers"]
        require((layers==0)==(size==0) and layers<=t and layers<=size,"dual layer count differs")
    retained_groups=p["boundary_groups"]+p["degenerate_groups"]
    require(retained_groups<=p["coordinate_groups"] and
        retained_groups<=r-p["zero_sites"]<=retained_groups+p["duplicate_ids"] and
        p["coordinate_groups"]<=p["layer_input_groups"]<=t*p["coordinate_groups"] and
        p["record_insertions"]<=p["layer_input_ids"]<=t*p["record_insertions"] and
        p["layer_input_groups"]<=p["layer_input_ids"] and p["compaction_moves"]<=p["layer_input_groups"] and
        p["hull_index_copies"]<=2*p["layer_input_groups"],"whole-boundary layers or preparation work differs")
    require(p["peak_live_bytes"]>=p["retained_bytes"]>=r*id_bytes,"dual preparation/output capacities missing")
    require(w["seed_candidates"]==w["acute_tests"]==r-2 and
        w["acute_seeds"]==w["seeds"]+w["owner_rejections"]<=w["seed_candidates"] and
        w["acute_seeds"]<=w["owner_tests"]<=2*w["acute_seeds"] and
        w["seeds"]<=initial_seeds,"retained seed enumeration ledger differs")
    if row["regime"] in ("far","cap"):require(w["seeds"]==2,"known positive seeds were removed")
    else:require(w["seeds"]==r-2,"all retained nonendpoint sites must be owned acute fixture seeds")
    seeds=w["seeds"]
    require(s["seed_queries"]==seeds and s["seed_owner_tests"]==2*seeds and
        s["seed_owner_rejections"]==s["removed_seed_rejections"]==0 and
        seeds<=s["membership_comparisons"]<=seeds*(r.bit_length()+1),"retained seed query/membership ledger differs")
    require(f["sites"]==seeds*r and
        f["sites"]==sum(f[key] for key in ("entries","exits","constant_inside","constant_on","constant_outside")) and
        f["event_count"]==f["entries"]+f["exits"] and f["constant_on"]>=3*seeds and
        f["groups"]==f["callbacks"]<=f["event_count"] and
        0<=f["event_count"]-f["group_comparisons"]<=seeds and
        f["max_group"]*f["groups"]>=f["event_count"],"retained witness/event partition differs")
    require(f["event_count"]==s["presentations"]+s["depth_skipped_ids"]+s["unexamined_after_emit"] and
        f["groups"]==s["depth_rejected_groups"]+s["groups_without_support"]+s["emitted"] and
        s["presentations"]==s["owner_rejections"]+s["positive_tests"] and
        s["positive_tests"]==s["positive_rejections"]+s["canonical_tests"] and
        s["canonical_tests"]==s["canonical_rejections"]+s["emitted"] and
        s["presentations"]<=s["owner_tests"]<=5*s["presentations"] and
        s["emitted"]==row["digest"]["q4"] and s["shell_ids"]==row["digest"]["shell_ids_visited"],
        "retained depth/presentation/canonical/output partition differs")
    require(s["peak_buffer_bytes"]==f["retained_capacity_bytes"]>=f["max_group"]*id_bytes and
        w["peak_live_buffer_bytes"]==g["peak_retained_bytes"]+
        max(p["peak_live_bytes"],p["retained_bytes"]+s["peak_buffer_bytes"]),
        "simultaneous geometry/selection/sweep capacities differ")

def validate_row(row,command):
    require(type(row) is dict and len(command)==5,"shallow command arity")
    n,regime,k,leaf=int(command[1]),command[2],int(command[3]),int(command[4])
    require(n>=8 and regime in REGIMES and k in (5,10) and leaf>=0 and
        (regime!="cap" or n<=35307) and (regime!="adversarial" or n<=514) and
        (not dense(regime) or n<=32720),"fixture/option domain differs")
    extra={"n","regime","kmax","baseline_options","input_hash","expected_initial_seeds","cover_sites",
        "generation","validation","baseline_work","baseline_digest","work","digest",
        "cloud_work","index_work","cover_work","memory","timings"}
    require(set(row)==set(FIXED)|extra and
        all(type(row[key]) is type(value) and row[key]==value for key,value in FIXED.items()) and
        all(type(x) is int for x in row["edge_ids"]),"shallow schema/scope differs")
    require(type(row["n"]) is int and row["n"]==n and type(row["kmax"]) is int and row["kmax"]==k and row["regime"]==regime,
        "command/result differs")
    options=dict(domain="positive",max_depth=7,node_budget=4096,z_test_budget=512,leaf_sites=leaf,clip_events=True)
    require(type(row["baseline_options"]) is dict and set(row["baseline_options"])==set(options) and
        all(type(row["baseline_options"][key]) is type(value) and row["baseline_options"][key]==value for key,value in options.items()),
        "baseline options differ")
    uint(row["input_hash"],"input_hash")
    seeds=n-2 if regime=="adversarial" or dense(regime) else 2
    population=6 if regime=="far" else n
    require(uint(row["expected_initial_seeds"],"expected_initial_seeds")==seeds and
        uint(row["cover_sites"],"cover_sites")==population,"fixture initial ownership/population differs")
    g=row["generation"];counts(g,reference.GENERATION_FIELDS,"generation")
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
        "actual local28/shallow29 output differential differs")
    if regime in ("far","cap"):require(d==reference.expected_digest(),"closed-form complete q4 fixture differs")
    if regime=="adversarial":require(d["q4"]>0,"bounded adverse output is vacuous")
    m=row["memory"];structural(m,reference.MEMORY_FIELDS,("scope",),"memory")
    require(m["scope"]=="dynamic_capacities_not_RSS_fixed_objects_or_judge_map_nodes" and m["id_bytes"] in (4,8) and
        min(m["input_capacity_bytes"],m["cloud_retained_bytes"])>=6*n and m["index_retained_bytes"]>=n*m["id_bytes"] and
        min(m["output_retained_bytes"],m["baseline_output_retained_bytes"])>=d["shell_ids_visited"]*m["id_bytes"],
        "memory scope/capacity differs")
    if regime=="dense_permuted":require(g["auxiliary_capacity_bytes"]>=32718*(8+m["id_bytes"]),"permutation capacity omitted")
    v=row["validation"];structural(v,reference.VALIDATION_FIELDS,("method",),"validation")
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
    # Explicit adapter: validates ONLY freshly executed baseline28 work,
    # never reads an old receipt or pretends a baseline command was executed.
    baseline_view={**options,"regime":regime,"cover_work":c,"work":row["baseline_work"],"digest":d}
    reference.validate_work(baseline_view,n,seeds,population,m["id_bytes"])
    validate_work(row,n,seeds,population,m["id_bytes"])
    t=row["timings"]
    require(type(t) is dict and set(t)==set(PART_TIMES)|{"baseline_prepare_run_sum_ms","shallow_prepare_run_sum_ms","total_ms"} and
        all(type(value) in (int,float) and math.isfinite(value) and value>=0 for value in t.values()),"invalid timings")
    common=("generation_ms","cloud_ms","index_ms","cover_ms")
    for total,parts in (("total_ms",PART_TIMES),("shallow_prepare_run_sum_ms",(*common,"run_callback_ms")),
                        ("baseline_prepare_run_sum_ms",(*common,"baseline_run_callback_ms"))):
        require(math.isclose(t[total],sum(t[key] for key in parts),rel_tol=1e-12,abs_tol=1e-6),"inclusive timing partition differs")

def validate_gate(row,executable):
    if executable in reference.GATES:return reference.validate_gate(row,executable)
    require(GATE_FLOORS and executable==GATES[-1] and type(row) is dict and
        set(row)==set(GATE_FLOORS)|{"schema","status"} and row["schema"]==executable+"_v1" and row["status"]=="passed",
        "shallow gate schema/fields differ")
    for key,floor in GATE_FLOORS.items():require(uint(row[key],"gate."+key)>=floor,"shallow gate floor failed")
    require(row["candidates"]==row["q4"] and row["exhaustive_edges"]==10 and
        row["parallel_calls"]==4 and row["callback_failures"]==1 and row["allocation_failures"]==4,
        "shallow gate exact output/lifecycle ledger differs")

def validate_xml(path):
    tree=ET.parse(path).getroot();cases=tree.findall("testcase");names=[c.get("name") for c in cases]
    require(tree.tag=="testsuite" and tree.get("tests")=="90" and all(tree.get(key)=="0" for key in ("failures","disabled","skipped")) and
        len(cases)==len(set(names))==90 and all(type(name) is str and name.startswith("mhgp8_") for name in names) and
        set(GATES)|{"mhgp8_q4_family_gate"}<=set(names),"regression90 inventory/status differs")
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
                result.append(("measure",[str(build/"mhgp8_q4_shallow_probe"),str(n),regime,str(k),"8" if campaign=="smoke" else "32"]))
    return result

def executable_names(build,campaign):
    if campaign=="regression":
        names=sorted(p.name for p in build.glob("mhgp8_*") if p.is_file() and os.access(p,os.X_OK))
        require(names and all((build/name).resolve()==build/name for name in names),"missing or linked executable")
        return names
    return sorted([*GATES,*(["mhgp8_q4_shallow_probe"] if campaign!="gate" else [])])

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
                metrics=[flatten({key:r[key] for key in ("work","baseline_work","generation","validation","cloud_work","index_work","cover_work","digest","memory","timings")}) for r in series]
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
        else:raise InvalidReceipt("shallow reader corruption survived")
    for route,value in ((["n"],True),(["baseline_options","clip_events"],1),(["edge_ids"],[False,1]),
        (["digest","hash"],0),(["baseline_digest","hash"],0),(["validation","point_tests"],0),
        (["validation","distinct_balls"],True),(["work","geometry","cover_sites"],0),
        (["work","selection","input_sites"],0),(["work","selection","retained_ids"],True),
        (["work","selection","discarded_ids"],999999),(["work","selection","layer_input_ids"],999999),
        (["work","seed_candidates"],0),(["work","sweep","family","sites"],0),
        (["work","sweep","depth_skipped_ids"],999999),(["work","peak_live_buffer_bytes"],0),
        (["baseline_work","sweep","active_sites"],0),(["cover_work","admitted_sites"],0),
        (["index_work","nodes"],0),(["memory","id_bytes"],True),(["timings","shallow_prepare_run_sum_ms"],float("nan"))):
        row=deepcopy(base["row"]);target=row
        for key in route[:-1]:target=target[key]
        target[route[-1]]=value;reject(lambda:validate_row(row,base["command"]))
    command=base["command"].copy();command[4]=str(int(command[4])+1);reject(lambda:validate_row(base["row"],command))
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
