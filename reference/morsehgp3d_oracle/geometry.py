"""Exact three-dimensional balls and affine predicates for the CPU oracle."""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from fractions import Fraction
from itertools import combinations
from typing import Iterable, Sequence

from .exact import (
    Point3,
    Scalar,
    add,
    affine_dimension,
    dot,
    normalize_point,
    normalize_points,
    scale,
    squared_distance,
    subtract,
)


class GeometryError(ValueError):
    """Base class for exact geometric construction failures."""


class AffineDependenceError(GeometryError):
    """Raised when a purported support is not affinely independent."""


class PointOutsideAffineHullError(GeometryError):
    """Raised when barycentric coordinates do not exist in a support hull."""


def _canonical_ids(
    point_ids: Iterable[int],
    *,
    upper_bound: int | None = None,
    allow_empty: bool = False,
) -> tuple[int, ...]:
    raw_ids = tuple(point_ids)
    if any(isinstance(point_id, bool) or not isinstance(point_id, int) for point_id in raw_ids):
        raise TypeError("point identifiers must be integers")
    if any(point_id < 0 for point_id in raw_ids):
        raise IndexError("point identifiers must be non-negative")
    if len(set(raw_ids)) != len(raw_ids):
        raise ValueError("point identifiers must not contain duplicates")
    canonical = tuple(sorted(raw_ids))
    if not canonical and not allow_empty:
        raise ValueError("a point identifier set must be non-empty")
    if upper_bound is not None and any(point_id >= upper_bound for point_id in canonical):
        raise IndexError("point identifier is outside the point array")
    return canonical


def _solve_square(
    matrix: Sequence[Sequence[Fraction]], right_hand_side: Sequence[Fraction]
) -> tuple[Fraction, ...]:
    """Solve one nonsingular rational square system deterministically."""

    size = len(matrix)
    if size != len(right_hand_side) or any(len(row) != size for row in matrix):
        raise ValueError("a square linear system is required")
    if size == 0:
        return ()
    augmented = [list(row) + [right_hand_side[index]] for index, row in enumerate(matrix)]
    for column in range(size):
        pivot = next(
            (row for row in range(column, size) if augmented[row][column]),
            None,
        )
        if pivot is None:
            raise AffineDependenceError("support Gram matrix is singular")
        augmented[column], augmented[pivot] = augmented[pivot], augmented[column]
        pivot_value = augmented[column][column]
        augmented[column] = [value / pivot_value for value in augmented[column]]
        for row in range(size):
            if row == column or not augmented[row][column]:
                continue
            factor = augmented[row][column]
            augmented[row] = [
                value - factor * pivot_entry
                for value, pivot_entry in zip(augmented[row], augmented[column])
            ]
    return tuple(augmented[row][-1] for row in range(size))


def _affine_coefficients(point: Point3, support: tuple[Point3, ...]) -> tuple[Fraction, ...]:
    if not support:
        raise ValueError("a barycentric support must be non-empty")
    if len(support) > 4:
        raise ValueError("a 3D affine-independent support has at most four points")
    if len(support) == 1:
        if point != support[0]:
            raise PointOutsideAffineHullError("point is outside the singleton affine hull")
        return (Fraction(1),)

    origin = support[0]
    vectors = tuple(subtract(vertex, origin) for vertex in support[1:])
    if affine_dimension(support) != len(support) - 1:
        raise AffineDependenceError("barycentric support is affinely dependent")
    gram = tuple(tuple(dot(left, right) for right in vectors) for left in vectors)
    offset = subtract(point, origin)
    coordinates = _solve_square(gram, tuple(dot(vector, offset) for vector in vectors))
    reconstructed = origin
    for coefficient, vector in zip(coordinates, vectors):
        reconstructed = add(reconstructed, scale(coefficient, vector))
    if reconstructed != point:
        raise PointOutsideAffineHullError("point is outside the support affine hull")
    return (Fraction(1) - sum(coordinates, Fraction(0)),) + coordinates


@dataclass(frozen=True, slots=True)
class Ball:
    """An exact closed Euclidean ball with a canonical support label."""

    center: Point3
    squared_radius: Fraction
    support_ids: tuple[int, ...]

    def __post_init__(self) -> None:
        center = normalize_point(self.center)
        radius = Fraction(self.squared_radius)
        support = _canonical_ids(self.support_ids)
        if radius < 0:
            raise ValueError("a squared radius cannot be negative")
        if len(support) > 4:
            raise ValueError("a 3D miniball support has at most four points")
        object.__setattr__(self, "center", center)
        object.__setattr__(self, "squared_radius", radius)
        object.__setattr__(self, "support_ids", support)


class BallRelation(str, Enum):
    """Exact relation of a point to a closed ball."""

    INTERIOR = "interior"
    SHELL = "shell"
    EXTERIOR = "exterior"


@dataclass(frozen=True, slots=True)
class BallClassification:
    """Canonical global classification of point identifiers."""

    interior_ids: tuple[int, ...]
    shell_ids: tuple[int, ...]
    exterior_ids: tuple[int, ...]

    def __post_init__(self) -> None:
        groups = (self.interior_ids, self.shell_ids, self.exterior_ids)
        if any(tuple(sorted(group)) != group for group in groups):
            raise ValueError("classification identifiers must be sorted")
        flattened = tuple(point_id for group in groups for point_id in group)
        if len(flattened) != len(set(flattened)):
            raise ValueError("classification groups must be disjoint")

    @property
    def closed_rank(self) -> int:
        return len(self.interior_ids) + len(self.shell_ids)


def barycentric_coordinates(
    point: Sequence[Scalar] | object,
    support_points: Iterable[Sequence[Scalar] | object],
) -> tuple[Fraction, ...]:
    """Return unique exact affine coordinates in an independent support."""

    normalized_point = normalize_point(point)
    support = normalize_points(support_points)
    return _affine_coefficients(normalized_point, support)


def is_relative_interior(
    point: Sequence[Scalar] | object,
    support_points: Iterable[Sequence[Scalar] | object],
) -> bool:
    """Return whether ``point`` is in the support's relative convex interior."""

    return all(
        coefficient > 0
        for coefficient in barycentric_coordinates(point, support_points)
    )

def circumball(
    points: Sequence[Sequence[Scalar] | object], support_ids: Iterable[int]
) -> Ball:
    """Construct the unique equidistant ball of an independent 1..4 support.

    For supports of size two through four, the centre is computed in their
    affine hull by solving the exact Gram system.  A non-well-centred support is
    still a valid circumball; relative-interior testing is a separate predicate.
    """

    normalized = normalize_points(points)
    support = _canonical_ids(support_ids, upper_bound=len(normalized))
    if len(support) > 4:
        raise ValueError("circumball supports are limited to four points in 3D")
    support_points = tuple(normalized[point_id] for point_id in support)
    if len(support) == 1:
        return Ball(support_points[0], Fraction(0), support)
    if affine_dimension(support_points) != len(support_points) - 1:
        raise AffineDependenceError("circumball support is affinely dependent")

    origin = support_points[0]
    vectors = tuple(subtract(vertex, origin) for vertex in support_points[1:])
    gram = tuple(tuple(dot(left, right) for right in vectors) for left in vectors)
    right_hand_side = tuple(dot(vector, vector) / 2 for vector in vectors)
    coefficients = _solve_square(gram, right_hand_side)
    center = origin
    for coefficient, vector in zip(coefficients, vectors):
        center = add(center, scale(coefficient, vector))
    squared_radius = squared_distance(center, origin)
    if any(
        squared_distance(center, vertex) != squared_radius
        for vertex in support_points[1:]
    ):
        raise AssertionError("exact Gram construction produced unequal radii")
    return Ball(center, squared_radius, support)


def classify(point: Sequence[Scalar] | object, ball: Ball) -> BallRelation:
    """Classify a point using exact comparison to ``ball.squared_radius``."""

    distance = squared_distance(normalize_point(point), ball.center)
    if distance < ball.squared_radius:
        return BallRelation.INTERIOR
    if distance == ball.squared_radius:
        return BallRelation.SHELL
    return BallRelation.EXTERIOR


def classify_points(
    points: Sequence[Sequence[Scalar] | object],
    ball: Ball,
    point_ids: Iterable[int] | None = None,
) -> BallClassification:
    """Classify all requested identifiers and return three sorted groups."""

    normalized = normalize_points(points)
    identifiers = (
        tuple(range(len(normalized)))
        if point_ids is None
        else _canonical_ids(
            point_ids, upper_bound=len(normalized), allow_empty=True
        )
    )
    groups: dict[BallRelation, list[int]] = {
        BallRelation.INTERIOR: [],
        BallRelation.SHELL: [],
        BallRelation.EXTERIOR: [],
    }
    for point_id in identifiers:
        groups[classify(normalized[point_id], ball)].append(point_id)
    return BallClassification(
        tuple(groups[BallRelation.INTERIOR]),
        tuple(groups[BallRelation.SHELL]),
        tuple(groups[BallRelation.EXTERIOR]),
    )


def minimum_enclosing_ball(
    points: Sequence[Sequence[Scalar] | object],
    subset_ids: Iterable[int] | None = None,
) -> Ball:
    """Return the exact miniball of a non-empty labelled subset.

    Every positive affine-independent support of size at most four is tried.
    Circumballs whose centre is on or outside the relative support boundary are
    rejected even when they enclose the subset: they are not minimal supports.
    Among containing candidates the exact radius is minimized, then support
    size and support label break degeneracy canonically.  This deliberately
    exhaustive routine is independent of the production miniball implementation.
    """

    normalized = normalize_points(points)
    if not normalized:
        raise ValueError("minimum_enclosing_ball requires at least one point")
    subset = (
        tuple(range(len(normalized)))
        if subset_ids is None
        else _canonical_ids(subset_ids, upper_bound=len(normalized))
    )
    if not subset:
        raise ValueError("minimum_enclosing_ball requires a non-empty subset")

    candidates: list[Ball] = []
    for support_size in range(1, min(4, len(subset)) + 1):
        for support in combinations(subset, support_size):
            try:
                candidate = circumball(normalized, support)
            except AffineDependenceError:
                continue
            support_points = tuple(normalized[point_id] for point_id in support)
            if not is_relative_interior(candidate.center, support_points):
                continue
            if all(
                classify(normalized[point_id], candidate) is not BallRelation.EXTERIOR
                for point_id in subset
            ):
                candidates.append(candidate)
    if not candidates:
        raise GeometryError("no enclosing support was found")
    return min(
        candidates,
        key=lambda ball: (
            ball.squared_radius,
            len(ball.support_ids),
            ball.support_ids,
        ),
    )
