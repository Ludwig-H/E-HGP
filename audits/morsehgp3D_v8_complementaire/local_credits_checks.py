#!/usr/bin/env python3
"""Run the bounded rails audit against a pinned temporary v8 source copy."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SOURCES = (
    "src/core/types.hpp", "src/spindle/predicates.hpp",
    "src/pipeline/local_credits.hpp", "src/pipeline/local_credits.cpp",
    "src/pipeline/tube_credits.hpp",
)


def checked(argv: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(argv, check=True, text=True, capture_output=True,
                          cwd=ROOT, timeout=60)


def run() -> dict[str, object]:
    compiler = shutil.which("g++")
    if compiler is None:
        raise RuntimeError("g++ unavailable")
    here = Path(__file__).resolve().parent
    manifest = {}
    with tempfile.TemporaryDirectory(prefix="mhgp8_local_credits_audit_") as name:
        temporary = Path(name)
        snapshot = temporary / "v8"
        for relative in SOURCES:
            source = ROOT / "morsehgp3D_v8" / relative
            data = source.read_bytes()
            manifest[str(source.relative_to(ROOT))] = hashlib.sha256(data).hexdigest()
            target = snapshot / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        probe = temporary / "probe.cpp"
        data = (here / "local_credits_probe.cpp").read_bytes()
        probe.write_bytes(data)
        manifest[str((here / "local_credits_probe.cpp").relative_to(ROOT))] = (
            hashlib.sha256(data).hexdigest())
        executable = temporary / "probe"
        command = [compiler, "-std=c++20", "-O2", "-Wall", "-Wextra",
                   "-Wpedantic", "-Werror", "-fsanitize=undefined",
                   "-fno-sanitize-recover=all", f"-I{snapshot / 'src'}",
                   str(probe), str(snapshot / "src/pipeline/local_credits.cpp"),
                   "-o", str(executable)]
        checked(command)
        positive = checked([str(executable)])
        rows = [json.loads(line) for line in positive.stdout.splitlines()]
        if len(rows) != 9 or any(row["status"] != "passed" for row in rows):
            raise RuntimeError("positive nonvacuity")
        if positive.stderr:
            raise RuntimeError(f"unexpected sanitizer diagnostic: {positive.stderr}")
        # Mutations affect only temporary copies, never developer files.
        mutants = []
        specifications = (
            ("src/pipeline/local_credits.hpp", "repeat_first_column",
             "consumer(a_order_[a], b_order_[b]);",
             "consumer(a_order_[a], b_order_[block.b.first]);", "duplicates"),
            ("src/pipeline/local_credits.cpp", "double_leaf_credit",
             "add(anchors, 1);", "add(anchors, 2);", "dual residual"),
        )
        for relative, label, before, after, expected_error in specifications:
            target = snapshot / relative
            original = target.read_text()
            if original.count(before) != 1:
                raise RuntimeError(f"mutant target changed: {label}")
            target.write_text(original.replace(before, after))
            try:
                checked(command)
                result = subprocess.run([str(executable)], text=True,
                                        capture_output=True, timeout=60)
                if result.returncode != 1 or expected_error not in result.stderr:
                    raise RuntimeError(f"mutant not causally rejected: {label}: {result}")
                mutants.append({"name": label, "exit_code": result.returncode,
                                "diagnostic": result.stderr.strip()})
            finally:
                target.write_text(original)
        return {
            "status": "passed", "scope": "v8_local_credit_module_not_full_tower",
            "source_state": "working_tree_snapshot",
            "git_head": checked(["git", "rev-parse", "HEAD"]).stdout.strip(),
            "git_status_short": checked(["git", "status", "--short"]).stdout.splitlines(),
            "source_sha256": manifest,
            "runner_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            "compiler": checked([compiler, "--version"]).stdout.splitlines()[0],
            "compile_command": command, "positive_exit_code": positive.returncode,
            "positive_stderr": positive.stderr, "cases": rows, "mutants": mutants,
        }


def main() -> int:
    if sys.argv[1:] != ["--selftest"]:
        print("usage: local_credits_checks.py --selftest", file=sys.stderr)
        return 2
    try:
        result = run()
    except (OSError, RuntimeError, subprocess.SubprocessError, ValueError) as error:
        print(f"local credit checks failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
