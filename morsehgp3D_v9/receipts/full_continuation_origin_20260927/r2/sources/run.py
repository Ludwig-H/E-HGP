#!/usr/bin/env python3
"""Fresh native continuation capture; explicit frozen harness reuse, no cloud."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
OLD = HERE.parent / "b_full_real_drafts_20260927"
spec = importlib.util.spec_from_file_location("continuation_capture_base", OLD / "run.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
ROOT = base.ROOT
need, sha, read, save = base.need, base.sha, base.read, base.save
BUILD = Path("/workspaces/E-HGP/build/v9-continuation-origin-20260927-r2")
OUT = ROOT / "morsehgp3D_v9/receipts/full_continuation_origin_20260927/r2"
TARGET = "mhgp9_continuation_origin"
SCHEMA = "mhgp9_continuation_origin_capture_v1"
base.HERE, base.BUILD, base.TARGET = HERE, BUILD, TARGET
SAN = dict(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1", LSAN_OPTIONS="exitcode=23")


def pins():
    paths = [HERE / n for n in ("gate.cpp", "capture_helpers.hpp", "CMakeLists.txt", "run.py")]
    paths += [OLD / "probe.cpp", OLD / "run.py", HERE.parent / "b_full_batch_encoder_20260927/encode.hpp",
              HERE.parent / "b_full_first_parent_20260927/first.hpp",
              ROOT / "morsehgp3D_v9/tests/tower/full_ball_tower_gate.cpp"]
    return base.base.source_pins() | {str(p): sha(p) for p in paths}


def recipe():
    rows = []
    for name, argv in base.qual_recipe():
        if name.endswith("_gate"):
            argv = [argv[0]]
        rows.append((name, argv, 0))
        if name.endswith("_gate"):
            rows.append((name.replace("_gate", "_mutant"), [argv[0], "--drop-continuations"], 1))
    return rows


def check():
    d = read(OUT / "capture.json")
    need(d["schema"] == SCHEMA and d["status"] == "completed" and d["GCP_used"] is False, "closed capture")
    need(d["source_before"] == d["source_after"] == pins(), "LIVE source closure")
    need(d["build_pins"] == base.build_pins(), "LIVE complete build inventory")
    plan = recipe()
    need(len(d["commands"]) == len(plan) == 8 and d["environment"] == SAN, "complete recipe/environment")
    gates = []
    for row, (name, argv, code) in zip(d["commands"], plan):
        need(row["name"] == name and row["argv"] == argv and row["exit_code"] == code and row["group_closed"], "command recipe")
        need(row == read(OUT / (name + ".command.json")), "command copy")
        need(all(row.get(k) == v for k, v in read(OUT / (name + ".intent.json")).items()), "intent binding")
        for stream in ("stdout", "stderr"):
            need(sha(OUT / (name + "." + stream)) == row[stream + "_sha256"], "output binding")
        if name.endswith(("_gate", "_mutant")):
            need(not (OUT / (name + ".stderr")).read_bytes(), "clean stderr")
            value = read(OUT / (name + ".stdout"))
            if name.endswith("_mutant"):
                need(value == dict(schema="mhgp9_continuation_origin_mutant_v1", status="killed",
                                   cause="dated_contribution_lost_with_identical_topology"), "causal mutant")
            else:
                need(value["schema"] == "mhgp9_continuation_origin_v1" and value["status"] == "passed" and
                     value["GCP_used"] is False and value["chains"] == 32 and value["regular_chains"] >= 16 and
                     value["extended_chains"] > 0 and value["extended_without_continuation"] > 0 and
                     value["continuation_actions"] > 0 and value["orders"] > 100 and
                     value["fast_comparisons"] + value["fallback_comparisons"] == 2*value["orders"] and
                     value["fallback_comparisons"] > 0, "native path nonvacuity")
                gates.append(value)
    need(len(gates) == 2 and gates[0] == gates[1], "Release/sanitizer agree")
    return dict(status="passed", commands=8, gate=gates[0], GCP_used=False)


def capture():
    need(not OUT.exists() and not BUILD.exists(), "fresh capture/build required")
    OUT.mkdir(parents=True)
    # Snapshot own sources before commands, retaining any failed experiment.
    snapshots = OUT / "sources"
    snapshots.mkdir()
    for name in ("gate.cpp", "capture_helpers.hpp", "CMakeLists.txt", "run.py"):
        (snapshots / name).write_bytes((HERE / name).read_bytes())
    collector = base.base.command_class()(OUT, dict(os.environ, **SAN))
    d = dict(schema=SCHEMA, status="running", GCP_used=False, environment=SAN, source_before=pins(), commands=[])
    save(OUT / "capture.json", d)
    try:
        for name, argv, expected in recipe():
            rc, _, _ = collector.run(name, argv, timeout=600)
            d["commands"] = collector.rows
            save(OUT / "capture.json", d)
            need(rc == expected, "command failed: " + name)
        d["source_after"] = pins()
        need(d["source_before"] == d["source_after"], "source drift")
        d["build_pins"] = base.build_pins()
        d["status"] = "completed"
        save(OUT / "capture.json", d)
        return check()
    except BaseException as error:
        d["status"] = "failed"
        d["error"] = type(error).__name__ + ": " + str(error)
        save(OUT / "capture.json", d)
        raise


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("capture", "check"))
    args = parser.parse_args()
    print(json.dumps(capture() if args.mode == "capture" else check(), sort_keys=True))
