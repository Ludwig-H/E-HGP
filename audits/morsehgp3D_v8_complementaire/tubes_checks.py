"""Reproduce the independently checked tube splice and two temporary mutants."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from typing import Any


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PINS = {
    # New source bytes: this types.hpp documents the 75-bit tube bound.
    # Earlier predicate receipts keep their old, separate 73-bit-comment pin.
    "core/types.hpp":
        "f4c05da3de254aadf95993988a44a933bf235282bc6f33930984689eba967f2b",
    "spindle/predicates.hpp":
        "38cb2dbc1bf1ad5dd9219d2c3ba3ae68e90003fea92c2371c9a9e853eb973845",
    "pipeline/local_credits.hpp":
        "791dfe12079278e31904b02552dbc7d852ec6ddcc610e9ab4d1489ccb5df6e65",
    "pipeline/local_credits.cpp":
        "b8a7eef8ca112ed0921772e4a7d380d802f479814ca686da8a1a856647a626c6",
    "pipeline/tube_credits.hpp":
        "a850a4425a7a2f79a46348e49fa7e662db155aaac871794138a6eb616fa17c2c",
}
EXPECTED = {
    "plans": 3051,
    "credits_checked": 65772,
    "pair_checks": 360933,
    "credited_sites": 30881,
    "sweep_tests": 76538,
    "fallbacks": 24,
    "inactive_or_core_saturated": 343,
}
FLAGS = [
    "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1",
    "-fsanitize=undefined", "-fno-sanitize-recover=all",
]


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def command(args: list[str]) -> dict[str, Any]:
    try:
        completed = subprocess.run(
            args, capture_output=True, text=True, timeout=55, check=False
        )
        return {
            "argv": args,
            "returncode": completed.returncode,
            "stdout": completed.stdout,
            "stderr": completed.stderr,
        }
    except subprocess.TimeoutExpired:
        return {"argv": args, "returncode": None, "status": "timeout"}


def audit() -> dict[str, Any]:
    report: dict[str, Any] = {
        "schema": "mhgp8_complementary_tubes_v1",
        "status": "failed",
        "scope": "bounded_tube_splice_only_not_wspd_or_full_qualification",
        "owner_scope": "factory_created_owner_without_copy_assignment_or_mutation",
        "ownership_review": "morsehgp3D_v8/audits/DIALOGUE_COURANT.md",
        "python": {
            "executable": sys.executable,
            "version": sys.version,
            "argv": sys.argv,
            "optimization": sys.flags.optimize,
        },
        "seed": 930174,
        "sources": {},
        "variants": [],
        "gcp_used": False,
    }
    try:
        sources: dict[str, bytes] = {}
        for name, expected in PINS.items():
            data = (ROOT / "morsehgp3D_v8" / "src" / name).read_bytes()
            actual = sha256(data)
            report["sources"][name] = {"expected": expected, "actual": actual}
            if actual != expected:
                raise RuntimeError(f"source pin mismatch: {name}")
            sources[name] = data
        probe = (HERE / "tubes_probe.cpp").read_bytes()
        report["probe_sha256"] = sha256(probe)
        report["runner_sha256"] = sha256(Path(__file__).read_bytes())
        compiler = shutil.which("g++")
        if compiler is None:
            raise RuntimeError("g++ unavailable")
        report["compiler_sha256"] = sha256(Path(compiler).read_bytes())
        report["compiler"] = command([compiler, "--version"])
        if report["compiler"]["returncode"] != 0:
            raise RuntimeError("compiler identification failed")
        variants = [
            ("original", None, None, 0, "PASS "),
            ("self_credit_at_zero_gap", "if (delta > 0 &&", "if (delta >= 0 &&",
             1, "unsafe tube credit\n"),
            ("insufficient_separation_25", "100 * static_cast<i128>(maximum_diameter_squared)",
             "25 * static_cast<i128>(maximum_diameter_squared)", 1, "fallback boundary\n"),
        ]
        with tempfile.TemporaryDirectory(prefix="mhgp8_tubes_audit_") as tmp:
            for name, old, new, expected_code, expected_text in variants:
                variant: dict[str, Any] = {
                    "name": name,
                    "expected_returncode": expected_code,
                    "status": "failed",
                    "snapshot_sha256": {},
                }
                report["variants"].append(variant)
                directory = Path(tmp) / name
                directory.mkdir()
                for filename, original in sources.items():
                    data = original
                    if filename == "pipeline/tube_credits.hpp" and old is not None:
                        text = original.decode()
                        if text.count(old) != 1:
                            raise RuntimeError(f"non-unique mutation site: {name}")
                        data = text.replace(old, new, 1).encode()
                        if data == original:
                            raise RuntimeError(f"mutation not applied: {name}")
                    path = directory / filename
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(data)
                    variant["snapshot_sha256"][filename] = sha256(data)
                source = directory / "probe.cpp"
                source.write_bytes(probe)
                executable = directory / "probe"
                variant["compile"] = command(
                    [compiler, *FLAGS, "-I", str(directory), str(source),
                     str(directory / "pipeline/local_credits.cpp"), "-o", str(executable)]
                )
                if variant["compile"]["returncode"] != 0:
                    raise RuntimeError(f"compilation failed: {name}")
                variant["executable_sha256"] = sha256(executable.read_bytes())
                run = command([str(executable)])
                variant["run"] = run
                if run["returncode"] != expected_code:
                    raise RuntimeError(f"unexpected return code: {name}")
                if name == "original":
                    if not run["stdout"].startswith(expected_text) or run["stderr"]:
                        raise RuntimeError("positive output or sanitizer check failed")
                    counters = {
                        key: int(value)
                        for key, value in re.findall(r"([a-z_]+)=(\d+)", run["stdout"])
                    }
                    variant["counters"] = counters
                    if counters != EXPECTED:
                        raise RuntimeError("positive counts or nonvacuity changed")
                elif run["stdout"] or run["stderr"] != expected_text:
                    raise RuntimeError(f"mutant failed for an unexpected reason: {name}")
                variant["status"] = "passed" if name == "original" else "refuted"
        report["status"] = "passed"
    except (OSError, RuntimeError, ValueError) as error:
        report["error"] = str(error)
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="Create a new JSON receipt; never overwrite.")
    args = parser.parse_args()
    if args.output is not None and args.output.exists():
        parser.error("output already exists; closed receipts must not be overwritten")
    report = audit()
    encoded = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output is not None:
        with args.output.open("x", encoding="utf-8") as output:
            output.write(encoded)
    print(encoded, end="")
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
