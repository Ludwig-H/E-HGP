#!/usr/bin/env python3
"""Create-only compact K9/K10 local evidence; no active publication or geometry."""
import difflib
import hashlib
import json
from pathlib import Path
import stat

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
BASE = REPO / "build/v7_gpu_terminal_k10_20260911"
CUDA = BASE / "cuda_trial"
OLD = REPO / "morsehgp3D_v7/receipts/gpu_static_terminal_cuda_20260911"
TARGET = ROOT / "packet_r2"
OLD_PIN = "5b631b10cf329f549e473f59b73777f441c85fe55374af1c6af56311bcb1da29"
HOST_PIN = "3ac9ca12fc22433fcedff2d7130f2a78031799fff81687c4b87c2d7945ca5ef1"
CAPTURES = {"export_r1": "export", "stub_r1": "stub", "san_root_r1": "san", "nvcc_r1": "nvcc", "nvcc_r2": "nvcc"}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as output:
        output.write(data)


def main():
    if TARGET.exists():
        raise RuntimeError("create-only packet already exists")
    found = {path.parent.name for path in CUDA.glob("*/receipt.json")}
    if found != set(CAPTURES):
        raise RuntimeError("unaccounted capture set")
    receipts, payloads = {}, {}
    for name, mode in CAPTURES.items():
        directory = CUDA / name
        data = json.loads((directory / "receipt.json").read_text())
        status = "failed" if name == "nvcc_r1" else "passed"
        if data.get("status") != status or data.get("mode") != mode or data.get("sources_stable") is not True or \
                data.get("snapshot_stable") is not True or data["sources_before"] != data["sources_after"] or \
                data.get("device_executed") is not False:
            raise RuntimeError("capture not closed as expected: " + name)
        for command in data["commands"]:
            expected = 1 if name == "nvcc_r1" and command["name"] == "compile_link" else command["expected_exit_code"]
            if command["exit_code"] != expected:
                raise RuntimeError("unexpected command: " + name)
        if name == "nvcc_r1" and b"Permission denied" not in (directory / "compile_link.stderr").read_bytes():
            raise RuntimeError("historical failure cause changed")
        receipts[name] = data
        for path in sorted(directory.rglob("*")):
            if path.is_symlink():
                raise RuntimeError("capture symlink")
            if path.is_file():
                payloads["captures/" + name + "/" + path.relative_to(directory).as_posix()] = path.read_bytes()
    exported = receipts["export_r1"]
    for name, pin in exported["sources_before"].items():
        data = (BASE / name).read_bytes()
        if sha(data) != pin:
            raise RuntimeError("current source drift: " + name)
        payloads["current/" + name] = data
    for data in receipts.values():
        if data["sources_before"] != exported["sources_before"] or data["fixture_binding"] != exported["fixture_binding"]:
            raise RuntimeError("different capture source closure")
    payloads["current/cuda_trial/terminal_fixtures.inc"] = (CUDA / "export_r1/terminal_fixtures.inc").read_bytes()
    payloads["current/README.md"] = (BASE / "README.md").read_bytes()
    for path in sorted((BASE / "originals").rglob("*")):
        if path.is_file():
            payloads["originals/" + path.relative_to(BASE / "originals").as_posix()] = path.read_bytes()
    for name in ("host_manifest.json", "host_capture_manifest.json"):
        payloads["qualification/" + name] = (BASE / "qualification" / name).read_bytes()
    if sha(payloads["qualification/host_manifest.json"]) != HOST_PIN:
        raise RuntimeError("host qualification authority changed")
    for name, pin in exported["ownerfix_prerequisites"].items():
        data = (BASE / "qualification" / name).read_bytes()
        if sha(data) != pin:
            raise RuntimeError("host prerequisite drift")
        payloads["qualification/" + name] = data
    if sha((OLD / "manifest.json").read_bytes()) != OLD_PIN:
        raise RuntimeError("imported public packet changed")
    for name in ("manifest.json", "capture_manifest.json"):
        payloads["qualification/k8_" + name] = (OLD / name).read_bytes()
    for name in ("export.cpp", "device_gate.cu", "record.py"):
        before = (BASE / "originals/k8" / name).read_text().splitlines(keepends=True)
        after = (CUDA / name).read_text().splitlines(keepends=True)
        payloads["patches/" + name + ".patch"] = "".join(difflib.unified_diff(before, after,
            fromfile="k8/cuda_trial/" + name, tofile="k10/cuda_trial/" + name)).encode()
    mode_observations = {}
    for name, expected_mode in (("nvcc_r1", 0o666), ("nvcc_r2", 0o700)):
        path = CUDA / name / "source_snapshot/nvcc_strict_host.py"
        observed_mode = stat.S_IMODE(path.stat().st_mode)
        pin = sha(path.read_bytes())
        if observed_mode != expected_mode or pin != exported["sources_before"]["nvcc_strict_host.py"]:
            raise RuntimeError("observed adapter mode/content mismatch")
        mode_observations[name] = {"logical_path": "captures/" + name + "/source_snapshot/nvcc_strict_host.py",
            "observed_mode_octal": format(observed_mode, "04o"), "content_sha256": pin}
    payloads["history/packaging_r1_manifest.json"] = (ROOT / "packet/manifest.json").read_bytes()
    files, binaries = {}, {}
    for name, data in payloads.items():
        entry = {"size": len(data), "sha256": sha(data)}
        if data[:4] == b"\x7fELF":
            binaries[name] = entry
        else:
            data.decode("utf-8")
            files[name] = entry
    TARGET.mkdir(exist_ok=False)
    stored, mapping = {}, {}
    for name, entry in sorted(files.items(), key=lambda item: (not item[0].startswith("current/"), item[0])):
        data = payloads[name]
        if name.startswith("current/"):
            destination = "sources/" + name
        elif name.startswith(("qualification/", "patches/")) or len(Path(name).parts) == 3:
            destination = name
        else:
            destination = stored.get(entry["sha256"], "objects/" + entry["sha256"] + ".source")
        if not (TARGET / destination).exists():
            write(TARGET / destination, data)
        stored.setdefault(entry["sha256"], destination)
        mapping[name] = {**entry, "storage": destination}
    captured = {"scope": "private_terminal_CUDA_gate_local_compile_only_K2_K10_c03",
        "base_commit": "c03f6be8488453486b112811071827a96303ec86", "device_executed": False,
        "gcp_used": False, "public_status": "not_claimed", "host_qualification_manifest_sha256": HOST_PIN,
        "imported_k8_manifest_sha256": OLD_PIN, "fixture_sha256": exported["fixture_sha256"],
        "export_summary": exported["export_summary"], "plan_sha256": exported["plan_sha256"],
        "files": files, "binary_pins_no_ELF": binaries,
        "executable_mode_repair": {"path": "current/nvcc_strict_host.py", "before_octal": "0666",
            "after_octal": "0700", "content_sha256": sha((BASE / "nvcc_strict_host.py").read_bytes()),
            "historical_failure": "nvcc_r1", "retry": "nvcc_r2", "snapshot_stat_observations": mode_observations}}
    if captured["base_commit"] != exported["base_commit"]:
        raise RuntimeError("unexpected product baseline")
    capture_bytes = encoded(captured)
    write(TARGET / "capture_manifest.json", capture_bytes)
    write(TARGET / "storage_map.json", encoded(mapping))
    for name in ("README.md", "MODES.md"):
        write(TARGET / name, (ROOT / name).read_bytes())
    write(TARGET / "scripts/publish.py", Path(__file__).read_bytes())
    write(TARGET / "verify.py", (ROOT / "verify.py").read_text().replace("@CAPTURE_PIN@", sha(capture_bytes)).encode())
    physical = {}
    for path in sorted(TARGET.rglob("*")):
        if path.is_file():
            data = path.read_bytes()
            physical[path.relative_to(TARGET).as_posix()] = {"size": len(data), "sha256": sha(data)}
    write(TARGET / "manifest.json", encoded({"scope": captured["scope"], "files": physical,
        "device_executed": False, "gcp_used": False}))
    print(json.dumps({"status": "published_private", "physical_files": len(physical) + 1,
        "logical_files": len(files), "ELF_pins_only": len(binaries),
        "bytes": sum(entry["size"] for entry in physical.values()),
        "manifest_sha256": sha((TARGET / "manifest.json").read_bytes())}, sort_keys=True))


if __name__ == "__main__":
    main()
