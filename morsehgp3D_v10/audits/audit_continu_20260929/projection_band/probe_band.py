"""Exact 1D k=2 audit fixture, embedded in 3D; no product or benchmark seeds.

Enumerates all pairs on at most three sites. This is an independent mathematical
oracle, not a proposed scalable candidate generator. Writes JSON to stdout only.
"""

from fractions import Fraction as F
from itertools import combinations, product
import json


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def components(points, radius):
    intervals = []
    for a, b in combinations(points, 2):
        low, high = max(a, b) - radius, min(a, b) + radius
        if low <= high:
            intervals.append((low, high))
    merged = []
    for low, high in sorted(intervals):
        if merged and low <= merged[-1][1]:
            merged[-1] = (merged[-1][0], max(merged[-1][1], high))
        else:
            merged.append((low, high))
    return merged


def candidates(points, eta):
    answer = []
    for i, x in enumerate(points):
        pairs = [((i, j), (x + y) / 2, abs(x - y) / 2)
                 for j, y in enumerate(points) if i != j]
        first = min(a[2] for a in pairs)
        answer.append([a for a in pairs if a[2] <= (1 + eta) * first])
    return answer


def join(points, anchors):
    activation = max(a[2] for a in anchors)
    centers = [a[1] for a in anchors]
    events = {activation}
    events.update(abs(a - b) / 2 for a, b in combinations(points, 2))
    for radius in sorted(r for r in events if r >= activation):
        if any(low <= min(centers) and max(centers) <= high
               for low, high in components(points, radius)):
            return radius
    raise RuntimeError("finite cloud must eventually merge")


def view(points, eta):
    points = tuple(F(x) for x in points)
    require(len(set(points)) == len(points), "unique sites required")
    selected = candidates(points, F(eta))
    entries = [join(points, a) for a in selected]
    heights = {(i, j): join(points, selected[i] + selected[j])
               for i, j in combinations(range(len(points)), 2)}
    return points, selected, entries, heights


def pair_height(heights, i, j):
    return F(0) if i == j else heights[tuple(sorted((i, j)))]


def partition(points, heights, radius):
    unseen = set(range(len(points)))
    blocks = []
    while unseen:
        i = min(unseen)
        block = {j for j in unseen if pair_height(heights, i, j) <= radius}
        unseen -= block
        blocks.append(sorted(block))
    return blocks


def summarize(v):
    points, selected, entries, heights = v
    return {
        "points": [str(p) for p in points],
        "candidate_ids": [[list(a[0]) for a in row] for row in selected],
        "entries": [str(t) for t in entries],
        "pair_heights": {f"{i},{j}": str(t) for (i, j), t in heights.items()},
    }


def check_ultrametric(v):
    points, _, entries, heights = v
    for i, j, k in product(range(len(points)), repeat=3):
        require(pair_height(heights, i, k) <= max(pair_height(heights, i, j),
                                               pair_height(heights, j, k)),
                "strong triangle inequality")
    for (i, j), height in heights.items():
        require(height >= max(entries[i], entries[j]), "activation respected")


def main():
    eta = F(1, 4)
    small = view((0, 8, 18), F(0))
    large = view((0, 8, 18), eta)
    require(small[2][1] == 4 and large[2][1] == 9, "delayed entry witness")
    require(partition(small[0], small[3], F(6)) == [[0, 1], [2]], "small band cut")
    require(partition(large[0], large[3], F(6)) == [[0], [1], [2]], "large band cut")
    for r in sorted(set(small[3].values()) | set(large[3].values())):
        coarse = [set(a) for a in partition(small[0], small[3], r)]
        require(all(any(set(a) <= b for b in coarse)
                    for a in partition(large[0], large[3], r)), "refinement in eta")

    threshold_low = view((0, 8000, 17999), eta)
    threshold_high = view((0, 8000, 18001), eta)
    require(threshold_low[2][1] == F(17999, 2), "lower threshold entry")
    require(threshold_high[2][1] == 4000, "upper threshold entry")

    original = view((0, 8, 19), eta)
    epsilon = F(1, 10)
    gaps = []
    for i, x in enumerate(original[0]):
        radii = [abs(x - y) / 2 for j, y in enumerate(original[0]) if j != i]
        first = min(radii)
        gaps.extend(abs(r - (1 + eta) * first) for r in radii)
    margin = min(gaps)
    require(margin > (2 + eta) * epsilon, "strict local margin certificate")
    signatures = [[a[0] for a in row] for row in original[1]]
    maximum_error = F(0)
    tests = 0
    all_views = [small, large, threshold_low, threshold_high, original]
    for shifts in product((-epsilon, F(0), epsilon), repeat=3):
        perturbed = view(tuple(x + d for x, d in zip(original[0], shifts)), eta)
        require([[a[0] for a in row] for row in perturbed[1]] == signatures,
                "candidate identities retained")
        errors = [abs(x - y) for x, y in zip(original[2], perturbed[2])]
        errors.extend(abs(original[3][pair] - perturbed[3][pair]) for pair in original[3])
        maximum_error = max(maximum_error, *errors)
        require(max(errors) <= 2 * epsilon, "local radius bound 2 epsilon")
        all_views.append(perturbed)
        tests += 1
    for v in all_views:
        check_ultrametric(v)

    result = {
        "schema": "mhgp10_k2_band_oracle_v1",
        "exact_arithmetic": "Fraction",
        "scope": "three_site_1d_examples_embedded_in_3d_not_product",
        "eta": str(eta),
        "refinement_witness": {"eta0": summarize(small), "eta_quarter": summarize(large)},
        "threshold_discontinuity": {
            "below": summarize(threshold_low), "above": summarize(threshold_high),
            "point_displacement": "2",
            "middle_entry_jump": str(threshold_low[2][1] - threshold_high[2][1]),
        },
        "local_certificate": {
            "reference": summarize(original), "epsilon": str(epsilon),
            "minimum_margin": str(margin), "required_margin": str((2 + eta) * epsilon),
            "perturbations": tests, "maximum_error": str(maximum_error),
            "allowed_error": str(2 * epsilon),
        },
        "views_checked_ultrametric": len(all_views),
        "status": "pass",
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
