"""Deterministic generic point clouds for exhaustive reference campaigns."""

from __future__ import annotations

from fractions import Fraction
from random import Random
from typing import Iterable, Sequence


Point3 = tuple[int, int, int]


def _unique_samples(
    rng: Random,
    *,
    count: int,
    dimension: int,
    coordinate_bound: int,
    initial: Iterable[tuple[int, ...]],
) -> list[tuple[int, ...]]:
    samples = list(initial)
    seen = set(samples)
    while len(samples) < count:
        candidate = tuple(
            rng.randint(-coordinate_bound, coordinate_bound)
            for _ in range(dimension)
        )
        if candidate in seen:
            continue
        seen.add(candidate)
        samples.append(candidate)
    return samples


def generate_affine_cloud(
    affine_dimension: int,
    point_count: int,
    seed: int,
    *,
    coordinate_bound: int = 16,
) -> tuple[Point3, ...]:
    """Return a reproducible integer cloud of exactly the requested rank.

    The fixed 2D and 3D frames are deliberately non-orthogonal.  Standard
    coordinate frames would manufacture exact cospherical shell ties in every
    generated cloud and would therefore test a degeneracy family rather than
    the generic phase-1 domain.
    """

    if affine_dimension not in (1, 2, 3):
        raise ValueError("affine_dimension must be 1, 2, or 3")
    if point_count < affine_dimension + 1:
        raise ValueError("point_count is too small for the requested dimension")
    if coordinate_bound < 2:
        raise ValueError("coordinate_bound must be at least 2")

    rng = Random(seed)
    if affine_dimension == 1:
        parameters = _unique_samples(
            rng,
            count=point_count,
            dimension=1,
            coordinate_bound=coordinate_bound,
            initial=((0,), (1,)),
        )
        points = [(value, 2 * value + 3, 1 - value) for (value,) in parameters]
    elif affine_dimension == 2:
        parameters = _unique_samples(
            rng,
            count=point_count,
            dimension=2,
            coordinate_bound=coordinate_bound,
            initial=((0, 0), (1, 0), (1, 2)),
        )
        points = [(u, v, u + 2 * v + 1) for u, v in parameters]
    else:
        points = _unique_samples(
            rng,
            count=point_count,
            dimension=3,
            coordinate_bound=coordinate_bound,
            initial=((0, 0, 0), (1, 2, 3), (4, 1, 2), (2, 5, 1)),
        )

    rng.shuffle(points)
    return tuple(points)


def affine_dimension(points: Sequence[Sequence[int | Fraction]]) -> int:
    """Compute the exact affine dimension of a non-empty 3D point sequence."""

    if not points:
        raise ValueError("points must be non-empty")
    if any(len(point) != 3 for point in points):
        raise ValueError("every point must have exactly three coordinates")

    origin = tuple(Fraction(value) for value in points[0])
    rows = [
        [Fraction(value) - origin[index] for index, value in enumerate(point)]
        for point in points[1:]
    ]
    rank = 0
    column = 0
    while rank < len(rows) and column < 3:
        pivot = next(
            (index for index in range(rank, len(rows)) if rows[index][column]),
            None,
        )
        if pivot is None:
            column += 1
            continue
        rows[rank], rows[pivot] = rows[pivot], rows[rank]
        pivot_value = rows[rank][column]
        rows[rank] = [value / pivot_value for value in rows[rank]]
        for index, row in enumerate(rows):
            if index == rank or not row[column]:
                continue
            factor = row[column]
            rows[index] = [
                value - factor * pivot_entry
                for value, pivot_entry in zip(row, rows[rank])
            ]
        rank += 1
        column += 1
    return rank


__all__ = ["Point3", "affine_dimension", "generate_affine_cloud"]
