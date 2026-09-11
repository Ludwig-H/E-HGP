#!/usr/bin/env python3
"""Create-only compact terminal host-stub evidence, after all ROOT SAN close."""
import difflib
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
OLD = REPO / "build/v7_gpu_static_terminal_20260911"
NEW = REPO / "build/v7_gpu_static_terminal_ownerfix_20260911"
TARGET = ROOT / "packet"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as output:
        output.write(data)


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def main():
    if TARGET.exists():
        raise RuntimeError("create-only packet already exists")
    required = {
        "r2": (OLD, ("o2_r1", "o2_r2", "san_root_r1", "t2_trial/o2_r1", "t2_trial/san_root_r1",
                     "guards_trial/o2_r1", "guards_trial/san_root_r1")),
        "ownerfix": (NEW, ("o2_r1", "san_root_r1", "t2_trial/o2_r1", "t2_trial/san_root_r1",
                          "guards_trial/o2_r1", "guards_trial/san_root_r1")),
    }
    accepted_receipts = {}
    for label, (base, captures) in required.items():
        for name in captures:
            path = base / name / "receipt.json"
            data = json.loads(path.read_text())
            if data.get("status") != "passed" or data.get("sources_stable") is not True or \
                    data.get("sources_before") != data.get("sources_after"):
                raise RuntimeError("capture not closed: " + str(path))
            for command in data["commands"]:
                if command["exit_code"] != command["expected_exit_code"] or \
                        command.get("causal_diagnostic_matched", True) is not True:
                    raise RuntimeError("unexpected captured command")
            accepted_receipts[label + "/" + name + "/receipt.json"] = digest(path.read_bytes())
    owner_pin = "be3c422c8ff7a09650c7a91d175eebfef21585ca160150fb40bae83dcdd13a60"
    old_owner_pin = "a6b88bafe9b546bb7569d5dd47bcae04c2db428f1b03d6d35027d96e9db4d971"
    if digest((NEW / "terminal_owner.hpp").read_bytes()) != owner_pin or \
            digest((OLD / "terminal_owner.hpp").read_bytes()) != old_owner_pin:
        raise RuntimeError("reviewed owner pin changed")
    causal = json.loads((NEW / "guards_trial/o2_r1/receipt.json").read_text())
    witness = [entry for entry in causal["mutants"] if entry["name"] == "historical_owner"]
    if len(witness) != 1 or witness[0]["source_sha256"] != old_owner_pin or \
            witness[0].get("historical_owner_byte_identical") is not True:
        raise RuntimeError("historical owner causal witness missing")

    logical = {}
    binaries = {}
    source_paths = {}
    for label, (base, _) in required.items():
        for path in sorted(base.rglob("*")):
            relative = path.relative_to(base)
            if "cuda_trial" in relative.parts or "__pycache__" in relative.parts:
                continue  # independent, still-preparatory CUDA work is not sealed here
            if path.is_symlink():
                raise RuntimeError("source symlink: " + str(path))
            if not path.is_file():
                continue
            payload = path.read_bytes()
            name = label + "/" + relative.as_posix()
            entry = {"size": len(payload), "sha256": digest(payload)}
            if payload[:4] == b"\x7fELF":
                binaries[name] = entry
                continue
            payload.decode("utf-8")
            logical[name] = entry
            source_paths[name] = path
    for name, expected in accepted_receipts.items():
        if logical.get(name, {}).get("sha256") != expected:
            raise RuntimeError("capture changed during collection")
    # Explicitly verify immutable historical import, not only its current owner.
    imported = json.loads((NEW / "ownerfix_import.json").read_text())
    for name, expected in imported["source_pins"].items():
        if logical.get("r2/" + name, {}).get("sha256") != expected:
            raise RuntimeError("historical tree changed: " + name)
    for name, expected in imported["historical_receipt_pins"].items():
        if logical.get("r2/" + name, {}).get("sha256") != expected:
            raise RuntimeError("historical receipt changed: " + name)

    TARGET.mkdir(exist_ok=False)
    stored = {}
    mapping = {}
    # Keep current readable source layout; history/snapshots use deduplicated
    # content objects with a lossless logical-path reconstruction table.
    for name, path in source_paths.items():
        relative = Path(name)
        direct = relative.parts[0] == "ownerfix" and (relative.parts[1] in ("source", "originals", "upstream_intruder") or
            len(relative.parts) == 2 or (len(relative.parts) == 3 and relative.parts[1] in ("t2_trial", "guards_trial")))
        if direct:
            destination = "sources/current/" + Path(*relative.parts[1:]).as_posix()
            payload = path.read_bytes()
            if digest(payload) != logical[name]["sha256"]:
                raise RuntimeError("source drift while copying")
            write(TARGET / destination, payload)
            stored.setdefault(logical[name]["sha256"], destination)
            mapping[name] = {**logical[name], "storage": destination}
    for name, path in source_paths.items():
        entry = logical[name]
        payload = path.read_bytes()
        if digest(payload) != entry["sha256"] or len(payload) != entry["size"]:
            raise RuntimeError("source drift while copying")
        if name in mapping:
            continue
        if entry["sha256"] not in stored:
            destination = "objects/" + entry["sha256"] + ".source"
            write(TARGET / destination, payload)
            stored[entry["sha256"]] = destination
        mapping[name] = {**entry, "storage": stored[entry["sha256"]]}

    patches = {
        "ownerfix.patch": (OLD / "terminal_owner.hpp", NEW / "terminal_owner.hpp"),
        "owner_cross_gate.patch": (OLD / "guards_trial/gate.cpp", NEW / "guards_trial/gate.cpp"),
        "full_stub_integration.patch": (OLD / "originals/src/forest/full_ball_tower.hpp",
                                        NEW / "source/morsehgp3D_v7/src/forest/full_ball_tower.hpp"),
        "ordinal_u64.patch": (OLD / "o2_r1/source_snapshot/terminal.cuh", OLD / "terminal.cuh"),
    }
    for name, (a, b) in patches.items():
        patch = "".join(difflib.unified_diff(a.read_text().splitlines(True), b.read_text().splitlines(True),
            fromfile=str(a.relative_to(REPO)), tofile=str(b.relative_to(REPO))))
        write(TARGET / "patches" / name, patch.encode())
    capture = {"scope": "private_terminal_FULL_host_stub_c03", "base_commit": "c03f6be8488453486b112811071827a96303ec86",
        "device_executed": False, "gcp_used": False, "public_status": "not_claimed",
        "historical_owner_is_defective": True, "ownerfix_pin": owner_pin, "historical_owner_pin": old_owner_pin,
        "required_receipts": accepted_receipts, "files": logical, "binary_pins_no_ELF": binaries}
    capture_bytes = encoded(capture)
    write(TARGET / "capture_manifest.json", capture_bytes)
    write(TARGET / "storage_map.json", encoded(mapping))
    write(TARGET / "README.md", (ROOT / "README.md").read_bytes())
    write(TARGET / "scripts/publish.py", Path(__file__).read_bytes())
    reader = (ROOT / "verify.py").read_text().replace("@CAPTURE_PIN@", digest(capture_bytes))
    write(TARGET / "verify.py", reader.encode())
    files = {}
    for path in sorted(TARGET.rglob("*")):
        if path.is_file():
            data = path.read_bytes()
            files[path.relative_to(TARGET).as_posix()] = {"size": len(data), "sha256": digest(data)}
    write(TARGET / "manifest.json", encoded({"scope": capture["scope"], "device_executed": False,
        "gcp_used": False, "files": files}))
    print(json.dumps({"status": "published_private", "physical_files": len(files)+1,
        "logical_files": len(logical), "ELF_pins_only": len(binaries),
        "bytes": sum(entry["size"] for entry in files.values()),
        "manifest_sha256": digest((TARGET / "manifest.json").read_bytes())}, sort_keys=True))


if __name__ == "__main__":
    main()
