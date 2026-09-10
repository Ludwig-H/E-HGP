#!/usr/bin/env python3
"""Freeze/build tiny observer gates only. This script NEVER runs 50k or GCP."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import time

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
BOOST = ROOT / "build/v7_boost_gate/extracted/usr/include"
FLAGS = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread", "-isystem", str(BOOST)]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def command(name, argv, expected=0, env=None):
    logs = BASE / "logs"
    logs.mkdir(exist_ok=True)
    if (logs / f"{name}.json").exists():
        raise RuntimeError(f"refuse overwriting command {name}")
    print(name, flush=True)
    start = time.monotonic()
    proc = subprocess.run(argv, cwd=ROOT, env=env, capture_output=True, check=False)
    (logs / f"{name}.stdout").write_bytes(proc.stdout)
    (logs / f"{name}.stderr").write_bytes(proc.stderr)
    row = {"argv": [str(v) for v in argv], "cwd": str(ROOT), "exit_code": proc.returncode,
           "expected_exit_code": expected, "seconds": time.monotonic()-start,
           "stdout_sha256": sha(logs / f"{name}.stdout"), "stderr_sha256": sha(logs / f"{name}.stderr"),
           "environment_overrides": {k: env[k] for k in ("ASAN_OPTIONS", "UBSAN_OPTIONS")} if env else {}}
    (logs / f"{name}.json").write_text(json.dumps(row, indent=2)+"\n")
    if proc.returncode != expected:
        raise RuntimeError(f"{name}: exit {proc.returncode}: {proc.stderr.decode(errors='replace')}")


def freeze():
    source = BASE / "observer_gate.cpp"
    paths = set()
    for macro in ([], ["-DMHGP7_NAMED_BLOCK_CUDA=1"]):
        out = subprocess.check_output(["g++",*FLAGS,*macro,"-I",str(ROOT),"-MM",str(source)],cwd=ROOT,text=True)
        for word in shlex.split(out.replace("\\\n", " ").split(":", 1)[1]):
            path = (ROOT / word).resolve()
            if not path.is_relative_to(ROOT):
                raise RuntimeError(f"nonproject dependency {path}")
            paths.add(path)
    pins = {str(path.relative_to(ROOT)): sha(path) for path in sorted(paths)}
    for variant in ("baseline", "overlay"):
        tree = BASE / variant
        if tree.exists():
            raise RuntimeError(f"refuse overwrite {tree}")
        for path in paths:
            target = tree / path.relative_to(ROOT)
            target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(path,target)
    if pins != {str(path.relative_to(ROOT)): sha(path) for path in sorted(paths)}:
        raise RuntimeError("source drift during freeze")
    (BASE / "source_before.json").write_text(json.dumps(pins,indent=2)+"\n")
    inputs = ["morsehgp3D_v7/audits/receipts_raccord_ancres_20260910/README.md",
              "morsehgp3D_v7/audits/receipts_plateaux_full_20260906/GLOBAL_PARENTS.md",
              "morsehgp3D_v7/audits/receipts_plateaux_full_20260906/LOCAL_DIAGNOSTICS.md",
              "morsehgp3D_v7/audits/receipts_plateaux_full_20260906/real_parent_certificates_normal.json",
              "morsehgp3D_v7/audits/receipts_plateaux_full_20260906/real_shell_census_normal.json",
              "morsehgp3D_v7/receipts/full_extra_shell_50000_20260906/run_r3/n50000_k10.stderr"]
    authorities = {}
    for name in inputs:
        source = ROOT / name; target = BASE / "authorities" / name
        target.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(source,target)
        authorities[name] = sha(source)
    (BASE / "authority_pins.json").write_text(json.dumps(authorities,indent=2)+"\n")
    print(f"frozen {len(pins)} project dependencies, CPU/CUDA union; no execution",flush=True)


def build(mode, revision=""):
    tree = BASE / "overlay"
    source = tree / "build/v7_named_blocks_gate_20260910_r1/observer_gate.cpp"
    tag = mode + revision
    binary = BASE / f"observer_{tag}"
    options = ["-O2"] if mode == "o2" else ["-O1","-g","-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie"]
    command(f"compile_{tag}",["g++",*FLAGS,*options,"-I",str(tree),str(source),"-o",str(binary)])
    env = None
    if mode == "san":
        env = dict(os.environ); env.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    command(f"selftest_{tag}",[str(binary),"--selftest"],env=env)
    command(f"describe_{tag}",[str(binary),"--describe"],env=env)
    command(f"argument_{tag}",[str(binary),"--not-a-mode"],2,env)
    (BASE / f"binary_{tag}.sha256").write_text(sha(binary)+"\n")


if __name__ == "__main__":
    if sys.argv[1:] == ["freeze"]: freeze()
    elif len(sys.argv) == 3 and sys.argv[1] == "build" and sys.argv[2] in ("o2","san"): build(sys.argv[2])
    elif len(sys.argv) == 4 and sys.argv[1] == "build" and sys.argv[2] in ("o2","san") and sys.argv[3] == "_r2": build(sys.argv[2],sys.argv[3])
    else: raise SystemExit("usage: prepare.py freeze | build o2|san")
