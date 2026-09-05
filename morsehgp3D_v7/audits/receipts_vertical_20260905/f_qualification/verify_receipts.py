#!/usr/bin/env python3
"""Independently read captured closed F receipts; never build or run CTest."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
F_CLI = "ee29d3d5cfb49a728fa9dfa44fdb85a5a6043c941b1f61d4a6d9531ea4671f85"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def unique(items: list) -> dict:
    result = {}
    for key, value in items:
        require(key not in result, "duplicate key: " + key)
        result[key] = value
    return result


def decode(data: bytes) -> object:
    return json.loads(data, object_pairs_hook=unique)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def judge(xml: bytes, inventory: dict, expected: list, log: str) -> dict:
    require(len(expected) in (48, 339), "nonvacuum expected count")
    require(len(set(expected)) == len(expected), "unique expected names")
    names = [test["name"] for test in inventory["tests"]]
    require(len(names) == len(set(names)) == len(expected) and set(names) == set(expected), "inventory names")
    root = ET.fromstring(xml)
    tests = list(root.iter("testcase"))
    actual = [test.attrib["name"] for test in tests]
    require(len(actual) == len(set(actual)) == len(expected) and set(actual) == set(expected), "JUnit names")
    require(int(root.attrib["tests"]) == len(expected), "JUnit total")
    for field in ("failures", "errors", "skipped", "disabled"):
        require(int(root.attrib.get(field, "0")) == 0, "JUnit " + field)
    chunks = re.split(r"^\d+/\d+ Testing: (.+)\n", log, flags=re.M)
    require(len(chunks) == 2 * len(expected) + 1, "LastTest block count")
    blocks = unique(list(zip(chunks[1::2], chunks[2::2])))
    require(set(blocks) == set(expected), "LastTest names")
    require(len(re.findall(r"^Start testing: .+$", log, re.M)) == 1 and len(re.findall(r"^End testing: .+$", log, re.M)) == 1, "LastTest terminality")
    outputs = {}
    truncated = []
    for test in tests:
        name = test.attrib["name"]
        require(test.attrib.get("status") == "run" and float(test.attrib["time"]) >= 0, "JUnit run state")
        require(not any(node.tag in {"failure", "error", "skipped"} for node in test.iter()), "JUnit failure node")
        block = blocks[name]
        require(len(re.findall(r"^Test Passed\.$", block, re.M)) == 1, "LastTest nonpass " + name)
        require(not re.search(r"^Test (Failed|Not Run|Timeout)", block, re.M), "LastTest failure")
        require(re.search(r'^"' + re.escape(name) + r'" end time: ', block, re.M) is not None, "LastTest named end")
        output = re.findall(r"Output:\n-+\n(.*?)\n<end of output>", block, re.S)
        require(len(output) == 1, "LastTest output boundary")
        xml_output = test.findtext("system-out") or ""
        if output[0].strip() != xml_output.strip():
            suffix = "...\n[This part of the test output was removed since it exceeds the threshold of 1024 bytes.]\n"
            require(xml_output.endswith(suffix), "XML/raw output equality or declared truncation " + name)
            prefix = xml_output[:-len(suffix)]
            require(len(prefix.encode()) == 1024 and output[0].startswith(prefix), "XML/raw exact 1024-byte prefix " + name)
            require(len(output[0].encode()) > 1024, "XML truncation nonvacuum " + name)
            truncated.append({"name": name, "verified_prefix_bytes": 1024, "raw_output_bytes": len(output[0].encode())})
        outputs[name] = output[0].strip()
    nominal = outputs["mhgp7_witness_stack"]
    semantic = outputs["mhgp7_witness_stack_semantic"]
    require(nominal.startswith("witness_stack_gate=passed case=all reason=none mutant=none public_status=not_claimed"), "nominal marker")
    for marker in ("allocation_observer=enabled", "helper_checks=8 overflow_allocations=9 allocation_failures=2", "queries=48960 guarded_queries=48960", "permutation_comparisons=32640", "max_height=48 max_topological_frontier=49", "nodes=808908 corners=74172", "reference_new_calls=118404 candidate_new_calls=0"):
        require(marker in nominal, "nominal nonvacuum " + marker)
    for marker in ("allocation_observer=disabled", "queries=48960 guarded_queries=0", "nodes=808908 corners=74172", "reference_new_calls=0 candidate_new_calls=0"):
        require(marker in semantic, "semantic scope " + marker)
    for name in ("mhgp7_witness_stack_lane_mask_mutant", "mhgp7_witness_stack_semantic_lane_mask_mutant"):
        require('"-DEXPECTED=4"' in blocks[name] and '"-DARGS=--case=lane-mask --mutant=witness-no-lane-mask"' in blocks[name], "real mutant expected command")
        require(outputs[name].startswith("witness_stack_gate=mutant_killed case=lane-mask reason=witness.lane_mask_double_credit mutant=witness-no-lane-mask public_status=not_claimed"), "real mutant output")
        require("lane_mask_fixtures=1 lane_mask_nominal=3 lane_mask_product=8" in outputs[name], "mutant literal 3-to-8")
    return {"tests": len(expected), "names": sorted(expected), "last_test_passed_blocks": len(blocks), "XML_outputs_verified_against_LastTest": True, "junit_truncated_outputs": truncated, "failure_error_skipped_nodes": 0, "witness_outputs": {name: output for name, output in outputs.items() if name.startswith("mhgp7_witness_stack")}}


def inspect(live: bool) -> dict:
    capture = decode((HERE / "capture_manifest.json").read_bytes())
    e_names_bytes = (HERE / "E_expected_names.json").read_bytes()
    require(sha(e_names_bytes) == capture["E_expected_names"]["sha256"], "E inventory pin")
    e_names = decode(e_names_bytes)
    require(len(e_names) == len(set(e_names)) == 324, "E inventory floor")
    entrypoint = decode((HERE / "entrypoint_F_variant.json").read_bytes())["variant"]["pinned_sources"]
    require(len(entrypoint) == 118, "118 entrypoint pins")
    results = {}
    for mode, entries in capture["modes"].items():
        def raw(name: str) -> bytes:
            entry = entries[name]
            relative = Path(entry["snapshot_path"])
            require(not relative.is_absolute() and ".." not in relative.parts, "unsafe snapshot path")
            path = HERE / relative
            require(path.resolve().is_relative_to(HERE), "snapshot symlink escape")
            data = path.read_bytes()
            if entry["compression"] == "gzip":
                data = gzip.decompress(data)
            else:
                require(entry["compression"] == "none", "compression kind")
            require(sha(data) == entry["sha256"] and len(data) == entry["bytes"], "capture pin " + name)
            return data

        def obj(name: str) -> object:
            return decode(raw(name))

        require(mode in ("release", "sanitized", "full"), "mode")
        expected_count = 339 if mode == "full" else 48
        expected = obj("expected_names.json")
        require(len(expected) == expected_count, "campaign count")
        names_f = {name for name in expected if name.startswith("mhgp7_witness_stack")}
        require(len(names_f) == 15, "15 F gates")
        if mode == "full":
            require(set(expected) == set(e_names) | names_f and not set(e_names) & names_f, "F339 = E324 inventory plus F15, executions separate")
        manifest = obj("receipt_manifest.json")
        require(len(manifest) == 31 and len(entries) == 35, "closed receipt coverage")
        original = {name for name, entry in entries.items() if "extra" not in entry}
        require({entry["path"] for entry in manifest} == original - {"receipt_manifest.json"}, "source manifest exhaustiveness")
        for entry in manifest:
            data = raw(entry["path"])
            require(len(data) == entry["size"] and sha(data) == entry["sha256"], "source manifest integrity")
        summary = obj("summary.json")
        require(summary["status"] == "passed" and summary["failure"] is None and summary["final_errors"] == [], "closed successful summary")
        judged = judge(raw("ctest.junit.xml"), obj("inventory.stdout"), expected, raw("LastTest.stdout").decode())
        require(judged["names"] == summary["junit"]["names"] and judged["tests"] == summary["junit"]["tests"], "summary/raw counts")
        inputs = obj("inputs_before.json")
        require(inputs == obj("inputs_after.json") and len(inputs["sources"]) == 143, "143 stable source files")
        source_map = {entry["path"]: entry["sha256"] for entry in inputs["sources"]}
        require(len(source_map) == 143 and all(source_map.get("morsehgp3D_v7/" + path) == pin for path, pin in entrypoint.items()), "118 entrypoint pins contained in 143 qualification source pins")
        bins = obj("binaries_tested.json")
        require(bins == obj("binaries_after.json") and len(bins) == (39 if mode == "full" else 11), "binary stability/floor")
        build = inputs["candidate_binding"]["record"]
        require(build["sources_before"] == build["sources_after"] and len(build["sources_before"]) == 51, "CLI 51 stable sources")
        require(build["binary"]["sha256"] == F_CLI and summary["expected_F_sha256"] == F_CLI, "F CLI identity")
        binding = obj("compile_binding.json")
        require(binding == obj("compile_binding_after.json"), "stable compile binding")
        database = obj("local_compile_commands.json")
        require(all(command in database for command in binding["commands"]), "actual compile database entries")
        witness_commands = [command for command in database if command["file"].endswith("/tests/witness_stack_gate.cpp")]
        require(len(witness_commands) == 2, "both actual F witness compile commands")
        require(all("-DMHGP7_TESTING=1" in command["command"] for command in witness_commands), "real product mutant compiled")
        require(sum("-DMHGP7_WITNESS_STACK_NO_ALLOC_OBSERVER=1" in command["command"] for command in witness_commands) == 1, "native semantic companion")
        for file, pin in binding["configuration"].items():
            require(sha(raw("local_" + Path(file).name)) == pin, "actual configuration capture")
        env = obj("environment.json")
        require(env["public_status"] == "not_claimed" and env["gate_results_reused"] is False, "public/attribution scope")
        flag = "-fsanitize=address,undefined" if mode == "sanitized" else "-O3 -DNDEBUG"
        require(all(flag in command["command"] for command in witness_commands), "actual F compilation mode")
        if mode == "sanitized":
            options = env["environment"]["sanitizer_options"]
            require(options["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1" and options["UBSAN_OPTIONS"] == "halt_on_error=1:print_stacktrace=1", "instrumented runtime options")
        commands = {}
        for action in ("configure", "build_incremental" if mode == "full" else "build_targets", "inventory", "ctest"):
            result = obj(action + ".result.json")
            require(type(result["exit_code"]) is int and result["exit_code"] == 0 and result["status"] == "completed", "command terminal return")
            require(summary["results"][action] == result and not raw(action + ".stderr"), "command record and stderr")
            commands[action] = result
        build_stdout = raw("build_incremental.stdout" if mode == "full" else "build_targets.stdout").decode()
        for target in ("mhgp7_witness_stack_gate", "mhgp7_witness_stack_semantic_gate"):
            require("Building CXX object CMakeFiles/" + target + ".dir/tests/witness_stack_gate.cpp.o" in build_stdout and "Linking CXX executable " + target in build_stdout, "raw fresh F witness compile/link " + target)
        fence = summary["last_test_log"]
        require(fence["source"]["sha256"] == fence["archive"]["sha256"] == sha(raw("LastTest.stdout")), "LastTest source/archive binding")
        require(fence["source"]["mtime_ns"] >= fence["boundary"]["fence"]["mtime_ns"], "LastTest post-CTest fence")
        local_count = 0
        if live:
            pins = dict(inputs["files"])
            pins.update(inputs["protected_historical"])
            pins.update(binding["configuration"])
            pins.update({entry["path"]: entry["sha256"] for entry in inputs["sources"] + bins})
            pins[fence["source"]["path"]] = fence["source"]["sha256"]
            for kind in ("binary", "compile_database", "cmake_cache"):
                pins[build[kind]["path"]] = build[kind]["sha256"]
            pins[inputs["candidate_binding"]["receipt"]] = inputs["candidate_binding"]["receipt_sha256"]
            for path, pin in pins.items():
                file = Path(path)
                file = file if file.is_absolute() else ROOT / file
                require(sha(file.read_bytes()) == pin, "live source/binary/config pin " + path)
            local_count = len(pins)
        judged.update({"mode": mode, "sources_stable": 143, "entrypoint_pins_contained_in_sources": 118, "CLI_build_sources_stable": 51, "binaries_stable": bins, "actual_witness_compile_commands": witness_commands, "commands": commands, "environment": env, "local_pins_verified": local_count, "LastTest_bytes": len(raw("LastTest.stdout")), "LastTest_sha256": sha(raw("LastTest.stdout"))})
        results[mode] = judged
    require(set(results) == {"release", "sanitized", "full"}, "all three campaigns")
    require(results["release"]["names"] == results["sanitized"]["names"], "paired targeted inventory")
    return {"schema": "mhgp7-F-independent-qualification-v1", "status": "verified_F339_F48_F48_closed_receipts", "phase": "exploration_v7_hors_registre", "backend": "cpu_reference", "profile": "quantized_u16_input_only", "mode": "audit_independant_math_and_architecture", "public_status": "not_claimed", "gcp": "not_used", "engine_build_CTest_runs_by_auditor": 0, "capture_sha256": sha((HERE / "capture_manifest.json").read_bytes()), "campaigns": results, "limits": ["These are independent readings of F constructor runs, not auditor engine reruns or transferred E test results.", "Compilation commands and source boundaries provide recorded build binding, not hermetic attestation.", "Allocation-disabled semantic target proves semantics under instrumentation, not zero allocations or allocation-failure injection.", "No performance, CI, GPU or global HGP exactness qualification."]}


def self_test() -> dict:
    folder = HERE / "snapshots/release"
    xml = (folder / "ctest.junit.xml").read_bytes()
    inv = decode((folder / "inventory.stdout").read_bytes())
    names = decode((folder / "expected_names.json").read_bytes())
    log = (folder / "LastTest.stdout").read_text()
    judge(xml, inv, names, log)
    rejects = {
        "zero_tests": (xml.replace(b'tests="48"', b'tests="0"'), log),
        "not_run": (xml.replace(b'status="run"', b'status="notrun"', 1), log),
        "failure_node": (xml.replace(b"</testcase>", b"<failure/></testcase>", 1), log),
        "empty_log": (xml, ""),
        "failed_block": (xml, log.replace("Test Passed.", "Test Failed.", 1)),
        "missing_footer": (xml, re.sub(r"^End testing: .*\n", "", log, flags=re.M)),
        "wrong_mutant_exit": (xml, log.replace('"-DEXPECTED=4"', '"-DEXPECTED=0"')),
        "raw_XML_disagree": (xml, log.replace("queries=48960", "queries=48961", 1)),
        "forged_truncation_size": (xml.replace(b"threshold of 1024 bytes", b"threshold of 1023 bytes"), log),
    }
    for name, (bad_xml, bad_log) in rejects.items():
        try:
            judge(bad_xml, inv, names, bad_log)
        except ValueError:
            continue
        raise ValueError("mutant survived " + name)
    try:
        judge(xml, inv, names[:-1] + [names[0]], log)
    except ValueError:
        pass
    else:
        raise ValueError("duplicate expected name survived")
    return {"status": "passed", "positive": 1, "rejections": sorted(list(rejects) + ["duplicate_expected_name"])}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--live", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        print(json.dumps(self_test() if args.self_test else inspect(args.live), indent=2, sort_keys=True))
    except (OSError, ValueError, KeyError, TypeError, ET.ParseError) as error:
        print(json.dumps({"status": "rejected", "error": str(error)}))
        sys.exit(1)
