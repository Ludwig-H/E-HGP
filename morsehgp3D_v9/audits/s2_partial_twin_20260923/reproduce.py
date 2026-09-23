#!/usr/bin/env python3
"""Reproduce the accepted, unpaired GPU LiDAR tower in commit 1f5dede11.

Only the existing tower protocol selftest's fake probe is changed in memory:
its sleep rule gains a batch-path selector. No GCP command or real GPU runs.
"""

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path
from unittest.mock import patch


COMMIT = "1f5dede11d841e2d726f643ea2b8fbe55fbba9aa"
NEEDLE = (
    "if rule['workers'] == workers and rule.get('k', k) == k "
    "and rule.get('scene', scene) == scene:"
)
REPLACEMENT = (
    "if rule['workers'] == workers and rule.get('k', k) == k "
    "and rule.get('scene', scene) == scene "
    "and rule.get('batch', levers['q34_batch_filter']) == levers['q34_batch_filter']:"
)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path, help="checkout at the pinned commit")
    args = parser.parse_args()
    source = args.source.resolve(strict=True)
    head = subprocess.run(
        ["git", "-C", str(source), "rev-parse", "HEAD"],
        check=True,
        text=True,
        capture_output=True,
    ).stdout.strip()
    if head != COMMIT:
        raise SystemExit(f"source HEAD {head} differs from {COMMIT}")
    sys.path.insert(0, str(source / "gcp-migration"))
    import tower_selftest_v9 as test  # pylint: disable=import-outside-toplevel

    selftest_blob = subprocess.run(
        ["git", "-C", str(source), "show", "HEAD:gcp-migration/tower_selftest_v9.py"],
        check=True,
        capture_output=True,
    ).stdout
    if (
        Path(test.__file__).read_bytes() != selftest_blob
        or test.FAKE_PROBE.count(NEEDLE) != 1
        or not test.protocol_committed()
    ):
        raise SystemExit("pinned fake-probe site or committed protocol differs")

    # Default plan: case 0 is GPU 00/K5/W48, case 1 its engine twin.
    # Sleep only case 1 until the useful budget kills it; no later case starts.
    with patch.object(test, "FAKE_PROBE", test.FAKE_PROBE.replace(NEEDLE, REPLACEMENT)):
        with tempfile.TemporaryDirectory(prefix="mhgp9-s2-partial-twin-") as directory:
            code, host_receipt, _, host = test.run_scenario(
                Path(directory),
                tools={"sleep": [{"workers": 48, "k": 5, "scene": "00", "batch": False, "seconds": 60}]},
                patches=[
                    (test.worker, "USEFUL_BUDGET_SECONDS", 15),
                    (test.worker, "MIN_CASE_START_SECONDS", 1),
                ],
            )
            worker_receipt = test.worker.strict_json(
                (host / "received/output/receipt.json").read_bytes()
            )
            observed = {
                "source_commit": head,
                "fake_g4_only": True,
                "host_exit_code": code,
                "host_status": host_receipt["status"],
                "worker_status": worker_receipt["status"],
                "backend": host_receipt["backend"],
                "GPU_executed": host_receipt["GPU_executed"],
                "FULL_executed": host_receipt["FULL_executed"],
                "completed_case_indices": worker_receipt["completed_case_indices"],
                "cross_worker_comparisons": worker_receipt["cross_worker_comparisons"],
                "case_outcomes": [item["outcome"] for item in worker_receipt["case_outcomes"]],
                "targeted_shutdown_certified": host_receipt["targeted_shutdown_certified"],
            }
            expected = {
                "source_commit": COMMIT,
                "fake_g4_only": True,
                "host_exit_code": 0,
                "host_status": "partial",
                "worker_status": "partial",
                "backend": "cuda_g4",
                "GPU_executed": True,
                "FULL_executed": True,
                "completed_case_indices": [0],
                "cross_worker_comparisons": [],
                "case_outcomes": ["complete_relative", "killed_budget"] + ["skipped_budget"] * 12,
                "targeted_shutdown_certified": True,
            }
            if observed != expected:
                raise SystemExit("unexpected protocol behavior:\n" + json.dumps(observed, indent=2))
            print(json.dumps(observed, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
