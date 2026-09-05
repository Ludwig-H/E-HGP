#!/usr/bin/env python3
"""Verify v7 SHA256SUMS receipts from Git's index, not untracked local files."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import PurePosixPath
import re
import subprocess
import sys
from collections.abc import Callable

PREFIX = "morsehgp3D_v7/receipts/"


def require(value: bool, reason: str) -> None:
    if not value:
        raise ValueError(reason)


def verify_manifest(manifest: str, data: bytes,
                    fetch: Callable[[str], bytes]) -> int:
    parent = str(PurePosixPath(manifest).parent) + "/"
    require(manifest.startswith(PREFIX) and manifest.endswith("/SHA256SUMS"),
            "manifest outside the v7 receipt scope")
    seen: set[str] = set()
    for line in data.decode("utf-8").splitlines():
        match = re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9_./-]+)", line)
        require(match is not None, f"{manifest}: malformed checksum line")
        expected, written_path = match.groups()
        parts = PurePosixPath(written_path).parts
        require(not written_path.startswith("/") and ".." not in parts
                and str(PurePosixPath(written_path)) == written_path,
                f"{manifest}: path escapes its receipt")
        # Both established repository-root paths and conventional receipt-local
        # paths resolve to indexed blobs; never fall back to the working tree.
        path = written_path if written_path.startswith(PREFIX) else parent + written_path
        require(path.startswith(parent), f"{manifest}: path escapes its receipt")
        require(path != manifest and path not in seen,
                f"{manifest}: repeated or recursive entry")
        seen.add(path)
        require(hashlib.sha256(fetch(path)).hexdigest() == expected,
                f"{path}: indexed bytes do not match the sealed receipt")
    require(bool(seen), f"{manifest}: empty receipt")
    return len(seen)


def git(*args: str) -> bytes:
    result = subprocess.run(["git", *args], check=False, capture_output=True)
    require(result.returncode == 0, "Git index is unavailable")
    return result.stdout


def check_index() -> tuple[int, int]:
    entries: dict[str, str] = {}
    for record in git("ls-files", "--stage", "-z", "--", PREFIX).split(b"\0"):
        if not record:
            continue
        metadata, encoded_path = record.split(b"\t", 1)
        mode, oid, stage = metadata.decode("ascii").split()
        path = encoded_path.decode("utf-8")
        require(stage == "0" and mode in {"100644", "100755"}
                and path not in entries, f"{path}: nonregular or conflicted index entry")
        entries[path] = oid

    def fetch(path: str) -> bytes:
        require(path in entries, f"{path}: sealed file is missing from Git's index")
        return git("cat-file", "blob", entries[path])

    manifests = sorted(path for path in entries if path.endswith("/SHA256SUMS"))
    require(bool(manifests), "no indexed v7 SHA256SUMS receipt")
    return len(manifests), sum(verify_manifest(path, fetch(path), fetch) for path in manifests)


def selftest() -> None:
    manifest = PREFIX + "fixture/SHA256SUMS"
    path = PREFIX + "fixture/result.log"
    payload = b"qualified\n"
    digest = hashlib.sha256(payload).hexdigest()
    line = f"{digest}  {path}\n".encode()

    def fetch(name: str) -> bytes:
        require(name == path, "missing indexed fixture")
        return payload

    require(verify_manifest(manifest, line, fetch) == 1, "positive fixture")
    local_line = f"{digest}  result.log\n".encode()
    require(verify_manifest(manifest, local_line, fetch) == 1, "receipt-local fixture")
    rejected = 0
    for bad in (b"", line + line, b"malformed\n",
                f"{'0' * 64}  {path}\n".encode(),
                f"{digest}  {PREFIX}fixture/missing.log\n".encode(),
                f"{digest}  {PREFIX}fixture/../escape.log\n".encode(),
                f"{digest}  {PREFIX}elsewhere/result.log\n".encode(),
                line + local_line, f"{digest}  /absolute/result.log\n".encode()):
        try:
            verify_manifest(manifest, bad, fetch)
        except ValueError:
            rejected += 1
    require(rejected == 9, "a publication counter-fixture survived")
    print("receipt_publication_selftest positive=2 rejected=9 PASS")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    try:
        if args.selftest:
            selftest()
        else:
            manifests, files = check_index()
            print(f"Validated {files} indexed files in {manifests} v7 SHA256SUMS receipts.")
        return 0
    except (ValueError, OSError, UnicodeError) as error:
        print(f"Receipt publication failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
