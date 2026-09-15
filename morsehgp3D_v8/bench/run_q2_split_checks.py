#!/usr/bin/env python3
"""Capture/check detached census tests and selected-anchor measurements.

Explicit runner port from run_q2_resume_checks.py (tranche15); own v1 schema.
The shared invoke collector is reused, never historical qualifications.

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
from run_q2_resume_checks import FIELDS as RESUME_FIELDS

FIELDS = {
    "geometry": (RESUME_FIELDS["census_work"] + " " +
                 " ".join("sibling_" + k for k in RESUME_FIELDS["sibling_work"].split()) + " " +
                 " ".join("order_" + k for k in RESUME_FIELDS["order_work"].split())),
    "resume": RESUME_FIELDS["resume_work"],
    "detach": "attempts detached_frames imported_frames transferred_pairs moved_frames",
    "schedule": "offer_checks offer_busy offer_full offer_no_sibling donations fragments_started fragments_completed waits wakes max_queue_size max_active_fragments",
    "timings_ms": "prepare front reference parallel_enclosing advance_sum payload_sum",
}
TOP_FIELDS = "schema scope public_status work_equal payload_equal n family kmax separation_s seed quantum workers queue anchor_rank a_node b_node input_hash front_products front_rectangles candidates accepted rejected hash_sum hash_xor geometry resume detach schedule queue_storage_bytes max_fragment_bytes timings_ms worker_transitions".split()


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
    n, family, k, separation, seed, quantum, workers, queue = args
    require(type(row) is dict and set(row) == set(TOP_FIELDS), "row fields mismatch")
    for name, fields in FIELDS.items():
        require(type(row[name]) is dict and set(row[name]) == set(fields.split()), "work fields mismatch")
    scalar = set(TOP_FIELDS) - set(FIELDS) - {
        "schema", "scope", "public_status", "work_equal", "payload_equal", "family", "worker_transitions"}
    for name in scalar:
        require(type(row[name]) is int and 0 <= row[name] < 2**64, "invalid integer metadata")
    require(row["schema"] == "mhgp8_q2_selected_anchor_split_v1" and
            row["scope"] == "one_anchor_not_full" and row["public_status"] == "not_claimed",
            "split scope mismatch")
    require(row["work_equal"] is True and row["payload_equal"] is True, "differential failed")
    require([row[key] for key in ("n", "family", "kmax", "separation_s", "seed", "quantum", "workers", "queue")] ==
            [int(n), family, int(k), int(separation), int(seed), int(quantum), int(workers), int(queue)],
            "command/row mismatch")
    require(row["n"] >= 2 and family in FAMILIES and 1 <= row["kmax"] <= 10 and
            all(row[key] > 0 for key in ("separation_s", "quantum", "workers", "queue")) and
            row["anchor_rank"] < row["n"] and row["candidates"] > 0 and
            row["candidates"] == row["accepted"] + row["rejected"], "invalid domain/mass")
    for name in ("geometry", "resume", "detach", "schedule"):
        require(all(type(v) is int and 0 <= v < 2**64 for v in row[name].values()), "invalid counter")
    g, r, d, c = (row[name] for name in ("geometry", "resume", "detach", "schedule"))
    require(all(g[key] == 0 for key in ("query_build_point_visits", "query_build_nodes",
            "query_build_max_depth", "query_cover_visits")) and
            g["count_node_visits"] == g["count_bound_tests"] + g["count_point_tests"] and
            g["payload_node_visits"] == g["payload_bound_tests"] + g["payload_point_tests"],
            "unexpected query build or geometric test partition")
    require(g["input_descriptors"] == g["count_root_starts"] == 1 and
            g["frontier_restarts"] == 0 and
            g["query_tasks"] == 1 + 2*g["query_splits"] == r["entry_steps"] and
            g["cursor_reuses"] == g["sibling_proposals"] == 2*g["query_splits"] and
            r["witness_steps"] == g["count_node_visits"] + sum(g[key] for key in g if key.startswith("order_")) and
            r["payload_steps"] == g["payload_supports"] == row["accepted"] and
            r["admission_steps"] <= r["payload_steps"] and
            r["transitions"] == sum(r[key] for key in ("entry_steps", "witness_steps", "admission_steps", "payload_steps")),
            "geometric/transition ledger mismatch")
    require(d["detached_frames"] == d["imported_frames"] == c["donations"] and
            c["fragments_started"] == c["fragments_completed"] == 1 + c["donations"] and
            r["advance_calls"] == r["pauses"] + c["fragments_completed"] and
            r["advance_calls"] >= (r["transitions"] + row["quantum"] - 1)//row["quantum"] and
            d["attempts"] == c["donations"] + c["offer_no_sibling"] and
            c["offer_checks"] == c["offer_busy"] + c["offer_full"] + d["attempts"] and
            c["waits"] == c["wakes"] and
            1 <= c["max_queue_size"] <= row["queue"] and
            1 <= c["max_active_fragments"] <= row["workers"],
            "schedule obligation ledger mismatch")
    require(all(r[key] <= r["pauses"] for key in
                ("pauses_after_credit", "pauses_inside_deferred", "pauses_during_emission")) and
            1 <= r["max_pending_tasks"] <= 49 and
            row["queue_storage_bytes"] >= row["queue"]*8 and row["max_fragment_bytes"] >= 6272,
            "state memory mismatch")
    if row["workers"] == 1:
        require(c["offer_checks"] == c["donations"] == d["attempts"] == 0, "serial scheduler donated")
    require(type(row["worker_transitions"]) is list and len(row["worker_transitions"]) == row["workers"] and
            all(type(v) is int and 0 <= v < 2**64 for v in row["worker_transitions"]) and
            sum(row["worker_transitions"]) == r["transitions"], "worker work lost")
    require(all(type(v) in (int, float) and math.isfinite(v) and v >= 0
                for v in row["timings_ms"].values()), "invalid timing")
    return row


def validate_ctest(path):
    suite = ET.fromstring(path.read_bytes())
    cases = list(suite.iter("testcase"))
    require(suite.tag == "testsuite" and len(cases) == len({c.attrib["name"] for c in cases}) == 66 and
            int(suite.attrib["tests"]) == 66 and int(suite.attrib["failures"]) == 0 and
            int(suite.attrib.get("errors", 0)) == int(suite.attrib.get("skipped", 0)) == 0 and
            all(c.attrib["name"].startswith("mhgp8_") and c.find("failure") is None and
                c.find("error") is None and c.find("skipped") is None for c in cases),
            "expected 66 distinct passing CTests")


def validate_pairs(rows):
    keys = ("n", "family", "kmax", "separation_s", "workers")
    configs = [tuple(row[k] for k in keys) for row in rows]
    expected = set(itertools.product((8000, 16000, 32000), FAMILIES, (5, 10), (8, 10, 12), (1, 4)))
    require(len(configs) == len(set(configs)) and set(configs) == expected, "matrix coverage mismatch")
    for one, many in zip(rows[::2], rows[1::2], strict=True):
        for key in ("input_hash", "a_node", "b_node", "anchor_rank", "candidates", "accepted", "rejected",
                    "hash_sum", "hash_xor", "geometry"):
            require(one[key] == many[key], "workers changed root/work/output")
        for key in ("transitions", "entry_steps", "witness_steps", "admission_steps", "payload_steps"):
            require(one["resume"][key] == many["resume"][key], "workers replayed transitions")


def plan(build, output, matrix):
    if matrix:
        return [("measure", [str(build / "mhgp8_q2_census_split_probe"), str(n), family,
                 str(k), str(s), "3", "256", str(w), "8"]) for n, family, k, s, w in
                itertools.product((8000, 16000, 32000), FAMILIES, (5, 10), (8, 10, 12), (1, 4))]
    return [("ctest", ["ctest", "--test-dir", str(build), "--output-on-failure",
                       "--output-junit", str(output / "ctest.xml")]),
            ("detach", [str(build / "mhgp8_q2_census_detach_gate"), "--selftest"]),
            ("parallel", [str(build / "mhgp8_q2_census_parallel_gate"), "--selftest"])]


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
    manifest = dict(schema="mhgp8_q2_split_attempt_v1", started_utc=utc_stamp(), build=str(build),
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
    require(manifest["schema"] == "mhgp8_q2_split_attempt_v1" and completion["status"] == "passed" and
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
