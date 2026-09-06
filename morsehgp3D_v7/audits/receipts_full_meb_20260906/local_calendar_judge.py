"""Judge fixed tetrahedron calendars; never compile or execute a C++ engine."""
from __future__ import annotations

import argparse
from fractions import Fraction
import hashlib
import itertools
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def det(matrix: list[list[int]]) -> int:
    return sum((-1) ** sum(p[i] > p[j] for i in range(3) for j in range(i + 1, 3)) *
               matrix[0][p[0]] * matrix[1][p[1]] * matrix[2][p[2]]
               for p in itertools.permutations(range(3)))


def expectations() -> dict:
    expected = json.loads((HERE / "local_calendar_expected.json").read_text())
    points, center = expected["points"], expected["center"]
    for a, b in itertools.combinations(points, 2):
        require(sum((x - y) ** 2 for x, y in zip(a, b)) == 8, "rational.equal_diameter")
    require([sum(p[j] for p in points) * Fraction(1, 4) for j in range(3)] == center,
            "rational.positive_center")
    require(all(sum((x - y) ** 2 for x, y in zip(p, center)) == 3 for p in points),
            "rational.shell")
    for omitted in range(4):
        face = [point for i, point in enumerate(points) if i != omitted]
        triangle_center = [sum(p[j] for p in face) * Fraction(1, 3) for j in range(3)]
        require(all(sum((x - y) ** 2 for x, y in zip(p, triangle_center)) == Fraction(8, 3)
                    for p in face), "rational.positive_equilateral_triangle")
        require(sum((x - y) ** 2 for x, y in zip(points[omitted], triangle_center)) == Fraction(16, 3),
                "rational.each_q3_misses_fourth_vertex")
    # Cramer determinants computed from the defining equations, not q4_form.
    matrix = [[2 * (p[j] - points[0][j]) for j in range(3)] for p in points[1:]]
    rhs = [sum((x - y) ** 2 for x, y in zip(p, points[0])) for p in points[1:]]
    determinant = det(matrix)
    numerators = []
    for column in range(3):
        replaced = [row[:] for row in matrix]
        for i in range(3):
            replaced[i][column] = rhs[i]
        numerators.append(det(replaced) * (-1 if determinant < 0 else 1))
    require(determinant == expected["signed_determinant_twice_edges"] and
            numerators == expected["orientation_normalized_cramer_numerators"], "rational.cramer")
    terminal = expected["terminal"]
    require(terminal["num"] == [sum(x * x for x in numerators), 0, 0] and
            terminal["den"] == determinant ** 2, "rational.raw_level")
    require(terminal["key"] == [1, *[-2 * x for x in center], sum(x * x for x in center) - 3],
            "rational.reduced_ball_key")
    require(expected["ordinal_decomposition"] ==
            [len(list(itertools.combinations(range(4), q))) for q in (2, 3, 4)] and
            expected["reference_ordinal"] == sum(expected["ordinal_decomposition"]) == 11,
            "rational.ordinal")
    return expected


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, nargs="?")
    parser.add_argument("--expect-q4-first", action="store_true")
    args = parser.parse_args()
    expected = expectations()
    if args.output is None:
        require(not args.expect_q4_first, "arguments.mutant_requires_output")
        print(json.dumps({"status": "expectations_checked", "engines_invoked": 0}))
        return
    raw = args.output.read_bytes()
    rows = [json.loads(line) for line in raw.decode().splitlines()]
    require(len(rows) == 9, "rows.exact_count")
    bound = []
    # Entire geometry is checked BEFORE any calendar difference is accepted.
    for sequence in expected["sequences"]:
        for call in range(1, 4):
            row = rows[len(bound)]
            require((row["P"], row["L"], row["call"], row["accounting"]) ==
                    (sequence["P"], 33, call, expected["accounting"]), "metadata.calendar")
            terminal = dict(expected["terminal"], c=11 * call, meb_calls=call)
            require(row["reference"] == terminal and row["proposed"] == terminal,
                    "geometry.literal_terminal_support_key_raw_level")
            bound.append((row, sequence, call))
    differences = []
    for row, sequence, call in bound:
        nominal = dict(zip(expected["work_columns"], sequence["nominal"][call - 1]))
        changed = dict(zip(expected["work_columns"], sequence["q4_first"][call - 1]))
        if row["work"] != nominal:
            differences.append({"P": sequence["P"], "call": call,
                                "nominal": nominal, "observed": row["work"]})
        require(row["work"] == (changed if args.expect_q4_first else nominal),
                "proposal_calendar.P_admission_work_changed")
    if args.expect_q4_first:
        first = differences[0] if differences else {}
        require(len(differences) == 9 and first.get("P") == 3 and first.get("call") == 1 and
                first["nominal"]["certified"] == 0 and first["observed"]["certified"] == 1,
                "mutant.causal_P3_certification_after_geometry_equality")
    print(json.dumps({"status": "refuted" if args.expect_q4_first else "passed",
                      "cause": "q4_first_changes_P3_certification_same_geometry" if args.expect_q4_first else "none",
                      "rows": 9, "literal_geometries_checked_before_calendar": 18,
                      "persistent_sequences": 3, "calendar_differences": differences,
                      "output_sha256": hashlib.sha256(raw).hexdigest(),
                      "public_status": "not_claimed", "engines_invoked": 0}, indent=2))


if __name__ == "__main__":
    main()
