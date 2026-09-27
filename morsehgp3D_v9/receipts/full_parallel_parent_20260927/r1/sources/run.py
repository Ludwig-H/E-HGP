#!/usr/bin/env python3
"""Independent parallel-parent prototype capture, no product/GCP changes."""
import argparse
import importlib.util
import json
from pathlib import Path
import shlex
import shutil

HERE = Path(__file__).resolve().parent
V9 = HERE.parent.parent
OLD = HERE.parent / "b_full_batch_encoder_20260927/run.py"
spec = importlib.util.spec_from_file_location("frozen_batch_capture", OLD)
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
need, sha = base.need, base.sha
ORIGINAL_PINS = base.pins
BUILD = Path("/workspaces/E-HGP/build/v9-audit-full-parallel-parent-20260927-r1")
RECEIPT = V9 / "receipts/full_parallel_parent_20260927/r1"
MUTANTS = ("ignore-duplicate", "first-visited", "last-occurrence")


def pins():
    old = ORIGINAL_PINS()
    for p in (HERE / "parallel.hpp", HERE / "gate.cpp", HERE / "run.py",
              HERE.parent / "b_full_first_parent_20260927/first.hpp",
              HERE.parent / "b_full_first_parent_20260927/fixture_helpers.hpp"):
        old[str(p)] = sha(p)
    return old


def recipe(build):
    rows = []
    common = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread", "-I", str(V9 / "src")]
    for mode, compiler in (("release", "g++"), ("sanitize", "clang++")):
        cc = shutil.which(compiler)
        need(cc is not None, "compiler")
        rows.append((mode + "_version", [cc, "--version"], {}, 0))
        rows.append((mode + "_dependencies", [cc, *common, "-MM", "-MT", "gate", str(HERE / "gate.cpp")], {}, 0))
        flags = ["-O3", "-DNDEBUG"] if mode == "release" else ["-O1", "-g1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
        rows.append((mode + "_compile", [cc, *common, *flags, str(HERE / "gate.cpp"), "-latomic", "-o", str(build / mode)], {}, 0))
        env = {} if mode == "release" else base.SAN_ENV
        rows.append((mode + "_gate", [str(build / mode)], env, 0))
        rows.extend((mode + "_mutant_" + m, [str(build / mode), "--mutant", m], env, 1) for m in MUTANTS)
    return rows


def check(dest):
    d = json.loads((dest / "capture.json").read_text())
    need(d["schema"] == "mhgp9_full_parallel_parent_capture_v1" and d["status"] == "completed" and d["GCP_used"] is False, "closed capture")
    p = pins()
    need(d["source_before"] == d["source_after"] == p, "LIVE sources")
    need(d["build"] == str(BUILD), "build identity")
    need(d["binaries"] == {str(BUILD / m): sha(BUILD / m) for m in ("release", "sanitize")}, "LIVE binary inventory")
    plan = recipe(BUILD)
    need(len(d["commands"]) == len(plan) == 14, "complete recipe")
    for row, (name, argv, env, code) in zip(d["commands"], plan):
        need(row["name"] == name and row["argv"] == argv and row["cwd"] == str(V9.parent) and row["extra_env"] == env, "command binding")
        need(row["returncode"] == row["expected_returncode"] == code and row["status"] == "completed", "command status")
        for stream in ("stdout", "stderr"):
            need(sha(dest / (name + "." + stream)) == row[stream + "_sha256"], "output binding")
        need(not (dest / (name + ".stderr")).read_bytes(), "clean stderr")
        if name.endswith("_dependencies"):
            words = shlex.split((dest / (name + ".stdout")).read_text().replace("\\\n", " "))
            need(words[0] == "gate:", "dependency target")
            need(all(str(Path(w).resolve()) in p for w in words[1:]), "complete compiled dependency pins")
        if "_mutant_" in name:
            need(json.loads((dest / (name + ".stdout")).read_text()) == dict(
                schema="mhgp9_full_parallel_parent_mutant_v1", status="killed", cause="object_or_reason_mismatch", mutant=argv[-1]), "semantic mutant")
    a, b = (json.loads((dest / (m + "_gate.stdout")).read_text()) for m in ("release", "sanitize"))
    need(a == b and a["schema"] == "mhgp9_full_parallel_parent_v1" and a["status"] == "passed", "gate equality")
    need(a["GCP_used"] is False and a["simultaneous_threads"] == a["parent_workers_active"] == 4 and a["accepted"] >= 60 and a["cases"] >= 350, "nonvacuity")
    need(a["comparisons"] == 6 * a["cases"] and a["accepted"] + a["rejected"] == a["cases"], "case partition")
    need(a["fast_calls"] > 1000 and a["fallback_calls"] > 100 and a["threads_started"] > 0, "both paths and threads exercised")
    need(a["workers"] == [1, 4] and a["grains"] == [1, 7] and a["schedules"] == 2 and a["scheduler_checks"] == 12, "scheduler coverage")
    need(a["atomic_cell_size"] == 8 and 0 < a["atomic_required_alignment"] <= 8 and isinstance(a["atomic_is_lock_free"], bool), "atomic representation")
    return dict(status="passed", commands=14, gate=a, mutants_killed=6, GCP_used=False)


# Reuse only the closed local process collector. These bindings are confined
# to this import instance; no source of the first capture is edited.
base.pins = pins
base.recipe = recipe
base.check = check
base.SCHEMA = "mhgp9_full_parallel_parent_capture_v1"

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("capture", "check"))
    args = parser.parse_args()
    result = base.capture(RECEIPT, BUILD) if args.mode == "capture" else check(RECEIPT)
    print(json.dumps(result, sort_keys=True))

