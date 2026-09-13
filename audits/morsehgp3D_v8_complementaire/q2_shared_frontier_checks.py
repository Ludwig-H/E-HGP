#!/usr/bin/env python3
"""Audit a shared query frontier on a fixed published v8 snapshot."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BASE = "f5430f57"
SOURCES = (
    "src/core/types.hpp", "src/spindle/predicates.hpp",
    "src/pipeline/local_credits.hpp", "src/pipeline/local_credits.cpp",
    "src/pipeline/tube_credits.hpp", "src/pipeline/axis_q2.hpp",
    "src/pipeline/axis_q2.cpp", "bench/p0_fixtures.hpp", "bench/sheet_full_fixture.hpp",
)


def run_command(command: list[str], timeout: int = 60) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=timeout)
    if result.returncode:
        raise RuntimeError(f"command exited {result.returncode}: {command}: {result.stderr}")
    return result


def run(large: bool) -> dict:
    here = Path(__file__).resolve().parent
    source = (here / "q2_shared_frontier_probe.cpp").read_bytes()
    hashes = {"probe.cpp": hashlib.sha256(source).hexdigest()}
    with tempfile.TemporaryDirectory(prefix="mhgp8_shared_frontier_") as name:
        folder = Path(name)
        for relative in SOURCES:
            data = subprocess.check_output(["git", "show", f"{BASE}:morsehgp3D_v8/{relative}"], cwd=ROOT)
            target = folder / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            hashes[relative] = hashlib.sha256(data).hexdigest()
        probe = folder / "probe.cpp"
        probe.write_bytes(source)
        compiler = shutil.which("g++")
        if not compiler:
            raise RuntimeError("g++ required")
        common = [compiler, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                  f"-I{folder / 'src'}", f"-I{folder / 'bench'}", str(probe),
                  str(folder / "src/pipeline/local_credits.cpp"), str(folder / "src/pipeline/axis_q2.cpp")]
        plain, sanitized = folder / "plain", folder / "sanitized"
        plain_compile = common + ["-o", str(plain)]
        sanitize_compile = common + ["-fsanitize=undefined", "-fno-sanitize-recover=all", "-o", str(sanitized)]
        run_command(plain_compile)
        run_command(sanitize_compile)
        normal = run_command([str(plain), "--selftest"])
        sanitizer = run_command([str(sanitized), "--selftest"])
        rows = [json.loads(line) for line in normal.stdout.splitlines()]
        if normal.stdout != sanitizer.stdout or normal.stderr or sanitizer.stderr or len(rows) != 57:
            raise RuntimeError("selftest semantics/sanitizer/nonvacuity mismatch")
        if not any(row["shared_inside_updates"] and row["query_splits"] for row in rows):
            raise RuntimeError("vacuous shared credit/split exercise")
        mutations = (
            ("restart_root_on_child", b"group(a,b.left,head,count,depth+1);", b"group(a,b.left,push(0,nil),count,depth+1);"),
            ("forget_acquired_count", b"group(a,b.left,head,count,depth+1);", b"group(a,b.left,head,0,depth+1);"),
            ("drop_pending_tail", b"const auto right=push(z.right,current.next);", b"const auto right=push(z.right,nil);"),
        )
        mutants = []
        for label, before, after in mutations:
            if source.count(before) != 1:
                raise RuntimeError(f"mutation anchor changed: {label}")
            probe.write_bytes(source.replace(before, after))
            run_command(sanitize_compile)
            result = subprocess.run([str(sanitized), "--selftest"], capture_output=True, text=True, timeout=60)
            if result.returncode != 1 or "shared result differs from individual census" not in result.stderr:
                raise RuntimeError(f"mutation not refuted: {label}: {result.returncode}: {result.stderr}")
            mutants.append({"name": label, "exit_code": result.returncode, "diagnostic": result.stderr.strip()})
        probe.write_bytes(source)
        large_runs = []
        if large:
            for n in (8000, 16000, 32000):
                for family in ("grid", "sheet_full"):
                    command = [str(plain), "--large", str(n), family]
                    try:
                        result = run_command(command)
                        row = json.loads(result.stdout)
                        large_runs.append({"status": "completed", "command": command, "result": row})
                    except subprocess.TimeoutExpired as error:
                        raw = error.stdout or b""
                        stdout = raw.decode(errors="replace") if isinstance(raw, bytes) else raw
                        large_runs.append({"status": "interrupted_at_60_seconds", "command": command, "stdout": stdout})
                        break
        return {
            "status": "passed", "base_commit": run_command(["git", "rev-parse", BASE]).stdout.strip(),
            "scope": "audit_shared_frontier_strict_count_no_ids_shell_or_canonical_output",
            "public_status": "not_claimed", "gcp_used": False,
            "source_sha256": hashes, "runner_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            "compiler": run_command([compiler, "--version"]).stdout.splitlines()[0],
            "compile_commands": [plain_compile, sanitize_compile],
            "binary_sha256": hashlib.sha256(plain.read_bytes()).hexdigest(),
            "small": {"cases": len(rows), "normal_UBSan_identical": True, "results": rows},
            "mutants": mutants, "large": large_runs,
            "large_checks": "histograms plus two modulo-u64 digests; no exhaustive geometric oracle",
            "small_checks": "every represented pair plus independent all-pairs point census, coverage and no duplicates",
            "timings_measured": False,
        }


def main() -> int:
    if sys.argv[1:] not in (["--selftest"], ["--selftest", "--large"]):
        print("usage: q2_shared_frontier_checks.py --selftest [--large]", file=sys.stderr)
        return 2
    try:
        result = run("--large" in sys.argv)
    except (OSError, RuntimeError, subprocess.SubprocessError, ValueError) as error:
        print(f"shared frontier checks failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
