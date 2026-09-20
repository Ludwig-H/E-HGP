#!/usr/bin/env python3
"""Targeted seed candidates and optional full Release regression; no build/GCP.

Explicit reuse of the previous collector, strict JSON, pinning and closure
helpers. Old source inventories/readers remain untouched. Measurements cover
ONE supplied seed, not enumeration of q3/q4 seeds or a complete HGP tower.
"""
from __future__ import annotations

import argparse
import base64
from copy import deepcopy
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from run_q4_family_checks import (SOURCES as OLD_SOURCES, WORK as FAMILY_FIELDS,
                                  counts, digest, pins, read_json, validate_closure)
from run_p0_matrix import InvalidReceipt, invoke, on_signal, parse_result, require, uint, utc_stamp, write_json

ROOT = Path(__file__).resolve().parents[2]
SOURCES = OLD_SOURCES | frozenset({
    "morsehgp3D_v8/src/lanes/exact_ball.hpp", "morsehgp3D_v8/src/lanes/exact_ball.cpp",
    "morsehgp3D_v8/src/lanes/q34_seed.hpp", "morsehgp3D_v8/src/lanes/q34_seed.cpp",
    "morsehgp3D_v8/tests/exact_ball_oracle.hpp",
    "morsehgp3D_v8/tests/exact_ball_gate.cpp", "morsehgp3D_v8/tests/q34_seed_gate.cpp",
    "morsehgp3D_v8/bench/q34_seed_probe.cpp", "morsehgp3D_v8/bench/run_q34_seed_checks.py",
})
SCHEMA = "mhgp8_q34_seed_attempt_v1"
FAMILIES = ("uniform", "rows", "coplanar")
GATES = ("mhgp8_exact_ball_gate", "mhgp8_q34_seed_gate")
WORK_FIELDS = ("seed_owner_tests", "seed_owner_rejections", "q3_point_tests", "q3_shell_ids",
    "q3_depth_rejections", "q3_emitted", "q3_shell_capacity_bytes", "q4_depth_rejected_groups",
    "q4_depth_skipped_ids", "q4_presentations", "q4_owner_tests", "q4_owner_rejections",
    "q4_positive_tests", "q4_positive_rejections", "q4_seed_tests", "q4_seed_rejections",
    "q4_groups_without_support", "q4_unexamined_after_emit", "q4_emitted")
TIMES = ("generation_ms", "cloud_ms", "oracle_prepare_ms", "seed_with_validation_callback_ms",
         "validation_ms", "release_ms", "total_ms")
FIXED = dict(schema="mhgp8_q34_seed_probe_v1", status="completed",
             scope="one_seed_positive_presentations_not_q34_producer", public_status="not_claimed",
             backend="cpu_reference", profile="quantized_u16_input_only", threads=1, seed=3,
             seed_ids=[0, 1, 2], recipe="local_positive_remote_v1",
             timing_scope="generation_owner_seed_callback_validation_release_excludes_json")
KEYS = [[1, -2000, -2045, -2015, 3050000], [1, -2000, -2050, -2000, 3040000]]
BALL_GATE_FLOORS = dict(checks=1, cases=1500, accepted_q2=400, accepted_q3=100, accepted_q4=50,
    rejected=100, rank_deficient=10, boundary_centres=10, exterior_centres=20, permutations=1,
    powers=10000, strict_interiors=100, shell_sites=500, strict_exteriors=1000,
    same_ball_across_arities=3, max_coefficient_bits=65, max_power_bits=65,
    max_naive_radius_bits=129, translated=9, judge_mutants=3)
SEED_GATE_FLOORS = dict(checks=1, calls=500, oracle_completions=1500, oracle_sites=10000,
    candidates=100, q3=100, q4=20, owner_refusals=5, q3_rejected=10, q4_without_q3=2,
    late_valid=1, max_shell=30, canonical_refusals=1, positive_refusals=1, depth_refusals=1,
    exhaustive_clouds=13, exhaustive_balls=21, invalid_inputs=6, callback_failures=1,
    parallel_calls=4, judge_mutants=3, unexamined=1, depth_drop_runs=1, larger_than_minimal=1)


def environment_record(environment):
    selected = {key: value for key, value in environment.items()
                if key in {"PATH", "LANG", "LC_ALL", "LC_CTYPE", "TZ", "LD_LIBRARY_PATH", "LD_PRELOAD"}
                or key.startswith(("ASAN_", "UBSAN_", "TSAN_", "LSAN_", "CTEST_", "OMP_", "PYTHON"))}
    return dict(selected=selected, names=sorted(environment),
                complete_sha256=hashlib.sha256(json.dumps(environment, sort_keys=True, ensure_ascii=True).encode()).hexdigest(),
                scope="selected_values_full_environment_fingerprint_no_secrets")


def expected_hash():
    value = 14695981039346656037
    def word(number):
        nonlocal value
        for byte in (number & ((1 << 64) - 1)).to_bytes(8, "little"):
            value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    for arity, key, shells in ((3, KEYS[0], ([0, 1, 2], [])), (4, KEYS[1], ([3], [0, 1, 2]))):
        for item in [arity, 2, *key, *range(arity)]:
            word(item)
        for shell in shells:
            word(len(shell))
            for item in shell:
                word(item)
    return value


def validate_row(row, command):
    require(type(row) is dict and len(command) == 4, "seed probe command arity")
    n, family, kmax = int(command[1]), command[2], int(command[3])
    require(n >= 8 and family in FAMILIES and kmax in (5, 10), "fixture domain mismatch")
    require(set(row) == set(FIXED) | {"n", "family", "kmax", "input_hash", "generation", "work", "digest",
                                    "oracle", "cloud_work", "memory", "timings"}, "probe fields differ")
    require(all(type(row[k]) is type(v) and row[k] == v for k, v in FIXED.items()) and
            all(type(v) is int for v in row["seed_ids"]), "probe scope/schema mismatch")
    require(type(row["n"]) is int and row["n"] == n and type(row["kmax"]) is int and
            row["kmax"] == kmax and row["family"] == family, "command/result mismatch")
    uint(row["input_hash"], "input_hash")
    work = row["work"]
    require(type(work) is dict and set(work) == set(WORK_FIELDS) | {"family"}, "seed work fields differ")
    for key in WORK_FIELDS:
        uint(work[key], "work." + key)
    f = work["family"]
    counts(f, FAMILY_FIELDS, "family")
    fixed = dict(seed_owner_tests=2, seed_owner_rejections=0, q3_point_tests=n, q3_shell_ids=3,
                 q3_depth_rejections=0, q3_emitted=1, q4_positive_tests=3, q4_positive_rejections=2,
                 q4_seed_tests=1, q4_seed_rejections=0, q4_unexamined_after_emit=0, q4_emitted=1)
    require(all(work[key] == value for key, value in fixed.items()), "local positive fixture not fully exercised")
    require(f["sites"] == n and f["entries"] == (0 if family == "coplanar" else n - 6) and
            f["exits"] == 3 and f["constant_inside"] == 0 and f["constant_on"] == 3 and
            f["constant_outside"] == (n - 6 if family == "coplanar" else 0) and
            f["event_count"] == f["entries"] + f["exits"], "family site partition mismatch")
    require(f["groups"] == f["callbacks"] and 3 <= f["groups"] <= f["event_count"] and
            1 <= f["max_group"] <= f["event_count"] and f["max_group"] * f["groups"] >= f["event_count"] and
            f["sort_comparisons"] > 0 and f["group_comparisons"] == f["event_count"] - 1,
            "family grouping mismatch")
    require(work["q4_presentations"] + work["q4_depth_skipped_ids"] + work["q4_unexamined_after_emit"] == f["event_count"] and
            work["q4_depth_rejected_groups"] + work["q4_groups_without_support"] + work["q4_emitted"] == f["groups"],
            "q4 root/ID partition mismatch")
    require(work["q4_presentations"] == work["q4_owner_rejections"] + work["q4_positive_tests"] and
            work["q4_positive_tests"] == work["q4_positive_rejections"] + work["q4_seed_tests"] and
            work["q4_seed_tests"] == work["q4_seed_rejections"] + work["q4_emitted"] and
            work["q4_presentations"] <= work["q4_owner_tests"] <= 5 * work["q4_presentations"],
            "q4 presentation cascade mismatch")
    out = row["digest"]
    require(type(out) is dict and set(out) == {"callbacks", "support_ids_visited", "shell_ids_visited", "depths", "ball_coefficients", "hash"},
            "digest fields differ")
    for key in ("callbacks", "support_ids_visited", "shell_ids_visited", "hash"):
        uint(out[key], "digest." + key)
    require(out["callbacks"] == 2 and out["support_ids_visited"] == out["shell_ids_visited"] == 7 and
            out["depths"] == [2, 2] and all(type(v) is int for v in out["depths"]) and
            out["ball_coefficients"] == KEYS and all(type(v) is int for key in out["ball_coefficients"] for v in key) and
            out["hash"] == expected_hash(), "independent candidate digest mismatch")
    counts(row["oracle"], ("scalar_point_tests", "remote_certificate_tests"), "oracle")
    require(row["oracle"] == dict(scalar_point_tests=2 * n, remote_certificate_tests=n - 6), "oracle work is not linear")
    counts(row["generation"], ("proposals", "duplicates", "random_calls"), "generation")
    g = row["generation"]
    require(g["proposals"] == n - 6 + g["duplicates"] and
            g["random_calls"] == (3 * g["proposals"] if family == "uniform" else 0), "generation partition mismatch")
    counts(row["cloud_work"], ("copies", "validation_points", "uniqueness_comparisons", "range_tree_nodes"), "cloud")
    require(row["cloud_work"]["copies"] == row["cloud_work"]["validation_points"] == n, "owner preparation missing")
    memory = row["memory"]
    require(type(memory) is dict and set(memory) == {"input_capacity_bytes", "cloud_retained_bytes", "event_id_bytes", "scope"} and
            memory["scope"] == "retained_capacities_not_RSS", "memory scope mismatch")
    for key in ("input_capacity_bytes", "cloud_retained_bytes"):
        require(uint(memory[key], key) >= 6 * n, "coordinate storage missing")
    require(uint(memory["event_id_bytes"], "event_id_bytes") in (4, 8) and
            f["retained_capacity_bytes"] >= 2 * n * memory["event_id_bytes"] and
            work["q3_shell_capacity_bytes"] >= 3 * memory["event_id_bytes"], "owned buffers missing")
    t = row["timings"]
    require(type(t) is dict and set(t) == set(TIMES) and
            all(type(v) in (int, float) and math.isfinite(v) and v >= 0 for v in t.values()), "invalid timing")
    require(math.isclose(sum(t[k] for k in TIMES[:-1]), t["total_ms"], rel_tol=1e-12, abs_tol=1e-6), "timing partition mismatch")


def validate_gate(row, executable):
    require(executable in GATES, "unknown gate")
    floors = BALL_GATE_FLOORS if executable == GATES[0] else SEED_GATE_FLOORS
    require(type(row) is dict and set(row) == set(floors) | {"schema", "status"} and
            row.get("schema") == executable + "_v1" and row.get("status") == "passed", "gate schema/fields mismatch")
    for key in floors:
        uint(row[key], "gate." + key)
    require(all(row[key] >= floor for key, floor in floors.items()), "gate non-vacuity floor failed")
    require(row["judge_mutants"] == 3, "unexpected judge mutant count")
    if executable == GATES[0]:
        require(row["cases"] == row["accepted_q2"] + row["accepted_q3"] + row["accepted_q4"] + row["rejected"] and
                row["rejected"] == row["rank_deficient"] + row["boundary_centres"] + row["exterior_centres"] and
                row["powers"] == row["strict_interiors"] + row["shell_sites"] + row["strict_exteriors"] and
                row["same_ball_across_arities"] == 3 and row["translated"] == 9, "ball gate ledger mismatch")
    else:
        require(row["candidates"] == row["q3"] + row["q4"] and row["exhaustive_clouds"] == 13 and
                row["invalid_inputs"] == 6 and row["callback_failures"] == 1 and row["parallel_calls"] == 4,
                "seed gate ledger/lifecycle mismatch")


def validate_xml(path):
    tree = ET.parse(path).getroot()
    require(tree.tag == "testsuite" and tree.get("tests") == "84" and
            all(tree.get(key) == "0" for key in ("failures", "disabled", "skipped")), "unsuccessful 84-test regression")
    cases = tree.findall("testcase")
    names = [case.get("name") for case in cases]
    require(len(cases) == 84 and len(set(names)) == 84 and all(type(n) is str and n.startswith("mhgp8_") for n in names) and
            set(GATES) | {"mhgp8_q4_family_gate"} <= set(names), "regression test inventory mismatch")
    require(all(case.get("status") == "run" and all(case.find(tag) is None for tag in ("failure", "error", "skipped")) for case in cases),
            "regression skipped/failed testcase")
    return sorted(names)


def plan(build, campaign, output, ctest=None):
    if campaign == "regression":
        return [("ctest", [ctest, "--test-dir", str(build), "--output-on-failure", "--parallel", "2",
                           "--output-junit", str(output / "result.xml")])]
    gates = [("gate", [str(build / name), "--selftest"]) for name in GATES]
    sizes = () if campaign == "gate" else (32,) if campaign == "smoke" else (8000, 16000, 32000)
    return gates + [("measure", [str(build / "mhgp8_q34_seed_probe"), str(n), family, str(k)])
                    for family in FAMILIES for k in (5, 10) for n in sizes]


def executable_names(build, campaign):
    if campaign == "regression":
        names = sorted(p.name for p in build.glob("mhgp8_*") if p.is_file() and os.access(p, os.X_OK))
        require(names and all((build / name).resolve() == build / name for name in names), "missing or linked executable")
        return names
    return sorted([*GATES, *(["mhgp8_q34_seed_probe"] if campaign != "gate" else [])])


def run(args):
    build = args.build.resolve()
    require(build.is_dir() and build.is_relative_to(ROOT / "build"), "local fresh build required")
    cache = (build / "CMakeCache.txt").read_text()
    if args.campaign == "regression":
        require("CMAKE_BUILD_TYPE:STRING=Release\n" in cache and "MHGP8_SANITIZE:BOOL=OFF\n" in cache, "regression requires nonsanitized Release")
    args.output.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=args.campaign + "_", dir=args.output)).resolve()
    ctest = None
    if args.campaign == "regression":
        found = shutil.which("ctest")
        require(found is not None, "CTest unavailable")
        ctest = str(Path(found).resolve())
    names = executable_names(build, args.campaign)
    files = {str((build / name).relative_to(ROOT)) for name in [*names, "CMakeCache.txt"]}
    def git(*args):
        return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()
    environment = dict(os.environ)
    commands = plan(build, args.campaign, output, ctest)
    manifest = dict(schema=SCHEMA, campaign=args.campaign, build=str(build), started_utc=utc_stamp(),
        public_status="not_claimed", gcp_used=False, full_contract_qualified=False, source_sha256=pins(SOURCES),
        artifact_sha256=pins(files), executable_names=names, planned_commands=commands, affinity=sorted(os.sched_getaffinity(0)),
        commit=git("rev-parse", "HEAD"), branch=git("branch", "--show-current"), worktree=git("status", "--short"),
        launch_command=[sys.executable, *sys.argv],
        environment=environment_record(environment),
        compiler_cache="\n".join(line for line in cache.splitlines() if line.startswith(("CMAKE_CXX_COMPILER", "CMAKE_CXX_FLAGS", "CMAKE_BUILD_TYPE:", "CMAKE_GENERATOR:", "MHGP8_SANITIZE:"))),
        sanitizer_environment={key: environment.get(key) for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS", "TSAN_OPTIONS")},
        ctest=ctest, ctest_sha256=digest(Path(ctest)) if ctest else None)
    write_json(output / "MANIFEST.json", manifest)
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, status, error = [], "failed", None
    try:
        for number, (kind, command) in enumerate(commands):
            record = dict(kind=kind, command=command, cwd=str(ROOT), started_utc=utc_stamp(), status="failed", exit_code=None,
                          stdout="", stderr="", stdout_base64="", stderr_base64="", environment=manifest["environment"])
            try:
                invoke(command, environment, ROOT, record, new_session=True)
                require(record["exit_code"] == 0, "targeted command failed")
                if kind == "ctest":
                    record["test_names"] = validate_xml(output / "result.xml")
                else:
                    row = parse_result(record["stdout"].encode())
                    validate_row(row, command) if kind == "measure" else validate_gate(row, Path(command[0]).name)
                    record["row"] = row
                record["status"] = "passed"
            finally:
                record["finished_utc"] = utc_stamp()
                file = output / f"record_{number:04}.json"
                write_json(file, record)
                records.append(dict(path=file.name, sha256=digest(file)))
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        errors = []
        def close(label, function):
            try:
                return function()
            except Exception as cause:
                errors.append(f"{label}: {type(cause).__name__}: {cause}")
                return None
        completion = dict(status=status, error=error, finished_utc=utc_stamp(), records=records,
            manifest_sha256=digest(output / "MANIFEST.json"), source_sha256_after=close("sources", lambda: pins(SOURCES)),
            artifact_sha256_after=close("artifacts", lambda: pins(files)),
            executable_names_after=close("executables", lambda: executable_names(build, args.campaign)),
            ctest_sha256_after=close("ctest", lambda: digest(Path(ctest))) if ctest else None,
            xml_sha256=close("xml", lambda: digest(output / "result.xml")) if (output / "result.xml").is_file() else None,
            closing_errors=errors)
        if errors or any(completion[after] != manifest[before] for before, after in (
                ("source_sha256", "source_sha256_after"), ("artifact_sha256", "artifact_sha256_after"),
                ("executable_names", "executable_names_after"), ("ctest_sha256", "ctest_sha256_after"))):
            completion["status"] = "failed"
            completion["error"] = error or "closure changed or could not be read"
        write_json(output / "COMPLETION.json", completion)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(output), status=completion["status"], error=completion["error"])), flush=True)
    require(completion["status"] == "passed", "capture closure failed")


def read(path, check_live=False):
    path = path.resolve()
    m, c = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    require(m["schema"] == SCHEMA and c["manifest_sha256"] == digest(path / "MANIFEST.json") and c["closing_errors"] == [], "invalid capture")
    validate_closure(m, c)
    require(m["public_status"] == "not_claimed" and m["gcp_used"] is False and m["full_contract_qualified"] is False and m["branch"] == "main", "scope/branch changed")
    require(type(m.get("commit")) is str and len(m["commit"]) == 40 and all(v in "0123456789abcdef" for v in m["commit"]) and
            type(m.get("worktree")) is str and type(m.get("compiler_cache")) is str and "CMAKE_CXX_COMPILER:" in m["compiler_cache"] and
            type(m.get("launch_command")) is list and len(m["launch_command"]) >= 2 and all(type(v) is str for v in m["launch_command"]) and
            type(m.get("environment")) is dict and m["environment"].get("scope") == "selected_values_full_environment_fingerprint_no_secrets",
            "capture provenance missing")
    build, campaign = Path(m["build"]), m["campaign"]
    require(build.is_absolute() and build.is_relative_to(ROOT / "build") and ".." not in build.parts and campaign in ("gate", "smoke", "scale", "regression"), "invalid build/campaign")
    names = m["executable_names"]
    require(type(names) is list and names == sorted(set(names)) and set(GATES) <= set(names) and
            all(type(name) is str and name.startswith("mhgp8_") and Path(name).name == name for name in names), "invalid executable names")
    require(c["executable_names_after"] == names and set(m["source_sha256"]) == SOURCES and
            set(m["artifact_sha256"]) == {str((build / name).relative_to(ROOT)) for name in [*names, "CMakeCache.txt"]}, "pin inventory mismatch")
    if campaign != "regression":
        require(names == executable_names(build, campaign) and m["ctest"] is None and m["ctest_sha256"] is None and
                c["ctest_sha256_after"] is None and c["xml_sha256"] is None, "targeted artifact inventory differs")
    else:
        require(Path(m["ctest"]).is_absolute() and Path(m["ctest"]).name == "ctest" and
                m["ctest_sha256"] == c["ctest_sha256_after"] and "MHGP8_SANITIZE:BOOL=OFF" in m["compiler_cache"] and
                "CMAKE_BUILD_TYPE:STRING=Release" in m["compiler_cache"], "CTest command/build scope mismatch")
        require(type(m["ctest_sha256"]) is str and len(m["ctest_sha256"]) == 64 and
                all(v in "0123456789abcdef" for v in m["ctest_sha256"]), "missing CTest executable pin")
    commands = [[kind, command] for kind, command in plan(build, campaign, path, m["ctest"])]
    require(m["planned_commands"] == commands and len(c["records"]) == len(commands) and
            {p.name for p in path.glob("record_*.json")} == {f"record_{i:04}.json" for i in range(len(commands))}, "command plan mismatch")
    if check_live:
        require(pins(SOURCES) == m["source_sha256"] and pins(m["artifact_sha256"]) == m["artifact_sha256"] and
                executable_names(build, campaign) == names and (not m["ctest"] or digest(Path(m["ctest"])) == m["ctest_sha256"]), "live sources/artifacts changed")
    rows = []
    for i, ((kind, command), info) in enumerate(zip(commands, c["records"], strict=True)):
        require(info["path"] == f"record_{i:04}.json" and digest(path / info["path"]) == info["sha256"], "record path/hash mismatch")
        rec = read_json(path / info["path"])
        require(rec["command"] == command and rec["kind"] == kind and rec["cwd"] == str(ROOT) and rec["environment"] == m["environment"] and
                rec["status"] == "passed" and type(rec["exit_code"]) is int and rec["exit_code"] == 0, "command/result mismatch")
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(rec[stream + "_base64"], validate=True).decode("utf-8", errors="replace") == rec[stream], "raw/decoded log mismatch")
        if kind == "ctest":
            require(c["xml_sha256"] == digest(path / "result.xml") and validate_xml(path / "result.xml") == rec["test_names"], "JUnit proof mismatch")
        else:
            row = parse_result(rec["stdout"].encode())
            require(rec["row"] == row, "parsed row changed")
            validate_row(row, command) if kind == "measure" else validate_gate(row, Path(command[0]).name)
            if kind == "measure": rows.append(row)
    for family in FAMILIES:
        for n in sorted({row["n"] for row in rows if row["family"] == family}):
            pair = [row for row in rows if row["family"] == family and row["n"] == n]
            require(len(pair) == 2 and {row["kmax"] for row in pair} == {5, 10}, "missing K5/K10 pair")
            require(all(pair[0][key] == pair[1][key] for key in ("input_hash", "generation", "cloud_work", "digest")) and
                    pair[0]["work"]["family"] == pair[1]["work"]["family"], "paired seeds differ beyond threshold cascade")
    growth = {}
    if campaign == "scale":
        for family in FAMILIES:
            for k in (5, 10):
                series = [r for r in rows if r["family"] == family and r["kmax"] == k]
                growth[f"{family}_K{k}"] = {}
                metrics = {field: [r["work"][field] for r in series] for field in WORK_FIELDS}
                metrics.update({"family." + field: [r["work"]["family"][field] for r in series] for field in FAMILY_FIELDS})
                for metric, values in metrics.items():
                    growth[f"{family}_K{k}"][metric] = dict(values=values, ratios=[b / a if a else None for a, b in zip(values, values[1:])],
                        above_quadrupling=[a > 0 and b > 4 * a for a, b in zip(values, values[1:])])
    return dict(status="passed", path=str(path), campaign=campaign, sources=len(SOURCES), records=len(commands),
                measurements=len(rows), full_contract_qualified=False, general_subquadratic_bound=False,
                scope="one_seed_not_q34_producer", growth=growth)


def selftest(path):
    summary = read(path)
    records = [read_json(file) for file in sorted(path.glob("record_*.json"))]
    base = next(r for r in records if r["kind"] == "measure" and r["row"]["family"] == "uniform")
    mutants = 0
    def reject(action):
        nonlocal mutants
        try: action()
        except InvalidReceipt: mutants += 1
        else: raise InvalidReceipt("reader mutant survived")
    for section, key, value in ((None, "n", True), (None, "kmax", True), (None, "seed_ids", [False, 1, 2]),
            ("work", "q3_emitted", 0), ("work", "q4_presentations", 0), ("work", "q4_positive_tests", True),
            ("digest", "hash", 0), ("digest", "depths", [2, 3]), ("digest", "shell_ids_visited", 0),
            ("oracle", "scalar_point_tests", 0), ("memory", "event_id_bytes", True), ("timings", "total_ms", float("nan"))):
        row = deepcopy(base["row"])
        (row if section is None else row[section])[key] = value
        reject(lambda: validate_row(row, base["command"]))
    cmd = base["command"].copy(); cmd[1] = str(int(cmd[1]) + 1)
    reject(lambda: validate_row(base["row"], cmd))
    reject(lambda: parse_result(b'{"x":NaN}'))
    reject(lambda: parse_result(b'{"x":1,"x":2}'))
    for record in records:
        if record["kind"] != "gate": continue
        gate = deepcopy(record["row"])
        gate["checks"] = True
        reject(lambda: validate_gate(gate, Path(record["command"][0]).name))
        gate["checks"] = 0
        reject(lambda: validate_gate(gate, Path(record["command"][0]).name))
    m, c = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    c["source_sha256_after"][next(iter(m["source_sha256"]))] = "0" * 64
    reject(lambda: validate_closure(m, c))
    return dict(status="passed", mutants=mutants, real_measurements=summary["measurements"], scope="receipt_reader_not_geometry")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest="operation", required=True)
    run_parser = sub.add_parser("run")
    run_parser.add_argument("--build", type=Path, required=True)
    run_parser.add_argument("--output", type=Path, required=True)
    run_parser.add_argument("--campaign", choices=("gate", "smoke", "scale", "regression"), required=True)
    reader = sub.add_parser("read"); reader.add_argument("path", type=Path)
    reader.add_argument("--check-live", action="store_true")
    unit = sub.add_parser("selftest"); unit.add_argument("path", type=Path)
    args = p.parse_args()
    if args.operation == "run": run(args)
    else:
        result = read(args.path, args.check_live) if args.operation == "read" else selftest(args.path)
        print(json.dumps(result, sort_keys=True, allow_nan=False))


if __name__ == "__main__":
    main()
