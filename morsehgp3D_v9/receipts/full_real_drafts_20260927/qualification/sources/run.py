#!/usr/bin/env python3
"""Capture actual FULL drafts without changing the product or frozen builds."""
import argparse
import importlib.util
import json
import math
import os
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BASE = HERE.parent / "b_q34_factor_plan_20260926"
sys.dont_write_bytecode = True
sys.path.insert(0, str(BASE))
spec = importlib.util.spec_from_file_location("real_draft_base", BASE / "run.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
need, sha, read, save = base.need, base.sha, base.read, base.save
OUT = ROOT / "morsehgp3D_v9/receipts/full_real_drafts_20260927"
BUILD = Path("/workspaces/E-HGP/build/v9-audit-full-real-drafts-20260927-r1")
TARGET = "mhgp9_full_real_drafts"


def pins():
    p = base.source_pins()
    for f in (HERE / "probe.cpp", HERE / "CMakeLists.txt", HERE / "run.py",
              HERE.parent / "b_full_batch_encoder_20260927/encode.hpp"):
        p[str(f)] = sha(f)
    return p


def build_pins():
    return {str(p): sha(p) for mode in ("release", "sanitize") for p in (
        BUILD / mode / TARGET, BUILD / mode / "CMakeCache.txt",
        BUILD / mode / "CMakeFiles" / (TARGET + ".dir") / "flags.make",
        BUILD / mode / "CMakeFiles" / (TARGET + ".dir") / "link.txt")}


def qual_recipe():
    rows = []
    for mode, cc in (("release", "g++"), ("sanitize", "clang++")):
        b = BUILD / mode
        rows += [(mode + "_configure", ["cmake", "-S", str(HERE), "-B", str(b),
                  "-DCMAKE_BUILD_TYPE=" + ("Release" if mode == "release" else "Debug"),
                  "-DCMAKE_CXX_COMPILER=" + cc, "-DMHGP9_SOURCE_ROOT=" + str(ROOT / "morsehgp3D_v9"),
                  "-DMHGP9_GEN_LIBRARY=" + str(base.LIBS[mode]),
                  "-DMHGP9_AUDIT_SANITIZE=" + ("OFF" if mode == "release" else "ON")]),
                 (mode + "_build", ["cmake", "--build", str(b), "--parallel", "1"]),
                 (mode + "_gate", [str(b / TARGET), "--gate"])]
    return rows


def case_recipe(name):
    if name == "ng00":
        data, extra, partitions = base.inputs.grounded("00")
        case = data["full"]
        return [str(BUILD / "release" / TARGET), "--frame", case["path"]], extra, dict(
            name=name, mode="frame", family="none", n=case["n"], input_hash=int(case["point_hash"], 16),
            input=case, partitions=partitions)
    family, n_text = name.rsplit("_", 1)
    n = int(n_text)
    need(family in ("uniform", "terrain", "clusters") and n in (8000, 16000, 32000), "case not in audit plan")
    prior_path = ROOT / f"morsehgp3D_v9/receipts/q3_payload_local_20260926/{name}_export.stdout"
    prior = read(prior_path)
    need(prior["family"] == family and prior["sites"] == n and prior["seed"] == 3, "synthetic identity")
    return [str(BUILD / "release" / TARGET), "--synthetic", family, str(n)], {str(prior_path): sha(prior_path)}, dict(
        name=name, mode="synthetic", family=family, n=n, input_hash=int(prior["fixture_hash"], 16))


def capture(name):
    dest = OUT / name
    need(not dest.exists(), "fresh receipt only")
    if name == "qualification":
        need(not BUILD.exists(), "fresh build only")
        recipe, extras, case = qual_recipe(), {}, None
    else:
        check("qualification")
        argv, extras, case = case_recipe(name)
        recipe = [("measure", argv)]
    dest.mkdir(parents=True)
    collector = base.command_class()(dest, dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
        UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1", LSAN_OPTIONS="exitcode=23"))
    state = dict(status="running", GCP_used=False, source_before=pins() | extras, case=case, commands=[])
    save(dest / "capture.json", state)
    try:
        for label, argv in recipe:
            rc, _, _ = collector.run(label, argv, timeout=1800 if label == "measure" else 600)
            state["commands"] = collector.rows
            save(dest / "capture.json", state)
            need(rc == 0, "command failed: " + label)
        state["source_after"] = pins() | extras
        need(state["source_after"] == state["source_before"], "source drift")
        state["build_pins"] = build_pins()
        state["status"] = "completed"
        save(dest / "capture.json", state)
        check(name)
    except BaseException as error:
        state["status"] = "failed"
        state["error"] = type(error).__name__ + ": " + str(error)
        save(dest / "capture.json", state)
        raise


def check(name):
    dest = OUT / name
    d = read(dest / "capture.json")
    need(d["status"] == "completed" and d["GCP_used"] is False, "closed local receipt")
    if name == "qualification":
        recipe, extras, case = qual_recipe(), {}, None
    else:
        argv, extras, case = case_recipe(name)
        recipe = [("measure", argv)]
    need(d["case"] == case and d["source_before"] == d["source_after"] == pins() | extras, "LIVE source and input mapping")
    need(d["build_pins"] == build_pins(), "complete LIVE build inventory")
    need(len(d["commands"]) == len(recipe), "recipe length")
    for row, (label, argv) in zip(d["commands"], recipe):
        need(row["name"] == label and row["argv"] == argv and row["exit_code"] == 0 and row["group_closed"], "command recipe")
        need(row == read(dest / (label + ".command.json")), "command copy")
        need(all(row.get(k) == v for k, v in read(dest / (label + ".intent.json")).items()), "command intent")
        for stream in ("stdout", "stderr"):
            need(sha(dest / (label + "." + stream)) == row[stream + "_sha256"], "output hash")
    if name == "qualification":
        a, b = read(dest / "release_gate.stdout"), read(dest / "sanitize_gate.stdout")
        need(a == b == dict(schema="mhgp9_full_real_drafts_gate_v1", status="passed", chains=4, orders_reencoded=10, pairs=30, GCP_used=False), "small differential gates")
        for mode in ("release", "sanitize"):
            need(not (dest / (mode + "_gate.stderr")).read_bytes(), "clean gate stderr")
        result = a
    else:
        result = read(dest / "measure.stdout")
        need(result["schema"] == "mhgp9_full_real_drafts_v1" and result["status"] == "passed" and result["GCP_used"] is False, "measure status")
        for key, expected in (("mode", case["mode"]), ("family", case["family"]), ("n", case["n"]),
                              ("input_hash_u64", case["input_hash"]), ("k", 5), ("s", 8), ("seed", 3), ("workers", 4), ("static_threads", 4)):
            need(result[key] == expected, "measure case binding: " + key)
        need([r["k"] for r in result["rows"]] == list(range(1, 6)), "all K rows")
        for r in result["rows"]:
            need(r["flat_source"] is True and r["nodes"] > 0 and r["actions"] == r["nodes"] + r["continuations"], "real flat actions")
            for field in ("native_ms", "prototype_ms"):
                need(len(r[field]) == 3 and all(math.isfinite(x) and x >= 0 for x in r[field]), "three finite timings")
        need(result["draft_capacity_bytes_sum"] == sum(r["draft_capacity_bytes"] for r in result["rows"]), "draft capacities")
        need(result["output_capacity_bytes_sum"] == sum(r["output_capacity_bytes"] for r in result["rows"]), "output capacities")
        need(not (dest / "measure.stderr").read_bytes(), "clean measurement stderr")
    print(json.dumps(dict(status="passed", capture=name, commands=len(recipe), result=result), sort_keys=True))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("capture", "check"))
    parser.add_argument("name")
    args = parser.parse_args()
    capture(args.name) if args.mode == "capture" else check(args.name)
