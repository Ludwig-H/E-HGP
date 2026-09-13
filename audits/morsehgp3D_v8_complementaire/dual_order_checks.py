#!/usr/bin/env python3
"""Compare a temporary witness-order experiment with the actual v8 planner."""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


ORIGINAL = """      visit(anchors, z.left, depth + 1);
      visit(anchors, z.right, depth + 1);"""
ORDERED = """      // Temporary auditor experiment for axis-aligned separation.
      if (opposite_.low.x > z.box.high.x) {
        visit(anchors, z.right, depth + 1);
        visit(anchors, z.left, depth + 1);
      } else {
        visit(anchors, z.left, depth + 1);
        visit(anchors, z.right, depth + 1);
      }"""


def require(condition: bool, cause: str) -> None:
    if not condition:
        raise RuntimeError(cause)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def execute(command: list[str], timeout: int = 60) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, capture_output=True, text=True,
                            check=False, timeout=timeout)
    require(result.returncode == 0,
            f"command failed ({result.returncode}): {command}: {result.stderr}")
    return result


def run() -> dict[str, object]:
    own = Path(__file__).resolve().parent
    root = own.parents[1]
    source_root = root / "morsehgp3D_v8" / "src"
    source_paths = sorted(path for path in source_root.rglob("*")
                          if path.is_file() and path.suffix in (".hpp", ".cpp"))
    source_data = {str(path.relative_to(root)): path.read_bytes()
                   for path in source_paths}
    probe_path = own / "dual_order_probe.cpp"
    probe_data = probe_path.read_bytes()
    boost_candidates = (Path("/usr/include"),
                        root / "build/v7_boost_gate/extracted/usr/include")
    boost = next((path for path in boost_candidates
                  if (path / "boost/multiprecision/cpp_int.hpp").is_file()), None)
    require(boost is not None, "Boost headers unavailable for the independent oracle")
    if boost is None:
        raise RuntimeError("unreachable missing Boost headers")
    relative_cpp = "morsehgp3D_v8/src/pipeline/local_credits.cpp"
    original_cpp = source_data[relative_cpp].decode()
    require(original_cpp.count(ORIGINAL) == 1, "witness recursion location changed")
    changed_cpp = original_cpp.replace(ORIGINAL, ORDERED)
    cases = [("boundary", size, 4) for size in (64, 128, 256, 512, 1024)]
    cases += [("reflected_boundary", 128, 4)]
    cases += [("line", 128, q) for q in (2, 3, 4)]
    cases += [("grid", size, q) for size in (128, 512) for q in (2, 3, 4)]
    rows: list[dict[str, object]] = []
    counter_increases: list[dict[str, object]] = []
    sanitized_cases = 0
    compile_flags = ["-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    with tempfile.TemporaryDirectory(prefix="mhgp8_dual_order_") as directory:
        temporary = Path(directory)
        snapshot = temporary / "snapshot"
        for relative, data in source_data.items():
            destination = snapshot / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
        probe = temporary / "dual_order_probe.cpp"
        probe.write_bytes(probe_data)
        include = snapshot / "morsehgp3D_v8/src"
        original = snapshot / relative_cpp
        changed = original.with_name("local_credits_ordered.cpp")
        changed.write_text(changed_cpp)
        commands: dict[str, list[str]] = {}
        for arm, implementation in (("original", original), ("ordered", changed)):
            executable = temporary / arm
            command = ["g++", *compile_flags, "-I", str(include), "-isystem", str(boost),
                       str(probe), str(implementation), "-o", str(executable)]
            execute(command)
            commands[arm] = [str(executable)]
        for family, size, lane in cases:
            by_arm: dict[str, list[dict[str, object]]] = {}
            for arm, command in commands.items():
                capture = execute([*command, family, str(size), str(lane)])
                parsed = [json.loads(line) for line in capture.stdout.splitlines()]
                require(len(parsed) == 3, "missing planner arm")
                by_arm[arm] = parsed
            for before, after in zip(by_arm["original"], by_arm["ordered"], strict=True):
                require(before["strategy"] == after["strategy"], "strategy order")
                require(before["a_credits"] == after["a_credits"] and
                        before["b_credits"] == after["b_credits"],
                        "temporary order changed literal credits")
                require(before["candidate_pairs"] == after["candidate_pairs"],
                        "temporary order changed candidate count")
                if before["strategy"] != "dual":
                    require(before == after, "unrelated strategy changed")
                else:
                    for metric in ("dual_tasks", "block_bound_tests", "negative_probes",
                                   "leaf_pairs", "universal_queries", "point_tests"):
                        if int(after[metric]) > int(before[metric]):
                            counter_increases.append({"family": family, "size": size,
                                                      "lane": lane, "metric": metric,
                                                      "before": before[metric],
                                                      "after": after[metric]})
                for arm, values in (("original", before), ("ordered", after)):
                    digest_input = json.dumps([values.pop("a_credits"),
                                               values.pop("b_credits")],
                                              separators=(",", ":")).encode()
                    rows.append({"arm": arm, **values,
                                 "credits_sha256": sha256(digest_input)})
        # Undefined-behaviour instrumentation on the temporary reordering.
        sanitized = temporary / "ordered_ubsan"
        execute(["g++", *compile_flags, "-fsanitize=undefined",
                 "-fno-sanitize-recover=all", "-I", str(include), "-isystem", str(boost),
                 str(probe), str(changed), "-o", str(sanitized)])
        for family, size, lane in (("boundary", 128, 4), ("reflected_boundary", 128, 4),
                                  ("grid", 128, 2), ("grid", 128, 3), ("grid", 128, 4)):
            capture = execute([str(sanitized), family, str(size), str(lane)])
            require(len(capture.stdout.splitlines()) == 3, "UBSan planner nonvacuity")
            sanitized_cases += 1
    source_hashes_still_match = all((root / relative).is_file() and
                                   (root / relative).read_bytes() == data
                                   for relative, data in source_data.items())
    require(probe_path.read_bytes() == probe_data, "auditor probe changed during receipt")
    require(len(cases) == 15 and len(rows) == 90 and sanitized_cases == 5,
            "campaign nonvacuity")
    largest = [row for row in rows if row["family"] == "boundary" and
               row["size"] == 1024 and row["strategy"] == "dual"]
    require(len(largest) == 2 and all(row["candidate_pairs"] == 36 for row in largest),
            "constant residual nonvacuity")
    require(int(largest[0]["dual_tasks"]) > 1000 * int(largest[1]["dual_tasks"]),
            "ordering effect no longer reproduced")
    return {"schema": "mhgp8_complementary_dual_order_checks_v1", "status": "passed",
            "created_utc": datetime.now(timezone.utc).isoformat(),
            "scope": "bounded_single_rectangle_credits_not_wspd_or_full",
            "public_status": "not_claimed", "threads_per_probe": 1,
            "source_state": "working_tree_snapshot",
            "source_hashes_still_match_worktree": source_hashes_still_match,
            "performance_basis": "deterministic_work_counters_no_timing_claim",
            "source_sha256": {relative: sha256(data) for relative, data in source_data.items()},
            "auditor_sources_sha256": {probe_path.name: sha256(probe_data),
                                        Path(__file__).name: sha256(Path(__file__).read_bytes())},
            "boost_include": str(boost),
            "boost_version_header_sha256": sha256((boost / "boost/version.hpp").read_bytes()),
            "compiler": execute(["g++", "--version"]).stdout.splitlines()[0],
            "compile_flags": compile_flags, "ordered_cpp_sha256": sha256(changed_cpp.encode()),
            "temporary_edit": {"old": ORIGINAL, "new": ORDERED},
            "cases_per_arm": len(cases), "rows": rows,
            "ubsan_cases": sanitized_cases, "counter_increases": counter_increases,
            "limitation": "Axis-aligned ordering experiment; general directions and whole-tour cost remain open. Tree construction is still paid. No pool credits are added to DualBlocks."}


def main() -> int:
    if sys.argv[1:] != ["--selftest"]:
        print("usage: dual_order_checks.py --selftest", file=sys.stderr)
        return 2
    try:
        result = run()
    except (OSError, RuntimeError, ValueError, subprocess.TimeoutExpired) as error:
        print(f"dual order checks failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
