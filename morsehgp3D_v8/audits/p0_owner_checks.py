#!/usr/bin/env python3
"""Capture/replay the bounded P0 owner observation and its local correction."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import tempfile
import time

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
SOURCES = (
    "core/types.hpp", "spindle/predicates.hpp", "pipeline/local_credits.hpp",
    "pipeline/local_credits.cpp", "pipeline/tube_credits.hpp",
)
HEADER = "pipeline/local_credits.hpp"
OPEN_HASH = "791dfe12079278e31904b02552dbc7d852ec6ddcc610e9ab4d1489ccb5df6e65"
ANCHOR = "  PreparedRectangle() = default;"
PATCH = ANCHOR + "\n" + "\n".join((
    "  PreparedRectangle(const PreparedRectangle&) = delete;",
    "  PreparedRectangle(PreparedRectangle&&) = delete;",
    "  PreparedRectangle& operator=(const PreparedRectangle&) = delete;",
    "  PreparedRectangle& operator=(PreparedRectangle&&) = delete;",
))


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def pin(path: Path) -> dict:
    raw = path.read_bytes()
    return {"path": str(path), "sha256": digest(raw), "bytes": len(raw)}


def snapshot(path: Path) -> dict:
    raw = path.read_bytes()
    return {"path": str(path.relative_to(ROOT)), "sha256": digest(raw),
            "text": raw.decode("utf-8")}


def require(condition: bool, cause: str) -> None:
    if not condition:
        raise RuntimeError(cause)


def command(argv: list[str], timeout: int, env: dict, executable: Path) -> dict:
    before = pin(executable)
    started = time.monotonic()
    process = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               env=env, start_new_session=True)
    expired = False
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        expired = True
        os.killpg(process.pid, signal.SIGKILL)
        stdout, stderr = process.communicate()
    return {"argv": argv, "timeout_seconds": timeout, "timed_out": expired,
            "exit_code": process.returncode, "elapsed_seconds": time.monotonic() - started,
            "stdout": stdout.decode("utf-8", errors="replace"),
            "stderr": stderr.decode("utf-8", errors="replace"),
            "stdout_sha256": digest(stdout), "stderr_sha256": digest(stderr),
            "executable_before": before, "executable_after": pin(executable)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--snapshot", type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    require(output.parent == BASE and not output.exists(), "owner.output_create_only_in_audits")
    receipt = {"status": "failed", "full_qualification": False, "performance_claim": False,
               "gcp_used": False, "scope": "P0 owner API, two four-point q2 fixtures",
               "commands": [], "runs": [], "runner": snapshot(Path(__file__).resolve()),
               "replay_source": str(args.snapshot) if args.snapshot else None}
    workspace = Path(tempfile.mkdtemp(prefix=".work_owner_", dir=BASE))
    try:
        old = json.loads(args.snapshot.read_text()) if args.snapshot else None
        receipt["sources"] = old["sources"] if old else {
            name: snapshot(ROOT / "morsehgp3D_v8/src" / name) for name in SOURCES}
        receipt["gate"] = old["gate"] if old else snapshot(BASE / "p0_owner_gate.cpp")
        require(set(receipt["sources"]) == set(SOURCES), "owner.source_set")
        for item in [*receipt["sources"].values(), receipt["gate"]]:
            require(digest(item["text"].encode()) == item["sha256"], "owner.snapshot_hash")
        header = receipt["sources"][HEADER]
        require(header["sha256"] == OPEN_HASH, "owner.open_header_changed")
        require(header["text"].count(ANCHOR) == 1, "owner.patch_anchor")
        receipt["local_patch"] = {"path": HEADER, "old": ANCHOR, "new": PATCH}
        compiler = Path(shutil.which("g++") or "missing-g++").resolve()
        env = dict(os.environ, TMPDIR=str(workspace))
        version = command([str(compiler), "--version"], 10, env, compiler)
        receipt["commands"].append(version)
        require(version["exit_code"] == 0, "owner.compiler_version")
        for mode in ("o2", "san"):
            for variant in ("original", "closed"):
                folder = workspace / (mode + "_" + variant)
                source_pins = {}
                for name, item in receipt["sources"].items():
                    path = folder / "src" / name
                    path.parent.mkdir(parents=True, exist_ok=True)
                    text = item["text"]
                    if variant == "closed" and name == HEADER:
                        text = text.replace(ANCHOR, PATCH)
                    path.write_text(text)
                    source_pins[name] = pin(path)
                gate = folder / "gate.cpp"
                gate.write_text(receipt["gate"]["text"])
                source_pins["gate.cpp"] = pin(gate)
                binary = folder / "owner_gate"
                flags = ["-O2"] if mode == "o2" else ["-O1", "-g0",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie"]
                run = {"mode": mode, "variant": variant, "source_before": source_pins,
                       "commands": []}
                receipt["runs"].append(run)
                argv = [str(compiler), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", *flags, "-I", str(folder / "src"), str(gate),
                        str(folder / "src/pipeline/local_credits.cpp"), "-o", str(binary)]
                build = command(argv, 30, env, compiler)
                run["commands"].append(build)
                require(build["exit_code"] == 0 and not build["timed_out"], "owner.compile")
                run_env = dict(env)
                if mode == "san":
                    run_env.update(ASAN_OPTIONS="detect_leaks=1:abort_on_error=1",
                                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
                run["sanitizer_environment"] = {key: run_env[key] for key in (
                    "ASAN_OPTIONS", "UBSAN_OPTIONS") if key in run_env}
                for label, options, code in (("selftest", ["--selftest"], 0),
                                               ("missing", [], 2), ("unknown", ["--unknown"], 2)):
                    result = command([str(binary), *options], 10, run_env, binary)
                    result["label"] = label
                    run["commands"].append(result)
                    require(result["exit_code"] == code and not result["timed_out"]
                            and result["stderr"] == "", "owner.execution_" + label)
                    if label != "selftest":
                        require(result["stdout"] == "", "owner.cli_stdout")
                        continue
                    expected = {"nominal_pairs": [1, 4], "product_scope": "P0_only",
                                "full_qualification": False, "gcp_used": False}
                    expected.update({"status": "observed_owner_rebinding", "lost_q2_pairs": 3}
                                    if variant == "original" else {
                                        "status": "passed_owner_closure", "closed_special_members": 4})
                    require(json.loads(result["stdout"]) == expected, "owner.result")
                run["source_after"] = {name: pin(Path(item["path"]))
                                       for name, item in source_pins.items()}
                require(run["source_before"] == run["source_after"], "owner.source_changed")
        for item in receipt["commands"] + [c for r in receipt["runs"] for c in r["commands"]]:
            require(item["executable_before"] == item["executable_after"], "owner.executable_changed")
        receipt["status"] = "passed_bounded_owner_observation_and_closure"
    except Exception as error:
        receipt["error"] = str(error)
    finally:
        shutil.rmtree(workspace)
        receipt["temporary_workspace_removed"] = not workspace.exists()
        receipt["live_source_after"] = {name: pin(ROOT / "morsehgp3D_v8/src" / name)
                                        for name in SOURCES}
        with output.open("x") as stream:
            json.dump(receipt, stream, ensure_ascii=False, indent=2)
            stream.write("\n")
    print(json.dumps({"status": receipt["status"], "receipt": str(output)}))
    return 0 if receipt["status"].startswith("passed_") else 1


if __name__ == "__main__":
    raise SystemExit(main())
