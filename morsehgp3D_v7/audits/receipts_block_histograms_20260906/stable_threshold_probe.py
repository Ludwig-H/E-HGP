"""Finite combinatorial check of stable threshold lists; no product imports.

The inputs are nonnegative credit vectors, not geometric clouds or engine runs.
The source lane loop remains exterior to the transformation examined here.
"""
import json

CreditRow = tuple[int, int, int]
Counts = dict[str, int]
Result = tuple[list[CreditRow], Counts]


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def validate(ha: list[int], hb: list[int], need: int) -> None:
    require(1 <= need <= 9 and bool(ha) and bool(hb), "finite_model_domain")
    require(all(type(value) is int and value >= 0 for value in ha + hb), "nonnegative_credits")


def double_loop(ha: list[int], hb: list[int], need: int) -> Result:
    validate(ha, hb, need)
    emitted: list[CreditRow] = []
    counts = {"killed_rows": 0, "killed_threshold": 0, "visited": 0}
    for a, credit_a in enumerate(ha):
        if credit_a >= need:
            counts["killed_rows"] += len(hb)
            continue
        for b, credit_b in enumerate(hb):
            if credit_a + credit_b >= need:
                counts["killed_threshold"] += 1
                continue
            counts["visited"] += 1
            emitted.append((a, b, credit_a + credit_b))
    return emitted, counts


def stable_lists(ha: list[int], hb: list[int], need: int,
                 saturate: bool, group_a: bool = False) -> Result:
    validate(ha, hb, need)
    a_values = [min(value, need) for value in ha] if saturate else ha
    b_values = [min(value, need) for value in hb] if saturate else hb
    thresholds = {need - value for value in a_values if value < need}
    lists = {t: [b for b, value in enumerate(b_values) if value < t] for t in thresholds}
    require(len(lists) <= need, "at_most_need_lists")
    require(sum(map(len, lists.values())) <= need * len(hb), "list_entry_bound")
    # Production-equivalent order is the original A order. Sorting by credit
    # is a deliberate mutant: each B list stays stable but A is reordered.
    order = sorted(range(len(ha)), key=lambda a: a_values[a]) if group_a else list(range(len(ha)))
    emitted: list[CreditRow] = []
    counts = {"killed_rows": 0, "killed_threshold": 0, "visited": 0}
    for a in order:
        credit_a = a_values[a]
        if credit_a >= need:
            counts["killed_rows"] += len(hb)
            continue
        admitted = lists[need - credit_a]
        counts["killed_threshold"] += len(hb) - len(admitted)
        counts["visited"] += len(admitted)
        emitted.extend((a, b, credit_a + b_values[b]) for b in admitted)
    return emitted, counts


def equivalent(reference: Result, candidate: Result) -> None:
    require(reference[1] == candidate[1], "row_threshold_visit_accounting")
    require(reference[0] == candidate[0], "ordered_survivor_credit_sequence")


def corpus(need: int) -> list[tuple[str, list[int], list[int]]]:
    return [
        ("all_zero", [0, 0], [0, 0, 0]),
        ("boundary_and_saturation", [0, need - 1, need, need + 2], [0, need - 1, need, need + 3]),
        ("interleaved_A_credits", [need - 1, 0, need - 1, 0], [need - 1, 0, need, need - 1]),
        ("all_rows_killed", [need, need + 1], [0, need - 1, need + 1]),
        ("all_columns_killed", [0, 0], [need, need + 1]),
        ("intermediate_credits", [need // 2, 0, need - 1], [0, need // 2, need - 1, need + 1]),
    ]


def main() -> None:
    rows: list[dict[str, object]] = []
    totals = {"killed_rows": 0, "killed_threshold": 0, "visited": 0}
    saturated_values = 0
    for need in range(1, 10):
        for name, ha, hb in corpus(need):
            reference = double_loop(ha, hb, need)
            equivalent(reference, stable_lists(ha, hb, need, False))
            equivalent(reference, stable_lists(ha, hb, need, True))
            require(sum(reference[1].values()) == len(ha) * len(hb), "partition_of_all_anchor_pairs")
            require(all(credit == ha[a] + hb[b] and credit < need for a, b, credit in reference[0]),
                    "exact_survivor_endpoint_credit")
            for key in totals:
                totals[key] += reference[1][key]
            saturated_values += sum(value > need for value in ha + hb)
            rows.append({"name": name, "need": need, "ha": ha, "hb": hb,
                         "counts": reference[1], "survivors_in_original_order": reference[0]})
    require(len(rows) == 54 and all(totals.values()) and saturated_values > 0, "nonvacuum")
    ha, hb, need = [1, 0], [0, 1], 2
    reference = double_loop(ha, hb, need)
    mutant = stable_lists(ha, hb, need, True, group_a=True)
    require(reference[1] == mutant[1] and sorted(reference[0]) == sorted(mutant[0]), "mutant_preserves_multiset")
    rejection = None
    try:
        equivalent(reference, mutant)
    except ValueError as error:
        rejection = str(error)
    require(rejection == "ordered_survivor_credit_sequence", "targeted_grouping_mutant_rejected")
    require(reference[0][0] == (0, 0, 1) and mutant[0][0] == (1, 0, 0), "different_first_survivor")
    result = {
        "schema": "mhgp7-independent-stable-threshold-probe-v1", "status": "passed",
        "scope": "54 finite credit models; ordered anchors and accounting only, no geometry or engine qualification",
        "public_status": "not_claimed", "engine_calls": 0, "product_imports": 0, "gcp": "not_used",
        "need_domain": [1, 9], "case_count": len(rows), "positive_comparisons": 2 * len(rows),
        "totals": totals, "values_strictly_clipped": saturated_values, "cases": rows,
        "group_A_mutant": {"ha": ha, "hb": hb, "need": need, "rejection": rejection,
                           "same_multiset_and_counts": True, "reference_order": reference[0], "mutant_order": mutant[0]},
        "limits": ["No lane-loop reordering is proposed.",
                   "No corner-predicate evaluations or P_factor costs are measured.",
                   "The first-survivor mutant illustrates prefix loss; no raw-candidate cap or throttle is executed."],
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
