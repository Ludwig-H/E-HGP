#!/usr/bin/env python3
"""Fresh captures of the widened front proposal window and strict, failure-preserving closed receipts.

Explicit collector/pin port from the batched-singleton runner at 8d615cfd.
Every configuration pairs the Coarse parallel probe of the SAME build (same
probe source as before the tranche, relinked with the tranche's front,
default proposals) with five runs of the proposals probe: factor 1 must
reproduce that reference on every discrete field; factors 2 and 4, with the
small-factor limit 16 or none, must keep the canonical supports and may only
remove census candidates. The separate `differential` campaign compares both
with the parallel probe of a PINNED pre-tranche build, outside this build:
only that campaign proves that the default reproduces the historical engine. The Coarse pipeline validator is imported as
a contract projection: the extension counters are subtracted from the front
work, which must then satisfy the historical invariants exactly.
"""
from __future__ import annotations

import argparse
import base64
from copy import deepcopy
import itertools
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import json

from run_p0_matrix import invoke, on_signal, utc_stamp, require, uint
from run_q2_split_checks import source_pins, digest, write_json
from proposals_source_paths import SOURCE_PATHS, ARTIFACT_NAMES
from run_wspd_q2_parallel_matrix import (
    POOL_FIELDS, DISCRETE_FIELDS, MEMORY_FIELDS, validate_result as validate_reference, FIXED, cross_check, counters)
from run_wspd_q2_cooperative_checks import strict_json

ROOT = Path(__file__).resolve().parents[2]
SCHEMA = "mhgp8_q2_proposals_attempt_v1"
FAMILIES = ("uniform", "terrain", "clusters", "rows")
# (window_factor, small_factor_limit): the historical window first, then the two
# policies of the note for 2K and 4K. "all" is the unlimited default of the API.
VARIANTS = (("1", "all"), ("2", "16"), ("2", "all"), ("4", "16"), ("4", "all"))
EXTENSION_FIELDS = ("extended_products", "extended_proposals", "extended_proposals_in_factors",
                    "extended_credits", "extended_rejections")
PINNED_PROBE = "mhgp8_wspd_q2_parallel_probe"
CTESTS = 78


def artifact_names(campaign):
    if campaign == "tsan":
        return {"mhgp8_wspd_q2_proposals_gate", "mhgp8_wspd_front_dispatch_gate"}
    if campaign in ("smoke", "differential"):
        return {"mhgp8_wspd_q2_proposals_probe", "mhgp8_wspd_q2_parallel_probe"}
    return ARTIFACT_NAMES


def validate_pins(manifest):
    sources, artifacts = manifest["source_sha256"], manifest["artifact_sha256"]
    require(type(sources) is dict and set(sources) == SOURCE_PATHS, "source pin inventory mismatch")
    require(type(artifacts) is dict and bool(artifacts), "missing artifact pins")
    for pins in (sources, artifacts):
        require(all(type(value) is str and re.fullmatch(r"[0-9a-f]{64}", value)
                    for value in pins.values()), "malformed SHA256 pin")
    build = Path(manifest["build"])
    require(build.is_absolute() and build.is_relative_to(ROOT) and ".." not in build.parts,
            "build escaped repository")
    build_relative = build.relative_to(ROOT)
    expected = artifact_names(manifest["campaign"])
    require(set(artifacts) == {str(build_relative / name) for name in {*expected, "CMakeCache.txt"}},
            "artifact inventory does not close all tests/probes")
    require(str(build_relative / "CMakeCache.txt") in artifacts, "missing CMake cache pin")
    for name in artifacts:
        path = Path(name)
        require(path.parent == build_relative and (path.name == "CMakeCache.txt" or
                re.fullmatch(r"mhgp8_[a-z0-9_]+", path.name)), "artifact escaped build")
    pinned = manifest.get("pinned_artifact_sha256", {})
    require(type(pinned) is dict and all(type(v) is str and re.fullmatch(r"[0-9a-f]{64}", v) for v in pinned.values()) and
            (bool(pinned) == (manifest["campaign"] == "differential")), "pinned reference pins do not match the campaign")
    for kind, command in manifest["planned_commands"]:
        if kind == "pinned":
            executable = Path(command[0])
            require(executable.is_relative_to(ROOT) and not executable.is_relative_to(build) and
                    executable.name == PINNED_PROBE and str(executable.relative_to(ROOT)) in pinned,
                    "pinned reference executable is not pinned or lies inside the measured build")
        elif kind != "ctest":
            executable = Path(command[0])
            require(executable.is_relative_to(ROOT) and
                    str(executable.relative_to(ROOT)) in artifacts, "command executable is not pinned")



def validate_row(row, args):
    require(len(args) == 10, "proposals command arity")
    n, family, k, separation, seed, workers, jobs, pool, factor, limit = args
    expected = (int(n), family, int(k), int(separation), int(seed), int(workers), int(jobs), int(pool),
                int(factor), None if limit == "all" else int(limit))
    keys = ("n", "family", "kmax", "s", "seed", "threads", "jobs_per_worker", "pool_min_factor",
            "window_factor", "small_factor_limit")
    require(type(row) is dict and tuple(row.get(key) for key in keys) == expected,
            "proposals command/result mismatch")
    require(type(row["window_factor"]) is int and row["window_factor"] in (1, 2, 4) and
            (row["small_factor_limit"] is None or
             (type(row["small_factor_limit"]) is int and row["small_factor_limit"] > 0)),
            "proposal options outside their declared domain")
    require(row.get("schema") == "mhgp8_wspd_q2_proposals_probe_v1", "proposals schema mismatch")
    extension = row.get("extension_work")
    counters(extension, EXTENSION_FIELDS, "extension_work")
    # Explicit current field-contract projection, not a historical result: the
    # historical-window part of the filter work is the total minus the extension.
    projected = deepcopy(row)
    projected["schema"] = FIXED["schema"]
    for key in ("window_factor", "small_factor_limit", "extension_work"):
        del projected[key]
    front = projected["front_work"]
    # Arithmetic below would turn a boolean into an integer: judge the four
    # totals that include the extension BEFORE subtracting anything.
    for key in ("proposed_sites", "proposals_in_factors", "h_bound_tests", "witness_lane_credits"):
        uint(front[key], "front_work." + key)
    extra, skipped, credits, rejections = (extension[key] for key in EXTENSION_FIELDS[1:])
    require(skipped <= extra <= front["proposed_sites"] and skipped <= front["proposals_in_factors"] and
            extra - skipped <= front["h_bound_tests"] and credits <= front["witness_lane_credits"] and
            credits <= extra - skipped, "extension counters exceed the totals that include them")
    front["proposed_sites"] -= extra
    front["proposals_in_factors"] -= skipped
    front["h_bound_tests"] -= extra - skipped
    front["witness_lane_credits"] -= credits
    validate_reference(projected, ["projected-probe", n, family, k, separation, seed, workers, jobs, pool])
    # Theorems of the engine, judged on the declared ledger. An extended product ran its whole
    # historical window and then proposed at least one extra rank; a product rejected by the
    # extension received at least one extra credit; a searched product that was not extended
    # was rejected by its historical window or was not eligible (never, without a limit).
    work = row["front_work"]
    products = extension["extended_products"]
    historical, wider = min(int(k), int(n)), min(int(k) * int(factor), int(n))
    searches, killed = work["witness_searches"], work["fully_rejected_products"]
    require(products <= searches and products <= extra <= (wider - historical) * products and
            rejections <= credits and rejections <= products and rejections <= killed,
            "extension work outside its widened-window bound")
    require(work["proposed_sites"] - extra >= historical * products + (searches - products) and
            searches - products >= killed - rejections,
            "historical-window part of the proposal ledger is impossible")
    if int(factor) != 1 and limit == "all":
        require(searches - products == killed - rejections,
                "an unlimited widened window left a surviving product without extension")
    if int(factor) == 1:
        require(all(extension[key] == 0 for key in EXTENSION_FIELDS), "historical window reported extension work")
    return row


def validate_ctest(path):
    suite = ET.fromstring(path.read_bytes())
    cases = list(suite.iter("testcase"))
    require(suite.tag == "testsuite" and len(cases) == len({c.attrib["name"] for c in cases}) == CTESTS and
            int(suite.attrib["tests"]) == CTESTS and int(suite.attrib["failures"]) == 0 and
            int(suite.attrib.get("errors", 0)) == int(suite.attrib.get("skipped", 0)) == 0 and
            all(c.attrib["name"].startswith("mhgp8_") and c.find("failure") is None and
                c.find("error") is None and c.find("skipped") is None for c in cases),
            "expected %d distinct passing CTests" % CTESTS)


def is_reference(row):
    return row["schema"] == FIXED["schema"]


def historical_view(row):
    """The reference-shaped view of a factor-1 proposals row (no arithmetic: its extension is zero)."""
    view = deepcopy(row)
    view["schema"] = FIXED["schema"]
    for key in ("window_factor", "small_factor_limit", "extension_work"):
        del view[key]
    return view


def validate_pairs(rows, campaign):
    """Reference and factor-1 rows share the full historical signature; widened rows keep the
    input identity and the canonical supports, are identical across worker counts inside one
    capture, and only remove candidates, monotonically in the factor and in the limit. In the
    differential campaign the pinned pre-tranche probe, the reference and factor 1 are equal."""
    identities, signatures, widened, groups = {}, {}, {}, {}
    if campaign == "differential":
        require(len(rows) % 3 == 0 and bool(rows), "incomplete differential triples")
        for offset in range(0, len(rows), 3):
            pinned, reference, default = rows[offset:offset + 3]
            require(is_reference(pinned) and is_reference(reference) and not is_reference(default) and
                    default["window_factor"] == 1 and default["small_factor_limit"] is None,
                    "unexpected differential triple")
            for row in (pinned, reference, historical_view(default)):
                cross_check(row, identities, signatures)
        return
    for row in rows:
        identity = row["family"], row["n"], row["seed"] if row["seed_affects_input"] else None
        if is_reference(row) or row["window_factor"] == 1:
            cross_check(row if is_reference(row) else historical_view(row), identities, signatures)
        else:
            value = {name: row[name] for name in ("input_hash", "total_unordered_pairs", "generation_work",
                                                 "cloud_work", "index_work")}
            value["memory"] = {name: row["memory"][name] for name in MEMORY_FIELDS[:3]}
            require(identities.get(("input", *identity), value) == value,
                    "same fixture changed across proposal windows")
            identities.setdefault(("input", *identity), value)
            support_key = ("support", *identity, row["kmax"])
            require(identities.get(support_key, row["digest"]) == row["digest"],
                    "canonical complete supports changed with the proposal window")
            identities.setdefault(support_key, row["digest"])
            key = (*identity, row["kmax"], row["s"], row["pool_min_factor"],
                   row["window_factor"], row["small_factor_limit"])
            signature = {name: row[name] for name in DISCRETE_FIELDS}
            signature["pool_work"] = {name: row["pool_work"][name] for name in POOL_FIELDS}
            signature["extension_work"] = row["extension_work"]
            require(widened.get(key, signature) == signature,
                    "widened integer work changed across threads or repeat")
            widened.setdefault(key, signature)
        group = (*identity, row["kmax"], row["s"], row["pool_min_factor"], row["threads"])
        label = "reference" if is_reference(row) else (row["window_factor"], row["small_factor_limit"])
        require(label not in groups.setdefault(group, {}), "repeated configuration inside one capture")
        groups[group][label] = row
    active = 0
    for group, members in groups.items():
        require(set(members) == {"reference", (1, None), (2, 16), (2, None), (4, 16), (4, None)},
                "incomplete proposal comparison group")
        reference = members["reference"]
        # The engraved pure-overhead regime: two parallel rows have almost no universal witness.
        generic = reference["family"] != "rows" and campaign != "smoke"
        for label, row in members.items():
            if label == "reference":
                continue
            require(row["accepted_pairs"] == reference["accepted_pairs"],
                    "widened window changed the accepted supports")
            require(row["candidate_pairs"] <= reference["candidate_pairs"],
                    "widened window added census candidates")
            require(row["front_work"]["rejected_pair_mass"][0] >= reference["front_work"]["rejected_pair_mass"][0],
                    "widened window rejected less pair mass at the front")
            require(row["input_rectangles"] <= reference["input_rectangles"],
                    "widened window emitted more rectangles")
            rejecting = row["extension_work"]["extended_rejections"] > 0
            active += rejecting
            if generic and label[0] != 1:
                require(rejecting and row["candidate_pairs"] < reference["candidate_pairs"],
                        "widened window rejected nothing on a generic cloud at a size of interest")
        for factor in (2, 4):
            require(members[(factor, None)]["candidate_pairs"] <= members[(factor, 16)]["candidate_pairs"],
                    "unlimited window kept more candidates than its small-factor variant")
        for limit in (16, None):
            require(members[(4, limit)]["candidate_pairs"] <= members[(2, limit)]["candidate_pairs"],
                    "window 4K kept more candidates than window 2K")
    if rows:
        require(active > 0, "no widened window rejected any product: vacuous capture")


def plan(build, output, campaign, pinned_build=None):
    probe = str(build / "mhgp8_wspd_q2_proposals_probe")
    reference = str(build / "mhgp8_wspd_q2_parallel_probe")
    if campaign == "qualification":
        return [("ctest", ["ctest", "--test-dir", str(build), "--parallel", "2", "--output-on-failure",
                           "--output-junit", str(output / "ctest.xml")]),
                ("gate", [str(build / "mhgp8_wspd_q2_proposals_gate"), "--selftest"])]
    if campaign == "tsan":
        return [("gate", [str(build / "mhgp8_wspd_q2_proposals_gate"), "--selftest"]),
                ("gate", [str(build / "mhgp8_wspd_front_dispatch_gate"), "--selftest"])]
    if campaign == "differential":
        require(pinned_build is not None, "differential campaign requires a pinned pre-tranche build")
        commands = []
        for n, family in itertools.product((8000, 16000, 32000), FAMILIES):
            common = [str(n), family, "10", "8", "3", "1", "16", "64"]
            commands += [("pinned", [str(Path(pinned_build) / PINNED_PROBE), *common]),
                         ("reference", [reference, *common]), ("measure", [probe, *common, "1", "all"])]
        return commands
    if campaign == "scale":
        configurations = itertools.product((8000, 16000, 32000), FAMILIES, (5, 10), (8,), (1,))
    elif campaign == "parallel":
        configurations = itertools.product((32000,), FAMILIES, (10,), (8,), (1, 4))
    elif campaign == "separations":
        configurations = [v for v in itertools.product((8000,), FAMILIES, (5, 10), (8, 10, 12), (1,))
                          if (v[2], v[3]) != (10, 8)]
    elif campaign == "smoke":
        configurations = itertools.product((32,), FAMILIES, (5,), (8,), (1, 4))
    else:
        raise RuntimeError("unknown campaign")
    commands = []
    for n, family, k, s, workers in configurations:
        common = [str(n), family, str(k), str(s), "3", str(workers), "16", "64"]
        commands.append(("reference", [reference, *common]))
        for factor, limit in VARIANTS:
            commands.append(("measure", [probe, *common, factor, limit]))
    return commands


def run(args):
    build = args.build.resolve()
    require(build.is_relative_to(ROOT) and (build / "CMakeCache.txt").is_file(), "build must exist inside repository")
    args.output.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=args.campaign + "_", dir=args.output.resolve()))
    environment = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    sources = source_pins()
    selected_artifacts = [build / name for name in sorted(artifact_names(args.campaign))]
    require(all(p.is_file() and os.access(p, os.X_OK) for p in selected_artifacts), "missing campaign executable")
    artifacts = {str(p.relative_to(ROOT)): digest(p) for p in selected_artifacts}
    artifacts[str((build / "CMakeCache.txt").relative_to(ROOT))] = digest(build / "CMakeCache.txt")
    pinned_build, pinned_pins = None, {}
    if args.campaign == "differential":
        pinned_build = args.pinned_build.resolve()
        require(pinned_build.is_relative_to(ROOT) and pinned_build != build and
                (pinned_build / PINNED_PROBE).is_file(), "pinned pre-tranche build is missing")
        pinned_pins = {str((pinned_build / name).relative_to(ROOT)): digest(pinned_build / name)
                       for name in (PINNED_PROBE, "CMakeCache.txt")}
    commands = plan(build, output, args.campaign, pinned_build)
    manifest = dict(schema=SCHEMA, started_utc=utc_stamp(), build=str(build),
                    campaign=args.campaign, command=[sys.executable, *sys.argv],
                    source_sha256=sources, artifact_sha256=artifacts,
                    pinned_build=str(pinned_build) if pinned_build else None, pinned_artifact_sha256=pinned_pins,
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
        validate_pins(manifest)
        for number, (kind, command) in enumerate(commands):
            print(json.dumps({"capture": str(output), "number": number, "command": command}), flush=True)
            record = dict(kind=kind, command=command, cwd=str(ROOT), started_utc=utc_stamp(),
                          status="failed", exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
            try:
                invoke(command, environment, ROOT, record, new_session=True)
                require(record["exit_code"] == 0, "command failed")
                if kind in ("measure", "reference", "pinned"):
                    parsed = strict_json(record["stdout"])
                    if kind == "measure":
                        validate_row(parsed, command[1:])
                    else:
                        validate_reference(parsed, command)
                    record["row"] = parsed
                    rows.append(parsed)
                if kind == "gate":
                    parsed = strict_json(record["stdout"])
                    require(parsed.get("status") == "passed", "gate did not pass")
                if kind == "ctest":
                    validate_ctest(output / "ctest.xml")
                record["status"] = "passed"
            finally:
                record["finished_utc"] = utc_stamp()
                path = output / f"record_{number:04}.json"
                write_json(path, record)
                records.append({"path": path.name, "sha256": digest(path)})
        validate_pairs(rows, args.campaign)
        require(source_pins() == sources and
                all(digest(ROOT / path) == value for path, value in {**artifacts, **pinned_pins}.items()),
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
            artifact_sha256_after={p: digest(ROOT / p) for p in artifacts},
            pinned_artifact_sha256_after={p: digest(ROOT / p) for p in pinned_pins}))
        for sig, handler in previous.items():
            signal.signal(sig, handler)
    read(output)
    return 0


def read(path):
    manifest = strict_json((path / "MANIFEST.json").read_text())
    completion = strict_json((path / "COMPLETION.json").read_text())
    require(manifest["schema"] == SCHEMA and completion["status"] == "passed" and
            completion["error"] is None and
            completion["manifest_sha256"] == digest(path / "MANIFEST.json"), "capture incomplete or corrupt")
    require(manifest["public_status"] == "not_claimed" and manifest["gcp_used"] is False and
            manifest["full_contract_qualified"] is False, "capture changed scope")
    require(manifest["source_sha256"] == completion["source_sha256_after"] and
            manifest["artifact_sha256"] == completion["artifact_sha256_after"] and
            manifest.get("pinned_artifact_sha256", {}) == completion.get("pinned_artifact_sha256_after", {}),
            "closure hash mismatch")
    commands = manifest["planned_commands"]
    expected_commands = plan(Path(manifest["build"]), path.resolve(), manifest["campaign"], manifest.get("pinned_build"))
    require(commands == [[kind, command] for kind, command in expected_commands], "unrecognized qualification commands")
    validate_pins(manifest)
    require(len(commands) == len(completion["records"]) == len(expected_commands), "missing records")
    require({p.name for p in path.glob("record_*.json")} ==
            {f"record_{i:04}.json" for i in range(len(commands))}, "orphan or missing record")
    rows = []
    for number, ((kind, command), info) in enumerate(zip(commands, completion["records"], strict=True)):
        require(info["path"] == f"record_{number:04}.json", "invalid or repeated record path")
        record_path = path / info["path"]
        require(digest(record_path) == info["sha256"], "record hash mismatch")
        record = strict_json(record_path.read_text())
        require(record["kind"] == kind and record["command"] == command and
                record["status"] == "passed" and type(record["exit_code"]) is int and
                record["exit_code"] == 0 and record["cwd"] == str(ROOT), "command/result mismatch")
        for channel in ("stdout", "stderr"):
            require(base64.b64decode(record[channel + "_base64"], validate=True).decode("utf-8", errors="replace") ==
                    record[channel], "raw/decoded log mismatch")
        if kind in ("measure", "reference", "pinned"):
            row = strict_json(record["stdout"])
            if kind == "measure":
                validate_row(row, command[1:])
            else:
                validate_reference(row, command)
            require(row == record["row"], "parsed row changed")
            rows.append(row)
        if kind == "gate":
            require(strict_json(record["stdout"]).get("status") == "passed", "gate did not pass")
    validate_pairs(rows, manifest["campaign"])
    if manifest["campaign"] == "qualification":
        require(completion["extra_sha256"].get("ctest.xml") == digest(path / "ctest.xml"), "CTest hash mismatch")
        validate_ctest(path / "ctest.xml")
    print(json.dumps(dict(status="passed", path=str(path), records=len(commands),
                          q2_measures=len(rows), full_contract_qualified=False), sort_keys=True))



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--build", type=Path, required=True)
    capture.add_argument("--output", type=Path, required=True)
    capture.add_argument("--campaign", choices=("qualification", "tsan", "smoke", "scale", "parallel", "separations",
                                                "differential"), default="qualification")
    capture.add_argument("--pinned-build", type=Path, default=ROOT / "build/v8_singleton_batch_r3_20260917",
                         help="pre-tranche pinned build whose parallel probe judges the default (differential only)")
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    args = parser.parse_args()
    if args.operation == "run":
        return run(args)
    read(args.path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
