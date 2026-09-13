#!/usr/bin/env python3
"""Exercise P0 receipt ingestion with real tiny probes and artificial failures."""

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
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "morsehgp3D_v8/bench/run_p0_matrix.py"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def invoke(probe: Path, output: Path, extra: list[str]) -> tuple[Any, Any, list[Any]]:
    flags = ["-B", "-O"] if sys.flags.optimize else ["-B"]
    command = [sys.executable, *flags, str(RUNNER), "--probe", str(probe),
               "--output", str(output), "--sizes", "8", "--families", "grid",
               "--repeats", "1", "--kmax", "10", "--s", "8", *extra]
    result = subprocess.run(command, capture_output=True, text=True, cwd=ROOT)
    require((output / "COMPLETION.json").is_file(), f"unclosed campaign: {command}")
    completion = json.loads((output / "COMPLETION.json").read_text())
    measurements = output / "MEASURES.jsonl"
    rows = ([json.loads(line) for line in measurements.read_text().splitlines()]
            if measurements.is_file() else [])
    require(completion["attempts"] == len(rows), "lost attempted row")
    require(completion["runs"] == sum(row["status"] == "completed" for row in rows),
            "invalid success accounting")
    for row in rows:
        require(base64.b64decode(row["stdout_base64"]).decode("utf-8", errors="replace") ==
                row["stdout"], "raw stdout was not preserved")
        require(base64.b64decode(row["stderr_base64"]).decode("utf-8", errors="replace") ==
                row["stderr"], "raw stderr was not preserved")
    return result, completion, rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    source_probe = args.probe.resolve()
    original_hash = digest(source_probe)
    runner_hash = digest(RUNNER)
    checks = 0
    with tempfile.TemporaryDirectory(prefix="mhgp8_campaign_gate_") as temporary_name:
        temporary = Path(temporary_name)
        actual = temporary / "actual_probe"
        shutil.copy2(source_probe, actual)
        shutil.copy2(source_probe.parent / "CMakeCache.txt", temporary / "CMakeCache.txt")
        captured = subprocess.run([str(actual), "8", "pool", "2", "grid", "10", "8"],
                                  capture_output=True, check=True)
        require(not captured.stderr, "genuine fixture emitted stderr")
        (temporary / "captured.json").write_bytes(captured.stdout)
        result, completion, rows = invoke(actual, temporary / "genuine", [])
        require(result.returncode == 0 and completion["status"] == "completed" and
                len(rows) == completion["runs"] == 9, "real nine-case matrix failed")
        require(completion["source_hashes_unchanged"] is True and
                completion["probe_hash_unchanged"] is True, "positive closing pins failed")
        manifest = json.loads((temporary / "genuine/MANIFEST.json").read_text())
        require(manifest["receipt_validation_version"] == 2 and
                manifest["source_sha256"] == completion["source_sha256_closing"] and
                manifest["probe_sha256"] == completion["probe_sha256_closing"],
                "closing pins not recorded")
        require(all(row["probe_sha256_before"] == row["probe_sha256_after"] == original_hash
                    for row in rows), "per-invocation binary pins missing")
        checks += 1

        # A compiler selected with -DCMAKE_CXX_COMPILER=clang++ is commonly
        # recorded as UNINITIALIZED, not FILEPATH. Metadata ingestion must
        # not be tied to the cache type chosen by CMake's invocation.
        cache_path = temporary / "CMakeCache.txt"
        cache = cache_path.read_text()
        alternate = "\n".join(
            "CMAKE_CXX_COMPILER:UNINITIALIZED=" + line.split("=", 1)[1]
            if line.startswith("CMAKE_CXX_COMPILER:") else line
            for line in cache.splitlines()) + "\n"
        cache_path.write_text(alternate)
        result, completion, rows = invoke(actual, temporary / "alternate_cache_type",
                                           ["--strategies", "pool", "--lanes", "2"])
        require(result.returncode == 0 and completion["status"] == "completed" and
                len(rows) == 1, "non-FILEPATH compiler cache was rejected")
        cache_path.write_text(cache)
        checks += 1

        prelude = (f"#!{sys.executable}\n"
                   "import json, os, signal, subprocess, sys\n"
                   "from pathlib import Path\n"
                   f"real = {str(actual)!r}\n")
        passthrough = (
            "result = subprocess.run([real, *sys.argv[1:]], capture_output=True, check=True)\n"
            "row = json.loads(result.stdout)\n")
        mutants = {
            "wrong_n": "row['n'] += 1\n",
            "wrong_strategy": "row['strategy'] = 'dual'\n",
            "wrong_lane": "row['lane'] = 3\n",
            "wrong_family": "row['family'] = 'sheet'\n",
            "wrong_kmax": "row['kmax'] = 5\n",
            "wrong_s": "row['separation_s'] = 12\n",
            "wrong_schema": "row['schema'] = 'another_schema'\n",
            "wrong_status": "row['status'] = 'failed'\n",
            "wrong_scope": "row['scope'] = 'FULL'\n",
            "false_claim": "row['public_status'] = 'exact'\n",
            "false_downstream": "row['downstream_measured'] = True\n",
            "boolean_integer": "row['threads'] = True\n",
            "wrong_core": "row['core_credit'] = 1\n",
            "wrong_threshold": "row['threshold'] += 1\n",
            "wrong_factor": "row['n_a'] += 1\n",
            "wrong_count": "row['candidate_pairs'] += 1\n",
            "wrong_fraction": "row['rejected_fraction'] = 0.125\n",
            "wrong_descriptors": "row['candidate_descriptors'] = 10000\n",
            "negative_time": "row['plan_ms'] = -1\n",
            "wrong_time_sum": "row['total_component_ms'] += 1\n",
            "nonfinite_time": "row['plan_ms'] = float('nan')\n",
            "overflow_time": "row['plan_ms'] = float('inf')\n",
            "negative_work": "row['plan_work']['pool_selection_tests'] = -1\n",
            "overflow_work": "row['plan_work']['pool_selection_tests'] = 2**64\n",
            "wrong_validation": "row['preparation_work']['validation_points'] -= 1\n",
            "wrong_strategy_work": "row['plan_work']['tube_records'] = 1\n",
            "wrong_identity": "row['input_fnv1a64_le_u16_xyz'] = 'not_hex'\n",
        }
        for name, mutation in mutants.items():
            fake = temporary / name
            fake.write_text(prelude + passthrough + mutation + "print(json.dumps(row))\n")
            fake.chmod(0o755)
            result, completion, rows = invoke(
                fake, temporary / f"receipt_{name}", ["--strategies", "pool", "--lanes", "2"])
            require(result.returncode == 1 and completion["status"] == "invalid" and
                    completion["runs"] == 0 and len(rows) == 1 and
                    rows[0]["status"] == "invalid" and bool(rows[0]["stdout"]),
                    f"mutant accepted or evidence lost: {name}: {result.stderr}")
            checks += 1

        failures = {
            "replay": (
                "sys.stdout.buffer.write(Path(__file__).with_name('captured.json').read_bytes())\n",
                [], "invalid", 1, 2),
            "malformed": ("sys.stdout.write('{\"schema\":')\n",
                          [], "invalid", 0, 1),
            "bad_utf8": ("sys.stdout.buffer.write(b'\\xff\\x80')\n",
                         [], "invalid", 0, 1),
            "duplicate_json_key": (
                "data = Path(__file__).with_name('captured.json').read_text().strip()\n"
                "sys.stdout.write(data[:-1] + ',\"n\":8}')\n",
                [], "invalid", 0, 1),
            "overflow_json_exponent": (
                "data = Path(__file__).with_name('captured.json').read_text().strip()\n"
                "sys.stdout.write(data[:-1] + ',\"overflow\":1e999}')\n",
                [], "invalid", 0, 1),
            "process_failure": (
                "print('partial stdout'); print('failure marker', file=sys.stderr); sys.exit(3)\n",
                [], "failed", 0, 1),
            "binary_changes_last_run": (
                passthrough + "print(json.dumps(row))\n"
                "with Path(__file__).open('a') as stream: stream.write('# changed\\n')\n",
                ["--strategies", "pool", "--lanes", "2"], "invalid", 0, 1),
            "changed_point_identities": (
                passthrough + "if sys.argv[2] == 'dual': row['input_fnv1a64_le_u16_xyz'] = '1'\n"
                "print(json.dumps(row))\n", [], "invalid", 1, 2),
            "changed_repeat_work": (
                passthrough + "state = Path(__file__).with_suffix('.state')\n"
                "if state.exists(): row['preparation_work']['uniqueness_comparisons'] += 1\n"
                "state.write_text('visited')\nprint(json.dumps(row))\n",
                ["--strategies", "pool", "--lanes", "2", "--repeats", "2"],
                "invalid", 1, 2),
            "interrupted": (
                "print('interrupted stdout', flush=True)\n"
                "print('interrupted stderr', file=sys.stderr, flush=True)\n"
                "os.kill(os.getppid(), signal.SIGINT)\n",
                [], "interrupted", 0, 1),
            "interrupted_ignores_term": (
                "signal.signal(signal.SIGTERM, signal.SIG_IGN)\n"
                "print('interrupted stdout', flush=True)\n"
                "os.kill(os.getppid(), signal.SIGINT)\n"
                "while True: signal.pause()\n",
                [], "interrupted", 0, 1),
            "terminated": (
                "print('terminated stdout', flush=True)\n"
                "os.kill(os.getppid(), signal.SIGTERM)\n",
                [], "interrupted", 0, 1),
        }
        for name, (body, extra, status, successes, attempts) in failures.items():
            fake = temporary / name
            fake.write_text(prelude + body)
            fake.chmod(0o755)
            result, completion, rows = invoke(fake, temporary / f"receipt_{name}", extra)
            expected_exit = (143 if name == "terminated" else
                             130 if status == "interrupted" else 1)
            require(result.returncode == expected_exit and completion["status"] == status and
                    completion["runs"] == successes and len(rows) == attempts,
                    f"wrong failure closure: {name}: {completion}: {result.stderr}")
            require(rows[-1]["status"] == status and bool(rows[-1]["stdout_base64"]),
                    f"failed attempt stdout lost: {name}")
            if name == "binary_changes_last_run":
                require(completion["probe_hash_unchanged"] is False and
                        rows[-1]["probe_sha256_before"] != rows[-1]["probe_sha256_after"],
                        "last-invocation mutation was not detected")
            if name in ("process_failure", "interrupted"):
                require(bool(rows[-1]["stderr_base64"]), f"stderr lost: {name}")
            if name == "interrupted_ignores_term":
                require(rows[-1]["exit_code"] == -9, "cancelled process was not killed")
            checks += 1
        require(checks == 41, "non-vacuity floor")
        require(digest(actual) == digest(source_probe) == original_hash and
                digest(RUNNER) == runner_hash, "gate changed its genuine inputs")
    print(json.dumps({"status": "passed", "checks": checks,
                      "python_optimized": bool(sys.flags.optimize),
                      "scope": "p0_campaign_receipt_ingestion_not_geometry"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
