#!/usr/bin/env python3
"""Fresh CMake/CTest closure for the u18 resumption; local LIVE evidence, not FULL.

All launches (including failures) use the existing interruption-safe collector.
No build is reused. Compiled dependency files bind actual headers, including
Boost, to the capture. A read never recompiles or changes a pinned build.
"""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import math
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
import xml.etree.ElementTree as ET

from run_p0_matrix import invoke, on_signal, parse_result, require, utc_stamp, write_json

ROOT = Path(__file__).resolve().parents[2]
V8 = ROOT / "morsehgp3D_v8"
SCHEMA = "mhgp8_u18_resume_checks_v1"
EXTRA = ("bench/run_u18_resume_checks.py", "tests/u18_numeric_domain_gate.cpp",
         "tests/q4_saturating_atlas_gate.cpp", "tests/ground_baseline_test.py",
         "tests/u18_resume_checks_test.py", "tests/q4_saturating_mutations.py")
DISABLED = {"mhgp8_q34_spatial_gate", "mhgp8_q34_spatial_gate_optimized",
            "mhgp8_q34_indexed_witness_mutations"}
SAN_DISABLED = DISABLED | {"mhgp8_q34_affine_mutations", "mhgp8_q4_seed_cells_mutations",
    "mhgp8_wspd_q34_mutations", "mhgp8_q34_indexed_census_mutations", "mhgp8_q4_saturating_mutations"}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def pins(paths):
    return {str(p): sha(p) for p in sorted(set(map(Path, paths)))}


def sources():
    names = subprocess.check_output(["git", "ls-files", "-z", "--",
        "morsehgp3D_v8/src", "morsehgp3D_v8/tests", "morsehgp3D_v8/oracle",
        "morsehgp3D_v8/bench", "morsehgp3D_v8/cmake", "morsehgp3D_v8/CMakeLists.txt"], cwd=ROOT)
    return {ROOT / p.decode() for p in names.split(b"\0") if p} | {V8 / p for p in EXTRA}


def load(path):
    return parse_result(Path(path).read_bytes())


def dep_paths(path, directory):
    tokens = shlex.split(path.read_text().replace("\\\n", " ").split(":", 1)[1])
    return {(directory / token).resolve(strict=True) for token in tokens}


def scan_plan(build, compiler, output):
    entries = json.loads((build / "compile_commands.json").read_text())
    require(isinstance(entries, list) and entries, "empty compile command inventory")
    result = []
    for number, entry in enumerate(entries):
        argv = entry.get("arguments") or shlex.split(entry["command"])
        require(argv and Path(argv[0]).samefile(compiler), "unexpected compile command driver")
        require(argv.count("-c") == 1 and argv.count("-o") == 1, "unsupported compile command")
        command, position = [argv[0]], 1
        while position < len(argv):
            token = argv[position]
            if token in ("-o", "-MF", "-MT", "-MQ"):
                require(position + 1 < len(argv), "missing compiler option argument")
                position += 2
            elif token in ("-c", "-MD", "-MMD", "-MP"):
                position += 1
            else:
                require(not token.startswith("@"), "response files require an explicit dependency plan")
                command.append(token); position += 1
        name = f"unit_{number:04d}"
        command.extend(["-M", "-MF", str(output / (name + ".d")), "-MT", "mhgp8_dependency_target"])
        result.append(dict(name=name, command=command, cwd=str(Path(entry["directory"]).resolve()),
                           source=str((Path(entry["directory"]) / entry["file"]).resolve(strict=True))))
    return result


def scan(build, compiler, output):
    require(not output.exists(), "fresh dependency scan required")
    output.mkdir(parents=True)
    state = dict(status="running", compiler_sha256=sha(compiler),
                 compile_commands_sha256=sha(build / "compile_commands.json"), commands=[], dependencies={})
    write_json(output / "SCAN.json", state)
    try:
        for item in scan_plan(build, compiler, output):
            record = dict(item, started=utc_stamp())
            record_path = output / (item["name"] + ".json")
            try:
                # Internal scan belongs to the outer collector's process group.
                # The outer scan command cancels/reaps this entire group.
                invoke(item["command"], dict(os.environ), Path(item["cwd"]), record)
            finally:
                record["finished"] = utc_stamp()
                write_json(record_path, record)
                state["commands"].append(dict(name=item["name"], sha256=sha(record_path)))
                write_json(output / "SCAN.json", state)
            require(record.get("exit_code") == 0, "dependency preprocessing failed")
            require(sha(compiler) == state["compiler_sha256"], "compiler changed during dependency scan")
            current = pins(dep_paths(output / (item["name"] + ".d"), Path(item["cwd"])))
            for path, digest in current.items():
                require(path not in state["dependencies"] or state["dependencies"][path] == digest,
                        "dependency changed between scan units")
                state["dependencies"][path] = digest
        require(pins(state["dependencies"]) == state["dependencies"], "dependency scan closure changed")
        require(sha(build / "compile_commands.json") == state["compile_commands_sha256"], "compile commands changed")
        state["status"] = "passed"
    except BaseException as error:
        state.update(status="failed", error=repr(error))
        raise
    finally:
        state["artifacts"] = pins(p for p in output.iterdir() if p.name != "SCAN.json")
        write_json(output / "SCAN.json", state)
    return dict(status="passed", units=len(state["commands"]), dependencies=len(state["dependencies"]))


def check_record(record, command):
    require(record["command"] == command and type(record["exit_code"]) is int and record["exit_code"] == 0,
            "command failed or different")
    for stream in ("stdout", "stderr"):
        require(base64.b64decode(record[stream+"_base64"], validate=True).decode("utf-8", errors="replace") == record[stream],
                "raw stream mismatch")


def read_scan(output, build, compiler):
    state = load(output / "SCAN.json")
    require(state["status"] == "passed", "dependency scan not passed")
    require(sha(compiler) == state["compiler_sha256"], "dependency scan compiler changed")
    require(sha(build / "compile_commands.json") == state["compile_commands_sha256"], "compile commands changed")
    require(pins(p for p in output.iterdir() if p.name != "SCAN.json") == state["artifacts"], "scan artifacts changed")
    plan = scan_plan(build, compiler, output)
    require(len(plan) == len(state["commands"]), "dependency scan unit inventory differs")
    dependencies = set()
    for item, recorded in zip(plan, state["commands"]):
        path = output / (item["name"] + ".json")
        require(recorded == dict(name=item["name"], sha256=sha(path)), "scan command identity differs")
        record = load(path)
        check_record(record, item["command"])
        require(record["cwd"] == item["cwd"] and record["source"] == item["source"], "scan unit provenance differs")
        dependencies.update(dep_paths(output / (item["name"] + ".d"), Path(item["cwd"])))
    require(dependencies and pins(dependencies) == state["dependencies"], "LIVE precompile dependency inventory differs")
    return state["dependencies"]


def compiled(build):
    dependencies = set()
    outputs = []
    for dep in (build / "CMakeFiles").rglob("*.o.d"):
        outputs.append(dep)
        dependencies.update(dep_paths(dep, build))
    require(dependencies, "empty compiler dependency closure")
    outputs.extend(p for p in build.glob("mhgp8_*") if p.is_file())
    outputs.extend(build.glob("*.a"))
    outputs.extend((build / "CMakeCache.txt", build / "CTestTestfile.cmake", build / "compile_commands.json"))
    outputs.extend(build.rglob("flags.make"))
    outputs.extend(build.rglob("link.txt"))
    return dict(dependencies=pins(dependencies), outputs=pins(outputs))


def judge_xml(path, expected, sanitize=False):
    tree = ET.parse(path)
    cases = tree.findall(".//testcase")
    require(len(cases) >= 132, "incomplete CTest registration (Boost judges required)")
    names = [x.attrib.get("name") for x in cases]
    require(len(set(names)) == len(names) and sorted(names) == sorted(expected), "CTest inventory differs or repeats tests")
    require({"mhgp8_u18_numeric_domain_gate", "mhgp8_q4_saturating_atlas_gate"} <= set(names), "new native gates missing")
    failed = [x.attrib.get("name") for x in cases if x.find("failure") is not None or x.find("error") is not None]
    skipped = {x.attrib.get("name") for x in cases if x.find("skipped") is not None}
    require(not failed, f"failed CTests: {failed}")
    allowed = SAN_DISABLED if sanitize else DISABLED
    require(skipped == allowed, f"disabled CTests differ: {skipped ^ allowed}")
    return dict(registered=len(cases), passed=len(cases)-len(skipped), disabled=sorted(skipped))


def command_plan(build, capture, compiler, sanitize, jobs, boost):
    require(type(sanitize) is bool and type(jobs) is int and jobs > 0, "invalid build settings")
    config = ["cmake", "-S", str(V8), "-B", str(build), "-DBUILD_TESTING=ON",
              "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
              "-DCMAKE_BUILD_TYPE=" + ("RelWithDebInfo" if sanitize else "Release"),
              "-DCMAKE_CXX_COMPILER=" + str(compiler), "-DBOOST_ROOT=" + str(boost),
              "-DMHGP8_SANITIZE=" + ("ON" if sanitize else "OFF")]
    if sanitize:
        config.append("-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-O1 -g -DNDEBUG")
    plan = [["compiler", [str(compiler), "--version"]], ["configure", config],
            ["scan", [sys.executable, str(Path(__file__).resolve()), "scan", "--build", str(build),
                      "--compiler", str(compiler), "--output", str(capture / "dependencies")]],
            ["build", ["cmake", "--build", str(build), "--parallel", str(jobs)]],
            ["inventory", ["ctest", "--test-dir", str(build), "--show-only=json-v1"]],
            ["ctest", ["ctest", "--test-dir", str(build), "--output-on-failure", "-j", "1",
                       "--output-junit", str(capture / "CTEST.xml")]]]
    for n in ([257] if sanitize else [8000,16000,32000]):
        plan.append([f"scale_{n}", [str(build / "mhgp8_q4_saturating_atlas_gate"), "--scale", str(n)]])
    return plan


def judge_scale(row, n):
    require(row["schema"] == "mhgp8_q4_saturating_atlas_scale_v1" and
            row["scope"] == "one_supplied_edge_empty_q4_stream_not_pipeline", "invalid scale scope")
    integer_fields = ("n", "kmax", "outputs", "baseline_tests", "saturated_total_tests",
        "baseline_node_visits", "saturated_total_node_visits", "baseline_frontier_copies",
        "saturated_total_frontier_copies", "unvisited_site_mass", "certificates",
        "baseline_peak_build_bytes", "saturated_peak_build_bytes")
    for name in integer_fields:
        require(type(row[name]) is int and row[name] >= 0, f"invalid scale integer {name}")
    require(row["n"] == n and row["kmax"] == 10 and row["outputs"] == 0 and
            row["certificates"] == 1 and 0 < row["unvisited_site_mass"] <= n, "scale fixture differs")
    for prefix in ("baseline", "saturated_total"):
        require(row[prefix+"_tests"] > 0 and row[prefix+"_node_visits"] >= row[prefix+"_tests"], "empty scale work")
    for name in ("prepare_ms", "baseline_ms", "saturated_ms"):
        require(type(row[name]) in (int,float) and math.isfinite(row[name]) and row[name] >= 0, "invalid scale time")
    return row


def run(args):
    build, capture = args.build.absolute(), args.output.absolute()
    require(not build.exists() and not build.is_symlink(), "fresh build required")
    require(not capture.exists() and not capture.is_symlink(), "fresh capture required")
    source_set = sources()
    require(all(p.is_file() for p in source_set), "source list contains an absent file")
    capture.mkdir(parents=True)
    compiler = args.compiler.absolute()
    plan = command_plan(build,capture,compiler,args.sanitize,args.jobs,args.boost.absolute())
    environment = dict(os.environ)
    environment.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    manifest = dict(schema=SCHEMA, phase="exploration_v8_hors_registre", backend="cpu_reference",
        profile="quantized_u18_input_only", public_status="not_claimed", gcp_used=False,
        scope="local_regression_u18_and_optional_atlas_not_FULL_or_scaling_qualification",
        started=utc_stamp(), source_sha256=pins(source_set), compiler_sha256=sha(compiler),
        build=str(build), compiler=str(compiler), sanitize=args.sanitize, jobs=args.jobs,
        boost=str(args.boost.absolute()), plan=plan,
        environment={k: environment[k] for k in ("ASAN_OPTIONS", "UBSAN_OPTIONS")},
        git_commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        worktree=subprocess.check_output(["git", "status", "--porcelain"], cwd=ROOT, text=True))
    write_json(capture / "MANIFEST.json", manifest)
    state = dict(schema=SCHEMA, status="running", manifest_sha256=sha(capture / "MANIFEST.json"), commands=[])
    write_json(capture / "COMPLETION.json", state)
    try:
        for name, command in plan:
            require(sha(compiler) == manifest["compiler_sha256"], f"compiler changed before {name}")
            if name == "build":
                require(read_scan(capture / "dependencies", build, compiler) == state["precompiled_dependencies"],
                        "precompile dependency closure changed before build")
            record = dict(name=name, command=command, started=utc_stamp())
            path = capture / (name + ".json")
            print(name, flush=True)
            try:
                invoke(command, environment, ROOT, record, new_session=True)
            finally:
                record["finished"] = utc_stamp()
                write_json(path, record)
                state["commands"].append(dict(name=name, path=path.name, sha256=sha(path)))
                write_json(capture / "COMPLETION.json", state)
            require(record.get("exit_code") == 0, f"{name} failed: {record.get('stderr', '')[-1000:]}")
            require(pins(source_set) == manifest["source_sha256"], f"sources changed during {name}")
            require(sha(compiler) == manifest["compiler_sha256"], f"compiler changed during {name}")
            if name == "scan":
                state["precompiled_dependencies"] = read_scan(capture / "dependencies", build, compiler)
                state["dependency_scan_sha256"] = sha(capture / "dependencies/SCAN.json")
            if name == "build":
                state["compiled"] = compiled(build)
                require(state["compiled"]["dependencies"] == state["precompiled_dependencies"],
                        "compiled dependencies differ from their precompile pins")
            if name == "inventory":
                inventory = parse_result(base64.b64decode(record["stdout_base64"]))
                state["test_names"] = [test["name"] for test in inventory["tests"]]
            if name.startswith("scale_"):
                state.setdefault("scales", {})[name] = judge_scale(parse_result(base64.b64decode(record["stdout_base64"])),int(command[-1]))
        state["ctest"] = judge_xml(capture / "CTEST.xml", state["test_names"], args.sanitize)
        state["ctest_sha256"] = sha(capture / "CTEST.xml")
        require(compiled(build) == state["compiled"], "compiled closure/inventory changed")
        require(read_scan(capture / "dependencies", build, compiler) == state["precompiled_dependencies"] and
                sha(capture / "dependencies/SCAN.json") == state["dependency_scan_sha256"], "scan closure changed")
        require(pins(sources()) == manifest["source_sha256"], "final source inventory changed")
        require(sha(compiler) == manifest["compiler_sha256"], "compiler changed at closure")
        state["status"] = "passed"
    except BaseException as error:
        state["status"] = "failed"
        state["error"] = repr(error)
        raise
    finally:
        state["finished"] = utc_stamp()
        write_json(capture / "COMPLETION.json", state)
    return state["ctest"]


def read(capture):
    capture = capture.absolute()
    proof_pins = pins(p for p in capture.rglob("*") if p.is_file())
    manifest, state = load(capture / "MANIFEST.json"), load(capture / "COMPLETION.json")
    require(manifest["schema"] == state["schema"] == SCHEMA and state["status"] == "passed", "capture not passed")
    require(manifest["public_status"] == "not_claimed" and manifest["gcp_used"] is False, "invalid promotion")
    require(sha(capture / "MANIFEST.json") == state["manifest_sha256"], "manifest changed")
    require(pins(manifest["source_sha256"]) == manifest["source_sha256"], "LIVE sources changed")
    require(pins(sources()) == manifest["source_sha256"], "LIVE source inventory differs")
    compiler = Path(manifest["compiler"])
    require(sha(compiler) == manifest["compiler_sha256"], "LIVE compiler changed")
    require(manifest["plan"] == command_plan(Path(manifest["build"]),capture,compiler,manifest["sanitize"],
            manifest["jobs"],Path(manifest["boost"])), "command plan differs from required qualification")
    require(len(state["commands"]) == len(manifest["plan"]), "incomplete command list")
    scales = {}
    for item, (name, command) in zip(state["commands"], manifest["plan"]):
        require(item["name"] == name and item["path"] == name + ".json", "command record identity mismatch")
        path = capture / item["path"]
        require(sha(path) == item["sha256"], "command record changed")
        record = load(path)
        check_record(record, command)
        if name == "inventory":
            inventory = parse_result(base64.b64decode(record["stdout_base64"]))
            require(state["test_names"] == [test["name"] for test in inventory["tests"]], "test inventory record differs")
        if name.startswith("scale_"):
            scales[name] = judge_scale(parse_result(base64.b64decode(record["stdout_base64"])),int(command[-1]))
    require(scales == state["scales"], "scale record inventory differs")
    for category in ("dependencies", "outputs"):
        require(pins(state["compiled"][category]) == state["compiled"][category], "LIVE compiled closure changed")
    require(compiled(Path(manifest["build"])) == state["compiled"], "LIVE compiler inventory differs")
    require(sha(capture / "dependencies/SCAN.json") == state["dependency_scan_sha256"], "dependency scan changed")
    require(read_scan(capture / "dependencies", Path(manifest["build"]), compiler) ==
            state["precompiled_dependencies"] == state["compiled"]["dependencies"], "pre/post dependency closure differs")
    require(sha(capture / "CTEST.xml") == state["ctest_sha256"], "CTest XML changed")
    require(judge_xml(capture / "CTEST.xml", state["test_names"], manifest["sanitize"]) == state["ctest"], "CTest summary changed")
    # Close again AFTER the complete replay, not only before consuming proofs.
    require(sha(capture / "MANIFEST.json") == state["manifest_sha256"] and load(capture / "COMPLETION.json") == state,
            "capture changed during read")
    require(pins(sources()) == manifest["source_sha256"] and sha(compiler) == manifest["compiler_sha256"],
            "LIVE source/compiler closure changed during read")
    require(compiled(Path(manifest["build"])) == state["compiled"], "LIVE compiled closure changed during read")
    require(pins(p for p in capture.rglob("*") if p.is_file()) == proof_pins, "raw proof closure changed during read")
    return dict(status="passed", **state["ctest"], public_status="not_claimed", gcp_used=False)


def main():
    signal.signal(signal.SIGTERM, on_signal)
    signal.signal(signal.SIGINT, on_signal)
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)
    launch = sub.add_parser("run")
    launch.add_argument("--build", type=Path, required=True)
    launch.add_argument("--output", type=Path, required=True)
    launch.add_argument("--compiler", type=Path, default=Path("/usr/bin/g++"))
    launch.add_argument("--boost", type=Path, default=ROOT / "build/v7_boost_gate/extracted/usr")
    launch.add_argument("--sanitize", action="store_true")
    launch.add_argument("--jobs", type=int, default=4)
    sub.add_parser("read").add_argument("capture", type=Path)
    scanner = sub.add_parser("scan")
    scanner.add_argument("--build", type=Path, required=True)
    scanner.add_argument("--compiler", type=Path, required=True)
    scanner.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = run(args) if args.mode == "run" else scan(args.build, args.compiler, args.output) if args.mode == "scan" else read(args.capture)
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
