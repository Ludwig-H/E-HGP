#!/usr/bin/env python3
"""One fresh local qualification; no timeout, rlimit, GCP or historical runner."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import time

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
ACCOUNTING = "preflight_survivor_then_direct_census_v2"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def verify(run):
    receipt = json.loads((run / "receipt.json").read_text())
    require(receipt["status"] == "completed", "receipt incomplete")
    for name, pin in receipt["artifacts"].items():
        require(sha(run / name) == pin, "artifact " + name)
    commands = receipt["commands"]
    require(len(commands) == 20 and all(c["exit_code"] == c["expected_exit_code"] for c in commands), "commands")
    for mode in ("O2", "SAN"):
        for option in ("selftest", "rejects"):
            row = json.loads((run / f"gate_{mode}_{option}.stdout").read_text())
            require(row["status"] == "passed" and type(row["checks"]) is int and row["checks"] == 40, "gate")
    fingerprints = []
    for p in ("0", "unlimited"):
        for threads in (1, 4):
            rows = [json.loads(line) for line in (run / f"micro_p{p}_t{threads}.stdout").read_text().splitlines()]
            require(len(rows) == 10 and rows[0]["type"] == "configuration" and rows[-1]["type"] == "terminal", "micro rows")
            require(all(row.get("census_payload_accounting") == ACCOUNTING for row in rows), "accounting")
            require(rows[-1]["outcome"] == "complete_relative" and rows[-1]["complete_requested_horizontal_orders"] is True, "micro terminal")
            require(rows[0]["n"] == 8 and rows[0]["threads"] == threads, "micro config")
            require(rows[0]["sizeof_survivor"] == 16, "captured ABI")
            fingerprints.append((rows[-1]["input_digest"], rows[-1]["certificate_digest"]))
    require(len(set(fingerprints)) == 1, "micro semantic parity")
    for name in ("probe_digest", "probe_unknown"):
        row = json.loads((run / (name + ".stdout")).read_text())
        require(row["census_payload_accounting"] == ACCOUNTING, "nonconfiguration accounting")
    print(json.dumps({"status": "passed", "commands": 20, "gate_checks_per_mode": 40,
                      "micros_n8": 4, "semantic_parity": True, "public_status": "not_claimed"}, sort_keys=True))


def execute():
    run = BASE / "run_r1"
    run.mkdir(exist_ok=False)
    commands = []
    receipt = {"status": "failed", "public_status": "not_claimed", "commands": commands,
               "engine_timeouts": None, "imposed_process_rlimits": None}
    selected = list((ROOT / "morsehgp3D_v7/src").rglob("*.hpp")) + list((ROOT / "morsehgp3D_v7/bench").glob("*.hpp"))
    selected += [ROOT / "morsehgp3D_v7" / name for name in (
        "bench/full_gabriel_lazy_probe.cpp", "tests/full_gabriel_census_payload_gate.cpp", "tests/full_gabriel_probe_limits_gate.cpp")]
    before = {str(path.relative_to(ROOT)): sha(path) for path in sorted(selected)}
    write(run / "sources_before.json", before)
    for path in selected:
        dest = run / "sources" / path.relative_to(ROOT)
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, dest)
    shutil.copyfile(__file__, run / "record.py")
    (run / "bin").mkdir()

    def command(name, argv, expected=0):
        item = {"name": name, "argv": list(map(str, argv)), "cwd": str(run), "expected_exit_code": expected,
                "started_ns": time.time_ns(), "timeout": None, "imposed_rlimits": None}
        write(run / (name + ".intent.json"), item)
        with (run / (name + ".stdout")).open("wb") as stdout, (run / (name + ".stderr")).open("wb") as stderr:
            process = subprocess.Popen(item["argv"], cwd=run, stdout=stdout, stderr=stderr, start_new_session=True)
            item["pid"] = process.pid
            write(run / (name + ".spawn.json"), item)
            try:
                item["exit_code"] = process.wait()
            except BaseException:
                os.killpg(process.pid, signal.SIGKILL)
                item["exit_code"] = process.wait()
                raise
            finally:
                item["ended_ns"] = time.time_ns()
                commands.append(item)
                write(run / (name + ".command.json"), item)
        print(name, item["exit_code"], flush=True)
        require(item["exit_code"] == expected, name + " unexpected exit")

    try:
        command("compiler_version", ["/usr/bin/g++", "--version"])
        source = run / "sources/morsehgp3D_v7"
        common = ["/usr/bin/g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread"]
        builds = [("gate_O2", "tests/full_gabriel_census_payload_gate.cpp", ["-O2", "-DNDEBUG"]),
                  ("gate_SAN", "tests/full_gabriel_census_payload_gate.cpp", ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]),
                  ("limits_O2", "tests/full_gabriel_probe_limits_gate.cpp", ["-O2", "-DNDEBUG"]),
                  ("probe_O3", "bench/full_gabriel_lazy_probe.cpp", ["-O3", "-DNDEBUG"])]
        for name, filename, flags in builds:
            command("compile_" + name, common + flags + ["-MMD", "-MF", run / (name + ".d"), source / filename, "-o", run / "bin" / name])
        for mode in ("O2", "SAN"):
            for option, expected in (("selftest", 0), ("rejects", 0), ("unknown", 2)):
                command("gate_" + mode + "_" + option, [run / "bin" / ("gate_" + mode), "--" + option], expected)
        for option, expected in (("selftest", 0), ("unknown", 2)):
            command("limits_" + option, [run / "bin/limits_O2", "--" + option], expected)
        command("probe_digest", [run / "bin/probe_O3", "--digest-selftest"])
        command("probe_unknown", [run / "bin/probe_O3", "--unknown"], 2)
        command("probe_help", [run / "bin/probe_O3", "--help"], 2)
        for p in ("0", "unlimited"):
            for threads in (1, 4):
                argv = [run / "bin/probe_O3", "--n=8", "--s=8", "--kmax=10", "--alias-policy=lazy", "--cache-entries=1000000", "--meb-proposal-supports=" + p]
                if threads != 1:
                    argv += ["--threads=" + str(threads)]
                command(f"micro_p{p}_t{threads}", argv)
        after = {name: sha(ROOT / name) for name in before}
        write(run / "sources_after.json", after)
        require(before == after, "live source drift")
        receipt["status"] = "completed"
    finally:
        receipt["artifacts"] = {str(path.relative_to(run)): sha(path) for path in sorted(run.rglob("*")) if path.is_file()}
        write(run / "receipt.json", receipt)
    verify(run)


if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "--execute":
        execute()
    elif len(sys.argv) == 3 and sys.argv[1] == "--verify":
        verify(Path(sys.argv[2]).resolve())
    else:
        print("record.py --execute | --verify RUN; inert otherwise", file=sys.stderr)
        sys.exit(2)
