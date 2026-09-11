#!/usr/bin/env python3
"""Verify the supplementary evidence without compilation or asserts."""
import argparse
import copy
import difflib
import hashlib
import json
from pathlib import Path

NEW_GATE = "13f06875da99ae8413b0e64115964979bc5afe9f0c6f7224a7cd4efe773883dc"
OLD_GATE = "bbf845bef42354bf6ae86ff55d178f5b7c49e8f851d73dcbfd6c4686e855adda"
PATCHED = ("t2_gate.cpp", "source/morsehgp3D_v7/src/forest/full_coverage_certificate.hpp",
           "source/morsehgp3D_v7/src/forest/full_ball_tower.hpp")
CASES = ("line12", "shell14", "spatial12")
CAPTURES = ("causal_r1", "o2_r1", "san_root_r1")


def sha(data):
    return hashlib.sha256(data).hexdigest()


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def validate(manifest, objects):
    need(manifest["schema"] == "mhgp7_t2_metadata_supplement_v1", "schema")
    need(manifest["scope"] == dict(public_status="not_claimed", GCP_used=False, device_executed=False,
         performance_contract=False, legacy_fault_runs_are_positive_miss_witnesses=True), "scope")
    files = manifest["files"]
    for name, row in files.items():
        data = objects(row["sha256"])
        need(sha(data) == row["sha256"] and len(data) == row["bytes"], "object.hash")
        need(not data.startswith(b"\x7fELF") and not name.startswith("/") and ".." not in Path(name).parts, "object.safe")

    def read(name):
        need(name in files, "required:" + name)
        return objects(files[name]["sha256"])

    def load(name):
        return json.loads(read(name))

    closed = load("closed/manifest.json")
    baseline = load("current/baseline.json")
    need(sha(read("closed/manifest.json")) == baseline["source_manifest_sha256"] == manifest["closed_receipt_sha256"],
         "closed.manifest_pin")
    for name in (*PATCHED, "t2_oracle.hpp"):
        need(files["closed/source/" + name]["sha256"] == closed["files"]["source/" + name], "closed.source_pin")
    need(files["current/source/t2_legacy_gate.cpp"]["sha256"] == OLD_GATE == files["closed/source/t2_gate.cpp"]["sha256"],
         "legacy.byte_identical")
    need(files["current/source/t2_gate.cpp"]["sha256"] == NEW_GATE, "checked.source_pin")
    for name, pin in baseline["files"].items():
        if name not in PATCHED:
            need(files["current/source/" + name]["sha256"] == pin, "unchanged.source:" + name)
    expected_patch = ""
    for name in PATCHED:
        expected_patch += "".join(difflib.unified_diff(read("closed/source/" + name).decode().splitlines(keepends=True),
            read("current/source/" + name).decode().splitlines(keepends=True), fromfile="closed/" + name, tofile="private/" + name))
    old_results = {}
    for case in CASES:
        path = "runs/o2/" + case + ".stdout"
        need(files["closed/" + path]["sha256"] == closed["files"][path], "closed.output_pin")
        old_results[case] = load("closed/" + path)

    receipts = {}
    for capture in CAPTURES:
        prefix = "captures/" + capture + "/"
        receipt = load(prefix + "receipt.json")
        receipts[capture] = receipt
        need(receipt["status"] == "passed" and receipt["sources_stable"] and
             receipt["sources_before"] == receipt["sources_after"] and receipt["legacy_gate_exact"], capture + ".closed_stable")
        need(receipt["GCP_used"] is False and receipt["device_executed"] is False and
             receipt["public_status"] == "not_claimed", capture + ".scope")
        need(receipt["source_receipt_manifest_sha256"] == manifest["closed_receipt_sha256"], capture + ".origin")
        need(load(prefix + "baseline_results.json") == old_results, capture + ".baseline_results")
        need(read(prefix + "private_patch.diff").decode() == expected_patch, capture + ".exact_patch")
        for name, pin in receipt["sources_before"].items():
            need(files[prefix + "source_snapshot/" + name]["sha256"] == pin == files["current/" + name]["sha256"],
                 capture + ".same_sources")
        for row in receipt["commands"]:
            for stream in ("stdout", "stderr"):
                need(files[prefix + row["name"] + "." + stream]["sha256"] == row[stream + "_sha256"], capture + ".log_pin")
            intent = load(prefix + row["name"] + ".intent.json")
            need(intent["argv"] == row["argv"] and intent["expected_exit"] == row["expected_exit"], capture + ".intent")
            if row["name"].startswith("compile_"):
                need(all(flag in row["argv"] for flag in ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror")),
                     capture + ".strict")
        for binary in receipt["binaries"].values():
            need(binary["before_execution_sha256"] == binary["after_execution_sha256"] and
                 len(binary["before_execution_sha256"]) == 64, capture + ".binary_stable")

    causal = receipts["causal_r1"]["commands"]
    names = ("legacy_order", "legacy_vertical", "checked_order", "checked_vertical")
    expected = []
    for name in names:
        expected += [("compile_" + name, 0), ("run_" + name, 0 if name.startswith("legacy") else 1)]
    need([(r["name"], r["exit_code"]) for r in causal] == expected, "causal.command_order_and_exits")
    for index, name in enumerate(names):
        legacy = name.startswith("legacy")
        compiler, run = causal[2*index:2*index+2]
        mutant = 1 if name.endswith("order") else 2
        need(f"-DMHGP7_T2_METADATA_MUTANT={mutant}" in compiler["argv"] and
             compiler["argv"][-3].endswith("/source/" + ("t2_legacy_gate.cpp" if legacy else "t2_gate.cpp")),
             "causal.correct_source_and_mutant")
        marker = "T2_METADATA_MUTANT " + ("published_order=8 expected_order=9" if mutant == 1 else "surplus_vertical_entry=1 order=9")
        stderr = read("captures/causal_r1/run_" + name + ".stderr").decode()
        need(run["marker_count"] == stderr.count(marker) == (18 if legacy else 1), "causal.actual_output_mutation")
        if legacy:
            need(load("captures/causal_r1/run_" + name + ".stdout") == old_results["line12"], "legacy.missed_fault_exact_output")
        else:
            reason = "T2.metadata." + ("order_identity" if mutant == 1 else "vertical_node_indexed")
            need(run["diagnostic"] == reason and run["diagnostic_matched"] and
                 "FAIL [line12/variant0] " + reason in stderr, "checked.causal_rejection")
    nominal = {}
    for capture in ("o2_r1", "san_root_r1"):
        commands = receipts[capture]["commands"]
        need([(r["name"], r["exit_code"]) for r in commands] ==
             [("compile_gate", 0), ("line12", 0), ("shell14", 0), ("spatial12", 0), ("invalid_args", 2)], capture + ".commands")
        need("-DMHGP7_T2_METADATA_MUTANT=0" in commands[0]["argv"], capture + ".nominal")
        nominal[capture] = {}
        for case in CASES:
            observed = load("captures/" + capture + "/" + case + ".stdout")
            expected_result = dict(old_results[case]); expected_result["checks"] += 40
            need(observed == expected_result, capture + ".identical_semantics_plus_40_guards")
            nominal[capture][case] = observed
            need(b"T2_METADATA_MUTANT" not in read("captures/" + capture + "/" + case + ".stderr"), capture + ".no_fault")
    need(nominal["o2_r1"] == nominal["san_root_r1"], "nominal.o2_san_identity")
    san = receipts["san_root_r1"]
    need("-fsanitize=address,undefined" in san["commands"][0]["argv"] and
         san["sanitizer_environment"]["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1", "san.no_disabled_leaks")
    totals = {field: sum(row[field] for row in nominal["o2_r1"].values()) for field in
              ("checks", "orders", "census_runs", "physical_tower_pairs", "cuts", "vertical_checks")}
    need(totals == dict(checks=12315725, orders=540, census_runs=18, physical_tower_pairs=48, cuts=13000,
                        vertical_checks=8103948), "nominal.exact_nonvacuity")
    return dict(status="passed", logical_files=len(files), legacy_faults_missed=2, corrected_faults_rejected=2,
                nominal_towers_per_build=54, new_metadata_checks_per_build=120, totals=totals)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("packet", nargs="?", default=str(Path(__file__).resolve().parent))
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    packet = Path(args.packet)
    raw = (packet / "manifest.json").read_bytes(); manifest = json.loads(raw)
    objects = lambda pin: (packet / "objects" / pin).read_bytes()
    result = validate(manifest, objects)
    if args.selftest:
        rejected = 0
        for fault in range(5):
            altered = copy.deepcopy(manifest); overlay = {}
            if fault == 0:
                altered["scope"]["device_executed"] = True
            else:
                path = "captures/" + ("causal_r1" if fault < 4 else "san_root_r1") + "/receipt.json"
                receipt = json.loads(objects(altered["files"][path]["sha256"]))
                if fault == 1:
                    receipt["commands"][1]["exit_code"] = 1
                elif fault == 2:
                    receipt["commands"][1]["marker_count"] = 0
                elif fault == 3:
                    receipt["commands"][5]["diagnostic_matched"] = False
                else:
                    receipt["sanitizer_environment"]["ASAN_OPTIONS"] = "detect_leaks=0"
                value = (json.dumps(receipt, sort_keys=True) + "\n").encode(); pin = sha(value)
                overlay[pin] = value; altered["files"][path] = dict(sha256=pin, bytes=len(value))
            try:
                validate(altered, lambda pin: overlay[pin] if pin in overlay else objects(pin))
            except (KeyError, ValueError):
                rejected += 1
        need(rejected == 5, "reader.faults")
        result["reader_only_faults_rejected"] = rejected
    result["manifest_sha256"] = sha(raw)
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
