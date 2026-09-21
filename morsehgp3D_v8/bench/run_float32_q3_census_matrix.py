#!/usr/bin/env python3
"""Run 36 bounded native-float32 q3 edge-census diagnostic observations.

The authority is a closed Release run_float32_q3_census_checks capture. Nothing
is rebuilt. Two synthetic regimes, n=8000/16000/32000, K=5/10 and three sharing
configurations are run once each. Equal digests are differential evidence, not
an exhaustive large-cloud oracle. A single edge is not the global generator,
a SemanticKITTI frame, a hierarchy, a GPU benchmark or a runtime contract.
"""
from __future__ import annotations

import argparse
import base64
from copy import deepcopy
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

import run_float32_q3_census_checks as qualification
from run_p0_matrix import invoke, on_signal, parse_result

ROOT, V8 = qualification.ROOT, qualification.V8
require, sha, pins = qualification.require, qualification.sha, qualification.pins
write, stamp = qualification.write, qualification.stamp
SOURCES = (*qualification.SOURCES, Path(__file__).resolve())
SCHEMA = "mhgp8_float32_q3_census_matrix_v1"
SCOPE = "two_synthetic_regimes_one_edge_seed_root_not_global_generator_or_frame_or_FULL_or_GPU"
TIMING_CONTEXT = "one_observation_nonisolated_shared_host_no_stable_speedup_claim"
CONFIGURATIONS = ((0, 1), (1, 1), (1, 8))


def cases():
    return [dict(n=n, regime=regime, mode=mode, kmax=kmax, relay_sites=grain)
            for regime in ("column", "slab") for kmax in (5, 10)
            for n in (8000, 16000, 32000) for mode, grain in CONFIGURATIONS]


def command(binary, case):
    return [str(binary), "--bench", str(case["n"]), case["regime"], str(case["mode"]),
            str(case["kmax"]), str(case["relay_sites"])]


def authority(path):
    path = path.resolve(strict=True)
    result = qualification.read(path)
    manifest = qualification.read_json(path / "MANIFEST.json")
    completion = qualification.read_json(path / "COMPLETION.json")
    require(result["status"] == "passed" and manifest["config"]["sanitize"] is False,
            "matrix requires a closed native q3 Release qualification")
    build = Path(manifest["config"]["build"])
    return dict(path=str(path), files=pins((path / "MANIFEST.json", path / "COMPLETION.json")),
                compiled=completion["compiled"], build=str(build), binary=str(build / qualification.BINARY),
                source_sha256=manifest["source_sha256"])


def artifact_pins(path):
    return qualification.artifact_pins(path)


def validate(row, case, oracle):
    oracle.validate_bench(row, case["n"], case["regime"], case["mode"], case["kmax"], case["relay_sites"])


def flatten(value, prefix=""):
    result = {}
    for name, item in value.items():
        key = prefix+name
        if type(item) is dict:
            result.update(flatten(item, key+"."))
        else:
            require(type(item) is int and item >= 0, "noninteger discrete census cost")
            result[key] = item
    return result


def discrete_costs(row):
    work = row["work"]
    result = flatten(dict(index=row["index_work"], census=work))
    # Three disjoint visit ledgers; never report only the cheaper suffix after
    # a shared prefix. Bounds/preparations/parabolas remain separate costs.
    result["total_geometry_visits"] = (work["shared_witness_visits"] + work["count_node_visits"] +
                                       work["shell_node_visits"])
    result["total_bound_preparations"] = work["shared_bounds"]["preparations"] + work["individual_bounds"]["preparations"]
    result["total_bound_queries"] = work["shared_bounds"]["bound_queries"] + work["individual_bounds"]["bound_queries"]
    result["total_bound_power_evaluations"] = (work["shared_bounds"]["power_evaluations"] +
                                               work["individual_bounds"]["power_evaluations"])
    return result


def analyze(rows):
    require(len(rows) == 36, "q3 matrix must contain all 36 observations")
    by_case = {}
    for row in rows:
        key = (row["regime"], row["kmax"], row["n"], row["mode"], row["relay_sites"])
        require(key not in by_case, "duplicate q3 matrix case")
        by_case[key] = row
    expected = {(c["regime"], c["kmax"], c["n"], c["mode"], c["relay_sites"]) for c in cases()}
    require(set(by_case) == expected, "missing or unexpected q3 matrix observation")
    paired = 0
    for regime in ("column", "slab"):
        for kmax in (5, 10):
            for n in (8000, 16000, 32000):
                baseline = by_case[regime, kmax, n, 0, 1]
                for mode, grain in CONFIGURATIONS[1:]:
                    row = by_case[regime, kmax, n, mode, grain]
                    require(row["payload"] == baseline["payload"], "individual/shared q3 payload differs")
                    require(row["index_work"] == baseline["index_work"] and row["index_bytes"] == baseline["index_bytes"],
                            "q3 sharing changed its independently rebuilt index")
                    paired += 1
    growth = []
    for regime in ("column", "slab"):
        for kmax in (5, 10):
            for mode, grain in CONFIGURATIONS:
                for small, large in ((8000, 16000), (16000, 32000)):
                    before, after = (by_case[regime, kmax, n, mode, grain] for n in (small, large))
                    left, right = discrete_costs(before), discrete_costs(after)
                    require(set(left) == set(right), "q3 matrix work inventory changed")
                    metrics = {}
                    for name in left:
                        a, b = left[name], right[name]
                        metrics[name] = dict(before=a, after=b, ratio=b/a if a else None,
                                             relation=("zero_both" if b == 0 else "starts_nonzero") if a == 0 else
                                             ("below" if b*small*small < a*large*large else
                                              "equal" if b*small*small == a*large*large else "above"))
                    growth.append(dict(regime=regime, kmax=kmax, mode=mode, relay_sites=grain,
                                       small_n=small, large_n=large, quadratic_ratio=4, metrics=metrics))
    return dict(paired_payloads=paired, growth=growth)


def mutations(rows, oracle):
    """Rejudge deliberate changed values; no mutation can be a no-op at zero."""
    changes = (
        ("payload_bit", lambda r: r[0]["payload"].__setitem__("digest_sum", r[0]["payload"]["digest_sum"] ^ 1)),
        ("drop_case", lambda r: r.pop()),
        ("duplicate_case", lambda r: r.__setitem__(1, deepcopy(r[0]))),
        ("wrong_n", lambda r: r[0].__setitem__("n", r[0]["n"]+1)),
        ("wrong_mode", lambda r: r[0].__setitem__("mode", r[0]["mode"] ^ 1)),
        ("wrong_k", lambda r: r[0].__setitem__("kmax", r[0]["kmax"]+1)),
        ("wrong_grain", lambda r: r[0].__setitem__("relay_sites", r[0]["relay_sites"]+1)),
        ("negative_work", lambda r: r[0]["work"].__setitem__("calls", -1)),
        ("boolean_work", lambda r: r[0]["work"].__setitem__("calls", True)),
        ("string_work", lambda r: r[0]["work"].__setitem__("calls", "1")),
        ("seed_mass", lambda r: r[0]["work"].__setitem__("input_seed_slots", r[0]["work"]["input_seed_slots"]+1)),
        ("negative_time", lambda r: r[0].__setitem__("census_ms", -1)),
    )
    rejected = []
    for name, change in changes:
        altered = deepcopy(rows)
        change(altered)
        require(json.dumps(altered, sort_keys=True) != json.dumps(rows, sort_keys=True),
                "q3 matrix mutation did not change its target")
        try:
            require(len(altered) == len(cases()), "q3 matrix mutated observation count")
            for row, case in zip(altered, cases(), strict=True):
                validate(row, case, oracle)
            analyze(altered)
        except ValueError:
            rejected.append(name)
    require(len(rejected) == len(changes) == 12, "q3 matrix output mutation survived")
    return rejected


def run(args):
    upstream = authority(args.qualification)
    require(len(SOURCES) == len(set(SOURCES)) == 16, "q3 matrix source inventory changed")
    source_pins = pins(SOURCES)
    args.output.mkdir(parents=True, exist_ok=True)
    target = Path(tempfile.mkdtemp(prefix="matrix_", dir=args.output.resolve()))
    for source in SOURCES:
        destination = target / "sources" / source.relative_to(V8)
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open("xb") as stream:
            stream.write(source.read_bytes())
    plan = [dict(case=case, command=command(upstream["binary"], case)) for case in cases()]
    environment = dict(os.environ)
    recorded_environment = {key: environment.get(key) for key in qualification.ENV_KEYS}
    manifest = dict(schema=SCHEMA, scope=SCOPE, public_status="not_claimed", gcp_used=False,
                    authority=upstream, source_sha256=source_pins, launch=[sys.executable, *sys.argv],
                    started_utc=stamp(), plan=plan, repeats=1, environment=recorded_environment,
                    timing_context=TIMING_CONTEXT,
                    git_commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip())
    write(target / "MANIFEST.json", manifest)
    state = dict(status="running", commands=[], started_utc=stamp(), manifest_sha256=sha(target / "MANIFEST.json"))
    oracle = qualification.oracle_module()
    rows = []
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    print(json.dumps(dict(status="starting", path=str(target))), flush=True)
    try:
        for number, planned in enumerate(plan):
            filename = f"record_{number:03}.json"
            record = dict(**planned, cwd=str(ROOT), environment=recorded_environment,
                          exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
            print(json.dumps(dict(number=number, **planned["case"])), flush=True)
            try:
                invoke(planned["command"], environment, ROOT, record, new_session=True)
            finally:
                write(target / filename, record)
                state["commands"].append(dict(path=filename, sha256=sha(target / filename)))
            require(type(record["exit_code"]) is int and record["exit_code"] == 0 and record["stderr"] == "",
                    "q3 matrix native command failed")
            row = parse_result(base64.b64decode(record["stdout_base64"], validate=True))
            validate(row, planned["case"], oracle)
            rows.append(row)
        analyze(rows)
        state["status"] = "passed"
    except BaseException as error:
        state.update(status="failed", error=f"{type(error).__name__}: {error}")
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        try:
            state["source_sha256_after"] = pins(source_pins)
            require(state["source_sha256_after"] == source_pins and
                    sha(target / "MANIFEST.json") == state["manifest_sha256"], "q3 matrix source/manifest closure changed")
            state["authority_after"] = authority(Path(upstream["path"]))
            require(state["authority_after"] == upstream, "q3 matrix authority/binary/dependency closure changed")
            state["artifact_sha256"] = artifact_pins(target)
        except BaseException as error:
            state.update(status="failed", closing_error=f"{type(error).__name__}: {error}")
        state["finished_utc"] = stamp()
        write(target / "COMPLETION.json", state)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    require(state["status"] == "passed", "q3 matrix failed; every completed/failed command is retained")
    result = read(target)
    print(json.dumps(compact(result), sort_keys=True))


def read(path):
    path = path.resolve(strict=True)
    require(path.is_dir(), "q3 matrix capture is not a directory")
    closure = sha(path / "COMPLETION.json")
    manifest, state = (qualification.read_json(path / name) for name in ("MANIFEST.json", "COMPLETION.json"))
    require(manifest["schema"] == SCHEMA and manifest["scope"] == SCOPE and manifest["public_status"] == "not_claimed" and
            manifest["timing_context"] == TIMING_CONTEXT and
            manifest["gcp_used"] is False and type(manifest["repeats"]) is int and manifest["repeats"] == 1 and
            state["status"] == "passed", "q3 matrix scope/status changed")
    parsed = parser().parse_args(manifest["launch"][2:])
    require(manifest["launch"][1] in (str(Path(__file__).resolve()), str(Path(__file__).resolve().relative_to(ROOT))) and
            Path(manifest["launch"][0]).is_file() and parsed.operation == "run" and parsed.output.resolve() == path.parent and
            str(parsed.qualification.resolve()) == manifest["authority"]["path"], "q3 matrix launch binding differs")
    require(set(manifest["environment"]) == set(qualification.ENV_KEYS) and all(value is None or type(value) is str
            for value in manifest["environment"].values()), "q3 matrix environment shape")
    expected_pins = pins(SOURCES)
    require(len(SOURCES) == 16 and manifest["source_sha256"] == state["source_sha256_after"] == expected_pins and
            sha(path / "MANIFEST.json") == state["manifest_sha256"], "q3 matrix source/manifest closure")
    for source in SOURCES:
        require(sha(path / "sources" / source.relative_to(V8)) == expected_pins[str(source)], "q3 matrix source snapshot differs")
    require(artifact_pins(path) == state["artifact_sha256"], "q3 matrix artifact closure")
    upstream = authority(Path(manifest["authority"]["path"]))
    require(upstream == manifest["authority"] == state["authority_after"], "q3 matrix compiled authority differs")
    plan = [dict(case=case, command=command(upstream["binary"], case)) for case in cases()]
    require(manifest["plan"] == plan and len(state["commands"]) == len(plan) == 36, "q3 matrix command plan differs")
    oracle, rows = qualification.oracle_module(), []
    for number, (planned, entry) in enumerate(zip(plan, state["commands"], strict=True)):
        require(entry["path"] == f"record_{number:03}.json" and sha(path / entry["path"]) == entry["sha256"], "q3 matrix record hash")
        record = qualification.read_json(path / entry["path"])
        require(record["case"] == planned["case"] and record["command"] == planned["command"] and
                record["cwd"] == str(ROOT) and record["environment"] == manifest["environment"] and
                type(record["exit_code"]) is int and record["exit_code"] == 0 and record["stderr"] == "",
                "q3 matrix command/configuration/exit differs")
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(record[stream+"_base64"], validate=True).decode("utf-8", errors="replace") == record[stream],
                    "q3 matrix raw stream differs")
        row = parse_result(base64.b64decode(record["stdout_base64"], validate=True))
        validate(row, planned["case"], oracle)
        rows.append(row)
    analysis = analyze(rows)
    rejected = mutations(rows, oracle)
    require(artifact_pins(path) == state["artifact_sha256"] and sha(path / "COMPLETION.json") == closure and
            pins(expected_pins) == expected_pins and pins(upstream["files"]) == upstream["files"] and
            pins(upstream["source_sha256"]) == upstream["source_sha256"] and
            qualification.compiled_pins(Path(upstream["build"])) == upstream["compiled"], "q3 matrix final LIVE closure changed")
    return dict(schema=SCHEMA, status="passed", path=str(path), commands=36, completion_sha256=closure,
                scope=SCOPE, timing_context=TIMING_CONTEXT, observations=rows, **analysis,
                rejected_mutations=rejected, gcp_used=False)


def compact(result):
    return {key: value for key, value in result.items() if key not in ("observations", "growth")}


def parser():
    result = argparse.ArgumentParser(description=__doc__)
    sub = result.add_subparsers(dest="operation", required=True)
    runner = sub.add_parser("run")
    runner.add_argument("--qualification", type=Path, required=True)
    runner.add_argument("--output", type=Path, required=True)
    for operation in ("read", "selftest"):
        reader = sub.add_parser(operation)
        reader.add_argument("--path", type=Path, required=True)
        reader.add_argument("--compact", action="store_true")
    return result


if __name__ == "__main__":
    arguments = parser().parse_args()
    if arguments.operation == "run":
        run(arguments)
    else:
        result = read(arguments.path)
        print(json.dumps(compact(result) if arguments.compact else result, sort_keys=True))
