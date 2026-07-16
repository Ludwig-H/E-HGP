#!/usr/bin/env python3
"""Validate the Phase 3 runtime contract without requiring a CUDA device."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


class ContractError(ValueError):
    """A frozen Phase 3 runtime invariant was violated."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        raise ContractError(f"cannot read {path}: {error}") from error


def require_tokens(text: str, tokens: tuple[str, ...], label: str) -> None:
    for token in tokens:
        require(token in text, f"{label} is missing {token}")


def validate_runtime(project: Path) -> None:
    runtime = read_text(project / "src/cuda/phase3_runtime.cu")
    require_tokens(
        runtime,
        (
            '#error "phase3_runtime.cu must be compiled ahead of time with NVCC"',
            "__CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9",
            "__CUDA_ARCH__ != 1200",
            "__FAST_MATH__",
            "cudaMallocAsync",
            "cudaFreeAsync",
            "cudaMemPoolTrimTo",
            "cub::DeviceReduce::Sum",
            "DLTensor tensor",
            "std::int64_t dlpack_strides[1] = {1}",
            "tensor.strides = dlpack_strides",
            "kDLCUDA",
            "nvtxRangePushA",
            'measure("warm"',
            'measure("resident"',
            'add_bool("compilation_during_measurement", false)',
            'add_bool("nvrtc_used", false)',
            'add_string("image_id"',
            'add_string("image_ref"',
            'add_string("git_sha"',
            'add_integer("clock_rate_khz"',
            'add_integer("memory_clock_rate_khz"',
            'add_integer("live_bytes_final"',
            'add_bool("zero_copy"',
            'add_bool("bitwise_equal"',
            'add_bool("cpu_reference_equal"',
            "phase3_cpu_reference_value",
            "cudaGetErrorName",
            "cudaGetErrorString",
            "int main(int argc, char** argv)",
        ),
        "CUDA runtime",
    )
    for forbidden in (
        "#include <nvrtc",
        "nvrtcCompileProgram",
        "cudaRuntimeCompile",
        "std::system(",
        "popen(",
        "dlopen(",
        "tensor.strides = nullptr",
    ):
        require(
            forbidden not in runtime, f"CUDA runtime contains forbidden {forbidden}"
        )
    require(
        len(re.findall(r"check_cuda\(cudaMallocAsync\s*\(", runtime)) == 1,
        "the runtime must issue exactly one asynchronous arena allocation",
    )
    require(
        'memory.allocate(options.allocation_ceiling, "single Phase 3 arena")'
        in runtime,
        "the asynchronous arena is not allocated at the configured ceiling",
    )
    require(
        runtime.index("allocations_.reserve(allocations_.size() + 1U)")
        < runtime.index('std::string operation = "cudaMallocAsync("')
        < runtime.index("cudaMallocAsync(&pointer, bytes, stream_)"),
        "host bookkeeping and diagnostics are not prepared before cudaMallocAsync",
    )
    require(
        runtime.index('std::string operation = "cudaFreeAsync("')
        < runtime.index("cudaFreeAsync(pointer, stream_)"),
        "the cudaFreeAsync diagnostic is not prepared before the device mutation",
    )
    require(
        "evidence.peak_live_bytes != options.allocation_ceiling" in runtime,
        "the exact allocation ceiling is not an enforced success invariant",
    )


def validate_dependencies_and_binding(project: Path) -> None:
    dependencies = read_text(project / "cmake/MorseHGP3DPhase3Dependencies.cmake")
    require_tokens(
        dependencies,
        (
            "84d107bf416c6bab9ae68ad285876600d230490d",
            "2a61ad2494d09fecb2e13322c1383342c299900d",
            "4ec1bf19c6a96125ea22062f38c2cf5b958e448e",
            "https://github.com/dmlc/dlpack.git",
            "https://github.com/wjakob/nanobind.git",
            "SOURCE_SUBDIR morsehgp3d-no-add-subdirectory",
            "GIT_SUBMODULES ext/robin_map",
            "_morsehgp3d_phase3_require_git_revision",
            "status --porcelain=v1",
            "--untracked-files=all --ignore-submodules=none",
            "find_package(CUDAToolkit 12.9 REQUIRED)",
            "CUDA::cudart",
            "CUDA::nvtx3",
            "cub/version.cuh",
            "morsehgp3d::phase3_dependencies",
        ),
        "Phase 3 dependency module",
    )
    require(
        "github.com/NVIDIA/cccl" not in dependencies,
        "CCCL must come from the selected CUDA toolkit rather than FetchContent",
    )

    binding = read_text(project / "src/python/phase3_module.cpp")
    require_tokens(
        binding,
        (
            "NB_MODULE(morsehgp3d_phase3, module)",
            'module.def(\n      "environment"',
            'module.def(\n      "make_dlpack_capsule"',
            'module.def(\n      "consume_dlpack_capsule"',
            'module.def(\n      "dlpack_zero_copy_probe"',
            "cudaMallocAsync",
            "cudaFreeAsync",
            "cudaMemPoolTrimTo",
            "DLManagedTensorVersioned",
            '"dltensor_versioned"',
            '"used_dltensor_versioned"',
            "DLPACK_FLAG_BITMASK_IS_COPIED",
            "exported_dlpack_tensors.find",
            "std::unordered_map<DLManagedTensorVersioned*, DLPackCapsuleContext*>",
            "PyCapsule_New",
            "PyCapsule_SetContext",
            "PyCapsule_GetContext",
            "PyCapsule_IsValid",
            "capsule_context != registered_context",
            "std::unique_ptr<DLPackCapsuleContext",
            "DLTensor tensor",
            "tensor.data == allocation.get()",
            'result["live_device_bytes_after"] = live_after',
            'result["scientific_result_claimed"] = false',
            'result["scientific_public_status"] = nb::none()',
        ),
        "Phase 3 Python binding",
    )
    binding_test = read_text(project / "tests/cuda/check_phase3_binding.py")
    require(
        "std::unordered_map<DLManagedTensorVersioned*, PyObject*>" not in binding,
        "the DLPack registry must not retain a reusable raw CPython address",
    )
    require_tokens(
        binding_test,
        (
            'MODULE_NAME = "morsehgp3d_phase3"',
            "module.make_dlpack_capsule(PROBE_BYTES)",
            "module.consume_dlpack_capsule(capsule)",
            '"dltensor_versioned"',
            '"used_dltensor_versioned"',
            'gpu["compute_capability"] != [12, 0]',
            'probe["zero_copy"]',
            '"live_device_bytes_after"',
            "expect_invalid_call",
            "validate_external_consumer_lifetime",
            "alias_capsule",
            "capsule_get_context",
            "ownership_context",
            "expect_invalid_capsule_size",
            "module.dlpack_zero_copy_probe(PROBE_BYTES)",
        ),
        "Phase 3 Python binding test",
    )


def validate_build(project: Path, repository: Path) -> None:
    cmake = read_text(project / "CMakeLists.txt")
    cuda_module = read_text(project / "cmake/MorseHGP3DCuda.cmake")
    presets = read_text(project / "CMakePresets.json")
    dockerfile = read_text(repository / "containers/cuda12.9-sm120.Dockerfile")
    require_tokens(
        cmake,
        (
            "safe.directory=${MORSEHGP3D_PHASE3_REPOSITORY_ROOT}",
            "status --porcelain --untracked-files=normal",
            "MORSEHGP3D_PHASE3_GIT_SHA",
            "MorseHGP3DPhase3Dependencies.cmake",
            "add_executable(\n    morsehgp3d_phase3_runtime",
            "nanobind_add_module(\n    morsehgp3d_phase3",
            "NB_SUPPRESS_WARNINGS",
            "morsehgp3d::phase3_dependencies",
            "MORSEHGP3D_CUDA_AUDIT=$<IF:",
        ),
        "root CMake",
    )
    require_tokens(
        cuda_module,
        (
            "--fmad=false",
            "--ftz=false",
            "--prec-div=true",
            "--prec-sqrt=true",
            'CUDA_ARCHITECTURES "120-real"',
        ),
        "CUDA target policy",
    )
    for target in (
        "morsehgp3d_cuda_environment_probe",
        "morsehgp3d_phase3_runtime",
        "morsehgp3d_phase3",
    ):
        require(
            presets.count(f'"{target}"') == 2, f"{target} is not in both CUDA builds"
        )
    require(
        'ENV CUDA_MODULE_LOADING="EAGER"' in dockerfile,
        "the image does not force eager CUDA module loading",
    )


def main(arguments: list[str]) -> int:
    if len(arguments) != 3:
        print(
            "usage: check_phase3_runtime.py PROJECT_SOURCE REPOSITORY", file=sys.stderr
        )
        return 2
    project = Path(arguments[1]).resolve()
    repository = Path(arguments[2]).resolve()
    try:
        validate_runtime(project)
        validate_dependencies_and_binding(project)
        validate_build(project, repository)
    except ContractError as error:
        print(f"Phase 3 runtime contract failed: {error}", file=sys.stderr)
        return 1
    print(
        json.dumps(
            {
                "allocation_model": "single_async_arena",
                "cuda_runtime_executed": False,
                "dependencies": ["cccl_cub", "dlpack", "nanobind", "nvtx3"],
                "protocols": ["resident", "warm"],
                "schema": "morsehgp3d.phase3.runtime.v1",
            },
            separators=(",", ":"),
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
