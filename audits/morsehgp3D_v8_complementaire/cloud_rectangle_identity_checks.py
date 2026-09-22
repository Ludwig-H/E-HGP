#!/usr/bin/env python3
"""Current C++ APIs plus explicit false migration models on 3/4/5 sites."""
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
SOURCES = ["core/types.hpp", "spindle/predicates.hpp", "spindle/q2_prepared_bounds.hpp",
           "pipeline/local_credits.hpp", "pipeline/local_credits.cpp", "pipeline/tube_credits.hpp",
           "pipeline/axis_q2.hpp", "pipeline/axis_q2.cpp", "pipeline/q2_census.hpp",
           "pipeline/q2_census.cpp"]


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
        old = json.loads(args.replay.read_text())
        snapshot = old["snapshot_utf8"]
        capture_head = old["capture_head"]
        require({name: digest(text.encode()) for name, text in snapshot.items()} == old["source_sha256"],
                "snapshot hash mismatch")
    else:
        paths = {name: ROOT / "morsehgp3D_v8/src" / name for name in SOURCES}
        paths["probe.cpp"] = HERE / "cloud_rectangle_identity_probe.cpp"
        raw = {name: path.read_bytes() for name, path in paths.items()}
        require(all(path.read_bytes() == raw[name] for name, path in paths.items()),
                "sources changed while capturing")
        snapshot = {name: text.decode() for name, text in raw.items()}
        head = command(["git", "rev-parse", "HEAD"])
        require(head["exit_code"] == 0, "capture head unavailable")
        capture_head = head["stdout"].strip()
    require(set(snapshot) == set(SOURCES + ["probe.cpp"]), "unexpected source manifest")
    compiler = shutil.which("g++")
    require(compiler is not None, "compiler unavailable")
    version = command([compiler, "--version"])
    require(version["exit_code"] == 0, "compiler identification failed")
    receipt = {
        "schema": "mhgp8_cloud_rectangle_identity_v1",
        "scope": "unchanged current APIs and deliberately false migration models; no CloudOwner implementation",
        "phase": "exploration_v8_hors_registre", "backend": "cpu_reference",
        "profile": "quantized_u16_input_only", "public_status": "not_claimed", "gcp_used": False,
        "source_state": "private working-tree snapshot", "capture_head": capture_head,
        "source_sha256": {name: digest(text.encode()) for name, text in snapshot.items()},
        "snapshot_utf8": snapshot, "python_optimized": not __debug__,
        "runner_sha256": digest(Path(__file__).read_bytes()), "compiler": version,
        "compiler_sha256": digest(Path(compiler).read_bytes()), "commands": [], "status": "failed",
    }
    try:
        with tempfile.TemporaryDirectory(prefix="mhgp8_cloud_identity_") as directory:
            build = Path(directory)
            for name, text in snapshot.items():
                path = build / "src" / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text)
            binary = build / "probe"
            run = command([compiler, "-std=c++20", "-O1", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                           "-fsanitize=undefined", "-fno-sanitize-recover=all", "-I" + str(build / "src"),
                           str(build / "src/probe.cpp"), str(build / "src/pipeline/local_credits.cpp"),
                           str(build / "src/pipeline/axis_q2.cpp"), str(build / "src/pipeline/q2_census.cpp"),
                           "-o", str(binary)])
            receipt["commands"].append(run)
            require(run["exit_code"] == 0, "identity probe compilation failed")
            receipt["binary_sha256"] = digest(binary.read_bytes())
            run = command([str(binary)])
            receipt["commands"].append(run)
            require(run["exit_code"] == 0, "identity probe failed")
            stats = json.loads(run["stdout"])
            require(stats["census_runs"] == 20 and stats["payloads_checked"] == 34 and
                    stats["current_owner_guard_rejections"] == 5 and
                    stats["false_rectangle_models_refuted"] == 3 and
                    stats["false_threshold_model_refuted"] is True and
                    stats["false_permutation_model_refuted"] is True,
                    "identity gate is vacuous")
            receipt["statistics"] = stats
        if not args.replay:
            receipt["source_sha256_after"] = {name: digest((ROOT / "morsehgp3D_v8/src" / name).read_bytes())
                                               for name in SOURCES}
            receipt["sources_unchanged_at_closing"] = all(receipt["source_sha256_after"][name] ==
                                                          receipt["source_sha256"][name] for name in SOURCES)
        receipt["status"] = "passed"
    finally:
        args.output.write_text(json.dumps(receipt, ensure_ascii=False, sort_keys=True, indent=2) + "\n")
    print(json.dumps({"status": receipt["status"], **receipt["statistics"]}, sort_keys=True))


if __name__ == "__main__":
    main()
