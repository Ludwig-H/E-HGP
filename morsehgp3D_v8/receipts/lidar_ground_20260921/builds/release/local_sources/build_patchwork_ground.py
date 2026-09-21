#!/usr/bin/env python3
"""Pinned standalone Patchwork++ build; no CMake, ROS, pip, Open3D or vendoring.

Only six immutable upstream files are fetched. All dependencies are inventoried
before compilation and rechecked after it. A build directory is never reused.
read(build, check_live=True) validates a closed build, not segmentation quality.
"""
from __future__ import annotations

import argparse
import base64
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shlex
import signal
import sys
import urllib.request

from run_p0_matrix import invoke, on_signal, parse_result

V8 = Path(__file__).resolve().parents[1]
PROBE = V8 / "bench/patchwork_ground_probe.cpp"
LOCAL = (Path(__file__).resolve(), PROBE, V8 / "bench/run_p0_matrix.py")
COMMIT = "3e6903a1d5537a4cc2ace897b0bbb98a92d6014c"
REPOSITORY = "https://github.com/url-kaist/patchwork-plusplus"
SCHEMA = "mhgp8_patchwork_ground_build_v1"
BINARY = "mhgp8_patchwork_ground_probe"
UPSTREAM = {
    "LICENSE": "b2ae335f9e2c0e9b7262226c8bb903743d4376bfca271ed8adbbfe9ad5755f7c",
    "cpp/patchworkpp/src/patchworkpp.cpp": "f7037532eea5027994f672fbdca34be9b7be1033fd8d6f23cf6e41d0912f9b2a",
    "cpp/patchworkpp/include/patchwork/patchworkpp.h": "3803a5cd4c1064c71fa09f3388369e04ca912d18cdc2b8f52a796a6bbfc362f8",
    "cpp/common/src/plane_fit.cpp": "a9b507089e6946954136a23b4e5ceb91e599fdca4da9ea6b2dc5f97fd8e9370c",
    "cpp/common/include/patchwork/plane_fit.h": "0f89a35178aa247bbdd80b36b983e8fd4ca85b338b9f8d882af99235b33fc7c2",
    "cpp/common/include/patchwork/types.h": "4265f49849f598d731908d4ce5ad05a78362fdda8518cd7f712046173bf31816",
}
ENV_KEYS = ("PATH", "LANG", "LC_ALL", "TZ", "CPATH", "CPLUS_INCLUDE_PATH",
            "C_INCLUDE_PATH", "GCC_EXEC_PREFIX", "COMPILER_PATH", "LIBRARY_PATH",
            "LD_LIBRARY_PATH", "LD_PRELOAD", "SOURCE_DATE_EPOCH",
            "ASAN_OPTIONS", "UBSAN_OPTIONS", "OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS")


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def pins(paths):
    return {str(p): sha(p) for p in sorted(map(Path, paths))}


def write(path, value):
    with Path(path).open("x") as stream:
        stream.write(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n")


def load(path):
    return parse_result(Path(path).read_bytes())


def stamp():
    return datetime.now(timezone.utc).isoformat()


def units(build):
    third = build / "third_party"
    return (
        ("patchworkpp", third / "cpp/patchworkpp/src/patchworkpp.cpp"),
        ("plane_fit", third / "cpp/common/src/plane_fit.cpp"),
        ("probe", build / "local_sources/patchwork_ground_probe.cpp"),
    )


def plan(config):
    build = Path(config["build"])
    common = ["-std=c++20", "-ffp-contract=off", "-fno-fast-math",
              "-DEIGEN_DONT_PARALLELIZE",
              '-DMHGP8_PATCHWORK_COMMIT="' + COMMIT + '"',
              "-isystem", config["eigen"], "-isystem",
              str(build / "third_party/cpp/patchworkpp/include"),
              "-isystem", str(build / "third_party/cpp/common/include")]
    common += (["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
               if config["sanitize"] else ["-O3", "-DNDEBUG"])
    # Third-party warnings are recorded, but not confused with our strict build.
    flags = {name: common + ["-Wall", "-Wextra", "-Wpedantic"] +
             (["-Werror"] if name == "probe" else []) for name, _ in units(build)}
    cxx = config["compiler"]
    result = [("compiler", [cxx, "--version"])]
    for name, source in units(build):
        result.append(("dependencies_" + name, [cxx, *flags[name], "-M", str(source),
                       "-MF", str(build / ("pre_" + name + ".d")),
                       "-MT", str(build / (name + ".o"))]))
    for name, source in units(build):
        result.append(("compile_" + name, [cxx, *flags[name], "-c", str(source),
                       "-MD", "-MF", str(build / (name + ".d")),
                       "-o", str(build / (name + ".o"))]))
    result.append(("link", [cxx, *common, *(str(build / (name + ".o")) for name, _ in units(build)),
                           "-o", str(build / BINARY)]))
    result.append(("describe", [str(build / BINARY), "--describe"]))
    return result


def dependencies(build, prefix=""):
    result = {}
    for name, source in units(build):
        data = (build / (prefix + name + ".d")).read_text().replace("\\\n", " ")
        require(":" in data, "missing dependency target")
        files = {Path(token).resolve(strict=True) for token in shlex.split(data.split(":", 1)[1])}
        require(source.resolve() in files, "unit source absent from dependency inventory")
        result[name] = pins(files)
    return result


def artifacts(build):
    require(not any(p.is_symlink() for p in build.rglob("*")), "symlink inside build")
    return pins(p for p in build.rglob("*") if p.is_file() and p != build / "COMPLETION.json")


def run(args):
    build = args.build.absolute()
    compiler = Path(args.compiler).absolute()  # Keep the driver name: clang++ is not clang.
    eigen = args.eigen.resolve(strict=True)
    require(not build.exists() and not build.is_symlink(), "fresh build directory required")
    require(compiler.is_file() and os.access(compiler, os.X_OK), "compiler is not executable")
    require((eigen / "Eigen/Core").is_file(), "existing Eigen include tree required")
    config = dict(build=str(build), compiler=str(compiler), eigen=str(eigen),
                  sanitize=bool(args.sanitize))
    source_before = pins([*LOCAL, compiler])
    environment = dict(os.environ)
    environment.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
    if args.sanitize:
        environment.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    env = {key: environment.get(key) for key in ENV_KEYS}
    build.mkdir(parents=True)
    for source in LOCAL:
        destination = build / "local_sources" / source.name
        destination.parent.mkdir(exist_ok=True)
        with destination.open("xb") as stream:
            stream.write(source.read_bytes())
    commands = plan(config)
    manifest = dict(schema=SCHEMA, commit=COMMIT, repository=REPOSITORY,
                    upstream_sha256=UPSTREAM, config=config, plan=commands,
                    source_sha256=source_before, environment=env, started_utc=stamp(),
                    launch=[sys.executable, *sys.argv],
                    scope="approximate_ground_mask_adapter_not_HGP_or_segmentation_quality",
                    download_base="https://raw.githubusercontent.com/url-kaist/patchwork-plusplus/" + COMMIT + "/",
                    offline_source=str(args.offline_source.absolute()) if args.offline_source else None)
    write(build / "MANIFEST.json", manifest)
    state = dict(status="running", downloads=[], commands=[],
                 manifest_sha256=sha(build / "MANIFEST.json"))
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    print(json.dumps(dict(build=str(build), status="starting")), flush=True)
    try:
        for relative, expected in UPSTREAM.items():
            source_url = manifest["download_base"] + relative
            if args.offline_source:
                content = (args.offline_source / relative).read_bytes()
            else:
                with urllib.request.urlopen(source_url, timeout=30) as response:
                    content = response.read()
            require(hashlib.sha256(content).hexdigest() == expected, "upstream content mismatch: " + relative)
            destination = build / "third_party" / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            with destination.open("xb") as stream:
                stream.write(content)
            state["downloads"].append(dict(path=relative, url=source_url, sha256=expected,
                                           bytes=len(content)))
        state["upstream_before"] = pins(build / "third_party" / name for name in UPSTREAM)
        for number, (label, command) in enumerate(commands):
            filename = f"command_{number:03}.json"
            record = dict(label=label, command=command, cwd=str(build), environment=env,
                          exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
            try:
                invoke(command, environment, build, record, new_session=True)
            finally:
                write(build / filename, record)
                state["commands"].append(dict(path=filename, sha256=sha(build / filename)))
            require(type(record["exit_code"]) is int and record["exit_code"] == 0, "command failed: " + label)
            if label == "dependencies_probe":
                state["dependencies_before"] = dependencies(build, "pre_")
            if label == "link":
                state["dependencies_after"] = dependencies(build)
                require(state["dependencies_after"] == state["dependencies_before"],
                        "compiled dependencies changed")
                state["binary_sha256"] = sha(build / BINARY)
            if label == "describe":
                description = parse_result(record["stdout"].encode())
                require(description["schema"] == "mhgp8_patchwork_ground_probe_v1" and
                        description["upstream_commit"] == COMMIT and
                        description["parameters"]["enable_RNR"] is False,
                        "unexpected native adapter identity")
                state["description"] = description
        state["status"] = "passed"
    except BaseException as error:
        state.update(status="failed", error=f"{type(error).__name__}: {error}")
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        closing_errors = []
        try:
            state["source_after"] = pins([*LOCAL, compiler])
            require(state["source_after"] == source_before, "live build sources/compiler changed")
            if "upstream_before" in state:
                state["upstream_after"] = pins(build / "third_party" / name for name in UPSTREAM)
                require(state["upstream_after"] == state["upstream_before"], "upstream sources changed")
            if "dependencies_after" in state:
                require(dependencies(build) == state["dependencies_before"], "closing dependency mismatch")
            state["artifacts"] = artifacts(build)
        except BaseException as error:
            closing_errors.append(f"{type(error).__name__}: {error}")
            state["status"] = "failed"
        state.update(closing_errors=closing_errors, finished_utc=stamp())
        write(build / "COMPLETION.json", state)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    require(state["status"] == "passed", state.get("error", "build closure failed"))
    return read(build)


def read(build, check_live=True):
    build = Path(build).resolve(strict=True)
    completion_hash = sha(build / "COMPLETION.json")
    closed = load(build / "COMPLETION.json")
    require(closed["status"] == "passed" and closed["closing_errors"] == [], "build is not closed PASS")
    require(sha(build / "MANIFEST.json") == closed["manifest_sha256"], "manifest hash mismatch")
    manifest = load(build / "MANIFEST.json")
    require(manifest["schema"] == SCHEMA and manifest["commit"] == COMMIT and
            manifest["repository"] == REPOSITORY and manifest["upstream_sha256"] == UPSTREAM,
            "incorrect upstream identity")
    require(set(manifest["source_sha256"]) ==
            {str(p) for p in LOCAL} | {manifest["config"]["compiler"]},
            "local source inventory mismatch")
    require(Path(manifest["config"]["build"]) == build, "build moved; absolute compiled paths are required")
    expected_plan = plan(manifest["config"])
    require(json.loads(json.dumps(expected_plan)) == manifest["plan"], "command plan mismatch")
    require(len(closed["commands"]) == len(expected_plan) == 9, "incomplete command sequence")
    require(closed["source_after"] == manifest["source_sha256"], "source closure mismatch")
    require(closed["upstream_after"] == closed["upstream_before"] ==
            {str(build / "third_party" / name): value for name, value in UPSTREAM.items()},
            "upstream source closure mismatch")
    for original, expected in manifest["source_sha256"].items():
        if Path(original).name in {path.name for path in LOCAL}:
            require(sha(build / "local_sources" / Path(original).name) == expected,
                    "local source archive mismatch")
    require(artifacts(build) == closed["artifacts"], "artifact closure mismatch")
    require(closed["dependencies_before"] == closed["dependencies_after"], "dependency closure mismatch")
    binary = build / BINARY
    require(sha(binary) == closed["binary_sha256"], "binary hash mismatch")
    for number, ((label, command), entry) in enumerate(zip(expected_plan, closed["commands"])):
        require(entry["path"] == f"command_{number:03}.json" and
                sha(build / entry["path"]) == entry["sha256"], "command receipt mismatch")
        record = load(build / entry["path"])
        require(record["label"] == label and record["command"] == command and
                record["cwd"] == str(build) and record["environment"] == manifest["environment"] and
                type(record["exit_code"]) is int and record["exit_code"] == 0,
                "command was not successful or differs from plan")
        for channel in ("stdout", "stderr"):
            require(base64.b64decode(record[channel + "_base64"], validate=True).decode("utf-8", errors="replace") ==
                    record[channel], "raw command output differs")
        if label == "describe":
            require(parse_result(record["stdout"].encode()) == closed["description"],
                    "native identity transcript mismatch")
    if check_live:
        require(pins(manifest["source_sha256"]) == manifest["source_sha256"],
                "live local sources or compiler differ")
        require(dependencies(build) == closed["dependencies_before"], "live dependency differs")
    authority = dict(status="passed", schema=SCHEMA, binary=str(binary),
                     binary_sha256=closed["binary_sha256"], commit=COMMIT,
                     source_sha256=manifest["source_sha256"],
                     dependencies=closed["dependencies_before"],
                     completion_sha256=completion_hash, check_live=bool(check_live),
                     sanitize=manifest["config"]["sanitize"])
    # Recheck after reading: concurrent mutation cannot be hidden by a start-only inventory.
    require(sha(build / "COMPLETION.json") == completion_hash and artifacts(build) == closed["artifacts"],
            "build changed during read")
    if check_live:
        require(pins(manifest["source_sha256"]) == manifest["source_sha256"] and
                dependencies(build) == closed["dependencies_before"], "live inputs changed during read")
    return authority


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="mode", required=True)
    build = commands.add_parser("run")
    build.add_argument("--build", type=Path, required=True)
    build.add_argument("--compiler", required=True)
    build.add_argument("--eigen", type=Path, default=Path("/usr/include/eigen3"))
    build.add_argument("--sanitize", action="store_true")
    build.add_argument("--offline-source", type=Path)
    reader = commands.add_parser("read")
    reader.add_argument("--path", type=Path, required=True)
    reader.add_argument("--historical", action="store_true")
    args = parser.parse_args()
    result = run(args) if args.mode == "run" else read(args.path, not args.historical)
    print(json.dumps(result, sort_keys=True, allow_nan=False))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, KeyError, TypeError) as error:
        print("patchwork ground build: " + str(error), file=sys.stderr)
        sys.exit(1)
