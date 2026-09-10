#!/usr/bin/env python3
"""Bounded independent MEB audit, exact Fraction/Gram elimination, never asserts."""

from __future__ import annotations

import hashlib
import itertools
import json
import math
import os
from pathlib import Path
import subprocess
import sys
from datetime import datetime, timezone
from fractions import Fraction as Q
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
AUDIT = Path(__file__).resolve().parent
WORK = AUDIT / ".work_20260905_meb"
RECEIPT = AUDIT / "receipts_20260905"
Point = tuple[int, int, int]


def check(condition: bool, reason: str) -> None:
    if not condition:
        raise RuntimeError(reason)


def dot(a: Any, b: Any) -> Any:
    return sum(x * y for x, y in zip(a, b))


def subtract(a: Any, b: Any) -> tuple[Any, ...]:
    return tuple(x - y for x, y in zip(a, b))


def determinant(rows: list[list[Any]]) -> Q:
    matrix = [[Q(value) for value in row] for row in rows]
    result = Q(1)
    for column in range(len(matrix)):
        pivot = next((r for r in range(column, len(matrix)) if matrix[r][column]), None)
        if pivot is None:
            return Q(0)
        if pivot != column:
            matrix[column], matrix[pivot] = matrix[pivot], matrix[column]
            result = -result
        divisor = matrix[column][column]
        result *= divisor
        for row in range(column + 1, len(matrix)):
            factor = matrix[row][column] / divisor
            matrix[row] = [v - factor * w for v, w in zip(matrix[row], matrix[column])]
    return result


def circumball(points: list[Point]) -> dict[str, Any] | None:
    """Solve Gram t=|d|^2/2 by rational Gaussian elimination, not Cramer."""
    base = points[0]
    deltas = [subtract(p, base) for p in points[1:]]
    rank = len(deltas)
    matrix = [[Q(dot(a, b)) for b in deltas] + [Q(dot(a, a), 2)]
              for a in deltas]
    for column in range(rank):
        pivot = next((r for r in range(column, rank) if matrix[r][column]), None)
        if pivot is None:
            return None
        matrix[column], matrix[pivot] = matrix[pivot], matrix[column]
        divisor = matrix[column][column]
        matrix[column] = [value / divisor for value in matrix[column]]
        for row in range(rank):
            if row != column:
                scale = matrix[row][column]
                matrix[row] = [v - scale * w for v, w in
                               zip(matrix[row], matrix[column])]
    weights = [row[-1] for row in matrix]
    center = tuple(Q(base[i]) + sum(weights[j] * deltas[j][i]
                                   for j in range(rank)) for i in range(3))
    radius = dot(subtract(center, base), subtract(center, base))
    coefficients = [Q(1)] + [-2 * c for c in center] + [dot(center, center) - radius]
    common = math.lcm(*(value.denominator for value in coefficients))
    key = [int(value * common) for value in coefficients]
    divisor = math.gcd(*key)
    key = [value // divisor for value in key]
    scale = (determinant([[dot(a, b) for b in deltas] for a in deltas])
             if rank == 2 else abs(determinant([[2 * v for v in d] for d in deltas]))
             if rank == 3 else Q(1))
    return {"center": center, "radius": radius, "key": key,
            "scale": scale,
            "positive": all(w > 0 for w in weights) and sum(weights) < 1}


def power(ball: dict[str, Any], point: Point) -> Q:
    delta = subtract(point, ball["center"])
    return dot(delta, delta) - ball["radius"]


def expected_meb(points: list[Point]) -> dict[str, Any]:
    charged = 0
    for q in range(2, min(4, len(points)) + 1):
        for support in itertools.combinations(range(len(points)), q):
            charged += 1
            ball = circumball([points[i] for i in support])
            if ball is None or not ball["positive"]:
                continue
            powers = [power(ball, point) for point in points]
            if any(p > 0 for p in powers):
                continue
            ball.update(q=q, support=list(support), charged=charged,
                        degenerate=sum(p == 0 for p in powers) != q)
            return ball
    raise RuntimeError("oracle.no_meb")


def points_text(points: list[Point]) -> str:
    return " ".join(str(v) for point in points for v in point)


def corpus() -> list[list[Point]]:
    values = [
        [(0, 0, 0), (65535, 65535, 65535)],
        [(0, 0, 0), (65535, 65535, 0), (65535, 0, 65535)],
        [(0, 0, 0), (65535, 65535, 0), (65535, 0, 65535), (0, 65535, 65535)],
        [(0, 0, 0), (8, 0, 0), (8, 8, 0), (0, 8, 0)],
        [(0, 0, 0), (4, 0, 0), (2, 0, 0)],
        [(0, 0, 0), (4, 0, 0), (2, 3, 0), (2, 0, 2)],
        [(0, 0, 0), (46368, 28657, 0), (28657, 17711, 0)],
        [(1, 1, 1), (65534, 1, 1), (1, 65534, 1), (1, 1, 65534)],
        [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)],
    ]
    state = 0x61756469742D6D65

    def next_coordinate() -> int:
        nonlocal state
        state = (state * 6364136223846793005 + 1442695040888963407) % (1 << 64)
        return (state >> 32) & 65535

    for i in range(80):
        points = [(next_coordinate(), next_coordinate(), next_coordinate())
                  for _ in range(2 + i % 10)]
        check(len(set(points)) == len(points), "corpus.duplicates")
        values.append(points)
    return values


def execute(binary: Path, commands: list[str], mutant: str = "") -> list[dict[str, Any]]:
    argv = [str(binary)] + (["--mutant=" + mutant] if mutant else [])
    run = subprocess.run(argv, input="\n".join(commands) + "\n", text=True,
                         capture_output=True, check=False)
    check(run.returncode == 0, f"bridge.exit={run.returncode}: {run.stderr}")
    check(not run.stderr, "bridge.stderr: " + run.stderr)
    result = [json.loads(line) for line in run.stdout.splitlines()]
    check(len(result) == len(commands), "bridge.response_count")
    return result


def main() -> int:
    WORK.mkdir(exist_ok=True)
    RECEIPT.mkdir(exist_ok=True)
    binary = WORK / "meb_oracle_bridge"
    started = datetime.now(timezone.utc).isoformat()
    snapshot_paths = [Path(__file__), AUDIT / "meb_oracle_bridge_20260905.cpp"]
    snapshot_paths += list((ROOT / "morsehgp3D_v7/src").rglob("*.hpp"))
    snapshot = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in sorted(snapshot_paths)}
    worktree_before = subprocess.check_output(["git", "status", "--porcelain"], cwd=ROOT, text=True)
    environment = dict(os.environ, TMPDIR=str(WORK))
    flags = ["g++", "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic",
             "-Werror", "-fsanitize=undefined", "-fno-sanitize-recover=all",
             "-DMHGP7_TESTING", "-I", str(ROOT / "morsehgp3D_v7"),
             str(AUDIT / "meb_oracle_bridge_20260905.cpp"), "-o", str(binary)]
    compiled = subprocess.run(flags, capture_output=True, text=True, check=False, env=environment)
    check(compiled.returncode == 0, "compile.failed: " + compiled.stderr)
    check(not compiled.stderr, "compile.stderr: " + compiled.stderr)
    cases = corpus()
    oracle = [expected_meb(points) for points in cases]
    commands: list[str] = []
    metadata: list[tuple[int, int]] = []
    for i, points in enumerate(cases):
        needed = oracle[i]["charged"]
        for cap in sorted({0, needed - 1, needed, needed + 1, 550}):
            commands.append(f"M {len(points)} {cap} " + points_text(points))
            metadata.append((i, cap))
    output = execute(binary, commands)
    counts = {"local_cases": len(cases), "local_calls": len(output), "cap_rejects": 0,
              "shell_rejects": 0, "q2": 0, "q3": 0, "q4": 0,
              "power_q3": 0, "power_q4": 0, "power_negative": 0,
              "power_zero": 0, "power_positive": 0, "rank_rejects": 0,
              "primitive_reductions": 0, "mutants_killed": 0}
    for observed, (index, cap) in zip(output, metadata):
        expected = oracle[index]
        needed = expected["charged"]
        check(observed["calls"] == 1 and observed["supports"] == min(cap, needed),
              f"case.{index}.budget.{cap}")
        if cap < needed:
            counts["cap_rejects"] += 1
            check(not observed["ok"] and observed["status"] == 3 and
                  observed["reason"] == "silent_meb_support_budget", "cap.status_reason")
            check(observed["q"] == 9 and observed["key"] == [7, 11, 13, 17, 19] and
                  observed["num"] == [23, 29, 31] and observed["den"] == 37 and
                  observed["support"] == [-1, -2, -3, -4], "cap.output_sentinel")
            continue
        degenerate = expected["degenerate"]
        check(observed["ok"] != degenerate, f"case.{index}.acceptance")
        check(observed["status"] == (2 if degenerate else 0), "local.status")
        check(observed["reason"] == ("silent_local_nonessential_shell" if degenerate else
              "complete_relative_to_supplied_regular_direct_catalogue"), "local.reason")
        counts["shell_rejects"] += degenerate
        counts["q" + str(expected["q"])] += 1
        check(observed["q"] == expected["q"] and
              observed["support"][:expected["q"]] == expected["support"], "local.support")
        check(observed["key"] == expected["key"], "local.key")
        numerator = sum(v << (64 * j) for j, v in enumerate(observed["num"]))
        check(Q(numerator, observed["den"]) == expected["radius"], "local.level")
        if expected["q"] < 4:
            check(numerator == expected["radius"].numerator and
                  observed["den"] == expected["radius"].denominator, "local.level_reduced")
        else:
            base = cases[index][expected["support"][0]]
            raw_center = [value * expected["scale"]
                          for value in subtract(expected["center"], base)]
            check(numerator == dot(raw_center, raw_center) and
                  observed["den"] == expected["scale"] ** 2, "local.q4_level_raw_repr")
    power_commands: list[str] = []
    power_expected: list[tuple[int, dict[str, Any], Point]] = []
    for points in cases:
        for q in (3, 4):
            # Every support in the first 9 cases, a deterministic bounded prefix thereafter.
            combinations = list(itertools.combinations(points, q))
            if points not in cases[:9]:
                combinations = combinations[:8]
            for support in combinations:
                ball = circumball(list(support))
                if ball is None:
                    counts["rank_rejects"] += 1
                    continue
                probes = list(support) + [(0, 0, 0), (65535, 65535, 65535),
                                          (32768, 32768, 32768)]
                for z in probes:
                    power_commands.append(f"P {q} " + points_text(list(support) + [z]))
                    power_expected.append((q, ball, z))
    power_output = execute(binary, power_commands)
    for observed, (q, ball, z) in zip(power_output, power_expected):
        expected = power(ball, z)
        check(observed["key"] == ball["key"], "power.primitive_key")
        check(observed["raw"] == expected * observed["scale"], "power.raw_exact")
        check(observed["primitive"] == expected * ball["key"][0], "power.primitive_exact")
        check(observed["scale"] == ball["scale"] and observed["scale"] > 0 and
              observed["scale"] % ball["key"][0] == 0,
              "power.positive_gcd")
        counts["power_q" + str(q)] += 1
        counts["primitive_reductions"] += observed["scale"] > ball["key"][0]
        counts["power_" + ("positive" if expected > 0 else "negative" if expected < 0 else "zero")] += 1
    mutants = []
    for index, q in ((1, 3), (2, 4)):
        points = cases[index]
        command = f"M {len(points)} 550 " + points_text(points)
        mutant = f"silent-meb-q{q}-reject-shell"
        altered = execute(binary, [command], mutant)[0]
        check(oracle[index]["q"] == q and not oracle[index]["degenerate"], "mutant.baseline")
        check(not altered["ok"] and altered["status"] == 4 and
              altered["reason"] == "silent_no_local_miniball", "mutant.not_killed")
        counts["mutants_killed"] += 1
        mutants.append({"name": mutant, "diagnostic": altered["reason"], "killed": True})
    check(all(counts[name] > 0 for name in counts), "nonvacuity")
    check(all(hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == digest
              for path, digest in snapshot.items()), "source.changed_during_audit")
    receipt = {"status": "passed", "public_status": "not_claimed", "backend": "cpu_reference",
               "profile": "quantized_u16_input_only", "oracle": "Python Fraction Gram elimination",
               "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
               "python_optimized": not __debug__, "compile_command": flags,
               "started_utc": started, "ended_utc": datetime.now(timezone.utc).isoformat(),
               "compiler": subprocess.check_output(["g++", "--version"], text=True).splitlines()[0],
               "python": sys.version, "worktree_before": worktree_before.splitlines(),
               "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
               "compile_exit_code": compiled.returncode, "bridge_exit_codes": [0, 0, 0, 0],
               "sanitizer": "undefined, abort on first finding", "counts": counts,
               "mutants": mutants, "sources": snapshot,
               "corpus_sha256": hashlib.sha256(json.dumps(cases).encode()).hexdigest(),
               "meb_output_sha256": hashlib.sha256(json.dumps(output, sort_keys=True).encode()).hexdigest(),
               "powers_output_sha256": hashlib.sha256(json.dumps(power_output, sort_keys=True).encode()).hexdigest(),
               "gcp": "not_used"}
    (RECEIPT / "meb_rational_raw.json").write_text(json.dumps(
        {"corpus": cases, "meb_commands": commands, "meb_output": output,
         "power_commands": power_commands, "power_output": power_output},
        separators=(",", ":")) + "\n")
    destination = RECEIPT / ("meb_rational_optimized.json" if not __debug__ else "meb_rational.json")
    destination.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"status": "passed", "counts": counts, "receipt": str(destination.relative_to(ROOT))}))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, ValueError) as error:
        print(json.dumps({"status": "failed", "reason": str(error)}), file=sys.stderr)
        sys.exit(1)
