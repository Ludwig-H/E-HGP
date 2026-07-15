#!/usr/bin/env python3
"""Audit the v4 circumcenter replay with an independent Fraction oracle."""

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
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path
from typing import Any, Callable


INPUT_KIND = "morsehgp3d_predicate_replay_input"
RESULT_KIND = "morsehgp3d_predicate_replay_result"
PREDICATE = "circumcenter_support"
V4_DOMAIN = b"MorseHGP3D/predicate-replay-v4/"
HISTORICAL_DOMAINS = {
    1: b"MorseHGP3D/predicate-replay-v1/",
    2: b"MorseHGP3D/predicate-replay-v2/",
    3: b"MorseHGP3D/predicate-replay-v3/",
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
}
EXPECTED_CENTER_FIXTURES = frozenset(
    {
        "center_pair_duplicate.json",
        "center_pair_max_finite.json",
        "center_pair_midpoint.json",
        "center_pair_subnormal.json",
        "center_tetrahedron_coplanar.json",
        "center_tetrahedron_nondyadic.json",
        "center_tetrahedron_normal_one_ulp.json",
        "center_tetrahedron_one_ulp.json",
        "center_triangle_collinear.json",
        "center_triangle_nondyadic.json",
        "center_triangle_normal_one_ulp.json",
        "center_triangle_obtuse.json",
        "center_triangle_one_ulp.json",
    }
)
RESULT_FIELDS = {
    "affine_dimension",
    "center_exact",
    "predicate",
    "squared_level_exact",
    "support_kind",
    "support_size",
}
RATIONAL3_FIELDS = {
    "denominator",
    "schema_version",
    "unit",
    "x_numerator",
    "y_numerator",
    "z_numerator",
}
LEVEL_FIELDS = {"denominator", "numerator", "schema_version", "unit"}
WRAPPER_TIMEOUT_SECONDS = 40

Point = tuple[Fraction, Fraction, Fraction]


@dataclass(frozen=True)
class CenterOracle:
    support_kind: str
    affine_dimension: int
    center: Point | None
    squared_level: Fraction | None


def canonical_json(value: object) -> str:
    return json.dumps(
        value, allow_nan=False, ensure_ascii=False, separators=(",", ":"), sort_keys=True
    )


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def point_from_words(words: list[str]) -> Point:
    if len(words) != 3:
        raise AssertionError("a center replay point does not have three coordinates")
    return tuple(
        Fraction.from_float(struct.unpack(">d", bytes.fromhex(word))[0])
        for word in words
    )  # type: ignore[return-value]


def words_from_point(point: Point) -> list[str]:
    words: list[str] = []
    for coordinate in point:
        value = float(coordinate)
        if not math.isfinite(value) or Fraction.from_float(value) != coordinate:
            raise AssertionError("a center metamorphism left exact binary64 input space")
        words.append(struct.pack(">d", value).hex())
    return words


def add(left: Point, right: Point) -> Point:
    return tuple(a + b for a, b in zip(left, right))  # type: ignore[return-value]


def subtract(left: Point, right: Point) -> Point:
    return tuple(a - b for a, b in zip(left, right))  # type: ignore[return-value]


def multiply(point: Point, factor: Fraction) -> Point:
    return tuple(coordinate * factor for coordinate in point)  # type: ignore[return-value]


def dot(left: Point, right: Point) -> Fraction:
    return sum((a * b for a, b in zip(left, right)), Fraction())


def squared_distance(left: Point, right: Point) -> Fraction:
    delta = subtract(left, right)
    return dot(delta, delta)


def matrix_rref(
    matrix: list[list[Fraction]],
) -> tuple[list[list[Fraction]], list[int]]:
    reduced = [list(row) for row in matrix]
    pivots: list[int] = []
    if not reduced:
        return reduced, pivots
    pivot_row = 0
    for column in range(len(reduced[0])):
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


def circumcenter_oracle(points: list[Point]) -> CenterOracle:
    if not 2 <= len(points) <= 4:
        raise AssertionError("a center oracle support must contain two to four points")
    origin = points[0]
    directions = [subtract(point, origin) for point in points[1:]]
    _, affine_pivots = matrix_rref([list(direction) for direction in directions])
    affine_dimension = len(affine_pivots)
    expected_dimension = len(points) - 1
    if affine_dimension != expected_dimension:
        return CenterOracle("affinely_dependent", affine_dimension, None, None)

    gram = [[dot(left, right) for right in directions] for left in directions]
    augmented = [
        [*row, gram[index][index] / 2] for index, row in enumerate(gram)
    ]
    reduced, pivots = matrix_rref(augmented)
    if pivots != list(range(expected_dimension)):
        raise AssertionError("an independent support produced a singular Gram system")
    coefficients = [reduced[index][-1] for index in range(expected_dimension)]
    center = tuple(
        origin[axis]
        + sum(
            (
                coefficients[index] * directions[index][axis]
                for index in range(expected_dimension)
            ),
            Fraction(),
        )
        for axis in range(3)
    )
    squared_level = squared_distance(center, origin)
    if squared_level <= 0:
        raise AssertionError("an independent support produced a nonpositive radius")
    if any(squared_distance(center, point) != squared_level for point in points[1:]):
        raise AssertionError("the Gram center is not equidistant from its support")
    reconstructed = add(
        origin,
        tuple(
            sum(
                (
                    coefficients[index] * directions[index][axis]
                    for index in range(expected_dimension)
                ),
                Fraction(),
            )
            for axis in range(3)
        ),  # type: ignore[arg-type]
    )
    if reconstructed != center:
        raise AssertionError("the Gram center left the affine hull of its support")
    return CenterOracle(
        "affinely_independent", affine_dimension, center, squared_level
    )


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


def rational3_from_record(value: Any, label: str) -> Point:
    if not isinstance(value, dict) or set(value) != RATIONAL3_FIELDS:
        raise AssertionError(f"{label} has an open ExactRational3 schema")
    if value["schema_version"] != "2.0.0" or value["unit"] != "input_coordinate_unit":
        raise AssertionError(f"{label} has the wrong ExactRational3 contract")
    integer_fields = ["denominator", "x_numerator", "y_numerator", "z_numerator"]
    if any(not isinstance(value[field], str) for field in integer_fields):
        raise AssertionError(f"{label} has a non-string exact integer")
    denominator = int(value["denominator"])
    numerators = [int(value[f"{axis}_numerator"]) for axis in "xyz"]
    if denominator <= 0 or any(
        value[field] != str(int(value[field])) for field in integer_fields
    ):
        raise AssertionError(f"{label} has noncanonical exact integers")
    point = tuple(Fraction(numerator, denominator) for numerator in numerators)
    if value != rational3_record(point):
        raise AssertionError(f"{label} is not jointly reduced canonically")
    return point  # type: ignore[return-value]


def level_record(value: Fraction) -> dict[str, str]:
    return {
        "denominator": str(value.denominator),
        "numerator": str(value.numerator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }


def level_from_record(value: Any, label: str) -> Fraction:
    if not isinstance(value, dict) or set(value) != LEVEL_FIELDS:
        raise AssertionError(f"{label} has an open ExactLevel schema")
    if (
        value["schema_version"] != "2.0.0"
        or value["unit"] != "input_coordinate_unit_squared"
        or not isinstance(value["numerator"], str)
        or not isinstance(value["denominator"], str)
    ):
        raise AssertionError(f"{label} has the wrong ExactLevel contract")
    numerator = int(value["numerator"])
    denominator = int(value["denominator"])
    if (
        numerator < 0
        or denominator <= 0
        or value["numerator"] != str(numerator)
        or value["denominator"] != str(denominator)
        or math.gcd(numerator, denominator) != 1
    ):
        raise AssertionError(f"{label} is not a canonical nonnegative level")
    return Fraction(numerator, denominator)


def audit_scientific_result(replay: dict[str, Any]) -> CenterOracle:
    replay_input = replay["input"]
    result = replay["result"]
    if not isinstance(result, dict) or set(result) != RESULT_FIELDS:
        raise AssertionError("the circumcenter result schema is open")
    points = [point_from_words(words) for words in replay_input["points"]]
    expected = circumcenter_oracle(points)
    if (
        result["predicate"] != PREDICATE
        or result["support_kind"] != expected.support_kind
        or type(result["support_size"]) is not int
        or result["support_size"] != len(points)
        or type(result["affine_dimension"]) is not int
        or result["affine_dimension"] != expected.affine_dimension
    ):
        raise AssertionError("the native circumcenter invariants differ from the oracle")
    if expected.center is None or expected.squared_level is None:
        if result["center_exact"] is not None or result["squared_level_exact"] is not None:
            raise AssertionError("a dependent support exposed circumcenter witnesses")
        return expected

    center = rational3_from_record(result["center_exact"], "circumcenter")
    squared_level = level_from_record(
        result["squared_level_exact"], "circumcenter squared level"
    )
    if center != expected.center or squared_level != expected.squared_level:
        raise AssertionError("the native witnesses differ from the Fraction Gram oracle")
    if any(squared_distance(center, point) != squared_level for point in points):
        raise AssertionError("the native center is not equidistant from its support")
    return expected


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
        raise AssertionError(f"{fixture.name} does not produce canonical replay JSON")
    return completed.stdout, replay


def audit_v4_envelope(replay: dict[str, Any], fixture_value: dict[str, Any]) -> None:
    if set(replay) != {"input", "kind", "replay_id", "result", "schema_version"}:
        raise AssertionError("the v4 replay envelope schema is open")
    if (
        replay["kind"] != RESULT_KIND
        or replay["schema_version"] != 4
        or replay["input"] != fixture_value
        or replay["input"].get("kind") != INPUT_KIND
        or replay["input"].get("schema_version") != 4
        or replay["input"].get("predicate") != PREDICATE
    ):
        raise AssertionError("the wrapper changed or mis-versioned its v4 input")
    serialized = canonical_json(replay["input"]).encode("utf-8")
    expected_id = hashlib.sha256(V4_DOMAIN + serialized).hexdigest()
    if replay["replay_id"] != expected_id:
        raise AssertionError("the v4 replay identifier uses the wrong bytes or domain")
    for domain in HISTORICAL_DOMAINS.values():
        if replay["replay_id"] == hashlib.sha256(domain + serialized).hexdigest():
            raise AssertionError("the v4 replay identifier aliases a historical domain")


def write_case(
    directory: Path, name: str, value: dict[str, Any], *, pretty: bool = False
) -> Path:
    path = directory / name
    serialized = (
        json.dumps(value, ensure_ascii=False, indent=2)
        if pretty
        else canonical_json(value)
    )
    path.write_text(serialized + "\n", encoding="utf-8")
    return path


def run_generated(
    wrapper: Path,
    executable: Path,
    directory: Path,
    name: str,
    value: dict[str, Any],
) -> tuple[dict[str, Any], CenterOracle]:
    _, replay = run_wrapper(wrapper, executable, write_case(directory, name, value))
    audit_v4_envelope(replay, value)
    return replay, audit_scientific_result(replay)


def check_historical_sentinels(
    wrapper: Path, executable: Path, fixture_directory: Path
) -> None:
    for filename, (version, expected_id) in HISTORICAL_SENTINELS.items():
        path = fixture_directory / filename
        if not path.is_file():
            raise AssertionError(f"historical sentinel fixture is missing: {filename}")
        _, replay = run_wrapper(wrapper, executable, path)
        if replay.get("schema_version") != version or replay.get("replay_id") != expected_id:
            raise AssertionError(f"the historical replay identity changed for {filename}")
        serialized = canonical_json(replay["input"]).encode("utf-8")
        if hashlib.sha256(HISTORICAL_DOMAINS[version] + serialized).hexdigest() != expected_id:
            raise AssertionError(f"the historical domain check failed for {filename}")


def make_input(points: list[list[str]]) -> dict[str, Any]:
    return {
        "kind": INPUT_KIND,
        "points": points,
        "predicate": PREDICATE,
        "schema_version": 4,
    }


def check_complete_permutations(
    wrapper: Path,
    executable: Path,
    fixtures: dict[str, dict[str, Any]],
    fixture_replays: dict[str, dict[str, Any]],
) -> int:
    selected = [
        "center_pair_midpoint.json",
        "center_pair_duplicate.json",
        "center_triangle_nondyadic.json",
        "center_triangle_collinear.json",
        "center_tetrahedron_nondyadic.json",
        "center_tetrahedron_coplanar.json",
    ]
    count = 0
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-center-permutations-") as temporary:
        directory = Path(temporary)
        for filename in selected:
            base = fixtures[filename]
            expected_result = fixture_replays[filename]["result"]
            for index, permutation in enumerate(itertools.permutations(base["points"])):
                transformed = make_input([list(point) for point in permutation])
                replay, _ = run_generated(
                    wrapper,
                    executable,
                    directory,
                    f"{Path(filename).stem}-{index}.json",
                    transformed,
                )
                if replay["result"] != expected_result:
                    raise AssertionError(
                        f"a complete support permutation changed {filename}"
                    )
                count += 1
    return count


def signed_axis_map(point: Point) -> Point:
    permutation = (2, 0, 1)
    signs = (Fraction(-1), Fraction(1), Fraction(-1))
    return tuple(
        signs[axis] * point[permutation[axis]] for axis in range(3)
    )  # type: ignore[return-value]


def check_metamorphisms(
    wrapper: Path,
    executable: Path,
    fixtures: dict[str, dict[str, Any]],
) -> int:
    selected = [
        "center_pair_midpoint.json",
        "center_pair_duplicate.json",
        "center_triangle_nondyadic.json",
        "center_triangle_collinear.json",
        "center_tetrahedron_nondyadic.json",
        "center_tetrahedron_coplanar.json",
    ]
    translation: Point = (Fraction(2), Fraction(-3), Fraction(1, 2))
    scale = Fraction(4)
    transforms: list[
        tuple[str, Callable[[Point], Point], Callable[[Point], Point], Fraction]
    ] = [
        (
            "translation",
            lambda point: add(point, translation),
            lambda center: add(center, translation),
            Fraction(1),
        ),
        ("signed_axes", signed_axis_map, signed_axis_map, Fraction(1)),
        (
            "power_of_two_scale",
            lambda point: multiply(point, scale),
            lambda center: multiply(center, scale),
            scale * scale,
        ),
    ]
    count = 0
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-center-metamorphisms-") as temporary:
        directory = Path(temporary)
        for filename in selected:
            base_points = [point_from_words(words) for words in fixtures[filename]["points"]]
            base_oracle = circumcenter_oracle(base_points)
            for transform_name, point_map, center_map, level_factor in transforms:
                transformed_points = [point_map(point) for point in base_points]
                transformed = make_input(
                    [words_from_point(point) for point in transformed_points]
                )
                _, transformed_oracle = run_generated(
                    wrapper,
                    executable,
                    directory,
                    f"{Path(filename).stem}-{transform_name}.json",
                    transformed,
                )
                if (
                    transformed_oracle.support_kind != base_oracle.support_kind
                    or transformed_oracle.affine_dimension
                    != base_oracle.affine_dimension
                ):
                    raise AssertionError(
                        f"{transform_name} changed the support class for {filename}"
                    )
                if base_oracle.center is None or base_oracle.squared_level is None:
                    if (
                        transformed_oracle.center is not None
                        or transformed_oracle.squared_level is not None
                    ):
                        raise AssertionError(
                            f"{transform_name} materialized witnesses for {filename}"
                        )
                elif (
                    transformed_oracle.center != center_map(base_oracle.center)
                    or transformed_oracle.squared_level
                    != base_oracle.squared_level * level_factor
                ):
                    raise AssertionError(
                        f"{transform_name} violated center covariance for {filename}"
                    )
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
    wrapper: Path, executable: Path, fixtures: dict[str, dict[str, Any]]
) -> int:
    base = fixtures["center_pair_midpoint.json"]
    variants: list[tuple[str, dict[str, Any]]] = []

    unknown = json.loads(json.dumps(base))
    unknown["unexpected"] = "closed schemas only"
    variants.append(("unknown-field.json", unknown))
    old_schema = json.loads(json.dumps(base))
    old_schema["schema_version"] = 3
    variants.append(("center-under-v3.json", old_schema))
    wrong_predicate = json.loads(json.dumps(base))
    wrong_predicate["predicate"] = "plane_through_points"
    variants.append(("wrong-v4-predicate.json", wrong_predicate))
    one_point = json.loads(json.dumps(base))
    one_point["points"] = one_point["points"][:1]
    variants.append(("one-point.json", one_point))
    five_points = json.loads(json.dumps(base))
    five_points["points"] = [*five_points["points"], *five_points["points"], base["points"][0]]
    variants.append(("five-points.json", five_points))
    uppercase = json.loads(json.dumps(base))
    uppercase["points"][1][0] = "3FF0000000000000"
    variants.append(("uppercase-word.json", uppercase))
    nonfinite = json.loads(json.dumps(base))
    nonfinite["points"][1][0] = "7ff0000000000000"
    variants.append(("nonfinite-word.json", nonfinite))
    short_point = json.loads(json.dumps(base))
    short_point["points"][0] = short_point["points"][0][:2]
    variants.append(("short-point.json", short_point))
    boolean_version = json.loads(json.dumps(base))
    boolean_version["schema_version"] = True
    variants.append(("boolean-version.json", boolean_version))
    floating_version = json.loads(json.dumps(base))
    floating_version["schema_version"] = 4.0
    variants.append(("floating-version.json", floating_version))

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-center-invalid-") as temporary:
        directory = Path(temporary)
        for name, value in variants:
            expect_closed_failure(
                wrapper, executable, write_case(directory, name, value)
            )
    return len(variants)


def make_fake_native(
    path: Path, output: dict[str, Any], *, diagnostic: str | None = None
) -> None:
    payload = canonical_json(output)
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
) -> int:
    pair_name = "center_pair_midpoint.json"
    dependent_name = "center_pair_duplicate.json"
    triangle_name = "center_triangle_nondyadic.json"
    valid_pair = run_wrapper(wrapper, executable, fixture_paths[pair_name])[1]["result"]
    valid_dependent = run_wrapper(
        wrapper, executable, fixture_paths[dependent_name]
    )[1]["result"]
    valid_triangle = run_wrapper(
        wrapper, executable, fixture_paths[triangle_name]
    )[1]["result"]

    off_affine = json.loads(json.dumps(valid_pair))
    off_affine["center_exact"] = rational3_record(
        (Fraction(1), Fraction(1), Fraction())
    )
    off_affine["squared_level_exact"] = level_record(Fraction(2))
    wrong_level = json.loads(json.dumps(valid_pair))
    wrong_level["squared_level_exact"] = level_record(Fraction(2))
    wrong_triangle_center = json.loads(json.dumps(valid_triangle))
    wrong_triangle_center["center_exact"] = rational3_record(
        (Fraction(4, 3), Fraction(-1, 6), Fraction())
    )
    wrong_dimension = json.loads(json.dumps(valid_pair))
    wrong_dimension["affine_dimension"] = 0
    wrong_kind = json.loads(json.dumps(valid_pair))
    wrong_kind["support_kind"] = "affinely_dependent"
    wrong_size = json.loads(json.dumps(valid_pair))
    wrong_size["support_size"] = 3
    dependent_witness = json.loads(json.dumps(valid_dependent))
    dependent_witness["center_exact"] = rational3_record(
        (Fraction(1), Fraction(-2), Fraction(3))
    )
    dependent_witness["squared_level_exact"] = level_record(Fraction())
    dependent_rank = json.loads(json.dumps(valid_dependent))
    dependent_rank["affine_dimension"] = 1

    variants = [
        ("off-affine-equidistant", pair_name, off_affine),
        ("wrong-level", pair_name, wrong_level),
        ("wrong-triangle-center", triangle_name, wrong_triangle_center),
        ("wrong-dimension", pair_name, wrong_dimension),
        ("wrong-kind", pair_name, wrong_kind),
        ("wrong-size", pair_name, wrong_size),
        ("dependent-witness", dependent_name, dependent_witness),
        ("dependent-rank", dependent_name, dependent_rank),
    ]
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-center-fake-") as temporary:
        directory = Path(temporary)
        for name, fixture_name, output in variants:
            fake = directory / f"fake-{name}"
            make_fake_native(fake, output)
            expect_closed_failure(wrapper, fake, fixture_paths[fixture_name])
        warning = directory / "fake-warning"
        make_fake_native(
            warning, valid_pair, diagnostic="unexpected native center warning"
        )
        expect_closed_failure(wrapper, warning, fixture_paths[pair_name])
    return len(variants)


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
    fixture_paths = {
        path.name: path for path in sorted(fixture_directory.glob("center_*.json"))
    }
    fixture_names = frozenset(fixture_paths)
    if fixture_names != EXPECTED_CENTER_FIXTURES:
        missing = sorted(EXPECTED_CENTER_FIXTURES - fixture_names)
        unexpected = sorted(fixture_names - EXPECTED_CENTER_FIXTURES)
        raise AssertionError(
            "the frozen center replay fixture set changed: "
            f"missing={missing}, unexpected={unexpected}"
        )

    fixtures: dict[str, dict[str, Any]] = {}
    fixture_replays: dict[str, dict[str, Any]] = {}
    support_sizes: set[int] = set()
    independent_sizes: set[int] = set()
    dependent_dimensions: set[int] = set()
    for name, path in fixture_paths.items():
        fixture = load_json(path)
        fixtures[name] = fixture
        first_stdout, first_replay = run_wrapper(wrapper, executable, path)
        second_stdout, second_replay = run_wrapper(wrapper, executable, path)
        if first_stdout != second_stdout or first_replay != second_replay:
            raise AssertionError(f"{name} is not byte-for-byte replay-stable")
        audit_v4_envelope(first_replay, fixture)
        oracle = audit_scientific_result(first_replay)
        fixture_replays[name] = first_replay
        size = len(fixture["points"])
        support_sizes.add(size)
        if oracle.support_kind == "affinely_independent":
            independent_sizes.add(size)
        else:
            dependent_dimensions.add(oracle.affine_dimension)

    if support_sizes != {2, 3, 4} or independent_sizes != {2, 3, 4}:
        raise AssertionError("the center fixtures do not cover every support size")
    if dependent_dimensions != {0, 1, 2}:
        raise AssertionError(
            "the center fixtures do not cover dependent affine dimensions 0, 1 and 2"
        )

    order_base_name = "center_triangle_nondyadic.json"
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-center-order-") as temporary:
        reordered = dict(reversed(list(fixtures[order_base_name].items())))
        reordered_path = write_case(
            Path(temporary), "reordered.json", reordered, pretty=True
        )
        _, reordered_replay = run_wrapper(wrapper, executable, reordered_path)
        original_replay = fixture_replays[order_base_name]
        if (
            reordered_replay["replay_id"] != original_replay["replay_id"]
            or reordered_replay["result"] != original_replay["result"]
        ):
            raise AssertionError("JSON order or whitespace changed v4 replay identity")

    check_historical_sentinels(wrapper, executable, fixture_directory)
    permutation_count = check_complete_permutations(
        wrapper, executable, fixtures, fixture_replays
    )
    metamorphism_count = check_metamorphisms(wrapper, executable, fixtures)
    invalid_count = check_invalid_inputs(wrapper, executable, fixtures)
    false_output_count = check_false_native_outputs(
        wrapper, executable, fixture_paths
    )
    print(
        "center replay differential checks passed: "
        f"fixtures={len(fixtures)}, support_sizes={len(support_sizes)}, "
        f"dependent_dimensions={len(dependent_dimensions)}, "
        f"complete_permutations={permutation_count}, "
        f"metamorphisms={metamorphism_count}, invalid_inputs={invalid_count}, "
        f"coherent_false_outputs={false_output_count}, unexpected_stderr=1, "
        f"historical_sentinels={len(HISTORICAL_SENTINELS)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
