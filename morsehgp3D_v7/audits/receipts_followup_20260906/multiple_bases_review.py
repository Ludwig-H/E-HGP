"""Exact fixtures for uniqueness of a positive basis at a strict native pivot.

No C++ invocation, trajectory execution, timing or product qualification.
"""

from __future__ import annotations

import hashlib
from itertools import combinations, permutations
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
AUDIT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(AUDIT))
from meb_rational_oracle_20260905 import (  # noqa: E402
    circumball, determinant, dot, expected_meb, power, subtract,
)


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def positive_bases(points: list[tuple[int, int, int]]) -> list[tuple[int, ...]]:
    found = []
    for q in range(2, min(4, len(points)) + 1):
        for support in combinations(range(len(points)), q):
            ball = circumball([points[i] for i in support])
            if ball is not None and ball["positive"] and all(
                    power(ball, point) <= 0 for point in points):
                found.append(support)
    return found


def affine_independent(points: list[tuple[int, int, int]]) -> bool:
    deltas = [subtract(point, points[0]) for point in points[1:]]
    return determinant([[dot(a, b) for b in deltas] for a in deltas]) != 0


def first_at_cap(points: list[tuple[int, int, int]],
                 sequence: list[tuple[int, ...]], cap: int) -> dict:
    charged = 0
    for support in sequence:
        if charged >= cap:
            return {"status": "exhausted", "forms": charged, "support": None}
        charged += 1
        ball = circumball([points[i] for i in support])
        if ball is not None and ball["positive"] and all(
                power(ball, point) <= 0 for point in points):
            return {"status": "accepted", "forms": charged,
                    "support": list(support)}
    return {"status": "rejected", "forms": charged, "support": None}


def main() -> None:
    scenes = [
        ("q2_to_q3", [(0, 0, 0), (2, 2, 0)], (2, 0, 2)),
        ("q3_to_q4", [(0, 0, 0), (2, 2, 0), (2, 0, 2)], (0, 2, 2)),
        ("q4_to_q3_with_diameter_bound",
         [(30, 30, 30), (30, 10, 10), (10, 30, 10), (10, 10, 30)],
         (20, 20, 38)),
        ("q4_replacement", [(1, 1, 1), (3, 3, 1), (3, 1, 3), (1, 3, 3)],
         (0, 0, 0)),
        ("extra_shell_unique_basis", [(2, 1, 1), (1, 2, 1), (1, 1, 2)],
         (0, 1, 1)),
    ]
    records = []
    orders = 0
    for name, old, z in scenes:
        prior = circumball(old)
        require(prior is not None and prior["positive"], "old positive simplex")
        require(power(prior, z) > 0, "strict violator")
        points = old + [z]
        final = expected_meb(points)
        require(final["radius"] > prior["radius"], "strict radius growth")
        diameter2 = max(dot(subtract(a, b), subtract(a, b))
                        for a, b in combinations(points, 2))
        shell = [i for i, point in enumerate(points) if power(final, point) == 0]
        require(affine_independent([points[i] for i in shell]),
                "whole final shell affine independent")
        delta = subtract(final["center"], prior["center"])
        radical_residue = (dot(delta, delta) + final["radius"] - prior["radius"])
        require(radical_residue > 0, "new center outside radical plane")
        for index in shell:
            if index == len(old):
                continue
            left = 2 * dot(delta, points[index])
            right = (dot(final["center"], final["center"])
                     - dot(prior["center"], prior["center"])
                     + prior["radius"] - final["radius"])
            require(left == right, "old shell sites on radical plane")
        for order in permutations(old):
            bases = positive_bases(list(order) + [z])
            require(len(bases) == 1 and len(old) in bases[0],
                    "unique positive basis containing strict violator")
            orders += 1
        records.append({
            "name": name, "old_support": old, "z": z,
            "old_radius_squared": str(prior["radius"]),
            "new_radius_squared": str(final["radius"]),
            "strict_violator_power": str(power(prior, z)),
            "diameter_squared": diameter2,
            "old_radius_at_least_half_diameter": 4 * prior["radius"] >= diameter2,
            "whole_shell_slots": shell, "positive_bases": positive_bases(points),
            "extra_shell": final["degenerate"],
            "radical_plane_residue_at_new_center": str(radical_residue),
        })
    require(orders == 62 and sum(row["extra_shell"] for row in records) == 1,
            "nonvacuity: five scenes, 62 permutations, extra shell")

    # Outside the theorem: z is on the old shell, not strictly outside.
    old = [(2, 9, 5), (1, 2, 5), (9, 2, 5)]
    z = (8, 9, 5)
    prior = circumball(old)
    require(prior is not None and prior["positive"], "outside-domain old positive")
    require(power(prior, z) == 0, "only the strict-violator premise is absent")
    bases = positive_bases(old + [z])
    require(bases == [(0, 1, 2), (1, 2, 3)], "two positive triangles off domain")
    require(not affine_independent(old + [z]), "off-domain affine dependence")
    outside_domain = {
        "old_support": old, "z": z, "center": [str(v) for v in prior["center"]],
        "radius_squared": str(prior["radius"]), "violator_power": "0",
        "positive_bases": bases,
        "rejected_generalization": "replace strict outside by on-shell",
    }

    # A genuine local pivot can still distinguish order through P and its cap.
    points = [(0, 0, 0), (2, 2, 0), (2, 0, 2), (0, 2, 2)]
    sequence = [(0, 1, 3), (0, 2, 3), (1, 2, 3), (0, 1, 2, 3)]
    reverse = list(reversed(sequence))
    ordered = first_at_cap(points, sequence, 4)
    reversed_order = first_at_cap(points, reverse, 4)
    require(ordered == {"status": "accepted", "forms": 4, "support": [0, 1, 2, 3]},
            "historical retained order takes four forms")
    require(reversed_order == {"status": "accepted", "forms": 1,
                               "support": [0, 1, 2, 3]},
            "reverse order keeps support but changes work")
    cap_pairs = [{"remaining_P": cap,
                  "ordered": first_at_cap(points, sequence, cap),
                  "reversed": first_at_cap(points, reverse, cap)}
                 for cap in range(5)]
    require(cap_pairs[1]["ordered"]["status"] == "exhausted"
            and cap_pairs[1]["reversed"]["status"] == "accepted",
            "order affects cap admission despite unique basis")
    # Analytic native trace with the precise strict pair/violator tie rules.
    pair = None
    distance = -1
    for a, b in combinations(range(len(points)), 2):
        delta = subtract(points[a], points[b])
        candidate_distance = dot(delta, delta)
        if candidate_distance > distance:
            pair, distance = (a, b), candidate_distance
    require(pair == (0, 1) and distance == 8, "first global diameter tie")
    initial = circumball([points[i] for i in pair])
    require(initial is not None and initial["positive"], "native initial pair")
    first_outside = next(i for i, point in enumerate(points) if power(initial, point) > 0)
    require(first_outside == 2, "first strict violator after global diameter")
    first_pivot = first_at_cap(points[:3], [(0, 1, 2)], 1)
    require(first_pivot == {"status": "accepted", "forms": 1, "support": [0, 1, 2]},
            "first native filtered pivot is the old Q of the second pivot")
    triangle = circumball(points[:3])
    require(triangle is not None and triangle["positive"], "native triangle positive")
    second_outside = next(i for i, point in enumerate(points) if power(triangle, point) > 0)
    require(second_outside == 3, "second strict violator in sites order")
    native_trace = {
        "scope": "analytic rational native trajectory, not C++ execution",
        "pair": list(pair), "pair_distance_squared": distance,
        "initial_radius_squared": str(initial["radius"]),
        "first_violator": first_outside,
        "first_violator_power": str(power(initial, points[first_outside])),
        "first_pivot": first_pivot,
        "after_first_radius_squared": str(triangle["radius"]),
        "second_violator": second_outside,
        "second_violator_power": str(power(triangle, points[second_outside])),
        "proposal_forms_before_second_pivot": 2,
        "complete_ordered_proposal_forms": 2 + ordered["forms"],
        "complete_reversed_proposal_forms": 2 + reversed_order["forms"],
        "global_P3_second_pivot": cap_pairs[1],
    }
    order_counterexample = {
        "points": points, "old_support_slots": [0, 1, 2], "strict_violator_slot": 3,
        "scope": "local rational pivot only; no C++ trajectory execution",
        "ordered_candidates": sequence, "reversed_candidates": reverse,
        "nonlimiting_ordered": ordered, "nonlimiting_reversed": reversed_order,
        "cap_pairs": cap_pairs,
        "counts_exclude": "global-diameter initialization and all other local calls",
        "analytic_native_trace": native_trace,
    }
    print(json.dumps({
        "status": "passed", "script_sha256": hashlib.sha256(
            Path(__file__).read_bytes()).hexdigest(),
        "oracle_sha256": hashlib.sha256(
            (AUDIT / "meb_rational_oracle_20260905.py").read_bytes()).hexdigest(),
        "scope": "exact rational fixtures accompanying an analytic proof; no C++",
        "correction": "two positive bases at a strict positive-simplex pivot are impossible",
        "old_base_permutations": orders, "scenes": records,
        "outside_domain_two_bases": outside_domain,
        "order_still_controls_work_and_budget": order_counterexample,
        "public_status": "not_claimed", "gcp": "not_used",
    }, indent=2))


if __name__ == "__main__":
    main()
