"""Independent exact oracle for bounded ordinary Voronoi diagrams.

The implementation is deliberately separate from the production half-space
kernel.  It decodes canonical IEEE-754 binary64 words with integer arithmetic,
uses :class:`fractions.Fraction` throughout, and constructs every non-empty
cell intersection directly from its affine equalities and inequalities.

The supported proof domain is intentionally small: one through eight distinct
canonical sites and the adjacent-finite, strictly padded Phase 8 box.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from fractions import Fraction
from itertools import combinations
from math import comb
from typing import ClassVar, Sequence, TypeAlias


Binary64Word: TypeAlias = str
PointWords: TypeAlias = tuple[Binary64Word, Binary64Word, Binary64Word]
OmegaWords: TypeAlias = tuple[PointWords, PointWords]
ExactPoint: TypeAlias = tuple[Fraction, Fraction, Fraction]

_UINT64_MASK = (1 << 64) - 1
_SIGN_MASK = 1 << 63
_FRACTION_MASK = (1 << 52) - 1
_EXPONENT_MASK = 0x7FF
_POSITIVE_MAXIMUM_FINITE_BITS = 0x7FEFFFFFFFFFFFFF
_NEGATIVE_MAXIMUM_FINITE_BITS = 0xFFEFFFFFFFFFFFFF


class OrdinaryDiagramOracleDecision(str, Enum):
    """Transactional outcome of the bounded oracle."""

    COMPLETE = "complete"
    INSUFFICIENT_BUDGET = "insufficient_budget"


class OrdinaryDiagramContactKind(str, Enum):
    """Semantic kind of one non-singleton cell intersection."""

    NONCANONICAL_QUOTIENT_CONTACT = "noncanonical_quotient_contact"
    NATURAL_FACE = "natural_face"
    NATURAL_EDGE = "natural_edge"
    NATURAL_VERTEX = "natural_vertex"
    BOX_SUPPORTED_CONTACT = "box_supported_contact"


@dataclass(frozen=True, slots=True)
class OrdinaryDiagramOracleBudget:
    """Nine independent preflight capacities for the ``n <= 8`` oracle."""

    trusted_maximum_subset_count: ClassVar[int] = 255
    trusted_maximum_equality_count: ClassVar[int] = 769
    trusted_maximum_inequality_count: ClassVar[int] = 2546
    trusted_maximum_candidate_system_count: ClassVar[int] = 13349
    trusted_maximum_form_evaluation_count: ClassVar[int] = 142982
    trusted_maximum_distance_evaluation_count: ClassVar[int] = 108768
    trusted_maximum_cell_vertex_reference_count: ClassVar[int] = 2288
    trusted_maximum_contact_vertex_reference_count: ClassVar[int] = 11061
    trusted_maximum_witness_count: ClassVar[int] = 247

    maximum_subset_count: int = trusted_maximum_subset_count
    maximum_equality_count: int = trusted_maximum_equality_count
    maximum_inequality_count: int = trusted_maximum_inequality_count
    maximum_candidate_system_count: int = trusted_maximum_candidate_system_count
    maximum_form_evaluation_count: int = trusted_maximum_form_evaluation_count
    maximum_distance_evaluation_count: int = (
        trusted_maximum_distance_evaluation_count
    )
    maximum_cell_vertex_reference_count: int = (
        trusted_maximum_cell_vertex_reference_count
    )
    maximum_contact_vertex_reference_count: int = (
        trusted_maximum_contact_vertex_reference_count
    )
    maximum_witness_count: int = trusted_maximum_witness_count


@dataclass(frozen=True, slots=True)
class OrdinaryDiagramOracleRequirements:
    """Conservative, input-size-derived capacities checked before geometry."""

    point_count: int
    conservative_subset_count: int
    conservative_equality_count: int
    conservative_inequality_count: int
    conservative_candidate_system_count: int
    conservative_form_evaluation_count: int
    conservative_distance_evaluation_count: int
    conservative_cell_vertex_reference_count: int
    conservative_contact_vertex_reference_count: int
    conservative_witness_count: int


@dataclass(frozen=True, slots=True)
class OrdinaryDiagramOracleAudit:
    """Actual exact work and payload counts of a complete oracle result."""

    subset_count: int = 0
    equality_count: int = 0
    inequality_count: int = 0
    candidate_system_count: int = 0
    form_evaluation_count: int = 0
    distance_evaluation_count: int = 0
    cell_vertex_reference_count: int = 0
    contact_vertex_reference_count: int = 0
    witness_count: int = 0


@dataclass(frozen=True, slots=True, order=True)
class OrdinaryDiagramCellVertex:
    """One exact vertex in one owner's clipped ordinary cell."""

    position: ExactPoint
    shell_ids: tuple[int, ...]
    box_mask: int


@dataclass(frozen=True, slots=True)
class OrdinaryDiagramCell:
    """Semantic cell projection, independent of boundary-plane ordering."""

    owner_id: int
    vertices: tuple[OrdinaryDiagramCellVertex, ...]


@dataclass(frozen=True, slots=True, order=True)
class OrdinaryDiagramGlobalVertex:
    """A position merged across all and only its nearest-site cells."""

    position: ExactPoint
    nearest_squared_distance: Fraction
    shell_ids: tuple[int, ...]
    box_mask: int
    owner_ids: tuple[int, ...]


@dataclass(frozen=True, slots=True)
class OrdinaryDiagramContact:
    """Directly reconstructed non-singleton intersection ``K_Q``."""

    query_ids: tuple[int, ...]
    carrier_ids: tuple[int, ...]
    vertex_positions: tuple[ExactPoint, ...]
    affine_dimension: int
    site_affine_rank: int
    box_mask: int
    witness: ExactPoint
    witness_squared_distance: Fraction
    kind: OrdinaryDiagramContactKind


@dataclass(frozen=True, slots=True)
class OrdinaryDiagramOracleResult:
    """Immutable bounded oracle receipt and its semantic projection."""

    schema: ClassVar[str] = (
        "morsehgp3d.phase8.independent_bounded_ordinary_subset_oracle.v1"
    )
    scope: ClassVar[str] = "bounded_n8_independent_affine_subset_oracle_only"

    decision: OrdinaryDiagramOracleDecision
    canonical_point_words: tuple[PointWords, ...]
    omega_words: OmegaWords
    requirements: OrdinaryDiagramOracleRequirements
    audit: OrdinaryDiagramOracleAudit
    cells: tuple[OrdinaryDiagramCell, ...] = ()
    global_vertices: tuple[OrdinaryDiagramGlobalVertex, ...] = ()
    contacts: tuple[OrdinaryDiagramContact, ...] = ()

    def semantic_projection(self) -> dict[str, object]:
        """Return the order/index-free shape used by the C++ differential."""

        cells = tuple(
            (
                cell.owner_id,
                tuple(
                    (vertex.position, vertex.shell_ids, vertex.box_mask)
                    for vertex in cell.vertices
                ),
            )
            for cell in self.cells
        )
        global_vertices = tuple(
            (
                vertex.position,
                vertex.nearest_squared_distance,
                vertex.shell_ids,
                vertex.box_mask,
                vertex.owner_ids,
            )
            for vertex in self.global_vertices
        )
        contacts = tuple(
            (
                contact.query_ids,
                contact.carrier_ids,
                contact.vertex_positions,
                contact.affine_dimension,
                contact.site_affine_rank,
                contact.box_mask,
                contact.witness,
                contact.witness_squared_distance,
                contact.kind.value,
            )
            for contact in self.contacts
        )
        return {
            "canonical_point_bits": self.canonical_point_words,
            "omega": self.omega_words,
            "cells": cells,
            "global_vertices": global_vertices,
            "contacts": contacts,
        }


@dataclass(frozen=True, slots=True)
class _AffineForm:
    normal: ExactPoint
    offset: Fraction

    def evaluate(self, point: ExactPoint) -> Fraction:
        return sum(
            (self.normal[axis] * point[axis] for axis in range(3)),
            self.offset,
        )


@dataclass(frozen=True, slots=True)
class _AffineParameterization:
    origin: ExactPoint
    basis: tuple[ExactPoint, ...]


@dataclass(slots=True)
class _MutableAudit:
    subset_count: int = 0
    equality_count: int = 0
    inequality_count: int = 0
    candidate_system_count: int = 0
    form_evaluation_count: int = 0
    distance_evaluation_count: int = 0
    cell_vertex_reference_count: int = 0
    contact_vertex_reference_count: int = 0
    witness_count: int = 0

    def freeze(self) -> OrdinaryDiagramOracleAudit:
        return OrdinaryDiagramOracleAudit(
            subset_count=self.subset_count,
            equality_count=self.equality_count,
            inequality_count=self.inequality_count,
            candidate_system_count=self.candidate_system_count,
            form_evaluation_count=self.form_evaluation_count,
            distance_evaluation_count=self.distance_evaluation_count,
            cell_vertex_reference_count=self.cell_vertex_reference_count,
            contact_vertex_reference_count=self.contact_vertex_reference_count,
            witness_count=self.witness_count,
        )


def _bits_from_word(word: Binary64Word) -> int:
    if not isinstance(word, str) or len(word) != 16 or word.lower() != word:
        raise ValueError("a binary64 word must be sixteen lowercase hex digits")
    try:
        bits = int(word, 16)
    except ValueError as error:
        raise ValueError("a binary64 word contains a non-hex digit") from error
    if not 0 <= bits <= _UINT64_MASK:
        raise ValueError("a binary64 word escaped uint64")
    exponent = (bits >> 52) & _EXPONENT_MASK
    if exponent == _EXPONENT_MASK:
        raise ValueError("non-finite binary64 words are outside the oracle")
    return bits


def _canonical_word(word: Binary64Word) -> Binary64Word:
    bits = _bits_from_word(word)
    if bits == _SIGN_MASK:
        bits = 0
    return f"{bits:016x}"


def _fraction_from_word(word: Binary64Word) -> Fraction:
    bits = _bits_from_word(word)
    sign = -1 if bits & _SIGN_MASK else 1
    exponent = (bits >> 52) & _EXPONENT_MASK
    fraction = bits & _FRACTION_MASK
    if exponent == 0:
        significand = fraction
        power = -1074
    else:
        significand = (1 << 52) | fraction
        power = exponent - 1023 - 52
    if significand == 0:
        return Fraction()
    numerator = sign * significand
    if power >= 0:
        return Fraction(numerator << power)
    return Fraction(numerator, 1 << (-power))


def _predecessor_word(word: Binary64Word) -> Binary64Word:
    bits = _bits_from_word(_canonical_word(word))
    if bits == _NEGATIVE_MAXIMUM_FINITE_BITS:
        raise ValueError("a site minimum has no finite binary64 predecessor")
    if bits == 0:
        return f"{(_SIGN_MASK | 1):016x}"
    predecessor = bits + 1 if bits & _SIGN_MASK else bits - 1
    return _canonical_word(f"{predecessor:016x}")


def _successor_word(word: Binary64Word) -> Binary64Word:
    bits = _bits_from_word(_canonical_word(word))
    if bits == _POSITIVE_MAXIMUM_FINITE_BITS:
        raise ValueError("a site maximum has no finite binary64 successor")
    successor = bits - 1 if bits & _SIGN_MASK else bits + 1
    return _canonical_word(f"{successor:016x}")


def _normalize_manifest(
    canonical_point_words: Sequence[Sequence[Binary64Word]],
) -> tuple[tuple[PointWords, ...], tuple[ExactPoint, ...]]:
    manifest: list[PointWords] = []
    points: list[ExactPoint] = []
    for point_index, point_words in enumerate(canonical_point_words):
        if len(point_words) != 3:
            raise ValueError(f"point {point_index} must contain exactly three words")
        canonical = tuple(_canonical_word(word) for word in point_words)
        if tuple(point_words) != canonical:
            raise ValueError("canonical point words may not contain negative zero")
        exact = tuple(_fraction_from_word(word) for word in canonical)
        manifest.append(canonical)  # type: ignore[arg-type]
        points.append(exact)  # type: ignore[arg-type]
    if not 1 <= len(points) <= 8:
        raise ValueError("the bounded ordinary oracle accepts one to eight sites")
    if any(points[index - 1] >= points[index] for index in range(1, len(points))):
        raise ValueError("point words must be distinct and canonically ordered")
    return tuple(manifest), tuple(points)


def _normalize_omega(
    omega_words: Sequence[Sequence[Binary64Word]] | Sequence[Binary64Word],
    manifest: tuple[PointWords, ...],
    points: tuple[ExactPoint, ...],
) -> tuple[OmegaWords, ExactPoint, ExactPoint]:
    if len(omega_words) == 2 and all(
        not isinstance(part, str) and len(part) == 3 for part in omega_words
    ):
        lower_source = omega_words[0]
        upper_source = omega_words[1]
    elif len(omega_words) == 6 and all(
        isinstance(word, str) for word in omega_words
    ):
        lower_source = omega_words[:3]
        upper_source = omega_words[3:]
    else:
        raise ValueError("omega words must be (lower3, upper3) or six flat words")
    lower_words = tuple(_canonical_word(word) for word in lower_source)
    upper_words = tuple(_canonical_word(word) for word in upper_source)
    if tuple(lower_source) != lower_words or tuple(upper_source) != upper_words:
        raise ValueError("omega words must use canonical signed-zero spelling")

    expected_lower: list[Binary64Word] = []
    expected_upper: list[Binary64Word] = []
    for axis in range(3):
        minimum_index = min(range(len(points)), key=lambda index: points[index][axis])
        maximum_index = max(range(len(points)), key=lambda index: points[index][axis])
        expected_lower.append(_predecessor_word(manifest[minimum_index][axis]))
        expected_upper.append(_successor_word(manifest[maximum_index][axis]))
    if lower_words != tuple(expected_lower) or upper_words != tuple(expected_upper):
        raise ValueError("omega is not the adjacent finite padding of the point cloud")

    lower = tuple(_fraction_from_word(word) for word in lower_words)
    upper = tuple(_fraction_from_word(word) for word in upper_words)
    if any(lower[axis] >= upper[axis] for axis in range(3)):
        raise ValueError("omega must have three strict finite intervals")
    if any(
        not lower[axis] < point[axis] < upper[axis]
        for point in points
        for axis in range(3)
    ):
        raise ValueError("every canonical site must lie strictly inside omega")
    normalized: OmegaWords = (lower_words, upper_words)  # type: ignore[assignment]
    return normalized, lower, upper  # type: ignore[return-value]


def _requirements_for(point_count: int) -> OrdinaryDiagramOracleRequirements:
    subset_count = (1 << point_count) - 1
    equality_count = sum(
        comb(point_count, size) * (size - 1)
        for size in range(1, point_count + 1)
    )
    inequality_count = sum(
        comb(point_count, size) * (6 + point_count - size)
        for size in range(1, point_count + 1)
    )
    cell_vertex_references = point_count * comb(point_count + 5, 3)
    contact_vertex_references = sum(
        comb(point_count, size) * comb(6 + point_count - size, 2)
        for size in range(2, point_count + 1)
    )
    candidate_systems = cell_vertex_references + contact_vertex_references
    form_evaluations = (
        cell_vertex_references * (point_count + 5)
        + sum(
            comb(point_count, size)
            * comb(6 + point_count - size, 2)
            * (6 + point_count - size)
            for size in range(2, point_count + 1)
        )
    )
    witness_count = subset_count - point_count
    distance_evaluations = point_count * (
        cell_vertex_references + contact_vertex_references + witness_count
    )
    return OrdinaryDiagramOracleRequirements(
        point_count=point_count,
        conservative_subset_count=subset_count,
        conservative_equality_count=equality_count,
        conservative_inequality_count=inequality_count,
        conservative_candidate_system_count=candidate_systems,
        conservative_form_evaluation_count=form_evaluations,
        conservative_distance_evaluation_count=distance_evaluations,
        conservative_cell_vertex_reference_count=cell_vertex_references,
        conservative_contact_vertex_reference_count=contact_vertex_references,
        conservative_witness_count=witness_count,
    )


_BUDGET_FIELDS = (
    ("maximum_subset_count", "conservative_subset_count"),
    ("maximum_equality_count", "conservative_equality_count"),
    ("maximum_inequality_count", "conservative_inequality_count"),
    ("maximum_candidate_system_count", "conservative_candidate_system_count"),
    ("maximum_form_evaluation_count", "conservative_form_evaluation_count"),
    ("maximum_distance_evaluation_count", "conservative_distance_evaluation_count"),
    (
        "maximum_cell_vertex_reference_count",
        "conservative_cell_vertex_reference_count",
    ),
    (
        "maximum_contact_vertex_reference_count",
        "conservative_contact_vertex_reference_count",
    ),
    ("maximum_witness_count", "conservative_witness_count"),
)


def _validate_budget(budget: OrdinaryDiagramOracleBudget) -> None:
    for budget_name, _ in _BUDGET_FIELDS:
        value = getattr(budget, budget_name)
        trusted_name = "trusted_" + budget_name
        trusted = getattr(OrdinaryDiagramOracleBudget, trusted_name)
        if isinstance(value, bool) or not isinstance(value, int) or value < 0:
            raise ValueError(f"{budget_name} must be a nonnegative integer")
        if value > trusted:
            raise ValueError(f"{budget_name} exceeds the bounded n8 trust cap")


def _budget_covers(
    budget: OrdinaryDiagramOracleBudget,
    requirements: OrdinaryDiagramOracleRequirements,
) -> bool:
    return all(
        getattr(budget, budget_name) >= getattr(requirements, requirement_name)
        for budget_name, requirement_name in _BUDGET_FIELDS
    )


def _subtract(left: ExactPoint, right: ExactPoint) -> ExactPoint:
    return tuple(left[axis] - right[axis] for axis in range(3))  # type: ignore[return-value]


def _dot(left: ExactPoint, right: ExactPoint) -> Fraction:
    return sum((left[axis] * right[axis] for axis in range(3)), Fraction())


def _squared_norm(point: ExactPoint) -> Fraction:
    return _dot(point, point)


def _squared_distance(left: ExactPoint, right: ExactPoint) -> Fraction:
    return _squared_norm(_subtract(left, right))


def _distance_difference(left: ExactPoint, right: ExactPoint) -> _AffineForm:
    normal = tuple(2 * (right[axis] - left[axis]) for axis in range(3))
    return _AffineForm(
        normal,  # type: ignore[arg-type]
        _squared_norm(left) - _squared_norm(right),
    )


def _box_inequalities(lower: ExactPoint, upper: ExactPoint) -> tuple[_AffineForm, ...]:
    forms: list[_AffineForm] = []
    for axis in range(3):
        lower_normal = [Fraction(), Fraction(), Fraction()]
        lower_normal[axis] = Fraction(-1)
        forms.append(_AffineForm(tuple(lower_normal), lower[axis]))  # type: ignore[arg-type]
        upper_normal = [Fraction(), Fraction(), Fraction()]
        upper_normal[axis] = Fraction(1)
        forms.append(_AffineForm(tuple(upper_normal), -upper[axis]))  # type: ignore[arg-type]
    return tuple(forms)


def _rref_parameterization(
    equalities: Sequence[_AffineForm],
) -> _AffineParameterization | None:
    matrix = [
        [*equality.normal, -equality.offset]
        for equality in equalities
    ]
    pivot_columns: list[int] = []
    pivot_row = 0
    for column in range(3):
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
                value - factor * pivot_entry
                for value, pivot_entry in zip(matrix[row], matrix[pivot_row])
            ]
        pivot_columns.append(column)
        pivot_row += 1
        if pivot_row == len(matrix):
            break
    for row in matrix:
        if not any(row[column] for column in range(3)) and row[3]:
            return None

    origin = [Fraction(), Fraction(), Fraction()]
    for row, column in enumerate(pivot_columns):
        origin[column] = matrix[row][3]
    free_columns = tuple(column for column in range(3) if column not in pivot_columns)
    basis: list[ExactPoint] = []
    for free_column in free_columns:
        direction = [Fraction(), Fraction(), Fraction()]
        direction[free_column] = Fraction(1)
        for row, pivot_column in enumerate(pivot_columns):
            direction[pivot_column] = -matrix[row][free_column]
        basis.append(tuple(direction))  # type: ignore[arg-type]
    return _AffineParameterization(
        tuple(origin),  # type: ignore[arg-type]
        tuple(basis),
    )


def _solve_square(
    coefficients: Sequence[Sequence[Fraction]],
    right_hand_side: Sequence[Fraction],
) -> tuple[Fraction, ...] | None:
    dimension = len(coefficients)
    if dimension == 0:
        return ()
    if len(right_hand_side) != dimension or any(
        len(row) != dimension for row in coefficients
    ):
        raise AssertionError("an internal active system is not square")
    matrix = [
        [*coefficients[row], right_hand_side[row]]
        for row in range(dimension)
    ]
    for column in range(dimension):
        pivot = next(
            (row for row in range(column, dimension) if matrix[row][column]),
            None,
        )
        if pivot is None:
            return None
        matrix[column], matrix[pivot] = matrix[pivot], matrix[column]
        pivot_value = matrix[column][column]
        matrix[column] = [value / pivot_value for value in matrix[column]]
        for row in range(dimension):
            if row == column or not matrix[row][column]:
                continue
            factor = matrix[row][column]
            matrix[row] = [
                value - factor * pivot_entry
                for value, pivot_entry in zip(matrix[row], matrix[column])
            ]
    return tuple(matrix[row][dimension] for row in range(dimension))


def _enumerate_vertices(
    equalities: tuple[_AffineForm, ...],
    inequalities: tuple[_AffineForm, ...],
    audit: _MutableAudit,
) -> tuple[ExactPoint, ...]:
    parameterization = _rref_parameterization(equalities)
    if parameterization is None:
        return ()
    origin = parameterization.origin
    basis = parameterization.basis
    dimension = len(basis)
    pulled_coefficients = tuple(
        tuple(_dot(inequality.normal, direction) for direction in basis)
        for inequality in inequalities
    )
    pulled_offsets = tuple(inequality.evaluate(origin) for inequality in inequalities)

    vertices: set[ExactPoint] = set()
    for active_indices in combinations(range(len(inequalities)), dimension):
        audit.candidate_system_count += 1
        coefficients = tuple(pulled_coefficients[index] for index in active_indices)
        right_hand_side = tuple(-pulled_offsets[index] for index in active_indices)
        parameters = _solve_square(coefficients, right_hand_side)
        if parameters is None:
            continue
        candidate = tuple(
            origin[axis]
            + sum(
                (
                    parameters[basis_index] * basis[basis_index][axis]
                    for basis_index in range(dimension)
                ),
                Fraction(),
            )
            for axis in range(3)
        )
        values = tuple(inequality.evaluate(candidate) for inequality in inequalities)
        audit.form_evaluation_count += len(values)
        if all(value <= 0 for value in values):
            vertices.add(candidate)  # type: ignore[arg-type]
    return tuple(sorted(vertices))


def _matrix_rank(rows: Sequence[Sequence[Fraction]]) -> int:
    matrix = [list(row) for row in rows]
    if not matrix:
        return 0
    width = len(matrix[0])
    if any(len(row) != width for row in matrix):
        raise AssertionError("an internal rank matrix is ragged")
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
        for row in range(pivot_row + 1, len(matrix)):
            if not matrix[row][column]:
                continue
            factor = matrix[row][column]
            matrix[row] = [
                value - factor * pivot_entry
                for value, pivot_entry in zip(matrix[row], matrix[pivot_row])
            ]
        pivot_row += 1
        if pivot_row == len(matrix):
            break
    return pivot_row


def _affine_dimension(points: Sequence[ExactPoint]) -> int:
    if not points:
        raise ValueError("an empty exact point family has no affine dimension")
    origin = points[0]
    return _matrix_rank(tuple(_subtract(point, origin) for point in points[1:]))


def _nearest_shell(
    points: tuple[ExactPoint, ...],
    query: ExactPoint,
    audit: _MutableAudit,
) -> tuple[Fraction, tuple[int, ...]]:
    distances = tuple(_squared_distance(point, query) for point in points)
    audit.distance_evaluation_count += len(points)
    minimum = min(distances)
    shell = tuple(
        point_id for point_id, distance in enumerate(distances) if distance == minimum
    )
    return minimum, shell


def _box_mask(position: ExactPoint, lower: ExactPoint, upper: ExactPoint) -> int:
    mask = 0
    for axis in range(3):
        if not lower[axis] <= position[axis] <= upper[axis]:
            raise AssertionError("an oracle vertex escaped omega")
        if position[axis] == lower[axis]:
            mask |= 1 << (2 * axis)
        if position[axis] == upper[axis]:
            mask |= 1 << (2 * axis + 1)
    return mask


def _average(points: Sequence[ExactPoint]) -> ExactPoint:
    if not points:
        raise ValueError("an empty exact point family has no barycenter")
    divisor = Fraction(len(points))
    return tuple(
        sum((point[axis] for point in points), Fraction()) / divisor
        for axis in range(3)
    )  # type: ignore[return-value]


def _audit_within_requirements(
    audit: OrdinaryDiagramOracleAudit,
    requirements: OrdinaryDiagramOracleRequirements,
) -> bool:
    return all(
        getattr(audit, audit_name) <= getattr(requirements, requirement_name)
        for audit_name, requirement_name in (
            ("subset_count", "conservative_subset_count"),
            ("equality_count", "conservative_equality_count"),
            ("inequality_count", "conservative_inequality_count"),
            ("candidate_system_count", "conservative_candidate_system_count"),
            ("form_evaluation_count", "conservative_form_evaluation_count"),
            ("distance_evaluation_count", "conservative_distance_evaluation_count"),
            (
                "cell_vertex_reference_count",
                "conservative_cell_vertex_reference_count",
            ),
            (
                "contact_vertex_reference_count",
                "conservative_contact_vertex_reference_count",
            ),
            ("witness_count", "conservative_witness_count"),
        )
    )


def build_bounded_ordinary_subset_oracle(
    canonical_point_words: Sequence[Sequence[Binary64Word]],
    omega_words: Sequence[Sequence[Binary64Word]] | Sequence[Binary64Word],
    budget: OrdinaryDiagramOracleBudget | None = None,
) -> OrdinaryDiagramOracleResult:
    """Build every bounded ``K_Q`` independently for one canonical cloud.

    ``omega_words`` accepts either ``(lower_xyz, upper_xyz)`` or the flat order
    ``lower_x, lower_y, lower_z, upper_x, upper_y, upper_z``.  An insufficient
    preflight returns an identity-bound receipt with no geometric payload.
    """

    effective_budget = budget or OrdinaryDiagramOracleBudget()
    _validate_budget(effective_budget)
    manifest, points = _normalize_manifest(canonical_point_words)
    normalized_omega, lower, upper = _normalize_omega(
        omega_words, manifest, points
    )
    requirements = _requirements_for(len(points))
    if not _budget_covers(effective_budget, requirements):
        return OrdinaryDiagramOracleResult(
            decision=OrdinaryDiagramOracleDecision.INSUFFICIENT_BUDGET,
            canonical_point_words=manifest,
            omega_words=normalized_omega,
            requirements=requirements,
            audit=OrdinaryDiagramOracleAudit(),
        )

    audit = _MutableAudit()
    box_inequalities = _box_inequalities(lower, upper)
    raw_cells: list[tuple[int, tuple[ExactPoint, ...]]] = []
    direct_contacts: list[tuple[tuple[int, ...], tuple[ExactPoint, ...]]] = []
    point_ids = tuple(range(len(points)))
    for subset_size in range(1, len(points) + 1):
        for query_ids in combinations(point_ids, subset_size):
            audit.subset_count += 1
            anchor_id = query_ids[0]
            equalities = tuple(
                _distance_difference(points[anchor_id], points[query_id])
                for query_id in query_ids[1:]
            )
            query_set = set(query_ids)
            inequalities = box_inequalities + tuple(
                _distance_difference(points[anchor_id], points[point_id])
                for point_id in point_ids
                if point_id not in query_set
            )
            audit.equality_count += len(equalities)
            audit.inequality_count += len(inequalities)
            vertices = _enumerate_vertices(equalities, inequalities, audit)
            if subset_size == 1:
                if not vertices:
                    raise AssertionError("a canonical site's ordinary cell is empty")
                audit.cell_vertex_reference_count += len(vertices)
                raw_cells.append((anchor_id, vertices))
            elif vertices:
                audit.contact_vertex_reference_count += len(vertices)
                direct_contacts.append((query_ids, vertices))

    owner_sets: dict[ExactPoint, set[int]] = {}
    for owner_id, vertices in raw_cells:
        for position in vertices:
            owner_sets.setdefault(position, set()).add(owner_id)

    global_by_position: dict[ExactPoint, OrdinaryDiagramGlobalVertex] = {}
    for position, owners_set in sorted(owner_sets.items()):
        distance, shell_ids = _nearest_shell(points, position, audit)
        owner_ids = tuple(sorted(owners_set))
        if owner_ids != shell_ids:
            raise AssertionError(
                "independent cell occurrences disagree with the exact nearest shell"
            )
        global_by_position[position] = OrdinaryDiagramGlobalVertex(
            position=position,
            nearest_squared_distance=distance,
            shell_ids=shell_ids,
            box_mask=_box_mask(position, lower, upper),
            owner_ids=owner_ids,
        )

    cells: list[OrdinaryDiagramCell] = []
    for owner_id, positions in raw_cells:
        vertices: list[OrdinaryDiagramCellVertex] = []
        for position in positions:
            global_vertex = global_by_position[position]
            if owner_id not in global_vertex.shell_ids:
                raise AssertionError("a local cell vertex omits its owner from the shell")
            vertices.append(
                OrdinaryDiagramCellVertex(
                    position=position,
                    shell_ids=global_vertex.shell_ids,
                    box_mask=global_vertex.box_mask,
                )
            )
        cells.append(OrdinaryDiagramCell(owner_id, tuple(sorted(vertices))))

    contacts: list[OrdinaryDiagramContact] = []
    for query_ids, direct_vertices in direct_contacts:
        derived_vertices = tuple(
            position
            for position, vertex in global_by_position.items()
            if set(query_ids).issubset(vertex.shell_ids)
        )
        if direct_vertices != tuple(sorted(derived_vertices)):
            raise AssertionError(
                "direct K_Q vertices disagree with the global shell projection"
            )
        carrier_set = set(point_ids)
        common_mask = 0x3F
        for position in direct_vertices:
            vertex = global_by_position[position]
            carrier_set.intersection_update(vertex.shell_ids)
            common_mask &= vertex.box_mask
        carrier_ids = tuple(sorted(carrier_set))
        witness = _average(direct_vertices)
        witness_distance, witness_shell = _nearest_shell(points, witness, audit)
        audit.witness_count += 1
        if witness_shell != carrier_ids:
            raise AssertionError("a direct contact barycenter disagrees with its carrier")
        dimension = _affine_dimension(direct_vertices)
        site_rank = _affine_dimension(tuple(points[point_id] for point_id in carrier_ids))
        if query_ids != carrier_ids:
            kind = OrdinaryDiagramContactKind.NONCANONICAL_QUOTIENT_CONTACT
        elif common_mask:
            kind = OrdinaryDiagramContactKind.BOX_SUPPORTED_CONTACT
        else:
            if site_rank + dimension != 3:
                raise AssertionError(
                    "a natural direct contact violates rank-dimension duality"
                )
            kind = {
                1: OrdinaryDiagramContactKind.NATURAL_FACE,
                2: OrdinaryDiagramContactKind.NATURAL_EDGE,
                3: OrdinaryDiagramContactKind.NATURAL_VERTEX,
            }.get(site_rank)
            if kind is None:
                raise AssertionError("a natural contact has an invalid site rank")
        contacts.append(
            OrdinaryDiagramContact(
                query_ids=query_ids,
                carrier_ids=carrier_ids,
                vertex_positions=direct_vertices,
                affine_dimension=dimension,
                site_affine_rank=site_rank,
                box_mask=common_mask,
                witness=witness,
                witness_squared_distance=witness_distance,
                kind=kind,
            )
        )

    frozen_audit = audit.freeze()
    if not _audit_within_requirements(frozen_audit, requirements):
        raise AssertionError("ordinary subset oracle work exceeded its preflight")
    return OrdinaryDiagramOracleResult(
        decision=OrdinaryDiagramOracleDecision.COMPLETE,
        canonical_point_words=manifest,
        omega_words=normalized_omega,
        requirements=requirements,
        audit=frozen_audit,
        cells=tuple(cells),
        global_vertices=tuple(global_by_position.values()),
        contacts=tuple(contacts),
    )


__all__ = [
    "Binary64Word",
    "ExactPoint",
    "OmegaWords",
    "OrdinaryDiagramCell",
    "OrdinaryDiagramCellVertex",
    "OrdinaryDiagramContact",
    "OrdinaryDiagramContactKind",
    "OrdinaryDiagramGlobalVertex",
    "OrdinaryDiagramOracleAudit",
    "OrdinaryDiagramOracleBudget",
    "OrdinaryDiagramOracleDecision",
    "OrdinaryDiagramOracleRequirements",
    "OrdinaryDiagramOracleResult",
    "PointWords",
    "build_bounded_ordinary_subset_oracle",
]
