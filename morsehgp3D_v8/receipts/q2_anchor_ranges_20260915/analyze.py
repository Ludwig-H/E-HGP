#!/usr/bin/env python3
"""Explicit tranche17 analyzer port: closed anchor-range captures, no inherited qualification."""
import argparse
from contextlib import redirect_stdout
import io
import json
import math
from pathlib import Path
import statistics
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_wspd_q2_ranges_checks as reader

MAIN = {
    "front_products": ("front_work", "product_visits"),
    "front_witness_descents": ("front_work", "witness_descent_steps"),
    "candidates": ("candidate_pairs",),
    "count_node_visits": ("census_work", "count_node_visits"),
    "order_structural_splits": ("order_work", "structural_splits"),
    "supports": ("digest", "supports"),
    "pool_factor_sites": ("pool_work", "factor_sites"),
    "pool_selection_tests": ("pool_work", "selection_tests"),
    "pool_witness_attempts": ("pool_work", "witness_attempts"),
}


def value(row, path):
    for field in path:
        row = row[field]
    return row


def ratio(a, b):
    return a / b if b else None


def growth(rows, metrics):
    result = {}
    for name, path in metrics.items():
        values = [value(row, path) for row in rows]
        ratios = [ratio(b, a) for a, b in zip(values, values[1:])]
        result[name] = dict(values=values, ratios=ratios,
            local_log2_exponents=[math.log2(r) if r and r > 0 else None for r in ratios],
            above_quadrupling=[a > 0 and b > 4*a for a, b in zip(values, values[1:])],
            zero_to_positive=[a == 0 and b > 0 for a, b in zip(values, values[1:])])
    return result


def analyze(paths):
    captures, measures, timing_comparisons, growth_groups = [], [], [], []
    identities, signatures = {}, {}
    for path in paths:
        sink = io.StringIO()
        with redirect_stdout(sink):
            reader.read(path)
        manifest = reader.strict_json((path / "MANIFEST.json").read_text())
        completion = reader.strict_json((path / "COMPLETION.json").read_text())
        rows = [reader.strict_json((path / info["path"]).read_text())["row"]
                for info in completion["records"]
                if "row" in reader.strict_json((path / info["path"]).read_text())]
        captures.append(dict(path=str(path.relative_to(ROOT)), campaign=manifest["campaign"],
                             affinity=manifest["affinity"], records=len(completion["records"]),
                             measures=len(rows), manifest_sha256=reader.digest(path / "MANIFEST.json")))
        measures.extend(rows)
        reader.validate_pairs(rows)
        for row in rows:
            reader.cross_check(row, identities, signatures)
        for offset in range(0, len(rows), 3):
            ref, one, many = rows[offset:offset + 3]
            reader.require(ref["execution"] == "parallel_front" and one["threads"] == 1 and
                           many["threads"] == 4, "unexpected paired sequence")
            old, serial, parallel = (r["timings"]["pipeline_wall_ms"] for r in (ref, one, many))
            timing_comparisons.append(dict(campaign=manifest["campaign"], family=ref["family"], n=ref["n"],
                kmax=ref["kmax"], s=ref["s"], pool=ref["pool_min_factor"],
                coarse_w4_ms=old, ranges_w1_ms=serial, ranges_w4_ms=parallel,
                old4_over_new4=ratio(old, parallel), new1_over_new4=ratio(serial, parallel),
                initial_anchors=many["range_work"]["initial_anchors"],
                live_pool_parents=many["range_storage"]["max_live_pool_parents"],
                live_pool_bytes=many["range_storage"]["max_live_pool_bytes"],
                donations=many["range_work"]["donations"],
                donations_after_seeds_claimed=many["range_work"]["donations_after_seeds_exhausted"]))
        if manifest["campaign"] not in ("scale", "rows_large"):
            continue
        keys = sorted({(r["family"], r["kmax"], r["s"], r["pool_min_factor"], r["threads"])
                       for r in rows if r["execution"] == "anchor_ranges_front_census"})
        for key in keys:
            selected = sorted([r for r in rows if r["execution"] == "anchor_ranges_front_census" and
                (r["family"], r["kmax"], r["s"], r["pool_min_factor"], r["threads"]) == key], key=lambda r: r["n"])
            reader.require([r["n"] for r in selected] == [8000, 16000, 32000], "growth grid incomplete")
            extra = {name: ("range_work", name) for name in reader.RANGE_FIELDS}
            growth_groups.append(dict(campaign=manifest["campaign"], family=key[0], kmax=key[1], s=key[2],
                pool=key[3], workers=key[4], geometry=growth(selected, MAIN),
                pool_all_counters=growth(selected, {name: ("pool_work", name) for name in reader.POOL_FIELDS}),
                census_operations=growth(selected, {name: ("census_work", name)
                    for name in selected[0]["census_work"] if name != "consumed_witness_sites"}),
                sibling_operations=growth(selected, {name: ("sibling_work", name)
                    for name in selected[0]["sibling_work"]}),
                order_operations=growth(selected, {name: ("order_work", name)
                    for name in selected[0]["order_work"]}),
                population_accounting=growth(selected, {"consumed_witness_sites": ("census_work", "consumed_witness_sites")}),
                scheduling=growth(selected, extra)))
    cooperative = [r for r in measures if r["execution"] == "anchor_ranges_front_census"]
    speeds = [r["old4_over_new4"] for r in timing_comparisons if r["n"] >= 8000]
    return dict(status="passed", scope="q2_all_cloud_supports_not_full", full_contract_qualified=False,
        gcp_used=False, captures=captures, measures=len(measures), range_measures=len(cooperative),
        reference_measures=len(measures)-len(cooperative), paired_comparisons=len(timing_comparisons),
        all_geometric_work_and_payloads_equal=True, general_subquadratic_bound=False,
        timing_scope="single paired observations; speed not qualified by repetitions",
        speed_summary=dict(excludes_smoke=True, comparisons=len(speeds),
                           min=min(speeds, default=None), median=statistics.median(speeds) if speeds else None,
                           max=max(speeds, default=None), above_one=sum(r > 1 for r in speeds)),
        donations=sum(r["range_work"]["donations"] for r in cooperative),
        donations_by_type={name: sum(r["range_work"][name] for r in cooperative)
                           for name in ("shared_donations", "pool_donations", "passthrough_donations")},
        after_seeds_claimed=sum(r["range_work"]["donations_after_seeds_exhausted"] for r in cooperative),
        timing_comparisons=timing_comparisons, growth=growth_groups)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("captures", type=Path, nargs="+")
    args = parser.parse_args()
    print(json.dumps(analyze([p.resolve() for p in args.captures]), sort_keys=True, allow_nan=False))
