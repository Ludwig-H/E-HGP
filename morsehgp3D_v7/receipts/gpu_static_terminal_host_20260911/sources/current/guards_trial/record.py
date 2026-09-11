#!/usr/bin/env python3
"""Small structural rejects and copied-source causal terminal mutants."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parent
BASE = ROOT.parent
REPO = BASE.parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--mode", required=True, choices=("stub", "san"))
    parser.add_argument("--mutants", action="store_true")
    args = parser.parse_args()
    out = ROOT / args.out
    out.mkdir(exist_ok=False)
    sources = sorted(path for path in (BASE / "source").rglob("*") if path.is_file())
    sources += sorted(path for path in BASE.iterdir() if path.is_file() and
        (path.suffix in (".cpp", ".cu", ".cuh", ".hpp", ".py") or path.name == "baseline_pins.json"))
    sources += sorted(path for path in ROOT.iterdir() if path.is_file())
    before = {str(path.relative_to(BASE)): sha(path) for path in sources}
    snapshot = out / "source_snapshot"
    for path in sources:
        target = snapshot / path.relative_to(BASE)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    receipt = {"status": "running", "mode": args.mode, "sources_before": before, "commands": [],
        "device_executed": False, "gcp_used": False, "mutants": []}
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    receipt["sanitizer_environment"] = {key: env[key] for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS")}
    def save():
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    def run(name, argv, expected=0, diagnostic=None):
        start = time.time_ns()
        with (out / (name + ".stdout")).open("wb") as stdout, (out / (name + ".stderr")).open("wb") as stderr:
            done = subprocess.run(argv, cwd=REPO, env=env, stdout=stdout, stderr=stderr)
        causal = diagnostic is None or diagnostic in (out / (name + ".stderr")).read_text()
        receipt["commands"].append({"name": name, "argv": argv, "exit_code": done.returncode,
            "expected_exit_code": expected, "causal_diagnostic": diagnostic, "causal_diagnostic_matched": causal,
            "started_ns": start, "ended_ns": time.time_ns(),
            "stdout_sha256": sha(out / (name + ".stdout")), "stderr_sha256": sha(out / (name + ".stderr"))})
        save(); print(name, done.returncode, flush=True)
        if done.returncode != expected or not causal:
            raise RuntimeError(name + " unexpected result")
    def compile_gate(name, tree, binary):
        flags = ["-O2"] if args.mode == "stub" else ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"]
        run(name, ["g++", "-std=c++20", *flags, "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
            "-MMD", "-MF", str(out / (name + ".d")), str(tree / "guards_trial/gate.cpp"), "-o", str(binary)])
    try:
        run("compiler", ["g++", "--version"])
        compile_gate("compile", snapshot, out / "gate")
        receipt["binary_sha256"] = sha(out / "gate")
        run("owner_cross", [str(out / "gate"), "--owner-cross"])
        run("selftest", [str(out / "gate"), "--selftest"])
        run("unknown", [str(out / "gate"), "--unknown"], 2)
        run("missing_arg", [str(out / "gate")], 2)
        if args.mutants:
            changes = [
                ("strict_before", "compare_exact_level(decode_level(local.level), decode_level(request.before)) >= 0",
                 "compare_exact_level(decode_level(local.level), decode_level(request.before)) > 0", 1,
                 "guard.equal_cut_must_be_refused"),
                ("rank_window", "if (request.k >= begin && request.k <= end)",
                 "if ((static_cast<void>(begin), request.k <= end))", 1,
                 "guard.present_but_wrong_window_descends_twice"),
                ("last_support", "sites[selected.slots[0]] = intruder.intruder;",
                 "sites[selected.slots[selected.q - 1]] = intruder.intruder;", 1,
                 "guard.present_but_wrong_window_descends_twice"),
                ("unsigned_key_order", " ^ (u64{1} << 63)", "", 2, "guard.signed_key_order"),
                ("trace_as_quota", "note(trace, sites, request.k, local, intruder.intruder, kAbsentBall);",
                 "note(trace, sites, request.k, local, intruder.intruder, kAbsentBall);\n"
                 "    if (trace && trace->overflow) { result.status = Status::kMissingWeakTerminal; return result; }",
                 1, "guard.trace_limit_not_search_limit"),
            ]
            for name, old, new, count, diagnostic in changes:
                tree = out / "mutants" / name / "source_snapshot"
                shutil.copytree(snapshot, tree)
                header = tree / "terminal.cuh"
                original = header.read_text()
                if original.count(old) != count:
                    raise RuntimeError("mutant anchor changed: " + name)
                header.write_text(original.replace(old, new))
                binary = out / "mutants" / name / "gate"
                compile_gate("compile_" + name, tree, binary)
                run("mutant_" + name, [str(binary), "--selftest"], 1, diagnostic)
                receipt["mutants"].append({"name": name, "source_sha256": sha(header), "binary_sha256": sha(binary),
                    "causal_diagnostic": diagnostic})
                save()
            # Actual historical owner, byte-identical to the closed r2 source;
            # the new cross-type fixture must causally reject its behavior.
            tree = out / "mutants" / "historical_owner" / "source_snapshot"
            shutil.copytree(snapshot, tree)
            header = tree / "terminal_owner.hpp"
            shutil.copy2(snapshot / "guards_trial/terminal_owner.r2.hpp", header)
            binary = out / "mutants" / "historical_owner" / "gate"
            diagnostic = "guard.cross_owner_must_refuse_foreign_index"
            compile_gate("compile_historical_owner", tree, binary)
            run("mutant_historical_owner", [str(binary), "--owner-cross"], 1, diagnostic)
            receipt["mutants"].append({"name": "historical_owner", "source_sha256": sha(header),
                "binary_sha256": sha(binary), "causal_diagnostic": diagnostic,
                "historical_owner_byte_identical": True})
            save()
        receipt["status"] = "passed"
    except Exception as error:
        receipt["status"] = "failed"; receipt["error"] = str(error)
    finally:
        receipt["sources_after"] = {str(path.relative_to(BASE)): sha(path) for path in sources}
        receipt["sources_stable"] = before == receipt["sources_after"]
        if not receipt["sources_stable"]: receipt["status"] = "failed"
        save()
    return 0 if receipt["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
