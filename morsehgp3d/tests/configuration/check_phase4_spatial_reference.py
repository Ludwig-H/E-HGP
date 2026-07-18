#!/usr/bin/env python3
"""Validate the static contract of the Phase 4 CUDA spatial reference."""

from __future__ import annotations

import copy
import json
import re
import sys
from pathlib import Path
from typing import Any, Callable


CUDA_TARGET = "morsehgp3d_gpu_spatial_reference"
CUDA_REPLAY_TARGET = "morsehgp3d_gpu_spatial_reference_replay"
CUDA_PHASE4_TARGETS = (CUDA_TARGET, CUDA_REPLAY_TARGET)
CUDA_PRESETS = ("cuda-release", "cuda-audit")
CPU_PRESETS = ("cpu-release", "sanitizer")


class ContractError(ValueError):
    """A Phase 4 spatial-reference invariant was violated."""


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
        require(token in text, f"{label} is missing {token!r}")


def require_order(text: str, tokens: tuple[str, ...], label: str) -> None:
    position = -1
    for token in tokens:
        next_position = text.find(token, position + 1)
        require(next_position >= 0, f"{label} is missing ordered token {token!r}")
        require(next_position > position, f"{label} has a stale order assertion")
        position = next_position


def extract_braced_block(text: str, marker: str, label: str) -> str:
    marker_position = text.find(marker)
    require(marker_position >= 0, f"{label} marker is missing: {marker!r}")
    opening = text.find("{", marker_position + len(marker))
    require(opening >= 0, f"{label} has no opening brace")
    depth = 0
    for position in range(opening, len(text)):
        character = text[position]
        if character == "{":
            depth += 1
        elif character == "}":
            depth -= 1
            if depth == 0:
                return text[opening : position + 1]
    raise ContractError(f"{label} has no closing brace")


def strict_json(path: Path) -> dict[str, Any]:
    def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise ContractError(f"duplicate JSON key in {path}: {key}")
            result[key] = value
        return result

    try:
        value = json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=reject_duplicate_keys,
        )
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise ContractError(f"cannot parse {path}: {error}") from error
    require(isinstance(value, dict), f"{path} must contain one JSON object")
    return value


def named_presets(value: object, section: str) -> dict[str, dict[str, Any]]:
    require(isinstance(value, list), f"{section} must be an array")
    result: dict[str, dict[str, Any]] = {}
    for index, preset in enumerate(value):
        require(isinstance(preset, dict), f"{section}[{index}] must be an object")
        name = preset.get("name")
        require(type(name) is str and bool(name), f"{section}[{index}] has no name")
        require(name not in result, f"duplicate {section} preset {name}")
        result[name] = preset
    return result


def validate_public_api(header: str) -> None:
    require_tokens(
        header,
        (
            '#include "morsehgp3d/spatial/brute_force.hpp"',
            "enum class QueryCoordinateProjection",
            "exact,",
            "rounded,",
            "underflow,",
            "overflow_clamped,",
            "struct SpatialReferenceAudit",
            '"non_certifying_fp64"',
            '"cpu_exact_all_points"',
            "gpu_input_point_count",
            "gpu_output_record_count",
            "gpu_unique_point_id_count",
            "gpu_finite_distance_proposal_count",
            "gpu_infinite_distance_proposal_count",
            "gpu_nan_distance_proposal_count",
            "gpu_launch_count",
            "cpu_exact_distance_evaluation_count",
            "buffer_epoch",
            "proposal_digest_fnv1a",
            "projected_query_bits",
            "query_projection",
            "all_points_enumerated",
            "cpu_exact_recertification_complete",
            "struct SpatialReferenceTopKResult",
            "spatial::TopKPartition exact_partition",
            "struct SpatialReferenceClosedBallResult",
            "spatial::ClosedBallPartition exact_partition",
            "class SpatialReferenceContext final",
            "explicit SpatialReferenceContext(",
            "SpatialReferenceContext(SpatialReferenceContext&&) noexcept",
            "SpatialReferenceContext& operator=(SpatialReferenceContext&&) noexcept",
            "SpatialReferenceContext(const SpatialReferenceContext&) = delete",
            "SpatialReferenceContext& operator=(const SpatialReferenceContext&) = delete",
            "SpatialReferenceTopKResult top_k(",
            "SpatialReferenceTopKResult nearest(",
            "SpatialReferenceClosedBallResult closed_ball(",
            "std::shared_ptr<const void> cloud_identity_",
        ),
        "Phase 4 spatial-reference API",
    )
    for forbidden in (
        "SpatialProposalRecord",
        "squared_distance_bits",
        "public_status",
        "gpu_exact",
        "exact_gpu",
    ):
        require(
            forbidden not in header,
            f"the public API leaks or overstates the FP64 proposal via {forbidden!r}",
        )


def validate_host(host: str) -> None:
    require_tokens(
        host,
        (
            '#include "morsehgp3d/gpu/spatial_reference.hpp"',
            '#include "phase4_spatial_reference_internal.hpp"',
            "project_nonnegative_rational",
            "positive_binary64_rational",
            "const ExactRational halfway =",
            "(lower_bits & 1U) == 0U ? lower_bits : adjacent_bits",
            "QueryCoordinateProjection::rounded",
            "QueryCoordinateProjection::exact",
            "QueryCoordinateProjection::underflow",
            "QueryCoordinateProjection::overflow_clamped",
            "validated_query",
            "validated_radius",
            "validate_proposal_batch",
            "kFnvOffsetBasis",
            "kFnvPrime",
            "audit.gpu_input_point_count = point_count",
            "audit.gpu_output_record_count = batch.records.size()",
            "audit.gpu_launch_count = 1U",
            "batch.records.size() != point_count",
            "std::vector<unsigned char> seen(point_count, 0U)",
            "std::vector<std::uint64_t> distance_bits_by_id(point_count, 0U)",
            "record.point_id",
            "seen[index] != 0U",
            "++audit.gpu_unique_point_id_count",
            "record.squared_distance_bits",
            "gpu_nan_distance_proposal_count",
            "gpu_infinite_distance_proposal_count",
            "gpu_finite_distance_proposal_count",
            "audit.gpu_nan_distance_proposal_count != 0U",
            "audit.gpu_unique_point_id_count != point_count",
            "audit.all_points_enumerated = true",
            "audit.proposal_digest_fnv1a = digest",
            "cloud.point(id).canonical_input_bits()",
            "coordinate_bits_[axis * point_count_ + index]",
            "cloud.identity_ != cloud_identity_",
            "return state_->with_gpu_section([&]",
            "detail::propose_squared_distances_on_gpu",
            "point_count_ - exclusions.ids().size()",
            "spatial::brute_force_top_k",
            "partition.distance_evaluation_count()",
            "audit.cpu_exact_distance_evaluation_count != eligible_point_count",
            "spatial::brute_force_closed_ball",
            "audit.cpu_exact_distance_evaluation_count != point_count_",
            "audit.cpu_exact_recertification_complete = true",
        ),
        "Phase 4 spatial-reference host",
    )

    proposal_validation = extract_braced_block(
        host,
        "SpatialReferenceAudit validate_proposal_batch(",
        "proposal-batch validation",
    )
    require_order(
        proposal_validation,
        (
            "batch.records.size() != point_count",
            "std::vector<unsigned char> seen(point_count, 0U)",
            "for (const detail::SpatialProposalRecord& record : batch.records)",
            "audit.gpu_nan_distance_proposal_count != 0U",
            "audit.gpu_unique_point_id_count != point_count",
            "audit.all_points_enumerated = true",
        ),
        "proposal validation must close before completeness is published",
    )
    require(
        host.count("record.squared_distance_bits")
        == proposal_validation.count("record.squared_distance_bits"),
        "FP64 proposal bits escaped validation and diagnostic hashing",
    )

    top_k = extract_braced_block(
        host,
        "SpatialReferenceTopKResult SpatialReferenceContext::top_k(",
        "GPU reference top-k",
    )
    require_order(
        top_k,
        (
            "const ExactRational3 canonical_query = validated_query(query)",
            "SpatialReferenceAudit audit = run_proposal(canonical_query)",
            "spatial::TopKPartition partition = spatial::brute_force_top_k(",
            "partition.distance_evaluation_count()",
            "audit.cpu_exact_distance_evaluation_count != eligible_point_count",
            "audit.cpu_exact_recertification_complete = true",
            "return SpatialReferenceTopKResult",
        ),
        "top-k proposal/recertification order",
    )
    closed_ball = extract_braced_block(
        host,
        "SpatialReferenceClosedBallResult SpatialReferenceContext::closed_ball(",
        "GPU reference closed ball",
    )
    require_order(
        closed_ball,
        (
            "const ExactRational3 canonical_query = validated_query(query)",
            "const exact::ExactLevel canonical_radius = validated_radius(squared_radius)",
            "SpatialReferenceAudit audit = run_proposal(canonical_query)",
            "spatial::ClosedBallPartition partition = spatial::brute_force_closed_ball(",
            "partition.distance_evaluation_count()",
            "audit.cpu_exact_distance_evaluation_count != point_count_",
            "audit.cpu_exact_recertification_complete = true",
            "return SpatialReferenceClosedBallResult",
        ),
        "closed-ball proposal/recertification order",
    )
    require(
        "record.squared_distance_bits" not in top_k
        and "record.squared_distance_bits" not in closed_ball,
        "an FP64 proposal leaked into a scientific partition decision",
    )


def validate_internal_header(internal: str) -> None:
    require_tokens(
        internal,
        (
            "struct SpatialProposalRecord",
            "std::uint64_t point_id",
            "std::uint64_t squared_distance_bits",
            "sizeof(SpatialProposalRecord) == 2U * sizeof(std::uint64_t)",
            "std::is_trivially_copyable_v<SpatialProposalRecord>",
            "struct SpatialProposalBatch",
            "std::vector<SpatialProposalRecord> records",
            "std::uint64_t buffer_epoch",
            "class SpatialReferenceContextState final",
            "std::lock_guard<std::mutex> lock{mutex_}",
            "poisoned_.load(std::memory_order_acquire)",
            "mark_poisoned()",
            "poisoned_.store(true, std::memory_order_release)",
            "advance_epoch()",
            "std::overflow_error",
            "propose_squared_distances_on_gpu(",
        ),
        "Phase 4 spatial-reference internal contract",
    )


def validate_kernel(kernel: str) -> None:
    require_tokens(
        kernel,
        (
            '#error "phase4_spatial_reference.cu must be compiled ahead of time with NVCC"',
            "__CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9",
            "defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)",
            "defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0",
            "defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200",
            "constexpr unsigned int kThreadsPerBlock = 256U",
            "#include <cuda/std/bit>",
            "cuda::std::bit_cast<double>",
            "__dsub_rn",
            "__dmul_rn",
            "__dadd_rn",
            "cuda::std::bit_cast<std::uint64_t>",
            "morsehgp3d_phase4_spatial_reference_kernel",
            "static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x",
            "static_cast<std::size_t>(gridDim.x) * blockDim.x",
            "while (index < point_count)",
            "records[index] = SpatialProposalRecord",
            "point_count - index <= stride",
            "index += stride",
            "properties.maxGridSize[0]",
            "required_blocks",
            "std::min(",
            "cuda.maximum_grid_x()",
            "cudaMemsetAsync(",
            "0xff",
            "cudaGetLastError()",
            "cudaMemcpyDeviceToHost",
            "cuda.synchronize()",
            "batch.buffer_epoch = context.advance_epoch()",
        ),
        "Phase 4 CUDA proposal kernel",
    )
    require(
        kernel.count("__dsub_rn") == 1
        and kernel.count("__dmul_rn") == 1
        and kernel.count("__dadd_rn") == 1,
        "the squared-distance recipe must use one explicit RN intrinsic per axis step",
    )
    require_order(
        extract_braced_block(
            kernel,
            "__global__ void morsehgp3d_phase4_spatial_reference_kernel(",
            "Phase 4 CUDA kernel body",
        ),
        (
            "const std::size_t first =",
            "const std::size_t stride =",
            "std::size_t index = first",
            "while (index < point_count)",
            "__dsub_rn",
            "__dmul_rn",
            "__dadd_rn",
            "records[index] = SpatialProposalRecord",
            "point_count - index <= stride",
            "index += stride",
        ),
        "grid-stride exhaustive kernel",
    )
    for forbidden in (
        "__fma",
        "fma(",
        "sqrt(",
        "rsqrt(",
        "pow(",
        "thrust::sort",
        "cub::DeviceRadixSort",
    ):
        require(
            forbidden not in kernel,
            f"the non-certifying proposal kernel contains forbidden {forbidden!r}",
        )


def validate_no_scientific_shortcuts(sources: dict[str, str]) -> None:
    forbidden_patterns = (
        r"\bepsilon\b",
        r"\btoleranc(?:e|es)\b",
        r"\bprun(?:e|ed|es|ing)\b",
        r"\bmax(?:imum)?_visits\b",
        r"\bvisit_(?:cap|limit|budget)\b",
        r"\bcandidate_(?:cap|limit|budget)\b",
        r"\bearly_stop\b",
        r"\bapproximate_cutoff\b",
        r"\bnextafter\b",
        r"\bcuvs\b",
        r"\braft\b",
    )
    for label, source in sources.items():
        for pattern in forbidden_patterns:
            require(
                re.search(pattern, source, re.IGNORECASE) is None,
                f"{label} contains forbidden spatial shortcut matching {pattern!r}",
            )


def validate_point_namespace(point_cloud_header: str) -> None:
    require_tokens(
        point_cloud_header,
        (
            "namespace morsehgp3d::gpu",
            "class SpatialReferenceContext;",
            "friend class gpu::SpatialReferenceContext;",
        ),
        "canonical PointId namespace",
    )


def validate_cmake(cmake: str, cuda_policy: str) -> None:
    target_match = re.search(
        r"add_library\(\s*"
        + re.escape(CUDA_TARGET)
        + r"\s+STATIC\s+"
        r"src/cuda/phase4_spatial_reference_internal[.]hpp\s+"
        r"src/cuda/phase4_spatial_reference[.]cu\s+"
        r"src/gpu/spatial_reference[.]cpp\s*\)",
        cmake,
        re.DOTALL,
    )
    require(target_match is not None, "the Phase 4 CUDA static target is incomplete")
    require_tokens(
        cmake,
        (
            "if(MORSEHGP3D_ENABLE_CUDA)",
            f"add_library(\n    morsehgp3d::gpu_spatial_reference\n    ALIAS {CUDA_TARGET}",
            f"target_include_directories(\n    {CUDA_TARGET}",
            'PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"',
            f"target_link_libraries(\n    {CUDA_TARGET}",
            "PUBLIC morsehgp3d::spatial",
            "PRIVATE Threads::Threads",
            f"target_compile_features(\n    {CUDA_TARGET}\n    PUBLIC cxx_std_20",
            f"morsehgp3d_configure_cuda_target({CUDA_TARGET})",
            (
                f"add_executable(\n    {CUDA_REPLAY_TARGET}\n"
                "    src/tools/gpu_spatial_reference_replay.cpp"
            ),
            (
                f"target_link_libraries(\n    {CUDA_REPLAY_TARGET}\n"
                "    PRIVATE morsehgp3d::gpu_spatial_reference"
            ),
            'PATTERN "morsehgp3d/gpu" EXCLUDE',
        ),
        "Phase 4 CUDA CMake target",
    )
    target_position = target_match.start()
    cuda_block_position = cmake.rfind(
        "if(MORSEHGP3D_ENABLE_CUDA)", 0, target_position
    )
    require(cuda_block_position >= 0, "the Phase 4 target is not CUDA-gated")
    require(
        cmake.find("if(MORSEHGP3D_BUILD_TOOLS)", target_position) > target_position,
        "the Phase 4 target escaped the expected CUDA-only section",
    )
    for install_match in re.finditer(r"install\(\s*TARGETS\b.*?\)", cmake, re.DOTALL):
        for target in CUDA_PHASE4_TARGETS:
            require(
                target not in install_match.group(0),
                "the internal Phase 4 GPU reference must not be exported as a stable target",
            )
    require_tokens(
        cuda_policy,
        (
            'set(_morsehgp3d_cuda_architecture "120-real")',
            'CUDA_ARCHITECTURES "120-real"',
            "$<$<COMPILE_LANGUAGE:CUDA>:--fmad=false>",
            "$<$<COMPILE_LANGUAGE:CUDA>:--ftz=false>",
            "$<$<COMPILE_LANGUAGE:CUDA>:--prec-div=true>",
            "$<$<COMPILE_LANGUAGE:CUDA>:--prec-sqrt=true>",
            "use_fast_math",
            "--generate-code",
            "--gpu-architecture",
            "--options-file",
        ),
        "closed CUDA target policy",
    )


def validate_presets(presets: dict[str, Any]) -> None:
    configure = named_presets(presets.get("configurePresets"), "configurePresets")
    builds = named_presets(presets.get("buildPresets"), "buildPresets")
    for name in CUDA_PRESETS:
        require(name in configure, f"missing CUDA configure preset {name}")
        cache = configure[name].get("cacheVariables")
        require(isinstance(cache, dict), f"{name} cacheVariables must be an object")
        require(
            cache.get("MORSEHGP3D_ENABLE_CUDA") == "ON",
            f"{name} does not enable CUDA",
        )
        require(
            cache.get("CMAKE_CUDA_ARCHITECTURES") == "120-real",
            f"{name} does not freeze sm_120 real code",
        )
        require(name in builds, f"missing CUDA build preset {name}")
        targets = builds[name].get("targets")
        require(isinstance(targets, list), f"{name} build targets must be an array")
        for target in CUDA_PHASE4_TARGETS:
            require(
                targets.count(target) == 1,
                f"{name} must build {target} exactly once",
            )
    for name in CPU_PRESETS:
        require(name in builds, f"missing CPU build preset {name}")
        targets = builds[name].get("targets", [])
        require(isinstance(targets, list), f"{name} build targets must be an array")
        for target in CUDA_PHASE4_TARGETS:
            require(
                target not in targets,
                f"CPU-only preset {name} unexpectedly builds {target}",
            )
    serialized = json.dumps(presets, separators=(",", ":"), sort_keys=True)
    for forbidden in (
        "--use_fast_math",
        "-use_fast_math",
        '"CMAKE_CUDA_ARCHITECTURES":"120"',
    ):
        require(forbidden not in serialized, f"presets contain forbidden {forbidden}")


def expect_text_rejected(
    validator: Callable[[str], None],
    text: str,
    old: str,
    new: str,
    label: str,
) -> None:
    require(old in text, f"negative mutation anchor disappeared: {label}")
    candidate = text.replace(old, new, 1)
    try:
        validator(candidate)
    except ContractError:
        return
    raise ContractError(f"negative source mutation was accepted: {label}")


def validate_mutation_sensitivity(
    header: str,
    host: str,
    kernel: str,
    cmake: str,
    cuda_policy: str,
    presets: dict[str, Any],
) -> None:
    expect_text_rejected(
        validate_public_api,
        header,
        '"non_certifying_fp64"',
        '"certified_fp64"',
        "FP64 proposal promoted to a certificate",
    )
    expect_text_rejected(
        validate_public_api,
        header,
        '"cpu_exact_all_points"',
        '"cpu_exact_candidates_only"',
        "CPU recertification narrowed to candidates",
    )
    expect_text_rejected(
        validate_host,
        host,
        "audit.gpu_nan_distance_proposal_count != 0U",
        "false",
        "NaN acceptance",
    )
    expect_text_rejected(
        validate_host,
        host,
        "audit.cpu_exact_distance_evaluation_count != eligible_point_count",
        "audit.cpu_exact_distance_evaluation_count > eligible_point_count",
        "incomplete top-k CPU recertification",
    )
    expect_text_rejected(
        validate_kernel,
        kernel,
        "while (index < point_count)",
        "if (index < point_count)",
        "single-stride kernel",
    )
    expect_text_rejected(
        validate_kernel,
        kernel,
        "__dmul_rn(difference, difference)",
        "difference * difference",
        "implicit multiplication rounding",
    )
    expect_text_rejected(
        validate_kernel,
        kernel,
        "__CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9",
        "false",
        "CUDA version guard removal",
    )

    def validate_mutated_cmake(candidate: str) -> None:
        validate_cmake(candidate, cuda_policy)

    expect_text_rejected(
        validate_mutated_cmake,
        cmake,
        "src/cuda/phase4_spatial_reference.cu",
        "src/cuda/phase4_spatial_reference.cpp",
        "CUDA source removed from target",
    )

    candidate_presets = copy.deepcopy(presets)
    builds = named_presets(
        candidate_presets.get("buildPresets"), "mutated buildPresets"
    )
    targets = builds["cuda-release"].get("targets")
    require(isinstance(targets, list), "mutation target list is missing")
    targets.remove(CUDA_TARGET)
    try:
        validate_presets(candidate_presets)
    except ContractError:
        return
    raise ContractError("negative preset mutation was accepted: missing CUDA target")


def validate_project(project: Path) -> dict[str, object]:
    paths = {
        "api": project / "include/morsehgp3d/gpu/spatial_reference.hpp",
        "host": project / "src/gpu/spatial_reference.cpp",
        "internal": project / "src/cuda/phase4_spatial_reference_internal.hpp",
        "kernel": project / "src/cuda/phase4_spatial_reference.cu",
        "point_cloud": project / "include/morsehgp3d/spatial/point_cloud.hpp",
        "cmake": project / "CMakeLists.txt",
        "cuda_policy": project / "cmake/MorseHGP3DCuda.cmake",
        "presets": project / "CMakePresets.json",
    }
    texts = {
        name: read_text(path)
        for name, path in paths.items()
        if name != "presets"
    }
    presets = strict_json(paths["presets"])

    validate_public_api(texts["api"])
    validate_host(texts["host"])
    validate_internal_header(texts["internal"])
    validate_kernel(texts["kernel"])
    validate_point_namespace(texts["point_cloud"])
    validate_no_scientific_shortcuts(
        {
            "public API": texts["api"],
            "host recertifier": texts["host"],
            "internal state": texts["internal"],
            "CUDA proposal": texts["kernel"],
        }
    )
    validate_cmake(texts["cmake"], texts["cuda_policy"])
    validate_presets(presets)
    validate_mutation_sensitivity(
        texts["api"],
        texts["host"],
        texts["kernel"],
        texts["cmake"],
        texts["cuda_policy"],
        presets,
    )
    return {
        "checked_artifacts": sorted(str(path.relative_to(project)) for path in paths.values()),
        "cpu_decision_semantics": "cpu_exact_all_points",
        "cuda_executed": False,
        "cuda_targets": list(CUDA_PHASE4_TARGETS),
        "proposal_semantics": "non_certifying_fp64",
        "schema": "morsehgp3d.phase4.spatial_reference_static.v1",
    }


def main(arguments: list[str]) -> int:
    if len(arguments) != 2:
        print(
            "usage: check_phase4_spatial_reference.py PROJECT_SOURCE",
            file=sys.stderr,
        )
        return 2
    project = Path(arguments[1]).resolve()
    try:
        result = validate_project(project)
    except ContractError as error:
        print(f"Phase 4 spatial reference contract failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, separators=(",", ":"), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
