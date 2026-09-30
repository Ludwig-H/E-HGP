"""One-shot private receipt collector; never called by the read-only reader."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from datetime import datetime, timezone
import check

ROOT = Path(__file__).resolve().parent
SOURCE_ROOT = Path("/workspaces/E-HGP/build/v10-verrou-points/masses_selection")

def clock():
    return datetime.now(timezone.utc).isoformat()

def pins():
    out = {}
    for name, pin in check.PINS.items():
        rel = name if name in ("dev_scenes.py", "MEMO_MASSES_SELECTION_20260930.md") else "lib/" + name
        p = SOURCE_ROOT / rel
        got = check.sha(p)
        check.require(got == pin, "live source changed: " + rel)
        out[rel] = got
    return out

def main():
    check.require(not (ROOT / "manifest.json").exists() and
                  (not (ROOT / "captures").exists() or not list((ROOT / "captures").iterdir())), "one-shot collector")
    before = pins()
    (ROOT / "captures").mkdir(exist_ok=True)
    payloads = []
    for name, options in (("normal", ["-I", "-B"]), ("optimized", ["-I", "-B", "-O"])):
        cmd = [sys.executable] + options + [str(ROOT / "check.py"), "--archive", str(ROOT)]
        start = clock()
        run = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        finish = clock()
        rec = {"argv": cmd, "start_utc": start, "end_utc": finish, "exit_code": run.returncode,
               "stdout": run.stdout, "stderr": run.stderr, "source_before": before, "source_after": pins()}
        (ROOT / "captures" / (name + ".json")).write_text(json.dumps(rec, sort_keys=True, indent=2) + "\n")
        check.require(run.returncode == 0 and run.stderr == "", "capture failed, evidence retained")
        payloads.append(run.stdout)
    check.require(payloads[0] == payloads[1] and pins() == before, "normal/optimized or closure disagreement")
    paths = sorted(p.relative_to(ROOT).as_posix() for p in ROOT.rglob("*") if p.is_file())
    check.require(len(paths) == 15, "closed payload inventory")
    entries = {rel: {"sha256": check.sha(ROOT / rel), "bytes": (ROOT / rel).stat().st_size} for rel in paths}
    manifest = {"schema": 1, "scope": "bounded_exact_selection_audit_no_native",
                "source_pins": check.PINS, "files": entries}
    (ROOT / "manifest.json").write_text(json.dumps(manifest, sort_keys=True, indent=2) + "\n")
    print("manifest_sha256", check.sha(ROOT / "manifest.json"))
    print("payload_files", len(paths))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
