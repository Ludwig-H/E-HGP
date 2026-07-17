#!/usr/bin/env python3
"""Validate the first Phase 2B GPU predicate increment without a CUDA device."""

from __future__ import annotations

import copy
import json
import re
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable


PUBLIC_HEADER = Path("include/morsehgp3d/gpu/predicate_filter.hpp")
INTERVAL_HEADER = Path("src/cuda/phase2b_interval.cuh")
INTERNAL_HEADER = Path("src/cuda/phase2b_distance_filter_internal.hpp")
CUDA_SOURCE = Path("src/cuda/phase2b_distance_filter.cu")
ORIENTATION_INTERNAL_HEADER = Path(
    "src/cuda/phase2b_orientation_filter_internal.hpp"
)
ORIENTATION_CUDA_SOURCE = Path("src/cuda/phase2b_orientation_filter.cu")
POWER_INTERNAL_HEADER = Path(
    "src/cuda/phase2b_power_bisector_filter_internal.hpp"
)
POWER_CUDA_SOURCE = Path("src/cuda/phase2b_power_bisector_filter.cu")
HOST_SOURCE = Path("src/gpu/predicate_filter.cpp")
CLI_SOURCE = Path("src/tools/gpu_predicate_replay.cpp")
CMAKE_SOURCE = Path("CMakeLists.txt")
CUDA_MODULE = Path("cmake/MorseHGP3DCuda.cmake")
PRESETS_SOURCE = Path("CMakePresets.json")

LIBRARY_TARGET = "morsehgp3d_gpu_predicates"
REPLAY_TARGET = "morsehgp3d_gpu_predicate_replay"
CUDA_PRESETS = ("cuda-release", "cuda-audit")
DIRECTED_INTRINSICS = (
    "__dsub_rd",
    "__dsub_ru",
    "__dmul_rd",
    "__dmul_ru",
    "__dadd_rd",
    "__dadd_ru",
)


class ContractError(ValueError):
    """A Phase 2B predicate invariant was violated."""


@dataclass
class ContractFiles:
    texts: dict[Path, str]
    presets: dict[str, Any]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        raise ContractError(f"cannot read {path}: {error}") from error


def strict_json(path: Path) -> dict[str, Any]:
    def reject_duplicates(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise ContractError(f"duplicate JSON key in {path}: {key}")
            result[key] = value
        return result

    try:
        value = json.loads(
            path.read_text(encoding="utf-8"), object_pairs_hook=reject_duplicates
        )
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise ContractError(f"cannot read {path}: {error}") from error
    require(isinstance(value, dict), f"{path} must contain a JSON object")
    return value


def load_contract(project: Path) -> ContractFiles:
    source_paths = (
        PUBLIC_HEADER,
        INTERVAL_HEADER,
        INTERNAL_HEADER,
        CUDA_SOURCE,
        ORIENTATION_INTERNAL_HEADER,
        ORIENTATION_CUDA_SOURCE,
        POWER_INTERNAL_HEADER,
        POWER_CUDA_SOURCE,
        HOST_SOURCE,
        CLI_SOURCE,
        CMAKE_SOURCE,
        CUDA_MODULE,
    )
    return ContractFiles(
        texts={path: read_text(project / path) for path in source_paths},
        presets=strict_json(project / PRESETS_SOURCE),
    )


def without_comments(text: str) -> str:
    return re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.DOTALL)


def struct_bodies(text: str) -> list[tuple[str, str]]:
    clean = without_comments(text)
    result: list[tuple[str, str]] = []
    for match in re.finditer(r"\bstruct\s+([A-Za-z_]\w*)[^;{]*\{", clean):
        depth = 1
        cursor = match.end()
        while cursor < len(clean) and depth > 0:
            if clean[cursor] == "{":
                depth += 1
            elif clean[cursor] == "}":
                depth -= 1
            cursor += 1
        if depth == 0:
            result.append((match.group(1), clean[match.end() : cursor - 1]))
    return result


def function_body(text: str, name: str) -> str:
    clean = without_comments(text)
    match = re.search(
        rf"\b{re.escape(name)}\s*\([^;{{}}]*\)\s*(?:noexcept\s*)?\{{",
        clean,
        flags=re.DOTALL,
    )
    require(match is not None, f"missing function body: {name}")
    depth = 1
    cursor = match.end()
    while cursor < len(clean) and depth > 0:
        if clean[cursor] == "{":
            depth += 1
        elif clean[cursor] == "}":
            depth -= 1
        cursor += 1
    require(depth == 0, f"unterminated function body: {name}")
    return clean[match.end() : cursor - 1]


def enum_members(body: str) -> dict[str, int]:
    result: dict[str, int] = {}
    for raw_member in body.split(","):
        member = raw_member.strip()
        if not member:
            continue
        match = re.fullmatch(
            r"(?P<name>[A-Za-z_]\w*)\s*=\s*(?P<value>[+-]?\d+)(?:[uUlL]*)",
            member,
        )
        require(match is not None, f"predicate sign member is not explicit: {member}")
        name = match.group("name")
        require(name not in result, f"duplicate predicate sign member: {name}")
        result[name] = int(match.group("value"))
    return result


def validate_sign_enum(header: str) -> None:
    candidates = re.findall(
        r"enum\s+class\s+([A-Za-z_]\w*)"
        r"(?:\s*:\s*[^\{]+)?\s*\{(?P<body>[^}]*)\}\s*;",
        without_comments(header),
        flags=re.DOTALL,
    )
    selected: tuple[str, str] | None = None
    for name, body in candidates:
        if re.search(r"\bunknown\b", body):
            selected = (name, body)
            break
    require(selected is not None, "the public header has no predicate sign enum")
    enum_name, body = selected
    members = enum_members(body)
    require(
        set(members) == {"unknown", "negative", "positive"},
        f"{enum_name} must contain exactly unknown, negative and positive",
    )
    require(members["unknown"] == 0, f"{enum_name} unknown must equal zero")
    require(
        len(set(members.values())) == 3,
        f"{enum_name} values must be pairwise distinct",
    )
    require(
        members["negative"] < 0 < members["positive"],
        f"{enum_name} known signs must straddle zero",
    )
    require(
        not re.search(r"\b(?:zero|equal|equality)\b", body, re.IGNORECASE),
        f"{enum_name} must not encode an exact-zero decision",
    )


def validate_binary64_replay_records(header: str) -> None:
    bodies = dict(struct_bodies(header))
    records = (
        (
            "SquaredDistanceFilterInput",
            9,
            ("witness_bits", "left_bits", "right_bits"),
        ),
        (
            "Orientation3DFilterInput",
            12,
            ("a_bits", "b_bits", "c_bits", "d_bits"),
        ),
    )
    for name, expected_words, fields in records:
        body = bodies.get(name)
        require(body is not None, f"the public header has no {name}")
        require(
            re.search(r"std::uint64_t\s+replay_id\b", body) is not None,
            f"{name}.replay_id must be an unsigned 64-bit value",
        )
        for field in fields:
            require(
                re.search(
                    rf"std::array\s*<\s*std::uint64_t\s*,\s*3\s*>\s*"
                    rf"{re.escape(field)}\b",
                    body,
                )
                is not None,
                f"{name} does not preserve {field} as three binary64 words",
            )
        word_arrays = re.findall(
            r"std::array\s*<\s*std::uint64_t\s*,\s*(\d+)\s*>", body
        )
        scalar_words = len(
            re.findall(r"std::uint64_t\s+(?!replay_id\b)[A-Za-z_]\w*", body)
        )
        require(
            sum(int(count) for count in word_arrays) + scalar_words
            == expected_words,
            f"{name} must preserve exactly {expected_words} binary64 words",
        )


def validate_power_replay_record(header: str) -> None:
    bodies = dict(struct_bodies(header))
    point = bodies.get("PowerBisectorLabelPoint")
    require(point is not None, "the public header has no PowerBisectorLabelPoint")
    require(
        re.search(r"std::uint32_t\s+point_id\b", point) is not None,
        "PowerBisectorLabelPoint.point_id must be uint32",
    )
    require(
        re.search(
            r"std::array\s*<\s*std::uint64_t\s*,\s*3\s*>\s*"
            r"coordinate_bits\b",
            point,
        )
        is not None,
        "PowerBisectorLabelPoint must preserve three binary64 coordinate words",
    )
    body = bodies.get("PowerBisectorFilterInput")
    require(body is not None, "the public header has no PowerBisectorFilterInput")
    for pattern, message in (
        (r"std::uint64_t\s+replay_id\b", "power replay_id must be uint64"),
        (
            r"std::array\s*<\s*std::uint64_t\s*,\s*3\s*>\s*"
            r"witness_numerator_bits\b",
            "power witness must preserve three numerator words",
        ),
        (
            r"std::uint64_t\s+witness_denominator_bits\b",
            "power witness must preserve its denominator word",
        ),
        (r"std::uint32_t\s+cardinality\b", "power cardinality must be uint32"),
    ):
        require(re.search(pattern, body) is not None, message)
    for field in ("r_points", "q_points"):
        require(
            re.search(
                rf"std::array\s*<\s*PowerBisectorLabelPoint\s*,\s*"
                rf"maximum_power_bisector_cardinality\s*>\s*{field}\b",
                body,
                flags=re.DOTALL,
            )
            is not None,
            f"PowerBisectorFilterInput.{field} is not bounded by cardinality ten",
        )


def validate_public_header(header: str) -> None:
    validate_sign_enum(header)
    validate_binary64_replay_records(header)
    validate_power_replay_record(header)
    require(
        re.search(r"std::future\s*<", without_comments(header)) is not None,
        "the public GPU predicate API does not expose asynchronous completion",
    )


def require_directed_call(body: str, call: str, expected: int = 1) -> None:
    pattern = re.escape(call).replace(r"\ ", r"\s*")
    actual = len(re.findall(pattern, body))
    require(
        actual == expected,
        f"directed interval call {call} occurs {actual} times instead of {expected}",
    )


def validate_interval_header(interval: str) -> None:
    clean = without_comments(interval)
    for intrinsic in DIRECTED_INTRINSICS:
        require(intrinsic in clean, f"CUDA interval filter is missing {intrinsic}")
    require(
        "kBinary64ExponentMask" in clean
        and re.search(
            r"if\s*\(\s*\(\s*bits\s*&\s*kBinary64ExponentMask\s*\)\s*"
            r"==\s*kBinary64ExponentMask\s*\)\s*\{\s*"
            r"return\s+invalid_interval\s*\(\s*\)\s*;",
            clean,
            flags=re.DOTALL,
        )
        is not None,
        "non-finite binary64 words do not become an invalid interval",
    )
    checked = function_body(clean, "checked_interval")
    require(
        re.search(
            r"!\s*is_finite\s*\(\s*lower\s*\).*?"
            r"!\s*is_finite\s*\(\s*upper\s*\).*?lower\s*>\s*upper.*?"
            r"return\s+invalid_interval\s*\(\s*\)\s*;",
            checked,
            flags=re.DOTALL,
        )
        is not None,
        "invalid, overflowed or inverted interval bounds do not fail closed",
    )

    add = function_body(clean, "add_intervals")
    require_directed_call(add, "__dadd_rd(left.lower, right.lower)")
    require_directed_call(add, "__dadd_ru(left.upper, right.upper)")
    subtract = function_body(clean, "subtract_intervals")
    require_directed_call(subtract, "__dsub_rd(left.lower, right.upper)")
    require_directed_call(subtract, "__dsub_ru(left.upper, right.lower)")

    multiply = function_body(clean, "multiply_intervals")
    multiplication_calls = (
        "__dmul_rd(left.lower, right.lower)",
        "__dmul_rd(left.lower, right.upper)",
        "__dmul_rd(left.upper, right.lower)",
        "__dmul_rd(left.upper, right.upper)",
        "__dmul_ru(left.upper, right.upper)",
        "__dmul_ru(left.upper, right.lower)",
        "__dmul_ru(left.lower, right.upper)",
        "__dmul_ru(left.lower, right.lower)",
    )
    for call in multiplication_calls:
        require_directed_call(multiply, call, expected=2)
    require(
        multiply.count("__dmul_rd") == 8 and multiply.count("__dmul_ru") == 8,
        "interval multiplication must use four two-product sign quadrants and one 4x2 fallback",
    )

    square = function_body(clean, "square_interval")
    for call in (
        "__dmul_rd(value.lower, value.lower)",
        "__dmul_rd(value.upper, value.upper)",
    ):
        require_directed_call(square, call)
    for call, expected in (
        ("__dmul_ru(value.lower, value.lower)", 2),
        ("__dmul_ru(value.upper, value.upper)", 2),
    ):
        require_directed_call(square, call, expected=expected)


def validate_filter_cuda_source(
    cuda: str,
    internal: str,
    public: str,
    *,
    label: str,
    kernel_pattern: str,
    interval_name: str,
) -> None:
    clean = without_comments(cuda)
    require(
        re.search(kernel_pattern, clean, flags=re.IGNORECASE) is not None,
        f"the {label} filter has no dedicated CUDA kernel",
    )
    require(
        "<<<" in clean and ">>>" in clean,
        f"the {label} kernel is never launched",
    )
    require(
        re.search(
            rf"if\s*\(\s*!\s*{re.escape(interval_name)}[.]valid\s*\)\s*\{{\s*"
            r"return\s+FilterSign::unknown\s*;",
            clean,
            flags=re.DOTALL,
        )
        is not None,
        f"an invalid {label} interval does not map to unknown",
    )
    require(
        re.search(
            rf"{re.escape(interval_name)}[.]lower\s*>\s*0(?:[.]0+)?",
            clean,
        )
        is not None,
        f"the {label} filter has no strictly-positive certification",
    )
    require(
        re.search(
            rf"{re.escape(interval_name)}[.]upper\s*<\s*0(?:[.]0+)?",
            clean,
        )
        is not None,
        f"the {label} filter has no strictly-negative certification",
    )
    require(
        INTERVAL_HEADER.name in clean,
        f"the {label} CUDA source bypasses the shared interval contract",
    )
    require(
        "unknown" in internal and "unknown" in public,
        f"the {label} launch contract loses tri-state semantics",
    )


def validate_cuda_sources(
    distance: str,
    distance_internal: str,
    orientation: str,
    orientation_internal: str,
    power: str,
    power_internal: str,
    public: str,
) -> None:
    validate_filter_cuda_source(
        distance,
        distance_internal,
        public,
        label="distance",
        kernel_pattern=(
            r"__global__\s+(?:static\s+)?void\s+"
            r"[A-Za-z_]\w*(?:distance|squared)[A-Za-z_]\w*\s*\("
        ),
        interval_name="difference",
    )
    distance_clean = without_comments(distance)
    require(
        re.search(
            r"difference\s*=\s*subtract_intervals\s*\(\s*"
            r"left_squared\s*,\s*right_squared\s*\)",
            distance_clean,
            flags=re.DOTALL,
        )
        is not None,
        "distance comparison no longer computes left squared minus right squared",
    )
    require(
        re.search(r"field\s*\*\s*count\s*\+\s*index", distance_clean)
        is not None,
        "distance coordinates are not packed field-major for coalesced loads",
    )
    validate_filter_cuda_source(
        orientation,
        orientation_internal,
        public,
        label="orientation",
        kernel_pattern=(
            r"__global__\s+(?:static\s+)?void\s+"
            r"[A-Za-z_]\w*orientation[A-Za-z_0-9]*\s*\("
        ),
        interval_name="determinant",
    )
    orientation_clean = without_comments(orientation)
    require(
        "multiply_intervals" in orientation_clean
        and re.search(
            r"add_intervals\s*\(\s*subtract_intervals\s*\(\s*"
            r"first_term\s*,\s*second_term\s*\)\s*,\s*third_term\s*\)",
            orientation_clean,
            flags=re.DOTALL,
        )
        is not None,
        "orientation_3d does not evaluate u0*m0 - u1*m1 + u2*m2",
    )
    require(
        re.search(r"field\s*\*\s*count\s*\+\s*index", orientation_clean)
        is not None,
        "orientation_3d coordinates are not packed field-major for coalesced loads",
    )
    validate_filter_cuda_source(
        power,
        power_internal,
        public,
        label="power-bisector",
        kernel_pattern=(
            r"__global__\s+(?:static\s+)?void\s+"
            r"[A-Za-z_]\w*power_bisector[A-Za-z_0-9]*\s*\("
        ),
        interval_name="homogeneous_value",
    )
    power_clean = without_comments(power)
    power_body = function_body(power_clean, "filter_power_bisector")
    require(
        re.search(
            r"q_minus_r\s*=\s*subtract_intervals\s*\(\s*q\s*,\s*r\s*\)",
            power_body,
            flags=re.DOTALL,
        )
        is not None,
        "power-bisector must use q-r to preserve the public sign convention",
    )
    require(
        re.search(
            r"a_minus_dr\s*=\s*subtract_intervals\s*\(\s*numerator\s*,\s*"
            r"multiply_intervals\s*\(\s*denominator\s*,\s*r\s*\)\s*\)",
            power_body,
            flags=re.DOTALL,
        )
        is not None
        and re.search(
            r"a_minus_dq\s*=\s*subtract_intervals\s*\(\s*numerator\s*,\s*"
            r"multiply_intervals\s*\(\s*denominator\s*,\s*q\s*\)\s*\)",
            power_body,
            flags=re.DOTALL,
        )
        is not None,
        "power-bisector no longer evaluates A-D*r and A-D*q",
    )
    require(
        re.search(
            r"paired_sum\s*=\s*add_intervals\s*\(\s*a_minus_dr\s*,\s*"
            r"a_minus_dq\s*\)",
            power_body,
            flags=re.DOTALL,
        )
        is not None
        and re.search(
            r"term\s*=\s*multiply_intervals\s*\(\s*q_minus_r\s*,\s*"
            r"paired_sum\s*\)",
            power_body,
            flags=re.DOTALL,
        )
        is not None,
        "power-bisector paired factorization changed",
    )
    require(
        "axis_value" in power_body
        and "cardinalities[index]" in power_body
        and re.search(r"field\s*\*\s*count\s*\+\s*index", power_clean)
        is not None,
        "power-bisector does not use cardinality-bounded axis subtotals and SoA loads",
    )


def validate_host_source(
    host: str,
    header: str,
    distance_internal: str,
    orientation_internal: str,
    power_internal: str,
) -> None:
    clean = without_comments(host)
    require(PUBLIC_HEADER.name in clean, "the host path does not include its public API")
    require(
        INTERNAL_HEADER.name in clean
        and ORIENTATION_INTERNAL_HEADER.name in clean
        and POWER_INTERNAL_HEADER.name in clean,
        "the host path does not include all private CUDA launch contracts",
    )
    require(
        len(re.findall(r"std::launch::async", clean)) >= 6
        and re.search(
            r"fallback_future\s*=\s*std::async\s*\(\s*"
            r"std::launch::async",
            clean,
            flags=re.DOTALL,
        )
        is not None,
        "the CPU fallback is not scheduled as an asynchronous task",
    )
    require(
        "compare_squared_distances" in clean
        or "decide_squared_distance_order" in clean,
        "the host fallback does not call the Phase 2A CPU predicate",
    )
    require(
        "decide_orientation_3d" in clean,
        "the orientation host fallback does not call the Phase 2A CPU predicate",
    )
    require(
        "decide_power_bisector_side" in clean
        and "power_witness" in clean
        and "ExactRational::from_binary64_bits" in clean,
        "the power-bisector host fallback does not reconstruct the exact rational witness",
    )
    require(
        "denominator.sign() <= 0" in clean
        and "denominator.denominator() != 1" in clean
        and "common_divisor != 1" in clean
        and "maximum_power_label_cardinality" in clean,
        "the host path does not enforce the bounded canonical integral power witness",
    )
    require(
        clean.count("PredicateFilterPolicy::allow_adaptive") >= 2,
        "both GPU predicates must use the adaptive CPU fallback",
    )
    require(
        clean.count("PredicateFilterPolicy::multiprecision_only") >= 2,
        "both GPU predicates must audit known signs with multiprecision",
    )
    require(
        re.search(
            r"cpu_decision\s*\(.*?PredicateFilterPolicy::allow_adaptive\s*\)",
            clean,
            flags=re.DOTALL,
        )
        is not None
        and re.search(
            r"fallback_future\s*=\s*std::async\s*\(.*?resolve_unknowns\s*\(",
            clean,
            flags=re.DOTALL,
        )
        is not None,
        "GPU unknown is not replayed through the adaptive CPU cascade",
    )
    require(
        re.search(r"if\s*\(\s*options[.]audit_gpu_signs\s*\)", clean) is not None
        and re.search(
            r"gpu_outputs\s*\[\s*index\s*\][.]sign\s*==\s*"
            r"FilterSign::unknown",
            clean,
        )
        is not None
        and re.search(
            r"cpu_decision\s*\(.*?"
            r"PredicateFilterPolicy::multiprecision_only\s*\)",
            clean,
            flags=re.DOTALL,
        )
        is not None
        and re.search(
            r"oracle[.]sign\s*\(\s*\)\s*!=\s*"
            r"result[.]decisions\s*\[\s*index\s*\][.]sign",
            clean,
        )
        is not None,
        "known GPU signs are not audited by the independent multiprecision path",
    )
    require(
        len(re.findall(r"\bunknown\b", clean)) >= 1,
        "the host path has no explicit GPU-unknown branch",
    )
    require(
        "replay_id" in clean,
        "the host path drops the replay identifier",
    )
    require(
        re.search(r"std::future\s*<", without_comments(header)) is not None
        and "filter_squared_distances_on_gpu" in distance_internal
        and "filter_squared_distances_on_gpu" in clean,
        "the distance asynchronous API is detached from the CUDA launcher",
    )
    require(
        "filter_orientations_3d_on_gpu" in orientation_internal
        and "filter_orientations_3d_on_gpu" in clean,
        "the orientation asynchronous API is detached from the CUDA launcher",
    )
    require(
        "filter_power_bisectors_on_gpu" in power_internal
        and "filter_power_bisectors_on_gpu" in clean,
        "the power-bisector asynchronous API is detached from the CUDA launcher",
    )


def validate_cli(cli: str) -> None:
    clean = without_comments(cli)
    for token in (
        "replay_id",
        "binary64",
        "compare_squared_distances",
        "orientation_3d",
        "power_bisector_side",
        "morsehgp3d",
    ):
        require(token in clean, f"GPU predicate replay CLI is missing {token}")


def target_body(cmake: str, command: str, target: str) -> str:
    match = re.search(
        rf"{re.escape(command)}\s*\(\s*{re.escape(target)}\b(?P<body>.*?)\)",
        cmake,
        flags=re.DOTALL,
    )
    require(match is not None, f"CMake does not declare {command}({target})")
    return match.group("body")


def validate_cmake(cmake: str, cuda_module: str) -> None:
    library_body = target_body(cmake, "add_library", LIBRARY_TARGET)
    replay_body = target_body(cmake, "add_executable", REPLAY_TARGET)
    for source in (
        CUDA_SOURCE,
        ORIENTATION_CUDA_SOURCE,
        POWER_CUDA_SOURCE,
        HOST_SOURCE,
    ):
        require(
            source.as_posix() in library_body,
            f"{LIBRARY_TARGET} does not compile {source}",
        )
    require(
        CLI_SOURCE.as_posix() in replay_body,
        f"{REPLAY_TARGET} does not compile {CLI_SOURCE}",
    )
    require(
        re.search(
            rf"morsehgp3d_configure_cuda_target\s*\(\s*"
            rf"{re.escape(LIBRARY_TARGET)}\s*\)",
            cmake,
        )
        is not None,
        f"{LIBRARY_TARGET} bypasses morsehgp3d_configure_cuda_target",
    )
    replay_links = target_body(cmake, "target_link_libraries", REPLAY_TARGET)
    require(
        LIBRARY_TARGET in replay_links
        or "morsehgp3d::gpu_predicates" in replay_links,
        f"{REPLAY_TARGET} is not linked to {LIBRARY_TARGET}",
    )
    combined = f"{cmake}\n{cuda_module}"
    for forbidden in ("--use_fast_math", "-use_fast_math"):
        require(forbidden not in combined, f"CUDA build enables forbidden {forbidden}")
    require(
        "morsehgp3d_configure_cuda_target" in cuda_module,
        "the CUDA target policy function is absent",
    )
    require(
        'CUDA_ARCHITECTURES "120-real"' in cuda_module,
        "the CUDA target policy is no longer AOT-only for sm_120",
    )


def named_presets(value: Any, section: str) -> dict[str, dict[str, Any]]:
    presets = value.get(section) if isinstance(value, dict) else None
    require(isinstance(presets, list), f"{section} must be an array")
    result: dict[str, dict[str, Any]] = {}
    for index, preset in enumerate(presets):
        require(isinstance(preset, dict), f"{section}[{index}] must be an object")
        name = preset.get("name")
        require(type(name) is str and bool(name), f"{section}[{index}] has no name")
        require(name not in result, f"duplicate {section} preset: {name}")
        result[name] = preset
    return result


def validate_presets(value: dict[str, Any]) -> None:
    presets = named_presets(value, "buildPresets")
    for name in CUDA_PRESETS:
        require(name in presets, f"missing CUDA build preset: {name}")
        targets = presets[name].get("targets")
        require(isinstance(targets, list), f"{name} targets must be an array")
        require(
            targets.count(REPLAY_TARGET) == 1,
            f"{name} must build {REPLAY_TARGET} exactly once",
        )


def validate_contract(files: ContractFiles) -> None:
    validate_public_header(files.texts[PUBLIC_HEADER])
    validate_interval_header(files.texts[INTERVAL_HEADER])
    validate_cuda_sources(
        files.texts[CUDA_SOURCE],
        files.texts[INTERNAL_HEADER],
        files.texts[ORIENTATION_CUDA_SOURCE],
        files.texts[ORIENTATION_INTERNAL_HEADER],
        files.texts[POWER_CUDA_SOURCE],
        files.texts[POWER_INTERNAL_HEADER],
        files.texts[PUBLIC_HEADER],
    )
    validate_host_source(
        files.texts[HOST_SOURCE],
        files.texts[PUBLIC_HEADER],
        files.texts[INTERNAL_HEADER],
        files.texts[ORIENTATION_INTERNAL_HEADER],
        files.texts[POWER_INTERNAL_HEADER],
    )
    validate_cli(files.texts[CLI_SOURCE])
    validate_cmake(files.texts[CMAKE_SOURCE], files.texts[CUDA_MODULE])
    validate_presets(files.presets)


def replace_once(text: str, old: str, new: str, label: str) -> str:
    require(old in text, f"negative mutation precondition is absent: {label}")
    return text.replace(old, new, 1)


def replace_all(text: str, old: str, new: str, label: str) -> str:
    require(old in text, f"negative mutation precondition is absent: {label}")
    return text.replace(old, new)


def replace_regex_once(text: str, pattern: str, replacement: str, label: str) -> str:
    result, count = re.subn(pattern, replacement, text, count=1, flags=re.DOTALL)
    require(count == 1, f"negative mutation precondition is absent: {label}")
    return result


def write_contract(project: Path, files: ContractFiles) -> None:
    for relative, text in files.texts.items():
        destination = project / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(text, encoding="utf-8")
    (project / PRESETS_SOURCE).write_text(
        json.dumps(files.presets, separators=(",", ":"), sort_keys=True),
        encoding="utf-8",
    )


def expect_rejected(
    files: ContractFiles,
    mutate: Callable[[ContractFiles], None],
    label: str,
) -> None:
    candidate = copy.deepcopy(files)
    mutate(candidate)
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-phase2b-negative-") as root:
        project = Path(root)
        write_contract(project, candidate)
        try:
            validate_contract(load_contract(project))
        except ContractError:
            return
    raise ContractError(f"negative mutation was accepted: {label}")


def mutate_text(
    path: Path, old: str, new: str, label: str
) -> Callable[[ContractFiles], None]:
    def mutate(files: ContractFiles) -> None:
        files.texts[path] = replace_once(files.texts[path], old, new, label)

    return mutate


def mutate_text_regex(
    path: Path, pattern: str, replacement: str, label: str
) -> Callable[[ContractFiles], None]:
    def mutate(files: ContractFiles) -> None:
        files.texts[path] = replace_regex_once(
            files.texts[path], pattern, replacement, label
        )

    return mutate


def mutate_text_all(
    path: Path, old: str, new: str, label: str
) -> Callable[[ContractFiles], None]:
    def mutate(files: ContractFiles) -> None:
        files.texts[path] = replace_all(files.texts[path], old, new, label)

    return mutate


def remove_replay_target(files: ContractFiles, preset_name: str) -> None:
    presets = named_presets(files.presets, "buildPresets")
    targets = presets[preset_name].get("targets")
    require(isinstance(targets, list), f"{preset_name} has no mutable targets")
    require(REPLAY_TARGET in targets, f"{preset_name} mutation target is absent")
    targets.remove(REPLAY_TARGET)


def validate_negative_mutations(files: ContractFiles) -> int:
    mutations: list[tuple[str, Callable[[ContractFiles], None]]] = [
        (
            "unknown no longer zero",
            mutate_text_regex(
                PUBLIC_HEADER,
                r"\bunknown\s*=\s*0\b",
                "unknown = 2",
                "unknown value",
            ),
        ),
        (
            "exact zero added",
            mutate_text_regex(
                PUBLIC_HEADER,
                r"\bunknown\s*=\s*0\b",
                "unknown = 0, zero = 2",
                "zero enumerator",
            ),
        ),
        (
            "replay id removed",
            mutate_text(PUBLIC_HEADER, "replay_id", "discarded_id", "replay id"),
        ),
        (
            "non-finite guard removed",
            mutate_text_regex(
                INTERVAL_HEADER,
                r"if\s*\(\s*\(\s*bits\s*&\s*kBinary64ExponentMask\s*\)\s*"
                r"==\s*kBinary64ExponentMask\s*\)\s*\{\s*"
                r"return\s+invalid_interval\s*\(\s*\)\s*;\s*\}",
                "if (false) { return invalid_interval(); }",
                "finite-word guard",
            ),
        ),
        (
            "asynchronous launch deferred",
            mutate_text(
                HOST_SOURCE,
                "std::launch::async",
                "std::launch::deferred",
                "async launch",
            ),
        ),
        (
            "adaptive fallback removed",
            mutate_text(
                HOST_SOURCE,
                "PredicateFilterPolicy::allow_adaptive",
                "PredicateFilterPolicy::multiprecision_only",
                "adaptive fallback",
            ),
        ),
        (
            "known-sign audit removed",
            mutate_text(
                HOST_SOURCE,
                "PredicateFilterPolicy::multiprecision_only",
                "PredicateFilterPolicy::allow_adaptive",
                "multiprecision audit",
            ),
        ),
        (
            "CUDA policy bypassed",
            mutate_text_regex(
                CMAKE_SOURCE,
                rf"morsehgp3d_configure_cuda_target\s*\(\s*"
                rf"{re.escape(LIBRARY_TARGET)}\s*\)",
                f"# configure_cuda_target_removed({LIBRARY_TARGET})",
                "CUDA target policy",
            ),
        ),
        (
            "fast math injected",
            lambda candidate: candidate.texts.__setitem__(
                CMAKE_SOURCE,
                candidate.texts[CMAKE_SOURCE]
                + f"\ntarget_compile_options({LIBRARY_TARGET} PRIVATE --use_fast_math)\n",
            ),
        ),
        (
            "release preset omits replay target",
            lambda candidate: remove_replay_target(candidate, "cuda-release"),
        ),
        (
            "audit preset omits replay target",
            lambda candidate: remove_replay_target(candidate, "cuda-audit"),
        ),
        (
            "addition rounding directions inverted",
            mutate_text_regex(
                INTERVAL_HEADER,
                r"__dadd_rd\(\s*left[.]lower\s*,\s*right[.]lower\s*\)\s*,\s*"
                r"__dadd_ru\(\s*left[.]upper\s*,\s*right[.]upper\s*\)",
                "__dadd_ru(left.lower, right.lower), "
                "__dadd_rd(left.upper, right.upper)",
                "addition directions",
            ),
        ),
        (
            "subtraction rounding directions inverted",
            mutate_text_regex(
                INTERVAL_HEADER,
                r"__dsub_rd\(\s*left[.]lower\s*,\s*right[.]upper\s*\)\s*,\s*"
                r"__dsub_ru\(\s*left[.]upper\s*,\s*right[.]lower\s*\)",
                "__dsub_ru(left.lower, right.upper), "
                "__dsub_rd(left.upper, right.lower)",
                "subtraction directions",
            ),
        ),
        (
            "positive square rounding directions inverted",
            mutate_text_regex(
                INTERVAL_HEADER,
                r"lower\s*=\s*__dmul_rd\(\s*value[.]lower\s*,\s*"
                r"value[.]lower\s*\)\s*;\s*upper\s*=\s*__dmul_ru\(\s*"
                r"value[.]upper\s*,\s*value[.]upper\s*\)\s*;",
                "lower = __dmul_ru(value.lower, value.lower); "
                "upper = __dmul_rd(value.upper, value.upper);",
                "positive square directions",
            ),
        ),
        (
            "negative square endpoints inverted",
            mutate_text_regex(
                INTERVAL_HEADER,
                r"lower\s*=\s*__dmul_rd\(\s*value[.]upper\s*,\s*"
                r"value[.]upper\s*\)\s*;\s*upper\s*=\s*__dmul_ru\(\s*"
                r"value[.]lower\s*,\s*value[.]lower\s*\)\s*;",
                "lower = __dmul_rd(value.lower, value.lower); "
                "upper = __dmul_ru(value.upper, value.upper);",
                "negative square endpoints",
            ),
        ),
        (
            "positive certification accepts zero",
            mutate_text(
                CUDA_SOURCE,
                "difference.lower > 0.0",
                "difference.lower >= 0.0",
                "strict positive distance sign",
            ),
        ),
        (
            "negative certification accepts zero",
            mutate_text(
                CUDA_SOURCE,
                "difference.upper < 0.0",
                "difference.upper <= 0.0",
                "strict negative distance sign",
            ),
        ),
        (
            "distance comparison operands inverted",
            mutate_text(
                CUDA_SOURCE,
                "subtract_intervals(left_squared, right_squared)",
                "subtract_intervals(right_squared, left_squared)",
                "distance subtraction order",
            ),
        ),
        (
            "orientation positive certification accepts zero",
            mutate_text(
                ORIENTATION_CUDA_SOURCE,
                "determinant.lower > 0.0",
                "determinant.lower >= 0.0",
                "strict positive orientation sign",
            ),
        ),
        (
            "orientation negative certification accepts zero",
            mutate_text(
                ORIENTATION_CUDA_SOURCE,
                "determinant.upper < 0.0",
                "determinant.upper <= 0.0",
                "strict negative orientation sign",
            ),
        ),
        (
            "orientation cofactor sign inverted",
            mutate_text(
                ORIENTATION_CUDA_SOURCE,
                "subtract_intervals(first_term, second_term)",
                "add_intervals(first_term, second_term)",
                "orientation cofactor sign",
            ),
        ),
        (
            "power-bisector subtraction order inverted",
            mutate_text(
                POWER_CUDA_SOURCE,
                "subtract_intervals(q, r)",
                "subtract_intervals(r, q)",
                "power q-r order",
            ),
        ),
        (
            "power-bisector paired sum changed to difference",
            mutate_text(
                POWER_CUDA_SOURCE,
                "add_intervals(a_minus_dr, a_minus_dq)",
                "subtract_intervals(a_minus_dr, a_minus_dq)",
                "power paired sum",
            ),
        ),
        (
            "power-bisector denominator factor removed",
            mutate_text(
                POWER_CUDA_SOURCE,
                "multiply_intervals(denominator, r)",
                "multiply_intervals(point_interval(0U), r)",
                "power denominator factor",
            ),
        ),
        (
            "power-bisector positive certification accepts zero",
            mutate_text(
                POWER_CUDA_SOURCE,
                "homogeneous_value.lower > 0.0",
                "homogeneous_value.lower >= 0.0",
                "strict positive power sign",
            ),
        ),
        (
            "power-bisector negative certification accepts zero",
            mutate_text(
                POWER_CUDA_SOURCE,
                "homogeneous_value.upper < 0.0",
                "homogeneous_value.upper <= 0.0",
                "strict negative power sign",
            ),
        ),
        (
            "power-bisector positive denominator guard removed",
            mutate_text(
                HOST_SOURCE,
                "denominator.sign() <= 0",
                "denominator.sign() < 0",
                "positive power denominator",
            ),
        ),
        (
            "power-bisector canonical witness guard removed",
            mutate_text(
                HOST_SOURCE,
                "common_divisor != 1",
                "false",
                "canonical power witness",
            ),
        ),
        (
            "power-bisector CUDA source removed",
            mutate_text(
                CMAKE_SOURCE,
                "src/cuda/phase2b_power_bisector_filter.cu",
                "src/cuda/removed_power_bisector_filter.cu",
                "power CUDA source",
            ),
        ),
        (
            "positive-negative multiplication endpoint changed",
            mutate_text(
                INTERVAL_HEADER,
                "__dmul_rd(left.upper, right.lower)",
                "__dmul_rd(left.lower, right.lower)",
                "positive-negative lower endpoint",
            ),
        ),
    ]
    for intrinsic in DIRECTED_INTRINSICS:
        mutations.append(
            (
                f"directed intrinsic {intrinsic} removed",
                mutate_text_all(
                    INTERVAL_HEADER,
                    intrinsic,
                    f"removed_{intrinsic.removeprefix('__')}",
                    intrinsic,
                ),
            )
        )
    for label, mutation in mutations:
        expect_rejected(files, mutation, label)
    return len(mutations)


def main(arguments: list[str]) -> int:
    if len(arguments) != 2:
        print("usage: check_phase2b_predicates.py PROJECT_SOURCE", file=sys.stderr)
        return 2
    project = Path(arguments[1]).resolve()
    try:
        files = load_contract(project)
        validate_contract(files)
        negative_mutations = validate_negative_mutations(files)
    except (ContractError, StopIteration) as error:
        print(f"Phase 2B predicate contract failed: {error}", file=sys.stderr)
        return 1
    print(
        json.dumps(
            {
                "cuda_runtime_executed": False,
                "cuda_target": LIBRARY_TARGET,
                "negative_mutations": negative_mutations,
                "predicates": [
                    "compare_squared_distances",
                    "orientation_3d",
                    "power_bisector_side",
                ],
                "replay_target": REPLAY_TARGET,
                "schema_version": 3,
            },
            separators=(",", ":"),
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
