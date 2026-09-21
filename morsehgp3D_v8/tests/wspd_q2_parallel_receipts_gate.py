#!/usr/bin/env python3
"""Tiny real parallel captures, strict receipt mutants and preserved failures."""

from __future__ import annotations

import argparse
import base64
from collections import Counter
import copy
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from typing import Any, Callable

ROOT = Path(__file__).resolve().parents[2]
BENCH = ROOT / "morsehgp3D_v8/bench"
RUNNER = BENCH / "run_wspd_q2_parallel_matrix.py"
sys.path.insert(0, str(BENCH))
from run_wspd_q2_parallel_matrix import RUNNER_SOURCE, digest, matrix, parse_result, sources, validate_result  # noqa: E402


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def command(*args: str) -> list[str]:
    return [sys.executable, "-B", *(["-O"] if sys.flags.optimize else []), str(RUNNER), *args]


def read(path: Path) -> dict[str, Any]:
    return parse_result(path.read_bytes())


def write(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, allow_nan=False) + "\n")


def records(path: Path) -> list[dict[str, Any]]:
    return [parse_result(line) for line in (path / "MEASURES.jsonl").read_bytes().splitlines()]


def write_records(path: Path, rows: list[dict[str, Any]]) -> None:
    (path / "MEASURES.jsonl").write_text("".join(json.dumps(row, allow_nan=False) + "\n" for row in rows))


def refresh(record: dict[str, Any]) -> None:
    raw = (json.dumps(record["result"], allow_nan=False) + "\n").encode()
    record["stdout"] = raw.decode()
    record["stdout_base64"] = base64.b64encode(raw).decode("ascii")


def capture(probe: Path, output: Path, edge: bool = False) -> subprocess.CompletedProcess:
    options = ["--sizes", "2", "--families", "uniform", "--threads", "0", "4",
               "--jobs-per-worker", "16", "--pool-min-factors", "64"] if edge else [
                   "--sizes", "32", "--families", "clusters", "rows", "--threads", "0", "1", "2",
                   "--jobs-per-worker", "1", "4", "--pool-min-factors", "1", "64"]
    return subprocess.run(command("run", "--probe", str(probe), "--output", str(output),
        *options, "--kmax", "1", "--s", "8", "--seeds", "3", "--repeats", "1"),
        capture_output=True, cwd=ROOT)


def checked(root: Path) -> subprocess.CompletedProcess:
    return subprocess.run(command("check", str(root), "--summary"), capture_output=True, cwd=ROOT)


def mutate_worker_digest(row: dict[str, Any]) -> None:
    # As in the dynamic receipt gate: an idle worker legitimately has sum=0.
    # Changing one bit is causal even in that case and remains a valid u64.
    value = int(row["workers"][0]["digest"]["sum"], 16)
    require(0 <= value < 1 << 64, "worker digest mutation requires a u64")
    row["workers"][0]["digest"]["sum"] = format(value ^ 1, "x")


def check_worker_digest_mutation(record: dict[str, Any]) -> None:
    for value in (0, 1, (1 << 64) - 1):
        row = copy.deepcopy(record["result"])
        previous = int(row["workers"][0]["digest"]["sum"], 16)
        total = (int(row["digest"]["sum"], 16) - previous + value) & ((1 << 64) - 1)
        row["workers"][0]["digest"]["sum"] = format(value, "x")
        row["digest"]["sum"] = format(total, "x")
        validate_result(row, record["command"])
        mutate_worker_digest(row)
        changed = int(row["workers"][0]["digest"]["sum"], 16)
        require(changed != value and 0 <= changed < 1 << 64,
                "worker digest mutation was unchanged or outside u64")
        try:
            validate_result(row, record["command"])
        except ValueError as error:
            require(str(error) == "worker canonical digest reduction mismatch",
                    "worker digest mutation failed for an unrelated reason")
        else:
            raise RuntimeError("worker digest reduction accepted a changed u64")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    original = args.probe.resolve()
    pins, binary_pin = sources(), digest(original)
    stats: Counter[str] = Counter()
    domain = dict(families=["rows"], sizes=[131072], kmax=[10], separations=[8], seeds=[3],
                  thread_counts=[0, 2], jobs_per_worker=[1], pool_min_factors=[64], repeats=1)
    require(len(matrix(domain)) == 2, "physical rows domain rejected")
    stats["domain_checks"] += 1
    for replacement in ({"sizes": [131074]}, {"thread_counts": [-1]}, {"thread_counts": [True]},
                        {"jobs_per_worker": [0]}, {"kmax": [0]}, {"separations": [0]},
                        {"thread_counts": [1 << 63], "jobs_per_worker": [3]}):
        try:
            matrix({**domain, **replacement})
        except ValueError:
            stats["domain_checks"] += 1
        else:
            raise RuntimeError("invalid matrix survived")
    with tempfile.TemporaryDirectory(prefix="mhgp8_parallel_receipts_") as name:
        temporary = Path(name)
        binary = temporary / "probe"
        shutil.copy2(original, binary)
        shutil.copy2(original.parent / "CMakeCache.txt", temporary / "CMakeCache.txt")
        genuine = temporary / "genuine"
        genuine.mkdir()
        for label in ("first", "second", "edge"):
            directory = genuine / label
            process = capture(binary, directory, label == "edge")
            require(process.returncode == 0 and not process.stderr,
                    f"genuine capture failed: {process.stdout!r} {process.stderr!r}")
            rows, completion = records(directory), read(directory / "COMPLETION.json")
            expected = 2 if label == "edge" else 24
            require(completion["status"] == "completed" and completion["runs"] == completion["attempts"] == len(rows) == expected,
                    "capture lost a configuration")
            require(read(directory / "MANIFEST.json")["source_sha256"] == pins, "capture lost source pins")
            for record in rows:
                require(record["result"]["accepted_pairs"] > 0, "empty support fixture")
                for key in ("stdout", "stderr"):
                    require(base64.b64decode(record[key + "_base64"], validate=True).decode("utf-8", errors="replace") == record[key],
                            "capture lost raw bytes")
            stats["genuine_rows"] += len(rows)
        rows = records(genuine / "first")
        require(any(row["result"]["pool_work"]["filtered_pairs"] > 0 for row in rows), "Pool filtering fixture vacuous")
        require(any(row["result"]["pool_work"]["passthrough_rectangles"] > 0 for row in rows), "Pool passthrough fixture vacuous")
        require(any(row["result"]["parallel_work"]["started_workers"] < row["result"]["threads"]
                    for row in records(genuine / "edge")), "workers greater than jobs fixture vacuous")
        evidence = {path: digest(path) for path in genuine.rglob("*") if path.is_file()}
        process = checked(genuine)
        require(process.returncode == 0 and not process.stderr, f"genuine reader failed: {process.stderr!r}")
        summary = parse_result(process.stdout)
        require(summary["measurements"] == 50 and summary["configurations_including_threads_seed"] == 26 and
                len(summary["summary"]) == 26 and summary["full_contract_qualified"] is False and
                summary["gcp_used"] is False and summary["current_reader_sha256"] == pins[RUNNER_SOURCE],
                "reader lost scope, matrix or hashes")
        require(all(digest(path) == pin for path, pin in evidence.items()), "reader modified evidence")
        stats["positive_reads"] += 1

        def mutant(label: str, change: Callable[[Path], None]) -> None:
            destination = temporary / label
            shutil.copytree(genuine, destination)
            change(destination)
            result = checked(destination)
            require(result.returncode == 1, f"mutant survived: {label} {result.stdout!r} {result.stderr!r}")
            stats["mutants"] += 1

        def change_document(root: Path, name: str, update: Callable[[dict], None]) -> None:
            path = root / "first" / name
            value = read(path)
            update(value)
            write(path, value)

        def change_record(root: Path, update: Callable[[dict], None], parallel: bool = False,
                          refresh_raw: bool = True) -> None:
            directory = root / "first"
            values = records(directory)
            position = next(i for i, record in enumerate(values) if
                            (record["result"]["threads"] == 2 if parallel else record["result"]["threads"] == 0))
            update(values[position])
            if refresh_raw:
                refresh(values[position])
            write_records(directory, values)

        def mutate_row(root: Path, update: Callable[[dict], None], parallel: bool = False) -> None:
            change_record(root, lambda record: update(record["result"]), parallel)

        for filename, label in (("morsehgp3D_v8/src/pipeline/q2_census.cpp", "engine_pin"),
                                ("morsehgp3D_v8/bench/parallel_probe_common.hpp", "callback_pin"),
                                ("morsehgp3D_v8/bench/run_p0_matrix.py", "collector_pin")):
            mutant(label, lambda root, filename=filename: change_document(root, "MANIFEST.json",
                   lambda row: row["source_sha256"].pop(filename)))
        for field, value in (("compiler_version", "different compiler"), ("gcp_used", True),
                             ("threads", 1), ("worktree_status", None), ("runner_command", [])):
            mutant("manifest_" + field, lambda root, field=field, value=value: change_document(
                root, "MANIFEST.json", lambda row: row.update({field: value})))
        mutant("machine", lambda root: change_document(root, "MANIFEST.json",
               lambda row: row.update(logical_cpu_count=row["logical_cpu_count"] + 1)))
        mutant("affinity", lambda root: change_document(root, "MANIFEST.json",
               lambda row: row.update(cpu_affinity=[max(row["cpu_affinity"]) + 1])))
        mutant("flags", lambda root: change_document(root, "MANIFEST.json",
               lambda row: row.update(cmake_cache=row["cmake_cache"] + "\nCMAKE_CXX_FLAGS:STRING=-ffast-math\n")))
        for field, value in (("status", "failed"), ("attempts", 999), ("probe_sha256_closing", "0" * 64)):
            mutant("closure_" + field, lambda root, field=field, value=value: change_document(
                root, "COMPLETION.json", lambda row: row.update({field: value})))
        mutant("source_closure", lambda root: change_document(root, "COMPLETION.json",
               lambda row: row["source_sha256_closing"].update({RUNNER_SOURCE: "0" * 64})))
        mutant("wrong_command", lambda root: change_record(root, lambda row: row["command"].__setitem__(6, "9")))
        mutant("wrong_raw", lambda root: change_record(root, lambda row: row.update(stdout="{}\n"), refresh_raw=False))
        mutant("binary_after", lambda root: change_record(root, lambda row: row.update(probe_sha256_after="0" * 64)))
        mutant("boolean_exit", lambda root: change_record(root, lambda row: row.update(exit_code=False)))
        mutant("nonserial", lambda root: change_record(root, lambda row: row.update(finished_utc="2000-01-01T00:00:00+00:00")))
        mutant("bad_mode", lambda root: mutate_row(root, lambda row: row.update(front_mode="pure")))
        mutant("bad_tuple", lambda root: mutate_row(root, lambda row: row.update(jobs_per_worker=0)))
        mutant("input_hash", lambda root: mutate_row(root, lambda row: row.update(input_hash="0")))
        mutant("lost_mass", lambda root: mutate_row(root, lambda row: row.update(candidate_pairs=0)))
        mutant("lost_lane", lambda root: mutate_row(root, lambda row: row["front_work"].update(residual_pair_mass=[0, 0, 0])))
        mutant("boolean_counter", lambda root: mutate_row(root, lambda row: row["census_work"].update(query_tasks=True)))
        mutant("lost_field", lambda root: mutate_row(root, lambda row: row["census_work"].pop("cursor_reuses")))
        mutant("logical_stack", lambda root: mutate_row(root, lambda row: row["front_work"].update(
               max_stack_size=row["front_work"]["max_stack_size"] + 1)))
        mutant("plan_maximum", lambda root: mutate_row(root, lambda row: row["pool_work"].update(
               plan_peak_bytes=row["pool_work"]["plan_peak_bytes"] + 1)))
        mutant("lost_job", lambda root: mutate_row(root, lambda row: row["parallel_work"].update(
               completed_jobs=row["parallel_work"]["completed_jobs"] - 1), True))
        mutant("duplicate_job", lambda root: mutate_row(root, lambda row: row["workers"][0].update(
               jobs=row["workers"][0]["jobs"] + 1), True))
        mutant("duplicate_worker", lambda root: mutate_row(root, lambda row: row["workers"].append(copy.deepcopy(row["workers"][0])), True))
        check_worker_digest_mutation(next(row for row in records(genuine / "first")
                                         if row["result"]["threads"] == 2))
        mutant("worker_digest", lambda root: mutate_row(root, mutate_worker_digest, True))
        mutant("unpaid_callback", lambda root: mutate_row(root, lambda row: row["callback_work"].update(copied_ids=0)))
        mutant("bad_total_time", lambda root: mutate_row(root, lambda row: row["timings"].update(total_ms=0)))
        mutant("wall_minus_payload", lambda root: mutate_row(root, lambda row: row["timings"].update(
               pipeline_wall_ms=row["timings"]["pipeline_wall_ms"] - row["timings"]["payload_ms_sum"])))
        mutant("invented_count_time", lambda root: mutate_row(root, lambda row: row["timings"].update(
               front_and_count_ms=row["timings"]["pipeline_wall_ms"] - row["timings"]["payload_ms_sum"])))
        mutant("wrong_worker_sum", lambda root: mutate_row(root, lambda row: row["timings"].update(worker_ms_sum=0), True))

        def duplicate_record(root: Path) -> None:
            values = records(root / "first")
            values[1] = copy.deepcopy(values[0])
            write_records(root / "first", values)
        mutant("duplicate_record", duplicate_record)
        mutant("missing_triplet", lambda root: (root / "first/MANIFEST.json").unlink())

        def orphan(root: Path) -> None:
            (root / "orphan").mkdir()
            write(root / "orphan/COMPLETION.json", {"status": "failed"})
        mutant("orphan_failure", orphan)
        mutant("nested", lambda root: shutil.copytree(root / "edge", root / "first/nested"))

        for suffix in ([], ["32", "rows", "0", "8", "3", "2", "4"],
                       ["32", "rows", "1", "0", "3", "2", "4"],
                       ["32", "rows", "1", "8", "3", "2", "0"],
                       ["32", "rows", "1", "8", "3", "-1", "4"],
                       ["33", "rows", "1", "8", "3", "2", "4"]):
            process = subprocess.run([str(binary), *suffix], capture_output=True, cwd=ROOT)
            require(process.returncode == 2 and not process.stdout and process.stderr, "invalid probe options not rejected")
            stats["invalid_options"] += 1
        failed_probe = temporary / "false_probe"
        shutil.copy2("/bin/false", failed_probe)
        failed = temporary / "failed"
        process = capture(failed_probe, failed)
        failed_rows = records(failed)
        completion = read(failed / "COMPLETION.json")
        require(process.returncode == 1 and completion["status"] == "failed" and completion["runs"] == 0 and
                completion["attempts"] == len(failed_rows) == 1 and failed_rows[0]["exit_code"] == 1 and
                checked(failed).returncode == 1, "failed execution was lost/promoted")
        stats["failed_captures"] += 1
        startup = temporary / "startup"
        process = capture(temporary / "absent", startup)
        require(process.returncode == 1 and read(startup / "COMPLETION.json")["attempts"] == 0 and
                checked(startup).returncode == 1, "startup failure was hidden")
        stats["failed_captures"] += 1
    require(stats["genuine_rows"] == 50 and stats["positive_reads"] == 1 and stats["mutants"] >= 42 and
            stats["failed_captures"] == 2 and stats["invalid_options"] == 6 and stats["domain_checks"] == 8,
            "parallel receipt gate lost non-vacuity")
    require(sources() == pins and digest(original) == binary_pin, "gate modified sources or probe")
    print(json.dumps(dict(status="passed", **stats), sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, RuntimeError, OSError, KeyError, TypeError) as error:
        print(f"parallel q2 receipt gate failed: {error}", file=sys.stderr)
        raise SystemExit(1) from error
