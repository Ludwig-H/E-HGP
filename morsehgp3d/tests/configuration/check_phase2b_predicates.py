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
CONTEXT_INTERNAL_HEADER = Path(
    "src/cuda/phase2b_predicate_context_internal.hpp"
)
CONTEXT_CUDA_HEADER = Path("src/cuda/phase2b_predicate_context.cuh")
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
CONTEXT_HARNESS_SOURCE = Path(
    "src/tools/gpu_predicate_context_harness.cpp"
)
CMAKE_SOURCE = Path("CMakeLists.txt")
CUDA_MODULE = Path("cmake/MorseHGP3DCuda.cmake")
PRESETS_SOURCE = Path("CMakePresets.json")

LIBRARY_TARGET = "morsehgp3d_gpu_predicates"
REPLAY_TARGET = "morsehgp3d_gpu_predicate_replay"
CONTEXT_HARNESS_TARGET = "morsehgp3d_gpu_predicate_context_harness"
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
        CONTEXT_INTERNAL_HEADER,
        CONTEXT_CUDA_HEADER,
        INTERNAL_HEADER,
        CUDA_SOURCE,
        ORIENTATION_INTERNAL_HEADER,
        ORIENTATION_CUDA_SOURCE,
        POWER_INTERNAL_HEADER,
        POWER_CUDA_SOURCE,
        HOST_SOURCE,
        CLI_SOURCE,
        CONTEXT_HARNESS_SOURCE,
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


def class_body(text: str, name: str) -> str:
    clean = without_comments(text)
    match = re.search(rf"\bclass\s+{re.escape(name)}\b[^;{{]*\{{", clean)
    require(match is not None, f"missing class body: {name}")
    depth = 1
    cursor = match.end()
    while cursor < len(clean) and depth > 0:
        if clean[cursor] == "{":
            depth += 1
        elif clean[cursor] == "}":
            depth -= 1
        cursor += 1
    require(depth == 0, f"unterminated class body: {name}")
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
    require(
        re.search(
            r"enum\s+class\s+FilterSign\s*:\s*std::int8_t\s*\{",
            without_comments(header),
        )
        is not None,
        "FilterSign must occupy exactly one signed byte",
    )
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
    clean = without_comments(header)
    context = class_body(clean, "PredicateFilterContext")
    require(
        re.search(
            r"std::shared_ptr\s*<\s*detail::PredicateFilterContextState\s*>\s*"
            r"state_\b",
            context,
        )
        is not None,
        "the public predicate context does not own shared asynchronous state",
    )
    require(
        re.search(
            r"PredicateFilterContext\s*\(\s*const\s+"
            r"PredicateFilterContext\s*&\s*\)\s*=\s*delete",
            context,
        )
        is not None
        and re.search(
            r"operator=\s*\(\s*const\s+PredicateFilterContext\s*&\s*\)\s*"
            r"=\s*delete",
            context,
        )
        is not None,
        "the public predicate context must be non-copyable",
    )
    require(
        re.search(
            r"PredicateFilterContext\s*\(\s*PredicateFilterContext\s*&&\s*\)\s*"
            r"noexcept",
            context,
        )
        is not None
        and re.search(
            r"operator=\s*\(\s*PredicateFilterContext\s*&&\s*\)\s*noexcept",
            context,
        )
        is not None,
        "the public predicate context must be nothrow move-enabled",
    )
    for function_name in (
        "decide_squared_distance_batch_async",
        "decide_orientation_3d_batch_async",
        "decide_power_bisector_batch_async",
    ):
        require(
            len(
                re.findall(
                    rf"{re.escape(function_name)}\s*\(\s*"
                    r"PredicateFilterContext\s*&",
                    clean,
                )
            )
            >= 2,
            f"{function_name} has no public context overload and friendship",
        )


def validate_context_state(internal: str, cuda: str) -> None:
    internal_clean = without_comments(internal)
    state = class_body(internal_clean, "PredicateFilterContextState")
    require(
        "cuda_runtime" not in internal_clean
        and "cudaStream_t" not in internal_clean,
        "the host context state must remain linkable without the CUDA runtime",
    )
    require(
        re.search(r"std::mutex\s+mutex_\b", state) is not None
        and re.search(
            r"std::lock_guard\s*<\s*std::mutex\s*>\s+lock\s*\{\s*mutex_\s*\}",
            state,
        )
        is not None,
        "the resident context does not serialize its GPU critical section",
    )
    require(
        re.search(r"std::shared_ptr\s*<\s*void\s*>\s+cuda_resources_\b", state)
        is not None,
        "the host-only context has no type-erased CUDA resource lifetime",
    )
    require(
        re.search(r"std::atomic\s*<\s*bool\s*>\s+poisoned_\b", state)
        is not None
        and re.search(
            r"poisoned_[.]load\s*\(\s*std::memory_order_acquire\s*\)",
            state,
        )
        is not None
        and re.search(r"catch\s*\(\s*[.][.][.]\s*\)", state) is not None
        and re.search(r"mark_poisoned\s*\(\s*\)\s*;", state) is not None
        and re.search(
            r"poisoned_[.]store\s*\(\s*true\s*,\s*"
            r"std::memory_order_release\s*\)",
            state,
        )
        is not None,
        "the resident context does not fail closed after a GPU failure",
    )

    cuda_clean = without_comments(cuda)
    require(
        re.search(r"#\s*if\s*!\s*defined\s*\(\s*__CUDACC__\s*\)", cuda)
        is not None,
        "the CUDA resource contract is not guarded against host-only inclusion",
    )
    require(
        len(re.findall(r"\bcudaStreamCreateWithFlags\s*\(", cuda_clean)) == 1
        and "cudaStreamNonBlocking" in cuda_clean
        and "cudaStreamDestroy" in cuda_clean
        and "cudaStreamSynchronize" in cuda_clean,
        "the resident CUDA resources do not own one reusable synchronized stream",
    )
    buffer = class_body(cuda_clean, "PredicateFilterDeviceBuffer")
    require(
        re.search(
            r"if\s*\(\s*required_count\s*<=\s*capacity_\s*\)\s*\{\s*return\s*;",
            buffer,
            flags=re.DOTALL,
        )
        is not None
        and "cudaMalloc" in buffer
        and "cudaFree" in buffer
        and re.search(r"capacity_\s*=\s*required_count", buffer) is not None,
        "the resident device buffer does not preserve high-water capacity",
    )
    resources = class_body(cuda_clean, "PredicateFilterCudaResources")
    for value_type, field in (
        ("std::uint64_t", "coordinate_bits_"),
        ("std::uint32_t", "cardinalities_"),
        ("FilterSign", "outputs_"),
    ):
        require(
            re.search(
                rf"PredicateFilterDeviceBuffer\s*<\s*{re.escape(value_type)}\s*>\s*"
                rf"{re.escape(field)}\b",
                resources,
            )
            is not None,
            f"resident CUDA resources are missing reusable {field}",
        )
    device_guard = class_body(cuda_clean, "PredicateFilterCudaDeviceGuard")
    require(
        "cudaGetDevice" in device_guard
        and device_guard.count("cudaSetDevice") >= 2
        and "restore_required_" in device_guard,
        "resident execution does not activate and restore its owning CUDA device",
    )
    require(
        re.search(r"cudaGetDevice\s*\(\s*&device_\s*\)", resources) is not None
        and re.search(r"int\s+device_\s*\{\s*-1\s*\}", resources) is not None
        and "outputs_.abandon()" in resources
        and "cudaSetDevice(device_)" in resources
        and "cudaStreamDestroy" in resources,
        "resident resources do not bind destruction to their creation device",
    )
    require(
        re.search(
            r"if\s*\(\s*!\s*opaque\s*\)\s*\{\s*opaque\s*=\s*"
            r"std::make_shared\s*<\s*PredicateFilterCudaResources\s*>",
            cuda_clean,
            flags=re.DOTALL,
        )
        is not None,
        "CUDA resources are not initialized lazily inside the shared state",
    )
    execute = function_body(cuda_clean, "execute_predicate_filter_gpu_section")
    require(
        "state.with_gpu_section" in execute
        and "PredicateFilterCudaDeviceGuard device_guard{cuda.device()}" in execute
        and "cuda.reserve" in execute
        and "cuda.synchronize()" in execute
        and "cuda.synchronize_after_failure()" in execute,
        "resident execution does not centralize locking, reuse and synchronization",
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
        CONTEXT_CUDA_HEADER.name in clean
        and "execute_predicate_filter_gpu_section" in clean,
        f"the {label} CUDA source bypasses the resident execution context",
    )
    empty_guard = re.search(
        r"if\s*\(\s*inputs[.]empty\s*\(\s*\)\s*\)\s*\{\s*"
        r"return\s*\{\s*\}\s*;\s*\}",
        clean,
        flags=re.DOTALL,
    )
    execute_offset = clean.find("execute_predicate_filter_gpu_section")
    require(
        empty_guard is not None
        and execute_offset >= 0
        and empty_guard.end() < execute_offset,
        f"an empty {label} batch may initialize CUDA resources",
    )
    require(
        re.search(
            r"PredicateFilterContextState\s*&\s*context",
            without_comments(internal),
        )
        is not None,
        f"the {label} launcher does not receive resident context state",
    )
    for forbidden in (
        "cudaStreamCreateWithFlags",
        "cudaStreamDestroy",
        "cudaMalloc",
        "cudaFree",
        "class Stream",
        "class DeviceBuffer",
    ):
        require(
            forbidden not in clean,
            f"the {label} CUDA source locally owns forbidden resource {forbidden}",
        )
    require(
        "FilterSign" in internal and "unknown" in public,
        f"the {label} launch contract loses tri-state semantics",
    )
    require(
        re.search(r"std::vector\s*<\s*FilterSign\s*>", internal) is not None
        and re.search(r"FilterSign\s*\*\s*outputs", clean) is not None
        and re.search(
            r"outputs\s*\[\s*index\s*\]\s*=\s*filter_[A-Za-z0-9_]+\s*\(",
            clean,
        )
        is not None
        and "sizeof(FilterSign)" in clean,
        f"the {label} device path does not transfer one positional sign per input",
    )
    require(
        all(
            forbidden not in clean and forbidden not in without_comments(internal)
            for forbidden in ("replay_id", "replay_ids", "device_replay_ids", "Raw")
        ),
        f"the {label} device path redundantly transfers replay identifiers",
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
    pairing_body = function_body(power_clean, "point_less_on_axis")
    require(
        "left.coordinate_bits[axis]" in pairing_body
        and "right.coordinate_bits[axis]" in pairing_body
        and re.search(
            r"for\s*\([^)]*axis[^)]*\)\s*\{\s*std::sort\s*\(",
            power_clean,
            flags=re.DOTALL,
        )
        is not None
        and "[axis](const PowerBisectorLabelPoint& left" in power_clean,
        "power-bisector labels are not paired independently on every axis",
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
        re.search(
            r"PredicateFilterContext::PredicateFilterContext\s*\(\s*\)\s*"
            r":\s*state_\s*\(\s*std::make_shared\s*<\s*"
            r"detail::PredicateFilterContextState\s*>",
            clean,
            flags=re.DOTALL,
        )
        is not None,
        "the public context does not construct shared host-only state",
    )
    require(
        clean.count("cannot schedule work on a moved-from predicate filter context")
        == 3
        and len(
            re.findall(
                r"std::shared_ptr\s*<\s*detail::PredicateFilterContextState\s*>\s*"
                r"state\s*=\s*context[.]state_",
                clean,
            )
        )
        == 3
        and len(re.findall(r"decide_batch\s*\(\s*\*state\s*,", clean)) == 3,
        "context futures do not retain and use their shared state safely",
    )
    require(
        "with_gpu_section" not in clean,
        "host validation or CPU fallback was pulled into the GPU critical section",
    )
    publication_guard = class_body(clean, "PostGpuPublicationGuard")
    require(
        "context_.mark_poisoned()" in publication_guard
        and re.search(r"if\s*\(\s*armed_\s*\)", publication_guard) is not None,
        "post-GPU publication failures do not poison the shared context",
    )
    require(
        clean.count("PostGpuPublicationGuard publication_guard{context};") == 3
        and clean.count("publication_guard.arm();") == 3
        and clean.count("publication_guard.release();") == 3
        and len(
            re.findall(
                r"filter_[A-Za-z0-9_]+_signs_on_gpu\s*\(\s*context\s*,\s*inputs\s*\)"
                r"\s*;\s*if\s*\(\s*!\s*inputs[.]empty\s*\(\s*\)\s*\)\s*\{\s*"
                r"publication_guard[.]arm\s*\(\s*\)\s*;",
                clean,
                flags=re.DOTALL,
            )
        )
        == 3,
        "the publication guard is not armed after every successful nonempty GPU section",
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
            r"gpu_outputs\s*\[\s*index\s*\]\s*==\s*"
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
        and "filter_squared_distance_signs_on_gpu" in distance_internal
        and "filter_squared_distance_signs_on_gpu" in clean,
        "the distance asynchronous API is detached from the CUDA launcher",
    )
    require(
        "filter_orientation_3d_signs_on_gpu" in orientation_internal
        and "filter_orientation_3d_signs_on_gpu" in clean,
        "the orientation asynchronous API is detached from the CUDA launcher",
    )
    require(
        "filter_power_bisector_signs_on_gpu" in power_internal
        and "filter_power_bisector_signs_on_gpu" in clean,
        "the power-bisector asynchronous API is detached from the CUDA launcher",
    )
    for decision_type in (
        "SquaredDistanceDecision",
        "Orientation3DDecision",
        "PowerBisectorDecision",
    ):
        require(
            re.search(
                rf"result[.]decisions\s*\[\s*index\s*\]\s*=\s*"
                rf"{decision_type}\s*\{{\s*inputs\s*\[\s*index\s*\]"
                r"[.]replay_id\s*,\s*output\s*,\s*"
                r"predicate_sign_from_gpu\s*\(\s*output\s*\)",
                clean,
                flags=re.DOTALL,
            )
            is not None,
            f"{decision_type} does not recover its authoritative host replay id",
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
    parse_body = function_body(clean, "parse_record")
    require(
        len(
            re.findall(
                r"if\s*\(\s*include_replay_command\s*\)",
                parse_body,
            )
        )
        == 3
        and re.search(
            r"parse_record\s*\(\s*line\s*,\s*line_number\s*,\s*"
            r"!\s*summary_only\s*\)",
            clean,
        )
        is not None,
        "summary-only replay must not construct replay commands",
    )


def validate_context_harness(harness: str) -> None:
    clean = without_comments(harness)
    for token in (
        "PredicateFilterContext primary",
        "PredicateFilterContext secondary",
        "distance_batch(513U",
        "orientation_batch(5U",
        "power_batch(385U",
        "distance_batch(257U",
        "orientation_batch(321U",
        "power_batch(289U",
        "run_first_independent_pair",
        "run_second_independent_pair",
        "resident and ephemeral decisions differ",
        "resident and ephemeral counters differ",
        "morsehgp3d.phase2b.resident_context.v1",
    ):
        require(token in clean, f"resident CUDA context harness is missing {token}")
    for function_name in (
        "decide_squared_distance_batch_async",
        "decide_orientation_3d_batch_async",
        "decide_power_bisector_batch_async",
    ):
        require(
            len(re.findall(rf"{re.escape(function_name)}\s*\(", clean)) >= 4,
            f"resident harness does not compare both {function_name} paths",
        )
    for runner, function_name in (
        ("run_distance", "decide_squared_distance_batch_async"),
        ("run_orientation", "decide_orientation_3d_batch_async"),
        ("run_power", "decide_power_bisector_batch_async"),
    ):
        body = function_body(clean, runner)
        require(
            re.search(
                rf"{re.escape(function_name)}\s*\(\s*context\s*,\s*inputs",
                body,
            )
            is not None
            and re.search(
                rf"{re.escape(function_name)}\s*\(\s*inputs\s*,", body
            )
            is not None,
            f"{runner} does not compare resident and ephemeral execution",
        )
    require(
        len(re.findall(r"BatchOptions\s+[A-Za-z_]*options\s*\{\s*true\s*\}", clean))
        == 7
        and "gpu_known_audited != coverage.gpu_known" in clean,
        "resident harness does not require known-sign GPU audits",
    )
    require(
        re.search(
            r"run_distance\s*\(\s*primary\s*,\s*\{\s*\}", clean
        )
        is not None
        and re.search(
            r"run_orientation\s*\(\s*primary\s*,\s*\{\s*\}", clean
        )
        is not None
        and re.search(r"run_power\s*\(\s*primary\s*,\s*\{\s*\}", clean)
        is not None,
        "resident harness does not exercise all empty-batch entry points",
    )
    require(
        "gpu_unknown == 0U" in clean
        and "fallback_batches == 0U" in clean
        and "exact_zeros == 0U" in clean,
        "resident harness does not require GPU-unknown CPU fallbacks",
    )
    require(
        re.search(r"if\s*\(\s*argc\s*!=\s*1\s*\)", clean) is not None
        and clean.count("std::cout") == 2
        and "std::cerr" in clean,
        "resident harness output or invocation is not deterministic",
    )


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
    harness_body = target_body(
        cmake, "add_executable", CONTEXT_HARNESS_TARGET
    )
    for source in (
        CONTEXT_INTERNAL_HEADER,
        CONTEXT_CUDA_HEADER,
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
        CONTEXT_HARNESS_SOURCE.as_posix() in harness_body,
        f"{CONTEXT_HARNESS_TARGET} does not compile {CONTEXT_HARNESS_SOURCE}",
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
    harness_links = target_body(
        cmake, "target_link_libraries", CONTEXT_HARNESS_TARGET
    )
    require(
        LIBRARY_TARGET in harness_links
        or "morsehgp3d::gpu_predicates" in harness_links,
        f"{CONTEXT_HARNESS_TARGET} is not linked to {LIBRARY_TARGET}",
    )
    require(
        re.search(
            r"if\s*\(\s*MORSEHGP3D_ENABLE_CUDA\s*\).*?"
            rf"add_executable\s*\(\s*{re.escape(CONTEXT_HARNESS_TARGET)}\b",
            cmake,
            flags=re.DOTALL,
        )
        is not None,
        "the real resident-context harness is not gated by CUDA enablement",
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
        require(
            targets.count(CONTEXT_HARNESS_TARGET) == 1,
            f"{name} must build {CONTEXT_HARNESS_TARGET} exactly once",
        )


def validate_contract(files: ContractFiles) -> None:
    validate_public_header(files.texts[PUBLIC_HEADER])
    validate_interval_header(files.texts[INTERVAL_HEADER])
    validate_context_state(
        files.texts[CONTEXT_INTERNAL_HEADER],
        files.texts[CONTEXT_CUDA_HEADER],
    )
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
    validate_context_harness(files.texts[CONTEXT_HARNESS_SOURCE])
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


def remove_context_harness_target(
    files: ContractFiles, preset_name: str
) -> None:
    presets = named_presets(files.presets, "buildPresets")
    targets = presets[preset_name].get("targets")
    require(isinstance(targets, list), f"{preset_name} has no mutable targets")
    require(
        CONTEXT_HARNESS_TARGET in targets,
        f"{preset_name} context-harness mutation target is absent",
    )
    targets.remove(CONTEXT_HARNESS_TARGET)


def validate_negative_mutations(files: ContractFiles) -> int:
    mutations: list[tuple[str, Callable[[ContractFiles], None]]] = [
        (
            "context future state made unique",
            mutate_text(
                PUBLIC_HEADER,
                "std::shared_ptr<detail::PredicateFilterContextState> state_;",
                "std::unique_ptr<detail::PredicateFilterContextState> state_;",
                "shared asynchronous context state",
            ),
        ),
        (
            "context made copyable",
            mutate_text(
                PUBLIC_HEADER,
                "PredicateFilterContext(const PredicateFilterContext&) = delete;",
                "PredicateFilterContext(const PredicateFilterContext&);",
                "non-copyable context",
            ),
        ),
        (
            "context GPU mutex removed",
            mutate_text(
                CONTEXT_INTERNAL_HEADER,
                "std::lock_guard<std::mutex> lock{mutex_};",
                "static_cast<void>(mutex_);",
                "context GPU mutex",
            ),
        ),
        (
            "context poisoning removed",
            mutate_text(
                CONTEXT_INTERNAL_HEADER,
                "poisoned_.store(true, std::memory_order_release);",
                "poisoned_.store(false, std::memory_order_release);",
                "context poisoning",
            ),
        ),
        (
            "context creation device capture removed",
            mutate_text(
                CONTEXT_CUDA_HEADER,
                "cudaGetDevice(&device_)",
                "cudaGetDevice(nullptr)",
                "context creation device",
            ),
        ),
        (
            "context execution device guard removed",
            mutate_text(
                CONTEXT_CUDA_HEADER,
                "PredicateFilterCudaDeviceGuard device_guard{cuda.device()};",
                "static_cast<void>(cuda.device());",
                "context execution device guard",
            ),
        ),
        (
            "post-GPU publication poisoning removed",
            mutate_text(
                HOST_SOURCE,
                "context_.mark_poisoned();",
                "static_cast<void>(context_);",
                "post-GPU publication poisoning",
            ),
        ),
        (
            "resident harness distance call made ephemeral",
            mutate_text(
                CONTEXT_HARNESS_SOURCE,
                "decide_squared_distance_batch_async(context, inputs, options)",
                "decide_squared_distance_batch_async(inputs, options)",
                "resident harness context overload",
            ),
        ),
        (
            "resident harness audit disabled",
            mutate_text(
                CONTEXT_HARNESS_SOURCE,
                "SquaredDistanceBatchOptions options{true}",
                "SquaredDistanceBatchOptions options{false}",
                "resident harness audit",
            ),
        ),
        (
            "resident capacity equality reallocates",
            mutate_text(
                CONTEXT_CUDA_HEADER,
                "required_count <= capacity_",
                "required_count < capacity_",
                "resident high-water reuse",
            ),
        ),
        (
            "empty distance batch enters CUDA context",
            mutate_text(
                CUDA_SOURCE,
                "if (inputs.empty()) {",
                "if (false) {",
                "empty-batch CUDA guard",
            ),
        ),
        (
            "context CUDA header removed from target",
            mutate_text(
                CMAKE_SOURCE,
                CONTEXT_CUDA_HEADER.as_posix(),
                "src/cuda/removed_phase2b_predicate_context.cuh",
                "resident context target source",
            ),
        ),
        (
            "resident context harness source removed from target",
            mutate_text(
                CMAKE_SOURCE,
                CONTEXT_HARNESS_SOURCE.as_posix(),
                "src/tools/removed_gpu_predicate_context_harness.cpp",
                "resident context harness target source",
            ),
        ),
        (
            "one future stops retaining shared context state",
            mutate_text(
                HOST_SOURCE,
                "std::shared_ptr<detail::PredicateFilterContextState> state = context.state_;",
                "auto* state = context.state_.get();",
                "future context-state retention",
            ),
        ),
        (
            "summary-only replay commands reconstructed",
            mutate_text(
                CLI_SOURCE,
                "parse_record(line, line_number, !summary_only)",
                "parse_record(line, line_number, true)",
                "summary-only replay-command guard",
            ),
        ),
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
            "filter sign widened",
            mutate_text(
                PUBLIC_HEADER,
                "enum class FilterSign : std::int8_t",
                "enum class FilterSign : std::int16_t",
                "one-byte filter sign",
            ),
        ),
        (
            "replay id removed",
            mutate_text(PUBLIC_HEADER, "replay_id", "discarded_id", "replay id"),
        ),
        (
            "device replay identifier restored",
            mutate_text(
                INTERNAL_HEADER,
                "namespace morsehgp3d::gpu::detail {",
                "namespace morsehgp3d::gpu::detail {\nstd::uint64_t replay_id{0};",
                "device replay identifier",
            ),
        ),
        (
            "distance result transfer widened",
            mutate_text(
                CUDA_SOURCE,
                "outputs.size() * sizeof(FilterSign)",
                "outputs.size() * sizeof(std::uint64_t)",
                "one-byte distance result transfer",
            ),
        ),
        (
            "distance result shifted by one index",
            mutate_text(
                CUDA_SOURCE,
                "outputs[index] = filter_squared_distance",
                "outputs[index + 1U] = filter_squared_distance",
                "positional distance result",
            ),
        ),
        (
            "distance decision takes the first replay id",
            mutate_text(
                HOST_SOURCE,
                "SquaredDistanceDecision{\n            inputs[index].replay_id",
                "SquaredDistanceDecision{\n            inputs[0U].replay_id",
                "authoritative distance replay id",
            ),
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
            "release preset omits resident context harness",
            lambda candidate: remove_context_harness_target(
                candidate, "cuda-release"
            ),
        ),
        (
            "audit preset omits replay target",
            lambda candidate: remove_replay_target(candidate, "cuda-audit"),
        ),
        (
            "audit preset omits resident context harness",
            lambda candidate: remove_context_harness_target(
                candidate, "cuda-audit"
            ),
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
            "power-bisector scalar pairing collapsed to the first axis",
            mutate_text(
                POWER_CUDA_SOURCE,
                "left.coordinate_bits[axis]",
                "left.coordinate_bits[0U]",
                "axis-specific power pairing",
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
