#!/usr/bin/env python3
"""Portable evidence reader; no compiler, geometry, kernel or infrastructure."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CAPTURE_PIN = "254c12d143aaef692fdfb88fc304358d605d8329926946065575e359caa7b218"
BASE_COMMIT = "c03f6be8488453486b112811071827a96303ec86"
CAPTURES = {"export_r1": ("export", "export_gate"), "stub_r1": ("stub", "stub_gate"),
    "san_root_r1": ("san", "stub_gate"), "nvcc_r1": ("nvcc", None), "nvcc_r2": ("nvcc", "device_gate")}


def need(good, reason):
    if not good:
        raise RuntimeError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def safe(name):
    path = Path(name)
    need(not path.is_absolute() and ".." not in path.parts and str(path) not in ("", "."), "unsafe path")
    return path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--extract", type=Path)
    args = parser.parse_args()
    manifest = json.loads((ROOT / "manifest.json").read_text())
    actual = {path.relative_to(ROOT).as_posix() for path in ROOT.rglob("*") if path.is_file()}
    need(actual == set(manifest["files"]) | {"manifest.json"}, "physical coverage")
    for path in ROOT.rglob("*"):
        need(not path.is_symlink(), "symlink")
    for name, entry in manifest["files"].items():
        data = (ROOT / safe(name)).read_bytes()
        need(sha(data) == entry["sha256"] and len(data) == entry["size"], "physical pin")
        need(data[:4] != b"\x7fELF", "ELF forbidden")
        data.decode("utf-8")
    raw = (ROOT / "capture_manifest.json").read_bytes()
    need(sha(raw) == CAPTURE_PIN, "capture authority")
    captured = json.loads(raw)
    need(captured["base_commit"] == BASE_COMMIT and captured["device_executed"] is False and
        captured["gcp_used"] is False and captured["public_status"] == "not_claimed", "scope")
    mapping = json.loads((ROOT / "storage_map.json").read_text())
    need(set(mapping) == set(captured["files"]), "logical coverage")
    for name, entry in mapping.items():
        safe(name)
        data = (ROOT / safe(entry["storage"])).read_bytes()
        expected = captured["files"][name]
        need(entry["size"] == expected["size"] and entry["sha256"] == expected["sha256"] and
            len(data) == entry["size"] and sha(data) == entry["sha256"], "logical pin")

    def read(name):
        need(name in mapping, "logical file missing: " + name)
        return (ROOT / safe(mapping[name]["storage"])).read_bytes()

    def value(name):
        return json.loads(read(name))

    receipts, commands = {}, 0
    for name, (mode, binary) in CAPTURES.items():
        prefix = "captures/" + name + "/"
        receipt = value(prefix + "receipt.json")
        expected_status = "failed" if name == "nvcc_r1" else "passed"
        need(receipt["status"] == expected_status and receipt["mode"] == mode and
            receipt["sources_stable"] is True and receipt["snapshot_stable"] is True and
            receipt["sources_before"] == receipt["sources_after"] and receipt["device_executed"] is False and
            receipt["base_commit"] == BASE_COMMIT, "closed capture")
        for source, pin in receipt["sources_before"].items():
            need(sha(read(prefix + "source_snapshot/" + source)) == pin, "compiled snapshot")
        if binary:
            need(captured["binary_pins_no_ELF"][prefix + binary]["sha256"] == receipt["binary_sha256"], "binary pin")
        else:
            need("binary_sha256" not in receipt and receipt["error"] == "compile_link unexpected exit" and
                b"Permission denied" in read(prefix + "compile_link.stderr"), "preserved NVCC failure")
        names = [command["name"] for command in receipt["commands"]]
        expected_names = ["compiler", "compile_link"] if mode == "nvcc" else \
            ["compiler", "compile", "selftest", "unknown", "missing_arg"]
        if mode == "export":
            expected_names = ["provenance_normal", "provenance_optimized", "compiler", "compile", "plan", "export", "unknown", "missing_arg"]
        need(names == expected_names, "complete command sequence")
        for command in receipt["commands"]:
            expected = 2 if command["name"] in ("unknown", "missing_arg") else 0
            need(command["expected_exit_code"] == expected, "declared command exit")
            if name == "nvcc_r1" and command["name"] == "compile_link":
                expected = 1
            need(command["exit_code"] == expected, "actual command exit")
            for stream in ("stdout", "stderr"):
                need(sha(read(prefix + command["name"] + "." + stream)) == command[stream + "_sha256"], "log pin")
            commands += 1
        receipts[name] = receipt
    exported = receipts["export_r1"]
    fixture_pin = captured["fixture_sha256"]
    need(sha(read("current/cuda_trial/terminal_fixtures.inc")) == fixture_pin == exported["fixture_sha256"], "fixture pin")
    for name, receipt in receipts.items():
        need(receipt["sources_before"] == exported["sources_before"] and
            receipt["fixture_binding"] == exported["fixture_binding"] and
            receipt["shared_source_closure"] == exported["shared_source_closure"], "same consumed source authority")
        if name != "export_r1":
            need(receipt["fixture_provenance"]["fixture_sha256"] == fixture_pin and
                receipt["fixture_provenance"]["receipt_sha256"] == sha(read("captures/export_r1/receipt.json")) and
                sha(read("captures/" + name + "/source_snapshot/cuda_trial/terminal_fixtures.inc")) == fixture_pin,
                "consumed exact export")
    for source, pin in exported["sources_before"].items():
        need(sha(read("current/" + source)) == pin, "current source equals captured source")
    host_raw = read("qualification/host_manifest.json")
    need(sha(host_raw) == captured["host_qualification_manifest_sha256"], "host authority")
    host = json.loads(host_raw)
    host_capture_raw = read("qualification/host_capture_manifest.json")
    need(sha(host_capture_raw) == host["files"]["capture_manifest.json"]["sha256"], "host capture authority")
    host_capture = json.loads(host_capture_raw)
    need(len(exported["ownerfix_prerequisites"]) == 6, "six host prerequisites")
    for name, pin in exported["ownerfix_prerequisites"].items():
        payload = read("qualification/" + name)
        need(sha(payload) == pin == host_capture["files"]["ownerfix/" + name]["sha256"], "host receipt bound")
        prior = json.loads(payload)
        need(prior["status"] == "passed" and prior["sources_stable"] is True and
            prior["sources_before"] == prior["sources_after"], "prior closed")
        for source, source_pin in exported["shared_source_closure"].items():
            logical = (Path("ownerfix") / Path(name).parent / "source_snapshot" / source).as_posix()
            need(prior["sources_before"].get(source) == source_pin == host_capture["files"][logical]["sha256"] and
                sha(read("current/" + source)) == source_pin, "full source closure of prerequisites")
    old_raw = read("qualification/k8_manifest.json")
    need(sha(old_raw) == captured["imported_k8_manifest_sha256"], "K8 import authority")
    old = json.loads(old_raw)
    need(sha(read("qualification/k8_capture_manifest.json")) == old["files"]["capture_manifest.json"]["sha256"], "K8 captures authority")
    imported = value("current/import.json")
    need(imported["packet_manifest_sha256"] == sha(old_raw) and imported["inherited_results"] is False and
        imported["product_reference_commit"] == BASE_COMMIT, "explicit import without inherited results")
    for source, field in (("terminal.cuh", "terminal_sha256"), ("terminal_owner.hpp", "owner_sha256")):
        need(sha(read("current/" + source)) == imported[field] == old["files"]["sources/current/" + source]["sha256"], "unchanged helper/owner")
    for name in ("export.cpp", "device_gate.cu", "record.py", "fixture_types.hpp", "reference.hpp", "provenance_gate.py"):
        need(sha(read("originals/k8/" + name)) == old["files"]["sources/current/cuda_trial/" + name]["sha256"], "original judge pin")
    need(sha(read("current/cuda_trial/t2_oracle.hpp")) == imported["t2_oracle_sha256"], "T2 authority")
    plan_raw = read("captures/export_r1/plan.stdout")
    need(sha(plan_raw) == captured["plan_sha256"] == exported["plan_sha256"], "predeclared plan pin")
    plan = json.loads(plan_raw)
    need(plan["status"] == "declared" and plan["geometry_executed"] is False and len(plan["clouds"]) == 17 and
        plan["K9_requests"] == 468 and plan["K10_requests"] == 160 and
        sum(len(cloud["masks"]) for cloud in plan["clouds"]) == 1577, "predeclared corpus")
    for cloud in plan["clouds"]:
        need(cloud["masks"] == sorted(set(cloud["masks"])), "deterministic distinct masks")
        need(all(type(mask) is int and 0 < mask < (1 << cloud["n"]) for mask in cloud["masks"]), "plan mask domain")
    need(sum(mask.bit_count() == 9 for cloud in plan["clouds"] for mask in cloud["masks"]) == 468 and
        sum(mask.bit_count() == 10 for cloud in plan["clouds"] for mask in cloud["masks"]) == 160,
        "actual high-order mask counts")
    timeline = {command["name"]: command for command in exported["commands"]}
    need(timeline["plan"]["ended_ns"] <= timeline["export"]["started_ns"], "plan precedes geometry")
    summary = value("captures/export_r1/export.stderr")
    need(summary == captured["export_summary"] == exported["export_summary"], "export summary authority")
    expected = {"clouds": 17, "requests": 1577, "trace_rows": 2763, "Gram_checked_rows": 2763,
        "q2": 1802, "q3": 691, "q4": 270, "extra_shells": 251, "strict_steps": 1185,
        "same_radius_steps": 1, "intruder_queries": 1186, "large_ordinals": 1577, "model_cross_checks": 1022,
        "K9_requests": 468, "K10_requests": 160, "K9_strict_steps": 577, "K10_strict_steps": 130,
        "K9_intruder_queries": 577, "K10_intruder_queries": 130, "K9_q3": 229, "K10_q3": 100, "K9_q4": 226, "K10_q4": 19}
    need(all(type(summary.get(key)) is int and summary[key] == item for key, item in expected.items()) and
        summary["all_declared_facets_nominal"] is True and summary["silently_filtered_failures"] == 0, "exact nonvacuity")
    for name in ("provenance_normal", "provenance_optimized"):
        data = value("captures/export_r1/" + name + ".stdout")
        need(data["status"] == "passed" and data["checks"] == 44 and data["rejections"] == 40 and
            data["geometry_executed"] is False, "provenance gate")
    observations = []
    for name in ("stub_r1", "san_root_r1"):
        data = value("captures/" + name + "/selftest.stdout")
        need(data["status"] == "passed" and data["backend"] == "HOST_STUB" and data["checks"] == 15539 and
            data["compared"] == 1577 and data["trace_rows"] == 2763 and data["zero_trace_complete_results"] == 1577 and
            data["large_ordinals"] == 1577 and data["rejections"] == 12 and data["high_K_last_slot_rejections"] == 6 and
            data["causal_transport_mutations"] == 8 and data["cleanup_certified"] is True and data["failures"] == 0 and
            data["allocation_bytes"] == data["certified_free_bytes"] and data["launches"] == 61 and
            data["infrastructure_authority"] == "external_controller" and "gcp_used" not in data, "stub/SAN transport checks")
        for key in ("q2", "q3", "q4", "extra_shells", "strict_steps", "same_radius_steps", "intruder_queries",
            "K9_requests", "K10_requests", "K9_strict_steps", "K10_strict_steps", "K9_intruder_queries", "K10_intruder_queries",
            "K9_q3", "K10_q3", "K9_q4", "K10_q4"):
            need(data[key] == summary[key], "matched geometric accounting")
        observations.append(data)
    need(observations[0] == observations[1], "deterministic O2/SAN observations")
    repair = captured["executable_mode_repair"]
    need(repair["before_octal"] == "0666" and repair["after_octal"] == "0700" and
        repair["content_sha256"] == sha(read(repair["path"])), "explicit mode-only repair")
    for name, expected_mode in (("nvcc_r1", "0666"), ("nvcc_r2", "0700")):
        observed = repair["snapshot_stat_observations"][name]
        need(observed["logical_path"] == "captures/" + name + "/source_snapshot/nvcc_strict_host.py" and
            observed["observed_mode_octal"] == expected_mode and
            observed["content_sha256"] == repair["content_sha256"] == sha(read(observed["logical_path"])),
            "mode observation on pinned actual snapshot")
    san = receipts["san_root_r1"]
    need(san["sanitizer_environment"] == {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
        "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}, "sanitizers and leak detection enabled")
    san_argv = next(command["argv"] for command in san["commands"] if command["name"] == "compile")
    need(all(flag in san_argv for flag in ("-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined",
        "-fno-sanitize-recover=all", "-Wall", "-Wextra", "-Wpedantic", "-Werror")), "strict SAN build flags")
    if args.extract:
        args.extract.mkdir(exist_ok=False)
        for name in mapping:
            path = args.extract / safe(name)
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open("xb") as output:
                output.write(read(name))
        # Restore only this pinned executable, not arbitrary stored modes.
        (args.extract / "current/nvcc_strict_host.py").chmod(0o700)
    print(json.dumps({"status": "passed", "scope": captured["scope"], "physical_files": len(actual),
        "logical_files": len(mapping), "captures": 5, "historical_failures": 1, "host_qualifications": 6,
        "commands": commands, "compared": 1577, "trace_rows": 2763, "K9_requests": 468, "K10_requests": 160,
        "ELF_pins_only": len(captured["binary_pins_no_ELF"]), "geometry_executed_by_reader": False,
        "device_executed": False}, sort_keys=True))


if __name__ == "__main__":
    main()
