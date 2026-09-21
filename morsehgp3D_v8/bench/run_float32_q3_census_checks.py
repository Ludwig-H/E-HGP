#!/usr/bin/env python3
"""LIVE qualification of the native binary32 q3 census for one edge/X node.

Explicit port of run_float32_identity_checks.py; old runners and captures are
not changed. Five per-unit dependency inventories precede all compilations.
The independent oracle runs normally and under -O; reading rejudges the raw
transcripts. This is a LIVE reader, not a portable historical archive reader.
It does not qualify a global candidate generator, a hierarchy, GPU, or GCP.
"""
from __future__ import annotations

import argparse
import base64
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
import tempfile

from run_p0_matrix import invoke, on_signal, parse_result

ROOT = Path(__file__).resolve().parents[2]
V8 = ROOT / "morsehgp3D_v8"
UNITS = (
    ("index", V8 / "src/spatial/float32_index.cpp"),
    ("ball", V8 / "src/core/float32_ball.cpp"),
    ("block", V8 / "src/core/float32_q3_block.cpp"),
    ("census", V8 / "src/lanes/float32_q3_census.cpp"),
    ("probe", V8 / "tests/float32_q3_census_probe.cpp"),
)
HEADERS = tuple(V8 / name for name in (
    "src/spatial/float32_index.hpp", "src/core/float32_predicates.hpp",
    "src/core/fixed_signed.hpp", "src/core/float32_ball.hpp",
    "src/core/float32_q3_block.hpp", "src/lanes/float32_q3_census.hpp"))
GATE = V8 / "tests/float32_q3_census_gate.py"
SOURCES = (*[source for _, source in UNITS], *HEADERS, GATE, V8 / "tests/float32_identity_gate.py",
           Path(__file__).resolve(), V8 / "bench/run_p0_matrix.py")
BINARY = "mhgp8_float32_q3_census_test"
SCHEMA = "mhgp8_float32_q3_census_qualification_v1"
SCOPE = "native_binary32_q3_edge_seed_node_census_not_global_generator_or_FULL_or_GPU"
ENV_KEYS = ("PATH", "LANG", "LC_ALL", "TZ", "CPATH", "CPLUS_INCLUDE_PATH", "C_INCLUDE_PATH",
            "GCC_EXEC_PREFIX", "COMPILER_PATH", "LIBRARY_PATH", "LD_LIBRARY_PATH", "LD_PRELOAD",
            "SOURCE_DATE_EPOCH", "ASAN_OPTIONS", "UBSAN_OPTIONS")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def pins(paths):
    return {str(path): sha(path) for path in sorted(map(Path, paths))}


def read_json(path):
    return parse_result(Path(path).read_bytes())


def write(path, value):
    with path.open("x") as stream:
        stream.write(json.dumps(value, indent=2, sort_keys=True, allow_nan=False)+"\n")


def stamp():
    return datetime.now(timezone.utc).isoformat()


def commands(config, target):
    build = Path(config["build"])
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
             "-ffp-contract=off", "-fno-fast-math", "-frounding-math", "-I", str(V8 / "src")]
    flags += (["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if config["sanitize"]
              else ["-O3", "-DNDEBUG"])
    compiler = config["compiler"]
    result = [("compiler", [compiler, "--version"])]
    for name, source in UNITS:
        result.append(("dependencies_"+name, [compiler, *flags, "-M", str(source),
                       "-MF", str(build / ("pre_"+name+".d")), "-MT", str(build / (name+".o"))]))
    for name, source in UNITS:
        result.append(("compile_"+name, [compiler, *flags, "-c", str(source), "-MD",
                       "-MF", str(build / (name+".d")), "-o", str(build / (name+".o"))]))
    binary = build / BINARY
    result.append(("link", [compiler, *flags, *(str(build / (name+".o")) for name, _ in UNITS),
                   "-o", str(binary)]))
    for option, mode in (([], "normal"), (["-O"], "optimized")):
        result.append(("gate_"+mode, [config["python"], *option, str(GATE),
                       "--binary", str(binary), "--output", str(target / ("gate_"+mode))]))
    return result


def dependency_pins(build, prefix=""):
    per_unit, complete = {}, set()
    for name, source in UNITS:
        data = (build / (prefix+name+".d")).read_text().replace("\\\n", " ")
        require(":" in data, "compiler dependency target missing")
        dependencies = {Path(token).resolve(strict=True) for token in shlex.split(data.split(":", 1)[1])}
        require(source in dependencies and all(path.is_relative_to(V8) or str(path).startswith(("/usr/", "/opt/"))
                                               for path in dependencies), "unexpected q3 census compiler dependency")
        per_unit[name] = pins(dependencies)
        complete.update(dependencies)
    require({*[source for _, source in UNITS], *HEADERS} <= complete, "compiler omitted a q3 census source")
    return per_unit


def compiled_pins(build):
    outputs = [build / (name+suffix) for name, _ in UNITS for suffix in (".d", ".o")]
    outputs.extend(build / ("pre_"+name+".d") for name, _ in UNITS)
    outputs.append(build / BINARY)
    return dict(dependencies=dependency_pins(build), outputs=pins(outputs))


def artifact_pins(path):
    require(not any(item.is_symlink() for item in path.rglob("*")), "q3 census capture has a symlink")
    # A nested completion, if present, is an artifact; only ours is excluded.
    return pins(item for item in path.rglob("*") if item.is_file() and item != path / "COMPLETION.json")


def run(args):
    build, compiler = args.build.absolute(), Path(args.compiler).absolute()
    require(not build.exists() and not build.is_symlink(), "fresh q3 census build required")
    require(compiler.is_file() and os.access(compiler, os.X_OK), "compiler driver is absent")
    require(len(SOURCES) == len(set(SOURCES)) == 15, "q3 census source inventory changed")
    config = dict(build=str(build), compiler=str(compiler), python=sys.executable, sanitize=args.sanitize)
    source_pins = pins([*SOURCES, compiler])
    args.output.mkdir(parents=True, exist_ok=True)
    target = Path(tempfile.mkdtemp(prefix="q3_census_", dir=args.output.resolve()))
    build.mkdir(parents=True, exist_ok=False)
    for source in SOURCES:
        destination = target / "sources" / source.relative_to(V8)
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open("xb") as stream:
            stream.write(source.read_bytes())
    plan = commands(config, target)
    require(len(plan) == 14, "q3 census command inventory changed")
    environment = dict(os.environ)
    if args.sanitize:
        environment.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    recorded_environment = {key: environment.get(key) for key in ENV_KEYS}
    manifest = dict(schema=SCHEMA, config=config, source_sha256=source_pins, started_utc=stamp(),
                    launch=[sys.executable, *sys.argv], plan=plan, environment=recorded_environment,
                    git_commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                    scope=SCOPE, public_status="not_claimed", gcp_used=False)
    write(target / "MANIFEST.json", manifest)
    state = dict(status="running", commands=[], started_utc=stamp(), manifest_sha256=sha(target / "MANIFEST.json"))
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    print(json.dumps(dict(path=str(target), status="starting")), flush=True)
    try:
        for number, (label, command) in enumerate(plan):
            filename = f"command_{number:03}.json"
            record = dict(label=label, command=command, cwd=str(ROOT), environment=recorded_environment,
                          exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
            print(label, flush=True)
            try:
                invoke(command, environment, ROOT, record, new_session=True)
            finally:
                write(target / filename, record)
                state["commands"].append(dict(path=filename, sha256=sha(target / filename)))
            require(type(record["exit_code"]) is int and record["exit_code"] == 0, "q3 census command failed: "+label)
            if label == "dependencies_probe":
                state["dependencies_before"] = dependency_pins(build, "pre_")
            if label == "link":
                state["compiled"] = compiled_pins(build)
                require(state["compiled"]["dependencies"] == state["dependencies_before"],
                        "q3 census dependencies changed during compilation")
        state["status"] = "passed"
    except BaseException as error:
        state.update(status="failed", error=f"{type(error).__name__}: {error}")
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        try:
            state["source_sha256_after"] = pins(source_pins)
            require(state["source_sha256_after"] == source_pins and
                    sha(target / "MANIFEST.json") == state["manifest_sha256"], "q3 census source/manifest closure changed")
            if "dependencies_before" in state:
                require(dependency_pins(build, "pre_") == state["dependencies_before"], "q3 census dependencies changed")
            if "compiled" in state:
                require(compiled_pins(build) == state["compiled"], "q3 census native closure changed")
            state["artifact_sha256"] = artifact_pins(target)
        except BaseException as error:
            state.update(status="failed", closing_error=f"{type(error).__name__}: {error}")
        state["finished_utc"] = stamp()
        write(target / "COMPLETION.json", state)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    require(state["status"] == "passed", "q3 census qualification failed; capture retained")
    print(json.dumps(read(target), sort_keys=True))


def oracle_module():
    name = "mhgp8_float32_q3_census_qualification_oracle"
    spec = importlib.util.spec_from_file_location(name, GATE)
    result = importlib.util.module_from_spec(spec)
    sys.modules[name] = result
    spec.loader.exec_module(result)
    return result


def read(path):
    path = path.resolve(strict=True)
    require(path.is_dir(), "q3 census capture is not a directory")
    closure = sha(path / "COMPLETION.json")
    manifest, state = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    require(manifest["schema"] == SCHEMA and manifest["scope"] == SCOPE and
            manifest["public_status"] == "not_claimed" and manifest["gcp_used"] is False and
            state["status"] == "passed", "q3 census capture scope/status")
    config = manifest["config"]
    require(type(config["sanitize"]) is bool and len(SOURCES) == 15, "q3 census configuration/inventory")
    parsed = parser().parse_args(manifest["launch"][2:])
    require(manifest["launch"][:2] in ([config["python"], str(Path(__file__).resolve())],
            [config["python"], str(Path(__file__).resolve().relative_to(ROOT))]), "q3 census launching script")
    require(parsed.operation == "run" and str(parsed.build.absolute()) == config["build"] and
            str(Path(parsed.compiler).absolute()) == config["compiler"] and parsed.sanitize == config["sanitize"] and
            parsed.output.resolve() == path.parent, "q3 census launch/configuration mismatch")
    require(set(manifest["environment"]) == set(ENV_KEYS) and all(value is None or type(value) is str
            for value in manifest["environment"].values()), "q3 census environment shape")
    if config["sanitize"]:
        require(manifest["environment"]["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1" and
                manifest["environment"]["UBSAN_OPTIONS"] == "halt_on_error=1:print_stacktrace=1", "q3 census sanitizer environment")
    expected_pins = pins([*SOURCES, config["compiler"]])
    require(manifest["source_sha256"] == state["source_sha256_after"] == expected_pins and
            sha(path / "MANIFEST.json") == state["manifest_sha256"], "q3 census source/manifest closure")
    for source in SOURCES:
        require(sha(path / "sources" / source.relative_to(V8)) == expected_pins[str(source)], "q3 census source snapshot differs")
    require(artifact_pins(path) == state["artifact_sha256"], "q3 census artifact closure")
    build, binary = Path(config["build"]), Path(config["build"]) / BINARY
    require(compiled_pins(build) == state["compiled"] and dependency_pins(build, "pre_") ==
            state["dependencies_before"] == state["compiled"]["dependencies"], "q3 census binary/dependency closure")
    plan = commands(config, path)
    require(len(plan) == 14 and manifest["plan"] == [[label, command] for label, command in plan] and
            len(state["commands"]) == len(plan), "q3 census command plan")
    results = {}
    for number, ((label, command), entry) in enumerate(zip(plan, state["commands"], strict=True)):
        require(entry["path"] == f"command_{number:03}.json" and sha(path / entry["path"]) == entry["sha256"], "q3 census command hash")
        record = read_json(path / entry["path"])
        require(record["label"] == label and record["command"] == command and record["cwd"] == str(ROOT) and
                record["environment"] == manifest["environment"] and type(record["exit_code"]) is int and
                record["exit_code"] == 0, "q3 census command identity/exit")
        for stream in ("stdout", "stderr"):
            raw = base64.b64decode(record[stream+"_base64"], validate=True)
            require(raw.decode("utf-8", errors="replace") == record[stream], "q3 census raw stream differs")
        if label.startswith("gate_"):
            require(record["stderr"] == "", "q3 census gate emitted a diagnostic")
            results[label] = parse_result(base64.b64decode(record["stdout_base64"], validate=True))
            require(results[label]["status"] == "passed", "q3 census gate returned a failed result")
    oracle = oracle_module()
    for mode in ("normal", "optimized"):
        replay = oracle.read_transcript(path / ("gate_"+mode), binary)
        require(all(results["gate_"+mode][key] == value for key, value in replay.items()), "q3 census oracle replay differs")
    normal, optimized = (dict(results["gate_"+name]) for name in ("normal", "optimized"))
    require(normal.pop("optimized") is False and optimized.pop("optimized") is True and normal == optimized,
            "q3 census normal/optimized gate mismatch")
    require(normal["schema"] == "mhgp8_float32_q3_census_gate_v1" and normal["gate_sha256"] == sha(GATE) and
            normal["binary"] == str(binary) and normal["binary_sha256"] == state["compiled"]["outputs"][str(binary)] and
            normal["source_sha256"] == pins(oracle.SOURCES) and normal["scope"] ==
            "one_supplied_edge_and_seed_subtree_not_generator_or_FULL", "q3 census gate binary/source/scope binding")
    require(len(oracle.SOURCES) == 13 and set(oracle.SOURCES) <= set(SOURCES), "q3 census oracle source inventory changed")
    require(normal["fixtures"] == 18 and normal["queries"] == 612 and normal["accepted_supports"] == 1798 and
            normal["shell_ids"] == 8610 and normal["max_shell"] == 30 and normal["invalid_inputs"] == 37 and
            normal["mutation_checks"] == 16, "q3 census oracle fixture inventory changed")
    fixtures = oracle.fixtures()
    rows = [read_json(path / "gate_normal" / f"case_{number:03}.stdout") for number in range(len(fixtures))]
    rejected = oracle.mutations(rows, fixtures)
    require(type(rejected) is int and rejected == 16,
            "q3 census oracle mutations changed during reading")
    require(artifact_pins(path) == state["artifact_sha256"] and sha(path / "COMPLETION.json") == closure and
            pins(expected_pins) == expected_pins and compiled_pins(build) == state["compiled"] and
            dependency_pins(build, "pre_") == state["dependencies_before"], "q3 census final LIVE closure changed")
    return dict(status="passed", schema=SCHEMA, path=str(path), commands=len(plan), completion_sha256=closure,
                gate=results["gate_normal"], scope=SCOPE, gcp_used=False)


def parser():
    result = argparse.ArgumentParser(description=__doc__)
    sub = result.add_subparsers(dest="operation", required=True)
    runner = sub.add_parser("run")
    runner.add_argument("--build", type=Path, required=True)
    runner.add_argument("--compiler", required=True)
    runner.add_argument("--sanitize", action="store_true")
    runner.add_argument("--output", type=Path, required=True)
    reader = sub.add_parser("read")
    reader.add_argument("--path", type=Path, required=True)
    return result


if __name__ == "__main__":
    arguments = parser().parse_args()
    if arguments.operation == "run":
        run(arguments)
    else:
        print(json.dumps(read(arguments.path), sort_keys=True))
