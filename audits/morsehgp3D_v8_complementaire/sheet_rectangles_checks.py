#!/usr/bin/env python3
"""Bounded audit prototype for certified row-major sheet subrectangles."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
BASE = "3589a2c9"
SOURCES = ("src/core/types.hpp", "src/spindle/predicates.hpp")


def checked(argv: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(argv, cwd=ROOT, capture_output=True, text=True,
                          timeout=60, check=True)


def run() -> dict[str, object]:
    here = Path(__file__).resolve().parent
    manifest = {}
    with tempfile.TemporaryDirectory(prefix="mhgp8_sheet_rectangles_") as name:
        directory = Path(name)
        for relative in SOURCES:
            path = f"morsehgp3D_v8/{relative}"
            data = checked(["git", "show", f"{BASE}:{path}"]).stdout.encode()
            manifest[path] = hashlib.sha256(data).hexdigest()
            target = directory / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        data = (here / "sheet_rectangles_probe.cpp").read_bytes()
        probe = directory / "probe.cpp"
        probe.write_bytes(data)
        manifest[str((here / "sheet_rectangles_probe.cpp").relative_to(ROOT))] = (
            hashlib.sha256(data).hexdigest())
        compiler = shutil.which("g++")
        if compiler is None:
            raise RuntimeError("g++ unavailable")
        executable = directory / "probe"
        command = [compiler, "-std=c++20", "-O2", "-Wall", "-Wextra",
                   "-Wpedantic", "-Werror", "-fsanitize=undefined",
                   "-fno-sanitize-recover=all", f"-I{directory / 'src'}",
                   str(probe), "-o", str(executable)]
        checked(command)
        positive = checked([str(executable)])
        rows = [json.loads(line) for line in positive.stdout.splitlines()]
        if len(rows) != 42 or positive.stderr:
            raise RuntimeError("positive nonvacuity or sanitizer diagnostic")
        # Remove geometric certification from this audit proposal only.
        # Permuted sheet coordinates must then refute a proposed rejection.
        before = b"decision == mhgp8::BlockDecision::Credit"
        after = b"(decision == mhgp8::BlockDecision::Credit || true)"
        if data.count(before) != 1:
            raise RuntimeError("mutation target changed")
        probe.write_bytes(data.replace(before, after))
        checked(command)
        mutant = subprocess.run([str(executable)], capture_output=True, text=True,
                                timeout=60)
        if mutant.returncode != 1 or "unsafe geometric rejection" not in mutant.stderr:
            raise RuntimeError(f"uncertified rank mutant not rejected: {mutant.stderr}")
        return {"status": "passed", "scope": "q2_sheet_subrectangles_audit_prototype",
                "not_production": True, "not_full_tower": True, "gcp_used": False,
                "base_commit": checked(["git", "rev-parse", BASE]).stdout.strip(),
                "source_sha256": manifest,
                "runner_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                "compiler": checked([compiler, "--version"]).stdout.splitlines()[0],
                "compile_command": command, "positive_exit_code": positive.returncode,
                "cases": rows, "mutant": {"name": "rank_without_geometry",
                "exit_code": mutant.returncode, "diagnostic": mutant.stderr.strip()}}


def main() -> int:
    if sys.argv[1:] != ["--selftest"]:
        print("usage: sheet_rectangles_checks.py --selftest", file=sys.stderr)
        return 2
    try:
        result = run()
    except (OSError, RuntimeError, subprocess.SubprocessError, ValueError) as error:
        print(f"sheet rectangle checks failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
