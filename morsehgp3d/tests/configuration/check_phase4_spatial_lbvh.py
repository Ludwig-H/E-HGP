#!/usr/bin/env python3
"""Validate the static contract of the Phase 4 resident CUDA LBVH path."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any


CUDA_LIBRARY_TARGET = "morsehgp3d_gpu_spatial_bounds"
CUDA_REPLAY_TARGET = "morsehgp3d_gpu_spatial_lbvh_replay"
CUDA_PRESETS = ("cuda-release", "cuda-audit")


class ContractError(ValueError):
    """A resident spatial-LBVH source invariant was violated."""


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
            require(key not in result, f"duplicate JSON key in {path}: {key}")
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
            '#include "morsehgp3d/gpu/spatial_bounds.hpp"',
            '#include "morsehgp3d/spatial/lbvh.hpp"',
            "class SpatialLbvhContextState;",
            "class SpatialLbvhHostState;",
            "struct SpatialLbvhAudit",
            '"gpu_resident_lbvh_strict_exterior_cover"',
            '"cpu_exact_cover_and_leaf_recertification"',
            "resident_node_count",
            "resident_point_count",
            "gpu_output_capacity",
            "gpu_output_cover_record_count",
            "gpu_prune_proposal_count",
            "gpu_candidate_leaf_count",
            "gpu_launch_count",
            "traversed_node_count",
            "internal_node_expansion_count",
            "cpu_exact_aabb_bound_evaluation_count",
            "cpu_exact_prune_recertification_count",
            "certified_pruned_subtree_count",
            "certified_pruned_point_count",
            "candidate_point_count",
            "cpu_exact_candidate_distance_evaluation_count",
            "cpu_exact_seed_distance_evaluation_count",
            "excluded_candidate_point_count",
            "certified_pruned_eligible_point_count",
            "unsupported_range_fallback_count",
            "buffer_epoch",
            "proposal_digest_fnv1a",
            "minimum_certified_strict_margin",
            "top_k_seed_squared_cutoff",
            "cover_antichain_complete",
            "point_partition_complete",
            "cpu_exact_recertification_complete",
            "exact_partition_complete",
            "struct SpatialLbvhCoverResult",
            "candidate_point_ids",
            "certified_exterior_point_ids",
            "struct SpatialLbvhClosedBallResult",
            "spatial::ClosedBallPartition exact_partition",
            "struct SpatialLbvhTopKResult",
            "spatial::TopKPartition exact_partition",
            "class SpatialLbvhContext final",
            "SpatialLbvhContext(",
            "SpatialLbvhContext(SpatialLbvhContext&&) noexcept",
            "SpatialLbvhContext& operator=(SpatialLbvhContext&&) noexcept",
            "SpatialLbvhContext(const SpatialLbvhContext&) = delete",
            "SpatialLbvhContext& operator=(const SpatialLbvhContext&) = delete",
            "SpatialLbvhCoverResult cover_strict_exterior(",
            "SpatialLbvhClosedBallResult closed_ball(",
            "SpatialLbvhTopKResult top_k(",
            "SpatialLbvhTopKResult nearest(",
            "std::shared_ptr<detail::SpatialLbvhContextState> state_",
            "std::unique_ptr<detail::SpatialLbvhHostState> host_",
        ),
        "resident spatial-LBVH public API",
    )
    for forbidden in (
        "public_status",
        "gpu_exact",
        "exact_gpu",
        "epsilon",
        "tolerance",
        "maximum_visits",
        "visit_limit",
        "candidate_limit",
    ):
        require(
            forbidden not in header,
            f"the resident LBVH API leaks or overstates {forbidden!r}",
        )


def validate_internal_contract(internal: str) -> None:
    require_tokens(
        internal,
        (
            "spatial_lbvh_cover_prune_code",
            "spatial_lbvh_cover_leaf_code",
            "spatial_lbvh_cover_invalid_code",
            "spatial_bounds_sentinel_code",
            "struct SpatialLbvhNodeInputRecord",
            "SpatialBoundsInputRecord bounds",
            "sizeof(SpatialBoundsInputRecord) == 6U * sizeof(std::uint64_t)",
            "std::is_standard_layout_v<SpatialBoundsInputRecord>",
            "std::uint64_t left_child",
            "std::uint64_t right_child",
            "std::uint64_t leaf_begin",
            "std::uint64_t leaf_end",
            "sizeof(SpatialLbvhNodeInputRecord) == 10U * sizeof(std::uint64_t)",
            "std::is_standard_layout_v<SpatialLbvhNodeInputRecord>",
            "std::is_trivially_copyable_v<SpatialLbvhNodeInputRecord>",
            "struct SpatialLbvhCoverRecord",
            "std::uint64_t node_index",
            "std::uint64_t lower_squared_distance_bits",
            "std::uint64_t upper_squared_distance_bits",
            "std::uint64_t kind",
            "sizeof(SpatialLbvhCoverRecord) == 4U * sizeof(std::uint64_t)",
            "std::is_standard_layout_v<SpatialLbvhCoverRecord>",
            "std::is_trivially_copyable_v<SpatialLbvhCoverRecord>",
            "struct SpatialLbvhCoverBatch",
            "std::vector<SpatialLbvhCoverRecord> records",
            "std::size_t record_count",
            "std::uint64_t buffer_epoch",
            "class SpatialLbvhContextState final",
            "std::lock_guard<std::mutex> lock{mutex_}",
            "poisoned_.load(std::memory_order_acquire)",
            "poisoned_.store(true, std::memory_order_release)",
            "std::shared_ptr<void>& cuda_resources()",
            "advance_epoch()",
            "std::overflow_error",
            "propose_strict_lbvh_cover_on_gpu(",
        ),
        "resident spatial-LBVH internal contract",
    )

    node_record = extract_braced_block(
        internal,
        "struct SpatialLbvhNodeInputRecord",
        "resident LBVH node record",
    )
    cover_record = extract_braced_block(
        internal,
        "struct SpatialLbvhCoverRecord",
        "resident LBVH terminal record",
    )
    for label, record in (
        ("resident node record", node_record),
        ("terminal cover record", cover_record),
    ):
        for forbidden in ("std::vector", "std::string", "std::unique_ptr", "std::shared_ptr"):
            require(forbidden not in record, f"{label} is not a fixed POD record")

    state = extract_braced_block(
        internal,
        "class SpatialLbvhContextState final",
        "poisonable resident LBVH state",
    )
    require_order(
        state,
        (
            "std::lock_guard<std::mutex> lock{mutex_}",
            "poisoned_.load(std::memory_order_acquire)",
            "try",
            "std::forward<Operation>(operation)()",
            "catch (...) ",
            "poisoned_.store(true, std::memory_order_release)",
            "throw;",
        ),
        "poison-on-failure state transition",
    )


def validate_cuda_kernel(kernel: str, cuda_policy: str) -> None:
    require_tokens(
        kernel,
        (
            '#error "phase4_spatial_bounds.cu must be compiled ahead of time with NVCC"',
            "__CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9",
            "defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)",
            "defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0",
            "defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200",
            "__dsub_rd",
            "__dsub_ru",
            "__dmul_rd",
            "__dmul_ru",
            "__dadd_rd",
            "__dadd_ru",
            "class SpatialLbvhCudaResources final",
            "void initialize_nodes(std::span<const SpatialLbvhNodeInputRecord> nodes)",
            "nodes_.allocate(nodes.size()",
            "traversal_stack_.allocate(",
            "nodes.size(), \"cudaMalloc Phase 4 spatial-LBVH traversal stack\"",
            "records_.allocate(",
            "nodes.size(), \"cudaMalloc Phase 4 spatial-LBVH cover records\"",
            "record_count_.allocate(",
            "1U, \"cudaMalloc Phase 4 spatial-LBVH cover count\"",
            "nodes.size() * sizeof(SpatialLbvhNodeInputRecord)",
            "__global__ void morsehgp3d_phase4_spatial_lbvh_cover_kernel(",
            "if (blockIdx.x != 0U || threadIdx.x != 0U)",
            "std::uint64_t stack_size = 1U",
            "std::uint64_t output_count = 0U",
            "traversal_stack[0] = root_index",
            "while (stack_size != 0U)",
            "output_count >= static_cast<std::uint64_t>(node_count)",
            "distance.lower > cutoff_upper",
            "spatial_lbvh_cover_prune_code",
            "spatial_lbvh_cover_leaf_code",
            "spatial_lbvh_cover_invalid_code",
            "stack_size <= static_cast<std::uint64_t>(node_count) - UINT64_C(2)",
            "traversal_stack[stack_size++] = node.right_child",
            "traversal_stack[stack_size++] = node.left_child",
            "*record_count = output_count",
            "cudaMemsetAsync(",
            "nodes.size() * sizeof(SpatialLbvhCoverRecord)",
            "morsehgp3d_phase4_spatial_lbvh_cover_kernel<<<1U, 1U, 0U, cuda.stream()>>>",
            "cudaMemcpyDeviceToHost",
            "output_count) > nodes.size()",
            "batch.buffer_epoch = context.advance_epoch()",
        ),
        "resident spatial-LBVH CUDA implementation",
    )

    resources = extract_braced_block(
        kernel,
        "class SpatialLbvhCudaResources final",
        "resident LBVH CUDA resources",
    )
    require_order(
        resources,
        (
            "nodes_.allocate(nodes.size()",
            "traversal_stack_.allocate(",
            "nodes.size()",
            "records_.allocate(",
            "nodes.size()",
            "record_count_.allocate(",
            "1U",
            "cudaMemcpyAsync(",
            "nodes.size() * sizeof(SpatialLbvhNodeInputRecord)",
            "node_count_ = nodes.size()",
        ),
        "node-count-bounded resident allocation",
    )

    traversal = extract_braced_block(
        kernel,
        "__global__ void morsehgp3d_phase4_spatial_lbvh_cover_kernel(",
        "resident LBVH cover kernel",
    )
    require_order(
        traversal,
        (
            "traversal_stack[0] = root_index",
            "while (stack_size != 0U)",
            "const std::uint64_t node_index = traversal_stack[--stack_size]",
            "output_count >= static_cast<std::uint64_t>(node_count)",
            "directed_squared_distance(",
            "distance.lower > cutoff_upper",
            "spatial_lbvh_cover_prune_code",
            "const bool left_missing",
            "spatial_lbvh_cover_leaf_code",
            "stack_size <= static_cast<std::uint64_t>(node_count) - UINT64_C(2)",
            "traversal_stack[stack_size++] = node.right_child",
            "traversal_stack[stack_size++] = node.left_child",
            "*record_count = output_count",
        ),
        "bounded complete resident traversal",
    )
    for forbidden in (
        "distance.lower >= cutoff_upper",
        "distance.lower == cutoff_upper",
        "__fma",
        "fma(",
        "sqrt(",
        "rsqrt(",
        "pow(",
        "malloc(",
        "new ",
        "--use_fast_math",
    ):
        require(
            forbidden not in traversal,
            f"resident traversal contains forbidden {forbidden!r}",
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


def validate_host_cover(host: str) -> None:
    require_tokens(
        host,
        (
            '#include "morsehgp3d/gpu/spatial_lbvh.hpp"',
            '#include "phase4_spatial_bounds_internal.hpp"',
            "class SpatialLbvhHostState final",
            "std::shared_ptr<const void> cloud_identity",
            "std::vector<SpatialLbvhNodeInputRecord> nodes",
            "std::vector<spatial::PointId> leaf_point_ids",
            "enclose_nonnegative_rational",
            "enclose_rational",
            "DirectedEnclosureStatus::unsupported_range",
            "exact_minimum_squared_distance",
            "require_interval_contains_exact_bound",
            "SpatialLbvhCoverResult unsupported_cover(",
            "SpatialLbvhCoverResult validate_and_recertify_cover(",
            "batch.records.size() != host.nodes.size()",
            "sentinel_record(batch.records[index])",
            "record_position_by_node",
            "the GPU spatial-LBVH cover repeated a node index",
            "record.lower_squared_distance_bits <=",
            "cutoff_enclosure.upper_bits",
            "std::vector<unsigned char> point_class(host.point_count, 0U)",
            "std::vector<unsigned char> record_used(batch.record_count, 0U)",
            "std::vector<std::size_t> traversal_stack{host.root_index}",
            "exact_minimum_squared_distance(node, query)",
            "require_interval_contains_exact_bound(record, exact_bound)",
            "exact_bound <= squared_cutoff",
            "exact_bound.rational() - squared_cutoff.rational()",
            "the GPU spatial-LBVH candidate record does not name one leaf",
            "left_child >= node_index || right_child >= node_index",
            "std::find(record_used.begin(), record_used.end(), 0U)",
            "std::find(point_class.begin(), point_class.end(), 0U)",
            "1U + 2U * audit.internal_node_expansion_count",
            "audit.certified_pruned_point_count + audit.candidate_point_count",
            "audit.cover_antichain_complete = true",
            "audit.point_partition_complete = true",
            "audit.cpu_exact_recertification_complete = true",
            "index.validated_for(cloud)",
            "host_->nodes.size() != 2U * host_->point_count - 1U",
            "a moved-from GPU spatial-LBVH context is not queryable",
            "the GPU spatial-LBVH context belongs to another PointId namespace",
            "SpatialLbvhCoverResult SpatialLbvhContext::cover_strict_exterior(",
            "spatial::detail::validated_query(query)",
            "spatial::detail::validated_squared_radius(squared_cutoff)",
            "enclosure_supported(query_enclosures, cutoff_enclosure)",
            "return unsupported_cover(",
            "return state_->with_gpu_section([&]",
            "detail::propose_strict_lbvh_cover_on_gpu(",
        ),
        "resident spatial-LBVH host cover",
    )

    unsupported = extract_braced_block(
        host,
        "SpatialLbvhCoverResult unsupported_cover(",
        "unsupported-range resident cover",
    )
    require_tokens(
        unsupported,
        (
            "result.candidate_point_ids.reserve(host.point_count)",
            "point_index < host.point_count",
            "result.candidate_point_ids.push_back(",
            "result.audit.gpu_output_capacity = host.nodes.size()",
            "result.audit.candidate_point_count = host.point_count",
            "result.audit.unsupported_range_fallback_count = host.point_count",
            "result.audit.cover_antichain_complete = true",
            "result.audit.point_partition_complete = true",
            "result.audit.cpu_exact_recertification_complete = true",
        ),
        "unsupported-range exact fallback",
    )
    for forbidden in (
        "propose_strict_lbvh_cover_on_gpu",
        "with_gpu_section",
        "gpu_launch_count = 1U",
    ):
        require(
            forbidden not in unsupported,
            f"unsupported range must not launch CUDA via {forbidden!r}",
        )

    validation = extract_braced_block(
        host,
        "SpatialLbvhCoverResult validate_and_recertify_cover(",
        "host antichain-cover validation",
    )
    require_order(
        validation,
        (
            "batch.records.size() != host.nodes.size()",
            "record_position_by_node",
            "record.lower_squared_distance_bits <=",
            "std::vector<unsigned char> point_class(host.point_count, 0U)",
            "std::vector<unsigned char> record_used(batch.record_count, 0U)",
            "std::vector<std::size_t> traversal_stack{host.root_index}",
            "exact_minimum_squared_distance(node, query)",
            "require_interval_contains_exact_bound(record, exact_bound)",
            "exact_bound <= squared_cutoff",
            "std::find(record_used.begin(), record_used.end(), 0U)",
            "std::find(point_class.begin(), point_class.end(), 0U)",
            "audit.certified_pruned_point_count + audit.candidate_point_count",
            "audit.cover_antichain_complete = true",
            "audit.point_partition_complete = true",
            "audit.cpu_exact_recertification_complete = true",
            "return result",
        ),
        "cover checks before certification publication",
    )

    cover = extract_braced_block(
        host,
        "SpatialLbvhCoverResult SpatialLbvhContext::cover_strict_exterior(",
        "resident public cover",
    )
    require_order(
        cover,
        (
            "require_matching_cloud(cloud)",
            "spatial::detail::validated_query(query)",
            "spatial::detail::validated_squared_radius(squared_cutoff)",
            "return state_->with_gpu_section([&]",
            "enclosure_supported(query_enclosures, cutoff_enclosure)",
            "return unsupported_cover(",
            "detail::propose_strict_lbvh_cover_on_gpu(",
            "validate_and_recertify_cover(",
        ),
        "serialized fail-closed fallback-before-launch cover control flow",
    )


def validate_host_queries(host: str) -> None:
    closed_ball = extract_braced_block(
        host,
        "SpatialLbvhClosedBallResult SpatialLbvhContext::closed_ball(",
        "resident closed-ball query",
    )
    require_tokens(
        closed_ball,
        (
            "spatial::detail::validated_query(query)",
            "spatial::detail::validated_squared_radius(squared_radius)",
            "cover_strict_exterior(",
            "std::move(cover.certified_exterior_point_ids)",
            "for (const spatial::PointId point_id : cover.candidate_point_ids)",
            "spatial::detail::exact_squared_distance(",
            "distance < canonical_radius",
            "distance == canonical_radius",
            "std::sort(exterior_ids.begin(), exterior_ids.end())",
            "spatial::ClosedBallPartition partition",
            "cover.audit.cpu_exact_candidate_distance_evaluation_count =",
            "cover.audit.exact_partition_complete = true",
        ),
        "resident exact closed-ball publication",
    )
    require_order(
        closed_ball,
        (
            "cover_strict_exterior(",
            "for (const spatial::PointId point_id : cover.candidate_point_ids)",
            "spatial::detail::exact_squared_distance(",
            "distance < canonical_radius",
            "distance == canonical_radius",
            "spatial::ClosedBallPartition partition",
            "cpu_exact_candidate_distance_evaluation_count",
            "exact_partition_complete = true",
            "return SpatialLbvhClosedBallResult",
        ),
        "closed-ball proposal, exact decision, publication order",
    )

    top_k = extract_braced_block(
        host,
        "SpatialLbvhTopKResult SpatialLbvhContext::top_k(",
        "resident top-k query",
    )
    require_tokens(
        top_k,
        (
            "spatial::detail::validated_query(query)",
            "exclusions.validated_for(cloud)",
            "requested_rank",
            "eligible_point_count",
            "std::vector<spatial::ExactNeighbor> seed_neighbors",
            "seed_neighbors.reserve(requested_rank)",
            "std::optional<exact::ExactLevel> seed_cutoff",
            "seed_neighbors.size() < requested_rank",
            "exclusions.contains(point_id)",
            "spatial::detail::exact_squared_distance(",
            "distance > *seed_cutoff",
            "seed_neighbors.push_back(",
            "seed_neighbors.size() != requested_rank",
            "cover_strict_exterior(",
            "std::find_if(",
            "reused_seed_count != requested_rank",
            "exclusions.contains(point_id)",
            "evaluated_neighbors.size() + pruned_eligible_point_count !=",
            "spatial::TopKPartition::from_evaluated_neighbors(",
            "partition.cutoff_squared_distance() > *seed_cutoff",
            "cpu_exact_seed_distance_evaluation_count",
            "top_k_seed_squared_cutoff",
            "excluded_candidate_point_count",
            "certified_pruned_eligible_point_count",
            "cpu_exact_candidate_distance_evaluation_count",
            "exact_partition_complete = true",
        ),
        "resident exact-seed top-k publication",
    )
    require_order(
        top_k,
        (
            "exclusions.validated_for(cloud)",
            "eligible_point_count",
            "std::vector<spatial::ExactNeighbor> seed_neighbors",
            "std::optional<exact::ExactLevel> seed_cutoff",
            "spatial::detail::exact_squared_distance(",
            "seed_neighbors.push_back(",
            "seed_neighbors.size() != requested_rank",
            "cover_strict_exterior(",
            "for (const spatial::PointId point_id : cover.candidate_point_ids)",
            "exclusions.contains(point_id)",
            "std::find_if(",
            "reused_seed_count != requested_rank",
            "evaluated_neighbors.size() + pruned_eligible_point_count !=",
            "spatial::TopKPartition::from_evaluated_neighbors(",
            "partition.cutoff_squared_distance() > *seed_cutoff",
            "top_k_seed_squared_cutoff = *seed_cutoff",
            "exact_partition_complete = true",
            "return SpatialLbvhTopKResult",
        ),
        "exact seed, certified cutoff cover, complete shell order",
    )
    require(
        "brute_force_top_k" not in top_k,
        "the resident top-k path silently fell back to all-point brute force",
    )

    nearest = extract_braced_block(
        host,
        "SpatialLbvhTopKResult SpatialLbvhContext::nearest(",
        "resident nearest-neighbor query",
    )
    require_tokens(nearest, ("return top_k(", "1U", "exclusions"), "resident 1-NN")


def validate_query_friend(query_header: str) -> None:
    require_tokens(
        query_header,
        (
            "class SpatialLbvhContext;",
            "class TopKPartition",
            "class ClosedBallPartition",
        ),
        "spatial result namespace",
    )
    require(
        query_header.count("friend class gpu::SpatialLbvhContext;") >= 2,
        "both exact partition types must authorize the resident LBVH publisher",
    )


def validate_cmake(
    cmake: str,
    unit_cmake: str,
    presets: dict[str, Any],
    replay: str,
) -> None:
    require_tokens(
        cmake,
        (
            "morsehgp3d_gpu_spatial_bounds",
            "src/cuda/phase4_spatial_bounds_internal.hpp",
            "src/cuda/phase4_spatial_bounds.cu",
            "src/gpu/spatial_bounds.cpp",
            "src/gpu/spatial_lbvh.cpp",
            "morsehgp3d::gpu_spatial_lbvh",
            "ALIAS morsehgp3d_gpu_spatial_bounds",
            "morsehgp3d_configure_cuda_target(morsehgp3d_gpu_spatial_bounds)",
            "morsehgp3d_gpu_spatial_lbvh_replay",
            "src/tools/gpu_spatial_lbvh_replay.cpp",
            "PRIVATE morsehgp3d::gpu_spatial_lbvh",
            'PATTERN "morsehgp3d/gpu" EXCLUDE',
        ),
        "top-level resident LBVH CMake",
    )
    require_tokens(
        unit_cmake,
        (
            "morsehgp3d_gpu_spatial_lbvh_context_tests",
            "test_gpu_spatial_lbvh_context.cpp",
            "fake_gpu_spatial_bounds_launchers.cpp",
            '"${PROJECT_SOURCE_DIR}/src/gpu/spatial_lbvh.cpp"',
            "morsehgp3d_gpu_spatial_lbvh_replay_host",
            '"${PROJECT_SOURCE_DIR}/src/tools/gpu_spatial_lbvh_replay.cpp"',
            "morsehgp3d.phase4_spatial_lbvh_configuration",
            "check_phase4_spatial_lbvh.py",
            "morsehgp3d.spatial_gpu_lbvh_host_differential",
            "check_spatial_gpu_lbvh.py",
            "morsehgp3d.spatial_gpu_lbvh_differential",
            "morsehgp3d_gpu_spatial_lbvh_replay",
        ),
        "resident LBVH unit and differential CMake",
    )
    require(
        "morsehgp3d-spatial-gpu-lbvh-v1" in replay,
        "the resident LBVH replay protocol is not versioned",
    )

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
        for target in (CUDA_LIBRARY_TARGET, CUDA_REPLAY_TARGET):
            require(
                targets.count(target) == 1,
                f"{name} must build {target} exactly once",
            )

    serialized = json.dumps(presets, separators=(",", ":"), sort_keys=True)
    for forbidden in ("--use_fast_math", "-use_fast_math"):
        require(forbidden not in serialized, f"presets contain forbidden {forbidden}")


def validate_no_shortcuts(sources: dict[str, str]) -> None:
    forbidden_patterns = (
        r"\bepsilon\b",
        r"\btoleranc(?:e|es)\b",
        r"\bmax(?:imum)?_visits\b",
        r"\bvisit_(?:cap|limit|budget)\b",
        r"\bcandidate_(?:cap|limit|budget)\b",
        r"\bearly_stop\b",
        r"\bnextafter\b",
        r"\bcuvs\b",
        r"\braft\b",
    )
    for label, source in sources.items():
        for pattern in forbidden_patterns:
            require(
                re.search(pattern, source, re.IGNORECASE) is None,
                f"{label} contains forbidden shortcut matching {pattern!r}",
            )


def validate_project(project: Path) -> dict[str, object]:
    paths = {
        "api": project / "include/morsehgp3d/gpu/spatial_lbvh.hpp",
        "query": project / "include/morsehgp3d/spatial/query.hpp",
        "host": project / "src/gpu/spatial_lbvh.cpp",
        "internal": project / "src/cuda/phase4_spatial_bounds_internal.hpp",
        "kernel": project / "src/cuda/phase4_spatial_bounds.cu",
        "replay": project / "src/tools/gpu_spatial_lbvh_replay.cpp",
        "cmake": project / "CMakeLists.txt",
        "unit_cmake": project / "tests/unit/CMakeLists.txt",
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
    validate_internal_contract(texts["internal"])
    validate_cuda_kernel(texts["kernel"], texts["cuda_policy"])
    validate_host_cover(texts["host"])
    validate_host_queries(texts["host"])
    validate_query_friend(texts["query"])
    validate_cmake(texts["cmake"], texts["unit_cmake"], presets, texts["replay"])
    validate_no_shortcuts(
        {
            "resident LBVH API": texts["api"],
            "resident LBVH host": texts["host"],
            "resident LBVH internal state": texts["internal"],
            "resident LBVH CUDA traversal": texts["kernel"],
        }
    )
    return {
        "checked_artifacts": sorted(
            str(path.relative_to(project)) for path in paths.values()
        ),
        "cuda_executed": False,
        "cuda_library_target": CUDA_LIBRARY_TARGET,
        "cuda_replay_target": CUDA_REPLAY_TARGET,
        "decision_semantics": "cpu_exact_cover_and_leaf_recertification",
        "proposal_semantics": "gpu_resident_lbvh_strict_exterior_cover",
        "schema": "morsehgp3d.phase4.spatial_lbvh_static.v1",
    }


def main(arguments: list[str]) -> int:
    if len(arguments) != 2:
        print(
            "usage: check_phase4_spatial_lbvh.py MORSEHGP3D_SOURCE",
            file=sys.stderr,
        )
        return 2
    project = Path(arguments[1]).resolve()
    try:
        result = validate_project(project)
    except ContractError as error:
        print(f"Phase 4 resident spatial-LBVH contract failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, separators=(",", ":"), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
