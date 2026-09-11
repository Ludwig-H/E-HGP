"""Exact geometric witnesses for a seed lookup after an intruder exchange.

No product code, catalogue generator, measured MEB counter or timing is used.
"""

from __future__ import annotations

from fractions import Fraction as Q
import json


def need(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def squared(point, center):
    return sum((Q(a) - b) ** 2 for a, b in zip(point, center))


def population(points, center, radius):
    inside = sorted(name for name, p in points.items() if squared(p, center) < radius)
    shell = sorted(name for name, p in points.items() if squared(p, center) == radius)
    return inside, shell


def pair(points, a, b):
    center = tuple(Q(x + y, 2) for x, y in zip(points[a], points[b]))
    radius = squared(points[a], center)
    need(radius == squared(points[b], center) and radius > 0, "positive diameter ball")
    return center, radius


def compute():
    points = {"A": (0, 3, 0), "B": (8, 3, 0), "Z": (4, 2, 0),
              "W": (6, 0, 0), "E": (4, 23, 0)}
    need(all(0 <= coordinate <= 65535 for p in points.values() for coordinate in p), "u16 input")
    consumer_center, consumer_radius = (Q(4), Q(63, 5), Q(0)), Q(2704, 25)
    bary = {"A": Q(13, 50), "B": Q(13, 50), "E": Q(12, 25)}
    need(sum(bary.values()) == 1 and min(bary.values()) > 0, "positive triangle support")
    need(tuple(sum(bary[name] * points[name][i] for name in bary) for i in range(3)) == consumer_center,
         "consumer center in positive convex hull")
    need(population(points, consumer_center, consumer_radius) == ([], ["A", "B", "E"]), "Gabriel consumer ABE")
    center, radius = pair(points, "A", "B")
    inside, shell = population(points, center, radius)
    need(radius == 16 and (inside, shell) == (["W", "Z"], ["A", "B"]), "initial nonterminal")
    need(len(inside) + 2 - 1 > 2 and radius < consumer_radius, "inadmissible at K2 before consumer")
    branches = []
    for intruder, expected_radius in (("Z", Q(17, 4)), ("W", Q(13, 4))):
        need(squared(points[intruder], center) < radius, "strict intruder")
        next_center, next_radius = pair(points, "B", intruder)
        next_inside, next_shell = population(points, next_center, next_radius)
        need(next_radius == expected_radius < radius, "strictly smaller seed MEB")
        need(not next_inside and next_shell == sorted(["B", intruder]), "whole-population seed")
        need(len(next_inside) + len(next_shell) == 2, "seed belongs to K2")
        branches.append(dict(intruder=intruder, removed="A", terminal_sites=next_shell,
                             squared_radius=str(next_radius),
                             algebraic_MEB_calls_current=2, algebraic_MEB_calls_with_lookup=1))

    # The surviving diagonal certifies equality of the two MEBs. A complete
    # population lookup cannot match this K4 facet: the ball contains eight sites.
    square = {"A": (0, 0, 0), "B": (10, 0, 0), "C": (0, 10, 0), "D": (10, 10, 0),
              "Z": (4, 4, 0), "P": (4, 6, 0), "Q": (6, 4, 0), "R": (6, 6, 0)}
    square_center, square_radius = pair(square, "A", "D")
    diagonal_center, diagonal_radius = pair(square, "B", "C")
    need((square_center, square_radius) == (diagonal_center, diagonal_radius), "surviving diameter")
    selected = ["B", "C", "D", "Z"]
    need(all(squared(square[s], square_center) <= square_radius for s in selected), "equal-radius replacement")
    sq_inside, sq_shell = population(square, square_center, square_radius)
    need(len(sq_inside) + len(sq_shell) == 8 > len(selected) == 4, "partial population is not a seed")
    need(sum(squared(square[s], square_center) == square_radius for s in selected) == 3, "selected shell decreases")
    return dict(status="passed_exact_seed_exchange_witnesses", input_points={k: list(v) for k, v in points.items()}, K=2,
                consumer="ABE", consumer_squared_radius=str(consumer_radius), branches=branches,
                equal_radius_counterfixture=dict(input_points={k: list(v) for k, v in square.items()}, K=4, squared_radius=str(square_radius),
                                                  selected_sites=selected, whole_population=8, seed_hit=False),
                engine_executed=False, timing_claim=False, public_status="not_claimed")


if __name__ == "__main__":
    print(json.dumps(compute(), sort_keys=True))
