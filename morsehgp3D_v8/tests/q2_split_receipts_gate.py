#!/usr/bin/env python3
"""Tiny real split captures and independent mutations of their row reader.

The scalar geometry oracle belongs to the C++ gates. This gate checks the
receipt contract, without treating a selected anchor as a whole-front or
FULL result. Explicitly follows the bounded resume-receipts gate pattern;
it imports the new reader, never an earlier qualification.
"""

import argparse
from copy import deepcopy
import json
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bench"))
from run_q2_split_checks import validate_row  # noqa: E402


def require(value, message):
    if not value:
        raise RuntimeError(message)


def rejected(row, command, label):
    try:
        validate_row(row, command)
    except (RuntimeError, ValueError, KeyError, TypeError, OverflowError):
        return 1
    raise RuntimeError(f"split reader accepted mutant: {label}")


def changed(row, command, path, value):
    mutant = deepcopy(row)
    target = mutant
    for name in path[:-1]:
        target = target[name]
    target[path[-1]] = value
    return rejected(mutant, command, ".".join(path))


def check_mutants(row, command, exhaustive_schema):
    g, r, d, c = (row[name] for name in ("geometry", "resume", "detach", "schedule"))
    edits = [
        (("schema",), "mhgp8_q2_selected_anchor_resume_v1"),
        (("scope",), "full"), (("public_status",), "qualified"),
        (("full_contract_qualified",), True),
        (("work_equal",), False), (("payload_equal",), False),
        (("work_equal",), 1), (("payload_equal",), 1),
        (("candidates",), row["candidates"] + 1),
        (("anchor_rank",), row["n"]),
        (("geometry", "input_descriptors"), 2),
        (("geometry", "count_root_starts"), 2),
        (("geometry", "frontier_restarts"), 1),
        (("geometry", "query_tasks"), g["query_tasks"] + 1),
        (("geometry", "query_splits"), g["query_splits"] + 1),
        (("geometry", "cursor_reuses"), g["cursor_reuses"] + 1),
        (("geometry", "sibling_proposals"), g["sibling_proposals"] + 1),
        (("geometry", "count_bound_tests"), g["count_bound_tests"] + 1),
        (("geometry", "payload_point_tests"), g["payload_point_tests"] + 1),
        (("geometry", "payload_supports"), g["payload_supports"] + 1),
        (("resume", "transitions"), r["transitions"] + 1),
        (("resume", "entry_steps"), r["entry_steps"] + 1),
        (("resume", "witness_steps"), r["witness_steps"] + 1),
        (("resume", "admission_steps"), r["payload_steps"] + 1),
        (("resume", "payload_steps"), r["payload_steps"] + 1),
        (("resume", "advance_calls"), 0),
        (("resume", "pauses"), r["pauses"] + 1),
        (("resume", "max_pending_tasks"), 0),
        (("resume", "max_pending_tasks"), 50),
        (("detach", "detached_frames"), d["detached_frames"] + 1),
        (("detach", "imported_frames"), d["imported_frames"] + 1),
        (("detach", "attempts"), d["attempts"] + 1),
        (("schedule", "donations"), c["donations"] + 1),
        (("schedule", "fragments_started"), c["fragments_started"] + 1),
        (("schedule", "fragments_completed"), c["fragments_completed"] + 1),
        (("schedule", "offer_checks"), c["offer_checks"] + 1),
        (("schedule", "offer_busy"), c["offer_busy"] + 1),
        (("schedule", "offer_full"), c["offer_full"] + 1),
        (("schedule", "offer_no_sibling"), c["offer_no_sibling"] + 1),
        (("schedule", "wakes"), c["wakes"] + 1),
        (("schedule", "max_queue_size"), row["queue"] + 1),
        (("schedule", "max_active_fragments"), row["workers"] + 1),
        (("queue_storage_bytes",), 0), (("max_fragment_bytes",), 0),
        (("timings_ms",), {}),
    ]
    for name in ("query_build_point_visits", "query_build_nodes", "query_build_max_depth",
                 "query_cover_visits"):
        edits.append((("geometry", name), 1))
    for name in ("pauses_after_credit", "pauses_inside_deferred", "pauses_during_emission"):
        edits.append((("resume", name), r["pauses"] + 1))
    for value in (-1, float("nan"), float("inf"), True, "0"):
        edits.append((("timings_ms", "parallel_enclosing"), value))

    count = sum(changed(row, command, path, value) for path, value in edits)
    for value in ([], row["worker_transitions"] + [0], tuple(row["worker_transitions"]),
                  [True] + row["worker_transitions"][1:],
                  [row["worker_transitions"][0] + 1] + row["worker_transitions"][1:]):
        count += changed(row, command, ("worker_transitions",), value)

    # Every argv component is bound to its corresponding row field. Use a
    # valid alternative family so this exercises correspondence, not parsing.
    for position, value in enumerate(command):
        other = command.copy()
        other[position] = ("terrain" if value != "terrain" else "uniform") if position == 1 else str(int(value) + 1)
        count += rejected(deepcopy(row), other, f"command[{position}]")
    for other in (command[:-1], command + ["extra"]):
        count += rejected(deepcopy(row), other, "command arity")

    # Once suffices for exhaustive key/type coverage; the arithmetic mutants
    # above run on every actual family/s/worker capture, not a synthetic row.
    if exhaustive_schema:
        for name in row:
            mutant = deepcopy(row)
            del mutant[name]
            count += rejected(mutant, command, f"missing top-level {name}")
        for name in ("geometry", "resume", "detach", "schedule", "timings_ms"):
            for key in row[name]:
                mutant = deepcopy(row)
                del mutant[name][key]
                count += rejected(mutant, command, f"missing {name}.{key}")
            mutant = deepcopy(row)
            mutant[name]["unexpected"] = 0
            count += rejected(mutant, command, f"extra {name} field")
        for name, value in row.items():
            if type(value) is int:
                for bad in (True, -1, 2**64, str(value)):
                    count += changed(row, command, (name,), bad)
        for name in ("geometry", "resume", "detach", "schedule"):
            for key in row[name]:
                count += changed(row, command, (name, key), True)
            for bad in (-1, 2**64, "0", None):
                count += changed(row, command, (name, next(iter(row[name]))), bad)
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    calls, mutants, invalid, paired = 0, 0, 0, 0
    covered_quantum, covered_queue = set(), set()
    for family_number, family in enumerate(("uniform", "terrain", "clusters", "rows")):
        for separation_number, separation in enumerate((8, 10, 12)):
            quantum = (1, 256)[(family_number + separation_number) % 2]
            reference = None
            for worker_number, workers in enumerate((1, 4)):
                queue = (1, 8)[(separation_number + worker_number) % 2]
                command = ["24", family, "5", str(separation), "3", str(quantum), str(workers), str(queue)]
                result = subprocess.run([args.probe, *command], capture_output=True, text=True, timeout=20)
                require(result.returncode == 0 and not result.stderr,
                        f"split probe failed: {result.stdout}\n{result.stderr}")
                row = json.loads(result.stdout)
                validate_row(row, command)
                mutants += check_mutants(row, command, calls == 0)
                calls += 1
                covered_quantum.add((workers, quantum))
                covered_queue.add((workers, queue))
                if reference is None:
                    reference = row
                else:
                    for key in ("input_hash", "anchor_rank", "a_node", "b_node", "front_products",
                                "front_rectangles", "candidates", "accepted", "rejected", "hash_sum",
                                "hash_xor", "geometry"):
                        require(reference[key] == row[key], f"worker count changed {key}")
                    for key in ("transitions", "entry_steps", "witness_steps", "admission_steps", "payload_steps"):
                        require(reference["resume"][key] == row["resume"][key], f"worker count replayed {key}")
                    paired += 1
    require(calls == 24 and paired == 12 and
            covered_quantum == {(w, q) for w in (1, 4) for q in (1, 256)} and
            covered_queue == {(w, q) for w in (1, 4) for q in (1, 8)}, "capture matrix incomplete")

    valid = ["24", "uniform", "5", "8", "3", "1", "4", "1"]
    invalid_commands = [[], valid[:-1], valid + ["extra"]]
    for position, values in ((0, ("0", "1", "-1")), (1, ("wrong",)),
                             (2, ("0", "11", "-1")), (3, ("0", "-1")),
                             (4, ("-1", "18446744073709551616")),
                             (5, ("0", "-1")), (6, ("0", "-1")), (7, ("0", "-1"))):
        for value in values:
            command = valid.copy()
            command[position] = value
            invalid_commands.append(command)
    invalid_commands.append(["25", "rows", "5", "8", "3", "1", "4", "1"])
    for command in invalid_commands:
        result = subprocess.run([args.probe, *command], capture_output=True, text=True, timeout=20)
        require(result.returncode == 2 and not result.stdout and result.stderr,
                f"invalid split CLI accepted: {command}")
        invalid += 1
    print(json.dumps(dict(status="passed", real_captures=calls, paired_workers=paired,
                          rejected_mutants=mutants, invalid_cli=invalid,
                          full_contract_qualified=False), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
