#!/usr/bin/env python3
"""Portable normal/-O reader; no compiler, kernel, geometry or infrastructure."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CAPTURE_PIN = "5c7e98fd782a9628ce039c9358d9cfeee07034dd2f1f47c774bba096e4697596"


def need(good, reason):
    if not good:
        raise RuntimeError(reason)


def sha(payload):
    return hashlib.sha256(payload).hexdigest()


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
    need(actual == set(manifest["files"]) | {"manifest.json"}, "physical manifest coverage")
    for path in ROOT.rglob("*"):
        need(not path.is_symlink(), "symlink")
    for name, entry in manifest["files"].items():
        payload = (ROOT / safe(name)).read_bytes()
        need(sha(payload) == entry["sha256"] and len(payload) == entry["size"], "physical pin")
        need(payload[:4] != b"\x7fELF", "ELF forbidden")
        payload.decode("utf-8")
    raw = (ROOT / "capture_manifest.json").read_bytes()
    need(sha(raw) == CAPTURE_PIN, "capture authority pin")
    captured = json.loads(raw)
    mapping = json.loads((ROOT / "storage_map.json").read_text())
    need(set(mapping) == set(captured["files"]), "logical mapping coverage")
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

    receipts = {}
    commands = 0
    for name, mode, binary in (("export_r1", "export", "export_gate"), ("stub_r1", "stub", "stub_gate"),
        ("san_root_r1", "san", "stub_gate"), ("nvcc_r1", "nvcc", "device_gate")):
        prefix = "captures/" + name + "/"
        receipt = value(prefix + "receipt.json")
        need(receipt["status"] == "passed" and receipt["mode"] == mode and receipt["sources_stable"] is True and
            receipt["snapshot_stable"] is True and receipt["sources_before"] == receipt["sources_after"] and
            receipt["device_executed"] is False, "closed local capture")
        for source, pin in receipt["sources_before"].items():
            need(sha(read(prefix + "source_snapshot/" + source)) == pin, "actual compiled source")
        need(captured["binary_pins_no_ELF"][prefix + binary]["sha256"] == receipt["binary_sha256"], "ELF pin")
        for command in receipt["commands"]:
            need(command["exit_code"] == command["expected_exit_code"], "command exit")
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
            receipt["shared_source_closure"] == exported["shared_source_closure"], "same authority across captures")
        if name != "export_r1":
            need(receipt["fixture_provenance"]["fixture_sha256"] == fixture_pin and
                receipt["fixture_provenance"]["receipt_sha256"] == sha(read("captures/export_r1/receipt.json")) and
                sha(read("captures/" + name + "/source_snapshot/cuda_trial/terminal_fixtures.inc")) == fixture_pin,
                "actual consumed fixture export binding")
    host_raw = read("qualification/host_manifest.json")
    need(sha(host_raw) == captured["host_qualification_manifest_sha256"], "host packet authority")
    host = json.loads(host_raw)
    host_capture_raw = read("qualification/host_capture_manifest.json")
    need(sha(host_capture_raw) == host["files"]["capture_manifest.json"]["sha256"], "host source authority")
    host_capture = json.loads(host_capture_raw)
    need(len(exported["ownerfix_prerequisites"]) == 6, "six prior O2/SAN qualifications")
    for name, pin in exported["ownerfix_prerequisites"].items():
        payload = read("qualification/" + name)
        need(sha(payload) == pin == host_capture["files"]["ownerfix/" + name]["sha256"], "host receipt bound")
        prior = json.loads(payload)
        need(prior["status"] == "passed" and prior["sources_stable"] is True and
            prior["sources_before"] == prior["sources_after"], "prior qualification closed")
        for source, source_pin in exported["shared_source_closure"].items():
            logical = (Path("ownerfix") / Path(name).parent / "source_snapshot" / source).as_posix()
            need(prior["sources_before"].get(source) == source_pin == host_capture["files"][logical]["sha256"] and
                sha(read("current/" + source)) == source_pin, "full shared source closure against qualified snapshot")
    summary = value("captures/export_r1/export.stderr")
    need(summary == captured["export_summary"] == exported["export_summary"] and summary["clouds"] == 14 and
        summary["requests"] == 949 and summary["trace_rows"] == 1428 and summary["Gram_checked_rows"] == 1428 and
        summary["q2"] == 1041 and summary["q3"] == 362 and summary["q4"] == 25 and summary["extra_shells"] == 206 and
        summary["strict_steps"] == 478 and summary["same_radius_steps"] == 1 and summary["intruder_queries"] == 479 and
        summary["large_ordinals"] == 949 and summary["silently_filtered_failures"] == 0 and
        summary["all_declared_facets_nominal"] is True, "export nonvacuity")
    for name in ("provenance_normal", "provenance_optimized"):
        data = value("captures/export_r1/" + name + ".stdout")
        need(data["status"] == "passed" and data["checks"] == 44 and data["rejections"] == 40 and
            data["geometry_executed"] is False, "provenance causal gates")
    observations = []
    for name in ("stub_r1", "san_root_r1"):
        data = value("captures/" + name + "/selftest.stdout")
        need(data["status"] == "passed" and data["backend"] == "HOST_STUB" and data["checks"] == 9162 and
            data["compared"] == 949 and data["trace_rows"] == 1428 and data["q2"] == 1041 and data["q3"] == 362 and
            data["q4"] == 25 and data["extra_shells"] == 206 and data["strict_steps"] == 478 and
            data["same_radius_steps"] == 1 and data["zero_trace_complete_results"] == 949 and
            data["large_ordinals"] == 949 and data["rejections"] == 12 and data["causal_transport_mutations"] == 8 and
            data["cleanup_certified"] is True and data["allocation_bytes"] == data["certified_free_bytes"] and
            data["infrastructure_authority"] == "external_controller" and "gcp_used" not in data, "stub/SAN qualification")
        observations.append(data)
    need(observations[0] == observations[1], "same deterministic stub/SAN result")
    need([command["name"] for command in receipts["nvcc_r1"]["commands"]] == ["compiler", "compile_link"],
        "NVCC compile/link only, no device execution")
    if args.extract:
        args.extract.mkdir(exist_ok=False)
        for name in mapping:
            path = args.extract / safe(name)
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open("xb") as output:
                output.write(read(name))
    print(json.dumps({"status": "passed", "scope": captured["scope"], "physical_files": len(actual),
        "logical_files": len(mapping), "captures": 4, "host_qualifications": 6, "commands": commands,
        "compared": 949, "trace_rows": 1428, "ELF_pins_only": len(captured["binary_pins_no_ELF"]),
        "geometry_executed_by_reader": False, "device_executed": False}, sort_keys=True))


if __name__ == "__main__":
    main()
