#!/usr/bin/env python3
"""Validate the static no-mosaic contract of the direct Phase 9 targets."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


class ContractError(ValueError):
    """A direct Phase 9 target invariant was violated."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        raise ContractError(f"cannot read {path}: {error}") from error


def parenthesized_block(text: str, marker: str) -> str:
    start = text.find(marker)
    require(start >= 0, f"missing CMake marker {marker!r}")
    opening = text.find("(", start)
    require(opening >= 0, f"missing opening parenthesis after {marker!r}")
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "(":
            depth += 1
        elif text[index] == ")":
            depth -= 1
            if depth == 0:
                return text[opening : index + 1]
    raise ContractError(f"unterminated CMake block after {marker!r}")


def braced_block(text: str, marker: str) -> str:
    start = text.find(marker)
    require(start >= 0, f"missing source marker {marker!r}")
    opening = text.find("{", start)
    require(opening >= 0, f"missing opening brace after {marker!r}")
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[opening : index + 1]
    raise ContractError(f"unterminated source block after {marker!r}")


def parenthesized_calls(text: str, marker: str) -> list[str]:
    calls: list[str] = []
    cursor = 0
    while True:
        start = text.find(marker, cursor)
        if start < 0:
            return calls
        opening = text.find("(", start)
        require(opening >= 0, f"missing opening parenthesis after {marker!r}")
        depth = 0
        for index in range(opening, len(text)):
            if text[index] == "(":
                depth += 1
            elif text[index] == ")":
                depth -= 1
                if depth == 0:
                    calls.append(text[opening : index + 1])
                    cursor = index + 1
                    break
        else:
            raise ContractError(f"unterminated call after {marker!r}")


def require_tokens_in_order(
    text: str, markers: tuple[str, ...], message: str
) -> None:
    cursor = 0
    for marker in markers:
        position = text.find(marker, cursor)
        require(position >= 0, f"{message}: missing ordered token {marker!r}")
        cursor = position + len(marker)


def validate(
    project: Path,
    binary: Path | None = None,
    nm_executable: Path | None = None,
) -> dict[str, object]:
    cmake = read_text(project / "CMakeLists.txt")
    header = read_text(
        project / "include/morsehgp3d/hierarchy/pair_support_stream.hpp"
    )
    source = read_text(project / "src/cpu/hierarchy/pair_support_stream.cpp")
    codec_header = read_text(
        project / "include/morsehgp3d/hierarchy/pair_support_stream_codec.hpp"
    )
    codec_source = read_text(
        project / "src/cpu/hierarchy/pair_support_stream_codec.cpp"
    )
    durable_header = read_text(
        project / "include/morsehgp3d/hierarchy/pair_support_stream_durable.hpp"
    )
    durable_source = read_text(
        project / "src/cpu/hierarchy/pair_support_stream_durable.cpp"
    )
    higher_product_header = read_text(
        project / "include/morsehgp3d/hierarchy/higher_support_product.hpp"
    )
    higher_product_source = read_text(
        project / "src/cpu/hierarchy/higher_support_product.cpp"
    )
    higher_stream_header = read_text(
        project / "include/morsehgp3d/hierarchy/higher_support_stream.hpp"
    )
    higher_stream_source = read_text(
        project / "src/cpu/hierarchy/higher_support_stream.cpp"
    )
    terminal_header = read_text(
        project / "include/morsehgp3d/hierarchy/direct_support_terminal.hpp"
    )
    terminal_source = read_text(
        project / "src/cpu/hierarchy/direct_support_terminal.cpp"
    )
    gpu_phi_header = read_text(
        project / "include/morsehgp3d/gpu/pair_support_phi.hpp"
    )
    gpu_phi_source = read_text(project / "src/gpu/pair_support_phi.cpp")
    gpu_phi_cuda = read_text(
        project / "src/cuda/phase9_pair_support_phi.cu"
    )
    phase9_gcp_assembler = read_text(
        project / "tests/cuda/assemble_phase9_pair_support_phi_qualification.py"
    )
    phase3_orchestrator = read_text(
        project.parent / "gcp-migration/run_phase3_qualification.sh"
    )
    phase3_worker = read_text(
        project.parent / "gcp-migration/phase3_remote_qualification.sh"
    )

    for marker in (
        '--phase9-pair-support-phi-output',
        'begin_unit "phase9-pair-support-phi-build"',
        '--target morsehgp3d_gpu_pair_support_phi_qualification',
        'begin_unit "phase9-pair-support-phi-qualification"',
        'begin_unit "phase9-pair-support-phi-cuobjdump-elf"',
        'begin_unit "phase9-pair-support-phi-cuobjdump-ptx"',
        'begin_unit "phase9-pair-support-phi-memcheck"',
        'begin_unit "phase9-pair-support-phi-racecheck"',
        '"strict_interior_count": 1',
        '"descend_count": 1',
        '"first_epoch": 1',
        '"second_epoch": 2',
        'PHASE9_PAIR_SUPPORT_PHI_ASSEMBLER',
    ):
        require(
            marker in phase3_worker,
            f"the guarded Phase 9 worker is missing {marker!r}",
        )
    require_tokens_in_order(
        phase3_worker,
        (
            'read_guest_shutdown_guard ||',
            'begin_unit "phase9-pair-support-phi-build"',
            'begin_unit "phase9-pair-support-phi-qualification"',
            'begin_unit "phase9-pair-support-phi-cuobjdump-elf"',
            'begin_unit "phase9-pair-support-phi-cuobjdump-ptx"',
            'begin_unit "phase9-pair-support-phi-memcheck"',
            'begin_unit "phase9-pair-support-phi-racecheck"',
            'python3 -B "${PHASE9_PAIR_SUPPORT_PHI_ASSEMBLER}"',
        ),
        "the guarded Phase 9 worker does not certify guard -> build -> execute -> AOT -> sanitizers -> assembly",
    )
    for marker in (
        '--phase9-pair-support-phi',
        'phase9-pair-support-phi-${HEAD_SHA}.json',
        '--phase9-pair-support-phi-output',
        'LOCAL_PHASE9_PAIR_SUPPORT_PHI_TEMP_RESULT',
        'assembler.SCHEMA',
        'if certify_target_stopped; then',
        '"targeted_stop_verified": True',
    ):
        require(
            marker in phase3_orchestrator,
            f"the guarded Phase 9 orchestrator is missing {marker!r}",
        )
    require_tokens_in_order(
        phase3_orchestrator,
        (
            'gcloud compute scp',
            'if ((PHASE9_PAIR_SUPPORT_PHI == 1)); then\n    [[ -s',
            'if certify_target_stopped; then',
            'python3 - "${LOCAL_TEMP_RESULT}" "${LOCAL_RESULT}"',
            'Qualification pair-support Phi Phase 9 publiée après certification TERMINATED',
        ),
        "the Phase 9 artifact is not validate -> targeted stop -> no-replace publish",
    )
    for marker in (
        'SCHEMA = "morsehgp3d.phase9.pair_support_phi_cuda_g4_qualification.v1"',
        '"strict_interior_count": 1',
        '"descend_count": 1',
        '"first_epoch": 1',
        '"second_epoch": 2',
        '"cpu_exact_recertification": "passed"',
        '"scientific_result_claimed": False',
        '"scientific_public_status": None',
    ):
        require(
            marker in phase9_gcp_assembler,
            f"the Phase 9 G4 assembler is missing {marker!r}",
        )

    pair_target = parenthesized_block(
        cmake, "add_library(\n  morsehgp3d_pair_support"
    )
    require(
        pair_target.count("src/cpu/hierarchy/pair_support_stream.cpp") == 1,
        "the pair target must contain exactly its direct producer source",
    )
    require("gamma" not in pair_target.lower(), "the pair target contains Gamma")
    require("ordinary" not in pair_target.lower(), "the pair target contains cells")

    pair_links = parenthesized_block(
        cmake, "target_link_libraries(\n  morsehgp3d_pair_support"
    )
    require(
        "PUBLIC morsehgp3d::spatial" in pair_links,
        "the pair target must depend on the certified spatial layer",
    )
    require(
        "morsehgp3d::hierarchy" not in pair_links,
        "the pair target must not link the historical Gamma archive",
    )

    codec_target = parenthesized_block(
        cmake, "add_library(\n  morsehgp3d_pair_support_codec"
    )
    require(
        codec_target.count(
            "src/cpu/hierarchy/pair_support_stream_codec.cpp"
        )
        == 1,
        "the codec target must contain exactly its canonical codec source",
    )
    codec_links = parenthesized_block(
        cmake, "target_link_libraries(\n  morsehgp3d_pair_support_codec"
    )
    require(
        "PUBLIC morsehgp3d::pair_support" in codec_links,
        "the codec target must depend on the direct pair stream",
    )
    require(
        "morsehgp3d::hierarchy" not in codec_links,
        "the codec target must not link the historical Gamma archive",
    )

    durable_target = parenthesized_block(
        cmake, "add_library(\n    morsehgp3d_pair_support_durable"
    )
    require(
        durable_target.count(
            "src/cpu/hierarchy/pair_support_stream_durable.cpp"
        )
        == 1,
        "the durable target must contain exactly its Unix sink source",
    )
    durable_links = parenthesized_block(
        cmake, "target_link_libraries(\n    morsehgp3d_pair_support_durable"
    )
    require(
        "PUBLIC morsehgp3d::pair_support_codec" in durable_links,
        "the durable target must depend on the bounded codec",
    )
    require(
        "morsehgp3d::hierarchy" not in durable_links,
        "the durable target must not link the historical Gamma archive",
    )

    higher_target = parenthesized_block(
        cmake, "add_library(\n  morsehgp3d_higher_support"
    )
    require(
        higher_target.count(
            "src/cpu/hierarchy/higher_support_product.cpp"
        )
        == 1
        and higher_target.count(
            "src/cpu/hierarchy/higher_support_stream.cpp"
        )
        == 1,
        "the higher-support target must contain exactly its product and direct stream sources",
    )
    require(
        "pair_support_stream" not in higher_target,
        "the higher-support target must remain independent of the pair checkpoint wire",
    )
    require(
        "gamma" not in higher_target.lower(),
        "the higher-support target contains Gamma",
    )
    require(
        "ordinary" not in higher_target.lower(),
        "the higher-support target contains cells",
    )
    higher_links = parenthesized_block(
        cmake, "target_link_libraries(\n  morsehgp3d_higher_support"
    )
    require(
        "PUBLIC morsehgp3d::spatial" in higher_links,
        "the higher-support target must depend on the certified spatial layer",
    )
    require(
        "morsehgp3d::contract" in higher_links,
        "the higher-support checkpoint must depend on canonical SHA-256",
    )
    require(
        "morsehgp3d::hierarchy" not in higher_links,
        "the higher-support target must not link the historical Gamma archive",
    )

    terminal_target = parenthesized_block(
        cmake, "add_library(\n  morsehgp3d_direct_support_terminal"
    )
    require(
        terminal_target.count(
            "src/cpu/hierarchy/direct_support_terminal.cpp"
        )
        == 1,
        "the terminal facade target must contain exactly its composition source",
    )
    terminal_links = parenthesized_block(
        cmake, "target_link_libraries(\n  morsehgp3d_direct_support_terminal"
    )
    require(
        "morsehgp3d::pair_support" in terminal_links
        and "morsehgp3d::higher_support" in terminal_links,
        "the terminal facade must compose both direct support lanes",
    )
    require(
        "morsehgp3d::hierarchy" not in terminal_links,
        "the terminal facade must not link the historical Gamma archive",
    )

    gpu_phi_target = parenthesized_block(
        cmake, "add_library(\n    morsehgp3d_gpu_pair_support_phi"
    )
    require(
        gpu_phi_target.count("src/cuda/phase9_pair_support_phi.cu") == 1
        and gpu_phi_target.count("src/gpu/pair_support_phi.cpp") == 1,
        "the CUDA phi target must contain one proposal kernel and one host recertifier",
    )
    gpu_phi_links = parenthesized_block(
        cmake, "target_link_libraries(\n    morsehgp3d_gpu_pair_support_phi"
    )
    require(
        "PUBLIC morsehgp3d::pair_support" in gpu_phi_links,
        "the CUDA phi target must depend on the certified pair-support layer",
    )
    require(
        "morsehgp3d::hierarchy" not in gpu_phi_links,
        "the CUDA phi target must not link the historical Gamma archive",
    )

    hierarchy_target = parenthesized_block(
        cmake, "add_library(\n  morsehgp3d_hierarchy"
    )
    require(
        "pair_support_stream.cpp" not in hierarchy_target,
        "the direct pair source leaked back into the Gamma archive",
    )
    require(
        "higher_support_product.cpp" not in hierarchy_target
        and "higher_support_stream.cpp" not in hierarchy_target,
        "the direct higher-support sources leaked back into the Gamma archive",
    )

    combined = "\n".join(
        (
            header,
            source,
            codec_header,
            codec_source,
            durable_header,
            durable_source,
        )
    )
    for forbidden in (
        "hierarchy/gamma.hpp",
        "critical_catalog",
        "ordinary_cell",
        "ordinary_diagram",
        "depth_zero_natural_support",
        "lbvh_closed_ball(",
        "std::vector<std::array<spatial::PointId, 2>>",
        "std::vector<std::array<PointId, 2>>",
    ):
        require(
            forbidden not in combined,
            f"the direct pair implementation contains forbidden token {forbidden!r}",
        )
    for required in (
        "remaining_frontier",
        "maximum_frontier_entry_count",
        "maximum_emitted_record_count",
        "maximum_emitted_point_id_reference_count",
        "ExactPairSupportStreamStatus::budget_exhausted",
        "no_forbidden_global_structure_materialized = true",
        "class ExactPairSupportAuthorityContext",
        "class ExactPairSupportIncrementalVerifier",
        "class PreparedNext",
        "trusted_next_checkpoint()",
        "prepare_next(",
        "commit_prepared(",
        "verify_next(",
        "quasi_linear_structure_validation_certified",
        "product_rectangles_are_disjoint",
        "witness_intervals_are_disjoint",
        "std::map<std::uint64_t, ActiveProductRectangle>",
        "manifest_for_with_audit",
        "audit->canonical_cloud_point_hash_count = checked_add",
        "source_checkpoint_already_trusted ||",
        "ExactPairSupportAuthorityContext&&) = delete",
        "spatial::MortonLbvhIndex&&,",
        "spatial::CanonicalPointCloud&&,",
        "ExactPairSupportAuthorityContext authority_;",
        "catch (...) {\n    poison();",
    ):
        require(required in combined, f"missing bounded-stream token {required!r}")

    higher_combined = "\n".join(
        (
            higher_product_header,
            higher_product_source,
            higher_stream_header,
            higher_stream_source,
        )
    )
    for forbidden in (
        "hierarchy/gamma.hpp",
        "critical_catalog",
        "ordinary_cell",
        "ordinary_diagram",
        "depth_zero_natural_support",
        "lbvh_closed_ball(",
        "std::vector<std::array<spatial::PointId, 3>>",
        "std::vector<std::array<spatial::PointId, 4>>",
        "std::vector<std::array<PointId, 3>>",
        "std::vector<std::array<PointId, 4>>",
    ):
        require(
            forbidden not in higher_combined,
            f"the direct higher-support implementation contains forbidden token {forbidden!r}",
        )
    for required in (
        "struct ExactHigherSupportNodeGroup",
        "std::uint8_t multiplicity",
        "struct ExactHigherSupportFrontierEntry",
        "exact::BigInt total_support_count",
        "exact::BigInt remaining_frontier_support_count",
        "exact_higher_support_candidate_universe_size(",
        "exact_binomial(",
        "child_coverage != entry_support_count(entry)",
        "exact_higher_support_product_aabb_analysis(",
        "query_strictly_inside_every_independent_sphere_certified()",
        "maximum_frontier_entry_count",
        "maximum_auxiliary_frontier_entry_count",
        "remaining_frontier",
        "ExactHigherSupportStreamStatus::budget_exhausted",
        "analyze_circumcenter_support(",
        "minimum_squared_distance_to_node(",
        "maximum_squared_distance_to_node(",
        "canonical_extra_shell_witness_id",
        "grouped_partition_accounting_certified",
        "no_forbidden_global_structure_materialized = true",
        "hierarchy_reduction_performed = false",
        "struct ExactHigherSupportPendingProduct",
        "struct ExactHigherSupportCheckpointManifest",
        "class ExactHigherSupportAuthorityContext",
        "struct ExactHigherSupportCheckpoint",
        "struct ExactHigherSupportStreamChunk",
        "std::vector<ExactHigherSupportNodeReceipt> rank_frontier",
        "std::vector<ExactHigherSupportNodeReceipt> strict_interior_receipts",
        "source_checkpoint_digest",
        "output_chain_digest",
        "make_initial_exact_higher_support_checkpoint(",
        "class ExactHigherSupportAnchoredSession",
        "ExactHigherSupportAnchoredSession::prepare_next(",
        "ExactHigherSupportAnchoredSession::commit_prepared(",
        "build_exact_higher_support_stream_chunk_after_source_verification(",
        "verify_exact_higher_support_checkpoint(",
        "verify_exact_higher_support_stream_chunk_from_trusted_source(",
        "verify_exact_higher_support_stream_run(",
        "checkpoint.output_record_count == audit.emitted_record_count",
        "pending_product_->product != frontier_.back()",
        "reinjected_source != trusted_checkpoint_",
        "source_checkpoint_anchored",
        "freshly_replayed_next",
        "output-record-chain/v2/prune",
        "receipt_payload_within_caps",
        "pending.strict_interior_receipts.size() <=",
        "pending.rank_frontier.size() <= maximum_rank_frontier_size",
        "verify_exact_higher_support_stream(",
        "verification.fresh_replay_certified = observed == expected",
    ):
        require(
            required in higher_combined,
            f"missing bounded higher-support token {required!r}",
        )

    require(
        "append_product_analysis(" not in higher_stream_source,
        "the compact higher output chain must recompute interval analyses",
    )

    terminal_combined = terminal_header + "\n" + terminal_source
    gpu_phi_combined = gpu_phi_header + "\n" + gpu_phi_source + "\n" + gpu_phi_cuda
    for forbidden in (
        "hierarchy/gamma.hpp",
        "critical_catalog",
        "ordinary_cell",
        "ordinary_diagram",
        "depth_zero_natural_support",
    ):
        require(
            forbidden not in terminal_combined.lower(),
            f"the terminal facade contains forbidden token {forbidden!r}",
        )
        require(
            forbidden not in gpu_phi_combined.lower(),
            f"the CUDA phi path contains forbidden token {forbidden!r}",
        )
    for required in (
        "pair_canonical_cloud_digest",
        "higher_canonical_cloud_digest",
        "direct_support_catalog_arities_two_through_four_only",
        "fresh_exact_pair_v1_and_grouped_higher_v2_replay_terminal_",
        "common_durable_checkpoint_certified{false}",
        "hierarchy_or_forest_certified{false}",
        "public_status_claimed{false}",
    ):
        require(
            required in terminal_combined,
            f"missing terminal composition token {required!r}",
        )
    for required in (
        "cuda_outward_binary64_diametral_phi_upper_bound_proposal_only",
        "cpu_exact_dyadic_aabb_phi_recertified_strict_witness_receipt_only",
        "exact_diametral_phi_aabb_maximum(",
        "exact_receipt.maximum_phi > proposed_upper",
        "exact_receipt.maximum_phi.sign() >= 0",
        "global_support_product_prune_published = false",
        "public_status_published = false",
        "maximum_query_count",
        "__CUDA_ARCH__ != 1200",
    ):
        require(
            required in gpu_phi_combined,
            f"missing CUDA proposal/CPU recertification token {required!r}",
        )

    verifier_start = header.find("class ExactPairSupportIncrementalVerifier")
    verifier_end = header.find(
        "struct ExactPairSupportStreamRunVerification", verifier_start
    )
    require(
        verifier_start >= 0 and verifier_end > verifier_start,
        "cannot isolate the incremental verifier declaration",
    )
    verifier_declaration = header[verifier_start:verifier_end]
    require(
        "std::vector<ExactPairSupportStreamChunk" not in verifier_declaration,
        "the incremental verifier must not retain prior chunks",
    )
    require(
        source.count("manifest_for(") == 2,
        "the cached-manifest path must have only one definition and one cold constructor call",
    )
    require(
        source.count("manifest_for_with_audit(") == 3,
        "the instrumented manifest builder must be called only by its cold wrapper and the authority constructor",
    )
    require(
        "trusted_checkpoint_ = std::move(prepared.trusted_next_);" in source,
        "commit_prepared must advance with the freshly replayed expected checkpoint",
    )
    require(
        "trusted_checkpoint_ = observed.next_checkpoint" not in source,
        "verify_next must never trust an observed checkpoint directly",
    )
    require(
        "std::vector<ExactPairSupportStreamChunk" not in durable_source,
        "the durable sink must not retain decoded chunk history",
    )
    require(
        "observed.next_checkpoint.checkpoint_digest" not in durable_source,
        "durable HEAD and anchors must use the freshly replayed trusted checkpoint",
    )

    chunk_encoder = braced_block(
        codec_source,
        "std::vector<std::uint8_t> encode_exact_pair_support_stream_chunk(",
    )
    require(
        chunk_encoder.count("VectorWriter writer{") == 1
        and "encode_envelope_header(writer, payload_byte_count);"
        in chunk_encoder
        and "encode_counted_payload(" in chunk_encoder,
        "the vector chunk encoder must write its envelope and payload through one writer",
    )
    for forbidden_codec_token in (
        "sizeof(ExactPairSupport",
        "std::memcpy",
        "const std::vector<std::uint8_t> payload",
        "wire.bytes(payload)",
    ):
        require(
            forbidden_codec_token not in codec_source,
            f"the codec contains raw-layout token {forbidden_codec_token!r}",
        )
    for required_codec_token in (
        "maximum_encoded_byte_count",
        "maximum_frontier_entry_count",
        "maximum_auxiliary_entry_count",
        "maximum_record_count",
        "maximum_point_id_reference_count",
        "maximum_exact_text_byte_count",
        "maximum_total_exact_text_byte_count",
        "ExactPairSupportStreamDecodeDecision",
        "checksum_mismatch",
        "trailing_bytes",
        "pair_support_stream_fd_buffer_byte_count",
        "pair_support_stream_default_maximum_exact_text_byte_count = 2048U",
        "encode_exact_pair_support_stream_chunk_to_fd(",
        "verify_exact_pair_support_stream_chunk_fd_wire(",
        "decode_exact_pair_support_stream_chunk_from_fd(",
        "verify_fd_wire_with_scratch(",
        "class CountWriter",
        "class BufferedFdWriter",
        "canonical_rational_allocation_is_bounded",
    ):
        require(
            required_codec_token in codec_header + codec_source,
            f"missing bounded-codec token {required_codec_token!r}",
        )

    fd_reader_block = braced_block(codec_source, "class FdByteReader")
    require(
        "std::span<std::uint8_t> buffer_;" in fd_reader_block
        and "std::array<std::uint8_t, pair_support_stream_fd_buffer_byte_count>"
        not in fd_reader_block,
        "the fd payload reader must borrow rather than own the 64-KiB scratch",
    )
    fd_decoder_block = braced_block(
        codec_source,
        "decode_exact_pair_support_stream_chunk_from_fd(",
    )
    require(
        fd_decoder_block.count(
            "std::array<std::uint8_t, pair_support_stream_fd_buffer_byte_count>"
        )
        == 1
        and fd_decoder_block.count("verify_fd_wire_with_scratch(") == 2
        and "FdByteReader payload_reader{" in fd_decoder_block,
        "fd decode must reuse one physical 64-KiB scratch across both checksum passes and parsing",
    )

    for required_factory_token in (
        "static ExactPairSupportDurableSink create_new(",
        "static ExactPairSupportDurableSink open_existing(",
        "struct ExactPairSupportDurableExternalPrefixAnchor",
        "std::optional<ExactPairSupportDurableExternalPrefixAnchor>",
        "enum class OpenMode",
        "pair_support_durable_schema_version = 2U",
        "ExactPairSupportDurableSink::create_new(",
        "ExactPairSupportDurableSink::open_existing(",
        "OpenMode::create_new",
        "OpenMode::open_existing",
        "open_existing requires an authoritative durable pair-support HEAD",
        "external_prefix_anchor_verified",
        "expected_prefix_anchor->committed_transition_count >",
        "expected_prefix_anchor->checkpoint_digest !=",
    ):
        require(
            required_factory_token in durable_header + durable_source,
            f"missing explicit durable-open token {required_factory_token!r}",
        )

    for required_head_token in (
        "constexpr std::array<std::uint8_t, 16U> head_magic{",
        "'A', 'I', 'R', '-', 'H', 'E', 'A', 'D'",
        "constexpr std::uint32_t head_wire_version = 1U;",
        "constexpr std::uint8_t head_wire_kind = 2U;",
        "constexpr std::uint8_t head_wire_flags = 0U;",
        "constexpr std::size_t head_payload_byte_count = 80U;",
        "constexpr std::size_t head_wire_byte_count =",
        "static_assert(head_wire_byte_count == 142U);",
        "head_checksum_domain",
        "compute_run_contract_digest(",
        "encode_head(const HeadRecord& head)",
        "decode_head(",
        "contract::CanonicalId run_contract_digest{};",
        "std::uint64_t committed_transition_count{};",
        "std::uint64_t total_encoded_byte_count{};",
        "contract::CanonicalId committed_checkpoint_digest{};",
    ):
        require(
            required_head_token in durable_source,
            f"missing fixed authoritative-HEAD token {required_head_token!r}",
        )

    for required_durable_token in (
        "O_NOFOLLOW",
        "O_NONBLOCK",
        "fdatasync",
        "fsync(directory.get())",
        "flock",
        "prepare_next(",
        "commit_prepared(",
        "integrity_failure = true;",
        "head_replaced || integrity_failure",
        "maximum_simultaneously_decoded_chunk_count = 1U",
        "validate_inventory_against_head(",
        "for (std::size_t sequence = 0U; sequence < head_count; ++sequence)",
        "retained_chunk_history_count = 0U",
        "persistent_top_m_cell_count = 0U",
        "global_gamma_coface_count = 0U",
        "global_gamma_incidence_count = 0U",
        "materialized_pair_arena_count = 0U",
        "maximum_codec_io_buffer_byte_count =",
        "materialized_transition_wire_byte_count",
        "streaming_fd_codec_used = true",
    ):
        require(
            required_durable_token in durable_header + durable_source,
            f"missing durable-publication token {required_durable_token!r}",
        )

    rename_calls = parenthesized_calls(durable_source, "::renameat")
    require(
        len(rename_calls) == 1
        and all(
            "head_temporary_file_name" in call
            and "head_file_name" in call
            and "final_name" not in call
            for call in rename_calls
        ),
        "renameat must be reserved to successor HEAD replacement",
    )
    link_calls = parenthesized_calls(durable_source, "::linkat")
    require(
        len(link_calls) == 2,
        "initial HEAD and transition publication must each use one no-replace linkat call",
    )

    initialize_block = braced_block(durable_source, "void initialize_new()")
    initial_head_links = parenthesized_calls(initialize_block, "::linkat")
    require(
        len(initial_head_links) == 1
        and "head_temporary_file_name" in initial_head_links[0]
        and "head_file_name" in initial_head_links[0]
        and "::renameat(" not in initialize_block,
        "create_new must publish HEAD0 once with no replacement and no rename",
    )
    for interrupted_initial_token in (
        "lone_initial_temporary",
        "validate_head_temporary(head)",
        "cannot remove an interrupted initial durable pair-support HEAD temporary",
    ):
        require(
            interrupted_initial_token in initialize_block,
            "create_new must recover only its lone validated uncommitted HEAD0 temporary",
        )
    require_tokens_in_order(
        initialize_block,
        (
            "create_temporary_file(",
            "::fdatasync(temporary.get())",
            "::linkat(",
            "::unlinkat(",
            "::fsync(directory.get())",
        ),
        "initial HEAD publication is not temporary -> sync -> link -> unlink -> directory",
    )

    publish_block = braced_block(
        durable_source,
        "[[nodiscard]] ExactPairSupportDurablePublishResult publish(",
    )
    require_tokens_in_order(
        publish_block,
        (
            "committed_transition_count >=",
            "remaining_total_encoded_byte_count =",
            "effective_codec_limits.maximum_encoded_byte_count = std::min(",
            "observed, effective_codec_limits",
        ),
        "publication must tighten count and total-byte caps before encoding",
    )
    require(
        "encode_exact_pair_support_stream_chunk(\n" not in publish_block,
        "the durable publication path must not materialize a vector wire",
    )
    require_tokens_in_order(
        publish_block,
        (
            "encode_exact_pair_support_stream_chunk_to_fd(",
            "transition_temporary_file_written",
            "::fdatasync(transition_temporary.get())",
            "verify_exact_pair_support_stream_chunk_fd_wire(",
            "transition_temporary_file_synchronized",
            "::linkat(",
        ),
        "durable fd publication is not encode -> sync -> bounded readback -> link",
    )
    for required_publication_identity_token in (
        "same_inode(linked_descriptor, linked_temporary)",
        "same_inode(linked_descriptor, linked_final)",
        "same_inode(verified_head_descriptor, verified_head_name)",
        "same_inode(verified_head_descriptor, published_head)",
        "require_verified_transition_receipt();",
        "the observer-visible published durable pair-support HEAD",
    ):
        require(
            required_publication_identity_token in publish_block,
            "durable fd publication is missing descriptor/name/content binding "
            + repr(required_publication_identity_token),
        )
    require(
        publish_block.count("require_verified_transition_receipt();") >= 2
        and publish_block.count("require_verified_publication();") >= 3,
        "observer-visible durability boundaries must be reauthenticated before in-memory commit",
    )
    require(
        publish_block.count("::linkat(") == 1
        and publish_block.count("::renameat(") == 1
        and publish_block.count("verifier.commit_prepared(") == 1,
        "the publication path must have one chunk link, one HEAD replacement and one in-memory commit",
    )
    transition_links = parenthesized_calls(publish_block, "::linkat")
    require(
        len(transition_links) == 1
        and "temporary_name.c_str()" in transition_links[0]
        and "final_name.c_str()" in transition_links[0]
        and "head_file_name" not in transition_links[0],
        "publish must create the immutable transition final with one no-replace link",
    )
    require_tokens_in_order(
        publish_block,
        (
            "::linkat(",
            "transition_final_link_created",
            "::unlinkat(directory.get(), temporary_name.c_str(), 0)",
            "transition_temporary_link_removed",
            "::fsync(directory.get())",
            "transition_directory_synchronized",
            "head_temporary = create_temporary_file(",
            "head_temporary_file_written",
            "::fdatasync(head_temporary.get())",
            "head_temporary_file_synchronized",
            "::renameat(",
            "head_replaced",
            "::fsync(directory.get())",
            "head_directory_synchronized",
            "verifier.commit_prepared(",
        ),
        "durable publication order is not chunk -> directory -> HEAD -> directory -> memory",
    )

    recovery_block = braced_block(durable_source, "void recover_existing()")
    require(
        "sequence < inventory.final_file_count" not in recovery_block,
        "recovery must never derive its authoritative replay length from directory inventory",
    )
    require_tokens_in_order(
        recovery_block,
        (
            "authoritative_head = read_head_file(",
            "const std::size_t head_count = checked_size(",
            "validate_inventory_against_head(",
            "sequence < head_count",
            "committed_transition_count != head_count",
            "validate_and_remove_uncommitted_suffix(",
        ),
        "recovery is not bounded and certified by authoritative HEAD",
    )
    read_transition_block = braced_block(
        durable_source, "[[nodiscard]] OpenedReadFile read_transition("
    )
    require_tokens_in_order(
        read_transition_block,
        (
            "remaining_total = maximum_total - current_total",
            "maximum_file_byte_count = std::min(",
            "config.codec_limits.maximum_encoded_byte_count",
            "remaining_total",
            "open_bounded_regular_file(",
            "maximum_file_byte_count",
        ),
        "recovery must tighten the file allocation cap to the HEAD byte remainder",
    )
    decode_transition_block = braced_block(
        durable_source,
        "[[nodiscard]] ExactPairSupportStreamChunk decode_transition(",
    )
    require(
        "decode_exact_pair_support_stream_chunk_from_fd("
        in decode_transition_block
        and "read.bytes" not in decode_transition_block,
        "durable recovery must decode directly from the bounded descriptor",
    )
    inventory_validation = braced_block(
        durable_source, "void validate_inventory_against_head("
    )
    require(
        "const std::size_t maximum_allowed_count = checked_add("
        in inventory_validation
        and "committed_count," in inventory_validation
        and "inventory.final_file_count < committed_count" in inventory_validation
        and "inventory.final_file_count > maximum_allowed_count"
        in inventory_validation,
        "directory inventory must be bounded by HEAD plus at most one validated orphan",
    )
    event_start = source.find("struct ProductRectangleSweepEvent")
    event_end = source.find("\n  };", event_start)
    require(
        event_start >= 0 and event_end > event_start,
        "cannot isolate the compact rectangle sweep event",
    )
    event_declaration = source[event_start:event_end]
    require(
        "std::size_t rectangle_index" in event_declaration
        and "ProductRectangle rectangle" not in event_declaration,
        "rectangle sweep events must retain only a compact rectangle index",
    )

    binary_symbol_gate = False
    if binary is not None or nm_executable is not None:
        require(
            binary is not None and nm_executable is not None,
            "--binary and --nm must be supplied together",
        )
        require(binary.is_file(), f"missing pair-stream test binary {binary}")
        require(nm_executable.is_file(), f"missing symbol reader {nm_executable}")
        try:
            completed = subprocess.run(
                [str(nm_executable), "-C", str(binary)],
                check=False,
                capture_output=True,
                text=True,
                timeout=30,
            )
        except (OSError, subprocess.SubprocessError) as error:
            raise ContractError(f"cannot inspect pair binary symbols: {error}") from error
        require(completed.returncode == 0, "the symbol reader rejected the pair binary")
        symbols = completed.stdout.lower()
        for forbidden_symbol in (
            "ordinary_cell",
            "ordinary_diagram",
            "critical_catalog",
            "reduced_gamma",
            "gamma_transition",
            "morse_gamma",
            "depth_zero_natural_support",
        ):
            require(
                forbidden_symbol not in symbols,
                f"the pair binary contains forbidden symbol {forbidden_symbol!r}",
            )
        binary_symbol_gate = True

    return {
        "schema": "morsehgp3d.phase9.direct_support_static.v8",
        "targets": [
            "morsehgp3d_pair_support",
            "morsehgp3d_higher_support",
            "morsehgp3d_pair_support_codec",
            "morsehgp3d_pair_support_durable",
            "morsehgp3d_direct_support_terminal",
            "morsehgp3d_gpu_pair_support_phi",
        ],
        "persistent_top_m_cell_count": 0,
        "global_gamma_coface_count": 0,
        "global_gamma_incidence_count": 0,
        "materialized_pair_arena_count": 0,
        "materialized_higher_support_arena_count": 0,
        "higher_support_grouped_bigint_frontier": True,
        "higher_support_fresh_replay": True,
        "higher_support_reinjectable_checkpoint": True,
        "higher_support_anchored_in_memory_session": True,
        "higher_support_three_kind_output_chain": True,
        "terminal_direct_support_facade": True,
        "cuda_phi_proposal_cpu_exact_recertification": True,
        "persistent_authority_context": True,
        "incremental_verify_next": True,
        "two_phase_durable_verification": True,
        "bounded_canonical_codec": True,
        "single_buffer_chunk_encoding": True,
        "fixed_buffer_fd_encoding": True,
        "two_pass_fd_decoding": True,
        "materialized_transition_wire_byte_count": 0,
        "pair_binary64_exact_text_byte_cap": 2048,
        "explicit_create_and_open": True,
        "authoritative_local_head": True,
        "external_prefix_anchor": True,
        "no_replace_initial_head": True,
        "no_replace_transition_publication": True,
        "head_bounded_recovery": True,
        "unix_atomic_publication": True,
        "quasi_linear_checkpoint_validation": True,
        "binary_symbol_gate": binary_symbol_gate,
        "checked_artifacts": [
            "CMakeLists.txt",
            "include/morsehgp3d/hierarchy/pair_support_stream.hpp",
            "src/cpu/hierarchy/pair_support_stream.cpp",
            "include/morsehgp3d/hierarchy/pair_support_stream_codec.hpp",
            "src/cpu/hierarchy/pair_support_stream_codec.cpp",
            "include/morsehgp3d/hierarchy/pair_support_stream_durable.hpp",
            "src/cpu/hierarchy/pair_support_stream_durable.cpp",
            "include/morsehgp3d/hierarchy/higher_support_product.hpp",
            "src/cpu/hierarchy/higher_support_product.cpp",
            "include/morsehgp3d/hierarchy/higher_support_stream.hpp",
            "src/cpu/hierarchy/higher_support_stream.cpp",
            "include/morsehgp3d/hierarchy/direct_support_terminal.hpp",
            "src/cpu/hierarchy/direct_support_terminal.cpp",
            "include/morsehgp3d/gpu/pair_support_phi.hpp",
            "src/gpu/pair_support_phi.cpp",
            "src/cuda/phase9_pair_support_phi.cu",
            "tests/cuda/assemble_phase9_pair_support_phi_qualification.py",
            "../gcp-migration/run_phase3_qualification.sh",
            "../gcp-migration/phase3_remote_qualification.sh",
        ],
    }


def main(arguments: list[str]) -> int:
    if len(arguments) not in (2, 6):
        print(
            "usage: check_phase9_pair_support.py PROJECT_SOURCE "
            "[--binary EXECUTABLE --nm NM]",
            file=sys.stderr,
        )
        return 2
    binary = None
    nm_executable = None
    if len(arguments) == 6:
        if arguments[2] != "--binary" or arguments[4] != "--nm":
            print("invalid Phase 9 checker arguments", file=sys.stderr)
            return 2
        binary = Path(arguments[3]).resolve()
        nm_executable = Path(arguments[5]).resolve()
    try:
        result = validate(
            Path(arguments[1]).resolve(), binary, nm_executable
        )
    except ContractError as error:
        print(f"Phase 9 direct-target contract failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, separators=(",", ":"), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
