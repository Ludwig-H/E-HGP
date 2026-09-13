#!/usr/bin/env python3
"""Qualify the bounded q2 residual consumer on a fresh v8 source snapshot."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from local_credits_checks import ROOT, SOURCES, checked


def run() -> dict[str, object]:
    here = Path(__file__).resolve().parent
    compiler = shutil.which("g++")
    if compiler is None:
        raise RuntimeError("g++ unavailable")
    manifest = {}
    with tempfile.TemporaryDirectory(prefix="mhgp8_transverse_audit_") as name:
        directory = Path(name)
        for relative in SOURCES:
            source = ROOT / "morsehgp3D_v8" / relative
            data = source.read_bytes()
            manifest[str(source.relative_to(ROOT))] = hashlib.sha256(data).hexdigest()
            target = directory / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        probe = directory / "probe.cpp"
        data = (here / "transverse_residual_probe.cpp").read_bytes()
        manifest[str((here / "transverse_residual_probe.cpp").relative_to(ROOT))] = (
            hashlib.sha256(data).hexdigest())
        probe.write_bytes(data)
        executable = directory / "probe"
        command = [compiler, "-std=c++20", "-O2", "-Wall", "-Wextra",
                   "-Wpedantic", "-Werror", "-fsanitize=undefined",
                   "-fno-sanitize-recover=all", f"-I{directory / 'src'}",
                   str(probe), str(directory / "src/pipeline/local_credits.cpp"),
                   "-o", str(executable)]
        checked(command)
        positive = checked([str(executable)])
        rows = [json.loads(line) for line in positive.stdout.splitlines()]
        if len(rows) != 24 or positive.stderr:
            raise RuntimeError("positive nonvacuity or sanitizer diagnostic")
        # A boundary site is not an interior: this mutant must fail the depth
        # formula checked independently of the product or the census formula.
        before, after = b"return four_h > 0;", b"return four_h >= 0;"
        if data.count(before) != 1:
            raise RuntimeError("mutation target changed")
        probe.write_bytes(data.replace(before, after))
        checked(command)
        mutant = subprocess.run([str(executable)], capture_output=True, text=True,
                                timeout=60)
        if mutant.returncode != 1 or "independent depth" not in mutant.stderr:
            raise RuntimeError("closed-boundary mutant not causally rejected")
        return {"status": "passed", "scope": "local_credit_and_bounded_q2_consumer",
                "not_full_tower": True, "not_production_census": True,
                "source_state": "working_tree_snapshot", "source_sha256": manifest,
                "git_head": checked(["git", "rev-parse", "HEAD"]).stdout.strip(),
                "runner_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                "helper_sha256": hashlib.sha256((here / "local_credits_checks.py").read_bytes()).hexdigest(),
                "compiler": checked([compiler, "--version"]).stdout.splitlines()[0],
                "compile_command": command, "positive_exit_code": positive.returncode,
                "cases": rows,
                "mutant": {"name": "closed_boundary", "exit_code": mutant.returncode,
                           "diagnostic": mutant.stderr.strip()}, "gcp_used": False}


def main() -> int:
    if sys.argv[1:] != ["--selftest"]:
        print("usage: transverse_residual_checks.py --selftest", file=sys.stderr)
        return 2
    try:
        result = run()
    except (OSError, RuntimeError, subprocess.SubprocessError, ValueError) as error:
        print(f"transverse residual checks failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
