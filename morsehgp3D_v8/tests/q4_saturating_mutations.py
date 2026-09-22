#!/usr/bin/env python3
"""Bounded atlas mutations: three isolated object replacements, no rebuild.

Explicit reuse of wspd_q34_mutations' raw-command/snapshot/closure protocol.
Each mutant has its own four-command capture (baseline, compile, link, gate),
because partition and atlas belong to different translation units. The first
two failures judge a geometric certificate; the last intentionally judges
the exact physical-work ledger, and is NOT a geometric mutation claim.
All CMake dependency files of the linked library/gate and their dependencies
are pinned, including system headers. Headers are hashed, never copied.
Historical reading still needs the original CMake dependency inventory; LIVE
also checks sources, objects, library, compiler and binaries at both ends.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import tempfile

import wspd_q34_mutations as protocol
from run_u18_resume_checks import dep_paths, sources

ROOT = protocol.ROOT
GATE = "q4_saturating_atlas"
HERE = Path(__file__).resolve()
PREFIX = "q4 saturating atlas gate: "
CASES = {
    "threshold_one_early": (
        "morsehgp3D_v8/src/lanes/q4_local_partition.cpp",
        (("if(stop_after!=0 && inside_count_>=stop_after)",
          "if(stop_after!=0 && inside_count_>=stop_after-1)"),),
        PREFIX + "partial frontier escaped or threshold changed\n", "geometry"),
    "contact_credited_inside": (
        "morsehgp3D_v8/src/lanes/q4_local_partition.cpp",
        (("if (bound.maximum<0)", "if (bound.maximum<=0)"),),
        PREFIX + "terminal certificate fails independent closed-cell oracle\n", "geometry"),
    "prefix_work_omitted": (
        "morsehgp3D_v8/src/lanes/q4_local.cpp",
        (("    merge(work.partition,result.work());",
          "    // MUTANT: interrupted prefix work omitted from physical total."),),
        # Earlier multi-factory fixtures already refute the omitted work:
        # paid prefix tests exceed the alleged total. Require this EXACT
        # physical-ledger failure, not any exception or missing floor. The
        # later single-factory equality remains in the unchanged native gate.
        PREFIX + "prefix is not a subset of total work\n", "physical_work"),
}


def inputs(build):
    local = {str(p.relative_to(ROOT)) for p in sources() | {HERE}}
    dependency_files = set((build / "CMakeFiles/mhgp8_p0.dir").rglob("*.o.d"))
    dependency_files.add(build / f"CMakeFiles/mhgp8_{GATE}_gate.dir/tests/{GATE}_gate.cpp.o.d")
    protocol.require(bool(dependency_files), "missing library dependency files")
    dependencies = set(dependency_files)
    for path in dependency_files:
        protocol.require(path.is_file(), "missing compiled dependency inventory: " + str(path))
        dependencies |= dep_paths(path, build)
    artifacts = {build / name for name in (
        "CMakeCache.txt", "libmhgp8_p0.a", f"mhgp8_{GATE}_gate",
        f"CMakeFiles/mhgp8_{GATE}_gate.dir/tests/{GATE}_gate.cpp.o")}
    # Absolute dependency names are deliberate: ROOT / absolute == absolute.
    return local, {str(p) for p in artifacts | dependencies}


def commands(build, temporary, compiler):
    name = protocol.MUTATIONS[0][0]
    return [
        ("baseline", [str(build / f"mhgp8_{GATE}_gate"), "--selftest"], 0),
        (name + "_compile", [compiler, "-std=c++20", "-O3", "-DNDEBUG", "-Wall", "-Wextra",
         "-Wpedantic", "-Werror", "-I" + str(ROOT / "morsehgp3D_v8/src"),
         "-I" + str(ROOT / "morsehgp3D_v8/src/lanes"), "-c", str(temporary / (name + ".cpp")),
         "-o", str(temporary / (name + ".o"))], 0),
        (name + "_link", [compiler,
         str(build / f"CMakeFiles/mhgp8_{GATE}_gate.dir/tests/{GATE}_gate.cpp.o"),
         str(temporary / (name + ".o")), str(build / "libmhgp8_p0.a"),
         "-pthread", "-o", str(temporary / name)], 0),
        (name + "_oracle", [str(temporary / name), "--selftest"], 1),
    ]


def baseline(stdout):
    row = protocol.parse_result(stdout.encode())
    protocol.require(row.get("schema") == "mhgp8_q4_saturating_atlas_gate_v1" and
                     row.get("status") == "passed", "atlas baseline did not pass")
    for field in ("checks", "partitions", "certificates", "interrupted", "exact_results",
                  "point_stops", "block_stops", "edge_calls", "outputs", "atlas_certificates", "q3_locations"):
        protocol.require(type(row.get(field)) is int and row[field] > 0, "missing baseline exercise: " + field)
    protocol.require(row.get("max_shell", 0) >= 30 and row.get("parallel_calls") == 4 and
                     row.get("callback_failures") == 1, "baseline lifecycle/shell mismatch")


def configure(name):
    source, replacements, failure, kind = CASES[name]
    protocol.SCHEMA = "mhgp8_q4_saturating_mutant_v1_" + name
    protocol.SCOPE = "one_atlas_" + kind + "_mutation_one_independent_gate"
    protocol.SOURCE = source
    protocol.FAILURE = failure
    protocol.EXPECTED_FAILURE = {}
    protocol.MUTATIONS = ((name, replacements),)
    protocol.inputs, protocol.planned_commands, protocol.baseline = inputs, commands, baseline


def read(path, check_live=False):
    path = path.resolve()
    before = protocol.pins({str(p) for p in path.iterdir() if p.is_file()})
    manifest = protocol.read_json(path / "MANIFEST.json")
    mutations = manifest.get("mutations")
    protocol.require(type(mutations) is list and len(mutations) == 1 and mutations[0][0] in CASES,
                     "unknown atlas mutation")
    name = mutations[0][0]
    configure(name)
    result = protocol.read(path, check_live)
    if check_live:
        local, artifacts = inputs(Path(manifest["build"]))
        protocol.require(protocol.pins(local) == manifest["source_sha256"] and
                         protocol.pins(artifacts) == manifest["artifact_sha256"] and
                         protocol.digest(Path(protocol.__file__)) == manifest["helper_sha256"] and
                         protocol.digest(Path(manifest["compiler"])) == manifest["compiler_sha256"],
                         "live inputs changed during mutation reading")
        completion = protocol.read_json(path / "COMPLETION.json")
        for evidence in completion["evidence"]:
            for field in ("object", "binary"):
                protocol.require(protocol.digest(Path(evidence[field])) == evidence[field + "_sha256"],
                                 "compiled evidence changed during reading")
    protocol.require(protocol.pins(set(before)) == before and
                     set(before) == {str(p) for p in path.iterdir() if p.is_file()},
                     "capture changed during reading")
    return dict(result, mutation=name, judgment=CASES[name][3])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    run = sub.add_parser("run")
    run.add_argument("--build", type=Path, required=True)
    run.add_argument("--output", type=Path, required=True)
    run.add_argument("--mutation", choices=["all", *CASES], default="all")
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    reader.add_argument("--check-live", action="store_true")
    args = parser.parse_args()
    if args.operation == "read":
        print(json.dumps(read(args.path, args.check_live), sort_keys=True))
        return
    args.output.mkdir(parents=True, exist_ok=True)
    names = list(CASES) if args.mutation == "all" else [args.mutation]
    reports = []
    for name in names:
        configure(name)
        # Unique parent makes even a failed attempt unambiguous and retained.
        destination = Path(tempfile.mkdtemp(prefix=name + "_", dir=args.output)).resolve()
        protocol.run(argparse.Namespace(build=args.build, output=destination))
        captures = list(destination.glob("compiled_*"))
        protocol.require(len(captures) == 1, "ambiguous mutation capture")
        reports.append(read(captures[0], True))
    print(json.dumps(dict(status="passed", captures=reports, geometry_mutants=sum(
        row["judgment"] == "geometry" for row in reports), physical_work_mutants=sum(
        row["judgment"] == "physical_work" for row in reports)), sort_keys=True))


if __name__ == "__main__":
    main()
