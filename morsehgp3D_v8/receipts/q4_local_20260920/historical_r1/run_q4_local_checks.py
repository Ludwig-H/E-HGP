#!/usr/bin/env python3
"""Real local q4 sweeps, bounded full-cover comparison and dense growth.

Explicit adapter for the frozen receipt collector: all raw commands, failures,
environment/source/binary pins and normal/-O closure remain enforced. This
runner never builds, provisions GCP, or executes a dense quadratic baseline
unless the probe is explicitly invoked outside this fixed campaign.
"""
from __future__ import annotations
import argparse
from copy import deepcopy
import json
import math
import os
from pathlib import Path
import xml.etree.ElementTree as ET

import run_q4_center_map_checks as reference
previous = reference.previous
collector = previous.collector
ROOT = previous.ROOT
from run_p0_matrix import InvalidReceipt, parse_result, require, uint
from run_q4_family_checks import counts, read_json, validate_closure

SOURCES = reference.SOURCES | frozenset({
    "morsehgp3D_v8/src/lanes/q4_local_partition.hpp", "morsehgp3D_v8/src/lanes/q4_local_partition.cpp",
    "morsehgp3D_v8/src/lanes/q4_local.hpp", "morsehgp3D_v8/src/lanes/q4_local.cpp",
    "morsehgp3D_v8/tests/q4_local_gate.cpp",
    "morsehgp3D_v8/bench/q4_local_probe.cpp", "morsehgp3D_v8/bench/run_q4_local_checks.py",
})
SCHEMA = "mhgp8_q4_local_attempt_v1"
GATES = (*reference.GATES, "mhgp8_q4_local_gate")
REGIMES = ("far", "cap", "adversarial", "dense_prefix", "dense_permuted")
ATLAS_FIELDS = ["cells_created","outside_cells","deep_cells","leaf_cells","splits","depth_stops","node_stops","small_stops","active_sites_sum","active_blocks_sum","terminal_refinements","terminal_deep_cells","max_depth","peak_fragment_bytes","peak_build_bytes","retained_bytes"]
SWEEP_FIELDS = ["seed_queries","seed_owner_tests","seed_owner_rejections","query_visits","line_tests","line_skips","leaf_queries","reference_points","reference_side_tests","active_blocks","active_sites","root_locations","clipped_events","clipped_inside","kept_events","constant_inside","constant_outside","constant_shell_ids","entries","exits","sort_comparisons","shell_sort_comparisons","group_comparisons","groups","boundary_skips","boundary_skipped_ids","depth_rejections","depth_skipped_ids","presentations","owner_tests","owner_rejections","positive_tests","positive_rejections","canonical_tests","canonical_rejections","emitted","shell_ids","groups_without_support","unexamined_after_emit","max_group","peak_buffer_bytes"]
LOCAL_EDGE_FIELDS = ["node_visits","bound_tests","point_tests","rejected_nodes","split_nodes","rejected_sites","acute_seeds","owner_tests","owner_rejections","seeds","peak_live_buffer_bytes"]
GEOMETRY_FIELDS = ["preparations","cover_node_visits","cover_range_advances","cover_disjoint_nodes","cover_splits","cover_blocks","cover_sites","cover_excluded_sites","cover_node_ids_copied","projection_points","hull_sort_comparisons","hull_orientation_tests","hull_vertices","facets","peak_retained_bytes"]
PARTITION_FIELDS = ["root_factories","child_factories","refine_factories","input_nodes","input_sites","inherited_inside_sites","node_visits","block_bound_tests","point_tests","z_splits","inside_nodes","outside_nodes","inside_sites","outside_sites","active_nodes","active_sites","budget_unexamined_nodes","budget_ambiguous_nodes","frontier_ids_copied","peak_retained_bytes"]
DOMAIN_QUERY_FIELDS = ["disk_tests","facet_tests"]
DOMAIN_FIELDS = reference.DOMAIN_FIELDS
GATE_FLOORS = {
    "checks": 1,
    "partitions": 1,
    "fragment_checks": 1,
    "active_blocks": 1,
    "active_sites": 1,
    "inactive_inside": 1,
    "inactive_outside": 1,
    "tangent_active": 1,
    "parent_checks": 1,
    "closed_roots": 0,
    "owned_roots": 0,
    "unowned_boundary_roots": 1,
    "corner_roots": 1,
    "root_cells": 0,
    "child_cells": 0,
    "exhausted_partitions": 0,
    "extreme_calls": 1,
    "clipped_events": 1,
    "retained_events": 1,
    "constant_shells": 1,
    "depth_drops": 0,
    "clip_pairs": 1,
    "atlas_reuses": 1,
    "full_sweeps": 0,
    "empty_outputs": 0,
    "rotations": 1,
    "partition_inside": 0,
    "partition_outside": 0,
    "partition_splits": 1,
    "allocation_failures": 5,
    "refinements": 1,
    "refine_inside_added": 1,
    "edge_calls": 1,
    "seed_calls": 1,
    "reference_calls": 1,
    "oracle_completions": 1,
    "oracle_sites": 1,
    "candidates": 1,
    "q4": 1,
    "max_shell": 30,
    "exhaustive_edges": 1,
    "invalid_inputs": 1,
    "callback_failures": 1,
    "parallel_calls": 4
}
FIXED = dict(schema="mhgp8_q4_local_probe_v1", status="completed",
    scope="one_edge_q4_real_local_sweeps_not_global_producer", public_status="not_claimed",
    backend="cpu_reference", profile="quantized_u16_input_only", threads=1, edge_ids=[0,1],
    timing_scope="enclosing_wall_including_validation_release_pipeline_sums_share_preparation")
PART_TIMES = ("generation_ms", "fixture_validation_ms", "cloud_ms", "index_ms", "cover_ms",
    "baseline_run_callback_ms", "baseline_validation_ms", "run_callback_ms", "validation_ms", "release_ms")
MEMORY_FIELDS = ("id_bytes", "input_capacity_bytes", "cloud_retained_bytes", "index_retained_bytes",
    "cover_retained_bytes", "baseline_output_retained_bytes", "output_retained_bytes")
GENERATION_FIELDS = ("proposals", "duplicates", "random_calls", "grid_size",
    "permutation_keys", "permutation_sort_comparisons", "auxiliary_capacity_bytes")
VALIDATION_FIELDS = ("fixture_point_tests", "supports", "owner_distance_tests", "distinct_balls",
    "point_tests", "shell_ids", "strict_interiors", "shell_capacity_bytes")
BASELINE = "full_q34_cover_collect_q4_not_q4_only_runtime"
NO_BASELINE = "none_no_quadratic_dense_reference"

def dense(regime):
    return regime.startswith("dense_")

def expected_digest():
    value = 14695981039346656037
    def word(number):
        nonlocal value
        for byte in (number & ((1<<64)-1)).to_bytes(8,"little"):
            value = ((value ^ byte)*1099511628211) & ((1<<64)-1)
    word(4); word(2)
    for coefficient in (1,-2000,-2050,-2000,3040000):
        word(coefficient); word(coefficient>>64)
    for site in (0,1,2,3): word(site)
    word(4)
    for site in (0,1,2,3): word(site)
    return dict(callbacks=1,q3=0,q4=1,support_ids_visited=4,shell_ids_visited=4,hash=value)

def structural(work, fields, nested, label):
    require(type(work) is dict and set(work) == set(fields)|set(nested), label+" fields differ")
    for key in fields: uint(work[key],label+"."+key)

def validate_generator(w,n,seeds):
    require(w["node_visits"] == w["bound_tests"]+w["point_tests"] <= 2*n-1 and
        w["bound_tests"] == w["rejected_nodes"]+w["split_nodes"] and
        w["rejected_sites"]+w["point_tests"] == n and w["seeds"] == seeds and
        w["acute_seeds"] == seeds+w["owner_rejections"] and w["acute_seeds"] <= w["point_tests"] and
        w["acute_seeds"] <= w["owner_tests"] <= 2*w["acute_seeds"], "owned seed generator ledger differs")

def validate_domain(w,n,regime,domain,geometry):
    counts(w,DOMAIN_FIELDS,"positive_domain")
    if domain == "disk":
        require(all(x==0 for x in w.values()) and
            all(geometry[x]==0 for x in ("projection_points","hull_sort_comparisons","hull_orientation_tests","hull_vertices","facets")),
            "disk-only geometry performed positive-domain preparation")
        return
    completions = n-2 if regime=="adversarial" or dense(regime) else 4
    require(w["node_visits"] == w["bound_tests"]+w["endpoint_leaf_tests"] <= 2*n-1 and
        w["node_visits"] == w["admitted_nodes"]+w["rejected_nodes"]+w["split_nodes"]+w["excluded_endpoints"] and
        w["endpoint_leaf_tests"] == w["point_tests"]+w["excluded_endpoints"] and w["excluded_endpoints"] == 2 and
        w["admitted_sites"] == completions and w["admitted_sites"]+w["rejected_sites"]+2 == n and
        w["box_merges"] == w["admitted_nodes"]-1 and w["endpoint_box_tests"]%2 == 0 and
        w["endpoint_box_tests"] <= 2*w["bound_tests"], "closed positive-domain partition differs")
    require(geometry["projection_points"] == 9 and geometry["hull_sort_comparisons"]>0 and
        geometry["hull_vertices"] == geometry["facets"] and
        (geometry["facets"]==0 or 3<=geometry["facets"]<=9), "projection/hull preparation differs")

def validate_work(row,n,seeds,population,id_bytes):
    w=row["work"]
    structural(w,LOCAL_EDGE_FIELDS,("atlas","geometry","sweep"),"work")
    validate_generator(w,n,seeds)
    a,g,s=w["atlas"],w["geometry"],w["sweep"]
    structural(a,ATLAS_FIELDS,("partition","domain"),"atlas")
    structural(g,GEOMETRY_FIELDS,("domain",),"geometry")
    counts(s,SWEEP_FIELDS,"sweep")
    p=a["partition"];counts(p,PARTITION_FIELDS,"partition")
    counts(a["domain"],DOMAIN_QUERY_FIELDS,"atlas.domain")
    validate_domain(g["domain"],n,row["regime"],row["domain"],g)
    require(g["preparations"]==1 and g["cover_node_visits"] == g["cover_disjoint_nodes"]+g["cover_splits"]+g["cover_blocks"] <= 2*n-1 and
        g["cover_sites"] == population and g["cover_sites"]+g["cover_excluded_sites"]==n and
        g["cover_blocks"] == g["cover_node_ids_copied"] > 0 and
        g["cover_range_advances"] <= row["cover_work"]["retained_ranges"] and
        g["peak_retained_bytes"] >= g["cover_blocks"]*id_bytes, "shared cover-node preparation differs")
    require(a["cells_created"]==1+4*a["splits"]<=row["node_budget"] and
        a["cells_created"]==a["outside_cells"]+a["deep_cells"]+a["leaf_cells"]+a["splits"] and
        a["leaf_cells"]+a["terminal_deep_cells"]==a["depth_stops"]+a["node_stops"]+a["small_stops"] and
        a["max_depth"]<=row["max_depth"] and a["domain"]["disk_tests"]==a["cells_created"] and
        a["domain"]["facet_tests"]<=a["cells_created"]*g["facets"], "atlas topology/domain ledger differs")
    factories=p["root_factories"]+p["child_factories"]
    require(p["root_factories"]==1 and factories==a["cells_created"]-a["outside_cells"] and
        p["node_visits"]==p["block_bound_tests"]+p["point_tests"]+p["budget_unexamined_nodes"] and
        p["node_visits"]==p["inside_nodes"]+p["outside_nodes"]+p["active_nodes"]+p["z_splits"] and
        p["refine_factories"]==a["terminal_refinements"]<=a["leaf_cells"]+a["terminal_deep_cells"] and
        a["terminal_deep_cells"]<=a["deep_cells"] and
        p["block_bound_tests"]+p["point_tests"]<=row["z_test_budget"]*factories+(2*n-1)*p["refine_factories"] and
        p["input_sites"]==p["inside_sites"]+p["outside_sites"]+p["active_sites"] and
        p["frontier_ids_copied"]==p["active_nodes"] and
        p["budget_unexamined_nodes"]+p["budget_ambiguous_nodes"]<=p["active_nodes"] and
        p["inside_nodes"]<=p["inside_sites"] and p["outside_nodes"]<=p["outside_sites"] and
        p["active_nodes"]<=p["active_sites"], "exact-fragment classification/frontier ledger differs")
    require(2*a["leaf_cells"]<=a["active_sites_sum"]<=population*a["leaf_cells"] and
        a["active_blocks_sum"]<=a["active_sites_sum"] and a["active_sites_sum"]<=p["active_sites"] and
        a["active_blocks_sum"]<=p["active_nodes"], "retained leaf incidence accounting differs")
    require(s["seed_queries"]==seeds and s["seed_owner_tests"]==2*seeds and s["seed_owner_rejections"]==0 and
        seeds<=s["query_visits"] and s["leaf_queries"]<=s["line_tests"]-s["line_skips"]<=s["query_visits"] and
        s["line_skips"]<=s["line_tests"]<=s["query_visits"] and
        3*s["leaf_queries"]<=s["constant_shell_ids"] and
        s["leaf_queries"]<=s["active_blocks"]<=s["active_sites"] and
        s["active_sites"]<=seeds*a["active_sites_sum"], "real seed/leaf incidence ledger differs")
    require(s["active_sites"]==sum(s[key] for key in
        ("constant_inside","constant_outside","constant_shell_ids","clipped_events","kept_events")) and
        s["kept_events"]==s["entries"]+s["exits"] and s["clipped_inside"]<=s["clipped_events"], "local site/event partition differs")
    if row["clip_events"]:
        require(s["reference_points"]==s["leaf_queries"] and
            s["reference_points"]<=s["reference_side_tests"]<=4*s["reference_points"] and
            s["root_locations"]==s["kept_events"]+s["clipped_events"]+s["groups"], "exact clipping/reference work differs")
    else:
        require(s["root_locations"]==s["groups"] and all(s[key]==0 for key in
            ("reference_points","reference_side_tests","clipped_events","clipped_inside")), "unclipped path performed clipping")
    require(s["groups"]<=s["kept_events"] and 0<=s["kept_events"]-s["group_comparisons"]<=s["leaf_queries"] and
        s["max_group"]*s["groups"]>=s["kept_events"] and
        (s["kept_events"]==0 or s["max_group"]>0), "local event grouping differs")
    require(s["groups"]==s["boundary_skips"]+s["depth_rejections"]+s["groups_without_support"]+s["emitted"] and
        s["kept_events"]==s["presentations"]+s["unexamined_after_emit"]+s["boundary_skipped_ids"]+s["depth_skipped_ids"] and
        s["presentations"]==s["owner_rejections"]+s["positive_tests"] and
        s["positive_tests"]==s["positive_rejections"]+s["canonical_tests"] and
        s["canonical_tests"]==s["canonical_rejections"]+s["emitted"] and
        s["presentations"]<=s["owner_tests"]<=5*s["presentations"] and
        s["emitted"]==row["digest"]["q4"] and s["shell_ids"]==row["digest"]["shell_ids_visited"],
        "local group/presentation/output partition differs")
    require(a["peak_fragment_bytes"]>=p["peak_retained_bytes"] and
        a["peak_build_bytes"]>=a["peak_fragment_bytes"]+g["peak_retained_bytes"] and
        a["retained_bytes"]>=a["active_blocks_sum"]*id_bytes+g["peak_retained_bytes"] and
        a["peak_build_bytes"]>=a["retained_bytes"] and
        s["peak_buffer_bytes"]>=s["max_group"]*id_bytes and
        w["peak_live_buffer_bytes"]==max(a["peak_build_bytes"],a["retained_bytes"]+s["peak_buffer_bytes"]),
        "actual atlas/build/sweep capacity coupling differs")

def validate_baseline(row,n,seeds,population,id_bytes,enabled):
    skipped=uint(row["baseline_q3_callbacks_skipped"],"baseline_q3_callbacks_skipped")
    if not enabled:
        require(row["baseline_work"] is None and row["baseline_digest"] is None and skipped==0,
            "disabled baseline fabricated work or output")
        return
    require(row["baseline_digest"]==row["digest"],"full-cover q4 differential differs")
    b=row["baseline_work"];structural(b,previous.EDGE_FIELDS,("covered",),"baseline")
    validate_generator(b,n,seeds)
    require(all(b[key]==row["work"][key] for key in previous.EDGE_FIELDS),"local seed generator differs from reference")
    c=b["covered"];structural(c,previous.COVERED_FIELDS,("seed",),"baseline.covered")
    s=c["seed"];structural(s,previous.SEED_FIELDS,("family",),"baseline.seed")
    f=s["family"];counts(f,previous.FAMILY_FIELDS,"baseline.family")
    require(c["site_reads"]==s["q3_point_tests"]==f["sites"]==seeds*population and
        s["seed_owner_tests"]==2*seeds and s["seed_owner_rejections"]==0 and
        s["q3_depth_rejections"]+s["q3_emitted"]==seeds and s["q3_emitted"]==skipped and
        s["q4_emitted"]==row["digest"]["q4"],"full q34 baseline scan/output ledger differs")
    require(f["sites"]==sum(f[key] for key in ("entries","exits","constant_inside","constant_on","constant_outside")) and
        f["event_count"]==f["entries"]+f["exits"] and f["constant_on"]>=3*seeds and
        f["groups"]==f["callbacks"]<=f["event_count"] and
        0<=f["event_count"]-f["group_comparisons"]<=seeds and f["max_group"]*f["groups"]>=f["event_count"],
        "full-cover family event ledger differs")
    require(s["q4_presentations"]+s["q4_depth_skipped_ids"]+s["q4_unexamined_after_emit"]==f["event_count"] and
        s["q4_depth_rejected_groups"]+s["q4_groups_without_support"]+s["q4_emitted"]==f["groups"] and
        s["q4_presentations"]==s["q4_owner_rejections"]+s["q4_positive_tests"] and
        s["q4_positive_tests"]==s["q4_positive_rejections"]+s["q4_seed_tests"] and
        s["q4_seed_tests"]==s["q4_seed_rejections"]+s["q4_emitted"] and
        s["q4_presentations"]<=s["q4_owner_tests"]<=5*s["q4_presentations"],"baseline presentation cascade differs")
    capacities=(s["q3_shell_capacity_bytes"],f["retained_capacity_bytes"])
    require(f["retained_capacity_bytes"]>=2*population*id_bytes and
        max(capacities)<=c["peak_buffer_bytes"]<=sum(capacities),"baseline capacities missing")

def validate_row(row,command):
    require(type(row) is dict and len(command)==11,"local probe command arity")
    n,regime,k,domain=int(command[1]),command[2],int(command[3]),command[4]
    depth,nodes,ztests,leaf,clip=map(int,command[5:10]);baseline=command[10]
    require(n>=8 and regime in REGIMES and k in (5,10) and domain in ("disk","positive") and
        0<=depth<=44 and nodes>0 and ztests>=0 and leaf>=0 and clip in (0,1) and baseline in ("cover","none") and
        (regime!="cap" or n<=35307) and (regime!="adversarial" or n<=514) and
        (not dense(regime) or n<=32720),"fixture/option domain differs")
    extra={"n","regime","kmax","domain","max_depth","node_budget","z_test_budget","leaf_sites","clip_events",
        "baseline","input_hash","expected_seeds","cover_sites","generation","validation","baseline_q3_callbacks_skipped",
        "baseline_work","baseline_digest","work","digest","cloud_work","index_work","cover_work","memory","timings"}
    require(set(row)==set(FIXED)|extra and all(type(row[key]) is type(value) and row[key]==value for key,value in FIXED.items()) and
        all(type(x) is int for x in row["edge_ids"]),"local schema/scope differs")
    for key,value in dict(n=n,kmax=k,max_depth=depth,node_budget=nodes,z_test_budget=ztests,leaf_sites=leaf).items():
        require(type(row[key]) is int and row[key]==value,"numeric command/result differs")
    require(row["regime"]==regime and row["domain"]==domain and type(row["clip_events"]) is bool and
        row["clip_events"]==(clip==1) and row["baseline"]==(BASELINE if baseline=="cover" else NO_BASELINE),"command/result scope differs")
    uint(row["input_hash"],"input_hash")
    seeds=n-2 if regime=="adversarial" or dense(regime) else 2
    population=6 if regime=="far" else n
    require(uint(row["expected_seeds"],"expected_seeds")==seeds and uint(row["cover_sites"],"cover_sites")==population,
        "fixture ownership/population differs")
    g=row["generation"];counts(g,GENERATION_FIELDS,"generation")
    require(g["proposals"]==n-(2 if regime=="adversarial" or dense(regime) else 6)+g["duplicates"] and
        g["random_calls"]==(3*g["proposals"] if regime=="far" else 0) and
        (regime=="far" or g["duplicates"]==0),"generation ledger differs")
    if dense(regime):
        require(g["grid_size"]==32718 and g["permutation_keys"]==(32718 if regime=="dense_permuted" else 0),"dense recipe changed")
        require((g["permutation_sort_comparisons"]>0)==(regime=="dense_permuted") and
            (g["auxiliary_capacity_bytes"]>0)==(regime=="dense_permuted"),"dense permutation preparation omitted")
    else:
        require(all(g[key]==0 for key in ("grid_size","permutation_keys","permutation_sort_comparisons","auxiliary_capacity_bytes")),
            "non-dense fixture performed dense generation")
    d=row["digest"];counts(d,previous.DIGEST_FIELDS,"digest")
    require(d["callbacks"]==d["q4"] and d["q3"]==0 and d["support_ids_visited"]==4*d["q4"] and
        d["shell_ids_visited"]>=d["support_ids_visited"],"q4-only output ledger differs")
    if regime in ("far","cap"):require(d==expected_digest(),"closed-form complete q4 fixture differs")
    if regime=="adversarial":require(d["q4"]>0,"bounded adverse output is vacuous")
    m=row["memory"];structural(m,MEMORY_FIELDS,("scope",),"memory")
    require(m["scope"]=="dynamic_capacities_not_RSS_fixed_objects_or_judge_map_nodes" and m["id_bytes"] in (4,8) and
        min(m["input_capacity_bytes"],m["cloud_retained_bytes"])>=6*n and m["index_retained_bytes"]>=n*m["id_bytes"] and
        m["output_retained_bytes"]>=d["shell_ids_visited"]*m["id_bytes"],"memory scope/capacity differs")
    if baseline=="cover":require(m["baseline_output_retained_bytes"]>=d["shell_ids_visited"]*m["id_bytes"],"baseline output capacity missing")
    else:require(m["baseline_output_retained_bytes"]==0,"disabled baseline allocated output")
    if regime=="dense_permuted":require(g["auxiliary_capacity_bytes"]>=32718*(8+m["id_bytes"]),"permutation capacity omitted")
    v=row["validation"];structural(v,VALIDATION_FIELDS,("method",),"validation")
    require(v["method"]=="independent_rational_support_and_full_census_per_published_ball_not_large_completeness" and
        v["fixture_point_tests"]==n and v["supports"]==d["q4"] and v["owner_distance_tests"]==6*v["supports"] and
        v["distinct_balls"]<=v["supports"] and (v["distinct_balls"]>0)==(v["supports"]>0) and
        v["point_tests"]==n*v["distinct_balls"] and v["shell_ids"]>=4*v["distinct_balls"] and
        v["strict_interiors"]<=(k-3)*v["distinct_balls"] and
        v["shell_capacity_bytes"]>=v["shell_ids"]*m["id_bytes"],"independent published-ball census work differs")
    c=row["cloud_work"];counts(c,previous.CLOUD_FIELDS,"cloud")
    require(c["coordinate_copies"]==c["validation_points"]==c["range_tree_leaf_visits"]==n and
        c["uniqueness_adjacent_tests"]==c["range_tree_merges"]==n-1 and c["range_tree_nodes"]==2*n-1,"shared cloud work differs")
    ix=row["index_work"];counts(ix,previous.INDEX_FIELDS,"index")
    require(ix["nodes"]==ix["escape_links"]==2*n-1 and ix["point_visits"]>=n and ix["max_depth"]>0,"shared index preparation differs")
    c=row["cover_work"];counts(c,previous.COVER_FIELDS,"cover")
    require(c["node_visits"]==c["bound_tests"]+c["point_tests"]<=2*n-1 and
        c["node_visits"]==c["admitted_nodes"]+c["rejected_nodes"]+c["split_nodes"] and
        c["admitted_sites"]==population and c["admitted_sites"]+c["rejected_sites"]==n and
        c["retained_ranges"]+c["merged_ranges"]==c["admitted_nodes"] and c["retained_ranges"]>0 and
        m["cover_retained_bytes"]>=2*m["id_bytes"]*c["retained_ranges"],"cover partition differs")
    validate_work(row,n,seeds,population,m["id_bytes"])
    validate_baseline(row,n,seeds,population,m["id_bytes"],baseline=="cover")
    t=row["timings"]
    require(type(t) is dict and set(t)==set(PART_TIMES)|{"baseline_prepare_run_sum_ms","local_prepare_run_sum_ms","total_ms"} and
        all(type(value) in (int,float) and math.isfinite(value) and value>=0 for value in t.values()),"invalid timings")
    common=("generation_ms","cloud_ms","index_ms","cover_ms")
    pairs=[("total_ms",PART_TIMES),("local_prepare_run_sum_ms",(*common,"run_callback_ms"))]
    if baseline=="cover":pairs.append(("baseline_prepare_run_sum_ms",(*common,"baseline_run_callback_ms")))
    else:require(t["baseline_prepare_run_sum_ms"]==t["baseline_run_callback_ms"]==0,"disabled baseline has a measured run")
    for total,parts in pairs:
        require(math.isclose(t[total],sum(t[key] for key in parts),rel_tol=1e-12,abs_tol=1e-6),"inclusive timing partition differs")

def validate_gate(row,executable):
    if executable in reference.GATES:return reference.validate_gate(row,executable)
    require(executable==GATES[-1] and type(row) is dict and set(row)==set(GATE_FLOORS)|{"schema","status"} and
        row["schema"]==executable+"_v1" and row["status"]=="passed","local gate schema/fields differ")
    for key,floor in GATE_FLOORS.items():require(uint(row[key],"gate."+key)>=floor,"local gate floor failed")
    require(row["candidates"]==row["q4"] and row["parallel_calls"]==4 and row["callback_failures"]==1 and
        row["allocation_failures"]==5,"local gate output/lifecycle ledger differs")

def validate_xml(path):
    tree=ET.parse(path).getroot();cases=tree.findall("testcase");names=[c.get("name") for c in cases]
    require(tree.tag=="testsuite" and tree.get("tests")=="89" and
        all(tree.get(key)=="0" for key in ("failures","disabled","skipped")) and len(cases)==len(set(names))==89 and
        all(type(name) is str and name.startswith("mhgp8_") for name in names) and
        set(GATES)|{"mhgp8_q4_family_gate"}<=set(names),"regression89 inventory/status differs")
    require(all(c.get("status")=="run" and all(c.find(tag) is None for tag in ("failure","error","skipped")) for c in cases),
        "regression testcase failed/skipped")
    return sorted(names)

def plan(build,campaign,output,ctest=None):
    if campaign=="regression":return [("ctest",[ctest,"--test-dir",str(build),"--output-on-failure","--parallel","2","--output-junit",str(output/"result.xml")])]
    result=[("gate",[str(build/name),"--selftest"]) for name in GATES]
    for regime in REGIMES:
        sizes=() if campaign=="gate" else (32,) if campaign=="smoke" else ((32,64,128,256) if regime=="adversarial" else (8000,16000,32000))
        for k in (5,10):
            for clip in ((0,1) if campaign=="smoke" else (1,)):
                for n in sizes:
                    baseline="none" if dense(regime) and campaign=="scale" else "cover"
                    result.append(("measure",[str(build/"mhgp8_q4_local_probe"),str(n),regime,str(k),"positive","7","4096","512",
                        "8" if campaign=="smoke" else "32",str(clip),baseline]))
    return result

def executable_names(build,campaign):
    if campaign=="regression":
        names=sorted(p.name for p in build.glob("mhgp8_*") if p.is_file() and os.access(p,os.X_OK))
        require(names and all((build/name).resolve()==build/name for name in names),"missing or linked executable")
        return names
    return sorted([*GATES, *(["mhgp8_q4_local_probe"] if campaign!="gate" else [])])

def install_adapters():
    collector.SOURCES,collector.SCHEMA,collector.GATES=SOURCES,SCHEMA,GATES
    collector.FAMILIES=()
    collector.plan,collector.executable_names=plan,executable_names
    collector.validate_row,collector.validate_gate,collector.validate_xml=validate_row,validate_gate,validate_xml

def read(path,check_live=False):
    install_adapters();summary=previous.BASE_READ(path,check_live)
    rows=[r["row"] for file in sorted(path.glob("record_*.json")) if (r:=read_json(file))["kind"]=="measure"]
    clips=(False,True) if summary["campaign"]=="smoke" else (True,)
    for regime in REGIMES:
        for n in sorted({r["n"] for r in rows if r["regime"]==regime}):
            group=[r for r in rows if (r["regime"],r["n"])==(regime,n)]
            require(len(group)==2*len(clips) and {(r["kmax"],r["clip_events"]) for r in group}=={(k,c) for k in (5,10) for c in clips},
                "missing K/clipping comparison")
            require(all(all(r[key]==group[0][key] for key in ("input_hash","generation","cloud_work","index_work","cover_work")) and
                r["work"]["geometry"]==group[0]["work"]["geometry"] for r in group),"immutable preparation changed")
            for k in (5,10):
                pair=[r for r in group if r["kmax"]==k]
                require(all(r["digest"]==pair[0]["digest"] and r["work"]["atlas"]==pair[0]["work"]["atlas"] and
                    r["baseline_work"]==pair[0]["baseline_work"] for r in pair),"clipping changed exact outputs, atlas or reference")
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
    summary.update(scope=FIXED["scope"],growth=growth,dense_sweeps_really_executed=True,
        dense_large_quadratic_baseline_executed=False,validation_proves_emission_validity_not_dense_completeness=True,
        reference_runtime_includes_q3=True,pipeline_sums_are_not_independent_wall_measurements=True)
    return summary

def selftest(path):
    summary=read(path);records=[read_json(file) for file in sorted(path.glob("record_*.json"))]
    base=next(r for r in records if r["kind"]=="measure" and r["row"]["regime"]=="far")
    mutations=0
    def reject(action):
        nonlocal mutations
        try:action()
        except InvalidReceipt:mutations+=1
        else:raise InvalidReceipt("local reader corruption survived")
    for route,value in ((["n"],True),(["clip_events"],1),(["edge_ids"],[False,1]),(["digest","hash"],0),
        (["digest","shell_ids_visited"],0),(["validation","point_tests"],0),(["validation","distinct_balls"],True),
        (["work","geometry","cover_sites"],0),(["work","atlas","partition","input_sites"],0),
        (["work","atlas","cells_created"],True),(["work","sweep","active_sites"],0),
        (["work","sweep","kept_events"],999999),(["work","sweep","boundary_skipped_ids"],999999),
        (["work","sweep","depth_skipped_ids"],999999),(["work","peak_live_buffer_bytes"],0),
        (["cover_work","admitted_sites"],0),(["index_work","nodes"],0),(["memory","id_bytes"],True),
        (["timings","local_prepare_run_sum_ms"],float("nan"))):
        row=deepcopy(base["row"]);target=row
        for key in route[:-1]:target=target[key]
        target[route[-1]]=value;reject(lambda:validate_row(row,base["command"]))
    command=base["command"].copy();command[10]="none";reject(lambda:validate_row(base["row"],command))
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
