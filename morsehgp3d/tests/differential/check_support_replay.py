#!/usr/bin/env python3
"""Audit v5 support analysis and sphere-side replay with Fraction oracles."""

from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import math
import struct
import subprocess
import sys
import tempfile
from fractions import Fraction
from pathlib import Path
from typing import Any, Callable


INPUT_KIND = "morsehgp3d_predicate_replay_input"
RESULT_KIND = "morsehgp3d_predicate_replay_result"
SUPPORT_PREDICATE = "circumcenter_support_analysis"
SPHERE_PREDICATE = "sphere_side"
V5_DOMAIN = b"MorseHGP3D/predicate-replay-v5/"
HISTORICAL_DOMAINS = {
    1: b"MorseHGP3D/predicate-replay-v1/",
    2: b"MorseHGP3D/predicate-replay-v2/",
    3: b"MorseHGP3D/predicate-replay-v3/",
    4: b"MorseHGP3D/predicate-replay-v4/",
}
HISTORICAL_SENTINELS = {
    "distance_one_ulp.json": (
        1,
        "db04d3582bb0a813cd60cf48ca5f0ce630b1f57ed209e63f363f8088a0bdd56c",
    ),
    "power_bisector_rational_zero.json": (
        2,
        "b90d0731fa0f77f867c44532b08e70cf28ac02417ed5b8a9cc01e581be504da3",
    ),
    "affine_plane_translated.json": (
        3,
        "f676bacc74860b38647592c2ea525444b7f588824c8d06eae141602d77524762",
    ),
    "center_triangle_nondyadic.json": (
        4,
        "e25d8274fb4331a630afd8995b38fa427c0b2bb72aa644b1d1943c1e974c73ec",
    ),
}
EXPECTED_SUPPORT_FIXTURES = frozenset(
    {
        "support_pair_distinct.json",
        "support_pair_duplicate.json",
        "support_singleton.json",
        "support_tetrahedron_boundary.json",
        "support_tetrahedron_coplanar.json",
        "support_tetrahedron_exterior.json",
        "support_tetrahedron_well_centered.json",
        "support_triangle_acute.json",
        "support_triangle_barycentric_negative_one_ulp.json",
        "support_triangle_barycentric_positive_one_ulp.json",
        "support_triangle_obtuse.json",
        "support_triangle_right.json",
    }
)
EXPECTED_SPHERE_FIXTURES = frozenset(
    {
        "sphere_boundary.json",
        "sphere_inside.json",
        "sphere_outside.json",
        "sphere_quasi_inside_one_ulp.json",
        "sphere_quasi_outside_one_ulp.json",
        "sphere_rational_boundary.json",
        "sphere_zero_level_boundary.json",
    }
)
SUPPORT_RESULT_FIELDS = {
    "affine_dimension",
    "barycentric_coordinates_exact",
    "barycentric_signs",
    "center_exact",
    "certification_stage",
    "convex_hull_location",
    "counters",
    "predicate",
    "reduced_support_indices",
    "squared_level_exact",
    "support_kind",
    "support_size",
    "support_status",
}
SPHERE_RESULT_FIELDS = {
    "certification_stage",
    "classification",
    "counters",
    "predicate",
    "sign",
    "signed_offset_exact",
    "squared_distance_exact",
}
COUNTER_FIELDS = {
    "cpu_multiprecision_certified",
    "exact_zeros",
    "expansion_certified",
    "fp32_proposals",
    "fp64_filtered_certified",
    "remaining_unknown",
}
WRAPPER_TIMEOUT_SECONDS = 40

Point = tuple[Fraction, Fraction, Fraction]


def canonical_json(value: object) -> str:
    return json.dumps(
        value, allow_nan=False, ensure_ascii=False, separators=(",", ":"), sort_keys=True
    )


def load_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise AssertionError(f"{path.name} is not a JSON object")
    return value


def point_from_words(words: list[str]) -> Point:
    if len(words) != 3:
        raise AssertionError("a replay point must contain three words")
    return tuple(
        Fraction.from_float(struct.unpack(">d", bytes.fromhex(word))[0])
        for word in words
    )  # type: ignore[return-value]


def word_from_fraction(value: Fraction) -> str:
    encoded = float(value)
    if not math.isfinite(encoded) or Fraction.from_float(encoded) != value:
        raise AssertionError("a metamorphism left exact binary64 input space")
    return struct.pack(">d", encoded).hex()


def words_from_point(point: Point) -> list[str]:
    return [word_from_fraction(coordinate) for coordinate in point]


def rational_record(value: Fraction) -> dict[str, str]:
    return {"denominator": str(value.denominator), "numerator": str(value.numerator)}


def level_record(value: Fraction) -> dict[str, str]:
    if value < 0:
        raise AssertionError("an exact level cannot be negative")
    return {
        "denominator": str(value.denominator),
        "numerator": str(value.numerator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }


def rational3_record(point: Point) -> dict[str, str]:
    denominator = math.lcm(*(coordinate.denominator for coordinate in point))
    numerators = [
        coordinate.numerator * (denominator // coordinate.denominator)
        for coordinate in point
    ]
    divisor = denominator
    for numerator in numerators:
        divisor = math.gcd(divisor, abs(numerator))
    denominator //= divisor
    numerators = [numerator // divisor for numerator in numerators]
    return {
        "denominator": str(denominator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit",
        "x_numerator": str(numerators[0]),
        "y_numerator": str(numerators[1]),
        "z_numerator": str(numerators[2]),
    }


def rational3_from_record(value: dict[str, Any]) -> Point:
    denominator = int(value["denominator"])
    return tuple(
        Fraction(int(value[f"{axis}_numerator"]), denominator) for axis in "xyz"
    )  # type: ignore[return-value]


def level_from_record(value: dict[str, Any]) -> Fraction:
    return Fraction(int(value["numerator"]), int(value["denominator"]))


def add(left: Point, right: Point) -> Point:
    return tuple(a + b for a, b in zip(left, right))  # type: ignore[return-value]


def multiply(point: Point, factor: Fraction) -> Point:
    return tuple(coordinate * factor for coordinate in point)  # type: ignore[return-value]


def dot(left: Point, right: Point) -> Fraction:
    return sum((a * b for a, b in zip(left, right)), Fraction())


def squared_distance(left: Point, right: Point) -> Fraction:
    return sum(((a - b) ** 2 for a, b in zip(left, right)), Fraction())


def rref(matrix: list[list[Fraction]]) -> tuple[list[list[Fraction]], list[int]]:
    reduced = [list(row) for row in matrix]
    pivots: list[int] = []
    pivot_row = 0
    for column in range(len(reduced[0]) if reduced else 0):
        selected = next(
            (row for row in range(pivot_row, len(reduced)) if reduced[row][column]),
            None,
        )
        if selected is None:
            continue
        reduced[pivot_row], reduced[selected] = reduced[selected], reduced[pivot_row]
        pivot = reduced[pivot_row][column]
        reduced[pivot_row] = [entry / pivot for entry in reduced[pivot_row]]
        for row in range(len(reduced)):
            if row == pivot_row or not reduced[row][column]:
                continue
            factor = reduced[row][column]
            reduced[row] = [
                entry - factor * pivot_entry
                for entry, pivot_entry in zip(reduced[row], reduced[pivot_row])
            ]
        pivots.append(column)
        pivot_row += 1
        if pivot_row == len(reduced):
            break
    return reduced, pivots


def sign(value: Fraction) -> str:
    if value < 0:
        return "negative"
    return "zero" if value == 0 else "positive"


def empty_counters() -> dict[str, int]:
    return {
        "cpu_multiprecision_certified": 0,
        "exact_zeros": 0,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 0,
        "remaining_unknown": 0,
    }


def independent_witness(points: list[Point]) -> tuple[Point, Fraction, list[Fraction]]:
    """Independent bordered solve; production uses dimension-specific formulas."""

    size = len(points)
    gram = [[dot(left, right) for right in points] for left in points]
    system = [
        [2 * entry for entry in row] + [Fraction(-1), gram[index][index]]
        for index, row in enumerate(gram)
    ]
    system.append([Fraction(1) for _ in points] + [Fraction(), Fraction(1)])
    reduced, pivots = rref(system)
    if pivots != list(range(size + 1)):
        raise AssertionError("an independent support has a singular bordered system")
    barycentric = [reduced[index][-1] for index in range(size)]
    if sum(barycentric, Fraction()) != 1:
        raise AssertionError("bordered solve produced non-affine coordinates")
    center = tuple(
        sum(
            (barycentric[index] * points[index][axis] for index in range(size)),
            Fraction(),
        )
        for axis in range(3)
    )
    squared_level = squared_distance(center, points[0])
    if any(squared_distance(center, point) != squared_level for point in points):
        raise AssertionError("bordered solve produced unequal support distances")
    return center, squared_level, barycentric  # type: ignore[return-value]


def support_oracle(value: dict[str, Any]) -> dict[str, Any]:
    points = [point_from_words(words) for words in value["points"]]
    directions = [
        [point[axis] - points[0][axis] for axis in range(3)]
        for point in points[1:]
    ]
    affine_dimension = len(rref(directions)[1])
    if affine_dimension + 1 != len(points):
        return {
            "affine_dimension": affine_dimension,
            "barycentric_coordinates_exact": None,
            "barycentric_signs": None,
            "center_exact": None,
            "certification_stage": None,
            "convex_hull_location": None,
            "counters": empty_counters(),
            "predicate": SUPPORT_PREDICATE,
            "reduced_support_indices": None,
            "squared_level_exact": None,
            "support_kind": "affinely_dependent",
            "support_size": len(points),
            "support_status": "affinely_dependent",
        }

    center, squared_level, barycentric = independent_witness(points)
    signs = [sign(coordinate) for coordinate in barycentric]
    if "negative" in signs:
        location = "exterior"
        status = "exterior_circumcenter"
        reduced = None
    elif "zero" in signs:
        location = "relative_boundary"
        status = "boundary_reduced"
        reduced = [index for index, value_sign in enumerate(signs) if value_sign == "positive"]
        reduced_center, reduced_level, reduced_barycentric = independent_witness(
            [points[index] for index in reduced]
        )
        if (
            reduced_center != center
            or reduced_level != squared_level
            or any(coordinate <= 0 for coordinate in reduced_barycentric)
        ):
            raise AssertionError("positive boundary reduction changed the sphere")
    else:
        location = "relative_interior"
        status = "minimal"
        reduced = list(range(len(points)))
    counters = empty_counters()
    counters["cpu_multiprecision_certified"] = len(points)
    counters["exact_zeros"] = signs.count("zero")
    return {
        "affine_dimension": affine_dimension,
        "barycentric_coordinates_exact": [
            rational_record(coordinate) for coordinate in barycentric
        ],
        "barycentric_signs": signs,
        "center_exact": rational3_record(center),
        "certification_stage": "cpu_multiprecision",
        "convex_hull_location": location,
        "counters": counters,
        "predicate": SUPPORT_PREDICATE,
        "reduced_support_indices": reduced,
        "squared_level_exact": level_record(squared_level),
        "support_kind": "affinely_independent",
        "support_size": len(points),
        "support_status": status,
    }


def sphere_oracle(value: dict[str, Any]) -> dict[str, Any]:
    center = rational3_from_record(value["center_exact"])
    point = point_from_words(value["point"])
    level = level_from_record(value["squared_level_exact"])
    distance = squared_distance(center, point)
    offset = distance - level
    offset_sign = sign(offset)
    counters = empty_counters()
    counters["cpu_multiprecision_certified"] = 1
    counters["exact_zeros"] = 1 if offset_sign == "zero" else 0
    return {
        "certification_stage": "cpu_multiprecision",
        "classification": {
            "negative": "strictly_inside",
            "zero": "boundary",
            "positive": "outside",
        }[offset_sign],
        "counters": counters,
        "predicate": SPHERE_PREDICATE,
        "sign": offset_sign,
        "signed_offset_exact": rational_record(offset),
        "squared_distance_exact": level_record(distance),
    }


def oracle(value: dict[str, Any]) -> dict[str, Any]:
    return support_oracle(value) if value["predicate"] == SUPPORT_PREDICATE else sphere_oracle(value)


def run_wrapper(
    wrapper: Path, executable: Path, fixture: Path
) -> tuple[str, dict[str, Any]]:
    completed = subprocess.run(
        [sys.executable, str(wrapper), str(fixture), "--executable", str(executable)],
        capture_output=True,
        encoding="utf-8",
        timeout=WRAPPER_TIMEOUT_SECONDS,
    )
    if completed.returncode != 0:
        raise AssertionError(
            f"{fixture.name} failed unexpectedly: rc={completed.returncode}, "
            f"stdout={completed.stdout!r}, stderr={completed.stderr!r}"
        )
    if completed.stderr:
        raise AssertionError(f"{fixture.name} wrote unexpected wrapper diagnostics")
    replay = json.loads(completed.stdout)
    if completed.stdout != canonical_json(replay) + "\n":
        raise AssertionError(f"{fixture.name} did not produce canonical replay JSON")
    return completed.stdout, replay


def audit_envelope(replay: dict[str, Any], fixture: dict[str, Any]) -> None:
    if set(replay) != {"input", "kind", "replay_id", "result", "schema_version"}:
        raise AssertionError("the v5 replay envelope schema is open")
    if (
        replay["kind"] != RESULT_KIND
        or replay["schema_version"] != 5
        or replay["input"] != fixture
        or replay["input"].get("kind") != INPUT_KIND
        or replay["input"].get("schema_version") != 5
    ):
        raise AssertionError("the wrapper changed or mis-versioned its v5 input")
    serialized = canonical_json(replay["input"]).encode("utf-8")
    expected_id = hashlib.sha256(V5_DOMAIN + serialized).hexdigest()
    if replay["replay_id"] != expected_id:
        raise AssertionError("the v5 replay identifier uses the wrong domain or bytes")
    if any(
        replay["replay_id"] == hashlib.sha256(domain + serialized).hexdigest()
        for domain in HISTORICAL_DOMAINS.values()
    ):
        raise AssertionError("the v5 replay identifier aliases a historical domain")


def audit_scientific_result(replay: dict[str, Any]) -> dict[str, Any]:
    result = replay["result"]
    predicate = replay["input"]["predicate"]
    fields = SUPPORT_RESULT_FIELDS if predicate == SUPPORT_PREDICATE else SPHERE_RESULT_FIELDS
    if not isinstance(result, dict) or set(result) != fields:
        raise AssertionError(f"the {predicate} result schema is open")
    expected = oracle(replay["input"])
    if result != expected:
        raise AssertionError(
            f"{predicate} differs from the independent Fraction oracle: "
            f"expected={canonical_json(expected)}, observed={canonical_json(result)}"
        )
    counters = result["counters"]
    if set(counters) != COUNTER_FIELDS or any(
        type(count) is not int or count < 0 for count in counters.values()
    ):
        raise AssertionError(f"{predicate} counters are not PredicateCounts v2")
    return expected


def write_case(
    directory: Path,
    name: str,
    value: dict[str, Any] | None = None,
    *,
    raw: str | None = None,
) -> Path:
    path = directory / name
    if raw is None:
        if value is None:
            raise AssertionError("a generated case has no payload")
        raw = canonical_json(value)
    path.write_text(raw + ("" if raw.endswith("\n") else "\n"), encoding="utf-8")
    return path


def run_generated(
    wrapper: Path,
    executable: Path,
    directory: Path,
    name: str,
    value: dict[str, Any],
) -> tuple[dict[str, Any], dict[str, Any]]:
    _, replay = run_wrapper(wrapper, executable, write_case(directory, name, value))
    audit_envelope(replay, value)
    expected = audit_scientific_result(replay)
    return replay, expected


def make_multiprecision_forwarder(path: Path, executable: Path) -> None:
    path.write_text(
        "#!/usr/bin/env python3\n"
        "import os\n"
        "import sys\n"
        f"native = {str(executable)!r}\n"
        "os.execv(native, [native, '--multiprecision-only', *sys.argv[1:]])\n",
        encoding="utf-8",
    )
    path.chmod(0o755)


def check_historical_sentinels(
    wrapper: Path, executable: Path, fixture_directory: Path
) -> None:
    for filename, (version, expected_id) in HISTORICAL_SENTINELS.items():
        path = fixture_directory / filename
        if not path.is_file():
            raise AssertionError(f"historical sentinel fixture is missing: {filename}")
        _, replay = run_wrapper(wrapper, executable, path)
        if replay.get("schema_version") != version or replay.get("replay_id") != expected_id:
            raise AssertionError(f"historical replay identity changed for {filename}")
        serialized = canonical_json(replay["input"]).encode("utf-8")
        if hashlib.sha256(HISTORICAL_DOMAINS[version] + serialized).hexdigest() != expected_id:
            raise AssertionError(f"historical replay domain check failed for {filename}")


def make_support_input(points: list[list[str]]) -> dict[str, Any]:
    return {
        "kind": INPUT_KIND,
        "points": points,
        "predicate": SUPPORT_PREDICATE,
        "schema_version": 5,
    }


def make_sphere_input(center: Point, level: Fraction, point: Point) -> dict[str, Any]:
    return {
        "center_exact": rational3_record(center),
        "kind": INPUT_KIND,
        "point": words_from_point(point),
        "predicate": SPHERE_PREDICATE,
        "schema_version": 5,
        "squared_level_exact": level_record(level),
    }


def check_complete_permutations(
    wrapper: Path, executable: Path, fixtures: dict[str, dict[str, Any]]
) -> int:
    count = 0
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-support-permutations-") as temporary:
        directory = Path(temporary)
        for filename, base in fixtures.items():
            base_expected = support_oracle(base)
            for permutation in itertools.permutations(range(len(base["points"]))):
                transformed = make_support_input(
                    [list(base["points"][index]) for index in permutation]
                )
                _, observed = run_generated(
                    wrapper,
                    executable,
                    directory,
                    f"{Path(filename).stem}-{count}.json",
                    transformed,
                )
                invariant_fields = {
                    "affine_dimension",
                    "center_exact",
                    "convex_hull_location",
                    "counters",
                    "squared_level_exact",
                    "support_kind",
                    "support_size",
                    "support_status",
                }
                if any(observed[field] != base_expected[field] for field in invariant_fields):
                    raise AssertionError(f"a support permutation changed {filename}")
                if base_expected["barycentric_coordinates_exact"] is not None:
                    expected_barycentric = [
                        base_expected["barycentric_coordinates_exact"][index]
                        for index in permutation
                    ]
                    expected_signs = [
                        base_expected["barycentric_signs"][index] for index in permutation
                    ]
                    if (
                        observed["barycentric_coordinates_exact"] != expected_barycentric
                        or observed["barycentric_signs"] != expected_signs
                    ):
                        raise AssertionError(f"barycentric order changed incorrectly for {filename}")
                base_reduced = base_expected["reduced_support_indices"]
                observed_reduced = observed["reduced_support_indices"]
                if base_reduced is None:
                    if observed_reduced is not None:
                        raise AssertionError(f"a permutation invented a reduction for {filename}")
                elif sorted(permutation[index] for index in observed_reduced) != base_reduced:
                    raise AssertionError(f"a permutation changed the reduced support for {filename}")
                count += 1
    return count


def signed_axes(point: Point) -> Point:
    return point[2], -point[0], point[1]


def check_metamorphisms(
    wrapper: Path,
    executable: Path,
    support_fixtures: dict[str, dict[str, Any]],
    sphere_fixtures: dict[str, dict[str, Any]],
) -> int:
    translation: Point = (Fraction(), Fraction(2), Fraction(-3))
    scale = Fraction(2)
    transforms: list[
        tuple[str, Callable[[Point], Point], Callable[[Point], Point], Fraction]
    ] = [
        (
            "translation",
            lambda point: add(point, translation),
            lambda center: add(center, translation),
            Fraction(1),
        ),
        ("signed_axes", signed_axes, signed_axes, Fraction(1)),
        (
            "power_of_two_scale",
            lambda point: multiply(point, scale),
            lambda center: multiply(center, scale),
            scale * scale,
        ),
    ]
    count = 0
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-support-metamorphisms-") as temporary:
        directory = Path(temporary)
        for filename, base in support_fixtures.items():
            base_points = [point_from_words(words) for words in base["points"]]
            base_expected = support_oracle(base)
            for transform_name, point_map, center_map, level_factor in transforms:
                transformed = make_support_input(
                    [words_from_point(point_map(point)) for point in base_points]
                )
                _, observed = run_generated(
                    wrapper,
                    executable,
                    directory,
                    f"{Path(filename).stem}-{transform_name}.json",
                    transformed,
                )
                unchanged = {
                    "affine_dimension",
                    "barycentric_coordinates_exact",
                    "barycentric_signs",
                    "convex_hull_location",
                    "counters",
                    "reduced_support_indices",
                    "support_kind",
                    "support_size",
                    "support_status",
                }
                if any(observed[field] != base_expected[field] for field in unchanged):
                    raise AssertionError(f"{transform_name} changed support facts for {filename}")
                if base_expected["center_exact"] is None:
                    if observed["center_exact"] is not None:
                        raise AssertionError(f"{transform_name} invented a center for {filename}")
                else:
                    base_center = rational3_from_record(base_expected["center_exact"])
                    base_level = level_from_record(base_expected["squared_level_exact"])
                    if (
                        observed["center_exact"] != rational3_record(center_map(base_center))
                        or observed["squared_level_exact"]
                        != level_record(base_level * level_factor)
                    ):
                        raise AssertionError(f"{transform_name} broke sphere covariance for {filename}")
                count += 1

        for filename, base in sphere_fixtures.items():
            center = rational3_from_record(base["center_exact"])
            level = level_from_record(base["squared_level_exact"])
            point = point_from_words(base["point"])
            base_expected = sphere_oracle(base)
            base_distance = level_from_record(base_expected["squared_distance_exact"])
            base_offset = Fraction(
                int(base_expected["signed_offset_exact"]["numerator"]),
                int(base_expected["signed_offset_exact"]["denominator"]),
            )
            for transform_name, point_map, center_map, level_factor in transforms:
                transformed = make_sphere_input(
                    center_map(center), level * level_factor, point_map(point)
                )
                _, observed = run_generated(
                    wrapper,
                    executable,
                    directory,
                    f"{Path(filename).stem}-{transform_name}.json",
                    transformed,
                )
                if (
                    observed["classification"] != base_expected["classification"]
                    or observed["sign"] != base_expected["sign"]
                    or observed["counters"] != base_expected["counters"]
                    or observed["squared_distance_exact"]
                    != level_record(base_distance * level_factor)
                    or observed["signed_offset_exact"]
                    != rational_record(base_offset * level_factor)
                ):
                    raise AssertionError(f"{transform_name} broke sphere-side covariance for {filename}")
                count += 1
    return count


def expect_closed_failure(wrapper: Path, executable: Path, fixture: Path) -> None:
    completed = subprocess.run(
        [sys.executable, str(wrapper), str(fixture), "--executable", str(executable)],
        capture_output=True,
        encoding="utf-8",
        timeout=WRAPPER_TIMEOUT_SECONDS,
    )
    if (
        completed.returncode != 2
        or completed.stdout
        or "predicate replay failed closed:" not in completed.stderr
    ):
        raise AssertionError(
            f"{fixture.name} did not fail closed: rc={completed.returncode}, "
            f"stdout={completed.stdout!r}, stderr={completed.stderr!r}"
        )


def check_invalid_inputs(
    wrapper: Path,
    executable: Path,
    support_base: dict[str, Any],
    sphere_base: dict[str, Any],
) -> int:
    variants: list[tuple[str, dict[str, Any] | None, str | None]] = []

    def clone(value: dict[str, Any]) -> dict[str, Any]:
        return json.loads(json.dumps(value))

    value = clone(support_base)
    value["unexpected"] = True
    variants.append(("support-extra-field.json", value, None))
    value = clone(support_base)
    value["schema_version"] = 4
    variants.append(("support-under-v4.json", value, None))
    value = clone(support_base)
    value["predicate"] = SPHERE_PREDICATE
    variants.append(("support-wrong-predicate.json", value, None))
    value = clone(support_base)
    value["points"] = []
    variants.append(("support-empty.json", value, None))
    value = clone(support_base)
    value["points"] = [*value["points"]] * 5
    variants.append(("support-five-points.json", value, None))
    value = clone(support_base)
    value["points"][0][0] = "3FF0000000000000"
    variants.append(("support-uppercase-word.json", value, None))
    value = clone(support_base)
    value["points"][0][0] = "7ff0000000000000"
    variants.append(("support-nonfinite.json", value, None))
    value = clone(support_base)
    value["points"][0] = value["points"][0][:2]
    variants.append(("support-short-point.json", value, None))
    value = clone(support_base)
    value["schema_version"] = True
    variants.append(("support-boolean-version.json", value, None))
    duplicate_raw = canonical_json(support_base).replace(
        '"schema_version":5', '"schema_version":5,"schema_version":5'
    )
    variants.append(("support-duplicate-key.json", None, duplicate_raw))

    value = clone(sphere_base)
    del value["center_exact"]
    variants.append(("sphere-missing-center.json", value, None))
    value = clone(sphere_base)
    value["center_exact"]["extra"] = "closed"
    variants.append(("sphere-open-center.json", value, None))
    value = clone(sphere_base)
    value["center_exact"]["denominator"] = "2"
    variants.append(("sphere-unreduced-center.json", value, None))
    value = clone(sphere_base)
    value["squared_level_exact"]["numerator"] = "-1"
    variants.append(("sphere-negative-level.json", value, None))
    value = clone(sphere_base)
    value["squared_level_exact"]["numerator"] = "2"
    value["squared_level_exact"]["denominator"] = "2"
    variants.append(("sphere-unreduced-level.json", value, None))
    value = clone(sphere_base)
    value["squared_level_exact"]["unit"] = "wrong"
    variants.append(("sphere-wrong-unit.json", value, None))
    value = clone(sphere_base)
    value["point"][0] = "7ff8000000000000"
    variants.append(("sphere-nonfinite-point.json", value, None))
    value = clone(sphere_base)
    value["point"] = value["point"][:2]
    variants.append(("sphere-short-point.json", value, None))
    value = clone(sphere_base)
    value["points"] = [value.pop("point")]
    variants.append(("sphere-support-fields.json", value, None))
    value = clone(sphere_base)
    value["schema_version"] = 5.0
    variants.append(("sphere-floating-version.json", value, None))

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-support-invalid-") as temporary:
        directory = Path(temporary)
        for name, value, raw in variants:
            expect_closed_failure(
                wrapper, executable, write_case(directory, name, value, raw=raw)
            )
    return len(variants)


def make_fake_native(
    path: Path,
    output: dict[str, Any],
    *,
    diagnostic: str | None = None,
    canonical: bool = True,
) -> None:
    payload = canonical_json(output) if canonical else json.dumps(output, indent=2)
    diagnostic_statement = (
        f"sys.stderr.write({diagnostic!r} + '\\n')\n" if diagnostic is not None else ""
    )
    path.write_text(
        "#!/usr/bin/env python3\n"
        "import sys\n"
        + diagnostic_statement
        + f"sys.stdout.write({payload!r} + '\\n')\n",
        encoding="utf-8",
    )
    path.chmod(0o755)


def check_false_native_outputs(
    wrapper: Path,
    executable: Path,
    fixture_paths: dict[str, Path],
) -> tuple[int, int]:
    def result(name: str) -> dict[str, Any]:
        return run_wrapper(wrapper, executable, fixture_paths[name])[1]["result"]

    pair_name = "support_pair_distinct.json"
    right_name = "support_triangle_right.json"
    exterior_name = "support_triangle_obtuse.json"
    dependent_name = "support_pair_duplicate.json"
    sphere_name = "sphere_boundary.json"
    pair = result(pair_name)
    right = result(right_name)
    exterior = result(exterior_name)
    dependent = result(dependent_name)
    sphere = result(sphere_name)

    variants: list[tuple[str, str, dict[str, Any]]] = []

    def changed(base: dict[str, Any], field: str, value: Any) -> dict[str, Any]:
        copy = json.loads(json.dumps(base))
        copy[field] = value
        return copy

    variants.extend(
        [
            ("wrong-dimension", pair_name, changed(pair, "affine_dimension", 0)),
            ("wrong-size", pair_name, changed(pair, "support_size", 3)),
            ("wrong-status", pair_name, changed(pair, "support_status", "boundary_reduced")),
            (
                "wrong-center",
                pair_name,
                changed(pair, "center_exact", rational3_record((Fraction(1), Fraction(), Fraction()))),
            ),
            ("wrong-level", pair_name, changed(pair, "squared_level_exact", level_record(Fraction(1)))),
            (
                "wrong-barycentric",
                pair_name,
                changed(pair, "barycentric_coordinates_exact", [rational_record(Fraction(1, 3)), rational_record(Fraction(2, 3))]),
            ),
            ("wrong-barycentric-sign", pair_name, changed(pair, "barycentric_signs", ["negative", "positive"])),
            ("wrong-location", pair_name, changed(pair, "convex_hull_location", "exterior")),
            ("wrong-reduction", right_name, changed(right, "reduced_support_indices", [0, 1])),
            ("exterior-reduction", exterior_name, changed(exterior, "reduced_support_indices", [0, 1])),
            (
                "dependent-witness",
                dependent_name,
                changed(dependent, "center_exact", rational3_record((Fraction(1), Fraction(-2), Fraction(3)))),
            ),
            ("missing-independent-witness", pair_name, changed(pair, "center_exact", None)),
            (
                "wrong-support-counters",
                pair_name,
                changed(pair, "counters", {**pair["counters"], "cpu_multiprecision_certified": 1}),
            ),
            ("support-extra-output", pair_name, {**pair, "extra": True}),
        ]
    )
    variants.extend(
        [
            ("wrong-sphere-distance", sphere_name, changed(sphere, "squared_distance_exact", level_record(Fraction(2)))),
            ("wrong-sphere-offset", sphere_name, changed(sphere, "signed_offset_exact", rational_record(Fraction(1)))),
            ("wrong-sphere-sign", sphere_name, changed(sphere, "sign", "positive")),
            ("wrong-sphere-class", sphere_name, changed(sphere, "classification", "outside")),
            ("wrong-sphere-stage", sphere_name, changed(sphere, "certification_stage", "fp64_filtered")),
            (
                "wrong-sphere-counters",
                sphere_name,
                changed(sphere, "counters", {**sphere["counters"], "exact_zeros": 0}),
            ),
            ("wrong-sphere-predicate", sphere_name, changed(sphere, "predicate", SUPPORT_PREDICATE)),
            ("sphere-extra-output", sphere_name, {**sphere, "extra": True}),
        ]
    )

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-support-fake-") as temporary:
        directory = Path(temporary)
        for name, fixture_name, output in variants:
            fake = directory / f"fake-{name}"
            make_fake_native(fake, output)
            expect_closed_failure(wrapper, fake, fixture_paths[fixture_name])
        noncanonical = directory / "fake-noncanonical"
        make_fake_native(noncanonical, pair, canonical=False)
        expect_closed_failure(wrapper, noncanonical, fixture_paths[pair_name])
        warning = directory / "fake-warning"
        make_fake_native(warning, pair, diagnostic="unexpected native support warning")
        expect_closed_failure(wrapper, warning, fixture_paths[pair_name])
    return len(variants), 1


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wrapper", type=Path)
    parser.add_argument("executable", type=Path)
    parser.add_argument("fixture_directory", type=Path)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    wrapper = arguments.wrapper.resolve(strict=True)
    executable = arguments.executable.resolve(strict=True)
    fixture_directory = arguments.fixture_directory.resolve(strict=True)
    support_paths = {
        path.name: path for path in sorted(fixture_directory.glob("support_*.json"))
    }
    sphere_paths = {
        path.name: path for path in sorted(fixture_directory.glob("sphere_*.json"))
    }
    if frozenset(support_paths) != EXPECTED_SUPPORT_FIXTURES:
        raise AssertionError(
            "the frozen support fixture set changed: "
            f"missing={sorted(EXPECTED_SUPPORT_FIXTURES - frozenset(support_paths))}, "
            f"unexpected={sorted(frozenset(support_paths) - EXPECTED_SUPPORT_FIXTURES)}"
        )
    if frozenset(sphere_paths) != EXPECTED_SPHERE_FIXTURES:
        raise AssertionError(
            "the frozen sphere fixture set changed: "
            f"missing={sorted(EXPECTED_SPHERE_FIXTURES - frozenset(sphere_paths))}, "
            f"unexpected={sorted(frozenset(sphere_paths) - EXPECTED_SPHERE_FIXTURES)}"
        )

    support_fixtures = {name: load_json(path) for name, path in support_paths.items()}
    sphere_fixtures = {name: load_json(path) for name, path in sphere_paths.items()}
    fixture_paths = {**support_paths, **sphere_paths}
    rational_boundary = sphere_fixtures["sphere_rational_boundary.json"]
    if (
        rational_boundary["center_exact"]["denominator"] != "3"
        or rational_boundary["squared_level_exact"]["numerator"] != "4"
        or rational_boundary["squared_level_exact"]["denominator"] != "9"
        or sphere_oracle(rational_boundary)["classification"] != "boundary"
    ):
        raise AssertionError("the rational sphere fixture lost its nontrivial denominators")
    zero_level_boundary = sphere_fixtures["sphere_zero_level_boundary.json"]
    if (
        zero_level_boundary["squared_level_exact"]["numerator"] != "0"
        or sphere_oracle(zero_level_boundary)["classification"] != "boundary"
    ):
        raise AssertionError("the zero-level sphere fixture lost its boundary contract")
    statuses: set[str] = set()
    locations: set[str] = set()
    reduced_sizes: set[int] = set()
    sphere_classes: set[str] = set()
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-support-mp-") as temporary:
        multiprecision = Path(temporary) / "multiprecision-native"
        make_multiprecision_forwarder(multiprecision, executable)
        for name, fixture in {**support_fixtures, **sphere_fixtures}.items():
            first_stdout, first_replay = run_wrapper(wrapper, executable, fixture_paths[name])
            second_stdout, second_replay = run_wrapper(wrapper, executable, fixture_paths[name])
            mp_stdout, mp_replay = run_wrapper(wrapper, multiprecision, fixture_paths[name])
            if (
                first_stdout != second_stdout
                or first_replay != second_replay
                or first_stdout != mp_stdout
                or first_replay != mp_replay
            ):
                raise AssertionError(f"{name} is not byte-stable in normal and MP modes")
            audit_envelope(first_replay, fixture)
            expected = audit_scientific_result(first_replay)
            if fixture["predicate"] == SUPPORT_PREDICATE:
                statuses.add(expected["support_status"])
                if expected["convex_hull_location"] is not None:
                    locations.add(expected["convex_hull_location"])
                if expected["reduced_support_indices"] is not None:
                    reduced_sizes.add(len(expected["reduced_support_indices"]))
            else:
                sphere_classes.add(expected["classification"])

    if statuses != {
        "minimal",
        "boundary_reduced",
        "exterior_circumcenter",
        "affinely_dependent",
    }:
        raise AssertionError(f"support fixtures miss statuses: {statuses}")
    if locations != {"relative_interior", "relative_boundary", "exterior"}:
        raise AssertionError(f"support fixtures miss hull locations: {locations}")
    if reduced_sizes != {1, 2, 3, 4}:
        raise AssertionError(f"support fixtures miss reduced sizes: {reduced_sizes}")
    if sphere_classes != {"strictly_inside", "boundary", "outside"}:
        raise AssertionError(f"sphere fixtures miss classifications: {sphere_classes}")

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-support-order-") as temporary:
        directory = Path(temporary)
        for name in ("support_triangle_acute.json", "sphere_boundary.json"):
            fixture = {**support_fixtures, **sphere_fixtures}[name]
            reordered = dict(reversed(list(fixture.items())))
            reordered_path = write_case(
                directory, f"reordered-{name}", raw=json.dumps(reordered, indent=2)
            )
            _, reordered_replay = run_wrapper(wrapper, executable, reordered_path)
            _, original_replay = run_wrapper(wrapper, executable, fixture_paths[name])
            if (
                reordered_replay["replay_id"] != original_replay["replay_id"]
                or reordered_replay["result"] != original_replay["result"]
            ):
                raise AssertionError(f"JSON order or whitespace changed v5 identity for {name}")

    check_historical_sentinels(wrapper, executable, fixture_directory)
    permutation_count = check_complete_permutations(
        wrapper, executable, support_fixtures
    )
    metamorphism_count = check_metamorphisms(
        wrapper, executable, support_fixtures, sphere_fixtures
    )
    invalid_count = check_invalid_inputs(
        wrapper,
        executable,
        support_fixtures["support_singleton.json"],
        sphere_fixtures["sphere_boundary.json"],
    )
    false_output_count, noncanonical_count = check_false_native_outputs(
        wrapper, executable, fixture_paths
    )
    print(
        "support replay differential checks passed: "
        f"fixtures={len(fixture_paths)}, support_fixtures={len(support_paths)}, "
        f"sphere_fixtures={len(sphere_paths)}, support_statuses={len(statuses)}, "
        f"hull_locations={len(locations)}, reduced_support_sizes={len(reduced_sizes)}, "
        f"sphere_classes={len(sphere_classes)}, complete_permutations={permutation_count}, "
        f"metamorphisms={metamorphism_count}, invalid_inputs={invalid_count}, "
        f"false_native_outputs={false_output_count}, noncanonical_stdout={noncanonical_count}, "
        f"unexpected_stderr=1, historical_sentinels={len(HISTORICAL_SENTINELS)}, "
        "normal_mp_byte_identical=true"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
