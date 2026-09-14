#!/usr/bin/env python3
"""Capture and recheck the real WSPD front; no census or HGP tower claim."""

from __future__ import annotations

import argparse
import base64
from collections import defaultdict
from datetime import datetime
import itertools
import json
import os
from pathlib import Path
import platform
import re
import signal
import statistics
import subprocess
import sys
from typing import Any

from check_paired_campaign import PROVENANCE_POLICY, provenance, sha256
from paired_receipts import finite_times, fingerprint
from run_p0_matrix import digest, invoke, on_signal, parse_result, require, uint, utc_stamp, write_json


ROOT = Path(__file__).resolve().parents[2]
RUNNER_SOURCE = "morsehgp3D_v8/bench/run_wspd_front_matrix.py"
SCOPE = "real_wspd_front_without_census_or_full"
SCHEMA = "mhgp8_wspd_front_campaign_v1"
FAMILIES = ("uniform", "terrain", "clusters", "rows")
MODES = ("pure", "samples")
KEYS = ("family", "n", "kmax", "s", "seed", "front_mode")
MATRIX_KEYS = ("families", "sizes", "kmax", "separations", "seeds", "modes")
RECEIPT_FILES = ("MANIFEST.json", "MEASURES.jsonl", "COMPLETION.json")
CLOUD_FIELDS = ("coordinate_copies", "validation_points", "uniqueness_comparisons",
                "uniqueness_adjacent_tests", "range_tree_leaf_visits", "range_tree_nodes",
                "range_tree_merges")
INDEX_FIELDS = ("point_visits", "nodes", "max_depth", "escape_links")
GENERATION_FIELDS = ("rng_calls", "proposed_points", "duplicate_rejections", "accepted_points",
                     "ordered_set_comparisons")
FRONT_FIELDS = ("product_visits", "diagonal_splits", "diagonal_leaves", "disjoint_splits",
                "separation_tests", "witness_searches", "witness_descent_steps",
                "witness_box_distance_tests", "proposed_sites", "proposals_in_factors",
                "h_bound_tests", "xi_bound_tests", "witness_lane_credits",
                "fully_rejected_products", "emitted_rectangles", "emitted_factor_sites",
                "max_factor_size", "leaf_pair_rectangles", "max_stack_size", "max_product_depth")
FRONT_ARRAYS = {"size_class_rectangles": 5, "size_class_pair_mass": 5,
                "rejected_pair_mass": 3, "residual_pair_mass": 3, "lane_rectangles": 3}
MEMORY_FIELDS = ("input_capacity_bytes", "cloud_retained_bytes", "index_retained_bytes")
TIMES = ("generation_ms", "cloud_ms", "index_ms", "front_and_callback_ms", "validation_ms",
         "destruction_ms", "total_ms")
RECIPES = dict(uniform="uniform_splitmix64_v1", terrain="terrain_splitmix64_v1",
               clusters="eight_corner_clusters_splitmix64_v1", rows="parallel_rows_v1")
DISCRETE_FIELDS = ("recipe", "seed_affects_input", "input_hash", "total_unordered_pairs",
                   "active_lane_mask", "generation_work", "cloud_work", "index_work",
                   "front_work", "digest", "memory")


def sources() -> dict[str, str]:
    source = ROOT / "morsehgp3D_v8"
    bench = source / "bench"
    paths = [source / "CMakeLists.txt", *sorted((source / "src").rglob("*.hpp")),
             *sorted((source / "src").rglob("*.cpp")), *sorted(bench.glob("*.hpp")),
             *(bench / name for name in ("wspd_front_probe.cpp", "run_wspd_front_matrix.py",
                 "run_p0_matrix.py", "paired_receipts.py", "check_paired_campaign.py")),
             source / "tests/wspd_front_receipts_gate.py"]
    return {str(path.relative_to(ROOT)): digest(path) for path in paths}


def timestamp(value: Any, name: str) -> datetime:
    require(type(value) is str, f"{name}: missing timestamp")
    result = datetime.fromisoformat(value)
    require(result.tzinfo is not None and result.utcoffset().total_seconds() == 0,
            f"{name}: timestamp must be UTC")
    return result


def counters(value: Any, names: tuple[str, ...], name: str) -> None:
    require(type(value) is dict and set(value) == set(names), f"{name}: wrong counter fields")
    for field in names:
        uint(value[field], f"{name}.{field}")


def array(value: Any, length: int, name: str) -> None:
    require(type(value) is list and len(value) == length, f"{name}: wrong array length")
    for item in value:
        uint(item, name)


def command_for(probe: str, key: tuple[Any, ...]) -> list[str]:
    family, n, kmax, separation, seed, mode = key
    return [probe, str(n), family, str(kmax), str(separation), str(seed), mode]


def validate_result(row: dict[str, Any], command: list[str]) -> None:
    fixed = dict(schema="mhgp8_wspd_front_probe_v1", status="completed", scope=SCOPE,
        phase="exploration_v8_hors_registre", backend="cpu_reference", profile="quantized_u16_input_only",
        mode="implementation_v8_p0", public_status="not_claimed", separation_convention="box_gap_diameter_v1")
    for field, expected in fixed.items():
        require(row.get(field) == expected, f"{field}: unsupported scope or convention")
    require(row.get("gcp_used") is False and uint(row.get("threads"), "threads") == 1,
            "unexpected cloud use or concurrency")
    require(len(command) == 7, "wrong command length")
    for field in ("n", "kmax", "s", "seed", "total_unordered_pairs", "active_lane_mask"):
        uint(row.get(field), field)
    expected = command[2], int(command[1]), int(command[3]), int(command[4]), int(command[5]), command[6]
    require(tuple(row.get(field) for field in KEYS) == expected, "command/result tuple mismatch")
    n, kmax, family = row["n"], row["kmax"], row["family"]
    require(n >= 2 and 1 <= kmax <= 10 and 1 <= row["s"] <= (1 << 32) - 1 and
            family in FAMILIES and row["front_mode"] in MODES, "unsupported tuple")
    require(row.get("recipe") == RECIPES[family] and row.get("seed_affects_input") is (family != "rows"),
            "wrong fixture recipe or seed contract")
    if family == "rows":
        require(n % 2 == 0 and n <= 131072, "rows fixture outside its exact domain")
    fingerprint(row.get("input_hash"), "input_hash")
    total = n * (n - 1) // 2
    active = (1 << min(kmax, 3)) - 1
    require(row["total_unordered_pairs"] == total and row["active_lane_mask"] == active,
            "wrong total pair count or active lanes")
    generation = row.get("generation_work")
    counters(generation, GENERATION_FIELDS, "generation_work")
    require(generation["accepted_points"] == n and
            generation["proposed_points"] == n + generation["duplicate_rejections"],
            "fixture generation lost its work or sites")
    if family == "rows":
        require(generation["rng_calls"] == generation["duplicate_rejections"] ==
                generation["ordered_set_comparisons"] == 0, "deterministic rows paid random generation")
    else:
        require(generation["rng_calls"] == (4 if family == "clusters" else 3) * generation["proposed_points"] and
                generation["ordered_set_comparisons"] > 0, "random generation lost RNG or uniqueness work")
    cloud, index = row.get("cloud_work"), row.get("index_work")
    counters(cloud, CLOUD_FIELDS, "cloud_work")
    counters(index, INDEX_FIELDS, "index_work")
    require(cloud["coordinate_copies"] == cloud["validation_points"] == cloud["range_tree_leaf_visits"] == n and
            cloud["range_tree_nodes"] == 2 * n - 1 and
            cloud["range_tree_merges"] == cloud["uniqueness_adjacent_tests"] == n - 1 and
            cloud["uniqueness_comparisons"] > 0, "shared cloud work changed")
    require(index["nodes"] == index["escape_links"] == 2 * n - 1 and
            1 <= index["max_depth"] <= 48 and n <= index["point_visits"] <= 97 * n,
            "global index outside its representation bounds")
    work = row.get("front_work")
    require(type(work) is dict and set(work) == {*FRONT_FIELDS, *FRONT_ARRAYS}, "wrong front work fields")
    for field in FRONT_FIELDS:
        uint(work[field], f"front_work.{field}")
    for field, length in FRONT_ARRAYS.items():
        array(work[field], length, field)
    rectangles, splits, killed = work["emitted_rectangles"], work["disjoint_splits"], work["fully_rejected_products"]
    require(work["diagonal_splits"] == n - 1 and work["diagonal_leaves"] == n and
            work["product_visits"] == 1 + 3 * (n - 1) + 2 * splits and
            rectangles + killed == n - 1 + splits and
            work["separation_tests"] == rectangles + splits, "front partition work is inconsistent")
    require(work["max_product_depth"] <= 96 and
            1 <= work["max_stack_size"] <= 2 * work["max_product_depth"] + 1,
            "front stack or product depth outside structural bounds")
    searches = work["witness_searches"]
    require(searches <= splits + killed + rectangles and
            searches <= work["witness_descent_steps"] <= 48 * searches and
            work["witness_box_distance_tests"] == 2 * work["witness_descent_steps"] and
            searches <= work["proposed_sites"] <= min(kmax, n) * searches and
            work["h_bound_tests"] + work["proposals_in_factors"] == work["proposed_sites"] and
            work["xi_bound_tests"] <= work["h_bound_tests"] and
            work["witness_lane_credits"] <= min(kmax, 3) * work["h_bound_tests"],
            "witness proposal or certification work outside its declared bound")
    if kmax == 1:
        require(work["xi_bound_tests"] == 0, "inactive higher lanes paid Xi")
    if row["front_mode"] == "pure":
        require(all(work[field] == 0 for field in FRONT_FIELDS[5:14]) and
                work["rejected_pair_mass"] == [0, 0, 0], "pure front performed rejection work")
    require(sum(work["size_class_rectangles"]) == rectangles and
            work["size_class_rectangles"][0] == work["leaf_pair_rectangles"] and
            work["size_class_pair_mass"][0] == work["leaf_pair_rectangles"],
            "size classes lost rectangles or singleton pair mass")
    union_mass = sum(work["size_class_pair_mass"])
    require(rectangles <= union_mass <= total and 2 * rectangles <= work["emitted_factor_sites"] <= n * rectangles and
            (rectangles == 0) == (work["max_factor_size"] == 0) and work["max_factor_size"] < n,
            "residual union or factor cardinality mismatch")
    for count, mass, minimum, maximum in zip(work["size_class_rectangles"], work["size_class_pair_mass"],
            (1, 2, 8, 64, 1024), (1, 7, 63, 1023, n - 1)):
        require((count == 0) == (mass == 0) and minimum * count <= mass <= maximum * maximum * count,
                "size class mass outside possible factor cardinalities")
    for lane in range(3):
        rejected, residual, count = (work[field][lane] for field in
            ("rejected_pair_mass", "residual_pair_mass", "lane_rectangles"))
        require(rejected + residual == (total if active & (1 << lane) else 0) and
                count <= rectangles and count <= residual <= union_mass and (count == 0) == (residual == 0),
                "lane mass ledger or rectangle count mismatch")
        if row["front_mode"] == "pure" and active & (1 << lane):
            require(count == rectangles, "pure front lost an active lane mask")
    require(rectangles <= sum(work["lane_rectangles"]) <= min(kmax, 3) * rectangles,
            "terminal mask population mismatch")
    output = row.get("digest")
    require(type(output) is dict and set(output) == {"rectangles", "factor_sites", "pair_mass",
            "lane_rectangles", "lane_pair_mass", "sum", "xor"}, "wrong terminal digest fields")
    for field in ("rectangles", "factor_sites", "pair_mass"):
        uint(output[field], field)
    array(output["lane_rectangles"], 3, "digest.lane_rectangles")
    array(output["lane_pair_mass"], 3, "digest.lane_pair_mass")
    fingerprint(output["sum"], "digest.sum")
    fingerprint(output["xor"], "digest.xor")
    require(output["rectangles"] == rectangles and output["factor_sites"] == work["emitted_factor_sites"] and
            output["pair_mass"] == union_mass and output["lane_rectangles"] == work["lane_rectangles"] and
            output["lane_pair_mass"] == work["residual_pair_mass"], "streamed digest disagrees with front ledger")
    memory = row.get("memory")
    counters(memory, MEMORY_FIELDS, "memory")
    require(6 * n <= memory["input_capacity_bytes"] and 6 * n <= memory["cloud_retained_bytes"] <= 128 * n and
            8 * n <= memory["index_retained_bytes"] <= 256 * n, "declared retained vector memory outside bounds")
    timings = row.get("timings")
    require(type(timings) is dict and set(timings) == set(TIMES), "wrong timing fields")
    finite_times(timings, TIMES)
    require(timings["total_ms"] + 1e-6 >= sum(timings[field] for field in TIMES[:-1]), "total clock omitted work")


def matrix(manifest: dict[str, Any]) -> list[tuple[Any, ...]]:
    require(uint(manifest.get("repeats"), "repeats") >= 1, "empty repetitions")
    for name in MATRIX_KEYS:
        values = manifest.get(name)
        require(type(values) is list and bool(values), f"{name}: empty matrix")
        for value in values:
            if name in ("families", "modes"):
                require(type(value) is str and value in (FAMILIES if name == "families" else MODES), f"{name}: invalid value")
            else:
                uint(value, name)
                require(value >= (2 if name == "sizes" else 0 if name == "seeds" else 1) and
                        (name != "kmax" or value <= 10) and (name != "separations" or value < 1 << 32),
                        f"{name}: invalid value")
        require(len(set(values)) == len(values), f"{name}: duplicate value")
    cases = list(itertools.product(*(manifest[name] for name in MATRIX_KEYS), range(manifest["repeats"])))
    for family, n, *_ in cases:
        limits = dict(uniform=1 << 48, terrain=1 << 40, clusters=1 << 33, rows=131072)
        require(n <= limits[family] and (family != "rows" or n % 2 == 0), "fixture outside its finite domain")
    return cases


def cross_check(row: dict[str, Any], identities: dict, signatures: dict) -> None:
    identity = row["family"], row["n"], row["seed"] if row["seed_affects_input"] else None
    value = {name: row[name] for name in ("input_hash", "total_unordered_pairs", "generation_work", "cloud_work", "index_work", "memory")}
    require(identity not in identities or identities[identity] == value,
            "same fixture changed across K, separation, mode or repetition")
    identities[identity] = value
    key = tuple(row[name] for name in KEYS)
    signature = {name: row[name] for name in DISCRETE_FIELDS}
    require(key not in signatures or signatures[key] == signature, "discrete work changed across identical repetitions")
    signatures[key] = signature


def campaign_directories(root: Path) -> list[Path]:
    require(root.is_dir(), "receipt root is not a directory")
    directories = sorted({path.parent for name in RECEIPT_FILES for path in root.rglob(name)})
    require(bool(directories), "no WSPD front campaigns")
    require(not any(parent in child.parents for parent in directories for child in directories), "nested campaign roots")
    for directory in directories:
        require(all((directory / name).is_file() for name in RECEIPT_FILES), f"incomplete campaign triplet: {directory}")
    return directories


def capture(args: argparse.Namespace) -> int:
    parameters = dict(families=args.families, sizes=args.sizes, kmax=args.kmax, separations=args.s,
                      seeds=args.seeds, modes=args.modes, repeats=args.repeats)
    cases = matrix(parameters)
    args.output.mkdir(parents=True, exist_ok=False)
    binary = args.probe.resolve()
    pins, binary_pin = {}, None
    attempts = completed = 0
    status, error, code = "failed", "campaign did not start", 1
    previous = {signum: signal.signal(signum, on_signal) for signum in (signal.SIGINT, signal.SIGTERM)}
    try:
        pins, binary_pin = sources(), digest(binary)
        cache = (binary.parent / "CMakeCache.txt").read_text()
        compilers = [line.split("=", 1)[1] for line in cache.splitlines() if line.startswith("CMAKE_CXX_COMPILER:") and "=" in line]
        require(len(compilers) == 1 and bool(compilers[0]), "missing compiler provenance")
        manifest = {**parameters, "schema": SCHEMA, "scope": SCOPE, "public_status": "not_claimed",
            "separation_convention": "box_gap_diameter_v1", "started_utc": utc_stamp(),
            "threads": 1, "gcp_used": False, "processes_sequential": True, "warmup_runs": 0,
            "probe": str(binary), "probe_sha256": binary_pin, "source_sha256": pins, "cmake_cache": cache,
            "compiler_version": subprocess.check_output([compilers[0], "--version"], text=True),
            "platform": platform.platform(), "logical_cpu_count": os.cpu_count(),
            "cpuinfo": Path("/proc/cpuinfo").read_text().split("\n\n", 1)[0],
            "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
            "worktree_status": subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True),
            "runner_command": [sys.executable, *sys.argv]}
        write_json(args.output / "MANIFEST.json", manifest)
        environment = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
        identities, signatures = {}, {}
        with (args.output / "MEASURES.jsonl").open("x") as stream:
            for *key, repeat in cases:
                command = command_for(str(binary), tuple(key))
                record = dict(command=command, repeat=repeat, status="failed", exit_code=None,
                              stdout="", stderr="", stdout_base64="", stderr_base64="", started_utc=utc_stamp())
                attempts += 1
                try:
                    record["probe_sha256_before"] = digest(binary)
                    require(record["probe_sha256_before"] == binary_pin, "binary changed before invocation")
                    print(json.dumps({"starting": command[1:], "repeat": repeat}), flush=True)
                    invoke(command, environment, ROOT, record)
                    record["probe_sha256_after"] = digest(binary)
                    require(record["probe_sha256_after"] == binary_pin, "binary changed during invocation")
                    if record["exit_code"] != 0:
                        raise RuntimeError(f"probe exited {record['exit_code']}")
                    require(not record["stderr_base64"], "probe wrote unexpected stderr")
                    row = parse_result(base64.b64decode(record["stdout_base64"], validate=True))
                    validate_result(row, command)
                    cross_check(row, identities, signatures)
                    record.update(status="completed", result=row)
                    completed += 1
                except KeyboardInterrupt as cause:
                    record.update(status="interrupted", error=str(cause))
                    raise
                except Exception as cause:
                    record.update(status="invalid" if isinstance(cause, ValueError) else "failed", error=str(cause))
                    raise
                finally:
                    record["finished_utc"] = utc_stamp()
                    stream.write(json.dumps(record, allow_nan=False) + "\n")
                    stream.flush()
        status, error, code = "completed", "", 0
    except KeyboardInterrupt as cause:
        status, error, code = "interrupted", str(cause), 128 + getattr(cause, "signum", signal.SIGINT)
    except Exception as cause:
        status, error = "invalid" if isinstance(cause, ValueError) else "failed", str(cause)
    finally:
        closing = {}
        for name in pins:
            try:
                closing[name] = digest(ROOT / name)
            except OSError:
                closing[name] = None
        try:
            binary_closing = digest(binary)
        except OSError:
            binary_closing = None
        source_same, binary_same = bool(pins) and pins == closing, binary_pin is not None and binary_pin == binary_closing
        if status == "completed" and not (source_same and binary_same):
            status, error, code = "invalid", "source or binary changed at closure", 1
        write_json(args.output / "COMPLETION.json", dict(status=status, error=error, runs=completed, attempts=attempts,
            finished_utc=utc_stamp(), source_hashes_unchanged=source_same, probe_hash_unchanged=binary_same,
            source_sha256_closing=closing, probe_sha256_closing=binary_closing))
        for signum, handler in previous.items():
            signal.signal(signum, handler)
    print(json.dumps(dict(status=status, runs=completed, attempts=attempts, error=error)))
    return code


def check(args: argparse.Namespace) -> int:
    directories = campaign_directories(args.receipt)
    pins = sources()
    identities, signatures, groups = {}, {}, defaultdict(list)
    common_provenance = published = None
    measures = 0
    for directory in directories:
        manifest = parse_result((directory / "MANIFEST.json").read_bytes())
        completion = parse_result((directory / "COMPLETION.json").read_bytes())
        require(manifest.get("schema") == SCHEMA and manifest.get("scope") == SCOPE and
                manifest.get("public_status") == "not_claimed" and manifest.get("gcp_used") is False and
                manifest.get("separation_convention") == "box_gap_diameter_v1" and
                uint(manifest.get("threads"), "threads") == 1 and manifest.get("processes_sequential") is True and
                uint(manifest.get("warmup_runs"), "warmup_runs") == 0, "wrong campaign scope")
        require(manifest.get("source_sha256") == pins, "source coverage/hash differs from current version")
        sha256(manifest.get("probe_sha256"), "probe_sha256")
        require(type(manifest.get("probe")) is str and Path(manifest["probe"]).is_absolute(), "missing absolute binary path")
        require(type(manifest.get("commit")) is str and re.fullmatch(r"[0-9a-f]{40}", manifest["commit"]) is not None and
                type(manifest.get("worktree_status")) is str and type(manifest.get("runner_command")) is list and
                bool(manifest["runner_command"]) and all(type(arg) is str for arg in manifest["runner_command"]),
                "missing repository/capture provenance")
        build, machine, published = provenance(manifest)
        require(common_provenance is None or common_provenance == (build, machine), "heterogeneous build/machine provenance")
        common_provenance = build, machine
        start, end = timestamp(manifest.get("started_utc"), "campaign start"), timestamp(completion.get("finished_utc"), "campaign end")
        require(start <= end and completion.get("status") == "completed" and completion.get("source_hashes_unchanged") is True and
                completion.get("probe_hash_unchanged") is True and completion.get("source_sha256_closing") == pins and
                completion.get("probe_sha256_closing") == manifest["probe_sha256"], "invalid campaign closure")
        expected = set(matrix(manifest))
        records = [parse_result(line) for line in (directory / "MEASURES.jsonl").read_bytes().splitlines()]
        require(uint(completion.get("runs"), "runs") == uint(completion.get("attempts"), "attempts") ==
                len(records) == len(expected), "incomplete matrix")
        seen, previous_end = set(), start
        for record in records:
            begun, finished = timestamp(record.get("started_utc"), "invocation start"), timestamp(record.get("finished_utc"), "invocation end")
            require(previous_end <= begun <= finished <= end, "invocations are not serial inside campaign timestamps")
            previous_end = finished
            require(record.get("status") == "completed" and uint(record.get("exit_code"), "exit_code") == 0 and
                    record.get("stderr") == record.get("stderr_base64") == "" and
                    record.get("probe_sha256_before") == record.get("probe_sha256_after") == manifest["probe_sha256"],
                    "invalid invocation or binary pin")
            raw = base64.b64decode(record["stdout_base64"], validate=True)
            row = parse_result(raw)
            key = tuple(row[name] for name in KEYS)
            command = command_for(manifest["probe"], key)
            require(raw.decode() == record.get("stdout") and row == record.get("result") and
                    record.get("command") == command, "raw/parsed/command mismatch")
            validate_result(row, command)
            identity = (*key, uint(record.get("repeat"), "repeat"))
            require(identity not in seen, "duplicate invocation")
            seen.add(identity)
            cross_check(row, identities, signatures)
            groups[key].append(row)
            measures += 1
        require(seen == expected, "missing or extra matrix tuple")
    require(sources() == pins, "sources changed while reading")
    result = dict(status="passed", campaigns=len(directories), measurements=measures,
                  configurations_including_mode_seed=len(groups), scope=SCOPE, full_contract_qualified=False,
                  gcp_used=False, provenance_policy=PROVENANCE_POLICY, provenance=published,
                  current_reader_sha256=pins[RUNNER_SOURCE], source_check="current_content_and_closing_pins_not_a_rebuild",
                  mode_comparison="same_input_and_initial_mass_not_equal_descriptors")
    if args.summary:
        result["summary"] = []
        for key, rows in sorted(groups.items()):
            item = dict(zip(KEYS, key), repeats=len(rows), **{name: rows[0][name] for name in DISCRETE_FIELDS})
            item["median_timings"] = {field: statistics.median(row["timings"][field] for row in rows) for field in TIMES}
            result["summary"].append(item)
    print(json.dumps(result, indent=2, allow_nan=False))
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    run = sub.add_parser("run")
    run.add_argument("--probe", required=True, type=Path)
    run.add_argument("--output", required=True, type=Path)
    run.add_argument("--sizes", nargs="+", type=int, default=[8000, 16000, 32000])
    run.add_argument("--families", nargs="+", choices=FAMILIES, default=list(FAMILIES))
    run.add_argument("--kmax", nargs="+", type=int, default=[5, 10])
    run.add_argument("--s", nargs="+", type=int, default=[8, 10, 12])
    run.add_argument("--seeds", nargs="+", type=int, default=[3])
    run.add_argument("--modes", nargs="+", choices=MODES, default=list(MODES))
    run.add_argument("--repeats", type=int, default=1)
    reader = sub.add_parser("check")
    reader.add_argument("receipt", type=Path)
    reader.add_argument("--summary", action="store_true")
    args = parser.parse_args()
    try:
        return capture(args) if args.operation == "run" else check(args)
    except (ValueError, OSError, KeyError, TypeError) as error:
        if args.operation == "run":
            parser.error(str(error))
        print(f"WSPD front receipt rejected: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
