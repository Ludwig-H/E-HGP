#!/usr/bin/env python3
"""Qualify native binary32 q3/q4 supports and signs, not a candidate generator."""
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
CORE = V8 / "src/core/float32_ball.cpp"
PROBE = V8 / "tests/float32_ball_probe.cpp"
INTEGER = V8 / "tests/fixed_signed_gate.cpp"
GATE = V8 / "tests/float32_ball_gate.py"
SOURCES = (CORE, CORE.with_suffix(".hpp"), V8 / "src/core/fixed_signed.hpp",
           V8 / "src/core/float32_predicates.hpp", PROBE, INTEGER, GATE,
           Path(__file__).resolve(), V8 / "bench/run_p0_matrix.py")
SCHEMA = "mhgp8_float32_ball_qualification_v1"
SCOPE = "native_binary32_q3_q4_positive_supports_and_power_not_generator_or_FULL"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def pins(paths):
    return {str(p): sha(p) for p in sorted(map(Path, paths))}


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
    plan = [("compiler", [compiler, "--version"])]
    for name, source in (("core", CORE), ("test", PROBE), ("integer", INTEGER)):
        plan.append(("dependencies_"+name, [compiler, *flags, "-M", str(source), "-MF", str(build / ("pre_"+name+".d")),
                                            "-MT", str(build / (name+".o"))]))
    for name, source in (("core", CORE), ("test", PROBE), ("integer", INTEGER)):
        plan.append(("compile_"+name, [compiler, *flags, "-c", str(source), "-MD", "-MF", str(build / (name+".d")),
                                       "-o", str(build / (name+".o"))]))
    plan.append(("link_test", [compiler, *flags, str(build / "core.o"), str(build / "test.o"),
                               "-o", str(build / "mhgp8_float32_ball_test")]))
    plan.append(("link_integer", [compiler, *flags, str(build / "integer.o"),
                                  "-o", str(build / "mhgp8_fixed_signed_test")]))
    plan.append(("integer_gate", [str(build / "mhgp8_fixed_signed_test")]))
    for option, name in (([], "normal"), (["-O"], "optimized")):
        plan.append(("gate_"+name, [config["python"], *option, str(GATE), "--binary", str(build / "mhgp8_float32_ball_test"),
                                    "--output", str(target / ("gate_"+name))]))
    return plan


def dependency_pins(build, prefix=""):
    dependencies = set()
    for name in ("core", "test", "integer"):
        dep = build / (prefix+name+".d")
        tokens = shlex.split(dep.read_text().replace("\\\n", " ").split(":", 1)[1])
        dependencies.update(Path(token).resolve(strict=True) for token in tokens)
    require(set(SOURCES[:6]) <= dependencies, "ball compiler dependencies omit sources")
    require(all(p.is_relative_to(V8) or str(p).startswith(("/usr/", "/opt/")) for p in dependencies),
            "unexpected ball dependency path")
    return pins(dependencies)


def compiled_pins(build):
    outputs = [build / (name+suffix) for name in ("core", "test", "integer") for suffix in (".d", ".o")]
    outputs.extend(build / ("pre_"+name+".d") for name in ("core", "test", "integer"))
    outputs.extend((build / "mhgp8_float32_ball_test", build / "mhgp8_fixed_signed_test"))
    return dict(dependencies=dependency_pins(build), outputs=pins(outputs))


def run(args):
    build, compiler = args.build.absolute(), Path(args.compiler).absolute()
    require(not build.exists() and not build.is_symlink(), "fresh ball build required")
    require(compiler.is_file(), "compiler driver is absent")
    config = dict(build=str(build), compiler=str(compiler), python=sys.executable, sanitize=args.sanitize)
    source_pins = pins([*SOURCES, compiler])
    args.output.mkdir(parents=True, exist_ok=True)
    target = Path(tempfile.mkdtemp(prefix="ball_", dir=args.output.resolve()))
    build.mkdir(parents=True, exist_ok=False)
    for source in SOURCES:
        destination = target / "sources" / source.relative_to(V8)
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(source.read_bytes())
    plan = commands(config, target)
    environment = dict(os.environ)
    if args.sanitize:
        environment.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    manifest = dict(schema=SCHEMA, config=config, source_sha256=source_pins,
                    started_utc=stamp(), launch=[sys.executable, *sys.argv], plan=plan,
                    environment={key: environment.get(key) for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS")},
                    git_commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                    scope=SCOPE, public_status="not_claimed", gcp_used=False)
    write(target / "MANIFEST.json", manifest)
    manifest_pin = sha(target / "MANIFEST.json")
    state = dict(status="running", commands=[], started_utc=stamp())
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    print(json.dumps(dict(path=str(target), status="starting")), flush=True)
    try:
        for number, (label, command) in enumerate(plan):
            filename = f"command_{number:03}.json"
            record = dict(label=label, command=command, cwd=str(ROOT), exit_code=None,
                          stdout="", stderr="", stdout_base64="", stderr_base64="")
            print(label, flush=True)
            try:
                invoke(command, environment, ROOT, record, new_session=True)
            finally:
                write(target / filename, record)
                state["commands"].append(dict(path=filename, sha256=sha(target / filename)))
            require(record["exit_code"] == 0, "ball command failed: "+label)
            if label == "dependencies_integer":
                state["dependencies_before"] = dependency_pins(build, "pre_")
            if label == "link_integer":
                state["compiled"] = compiled_pins(build)
                require(state["compiled"]["dependencies"] == state["dependencies_before"],
                        "ball dependencies changed during compilation")
        state["status"] = "passed"
    except BaseException as error:
        state.update(status="failed", error=f"{type(error).__name__}: {error}")
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        try:
            state["source_sha256_after"] = pins(source_pins)
            require(state["source_sha256_after"] == source_pins and sha(target / "MANIFEST.json") == manifest_pin,
                    "ball source/manifest changed during qualification")
            if "compiled" in state:
                require(state["compiled"] == compiled_pins(build), "ball native closure changed")
            if "dependencies_before" in state:
                require(pins(state["dependencies_before"]) == state["dependencies_before"], "ball dependencies changed")
            state["artifact_sha256"] = pins(p for p in target.rglob("*") if p.is_file())
        except BaseException as error:
            state.update(status="failed", closing_error=f"{type(error).__name__}: {error}")
        state["finished_utc"] = stamp()
        write(target / "COMPLETION.json", state)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    require(state["status"] == "passed", "ball qualification failed; capture retained")
    print(json.dumps(read(target), sort_keys=True))


def read(path):
    path = path.resolve()
    require(path.is_dir() and not any(p.is_symlink() for p in path.rglob("*")), "ball capture has a link")
    manifest, state = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    closure = sha(path / "COMPLETION.json")
    require(manifest["schema"] == SCHEMA and manifest["scope"] == SCOPE and
            manifest["public_status"] == "not_claimed" and manifest["gcp_used"] is False and
            state["status"] == "passed", "ball capture scope/status")
    config = manifest["config"]
    parsed = parser().parse_args(manifest["launch"][2:])
    require(manifest["launch"][:2] in ([config["python"], str(Path(__file__).resolve())],
            [config["python"], str(Path(__file__).resolve().relative_to(ROOT))]), "ball launching script")
    require(parsed.operation == "run" and str(parsed.build.absolute()) == config["build"] and
            str(Path(parsed.compiler).absolute()) == config["compiler"] and parsed.sanitize == config["sanitize"] and
            parsed.output.resolve() == path.parent, "ball launch/configuration mismatch")
    if config["sanitize"]:
        require(manifest["environment"] == dict(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"), "ball sanitizer environment")
    expected_pins = pins([*SOURCES, config["compiler"]])
    require(manifest["source_sha256"] == state["source_sha256_after"] == expected_pins, "ball source closure")
    for source in SOURCES:
        require(sha(path / "sources" / source.relative_to(V8)) == expected_pins[str(source)], "ball source snapshot differs")
    files = [p for p in path.rglob("*") if p.is_file() and p != path / "COMPLETION.json"]
    require(pins(files) == state["artifact_sha256"], "ball artifact closure")
    build = Path(config["build"])
    require(compiled_pins(build) == state["compiled"] and dependency_pins(build, "pre_") ==
            state["dependencies_before"] == state["compiled"]["dependencies"], "ball binary/dependency closure")
    plan = commands(config, path)
    require(manifest["plan"] == [[a, b] for a, b in plan] and len(state["commands"]) == len(plan), "ball command plan")
    results = {}
    for number, ((label, command), entry) in enumerate(zip(plan, state["commands"], strict=True)):
        require(entry["path"] == f"command_{number:03}.json" and sha(path / entry["path"]) == entry["sha256"], "ball command hash")
        record = read_json(path / entry["path"])
        require(record["label"] == label and record["command"] == command and record["cwd"] == str(ROOT) and
                type(record["exit_code"]) is int and record["exit_code"] == 0, "ball command identity/exit")
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(record[stream+"_base64"], validate=True).decode("utf-8") == record[stream],
                    "ball raw stream differs")
        if label.startswith("gate_") or label == "integer_gate":
            require(record["stderr"] == "", "ball gate emitted a diagnostic")
            results[label] = parse_result(record["stdout"].encode())
            require(results[label]["status"] == "passed", "ball command gate failed")
    spec = importlib.util.spec_from_file_location("ball_fraction_gate", GATE)
    gate = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gate)
    for name in ("normal", "optimized"):
        replay = gate.read_transcript(path / ("gate_"+name), build / "mhgp8_float32_ball_test")
        require(all(results["gate_"+name][key] == item for key, item in replay.items()), "ball native oracle replay differs")
    normal, optimized = (dict(results["gate_"+name]) for name in ("normal", "optimized"))
    require(normal.pop("optimized") is False and optimized.pop("optimized") is True and normal == optimized,
            "ball normal/optimized gate mismatch")
    require(normal["gate_sha256"] == sha(GATE) and normal["binary"] == str(build / "mhgp8_float32_ball_test") and
            normal["binary_sha256"] == state["compiled"]["outputs"][normal["binary"]] and
            normal["source_sha256"] == pins(gate.SOURCES) and normal["rejected_mutations"] == 13,
            "ball gate binary/source/mutation binding")
    native = normal["native_selftest"]
    require(normal["schema"] == "mhgp8_float32_ball_gate_v1" and normal["cases"] == 1636 and
            normal["rejected_inputs"] == 53 and native["tests"] == 39+312*native["flush_modes"] and
            native["query_checks"] == 132+104*native["flush_modes"] and native["invalid_modes"] == 2 and
            native["nonfinite_rejections"] == 18, "ball fixture/selftest inventory changed")
    rows = [parse_result(line) for line in (path / "gate_normal/probe.stdout").read_bytes().splitlines()]
    require(gate.mutations(rows, gate.fixtures()) == 13, "ball oracle mutations changed during reading")
    integer = results["integer_gate"]
    require(integer["tests"] == 948 and integer["overflow_rejections"] == 10 and integer["invalid_rejections"] == 6,
            "fixed integer gate lacks positive/refusal checks")
    require(pins(files) == state["artifact_sha256"] and sha(path / "COMPLETION.json") == closure and
            pins(expected_pins) == expected_pins and compiled_pins(build) == state["compiled"], "ball final closure changed")
    return dict(status="passed", schema=SCHEMA, path=str(path), commands=len(plan), completion_sha256=closure,
                gate=results["gate_normal"], integer=integer, scope=SCOPE, gcp_used=False)


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
