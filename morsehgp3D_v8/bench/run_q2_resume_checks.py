#!/usr/bin/env python3
"""Capture/check resumable census tests and selected-anchor measurements.

Builds must already exist. This runner does not provision GCP, rebuild pinned
binaries, or reinterpret one selected anchor as a whole-front/tower timing.
The interruption-safe subprocess collector is reused explicitly from the v8
runner; each failed attempt and its partial output is retained in a fresh dir.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import itertools
import json
import math
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from run_p0_matrix import invoke, on_signal, utc_stamp

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "morsehgp3D_v8"
FAMILIES = ("uniform", "terrain", "clusters", "rows")
FIELDS = {
    "census_work": "query_build_point_visits query_build_nodes query_build_max_depth input_descriptors query_cover_visits query_tasks query_splits witness_splits count_root_starts shared_splits_after_credit cursor_advances cursor_reuses count_node_visits count_bound_tests count_point_tests uniform_credited_pairs uniform_rejected_pairs uniform_accepted_pairs consumed_witness_sites frontier_restarts payload_node_visits payload_bound_tests payload_point_tests payload_interior_sites payload_shell_sites payload_supports",
    "sibling_work": "proposals cardinality_skips bound_tests rejected_tasks rejected_pairs rejected_after_credit",
    "order_work": "structural_splits deferred_skips anchor_skips phase_switches",
    "resume_work": "advance_calls transitions entry_steps witness_steps admission_steps payload_steps pauses pauses_after_credit pauses_inside_deferred pauses_during_emission max_pending_tasks",
    "memory": "stack_capacity stack_bytes payload_bytes retained_bytes",
    "timing_ms": "generation preparation front_selection reference creation resume_active resume_enclosing max_slice probe",
}
TOP_FIELDS = "schema public_status scope full_contract_qualified n family kmax separation_s seed quantum input_hash a_node b_node anchor_rank anchor_id a_size b_size front_products front_rectangles front_residual_pairs candidate_pairs accepted_pairs rejected_pairs digest work_equal ordered_payload_equal census_work sibling_work order_work resume_work memory timing_ms".split()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, value):
    with path.open("x", encoding="utf-8") as stream:
        json.dump(value, stream, indent=2, allow_nan=False)
        stream.write("\n")


def source_pins():
    paths = [SOURCE / "CMakeLists.txt"]
    for name in ("src", "bench", "tests", "oracle"):
        paths.extend(p for p in (SOURCE / name).rglob("*") if p.suffix in (".cpp", ".hpp", ".py"))
    return {str(p.relative_to(ROOT)): digest(p) for p in sorted(paths)}


def validate_row(row, args):
    n, family, k, separation, seed, quantum = args
    require(type(row) is dict and set(row) == set(TOP_FIELDS), "missing or extra row fields")
    for name, fields in FIELDS.items():
        require(type(row[name]) is dict and set(row[name]) == set(fields.split()), "missing or extra work/time fields")
    for name in set(TOP_FIELDS) - set(FIELDS) - {"schema", "public_status", "scope", "family", "full_contract_qualified", "work_equal", "ordered_payload_equal"}:
        require(type(row[name]) is int and 0 <= row[name] <= 2**64 - 1, "invalid integer metadata")
    require(row["schema"] == "mhgp8_q2_selected_anchor_resume_v1" and
            row["scope"] == "one_selected_anchor_not_whole_front_or_full" and
            row["public_status"] == "not_claimed" and row["full_contract_qualified"] is False,
            "resume scope mismatch")
    require([row[key] for key in ("n", "family", "kmax", "separation_s", "seed", "quantum")] ==
            [int(n), family, int(k), int(separation), int(seed), int(quantum)], "command/row mismatch")
    require(row["n"] >= 2 and family in FAMILIES and 1 <= row["kmax"] <= 10 and
            row["separation_s"] > 0 and row["quantum"] > 0 and
            row["anchor_id"] < row["n"] and row["anchor_rank"] < row["n"], "invalid root domain")
    require(row["work_equal"] is True and row["ordered_payload_equal"] is True, "differential failed")
    require(1 <= row["a_size"] <= row["b_size"] < row["n"] and
            row["candidate_pairs"] == row["b_size"] == row["accepted_pairs"] + row["rejected_pairs"],
            "root mass mismatch")
    w, r, memory = row["census_work"], row["resume_work"], row["memory"]
    for values in (w, r, memory, row["sibling_work"], row["order_work"]):
        require(all(type(v) is int and 0 <= v <= 2**64 - 1 for v in values.values()), "invalid work field")
    require(w["input_descriptors"] == w["count_root_starts"] == 1 and w["frontier_restarts"] == 0 and
            w["payload_supports"] == row["accepted_pairs"] == r["payload_steps"] and
            w["query_tasks"] == r["entry_steps"] and
            r["witness_steps"] == w["count_node_visits"] + sum(row["order_work"].values()) and
            r["admission_steps"] <= r["payload_steps"] and
            r["transitions"] == r["entry_steps"] + r["witness_steps"] + r["admission_steps"] + r["payload_steps"] and
            r["advance_calls"] == r["pauses"] + 1 and
            r["advance_calls"] == (r["transitions"] + row["quantum"] - 1) // row["quantum"],
            "resume transition ledger mismatch")
    require(all(r[key] <= r["pauses"] for key in
                ("pauses_after_credit", "pauses_inside_deferred", "pauses_during_emission")) and
            r["max_pending_tasks"] <= memory["stack_capacity"] and
            memory["retained_bytes"] == memory["stack_bytes"] + memory["payload_bytes"], "state memory mismatch")
    require(all(type(v) in (int, float) and math.isfinite(v) and v >= 0
                for v in row["timing_ms"].values()), "invalid timing")
    return row


def validate_ctest(path):
    suite = ET.fromstring(path.read_bytes())
    cases = list(suite.iter("testcase"))
    require(suite.tag == "testsuite" and len(cases) == len({c.attrib["name"] for c in cases}) == 62 and
            int(suite.attrib["tests"]) == 62 and int(suite.attrib["failures"]) == 0 and
            int(suite.attrib.get("errors", 0)) == int(suite.attrib.get("skipped", 0)) == 0 and
            all(c.attrib["name"].startswith("mhgp8_") and c.find("failure") is None and
                c.find("error") is None and c.find("skipped") is None for c in cases),
            "expected 62 distinct passing CTests")


def validate_pairs(rows):
    configs = [tuple(row[k] for k in ("n", "family", "kmax", "separation_s", "quantum")) for row in rows]
    expected = set(itertools.product((8000, 16000, 32000), FAMILIES, (5, 10), (8, 10, 12), (1, 256)))
    require(len(configs) == len(set(configs)) and set(configs) == expected, "matrix coverage mismatch")
    for one, many in zip(rows[::2], rows[1::2], strict=True):
        for key in ("input_hash", "a_node", "b_node", "anchor_rank", "anchor_id", "a_size", "b_size",
                    "candidate_pairs", "accepted_pairs", "rejected_pairs", "digest", "census_work", "sibling_work", "order_work"):
            require(one[key] == many[key], "quantum changed selected root/work/output")


def plan(build, output, matrix):
    if matrix:
        return [("measure", [str(build / "mhgp8_q2_census_resume_probe"), str(n), family,
                 str(k), str(s), "3", str(q)]) for n, family, k, s, q in
                itertools.product((8000, 16000, 32000), FAMILIES, (5, 10), (8, 10, 12), (1, 256))]
    return [("ctest", ["ctest", "--test-dir", str(build), "--output-on-failure",
                        "--output-junit", str(output / "ctest.xml")]),
            ("gate", [str(build / "mhgp8_q2_census_resume_gate"), "--selftest"]),
            ("q3q4", [sys.executable, "-B", str(SOURCE / "tests/q3_q4_owner_independence_gate.py"), "--selftest"])]


def run(args):
    build = args.build.resolve()
    require(build.is_relative_to(ROOT) and (build / "CMakeCache.txt").is_file(), "build must exist inside repository")
    args.output.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix="matrix_" if args.matrix else "qualification_", dir=args.output.resolve()))
    environment = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    sources = source_pins()
    artifacts = {str(p.relative_to(ROOT)): digest(p) for p in sorted(build.glob("mhgp8_*"))
                 if p.is_file() and os.access(p, os.X_OK)}
    artifacts[str((build / "CMakeCache.txt").relative_to(ROOT))] = digest(build / "CMakeCache.txt")
    commands = plan(build, output, args.matrix)
    manifest = dict(schema="mhgp8_q2_resume_attempt_v1", started_utc=utc_stamp(), build=str(build),
                    matrix=args.matrix, command=[sys.executable, *sys.argv],
                    source_sha256=sources, artifact_sha256=artifacts,
                    planned_commands=commands, affinity=sorted(os.sched_getaffinity(0)),
                    cmake_cache=(build / "CMakeCache.txt").read_text(),
                    commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                    worktree_status=subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True),
                    environment={key: environment.get(key) for key in
                                 ("ASAN_OPTIONS", "UBSAN_OPTIONS", "TSAN_OPTIONS", "OMP_NUM_THREADS")},
                    public_status="not_claimed", gcp_used=False, full_contract_qualified=False)
    write_json(output / "MANIFEST.json", manifest)
    records = []
    rows = []
    status, error = "failed", "not started"
    try:
        for number, (kind, command) in enumerate(commands):
            print(json.dumps({"capture": str(output), "number": number, "command": command}), flush=True)
            record = dict(kind=kind, command=command, cwd=str(ROOT), started_utc=utc_stamp(),
                          status="failed", exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
            try:
                invoke(command, environment, ROOT, record, new_session=True)
                require(record["exit_code"] == 0, "command failed")
                if kind == "measure":
                    record["row"] = validate_row(json.loads(record["stdout"]), command[1:])
                    rows.append(record["row"])
                if kind == "ctest":
                    validate_ctest(output / "ctest.xml")
                record["status"] = "passed"
            finally:
                record["finished_utc"] = utc_stamp()
                path = output / f"record_{number:04}.json"
                write_json(path, record)
                records.append({"path": path.name, "sha256": digest(path)})
        if args.matrix:
            validate_pairs(rows)
        require(source_pins() == sources and all(digest(ROOT / path) == value for path, value in artifacts.items()),
                "sources or binaries changed during capture")
        status, error = "passed", None
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        extra = {}
        if (output / "ctest.xml").is_file():
            extra["ctest.xml"] = digest(output / "ctest.xml")
        write_json(output / "COMPLETION.json", dict(status=status, error=error, finished_utc=utc_stamp(),
            manifest_sha256=digest(output / "MANIFEST.json"), records=records, extra_sha256=extra,
            source_sha256_after=source_pins(),
            artifact_sha256_after={p: digest(ROOT / p) for p in artifacts}))
        for sig, handler in previous.items():
            signal.signal(sig, handler)
    read(output)
    return 0


def read(path):
    manifest = json.loads((path / "MANIFEST.json").read_text())
    completion = json.loads((path / "COMPLETION.json").read_text())
    require(manifest["schema"] == "mhgp8_q2_resume_attempt_v1" and completion["status"] == "passed" and
            completion["manifest_sha256"] == digest(path / "MANIFEST.json"), "capture incomplete or corrupt")
    require(manifest["public_status"] == "not_claimed" and manifest["gcp_used"] is False and
            manifest["full_contract_qualified"] is False, "capture changed scope")
    require(manifest["source_sha256"] == completion["source_sha256_after"] and
            manifest["artifact_sha256"] == completion["artifact_sha256_after"], "closure hash mismatch")
    commands = manifest["planned_commands"]
    expected_commands = plan(Path(manifest["build"]), path.resolve(), manifest["matrix"])
    require(commands == [[kind, command] for kind, command in expected_commands], "unrecognized qualification commands")
    require(len(commands) == len(completion["records"]) == (144 if manifest["matrix"] else 3), "missing records")
    rows = []
    for number, ((kind, command), info) in enumerate(zip(commands, completion["records"], strict=True)):
        require(info["path"] == f"record_{number:04}.json", "invalid or repeated record path")
        record_path = path / info["path"]
        require(digest(record_path) == info["sha256"], "record hash mismatch")
        record = json.loads(record_path.read_text())
        require(record["kind"] == kind and record["command"] == command and
                record["status"] == "passed" and record["exit_code"] == 0, "command/result mismatch")
        for channel in ("stdout", "stderr"):
            require(base64.b64decode(record[channel + "_base64"], validate=True).decode("utf-8", errors="replace") ==
                    record[channel], "raw/decoded log mismatch")
        if kind == "measure":
            row = validate_row(json.loads(record["stdout"]), command[1:])
            require(row == record["row"], "parsed row changed")
            rows.append(row)
    if manifest["matrix"]:
        validate_pairs(rows)
    else:
        require(completion["extra_sha256"].get("ctest.xml") == digest(path / "ctest.xml"), "CTest hash mismatch")
        validate_ctest(path / "ctest.xml")
    print(json.dumps(dict(status="passed", path=str(path), records=len(commands),
                          selected_anchor_measures=len(rows), full_contract_qualified=False), sort_keys=True))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--build", type=Path, required=True)
    capture.add_argument("--output", type=Path, required=True)
    capture.add_argument("--matrix", action="store_true")
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    args = parser.parse_args()
    if args.operation == "run":
        return run(args)
    read(args.path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
