#!/usr/bin/env python3
"""Archive only the initial global capture, preserving originals in its build.

Two deterministic GNU tar/gzip generations and a fresh extraction are checked
byte-for-byte before the original directory is moved. Nothing is deleted.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_wspd_q34_lidar import SOURCES
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import require, utc_stamp, write_json

NAME = "global_vwtz76da"


def tree(path):
    require(path.is_dir() and not path.is_symlink(), "expected real source directory")
    result = {}
    for entry in sorted(path.rglob("*")):
        require(not entry.is_symlink() and (entry.is_file() or entry.is_dir()), "special archive input")
        if entry.is_file():
            result[str(entry.relative_to(path))] = digest(entry)
    return result


def main():
    original = HERE / NAME
    archive = HERE / (NAME + ".tar.gz")
    destination = ROOT / "build/v8_lidar_global_20260921/archived_receipts" / NAME
    receipt = HERE / "INITIAL_GLOBAL_ARCHIVE.json"
    require(not archive.exists() and not destination.exists() and not receipt.exists(),
            "archive, recovery directory, or receipt already exists; refuse overwrite")
    readback = read_json(HERE / "FINAL_READBACK.json")
    require(readback["status"] == "passed" and readback["normal_optimized_identical"], "read closure required")
    before, sources = tree(original), pins(SOURCES)
    for name, value in before.items():
        require(readback["input_sha256"][str((original / name).relative_to(ROOT))] == value,
                "initial global capture changed after readback")
    scratch = Path(tempfile.mkdtemp(prefix="mhgp8-initial-global-archive-", dir="/tmp"))
    second = scratch / archive.name
    extracted = scratch / "extracted"
    extracted.mkdir()
    commands, status, error, moved = [], "failed", None, False
    started = utc_stamp()
    def run(command):
        import base64
        item = dict(command=command, cwd=str(ROOT), started_utc=utc_stamp())
        result = subprocess.run(command, cwd=ROOT, env={**os.environ, "LC_ALL": "C", "TZ": "UTC"},
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        item.update(exit_code=result.returncode, stdout=result.stdout.decode(errors="replace"),
                    stderr=result.stderr.decode(errors="replace"),
                    stdout_base64=base64.b64encode(result.stdout).decode(),
                    stderr_base64=base64.b64encode(result.stderr).decode(), finished_utc=utc_stamp())
        commands.append(item)
        require(result.returncode == 0, "archive command failed")
        return item["stdout"]
    try:
        run(["tar", "--version"])
        run(["gzip", "--version"])
        for target in (archive, second):
            run(["tar", "--sort=name", "--mtime=@0", "--owner=0", "--group=0", "--numeric-owner",
                 "--format=gnu", "-czf", str(target), "-C", str(HERE), NAME])
        require(digest(archive) == digest(second), "two archive generations differ")
        gzip_header = archive.read_bytes()[:10]
        require(gzip_header[:3] == b"\x1f\x8b\x08" and gzip_header[3] == 0 and
                gzip_header[4:8] == b"\0\0\0\0", "gzip header carries timestamp or optional name")
        members = {}
        with tarfile.open(archive, "r:gz") as stream:
            for member in stream.getmembers():
                path = Path(member.name)
                require(not path.is_absolute() and ".." not in path.parts and path.parts[0] == NAME and
                        (member.isfile() or member.isdir()), "unsafe or special archive member")
                require(member.uid == member.gid == member.mtime == 0, "nondeterministic tar ownership/time")
                if member.isfile():
                    name = str(path.relative_to(NAME))
                    require(name not in members, "duplicate archive member")
                    with stream.extractfile(member) as source:
                        members[name] = hashlib.file_digest(source, "sha256").hexdigest()
        require(members == before, "archive content differs from originals")
        run(["tar", "-xzf", str(archive), "-C", str(extracted)])
        require(tree(extracted / NAME) == before and tree(original) == before,
                "fresh extraction or original content differs")
        destination.parent.mkdir(parents=True, exist_ok=True)
        require(not destination.exists(), "recovery path unexpectedly exists")
        original.rename(destination)
        moved = True
        require(tree(destination) == before and not original.exists(), "recovery move differs")
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        errors = []
        def close(label, function):
            try:
                return function()
            except Exception as cause:
                errors.append(f"{label}: {type(cause).__name__}: {cause}")
                return None
        result = dict(schema="mhgp8_initial_global_archive_v1", status=status, error=error,
            started_utc=started, finished_utc=utc_stamp(), commands=commands,
            helper_sha256=digest(Path(__file__)), readback_sha256=digest(HERE / "FINAL_READBACK.json"),
            original=str(original.relative_to(ROOT)), recovery=str(destination.relative_to(ROOT)),
            archive=str(archive.relative_to(ROOT)), archive_sha256=digest(archive) if archive.exists() else None,
            archive_bytes=archive.stat().st_size if archive.exists() else None,
            reproduction_path=str(second), reproduction_sha256=digest(second) if second.exists() else None,
            checked_extraction=str(extracted / NAME), originals_moved=moved, originals_deleted=False,
            original_file_sha256=before,
            recovery_file_sha256=close("recovery", lambda: tree(destination if moved else original)),
            source_sha256=sources, source_sha256_after=close("sources", lambda: pins(SOURCES)),
            closing_errors=errors, historical_capture_promoted=False, native_reexecutions=0)
        if errors or result["source_sha256_after"] != sources or result["recovery_file_sha256"] != before:
            result["status"] = "failed"
            result["error"] = error or "archive closure changed"
        write_json(receipt, result)
        print(json.dumps({key: result[key] for key in
                         ("status", "error", "archive", "archive_sha256", "archive_bytes", "recovery")}), flush=True)
    require(result["status"] == "passed", "archive closure failed")


if __name__ == "__main__":
    main()
