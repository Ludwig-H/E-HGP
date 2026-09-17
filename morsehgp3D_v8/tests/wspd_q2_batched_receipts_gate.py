#!/usr/bin/env python3
"""Bounded real batched captures and hostile receipt mutations.

Explicit port of anchor-range receipt mutations at 2741d614. The new row reader alone
is imported; geometric independence is supplied by the separate C++ gate,
not by a historical receipt or by the checks in this Python file.
"""

import argparse
from copy import deepcopy
import json
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bench"))
from run_wspd_q2_batched_checks import (  # noqa: E402
    ARTIFACT_NAMES, SOURCE_PATHS, strict_json, validate_pins, validate_row)


def require(value, message):
    if not value:
        raise RuntimeError(message)


def at(value, path):
    for key in path:
        value = value[key]
    return value


def reject(row, command, label):
    try:
        validate_row(row, command)
    except (RuntimeError, ValueError, KeyError, TypeError, OverflowError):
        return 1
    raise RuntimeError(f"batched reader accepted mutant: {label}")


def replace(row, command, path, value):
    mutant = deepcopy(row)
    at(mutant, path[:-1])[path[-1]] = value
    return reject(mutant, command, repr(path))


def dictionaries(value, path=()):
    if type(value) is dict:
        yield path, value
        for key, child in value.items():
            yield from dictionaries(child, (*path, key))
    elif type(value) is list:
        for key, child in enumerate(value):
            yield from dictionaries(child, (*path, key))


def check_unit_contracts():
    """Pure unit models: no claimed capture, executable, or qualification."""
    def rejected_call(call, label):
        try:
            call()
        except (RuntimeError, ValueError, KeyError, TypeError, OverflowError):
            return 1
        raise RuntimeError(f"reader accepted unit-contract mutant: {label}")

    require(strict_json('{"x":1,"nested":{"x":2},"values":[null,true]}') ==
            {"x": 1, "nested": {"x": 2}, "values": [None, True]}, "valid strict JSON rejected")
    count = 0
    for text in ('{"x":1,"x":2}', '{"outer":{"x":1,"x":2}}',
                 '{"x":NaN}', '{"x":Infinity}', '{"x":1e999}'):
        count += rejected_call(lambda: strict_json(text), "duplicate/nonfinite JSON")

    # This nonexistent model build tests pin membership/path rules only. Hash
    # values are deliberately synthetic, not measurements of repository files.
    root = Path(__file__).resolve().parents[2]
    build = root / "build" / "batched_receipts_gate_model"
    relative = build.relative_to(root)
    cache = str(relative / "CMakeCache.txt")
    executable = build / "mhgp8_wspd_q2_batched_probe"
    manifest = dict(source_sha256={name: "0" * 64 for name in SOURCE_PATHS},
                    artifact_sha256={str(relative / name): "1" * 64 for name in
                                     {*ARTIFACT_NAMES, "CMakeCache.txt"}},
                    campaign="qualification", build=str(build), planned_commands=[["measure", [str(executable)]]])
    validate_pins(manifest)
    source = sorted(SOURCE_PATHS)[0]
    mutants = []
    missing_source = deepcopy(manifest)
    del missing_source["source_sha256"][source]
    mutants.append((missing_source, "missing source pin"))
    extra_source = deepcopy(manifest)
    extra_source["source_sha256"]["morsehgp3D_v8/src/unknown_source.cpp"] = "0" * 64
    mutants.append((extra_source, "unknown source pin"))
    wrong_hash = deepcopy(manifest)
    wrong_hash["source_sha256"][source] = "A" * 64
    mutants.append((wrong_hash, "noncanonical source hash"))
    missing_cache = deepcopy(manifest)
    del missing_cache["artifact_sha256"][cache]
    mutants.append((missing_cache, "missing cache pin"))
    missing_indirect_executable = deepcopy(manifest)
    del missing_indirect_executable["artifact_sha256"][str(relative / "mhgp8_axis_q2_gate")]
    mutants.append((missing_indirect_executable, "missing indirectly executed CTest binary"))
    escaped_artifact = deepcopy(manifest)
    escaped_artifact["artifact_sha256"]["build/elsewhere/mhgp8_probe"] = "3" * 64
    mutants.append((escaped_artifact, "artifact outside model build"))
    unpinned_command = deepcopy(manifest)
    unpinned_command["planned_commands"][0][1][0] = str(build / "mhgp8_unpinned_probe")
    mutants.append((unpinned_command, "unpinned command executable"))
    for mutant, label in mutants:
        count += rejected_call(lambda: validate_pins(mutant), label)
    require(count == 12, "unit mutation inventory changed")
    return count


def check_mutants(row, command, exhaustive):
    count = 0
    edits = [
        (("schema",), "mhgp8_wspd_q2_parallel_probe_v1"),
        (("scope",), "full"), (("public_status",), "qualified"),
        (("full_contract_qualified",), True), (("gcp_used",), True),
        (("execution",), "parallel_front"), (("worker_clock_scope",), "presence_including_waits"),
        (("pool_clock_scope",), "enclosing_parent_wall"),
        (("batch_storage", "state_bytes"), 0),
        (("batch_storage", "state_capacity_sum"), row["batch_storage"]["state_capacity_sum"] + 1),
        (("candidate_pairs",), row["candidate_pairs"] + 1),
        (("census_work", "input_descriptors"), row["input_rectangles"] + 1),
        (("census_work", "query_cover_visits"), 1),
        (("census_work", "frontier_restarts"), 1),
        (("census_work", "count_root_starts"), row["census_work"]["count_root_starts"] + 1),
        (("parallel_work", "queue_storage_bytes"), 1),
        (("parallel_work", "completed_jobs"), row["parallel_work"]["jobs"] + 1),
        (("parallel_work", "started_workers"), row["threads"] + 1),
        (("workers",), []), (("workers",), tuple(row["workers"])),
        (("workers", 0, "jobs"), row["workers"][0]["jobs"] + 1),
        (("workers", 0, "front_products"), row["workers"][0]["front_products"] + 1),
        (("workers", 0, "count_node_visits"), row["workers"][0]["count_node_visits"] + 1),
        (("timings", "pipeline_wall_ms"), row["timings"]["total_ms"] + 1),
    ]
    for path in (("timings", "total_ms"), ("workers", 0, "elapsed_ms"),
                 ("pool_work", "preparation_ms_sum")):
        for value in (-1, float("nan"), float("inf"), True, "0"):
            edits.append((path, value))
    count += sum(replace(row, command, path, value) for path, value in edits)

    # Alter the global reduction and, independently, one worker contribution.
    # Raise worker maxima ABOVE the global maximum, even if worker0 was not
    # the maximizing slot. Every scalar is covered, including transfer traffic
    # that has no useful bound by the original Cartesian population.
    total = row["batch_work"]
    for name, value in total.items():
        count += replace(row, command, ("batch_work", name), value + 1)
        worker_path = ("workers", 0, "batch_work", name)
        count += replace(row, command, worker_path, max(value, at(row, worker_path)) + 1)

    # Changing a global lifecycle total cannot be hidden by locally valid
    # transition equalities: worker contributions must still close exactly.
    mutant = deepcopy(row)
    mutant["batch_work"]["seed_flushes"] += 1
    count += reject(mutant, command, "global seed flush without worker contribution")

    # Counterfactual max-as-sum model, only where its numerical value differs.
    max_models = 0
    for suffix in (("max_active",),):
        summed = sum(at(worker["batch_work"], suffix) for worker in row["workers"])
        if summed != at(total, suffix):
            count += replace(row, command, ("batch_work", *suffix), summed)
            max_models += 1

    for position, value in enumerate(command):
        wrong = command.copy()
        wrong[position] = ("terrain" if value != "terrain" else "uniform") if position == 1 else str(int(value) + 1)
        count += reject(deepcopy(row), wrong, f"command[{position}]")
    for wrong in (command[:-1], command + ["extra"]):
        count += reject(deepcopy(row), wrong, "command arity")

    if exhaustive:
        for path, obj in dictionaries(row):
            for name, value in obj.items():
                mutant = deepcopy(row)
                del at(mutant, path)[name]
                count += reject(mutant, command, f"missing {(*path, name)}")
                if type(value) is int:
                    for wrong in (True, -1, 2**64, str(value)):
                        count += replace(row, command, (*path, name), wrong)
            mutant = deepcopy(row)
            at(mutant, path)["unexpected"] = 0
            count += reject(mutant, command, f"unknown field in {path}")
    return count, max_models


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    calls, paired, mutants, invalid, max_models = 0, 0, 0, 0, 0
    unit_mutants = check_unit_contracts()
    mutants += unit_mutants
    covered_sizes, covered_lanes, covered_pool = set(), set(), set()
    pool_rows, completed_rows, credited_rows = 0, 0, 0
    for family_number, family in enumerate(("uniform", "terrain", "clusters", "rows")):
        for separation_number, separation in enumerate((8, 10, 12)):
            n = (32, 64, 128)[separation_number]
            k = (1, 5, 10)[(family_number + separation_number) % 3]
            lanes = (1, 8, 16)[(family_number + separation_number) % 3]
            pool = (0, 16, 64)[(family_number + 2 * separation_number) % 3]
            if family == "clusters" and n == 128:
                k, pool = 1, 2  # Positive filtered bands, not only Pool passthrough.
            quantum = (1, 3)[(family_number + separation_number) % 2]
            reference = None
            for workers in (1, 4):
                command = [str(n), family, str(k), str(separation), "3", str(workers),
                           "1", str(lanes), str(quantum), str(pool)]
                capture = subprocess.run([args.probe, *command], capture_output=True, text=True, timeout=30)
                require(capture.returncode == 0 and not capture.stderr,
                        f"batched probe failed: {capture.stdout}\n{capture.stderr}")
                row = strict_json(capture.stdout)
                validate_row(row, command)
                checked, maximum = check_mutants(row, command, calls == 0)
                mutants += checked
                max_models += maximum
                calls += 1
                pool_rows += row["pool_work"]["bands"] > 0
                completed_rows += row["batch_work"]["completed"] > 0
                credited_rows += row["batch_work"]["entry_after_credit"] > 0
                covered_sizes.add(n)
                covered_lanes.add(lanes)
                covered_pool.add(pool)
                if reference is None:
                    reference = row
                else:
                    for key in ("input_hash", "total_unordered_pairs", "active_lane_mask", "generation_work",
                                "cloud_work", "index_work", "front_work", "census_work", "sibling_work",
                                "order_work", "joint_work", "callback_work", "digest", "input_rectangles",
                                "anchor_queries", "candidate_pairs", "accepted_pairs", "rejected_pairs"):
                        require(row[key] == reference[key], f"worker count changed discrete {key}")
                    for key, value in row["pool_work"].items():
                        if type(value) is int:
                            require(value == reference["pool_work"][key], f"worker count changed Pool {key}")
                    paired += 1
    require(calls == 24 and paired == 12 and covered_sizes == {32, 64, 128} and
            covered_lanes == {1, 8, 16} and covered_pool == {0, 2, 16, 64} and
            completed_rows > 0 and pool_rows > 0 and credited_rows > 0,
            "tiny batched matrix incomplete or vacuous")

    valid = ["32", "uniform", "5", "8", "3", "4", "1", "1", "1", "0"]
    invalid_commands = [[], valid[:-1], valid + ["extra"]]
    for position in (0, 2, 3, 5, 6, 7, 8):
        for value in ("0", "-1"):
            command = valid.copy()
            command[position] = value
            invalid_commands.append(command)
    for position, value in ((0, "1"), (1, "wrong"), (2, "11"),
                            (4, "-1"), (4, "18446744073709551616"), (9, "-1")):
        command = valid.copy()
        command[position] = value
        invalid_commands.append(command)
    invalid_commands.append(["33", "rows", *valid[2:]])
    for command in invalid_commands:
        capture = subprocess.run([args.probe, *command], capture_output=True, text=True, timeout=30)
        require(capture.returncode == 2 and not capture.stdout and capture.stderr,
                f"invalid batched CLI accepted: {command}")
        invalid += 1
    print(json.dumps(dict(status="passed", real_captures=calls, paired_workers=paired,
                          rejected_mutants=mutants, max_as_sum_models=max_models,
                          pool_captures=pool_rows, completed_captures=completed_rows,
                          credited_captures=credited_rows,
                          unit_contract_mutants=unit_mutants, invalid_cli=invalid,
                          full_contract_qualified=False), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
