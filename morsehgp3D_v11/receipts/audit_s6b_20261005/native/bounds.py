#!/usr/bin/env python3
"""Independent integer bounds for the prepared S6b domain; no native execution.

The ABI sizes below are declared assumptions from the source/report contract.
This program neither compiles nor executes sizeof, and uses no assertion.
"""
import json
import math
import sys


def choose(n, k):
    return math.comb(n, k) if 0 <= k <= n else 0


def calculate():
    names = ("kparties", "compressed", "cofaces_bound", "support_cofaces", "gabriel_bound")
    maxima = {name: {"value": 0, "shape": None} for name in names}
    shapes = 0
    for p in range(12):
        for m in range(2, 25):
            for q in range(2, min(m, 4) + 1):
                for k in range(1, 13):
                    if not p + q - 1 <= k <= p + m:
                        continue
                    shapes += 1
                    t = k - p
                    values = {
                        "kparties": choose(p + m, k),
                        "compressed": choose(m, t),
                        "cofaces_bound": choose(p + m, k + 1),
                        "gabriel_bound": choose(m, t + 1),
                    }
                    for arity in range(2, min(4, m) + 1):
                        value = choose(p + m - arity, k + 1 - arity)
                        if value > maxima["support_cofaces"]["value"]:
                            maxima["support_cofaces"] = {"value": value, "shape": [p, m, q, k, arity]}
                    for name, value in values.items():
                        if value > maxima[name]["value"]:
                            maxima[name] = {"value": value, "shape": [p, m, q, k]}

    u32_max = (1 << 32) - 1
    u64_max = (1 << 64) - 1
    nodes, balls, workers = u32_max - 1, u32_max, 256
    prior = balls * choose(24, 12)
    supports = balls * 12926
    # Declared ABI assumptions; no native sizeof is evaluated by this script.
    support_bytes, ball_bytes, ledger_bytes = 20, 40, 64
    per_worker = ledger_bytes + 8 * (1 << (24 - 6)) + support_bytes * 12926
    first = (
        8 * nodes + 8 * (nodes + 1) + ball_bytes * balls + 16 * (balls + 1)
        + 4 * prior + 4 * balls + workers * per_worker
    )
    second = (support_bytes + 4) * supports
    return {
        "kind": "independent exact-integer domain bound, stdlib math.comb; no native execution",
        "conditions": [
            "prepared immutable OrderTree; every published shell m<=24 after prepass",
            "I1-I4: every ant branch has at least one strict trace; prior is stored only for merge",
            "N < UINT32_MAX; B <= UINT32_MAX; 1<=W<=256",
            "Declared ABI assumptions: sizeof(Support)=20, sizeof(Ball)=40, sizeof(SupportLedger)=64; native source formulas use sizeof, not executed here",
        ],
        "shapes_enumerated": shapes,
        "maxima": maxima,
        "upper_dimensions": {"nodes": nodes, "balls": balls, "prior": prior, "supports": supports, "workers": workers},
        "upper_bytes": {"per_worker": per_worker, "first": first, "second": second, "first_plus_second": first + second},
        "all_counts_below_u32": all(value["value"] <= u32_max for value in maxima.values()),
        "all_admission_sums_below_u64": first + second <= u64_max,
        "extra_second_guard_inactive_in_prepared_domain": supports <= (u64_max // support_bytes) // (support_bytes + 4),
        "limitations": [
            "Not a proof that an arbitrary forged/corrupted OrderTree satisfies I1-I4.",
            "Does not execute native sizeof or test profile-specific primitives.",
        ],
    }


def main():
    result = calculate()
    print(json.dumps(result, indent=2))
    checks = (
        result["shapes_enumerated"] == 3896,
        result["all_counts_below_u32"],
        result["all_admission_sums_below_u64"],
        result["extra_second_guard_inactive_in_prepared_domain"],
    )
    return 0 if all(checks) else 1


if __name__ == "__main__":
    sys.exit(main())
