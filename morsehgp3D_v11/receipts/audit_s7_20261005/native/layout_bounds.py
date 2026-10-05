#!/usr/bin/env python3
"""Reproducible S7 format and aggregate bounds, standard-library integers only.

No product import, native execution, sizeof, input cloud or dataset is required.
Column widths are prescribed by MHGP11SP v1, not by native ABI assumptions.
"""
import json
import math
import sys


def choose(n, k):
    return math.comb(n, k) if 0 <= k <= n else 0


def pad8(size):
    return (size + 7) & ~7


def calculate():
    u32_max, u64_max = (1 << 32) - 1, (1 << 64) - 1
    shapes = 0
    maxima = {name: 0 for name in ("kparties", "cofaces", "strict_traces_bound")}
    for p in range(12):
        for m in range(2, 25):
            for q in range(2, min(4, m) + 1):
                for k in range(1, 13):
                    if not p + q - 1 <= k <= p + m:
                        continue
                    shapes += 1
                    maxima["kparties"] = max(maxima["kparties"], choose(p + m, k))
                    # Vandermonde: sum_j C(p,K+1-j)N_j <= C(p+m,K+1).
                    maxima["cofaces"] = max(maxima["cofaces"], choose(p + m, k + 1))
                    maxima["strict_traces_bound"] = max(maxima["strict_traces_bound"], choose(m, k - p))

    max_supports = sum(choose(24, arity) for arity in range(2, 5))
    n = nodes = u32_max - 1
    balls = u32_max
    supports = balls * max_supports
    arities = 4 * supports
    # Every distinct prior branch has at least one strict trace; only merges store prior.
    prior = balls * maxima["strict_traces_bound"]
    sizes = {
        "n": n, "N": nodes, "B": balls, "S": supports, "Z": arities, "A": prior,
        "support_count_per_ball": max_supports,
        "prior_count_per_ball": maxima["strict_traces_bound"],
        "ball_count_per_node": balls,
    }
    sites_at = 136
    nodes_at = sites_at + 4 * pad8(4 * n)
    balls_at = nodes_at + 3 * pad8(4 * nodes) + pad8(nodes)
    supports_at = balls_at + 2 * pad8(4 * balls) + pad8(2 * balls) + 3 * pad8(balls)
    prior_at = supports_at + pad8(supports) + pad8(4 * arities)
    size = prior_at + pad8(4 * prior)
    layout = dict(SITES=sites_at, NODES=nodes_at, BALLS=balls_at, SUPPORTS=supports_at, PRIOR=prior_at, total=size)
    aggregates = {
        "kparties_sum": balls * maxima["kparties"],
        "cofaces_sum": balls * maxima["cofaces"],
        "roles_sum": balls,
        "arities_sum": supports,
    }
    # Guard every expression BEFORE the native pad8 adds 7.
    padded_arguments = [4 * n, 4 * nodes, nodes, 4 * balls, 2 * balls, balls, supports, 4 * arities, 4 * prior]
    checks = {
        "forms_3896": shapes == 3896,
        "support_count_fits_u16": max_supports <= (1 << 16) - 1,
        "prior_and_ball_counts_fit_u32": max(sizes["prior_count_per_ball"], balls) <= u32_max,
        "pad8_arguments_plus7_fit_u64": max(padded_arguments) <= u64_max - 7,
        "all_layout_offsets_and_total_fit_u64": max(layout.values()) <= u64_max,
        "all_manifest_aggregate_sums_fit_u64": max(aggregates.values()) <= u64_max,
    }
    return {
        "conditions": [
            "prepared immutable S6b hierarchy; all published shells m<=24 and 1<=K<=12",
            "0<=p<=11; 2<=qmin<=4; qmin<=m; p+qmin-1<=K<=p+m",
            "N<UINT32_MAX; n<UINT32_MAX; B<=UINT32_MAX",
            "each ant branch has at least one strict trace (I1-I4); prior stored only for merges",
            "all support arities are 2..4; each Q_b has at most C(24,2)+C(24,3)+C(24,4) supports",
            "format column widths are normative bytes; no native sizeof evaluated",
        ],
        "forms_enumerated": shapes,
        "per_ball_maxima": maxima,
        "upper_dimensions": sizes,
        "layout_upper_bytes": layout,
        "manifest_aggregate_upper_values": aggregates,
        "checks": checks,
        "qualification": "pure integer bounds only; no native, sanitizer, GPU, dataset or performance execution",
    }


def main():
    result = calculate()
    print(json.dumps(result, indent=2))
    return 0 if all(result["checks"].values()) else 1


if __name__ == "__main__":
    sys.exit(main())
