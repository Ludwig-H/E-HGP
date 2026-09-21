#!/usr/bin/env python3
"""Compile three isolated index mutations; one selftest, not three independent oracles.

Run: --build FRESH --output FRESH [--compiler /usr/bin/g++]
Read: read --path CAPTURE [--check-live]
No execution timeout. SIGINT/SIGTERM cancel and join the entire child group;
the short cancellation grace is not a benchmark or search budget.
"""
from __future__ import annotations

import argparse
import base64
from datetime import datetime, timezone
import difflib
import hashlib
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys

V8 = Path(__file__).resolve().parents[1]
SOURCES = ("src/core/float32_predicates.hpp", "src/spatial/float32_index.hpp",
           "src/spatial/float32_index.cpp", "tests/float32_index_probe.cpp")
SCHEMA = "mhgp8_float32_index_compiled_mutations_v1"
FLAGS = ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
         "-ffp-contract=off", "-fno-fast-math", "-O3", "-DNDEBUG")
ENVIRONMENT = {"PATH": os.defpath, "LANG": "C", "LC_ALL": "C", "TZ": "UTC"}
MUTATIONS = (
    dict(name="signed_zero", source=SOURCES[1],
         before="  if ((word & 0x7fffffffU) == 0) word = 0;\n", after="",
         diagnostic="positive-zero query missed negative-zero coordinates"),
    dict(name="open_box", source=SOURCES[2],
         before="if (nh < ql || nl > qh)", after="if (nh <= ql || nl >= qh)",
         diagnostic="subnormal singleton query lost its site"),
    dict(name="internal_escape", source=SOURCES[2],
         before="  nodes_[current].right = right;\n  nodes_[current].escape = nodes_.size();",
         after="  nodes_[current].right = right;\n  nodes_[current].escape = nodes_.size() - 1;",
         diagnostic="whole box lost original IDs"),
)
CASES = ("baseline", *(item["name"] for item in MUTATIONS))
ARTIFACTS = ("pre_core.d", "pre_test.d", "core.d", "test.d", "core.o", "test.o", "probe")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def stamp():
    return datetime.now(timezone.utc).isoformat()


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def pins(paths):
    return {str(path): digest(path) for path in sorted(map(Path, paths))}


def write_bytes(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as stream:
        stream.write(data)


def write_json(path, value):
    write_bytes(path, (json.dumps(value, indent=2, sort_keys=True, allow_nan=False)+"\n").encode())


def load(path):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            require(key not in result, "duplicate JSON key")
            result[key] = value
        return result
    return json.loads(Path(path).read_bytes(), object_pairs_hook=unique,
                      parse_constant=lambda value: (_ for _ in ()).throw(ValueError(value)))


def mutation(name):
    return next((item for item in MUTATIONS if item["name"] == name), None)


def changed(original, name, source):
    item = mutation(name)
    if item is None or item["source"] != source:
        return original
    before, after = item["before"].encode(), item["after"].encode()
    require(original.count(before) == 1, "mutation site is not unique: "+name)
    return original.replace(before, after, 1)


def plan(manifest):
    compiler, output, build = manifest["compiler"], Path(manifest["output"]), Path(manifest["build"])
    result = [dict(label="compiler_version", command=[compiler, "--version"], case=None, phase="version")]
    for name in CASES:
        source = output / "cases" / name
        target = build / name
        flags = [*FLAGS, "-I", str(source / "src")]
        for phase in ("dependencies", "compile"):
            for unit, filename in (("core", SOURCES[2]), ("test", SOURCES[3])):
                command = [compiler, *flags]
                if phase == "dependencies":
                    command += ["-M", str(source / filename), "-MF", str(target / ("pre_"+unit+".d")),
                                "-MT", str(target / (unit+".o"))]
                else:
                    command += ["-c", str(source / filename), "-MD", "-MF", str(target / (unit+".d")),
                                "-o", str(target / (unit+".o"))]
                result.append(dict(label=name+"_"+phase+"_"+unit, command=command, case=name, phase=phase))
        result.append(dict(label=name+"_link", case=name, phase="link",
                           command=[compiler, *flags, str(target / "core.o"), str(target / "test.o"),
                                    "-o", str(target / "probe")]))
        result.append(dict(label=name+"_selftest", case=name, phase="selftest",
                           command=[str(target / "probe"), "--selftest"]))
    return result


def dependency_paths(data):
    text = data.decode().replace("\\\n", " ")
    require(":" in text, "compiler dependency target missing")
    return {Path(token) for token in shlex.split(text.split(":", 1)[1])}


def dependencies(directory, prefix):
    result = set()
    for unit in ("core", "test"):
        result.update(dependency_paths((directory / (prefix+unit+".d")).read_bytes()))
    require(result and all(path.is_absolute() for path in result), "dependency paths must be absolute")
    return result


def interrupted(number, _frame):
    raise InterruptedError("received signal "+str(number))


def invoke(item, output, cwd):
    label = item["label"]
    stdout_path, stderr_path = output / (label+".stdout"), output / (label+".stderr")
    record = dict(**item, cwd=str(cwd), environment=ENVIRONMENT, started_utc=stamp(), exit_code=None)
    child = None
    try:
        with stdout_path.open("xb") as stdout, stderr_path.open("xb") as stderr:
            # Do not expose an unassigned/unjoinable child at a signal boundary.
            previous = signal.pthread_sigmask(signal.SIG_BLOCK, {signal.SIGINT, signal.SIGTERM})
            try:
                child = subprocess.Popen(item["command"], cwd=cwd, env=ENVIRONMENT,
                                         stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr,
                                         start_new_session=True)
            finally:
                signal.pthread_sigmask(signal.SIG_SETMASK, previous)
            record["exit_code"] = child.wait()
    except BaseException as error:
        record["error"] = f"{type(error).__name__}: {error}"
        handlers = {sig: signal.signal(sig, signal.SIG_IGN) for sig in (signal.SIGINT, signal.SIGTERM)}
        try:
            if child is not None:
                try:
                    os.killpg(child.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                try:
                    child.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(child.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    child.wait()
                record["exit_code"] = child.returncode
        finally:
            for sig, handler in handlers.items():
                signal.signal(sig, handler)
        raise
    finally:
        for name, path in (("stdout", stdout_path), ("stderr", stderr_path)):
            data = path.read_bytes() if path.exists() else b""
            record[name] = data.decode("utf-8", errors="replace")
            record[name+"_base64"] = base64.b64encode(data).decode("ascii")
        record["finished_utc"] = stamp()
        write_json(output / (label+".json"), record)
    return record


def verdict(item, record):
    require("error" not in record, "command collection failed: "+item["label"])
    code = record["exit_code"]
    require(type(code) is int, "missing native exit code")
    if item["phase"] != "selftest":
        require(code == 0, "compiler/dependency/link failure is not a killed mutation")
        return
    if item["case"] == "baseline":
        require(code == 0 and record["stderr"] == "", "unmodified selftest failed")
        value = json.loads(record["stdout"])
        require(value.get("schema") == "mhgp8_float32_index_selftest_v1" and value.get("status") == "passed" and
                type(value.get("tests")) is int and value["tests"] > 0 and
                value.get("rounding_modes") == 4 and value.get("concurrent_queries") == 64,
                "baseline selftest did not exercise its geometry")
    else:
        expected = "float32 index probe: "+mutation(item["case"])["diagnostic"]+"\n"
        require(code == 1 and record["stderr"] == expected and
                record["stdout"] == '{"status":"failed"}\n',
                "mutation survived or failed for a noncausal reason: "+item["case"])


def inventory(output):
    result = {}
    for path in sorted(output.rglob("*")):
        require(not path.is_symlink(), "symlink in capture")
        if path.is_file() and path != output / "COMPLETION.json":
            result[str(path.relative_to(output))] = digest(path)
    return result


def run(args):
    require(os.name == "posix" and hasattr(signal, "pthread_sigmask"), "POSIX process-group collection required")
    # Preserve the driver spelling: resolving clang++'s symlink invokes clang.
    build, output, compiler = args.build.resolve(), args.output.resolve(), args.compiler.absolute()
    require(not build.exists() and not output.exists(), "build and capture must both be fresh")
    require(build != output and not build.is_relative_to(output) and not output.is_relative_to(build),
            "build and capture must be disjoint")
    require(compiler.is_file() and os.access(compiler, os.X_OK), "compiler is not executable")
    sources = [V8 / source for source in SOURCES]
    source_pins = pins([*sources, Path(__file__).resolve()])
    output.mkdir(parents=True, exist_ok=False)
    build.mkdir(parents=True, exist_ok=False)
    manifest = dict(schema=SCHEMA, output=str(output), build=str(build), compiler=str(compiler),
                    compiler_sha256=digest(compiler), source_sha256=source_pins,
                    helper_sha256=digest(Path(__file__)), mutations=list(MUTATIONS), flags=list(FLAGS),
                    environment=ENVIRONMENT, started_utc=stamp(), launch=[sys.executable, *sys.argv],
                    scope="three_causal_mutations_one_native_index_selftest_not_FULL_or_GPU")
    manifest["plan"] = plan(manifest)
    write_json(output / "MANIFEST.json", manifest)
    state = dict(status="failed", commands=[], compiled={}, killed=[], started_utc=stamp(),
                 manifest_sha256=digest(output / "MANIFEST.json"))
    handlers = {sig: signal.signal(sig, interrupted) for sig in (signal.SIGINT, signal.SIGTERM)}
    print(json.dumps(dict(path=str(output), status="starting")), flush=True)
    try:
        for source in SOURCES:
            write_bytes(output / "originals" / source, (V8 / source).read_bytes())
        write_bytes(output / "originals" / "helper.py", Path(__file__).read_bytes())
        for name in CASES:
            (build / name).mkdir()
            patch = []
            for source in SOURCES:
                original = (output / "originals" / source).read_bytes()
                altered = changed(original, name, source)
                write_bytes(output / "cases" / name / source, altered)
                if altered != original:
                    patch.extend(difflib.unified_diff(original.decode().splitlines(keepends=True),
                                 altered.decode().splitlines(keepends=True), fromfile=source, tofile=name+"/"+source))
            write_bytes(output / "cases" / name / "change.patch", "".join(patch).encode())
        for item in manifest["plan"]:
            try:
                record = invoke(item, output, output)
            finally:
                path = output / (item["label"]+".json")
                if path.exists():
                    state["commands"].append(dict(path=path.name, sha256=digest(path)))
            verdict(item, record)
            name = item["case"]
            if name and item["label"] == name+"_dependencies_test":
                dep = dependencies(build / name, "pre_")
                require({output / "cases" / name / source for source in SOURCES} <= dep,
                        "preprocessor omitted a local source")
                before = pins(dep)
                state["compiled"][name] = dict(dependencies_before=before)
                for path, pin in before.items():
                    blob = output / "dependencies" / pin
                    if not blob.exists():
                        write_bytes(blob, Path(path).read_bytes())
                    require(digest(blob) == pin, "dependency changed while being archived")
            if name and item["phase"] == "link":
                detail = state["compiled"][name]
                after = pins(dependencies(build / name, ""))
                require(after == detail["dependencies_before"], "compiled dependencies changed")
                detail["dependencies_after"] = after
                detail["artifacts"] = {}
                for filename in ARTIFACTS:
                    source = build / name / filename
                    destination = output / "artifacts" / name / filename
                    write_bytes(destination, source.read_bytes())
                    detail["artifacts"][str(source)] = dict(stored=str(destination.relative_to(output)), sha256=digest(source))
            if name != "baseline" and item["phase"] == "selftest":
                state["killed"].append(name)
        state["status"] = "passed"
    except BaseException as error:
        state["error"] = f"{type(error).__name__}: {error}"
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        try:
            # Preserve whatever the compiler produced even if a later compile,
            # link or oracle failed. These copies never replace earlier bytes.
            state["partial_build_artifacts"] = {}
            for name in CASES:
                for filename in ARTIFACTS:
                    source = build / name / filename
                    if source.is_file():
                        destination = output / "closure_build" / name / filename
                        write_bytes(destination, source.read_bytes())
                        state["partial_build_artifacts"][str(source)] = dict(
                            stored=str(destination.relative_to(output)), sha256=digest(source))
            state["source_sha256_after"] = pins(source_pins)
            state["compiler_sha256_after"] = digest(compiler)
            require(state["source_sha256_after"] == source_pins and
                    state["compiler_sha256_after"] == manifest["compiler_sha256"] and
                    digest(output / "MANIFEST.json") == state["manifest_sha256"],
                    "original source/compiler/manifest closure changed")
            for name, detail in state["compiled"].items():
                detail["dependencies_closed"] = pins(detail["dependencies_before"])
                require(detail["dependencies_closed"] == detail["dependencies_before"], "dependency closure changed")
                detail["artifacts_closed"] = pins(detail.get("artifacts", {}))
                require(detail["artifacts_closed"] == {path: item["sha256"] for path, item in detail.get("artifacts", {}).items()},
                        "native artifact closure changed")
            state["artifact_sha256"] = inventory(output)
        except BaseException as error:
            state.update(status="failed", closing_error=f"{type(error).__name__}: {error}")
        state["finished_utc"] = stamp()
        write_json(output / "COMPLETION.json", state)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    require(state["status"] == "passed", "mutation qualification failed; all available evidence retained in "+str(output))
    return read(output, check_live=True)


def read(output, check_live=False):
    output = output.resolve(strict=True)
    completion_pin = digest(output / "COMPLETION.json")
    manifest, state = load(output / "MANIFEST.json"), load(output / "COMPLETION.json")
    require(manifest["schema"] == SCHEMA and manifest["mutations"] == list(MUTATIONS) and
            manifest["flags"] == list(FLAGS) and manifest["environment"] == ENVIRONMENT,
            "mutation manifest configuration differs")
    require(inventory(output) == state["artifact_sha256"], "capture artifact closure differs")
    require(digest(output / "MANIFEST.json") == state["manifest_sha256"], "initial manifest hash differs")
    require(digest(output / "originals/helper.py") == manifest["helper_sha256"] == digest(Path(__file__)),
            "reader/helper is not the pinned qualification version")
    require(state["source_sha256_after"] == manifest["source_sha256"] and
            state["compiler_sha256_after"] == manifest["compiler_sha256"], "original inputs changed during capture")
    original_output, build = Path(manifest["output"]), Path(manifest["build"])
    require(manifest["plan"] == plan(manifest), "captured command plan differs")
    for source in SOURCES:
        original = (output / "originals" / source).read_bytes()
        require(hashlib.sha256(original).hexdigest() == manifest["source_sha256"][str(V8 / source)],
                "source snapshot does not match its original pin")
        for name in CASES:
            require((output / "cases" / name / source).read_bytes() == changed(original, name, source),
                    "mutation copy differs from the unique prescribed replacement")
    for name in CASES:
        patch = []
        for source in SOURCES:
            original = (output / "originals" / source).read_bytes()
            altered = changed(original, name, source)
            if altered != original:
                patch.extend(difflib.unified_diff(original.decode().splitlines(keepends=True),
                             altered.decode().splitlines(keepends=True), fromfile=source, tofile=name+"/"+source))
        require((output / "cases" / name / "change.patch").read_bytes() == "".join(patch).encode(),
                "archived patch differs from the prescribed replacement")
    require(state["status"] == "passed" and state["killed"] == list(CASES[1:]), "capture is not a successful three-kill qualification")
    require(len(state["commands"]) == len(manifest["plan"]), "command count differs")
    for item, receipt in zip(manifest["plan"], state["commands"], strict=True):
        require(receipt["path"] == item["label"]+".json" and digest(output / receipt["path"]) == receipt["sha256"],
                "command receipt hash/order differs")
        record = load(output / receipt["path"])
        require(all(record[key] == value for key, value in item.items()) and
                record["cwd"] == str(original_output) and record["environment"] == ENVIRONMENT,
                "command invocation differs from the strict plan")
        for channel in ("stdout", "stderr"):
            data = (output / (item["label"]+"."+channel)).read_bytes()
            require(base64.b64decode(record[channel+"_base64"], validate=True) == data and
                    record[channel] == data.decode("utf-8", errors="replace"), "raw/base64/text stream mismatch")
        verdict(item, record)
    require(set(state["compiled"]) == set(CASES), "compiled evidence inventory differs")
    require(set(state["partial_build_artifacts"]) == {
        str(build / name / filename) for name in CASES for filename in ARTIFACTS},
        "final build artifact inventory differs")
    for name in CASES:
        detail = state["compiled"][name]
        before = detail["dependencies_before"]
        require(before == detail["dependencies_after"] == detail["dependencies_closed"], "dependency hashes did not close")
        archived = output / "artifacts" / name
        require(dependencies(archived, "pre_") == dependencies(archived, "") == set(map(Path, before)),
                "recorded dependencies differ from both compiler dependency files")
        require({original_output / "cases" / name / source for source in SOURCES} <= set(map(Path, before)),
                "local inputs absent from compiler dependencies")
        for path, pin in before.items():
            require(digest(output / "dependencies" / pin) == pin, "archived compiler dependency changed")
            local = Path(path)
            if local.is_relative_to(original_output / "cases" / name):
                relative = local.relative_to(original_output)
                require(digest(output / relative) == pin, "compiled source differs from prescribed mutation")
        require(set(detail["artifacts"]) == {str(build / name / filename) for filename in ARTIFACTS},
                "compiled artifact set differs")
        for path, item in detail["artifacts"].items():
            expected = str(Path("artifacts") / name / Path(path).name)
            require(item["stored"] == expected and digest(output / expected) == item["sha256"] == detail["artifacts_closed"][path],
                    "binary/object/dependency artifact hash differs")
            closed = state["partial_build_artifacts"][path]
            expected_closed = str(Path("closure_build") / name / Path(path).name)
            require(closed["stored"] == expected_closed and digest(output / expected_closed) ==
                    closed["sha256"] == item["sha256"], "final compiler artifact copy differs")
        if check_live:
            require(pins(before) == before and pins(detail["artifacts"]) == detail["artifacts_closed"],
                    "live build/dependencies differ from the closed capture")
    if check_live:
        require(pins(manifest["source_sha256"]) == manifest["source_sha256"] and
                digest(manifest["compiler"]) == manifest["compiler_sha256"], "live original sources/compiler differ")
    require(inventory(output) == state["artifact_sha256"] and
            digest(output / "COMPLETION.json") == completion_pin, "capture changed during reading")
    return dict(schema=SCHEMA, status="passed", path=str(output), killed=list(CASES[1:]),
                commands=len(manifest["plan"]), check_live=check_live,
                manifest_sha256=digest(output / "MANIFEST.json"),
                completion_sha256=completion_pin, reader_sha256=digest(Path(__file__)),
                scope=manifest["scope"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    if len(sys.argv) > 1 and sys.argv[1] == "read":
        parser.add_argument("--path", type=Path, required=True)
        parser.add_argument("--check-live", action="store_true")
        args = parser.parse_args(sys.argv[2:])
        result = read(args.path, args.check_live)
    else:
        parser.add_argument("--build", type=Path, required=True)
        parser.add_argument("--output", type=Path, required=True)
        parser.add_argument("--compiler", type=Path, default=Path("/usr/bin/g++"))
        result = run(parser.parse_args())
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except BaseException as error:
        print(json.dumps(dict(schema=SCHEMA, status="failed", error=f"{type(error).__name__}: {error}")), file=sys.stderr)
        sys.exit(1)
