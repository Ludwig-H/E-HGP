#!/usr/bin/env python3
"""Audit frozen v7 support fixtures through both wrapper and native replay."""

from __future__ import annotations

import argparse
from copy import deepcopy
import hashlib
import importlib.util
import json
import subprocess
import sys
import tempfile
from types import ModuleType
from pathlib import Path
from typing import Any, Sequence

from check_support_adaptive_random import (
    Case,
    canonical_json,
    decision_signs,
    make_analysis_case,
    make_barycentric_case,
    make_level_case,
    make_sphere_case,
    scientific_projection,
    validate_stage_and_counters,
)

INPUT_KIND = "morsehgp3d_predicate_replay_input"
RESULT_KIND = "morsehgp3d_predicate_replay_result"
V7_DOMAIN = b"MorseHGP3D/predicate-replay-v7/"
EXPECTED_FIXTURE_COUNT = 30
EXPECTED_FIXTURE_MANIFEST_SHA256 = (
    "9c4a399b7cc81da3274407e38c84a53041a1e335a34f32d741deb13bcb3fb8fc"
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
}
STAGES = ("fp64_filtered", "expansion", "cpu_multiprecision")
SIGNS = ("negative", "zero", "positive")
WRAPPER_TIMEOUT_SECONDS = 40


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
    value = strict_json_loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise AssertionError(f"{path.name} is not a JSON object")
    if (
        value.get("kind") != INPUT_KIND
        or value.get("schema_version") != 7
        or not isinstance(value.get("predicate"), str)
    ):
        raise AssertionError(f"{path.name} is not a closed v7 replay input")
    return value


def expected_replay_id(value: dict[str, Any]) -> str:
    return hashlib.sha256(V7_DOMAIN + canonical_json(value).encode("utf-8")).hexdigest()


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
    value = json.loads(completed.stdout)
    if not isinstance(value, dict) or completed.stdout != canonical_json(value) + "\n":
        raise AssertionError(f"wrapper output is not canonical for {fixture.name}")
    return value


def load_wrapper_module(wrapper: Path) -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "morsehgp3d_v7_replay_wrapper", wrapper
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
        raise AssertionError("the v7 wrapper changed a canonical fixture input")
    replay = wrapper.replay(normalized, executable, tuple(prefix))
    if not isinstance(replay, dict):
        raise AssertionError("the imported v7 wrapper did not return an envelope")
    return replay


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
    value = json.loads(completed.stdout)
    if not isinstance(value, dict) or completed.stdout != canonical_json(value) + "\n":
        raise AssertionError(f"native output is not canonical for {case.predicate}")
    return value


def word_points(value: object) -> list[tuple[str, str, str]]:
    if not isinstance(value, list):
        raise AssertionError("a v7 support is not an array")
    points: list[tuple[str, str, str]] = []
    for point in value:
        if not isinstance(point, list) or len(point) != 3:
            raise AssertionError("a v7 point does not contain three words")
        points.append((str(point[0]), str(point[1]), str(point[2])))
    return points


def word_point(value: object) -> tuple[str, str, str]:
    points = word_points([value])
    return points[0]


def case_from_fixture(value: dict[str, Any]) -> Case:
    predicate = value["predicate"]
    if predicate == "binary64_barycentric_coordinates":
        return make_barycentric_case(
            word_points(value["support"]), word_point(value["query"])
        )
    if predicate == "binary64_circumcenter_support_analysis":
        return make_analysis_case(word_points(value["points"]))
    if predicate == "support_sphere_side":
        return make_sphere_case(
            word_points(value["support"]), word_point(value["point"])
        )
    if predicate == "compare_support_levels":
        return make_level_case(
            word_points(value["left_support"]),
            word_points(value["right_support"]),
        )
    raise AssertionError(f"unsupported adaptive fixture predicate {predicate}")


def expected_fixture_stage(name: str) -> str:
    if "_dependent_" in name:
        return "none"
    if "_fp64_" in name:
        return "fp64_filtered"
    if "_expansion_" in name:
        return "expansion"
    if "_multiprecision_" in name:
        return "cpu_multiprecision"
    raise AssertionError(f"fixture {name} does not declare its forced stage")


def audit_envelope(
    envelope: dict[str, Any], fixture: dict[str, Any], replay_id: str
) -> dict[str, Any]:
    if set(envelope) != {"input", "kind", "replay_id", "result", "schema_version"}:
        raise AssertionError("the v7 replay envelope is not closed")
    if (
        envelope["input"] != fixture
        or envelope["kind"] != RESULT_KIND
        or envelope["schema_version"] != 7
        or envelope["replay_id"] != replay_id
        or not isinstance(envelope["result"], dict)
    ):
        raise AssertionError(
            "the v7 replay envelope or domain-separated hash is invalid"
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
    base = fixtures["adaptive_barycentric_fp64_exterior.json"]
    variants: list[dict[str, Any]] = []

    unknown = deepcopy(base)
    unknown["epsilon"] = 0
    variants.append(unknown)

    missing = deepcopy(base)
    del missing["query"]
    variants.append(missing)

    uppercase_word = deepcopy(base)
    uppercase_word["query"][0] = "Bff0000000000000"
    variants.append(uppercase_word)

    nonfinite = deepcopy(base)
    nonfinite["query"][0] = "7ff0000000000000"
    variants.append(nonfinite)

    dependent_barycentric = deepcopy(base)
    dependent_barycentric["support"][1] = list(dependent_barycentric["support"][0])
    variants.append(dependent_barycentric)

    off_hull = deepcopy(base)
    off_hull["query"] = [
        "0000000000000000",
        "3ff0000000000000",
        "0000000000000000",
    ]
    variants.append(off_hull)

    dependent_sphere = deepcopy(fixtures["adaptive_sphere_fp64_inside.json"])
    dependent_sphere["support"][1] = list(dependent_sphere["support"][0])
    variants.append(dependent_sphere)

    dependent_level = deepcopy(fixtures["adaptive_level_fp64_less.json"])
    dependent_level["left_support"][1] = list(dependent_level["left_support"][0])
    variants.append(dependent_level)

    wrong_version = deepcopy(base)
    wrong_version["schema_version"] = 6
    variants.append(wrong_version)

    for index, value in enumerate(variants):
        try:
            wrapper.validate_input(value)
        except (ValueError, TypeError):
            continue
        raise AssertionError(f"invalid closed-schema v7 variant {index} was accepted")

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-v7-invalid-") as temporary:
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
            raise AssertionError("the wrapper CLI did not reject an unknown v7 field")

        duplicate_path = Path(temporary) / "duplicate-key.json"
        duplicate_text = canonical_json(base).replace(
            '"schema_version":7',
            '"schema_version":7,"schema_version":7',
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
            raise AssertionError("the wrapper CLI accepted a duplicate v7 JSON key")
    return len(variants) + 1


def check_invalid_native_commands(executable: Path) -> int:
    zero = "0000000000000000"
    origin = [zero, zero, zero]
    commands = [
        ["binary64_barycentric_coordinates", "0"],
        ["binary64_barycentric_coordinates", "5"],
        ["binary64_barycentric_coordinates", "1", *origin],
        [
            "binary64_circumcenter_support_analysis",
            "1",
            "7ff0000000000000",
            zero,
            zero,
        ],
        ["support_sphere_side", "2", *origin, *origin, *origin],
        ["compare_support_levels", "2", *origin, *origin, "1", *origin],
        ["compare_support_levels", "1", *origin, "1", *origin, "trailing"],
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
                f"invalid native v7 command {index} did not fail closed"
            )
    return len(commands)


def check_forged_native_rejected(
    wrapper: ModuleType,
    fixture: dict[str, Any],
    authentic_result: dict[str, Any],
) -> None:
    forged = deepcopy(authentic_result)
    forged["classification"] = "outside"
    forged["sign"] = "positive"
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-v7-forged-") as temporary:
        fake_native = Path(temporary) / "fake_native.py"
        fake_native.write_text(
            "import sys\n"
            f"sys.stdout.write({(canonical_json(forged) + chr(10))!r})\n",
            encoding="utf-8",
        )
        try:
            wrapper.replay(
                wrapper.validate_input(fixture),
                Path(sys.executable),
                (str(fake_native),),
            )
        except (ValueError, subprocess.SubprocessError):
            return
    raise AssertionError(
        "the wrapper accepted a scientifically forged native v7 result"
    )


def empty_stage_sign_histogram() -> dict[str, dict[str, int]]:
    return {stage: {sign: 0 for sign in SIGNS} for stage in (*STAGES, "none")}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wrapper", type=Path)
    parser.add_argument("native_replay", type=Path)
    parser.add_argument("fixture_directory", type=Path)
    arguments = parser.parse_args()
    wrapper_module = load_wrapper_module(arguments.wrapper)

    fixture_paths = sorted(arguments.fixture_directory.glob("adaptive_*.json"))
    if len(fixture_paths) != EXPECTED_FIXTURE_COUNT:
        raise AssertionError(
            f"the v7 fixture set has {len(fixture_paths)} files, expected "
            f"{EXPECTED_FIXTURE_COUNT}"
        )
    fixtures = [(path, load_fixture(path)) for path in fixture_paths]
    replay_ids = {path.name: expected_replay_id(value) for path, value in fixtures}
    manifest = "".join(f"{name}\0{replay_ids[name]}\n" for name in sorted(replay_ids))
    manifest_hash = hashlib.sha256(manifest.encode("utf-8")).hexdigest()
    if manifest_hash != EXPECTED_FIXTURE_MANIFEST_SHA256:
        raise AssertionError(
            "the frozen v7 fixture set or predicate-replay-v7 domain changed"
        )

    predicates = sorted({value["predicate"] for _, value in fixtures})
    stage_sign_histogram = {
        predicate: empty_stage_sign_histogram() for predicate in predicates
    }
    fixture_counts = {predicate: 0 for predicate in predicates}
    dependent_count = 0
    for path, fixture in fixtures:
        case = case_from_fixture(fixture)
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
            raise AssertionError(f"v7 Fraction oracle mismatch for {path.name}")
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
        signs = decision_signs(case)
        for sign in signs:
            stage_sign_histogram[case.predicate][stage][sign] += 1
        if stage == "none":
            dependent_count += 1
        fixture_counts[case.predicate] += 1

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
            raise AssertionError("the v7 wrapper CLI differs from its imported path")

    missing: list[str] = []
    for predicate in predicates:
        for sign in ("negative", "positive"):
            if stage_sign_histogram[predicate]["fp64_filtered"][sign] == 0:
                missing.append(f"{predicate}:fp64_filtered:{sign}")
        for stage in ("expansion", "cpu_multiprecision"):
            for sign in SIGNS:
                if stage_sign_histogram[predicate][stage][sign] == 0:
                    missing.append(f"{predicate}:{stage}:{sign}")
    if missing:
        raise AssertionError(
            "the frozen v7 fixtures omit required available classes: "
            + ", ".join(missing)
        )
    if dependent_count != 1:
        raise AssertionError("the v7 fixture set must contain one dependent analysis")

    fixtures_by_name = {path.name: fixture for path, fixture in fixtures}
    invalid_input_count = check_invalid_inputs(
        wrapper_module,
        arguments.wrapper,
        arguments.native_replay,
        fixtures_by_name,
    )
    invalid_native_command_count = check_invalid_native_commands(
        arguments.native_replay
    )
    forged_fixture = fixtures_by_name["adaptive_sphere_fp64_inside.json"]
    forged_case = case_from_fixture(forged_fixture)
    check_forged_native_rejected(
        wrapper_module,
        forged_fixture,
        run_native(arguments.native_replay, forged_case),
    )

    check_historical_sentinels(
        arguments.wrapper, arguments.native_replay, arguments.fixture_directory
    )
    print(
        canonical_json(
            {
                "adaptive_multiprecision_scientific_projection_equal": True,
                "fixture_count": len(fixtures),
                "fixture_counts_by_predicate": fixture_counts,
                "fixture_manifest_sha256": manifest_hash,
                "historical_replay_sentinel_count": len(HISTORICAL_SENTINELS),
                "historical_replay_sentinels_unchanged": True,
                "invalid_closed_schema_case_count": invalid_input_count,
                "invalid_native_command_count": invalid_native_command_count,
                "native_wrapper_results_equal": True,
                "predicate_replay_domain": V7_DOMAIN.decode("ascii"),
                "scientifically_forged_native_rejected": True,
                "stage_sign_histogram": stage_sign_histogram,
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
