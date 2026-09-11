#!/usr/bin/env python3
"""Create-only PRIVATE evidence package. No active tree, compiler, Git or GCP."""
import difflib
import json
from pathlib import Path

import verify

HERE = Path(__file__).resolve().parent
BASE = HERE.parent / "v7_terminal_batch_20260911"
TRAFFIC = HERE.parent / "v7_terminal_batch_traffic_20260911"
TARGET = HERE / "packet"


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as output:
        output.write(data)


def tree(root):
    result = {}
    for path in sorted(root.rglob("*")):
        verify.need(not path.is_symlink(), "source symlink")
        if path.is_file():
            result[path.relative_to(root).as_posix()] = path.read_bytes()
    return result


def main():
    verify.need(not TARGET.exists(), "create-only packet already exists")
    expected_root = {name.split("/")[1] for name in verify.CAPTURES if name.startswith("root/")}
    expected_traffic = {name.split("/")[1] for name in verify.CAPTURES if name.startswith("traffic/")}
    verify.need({p.parent.name for p in BASE.glob("*/receipt.json")} == expected_root, "unaccounted or missing ROOT capture")
    verify.need({p.parent.name for p in TRAFFIC.glob("*/receipt.json")} == expected_traffic, "unaccounted isolated capture")
    payloads, receipts, original_pins = {}, {}, {}
    for name, (_, status, _) in verify.CAPTURES.items():
        family, short = name.split("/")
        directory = (BASE if family == "root" else TRAFFIC) / short
        data = tree(directory)
        receipt = json.loads(data["receipt.json"])
        verify.need(receipt["status"] == status, "capture not finally closed: " + name)
        for relative, contents in data.items():
            payloads["captures/" + name + "/" + relative] = contents
            original_pins[str(directory / relative)] = verify.sha(contents)
        receipts[name] = receipt
    final = receipts["root/o2_r5"]["sources_before"]
    for name, pin in final.items():
        source = BASE / name
        contents = source.read_bytes()
        verify.need(verify.sha(contents) == pin, "ROOT current source drift: " + name)
        payloads["current/" + name] = contents
        original_pins[str(source)] = pin
    declared_root = {"record.py"} | {"prototype/" + name for name in tree(BASE / "prototype")}
    verify.need(declared_root == set(final), "ROOT current full closure changed")
    for name in ("batch_route.cuh.patch", "batch_gate.cpp.patch"):
        source = TRAFFIC / "patches" / name
        payloads["isolated_traffic/patches/" + name] = source.read_bytes()
        original_pins[str(source)] = verify.sha(source.read_bytes())
    for before, after in (("o2_r1", "o2_r2"), ("o2_r2", "o2_r3"), ("o2_r3", "o2_r4"), ("o2_r4", "o2_r5")):
        for name in sorted(verify.CHANGING):
            old = payloads["captures/root/" + before + "/source_snapshot/" + name]
            new = payloads["captures/root/" + after + "/source_snapshot/" + name]
            if old != new:
                patch = "".join(difflib.unified_diff(old.decode().splitlines(keepends=True),
                    new.decode().splitlines(keepends=True), fromfile=before + "/" + name, tofile=after + "/" + name))
                payloads["patches/" + before + "_to_" + after + "/" + name + ".patch"] = patch.encode()
    files, binaries = {}, {}
    for name, data in payloads.items():
        entry = {"sha256": verify.sha(data), "size": len(data)}
        if data[:4] == b"\x7fELF":
            binaries[name] = entry
        else:
            data.decode("utf-8")
            files[name] = entry
    captured = {
        "scope": "private_seeded_terminal_batch_HOST_STUB", "public_status": "not_claimed",
        "device_executed": False, "cuda_compiled": False, "gcp_used": False,
        "captures": {name: verify.sha(payloads["captures/" + name + "/receipt.json"]) for name in verify.CAPTURES},
        "summaries": {name: json.loads(payloads["captures/" + name + "/selftest.stdout"])
                      for name, receipt in receipts.items() if receipt["status"] == "passed"},
        "files": files, "binary_pins_no_ELF": binaries,
        "historical_failures": {"root/o2_r1": "strict compile: signedness and nonexistent FullBallStatus::kOk",
                                "root/o2_r3": "SIGABRT: fixture caught std::overflow_error, not full_ball_detail::Failure"},
        "isolated_traffic_not_inherited": True,
    }
    # All semantic and source checks precede directory creation. Missing/running
    # SAN or an unexpected prior failure cannot leave an apparently sealed pack.
    report = verify.qualify(lambda name: payloads[name], captured)
    for name, pin in original_pins.items():
        verify.need(verify.sha(Path(name).read_bytes()) == pin, "input drift while packing")
    TARGET.mkdir(exist_ok=False)
    mapping, stored = {}, {}
    for name, entry in sorted(files.items(), key=lambda row: (not row[0].startswith("current/"), row[0])):
        if name.startswith("current/"):
            destination = "sources/" + name
        elif name.startswith(("patches/", "isolated_traffic/")) or (name.startswith("captures/") and len(Path(name).parts) == 4):
            destination = name
        else:
            destination = stored.get(entry["sha256"], "objects/" + entry["sha256"] + ".source")
        if not (TARGET / destination).exists():
            write(TARGET / destination, payloads[name])
        stored.setdefault(entry["sha256"], destination)
        mapping[name] = {**entry, "storage": destination}
    capture_bytes = encoded(captured)
    write(TARGET / "capture_manifest.json", capture_bytes)
    write(TARGET / "storage_map.json", encoded(mapping))
    write(TARGET / "README.md", (HERE / "README.md").read_bytes())
    write(TARGET / "scripts/publish.py", Path(__file__).read_bytes())
    write(TARGET / "verify.py", (HERE / "verify.py").read_text().replace("@CAPTURE_PIN@", verify.sha(capture_bytes)).encode())
    physical = {}
    for name, data in tree(TARGET).items():
        physical[name] = {"sha256": verify.sha(data), "size": len(data)}
    write(TARGET / "manifest.json", encoded({"scope": captured["scope"], "files": physical,
        "device_executed": False, "cuda_compiled": False, "gcp_used": False}))
    report.update(manifest_sha256=verify.sha((TARGET / "manifest.json").read_bytes()),
                  physical_files=len(physical) + 1, logical_files=len(files), ELF_pins_only=len(binaries))
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
