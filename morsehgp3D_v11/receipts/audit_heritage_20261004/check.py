#!/usr/bin/env python3
"""Portable integrity reader of retained ideas only; no native, fit or cloud."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
checks = 0


def need(ok, message):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(message)


def read_manifest(folder):
    records = {}
    for line in (folder / "SHA256SUMS").read_text().splitlines():
        digest, rel = line.split("  ", 1)
        need(len(digest) == 64 and all(c in "0123456789abcdef" for c in digest), "hash syntax")
        need(not Path(rel).is_absolute() and ".." not in Path(rel).parts, "unsafe path")
        need(rel not in records, "duplicate entry")
        records[rel] = digest
    actual = {p.relative_to(folder).as_posix() for p in folder.rglob("*")
              if p.is_file() and p != folder / "SHA256SUMS"}
    need(set(records) == actual, "manifest inventory " + folder.name)
    for rel, digest in records.items():
        need(hashlib.sha256((folder / rel).read_bytes()).hexdigest() == digest, "changed " + rel)
    return records


def main():
    records = read_manifest(ROOT)
    for name in ("q3_deferred", "q2_coupled"):
        read_manifest(ROOT / name)
    runs = json.loads((ROOT / "REPLAYS.json").read_text())["runs"]
    need(len(runs) == 4, "replay count")
    for name in ("q3_deferred", "q2_coupled"):
        pair = [r for r in runs if r["capsule"] == name]
        need(len(pair) == 2 and {r["optimized"] for r in pair} == {False, True}, "mode pair")
        need(all(r["exit_code"] == 0 and not r["stderr"] for r in pair), "replay failure")
        need(pair[0]["stdout_sha256"] == pair[1]["stdout_sha256"], "different optimized output")
        for run in pair:
            rel = run["stdout_file"]
            need(rel in records, "missing replay output")
            need(records[rel] == run["stdout_sha256"], "replay output identity")
            output = json.loads((ROOT / rel).read_text())
            need(output["checks"] == run["checks"], "replay result count")
        need(all(r["checks"] == {"q3_deferred": 2071, "q2_coupled": 633}[name]
                 for r in pair), "proof coverage")
    print(json.dumps({"status": "PASS", "integrity_checks": checks,
                      "payloads": len(records), "retained_ideas": 2,
                      "native_runs": 0, "fits": 0, "gcp_actions": 0}, sort_keys=True))


if __name__ == "__main__":
    main()
