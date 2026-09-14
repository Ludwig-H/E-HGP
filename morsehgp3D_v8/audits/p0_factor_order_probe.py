#!/usr/bin/env python3
"""Independent model: plan prefixes need not be short global-tree covers.

Membership counts use an explicit O(m) scan, not a proposed product index.
The root budget limits tentative cover storage, not total traversal work.
No claim is made that current C++ credit strategies produce these permutations.
"""

from __future__ import annotations

from collections import Counter
from itertools import permutations
import json


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def cover(order: tuple[int, ...], selected: set[int], budget: int,
          mutant: str = "") -> dict:
    """Inspect an implicit balanced tree; publish only after preparation ends."""
    require(budget >= 0 and selected <= set(order), "invalid cover request")
    require(len(set(order)) == len(order), "duplicate physical IDs")
    roots: list[tuple[int, int]] = []
    visits = peak = 0
    counts = [0]
    for site in order:
        counts.append(counts[-1] + int(site in selected))

    def visit(first: int, last: int) -> bool:
        nonlocal visits, peak
        visits += 1
        count = counts[last] - counts[first]
        if count == 0:
            return True
        if count == last - first or mutant == "emit_enclosing_node":
            if len(roots) == budget:
                return False
            roots.append((first, last))
            peak = max(peak, len(roots))
            return True
        middle = (first + last) // 2
        return visit(first, middle) and visit(middle, last)

    complete = not order or visit(0, len(order))
    return {"roots": tuple(roots) if complete else (), "visits": visits,
            "peak_roots": peak, "fallback": not complete,
            "preparation_sites": len(order), "emitted_during_preparation": 0}


def execute(order: tuple[int, ...], plan_prefix: tuple[int, ...],
            prepared: dict) -> tuple[int, ...]:
    # The fallback retains the original descriptor; enumerate it exactly once.
    if prepared["fallback"]:
        require(not prepared["roots"], "tentative cover survived fallback")
        return plan_prefix
    return tuple(site for first, last in prepared["roots"]
                 for site in order[first:last])


def judge(order: tuple[int, ...], prefix: tuple[int, ...], prepared: dict,
          budget: int) -> None:
    output = execute(order, prefix, prepared)
    require(Counter(output) == Counter(prefix), "coverage or multiplicity mismatch")
    require(len(output) == len(set(output)), "duplicate output ID")
    require(prepared["peak_roots"] <= budget, "root budget exceeded")
    require(prepared["emitted_during_preparation"] == 0,
            "output before cover or fallback commitment")
    require(prepared["visits"] <= max(0, 2 * len(order) - 1),
            "a physical node was revisited")


def alternating_family() -> list[dict]:
    rows = []
    for size in (4, 8, 16, 32, 64, 128, 256):
        physical = tuple(range(size))
        logical = physical[::2] + physical[1::2]
        prefix = logical[:size // 2]
        # A={0}; odd B_j credits use B_(j-1) strictly inside (0, B_j).
        # Choosing zero on even j is conservative, not a C++ Pool prediction.
        distance = 12 * (size - 1) + 1
        coordinates = tuple(distance + j for j in physical)
        require(coordinates[-1] <= 65535, "fixture left the u16 profile")
        for separation in (8, 10, 12):
            require(distance > separation * (size - 1), "separation failed")
        for j in physical[1::2]:
            z, b = coordinates[j - 1], coordinates[j]
            require(z * (b - z) > 0, "odd credit lacks a strict witness")
        selected = set(prefix)
        global_cover = cover(physical, selected, size)
        own_cover = cover(logical, selected, size)
        judge(physical, prefix, global_cover, size)
        judge(logical, prefix, own_cover, size)
        require(len(global_cover["roots"]) == size // 2,
                "alternating global cover must consist of singletons")
        require(all(last - first == 1 for first, last in global_cover["roots"]),
                "a mixed global subtree was emitted")
        require(global_cover["visits"] == 2 * size - 1,
                "alternating selection must visit the entire global tree")
        require(len(own_cover["roots"]) == 1 and own_cover["visits"] == 3,
                "the same prefix is one subtree in its own order")
        rows.append({"size": size, "max_coordinate": coordinates[-1],
                     "strict_credit_witnesses": size // 2,
                     "global_roots": len(global_cover["roots"]),
                     "global_visits": global_cover["visits"],
                     "own_roots": len(own_cover["roots"]),
                     "own_visits": own_cover["visits"]})
    return rows


def exhaustive_prefixes() -> dict:
    cases = bounded = fallbacks = committed = 0
    for size in range(1, 7):
        physical = tuple(range(size))
        for logical in permutations(physical):
            for length in range(size + 1):
                prefix = logical[:length]
                exact = cover(physical, set(prefix), size)
                judge(physical, prefix, exact, size)
                require(not exact["fallback"], "unbounded cover unexpectedly failed")
                cases += 1
                for budget in sorted({0, 1, 2, size}):
                    prepared = cover(physical, set(prefix), budget)
                    judge(physical, prefix, prepared, budget)
                    require(prepared["fallback"] == (len(exact["roots"]) > budget),
                            "wrong fallback boundary")
                    bounded += 1
                    fallbacks += int(prepared["fallback"])
                    committed += int(not prepared["fallback"])
    require(cases == 5912 and bounded > 20000 and fallbacks > 1000,
            "exhaustive checks became vacuous")
    require(committed > 1000, "bounded covers never committed")
    return {"permutation_prefixes": cases, "bounded_runs": bounded,
            "fallbacks": fallbacks, "committed_covers": committed}


def reject_mutants() -> list[str]:
    physical, prefix = (0, 1, 2, 3), (0, 2)
    mutants = {
        "treat_plan_rank_as_global_rank": cover(physical, set(physical[:2]), 4),
        "emit_enclosing_node": cover(physical, set(prefix), 4, "emit_enclosing_node"),
    }
    rejected = []
    for name, prepared in mutants.items():
        try:
            judge(physical, prefix, prepared, 4)
        except ValueError as error:
            require(str(error) == "coverage or multiplicity mismatch",
                    "mutant rejected for an unrelated reason")
            rejected.append(name)
        else:
            raise ValueError(f"mutant survived: {name}")
    return rejected


def main() -> None:
    result = {"status": "passed", "scope": "independent_permutation_cover_model",
              "alternating_family": alternating_family(),
              "exhaustive": exhaustive_prefixes(), "mutants_rejected": reject_mutants(),
              "limits": ["membership counts require an explicit O(m) preparation",
                         "root budget does not bound traversal work",
                         "no current C++ strategy or hardware performance claim"]}
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
