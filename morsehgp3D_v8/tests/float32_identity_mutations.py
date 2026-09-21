#!/usr/bin/env python3
"""Two compiled geometric mutations: global ball identity and oriented roots.

Only fresh copied sources are modified. Reuses the prior process-group/raw
command collector, not its oracle or mutation judgment. Independent Fraction
centre/radius and root oracles judge normal-exit native fixture responses.
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
import float32_identity_gate as oracle

V8 = Path(__file__).resolve().parents[1]
require, stamp, digest, pins = capture.require, capture.stamp, capture.digest, capture.pins
write_bytes, write_json, load = capture.write_bytes, capture.write_json, capture.load
SCHEMA = "mhgp8_float32_identity_compiled_mutations_v1"
FLAGS, ENVIRONMENT = capture.FLAGS, capture.ENVIRONMENT
LOCAL = ("src/core/float32_predicates.hpp", "src/core/fixed_signed.hpp",
         "src/core/float32_ball.hpp", "src/core/float32_ball.cpp",
         "src/core/float32_ball_key.hpp", "src/core/float32_ball_key.cpp",
         "src/core/float32_q4_events.hpp", "src/core/float32_q4_events.cpp",
         "tests/float32_key_probe.cpp", "tests/float32_q4_events_probe.cpp")
HELPERS = ("tests/float32_identity_gate.py", "tests/float32_identity_mutations.py",
           "tests/float32_ball_mutations.py", "tests/float32_ball_gate.py",
           "tests/float32_index_mutation_gate.py", "bench/run_p0_matrix.py")
SOURCES = (*LOCAL, *HELPERS)
UNITS = dict(ball="src/core/float32_ball.cpp", key="src/core/float32_ball_key.cpp",
             events="src/core/float32_q4_events.cpp", key_test="tests/float32_key_probe.cpp",
             events_test="tests/float32_q4_events_probe.cpp")
MUTATIONS = (
    dict(name="global_translation_omitted", source="src/core/float32_ball.cpp", occurrences=1,
         before="  // A*|Q-a|^2-W.(Q-a), expanded in the single GLOBAL unit 2^-149.",
         after="  exact.origin = {};  // MUTANT: interpret local coordinates as global.\n"
               "  // A*|Q-a|^2-W.(Q-a), expanded in the single GLOBAL unit 2^-149.",
         fixture="key", property="translated acute support must retain its global centre and radius"),
    dict(name="second_denominator_sign_omitted", source="src/core/float32_q4_events.cpp", occurrences=2,
         before="return -sign * b1 * b2;", after="return -sign * b1;",
         fixture="events", property="root comparison must retain the second negative denominator sign"),
)
CASES = ("baseline", *(m["name"] for m in MUTATIONS))
ARTIFACTS = (*(prefix+unit+suffix for prefix, suffix in (("pre_", ".d"), ("", ".d"), ("", ".o"))
               for unit in UNITS), "key_probe", "events_probe")
KEY_WORK = ("q2_requests", "q3_requests", "q4_requests", "from_support_requests", "rejected_supports",
            "keys_created", "canonical_gcd_calls", "canonical_divisions", "packed_words")
EVENT_WORK = ("preparations", "accepted_seeds", "rejected_seeds", "interval_preparations",
              "side_queries", "side_filter_attempts", "side_filter_accepts", "side_exact_fallbacks",
              "side_exact_evaluations", "root_queries", "root_coplanar_rejections", "root_filter_attempts",
              "root_filter_accepts", "root_exact_fallbacks", "root_exact_evaluations", "root_equalities",
              "interval_additions", "interval_products", "exact_additions", "exact_products", "exact_point_decodes")


def fixtures():
    return dict(key=dict(support=oracle.encoded(((2, 0, 0), (4, 0, 0), (3, 2, 0)))),
                events=dict(seed=oracle.encoded(((-1, 0, 0), (1, 0, 0), (0, 2, 0))),
                            first=oracle.encoded(((0, 0, 2),))[0],
                            second=oracle.encoded(((0, 0, -2),))[0]))


def expected():
    case = fixtures()
    coefficients = oracle.key_oracle(case["key"]["support"])
    unit = 1 << 149
    require(coefficients == (1, -6*unit, -3*(unit//2), 0, 8*unit*unit), "translated key oracle changed")
    events = oracle.event_oracle(**case["events"])
    require(events == dict(valid=True, sides=[1, -1], order=1, coplanar=False), "signed root oracle changed")
    return dict(key=dict(valid=True, words=oracle.pack(coefficients), coefficients=oracle.coefficients_hex(coefficients)),
                events=events)


def payload(kind):
    return (oracle.key_payload if kind == "key" else oracle.event_payload)([fixtures()[kind]])


def mutation(name):
    return next((m for m in MUTATIONS if m["name"] == name), None)


def changed(data, name, source):
    item = mutation(name)
    if item is None or item["source"] != source:
        return data
    before, after = item["before"].encode(), item["after"].encode()
    require(data.count(before) == item["occurrences"], "mutation site count changed: " + name)
    return data.replace(before, after)


def plan(manifest):
    output, build = Path(manifest["output"]), Path(manifest["build"])
    compiler = manifest["compiler"]
    result = [dict(label="compiler_version", command=[compiler, "--version"], case=None, phase="version", stdin=None)]
    for name in CASES:
        source, target = output/"cases"/name, build/name
        flags = [*FLAGS, "-I", str(source/"src")]
        for phase in ("dependencies", "compile"):
            for unit, filename in UNITS.items():
                command = [compiler, *flags]
                if phase == "dependencies":
                    command += ["-M", str(source/filename), "-MF", str(target/("pre_"+unit+".d")), "-MT", str(target/(unit+".o"))]
                else:
                    command += ["-c", str(source/filename), "-MD", "-MF", str(target/(unit+".d")), "-o", str(target/(unit+".o"))]
                result.append(dict(label=name+"_"+phase+"_"+unit, command=command, case=name, phase=phase, stdin=None))
        for kind in ("key", "events"):
            objects = [target/(unit+".o") for unit in ("ball", "key", "events", kind+"_test")]
            result.append(dict(label=name+"_link_"+kind,
                               command=[compiler, *flags, *map(str, objects), "-o", str(target/(kind+"_probe"))],
                               case=name, phase="link", stdin=None))
        for kind in ("key", "events"):
            result.append(dict(label=name+"_geometry_"+kind, command=[str(target/(kind+"_probe"))],
                               case=name, phase="geometry", kind=kind, stdin=str(output/(kind+"_fixture.txt"))))
    return result


def work_shape(value, scalars, nested):
    require(type(value) is dict and set(value) == {*scalars, nested}, "native work inventory changed")
    require(all(type(value[name]) is int and 0 <= value[name] < 2**64 for name in scalars), "native work type changed")
    child = value[nested]
    require(type(child) is dict and set(child) == set(capture.oracle.FIELDS) and
            all(type(v) is int and 0 <= v < 2**64 for v in child.values()), "nested support work changed")


def judgment(item, record):
    require(type(record["exit_code"]) is int and record["exit_code"] == 0 and "error" not in record,
            "compile/crash/nonzero exit is not a geometric kill: " + item["label"])
    if item["phase"] != "geometry":
        return None
    require(record["stderr"] == "", "native geometric probe printed an error")
    lines = record["stdout"].splitlines()
    require(len(lines) == 1, "expected exactly one native fixture result")
    row = oracle.strict_json(lines[0])
    kind = item["kind"]
    truth = expected()[kind]
    if kind == "key":
        require(type(row) is dict and set(row) == {"arity", "valid", "words", "coefficients", "work",
                                                  "from_support_words", "from_support_work"}, "key response shape differs")
        require(type(row["arity"]) is int and row["arity"] == 3 and row["valid"] is True,
                "key mutation changed support eligibility instead of identity")
        require(type(row["words"]) is list and all(type(w) is int and 0 <= w < 2**32 for w in row["words"]),
                "invalid serialized key word type")
        require(type(row["coefficients"]) is list and len(row["coefficients"]) == 5 and
                all(type(c) is str for c in row["coefficients"]), "invalid key coefficient type")
        coefficients = tuple(int(c, 16) for c in row["coefficients"])
        require(oracle.coefficients_hex(coefficients) == row["coefficients"] and oracle.pack(coefficients) == row["words"],
                "native key coefficient export disagrees with packed identity")
        require(row["from_support_words"] == row["words"], "two key construction paths disagree")
        work_shape(row["work"], KEY_WORK, "support")
        work_shape(row["from_support_work"], KEY_WORK, "support")
        actual = dict(valid=True, words=row["words"], coefficients=row["coefficients"])
    else:
        names = ("valid", "sides", "order", "coplanar", "preparation_work", "side_work", "comparison_work")
        require(type(row) is dict and set(row) == {p+"_"+n for p in ("filtered", "exact") for n in names},
                "events response shape differs")
        decisions = []
        for prefix in ("filtered", "exact"):
            require(row[prefix+"_valid"] is True and row[prefix+"_coplanar"] is False and
                    type(row[prefix+"_sides"]) is list and len(row[prefix+"_sides"]) == 2 and
                    all(type(v) is int and v in (-1, 1) for v in row[prefix+"_sides"]),
                    "root mutation changed eligibility or denominator classification")
            order = row[prefix+"_order"]
            require(type(order) is int and order in (-1, 0, 1), "invalid root order type")
            require(row[prefix+"_sides"] == truth["sides"], "root mutation changed side predicates")
            for phase in ("preparation", "side", "comparison"):
                work_shape(row[prefix+"_"+phase+"_work"], EVENT_WORK, "seed")
            decisions.append(dict(valid=True, sides=row[prefix+"_sides"], order=order, coplanar=False))
        require(decisions[0] == decisions[1], "filtered/exact root decisions differ")
        actual = decisions[1]
    target = mutation(item["case"])
    killed = target is not None and target["fixture"] == kind
    require((actual != truth) if killed else (actual == truth),
            "target mutation survived or unmodified geometry changed: " + item["label"])
    return dict(case=item["case"], fixture=kind, expected=truth, actual=actual, geometric_kill=killed)


def dependencies(directory, prefix):
    result = set()
    for unit in UNITS:
        result.update(capture.utility.dependency_paths((directory/(prefix+unit+".d")).read_bytes()))
    require(result and all(path.is_absolute() for path in result), "dependency paths must be absolute")
    return result


def run(args):
    require(os.name == "posix", "POSIX process-group/descriptor collector required")
    output, build, compiler = args.output.resolve(), args.build.resolve(), args.compiler.absolute()
    require(not output.exists() and not build.exists(), "capture and build must be fresh")
    require(not output.is_relative_to(build) and not build.is_relative_to(output), "capture and build overlap")
    require(compiler.is_file() and os.access(compiler, os.X_OK), "compiler unavailable")
    original = pins([*(V8/name for name in SOURCES), compiler])
    output.mkdir(parents=True); build.mkdir(parents=True)
    manifest = dict(schema=SCHEMA, output=str(output), build=str(build), compiler=str(compiler),
                    source_sha256=original, flags=list(FLAGS), environment=ENVIRONMENT, mutations=list(MUTATIONS),
                    fixtures=fixtures(), expected=expected(), launch=[sys.executable, *sys.argv], started_utc=stamp(),
                    system_headers_archived=False, scope="two_targeted_identity_root_mutations_not_catalogue_FULL_or_GPU")
    manifest["plan"] = plan(manifest)
    write_json(output/"MANIFEST.json", manifest)
    state = dict(status="failed", manifest_sha256=digest(output/"MANIFEST.json"), commands=[], compiled={}, judgments=[], killed=[])
    handlers = {sig: signal.signal(sig, capture.on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    print(str(output), flush=True)
    try:
        for kind in ("key", "events"):
            write_bytes(output/(kind+"_fixture.txt"), payload(kind))
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
            result = judgment(item, record)
            case = item["case"]
            if item["label"] == str(case)+"_dependencies_events_test":
                dep = dependencies(build/case, "pre_")
                require({output/"cases"/case/name for name in LOCAL} <= dep, "compiler omitted local source")
                state["compiled"][case] = dict(dependencies_before=pins(dep))
            if item["label"] == str(case)+"_link_events":
                detail = state["compiled"][case]
                detail["dependencies_after"] = pins(dependencies(build/case, ""))
                require(detail["dependencies_before"] == detail["dependencies_after"], "dependencies changed while compiling")
                detail["artifacts_before"] = pins(build/case/name for name in ARTIFACTS)
            if result is not None:
                state["judgments"].append(result)
                if result["geometric_kill"]: state["killed"].append(case)
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
        for detail in state["compiled"].values():
            groups.extend((detail["dependencies_closed"], detail["artifacts_closed"]))
        for group in groups:
            for name, value in group.items():
                require(name not in live or live[name] == value, "conflicting live pins")
                live[name] = value
        require(pins(live) == live, "live inputs changed before read")
    require(manifest["schema"] == SCHEMA and manifest["mutations"] == list(MUTATIONS) and
            manifest["flags"] == list(FLAGS) and manifest["environment"] == ENVIRONMENT and
            manifest["system_headers_archived"] is False and manifest["plan"] == plan(manifest), "manifest configuration differs")
    require(capture.inventory(output) == state["artifact_sha256"] and
            digest(output/"MANIFEST.json") == state["manifest_sha256"], "capture closure differs")
    require(state["status"] == "passed" and state["killed"] == list(CASES[1:]) and
            state["source_sha256_after"] == manifest["source_sha256"], "capture is not closed two-kill PASS")
    require(manifest["fixtures"] == json.loads(json.dumps(fixtures())) and manifest["expected"] == expected(), "fixture oracle differs")
    for kind in ("key", "events"):
        require((output/(kind+"_fixture.txt")).read_bytes() == payload(kind), "native fixture input differs")
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
    expected_artifacts = {}
    for case, detail in state["compiled"].items():
        require(detail["dependencies_before"] == detail["dependencies_after"] == detail["dependencies_closed"], "dependency closure differs")
        require(detail["artifacts_before"] == detail["artifacts_closed"], "native artifact closure differs")
        dep = set(map(Path, detail["dependencies_before"]))
        require({original_output/"cases"/case/name for name in LOCAL} <= dep, "compiled local source inventory incomplete")
        for name in LOCAL:
            require(detail["dependencies_before"][str(original_output/"cases"/case/name)] == digest(output/"cases"/case/name),
                    "compiled source differs from mutation copy")
        require(set(detail["artifacts_before"]) == {str(build/case/name) for name in ARTIFACTS}, "compiled artifact inventory differs")
        expected_artifacts.update(detail["artifacts_closed"])
        if check_live:
            require(dependencies(build/case, "pre_") == dependencies(build/case, "") == dep, "live dependency file inventory differs")
    require(state["available_build_artifacts"] == expected_artifacts, "available build artifact inventory differs")
    require(capture.inventory(output) == state["artifact_sha256"] and digest(output/"COMPLETION.json") == closure_pin,
            "capture changed during read")
    if check_live:
        require(pins(live) == live, "live sources/dependencies/build changed during read")
    return dict(schema=SCHEMA, status="passed", path=str(output), check_live=check_live, commands=len(manifest["plan"]),
                fixtures=2, killed=list(CASES[1:]), manifest_sha256=digest(output/"MANIFEST.json"),
                completion_sha256=closure_pin, reader_sha256=digest(Path(__file__)), native_reexecuted=False, scope=manifest["scope"])


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
        args = parser.parse_args(sys.argv[2:] if len(sys.argv) > 1 and sys.argv[1] == "run" else None)
        result = run(args)
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except BaseException as error:
        print(json.dumps(dict(schema=SCHEMA, status="failed", error=f"{type(error).__name__}: {error}")), file=sys.stderr)
        sys.exit(1)
