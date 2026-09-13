#!/usr/bin/env python3
"""Bounded range-composition audit; product sources pinned to published v8."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

COMMIT = "8e406f9b"
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PRODUCT = [
    "core/types.hpp", "spindle/predicates.hpp", "pipeline/local_credits.hpp",
    "pipeline/local_credits.cpp", "pipeline/tube_credits.hpp",
    "pipeline/axis_q2.hpp", "pipeline/axis_q2.cpp",
]


def require(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


def sha(text: str) -> str:
    return hashlib.sha256(text.encode()).hexdigest()


def command(args: list[str]) -> dict:
    run = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, check=False)
    return {"argv": args, "exit_code": run.returncode,
            "stdout": run.stdout, "stderr": run.stderr}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--replay", type=Path)
    args = parser.parse_args()
    if args.replay:
        previous = json.loads(args.replay.read_text())
        snapshot = previous["snapshot_utf8"]
        source_commit = previous["source_commit"]
        for name, text in snapshot.items():
            require(sha(text) == previous["source_sha256"][name], "invalid snapshot hash")
    else:
        commit = command(["git", "rev-parse", COMMIT])
        require(commit["exit_code"] == 0, "cannot resolve source commit")
        source_commit = commit["stdout"].strip()
        snapshot = {}
        for name in PRODUCT:
            path = "morsehgp3D_v8/src/" + name
            source = command(["git", "show", source_commit + ":" + path])
            require(source["exit_code"] == 0, "cannot read pinned source")
            snapshot[name] = source["stdout"]
        snapshot["probe.cpp"] = (HERE / "residual_intersection_probe.cpp").read_text()
    require(set(snapshot) == set(PRODUCT + ["probe.cpp"]), "unexpected source manifest")
    sources = {name: sha(text) for name, text in snapshot.items()}
    compiler = command(["g++", "--version"])
    require(compiler["exit_code"] == 0, "compiler unavailable")
    receipt = {
        "schema": "mhgp8_residual_intersection_audit_v1",
        "scope": "exact set intersection of two existing q2 residuals, no census",
        "phase": "exploration_v8_hors_registre", "backend": "cpu_reference",
        "profile": "quantized_u16_input_only", "public_status": "not_claimed",
        "source_commit": source_commit, "source_sha256": sources,
        "snapshot_utf8": snapshot, "compiler": compiler["stdout"].splitlines()[0],
        "runner_sha256": sha(Path(__file__).read_text()),
        "python_optimized": not __debug__, "gcp_used": False, "commands": [],
    }
    flags = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic",
             "-Werror", "-fsanitize=undefined", "-fno-sanitize-recover=all"]
    mutations = {
        "positive": None,
        "strict_boundary": ("if (value < t) {  // MUTATION_STRICT_BOUNDARY",
                            "if (value <= t) {  // MUTATION_STRICT_BOUNDARY"),
        "confuse_original_id_and_rank": ("credit.b_credits()[id - b.first]",
                                         "credit.b_credits()[i]"),
    }
    results = {}
    with tempfile.TemporaryDirectory(prefix="mhgp8_residual_intersection_") as directory:
        build = Path(directory)
        for name in PRODUCT:
            target = build / "src" / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(snapshot[name])
        objects = []
        for unit in ("pipeline/local_credits.cpp", "pipeline/axis_q2.cpp"):
            obj = build / (Path(unit).stem + ".o")
            run = command(flags + ["-I" + str(build / "src"), "-c",
                                   str(build / "src" / unit), "-o", str(obj)])
            receipt["commands"].append(run)
            require(run["exit_code"] == 0, "product compilation failed")
            objects.append(str(obj))
        for name, mutation in mutations.items():
            text = snapshot["probe.cpp"]
            if mutation is not None:
                require(text.count(mutation[0]) == 1, "mutation target is not unique")
                text = text.replace(*mutation)
            source = build / (name + ".cpp")
            binary = build / name
            source.write_text(text)
            compile_run = command(flags + ["-I" + str(build / "src"), str(source)] +
                                  objects + ["-o", str(binary)])
            receipt["commands"].append(compile_run)
            require(compile_run["exit_code"] == 0, "probe compilation failed: " + name)
            result = command([str(binary)])
            receipt["commands"].append(result)
            results[name] = {**result, "probe_sha256": sha(text),
                             "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest()}
            if name == "positive":
                require(result["exit_code"] == 0, "positive composition failed")
                stats = json.loads(result["stdout"])
                require(stats["plans"] == 48 and stats["pairs_checked"] == 172800 and
                        stats["improvements_both"] > 0 and stats["empty_plans"] > 0 and
                        stats["invalid_combinations_rejected"] == 5,
                        "non-vacuity gate failed")
                receipt["statistics"] = stats
            else:
                require(result["exit_code"] == 1 and
                        result["stderr"] == "residual intersection audit: "
                        "intersection differs from conjunction\n", "mutant not causally rejected")
    receipt["results"] = results
    receipt["status"] = "passed"
    args.output.write_text(json.dumps(receipt, ensure_ascii=False, sort_keys=True, indent=2) + "\n")
    print(json.dumps({"status": "passed", **receipt["statistics"],
                      "mutants_rejected": len(mutations) - 1}, sort_keys=True))


if __name__ == "__main__":
    main()
