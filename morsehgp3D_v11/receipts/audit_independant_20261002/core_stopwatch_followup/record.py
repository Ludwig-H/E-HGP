#!/usr/bin/env python3
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
LIVE = HERE.parents[2]
CHECKOUT = LIVE.parent
FILES = ["src/core/ledger.hpp", "src/core/ledger.cpp", "src/core/status.hpp", "src/core/types.hpp", "src/core/reasons.def", "tests/core/ledger_test.cpp"]

def digest(data):
    return hashlib.sha256(data).hexdigest()

def write_json(name, obj):
    (HERE / name).write_text(json.dumps(obj, indent=2, ensure_ascii=False) + "\n")

def snapshot(copy=False):
    entries = []
    for name in FILES:
        data = (LIVE / name).read_bytes()
        if copy:
            dest = HERE / "sources" / name
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
        entries.append({"path": name, "bytes": len(data), "sha256": digest(data)})
    head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=CHECKOUT, capture_output=True, text=True, check=True).stdout.strip()
    status = subprocess.run(["git", "status", "--short", "--"] + [str(LIVE.relative_to(CHECKOUT) / n) for n in FILES], cwd=CHECKOUT, capture_output=True, text=True, check=True).stdout
    return {"time_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "head": head, "git_status_named_scope": status, "sources": entries}

def run(name, argv, cwd=None):
    result = subprocess.run(argv, cwd=cwd, capture_output=True)
    (HERE / (name + ".stdout.txt")).write_bytes(result.stdout)
    (HERE / (name + ".stderr.txt")).write_bytes(result.stderr)
    entry = {"argv": [str(a) for a in argv], "cwd": str(cwd) if cwd else None, "exit_code": result.returncode,
             "stdout_sha256": digest(result.stdout), "stderr_sha256": digest(result.stderr)}
    write_json(name + ".json", entry)
    return result

before = snapshot(copy=True)
write_json("SOURCE_BEFORE.json", before)
scratch = Path(tempfile.mkdtemp(prefix="mhgp11-stopwatch-independent-20261002-"))
run("compiler_version", ["g++", "--version"])
flags = ["-std=c++20", "-O1", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-DMHGP11_COORD_BITS=18", "-I" + str(HERE / "sources/src")]
units = [str(HERE / "probe.cpp"), str(HERE / "sources/src/core/ledger.cpp")]
dependencies = run("dependencies", ["g++"] + flags + ["-MM"] + units)
compile_result = run("compile", ["g++"] + flags + units + ["-o", str(scratch / "probe")])
if compile_result.returncode == 0:
    native = run("native", [str(scratch / "probe")])
    write_json("BINARY.json", {"path": str(scratch / "probe"), "sha256": digest((scratch / "probe").read_bytes())})
else:
    native = None
after = snapshot()
after["stable"] = before["sources"] == after["sources"]
after["snapshot_intact"] = all(digest((HERE / "sources" / item["path"]).read_bytes()) == item["sha256"] for item in before["sources"])
write_json("SOURCE_AFTER.json", after)
write_json("EXECUTION.json", {"normal_only": True, "compiled": compile_result.returncode == 0, "native_exit_code": native.returncode if native else None, "dependencies_exit_code": dependencies.returncode})
print(json.dumps({"compile_exit_code": compile_result.returncode, "native_exit_code": native.returncode if native else None, "sources_stable": after["stable"], "snapshot_intact": after["snapshot_intact"]}))
