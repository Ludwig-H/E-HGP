#!/usr/bin/env python3
"""Small genuine cloud-reuse captures, failed attempts and receipt mutants."""

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
RUNNER = BENCH / "run_cloud_reuse_matrix.py"
sys.path.insert(0, str(BENCH))
from run_cloud_reuse_matrix import RUNNER_SOURCE, digest, parse_result, sources  # noqa: E402


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


def capture(probe: Path, output: Path) -> subprocess.CompletedProcess:
    return subprocess.run(command("run", "--probe", str(probe), "--output", str(output),
        "--sizes", "16", "32", "64", "--families", "grid", "--kmax", "5", "--s", "8",
        "--rectangles", "2", "--repeats", "1"), capture_output=True, cwd=ROOT)


def checked(root: Path) -> subprocess.CompletedProcess:
    return subprocess.run(command("check", str(root), "--summary"), capture_output=True, cwd=ROOT)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    original = args.probe.resolve()
    pins = sources()
    binary_pin = digest(original)
    stats: Counter[str] = Counter()
    with tempfile.TemporaryDirectory(prefix="mhgp8_cloud_reuse_receipts_") as name:
        temporary = Path(name)
        binary = temporary / "probe"
        shutil.copy2(original, binary)
        shutil.copy2(original.parent / "CMakeCache.txt", temporary / "CMakeCache.txt")
        genuine = temporary / "genuine"
        genuine.mkdir()
        for repeat in ("first", "second"):
            directory = genuine / repeat
            process = capture(binary, directory)
            require(process.returncode == 0 and not process.stderr, f"genuine capture failed: {process.stderr!r} {process.stdout!r}")
            completion = read(directory / "COMPLETION.json")
            rows = records(directory)
            require(completion["status"] == "completed" and completion["runs"] == completion["attempts"] == len(rows) == 6,
                    "genuine capture lost a tuple")
            require(read(directory / "MANIFEST.json")["source_sha256"] == pins, "capture lost source perimeter")
            for row in rows:
                for field in ("stdout", "stderr"):
                    require(base64.b64decode(row[f"{field}_base64"], validate=True).decode("utf-8", errors="replace") == row[field],
                            "capture lost raw process output")
                require(row["result"]["arms"][0]["digest"]["supports"] > 0, "genuine payload fixture vacuous")
            stats["genuine_rows"] += len(rows)
        evidence = {path: digest(path) for path in genuine.rglob("*") if path.is_file()}
        process = checked(genuine)
        require(process.returncode == 0 and not process.stderr, f"genuine reader failed: {process.stderr!r}")
        summary = parse_result(process.stdout)
        require(summary["measurements"] == 12 and summary["configurations_including_order"] == 6 and
                len(summary["summary"]) == 6 and all(row["repeats"] == 2 for row in summary["summary"]) and
                summary["full_contract_qualified"] is False and summary["gcp_used"] is False and
                summary["current_reader_sha256"] == pins[RUNNER_SOURCE], "reader lost repetitions, scope or pins")
        require(all(digest(path) == pin for path, pin in evidence.items()), "reader rewrote evidence")
        stats["positive_reads"] += 1

        # All mutants are temporary receipt models, not altered producer code.
        def mutant(label: str, change: Callable[[Path], None]) -> None:
            destination = temporary / label
            shutil.copytree(genuine, destination)
            change(destination)
            result = checked(destination)
            require(result.returncode == 1, f"mutant survived or returned wrong code: {label} {result.stdout!r} {result.stderr!r}")
            stats["mutants"] += 1

        def manifest_change(root: Path, update: Callable[[dict], None]) -> None:
            path = root / "first/MANIFEST.json"
            value = read(path)
            update(value)
            write(path, value)

        def completion_change(root: Path, update: Callable[[dict], None]) -> None:
            path = root / "first/COMPLETION.json"
            value = read(path)
            update(value)
            write(path, value)

        def record_change(root: Path, update: Callable[[dict], None], raw_refresh: bool = False) -> None:
            directory = root / "first"
            rows = records(directory)
            update(rows[0])
            if raw_refresh:
                refresh(rows[0])
            write_records(directory, rows)

        def arm_change(root: Path, field: str, value: Any, both: bool = False) -> None:
            def change(record: dict) -> None:
                for arm in record["result"]["arms"][:2 if both else 1]:
                    arm[field] = value
            record_change(root, change, True)

        mutant("pin_omission", lambda root: manifest_change(root, lambda value: value["source_sha256"].pop(
            "morsehgp3D_v8/src/pipeline/prepared_cloud.cpp")))
        mutant("pin_wrong", lambda root: manifest_change(root, lambda value: value["source_sha256"].update({RUNNER_SOURCE: "0" * 64})))
        mutant("compiler", lambda root: manifest_change(root, lambda value: value.update(compiler_version="different compiler")))
        mutant("flags", lambda root: manifest_change(root, lambda value: value.update(cmake_cache=value["cmake_cache"] + "\nCMAKE_CXX_FLAGS:STRING=-ffast-math\n")))
        mutant("machine", lambda root: manifest_change(root, lambda value: value.update(logical_cpu_count=value["logical_cpu_count"] + 1)))
        mutant("gpu_claim", lambda root: manifest_change(root, lambda value: value.update(gcp_used=True)))
        mutant("duplicate_size", lambda root: manifest_change(root, lambda value: value["sizes"].append(value["sizes"][0])))
        mutant("bool_rectangles", lambda root: manifest_change(root, lambda value: value.update(rectangles=[True])))
        mutant("closing_source", lambda root: completion_change(root, lambda value: value["source_sha256_closing"].update({RUNNER_SOURCE: "0" * 64})))
        mutant("closing_binary", lambda root: completion_change(root, lambda value: value.update(probe_sha256_closing="0" * 64)))
        mutant("attempts", lambda root: completion_change(root, lambda value: value.update(attempts=value["attempts"] + 1)))
        mutant("unfinished", lambda root: completion_change(root, lambda value: value.update(status="failed")))
        mutant("wrong_command", lambda root: record_change(root, lambda value: value["command"].__setitem__(5, "3")))
        mutant("raw_divergence", lambda root: record_change(root, lambda value: value.update(stdout="{}\n")))
        mutant("binary_after", lambda root: record_change(root, lambda value: value.update(probe_sha256_after="0" * 64)))
        mutant("boolean_exit", lambda root: record_change(root, lambda value: value.update(exit_code=False)))
        mutant("nonserial_time", lambda root: record_change(root, lambda value: value.update(finished_utc="2000-01-01T00:00:00+00:00")))
        mutant("wrong_tuple", lambda root: record_change(root, lambda value: value["result"].update(n=32), True))
        mutant("input_hash", lambda root: record_change(root, lambda value: value["result"].update(input_fnv1a64_le_u16_xyz="0"), True))
        mutant("bool_count", lambda root: arm_change(root, "candidate_pairs", True))
        mutant("preparation_count", lambda root: arm_change(root, "cloud_preparations", 1))
        mutant("invisible_box_steps", lambda root: arm_change(root, "factor_box_steps", 0, True))
        mutant("unbounded_memory", lambda root: arm_change(root, "max_cloud_retained_bytes", 1 << 60, True))
        mutant("total_omits_work", lambda root: arm_change(root, "total_ms", 0, True))
        mutant("extra_global_copy", lambda root: record_change(root, lambda value:
            value["result"]["arms"][1]["cloud_work"].update(coordinate_copies=32), True))
        mutant("lost_shell", lambda root: record_change(root, lambda value:
            value["result"]["arms"][0]["digest"].update(shell_ids=0), True))
        mutant("different_local_work", lambda root: record_change(root, lambda value:
            value["result"]["arms"][0]["local_work"]["counters"].update(pool_selected=0), True))

        def duplicate_row(root: Path) -> None:
            rows = records(root / "first")
            rows[1] = copy.deepcopy(rows[0])
            write_records(root / "first", rows)
        mutant("duplicate_row", duplicate_row)
        mutant("missing_triplet", lambda root: (root / "first/MANIFEST.json").unlink())

        def orphan(root: Path) -> None:
            (root / "orphan").mkdir()
            write(root / "orphan/COMPLETION.json", {"status": "failed"})
        mutant("orphan_sibling", orphan)

        def nested(root: Path) -> None:
            shutil.copytree(root / "second", root / "first/nested")
        mutant("nested_campaign", nested)

        # A real nonzero process must leave the failed attempt visible.
        failed_probe = temporary / "failing_probe"
        shutil.copy2("/bin/false", failed_probe)
        failed = temporary / "failed_capture"
        result = capture(failed_probe, failed)
        require(result.returncode == 1 and (failed / "COMPLETION.json").is_file(), "failed process did not close")
        failed_rows = records(failed)
        completion = read(failed / "COMPLETION.json")
        require(completion["status"] == "failed" and completion["runs"] == 0 and
                completion["attempts"] == len(failed_rows) == 1 and failed_rows[0]["exit_code"] == 1 and
                failed_rows[0]["status"] == "failed", "failed invocation was lost or promoted")
        require(checked(failed).returncode == 1, "failed campaign qualified")
        stats["failed_captures"] += 1
        initial_failure = temporary / "startup_failure"
        result = capture(temporary / "absent_probe", initial_failure)
        require(result.returncode == 1 and read(initial_failure / "COMPLETION.json")["attempts"] == 0 and
                checked(initial_failure).returncode == 1, "startup failure was hidden")
        stats["failed_captures"] += 1
    require(stats["genuine_rows"] == 12 and stats["mutants"] >= 30 and stats["failed_captures"] == 2 and
            stats["positive_reads"] == 1, "receipt gate lost its non-vacuity floor")
    require(sources() == pins and digest(original) == binary_pin, "gate modified sources or actual probe")
    print(json.dumps(dict(status="passed", **stats), sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, RuntimeError, OSError, KeyError, TypeError) as error:
        print(f"cloud reuse receipt gate failed: {error}", file=sys.stderr)
        raise SystemExit(1) from error
