#!/usr/bin/env python3
"""Close the independent private proof; preserve final normal/-O captures."""
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).absolute().parent
FIXED = ("README.md", "check.py", "read.py", "record.py")
SHARED = (
    "/workspaces/E-HGP/build/v10-verrou-points/revision_cible/majorites_continues/MEMO.md",
    "/workspaces/E-HGP/build/v10-verrou-points/revision_cible/majorites_continues/mmc.py",
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(ok, what):
    if not ok:
        raise RuntimeError(what)


def main():
    require(not (ROOT / "manifest.json").exists(), "already closed; do not overwrite")
    files_before = {name: digest(ROOT / name) for name in FIXED}
    shared_before = {name: digest(pathlib.Path(name)) for name in SHARED}
    captures = []
    for optimized, name in ((False, "capture_normal.json"), (True, "capture_optimized.json")):
        command = [sys.executable, "-B"] + (["-O"] if optimized else []) + [str(ROOT / "check.py")]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=30)
        capture = {"optimized": optimized, "command": command, "returncode": result.returncode,
                   "stdout": result.stdout, "stderr": result.stderr,
                   "files_before": files_before, "shared_before": shared_before,
                   "files_after": {n: digest(ROOT / n) for n in FIXED},
                   "shared_after": {n: digest(pathlib.Path(n)) for n in SHARED}}
        (ROOT / name).write_text(json.dumps(capture, sort_keys=True, indent=2) + "\n")
        require(result.returncode == 0 and not result.stderr, "run failed (capture preserved)")
        require(capture["files_before"] == capture["files_after"], "private source changed")
        require(capture["shared_before"] == capture["shared_after"], "shared source changed")
        captures.append(capture)
    require(captures[0]["stdout"] == captures[1]["stdout"], "normal/-O differ")
    names = sorted(FIXED + ("capture_normal.json", "capture_optimized.json"))
    manifest = {"format": "meb-shell-euler-private-v2", "files": {n: digest(ROOT / n) for n in names}}
    (ROOT / "manifest.json").write_text(json.dumps(manifest, sort_keys=True, indent=2) + "\n")
    print(digest(ROOT / "manifest.json"))


if __name__ == "__main__":
    main()
