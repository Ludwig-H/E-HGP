"""Independent finite models of phase admission and stable anchor selection.

No product import, C++ execution, geometric catalogue or timing qualification.
All conditions remain active under python -O.
"""

from __future__ import annotations

import hashlib
from itertools import product
import json


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def ownership_peak(parts: tuple[int, ...]) -> int:
    """Count live Survivor entries during ordered copy then shard release."""
    pending = sum(parts)
    output = 0
    peak = pending
    for size in parts:
        output += size
        peak = max(peak, pending + output)
        pending -= size
    return peak


def checked_payload(terms: tuple[tuple[int, int], ...], budget: int) -> bool:
    """u64 admission without overflowing multiplication or addition."""
    remaining = budget
    for count, size in terms:
        require(count >= 0 and size > 0, "payload_domain")
        if count > remaining // size:
            return False
        remaining -= count * size
    return True


def memory_models() -> dict[str, object]:
    candidate, survivor, ball = 144, 16, 224
    partitions = 0
    for parts in product(range(6), repeat=4):
        size = sum(parts)
        peak = ownership_peak(parts)
        require(peak == size + max(parts), "shard_live_entry_peak")
        require(peak <= 2 * size, "prefilter_proxy")
        partitions += 1
    boundaries = 0
    for unique in range(101):
        for surviving in range(unique + 1):
            census = unique * candidate + surviving * (survivor + ball)
            terms = ((unique, candidate), (surviving, survivor + ball))
            require(checked_payload(terms, census), "exact_boundary")
            if census:
                require(not checked_payload(terms, census - 1), "minus_one")
            require(unique * (candidate + 2 * survivor) <= 2 * unique * candidate,
                    "prefilter_implied_by_earlier_sort")
            boundaries += 1
    # Reachable in the admission model: previous 2E guard passes at E=U.
    unique, surviving, budget = 100, 80, 32000
    true_census = unique * candidate + surviving * (survivor + ball)
    wrong_census = surviving * (candidate + survivor + ball)
    require(2 * unique * candidate <= budget < true_census, "reachable_census_refusal")
    require(wrong_census <= budget, "candidate_compaction_mutant")
    wide_budget = 38400
    require(max(2 * unique * candidate, unique * (candidate + 2 * survivor),
                unique * (candidate + survivor + ball)) <= wide_budget,
            "new_admission_positive")
    require(unique * (candidate + survivor + 2 * ball) > wide_budget,
            "old_admission_false_refusal")
    maximum = (1 << 64) - 1
    overflow_terms = ((maximum // candidate, candidate), (1, survivor + ball))
    total = sum(n * unit for n, unit in overflow_terms)
    require(total > maximum and not checked_payload(overflow_terms, maximum), "u64_sum_overflow")
    require((total & maximum) <= maximum, "wrapped_sum_mutant")
    return dict(partitions=partitions, boundaries=boundaries,
                census_countermodel=dict(unique=unique, survivors=surviving, budget=budget,
                                         correct=true_census, incorrect=wrong_census),
                old_refusal_new_positive=dict(unique=unique, survivors=unique,
                                             budget=wide_budget, old=60800, new=38400),
                overflow_total=total, scope="logical_entries_not_vector_capacity_or_RSS")


def reference(a: tuple[int, ...], b: tuple[int, ...], need: int) -> list[tuple[int, int, int]]:
    return [(i, j, x + y) for i, x in enumerate(a) for j, y in enumerate(b) if x + y < need]


def deletion_lists(a: tuple[int, ...], b: tuple[int, ...], need: int) -> tuple[list[tuple[int, int, int]], int]:
    """Threshold buckets control deletions, never reorder emitted B indices."""
    requested = {need - x for x in a if x < need}
    buckets: list[list[int]] = [[] for _ in range(need + 1)]
    for j, value in enumerate(b):
        buckets[min(value, need)].append(j)
    previous = [j - 1 for j in range(len(b))]
    following = [j + 1 if j + 1 < len(b) else -1 for j in range(len(b))]
    head = 0 if b else -1
    snapshots: dict[int, list[int]] = {}
    for threshold in range(need, 0, -1):
        for j in buckets[threshold]:
            left, right = previous[j], following[j]
            if left == -1:
                head = right
            else:
                following[left] = right
            if right != -1:
                previous[right] = left
        if threshold in requested:
            indices = []
            cursor = head
            while cursor != -1:
                indices.append(cursor)
                cursor = following[cursor]
            snapshots[threshold] = indices
    result = [(i, j, x + b[j]) for i, x in enumerate(a) if x < need
              for j in snapshots[need - x]]
    return result, sum(map(len, snapshots.values()))


def selection_models() -> dict[str, object]:
    cases, emitted, nonempty = 0, 0, 0
    digest = hashlib.sha256()
    for need in range(1, 5):
        for values in product(range(need + 2), repeat=5):
            a, b = values[:2], values[2:]
            expected = reference(a, b, need)
            actual, stored = deletion_lists(a, b, need)
            clipped_a = tuple(min(x, need) for x in a)
            clipped_b = tuple(min(x, need) for x in b)
            require(actual == expected, "stable_pairs_and_exact_credits")
            require(reference(clipped_a, clipped_b, need) == expected, "global_cap_safe")
            require(stored <= len(expected), "snapshot_output_bound")
            cases += 1
            emitted += len(expected)
            nonempty += bool(expected)
            digest.update(json.dumps(actual, separators=(",", ":")).encode())
    for a, b in (((), ()), ((), (0, 1)), ((0, 1), ())):
        require(deletion_lists(a, b, 2) == ([], 0), "empty_factor")
    # Minimal order mutant: sorting by credit changes B0,B1 into B1,B0.
    ordered = reference((0,), (1, 0), 2)
    score_ordered = sorted(ordered, key=lambda item: item[2])
    require(ordered != score_ordered and sorted(ordered) == sorted(score_ordered), "credit_sort_mutant")
    # Actual collinear u16 geometry, separated at s=8; all three spindles
    # reduce to the same strict between predicate because cross product=0.
    factor_a, factor_b = (0, 1), (100, 101, 102)
    ha = tuple(sum(all((z - a) * (b - z) > 0 for b in factor_b)
                   for z in factor_a if z != a) for a in factor_a)
    hb = tuple(sum(all((z - a) * (b - z) > 0 for a in factor_a)
                   for z in factor_b if z != b) for b in factor_b)
    require(ha == (1, 0) and hb == (0, 1, 2), "fixed_geometric_credits")
    require(201 ** 2 >= 10 ** 2 * 4, "s8_box_separation")
    incorrect_b = tuple(min(x, 2 - ha[0]) for x in hb)
    correct = reference(ha, hb, 2)
    wrong = reference(ha, incorrect_b, 2)
    require((1, 2, 1) in wrong and all(pair[:2] != (1, 2) for pair in correct),
            "row_specific_cap_reused_globally")
    require(nonempty > 0 and emitted > 0, "nonvacuity")
    return dict(cases=cases, nonempty=nonempty, emitted=emitted, digest=digest.hexdigest(),
                order_mutant=dict(correct=ordered, wrong=score_ordered),
                local_cap_mutant=dict(a=factor_a, b=factor_b, ha=ha, hb=hb,
                                      wrong_hb=incorrect_b, correct=correct, wrong=wrong))


def saturated_block_ledger() -> dict[str, int]:
    # First reject three positions by one block certificate, then test one
    # positive point, credit a positive block of ten, and stop before two
    # pending positions. This models distinct original witness populations.
    rejected, scalar, positive, unvisited = 3, 1, 10, 2
    need, already = 3, 1
    increment = min(positive, need - already)
    total = rejected + scalar + positive + unvisited
    wrong = rejected + scalar + increment + unvisited
    require(already + increment == need, "saturation_stops")
    require(total == 16 and wrong == 8, "clipped_credit_is_not_block_population")
    return dict(logical_positions=total, scalar_calls=scalar, rejected_positions=rejected,
                certified_positions=positive, unvisited_positions=unvisited,
                clipped_increment=increment, wrong_ledger=wrong)


def main() -> None:
    print(json.dumps(dict(status="passed", public_status="not_claimed", memory=memory_models(),
                          selection=selection_models(), ledger=saturated_block_ledger(),
                          engine_executed=False, gcp="not_used"),
                     sort_keys=True, separators=(",", ":")))


if __name__ == "__main__":
    main()
