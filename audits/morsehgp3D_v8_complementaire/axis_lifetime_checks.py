#!/usr/bin/env python3
"""Fresh, bounded lifetime/minimum-pair checks against the published API."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PIN = "f5430f57d42873cce44de482198f90d3068f0d77"
SOURCES = ["core/types.hpp", "spindle/predicates.hpp", "pipeline/local_credits.hpp",
           "pipeline/local_credits.cpp", "pipeline/tube_credits.hpp",
           "pipeline/axis_q2.hpp", "pipeline/axis_q2.cpp"]


def require(value: bool, reason: str) -> None:
    if not value:
        raise RuntimeError(reason)


def digest(text: str) -> str:
    return hashlib.sha256(text.encode()).hexdigest()


def run(args: list[str]) -> dict:
    p = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, check=False)
    return {"argv": args, "exit_code": p.returncode, "stdout": p.stdout, "stderr": p.stderr}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--replay", type=Path)
    args = parser.parse_args()
    if args.replay:
        previous = json.loads(args.replay.read_text())
        snapshot = previous["snapshot_utf8"]
        require(previous["source_commit"] == PIN, "wrong source commit")
        require({name: digest(text) for name, text in snapshot.items()} ==
                previous["source_sha256"], "snapshot hash mismatch")
    else:
        snapshot = {}
        for name in SOURCES:
            r = run(["git", "show", PIN + ":morsehgp3D_v8/src/" + name])
            require(r["exit_code"] == 0, "pinned source unavailable")
            snapshot[name] = r["stdout"]
        snapshot["probe.cpp"] = (HERE / "axis_lifetime_probe.cpp").read_text()
    require(set(snapshot) == set(SOURCES + ["probe.cpp"]), "unexpected manifest")
    compiler = shutil.which("g++")
    require(compiler is not None, "compiler unavailable")
    version = run([compiler, "--version"])
    require(version["exit_code"] == 0, "compiler identification failed")
    receipt = {
        "schema": "mhgp8_axis_lifetime_audit_v1", "source_commit": PIN,
        "scope": "factory allocation rollback, move construction, and nonempty q2 restriction",
        "phase": "exploration_v8_hors_registre", "backend": "cpu_reference",
        "profile": "quantized_u16_input_only", "public_status": "not_claimed",
        "python_optimized": not __debug__, "gcp_used": False,
        "compiler": version, "compiler_sha256": hashlib.sha256(Path(compiler).read_bytes()).hexdigest(),
        "source_sha256": {name: digest(text) for name, text in snapshot.items()},
        "snapshot_utf8": snapshot, "runner_sha256": digest(Path(__file__).read_text()),
        "commands": [], "status": "failed",
    }
    flags = [compiler, "-std=c++20", "-O1", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-fsanitize=undefined", "-fno-sanitize-recover=all"]
    try:
        with tempfile.TemporaryDirectory(prefix="mhgp8_axis_lifetime_") as directory:
            build = Path(directory)
            for name, text in snapshot.items():
                target = build / "src" / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(text)
            local_object = build / "local.o"
            r = run(flags + ["-I" + str(build / "src"), "-c",
                             str(build / "src/pipeline/local_credits.cpp"), "-o", str(local_object)])
            receipt["commands"].append(r)
            require(r["exit_code"] == 0, "local credit compilation failed")
            receipt["results"] = {}
            for label in ("positive", "lost_restriction_on_move"):
                axis_text = snapshot["pipeline/axis_q2.cpp"]
                if label != "positive":
                    old = "has_restriction_(std::exchange(other.has_restriction_, false))"
                    require(axis_text.count(old) == 1, "move mutation target not unique")
                    axis_text = axis_text.replace(old, "has_restriction_(false)")
                axis_source = build / (label + "_axis.cpp")
                axis_source.write_text(axis_text)
                binary = build / label
                r = run(flags + ["-I" + str(build / "src"),
                                 "-I" + str(build / "src/pipeline"),
                                 str(build / "src/probe.cpp"), str(axis_source),
                                 str(local_object), "-o", str(binary)])
                receipt["commands"].append(r)
                require(r["exit_code"] == 0, "axis/probe compilation failed")
                r = run([str(binary)])
                receipt["commands"].append(r)
                receipt["results"][label] = {**r, "axis_cpp_sha256": digest(axis_text),
                    "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest()}
                if label == "positive":
                    require(r["exit_code"] == 0, "positive lifetime gate failed")
                    stats = json.loads(r["stdout"])
                    require(stats["axis_plans"] == 96 and stats["membership_checks"] == 345600 and
                            stats["factory_failures"] >= 16 and stats["factory_successes"] == 4 and
                            stats["positive_need_plans"] == 42 and stats["zero_need_plans"] == 6 and
                            stats["moves_without_allocation"] == 4 and stats["moved_rejections"] == 12,
                            "non-vacuity gate failed")
                    receipt["statistics"] = stats
                else:
                    require(r["exit_code"] == 1 and r["stderr"] ==
                            "axis lifetime audit: keeps differs from fragments\n",
                            "move mutation not causally rejected")
        receipt["status"] = "passed"
    finally:
        args.output.write_text(json.dumps(receipt, ensure_ascii=False, sort_keys=True, indent=2) + "\n")
    print(json.dumps({"status": receipt["status"], **receipt["statistics"]}, sort_keys=True))


if __name__ == "__main__":
    main()
