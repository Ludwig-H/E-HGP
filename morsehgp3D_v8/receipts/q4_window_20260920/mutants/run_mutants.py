#!/usr/bin/env python3
"""Three compiled root-window mutations, isolated from the original source/build.

Explicit adaptation of receipts/q4_shallow_20260920/mutants/run_mutants.py;
previous captures/helper remain untouched. One gate, three causal changes,
not three independent oracles. Closed endpoints, fixed interiors and constant shell are changed separately.
Only an explicit geometric-oracle failure kills a mutant; ledger/floor failures,
compiler errors, crashes and arbitrary gate exceptions do not qualify.
"""
from __future__ import annotations

import argparse
import difflib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_p0_matrix import digest, invoke, on_signal, parse_result, require, utc_stamp, write_json  # noqa: E402
from run_q34_seed_checks import environment_record  # noqa: E402

GATE = "q4_window"
FAILURE_PREFIX = "q4 window gate: "
CAUSAL_MESSAGES = (
    "window sweep differs from complete rational ball/depth/support/shell oracle",
)
MUTATIONS = (
    ("point_window_dropped", "q4_window", (
        ("if (order>0)", "if (order>=0)"),
    )),
    ("fixed_interiors_dropped", "q4_window", (
        ("const auto base=constants+fixed_inside;", "const auto base=constants;"),
    )),
    ("constant_shell_dropped", "q4_window", (
        ("shell.push_back(id);counter_add(sweep.family.constant_on);",
         "counter_add(sweep.family.constant_on);"),
    )),
)


def mutation_plan():
    return json.loads(json.dumps(MUTATIONS))


def mutate(original, replacements, name):
    changed = original
    for before, after in replacements:
        require(changed.count(before) == 1, "mutation site is not unique: " + name)
        changed = changed.replace(before, after, 1)
    return changed


def pins(paths):
    return {name: digest(ROOT / name) for name in sorted(paths)}


def read_json(path):
    return parse_result(path.read_bytes())


def inputs(build):
    # Imported only once the main constructor has finished this tranche's
    # runner. This auxiliary experiment is itself outside that source inventory.
    from run_q4_window_checks import SOURCES
    files = {str((build / name).relative_to(ROOT)) for name in (
        "CMakeCache.txt", "libmhgp8_p0.a", "mhgp8_q4_window_gate",
        "CMakeFiles/mhgp8_q4_window_gate.dir/tests/q4_window_gate.cpp.o")}
    require(len(SOURCES) == 189, "expected the frozen 189-source tranche inventory")
    return SOURCES, files


def planned_commands(build, temporary, compiler):
    result = [("baseline_" + GATE, [str(build / f"mhgp8_{GATE}_gate"), "--selftest"], 0)]
    for name, _, _ in MUTATIONS:
        obj, binary = temporary / f"{name}.o", temporary / name
        result.extend([
            (name + "_compile", [compiler, "-std=c++20", "-O3", "-DNDEBUG", "-Wall", "-Wextra",
             "-Wpedantic", "-Werror", "-I" + str(ROOT / "morsehgp3D_v8/src"),
             "-c", str(temporary / f"{name}.cpp"), "-o", str(obj)], 0),
            (name + "_link", [compiler, str(build / f"CMakeFiles/mhgp8_{GATE}_gate.dir/tests/{GATE}_gate.cpp.o"),
             str(obj), str(build / "libmhgp8_p0.a"), "-pthread", "-o", str(binary)], 0),
            (name + "_oracle", [str(binary), "--selftest"], 1)])
    return result


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
    temporary = Path(tempfile.mkdtemp(prefix="mhgp8_q4_window_mutants_")).resolve()
    environment = dict(os.environ)
    manifest = dict(schema="mhgp8_q4_window_compiled_mutants_v1", started_utc=utc_stamp(),
        build=str(build), temporary=str(temporary), source_sha256=pins(sources),
        artifact_sha256=pins(artifacts), helper_sha256=digest(Path(__file__)),
        compiler=compiler, compiler_sha256=digest(Path(compiler)),
        commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        launch_command=[sys.executable, *sys.argv], environment=environment_record(environment),
        scope="three_causal_product_mutations_one_existing_gate", gcp_used=False,
        full_contract_qualified=False, public_status="not_claimed", mutations=mutation_plan(),
        causal_messages=list(CAUSAL_MESSAGES))
    manifest["planned_commands"] = planned_commands(build, temporary, compiler)
    write_json(capture / "MANIFEST.json", manifest)
    records, killed, evidence = [], [], []
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    status, error = "failed", None

    def execute(kind, command, expected=0, prefix=None):
        record = dict(kind=kind, command=command, cwd=str(ROOT), started_utc=utc_stamp(),
                      expected_exit_code=expected, status="failed", exit_code=None,
                      stdout="", stderr="", stdout_base64="", stderr_base64="")
        try:
            invoke(command, environment, ROOT, record, new_session=True)
            require(type(record["exit_code"]) is int and record["exit_code"] == expected,
                    "command returned the wrong exit code: " + kind)
            if prefix:
                require(record["stderr"].startswith(prefix) and
                        any(record["stderr"] == prefix + message + "\n" for message in CAUSAL_MESSAGES),
                        "mutation did not fail a causal oracle comparison")
            if kind.startswith("baseline_"):
                row = parse_result(record["stdout"].encode())
                require(row.get("status") == "passed", "baseline oracle failed")
            record["status"] = "passed"
        finally:
            record["finished_utc"] = utc_stamp()
            path = capture / f"record_{len(records):02}.json"
            write_json(path, record)
            records.append(dict(path=path.name, sha256=digest(path)))

    try:
        # No modified object is compiled until the unmodified gate passes.
        # The overriding object comes BEFORE the unchanged static archive;
        # its original member is never extracted by the linker.
        execute("baseline_" + GATE, [str(build / f"mhgp8_{GATE}_gate"), "--selftest"])
        for name, source, replacements in MUTATIONS:
            original_path = ROOT / f"morsehgp3D_v8/src/lanes/{source}.cpp"
            original = original_path.read_text()
            changed = mutate(original, replacements, name)
            snapshot = capture / f"{name}.original.cpp"
            modified = capture / f"{name}.mutated.cpp"
            patch = capture / f"{name}.patch"
            snapshot.write_text(original)
            modified.write_text(changed)
            patch.write_text("".join(difflib.unified_diff(original.splitlines(keepends=True),
                changed.splitlines(keepends=True), fromfile=str(original_path.relative_to(ROOT)),
                tofile=f"mutated/{source}.cpp")))
            local_source = temporary / f"{name}.cpp"
            local_source.write_text(changed)
            obj, binary = temporary / f"{name}.o", temporary / name
            compile_command = [compiler, "-std=c++20", "-O3", "-DNDEBUG", "-Wall", "-Wextra",
                "-Wpedantic", "-Werror", "-I" + str(ROOT / "morsehgp3D_v8/src"),
                "-c", str(local_source), "-o", str(obj)]
            execute(name + "_compile", compile_command)
            gate_object = build / f"CMakeFiles/mhgp8_{GATE}_gate.dir/tests/{GATE}_gate.cpp.o"
            execute(name + "_link", [compiler, str(gate_object), str(obj),
                str(build / "libmhgp8_p0.a"), "-pthread", "-o", str(binary)])
            # Record compiled evidence BEFORE invoking the mutant, so a
            # surviving/noncausal mutant also leaves its exact artifact pins.
            evidence.append(dict(name=name, original=snapshot.name, mutated=modified.name, patch=patch.name,
                original_sha256=digest(snapshot), mutated_sha256=digest(modified), patch_sha256=digest(patch),
                object=str(obj), object_sha256=digest(obj), binary=str(binary), binary_sha256=digest(binary)))
            execute(name + "_oracle", [str(binary), "--selftest"], expected=1, prefix=FAILURE_PREFIX)
            killed.append(name)
        require(pins(sources) == manifest["source_sha256"] and pins(artifacts) == manifest["artifact_sha256"] and
                digest(Path(__file__)) == manifest["helper_sha256"], "baseline sources/artifacts changed")
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
            helper_sha256_after=digest(Path(__file__)), compiler_sha256_after=digest(Path(compiler)))
        if completion["source_sha256_after"] != manifest["source_sha256"] or \
           completion["artifact_sha256_after"] != manifest["artifact_sha256"] or \
           completion["helper_sha256_after"] != manifest["helper_sha256"] or \
           completion["compiler_sha256_after"] != manifest["compiler_sha256"]:
            completion["status"] = "failed"
            completion["error"] = error or "closure hashes differ"
        write_json(capture / "COMPLETION.json", completion)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(capture), status=completion["status"], killed=killed, error=completion["error"])), flush=True)
    require(completion["status"] == "passed", "mutation capture did not close")


def read(capture, check_live=False):
    capture = capture.resolve()
    manifest, completion = read_json(capture / "MANIFEST.json"), read_json(capture / "COMPLETION.json")
    require(manifest["schema"] == "mhgp8_q4_window_compiled_mutants_v1" and completion["status"] == "passed" and
            completion["error"] is None and manifest["mutations"] == mutation_plan() and
            manifest["causal_messages"] == list(CAUSAL_MESSAGES),
            "mutation capture not successful or changed")
    require(completion["manifest_sha256"] == digest(capture / "MANIFEST.json") and
            completion["killed"] == [m[0] for m in MUTATIONS] and len(completion["records"]) == 10,
            "mutation plan or record count differs")
    sources, artifacts = inputs(Path(manifest["build"]))
    require(set(manifest["source_sha256"]) == sources and set(manifest["artifact_sha256"]) == artifacts,
            "mutation input inventory changed")
    expected_commands = planned_commands(Path(manifest["build"]), Path(manifest["temporary"]), manifest["compiler"])
    require(manifest["planned_commands"] == [[kind, command, expected] for kind, command, expected in expected_commands],
            "mutation command plan changed")
    for key in ("source_sha256", "artifact_sha256", "helper_sha256", "compiler_sha256"):
        require(manifest[key] == completion[key + "_after"], "mutation closure hashes differ")
    if check_live:
        require(pins(sources) == manifest["source_sha256"] and pins(artifacts) == manifest["artifact_sha256"] and
                digest(Path(__file__)) == manifest["helper_sha256"] and
                digest(Path(manifest["compiler"])) == manifest["compiler_sha256"],
                "live mutation inputs differ from capture")
    require(len(completion["evidence"]) == 3, "missing mutation evidence")
    for item, mutation in zip(completion["evidence"], MUTATIONS, strict=True):
        name, source, replacements = mutation
        require(item["name"] == name, "mutant evidence order differs")
        for field, suffix in (("original", "original.cpp"), ("mutated", "mutated.cpp"), ("patch", "patch")):
            require(item[field] == f"{name}.{suffix}" and digest(capture / item[field]) == item[field + "_sha256"],
                    "snapshot/patch hash mismatch")
        original = (capture / item["original"]).read_text()
        require(mutate(original, replacements, name) == (capture / item["mutated"]).read_text(),
                "mutation differs from its designated replacement sites")
        require(item["original_sha256"] == manifest["source_sha256"][f"morsehgp3D_v8/src/lanes/{source}.cpp"],
                "mutated source was not the pinned product source")
        require(item["binary"] == str(Path(manifest["temporary"]) / name) and
                item["object"] == str(Path(manifest["temporary"]) / (name + ".o")),
                "mutant artifacts differ from planned temporary targets")
        for field in ("binary", "object"):
            recorded_hash = item[field + "_sha256"]
            require(type(recorded_hash) is str and len(recorded_hash) == 64 and
                    all(character in "0123456789abcdef" for character in recorded_hash),
                    "invalid recorded mutant artifact hash")
            if check_live:
                require(digest(Path(item[field])) == recorded_hash, "temporary mutant artifact hash mismatch")
    for number, item in enumerate(completion["records"]):
        require(item["path"] == f"record_{number:02}.json" and digest(capture / item["path"]) == item["sha256"],
                "command record hash/order differs")
        record = read_json(capture / item["path"])
        kind, command, expected = expected_commands[number]
        require(record["status"] == "passed" and record["exit_code"] == expected and
                type(record["exit_code"]) is int and record["expected_exit_code"] == expected and
                record["kind"] == kind and record["command"] == command and record["cwd"] == str(ROOT),
                "command result differs")
        import base64
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(record[stream + "_base64"], validate=True).decode("utf-8", errors="replace") ==
                    record[stream], "raw command output differs")
        if number == 0:
            require(parse_result(record["stdout"].encode()).get("status") == "passed", "baseline gate did not pass")
        elif expected == 1:
            require(any(record["stderr"] == FAILURE_PREFIX + message + "\n" for message in CAUSAL_MESSAGES),
                    "noncausal mutant failure")
    return dict(status="passed", path=str(capture), baseline_gates=1, compiled_product_mutants=3,
                command_count=10, live_checked=check_live, full_contract_qualified=False, gcp_used=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--build", type=Path, required=True)
    capture.add_argument("--output", type=Path, default=HERE)
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    reader.add_argument("--check-live", action="store_true",
                        help="also require current inputs and temporary binaries/objects to match their recorded hashes")
    args = parser.parse_args()
    if args.operation == "run":
        run(args)
    else:
        print(json.dumps(read(args.path, args.check_live), sort_keys=True))


if __name__ == "__main__":
    main()
