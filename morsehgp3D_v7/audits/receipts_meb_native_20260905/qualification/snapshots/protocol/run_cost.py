#!/usr/bin/env python3
"""Private, separately gated build and measurement; preview launches nothing."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import signal
import stat
import subprocess
import time

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
GEOMETRY_RUNNER = ROOT / "build/v7_meb_dual_budget_geometry/run_geometry.py"
GEOMETRY_SHA = "b04dc2a69aec60c6c5e41e83688588a3b963a0ba5f4e91260580e4d195bda727"
GEOMETRY_RECEIPT_SHA = "b81d8e480b158710874de230c3485f79d0a42f1cb228e321c750de0f58bed49e"
if hashlib.sha256(GEOMETRY_RUNNER.read_bytes()).hexdigest() != GEOMETRY_SHA:
    raise RuntimeError("geometry_runner_pin_mismatch")
SPEC = importlib.util.spec_from_file_location("cost_geometry_authority", GEOMETRY_RUNNER)
G = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(G)
require, read_bytes, digest, load_json, save = G.require, G.read_bytes, G.digest, G.load_json, G.save
BUILD = BASE / "build_20260905"
MEASURE = BASE / "run_20260905"
BINARY = BUILD / "cost_harness"
OBJDUMP = Path("/usr/bin/objdump").resolve()
PINS = {
    BASE / "cost_harness.cpp": "5a0fd39703c26279d91796fba7c099df9d25829a6f84b7fae4ddf99ffa61f5d8",
    ROOT / "build/v7_meb_dual_budget_cost_plan/PROTOCOL.md": "4a76f875485f39a7b0c5707e53f82c635cc1dfdab73bf595bf8348681b8f9ea7",
    OBJDUMP: "d0041e7511460b9ebfd95ce4f9d93c7ffea398ffc1cb2c316a220dd8e533db3d",
}
U64 = (1 << 64) - 1
HASH_SEED = 1469598103934665603
THREAD_ENV = ("OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS", "NUMEXPR_NUM_THREADS")


def cpu_metadata(cpu: int) -> dict:
    allowed = {"processor", "vendor_id", "cpu family", "model", "model name", "stepping",
               "microcode", "physical id", "core id", "siblings", "cpu cores"}
    selected = []
    for block in read_bytes(Path("/proc/cpuinfo")).decode("ascii").split("\n\n"):
        fields = {}
        for line in block.splitlines():
            if ":" in line:
                name, value = (part.strip() for part in line.split(":", 1))
                if name in allowed:
                    require(name not in fields, "duplicate_cpu_field")
                    fields[name] = value
        if fields.get("processor") == str(cpu):
            selected.append(fields)
    require(len(selected) == 1 and bool(selected[0].get("model name")), "selected_CPU_model_missing")
    online = read_bytes(Path("/sys/devices/system/cpu/online")).decode("ascii").strip()
    require(online and all(char in "0123456789,-" for char in online), "CPU_online_shape")
    return dict(selected_cpu=selected[0], online=online)


def pin(value: str | None, label: str) -> str:
    require(isinstance(value, str) and len(value) == 64 and
            all(char in "0123456789abcdef" for char in value), "explicit_" + label + "_pin_required")
    return value


def closed_capture(directory: Path, expected_sha: str) -> tuple[dict, dict[str, bytes]]:
    """Exact regular-file inventory, immutable bytes, no synthetic status-only pass."""
    pin(expected_sha, "receipt")
    require(stat.S_ISDIR(directory.lstat().st_mode), "capture_directory_not_regular")
    receipt_path = directory / "receipt.json"
    require(digest(receipt_path) == expected_sha, "receipt_pin_mismatch")
    receipt = load_json(receipt_path)
    require(receipt["status"] == "completed" and receipt["errors"] == [] and
            receipt["sources_stable"] is True and
            receipt["source_before"] == receipt["source_after"] and
            receipt["public_status"] == "not_claimed", "capture_not_closed_completed")
    actual_names = {path.name for path in directory.iterdir()}
    require(actual_names == set(receipt["artifacts"]) | {"receipt.json"}, "capture_inventory_mismatch")
    data = {}
    for name, record in receipt["artifacts"].items():
        require(Path(name).name == name and name not in (".", "..", "receipt.json"), "unsafe_artifact_name")
        content = read_bytes(directory / name)
        require(set(record) == {"sha256", "bytes"} and type(record["bytes"]) is int and
                record["bytes"] == len(content) and hashlib.sha256(content).hexdigest() == record["sha256"],
                "capture_artifact_mismatch: " + name)
        data[name] = content
    require(digest(receipt_path) == expected_sha, "receipt_changed_after_capture")
    return receipt, data


def json_bytes(data: bytes):
    return json.loads(data, object_pairs_hook=G.unique_object,
                      parse_constant=lambda value: (_ for _ in ()).throw(RuntimeError("nonfinite_json_" + value)))


def command_evidence(receipt: dict, data: dict[str, bytes], commands: list) -> None:
    require(set(receipt["commands"]) == {name for name, _, _ in commands}, "command_inventory_mismatch")
    for label, argv, rc in commands:
        row = receipt["commands"][label]
        require(row == json_bytes(data[label + ".result.json"]) and row["argv"] == argv and
                row["expected_rc"] == rc and row["returncode"] == rc and
                row["status"] == "completed" and row["error"] is None and row["timed_out"] is False,
                "command_not_completed_as_declared: " + label)
        attempt = json_bytes(data[label + ".attempt.json"])
        require(attempt["argv"] == argv and attempt["expected_rc"] == rc and
                attempt["cwd"] == str(ROOT), "command_attempt_mismatch")
        spawned = json_bytes(data[label + ".spawned.json"])
        require(type(spawned["pid"]) is int and spawned["pid"] > 0 and
                spawned["pgid"] == spawned["pid"], "owned_group_missing")
        for stream in ("stdout", "stderr"):
            require(hashlib.sha256(data[label + "." + stream]).hexdigest() == row[stream + "_sha256"],
                    "command_stream_mismatch")


def admit_geometry(expected_sha: str) -> dict:
    require(pin(expected_sha, "geometry_receipt") == GEOMETRY_RECEIPT_SHA, "geometry_receipt_not_approved")
    receipt, data = closed_capture(G.OUTPUT, pin(expected_sha, "geometry_receipt"))
    require(receipt["schema"] == "mhgp7-private-meb-dual-budget-geometry-capture-v1" and
            receipt["observer_route"] == "Trace_only_NoObserver_not_qualified" and
            receipt["testing_macro"] is False and receipt["sanitizers"] is False and
            receipt["gcp_used"] is False and receipt["combined_deadline_seconds"] == 60 and
            receipt["meb_work_accounting"] == "reference_ordinal_plus_proposal_v1", "geometry_capture_schema")
    live = G.source_snapshot(GEOMETRY_SHA)
    require(receipt["source_before"] == live and json_bytes(data["inputs_before.json"]) == live and
            json_bytes(data["inputs_after.json"]) == live, "geometry_source_binding")
    command_evidence(receipt, data, G.commands())
    for label, _, _ in G.commands():
        G.judge_output(label, data[label + ".stdout"], data[label + ".stderr"])
    G.paired_judge(data["nominal.stdout"], data["charge_after_mutant.stdout"])
    binding = G.dependency_binding(data["dependencies.d"], live)
    require(json_bytes(data["dependency_binding.json"]) == binding, "geometry_compile_binding")
    binary_sha = hashlib.sha256(data["geometry_gate"]).hexdigest()
    require(json_bytes(data["binary_before.json"]) == {"sha256": binary_sha} and
            json_bytes(data["binary_after.json"]) == {"sha256": binary_sha}, "geometry_binary_binding")
    require(digest(G.OUTPUT / "receipt.json") == expected_sha and
            G.source_snapshot(GEOMETRY_SHA) == live, "geometry_terminal_drift")
    return {"receipt_sha256": expected_sha, "binary_sha256": binary_sha,
            "source": live, "nominal": G.geometric_record(data["nominal.stdout"]),
            "scope": "Trace_only_not_yet_cost_NoObserver"}


def source_snapshot(expected_runner: str) -> dict[str, str]:
    require(digest(BASE / "run_cost.py") == pin(expected_runner, "runner"), "cost_runner_pin_mismatch")
    result = G.source_snapshot(GEOMETRY_SHA)
    for path, expected in PINS.items():
        require(digest(path) == expected, "cost_authority_pin_mismatch: " + str(path))
        result[str(path)] = expected
    for name in ("run_cost.py", "selftest.py", "README.md"):
        result[str(BASE / name)] = digest(BASE / name)
    return result


def build_commands() -> list:
    return [
        ("compiler", [str(G.COMPILER), "--version"], 0),
        ("compile", [str(G.COMPILER), "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic",
                     "-Werror", "-pthread", "-fno-lto", "-I", str(G.PRODUCT), "-MMD", "-MF",
                     str(BUILD / "dependencies.d"), str(BASE / "cost_harness.cpp"), "-o", str(BINARY)], 0),
        ("disassembly", [str(OBJDUMP), "-d", "-C", str(BINARY)], 0),
    ]


def dependency_binding(data: bytes, admitted: dict[str, str]) -> dict:
    text = data.decode("utf-8").replace("\\\n", " ")
    require(text.count(":") == 1 and "$" not in text, "unsupported_depfile_syntax")
    target, body = text.split(":", 1)
    require(shlex.split(target) == [str(BINARY)], "wrong_cost_depfile_target")
    paths = [str((ROOT / name).resolve()) for name in shlex.split(body)]
    mandatory = {str(BASE / "cost_harness.cpp"), str(G.BASE / "geometry_gate.cpp"),
                 str(G.BASE / "additional_scenes.inc"), str(G.PRIOR / "pivot.hpp"),
                 str(ROOT / "build/v7_meb_pivot_prototype/pivot.hpp"),
                 str(G.PRODUCT / "src/forest/silent_incidence.hpp")}
    require(paths and len(paths) == len(set(paths)) and mandatory <= set(paths) <= set(admitted),
            "cost_dependency_inventory")
    result = {name: digest(Path(name)) for name in sorted(paths)}
    require(all(admitted[name] == value for name, value in result.items()), "cost_dependency_drift")
    return result


def admit_build(expected_receipt: str, expected_binary: str, expected_disassembly: str,
                geometry_sha: str, sources: dict) -> dict:
    receipt, data = closed_capture(BUILD, pin(expected_receipt, "build_receipt"))
    require(receipt["schema"] == "mhgp7-private-dual-budget-cost-capture-v1" and receipt["stage"] == "build" and
            receipt["measurement_executed"] is False and receipt["deadline_seconds"] == 60 and
            receipt["geometry"]["receipt_sha256"] == geometry_sha and
            receipt["source_before"] == sources, "build_not_bound_to_current_cost")
    command_evidence(receipt, data, build_commands())
    require(hashlib.sha256(data["cost_harness"]).hexdigest() == pin(expected_binary, "binary") and
            hashlib.sha256(data["disassembly.stdout"]).hexdigest() == pin(expected_disassembly, "reviewed_disassembly"),
            "reviewed_binary_or_disassembly_mismatch")
    require(receipt["binary_sha256"] == expected_binary and
            json_bytes(data["binary_before.json"]) == {"sha256": expected_binary} and
            json_bytes(data["binary_after.json"]) == {"sha256": expected_binary}, "cost_binary_binding")
    require(json_bytes(data["dependency_binding.json"]) == dependency_binding(data["dependencies.d"], sources),
            "cost_compile_binding")
    require(json_bytes(data["inputs_before.json"]) == sources and json_bytes(data["inputs_after.json"]) == sources,
            "cost_build_source_files")
    return {"receipt_sha256": expected_receipt, "binary_sha256": expected_binary,
            "reviewed_disassembly_sha256": expected_disassembly}


def mix(value: int, word: int) -> int:
    return ((value ^ word) * 1099511628211) & U64


def work_hash(words) -> int:
    value = HASH_SEED
    for word in words:
        value = mix(value, word)
    return value


def expected_jobs(rows: list[dict]) -> list[dict]:
    """Independent closed schedule; observed R chooses only the declared cap triplet."""
    ranks = [rows[24 * i]["R"] for i in range(384)]
    require(all(type(r) is int and 1 <= r <= 550 for r in ranks), "cost_rank_domain")
    require(ranks[0] == ranks[1] == 1 and ranks[348] == 55 and ranks[350] == 550, "cost_named_ranks")
    result = []

    def add(order, l, p, c=0, proposed=0, pivot=16, cohort="main", steps=1, repeats=1):
        scene = order // 2 if order < 352 else (0 if order < 354 else (1 if order < 360 else 2))
        if order == 384:
            scene = 176
        n = ([2, 3, 4, 4, 5, 3, 4, 3][scene] if scene < 8 else
             (2 + (scene - 8) % 10 if scene < 168 else [3, 3, 3, 4, 4, 4, 11, 11, 3][scene - 168]))
        result.append(dict(id=len(result), order=order, scene=scene, R=4 if order == 384 else ranks[order],
                           L=l, P=p, c=c, p=proposed, n=n, pivot_cap=pivot, cohort=cohort, steps=steps, repeats=repeats))

    for order, r in enumerate(ranks):
        for p in (0, 1, 4, 5, 15, 16, 25, 401):
            for l in (r - 1, r, r + 1):
                add(order, l, p)
    for scene in (0, 1, 2, 174, 175):
        order, r = 2 * scene, ranks[2 * scene]
        for p in (0, 1, 401):
            for l in (7 + r - 1, 7 + r, 7 + r + 1):
                add(order, l, p, 7, cohort="boundary")
        for c, l in ((0, 0), (U64, U64), (U64, U64 - 1), (U64 - 1, U64), (U64 - 4, U64)):
            for p in (0, 401):
                add(order, l, p, c, cohort="boundary")
        for proposed, p in ((U64, U64), (U64 - 1, U64), (5, 4)):
            add(order, 551, p, proposed=proposed, cohort="boundary")
        add(order, 551, 401, pivot=0, cohort="boundary")
    add(384, 12, 7, cohort="cumulative_P7", steps=4)
    add(384, 8, 0, cohort="cumulative_P0", steps=2)
    for pivot in (17, U64):
        add(4, 551, 401, pivot=pivot, cohort="boundary")
    for order in (0, 1):
        for p in (0, 1, 401):
            for l in (1, 2):
                add(order, l, p, cohort="immediate_q2", repeats=4096)
    require(len(result) == 9347 and sum(job["steps"] for job in result) == 9351, "cost_expected_inventory")
    return result


def judge_measurement(stdout: bytes, stderr: bytes, geometry_sha: str) -> dict:
    require(stderr == b"" and stdout.endswith(b"\n") and len(stdout) <= 128 * 1024 * 1024,
            "cost_stream_empty_truncated_or_oversize")
    lines = stdout.splitlines()
    require(len(lines) <= 200000, "cost_output_line_bound")
    values = [json_bytes(line) for line in lines]
    header, terminal = values[0], values[-1]
    expected_header = dict(kind="header", schema="mhgp7-private-dual-budget-cost-v1",
            geometry_receipt_sha256=geometry_sha, public_status="not_claimed", scenes=176, orders=384,
            boundary_calls=123, jobs=9347, NoObserver=True, capture_included=True, ordinals=1507,
            calls_per_arm_pass=58491)
    require(type(header) is dict and set(header) == set(expected_header) and
            all(type(header[key]) is type(value) and header[key] == value for key, value in expected_header.items()),
            "cost_header")
    terminal_fields = {"kind", "status", "public_status", "helper_entries", "max_helper_entries",
                       "clock_tick_ns", "groups", "warmups", "measured_passes",
                       "full_terminals_equal_before_after", "timed_captures_equal"}
    require(type(terminal) is dict and set(terminal) == terminal_fields and
            all(type(terminal[name]) is str for name in ("kind", "status", "public_status")) and
            all(type(terminal[name]) is int and 0 <= terminal[name] <= U64 for name in
                ("helper_entries", "max_helper_entries", "clock_tick_ns", "groups", "warmups", "measured_passes")) and
            terminal["kind"] == "terminal" and terminal["status"] == "completed" and
            terminal["public_status"] == "not_claimed" and terminal["max_helper_entries"] == 2000000 and
            terminal["full_terminals_equal_before_after"] is True and terminal["timed_captures_equal"] is True and
            terminal["warmups"] == 2 and terminal["measured_passes"] == 7 and
            type(terminal["helper_entries"]) is int and 0 < terminal["helper_entries"] <= 2000000 and
            type(terminal["clock_tick_ns"]) is int and 0 < terminal["clock_tick_ns"] <= 1000000000,
            "cost_terminal_not_closed")
    case_rows = values[1:9352]
    require(len(case_rows) == 9351 and all(row["kind"] == "case" for row in case_rows), "cost_case_inventory")
    jobs = expected_jobs(case_rows)
    row_fields = {"kind", "id", "step", "cohort", "scene", "order", "n", "q_result", "ok", "reason", "route",
                  "R", "L", "P", "c0", "p0", "pivot_cap", "legacy_delta", "proposal_delta",
                  "actual_F_fallback_candidates", "pivots_delta", "certified_delta", "fallback_delta",
                  "terminal_hash", "work_hash"}
    cursor = 0
    for job in jobs:
        job["rows"] = []
        c, p = job["c"], job["p"]
        work = [p, 0, 0, 0]
        for step in range(job["steps"]):
            row = case_rows[cursor]
            cursor += 1
            require(set(row) == row_fields and row["id"] == job["id"] and row["step"] == step and
                    all(row[k] == job[k] for k in ("cohort", "scene", "order", "n", "R", "L", "P", "pivot_cap")),
                    "cost_case_schedule_mismatch")
            integers = row_fields - {"kind", "cohort", "q_result", "reason", "route"}
            require(all(type(row[k]) is int and 0 <= row[k] <= U64 for k in integers) and
                    2 <= row["n"] <= 11 and row["ok"] in (0, 1) and row["c0"] == c and row["p0"] == p,
                    "cost_case_integer_or_initial_state")
            dc, dp, fallback, certified = (row[k] for k in ("legacy_delta", "proposal_delta", "fallback_delta", "certified_delta"))
            require(dc == min(job["R"], max(0, job["L"] - c)) and dp <= max(0, job["P"] - p) and
                    dp <= 1 + 25 * min(16, job["pivot_cap"]) and fallback + certified <= 1 and
                    row["pivots_delta"] <= min(16, job["pivot_cap"]) and
                    row["actual_F_fallback_candidates"] == (dc if fallback else 0), "cost_physical_accounting")
            reason = row["reason"]
            require(reason in ("seed_local_state", "silent_meb_support_budget", "silent_local_nonessential_shell") and
                    bool(row["ok"]) == (reason == "seed_local_state") and
                    ((row["q_result"] is None) == (reason == "silent_meb_support_budget")), "cost_terminal_class")
            require(row["q_result"] is None or (type(row["q_result"]) is int and 2 <= row["q_result"] <= 4), "cost_arity")
            expected_route = "legacy_guard" if c >= job["L"] else (
                ("certificate_accepted" if row["ok"] else "certificate_legacy_refused") if certified else
                ("initial_P_fallback" if p >= job["P"] else "fallback_unattributed"))
            require(row["route"] == expected_route and (fallback + certified == (0 if c >= job["L"] else 1)),
                    "cost_route_attribution")
            c += dc
            p += dp
            work = [p, work[1] + row["pivots_delta"], work[2] + certified, work[3] + fallback]
            require(row["work_hash"] == work_hash(work), "cost_work_capture")
            job["rows"].append(row)
        qref = 3 if job["order"] == 384 else case_rows[24 * job["order"] + 1]["q_result"]
        require(qref in (2, 3, 4), "cost_reference_q")
        terminal_classes = ["success" if r["ok"] else
                            ("legacy_refused" if r["reason"] == "silent_meb_support_budget" else "shell_refused") for r in job["rows"]]
        job["group"] = (f'{job["cohort"]}/n={job["rows"][0]["n"]}/qref={qref}/P={job["P"]}/L={job["L"]}'
                        f'/c={job["c"]}/p={job["p"]}/pivot={job["pivot_cap"]}/terminal={"+".join(terminal_classes)}'
                        f'/route={"+".join(r["route"] for r in job["rows"])}')
    groups = {}
    for job in jobs:
        groups.setdefault(job["group"], []).append(job["id"])
    group_rows = values[9352:9352 + len(groups)]
    require(all(type(row) is dict and set(row) == {"kind", "group", "jobs"} and
                type(row["kind"]) is str and type(row["group"]) is str and type(row["jobs"]) is list and
                all(type(ident) is int for ident in row["jobs"]) for row in group_rows) and
            group_rows == [dict(kind="group", group=key, jobs=ids) for key, ids in sorted(groups.items())],
            "cost_group_inventory")
    times = values[9352 + len(groups):-1]
    require(terminal["groups"] == len(groups) and len(times) == 18 * len(groups), "cost_timing_inventory")
    expected_captures = {}
    for key, ids in groups.items():
        for arm in ("F", "dual"):
            th, wh, calls, nested = HASH_SEED, HASH_SEED, 0, 0
            for ident in ids:
                job = jobs[ident]
                for _ in range(job["repeats"]):
                    for row in job["rows"]:
                        th = mix(th, row["terminal_hash"])
                        wh = mix(wh, row["work_hash"] if arm == "dual" else work_hash([job["p"], 0, 0, 0]))
                        calls += 1
                        nested += row["fallback_delta"] if arm == "dual" else 0
            expected_captures[key, arm] = dict(calls=calls, nested_F_calls=nested, terminal_hash=th, work_hash=wh)
    cursor, timed_entries = 0, 0
    for warmup, passes in ((True, 2), (False, 7)):
        for passage in range(1, passes + 1):
            for key in sorted(groups):
                for arm in (("F", "dual") if passage % 2 else ("dual", "F")):
                    row = times[cursor]
                    cursor += 1
                    timing_fields = {"kind", "group", "arm", "pass", "warmup", "calls", "nested_F_calls",
                                     "elapsed_ns", "terminal_hash", "work_hash", "short_batch"}
                    require(type(row) is dict and set(row) == timing_fields and
                            all(type(row[name]) is str for name in ("kind", "group", "arm")) and
                            all(type(row[name]) is int and 0 <= row[name] <= U64 for name in
                                ("pass", "calls", "nested_F_calls", "elapsed_ns", "terminal_hash", "work_hash")) and
                            type(row["warmup"]) is bool and type(row["short_batch"]) is bool and
                            row["kind"] == "timing" and row["group"] == key and row["arm"] == arm and
                            row["pass"] == passage and row["warmup"] is warmup and
                            all(row[name] == value for name, value in expected_captures[key, arm].items()) and
                            type(row["elapsed_ns"]) is int and row["elapsed_ns"] >= 0 and
                            row["short_batch"] is (row["elapsed_ns"] < 100 * terminal["clock_tick_ns"]),
                            "cost_timing_order_or_capture")
                    timed_entries += row["calls"] + row["nested_F_calls"]
    # Both off-clock qualification passes: F+Trace, F+NoObserver, plus the
    # donor boundary replay; each nested fallback is accounted separately.
    total_fallback = sum(row["fallback_delta"] for row in case_rows)
    boundary_fallback = sum(row["fallback_delta"] for row in case_rows if row["cohort"] not in ("main", "immediate_q2"))
    expected_entries = 384 + 2 * (4 * 9351 + 2 * total_fallback + 246 + boundary_fallback) + timed_entries
    require(terminal["helper_entries"] == expected_entries, "cost_total_entry_accounting")
    return dict(status="completed", jobs=len(jobs), groups=len(groups), helper_entries=expected_entries,
                main_comparisons=9216, boundary_comparisons=123, immediate_q2_cases=12,
                measured_passes=7, warmups=2, claim="local_O2_helper_plus_capture_not_CLI_tower")


def run_command(output: Path, label: str, argv: list[str], deadline: float, environment: dict, cpu: int) -> dict:
    remaining = deadline - time.monotonic() - 2.0
    require(remaining > 0, "stage_deadline_exhausted")
    save(output / (label + ".attempt.json"), dict(argv=argv, expected_rc=0, cwd=str(ROOT),
         timeout_seconds=remaining, cpu_affinity=[cpu]))
    start, process, rc, error, timed_out = time.monotonic(), None, None, None, False
    with (output / (label + ".stdout")).open("xb") as stdout, (output / (label + ".stderr")).open("xb") as stderr:
        try:
            process = subprocess.Popen(argv, cwd=ROOT, env=environment, stdout=stdout, stderr=stderr, start_new_session=True)
            save(output / (label + ".spawned.json"), dict(pid=process.pid, pgid=process.pid))
            rc = process.wait(timeout=remaining)
            require(not G.group_present(process.pid), "descendant_survived_command")
        except BaseException as exception:
            timed_out = isinstance(exception, subprocess.TimeoutExpired)
            error = f"{type(exception).__name__}: {exception}"
        finally:
            if process is not None:
                try:
                    G.drain_group(process)
                    rc = process.returncode
                except BaseException as exception:
                    error = (error or "") + f"; cleanup: {type(exception).__name__}: {exception}"
    result = dict(argv=argv, expected_rc=0, returncode=rc, status="completed" if error is None else "failed",
                  error=error, timed_out=timed_out, elapsed_seconds=time.monotonic() - start,
                  stdout_sha256=digest(output / (label + ".stdout")), stderr_sha256=digest(output / (label + ".stderr")))
    save(output / (label + ".result.json"), result)
    return result


def execute(args) -> int:
    stage = args.stage
    output, cpu, seconds = (BUILD, 0, 60) if stage == "build" else (MEASURE, 6, 120)
    require(not output.exists(), "create_only_output_already_exists")
    # All authority/preflight failures occur before creating a new destination.
    before = source_snapshot(args.expected_runner_sha256)
    geometry = admit_geometry(args.expected_geometry_receipt_sha256)
    build = None
    if stage == "measure":
        require(args.disassembly_reviewed, "explicit_disassembly_review_required")
        build = admit_build(args.expected_build_receipt_sha256, args.expected_binary_sha256,
                            args.expected_disassembly_sha256, args.expected_geometry_receipt_sha256, before)
    require(not any(os.environ.get(name) for name in G.FORBIDDEN_ENV), "compiler_or_loader_override")
    require(cpu in os.sched_getaffinity(0), "required_cpu_unavailable")
    output.mkdir(exist_ok=False)
    started, deadline = datetime.now(timezone.utc).isoformat(), time.monotonic() + seconds
    errors, records, after, binding, binary_sha, judgment = [], {}, None, None, None, None
    hardware_before = None
    handlers = {sig: signal.signal(sig, G.interrupted) for sig in G.SIGNALS}
    original_affinity = os.sched_getaffinity(0)
    try:
        os.sched_setaffinity(0, {cpu})
        require(os.sched_getaffinity(0) == {cpu}, "affinity_not_effective")
        hardware_before = cpu_metadata(cpu)
        environment = dict(os.environ, LC_ALL="C", LANG="C", TZ="UTC", **{name: "1" for name in THREAD_ENV})
        save(output / "environment.json", dict(cpu_affinity=[cpu], affinity_before=sorted(original_affinity),
             hardware=hardware_before, host=list(os.uname()),
             thread_values_before={name: os.environ.get(name) for name in THREAD_ENV},
             thread_values_effective={name: environment[name] for name in THREAD_ENV},
             forbidden_overrides={name: os.environ.get(name) for name in G.FORBIDDEN_ENV}))
        save(output / "inputs_before.json", before)
        if stage == "measure":
            binary_sha = args.expected_binary_sha256
            save(output / "binary_before.json", {"sha256": binary_sha})
        commands = build_commands() if stage == "build" else [
            ("measurement", [str(BINARY), "--measure", args.expected_geometry_receipt_sha256], 0)]
        for label, argv, _ in commands:
            if binary_sha is not None:
                require(digest(BINARY) == binary_sha, "binary_changed_before_command")
            record = run_command(output, label, argv, deadline, environment, cpu)
            records[label] = record
            require(record["status"] == "completed" and record["returncode"] == 0, "unexpected_command_terminal: " + label)
            if label == "compile":
                binding = dependency_binding(read_bytes(BUILD / "dependencies.d"), before)
                save(output / "dependency_binding.json", binding)
                binary_sha = digest(BINARY)
                save(output / "binary_before.json", {"sha256": binary_sha})
            elif label == "measurement":
                judgment = judge_measurement(read_bytes(output / "measurement.stdout"),
                                              read_bytes(output / "measurement.stderr"), args.expected_geometry_receipt_sha256)
                save(output / "judgment.json", judgment)
        require(binary_sha is not None and (binding is not None if stage == "build" else judgment is not None),
                "missing_stage_evidence")
    except BaseException as exception:
        errors.append(f"{type(exception).__name__}: {exception}")
    finally:
        try:
            after = source_snapshot(args.expected_runner_sha256)
            save(output / "inputs_after.json", after)
            require(after == before and admit_geometry(args.expected_geometry_receipt_sha256) == geometry,
                    "terminal_source_or_geometry_drift")
            if stage == "measure":
                require(admit_build(args.expected_build_receipt_sha256, args.expected_binary_sha256,
                        args.expected_disassembly_sha256, args.expected_geometry_receipt_sha256, after) == build,
                        "build_changed_during_measurement")
            if binary_sha is not None:
                terminal_binary = digest(BINARY)
                save(output / "binary_after.json", {"sha256": terminal_binary})
                require(terminal_binary == binary_sha, "binary_changed_after_command")
            if binding is not None:
                require(dependency_binding(read_bytes(BUILD / "dependencies.d"), after) == binding, "compile_closure_drift")
            require(os.sched_getaffinity(0) == {cpu}, "affinity_changed")
            hardware_after = cpu_metadata(cpu)
            save(output / "environment_after.json", dict(cpu_affinity=sorted(os.sched_getaffinity(0)), hardware=hardware_after))
            require(hardware_after == hardware_before, "CPU_model_or_online_changed")
            require(time.monotonic() <= deadline, "stage_deadline_exceeded")
        except BaseException as exception:
            errors.append(f"final_validation: {type(exception).__name__}: {exception}")
        try:
            os.sched_setaffinity(0, original_affinity)
            save(output / "affinity_restored.json", {"cpu_affinity": sorted(os.sched_getaffinity(0))})
            require(os.sched_getaffinity(0) == original_affinity, "affinity_restore_not_effective")
        except BaseException as exception:
            errors.append(f"affinity_restore: {type(exception).__name__}: {exception}")
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    artifacts = {path.name: dict(sha256=digest(path), bytes=path.stat().st_size)
                 for path in sorted(output.iterdir()) if path.is_file()}
    if time.monotonic() > deadline:
        errors.append("stage_deadline_exceeded_at_closure")
    receipt = dict(schema="mhgp7-private-dual-budget-cost-capture-v1", stage=stage,
        status="completed" if not errors else "failed", errors=errors, started_at=started,
        finished_at=datetime.now(timezone.utc).isoformat(), deadline_seconds=seconds, commands=records,
        geometry=geometry, build=build, source_before=before, source_after=after, sources_stable=before == after,
        binary_sha256=binary_sha, artifacts=artifacts, measurement_executed=stage == "measure",
        public_status="not_claimed", gcp_used=False,
        scope="private_local_O2_helper_plus_capture_not_CLI_or_tower")
    save(output / "receipt.json", receipt)
    print(json.dumps(dict(status=receipt["status"], receipt=str(output / "receipt.json"),
                         receipt_sha256=digest(output / "receipt.json"), errors=errors)))
    return 0 if not errors else 1


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", choices=("build", "measure"), default="build")
    parser.add_argument("--execute", action="store_true")
    for name in ("runner", "geometry-receipt", "build-receipt", "binary", "disassembly"):
        parser.add_argument("--expected-" + name + "-sha256")
    parser.add_argument("--disassembly-reviewed", action="store_true")
    args = parser.parse_args(argv)
    if not args.execute:
        print(json.dumps(dict(status="prepared_not_executed", writes=False, subprocesses=0,
            stage=args.stage, deadline_seconds=60 if args.stage == "build" else 120,
            cpu_affinity=[0 if args.stage == "build" else 6], geometry_completed_pin_required=True,
            distinct_measurement_GO_and_binary_disassembly_pins_required=True), indent=2))
        return 0
    pin(args.expected_runner_sha256, "runner")
    pin(args.expected_geometry_receipt_sha256, "geometry_receipt")
    if args.stage == "measure":
        for name in ("build_receipt", "binary", "disassembly"):
            pin(getattr(args, "expected_" + name + "_sha256"), name)
    return execute(args)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, ValueError, KeyError, OSError) as error:
        print(json.dumps(dict(status="preflight_failed", error=f"{type(error).__name__}: {error}")))
        raise SystemExit(1)
