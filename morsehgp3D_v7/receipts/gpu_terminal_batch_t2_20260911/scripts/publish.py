#!/usr/bin/env python3
"""Create-only PRIVATE T2 batch package; no tests, compiler or product writes."""
import difflib
import json
from pathlib import Path

import verify

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
BASE = HERE.parent / "v7_terminal_batch_t2_20260911"
BATCH = REPO / "morsehgp3D_v7/receipts/gpu_terminal_batch_host_20260911"
CENSUS = REPO / "morsehgp3D_v7/receipts/census_tower_permanent_20260911"
TARGET = HERE / "packet"


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as output:
        output.write(data)


def main():
    verify.need(not TARGET.exists(), "create-only packet exists")
    names = ("o2_r1", "o2_r2", "o2_r3", "san_root_r1")
    verify.need({p.parent.name for p in BASE.glob("*/receipt.json")} == set(names), "complete capture inventory")
    payloads, receipts, originals = {}, {}, {}
    for name in names:
        directory = BASE / name
        receipt = json.loads((directory / "receipt.json").read_text())
        verify.need(receipt["status"] == ("failed" if name == "o2_r1" else "passed") and
                    receipt["sources_stable"] is True and receipt["snapshot_stable"] is True, "closed capture required")
        receipts[name] = receipt
        for path in sorted(directory.rglob("*")):
            verify.need(not path.is_symlink(), "source symlink")
            if path.is_file():
                contents = path.read_bytes()
                payloads["captures/" + name + "/" + path.relative_to(directory).as_posix()] = contents
                originals[str(path)] = verify.sha(contents)
    for name, pin in receipts["o2_r3"]["sources_before"].items():
        contents = (BASE / name).read_bytes()
        verify.need(verify.sha(contents) == pin, "current source drift")
        payloads["current/" + name] = contents
        originals[str(BASE / name)] = pin
    authorities = {"batch_manifest.json": BATCH / "manifest.json", "census_manifest.json": CENSUS / "manifest.json",
        "batch_r5_receipt.json": BATCH / "captures/root/o2_r5/receipt.json"}
    for name in ("census_tower_gate.cpp", "census_tower_oracle.hpp"):
        authorities[name] = CENSUS / "sources/current/morsehgp3D_v7/tests" / name
    for name, path in authorities.items():
        contents = path.read_bytes()
        payloads["qualification/" + name] = contents
        originals[str(path)] = verify.sha(contents)
    for before, after in (("o2_r1", "o2_r2"), ("o2_r2", "o2_r3")):
        relative = "prototype/batch_t2_gate.cpp"
        old = payloads["captures/" + before + "/source_snapshot/" + relative].decode()
        new = payloads["captures/" + after + "/source_snapshot/" + relative].decode()
        payloads["patches/" + before + "_to_" + after + ".patch"] = "".join(difflib.unified_diff(
            old.splitlines(keepends=True), new.splitlines(keepends=True),
            fromfile=before + "/" + relative, tofile=after + "/" + relative)).encode()
    files, binaries = {}, {}
    for name, data in payloads.items():
        entry = {"sha256": verify.sha(data), "size": len(data)}
        if data[:4] == b"\x7fELF":
            binaries[name] = entry
        else:
            data.decode("utf-8")
            files[name] = entry
    observations = {}
    for name in names[1:]:
        observations[name] = {fixture: [json.loads(line) for line in payloads["captures/" + name + "/" + fixture + ".stdout"].decode().splitlines()
                              if line.startswith('{"status":')] for fixture in verify.FIXTURES}
    captured = {"scope": "HOST_STUB_real_census_seeded_batch_T2_and_separate_highK", "public_status": "not_claimed",
        "device_executed": False, "cuda_compiled": False, "gcp_used": False,
        "captures": {name: verify.sha(payloads["captures/" + name + "/receipt.json"]) for name in names},
        "observations": observations, "files": files, "binary_pins_no_ELF": binaries,
        "historical_failure": "o2_r1 const scalar result tried to copy a noncopyable forest; no execution"}
    report = verify.qualify(lambda name: payloads[name], captured)
    for name, pin in originals.items():
        verify.need(verify.sha(Path(name).read_bytes()) == pin, "input drift before sealing")
    TARGET.mkdir(exist_ok=False)
    mapping, stored = {}, {}
    for name, entry in sorted(files.items(), key=lambda row: (not row[0].startswith("current/"), row[0])):
        if name.startswith("current/"):
            destination = "sources/" + name
        elif name.startswith(("qualification/", "patches/")) or len(Path(name).parts) == 3:
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
    for path in sorted(TARGET.rglob("*")):
        if path.is_file():
            data = path.read_bytes()
            physical[path.relative_to(TARGET).as_posix()] = {"sha256": verify.sha(data), "size": len(data)}
    write(TARGET / "manifest.json", encoded({"scope": captured["scope"], "files": physical,
        "device_executed": False, "cuda_compiled": False, "gcp_used": False}))
    report.update(manifest_sha256=verify.sha((TARGET / "manifest.json").read_bytes()),
                  physical_files=len(physical) + 1, logical_files=len(files), ELF_pins_only=len(binaries))
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
