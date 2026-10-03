"""Build the audit point exporter only inside the existing guarded G4 worker.

CLI: --source REPO_ROOT --build FRESH_DIR --out FRESH_DIR.  This wrapper
compiles a CPU/u21 audit consumer; it runs no native test or point experiment.
Worker variables are an invocation guard, not independent hardware evidence.
The session controller remains responsible for the G4 target and its closure.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import time

TIMEOUT_SECONDS = 300
PROFILE = "quantized_u21_input_only"
FLAGS = ["-std=c++20", "-O3", "-DNDEBUG", "-DMHGP11_COORD_BITS=21",
         "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread"]


class BuildFailure(RuntimeError):
    def __init__(self, reason, code=1):
        super().__init__(reason)
        self.code = code


def require(condition, reason):
    if not condition:
        raise BuildFailure(reason, 2)


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        while block := stream.read(1024 * 1024):
            result.update(block)
    return result.hexdigest()


def describe(path):
    require(path.is_file() and not path.is_symlink(), "nonregular_source:" + str(path))
    return dict(bytes=path.stat().st_size, sha256=digest(path))


def snapshot(repo, module, probe, wrapper):
    files = {module / "CMakeLists.txt", module / "bench" / "whole_input.hpp", probe, wrapper}
    for directory in (module / "src", module / "cmake"):
        require(directory.is_dir() and not directory.is_symlink(), "missing_source_directory:" + str(directory))
        files.update(path for path in directory.rglob("*") if not path.is_dir())
    return {str(path.relative_to(repo)): describe(path) for path in sorted(files)}


def fresh_directory(path, source):
    require(not path.is_symlink(), "directory_symlink:" + str(path))
    path = path.resolve()
    require(not path.is_relative_to(source), "build_or_output_inside_source:" + str(path))
    require(not path.exists() or path.is_dir() and not any(path.iterdir()),
            "directory_not_empty:" + str(path))
    path.mkdir(parents=True, exist_ok=True)
    return path


def close_group(process):
    # A successful leader may leave descendants. Close its private group too;
    # do not rely on leader returncode alone as a quiescence certificate.
    attempted = []
    for sig in (signal.SIGTERM, signal.SIGKILL):
        try:
            os.killpg(process.pid, sig)
            attempted.append(sig.name)
        except ProcessLookupError:
            break
    if process.poll() is None:
        process.wait(timeout=5)
    return attempted


def run_phase(name, argv, cwd, output, deadline, phases):
    record = dict(name=name, argv=[str(x) for x in argv], cwd=str(cwd),
                  status="intent", started_unix_ns=time.time_ns())
    phases.append(record)
    command_path = output / (name + ".command.json")
    command_path.write_text(json.dumps(record, sort_keys=True, indent=2) + "\n")
    started = time.monotonic()
    process = None
    try:
        remaining = deadline - started
        if remaining <= 0:
            raise BuildFailure("build_total_timeout", 124)
        with (output / (name + ".stdout")).open("wb") as stdout, \
                (output / (name + ".stderr")).open("wb") as stderr:
            process = subprocess.Popen(record["argv"], cwd=cwd, stdout=stdout,
                                       stderr=stderr, start_new_session=True)
            record.update(status="running", pid=process.pid)
            command_path.write_text(json.dumps(record, sort_keys=True, indent=2) + "\n")
            try:
                record["returncode"] = process.wait(timeout=remaining)
            except subprocess.TimeoutExpired as error:
                record["status"] = "timeout"
                raise BuildFailure("build_total_timeout:" + name, 124) from error
            record["status"] = "ok" if process.returncode == 0 else "failed"
            if process.returncode != 0:
                raise BuildFailure("build_phase_failed:" + name)
    except OSError as error:
        record.update(status="launch_failed", error=type(error).__name__ + ":" + str(error))
        raise BuildFailure("build_phase_launch_failed:" + name) from error
    except BuildFailure:
        if record["status"] in ("intent", "running"):
            record["status"] = "interrupted_or_refused"
        raise
    finally:
        if process is not None:
            record["group_close_signals"] = close_group(process)
            record["returncode"] = process.returncode
        record["wall_seconds"] = time.monotonic() - started
        command_path.write_text(json.dumps(record, sort_keys=True, indent=2) + "\n")


def interrupted(signum, _frame):
    raise BuildFailure("build_interrupted:" + signal.Signals(signum).name, 128 + signum)


def main():
    started = time.monotonic()
    deadline = started + TIMEOUT_SECONDS
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--build", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    output, manifest = None, None
    code = 0
    try:
        require(not args.source.is_symlink(), "source_directory_symlink")
        repo = args.source.resolve()
        worker_source = os.environ.get("V11_SRC", "")
        pin = os.environ.get("V11_SOURCE_PIN", "")
        package_sha = os.environ.get("V11_PACKAGE_SHA256", "")
        generation = os.environ.get("V11_GENERATION", "")
        source_pin = re.fullmatch(r"commit:([0-9a-f]{40})", pin)
        require(worker_source and Path(worker_source).resolve() == repo
                and source_pin is not None
                and re.fullmatch(r"[0-9a-f]{64}", package_sha) and generation,
                "guarded_v11_worker_context_required")
        module = repo / "morsehgp3D_v11"
        wrapper = Path(__file__).resolve()
        require(wrapper.is_relative_to(module), "wrapper_outside_pinned_source")
        probe = wrapper.with_name("points_probe.cpp")
        before = snapshot(repo, module, probe, wrapper)
        require(args.build.resolve() != args.out.resolve()
                and not args.build.resolve().is_relative_to(args.out.resolve())
                and not args.out.resolve().is_relative_to(args.build.resolve()),
                "build_output_directories_overlap")
        build = fresh_directory(args.build, repo)
        output = fresh_directory(args.out, repo)
        cmake_name, compiler_name = shutil.which("cmake"), shutil.which("g++")
        require(cmake_name and compiler_name, "cmake_or_gxx_absent")
        cmake, compiler = Path(cmake_name).resolve(), Path(compiler_name).resolve()
        source_kind, source_hash = pin.split(":")
        manifest = dict(schema="mhgp11.full_points.probe_build.v1", status="building",
                        source_pin=pin, source_kind=source_kind, source_hash=source_hash,
                        source_commit=source_hash if source_kind == "commit" else None,
                        source_package_sha256=package_sha,
                        generation=generation, worker_context="invocation guard only; hardware/closure in session receipt",
                        backend="cpu_reference", profile=PROFILE, coord_bits=21,
                        public_status="not_claimed", source_root=str(repo), build_root=str(build),
                        timeout_seconds=TIMEOUT_SECONDS, jobs=16,
                        flags=FLAGS, compiler_path=str(compiler), compiler_binary=describe(compiler),
                        sources_before=before, phases=[])
        signal.signal(signal.SIGTERM, interrupted)
        signal.signal(signal.SIGINT, interrupted)
        try:
            run_phase("compiler_version", [compiler, "--version"], repo, output,
                      deadline, manifest["phases"])
            run_phase("configure", [cmake, "-S", module, "-B", build,
                      "-DCMAKE_BUILD_TYPE=Release", "-DMHGP11_MODULES=tower",
                      "-DMHGP11_COORD_BITS=21", "-DBUILD_TESTING=OFF",
                      "-DMHGP11_SANITIZE=OFF", "-DMHGP11_TSAN=OFF", "-DMHGP11_POISON=OFF",
                      "-DMHGP11_MARCH=", "-DCMAKE_CXX_COMPILER=" + str(compiler),
                      "-DCMAKE_CXX_FLAGS=", "-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG",
                      "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"], repo, output,
                      deadline, manifest["phases"])
            run_phase("library", [cmake, "--build", build, "--target", "mhgp11", "-j16"],
                      repo, output, deadline, manifest["phases"])
            library = build / "libmhgp11.a"
            manifest["library"] = dict(path=str(library), **describe(library))
            run_phase("probe", [compiler, *FLAGS, "-I" + str(module / "src"),
                      "-I" + str(module / "bench"), probe, library,
                      "-o", output / "points_probe"], repo, output, deadline, manifest["phases"])
            manifest["binary"] = dict(name="points_probe", **describe(output / "points_probe"))
            manifest["status"] = "built"
        finally:
            manifest["sources_after"] = snapshot(repo, module, probe, wrapper)
            manifest["sources_unchanged"] = manifest["sources_after"] == before
            manifest["build_wall_seconds"] = time.monotonic() - started
            require(manifest["sources_unchanged"], "source_drift_during_build")
            for name in ("CMakeCache.txt", "compile_commands.json"):
                path = build / name
                if path.is_file():
                    shutil.copyfile(path, output / name)
    except (BuildFailure, OSError, ValueError) as error:
        code = error.code if isinstance(error, BuildFailure) else 1
        if manifest is not None:
            manifest.update(status="failed", failure=str(error), exit_code=code)
        print(json.dumps(dict(status="refused_or_failed", reason=str(error), exit_code=code)), file=sys.stderr)
    finally:
        if output is not None and manifest is not None:
            manifest.setdefault("exit_code", code)
            payloads = {p.name: describe(p) for p in sorted(output.iterdir()) if p.is_file()}
            manifest["payloads"] = payloads
            pending = output / "manifest.json.pending"
            pending.write_text(json.dumps(manifest, sort_keys=True, indent=2, allow_nan=False) + "\n")
            pending.replace(output / "manifest.json")
    return code


if __name__ == "__main__":
    raise SystemExit(main())
