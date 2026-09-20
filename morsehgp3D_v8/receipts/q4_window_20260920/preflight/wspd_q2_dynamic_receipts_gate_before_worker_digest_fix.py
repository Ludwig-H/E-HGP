#!/usr/bin/env python3
"""Tiny coarse/donation captures, strict receipt mutants and preserved failures.

Explicit protocol port of the parallel receipt gate at 4c0bfe1e.
"""

from __future__ import annotations

import argparse
import base64
from collections import Counter
import copy
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
from typing import Any, Callable

ROOT = Path(__file__).resolve().parents[2]
BENCH = ROOT / "morsehgp3D_v8/bench"
RUNNER = BENCH / "run_wspd_q2_dynamic_matrix.py"
sys.path.insert(0, str(BENCH))
from run_wspd_q2_dynamic_matrix import RUNNER_SOURCE, digest, matrix, parse_result, sources  # noqa: E402


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
        *options, "--kmax", "1", "--s", "8", "--seeds", "3", "--repeats", "1",
        "--schedules", "coarse", "donate", "--queue-capacities", "1", "--donation-intervals", "1"),
        capture_output=True, cwd=ROOT)


def checked(root: Path) -> subprocess.CompletedProcess:
    return subprocess.run(command("check", str(root), "--summary"), capture_output=True, cwd=ROOT)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    original = args.probe.resolve()
    pins, binary_pin = sources(), digest(original)
    stats: Counter[str] = Counter()
    domain = dict(families=["rows"], sizes=[131072], kmax=[10], separations=[8], seeds=[3],
                  thread_counts=[0, 2], jobs_per_worker=[1], schedules=["donate"],
                  queue_capacities=[1], donation_intervals=[1], pool_min_factors=[64], repeats=1)
    require(len(matrix(domain)) == 2, "physical rows domain rejected")
    stats["domain_checks"] += 1
    for replacement in ({"sizes": [131074]}, {"thread_counts": [-1]}, {"thread_counts": [True]},
                        {"jobs_per_worker": [0]}, {"kmax": [0]}, {"separations": [0]},
                        {"thread_counts": [1 << 63], "jobs_per_worker": [3]},
                        {"schedules": ["invalid"]}, {"queue_capacities": [0]},
                        {"donation_intervals": [0]}, {"donation_intervals": [True]}):
        try:
            matrix({**domain, **replacement})
        except ValueError:
            stats["domain_checks"] += 1
        else:
            raise RuntimeError("invalid matrix survived")
    with tempfile.TemporaryDirectory(prefix="mhgp8_dynamic_receipts_") as name:
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
            expected = 4 if label == "edge" else 48
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
        file_a, file_b = temporary / 'points with "quote".u16le', temporary / "second.u16le"
        points_a = [(0, 0, 0), (5, 0, 0), (10, 0, 0), (11, 0, 0)]
        points_b = [(1000, 0, 0), (0, 0, 0), (0, 1, 0), (300, 17, 7), (65535, 65535, 65535), (1, 2, 3)]
        file_a.write_bytes(b"".join(struct.pack("<HHH", *point) for point in points_a))
        file_b.write_bytes(b"".join(struct.pack("<HHH", *point) for point in points_b))
        process = subprocess.run(command("run", "--probe", str(binary), "--output", str(genuine / "files"),
            "--input-files", str(file_a), str(file_b), "--kmax", "2", "--s", "8", "--threads", "0", "2",
            "--jobs-per-worker", "1", "--schedules", "coarse", "donate", "--queue-capacities", "1",
            "--donation-intervals", "1", "--pool-min-factors", "1"), capture_output=True, cwd=ROOT)
        require(process.returncode == 0 and not process.stderr, f"file capture failed: {process.stdout!r} {process.stderr!r}")
        file_rows = records(genuine / "files")
        require(len(file_rows) == 8 and {row["result"]["n"] for row in file_rows} == {4, 6} and
                all(row["result"]["input_kind"] == "file_u16le" and row["result"]["seed"] == 0 and
                    row["result"]["timings"]["generation_ms"] == 0 for row in file_rows),
                "file size pairing or source identity lost")
        stats["genuine_rows"] += len(file_rows)
        evidence = {path: digest(path) for path in genuine.rglob("*") if path.is_file()}
        process = checked(genuine)
        require(process.returncode == 0 and not process.stderr, f"genuine reader failed: {process.stderr!r}")
        summary = parse_result(process.stdout)
        require(summary["measurements"] == 108 and summary["configurations_including_threads_seed"] == 60 and
                len(summary["summary"]) == 60 and summary["full_contract_qualified"] is False and
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
                                ("morsehgp3D_v8/bench/dynamic_probe_common.hpp", "input_adapter_pin"),
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
        mutant("worker_digest", lambda root: mutate_row(root, lambda row: row["workers"][0]["digest"].update(sum="0"), True))
        mutant("unpaid_callback", lambda root: mutate_row(root, lambda row: row["callback_work"].update(copied_ids=0)))
        mutant("bad_total_time", lambda root: mutate_row(root, lambda row: row["timings"].update(total_ms=0)))
        mutant("wall_minus_payload", lambda root: mutate_row(root, lambda row: row["timings"].update(
               pipeline_wall_ms=row["timings"]["pipeline_wall_ms"] - row["timings"]["payload_ms_sum"])))
        mutant("invented_count_time", lambda root: mutate_row(root, lambda row: row["timings"].update(
               front_and_count_ms=row["timings"]["pipeline_wall_ms"] - row["timings"]["payload_ms_sum"])))
        mutant("wrong_worker_sum", lambda root: mutate_row(root, lambda row: row["timings"].update(worker_ms_sum=0), True))
        mutant("dispatch_field_omitted", lambda root: mutate_row(root, lambda row: row["dispatch_work"].pop("offer_busy"), True))
        mutant("dispatch_boolean", lambda root: mutate_row(root, lambda row: row["dispatch_work"].update(donations=True), True))
        mutant("wrong_schedule", lambda root: mutate_row(root, lambda row: row.update(schedule="invalid"), True))
        mutant("queue_capacity_zero", lambda root: mutate_row(root, lambda row: row.update(queue_capacity=0), True))
        mutant("donation_interval_zero", lambda root: mutate_row(root, lambda row: row.update(donation_interval=0), True))
        mutant("input_pin_omitted", lambda root: change_record(root, lambda row: row.pop("input_sha256_after")))
        mutant("input_pin_changed", lambda root: change_record(root, lambda row: row.update(input_sha256_after="0" * 64)))
        mutant("input_closure", lambda root: change_document(root, "COMPLETION.json", lambda row: row.update(input_hashes_unchanged=False)))
        mutant("load_vs_generation", lambda root: mutate_row(root, lambda row: row["timings"].update(load_ms=1)))

        def donated_mutant(root: Path, update: Callable[[dict], None]) -> None:
            directory = root / "first"
            values = records(directory)
            position = next(i for i, record in enumerate(values)
                            if record["result"]["threads"] == 2 and record["result"]["schedule"] == "donate")
            update(values[position]["result"])
            refresh(values[position])
            write_records(directory, values)
        mutant("lost_seed", lambda root: donated_mutant(root, lambda row: row["dispatch_work"].update(seeds_completed=0)))
        mutant("invented_donation", lambda root: donated_mutant(root, lambda row: row["dispatch_work"].update(
               donations=row["dispatch_work"]["donations"] + 1)))
        mutant("lost_stolen_job", lambda root: donated_mutant(root, lambda row: row["dispatch_work"].update(
               stolen_completed=row["dispatch_work"]["stolen_completed"] + 1)))
        mutant("queue_overflow", lambda root: donated_mutant(root, lambda row: row["dispatch_work"].update(max_queue_size=2)))
        mutant("physical_stack", lambda root: donated_mutant(root, lambda row: row["dispatch_work"].update(max_local_stack_size=98)))
        mutant("unpaid_queue", lambda root: donated_mutant(root, lambda row: row["parallel_work"].update(queue_storage_bytes=0)))

        def file_manifest(root: Path) -> None:
            path = root / "files/MANIFEST.json"
            value = read(path)
            next(iter(value["input_files"].values()))["sha256"] = "0" * 64
            write(path, value)
        mutant("file_manifest_hash", file_manifest)
        original_bytes = file_a.read_bytes()
        try:
            file_a.write_bytes(original_bytes[:-1] + bytes([original_bytes[-1] ^ 1]))
            require(checked(genuine).returncode == 1, "changed input content was accepted")
            stats["mutants"] += 1
        finally:
            file_a.write_bytes(original_bytes)

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

        for suffix in ([], ["32", "rows", "0", "8", "3", "2", "4", "coarse", "1", "1"],
                       ["32", "rows", "1", "0", "3", "2", "4", "coarse", "1", "1"],
                       ["32", "rows", "1", "8", "3", "2", "0", "coarse", "1", "1"],
                       ["32", "rows", "1", "8", "3", "-1", "4", "coarse", "1", "1"],
                       ["33", "rows", "1", "8", "3", "2", "4", "coarse", "1", "1"],
                       ["32", "rows", "1", "8", "3", "2", "4", "bad", "1", "1"],
                       ["32", "rows", "1", "8", "3", "2", "4", "donate", "0", "1"],
                       ["32", "rows", "1", "8", "3", "2", "4", "donate", "1", "0"]):
            process = subprocess.run([str(binary), *suffix], capture_output=True, cwd=ROOT)
            require(process.returncode == 2 and not process.stdout and process.stderr, "invalid probe options not rejected")
            stats["invalid_options"] += 1
        for suffix in (["4", "file:relative.u16le", "2", "8", "0", "2", "1", "donate", "1", "1"],
                       ["4", "file:" + str(file_a), "2", "8", "1", "2", "1", "donate", "1", "1"],
                       ["5", "file:" + str(file_a), "2", "8", "0", "2", "1", "donate", "1", "1"],
                       ["4", "file:" + str(temporary / "missing.u16le"), "2", "8", "0", "2", "1", "donate", "1", "1"]):
            process = subprocess.run([str(binary), *suffix], capture_output=True, cwd=ROOT)
            require(process.returncode == 2 and not process.stdout and process.stderr, "invalid file options not rejected")
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
        missing_input = temporary / "missing_input"
        process = subprocess.run(command("run", "--probe", str(binary), "--output", str(missing_input),
            "--input-files", str(temporary / "absent.u16le")), capture_output=True, cwd=ROOT)
        require(process.returncode == 1 and read(missing_input / "COMPLETION.json")["attempts"] == 0 and
                checked(missing_input).returncode == 1, "input startup failure was hidden")
        stats["failed_captures"] += 1
        startup = temporary / "startup"
        process = capture(temporary / "absent", startup)
        require(process.returncode == 1 and read(startup / "COMPLETION.json")["attempts"] == 0 and
                checked(startup).returncode == 1, "startup failure was hidden")
        stats["failed_captures"] += 1
    require(stats["genuine_rows"] == 108 and stats["positive_reads"] == 1 and stats["mutants"] >= 59 and
            stats["failed_captures"] == 3 and stats["invalid_options"] == 13 and stats["domain_checks"] == 12,
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
