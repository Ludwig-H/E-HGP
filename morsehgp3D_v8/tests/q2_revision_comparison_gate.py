#!/usr/bin/env python3
"""Small real probe rows plus synthetic revision-receipt mutations; no timing claim."""

from __future__ import annotations

import argparse
import base64
from collections import Counter
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile
from typing import Any, Callable


ROOT = Path(__file__).resolve().parents[2]
BENCH = ROOT / "morsehgp3D_v8/bench"
sys.path.insert(0, str(BENCH))
import compare_q2_revisions as comparison  # noqa: E402
import run_q2_census_matrix as reader  # noqa: E402

COMPARATOR = BENCH / "compare_q2_revisions.py"
OLD_PROBE_PIN = "243387ac6df40e3f0f0c38a1d7d7a86735ed260ba11d904bf032bc4eb1c6e432"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def read(path: Path) -> dict[str, Any]:
    return reader.parse_result(path.read_bytes())


def write(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, allow_nan=False) + "\n")


def read_rows(directory: Path) -> list[dict[str, Any]]:
    return [reader.parse_result(line) for line in (directory / "MEASURES.jsonl").read_bytes().splitlines()]


def write_rows(directory: Path, rows: list[dict[str, Any]]) -> None:
    (directory / "MEASURES.jsonl").write_text("".join(json.dumps(row, allow_nan=False) + "\n" for row in rows))


def refresh(record: dict[str, Any]) -> None:
    raw = (json.dumps(record["result"], allow_nan=False) + "\n").encode()
    record["stdout"] = raw.decode()
    record["stdout_base64"] = base64.b64encode(raw).decode("ascii")


def invoke(root: Path, summary: bool = False) -> subprocess.CompletedProcess:
    command = [sys.executable, "-B", *(["-O"] if sys.flags.optimize else []), str(COMPARATOR),
               str(root / "baseline"), str(root / "candidate"), *(["--summary"] if summary else [])]
    return subprocess.run(command, cwd=ROOT, capture_output=True)


def fixture(directory: Path, rows: list[dict[str, Any]], pins: dict[str, str],
            binary: str, binary_pin: str, cache: str, compiler: str) -> None:
    """Synthetic manifest provenance, deliberately not a performance capture.

    Default baseline reuses real NEW probe results but declares the fixed old
    revision/binary as a reader fixture. No file leaves the temporary gate root.
    Actual cross-revision measurements are captured independently by the runner.
    """
    directory.mkdir()
    manifest = {"families": ["grid"], "sizes": [8, 16, 32], "kmax": [5], "separations": [8],
                "prefilters": ["intersection_pool"], "orders": list(reader.ORDERS), "repeats": 1,
                "schema": "mhgp8_q2_census_campaign_v1", "started_utc": reader.utc_stamp(),
                "scope": "single_rectangle_q2_materialized_census", "public_status": "not_claimed",
                "threads": 1, "gcp_used": False, "processes_sequential": True, "warmup_runs": 0,
                "probe": binary, "probe_sha256": binary_pin, "source_sha256": pins,
                "cmake_cache": cache, "compiler_version": compiler, "platform": platform.platform(),
                "logical_cpu_count": os.cpu_count(), "cpuinfo": "synthetic common machine for reader fixtures",
                "commit": comparison.BASELINE_REVISION, "worktree_status": "synthetic reader fixture",
                "runner_command": ["synthetic reader gate, not a measured capture"]}
    records = []
    for row in rows:
        command = reader.command_for(binary, tuple(row[key] for key in reader.KEYS))
        record = {"command": command, "repeat": 0, "status": "completed", "exit_code": 0,
                  "stderr": "", "stderr_base64": "", "probe_sha256_before": binary_pin,
                  "probe_sha256_after": binary_pin, "result": row}
        refresh(record)
        records.append(record)
    write(directory / "MANIFEST.json", manifest)
    write_rows(directory, records)
    write(directory / "COMPLETION.json", {"status": "completed", "error": "", "runs": len(rows),
        "attempts": len(rows), "finished_utc": reader.utc_stamp(), "source_hashes_unchanged": True,
        "probe_hash_unchanged": True, "source_sha256_closing": pins, "probe_sha256_closing": binary_pin})


def probe_rows(probe: Path, stats: Counter[str]) -> list[dict[str, Any]]:
    result = []
    for n in (8, 16, 32):
        for order in reader.ORDERS:
            command = [str(probe), str(n), "grid", "5", "8", "intersection_pool", order]
            process = subprocess.run(command, cwd=ROOT, capture_output=True)
            require(process.returncode == 0 and not process.stderr, f"small probe failed: {process.stderr!r}")
            row = reader.parse_result(process.stdout)
            reader.validate_result(row, command)
            result.append(row)
            stats["genuine_probe_rows"] += 1
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--baseline-probe", type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    probe = args.probe.resolve()
    protected = {path: reader.digest(path) for path in (probe, COMPARATOR, BENCH / "run_q2_census_matrix.py")}
    if args.baseline_probe:
        protected[args.baseline_probe.resolve()] = reader.digest(args.baseline_probe.resolve())
    expected_current = reader.sources()
    expected_old = comparison.baseline_sources()
    stats: Counter[str] = Counter()
    current_rows = probe_rows(probe, stats)
    old_rows = probe_rows(args.baseline_probe.resolve(), stats) if args.baseline_probe else current_rows
    cache = (probe.parent / "CMakeCache.txt").read_text()
    compilers = [line.split("=", 1)[1] for line in cache.splitlines()
                 if line.startswith("CMAKE_CXX_COMPILER:")]
    require(len(compilers) == 1, "missing compiler for synthetic fixture metadata")
    compiler = subprocess.check_output([compilers[0], "--version"], text=True)
    with tempfile.TemporaryDirectory(prefix="mhgp8_q2_revision_comparison_gate_") as name:
        temporary = Path(name)
        genuine = temporary / "positive"
        genuine.mkdir()
        old_binary = str(args.baseline_probe.resolve()) if args.baseline_probe else "/synthetic/f481/q2_probe"
        old_pin = reader.digest(args.baseline_probe.resolve()) if args.baseline_probe else OLD_PROBE_PIN
        fixture(genuine / "baseline", old_rows, expected_old, old_binary, old_pin, cache, compiler)
        fixture(genuine / "candidate", current_rows, expected_current, str(probe), protected[probe], cache, compiler)
        evidence = {path: reader.digest(path) for path in genuine.rglob("*") if path.is_file()}
        process = invoke(genuine, True)
        require(process.returncode == 0 and not process.stderr, f"revision positive rejected: {process.stderr!r}")
        result = reader.parse_result(process.stdout)
        require(result["matching_work_and_outputs"] is True and result["full_contract_qualified"] is False and
                result["measurements_per_revision"] == result["configurations_including_order"] == 6 and
                len(result["summary"]) == 6 and result["baseline_revision"] == comparison.BASELINE_REVISION and
                result["baseline"]["source_sha256"] == expected_old and
                result["candidate"]["source_sha256"] == expected_current and
                result["baseline"]["provenance"]["probe_sha256"] == old_pin and
                result["candidate"]["provenance"]["probe_sha256"] == protected[probe],
                "revision reader merged identities, sources, modes or scope")
        require(len(result["doublings"]["baseline"]) == len(result["doublings"]["candidate"]) == 4 and
                {item["order"] for item in result["summary"]} == set(reader.ORDERS),
                "doubling summary lost sizes or execution order")
        for item in result["summary"]:
            for arm in item["arms"]:
                measured = arm["times"]["total_ms"]
                require(measured["candidate_over_baseline"] ==
                        comparison.ratio(measured["candidate_ms"], measured["baseline_ms"]) and
                        all(value in (None, 1.0) for value in arm["work_candidate_over_baseline"].values()),
                        "revision ratio convention or work identity is wrong")
        require(all(reader.digest(path) == pin for path, pin in evidence.items()), "comparator rewrote evidence")
        stats["positive_comparisons"] += 1

        def repeat_with_times(directory: Path, multipliers: tuple[int, ...]) -> None:
            manifest = read(directory / "MANIFEST.json")
            manifest["repeats"] = len(multipliers)
            repeated = []
            for original in read_rows(directory):
                for repeat, multiplier in enumerate(multipliers):
                    record = json.loads(json.dumps(original))
                    record["repeat"] = repeat
                    row = record["result"]
                    for field in reader.COMMON_TIMES:
                        row[field] *= multiplier
                    for arm in row["arms"]:
                        for field in reader.ARM_TIMES:
                            arm[field] *= multiplier
                    refresh(record)
                    repeated.append(record)
            completion = read(directory / "COMPLETION.json")
            completion["runs"] = completion["attempts"] = len(repeated)
            write(directory / "MANIFEST.json", manifest)
            write(directory / "COMPLETION.json", completion)
            write_rows(directory, repeated)

        synthetic_times = temporary / "known_medians"
        shutil.copytree(genuine, synthetic_times)
        repeat_with_times(synthetic_times / "baseline", (1, 1, 1))
        repeat_with_times(synthetic_times / "candidate", (4, 2, 1))
        process = invoke(synthetic_times, True)
        require(process.returncode == 0 and not process.stderr, f"known medians rejected: {process.stderr!r}")
        medians = reader.parse_result(process.stdout)
        require(medians["measurements_per_revision"] == 18 and
                all(row["repeats_per_revision"] == 3 for row in medians["summary"]),
                "synthetic repetitions were not retained")
        for before, after in zip(result["summary"], medians["summary"]):
            for first, second in zip(before["arms"], after["arms"]):
                expected = first["times"]["total_ms"]
                measured = second["times"]["total_ms"]
                require(measured["baseline_ms"] == expected["baseline_ms"] and
                        measured["candidate_ms"] == 2 * expected["candidate_ms"] and
                        measured["candidate_over_baseline"] == 2 * expected["candidate_over_baseline"],
                        "known median or revision ratio was computed incorrectly")
        stats["positive_comparisons"] += 1

        def reject(label: str, mutate: Callable[[Path], None], fragment: str) -> None:
            target = temporary / label
            shutil.copytree(genuine, target)
            mutate(target)
            process = invoke(target)
            require(process.returncode == 1 and not process.stdout and fragment in process.stderr.decode(),
                    f"revision mutant survived/wrong rejection: {label}: {process.stderr!r}")
            stats["mutants"] += 1

        def source_mutation(root: Path, side: str, field: str, value: str) -> None:
            directory = root / side
            manifest = read(directory / "MANIFEST.json")
            completion = read(directory / "COMPLETION.json")
            manifest["source_sha256"][field] = value
            completion["source_sha256_closing"] = manifest["source_sha256"]
            write(directory / "MANIFEST.json", manifest)
            write(directory / "COMPLETION.json", completion)
        for side in ("baseline", "candidate"):
            reject(f"{side}_unknown_source", lambda root, side=side: source_mutation(
                root, side, "morsehgp3D_v8/src/pipeline/q2_census.cpp", "0" * 64), "explicitly pinned version")
        reject("old_runner_exception_not_inherited", lambda root: source_mutation(
            root, "baseline", reader.RUNNER_SOURCE, reader.LEGACY_RUNNER_SHA256), "explicitly pinned version")

        def mutate_manifest(root: Path, field: str, value: Any) -> None:
            path = root / "candidate/MANIFEST.json"
            manifest = read(path)
            manifest[field] = value
            write(path, manifest)
        reject("machine", lambda root: mutate_manifest(root, "cpuinfo", "different synthetic machine"),
               "machine metadata")
        reject("compiler", lambda root: mutate_manifest(root, "compiler_version", "different compiler"),
               "compiler/configuration")
        reject("compiler_flags", lambda root: mutate_manifest(root, "cmake_cache", cache.replace(
            "CMAKE_CXX_FLAGS:STRING=", "CMAKE_CXX_FLAGS:STRING=-DSYNTHETIC_FLAG ")), "compiler/configuration")

        def cache_option(root: Path, name: str, value: str) -> None:
            path = root / "candidate/MANIFEST.json"
            manifest = read(path)
            lines = manifest["cmake_cache"].splitlines()
            matching = [i for i, line in enumerate(lines) if line.startswith(name + ":")]
            require(len(matching) <= 1, "fixture cache contains duplicate configuration keys")
            replacement = name + ":STRING=" + value
            if matching:
                lines[matching[0]] = replacement
            else:
                lines.append(replacement)
            manifest["cmake_cache"] = "\n".join(lines) + "\n"
            write(path, manifest)

        for name, value in (("CMAKE_EXE_LINKER_FLAGS", "-flto"),
                            ("CMAKE_EXE_LINKER_FLAGS_RELEASE", "-flto"),
                            ("CMAKE_INTERPROCEDURAL_OPTIMIZATION", "ON"),
                            ("CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE", "ON")):
            reject(name.lower(), lambda root, name=name, value=value: cache_option(root, name, value),
                   "compiler/configuration")

        def change_rows(root: Path, kind: str) -> None:
            directory = root / "candidate"
            records = read_rows(directory)
            for record in records:
                row = record["result"]
                if kind == "input":
                    row["input_fnv1a64_le_u16_xyz"] = "0" * 16
                elif kind == "work":
                    row["arms"][1]["work"]["consumed_witness_sites"] += 1
                elif kind == "output":
                    for arm in row["arms"]:
                        arm["digest"]["sum"] = "0" * 16
                elif kind == "exit_bool":
                    record["exit_code"] = False
                elif kind == "raw":
                    record["stdout_base64"] = base64.b64encode(b'{"bad": 1}\n').decode()
                    continue
                elif kind == "binary":
                    record["probe_sha256_after"] = "0" * 64
                elif kind == "tuple":
                    record["command"][1] = "64"
                refresh(record)
            write_rows(directory, records)
        for label, fragment in (("input", "input identity"), ("work", "work/output digest"),
                                ("output", "work/output digest"), ("exit_bool", "exit_code"),
                                ("raw", "family"), ("binary", "binary pin"), ("tuple", "command/raw/result")):
            reject(label, lambda root, label=label: change_rows(root, label), fragment)

        def missing_order(root: Path) -> None:
            directory = root / "candidate"
            manifest = read(directory / "MANIFEST.json")
            manifest["orders"] = [reader.ORDERS[0]]
            records = [record for record in read_rows(directory) if record["result"]["order"] == reader.ORDERS[0]]
            completion = read(directory / "COMPLETION.json")
            completion["runs"] = completion["attempts"] = len(records)
            write(directory / "MANIFEST.json", manifest)
            write(directory / "COMPLETION.json", completion)
            write_rows(directory, records)
        reject("closed_but_unpaired_matrix", missing_order, "matrices/repetition multiplicities")
        reject("closed_but_unpaired_repetitions", lambda root: repeat_with_times(root / "candidate", (1, 1)),
               "matrices/repetition multiplicities")

        def failed_sibling(root: Path) -> None:
            directory = root / "candidate/failed"
            directory.mkdir()
            write(directory / "COMPLETION.json", {"status": "failed", "runs": 0, "attempts": 0})
        reject("failed_nested_campaign", failed_sibling, "mixed/nested")

    require(stats["positive_comparisons"] == 2 and stats["mutants"] == 20 and
            stats["genuine_probe_rows"] == (12 if args.baseline_probe else 6), "revision gate was vacuous")
    require(expected_current == reader.sources() and all(reader.digest(path) == pin for path, pin in protected.items()),
            "revision gate changed product sources or an input binary")
    print(json.dumps({"status": "passed", **stats, "python_optimized": bool(sys.flags.optimize),
                      "scope": "synthetic_revision_reader_fixtures_not_performance_qualification"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
