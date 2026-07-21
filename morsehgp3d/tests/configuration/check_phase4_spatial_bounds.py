#!/usr/bin/env python3
"""Validate the static contract of the Phase 4 CUDA AABB-bound filter."""

from __future__ import annotations

import sys
from pathlib import Path


class ContractError(ValueError):
    """A spatial-bounds source invariant was violated."""


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
        position = text.find(token, position + 1)
        require(position >= 0, f"{label} is missing ordered token {token!r}")


def validate(root: Path) -> None:
    header = read_text(root / "include/morsehgp3d/gpu/spatial_bounds.hpp")
    internal = read_text(root / "src/cuda/phase4_spatial_bounds_internal.hpp")
    host = read_text(root / "src/gpu/spatial_bounds.cpp")
    enclosure = read_text(root / "src/gpu/rational_binary64_enclosure.hpp")
    kernel = read_text(root / "src/cuda/phase4_spatial_bounds.cu")
    replay = read_text(root / "src/tools/gpu_spatial_bounds_replay.cpp")
    cmake = read_text(root / "CMakeLists.txt")
    unit_cmake = read_text(root / "tests/unit/CMakeLists.txt")

    require_tokens(
        header,
        (
            "enum class DirectedEnclosureStatus",
            "unsupported_range",
            "enum class SpatialBoundsDecision",
            "prune,",
            "visit,",
            "unknown,",
            '"gpu_outward_interval_aabb"',
            '"cpu_exact_recertified_strict_prune"',
            "cpu_exact_prune_recertification_count",
            "certified_prune_count",
            "minimum_certified_strict_margin",
            "proposal_permutation_complete",
            "cpu_exact_recertification_complete",
            "class SpatialBoundsContext final",
            "SpatialBoundsResult classify_strict_prune(",
        ),
        "spatial-bounds public API",
    )
    for forbidden in (
        "public_status",
        "gpu_exact",
        "exact_gpu",
        "ClosedBallPartition",
        "TopKPartition",
    ):
        require(
            forbidden not in header,
            f"the bounded primitive overstates its scope via {forbidden!r}",
        )

    require_tokens(
        internal,
        (
            "SpatialBoundsInputRecord",
            "SpatialBoundsProposalRecord",
            "spatial_bounds_sentinel_code",
            "std::is_trivially_copyable_v<SpatialBoundsProposalRecord>",
            "class SpatialBoundsContextState final",
            "std::lock_guard<std::mutex> lock{mutex_}",
            "poisoned_.load(std::memory_order_acquire)",
            "poisoned_.store(true, std::memory_order_release)",
            "advance_epoch()",
            "propose_strict_aabb_prunes_on_gpu(",
        ),
        "spatial-bounds internal API",
    )

    require_tokens(
        enclosure,
        (
            '#include "morsehgp3d/gpu/spatial_bounds.hpp"',
            "struct DirectedEnclosure",
            "std::uint64_t lower_bits",
            "std::uint64_t upper_bits",
            "DirectedEnclosureStatus status",
            "enclose_nonnegative_rational",
            "enclose_rational",
            "positive_binary64_rational(midpoint_bits) <= value",
            "DirectedEnclosureStatus::unsupported_range",
            "DirectedEnclosureStatus::enclosed",
            "kSignBit | magnitude.upper_bits",
            "kSignBit | magnitude.lower_bits",
        ),
        "shared rational-to-binary64 enclosure",
    )
    require_tokens(
        host,
        (
            '#include "rational_binary64_enclosure.hpp"',
            "using detail::DirectedEnclosure",
            "using detail::enclose_nonnegative_rational",
            "using detail::enclose_rational",
            "validate_box",
            "exact_minimum_squared_distance",
            "validate_and_recertify",
            "batch.records.size() != boxes.size()",
            "std::vector<unsigned char> seen(boxes.size(), 0U)",
            "record.lower_squared_distance_bits <=",
            "cutoff_enclosure.upper_bits",
            "record.upper_squared_distance_bits >= cutoff_enclosure.lower_bits",
            "audit.proposal_permutation_complete = true",
            "lower_bound <= squared_cutoff",
            "lower_bound.rational() - squared_cutoff.rational()",
            "audit.cpu_exact_recertification_complete = true",
            "unsupported_result(",
            "unsupported_range_fallback_count = box_count",
            "return state_->with_gpu_section([&]",
            "detail::propose_strict_aabb_prunes_on_gpu",
        ),
        "spatial-bounds host",
    )
    require(
        "struct DirectedEnclosure" not in host,
        "the spatial-bounds host duplicates the shared enclosure record",
    )
    require_order(
        host,
        (
            "audit.proposal_permutation_complete = true",
            "exact_minimum_squared_distance(boxes[index], query)",
            "lower_bound <= squared_cutoff",
            "audit.cpu_exact_recertification_complete = true",
            "audit.all_boxes_classified = true",
        ),
        "host publication order",
    )
    unsupported_start = host.find("SpatialBoundsResult unsupported_result(")
    unsupported_end = host.find("SpatialBoundsResult validate_and_recertify(")
    require(
        0 <= unsupported_start < unsupported_end,
        "cannot isolate the unsupported-range fallback",
    )
    unsupported = host[unsupported_start:unsupported_end]
    require(
        "propose_strict_aabb_prunes_on_gpu" not in unsupported,
        "unsupported range must not launch CUDA",
    )

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
            "spatial_bounds_prune_code",
            "spatial_bounds_visit_code",
            "spatial_bounds_unknown_code",
            "kPositiveInfinityBits",
            "cudaMemsetAsync",
            "propose_strict_aabb_prunes_on_gpu(",
            "cudaMemcpyAsync",
            "cudaStreamSynchronize",
        ),
        "spatial-bounds CUDA kernel",
    )
    for forbidden in ("__dsub_rn", "__dmul_rn", "__dadd_rn", "--use_fast_math"):
        require(
            forbidden not in kernel,
            f"directed spatial bounds contain forbidden operation {forbidden!r}",
        )

    require_tokens(
        cmake,
        (
            "morsehgp3d_gpu_spatial_bounds",
            "src/cuda/phase4_spatial_bounds.cu",
            "src/gpu/spatial_bounds.cpp",
            "morsehgp3d_configure_cuda_target(morsehgp3d_gpu_spatial_bounds)",
            "morsehgp3d_gpu_spatial_bounds_replay",
        ),
        "top-level CMake",
    )
    require_tokens(
        unit_cmake,
        (
            "morsehgp3d_gpu_spatial_bounds_context_tests",
            "fake_gpu_spatial_bounds_launchers.cpp",
            "morsehgp3d_gpu_spatial_bounds_replay_host",
            "check_phase4_spatial_bounds.py",
            "check_spatial_gpu_bounds.py",
        ),
        "unit-test CMake",
    )
    require(
        "morsehgp3d-spatial-gpu-bounds-v1" in replay,
        "the spatial-bounds replay protocol is not versioned",
    )


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_phase4_spatial_bounds.py MORSEHGP3D_SOURCE", file=sys.stderr)
        return 2
    try:
        validate(Path(sys.argv[1]).resolve())
    except ContractError as error:
        print(f"Phase 4 spatial-bounds configuration check failed: {error}", file=sys.stderr)
        return 1
    print("Phase 4 spatial-bounds configuration check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
