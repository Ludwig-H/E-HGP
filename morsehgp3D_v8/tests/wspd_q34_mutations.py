#!/usr/bin/env python3
"""Compile three causal global-q34 mutations without changing product sources.

Explicit adaptation of the constructor30 compiled-mutation receipt helper.
One existing rational gate judges three changed product semantics; this is
not three independent oracles. A ledger error, crash, build failure, missing
non-vacuity floor or arbitrary exception never counts as a killed mutant.
Raw outputs, original/changed sources, command plans and closure hashes are
retained even when an attempt fails. No existing build is modified.
"""
from __future__ import annotations

import argparse
import base64
import difflib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_p0_matrix import digest, invoke, on_signal, parse_result, require, utc_stamp, write_json  # noqa: E402
from run_q34_seed_checks import environment_record  # noqa: E402

SCHEMA = "mhgp8_wspd_q34_compiled_mutants_v1"
GATE = "wspd_q34"
SOURCE = "morsehgp3D_v8/src/pipeline/wspd_q34.cpp"
FAILURE = "wspd q34 gate: global q34 stream differs from independent rational support/depth/shell oracle\n"
MUTATIONS = (
    ("q3_shell_counted_inside", (
        ("if (power < 0) {", "if (power <= 0) {"),
    )),
    ("q4_gated_by_q3_acceptance", (
        ("    observe(cover);\n    if (q3) {",
         "    observe(cover);\n    const auto q3_before = work.q3.emitted;\n    if (q3) {"),
        ("    if (q4) {", "    if (q4 && (!q3 || work.q3.emitted > q3_before)) {"),
    )),
    ("q4_gated_by_q3_front_mask", (
        ("    if (q4) {", "    if (q4 && (options_.requested_lane_mask != 6 || q3)) {"),
    )),
)


def mutation_plan():
    return json.loads(json.dumps(MUTATIONS))


def mutate(original, replacements, name):
    result = original
    for before, after in replacements:
        require(result.count(before) == 1, "mutation site is not unique: " + name)
        result = result.replace(before, after, 1)
    return result


def pins(paths):
    return {name: digest(ROOT / name) for name in sorted(paths)}


def read_json(path):
    return parse_result(path.read_bytes())


def inputs(build):
    from run_q34_lidar_checks import SOURCES
    require(len(SOURCES) == 196, "expected the constructor31 196-source inventory")
    require(str(Path(__file__).relative_to(ROOT)) in SOURCES, "mutation helper absent from inventory")
    artifacts = {str((build / name).relative_to(ROOT)) for name in (
        "CMakeCache.txt", "libmhgp8_p0.a", "mhgp8_wspd_q34_gate",
        "CMakeFiles/mhgp8_wspd_q34_gate.dir/tests/wspd_q34_gate.cpp.o")}
    return SOURCES, artifacts


def planned_commands(build, temporary, compiler):
    result = [("baseline", [str(build / "mhgp8_wspd_q34_gate"), "--selftest"], 0)]
    for name, _ in MUTATIONS:
        result.extend([
            (name + "_compile", [compiler, "-std=c++20", "-O3", "-DNDEBUG", "-Wall", "-Wextra",
             "-Wpedantic", "-Werror", "-I" + str(ROOT / "morsehgp3D_v8/src"),
             "-c", str(temporary / (name + ".cpp")), "-o", str(temporary / (name + ".o"))], 0),
            (name + "_link", [compiler,
             str(build / "CMakeFiles/mhgp8_wspd_q34_gate.dir/tests/wspd_q34_gate.cpp.o"),
             str(temporary / (name + ".o")), str(build / "libmhgp8_p0.a"),
             "-pthread", "-o", str(temporary / name)], 0),
            (name + "_oracle", [str(temporary / name), "--selftest"], 1),
        ])
    return result


def baseline(stdout):
    row = parse_result(stdout.encode())
    require(row.get("schema") == "mhgp8_wspd_q34_gate_v1" and row.get("status") == "PASS" and
            row.get("checks", 0) > 0 and row.get("parallel_pipeline_calls", 0) > 0,
            "unmodified rational gate did not pass")


def run(args):
    build = args.build.resolve()
    require(build.is_dir() and build.is_relative_to(ROOT / "build"), "existing local build required")
    sources, artifacts = inputs(build)
    cache = (build / "CMakeCache.txt").read_text()
    require("CMAKE_BUILD_TYPE:STRING=Release\n" in cache and "MHGP8_SANITIZE:BOOL=OFF\n" in cache,
            "mutations require the nonsanitized Release build")
    compilers = [line.split("=", 1)[1] for line in cache.splitlines()
                 if line.startswith(("CMAKE_CXX_COMPILER:FILEPATH=", "CMAKE_CXX_COMPILER:STRING="))]
    require(len(compilers) == 1, "ambiguous compiler in CMake cache")
    compiler = str(Path(compilers[0]).resolve())
    args.output.mkdir(parents=True, exist_ok=True)
    capture = Path(tempfile.mkdtemp(prefix="compiled_", dir=args.output)).resolve()
    temporary = Path(tempfile.mkdtemp(prefix="mhgp8_wspd_q34_mutants_")).resolve()
    environment = dict(os.environ)
    commands = planned_commands(build, temporary, compiler)
    manifest = dict(schema=SCHEMA, started_utc=utc_stamp(), build=str(build), temporary=str(temporary),
        source_sha256=pins(sources), artifact_sha256=pins(artifacts), helper_sha256=digest(Path(__file__)),
        compiler=compiler, compiler_sha256=digest(Path(compiler)),
        commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        launch_command=[sys.executable, *sys.argv], environment=environment_record(environment),
        scope="three_causal_product_mutations_one_rational_global_gate", gcp_used=False,
        full_contract_qualified=False, public_status="not_claimed", mutations=mutation_plan(),
        causal_failure=FAILURE, planned_commands=commands)
    write_json(capture / "MANIFEST.json", manifest)
    records, evidence, killed = [], [], []
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    status, error = "failed", None

    def execute(number):
        kind, command, expected = commands[number]
        record = dict(kind=kind, command=command, cwd=str(ROOT), started_utc=utc_stamp(),
                      expected_exit_code=expected, status="failed", exit_code=None,
                      stdout="", stderr="", stdout_base64="", stderr_base64="")
        try:
            invoke(command, environment, ROOT, record, new_session=True)
            require(type(record["exit_code"]) is int and record["exit_code"] == expected,
                    "command returned the wrong exit code: " + kind)
            if number == 0:
                baseline(record["stdout"])
            elif expected == 1:
                require(record["stderr"] == FAILURE,
                        "mutant did not fail the geometric global oracle comparison: " + kind)
            record["status"] = "passed"
        finally:
            record["finished_utc"] = utc_stamp()
            path = capture / f"record_{number:02}.json"
            write_json(path, record)
            records.append(dict(path=path.name, sha256=digest(path)))

    try:
        execute(0)
        original_path = ROOT / SOURCE
        original = original_path.read_text()
        for position, (name, replacements) in enumerate(MUTATIONS):
            changed = mutate(original, replacements, name)
            snapshot, modified, patch = (capture / (name + suffix) for suffix in
                                         (".original.cpp", ".mutated.cpp", ".patch"))
            snapshot.write_text(original)
            modified.write_text(changed)
            patch.write_text("".join(difflib.unified_diff(original.splitlines(keepends=True),
                changed.splitlines(keepends=True), fromfile=SOURCE, tofile="mutated/wspd_q34.cpp")))
            (temporary / (name + ".cpp")).write_text(changed)
            execute(1 + 3 * position)
            execute(2 + 3 * position)
            obj, binary = temporary / (name + ".o"), temporary / name
            evidence.append(dict(name=name, original=snapshot.name, mutated=modified.name, patch=patch.name,
                original_sha256=digest(snapshot), mutated_sha256=digest(modified), patch_sha256=digest(patch),
                object=str(obj), object_sha256=digest(obj), binary=str(binary), binary_sha256=digest(binary)))
            execute(3 + 3 * position)
            killed.append(name)
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        completion = dict(status=status, error=error, finished_utc=utc_stamp(), killed=killed,
            manifest_sha256=digest(capture / "MANIFEST.json"), records=records, evidence=evidence,
            source_sha256_after=pins(sources), artifact_sha256_after=pins(artifacts),
            helper_sha256_after=digest(Path(__file__)), compiler_sha256_after=digest(Path(compiler)),
            closing_errors=[])
        for key in ("source_sha256", "artifact_sha256", "helper_sha256", "compiler_sha256"):
            if completion[key + "_after"] != manifest[key]:
                completion["closing_errors"].append(key + " changed")
        if completion["closing_errors"]:
            completion["status"] = "failed"
            completion["error"] = error or "closure hashes differ"
        write_json(capture / "COMPLETION.json", completion)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(capture), status=completion["status"], killed=killed,
                              error=completion["error"])), flush=True)
    require(completion["status"] == "passed", "mutation capture did not close")


def read(capture, check_live=False):
    capture = capture.resolve()
    manifest, completion = read_json(capture / "MANIFEST.json"), read_json(capture / "COMPLETION.json")
    require(manifest["schema"] == SCHEMA and completion["status"] == "passed" and
            completion["error"] is None and completion["closing_errors"] == [] and
            manifest["mutations"] == mutation_plan() and manifest["causal_failure"] == FAILURE,
            "mutation capture not successful or plan changed")
    require(completion["manifest_sha256"] == digest(capture / "MANIFEST.json") and
            completion["killed"] == [item[0] for item in MUTATIONS] and len(completion["records"]) == 10,
            "mutation plan, manifest or record count differs")
    sources, artifacts = inputs(Path(manifest["build"]))
    require(set(manifest["source_sha256"]) == sources and set(manifest["artifact_sha256"]) == artifacts,
            "mutation input inventory changed")
    commands = planned_commands(Path(manifest["build"]), Path(manifest["temporary"]), manifest["compiler"])
    require(manifest["planned_commands"] == [[kind, command, expected] for kind, command, expected in commands],
            "mutation command plan changed")
    for key in ("source_sha256", "artifact_sha256", "helper_sha256", "compiler_sha256"):
        require(manifest[key] == completion[key + "_after"], "mutation closure hashes differ")
    if check_live:
        require(pins(sources) == manifest["source_sha256"] and pins(artifacts) == manifest["artifact_sha256"] and
                digest(Path(__file__)) == manifest["helper_sha256"] and
                digest(Path(manifest["compiler"])) == manifest["compiler_sha256"],
                "live mutation inputs differ")
    require(len(completion["evidence"]) == 3, "missing compiled evidence")
    for item, mutation in zip(completion["evidence"], MUTATIONS, strict=True):
        name, replacements = mutation
        require(item["name"] == name, "mutant evidence order differs")
        for field, suffix in (("original", ".original.cpp"), ("mutated", ".mutated.cpp"), ("patch", ".patch")):
            require(item[field] == name + suffix and digest(capture / item[field]) == item[field + "_sha256"],
                    "snapshot/patch hash mismatch")
        original = (capture / item["original"]).read_text()
        changed = (capture / item["mutated"]).read_text()
        require(mutate(original, replacements, name) == changed and
                item["original_sha256"] == manifest["source_sha256"][SOURCE],
                "mutation differs from designated pinned source")
        patch = "".join(difflib.unified_diff(original.splitlines(keepends=True), changed.splitlines(keepends=True),
                       fromfile=SOURCE, tofile="mutated/wspd_q34.cpp"))
        require((capture / item["patch"]).read_text() == patch, "mutation patch content differs")
        for field, suffix in (("binary", ""), ("object", ".o")):
            require(item[field] == str(Path(manifest["temporary"]) / (name + suffix)),
                    "unexpected compiled artifact target")
            sha = item[field + "_sha256"]
            require(type(sha) is str and len(sha) == 64 and all(c in "0123456789abcdef" for c in sha),
                    "invalid compiled artifact hash")
            if check_live:
                require(digest(Path(item[field])) == sha, "temporary compiled artifact differs")
    for number, item in enumerate(completion["records"]):
        require(item["path"] == f"record_{number:02}.json" and digest(capture / item["path"]) == item["sha256"],
                "command record hash/order differs")
        record = read_json(capture / item["path"])
        kind, command, expected = commands[number]
        require(record["status"] == "passed" and type(record["exit_code"]) is int and
                record["exit_code"] == expected and record["expected_exit_code"] == expected and
                record["kind"] == kind and record["command"] == command and record["cwd"] == str(ROOT),
                "command result differs")
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(record[stream + "_base64"], validate=True).decode("utf-8", errors="replace") ==
                    record[stream], "raw command output differs")
        if number == 0:
            baseline(record["stdout"])
        elif expected == 1:
            require(record["stderr"] == FAILURE, "noncausal mutant failure")
    return dict(status="passed", path=str(capture), baseline_gates=1, compiled_product_mutants=3,
                command_count=10, source_count=len(sources), live_checked=check_live,
                full_contract_qualified=False, gcp_used=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--build", type=Path, required=True)
    capture.add_argument("--output", type=Path,
                         default=ROOT / "morsehgp3D_v8/receipts/lidar_global_20260921/mutations")
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    reader.add_argument("--check-live", action="store_true")
    args = parser.parse_args()
    if args.operation == "run":
        run(args)
    else:
        print(json.dumps(read(args.path, args.check_live), sort_keys=True))


if __name__ == "__main__":
    main()
