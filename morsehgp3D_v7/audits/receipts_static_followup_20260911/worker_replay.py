"""Replay the pinned worker-failure SAN binary; write only a fresh audit receipt."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
BASE = REPO / "morsehgp3D_v7/receipts/static_worker_failure_20260911"
MANIFEST_SHA = "3afcd1f8b24aa5689f3ef633cf181aa422be4a8267b6a88a089741eead7b9f8a"
BINARY_SHA = "70379dd9dfeb3d7643debca660eb2239ecc9f1d7206222bfee27258113ce20cf"


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--name", required=True)
    args = parser.parse_args()
    need(args.name.isidentifier(), "capture name")
    out = HERE / (args.name + ".json")
    need(not out.exists(), "create-only capture")
    need(sha(BASE / "MANIFEST.json") == MANIFEST_SHA, "constructor manifest")
    manifest = json.loads((BASE / "MANIFEST.json").read_text())
    for name, expected in manifest["files"].items():
        need(sha(BASE / name) == expected, "constructor bytes: " + name)
    mapping = json.loads((BASE / "storage_map.json").read_text())

    def raw(name: str) -> bytes:
        row = mapping[name]
        data = (BASE / row["physical"]).read_bytes()
        need(hashlib.sha256(data).hexdigest() == row["sha256"], "logical source")
        return data

    previous = json.loads(raw("san/commands.json"))
    need(previous[0]["exit_code"] == 0 and "-fsanitize=address,undefined" in previous[0]["argv"],
         "historical sanitizer compile")
    argv = previous[1]["argv"]
    binary = Path(argv[0])
    need(binary.is_file() and sha(binary) == BINARY_SHA == raw("san/binary.sha256").decode().strip(),
         "same historical binary")
    tracked = {str(HERE / "worker_replay.py"): sha(HERE / "worker_replay.py"),
               str(BASE / "MANIFEST.json"): MANIFEST_SHA, str(binary): BINARY_SHA}
    for path, name in zip(argv[1:], ("fixtures.txt", "expected.txt")):
        expected = hashlib.sha256(raw("san/" + name)).hexdigest()
        need(sha(Path(path)) == expected, "same historical input")
        tracked[path] = expected
    # New before/after observations; never backfill the failed historical run.
    source_base = binary.parent / "source"
    for name, expected in json.loads(raw("san/sources_before.json")).items():
        path = source_base / name
        need(sha(path) == expected, "current source matches pinned compile source")
        tracked[str(path)] = expected
    need((binary.parent / "driver.cpp").read_bytes() == raw("san/driver.cpp"), "same driver")
    tracked[str(binary.parent / "driver.cpp")] = sha(binary.parent / "driver.cpp")
    overlay = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
               "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
    receipt = dict(schema="mhgp7-independent-worker-SAN-replay-v1", status="running",
                   authority="private_instrumented_static_resolver", public_status="not_claimed",
                   gcp_used=False, constructor_manifest_sha256=MANIFEST_SHA,
                   binary_sha256=BINARY_SHA, argv=argv, cwd=str(REPO),
                   environment_overlay=overlay, before=tracked, started_ns=time.time_ns())

    def save() -> None:
        out.write_text(json.dumps(receipt, indent=2) + "\n")

    save()
    try:
        done = subprocess.run(argv, cwd=REPO, env={**os.environ, **overlay},
                              capture_output=True, timeout=60)
        receipt.update(exit_code=done.returncode, stdout=done.stdout.decode(), stderr=done.stderr.decode())
        need(done.returncode == 0 and not done.stderr, "SAN replay refused")
        rows = [json.loads(line) for line in done.stdout.splitlines()]
        need(len(rows) == 2 and rows[0]["status"] == "passed_post_admission_failure" and
             rows[0]["failed_runs"] == 2 and rows[0]["retained_MEB_calls"] >= 2, "paid worker failures")
        need(rows[1]["status"] == "passed" and rows[1]["rows"] == 2524 and rows[1]["vertical"] == 1506,
             "nominal reuse after failure")
        receipt["status"] = "passed"
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        receipt["status"] = "failed"
        receipt["error"] = str(error)
    finally:
        receipt["ended_ns"] = time.time_ns()
        receipt["after"] = {path: sha(Path(path)) for path in tracked}
        receipt["stable"] = receipt["before"] == receipt["after"]
        if not receipt["stable"]:
            receipt["status"] = "failed"
        save()
    print(json.dumps({k: receipt[k] for k in ("status", "binary_sha256", "stable")}))
    return 0 if receipt["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
