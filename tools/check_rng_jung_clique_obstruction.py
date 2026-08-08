#!/usr/bin/env python3
"""Exact regressions for the RNG--Jung clique obstruction.

Only Python's Fraction arithmetic is used.  The fixture is the product case
q=11; additional orders are generated from the same dyadic construction so a
test cannot accidentally validate only one hard-coded point cloud.
"""

from __future__ import annotations

import argparse
import json
from fractions import Fraction
from itertools import combinations
from pathlib import Path
from typing import Mapping, Sequence


Point = tuple[Fraction, Fraction, Fraction]


def point(values: Sequence[str | int]) -> Point:
    if len(values) != 3:
        raise ValueError("a point must have three coordinates")
    return tuple(Fraction(value) for value in values)  # type: ignore[return-value]


def subtract(left: Point, right: Point) -> Point:
    return tuple(a - b for a, b in zip(left, right))  # type: ignore[return-value]


def add(left: Point, right: Point) -> Point:
    return tuple(a + b for a, b in zip(left, right))  # type: ignore[return-value]


def scale(value: Fraction, operand: Point) -> Point:
    return tuple(value * coordinate for coordinate in operand)  # type: ignore[return-value]


def squared_norm(value: Point) -> Fraction:
    return sum((coordinate * coordinate for coordinate in value), Fraction(0))


def dot_product(left: Point, right: Point) -> Fraction:
    return sum((a * b for a, b in zip(left, right)), Fraction(0))


def squared_distance(left: Point, right: Point) -> Fraction:
    return squared_norm(subtract(left, right))


def determinant(first: Point, second: Point, third: Point) -> Fraction:
    return (
        first[0] * (second[1] * third[2] - second[2] * third[1])
        - first[1] * (second[0] * third[2] - second[2] * third[0])
        + first[2] * (second[0] * third[1] - second[1] * third[0])
    )


def next_power_of_two_at_least(value: int) -> int:
    result = 1
    while result < value:
        result *= 2
    return result


def generated_case(order: int) -> dict[str, object]:
    if order < 0:
        raise ValueError("an RNG order must be non-negative")
    support: dict[str, Point] = {
        "A": point((0, 0, 0)),
        "B": point((-2, -2, -1)),
        "C": point((-2, 1, 0)),
        "D": point((0, -1, -1)),
    }
    center = point(("-3/2", "-1/2", "-1/2"))
    step = Fraction(1, 16)
    axes = (point((1, 0, 0)), point((0, 1, 0)), point((0, 0, 1)))
    interior = [center]
    for axis in axes:
        interior.append(add(center, scale(step, axis)))
        interior.append(add(center, scale(-step, axis)))
    denominator = next_power_of_two_at_least(16 * (order + 2))
    witnesses = [
        (Fraction(1) + Fraction(index, denominator), Fraction(-1, 2), Fraction(-1, 2))
        for index in range(1, order + 2)
    ]
    return {
        "order": order,
        "support": support,
        "center": center,
        "squared_radius": Fraction(11, 4),
        "weights": (Fraction(1, 8), Fraction(3, 8), Fraction(3, 8), Fraction(1, 8)),
        "interior": interior,
        "witnesses": witnesses,
    }


def assert_case(case: Mapping[str, object]) -> None:
    order = int(case["order"])
    support = case["support"]
    if not isinstance(support, Mapping):
        raise AssertionError("support mapping missing")
    vertices = tuple(support[name] for name in ("A", "B", "C", "D"))
    if not all(isinstance(vertex, tuple) for vertex in vertices):
        raise AssertionError("support points were not decoded")
    a, b, c, d = vertices  # type: ignore[misc]
    center = case["center"]
    radius = case["squared_radius"]
    weights = case["weights"]
    interior = case["interior"]
    witnesses = case["witnesses"]
    if not isinstance(center, tuple) or not isinstance(radius, Fraction):
        raise AssertionError("circumball was not decoded")
    if (
        not isinstance(weights, tuple)
        or not isinstance(interior, list)
        or not isinstance(witnesses, list)
    ):
        raise AssertionError("case payload was not decoded")

    affine_determinant = determinant(subtract(b, a), subtract(c, a), subtract(d, a))
    if affine_determinant != 4:
        raise AssertionError(f"unexpected affine determinant {affine_determinant}")
    if any(weight <= 0 for weight in weights) or sum(weights, Fraction(0)) != 1:
        raise AssertionError("the circumcenter weights are not strictly barycentric")
    weighted_center = point((0, 0, 0))
    for weight, vertex in zip(weights, vertices):
        weighted_center = add(weighted_center, scale(weight, vertex))
    if weighted_center != center:
        raise AssertionError("the barycentric witness does not reconstruct the center")
    if any(squared_distance(vertex, center) != radius for vertex in vertices):
        raise AssertionError("a support vertex is not on the claimed sphere")
    if len(interior) != 7 or any(
        squared_distance(candidate, center) >= radius for candidate in interior
    ):
        raise AssertionError("the seven rank witnesses are not strictly interior")
    if len(witnesses) != order + 1 or any(
        squared_distance(candidate, center) <= radius for candidate in witnesses
    ):
        raise AssertionError("an RNG lune witness is not strictly outside the circumball")

    cloud = list(vertices) + interior + witnesses
    closed_rank = sum(squared_distance(candidate, center) <= radius for candidate in cloud)
    if closed_rank != 11:
        raise AssertionError(f"expected closed rank 11, observed {closed_rank}")

    # Positive counterpart to the RNG obstruction.  A diameter pair anchors a
    # two-dimensional Jung disc in its perpendicular bisector plane.  At the
    # support centre, every other point contributes one oriented halfplane;
    # its signed value is exactly radius^2 - distance^2.  Hence the strict
    # halfplane depth is the number of strict circumball interiors, while the
    # closed rank is two plus the closed depth (the anchor endpoints are the
    # other two shell points).
    indexed_edges = [
        (squared_distance(vertices[left], vertices[right]), left, right)
        for left in range(len(vertices))
        for right in range(left + 1, len(vertices))
    ]
    diameter_squared = max(edge[0] for edge in indexed_edges)
    diameter_edges = [edge for edge in indexed_edges if edge[0] == diameter_squared]
    if len(diameter_edges) != 1:
        raise AssertionError("the obstruction support lost its unique diameter anchor")
    _, first_index, second_index = diameter_edges[0]
    first = vertices[first_index]
    second = vertices[second_index]
    midpoint = scale(Fraction(1, 2), add(first, second))
    centre_offset = subtract(center, midpoint)
    if dot_product(centre_offset, subtract(second, first)) != 0:
        raise AssertionError("the support centre left the diameter bisector plane")
    if squared_norm(centre_offset) > diameter_squared / 8:
        raise AssertionError("the support centre left the tetrahedral Jung disc")

    strict_depth = 0
    closed_depth = 0
    for cloud_index, candidate in enumerate(cloud):
        if cloud_index in (first_index, second_index):
            continue
        from_midpoint = subtract(candidate, midpoint)
        halfplane_value = (
            diameter_squared / 4
            - squared_norm(from_midpoint)
            + 2 * dot_product(centre_offset, from_midpoint)
        )
        power_complement = radius - squared_distance(candidate, center)
        if halfplane_value != power_complement:
            raise AssertionError("the anchored halfplane identity changed")
        strict_depth += halfplane_value > 0
        closed_depth += halfplane_value >= 0
    if strict_depth != 7 or 2 + closed_depth != closed_rank:
        raise AssertionError(
            "the shallow-arrangement depth no longer reconstructs closed rank 11"
        )

    edge_squared = squared_distance(a, d)
    if edge_squared != 2:
        raise AssertionError(f"unexpected AD squared length {edge_squared}")
    open_lune = [
        candidate
        for candidate in cloud
        if candidate not in (a, d)
        and squared_distance(candidate, a) < edge_squared
        and squared_distance(candidate, d) < edge_squared
    ]
    if open_lune != witnesses:
        raise AssertionError("the open lune does not contain exactly the external witnesses")
    if len(open_lune) <= order:
        raise AssertionError("AD unexpectedly survives the declared order-q RNG")


def decode_fixture(path: Path) -> tuple[dict[str, object], Mapping[str, object]]:
    raw = json.loads(path.read_text(encoding="utf-8"))
    support = {name: point(values) for name, values in raw["support"].items()}
    decoded: dict[str, object] = {
        "order": int(raw["rng_order"]),
        "support": support,
        "center": point(raw["circumball"]["center"]),
        "squared_radius": Fraction(raw["circumball"]["squared_radius"]),
        "weights": tuple(
            Fraction(value)
            for value in raw["circumball"]["barycentric_weights_ABCD"]
        ),
        "interior": [point(values) for values in raw["strict_interior_points"]],
        "witnesses": [point(values) for values in raw["external_open_lune_witnesses"]],
    }
    return decoded, raw["expected"]


def assert_fixture(path: Path) -> None:
    decoded, expected = decode_fixture(path)
    assert_case(decoded)
    if int(expected["closed_rank"]) != 11:
        raise AssertionError("the fixture expectation changed its closed rank")
    if bool(expected["edge_AD_present_in_order_q_rng"]):
        raise AssertionError("the fixture claims that the obstructed edge survives")
    if bool(expected["support_is_four_clique"]):
        raise AssertionError("the fixture claims that the support remains a clique")
    if expected["diameter_edge"] != ["B", "C"]:
        raise AssertionError("the fixture expectation changed its diameter anchor")
    if int(expected["strict_halfplane_depth_at_support_center"]) != 7:
        raise AssertionError("the fixture expectation changed its shallow depth")
    if not bool(expected["support_center_inside_tetrahedral_jung_disc"]):
        raise AssertionError("the fixture excludes its centre from the Jung disc")
    if not bool(expected["symmetric_family_removes_every_diameter_edge"]):
        raise AssertionError("the fixture dropped the all-diameter obstruction")
    expected_edge_counts = {
        "symmetric_family_order_q_rng_edge_count": 1003,
        "one_stage_alpha3_endpoint_max_edge_count": 2101,
        "first_stage_alpha2_endpoint_max_edge_count": 2053,
        "two_stage_alpha2_alpha3_endpoint_max_edge_count": 2965,
        "fixed_point_endpoint_max_edge_count": 3403,
        "fixed_point_mutual_min_edge_count": 1285,
    }
    for key, value in expected_edge_counts.items():
        if int(expected[key]) != value:
            raise AssertionError(f"the fixture changed {key}")
    if bool(expected["one_stage_endpoint_max_jung_grown_q11_support_is_four_clique"]):
        raise AssertionError("the fixture claims one-stage Jung growth is complete")
    if int(expected["one_stage_endpoint_max_jung_grown_q11_support_edge_count"]) != 0:
        raise AssertionError("the fixture changed the one-stage support edge count")
    if bool(expected["two_stage_alpha2_alpha3_endpoint_max_jung_grown_q11_support_is_four_clique"]):
        raise AssertionError("the fixture claims alpha2--alpha3 Jung growth is complete")
    if int(expected["two_stage_alpha2_alpha3_endpoint_max_jung_grown_q11_support_edge_count"]) != 0:
        raise AssertionError("the fixture changed the two-stage support edge count")
    if not bool(expected["fixed_point_endpoint_max_jung_grown_q11_support_is_four_clique"]):
        raise AssertionError("the fixture hides endpoint-max fixed-point recovery")
    if int(expected["fixed_point_endpoint_max_jung_grown_q11_support_edge_count"]) != 6:
        raise AssertionError("the fixture changed the endpoint-max support edge count")
    if bool(expected["fixed_point_mutual_min_jung_grown_q11_support_is_four_clique"]):
        raise AssertionError("the fixture claims mutual-min fixed-point recovery")
    if int(expected["fixed_point_mutual_min_jung_grown_q11_support_edge_count"]) != 0:
        raise AssertionError("the fixture changed the mutual-min support edge count")


def assert_classical_rng_longest_edge_obstruction() -> None:
    first = point((0, 0, 0))
    second = point((3, 0, 0))
    third = point((0, 1, 0))
    longest = squared_distance(second, third)
    if not (squared_distance(first, second) < longest and squared_distance(first, third) < longest):
        raise AssertionError("the third point did not kill the unique longest edge")


def assert_regular_tetrahedron_jung_sharpness_and_rng_obstruction(order: int) -> None:
    support = (
        point((1, 1, 1)),
        point((1, -1, -1)),
        point((-1, 1, -1)),
        point((-1, -1, 1)),
    )
    edge_squared = squared_distance(support[0], support[1])
    edge_cech_birth_squared = edge_squared / 4
    tetra_cech_birth_squared = Fraction(3)
    if edge_squared != 8 or tetra_cech_birth_squared != Fraction(3, 2) * edge_cech_birth_squared:
        raise AssertionError("the regular tetrahedron did not attain alpha_3 squared = 3/2")
    face_cech_birth_squared = Fraction(8, 3)
    if not edge_cech_birth_squared < face_cech_birth_squared < tetra_cech_birth_squared:
        raise AssertionError("the Cech two-skeleton obstruction is not strict")

    # Every edge of this regular tetrahedron is diametral.  Its midpoint m has
    # squared norm one.  Points (2+epsilon)m are outside the support sphere but
    # remain strictly in the edge's open lune for 0<epsilon<1/16.  Giving every
    # edge q+1 such points removes all possible RNG diameter anchors while the
    # seven strict interiors keep the support at closed rank eleven.
    center = point((0, 0, 0))
    step = Fraction(1, 16)
    axes = (point((1, 0, 0)), point((0, 1, 0)), point((0, 0, 1)))
    interior = [center]
    for axis in axes:
        interior.append(scale(step, axis))
        interior.append(scale(-step, axis))
    denominator = next_power_of_two_at_least(16 * (order + 2))
    all_external_witnesses: list[Point] = []
    edge_witnesses: dict[tuple[int, int], list[Point]] = {}
    for first_index in range(len(support)):
        for second_index in range(first_index + 1, len(support)):
            midpoint = scale(
                Fraction(1, 2),
                add(support[first_index], support[second_index]),
            )
            if squared_norm(midpoint) != 1:
                raise AssertionError("a regular-tetrahedron edge lost its midpoint norm")
            witnesses = [
                scale(Fraction(2) + Fraction(index, denominator), midpoint)
                for index in range(1, order + 2)
            ]
            edge_witnesses[(first_index, second_index)] = witnesses
            all_external_witnesses.extend(witnesses)

    cloud = list(support) + interior + all_external_witnesses
    closed_rank = sum(squared_norm(candidate) <= 3 for candidate in cloud)
    if closed_rank != 11:
        raise AssertionError(
            "the all-diameter RNG obstruction changed the support's closed rank"
        )
    for (first_index, second_index), own_witnesses in edge_witnesses.items():
        first = support[first_index]
        second = support[second_index]
        edge_squared = squared_distance(first, second)
        if edge_squared != 8:
            raise AssertionError("a regular-tetrahedron edge stopped being diametral")
        if any(
            squared_norm(candidate) <= 3
            or squared_distance(candidate, first) >= edge_squared
            or squared_distance(candidate, second) >= edge_squared
            for candidate in own_witnesses
        ):
            raise AssertionError("an all-diameter lune witness lost strict separation")
        open_lune_population = sum(
            candidate not in (first, second)
            and squared_distance(candidate, first) < edge_squared
            and squared_distance(candidate, second) < edge_squared
            for candidate in cloud
        )
        if open_lune_population <= order:
            raise AssertionError("a diameter edge survived the order-q RNG")

    if order == 11:
        # Formalise the stronger proposal, not merely the raw RNG.  Build the
        # whole graph exactly: endpoint-max growth can transmit a large scale
        # from an external vertex, so inspecting only the four support scales
        # would not validate the proposed arity-2 -> arity-3 -> arity-4 cascade.
        distances = [
            [squared_distance(left, right) for right in cloud]
            for left in cloud
        ]
        rng_edges: set[tuple[int, int]] = set()
        for first_index in range(len(cloud)):
            for second_index in range(first_index + 1, len(cloud)):
                candidate_squared = distances[first_index][second_index]
                lune_population = 0
                for blocker_index in range(len(cloud)):
                    if blocker_index in (first_index, second_index):
                        continue
                    if (
                        distances[blocker_index][first_index] < candidate_squared
                        and distances[blocker_index][second_index] < candidate_squared
                    ):
                        lune_population += 1
                        if lune_population > order:
                            break
                if lune_population <= order:
                    rng_edges.add((first_index, second_index))

        def incident_scales(edges: set[tuple[int, int]]) -> list[Fraction]:
            result = [Fraction(0) for _ in cloud]
            for first_index, second_index in edges:
                edge_length = distances[first_index][second_index]
                result[first_index] = max(result[first_index], edge_length)
                result[second_index] = max(result[second_index], edge_length)
            return result

        def grow(
            edges: set[tuple[int, int]],
            multiplier_squared: Fraction,
            *,
            endpoint_maximum: bool,
        ) -> set[tuple[int, int]]:
            scales = incident_scales(edges)
            grown = set(edges)
            for first_index in range(len(cloud)):
                for second_index in range(first_index + 1, len(cloud)):
                    endpoint_scale = (
                        max(scales[first_index], scales[second_index])
                        if endpoint_maximum
                        else min(scales[first_index], scales[second_index])
                    )
                    if distances[first_index][second_index] <= (
                        multiplier_squared * endpoint_scale
                    ):
                        grown.add((first_index, second_index))
            return grown

        support_edges = set(combinations(range(len(support)), 2))
        largest_incident_squared = incident_scales(rng_edges)[: len(support)]

        expected_largest_squared = Fraction(801, 256)
        if any(
            value != expected_largest_squared
            for value in largest_incident_squared
        ):
            raise AssertionError(
                "the rank-11 fixture changed its longest incident q-RNG scale"
            )
        for support_index in range(len(support)):
            largest_subdiameter_squared = max(
                squared_distance(support[support_index], candidate)
                for candidate in cloud
                if candidate != support[support_index]
                and squared_distance(support[support_index], candidate)
                < edge_squared
            )
            if largest_subdiameter_squared != expected_largest_squared:
                raise AssertionError(
                    "the Jung-growth fixed-point gap below the support diameter changed"
                )
        jung_triangle_squared = Fraction(4, 3)
        jung_tetrahedron_squared = Fraction(3, 2)
        one_stage_max = grow(
            rng_edges, jung_tetrahedron_squared, endpoint_maximum=True
        )
        first_stage_max = grow(
            rng_edges, jung_triangle_squared, endpoint_maximum=True
        )
        two_stage_max = grow(
            first_stage_max,
            jung_tetrahedron_squared,
            endpoint_maximum=True,
        )
        if (
            len(rng_edges) != 1003
            or len(one_stage_max) != 2101
            or len(first_stage_max) != 2053
            or len(two_stage_max) != 2965
        ):
            raise AssertionError("the exact Jung-growth edge census changed")
        if support_edges & one_stage_max or support_edges & two_stage_max:
            raise AssertionError(
                "the bounded endpoint-max Jung cascade recovered a support edge"
            )

        # Do not overstate that bounded obstruction.  Repeating endpoint-max
        # growth propagates scales from the external witnesses and this fixture
        # eventually becomes complete on the support.  The mutual/minimum rule
        # does reach a genuinely incomplete fixed point here.
        fixed_point_max = set(rng_edges)
        fixed_point_min = set(rng_edges)
        for endpoint_maximum, edges in (
            (True, fixed_point_max),
            (False, fixed_point_min),
        ):
            while True:
                successor = grow(
                    edges,
                    jung_tetrahedron_squared,
                    endpoint_maximum=endpoint_maximum,
                )
                if successor == edges:
                    break
                edges.clear()
                edges.update(successor)
        if not support_edges <= fixed_point_max:
            raise AssertionError(
                "endpoint-max fixed-point recovery unexpectedly disappeared"
            )
        if support_edges & fixed_point_min:
            raise AssertionError(
                "mutual-min fixed-point growth unexpectedly recovered a support edge"
            )
        if len(fixed_point_max) != 3403 or len(fixed_point_min) != 1285:
            raise AssertionError("a fixed-point Jung-growth edge census changed")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--orders", type=int, nargs="+", default=[0, 1, 7, 11, 1000])
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    assert_fixture(arguments.fixture)
    for order in arguments.orders:
        assert_case(generated_case(order))
        assert_regular_tetrahedron_jung_sharpness_and_rng_obstruction(order)
    assert_classical_rng_longest_edge_obstruction()
    print(
        "rng_jung_clique_obstruction_exact: pass; "
        f"orders={','.join(str(order) for order in arguments.orders)}; "
        "closed_rank=11; all_diameter_edges_removed=true; "
        "q11_edge_census=1003->2053->2965; "
        "alpha2_alpha3_endpoint_max_q11_four_clique=false; "
        "fixed_point_endpoint_max_q11_four_clique=true; "
        "fixed_point_mutual_min_q11_four_clique=false; "
        "shallow_depth=7; alpha3_squared=3/2"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
