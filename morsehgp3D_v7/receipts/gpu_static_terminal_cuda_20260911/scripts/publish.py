#!/usr/bin/env python3
"""Create-only portable local CUDA-terminal evidence; no active publication."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
BASE = REPO / "build/v7_gpu_static_terminal_ownerfix_20260911"
CUDA = BASE / "cuda_trial"
HOST = REPO / "build/v7_gpu_static_terminal_publish_20260911/packet"
TARGET = ROOT / "packet"
HOST_PIN = "3ac9ca12fc22433fcedff2d7130f2a78031799fff81687c4b87c2d7945ca5ef1"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def write(path, payload):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as output:
        output.write(payload)


def main():
    if TARGET.exists():
        raise RuntimeError("create-only packet already exists")
    receipts = {}
    paths = {}
    for name, mode in (("export_r1", "export"), ("stub_r1", "stub"), ("san_root_r1", "san"), ("nvcc_r1", "nvcc")):
        directory = CUDA / name
        data = json.loads((directory / "receipt.json").read_text())
        if data.get("status") != "passed" or data.get("mode") != mode or data.get("sources_stable") is not True or \
                data.get("snapshot_stable") is not True or data["sources_before"] != data["sources_after"] or \
                data.get("device_executed") is not False:
            raise RuntimeError("capture not closed: " + name)
        for command in data["commands"]:
            if command["exit_code"] != command["expected_exit_code"]:
                raise RuntimeError("unexpected command: " + name)
        receipts[name] = data
        for path in sorted(directory.rglob("*")):
            if path.is_symlink():
                raise RuntimeError("capture symlink")
            if path.is_file():
                paths["captures/" + name + "/" + path.relative_to(directory).as_posix()] = path
    exported = receipts["export_r1"]
    for name, digest in exported["sources_before"].items():
        path = BASE / name
        if sha(path.read_bytes()) != digest:
            raise RuntimeError("current source changed: " + name)
        paths["current/" + name] = path
    for data in receipts.values():
        if data["sources_before"] != exported["sources_before"] or data["fixture_binding"] != exported["fixture_binding"]:
            raise RuntimeError("capture source/fixture authority differs")
    fixture = CUDA / "export_r1/terminal_fixtures.inc"
    if sha(fixture.read_bytes()) != exported["fixture_sha256"]:
        raise RuntimeError("fixture drift")
    paths["current/cuda_trial/terminal_fixtures.inc"] = fixture
    if sha((HOST / "manifest.json").read_bytes()) != HOST_PIN:
        raise RuntimeError("host qualification packet changed")
    paths["qualification/host_manifest.json"] = HOST / "manifest.json"
    paths["qualification/host_capture_manifest.json"] = HOST / "capture_manifest.json"
    for name, digest in exported["ownerfix_prerequisites"].items():
        path = BASE / name
        if sha(path.read_bytes()) != digest:
            raise RuntimeError("ownerfix prerequisite changed")
        paths["qualification/" + name] = path

    files = {}
    binaries = {}
    for name, path in paths.items():
        payload = path.read_bytes()
        entry = {"size": len(payload), "sha256": sha(payload)}
        if payload[:4] == b"\x7fELF":
            binaries[name] = entry
        else:
            payload.decode("utf-8")
            files[name] = entry
    TARGET.mkdir(exist_ok=False)
    stored = {}
    mapping = {}
    for name, entry in sorted(files.items(), key=lambda item: (not item[0].startswith("current/"), item[0])):
        payload = paths[name].read_bytes()
        if sha(payload) != entry["sha256"] or len(payload) != entry["size"]:
            raise RuntimeError("source drift during packaging")
        if name.startswith("current/"):
            destination = "sources/" + name
        elif name.startswith("qualification/"):
            destination = name
        elif len(Path(name).parts) == 3:
            destination = name
        else:
            destination = stored.get(entry["sha256"], "objects/" + entry["sha256"] + ".source")
        if not (TARGET / destination).exists():
            write(TARGET / destination, payload)
        stored.setdefault(entry["sha256"], destination)
        mapping[name] = {**entry, "storage": destination}
    captured = {"scope": "private_terminal_CUDA_gate_local_compile_only_K2_K8",
        "base_commit": "c03f6be8488453486b112811071827a96303ec86", "device_executed": False,
        "gcp_used": False, "public_status": "not_claimed", "host_qualification_manifest_sha256": HOST_PIN,
        "fixture_sha256": exported["fixture_sha256"], "export_summary": exported["export_summary"],
        "files": files, "binary_pins_no_ELF": binaries}
    capture_bytes = encoded(captured)
    write(TARGET / "capture_manifest.json", capture_bytes)
    write(TARGET / "storage_map.json", encoded(mapping))
    write(TARGET / "README.md", (ROOT / "README.md").read_bytes())
    write(TARGET / "scripts/publish.py", Path(__file__).read_bytes())
    reader = (ROOT / "verify.py").read_text().replace("@CAPTURE_PIN@", sha(capture_bytes))
    write(TARGET / "verify.py", reader.encode())
    physical = {}
    for path in sorted(TARGET.rglob("*")):
        if path.is_file():
            payload = path.read_bytes()
            physical[path.relative_to(TARGET).as_posix()] = {"size": len(payload), "sha256": sha(payload)}
    write(TARGET / "manifest.json", encoded({"scope": captured["scope"], "files": physical,
        "device_executed": False, "gcp_used": False}))
    print(json.dumps({"status": "published_private", "physical_files": len(physical)+1,
        "logical_files": len(files), "ELF_pins_only": len(binaries),
        "bytes": sum(entry["size"] for entry in physical.values()),
        "manifest_sha256": sha((TARGET / "manifest.json").read_bytes())}, sort_keys=True))


if __name__ == "__main__":
    main()
