#!/usr/bin/env python3
"""Bounded real captures and independent mutations of the resume row reader."""

import argparse
from copy import deepcopy
import json
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bench"))
from run_q2_resume_checks import validate_row  # noqa: E402


def require(value, message):
    if not value:
        raise RuntimeError(message)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", required=True)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    calls, mutants, invalid = 0, 0, 0
    for family in ("uniform", "terrain", "clusters", "rows"):
        for separation in (8, 10, 12):
            for quantum in (1, 7):
                command = ["24", family, "5", str(separation), "3", str(quantum)]
                result = subprocess.run([args.probe, *command], capture_output=True, text=True, timeout=20)
                require(result.returncode == 0, f"probe failed: {result.stdout}\n{result.stderr}")
                row = json.loads(result.stdout)
                validate_row(row, command)
                calls += 1
                edits = [(("scope",), "full"), (("full_contract_qualified",), True),
                         (("quantum",), quantum + 1), (("work_equal",), False),
                         (("ordered_payload_equal",), False), (("candidate_pairs",), row["b_size"] + 1),
                         (("census_work", "frontier_restarts"), 1),
                         (("census_work", "count_root_starts"), 2),
                         (("resume_work", "transitions"), row["resume_work"]["transitions"] + 1),
                         (("resume_work", "advance_calls"), 0),
                         (("resume_work", "pauses_after_credit"), row["resume_work"]["pauses"] + 1),
                         (("memory", "stack_capacity"), 0),
                         (("memory", "retained_bytes"), row["memory"]["retained_bytes"] + 1),
                         (("timing_ms",), {}), (("sibling_work",), {}),
                         (("timing_ms", "max_slice"), float("nan")),
                         (("timing_ms", "probe"), -1)]
                for path, value in edits:
                    mutant = deepcopy(row)
                    target = mutant
                    for key in path[:-1]:
                        target = target[key]
                    target[path[-1]] = value
                    rejected = False
                    try:
                        validate_row(mutant, command)
                    except (RuntimeError, ValueError, KeyError):
                        rejected = True
                    require(rejected, f"reader accepted mutant {path}")
                    mutants += 1
                for group in ("census_work", "sibling_work", "order_work", "resume_work", "memory", "timing_ms"):
                    mutant = deepcopy(row)
                    del mutant[group][next(iter(mutant[group]))]
                    rejected = False
                    try:
                        validate_row(mutant, command)
                    except (RuntimeError, ValueError, KeyError):
                        rejected = True
                    require(rejected, f"reader accepted missing field in {group}")
                    mutants += 1
    for command in ([], ["24", "uniform", "5", "8", "3", "0"],
                    ["24", "uniform", "5", "8", "3", "-1"],
                    ["24", "wrong", "5", "8", "3", "1"],
                    ["24", "uniform", "0", "8", "3", "1"],
                    ["24", "uniform", "5", "0", "3", "1"],
                    ["25", "rows", "5", "8", "3", "1"]):
        result = subprocess.run([args.probe, *command], capture_output=True, text=True, timeout=20)
        require(result.returncode == 2 and not result.stdout and result.stderr, "invalid CLI accepted")
        invalid += 1
    print(json.dumps(dict(status="passed", real_captures=calls, rejected_mutants=mutants,
                          invalid_cli=invalid), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
