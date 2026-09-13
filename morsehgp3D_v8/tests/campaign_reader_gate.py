#!/usr/bin/env python3
"""Check sheet receipt invariants on active and inactive geometric lanes."""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "morsehgp3D_v8/bench/run_p0_matrix.py"
READER = ROOT / "morsehgp3D_v8/bench/check_p0_campaign.py"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    probe = args.probe.resolve()
    original_hash = hashlib.sha256(probe.read_bytes()).hexdigest()
    flags = ["-B", "-O"] if sys.flags.optimize else ["-B"]
    checks = 0

    with tempfile.TemporaryDirectory(prefix="mhgp8_campaign_reader_gate_") as name:
        temporary = Path(name)
        positive = temporary / "positive"
        campaign = positive / "sheet"
        command = [sys.executable, *flags, str(RUNNER), "--probe", str(probe),
                   "--output", str(campaign), "--sizes", "16", "--families", "sheet",
                   "--repeats", "1", "--kmax", "1", "--s", "8"]
        capture = subprocess.run(command, capture_output=True, text=True, cwd=ROOT)
        require(capture.returncode == 0, f"genuine sheet matrix failed: {capture.stderr}")
        completion = json.loads((campaign / "COMPLETION.json").read_text())
        require(completion["status"] == "completed" and completion["runs"] == 9 and
                completion["attempts"] == 9, "genuine sheet matrix is incomplete")
        path = campaign / "MEASURES.jsonl"
        records = [json.loads(line) for line in path.read_text().splitlines()]
        for record in records:
            row = record["result"]
            require(row["kmax"] == 1 and row["family"] == "sheet", "wrong control fixture")
            require(row["candidate_pairs"] == (row["total_pairs"] if row["lane"] == 2 else 0),
                    "genuine active/inactive sheet result changed")
        require(sum(record["result"]["lane"] == 3 and
                    record["result"]["threshold"] == 0 for record in records) == 3,
                "missing q3/Kmax=1 inactive controls")
        checked = subprocess.run([sys.executable, *flags, str(READER), str(positive)],
                                 capture_output=True, text=True, cwd=ROOT)
        require(checked.returncode == 0 and not checked.stderr,
                f"reader rejected valid inactive sheet lanes: {checked.stderr}")
        verdict = json.loads(checked.stdout)
        require(verdict["status"] == "passed" and verdict["measurements"] == 9 and
                verdict["full_contract_qualified"] is False, "wrong positive reader verdict")
        checks += 1

        for strategy in ("pool", "dual", "tubes"):
            # Mutate only a disposable copy. Keep command, tuple, raw JSON and
            # cardinal accounting coherent so the sheet invariant is the
            # rejecting condition, not an unrelated malformed receipt.
            mutant = temporary / f"active_drop_{strategy}"
            shutil.copytree(positive, mutant)
            mutant_path = mutant / "sheet/MEASURES.jsonl"
            changed = [json.loads(line) for line in mutant_path.read_text().splitlines()]
            touched = 0
            for record in changed:
                row = record["result"]
                if row["lane"] != 2 or row["strategy"] != strategy:
                    continue
                require(row["threshold"] == 1 and row["candidate_descriptors"] == 1,
                        "mutant control is not an active one-block sheet")
                row["candidate_pairs"] -= 1
                row["rejected_pairs"] += 1
                row["rejected_fraction"] = row["rejected_pairs"] / row["total_pairs"]
                raw = json.dumps(row, separators=(",", ":")) + "\n"
                record["stdout"] = raw
                record["stdout_base64"] = base64.b64encode(raw.encode()).decode("ascii")
                touched += 1
            require(touched == 1, "active sheet mutant was vacuous or ambiguous")
            mutant_path.write_text("".join(json.dumps(row) + "\n" for row in changed))
            checked = subprocess.run([sys.executable, *flags, str(READER), str(mutant)],
                                     capture_output=True, text=True, cwd=ROOT)
            require(checked.returncode == 1 and not checked.stdout and
                    "sheet counter-fixture changed" in checked.stderr,
                    f"active sheet pair deletion was not specifically rejected: {strategy}")
            checks += 1

    require(checks == 4 and hashlib.sha256(probe.read_bytes()).hexdigest() == original_hash,
            "reader gate non-vacuity or binary preservation failed")
    print(json.dumps({"status": "passed", "checks": checks, "genuine_measurements": 9,
                      "active_sheet_mutants": 3, "inactive_q3_controls": 3,
                      "python_optimized": bool(sys.flags.optimize),
                      "scope": "sheet_receipt_active_lane_invariant_not_full_hgp"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
