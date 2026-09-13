"""Check the pinned exceptional CreditPlan assignment and a temporary repair."""

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


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
COMMIT = "3589a2c92de29da9f140e97cdb6b2970f1d4012c"
PINS = {
    "core/types.hpp":
        "f4c05da3de254aadf95993988a44a933bf235282bc6f33930984689eba967f2b",
    "spindle/predicates.hpp":
        "38cb2dbc1bf1ad5dd9219d2c3ba3ae68e90003fea92c2371c9a9e853eb973845",
    "pipeline/local_credits.hpp":
        "af01776e5263abebd20b46c6739f64f72a8834c1eebcfa7a2a012eb0f2b837c5",
    "pipeline/local_credits.cpp":
        "d68e694ea841ddc25af5f8fe82fe6961b8e5a6c4d64744c7d3824141c5784e39",
    "pipeline/tube_credits.hpp":
        "a850a4425a7a2f79a46348e49fa7e662db155aaac871794138a6eb616fa17c2c",
}
FLAGS = [
    "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1",
    "-fsanitize=undefined", "-fno-sanitize-recover=all",
]
COPY_AND_SWAP = """class CreditPlan final {
 public:
  CreditPlan(const CreditPlan&) = default;
  CreditPlan(CreditPlan&&) noexcept = default;
  CreditPlan& operator=(CreditPlan&&) noexcept = default;
  CreditPlan& operator=(const CreditPlan& other) {
    CreditPlan temporary(other);
    swap(temporary);
    return *this;
  }
  void swap(CreditPlan& other) noexcept {
    using std::swap;
    swap(rectangle_, other.rectangle_);
    swap(lane_, other.lane_);
    swap(strategy_, other.strategy_);
    swap(threshold_, other.threshold_);
    swap(core_, other.core_);
    swap(a_, other.a_);
    swap(b_, other.b_);
    swap(a_order_, other.a_order_);
    swap(b_order_, other.b_order_);
    swap(blocks_, other.blocks_);
    swap(total_, other.total_);
    swap(candidates_, other.candidates_);
    swap(work_, other.work_);
  }
"""


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def command(args: list[str], cwd: Path | None = None) -> dict[str, Any]:
    try:
        completed = subprocess.run(
            args, cwd=cwd, capture_output=True, text=True, timeout=55, check=False
        )
        return {
            "argv": args, "returncode": completed.returncode,
            "stdout": completed.stdout, "stderr": completed.stderr,
        }
    except subprocess.TimeoutExpired:
        return {"argv": args, "returncode": None, "status": "timeout"}


def audit() -> dict[str, Any]:
    report: dict[str, Any] = {
        "schema": "mhgp8_plan_assignment_exception_v1",
        "status": "failed",
        "source_commit": COMMIT,
        "scope": "copy_assignment_allocation_failure_then_target_reuse",
        "normal_execution_regression_claimed": False,
        "undefined_behavior_executed_by_probe": False,
        "python_optimization": sys.flags.optimize,
        "invocation": sys.argv,
        "sources": {},
        "variants": [],
        "gcp_used": False,
    }
    try:
        sources: dict[str, bytes] = {}
        for name, expected in PINS.items():
            args = ["git", "show", f"{COMMIT}:morsehgp3D_v8/src/{name}"]
            read = subprocess.run(args, cwd=ROOT, capture_output=True, timeout=55, check=True)
            data = read.stdout
            actual = digest(data)
            report["sources"][name] = {
                "read_command": args, "read_returncode": read.returncode,
                "expected_sha256": expected, "actual_sha256": actual,
            }
            if actual != expected:
                raise RuntimeError(f"source pin mismatch: {name}")
            sources[name] = data
        probe = (HERE / "plan_assignment_probe.cpp").read_bytes()
        report["probe_sha256"] = digest(probe)
        report["runner_sha256"] = digest(Path(__file__).read_bytes())
        compiler = shutil.which("g++")
        if compiler is None:
            raise RuntimeError("g++ unavailable")
        report["compiler"] = command([compiler, "--version"])
        report["compiler_sha256"] = digest(Path(compiler).read_bytes())
        if report["compiler"]["returncode"] != 0:
            raise RuntimeError("compiler identification failed")
        with tempfile.TemporaryDirectory(prefix="mhgp8_plan_assignment_") as tmp:
            for variant_name in ("original", "temporary_copy_and_swap"):
                repaired = variant_name != "original"
                variant: dict[str, Any] = {
                    "name": variant_name, "status": "failed", "snapshot_sha256": {},
                }
                report["variants"].append(variant)
                directory = Path(tmp) / variant_name
                directory.mkdir()
                for name, original in sources.items():
                    data = original
                    if repaired and name == "pipeline/local_credits.hpp":
                        text = data.decode()
                        marker = "class CreditPlan final {\n public:\n"
                        if text.count(marker) != 1 or text.count("#include <vector>") != 1:
                            raise RuntimeError("repair site not unique")
                        text = text.replace(marker, COPY_AND_SWAP, 1)
                        data = text.replace(
                            "#include <vector>", "#include <vector>\n#include <utility>", 1
                        ).encode()
                        if data == original:
                            raise RuntimeError("repair was not applied")
                    destination = directory / name
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    destination.write_bytes(data)
                    variant["snapshot_sha256"][name] = digest(data)
                source = directory / "probe.cpp"
                source.write_bytes(probe)
                executable = directory / "probe"
                variant["compile"] = command([
                    compiler, *FLAGS, "-I", str(directory), str(source),
                    str(directory / "pipeline/local_credits.cpp"), "-o", str(executable),
                ])
                if variant["compile"]["returncode"] != 0:
                    raise RuntimeError(f"compilation failed: {variant_name}")
                variant["executable_sha256"] = digest(executable.read_bytes())
                mode = "--expect-strong" if repaired else "--expect-vulnerable"
                run = command([str(executable), mode])
                variant["run"] = run
                if run["returncode"] != 0 or run["stderr"]:
                    raise RuntimeError(f"fixture failed: {variant_name}")
                observed = json.loads(run["stdout"])
                expected = {
                    "allocation_failure_caught": 1,
                    "owner_changed": int(not repaired),
                    "new_owner_a_size": 2 if repaired else 3,
                    "retained_a_credits_size": 2,
                    "new_owner_b_size": 2 if repaired else 3,
                    "retained_b_credits_size": 2,
                    "retained_candidates": 1,
                    "source_candidates": 9,
                    "invalid_emissions": 0 if repaired else 1,
                    "wrong_lengths": int(not repaired),
                }
                if observed != expected:
                    raise RuntimeError(f"unexpected physical result: {variant_name}")
                variant["observed"] = observed
                variant["status"] = "repair_verified" if repaired else "defect_reproduced"
        report["status"] = "passed"
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as error:
        report["error"] = str(error)
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="Create a new receipt; never overwrite.")
    args = parser.parse_args()
    if args.output is not None and args.output.exists():
        parser.error("output already exists")
    report = audit()
    encoded = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output is not None:
        with args.output.open("x", encoding="utf-8") as output:
            output.write(encoded)
    print(encoded, end="")
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
