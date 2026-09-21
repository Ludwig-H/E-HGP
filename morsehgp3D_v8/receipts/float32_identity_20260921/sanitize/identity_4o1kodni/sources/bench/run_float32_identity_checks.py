#!/usr/bin/env python3
"""Common LIVE qualification of binary32 balls, canonical keys and q4 events.

Explicit adaptation of run_float32_ball_checks.py; old captures are unchanged.
All eight dependency inventories are captured before any compilation. The two
Python oracles run normally and under -O, then their transcripts and mutations
are replayed. Reading requires the pinned live sources, toolchain and build;
this is deliberately not a portable historical/archive reader.
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
    ("ball", V8 / "src/core/float32_ball.cpp"),
    ("key", V8 / "src/core/float32_ball_key.cpp"),
    ("events", V8 / "src/core/float32_q4_events.cpp"),
    ("ball_probe", V8 / "tests/float32_ball_probe.cpp"),
    ("key_probe", V8 / "tests/float32_key_probe.cpp"),
    ("events_probe", V8 / "tests/float32_q4_events_probe.cpp"),
    ("integer", V8 / "tests/fixed_signed_gate.cpp"),
    ("division", V8 / "tests/fixed_signed_division_gate.cpp"),
)
HEADERS = tuple(V8 / ("src/core/"+name+".hpp") for name in (
    "fixed_signed", "float32_predicates", "float32_ball", "float32_ball_key", "float32_q4_events"))
BALL_GATE = V8 / "tests/float32_ball_gate.py"
IDENTITY_GATE = V8 / "tests/float32_identity_gate.py"
SOURCES = (*[path for _, path in UNITS], *HEADERS, BALL_GATE, IDENTITY_GATE,
           Path(__file__).resolve(), V8 / "bench/run_p0_matrix.py")
BINARIES = (
    ("ball", "mhgp8_float32_ball_test", ("ball", "ball_probe")),
    ("key", "mhgp8_float32_ball_key_test", ("ball", "key", "key_probe")),
    ("events", "mhgp8_float32_q4_events_test", ("ball", "events", "events_probe")),
    ("integer", "mhgp8_fixed_signed_test", ("integer",)),
    ("division", "mhgp8_fixed_signed_division_test", ("division",)),
)
SCHEMA = "mhgp8_float32_identity_qualification_v1"
SCOPE = "native_binary32_ball_keys_and_reduced_q4_events_not_pipeline_or_FULL_or_GPU"
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


def binary_paths(build):
    return {name: build / filename for name, filename, _ in BINARIES}


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
    binaries = binary_paths(build)
    for name, _, objects in BINARIES:
        result.append(("link_"+name, [compiler, *flags, *(str(build / (obj+".o")) for obj in objects),
                       "-o", str(binaries[name])]))
    result.extend((name+"_gate", [str(binaries[name])]) for name in ("integer", "division"))
    for option, mode in (([], "normal"), (["-O"], "optimized")):
        result.append(("ball_gate_"+mode, [config["python"], *option, str(BALL_GATE),
                       "--binary", str(binaries["ball"]), "--output", str(target / ("ball_gate_"+mode))]))
        result.append(("identity_gate_"+mode, [config["python"], *option, str(IDENTITY_GATE),
                       "--binary", str(binaries["key"]), "--events-binary", str(binaries["events"]),
                       "--output", str(target / ("identity_gate_"+mode))]))
    return result


def dependency_pins(build, prefix=""):
    per_unit, complete = {}, set()
    for name, source in UNITS:
        data = (build / (prefix+name+".d")).read_text().replace("\\\n", " ")
        require(":" in data, "compiler dependency target missing")
        dependencies = {Path(token).resolve(strict=True) for token in shlex.split(data.split(":", 1)[1])}
        require(source in dependencies and all(path.is_relative_to(V8) or str(path).startswith(("/usr/", "/opt/"))
                                               for path in dependencies), "unexpected identity compiler dependency")
        per_unit[name] = pins(dependencies)
        complete.update(dependencies)
    require({*[source for _, source in UNITS], *HEADERS} <= complete, "compiler omitted a native identity source")
    return per_unit


def compiled_pins(build):
    outputs = [build / (name+suffix) for name, _ in UNITS for suffix in (".d", ".o")]
    outputs.extend(build / ("pre_"+name+".d") for name, _ in UNITS)
    outputs.extend(binary_paths(build).values())
    return dict(dependencies=dependency_pins(build), outputs=pins(outputs))


def artifact_pins(path):
    require(not any(item.is_symlink() for item in path.rglob("*")), "identity capture has a symlink")
    # Only THIS completion is excluded, never a nested file of the same name.
    return pins(item for item in path.rglob("*") if item.is_file() and item != path / "COMPLETION.json")


def run(args):
    build, compiler = args.build.absolute(), Path(args.compiler).absolute()
    require(not build.exists() and not build.is_symlink(), "fresh identity build required")
    require(compiler.is_file() and os.access(compiler, os.X_OK), "compiler driver is absent")
    require(len(SOURCES) == len(set(SOURCES)) == 17, "identity source inventory changed")
    config = dict(build=str(build), compiler=str(compiler), python=sys.executable, sanitize=args.sanitize)
    source_pins = pins([*SOURCES, compiler])
    args.output.mkdir(parents=True, exist_ok=True)
    target = Path(tempfile.mkdtemp(prefix="identity_", dir=args.output.resolve()))
    build.mkdir(parents=True, exist_ok=False)
    for source in SOURCES:
        destination = target / "sources" / source.relative_to(V8)
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open("xb") as stream:
            stream.write(source.read_bytes())
    plan = commands(config, target)
    require(len(plan) == 28, "identity command inventory changed")
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
            require(type(record["exit_code"]) is int and record["exit_code"] == 0, "identity command failed: "+label)
            if label == "dependencies_division":
                state["dependencies_before"] = dependency_pins(build, "pre_")
            if label == "link_division":
                state["compiled"] = compiled_pins(build)
                require(state["compiled"]["dependencies"] == state["dependencies_before"],
                        "identity dependencies changed during compilation")
        state["status"] = "passed"
    except BaseException as error:
        state.update(status="failed", error=f"{type(error).__name__}: {error}")
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        try:
            state["source_sha256_after"] = pins(source_pins)
            require(state["source_sha256_after"] == source_pins and
                    sha(target / "MANIFEST.json") == state["manifest_sha256"], "identity source/manifest closure changed")
            if "dependencies_before" in state:
                require(dependency_pins(build, "pre_") == state["dependencies_before"], "identity dependencies changed")
            if "compiled" in state:
                require(compiled_pins(build) == state["compiled"], "identity native closure changed")
            state["artifact_sha256"] = artifact_pins(target)
        except BaseException as error:
            state.update(status="failed", closing_error=f"{type(error).__name__}: {error}")
        state["finished_utc"] = stamp()
        write(target / "COMPLETION.json", state)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    require(state["status"] == "passed", "identity qualification failed; capture retained")
    print(json.dumps(read(target), sort_keys=True))


def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    sys.modules[name] = result
    spec.loader.exec_module(result)
    return result


def normalized_pair(results, prefix):
    normal, optimized = (dict(results[prefix+"_"+name]) for name in ("normal", "optimized"))
    require(normal.pop("optimized") is False and optimized.pop("optimized") is True and normal == optimized,
            prefix+" normal/optimized gate mismatch")
    return normal


def read(path):
    path = path.resolve(strict=True)
    require(path.is_dir(), "identity capture is not a directory")
    closure = sha(path / "COMPLETION.json")
    manifest, state = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    require(manifest["schema"] == SCHEMA and manifest["scope"] == SCOPE and
            manifest["public_status"] == "not_claimed" and manifest["gcp_used"] is False and
            state["status"] == "passed", "identity capture scope/status")
    config = manifest["config"]
    require(type(config["sanitize"]) is bool and len(SOURCES) == 17, "identity configuration/inventory")
    parsed = parser().parse_args(manifest["launch"][2:])
    require(manifest["launch"][:2] in ([config["python"], str(Path(__file__).resolve())],
            [config["python"], str(Path(__file__).resolve().relative_to(ROOT))]), "identity launching script")
    require(parsed.operation == "run" and str(parsed.build.absolute()) == config["build"] and
            str(Path(parsed.compiler).absolute()) == config["compiler"] and parsed.sanitize == config["sanitize"] and
            parsed.output.resolve() == path.parent, "identity launch/configuration mismatch")
    require(set(manifest["environment"]) == set(ENV_KEYS) and all(value is None or type(value) is str
            for value in manifest["environment"].values()), "identity environment shape")
    if config["sanitize"]:
        require(manifest["environment"]["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1" and
                manifest["environment"]["UBSAN_OPTIONS"] == "halt_on_error=1:print_stacktrace=1", "identity sanitizer environment")
    expected_pins = pins([*SOURCES, config["compiler"]])
    require(manifest["source_sha256"] == state["source_sha256_after"] == expected_pins and
            sha(path / "MANIFEST.json") == state["manifest_sha256"], "identity source/manifest closure")
    for source in SOURCES:
        require(sha(path / "sources" / source.relative_to(V8)) == expected_pins[str(source)], "identity source snapshot differs")
    require(artifact_pins(path) == state["artifact_sha256"], "identity artifact closure")
    build, binaries = Path(config["build"]), binary_paths(Path(config["build"]))
    require(compiled_pins(build) == state["compiled"] and dependency_pins(build, "pre_") ==
            state["dependencies_before"] == state["compiled"]["dependencies"], "identity binary/dependency closure")
    plan = commands(config, path)
    require(len(plan) == 28 and manifest["plan"] == [[label, command] for label, command in plan] and
            len(state["commands"]) == len(plan), "identity command plan")
    results = {}
    for number, ((label, command), entry) in enumerate(zip(plan, state["commands"], strict=True)):
        require(entry["path"] == f"command_{number:03}.json" and sha(path / entry["path"]) == entry["sha256"], "identity command hash")
        record = read_json(path / entry["path"])
        require(record["label"] == label and record["command"] == command and record["cwd"] == str(ROOT) and
                record["environment"] == manifest["environment"] and type(record["exit_code"]) is int and
                record["exit_code"] == 0, "identity command identity/exit")
        for stream in ("stdout", "stderr"):
            raw = base64.b64decode(record[stream+"_base64"], validate=True)
            require(raw.decode("utf-8", errors="replace") == record[stream], "identity raw stream differs")
        if "gate" in label:
            require(record["stderr"] == "", "identity gate emitted a diagnostic")
            results[label] = parse_result(base64.b64decode(record["stdout_base64"], validate=True))
            require(results[label]["status"] == "passed", "identity gate returned a failed result")

    ball_gate = module(BALL_GATE, "mhgp8_ball_qualification_oracle")
    identity_gate = module(IDENTITY_GATE, "mhgp8_identity_qualification_oracle")
    for mode in ("normal", "optimized"):
        ball_replay = ball_gate.read_transcript(path / ("ball_gate_"+mode), binaries["ball"])
        require(all(results["ball_gate_"+mode][key] == value for key, value in ball_replay.items()), "ball oracle replay differs")
        identity_replay = identity_gate.read_transcript(path / ("identity_gate_"+mode), binaries["key"], binaries["events"])
        require(all(results["identity_gate_"+mode][key] == value for key, value in identity_replay.items()), "identity oracle replay differs")
    ball = normalized_pair(results, "ball_gate")
    require(ball["schema"] == "mhgp8_float32_ball_gate_v1" and ball["gate_sha256"] == sha(BALL_GATE) and
            ball["binary"] == str(binaries["ball"]) and ball["binary_sha256"] == state["compiled"]["outputs"][ball["binary"]] and
            ball["source_sha256"] == pins(ball_gate.SOURCES) and ball["rejected_mutations"] == 13,
            "ball gate binary/source/mutation binding")
    native = ball["native_selftest"]
    require(ball["cases"] == 1636 and ball["rejected_inputs"] == 53 and
            native["tests"] == 39+312*native["flush_modes"] and native["query_checks"] == 132+104*native["flush_modes"] and
            native["invalid_modes"] == 2 and native["nonfinite_rejections"] == 18, "ball fixture/selftest inventory changed")
    ball_rows = [parse_result(line) for line in (path / "ball_gate_normal/probe.stdout").read_bytes().splitlines()]
    require(ball_gate.mutations(ball_rows, ball_gate.fixtures()) == 13, "ball oracle mutations changed during reading")
    identity = normalized_pair(results, "identity_gate")
    require(identity["schema"] == "mhgp8_float32_identity_gate_v1" and identity["gate_sha256"] == sha(IDENTITY_GATE) and
            identity["binary"] == str(binaries["key"]) and identity["events_binary"] == str(binaries["events"]) and
            identity["binary_sha256"] == state["compiled"]["outputs"][identity["binary"]] and
            identity["events_binary_sha256"] == state["compiled"]["outputs"][identity["events_binary"]] and
            identity["source_sha256"] == pins(identity_gate.SOURCES) and identity["scope"] ==
            "binary32_ball_identity_and_q4_event_primitives_not_radius_sort_census_or_FULL",
            "identity gate binary/source/scope binding")
    require(identity["keys"]["cases"] == 229 and identity["events"]["cases"] == 960 and
            identity["rejected_inputs"] == dict(keys=77, events=64) and identity["rejected_mutations"] == 18,
            "identity fixture/mutation inventory changed")
    key_rows = [parse_result(line) for line in (path / "identity_gate_normal/keys.stdout").read_bytes().splitlines()]
    event_rows = [parse_result(line) for line in (path / "identity_gate_normal/events.stdout").read_bytes().splitlines()]
    rejected = identity_gate.mutations(key_rows, identity_gate.key_fixtures(), event_rows, identity_gate.event_fixtures())
    require(type(rejected) is int and rejected == 18,
            "identity oracle mutations changed during reading")
    integer, division = results["integer_gate"], results["division_gate"]
    require(integer == dict(status="passed", tests=948, overflow_rejections=10, invalid_rejections=6),
            "original fixed integer gate inventory changed")
    require(division == dict(status="passed", tests=9051, exact_divisions=185, gcd_checks=1124, shift_checks=47,
                            nonexact_rejections=347, invalid_divisor_rejections=9, overflow_rejections=9),
            "fixed integer division gate inventory changed")
    require(artifact_pins(path) == state["artifact_sha256"] and sha(path / "COMPLETION.json") == closure and
            pins(expected_pins) == expected_pins and compiled_pins(build) == state["compiled"] and
            dependency_pins(build, "pre_") == state["dependencies_before"], "identity final LIVE closure changed")
    return dict(status="passed", schema=SCHEMA, path=str(path), commands=len(plan), completion_sha256=closure,
                ball_gate=results["ball_gate_normal"], identity_gate=results["identity_gate_normal"],
                integer=integer, division=division, scope=SCOPE, gcp_used=False)


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
