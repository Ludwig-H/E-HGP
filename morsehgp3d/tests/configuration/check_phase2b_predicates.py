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
INTERNAL_HEADER = Path("src/cuda/phase2b_distance_filter_internal.hpp")
CUDA_SOURCE = Path("src/cuda/phase2b_distance_filter.cu")
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
        INTERNAL_HEADER,
        CUDA_SOURCE,
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


def validate_binary64_replay_record(header: str) -> None:
    matching_body: str | None = None
    for _, body in struct_bodies(header):
        if re.search(r"\breplay_id\b", body):
            matching_body = body
            break
    require(matching_body is not None, "no public GPU record contains replay_id")
    require(
        re.search(r"std::uint64_t\s+replay_id\b", matching_body) is not None,
        "replay_id must be an unsigned 64-bit value",
    )
    word_arrays = re.findall(
        r"std::array\s*<\s*std::uint64_t\s*,\s*(\d+)\s*>", matching_body
    )
    word_count = sum(int(count) for count in word_arrays)
    scalar_words = len(
        re.findall(r"std::uint64_t\s+(?!replay_id\b)[A-Za-z_]\w*", matching_body)
    )
    require(
        word_count + scalar_words >= 9,
        "the replayable distance record must preserve all nine binary64 words",
    )
    require(
        re.search(r"(?:binary64|bits|words?)", matching_body, re.IGNORECASE) is not None,
        "the replay record does not identify its binary64 bit words",
    )


def validate_public_header(header: str) -> None:
    validate_sign_enum(header)
    validate_binary64_replay_record(header)
    require(
        re.search(r"std::future\s*<", without_comments(header)) is not None,
        "the public GPU predicate API does not expose asynchronous completion",
    )


def validate_cuda_source(cuda: str, internal: str, public: str) -> None:
    clean = without_comments(cuda)
    for intrinsic in DIRECTED_INTRINSICS:
        require(intrinsic in clean, f"CUDA interval filter is missing {intrinsic}")
    require(
        re.search(
            r"__global__\s+(?:static\s+)?void\s+"
            r"[A-Za-z_]\w*(?:distance|squared)[A-Za-z_]\w*\s*\(",
            clean,
            flags=re.IGNORECASE,
        )
        is not None,
        "the distance filter has no dedicated CUDA kernel",
    )
    require("<<<" in clean and ">>>" in clean, "the distance kernel is never launched")
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
    require(
        re.search(
            r"if\s*\(\s*!\s*difference[.]valid\s*\)\s*\{\s*"
            r"return\s+FilterSign::unknown\s*;",
            clean,
            flags=re.DOTALL,
        )
        is not None,
        "an invalid or non-finite CUDA interval does not map to unknown",
    )
    require(
        "isfinite" in clean
        and re.search(
            r"!\s*device_is_finite\s*\(\s*lower\s*\).*?"
            r"!\s*device_is_finite\s*\(\s*upper\s*\).*?"
            r"return\s+invalid_interval\s*\(\s*\)\s*;",
            clean,
            flags=re.DOTALL,
        )
        is not None,
        "overflowed directed operations do not fail closed",
    )
    require(
        re.search(r"\blower\b\s*>\s*0(?:[.]0+)?", clean) is not None,
        "the CUDA interval filter has no strictly-positive certification",
    )
    require(
        re.search(r"\bupper\b\s*<\s*0(?:[.]0+)?", clean) is not None,
        "the CUDA interval filter has no strictly-negative certification",
    )
    require(
        re.search(r"(?:==|<=|>=)\s*0(?:[.]0+)?[^;\n]*(?:zero|equal)", clean)
        is None,
        "the CUDA filter appears to certify exact zero",
    )
    require(
        INTERNAL_HEADER.name in cuda,
        "the CUDA source does not include the private launch contract",
    )
    require("unknown" in internal and "unknown" in public, "the launch contract loses tri-state semantics")


def validate_host_source(host: str, header: str, internal: str) -> None:
    clean = without_comments(host)
    require(PUBLIC_HEADER.name in clean, "the host path does not include its public API")
    require(
        INTERNAL_HEADER.name in clean,
        "the host path does not include the private CUDA launch contract",
    )
    require(
        len(re.findall(r"std::launch::async", clean)) >= 2
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
        and "filter_squared_distances_on_gpu" in internal
        and "filter_squared_distances_on_gpu" in clean,
        "the asynchronous API is detached from the CUDA launcher",
    )


def validate_cli(cli: str) -> None:
    clean = without_comments(cli)
    for token in (
        "replay_id",
        "binary64",
        "compare_squared_distances",
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
    for source in (CUDA_SOURCE, HOST_SOURCE):
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
    validate_cuda_source(
        files.texts[CUDA_SOURCE],
        files.texts[INTERNAL_HEADER],
        files.texts[PUBLIC_HEADER],
    )
    validate_host_source(
        files.texts[HOST_SOURCE],
        files.texts[PUBLIC_HEADER],
        files.texts[INTERNAL_HEADER],
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
                CUDA_SOURCE,
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
    ]
    for intrinsic in DIRECTED_INTRINSICS:
        mutations.append(
            (
                f"directed intrinsic {intrinsic} removed",
                mutate_text_all(
                    CUDA_SOURCE,
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
                "predicate": "compare_squared_distances",
                "replay_target": REPLAY_TARGET,
                "schema_version": 1,
            },
            separators=(",", ":"),
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
