#!/usr/bin/env python3
"""Rational judge of real sector/chord C++ helpers, with two small builds."""

from __future__ import annotations

from datetime import datetime, timezone
from fractions import Fraction as Q
import hashlib
import importlib.util
import itertools
import json
import math
import os
from pathlib import Path
import subprocess
import sys
from typing import Any


sys.dont_write_bytecode = True
AUDIT = Path(__file__).resolve().parent
ROOT = AUDIT.parent.parent
WORK = AUDIT / ".work_secteur_corde_compiled_20260905"
OUT = AUDIT / "receipts_front_compiled_20260905/secteur_corde"
spec = importlib.util.spec_from_file_location("independent_gram", AUDIT / "meb_rational_oracle_20260905.py")
if spec is None or spec.loader is None:
    raise RuntimeError("cannot_load_independent_Gram_judge")
gram = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gram)
check = gram.check
dot = gram.dot
sub = gram.subtract


def text_points(points: Any) -> str:
    return " ".join(str(value) for point in points for value in point)


def directions() -> list[tuple[int, int, int]]:
    return [d for d in itertools.chain(itertools.product(range(-3, 4), repeat=3),
                                      itertools.product((-65535, 0, 65535), repeat=3)) if any(d)]


def root_values() -> list[int]:
    values = {0, 1, 2, (1 << 120) - 1, 81 * 65535 ** 6 // 2}
    for exponent in (1, 2, 10, 30, 50, 59):
        for offset in (-1, 0, 1):
            for delta in (-1, 0, 1):
                value = ((1 << exponent) + offset) ** 2 + delta
                if 0 <= value < 1 << 120:
                    values.add(value)
    return sorted(values)


def corpus() -> tuple[list[str], list[tuple[str, Any]]]:
    commands, metadata = [], []
    for d in directions():
        a = tuple(max(-value, 0) for value in d)
        b = tuple(max(value, 0) for value in d)
        for den in (8, 12):
            commands.append("B " + text_points([a, b]) + " " + str(den))
            metadata.append(("B", (d, den)))
    for mode in range(4):
        for value in root_values():
            commands.append(f"S {mode} {value}")
            metadata.append(("S", (mode, value)))
    for j, b in ((8, 1), (8, -1), (8, 0), (3, 2), (2, -3), ((1 << 102) - 1, (1 << 50) - 1)):
        mu = math.isqrt(j // 2) + 1
        levels = sorted({c * mu * b + delta for c in (-4, -2, 0, 2, 4) for delta in (-1, 0, 1)})
        for exact in (0, 1):
            for level in levels:
                commands.append(f"C 0 {exact} {j} {b} {level}")
                metadata.append(("C", (j, b, level, exact)))
    seeds = [((0, 0, 0), (4, 0, 0), (2, 3, 0)),
             ((0, 0, 0), (65535, 65535, 0), (65535, 0, 65535)),
             ((65535, 65535, 65535), (65531, 65535, 65535), (65533, 65532, 65535))]
    for points in seeds:
        oracle = gram.circumball(list(points))
        check(oracle is not None and oracle["positive"], "corpus.positive_seed")
        for z in list(points) + [(2, 1, 0), (2, 1, 3), (32768, 32768, 32768), (0, 0, 0), (65535, 65535, 65535)]:
            for mode in range(4):
                commands.append(f"A {mode} " + text_points(list(points) + [z]))
                metadata.append(("A", (points, z, mode, oracle)))
    for z in ((1, 0, 0), (1, 1, 0), (1, 2, 0)):
        points = [(0, 0, 0), (2, 0, 0), z]
        commands.append("T " + text_points(points))
        metadata.append(("T", points))
    return commands, metadata


def invoke(binary: Path, commands: list[str], args: list[str] | None = None) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    argv = [str(binary)] + (args or [])
    result = subprocess.run(argv, input="\n".join(commands) + "\n", text=True,
                            capture_output=True, check=False)
    record = {"argv": argv, "exit_code": result.returncode, "stdout": result.stdout, "stderr": result.stderr}
    check(result.returncode == 0 and not result.stderr, "bridge.failed: " + str(record))
    parsed = [json.loads(line) for line in result.stdout.splitlines()]
    check(len(parsed) == len(commands), "bridge.response_count")
    return parsed, record


def judge_chord(row: dict[str, Any], j: int, b: int, level: int, counts: dict[str, int]) -> None:
    mu = math.isqrt(j // 2) + 1
    values = [level - c * mu * b for c in (-4, -2, 0, 2, 4)]
    expected = [int(values[i] < 0 and values[i + 1] < 0) for i in range(4)]
    check(row["mu"] == mu and row["counts"] == expected, "chord.endpoint_counts")
    check(row["dead1"] == all(expected) and not row["dead2"], "chord.threshold")
    check(row["exact_calls"] in (0, 1), "chord.lazy_exact_at_most_once")
    counts["chord_exact_updates"] += row["exact_calls"] == 1
    counts["chord_all_certified_updates"] += row["exact_calls"] == 0
    counts["chord_dead"] += row["dead1"]
    counts["chord_alive"] += not row["dead1"]
    for sign, value in zip(row["signs"], values):
        check(sign == 0 or (sign == -1 and value < 0) or (sign == 1 and value >= 0), "chord.certified_sign")
        counts["negative_endpoints"] += value < 0
        counts["zero_endpoints"] += value == 0
        counts["positive_endpoints"] += value > 0
        counts["certified_negative"] += sign == -1
        counts["certified_nonnegative"] += sign == 1
        counts["uncertain_endpoints"] += sign == 0


def sector_expected(points: list[tuple[int, int, int]], u: list[int], v: list[int]) -> list[int]:
    a, b, z = points
    center = tuple(Q(x + y, 2) for x, y in zip(a, b))
    d2 = dot(sub(b, a), sub(b, a))
    vertices = [u, [x + y for x, y in zip(u, v)], v, [-x + y for x, y in zip(u, v)],
                [-x for x in u], [-x - y for x, y in zip(u, v)], [-x for x in v],
                [x - y for x, y in zip(u, v)]]
    def interior(offset: list[int]) -> bool:
        actual_center = [x + y for x, y in zip(center, offset)]
        distance = sub(z, actual_center)
        return dot(distance, distance) < Q(d2, 4) + dot(offset, offset)
    at_origin = interior([0, 0, 0])
    flags = [interior(point) for point in vertices]
    return [int(at_origin and flags[i] and flags[(i + 1) % 8]) for i in range(8)]


def judge(rows: list[dict[str, Any]], metadata: list[tuple[str, Any]]) -> dict[str, int]:
    counts = dict.fromkeys(("basis_cases", "sqrt_cases", "affine_cases", "sector_cases", "chord_cases",
                           "chord_exact_updates", "chord_all_certified_updates", "chord_dead", "chord_alive",
                           "negative_endpoints", "zero_endpoints", "positive_endpoints", "certified_negative",
                           "certified_nonnegative", "uncertain_endpoints", "rounding_fallbacks"), 0)
    basis_for_sector = None
    for row, (kind, data) in zip(rows, metadata):
        if kind == "B":
            d, den = data
            u, v = row["u"], row["v"]
            d2 = dot(d, d)
            candidates = [(0, d[2], -d[1]), (-d[2], 0, d[0]), (d[1], -d[0], 0)]
            norms = sorted((dot(point, point) for point in candidates), reverse=True)
            check(row["ok"] and tuple(u) in candidates and tuple(v) in candidates,
                  "basis.initial_scale_vectors")
            check([dot(u, u), dot(v, v)] == norms[:2], "basis.two_largest_norms")
            check(dot(u, d) == 0 and dot(v, d) == 0, "basis.plane")
            check(dot(u, u) * dot(v, v) > dot(u, v) ** 2, "basis.independent")
            # Alternative judge: minimize |u+t*(sign*v-u)|² over each supporting line.
            for sign in (-1, 1):
                edge = [sign * x - y for x, y in zip(v, u)]
                distance2 = Q(dot(u, u)) - Q(dot(u, edge) ** 2, dot(edge, edge))
                check(distance2 >= Q(d2, 6) and distance2 >= Q(d2, den), "basis.supporting_line_clearance")
            if d == (2, 0, 0):
                basis_for_sector = (u, v)
            counts["basis_cases"] += 1
        elif kind == "S":
            mode, value = data
            check(row["root"] == math.isqrt(value), "sqrt.floor_in_rounding_mode." + str(mode))
            counts["sqrt_cases"] += 1
        elif kind == "C":
            j, b, level, unused = data
            judge_chord(row, j, b, level, counts)
            counts["chord_cases"] += 1
        elif kind == "A":
            points, z, mode, oracle = data
            expected_g = oracle["scale"]
            expected_p = expected_g * gram.power(oracle, z)
            d = sub(points[1], points[0])
            expected_j = 8 * expected_g * (Q(3 * dot(d, d), 8) - oracle["radius"])
            expected_b = gram.determinant([list(d), list(sub(points[2], points[0])), list(sub(z, points[0]))])
            check(row["G"] == expected_g and row["P"] == expected_p and row["L"] == 4 * expected_p,
                  "affine.independent_geometric_power")
            check(row["J"] == expected_j and row["B"] == expected_b, "chord.independent_geometric_coefficients")
            check(row["filter_on"] == (mode == 0), "affine.rounding_guard")
            if mode != 0:
                check(row["exact_calls"] == 1 and row["signs"] == [0] * 5, "rounding.forces_exact")
                counts["rounding_fallbacks"] += 1
            judge_chord(row, int(expected_j), int(expected_b), int(4 * expected_p), counts)
            counts["affine_cases"] += 1
        else:
            check(basis_for_sector is not None, "sector.basis_judged_first")
            expected = sector_expected(data, *basis_for_sector)
            check(row["counts"] == expected and row["minimum"] == min(expected) and
                  row["dead"] == all(expected), "sector.direct_ball_reference")
            counts["sector_cases"] += 1
    check(all(value > 0 for value in counts.values()), "compiled.nonvacuity")
    return counts


def main() -> int:
    WORK.mkdir(exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    commands, metadata = corpus()
    sources = [Path(__file__), AUDIT / "secteur_corde_compiled_probe.cpp", AUDIT / "meb_rational_oracle_20260905.py"]
    sources += list((ROOT / "morsehgp3D_v7/src").rglob("*.hpp"))
    snapshot = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(sources)}
    started = datetime.now(timezone.utc).isoformat()
    signatures = []
    for label, options in (("o2", ["-O2"]), ("ubsan", ["-O1", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all"])):
        binary = WORK / ("probe_" + label)
        flags = ["g++", "-std=c++20"] + options + ["-Wall", "-Wextra", "-Wpedantic", "-Werror",
                 "-frounding-math", "-ffp-contract=off", "-DMHGP7_TESTING", "-pthread", "-I",
                 str(ROOT / "morsehgp3D_v7"), str(AUDIT / "secteur_corde_compiled_probe.cpp"), "-o", str(binary)]
        compiled = subprocess.run(flags, text=True, capture_output=True, env=dict(os.environ, TMPDIR=str(WORK)), check=False)
        check(compiled.returncode == 0 and not compiled.stderr, "compile.failed: " + compiled.stderr)
        rows, nominal = invoke(binary, commands)
        counts = judge(rows, metadata)
        sector_command = "T 0 0 0 2 0 0 1 1 0"
        sector_rows, sector_record = invoke(binary, [sector_command], ["--mutant=sector-kill-nonstrict"])
        check(any(sector_rows[0]["counts"]) and not any(rows[-2]["counts"]), "sector_mutant.count_divergence")
        chord_command = "C 0 1 8 1 -12"
        chord_rows, chord_record = invoke(binary, [chord_command], ["--fault=chord-nonstrict-parameter"])
        chord_base, chord_base_record = invoke(binary, [chord_command])
        check(chord_base[0]["counts"] == [0, 1, 1, 1] and chord_rows[0]["counts"] == [1, 1, 1, 1],
              "chord_parameter_fault.shell_changes_dead")
        data = {"status": "passed", "label": label, "counts": counts, "compile_argv": flags,
                "compile_exit_code": compiled.returncode, "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                "nominal": nominal, "sector_product_mutant": sector_record,
                "chord_parameter_fault": chord_record, "chord_parameter_nominal": chord_base_record,
                "commands": commands, "product_mutants_killed": ["sector-kill-nonstrict"],
                "audit_parameter_faults_killed": ["chord-nonstrict-parameter"]}
        (OUT / (label + ".json")).write_text(json.dumps(data, indent=2, sort_keys=True) + "\n")
        signatures.append({"label": label, "counts": counts,
                           "objects_sha256": hashlib.sha256(json.dumps(rows, sort_keys=True).encode()).hexdigest()})
        print(json.dumps({"build": label, "status": "passed", "counts": counts}), flush=True)
    check(signatures[0]["objects_sha256"] == signatures[1]["objects_sha256"], "builds.differ")
    check(all(hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == value for path, value in snapshot.items()), "source.changed")
    receipt = {"status": "passed", "public_status": "not_claimed", "backend": "cpu_reference",
               "python_optimized": not __debug__, "started_utc": started,
               "ended_utc": datetime.now(timezone.utc).isoformat(), "sources": snapshot,
               "builds": signatures, "gcp": "not_used", "scope": "compiled_local_helpers_only",
               "compiler": subprocess.check_output(["g++", "--version"], text=True).splitlines()[0]}
    (OUT / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
