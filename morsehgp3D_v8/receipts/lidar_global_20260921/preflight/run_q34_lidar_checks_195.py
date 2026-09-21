#!/usr/bin/env python3
"""Constructor LiDAR edge qualification; audit files supply INPUTS, not verdicts.

Three real q4 engines, original IDs, complete normalized payloads and new
constructor census of every published ball (C++), plus independent Python
recensus of the selected target ball at every prefix. No GCP or build step.
Capture failures and interrupted commands remain immutable, separate attempts.
"""
from __future__ import annotations
import argparse
import base64
from copy import deepcopy
from fractions import Fraction
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import time
import xml.etree.ElementTree as ET

import run_q4_window_checks as window
shallow = window.reference
local = shallow.reference
previous = shallow.previous
from run_q34_seed_checks import environment_record
from run_q4_family_checks import counts, digest, pins, read_json, validate_closure
from run_p0_matrix import InvalidReceipt, invoke, on_signal, parse_result, require, uint, utc_stamp, write_json

ROOT = window.ROOT
SOURCES = window.SOURCES | frozenset({
    "morsehgp3D_v8/src/pipeline/wspd_q34.hpp",
    "morsehgp3D_v8/src/pipeline/wspd_q34.cpp",
    "morsehgp3D_v8/tests/wspd_q34_gate.cpp",
    "morsehgp3D_v8/bench/wspd_q34_probe.cpp",
    "morsehgp3D_v8/bench/q4_lidar_probe.cpp",
    "morsehgp3D_v8/bench/run_q34_lidar_checks.py",
})
SCHEMA = "mhgp8_q34_lidar_attempt_v1"
FIXTURES = "morsehgp3D_v8/audits/q4_kernel_composition_20260920/POSITIVE_FIXTURES.json"
PREPARED = "morsehgp3D_v8/audits/lidar08_20260914/prepared"
ORDERS = ("28,29,30", "30,29,28")
EDGE_CAMPAIGNS = ("edge_pilot12", "edge_pilot36", "edge_scale")
GATES = (*window.GATES, "mhgp8_wspd_q34_gate")
structural = local.structural
SELECTION_FIELDS, SWEEP_FIELDS, EDGE_FIELDS = shallow.SELECTION_FIELDS, shallow.SWEEP_FIELDS, shallow.EDGE_FIELDS
validate_generator = local.validate_generator
MASK64 = (1 << 64)-1

def word(h, value):
    for byte in (value & MASK64).to_bytes(8, "little"):
        h = ((h ^ byte) * 1099511628211) & MASK64
    return h

def input_hash(points):
    h = word(14695981039346656037, len(points))
    for point in points:
        for coordinate in point:
            h = word(h, coordinate)
    return h

def safe_path(name):
    require(type(name) is str and not Path(name).is_absolute() and ".." not in Path(name).parts,
            "unsafe relative input path")
    p = ROOT / name
    require(p.resolve().is_relative_to(ROOT), "input escapes repository")
    return p

def solve(matrix, rhs):
    size = len(rhs)
    a = [[Fraction(v) for v in row] + [Fraction(rhs[i])] for i, row in enumerate(matrix)]
    for col in range(size):
        pivot = next((i for i in range(col, size) if a[i][col]), None)
        require(pivot is not None, "target support is singular")
        a[col], a[pivot] = a[pivot], a[col]
        divisor = a[col][col]
        a[col] = [v/divisor for v in a[col]]
        for i in range(size):
            if i != col:
                factor = a[i][col]
                a[i] = [v-factor*w for v, w in zip(a[i], a[col], strict=True)]
    return [row[-1] for row in a]

def target_key(support):
    p0 = support[0]
    matrix = [[2*(p[j]-p0[j]) for j in range(3)] for p in support[1:]]
    rhs = [sum(v*v for v in p)-sum(v*v for v in p0) for p in support[1:]]
    centre = solve(matrix, rhs)
    weights = solve([[support[i][j]-p0[j] for i in range(1,4)] for j in range(3)],
                    [centre[j]-p0[j] for j in range(3)])
    require(all(w > 0 for w in [1-sum(weights), *weights]), "target ball is not strictly positive")
    constant = 2*sum(centre[j]*p0[j] for j in range(3))-sum(v*v for v in p0)
    coefficients = [Fraction(1), *[-2*v for v in centre], constant]
    denominator = math.lcm(*(v.denominator for v in coefficients))
    integers = [int(v*denominator) for v in coefficients]
    common = math.gcd(*integers)
    return [v//common for v in integers]

def census(key, points):
    inside, shell = [], []
    for i, (x,y,z) in enumerate(points):
        power = key[0]*(x*x+y*y+z*z)+key[1]*x+key[2]*y+key[3]*z+key[4]
        if power < 0: inside.append(i)
        elif power == 0: shell.append(i)
    return inside, shell

def prepare_inputs(campaign):
    if campaign not in EDGE_CAMPAIGNS:
        return dict(source="none", files={}, datasets=[], targets=[], census_point_tests=0,
                    rational_supports=0, prefix_byte_tests=0)
    fixture_file = safe_path(FIXTURES)
    document = read_json(fixture_file)
    fixtures = document["fixtures"]
    require(type(fixtures) is list and len(fixtures)==9, "expected nine independent INPUT fixtures")
    require([f["scan"] for f in fixtures]==[0,0,0,100,100,100,200,200,200], "fixture scan inventory differs")
    selected = [0,3,6] if campaign=="edge_pilot12" else list(range(9))
    sizes = (8000,16000,32000,50000) if campaign=="edge_scale" else (8000,)
    names = {FIXTURES}
    datasets, targets = [], []
    loaded = {}
    prefix_tests = 0
    for scan in (0,100,200):
        metadata_name=f"{PREPARED}/single_{scan:06}/METADATA.json"
        metadata=read_json(safe_path(metadata_name));names.add(metadata_name)
        require(metadata["frames"]==[scan] and metadata["frame"]=="LiDAR_scan_000000" and
            metadata["sampling"]==dict(hash="blake2b_128(seed_bytes + packed_u16le_xyz)", order="ascending_priority",
                seed_u32_le=3, targets=[8000,16000,32000,50000], tie_break="lexicographic_xyz") and
            metadata["quantization"]==dict(all_axes_same_grid=True,clipping_jitter_adaptation=False,
                formula="floor(50*x + 32768 + 0.5)",invalid_input_policy="reject_entire_preparation",step_m=0.02),
            "prepared quantization/sampling contract differs")
        last=b""
        for n in sizes:
            entry=next((e for e in metadata["samples"] if e["n"]==n),None)
            name=f"{PREPARED}/single_{scan:06}/n{n}.u16le"
            require(entry is not None and entry["requested_n"]==n and entry["bytes"]==6*n and
                entry["status"]=="prepared" and entry["path"]==f"single_{scan:06}/n{n}.u16le", "metadata sample differs")
            raw=safe_path(name).read_bytes();names.add(name)
            require(len(raw)==6*n and hashlib.sha256(raw).hexdigest()==entry["sha256"], "prepared bytes/hash differ")
            require(raw.startswith(last), "prepared samples are not nested original-ID prefixes")
            prefix_tests+=len(last);last=raw
            points=list(struct.iter_unpack("<HHH",raw))
            require(len(set(points))==n,"prepared input has duplicate coordinates")
            loaded[scan,n]=points
            datasets.append(dict(scan=scan,n=n,path=name,sha256=entry["sha256"],fnv1a64=input_hash(points)))
    for index in selected:
        fixture=fixtures[index]
        scan=fixture["scan"];edge=fixture["edge_ids"];support_ids=fixture["support_ids"]
        require(type(edge) is list and len(edge)==2 and all(type(i) is int for i in edge) and
            edge==sorted(set(edge)) and type(support_ids) is list and len(support_ids)==4 and
            all(type(i) is int and 0<=i<8000 for i in support_ids) and support_ids==sorted(set(support_ids)) and
            set(edge)<=set(support_ids),"fixture original IDs differ")
        base=loaded[scan,8000]
        support=[list(base[i]) for i in support_ids]
        require(support==fixture["support_coordinates"] and [list(base[i]) for i in edge]==fixture["edge_coordinates"],
                "fixture coordinates differ from original prepared IDs")
        base_name=f"{PREPARED}/single_{scan:06}/n8000.u16le"
        require(fixture["input_sha256"]=={base_name:digest(safe_path(base_name))}, "fixture input pin differs")
        key=target_key(support)
        require(key==fixture["coefficients"],"independent rational target key differs")
        length=sum((base[edge[0]][j]-base[edge[1]][j])**2 for j in range(3))
        for i in range(4):
            for j in range(i+1,4):
                distance=sum((support[i][axis]-support[j][axis])**2 for axis in range(3))
                require(distance<length or (distance==length and edge<=[support_ids[i],support_ids[j]]),
                        "target owner edge differs under original-ID ties")
        for n in sizes:
            inside,shell=census(key,loaded[scan,n])
            if n==8000:
                require(len(inside)==fixture["depth"] and inside==fixture["interior_ids"] and shell==fixture["shell_ids"],
                        "constructor target census differs from input fixture")
            targets.append(dict(fixture=index,scan=scan,n=n,edge_ids=edge,support_ids=support_ids,
                coefficients=[str(v) for v in key],depth=len(inside),shell=shell,
                expected_K5=len(inside)<3,expected_K10=len(inside)<8))
    return dict(source="explicit_read_only_audit_INPUTS_constructor_rational_key_and_recensus",
        files=pins(names),datasets=datasets,targets=targets,
        census_point_tests=sum(t["n"] for t in targets),rational_supports=len(selected),prefix_byte_tests=prefix_tests)

def validate_domain(w,n,regime,domain,geometry):
    if domain=="disk":
        return local.validate_domain(w,n,regime,domain,geometry)
    counts(w,local.DOMAIN_FIELDS,"positive_domain")
    require(w["node_visits"]==w["bound_tests"]+w["endpoint_leaf_tests"]<=2*n-1 and
        w["node_visits"]==w["admitted_nodes"]+w["rejected_nodes"]+w["split_nodes"]+w["excluded_endpoints"] and
        w["endpoint_leaf_tests"]==w["point_tests"]+w["excluded_endpoints"] and w["excluded_endpoints"]==2 and
        w["admitted_sites"]+w["rejected_sites"]+2==n and w["box_merges"]==max(0,w["admitted_nodes"]-1) and
        w["endpoint_box_tests"]%2==0 and w["endpoint_box_tests"]<=2*w["bound_tests"],
        "general positive-domain partition differs")
    require(geometry["projection_points"] in (0,9) and geometry["hull_vertices"]==geometry["facets"] and
        (geometry["facets"]==0 or 3<=geometry["facets"]<=9), "general projection/hull preparation differs")

# Explicit general-input port of constructor28/29 work ledgers; no synthetic seed-count assumption.
def validate_local_work(row,n,seeds,population,id_bytes):
    w=row["work"]
    structural(w,local.LOCAL_EDGE_FIELDS,("atlas","geometry","sweep"),"work")
    validate_generator(w,n,seeds)
    a,g,s=w["atlas"],w["geometry"],w["sweep"]
    structural(a,local.ATLAS_FIELDS,("partition","domain"),"atlas")
    structural(g,local.GEOMETRY_FIELDS,("domain",),"geometry")
    counts(s,local.SWEEP_FIELDS,"sweep")
    p=a["partition"];counts(p,local.PARTITION_FIELDS,"partition")
    counts(a["domain"],local.DOMAIN_QUERY_FIELDS,"atlas.domain")
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

def validate_shallow_work(row,n,initial_seeds,population,id_bytes):
    w=row["work"];structural(w,EDGE_FIELDS,("geometry","selection","sweep"),"work")
    g,p,s=w["geometry"],w["selection"],w["sweep"]
    structural(g,local.GEOMETRY_FIELDS,("domain",),"geometry")
    counts(p,SELECTION_FIELDS,"selection")
    structural(s,SWEEP_FIELDS,("family",),"sweep")
    f=s["family"];counts(f,previous.FAMILY_FIELDS,"family")
    local.validate_domain(g["domain"],n,row["regime"],"disk",g)
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

FIXED = dict(schema="mhgp8_q4_lidar_probe_v1",status="passed",
    scope="one_original_edge_q4_three_actual_product_apis_not_global_producer",
    phase="exploration_v8_hors_registre",backend="cpu_reference",profile="quantized_u16_input_only",
    mode="implementation_v8_p0",public_status="not_claimed",completeness_large_cloud_claimed=False,
    input_reindexed=False,input_format="u16le_xyz_no_header")
MEMORY_SCOPE = "Separate capacities, not RSS; earlier outputs coexist with later lanes; product peaks exclude shared input/cloud/index/cover, outputs, judge map and allocator metadata"
TIMING_SCOPE = "Actual serial order is recorded; each edge interval includes its OWN geometry/atlas or shallow selection, generation, census, callbacks and private destruction. Shared cloud/index/cover is timed once; inclusive sums reuse that interval and are NOT three independent walls. Judge scans n once per distinct emitted ball. JSON and final shared/output destruction excluded"
PART_TIMES = ["read_hash","cloud","index","cover","three_lanes_wall","sort_digest_exact_compare","independent_judge"]
LANES = ("local28","shallow29","window30")

def validate_records(records,n,k,edge):
    require(type(records) is list,"records must be a full list")
    h=14695981039346656037; shell_count=0; distinct={}
    last=None
    for rec in records:
        require(type(rec) is dict and set(rec)=={"arity","depth","support","coefficients","shell"} and
            type(rec["arity"]) is int and rec["arity"]==4 and uint(rec["depth"],"record.depth")<k-2,
            "record arity/depth differs")
        support,shell,key=rec["support"],rec["shell"],rec["coefficients"]
        for ids in (support,shell):
            require(type(ids) is list and all(type(i) is int and 0<=i<n for i in ids) and ids==sorted(set(ids)),
                    "record IDs are unordered, duplicated or invalid")
        require(len(support)==4 and set(edge)<=set(support)<=set(shell),"record edge/support/shell differs")
        require(type(key) is list and len(key)==5 and all(type(v) is str for v in key),"exact key representation differs")
        integers=[]
        for value in key:
            try: integer=int(value)
            except (ValueError,TypeError):raise InvalidReceipt("invalid integer coefficient")
            require(str(integer)==value and -(1<<127)<=integer<(1<<127),"noncanonical or oversized integer coefficient")
            integers.append(integer)
        require(integers[0]>0 and math.gcd(*integers)==1,"nonprimitive ball key")
        ordering=(4,tuple(support),tuple(integers),rec["depth"],tuple(shell))
        require(last is None or last<ordering,"duplicate or unordered record")
        last=ordering
        h=word(word(h,4),rec["depth"])
        for v in integers:
            bits=v% (1<<128);h=word(word(h,bits&MASK64),bits>>64)
        for i in support:h=word(h,i)
        h=word(h,len(shell))
        for i in shell:h=word(h,i)
        shell_count+=len(shell)
        canonical=(rec["depth"],tuple(shell))
        require(tuple(key) not in distinct or distinct[tuple(key)]==canonical,"same ball census differs")
        distinct[tuple(key)]=canonical
    expected=dict(callbacks=len(records),q3=0,q4=len(records),support_ids_visited=4*len(records),
                  shell_ids_visited=shell_count,hash=h)
    return expected,distinct

def validate_row(row,command,inputs):
    require(type(row) is dict and len(command)==6 and Path(command[0]).name=="mhgp8_q4_lidar_probe",
            "LiDAR probe command arity/name differs")
    file=str(Path(command[1]).relative_to(ROOT))
    datasets=[d for d in inputs["datasets"] if d["path"]==file]
    require(len(datasets)==1,"command uses unpinned input")
    dataset=datasets[0];n=dataset["n"];k=int(command[4]);order=[int(v) for v in command[5].split(",")]
    edge=sorted([int(command[2]),int(command[3])])
    require(k in (5,10) and command[5] in ORDERS and edge[0]<edge[1],"unsupported campaign options")
    target=[t for t in inputs["targets"] if t["scan"]==dataset["scan"] and t["n"]==n and t["edge_ids"]==edge]
    require(len(target)==1,"command edge lacks independent target")
    target=target[0]
    keys={"edge_ids","execution_order","n","kmax","input_fnv1a64_u64le_n_xyz","cover_sites","local_options",
          "cloud_work","index_work","cover_work","local28","shallow29","window30","comparison","judge","records",
          "derived_work","memory","timings_ms","timing_scope"}
    require(set(row)==set(FIXED)|keys and all(type(row[key]) is type(v) and row[key]==v for key,v in FIXED.items()),
            "LiDAR scope/schema differs")
    require(type(row["n"]) is int and row["n"]==n and type(row["kmax"]) is int and row["kmax"]==k and
        row["edge_ids"]==edge and all(type(i) is int for i in row["edge_ids"]) and
        row["execution_order"]==order and all(type(i) is int for i in row["execution_order"]) and
        uint(row["input_fnv1a64_u64le_n_xyz"],"input hash")==dataset["fnv1a64"],"input/command/result differs")
    population=uint(row["cover_sites"],"cover_sites")
    require(2<=population<=n and row["local_options"]==dict(domain="Positive",depth=7,node_budget=4096,
        z_test_budget=512,leaf_sites=32,clip_events=True) and
        all(type(row["local_options"][key]) is int for key in ("depth","node_budget","z_test_budget","leaf_sites")) and
        row["local_options"]["clip_events"] is True,"cover or local options differ")
    expected,distinct=validate_records(row["records"],n,k,edge)
    for lane in LANES:
        bundle=row[lane]
        extras=("edge","geometry","atlas","sweep","output") if lane=="local28" else (
            ("edge","geometry","selection","sweep","output") if lane=="shallow29" else
            ("edge","geometry","selection","sweep","window","output"))
        require(type(bundle) is dict and set(bundle)==set(extras),"lane fields differ")
        counts(bundle["output"],previous.DIGEST_FIELDS,lane+".output")
        require(bundle["output"]==expected,"full records and engine digest differ")
    require(row["comparison"]==dict(normalized_record_sets=3,equal_pairs=2,judged_common_payloads=1) and
        all(type(v) is int for v in row["comparison"].values()),"actual three-lane comparison differs")
    actual=distinct.get(tuple(target["coefficients"]))
    require((actual is not None)==target[f"expected_K{k}"],"target ball retained/removed against independent census differs")
    if actual is not None:require(actual==(target["depth"],tuple(target["shell"])),"target global depth/shell differs")
    judge=row["judge"]
    counts(judge,("supports","owner_distance_tests","distinct_balls","point_tests","shell_ids","strict_interiors"),"judge")
    require(judge==dict(supports=expected["q4"],owner_distance_tests=6*expected["q4"],distinct_balls=len(distinct),
        point_tests=n*len(distinct),shell_ids=sum(len(v[1]) for v in distinct.values()),
        strict_interiors=sum(v[0] for v in distinct.values())),"published-ball global judge ledger differs")
    memory=row["memory"]
    structural(memory,("id_bytes","input_point_capacity_bytes","cloud_retained_bytes","index_retained_bytes",
        "cover_retained_bytes","local_output_retained_bytes","shallow_output_retained_bytes","window_output_retained_bytes"),
        ("scope",),"memory")
    id_bytes=memory["id_bytes"]
    require(memory["scope"]==MEMORY_SCOPE and id_bytes in (4,8) and
        min(memory["input_point_capacity_bytes"],memory["cloud_retained_bytes"])>=6*n and
        memory["index_retained_bytes"]>=n*id_bytes and
        min(memory[key] for key in ("local_output_retained_bytes","shallow_output_retained_bytes",
            "window_output_retained_bytes"))>=expected["shell_ids_visited"]*id_bytes,"memory capacities/scope differ")
    c=row["cloud_work"];counts(c,previous.CLOUD_FIELDS,"cloud")
    require(c["coordinate_copies"]==c["validation_points"]==c["range_tree_leaf_visits"]==n and
        c["uniqueness_adjacent_tests"]==c["range_tree_merges"]==n-1 and c["range_tree_nodes"]==2*n-1,"cloud work differs")
    ix=row["index_work"];counts(ix,previous.INDEX_FIELDS,"index")
    require(ix["nodes"]==ix["escape_links"]==2*n-1 and ix["point_visits"]>=n and ix["max_depth"]>0,"index work differs")
    c=row["cover_work"];counts(c,previous.COVER_FIELDS,"cover")
    require(c["node_visits"]==c["bound_tests"]+c["point_tests"]<=2*n-1 and
        c["node_visits"]==c["admitted_nodes"]+c["rejected_nodes"]+c["split_nodes"] and
        c["admitted_sites"]==population and c["admitted_sites"]+c["rejected_sites"]==n and
        c["retained_ranges"]+c["merged_ranges"]==c["admitted_nodes"] and c["retained_ranges"]>0 and
        memory["cover_retained_bytes"]>=2*id_bytes*c["retained_ranges"],"cover work differs")
    a,b,w=(row[lane] for lane in LANES)
    work28={**a["edge"],"geometry":a["geometry"],"atlas":a["atlas"],"sweep":a["sweep"]}
    work29={**b["edge"],"geometry":b["geometry"],"selection":b["selection"],"sweep":b["sweep"]}
    work30={**w["edge"],"geometry":w["geometry"],"selection":w["selection"],
            "sweep":{"sweep":w["sweep"],"window":w["window"]}}
    view=dict(kmax=k,regime="lidar",domain="positive",node_budget=4096,max_depth=7,z_test_budget=512,
              clip_events=True,cover_work=c,digest=expected)
    seeds=work28["seeds"]
    validate_local_work({**view,"work":work28},n,seeds,population,id_bytes)
    validate_shallow_work({**view,"work":work29},n,seeds,population,id_bytes)
    window.validate_work({**view,"work":work30,"baseline_work":work29},n,seeds,population,id_bytes)
    f29=b["sweep"]["family"];f30=w["sweep"]["family"];p=w["window"]
    derived=dict(local28_active_sites=a["sweep"]["active_sites"],
        local28_root_order_comparisons=a["sweep"]["sort_comparisons"]+a["sweep"]["group_comparisons"],
        shallow29_first_pass_sites=f29["sites"],
        shallow29_root_order_comparisons=f29["sort_comparisons"]+f29["group_comparisons"],
        window30_first_pass_sites=f30["sites"],window30_second_pass_sites=p["second_pass_sites"],
        window30_total_pass_sites=f30["sites"]+p["second_pass_sites"],
        window30_root_order_comparisons=sum(p[key] for key in ("heap_comparisons","heap_sort_comparisons","window_comparisons"))+
            f30["sort_comparisons"]+f30["group_comparisons"])
    counts(row["derived_work"],derived,"derived_work")
    require(row["derived_work"]==derived,"derived total work differs")
    times=row["timings_ms"]
    time_fields=set(PART_TIMES)|{"total_before_json"}|{lane+suffix for lane in LANES
        for suffix in ("_prepare_edge_collect","_shared_prepare_plus_edge_sum")}
    require(type(times) is dict and set(times)==time_fields and
        all(type(v) in (int,float) and math.isfinite(v) and v>=0 for v in times.values()) and
        row["timing_scope"]==TIMING_SCOPE,"timing fields/scope differ")
    require(math.isclose(times["total_before_json"],sum(times[key] for key in PART_TIMES),abs_tol=5e-6,rel_tol=1e-12) and
        times["three_lanes_wall"]+2e-6>=sum(times[lane+"_prepare_edge_collect"] for lane in LANES),"serial wall partition differs")
    for lane in LANES:
        require(math.isclose(times[lane+"_shared_prepare_plus_edge_sum"],
            sum(times[key] for key in ("cloud","index","cover",lane+"_prepare_edge_collect")),abs_tol=2e-6,rel_tol=1e-12),
            "shared preparation plus lane sum differs")
    return dict(fixture=target["fixture"],scan=target["scan"],n=n,kmax=k,order=command[5],
                target_retained=actual is not None)

GLOBAL_GATE_FIELDS = ("checks global_calls oracle_clouds oracle_triangles oracle_tetrahedra oracle_balls positive_triangles positive_tetrahedra canonical_groups oracle_sites front_replays expanded_pairs fat_rectangles q3_only_edges q4_only_edges both_edges split_q3_only_edges split_q4_only_edges q3_front_rejections q4_front_rejections xi_tests q3_depth_rejections q3_unread_sites pure_calls sample_calls local_calls window_calls inactive_calls singleton_calls s8_calls s10_calls s12_calls strict_w3_contacts strict_w4_contacts independent_q2_cases independent_q3_cases isolated_cases extreme_calls candidates q3 q4 max_shell permutations invalid_inputs allocation_failures callback_failures parallel_calls nested_calls owner_reset_calls input_alias_checks").split()
GLOBAL_GATE_FIELDS += ("parallel_pipeline_calls parallel_w1_calls parallel_w2_calls parallel_w4_calls parallel_geometry_checks worker_ledger_checks callback_copy_checks multiworker_calls parallel_callback_failures parallel_join_checks parallel_owner_resets parallel_empty_calls").split()

def validate_gate(row,name):
    if name in window.GATES:return window.validate_gate(row,name)
    require(name==GATES[-1] and type(row) is dict and set(row)==set(GLOBAL_GATE_FIELDS)|{"schema","status"} and
        row["schema"]=="mhgp8_wspd_q34_gate_v1" and row["status"]=="PASS","global gate schema differs")
    for key in GLOBAL_GATE_FIELDS:require(uint(row[key],"gate."+key)>0,"global gate vacuity")
    require(row["candidates"]==row["q3"]+row["q4"] and row["max_shell"]>=30 and
        row["allocation_failures"]==4 and row["callback_failures"]==1 and row["parallel_calls"]==4 and
        row["nested_calls"]==row["owner_reset_calls"]==row["input_alias_checks"]==1 and
        row["parallel_callback_failures"]==row["parallel_owner_resets"]==1 and
        row["parallel_join_checks"]==row["parallel_empty_calls"]==2,"global gate lifecycle differs")

def validate_xml(path):
    root=ET.parse(path).getroot();cases=root.findall("testcase");names=[x.get("name") for x in cases]
    require(root.tag=="testsuite" and root.get("tests")=="92" and all(root.get(k)=="0" for k in ("failures","disabled","skipped")) and
        len(cases)==len(set(names))==92 and set(GATES)|{"mhgp8_q4_family_gate"}<=set(names) and
        all(type(name) is str and name.startswith("mhgp8_") for name in names),"regression92 inventory differs")
    require(all(c.get("status")=="run" and all(c.find(tag) is None for tag in ("failure","error","skipped")) for c in cases),
            "regression testcase failed/skipped")
    return sorted(names)

def input_inventory(campaign):
    if campaign not in EDGE_CAMPAIGNS:return frozenset()
    sizes=(8000,16000,32000,50000) if campaign=="edge_scale" else (8000,)
    return frozenset({FIXTURES,*[f"{PREPARED}/single_{scan:06}/METADATA.json" for scan in (0,100,200)],
        *[f"{PREPARED}/single_{scan:06}/n{n}.u16le" for scan in (0,100,200) for n in sizes]})

def plan(build,campaign,output,inputs,ctest=None):
    if campaign=="regression":
        return [("ctest",[ctest,"--test-dir",str(build),"--output-on-failure","--parallel","2","--output-junit",str(output/"result.xml")])]
    if campaign=="gate":return [("gate",[str(build/name),"--selftest"]) for name in GATES]
    require(campaign in EDGE_CAMPAIGNS,"unknown campaign")
    commands=[]
    for target in inputs["targets"]:
        dataset=next(d for d in inputs["datasets"] if (d["scan"],d["n"])==(target["scan"],target["n"]))
        for k in (5,10):
            for order in ORDERS:
                commands.append(("edge",[str(build/"mhgp8_q4_lidar_probe"),str(ROOT/dataset["path"]),
                    *(str(i) for i in target["edge_ids"]),str(k),order]))
    require(len(commands)==dict(edge_pilot12=12,edge_pilot36=36,edge_scale=144)[campaign],"edge plan size differs")
    return commands

def executable_names(build,campaign):
    if campaign=="regression":
        names=sorted(p.name for p in build.glob("mhgp8_*") if p.is_file() and os.access(p,os.X_OK))
        require(set(GATES)<=set(names),"regression executables incomplete")
    elif campaign=="gate":names=sorted(GATES)
    else:names=["mhgp8_q4_lidar_probe"]
    require(all((build/name).resolve()==build/name for name in names),"linked executable")
    return names

def validate_input_closure(m,c):
    require(m["input_sha256"]==c["input_sha256_after"]==m["inputs"]["files"],"input closure differs")
    require(set(m["input_sha256"])==input_inventory(m["campaign"]),"input inventory differs")
    for name,value in m["input_sha256"].items():
        safe_path(name)
        require(type(value) is str and len(value)==64 and all(x in "0123456789abcdef" for x in value),"invalid input SHA256")

def run(args):
    build=args.build.resolve()
    require(build.is_dir() and build.is_relative_to(ROOT/"build"),"fresh local build required")
    cache=(build/"CMakeCache.txt").read_text()
    if args.campaign=="regression":
        require("CMAKE_BUILD_TYPE:STRING=Release\n" in cache and "MHGP8_SANITIZE:BOOL=OFF\n" in cache,
                "regression requires nonsanitized Release")
    args.output.mkdir(parents=True,exist_ok=True)
    output=Path(tempfile.mkdtemp(prefix=args.campaign+"_",dir=args.output)).resolve()
    ctest=str(Path(shutil.which("ctest")).resolve()) if args.campaign=="regression" and shutil.which("ctest") else None
    require(args.campaign!="regression" or ctest is not None,"CTest unavailable")
    names=executable_names(build,args.campaign)
    files={str((build/name).relative_to(ROOT)) for name in [*names,"CMakeCache.txt","libmhgp8_p0.a"]}
    def git(*options):return subprocess.check_output(["git",*options],cwd=ROOT,text=True).strip()
    environment=dict(os.environ)
    sources_before=pins(SOURCES);artifacts_before=pins(files);inputs_before=pins(input_inventory(args.campaign))
    prepare_started=time.monotonic()
    try:
        inputs=prepare_inputs(args.campaign)
        require(inputs["files"]==inputs_before,"inputs changed during constructor recensus")
    except BaseException as cause:
        error=f"{type(cause).__name__}: {cause}"
        write_json(output/"SETUP_FAILURE.json",dict(status="failed",error=error,
            finished_utc=utc_stamp(),campaign=args.campaign,qualification=args.qualification,
            launch_command=[sys.executable,*sys.argv],source_sha256=sources_before,
            artifact_sha256=artifacts_before,input_sha256=inputs_before,
            scope="input preparation failed before any probe command; not a closed successful capture"))
        print(json.dumps(dict(path=str(output),status="failed",error=error)),flush=True)
        raise
    commands=plan(build,args.campaign,output,inputs,ctest)
    manifest=dict(schema=SCHEMA,campaign=args.campaign,qualification=args.qualification,build=str(build),
        started_utc=utc_stamp(),public_status="not_claimed",gcp_used=False,full_contract_qualified=False,
        source_sha256=sources_before,artifact_sha256=artifacts_before,input_sha256=inputs_before,
        inputs=inputs,input_validation_ms=(time.monotonic()-prepare_started)*1000,
        executable_names=names,planned_commands=commands,affinity=sorted(os.sched_getaffinity(0)),
        commit=git("rev-parse","HEAD"),branch=git("branch","--show-current"),worktree=git("status","--short"),
        launch_command=[sys.executable,*sys.argv],environment=environment_record(environment),
        compiler_cache="\n".join(line for line in cache.splitlines() if line.startswith(("CMAKE_CXX_COMPILER",
            "CMAKE_CXX_FLAGS","CMAKE_BUILD_TYPE:","CMAKE_GENERATOR:","MHGP8_SANITIZE:"))),
        sanitizer_environment={key:environment.get(key) for key in ("ASAN_OPTIONS","UBSAN_OPTIONS","TSAN_OPTIONS")},
        ctest=ctest,ctest_sha256=digest(Path(ctest)) if ctest else None)
    write_json(output/"MANIFEST.json",manifest)
    handlers={sig:signal.signal(sig,on_signal) for sig in (signal.SIGINT,signal.SIGTERM)}
    records=[];status="failed";error=None
    try:
        for i,(kind,command) in enumerate(commands):
            record=dict(kind=kind,command=command,cwd=str(ROOT),started_utc=utc_stamp(),status="failed",exit_code=None,
                stdout="",stderr="",stdout_base64="",stderr_base64="",environment=manifest["environment"])
            try:
                invoke(command,environment,ROOT,record,new_session=True)
                require(record["exit_code"]==0,"targeted command failed")
                if kind=="ctest":record["test_names"]=validate_xml(output/"result.xml")
                else:
                    row=parse_result(record["stdout"].encode())
                    if kind=="edge":record["case"]=validate_row(row,command,inputs)
                    else:validate_gate(row,Path(command[0]).name)
                    record["row"]=row
                record["status"]="passed"
            finally:
                record["finished_utc"]=utc_stamp()
                file=output/f"record_{i:04}.json";write_json(file,record)
                records.append(dict(path=file.name,sha256=digest(file)))
        status="passed"
    except BaseException as cause:
        error=f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in handlers:signal.signal(sig,signal.SIG_IGN)
        errors=[]
        def close(label,fn):
            try:return fn()
            except Exception as cause:
                errors.append(f"{label}: {type(cause).__name__}: {cause}");return None
        completion=dict(status=status,error=error,finished_utc=utc_stamp(),records=records,
            manifest_sha256=digest(output/"MANIFEST.json"),source_sha256_after=close("sources",lambda:pins(SOURCES)),
            artifact_sha256_after=close("artifacts",lambda:pins(files)),
            input_sha256_after=close("inputs",lambda:pins(input_inventory(args.campaign))),
            executable_names_after=close("executables",lambda:executable_names(build,args.campaign)),
            ctest_sha256_after=close("ctest",lambda:digest(Path(ctest))) if ctest else None,
            xml_sha256=close("xml",lambda:digest(output/"result.xml")) if (output/"result.xml").is_file() else None,
            closing_errors=errors)
        if errors or any(completion[b]!=manifest[a] for a,b in (
            ("source_sha256","source_sha256_after"),("artifact_sha256","artifact_sha256_after"),
            ("input_sha256","input_sha256_after"),("executable_names","executable_names_after"),
            ("ctest_sha256","ctest_sha256_after"))):
            completion["status"]="failed";completion["error"]=error or "capture closure changed"
        write_json(output/"COMPLETION.json",completion)
        for sig,handler in handlers.items():signal.signal(sig,handler)
        print(json.dumps(dict(path=str(output),status=completion["status"],error=completion["error"])),flush=True)
    require(completion["status"]=="passed","capture closure failed")

def summarize(rows,cases):
    paired={}
    for row,case in zip(rows,cases,strict=True):
        key=(case["fixture"],case["n"],case["kmax"])
        paired.setdefault(key,[]).append(row)
    for pair in paired.values():
        require(len(pair)==2 and {tuple(r["execution_order"]) for r in pair}=={(28,29,30),(30,29,28)},
                "missing execution-order pair")
        def discrete(row):return {k:v for k,v in row.items() if k not in ("execution_order","timings_ms")}
        require(discrete(pair[0])==discrete(pair[1]),"engine order changed work or outputs")
    for fixture,n,_ in paired:
        low=paired[fixture,n,5][0];high=paired[fixture,n,10][0]
        low_records={json.dumps(x,sort_keys=True) for x in low["records"]}
        high_records={json.dumps(x,sort_keys=True) for x in high["records"]}
        require(low_records<=high_records and low["cover_work"]==high["cover_work"],"K5 is not nested in K10")
    def flatten(value,prefix=""):
        result={}
        for key,item in value.items():
            if type(item) is dict:result.update(flatten(item,prefix+key+"."))
            elif type(item) in (int,float) and key not in ("hash","id_bytes"):result[prefix+key]=item
        return result
    growth={}
    for fixture in sorted({c["fixture"] for c in cases}):
        for k in (5,10):
            for order in ORDERS:
                sequence=sorted([(r,c) for r,c in zip(rows,cases,strict=True) if
                    (c["fixture"],c["kmax"],c["order"])==(fixture,k,order)],key=lambda rc:rc[1]["n"])
                sizes=[c["n"] for _,c in sequence]
                values=[flatten({key:r[key] for key in (*LANES,"cloud_work","index_work","cover_work",
                    "derived_work","judge","memory","timings_ms")}) for r,_ in sequence]
                metrics={}
                for field in values[0] if values else ():
                    series=[v[field] for v in values]
                    metrics[field]=dict(values=series,ratios=[b/a if a else None for a,b in zip(series,series[1:])],
                        above_quadratic_step=[a>0 and b/a>(nb/na)**2 for a,b,na,nb in
                            zip(series,series[1:],sizes,sizes[1:])])
                growth[f"fixture{fixture}_K{k}_order{order}"]=dict(n=sizes,metrics=metrics)
    return dict(growth=growth,order_pairs=len(paired),
        target_retained=sum(c["target_retained"] for c in cases),
        target_removed=sum(not c["target_retained"] for c in cases))

def read(path,check_live=False):
    path=path.resolve();m=read_json(path/"MANIFEST.json");c=read_json(path/"COMPLETION.json")
    require(m["schema"]==SCHEMA and c["manifest_sha256"]==digest(path/"MANIFEST.json") and c["closing_errors"]==[],
            "invalid capture")
    validate_closure(m,c);validate_input_closure(m,c)
    require(m["public_status"]=="not_claimed" and m["gcp_used"] is False and m["full_contract_qualified"] is False and
        m["branch"]=="main" and m["qualification"] in ("preflight","candidate"),"capture scope changed")
    require(type(m["commit"]) is str and len(m["commit"])==40 and all(v in "0123456789abcdef" for v in m["commit"]) and
        type(m["worktree"]) is str and type(m["launch_command"]) is list and
        all(type(v) is str for v in m["launch_command"]) and "CMAKE_CXX_COMPILER:" in m["compiler_cache"] and
        m["environment"]["scope"]=="selected_values_full_environment_fingerprint_no_secrets","provenance missing")
    build=Path(m["build"]);campaign=m["campaign"]
    require(build.is_absolute() and build.is_relative_to(ROOT/"build") and ".." not in build.parts and
        campaign in (*EDGE_CAMPAIGNS,"gate","regression"),"build/campaign differs")
    names=m["executable_names"]
    require(names==sorted(set(names)) and all(type(name) is str and Path(name).name==name and name.startswith("mhgp8_") for name in names),
            "invalid executable inventory")
    require(set(m["source_sha256"])==SOURCES and set(m["artifact_sha256"])==
        {str((build/name).relative_to(ROOT)) for name in [*names,"CMakeCache.txt","libmhgp8_p0.a"]} and
        c["executable_names_after"]==names,"pin inventory differs")
    if campaign!="regression":
        require(names==executable_names(build,campaign) and m["ctest"] is None and m["ctest_sha256"] is None and
            c["ctest_sha256_after"] is None and c["xml_sha256"] is None,"targeted artifacts differ")
    else:
        require(Path(m["ctest"]).is_absolute() and Path(m["ctest"]).name=="ctest" and
            m["ctest_sha256"]==c["ctest_sha256_after"] and
            "MHGP8_SANITIZE:BOOL=OFF" in m["compiler_cache"] and "CMAKE_BUILD_TYPE:STRING=Release" in m["compiler_cache"],
            "regression build differs")
    # The byte-pinned INPUTS are replayed, not any audit's execution/qualification.
    inputs=prepare_inputs(campaign)
    require(inputs==m["inputs"] and pins(input_inventory(campaign))==m["input_sha256"],
            "constructor target recensus or inputs changed")
    commands=[[kind,command] for kind,command in plan(build,campaign,path,inputs,m["ctest"])]
    require(m["planned_commands"]==commands and len(c["records"])==len(commands) and
        {p.name for p in path.glob("record_*.json")}=={f"record_{i:04}.json" for i in range(len(commands))},
        "command plan differs")
    if check_live:
        require(pins(SOURCES)==m["source_sha256"] and pins(m["artifact_sha256"])==m["artifact_sha256"] and
            executable_names(build,campaign)==names and (not m["ctest"] or digest(Path(m["ctest"]))==m["ctest_sha256"]),
            "live source/build changed")
    rows=[];cases=[]
    for i,((kind,command),info) in enumerate(zip(commands,c["records"],strict=True)):
        require(info["path"]==f"record_{i:04}.json" and digest(path/info["path"])==info["sha256"],"record hash/path differs")
        record=read_json(path/info["path"])
        require(record["kind"]==kind and record["command"]==command and record["cwd"]==str(ROOT) and
            record["environment"]==m["environment"] and record["status"]=="passed" and
            type(record["exit_code"]) is int and record["exit_code"]==0,"command/result differs")
        for stream in ("stdout","stderr"):
            require(base64.b64decode(record[stream+"_base64"],validate=True).decode("utf-8",errors="replace")==record[stream],
                    "raw/decoded log differs")
        if kind=="ctest":
            require(c["xml_sha256"]==digest(path/"result.xml") and validate_xml(path/"result.xml")==record["test_names"],
                    "JUnit proof differs")
        else:
            row=parse_result(record["stdout"].encode());require(row==record["row"],"parsed row differs")
            if kind=="edge":
                case=validate_row(row,command,inputs);require(case==record["case"],"target verdict differs")
                rows.append(row);cases.append(case)
            else:validate_gate(row,Path(command[0]).name)
    return dict(status="passed",path=str(path),campaign=campaign,qualification=m["qualification"],sources=len(SOURCES),
        records=len(commands),measurements=len(rows),tests=92 if campaign=="regression" else 0,
        input_files=len(m["input_sha256"]),constructor_target_census_point_tests=inputs["census_point_tests"],
        full_contract_qualified=False,general_subquadratic_bound=False,
        scope="provided_original_edges_not_global_enumeration",**summarize(rows,cases))

def selftest(path):
    summary=read(path);m=read_json(path/"MANIFEST.json");c=read_json(path/"COMPLETION.json")
    records=[read_json(file) for file in sorted(path.glob("record_*.json"))]
    base=next((r for r in records if r["kind"]=="edge" and r["row"]["records"]),None)
    require(base is not None,"selftest needs actual nonempty edge output")
    mutations=0
    def reject(action):
        nonlocal mutations
        try:action()
        except InvalidReceipt:mutations+=1
        else:raise InvalidReceipt("LiDAR receipt mutation survived")
    changes=[
        (["n"],True),(["edge_ids"],[False,1]),(["input_fnv1a64_u64le_n_xyz"],base["row"]["input_fnv1a64_u64le_n_xyz"]^1),
        (["local28","output","hash"],base["row"]["local28"]["output"]["hash"]^1),
        (["window30","selection","retained_ids"],True),(["window30","window","second_pass_sites"],1<<63),
        (["window30","window","rejected_event_ids"],1<<63),(["window30","sweep","family","sites"],1<<63),
        (["shallow29","sweep","family","sites"],1<<63),(["local28","sweep","active_sites"],1<<63),
        (["cover_work","admitted_sites"],0),(["cloud_work","coordinate_copies"],0),
        (["judge","point_tests"],0),(["memory","id_bytes"],True),(["timings_ms","total_before_json"],float("nan")),
        (["records",0,"depth"],1<<63),(["records",0,"shell"],[]),(["records",0,"coefficients"],["1","0","0","0","0"]),
        (["execution_order"],[28,28,30]),(["comparison","equal_pairs"],0)]
    for route,value in changes:
        row=deepcopy(base["row"]);target=row
        for key in route[:-1]:target=target[key]
        target[route[-1]]=value;reject(lambda:validate_row(row,base["command"],m["inputs"]))
    command=base["command"].copy();command[2]=str(int(command[2])+1)
    reject(lambda:validate_row(base["row"],command,m["inputs"]))
    inputs=deepcopy(m["inputs"])
    for target in inputs["targets"]:
        target["expected_K5"]=not target["expected_K5"];target["expected_K10"]=not target["expected_K10"]
    reject(lambda:validate_row(base["row"],base["command"],inputs))
    reject(lambda:parse_result(b'{"x":NaN}'));reject(lambda:parse_result(b'{"x":1,"x":2}'))
    changed=deepcopy(c);changed["source_sha256_after"][next(iter(m["source_sha256"]))]="0"*64
    reject(lambda:validate_closure(m,changed))
    changed=deepcopy(c);changed["input_sha256_after"][next(iter(m["input_sha256"]))]="0"*64
    reject(lambda:validate_input_closure(m,changed))
    return dict(status="passed",mutants=mutations,real_measurements=summary["measurements"],
        scope="receipt_reader_not_geometry",mutations_change_actual_values=True)

def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest="operation",required=True)
    run_parser=sub.add_parser("run");run_parser.add_argument("--build",type=Path,required=True)
    run_parser.add_argument("--output",type=Path,required=True)
    run_parser.add_argument("--campaign",choices=(*EDGE_CAMPAIGNS,"gate","regression"),required=True)
    run_parser.add_argument("--qualification",choices=("preflight","candidate"),default="preflight")
    reader=sub.add_parser("read");reader.add_argument("path",type=Path);reader.add_argument("--check-live",action="store_true")
    reader.add_argument("--compact",action="store_true")
    unit=sub.add_parser("selftest");unit.add_argument("path",type=Path)
    args=parser.parse_args()
    if args.operation=="run":run(args)
    else:
        result=read(args.path,args.check_live) if args.operation=="read" else selftest(args.path)
        if getattr(args,"compact",False):result={k:v for k,v in result.items() if k!="growth"}
        print(json.dumps(result,sort_keys=True,allow_nan=False))

if __name__=="__main__":main()
