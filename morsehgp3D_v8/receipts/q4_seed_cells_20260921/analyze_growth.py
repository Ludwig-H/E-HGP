#!/usr/bin/env python3
"""Post-hoc constructor34 analysis. Read closed receipts, NEVER run natives.

Explicit reuse: the previous constructor33 analysis supplies unchanged metric
definitions and JSON/hash utilities. The historical executions keep their own
schemas; projections below are comparison objects, not invented executions.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
import importlib.util
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BENCH = ROOT / "morsehgp3D_v8/bench"
sys.path.insert(0, str(BENCH))
import run_q34_affine_checks as reader33
import run_q4_seed_cells_checks as reader34

_spec = importlib.util.spec_from_file_location("_constructor33_growth_helpers",
    HERE.parent / "q34_affine_20260921/analyze_growth.py")
if _spec is None or _spec.loader is None:
    raise RuntimeError("missing explicit historical analysis helper")
previous = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(previous)
require, at, flatten = previous.require, previous.at, previous.flatten
read_json, pins, stamp = previous.read_json, previous.pins, previous.stamp
first_difference, write_new = previous.first_difference, previous.write_new

CONFIG = (*previous.CONFIG, "witness_bounds_mode")
NAVIGATION_FIELDS = ("seed_queries", "seed_owner_tests", "seed_owner_rejections",
                     "query_visits", "line_tests", "line_skips")
NORMALIZED = previous.NORMALIZED


def mode(entry):
    return entry["row"].get("q4_seed_mode", "individual")


def key(entry, workers=True, backend=True):
    row = entry["row"]
    return tuple(row[name] for name in CONFIG if (workers or name != "workers") and
                 (backend or name != "q4_backend")) + (row["input_hash"],)


def common_work(row, remove_navigation, remove_q4=False):
    work = deepcopy(row["work"])
    work["q3"]["peak_shell_bytes"] = 0
    work["peak_edge_buffer_bytes"] = 0
    work.pop("q4_seed_cells", None)
    if remove_q4:
        del work["local28"]
        del work["window30"]
    elif remove_navigation:
        require(row["q4_backend"] == 28, "navigation projection is Local28 only")
        del work["local28"]["edge"]
        for name in NAVIGATION_FIELDS:
            del work["local28"]["sweep"][name]
    return dict(front=row["front"], work=work, cloud_work=row["cloud_work"], index_work=row["index_work"])


def primary_metrics(entry):
    row, version = entry["row"], entry["version"]
    values, definitions = previous.primary_metrics(row, 33)

    def add(name, *paths, kind="executed_work", note=""):
        values[name] = sum(at(row, path) for path in paths)
        definitions[name] = dict(source_paths=list(paths), operation="sum of distinct executed stages" if len(paths)>1 else "identity",
                                 kind=kind, note=note)

    if row["q4_backend"] == 28:
        for field in ("bound_tests", "point_tests"):
            add("local28.seed_" + field, "work.local28.edge." + field)
        add("local28.seed_family_preparations", "work.local28.sweep.seed_queries")
        add("local28.all_seed_owner_tests", "work.local28.edge.owner_tests", "work.local28.sweep.seed_owner_tests")
        add("local28.terminal_incidences", "work.local28.sweep.leaf_queries")
        add("local28.atlas_cells", "work.local28.atlas.cells_created")
        add("local28.terminal_refinements", "work.local28.atlas.terminal_refinements")
        add("local28.depth_stops", "work.local28.atlas.depth_stops")
        add("local28.partition_block_bounds", "work.local28.atlas.partition.block_bound_tests")
        add("local28.partition_point_tests", "work.local28.atlas.partition.point_tests")
        add("local28.total_edge_peak_bytes", "work.local28.edge.peak_live_buffer_bytes", kind="storage_not_work",
            note="Coupled per-edge capacity peak, including atlas preparation; not RSS or worker-sum.")
        local = row["work"]["local28"]
        extra = row["work"].get("q4_seed_cells")
        navigation = local["sweep"]["query_visits"]
        if mode(entry) == "joined":
            # edge.node_visits is a SUBSET of predicates evaluated within the
            # antichain/product visits; adding it here would count twice.
            navigation += extra["antichain_node_visits"] + extra["product_visits"]
        else:
            navigation += local["edge"]["node_visits"]
        if extra is not None:
            navigation += extra["live_node_visits"]
        values["local28.navigation_frames"] = navigation
        definitions["local28.navigation_frames"] = dict(kind="executed_navigation_not_weighted_cpu_total",
            operation="Individual/Live: spatial node visits + seed query visits; Joined: antichain visits + product visits; both add actually prepared live-summary node visits",
            note="Joined's old edge.node_visits are predicate subsets of those traversals and are NOT added. Summary child reads remain separately reported.")
        values["local28.seed_cell_geometry_tests"] = local["sweep"]["line_tests"]
        if extra is not None:
            values["local28.seed_cell_geometry_tests"] += extra["block_bound_tests"] + extra["singleton_bound_tests"]
        definitions["local28.seed_cell_geometry_tests"] = dict(kind="executed_work",
            operation="old line tests + new block bounds + new singleton bounds, only stages actually executed",
            note="Block quadratic bounds and singleton linear bounds have different costs; no time-equivalence asserted.")
    if version == 34:
        executed = set("live_node_visits live_child_reads antichain_node_visits cache_entries_initialized cache_hits cache_misses family_preparations form_preparations product_visits block_bound_tests singleton_bound_tests".split())
        for field in reader34.SUM_FIELDS:
            add("seed_cells." + field, "work.q4_seed_cells." + field,
                kind="executed_work_or_predicate_subset" if field in executed else "counter_not_additive_by_default",
                note="block_sites aliases initialized cache entries; cache_misses aliases seed point tests; outcome counters partition parent visits. Do not sum these aliases.")
        for field in reader34.MAX_FIELDS:
            add("seed_cells." + field, "work.q4_seed_cells." + field,
                kind="storage_not_work" if "bytes" in field else "maximum_not_additive")
    add("memory.worker_edge_peak_sum", "parallel.edge_buffer_bytes_sum", kind="storage_not_work",
        note="Sum of per-worker maxima, not simultaneous peak, not RSS. Can vary with scheduling.")
    return values, definitions


def compare(entries):
    counts = dict(payload_digest_pairs=0, full_record_pairs=0, upstream_and_terminal_pairs=0, cross_backend_upstream_pairs=0,
                  all_common_work_pairs=0, constructor33_34_pairs=0, r1_r2_pairs=0, worker_pairs=0)
    comparisons, seen = [], {}
    for entry in entries:
        row = entry["row"]
        payload_key = tuple(row[name] for name in ("scan", "n", "input_hash", "kmax", "mask"))
        if payload_key in seen:
            reference = seen[payload_key]
            require(entry["input_sha256"] == reference["input_sha256"], "same input identity differs in byte SHA256")
            require(row["output"] == reference["row"]["output"], "counts/digests differ: " + entry["id"])
            counts["payload_digest_pairs"] += 1
            if "records" in row and "records" in reference["row"]:
                require(row["records"] == reference["row"]["records"], "complete payload records differ")
                counts["full_record_pairs"] += 1
        else:
            seen[payload_key] = entry
    for i, left in enumerate(entries):
        for right in entries[i+1:]:
            if key(left, False, False) != key(right, False, False):
                continue
            same_backend = left["row"]["q4_backend"] == right["row"]["q4_backend"]
            same_mode = mode(left) == mode(right)
            a = common_work(left["row"], not same_mode, not same_backend)
            b = common_work(right["row"], not same_mode, not same_backend)
            difference = first_difference(a, b)
            require(difference is None, f"common work differs at {difference}: {left['id']} / {right['id']}")
            if same_mode and same_backend and left["version"] == right["version"] == 34:
                difference = first_difference(left["row"]["work"]["q4_seed_cells"], right["row"]["work"]["q4_seed_cells"])
                require(difference is None, "same-mode new ledger differs: " + str(difference))
            counts["cross_backend_upstream_pairs" if not same_backend else "all_common_work_pairs" if same_mode else "upstream_and_terminal_pairs"] += 1
            counts["constructor33_34_pairs"] += int(left["version"] != right["version"])
            counts["r1_r2_pairs"] += int({left["revision"],right["revision"]} == {"34-r1","34-r2"})
            counts["worker_pairs"] += int(left["row"]["workers"] != right["row"]["workers"])
            va, _ = primary_metrics(left); vb, _ = primary_metrics(right)
            names = sorted(va.keys() & vb.keys())
            comparisons.append(dict(records=[left["id"],right["id"]],profiles=[left["profile"],right["profile"]],
                full_common_work_identical=same_mode and same_backend, upstream_q3_cover_and_filters_identical=True,
                upstream_atlas_and_complete_terminal_sweep_identical=same_backend,
                normalized_capacity_fields=list(NORMALIZED),
                metrics_before={n:va[n] for n in names},metrics_after={n:vb[n] for n in names},
                ratios={n:vb[n]/va[n] if va[n] else None for n in names},
                zero_to_positive=[n for n in names if va[n]==0 and vb[n]>0],
                new_only={n:vb[n] for n in vb.keys()-va.keys()},old_only={n:va[n] for n in va.keys()-vb.keys()},
                timings_before=left["row"]["timings_ms"],timings_after=right["row"]["timings_ms"],
                stable_time_gain_claimed=False))
    return counts, comparisons


def growth(entries):
    groups, definitions, measurements = {}, {}, []
    for entry in entries:
        row = entry["row"]
        values, defs = primary_metrics(entry)
        for name, definition in defs.items():
            require(name not in definitions or definitions[name] == definition, "metric definition changed: " + name)
            definitions[name] = definition
        configuration = {name:row[name] for name in CONFIG}
        series = (entry["profile"],) + tuple(row[name] for name in CONFIG if name != "n")
        groups.setdefault(series, {}).setdefault(row["n"], []).append((entry, values))
        measurements.append(dict(id=entry["id"],capture=entry["capture"],revision=entry["revision"],profile=entry["profile"],
            configuration=configuration,seed_mode=mode(entry),input_hash=row["input_hash"],input_sha256=entry["input_sha256"],
            record_sha256=entry["record_sha256"],output=row["output"],metrics=values,
            raw_counters=previous.raw_metrics(row),timings_ms=row["timings_ms"]))
    ratios, repetitions = [], []
    for series, sizes in sorted(groups.items()):
        points = []
        for n, members in sorted(sizes.items()):
            entry, values = members[0]
            # Scheduling/capacity is not deterministic across repetitions.
            deterministic = {k:v for k,v in values.items() if definitions[k]["kind"] != "storage_not_work"}
            require(all({k:v for k,v in other.items() if definitions[k]["kind"] != "storage_not_work"} == deterministic
                        for _,other in members), "repeated primary geometric work changed")
            points.append((n, entry, values))
            if len(members)>1:
                repetitions.append(dict(profile=series[0],n=n,records=[e["id"] for e,_ in members],
                    timings_ms=[e["row"]["timings_ms"] for e,_ in members],deterministic_geometry_identical=True))
        for (na,a,va),(nb,b,vb) in zip(points,points[1:]):
            require(va.keys()==vb.keys(),"growth metrics differ")
            factor=(nb/na)**2
            r={k:vb[k]/v if v else None for k,v in va.items()}
            ra,rb=previous.raw_metrics(a["row"]),previous.raw_metrics(b["row"])
            require(ra.keys()==rb.keys(),"raw growth metrics differ")
            raw={k:rb[k]/v if v else None for k,v in ra.items()}
            ratios.append(dict(profile=series[0],configuration={k:v for k,v in zip((k for k in CONFIG if k!="n"),series[1:])},
                n=[na,nb],records=[a["id"],b["id"]],quadratic_step=factor,metrics_before=va,metrics_after=vb,ratios=r,
                above_quadratic_step=[k for k,v in r.items() if v is not None and v>factor],
                zero_to_positive=[k for k,v in va.items() if v==0 and vb[k]>0],raw_ratios=raw,
                raw_above_quadratic_step=[dict(name=k,ratio=v,kind=previous.raw_kind(k)) for k,v in raw.items() if v is not None and v>factor],
                stable_time_gain_claimed=False))
    return dict(metric_definitions=definitions,measurements=measurements,growth=ratios,repetitions=repetitions)


def analyze(captures):
    entries, readers = [], []
    for revision, path in captures:
        version=33 if revision=="33" else 34
        reader=reader33 if version==33 else reader34
        result=reader.read(path,False)
        require(result["status"]=="passed" and result["campaign"]=="lidar","capture is not closed LiDAR PASS")
        readers.append(dict(revision=revision,path=str(path),reader=reader.__name__,check_live=False,
                            result={k:v for k,v in result.items() if k!="growth"}))
        manifest=read_json(path/"MANIFEST.json")
        for item in read_json(path/"COMPLETION.json")["records"]:
            record=read_json(path/item["path"])
            row=record["row"]
            require(record["kind"]=="probe","non-probe performance record")
            require(row["schema"]==("mhgp8_wspd_q34_probe_v3" if version==33 else "mhgp8_wspd_q34_probe_v4"),"wrong actual execution schema")
            input_name=str(Path(record["command"][1]).resolve().relative_to(ROOT))
            seed_mode=row.get("q4_seed_mode","individual")
            profile=revision+"/"+row["witness_bounds_mode"]+"/"+seed_mode
            if version==34:profile+="/grain"+str(row["q4_seed_block_size"])
            entries.append(dict(id=str(path/item["path"]),capture=str(path),revision=revision,version=version,
                profile=profile,row=row,input_sha256=manifest["input_sha256"][input_name],record_sha256=item["sha256"]))
    counts, comparisons=compare(entries)
    return dict(reader_results=readers,comparisons=counts,paired_work=comparisons,**growth(entries))


def verify(paths,output):
    require(len(set(paths))==2,"distinct reports required")
    reports=[read_json(p) for p in paths]
    require([r["runtime_optimize"] for r in reports]==[0,1],"normal then optimized required")
    watched={str(p.resolve()) for p in paths}|{str(Path(__file__).resolve())}
    for r in reports:
        require(r["schema"]=="mhgp8_q4_seed_cells_growth_analysis_v1" and r["status"]=="passed" and
                r["error"] is None and not r["closing_errors"] and r["native_executions"]==0,"analysis not closed PASS")
        for family in ("source_sha256","input_sha256"):
            require(r[family]==r[family+"_after"]==pins(r[family]),"analysis inputs changed")
            watched.update(r[family])
    before=pins(watched)
    ignored={"argv","started_utc","finished_utc","runtime_optimize"}
    difference=first_difference(*({k:v for k,v in r.items() if k not in ignored} for r in reports))
    after=pins(watched)
    success=difference is None and before==after
    result=dict(schema="mhgp8_q4_seed_cells_growth_readback_v1",status="passed" if success else "failed",
        reports=[dict(path=str(p.resolve()),sha256=before[str(p.resolve())],runtime_optimize=r["runtime_optimize"]) for p,r in zip(paths,reports)],
        first_difference=difference,normal_optimized_identical=difference is None,ignored_execution_metadata=sorted(ignored),
        input_sha256=before,input_sha256_after=after,closing_errors=[] if before==after else ["readback inputs changed"],
        native_executions=0,finished_utc=stamp(),full_contract_qualified=False,universal_subquadratic_claim=False,stable_time_gain_claimed=False)
    write_new(output,result)
    print(json.dumps(dict(status=result["status"],files=len(before),output=str(output)),sort_keys=True))
    return 0 if success else 1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture33",type=Path,nargs="*",default=[])
    parser.add_argument("--capture-r1",type=Path,nargs="*",default=[])
    parser.add_argument("--capture-r2",type=Path,nargs="*",default=[])
    parser.add_argument("--verify-reports",type=Path,nargs=2)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    output=args.output.resolve()
    require(output.is_relative_to(HERE) and not output.exists(),"output must be new and inside receipts34")
    if args.verify_reports:
        require(not args.capture33 and not args.capture_r1 and not args.capture_r2,"readback cannot also analyze")
        return verify(args.verify_reports,output)
    captures=[(revision,p.resolve()) for revision,paths in (("33",args.capture33),("34-r1",args.capture_r1),("34-r2",args.capture_r2)) for p in paths]
    require(captures and len(captures)==len(set(captures)),"missing or repeated captures")
    files={Path(__file__).resolve(),Path(previous.__file__).resolve()}
    for module in tuple(sys.modules.values()):
        name=getattr(module,"__file__",None)
        if name and Path(name).resolve().is_relative_to(BENCH):files.add(Path(name).resolve())
    expected={}
    for revision,path in captures:
        require(path.is_relative_to(ROOT/"morsehgp3D_v8/receipts"),"capture outside receipts")
        require((path/"COMPLETION.json").is_file(),"capture still open: "+str(path))
        manifest=read_json(path/"MANIFEST.json");completion=read_json(path/"COMPLETION.json")
        require(completion["status"]=="passed" and not completion["closing_errors"],"capture not closed successfully")
        files.update((path/"MANIFEST.json",path/"COMPLETION.json"))
        files.update(path/item["path"] for item in completion["records"])
        for family in ("input_sha256","artifact_sha256"):
            for name,value in manifest[family].items():
                target=(ROOT/name).resolve()
                require(str(target) not in expected or expected[str(target)]==value,"conflicting input/artifact pin")
                expected[str(target)]=value;files.add(target)
    before=pins(files)
    for name,value in expected.items():require(before[name]==value,"input/artifact changed: "+name)
    sources=pins(ROOT/name for name in reader34.SOURCES)
    result=dict(schema="mhgp8_q4_seed_cells_growth_analysis_v1",status="failed",error=None,started_utc=stamp(),
        argv=[sys.executable,*sys.argv],runtime_optimize=sys.flags.optimize,native_executions=0,
        normalized_capacity_fields=list(NORMALIZED),historical_rows_reinterpreted_as_executions=False,
        full_contract_qualified=False,universal_subquadratic_claim=False,stable_time_gain_claimed=False,gcp_used=False,
        source_sha256=sources,input_sha256=before,closing_errors=[])
    try:
        result.update(analyze(captures));result["status"]="passed"
    except BaseException as cause:
        result["error"]=f"{type(cause).__name__}: {cause}"
    finally:
        try:
            result["source_sha256_after"]=pins(ROOT/name for name in reader34.SOURCES)
            result["input_sha256_after"]=pins(files)
            if result["source_sha256_after"]!=sources:result["closing_errors"].append("current216 sources changed")
            if result["input_sha256_after"]!=before:result["closing_errors"].append("readers/captures/inputs/artifacts changed")
        except BaseException as cause:
            result["closing_errors"].append(f"closing hash failure: {type(cause).__name__}: {cause}")
        result["finished_utc"]=stamp()
        if result["closing_errors"]:result["status"]="failed"
        write_new(output,result)
    print(json.dumps(dict(status=result["status"],error=result["error"],closing_errors=result["closing_errors"],
        measurements=len(result.get("measurements",[])),growth_steps=len(result.get("growth",[])),
        comparisons=result.get("comparisons",{}),output=str(output)),sort_keys=True))
    return 0 if result["status"]=="passed" else 1


if __name__=="__main__":
    raise SystemExit(main())
