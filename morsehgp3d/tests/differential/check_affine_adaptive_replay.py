#!/usr/bin/env python3
"""Audit the frozen v8 adaptive affine fixtures through wrapper and native replay."""

from __future__ import annotations

import argparse
from copy import deepcopy
from fractions import Fraction
import hashlib
import importlib.util
import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path
from types import ModuleType
from typing import Any, Sequence

from check_affine_adaptive_random import (
    Case,
    PlaneSource,
    case_sign,
    canonical_json,
    coefficient_source,
    exact_source,
    fourth_case,
    intersection_case,
    orientation_case,
    plane_side_case,
    power_source,
    scientific_projection,
    through_source,
    validate_stage_and_counters,
)

INPUT_KIND = "morsehgp3d_predicate_replay_input"
RESULT_KIND = "morsehgp3d_predicate_replay_result"
V8_DOMAIN = b"MorseHGP3D/predicate-replay-v8/"
EXPECTED_FIXTURE_COUNT = 37
EXPECTED_FIXTURE_COUNTS = {
    "adaptive_fourth_plane_incidence": 8,
    "adaptive_intersect_three_planes": 11,
    "adaptive_orientation_2d_in_plane": 9,
    "adaptive_plane_side": 9,
}
EXPECTED_FIXTURE_MANIFEST_SHA256 = (
    "20649770c099e9c4fca6837af0ea86af0b8e7399d82c786e4ed532941727cf49"
)
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
    "sphere_rational_boundary.json": (
        5,
        "b2fc1c9710f09984ac90c56fc522a43eae52d90606e0f5b5f1d6ade4c1b74c3d",
    ),
    "level_compare_cross_minus_one.json": (
        6,
        "7b2ffaf345056baeb42c6d578411b01dfed2b99fd3b6daed35f475671ef70a37",
    ),
    "adaptive_sphere_fp64_inside.json": (
        7,
        "43c5cb66e9e79a129a99db2eaba96ecaa48f697f90808e67cb799f8b50c97d7d",
    ),
}
PREDICATES = {
    "adaptive_orientation_2d_in_plane",
    "adaptive_plane_side",
    "adaptive_intersect_three_planes",
    "adaptive_fourth_plane_incidence",
}
STAGES = ("fp64_filtered", "expansion", "cpu_multiprecision")
SIGNS = ("negative", "zero", "positive")
ORIGINS = ("coeff", "through", "power", "exact")
EXPECTED_ZERO_RANK_CLASSES = {
    ("empty", 1, 2, None),
    ("empty", 2, 3, None),
    ("affine_family", 2, 2, 1),
    ("affine_family", 1, 1, 2),
}
WRAPPER_TIMEOUT_SECONDS = 40
EXPANSION_MAGNITUDE = 1 << 26


def strict_json_loads(text: str) -> Any:
    def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate JSON key: {key}")
            result[key] = value
        return result

    def reject_nonfinite(value: str) -> None:
        raise ValueError(f"non-finite JSON number is forbidden: {value}")

    return json.loads(
        text,
        object_pairs_hook=reject_duplicate_keys,
        parse_constant=reject_nonfinite,
    )


def load_fixture(path: Path) -> dict[str, Any]:
    text = path.read_text(encoding="utf-8")
    value = strict_json_loads(text)
    if not isinstance(value, dict):
        raise AssertionError(f"{path.name} is not a JSON object")
    if (
        value.get("kind") != INPUT_KIND
        or value.get("schema_version") != 8
        or value.get("predicate") not in PREDICATES
    ):
        raise AssertionError(f"{path.name} is not a closed v8 replay input")
    if text != canonical_json(value) + "\n":
        raise AssertionError(f"{path.name} is not canonical JSON")
    return value


def expected_replay_id(value: dict[str, Any]) -> str:
    return hashlib.sha256(V8_DOMAIN + canonical_json(value).encode("utf-8")).hexdigest()


def run_wrapper_cli(
    wrapper: Path,
    executable: Path,
    fixture: Path,
    prefix: Sequence[str] = (),
) -> dict[str, Any]:
    command = [
        sys.executable,
        str(wrapper),
        str(fixture),
        "--executable",
        str(executable),
    ]
    command.extend(f"--executable-prefix-argument={argument}" for argument in prefix)
    completed = subprocess.run(
        command,
        check=False,
        capture_output=True,
        encoding="utf-8",
        timeout=WRAPPER_TIMEOUT_SECONDS,
    )
    if completed.returncode != 0:
        raise AssertionError(
            f"wrapper failed for {fixture.name}: {completed.stderr.strip()}"
        )
    if completed.stderr:
        raise AssertionError(f"wrapper wrote diagnostics for {fixture.name}")
    value = strict_json_loads(completed.stdout)
    if not isinstance(value, dict) or completed.stdout != canonical_json(value) + "\n":
        raise AssertionError(f"wrapper output is not canonical for {fixture.name}")
    return value


def load_wrapper_module(wrapper: Path) -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "morsehgp3d_v8_replay_wrapper", wrapper
    )
    if specification is None or specification.loader is None:
        raise AssertionError("the replay wrapper cannot be imported")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


def run_wrapper_in_process(
    wrapper: ModuleType,
    executable: Path,
    fixture: dict[str, Any],
    prefix: Sequence[str] = (),
) -> dict[str, Any]:
    normalized = wrapper.validate_input(fixture)
    if normalized != fixture:
        raise AssertionError("the v8 wrapper changed a canonical fixture input")
    envelope = wrapper.replay(normalized, executable, tuple(prefix))
    if not isinstance(envelope, dict):
        raise AssertionError("the imported v8 wrapper did not return an envelope")
    return envelope


def run_native(
    executable: Path, case: Case, prefix: Sequence[str] = ()
) -> dict[str, Any]:
    completed = subprocess.run(
        [str(executable), *prefix, *case.command.split()],
        check=False,
        capture_output=True,
        encoding="utf-8",
        timeout=WRAPPER_TIMEOUT_SECONDS,
    )
    if completed.returncode != 0:
        raise AssertionError(
            f"native replay failed for {case.predicate}: {completed.stderr.strip()}"
        )
    if completed.stderr:
        raise AssertionError(f"native replay wrote diagnostics for {case.predicate}")
    value = strict_json_loads(completed.stdout)
    if not isinstance(value, dict) or completed.stdout != canonical_json(value) + "\n":
        raise AssertionError(f"native output is not canonical for {case.predicate}")
    return value


def point_from_words(words: object) -> tuple[Fraction, Fraction, Fraction]:
    if not isinstance(words, list) or len(words) != 3:
        raise AssertionError("a v8 fixture point does not contain three words")
    result = tuple(
        Fraction.from_float(struct.unpack(">d", bytes.fromhex(str(word)))[0])
        for word in words
    )
    return result  # type: ignore[return-value]


def source_from_fixture(value: object) -> PlaneSource:
    if not isinstance(value, dict):
        raise AssertionError("a v8 plane source is not an object")
    source_kind = value.get("source_kind")
    if source_kind == "binary64_coefficients":
        words = value.get("coefficient_words")
        if not isinstance(words, list) or len(words) != 4:
            raise AssertionError("a coefficient source does not contain four words")
        coefficients = tuple(
            Fraction.from_float(struct.unpack(">d", bytes.fromhex(str(word)))[0])
            for word in words
        )
        return coefficient_source(coefficients)
    if source_kind == "through_points":
        points = value.get("points")
        if not isinstance(points, list):
            raise AssertionError("a through source has no point array")
        return through_source([point_from_words(point) for point in points])
    if source_kind == "power_bisector":
        table = value.get("point_table")
        r_ids = value.get("r_ids")
        q_ids = value.get("q_ids")
        if (
            not isinstance(table, list)
            or not isinstance(r_ids, list)
            or not isinstance(q_ids, list)
        ):
            raise AssertionError("a power source has malformed labels")
        points = [point_from_words(point) for point in table]
        return power_source(
            [points[int(identifier)] for identifier in r_ids],
            [points[int(identifier)] for identifier in q_ids],
        )
    if source_kind == "exact_plane":
        plane = value.get("plane")
        if not isinstance(plane, dict):
            raise AssertionError("an exact source has no plane record")
        return exact_source(tuple(int(plane[field]) for field in ("a", "b", "c", "d")))
    raise AssertionError(f"unsupported v8 plane-source kind {source_kind!r}")


def case_from_fixture(value: dict[str, Any]) -> Case:
    predicate = value["predicate"]
    if predicate == "adaptive_plane_side":
        return plane_side_case(
            source_from_fixture(value["plane_source"]),
            point_from_words(value["point"]),
        )
    if predicate == "adaptive_orientation_2d_in_plane":
        return orientation_case(
            source_from_fixture(value["plane_source"]),
            [point_from_words(point) for point in value["points"]],
        )
    sources = [source_from_fixture(source) for source in value["plane_sources"]]
    if predicate == "adaptive_intersect_three_planes":
        return intersection_case(sources)
    if predicate == "adaptive_fourth_plane_incidence":
        return fourth_case(sources[:3], sources[3])
    raise AssertionError(f"unsupported adaptive fixture predicate {predicate}")


def expected_fixture_stage(name: str) -> str:
    if "_fp64_" in name:
        return "fp64_filtered"
    if "_expansion_" in name:
        return "expansion"
    if "_multiprecision_" in name:
        return "cpu_multiprecision"
    raise AssertionError(f"fixture {name} does not declare its forced stage")


def expected_fixture_sign(name: str) -> str:
    for sign in SIGNS:
        if f"_{sign}" in name:
            return sign
    raise AssertionError(f"fixture {name} does not declare its exact sign")


def audit_envelope(
    envelope: dict[str, Any], fixture: dict[str, Any], replay_id: str
) -> dict[str, Any]:
    if set(envelope) != {"input", "kind", "replay_id", "result", "schema_version"}:
        raise AssertionError("the v8 replay envelope is not closed")
    if (
        envelope["input"] != fixture
        or envelope["kind"] != RESULT_KIND
        or envelope["schema_version"] != 8
        or envelope["replay_id"] != replay_id
        or not isinstance(envelope["result"], dict)
    ):
        raise AssertionError(
            "the v8 replay envelope or domain-separated hash is invalid"
        )
    return envelope["result"]


def check_historical_sentinels(
    wrapper: Path, executable: Path, fixture_directory: Path
) -> None:
    for filename, (version, replay_id) in HISTORICAL_SENTINELS.items():
        replay = run_wrapper_cli(wrapper, executable, fixture_directory / filename)
        if (
            replay.get("schema_version") != version
            or replay.get("replay_id") != replay_id
        ):
            raise AssertionError(f"historical replay identity changed for {filename}")


def check_invalid_inputs(
    wrapper: ModuleType,
    wrapper_path: Path,
    executable: Path,
    fixtures: dict[str, dict[str, Any]],
) -> int:
    orientation = fixtures["affine8_orientation_fp64_positive.json"]
    coefficient = fixtures["affine8_orientation_expansion_positive.json"]
    power = fixtures["affine8_orientation_fp64_negative.json"]
    exact = fixtures["affine8_orientation_multiprecision_exact_positive.json"]
    intersection = fixtures["affine8_intersection_fp64_negative.json"]
    incidence = fixtures["affine8_incidence_fp64_negative.json"]
    plane_side = fixtures["affine8_plane_side_fp64_positive.json"]
    variants: list[dict[str, Any]] = []

    unknown = deepcopy(orientation)
    unknown["epsilon"] = 0
    variants.append(unknown)

    missing = deepcopy(orientation)
    del missing["points"]
    variants.append(missing)

    wrong_version = deepcopy(orientation)
    wrong_version["schema_version"] = 7
    variants.append(wrong_version)

    nested_unknown = deepcopy(orientation)
    nested_unknown["plane_source"]["epsilon"] = 0
    variants.append(nested_unknown)

    missing_tag = deepcopy(orientation)
    del missing_tag["plane_source"]["source_kind"]
    variants.append(missing_tag)

    invalid_tag = deepcopy(orientation)
    invalid_tag["plane_source"]["source_kind"] = "raw_bigint"
    variants.append(invalid_tag)

    uppercase = deepcopy(coefficient)
    uppercase["plane_source"]["coefficient_words"][2] = "3FF0000000000000"
    variants.append(uppercase)

    nonfinite = deepcopy(coefficient)
    nonfinite["plane_source"]["coefficient_words"][2] = "7ff0000000000000"
    variants.append(nonfinite)

    zero_normal = deepcopy(coefficient)
    zero_normal["plane_source"]["coefficient_words"] = [
        "0000000000000000",
        "0000000000000000",
        "0000000000000000",
        "3ff0000000000000",
    ]
    variants.append(zero_normal)

    dependent_through = deepcopy(orientation)
    dependent_through["plane_source"]["points"][1] = list(
        dependent_through["plane_source"]["points"][0]
    )
    variants.append(dependent_through)

    constant_power = deepcopy(power)
    constant_power["plane_source"]["q_ids"] = list(
        constant_power["plane_source"]["r_ids"]
    )
    variants.append(constant_power)

    mismatched_power = deepcopy(power)
    mismatched_power["plane_source"]["q_ids"] = [0, 1]
    variants.append(mismatched_power)

    nonprimitive_exact = deepcopy(exact)
    nonprimitive_exact["plane_source"]["plane"]["c"] = "2"
    variants.append(nonprimitive_exact)

    off_plane = deepcopy(orientation)
    off_plane["points"][0][2] = "3ff0000000000000"
    variants.append(off_plane)

    wrong_plane_count = deepcopy(intersection)
    wrong_plane_count["plane_sources"].pop()
    variants.append(wrong_plane_count)

    extra_plane = deepcopy(intersection)
    extra_plane["plane_sources"].append(deepcopy(extra_plane["plane_sources"][0]))
    variants.append(extra_plane)

    nonunique_incidence = deepcopy(incidence)
    repeated = {
        "plane": {
            "a": "1",
            "b": "0",
            "c": "0",
            "d": "0",
            "schema_version": "2.0.0",
        },
        "source_kind": "exact_plane",
    }
    nonunique_incidence["plane_sources"][:3] = [
        deepcopy(repeated),
        deepcopy(repeated),
        deepcopy(repeated),
    ]
    variants.append(nonunique_incidence)

    missing_side_point = deepcopy(plane_side)
    del missing_side_point["point"]
    variants.append(missing_side_point)

    malformed_side_point = deepcopy(plane_side)
    malformed_side_point["point"].append("0000000000000000")
    variants.append(malformed_side_point)

    for index, value in enumerate(variants):
        try:
            wrapper.validate_input(value)
        except (ValueError, TypeError):
            continue
        raise AssertionError(f"invalid closed-schema v8 variant {index} was accepted")

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-v8-invalid-") as temporary:
        invalid_path = Path(temporary) / "unknown-field.json"
        invalid_path.write_text(json.dumps(unknown), encoding="utf-8")
        completed = subprocess.run(
            [
                sys.executable,
                str(wrapper_path),
                str(invalid_path),
                "--executable",
                str(executable),
            ],
            check=False,
            capture_output=True,
            encoding="utf-8",
            timeout=WRAPPER_TIMEOUT_SECONDS,
        )
        if (
            completed.returncode != 2
            or completed.stdout
            or "failed closed" not in completed.stderr
        ):
            raise AssertionError("the wrapper CLI did not reject an unknown v8 field")

        duplicate_path = Path(temporary) / "duplicate-key.json"
        duplicate_text = canonical_json(orientation).replace(
            '"source_kind":"through_points"',
            '"source_kind":"through_points","source_kind":"through_points"',
            1,
        )
        duplicate_path.write_text(duplicate_text, encoding="utf-8")
        duplicate = subprocess.run(
            [
                sys.executable,
                str(wrapper_path),
                str(duplicate_path),
                "--executable",
                str(executable),
            ],
            check=False,
            capture_output=True,
            encoding="utf-8",
            timeout=WRAPPER_TIMEOUT_SECONDS,
        )
        if (
            duplicate.returncode != 2
            or duplicate.stdout
            or "duplicate JSON key" not in duplicate.stderr
        ):
            raise AssertionError("the wrapper CLI accepted a duplicate nested v8 key")
    return len(variants) + 2


def check_invalid_native_commands(
    executable: Path, fixtures: dict[str, dict[str, Any]]
) -> int:
    zero = "0000000000000000"
    one = "3ff0000000000000"
    origin = [zero, zero, zero]
    orientation = case_from_fixture(
        fixtures["affine8_orientation_fp64_positive.json"]
    ).command.split()
    intersection = case_from_fixture(
        fixtures["affine8_intersection_fp64_negative.json"]
    ).command.split()
    incidence = case_from_fixture(
        fixtures["affine8_incidence_fp64_negative.json"]
    ).command.split()
    plane_side = case_from_fixture(
        fixtures["affine8_plane_side_fp64_positive.json"]
    ).command.split()
    exact_x = ["exact", "1", "0", "0", "0"]
    exact_fourth = ["exact", "1", "0", "0", "-1"]
    commands = [
        orientation[:-1],
        [*orientation, "trailing"],
        ["adaptive_orientation_2d_in_plane", "invalid"],
        [
            "adaptive_orientation_2d_in_plane",
            "coeff",
            "7ff0000000000000",
            zero,
            zero,
            zero,
        ],
        ["adaptive_orientation_2d_in_plane", "through", *origin, *origin, *origin],
        ["adaptive_orientation_2d_in_plane", "power", "0"],
        [
            "adaptive_orientation_2d_in_plane",
            "power",
            "1",
            *origin,
            *origin,
        ],
        ["adaptive_orientation_2d_in_plane", "exact", "0", "0", "2", "0"],
        ["adaptive_intersect_three_planes", *exact_x, *exact_x],
        [*intersection, "trailing"],
        [
            "adaptive_fourth_plane_incidence",
            *exact_x,
            *exact_x,
            *exact_x,
            *exact_fourth,
        ],
        [*incidence, "trailing"],
        plane_side[:-1],
        [*plane_side, "trailing"],
        [
            "adaptive_orientation_2d_in_plane",
            "coeff",
            zero,
            zero,
            one,
            zero,
            *origin,
        ],
    ]
    for index, command in enumerate(commands):
        completed = subprocess.run(
            [str(executable), *command],
            check=False,
            capture_output=True,
            encoding="utf-8",
            timeout=WRAPPER_TIMEOUT_SECONDS,
        )
        if (
            completed.returncode != 2
            or completed.stdout
            or "failed closed" not in completed.stderr
        ):
            raise AssertionError(
                f"invalid native v8 command {index} did not fail closed"
            )
    return len(commands)


def check_forged_native_rejected(
    wrapper: ModuleType,
    fixtures: dict[str, dict[str, Any]],
    authentic_results: dict[str, dict[str, Any]],
) -> int:
    forgeries: list[tuple[str, dict[str, Any]]] = []

    orientation_name = "affine8_orientation_fp64_positive.json"
    forged_orientation = deepcopy(authentic_results[orientation_name])
    forged_orientation["orientation_value_exact"]["numerator"] = "2"
    forgeries.append((orientation_name, forged_orientation))

    intersection_name = "affine8_intersection_fp64_negative.json"
    forged_intersection = deepcopy(authentic_results[intersection_name])
    forged_intersection["normal_rank"] = 2
    forgeries.append((intersection_name, forged_intersection))

    incidence_name = "affine8_incidence_fp64_negative.json"
    forged_incidence = deepcopy(authentic_results[incidence_name])
    forged_incidence["signed_value_exact"]["numerator"] = "-2"
    forgeries.append((incidence_name, forged_incidence))

    forged_counter_types = deepcopy(authentic_results[orientation_name])
    forged_counter_types["counters"] = {
        field: bool(count) for field, count in forged_counter_types["counters"].items()
    }
    forgeries.append((orientation_name, forged_counter_types))

    forged_rank_type = deepcopy(authentic_results[intersection_name])
    forged_rank_type["affine_dimension"] = False
    forgeries.append((intersection_name, forged_rank_type))

    plane_side_name = "affine8_plane_side_fp64_positive.json"
    forged_plane_side = deepcopy(authentic_results[plane_side_name])
    forged_plane_side["signed_value_exact"]["numerator"] = "2"
    forgeries.append((plane_side_name, forged_plane_side))

    for name, forged in forgeries:
        with tempfile.TemporaryDirectory(prefix="morsehgp3d-v8-forged-") as temporary:
            fake_native = Path(temporary) / "fake_native.py"
            fake_native.write_text(
                "import sys\n"
                f"sys.stdout.write({(canonical_json(forged) + chr(10))!r})\n",
                encoding="utf-8",
            )
            try:
                wrapper.replay(
                    wrapper.validate_input(fixtures[name]),
                    Path(sys.executable),
                    (str(fake_native),),
                )
            except (ValueError, subprocess.SubprocessError):
                continue
        raise AssertionError(f"the wrapper accepted a forged v8 result for {name}")
    return len(forgeries)


def empty_stage_sign_histogram() -> dict[str, dict[str, int]]:
    return {stage: {sign: 0 for sign in SIGNS} for stage in STAGES}


def rank_class_label(rank_class: tuple[str, int, int, int | None]) -> str:
    kind, normal_rank, augmented_rank, affine_dimension = rank_class
    dimension = "null" if affine_dimension is None else str(affine_dimension)
    return f"{kind}:normal{normal_rank}:augmented{augmented_rank}:dimension{dimension}"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wrapper", type=Path)
    parser.add_argument("native_replay", type=Path)
    parser.add_argument("fixture_directory", type=Path)
    arguments = parser.parse_args()
    wrapper_module = load_wrapper_module(arguments.wrapper)

    fixture_paths = sorted(arguments.fixture_directory.glob("affine8_*.json"))
    if len(fixture_paths) != EXPECTED_FIXTURE_COUNT:
        raise AssertionError(
            f"the v8 fixture set has {len(fixture_paths)} files, expected "
            f"{EXPECTED_FIXTURE_COUNT}"
        )
    fixtures = [(path, load_fixture(path)) for path in fixture_paths]
    for path, fixture in fixtures:
        if wrapper_module.validate_input(fixture) != fixture:
            raise AssertionError(f"the wrapper changed canonical fixture {path.name}")

    replay_ids = {path.name: expected_replay_id(value) for path, value in fixtures}
    manifest = "".join(f"{name}\0{replay_ids[name]}\n" for name in sorted(replay_ids))
    manifest_hash = hashlib.sha256(manifest.encode("utf-8")).hexdigest()
    if manifest_hash != EXPECTED_FIXTURE_MANIFEST_SHA256:
        raise AssertionError(
            "the frozen v8 fixture set or predicate-replay-v8 domain changed"
        )

    stage_sign_histogram = {
        predicate: empty_stage_sign_histogram() for predicate in sorted(PREDICATES)
    }
    fast_provenance_stage_sign_histogram = {
        predicate: empty_stage_sign_histogram() for predicate in sorted(PREDICATES)
    }
    fixture_counts = {predicate: 0 for predicate in sorted(PREDICATES)}
    origin_counts = {origin: 0 for origin in ORIGINS}
    power_cardinality_histogram = {str(size): 0 for size in range(1, 11)}
    oblique_plane_source_count = 0
    zero_rank_class_counts: dict[tuple[str, int, int, int | None], int] = {}
    authentic_results: dict[str, dict[str, Any]] = {}

    for path, fixture in fixtures:
        case = case_from_fixture(fixture)
        if case_sign(case) != expected_fixture_sign(path.name):
            raise AssertionError(f"{path.name} does not encode its exact oracle sign")
        replay_id = replay_ids[path.name]
        normal_envelope = run_wrapper_in_process(
            wrapper_module, arguments.native_replay, fixture
        )
        mp_envelope = run_wrapper_in_process(
            wrapper_module,
            arguments.native_replay,
            fixture,
            ("--multiprecision-only",),
        )
        normal = audit_envelope(normal_envelope, fixture, replay_id)
        multiprecision = audit_envelope(mp_envelope, fixture, replay_id)
        native_normal = run_native(arguments.native_replay, case)
        native_mp = run_native(
            arguments.native_replay, case, ("--multiprecision-only",)
        )
        if normal != native_normal or multiprecision != native_mp:
            raise AssertionError(f"wrapper/native result mismatch for {path.name}")
        if scientific_projection(normal) != case.expected:
            raise AssertionError(f"v8 Fraction oracle mismatch for {path.name}")
        if scientific_projection(normal) != scientific_projection(multiprecision):
            raise AssertionError(
                f"adaptive/MP scientific projection mismatch for {path.name}"
            )
        stage = validate_stage_and_counters(normal, case, multiprecision_only=False)
        validate_stage_and_counters(multiprecision, case, multiprecision_only=True)
        if stage != expected_fixture_stage(path.name):
            raise AssertionError(
                f"{path.name} obtained {stage}, not its declared forced stage"
            )
        sign = case_sign(case)
        stage_sign_histogram[case.predicate][stage][sign] += 1
        if all(origin != "exact" for origin in case.origins):
            fast_provenance_stage_sign_histogram[case.predicate][stage][sign] += 1
        fixture_counts[case.predicate] += 1
        for origin in case.origins:
            origin_counts[origin] += 1
        raw_sources = (
            [fixture["plane_source"]]
            if "plane_source" in fixture
            else fixture["plane_sources"]
        )
        for raw_source in raw_sources:
            resolved_source = source_from_fixture(raw_source)
            if sum(coefficient != 0 for coefficient in resolved_source.plane[:3]) >= 2:
                oblique_plane_source_count += 1
            if raw_source["source_kind"] == "power_bisector":
                cardinality = len(raw_source["r_ids"])
                power_cardinality_histogram[str(cardinality)] += 1
        if case.predicate == "adaptive_intersect_three_planes" and sign == "zero":
            expected = case.expected
            rank_class = (
                str(expected["intersection_kind"]),
                int(expected["normal_rank"]),
                int(expected["augmented_rank"]),
                (
                    None
                    if expected["affine_dimension"] is None
                    else int(expected["affine_dimension"])
                ),
            )
            zero_rank_class_counts[rank_class] = (
                zero_rank_class_counts.get(rank_class, 0) + 1
            )
        authentic_results[path.name] = native_normal

    if fixture_counts != EXPECTED_FIXTURE_COUNTS:
        raise AssertionError(
            f"the v8 fixture partition changed: observed {fixture_counts}"
        )
    missing: list[str] = []
    for predicate in sorted(PREDICATES):
        for sign in ("negative", "positive"):
            if stage_sign_histogram[predicate]["fp64_filtered"][sign] == 0:
                missing.append(f"{predicate}:fp64_filtered:{sign}")
        for stage in ("expansion", "cpu_multiprecision"):
            for sign in SIGNS:
                if stage_sign_histogram[predicate][stage][sign] == 0:
                    missing.append(f"{predicate}:{stage}:{sign}")
    if missing:
        raise AssertionError(
            "the frozen v8 fixtures omit required classes: " + ", ".join(missing)
        )
    missing_fast_multiprecision = [
        f"{predicate}:cpu_multiprecision:{sign}"
        for predicate in sorted(PREDICATES)
        for sign in SIGNS
        if fast_provenance_stage_sign_histogram[predicate]["cpu_multiprecision"][sign]
        == 0
    ]
    if missing_fast_multiprecision:
        raise AssertionError(
            "the frozen v8 fixtures omit binary64-provenance fallbacks: "
            + ", ".join(missing_fast_multiprecision)
        )
    if not EXPECTED_ZERO_RANK_CLASSES.issubset(zero_rank_class_counts):
        omitted = EXPECTED_ZERO_RANK_CLASSES - set(zero_rank_class_counts)
        raise AssertionError(
            f"the v8 intersection fixtures omit rank classes {omitted}"
        )
    if any(origin_counts[origin] == 0 for origin in ORIGINS):
        raise AssertionError("the v8 fixture set does not mix every plane origin")
    if oblique_plane_source_count == 0:
        raise AssertionError("the v8 fixture set has no oblique plane source")
    if sum(power_cardinality_histogram[str(size)] for size in range(2, 11)) == 0:
        raise AssertionError("the v8 fixture set has no multi-label power source")

    cli_probe_path, cli_probe_fixture = fixtures[0]
    for prefix in ((), ("--multiprecision-only",)):
        cli_probe = run_wrapper_cli(
            arguments.wrapper,
            arguments.native_replay,
            cli_probe_path,
            prefix,
        )
        imported_probe = run_wrapper_in_process(
            wrapper_module,
            arguments.native_replay,
            cli_probe_fixture,
            prefix,
        )
        if cli_probe != imported_probe:
            raise AssertionError("the v8 wrapper CLI differs from its imported path")

    fixtures_by_name = {path.name: fixture for path, fixture in fixtures}
    invalid_input_count = check_invalid_inputs(
        wrapper_module,
        arguments.wrapper,
        arguments.native_replay,
        fixtures_by_name,
    )
    invalid_native_command_count = check_invalid_native_commands(
        arguments.native_replay, fixtures_by_name
    )
    forged_result_count = check_forged_native_rejected(
        wrapper_module, fixtures_by_name, authentic_results
    )
    check_historical_sentinels(
        arguments.wrapper, arguments.native_replay, arguments.fixture_directory
    )

    print(
        canonical_json(
            {
                "adaptive_multiprecision_scientific_projection_equal": True,
                "binary64_provenance_stage_sign_histogram": (
                    fast_provenance_stage_sign_histogram
                ),
                "expansion_magnitude": EXPANSION_MAGNITUDE,
                "fixture_count": len(fixtures),
                "fixture_counts_by_predicate": fixture_counts,
                "fixture_manifest_sha256": manifest_hash,
                "fixture_replay_ids": replay_ids,
                "forged_scientific_result_rejection_count": forged_result_count,
                "historical_replay_sentinel_count": len(HISTORICAL_SENTINELS),
                "historical_replay_sentinels_unchanged": True,
                "invalid_closed_schema_case_count": invalid_input_count,
                "invalid_native_command_count": invalid_native_command_count,
                "native_wrapper_results_equal": True,
                "oblique_plane_source_count": oblique_plane_source_count,
                "plane_origin_counts": origin_counts,
                "power_label_cardinality_histogram": {
                    cardinality: count
                    for cardinality, count in power_cardinality_histogram.items()
                    if count != 0
                },
                "predicate_replay_domain": V8_DOMAIN.decode("ascii"),
                "stage_sign_histogram": stage_sign_histogram,
                "zero_determinant_rank_class_counts": {
                    rank_class_label(rank_class): count
                    for rank_class, count in sorted(
                        zero_rank_class_counts.items(), key=lambda item: str(item[0])
                    )
                },
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
