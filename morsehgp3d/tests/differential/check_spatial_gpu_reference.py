#!/usr/bin/env python3
"""Fraction and binary64 audit of the exhaustive CUDA spatial reference."""

from __future__ import annotations

import argparse
from fractions import Fraction
import importlib.util
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from types import ModuleType


PROTOCOL_HEADER = "morsehgp3d-spatial-gpu-reference-v1"
RESULT_SCHEMA = "morsehgp3d.spatial_gpu_reference.v1"
SUMMARY_SCHEMA = "morsehgp3d.phase4.spatial_gpu_differential.v1"
DEFAULT_TIMEOUT_SECONDS = 300
FNV_OFFSET_BASIS = 14_695_981_039_346_656_037
FNV_PRIME = 1_099_511_628_211
UINT64_MASK = (1 << 64) - 1
AUDIT_KEYS = {
    "all_points_enumerated",
    "buffer_epoch",
    "cpu_exact_distance_evaluation_count",
    "cpu_exact_recertification_complete",
    "decision_semantics",
    "gpu_finite_distance_proposal_count",
    "gpu_infinite_distance_proposal_count",
    "gpu_input_point_count",
    "gpu_launch_count",
    "gpu_nan_distance_proposal_count",
    "gpu_output_record_count",
    "gpu_unique_point_id_count",
    "projected_query_bits",
    "proposal_digest_fnv1a",
    "proposal_semantics",
    "query_projection",
}
REQUIRED_IEEE_COVERAGE = {
    "addition_only_overflow",
    "finite_subnormal_distance",
    "max_finite_query",
    "normal_subnormal_tie",
    "overflow_clamped_query",
    "subnormal_tie",
}


def load_lbvh_oracle() -> ModuleType:
    path = Path(__file__).with_name("check_spatial_lbvh.py")
    specification = importlib.util.spec_from_file_location(
        "morsehgp3d_spatial_lbvh_oracle", path
    )
    if specification is None or specification.loader is None:
        raise AssertionError(f"cannot load independent oracle helpers from {path}")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


def gpu_specific_cases(oracle: ModuleType) -> tuple[dict[str, object], ...]:
    fixture_path = (
        Path(__file__).parents[1] / "fixtures" / "spatial" / "gpu_fp64_tie_split.json"
    )
    fixture = oracle.strict_json_object(fixture_path.read_text(encoding="utf-8"))
    expected_fixture_keys = {
        "claim_refuted",
        "expected_exact_cutoff",
        "expected_exact_shell_ids",
        "points_binary64",
        "projected_query_binary64",
        "proposal_squared_distance_binary64",
        "query",
        "requested_rank",
        "schema",
    }
    if set(fixture) != expected_fixture_keys or fixture["schema"] != (
        "morsehgp3d.spatial.gpu_fp64_tie_split.v1"
    ):
        raise AssertionError("the permanent FP64 tie-split fixture has changed schema")
    query_record = fixture["query"]
    point_records = fixture["points_binary64"]
    if not isinstance(query_record, dict) or not isinstance(point_records, list):
        raise AssertionError("the permanent FP64 tie-split fixture is malformed")
    exact_tie_split_by_projection = oracle.make_case(
        case_id=30_001,
        source_points=tuple(tuple(words) for words in point_records),
        query_components=(
            int(query_record["x_numerator"]),
            int(query_record["y_numerator"]),
            int(query_record["z_numerator"]),
            int(query_record["denominator"]),
        ),
        requested_rank=fixture["requested_rank"],
        run_m_star=0,
        exclusion_ids=(),
        label="exact-tie-split-by-rn-even-query-projection",
    )
    exact_expected = oracle.expected_scientific_result(
        exact_tie_split_by_projection
    )["top_k"]
    if (
        exact_expected["cutoff_squared_distance"] != fixture["expected_exact_cutoff"]
        or exact_expected["cutoff_shell_ids"] != fixture["expected_exact_shell_ids"]
    ):
        raise AssertionError("the tie-split fixture no longer matches Fraction")
    proposal = expected_proposal(exact_tie_split_by_projection, oracle)
    point_proposals = [
        f"{proposal_bits(point, projected_query(exact_tie_split_by_projection['query'])[0]):016x}"
        for point in oracle.canonicalize_points(
            exact_tie_split_by_projection["source_points"]
        )
    ]
    if (
        proposal["projection_bits"] != fixture["projected_query_binary64"]
        or point_proposals != fixture["proposal_squared_distance_binary64"]
    ):
        raise AssertionError("the tie-split fixture no longer matches RN-even")
    tiny_denominator = 1 << 1100
    projection_underflow = oracle.make_case(
        case_id=30_002,
        source_points=(
            oracle.point_words(0, 0, 0),
            ("0000000000000001", "0000000000000000", "0000000000000000"),
            oracle.point_words(1, 0, 0),
        ),
        query_components=(1, 0, -1, tiny_denominator),
        requested_rank=2,
        run_m_star=1,
        exclusion_ids=(2,),
        label="positive-and-negative-query-projection-underflow",
    )
    finite_subnormal_distance = oracle.make_case(
        case_id=30_003,
        source_points=(
            oracle.point_words(0, 0, 0),
            ("1e60000000000000", "0000000000000000", "0000000000000000"),
        ),
        query_components=(0, 0, 0, 1),
        requested_rank=1,
        run_m_star=0,
        exclusion_ids=(),
        label="minimum-subnormal-squared-distance-proposal",
    )
    addition_only_overflow = oracle.make_case(
        case_id=30_004,
        source_points=(
            oracle.point_words(0, 0, 0),
            ("5fe7dddf6b095ff1", "5fe7dddf6b095ff1", "0000000000000000"),
        ),
        query_components=(0, 0, 0, 1),
        requested_rank=1,
        run_m_star=0,
        exclusion_ids=(),
        label="finite-squares-overflow-only-on-addition",
    )
    midpoint_denominator = 1 << 1075
    projection_midpoint_boundaries = oracle.make_case(
        case_id=30_005,
        source_points=(
            oracle.point_words(0, 0, 0),
            oracle.point_words(1, 1, 0),
        ),
        query_components=(3, (1 << 53) - 1, 0, midpoint_denominator),
        requested_rank=1,
        run_m_star=0,
        exclusion_ids=(),
        label="subnormal-and-normal-boundary-rn-even-ties",
    )
    maximum_finite_integer = ((1 << 53) - 1) << 971
    maximum_finite_query = oracle.make_case(
        case_id=30_006,
        source_points=(
            oracle.point_words(0, 0, 0),
            oracle.point_words(1, 0, 0),
        ),
        query_components=(
            maximum_finite_integer,
            -maximum_finite_integer,
            0,
            1,
        ),
        requested_rank=1,
        run_m_star=0,
        exclusion_ids=(),
        label="positive-and-negative-maximum-finite-query",
    )
    overflow_clamped_query = oracle.make_case(
        case_id=30_007,
        source_points=(
            oracle.point_words(-1, 0, 0),
            oracle.point_words(1, 0, 0),
        ),
        query_components=(1 << 1024, -(1 << 1024), 0, 1),
        requested_rank=1,
        run_m_star=0,
        exclusion_ids=(),
        label="rational-query-overflow-clamped-only-for-proposal",
    )
    boundary_projection = expected_proposal(projection_midpoint_boundaries, oracle)
    if boundary_projection["projection_bits"] != [
        "0000000000000002",
        "0010000000000000",
        "0000000000000000",
    ]:
        raise AssertionError("RN-even boundary projection coverage has drifted")
    maximum_projection = expected_proposal(maximum_finite_query, oracle)
    if maximum_projection["projection_bits"] != [
        "7fefffffffffffff",
        "ffefffffffffffff",
        "0000000000000000",
    ]:
        raise AssertionError("maximum-finite query projection coverage has drifted")
    overflow_projection = expected_proposal(overflow_clamped_query, oracle)
    if (
        overflow_projection["projection_bits"]
        != ["7fefffffffffffff", "ffefffffffffffff", "0000000000000000"]
        or overflow_projection["projection_statuses"]
        != ["overflow_clamped", "overflow_clamped", "exact"]
    ):
        raise AssertionError("overflow-clamped projection coverage has drifted")
    return (
        exact_tie_split_by_projection,
        projection_underflow,
        finite_subnormal_distance,
        addition_only_overflow,
        projection_midpoint_boundaries,
        maximum_finite_query,
        overflow_clamped_query,
    )


def projected_query(
    query_components: tuple[int, int, int, int]
) -> tuple[tuple[float, float, float], list[str], list[str]]:
    x_numerator, y_numerator, z_numerator, denominator = query_components
    exact_coordinates = (
        Fraction(x_numerator, denominator),
        Fraction(y_numerator, denominator),
        Fraction(z_numerator, denominator),
    )
    maximum_finite = Fraction.from_float(sys.float_info.max)
    projected_values: list[float] = []
    overflow_clamped: list[bool] = []
    for value in exact_coordinates:
        if abs(value) > maximum_finite:
            projected_values.append(
                math.copysign(sys.float_info.max, -1.0 if value < 0 else 1.0)
            )
            overflow_clamped.append(True)
        else:
            projected_values.append(float(value))
            overflow_clamped.append(False)
    projected = tuple(projected_values)
    bits = [struct.pack(">d", value).hex() for value in projected]
    statuses: list[str] = []
    for exact, approximate, clamped in zip(
        exact_coordinates, projected, overflow_clamped, strict=True
    ):
        if clamped:
            statuses.append("overflow_clamped")
        elif Fraction.from_float(approximate) == exact:
            statuses.append("exact")
        elif approximate == 0.0:
            statuses.append("underflow")
        else:
            statuses.append("rounded")
    return projected, bits, statuses


def proposal_recipe(
    point: tuple[Fraction, Fraction, Fraction],
    query: tuple[float, float, float],
) -> tuple[int, set[str]]:
    squared_distance = 0.0
    coverage: set[str] = set()
    for axis in range(3):
        difference = float(point[axis]) - query[axis]
        square = difference * difference
        accumulated = squared_distance + square
        if (
            math.isfinite(squared_distance)
            and math.isfinite(square)
            and math.isinf(accumulated)
        ):
            coverage.add("addition_only_overflow")
        squared_distance = accumulated
    bits = int.from_bytes(struct.pack(">d", squared_distance), "big")
    if bits != 0 and bits & 0x7FF0_0000_0000_0000 == 0:
        coverage.add("finite_subnormal_distance")
    return bits, coverage


def proposal_bits(
    point: tuple[Fraction, Fraction, Fraction],
    query: tuple[float, float, float],
) -> int:
    return proposal_recipe(point, query)[0]


def fnv1a_proposal_digest(words: list[tuple[int, int]]) -> str:
    digest = FNV_OFFSET_BASIS
    for point_id, distance_bits in words:
        for word in (point_id, distance_bits):
            for shift in range(0, 64, 8):
                digest ^= (word >> shift) & 0xFF
                digest = (digest * FNV_PRIME) & UINT64_MASK
    return f"{digest:016x}"


def expected_proposal(case: dict[str, object], oracle: ModuleType) -> dict[str, object]:
    source_points = case["source_points"]
    query_components = case["query"]
    if not isinstance(source_points, tuple) or not isinstance(query_components, tuple):
        raise AssertionError("invalid generated GPU-reference case")
    points = oracle.canonicalize_points(source_points)
    query, query_bits, statuses = projected_query(query_components)
    recipes = [proposal_recipe(point, query) for point in points]
    records = [
        (point_id, recipe[0]) for point_id, recipe in enumerate(recipes)
    ]
    ieee_coverage = set().union(*(recipe[1] for recipe in recipes))
    if case["case_id"] == 30_005:
        ieee_coverage.update({"normal_subnormal_tie", "subnormal_tie"})
    if case["case_id"] == 30_006:
        ieee_coverage.add("max_finite_query")
    if case["case_id"] == 30_007:
        ieee_coverage.add("overflow_clamped_query")
    infinity_bits = 0x7FF0_0000_0000_0000
    infinite_count = sum(bits == infinity_bits for _, bits in records)
    return {
        "digest": fnv1a_proposal_digest(records),
        "finite_count": len(records) - infinite_count,
        "infinite_count": infinite_count,
        "ieee_coverage": ieee_coverage,
        "projection_bits": query_bits,
        "projection_statuses": statuses,
    }


def require_audit(
    value: object,
    *,
    case: dict[str, object],
    oracle: ModuleType,
    exact_count: int,
    epoch: int,
) -> None:
    if not isinstance(value, dict) or set(value) != AUDIT_KEYS:
        raise AssertionError("proposal audit has an unexpected schema")
    integer_keys = {
        "buffer_epoch",
        "cpu_exact_distance_evaluation_count",
        "gpu_finite_distance_proposal_count",
        "gpu_infinite_distance_proposal_count",
        "gpu_input_point_count",
        "gpu_launch_count",
        "gpu_nan_distance_proposal_count",
        "gpu_output_record_count",
        "gpu_unique_point_id_count",
    }
    for key in integer_keys:
        if type(value[key]) is not int or value[key] < 0:
            raise AssertionError(f"proposal audit {key} is not a nonnegative integer")
    for key in ("all_points_enumerated", "cpu_exact_recertification_complete"):
        if type(value[key]) is not bool:
            raise AssertionError(f"proposal audit {key} is not a boolean")
    for key in ("decision_semantics", "proposal_semantics"):
        if type(value[key]) is not str:
            raise AssertionError(f"proposal audit {key} is not a string")
    if (
        type(value["proposal_digest_fnv1a"]) is not str
        or len(value["proposal_digest_fnv1a"]) != 16
        or any(character not in "0123456789abcdef" for character in value["proposal_digest_fnv1a"])
    ):
        raise AssertionError("proposal audit digest is not one lowercase uint64 word")
    projected_bits = value["projected_query_bits"]
    if (
        type(projected_bits) is not list
        or len(projected_bits) != 3
        or any(
            type(word) is not str
            or len(word) != 16
            or any(character not in "0123456789abcdef" for character in word)
            for word in projected_bits
        )
    ):
        raise AssertionError("proposal audit projected-query words are malformed")
    projection = value["query_projection"]
    if (
        type(projection) is not list
        or len(projection) != 3
        or any(
            status not in {"exact", "rounded", "underflow", "overflow_clamped"}
            for status in projection
        )
    ):
        raise AssertionError("proposal audit query-projection statuses are malformed")
    point_count = len(case["source_points"])  # type: ignore[arg-type]
    proposal = expected_proposal(case, oracle)
    expected_scalars = {
        "all_points_enumerated": True,
        "buffer_epoch": epoch,
        "cpu_exact_distance_evaluation_count": exact_count,
        "cpu_exact_recertification_complete": True,
        "decision_semantics": "cpu_exact_all_points",
        "gpu_finite_distance_proposal_count": proposal["finite_count"],
        "gpu_infinite_distance_proposal_count": proposal["infinite_count"],
        "gpu_input_point_count": point_count,
        "gpu_launch_count": 1,
        "gpu_nan_distance_proposal_count": 0,
        "gpu_output_record_count": point_count,
        "gpu_unique_point_id_count": point_count,
        "projected_query_bits": proposal["projection_bits"],
        "proposal_digest_fnv1a": proposal["digest"],
        "proposal_semantics": "non_certifying_fp64",
        "query_projection": proposal["projection_statuses"],
    }
    difference = oracle.first_difference(expected_scalars, value)
    if difference is not None:
        raise AssertionError(f"proposal audit mismatch: {difference}")


def validate_case_result(
    actual: dict[str, object],
    case: dict[str, object],
    oracle: ModuleType,
) -> set[str]:
    expected_keys = {
        "case_id",
        "closed_ball",
        "closed_ball_audit",
        "schema",
        "top_k",
        "top_k_audit",
    }
    if set(actual) != expected_keys:
        raise AssertionError("GPU-reference result keys differ")
    if actual["schema"] != RESULT_SCHEMA or actual["case_id"] != case["case_id"]:
        raise AssertionError("GPU-reference result identity differs")
    expected_scientific = oracle.expected_scientific_result(case)
    actual_scientific = {
        "closed_ball": actual["closed_ball"],
        "top_k": actual["top_k"],
    }
    if actual_scientific != expected_scientific:
        difference = oracle.first_difference(expected_scientific, actual_scientific)
        raise AssertionError(f"scientific projection mismatch: {difference}")

    source_points = case["source_points"]
    exclusions = case["exclusion_ids"]
    if not isinstance(source_points, tuple) or not isinstance(exclusions, tuple):
        raise AssertionError("invalid generated case cardinalities")
    require_audit(
        actual["top_k_audit"],
        case=case,
        oracle=oracle,
        exact_count=len(source_points) - len(exclusions),
        epoch=1,
    )
    require_audit(
        actual["closed_ball_audit"],
        case=case,
        oracle=oracle,
        exact_count=len(source_points),
        epoch=2,
    )
    _, _, statuses = projected_query(case["query"])  # type: ignore[arg-type]
    return set(statuses)


def write_summary(path: Path, value: dict[str, object]) -> None:
    if not path.is_absolute():
        raise ValueError("--summary-json must be an absolute path")
    parent = path.parent.resolve(strict=True)
    target = parent / path.name
    if not path.name or path.name in {".", ".."}:
        raise ValueError("--summary-json has an invalid file name")
    if target.exists() or target.is_symlink():
        raise ValueError(f"refusing to replace existing summary: {target}")

    encoded = (
        json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":"))
        + "\n"
    ).encode("utf-8")
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{target.name}.", suffix=".partial", dir=parent
    )
    temporary = Path(temporary_name)
    try:
        offset = 0
        while offset < len(encoded):
            offset += os.write(descriptor, encoded[offset:])
        os.fsync(descriptor)
        os.close(descriptor)
        descriptor = -1
        os.link(temporary, target, follow_symlinks=False)
        os.unlink(temporary)
        directory = os.open(parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        if descriptor >= 0:
            os.close(descriptor)
        temporary.unlink(missing_ok=True)


def run_differential(
    executable: Path, timeout_seconds: int, *, quick: bool
) -> dict[str, object]:
    oracle = load_lbvh_oracle()
    quick_campaign_sizes = (1, 2, 3, 4, 17, 257, 1000)
    if quick:
        cases = (
            tuple(oracle.target_cases())
            + tuple(oracle.campaign_case(size) for size in quick_campaign_sizes)
            + gpu_specific_cases(oracle)
        )
        expected_sizes = set(quick_campaign_sizes)
    else:
        cases = tuple(oracle.all_cases()) + gpu_specific_cases(oracle)
        expected_sizes = set(
            range(oracle.CAMPAIGN_MIN_SIZE, oracle.CAMPAIGN_MAX_SIZE + 1)
        )
    payload = "\n".join(
        [PROTOCOL_HEADER, *(oracle.case_protocol_line(case) for case in cases), "end", ""]
    )
    completed = subprocess.run(
        [str(executable)],
        input=payload,
        text=True,
        capture_output=True,
        timeout=timeout_seconds,
        check=False,
    )
    if completed.returncode != 0:
        raise AssertionError(
            f"GPU-reference replay failed with {completed.returncode}: "
            f"{completed.stderr.strip()}"
        )
    if completed.stderr:
        raise AssertionError(f"GPU-reference replay wrote stderr: {completed.stderr!r}")
    output_lines = completed.stdout.splitlines()
    if len(output_lines) != len(cases):
        raise AssertionError(
            f"expected {len(cases)} GPU-reference results, got {len(output_lines)}"
        )

    projection_coverage: set[str] = set()
    checked_sizes: set[int] = set()
    ieee_coverage: set[str] = set()
    proposal_infinity_covered = False
    for line, case in zip(output_lines, cases, strict=True):
        actual = oracle.strict_json_object(line)
        if line != oracle.canonical_json(actual):
            raise AssertionError(f"case {case['case_id']} output is not canonical JSON")
        try:
            projection_coverage.update(validate_case_result(actual, case, oracle))
        except AssertionError as error:
            raise AssertionError(
                f"case {case['case_id']} ({case['label']}): {error}"
            ) from error
        case_id = case["case_id"]
        top_audit = actual["top_k_audit"]
        if not isinstance(top_audit, dict):
            raise AssertionError("top-k audit is not an object")
        proposal_infinity_covered |= (
            top_audit["gpu_infinite_distance_proposal_count"] != 0
        )
        ieee_coverage.update(expected_proposal(case, oracle)["ieee_coverage"])
        if isinstance(case_id, int) and case_id > oracle.CAMPAIGN_CASE_ID_BASE:
            size = case_id - oracle.CAMPAIGN_CASE_ID_BASE
            if oracle.CAMPAIGN_MIN_SIZE <= size <= oracle.CAMPAIGN_MAX_SIZE:
                checked_sizes.add(size)

    if checked_sizes != expected_sizes:
        raise AssertionError("the GPU campaign did not cover every size 1..1000")
    if projection_coverage != {
        "exact",
        "overflow_clamped",
        "rounded",
        "underflow",
    }:
        raise AssertionError(
            f"query projection coverage differs: {sorted(projection_coverage)}"
        )
    if not proposal_infinity_covered:
        raise AssertionError("the GPU campaign did not exercise proposal overflow")
    if ieee_coverage != REQUIRED_IEEE_COVERAGE:
        raise AssertionError(
            f"IEEE-754 coverage differs: {sorted(ieee_coverage)}"
        )
    print(
        "spatial CUDA proposal/CPU exact differential: "
        f"{len(cases)} Fraction cases passed; "
        f"scope={'quick' if quick else 'full'}, "
        f"campaign_sizes={len(checked_sizes)} and "
        "exact/rounded/underflow/overflow-clamped projections covered"
    )
    return {
        "all_cases_passed": True,
        "campaign_complete": not quick,
        "campaign_sizes_checked": sorted(checked_sizes),
        "case_count": len(cases),
        "closed_ball_query_count": len(cases),
        "cpu_exact_recertification_complete": True,
        "decision_semantics": "cpu_exact_all_points",
        "gpu_launch_count": 2 * len(cases),
        "ieee_coverage": sorted(ieee_coverage),
        "projection_coverage": sorted(projection_coverage),
        "proposal_semantics": "non_certifying_fp64",
        "schema": SUMMARY_SCHEMA,
        "scope": "quick" if quick else "full",
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "top_k_query_count": len(cases),
    }


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT_SECONDS)
    parser.add_argument("--summary-json", type=Path)
    parser.add_argument(
        "--quick",
        action="store_true",
        help="run the bounded sanitizer subset instead of the full sizes 1..1000",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.timeout <= 0:
        raise ValueError("--timeout must be positive")
    summary = run_differential(
        arguments.executable, arguments.timeout, quick=arguments.quick
    )
    if arguments.summary_json is not None:
        write_summary(arguments.summary_json, summary)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(f"check_spatial_gpu_reference: {error}", file=sys.stderr)
        raise SystemExit(2) from error
