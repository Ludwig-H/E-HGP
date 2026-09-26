#!/usr/bin/env python3
"""Bounded exact geometry fixtures for the support carrier, not a FULL test.

Only the explicit planar square and line segments below are covered. No
native catalogue, learning, surface reconstruction or performance claim.
"""
from fractions import Fraction as F
import hashlib
from itertools import combinations, permutations
import json
from pathlib import Path
import sys


def require(condition, message):
    if not condition:
        raise ValueError(message)


def point(x, y):
    return (F(x), F(y))


def dot(a, b):
    return sum((x * y for x, y in zip(a, b)), F(0))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def distance_squared_to_segment(x, a, b):
    ab = sub(b, a)
    denominator = dot(ab, ab)
    require(denominator > 0, "degenerate segment")
    t = min(F(1), max(F(0), dot(sub(x, a), ab) / denominator))
    closest = tuple(y + t * d for y, d in zip(a, ab))
    residual = sub(x, closest)
    return dot(residual, residual)


def square_minimal_supports(points):
    """Enumerate positive minimal supports of this circle, in the plane."""
    require(len(set(points)) == 4, "expected four distinct square vertices")
    require(set(points) == {point(-1, -1), point(-1, 1),
                            point(1, -1), point(1, 1)}, "square domain only")
    supports = []
    for a, b in combinations(points, 2):
        if tuple(x + y for x, y in zip(a, b)) == point(0, 0):
            supports.append(tuple(sorted((a, b))))
    # Every square triple includes a diametral pair; it is not minimal.
    for triple in combinations(points, 3):
        require(any(set(pair).issubset(triple) for pair in supports),
                "unexpected minimal triple")
    return tuple(sorted(supports))


def carrier_distance_squared(x, supports):
    require(bool(supports), "empty carrier has no finite distance")
    return min(distance_squared_to_segment(x, *segment) for segment in supports)


def normalized_segment_xy(a, b):
    # Integral over t in [0,1] of (a_x+t d_x)(a_y+t d_y).
    dx, dy = sub(b, a)
    return a[0] * a[1] + (a[0] * dy + a[1] * dx) / 2 + dx * dy / 3


def result(name, facts, rejected):
    require(all(rejected.values()), "an incorrect alternative survived")
    return {"name": name, "status": "PASS", "facts": facts,
            "incorrect_variants": {name: "REJECTED" for name in rejected}}


def choice_and_relabeling():
    square = (point(-1, -1), point(1, 1), point(-1, 1), point(1, -1))
    supports = square_minimal_supports(square)
    require(len(supports) == 2, "expected two minimal supports")
    require(all(dot(vertex, vertex) == 2 for vertex in square), "wrong ball")
    probe = point(1, 1)
    distances = sorted(distance_squared_to_segment(probe, *s) for s in supports)
    require(distances == [F(0), F(2)], "representative distances changed")
    require(all(square_minimal_supports(order) == supports
                for order in permutations(square)), "carrier depends on IDs")
    return result("one_ball_two_supports", {
        "ball_center": [0, 0], "ball_radius_squared": 2,
        "q_min": 2, "ball_count": 1, "minimal_support_count": 2,
        "probe": probe, "representative_distances_squared": distances,
        "permutations_checked": 24,
    }, {
        "one_representative_defines_the_geometry": distances[0] != distances[1],
        "q_min_histogram_counts_all_supports": 1 != len(supports),
    })


def different_moments():
    supports = square_minimal_supports((point(-1, -1), point(1, 1),
                                      point(-1, 1), point(1, -1)))
    moments = sorted(normalized_segment_xy(*segment) for segment in supports)
    require(moments == [F(-1, 3), F(1, 3)], "wrong segment moment")
    combined = sum(moments, F(0)) / 2  # Equal segment lengths.
    require(combined == 0, "canonical carrier moment")
    return result("representative_moments", {
        "normalized_xy_moments": moments, "all_supports_xy_moment": combined,
    }, {
        "one_support_gives_canonical_moments": all(x != combined for x in moments),
    })


def hull_and_population():
    supports = square_minimal_supports((point(-1, -1), point(1, 1),
                                      point(-1, 1), point(1, -1)))
    hull_probe = point(1, 0)
    interior = point(0, F(1, 2))
    hull_distance = F(0)  # hull_probe is on the square's boundary edge.
    require(-1 <= hull_probe[0] <= 1 and -1 <= hull_probe[1] <= 1,
            "probe outside filled square")
    skeleton_distance = carrier_distance_squared(hull_probe, supports)
    interior_distance = carrier_distance_squared(interior, supports)
    require(skeleton_distance == F(1, 2), "wrong cross distance")
    require(interior_distance == F(1, 8) and dot(interior, interior) < 2,
            "interior point example")
    return result("carrier_not_hull_or_population", {
        "filled_square_distance_squared": hull_distance,
        "skeleton_distance_squared": skeleton_distance,
        "strict_interior_point_distance_squared": interior_distance,
    }, {
        "union_equals_shell_convex_hull": hull_distance != skeleton_distance,
        "all_interior_points_lie_in_support_carrier": interior_distance > 0,
    })


def multiplicity_and_empty():
    a, b = point(-1, 0), point(1, 0)
    full = F(2)
    inner = F(1)  # Collinear segment [-1/2, 1/2], already contained.
    incidence_mass = full + inner
    union_mass = full
    require(incidence_mass == 3 and union_mass == 2, "wrong segment lengths")
    try:
        carrier_distance_squared(point(0, 0), ())
    except ValueError:
        empty_rejected = True
    else:
        empty_rejected = False
    return result("measure_and_empty_domain", {
        "incidence_length": incidence_mass, "union_length": union_mass,
        "empty_distance": "undefined_as_finite_feature",
    }, {
        "sum_of_primitive_masses_is_union_measure": incidence_mass != union_mass,
        "empty_support_has_finite_zero_distance": empty_rejected,
    })


def encode(value):
    if isinstance(value, F):
        return str(value.numerator) + "/" + str(value.denominator)
    if isinstance(value, dict):
        return {key: encode(item) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [encode(item) for item in value]
    return value


def main():
    fixtures = []
    for check in (choice_and_relabeling, different_moments,
                  hull_and_population, multiplicity_and_empty):
        try:
            fixtures.append(check())
        except Exception as error:
            fixtures.append({"name": check.__name__, "status": "FAIL",
                             "error": str(error)})
    passed = all(item["status"] == "PASS" for item in fixtures)
    report = {
        "schema": "zoltan.support_carrier.v1",
        "scope": "explicit_square_and_segment_geometry_only",
        "engine_qualification": False,
        "native_export_qualification": False,
        "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "status": "PASS" if passed else "FAIL",
        "fixture_count": len(fixtures),
        "rejected_variant_count": sum(len(item.get("incorrect_variants", {}))
                                      for item in fixtures),
        "fixtures": fixtures,
    }
    print(json.dumps(encode(report), ensure_ascii=False, sort_keys=True, indent=2))
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
