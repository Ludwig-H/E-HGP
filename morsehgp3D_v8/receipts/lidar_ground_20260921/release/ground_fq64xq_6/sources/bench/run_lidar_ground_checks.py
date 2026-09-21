#!/usr/bin/env python3
"""LIVE qualification of a ground-mask adapter and lossless masked preparation.

Consumes a separately closed Patchwork++ build; never builds/downloads/runs GCP.
Masks classify approximately, while IDs/coordinates/hashes are checked exactly.
Fresh processes repeat complete frames. Neither segmentation quality nor HGP,
FULL, GPU or a whole-frame latency contract is certified by these receipts.
"""
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
import signal
import struct
import subprocess
import sys
import tempfile

from run_p0_matrix import invoke, on_signal, parse_result

ROOT = Path(__file__).resolve().parents[2]
V8 = ROOT / "morsehgp3D_v8"
HERE = Path(__file__).resolve()
BUILDER = V8 / "bench/build_patchwork_ground.py"
PREPARER = V8 / "bench/prepare_lidar_ground.py"
GATE = V8 / "tests/lidar_ground_test.py"
SOURCES = (HERE, BUILDER, PREPARER, GATE, V8 / "bench/prepare_lidar_precision.py",
           V8 / "bench/patchwork_ground_probe.cpp", V8 / "bench/run_p0_matrix.py")
SCHEMA = "mhgp8_lidar_ground_checks_v1"
SCOPE = "approximate_ground_mask_and_exact_retained_coordinates_not_segmentation_quality_or_HGP"
ENV_KEYS = ("PATH", "LANG", "LC_ALL", "TZ", "LD_LIBRARY_PATH", "LD_PRELOAD", "PYTHONPATH",
            "ASAN_OPTIONS", "UBSAN_OPTIONS", "OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS")
PROBE_SCHEMA = "mhgp8_patchwork_ground_probe_v1"
ENCODING = "u8_0_unknown_1_ground_2_nonground"
TIMES = ("read", "prepare", "segment", "mask", "write", "wall")
REFUSALS = dict(missing="cannot open input", truncated="input size is not a multiple of 16",
                nonfinite="nonfinite XYZ", parameter_nan="expected a finite numeric option",
                parameter_negative="invalid ground parameters", range="invalid ground parameters",
                duplicate_option="duplicate option", unknown_option="unknown option",
                missing_value="option lacks a value", existing_output="cannot create fresh mask")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def pins(paths):
    return {str(path): sha(path) for path in sorted(map(Path, paths))}


def stamp():
    return datetime.now(timezone.utc).isoformat()


def write_bytes(path, value):
    with path.open("xb") as stream:
        stream.write(value)


def write(path, value):
    write_bytes(path, (json.dumps(value, indent=2, sort_keys=True, allow_nan=False)+"\n").encode())


def load(path):
    return parse_result(Path(path).read_bytes())


def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    require(spec is not None and spec.loader is not None, "missing qualification dependency")
    result = importlib.util.module_from_spec(spec)
    sys.modules[name] = result
    spec.loader.exec_module(result)
    return result


def artifact_pins(path):
    require(not any(item.is_symlink() for item in path.rglob("*")), "linked qualification artifact")
    return {str(item.relative_to(path)): sha(item) for item in sorted(path.rglob("*"))
            if item.is_file() and item != path / "COMPLETION.json"}


def fnv(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & ((1 << 64)-1)
    return f"{value:016x}"


def fixture_bytes():
    # Sensor-frame plane with raised points and duplicate returns. This is a
    # non-vacuity fixture, NOT an independently labelled segmentation dataset.
    plane = [(x/2, y/2, -1.723, .5) for x in range(-24, 25) for y in range(-24, 25)]
    plane += [(7+x/8, 2+y/8, -.25, .7) for x in range(8) for y in range(8)]
    plane += plane[700:704]
    packed = lambda rows: b"".join(struct.pack("<ffff", *row) for row in rows)
    data = packed(plane)
    reflectance = b"".join(data[i:i+12]+struct.pack("<I", 0x7fc12345) for i in range(0, len(data), 16))
    unknown = packed([(0, 0, 0, .1), (100, 0, 0, .2), (2**41, 0, 0, .3),
                      (-100, 0, 0, .4), (0, 0, -10, .5)])
    unknown += struct.pack("<IIII", 0x40a00000, 0, 0x00800000, 0x7fc12345)
    return {"empty.bin": b"", "unknown.bin": unknown, "plane.bin": data,
            "reflectance.bin": reflectance, "truncated.bin": b"\0"*17,
            "nonfinite.bin": struct.pack("<IIII", 0x7f800000, 0, 0, 0)}


def native_command(binary, input_path, output_path):
    return [binary, "--input", str(input_path), "--output", str(output_path)]


def plan(config, path, binary):
    result = [("test_normal", [config["python"], "-B", str(GATE)], 0),
              ("test_optimized", [config["python"], "-B", "-O", str(GATE)], 0),
              ("describe", [binary, "--describe"], 0)]
    for name in ("empty", "unknown", "plane", "reflectance"):
        result.append(("fixture_"+name, native_command(binary, path/"fixtures"/(name+".bin"),
                                                       path/(name+".u8")), 0))
    default = native_command(binary, path/"fixtures/unknown.bin", path/"rejected.u8")
    rejects = (("missing", native_command(binary, path/"fixtures/missing.bin", path/"rejected.u8")),
               ("truncated", native_command(binary, path/"fixtures/truncated.bin", path/"rejected.u8")),
               ("nonfinite", native_command(binary, path/"fixtures/nonfinite.bin", path/"rejected.u8")),
               ("parameter_nan", [*default, "--sensor-height", "nan"]),
               ("parameter_negative", [*default, "--distance", "-1"]),
               ("range", [*default, "--min-range", "10", "--max-range", "10"]),
               ("duplicate_option", [*default, "--input", str(path/"fixtures/unknown.bin")]),
               ("unknown_option", [*default, "--other", "1"]),
               ("missing_value", [*default, "--distance"]),
               ("existing_output", native_command(binary, path/"fixtures/unknown.bin", path/"unknown.u8")))
    result.extend(("reject_"+name, command, 1) for name, command in rejects)
    for number, source in enumerate(config["inputs"]):
        for repeat in range(3):
            result.append((f"scene_{number:02}_repeat_{repeat}", native_command(
                binary, source, path/f"scene_{number:02}_repeat_{repeat}.u8"), 0))
        for profile in ("float32", "grid"):
            destination = path/f"scene_{number:02}_{profile}"
            result.append((f"prepare_{number:02}_{profile}", [config["python"], "-B", str(PREPARER), "prepare",
                "--input", source, "--mask", str(path/f"scene_{number:02}_repeat_0.u8"),
                "--mask-receipt", str(path/f"scene_{number:02}_mask.json"), "--output", str(destination),
                "--profile", profile], 0))
            for name, optimize in (("normal", []), ("optimized", ["-O"])):
                result.append((f"read_{number:02}_{profile}_{name}", [config["python"], "-B", *optimize,
                    str(PREPARER), "read", "--path", str(destination)], 0))
    return result


def integer(value, name):
    require(type(value) is int and 0 <= value <= (1 << 64)-1, name+" must be u64, not bool/float")
    return value


def validate_description(row, authority):
    require(set(row) == {"schema", "upstream_commit", "encoding", "parameters"} and
            row["schema"] == PROBE_SCHEMA and row["encoding"] == ENCODING and
            row["upstream_commit"] == authority["commit"], "native description/build identity differs")
    p = row["parameters"]
    fields = {"verbose", "enable_RNR", "enable_RVPF", "enable_TGR", "num_iter", "num_lpr", "num_min_pts",
              "num_zones", "num_rings_of_interest", "RNR_ver_angle_thr", "RNR_intensity_thr", "sensor_height",
              "th_seeds", "th_dist", "th_seeds_v", "th_dist_v", "max_range", "min_range", "uprightness_thr",
              "adaptive_seed_selection_margin", "intensity_thr", "max_flatness_storage", "max_elevation_storage",
              "num_sectors_each_zone", "num_rings_each_zone", "elevation_thr", "flatness_thr"}
    require(type(p) is dict and set(p) == fields and p["verbose"] is False and p["enable_RNR"] is False and
            p["enable_RVPF"] is True and p["enable_TGR"] is True, "effective parameter schema/flags differ")
    require(p["sensor_height"] == 1.723 and p["min_range"] == 2.7 and p["max_range"] == 80.0 and
            p["th_dist"] == .125 and p["th_seeds"] == .125 and p["uprightness_thr"] == .707,
            "fixed qualification parameters differ")


def validate_native(row, raw, mask, description):
    fields = {"schema", "upstream_commit", "encoding", "raw_returns", "processed_returns", "algorithm_invocations", "counts",
              "unknown_reasons", "nonfinite_reflectance", "numeric_abs_limit", "rnr_enabled", "reflectance_policy",
              "fresh_state", "eigen_threads", "openmp", "noise_status_available", "parameters",
              "input_fnv1a64", "mask_fnv1a64", "timing_ms"}
    require(set(row) == fields and all(row[key] == description[key] for key in
            ("schema", "upstream_commit", "encoding", "parameters")), "native output schema/parameters differ")
    require(len(raw) % 16 == 0 and integer(row["raw_returns"], "returns") == len(raw)//16 == len(mask) and
            all(value in (0, 1, 2) for value in mask), "mask length/status domain differs")
    expected_counts = dict(unknown=mask.count(0), ground=mask.count(1), nonground=mask.count(2))
    require(type(row["counts"]) is dict and set(row["counts"]) == set(expected_counts) and
            all(integer(row["counts"][name], name) == value for name, value in expected_counts.items()), "mask counts differ")
    require(row["fresh_state"] is True and row["rnr_enabled"] is False and row["openmp"] is False and
            row["noise_status_available"] is False and type(row["eigen_threads"]) is int and row["eigen_threads"] == 1 and
            row["reflectance_policy"] == "ignored_preserved_in_raw" and row["numeric_abs_limit"] == 2**40,
            "native policy differs")
    require(row["input_fnv1a64"] == fnv(raw) and row["mask_fnv1a64"] == fnv(mask), "native raw/mask digest differs")
    reasons = row["unknown_reasons"]
    require(type(reasons) is dict and set(reasons) == {"outside_range", "numeric_domain", "upstream_z_sentinel", "upstream_unassigned"},
            "unknown reason fields differ")
    for name in reasons:
        integer(reasons[name], name)
    numeric = sentinel = outside = nonfinite_reflectance = 0
    minimum, maximum = description["parameters"]["min_range"], description["parameters"]["max_range"]
    for identifier, words in enumerate(struct.iter_unpack("<IIII", raw)):
        require(all((word & 0x7f800000) != 0x7f800000 for word in words[:3]), "accepted nonfinite XYZ")
        nonfinite_reflectance += (words[3] & 0x7f800000) == 0x7f800000
        xyz = struct.unpack("<fff", struct.pack("<III", *words[:3]))
        excluded = False
        if any(abs(value) > 2**40 for value in xyz):
            numeric += 1; excluded = True
        elif words[2] == 0x00800000:
            sentinel += 1; excluded = True
        elif not minimum < math.hypot(xyz[0], xyz[1]) <= maximum:
            outside += 1; excluded = True
        require(not excluded or mask[identifier] == 0, "adapter-excluded return was classified/removed")
    require(reasons == dict(outside_range=outside, numeric_domain=numeric, upstream_z_sentinel=sentinel,
                           upstream_unassigned=mask.count(0)-outside-numeric-sentinel), "unknown accounting differs")
    require(integer(row["processed_returns"], "processed") == len(mask)-outside-numeric-sentinel and
            integer(row["nonfinite_reflectance"], "reflectance") == nonfinite_reflectance, "processed/reflection counts differ")
    require(integer(row["algorithm_invocations"], "algorithm invocations") == int(row["processed_returns"] != 0),
            "empty processed cloud must bypass the third-party algorithm")
    times = row["timing_ms"]
    require(type(times) is dict and set(times) == set(TIMES) and all(type(value) in (int, float) and
            math.isfinite(value) and value >= 0 for value in times.values()), "invalid native durations")
    require(math.isclose(sum(times[name] for name in TIMES[:-1]), times["wall"], rel_tol=1e-9, abs_tol=1e-6),
            "native duration decomposition differs")


def producer(authority, description, command_sha256):
    return dict(method="Patchwork++", upstream_commit=authority["commit"], binary_sha256=authority["binary_sha256"],
                build_completion_sha256=authority["completion_sha256"], wrapper_sha256=sha(V8/"bench/patchwork_ground_probe.cpp"),
                native_command_sha256=command_sha256, parameters=description["parameters"], fresh_state=True,
                labels_used=False, eigen_threads=1, openmp=False, rnr_enabled=False,
                unknown_policy="retained", reflectance_policy="ignored_preserved_in_raw",
                classification="approximate_not_semantic_ground_truth")


def validate_tests(normal, optimized):
    first, second = dict(normal), dict(optimized)
    require(first.pop("optimized") is False and second.pop("optimized") is True and first == second,
            "normal/optimized preparation tests differ")
    require(first == dict(schema="mhgp8_lidar_ground_test_v1", status="passed", tests=20, failures=0, errors=0,
                         source_sha256=sha(PREPARER), precision_source_sha256=sha(V8/"bench/prepare_lidar_precision.py"),
                         test_sha256=sha(GATE)), "preparation test identity/inventory differs")


def validate_results(config, path, authority, rows, command_hashes):
    """Rejudge semantic outputs both BEFORE success closure and during reading."""
    validate_tests(rows["test_normal"], rows["test_optimized"])
    validate_description(rows["describe"], authority)
    expected_files = {"MANIFEST.json", "BUILD_AUTHORITY.json"}
    expected_files.update(str(Path("sources")/source.relative_to(V8)) for source in SOURCES)
    expected_files.update("fixtures/"+name for name in fixture_bytes())
    for name, data in fixture_bytes().items():
        require((path/"fixtures"/name).read_bytes() == data, "native fixture differs")
    for number, (label, command, expected_exit) in enumerate(plan(config, path, authority["binary"])):
        filename = f"command_{number:03}.json"
        expected_files.add(filename)
        require(sha(path/filename) == command_hashes[label], "command changed after validation")
        if expected_exit == 0 and label.startswith(("fixture_", "scene_")):
            validate_native(rows[label], Path(command[2]).read_bytes(), Path(command[4]).read_bytes(), rows["describe"])
            expected_files.add(str(Path(command[4]).relative_to(path)))
    require((path/"unknown.u8").read_bytes() == bytes(len(fixture_bytes()["unknown.bin"])//16) and
            (path/"empty.u8").read_bytes() == b"", "unknown/empty fixture changed")
    require((path/"plane.u8").read_bytes() == (path/"reflectance.u8").read_bytes() and
            rows["fixture_plane"]["counts"]["ground"] > 0, "reflectance independence/ground non-vacuity differs")
    preparer = module(PREPARER, "mhgp8_ground_results_preparer")
    scenes = []
    for number, source in enumerate(config["inputs"]):
        native = [rows[f"scene_{number:02}_repeat_{repeat}"] for repeat in range(3)]
        masks = [(path/f"scene_{number:02}_repeat_{repeat}.u8").read_bytes() for repeat in range(3)]
        require(masks[0] == masks[1] == masks[2] and all({key: value for key, value in row.items() if key != "timing_ms"} ==
            {key: value for key, value in native[0].items() if key != "timing_ms"} for row in native[1:]),
            "fresh-state full-scan repetitions differ")
        expected = preparer.descriptor(Path(source).read_bytes(), masks[0], producer(
            authority, rows["describe"], command_hashes[f"scene_{number:02}_repeat_0"]))
        expected_files.add(f"scene_{number:02}_mask.json")
        require(load(path/f"scene_{number:02}_mask.json") == expected, "mask descriptor provenance differs")
        prepared = {}
        for profile in ("float32", "grid"):
            destination = path/f"scene_{number:02}_{profile}"
            result = preparer.read(destination)
            require(result == rows[f"prepare_{number:02}_{profile}"] == rows[f"read_{number:02}_{profile}_normal"] ==
                    rows[f"read_{number:02}_{profile}_optimized"], "closed preparation/replays differ")
            require(result["parameters"] == dict(profile=profile, precision_mm=None if profile == "float32" else "1") and
                    result["datasets"] == 7, "prepared profile/grain/partition inventory differs")
            # The preparer has just checked its exact 27-payload inventory,
            # manifest and nested COMPLETION. None is omitted by basename.
            expected_files.update(str(item.relative_to(path)) for item in destination.iterdir())
            prepared[profile] = result
        scenes.append(dict(input=source, input_sha256=sha(source), mask_sha256=hashlib.sha256(masks[0]).hexdigest(),
                           raw_returns=native[0]["raw_returns"], counts=native[0]["counts"],
                           unknown_reasons=native[0]["unknown_reasons"], timing_ms=[row["timing_ms"] for row in native], prepared=prepared))
    require(set(artifact_pins(path)) == expected_files, "successful capture artifact inventory differs")
    return scenes


def configuration(args):
    inputs = [str(path.resolve(strict=True)) for path in args.inputs]
    require(len(inputs) == len(set(inputs)) and all(Path(path).is_file() for path in inputs), "input scans must be distinct files")
    require((args.small_only and not inputs) or (not args.small_only and len(inputs) == 3),
            "use three complete scans, or --small-only without inputs")
    require(not args.sanitize or args.small_only, "sanitizer qualification uses small fixtures only")
    return dict(build=str(args.build.resolve(strict=True)), inputs=inputs, small_only=args.small_only,
                sanitize=args.sanitize, python=sys.executable)


def run(args):
    config = configuration(args)
    args.output.mkdir(parents=True, exist_ok=True)
    path = Path(tempfile.mkdtemp(prefix="ground_", dir=args.output.resolve()))
    before = pins((*SOURCES, *map(Path, config["inputs"])))
    environment = dict(os.environ)
    if config["sanitize"]:
        environment.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    recorded_environment = {name: environment.get(name) for name in ENV_KEYS}
    manifest = dict(schema=SCHEMA, scope=SCOPE, public_status="not_claimed", gcp_used=False, config=config,
                    started_utc=stamp(), launch=[sys.executable, *sys.argv], environment=recorded_environment,
                    source_input_sha256=before,
                    git_commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip())
    write(path/"MANIFEST.json", manifest)
    state = dict(status="running", manifest_sha256=sha(path/"MANIFEST.json"), commands=[], started_utc=stamp())
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    print(json.dumps(dict(path=str(path), status="starting")), flush=True)
    try:
        for source in SOURCES:
            destination = path/"sources"/source.relative_to(V8)
            destination.parent.mkdir(parents=True, exist_ok=True)
            write_bytes(destination, source.read_bytes())
        (path/"fixtures").mkdir()
        for name, data in fixture_bytes().items():
            write_bytes(path/"fixtures"/name, data)
        builder = module(BUILDER, "mhgp8_ground_qualification_builder")
        authority = builder.read(Path(config["build"]), check_live=True)
        require(authority["status"] == "passed", "native build is not closed")
        require(authority["sanitize"] == config["sanitize"], "build/sanitizer options differ")
        state["build_authority_before"] = authority
        write(path/"BUILD_AUTHORITY.json", authority)
        results, command_hashes = {}, {}
        preparer = module(PREPARER, "mhgp8_ground_qualification_preparer")
        for number, (label, command, expected_exit) in enumerate(plan(config, path, authority["binary"])):
            record = dict(label=label, command=command, cwd=str(ROOT), environment=recorded_environment,
                          expected_exit=expected_exit, exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
            print(label, flush=True)
            filename = f"command_{number:03}.json"
            try:
                invoke(command, environment, ROOT, record, new_session=True)
            finally:
                write(path/filename, record)
                state["commands"].append(dict(path=filename, sha256=sha(path/filename)))
                command_hashes[label] = sha(path/filename)
            require(type(record["exit_code"]) is int and record["exit_code"] == expected_exit, "unexpected exit: "+label)
            if label.startswith("reject_"):
                require(record["stdout"] == "" and record["stderr"].startswith("patchwork ground probe: "+REFUSALS[label[7:]]) and
                        not (path/"rejected.u8").exists(), "refusal must be explicit and leave no output")
                continue
            row = parse_result(base64.b64decode(record["stdout_base64"], validate=True))
            results[label] = row
            if label == "test_optimized":
                validate_tests(results["test_normal"], row)
            elif label == "describe":
                validate_description(row, authority)
            elif label.startswith(("fixture_", "scene_")):
                raw, mask = Path(command[2]).read_bytes(), Path(command[4]).read_bytes()
                validate_native(row, raw, mask, results["describe"])
                if label.startswith("scene_") and label.endswith("_repeat_0"):
                    descriptor = preparer.descriptor(raw, mask, producer(authority, results["describe"], sha(path/filename)))
                    write(path/(label.removesuffix("_repeat_0")+"_mask.json"), descriptor)
        validate_results(config, path, authority, results, command_hashes)
        state["validated_artifact_sha256"] = artifact_pins(path)
        state["status"] = "passed"
    except BaseException as error:
        state.update(status="failed", error=f"{type(error).__name__}: {error}")
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        try:
            state["source_input_sha256_after"] = pins(before)
            require(state["source_input_sha256_after"] == before and sha(path/"MANIFEST.json") == state["manifest_sha256"],
                    "source/input/manifest closure changed")
            if "build_authority_before" in state:
                state["build_authority_after"] = builder.read(Path(config["build"]), check_live=True)
                require(state["build_authority_after"] == state["build_authority_before"], "build/dependency closure changed")
            state["artifact_sha256"] = artifact_pins(path)
            if state["status"] == "passed":
                require(state["artifact_sha256"] == state["validated_artifact_sha256"], "validated artifacts changed at closure")
                for source in SOURCES:
                    require(sha(path/"sources"/source.relative_to(V8)) == before[str(source)], "source snapshot changed")
        except BaseException as error:
            state.update(status="failed", closing_error=f"{type(error).__name__}: {error}")
        state["finished_utc"] = stamp()
        write(path/"COMPLETION.json", state)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    require(state["status"] == "passed", "ground qualification failed; capture preserved at "+str(path))
    print(json.dumps(read(path), sort_keys=True))


def read(path):
    path = path.resolve(strict=True)
    closure = sha(path/"COMPLETION.json")
    manifest, state = load(path/"MANIFEST.json"), load(path/"COMPLETION.json")
    require(manifest["schema"] == SCHEMA and manifest["scope"] == SCOPE and manifest["public_status"] == "not_claimed" and
            manifest["gcp_used"] is False and state["status"] == "passed", "qualification scope/status differs")
    config = manifest["config"]
    launch = manifest["launch"]
    require(launch[:2] in ([config["python"], str(HERE)], [config["python"], str(HERE.relative_to(ROOT))]), "launching script differs")
    args = parser().parse_args(launch[2:])
    require(args.operation == "run" and configuration(args) == config and args.output.resolve() == path.parent,
            "command/configuration binding differs")
    require(type(config["small_only"]) is bool and type(config["sanitize"]) is bool and
            set(manifest["environment"]) == set(ENV_KEYS) and all(value is None or type(value) is str
            for value in manifest["environment"].values()), "configuration/environment shape differs")
    if config["sanitize"]:
        require(manifest["environment"]["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1" and
                manifest["environment"]["UBSAN_OPTIONS"] == "halt_on_error=1:print_stacktrace=1", "sanitizer environment differs")
    expected_pins = pins((*SOURCES, *map(Path, config["inputs"])))
    require(manifest["source_input_sha256"] == state["source_input_sha256_after"] == expected_pins and
            sha(path/"MANIFEST.json") == state["manifest_sha256"], "source/input closure differs")
    require(artifact_pins(path) == state["artifact_sha256"] == state["validated_artifact_sha256"], "artifact closure differs")
    for source in SOURCES:
        require(sha(path/"sources"/source.relative_to(V8)) == expected_pins[str(source)], "source snapshot differs")
    for name, data in fixture_bytes().items():
        require((path/"fixtures"/name).read_bytes() == data, "native fixture differs")
    builder = module(BUILDER, "mhgp8_ground_reader_builder")
    authority = builder.read(Path(config["build"]), check_live=True)
    require(authority == state["build_authority_before"] == state["build_authority_after"] == load(path/"BUILD_AUTHORITY.json"),
            "native build authority changed")
    require(authority["sanitize"] == config["sanitize"], "build mode differs")
    expected_plan = plan(config, path, authority["binary"])
    require(len(state["commands"]) == len(expected_plan), "command inventory differs")
    rows, command_hashes = {}, {}
    for number, ((label, command, expected_exit), entry) in enumerate(zip(expected_plan, state["commands"], strict=True)):
        require(entry["path"] == f"command_{number:03}.json" and sha(path/entry["path"]) == entry["sha256"], "command hash differs")
        record = load(path/entry["path"])
        require(record["label"] == label and record["command"] == command and record["cwd"] == str(ROOT) and
                record["environment"] == manifest["environment"] and record["expected_exit"] == expected_exit and
                type(record["exit_code"]) is int and record["exit_code"] == expected_exit, "raw command identity/exit differs")
        for stream in ("stdout", "stderr"):
            raw = base64.b64decode(record[stream+"_base64"], validate=True)
            require(raw.decode("utf-8", errors="replace") == record[stream], "raw stream binding differs")
        if label.startswith("reject_"):
            require(record["stdout"] == "" and record["stderr"].startswith("patchwork ground probe: "+REFUSALS[label[7:]]) and
                    not (path/"rejected.u8").exists(), "native refusal differs")
        else:
            rows[label] = parse_result(base64.b64decode(record["stdout_base64"], validate=True))
        command_hashes[label] = entry["sha256"]
    scenes = validate_results(config, path, authority, rows, command_hashes)
    require(artifact_pins(path) == state["artifact_sha256"] and sha(path/"COMPLETION.json") == closure and
            pins(expected_pins) == expected_pins and builder.read(Path(config["build"]), check_live=True) == authority,
            "final LIVE closure changed")
    return dict(schema=SCHEMA, status="passed", path=str(path), completion_sha256=closure, commands=len(expected_plan),
                preparation_tests=20, native_fixtures=4, native_refusals=10, repeat_count=3 if scenes else 0,
                scenes=scenes, native_binary_sha256=authority["binary_sha256"], scope=SCOPE, gcp_used=False)


def parser():
    result = argparse.ArgumentParser(description=__doc__)
    sub = result.add_subparsers(dest="operation", required=True)
    runner = sub.add_parser("run")
    runner.add_argument("--build", type=Path, required=True)
    runner.add_argument("--output", type=Path, required=True)
    runner.add_argument("--inputs", type=Path, nargs="*", default=[])
    runner.add_argument("--small-only", action="store_true")
    runner.add_argument("--sanitize", action="store_true")
    reader = sub.add_parser("read")
    reader.add_argument("--path", type=Path, required=True)
    return result


if __name__ == "__main__":
    arguments = parser().parse_args()
    try:
        if arguments.operation == "run":
            run(arguments)
        else:
            print(json.dumps(read(arguments.path), sort_keys=True))
    except (Exception, KeyboardInterrupt) as error:
        print(json.dumps(dict(status="failed", error=f"{type(error).__name__}: {error}")), file=sys.stderr)
        raise SystemExit(1) from error
