#!/usr/bin/env python3
"""Prepared, create-only targeted FULL lazy qualification; ROOT executes only.

No old result is imported. The LastTest grammar is an explicit standalone port
from build/v7_next_q2_tests_20260905/qualification.py, SHA256
5f62b29d96895e9b46df591f0540dd5d4ecbd3fec4a8eb291e4e6d3c27fb7a60.
This is a fresh local targeted qualification, not a hermetic build, whole-suite
qualification, performance result, or public exactness promotion.
"""
from __future__ import annotations

import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import re
import resource
import shlex
import signal
import stat
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

ROOT = Path("/workspaces/E-HGP")
BASE = ROOT / "build/v7_full_lazy_20260905_controller"
OUT = BASE / "runs"
SOURCE = ROOT / "morsehgp3D_v7"
BOOST = ROOT / "build/v7_boost_gate/extracted/usr/include"
# The closed precheck supplies DEPENDENCY IDENTITIES, never scientific results.
# All 521 consumed Boost headers are hashed before either fresh compilation.
# The canonical map below is pinned as a whole, then retained per header in
# sources_before.json. A new/missing/changed dependency is fail-closed.
BOOST_DEPFILE = ROOT / ("build/v7_full_lazy_20260905_precheck/CMakeFiles/"
                       "mhgp7_full_gabriel_digest_gate.dir/tests/full_gabriel_digest_gate.cpp.o.d")
BOOST_DEPFILE_SHA = "ec9abb0b435d292607f40e7fc163f6dec7e38c900830492505faddddd8a9dc89"
BOOST_MAP_SHA = "409cccaa1709a6aecf93caac66303835b804da78b315160e76ce14a5352b18b7"
BOOST_HEADER_COUNT = 521
BUILDS = {mode: ROOT / ("build/v7_full_lazy_20260905_" + mode) for mode in ("release", "san")}
REGEX = "^mhgp7_full_(certificate|gabriel)"
ASAN = "detect_leaks=1:halt_on_error=1"
UBSAN = "halt_on_error=1:print_stacktrace=1"
# Closed target/argv/expected-exit inventory confirmed by ROOT; a mismatch
# always refuses before launching any CTest engine.
TARGETS = {
    "mhgp7_full_certificate_gate": "tests/full_certificate_gate.cpp",
    "mhgp7_full_gabriel_gate": "tests/full_gabriel_gate.cpp",
    "mhgp7_full_gabriel_allocation_gate": "tests/full_gabriel_allocation_gate.cpp",
    "mhgp7_full_gabriel_lazy_gate": "tests/full_gabriel_lazy_gate.cpp",
    "mhgp7_full_gabriel_lazy_allocation_gate": "tests/full_gabriel_lazy_allocation_gate.cpp",
    "mhgp7_full_gabriel_digest_gate": "tests/full_gabriel_digest_gate.cpp",
}
TESTS = {
    "mhgp7_full_certificate": ("mhgp7_full_certificate_gate", "--selftest", 0),
    "mhgp7_full_certificate_rejects": ("mhgp7_full_certificate_gate", "--rejects", 0),
    "mhgp7_full_gabriel": ("mhgp7_full_gabriel_gate", "--selftest", 0),
    "mhgp7_full_gabriel_rejects": ("mhgp7_full_gabriel_gate", "--rejects", 0),
    "mhgp7_full_gabriel_bad_argument": ("mhgp7_full_gabriel_gate", "--unknown", 2),
    "mhgp7_full_gabriel_allocation": ("mhgp7_full_gabriel_allocation_gate", "--selftest", 0),
    "mhgp7_full_gabriel_allocation_bad_argument": ("mhgp7_full_gabriel_allocation_gate", "--unknown", 2),
    "mhgp7_full_gabriel_lazy": ("mhgp7_full_gabriel_lazy_gate", "--selftest", 0),
    "mhgp7_full_gabriel_lazy_rejects": ("mhgp7_full_gabriel_lazy_gate", "--rejects", 0),
    "mhgp7_full_gabriel_lazy_bad_argument": ("mhgp7_full_gabriel_lazy_gate", "--unknown", 2),
    "mhgp7_full_gabriel_lazy_allocation": ("mhgp7_full_gabriel_lazy_allocation_gate", "--selftest", 0),
    "mhgp7_full_gabriel_lazy_allocation_bad_argument": ("mhgp7_full_gabriel_lazy_allocation_gate", "--unknown", 2),
    "mhgp7_full_gabriel_digest": ("mhgp7_full_gabriel_digest_gate", "--selftest", 0),
    "mhgp7_full_gabriel_digest_bad_argument": ("mhgp7_full_gabriel_digest_gate", "--unknown", 2),
}
TOOLS = ("/usr/bin/cmake", "/usr/bin/ctest", "/usr/bin/make", "/usr/bin/g++",
         "/usr/bin/c++", "/usr/bin/taskset", "/usr/bin/as", "/usr/bin/ld", "/usr/bin/git")


def require(ok: bool, why: str) -> None:
    if not ok:
        raise RuntimeError(why)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def utc() -> str:
    return datetime.now(timezone.utc).isoformat()


def encoded(obj) -> bytes:
    return (json.dumps(obj, sort_keys=True, indent=2) + "\n").encode()


def save(path: Path, obj) -> None:
    with path.open("xb") as stream:
        stream.write(encoded(obj))
        stream.flush()
        os.fsync(stream.fileno())


def metadata(value) -> dict:
    return {"device": value.st_dev, "inode": value.st_ino, "bytes": value.st_size,
            "mtime_ns": value.st_mtime_ns, "ctime_ns": value.st_ctime_ns}


def read_stable(path: Path) -> tuple[bytes, dict]:
    before = path.lstat()
    require(stat.S_ISREG(before.st_mode), "not a regular nonsymlink file: " + str(path))
    with path.open("rb") as stream:
        require(metadata(before) == metadata(os.fstat(stream.fileno())), "file changed at open")
        raw = stream.read()
        require(metadata(before) == metadata(os.fstat(stream.fileno())), "file changed while read")
    require(metadata(before) == metadata(path.lstat()) and len(raw) == before.st_size,
            "file changed after read: " + str(path))
    return raw, {"path": str(path), "sha256": sha(raw), **metadata(before)}


def copy_exact(source: Path, target: Path) -> dict:
    raw, first = read_stable(source)
    with target.open("xb") as stream:
        stream.write(raw)
        stream.flush()
        os.fsync(stream.fileno())
    copied, archived = read_stable(target)
    final, after = read_stable(source)
    require(raw == copied == final and first == after, "copy or source changed: " + str(source))
    return {"source": first, "archive": archived}


def boost_snapshot() -> dict:
    raw, binding = read_stable(BOOST_DEPFILE)
    require(binding["sha256"] == BOOST_DEPFILE_SHA, "closed Boost dependency file changed")
    text = raw.decode("utf-8").replace("\\\n", " ")
    require(":" in text, "Boost dependency file malformed")
    head, body = text.split(":", 1)
    require(head.strip() == "CMakeFiles/mhgp7_full_gabriel_digest_gate.dir/tests/full_gabriel_digest_gate.cpp.o",
            "Boost precheck target identity differs")
    paths = {Path(token).resolve() for token in shlex.split(body)
             if token.startswith(str(BOOST) + "/")}
    require(len(paths) == BOOST_HEADER_COUNT and all(path.is_relative_to(BOOST) for path in paths),
            "Boost transitive inventory differs")
    result = {str(path.relative_to(BOOST)): read_stable(path)[1]["sha256"] for path in sorted(paths)}
    require(sha(encoded(result)) == BOOST_MAP_SHA, "pre-build Boost header fingerprint differs")
    return result


def source_snapshot() -> dict:
    paths = {SOURCE / "CMakeLists.txt", SOURCE / "bench/full_gabriel_semantic_digest.hpp",
             *(SOURCE / name for name in TARGETS.values())}
    for directory in ("src", "oracle", "cmake"):
        for path in (SOURCE / directory).rglob("*"):
            require(not path.is_symlink(), "source symlink forbidden: " + str(path))
            if path.is_file():
                paths.add(path)
    result = {str(path.relative_to(ROOT)): read_stable(path)[1]["sha256"] for path in sorted(paths)}
    result.update({str((BOOST / name).relative_to(ROOT)): pin for name, pin in boost_snapshot().items()})
    result[str(BOOST_DEPFILE.relative_to(ROOT))] = BOOST_DEPFILE_SHA
    return result


def tool_snapshot() -> dict:
    # Observable executables, not a claim that every system header/library is pinned.
    result = {}
    for name in (*TOOLS, sys.executable):
        path = Path(name).resolve(strict=True)
        require(os.access(path, os.X_OK), "tool is not executable")
        result[name] = {"resolved": str(path), "sha256": read_stable(path)[1]["sha256"]}
    return result


def host() -> dict:
    status = {}
    for line in Path("/proc/self/status").read_text().splitlines():
        key, _, value = line.partition(":")
        if key in ("TracerPid", "NoNewPrivs", "Seccomp", "Cpus_allowed_list", "VmRSS", "VmHWM"):
            status[key] = value.strip()
    cpus = {}
    for block in Path("/proc/cpuinfo").read_text().split("\n\n"):
        fields = dict(line.split(":", 1) for line in block.splitlines() if ":" in line)
        normalized = {key.strip(): value.strip() for key, value in fields.items()}
        if normalized.get("processor") in ("0", "6"):
            cpus[normalized["processor"]] = {
                key: normalized.get(key) for key in ("model name", "vendor_id", "cpu family", "model", "stepping")}
    return {"utc": utc(), "uname": list(platform.uname()), "python": sys.version,
            "process_status": status, "affinity": sorted(os.sched_getaffinity(0)),
            "cpu_models": cpus, "cpus_online": Path("/sys/devices/system/cpu/online").read_text().strip(),
            "rlimit_as": list(resource.getrlimit(resource.RLIMIT_AS)),
            "rlimit_cpu": list(resource.getrlimit(resource.RLIMIT_CPU)),
            "permission_override": "none", "gcp": "not_used"}


def environment(mode: str) -> dict:
    require(not any(value for key, value in os.environ.items() if key.startswith("LD_")),
            "nonempty LD_* forbidden; values not captured")
    require(not os.environ.get("LSAN_OPTIONS"), "LSAN override forbidden; leak detection never disabled")
    for key, expected in (("ASAN_OPTIONS", ASAN), ("UBSAN_OPTIONS", UBSAN)):
        require(os.environ.get(key, "") in ("", expected), "unreviewed sanitizer override: " + key)
    # Explicit child environment: no inherited compiler flags, launchers, CMake
    # toolchain overrides, LSAN suppression, or unrelated application secrets.
    result = {"PATH": "/usr/bin:/bin", "LANG": "C", "LC_ALL": "C",
              "PYTHONDONTWRITEBYTECODE": "1"}
    if mode == "san":
        result.update(ASAN_OPTIONS=ASAN, UBSAN_OPTIONS=UBSAN)
    return result


def commands(mode: str) -> list[tuple[str, list[str], int]]:
    build = BUILDS[mode]
    out = OUT / mode
    sanitized = mode == "san"
    base_flags = "-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie" if sanitized else ""
    linker = "-fsanitize=address,undefined -no-pie" if sanitized else ""
    build_type = "RelWithDebInfo" if sanitized else "Release"
    flags_name = "RELWITHDEBINFO" if sanitized else "RELEASE"
    flags = "-O1 -g -DNDEBUG" if sanitized else "-O3 -DNDEBUG"
    return [
        ("configure", ["/usr/bin/taskset", "-c", "0", "/usr/bin/cmake", "-S", str(SOURCE),
         "-B", str(build), "-G", "Unix Makefiles", "-DCMAKE_CXX_COMPILER=/usr/bin/g++",
         "-DCMAKE_BUILD_TYPE=" + build_type, "-DCMAKE_CXX_FLAGS=" + base_flags,
         "-DCMAKE_CXX_FLAGS_" + flags_name + "=" + flags,
         "-DCMAKE_EXE_LINKER_FLAGS=" + linker, "-DCMAKE_EXE_LINKER_FLAGS_" + flags_name + "=",
         "-DMHGP7_DIGEST_BOOST_INCLUDE_DIR:PATH=" + str(BOOST),
         "-DMHGP7_ENABLE_CUDA=OFF", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"], 120),
        ("build", ["/usr/bin/taskset", "-c", "0", "/usr/bin/cmake", "--build", str(build),
                   "--parallel", "2", "--target", *TARGETS], 600),
        ("inventory", ["/usr/bin/taskset", "-c", "0", "/usr/bin/ctest", "--test-dir", str(build),
                       "--show-only=json-v1", "-R", REGEX], 60),
        ("ctest", ["/usr/bin/taskset", "-c", "6", "/usr/bin/ctest", "--test-dir", str(build),
                   "-j1", "-V", "--output-on-failure", "--no-tests=error", "--timeout", "60",
                   "-R", REGEX, "--output-junit", str(out / "ctest.junit.xml")], 120),
    ]


def terminate_owned(process: subprocess.Popen) -> None:
    # Only the process group created by this invocation; never a name-based kill.
    for sig in (signal.SIGTERM, signal.SIGKILL):
        try:
            os.killpg(process.pid, sig)
        except ProcessLookupError:
            pass
        if sig == signal.SIGTERM:
            try:
                process.wait(timeout=0.5)
            except subprocess.TimeoutExpired:
                pass
    process.wait(timeout=5)


def run(out: Path, label: str, argv: list[str], limit: int, env: dict, records: list) -> dict:
    record = {"label": label, "argv": argv, "cwd": str(ROOT), "environment": env,
              "timeout_seconds": limit, "expected_code": 0, "started_utc": utc()}
    save(out / (label + ".intent.json"), record)
    begin = time.monotonic()
    process = None
    error = None
    print(json.dumps({"begin": label, "directory": str(out), "timeout_seconds": limit}), flush=True)
    try:
        with (out / (label + ".stdout")).open("xb") as stdout, (out / (label + ".stderr")).open("xb") as stderr:
            process = subprocess.Popen(argv, cwd=ROOT, env=env, stdout=stdout, stderr=stderr,
                                       start_new_session=True)
            deadline = begin + limit
            while process.poll() is None:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise subprocess.TimeoutExpired(argv, limit)
                try:
                    process.wait(timeout=min(30, remaining))
                except subprocess.TimeoutExpired:
                    print(json.dumps({"progress": label, "elapsed_seconds": time.monotonic() - begin}), flush=True)
    except BaseException as exc:
        error = f"{type(exc).__name__}: {exc}"
        if process is not None:
            try:
                terminate_owned(process)
            except BaseException as cleanup:
                error += f"; cleanup={type(cleanup).__name__}: {cleanup}"
    finally:
        record.update(exit_code=None if process is None else process.returncode, error=error,
                      elapsed_seconds=time.monotonic() - begin, ended_utc=utc(),
                      status="completed" if error is None and process is not None and process.returncode == 0 else "failed")
        record["raw"] = {kind: read_stable(out / (label + "." + kind))[1]
                         for kind in ("stdout", "stderr") if (out / (label + "." + kind)).exists()}
        save(out / (label + ".command.json"), record)
        records.append(record)
        print(json.dumps({"end": label, "status": record["status"], "exit_code": record["exit_code"],
                          "elapsed_seconds": record["elapsed_seconds"]}), flush=True)
    return record


def inventory(raw: bytes, build: Path) -> dict:
    obj = json.loads(raw)
    rows = obj.get("tests", [])
    names = [row.get("name") for row in rows]
    require(len(names) == len(TESTS) == 14 and len(set(names)) == 14 and sorted(names) == sorted(TESTS),
            "closed CTest inventory mismatch (zero tests is never passed)")
    for row in rows:
        target, argument, code = TESTS[row["name"]]
        expected = ["/usr/bin/cmake", "-DEXPECTED=" + str(code), "-DCMD=" + str(build / target),
                    "-DARGS=" + argument, "-DEXPECT_LINE=", "-DEXPECT_PREFIX=", "-P",
                    str(SOURCE / "cmake/run_expect.cmake")]
        require(row.get("command") == expected, "test command/code binding mismatch: " + row["name"])
        properties = {item["name"]: item["value"] for item in row.get("properties", [])}
        require(properties.get("TIMEOUT") == 60 and properties.get("WORKING_DIRECTORY") == str(build),
                "test timeout or working directory differs")
        require(not any(key in properties for key in
                        ("DISABLED", "WILL_FAIL", "PASS_REGULAR_EXPRESSION", "SKIP_RETURN_CODE",
                         "ENVIRONMENT", "ENVIRONMENT_MODIFICATION", "FIXTURES_REQUIRED", "DEPENDS")),
                "unreviewed test mutation property")
    return {"tests": len(rows), "names": sorted(names), "commands_codes_and_timeouts_exact": True}


def binary_snapshot(build: Path) -> dict:
    result = {}
    for target in TARGETS:
        path = build / target
        require(os.access(path, os.X_OK), "missing/nonexecutable binary: " + target)
        result[target] = read_stable(path)[1]
    return result


def compile_binding(build: Path, mode: str, before: dict) -> dict:
    cache = (build / "CMakeCache.txt").read_text().splitlines()
    sanitized = mode == "san"
    mode_name = "RELWITHDEBINFO" if sanitized else "RELEASE"
    build_type = "RelWithDebInfo" if sanitized else "Release"
    base = "-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie" if sanitized else ""
    opts = "-O1 -g -DNDEBUG" if sanitized else "-O3 -DNDEBUG"
    link = "-fsanitize=address,undefined -no-pie" if sanitized else ""
    for exact in ("CMAKE_BUILD_TYPE:STRING=" + build_type, "CMAKE_CXX_FLAGS:STRING=" + base,
                  "CMAKE_CXX_FLAGS_" + mode_name + ":STRING=" + opts,
                  "CMAKE_EXE_LINKER_FLAGS:STRING=" + link, "CMAKE_EXE_LINKER_FLAGS_" + mode_name + ":STRING=",
                  "CMAKE_HOME_DIRECTORY:INTERNAL=" + str(SOURCE), "MHGP7_ENABLE_CUDA:BOOL=OFF",
                  "MHGP7_DIGEST_BOOST_INCLUDE_DIR:PATH=" + str(BOOST),
                  "CMAKE_GENERATOR:INTERNAL=Unix Makefiles"):
        require(exact in cache, "CMake cached configuration differs: " + exact)
    database = json.loads((build / "compile_commands.json").read_text())
    result = {"targets": {}, "system_headers": "listed by .o.d; not individually pinned before compilation",
              "fresh_not_hermetic": True}
    required_flags = shlex.split(base) + shlex.split(opts) + [
        "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    for target, filename in TARGETS.items():
        relobject = "CMakeFiles/" + target + ".dir/" + filename + ".o"
        selected = [row for row in database if row.get("output") == relobject]
        require(len(selected) == 1, "compile target missing/repeated: " + target)
        row = selected[0]
        require(row["directory"] == str(build) and row["file"] == str(SOURCE / filename),
                "compile source/directory binding")
        argv = shlex.split(row["command"])
        require(argv[0] == "/usr/bin/g++", "compiler wrapper or unreviewed compiler")
        includes = ["-isystem", str(BOOST)] if target == "mhgp7_full_gabriel_digest_gate" else []
        require(Counter(argv[1:]) == Counter(required_flags + includes + ["-o", relobject, "-c", str(SOURCE / filename)]),
                "unreviewed/missing compiler option: " + target)
        link_path = build / ("CMakeFiles/" + target + ".dir/link.txt")
        link_argv = shlex.split(link_path.read_text())
        require(link_argv[0] == "/usr/bin/g++" and
                Counter(link_argv[1:]) == Counter(shlex.split(base) + shlex.split(opts) +
                                                shlex.split(link) + [relobject, "-o", target]),
                "unreviewed link option: " + target)
        dep_path = build / (relobject + ".d")
        dep = dep_path.read_text().replace("\\\n", " ")
        require(":" in dep, "dependency file malformed")
        head, body = dep.split(":", 1)
        require(head.strip() == relobject, "dependency output binding mismatch")
        dependencies = [(build / token).resolve() if not Path(token).is_absolute() else Path(token).resolve()
                        for token in shlex.split(body)]
        project = {}
        boost_dependencies = {}
        external = []
        for path in dependencies:
            if path.is_relative_to(BOOST):
                key = str(path.relative_to(ROOT))
                pin = read_stable(path)[1]["sha256"]
                require(key in before and pin == before[key], "Boost header new or changed since pre-build: " + key)
                boost_dependencies[key] = pin
                continue
            if path.is_relative_to(ROOT):
                key = str(path.relative_to(ROOT))
                require(key in before, "project dependency was not in pre-build source inventory: " + key)
                pin = read_stable(path)[1]["sha256"]
                require(pin == before[key], "project dependency changed: " + key)
                project[key] = pin
            else:
                external.append(str(path))
        require(str(SOURCE / filename) in [str(path) for path in dependencies] and project,
                "empty or missing primary project dependency")
        result["targets"][target] = {"compile": row, "link_argv": link_argv,
                                     "project_dependencies": project, "external_headers": external,
                                     "boost_headers_pre_pinned_and_rechecked": boost_dependencies,
                                     "object": read_stable(build / relobject)[1],
                                     "depfile": read_stable(dep_path)[1],
                                     "link_file": read_stable(link_path)[1]}
    result["configuration"] = {name: read_stable(build / name)[1] for name in
                               ("CMakeCache.txt", "compile_commands.json", "CTestTestfile.cmake")}
    return result


# Explicit standalone port of the historical closed grammar cited above.

def judge_log(data: bytes, expected: list[str]) -> dict:
    text = data.decode("utf-8", errors="strict")
    separator = "-" * 58
    start = re.match(r"Start testing: [^\n]+\n" + separator + r"\n", text)
    require(start is not None, "LastTest start header missing")
    block = re.compile(
        r'([1-9][0-9]*)/([1-9][0-9]*) Testing: (mhgp7_[^\s]+)\n'
        r'\1/\2 Test: \3\nCommand: [^\n]+\nDirectory: [^\n]+\n'
        r'"\3" start time: [^\n]+\nOutput:\n' + separator + r'\n[\s\S]*?'
        r'^<end of output>\nTest time =\s*[0-9]+(?:\.[0-9]+)? sec\n' + separator + r'\n'
        r'Test Passed\.\n"\3" end time: [^\n]+\n'
        r'"\3" time elapsed: [0-9]{2}:[0-9]{2}:[0-9]{2}\n' + separator + r'\n\n', re.M)
    position, actual, indices, totals = start.end(), [], [], []
    while not text.startswith("End testing: ", position):
        match = block.match(text, position)
        require(match is not None, "LastTest test block truncated, failed, or malformed")
        index, total, name = int(match[1]), int(match[2]), match[3]
        require(index <= total, "LastTest test index exceeds inventory")
        actual.append(name)
        indices.append(index)
        totals.append(total)
        position = match.end()
    require(re.fullmatch(r"End testing: [^\n]+\n\n(?:[^\s=]+ = +[0-9]+(?:\.[0-9]+)? sec\*proc\n\n)*",
                         text[position:]) is not None, "LastTest terminal footer missing or malformed")
    require(len(actual) == len(expected) and len(set(actual)) == len(expected) and sorted(actual) == expected,
            "LastTest names missing, duplicate, or unexpected")
    require(len(set(indices)) == len(expected) and len(set(totals)) == 1, "LastTest test indices repeated or inconsistent")
    return {"tests": len(actual), "names": sorted(actual), "terminal_footer": True, "all_blocks_passed": True}




def judge_junit(raw: bytes) -> dict:
    root = ET.fromstring(raw)
    require(root.tag == "testsuite" and root.get("tests") == "14", "JUnit suite/count mismatch")
    require(all(root.get(key) == "0" for key in ("failures", "disabled", "skipped")),
            "JUnit failure/disabled/skipped summary")
    rows = root.findall("testcase")
    names = [row.get("name") for row in rows]
    require(len(names) == 14 and len(set(names)) == 14 and sorted(names) == sorted(TESTS),
            "JUnit names missing/duplicate/foreign")
    for row in rows:
        duration = float(row.get("time", "nan"))
        require(row.get("status") == "run" and math.isfinite(duration) and duration >= 0,
                "JUnit case not run or invalid duration")
        require(not any(row.find(key) is not None for key in ("failure", "error", "skipped")),
                "JUnit failure/error/skipped case")
    return {"tests": 14, "names": sorted(names), "all_run": True}


def fence(build: Path, out: Path) -> dict:
    directory = build / "Testing/Temporary"
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / ".full_lazy_owned_ctest_fence"
    save(path, {"run": str(out), "utc_diagnostic_only": utc()})
    result = copy_exact(path, out / "ctest.fence")
    save(out / "ctest.fence.json", result)
    return result


def archive_test_outputs(build: Path, out: Path, boundary: dict, previous_log: dict | None) -> dict:
    log = build / "Testing/Temporary/LastTest.log"
    # Copy bytes BEFORE judging. Failed/truncated output remains evidence.
    copies = {}
    errors = []
    for source, name in ((log, "LastTest.stdout"), (out / "ctest.junit.xml", "JUnit.stdout")):
        try:
            copies[name] = copy_exact(source, out / name)
        except BaseException as exc:
            errors.append(f"{name}: {type(exc).__name__}: {exc}")
    save(out / "test_output_copies.json", {"copies": copies, "errors": errors})
    require(not errors, "test output absent or unstable: " + repr(errors))
    prior = boundary["source"]
    require(read_stable(Path(prior["path"]))[1] == prior, "CTest fence changed")
    for name, binding in copies.items():
        current = binding["source"]
        require(current["device"] == prior["device"] and current["mtime_ns"] >= prior["mtime_ns"] and
                current["ctime_ns"] >= prior["ctime_ns"], "test output predates same-filesystem fence: " + name)
    if previous_log is not None:
        require(copies["LastTest.stdout"]["source"]["sha256"] != previous_log["source"]["sha256"],
                "LastTest remained unchanged from the pre-CTest state")
    judged = {"last_test": judge_log((out / "LastTest.stdout").read_bytes(), sorted(TESTS)),
              "junit": judge_junit((out / "JUnit.stdout").read_bytes()), "copies": copies, "fresh": True}
    for binding in copies.values():
        original, original_meta = read_stable(Path(binding["source"]["path"]))
        copied, copy_meta = read_stable(Path(binding["archive"]["path"]))
        require(original_meta == binding["source"] and copy_meta == binding["archive"] and original == copied,
                "test output changed after judgment")
    return judged


def collect_build_artifacts(build: Path, out: Path) -> dict:
    # Explicit bounded paths, also collected for partial/failed builds.
    paths = [build / name for name in ("CMakeCache.txt", "compile_commands.json", "CTestTestfile.cmake")]
    for target, filename in TARGETS.items():
        directory = build / ("CMakeFiles/" + target + ".dir")
        paths += [directory / "flags.make", directory / "link.txt",
                  directory / (filename + ".o.d"), directory / "build.make"]
    dest = out / "build_artifacts"
    dest.mkdir(exist_ok=False)
    result = {}
    for source in paths:
        if source.exists() or source.is_symlink():
            name = str(source.relative_to(build)).replace("/", "__")
            result[str(source.relative_to(build))] = copy_exact(source, dest / name)
    save(out / "build_artifact_copies.json", result)
    return result


def phase(mode: str, before: dict, controller_pin: str, tools_before: dict) -> dict:
    build, out = BUILDS[mode], OUT / mode
    out.mkdir(exist_ok=False)
    records, errors = [], []
    result = {"mode": mode, "status": "failed", "commands": records, "errors": errors}
    binding = binaries = boundary = previous_log = None
    try:
        require(not build.exists() and not build.is_symlink(), "build ceased to be fresh: " + mode)
        save(out / "sources_before.json", source_snapshot())
        save(out / "host_before.json", host())
        env = environment(mode)
        save(out / "environment.json", env)
        for label, argv, limit in commands(mode):
            require(source_snapshot() == before and read_stable(Path(__file__))[1]["sha256"] == controller_pin,
                    "input/controller drift before " + label)
            require(tool_snapshot() == tools_before, "toolchain changed before " + label)
            if label == "ctest":
                require(host()["process_status"].get("TracerPid") == "0", "traced controller context forbidden")
                log = build / "Testing/Temporary/LastTest.log"
                if log.exists() or log.is_symlink():
                    # An inventory-only CTest may have created a diagnostic log.
                    # Preserve it; it cannot be reused as fresh test authority.
                    previous_log = copy_exact(log, out / "LastTest.preexisting.stdout")
                save(out / "LastTest.prestate.json", {"present": previous_log is not None,
                                                       "binding": previous_log})
                require(not (out / "ctest.junit.xml").exists(), "preexisting JUnit")
                require(binary_snapshot(build) == binaries and compile_binding(build, mode, before) == binding,
                        "binary or compile-binding drift before CTest")
                save(out / "sources_before_ctest.json", source_snapshot())
                save(out / "binaries_before_ctest.json", binary_snapshot(build))
                save(out / "compile_binding_before_ctest.json", compile_binding(build, mode, before))
                boundary = fence(build, out)
            record = run(out, label, argv, limit, env, records)
            require(record["status"] == "completed" and record["exit_code"] == 0, label + " failed")
            require(source_snapshot() == before, "source drift after " + label)
            if label == "build":
                binding, binaries = compile_binding(build, mode, before), binary_snapshot(build)
                save(out / "compile_binding.json", binding)
                save(out / "binaries_after_build.json", binaries)
            elif label == "inventory":
                result["inventory"] = inventory((out / "inventory.stdout").read_bytes(), build)
    except BaseException as exc:
        errors.append(f"{type(exc).__name__}: {exc}")
    # Every terminal observation is attempted even after a command failed.
    def observe(key, function):
        try:
            value = function()
            result[key] = value
            save(out / (key + ".json"), value)
            return value
        except BaseException as exc:
            errors.append(f"{key}: {type(exc).__name__}: {exc}")
            return None
    source_after = observe("sources_after", source_snapshot)
    binaries_after = observe("binaries_after", lambda: binary_snapshot(build))
    binding_after = observe("compile_binding_after", lambda: compile_binding(build, mode, before))
    observe("host_after", host)
    observe("toolchain_after", tool_snapshot)
    observe("build_artifacts", lambda: collect_build_artifacts(build, out))
    if boundary is not None:
        observe("test_outputs", lambda: archive_test_outputs(build, out, boundary, previous_log))
    else:
        errors.append("CTest was not admitted; no test success inferred")
    result["sources_stable"] = source_after == before
    result["binaries_stable"] = binaries is not None and binaries_after == binaries
    result["compile_binding_stable"] = binding is not None and binding_after == binding
    required_labels = [row[0] for row in commands(mode)]
    passed = (not errors and result["sources_stable"] and result["binaries_stable"] and
              result["compile_binding_stable"] and result.get("toolchain_after") == tools_before and
              result.get("inventory") is not None and result.get("test_outputs") is not None and
              [row["label"] for row in records] == required_labels and
              all(row["status"] == "completed" and row["exit_code"] == 0 for row in records))
    result["status"] = "completed" if passed else "failed"
    save(out / "summary.json", result)
    return result


def execute(args) -> int:
    require(not OUT.exists() and not OUT.is_symlink(), "runs already exists; no retry or overwrite")
    OUT.mkdir(mode=0o700)
    receipt = {"schema": "mhgp7-private-full-lazy-qualification-v1", "status": "failed",
               "started_utc": utc(), "phases": [], "errors": [], "public_status": "not_claimed",
               "fresh_tests_only": True, "historical_results_reused": False, "gcp": "not_used",
               "scope": "14 targeted tests per fresh Release/SAN build, not the complete suite",
               "hermetic": False, "permission_override": "none"}
    before = tools_before = None
    try:
        require(re.fullmatch("[0-9a-f]{64}", args.expected_controller_sha256 or "") is not None and
                re.fullmatch("[0-9a-f]{64}", args.expected_source_sha256 or "") is not None,
                "two explicit reviewed SHA256 pins required")
        require(read_stable(Path(__file__))[1]["sha256"] == args.expected_controller_sha256,
                "controller SHA256 mismatch")
        require(Path(__file__).resolve() == BASE / "capture.py", "controller path changed")
        for build in BUILDS.values():
            require(not build.exists() and not build.is_symlink(), "both build directories must be absent at admission")
        before = source_snapshot()
        save(OUT / "sources_before.json", before)
        require(sha(encoded(before)) == args.expected_source_sha256, "source admission fingerprint differs")
        admission = host()
        save(OUT / "host_before.json", admission)
        require(admission["process_status"].get("TracerPid") == "0", "ROOT nontraced context required")
        environment("release")
        environment("san")
        tools_before = tool_snapshot()
        save(OUT / "toolchain_before.json", tools_before)
        save(OUT / "boost_dependency_binding.json", {
            "dependency_file": copy_exact(BOOST_DEPFILE, OUT / "boost_precheck_dependencies.stdout"),
            "header_count": BOOST_HEADER_COUNT, "canonical_header_map_sha256": BOOST_MAP_SHA,
            "authority": "dependency_inventory_only_no_precheck_test_result_reused"})
        save(OUT / "controller_binding.json", {"source_sha256": args.expected_source_sha256,
             "controller_sha256": args.expected_controller_sha256, "expected_tests": TESTS, "targets": TARGETS})
        tool_records = []
        for label, command in (
                ("cmake_version", ["/usr/bin/cmake", "--version"]),
                ("ctest_version", ["/usr/bin/ctest", "--version"]),
                ("compiler_version", ["/usr/bin/g++", "--version"]),
                ("compiler_target", ["/usr/bin/g++", "-dumpmachine"]),
                ("compiler_cc1plus", ["/usr/bin/g++", "-print-prog-name=cc1plus"]),
                ("make_version", ["/usr/bin/make", "--version"]),
                ("git_head_before", ["/usr/bin/git", "--no-optional-locks", "-C", str(ROOT), "rev-parse", "HEAD"]),
                ("git_status_before", ["/usr/bin/git", "--no-optional-locks", "-C", str(ROOT), "status",
                 "--porcelain=v1", "--untracked-files=normal", "--", "morsehgp3D_v7"])):
            record = run(OUT, label, ["/usr/bin/taskset", "-c", "0", *command], 15,
                         environment("release"), tool_records)
            require(record["status"] == "completed", "toolchain observation failed")
        save(OUT / "toolchain_commands.json", tool_records)
        for mode in ("release", "san"):
            observed = phase(mode, before, args.expected_controller_sha256, tools_before)
            receipt["phases"].append(observed)
            require(observed["status"] == "completed", mode + " qualification failed; no automatic retry")
        require(source_snapshot() == before and tool_snapshot() == tools_before and
                read_stable(Path(__file__))[1]["sha256"] == args.expected_controller_sha256,
                "terminal source/controller/toolchain drift")
        receipt["status"] = "completed"
    except BaseException as exc:
        receipt["errors"].append(f"{type(exc).__name__}: {exc}")
    if tools_before is not None:
        repository_after = []
        try:
            for label, command in (
                    ("git_head_after", ["rev-parse", "HEAD"]),
                    ("git_status_after", ["status", "--porcelain=v1", "--untracked-files=normal", "--", "morsehgp3D_v7"])):
                record = run(OUT, label, ["/usr/bin/taskset", "-c", "0", "/usr/bin/git",
                             "--no-optional-locks", "-C", str(ROOT), *command], 15,
                             environment("release"), repository_after)
                require(record["status"] == "completed", "terminal repository observation failed")
            save(OUT / "repository_observation.json", {
                "after_commands": repository_after,
                "scope": "HEAD_and_v7_porcelain_v1_paths_only_no_diff_payload",
                "head_or_unrelated_worktree_stability_required": False,
                "source_byte_stability_remains_required": True})
        except BaseException as exc:
            receipt["errors"].append(f"repository_after: {type(exc).__name__}: {exc}")
            receipt["status"] = "failed"
    for key, function in (("sources_after", source_snapshot), ("toolchain_after", tool_snapshot),
                          ("host_after", host)):
        try:
            value = function()
            save(OUT / (key + ".json"), value)
            if key == "sources_after":
                require(before is not None and value == before, "final source snapshot differs")
            elif key == "toolchain_after":
                require(tools_before is not None and value == tools_before, "final toolchain snapshot differs")
        except BaseException as exc:
            receipt["errors"].append(f"{key}: {type(exc).__name__}: {exc}")
            receipt["status"] = "failed"
    receipt["ended_utc"] = utc()
    save(OUT / "receipt.json", receipt)
    manifest = {str(path.relative_to(OUT)): read_stable(path)[1]
                for path in sorted(OUT.rglob("*")) if path.is_file()}
    save(OUT / "manifest.json", manifest)
    sums = {**{key: value["sha256"] for key, value in manifest.items()},
            "manifest.json": read_stable(OUT / "manifest.json")[1]["sha256"]}
    with (OUT / "SHA256SUMS").open("xb") as stream:
        stream.write("".join(pin + "  " + name + "\n" for name, pin in sorted(sums.items())).encode())
    print(json.dumps({"status": receipt["status"], "errors": receipt["errors"],
                      "receipt": str(OUT / "receipt.json")}), flush=True)
    return 0 if receipt["status"] == "completed" else 1


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", action="store_true")
    parser.add_argument("--expected-controller-sha256")
    parser.add_argument("--expected-source-sha256")
    args = parser.parse_args(argv)
    if not args.execute:
        sources = source_snapshot()
        print(json.dumps({"status": "prepared_not_executed", "sources": sources,
                          "source_sha256": sha(encoded(sources)),
                          "controller_sha256": read_stable(Path(__file__))[1]["sha256"],
                          "build_directories_absent": {mode: not path.exists() and not path.is_symlink()
                                                       for mode, path in BUILDS.items()},
                          "commands": {mode: commands(mode) for mode in BUILDS},
                          "tests": TESTS, "targets": TARGETS, "requires_separate_ROOT_GO": True}, indent=2))
        return 0
    return execute(args)


def interrupted(signum, _frame):
    raise InterruptedError(f"signal {signum}")


if __name__ == "__main__":
    for number in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(number, interrupted)
    raise SystemExit(main())
