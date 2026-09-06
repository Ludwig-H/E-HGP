"""Fixed countermodels; no product import, traversal or C++ claim."""

from __future__ import annotations

from itertools import combinations, product
import json
import sys

Point = tuple[int, int, int]
Box = tuple[Point, Point]


def require(ok: bool, message: str) -> None:
    if not ok:
        raise ValueError(message)


def dot(a: Point, b: Point) -> int:
    return sum(x * y for x, y in zip(a, b))


def sub(a: Point, b: Point) -> Point:
    return a[0] - b[0], a[1] - b[1], a[2] - b[2]


def diametral_count(points: tuple[Point, ...], i: int, j: int) -> int:
    return sum(dot(sub(z, points[i]), sub(points[j], z)) > 0
               for k, z in enumerate(points) if k not in (i, j))


def hmax4(a: Point, b: Point, box: Box) -> int:
    result = 0
    for i in range(3):
        center2 = a[i] + b[i]
        z2 = min(max(center2, 2 * box[0][i]), 2 * box[1][i])
        result += (b[i] - a[i]) ** 2 - (z2 - center2) ** 2
    return result


def cross_bounds(a: Point, b: Point, box: Box) -> tuple[int, int]:
    d = sub(b, a)
    w = tuple((box[0][i] - a[i], box[1][i] - a[i]) for i in range(3))
    low = high = 0
    for j, k in ((1, 2), (2, 0), (0, 1)):
        first = sorted(d[j] * value for value in w[k])
        second = sorted(d[k] * value for value in w[j])
        lo, hi = first[0] - second[1], first[1] - second[0]
        near = 0 if lo <= 0 <= hi else min(abs(lo), abs(hi))
        low += near * near
        high += max(abs(lo), abs(hi)) ** 2
    return low, high


def h_xi(a: Point, b: Point, z: Point) -> tuple[int, int]:
    d, w = sub(b, a), sub(z, a)
    cross = (d[1] * w[2] - d[2] * w[1],
             d[2] * w[0] - d[0] * w[2],
             d[0] * w[1] - d[1] * w[0])
    return dot(w, sub(b, z)), dot(cross, cross)


def noncredit_models() -> dict[str, object]:
    a, b = (0, 0, 0), (100, 0, 0)
    box = ((1, 4, 0), (2, 5, 1))
    corners = tuple(product(*zip(*box)))
    m4 = hmax4(a, b, box)
    xi_min, _ = cross_bounds(a, b, box)
    require(m4 == 720 and xi_min == 160000, "fixed noncredit interval values")
    require(min(h_xi(a, b, z)[0] for z in corners) == 73,
            "whole witness block strictly inside W2")
    require(198 ** 2 + 5 ** 2 + 1 >= 100 * (2 ** 2 + 5 ** 2 + 1),
            "original A={a}+eight Z corners and B={b} satisfy s8")
    for t in (3, 2):
        require(t * m4 * m4 <= 16 * xi_min, "block noncredit certificate")
        require(all(t * h_xi(a, b, z)[0] ** 2 <= h_xi(a, b, z)[1]
                    for z in corners), "all eight corners fail the strict lane")
    for aa, bb, zz, t in (
        ((0, 1, 1), (2, 0, 0), (1, 1, 0), 3),
        ((0, 0, 0), (2, 2, 2), (1, 1, 0), 2),
    ):
        mm, (xx, _) = hmax4(aa, bb, (zz, zz)), cross_bounds(aa, bb, (zz, zz))
        require(mm > 0 and t * mm * mm == 16 * xx,
                "exact boundary is correctly rejected as noncredit")
    mixed = ((1, 0, 0), (2, 5, 1))
    mm = hmax4(a, b, mixed)
    lo, hi = cross_bounds(a, b, mixed)
    require(lo == 0, "mixed block has no positive Xi lower bound")
    for t in (3, 2):
        require(t * mm * mm > 16 * lo, "mixed block cannot be rejected")
        require(t * mm * mm <= 16 * hi, "upper-bound-as-lower mutant rejects it")
        h, xi = h_xi(a, b, mixed[0])
        require(h > 0 and t * h * h > xi, "mutant loses an actual witness")
    return dict(Hmin=73, M4=m4, Xi_min=xi_min, witness_corners=8,
                W2_cannot_reject=True, q3_q4_block_rejected=True,
                strict_boundary_controls=2, upper_as_lower_mutants_refuted=2,
                general_continuous_proof_replaced_by_corners=False)


def main() -> dict[str, object]:
    require(len(sys.argv) == 1, "no arguments")
    # Fictitious coupled counter: componentwise domination for every mask.
    cheap = {1: (2, 0), 2: (0, 0), 3: (2, 0)}
    full = {1: (2, 0), 2: (0, 2), 3: (2, 1)}
    for mask in (1, 2, 3):
        for lane in (0, 1):
            if mask & (1 << lane):
                require(0 <= cheap[mask][lane] <= full[mask][lane] <= 2,
                        "domination holds even in countermodel")
    require(cheap[3][0] == 2 and cheap[3][1] < 2,
            "cheap pass removes first lane only")
    require(full[2][1] == 2 and full[3][1] < 2,
            "two-pass kills second lane; single-pass keeps it")
    require(full[2][1] != full[3][1], "mask invariance is the missing premise")

    minimal = ((0, 0, 0), (1, 0, 0), (2, 0, 0))
    pairs = tuple(combinations(range(3), 2))
    counts = tuple(diametral_count(minimal, i, j) for i, j in pairs)
    require(counts == (0, 1, 0), "fixed strict diametral census")
    require(dot(sub(minimal[1], minimal[0]), sub(minimal[2], minimal[1])) == 1,
            "extreme pair has a strictly interior witness")
    require(len(set(minimal)) == 3, "distinct u16 positions")
    # Any distinct singleton boxes satisfy separation s8: their radii vanish.
    require(all(dot(sub(minimal[j], minimal[i]), sub(minimal[j], minimal[i])) > 0
                for i, j in pairs), "singleton terminal separation")
    expected = dict(mask=1, h2=2, emitted_mass=3, killed_mass=0, cores=counts)
    zero_copy_mutant = (0, 0, 0)
    require(all(c < expected["h2"] for c in counts + zero_copy_mutant),
            "old local below-threshold predicate accepts both")
    require(zero_copy_mutant != counts and zero_copy_mutant[1] != 1,
            "fixed positive-core expectation rejects omitted copy")
    existing = tuple((x, 0, 0) for x in (0, 10, 20, 30, 40))
    require(diametral_count(existing, 0, 2) == 1,
            "existing scene1 leaf pair -1,-3 also has core one")
    return {
        "schema": "mhgp7-terminal-count-fixed-countermodels-v1",
        "status": "passed",
        "domination_without_mask_invariance": {
            "thresholds": [2, 2], "cheap": cheap, "full": full,
            "two_pass_live_mask": 0, "one_pass_live_mask": 2,
            "scope": "fictitious counter, not v7 behavior",
        },
        "positive_q2_core": {
            "points": minimal, "pairs": pairs, "expected": expected,
            "omitted_copy": zero_copy_mutant,
            "old_local_predicate_accepts_mutant": True,
            "positive_equality_rejects_mutant": True,
            "existing_scene1_core_on_pair_0_2": 1,
        },
        "block_noncredit": noncredit_models(),
        "scope": "fixed mathematical countermodels, no general finite oracle",
        "product_imported": False, "cpp_executed": False,
        "gcp": "not_used", "public_status": "not_claimed",
    }


if __name__ == "__main__":
    try:
        print(json.dumps(main(), sort_keys=True, separators=(",", ":")))
    except (ValueError, TypeError) as error:
        print(json.dumps({"status": "failed", "reason": str(error)}))
        raise SystemExit(1)
