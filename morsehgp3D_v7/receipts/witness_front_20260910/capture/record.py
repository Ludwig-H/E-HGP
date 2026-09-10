"""Fresh pinned host seam qualification. Never uses GCP or Git."""
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
TEST = Path("morsehgp3D_v7/tests/witness_front_gate.cpp")
HEADER = Path("morsehgp3D_v7/src/pipeline/witness_front.hpp")
FLAGS = ["-std=c++20","-pthread","-Wall","-Wextra","-Wpedantic","-Werror"]
MUTANTS = {"index_binding":"binding.wrong_cloud_rejected_before_query",
           "generation_reserved":"generation.reserved_before_first_query",
           "singleton_stale_work":"work.singleton_clears_per_call_only"}


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def command(name, argv, expected=0, env=None):
    logs = BASE / "logs"; logs.mkdir(exist_ok=True)
    if (logs / f"{name}.json").exists(): raise RuntimeError("refuse overwrite command: "+name)
    print(name,flush=True); start = time.monotonic()
    proc = subprocess.run(argv,cwd=ROOT,env=env,capture_output=True,check=False)
    for suffix,data in (("stdout",proc.stdout),("stderr",proc.stderr)):
        (logs / f"{name}.{suffix}").write_bytes(data)
    row = {"argv":[str(v) for v in argv],"cwd":str(ROOT),"exit_code":proc.returncode,"expected_exit_code":expected,
           "elapsed_seconds":time.monotonic()-start,
           "stdout_sha256":sha(logs / f"{name}.stdout"),"stderr_sha256":sha(logs / f"{name}.stderr"),
           "environment_overrides":{k:env[k] for k in ("ASAN_OPTIONS","UBSAN_OPTIONS")} if env else {}}
    (logs / f"{name}.json").write_text(json.dumps(row,indent=2)+"\n")
    if proc.returncode != expected: raise RuntimeError(f"{name}: code {proc.returncode}: {proc.stderr.decode(errors='replace')}")


def dependencies(path, tree):
    text = path.read_text().replace("\\\n", " ").split(":",1)[1]
    rows = {}
    for spelling in shlex.split(text):
        target = (ROOT / spelling).resolve()
        relative = target.relative_to(tree)
        rows[str(relative)] = sha(target)
    return rows


def freeze():
    command("compiler",["g++","--version"])
    command("dependencies",["g++",*FLAGS,"-MM",str(ROOT / TEST)])
    names = shlex.split((BASE / "logs/dependencies.stdout").read_text().replace("\\\n"," ").split(":",1)[1])
    pins = {}
    for name in names:
        path = (ROOT / name).resolve(); relative = path.relative_to(ROOT)
        pins[str(relative)] = sha(path)
    for kind in ["snapshot",*MUTANTS]:
        tree = BASE / kind
        if tree.exists(): raise RuntimeError("refuse overwrite tree")
        for name in pins:
            target = tree / name; target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(ROOT / name,target)
    if pins != {name:sha(ROOT/name) for name in pins}: raise RuntimeError("active source drift during freeze")
    (BASE / "source_before.json").write_text(json.dumps(pins,indent=2)+"\n")


def build(kind):
    nominal = kind in ("o2","san")
    tree = BASE / ("snapshot" if nominal else kind)
    pins = json.loads((BASE / "source_before.json").read_text())
    before = {name:sha(tree/name) for name in pins}
    (BASE / f"{kind}_sources_before.json").write_text(json.dumps(before,indent=2)+"\n")
    binary = BASE / f"gate_{kind}"
    options = ["-O1","-g","-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie"] if kind == "san" else ["-O2"]
    dep = BASE / f"{kind}.d"
    command(kind+"_compile",["g++",*FLAGS,*options,"-MMD","-MF",str(dep),str(tree/TEST),"-o",str(binary)])
    consumed = dependencies(dep,tree)
    if consumed != before: raise RuntimeError("compiled closure differs: "+kind)
    (BASE / f"{kind}_compiled_sources.json").write_text(json.dumps(consumed,indent=2)+"\n")
    env = None
    if kind == "san":
        env = dict(os.environ); env.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    command(kind+"_selftest",[str(binary),"--selftest"],0 if nominal else 1,env)
    if nominal: command(kind+"_argument",[str(binary),"--unknown"],2,env)
    elif (BASE / f"logs/{kind}_selftest.stderr").read_text() != "witness_front_gate: "+MUTANTS[kind]+"\n":
        raise RuntimeError("wrong causal mutant reason: "+kind)
    after = {name:sha(tree/name) for name in pins}
    if after != before: raise RuntimeError("source drift while compiling/running: "+kind)
    (BASE / f"{kind}_sources_after.json").write_text(json.dumps(after,indent=2)+"\n")
    (BASE / f"{kind}_binary.sha256").write_text(sha(binary)+"\n")


if __name__ == "__main__":
    if sys.argv[1:] == ["freeze"]: freeze()
    elif len(sys.argv)==3 and sys.argv[1]=="build" and sys.argv[2] in ("o2","san",*MUTANTS): build(sys.argv[2])
    else: raise SystemExit("usage: record.py freeze | build o2|san|index_binding|generation_reserved|singleton_stale_work")
