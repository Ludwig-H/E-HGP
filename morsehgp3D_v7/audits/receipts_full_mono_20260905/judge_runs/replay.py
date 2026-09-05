#!/usr/bin/env python3
"""Bounded replay of a captured receipt judge; never runs the C++ engine."""
from __future__ import annotations

import argparse
import copy
from decimal import Decimal
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from typing import Any


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
SOURCE = ROOT / "morsehgp3D_v7/receipts/full_gabriel_mono_20260905"
SNAPSHOT = HERE / "judge_at_review.py"
JUDGE_SHA = "24e789459ee7adb8b48819dddc8bef8832b2b152ad9418c1a1d281038315e2c7"
IDS = ["n8000_s8_k10", "n16000_s8_k10", "n32000_s8_k10",
       "n8000_s10_k10", "n8000_s12_k10"]
Json = dict[str, Any]


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def source_paths() -> list[Path]:
    paths = [SOURCE / name for name in
             ("protocol.json", "sources_before.json", "sources_after.json",
              "README.md", "verification.json")]
    for rid in IDS:
        paths.extend(SOURCE / (rid + suffix) for suffix in
                     (".receipt.json", ".raw.txt", ".intent.json"))
    paths.extend(ROOT / name for name in (
        "morsehgp3D_v7/bench/full_gabriel_probe_audit.py",
        "morsehgp3D_v7/bench/full_gabriel_probe.cpp",
        "morsehgp3D_v7/src/pipeline/run.hpp",
        "morsehgp3D_v7/docs/RESULTATS_MONO_FULL_20260905.md"))
    return paths


def source_pins() -> dict[str, str]:
    return {str(p.relative_to(ROOT)): sha(p) for p in source_paths()}


def strengthened(rows: list[Json]) -> list[str]:
    """Necessary EAGER/source-e02d identities, not geometric sufficiency."""
    n = rows[0]["n"]
    errors: list[str] = []
    for row in rows[1:-1]:
        if row["outcome"] != "complete_relative":
            continue
        k = row["k"]
        leaves = row["certificate_minima"]
        nodes = row["certificate_nodes"]
        expected_leaves = n if k == 1 else row["minimum_catalogue_records"]
        if leaves != expected_leaves:
            errors.append(f"k{k}:minimum_catalogue_binding")
        if nodes > 2 * leaves - 1:
            errors.append(f"k{k}:merge_arity_forest_bound")
        if row["meb_calls"] != row["portal_requests"] + row["chain_steps"]:
            errors.append(f"k{k}:physical_meb_decomposition")
        equal_facets = 2 * (k + 1) * row["connection_catalogue_records"] - row["face_visits"]
        if equal_facets < 0 or row["aliases"] != leaves + equal_facets + row["portal_requests"]:
            errors.append(f"k{k}:eager_alias_decomposition")
    terminal = rows[-1]
    elapsed = Decimal(str(terminal["elapsed_before_terminal_ms"]))
    # Printed values are rounded independently to six digits after the dot.
    tolerance = Decimal("0.000003")
    if any(Decimal(str(v)) > elapsed + tolerance for v in terminal["stage_ms"].values()):
        errors.append("terminal:stage_exceeds_elapsed")
    output = Decimal(str(terminal["provisional_output_ms"]))
    diagnostic = Decimal(str(terminal["compute_read_release_ms_subtracted_diagnostic"]))
    if abs(diagnostic - max(Decimal(0), elapsed - output)) > tolerance:
        errors.append("terminal:subtracted_diagnostic")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--result", type=Path, required=True)
    args = parser.parse_args()
    require(sha(SNAPSHOT) == JUDGE_SHA, "snapshot_pin")
    before = source_pins()
    spec = importlib.util.spec_from_file_location("mono_judge_at_review", SNAPSHOT)
    require(spec is not None and spec.loader is not None, "import_spec")
    judge = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(judge)
    python = [sys.executable, "-B"] + (["-O"] if sys.flags.optimize else [])
    commands: list[Json] = []
    successful = refused = completed_orders = printed_orders = 0
    for rid in IDS:
        receipt_path = SOURCE / (rid + ".receipt.json")
        argv = python + [str(SNAPSHOT), str(receipt_path)]
        process = subprocess.run(argv, capture_output=True, text=True, timeout=20, check=False)
        require(process.returncode == 0 and process.stderr == "", "canonical_replay:" + rid)
        result = json.loads(process.stdout)
        success = rid.startswith("n8000_")
        require(result["attempt_success"] is success, "attempt_outcome:" + rid)
        require(result["audit_status"] == "valid", "canonical_status")
        require(result["raw_sha256"] == sha(SOURCE / (rid + ".raw.txt")), "canonical_raw_pin")
        commands.append({"argv": argv, "exit_code": process.returncode,
                         "stdout": result, "stderr": process.stderr})
        receipt = read(receipt_path)
        raw_rows = [json.loads(line) for line in
                    (SOURCE / (rid + ".raw.txt")).read_text().splitlines()
                    if line.startswith("{")]
        require(strengthened(raw_rows) == [], "additional_nominal_identity:" + rid)
        printed_orders += len(receipt["orders"])
        completed_orders += result["orders_complete"]
        successful += int(success)
        refused += int(not success)
    argv = python + [str(SNAPSHOT), "--selftest", str(SOURCE / (IDS[0] + ".receipt.json"))]
    process = subprocess.run(argv, capture_output=True, text=True, timeout=20, check=False)
    require(process.returncode == 0 and process.stderr == "", "selftest_replay")
    selftests = json.loads(process.stdout)
    require(len(selftests["mutants_killed"]) == 9, "selftest_nine")
    require(all(selftests[key] == 1 for key in
                ("real_positive", "synthetic_refusal", "synthetic_unemitted_read_refusal")),
            "selftest_nonvacuum")
    commands.append({"argv": argv, "exit_code": process.returncode,
                     "stdout": selftests, "stderr": process.stderr})

    receipt = read(SOURCE / (IDS[0] + ".receipt.json"))
    intent = read(SOURCE / (IDS[0] + ".intent.json"))
    protocol = read(SOURCE / "protocol.json")
    raw = (SOURCE / (IDS[0] + ".raw.txt")).read_text()
    rows = [json.loads(line) for line in raw.splitlines() if line.startswith("{")]
    tail = "\n".join(line for line in raw.splitlines() if not line.startswith("{")) + "\n"
    k2_index = next(i for i, row in enumerate(rows) if row.get("type") == "order" and row["k"] == 2)
    require(rows[k2_index]["portal_requests"] > 0 and rows[k2_index]["chain_steps"] > 0,
            "meb_counterfixture_nonvacuum")
    changes = [
        ("impossible_minima", 1, {"certificate_minima": 1}, "k1:minimum_catalogue_binding"),
        ("erased_mirrored_meb_work", k2_index,
         {"meb_calls": 0, "geometry_meb_calls": 0}, "k2:physical_meb_decomposition"),
        ("aliases_plus_one_below_cap", 1,
         {"aliases": rows[1]["aliases"] + 1}, "k1:eager_alias_decomposition"),
        ("erased_elapsed", len(rows) - 1,
         {"elapsed_before_terminal_ms": 0}, "terminal:stage_exceeds_elapsed"),
    ]
    corruptions: list[Json] = []
    for name, row_index, patch, reason in changes:
        altered = copy.deepcopy(rows)
        old_values = {key: altered[row_index][key] for key in patch}
        altered[row_index].update(patch)
        rec = copy.deepcopy(receipt)
        rec.update(terminal=altered[-1], orders=altered[1:-1])
        changed_raw = "\n".join(json.dumps(row) for row in altered) + "\n" + tail
        accepted = judge.judge(changed_raw, rec, intent, protocol)
        failures = strengthened(altered)
        require(accepted["audit_status"] == "valid" and accepted["attempt_success"] is True,
                "counterfixture_expected_acceptance:" + name)
        require(reason in failures, "counterfixture_expected_refutation:" + name)
        corruptions.append({
            "name": name, "kind": "audit_data_corruption_not_product_mutant",
            "raw_and_receipt_changed_together": True, "zero_based_raw_row": row_index,
            "old_values": old_values, "replacement_values": patch,
            "changed_raw_sha256": hashlib.sha256(changed_raw.encode()).hexdigest(),
            "captured_judge_result": accepted, "independent_failures": failures,
        })
    require((successful, refused, completed_orders, printed_orders) == (3, 2, 44, 46),
            "population_floor")
    require(len(corruptions) == 4, "corruption_floor")
    after = source_pins()
    require(after == before, "sources_changed_during_replay")
    output = {
        "schema": "mhgp7-audit-mono-judge-review-v1", "status": "passed",
        "public_status": "not_claimed", "python_optimized": bool(sys.flags.optimize),
        "engine_invoked": False, "scope": "receipt_judge_replay_and_necessary_identities",
        "judge_snapshot_sha256": JUDGE_SHA, "review_script_sha256": sha(Path(__file__)),
        "pins_as_read": before, "source_bytes_stable_during_replay": True,
        "counts": {"receipts": 5, "valid_successful_attempts": successful,
                   "valid_refused_attempts": refused, "completed_orders": completed_orders,
                   "printed_orders": printed_orders, "selftest_data_mutations_rejected": 9,
                   "additional_data_corruptions_accepted_by_captured_judge": 4,
                   "additional_data_corruptions_rejected_by_necessary_identities": 4},
        "canonical_commands": commands, "additional_corruptions": corruptions,
        "additional_identities": {
            "successful_order_rows_checked": 44, "terminal_rows_checked": 5,
            "refused_order_rows_excluded_from_success_identities": 2,
            "failures_on_nominal_receipts": 0,
            "timing_print_tolerance_ms": "0.000003",
            "not_a_forest_or_geometry_oracle": True,
        },
    }
    args.result.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"status": "passed", "counts": output["counts"]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
