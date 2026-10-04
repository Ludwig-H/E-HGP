#!/usr/bin/env python3
"""Portable integrity reader; no product, Git, build, fit or cloud execution."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
checks = 0


def need(ok, reason):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(reason)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def manifest(folder):
    records = {}
    for line in (folder / "SHA256SUMS").read_text().splitlines():
        sha, rel = line.split("  ", 1)
        need(len(sha) == 64 and all(c in "0123456789abcdef" for c in sha), "bad hash")
        need(not Path(rel).is_absolute() and ".." not in Path(rel).parts, "unsafe path")
        need(rel not in records, "duplicate path")
        records[rel] = sha
    files = {p.relative_to(folder).as_posix() for p in folder.rglob("*")
             if p.is_file() and p != folder / "SHA256SUMS"}
    need(files == set(records), "manifest inventory " + folder.name)
    for rel, sha in records.items():
        need(digest(folder / rel) == sha, "changed payload " + rel)
    return records


def main():
    records = manifest(ROOT)
    ledger = json.loads((ROOT / "LEDGER.json").read_text())
    need(set(ledger["files"]) == set(records) - {"LEDGER.json"}, "ledger inventory")
    for rel, sha in ledger["files"].items():
        need(records[rel] == sha, "ledger hash " + rel)
    groups = ("foundations", "parallel", "geometry", "math", "performance",
              "performance_precision", "source_pins")
    for group in groups:
        manifest(ROOT / group)
    review = json.loads((ROOT / "CODE_REVIEW.json").read_text())
    rows = review["files"]
    need(review["source_files"] == len(rows) == 101, "source coverage")
    need(len({r["path"] for r in rows}) == 101, "source uniqueness")
    need(review["source_delta_e02_latest"] == [], "native source drift")
    for row in rows:
        need(row["path"].startswith("morsehgp3D_v11/src/"), "source scope")
        need(digest(ROOT / row["snapshot"]) == row["sha256"], "source copy " + row["path"])
    runs = json.loads((ROOT / "RUNS.json").read_text())["runs"]
    need(len(runs) == 12, "replay inventory")
    for script in {r["script"] for r in runs}:
        pair = [r for r in runs if r["script"] == script]
        need(len(pair) == 2 and {r["optimized"] for r in pair} == {False, True}, "mode pair")
        need(all(r["exit_code"] == 0 and not r["stderr"] for r in pair), "replay failure")
        need(pair[0]["stdout_sha256"] == pair[1]["stdout_sha256"], "optimized output")
    late = json.loads((ROOT / "LATEST_READ.json").read_text())
    need(late["input_hashes_before_after_equal"], "metadata drift")
    need(digest(ROOT / late["reader_snapshot"]) == late["reader_sha256"], "latest reader source")
    need(len(late["runs"]) == 2 and all(r["exit_code"] == 0 for r in late["runs"]), "latest reads")
    need(late["runs"][0]["stdout"] == late["runs"][1]["stdout"], "latest optimized output")
    print(json.dumps({"status": "PASS", "integrity_checks": checks,
                      "files": len(records), "source_files": 101,
                      "groups": len(groups), "native_runs": 0, "fits": 0,
                      "gcp_actions": 0}, sort_keys=True))


if __name__ == "__main__":
    main()
