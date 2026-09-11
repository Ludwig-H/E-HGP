#!/usr/bin/env python3
"""Create-only O2/SAN reproduction, confined to this receipt directory."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tarfile


PACKET = Path(__file__).resolve().parent
STRICT = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
MODES = {
    "O2": ["-O2"],
    "SAN": ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"],
}
SAN_ENV = {
    "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise SystemExit(message)


def read_sources() -> tuple[dict, dict[str, bytes]]:
    pins = json.loads((PACKET / "source_pins.json").read_text())
    archive = (PACKET / pins["archive"]).resolve()
    manifest_path = (PACKET / pins["source_manifest"]).resolve()
    if sha(archive.read_bytes()) != pins["archive_sha256"]:
        fail("archive pin differs")
    if sha(manifest_path.read_bytes()) != pins["source_manifest_sha256"]:
        fail("source manifest pin differs")
    manifest = json.loads(manifest_path.read_text())
    if manifest["base_commit"] != pins["base_commit"]:
        fail("base commit differs")
    files = {}
    with tarfile.open(archive, "r:gz") as source:
        for member in source.getmembers():
            if member.isdir():
                continue
            if not member.isfile() or member.name not in manifest["files"]:
                fail("unexpected archive member")
            if member.name in files:
                fail("duplicate archive member")
            stream = source.extractfile(member)
            if stream is None:
                fail("unreadable archive member")
            content = stream.read()
            if sha(content) != manifest["files"][member.name]:
                fail("archived source hash differs")
            relative = Path(member.name)
            if relative.is_absolute() or ".." in relative.parts:
                fail("unsafe archive path")
            files[member.name] = content
    if set(files) != set(manifest["files"]) or len(files) != pins["source_files"]:
        fail("archive closure differs")
    return pins, files


def source_hashes(work: Path, files: dict[str, bytes]) -> dict[str, str]:
    names = sorted([*files, "certified_support.hpp", "probe.cpp"])
    return {name: sha((work / name).read_bytes()) for name in names}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", required=True)
    args = parser.parse_args()
    work = Path(args.work).resolve()
    if not work.is_relative_to(PACKET) or work == PACKET:
        fail("work must be a new child of this receipt directory")
    pins, files = read_sources()
    work.mkdir(parents=False, exist_ok=False)
    for name, content in files.items():
        destination = work / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(content)
    for name in ("certified_support.hpp", "probe.cpp"):
        (work / name).write_bytes((PACKET / name).read_bytes())
    before = source_hashes(work, files)
    capture = {
        "source_pins": pins,
        "source_hashes_before": before,
        "commands": [],
        "device_executed": False,
        "proposal_generation_executed": False,
        "gcp_used": False,
        "public_status": "not_claimed",
    }
    for mode, flags in MODES.items():
        commands = [
            ("compile", ["g++", *STRICT, *flags, "-I", "morsehgp3D_v7", "probe.cpp", "-o", mode]),
            ("run", ["./" + mode]),
        ]
        for kind, command in commands:
            overrides = SAN_ENV if mode == "SAN" and kind == "run" else {}
            result = subprocess.run(command, cwd=work, env={**os.environ, **overrides},
                                    capture_output=True, text=True, timeout=60, check=False)
            capture["commands"].append({
                "mode": mode, "kind": kind, "argv": command, "env": overrides,
                "returncode": result.returncode, "stdout": result.stdout, "stderr": result.stderr,
            })
            capture["source_hashes_after"] = source_hashes(work, files)
            (work / "commands.json").write_text(json.dumps(capture, indent=2) + "\n")
            if result.returncode != 0 or result.stderr:
                fail(f"{mode} {kind} failed; capture preserved in {work / 'commands.json'}")
    if capture["source_hashes_after"] != before:
        fail("source bytes changed during reproduction")
    print(json.dumps({"status": "passed", "commands": 4, "capture": str(work / "commands.json")}))


if __name__ == "__main__":
    main()
