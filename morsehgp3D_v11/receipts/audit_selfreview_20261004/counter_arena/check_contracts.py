"""Gardes scalaires independantes : aucun moteur, allocation massive ou chrono."""
import json
from math import comb

LIMIT = (1 << 64) - 1
checks = 0


def require(condition, message):
    global checks
    checks += 1
    if not condition:
        raise RuntimeError(message)


def choose(m, q):
    return comb(m, q) if m >= q else 0


def bounds(m):
    support = sum(choose(m, q) for q in range(2, 5))
    return {
        'dominance_tests': choose(m, 2),
        'prefixes': m + support,
        'region_pair_tests': sum((q - 1) * choose(m, q) for q in range(2, 5)),
        'region_line_tests': choose(m, 3) + 3 * choose(m, 4),
        'judged_emitted': support,
        'census_tests_incidences': m * support,
        'q4_candidates_levels': choose(m, 4),
    }


def checked_add(a, b):
    return (False, a) if b > LIMIT - a else (True, a + b)


previous = bounds(0)
for m in range(1025):
    current = bounds(m)
    require(all(0 <= v < (1 << 46) for v in current.values()), 'u64 local bound')
    require(all(current[k] >= previous[k] for k in current), 'monotone cardinal bounds')
    previous = current
for amount in (0, 1, bounds(32)['census_tests_incidences'], bounds(1024)['census_tests_incidences']):
    ok, value = checked_add(LIMIT - amount, amount)
    require(ok and value == LIMIT, 'checked flush exact boundary')
    if amount:
        ok, value = checked_add(LIMIT - amount + 1, amount)
        require(not ok and value == LIMIT - amount + 1, 'checked flush refusal unchanged')
# Scalar DFS model : a ReadyNode job remains owned OUTSIDE the descendant arena.
# Each frame allocates one child list, at most root_count entries. The left child
# is rewound before the right child starts; both must remain complete.
for depth in range(7):
    capacity = 5 * depth
    cursor = peak = visits = 0

    def descend(remaining):
        global cursor, peak, visits
        if not remaining:
            return
        for _child in range(2):
            mark = cursor
            cursor += 5
            visits += 1
            peak = max(peak, cursor)
            require(cursor <= capacity, 'DFS simultaneous capacity')
            descend(remaining - 1)
            cursor = mark
    descend(depth)
    require(cursor == 0 and peak == capacity, 'rewind fully releases views')
    require(visits == (1 << (depth + 1)) - 2, 'all child visits retained')
# Task arenas retained for ALL ordinals need SUM(all), not SUM(W largest).
capacities = [10, 20, 30, 40]
require(sum(sorted(capacities, reverse=True)[:2]) == 70, 'two active tasks bound')
require(sum(capacities) == 100, 'all task reservations coexist')
print(json.dumps({'status': 'PASS', 'checks': checks, 'native_execution': False,
                  'leaf_bound_domain': [0, 1024], 'table': {str(m): bounds(m) for m in (32, 256, 1024)},
                  'scope': 'scalar combinatorial upper bounds, checked addition and DFS lifetime model; no production execution'},
                 sort_keys=True, indent=2))
