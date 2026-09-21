#!/usr/bin/env python3
"""Compile two isolated prefix/shell errors and judge complete Fraction payloads.

Only fresh source copies are changed. A nonzero exit, crash, malformed output
or counter-only difference does not kill a mutant. Native input, commands,
sources, dependencies and binaries are closed before any PASS is published.
"""
from __future__ import annotations

import argparse
import base64
import difflib
import json
import os
from pathlib import Path
import signal
import sys

import float32_ball_mutations as capture
import float32_q3_census_gate as oracle

V8 = Path(__file__).resolve().parents[1]
require, stamp, digest, pins = capture.require, capture.stamp, capture.digest, capture.pins
write_bytes, write_json, load = capture.write_bytes, capture.write_json, capture.load
SCHEMA = "mhgp8_float32_q3_census_compiled_mutations_v1"
FLAGS, ENVIRONMENT = capture.FLAGS, capture.ENVIRONMENT
LOCAL = ("src/core/float32_predicates.hpp", "src/core/fixed_signed.hpp",
         "src/core/float32_ball.hpp", "src/core/float32_ball.cpp",
         "src/spatial/float32_index.hpp", "src/spatial/float32_index.cpp",
         "src/core/float32_q3_block.hpp", "src/core/float32_q3_block.cpp",
         "src/lanes/float32_q3_census.hpp", "src/lanes/float32_q3_census.cpp",
         "tests/float32_q3_census_probe.cpp")
HELPERS = ("tests/float32_q3_census_gate.py", "tests/float32_identity_gate.py",
           "tests/float32_q3_census_mutations.py", "tests/float32_ball_mutations.py",
           "tests/float32_ball_gate.py", "tests/float32_index_mutation_gate.py", "bench/run_p0_matrix.py")
SOURCES = (*LOCAL, *HELPERS)
UNITS = dict(ball="src/core/float32_ball.cpp", index="src/spatial/float32_index.cpp",
             block="src/core/float32_q3_block.cpp", census="src/lanes/float32_q3_census.cpp",
             test="tests/float32_q3_census_probe.cpp")
TARGET = "src/lanes/float32_q3_census.cpp"
MUTATIONS = (
    dict(name="prefix_recounted_from_root", edits=[
        ["      auto cursor = ticket.cursor;", "      std::size_t cursor = 0; // MUTANT: recount credited prefix."]],
         property="a relayed seed must not recount the already credited witness prefix"),
    dict(name="shell_prefix_omitted", edits=[
        ["      collect_shell(*ball, prepared);", "      collect_shell(*ball, prepared, ticket.cursor);"],
        ["  void collect_shell(const Float32Ball& ball, const Float32Q3Block& prepared) {",
         "  void collect_shell(const Float32Ball& ball, const Float32Q3Block& prepared, std::size_t cursor) {"],
        ["    std::size_t cursor = 0;", "    // MUTANT: omit all contacts preceding the count cursor."]],
         property="every accepted support must recover shell contacts from the entire global index"),
)
CASES = ("baseline", *(m["name"] for m in MUTATIONS))
ARTIFACTS = (*(prefix+unit+suffix for prefix, suffix in (("pre_", ".d"), ("", ".d"), ("", ".o"))
               for unit in UNITS), "probe")


def fixture():
    points = [(-1, 0, 0), (1, 0, 0)]+[(0, 2+i/4, 0) for i in range(7)]
    return dict(name="nested_column_prefix_and_global_contacts", points=oracle.encoded(points),
                requests=[dict(a=0, b=1, node=0, kmax=5, mode=mode, grain=1) for mode in (0, 1)])


def expected():
    case = fixture()
    rows = []
    for request in case["requests"]:
        accepted, _, _ = oracle.scalar_census(case, request, range(len(case["points"])))
        rows.append([accepted[seed] for seed in sorted(accepted)])
    require(rows[0] == rows[1] == [dict(seed=i+2, depth=i, shell=[0, 1, i+2]) for i in range(4)],
            "independent column fixture changed")
    return rows


def mutation(name):
    return next((item for item in MUTATIONS if item["name"] == name), None)


def changed(data, case, name):
    item = mutation(case)
    if item is None or name != TARGET:
        return data
    for before, after in item["edits"]:
        require(data.count(before.encode()) == 1, "mutation site not unique: " + case)
        data = data.replace(before.encode(), after.encode(), 1)
    return data


def plan(manifest):
    output, build, compiler = Path(manifest["output"]), Path(manifest["build"]), manifest["compiler"]
    rows = [dict(label="compiler_version", command=[compiler, "--version"], case=None, phase="version", stdin=None)]
    for case in CASES:
        source, target = output/"cases"/case, build/case
        flags = [*FLAGS, "-I", str(source/"src")]
        for phase in ("dependencies", "compile"):
            for unit, filename in UNITS.items():
                command = [compiler, *flags]
                if phase == "dependencies":
                    command += ["-M", str(source/filename), "-MF", str(target/("pre_"+unit+".d")), "-MT", str(target/(unit+".o"))]
                else:
                    command += ["-c", str(source/filename), "-MD", "-MF", str(target/(unit+".d")), "-o", str(target/(unit+".o"))]
                rows.append(dict(label=case+"_"+phase+"_"+unit, command=command, case=case, phase=phase, stdin=None))
        rows.append(dict(label=case+"_link", command=[compiler, *flags,
                         *(str(target/(unit+".o")) for unit in UNITS), "-o", str(target/"probe")],
                         case=case, phase="link", stdin=None))
        rows.append(dict(label=case+"_geometry", command=[str(target/"probe")], case=case, phase="geometry",
                         stdin=str(output/"fixture.txt")))
    return rows


def judgment(item, record):
    require(type(record["exit_code"]) is int and record["exit_code"] == 0 and "error" not in record,
            "compile/crash/nonzero exit is not a geometric kill: " + item["label"])
    if item["phase"] != "geometry":
        return None
    require(record["stderr"] == "" and len(record["stdout"].splitlines()) == 1, "native geometric response malformed")
    row = oracle.strict_json(record["stdout"])
    require(type(row) is dict and set(row) == {"schema", "status", "points", "permutation", "nodes", "queries"} and
            row["schema"] == "mhgp8_float32_q3_census_probe_v1" and row["status"] == "passed", "native census row shape differs")
    case = fixture()
    oracle.validate_tree(row, case)
    require(type(row["queries"]) is list and len(row["queries"]) == 2, "native census query count differs")
    actual = []
    for query, request in zip(row["queries"], case["requests"], strict=True):
        require(type(query) is dict and set(query) == {"request", "emissions", "work"} and
                query["request"] == request and all(type(v) is int for v in query["request"].values()),
                "native census request shape differs")
        require(type(query["emissions"]) is list, "native census emission list missing")
        emissions, seen = [], set()
        for value in query["emissions"]:
            require(type(value) is dict and set(value) == {"seed", "depth", "shell"} and
                    type(value["seed"]) is int and type(value["depth"]) is int and
                    0 <= value["seed"] < len(case["points"]) and 0 <= value["depth"] < request["kmax"]-1 and
                    value["seed"] not in seen and type(value["shell"]) is list and
                    all(type(i) is int and 0 <= i < len(case["points"]) for i in value["shell"]) and
                    len(set(value["shell"])) == len(value["shell"]), "native census payload malformed or duplicated")
            seen.add(value["seed"])
            emissions.append(dict(seed=value["seed"], depth=value["depth"], shell=sorted(value["shell"])))
        actual.append(sorted(emissions, key=lambda e: e["seed"]))
    truth = expected()
    # Compare geometry BEFORE consulting any counter. A missing support,
    # wrong strict depth or missing shell contact is the only mutation judge.
    require(actual[0] == truth[0], "unmodified individual reference differs from Fraction")
    killed = item["case"] != "baseline"
    require((actual[1] != truth[1]) if killed else (actual[1] == truth[1]),
            "shared mutant survived or baseline geometry differs from Fraction")
    if not killed:
        work = row["queries"][1]["work"]
        require(type(work["relays_with_credit"]) is int and work["relays_with_credit"] > 0 and
                type(work["shared_endpoint_skips"]) is int and work["shared_endpoint_skips"] > 0,
                "fixture did not exercise certified credit and prefix endpoint consumption")
    return dict(case=item["case"], expected=truth, actual=actual, geometric_kill=killed)


def dependencies(directory, prefix):
    result = set()
    for unit in UNITS:
        result.update(capture.utility.dependency_paths((directory/(prefix+unit+".d")).read_bytes()))
    require(result and all(path.is_absolute() for path in result), "compiled dependency paths must be absolute")
    return result


def run(args):
    require(os.name == "posix", "POSIX process-group/descriptor collector required")
    output, build, compiler = args.output.resolve(), args.build.resolve(), args.compiler.absolute()
    require(not output.exists() and not build.exists(), "capture and build must be fresh")
    require(not output.is_relative_to(build) and not build.is_relative_to(output), "capture and build overlap")
    require(compiler.is_file() and os.access(compiler, os.X_OK), "compiler unavailable")
    original = pins([*(V8/name for name in SOURCES), compiler])
    output.mkdir(parents=True); build.mkdir(parents=True)
    manifest = dict(schema=SCHEMA, output=str(output), build=str(build), compiler=str(compiler), source_sha256=original,
                    flags=list(FLAGS), environment=ENVIRONMENT, mutations=list(MUTATIONS), fixture=fixture(), expected=expected(),
                    launch=[sys.executable, *sys.argv], started_utc=stamp(), system_headers_archived=False,
                    scope="two_prefix_shell_mutations_of_one_supplied_edge_not_WSPD_FULL_or_GPU")
    manifest["plan"] = plan(manifest)
    write_json(output/"MANIFEST.json", manifest)
    state = dict(status="failed", manifest_sha256=digest(output/"MANIFEST.json"), commands=[], compiled={}, judgments=[], killed=[])
    handlers = {sig: signal.signal(sig, capture.on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    print(str(output), flush=True)
    try:
        write_bytes(output/"fixture.txt", oracle.payload(fixture()))
        for name in SOURCES:
            write_bytes(output/"originals"/name, (V8/name).read_bytes())
        for case in CASES:
            (build/case).mkdir()
            patch = []
            for name in LOCAL:
                data = (output/"originals"/name).read_bytes()
                altered = changed(data, case, name)
                write_bytes(output/"cases"/case/name, altered)
                if altered != data:
                    patch.extend(difflib.unified_diff(data.decode().splitlines(True), altered.decode().splitlines(True),
                                                      fromfile=name, tofile=case+"/"+name))
            write_bytes(output/"cases"/case/"change.patch", "".join(patch).encode())
        for item in manifest["plan"]:
            try:
                record = capture.execute(item, output)
            finally:
                path = output/(item["label"]+".json")
                if path.exists(): state["commands"].append(dict(path=path.name, sha256=digest(path)))
            decision = judgment(item, record)
            case = item["case"]
            if item["label"] == str(case)+"_dependencies_test":
                dep = dependencies(build/case, "pre_")
                require({output/"cases"/case/name for name in LOCAL} <= dep, "compiler omitted local source")
                state["compiled"][case] = dict(dependencies_before=pins(dep))
            if item["phase"] == "link":
                detail = state["compiled"][case]
                detail["dependencies_after"] = pins(dependencies(build/case, ""))
                require(detail["dependencies_before"] == detail["dependencies_after"], "dependencies changed while compiling")
                detail["artifacts_before"] = pins(build/case/name for name in ARTIFACTS)
            if decision is not None:
                state["judgments"].append(decision)
                if decision["geometric_kill"]: state["killed"].append(case)
        state["status"] = "passed"
    except BaseException as error:
        state["error"] = f"{type(error).__name__}: {error}"
    finally:
        for sig in handlers: signal.signal(sig, signal.SIG_IGN)
        try:
            state["source_sha256_after"] = pins(original)
            require(state["source_sha256_after"] == original and digest(output/"MANIFEST.json") == state["manifest_sha256"],
                    "sources/compiler/manifest changed")
            for detail in state["compiled"].values():
                detail["dependencies_closed"] = pins(detail["dependencies_before"])
                require(detail["dependencies_closed"] == detail["dependencies_before"], "compiled dependencies changed at closure")
                if "artifacts_before" in detail:
                    detail["artifacts_closed"] = pins(detail["artifacts_before"])
                    require(detail["artifacts_closed"] == detail["artifacts_before"], "compiled artifacts changed at closure")
            state["available_build_artifacts"] = pins(p for p in build.rglob("*") if p.is_file())
            state["artifact_sha256"] = capture.inventory(output)
        except BaseException as error:
            state.update(status="failed", closing_error=f"{type(error).__name__}: {error}")
        state["finished_utc"] = stamp(); write_json(output/"COMPLETION.json", state)
        for sig, handler in handlers.items(): signal.signal(sig, handler)
    require(state["status"] == "passed", "mutation capture failed; evidence preserved: " + str(output))
    return read(output, True)


def read(output, check_live=False):
    output = output.resolve(strict=True)
    closure_pin = digest(output/"COMPLETION.json")
    manifest, state = load(output/"MANIFEST.json"), load(output/"COMPLETION.json")
    live = {}
    if check_live:
        groups = [manifest["source_sha256"], state["available_build_artifacts"]]
        for detail in state["compiled"].values(): groups.extend((detail["dependencies_closed"], detail["artifacts_closed"]))
        for group in groups:
            for name, value in group.items():
                require(name not in live or live[name] == value, "conflicting live pins")
                live[name] = value
        require(pins(live) == live, "live inputs changed before read")
    require(manifest["schema"] == SCHEMA and manifest["mutations"] == list(MUTATIONS) and manifest["flags"] == list(FLAGS) and
            manifest["environment"] == ENVIRONMENT and manifest["system_headers_archived"] is False and
            manifest["plan"] == plan(manifest), "manifest configuration differs")
    require(capture.inventory(output) == state["artifact_sha256"] and
            digest(output/"MANIFEST.json") == state["manifest_sha256"], "capture closure differs")
    require(state["status"] == "passed" and state["killed"] == list(CASES[1:]) and
            state["source_sha256_after"] == manifest["source_sha256"], "capture is not closed two-kill PASS")
    require(manifest["fixture"] == json.loads(json.dumps(fixture())) and manifest["expected"] == expected() and
            (output/"fixture.txt").read_bytes() == oracle.payload(fixture()), "fixture/oracle input differs")
    for name in SOURCES:
        require(digest(output/"originals"/name) == manifest["source_sha256"][str(V8/name)], "archived source mismatch")
    for name in HELPERS:
        require(digest(V8/name) == manifest["source_sha256"][str(V8/name)], "historical reader needs pinned helper version")
    original_output, build = Path(manifest["output"]), Path(manifest["build"])
    for case in CASES:
        patch = []
        for name in LOCAL:
            original = (output/"originals"/name).read_bytes()
            altered = changed(original, case, name)
            require((output/"cases"/case/name).read_bytes() == altered, "mutated source mismatch")
            if altered != original:
                patch.extend(difflib.unified_diff(original.decode().splitlines(True), altered.decode().splitlines(True),
                                                  fromfile=name, tofile=case+"/"+name))
        require((output/"cases"/case/"change.patch").read_bytes() == "".join(patch).encode(), "mutation patch differs")
    require(len(state["commands"]) == len(manifest["plan"]), "command count differs")
    judgments = []
    for item, entry in zip(manifest["plan"], state["commands"], strict=True):
        require(entry["path"] == item["label"]+".json" and digest(output/entry["path"]) == entry["sha256"], "command receipt order/hash differs")
        record = load(output/entry["path"])
        require(all(record[k] == v for k, v in item.items()) and record["cwd"] == str(original_output) and
                record["environment"] == ENVIRONMENT, "raw command invocation differs")
        for channel in ("stdout", "stderr"):
            require(base64.b64decode(record[channel+"_base64"], validate=True).decode("utf-8", errors="replace") == record[channel],
                    "raw/text output differs")
        decision = judgment(item, record)
        if decision is not None: judgments.append(decision)
    require(judgments == state["judgments"] and set(state["compiled"]) == set(CASES), "judgments/compiled cases differ")
    artifacts = {}
    for case, detail in state["compiled"].items():
        require(detail["dependencies_before"] == detail["dependencies_after"] == detail["dependencies_closed"], "dependency closure differs")
        require(detail["artifacts_before"] == detail["artifacts_closed"], "native artifact closure differs")
        dep = set(map(Path, detail["dependencies_before"]))
        require({original_output/"cases"/case/name for name in LOCAL} <= dep, "compiled local source inventory incomplete")
        for name in LOCAL:
            require(detail["dependencies_before"][str(original_output/"cases"/case/name)] == digest(output/"cases"/case/name),
                    "compiled source differs from mutation copy")
        require(set(detail["artifacts_before"]) == {str(build/case/name) for name in ARTIFACTS}, "compiled artifact inventory differs")
        artifacts.update(detail["artifacts_closed"])
        if check_live:
            require(dependencies(build/case, "pre_") == dependencies(build/case, "") == dep, "live dependency inventory differs")
    require(state["available_build_artifacts"] == artifacts, "available artifact inventory differs")
    require(capture.inventory(output) == state["artifact_sha256"] and digest(output/"COMPLETION.json") == closure_pin,
            "capture changed during read")
    if check_live: require(pins(live) == live, "live sources/dependencies/build changed during read")
    return dict(schema=SCHEMA, status="passed", path=str(output), check_live=check_live, commands=len(manifest["plan"]),
                fixtures=1, requests=2, killed=list(CASES[1:]), manifest_sha256=digest(output/"MANIFEST.json"),
                completion_sha256=closure_pin, reader_sha256=digest(Path(__file__)), native_reexecuted=False, scope=manifest["scope"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    if len(sys.argv) > 1 and sys.argv[1] == "read":
        parser.add_argument("--path", type=Path, required=True); parser.add_argument("--check-live", action="store_true")
        args = parser.parse_args(sys.argv[2:]); result = read(args.path, args.check_live)
    else:
        parser.add_argument("--build", type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
        parser.add_argument("--compiler", type=Path, default=Path("/usr/bin/g++"))
        args = parser.parse_args(sys.argv[2:] if len(sys.argv) > 1 and sys.argv[1] == "run" else None)
        result = run(args)
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except BaseException as error:
        print(json.dumps(dict(schema=SCHEMA, status="failed", error=f"{type(error).__name__}: {error}")), file=sys.stderr)
        sys.exit(1)
