#!/usr/bin/env python3
"""Portable independent reader of preserved native-cost bytes; never runs C++."""

import argparse
from collections import Counter, defaultdict
from datetime import datetime
import gzip
import hashlib
import json
import math
from pathlib import Path, PurePosixPath
import posixpath
import shlex
import sys

HERE = Path(__file__).resolve().parent
AUDITS = HERE.parents[1]
ORIGINAL = "/workspaces/E-HGP"
BASE = ORIGINAL + "/build/v7_meb_dual_budget_cost_v2"
BINARY = BASE + "/build_20260905/cost_harness"
RECEIPT = "874f100ffb1d65956f6d640c5e7ab838a81e9f5c7900f7c1d69b14504235c208"
BUILD = "de6de29f55ab55d8edd64f9e3307d4748688635ca7338c36105555da39e0574f"
GEOMETRY = "b81d8e480b158710874de230c3485f79d0a42f1cb228e321c750de0f58bed49e"
BIN_SHA = "56e022c817d2e726eb2e3b135e78e577bbdf344ebd0ff352d64d1121300fd976"
DISASSEMBLY = "52392c6a8b9a8a230133113fdad0bfa9ca64b25291600349b9905be9f126c9c4"
U64 = (1 << 64) - 1
SEED = 1469598103934665603


def require(condition, cause):
    if not condition:
        raise ValueError(cause)


def unique(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate JSON key")
        result[key] = value
    return result


def decode(data):
    def nonfinite(value):
        raise ValueError("non-finite JSON " + value)
    return json.loads(data, object_pairs_hook=unique, parse_constant=nonfinite)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def uint(value):
    return type(value) is int and 0 <= value <= U64


def safe_path(name):
    require(type(name) is str and name and "\\" not in name, "unsafe audit path")
    path = PurePosixPath(name)
    require(not path.is_absolute() and all(x not in ("", ".", "..") for x in name.split("/")), "unsafe audit path")
    result = AUDITS.joinpath(*path.parts).resolve()
    require(AUDITS.resolve() in result.parents, "audit path escapes root")
    return result


def checked_bytes(data, record):
    require(type(record["bytes"]) is int and len(data) == record["bytes"] and sha(data) == record["sha256"], "captured bytes mismatch")
    return data


def load_capture():
    manifest = decode((HERE / "capture_manifest.json").read_bytes())
    require(manifest["schema"] == "mhgp7-native-cost-independent-capture-v1" and manifest["public_status"] == "not_claimed", "capture scope")
    entries, contents = manifest["entries"], {}
    require(type(entries) is dict and len(entries) == 176, "capture inventory")
    for key, record in entries.items():
        require(type(record["sha256"]) is str and len(record["sha256"]) == 64 and all(c in "0123456789abcdef" for c in record["sha256"]), "invalid source hash")
        if "audit_path" in record:
            stored = safe_path(record["audit_path"]).read_bytes()
            require(record["compression"] in ("none", "gzip"), "unknown compression")
            if record["compression"] == "gzip":
                require(sha(stored) == record["stored_sha256"] and len(stored) == record["stored_bytes"], "stored gzip mismatch")
                stored = gzip.decompress(stored)
            contents[key] = checked_bytes(stored, record)
        else:
            require(record["preservation"] == "hash_only_private_or_system_binary", "uncaptured non-binary authority")
    require(len(entries) - len(contents) == 5, "binary-only inventory")
    return manifest, contents


def closed(receipt, stage):
    require(receipt["schema"] == "mhgp7-private-dual-budget-cost-capture-v1" and receipt["stage"] == stage, "stage/schema mismatch")
    require(receipt["status"] == "completed" and receipt["errors"] == [] and receipt["sources_stable"] is True and receipt["source_before"] == receipt["source_after"], "unclosed or drifting receipt")
    require(receipt["public_status"] == "not_claimed" and receipt["gcp_used"] is False and receipt["scope"] == "private_local_O2_helper_plus_capture_not_CLI_or_tower", "receipt scope")
    require(receipt["measurement_executed"] is (stage == "measure"), "measurement execution mismatch")
    duration = (datetime.fromisoformat(receipt["finished_at"]) - datetime.fromisoformat(receipt["started_at"])).total_seconds()
    require(0 < duration <= receipt["deadline_seconds"] == (120 if stage == "measure" else 60), "stage time bound")
    return duration


def command(record, attempt, spawned, stdout, stderr, argv, cpu, seconds):
    require(record["argv"] == attempt["argv"] == argv and attempt["cwd"] == ORIGINAL and attempt["cpu_affinity"] == [cpu], "command intent mismatch")
    require(type(record["returncode"]) is int and record["returncode"] == record["expected_rc"] == attempt["expected_rc"] == 0 and record["status"] == "completed" and record["error"] is None and record["timed_out"] is False, "command not terminal zero")
    require(type(record["elapsed_seconds"]) in (int, float) and math.isfinite(record["elapsed_seconds"]) and 0 < record["elapsed_seconds"] <= attempt["timeout_seconds"] < seconds, "command time bound")
    require(type(spawned["pid"]) is int and spawned["pid"] == spawned["pgid"] > 0, "owned process record")
    require(sha(stdout) == record["stdout_sha256"] and sha(stderr) == record["stderr_sha256"] and stderr == b"", "command stream binding")


def mix(value, word):
    return ((value ^ word) * 1099511628211) & U64


def folded(words, initial=SEED):
    for word in words:
        initial = mix(initial, word)
    return initial


def schedule(cases):
    # Reconstruct the finite corpus inventory from the pinned source, not from
    # the printed grouping. Only each observed F rank chooses its cap triplet.
    ranks = [cases[24 * order]["R"] for order in range(384)]
    require(all(uint(r) and 1 <= r <= 550 for r in ranks), "rank domain")
    require([ranks[i] for i in (0, 1, 348, 350)] == [1, 1, 55, 550], "named ranks")
    jobs = []

    def add(order, limit, proposal, c=0, p=0, pivot=16, cohort="main", steps=1, repeats=1):
        scene = order // 2 if order < 352 else (0 if order < 354 else 1 if order < 360 else 2)
        if order == 384:
            scene = 176
        n = ([2, 3, 4, 4, 5, 3, 4, 3][scene] if scene < 8 else
             2 + (scene - 8) % 10 if scene < 168 else [3, 3, 3, 4, 4, 4, 11, 11, 3][scene - 168])
        jobs.append(dict(id=len(jobs), order=order, scene=scene, n=n, R=4 if order == 384 else ranks[order],
                         L=limit, P=proposal, c=c, p=p, pivot_cap=pivot, cohort=cohort, steps=steps, repeats=repeats))

    for order, rank in enumerate(ranks):
        for proposal in (0, 1, 4, 5, 15, 16, 25, 401):
            for limit in range(rank - 1, rank + 2):
                add(order, limit, proposal)
    for scene in (0, 1, 2, 174, 175):
        order, rank = 2 * scene, ranks[2 * scene]
        for proposal in (0, 1, 401):
            for limit in range(7 + rank - 1, 7 + rank + 2):
                add(order, limit, proposal, c=7, cohort="boundary")
        for c, limit in ((0, 0), (U64, U64), (U64, U64 - 1), (U64 - 1, U64), (U64 - 4, U64)):
            for proposal in (0, 401):
                add(order, limit, proposal, c=c, cohort="boundary")
        for p, proposal in ((U64, U64), (U64 - 1, U64), (5, 4)):
            add(order, 551, proposal, p=p, cohort="boundary")
        add(order, 551, 401, pivot=0, cohort="boundary")
    add(384, 12, 7, cohort="cumulative_P7", steps=4)
    add(384, 8, 0, cohort="cumulative_P0", steps=2)
    for pivot in (17, U64):
        add(4, 551, 401, pivot=pivot, cohort="boundary")
    for order in (0, 1):
        for proposal in (0, 1, 401):
            for limit in (1, 2):
                add(order, limit, proposal, cohort="immediate_q2", repeats=4096)
    require(len(jobs) == 9347 and sum(j["steps"] for j in jobs) == 9351, "finite schedule non-vacuity")
    return jobs


CASE_FIELDS = set("kind id step cohort scene order n q_result ok reason route R L P c0 p0 pivot_cap legacy_delta proposal_delta actual_F_fallback_candidates pivots_delta certified_delta fallback_delta terminal_hash work_hash".split())


def check_case(row, job, step, c, work):
    require(type(row) is dict and set(row) == CASE_FIELDS and row["kind"] == "case", "case shape")
    integers = CASE_FIELDS - {"kind", "cohort", "q_result", "reason", "route"}
    require(all(uint(row[k]) for k in integers) and row["ok"] in (0, 1), "case integer types")
    require(row["id"] == job["id"] and row["step"] == step and all(row[k] == job[k] for k in ("cohort", "scene", "order", "n", "R", "L", "P", "pivot_cap")), "case schedule")
    require(row["c0"] == c and row["p0"] == work[0], "cumulative state reset")
    dc, dp, fb, cert, pivots = (row[k] for k in ("legacy_delta", "proposal_delta", "fallback_delta", "certified_delta", "pivots_delta"))
    require(dc == min(job["R"], max(0, job["L"] - c)) and dp <= max(0, job["P"] - work[0]) and dp <= 1 + 25 * min(16, job["pivot_cap"]) and pivots <= min(16, job["pivot_cap"]) and fb + cert == int(c < job["L"]), "case prospective budget accounting")
    require(row["actual_F_fallback_candidates"] == (dc if fb else 0), "fallback charge accounting")
    require(row["reason"] in ("seed_local_state", "silent_meb_support_budget", "silent_local_nonessential_shell") and bool(row["ok"]) == (row["reason"] == "seed_local_state"), "case terminal class")
    require((row["q_result"] is None) == (row["reason"] == "silent_meb_support_budget") and (row["q_result"] is None or type(row["q_result"]) is int and 2 <= row["q_result"] <= 4), "case support arity")
    route = "legacy_guard" if c >= job["L"] else (("certificate_accepted" if row["ok"] else "certificate_legacy_refused") if cert else ("initial_P_fallback" if work[0] >= job["P"] else "fallback_unattributed"))
    require(row["route"] == route, "case route attribution")
    after = [work[0] + dp, work[1] + pivots, work[2] + cert, work[3] + fb]
    require(all(uint(v) for v in after) and row["work_hash"] == folded(after), "case Work digest")
    return c + dc, after


def check_header(header):
    expected = dict(kind="header", schema="mhgp7-private-dual-budget-cost-v1", geometry_receipt_sha256=GEOMETRY,
                    public_status="not_claimed", scenes=176, orders=384, boundary_calls=123, jobs=9347,
                    NoObserver=True, capture_included=True, ordinals=1507, calls_per_arm_pass=58491)
    require(type(header) is dict and set(header) == set(expected) and all(type(header[k]) is type(v) and header[k] == v for k, v in expected.items()), "header scope/inventory")


def check_terminal(terminal, groups, entries):
    expected = dict(kind="terminal", status="completed", public_status="not_claimed", helper_entries=entries,
                    max_helper_entries=2000000, groups=groups, warmups=2, measured_passes=7,
                    full_terminals_equal_before_after=True, timed_captures_equal=True)
    require(type(terminal) is dict and set(terminal) == set(expected) | {"clock_tick_ns"} and all(type(terminal[k]) is type(v) and terminal[k] == v for k, v in expected.items()), "terminal qualification/ledger")
    require(uint(terminal["clock_tick_ns"]) and 0 < terminal["clock_tick_ns"] <= 1000000000 and 0 < entries <= 2000000, "terminal non-vacuity/bounds")


def check_timing(row, key, arm, passage, warmup, expected, tick):
    require(set(row) == set("kind group arm pass warmup calls nested_F_calls elapsed_ns terminal_hash work_hash short_batch".split()), "timing shape")
    require(row["kind"] == "timing" and row["group"] == key and row["arm"] == arm and type(row["warmup"]) is bool and row["warmup"] is warmup and type(row["pass"]) is int and row["pass"] == passage, "timing paired order")
    require(all(uint(row[k]) for k in ("calls", "nested_F_calls", "elapsed_ns", "terminal_hash", "work_hash")) and all(row[k] == v for k, v in expected.items()), "timing actual calls/digests")
    require(type(row["short_batch"]) is bool and row["short_batch"] is (row["elapsed_ns"] < 100 * tick), "timing resolution annotation")


def measurement(data):
    require(data.endswith(b"\n") and len(data) < 128 * 1024 * 1024, "raw stream termination/size")
    lines = data.splitlines()
    require(len(lines) == 98634, "raw line inventory")
    values = [decode(line) for line in lines]
    header, terminal = values[0], values[-1]
    check_header(header)
    cases = values[1:9352]
    jobs = schedule(cases)
    groups, position = defaultdict(list), 0
    for job in jobs:
        job["rows"] = cases[position:position + job["steps"]]
        position += job["steps"]
        c, work = job["c"], [job["p"], 0, 0, 0]
        for step, row in enumerate(job["rows"]):
            c, work = check_case(row, job, step, c, work)
        q = 3 if job["order"] == 384 else cases[24 * job["order"] + 1]["q_result"]
        require(q in (2, 3, 4), "group reference arity")
        labels = ["success" if r["ok"] else "legacy_refused" if r["q_result"] is None else "shell_refused" for r in job["rows"]]
        key = (f'{job["cohort"]}/n={job["n"]}/qref={q}/P={job["P"]}/L={job["L"]}/c={job["c"]}/p={job["p"]}'
               f'/pivot={job["pivot_cap"]}/terminal={"+".join(labels)}/route={"+".join(r["route"] for r in job["rows"])}')
        groups[key].append(job["id"])
    keys = sorted(groups)
    require(len(keys) == 4699, "nonempty exact group inventory")
    group_rows = values[9352:9352 + len(keys)]
    require(group_rows == [dict(kind="group", group=k, jobs=groups[k]) for k in keys] and all(type(i) is int for r in group_rows for i in r["jobs"]), "group membership/order")
    expected = {}
    for key in keys:
        for arm in ("F", "dual"):
            th, wh, calls, nested = SEED, SEED, 0, 0
            for ident in groups[key]:
                job = jobs[ident]
                for _ in range(job["repeats"]):
                    for row in job["rows"]:
                        th = mix(th, row["terminal_hash"])
                        wh = mix(wh, row["work_hash"] if arm == "dual" else folded([job["p"], 0, 0, 0]))
                        calls += 1
                        nested += row["fallback_delta"] if arm == "dual" else 0
            expected[key, arm] = dict(terminal_hash=th, work_hash=wh, calls=calls, nested_F_calls=nested)
    require(all(sum(expected[k, arm]["calls"] for k in keys) == 58491 for arm in ("F", "dual")), "calls per arm/pass")
    timings = values[9352 + len(keys):-1]
    require(len(timings) == 18 * len(keys), "timing matrix non-vacuity")
    check_terminal(terminal, len(keys), terminal["helper_entries"])
    cursor, timed_entries = 0, 0
    timer_ns, short_counts = Counter(), Counter()
    for warmup, passes in ((True, 2), (False, 7)):
        for passage in range(1, passes + 1):
            for key in keys:
                for arm in (("F", "dual") if passage % 2 else ("dual", "F")):
                    row = timings[cursor]
                    cursor += 1
                    check_timing(row, key, arm, passage, warmup, expected[key, arm], terminal["clock_tick_ns"])
                    timed_entries += row["calls"] + row["nested_F_calls"]
                    phase = "warmup" if warmup else "measured"
                    timer_ns[phase + "/" + arm] += row["elapsed_ns"]
                    short_counts[phase + "/" + arm] += int(row["short_batch"])
    fb = sum(r["fallback_delta"] for r in cases)
    boundary_fb = sum(r["fallback_delta"] for r in cases if r["cohort"] not in ("main", "immediate_q2"))
    offclock = 384 + 2 * (4 * len(cases) + 2 * fb + 246 + boundary_fb)
    check_terminal(terminal, len(keys), timed_entries + offclock)
    return dict(status="completed", jobs=len(jobs), cases=len(cases), groups=len(keys), timing_rows=len(timings),
                calls_per_arm_pass=58491, timed_entries=timed_entries, off_clock_entries=offclock,
                helper_entries=timed_entries + offclock, clock_tick_ns=terminal["clock_tick_ns"],
                timer_sums_ns=dict(timer_ns), short_batch_counts=dict(short_counts),
                case_routes=dict(Counter(r["route"] for r in cases)),
                main_comparisons=9216, boundary_comparisons=123, immediate_q2_cases=12, measured_passes=7, warmups=2)


def inspect(live=False):
    manifest, data = load_capture()
    obj = lambda key: decode(data[key])
    require(sha(data["run/receipt.json"]) == RECEIPT and sha(data["build/receipt.json"]) == BUILD and sha(data["geometry/receipt.json"]) == GEOMETRY, "receipt authority seals")
    run, build, geo = (obj(k + "/receipt.json") for k in ("run", "build", "geometry"))
    durations = dict(measure=closed(run, "measure"), build=closed(build, "build"))
    pins = manifest["source_authorities_verified_live"]
    require(len(pins) == 72 and run["source_before"] == build["source_before"] == pins, "72 shared source authorities")
    require(geo["source_before"] == geo["source_after"] and geo["status"] == "completed" and geo["errors"] == [], "geometry closure")
    require(run["geometry"] == build["geometry"] and run["geometry"]["source"] == geo["source_before"] and run["geometry"]["nominal"] == obj("geometry/nominal.stdout"), "geometry raw linkage")
    require(run["build"] == dict(receipt_sha256=BUILD, binary_sha256=BIN_SHA, reviewed_disassembly_sha256=DISASSEMBLY) and sha(data["build/disassembly.stdout"]) == DISASSEMBLY, "reviewed binary/build binding")
    entry = manifest["entries"]
    for prefix, receipt in (("run", run), ("build", build), ("geometry", geo)):
        actual = {k[len(prefix) + 1:] for k in entry if k.startswith(prefix + "/")}
        require(actual == set(receipt["artifacts"]) | {"receipt.json"}, "stage artifact inventory")
        for name, record in receipt["artifacts"].items():
            preserved = entry[prefix + "/" + name]
            require(record == {k: preserved[k] for k in ("bytes", "sha256")}, "stage artifact seal")
    compiler = "/usr/bin/x86_64-linux-gnu-g++-13"
    commands = {"compiler": [compiler, "--version"],
                "compile": [compiler, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread", "-fno-lto", "-I", ORIGINAL + "/morsehgp3D_v7", "-MMD", "-MF", BASE + "/build_20260905/dependencies.d", BASE + "/cost_harness.cpp", "-o", BINARY],
                "disassembly": ["/usr/bin/x86_64-linux-gnu-objdump", "-d", "-C", BINARY],
                "measurement": [BINARY, "--measure", GEOMETRY]}
    process_ids = []
    hosts = []
    for prefix, receipt, cpu in (("build", build, 0), ("run", run, 6)):
        require(set(receipt["commands"]) == ({"measurement"} if prefix == "run" else set(commands) - {"measurement"}), "command inventory")
        require(receipt["binary_sha256"] == BIN_SHA and obj(prefix + "/binary_before.json") == obj(prefix + "/binary_after.json") == {"sha256": BIN_SHA}, "binary before/after")
        require(obj(prefix + "/inputs_before.json") == obj(prefix + "/inputs_after.json") == pins, "source snapshots")
        env, after = obj(prefix + "/environment.json"), obj(prefix + "/environment_after.json")
        require(env["cpu_affinity"] == after["cpu_affinity"] == [cpu] and env["hardware"] == after["hardware"] and env["hardware"]["selected_cpu"]["processor"] == str(cpu), "affinity/hardware drift")
        require(obj(prefix + "/affinity_restored.json")["cpu_affinity"] == env["affinity_before"] and all(v is None for v in env["forbidden_overrides"].values()) and set(env["thread_values_effective"].values()) == {"1"}, "restored affinity/environment")
        hosts.append(env["host"])
        stage_pids = []
        for label, result in receipt["commands"].items():
            require(result == obj(prefix + "/" + label + ".result.json"), "raw result summary disagreement")
            spawn = obj(prefix + "/" + label + ".spawned.json")
            command(result, obj(prefix + "/" + label + ".attempt.json"), spawn,
                    data[prefix + "/" + label + ".stdout"], data[prefix + "/" + label + ".stderr"],
                    commands[label], cpu, receipt["deadline_seconds"])
            process_ids.append(spawn["pid"])
            stage_pids.append(spawn["pid"])
        require(len(set(stage_pids)) == len(stage_pids), "distinct process records within stage")
    require(hosts[0] == hosts[1] and len(process_ids) == 4, "host/process records")
    dep = data["build/dependencies.d"].decode().replace("\\\n", " ")
    target, sources = dep.split(":")
    require(shlex.split(target) == [BINARY], "depfile target")
    dependencies = [posixpath.normpath(posixpath.join(ORIGINAL, v)) for v in shlex.split(sources)]
    require(len(dependencies) == len(set(dependencies)) == 22 and set(dependencies) <= set(pins), "local dependency inventory")
    require(obj("build/dependency_binding.json") == {k: pins[k] for k in dependencies}, "dependency source closure")
    for origin, pin in pins.items():
        key = "authority/" + (origin[len(ORIGINAL) + 1:] if origin.startswith(ORIGINAL + "/") else origin.lstrip("/"))
        require(entry[key]["origin"] == origin and entry[key]["sha256"] == pin, "preserved authority mapping")
    require(all("audit_path" in entry["authority/" + k[len(ORIGINAL) + 1:]] for k in dependencies), "local dependency bytes absent")
    for line in data["protocol/SHA256SUMS"].decode().splitlines():
        digest, name = line.split("  ", 1)
        key = "protocol_selftests/" + name.split("/", 1)[1] if name.startswith("protocol_selftests_20260905/") else "protocol/" + name
        require(sha(data[key]) == digest, "protocol seal mismatch")
    failed = obj("prior_failed/receipt.json")
    require(sha(data["prior_failed/receipt.json"]) == "247c952cd6000812ed0bff04390a0848c81e527c74c0e0ac26244144f4c83c15" and failed["status"] == "failed" and failed["measurement_executed"] is False and failed["binary_sha256"] is None, "prior failure attribution")
    require(obj("prior_failed/compile.result.json") == failed["commands"]["compile"] and failed["commands"]["compile"]["returncode"] == 1 and sha(data["prior_failed/compile.stderr"]) == failed["commands"]["compile"]["stderr_sha256"], "prior raw compiler failure")
    before, after = data["prior_failed/cost_harness.cpp"], data["protocol/cost_harness.cpp"]
    require(before.replace(b"#define main mhgp7_geometry_gate_uninvoked_main", b"#define main(...) mhgp7_geometry_gate_uninvoked_main(__VA_ARGS__)") == after, "v1/v2 single macro repair")
    require(sha(before) == failed["source_before"][ORIGINAL + "/build/v7_meb_dual_budget_cost/cost_harness.cpp"], "v1 source pin")
    parsed = measurement(data["run/measurement.stdout"])
    expected_judgment = {k: parsed[k] for k in ("status", "jobs", "groups", "helper_entries", "main_comparisons", "boundary_comparisons", "immediate_q2_cases", "measured_passes", "warmups")}
    expected_judgment["claim"] = "local_O2_helper_plus_capture_not_CLI_tower"
    require(obj("run/judgment.json") == expected_judgment, "independent judgment disagreement")
    elapsed = run["commands"]["measurement"]["elapsed_seconds"]
    require(sum(parsed["timer_sums_ns"].values()) / 1e9 <= elapsed <= durations["measure"], "nested timing interval bounds")
    live_result = {"performed": live}
    if live:
        live_pins = {**pins, BINARY: BIN_SHA, BASE + "/run_20260905/receipt.json": RECEIPT, BASE + "/build_20260905/receipt.json": BUILD}
        require(all(sha(Path(k).read_bytes()) == v for k, v in live_pins.items()), "current authority drift")
        live_result["pins_verified"] = len(live_pins)
    return dict(status="constructor_native_cost_receipt_verified", phase="exploration_v7_hors_registre", backend="cpu_reference",
                profile="quantized_u16_input_only", mode="audit_independant_math_and_architecture", public_status="not_claimed",
                receipt_sha256=RECEIPT, source_authorities=72, local_compile_dependencies=22,
                capture_manifest_sha256=sha((HERE / "capture_manifest.json").read_bytes()),
                stage_wall_seconds=durations, measurement_process_seconds=elapsed,
                started_at=run["started_at"], finished_at=run["finished_at"], host=hosts[1],
                measurement_environment=obj("run/environment.json"), parsed=parsed, live=live_result,
                auditor_engine_or_build_runs=0, GCP="non utilisé",
                scope="Constructor native NoObserver/F/Trace comparisons and helper-plus-capture timings; bounded corpus, shared host, no CLI/tower/SLO promotion.")


def self_test():
    _, data = load_capture()
    lines = data["run/measurement.stdout"].splitlines()
    header, first, terminal = decode(lines[0]), decode(lines[1]), decode(lines[-1])
    cases = [decode(x) for x in lines[1:9352]]
    job = schedule(cases)[0]
    timed = decode(lines[9352 + 4699])
    exp = {k: timed[k] for k in ("calls", "nested_F_calls", "terminal_hash", "work_hash")}
    check_header(header)
    check_case(first, job, 0, 0, [0, 0, 0, 0])
    check_timing(timed, timed["group"], "F", 1, True, exp, terminal["clock_tick_ns"])
    check_terminal(terminal, 4699, 1325812)
    r = decode(data["run/receipt.json"])
    calls = {
        "duplicate_JSON": lambda: decode('{"a":1,"a":2}'),
        "nonfinite_JSON": lambda: decode('{"a":NaN}'),
        "traversal": lambda: safe_path("../outside"),
        "tampered_stream": lambda: checked_bytes(b"x", {"bytes":1,"sha256":sha(b"y")}),
        "pending_receipt": lambda: closed({**r, "status":"pending"}, "measure"),
        "source_drift": lambda: closed({**r, "source_after":{}}, "measure"),
        "exact_claim": lambda: check_header({**header, "public_status":"exact"}),
        "NoObserver_absent": lambda: check_header({**header, "NoObserver":False}),
        "bool_integer": lambda: check_case({**first, "legacy_delta":False}, job, 0, 0, [0,0,0,0]),
        "legacy_overcharge": lambda: check_case({**first, "legacy_delta":1}, job, 0, 0, [0,0,0,0]),
        "proposal_overcharge": lambda: check_case({**first, "proposal_delta":1}, job, 0, 0, [0,0,0,0]),
        "Work_capture_corruption": lambda: check_case({**first, "work_hash":0}, job, 0, 0, [0,0,0,0]),
        "timed_capture_corruption": lambda: check_timing({**timed, "terminal_hash":0}, timed["group"], "F", 1, True, exp, 30),
        "paired_order_changed": lambda: check_timing({**timed, "pass":2}, timed["group"], "F", 1, True, exp, 30),
        "short_batch_hidden": lambda: check_timing({**timed, "short_batch":False}, timed["group"], "F", 1, True, exp, 30),
        "lost_helper_entry": lambda: check_terminal({**terminal, "helper_entries":1325811}, 4699, 1325812),
        "qualification_incomplete": lambda: check_terminal({**terminal, "full_terminals_equal_before_after":False}, 4699, 1325812),
    }
    result = decode(data["run/measurement.result.json"])
    attempt, spawn = decode(data["run/measurement.attempt.json"]), decode(data["run/measurement.spawned.json"])
    calls["wrong_exit_code"] = lambda: command({**result, "returncode":1}, attempt, spawn, data["run/measurement.stdout"], b"", result["argv"], 6, 120)
    calls["stream_seal_mismatch"] = lambda: command({**result, "stdout_sha256":"0" * 64}, attempt, spawn, data["run/measurement.stdout"], b"", result["argv"], 6, 120)
    for name, action in calls.items():
        try:
            action()
        except ValueError:
            continue
        raise ValueError("reader rejection survived: " + name)
    return dict(status="passed", positives=4, rejections=sorted(calls), engine_runs=0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--live", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        print(json.dumps(self_test() if args.self_test else inspect(args.live), indent=2, sort_keys=True))
    except (OSError, ValueError, KeyError, TypeError, IndexError) as error:
        print(json.dumps(dict(status="rejected", error=str(error))))
        sys.exit(1)
