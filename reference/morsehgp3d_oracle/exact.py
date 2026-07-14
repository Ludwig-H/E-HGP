"""Small exact-arithmetic helpers for the independent phase-1 oracle.

Only :class:`fractions.Fraction` is used.  In particular, a finite Python
``float`` is interpreted as its exact IEEE-754 binary64 dyadic value; it is
never rounded to a decimal surrogate.
"""

from __future__ import annotations

from fractions import Fraction
from math import isfinite
from typing import Iterable, Sequence, TypeAlias


Scalar: TypeAlias = int | float | Fraction
Point3: TypeAlias = tuple[Fraction, Fraction, Fraction]


class ExactInputError(ValueError):
    """Base class for invalid exact-oracle inputs."""


class NonFiniteCoordinateError(ExactInputError):
    """Raised when an IEEE-754 input is NaN or infinite."""


class DuplicatePointError(ExactInputError):
    """Raised when the phase-1 set oracle receives duplicate sites."""


def normalize_scalar(value: Scalar) -> Fraction:
    """Return the exact rational denoted by an accepted scalar.

    Booleans are rejected even though ``bool`` subclasses ``int`` in Python.
    Signed floating zero is canonically mapped to ``Fraction(0)``.
    """

    if isinstance(value, bool):
        raise TypeError("boolean coordinates are not allowed")
    if isinstance(value, Fraction):
        return value
    if isinstance(value, int):
        return Fraction(value)
    if isinstance(value, float):
        if not isfinite(value):
            raise NonFiniteCoordinateError("coordinates must be finite")
        if value == 0.0:
            return Fraction(0)
        return Fraction.from_float(value)
    raise TypeError(f"unsupported coordinate type {type(value).__name__}")


def _coordinates(point: object) -> object:
    """Unwrap the lightweight point records used by sibling oracle modules."""

    if hasattr(point, "coordinates"):
        coordinates = getattr(point, "coordinates")
        return coordinates() if callable(coordinates) else coordinates
    if hasattr(point, "coords"):
        coordinates = getattr(point, "coords")
        return coordinates() if callable(coordinates) else coordinates
    if all(hasattr(point, axis) for axis in ("x", "y", "z")):
        return (getattr(point, "x"), getattr(point, "y"), getattr(point, "z"))
    return point


def normalize_point(point: Sequence[Scalar] | object) -> Point3:
    """Normalize one three-dimensional point without changing its value."""

    raw_coordinates = _coordinates(point)
    try:
        coordinates = tuple(raw_coordinates)  # type: ignore[arg-type]
    except TypeError as error:
        raise TypeError("a point must expose an iterable of coordinates") from error
    if len(coordinates) != 3:
        raise ExactInputError("every point must contain exactly three coordinates")
    return tuple(normalize_scalar(value) for value in coordinates)  # type: ignore[return-value]


def normalize_points(
    points: Iterable[Sequence[Scalar] | object],
    *,
    canonical: bool = False,
    reject_duplicates: bool = False,
) -> tuple[Point3, ...]:
    """Normalize a point iterable, optionally sorting and checking duplicates.

    With ``canonical=False`` the input order, hence external point identifiers,
    is preserved.  Catalogue construction uses ``canonical=True`` so its public
    identifiers do not depend on arrival order.
    """

    normalized = tuple(normalize_point(point) for point in points)
    if canonical:
        normalized = tuple(sorted(normalized))
    if reject_duplicates:
        seen: set[Point3] = set()
        for point in normalized:
            if point in seen:
                raise DuplicatePointError(
                    "duplicate coordinates are outside the phase-1 set oracle contract"
                )
            seen.add(point)
    return normalized


def canonicalize_points(
    points: Iterable[Sequence[Scalar] | object], *, reject_duplicates: bool = True
) -> tuple[Point3, ...]:
    """Normalize and lexicographically sort exact points."""

    return normalize_points(
        points, canonical=True, reject_duplicates=reject_duplicates
    )


def add(left: Sequence[Scalar], right: Sequence[Scalar]) -> Point3:
    """Return the exact component-wise sum of two 3-vectors."""

    left_point = normalize_point(left)
    right_point = normalize_point(right)
    return tuple(a + b for a, b in zip(left_point, right_point))  # type: ignore[return-value]


def subtract(left: Sequence[Scalar], right: Sequence[Scalar]) -> Point3:
    """Return the exact component-wise difference of two 3-vectors."""

    left_point = normalize_point(left)
    right_point = normalize_point(right)
    return tuple(a - b for a, b in zip(left_point, right_point))  # type: ignore[return-value]


def scale(value: Scalar, vector: Sequence[Scalar]) -> Point3:
    """Multiply a 3-vector by an exact scalar."""

    factor = normalize_scalar(value)
    point = normalize_point(vector)
    return tuple(factor * coordinate for coordinate in point)  # type: ignore[return-value]


def dot(left: Sequence[Scalar], right: Sequence[Scalar]) -> Fraction:
    """Return the exact Euclidean dot product."""

    left_point = normalize_point(left)
    right_point = normalize_point(right)
    return sum((a * b for a, b in zip(left_point, right_point)), Fraction(0))


def squared_distance(left: Sequence[Scalar], right: Sequence[Scalar]) -> Fraction:
    """Return an exact squared Euclidean distance."""

    delta = subtract(left, right)
    return dot(delta, delta)


def _matrix_rank(rows: Iterable[Iterable[Scalar]]) -> int:
    """Compute a rational row rank by deterministic Gaussian elimination."""

    matrix = [list(map(normalize_scalar, row)) for row in rows]
    if not matrix:
        return 0
    width = len(matrix[0])
    if any(len(row) != width for row in matrix):
        raise ValueError("matrix rows must have equal length")
    pivot_row = 0
    for column in range(width):
        pivot = next(
            (row for row in range(pivot_row, len(matrix)) if matrix[row][column]),
            None,
        )
        if pivot is None:
            continue
        matrix[pivot_row], matrix[pivot] = matrix[pivot], matrix[pivot_row]
        pivot_value = matrix[pivot_row][column]
        matrix[pivot_row] = [value / pivot_value for value in matrix[pivot_row]]
        for row in range(len(matrix)):
            if row == pivot_row or not matrix[row][column]:
                continue
            factor = matrix[row][column]
            matrix[row] = [
                value - factor * pivot_value
                for value, pivot_value in zip(matrix[row], matrix[pivot_row])
            ]
        pivot_row += 1
        if pivot_row == len(matrix):
            break
    return pivot_row


def affine_dimension(points: Iterable[Sequence[Scalar] | object]) -> int:
    """Return the affine dimension 0, 1, 2, or 3 of a non-empty set."""

    normalized = normalize_points(points)
    if not normalized:
        raise ExactInputError("affine dimension is undefined for an empty set")
    origin = normalized[0]
    return _matrix_rank(subtract(point, origin) for point in normalized[1:])
