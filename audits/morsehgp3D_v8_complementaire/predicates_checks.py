"""Replay pinned v8 predicates and two source mutants in fresh temporary trees."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from typing import Any


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PINS = {
    "core/types.hpp":
        "b25f9b2b9041f4d27d9846b30a74d46981d20d71e4ef20920dd683775d411b80",
    "spindle/predicates.hpp":
        "38cb2dbc1bf1ad5dd9219d2c3ba3ae68e90003fea92c2371c9a9e853eb973845",
}
EXPECTED = {
    "point_tests": 900000,
    "universal_queries": 7290000,
    "credit_blocks": 411,
    "negative_blocks": 46913,
    "uncertain_blocks": 42676,
    "oracle_checks": 65610000,
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


def main() -> int:
    report: dict[str, Any] = {
        "schema": "mhgp8_complementary_predicates_v1",
        "status": "failed",
        "scope": "bounded_predicates_only_not_wspd_or_full_qualification",
        "seed": 937104,
        "headers": {},
        "variants": [],
        "gcp_used": False,
    }
    try:
        sources: dict[str, bytes] = {}
        for name, expected in PINS.items():
            path = ROOT / "morsehgp3D_v8" / "src" / name
            data = path.read_bytes()
            actual = sha256(data)
            report["headers"][name] = {"expected": expected, "actual": actual}
            if actual != expected:
                raise RuntimeError(f"source pin mismatch: {name}")
            sources[name] = data
        probe = (HERE / "predicates_probe.cpp").read_bytes()
        report["probe_sha256"] = sha256(probe)
        report["runner_sha256"] = sha256(Path(__file__).read_bytes())
        compiler = shutil.which("g++")
        if compiler is None:
            raise RuntimeError("g++ unavailable")
        report["compiler"] = command([compiler, "--version"])
        if report["compiler"]["returncode"] != 0:
            raise RuntimeError("compiler identification failed")
        variants = [
            ("original", None, None, 0, "PASS "),
            ("strict_boundary", "if (h <= 0) {", "if (h < 0) {",
             1, "point mismatch"),
            ("maximum_to_minimum", "result += std::max(at_a(",
             "result += std::min(at_a(", 4, "negative mismatch"),
        ]
        with tempfile.TemporaryDirectory(prefix="mhgp8_predicate_audit_") as tmp:
            for name, old, new, expected_code, expected_text in variants:
                variant: dict[str, Any] = {
                    "name": name, "expected_returncode": expected_code,
                    "status": "failed",
                }
                report["variants"].append(variant)
                directory = Path(tmp) / name
                directory.mkdir()
                for filename, original in sources.items():
                    data = original
                    if filename == "spindle/predicates.hpp" and old is not None:
                        text = original.decode()
                        if text.count(old) != 1:
                            raise RuntimeError(f"non-unique mutation site: {name}")
                        data = text.replace(old, new, 1).encode()
                        if data == original:
                            raise RuntimeError(f"mutation not applied: {name}")
                    path = directory / filename
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(data)
                    if filename == "spindle/predicates.hpp":
                        variant["predicates_sha256"] = sha256(data)
                source = directory / "probe.cpp"
                source.write_bytes(probe)
                executable = directory / "probe"
                variant["compile"] = command(
                    [compiler, *FLAGS, "-I", str(directory), str(source),
                     "-o", str(executable)]
                )
                if variant["compile"]["returncode"] != 0:
                    raise RuntimeError(f"compilation failed: {name}")
                variant["executable_sha256"] = sha256(executable.read_bytes())
                run = command([str(executable)])
                variant["run"] = run
                if run["returncode"] != expected_code:
                    raise RuntimeError(f"unexpected return code: {name}")
                selected_output = run["stdout"] if name == "original" else run["stderr"]
                if not selected_output.startswith(expected_text):
                    raise RuntimeError(f"missing expected diagnostic: {name}")
                if name == "original":
                    counters = {
                        key: int(value)
                        for key, value in re.findall(r"([a-z_]+)=(\d+)", run["stdout"])
                    }
                    variant["counters"] = counters
                    if counters != EXPECTED or run["stderr"]:
                        raise RuntimeError("positive counts or sanitizer check failed")
                elif run["stdout"] or "runtime error:" in run["stderr"]:
                    raise RuntimeError(f"mutant failed for an unexpected reason: {name}")
                variant["status"] = "passed" if name == "original" else "refuted"
        report["status"] = "passed"
    except (OSError, RuntimeError, ValueError) as error:
        report["error"] = str(error)
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
