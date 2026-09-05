#!/usr/bin/env python3
"""Compose the first-C supplement with already retained frozen-v2 replays."""
from __future__ import annotations

import argparse
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from typing import Any

import replay


HERE = Path(__file__).resolve().parent
COMPANION = HERE / "companion_at_review.py"


def load_module(name: str, path: Path) -> Any:
    spec = importlib.util.spec_from_file_location(name, path)
    replay.require(spec is not None and spec.loader is not None, "import_spec")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--result", type=Path, required=True)
    args = parser.parse_args()
    mode = "optimized" if sys.flags.optimize else "normal"
    baseline_path = HERE / (mode + ".json")
    baseline = replay.read(baseline_path)
    package = replay.verify_package()
    replay.require(baseline["package"] == package and baseline["status"] == "passed", "frozen_v2_prior_replay")
    companion = load_module("first_c_at_review", COMPANION)
    replay.require(companion.JUDGE_SHA == baseline["judge_sha256"] == replay.JUDGE_SHA, "required_v2_identity")
    python = [sys.executable, "-B"] + (["-O"] if sys.flags.optimize else [])
    commands: list[dict[str, Any]] = []
    checked = 0
    for path in sorted((replay.PACKAGE / "micro").glob("k*/n8_*.receipt.json")):
        argv = python + [str(COMPANION), str(path)]
        process = subprocess.run(argv, capture_output=True, text=True, timeout=20, check=False)
        replay.require(process.returncode == 0 and process.stderr == "", "supplement_replay:" + path.name)
        result = json.loads(process.stdout)
        replay.require(result["supplement_status"] == "valid" and result["attempt_success"] is True, "supplement_success")
        replay.require(result["requires_frozen_v2_judge_sha256"] == baseline["judge_sha256"], "companion_binding")
        checked += result["successful_lazy_orders_checked"]
        commands.append({"argv": argv, "exit_code": 0, "stdout": result, "stderr": ""})
    replay.require(len(commands) == 24 and checked == 117, "companion_nonvacuum")
    for rid in ("n8_s8_k10_eager_c0", "n8_s8_k10_lazy_c1"):
        path = replay.PACKAGE / "micro/k10" / (rid + ".receipt.json")
        argv = python + [str(COMPANION), "--selftest", str(path)]
        process = subprocess.run(argv, capture_output=True, text=True, timeout=20, check=False)
        replay.require(process.returncode == 0 and process.stderr == "", "companion_selftest:" + rid)
        result = json.loads(process.stdout)
        replay.require(result["real_positive"] == 1 and result["scalar_success_models"] == 9
                       and result["scalar_refusal_prefix_models"] == 3 and len(result["mutants_killed"]) == 12,
                       "companion_selftest_floor")
        commands.append({"argv": argv, "exit_code": 0, "stdout": result, "stderr": ""})
    fixture = baseline["first_c_counterfixture"]
    path = replay.ROOT / fixture["source_receipt"]
    receipt = replay.read(path)
    raw = path.with_name(path.name.replace(".receipt.json", ".raw.txt")).read_text()
    lines = raw.splitlines()
    split = next(i for i, line in enumerate(lines) if not line.startswith("{"))
    rows = [json.loads(line) for line in lines[:split]]
    rows[fixture["zero_based_raw_row"]].update(fixture["new_values"])
    changed_receipt = copy.deepcopy(receipt)
    changed_receipt.update(orders=rows[1:-1], terminal=rows[-1])
    changed_raw = "\n".join(json.dumps(row) for row in rows) + "\n" + "\n".join(lines[split:]) + "\n"
    refused = ""
    try:
        companion.judge(changed_raw.encode(), changed_receipt)
    except ValueError as error:
        refused = str(error)
    replay.require(refused == "first_c_success", "real_corruption_refusal")
    replay.require(replay.verify_package() == package, "package_unchanged")
    result = {
        "schema": "mhgp7-first-c-companion-admission-replay-v1", "status": "passed",
        "public_status": "not_claimed", "python_optimized": bool(sys.flags.optimize),
        "authority": "frozen_v2_judge_AND_first_c_supplement",
        "supplement_replaces_v2": False, "v2_replay_sha256": replay.sha(baseline_path),
        "v2_judge_sha256": baseline["judge_sha256"], "companion_sha256": replay.sha(COMPANION),
        "script_sha256": replay.sha(Path(__file__)), "package": package,
        "counts": {"supplement_receipts_passed": 24, "successful_lazy_rows_checked": checked,
                   "selftest_fixtures": 2, "distinct_supplement_mutations_per_fixture": 12,
                   "success_scalar_models_per_fixture": 9, "refusal_prefix_models_per_fixture": 3,
                   "real_receipt_corruption_refused": 1},
        "commands": commands,
        "closed_counterfixture": {"frozen_v2_accepts": True, "supplement_refuses_with": refused,
                                  "joint_acceptance": False, "source": fixture},
        "engine_or_build_reexecuted": False, "heavy_campaign_followed": False,
    }
    args.result.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"status": "passed", "counts": result["counts"]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
