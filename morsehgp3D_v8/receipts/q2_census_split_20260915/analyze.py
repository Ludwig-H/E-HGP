#!/usr/bin/env python3
"""Read-only, deterministic analysis of tranche16's closed CPU receipts.

No benchmark, rebuild, receipt mutation or historical requalification occurs.
Exit0: all requested captures closed and checked; 2: a completion is missing;
1: invalid data. stdout is one JSON object, identical under Python normal/-O.
"""

from __future__ import annotations

import argparse
import base64
from contextlib import redirect_stdout
from hashlib import sha256
import io
import itertools
import json
import math
from pathlib import Path
import statistics
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BENCH = ROOT / "morsehgp3D_v8/bench"
sys.dont_write_bytecode = True
sys.path.insert(0, str(BENCH))
import run_q2_split_checks as split_reader
import run_wspd_q2_parallel_matrix as full_reader

SIZES = (8000, 16000, 32000)
FAMILIES = ("uniform", "terrain", "clusters", "rows")
STEP_FIELDS = ("entry_steps", "witness_steps", "admission_steps", "payload_steps")
FULL_METRICS = {
    "front_products": ("front_work", "product_visits"),
    "front_witness_descents": ("front_work", "witness_descent_steps"),
    "candidates": ("candidate_pairs",),
    "count_node_visits": ("census_work", "count_node_visits"),
    "order_structural_splits": ("order_work", "structural_splits"),
    "supports": ("digest", "supports"),
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return sha256(path.read_bytes()).hexdigest()


def canonical_hash(value):
    return sha256(json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False).encode()).hexdigest()


def load(path):
    return json.loads(path.read_bytes())


def relative(path):
    path = path.resolve()
    return str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)


def read_call(function, *args):
    capture = io.StringIO()
    with redirect_stdout(capture):
        status = function(*args)
    require(status in (None, 0), "receipt reader returned failure")
    return json.loads(capture.getvalue())


def ratio(numerator, denominator):
    return None if denominator == 0 else numerator / denominator


def growth(values):
    changes = []
    for i, (before, after) in enumerate(zip(values, values[1:])):
        r = ratio(after, before)
        changes.append(dict(n_from=SIZES[i], n_to=SIZES[i + 1], ratio=r,
            local_log2_exponent=math.log2(r) if r is not None and r > 0 else None,
            above_quadrupling=before > 0 and after > 4 * before,
            from_zero_to_positive=before == 0 and after > 0))
    return dict(values=values, changes=changes)


def path_value(row, path):
    for key in path:
        row = row[key]
    return row


def raw_capture(path, historical=False):
    """Check old raw/decoded records and captured closures, not current sources.

    This is deliberately NOT the current full reader with checks bypassed.
    Current captures additionally pass that reader separately below.
    """
    files = ("MANIFEST.json", "COMPLETION.json", "MEASURES.jsonl")
    pins = {name: digest(path / name) for name in files}
    manifest, completion = load(path / files[0]), load(path / files[1])
    require(manifest["schema"] == full_reader.SCHEMA and manifest["scope"] == full_reader.SCOPE and
            manifest["public_status"] == "not_claimed" and manifest["gcp_used"] is False,
            "full-q2 capture changed its declared scope")
    require(completion["status"] == "completed" and completion["source_hashes_unchanged"] is True and
            completion["probe_hash_unchanged"] is True and
            manifest["source_sha256"] == completion["source_sha256_closing"] and
            manifest["probe_sha256"] == completion["probe_sha256_closing"], "full-q2 captured closure is invalid")
    records = [json.loads(line) for line in (path / files[2]).read_bytes().splitlines()]
    require(completion["runs"] == completion["attempts"] == len(records) == 12,
            "full-q2 regression must contain exactly12 successful records")
    rows = []
    for record in records:
        raw = base64.b64decode(record["stdout_base64"], validate=True)
        row = json.loads(raw)
        require(record["status"] == "completed" and record["exit_code"] == 0 and
                record["probe_sha256_before"] == record["probe_sha256_after"] == manifest["probe_sha256"] and
                record["stderr"] == record["stderr_base64"] == "" and
                raw.decode() == record["stdout"] and row == record["result"], "historical/raw record mismatch")
        key = tuple(row[name] for name in full_reader.KEYS)
        require(record["command"] == full_reader.command_for(manifest["probe"], key) and record["repeat"] == 0,
                "full-q2 raw command/configuration mismatch")
        rows.append(row)
    expected = {(family, n, 10, 8, 3, 4, 16, 64) for family, n in itertools.product(FAMILIES, SIZES)}
    keys = [tuple(row[key] for key in full_reader.KEYS) for row in rows]
    require(len(set(keys)) == len(keys) and set(keys) == expected, "full-q2 regression tuple coverage changed")
    require({name: digest(path / name) for name in files} == pins, "capture changed while raw bytes were read")
    return rows, dict(path=relative(path), raw_file_sha256=pins, rows=len(rows),
        authority="historical_raw_and_declared_closure_only_not_requalified" if historical else "current_reader_plus_raw_and_closure",
        captured_source_closure_sha256=canonical_hash(manifest["source_sha256"]),
        captured_source_files=len(manifest["source_sha256"]), captured_probe_sha256=manifest["probe_sha256"],
        cpu_affinity=manifest["cpu_affinity"], repeats=manifest["repeats"])


def compare_full(current_path, historical_path):
    reader_verdict = read_call(full_reader.check, argparse.Namespace(receipt=current_path, summary=False))
    current, current_provenance = raw_capture(current_path)
    historical, historical_provenance = raw_capture(historical_path, historical=True)
    indexed = {tuple(row[key] for key in full_reader.KEYS): row for row in historical}
    comparisons = []
    for row in sorted(current, key=lambda item: tuple(item[key] for key in full_reader.KEYS)):
        key = tuple(row[field] for field in full_reader.KEYS)
        prior = indexed[key]
        differences = [field for field in full_reader.DISCRETE_FIELDS if row[field] != prior[field]]
        differences += ["pool_work." + field for field in full_reader.POOL_FIELDS
                        if row["pool_work"][field] != prior["pool_work"][field]]
        require(not differences, "full-q2 historical differential failed: " + repr((key, differences)))
        comparisons.append(dict(zip(full_reader.KEYS, key), discrete_equal=True,
            current_total_ms=row["timings"]["total_ms"], historical_total_ms=prior["timings"]["total_ms"],
            historical_over_current_wall_ratio=ratio(prior["timings"]["total_ms"], row["timings"]["total_ms"]),
            metrics={name: path_value(row, path) for name, path in FULL_METRICS.items()}))
    sequences = []
    for family in FAMILIES:
        rows = sorted((row for row in current if row["family"] == family), key=lambda row: row["n"])
        require(tuple(row["n"] for row in rows) == SIZES, "full-q2 growth size coverage changed")
        sequences.append(dict(family=family, kmax=10, s=8, threads=4, pool_min_factor=64, n=list(SIZES),
            metrics={name: growth([path_value(row, path) for row in rows]) for name, path in FULL_METRICS.items()}))
    return current, dict(reader=reader_verdict, current=current_provenance, historical=historical_provenance,
        checked_discrete_fields=list(full_reader.DISCRETE_FIELDS), checked_pool_counters=list(full_reader.POOL_FIELDS),
        equal_pairs=len(comparisons), comparisons=comparisons, growth=sequences,
        timing_interpretation="descriptive_only_one_repeat_concurrent_external_loads_not_speedup_qualification",
        scope="q2_all_cloud_supports_not_full_no_detached_census_routing_in_this_regression")


def matrix_rows(path):
    manifest_pin, completion_pin = digest(path / "MANIFEST.json"), digest(path / "COMPLETION.json")
    verdict = read_call(split_reader.read, path)
    manifest, completion = load(path / "MANIFEST.json"), load(path / "COMPLETION.json")
    require(manifest["matrix"] is True, "selected-root input is not a matrix capture")
    require(split_reader.source_pins() == manifest["source_sha256"],
            "matrix's pinned source closure differs from current source content")
    records = []
    rows = []
    for item in completion["records"]:
        require(Path(item["path"]).name == item["path"] and digest(path / item["path"]) == item["sha256"],
                "matrix record path/hash mismatch after reader")
        record = load(path / item["path"])
        rows.append(record["row"])
        records.append(dict(path=item["path"], sha256=item["sha256"]))
    require(digest(path / "MANIFEST.json") == manifest_pin and digest(path / "COMPLETION.json") == completion_pin,
            "matrix manifest/completion changed during reading")
    return rows, dict(reader=verdict, path=relative(path), manifest_sha256=manifest_pin,
        completion_sha256=completion_pin, record_sha256=records,
        captured_source_closure_sha256=canonical_hash(manifest["source_sha256"]),
        captured_source_files=len(manifest["source_sha256"]), current_source_closure_equal=True,
        captured_artifact_sha256=manifest["artifact_sha256"],
        cpu_affinity=manifest["affinity"], repeats=1)


def analyze_matrix(path, full_rows):
    rows, provenance = matrix_rows(path)
    key_fields = ("family", "n", "kmax", "separation_s", "seed", "quantum", "queue")
    grouped = {}
    for row in rows:
        key = tuple(row[field] for field in key_fields)
        group = grouped.setdefault(key, {})
        require(row["workers"] not in group, "duplicate worker count within a selected-root pair")
        group[row["workers"]] = row
    require(len(grouped) == 72, "selected-root matrix must contain72 worker pairs")
    pair_records, serial = [], []
    nonvacuity = dict(worker4_runs=0, positive_donations=0, multiple_workers_with_positive_work=0,
        nonempty_aggregate_output=0, multi_worker_work_and_nonempty_aggregate_output=0,
        zero_output_pairs=0, all_work_on_one_worker=0)
    for key, group in sorted(grouped.items()):
        require(set(group) == {1, 4}, "selected-root pair lacks W1 or W4")
        one, four = group[1], group[4]
        for field in ("input_hash", "anchor_rank", "a_node", "b_node", "candidates", "accepted", "rejected",
                      "front_products", "front_rectangles", "geometry", "hash_sum", "hash_xor"):
            require(one[field] == four[field], "worker count changed selected root/work/payload: " + field)
        require(len(one["geometry"]) == 36, "expected all36 exact geometric fields")
        for field in (*STEP_FIELDS, "transitions"):
            require(one["resume"][field] == four["resume"][field], "worker count replayed a declared transition")
        active = sum(value > 0 for value in four["worker_transitions"])
        nonvacuity["worker4_runs"] += 1
        nonvacuity["positive_donations"] += four["schedule"]["donations"] > 0
        nonvacuity["multiple_workers_with_positive_work"] += active > 1
        nonvacuity["nonempty_aggregate_output"] += four["accepted"] > 0
        nonvacuity["multi_worker_work_and_nonempty_aggregate_output"] += active > 1 and four["accepted"] > 0
        nonvacuity["zero_output_pairs"] += four["accepted"] == 0
        nonvacuity["all_work_on_one_worker"] += active == 1
        pair_records.append(dict(zip(key_fields, key), exact_geometry36=True, exact_steps4_and_total=True,
            selected_anchor_rank=one["anchor_rank"], selected_a_node=one["a_node"], selected_b_node=one["b_node"],
            candidates=one["candidates"], accepted=one["accepted"], rejected=one["rejected"],
            input_hash=one["input_hash"], hash_sum=one["hash_sum"], hash_xor=one["hash_xor"],
            one_worker_ms=one["timings_ms"]["parallel_enclosing"], four_workers_ms=four["timings_ms"]["parallel_enclosing"],
            descriptive_wall_speedup_1_over_4=ratio(one["timings_ms"]["parallel_enclosing"], four["timings_ms"]["parallel_enclosing"]),
            prepare_ms_w1=one["timings_ms"]["prepare"], front_ms_w1=one["timings_ms"]["front"],
            advance_sum_ms_w4=four["timings_ms"]["advance_sum"], payload_sum_ms_w4=four["timings_ms"]["payload_sum"],
            worker_transitions_w4=four["worker_transitions"], workers_with_positive_work=active,
            largest_worker_fraction=ratio(max(four["worker_transitions"]), four["resume"]["transitions"]),
            detach_w4=four["detach"], schedule_w4=four["schedule"],
            transferred_pair_mass_over_candidates=ratio(four["detach"]["transferred_pairs"], four["candidates"]),
            queue_storage_bytes_w4=four["queue_storage_bytes"], max_fragment_bytes_w1=one["max_fragment_bytes"],
            max_fragment_bytes_w4=four["max_fragment_bytes"]))
        serial.append(one)
    growth_groups, above, from_zero = [], [], []
    for family, k, separation in itertools.product(FAMILIES, (5, 10), (8, 10, 12)):
        sequence = sorted((row for row in serial if (row["family"], row["kmax"], row["separation_s"]) ==
                           (family, k, separation)), key=lambda row: row["n"])
        require(tuple(row["n"] for row in sequence) == SIZES, "selected-root growth size coverage changed")
        fields = {name: [row[name] for row in sequence] for name in
                  ("front_products", "front_rectangles", "candidates", "accepted", "rejected")}
        fields.update({"geometry." + name: [row["geometry"][name] for row in sequence] for name in sorted(sequence[0]["geometry"])})
        fields.update({"resume." + name: [row["resume"][name] for row in sequence] for name in (*STEP_FIELDS, "transitions")})
        metrics = {name: growth(values) for name, values in sorted(fields.items())}
        for name, metric in metrics.items():
            for change in metric["changes"]:
                entry = dict(family=family, kmax=k, separation_s=separation, metric=name, **change)
                if change["above_quadrupling"]: above.append(entry)
                if change["from_zero_to_positive"]: from_zero.append(entry)
        growth_groups.append(dict(family=family, kmax=k, separation_s=separation, n=list(SIZES),
            selected_roots=[dict(n=row["n"], anchor_rank=row["anchor_rank"], a_node=row["a_node"], b_node=row["b_node"])
                            for row in sequence], metrics=metrics))
    # Only entry identity and front accounting are compared across scopes;
    # the selected anchor's residual is NOT the all-cloud q2 residual.
    cross_scope = 0
    for full in full_rows:
        selected = next(row for row in serial if (row["family"], row["n"], row["kmax"], row["separation_s"])
                        == (full["family"], full["n"], full["kmax"], full["s"]))
        require(selected["input_hash"] == int(full["input_hash"], 16) and
                selected["front_products"] == full["front_work"]["product_visits"] and
                selected["front_rectangles"] == full["front_work"]["emitted_rectangles"],
                "selected/all-cloud probes changed their common input or front accounting")
        cross_scope += 1
    speeds = [item["descriptive_wall_speedup_1_over_4"] for item in pair_records]
    require(all(value is not None for value in speeds), "zero elapsed interval prevents descriptive timing ratio")
    return dict(provenance=provenance, equal_worker_pairs=len(pair_records), checked_geometry_fields=sorted(rows[0]["geometry"]),
        checked_step_fields=list(STEP_FIELDS), total_transitions_also_equal=True, pairs=pair_records,
        growth=growth_groups, above_quadrupling=above, from_zero_to_positive=from_zero,
        nonvacuity=nonvacuity, common_input_front_checks=cross_scope,
        descriptive_wall_speedup=dict(min=min(speeds), median=statistics.median(speeds), max=max(speeds)),
        caveats=["one selected anchor; selected root may change with n, K or s",
                 "local growth ratios are observations, not a global complexity bound",
                 "single repetition and concurrent external load; wall ratios are not a speedup qualification",
                 "aggregate nonempty output plus multiple working slots does not prove multiple emitting slots",
                 "transferred pair mass is traffic; recursive transfers can count the same pair repeatedly",
                 "max fragment capacity is neither total resident memory nor RSS; worker time sums are not wall time"])


def implementation_pins():
    paths = {Path(__file__).resolve()}
    for module in list(sys.modules.values()):
        file = getattr(module, "__file__", None)
        if file:
            path = Path(file).resolve()
            if path.is_relative_to(BENCH) and path.suffix == ".py":
                paths.add(path)
    return {relative(path): digest(path) for path in sorted(paths)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--matrix", type=Path, default=HERE / "matrix_ocf7f2oa")
    parser.add_argument("--full-current", type=Path, default=HERE / "full_q2_regression")
    parser.add_argument("--full-historical", type=Path, default=HERE.parent / "q2_census_resume_20260914/full_q2_regression")
    args = parser.parse_args()
    pins = implementation_pins()
    result = dict(schema="mhgp8_q2_census_split_analysis_v1", status="incomplete", public_status="not_claimed",
                  full_contract_qualified=False, gcp_used=False, global_subquadratic_bound_proved=False,
                  analysis_source_sha256=pins)
    missing = [relative(path / "COMPLETION.json") for path in (args.matrix, args.full_current, args.full_historical)
               if not (path / "COMPLETION.json").is_file()]
    if missing:
        result["missing_completions"] = missing
        result["partial_rows_analyzed"] = False
        result["available_manifest_sha256"] = {relative(path / "MANIFEST.json"): digest(path / "MANIFEST.json")
            for path in (args.matrix, args.full_current, args.full_historical) if (path / "MANIFEST.json").is_file()}
        code = 2
    else:
        full_rows, result["full_q2_regression"] = compare_full(args.full_current, args.full_historical)
        result["selected_anchor_matrix"] = analyze_matrix(args.matrix, full_rows)
        result["status"] = "passed"
        code = 0
    require(implementation_pins() == pins, "analysis/reader sources changed during reading")
    print(json.dumps(result, sort_keys=True, indent=2, allow_nan=False))
    return code


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(json.dumps(dict(schema="mhgp8_q2_census_split_analysis_v1", status="invalid",
            public_status="not_claimed", full_contract_qualified=False,
            error=type(error).__name__ + ": " + str(error)), sort_keys=True, allow_nan=False))
        raise SystemExit(1)
