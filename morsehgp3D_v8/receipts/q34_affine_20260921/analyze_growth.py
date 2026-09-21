#!/usr/bin/env python3
"""Read closed32/33 captures and analyze work; NEVER run a native executable.

Historical rows remain in their original schema. Common-field projections are
comparison objects only, never manufactured executions. Repeated measurements
retain every timing observation; deterministic work defines one growth point.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BENCH = ROOT / "morsehgp3D_v8/bench"
sys.path.insert(0, str(BENCH))
import run_q34_indexed_checks as old
import run_q34_affine_checks as new

NORMALIZED = ("work.q3.peak_shell_bytes", "work.peak_edge_buffer_bytes")
CONFIG = ("scan", "n", "kmax", "s", "mask", "q4_backend", "workers", "front_mode", "witness_mode", "q3_census_mode")
SERIES = tuple(name for name in CONFIG if name != "n")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def stamp():
    return datetime.now(timezone.utc).isoformat()


def sha(path):
    with Path(path).open("rb") as stream:
        result = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def read_json(path):
    return json.loads(Path(path).read_text())


def pins(paths):
    return {str(Path(path).resolve()): sha(path) for path in sorted(set(map(str, paths)))}


def at(value, path):
    for part in path.split("."):
        value = value[part]
    return value


def flatten(value, prefix=""):
    result = {}
    if isinstance(value, dict):
        for key, item in value.items():
            result.update(flatten(item, f"{prefix}.{key}" if prefix else key))
    elif isinstance(value, list):
        for key, item in enumerate(value):
            result.update(flatten(item, f"{prefix}[{key}]"))
    elif type(value) in (int, float):
        result[prefix] = value
    return result


def comparison_work(row, legacy=False):
    result = deepcopy(row["work"])
    result["q3"]["peak_shell_bytes"] = 0
    result["peak_edge_buffer_bytes"] = 0
    if legacy:
        for name in ("rectangles_bounds", "pairs_bounds"):
            if name in result["witness"]:
                require(all(value == 0 for value in result["witness"][name].values()),
                        "nonzero new bounds ledger cannot be projected as Legacy")
                del result["witness"][name]
    else:
        # Before final exact pair filtering, stronger rectangle bounds may
        # change expansion and its own rejection masses. ALL other fields,
        # including final edge counts, covers, seeds and payload, stay checked.
        del result["witness"]
        del result["expanded_pairs"]
    return dict(front=row["front"], work=result, cloud_work=row["cloud_work"], index_work=row["index_work"])


def first_difference(a, b, path=""):
    if type(a) is not type(b):
        return path + " (type)"
    if isinstance(a, dict):
        if a.keys() != b.keys():
            return path + " (keys)"
        for key in a:
            different = first_difference(a[key], b[key], f"{path}.{key}" if path else key)
            if different:
                return different
    elif isinstance(a, list):
        if len(a) != len(b):
            return path + " (length)"
        for key, (left, right) in enumerate(zip(a, b)):
            different = first_difference(left, right, f"{path}[{key}]")
            if different:
                return different
    elif a != b:
        return path
    return None


def compare_entries(entries):
    payloads, downstream, legacy = {}, {}, {}
    comparisons = dict(payload_digest_pairs=0, full_record_pairs=0, downstream_pairs=0,
                       legacy32_33_pairs=0, profile_pairs=0, cross_version_downstream_pairs=0)
    groups = {}
    for entry in entries:
        row, version = entry["row"], entry["version"]
        payload_key = tuple(row[name] for name in ("scan", "n", "input_hash", "kmax", "mask"))
        if payload_key in payloads:
            previous = payloads[payload_key]
            require(entry["input_sha256"]==previous["input_sha256"], "same input identity has different exact byte SHA256")
            require(row["output"] == previous["row"]["output"],
                    f"payload counts/digests differ: {previous['id']} / {entry['id']}")
            comparisons["payload_digest_pairs"] += 1
            if "records" in row and "records" in previous["row"]:
                require(row["records"] == previous["row"]["records"], "complete payload records differ")
                comparisons["full_record_pairs"] += 1
        else:
            payloads[payload_key] = entry
        key = tuple(row[name] for name in CONFIG) + (row["input_hash"],)
        projected = comparison_work(row)
        if key in downstream:
            previous, reference = downstream[key]
            difference = first_difference(reference, projected)
            require(difference is None,
                    f"downstream changed at {difference}: {previous['id']} / {entry['id']}")
            comparisons["downstream_pairs"] += 1
            comparisons["cross_version_downstream_pairs"] += int(previous["version"] != version)
            comparisons["profile_pairs"] += int(previous["profile"] != entry["profile"])
        else:
            downstream[key] = (entry, projected)
        groups.setdefault(key, []).append(entry)
        if version == 32 or row["witness_bounds_mode"] == "legacy":
            projected = comparison_work(row, legacy=True)
            if key in legacy:
                previous, reference = legacy[key]
                difference = first_difference(reference, projected)
                require(difference is None,
                        f"Legacy32/33 common work changed at {difference}: {previous['id']} / {entry['id']}")
                comparisons["legacy32_33_pairs"] += int(previous["version"] != version)
            else:
                legacy[key] = (entry, projected)
    coverage, profile_comparisons = [], []
    for key, members in sorted(groups.items()):
        present = sorted({entry["profile"] for entry in members})
        coverage.append(dict(configuration=dict(zip(CONFIG, key[:-1])), input_hash=key[-1],
            profiles=present, missing33_modes=[mode for mode in ("legacy", "exclude", "affine")
                if "33/" + mode not in present], records=[entry["id"] for entry in members]))
        for index, before_entry in enumerate(members):
            for after_entry in members[index+1:]:
                if before_entry["profile"] == after_entry["profile"]:
                    continue
                before, _ = primary_metrics(before_entry["row"], before_entry["version"])
                after, _ = primary_metrics(after_entry["row"], after_entry["version"])
                shared = sorted(before.keys() & after.keys())
                profile_comparisons.append(dict(configuration=dict(zip(CONFIG, key[:-1])),
                    input_hash=key[-1], profiles=[before_entry["profile"], after_entry["profile"]],
                    records=[before_entry["id"], after_entry["id"]],
                    common_metrics_before={name:before[name] for name in shared},
                    common_metrics_after={name:after[name] for name in shared},
                    ratios={name:after[name]/before[name] if before[name] else None for name in shared},
                    zero_to_positive=[name for name in shared if before[name]==0 and after[name]>0],
                    before_only={name:before[name] for name in sorted(before.keys()-after.keys())},
                    after_only={name:after[name] for name in sorted(after.keys()-before.keys())},
                    absent_historical_counters_fabricated=False, stable_time_gain_claimed=False))
    return comparisons, coverage, profile_comparisons


def primary_metrics(row, version):
    metrics, definitions = {}, {}
    def add(name, *paths, kind="executed_work", note=""):
        metrics[name] = sum(at(row, path) for path in paths)
        definitions[name] = dict(source_paths=list(paths), operation="sum of distinct executed stages" if len(paths)>1 else "identity",
                                 kind=kind, note=note)
    direct = {
        "prepare.unique_comparisons": "cloud_work.uniqueness_comparisons",
        "prepare.index_point_visits": "index_work.point_visits",
        "front.product_visits": "front.work.product_visits",
        "front.witness_box_distance_tests": "front.work.witness_box_distance_tests",
        "front.h_bound_tests": "front.work.h_bound_tests",
        "front.xi_bound_tests": "front.work.xi_bound_tests",
        "expanded_pairs": "work.expanded_pairs",
        "cover.node_visits": "work.cover.node_visits",
        "q3.seed_node_visits": "work.q3.seed_node_visits",
        "q3.owner_tests": "work.q3.owner_tests",
        "q3.ball_builds": "work.q3.ball_builds",
        "q3.count_bounds_prepared": "work.q3_blocks.count_bounds_prepared",
        "q3.shell_bounds_prepared": "work.q3_blocks.shell_bounds_prepared",
        "q3.count_prepared_unvisited": "work.q3_blocks.count_prepared_unvisited",
        "q3.scalar_census_points": "work.q3.census_point_tests",
        "q3.shell_sort_comparisons": "work.q3.shell_sort_comparisons",
    }
    for name, path in direct.items():
        add(name, path)
    add("q3.all_prepared_bounds", "work.q3_blocks.count_bounds_prepared", "work.q3_blocks.shell_bounds_prepared",
        note="Includes prepared-unvisited child bounds; do not add that subset again.")
    for field in ("prepared_bounds", "h_bound_tests", "xi_bound_tests", "midpoint_box_tests"):
        add("filter." + field, *(f"work.witness.{section}.{field}" for section in ("rectangles", "pairs")))
    add("filter.admission_lane_tests", *(f"work.witness.{section}.{lane}_lane_tests"
        for section in ("rectangles", "pairs") for lane in ("q3", "q4")))
    for section in ("rectangles", "pairs"):
        for field in ("h_bound_tests", "xi_bound_tests", "q3_lane_tests", "q4_lane_tests", "midpoint_box_tests"):
            add(f"filter.{section}.{field}", f"work.witness.{section}.{field}")
    if version == 33:
        for field in new.BOUNDS_FIELDS:
            add("bounds." + field, *(f"work.witness.{section}_bounds.{field}" for section in ("rectangles", "pairs")),
                kind="executed_test_subset" if field.startswith("affine_") else "counter",
                note="Affine H/Xi tests are INCLUDED in filter H/Xi, not additional evaluations." if field.startswith("affine_") else "")
        add("filter.exclusion_lane_tests", *(f"work.witness.{section}_bounds.{lane}_exclusion_tests"
            for section in ("rectangles", "pairs") for lane in ("q3", "q4")))
        metrics["filter.all_lane_predicate_tests"] = metrics["filter.admission_lane_tests"] + metrics["filter.exclusion_lane_tests"]
        definitions["filter.all_lane_predicate_tests"] = dict(operation="admission lane tests + exclusion lane tests",
            kind="executed_work", note="Distinct comparisons, but not a time-weighted CPU total.")
        metrics["filter.xi_hmin_positive"] = metrics["filter.xi_bound_tests"] - metrics["bounds.xi_on_nonpositive_minimum"]
        definitions["filter.xi_hmin_positive"] = dict(operation="all Xi evaluations - Xi on Hmin<=0", kind="executed_test_subset",
            note="An Xi evaluation may serve BOTH exclusion and admission; never count it twice.")
        require(metrics["filter.xi_hmin_positive"] >= 0, "negative Xi-positive subset")
    if row["q4_backend"] == 28:
        prefix = "work.local28"
        for name, path in {
            "seeds":"edge.seeds", "seed_node_visits":"edge.node_visits", "seed_owner_tests":"edge.owner_tests",
            "cover_node_visits":"geometry.cover_node_visits", "projection_points":"geometry.projection_points",
            "domain_node_visits":"geometry.domain.node_visits", "domain_endpoint_tests":"geometry.domain.endpoint_box_tests",
            "frontier_id_copies":"atlas.partition.frontier_ids_copied",
            "unexamined_nodes":"atlas.partition.budget_unexamined_nodes", "query_visits":"sweep.query_visits",
            "line_tests":"sweep.line_tests", "site_scan":"sweep.active_sites", "root_locations":"sweep.root_locations",
            "presentations":"sweep.presentations", "owner_tests":"sweep.owner_tests",
            "positive_tests":"sweep.positive_tests", "canonical_tests":"sweep.canonical_tests",
        }.items():
            add("local28." + name, prefix + "." + path)
        for name, fields in {
            "hull_comparisons_orientations":("geometry.hull_sort_comparisons", "geometry.hull_orientation_tests"),
            "partition_geometry":("atlas.partition.block_bound_tests", "atlas.partition.point_tests"),
            "cell_domain_tests":("atlas.domain.disk_tests", "atlas.domain.facet_tests"),
            "sort_group_comparisons":("sweep.sort_comparisons", "sweep.shell_sort_comparisons", "sweep.group_comparisons"),
        }.items():
            add("local28." + name, *(prefix + "." + field for field in fields))
    else:
        prefix = "work.window30"
        for name, path in {
            "seeds":"edge.seeds", "seed_candidates":"edge.seed_candidates", "seed_owner_tests":"edge.owner_tests",
            "cover_node_visits":"geometry.cover_node_visits", "selection_site_forms":"selection.form_tests",
            "layer_group_visits":"selection.layer_input_groups", "hull_index_copies":"selection.hull_index_copies",
            "compaction_moves":"selection.compaction_moves", "first_scan":"sweep.family.sites",
            "second_scan":"window.second_pass_sites", "presentations":"sweep.presentations",
            "owner_tests":"sweep.owner_tests", "positive_tests":"sweep.positive_tests", "canonical_tests":"sweep.canonical_tests",
        }.items():
            add("window30." + name, prefix + "." + path)
        add("window30.two_scans", prefix+".sweep.family.sites", prefix+".window.second_pass_sites")
        add("window30.selection_comparisons_orientations", *(prefix+".selection."+field for field in
            ("lex_comparisons", "orientation_tests", "retained_id_sort_comparisons")))
        add("window30.root_comparisons", *(prefix+"."+field for field in ("window.heap_comparisons",
            "window.heap_sort_comparisons", "window.window_comparisons", "sweep.family.sort_comparisons", "sweep.family.group_comparisons")))
    for field in ("callbacks", "q3", "q4", "support_ids", "shell_ids"):
        add("output."+field, "output."+field, kind="actual_payload")
    return metrics, definitions


def raw_metrics(row):
    return flatten({name: row[name] for name in ("front", "work", "cloud_work", "index_work", "parallel", "memory")})


def raw_kind(name):
    # Keep these raw values in the report, but never treat represented mass
    # as individually visited geometry or sum aliases into a work total.
    population_tokens = ("pair_mass", "factor_sites", "rejected_sites", "excluded_sites", "cover_sites",
                         "input_sites", "inherited_inside_sites", "layer_input_ids", "count_inside_sites", "count_nonnegative_sites")
    if any(token in name for token in population_tokens) or ".partition.active_sites" in name or ".partition.inside_sites" in name or ".partition.outside_sites" in name:
        return "represented_population_not_scalar_work"
    if "bytes" in name or "capacity" in name:
        return "storage_not_work"
    if name.endswith(".depth_stops"):
        return "executed_construction_stop_decisions"
    if "max_" in name or "peak_" in name:
        return "maximum_or_structural_class"
    return "raw_counter_or_subset_not_additive_by_default"


def growth(entries):
    groups, definitions, measurements = {}, {}, []
    for entry in entries:
        row = entry["row"]
        metrics, defs = primary_metrics(row, entry["version"])
        for name, definition in defs.items():
            require(name not in definitions or definitions[name] == definition, "metric definition changed")
            definitions[name] = definition
        series = (entry["version"], entry["profile"]) + tuple(row[name] for name in SERIES)
        groups.setdefault(series, {}).setdefault(row["n"], []).append((entry, metrics))
        measurements.append(dict(id=entry["id"], capture=entry["capture"], version=entry["version"], profile=entry["profile"],
            configuration={name: row[name] for name in CONFIG}, input_hash=row["input_hash"],
            input_sha256=entry["input_sha256"],record_sha256=entry["record_sha256"],
            output=deepcopy(row["output"]), metrics=metrics, raw_counters=raw_metrics(row), timings_ms=deepcopy(row["timings_ms"])))
    ratios, repetitions = [], []
    for key, sizes in sorted(groups.items()):
        points = []
        for n, repeats in sorted(sizes.items()):
            reference, metrics = repeats[0]
            require(all(other == metrics for _, other in repeats), "repeated configuration changed deterministic primary work")
            times = [dict(id=entry["id"], timings_ms=entry["row"]["timings_ms"]) for entry, _ in repeats]
            points.append((n, reference, metrics, times))
            if len(repeats)>1:
                repetitions.append(dict(version=key[0], profile=key[1], configuration={**dict(zip(SERIES,key[2:])),"n":n},
                                        observations=times, deterministic_primary_work_identical=True))
        for left, right in zip(points, points[1:]):
            na, a, before, ta = left; nb, b, after, tb = right
            require(before.keys()==after.keys(), "growth metric inventories differ")
            threshold=(nb/na)**2
            values={name:after[name]/v if v else None for name,v in before.items()}
            # Raw maxima and scheduling values come from explicit representative
            # records; repetitions are retained above, not silently averaged.
            raw_a,raw_b=raw_metrics(a["row"]),raw_metrics(b["row"])
            require(raw_a.keys()==raw_b.keys(), "raw growth inventory differs")
            raw_ratios={name:raw_b[name]/v if v else None for name,v in raw_a.items()}
            ratios.append(dict(version=key[0],profile=key[1],configuration=dict(zip(SERIES,key[2:])),n=[na,nb],
                representative_records=[a["id"],b["id"]],quadratic_step=threshold,metrics_before=before,metrics_after=after,
                ratios=values,above_quadratic_step=[name for name,v in values.items() if v is not None and v>threshold],
                zero_to_positive=[name for name,v in before.items() if v==0 and after[name]>0],
                raw_ratios=raw_ratios,raw_above_quadratic_step=[dict(name=name,ratio=v,kind=raw_kind(name))
                    for name,v in raw_ratios.items() if v is not None and v>threshold],
                timing_observations_before=ta,timing_observations_after=tb,stable_time_gain_claimed=False))
    return dict(metric_definitions=definitions,measurements=measurements,growth=ratios,repeated_configurations=repetitions)


def analyze(captures):
    entries,readbacks=[],[]
    for version,path in captures:
        reader=old if version==32 else new
        summary=reader.read(path,False)
        require(summary["status"]=="passed" and summary["campaign"]=="lidar", "capture is not closed LiDAR PASS")
        readbacks.append(dict(version=version,path=str(path),method=reader.__name__+".read(path, check_live=False)",
                              result={k:v for k,v in summary.items() if k!="growth"}))
        completion=read_json(path/"COMPLETION.json")
        manifest=read_json(path/"MANIFEST.json")
        for item in completion["records"]:
            record=read_json(path/item["path"])
            require(record["kind"]=="probe", "unexpected non-probe in performance capture")
            row=record["row"]
            expected="mhgp8_wspd_q34_probe_v2" if version==32 else "mhgp8_wspd_q34_probe_v3"
            require(row["schema"]==expected, "historical execution schema differs")
            profile="32/legacy-reference" if version==32 else "33/"+row["witness_bounds_mode"]
            input_name=str(Path(record["command"][1]).resolve().relative_to(ROOT))
            entries.append(dict(id=str(path/item["path"]),capture=str(path),version=version,profile=profile,row=row,
                input_sha256=manifest["input_sha256"][input_name],record_sha256=item["sha256"]))
    require(entries,"no measurements to analyze")
    comparisons,coverage,profile_comparisons=compare_entries(entries)
    return dict(reader_results=readbacks,comparisons=comparisons,configuration_coverage=coverage,
                profile_comparisons=profile_comparisons,**growth(entries))


def verify_reports(paths,output):
    require(len(set(paths))==2,"normal and optimized reports must be distinct")
    reports=[read_json(path) for path in paths]
    require([r.get("runtime_optimize") for r in reports]==[0,1],"expected normal then -O report")
    watched={str(path.resolve()) for path in paths}|{str(Path(__file__).resolve())}
    for report in reports:
        require(report["schema"]=="mhgp8_q34_affine_growth_analysis_v1" and report["status"]=="passed" and
                report["error"] is None and not report["closing_errors"] and report["native_executions"]==0,
                "analysis report did not close successfully")
        for family in ("source_sha256","input_sha256"):
            require(report[family]==report[family+"_after"]==pins(report[family]),"analysis report input closure no longer holds")
            watched.update(report[family])
    before=pins(watched)
    ignore={"argv","started_utc","finished_utc","runtime_optimize"}
    left={k:v for k,v in reports[0].items() if k not in ignore}
    right={k:v for k,v in reports[1].items() if k not in ignore}
    difference=first_difference(left,right)
    after=pins(watched)
    success=difference is None and before==after
    result=dict(schema="mhgp8_q34_affine_growth_readback_v1",status="passed" if success else "failed",
        reports=[dict(path=str(path.resolve()),sha256=before[str(path.resolve())],runtime_optimize=report["runtime_optimize"])
                 for path,report in zip(paths,reports)],normal_optimized_identical=difference is None,
        first_difference=difference,ignored_execution_metadata=sorted(ignore),native_executions=0,
        input_sha256=before,input_sha256_after=after,closing_errors=[] if before==after else ["readback inputs changed"],
        finished_utc=stamp(),full_contract_qualified=False,universal_subquadratic_claim=False,stable_time_gain_claimed=False)
    write_new(output,result)
    print(json.dumps(dict(status=result["status"],reports=len(paths),files=len(before),output=str(output)),sort_keys=True))
    return 0 if success else 1


def write_new(output,result):
    output.parent.mkdir(parents=True,exist_ok=True)
    descriptor=os.open(output,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o644)
    with os.fdopen(descriptor,"w") as stream:
        json.dump(result,stream,indent=2,sort_keys=True,allow_nan=False);stream.write("\n")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture33",type=Path,nargs="+",default=[])
    parser.add_argument("--capture32",type=Path,nargs="*",default=[])
    parser.add_argument("--verify-reports",type=Path,nargs=2)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    output=args.output.resolve()
    require(output.is_relative_to(HERE) and not output.exists(),"output must be new and inside this receipt directory")
    if args.verify_reports:
        require(not args.capture33 and not args.capture32,"report readback cannot also analyze captures")
        return verify_reports(args.verify_reports,output)
    require(args.capture33,"at least one closed tranche33 capture is required")
    captures=[(version,path.resolve()) for version,paths in ((32,args.capture32),(33,args.capture33)) for path in paths]
    require(len(captures)==len(set(captures)),"duplicate capture argument")
    files={Path(__file__).resolve()}
    for module in tuple(sys.modules.values()):
        name=getattr(module,"__file__",None)
        if name and Path(name).resolve().is_relative_to(BENCH):files.add(Path(name).resolve())
    expected_artifacts={}
    for version,path in captures:
        require(path.is_relative_to(ROOT/"morsehgp3D_v8/receipts"),"capture outside receipt tree")
        require((path/"COMPLETION.json").is_file(),"capture still open: "+str(path))
        manifest=read_json(path/"MANIFEST.json");completion=read_json(path/"COMPLETION.json")
        require(completion["status"]=="passed" and not completion["closing_errors"],"capture did not close successfully")
        files.update((path/"MANIFEST.json",path/"COMPLETION.json"))
        files.update(path/item["path"] for item in completion["records"])
        for family in ("input_sha256","artifact_sha256"):
            for name,value in manifest[family].items():
                target=(ROOT/name).resolve()
                require(str(target) not in expected_artifacts or expected_artifacts[str(target)]==value,"conflicting artifact/input pin")
                expected_artifacts[str(target)]=value;files.add(target)
    before=pins(files)
    for name,value in expected_artifacts.items():require(before[name]==value,"historical artifact/input changed: "+name)
    sources=pins(ROOT/name for name in new.SOURCES)
    result=dict(schema="mhgp8_q34_affine_growth_analysis_v1",status="failed",error=None,started_utc=stamp(),
                argv=[sys.executable,*sys.argv],runtime_optimize=sys.flags.optimize,native_executions=0,
                normalized_capacity_fields=list(NORMALIZED),
                historical_rows_reinterpreted_as_executions=False,full_contract_qualified=False,
                universal_subquadratic_claim=False,stable_time_gain_claimed=False,gcp_used=False,
                source_sha256=sources,input_sha256=before,closing_errors=[])
    try:
        result.update(analyze(captures));result["status"]="passed"
    except BaseException as cause:
        result["error"]=f"{type(cause).__name__}: {cause}"
    finally:
        try:
            result["source_sha256_after"]=pins(ROOT/name for name in new.SOURCES)
            result["input_sha256_after"]=pins(files)
            if result["source_sha256_after"]!=sources:result["closing_errors"].append("current211 sources changed")
            if result["input_sha256_after"]!=before:result["closing_errors"].append("reader/capture/input/artifact changed")
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
