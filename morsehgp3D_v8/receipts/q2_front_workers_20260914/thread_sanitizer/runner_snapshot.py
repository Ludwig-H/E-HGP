#!/usr/bin/env python3
"""Capture an isolated ThreadSanitizer attempt; never weaken its runtime checks."""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import resource
import signal
import subprocess
import sys
import tempfile
import time
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "morsehgp3D_v8"
BUILD = ROOT / "build/v8_front_workers_tsan_20260914"
DEST = SOURCE / "receipts/q2_front_workers_20260914/thread_sanitizer"
TARGET = "mhgp8_wspd_q2_parallel_gate"
COMPILER = Path("/usr/bin/clang++")
BOOST = ROOT / "build/v7_boost_gate/extracted/usr"
sys.path.insert(0, str(SOURCE / "bench"))
from run_p0_matrix import invoke, on_signal, utc_stamp  # noqa: E402


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path: Path, value: Any) -> None:
    with path.open("x", encoding="utf-8") as stream:
        json.dump(value, stream, indent=2, allow_nan=False)
        stream.write("\n")


def source_pins() -> dict[str, str]:
    paths = [Path(__file__).resolve(), SOURCE / "CMakeLists.txt"]
    for directory in ("src", "tests", "oracle", "bench"):
        paths.extend(path for path in (SOURCE / directory).rglob("*")
                     if path.is_file() and path.suffix in (".cpp", ".hpp", ".py"))
    return {str(path.relative_to(ROOT)): digest(path) for path in sorted(paths)}


def artifact_pins() -> dict[str, str]:
    paths = [BUILD / "CMakeCache.txt", BUILD / "Makefile", BUILD / TARGET,
             BUILD / "libmhgp8_p0.a"]
    for name in ("mhgp8_p0", TARGET):
        paths.extend(BUILD / f"CMakeFiles/{name}.dir/{part}"
                     for part in ("flags.make", "link.txt", "build.make"))
    return {str(path.relative_to(ROOT)): digest(path) for path in sorted(paths) if path.is_file()}


def capture(command: list[str], label: str, output: Path,
            environment: dict[str, str]) -> dict[str, Any]:
    record: dict[str, Any] = dict(command=command, cwd=str(ROOT), label=label,
        started_utc=utc_stamp(), exit_code=None, stdout="", stderr="",
        stdout_base64="", stderr_base64="", status="failed")
    start = time.monotonic()
    before = resource.getrusage(resource.RUSAGE_CHILDREN)
    print(json.dumps(dict(starting=label, command=command)), flush=True)
    try:
        invoke(command, environment, ROOT, record, new_session=True)
        record["status"] = "completed" if record["exit_code"] == 0 else "failed"
    except BaseException as error:
        record["error"] = repr(error)
        record["status"] = "interrupted" if isinstance(error, KeyboardInterrupt) else "failed"
        raise
    finally:
        after = resource.getrusage(resource.RUSAGE_CHILDREN)
        record.update(finished_utc=utc_stamp(), elapsed_seconds=time.monotonic() - start,
            child_user_seconds=after.ru_utime - before.ru_utime,
            child_system_seconds=after.ru_stime - before.ru_stime,
            child_maxrss_kib_cumulative=after.ru_maxrss)
        logs = {}
        for channel in ("stdout", "stderr"):
            path = output / f"{label}.{channel}.bin"
            raw = base64.b64decode(record[f"{channel}_base64"], validate=True)
            with path.open("xb") as stream:
                stream.write(raw)
            logs[channel] = dict(path=str(path.relative_to(ROOT)), sha256=digest(path), bytes=len(raw))
        record["logs"] = logs
        write_json(output / f"{label}.json", record)
        print(json.dumps(dict(finished=label, exit_code=record["exit_code"],
            elapsed_seconds=record["elapsed_seconds"], stdout=record["stdout"],
            stderr=record["stderr"])), flush=True)
    return record


def check_cache() -> str:
    cache = (BUILD / "CMakeCache.txt").read_text()
    lines = cache.splitlines()
    for required in ("CMAKE_BUILD_TYPE:STRING=Debug", "MHGP8_SANITIZE:BOOL=OFF",
                     "BUILD_TESTING:BOOL=ON",
                     "CMAKE_CXX_FLAGS:STRING=-fsanitize=thread -fno-omit-frame-pointer",
                     "CMAKE_EXE_LINKER_FLAGS:STRING=-fsanitize=thread -fno-omit-frame-pointer"):
        require(required in lines, f"unexpected CMake cache: missing {required}")
    require(any(line in ("CMAKE_CXX_COMPILER:STRING=/usr/bin/clang++",
                         "CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++") for line in lines),
            "unexpected C++ compiler")
    return cache


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=("prepare", "execute"))
    parser.add_argument("--execution-context", required=True,
                        choices=("workspace", "approved-escalation"))
    args = parser.parse_args()
    DEST.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=f"{args.operation}_{args.execution_context}_", dir=DEST))
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    environment = dict(os.environ, TSAN_OPTIONS="halt_on_error=1:exitcode=66",
                       OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
    start = time.monotonic()
    started = utc_stamp()
    before: dict[str, str] = {}
    pre_run: dict[str, str] = {}
    status, error, code = "failed", "attempt did not start", 1
    gate_result = None
    try:
        before = source_pins()
        write_json(output / "INVOCATION.json", dict(
            schema="mhgp8_q2_front_workers_thread_sanitizer_attempt_v1",
            started_utc=started, command=[sys.executable, *sys.argv], cwd=str(ROOT),
            operation=args.operation, execution_context=args.execution_context,
            source_sha256=before, artifact_before_sha256=artifact_pins(),
            compiler=dict(path=str(COMPILER), resolved_path=str(COMPILER.resolve()),
                          sha256=digest(COMPILER)),
            commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
            worktree_status=subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True),
            environment={name: environment.get(name) for name in
                         ("TSAN_OPTIONS", "ASAN_OPTIONS", "LSAN_OPTIONS", "UBSAN_OPTIONS",
                          "OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS")},
            scope="only_bounded_cpu_q2_parallel_gate_not_full_or_gpu",
            public_status="not_claimed", gcp_used=False, cloud_cost=0,
            full_contract_qualified=False, fifty_thousand_full_contract_qualified=False,
            gpu_qualified=False, address_space_controls_changed=False))
        version = capture([str(COMPILER), "--version"], "compiler_version", output, environment)
        require(version["exit_code"] == 0, "compiler version failed")
        if args.operation == "prepare":
            require(not (BUILD / "CMakeCache.txt").exists(), "refusing to reconfigure an existing build")
            configure = capture(["cmake", "-S", str(SOURCE), "-B", str(BUILD),
                "-DCMAKE_BUILD_TYPE=Debug", f"-DCMAKE_CXX_COMPILER={COMPILER}",
                "-DCMAKE_CXX_FLAGS=-fsanitize=thread -fno-omit-frame-pointer",
                "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=thread -fno-omit-frame-pointer",
                "-DMHGP8_SANITIZE=OFF", "-DBUILD_TESTING=ON",
                f"-DBoost_DIR={BOOST / 'lib/x86_64-linux-gnu/cmake/Boost-1.83.0'}",
                f"-DBoost_INCLUDE_DIR={BOOST / 'include'}"], "configure", output, environment)
            require(configure["exit_code"] == 0, "configuration failed")
            check_cache()
            build = capture(["cmake", "--build", str(BUILD), "--target", TARGET,
                             "--parallel", "2"], "build", output, environment)
            require(build["exit_code"] == 0, "target build failed")
        cache = check_cache()
        require((BUILD / TARGET).is_file(), "missing target binary")
        pre_run = artifact_pins()
        write_json(output / "PRE_EXECUTION.json", dict(artifact_sha256=pre_run, cmake_cache=cache))
        execution = capture([str(BUILD / TARGET), "--selftest"], "gate", output, environment)
        combined = execution["stdout"] + execution["stderr"]
        unsupported = any(marker in combined for marker in
                          ("unexpected memory mapping", "ADDR_NO_RANDOMIZE", "personality("))
        if execution["exit_code"] != 0 and unsupported:
            status, error = "runtime_unavailable", "ThreadSanitizer address-space runtime initialization failed"
        else:
            require(execution["exit_code"] == 0, f"gate exit {execution['exit_code']}")
            require("ThreadSanitizer:" not in combined, "unexpected ThreadSanitizer diagnostic")
            gate_result = json.loads(execution["stdout"])
            require(gate_result.get("schema") == "mhgp8_wspd_q2_parallel_gate_v1" and
                    gate_result.get("status") == "passed", "missing exact gate success record")
            status, error, code = "passed", None, 0
    except BaseException as cause:
        status, error = ("interrupted" if isinstance(cause, KeyboardInterrupt) else "failed"), repr(cause)
        code = 130 if isinstance(cause, KeyboardInterrupt) else 1
    finally:
        after = source_pins()
        artifacts = artifact_pins()
        stable_sources = bool(before) and before == after
        stable_execution = not pre_run or pre_run == artifacts
        if not stable_sources or not stable_execution:
            status, error, code = "invalid", "source or execution artifact changed during attempt", 1
        completion = dict(status=status, error=error, exit_code=code,
            started_utc=started, finished_utc=utc_stamp(), elapsed_seconds=time.monotonic() - start,
            source_before_sha256=before, source_after_sha256=after, source_pins_unchanged=stable_sources,
            artifact_after_sha256=artifacts, execution_artifacts_unchanged=stable_execution,
            gate_result=gate_result, thread_sanitizer_qualified=status == "passed",
            public_status="not_claimed", gcp_used=False, cloud_cost=0,
            receipt_sha256={path.name: digest(path) for path in sorted(output.iterdir()) if path.is_file()})
        write_json(output / "COMPLETION.json", completion)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(receipt=str(output.relative_to(ROOT)), **completion)), flush=True)
    return code


if __name__ == "__main__":
    raise SystemExit(main())
