"""Exact local fixtures for two proposed pivot filters; no C++ execution."""

from __future__ import annotations

import hashlib
from itertools import combinations, permutations
import json
from math import comb
from pathlib import Path

from meb_rational_oracle_20260905 import circumball, dot, expected_meb, power, subtract


HERE = Path(__file__).resolve().parent
HELPER = HERE / "receipts_meb_dual_20260905/inputs/dual_pivot.hpp"
HELPER_SHA = "0645aa00add4d4cb387861b8f6dbd4fa0734ba5b4f3ad712caad8886b3541c2d"


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def candidates(points: list[tuple], require_last: bool = False,
               omit_pairs: bool = False) -> tuple[list[tuple], int]:
    accepted, tried = [], 0
    for q in range(3 if omit_pairs else 2, min(4, len(points)) + 1):
        for support in combinations(range(len(points)), q):
            if require_last and len(points) - 1 not in support:
                continue
            tried += 1
            ball = circumball([points[i] for i in support])
            if ball and ball["positive"] and all(power(ball, p) <= 0 for p in points):
                accepted.append(support)
    return accepted, tried


def main() -> None:
    require(hashlib.sha256(HELPER.read_bytes()).hexdigest() == HELPER_SHA,
            "native helper premise")
    scenes = [
        ("q2_to_q3", [(0, 0, 0), (2, 2, 0)], (2, 0, 2), True, 3),
        ("q3_to_q4", [(0, 0, 0), (2, 2, 0), (2, 0, 2)], (0, 2, 2), True, 4),
        ("q4_to_q3_with_diameter_bound",
         [(30, 30, 30), (30, 10, 10), (10, 30, 10), (10, 10, 30)],
         (20, 20, 38), True, 3),
        ("q4_replacement_general",
         [(1, 1, 1), (3, 3, 1), (3, 1, 3), (1, 3, 3)], (0, 0, 0), False, 4),
        ("extra_shell_general", [(2, 1, 1), (1, 2, 1), (1, 1, 2)],
         (0, 1, 1), False, 2),
    ]
    results, orders = [], 0
    for name, support, z, diameter_bound, final_q in scenes:
        prior = expected_meb(support)
        require(prior["q"] == len(support) and prior["positive"], "positive old base")
        require(power(prior, z) > 0, "strict violator")
        points = support + [z]
        diameter2 = max(dot(subtract(a, b), subtract(a, b)) for a, b in combinations(points, 2))
        require((4 * prior["radius"] >= diameter2) == diameter_bound, "diameter premise")
        expected = expected_meb(points)
        require(expected["radius"] > prior["radius"] and expected["q"] == final_q,
                "radius increase and expected arity")
        for order in permutations(support):
            sites = list(order) + [z]
            all_valid, _ = candidates(sites)
            with_z, _ = candidates(sites, require_last=True)
            require(bool(all_valid) and all_valid == with_z, "all accepted bases retain z")
            if diameter_bound:
                no_pairs, _ = candidates(sites, require_last=True, omit_pairs=True)
                require(all_valid == no_pairs, "no accepted pair after diameter bound")
            orders += 1
        results.append({"name": name, "old_support": support, "z": z,
                        "old_radius_squared": str(prior["radius"]),
                        "outside_power": str(power(prior, z)),
                        "diameter_squared": diameter2,
                        "pair_filter_premise": diameter_bound,
                        "new_support_slots": expected["support"],
                        "new_radius_squared": str(expected["radius"]),
                        "extra_shell": expected["degenerate"]})
    require(orders == 62 and sum(r["extra_shell"] for r in results) == 1,
            "positive and extra-shell floors")
    # These are false generalizations of the proposed filters, not C++ mutants.
    counterexamples = []
    for name, sites, last, pairs in [
        ("interior_is_not_strict", [(0, 0, 0), (4, 0, 0), (2, 0, 0)], True, False),
        ("shell_is_not_strict", [(0, 2, 0), (4, 2, 0), (2, 4, 0)], True, False),
        ("nonmaximal_pair", [(0, 0, 0), (2, 0, 0), (5, 0, 0)], True, True),
    ]:
        valid, _ = candidates(sites)
        filtered, _ = candidates(sites, require_last=last, omit_pairs=pairs)
        require(bool(valid) and not filtered, "false generalization refuted: " + name)
        counterexamples.append({"name": name, "points": sites, "lost_bases": valid})
    counts = []
    for old_q in (2, 3, 4):
        n = old_q + 1
        original = sum(comb(n, q) for q in range(2, min(n, 4) + 1))
        with_z = sum(comb(old_q, q - 1) for q in range(2, min(n, 4) + 1))
        both = sum(comb(old_q, q - 1) for q in range(3, min(n, 4) + 1))
        counts.append({"old_q": old_q, "original": original, "z_only": with_z, "both": both})
    require([r["both"] for r in counts] == [1, 4, 10], "candidate ceilings")
    print(json.dumps({"status": "passed", "source_sha256": HELPER_SHA,
                      "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                      "oracle_sha256": hashlib.sha256((HERE / "meb_rational_oracle_20260905.py").read_bytes()).hexdigest(),
                      "scope": "Exact rational local proof fixtures; no modified C++, runtime qualification or performance claim",
                      "old_base_permutations": orders, "scenes": results,
                      "counterexamples": counterexamples, "candidate_ceilings": counts,
                      "max_forms_at_16_pivots": {"original": 401, "simple": 161, "refined": 146},
                      "public_status": "not_claimed", "gcp": "not_used"}, indent=2))


if __name__ == "__main__":
    main()
