#!/usr/bin/env python3
"""Create-only packaging of existing captures; never compiles or runs C++."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import zlib

HERE = Path(__file__).resolve().parent
SOURCE = Path("/workspaces/E-HGP/build/v7_meb_diameter_20260911")
CAPTURES = ("o2_r1", "san_root_r1", "mutant_ties_r1", "mutant_counter_r1", "mutant_order_r1", "mutant_shell_r1")
VISIBLE = ("anchor_meb_diameter.hpp", "gate.cpp", "PROOF.md", "variant.diff", "origin.json", "prepare.py", "record.py")


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def pin(data):
    return {"sha256": sha(data), "bytes": len(data)}


def json_bytes(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as output:
        output.write(data)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--publish", type=Path)
    args = parser.parse_args()
    out = args.out.resolve()
    need(not out.exists(), "fresh output required")
    if args.publish:
        need(not args.publish.exists(), "fresh publication required")
    logical = {}
    capture_meta = {}
    pins = None
    original_files = {}
    for capture in CAPTURES:
        root = SOURCE / capture
        receipt_bytes = (root / "receipt.json").read_bytes()
        receipt = json.loads(receipt_bytes)
        need(receipt["status"] == "passed" and receipt["sources_stable"] is True and receipt["snapshot_stable"] is True, "closed stable capture required")
        if pins is None:
            pins = receipt["sources_before"]
        need(receipt["sources_before"] == receipt["sources_after"] == receipt["snapshot_after"] == pins, "capture source drift")
        files = [path for path in root.rglob("*") if path.is_file()]
        need(not any(path.is_symlink() for path in root.rglob("*")), "capture symlink")
        binary = (root / "gate").read_bytes()
        need(binary.startswith(b"\x7fELF") and sha(binary) == receipt["binary_sha256"] == receipt["binary_after_sha256"], "current ELF pin")
        capture_meta[capture] = {"receipt_sha256": sha(receipt_bytes), "binary_sha256": sha(binary),
            "binary_bytes": len(binary), "body_omitted": True, "binary_rehashed_at_packaging": True}
        for path in files:
            original_files[path] = sha(path.read_bytes())
            relative = path.relative_to(root).as_posix()
            if relative == "gate":
                continue
            data = path.read_bytes()
            need(not data.startswith(b"\x7fELF") and not relative.endswith(".pyc"), "unexpected executable")
            logical["captures/" + capture + "/" + relative] = data
    need(pins is not None and len(pins) == 68, "source count")
    for name, expected in pins.items():
        data = (SOURCE / name).read_bytes()
        need(sha(data) == expected, "current source drift")
        logical["sources/current/" + name] = data
        original_files[SOURCE / name] = expected
    capture_manifest = {"schema": "meb_diameter_captures_v1",
        "scope": "private_cpu_u16_diameter_no_performance_no_cuda",
        "logical_files": {name: pin(data) for name, data in sorted(logical.items())},
        "captures": capture_meta, "source_pins": pins,
        "limits": {"compiler_pinned": False, "boost_system_headers_pinned": False,
                   "ELF_bodies_distributed": False, "cuda_qualified": False,
                   "benchmark": False, "inherited_results": False}}
    capture_bytes = json_bytes(capture_manifest)
    out.mkdir(parents=True, exist_ok=False)
    storage_by_hash = {}
    for relative in VISIBLE:
        name = "sources/current/" + relative
        data = logical[name]
        write(out / name, data)
        storage_by_hash[sha(data)] = {"storage": name, "encoding": "raw",
            "stored_sha256": sha(data), "stored_bytes": len(data), **pin(data)}
    mapping = {}
    for name, data in sorted(logical.items()):
        digest = sha(data)
        if digest not in storage_by_hash:
            stored = zlib.compress(data, 9)
            storage = "objects/" + digest + ".zlib"
            write(out / storage, stored)
            storage_by_hash[digest] = {"storage": storage, "encoding": "zlib",
                "stored_sha256": sha(stored), "stored_bytes": len(stored), **pin(data)}
        mapping[name] = storage_by_hash[digest]
    write(out / "capture_manifest.json", capture_bytes)
    write(out / "storage_map.json", json_bytes(mapping))
    reader = (HERE / "verify.py").read_bytes()
    marker = b"@CAPTURE_MANIFEST_SHA256@"
    need(reader.count(marker) == 1, "reader marker")
    write(out / "verify.py", reader.replace(marker, sha(capture_bytes).encode()))
    write(out / "publish.py", Path(__file__).read_bytes())
    write(out / "README.md", (HERE / "README.md").read_bytes())
    physical = {path.relative_to(out).as_posix(): pin(path.read_bytes()) for path in out.rglob("*") if path.is_file()}
    write(out / "manifest.json", json_bytes({"schema": "meb_diameter_physical_v1", "files": physical}))
    validation = out.with_name(out.name + "_validation")
    validation.mkdir(exist_ok=False)
    commands = []

    def check(label, argv):
        result = subprocess.run(argv, capture_output=True, check=False)
        write(validation / (label + ".stdout"), result.stdout)
        write(validation / (label + ".stderr"), result.stderr)
        commands.append({"name": label, "argv": argv, "exit_code": result.returncode,
            "stdout": pin(result.stdout), "stderr": pin(result.stderr)})
        write(validation / (label + ".json"), json_bytes(commands[-1]))
        need(result.returncode == 0 and not result.stderr, "reader failed: " + label)

    check("normal", [sys.executable, "-B", str(out / "verify.py")])
    check("optimized", [sys.executable, "-B", "-O", str(out / "verify.py")])
    extracted = validation / "extracted"
    check("extraction", [sys.executable, "-B", str(out / "verify.py"), "--extract", str(extracted)])
    actual = {path.relative_to(extracted).as_posix(): path.read_bytes() for path in extracted.rglob("*") if path.is_file()}
    need(actual == logical, "extracted bytes differ")
    need(all(sha(path.read_bytes()) == digest for path, digest in original_files.items()), "original changed during package")
    if args.publish:
        shutil.copytree(out, args.publish)
        check("published_normal", [sys.executable, "-B", str(args.publish / "verify.py")])
        check("published_optimized", [sys.executable, "-B", "-O", str(args.publish / "verify.py")])
        published = {path.relative_to(args.publish).as_posix(): pin(path.read_bytes()) for path in args.publish.rglob("*") if path.is_file()}
        expected = {**physical, "manifest.json": pin((out / "manifest.json").read_bytes())}
        need(published == expected, "publication differs")
    report = {"status": "passed", "geometry_reexecuted": False, "GCP_used": False,
        "commands": commands, "extraction_equal": True, "original_sources_captures_ELFs_stable": True,
        "manifest_sha256": sha((out / "manifest.json").read_bytes()), "capture_manifest_sha256": sha(capture_bytes),
        "logical_files": len(logical), "physical_files": len(physical) + 1,
        "physical_bytes": sum(value["bytes"] for value in physical.values()) + (out / "manifest.json").stat().st_size,
        "publication": str(args.publish) if args.publish else None}
    write(validation / "receipt.json", json_bytes(report))
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
