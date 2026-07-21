#!/usr/bin/env python3
"""Short exact differential for the bounded Phase 8 ordinary diagram."""

from __future__ import annotations

import argparse
import json
import math
import re
import struct
import subprocess
import sys
from dataclasses import dataclass
from fractions import Fraction
from itertools import combinations
from pathlib import Path
from typing import TypeAlias


REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
if str(REPOSITORY_ROOT) not in sys.path:
    sys.path.insert(0, str(REPOSITORY_ROOT))

from reference.morsehgp3d_oracle.ordinary_diagram import (
    OrdinaryDiagramOracleDecision,
    build_bounded_ordinary_subset_oracle,
)


INPUT_HEADER = "morsehgp3d-phase8-ordinary-diagram-input-v1"
OUTPUT_HEADER = "morsehgp3d-phase8-ordinary-diagram-output-v1"
RESULT_SCHEMA = "morsehgp3d.phase8.exact_bounded_ordinary_diagram_closure.v1"
DEFAULT_TIMEOUT_SECONDS = 40
UINT64_MASK = (1 << 64) - 1
SIGN_MASK = 1 << 63
EXPONENT_MASK = 0x7FF << 52
POSITIVE_MAXIMUM_FINITE_BITS = 0x7FEFFFFFFFFFFFFF
NEGATIVE_MAXIMUM_FINITE_BITS = 0xFFEFFFFFFFFFFFFF
CASE_NAME_PATTERN = re.compile(r"[A-Za-z0-9_-]+\Z")

PointWords: TypeAlias = tuple[str, str, str]
ExactPoint: TypeAlias = tuple[Fraction, Fraction, Fraction]


@dataclass(frozen=True, slots=True)
class FixedCase:
    name: str
    source_points: tuple[PointWords, ...]
    permutation_of: str | None = None


def _bits_from_word(word: str) -> int:
    if not isinstance(word, str) or len(word) != 16 or word.lower() != word:
        raise AssertionError(f"noncanonical binary64 spelling {word!r}")
    try:
        bits = int(word, 16)
    except ValueError as error:
        raise AssertionError(f"invalid binary64 word {word!r}") from error
    if bits & EXPONENT_MASK == EXPONENT_MASK:
        raise AssertionError("the Phase 8 campaign contains a non-finite word")
    return bits


def _word_from_bits(bits: int) -> str:
    if not 0 <= bits <= UINT64_MASK:
        raise AssertionError("a binary64 word escaped the uint64 domain")
    word = f"{bits:016x}"
    _bits_from_word(word)
    return word


def _binary64_word(value: int | float | Fraction | str) -> str:
    if isinstance(value, str):
        _bits_from_word(value)
        return value
    exact = Fraction(value)
    encoded = float(value)
    if not math.isfinite(encoded) or Fraction.from_float(encoded) != exact:
        raise AssertionError(f"fixture coordinate {value!r} is not exact binary64")
    return struct.pack(">d", encoded).hex()


def _point(
    x: int | float | Fraction | str,
    y: int | float | Fraction | str,
    z: int | float | Fraction | str,
) -> PointWords:
    return (_binary64_word(x), _binary64_word(y), _binary64_word(z))


def _canonical_word(word: str) -> str:
    bits = _bits_from_word(word)
    return "0000000000000000" if bits == SIGN_MASK else word


def _total_order_key(word: str) -> int:
    bits = _bits_from_word(_canonical_word(word))
    return (~bits) & UINT64_MASK if bits & SIGN_MASK else bits ^ SIGN_MASK


def _fraction_from_word(word: str) -> Fraction:
    canonical = _canonical_word(word)
    return Fraction.from_float(struct.unpack(">d", bytes.fromhex(canonical))[0])


def _canonical_manifest(source_points: tuple[PointWords, ...]) -> tuple[PointWords, ...]:
    canonical = [
        tuple(_canonical_word(word) for word in point) for point in source_points
    ]
    canonical.sort(key=lambda point: tuple(_total_order_key(word) for word in point))
    if len(set(canonical)) != len(canonical):
        raise AssertionError("a fixture contains duplicate canonical points")
    return tuple(canonical)  # type: ignore[return-value]


def _predecessor_word(word: str) -> str:
    bits = _bits_from_word(_canonical_word(word))
    if bits == NEGATIVE_MAXIMUM_FINITE_BITS:
        raise AssertionError("a fixture minimum has no finite predecessor")
    if bits == 0:
        return _word_from_bits(SIGN_MASK | 1)
    predecessor = bits + 1 if bits & SIGN_MASK else bits - 1
    return _canonical_word(_word_from_bits(predecessor))


def _successor_word(word: str) -> str:
    bits = _bits_from_word(_canonical_word(word))
    if bits == POSITIVE_MAXIMUM_FINITE_BITS:
        raise AssertionError("a fixture maximum has no finite successor")
    successor = bits - 1 if bits & SIGN_MASK else bits + 1
    return _canonical_word(_word_from_bits(successor))


def _expected_omega(
    canonical_manifest: tuple[PointWords, ...],
) -> tuple[PointWords, PointWords]:
    lower = []
    upper = []
    for axis in range(3):
        axis_words = tuple(point[axis] for point in canonical_manifest)
        minimum = min(axis_words, key=_total_order_key)
        maximum = max(axis_words, key=_total_order_key)
        lower.append(_predecessor_word(minimum))
        upper.append(_successor_word(maximum))
    return tuple(lower), tuple(upper)  # type: ignore[return-value]


def _permuted_case(
    name: str,
    base: FixedCase,
    permutation: tuple[int, ...],
) -> FixedCase:
    if tuple(sorted(permutation)) != tuple(range(len(base.source_points))):
        raise AssertionError("a fixture permutation is not bijective")
    return FixedCase(
        name,
        tuple(base.source_points[index] for index in permutation),
        base.name,
    )


def _fixed_cases() -> tuple[FixedCase, ...]:
    signed_zero_subnormal = FixedCase(
        "signed_zero_subnormal",
        (_point("8000000000000000", "0000000000000001", "8000000000000001"),),
    )
    pair_oblique = FixedCase(
        "pair_oblique",
        (_point(-3, -1, 2), _point(2, 4, -2)),
    )
    base = 1 << 52
    box_supported = FixedCase(
        "box_supported_2p52",
        (
            _point(-2, base + 1, 0),
            _point(2, base + 1, 0),
            _point(-1, base + 2, 0),
        ),
    )
    collinear_four = FixedCase(
        "collinear_four",
        tuple(_point(coordinate, 0, 0) for coordinate in (0, 2, 4, 6)),
    )
    square_points = (
        _point(-1, -1, 0),
        _point(1, -1, 0),
        _point(-1, 1, 0),
        _point(1, 1, 0),
    )
    square_exact = FixedCase("square_exact", square_points)
    square_plus_ulp = FixedCase(
        "square_plus_one_ulp",
        square_points[:-1] + (_point("3ff0000000000001", 1, 0),),
    )
    square_minus_ulp = FixedCase(
        "square_minus_one_ulp",
        square_points[:-1] + (_point("3fefffffffffffff", 1, 0),),
    )
    tetra_points = (
        _point(1, 1, 1),
        _point(1, -1, -1),
        _point(-1, 1, -1),
        _point(-1, -1, 1),
    )
    tetrahedron = FixedCase("tetrahedron", tetra_points)
    tetrahedron_with_center = FixedCase(
        "tetrahedron_with_center", tetra_points + (_point(0, 0, 0),)
    )
    moment_curve = FixedCase(
        "moment_curve_six",
        tuple(_point(value, value * value, value * value * value) for value in range(-2, 4)),
    )
    cube_points = tuple(
        _point(x, y, z)
        for x in (-1, 1)
        for y in (-1, 1)
        for z in (-1, 1)
    )
    cube_minus_one = FixedCase("cube_minus_one", cube_points[:-1])
    cube = FixedCase("cube", cube_points)
    cube_one_ulp = FixedCase(
        "cube_one_ulp",
        cube_points[:-1] + (_point("3ff0000000000001", 1, 1),),
    )
    cases = (
        signed_zero_subnormal,
        pair_oblique,
        box_supported,
        collinear_four,
        square_exact,
        square_plus_ulp,
        square_minus_ulp,
        tetrahedron,
        tetrahedron_with_center,
        moment_curve,
        cube_minus_one,
        cube,
        cube_one_ulp,
        _permuted_case("pair_oblique_permuted", pair_oblique, (1, 0)),
        _permuted_case("square_exact_permuted", square_exact, (3, 1, 0, 2)),
        _permuted_case(
            "moment_curve_six_permuted", moment_curve, (5, 0, 3, 1, 4, 2)
        ),
    )
    if len({case.name for case in cases}) != len(cases):
        raise AssertionError("fixed-case names must be unique")
    if any(CASE_NAME_PATTERN.fullmatch(case.name) is None for case in cases):
        raise AssertionError("fixed-case names must satisfy the batch protocol")
    sizes = {len(case.source_points) for case in cases}
    if not set(range(1, 9)).issubset(sizes):
        raise AssertionError("the fixed matrix must cover every n from one to eight")
    if sum(case.permutation_of is not None for case in cases) != 3:
        raise AssertionError("the fixed matrix must contain exactly three permutations")
    for case in cases:
        _canonical_manifest(case.source_points)
    return cases


def _case_protocol_line(case: FixedCase) -> str:
    encoded_points = ";".join(",".join(point) for point in case.source_points)
    return f"{case.name}|{encoded_points}"


def _canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
    )


def _strict_json_object(line: str) -> dict[str, object]:
    def reject_constant(value: str) -> object:
        raise ValueError(f"non-finite JSON constant {value!r}")

    def reject_duplicates(pairs: list[tuple[str, object]]) -> dict[str, object]:
        result: dict[str, object] = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate JSON key {key!r}")
            result[key] = value
        return result

    value = json.loads(
        line,
        parse_constant=reject_constant,
        object_pairs_hook=reject_duplicates,
    )
    if not isinstance(value, dict):
        raise ValueError("each result line must contain one JSON object")
    if _canonical_json(value) != line:
        raise ValueError("a result line is not compact canonical JSON")
    return value


def _require_keys(
    value: object,
    expected_keys: set[str],
    path: str,
) -> dict[str, object]:
    if not isinstance(value, dict):
        raise AssertionError(f"{path} must be an object")
    observed_keys = set(value)
    if observed_keys != expected_keys:
        raise AssertionError(
            f"{path} has missing keys {sorted(expected_keys - observed_keys)!r} "
            f"and extra keys {sorted(observed_keys - expected_keys)!r}"
        )
    return value


def _require_list(value: object, path: str) -> list[object]:
    if not isinstance(value, list):
        raise AssertionError(f"{path} must be an array")
    return value


def _require_integer(value: object, path: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise AssertionError(f"{path} must be an integer")
    return value


def _canonical_decimal(value: object, path: str) -> int:
    if not isinstance(value, str):
        raise AssertionError(f"{path} must be a decimal string")
    try:
        parsed = int(value)
    except ValueError as error:
        raise AssertionError(f"{path} is not a decimal integer") from error
    if str(parsed) != value:
        raise AssertionError(f"{path} is not a canonical decimal integer")
    return parsed


def _ratio(value: object, path: str) -> Fraction:
    record = _require_keys(value, {"numerator", "denominator"}, path)
    numerator = _canonical_decimal(record["numerator"], f"{path}.numerator")
    denominator = _canonical_decimal(record["denominator"], f"{path}.denominator")
    if denominator <= 0:
        raise AssertionError(f"{path} must have a positive denominator")
    result = Fraction(numerator, denominator)
    if result.numerator != numerator or result.denominator != denominator:
        raise AssertionError(f"{path} must be reduced canonically")
    return result


def _position(value: object, path: str) -> ExactPoint:
    record = _require_keys(value, {"x", "y", "z"}, path)
    return (
        _ratio(record["x"], f"{path}.x"),
        _ratio(record["y"], f"{path}.y"),
        _ratio(record["z"], f"{path}.z"),
    )


def _identifier_set(value: object, point_count: int, path: str) -> tuple[int, ...]:
    identifiers = tuple(
        _require_integer(item, f"{path}[{index}]")
        for index, item in enumerate(_require_list(value, path))
    )
    if any(identifier < 0 or identifier >= point_count for identifier in identifiers):
        raise AssertionError(f"{path} contains an identifier outside the cloud")
    if len(set(identifiers)) != len(identifiers):
        raise AssertionError(f"{path} contains duplicate identifiers")
    return tuple(sorted(identifiers))


def _box_mask(value: object, path: str) -> int:
    mask = _require_integer(value, path)
    if not 0 <= mask <= 0x3F:
        raise AssertionError(f"{path} escapes the six-face mask")
    return mask


def _subtract(left: ExactPoint, right: ExactPoint) -> ExactPoint:
    return tuple(a - b for a, b in zip(left, right))  # type: ignore[return-value]


def _squared_distance(left: ExactPoint, right: ExactPoint) -> Fraction:
    return sum((coordinate * coordinate for coordinate in _subtract(left, right)), Fraction())


def _matrix_rank(rows: list[list[Fraction]]) -> int:
    if not rows:
        return 0
    matrix = [row[:] for row in rows]
    width = len(matrix[0])
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


def _affine_dimension(points: tuple[ExactPoint, ...]) -> int:
    if not points:
        raise AssertionError("an empty point set has no affine dimension")
    origin = points[0]
    return _matrix_rank([list(_subtract(point, origin)) for point in points[1:]])


def _average(points: tuple[ExactPoint, ...]) -> ExactPoint:
    if not points:
        raise AssertionError("an empty point set has no average")
    count = Fraction(len(points))
    return tuple(
        sum((point[axis] for point in points), Fraction()) / count
        for axis in range(3)
    )  # type: ignore[return-value]


def _nearest_shell(
    points: tuple[ExactPoint, ...], query: ExactPoint
) -> tuple[Fraction, tuple[int, ...]]:
    distances = tuple(_squared_distance(point, query) for point in points)
    minimum = min(distances)
    return minimum, tuple(
        point_id for point_id, distance in enumerate(distances) if distance == minimum
    )


def _expected_box_mask(
    position: ExactPoint,
    lower: ExactPoint,
    upper: ExactPoint,
) -> int:
    mask = 0
    for axis in range(3):
        if position[axis] == lower[axis]:
            mask |= 1 << (2 * axis)
        if position[axis] == upper[axis]:
            mask |= 1 << (2 * axis + 1)
        if not lower[axis] <= position[axis] <= upper[axis]:
            raise AssertionError("a reported vertex lies outside omega")
    return mask


def _word_triplet(value: object, path: str) -> PointWords:
    entries = _require_list(value, path)
    if len(entries) != 3:
        raise AssertionError(f"{path} must contain exactly three words")
    words = []
    for axis, entry in enumerate(entries):
        if not isinstance(entry, str):
            raise AssertionError(f"{path}[{axis}] must be a hexadecimal string")
        _bits_from_word(entry)
        if _canonical_word(entry) != entry:
            raise AssertionError(f"{path}[{axis}] contains noncanonical signed zero")
        words.append(entry)
    return tuple(words)  # type: ignore[return-value]


def _normalize_dump(
    case: FixedCase,
    document: dict[str, object],
) -> dict[str, object]:
    top = _require_keys(
        document,
        {
            "schema",
            "case",
            "canonical_point_bits",
            "omega",
            "claims",
            "cells",
            "global_vertices",
            "contacts",
        },
        "$",
    )
    if top["schema"] != RESULT_SCHEMA:
        raise AssertionError(f"$.schema is not {RESULT_SCHEMA!r}")
    if top["case"] != case.name:
        raise AssertionError(
            f"$.case expected {case.name!r}, observed {top['case']!r}"
        )

    expected_manifest = _canonical_manifest(case.source_points)
    manifest = tuple(
        _word_triplet(item, f"$.canonical_point_bits[{index}]")
        for index, item in enumerate(
            _require_list(top["canonical_point_bits"], "$.canonical_point_bits")
        )
    )
    if manifest != expected_manifest:
        raise AssertionError(
            "$.canonical_point_bits disagrees with independent canonicalization"
        )
    point_count = len(manifest)
    points = tuple(
        tuple(_fraction_from_word(word) for word in point) for point in manifest
    )

    omega_record = _require_keys(top["omega"], {"lower", "upper"}, "$.omega")
    lower_words = _word_triplet(omega_record["lower"], "$.omega.lower")
    upper_words = _word_triplet(omega_record["upper"], "$.omega.upper")
    expected_lower, expected_upper = _expected_omega(manifest)
    if (lower_words, upper_words) != (expected_lower, expected_upper):
        raise AssertionError("$.omega disagrees with adjacent binary64 padding")
    lower = tuple(_fraction_from_word(word) for word in lower_words)
    upper = tuple(_fraction_from_word(word) for word in upper_words)
    for point_id, point in enumerate(points):
        if any(
            not lower[axis] < point[axis] < upper[axis] for axis in range(3)
        ):
            raise AssertionError(f"canonical point {point_id} is not strict in omega")

    claim_names = {
        "all_local_queues_empty_certified",
        "all_cells_full_dimensional_nonempty_certified",
        "global_vertex_occurrence_bijection_certified",
        "natural_incidences_reconciled_certified",
        "artificial_box_boundaries_certified",
    }
    claims = _require_keys(top["claims"], claim_names, "$.claims")
    for name in sorted(claim_names):
        if claims[name] is not True:
            raise AssertionError(f"$.claims.{name} must be exactly true")

    cells: dict[int, dict[ExactPoint, tuple[tuple[int, ...], int]]] = {}
    for cell_index, cell_value in enumerate(_require_list(top["cells"], "$.cells")):
        path = f"$.cells[{cell_index}]"
        cell = _require_keys(cell_value, {"owner_id", "vertices"}, path)
        owner_id = _require_integer(cell["owner_id"], f"{path}.owner_id")
        if owner_id < 0 or owner_id >= point_count:
            raise AssertionError(f"{path}.owner_id lies outside the cloud")
        if owner_id in cells:
            raise AssertionError(f"{path}.owner_id repeats a cell owner")
        vertices: dict[ExactPoint, tuple[tuple[int, ...], int]] = {}
        for vertex_index, vertex_value in enumerate(
            _require_list(cell["vertices"], f"{path}.vertices")
        ):
            vertex_path = f"{path}.vertices[{vertex_index}]"
            vertex = _require_keys(
                vertex_value,
                {"position", "shell_ids", "box_mask"},
                vertex_path,
            )
            position = _position(vertex["position"], f"{vertex_path}.position")
            if position in vertices:
                raise AssertionError(f"{vertex_path}.position repeats a cell vertex")
            shell = _identifier_set(
                vertex["shell_ids"], point_count, f"{vertex_path}.shell_ids"
            )
            if owner_id not in shell:
                raise AssertionError(f"{vertex_path}.shell_ids omits its owner")
            mask = _box_mask(vertex["box_mask"], f"{vertex_path}.box_mask")
            expected_distance, expected_shell = _nearest_shell(points, position)
            if expected_distance < 0 or shell != expected_shell:
                raise AssertionError(f"{vertex_path}.shell_ids is not the exact 1-NN shell")
            if mask != _expected_box_mask(position, lower, upper):
                raise AssertionError(f"{vertex_path}.box_mask is not geometric")
            vertices[position] = (shell, mask)
        if not vertices:
            raise AssertionError(f"{path}.vertices must be nonempty")
        cells[owner_id] = vertices
    if set(cells) != set(range(point_count)):
        raise AssertionError("$.cells does not contain exactly one cell per owner")

    globals_by_position: dict[
        ExactPoint, tuple[Fraction, tuple[int, ...], int, tuple[int, ...]]
    ] = {}
    for vertex_index, vertex_value in enumerate(
        _require_list(top["global_vertices"], "$.global_vertices")
    ):
        path = f"$.global_vertices[{vertex_index}]"
        vertex = _require_keys(
            vertex_value,
            {"position", "distance", "shell_ids", "box_mask", "owner_ids"},
            path,
        )
        position = _position(vertex["position"], f"{path}.position")
        if position in globals_by_position:
            raise AssertionError(f"{path}.position repeats a global vertex")
        distance = _ratio(vertex["distance"], f"{path}.distance")
        if distance < 0:
            raise AssertionError(f"{path}.distance must be nonnegative")
        shell = _identifier_set(vertex["shell_ids"], point_count, f"{path}.shell_ids")
        owners = _identifier_set(vertex["owner_ids"], point_count, f"{path}.owner_ids")
        if not shell or shell != owners:
            raise AssertionError(f"{path} violates the shell-owner bijection")
        mask = _box_mask(vertex["box_mask"], f"{path}.box_mask")
        expected_distance, expected_shell = _nearest_shell(points, position)
        if (distance, shell) != (expected_distance, expected_shell):
            raise AssertionError(f"{path} disagrees with exact exhaustive distances")
        if mask != _expected_box_mask(position, lower, upper):
            raise AssertionError(f"{path}.box_mask is not geometric")
        globals_by_position[position] = (distance, shell, mask, owners)
    if not globals_by_position:
        raise AssertionError("$.global_vertices must be nonempty")

    observed_owners: dict[ExactPoint, set[int]] = {}
    for owner_id, vertices in cells.items():
        for position, (shell, mask) in vertices.items():
            observed_owners.setdefault(position, set()).add(owner_id)
            global_record = globals_by_position.get(position)
            if global_record is None:
                raise AssertionError("a cell vertex is absent from global_vertices")
            _, global_shell, global_mask, _ = global_record
            if (shell, mask) != (global_shell, global_mask):
                raise AssertionError("a local vertex disagrees with its global record")
    if set(observed_owners) != set(globals_by_position):
        raise AssertionError("global_vertices contains a point absent from all cells")
    for position, (_, shell, _, owners) in globals_by_position.items():
        if tuple(sorted(observed_owners[position])) != owners or owners != shell:
            raise AssertionError("global vertex occurrences do not close on its shell")

    contacts: dict[
        tuple[int, ...],
        tuple[
            tuple[int, ...],
            tuple[ExactPoint, ...],
            int,
            int,
            int,
            ExactPoint,
            Fraction,
            str,
        ],
    ] = {}
    allowed_kinds = {
        "noncanonical_quotient_contact",
        "natural_face",
        "natural_edge",
        "natural_vertex",
        "box_supported_contact",
    }
    for contact_index, contact_value in enumerate(
        _require_list(top["contacts"], "$.contacts")
    ):
        path = f"$.contacts[{contact_index}]"
        contact = _require_keys(
            contact_value,
            {
                "query",
                "carrier",
                "vertex_positions",
                "dimension",
                "site_rank",
                "box_mask",
                "witness",
                "witness_distance",
                "kind",
            },
            path,
        )
        query = _identifier_set(contact["query"], point_count, f"{path}.query")
        if len(query) < 2:
            raise AssertionError(f"{path}.query must be non-singleton")
        if query in contacts:
            raise AssertionError(f"{path}.query repeats a contact key")
        carrier = _identifier_set(
            contact["carrier"], point_count, f"{path}.carrier"
        )
        if not set(query).issubset(carrier):
            raise AssertionError(f"{path}.carrier does not contain its query")
        vertex_positions = tuple(
            _position(item, f"{path}.vertex_positions[{index}]")
            for index, item in enumerate(
                _require_list(contact["vertex_positions"], f"{path}.vertex_positions")
            )
        )
        if not vertex_positions or len(set(vertex_positions)) != len(vertex_positions):
            raise AssertionError(f"{path}.vertex_positions must be nonempty and unique")
        vertex_positions = tuple(sorted(vertex_positions))
        if any(position not in globals_by_position for position in vertex_positions):
            raise AssertionError(f"{path}.vertex_positions cites a non-global vertex")
        if any(
            not set(query).issubset(globals_by_position[position][1])
            for position in vertex_positions
        ):
            raise AssertionError(f"{path}.query is absent from a cited shell")

        dimension = _require_integer(contact["dimension"], f"{path}.dimension")
        site_rank = _require_integer(contact["site_rank"], f"{path}.site_rank")
        if not 0 <= dimension <= 2 or not 1 <= site_rank <= 3:
            raise AssertionError(f"{path} has an impossible dimension or site rank")
        mask = _box_mask(contact["box_mask"], f"{path}.box_mask")
        witness = _position(contact["witness"], f"{path}.witness")
        witness_distance = _ratio(
            contact["witness_distance"], f"{path}.witness_distance"
        )
        kind = contact["kind"]
        if not isinstance(kind, str) or kind not in allowed_kinds:
            raise AssertionError(f"{path}.kind is unknown")

        shell_intersection = set(range(point_count))
        common_mask = 0x3F
        for position in vertex_positions:
            _, shell, vertex_mask, _ = globals_by_position[position]
            shell_intersection.intersection_update(shell)
            common_mask &= vertex_mask
        if carrier != tuple(sorted(shell_intersection)):
            raise AssertionError(f"{path}.carrier is not the vertex-shell intersection")
        if mask != common_mask:
            raise AssertionError(f"{path}.box_mask is not common to all vertices")
        if witness != _average(vertex_positions):
            raise AssertionError(f"{path}.witness is not the exact vertex barycenter")
        expected_witness_distance, expected_witness_shell = _nearest_shell(points, witness)
        if (witness_distance, carrier) != (
            expected_witness_distance,
            expected_witness_shell,
        ):
            raise AssertionError(f"{path}.witness does not recover its carrier")
        if dimension != _affine_dimension(vertex_positions):
            raise AssertionError(f"{path}.dimension is not the exact vertex rank")
        carrier_points = tuple(points[point_id] for point_id in carrier)
        if site_rank != _affine_dimension(carrier_points):
            raise AssertionError(f"{path}.site_rank is not the carrier affine rank")

        if query != carrier:
            expected_kind = "noncanonical_quotient_contact"
        elif mask:
            expected_kind = "box_supported_contact"
        else:
            if site_rank + dimension != 3:
                raise AssertionError(f"{path} violates natural rank-dimension duality")
            expected_kind = {
                1: "natural_face",
                2: "natural_edge",
                3: "natural_vertex",
            }[site_rank]
        if kind != expected_kind:
            raise AssertionError(
                f"{path}.kind expected {expected_kind!r}, observed {kind!r}"
            )
        contacts[query] = (
            carrier,
            vertex_positions,
            dimension,
            site_rank,
            mask,
            witness,
            witness_distance,
            kind,
        )

    derived_contact_vertices: dict[tuple[int, ...], set[ExactPoint]] = {}
    for position, (_, shell, _, _) in globals_by_position.items():
        for size in range(2, len(shell) + 1):
            for query in combinations(shell, size):
                derived_contact_vertices.setdefault(query, set()).add(position)
    if set(contacts) != set(derived_contact_vertices):
        missing = sorted(set(derived_contact_vertices) - set(contacts))
        extra = sorted(set(contacts) - set(derived_contact_vertices))
        raise AssertionError(
            f"$.contacts misses {missing!r} and invents {extra!r} query keys"
        )
    for query, positions in derived_contact_vertices.items():
        if contacts[query][1] != tuple(sorted(positions)):
            raise AssertionError(f"contact {query!r} has an incomplete vertex set")

    cell_projection = tuple(
        (
            owner_id,
            tuple(
                sorted(
                    (position, shell, mask)
                    for position, (shell, mask) in vertices.items()
                )
            ),
        )
        for owner_id, vertices in sorted(cells.items())
    )
    global_projection = tuple(
        (position, distance, shell, mask, owners)
        for position, (distance, shell, mask, owners) in sorted(
            globals_by_position.items()
        )
    )
    contact_projection = tuple(
        (query, *record) for query, record in sorted(contacts.items())
    )
    return {
        "canonical_point_bits": manifest,
        "omega": (lower_words, upper_words),
        "cells": cell_projection,
        "global_vertices": global_projection,
        "contacts": contact_projection,
    }


def _first_difference(expected: object, actual: object, path: str = "$") -> str | None:
    if type(expected) is not type(actual):
        return (
            f"{path}: expected type {type(expected).__name__}, "
            f"observed {type(actual).__name__}"
        )
    if isinstance(expected, dict):
        expected_keys = set(expected)
        actual_keys = set(actual)  # type: ignore[arg-type]
        if expected_keys != actual_keys:
            return (
                f"{path}: missing keys {sorted(expected_keys - actual_keys)!r}, "
                f"extra keys {sorted(actual_keys - expected_keys)!r}"
            )
        for key in sorted(expected):
            difference = _first_difference(
                expected[key], actual[key], f"{path}.{key}"  # type: ignore[index]
            )
            if difference is not None:
                return difference
        return None
    if isinstance(expected, tuple):
        if len(expected) != len(actual):  # type: ignore[arg-type]
            return (
                f"{path}: expected length {len(expected)}, "
                f"observed {len(actual)}"  # type: ignore[arg-type]
            )
        for index, (expected_item, actual_item) in enumerate(
            zip(expected, actual)  # type: ignore[arg-type]
        ):
            difference = _first_difference(
                expected_item, actual_item, f"{path}[{index}]"
            )
            if difference is not None:
                return difference
        return None
    if expected != actual:
        return f"{path}: expected {expected!r}, observed {actual!r}"
    return None


def _oracle_exact_point(value: object, path: str) -> ExactPoint:
    if not isinstance(value, tuple) or len(value) != 3:
        raise AssertionError(f"{path} must be one exact point tuple")
    if any(not isinstance(coordinate, Fraction) for coordinate in value):
        raise AssertionError(f"{path} coordinates must be Fraction values")
    return value  # type: ignore[return-value]


def _oracle_identifiers(value: object, point_count: int, path: str) -> tuple[int, ...]:
    if not isinstance(value, tuple):
        raise AssertionError(f"{path} must be an identifier tuple")
    identifiers = tuple(
        _require_integer(identifier, f"{path}[{index}]")
        for index, identifier in enumerate(value)
    )
    if any(identifier < 0 or identifier >= point_count for identifier in identifiers):
        raise AssertionError(f"{path} contains an identifier outside the cloud")
    if len(set(identifiers)) != len(identifiers):
        raise AssertionError(f"{path} contains duplicate identifiers")
    return tuple(sorted(identifiers))


def _normalize_oracle_projection(
    case: FixedCase,
    projection: dict[str, object],
) -> dict[str, object]:
    expected_keys = {
        "canonical_point_bits",
        "omega",
        "cells",
        "global_vertices",
        "contacts",
    }
    if set(projection) != expected_keys:
        raise AssertionError("the independent oracle projection has an unknown shape")
    manifest = projection["canonical_point_bits"]
    if not isinstance(manifest, tuple):
        raise AssertionError("oracle canonical_point_bits must be a tuple")
    normalized_manifest = tuple(
        tuple(point) if isinstance(point, tuple) else () for point in manifest
    )
    if any(len(point) != 3 for point in normalized_manifest):
        raise AssertionError("oracle canonical points must contain three words")
    for point in normalized_manifest:
        for word in point:
            if not isinstance(word, str):
                raise AssertionError("oracle point words must be strings")
            _bits_from_word(word)
            if _canonical_word(word) != word:
                raise AssertionError("oracle point words must spell zero canonically")
    expected_manifest = _canonical_manifest(case.source_points)
    if normalized_manifest != expected_manifest:
        raise AssertionError("the independent oracle changed the canonical manifest")
    point_count = len(normalized_manifest)

    omega = projection["omega"]
    if not isinstance(omega, tuple) or len(omega) != 2:
        raise AssertionError("oracle omega must be a lower/upper tuple")
    normalized_omega = tuple(
        tuple(part) if isinstance(part, tuple) else () for part in omega
    )
    if any(len(part) != 3 for part in normalized_omega):
        raise AssertionError("oracle omega bounds must contain three words")
    if normalized_omega != _expected_omega(expected_manifest):
        raise AssertionError("the independent oracle changed omega")

    raw_cells = projection["cells"]
    if not isinstance(raw_cells, tuple):
        raise AssertionError("oracle cells must be a tuple")
    cells: dict[int, tuple[tuple[ExactPoint, tuple[int, ...], int], ...]] = {}
    for cell_index, raw_cell in enumerate(raw_cells):
        path = f"oracle.cells[{cell_index}]"
        if not isinstance(raw_cell, tuple) or len(raw_cell) != 2:
            raise AssertionError(f"{path} must contain owner and vertices")
        owner = _require_integer(raw_cell[0], f"{path}.owner")
        if owner < 0 or owner >= point_count or owner in cells:
            raise AssertionError(f"{path}.owner is invalid or duplicated")
        raw_vertices = raw_cell[1]
        if not isinstance(raw_vertices, tuple) or not raw_vertices:
            raise AssertionError(f"{path}.vertices must be a nonempty tuple")
        vertices = []
        seen_positions: set[ExactPoint] = set()
        for vertex_index, raw_vertex in enumerate(raw_vertices):
            vertex_path = f"{path}.vertices[{vertex_index}]"
            if not isinstance(raw_vertex, tuple) or len(raw_vertex) != 3:
                raise AssertionError(f"{vertex_path} has an invalid shape")
            position = _oracle_exact_point(raw_vertex[0], f"{vertex_path}.position")
            if position in seen_positions:
                raise AssertionError(f"{vertex_path}.position is duplicated")
            seen_positions.add(position)
            shell = _oracle_identifiers(
                raw_vertex[1], point_count, f"{vertex_path}.shell"
            )
            mask = _box_mask(raw_vertex[2], f"{vertex_path}.mask")
            vertices.append((position, shell, mask))
        cells[owner] = tuple(sorted(vertices))
    if set(cells) != set(range(point_count)):
        raise AssertionError("oracle cells do not contain every owner exactly once")

    raw_globals = projection["global_vertices"]
    if not isinstance(raw_globals, tuple) or not raw_globals:
        raise AssertionError("oracle global_vertices must be a nonempty tuple")
    globals_by_position: dict[
        ExactPoint, tuple[ExactPoint, Fraction, tuple[int, ...], int, tuple[int, ...]]
    ] = {}
    for vertex_index, raw_vertex in enumerate(raw_globals):
        path = f"oracle.global_vertices[{vertex_index}]"
        if not isinstance(raw_vertex, tuple) or len(raw_vertex) != 5:
            raise AssertionError(f"{path} has an invalid shape")
        position = _oracle_exact_point(raw_vertex[0], f"{path}.position")
        if position in globals_by_position:
            raise AssertionError(f"{path}.position is duplicated")
        distance = raw_vertex[1]
        if not isinstance(distance, Fraction) or distance < 0:
            raise AssertionError(f"{path}.distance must be a nonnegative Fraction")
        shell = _oracle_identifiers(raw_vertex[2], point_count, f"{path}.shell")
        mask = _box_mask(raw_vertex[3], f"{path}.mask")
        owners = _oracle_identifiers(raw_vertex[4], point_count, f"{path}.owners")
        globals_by_position[position] = (position, distance, shell, mask, owners)

    raw_contacts = projection["contacts"]
    if not isinstance(raw_contacts, tuple):
        raise AssertionError("oracle contacts must be a tuple")
    contacts: dict[tuple[int, ...], tuple[object, ...]] = {}
    for contact_index, raw_contact in enumerate(raw_contacts):
        path = f"oracle.contacts[{contact_index}]"
        if not isinstance(raw_contact, tuple) or len(raw_contact) != 9:
            raise AssertionError(f"{path} has an invalid shape")
        query = _oracle_identifiers(raw_contact[0], point_count, f"{path}.query")
        if len(query) < 2 or query in contacts:
            raise AssertionError(f"{path}.query is singleton or duplicated")
        carrier = _oracle_identifiers(raw_contact[1], point_count, f"{path}.carrier")
        raw_positions = raw_contact[2]
        if not isinstance(raw_positions, tuple) or not raw_positions:
            raise AssertionError(f"{path}.positions must be a nonempty tuple")
        positions = tuple(
            _oracle_exact_point(position, f"{path}.positions[{index}]")
            for index, position in enumerate(raw_positions)
        )
        if len(set(positions)) != len(positions):
            raise AssertionError(f"{path}.positions contains duplicates")
        dimension = _require_integer(raw_contact[3], f"{path}.dimension")
        site_rank = _require_integer(raw_contact[4], f"{path}.site_rank")
        mask = _box_mask(raw_contact[5], f"{path}.mask")
        witness = _oracle_exact_point(raw_contact[6], f"{path}.witness")
        witness_distance = raw_contact[7]
        kind = raw_contact[8]
        if not isinstance(witness_distance, Fraction) or not isinstance(kind, str):
            raise AssertionError(f"{path} has an invalid witness distance or kind")
        contacts[query] = (
            query,
            carrier,
            tuple(sorted(positions)),
            dimension,
            site_rank,
            mask,
            witness,
            witness_distance,
            kind,
        )

    return {
        "canonical_point_bits": normalized_manifest,
        "omega": normalized_omega,
        "cells": tuple((owner, cells[owner]) for owner in sorted(cells)),
        "global_vertices": tuple(
            globals_by_position[position] for position in sorted(globals_by_position)
        ),
        "contacts": tuple(contacts[query] for query in sorted(contacts)),
    }


def _expected_projection(
    case: FixedCase,
    cache: dict[
        tuple[tuple[PointWords, ...], tuple[PointWords, PointWords]],
        dict[str, object],
    ],
) -> dict[str, object]:
    manifest = _canonical_manifest(case.source_points)
    omega = _expected_omega(manifest)
    key = (manifest, omega)
    if key not in cache:
        result = build_bounded_ordinary_subset_oracle(manifest, omega)
        if result.decision is not OrdinaryDiagramOracleDecision.COMPLETE:
            raise AssertionError("the independent oracle unexpectedly lacked budget")
        cache[key] = _normalize_oracle_projection(case, result.semantic_projection())
    return cache[key]


def _parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "executable",
        type=Path,
        help="path to morsehgp3d_ordinary_diagram_closure_dump",
    )
    parser.add_argument(
        "--timeout-seconds",
        type=int,
        default=DEFAULT_TIMEOUT_SECONDS,
        help="single-subprocess timeout (default: %(default)s)",
    )
    return parser.parse_args()


def main() -> int:
    arguments = _parse_arguments()
    if arguments.timeout_seconds <= 0:
        raise ValueError("--timeout-seconds must be positive")
    executable = arguments.executable.resolve(strict=True)
    if not executable.is_file():
        raise ValueError(f"dump executable is not a regular file: {executable}")

    cases = _fixed_cases()
    protocol_input = "\n".join(
        (INPUT_HEADER, *(_case_protocol_line(case) for case in cases), "")
    )
    completed = subprocess.run(
        [str(executable)],
        input=protocol_input,
        check=False,
        capture_output=True,
        text=True,
        timeout=arguments.timeout_seconds,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            "ordinary-diagram dump exited with "
            f"{completed.returncode}: {completed.stderr.strip()}"
        )

    lines = completed.stdout.splitlines()
    if not lines or lines[0] != OUTPUT_HEADER:
        observed_header = lines[0] if lines else None
        raise AssertionError(
            f"expected output header {OUTPUT_HEADER!r}, observed {observed_header!r}"
        )
    result_lines = lines[1:]
    if len(result_lines) != len(cases) or any(not line for line in result_lines):
        raise AssertionError(
            f"expected {len(cases)} nonempty result lines, "
            f"observed {len(result_lines)}"
        )

    oracle_cache: dict[
        tuple[tuple[PointWords, ...], tuple[PointWords, PointWords]],
        dict[str, object],
    ] = {}
    observed_by_name: dict[str, dict[str, object]] = {}
    for case, line in zip(cases, result_lines):
        observed = _normalize_dump(case, _strict_json_object(line))
        expected = _expected_projection(case, oracle_cache)
        difference = _first_difference(expected, observed)
        if difference is not None:
            raise AssertionError(f"{case.name}: {difference}")
        observed_by_name[case.name] = observed

    for case in cases:
        if case.permutation_of is None:
            continue
        difference = _first_difference(
            observed_by_name[case.permutation_of],
            observed_by_name[case.name],
        )
        if difference is not None:
            raise AssertionError(
                f"{case.name}: canonical permutation differs from "
                f"{case.permutation_of}: {difference}"
            )

    print(
        "ordinary-diagram closure differential passed for "
        f"{len(cases)} exact bounded cases"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (
        AssertionError,
        OSError,
        RuntimeError,
        ValueError,
        subprocess.TimeoutExpired,
    ) as error:
        print(f"ordinary-diagram closure differential failed: {error}", file=sys.stderr)
        raise SystemExit(1) from error
