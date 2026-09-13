#!/usr/bin/env python3
"""Snapshot-only, small exception/lifetime audit of the census implementation."""
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
SOURCES = ["core/types.hpp", "spindle/predicates.hpp", "pipeline/local_credits.hpp",
           "pipeline/local_credits.cpp", "pipeline/tube_credits.hpp", "pipeline/axis_q2.hpp",
           "pipeline/axis_q2.cpp", "pipeline/q2_census.hpp", "pipeline/q2_census.cpp"]


def require(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def command(args: list[str]) -> dict:
    result = subprocess.run(args, cwd=ROOT, text=True, capture_output=True, check=False)
    return {"argv": args, "exit_code": result.returncode,
            "stdout": result.stdout, "stderr": result.stderr}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--replay", type=Path)
    args = parser.parse_args()
    if args.replay:
        previous = json.loads(args.replay.read_text())
        snapshot = previous["snapshot_utf8"]
        source_state = previous["source_state"]
        capture_head = previous["capture_head"]
        require({name: digest(text.encode()) for name, text in snapshot.items()} ==
                previous["source_sha256"], "snapshot digest mismatch")
    else:
        paths = {name: ROOT / "morsehgp3D_v8/src" / name for name in SOURCES}
        paths["probe.cpp"] = HERE / "q2_census_lifetime_probe.cpp"
        raw = {name: path.read_bytes() for name, path in paths.items()}
        require(all(path.read_bytes() == raw[name] for name, path in paths.items()),
                "source files changed while capturing snapshot")
        snapshot = {name: data.decode() for name, data in raw.items()}
        source_state = "private working-tree snapshot; unpublished census source bytes"
        head = command(["git", "rev-parse", "HEAD"])
        require(head["exit_code"] == 0, "cannot record capture head")
        capture_head = head["stdout"].strip()
    require(set(snapshot) == set(SOURCES + ["probe.cpp"]), "unexpected source manifest")
    compiler = shutil.which("g++")
    require(compiler is not None, "compiler unavailable")
    version = command([compiler, "--version"])
    require(version["exit_code"] == 0, "compiler identification failed")
    receipt = {
        "schema": "mhgp8_q2_census_lifetime_v1", "scope": "small lifecycle and exception audit",
        "phase": "exploration_v8_hors_registre", "backend": "cpu_reference",
        "profile": "quantized_u16_input_only", "public_status": "not_claimed", "gcp_used": False,
        "source_state": source_state, "capture_head": capture_head,
        "snapshot_utf8": snapshot,
        "source_sha256": {name: digest(text.encode()) for name, text in snapshot.items()},
        "runner_sha256": digest(Path(__file__).read_bytes()), "python_optimized": not __debug__,
        "compiler": version, "compiler_sha256": digest(Path(compiler).read_bytes()),
        "commands": [], "status": "failed",
    }
    flags = [compiler, "-std=c++20", "-O1", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-fsanitize=undefined", "-fno-sanitize-recover=all"]
    variants = {
        "positive": None,
        "swallow_callback_exception": (
            "consumer(Q2Support{a_id, b_id, key, interior, shell});",
            "try { consumer(Q2Support{a_id, b_id, key, interior, shell}); } catch (...) {}",
            "census lifetime audit: callback exception was swallowed\n"),
        "empty_before_validation": (
            "  if (&index.rectangle() != &plan.rectangle()) {",
            "  if (plan.candidate_pairs() == 0) return {};\n"
            "  if (&index.rectangle() != &plan.rectangle()) {",
            "census lifetime audit: empty-residual validation was bypassed\n"),
    }
    try:
        with tempfile.TemporaryDirectory(prefix="mhgp8_q2_lifetime_") as directory:
            build = Path(directory)
            for name, text in snapshot.items():
                path = build / "src" / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text)
            objects = []
            for name in ("pipeline/local_credits.cpp", "pipeline/axis_q2.cpp"):
                output = build / (Path(name).stem + ".o")
                run = command(flags + ["-I" + str(build / "src"), "-c",
                                       str(build / "src" / name), "-o", str(output)])
                receipt["commands"].append(run)
                require(run["exit_code"] == 0, "dependency compilation failed")
                objects.append(str(output))
            receipt["results"] = {}
            for label, mutation in variants.items():
                text = snapshot["pipeline/q2_census.cpp"]
                if mutation:
                    require(text.count(mutation[0]) == 1, "mutation anchor is not unique")
                    text = text.replace(mutation[0], mutation[1])
                source = build / (label + ".cpp")
                binary = build / label
                source.write_text(text)
                run = command(flags + ["-I" + str(build / "src"),
                                       "-I" + str(build / "src/pipeline"),
                                       str(build / "src/probe.cpp"), str(source)] +
                              objects + ["-o", str(binary)])
                receipt["commands"].append(run)
                require(run["exit_code"] == 0, "census/probe compilation failed")
                run = command([str(binary)])
                receipt["commands"].append(run)
                receipt["results"][label] = {**run, "census_cpp_sha256": digest(text.encode()),
                                             "binary_sha256": digest(binary.read_bytes())}
                if mutation is None:
                    require(run["exit_code"] == 0, "positive census lifetime gate failed")
                    stats = json.loads(run["stdout"])
                    require(stats["index_allocation_failures"] >= 5 and
                            stats["census_allocation_failures"] >= 5 and
                            stats["allocation_failures_after_output"] >= 2 and
                            stats["callback_exceptions"] == 2 and
                            stats["partial_supports_retained"] == 6 and
                            stats["empty_api_rejections"] == 8,
                            "non-vacuity gate failed")
                    receipt["statistics"] = stats
                else:
                    require(run["exit_code"] == 1 and run["stderr"] == mutation[2],
                            "mutation did not fail at the expected contract gate")
        if not args.replay:
            receipt["source_sha256_after"] = {
                name: digest((ROOT / "morsehgp3D_v8/src" / name).read_bytes()) for name in SOURCES}
            receipt["sources_unchanged_at_closing"] = all(
                receipt["source_sha256_after"][name] == receipt["source_sha256"][name]
                for name in SOURCES)
        receipt["status"] = "passed"
    finally:
        args.output.write_text(json.dumps(receipt, ensure_ascii=False, sort_keys=True, indent=2) + "\n")
    print(json.dumps({"status": receipt["status"], **receipt["statistics"]}, sort_keys=True))


if __name__ == "__main__":
    main()
