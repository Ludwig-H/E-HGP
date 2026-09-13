#!/usr/bin/env python3
"""Exercise specialized q2 campaign closure, materialization and provenance."""

from __future__ import annotations

import argparse
import base64
from collections import Counter
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from typing import Any, Callable

ROOT = Path(__file__).resolve().parents[2]
BENCH = ROOT / "morsehgp3D_v8/bench"
RUNNER = BENCH / "run_q2_census_matrix.py"
sys.path.insert(0, str(BENCH))
from run_q2_census_matrix import (CAPTURE_RUNNER_ARCHIVE, LEGACY_RUNNER_SHA256, RUNNER_SOURCE,
                                 InvalidReceipt, digest, parse_result, validate_legacy_archive,
                                 validate_result)  # noqa: E402


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


def capture(probe: Path, output: Path) -> tuple[Any, dict[str, Any], list[dict[str, Any]]]:
    process = subprocess.run(command("run", "--probe", str(probe), "--output", str(output),
        "--sizes", "32", "--families", "sheet_full", "--kmax", "5", "--s", "8",
        "--prefilters", "independent", "additive", "intersection_pool", "--repeats", "1"),
        capture_output=True, cwd=ROOT)
    require((output / "COMPLETION.json").is_file(), "q2 campaign did not close")
    completion = read(output / "COMPLETION.json")
    rows = records(output) if (output / "MEASURES.jsonl").exists() else []
    require(completion["attempts"] == len(rows) and
            completion["runs"] == sum(row["status"] == "completed" for row in rows),
            "q2 capture lost or miscounted an attempted row")
    for row in rows:
        for name in ("stdout", "stderr"):
            require(base64.b64decode(row[f"{name}_base64"], validate=True).decode(
                "utf-8", errors="replace") == row[name], "q2 raw output was not preserved")
    return process, completion, rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    source = args.probe.resolve()
    pins = {path: digest(path) for path in (source, RUNNER)}
    stats: Counter[str] = Counter()
    with tempfile.TemporaryDirectory(prefix="mhgp8_q2_census_campaign_gate_") as name:
        temporary = Path(name)
        actual = temporary / "actual_probe"
        shutil.copy2(source, actual)
        shutil.copy2(source.parent / "CMakeCache.txt", temporary / "CMakeCache.txt")
        genuine = temporary / "genuine"
        genuine.mkdir()
        for name in ("first", "repeat"):
            process, completion, rows = capture(actual, genuine / name)
            require(process.returncode == 0 and not process.stderr and completion["status"] == "completed" and
                    completion["runs"] == 6 and completion["source_hashes_unchanged"] is True and
                    completion["probe_hash_unchanged"] is True, f"genuine capture failed: {process.stderr!r}")
            stats["genuine_rows"] += len(rows)
        before_read = {path: digest(path) for path in genuine.rglob("*") if path.is_file()}
        process = subprocess.run(command("check", str(genuine), "--summary"), capture_output=True, cwd=ROOT)
        require(process.returncode == 0 and not process.stderr, f"genuine q2 reader failed: {process.stderr!r}")
        summary = parse_result(process.stdout)
        require(summary["status"] == "passed" and summary["measurements"] == 12 and
                summary["configurations_including_order"] == len(summary["summary"]) == 6 and
                summary["full_contract_qualified"] is False and all(item["repeats"] == 2 for item in summary["summary"]),
                "q2 reader lost repetitions/prefilters/order or changed scope")
        require(summary["current_reader_sha256"] == pins[RUNNER] and
                summary["capture_runner_sha256"] == [pins[RUNNER]] and
                all(read(genuine / name / "MANIFEST.json")["source_sha256"][RUNNER_SOURCE] == pins[RUNNER]
                    for name in ("first", "repeat")), "new capture did not pin the current runner")
        require(all(digest(path) == value for path, value in before_read.items()), "q2 check rewrote evidence")
        stats["positive_reads"] += 1

        original = records(genuine / "first")[0]
        upper = json.loads(json.dumps(original["result"]))
        # Synthetic consistency bounds, not purported measured geometry:
        # an internal depth scans coordinates twice (bbox + partition).
        upper["index_work"]["point_visits"] = 97 * upper["n"]
        validate_result(upper, original["command"])
        upper["index_work"]["point_visits"] += 1
        try:
            validate_result(upper, original["command"])
        except InvalidReceipt:
            stats["index_scan_boundary_controls"] += 1
        else:
            raise RuntimeError("index work beyond 97n was accepted")

        prelude = (f"#!{sys.executable}\nimport json, os, signal, subprocess\nfrom pathlib import Path\n"
                   "import sys\n" + f"real = {str(actual)!r}\n")
        passthrough = ("captured = subprocess.run([real, *sys.argv[1:]], capture_output=True, check=True)\n"
                       "row = json.loads(captured.stdout)\n")
        mutations = {
            "wrong_prefilter": "row['prefilter'] = 'unknown'\n",
            "count_only": "row['payload_materialized'] = False\n",
            "wrong_ball_claim": "row['canonical_balls_deduplicated'] = True\n",
            "omit_payload_time": "row['arms'][0]['consumption_ms'] -= row['arms'][0]['payload_ms']\n",
            "omit_destruction": "row['arms'][0]['total_ms'] -= row['destruction_ms']\n",
            "wrong_cursor_reuse": "row['arms'][1]['work']['cursor_reuses'] += 1\n",
            "wrong_escape_links": "row['index_work']['escape_links'] -= 1\n",
            "omit_query_nodes": "row['arms'][1]['work']['query_build_nodes'] = 0\n",
            "omit_query_points": "row['arms'][1]['work']['query_build_point_visits'] = 0\n",
            "omit_query_index": ("row['arms'][1]['work']['query_build_nodes'] = 0\n"
                                 "row['arms'][1]['work']['query_build_point_visits'] = 0\n"),
            "vacuous_shared_count": (
                "work = row['arms'][1]['work']\n"
                "for key in work:\n"
                "  if not key.startswith(('query_build_', 'payload_')) and key != 'input_descriptors':\n"
                "    work[key] = 0\n"),
            "cross_prefilter_outputs": (
                "if row['prefilter'] == 'additive':\n"
                "  for arm in row['arms']: arm['digest']['sum'] = '1' if arm['digest']['sum'] != '1' else '2'\n"),
        }
        for label, body in mutations.items():
            fake = temporary / label
            fake.write_text(prelude + passthrough + body + "print(json.dumps(row))\n")
            fake.chmod(0o755)
            process, completion, rows = capture(fake, temporary / f"failed_{label}")
            require(process.returncode == 1 and completion["status"] == "invalid" and
                    rows[-1]["status"] == "invalid" and bool(rows[-1]["stdout_base64"]),
                    f"q2 counterfeit survived or evidence lost: {label}: {completion}")
            stats["runner_mutants"] += 1
        for label, body, status, code in (
                ("overflow_json", "print('{\"value\":1e999}')\n", "invalid", 1),
                ("binary_changes", passthrough + "print(json.dumps(row))\n"
                 "with Path(__file__).open('a') as stream: stream.write('# changed\\n')\n", "invalid", 1),
                ("interrupted", "print('partial stdout', flush=True)\n"
                 "os.kill(os.getppid(), signal.SIGINT)\n", "interrupted", 130)):
            fake = temporary / label
            fake.write_text(prelude + body)
            fake.chmod(0o755)
            process, completion, rows = capture(fake, temporary / f"failed_{label}")
            require(process.returncode == code and completion["status"] == status and
                    completion["runs"] == 0 and len(rows) == 1 and bool(rows[0]["stdout_base64"]),
                    f"q2 failure closure lost evidence: {label}: {completion}")
            if label == "binary_changes":
                require(completion["probe_hash_unchanged"] is False, "changed binary pin was accepted")
            stats["runner_mutants"] += 1

        initial_failure = temporary / "initial_failure"
        initial_failure.mkdir()
        shutil.copytree(genuine / "first", initial_failure / "successful")
        failed = initial_failure / "missing_binary"
        process, completion, rows = capture(temporary / "binary_does_not_exist", failed)
        require(process.returncode == 1 and completion["status"] == "failed" and
                completion["runs"] == completion["attempts"] == 0 and not rows and
                not (failed / "MANIFEST.json").exists(), "missing binary did not preserve its initial failure")
        process = subprocess.run(command("check", str(initial_failure)), capture_output=True, cwd=ROOT)
        require(process.returncode == 1 and not process.stdout and b"incomplete q2 campaign" in process.stderr,
                "parent reader silently ignored a genuine failed sibling without MANIFEST")
        stats["initial_failure_group_controls"] += 1

        def reject(label: str, mutate: Callable[[Path], None], fragment: str) -> None:
            target = temporary / f"reader_{label}"
            shutil.copytree(genuine, target)
            mutate(target)
            process = subprocess.run(command("check", str(target)), capture_output=True, cwd=ROOT)
            require(process.returncode == 1 and not process.stdout and
                    fragment in process.stderr.decode("utf-8", errors="replace"),
                    f"q2 reader mutant survived or wrong cause: {label}: {process.stderr!r}")
            stats["reader_mutants"] += 1

        def row_mutation(root: Path, mode: str) -> None:
            for directory in (root / "first", root / "repeat"):
                rows = records(directory)
                for record in rows:
                    row = record["result"]
                    if row["prefilter"] != "additive":
                        continue
                    if mode == "cross_outputs":
                        for arm in row["arms"]:
                            arm["digest"]["sum"] = "1" if arm["digest"]["sum"] != "1" else "2"
                        validate_result(row, record["command"])
                    elif mode == "exit_bool":
                        record["exit_code"] = False
                    elif mode == "payload_count":
                        row["arms"][0]["work"]["payload_shell_sites"] -= 1
                    elif mode == "cursor_bool":
                        row["arms"][1]["work"]["cursor_advances"] = True
                    elif mode == "omit_payload":
                        row["arms"][0]["census_total_ms"] -= row["arms"][0]["payload_ms"]
                    elif mode == "wrong_tuple":
                        record["command"][5] = "independent"
                    elif mode in ("omit_query_nodes", "omit_query_points", "omit_query_index"):
                        if mode != "omit_query_points":
                            row["arms"][1]["work"]["query_build_nodes"] = 0
                        if mode != "omit_query_nodes":
                            row["arms"][1]["work"]["query_build_point_visits"] = 0
                    elif mode == "vacuous_shared_count":
                        work = row["arms"][1]["work"]
                        for key in work:
                            if not key.startswith(("query_build_", "payload_")) and key != "input_descriptors":
                                work[key] = 0
                    refresh(record)
                write_records(directory, rows)
        for mode, fragment in (("cross_outputs", "changed across prefilters"), ("exit_bool", "exit_code"),
                               ("payload_count", "materialized"), ("cursor_bool", "cursor_advances"),
                               ("omit_payload", "count/payload partition"), ("wrong_tuple", "command/raw/result"),
                               ("omit_query_nodes", "shared query index"),
                               ("omit_query_points", "shared query index"),
                               ("omit_query_index", "shared query index"),
                               ("vacuous_shared_count", "nonempty census omitted counting work")):
            reject(mode, lambda path, mode=mode: row_mutation(path, mode), fragment)

        for field in ("compiler_version", "cpuinfo"):
            def change_provenance(root: Path, field: str = field) -> None:
                path = root / "repeat/MANIFEST.json"
                manifest = read(path)
                manifest[field] += "\nsynthetic alternate metadata\n"
                write(path, manifest)
            reject(field, change_provenance, "heterogeneous provenance")

        def omit_source(root: Path) -> None:
            path = root / "first"
            manifest = read(path / "MANIFEST.json")
            manifest["source_sha256"].pop("morsehgp3D_v8/bench/sheet_full_fixture.hpp")
            completion = read(path / "COMPLETION.json")
            completion["source_sha256_closing"] = manifest["source_sha256"]
            write(path / "MANIFEST.json", manifest)
            write(path / "COMPLETION.json", completion)
        reject("source_coverage", omit_source, "source coverage/hash")

        def change_pins(directory: Path, values: dict[str, str]) -> None:
            manifest = read(directory / "MANIFEST.json")
            completion = read(directory / "COMPLETION.json")
            manifest["source_sha256"].update(values)
            completion["source_sha256_closing"] = manifest["source_sha256"]
            write(directory / "MANIFEST.json", manifest)
            write(directory / "COMPLETION.json", completion)

        # Synthetic metadata fixtures exercise the one permitted reader lineage;
        # they are not new performance receipts from the archived runner.
        legacy = temporary / "legacy_lineage"
        shutil.copytree(genuine, legacy)
        change_pins(legacy / "first", {RUNNER_SOURCE: LEGACY_RUNNER_SHA256})
        legacy_pins = {path: digest(path) for path in legacy.rglob("*") if path.is_file()}
        process = subprocess.run(command("check", str(legacy)), capture_output=True, cwd=ROOT)
        require(process.returncode == 0 and not process.stderr, f"explicit legacy lineage rejected: {process.stderr!r}")
        lineage = parse_result(process.stdout)
        require(lineage["current_reader_sha256"] == pins[RUNNER] and
                lineage["capture_runner_sha256"] == sorted((pins[RUNNER], LEGACY_RUNNER_SHA256)) and
                all(digest(path) == value for path, value in legacy_pins.items()),
                "reader hid the capture lineage or rewrote its evidence")
        stats["positive_reads"] += 1

        reject("unknown_runner", lambda root: change_pins(root / "first", {RUNNER_SOURCE: "0" * 64}),
               "unsupported capture runner hash")
        reject("legacy_nonrunner_changed", lambda root: change_pins(root / "first", {
            RUNNER_SOURCE: LEGACY_RUNNER_SHA256, "morsehgp3D_v8/bench/q2_census_probe.cpp": "0" * 64}),
            "non-runner source coverage/hash")

        archive = read(CAPTURE_RUNNER_ARCHIVE)
        validate_legacy_archive(archive)
        for field, value in (("path", "morsehgp3D_v8/bench/q2_census_probe.cpp"),
                             ("sha256", "0" * 64), ("source_base64", base64.b64encode(b"changed").decode()),
                             ("source_base64", "invalid base64!")):
            mutant = dict(archive, **{field: value})
            try:
                validate_legacy_archive(mutant)
            except (ValueError, TypeError):
                stats["archive_mutants"] += 1
            else:
                raise RuntimeError(f"capture archive mutant accepted: {field}")

        reject("nested_campaign", lambda root: shutil.copytree(root / "repeat", root / "first/nested"),
               "mixed/nested q2 campaign roots")

        def duplicate_mode(root: Path) -> None:
            path = root / "first/MANIFEST.json"
            manifest = read(path)
            manifest["prefilters"].append("additive")
            write(path, manifest)
        reject("duplicate_prefilter", duplicate_mode, "duplicate values")

    require(stats["genuine_rows"] == 12 and stats["positive_reads"] == 2 and
            stats["runner_mutants"] == 15 and stats["reader_mutants"] == 17 and
            stats["index_scan_boundary_controls"] == stats["initial_failure_group_controls"] == 1 and
            stats["archive_mutants"] == 4, "q2 campaign gate was vacuous")
    require(all(digest(path) == value for path, value in pins.items()), "q2 gate changed its input sources/binary")
    print(json.dumps(dict(status="passed", **stats, python_optimized=bool(sys.flags.optimize),
                         scope="q2_materialized_receipts_not_geometry_or_full")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
