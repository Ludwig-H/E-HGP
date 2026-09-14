#!/usr/bin/env python3
"""Small real WSPD/q2 captures and receipt mutants, including failure paths."""

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
RUNNER = BENCH / "run_wspd_q2_matrix.py"
sys.path.insert(0, str(BENCH))
from run_wspd_q2_matrix import RUNNER_SOURCE, digest, matrix, parse_result, sources  # noqa: E402


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
        "--sizes", "16", "32", "--families", "uniform", "rows", "--kmax", "1", "5",
        "--s", "8", "12", "--seeds", "3", "--modes", "pure", "samples",
        "--census-modes", "pairwise", "shared", "--repeats", "1"),
        capture_output=True, cwd=ROOT)


def capture_sibling(probe: Path, output: Path) -> subprocess.CompletedProcess:
    return subprocess.run(command("run", "--probe", str(probe), "--output", str(output),
        "--sizes", "16", "32", "--families", "uniform", "rows", "--kmax", "1", "5",
        "--s", "8", "--seeds", "3", "--modes", "pure", "samples",
        "--census-modes", "shared", "--sibling-modes", "none", "sibling", "--repeats", "1"),
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
    # Check the exact physical domain without constructing a large cloud.
    domain = dict(families=["rows"], sizes=[131072], kmax=[10], separations=[8],
                  seeds=[3], modes=["samples"], census_modes=["shared"], repeats=1)
    require(matrix(domain) == [("rows", 131072, 10, 8, 3, "samples", "shared", 0)],
            "reader rejected the last physically representable row fixture")
    stats["domain_checks"] += 1
    try:
        matrix({**domain, "sizes": [131074]})
    except ValueError:
        stats["domain_checks"] += 1
    else:
        raise RuntimeError("reader accepted rows beyond their coordinate domain")
    with tempfile.TemporaryDirectory(prefix="mhgp8_wspd_q2_receipts_") as name:
        temporary = Path(name)
        binary = temporary / "probe"
        shutil.copy2(original, binary)
        shutil.copy2(original.parent / "CMakeCache.txt", temporary / "CMakeCache.txt")
        genuine = temporary / "genuine"
        genuine.mkdir()
        for repeat in ("first", "second"):
            directory = genuine / repeat
            process = capture(binary, directory)
            require(process.returncode == 0 and not process.stderr,
                    f"genuine capture failed: {process.stderr!r} {process.stdout!r}")
            completion, rows = read(directory / "COMPLETION.json"), records(directory)
            require(completion["status"] == "completed" and completion["runs"] == completion["attempts"] == len(rows) == 64,
                    "genuine capture lost a tuple")
            require(read(directory / "MANIFEST.json")["source_sha256"] == pins, "capture lost source perimeter")
            for row in rows:
                for field in ("stdout", "stderr"):
                    require(base64.b64decode(row[f"{field}_base64"], validate=True).decode("utf-8", errors="replace") == row[field],
                            "capture lost raw process output")
                require(row["result"]["digest"]["supports"] > 0, "genuine materialized support fixture vacuous")
            require(any(r["result"]["front_work"]["fully_rejected_products"] > 0 for r in rows),
                    "genuine front rejection fixture vacuous")
            require(any(r["result"]["rejected_pairs"] > 0 for r in rows),
                    "genuine census rejection fixture vacuous")
            require(any(r["result"]["digest"]["shell_ids"] > 2 * r["result"]["accepted_pairs"] for r in rows),
                    "genuine shell fixture has only support endpoints")
            require(any(r["result"]["census_work"]["query_splits"] > 0 for r in rows),
                    "shared query subdivision fixture vacuous")
            stats["genuine_rows"] += len(rows)
        evidence = {path: digest(path) for path in genuine.rglob("*") if path.is_file()}
        process = checked(genuine)
        require(process.returncode == 0 and not process.stderr, f"genuine reader failed: {process.stderr!r}")
        summary = parse_result(process.stdout)
        require(summary["measurements"] == 128 and summary["configurations_including_front_census_seed"] == 64 and
                len(summary["summary"]) == 64 and all(row["repeats"] == 2 for row in summary["summary"]) and
                summary["full_contract_qualified"] is False and summary["gcp_used"] is False and
                summary["current_reader_sha256"] == pins[RUNNER_SOURCE] and
                summary["mode_comparison"] == "same_canonical_q2_support_digest_not_equal_front_or_census_work",
                "reader lost repetitions, scope, comparison or pins")
        require(all(digest(path) == pin for path, pin in evidence.items()), "reader rewrote evidence")
        stats["positive_reads"] += 1

        # Mutations below affect disposable evidence, never the producer.
        def mutant(label: str, change: Callable[[Path], None], fixture: Path = genuine) -> None:
            destination = temporary / label
            shutil.copytree(fixture, destination)
            change(destination)
            result = checked(destination)
            require(result.returncode == 1,
                    f"mutant survived or wrong exit code: {label} {result.stdout!r} {result.stderr!r}")
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

        def record_change(root: Path, update: Callable[[dict], None], raw_refresh: bool = False, sample: bool = False) -> None:
            directory = root / "first"
            rows = records(directory)
            index = 2 if sample else 0
            update(rows[index])
            if raw_refresh:
                refresh(rows[index])
            write_records(directory, rows)

        def work_change(root: Path, field: str, value: Any, sample: bool = False) -> None:
            record_change(root, lambda row: row["result"]["front_work"].update({field: value}), True, sample)

        mutant("pin_omission", lambda root: manifest_change(root, lambda value: value["source_sha256"].pop(
            "morsehgp3D_v8/src/wspd/front.cpp")))
        mutant("pin_wrong", lambda root: manifest_change(root, lambda value: value["source_sha256"].update({RUNNER_SOURCE: "0" * 64})))
        mutant("compiler", lambda root: manifest_change(root, lambda value: value.update(compiler_version="different compiler")))
        mutant("flags", lambda root: manifest_change(root, lambda value: value.update(
            cmake_cache=value["cmake_cache"] + "\nCMAKE_CXX_FLAGS:STRING=-ffast-math\n")))
        mutant("machine", lambda root: manifest_change(root, lambda value: value.update(logical_cpu_count=value["logical_cpu_count"] + 1)))
        mutant("gpu_claim", lambda root: manifest_change(root, lambda value: value.update(gcp_used=True)))
        mutant("separation_convention", lambda root: manifest_change(root, lambda value: value.update(separation_convention="v4")))
        mutant("unknown_census_mode", lambda root: manifest_change(root, lambda value: value.update(census_modes=["approximate"])))
        mutant("duplicate_census_mode", lambda root: manifest_change(root, lambda value: value.update(census_modes=["shared", "shared"])))
        mutant("duplicate_size", lambda root: manifest_change(root, lambda value: value["sizes"].append(value["sizes"][0])))
        mutant("boolean_seed", lambda root: manifest_change(root, lambda value: value.update(seeds=[True])))
        mutant("closing_source", lambda root: completion_change(root, lambda value: value["source_sha256_closing"].update({RUNNER_SOURCE: "0" * 64})))
        mutant("closing_binary", lambda root: completion_change(root, lambda value: value.update(probe_sha256_closing="0" * 64)))
        mutant("attempts", lambda root: completion_change(root, lambda value: value.update(attempts=value["attempts"] + 1)))
        mutant("unfinished", lambda root: completion_change(root, lambda value: value.update(status="failed")))
        mutant("wrong_command", lambda root: record_change(root, lambda value: value["command"].__setitem__(5, "4")))
        mutant("raw_divergence", lambda root: record_change(root, lambda value: value.update(stdout="{}\n")))
        mutant("binary_after", lambda root: record_change(root, lambda value: value.update(probe_sha256_after="0" * 64)))
        mutant("boolean_exit", lambda root: record_change(root, lambda value: value.update(exit_code=False)))
        mutant("nonserial_time", lambda root: record_change(root, lambda value: value.update(finished_utc="2000-01-01T00:00:00+00:00")))
        mutant("wrong_tuple", lambda root: record_change(root, lambda value: value["result"].update(n=32), True))
        mutant("input_hash", lambda root: record_change(root, lambda value: value["result"].update(input_hash="0"), True))
        mutant("wrong_recipe", lambda root: record_change(root, lambda value: value["result"].update(recipe="parallel_rows_v1"), True))
        mutant("boolean_count", lambda root: work_change(root, "product_visits", True))
        mutant("wrong_partition_work", lambda root: work_change(root, "diagonal_splits", 0))
        mutant("unbounded_descent", lambda root: work_change(root, "witness_descent_steps", 1 << 50, True))
        mutant("hidden_distances", lambda root: work_change(root, "witness_box_distance_tests", 0, True))
        mutant("unbounded_proposals", lambda root: work_change(root, "proposed_sites", 1 << 50, True))
        mutant("wrong_pair_total", lambda root: record_change(root, lambda value: value["result"].update(total_unordered_pairs=0), True))
        mutant("wrong_lane_mask", lambda root: record_change(root, lambda value: value["result"].update(active_lane_mask=7), True))
        mutant("lost_residual_mass", lambda root: work_change(root, "residual_pair_mass", [0, 0, 0]))
        mutant("pure_rejection", lambda root: work_change(root, "rejected_pair_mass", [1, 0, 0]))
        mutant("lost_bin_mass", lambda root: work_change(root, "size_class_pair_mass", [0, 0, 0, 0, 0]))
        mutant("wrong_digest", lambda root: record_change(root, lambda value: value["result"]["digest"].update(sum="0"), True))
        mutant("lost_rng", lambda root: record_change(root, lambda value: value["result"]["generation_work"].update(rng_calls=0), True))
        mutant("repeated_cloud", lambda root: record_change(root, lambda value: value["result"]["cloud_work"].update(coordinate_copies=32), True))
        mutant("invalid_memory", lambda root: record_change(root, lambda value: value["result"]["memory"].update(index_retained_bytes=0), True))
        mutant("omitted_time", lambda root: record_change(root, lambda value: value["result"]["timings"].update(total_ms=0), True))

        mutant("wrong_census_command", lambda root: record_change(root, lambda value: value["command"].__setitem__(7, "shared")))
        mutant("lost_payload_shell", lambda root: record_change(root, lambda value: value["result"]["digest"].update(shell_ids=0), True))
        mutant("lost_payload_count", lambda root: record_change(root, lambda value: value["result"]["census_work"].update(payload_supports=0), True))
        mutant("wrong_census_mass", lambda root: record_change(root, lambda value: value["result"].update(candidate_pairs=0), True))
        mutant("rebuilt_query_index", lambda root: record_change(root, lambda value: value["result"]["census_work"].update(query_build_nodes=1), True))
        mutant("query_cover_scan", lambda root: record_change(root, lambda value: value["result"]["census_work"].update(query_cover_visits=1), True))
        mutant("wrong_census_roots", lambda root: record_change(root, lambda value: value["result"]["census_work"].update(count_root_starts=0), True))
        mutant("anchor_as_descriptor", lambda root: record_change(root, lambda value: value["result"]["census_work"].update(input_descriptors=0), True))
        mutant("uncounted_callback_copy", lambda root: record_change(root, lambda value: value["result"]["callback_work"].update(copied_ids=0), True))
        mutant("uncounted_callback_hash", lambda root: record_change(root, lambda value: value["result"]["callback_work"].update(hash_words=0), True))
        mutant("unpaid_query_build", lambda root: record_change(root, lambda value: value["result"]["timings"].update(query_index_ms=1), True))

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

        failed_probe = temporary / "failing_probe"
        shutil.copy2("/bin/false", failed_probe)
        failed = temporary / "failed_capture"
        result = capture(failed_probe, failed)
        require(result.returncode == 1 and (failed / "COMPLETION.json").is_file(), "failed process did not close")
        failed_rows, completion = records(failed), read(failed / "COMPLETION.json")
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

        # Explicit v2 captures supplement, rather than replace, the v1 gates.
        # Their sibling mode is not a new geometric family or an HGP level.
        versioned = temporary / "versioned"
        versioned.mkdir()
        shutil.copytree(genuine, versioned / "legacy")
        for name in ("first", "second"):
            destination = versioned / name
            result = capture_sibling(binary, destination)
            require(result.returncode == 0 and not result.stderr,
                    f"genuine sibling capture failed: {result.stdout!r} {result.stderr!r}")
            rows = records(destination)
            manifest, completion = read(destination / "MANIFEST.json"), read(destination / "COMPLETION.json")
            require(manifest["schema"] == "mhgp8_wspd_q2_campaign_v2" and
                    manifest["sibling_modes"] == ["none", "sibling"] and
                    completion["status"] == "completed" and completion["runs"] == completion["attempts"] == len(rows) == 32,
                    "v2 capture lost version, modes or tuples")
            require(all(row["result"]["schema"] == "mhgp8_wspd_q2_census_probe_v2" and
                        row["result"]["digest"]["supports"] > 0 for row in rows),
                    "v2 support fixture is vacuous or mislabeled")
            enabled = [row["result"]["sibling_work"] for row in rows
                       if row["result"]["sibling_mode"] == "sibling"]
            require(any(work["proposals"] > 0 for work in enabled) and
                    any(work["bound_tests"] > 0 for work in enabled) and
                    any(work["rejected_tasks"] > 0 for work in enabled),
                    "v2 certificate fixture never proposes, tests or rejects")
            stats["sibling_rows"] += len(rows)
        result = checked(versioned)
        require(result.returncode == 0 and not result.stderr,
                f"mixed v1/v2 reader failed: {result.stdout!r} {result.stderr!r}")
        summary = parse_result(result.stdout)
        require(summary["measurements"] == 192 and len(summary["summary"]) == 96 and
                sum("sibling_mode" in row for row in summary["summary"]) == 32 and
                summary.get("sibling_comparison") == "same_front_candidates_and_canonical_digest_not_equal_census_work",
                "mixed-version summary lost historical or explicit mode keys")
        stats["sibling_positive_reads"] += 1

        def sibling_change(root: Path, update: Callable[[dict], None], enabled: bool = True) -> None:
            directory = root / "first"
            rows = records(directory)
            chosen = next(row for row in rows if
                          (row["result"]["sibling_mode"] == "sibling") == enabled)
            update(chosen)
            refresh(chosen)
            write_records(directory, rows)

        def sibling_work(root: Path, field: str, change: Callable[[dict], Any], enabled: bool = True) -> None:
            sibling_change(root, lambda row: row["result"]["sibling_work"].update({field: change(row["result"])}), enabled)

        mutant("sibling_unknown_mode", lambda root: manifest_change(root, lambda m: m.update(sibling_modes=["wrong"])), versioned)
        mutant("sibling_duplicate_mode", lambda root: manifest_change(root, lambda m: m.update(sibling_modes=["none", "none"])), versioned)
        mutant("sibling_pairwise_matrix", lambda root: manifest_change(root, lambda m: m.update(census_modes=["pairwise", "shared"])), versioned)
        mutant("sibling_unversioned_manifest", lambda root: manifest_change(root, lambda m: m.update(schema="mhgp8_wspd_q2_campaign_v1")), versioned)
        mutant("sibling_unversioned_result", lambda root: sibling_change(root, lambda r: r["result"].update(schema="mhgp8_wspd_q2_census_probe_v1")), versioned)
        mutant("sibling_wrong_command", lambda root: sibling_change(root, lambda r: r["command"].__setitem__(8, "none")), versioned)
        mutant("sibling_missing_field", lambda root: sibling_change(root, lambda r: r["result"]["sibling_work"].pop("proposals")), versioned)
        mutant("sibling_boolean_count", lambda root: sibling_work(root, "proposals", lambda _r: True), versioned)
        mutant("sibling_hidden_proposal", lambda root: sibling_work(root, "proposals", lambda r: r["sibling_work"]["proposals"] + 1), versioned)
        mutant("sibling_hidden_bound", lambda root: sibling_work(root, "bound_tests", lambda r: r["sibling_work"]["bound_tests"] + 1), versioned)
        mutant("sibling_hidden_skip", lambda root: sibling_work(root, "cardinality_skips", lambda r: r["sibling_work"]["cardinality_skips"] + 1), versioned)
        mutant("sibling_excess_reject", lambda root: sibling_work(root, "rejected_tasks", lambda r: r["sibling_work"]["bound_tests"] + 1), versioned)
        mutant("sibling_excess_credit", lambda root: sibling_work(root, "rejected_after_credit", lambda r: r["sibling_work"]["rejected_tasks"] + 1), versioned)
        mutant("sibling_excess_pairs", lambda root: sibling_work(root, "rejected_pairs", lambda r: r["rejected_pairs"] + 1), versioned)
        def sibling_pairs_without_tasks(root: Path) -> None:
            directory = root / "first"
            rows = records(directory)
            chosen = next(row for row in rows if row["result"]["sibling_mode"] == "sibling" and
                          row["result"]["sibling_work"]["rejected_tasks"] == 0 and
                          row["result"]["rejected_pairs"] > 0)
            chosen["result"]["sibling_work"]["rejected_pairs"] = 1
            refresh(chosen)
            write_records(directory, rows)
        mutant("sibling_pairs_without_tasks", sibling_pairs_without_tasks, versioned)

        def lost_sibling_pairs(root: Path) -> None:
            directory = root / "first"
            rows = records(directory)
            chosen = next(row for row in rows if row["result"]["sibling_work"]["rejected_tasks"] > 0)
            work = chosen["result"]["sibling_work"]
            work["rejected_pairs"] = work["rejected_tasks"] - 1
            refresh(chosen)
            write_records(directory, rows)
        mutant("sibling_fewer_pairs_than_tasks", lost_sibling_pairs, versioned)
        mutant("sibling_disabled_work", lambda root: sibling_work(root, "bound_tests", lambda _r: 1, False), versioned)
        mutant("sibling_changed_front", lambda root: sibling_change(root, lambda r:
            r["result"]["front_work"].update(max_factor_size=r["result"]["front_work"]["max_factor_size"] + 1)), versioned)
        mutant("sibling_changed_digest", lambda root: sibling_change(root, lambda r: r["result"]["digest"].update(sum="0")), versioned)

        # Exercise the public CLI rejection, not just receipt metadata.
        invalid_cli = [str(original), "16", "rows", "5", "8", "3", "pure", "pairwise", "sibling"]
        require(subprocess.run(invalid_cli, cwd=ROOT, capture_output=True).returncode == 2,
                "probe accepted pairwise+sibling")
        invalid_cli[7], invalid_cli[8] = "shared", "unknown"
        require(subprocess.run(invalid_cli, cwd=ROOT, capture_output=True).returncode == 2,
                "probe accepted an unknown sibling mode")
        stats["sibling_cli_rejects"] += 2
    require(stats["genuine_rows"] == 128 and stats["mutants"] >= 48 and stats["failed_captures"] == 2 and
            stats["positive_reads"] == 1 and stats["domain_checks"] == 2,
            "receipt gate lost its non-vacuity floor")
    require(stats["sibling_rows"] == 64 and stats["sibling_positive_reads"] == 1 and
            stats["sibling_cli_rejects"] == 2 and stats["mutants"] >= 67,
            "sibling extension lost its non-vacuity floor")
    require(sources() == pins and digest(original) == binary_pin, "gate modified sources or actual probe")
    print(json.dumps(dict(status="passed", **stats), sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, RuntimeError, OSError, KeyError, TypeError) as error:
        print(f"WSPD q2 receipt gate failed: {error}", file=sys.stderr)
        raise SystemExit(1) from error
