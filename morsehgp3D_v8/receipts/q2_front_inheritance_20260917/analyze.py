#!/usr/bin/env python3
"""Explicit tranche20 analyzer port: closed captures of the inherited front witnesses.

Every comparison pairs the Coarse reference of one configuration with the six runs of the
inheritance probe recorded right after it in the same capture: three proposal windows, each
plain then inheriting. An inheriting run is also compared with its plain TWIN, the run of
the same window recorded just before it: that ratio isolates the inheritance. A configuration
captured several times (repeated scale captures) must repeat every deterministic counter; its
times are then reported as minimum and median. The differential capture compares this engine,
inheritance off, with the pinned tranche-20 build, for the historical and the widened window.
Times are observations on a shared host; counters carry the conclusions.
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
import run_wspd_q2_inheritance_checks as reader

# A closed scale capture of the previous tranche: it records the SHA256 of the probes that the
# differential campaign of this tranche uses as its pinned, external reference.
PINNED_MANIFEST = "morsehgp3D_v8/receipts/q2_front_proposals_20260917/scale_wgblrm2_/MANIFEST.json"

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
    "front_rejected_pair_mass": ("front_work", "rejected_pair_mass", 0),
    "inherited_credits": ("inheritance_work", "inherited_credits"),
    "inherited_duplicates": ("inheritance_work", "inherited_duplicates"),
    "extended_inherited_duplicates": ("inheritance_work", "extended_inherited_duplicates"),
    "inherited_rejections": ("inheritance_work", "inherited_rejections"),
    "emitted_witness_credits": ("inheritance_work", "emitted_witness_credits"),
}


def value(row, path):
    for field in path:
        if type(field) is int:
            row = row[field]
            continue
        if field not in row:
            return 0  # The historical reference has neither extension nor inheritance work.
        row = row[field]
    return row


def ratio(a, b):
    return a / b if b else None


def label(row):
    if reader.is_reference(row):
        return "reference"
    limit = row["small_factor_limit"]
    return f"{row['window_factor']}K/" + ("all" if limit is None else str(limit)) + \
        ("+inherit" if row["inherit_witnesses"] else "")


def variant_label(variant):
    factor, limit, inherit = variant
    return f"{factor}K/{limit}" + ("+inherit" if inherit == "1" else "")


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
            reader.require(len(rows) % 7 == 0, "incomplete differential septuplets")
            # The pinned probes must be the very binaries that the tranche-20 receipts recorded.
            previous = reader.strict_json((ROOT / PINNED_MANIFEST).read_text())["artifact_sha256"]
            for name, pin in manifest["pinned_artifact_sha256"].items():
                reader.require(Path(name).name == "CMakeCache.txt" or previous.get(name) == pin,
                               "pinned probe is not the binary recorded by the tranche-20 receipts: " + name)
            ms = lambda row: row["timings"]["pipeline_wall_ms"]
            keys = ("digest", "candidate_pairs", "accepted_pairs", "front_work", "census_work")
            for offset in range(0, len(rows), 7):
                pinned, reference, default, pinned_2k, widened_2k, pinned_4k, widened_4k = rows[offset:offset + 7]
                differential.append(dict(family=pinned["family"], n=pinned["n"], kmax=pinned["kmax"], s=pinned["s"],
                    pinned_ms=ms(pinned), reference_ms=ms(reference), default_ms=ms(default),
                    pinned_2k_ms=ms(pinned_2k), widened_2k_ms=ms(widened_2k),
                    pinned_4k_ms=ms(pinned_4k), widened_4k_ms=ms(widened_4k),
                    reference_over_pinned=ratio(ms(reference), ms(pinned)),
                    default_over_pinned=ratio(ms(default), ms(pinned)),
                    widened_2k_over_pinned=ratio(ms(widened_2k), ms(pinned_2k)),
                    widened_4k_over_pinned=ratio(ms(widened_4k), ms(pinned_4k)),
                    job_storage_bytes=dict(pinned=pinned["parallel_work"]["job_storage_bytes"],
                                           default=default["parallel_work"]["job_storage_bytes"]),
                    discrete_equal=all(pinned[k] == reference[k] == default[k] for k in keys) and
                                   all(a[k] == b[k] for a, b in ((pinned_2k, widened_2k), (pinned_4k, widened_4k))
                                       for k in (*keys, "extension_work"))))
            continue
        width = 1 + len(reader.VARIANTS)
        reader.require(len(rows) % width == 0, "incomplete paired observation group")
        for offset in range(0, len(rows), width):
            reference, *candidates = rows[offset:offset + width]
            reader.require(reader.is_reference(reference) and
                           [(str(r["window_factor"]), "all" if r["small_factor_limit"] is None
                             else str(r["small_factor_limit"]), "1" if r["inherit_witnesses"] else "0")
                            for r in candidates] == list(reader.VARIANTS),
                           "unexpected paired configuration order")
            twins = {(r["window_factor"], r["small_factor_limit"]): r for r in candidates if not r["inherit_witnesses"]}
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
                    extension_work=candidate["extension_work"], inheritance_work=candidate["inheritance_work"]))
                if candidate["inherit_witnesses"]:
                    twin = twins[(candidate["window_factor"], candidate["small_factor_limit"])]
                    comparisons[-1].update(twin_ms=twin["timings"]["pipeline_wall_ms"],
                        candidates_over_twin=ratio(candidate["candidate_pairs"], twin["candidate_pairs"]),
                        products_over_twin=ratio(candidate["front_work"]["product_visits"], twin["front_work"]["product_visits"]),
                        h_tests_over_twin=ratio(candidate["front_work"]["h_bound_tests"], twin["front_work"]["h_bound_tests"]),
                        count_node_visits_over_twin=ratio(candidate["census_work"]["count_node_visits"],
                                                          twin["census_work"]["count_node_visits"]))
        if manifest["campaign"] != "scale" or any(g["capture"] != str(path.relative_to(ROOT)) for g in growth_groups):
            continue  # Growth is deterministic: the first scale capture carries it.
        for variant in ("reference", *map(variant_label, reader.VARIANTS)):
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
            merged[key] = dict(c, reference_ms_all=[c["reference_ms"]], variant_ms_all=[c["variant_ms"]],
                               twin_ms_all=[c["twin_ms"]] if "twin_ms" in c else [])
            continue
        kept = merged[key]
        reader.require(kept["metrics"] == c["metrics"] and kept["extension_work"] == c["extension_work"] and
                       kept["inheritance_work"] == c["inheritance_work"] and
                       kept.get("candidates_over_twin") == c.get("candidates_over_twin"),
                       "a repeated capture changed a deterministic counter")
        kept["reference_ms_all"].append(c["reference_ms"])
        kept["variant_ms_all"].append(c["variant_ms"])
        if "twin_ms" in c:
            kept["twin_ms_all"].append(c["twin_ms"])
    comparisons = []
    for kept in merged.values():
        kept["repeats"] = len(kept["variant_ms_all"])
        kept["reference_ms"], kept["variant_ms"] = min(kept["reference_ms_all"]), min(kept["variant_ms_all"])
        kept["reference_ms_median"] = statistics.median(kept["reference_ms_all"])
        kept["variant_ms_median"] = statistics.median(kept["variant_ms_all"])
        kept["variant_over_reference"] = ratio(kept["variant_ms"], kept["reference_ms"])
        kept["variant_over_reference_median"] = ratio(kept["variant_ms_median"], kept["reference_ms_median"])
        if kept["twin_ms_all"]:
            kept["twin_ms"] = min(kept["twin_ms_all"])
            kept["variant_over_twin"] = ratio(kept["variant_ms"], kept["twin_ms"])
            kept["variant_over_twin_median"] = ratio(kept["variant_ms_median"], statistics.median(kept["twin_ms_all"]))
        comparisons.append(kept)
    large = [c for c in comparisons if c["n"] >= 8000]
    summary = {}
    for variant in sorted({c["variant"] for c in large}):
        for family in (*reader.FAMILIES, "all"):
            speeds = [c["variant_over_reference"] for c in large
                      if c["variant"] == variant and family in (c["family"], "all")]
            candidates = [c["metrics"]["candidates"]["ratio"] for c in large
                          if c["variant"] == variant and family in (c["family"], "all")]
            selected = [c for c in large if c["variant"] == variant and family in (c["family"], "all")]
            twin_speeds = [c["variant_over_twin"] for c in selected if "variant_over_twin" in c]
            twin_candidates = [c["candidates_over_twin"] for c in selected if "candidates_over_twin" in c]
            summary[f"{variant}:{family}"] = dict(comparisons=len(speeds),
                time_min=min(speeds, default=None), time_median=statistics.median(speeds) if speeds else None,
                time_max=max(speeds, default=None), slower_than_reference=sum(s > 1 for s in speeds),
                candidates_min=min(candidates, default=None), candidates_max=max(candidates, default=None),
                time_over_twin_min=min(twin_speeds, default=None),
                time_over_twin_median=statistics.median(twin_speeds) if twin_speeds else None,
                time_over_twin_max=max(twin_speeds, default=None), slower_than_twin=sum(s > 1 for s in twin_speeds),
                candidates_over_twin_min=min(twin_candidates, default=None),
                candidates_over_twin_max=max(twin_candidates, default=None))
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
