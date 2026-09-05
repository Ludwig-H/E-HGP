#!/usr/bin/env python3
"""Replay the unchanged D rational judge against E, with separate artifacts."""

from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import sys


sys.dont_write_bytecode = True
AUDIT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "meb_d_judge", AUDIT / "meb_rational_oracle_20260905.py")
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("cannot_load_pinned_D_judge")
JUDGE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(JUDGE)
JUDGE.WORK = AUDIT / ".work_20260905_meb_e_q2"
JUDGE.RECEIPT = AUDIT / "receipts_20260905/e_q2"
JUDGE.RECEIPT.mkdir(exist_ok=True)


def main() -> int:
    baseline_path = AUDIT / "receipts_20260905/meb_rational.json"
    baseline = json.loads(baseline_path.read_text())
    JUDGE.check(JUDGE.main() == 0, "E.independent_judge_failed")
    target = JUDGE.RECEIPT / ("meb_rational_optimized.json" if not __debug__ else "meb_rational.json")
    replay = json.loads(target.read_text())
    for key in ("counts", "corpus_sha256", "meb_output_sha256", "powers_output_sha256"):
        JUDGE.check(replay[key] == baseline[key], "D_E.divergence." + key)
    q2_case = JUDGE.corpus()[0]
    command = "M 2 550 " + JUDGE.points_text(q2_case)
    binary = JUDGE.WORK / "meb_oracle_bridge"
    nominal = JUDGE.execute(binary, [command])[0]
    mutant = JUDGE.execute(binary, [command], "silent-meb-q2-reject-shell")[0]
    JUDGE.check(nominal["ok"] and nominal["q"] == 2 and nominal["status"] == 0,
                "q2_mutant.nonvacuous_nominal")
    JUDGE.check(not mutant["ok"] and mutant["status"] == 4 and
                mutant["reason"] == "silent_no_local_miniball", "q2_mutant.not_killed")
    signature = {"status": "passed", "public_status": "not_claimed", "scope": "E_q2_local_only",
                 "D_full323_does_not_qualify_E324": True,
                 "same_D_E_fields": ["counts", "corpus_sha256", "meb_output_sha256", "powers_output_sha256"],
                 "D_receipt_sha256": hashlib.sha256(baseline_path.read_bytes()).hexdigest(),
                 "E_receipt_sha256": hashlib.sha256(target.read_bytes()).hexdigest(),
                 "driver_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                 "mutant": "silent-meb-q2-reject-shell", "nominal": nominal,
                 "mutated": mutant, "gcp": "not_used"}
    (JUDGE.RECEIPT / "q2_addendum.json").write_text(json.dumps(signature, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"E_q2_audit": "passed", "D_E_local_objects": "identical", "q2_mutant": "killed"}))
    return 0


if __name__ == "__main__":
    sys.exit(main())
