#!/usr/bin/env python3
"""Portable read-only verification of the six closed CPU diameter captures."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path, PurePosixPath
import posixpath
import sys
import zlib

CAPTURE_MANIFEST_SHA256 = "9c65a01fc21b3f8c92bdac2df7f9f2e3473c7518b00e1d03b882cf5bf3487fd8"
ORIGINAL = "/workspaces/E-HGP/build/v7_meb_diameter_20260911"
BOOST = "/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include"
CAPTURES = {
    "o2_r1": ("o2", 0, None, None),
    "san_root_r1": ("san", 0, None, None),
    "mutant_ties_r1": ("o2", 1, "ties", "diameter.canonical"),
    "mutant_counter_r1": ("o2", 2, "counter", "diameter.paid_pairs"),
    "mutant_order_r1": ("o2", 3, "order", "diameter.paid_powers"),
    "mutant_shell_r1": ("o2", 4, "shell", "diameter.shell"),
}
PINS = {
    "anchor_meb_diameter.hpp": "e43c8c619e1f327d9e804c79de4c46412a4b70fa173b27846ad511c145eb20db",
    "gate.cpp": "61a510ccd745f8932662ae1aad8694fccfafab43ca982b42a28dccbb0ceee842",
    "record.py": "f6ef0b0f7004f5f524e28fe80378b6823d9175ad419df4e66f2849ff97b5124c",
    "variant.diff": "a5cec59f47e3900fe1d0b3df09158f2d82994d332194880c48931d16be8a0133",
    "PROOF.md": "1b68e89b5e983d700e5d8bae1cbd0c2401bc91ca6063a0fc5129114df0731dad",
    "origin.json": "e51d3483b158231eacd9e46d02e6a1d0271f2fb8ff4e2ec2667cd89c67821822",
    "prepare.py": "4d26dd56ce5cd5e1ae08df719b472eeecc7df401fcf4ff21950ebb83cec689b6",
    "source/morsehgp3D_v7/src/forest/anchor_meb.hpp": "386072c8a02bbb836d0070a10e1421418a36d3f4024ade63b4c9014c5536f786",
    "source/morsehgp3D_v7/src/forest/full_ball_tower.hpp": "83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366",
    "source/morsehgp3D_v7/oracle/local_plateau_oracle.hpp": "7a002853749784bb14a8db178fbfe637244bd3019a08ae1daf92fd20aeae670d",
}


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def safe(name):
    need(isinstance(name, str), "path type")
    path = PurePosixPath(name)
    need(name and not path.is_absolute() and str(path) == name and
         all(part not in (".", "..") for part in path.parts) and "\\" not in name,
         "unsafe path: " + name)
    return name


def entry(data):
    return {"sha256": sha(data), "bytes": len(data)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--extract", type=Path)
    parser.add_argument("--show")
    args = parser.parse_args()
    need(not (args.extract and args.show), "choose extract or show")
    root = Path(__file__).resolve().parent
    paths = list(root.rglob("*"))
    need(not any(path.is_symlink() for path in paths), "symlink in packet")
    physical = {path.relative_to(root).as_posix(): path.read_bytes()
                for path in paths if path.is_file()}
    manifest = json.loads(physical["manifest.json"])
    need(set(manifest) == {"schema", "files"} and manifest["schema"] == "meb_diameter_physical_v1", "manifest schema")
    need(set(physical) == set(manifest["files"]) | {"manifest.json"}, "physical inventory")
    for name, pin in manifest["files"].items():
        safe(name)
        need(entry(physical[name]) == pin, "physical hash: " + name)
        need(not physical[name].startswith(b"\x7fELF") and not name.endswith(".pyc"), "binary body forbidden")
    need(sha(physical["capture_manifest.json"]) == CAPTURE_MANIFEST_SHA256, "capture authority")
    capture = json.loads(physical["capture_manifest.json"])
    mapping = json.loads(physical["storage_map.json"])
    need(set(capture) == {"schema", "scope", "logical_files", "captures", "source_pins", "limits"}, "capture fields")
    need(capture["schema"] == "meb_diameter_captures_v1" and
         capture["scope"] == "private_cpu_u16_diameter_no_performance_no_cuda", "capture scope")
    need(capture["limits"] == {"compiler_pinned": False, "boost_system_headers_pinned": False,
         "ELF_bodies_distributed": False, "cuda_qualified": False, "benchmark": False,
         "inherited_results": False}, "limits")
    need(set(mapping) == set(capture["logical_files"]), "logical inventory")
    logical = {}
    used = set()
    for name, row in mapping.items():
        safe(name)
        need(set(row) == {"storage", "encoding", "stored_sha256", "stored_bytes", "sha256", "bytes"}, "storage row")
        storage = safe(row["storage"])
        used.add(storage)
        stored = physical[storage]
        need(sha(stored) == row["stored_sha256"] and len(stored) == row["stored_bytes"], "stored pin")
        need(row["encoding"] in ("raw", "zlib"), "encoding")
        data = zlib.decompress(stored) if row["encoding"] == "zlib" else stored
        if row["encoding"] == "zlib":
            decoder = zlib.decompressobj()
            need(decoder.decompress(stored) == data and decoder.eof and not decoder.unused_data, "zlib closure")
        need(entry(data) == {"sha256": row["sha256"], "bytes": row["bytes"]} == capture["logical_files"][name], "logical pin")
        need(not data.startswith(b"\x7fELF") and not name.endswith(".pyc"), "logical executable forbidden")
        logical[name] = data
    need(set(physical) == used | {"manifest.json", "capture_manifest.json", "storage_map.json", "verify.py", "publish.py", "README.md"}, "unused physical objects")
    pins = capture["source_pins"]
    need(len(pins) == 68 and all(pins.get(name) == pin for name, pin in PINS.items()), "source authority")
    expected_names = {"sources/current/" + name for name in pins}
    for name, pin in pins.items():
        need(sha(logical["sources/current/" + name]) == pin, "current source pin")
    origin = json.loads(logical["sources/current/origin.json"])
    need(origin["active_modified"] is False and origin["inherited_results"] is False and
         origin["scope"] == "private_diameter_MEB_only" and origin["variant_sha256"] == PINS["anchor_meb_diameter.hpp"], "origin scope")
    need({"source/" + name: pin for name, pin in origin["source_pins"].items()} ==
         {name: pin for name, pin in pins.items() if name.startswith("source/")}, "origin closure")
    baseline = logical["sources/current/source/morsehgp3D_v7/src/forest/anchor_meb.hpp"].decode()
    variant = logical["sources/current/anchor_meb_diameter.hpp"].decode()
    old_body = baseline[baseline.index("inline AnchorMebResult anchor_meb("):baseline.rindex("}  // namespace mhgp7")]
    new_body = variant[variant.index("inline AnchorMebResult anchor_meb_diameter("):variant.rindex("}  // namespace mhgp7")]
    difference = "".join(difflib.unified_diff(old_body.splitlines(True), new_body.splitlines(True),
        fromfile="baseline/anchor_meb", tofile="private/anchor_meb_diameter")).encode()
    need(difference == logical["sources/current/variant.diff"], "function diff")
    need(set(capture["captures"]) == set(CAPTURES), "capture inventory")
    command_count = 0
    nominals = []
    deps_reference = None
    for name, (mode, mutant, mutant_name, cause) in CAPTURES.items():
        prefix = "captures/" + name + "/"
        receipt = json.loads(logical[prefix + "receipt.json"])
        expected_names.add(prefix + "receipt.json")
        expected_names.add(prefix + "gate.d")
        need(receipt["status"] == "passed" and receipt["mode"] == mode and receipt["mutant"] == mutant, "capture status")
        need(receipt["sources_stable"] is True and receipt["snapshot_stable"] is True and
             receipt["sources_before"] == receipt["sources_after"] == receipt["snapshot_after"] == pins, "snapshot stability")
        need(receipt["scope"] == "private_diameter_MEB_and_actual_small_census_calls" and
             receipt["compiler_jobs"] == 1 and receipt["benchmark"] is False and
             receipt["device_executed"] is False and receipt["gcp_used"] is False, "receipt scope")
        for source, pin in pins.items():
            expected_names.add(prefix + "source_snapshot/" + source)
            need(sha(logical[prefix + "source_snapshot/" + source]) == pin, "compiled snapshot source")
        binary = capture["captures"][name]
        need(set(binary) == {"receipt_sha256", "binary_sha256", "binary_bytes", "body_omitted", "binary_rehashed_at_packaging"}, "binary fields")
        need(binary["receipt_sha256"] == sha(logical[prefix + "receipt.json"]) and
             receipt["binary_sha256"] == receipt["binary_after_sha256"] == binary["binary_sha256"] and
             type(binary["binary_bytes"]) is int and binary["binary_bytes"] > 0 and
             binary["body_omitted"] is True and binary["binary_rehashed_at_packaging"] is True, "ELF pins attestation only")
        flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread", "-isystem", BOOST]
        flags += ["-O2"] if mode == "o2" else ["-O1", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
        recorded = ORIGINAL + "/" + name
        compile_argv = ["g++", *flags, "-DMHGP7_DIAMETER_MUTANT=" + str(mutant), "-MMD", "-MF",
                        recorded + "/gate.d", recorded + "/source_snapshot/gate.cpp", "-o", recorded + "/gate"]
        commands = [("compile", compile_argv, 0, b"", b"")]
        if mutant:
            commands.append(("mutant", [recorded + "/gate", "--mutant-" + mutant_name], 1, b"", (cause + "\n").encode()))
        else:
            commands += [("selftest", [recorded + "/gate", "--selftest"], 0, None, b""),
                         ("unknown", [recorded + "/gate", "--unknown"], 2, b"", b""),
                         ("missing", [recorded + "/gate"], 2, b"", b"")]
        need(len(receipt["commands"]) == len(commands), "command count")
        for actual, (label, argv, code, out, err) in zip(receipt["commands"], commands):
            command_count += 1
            need(actual["name"] == label and actual["argv"] == argv and
                 actual["exit_code"] == actual["expected_exit_code"] == code, "command/code: " + name + "/" + label)
            need(actual["elapsed_s_instrumented_not_benchmark"] >= 0, "elapsed metadata")
            for stream, expected in (("stdout", out), ("stderr", err)):
                target = prefix + label + "." + stream
                expected_names.add(target)
                need(sha(logical[target]) == actual[stream + "_sha256"], "stream pin")
                if expected is not None:
                    need(logical[target] == expected, "stream cause/empty: " + target)
            if label == "mutant":
                need(actual["expected_stderr_line"] == cause, "recorded mutation cause")
        env = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1", "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"} if mode == "san" else {}
        need(receipt["sanitizer_environment"] == env, "sanitizer environment")
        dependency = logical[prefix + "gate.d"].decode().replace("\\\n", " ")
        target, dependencies = dependency.split(":", 1)
        need(target == recorded + "/gate", "dependency target")
        resolved = {posixpath.normpath(item) for item in dependencies.split()}
        dep_prefix = recorded + "/source_snapshot/"
        need(resolved and all(item.startswith(dep_prefix) for item in resolved), "dependency outside source snapshot")
        relative_deps = {item[len(dep_prefix):] for item in resolved}
        need(relative_deps <= set(pins) and {"gate.cpp", "anchor_meb_diameter.hpp", "source/morsehgp3D_v7/src/forest/full_ball_tower.hpp", "source/morsehgp3D_v7/oracle/local_plateau_oracle.hpp"} <= relative_deps, "consumed source closure")
        if deps_reference is None:
            deps_reference = relative_deps
        need(relative_deps == deps_reference, "dependencies across captures")
        if not mutant:
            nominals.append(logical[prefix + "selftest.stdout"])
    need(set(logical) == expected_names and command_count == 16, "exact final inventory/count")
    need(len(nominals) == 2 and nominals[0] == nominals[1], "O2/SAN raw stdout equality")
    result = json.loads(nominals[0])
    expected_result = {"status": "passed", "checks": 52488, "cases": 6416, "Gram_cases": 161,
        "rejections": 13, "real_census_towers": 3, "real_MEB_calls": 6255, "distance_pairs": 158088,
        "reference_supports": 437473, "variant_supports": 293135,
        "reference_powers": 579018, "variant_powers": 258574, "device_executed": False, "benchmark": False}
    need(result == expected_result, "nominal counters/nonvacuity")
    need(result["cases"] == result["Gram_cases"] + result["real_MEB_calls"], "comparison split")
    summary = {"status": "passed", "captures": len(CAPTURES), "commands": command_count,
        "logical_files": len(logical), "source_files": len(pins), "repository_dependencies": len(deps_reference),
        "checks_per_nominal": result["checks"], "cases_per_nominal": result["cases"],
        "supports_saved": result["reference_supports"] - result["variant_supports"],
        "powers_saved": result["reference_powers"] - result["variant_powers"],
        "additional_distance_pairs": result["distance_pairs"], "compiler_boost_pinned": False,
        "geometry_reexecuted": False, "device_executed": False, "benchmark": False}
    need(summary["supports_saved"] == 144338 and summary["powers_saved"] == 320444, "work deltas")
    if args.extract:
        args.extract.mkdir(parents=True, exist_ok=False)
        for name, data in logical.items():
            path = args.extract / safe(name)
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open("xb") as output:
                output.write(data)
        summary["extracted_files"] = len(logical)
    if args.show:
        safe(args.show)
        need(args.show in logical, "unknown logical file")
        sys.stdout.buffer.write(logical[args.show])
    else:
        print(json.dumps(summary, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, ValueError, KeyError, OSError, zlib.error) as error:
        print("meb_diameter_packet: " + str(error), file=sys.stderr)
        raise SystemExit(1)
