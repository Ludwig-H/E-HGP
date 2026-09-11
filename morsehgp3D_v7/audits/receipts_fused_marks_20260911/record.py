"""Create-only bounded audit runner; retain every attempted command and failure."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--san", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    if Path(args.out).name != args.out or args.out in (".", "..", ""):
        parser.error("--out must be one new child directory name")
    target = root / args.out
    target.mkdir(exist_ok=False)
    source = root.parents[1] / "receipts/atlas_graph_full_20260911/objects/1cf438cbac839c8f502de0c8daebbf89296a3884d2d40ba50715b5aaca161fa2"
    paths = (source, root / "fused_marks.hpp", root / "gate.cpp", Path(__file__).resolve())
    def file_pin(path: Path) -> dict[str, object]:
        content = path.read_bytes()
        return {"path": str(path), "bytes": len(content), "sha256": hashlib.sha256(content).hexdigest()}

    def observed_pin(path: Path | None) -> dict[str, object] | None:
        if path is None:
            return None
        try:
            return file_pin(path)
        except OSError as error:
            return {"path": str(path), "error": repr(error)}

    pins = {}
    environment = dict(os.environ)
    (root / "tmp").mkdir(exist_ok=True)
    environment["TMPDIR"] = str(root / "tmp")
    if args.san:
        environment["ASAN_OPTIONS"] = "detect_leaks=1:abort_on_error=1"
        environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    records = []
    status = "running"
    failure = None
    compiler = None
    compiler_before = compiler_after = None
    executable = target / "gate"
    executable_after_compile = executable_after_tests = None

    def command(label: str, argv: list[str], expected: int = 0, timeout: int = 30) -> bytes:
        process = None
        stdout = stderr = b""
        error_text = None
        timed_out = False
        try:
            process = subprocess.Popen(argv, cwd=root, env=environment, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, start_new_session=True)
            stdout, stderr = process.communicate(timeout=timeout)
        except BaseException as error:
            error_text = repr(error)
            timed_out = isinstance(error, subprocess.TimeoutExpired)
            if process is not None:
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                try:
                    stdout, stderr = process.communicate(timeout=5)
                except subprocess.TimeoutExpired as cleanup:
                    stdout, stderr = cleanup.output or b"", cleanup.stderr or b""
                    error_text += "; process-group cleanup exceeded five seconds"
        returncode = None if process is None else process.returncode
        (target / (label + ".stdout")).write_bytes(stdout)
        (target / (label + ".stderr")).write_bytes(stderr)
        records.append({"label": label, "argv": argv, "expected_exit": expected,
                        "exit": returncode, "timeout_seconds": timeout,
                        "timed_out": timed_out, "execution_error": error_text,
                        "stdout_sha256": hashlib.sha256(stdout).hexdigest(),
                        "stderr_sha256": hashlib.sha256(stderr).hexdigest()})
        (target / "commands.json").write_text(json.dumps(records, indent=2) + "\n")
        if error_text is not None or returncode != expected:
            raise RuntimeError(f"{label}: exit {returncode}, expected {expected}; error={error_text}")
        return stdout

    try:
        pins = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
        if pins[str(source)] != source.name:
            raise RuntimeError("pinned calendar mismatch")
        compiler_path = shutil.which("g++")
        if compiler_path is None:
            raise RuntimeError("compiler not found")
        compiler = Path(compiler_path).resolve(strict=True)
        compiler_before = file_pin(compiler)
        command("compiler", [str(compiler), "--version"], timeout=10)
        flags = ["-O1", "-g0", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie"] if args.san else ["-O2"]
        command("compile", [str(compiler), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", *flags,
                            '-DMHGP7_AUDIT_CALENDAR_HEADER="' + str(source) + '"',
                            str(root / "gate.cpp"), "-o", str(executable)], timeout=120)
        executable_after_compile = file_pin(executable)
        output = command("selftest", [str(executable), "--selftest"])
        parsed = json.loads(output)
        if parsed.get("status") != "passed_bounded_fused_marks_model":
            raise RuntimeError("unqualified selftest status")
        for label, suffix in (("missing", []), ("unknown", ["--unknown"])):
            if command(label, [str(executable), *suffix], 2, timeout=10):
                raise RuntimeError("CLI rejection wrote stdout")
        if any(path.read_bytes() and path.name != "compiler.stderr" for path in target.glob("*.stderr")):
            raise RuntimeError("unexpected stderr")
        if any(hashlib.sha256(path.read_bytes()).hexdigest() != pins[str(path)] for path in paths):
            raise RuntimeError("source changed during qualification")
        compiler_after = file_pin(compiler)
        executable_after_tests = file_pin(executable)
        if compiler_before != compiler_after or executable_after_compile != executable_after_tests:
            raise RuntimeError("compiler or executable changed during qualification")
        status = "passed"
        print(output.decode(), end="")
        return 0
    except BaseException as error:
        failure = repr(error)
        raise
    finally:
        if compiler_after is None:
            compiler_after = observed_pin(compiler)
        if executable_after_tests is None:
            executable_after_tests = observed_pin(executable)
        (target / "context.json").write_text(json.dumps({"status": status if status == "passed" else "failed",
            "failure": failure, "sanitizers": args.san, "source_pins": pins,
            "compiler_before": compiler_before, "compiler_after": compiler_after,
            "executable_after_compile": executable_after_compile, "executable_after_tests": executable_after_tests,
            "environment": {key: environment[key] for key in ("TMPDIR", "ASAN_OPTIONS", "UBSAN_OPTIONS") if key in environment},
            "scope": "bounded synthetic fixtures; pinned sources/compiler/ELF, system headers and libraries are not hermetically pinned; no timing claim"}, indent=2) + "\n")


if __name__ == "__main__":
    sys.exit(main())
