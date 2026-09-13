#!/usr/bin/env python3
"""Reproduce pinned campaign-ingestion gaps with explicit artificial stand-ins.

This is an audit of result ingestion, not a product benchmark. A fresh run
prints a new diagnostic and never overwrites CAMPAIGN_RECEIPT_CHECKS.json.
A changed runner or unavailable pinned probe requires a new audit (exit 2).
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "morsehgp3D_v8/bench/run_p0_matrix.py"
RUNNER_SHA256 = "d2f2514b284d7cfe00ba8cd009f3d2a46782047db3dbd5f8a8eb9243159a0c09"
PROBE_SHA256 = "fbbd027269c88a20bd24ae8edf15f705b9a1fac3f094b377a752918eef54d0a0"
DEFAULT_PROBE = ROOT / "build/v8_p0_20260913/mhgp8_p0_probe"


class AuditInputChanged(RuntimeError):
    """The historical finding must not silently be applied to different bytes."""


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def require_pin(path: Path, expected: str) -> None:
    if not path.is_file() or digest(path) != expected:
        raise AuditInputChanged(f"unavailable or changed audited input: {path}")


def invoke(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, text=True, capture_output=True,
                          cwd=ROOT, timeout=30)


def configuration_error(record: dict[str, Any]) -> dict[str, Any] | None:
    """Independent gate: mismatched command/result configurations are rejected."""
    command = record["command"]
    expected = [int(command[1]), command[2], int(command[3]), command[4],
                int(command[5]), int(command[6])]
    row = record["result"]
    observed = [row.get(key) for key in
                ("n", "strategy", "lane", "family", "kmax", "separation_s")]
    if expected == observed:
        return None
    return {"expected": expected, "observed": observed}


def measure_case(name: str, probe: Path, directory: Path) -> dict[str, Any]:
    require_pin(RUNNER, RUNNER_SHA256)
    output = directory / name
    python_flags = ["-B", "-O"] if sys.flags.optimize else ["-B"]
    command = [sys.executable, *python_flags, str(RUNNER), "--probe", str(probe),
               "--output", str(output), "--sizes", "8", "--families", "grid",
               "--repeats", "1", "--kmax", "10", "--s", "8"]
    completed = invoke(command)
    require_pin(RUNNER, RUNNER_SHA256)
    manifest = json.loads((output / "MANIFEST.json").read_text())
    require(manifest["source_sha256"][str(RUNNER.relative_to(ROOT))] ==
            RUNNER_SHA256, "campaign manifest did not capture the pinned runner")
    records = [json.loads(line) for line in
               (output / "MEASURES.jsonl").read_text().splitlines()]
    rejected = []
    for index, record in enumerate(records):
        error = configuration_error(record)
        if error is not None:
            rejected.append({"row": index, **error})
    completion_path = output / "COMPLETION.json"
    completion = (json.loads(completion_path.read_text())
                  if completion_path.exists() else None)
    return {
        "case": name, "campaign_exit_code": completed.returncode,
        "recorded_rows": len(records), "audit_rejected_rows": rejected,
        "completion": completion,
        "last_stderr_line": (completed.stderr.splitlines()[-1]
                             if completed.stderr else ""),
    }


def run(probe: Path) -> dict[str, Any]:
    require_pin(RUNNER, RUNNER_SHA256)
    require_pin(probe, PROBE_SHA256)
    cache = probe.parent / "CMakeCache.txt"
    if not cache.is_file():
        raise AuditInputChanged(f"CMakeCache.txt unavailable beside probe: {probe}")
    with tempfile.TemporaryDirectory(prefix="mhgp8_campaign_receipt_audit_") as name:
        temporary = Path(name)
        actual = temporary / "actual_probe"
        shutil.copy2(probe, actual)
        require_pin(actual, PROBE_SHA256)
        shutil.copy2(cache, temporary / "CMakeCache.txt")
        captured = invoke([str(actual), "8", "pool", "2", "grid", "10", "8"])
        require(captured.returncode == 0 and not captured.stderr,
                "real probe capture failed")
        row = json.loads(captured.stdout)
        require(row["schema"] == "mhgp8_p0_probe_v1" and
                row["status"] == "completed", "real probe capture scope changed")
        (temporary / "captured_row.json").write_text(captured.stdout)
        # These scripts simulate stale cached output and truncated stdout only.
        replay = temporary / "replay_probe"
        replay.write_text(
            "#!/usr/bin/env python3\nfrom pathlib import Path\n"
            "print(Path(__file__).with_name('captured_row.json').read_text(), "
            "end='')\n")
        replay.chmod(0o755)
        truncated = temporary / "truncated_probe"
        truncated.write_text(
            "#!/usr/bin/env python3\nprint('{\"schema\":', end='')\n")
        truncated.chmod(0o755)
        results = [measure_case(case, target, temporary) for case, target in
                   (("genuine", actual), ("stale_replay", replay),
                    ("truncated_json", truncated))]
        positive, stale, malformed = results
        for result in (positive, stale):
            require(result["campaign_exit_code"] == 0 and
                    result["recorded_rows"] == 9 and
                    result["completion"]["status"] == "completed" and
                    result["completion"]["source_hashes_unchanged"] is True,
                    f"historical campaign behavior changed: {result['case']}")
        require(not positive["audit_rejected_rows"], "positive control rejected")
        require(len(stale["audit_rejected_rows"]) == 8,
                "audit gate did not reject eight contradictory rows")
        require(malformed["campaign_exit_code"] == 1 and
                malformed["recorded_rows"] == 0 and
                malformed["completion"] is None and
                malformed["last_stderr_line"].startswith("json.decoder.JSONDecodeError:"),
                "truncated-output diagnostic gap not reproduced")
        require_pin(actual, PROBE_SHA256)
        return {
            "status": "diagnostic_receipt_gaps_confirmed",
            "scope": "artificial_receipt_ingestion_tests_not_a_product_measurement",
            "public_status": "not_claimed", "gcp_used": False,
            "runner_sha256": RUNNER_SHA256, "genuine_probe_sha256": PROBE_SHA256,
            "audit_script_sha256": digest(Path(__file__)),
            "python_optimized": bool(sys.flags.optimize),
            "configuration_fields": ["n", "strategy", "lane", "family", "kmax",
                                     "separation_s"],
            "results": results,
        }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", required=True, action="store_true")
    parser.add_argument("--probe", type=Path, default=DEFAULT_PROBE)
    args = parser.parse_args()
    try:
        result = run(args.probe.resolve())
    except AuditInputChanged as error:
        print(f"audit input changed; historical receipt preserved: {error}",
              file=sys.stderr)
        return 2
    except (OSError, RuntimeError, subprocess.SubprocessError, ValueError,
            KeyError, TypeError) as error:
        print(f"campaign receipt audit failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
