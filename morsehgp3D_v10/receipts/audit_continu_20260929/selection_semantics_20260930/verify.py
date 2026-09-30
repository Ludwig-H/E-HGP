"""Read-only hash-first reader; external manifest SHA is mandatory."""
import argparse
import hashlib
import json
import re
import stat
import subprocess
import sys
from pathlib import Path

PAYLOADS = {
    "README.md", "PROTOCOL.md", "check.py", "capture.py", "verify.py", "preflight_first_order.json",
    "preflight_ast_guard.json", "preflight_guard_check.py",
    "captures/normal.json", "captures/optimized.json",
    "snapshots/selection.py", "snapshots/dev_scenes.py", "snapshots/participation.py",
    "snapshots/scale.py", "snapshots/MEMO_MASSES_SELECTION_20260930.md",
}

def need(ok, why):
    if not ok:
        raise ValueError(why)

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def unique(pairs):
    d = {}
    for k, v in pairs:
        need(k not in d, "duplicate JSON key")
        d[k] = v
    return d

def closed_hashes(root, manifest):
    need(not root.is_symlink() and root.is_dir(), "archive root directory")
    dirs = {"captures", "snapshots"}
    seen = set()
    for p in root.rglob("*"):
        rel = p.relative_to(root).as_posix()
        mode = p.lstat().st_mode
        need(not stat.S_ISLNK(mode), "symlink refused: " + rel)
        if stat.S_ISDIR(mode):
            need(rel in dirs, "unlisted directory: " + rel)
        else:
            need(stat.S_ISREG(mode), "nonregular archive member")
            seen.add(rel)
    need(seen == PAYLOADS | {"manifest.json"}, "closed inventory")
    need(set(manifest) == {"schema", "scope", "source_pins", "files"} and manifest["schema"] == 1
         and manifest["scope"] == "bounded_exact_selection_audit_no_native", "manifest envelope")
    entries = manifest["files"]
    need(isinstance(entries, dict) and set(entries) == PAYLOADS, "manifest payload inventory")
    actual = {}
    for rel, entry in entries.items():
        need(set(entry) == {"sha256", "bytes"} and isinstance(entry["bytes"], int)
             and not isinstance(entry["bytes"], bool) and entry["bytes"] >= 0
             and isinstance(entry["sha256"], str) and re.fullmatch("[0-9a-f]{64}", entry["sha256"]), "entry domain")
        p = root / rel
        actual[rel] = sha(p)
        need(actual[rel] == entry["sha256"] and p.stat().st_size == entry["bytes"], "payload hash: " + rel)
    return actual

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--archive", required=True, type=Path)
    ap.add_argument("--manifest-sha256", required=True)
    a = ap.parse_args()
    root = a.archive
    need(re.fullmatch("[0-9a-f]{64}", a.manifest_sha256) is not None, "external SHA domain")
    m = root / "manifest.json"
    need(not root.is_symlink() and not m.is_symlink() and m.is_file(), "regular manifest and root")
    need(sha(m) == a.manifest_sha256, "external manifest SHA before JSON or source loading")
    manifest = json.loads(m.read_text(), object_pairs_hook=unique)
    before = closed_hashes(root, manifest)
    records = {}
    for name, opt in (("normal", []), ("optimized", ["-O"])):
        rec = json.loads((root / "captures" / (name + ".json")).read_text(), object_pairs_hook=unique)
        need(rec["exit_code"] == 0 and rec["stderr"] == "" and rec["source_before"] == rec["source_after"],
             "terminal capture")
        output = json.loads(rec["stdout"], object_pairs_hook=unique)
        need(output["pins"] == manifest["source_pins"], "captured source identity")
        cmd = [sys.executable, "-I", "-B"] + opt + [str(root / "check.py"), "--archive", str(root)]
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        need(r.returncode == 0 and r.stderr == "" and r.stdout == rec["stdout"], "bounded replay: " + name)
        records[name] = rec
    need(records["normal"]["stdout"] == records["optimized"]["stdout"], "normal/optimized identity")
    pre = json.loads((root / "preflight_first_order.json").read_text(), object_pairs_hook=unique)
    need(pre["exit_code"] == 1 and pre["diagnosis"] == "incorrect_expected_traversal_order"
         and pre["exact_utc"] is None and pre["expected_order"] == [3, 1, 0]
         and pre["observed_order"] == [3, 0, 1], "historical first failure retained, not a product failure")
    guard = json.loads((root / "preflight_ast_guard.json").read_text(), object_pairs_hook=unique)
    need(guard["exit_code"] == 1 and guard["stdout"] == "" and
         "ValueError: real hard-method loop" in guard["stderr"] and
         guard["source_before"] == guard["source_after"], "historical AST guard failure retained")
    need(closed_hashes(root, manifest) == before and sha(m) == a.manifest_sha256, "after archive hashes")
    print("selection_semantics_audit_ok payloads=15 replays=2 normal_O_identical=1")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, KeyError, OSError, subprocess.TimeoutExpired) as e:
        print("REFUS", str(e), file=sys.stderr)
        raise SystemExit(2)
