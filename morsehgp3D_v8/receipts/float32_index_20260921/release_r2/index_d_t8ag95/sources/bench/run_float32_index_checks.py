#!/usr/bin/env python3
"""Standalone qualification of the native float32 index, not WSPD or FULL."""
from __future__ import annotations

import argparse
import base64
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import shlex
import signal
import statistics
import struct
import subprocess
import sys
import tempfile

import prepare_lidar_precision as preparation
from run_p0_matrix import invoke, on_signal

ROOT = Path(__file__).resolve().parents[2]
V8 = ROOT / "morsehgp3D_v8"
CORE = V8 / "src/spatial/float32_index.cpp"
PROBE = V8 / "tests/float32_index_probe.cpp"
BENCH = V8 / "bench/float32_index_probe.cpp"
GATE = V8 / "tests/float32_index_gate.py"
SOURCES = (CORE, CORE.with_suffix(".hpp"), V8 / "src/core/float32_predicates.hpp",
           PROBE, BENCH, GATE, Path(__file__).resolve(),
           V8 / "bench/prepare_lidar_precision.py", V8 / "bench/run_p0_matrix.py")
SCHEMA = "mhgp8_float32_index_qualification_v1"


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def pins(paths):
    return {str(p): sha(p) for p in sorted(map(Path, paths))}


def read_json(path):
    return preparation.load_json(Path(path).read_bytes())


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False)+"\n")


def stamp():
    return datetime.now(timezone.utc).isoformat()


def inputs(directories):
    records, paths = [], []
    for number, directory in enumerate(map(Path, directories)):
        checked = preparation.read(directory)
        require(checked["parameters"] == dict(profile="float32", precision_mm=None), "input is not lossless float32")
        data = read_json(directory / "MANIFEST.json")
        paths.extend((directory / "MANIFEST.json", directory / "COMPLETION.json", Path(data["raw"]["path"])))
        names = data["dataset_order"] if number == 0 else ["full"]
        for name in names:
            item = data["datasets"][name]
            path = directory / item["points_file"]
            paths.append(path)
            records.append(dict(label=f"scene_{number}_{name}", input=str(path), n=item["sites"],
                                sha256=sha(path), prepared=str(directory), piece=name))
    return records, paths


def commands(config, target, scenes):
    build = Path(config["build"])
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
             "-ffp-contract=off", "-fno-fast-math", "-I", str(V8 / "src")]
    flags += (["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if config["sanitize"]
              else ["-O3", "-DNDEBUG"])
    compiler = config["compiler"]
    plan = [("compiler", [compiler, "--version"])]
    for name, source in (("core", CORE), ("test", PROBE), ("bench", BENCH)):
        plan.append(("compile_"+name, [compiler, *flags, "-c", str(source), "-MD", "-MF", str(build / (name+".d")),
                                       "-o", str(build / (name+".o"))]))
    for name in ("test", "bench"):
        plan.append(("link_"+name, [compiler, *flags, str(build / "core.o"), str(build / (name+".o")),
                                   "-o", str(build / ("mhgp8_float32_index_"+name))]))
    for option, name in (([], "normal"), (["-O"], "optimized")):
        plan.append(("gate_"+name, [config["python"], *option, str(GATE), "--binary", str(build / "mhgp8_float32_index_test"),
                                    "--output", str(target / ("gate_"+name))]))
    sizes = (257,) if config["sanitize"] else (8000, 16000, 32000)
    for regime in ("uniform", "terrain", "clusters"):
        for n in sizes:
            plan.append((f"synthetic_{regime}_{n}", [str(build / "mhgp8_float32_index_bench"), "--synthetic", regime,
                         "--n", str(n), "--repeat", "3"]))
    for scene in scenes:
        plan.append((scene["label"], [str(build / "mhgp8_float32_index_bench"), "--input", scene["input"], "--repeat", "3"]))
    return plan


def compiled_pins(build):
    dependencies = set()
    outputs = []
    for name in ("core", "test", "bench"):
        dep = build / (name+".d")
        outputs.extend((dep, build / (name+".o")))
        tokens = shlex.split(dep.read_text().replace("\\\n", " ").split(":", 1)[1])
        dependencies.update(Path(token).resolve(strict=True) for token in tokens)
    require({CORE, CORE.with_suffix(".hpp"), PROBE, BENCH, V8 / "src/core/float32_predicates.hpp"} <= dependencies,
            "index compiler dependencies omit sources")
    require(all(p.is_relative_to(V8) or str(p).startswith(("/usr/", "/opt/")) for p in dependencies),
            "unexpected index dependency path")
    outputs.extend(build / ("mhgp8_float32_index_"+name) for name in ("test", "bench"))
    return dict(dependencies=pins(dependencies), outputs=pins(outputs))


def run(args):
    build, compiler = args.build.absolute(), Path(args.compiler).absolute()
    require(not build.exists() and not build.is_symlink(), "fresh index build required")
    require(compiler.is_file(), "compiler driver is absent")
    directories = [str(p.resolve(strict=True)) for p in args.prepared]
    require(len(set(directories)) == len(directories), "duplicate prepared scenes")
    scenes, data_paths = inputs(directories)
    config = dict(build=str(build), compiler=str(compiler), python=sys.executable,
                  sanitize=args.sanitize, prepared=directories)
    source_pins = pins([*SOURCES, *data_paths, compiler])
    args.output.mkdir(parents=True, exist_ok=True)
    target = Path(tempfile.mkdtemp(prefix="index_", dir=args.output.resolve()))
    build.mkdir(parents=True, exist_ok=False)
    for source in SOURCES:
        destination = target / "sources" / source.relative_to(V8)
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(source.read_bytes())
    plan = commands(config, target, scenes)
    environment = dict(os.environ)
    if args.sanitize:
        environment.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    manifest = dict(schema=SCHEMA, config=config, scenes=scenes, source_sha256=source_pins,
                    started_utc=stamp(), launch=[sys.executable, *sys.argv], plan=plan,
                    environment={key: environment.get(key) for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS")},
                    git_commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                    scope="native_lossless_float32_index_not_WSPD_or_FULL", public_status="not_claimed", gcp_used=False)
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
            require(record["exit_code"] == 0, "index command failed: "+label)
            if label == "link_bench":
                state["compiled"] = compiled_pins(build)
        state["status"] = "passed"
    except BaseException as error:
        state.update(status="failed", error=f"{type(error).__name__}: {error}")
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        try:
            state["source_sha256_after"] = pins(source_pins)
            require(state["source_sha256_after"] == source_pins and sha(target / "MANIFEST.json") == manifest_pin,
                    "index source/manifest changed during qualification")
            if "compiled" in state:
                require(state["compiled"] == compiled_pins(build), "index native closure changed")
            state["artifact_sha256"] = pins(p for p in target.rglob("*") if p.is_file())
        except BaseException as error:
            state.update(status="failed", closing_error=f"{type(error).__name__}: {error}")
        state["finished_utc"] = stamp()
        write(target / "COMPLETION.json", state)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    require(state["status"] == "passed", "index qualification failed; capture retained")
    print(json.dumps(read(target), sort_keys=True))


def input_checksum(label, n, scene=None):
    """Independent serialization of the declared input; FNV is not a proof hash."""
    mask = (1 << 64)-1
    if scene is not None:
        data = Path(scene["input"]).read_bytes()
        require(len(data) == 12*n, "float32 index input byte count")
    else:
        kind = label.split("_")[1]
        data = bytearray()
        for identifier in range(n):
            mixed = (identifier+0x9e3779b97f4a7c15) & mask
            mixed = ((mixed ^ (mixed >> 30))*0xbf58476d1ce4e5b9) & mask
            mixed = ((mixed ^ (mixed >> 27))*0x94d049bb133111eb) & mask
            mixed ^= mixed >> 31
            fx, fy, fz = mixed & ((1 << 22)-1), (mixed >> 22) & ((1 << 21)-1), mixed >> 43
            x, y, z = fx-(1 << 21), fy-(1 << 20), fz-(1 << 20)
            if kind == "clusters":
                cluster = fx >> 19
                point = (((1 if cluster & 1 else -1)*(1 << 20)+(fx & ((1 << 19)-1))-(1 << 18))*2.0**-18,
                         ((1 if cluster & 2 else -1)*(1 << 22)+y)*2.0**-20,
                         ((1 if cluster & 4 else -1)*(1 << 22)+z)*2.0**-20)
            elif kind == "terrain":
                point = (x*2.0**-16, y*2.0**-15, (z+abs(x)//2+abs(y))*2.0**-20)
            else:
                point = (x*2.0**-16, y*2.0**-15, z*2.0**-15)
            data.extend(struct.pack("<fff", *point))
    digest = 14695981039346656037
    for byte in struct.pack("<Q", n)+data:
        digest = ((digest ^ byte)*1099511628211) & mask
    return f"{digest:016x}"


def measurement(value, expected_n, expected_checksum):
    require(value["schema"] == "mhgp8_float32_index_bench_v1" and value["status"] == "passed" and
            type(value["n"]) is int and value["n"] == expected_n and len(value["records"]) == 3,
            "index measurement identity/repetitions")
    n = value["n"]
    depth = (n-1).bit_length()
    floor_log = n.bit_length()-1
    mass = n*floor_log + 2*(n-(1 << floor_log))
    discrete = []
    for record in value["records"]:
        require(record["point_checksum"] == expected_checksum, "index owned input differs from declared coordinates")
        require(record["nodes"] == 2*n-1 and record["max_depth"] == depth, "index node/depth contract")
        work = record["work"]
        expected = dict(input_word_triples_copied=n, finite_points_validated=n, point_objects_constructed=n,
                        duplicate_adjacent_tests=n-1, box_endpoint_reads=6*(2*n-1), partition_rank_writes=mass,
                        partition_id_reads=5*mass, partition_id_writes=4*mass, inverse_rank_writes=n, nodes=2*n-1, leaves=n)
        require(set(work) == set(expected) | {"presort_comparisons"} and all(type(v) is int and v >= 0 for v in work.values()) and
                all(work[k] == v for k,v in expected.items()) and (work["presort_comparisons"] > 0 or n == 1),
                "index construction work differs from median proof")
        query = record["query_work"]
        require(query["node_visits"] == query["accepted_nodes"]+query["rejected_nodes"]+query["refined_nodes"] and
                query["node_visits"] <= query["axis_tests"] <= 3*query["node_visits"] and
                query["callbacks"] == query["accepted_nodes"] and query["emitted_sites"] >= n,
                "index closed-box query ledger")
        for key in ("build_ms", "query_ms"):
            require(type(record[key]) in (int,float) and math.isfinite(record[key]) and record[key] >= 0, "index invalid time")
        require(record["construction_peak_vector_bytes"] >= record["retained_bytes"] >= n*value["point_bytes"],
                "index capacity ledger")
        discrete.append({k:v for k,v in record.items() if k not in ("build_ms", "query_ms")})
    require(all(item == discrete[0] for item in discrete), "index repetitions change discrete results")
    return dict(n=n, build_ms_median=statistics.median(r["build_ms"] for r in value["records"]),
                query_ms_median=statistics.median(r["query_ms"] for r in value["records"]), **discrete[0])


def read(path):
    path = path.resolve()
    require(path.is_dir() and not any(p.is_symlink() for p in path.rglob("*")), "index capture has a link")
    manifest, state = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    closure = sha(path / "COMPLETION.json")
    require(manifest["schema"] == SCHEMA and manifest["public_status"] == "not_claimed" and
            manifest["gcp_used"] is False and state["status"] == "passed", "index capture scope/status")
    config = manifest["config"]
    parsed = parser().parse_args(manifest["launch"][2:])
    require(manifest["launch"][:2] == [config["python"], str(Path(__file__).resolve())] or
            manifest["launch"][:2] == [config["python"], str(Path(__file__).resolve().relative_to(ROOT))], "index launching script")
    require(parsed.operation == "run" and str(parsed.build.absolute()) == config["build"] and
            str(Path(parsed.compiler).absolute()) == config["compiler"] and parsed.sanitize == config["sanitize"] and
            [str(p.resolve()) for p in parsed.prepared] == config["prepared"] and parsed.output.resolve() == path.parent,
            "index launch/configuration mismatch")
    if config["sanitize"]:
        require(manifest["environment"] == dict(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"), "index sanitizer environment")
    scenes, data_paths = inputs(config["prepared"])
    expected_pins = pins([*SOURCES, *data_paths, config["compiler"]])
    require(manifest["source_sha256"] == state["source_sha256_after"] == expected_pins and
            scenes == manifest["scenes"], "index source/input closure")
    for source in SOURCES:
        require(sha(path / "sources" / source.relative_to(V8)) == expected_pins[str(source)], "index source snapshot differs")
    files = [p for p in path.rglob("*") if p.is_file() and p != path / "COMPLETION.json"]
    require(pins(files) == state["artifact_sha256"], "index artifact closure")
    build = Path(config["build"])
    require(compiled_pins(build) == state["compiled"], "index binary/dependency closure")
    plan = commands(config, path, scenes)
    require(manifest["plan"] == [[a,b] for a,b in plan] and len(state["commands"]) == len(plan), "index command plan")
    results = {}
    for number, ((label, command), entry) in enumerate(zip(plan, state["commands"], strict=True)):
        require(entry["path"] == f"command_{number:03}.json" and sha(path / entry["path"]) == entry["sha256"], "index command hash")
        record = read_json(path / entry["path"])
        require(record["label"] == label and record["command"] == command and record["cwd"] == str(ROOT) and
                record["exit_code"] == 0, "index command identity/exit")
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(record[stream+"_base64"], validate=True).decode("utf-8") == record[stream], "index raw stream differs")
        if label.startswith(("gate_", "synthetic_", "scene_")):
            results[label] = preparation.load_json(record["stdout"].encode())
            require(results[label]["status"] == "passed", "index command gate failed")
    # The independent gate also replays its archived native transcripts.
    spec = importlib.util.spec_from_file_location("index_fraction_gate", GATE)
    gate = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gate)
    for name in ("normal", "optimized"):
        replay = gate.read_transcript(path / ("gate_"+name), build / "mhgp8_float32_index_test")
        require(all(results["gate_"+name][key] == item for key,item in replay.items()), "index native oracle replay differs")
    normal, optimized = (dict(results["gate_"+name]) for name in ("normal", "optimized"))
    require(normal.pop("optimized") is False and optimized.pop("optimized") is True and normal == optimized,
            "index normal/optimized gate mismatch")
    require(normal["schema"] == "mhgp8_float32_index_gate_v1" and normal["gate_sha256"] == sha(GATE) and
            normal["binary"] == str(build / "mhgp8_float32_index_test") and
            normal["binary_sha256"] == state["compiled"]["outputs"][normal["binary"]] and
            normal["source_sha256"] == pins(gate.SOURCES) and normal["rejected_mutations"] == 11,
            "index gate source/binary/mutation binding")
    native_test = normal["native_selftest"]
    require(native_test["tests"] == 103+12*native_test["flush_modes"], "index native selftest count differs")
    fixture = gate.fixtures()[1]
    row = gate.read_command(path / "gate_normal", build / "mhgp8_float32_index_test", "case_001", gate.payload(fixture))
    require(gate.mutations(row, fixture) == 11, "index oracle mutations changed during read")
    measurements = {}
    scene_by_label = {s["label"]:s for s in scenes}
    for label, value in results.items():
        if label.startswith(("synthetic_", "scene_")):
            n = int(label.rsplit("_", 1)[1]) if label.startswith("synthetic_") else scene_by_label[label]["n"]
            checksum = input_checksum(label, n, scene_by_label.get(label))
            measurements[label] = measurement(value, n, checksum)
    require(pins(files) == state["artifact_sha256"] and sha(path / "COMPLETION.json") == closure and
            pins(expected_pins) == expected_pins and compiled_pins(build) == state["compiled"], "index final closure changed")
    return dict(status="passed", schema=SCHEMA, path=str(path), commands=len(plan), completion_sha256=closure,
                gate=results["gate_normal"], measurements=measurements, scope=manifest["scope"], gcp_used=False)


def parser():
    result = argparse.ArgumentParser(description=__doc__)
    sub = result.add_subparsers(dest="operation", required=True)
    run_parser = sub.add_parser("run")
    run_parser.add_argument("--build", type=Path, required=True)
    run_parser.add_argument("--compiler", required=True)
    run_parser.add_argument("--sanitize", action="store_true")
    run_parser.add_argument("--prepared", nargs="*", type=Path, default=[])
    run_parser.add_argument("--output", type=Path, required=True)
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
