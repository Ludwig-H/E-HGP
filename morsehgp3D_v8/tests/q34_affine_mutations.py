#!/usr/bin/env python3
"""Three causal tranche33 search mutations, without touching frozen builds.

Explicit reuse of constructor31's ten-command capture/read/closure protocol.
Only a temporary q34_witness_search.cpp object changes; the independent
singleton witness oracle must reject geometry before any counter ledger.
The header-only prepared bounds have a separate exact cpp_int gate, not a
new overlay build system. Earlier mutation receipts remain untouched.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path

import wspd_q34_mutations as protocol

ROOT = protocol.ROOT
GATE = "q34_witness_search"
protocol.SCHEMA = "mhgp8_q34_affine_mutants_v1"
protocol.SCOPE = "three_causal_affine_search_mutations_independent_singleton_geometry"
protocol.SOURCE = "morsehgp3D_v8/src/lanes/q34_witness_search.cpp"
protocol.FAILURE = "q34 witness search gate: singleton witness decision differs from independent oracle\n"
protocol.MUTATIONS = (
    ("xi_upper_used_to_exclude", (("static_cast<i128>(16) * xi.low)", "static_cast<i128>(16) * xi.high)"),)),
    ("new_admission_wrong_xi_scale", (("const i128 xi16 = static_cast<i128>(16) * xi.high;",
        "const i128 xi16 = static_cast<i128>(Exclusion ? 4 : 16) * xi.high;"),)),
    ("local_exclusion_removes_global_lane", ((
        "            frame.mask &= static_cast<std::uint8_t>(~bit);\n            continue;",
        "            frame.mask &= static_cast<std::uint8_t>(~bit);\n"
        "            remaining &= static_cast<std::uint8_t>(~bit);\n            continue;"),)),
)


def inputs(build):
    from run_q34_affine_checks import SOURCES
    protocol.require(len(SOURCES) == 211 and str(Path(__file__).relative_to(ROOT)) in SOURCES,
                     "affine mutation adapter missing from constructor33 inventory")
    artifacts = {str((build / name).relative_to(ROOT)) for name in (
        "CMakeCache.txt", "libmhgp8_p0.a", f"mhgp8_{GATE}_gate",
        f"CMakeFiles/mhgp8_{GATE}_gate.dir/tests/{GATE}_gate.cpp.o")}
    return SOURCES, artifacts


def planned_commands(build, temporary, compiler):
    result = [("baseline", [str(build / f"mhgp8_{GATE}_gate"), "--selftest"], 0)]
    for name, _ in protocol.MUTATIONS:
        result.extend([
            (name + "_compile", [compiler, "-std=c++20", "-O3", "-DNDEBUG", "-Wall", "-Wextra",
             "-Wpedantic", "-Werror", "-I" + str(ROOT / "morsehgp3D_v8/src"),
             "-I" + str(ROOT / "morsehgp3D_v8/src/lanes"), "-c", str(temporary / (name + ".cpp")),
             "-o", str(temporary / (name + ".o"))], 0),
            (name + "_link", [compiler,
             str(build / f"CMakeFiles/mhgp8_{GATE}_gate.dir/tests/{GATE}_gate.cpp.o"),
             str(temporary / (name + ".o")), str(build / "libmhgp8_p0.a"),
             "-pthread", "-o", str(temporary / name)], 0),
            (name + "_oracle", [str(temporary / name), "--selftest"], 1),
        ])
    return result


def baseline(stdout):
    from run_q34_affine_checks import validate_gate
    validate_gate(protocol.parse_result(stdout.encode()), f"mhgp8_{GATE}_gate")


protocol.inputs = inputs
protocol.planned_commands = planned_commands
protocol.baseline = baseline


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--build", type=Path, required=True)
    capture.add_argument("--output", type=Path, required=True)
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    reader.add_argument("--check-live", action="store_true")
    args = parser.parse_args()
    if args.operation == "run":
        protocol.run(args)
    else:
        print(json.dumps(protocol.read(args.path, args.check_live), sort_keys=True, allow_nan=False))


if __name__ == "__main__":
    main()
