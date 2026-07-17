#!/usr/bin/env python3
"""Validate the closed, compile-only Phase 3.1 build envelope without CUDA."""

from __future__ import annotations

import copy
import json
import re
import sys
from pathlib import Path
from typing import Any, Callable

CUDA_IMAGE = (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
)
CONFIGURE_NAMES = {
    "phase3-base",
    "cpu-release",
    "sanitizer",
    "cuda-release",
    "cuda-audit",
}
PUBLIC_NAMES = {"cpu-release", "sanitizer", "cuda-release", "cuda-audit"}
CUDA_TARGETS = [
    "morsehgp3d_cuda_environment_probe",
    "morsehgp3d_phase3_runtime",
    "morsehgp3d_phase3",
]
CUDA_BUILD_JOBS = 8


class ContractError(ValueError):
    """A Phase 3.1 build-contract invariant was violated."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def require_keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    require(actual == expected, f"{label} fields differ: {sorted(actual ^ expected)}")


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


def preset_map(value: Any, expected: set[str], label: str) -> dict[str, dict[str, Any]]:
    require(isinstance(value, list), f"{label} must be an array")
    result: dict[str, dict[str, Any]] = {}
    for index, preset in enumerate(value):
        require(isinstance(preset, dict), f"{label}[{index}] must be an object")
        name = preset.get("name")
        require(type(name) is str and bool(name), f"{label}[{index}] has no name")
        require(name not in result, f"duplicate {label} name: {name}")
        result[name] = preset
    require(set(result) == expected, f"{label} names differ from the frozen catalog")
    return result


def validate_configure_presets(presets: dict[str, dict[str, Any]]) -> None:
    base = presets["phase3-base"]
    require_keys(
        base,
        {"name", "hidden", "binaryDir", "environment", "cacheVariables"},
        "base preset",
    )
    require(base["hidden"] is True, "phase3-base must be hidden")
    require(
        base["binaryDir"] == "${sourceDir}/../build/morsehgp3d-${presetName}",
        "each preset must have an isolated build directory",
    )
    require(
        base["cacheVariables"]
        == {
            "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
            "MORSEHGP3D_BUILD_TESTS": "ON",
            "MORSEHGP3D_BUILD_TOOLS": "ON",
        },
        "phase3-base cache variables are not closed",
    )
    require(
        base["environment"]
        == {
            "CUDAFLAGS": "",
            "NVCC_APPEND_FLAGS": "",
            "NVCC_PREPEND_FLAGS": "",
        },
        "CUDA compiler environment injection is not neutralized",
    )

    expected = {
        "cpu-release": (
            "Unix Makefiles",
            {
                "CMAKE_BUILD_TYPE": "Release",
                "MORSEHGP3D_CUDA_AUDIT": "OFF",
                "MORSEHGP3D_ENABLE_CUDA": "OFF",
                "MORSEHGP3D_ENABLE_SANITIZERS": "OFF",
            },
        ),
        "sanitizer": (
            "Unix Makefiles",
            {
                "CMAKE_BUILD_TYPE": "Debug",
                "MORSEHGP3D_CUDA_AUDIT": "OFF",
                "MORSEHGP3D_ENABLE_CUDA": "OFF",
                "MORSEHGP3D_ENABLE_SANITIZERS": "ON",
            },
        ),
        "cuda-release": (
            "Ninja",
            {
                "CMAKE_BUILD_TYPE": "Release",
                "CMAKE_CUDA_ARCHITECTURES": "120-real",
                "MORSEHGP3D_CUDA_AUDIT": "OFF",
                "MORSEHGP3D_ENABLE_CUDA": "ON",
                "MORSEHGP3D_ENABLE_SANITIZERS": "OFF",
            },
        ),
        "cuda-audit": (
            "Ninja",
            {
                "CMAKE_BUILD_TYPE": "RelWithDebInfo",
                "CMAKE_CUDA_ARCHITECTURES": "120-real",
                "MORSEHGP3D_CUDA_AUDIT": "ON",
                "MORSEHGP3D_ENABLE_CUDA": "ON",
                "MORSEHGP3D_ENABLE_SANITIZERS": "OFF",
            },
        ),
    }
    for name, (generator, cache_variables) in expected.items():
        preset = presets[name]
        require_keys(
            preset,
            {"name", "displayName", "inherits", "generator", "cacheVariables"},
            f"configure preset {name}",
        )
        require(preset["inherits"] == "phase3-base", f"{name} escaped phase3-base")
        require(preset["generator"] == generator, f"{name} has the wrong generator")
        require(
            preset["cacheVariables"] == cache_variables,
            f"{name} cache variables are not closed",
        )


def validate_build_presets(presets: dict[str, dict[str, Any]]) -> None:
    for name in ("cpu-release", "sanitizer"):
        preset = presets[name]
        require_keys(
            preset, {"name", "configurePreset", "jobs"}, f"build preset {name}"
        )
        require(preset["configurePreset"] == name, f"{name} build/configure mismatch")
        require(
            type(preset["jobs"]) is int and preset["jobs"] > 0, f"{name} jobs is open"
        )
    for name in ("cuda-release", "cuda-audit"):
        preset = presets[name]
        require_keys(
            preset,
            {"name", "configurePreset", "targets", "jobs"},
            f"build preset {name}",
        )
        require(preset["configurePreset"] == name, f"{name} build/configure mismatch")
        require(
            preset["targets"] == CUDA_TARGETS,
            f"{name} does not compile the frozen Phase 3 target catalog",
        )
        require(
            type(preset["jobs"]) is int and preset["jobs"] == CUDA_BUILD_JOBS,
            f"{name} jobs must stay fixed at {CUDA_BUILD_JOBS}",
        )


def validate_test_presets(presets: dict[str, dict[str, Any]]) -> None:
    cpu = presets["cpu-release"]
    require_keys(
        cpu, {"name", "configurePreset", "output", "execution"}, "CPU test preset"
    )
    require(cpu["configurePreset"] == "cpu-release", "CPU test preset is detached")
    sanitizer = presets["sanitizer"]
    require_keys(
        sanitizer,
        {"name", "configurePreset", "environment", "output", "execution"},
        "sanitizer test preset",
    )
    require(
        sanitizer["configurePreset"] == "sanitizer", "sanitizer test preset is detached"
    )
    require(
        sanitizer["environment"]
        == {
            "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
        },
        "sanitizer runtime policy is not closed",
    )
    for name, preset in presets.items():
        require(preset["output"] == {"outputOnFailure": True}, f"{name} hides failures")
        require(
            preset["execution"] == {"stopOnFailure": True},
            f"{name} continues after failure",
        )


def validate_workflows(presets: dict[str, dict[str, Any]]) -> None:
    expected = {
        "cpu-release": [
            ("configure", "cpu-release"),
            ("build", "cpu-release"),
            ("test", "cpu-release"),
        ],
        "sanitizer": [
            ("configure", "sanitizer"),
            ("build", "sanitizer"),
            ("test", "sanitizer"),
        ],
        "cuda-release": [("configure", "cuda-release"), ("build", "cuda-release")],
        "cuda-audit": [("configure", "cuda-audit"), ("build", "cuda-audit")],
    }
    for name, expected_steps in expected.items():
        preset = presets[name]
        require_keys(preset, {"name", "steps"}, f"workflow {name}")
        require(
            isinstance(preset["steps"], list), f"workflow {name} steps must be an array"
        )
        actual_steps: list[tuple[Any, Any]] = []
        for index, step in enumerate(preset["steps"]):
            require(
                isinstance(step, dict), f"workflow {name} step {index} is not an object"
            )
            require_keys(step, {"type", "name"}, f"workflow {name} step {index}")
            actual_steps.append((step["type"], step["name"]))
        require(actual_steps == expected_steps, f"workflow {name} is not frozen")


def validate_presets(value: dict[str, Any]) -> None:
    require_keys(
        value,
        {
            "version",
            "cmakeMinimumRequired",
            "configurePresets",
            "buildPresets",
            "testPresets",
            "workflowPresets",
        },
        "CMakePresets.json",
    )
    require(
        type(value["version"]) is int and value["version"] == 6,
        "preset schema must be 6",
    )
    require(
        value["cmakeMinimumRequired"] == {"major": 3, "minor": 25, "patch": 0},
        "preset CMake floor must match the project",
    )
    serialized = json.dumps(value, sort_keys=True)
    for forbidden in (
        "use_fast_math",
        "-gencode",
        "--generate-code",
        "--gpu-architecture",
        "--gpu-code",
        "--ptx",
        "--options-file",
        "-optf",
    ):
        require(
            forbidden not in serialized,
            f"a preset contains forbidden CUDA flag {forbidden}",
        )
    configure = preset_map(
        value["configurePresets"], CONFIGURE_NAMES, "configure presets"
    )
    build = preset_map(value["buildPresets"], PUBLIC_NAMES, "build presets")
    tests = preset_map(
        value["testPresets"], {"cpu-release", "sanitizer"}, "test presets"
    )
    workflows = preset_map(value["workflowPresets"], PUBLIC_NAMES, "workflow presets")
    validate_configure_presets(configure)
    validate_build_presets(build)
    validate_test_presets(tests)
    validate_workflows(workflows)


def expect_rejected(
    value: dict[str, Any], mutate: Callable[[dict[str, Any]], None], label: str
) -> None:
    candidate = copy.deepcopy(value)
    mutate(candidate)
    try:
        validate_presets(candidate)
    except ContractError:
        return
    raise ContractError(f"negative preset mutation was accepted: {label}")


def named_preset(value: dict[str, Any], section: str, name: str) -> dict[str, Any]:
    return next(preset for preset in value[section] if preset["name"] == name)


def validate_negative_mutations(value: dict[str, Any]) -> None:
    expect_rejected(
        value,
        lambda candidate: named_preset(candidate, "configurePresets", "cuda-release")[
            "cacheVariables"
        ].__setitem__("CMAKE_CUDA_ARCHITECTURES", "120"),
        "PTX-capable architecture",
    )
    expect_rejected(
        value,
        lambda candidate: named_preset(candidate, "configurePresets", "cuda-release")[
            "cacheVariables"
        ].__setitem__("CMAKE_CUDA_FLAGS", "--use_fast_math"),
        "CUDA fast math",
    )
    expect_rejected(
        value,
        lambda candidate: named_preset(candidate, "configurePresets", "cuda-release")[
            "cacheVariables"
        ].__setitem__(
            "CMAKE_CUDA_FLAGS",
            "-gencode=arch=compute_120,code=compute_120",
        ),
        "raw PTX architecture",
    )
    expect_rejected(
        value,
        lambda candidate: named_preset(candidate, "configurePresets", "phase3-base")[
            "environment"
        ].__setitem__("NVCC_APPEND_FLAGS", "--use_fast_math"),
        "NVCC environment injection",
    )
    expect_rejected(
        value,
        lambda candidate: named_preset(candidate, "configurePresets", "phase3-base")[
            "environment"
        ].__setitem__("CUDAFLAGS", "--options-file=hidden-flags"),
        "CUDAFLAGS options-file injection",
    )
    expect_rejected(
        value,
        lambda candidate: named_preset(candidate, "configurePresets", "sanitizer")[
            "cacheVariables"
        ].__setitem__("MORSEHGP3D_ENABLE_CUDA", "ON"),
        "CUDA sanitizer profile",
    )
    for name in ("cuda-release", "cuda-audit"):
        expect_rejected(
            value,
            lambda candidate, preset_name=name: named_preset(
                candidate, "buildPresets", preset_name
            ).__setitem__("jobs", 2),
            f"{name} build parallelism",
        )
    expect_rejected(
        value,
        lambda candidate: named_preset(candidate, "workflowPresets", "cuda-audit")[
            "steps"
        ].append({"type": "test", "name": "cpu-release"}),
        "executed CUDA probe",
    )


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        raise ContractError(f"cannot read {path}: {error}") from error


def validate_sources(project: Path, repository: Path) -> None:
    cmake = read_text(project / "CMakeLists.txt")
    cuda_module = read_text(project / "cmake/MorseHGP3DCuda.cmake")
    probe = read_text(project / "src/cuda/environment_probe.cu")
    unit_cmake = read_text(project / "tests/unit/CMakeLists.txt")
    dockerfile = read_text(repository / "containers/cuda12.9-sm120.Dockerfile")

    require(
        re.search(r"option\(\s*MORSEHGP3D_ENABLE_CUDA\b.*?\bOFF\s*\)", cmake, re.DOTALL)
        is not None,
        "CUDA is not opt-in OFF",
    )
    require(
        "MORSEHGP3D_ENABLE_SANITIZERS AND MORSEHGP3D_ENABLE_CUDA" in cmake,
        "sanitizer and CUDA are not mutually exclusive",
    )
    for token in (
        "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer",
        "target_link_options",
        "OBJECT",
        "EXCLUDE_FROM_ALL",
        "src/cuda/environment_probe.cu",
        "morsehgp3d_configure_cuda_target",
        "morsehgp3d_apply_sanitizers(morsehgp3d_replay_predicate)",
        "MorseHGP3DPhase3Dependencies.cmake",
        "src/cuda/phase3_runtime.cu",
        "src/python/phase3_module.cpp",
        "MORSEHGP3D_PHASE3_GIT_SHA",
        "safe.directory=${MORSEHGP3D_PHASE3_REPOSITORY_ROOT}",
        "status --porcelain --untracked-files=normal",
        "morsehgp3d_phase3_runtime",
        "nanobind_add_module",
    ):
        require(token in cmake, f"root CMake is missing {token}")

    cuda_host_policy = f"{cmake}\n{cuda_module}"
    for forbidden in ("-Xcompiler", "--compiler-options"):
        require(
            forbidden not in cuda_host_policy,
            "CUDA targets must not forward host compiler options to "
            f"nvcc-generated code: {forbidden}",
        )
    for target in ("morsehgp3d_phase3", "morsehgp3d_replay_predicate"):
        native_warning_blocks = re.findall(
            rf"target_compile_options\(\s*{target}\s+PRIVATE(.*?)\)",
            cmake,
            re.DOTALL,
        )
        require(
            any("-Werror" in block for block in native_warning_blocks),
            f"{target} lost the strict native C++ warning policy",
        )

    for token in (
        "check_language(CUDA)",
        "enable_language(CUDA)",
        'CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA"',
        'CMAKE_CUDA_COMPILER_VERSION MATCHES "^12[.]9([.]|$)"',
        'CUDA_ARCHITECTURES "120-real"',
        "CMAKE_CUDA_FLAGS_DEBUG",
        "CMAKE_CUDA_FLAGS_RELEASE",
        "CMAKE_CUDA_FLAGS_RELWITHDEBINFO",
        "CMAKE_CUDA_FLAGS_MINSIZEREL",
        "CUDAFLAGS",
        "NVCC_PREPEND_FLAGS",
        "NVCC_APPEND_FLAGS",
        "CUDA_COMPILER_LAUNCHER",
        "-gencode|--generate-code|-arch|--gpu-architecture|-code|--gpu-code|-ptx|--ptx",
        "@[^ \\t;>]+|--options-file|-optf",
        "_morsehgp3d_reject_cuda_options(",
        "_morsehgp3d_validate_global_cuda_flags()",
    ):
        require(token in cuda_module, f"CUDA CMake guard is missing {token}")
    require("120-virtual" not in cuda_module, "the CUDA module permits virtual PTX")
    require("--use_fast_math" not in cuda_module, "the CUDA module adds fast math")
    require("-use_fast_math" not in cuda_module, "the CUDA module adds fast math")

    require("__global__" in probe, "the compile-only source has no CUDA kernel")
    for forbidden in (
        "int main",
        "void main",
        "<<<",
        "cudaLaunch",
        "cudaDeviceSynchronize",
    ):
        require(
            forbidden not in probe, f"the compile-only probe executes CUDA: {forbidden}"
        )
    require(
        re.search(
            rf"add_test\([^)]*\b{CUDA_TARGETS[0]}\b",
            cmake + unit_cmake,
            re.DOTALL,
        )
        is None,
        "the CUDA compile-only target is registered as a test",
    )
    require(
        re.search(rf"install\([^)]*\b{CUDA_TARGETS[0]}\b", cmake, re.DOTALL) is None,
        "the CUDA compile-only target is installed",
    )
    require(
        "../configuration/check_phase3_build.py" in unit_cmake,
        "the Phase 3.1 static check is not registered in CTest",
    )
    sanitizer_targets = {
        "morsehgp3d_exact_types_tests",
        "morsehgp3d_exact_types_dump",
        "morsehgp3d_predicates_tests",
        "morsehgp3d_affine_tests",
        "morsehgp3d_centers_tests",
        "morsehgp3d_support_tests",
        "morsehgp3d_level_order_tests",
        "morsehgp3d_expansion_tests",
    }
    sanitizer_block_match = re.search(
        r"set\(\s*_morsehgp3d_cpu_test_targets(?P<targets>.*?)\)\s*foreach",
        unit_cmake,
        re.DOTALL,
    )
    require(sanitizer_block_match is not None, "the sanitizer target catalog is absent")
    actual_sanitizer_targets = set(sanitizer_block_match.group("targets").split())
    require(
        actual_sanitizer_targets == sanitizer_targets,
        "the sanitizer target catalog is not closed",
    )
    require(
        'morsehgp3d_apply_sanitizers("${_morsehgp3d_cpu_test_target}")' in unit_cmake,
        "the sanitizer target catalog is not instrumented",
    )

    docker_lines = [line.strip() for line in dockerfile.splitlines() if line.strip()]
    require(
        docker_lines[0] == f"FROM {CUDA_IMAGE}",
        "the CUDA build image is not digest-pinned",
    )
    require(
        f'ENV MORSEHGP3D_CUDA_IMAGE_REF="{CUDA_IMAGE}"' in dockerfile,
        "image ref is not exported",
    )
    require(
        'ENV CUDAARCHS="120-real"' in dockerfile,
        "container architecture is not real-only",
    )
    require(
        'CMD ["cmake", "--workflow", "--preset", "cuda-release"]' in dockerfile,
        "container default is not the compile-only workflow",
    )


def main(arguments: list[str]) -> int:
    if len(arguments) != 3:
        print("usage: check_phase3_build.py PROJECT_SOURCE REPOSITORY", file=sys.stderr)
        return 2
    project = Path(arguments[1]).resolve()
    repository = Path(arguments[2]).resolve()
    try:
        presets = strict_json(project / "CMakePresets.json")
        validate_presets(presets)
        validate_negative_mutations(presets)
        validate_sources(project, repository)
    except (ContractError, StopIteration) as error:
        print(f"Phase 3.1 build contract failed: {error}", file=sys.stderr)
        return 1
    print(
        json.dumps(
            {
                "cuda_architecture": "120-real",
                "cuda_runtime_executed": False,
                "cuda_targets": CUDA_TARGETS,
                "negative_mutations": 9,
                "public_presets": sorted(PUBLIC_NAMES),
                "schema_version": 6,
            },
            separators=(",", ":"),
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
