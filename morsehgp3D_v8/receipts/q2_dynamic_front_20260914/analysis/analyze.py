#!/usr/bin/env python3
"""Read completed dynamic-front campaigns without running a probe or writing data.

--root accepts campaigns/ or its qualification parent. Other artifact schemas
are not campaign inputs. --file-series-directory explicitly authorizes grouping
n{n}.u16le files of ONE directory for growth; their byte prefixes are checked.
Without that option, distinct file inputs never become one growth series.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
from contextlib import redirect_stdout
import hashlib
import io
import json
from pathlib import Path
import statistics
import sys
from typing import Any

REPO = next(parent for parent in Path(__file__).resolve().parents
            if (parent / "morsehgp3D_v8/bench/run_wspd_q2_dynamic_matrix.py").is_file())
sys.path.insert(0, str(REPO / "morsehgp3D_v8/bench"))
import run_wspd_q2_dynamic_matrix as reader  # noqa: E402

SIZES = (8000, 16000, 32000)
PAIR_KEYS = tuple(key for key in reader.KEYS if key != "schedule")
GROWTH_KEYS = tuple(key for key in reader.KEYS if key not in ("family", "n"))
GEOMETRY = {
    "candidate_pairs": ("candidate_pairs",),
    "front.product_visits": ("front_work", "product_visits"),
    "front.witness_descent_steps": ("front_work", "witness_descent_steps"),
    "census.count_node_visits": ("census_work", "count_node_visits"),
    "order.structural_splits": ("order_work", "structural_splits"),
    "census.payload_supports": ("census_work", "payload_supports"),
}
LOAD_FIELDS = ("jobs", "front_products", "input_rectangles", "count_node_visits", "supports")


def quotient(numerator: int | float | None, denominator: int | float | None) -> float | None:
    return None if numerator is None or denominator in (None, 0) else numerator / denominator


def stats(values: list[int | float]) -> dict[str, int | float] | None:
    if not values:
        return None
    return dict(n_samples=len(values), median=statistics.median(values), min=min(values), max=max(values))


def path_value(row: dict, path: tuple[str, ...]) -> Any:
    for part in path:
        row = row[part]
    return row


def imbalance(values: list[int | float]) -> float | None:
    return quotient(max(values, default=0), statistics.mean(values) if values else 0)


def row_key(row: dict, keys: tuple[str, ...]) -> tuple:
    return tuple(row[key] for key in keys)


def timing_summary(records: list[dict]) -> dict:
    return {field: stats([record["result"]["timings"][field] for record in records]) for field in reader.TIMES}


def dispatch_summary(records: list[dict]) -> dict:
    return {field: stats([record["result"]["dispatch_work"][field] for record in records])
            for field in reader.DISPATCH_FIELDS}


def topology(affinity: tuple[int, ...]) -> dict:
    if affinity == (0, 2, 4, 6):
        return dict(label="four_distinct_physical_cores", physical_cores=4,
                    basis="declared_host_mapping_not_inferred_from_worker_count")
    if affinity == (0, 1, 2, 3):
        return dict(label="two_physical_cores_four_SMT_threads", physical_cores=2,
                    basis="declared_host_mapping_not_inferred_from_worker_count")
    return dict(label="other_affinity", physical_cores=None, basis="not_inferred")


def verify_file_series(directories: list[Path], pins: dict[str, dict]) -> tuple[dict[str, str], list[dict]]:
    resolved = [directory.resolve(strict=True) for directory in directories]
    reader.require(len(resolved) == len(set(resolved)) and all(path.is_dir() for path in resolved),
                   "duplicate or invalid explicit file-series directory")
    labels, proofs = {}, []
    for directory in resolved:
        inputs = [(family, pin) for family, pin in pins.items() if Path(pin["path"]).parent.resolve() == directory]
        reader.require(bool(inputs), f"explicit series has no measured input: {directory}")
        by_size = sorted(inputs, key=lambda item: item[1]["n"])
        reader.require(len({pin["n"] for _, pin in by_size}) == len(by_size), "duplicate size in file series")
        previous, evidence = b"", []
        label = "file_series:" + str(directory)
        for family, pin in by_size:
            path = Path(pin["path"])
            reader.require(path.name == f"n{pin['n']}.u16le", "file-series name does not declare its measured size")
            data = path.read_bytes()
            reader.require(len(data) == pin["bytes"] == 6 * pin["n"] and
                           hashlib.sha256(data).hexdigest() == pin["sha256"], "file changed before prefix check")
            reader.require(data.startswith(previous), "file series is not made of nested exact byte prefixes")
            previous = data
            labels[family] = label
            evidence.append(dict(family=family, n=pin["n"], sha256=pin["sha256"]))
        proofs.append(dict(series=label, inputs=evidence, nested_byte_prefixes=True,
                           interpretation="more sampled sites in one explicit input series, not independent acquisitions"))
    return labels, proofs


def summarize_group(group: dict, series_labels: dict[str, str]) -> dict:
    records = group.pop("records")
    cases: dict[tuple, list[dict]] = defaultdict(list)
    paired: dict[tuple, dict[str, list[dict]]] = defaultdict(lambda: defaultdict(list))
    growth: dict[tuple, dict[int, list[dict]]] = defaultdict(lambda: defaultdict(list))
    identities, signatures = {}, {}
    for record in records:
        row = record["result"]
        reader.cross_check(row, identities, signatures)
        cases[row_key(row, reader.KEYS)].append(record)
        paired[row_key(row, PAIR_KEYS)][row["schedule"]].append(record)
        if row["n"] in SIZES:
            family = series_labels.get(row["family"], row["family"])
            growth[(family, *row_key(row, GROWTH_KEYS))][row["n"]].append(record)
    result = {**group, "measurements": len(records), "configurations": len(cases)}
    result["schedule_comparisons"] = []
    for key, schedules in sorted(paired.items()):
        cells = {schedule: dict(n_samples=len(values), timings=timing_summary(values),
                                dispatch_work=dispatch_summary(values)) if values else None
                 for schedule in ("coarse", "donate") for values in [schedules.get(schedule, [])]}
        ratios = {}
        if cells["coarse"] and cells["donate"]:
            for name in ("total_ms", "pipeline_wall_ms", "partition_ms"):
                ratios[name] = quotient(cells["coarse"]["timings"][name]["median"],
                                        cells["donate"]["timings"][name]["median"])
        result["schedule_comparisons"].append(dict(zip(PAIR_KEYS, key), schedules=cells,
            coarse_over_donate_median_time=ratios, paired_parameters=bool(ratios)))

    result["configurations_detail"] = []
    for key, values in sorted(cases.items()):
        row = values[0]["result"]
        geometric = {name: path_value(row, path) for name, path in GEOMETRY.items()}
        reader.require(all(all(path_value(record["result"], path) == geometric[name] for name, path in GEOMETRY.items())
                           for record in values), "repeated geometric counts differ")
        runs = []
        for record in values:
            value, workers = record["result"], record["result"]["workers"]
            loads = {field: [worker[field] for worker in workers] for field in LOAD_FIELDS}
            presence = [worker["elapsed_ms"] for worker in workers]
            runs.append(dict(campaign=record["campaign"], repeat=record["repeat"],
                worker_counts=loads, discrete_max_over_mean={field: imbalance(items) for field, items in loads.items()},
                worker_presence_elapsed_ms=presence, presence_max_over_mean=imbalance(presence),
                worker_payload_ms=[worker["payload_ms"] for worker in workers],
                worker_dispatch=[worker["dispatch_work"] for worker in workers],
                parallel_work=value["parallel_work"]))
        result["configurations_detail"].append(dict(zip(reader.KEYS, key), n_samples=len(values),
            timings=timing_summary(values), geometric_work=geometric, dispatch_work=dispatch_summary(values),
            discrete_max_over_mean={field: stats([run["discrete_max_over_mean"][field] for run in runs
                                                   if run["discrete_max_over_mean"][field] is not None]) for field in LOAD_FIELDS},
            presence_max_over_mean=stats([run["presence_max_over_mean"] for run in runs
                                         if run["presence_max_over_mean"] is not None]),
            input_hash=row["input_hash"], digest=row["digest"], runs=runs))

    result["growth_8k_16k_32k"] = []
    for key, sizes in sorted(growth.items()):
        metrics = {}
        specifications = [("geometry", name, path) for name, path in GEOMETRY.items()]
        specifications += [("schedule_variable", "dispatch." + name, ("dispatch_work", name))
                           for name in reader.DISPATCH_FIELDS]
        for category, name, path in specifications:
            cells = [stats([path_value(record["result"], path) for record in sizes.get(n, [])]) for n in SIZES]
            ratios = [quotient(cells[i + 1]["median"], cells[i]["median"])
                      if cells[i] and cells[i + 1] else None for i in range(2)]
            metrics[name] = dict(category=category, samples=cells, doubling_ratios=ratios,
                at_least_four=[value is not None and value >= 4 for value in ratios],
                near_four=[value is not None and 3.8 <= value < 4 for value in ratios])
        times = {}
        for name in ("total_ms", "pipeline_wall_ms"):
            cells = [stats([record["result"]["timings"][name] for record in sizes.get(n, [])]) for n in SIZES]
            times[name] = dict(samples=cells, doubling_ratios=[quotient(cells[i + 1]["median"], cells[i]["median"])
                                if cells[i] and cells[i + 1] else None for i in range(2)])
        result["growth_8k_16k_32k"].append(dict(zip(("family_or_explicit_series", *GROWTH_KEYS), key),
            sizes=list(SIZES), available_sizes=sorted(sizes), metrics=metrics, timings=times))
    return result


def analyze(root: Path, file_series: list[Path]) -> dict:
    root = root.resolve(strict=True)
    container = root / "campaigns" if (root / "campaigns").is_dir() else root
    analyzer_pin = reader.digest(Path(__file__).resolve())
    source_pins = reader.sources()
    directories = reader.campaign_directories(container)
    triplets = {str((directory / name).relative_to(root)): reader.digest(directory / name)
                for directory in directories for name in reader.RECEIPT_FILES}
    groups: dict[tuple, dict] = {}
    inputs: dict[str, dict] = {}
    for directory in directories:
        output = io.StringIO()
        with redirect_stdout(output):
            reader.require(reader.check(argparse.Namespace(receipt=directory, summary=False)) == 0,
                           "strict campaign reader failed")
        verdict = reader.parse_result(output.getvalue().encode())
        manifest = reader.parse_result((directory / "MANIFEST.json").read_bytes())
        for family, pin in manifest["input_files"].items():
            reader.require(family not in inputs or inputs[family] == pin, "input provenance changed across campaigns")
            inputs[family] = pin
        affinity = tuple(manifest["cpu_affinity"])
        provenance = verdict["provenance"]
        key = affinity, provenance["build_id"], provenance["machine_id"]
        if key not in groups:
            groups[key] = dict(cpu_affinity=list(affinity), topology=topology(affinity),
                build_id=provenance["build_id"], machine_id=provenance["machine_id"], campaigns=[], records=[])
        group = groups[key]
        relative = str(directory.relative_to(root))
        group["campaigns"].append(relative)
        for line in (directory / "MEASURES.jsonl").read_bytes().splitlines():
            record = reader.parse_result(line)
            group["records"].append(dict(result=record["result"], repeat=record["repeat"], campaign=relative))
    labels, proofs = verify_file_series(file_series, inputs)
    summaries = [summarize_group(groups[key], labels) for key in sorted(groups)]
    reader.require(reader.campaign_directories(container) == directories, "campaign set changed during analysis")
    reader.require(all(reader.digest(root / name) == pin for name, pin in triplets.items()),
                   "receipt triplet changed during analysis")
    reader.require(reader.input_hashes(inputs) == {family: pin["sha256"] for family, pin in inputs.items()},
                   "input file changed during analysis")
    reader.require(reader.sources() == source_pins and reader.digest(Path(__file__).resolve()) == analyzer_pin,
                   "analysis or reader source changed during analysis")
    return dict(schema="mhgp8_dynamic_campaign_analysis_v1", status="passed", root=str(root),
        campaign_container=str(container.relative_to(root)), analyzer_sha256=analyzer_pin,
        reader_sha256=source_pins[reader.RUNNER_SOURCE], triplet_sha256=triplets, input_files=inputs,
        explicit_file_series=proofs, groups=summaries, full_contract_qualified=False, gcp_used=False,
        conventions=dict(time_unit="milliseconds", statistics="all repetitions retained: median/min/max/n_samples",
            pairing="same family/n/K/s/seed/workers/jobs/queue/interval/Pool/build/machine/affinity; only schedule differs",
            timings="observations on a shared host; median ratios are not stable speedup guarantees",
            presence="Donate worker elapsed includes condition-variable waiting until global completion; max/mean near 1 is NOT balance evidence",
            worker_sums="sums of presence/payload intervals, not CPU time; never subtract them from wall time",
            threads_0="fresh mono reference; schedule arguments are inert, not two different schedulers",
            load="front/census/support counts are separate proxies, never summed into a fictitious common unit",
            growth="two observed doublings, not an asymptotic bound; dispatch counters are scheduling-dependent",
            null_ratio="missing sample or zero denominator, not zero growth",
            scope="q2 front+census+paid callback component only; no FULL/G4 or multi-million-point qualification"))


def selftest() -> dict:
    reader.require(stats([7, 1, 3]) == dict(n_samples=3, median=3, min=1, max=7), "sample aggregation failed")
    reader.require(quotient(8, 2) == 4 and quotient(8, 0) is None and quotient(None, 2) is None,
                   "zero/missing ratio semantics failed")
    reader.require(imbalance([1, 3]) == 1.5 and imbalance([0, 0]) is None, "load ratio failed")
    base = dict(zip(reader.KEYS, ("uniform", 8000, 10, 8, 3, 4, 16, "coarse", 64, 64, 64)))
    reader.require(row_key(base, PAIR_KEYS) == row_key({**base, "schedule": "donate"}, PAIR_KEYS), "pairing schedule failed")
    for key in ("family", "n", "kmax", "s", "seed", "threads", "jobs_per_worker", "queue_capacity", "donation_interval", "pool_min_factor"):
        value = "terrain" if key == "family" else base[key] + 1
        reader.require(row_key(base, PAIR_KEYS) != row_key({**base, key: value}, PAIR_KEYS), "pairing mixed " + key)
    reader.require(topology((0, 2, 4, 6))["physical_cores"] == 4 and
                   topology((0, 1, 2, 3))["physical_cores"] == 2, "affinity labels failed")
    return dict(status="passed", selftest="in_memory_only", checks=15)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path)
    parser.add_argument("--file-series-directory", action="append", type=Path, default=[])
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    try:
        if args.selftest:
            reader.require(args.root is None and not args.file_series_directory, "selftest does not read campaigns")
            result = selftest()
        else:
            reader.require(args.root is not None, "--root is required for analysis")
            result = analyze(args.root, args.file_series_directory)
        print(json.dumps(result, allow_nan=False, sort_keys=True, separators=(",", ":")))
        return 0
    except (ValueError, RuntimeError, OSError, KeyError, TypeError) as error:
        print(json.dumps(dict(status="rejected", error=str(error)), sort_keys=True), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
