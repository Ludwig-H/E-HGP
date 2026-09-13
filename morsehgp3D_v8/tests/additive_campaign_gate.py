#!/usr/bin/env python3
"""Exercise additive paired receipts, cross-variant references and complete costs.

Existing campaign gates cover malformed JSON, signals and binary changes. This
bounded extension covers only the new additive/intersection receipt contracts.
"""

from __future__ import annotations

import argparse
import base64
from collections import Counter, defaultdict
import json
from pathlib import Path
import shutil
import statistics
import subprocess
import sys
import tempfile
from typing import Any, Callable

from paired_reader_gate import (BENCH, READER, ROOT, RUNNER, digest, provenance_field,
                                 python_command, read, write)

sys.path.insert(0, str(BENCH))
from paired_receipts import validate_additive  # noqa: E402
from run_p0_matrix import parse_result  # noqa: E402


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def rows_at(campaign: Path) -> list[dict[str, Any]]:
    return [parse_result(line) for line in
            (campaign / "MEASURES.jsonl").read_bytes().splitlines()]


def store_rows(campaign: Path, rows: list[dict[str, Any]]) -> None:
    (campaign / "MEASURES.jsonl").write_text(
        "".join(json.dumps(row, allow_nan=False) + "\n" for row in rows))


def raw_from_result(record: dict[str, Any]) -> None:
    raw = (json.dumps(record["result"], allow_nan=False) + "\n").encode()
    record["stdout"] = raw.decode()
    record["stdout_base64"] = base64.b64encode(raw).decode("ascii")


def campaign(probe: Path, output: Path) -> tuple[Any, dict[str, Any], list[Any]]:
    arguments = ["--probe", str(probe), "--probe-kind", "additive", "--output", str(output),
                 "--sizes", "128", "--families", "sheet_full", "--strategies",
                 "pool", "dual", "tubes", "--kmax", "5", "--s", "8", "--repeats", "1"]
    result = subprocess.run(python_command(RUNNER, arguments), capture_output=True, cwd=ROOT)
    require((output / "COMPLETION.json").is_file(), "additive campaign was not closed")
    completion = read(output / "COMPLETION.json")
    rows = rows_at(output)
    require(completion["attempts"] == len(rows) and
            completion["runs"] == sum(row["status"] == "completed" for row in rows),
            "additive campaign lost or miscounted attempted rows")
    for row in rows:
        for field in ("stdout", "stderr"):
            require(base64.b64decode(row[f"{field}_base64"], validate=True).decode(
                "utf-8", errors="replace") == row[field], "additive campaign lost raw output")
    return result, completion, rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    source_probe = args.probe.resolve()
    original_pins = {path: digest(path) for path in
                     (source_probe, RUNNER, READER, BENCH / "paired_receipts.py")}
    stats: Counter[str] = Counter()
    with tempfile.TemporaryDirectory(prefix="mhgp8_additive_campaign_gate_") as name:
        temporary = Path(name)
        binary_dir = temporary / "bin"
        binary_dir.mkdir()
        actual = binary_dir / "actual_probe"
        shutil.copy2(source_probe, actual)
        shutil.copy2(source_probe.parent / "CMakeCache.txt", binary_dir / "CMakeCache.txt")
        genuine = temporary / "genuine"
        genuine.mkdir()
        all_rows = []
        for name in ("first", "repeat"):
            result, completion, rows = campaign(actual, genuine / name)
            require(result.returncode == 0 and not result.stderr and
                    completion["status"] == "completed" and completion["runs"] == 12 and
                    completion["source_hashes_unchanged"] is True and
                    completion["probe_hash_unchanged"] is True,
                    f"genuine additive matrix failed: {completion}: {result.stderr!r}")
            all_rows.extend(rows)
            stats["genuine_campaigns"] += 1
            stats["genuine_measurements"] += len(rows)
        result = subprocess.run(python_command(READER, [str(genuine), "--summary"]),
                                capture_output=True, cwd=ROOT)
        require(result.returncode == 0 and not result.stderr,
                f"genuine additive reader failed: {result.stderr!r}")
        summary = parse_result(result.stdout)
        require(summary["status"] == "passed" and summary["campaigns"] == 2 and
                summary["measurements"] == 24 and
                summary["configurations_including_order"] == len(summary["summary"]) == 12 and
                summary["full_contract_qualified"] is False and summary["gcp_used"] is False,
                "additive reader lost matrix scope or variant/order groups")
        provenance = summary["provenance_by_probe_kind"]
        require(set(provenance) == {"additive"} and
                provenance["additive"]["probe_sha256"] == digest(actual),
                "additive provenance was lost")
        grouped: dict[tuple[str, str, str], list[dict[str, Any]]] = defaultdict(list)
        for record in all_rows:
            row = record["result"]
            grouped[(row["strategy"], row["order"], row["variant"])].append(row)
        for item in summary["summary"]:
            rows = grouped[(item["strategy"], item["order"], item["variant"])]
            require(item["repeats"] == len(rows) == 2 and item["provenance"] == provenance["additive"],
                    "additive summary merged variants/orders or mixed provenance")
            for field in ("independent_ms", "local_plan_ms", "selection_ms", "variant_ms",
                          "independent_total_ms", "variant_total_ms"):
                require(item[f"median_{field}"] == statistics.median(row[field] for row in rows),
                        "additive summary median was not computed within its paired group")
        stats["positive_reads"] += 1

        # Real subprocess counterfeits: each mutation retains the raw invalid
        # output and must close invalid, never count a false completed run.
        prelude = (f"#!{sys.executable}\nimport json, subprocess, sys\n"
                   f"real = {str(actual)!r}\n"
                   "captured = subprocess.run([real, *sys.argv[1:]], capture_output=True, check=True)\n"
                   "row = json.loads(captured.stdout)\n")
        fakes = {
            "wrong_variant": "row['variant'] = 'union'\n",
            "wrong_independent_kind": "row['independent_kind'] = 'local_q2_credit_plan'\n",
            "omit_local_cost": (
                "if row['variant'] == 'intersection': row['variant_ms'] -= row['local_plan_ms']\n"),
            "wrong_private_copies": (
                "if row['variant'] == 'intersection': "
                "row['variant_work']['restriction_credit_copies'] -= 1\n"),
            "cross_variant_checksum": (
                "if row['variant'] == 'intersection': row['independent_checksum'] = '1'\n"),
            "cross_variant_work": (
                "if row['variant'] == 'intersection': "
                "row['independent_work']['sort_comparisons'] += 1\n"),
            "cross_strategy_additive": (
                "if row['variant'] == 'additive' and row['strategy'] == 'dual': "
                "row['variant_checksum'] = '1'\n"),
        }
        for label, body in fakes.items():
            fake = binary_dir / label
            fake.write_text(prelude + body + "print(json.dumps(row))\n")
            fake.chmod(0o755)
            result, completion, rows = campaign(fake, temporary / f"failed_{label}")
            require(result.returncode == 1 and completion["status"] == "invalid" and
                    rows[-1]["status"] == "invalid" and bool(rows[-1]["stdout_base64"]) and
                    completion["probe_hash_unchanged"] is True and
                    completion["source_hashes_unchanged"] is True,
                    f"additive counterfeit survived or lost proof: {label}: {completion}")
            stats["runner_mutants"] += 1

        def reject(label: str, mutate: Callable[[Path], None], error_fragment: str) -> None:
            target = temporary / f"reader_{label}"
            shutil.copytree(genuine, target)
            mutate(target)
            result = subprocess.run(python_command(READER, [str(target)]),
                                    capture_output=True, cwd=ROOT)
            require(result.returncode == 1 and not result.stdout and
                    error_fragment in result.stderr.decode("utf-8", errors="replace"),
                    f"additive reader mutant survived or wrong rejection: {label}: {result.stderr!r}")
            stats["reader_mutants"] += 1

        def cross_reference(root: Path, field: str) -> None:
            for campaign_path in (root / "first", root / "repeat"):
                rows = rows_at(campaign_path)
                changed = 0
                for record in rows:
                    row = record["result"]
                    if row["variant"] != "intersection":
                        continue
                    if field == "checksum":
                        row["independent_checksum"] = "1" if row["independent_checksum"] != "1" else "2"
                    else:
                        row["independent_work"]["sort_comparisons"] += 1
                    # All individually valid and all rows of this variant
                    # mutated alike: only a cross-variant comparison refutes it.
                    validate_additive(row, record["command"])
                    raw_from_result(record)
                    changed += 1
                require(changed == 6, "cross-variant mutant missed an order or strategy")
                store_rows(campaign_path, rows)

        for field in ("checksum", "work"):
            reject(f"cross_variant_{field}", lambda root, field=field: cross_reference(root, field),
                   "unrestricted reference changed across strategies or variants")

        def cross_strategy(root: Path) -> None:
            for campaign_path in (root / "first", root / "repeat"):
                rows = rows_at(campaign_path)
                changed = 0
                for record in rows:
                    row = record["result"]
                    if row["variant"] == "additive" and row["strategy"] == "dual":
                        row["variant_checksum"] = "1" if row["variant_checksum"] != "1" else "2"
                        validate_additive(row, record["command"])
                        raw_from_result(record)
                        changed += 1
                require(changed == 2, "cross-strategy mutant missed an execution order")
                store_rows(campaign_path, rows)
        reject("cross_strategy", cross_strategy,
               "unrestricted reference changed across strategies or variants")

        def one_record(root: Path, mode: str) -> None:
            campaign_path = root / "first"
            rows = rows_at(campaign_path)
            record = next(row for row in rows if row["result"]["variant"] == "intersection")
            row = record["result"]
            if mode == "exit_code":
                record["exit_code"] = False
            elif mode == "kind":
                row["independent_kind"] = "local_q2_credit_plan"
            elif mode == "cost":
                row["variant_ms"] -= row["local_plan_ms"]
            elif mode == "copies":
                row["variant_work"]["restriction_credit_copies"] -= 1
            elif mode == "slab":
                row["variant_work"]["axis_slab_rejects"] += 1
            elif mode == "counter_bool":
                row["variant_work"]["axis_count_queries"] = True
            elif mode == "coalescence":
                row["variant_work"]["coalesced_blocks"] += 1
            else:
                raise RuntimeError(f"unknown row mutation {mode}")
            raw_from_result(record)
            store_rows(campaign_path, rows)

        for mode, fragment in (
                ("exit_code", "exit_code"), ("kind", "wrong independent baseline"),
                ("cost", "variant: inconsistent timing partition"),
                ("copies", "restriction credits not copied exactly once"),
                ("slab", "pruning reasons disagree with rejected nodes"),
                ("counter_bool", "axis_count_queries"),
                ("coalescence", "inconsistent selection accounting")):
            reject(mode, lambda root, mode=mode: one_record(root, mode), fragment)

        for field in ("compiler_version", "cpuinfo"):
            def mutate_provenance(root: Path, field: str = field) -> None:
                current = read(root / "repeat/MANIFEST.json")[field]
                provenance_field(root / "repeat", field, current + "\nmutant: alternate_metadata\n")
            reject(field, mutate_provenance, "heterogeneous")

        def remove_fixture_source(root: Path) -> None:
            path = root / "first"
            manifest = read(path / "MANIFEST.json")
            completion = read(path / "COMPLETION.json")
            manifest["source_sha256"].pop("morsehgp3D_v8/bench/sheet_full_fixture.hpp")
            completion["source_sha256_closing"] = manifest["source_sha256"]
            write(path / "MANIFEST.json", manifest)
            write(path / "COMPLETION.json", completion)
        reject("fixture_source", remove_fixture_source, "source coverage")

        def duplicate_variant(root: Path) -> None:
            path = root / "first/MANIFEST.json"
            manifest = read(path)
            manifest["variants"].append(manifest["variants"][0])
            write(path, manifest)
        reject("duplicate_variant", duplicate_variant, "variants")

    require(stats["genuine_campaigns"] == 2 and stats["genuine_measurements"] == 24 and
            stats["positive_reads"] == 1 and stats["runner_mutants"] == 7 and
            stats["reader_mutants"] == 14, "additive campaign non-vacuity floor")
    require(all(digest(path) == expected for path, expected in original_pins.items()),
            "additive gate modified a source or supplied binary")
    print(json.dumps({"status": "passed", **stats, "python_optimized": bool(sys.flags.optimize),
                      "scope": "additive_component_receipts_not_geometry_or_full"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
