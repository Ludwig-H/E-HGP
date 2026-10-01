#!/usr/bin/env python3
"""PREPARED, NOT RUN. After main review only; two <=30s pure-Python replays. No native engine."""
import argparse
import datetime
import hashlib
import importlib.util
import json
import os
import pathlib
import re
import stat
import subprocess
import sys
import time

sys.dont_write_bytecode = True
BASE = pathlib.Path(__file__).absolute().parent
FILES = {"README.txt", "origin.json", "protocol.json", "probe.py", "read.py", "record.py",
         "original/fullk.py", "original/frontier_core.py"}


def require(cond, message):
    if not cond:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="microseconds").replace("+00:00", "Z")

def safe_absolute(path, must_exist=True, directory=False):
    require(path.is_absolute() and str(path) == str(pathlib.Path(os.path.normpath(str(path)))),
            "canonical absolute path")
    for part in [path] + list(path.parents):
        try:
            mode = part.lstat().st_mode
        except FileNotFoundError:
            require(part == path and not must_exist, "missing parent/path")
            continue
        require(not stat.S_ISLNK(mode), "symlink root/parent/payload: " + str(part))
        if part != path or directory:
            require(stat.S_ISDIR(mode), "directory required: " + str(part))
        elif must_exist:
            require(stat.S_ISREG(mode), "regular file required: " + str(part))
    return path

def verify_tree(base, external_pin, expected):
    safe_absolute(base, must_exist=True, directory=True)
    require(type(external_pin) is str and re.fullmatch("[0-9a-f]{64}", external_pin), "external manifest SHA")
    manifest = base / "SHA256SUMS"
    safe_absolute(manifest, must_exist=True, directory=False)
    raw = manifest.read_bytes()
    require(sha(raw) == external_pin, "external manifest SHA BEFORE parsing/import")
    listed = {}
    for line in raw.decode("ascii").splitlines():
        digest, rel = line.split("  ", 1)
        p = pathlib.PurePosixPath(rel)
        require(re.fullmatch("[0-9a-f]{64}", digest) and rel and not p.is_absolute()
                and ".." not in p.parts and str(p) == rel and rel not in listed, "manifest syntax")
        listed[rel] = digest
    require(set(listed) == expected, "manifest inventory")
    actual = set()
    for path in base.rglob("*"):
        mode = path.lstat().st_mode
        require(not stat.S_ISLNK(mode), "payload symlink")
        require(stat.S_ISDIR(mode) or stat.S_ISREG(mode), "nonregular payload")
        if stat.S_ISREG(mode):
            actual.add(path.relative_to(base).as_posix())
    require(actual == expected | {"SHA256SUMS"}, "actual inventory missing/extra")
    for rel, digest in listed.items():
        payload = base / rel
        safe_absolute(payload, must_exist=True, directory=False)
        require(sha(payload.read_bytes()) == digest, "payload SHA: " + rel)
    return listed

def bootstrap(pin):
    verify_tree(BASE, pin, FILES)  # Hash-first BEFORE local reader import.
    spec = importlib.util.spec_from_file_location("closure_verified_reader_r2", BASE / "read.py")
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    listed, protocol_sha = reader.verify_package(pin)
    return reader, listed, protocol_sha


def close_capture(out, runs, source_pin, origin, protocol_sha):
    receipt = dict(schema="ancestor_closure_1d_run_receipt_v2", source_manifest_sha=source_pin,
                   origin=origin, protocol=dict(path=str(BASE / "protocol.json"), sha256=protocol_sha),
                   run_limit_seconds=30, runs=runs)
    (out / "run_receipt.json").write_text(json.dumps(receipt, indent=1, sort_keys=True) + "\n")
    names = ["normal.json", "optimized.json", "run_receipt.json"]
    existing = [name for name in names if (out / name).exists()]
    manifest = "".join(sha((out / name).read_bytes()) + "  " + name + "\n" for name in existing)
    (out / "SHA256SUMS").write_text(manifest)
    return sha(manifest.encode())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source-manifest-sha", required=True)
    ap.add_argument("--out", required=True, help="fresh canonical absolute path outside source package")
    a = ap.parse_args()
    reader, listed, protocol_sha = bootstrap(a.source_manifest_sha)
    out = reader.absolute(a.out)
    safe_absolute(out, must_exist=False, directory=True)
    require(not out.exists() and not out.is_relative_to(BASE), "output must be fresh/outside package")
    execution = reader.python_identity()
    origin = dict(package_root=str(BASE),
                  source_files={name: dict(path=str(BASE / name), sha256=listed[name]) for name in sorted(FILES)},
                  manifest=dict(path=str(BASE / "SHA256SUMS"), sha256=a.source_manifest_sha),
                  executable=execution)
    out.mkdir(parents=False)
    runs = []
    for mode, flags in (("normal", ["-B"]), ("optimized", ["-B", "-O"])):
        reader.verify_package(a.source_manifest_sha)
        require(reader.python_identity() == execution, "Python before run")
        argv = [execution["path"]] + flags + [str(BASE / "probe.py"), "--protocol", str(BASE / "protocol.json")]
        start = utc()
        tick = time.monotonic()
        timed_out = False
        try:
            proc = subprocess.run(argv, cwd=str(BASE), capture_output=True, timeout=30)
            code, stdout, stderr = proc.returncode, proc.stdout, proc.stderr
        except subprocess.TimeoutExpired as error:
            timed_out = True
            code, stdout, stderr = 124, error.stdout or b"", error.stderr or b""
        except OSError as error:
            code, stdout, stderr = 125, b"", ("launch_error: " + repr(error)).encode()
        duration = time.monotonic() - tick
        end = utc()
        (out / (mode + ".json")).write_bytes(stdout)
        runs.append(dict(mode=mode, argv=argv, cwd=str(BASE), timeout_seconds=30, timed_out=timed_out,
                         code=code, start_utc=start, end_utc=end, duration_seconds=duration,
                         stdout_file=mode + ".json", stdout_sha256=sha(stdout),
                         stderr=stderr.decode("utf-8", errors="replace")))
        # Preserve the FULL stdout and diagnostics BEFORE any post-run integrity check can fail.
        close_capture(out, runs, a.source_manifest_sha, origin, protocol_sha)
        reader.verify_package(a.source_manifest_sha)
        require(reader.python_identity() == execution, "Python after run")
    capture_pin = close_capture(out, runs, a.source_manifest_sha, origin, protocol_sha)
    accepted = True
    try:
        reader.verify_capture(out, capture_pin, a.source_manifest_sha, listed, protocol_sha)
    except Exception as error:
        accepted = False
        print("capture rejected: " + str(error), file=sys.stderr)
    print(json.dumps(dict(capture=str(out), capture_manifest_sha=capture_pin, accepted=accepted), sort_keys=True))
    return 0 if accepted else 1


if __name__ == "__main__":
    sys.exit(main())
