#!/usr/bin/env python3
"""Read completed parallel q2 campaigns; never execute a probe or mix affinities.

Scratch analysis, outside the frozen producer/runner source closure. The strict
campaign reader is reused read-only. Run only after the requested root closes:
  python3 -B build/v8_front_workers_20260914/analyze.py --root <receipt-root>
All durations are milliseconds; ratios compare like units inside one recorded
build/machine/affinity group. A 50k q2 component is not a FULL/G4 tower result.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import statistics
import sys
from typing import Any

REPO = next(parent for parent in Path(__file__).resolve().parents
            if (parent / "morsehgp3D_v8/bench/run_wspd_q2_parallel_matrix.py").is_file())
sys.path.insert(0, str(REPO / "morsehgp3D_v8/bench"))
import run_wspd_q2_parallel_matrix as reader  # noqa: E402

SIZES = (8000, 16000, 32000)
GRAINS = (1, 16, 64)
METRICS = {
    "candidate_pairs": ("candidate_pairs",),
    "front.product_visits": ("front_work", "product_visits"),
    "front.witness_descent_steps": ("front_work", "witness_descent_steps"),
    "census.count_node_visits": ("census_work", "count_node_visits"),
    "order.structural_splits": ("order_work", "structural_splits"),
    "census.payload_supports": ("census_work", "payload_supports"),
}


def ratio(numerator: int | float | None, denominator: int | float | None) -> float | None:
    return None if numerator is None or denominator in (None, 0) else numerator / denominator


def metric(row: dict, path: tuple[str, ...]) -> int:
    value = row
    for key in path:
        value = value[key]
    return value


def case_key(row: dict) -> tuple:
    return tuple(row[key] for key in reader.KEYS)


def time_cell(records: list[dict]) -> dict | None:
    if not records:
        return None
    names = dict(total_ms="total_ms", q2_wall_ms="pipeline_wall_ms", partition_ms="partition_ms",
                 worker_ms_sum="worker_ms_sum", payload_ms_sum="payload_ms_sum")
    values = {name: [item["result"]["timings"][field] for item in records] for name, field in names.items()}
    return dict(n_samples=len(records), **{name: statistics.median(items) for name, items in values.items()},
                min_ms={name: min(items) for name, items in values.items()},
                max_ms={name: max(items) for name, items in values.items()})


def describe_affinity(affinity: tuple[int, ...]) -> dict:
    if affinity == (0, 1, 2, 3):
        return dict(label="two_physical_cores_four_SMT_threads", physical_cores=2,
                    topology_basis="declared_mapping_for_this_campaign_host")
    if affinity == (0, 2, 4, 6):
        return dict(label="four_physical_cores", physical_cores=4,
                    topology_basis="declared_mapping_for_this_campaign_host")
    return dict(label="other_affinity", physical_cores=None, topology_basis="not_inferred")


def summarize_group(group: dict) -> dict:
    records = group.pop("records")
    cases: dict[tuple, list[dict]] = defaultdict(list)
    geometry: dict[tuple, dict[int, dict]] = defaultdict(dict)
    identities, signatures = {}, {}
    for record in records:
        row = record["result"]
        # Cross-campaign equality is scoped to the SAME affinity/provenance.
        reader.cross_check(row, identities, signatures)
        cases[case_key(row)].append(record)
        if row["s"] == 8 and row["kmax"] in (5, 10) and row["n"] in SIZES:
            key = row["family"], row["kmax"], row["seed"], row["pool_min_factor"]
            geometry[key][row["n"]] = row
    result = {**group, "measurements": len(records), "configurations": len(cases)}

    timing_keys = sorted({(row["result"]["family"], row["result"]["seed"], row["result"]["pool_min_factor"])
                          for row in records if row["result"]["kmax"] == 10 and row["result"]["s"] == 8
                          and row["result"]["jobs_per_worker"] == 16 and row["result"]["n"] in SIZES})
    times = []
    for family, seed, pool in timing_keys:
        cells = []
        for n in SIZES:
            cell = {str(threads): time_cell(cases.get((family, n, 10, 8, seed, threads, 16, pool), []))
                    for threads in (0, 1, 2, 4)}
            one, four = cell["1"], cell["4"]
            cell["n"] = n
            cell["observed_ratio_1_to_4_total"] = ratio(one["total_ms"], four["total_ms"]) if one and four else None
            cell["observed_ratio_1_to_4_q2"] = ratio(one["q2_wall_ms"], four["q2_wall_ms"]) if one and four else None
            cell["ratio_above_worker_count"] = one is not None and four is not None and cell["observed_ratio_1_to_4_total"] > 4
            two = cell["2"]
            cell["observed_ratio_1_to_2_total"] = ratio(one["total_ms"], two["total_ms"]) if one and two else None
            cells.append(cell)
        times.append(dict(family=family, seed=seed, pool_min_factor=pool, kmax=10, s=8,
                          jobs_per_worker=16, sizes=cells))
    result["times_k10_s8"] = times

    growth = []
    for (family, kmax, seed, pool), by_size in sorted(geometry.items()):
        metrics = {}
        for name, path in METRICS.items():
            values = [metric(by_size[n], path) if n in by_size else None for n in SIZES]
            ratios = [ratio(values[i + 1], values[i]) for i in range(2)]
            metrics[name] = dict(values=values, doubling_ratios=ratios,
                                 at_least_four=[r is not None and r >= 4 for r in ratios],
                                 near_four=[r is not None and 3.8 <= r < 4 for r in ratios])
        growth.append(dict(family=family, kmax=kmax, seed=seed, pool_min_factor=pool,
                           available_sizes=sorted(by_size), sizes=list(SIZES), metrics=metrics))
    result["integer_growth_s8"] = growth

    grain_groups: dict[tuple, dict[int, list[dict]]] = defaultdict(dict)
    for key, values in cases.items():
        family, n, kmax, separation, seed, threads, jobs, pool = key
        if threads > 0 and jobs in GRAINS:
            grain_groups[(family, n, kmax, separation, seed, threads, pool)][jobs] = values
    grains = []
    for key, by_grain in sorted(grain_groups.items()):
        if not ({1, 64} & by_grain.keys()):
            continue
        family, n, kmax, separation, seed, threads, pool = key
        cells = {str(grain): time_cell(by_grain.get(grain, [])) for grain in GRAINS}
        reference = cells["16"]
        relative = {str(grain): ratio(cells[str(grain)]["total_ms"], reference["total_ms"])
                    if cells[str(grain)] and reference else None for grain in GRAINS}
        grains.append(dict(family=family, n=n, kmax=kmax, s=separation, seed=seed,
                           threads=threads, pool_min_factor=pool, grains=cells,
                           total_time_ratio_to_grain16=relative))
    result["grain_effect_1_16_64"] = grains

    result["q2_component_50k"] = [dict(zip(reader.KEYS, key), timings=time_cell(values))
                                   for key, values in sorted(cases.items()) if key[1] == 50000]
    result["worker_load"] = []
    for key, values in sorted(cases.items()):
        runs = []
        for record in values:
            row = record["result"]
            workers = row["workers"]
            load = {name: [worker[name] for worker in workers] for name in
                    ("jobs", "front_products", "input_rectangles", "count_node_visits", "supports",
                     "elapsed_ms", "payload_ms")}
            load.update(campaign=record["campaign"], repeat=record["repeat"],
                        completed_jobs=row["parallel_work"]["completed_jobs"],
                        target_jobs=row["parallel_work"]["target_jobs"],
                        prefix_product_visits=row["parallel_work"]["prefix_product_visits"],
                        max_elapsed_over_mean=ratio(max(load["elapsed_ms"], default=0),
                                                   statistics.mean(load["elapsed_ms"]) if workers else 0),
                        pool_peak_bytes_sum=row["parallel_work"]["pool_peak_bytes_sum"],
                        job_storage_bytes=row["parallel_work"]["job_storage_bytes"])
            runs.append(load)
        result["worker_load"].append(dict(zip(reader.KEYS, key), runs=runs))
    return result


def analyze(root: Path) -> dict:
    root = root.resolve(strict=True)
    analyzer_pin = reader.digest(Path(__file__).resolve())
    # A qualification bundle contains other artifact schemas (for example
    # thread_sanitizer/COMPLETION.json). Route its two explicit campaign
    # containers, but retain strict orphan/incomplete detection inside each.
    containers = ([root / "campaigns", root / "physical_cores"]
                  if (root / "campaigns").is_dir() and (root / "physical_cores").is_dir()
                  else [root])
    def campaign_set() -> list[Path]:
        return sorted(directory for container in containers
                      for directory in reader.campaign_directories(container))
    directories = campaign_set()
    files = [directory / name for directory in directories for name in reader.RECEIPT_FILES]
    pins = {str(path.relative_to(root)): reader.digest(path) for path in files}
    groups: dict[tuple, dict] = {}
    for directory in directories:
        # This reader rejects incomplete/failed campaigns, validates matrix,
        # commands, raw outputs, source closure, all counters and provenance.
        output = io.StringIO()
        with redirect_stdout(output):
            reader.require(reader.check(argparse.Namespace(receipt=directory, summary=False)) == 0,
                           "strict reader failed")
        verdict = reader.parse_result(output.getvalue().encode())
        manifest = reader.parse_result((directory / "MANIFEST.json").read_bytes())
        affinity = tuple(manifest["cpu_affinity"])
        provenance = verdict["provenance"]
        group_key = affinity, provenance["build_id"], provenance["machine_id"]
        if group_key not in groups:
            groups[group_key] = dict(cpu_affinity=list(affinity), topology=describe_affinity(affinity),
                                     build_id=provenance["build_id"], machine_id=provenance["machine_id"],
                                     campaigns=[], records=[])
        group = groups[group_key]
        relative = str(directory.relative_to(root))
        group["campaigns"].append(relative)
        for line in (directory / "MEASURES.jsonl").read_bytes().splitlines():
            record = reader.parse_result(line)
            group["records"].append(dict(result=record["result"], repeat=record["repeat"], campaign=relative))
    summaries = [summarize_group(groups[key]) for key in sorted(groups)]
    reader.require(campaign_set() == directories, "campaign set changed during analysis")
    reader.require(all(reader.digest(root / name) == pin for name, pin in pins.items()),
                   "receipt triplet changed during analysis")
    reader.require(reader.digest(Path(__file__).resolve()) == analyzer_pin, "analyzer changed during analysis")
    return dict(schema="mhgp8_parallel_scratch_analysis_v1", status="passed", root=str(root),
                analyzer_sha256=analyzer_pin, reader_sha256=reader.digest(REPO / reader.RUNNER_SOURCE),
                triplet_sha256=pins, campaign_containers=[str(path.relative_to(root)) for path in containers],
                groups=summaries, full_contract_qualified=False, gcp_used=False,
                conventions=dict(time_unit="milliseconds", total_ms="probe including preprocessing and owned payload destruction",
                                 time_cells="direct timing fields are medians; min_ms/max_ms cover all n_samples",
                                 timing_ratios="same-configuration median ratios on a shared host, not stable speedup claims",
                                 q2_wall_ms="front plus census and callback; excludes caller preprocessing",
                                 threads_0="fresh mono reference, not a parallel worker",
                                 threads_1="parallel API with one inline worker",
                                 ratio_null="missing observation or zero denominator; never treated as zero growth",
                                 growth="observed two doublings, not an asymptotic bound",
                                 affinities="never pooled or paired across affinity/build/machine groups",
                                 work="distinct counter units remain separate; sums of worker intervals are not wall times"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", required=True, type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(analyze(args.root), allow_nan=False, sort_keys=True, separators=(",", ":")))
        return 0
    except (ValueError, RuntimeError, OSError, KeyError, TypeError) as error:
        print(json.dumps(dict(status="rejected", error=str(error)), sort_keys=True), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
