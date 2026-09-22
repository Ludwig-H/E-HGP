#!/usr/bin/env python3
"""Test paired receipt validation/closure; no geometry or FULL qualification.

The single-probe campaign_gate.py already covers signals, malformed bytes and
child-process cancellation. This gate adds batch/axis tuple, work-sharing and
paired-time contracts, with real tiny campaigns and preserved invalid output.
"""

from __future__ import annotations

import argparse
import base64
from collections import Counter
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from typing import Any, Callable


ROOT = Path(__file__).resolve().parents[2]
BENCH = ROOT / "morsehgp3D_v8/bench"
RUNNER = BENCH / "run_p0_matrix.py"
sys.path.insert(0, str(BENCH))

from paired_receipts import stable_signature, validate_axis, validate_batch  # noqa: E402
from run_p0_matrix import parse_result  # noqa: E402


Validator = Callable[[dict[str, Any], list[str]], None]
Mutation = Callable[[dict[str, Any]], None]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def clone(row: dict[str, Any]) -> dict[str, Any]:
    return parse_result(json.dumps(row, allow_nan=False).encode("utf-8"))


def set_path(path: tuple[str | int, ...], value: Any) -> Mutation:
    def mutate(row: dict[str, Any]) -> None:
        target: Any = row
        for key in path[:-1]:
            target = target[key]
        target[path[-1]] = value
    return mutate


def rejected(function: Callable[[], Any], label: str, stats: Counter[str]) -> None:
    try:
        function()
    except (ValueError, KeyError, TypeError):
        stats["direct_rejections"] += 1
        return
    raise RuntimeError(f"paired receipt mutant survived: {label}")


def erase_batch_work(row: dict[str, Any]) -> None:
    def zero(work: dict[str, Any]) -> None:
        for key in work:
            if key == "predicates":
                work[key] = dict.fromkeys(work[key], 0)
            else:
                work[key] = 0
    zero(row["shared_work"])
    for lane in row["lanes"]:
        zero(lane["baseline_work"])
        zero(lane["batch_work"])


def exercise_mutations(kind: str, row: dict[str, Any], command: list[str],
                       validate: Validator, stats: Counter[str]) -> None:
    alternate_order = "batch-first" if kind == "batch" else "axis-first"
    tuple_values = {"n": 16, "strategy": "pool", "family": "sheet", "kmax": 5,
                    "separation_s": 12, "order": alternate_order}
    for field, value in tuple_values.items():
        mutated = clone(row)
        mutated[field] = value
        rejected(lambda: validate(mutated, command), f"{kind}.tuple.{field}", stats)
        stats["tuple_rejections"] += 1

    mutations: list[tuple[str, Mutation]] = []

    def add(label: str, path: tuple[str | int, ...], value: Any) -> None:
        mutations.append((label, set_path(path, value)))

    for field, value in {
        "schema": "foreign_schema", "status": "failed", "scope": "FULL",
        "backend": "gpu", "profile": "float_input", "public_status": "exact",
        "s_role": "wspd_generation", "totals_kind": "sum_of_both_arms",
        "same_owner": 1, "candidates_expanded": 0, "downstream_measured": 0,
        "threads": True, "owner_preparations": True, "fixture_version": True,
        "seed": 0, "n_a": row["n_a"] + 1, "n_b": True,
        "input_fnv1a64_le_u16_xyz": "not_hex",
        "paired_execution_ms": 0, "generation_ms": -1,
        "prepare_ms": True, "baseline_total_ms": row["baseline_total_ms"] + 1,
    }.items():
        add(field, (field,), value)
    for value, label in ((float("nan"), "nan"), (float("inf"), "inf"),
                         (-float("inf"), "minus_inf")):
        add(f"time_{label}", ("generation_ms",), value)
    for label, path, value in (
        ("prep_missing", ("preparation_work",), None),
        ("prep_unknown", ("preparation_work", "unknown"), 0),
        ("predicate_unknown", ("preparation_work", "predicates", "unknown"), 0),
        ("prep_validation", ("preparation_work", "validation_points"), row["n"] - 1),
        ("work_bool", ("preparation_work", "uniqueness_comparisons"), True),
        ("work_negative", ("preparation_work", "uniqueness_comparisons"), -1),
        ("work_overflow", ("preparation_work", "uniqueness_comparisons"), 1 << 64),
    ):
        add(label, path, value)

    if kind == "batch":
        first = ("lanes", 0)
        add("plans_bool", ("plans_identical",), 1)
        add("checksum_kind", ("checksum_kind",), "foreign_checksum")
        add("lane_null", first, None)
        add("lanes_missing", ("lanes",), [])
        add("lane_order", (*first, "lane"), 4)
        add("threshold", (*first, "threshold"), 9)
        add("core", (*first, "core_credit"), 1)
        add("pair_total", (*first, "total_pairs"), row["lanes"][0]["total_pairs"] + 1)
        add("candidates", (*first, "candidate_pairs"), row["lanes"][0]["total_pairs"] + 1)
        add("descriptors", (*first, "candidate_descriptors"), 1000)
        add("empty_descriptors", (*first, "candidate_descriptors"), 0)
        add("checksum_syntax", (*first, "batch_checksum"), "xyz")
        old_hash = row["lanes"][0]["baseline_checksum"]
        add("checksum_disagreement", (*first, "batch_checksum"),
            "1" if old_hash != "1" else "2")
        add("shared_missing", ("shared_work",), None)
        add("shared_unknown", ("shared_work", "unknown"), 0)
        add("shared_query", ("shared_work", "tube_sweep_tests"), 1)
        add("shared_preparation", ("shared_work", "tube_records"),
            row["shared_work"]["tube_records"] + 1)
        add("new_work_unknown", (*first, "batch_work", "unknown"), 0)
        add("old_work_unknown", (*first, "baseline_work", "unknown"), 0)
        add("new_work_missing", (*first, "batch_work"), None)
        add("query_changed", (*first, "batch_work", "tube_sweep_tests"),
            row["lanes"][0]["batch_work"]["tube_sweep_tests"] + 1)
        add("prep_repeated", (*first, "batch_work", "tube_records"), 1)
        add("prep_counter_bool", (*first, "batch_work", "tube_records"), False)
        add("new_counter_bool", (*first, "batch_work", "tube_credited_sites"), True)
        add("batch_total", ("batch_total_ms",), row["batch_total_ms"] + 1)
        mutations.append(("all_tube_work_erased", erase_batch_work))
    else:
        for field, value in {
            "baseline_kind": "three_lanes", "total_pairs": row["total_pairs"] + 1,
            "baseline_candidates": row["total_pairs"] + 1,
            "axis_candidates": row["total_pairs"] + 1, "axis_descriptors": 0,
            "axis_checksum": "xyz", "axis_total_ms": row["axis_total_ms"] + 1,
            "checksum_kind": "fnv1a64_be_u64_axis_plan_v0",
        }.items():
            add(field, (field,), value)
        for field, value in {
            "unknown": 0, "sort_passes": 2, "sorted_sites": 3 * row["n_a"] + 1,
            "constrained_anchors": row["n_a"] + 1, "max_tree_depth": 55,
            "emitted_blocks": row["axis_descriptors"] + 1,
            "slab_bound_updates": 6 * row["n_a"] + 1,
            "query_nodes": True, "tree_nodes": -1,
            "columns": 1 << 64,
        }.items():
            add(f"axis_work_{field}", ("axis_work", field), value)
        add("axis_work_missing", ("axis_work",), None)
        add("baseline_work_unknown", ("baseline_work", "unknown"), 0)
        add("baseline_work_missing", ("baseline_work",), None)
        add("baseline_predicate_bool", ("baseline_work", "predicates", "point_tests"), True)

    for label, mutate in mutations:
        mutated = clone(row)
        mutate(mutated)
        rejected(lambda: validate(mutated, command), f"{kind}.{label}", stats)

    # Reject non-finite values at ingestion too, not only inside validators.
    for token in ("NaN", "Infinity", "-Infinity", "1e999"):
        payload = b'{"generation_ms":' + token.encode("ascii") + b'}'
        rejected(lambda: parse_result(payload), f"{kind}.parse.{token}", stats)
        stats["parser_rejections"] += 1


def campaign(probe: Path, kind: str, output: Path) -> tuple[Any, dict[str, Any], list[Any]]:
    flags = ["-B", "-O"] if sys.flags.optimize else ["-B"]
    command = [sys.executable, *flags, str(RUNNER), "--probe", str(probe),
               "--probe-kind", kind, "--output", str(output), "--sizes", "8",
               "--families", "grid", "--strategies", "tubes", "--kmax", "10",
               "--s", "8", "--repeats", "1"]
    result = subprocess.run(command, capture_output=True, cwd=ROOT)
    require((output / "COMPLETION.json").is_file(), "paired campaign was not closed")
    completion = parse_result((output / "COMPLETION.json").read_bytes())
    measures = output / "MEASURES.jsonl"
    rows = [parse_result(line) for line in measures.read_bytes().splitlines()]
    require(completion["attempts"] == len(rows), "paired campaign lost an attempt")
    require(completion["runs"] == sum(row["status"] == "completed" for row in rows),
            "paired campaign miscounted completed runs")
    for row in rows:
        for name in ("stdout", "stderr"):
            raw = base64.b64decode(row[f"{name}_base64"], validate=True)
            require(raw.decode("utf-8", errors="replace") == row[name],
                    f"paired campaign lost raw {name}")
    return result, completion, rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--batch-probe", required=True, type=Path)
    parser.add_argument("--axis-probe", required=True, type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    sources = {"batch": args.batch_probe.resolve(), "axis": args.axis_probe.resolve()}
    source_pins = {kind: digest(path) for kind, path in sources.items()}
    control_pins = {path: digest(path) for path in (RUNNER, BENCH / "paired_receipts.py")}
    stats: Counter[str] = Counter()
    with tempfile.TemporaryDirectory(prefix="mhgp8_paired_campaign_gate_") as directory:
        temporary = Path(directory)
        for kind, source in sources.items():
            target = temporary / kind
            target.mkdir()
            actual = target / "actual_probe"
            shutil.copy2(source, actual)
            shutil.copy2(source.parent / "CMakeCache.txt", target / "CMakeCache.txt")
            validate = validate_batch if kind == "batch" else validate_axis
            alternate = "batch-first" if kind == "batch" else "axis-first"
            signatures = []
            for order in ("baseline-first", alternate):
                command = [str(actual), "8", "tubes", "grid", "10", "8", order]
                result = subprocess.run(command, capture_output=True, cwd=ROOT)
                require(result.returncode == 0 and not result.stderr,
                        f"real {kind} probe failed: {result.stderr!r}")
                row = parse_result(result.stdout)
                validate(row, command)
                signatures.append(stable_signature(row, kind))
                stats["genuine_probe_rows"] += 1
                if order == "baseline-first":
                    exercise_mutations(kind, row, command, validate, stats)
            require(signatures[0] == signatures[1], "execution order changed paired objects")

            output = target / "genuine_campaign"
            result, completion, rows = campaign(actual, kind, output)
            require(result.returncode == 0 and completion["status"] == "completed" and
                    completion["runs"] == 2 and len(rows) == 2,
                    f"real {kind} campaign failed: {completion}: {result.stderr!r}")
            manifest = parse_result((output / "MANIFEST.json").read_bytes())
            require(manifest["probe_kind"] == kind and
                    manifest["orders"] == ["baseline-first", alternate] and
                    manifest["receipt_validation_version"] == 2,
                    "paired manifest lost probe kind/order/validation version")
            require(completion["source_hashes_unchanged"] is True and
                    completion["probe_hash_unchanged"] is True and
                    manifest["source_sha256"] == completion["source_sha256_closing"] and
                    manifest["probe_sha256"] == completion["probe_sha256_closing"],
                    "paired campaign did not close its source/binary pins")
            require("morsehgp3D_v8/bench/p0_fixtures.hpp" in manifest["source_sha256"] and
                    "morsehgp3D_v8/bench/paired_receipts.py" in manifest["source_sha256"],
                    "shared recipe or validator was omitted from campaign pins")
            for record in rows:
                require(record["probe_sha256_before"] == record["probe_sha256_after"] ==
                        source_pins[kind], "paired invocation has wrong binary pins")
                validate(record["result"], record["command"])
            require(stable_signature(rows[0]["result"], kind) ==
                    stable_signature(rows[1]["result"], kind),
                    "campaign order changed stable paired output")
            stats["genuine_campaigns"] += 1
            stats["genuine_campaign_rows"] += len(rows)

            # Exercise the real runner, not just its validator: a completed
            # proxy process lies about n. Preserve its exact stdout as evidence.
            proxy = target / "wrong_tuple_proxy"
            proxy.write_text(
                f"#!{sys.executable}\n"
                "import json, subprocess, sys\nfrom pathlib import Path\n"
                f"real = {str(actual)!r}\n"
                "result = subprocess.run([real, *sys.argv[1:]], capture_output=True, check=True)\n"
                "row = json.loads(result.stdout)\nrow['n'] += 1\n"
                "raw = (json.dumps(row) + '\\n').encode('utf-8')\n"
                "Path(__file__).with_suffix('.stdout').write_bytes(raw)\n"
                "sys.stdout.buffer.write(raw)\n")
            proxy.chmod(0o755)
            result, completion, rows = campaign(proxy, kind, target / "invalid_campaign")
            require(result.returncode == 1 and completion["status"] == "invalid" and
                    completion["runs"] == 0 and completion["attempts"] == 1 and
                    len(rows) == 1 and rows[0]["status"] == "invalid" and
                    rows[0]["exit_code"] == 0 and "result" not in rows[0],
                    f"bad {kind} tuple was promoted or misclassified: {completion}")
            expected = proxy.with_suffix(".stdout").read_bytes()
            require(bool(expected) and base64.b64decode(rows[0]["stdout_base64"]) == expected and
                    rows[0]["stdout"] == expected.decode("utf-8") and
                    completion["probe_hash_unchanged"] is True,
                    "invalid paired stdout bytes or stable proxy pin were lost")
            require("n" in rows[0]["error"], "bad tuple failed for an unrelated reason")
            stats["proxy_campaigns_rejected"] += 1
            require(digest(actual) == source_pins[kind], "gate changed a genuine copied probe")

    require(stats["genuine_probe_rows"] == 4 and stats["genuine_campaigns"] == 2 and
            stats["genuine_campaign_rows"] == 4 and stats["proxy_campaigns_rejected"] == 2 and
            stats["tuple_rejections"] == 12 and stats["parser_rejections"] == 8 and
            stats["direct_rejections"] >= 120, "paired campaign non-vacuity floor")
    require(all(digest(path) == source_pins[kind] for kind, path in sources.items()) and
            all(digest(path) == pin for path, pin in control_pins.items()),
            "gate changed original probes or protocol sources")
    print(json.dumps({"status": "passed", **dict(stats),
                      "python_optimized": bool(sys.flags.optimize),
                      "scope": "paired_component_receipt_ingestion_not_geometry_or_full"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
