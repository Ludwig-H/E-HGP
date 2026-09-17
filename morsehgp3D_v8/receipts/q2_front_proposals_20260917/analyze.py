#!/usr/bin/env python3
"""Explicit tranche19 analyzer port: closed captures of the widened front proposal window.

Every comparison pairs the Coarse reference of one configuration with the five runs of the
proposals probe recorded right after it in the same capture. A configuration captured several
times (repeated scale captures) must repeat every deterministic counter; its times are then
reported as minimum and median. The differential capture compares the default with a pinned
pre-tranche build. Times are observations on a shared host; counters carry the conclusions.
"""
import argparse
from contextlib import redirect_stdout
import io
import hashlib
import json
import math
from pathlib import Path
import statistics
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_wspd_q2_proposals_checks as reader

MAIN = {
    "front_products": ("front_work", "product_visits"),
    "front_witness_searches": ("front_work", "witness_searches"),
    "front_witness_descents": ("front_work", "witness_descent_steps"),
    "front_proposed_sites": ("front_work", "proposed_sites"),
    "front_h_bound_tests": ("front_work", "h_bound_tests"),
    "front_rejected_products": ("front_work", "fully_rejected_products"),
    "input_rectangles": ("input_rectangles",),
    "anchor_queries": ("anchor_queries",),
    "candidates": ("candidate_pairs",),
    "count_node_visits": ("census_work", "count_node_visits"),
    "cursor_advances": ("census_work", "cursor_advances"),
    "supports": ("digest", "supports"),
    "pool_factor_sites": ("pool_work", "factor_sites"),
    "pool_selection_tests": ("pool_work", "selection_tests"),
    "extended_products": ("extension_work", "extended_products"),
    "extended_proposals": ("extension_work", "extended_proposals"),
    "extended_proposals_in_factors": ("extension_work", "extended_proposals_in_factors"),
    "extended_credits": ("extension_work", "extended_credits"),
    "extended_rejections": ("extension_work", "extended_rejections"),
}


def value(row, path):
    for field in path:
        if field not in row:
            return 0  # The historical reference has no extension work.
        row = row[field]
    return row


def ratio(a, b):
    return a / b if b else None


def label(row):
    if reader.is_reference(row):
        return "reference"
    limit = row["small_factor_limit"]
    return f"{row['window_factor']}K/" + ("all" if limit is None else str(limit))


def growth(rows, metrics):
    result = {}
    for name, path in metrics.items():
        values = [value(row, path) for row in rows]
        ratios = [ratio(b, a) for a, b in zip(values, values[1:])]
        result[name] = dict(values=values, ratios=ratios,
            local_log2_exponents=[math.log2(r) if r and r > 0 else None for r in ratios],
            above_quadrupling=[a > 0 and b > 4 * a for a, b in zip(values, values[1:])],
            zero_to_positive=[a == 0 and b > 0 for a, b in zip(values, values[1:])])
    return result


def analyze(paths):
    reader.require(paths and len(paths) == len(set(paths)) and all(p.parent == HERE for p in paths),
                   "explicit distinct local capture paths required")
    captures, comparisons, growth_groups, gates, measures, differential = [], [], [], [], 0, []
    for path in paths:
        with redirect_stdout(io.StringIO()):
            reader.read(path)
        manifest = reader.strict_json((path / "MANIFEST.json").read_text())
        completion = reader.strict_json((path / "COMPLETION.json").read_text())
        records = [reader.strict_json((path / info["path"]).read_text()) for info in completion["records"]]
        rows = [record["row"] for record in records if "row" in record]
        source_fingerprint = hashlib.sha256(json.dumps(manifest["source_sha256"], sort_keys=True,
                                                       separators=(",", ":")).encode()).hexdigest()
        captures.append(dict(path=str(path.relative_to(ROOT)), campaign=manifest["campaign"],
                             affinity=manifest["affinity"], source_fingerprint=source_fingerprint,
                             records=len(records), measures=len(rows),
                             manifest_sha256=reader.digest(path / "MANIFEST.json")))
        gates.extend(dict(capture=str(path.relative_to(ROOT)), gate=reader.strict_json(record["stdout"]))
                     for record in records if record["kind"] == "gate")
        measures += len(rows)
        if manifest["campaign"] == "differential":
            reader.require(len(rows) % 3 == 0, "incomplete differential triples")
            for offset in range(0, len(rows), 3):
                pinned, reference, default = rows[offset:offset + 3]
                differential.append(dict(family=pinned["family"], n=pinned["n"], kmax=pinned["kmax"], s=pinned["s"],
                    pinned_ms=pinned["timings"]["pipeline_wall_ms"], reference_ms=reference["timings"]["pipeline_wall_ms"],
                    default_ms=default["timings"]["pipeline_wall_ms"],
                    reference_over_pinned=ratio(reference["timings"]["pipeline_wall_ms"], pinned["timings"]["pipeline_wall_ms"]),
                    default_over_pinned=ratio(default["timings"]["pipeline_wall_ms"], pinned["timings"]["pipeline_wall_ms"]),
                    discrete_equal=all(pinned[k] == reference[k] == default[k] for k in
                                       ("digest", "candidate_pairs", "accepted_pairs", "front_work", "census_work"))))
            continue
        width = 1 + len(reader.VARIANTS)
        reader.require(len(rows) % width == 0, "incomplete paired observation group")
        for offset in range(0, len(rows), width):
            reference, *candidates = rows[offset:offset + width]
            reader.require(reader.is_reference(reference) and
                           [(str(r["window_factor"]), "all" if r["small_factor_limit"] is None
                             else str(r["small_factor_limit"])) for r in candidates] == list(reader.VARIANTS),
                           "unexpected paired configuration order")
            for candidate in candidates:
                reader.require(all(reference[k] == candidate[k] for k in
                                   ("n", "family", "kmax", "s", "seed", "threads", "jobs_per_worker",
                                    "pool_min_factor", "digest", "accepted_pairs")),
                               "comparison changed more than the proposal window")
                old, new = reference["timings"]["pipeline_wall_ms"], candidate["timings"]["pipeline_wall_ms"]
                comparisons.append(dict(campaign=manifest["campaign"], family=reference["family"], n=reference["n"],
                    kmax=reference["kmax"], s=reference["s"], workers=reference["threads"], variant=label(candidate),
                    reference_ms=old, variant_ms=new, variant_over_reference=ratio(new, old),
                    metrics={name: dict(reference=value(reference, p), variant=value(candidate, p),
                                        ratio=ratio(value(candidate, p), value(reference, p)))
                             for name, p in MAIN.items()},
                    extension_work=candidate["extension_work"]))
        if manifest["campaign"] != "scale" or any(g["capture"] != str(path.relative_to(ROOT)) for g in growth_groups):
            continue  # Growth is deterministic: the first scale capture carries it.
        for variant in ("reference", *("%sK/%s" % v for v in reader.VARIANTS)):
            for family in reader.FAMILIES:
                selected = sorted([r for r in rows if label(r) == variant and r["family"] == family],
                                  key=lambda r: r["n"])
                for kmax in sorted({r["kmax"] for r in selected}):
                    series = [r for r in selected if r["kmax"] == kmax]
                    reader.require([r["n"] for r in series] == [8000, 16000, 32000], "growth grid incomplete")
                    growth_groups.append(dict(capture=str(path.relative_to(ROOT)), family=family, variant=variant,
                                              kmax=kmax, s=series[0]["s"], workers=series[0]["threads"],
                                              geometry=growth(series, MAIN)))
    # Repeated captures of one configuration: identical counters required, times aggregated.
    merged = {}
    for c in comparisons:
        key = (c["campaign"], c["family"], c["n"], c["kmax"], c["s"], c["workers"], c["variant"])
        if key not in merged:
            merged[key] = dict(c, reference_ms_all=[c["reference_ms"]], variant_ms_all=[c["variant_ms"]])
            continue
        kept = merged[key]
        reader.require(kept["metrics"] == c["metrics"] and kept["extension_work"] == c["extension_work"],
                       "a repeated capture changed a deterministic counter")
        kept["reference_ms_all"].append(c["reference_ms"])
        kept["variant_ms_all"].append(c["variant_ms"])
    comparisons = []
    for kept in merged.values():
        kept["repeats"] = len(kept["variant_ms_all"])
        kept["reference_ms"], kept["variant_ms"] = min(kept["reference_ms_all"]), min(kept["variant_ms_all"])
        kept["reference_ms_median"] = statistics.median(kept["reference_ms_all"])
        kept["variant_ms_median"] = statistics.median(kept["variant_ms_all"])
        kept["variant_over_reference"] = ratio(kept["variant_ms"], kept["reference_ms"])
        kept["variant_over_reference_median"] = ratio(kept["variant_ms_median"], kept["reference_ms_median"])
        comparisons.append(kept)
    large = [c for c in comparisons if c["n"] >= 8000]
    summary = {}
    for variant in sorted({c["variant"] for c in large}):
        for family in (*reader.FAMILIES, "all"):
            speeds = [c["variant_over_reference"] for c in large
                      if c["variant"] == variant and family in (c["family"], "all")]
            candidates = [c["metrics"]["candidates"]["ratio"] for c in large
                          if c["variant"] == variant and family in (c["family"], "all")]
            summary[f"{variant}:{family}"] = dict(comparisons=len(speeds),
                time_min=min(speeds, default=None), time_median=statistics.median(speeds) if speeds else None,
                time_max=max(speeds, default=None), slower_than_reference=sum(s > 1 for s in speeds),
                candidates_min=min(candidates, default=None), candidates_max=max(candidates, default=None))
    return dict(status="passed", scope="q2_all_cloud_supports_not_full", full_contract_qualified=False,
        gcp_used=False, captures=captures, gates=gates, measures=measures, paired_comparisons=len(comparisons),
        all_supports_equal=True, general_subquadratic_bound=False,
        timing_scope="minimum and median over repeated captures where repeated; shared host; speed not a G4 contract",
        differential=differential,
        summary_n_at_least_8000=summary, timing_comparisons=comparisons, growth=growth_groups)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("captures", type=Path, nargs="+")
    args = parser.parse_args()
    print(json.dumps(analyze([p.resolve() for p in args.captures]), sort_keys=True, allow_nan=False))
