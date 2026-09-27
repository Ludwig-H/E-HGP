#!/usr/bin/env python3
"""Fresh local capture and strict LIVE readback; no cloud or product mutation."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time


HERE = Path(__file__).resolve().parent
V9 = HERE.parent.parent
REPO = V9.parent
SCHEMA = "mhgp9_full_batch_encoder_capture_v1"
SAN_ENV = {
    "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
    "LSAN_OPTIONS": "exitcode=23",
}


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def pins():
    paths = sorted((V9 / "src/tower").rglob("*.hpp"))
    paths += [V9 / "src/common/raw_vector.hpp", HERE / "encode.hpp", HERE / "gate.cpp", Path(__file__).resolve()]
    return {str(p): sha(p) for p in paths}


def recipe(build):
    cc = {"release": shutil.which("g++"), "sanitize": shutil.which("clang++")}
    need(all(cc.values()), "missing compiler")
    entries = []
    base = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread", "-I", str(V9 / "src")]
    for mode in cc:
        entries.append((mode + "_version", [cc[mode], "--version"], {}, 0))
        entries.append((mode + "_dependencies", [cc[mode], *base, "-MM", "-MT", "gate", str(HERE / "gate.cpp")], {}, 0))
        flags = ["-O3", "-DNDEBUG"] if mode == "release" else ["-O1", "-g1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
        binary = str(build / mode)
        entries.append((mode + "_compile", [cc[mode], *base, *flags, str(HERE / "gate.cpp"), "-o", binary], {}, 0))
        env = {} if mode == "release" else SAN_ENV
        entries.append((mode + "_gate", [binary], env, 0))
        for mutant in ("ignore-dead", "first-visited", "normalize-level"):
            entries.append((mode + "_mutant_" + mutant, [binary, "--mutant", mutant], env, 1))
    return entries


def dump(path, obj):
    path.write_text(json.dumps(obj, indent=2, sort_keys=True) + "\n")


def capture(dest, build):
    need(not dest.exists(), "capture destination must be new")
    need(not build.exists(), "build destination must be new")
    dest.mkdir(parents=True)
    build.mkdir(parents=True)
    before = pins()
    meta = {"schema": SCHEMA, "status": "running", "GCP_used": False,
            "build": str(build), "source_before": before, "commands": []}
    dump(dest / "capture.json", meta)
    try:
        for name, argv, extra_env, wanted in recipe(build):
            entry = {"name": name, "argv": argv, "cwd": str(REPO), "extra_env": extra_env,
                     "expected_returncode": wanted, "status": "running"}
            meta["commands"].append(entry)
            dump(dest / "capture.json", meta)
            started = time.monotonic()
            with (dest / (name + ".stdout")).open("wb") as stdout, (dest / (name + ".stderr")).open("wb") as stderr:
                proc = subprocess.run(argv, cwd=REPO, env={**os.environ, **extra_env}, stdout=stdout, stderr=stderr, check=False)
            entry.update(returncode=proc.returncode, elapsed_s=time.monotonic() - started, status="completed")
            entry["stdout_sha256"] = sha(dest / (name + ".stdout"))
            entry["stderr_sha256"] = sha(dest / (name + ".stderr"))
            dump(dest / "capture.json", meta)
            need(proc.returncode == wanted, "unexpected return code: " + name)
            need(not (dest / (name + ".stderr")).read_bytes(), "unexpected stderr: " + name)
        meta["source_after"] = pins()
        need(meta["source_after"] == before, "sources changed during capture")
        meta["binaries"] = {str(build / mode): sha(build / mode) for mode in ("release", "sanitize")}
        meta["status"] = "completed"
    except BaseException as error:
        meta["status"] = "failed"
        meta["error"] = type(error).__name__ + ": " + str(error)
        raise
    finally:
        dump(dest / "capture.json", meta)
    return check(dest)


def check(dest):
    meta = json.loads((dest / "capture.json").read_text())
    need(meta["schema"] == SCHEMA and meta["status"] == "completed" and meta["GCP_used"] is False, "capture status")
    current = pins()
    need(meta["source_before"] == current == meta["source_after"], "LIVE source mismatch")
    plan = recipe(Path(meta["build"]))
    need(len(meta["commands"]) == len(plan) == 14, "command count")
    for entry, (name, argv, env, wanted) in zip(meta["commands"], plan):
        need(entry["name"] == name and entry["argv"] == argv and entry["extra_env"] == env and entry["cwd"] == str(REPO), "command recipe binding")
        need(entry["status"] == "completed" and entry["returncode"] == wanted == entry["expected_returncode"], "command result")
        for stream in ("stdout", "stderr"):
            p = dest / (name + "." + stream)
            need(sha(p) == entry[stream + "_sha256"], "output hash")
        need(not (dest / (name + ".stderr")).read_bytes(), "nonempty stderr")
        if "_dependencies" in name:
            words = shlex.split((dest / (name + ".stdout")).read_text().replace("\\\n", " "))
            need(words[0] == "gate:", "dependency target")
            for word in words[1:]:
                p = str(Path(word).resolve())
                need(p in current, "compiled dependency missing pin: " + p)
        if "_mutant_" in name:
            obj = json.loads((dest / (name + ".stdout")).read_text())
            need(obj == {"schema": "mhgp9_full_batch_mutant_v1", "mutant": argv[-1], "status": "killed", "cause": "object_or_reason_mismatch"}, "mutant must fail semantically")
    need(set(meta["binaries"]) == {str(Path(meta["build"]) / mode) for mode in ("release", "sanitize")}, "binary names")
    for binary, digest in meta["binaries"].items():
        need(sha(binary) == digest, "LIVE binary mismatch")
    a = json.loads((dest / "release_gate.stdout").read_text())
    b = json.loads((dest / "sanitize_gate.stdout").read_text())
    need(a == b and a["schema"] == "mhgp9_full_batch_encoder_v1" and a["status"] == "passed", "gate equality")
    need(a["scope"] == "structural_flat_only" and a["GCP_used"] is False and a["threads_executed"] == 1, "scope")
    need(a["comparisons"] == 2 * a["cases"] and a["cases"] == a["accepted"] + a["rejected"], "case partition")
    need(a["schedules"] == 2 and a["accepted"] >= 400 and a["rejected"] >= 6000 and a["nodes"] > 10000 and a["seed"] == 20260927, "gate nonvacuity")
    return {"status": "passed", "scope": "LIVE_local_structural_prototype", "commands": 14, "gate": a, "mutants_killed": 6, "GCP_used": False}


def main():
    p = argparse.ArgumentParser()
    p.add_argument("mode", choices=("capture", "check"))
    p.add_argument("--receipt", type=Path, required=True)
    p.add_argument("--build", type=Path)
    args = p.parse_args()
    if args.mode == "capture":
        need(args.build is not None, "build required")
        result = capture(args.receipt.resolve(), args.build.resolve())
    else:
        result = check(args.receipt.resolve())
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
