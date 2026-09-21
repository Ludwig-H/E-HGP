#!/usr/bin/env python3
"""Three compiled witness-filter mutations, new oracle and frozen inventory.

Explicit reuse of constructor31's ten-command capture/closure/read protocol.
The adapter replaces its source, compiler commands, baseline judge, expected
causal failure and inventory. Historical31 captures and defaults are unchanged.
Both this adapter AND that protocol helper belong to the new source inventory.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import wspd_q34_mutations as protocol

ROOT = protocol.ROOT
GATE = "q34_witness_search"
WITNESS_MUTATIONS = (
    ("q4_wrong_alpha", (("lane == 0 ? 3 : 2", "lane == 0 ? 3 : 3"),)),
    ("lemon_contact_counted_inside", (("alpha * h4_squared <= xi16", "alpha * h4_squared < xi16"),)),
    ("admitted_lane_recounted_in_children", (
        ("        frame.mask &= static_cast<std::uint8_t>(~bit);", "        /* mutant: leave admitted lane in children */"),)),
)
CENSUS_MUTATIONS = (
    ("integer_minimum_wrong_rounding", (("std::min(at_floor, at_ceil)", "std::max(at_floor, at_ceil)"),)),
    ("whole_contact_counted_inside", (("if (frame.bounds.maximum < 0) {", "if (frame.bounds.maximum <= 0) {"),)),
    ("shell_contact_excluded", (("frame.bounds.minimum > 0 || frame.bounds.maximum < 0",
                                 "frame.bounds.minimum >= 0 || frame.bounds.maximum < 0"),)),
)


def configure(kind):
    global GATE
    protocol.require(kind in ("witness", "census"), "invalid mutation family")
    GATE = "q34_witness_search" if kind == "witness" else "q3_ball_census"
    protocol.SCHEMA = f"mhgp8_q34_indexed_{kind}_mutants_v1"
    protocol.SCOPE = f"three_causal_{kind}_mutations_independent_geometric_oracle"
    protocol.SOURCE = f"morsehgp3D_v8/src/lanes/{GATE}.cpp"
    protocol.FAILURE = ("q34 witness search gate: singleton witness decision differs from independent oracle\n"
        if kind == "witness" else "q3 ball census gate: global census differs from independent rational depth and shell oracle\n")
    protocol.MUTATIONS = WITNESS_MUTATIONS if kind == "witness" else CENSUS_MUTATIONS


def inputs(build):
    from run_q34_indexed_checks import SOURCES
    protocol.require(str(Path(__file__).relative_to(ROOT)) in SOURCES,
                     "indexed mutation adapter absent from inventory")
    artifacts = {str((build / name).relative_to(ROOT)) for name in (
        "CMakeCache.txt", "libmhgp8_p0.a", f"mhgp8_{GATE}_gate",
        f"CMakeFiles/mhgp8_{GATE}_gate.dir/tests/{GATE}_gate.cpp.o")}
    return SOURCES, artifacts


def planned_commands(build, temporary, compiler):
    commands = [("baseline", [str(build / f"mhgp8_{GATE}_gate"), "--selftest"], 0)]
    for name, _ in protocol.MUTATIONS:
        commands.extend([
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
    return commands


def baseline(stdout):
    row = protocol.parse_result(stdout.encode())
    if GATE == "q3_ball_census":
        protocol.require(row.get("schema") == "mhgp8_q3_ball_census_gate_v1" and row.get("status") == "PASS" and
                         row.get("ceil_required", 0) > 0 and row.get("floor_required", 0) > 0 and
                         row.get("accepted", 0) > 0 and row.get("rejected", 0) > 0 and row.get("max_shell", 0) >= 30,
                         "unmodified census oracle did not pass nonvacuously")
        return
    protocol.require(row.get("schema") == "mhgp8_q34_witness_search_gate_v1" and row.get("status") == "PASS" and
                     row.get("singleton_queries", 0) > 0 and row.get("partial_admission_cases", 0) > 0 and
                     row.get("wrong_alpha_cases", 0) > 0 and row.get("q3_contacts", 0) > 0 and
                     row.get("q4_contacts", 0) > 0, "unmodified witness oracle did not pass nonvacuously")


protocol.inputs = inputs
protocol.planned_commands = planned_commands
protocol.baseline = baseline


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--build", type=Path, required=True)
    capture.add_argument("--output", type=Path, required=True)
    capture.add_argument("--kind", choices=("witness", "census"), required=True)
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    reader.add_argument("--check-live", action="store_true")
    args = parser.parse_args()
    if args.operation == "run":
        configure(args.kind)
        protocol.run(args)
    else:
        schema = protocol.read_json(args.path / "MANIFEST.json")["schema"]
        kinds = {f"mhgp8_q34_indexed_{kind}_mutants_v1": kind for kind in ("witness", "census")}
        protocol.require(schema in kinds, "unrecognized indexed mutation capture")
        configure(kinds[schema])
        print(json.dumps(protocol.read(args.path, args.check_live), sort_keys=True))


if __name__ == "__main__":
    main()
