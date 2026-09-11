#!/usr/bin/env python3
"""Create-only private portable CMake/CTest packet, without ELF/vendor/pyc."""
import json
from pathlib import Path

import verify

HERE = Path(__file__).resolve().parent
BASE = HERE.parent / "v7_batch_permanent_20260911"
TARGET = HERE / "packet"


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as output:
        output.write(data)


def main():
    verify.need(not TARGET.exists(), "create-only packet exists")
    verify.need({p.parent.name for p in BASE.glob("*/receipt.json")} == {"o2_r1", "san_root_r1"}, "capture inventory")
    payloads, incidental, binaries, receipts, original_pins = {}, {}, {}, {}, {}
    for name in ("o2_r1", "san_root_r1"):
        directory = BASE / name
        receipt = json.loads((directory / "receipt.json").read_text())
        verify.need(receipt["status"] == "passed" and receipt["sources_stable"] is True and
                    receipt["snapshot_stable"] is True, "capture must be closed PASS: " + name)
        receipts[name] = receipt
        selected = [p for p in sorted(directory.iterdir()) if p.is_file()]
        selected += [p for p in sorted((directory / "source_snapshot").rglob("*")) if p.is_file()]
        for relative in ("CMakeCache.txt", "CTestTestfile.cmake", "compile_commands.json", "Makefile",
                         "CMakeFiles/Makefile2", "CMakeFiles/Makefile.cmake", "CMakeFiles/CMakeConfigureLog.yaml", verify.TARGET):
            selected.append(directory / "cmake_build" / relative)
        target_dir = directory / verify.TARGET_DIR
        selected += [p for p in sorted(target_dir.rglob("*")) if p.is_file() and not p.name.endswith(".o")]
        for path in selected:
            verify.need(not path.is_symlink(), "source symlink")
            relative = path.relative_to(directory).as_posix()
            logical = "captures/" + name + "/" + relative
            data = path.read_bytes()
            entry = {"sha256": verify.sha(data), "size": len(data)}
            original_pins[str(path)] = entry["sha256"]
            if path.suffix == ".pyc":
                verify.need("__pycache__" in path.parts and relative.startswith("source_snapshot/"), "unexpected bytecode")
                incidental[logical] = {**entry, "kind": "unconsumed_python_bytecode"}
            elif data[:4] == b"\x7fELF":
                verify.need(relative == "cmake_build/" + verify.TARGET and entry["sha256"] == receipt["binary_sha256"], "main ELF current pin")
                binaries[logical] = entry
            else:
                data.decode("utf-8")
                payloads[logical] = data
    for relative, pin in receipts["o2_r1"]["snapshot_before"].items():
        if relative.endswith(".pyc"):
            continue
        data = payloads["captures/o2_r1/source_snapshot/" + relative]
        verify.need(verify.sha(data) == pin, "current source pin")
        payloads["current/" + relative] = data
    files = {name: {"sha256": verify.sha(data), "size": len(data)} for name, data in payloads.items()}
    captured = {"scope": "permanent_batch_CTest_CPU83f1", "public_status": "not_claimed",
        "callback_supplied": True, "device_executed": False, "gcp_used": False,
        "captures": {name: verify.sha(payloads["captures/" + name + "/receipt.json"]) for name in receipts},
        "results": receipts["o2_r1"]["CTest_results"], "files": files, "binary_pins_no_ELF": binaries,
        "incidental_pins_no_bytes": incidental, "product_instrumentation": False,
        "generated_selection": "source snapshots, controller logs, CMake cache/inventory/compile commands, target flags/link/dependencies; no object or vendor"}
    report = verify.qualify(lambda name: payloads[name], captured)
    for name, pin in original_pins.items():
        verify.need(verify.sha(Path(name).read_bytes()) == pin, "input drift before sealing")
    TARGET.mkdir(exist_ok=False)
    mapping, stored = {}, {}
    for name, entry in sorted(files.items(), key=lambda row: (not row[0].startswith("current/"), row[0])):
        if name.startswith("current/"):
            destination = "sources/" + name
        elif name.startswith("captures/") and len(Path(name).parts) == 3:
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
    write(TARGET / "scripts/prepare.py", (HERE / "prepare.py").read_bytes())
    write(TARGET / "verify.py", (HERE / "verify.py").read_text().replace("@CAPTURE_PIN@", verify.sha(capture_bytes)).encode())
    physical = {}
    for path in sorted(TARGET.rglob("*")):
        if path.is_file():
            data = path.read_bytes()
            physical[path.relative_to(TARGET).as_posix()] = {"sha256": verify.sha(data), "size": len(data)}
    write(TARGET / "manifest.json", encoded({"scope": captured["scope"], "files": physical,
        "device_executed": False, "gcp_used": False, "callback_supplied": True}))
    report.update(manifest_sha256=verify.sha((TARGET / "manifest.json").read_bytes()),
                  physical_files=len(physical) + 1, logical_files=len(files), ELF_pins_only=len(binaries))
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
