#!/usr/bin/env python3
"""Private source-closure and command recorder. No active or cloud writes."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
BASE = Path(__file__).resolve().parent
FLAGS = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread"]
BOOST = ROOT / "build/v7_boost_gate/extracted/usr/include"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(name, argv, *, cwd=ROOT, env=None, expected=0):
    target = BASE / "logs"
    target.mkdir(exist_ok=True)
    original = name
    suffix = 1
    while (target / f"{name}.json").exists():
        suffix += 1
        name = f"{original}_r{suffix}"
    start = time.monotonic()
    with (target / f"{name}.stdout").open("wb") as out, (target / f"{name}.stderr").open("wb") as err:
        proc = subprocess.run(argv, cwd=cwd, env=env, stdout=out, stderr=err, check=False)
    meta = {"argv": [str(v) for v in argv], "cwd": str(cwd), "exit_code": proc.returncode,
            "expected_exit_code": expected, "elapsed_seconds": time.monotonic() - start,
            "stdout_sha256": sha(target / f"{name}.stdout"),
            "stderr_sha256": sha(target / f"{name}.stderr")}
    (target / f"{name}.json").write_text(json.dumps(meta, indent=2) + "\n")
    print(json.dumps({"name": name, **meta}), flush=True)
    if proc.returncode != expected:
        raise SystemExit(f"{name}: unexpected exit {proc.returncode}")


def freeze():
    sources = [BASE / "probe.cpp", ROOT / "morsehgp3D_v7/tests/full_ball_tower_gate.cpp",
               ROOT / "morsehgp3D_v7/tests/full_coverage_certificate_gate.cpp"]
    paths = set()
    for source in sources:
        dep = subprocess.check_output(["g++", *FLAGS, "-I", str(ROOT), "-MM", str(source)], cwd=ROOT, text=True)
        for word in dep.replace("\\\n", " ").split()[1:]:
            path = (ROOT / word).resolve()
            if not path.is_relative_to(ROOT):
                raise SystemExit(f"outside source root: {path}")
            paths.add(path)
    before = {str(p.relative_to(ROOT)): sha(p) for p in sorted(paths)}
    for variant in ("baseline", "release", "reserve", "both"):
        target = BASE / variant
        if target.exists():
            raise SystemExit(f"refuse overwrite snapshot: {target}")
        for source in paths:
            relative = source.relative_to(ROOT)
            destination = target / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)
    after = {str(p.relative_to(ROOT)): sha(p) for p in sorted(paths)}
    if before != after:
        raise SystemExit("active source drift during freeze")
    (BASE / "sources_frozen.json").write_text(json.dumps(before, indent=2) + "\n")
    print(f"froze {len(before)} inputs in four private trees", flush=True)


def measure(variant):
    tree = BASE / variant
    source = tree / "build/v7_full_residence_20260910_r2/probe.cpp"
    binary = BASE / f"probe_{variant}_frozen"
    run(f"{variant}_probe_compile", ["g++", *FLAGS, "-O2", "-I", str(tree), str(source), "-o", str(binary)])
    run(f"{variant}_probe", [str(binary)])
    (BASE / f"{variant}_probe_binary.sha256").write_text(sha(binary) + "\n")


def gates(variant, mode):
    tree = BASE / variant
    extra = ["-O2"] if mode == "o2" else ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    env = dict(os.environ)
    if mode == "san":
        env.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    for gate in ("full_coverage_certificate_gate", "full_ball_tower_gate"):
        source = tree / f"morsehgp3D_v7/tests/{gate}.cpp"
        binary = BASE / f"{variant}_{gate}_{mode}"
        name = f"{variant}_{gate}_{mode}"
        run(name + "_compile", ["g++", *FLAGS, "-isystem", str(BOOST), *extra, str(source), "-o", str(binary)])
        run(name, [str(binary), "--selftest"], env=env)
        run(name + "_arg", [str(binary), "unexpected"], env=env, expected=2)
        (BASE / f"{name}_binary.sha256").write_text(sha(binary) + "\n")


def freeze_final_gate():
    source = ROOT / "morsehgp3D_v7/tests/full_ball_tower_gate.cpp"
    before = sha(source)
    destination = BASE / "both_final"
    shutil.copytree(BASE / "both", destination)
    shutil.copy2(source, destination / source.relative_to(ROOT))
    if before != sha(source):
        raise SystemExit("new gate drift during freeze")
    pins = {str(p.relative_to(destination)): sha(p) for p in sorted(destination.rglob("*")) if p.is_file()}
    (BASE / "both_final_sources_before.json").write_text(json.dumps(pins, indent=2) + "\n")
    print(f"froze final grouped gate {before}", flush=True)


def phase_snapshot():
    shutil.copytree(BASE / "baseline", BASE / "phase")


if __name__ == "__main__":
    if sys.argv[1:] == ["freeze"]:
        freeze()
    elif sys.argv[1:] == ["freeze-final-gate"]:
        freeze_final_gate()
    elif sys.argv[1:] == ["phase-snapshot"]:
        phase_snapshot()
    elif len(sys.argv) == 3 and sys.argv[1] == "measure":
        measure(sys.argv[2])
    elif len(sys.argv) == 4 and sys.argv[1] == "gates":
        gates(sys.argv[2], sys.argv[3])
    else:
        raise SystemExit("usage: record.py freeze | measure VARIANT | gates VARIANT o2|san")
