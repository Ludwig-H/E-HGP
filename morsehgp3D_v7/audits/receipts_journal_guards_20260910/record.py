"""Build/replay bounded guard probes, preserving each attempt independently."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
WORK = BASE / ".work_build"
SOURCE = BASE / "snapshot/morsehgp3D_v7/src/forest/full_coverage_certificate.hpp"


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path: Path, value: object) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def inputs() -> dict:
    paths = list((BASE / "snapshot").rglob("*.hpp"))
    paths += [BASE / "guard_probe.cpp", BASE / "baseline_full_coverage_certificate.hpp",
              BASE / "mutations.json", BASE / "record.py"]
    return {p.relative_to(BASE).as_posix(): sha(p) for p in sorted(paths)}


class Capture:
    def __init__(self, path: Path):
        path.mkdir()
        self.path = path
        self.commands = []

    def run(self, name: str, argv: list[str], sanitizer: bool = False) -> int:
        env = os.environ.copy()
        delta = {}
        if sanitizer:
            delta = dict(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                         UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
            env.update(delta)
        started = time.time_ns()
        with (self.path / (name + ".stdout")).open("wb") as out:
            with (self.path / (name + ".stderr")).open("wb") as err:
                result = subprocess.run(argv, cwd=ROOT, env=env, stdout=out, stderr=err, check=False)
        self.commands.append(dict(name=name, argv=argv, environment_delta=delta,
                                  started_ns=started, ended_ns=time.time_ns(), exit_code=result.returncode))
        write(self.path / "commands.json", self.commands)
        print(name, result.returncode, flush=True)
        return result.returncode


def build() -> None:
    WORK.mkdir()
    capture = Capture(BASE / "build_capture")
    before = inputs()
    write(capture.path / "sources_before.json", before)
    capture.run("compiler", ["g++", "--version"])
    mutations = json.loads((BASE / "mutations.json").read_text())
    variants = dict(nominal_o2=SOURCE, nominal_san=SOURCE,
                    baseline_o2=BASE / "baseline_full_coverage_certificate.hpp")
    text = SOURCE.read_text()
    for name, delta in mutations.items():
        if text.count(delta["before"]) != 1:
            raise ValueError("mutation is not unique: " + name)
        private = WORK / (name + ".hpp")
        private.write_text(text.replace(delta["before"], delta["after"]))
        variants[name] = private
    binaries = {}
    header_pins = {}
    codes = []
    for name, header in variants.items():
        flags = ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if name.endswith("_san") else ["-O2"]
        target = WORK / name
        argv = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", *flags,
                "-I", str(SOURCE.parent), '-DMHGP7_AUDIT_HEADER="' + str(header) + '"',
                "-MMD", "-MF", str(capture.path / (name + ".d")), str(BASE / "guard_probe.cpp"),
                "-o", str(target)]
        header_pins[name] = sha(header)
        code = capture.run("compile_" + name, argv)
        codes.append(code)
        if code:
            break
        binaries[name] = sha(target)
        if sha(header) != header_pins[name]:
            raise ValueError("private header changed during compile")
    after = inputs()
    write(capture.path / "sources_after.json", after)
    ok = before == after and not any(codes) and len(binaries) == len(variants)
    write(capture.path / "receipt.json", dict(status="passed" if ok else "failed",
          sources_stable=before == after, binaries=binaries, headers=header_pins,
          engine_executed=False, gcp_used=False))
    if not ok:
        raise SystemExit(1)


def test(attempt: str) -> None:
    if not attempt.isalnum():
        raise ValueError("attempt must be alphanumeric")
    capture = Capture(BASE / ("test_" + attempt))
    built = json.loads((BASE / "build_capture/receipt.json").read_text())
    if built["status"] != "passed":
        raise ValueError("no complete build")
    before = inputs()
    write(capture.path / "sources_before.json", before)
    actual = {}
    ok = True
    for name, pin in built["binaries"].items():
        binary = WORK / name
        if sha(binary) != pin:
            raise ValueError("binary drift before " + name)
        code = capture.run(name, [str(binary)], sanitizer=name.endswith("_san"))
        actual[name] = code
        ok = ok and code == (0 if name.endswith(("_o2", "_san")) else 1)
        if sha(binary) != pin:
            raise ValueError("binary drift after " + name)
    after = inputs()
    write(capture.path / "sources_after.json", after)
    ok = ok and before == after
    write(capture.path / "receipt.json", dict(status="passed" if ok else "failed",
          sources_stable=before == after, binaries=built["binaries"], exits=actual,
          engine_executed=False, gcp_used=False))
    if not ok:
        raise SystemExit(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("build", "test"))
    parser.add_argument("--attempt", default="r1")
    args = parser.parse_args()
    build() if args.mode == "build" else test(args.attempt)
