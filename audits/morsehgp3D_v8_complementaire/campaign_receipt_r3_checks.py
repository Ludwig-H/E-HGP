#!/usr/bin/env python3
"""Recheck the R3 campaign repairs and a genuine inactive-sheet reader case.

Runs use tiny real probes plus explicitly artificial receipt-ingestion
failures. This does not measure performance or certify product geometry.
Historical inputs are pinned; changed inputs exit 2 without rewriting any
closed receipt. A successful invocation prints a new diagnostic on stdout.
"""

from __future__ import annotations

import argparse
import base64
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
CHECKER = ROOT / "morsehgp3D_v8/bench/check_p0_campaign.py"
PINS = {
    "morsehgp3D_v8/bench/run_p0_matrix.py":
        "aefd76e59574d3b663f0f6112e413f32d1393d50ddf95c1cace1541ec59a3715",
    "morsehgp3D_v8/bench/check_p0_campaign.py":
        "6fdfd8fe3b59f025eb8d999480f436895d4c0783c5bf67a2ff7c9874eb4da935",
}
PROBE_SHA256 = "9b19bc7eff0c5d20f06b70c792528537c663255998a22239f2d5a0d6c860cfd2"
DEFAULT_PROBE = ROOT / "build/v8_p0_r3_20260913/mhgp8_p0_probe"


class AuditInputChanged(RuntimeError):
    """The historical finding must not be silently applied to new bytes."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require_pin(path: Path, expected: str) -> None:
    if not path.is_file() or digest(path) != expected:
        raise AuditInputChanged(f"unavailable or changed audited input: {path}")


def require_sources() -> None:
    for name, expected in PINS.items():
        require_pin(ROOT / name, expected)


def python_command(script: Path, *arguments: str) -> list[str]:
    flags = ["-B", "-O"] if sys.flags.optimize else ["-B"]
    return [sys.executable, *flags, str(script), *arguments]


def invoke(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, text=True, capture_output=True,
                          cwd=ROOT, timeout=30)


def campaign(probe: Path, output: Path, extra: list[str]) -> dict[str, Any]:
    require_sources()
    command = python_command(
        RUNNER, "--probe", str(probe), "--output", str(output),
        "--sizes", "8", "--families", "grid", "--repeats", "1",
        "--kmax", "10", "--s", "8", *extra)
    completed = invoke(command)
    require_sources()
    manifest = json.loads((output / "MANIFEST.json").read_text())
    closing = json.loads((output / "COMPLETION.json").read_text())
    rows = [json.loads(line) for line in
            (output / "MEASURES.jsonl").read_text().splitlines()]
    require(manifest["receipt_validation_version"] == 2, "receipt version")
    require(closing["attempts"] == len(rows), "lost attempted row")
    require(closing["runs"] == sum(row["status"] == "completed" for row in rows),
            "incorrect success accounting")
    require(manifest["source_sha256"] == closing["source_sha256_closing"] and
            closing["source_hashes_unchanged"] is True, "source changed during audit")
    observed = []
    for row in rows:
        require(base64.b64decode(row["stdout_base64"], validate=True).decode(
            "utf-8", errors="replace") == row["stdout"], "stdout bytes lost")
        require(base64.b64decode(row["stderr_base64"], validate=True).decode(
            "utf-8", errors="replace") == row["stderr"], "stderr bytes lost")
        item = {
            "arguments": row["command"][1:], "repeat": row["repeat"],
            "status": row["status"], "exit_code": row["exit_code"],
            "error": row.get("error", ""), "stdout_bytes_preserved": True,
            "stdout_length": len(base64.b64decode(row["stdout_base64"])),
            "stderr_bytes_preserved": True,
            "probe_sha256_before": row["probe_sha256_before"],
            "probe_sha256_after": row["probe_sha256_after"],
        }
        if "result" in row:
            result = row["result"]
            item["result"] = {key: result[key] for key in (
                "n", "family", "strategy", "lane", "kmax", "separation_s",
                "threshold", "candidate_pairs", "total_pairs",
                "input_fnv1a64_le_u16_xyz")}
        observed.append(item)
    return {
        "campaign_exit_code": completed.returncode,
        "manifest_probe_sha256": manifest["probe_sha256"],
        "source_sha256": manifest["source_sha256"],
        "completion": closing, "rows": observed,
    }


def check_receipt(bundle: Path) -> dict[str, Any]:
    require_sources()
    result = invoke(python_command(CHECKER, str(bundle)))
    require_sources()
    return {
        "exit_code": result.returncode,
        "result": json.loads(result.stdout) if result.returncode == 0 else None,
        "last_stderr_line": (result.stderr.splitlines()[-1].replace(
            str(bundle), "<temporary_campaign_bundle>") if result.stderr else ""),
    }


def make_stand_in(path: Path, body: str) -> None:
    path.write_text(f"#!{sys.executable}\nfrom pathlib import Path\nimport sys\n" + body)
    path.chmod(0o755)


def run(probe: Path) -> dict[str, Any]:
    require_sources()
    require_pin(probe, PROBE_SHA256)
    cache = probe.parent / "CMakeCache.txt"
    if not cache.is_file():
        raise AuditInputChanged(f"missing cache beside pinned probe: {probe}")
    with tempfile.TemporaryDirectory(prefix="mhgp8_campaign_r3_audit_") as name:
        temporary = Path(name)
        actual = temporary / "actual_probe"
        shutil.copy2(probe, actual)
        require_pin(actual, PROBE_SHA256)
        shutil.copy2(cache, temporary / "CMakeCache.txt")
        capture = invoke([str(actual), "8", "pool", "2", "grid", "10", "8"])
        require(capture.returncode == 0 and not capture.stderr, "real capture failed")
        (temporary / "captured.json").write_text(capture.stdout)
        results: dict[str, Any] = {}

        # Nine real cases cover strategy/lane matching and ordinary readback.
        bundle = temporary / "genuine_grid"
        positive = campaign(actual, bundle / "case", [])
        positive["reader"] = check_receipt(bundle)
        require(positive["campaign_exit_code"] == 0 and
                positive["completion"]["status"] == "completed" and
                positive["completion"]["runs"] == 9 and
                positive["reader"]["exit_code"] == 0, "real grid control failed")
        require(positive["completion"]["probe_hash_unchanged"] is True,
                "positive binary closure failed")
        results["genuine_grid"] = positive

        # Same real executable: active sheets pass; inactive grids also pass.
        real_cases = (
            ("active_sheet", "sheet", "10", 0),
            ("inactive_grid", "grid", "1", 0),
            ("inactive_sheet", "sheet", "1", 1),
        )
        for label, family, kmax, reader_exit in real_cases:
            bundle = temporary / label
            result = campaign(actual, bundle / "case", [
                "--families", family, "--kmax", kmax,
                "--strategies", "dual", "--lanes", "3"])
            require(result["campaign_exit_code"] == 0 and
                    result["completion"]["status"] == "completed" and
                    result["completion"]["runs"] == 1, f"real case failed: {label}")
            row = result["rows"][0]["result"]
            require(row["threshold"] == (9 if kmax == "10" else 0),
                    f"incorrect fixture lane state: {label}")
            if kmax == "1":
                require(row["candidate_pairs"] == 0, "inactive lane has candidates")
            result["reader"] = check_receipt(bundle)
            require(result["reader"]["exit_code"] == reader_exit,
                    f"historical reader behavior changed: {label}")
            if label == "inactive_sheet":
                require(result["reader"]["last_stderr_line"] ==
                        "RuntimeError: sheet counter-fixture changed",
                        "inactive sheet rejection is not causally isolated")
            results[label] = result

        # The three stand-ins reproduce only ingestion/provenance conditions.
        replay_body = (
            "sys.stdout.buffer.write(Path(__file__).with_name('captured.json').read_bytes())\n")
        stand_ins = (
            ("stale_replay", replay_body, [], 1, 2, "command/result mismatch: strategy"),
            ("truncated_json", "sys.stdout.write('{\"schema\":')\n", [], 0, 1,
             "Expecting value:"),
            ("binary_changed_last_run", replay_body +
             "with Path(__file__).open('a') as stream: stream.write('# changed\\n')\n",
             ["--strategies", "pool", "--lanes", "2"], 0, 1,
             "probe binary changed during invocation"),
        )
        for label, body, extra, successes, attempts, error in stand_ins:
            fake = temporary / label
            make_stand_in(fake, body)
            bundle = temporary / f"receipt_{label}"
            result = campaign(fake, bundle / "case", extra)
            closing = result["completion"]
            require(result["campaign_exit_code"] == 1 and closing["status"] == "invalid"
                    and closing["runs"] == successes and closing["attempts"] == attempts,
                    f"failure not closed with exact status/counts: {label}")
            last = result["rows"][-1]
            require(last["status"] == "invalid" and last["exit_code"] == 0 and
                    last["stdout_length"] > 0 and error in last["error"],
                    f"failure evidence lost or different rejection: {label}")
            result["reader"] = check_receipt(bundle)
            require(result["reader"]["exit_code"] == 1,
                    f"reader accepted invalid campaign: {label}")
            if label == "binary_changed_last_run":
                require(last["probe_sha256_before"] != last["probe_sha256_after"] and
                        closing["probe_hash_unchanged"] is False and
                        closing["probe_sha256_closing"] == last["probe_sha256_after"],
                        "last-invocation binary change not pinned in final closure")
            else:
                require(closing["probe_hash_unchanged"] is True,
                        f"unexpected binary mutation: {label}")
            results[label] = result
        require(len(results) == 7, "seven independent cases required")
        require_pin(actual, PROBE_SHA256)
        require_pin(probe, PROBE_SHA256)
        require_sources()
        return {
            "status": "repairs_confirmed_with_inactive_sheet_reader_counterexample",
            "scope": "tiny_campaign_receipt_audit_not_product_geometry_or_performance",
            "phase": "exploration_v8_hors_registre", "backend": "cpu_reference",
            "profile": "quantized_u16_input_only",
            "mode": "audit_independant_math_and_architecture",
            "public_status": "not_claimed", "gcp_used": False,
            "git_head": invoke(["git", "rev-parse", "HEAD"]).stdout.strip(),
            "source_pins": PINS, "probe_sha256": PROBE_SHA256,
            "probe_origin": "build/v8_p0_r3_20260913/mhgp8_p0_probe",
            "source_binary_binding": "separate observed hashes; no build binding claimed",
            "audit_script_sha256": digest(Path(__file__)),
            "python_optimized": bool(sys.flags.optimize),
            "cases": 7, "results": results,
            "verdicts": {
                "initial_P1_command_result_mismatch": "closed_for_tested_runner",
                "initial_P2_truncated_stdout_closure": "closed_for_tested_runner",
                "binary_last_invocation_mutation": "detected_and_closed_invalid",
                "P2_inactive_sheet_reader": "open_false_rejection_of_valid_receipt",
            },
        }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", required=True, action="store_true")
    parser.add_argument("--probe", type=Path, default=DEFAULT_PROBE)
    args = parser.parse_args()
    try:
        result = run(args.probe.resolve())
    except AuditInputChanged as error:
        print(f"audit inputs changed; historical R3 receipt preserved: {error}",
              file=sys.stderr)
        return 2
    except (OSError, RuntimeError, subprocess.SubprocessError, ValueError,
            KeyError, TypeError) as error:
        print(f"R3 campaign receipt audit failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
