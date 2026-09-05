#!/usr/bin/env python3
"""Independently read closed E CTest evidence; no CTest or build invocation."""
import argparse
import gzip
import json
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

from verify_q2_receipt import HERE, REPO, decode, require, sha, unique


def judge(xml, inventory, expected, log):
    require(len(expected) in (33, 324) and len(set(expected)) == len(expected), "expected nonvacuum")
    inv = [test["name"] for test in inventory["tests"]]
    require(len(set(inv)) == len(inv) and set(inv) == set(expected), "inventory names")
    root = ET.fromstring(xml)
    tests = list(root.iter("testcase"))
    names = [test.attrib["name"] for test in tests]
    require(len(names) == len(set(names)) == len(expected) and set(names) == set(expected), "JUnit names")
    require(int(root.attrib["tests"]) == len(expected), "JUnit total")
    for key in ("failures", "errors", "skipped", "disabled"):
        require(int(root.attrib.get(key, "0")) == 0, "JUnit " + key)
    for test in tests:
        require(test.attrib.get("status") == "run", "JUnit status")
        require(float(test.attrib["time"]) >= 0, "JUnit time")
        require(not any(node.tag in {"failure", "error", "skipped"} for node in test.iter()), "JUnit failure node")
    require(len(re.findall(r"^Start testing: .+$", log, re.M)) == 1 and len(re.findall(r"^End testing: .+$", log, re.M)) == 1, "LastTest terminal boundaries")
    chunks = re.split(r"^\d+/\d+ Testing: (.+)\n", log, flags=re.M)
    require(len(chunks) == 2 * len(expected) + 1, "LastTest block count")
    blocks = unique(zip(chunks[1::2], chunks[2::2]))
    require(set(blocks) == set(expected), "LastTest names")
    for name, block in blocks.items():
        require(len(re.findall(r"^Test Passed\.$", block, re.M)) == 1, "LastTest pass: " + name)
        require("<end of output>" in block and re.search(r"^Command: .+$", block, re.M), "LastTest raw output and command")
        require(re.search(r'^"' + re.escape(name) + r'" end time: ', block, re.M), "LastTest named end")
        require(not re.search(r"^Test (Failed|Not Run|Timeout)", block, re.M), "LastTest failure")
    q2 = blocks["mhgp7_meb_lazy_q2_reject_shell"]
    require('"-DEXPECTED=4"' in q2 and '"-DARGS=--mutant=silent-meb-q2-reject-shell"' in q2, "q2 mutant raw command")
    require(re.search(r"^meb_lazy_gate=mutant_killed mutant=silent-meb-q2-reject-shell divergence=differential.status_reason public_status=not_claimed$", q2, re.M), "q2 mutant raw output")
    return {"tests": len(names), "names": sorted(names), "failure_error_skip_nodes": 0, "last_test_passed_blocks": len(blocks), "q2_rejection_raw_block": q2}


def inspect(live):
    capture = decode((HERE / "e_tests_capture.json").read_bytes())
    require({"release", "sanitized"} <= set(capture["modes"]) <= {"release", "sanitized", "full"}, "captured modes")
    d_entry = capture["baseline_D_expected_names"]
    d_bytes = (HERE / "d_expected_names.json").read_bytes()
    require(sha(d_bytes) == d_entry["sha256"] and len(d_bytes) == d_entry["bytes"], "D expected inventory pin")
    d_names = decode(d_bytes)
    require(len(d_names) == len(set(d_names)) == 323, "D inventory nonvacuum")
    results = {}
    for mode, files in capture["modes"].items():
        def raw(name):
            entry = files[name]
            relative = Path(entry["snapshot_path"])
            require(not relative.is_absolute() and ".." not in relative.parts, "snapshot path")
            path = HERE / relative
            require(path.resolve().is_relative_to(HERE), "snapshot symlink escape")
            data = path.read_bytes()
            if entry["compression"] == "gzip":
                data = gzip.decompress(data)
            else:
                require(entry["compression"] == "none", "compression kind")
            require(sha(data) == entry["sha256"] and len(data) == entry["bytes"], "capture pin " + name)
            return data

        def obj(name):
            return decode(raw(name))

        manifest = obj("receipt_manifest.json")
        require(len(manifest) == 31 and len(files) == 32, "closed receipt inventory floor")
        require({entry["path"] for entry in manifest} == set(files) - {"receipt_manifest.json"}, "manifest coverage")
        for entry in manifest:
            data = raw(entry["path"])
            require(sha(data) == entry["sha256"] and len(data) == entry["size"], "constructor manifest pin")
        summary = obj("summary.json")
        require(summary["status"] == "passed" and summary["failure"] is None and summary["final_errors"] == [], "closed summary")
        expected = obj("expected_names.json")
        require(len(expected) == (324 if mode == "full" else 33), "mode count")
        if mode == "full":
            require(set(expected) == set(d_names) | {"mhgp7_meb_lazy_q2_reject_shell"}, "E full = D inventory plus q2 rejection")
        output = judge(raw("ctest.junit.xml"), obj("inventory.stdout"), expected, raw("LastTest.stdout").decode())
        require(summary["junit"]["tests"] == output["tests"] and summary["junit"]["names"] == output["names"], "summary/JUnit agreement")
        before = obj("inputs_before.json")
        require(before == obj("inputs_after.json"), "input stability")
        require(len(before["sources"]) == 140, "source floor")
        binaries = obj("binaries_tested.json")
        require(binaries == obj("binaries_after.json") and len(binaries) == (37 if mode == "full" else 9), "binary stability/floor")
        binding = obj("compile_binding.json")
        require(binding == obj("compile_binding_after.json"), "compile binding stability")
        commands = {}
        for action in ("configure", "build_incremental" if mode == "full" else "build_targets", "inventory", "ctest"):
            result = obj(action + ".result.json")
            require(type(result["exit_code"]) is int and result["exit_code"] == 0 and result["status"] == "completed", "command exit " + action)
            require(result == summary["results"][action], "command/summary equality")
            require(raw(action + ".stderr") == b"", "command stderr " + action)
            commands[action] = result
        log_meta = summary["last_test_log"]
        require(log_meta["archive"]["sha256"] == log_meta["source"]["sha256"] == sha(raw("LastTest.stdout")), "LastTest binding")
        require(log_meta["source"]["mtime_ns"] >= log_meta["boundary"]["fence"]["mtime_ns"], "LastTest freshness fence")
        env = obj("environment.json")
        require(env["public_status"] == "not_claimed" and env["gate_results_reused"] is False, "environment scope")
        if mode == "sanitized":
            require(env["environment"]["sanitizer_options"]["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1", "ASAN options")
            require(env["environment"]["sanitizer_options"]["UBSAN_OPTIONS"] == "halt_on_error=1:print_stacktrace=1", "UBSAN options")
            require(all("-fsanitize=address,undefined" in command["command"] for command in binding["commands"]), "sanitizer compiled commands")
        else:
            require(all("-O3 -DNDEBUG" in command["command"] for command in binding["commands"]), "Release compiled commands")
        local_count = 0
        if live:
            pins = dict(binding["configuration"])
            pins.update(before["files"])
            pins.update(before["protected_historical"])
            pins.update({entry["path"]: entry["sha256"] for entry in before["sources"] + binaries})
            pins[log_meta["source"]["path"]] = log_meta["source"]["sha256"]
            for path, pin in pins.items():
                p = Path(path)
                p = p if p.is_absolute() else REPO / p
                require(sha(p.read_bytes()) == pin, "live source/binary/binding " + path)
            local_count = len(pins)
        output.update({"mode": mode, "source_files": 140, "binaries": binaries, "commands": commands, "environment": env, "local_pins_verified": local_count, "LastTest_sha256": sha(raw("LastTest.stdout")), "receipt_manifest_sha256": sha(raw("receipt_manifest.json"))})
        results[mode] = output
    return {"schema": "mhgp7-E-independent-tests-replay-v1", "status": "verified_closed_receipts", "phase": "exploration_v7_hors_registre", "backend": "cpu_reference", "profile": "quantized_u16_input_only", "mode": "audit_independant_math_and_architecture", "public_status": "not_claimed", "gcp": "not_used", "engine_runs_executed_by_this_audit": 0, "capture_sha256": sha((HERE / "e_tests_capture.json").read_bytes()), "campaigns": results, "limits": ["Read-only replay of constructor executions; no independent engine execution.", "Compiled source and binary boundary records are not a hermetic build attestation.", "CTest mutant passes preserve their expected rejections; no global HGP exactness or industrial performance claim."]}


def self_test():
    folder = HERE / "e_tests_snapshot/release"
    xml = (folder / "ctest.junit.xml").read_text()
    inventory = decode((folder / "inventory.stdout").read_bytes())
    expected = decode((folder / "expected_names.json").read_bytes())
    log = (folder / "LastTest.stdout").read_text()
    judge(xml, inventory, expected, log)
    xml_bad = {
        "zero_tests": xml.replace('tests="33"', 'tests="0"'),
        "wrong_status": xml.replace('status="run"', 'status="notrun"', 1),
        "failure_node": xml.replace("</testcase>", "<failure/></testcase>", 1),
        "skipped_node": xml.replace("</testcase>", "<skipped/></testcase>", 1),
    }
    log_bad = {"empty_log": "", "failed_block": log.replace("Test Passed.", "Test Failed.", 1), "missing_footer": re.sub(r"^End testing: .*\n", "", log, flags=re.M), "wrong_q2_exit": log.replace('"-DEXPECTED=4"', '"-DEXPECTED=0"')}
    for name, bad_xml, bad_log in [(name, value, log) for name, value in xml_bad.items()] + [(name, xml, value) for name, value in log_bad.items()]:
        try:
            judge(bad_xml, inventory, expected, bad_log)
        except ValueError:
            continue
        raise ValueError("mutant survived: " + name)
    try:
        judge(xml, inventory, expected[:-1] + [expected[0]], log)
    except ValueError:
        pass
    else:
        raise ValueError("duplicate expected name survived")
    return {"status": "passed", "positive": 1, "rejections": sorted(list(xml_bad) + list(log_bad) + ["duplicate_expected_name"]), "exitcode": 0}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--live", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        output = self_test() if args.self_test else inspect(args.live)
    except (ValueError, KeyError, TypeError, OSError, ET.ParseError) as error:
        print(json.dumps({"status": "rejected", "error": str(error)}))
        return 1
    print(json.dumps(output, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
