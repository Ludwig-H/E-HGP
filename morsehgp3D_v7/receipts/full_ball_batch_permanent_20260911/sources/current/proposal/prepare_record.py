#!/usr/bin/env python3
"""Mechanical private adaptation of the already-read CMake/CTest recorder."""
import difflib
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT.parent / "v7_census_tower_permanent_20260911/record.py"
PIN = "f20247a4f9d9c50c45d2444647ed1ab5992f6979e8de4f669879c7c06e4896e8"


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError("unique adaptation marker: " + old)
    return text.replace(old, new)


def main():
    if hashlib.sha256(SOURCE.read_bytes()).hexdigest() != PIN:
        raise RuntimeError("recorder source changed")
    text = SOURCE.read_text()
    text = replace_once(text, 'TARGET_NAME = "mhgp7_census_tower_gate"', 'TARGET_NAME = "mhgp7_full_ball_batch_gate"')
    start = 'TEST_NAMES = {"mhgp7_census_tower_" + name for name in ('
    end = '"mutant_assignment", "mutant_open", "mutant_adjacency", "mutant_census")}\n'
    begin, finish = text.index(start), text.index(end) + len(end)
    text = text[:begin] + 'TEST_NAMES = {"mhgp7_full_ball_batch_" + name for name in (\n' + \
        '    "cpu1", "cpu4", "rejects", "bad_argument", "missing_argument")}\n' + text[finish:]
    text = text.replace('^mhgp7_census_tower_', '^mhgp7_full_ball_batch_')
    text = replace_once(text, 'len(tests) != 11', 'len(tests) != 5')
    text = replace_once(text,
        'source += [path for path in sorted((ACTIVE / directory).rglob("*")) if path.is_file()]',
        'source += [path for path in sorted((ACTIVE / directory).rglob("*")) if path.is_file() and\n'
        '            "__pycache__" not in path.parts and path.suffix not in (".pyc", ".pyo")]')
    begin = text.index('        real = [data for data in results')
    finish = text.index('        receipt["CTest_results"] = results', begin)
    text = text[:begin] + '''        nominal = [data for data in results if data.get("status") == "passed_batch_callback"]
        rejects = [data for data in results if data.get("status") == "passed_batch_rejections"]
        if len(nominal) != 2 or {data.get("workers") for data in nominal} != {1, 4} or \\
                any(data.get("batch_calls", 0) < 20 or data.get("batch_requests", 0) < 80 or
                    data.get("batch_requests") != data.get("direct_terminals") or
                    data.get("diagnostic_reference_MEB_calls", 0) <= 0 or
                    data.get("product_instrumentation") is not False for data in nominal):
            raise RuntimeError("callback nominal nonvacuity absent")
        if len(rejects) != 1 or rejects[0].get("cases") != 65 or rejects[0].get("prefix_cases") != 32 or \\
                rejects[0].get("known_paid") != 44 or rejects[0].get("product_instrumentation") is not False:
            raise RuntimeError("callback causal refusals absent")
''' + text[finish:]
    with (ROOT / "record.py").open("x") as stream:
        stream.write(text)
    cmake = (ROOT / "baseline/CMakeLists.txt").read_text()
    anchor = "mhgp7_product_executable(mhgp7_full_ball_work_gate tests/full_ball_work_gate.cpp)"
    updated = replace_once(cmake, anchor, (ROOT / "CMake.fragment").read_text() + anchor)
    with (ROOT / "CMake.diff").open("x") as stream:
        stream.write("".join(difflib.unified_diff(cmake.splitlines(keepends=True), updated.splitlines(keepends=True),
            fromfile="a/morsehgp3D_v7/CMakeLists.txt", tofile="b/morsehgp3D_v7/CMakeLists.txt")))
    print("prepared recorder and CMake.diff; no compilation")


if __name__ == "__main__":
    main()
