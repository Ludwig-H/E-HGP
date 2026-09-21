#!/usr/bin/env python3
"""Qualify lossless/grid inputs and the separate exact binary32 q2 primitive.

No old engine modification, native whole-frame benchmark or GCP action.
Build in a fresh directory; keep failed captures. Full scans are reconstructed
from raw sources, not accepted merely because their payload hashes match.
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

import prepare_lidar_precision as preparation
from run_p0_matrix import invoke, on_signal

ROOT = Path(__file__).resolve().parents[2]
PREPARER = ROOT / "morsehgp3D_v8/bench/prepare_lidar_precision.py"
INPUT_GATE = ROOT / "morsehgp3D_v8/tests/lidar_precision_gate.py"
NATIVE_GATE = ROOT / "morsehgp3D_v8/tests/float32_predicate_gate.py"
PROBE = ROOT / "morsehgp3D_v8/tests/float32_predicate_probe.cpp"
HEADER = ROOT / "morsehgp3D_v8/src/core/float32_predicates.hpp"
SOURCES = (PREPARER, INPUT_GATE, NATIVE_GATE, PROBE, HEADER, Path(__file__).resolve(),
           ROOT / "morsehgp3D_v8/bench/run_p0_matrix.py")
SCHEMA = "mhgp8_lossless_input_and_float32_q2_checks_v1"


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def pins(paths):
    return {str(p): sha(p) for p in sorted(map(Path, paths))}


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False)+"\n")


def read_json(path):
    return preparation.load_json(Path(path).read_bytes())


def stamp():
    return datetime.now(timezone.utc).isoformat()


def commands(config, target):
    build = Path(config["build"])
    binary = build / "mhgp8_float32_predicate_probe"
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-ffp-contract=off", "-fno-fast-math",
             "-I", str(ROOT / "morsehgp3D_v8/src")]
    flags += (["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if config["sanitize"]
              else ["-O3", "-DNDEBUG"])
    plan = [("compiler", [config["compiler"], "--version"]),
            ("compile", [config["compiler"], *flags, "-MD", "-MF", str(build / "probe.d"), str(PROBE), "-o", str(binary)])]
    for option, name in (([], "normal"), (["-O"], "optimized")):
        plan.extend((("input_gate_"+name, [config["python"], *option, str(INPUT_GATE)]),
                     ("native_gate_"+name, [config["python"], *option, str(NATIVE_GATE), "--binary", str(binary),
                                           "--output", str(target / ("native_"+name))])))
    prepared = []
    for number, source in enumerate(config["inputs"]):
        for profile in ("float32", "grid"):
            destination = target / f"scene_{number:02}_{Path(source).stem}_{profile}"
            label = destination.name
            # The grid's omitted precision is deliberately the default 1 mm.
            plan.extend(((label+"_prepare", [config["python"], str(PREPARER), "prepare", "--input", source,
                                             "--output", str(destination), "--profile", profile]),
                         (label+"_read", [config["python"], str(PREPARER), "read", "--path", str(destination)]),
                         (label+"_read_O", [config["python"], "-O", str(PREPARER), "read", "--path", str(destination)])))
            prepared.append(dict(path=str(destination), source=source, profile=profile))
    return plan, prepared


def dependencies(path):
    tokens = shlex.split(path.read_text().replace("\\\n", " ").split(":", 1)[1])
    paths = {Path(token).resolve(strict=True) for token in tokens}
    require(PROBE in paths and HEADER in paths, "binary32 compilation lacks declared project dependencies")
    require(all(p.is_relative_to(ROOT / "morsehgp3D_v8/src") or p == PROBE or str(p).startswith(("/usr/", "/opt/"))
                for p in paths), "unexpected native dependency root")
    return pins(paths)


def run(args):
    build = args.build.absolute()
    require(not build.exists() and not build.is_symlink(), "fresh binary32 build required")
    compiler = Path(args.compiler).resolve(strict=True)
    inputs = [str(p.resolve(strict=True)) for p in args.inputs]
    require(len(inputs) == len(set(inputs)), "duplicate raw scans")
    config = dict(build=str(build), compiler=str(compiler), sanitize=args.sanitize,
                  inputs=inputs, python=sys.executable)
    before = pins([*SOURCES, *inputs, compiler])
    args.output.mkdir(parents=True, exist_ok=True)
    target = Path(tempfile.mkdtemp(prefix="precision_", dir=args.output.resolve()))
    build.mkdir(parents=True, exist_ok=False)
    snapshot = target / "sources"
    snapshot.mkdir()
    for source in SOURCES:
        (snapshot / source.name).write_bytes(source.read_bytes())
    plan, prepared = commands(config, target)
    environment = dict(os.environ)
    if args.sanitize:
        environment.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    manifest = dict(schema=SCHEMA, started_utc=stamp(), config=config,
        launch=[sys.executable, *sys.argv], source_sha256=before, plan=plan, prepared=prepared,
        environment={k: environment.get(k) for k in ("ASAN_OPTIONS", "UBSAN_OPTIONS")},
        git_commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        scope="input_preparation_and_binary32_q2_primitive_not_full_pipeline",
        gcp_used=False, full_contract_qualified=False, public_status="not_claimed")
    write(target / "MANIFEST.json", manifest)
    manifest_pin = sha(target / "MANIFEST.json")
    state = dict(status="running", commands=[], started_utc=stamp())
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    print(json.dumps(dict(path=str(target), status="starting")), flush=True)
    try:
        for number, (label, command) in enumerate(plan):
            name = f"command_{number:03}.json"
            record = dict(label=label, command=command, cwd=str(ROOT), exit_code=None,
                          stdout="", stderr="", stdout_base64="", stderr_base64="")
            print(label, flush=True)
            try:
                invoke(command, environment, ROOT, record, new_session=True)
            finally:
                write(target / name, record)
                state["commands"].append(dict(path=name, sha256=sha(target / name)))
            require(record["exit_code"] == 0, "precision command failed: " + label)
            if label == "compile":
                state["compiled_dependencies"] = dependencies(build / "probe.d")
        state["status"] = "passed"
    except BaseException as error:
        state.update(status="failed", error=f"{type(error).__name__}: {error}")
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        try:
            state["source_sha256_after"] = pins(before)
            require(state["source_sha256_after"] == before, "precision sources changed during qualification")
            require(sha(target / "MANIFEST.json") == manifest_pin, "precision manifest changed")
            if "compiled_dependencies" in state:
                require(pins(state["compiled_dependencies"]) == state["compiled_dependencies"], "compiler dependencies changed")
                state["binary_sha256"] = sha(build / "mhgp8_float32_predicate_probe")
                state["dependency_file_sha256"] = sha(build / "probe.d")
            state["artifact_sha256"] = pins(p for p in target.rglob("*") if p.is_file())
        except BaseException as error:
            state.update(status="failed", closing_error=f"{type(error).__name__}: {error}")
        state["finished_utc"] = stamp()
        write(target / "COMPLETION.json", state)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    require(state["status"] == "passed", "precision qualification failed; capture retained")
    result = read(target)
    print(json.dumps(result, sort_keys=True))


def read(path):
    path = path.resolve()
    require(path.is_dir() and not any(p.is_symlink() for p in path.rglob("*")), "precision receipt contains a link")
    manifest, state = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    closure_hash = sha(path / "COMPLETION.json")
    require(manifest["schema"] == SCHEMA and state["status"] == "passed" and manifest["gcp_used"] is False and
            manifest["full_contract_qualified"] is False and manifest["public_status"] == "not_claimed",
            "precision scope/status differs")
    config = manifest["config"]
    if config["sanitize"]:
        require(manifest["environment"] == dict(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"), "precision sanitizer environment differs")
    parsed = parser().parse_args(manifest["launch"][2:])
    require(manifest["launch"][0] == config["python"] and Path(manifest["launch"][1]).resolve() == Path(__file__).resolve() and
            parsed.operation == "run" and str(parsed.build.absolute()) == config["build"] and
            str(Path(parsed.compiler).resolve()) == config["compiler"] and parsed.sanitize == config["sanitize"] and
            [str(p.resolve()) for p in parsed.inputs] == config["inputs"] and path.parent == parsed.output.resolve(),
            "precision launch differs from configuration")
    expected_sources = {str(p) for p in SOURCES} | set(config["inputs"]) | {config["compiler"]}
    require(set(manifest["source_sha256"]) == expected_sources and manifest["source_sha256"] ==
            state["source_sha256_after"] == pins(expected_sources), "precision source closure differs")
    for source in SOURCES:
        require(sha(path / "sources" / source.name) == manifest["source_sha256"][str(source)], "precision source snapshot differs")
    files = {str(p) for p in path.rglob("*") if p.is_file() and p.name != "COMPLETION.json"}
    # Prepared subdirectories have their own COMPLETION.json, included too.
    files |= {str(p) for p in path.rglob("COMPLETION.json") if p != path / "COMPLETION.json"}
    require(files == set(state["artifact_sha256"]) and pins(files) == state["artifact_sha256"], "precision artifact closure differs")
    plan, prepared = commands(config, path)
    require(manifest["plan"] == [[name, command] for name, command in plan] and manifest["prepared"] == prepared and
            len(state["commands"]) == len(plan), "precision command plan/count differs")
    build = Path(config["build"])
    require(sha(build / "mhgp8_float32_predicate_probe") == state["binary_sha256"] and
            sha(build / "probe.d") == state["dependency_file_sha256"] and
            dependencies(build / "probe.d") == state["compiled_dependencies"], "precision native/dependency closure differs")
    results = {}
    for number, ((label, command), entry) in enumerate(zip(plan, state["commands"], strict=True)):
        require(entry["path"] == f"command_{number:03}.json" and sha(path / entry["path"]) == entry["sha256"], "precision command hash")
        record = read_json(path / entry["path"])
        require(record["command"] == command and record["label"] == label and record["cwd"] == str(ROOT) and record["exit_code"] == 0,
                "precision command identity/exit")
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(record[stream+"_base64"], validate=True).decode("utf-8") == record[stream], "precision raw stream differs")
        if label not in ("compiler", "compile"):
            results[label] = preparation.load_json(record["stdout"].encode())
            require(results[label]["status"] == "passed", "precision command not scientifically passed")
    for kind in ("input_gate", "native_gate"):
        normal, optimized = (dict(results[kind+"_"+name]) for name in ("normal", "optimized"))
        require(normal.pop("optimized") is False and optimized.pop("optimized") is True and normal == optimized,
                "precision gate normal/optimized mismatch")
        require(normal["gate_sha256"] == sha(INPUT_GATE if kind == "input_gate" else NATIVE_GATE), "precision gate source binding")
        if kind == "input_gate":
            require(normal["source_sha256"] == sha(PREPARER) and normal["tests"] == 15 and normal["failures"] == normal["errors"] == 0,
                    "precision preparation gate result")
        else:
            require(normal["cases"] == 3923 and normal["rejected_mutations"] == 5 and
                    normal["binary_sha256"] == state["binary_sha256"] and normal["native_selftest"]["status"] == "passed",
                    "precision native Fraction gate result")
            spec = importlib.util.spec_from_file_location("float32_fraction_gate", NATIVE_GATE)
            gate = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(gate)
            for name in ("normal", "optimized"):
                transcript = gate.read_transcript(path / ("native_"+name), build / "mhgp8_float32_predicate_probe")
                require(all(normal[key] == value for key, value in transcript.items()), "precision native transcript differs")
    summaries = []
    for item in prepared:
        destination = Path(item["path"])
        actual = preparation.read(destination)
        label = destination.name
        require(results[label+"_read"] == results[label+"_read_O"] == actual, "precision raw reconstruction/reader equality")
        initial = results[label+"_prepare"]
        initial_fields = {"status", "path", "schema", "raw_returns", "unique_sites", "datasets", "parameters",
                          "profile", "manifest_sha256", "completion_sha256"}
        require(set(initial) == initial_fields and json.dumps(initial, sort_keys=True) ==
                json.dumps({key: actual[key] for key in initial_fields}, sort_keys=True), "precision prepare/read identity")
        require(actual["parameters"] == dict(profile=item["profile"], precision_mm=None if item["profile"] == "float32" else "1"),
                "precision selected/default profile differs")
        data = read_json(destination / "MANIFEST.json")
        require(data["raw"]["path"] == item["source"] and data["raw"]["sha256"] == manifest["source_sha256"][item["source"]],
                "precision input provenance differs")
        summaries.append(dict(**item, counts=data["counts"], encoding_profile=data["profile"]))
    require(pins(files) == state["artifact_sha256"] and sha(path / "COMPLETION.json") == closure_hash and
            pins(expected_sources) == manifest["source_sha256"], "precision closure changed while reading")
    return dict(status="passed", path=str(path), completion_sha256=closure_hash,
                commands=len(plan), input_gate=results["input_gate_normal"], native_gate=results["native_gate_normal"],
                prepared=summaries, gcp_used=False, scope=manifest["scope"])


def parser():
    result = argparse.ArgumentParser(description=__doc__)
    sub = result.add_subparsers(dest="operation", required=True)
    run_parser = sub.add_parser("run")
    run_parser.add_argument("--build", type=Path, required=True)
    run_parser.add_argument("--compiler", required=True)
    run_parser.add_argument("--sanitize", action="store_true")
    run_parser.add_argument("--output", type=Path, required=True)
    run_parser.add_argument("--inputs", nargs="*", type=Path, default=[])
    reader = sub.add_parser("read")
    reader.add_argument("--path", type=Path, required=True)
    return result


def main():
    args = parser().parse_args()
    if args.operation == "run":
        run(args)
    else:
        print(json.dumps(read(args.path), sort_keys=True))


if __name__ == "__main__":
    main()
