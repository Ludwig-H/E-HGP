#!/usr/bin/env python3
"""One private compilation at a time, immutable snapshot, exact exit/cause gates."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

HERE = Path(__file__).resolve().parent
BOOST = "/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_files():
    selected = [p for p in HERE.iterdir() if p.is_file() and p.suffix in
                (".py", ".cpp", ".hpp", ".md", ".json", ".diff")]
    selected += [p for p in (HERE / "source").rglob("*") if p.is_file()]
    return {p.relative_to(HERE).as_posix(): sha(p) for p in sorted(selected)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--mode", choices=("o2", "san"), default="o2")
    parser.add_argument("--mutant", type=int, choices=range(5), default=0)
    args = parser.parse_args()
    if Path(args.out).name != args.out or args.out in (".", ".."):
        raise RuntimeError("single fresh directory name required")
    out = HERE / args.out
    out.mkdir(exist_ok=False)
    source_before = source_files()
    snapshot = out / "source_snapshot"
    for name, pin in source_before.items():
        data = (HERE / name).read_bytes()
        if hashlib.sha256(data).hexdigest() != pin:
            raise RuntimeError("source changed before snapshot")
        target = snapshot / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("xb") as output:
            output.write(data)
    env = dict(os.environ)
    if args.mode == "san":
        env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
        env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    report = {"status": "running", "mode": args.mode, "mutant": args.mutant,
        "scope": "private_diameter_MEB_and_actual_small_census_calls", "compiler_jobs": 1,
        "sources_before": source_before, "commands": [], "benchmark": False,
        "device_executed": False, "gcp_used": False,
        "sanitizer_environment": {key: env[key] for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS") if args.mode == "san"}}

    def save():
        (out / "receipt.json").write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")

    def execute(name, argv, expected, cause=None):
        started = time.monotonic()
        print("start=" + name, flush=True)
        with (out / (name + ".stdout")).open("xb") as stdout, (out / (name + ".stderr")).open("xb") as stderr:
            child = subprocess.Popen(argv, stdout=stdout, stderr=stderr, env=env, cwd=HERE)
            beat = started
            while child.poll() is None:
                time.sleep(0.25)
                if time.monotonic() - beat >= 30:
                    print("running=" + name, flush=True)
                    beat = time.monotonic()
            code = child.returncode
        command = {"name": name, "argv": argv, "exit_code": code, "expected_exit_code": expected,
            "elapsed_s_instrumented_not_benchmark": time.monotonic() - started,
            "stdout_sha256": sha(out / (name + ".stdout")), "stderr_sha256": sha(out / (name + ".stderr"))}
        if cause is not None:
            command["expected_stderr_line"] = cause
        report["commands"].append(command)
        save()
        print("complete=" + name + " exit=" + str(code), flush=True)
        if code != expected or (cause is not None and (out / (name + ".stderr")).read_text() != cause + "\n"):
            raise RuntimeError("unexpected code/cause: " + name)

    save()
    try:
        binary = out / "gate"
        flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread", "-isystem", BOOST]
        flags += ["-O2"] if args.mode == "o2" else ["-O1", "-g", "-fsanitize=address,undefined",
            "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
        execute("compile", ["g++", *flags, "-DMHGP7_DIAMETER_MUTANT=" + str(args.mutant),
            "-MMD", "-MF", str(out / "gate.d"), str(snapshot / "gate.cpp"), "-o", str(binary)], 0)
        report["binary_sha256"] = sha(binary)
        if args.mutant:
            mode, cause = {1: ("ties", "diameter.canonical"), 2: ("counter", "diameter.paid_pairs"),
                3: ("order", "diameter.paid_powers"), 4: ("shell", "diameter.shell")}[args.mutant]
            execute("mutant", [str(binary), "--mutant-" + mode], 1, cause)
        else:
            execute("selftest", [str(binary), "--selftest"], 0)
            execute("unknown", [str(binary), "--unknown"], 2)
            execute("missing", [str(binary)], 2)
        report["binary_after_sha256"] = sha(binary)
        if report["binary_after_sha256"] != report["binary_sha256"]:
            raise RuntimeError("binary drift")
        report["status"] = "passed"
    except BaseException as error:
        report["status"] = "failed"
        report["error"] = str(error)
        raise
    finally:
        report["sources_after"] = source_files()
        report["snapshot_after"] = {p.relative_to(snapshot).as_posix(): sha(p) for p in snapshot.rglob("*") if p.is_file()}
        report["sources_stable"] = report["sources_after"] == source_before
        report["snapshot_stable"] = report["snapshot_after"] == source_before
        if not report["sources_stable"] or not report["snapshot_stable"]:
            report["status"] = "failed"
        save()
    if report["status"] != "passed":
        raise SystemExit(1)


if __name__ == "__main__":
    main()
