#!/usr/bin/env python3
"""Measure only initial singleton-root policies, with full q2 output paid."""
from __future__ import annotations

import argparse
import copy
import importlib.util
import json
import os
from pathlib import Path
import platform
import signal
import subprocess
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
PARENT = BASE.parent / "q2_pool_bridge_20260914"
SPEC = importlib.util.spec_from_file_location("parent_measure", PARENT / "measure.py")
parent = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(parent)
require, sha, stamp, write = parent.require, parent.sha, parent.stamp, parent.write
require(sha(PARENT / "measure.py") ==
        "3e97507f7f42ec3161f7175c05add82ea5036956860f82e0ec0f50b2019a98e8",
        "changed explicit parent validator dependency")
POLICIES = ("complement", "global-iter", "global-pair")


def matrix(dataset, sizes, separations=(8,), reverse=False):
    policies = tuple(reversed(POLICIES)) if reverse else POLICIES
    return [(dataset, n, s, policy) for n in sizes for s in separations for policy in policies]


PLANS = {
    "pilot": sum((matrix(d, (8000,)) for d in
                  ("single_000000", "single_000100", "single_000200")), []),
    "check50k": matrix("single_000000", (50000,)),
    "repeat50k": matrix("single_000000", (50000,), reverse=True),
    "growth": matrix("single_000000", (16000, 32000)),
    "separation": matrix("single_000000", (8000,), (10, 12)),
    "clusters": matrix("clusters", (8000,)),
}


def command_for(binary, key):
    dataset, n, s, policy = key
    path = parent.input_path(dataset, n)
    return [str(binary), str(path) if path else "clusters", str(n), "10", str(s),
            "64", "pool-pair", "samples", policy]


def check(row):
    json.dumps(row, allow_nan=False)
    require(row["status"] == "completed", "incomplete measure")
    result = row["result"]
    require(json.loads(row["stdout"]) == result, "raw/result mismatch")
    require(result["schema"] == "mhgp8_audit_small_roots_v1" and
            result["root_policy"] == row["key"][3] and
            result["root_policy"] in POLICIES, "wrong root policy")
    require(type(result["small_roots"]) is int and result["small_roots"] > 0 and
            result["small_roots"] == result["front_work"]["leaf_pair_rectangles"],
            "initial singleton roots not exercised")
    # Reuse the pinned parent's full mass, payload, counter and timing guards.
    # Raw/result correspondence above is checked BEFORE this schema adaptation.
    adapted = copy.deepcopy(row)
    adapted["key"][3] = "pool-pair"
    adapted["result"]["schema"] = "mhgp8_audit_pool_bridge_v1"
    adapted["stdout"] = json.dumps(adapted["result"], allow_nan=False)
    parent.check(adapted)


def discrete(result):
    return {key: value for key, value in result.items()
            if not key.endswith("_ms") and key not in ("root_policy", "schema", "small_roots")}


def check_pairing(rows):
    groups = {}
    for row in rows:
        key = tuple(row["key"][:3])
        groups.setdefault(key, {})[row["key"][3]] = row["result"]
    for policies in groups.values():
        require(set(policies) == set(POLICIES), "missing paired policy")
        a, b, c = (policies[p] for p in POLICIES)
        for result in (b, c):
            for field in ("input_fnv64", "front_work", "pool_work", "sibling_work", "digest",
                          "candidate_pairs", "accepted_pairs", "rejected_pairs", "small_roots",
                          "input_rectangles", "anchor_queries"):
                require(result[field] == a[field], "paired work/output changed: " + field)
            payload = {k: v for k, v in result["census_work"].items() if k.startswith("payload_")}
            require(payload == {k: v for k, v in a["census_work"].items() if k.startswith("payload_")},
                    "payload traversal changed")
        # The two Global paths differ only in cursor mechanics, not geometry.
        for field, value in b["census_work"].items():
            if field not in ("cursor_advances", "cursor_reuses"):
                require(value == c["census_work"][field], "global discrete work changed: " + field)
        require(b["order_work"] == c["order_work"], "global structural work differs")
        require(b["census_work"]["cursor_advances"] - c["census_work"]["cursor_advances"] >= a["small_roots"],
                "iterative/recursive policies did not differ")
    return len(groups)


def validate(directory):
    manifest = json.loads((directory / "MANIFEST.json").read_text())
    done = json.loads((directory / "COMPLETION.json").read_text())
    require(done["status"] == "completed" and
            done["manifest_sha256"] == sha(directory / "MANIFEST.json") and
            done["measures_sha256"] == sha(directory / "MEASURES.jsonl"), "campaign not closed")
    require(manifest["runner_sha256"] == sha(Path(__file__)), "changed runner")
    for path, pin in manifest["pins"].items():
        require(sha(ROOT / path) == pin, "changed input: " + path)
    binary, _ = parent.build_paths(ROOT / manifest["build_receipt"])
    expected = [list(key) for key in PLANS[manifest["plan"]]]
    rows = [json.loads(line) for line in (directory / "MEASURES.jsonl").read_text().splitlines()]
    require(manifest["matrix"] == expected and [row["key"] for row in rows] == expected,
            "matrix incomplete or reordered")
    for row in rows:
        check(row)
        require(row["command"] == command_for(binary, row["key"]), "command/result mismatch")
        path = parent.input_path(*row["key"][:2])
        if path:
            require(row["input_sha256"] == manifest["pins"][str(path.relative_to(ROOT))] and
                    row["input_fnv64"] == parent.fnv(path), "input provenance differs")
    return dict(status="passed", plan=manifest["plan"], rows=len(rows),
                paired_inputs=check_pairing(rows))


def invoke(command, cpu):
    # Delay SIGINT/SIGTERM until communicate has saved stdout, within the
    # declared timeout. Never turn an interrupted child into a valid measure.
    received = []
    handlers = {sig: signal.getsignal(sig) for sig in (signal.SIGINT, signal.SIGTERM)}
    for sig in handlers:
        signal.signal(sig, lambda number, frame: received.append(number))
    try:
        with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              text=True, preexec_fn=lambda: os.sched_setaffinity(0, {cpu})) as process:
            timed_out = False
            try:
                stdout, stderr = process.communicate(timeout=180)
            except subprocess.TimeoutExpired:
                timed_out = True
                process.kill()
                stdout, stderr = process.communicate()
            return dict(returncode=process.returncode, stdout=stdout, stderr=stderr,
                        timed_out=timed_out, deferred_signals=received)
    finally:
        for sig, handler in handlers.items():
            signal.signal(sig, handler)


def measure(plan, receipt):
    directory = BASE / ("campaign_" + plan)
    require(not directory.exists(), "refuse overwrite")
    binary, paths = parent.build_paths(receipt)
    require(json.loads(receipt.read_text())["schema"] == "mhgp8_audit_small_roots_build_v1", "wrong build schema")
    paths.update((PARENT / "measure.py", Path(__file__)))
    paths.update(parent.input_path(d, n) for d, n, _, _ in PLANS[plan] if d != "clusters")
    pins = {str(path.relative_to(ROOT)): sha(path) for path in sorted(paths)}
    cpus = sorted(os.sched_getaffinity(0))
    cpu = cpus[-1]
    manifest = dict(schema="mhgp8_audit_small_roots_campaign_v1", plan=plan, matrix=PLANS[plan],
                    build_receipt=str(receipt.relative_to(ROOT)), pins=pins,
                    runner_sha256=sha(Path(__file__)), started_utc=stamp(), command=sys.argv,
                    allowed_cpus=cpus, selected_cpu=cpu, timeout_seconds=180,
                    timeout_policy="kill_drain_preserve_failure", signal_policy="defer_until_capture_then_fail",
                    platform=platform.platform(), python=sys.version, gcp_used=False,
                    public_status="not_claimed", warmups=0, shared_host=True)
    directory.mkdir()
    write(directory / "MANIFEST.json", manifest)
    status = "failed"
    rows = []
    try:
        with (directory / "MEASURES.jsonl").open("x") as stream:
            for key in PLANS[plan]:
                row = dict(key=list(key), command=command_for(binary, key), status="failed", started_utc=stamp())
                try:
                    path = parent.input_path(*key[:2])
                    if path:
                        row.update(input_sha256=sha(path), input_fnv64=parent.fnv(path))
                    row["loadavg_before"] = os.getloadavg()
                    row.update(invoke(row["command"], cpu))
                    require(not row["timed_out"] and not row["deferred_signals"], "interrupted invocation")
                    require(row["returncode"] == 0 and not row["stderr"], "failed invocation")
                    row["result"] = json.loads(row["stdout"])
                    row["status"] = "completed"
                    check(row)
                except BaseException as error:
                    row.update(status="failed", error_type=type(error).__name__, error=str(error))
                    raise
                finally:
                    row["finished_utc"] = stamp()
                    stream.write(json.dumps(row, allow_nan=False) + "\n")
                    stream.flush()
                rows.append(row)
                result = row["result"]
                print(json.dumps(dict(key=key, total_ms=result["total_ms"],
                                      count_nodes=result["census_work"]["count_node_visits"],
                                      structural_steps=sum(result["order_work"].values()))), flush=True)
        check_pairing(rows)
        require(all(sha(ROOT / path) == pin for path, pin in pins.items()), "inputs changed during campaign")
        status = "completed"
    finally:
        write(directory / "COMPLETION.json", dict(status=status, finished_utc=stamp(),
              manifest_sha256=sha(directory / "MANIFEST.json"),
              measures_sha256=sha(directory / "MEASURES.jsonl")))
    print(json.dumps(validate(directory)), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plan", choices=PLANS)
    parser.add_argument("--validate", type=Path)
    args = parser.parse_args()
    require((args.plan is None) != (args.validate is None), "choose a plan or validation")
    if args.validate:
        print(json.dumps(validate(args.validate.resolve()), sort_keys=True))
    else:
        measure(args.plan, BASE / "r1_BUILD.json")
