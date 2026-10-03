#!/usr/bin/env python3
"""Independent aligned-point model of qualified co-coverage, audit only.

L_k(r) is a union of intersections of k intervals. Projecting centres onto
the line preserves membership and coverage, so this also describes the
components for aligned points in R^3. No native code or performance claim.
"""
from fractions import Fraction
from itertools import combinations, combinations_with_replacement, product
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.dont_write_bytecode = True
sys.path.insert(0, str(HERE / 'projection' / 'source'))
from hgp11_ref import Definition, judge  # noqa: E402

COUNTS = {}


def require(value, category):
    COUNTS[category] = COUNTS.get(category, 0) + 1
    if not value:
        raise RuntimeError(category)


def components(positions, k, radius):
    """Compute continuous components, independently of Gamma_k/trees."""
    ordered = sorted(positions)
    intervals = []
    for first in range(len(ordered) - k + 1):
        left = ordered[first + k - 1] - radius
        right = ordered[first] + radius
        if left <= right:
            intervals.append((left, right))
    merged = []
    for left, right in sorted(intervals):
        if merged and left <= merged[-1][1]:
            merged[-1] = (merged[-1][0], max(right, merged[-1][1]))
        else:
            merged.append((left, right))
    return tuple(frozenset(i for i, x in enumerate(positions)
                           if left - radius <= x <= right + radius)
                 for left, right in merged)


def partition(covers, n, threshold):
    """Equivalence closure; it intentionally may join FULL components."""
    roots = list(range(n))

    def root(i):
        while roots[i] != i:
            i = roots[i]
        return i

    for cover in covers:
        if len(cover) < threshold:
            continue
        representative = min(cover)
        for i in cover:
            roots[root(i)] = root(representative)
    groups = {}
    for i in range(n):
        groups.setdefault(root(i), set()).add(i)
    return frozenset(frozenset(g) for g in groups.values())


def hierarchy(positions, k, threshold):
    """All changes occur at half a pair span; include closed plateaus."""
    n = len(positions)
    events = sorted({Fraction(0)} | {
        Fraction(abs(a - b), 2) for a, b in combinations(positions, 2)})
    heights = [[None] * n for _ in range(n)]
    previous = frozenset(frozenset([i]) for i in range(n))
    for radius in events:
        current = partition(components(positions, k, radius), n, threshold)
        require(all(any(old <= new for new in current) for old in previous),
                'laminar_cuts')
        for group in current:
            for i in group:
                for j in group:
                    if heights[i][j] is None:
                        heights[i][j] = radius
        previous = current
    require(all(value is not None for row in heights for value in row),
            'eventual_total_cover')
    for i, j, h in product(range(n), repeat=3):
        require(heights[i][h] <= max(heights[i][j], heights[j][h]),
                'ultrametric_triples')
    return heights


def main():
    checked_clouds = 0
    paired_perturbations = 0
    for n in (2, 3, 4, 5):
        for positions in combinations_with_replacement(range(4), n):
            checked_clouds += 1
            reference = Definition([(x, 0, 0) for x in positions])
            previous_order = {}
            for k in range(1, n + 1):
                result = reference.order(k)
                for cut in result.cuts:
                    # Squared levels need not have rational square roots.
                    # Aligned inputs do: every event is half a pair span.
                    candidates = [Fraction(0)] + [
                        Fraction(abs(a - b), denominator)
                        for a, b in combinations(positions, 2)
                        for denominator in (1, 2)]
                    radius = next(r for r in candidates if r * r == cut.level)
                    actual = sorted(sum(1 << i for i in cover)
                                    for cover in components(positions, k, radius))
                    expected = sorted(mask for _, mask, _ in cut.closed)
                    require(actual == expected, 'definition_closed_covers')
                for threshold in range(1, n + 1):
                    heights = hierarchy(positions, k, threshold)
                    if k > 1:
                        before = previous_order[threshold]
                        require(all(before[i][j] <= heights[i][j]
                                    for i in range(n) for j in range(n)),
                                'vertical_refinement')
                    previous_order[threshold] = heights
                    if threshold > 1:
                        require(all(previous_threshold[i][j] <= heights[i][j]
                                    for i in range(n) for j in range(n)),
                                'threshold_refinement')
                    previous_threshold = heights
                    reversed_heights = hierarchy(tuple(reversed(positions)), k, threshold)
                    require(all(heights[i][j] == reversed_heights[n - 1 - i][n - 1 - j]
                                for i in range(n) for j in range(n)),
                            'permutation_equivariance')
                    shifted = hierarchy(tuple(7 + 3 * x for x in positions), k, threshold)
                    require(all(shifted[i][j] == 3 * heights[i][j]
                                for i in range(n) for j in range(n)),
                            'translation_scaling_equivariance')
                    # Unequal movements, identity preserved through any crossing.
                    changed = tuple(x + (1 if i % 2 else -1)
                                    for i, x in enumerate(positions))
                    perturbed = hierarchy(changed, k, threshold)
                    require(all(abs(heights[i][j] - perturbed[i][j]) <= 1
                                for i in range(n) for j in range(n)),
                            'one_lipschitz_paired_radii')
                    paired_perturbations += 1

    pair_case = (0, 2, 100, 102)
    pair_heights = hierarchy(pair_case, 2, 2)
    lost_heights = hierarchy(pair_case, 2, 3)
    require(pair_heights[0][1] == 1 and pair_heights[2][3] == 1,
            'isolated_pairs_threshold_two')
    require(lost_heights[0][1] == 50 and lost_heights[2][3] == 50,
            'isolated_pairs_lost_threshold_three')
    # Symmetric boundary causes premature connection, even at m=3.
    boundary = (0, -6, -5, 5, 6)
    radius = Fraction(3)
    cover = components(boundary, 2, radius)
    require(len(cover) == 2 and all(len(c) == 3 for c in cover),
            'boundary_two_qualified_components')
    block = partition(cover, len(boundary), 3)
    require(block == frozenset([frozenset(range(5))]),
            'boundary_premature_closure')
    # Wrong substitute: retaining only each point's first covering components.
    # At m=3 these two first pair covers would never recover the later triples.
    output = {
        'scope': 'exact aligned-point audit model; no native/product qualification',
        'clouds': checked_clouds,
        'paired_perturbations': paired_perturbations,
        'checks': COUNTS,
        'pairs': {'m2_within': str(pair_heights[0][1]),
                  'm3_within': str(lost_heights[0][1])},
        'boundary': {'positions': boundary, 'radius': str(radius),
                     'covers': [sorted(c) for c in cover],
                     'closure': [sorted(g) for g in block]},
    }
    print(json.dumps(output, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
