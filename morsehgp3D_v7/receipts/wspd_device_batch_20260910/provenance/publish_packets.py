#!/usr/bin/env python3
"""Assemble two proof-only receipt directories; no Git/GCP mutation."""
from pathlib import Path
import hashlib
import json
import shutil


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path: Path, value: object) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def seal(root: Path) -> None:
    files = {}
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path == root / "manifest.json":
            continue
        if path.is_symlink() or path.read_bytes().startswith(b"\x7fELF"):
            raise RuntimeError("symlink or ELF forbidden")
        files[str(path.relative_to(root))] = dict(bytes=path.stat().st_size, sha256=sha(path))
    save(root / "manifest.json", files)


def main() -> int:
    root = Path(__file__).resolve().parent
    repo = root.parents[1]
    receipts = repo / "morsehgp3D_v7/receipts"
    nvcc = receipts / "nvcc_strict_host_20260910"
    witness = receipts / "wspd_device_batch_20260910"
    if nvcc.exists() or witness.exists():
        raise RuntimeError("publication target exists")
    original = root / "run_r1/packet"
    shutil.copytree(original, nvcc)
    (nvcc / "provenance").mkdir()
    shutil.copy2(original / "manifest.json", nvcc / "provenance/private_manifest.json")
    shutil.copy2(root / "PUBLISHED_README.md", nvcc / "README.md")
    seal(nvcc)
    witness.mkdir()
    for directory in ("objects", "source_views", "runs", "provenance"):
        (witness / directory).mkdir()
    experiment = repo / "build/v7_wspd_device_20260910_r2"
    shutil.copy2(experiment / "packet_reader.py", witness / "reader.py")
    shutil.copy2(experiment / "PUBLISHED_README.md", witness / "README.md")
    shutil.copy2(experiment / "README.md", witness / "IMPLEMENTATION_NOTE.md")
    shutil.copy2(Path(__file__).resolve(), witness / "provenance/publish_packets.py")
    capture = dict(device_executed=False, GCP="not_used_by_this_agent", runs={})
    for name in ("run_r1", "run_r2"):
        source = experiment / name
        target = witness / "runs" / name
        target.mkdir()
        receipt_path = source / "receipt.json"
        receipt = json.loads(receipt_path.read_text())
        shutil.copy2(receipt_path, target / "receipt.json")
        save(witness / "source_views" / (name + ".json"), receipt["sources_before"])
        for relative, digest in receipt["sources_before"].items():
            path = source / "source" / relative
            if sha(path) != digest:
                raise RuntimeError("source snapshot pin")
            blob = witness / "objects" / digest
            if not blob.exists():
                shutil.copy2(path, blob)
            if sha(blob) != digest:
                raise RuntimeError("object pin")
        for command in receipt["commands"]:
            for stream in ("stdout", "stderr"):
                item = command[stream]
                path = source / item["path"]
                if sha(path) != item["sha256"] or path.stat().st_size != item["bytes"]:
                    raise RuntimeError("original stream pin")
                shutil.copy2(path, target / item["path"])
        binary_hashes = {}
        for key, item in receipt["binaries"].items():
            digest = sha(source / item["path"])
            if digest != item["sha256"] or digest != item["sha256_after"]:
                raise RuntimeError("original binary pin")
            binary_hashes[key] = digest
        capture["runs"][name] = dict(receipt_sha256=sha(receipt_path), binary_sha256_at_capture=binary_hashes)
    save(witness / "capture.json", capture)
    seal(witness)
    print("published without Git:", nvcc, witness)
    print("nvcc manifest", sha(nvcc / "manifest.json"))
    print("witness manifest", sha(witness / "manifest.json"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
