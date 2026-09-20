#!/usr/bin/env python3
"""Four compiled product mutations, isolated from the original source/build."""
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

MUTATIONS = (
    ("q4_wrong_orientation", "exact_ball", "exact_ball",
     "for (auto& coefficient : numerator) coefficient = -coefficient;",
     "static_cast<void>(numerator);  // MUTANT: orientation not applied to linear term",
     "exact ball gate:"),
    ("q4_boundary_origin_admitted", "exact_ball", "exact_ball",
     "if (remaining <= 0) return std::nullopt;",
     "if (remaining < 0) return std::nullopt;  // MUTANT: zero barycentric weight admitted",
     "exact ball gate:"),
    ("q3_shell_counted_inside", "q34_seed", "q34_seed",
     "if (power < 0) {", "if (power <= 0) {  // MUTANT: shell credited as strict interior",
     "q34 seed gate:"),
    ("q4_stop_at_first_unowned", "q34_seed", "q34_seed",
     "counter_add(work.q4_owner_rejections);\n        continue;",
     "counter_add(work.q4_owner_rejections);\n        return;  // MUTANT: later root presentations lost",
     "q34 seed gate:"),
)


def pins(paths):
    return {name: digest(ROOT / name) for name in sorted(paths)}


def read_json(path):
    return parse_result(path.read_bytes())


def inputs(build):
    # Imported only once the main constructor has finished this tranche's
    # runner. This auxiliary experiment is itself outside that source inventory.
    from run_q34_seed_checks import SOURCES
    files = {str((build / name).relative_to(ROOT)) for name in (
        "CMakeCache.txt", "libmhgp8_p0.a", "mhgp8_exact_ball_gate", "mhgp8_q34_seed_gate",
        "CMakeFiles/mhgp8_exact_ball_gate.dir/tests/exact_ball_gate.cpp.o",
        "CMakeFiles/mhgp8_q34_seed_gate.dir/tests/q34_seed_gate.cpp.o")}
    require(len(SOURCES) == 144, "expected the frozen 144-source tranche inventory")
    return SOURCES, files


def planned_commands(build, temporary, compiler):
    result = [("baseline_" + gate, [str(build / f"mhgp8_{gate}_gate"), "--selftest"], 0)
              for gate in ("exact_ball", "q34_seed")]
    for name, _, gate, _, _, _ in MUTATIONS:
        obj, binary = temporary / f"{name}.o", temporary / name
        result.extend([
            (name + "_compile", [compiler, "-std=c++20", "-O3", "-DNDEBUG", "-Wall", "-Wextra",
             "-Wpedantic", "-Werror", "-I" + str(ROOT / "morsehgp3D_v8/src"),
             "-c", str(temporary / f"{name}.cpp"), "-o", str(obj)], 0),
            (name + "_link", [compiler, str(build / f"CMakeFiles/mhgp8_{gate}_gate.dir/tests/{gate}_gate.cpp.o"),
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
    temporary = Path(tempfile.mkdtemp(prefix="mhgp8_q34_mutants_")).resolve()
    manifest = dict(schema="mhgp8_q34_compiled_mutants_v1", started_utc=utc_stamp(),
        build=str(build), temporary=str(temporary), source_sha256=pins(sources),
        artifact_sha256=pins(artifacts), helper_sha256=digest(Path(__file__)),
        compiler=compiler, compiler_sha256=digest(Path(compiler)),
        commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        scope="four_causal_product_mutations_not_four_independent_oracles", gcp_used=False,
        full_contract_qualified=False, public_status="not_claimed", mutations=[list(m) for m in MUTATIONS])
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
            invoke(command, dict(os.environ), ROOT, record, new_session=True)
            require(type(record["exit_code"]) is int and record["exit_code"] == expected,
                    "command returned the wrong exit code: " + kind)
            if prefix:
                require(record["stderr"].startswith(prefix) and "nonvacuity" not in record["stderr"],
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
        # Do NOT compile or run any mutated object unless both unmodified gates
        # have just passed. Each mutant links one overriding object BEFORE the
        # unchanged static archive: its original member is never extracted.
        for gate in ("exact_ball", "q34_seed"):
            execute("baseline_" + gate, [str(build / f"mhgp8_{gate}_gate"), "--selftest"])
        for name, source, gate, before, after, prefix in MUTATIONS:
            original_path = ROOT / f"morsehgp3D_v8/src/lanes/{source}.cpp"
            original = original_path.read_text()
            require(original.count(before) == 1, "mutation site is not unique: " + name)
            changed = original.replace(before, after, 1)
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
            gate_object = build / f"CMakeFiles/mhgp8_{gate}_gate.dir/tests/{gate}_gate.cpp.o"
            execute(name + "_link", [compiler, str(gate_object), str(obj),
                str(build / "libmhgp8_p0.a"), "-pthread", "-o", str(binary)])
            execute(name + "_oracle", [str(binary), "--selftest"], expected=1, prefix=prefix)
            killed.append(name)
            evidence.append(dict(name=name, original=snapshot.name, mutated=modified.name, patch=patch.name,
                original_sha256=digest(snapshot), mutated_sha256=digest(modified), patch_sha256=digest(patch),
                object=str(obj), object_sha256=digest(obj), binary=str(binary), binary_sha256=digest(binary)))
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
    require(manifest["schema"] == "mhgp8_q34_compiled_mutants_v1" and completion["status"] == "passed" and
            completion["error"] is None and manifest["mutations"] == [list(m) for m in MUTATIONS],
            "mutation capture not successful or changed")
    require(completion["manifest_sha256"] == digest(capture / "MANIFEST.json") and
            completion["killed"] == [m[0] for m in MUTATIONS] and len(completion["records"]) == 14,
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
    require(len(completion["evidence"]) == 4, "missing mutation evidence")
    for item, mutation in zip(completion["evidence"], MUTATIONS, strict=True):
        name, source, _, before, after, _ = mutation
        require(item["name"] == name, "mutant evidence order differs")
        for field, suffix in (("original", "original.cpp"), ("mutated", "mutated.cpp"), ("patch", "patch")):
            require(item[field] == f"{name}.{suffix}" and digest(capture / item[field]) == item[field + "_sha256"],
                    "snapshot/patch hash mismatch")
        original = (capture / item["original"]).read_text()
        require(original.count(before) == 1 and original.replace(before, after, 1) == (capture / item["mutated"]).read_text(),
                "mutation changed more than its one designated site")
        require(item["original_sha256"] == manifest["source_sha256"][f"morsehgp3D_v8/src/lanes/{source}.cpp"],
                "mutated source was not the pinned product source")
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
        if number < 2:
            require(parse_result(record["stdout"].encode()).get("status") == "passed", "baseline gate did not pass")
        elif expected == 1:
            require(record["stderr"].startswith(MUTATIONS[(number - 2) // 3][5]) and
                    "nonvacuity" not in record["stderr"], "noncausal mutant failure")
    return dict(status="passed", path=str(capture), baseline_gates=2, compiled_product_mutants=4,
                command_count=14, live_checked=check_live, full_contract_qualified=False, gcp_used=False)


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
