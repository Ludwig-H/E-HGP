#!/usr/bin/env python3
"""Validate the static Phase 7 H-polytope CUDA proposal contract.

The checker intentionally executes no CUDA code.  It freezes the proposal-only
ABI and verifies that the CUDA translation unit implements conservative,
directed-rounding interval logic before it can enter a CUDA build preset.
"""

from __future__ import annotations

import copy
import json
import re
import sys
from pathlib import Path
from typing import Any, Callable


PUBLIC_HEADER = Path("include/morsehgp3d/gpu/h_polytope_proposal.hpp")
INTERNAL_HEADER = Path("src/cuda/phase7_h_polytope_proposal_internal.hpp")
INTERVAL_HEADER = Path("src/cuda/phase2b_interval.cuh")
HOST_SOURCE = Path("src/gpu/h_polytope_proposal.cpp")
CUDA_SOURCE = Path("src/cuda/phase7_h_polytope_proposal.cu")
FAKE_HEADER = Path("tests/unit/fake_gpu_h_polytope_proposal_launchers.hpp")
FAKE_SOURCE = Path("tests/unit/fake_gpu_h_polytope_proposal_launchers.cpp")
UNIT_TEST_SOURCE = Path("tests/unit/test_gpu_h_polytope_proposal_context.cpp")
CMAKE_SOURCE = Path("CMakeLists.txt")
CUDA_POLICY = Path("cmake/MorseHGP3DCuda.cmake")
PRESETS_SOURCE = Path("CMakePresets.json")

CUDA_TARGET = "morsehgp3d_gpu_h_polytope_proposal"
CUDA_PRESETS = ("cuda-release", "cuda-audit")
CPU_PRESETS = ("cpu-release", "sanitizer")
DIRECTED_INTRINSICS = (
    "__dadd_rd",
    "__dadd_ru",
    "__dsub_rd",
    "__dsub_ru",
    "__dmul_rd",
    "__dmul_ru",
    "__ddiv_rd",
    "__ddiv_ru",
)
SHARED_DIRECTED_INTRINSICS = DIRECTED_INTRINSICS[:6]
LOCAL_DIVISION_INTRINSICS = DIRECTED_INTRINSICS[6:]


class ContractError(ValueError):
    """A Phase 7 H-polytope proposal invariant was violated."""


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


def without_comments(text: str) -> str:
    return re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.DOTALL)


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


def parenthesized_calls(text: str, callee: str) -> list[str]:
    """Return complete argument lists for direct calls to *callee*."""
    result: list[str] = []
    pattern = re.compile(rf"\b{re.escape(callee)}\s*\(")
    for match in pattern.finditer(text):
        opening = text.find("(", match.start())
        depth = 0
        for position in range(opening, len(text)):
            character = text[position]
            if character == "(":
                depth += 1
            elif character == ")":
                depth -= 1
                if depth == 0:
                    result.append(text[opening + 1 : position])
                    break
        else:
            raise ContractError(f"unterminated call to {callee}")
    return result


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
            '#include "morsehgp3d/spatial/h_polytope_reference.hpp"',
            "enum class HPolytopeProposalBatchDecision : std::uint8_t",
            "exact_recertified_local",
            "insufficient_exact_budget",
            "enum class HPolytopeProposalCellStatus : std::uint8_t",
            "validated_exhaustive_transcript",
            "exact_fallback_interval_unknown",
            "exact_fallback_capacity_exhausted",
            "exact_fallback_unsupported_projection",
            "enum class HPolytopeProposalRecordStatus : std::uint8_t",
            "unknown_requires_cpu_exact",
            "proposed_strict_reject",
            "proposed_survivor",
            "struct HPolytopeProposalCoordinateIntervals3",
            "lower_binary64_bits",
            "upper_binary64_bits",
            "struct HPolytopeProposalRecord",
            "strict_reject_boundary_witness",
            "survivor_coordinate_intervals",
            "could_be_active_boundary_mask",
            "struct HPolytopeProposalAudit",
            '"proposal_only_exhaustive_plane_triple_transcript"',
            '"reference_cpu_exact_all_constraints"',
            "physical_proposal_record_capacity",
            "initialized_proposal_record_count",
            "exact_plane_triple_replay_count",
            "cpu_exact_recertification_complete",
            "bool global_status_published{false}",
            "struct HPolytopeProposalBatchResult",
            "std::vector<std::size_t> proposal_offsets",
            "std::vector<HPolytopeProposalRecord> proposal_records",
            "class HPolytopeProposalContext final",
            "HPolytopeProposalContext(HPolytopeProposalContext&&) noexcept",
            "HPolytopeProposalContext(const HPolytopeProposalContext&) = delete",
            "HPolytopeProposalBatchResult build(",
            "std::shared_ptr<detail::HPolytopeProposalContextState> state_",
        ),
        "Phase 7 H-polytope proposal public API",
    )
    for forbidden in (
        "cudaStream_t",
        "cudaError_t",
        "HPolytopeProposalDeviceRecord",
        "gpu_exact",
        "exact_gpu",
        "bool global_status_published{true}",
    ):
        require(
            forbidden not in header,
            f"the public proposal API leaks or overstates {forbidden!r}",
        )


def _validate_pod_record(
    internal: str,
    marker: str,
    scalar_fields: tuple[str, ...],
    array_fields: tuple[tuple[str, int], ...],
) -> None:
    block = without_comments(extract_braced_block(internal, marker, marker))
    for field in scalar_fields:
        require(
            re.search(rf"std::uint64_t\s+{re.escape(field)}\b", block) is not None,
            f"{marker} is missing uint64 field {field}",
        )
    for field, count in array_fields:
        require(
            re.search(
                rf"std::array\s*<\s*std::uint64_t\s*,\s*{count}U?\s*>\s*"
                rf"{re.escape(field)}\b",
                block,
            )
            is not None,
            f"{marker} is missing {count}-word field {field}",
        )
    require(
        len(re.findall(r"std::uint64_t\s+[A-Za-z_]\w*", block))
        == len(scalar_fields),
        f"{marker} changed its scalar-word ABI",
    )
    require(
        len(re.findall(r"std::array\s*<\s*std::uint64_t\s*,", block))
        == len(array_fields),
        f"{marker} changed its array-word ABI",
    )
    for forbidden in (
        "std::vector",
        "std::string",
        "std::span",
        "std::unique_ptr",
        "std::shared_ptr",
        "*",
    ):
        require(forbidden not in block, f"{marker} is not a fixed POD record")


def validate_internal_contract(internal: str) -> None:
    require_tokens(
        internal,
        (
            "kHPolytopeProposalInitializedSlotSentinel",
            "UINT64_C(0x4850475033445631)",
            "kHPolytopeProposalNoBoundaryWitness",
            "std::numeric_limits<std::uint64_t>::max()",
            "enum class HPolytopeProposalDeviceCellStatus : std::uint64_t",
            "validated_exhaustive_transcript = 1U",
            "exact_fallback_interval_unknown = 2U",
            "exact_fallback_capacity_exhausted = 3U",
            "exact_fallback_unsupported_projection = 4U",
            "enum class HPolytopeProposalDeviceRecordStatus : std::uint64_t",
            "unknown_requires_cpu_exact = 1U",
            "proposed_strict_reject = 2U",
            "proposed_survivor = 3U",
            "struct HPolytopeProposalDeviceBatch",
            "std::vector<std::uint64_t> cell_ids",
            "std::vector<HPolytopeProposalDeviceCellStatus> cell_statuses",
            "std::vector<std::uint64_t> cell_record_offsets",
            "std::vector<HPolytopeProposalDeviceRecord> records",
            "std::uint64_t record_count{0U}",
            "std::uint64_t buffer_epoch{0U}",
            "class HPolytopeProposalContextState final",
            "std::lock_guard<std::mutex> lock{mutex_}",
            "poisoned_.load(std::memory_order_acquire)",
            "poisoned_.store(true, std::memory_order_release)",
            "std::shared_ptr<void>& cuda_resources()",
            "advance_epoch()",
            "current_epoch() const noexcept",
            "propose_h_polytope_transcript_on_gpu(",
        ),
        "Phase 7 H-polytope internal ABI",
    )
    _validate_pod_record(
        internal,
        "struct HPolytopeProposalInputCell",
        (
            "cell_id",
            "boundary_begin",
            "boundary_end",
            "unsupported_projection",
            "force_interval_fallback",
        ),
        (),
    )
    _validate_pod_record(
        internal,
        "struct HPolytopeProposalInputBoundary",
        (),
        (("coefficient_lower_bits", 4), ("coefficient_upper_bits", 4)),
    )
    _validate_pod_record(
        internal,
        "struct HPolytopeProposalDeviceRecord",
        (
            "initialized_slot_sentinel",
            "buffer_epoch",
            "cell_id",
            "first_boundary_index",
            "second_boundary_index",
            "third_boundary_index",
            "status_code",
            "strict_reject_boundary_witness",
            "could_be_active_boundary_mask",
        ),
        (("coordinate_lower_bits", 3), ("coordinate_upper_bits", 3)),
    )
    require(
        internal.count("std::is_trivially_copyable_v<HPolytopeProposal") == 3,
        "all three Phase 7 transfer records must remain trivially copyable",
    )
    state = extract_braced_block(
        internal,
        "class HPolytopeProposalContextState final",
        "Phase 7 poisonable context state",
    )
    require_order(
        state,
        (
            "std::lock_guard<std::mutex> lock{mutex_}",
            "poisoned_.load(std::memory_order_acquire)",
            "try",
            "std::forward<Operation>(operation)()",
            "catch (...) ",
            "mark_poisoned()",
            "throw;",
        ),
        "Phase 7 poison-on-failure transaction",
    )


def validate_host(host: str) -> None:
    require_tokens(
        host,
        (
            '#include "rational_binary64_enclosure.hpp"',
            "canonical_cell_slices(",
            "std::sort(",
            "cell ids must be unique",
            "plane_triple_count(",
            "spatial::build_exact_bounded_h_polytope_reference(",
            "detail::enclose_rational(",
            "force_interval_fallback",
            "valid_boundary_mask(",
            "boundary_count > 61U",
            "is_zero_tail_record(",
            "batch.cell_record_offsets.front() != 0U",
            "batch.cell_record_offsets.back() != batch.record_count",
            "batch.records.size() != physical_capacity",
            "batch.buffer_epoch == 0U",
            "record.initialized_slot_sentinel !=",
            "detail::kHPolytopeProposalInitializedSlotSentinel",
            "record.buffer_epoch != batch.buffer_epoch",
            "for (std::size_t index = record_count; index < batch.records.size()",
            "!is_zero_tail_record(batch.records[index])",
            "row_size != 0U",
            "row_size != cell.requirements.exhaustive_proposal_record_count",
            "for (std::size_t first = 0U; first < boundary_count; ++first)",
            "for (std::size_t second = first + 1U; second < boundary_count",
            "for (std::size_t third = second + 1U; third < boundary_count",
            "device_record.first_boundary_index != first",
            "device_record.second_boundary_index != second",
            "device_record.third_boundary_index != third",
            "exact::intersect_three_planes(",
            "if (sign > 0)",
            "if (sign == 0)",
            "~device_record.could_be_active_boundary_mask",
            "exact_result_contains_vertex(",
            "finite_canonical_binary64_rational(",
            "intersection.point()->coordinate(axis) < lower",
            "intersection.point()->coordinate(axis) > upper",
            "result.audit.exhaustive_transcript_validated = true",
            "result.audit.cpu_exact_recertification_complete = true",
            "state_->with_gpu_section([&]",
            "const std::uint64_t previous_epoch = state_->current_epoch()",
            "detail::propose_h_polytope_transcript_on_gpu(",
            "state_->current_epoch() != previous_epoch + UINT64_C(1)",
            "batch.buffer_epoch != state_->current_epoch()",
        ),
        "Phase 7 H-polytope host recertifier",
    )
    require(
        re.search(
            r"strict_reject_boundary_witness[^;{}]*?evaluate\s*\(\s*"
            r"\*intersection[.]point\s*\(\s*\)\s*\)[^;{}]*?"
            r"[.]sign\s*\(\s*\)\s*<=\s*0",
            without_comments(host),
            flags=re.DOTALL,
        )
        is not None,
        "a proposed strict reject is not recertified by an exact positive witness",
    )
    require(
        "global_status_published = true" not in host,
        "the proposal host must never publish a global scientific status",
    )
    require(
        re.search(
            r"cell_status\s*!=\s*HPolytopeProposalCellStatus::\s*"
            r"validated_exhaustive_transcript\s*&&\s*row_size\s*!=\s*0U",
            without_comments(host),
            flags=re.DOTALL,
        )
        is not None,
        "a fallback cell must be rejected whenever it exposes a partial row",
    )
    validation = extract_braced_block(
        host,
        "HPolytopeProposalBatchResult validate_device_batch(",
        "Phase 7 device-batch validation",
    )
    require_order(
        validation,
        (
            "batch.records.size() != physical_capacity",
            "record.initialized_slot_sentinel !=",
            "for (std::size_t index = record_count; index < batch.records.size()",
            "HPolytopeProposalBatchResult result",
            "for (std::size_t cell_index = 0U; cell_index < built_cells.size()",
            "row_size != 0U",
            "for (std::size_t first = 0U; first < boundary_count; ++first)",
            "exact::intersect_three_planes(",
            "result.audit.exhaustive_transcript_validated = true",
            "result.audit.cpu_exact_recertification_complete = true",
            "return result",
        ),
        "validate-before-publish Phase 7 host order",
    )


def validate_fake(fake_header: str, fake_source: str, unit_test: str) -> None:
    require_tokens(
        fake_header,
        (
            "enum class FakeHPolytopeProposalValues : std::uint8_t",
            "actual_binary64_recipe",
            "all_unknown",
            "whole_cell_interval_fallback",
            "enum class FakeHPolytopeProposalCorruption : std::uint8_t",
            "missing_slot",
            "duplicate_slot",
            "omitted_true_incidence",
            "out_of_range_incidence_bit",
            "nonfinite_survivor_coordinate",
            "false_strict_reject",
            "wrong_epoch",
            "wrong_batch_epoch",
            "stale_epoch_without_advance",
            "double_epoch_advance",
            "wrong_ordinal",
            "invalid_offsets",
            "invalid_cell_id",
            "invalid_cell_status",
            "invalid_record_status",
            "tail_write",
            "simulated_async_failure",
        ),
        "Phase 7 fake launcher controls",
    )
    require_tokens(
        fake_source,
        (
            "HPolytopeProposalDeviceBatch propose_h_polytope_transcript_on_gpu(",
            "batch.records.resize(maximum_total_proposal_record_count)",
            "batch.cell_record_offsets.push_back(0U)",
            "triplet_count(boundary_count)",
            "maximum_total_proposal_record_count -\n                               record_count",
            "exact_fallback_capacity_exhausted",
            "exact_fallback_interval_unknown",
            "for (std::size_t first = 0U; first + 2U < boundary_count; ++first)",
            "for (std::size_t second = first + 1U;",
            "for (std::size_t third = second + 1U; third < boundary_count",
            "batch.records[record_count] = recipe_record(",
            "++record_count",
            "batch.record_count = static_cast<std::uint64_t>(record_count)",
            "batch.buffer_epoch = context.advance_epoch()",
            "inject_corruption(batch, cells, corruption)",
        ),
        "Phase 7 fake launcher",
    )
    require_tokens(
        unit_test,
        (
            "std::string{result.audit.proposal_semantics}.find(\"proposal_only\")",
            "all_unknown",
            "whole_cell_interval_fallback",
            "missing_slot",
            "duplicate_slot",
            "omitted_true_incidence",
            "out_of_range_incidence_bit",
            "false_strict_reject",
            "wrong_epoch",
            "stale_epoch_without_advance",
            "double_epoch_advance",
            "wrong_ordinal",
            "tail_write",
            "simulated_async_failure",
            "global_status_published",
        ),
        "Phase 7 fake-backed unit suite",
    )


def _contains_zero_contract(kernel: str) -> None:
    clean = without_comments(kernel)
    explicit = re.search(
        r"determinant[A-Za-z0-9_.>\-]*(?:lower|lo)\s*<=\s*0(?:[.]0)?\s*&&\s*"
        r"determinant[A-Za-z0-9_.>\-]*(?:upper|hi)\s*>=\s*0(?:[.]0)?",
        clean,
        flags=re.DOTALL,
    )
    helper_calls = re.findall(
        r"([A-Za-z_]\w*zero[A-Za-z_]*)\s*\(\s*"
        r"determinant(?:_interval)?\s*\)",
        clean,
        flags=re.IGNORECASE,
    )
    helper_valid = False
    for helper in helper_calls:
        try:
            body = extract_braced_block(clean, f"{helper}(", f"{helper} helper")
        except ContractError:
            continue
        if re.search(r"[.](?:lower|lo)\s*<=\s*0(?:[.]0)?", body) and re.search(
            r"[.](?:upper|hi)\s*>=\s*0(?:[.]0)?", body
        ):
            helper_valid = True
            break
    require(
        explicit is not None or helper_valid,
        "the determinant interval must classify lower <= 0 <= upper as unknown",
    )
    base = extract_braced_block(
        clean,
        "HPolytopeProposalDeviceRecord base_record(",
        "Phase 7 default unknown record",
    )
    require_tokens(
        base,
        (
            "HPolytopeProposalDeviceRecordStatus::unknown_requires_cpu_exact",
            "kHPolytopeProposalNoBoundaryWitness",
        ),
        "Phase 7 default unknown record",
    )
    proposal = extract_braced_block(
        clean,
        "HPolytopeProposalDeviceRecord propose_record(",
        "Phase 7 interval record proposal",
    )
    require_order(
        proposal,
        (
            "base_record(",
            "const DeviceInterval determinant",
            "contains_zero(determinant)",
            "return result",
            "divide_intervals(numerator.x, determinant)",
        ),
        "zero-determinant unknown fallback before interval division",
    )


def _strict_reject_and_survivor_contract(kernel: str) -> None:
    clean = without_comments(kernel)
    lower_positive = re.search(
        r"(?:value|evaluation|constraint|residual)[A-Za-z0-9_.>\-]*(?:lower|lo)\s*>\s*"
        r"0(?:[.]0)?",
        clean,
        flags=re.IGNORECASE,
    )
    require(lower_positive is not None, "strict rejection lacks a lower > 0 proof")
    reject_position = clean.find("proposed_strict_reject", lower_positive.start())
    require(
        reject_position > lower_positive.start(),
        "strict rejection is not downstream of its positive lower-bound proof",
    )
    reject_prefix = clean[max(0, reject_position - 1200) : reject_position]
    for forbidden in ("upper > 0.0", "upper >= 0.0", "fabs(", "abs("):
        require(
            forbidden not in reject_prefix,
            f"strict rejection uses forbidden proof condition {forbidden!r}",
        )

    upper_nonpositive = re.search(
        r"(?:value|evaluation|constraint|residual)[A-Za-z0-9_.>\-]*(?:upper|hi)\s*"
        r"<=\s*0(?:[.]0)?",
        clean,
        flags=re.IGNORECASE,
    )
    uncertain_positive = re.search(
        r"(?:value|evaluation|constraint|residual)[A-Za-z0-9_.>\-]*(?:upper|hi)\s*"
        r">\s*0(?:[.]0)?",
        clean,
        flags=re.IGNORECASE,
    )
    survivor_position = clean.find("proposed_survivor")
    proof_position = (
        upper_nonpositive.start()
        if upper_nonpositive is not None
        else uncertain_positive.start()
        if uncertain_positive is not None
        else -1
    )
    require(
        proof_position >= 0 and survivor_position > proof_position,
        "survivor publication must follow an all-constraints upper <= 0 proof",
    )
    require(
        re.search(
            r"for\s*\([^)]*boundary",
            clean[max(0, proof_position - 5000) : survivor_position],
            re.DOTALL,
        )
        is not None
        or "all_constraints" in clean[proof_position:survivor_position]
        or "feasibility" in clean[proof_position:survivor_position],
        "the survivor proof is not visibly exhaustive over boundaries",
    )


def validate_shared_interval(interval_header: str) -> None:
    require_tokens(
        interval_header,
        (
            "struct DeviceInterval",
            "double lower{0.0}",
            "double upper{0.0}",
            "bool valid{false}",
            "DeviceInterval checked_interval(",
            "lower > upper",
            "DeviceInterval add_intervals(",
            "DeviceInterval subtract_intervals(",
            "DeviceInterval multiply_intervals(",
        )
        + SHARED_DIRECTED_INTRINSICS,
        "shared Phase 2B directed interval primitive",
    )
    clean = without_comments(interval_header)
    addition = extract_braced_block(
        clean, "DeviceInterval add_intervals(", "shared interval addition"
    )
    require_order(
        addition,
        (
            "__dadd_rd(left.lower, right.lower)",
            "__dadd_ru(left.upper, right.upper)",
        ),
        "shared interval addition endpoints",
    )
    subtraction = extract_braced_block(
        clean,
        "DeviceInterval subtract_intervals(",
        "shared interval subtraction",
    )
    require_order(
        subtraction,
        (
            "__dsub_rd(left.lower, right.upper)",
            "__dsub_ru(left.upper, right.lower)",
        ),
        "shared interval subtraction endpoints",
    )
    multiplication = extract_braced_block(
        clean,
        "DeviceInterval multiply_intervals(",
        "shared interval multiplication",
    )
    require_tokens(
        multiplication,
        (
            "left.lower >= 0.0 && right.lower >= 0.0",
            "__dmul_rd(left.lower, right.lower)",
            "__dmul_ru(left.upper, right.upper)",
            "left.upper <= 0.0 && right.lower >= 0.0",
            "__dmul_rd(left.lower, right.upper)",
            "__dmul_ru(left.upper, right.lower)",
            "left.lower >= 0.0 && right.upper <= 0.0",
            "__dmul_rd(left.upper, right.lower)",
            "__dmul_ru(left.lower, right.upper)",
            "left.upper <= 0.0 && right.upper <= 0.0",
            "__dmul_rd(left.upper, right.upper)",
            "__dmul_ru(left.lower, right.lower)",
            "const double lower_products[4]",
            "const double upper_products[4]",
            "return checked_interval(lower, upper)",
        ),
        "shared interval multiplication sign quadrants",
    )
    require(
        multiplication.count("__dmul_rd(") == 8
        and multiplication.count("__dmul_ru(") == 8,
        "shared interval multiplication must cover four sign quadrants and four endpoint pairs",
    )
    for pattern in (
        r"\bepsilon\b",
        r"\btoleranc(?:e|es)\b",
        r"\bnextafter\b",
        r"--use_fast_math",
        r"(?<![A-Za-z0-9_])(?:__)?fma(?:f|_[a-z0-9_]+)?\s*\(",
        r"(?<![A-Za-z0-9_])sqrt\s*\(",
        r"(?<![A-Za-z0-9_])pow\s*\(",
    ):
        require(
            re.search(pattern, clean, flags=re.IGNORECASE) is None,
            f"the shared interval primitive contains forbidden {pattern!r}",
        )


def validate_cuda_kernel(kernel: str, interval_header: str) -> None:
    validate_shared_interval(interval_header)
    require_tokens(
        kernel,
        (
            '#include "phase7_h_polytope_proposal_internal.hpp"',
            '#include "phase2b_interval.cuh"',
            "#include <cuda_runtime.h>",
            '#error "phase7_h_polytope_proposal.cu must be compiled ahead of time with NVCC"',
            "__CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9",
            "defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)",
            "defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0",
            "defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200",
            "cudaStreamCreateWithFlags(",
            "cudaStreamNonBlocking",
            "cudaStreamDestroy(",
            "cudaStreamSynchronize(",
            "cudaMalloc(",
            "cudaFree(",
            "cudaMemcpyAsync(",
            "cudaMemsetAsync(",
            "cudaGetLastError()",
            "HPolytopeProposalDeviceRecord",
            "kHPolytopeProposalInitializedSlotSentinel",
            "kHPolytopeProposalNoBoundaryWitness",
            "HPolytopeProposalDeviceRecordStatus::unknown_requires_cpu_exact",
            "HPolytopeProposalDeviceRecordStatus::proposed_strict_reject",
            "HPolytopeProposalDeviceRecordStatus::proposed_survivor",
            "HPolytopeProposalDeviceCellStatus::validated_exhaustive_transcript",
            "HPolytopeProposalDeviceCellStatus::exact_fallback_interval_unknown",
            "HPolytopeProposalDeviceCellStatus::exact_fallback_capacity_exhausted",
            "HPolytopeProposalDeviceCellStatus::exact_fallback_unsupported_projection",
            "cell_record_offsets",
            "ordinal",
            "could_be_active_boundary_mask",
            "constexpr std::size_t kMaximumBoundaryCount = 61U",
            "boundary_count > kMaximumBoundaryCount",
            "UINT64_C(1) << boundary_index",
            "context.advance_epoch()",
            "device::add_intervals(",
            "device::subtract_intervals(",
            "device::multiply_intervals(",
        )
        + LOCAL_DIVISION_INTRINSICS,
        "Phase 7 H-polytope CUDA proposal",
    )
    clean = without_comments(kernel)
    division = extract_braced_block(
        clean,
        "DeviceInterval divide_intervals(",
        "Phase 7 directed interval division",
    )
    require_tokens(
        division,
        (
            "contains_zero(denominator)",
            "const double lower_candidates[4]",
            "__ddiv_rd(numerator.lower, denominator.lower)",
            "__ddiv_rd(numerator.lower, denominator.upper)",
            "__ddiv_rd(numerator.upper, denominator.lower)",
            "__ddiv_rd(numerator.upper, denominator.upper)",
            "const double upper_candidates[4]",
            "__ddiv_ru(numerator.lower, denominator.lower)",
            "__ddiv_ru(numerator.lower, denominator.upper)",
            "__ddiv_ru(numerator.upper, denominator.lower)",
            "__ddiv_ru(numerator.upper, denominator.upper)",
            "return device::checked_interval(lower, upper)",
        ),
        "Phase 7 directed interval division",
    )
    require(
        division.count("__ddiv_rd(") == 4
        and division.count("__ddiv_ru(") == 4,
        "interval division must evaluate all four endpoint pairs in both directions",
    )
    row_planning = extract_braced_block(
        clean,
        "HPolytopeProposalDeviceBatch propose_h_polytope_transcript_on_gpu(",
        "Phase 7 whole-row launch planning",
    )
    require_order(
        row_planning,
        (
            "batch.cell_record_offsets.push_back(0U)",
            "std::size_t record_count = 0U",
            "std::size_t expected_boundary_begin = 0U",
            "for (std::size_t cell_index = 0U; cell_index < cells.size(); ++cell_index)",
            "const HPolytopeProposalInputCell& cell = cells[cell_index]",
            "boundary_begin != expected_boundary_begin",
            "cells[cell_index - 1U].cell_id >= cell.cell_id",
            "expected_boundary_begin = boundary_end",
            "const std::size_t required = plane_triple_count(boundary_count)",
            "HPolytopeProposalDeviceCellStatus::validated_exhaustive_transcript",
            "if (cell.unsupported_projection != 0U)",
            "exact_fallback_unsupported_projection",
            "else if (cell.force_interval_fallback != 0U)",
            "exact_fallback_interval_unknown",
            "maximum_total_proposal_record_count - record_count",
            "exact_fallback_capacity_exhausted",
            "record_count += required",
            "batch.cell_record_offsets.push_back(",
            "static_cast<std::uint64_t>(record_count)",
            "if (expected_boundary_begin != boundaries.size())",
            "batch.record_count = static_cast<std::uint64_t>(record_count)",
            "batch.records.resize(maximum_total_proposal_record_count)",
        ),
        "whole-row CSR planning without partial capacity emission",
    )
    require(
        re.search(r"class\s+[A-Za-z_]\w*CudaResources\s+final", clean) is not None,
        "the CUDA context has no owned resource aggregate",
    )
    require(
        re.search(
            r"records_[.]ensure_capacity\s*\(\s*record_capacity\s*,\s*"
            r"\"cudaMalloc Phase 7 H-polytope records\"",
            clean,
            flags=re.DOTALL,
        )
        is not None,
        "the resource aggregate does not own the full record capacity",
    )
    require(
        re.search(
            r"cudaMemsetAsync\s*\([^;]*records[^;]*0(?:U)?\s*,[^;]*"
            r"sizeof\s*\(\s*HPolytopeProposalDeviceRecord\s*\)[^;]*stream",
            clean,
            flags=re.DOTALL,
        )
        is not None,
        "the complete physical record capacity is not zeroed on the owned stream",
    )
    for callee in ("cudaMemcpyAsync", "cudaMemsetAsync"):
        calls = parenthesized_calls(clean, callee)
        require(bool(calls), f"the CUDA proposal never calls {callee}")
        for arguments in calls:
            require(
                re.search(r"\b[A-Za-z_]*stream[A-Za-z_]*\b", arguments) is not None,
                f"{callee} escaped the owned nondefault stream",
            )
            require(
                "cudaStreamDefault" not in arguments and "nullptr" not in arguments,
                f"{callee} names a default stream",
            )
    kernel_match = re.search(
        r"__global__\s+void\s+([A-Za-z_]\w*h_polytope[A-Za-z_]\w*)\s*\(",
        clean,
        flags=re.IGNORECASE,
    )
    require(kernel_match is not None, "the Phase 7 global proposal kernel is missing")
    kernel_body = extract_braced_block(
        clean,
        kernel_match.group(0),
        "Phase 7 H-polytope proposal kernel",
    )
    require(
        re.search(
            r"records\s*\[\s*[^\]]*(?:record_begin|record_offset|row_begin)"
            r"[^\]]*\+\s*ordinal\s*\]",
            kernel_body,
        )
        is not None,
        "each combinadic ordinal must address its deterministic row slot directly",
    )
    require(
        "atomicAdd(" not in kernel_body,
        "the direct-ordinal proposal kernel must not use an emission counter",
    )
    require(
        re.search(
            rf"{re.escape(kernel_match.group(1))}\s*<<<.*?stream\s*\(\s*\)\s*>>>",
            clean,
            re.DOTALL,
        )
        is not None,
        "the proposal kernel launch does not name the owned nondefault stream",
    )
    require_order(
        clean,
        (
            "cudaMemsetAsync(",
            f"{kernel_match.group(1)}",
            "cudaGetLastError()",
            "cudaMemcpyAsync(",
            "cuda.synchronize()",
            "context.advance_epoch()",
        ),
        "zero-launch-copy-synchronize-epoch CUDA transaction",
    )
    _contains_zero_contract(clean)
    _strict_reject_and_survivor_contract(clean)

    proposal = extract_braced_block(
        clean,
        "HPolytopeProposalDeviceRecord propose_record(",
        "Phase 7 conservative record proposal",
    )
    require_order(
        proposal,
        (
            "HPolytopeProposalDeviceRecord result =",
            "base_record(",
            "contains_zero(determinant)",
            "divide_intervals(numerator.x, determinant)",
            "bool feasibility_unknown = false",
            "(UINT64_C(1) << first)",
            "(UINT64_C(1) << second)",
            "(UINT64_C(1) << third)",
            "for (std::size_t boundary_index = 0U",
            "if (!evaluation.valid)",
            "feasibility_unknown = true",
            "if (evaluation.lower > 0.0)",
            "HPolytopeProposalDeviceRecordStatus::proposed_strict_reject",
            "if (evaluation.upper > 0.0)",
            "feasibility_unknown = true",
            "if (contains_zero(evaluation))",
            "could_be_active_boundary_mask |= UINT64_C(1) << boundary_index",
            "if (feasibility_unknown)",
            "return result",
            "HPolytopeProposalDeviceRecordStatus::proposed_survivor",
            "result.could_be_active_boundary_mask = could_be_active_boundary_mask",
        ),
        "lower-proof reject and all-boundary survivor/mask protocol",
    )
    require(
        proposal.count(
            "HPolytopeProposalDeviceRecordStatus::proposed_strict_reject"
        )
        == 1
        and proposal.count(
            "HPolytopeProposalDeviceRecordStatus::proposed_survivor"
        )
        == 1,
        "record verdicts must have one conservative publication site each",
    )

    require(
        re.search(
            r"(?:canonical|canonicalize)[A-Za-z_]*zero[A-Za-z_]*\s*\(",
            clean,
            flags=re.IGNORECASE,
        )
        is not None
        and (
            re.search(
                r"(?:value|endpoint)\s*==\s*0(?:[.]0)?\s*\?\s*0(?:[.]0)?",
                clean,
                flags=re.IGNORECASE,
            )
            is not None
            or re.search(
                r"if\s*\(\s*(?:value|endpoint)\s*==\s*0(?:[.]0)?\s*\)\s*"
                r"\{?\s*(?:value|endpoint)\s*=\s*0(?:[.]0)?",
                clean,
                flags=re.IGNORECASE,
            )
            is not None
            or re.search(
                r"\(\s*bits\s*<<\s*1U?\s*\)\s*==\s*0U?",
                clean,
                flags=re.IGNORECASE,
            )
            is not None
        ),
        "binary64 interval endpoints do not visibly canonicalize -0 to +0",
    )
    require(
        re.search(
            r"contains[A-Za-z_]*zero\s*\([^)]*(?:value|evaluation|constraint|residual)",
            clean,
            flags=re.IGNORECASE,
        )
        is not None,
        "the incidence mask is not based on interval containment of zero",
    )

    forbidden_patterns = (
        r"\bepsilon\b",
        r"\btoleranc(?:e|es)\b",
        r"\bnextafter\b",
        r"--use_fast_math",
        r"(?<![A-Za-z0-9_])(?:__)?fma(?:f|_[a-z0-9_]+)?\s*\(",
        r"(?<![A-Za-z0-9_])sqrt\s*\(",
        r"(?<![A-Za-z0-9_])rsqrt\s*\(",
        r"(?<![A-Za-z0-9_])pow\s*\(",
        r"\batomicAdd\s*\(",
        r"\bcudaStreamDefault\b",
        r"\bHPolytopeProposalBatchDecision\b",
        r"\bexact_recertified_local\b",
        r"\binsufficient_exact_budget\b",
        r"\bglobal_status_published\b",
        r"\bcpu_exact_recertification_complete\b",
        r"\bpublic_status\b",
        r"\bcomplete_(?:empty|nonempty)\b",
    )
    for pattern in forbidden_patterns:
        require(
            re.search(pattern, clean, flags=re.IGNORECASE) is None,
            f"the CUDA proposal contains forbidden shortcut/verdict matching {pattern!r}",
        )


def validate_cmake(cmake: str, cuda_policy: str, presets: dict[str, Any]) -> None:
    target_match = re.search(
        r"add_library\(\s*"
        + re.escape(CUDA_TARGET)
        + r"\s+STATIC\s+"
        r"src/cuda/phase7_h_polytope_proposal_internal[.]hpp\s+"
        r"src/cuda/phase7_h_polytope_proposal[.]cu\s+"
        r"src/gpu/rational_binary64_enclosure[.]hpp\s+"
        r"src/gpu/h_polytope_proposal[.]cpp\s*\)",
        cmake,
        flags=re.DOTALL,
    )
    require(target_match is not None, "the Phase 7 CUDA target/source list is not closed")
    require_tokens(
        cmake,
        (
            f"morsehgp3d_configure_cuda_target({CUDA_TARGET})",
            f"PUBLIC morsehgp3d::spatial\n    PRIVATE Threads::Threads",
            f"target_compile_features(\n    {CUDA_TARGET}\n    PUBLIC cxx_std_20",
        ),
        "Phase 7 CUDA CMake target",
    )
    cuda_gate = cmake.rfind("if(MORSEHGP3D_ENABLE_CUDA)", 0, target_match.start())
    require(cuda_gate >= 0, "the Phase 7 proposal target is not CUDA-gated")
    for install_match in re.finditer(r"install\(\s*TARGETS\b.*?\)", cmake, re.DOTALL):
        require(
            CUDA_TARGET not in install_match.group(0),
            "the experimental Phase 7 CUDA proposal target must not be installed",
        )
    require_tokens(
        cuda_policy,
        (
            'set(_morsehgp3d_cuda_architecture "120-real")',
            'CUDA_ARCHITECTURES "120-real"',
            'CMAKE_CUDA_COMPILER_VERSION MATCHES "^12[.]9([.]|$)"',
            "$<$<COMPILE_LANGUAGE:CUDA>:--fmad=false>",
            "$<$<COMPILE_LANGUAGE:CUDA>:--ftz=false>",
            "$<$<COMPILE_LANGUAGE:CUDA>:--prec-div=true>",
            "$<$<COMPILE_LANGUAGE:CUDA>:--prec-sqrt=true>",
            "use_fast_math",
            "--generate-code",
            "--gpu-architecture",
            "--options-file",
        ),
        "closed CUDA 12.9 AOT policy",
    )
    configure = named_presets(presets.get("configurePresets"), "configurePresets")
    builds = named_presets(presets.get("buildPresets"), "buildPresets")
    for name in CUDA_PRESETS:
        require(name in configure and name in builds, f"missing CUDA preset {name}")
        cache = configure[name].get("cacheVariables")
        require(isinstance(cache, dict), f"{name} cacheVariables must be an object")
        require(cache.get("MORSEHGP3D_ENABLE_CUDA") == "ON", f"{name} is not CUDA")
        require(
            cache.get("CMAKE_CUDA_ARCHITECTURES") == "120-real",
            f"{name} does not freeze real sm_120 code",
        )
        targets = builds[name].get("targets")
        require(isinstance(targets, list), f"{name} targets must be an array")
        require(
            targets.count(CUDA_TARGET) == 1,
            f"{name} must build {CUDA_TARGET} exactly once",
        )
    for name in CPU_PRESETS:
        require(name in builds, f"missing CPU preset {name}")
        targets = builds[name].get("targets", [])
        require(isinstance(targets, list), f"{name} targets must be an array")
        require(CUDA_TARGET not in targets, f"CPU preset {name} builds a CUDA target")
    serialized = json.dumps(presets, separators=(",", ":"), sort_keys=True)
    for forbidden in ("--use_fast_math", "-use_fast_math", '"120"'):
        require(forbidden not in serialized, f"CUDA presets contain forbidden {forbidden}")


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
    public: str,
    internal: str,
    host: str,
    kernel: str,
    interval_header: str,
    fake_header: str,
    fake_source: str,
    unit_test: str,
    cmake: str,
    cuda_policy: str,
    presets: dict[str, Any],
) -> None:
    expect_text_rejected(
        validate_public_api,
        public,
        '"proposal_only_exhaustive_plane_triple_transcript"',
        '"gpu_exact_h_polytope"',
        "proposal promoted to a scientific verdict",
    )
    expect_text_rejected(
        validate_internal_contract,
        internal,
        "std::uint64_t status_code{0U}",
        "std::uint32_t status_code{0U}",
        "device record ABI narrowing",
    )
    expect_text_rejected(
        validate_host,
        host,
        "cell_status !=\n            HPolytopeProposalCellStatus::validated_exhaustive_transcript &&\n        row_size != 0U",
        "false",
        "partial fallback row acceptance",
    )

    def validate_mutated_fake_header(candidate: str) -> None:
        validate_fake(candidate, fake_source, unit_test)

    expect_text_rejected(
        validate_mutated_fake_header,
        fake_header,
        "omitted_true_incidence",
        "omitted_incidence_test_removed",
        "incidence-corruption oracle removal",
    )
    expect_text_rejected(
        validate_shared_interval,
        interval_header,
        "__dadd_rd(left.lower, right.lower)",
        "left.lower + right.lower",
        "shared directed lower addition removal",
    )
    expect_text_rejected(
        lambda candidate: validate_cuda_kernel(candidate, interval_header),
        kernel,
        "__ddiv_rd",
        "ordinary_division_rd",
        "directed lower division removal",
    )
    expect_text_rejected(
        lambda candidate: validate_cuda_kernel(candidate, interval_header),
        kernel,
        "cudaStreamNonBlocking",
        "cudaStreamDefault",
        "nondefault stream removal",
    )
    expect_text_rejected(
        lambda candidate: validate_cuda_kernel(candidate, interval_header),
        kernel,
        "cudaMemsetAsync(",
        "removed_capacity_memset(",
        "physical-capacity zeroing removal",
    )
    expect_text_rejected(
        lambda candidate: validate_cuda_kernel(candidate, interval_header),
        kernel,
        "__CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9",
        "false",
        "CUDA 12.9 guard removal",
    )
    injected = kernel + "\nvoid forbidden_phase7_math() { nextafter(0.0, 1.0); }\n"
    try:
        validate_cuda_kernel(injected, interval_header)
    except ContractError:
        pass
    else:
        raise ContractError("negative source mutation was accepted: nextafter shortcut")

    def validate_mutated_cmake(candidate: str) -> None:
        validate_cmake(candidate, cuda_policy, presets)

    expect_text_rejected(
        validate_mutated_cmake,
        cmake,
        "src/cuda/phase7_h_polytope_proposal.cu",
        "src/cuda/phase7_h_polytope_proposal.cpp",
        "CUDA translation unit removed from target",
    )
    mutated_presets = copy.deepcopy(presets)
    builds = named_presets(mutated_presets.get("buildPresets"), "mutated buildPresets")
    targets = builds["cuda-release"].get("targets")
    require(isinstance(targets, list), "mutated cuda-release targets are missing")
    targets.remove(CUDA_TARGET)
    try:
        validate_cmake(cmake, cuda_policy, mutated_presets)
    except ContractError:
        return
    raise ContractError("negative preset mutation was accepted: missing Phase 7 target")


def validate_project(project: Path) -> dict[str, object]:
    paths = {
        "public": project / PUBLIC_HEADER,
        "internal": project / INTERNAL_HEADER,
        "interval": project / INTERVAL_HEADER,
        "host": project / HOST_SOURCE,
        "kernel": project / CUDA_SOURCE,
        "fake_header": project / FAKE_HEADER,
        "fake_source": project / FAKE_SOURCE,
        "unit_test": project / UNIT_TEST_SOURCE,
        "cmake": project / CMAKE_SOURCE,
        "cuda_policy": project / CUDA_POLICY,
        "presets": project / PRESETS_SOURCE,
    }
    texts = {
        name: read_text(path)
        for name, path in paths.items()
        if name != "presets"
    }
    presets = strict_json(paths["presets"])

    validate_public_api(texts["public"])
    validate_internal_contract(texts["internal"])
    validate_host(texts["host"])
    validate_fake(texts["fake_header"], texts["fake_source"], texts["unit_test"])
    validate_cuda_kernel(texts["kernel"], texts["interval"])
    validate_cmake(texts["cmake"], texts["cuda_policy"], presets)
    validate_mutation_sensitivity(
        texts["public"],
        texts["internal"],
        texts["host"],
        texts["kernel"],
        texts["interval"],
        texts["fake_header"],
        texts["fake_source"],
        texts["unit_test"],
        texts["cmake"],
        texts["cuda_policy"],
        presets,
    )
    return {
        "checked_artifacts": sorted(
            str(path.relative_to(project)) for path in paths.values()
        ),
        "cuda_executed": False,
        "cuda_target": CUDA_TARGET,
        "decision_semantics": "reference_cpu_exact_all_constraints",
        "proposal_semantics": "proposal_only_exhaustive_plane_triple_transcript",
        "schema": "morsehgp3d.phase7.h_polytope_proposal_static.v1",
    }


def main(arguments: list[str]) -> int:
    if len(arguments) != 2:
        print(
            "usage: check_phase7_h_polytope_proposal.py PROJECT_SOURCE",
            file=sys.stderr,
        )
        return 2
    project = Path(arguments[1]).resolve()
    try:
        result = validate_project(project)
    except ContractError as error:
        print(f"Phase 7 H-polytope proposal contract failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, separators=(",", ":"), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
