#!/usr/bin/env python3
"""Three temporary constructor34 mutations, geometric oracle before ledgers.

The constructor31 capture/compiler/closure harness is explicitly reused.
Only a temporary local-engine object is replaced; pinned libraries, sources,
executables and prior verdicts are never overwritten or promoted.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path

import wspd_q34_mutations as protocol

ROOT = protocol.ROOT
GATE = "q4_seed_cells"
protocol.SCHEMA = "mhgp8_q4_seed_cells_mutants_v1"
protocol.SCOPE = "three_causal_seed_cell_mutations_independent_rational_geometry"
protocol.SOURCE = "morsehgp3D_v8/src/lanes/q4_local.cpp"
protocol.FAILURE = "q4 seed cells gate: seed-cell stream differs from independent rational support/depth/shell oracle\n"
# Replacements will be checked against the stabilized engine and exercised
# by the new gate before the constructor34 inventory is frozen.
protocol.MUTATIONS = (
    ("positive_contact_lost", ((
        "if(bound.minimum>0) {counter_add(extra.positive_products);continue;}",
        "if(bound.minimum>=0) {counter_add(extra.positive_products);continue;}"),)),
    ("single_live_leaf_discarded", ((
        "if(result.atlas.leaf_cells==0)", "if(result.atlas.leaf_cells<=1)"),)),
    ("fourth_cell_incidence_lost", ((
        "for(std::size_t quadrant=4;quadrant!=0;--quadrant)",
        "for(std::size_t quadrant=3;quadrant!=0;--quadrant)"),)),
)


def inputs(build):
    from run_q4_seed_cells_checks import SOURCES
    protocol.require(len(SOURCES) == 216 and str(Path(__file__).relative_to(ROOT)) in SOURCES,
                     "seed/cell mutation adapter missing from constructor34 inventory")
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
    from run_q4_seed_cells_checks import validate_gate
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
