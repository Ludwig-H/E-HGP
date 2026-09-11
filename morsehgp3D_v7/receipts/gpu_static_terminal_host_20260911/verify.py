#!/usr/bin/env python3
"""Portable, normal/-O evidence reader; no compilation or geometry execution."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CAPTURE_PIN = "4de22c297cee722093f923cfd05581b3a27a6bc9f027e4e6822b9ef650af3af8"


def need(good, why):
    if not good:
        raise RuntimeError(why)


def digest(payload):
    return hashlib.sha256(payload).hexdigest()


def relative(name):
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
        data = (ROOT / relative(name)).read_bytes()
        need(len(data) == entry["size"] and digest(data) == entry["sha256"], "physical pin: " + name)
        need(data[:4] != b"\x7fELF", "ELF forbidden")
        data.decode("utf-8")
    raw = (ROOT / "capture_manifest.json").read_bytes()
    need(digest(raw) == CAPTURE_PIN, "capture authority pin")
    capture = json.loads(raw)
    mapping = json.loads((ROOT / "storage_map.json").read_text())
    need(set(mapping) == set(capture["files"]), "logical mapping coverage")
    for name, entry in mapping.items():
        relative(name)
        payload = (ROOT / relative(entry["storage"])).read_bytes()
        expected = capture["files"][name]
        need(entry["size"] == expected["size"] and entry["sha256"] == expected["sha256"] and
            len(payload) == entry["size"] and digest(payload) == entry["sha256"], "logical pin: " + name)

    def read(name):
        need(name in mapping, "logical source missing: " + name)
        return (ROOT / relative(mapping[name]["storage"])).read_bytes()

    commands = 0
    causal = 0
    for name, expected in capture["required_receipts"].items():
        payload = read(name)
        need(digest(payload) == expected, "receipt pin")
        receipt = json.loads(payload)
        need(receipt["status"] == "passed" and receipt["sources_stable"] is True and
            receipt["sources_before"] == receipt["sources_after"], "closed receipt")
        directory = Path(name).parent
        binary = (directory / "gate").as_posix()
        need(capture["binary_pins_no_ELF"].get(binary, {}).get("sha256") == receipt["binary_sha256"],
            "nominal ELF pin")
        for mutant in receipt.get("mutants", []):
            mutant_root = directory / "mutants" / mutant["name"]
            need(capture["binary_pins_no_ELF"].get((mutant_root / "gate").as_posix(), {}).get("sha256") ==
                mutant["binary_sha256"], "mutant ELF pin")
            header = "terminal_owner.hpp" if mutant["name"] == "historical_owner" else "terminal.cuh"
            need(digest(read((mutant_root / "source_snapshot" / header).as_posix())) ==
                mutant["source_sha256"], "mutant actual source pin")
        for source, pin in receipt["sources_before"].items():
            need(digest(read((directory / "source_snapshot" / relative(source)).as_posix())) == pin,
                "compiled snapshot source pin")
        for command in receipt["commands"]:
            need(command["exit_code"] == command["expected_exit_code"], "command exit")
            for stream in ("stdout", "stderr"):
                data = read((directory / (command["name"] + "." + stream)).as_posix())
                need(digest(data) == command[stream + "_sha256"], "command output pin")
                if stream == "stderr" and command.get("causal_diagnostic"):
                    need(command.get("causal_diagnostic_matched") is True and
                        command["causal_diagnostic"].encode() in data, "causal diagnostic")
                    causal += 1
            commands += 1
    need(len(capture["required_receipts"]) == 13 and causal > 0, "capture nonvacuity")
    need(digest(read("ownerfix/terminal_owner.hpp")) == capture["ownerfix_pin"] and
        digest(read("r2/terminal_owner.hpp")) == capture["historical_owner_pin"] and
        capture["historical_owner_is_defective"] is True, "owner revision authority")
    history = "ownerfix/guards_trial/o2_r1/mutant_historical_owner.stderr"
    need(b"guard.cross_owner_must_refuse_foreign_index" in read(history), "actual old-owner causal failure")
    for mode in ("o2_r1", "san_root_r1"):
        for lane in ("static1", "static4"):
            full, unit = [json.loads(line) for line in read("ownerfix/" + mode + "/" + lane + ".stdout").splitlines()]
            need(full["checks"] == 256672 and full["clouds"] == 30 and full["orders"] == 124 and
                unit["unit_checks"] == 20851 and unit["unit_compared"] == 523 and unit["unit_rejections"] == 605 and
                unit["q2"] == 393 and unit["q3"] == 110 and unit["q4"] == 20 and unit["extra_shells"] == 197 and
                unit["large_ordinals"] == 2 and unit["paired_terminals"] == 88 and unit["paired_trace_rows"] == 106 and
                unit["strict_steps"] == 14 and unit["same_radius_steps"] == 4 and unit["zero_trace_capacity_checks"] == 88,
                "core nonvacuity")
        for geometry, count, rows in (("line12", 0, 0), ("shell14", 444, 456), ("spatial12", 1428, 1644)):
            census, terminal = [json.loads(line) for line in
                read("ownerfix/t2_trial/" + mode + "/" + geometry + ".stdout").splitlines()]
            need(census["orders"] == 180 and census["physical_tower_pairs"] == 16 and
                terminal["paired_terminals"] == count and terminal["paired_trace_rows"] == rows and
                terminal["zero_trace_capacity_checks"] == count, "T2 execution/nonexecution nonvacuity")
        cross = json.loads(read("ownerfix/guards_trial/" + mode + "/owner_cross.stdout"))
        need(cross["checks"] == 5 and cross["rejections"] == 1 and
            cross["foreign_owner_refused_before_work"] is True and cross["fresh_process"] is True,
            "cross-type owner nominal rejection")
        guards = json.loads(read("ownerfix/guards_trial/" + mode + "/selftest.stdout"))
        need(guards["checks"] == 139 and guards["rejections"] == 31 and guards["signed_key_comparisons"] == 81 and
            guards["trace_capacity_one_complete_result"] is True, "guard nonvacuity")
    if args.extract:
        args.extract.mkdir(exist_ok=False)
        for name in mapping:
            target = args.extract / relative(name)
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open("xb") as output:
                output.write(read(name))
    print(json.dumps({"status": "passed", "scope": capture["scope"], "logical_files": len(mapping),
        "physical_files": len(actual), "receipts": len(capture["required_receipts"]), "commands": commands,
        "causal_diagnostics": causal, "ELF_pins_only": len(capture["binary_pins_no_ELF"]),
        "geometry_executed_by_reader": False, "device_executed": False}, sort_keys=True))


if __name__ == "__main__":
    main()
