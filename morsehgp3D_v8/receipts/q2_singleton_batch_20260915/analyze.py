#!/usr/bin/env python3
"""Explicit tranche18 analyzer port: closed batched captures, no inherited qualification."""
import argparse
import base64
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
import run_wspd_q2_batched_checks as reader
import record_quantums as quantums

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


def failed_summary(path):
    """Validate an archived failure, never promote its partial successful rows.

    Historical before/after source or binary differences are reported, not
    compared to today's bytes and not accepted as a completed qualification.
    The enclosing recorder separately pins all archived bytes unchanged.
    """
    manifest = reader.strict_json((path / "MANIFEST.json").read_text())
    completion = reader.strict_json((path / "COMPLETION.json").read_text())
    reader.require(manifest["schema"] in ("mhgp8_q2_batched_attempt_v1", quantums.SCHEMA) and
                   manifest["public_status"] == "not_claimed" and
                   manifest["gcp_used"] is False and manifest["full_contract_qualified"] is False and
                   completion["status"] == "failed" and type(completion["error"]) is str and
                   bool(completion["error"]) and
                   completion["manifest_sha256"] == reader.digest(path / "MANIFEST.json"),
                   "expected a preserved failed capture, not a qualification")
    commands = manifest["planned_commands"]
    is_quantums = manifest["schema"] == quantums.SCHEMA
    expected = quantums.plan(Path(manifest["build"])) if is_quantums else reader.plan(
        Path(manifest["build"]), path.resolve(), manifest["campaign"], manifest["lanes"])
    reader.require(commands == [[kind, command] for kind, command in expected],
                   "failed capture command plan changed")
    if is_quantums:
        quantums.validate_manifest(manifest)
    else:
        reader.validate_pins(manifest)
    records = completion["records"]
    reader.require(len(records) <= len(commands) and
                   {p.name for p in path.glob("record_*.json")} ==
                   {f"record_{i:04}.json" for i in range(len(records))}, "failed capture lost records")
    successful = 0
    for number, info in enumerate(records):
        reader.require(info["path"] == f"record_{number:04}.json", "failed record path changed")
        target = path / info["path"]
        reader.require(info["sha256"] == reader.digest(target), "failed record hash changed")
        record = reader.strict_json(target.read_text())
        kind, command = commands[number]
        reader.require(record["kind"] == kind and record["command"] == command and
                       record["cwd"] == str(ROOT) and record["status"] in ("passed", "failed"),
                       "failed record command binding changed")
        for channel in ("stdout", "stderr"):
            reader.require(base64.b64decode(record[channel + "_base64"], validate=True)
                           .decode("utf-8", errors="replace") == record[channel],
                           "failed record raw log changed")
        if record["status"] == "passed":
            reader.require(type(record["exit_code"]) is int and record["exit_code"] == 0,
                           "historical successful command has invalid exit code")
            successful += 1
    changes = {}
    for name, after in (("source_sha256", "source_sha256_after"),
                        ("artifact_sha256", "artifact_sha256_after")):
        before_pins, after_pins = manifest[name], completion[after]
        changes[name] = sorted(k for k in before_pins.keys() | after_pins.keys()
                               if before_pins.get(k) != after_pins.get(k))
    return dict(path=str(path.relative_to(ROOT)), status="failed", error=completion["error"],
                recorded_commands=len(records), planned_commands=len(commands),
                partial_successful_commands=successful, included_in_measures=False,
                source_or_artifact_changes=changes,
                manifest_sha256=reader.digest(path / "MANIFEST.json"))


def read_passed(path):
    manifest = reader.strict_json((path / "MANIFEST.json").read_text())
    if manifest["schema"] == quantums.SCHEMA:
        quantums.read(path)
    else:
        reader.read(path)
    return manifest


def analyze(paths, failures=(), historical=()):
    reader.require(paths and len(paths) == len(set(paths)) and
                   len((*paths, *failures, *historical)) == len(set((*paths, *failures, *historical))) and
                   all(p.parent == HERE for p in (*paths, *failures, *historical)),
                   "explicit distinct local capture paths required")
    captures, measures, comparisons, growth_groups, gates = [], [], [], [], []
    identities, signatures = {}, {}
    for path in (*paths, *historical):
        with redirect_stdout(io.StringIO()):
            manifest = read_passed(path)
        is_historical = path in historical
        is_quantums = manifest["schema"] == quantums.SCHEMA
        source_fingerprint = hashlib.sha256(json.dumps(manifest["source_sha256"], sort_keys=True,
                                                       separators=(",", ":")).encode()).hexdigest()
        completion = reader.strict_json((path / "COMPLETION.json").read_text())
        records = [reader.strict_json((path / info["path"]).read_text())
                   for info in completion["records"]]
        rows = [record["row"] for record in records if "row" in record]
        captures.append(dict(path=str(path.relative_to(ROOT)), campaign=manifest["campaign"],
                             lanes=manifest.get("lanes"), affinity=manifest["affinity"],
                             historical=is_historical, source_fingerprint=source_fingerprint,
                             records=len(records), measures=len(rows),
                             manifest_sha256=reader.digest(path / "MANIFEST.json")))
        gates.extend(dict(capture=str(path.relative_to(ROOT)),
                          gate=reader.strict_json(record["stdout"]))
                     for record in records if record["kind"] == "gate")
        measures.extend(rows)
        reader.validate_pairs(rows)
        for row in rows:
            reader.cross_check(row, identities, signatures)
        grouped = manifest["campaign"] in ("tuning", "smoke")
        width = 7 if is_quantums else 4 if grouped else 2
        reader.require(len(rows) % width == 0, "incomplete paired observation group")
        for offset in range(0, len(rows), width):
            reference, *candidates = rows[offset:offset + width]
            reader.require(reference["execution"] == "parallel_front" and
                           [(r["lanes"], r["quantum"]) for r in candidates] ==
                           ([(lanes, quantum) for lanes in (1, 8, 16) for quantum in (8, 64)]
                            if is_quantums else
                            [(lanes, manifest.get("quantum", 1)) for lanes in (1, 8, 16)] if grouped
                            else [(manifest["lanes"], manifest.get("quantum", 1))]),
                           "unexpected paired configuration order")
            for candidate in candidates:
                reader.require(candidate["execution"] == "batched_singleton_front_census" and
                               all(reference[k] == candidate[k] for k in
                                   ("n", "family", "kmax", "s", "seed", "threads",
                                    "jobs_per_worker", "pool_min_factor")),
                               "timing comparison changed more than the batching policy")
                old = reference["timings"]["pipeline_wall_ms"]
                new = candidate["timings"]["pipeline_wall_ms"]
                comparisons.append(dict(campaign=manifest["campaign"], family=reference["family"],
                    historical=is_historical, source_fingerprint=source_fingerprint,
                    n=reference["n"], kmax=reference["kmax"], s=reference["s"],
                    pool=reference["pool_min_factor"], workers=reference["threads"],
                    lanes=candidate["lanes"], quantum=candidate["quantum"],
                    coarse_ms=old, batched_ms=new, old_over_new=ratio(old, new),
                    batch_work=candidate["batch_work"], batch_storage=candidate["batch_storage"]))
        if manifest["campaign"] != "scale":
            continue
        keys = sorted({(r["family"], r["kmax"], r["s"], r["pool_min_factor"],
                        r["threads"], r["lanes"], r["quantum"])
                       for r in rows if r["execution"] == "batched_singleton_front_census"})
        for key in keys:
            selected = sorted([r for r in rows if r["execution"] == "batched_singleton_front_census" and
                (r["family"], r["kmax"], r["s"], r["pool_min_factor"],
                 r["threads"], r["lanes"], r["quantum"]) == key], key=lambda r: r["n"])
            reader.require([r["n"] for r in selected] == [8000, 16000, 32000], "growth grid incomplete")
            growth_groups.append(dict(campaign=manifest["campaign"], family=key[0], kmax=key[1], s=key[2],
                pool=key[3], workers=key[4], lanes=key[5], quantum=key[6], geometry=growth(selected, MAIN),
                pool_all_counters=growth(selected, {name: ("pool_work", name) for name in reader.POOL_FIELDS}),
                census_operations=growth(selected, {name: ("census_work", name)
                    for name in selected[0]["census_work"] if name != "consumed_witness_sites"}),
                sibling_operations=growth(selected, {name: ("sibling_work", name)
                    for name in selected[0]["sibling_work"]}),
                order_operations=growth(selected, {name: ("order_work", name)
                    for name in selected[0]["order_work"]}),
                population_accounting=growth(selected, {"consumed_witness_sites": ("census_work", "consumed_witness_sites")}),
                scheduling=growth(selected, {name: ("batch_work", name) for name in reader.BATCH_FIELDS}),
                storage=growth(selected, {name: ("batch_storage", name) for name in selected[0]["batch_storage"]})))
    batched = [r for r in measures if r["execution"] == "batched_singleton_front_census"]
    speeds = [r["old_over_new"] for r in comparisons if r["n"] >= 8000 and not r["historical"]]
    revision_comparisons = []
    configuration = ("family", "n", "kmax", "s", "pool", "workers", "lanes", "quantum")
    for previous in (r for r in comparisons if r["historical"]):
        for current in (r for r in comparisons if not r["historical"]):
            if any(previous[key] != current[key] for key in configuration):
                continue
            revision_comparisons.append(dict(
                configuration={key: current[key] for key in configuration},
                historical_source=previous["source_fingerprint"], current_source=current["source_fingerprint"],
                historical_campaign=previous["campaign"], current_campaign=current["campaign"],
                historical_ms=previous["batched_ms"], current_ms=current["batched_ms"],
                historical_over_current=ratio(previous["batched_ms"], current["batched_ms"]),
                batch_work_equal=previous["batch_work"] == current["batch_work"],
                storage_equal=previous["batch_storage"] == current["batch_storage"],
                timing_scope="distinct historical observations, not simultaneous paired measurements"))
    return dict(status="passed", scope="q2_all_cloud_supports_not_full", full_contract_qualified=False,
        gcp_used=False, captures=captures, preserved_failures=[failed_summary(p) for p in failures],
        gates=gates, measures=len(measures), batched_measures=len(batched),
        reference_measures=len(measures)-len(batched), paired_comparisons=len(comparisons),
        all_geometric_work_and_payloads_equal=True, general_subquadratic_bound=False,
        timing_scope="single paired observations on shared host; speed not qualified by repetitions",
        speed_summary=dict(excludes_smoke=True, excludes_historical=True, comparisons=len(speeds),
                           min=min(speeds, default=None), median=statistics.median(speeds) if speeds else None,
                           max=max(speeds, default=None), above_one=sum(r > 1 for r in speeds)),
        batched_work_across_all_configurations={name: sum(r["batch_work"][name] for r in batched)
                                              for name in reader.BATCH_FIELDS if name not in reader.MAXIMA},
        max_active=max((r["batch_work"]["max_active"] for r in batched), default=0),
        state_bytes=sorted({r["batch_storage"]["state_bytes"] for r in batched}),
        timing_comparisons=comparisons, revision_comparisons=revision_comparisons, growth=growth_groups)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("captures", type=Path, nargs="+")
    parser.add_argument("--failed", type=Path, action="append", default=[])
    parser.add_argument("--historical", type=Path, action="append", default=[])
    args = parser.parse_args()
    print(json.dumps(analyze([p.resolve() for p in args.captures],
                             [p.resolve() for p in args.failed],
                             [p.resolve() for p in args.historical]), sort_keys=True, allow_nan=False))
