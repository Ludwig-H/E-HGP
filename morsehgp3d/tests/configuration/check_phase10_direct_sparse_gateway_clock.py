#!/usr/bin/env python3
"""Validate the isolated Phase-10.11-CLOCK sparse clock certificate target."""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


class ContractError(ValueError):
    """A Phase-10.11-CLOCK invariant was violated."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def require_all(text: str, needles: tuple[str, ...], context: str) -> None:
    for needle in needles:
        require(needle in text, f"{context} is missing {needle!r}")


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        raise ContractError(f"cannot read {path}: {error}") from error


def compact_whitespace(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def cmake_blocks(text: str, command: str) -> list[str]:
    blocks: list[str] = []
    marker = re.compile(rf"(?m)^\s*{re.escape(command)}\s*\(")
    for match in marker.finditer(text):
        opening = text.find("(", match.start())
        depth = 0
        for index in range(opening, len(text)):
            if text[index] == "(":
                depth += 1
            elif text[index] == ")":
                depth -= 1
                if depth == 0:
                    blocks.append(text[match.start() : index + 1])
                    break
        else:
            raise ContractError(f"unterminated CMake {command} block")
    return blocks


def unique_target_block(text: str, command: str, target: str) -> str:
    matches = [
        block
        for block in cmake_blocks(text, command)
        if re.match(
            rf"\s*{re.escape(command)}\s*\(\s*{re.escape(target)}(?:\s|\))",
            block,
        )
    ]
    require(
        len(matches) == 1,
        f"expected exactly one {command} block for {target}; found {len(matches)}",
    )
    return matches[0]


def unique_marked_block(text: str, command: str, marker: str, context: str) -> str:
    matches = [block for block in cmake_blocks(text, command) if marker in block]
    require(len(matches) == 1, f"expected one {context}; found {len(matches)}")
    return matches[0]


def enclosing_cmake_branches(text: str, byte_index: int) -> list[tuple[str, str]]:
    require(0 <= byte_index <= len(text), "invalid CMake source position")
    stack: list[tuple[str, str]] = []
    control = re.compile(r"(?im)^\s*(if|elseif|else|endif)\s*\(([^)]*)\)")
    for match in control.finditer(text, 0, byte_index):
        command = match.group(1).lower()
        condition = compact_whitespace(match.group(2))
        if command == "if":
            stack.append(("if", condition))
        elif command == "elseif":
            require(stack, "unbalanced CMake elseif before CLOCK wiring")
            stack[-1] = ("elseif", condition)
        elif command == "else":
            require(stack, "unbalanced CMake else before CLOCK wiring")
            require(condition == "", "a CMake else must not have arguments")
            stack[-1] = ("else", "")
        else:
            require(stack, "unbalanced CMake endif before CLOCK wiring")
            stack.pop()
    return stack


def strip_cpp_comments_and_literals(text: str) -> str:
    pattern = re.compile(
        r"//[^\n]*|/\*.*?\*/|R\"([^ ()\\\t\r\n]*)\(.*?\)\1\"|"
        r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        re.S,
    )
    return pattern.sub(" ", text)


def unique_cpp_braced_block(text: str, marker: str, context: str) -> str:
    starts = [match.start() for match in re.finditer(re.escape(marker), text)]
    require(len(starts) == 1, f"expected one {context}; found {len(starts)}")
    opening = text.find("{", starts[0])
    require(opening != -1, f"the {context} has no opening brace")
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[starts[0] : index + 1]
    raise ContractError(f"unterminated {context}")


def parenthesized_block(text: str, marker: str, context: str) -> str:
    starts = [match.start() for match in re.finditer(re.escape(marker), text)]
    require(len(starts) == 1, f"expected one {context}; found {len(starts)}")
    opening = text.find("(", starts[0] + len(marker))
    require(opening != -1, f"the {context} has no opening parenthesis")
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "(":
            depth += 1
        elif text[index] == ")":
            depth -= 1
            if depth == 0:
                return text[starts[0] : index + 1]
    raise ContractError(f"unterminated {context}")


def validate_main_cmake(cmake: str) -> None:
    target = "morsehgp3d_direct_sparse_gateway_clock"
    alias = "morsehgp3d::direct_sparse_gateway_clock"

    target_block = unique_target_block(cmake, "add_library", target)
    require("STATIC" in target_block, "CLOCK must remain a static archive")
    require(
        re.findall(r"[A-Za-z0-9_./+-]+\.cpp", target_block)
        == ["src/cpu/hierarchy/direct_sparse_gateway_clock.cpp"],
        "the CLOCK archive must contain exactly its dedicated source",
    )

    alias_block = unique_target_block(cmake, "add_library", alias)
    require(
        "ALIAS" in alias_block and target in alias_block,
        "the CLOCK alias must name the isolated archive",
    )

    properties = compact_whitespace(
        unique_target_block(cmake, "set_target_properties", target)
    )
    require_all(
        properties,
        (
            "EXPORT_NAME direct_sparse_gateway_clock",
            "POSITION_INDEPENDENT_CODE ON",
        ),
        "the CLOCK export properties",
    )
    features = compact_whitespace(
        unique_target_block(cmake, "target_compile_features", target)
    )
    require(
        "PUBLIC cxx_std_20" in features,
        "the CLOCK archive must publish its C++20 requirement",
    )
    includes = unique_target_block(cmake, "target_include_directories", target)
    require_all(
        includes,
        ("BUILD_INTERFACE", "INSTALL_INTERFACE"),
        "the CLOCK include routing",
    )

    links = compact_whitespace(
        unique_target_block(cmake, "target_link_libraries", target)
    )
    require(
        re.fullmatch(
            r"target_link_libraries\s*\(\s*" + target + r"\s+PUBLIC\s+"
            r"morsehgp3d::direct_sparse_gateway_candidate_journal\s+"
            r"morsehgp3d::direct_sparse_positive_facet_locator\s+"
            r"PRIVATE\s+morsehgp3d::sanitizers\s*\)",
            links,
            re.I,
        )
        is not None,
        "CLOCK must link exactly 10.7 then the locator publicly, with "
        "sanitizers private",
    )

    install_blocks = [
        block
        for block in cmake_blocks(cmake, "install")
        if "TARGETS" in block
        and re.search(rf"\b{re.escape(target)}\b", block) is not None
    ]
    require(
        len(install_blocks) == 1
        and len(re.findall(rf"\b{re.escape(target)}\b", install_blocks[0])) == 1
        and "EXPORT MorseHGP3DTargets" in compact_whitespace(install_blocks[0]),
        "CLOCK must be installed exactly once in the public export",
    )


def validate_test_wiring(unit_cmake: str) -> None:
    test_target = "morsehgp3d_hierarchy_direct_sparse_gateway_clock_tests"
    public_target = "morsehgp3d::direct_sparse_gateway_clock"

    executable = unique_target_block(unit_cmake, "add_executable", test_target)
    require(
        re.findall(r"[A-Za-z0-9_./+-]+\.cpp", executable)
        == ["test_hierarchy_direct_sparse_gateway_clock.cpp"],
        "the CLOCK test executable must contain exactly its dedicated source",
    )
    links = compact_whitespace(
        unique_target_block(unit_cmake, "target_link_libraries", test_target)
    )
    require(
        re.fullmatch(
            r"target_link_libraries\s*\(\s*"
            + test_target
            + r"\s+PRIVATE\s+"
            + re.escape(public_target)
            + r"\s*\)",
            links,
            re.I,
        )
        is not None,
        "the CLOCK test must link only through the isolated public target",
    )

    cpu_targets = unique_target_block(unit_cmake, "set", "_morsehgp3d_cpu_test_targets")
    require(
        len(re.findall(rf"\b{re.escape(test_target)}\b", cpu_targets)) == 1,
        "the CLOCK test must enter the sanitizer-applied CPU list once",
    )

    functional_name = "morsehgp3d.hierarchy_direct_sparse_gateway_clock"
    functional = unique_marked_block(
        unit_cmake, "add_test", functional_name, "CLOCK functional CTest"
    )
    require(
        re.fullmatch(
            r"add_test\s*\(\s*NAME\s+"
            + re.escape(functional_name)
            + r"\s+COMMAND\s+"
            + test_target
            + r"\s*\)",
            compact_whitespace(functional),
            re.I,
        )
        is not None,
        "the CLOCK functional CTest must run exactly its dedicated executable",
    )

    variable = "_morsehgp3d_phase10_direct_sparse_gateway_clock_configuration_command"
    checker_name = "check_phase10_direct_sparse_gateway_clock.py"
    configuration_name = "morsehgp3d.phase10_direct_sparse_gateway_clock_configuration"
    configuration_set = compact_whitespace(
        unique_target_block(unit_cmake, "set", variable)
    )
    require(
        re.fullmatch(
            r"set\s*\(\s*" + variable + r"\s+\"\$\{Python3_EXECUTABLE\}\"\s+"
            r"\"\$\{CMAKE_CURRENT_SOURCE_DIR\}/\.\./configuration/"
            + re.escape(checker_name)
            + r"\"\s+\"\$\{PROJECT_SOURCE_DIR\}\"\s*\)",
            configuration_set,
            re.I,
        )
        is not None,
        "the CLOCK checker command must contain only Python, checker and project",
    )
    append_block = unique_marked_block(
        unit_cmake, "list", variable, "CLOCK nm command append"
    )
    require(
        re.fullmatch(
            r"list\s*\(\s*APPEND\s+" + variable + r"\s+--binary\s+\"\$<TARGET_FILE:"
            r"morsehgp3d_direct_sparse_gateway_clock>\"\s+"
            r"--nm\s+\"\$\{CMAKE_NM\}\"\s*\)",
            compact_whitespace(append_block),
            re.I,
        )
        is not None,
        "the CLOCK nm append must inspect the isolated archive",
    )
    append_position = unit_cmake.find(append_block)
    require(
        append_position >= 0
        and enclosing_cmake_branches(unit_cmake, append_position)[-1:]
        == [("if", "CMAKE_NM AND NOT MSVC")],
        "the CLOCK nm append must be directly guarded by " "if(CMAKE_NM AND NOT MSVC)",
    )
    configuration = unique_marked_block(
        unit_cmake, "add_test", configuration_name, "CLOCK configuration CTest"
    )
    require(
        re.fullmatch(
            r"add_test\s*\(\s*NAME\s+"
            + re.escape(configuration_name)
            + r"\s+COMMAND\s+\$\{"
            + variable
            + r"\}\s*\)",
            compact_whitespace(configuration),
            re.I,
        )
        is not None,
        "the CLOCK configuration CTest must execute its assembled command",
    )


def validate_public_contract(header: str) -> None:
    compact = compact_whitespace(header)
    require_all(
        compact,
        (
            'direct_sparse_gateway_clock_backend = "reference_cpu"',
            'direct_sparse_gateway_clock_profile = "hgp_reduced"',
            'direct_sparse_gateway_clock_mode = "certified"',
            'direct_sparse_gateway_clock_refinement_status = "partial_refinement"',
            'direct_sparse_gateway_clock_public_status = "not_claimed"',
            "conditional_on_separate_external_clock_authority_replay_v1",
        ),
        "the Phase-10.11-CLOCK status axes",
    )

    project_includes = re.findall(r'#include\s+"([^"]+)"', header)
    require(
        project_includes
        == [
            "morsehgp3d/hierarchy/direct_sparse_gateway_candidate_journal.hpp",
            "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp",
        ],
        "the CLOCK public contract must include exactly 10.7 and the locator",
    )

    require_all(
        header,
        (
            "direct_sparse_gateway_candidate_scientific_identity_domain",
            "direct_sparse_gateway_clock_certificate_digest_domain",
            "maximum_deletion_projection_count",
            "maximum_facet_token_count",
            "maximum_gateway_candidate_count",
            "maximum_batch_count",
            "maximum_batch_facet_token_index_count",
            "maximum_single_exact_level_integer_bit_count",
            "maximum_exact_level_decimal_byte_count",
            "maximum_digest_payload_byte_count",
            "required_maximum_single_exact_level_integer_bit_count",
            "exact_level_integer_bits_within_budget",
            "all_five_scientific_arenas_bound",
            "ExactDirectSparseGatewayExternalClockAnchor",
            "expected_certificate_digest",
            "ExactDirectSparseGatewayClockBoundary",
            "source_batch_index",
            "strict_pre_locator_prefix_count",
            "historical_locator_stamp",
            "ExactDirectSparseGatewayClockCertificate",
            "source_scientific_identity_digest",
            "final_locator_stamp",
            "certificate_digest",
            "digest_present",
            "maximum_boundary_scan_count",
            "maximum_sort_comparison_count",
            "maximum_sort_scratch_entry_count",
            "maximum_prefix_scratch_entry_count",
            "maximum_temporary_scratch_byte_count",
            "source_identity_budget",
            "certificate_digest_budget",
            "locator_structure_budget",
            "prefix_stamp_sweep_budget",
            "external_anchor_non_null_and_payload_matched",
            "source_journal_freshly_replayed",
            "locator_structure_freshly_replayed",
            "source_scientific_identity_freshly_replayed",
            "certificate_digest_freshly_replayed",
            "boundaries_dense_and_prefixes_in_history",
            "boundaries_sorted_by_prefix_without_source_monotonicity_assumption",
            "every_historical_stamp_freshly_replayed",
            "final_locator_stamp_matches_entry_and_exit",
            "external_clock_authority_replayed",
            "conditional_on_caller_clock_authority_replay",
            "complete_conditional_source_batch_locator_clock_certificate",
            "compute_exact_direct_sparse_gateway_candidate_scientific_identity(",
            "compute_exact_direct_sparse_gateway_clock_certificate_digest(",
            "verify_exact_direct_sparse_gateway_clock_certificate(",
        ),
        "the bounded CLOCK public contract",
    )
    require(
        "136 + 92*S" in header,
        "the public certificate contract must state its exact payload formula",
    )

    code = strip_cpp_comments_and_literals(header)
    anchor = unique_cpp_braced_block(
        code,
        "struct ExactDirectSparseGatewayExternalClockAnchor",
        "separate external clock anchor",
    )
    certificate = unique_cpp_braced_block(
        code,
        "struct ExactDirectSparseGatewayClockCertificate {",
        "clock certificate payload",
    )
    require_all(
        anchor,
        ("authority_id", "replay_token", "expected_certificate_digest"),
        "the separate external clock anchor",
    )
    require(
        "expected_certificate_digest" not in certificate
        and "ExactDirectSparseGatewayExternalClockAnchor" not in certificate,
        "the expected digest and external anchor must not be embedded in the payload",
    )
    require_all(
        certificate,
        (
            "authority_id",
            "replay_token",
            "source_scientific_identity_digest",
            "final_locator_stamp",
            "boundaries",
            "certificate_digest",
            "digest_present",
        ),
        "the clock certificate payload",
    )

    digest_declaration = parenthesized_block(
        header,
        "compute_exact_direct_sparse_gateway_clock_certificate_digest",
        "certificate digest API",
    )
    require(
        "ExactDirectSparseGatewayExternalClockAnchor" not in digest_declaration,
        "the payload digest API must remain independent of the external anchor",
    )
    verifier_declaration = parenthesized_block(
        header,
        "verify_exact_direct_sparse_gateway_clock_certificate",
        "CLOCK fresh verifier API",
    )
    require_all(
        verifier_declaration,
        (
            "const ExactDirectSparseGatewayExternalClockAnchor& external_anchor",
            "const ExactDirectSparseGatewayClockCertificate& certificate",
        ),
        "the separate anchor and payload verifier signature",
    )

    lowered = header.lower()
    for forbidden in (
        "std::vector<std::vector",
        "std::map<",
        "std::unordered_map",
        "#include <map>",
        "#include <unordered_map>",
        "exactdirectsparsecofacekey",
        "direct_sparse_gateway_candidate_localization",
        "direct_sparse_positive_facet_prefix_sweep",
    ):
        require(
            forbidden not in lowered,
            f"the CLOCK ABI contains forbidden shape {forbidden!r}",
        )
    require(
        re.search(r"std\s*::\s*array\s*<[^;>]*PointId[^;>]*,\s*11U?\s*>", header)
        is None,
        "the CLOCK ABI must not persist an eleven-point coface key",
    )


def validate_identity_serializer(source: str, code: str) -> None:
    serializer = unique_cpp_braced_block(
        code,
        "[[nodiscard]] bool append_gateway_candidate_scientific_identity(",
        "single 10.7 scientific identity serializer",
    )
    require_all(
        serializer,
        (
            "journal.schema_version",
            "journal.traversal_order",
            "journal.point_count",
            "journal.source_direct_event_count",
            "journal.source_pair_canonical_cloud_digest",
            "journal.source_higher_canonical_cloud_digest",
            "journal.source_pair_semantic_digest",
            "journal.source_higher_semantic_digest",
            "journal.deletion_projections.size()",
            "projection.deletion_projection_index",
            "projection.source_family_index",
            "projection.source",
            "projection.source_deletion_index",
            "projection.source_event_index",
            "projection.source_order",
            "projection.removed_point_id",
            "projection.facet_token_index",
            "projection.saddle_squared_level",
            "projection.level_relation",
            "projection.removed_point_is_first_incidence_cominimizer",
            "journal.facet_tokens.size()",
            "token.facet_token_index",
            "token.source_facet_key",
            "token.source_miniball_squared_level",
            "token.first_incidence_squared_level",
            "token.first_incidence_audit",
            "token.deletion_projection_offset",
            "token.deletion_projection_count",
            "token.gateway_candidate_offset",
            "token.gateway_candidate_count",
            "token.batch_index",
            "journal.gateway_candidates.size()",
            "candidate.gateway_candidate_index",
            "candidate.facet_token_index",
            "candidate.added_point_id",
            "candidate.positive_support_point_ids",
            "candidate.positive_support_point_count",
            "candidate.added_point_in_source_closed_ball",
            "candidate.added_point_in_selected_positive_support",
            "journal.batches.size()",
            "batch.batch_index",
            "batch.facet_cardinality",
            "batch.first_incidence_squared_level",
            "batch.facet_token_index_offset",
            "batch.facet_token_index_count",
            "journal.batch_facet_token_indices.size()",
        ),
        "the exhaustive five-arena scientific identity",
    )
    for excluded in (
        "requested_budget",
        "budget_preflight_certified",
        "no_forbidden_global_structure_materialized",
        "decision",
        "scope",
        "public_status_claimed",
    ):
        require(
            excluded not in serializer,
            f"the scientific identity must exclude operational field {excluded!r}",
        )
    for tag in ("1U", "2U", "3U", "4U", "5U"):
        require(
            f"sink.append_byte({tag})" in serializer,
            f"the scientific identity is missing arena tag {tag}",
        )

    facet_key = unique_cpp_braced_block(
        code, "[[nodiscard]] bool append_facet_key(", "full facet-key serializer"
    )
    require_all(
        facet_key,
        ("key.point_count", "for (const spatial::PointId point_id : key.point_ids)"),
        "the full fixed-width facet-key serializer",
    )

    audit = unique_cpp_braced_block(
        code,
        "[[nodiscard]] bool append_first_incidence_audit(",
        "complete 10.6 audit serializer",
    )
    audit_fields = (
        "eligible_coface_point_count",
        "source_support_enumeration_count",
        "node_visit_count",
        "internal_node_expansion_count",
        "exact_aabb_bound_evaluation_count",
        "exact_point_evaluation_count",
        "excluded_facet_point_count",
        "coface_support_enumeration_count",
        "candidate_point_classification_count",
        "inside_or_boundary_source_ball_point_count",
        "outside_source_ball_point_count",
        "pruned_node_count",
        "pruned_eligible_point_count",
        "peak_frontier_entry_count",
        "peak_cominimizer_entry_count",
        "incumbent_improvement_count",
        "equal_incumbent_observation_count",
        "provisional_cominimizer_overflow_count",
        "traversal_complete",
    )
    require_all(audit, audit_fields, "the complete 10.6 audit serializer")
    require(
        "std::array<std::size_t, 18U>" in audit,
        "the audit serializer must bind exactly its eighteen counters",
    )

    for marker, variants, context in (
        (
            "[[nodiscard]] bool append_traversal_order_tag(",
            ("near_first", "far_first"),
            "LBVH traversal tag serializer",
        ),
        (
            "[[nodiscard]] bool append_deletion_source_tag(",
            ("unspecified", "strict_arm_seed", "equal_level_facet_seed"),
            "deletion-source tag serializer",
        ),
        (
            "[[nodiscard]] bool append_level_relation_tag(",
            (
                "unspecified",
                "first_incidence_strictly_below_saddle",
                "first_incidence_equal_to_saddle",
            ),
            "level-relation tag serializer",
        ),
    ):
        enum_serializer = unique_cpp_braced_block(code, marker, context)
        require_all(enum_serializer, variants, context)

    require(
        source.count("append_gateway_candidate_scientific_identity(") == 3,
        "the scientific identity serializer must have one definition and "
        "exactly two calls for measurement and hashing",
    )


def validate_implementation(source: str) -> None:
    include_lines = "\n".join(
        line for line in source.splitlines() if line.lstrip().startswith("#include")
    )
    project_includes = re.findall(r'#include\s+"([^"]+)"', include_lines)
    require(
        project_includes
        == [
            "morsehgp3d/hierarchy/direct_sparse_gateway_clock.hpp",
            "morsehgp3d/contract/canonical_id.hpp",
        ],
        "the isolated CLOCK source may include only its own header and the "
        "canonical SHA implementation",
    )

    code = strip_cpp_comments_and_literals(source)
    compact = re.sub(r"\s*::\s*", "::", compact_whitespace(code))
    compact = re.sub(r"\s*\.\s*", ".", compact)

    validate_identity_serializer(source, code)

    identity_compute = unique_cpp_braced_block(
        code,
        "ExactDirectSparseGatewayCandidateScientificIdentityResult\n"
        "compute_exact_direct_sparse_gateway_candidate_scientific_identity(",
        "scientific identity computation",
    )
    digest_compute = unique_cpp_braced_block(
        code,
        "ExactDirectSparseGatewayClockCertificateDigestResult\n"
        "compute_exact_direct_sparse_gateway_clock_certificate_digest(",
        "certificate digest computation",
    )
    verifier = unique_cpp_braced_block(
        code,
        "ExactDirectSparseGatewayClockVerification\n"
        "verify_exact_direct_sparse_gateway_clock_certificate(",
        "CLOCK fresh verifier",
    )

    require(
        code.count("compute_exact_direct_sparse_gateway_candidate_scientific_identity(")
        == 2
        and verifier.count(
            "compute_exact_direct_sparse_gateway_candidate_scientific_identity("
        )
        == 1,
        "the scientific identity API must have one definition and one verifier call",
    )
    require(
        code.count("compute_exact_direct_sparse_gateway_clock_certificate_digest(") == 2
        and verifier.count(
            "compute_exact_direct_sparse_gateway_clock_certificate_digest("
        )
        == 1,
        "the certificate digest API must have one definition and one verifier call",
    )
    require(
        code.count("append_gateway_clock_certificate_payload(") == 2
        and digest_compute.count("append_gateway_clock_certificate_payload(") == 1,
        "the certificate payload must have one serializer and one hashing call",
    )
    require(
        code.count("[[nodiscard]] bool append_locator_stamp(") == 1,
        "locator stamps must share one canonical serializer",
    )

    required_scientific_calls = (
        "verify_exact_direct_sparse_gateway_candidate_journal(",
        "verify_exact_direct_sparse_positive_facet_locator_structure(",
        "build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(",
    )
    for call in required_scientific_calls:
        require(
            verifier.count(call) == 1,
            f"the CLOCK verifier must contain exactly one site for {call!r}",
        )
    require(
        "verify_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep("
        not in code,
        "CLOCK must use one PSTAMP builder after a full structural verification, "
        "not the separate PSTAMP verifier",
    )
    require(
        "build_exact_direct_sparse_positive_facet_prefix_sweep(" not in code
        and "verify_exact_direct_sparse_positive_facet_prefix_sweep(" not in code,
        "CLOCK must not depend on the 10.10 historical resolution target",
    )

    source_verify_position = verifier.find(
        "verify_exact_direct_sparse_gateway_candidate_journal("
    )
    source_identity_position = verifier.find(
        "compute_exact_direct_sparse_gateway_candidate_scientific_identity("
    )
    structure_position = verifier.find(
        "verify_exact_direct_sparse_positive_facet_locator_structure("
    )
    pstamp_position = verifier.find(
        "build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep("
    )
    require(
        0 <= source_verify_position < source_identity_position < pstamp_position
        and 0 <= structure_position < pstamp_position,
        "fresh 10.7 and complete locator structure replay must precede PSTAMP",
    )
    require_all(
        verifier,
        (
            "source_journal_verification.result_certified",
            "locator_structure_verification.result_certified",
            "source_identity_result.certified_identity()",
            "prefix_stamp_sweep.certified_partial_refinement()",
            "prefix_stamp_sweep.certified_outcome()",
        ),
        "the nested fresh-verification gates",
    )

    measure_sink = unique_cpp_braced_block(
        code, "class MeasureSink", "bounded identity measurement sink"
    )
    level_position = measure_sink.find("append_level(const exact::ExactLevel& level)")
    bit_measure_position = measure_sink.find("integer_bit_count(", level_position)
    bit_guard_position = measure_sink.find(
        "maximum_exact_level_integer_bit_count_", bit_measure_position
    )
    numerator_text_position = measure_sink.find(
        "level.numerator_string()", bit_guard_position
    )
    denominator_text_position = measure_sink.find(
        "level.denominator_string()", bit_guard_position
    )
    require(
        0
        <= level_position
        < bit_measure_position
        < bit_guard_position
        < min(numerator_text_position, denominator_text_position),
        "the per-integer bit cap must reject hostile exact levels before "
        "decimal string allocation",
    )
    require_all(
        measure_sink,
        (
            "boost::multiprecision::msb(",
            "exact_level_decimal_byte_count_",
            "payload_byte_count_",
            "capacity_overflow_",
            "bit_budget_exhausted_",
        ),
        "the bounded identity measurement sink",
    )

    identity_preflight_position = identity_compute.find(
        "result.population_budget_preflight_certified = true"
    )
    identity_measure_position = identity_compute.find(
        "MeasureSink measurement", identity_preflight_position
    )
    identity_builder_position = identity_compute.find(
        "contract::CanonicalSha256Builder builder", identity_measure_position
    )
    identity_sha_guard_position = identity_compute.find(
        "sha256_input_size_is_valid(", identity_measure_position
    )
    require(
        0
        <= identity_preflight_position
        < identity_measure_position
        < identity_sha_guard_position
        < identity_builder_position,
        "identity populations, exact text, payload bytes and SHA length must be "
        "preflighted before hashing",
    )
    for cap in (
        "maximum_deletion_projection_count",
        "maximum_facet_token_count",
        "maximum_gateway_candidate_count",
        "maximum_batch_count",
        "maximum_batch_facet_token_index_count",
    ):
        require(
            identity_compute.find(cap) < identity_preflight_position,
            f"the {cap} population cap must precede identity arena scans",
        )
    require_all(
        identity_compute,
        (
            "maximum_single_exact_level_integer_bit_count",
            "maximum_exact_level_decimal_byte_count",
            "maximum_digest_payload_byte_count",
            "direct_sparse_gateway_candidate_scientific_identity_domain",
        ),
        "the bounded scientific identity computation",
    )

    require_all(
        digest_compute,
        (
            "certificate_fixed_payload_byte_count",
            "certificate_boundary_payload_byte_count",
            "checked_multiply(",
            "checked_add(",
            "sha256_input_size_is_valid(",
            "maximum_boundary_count",
            "maximum_digest_payload_byte_count",
            "result.budget_preflight_certified = true",
            "direct_sparse_gateway_clock_certificate_digest_domain",
        ),
        "the exact 136+92S certificate digest preflight",
    )
    digest_preflight_position = digest_compute.find(
        "result.budget_preflight_certified = true"
    )
    digest_builder_position = digest_compute.find(
        "contract::CanonicalSha256Builder builder"
    )
    require(
        0 <= digest_preflight_position < digest_builder_position,
        "certificate size and SHA limits must pass before hashing",
    )
    require_all(
        code,
        (
            "maximum_sha256_input_byte_count",
            "std::numeric_limits<std::uint64_t>::max() / UINT64_C(8)",
            "certificate_fixed_payload_byte_count = 136U",
            "certificate_boundary_payload_byte_count = 92U",
        ),
        "the canonical payload and SHA size constants",
    )

    verifier_preflight_position = verifier.find(
        "result.boundary_and_scratch_budget_preflight_certified = true"
    )
    first_vector_position = verifier.find("std::vector<")
    require(
        0 <= verifier_preflight_position < first_vector_position,
        "all boundary and scratch populations and bytes must pass before "
        "either CLOCK scratch vector is allocated",
    )
    require(
        verifier.count("std::vector<") == 2,
        "CLOCK must allocate exactly the sorted-boundary and prefix scratch vectors",
    )
    for cap in (
        "maximum_boundary_count",
        "maximum_boundary_scan_count",
        "maximum_sort_scratch_entry_count",
        "maximum_prefix_scratch_entry_count",
        "maximum_temporary_scratch_byte_count",
    ):
        require(
            verifier.find(cap) < verifier_preflight_position,
            f"the {cap} cap must be checked before scratch allocation",
        )
    require(
        re.search(
            r"checked_multiply\s*\(\s*"
            r"result\.required_boundary_count\s*,\s*2U\s*,\s*"
            r"result\.required_boundary_scan_count\s*\)",
            verifier,
        )
        is not None,
        "the CLOCK verifier must preflight exactly two boundary scans",
    )

    anchor_gate = unique_cpp_braced_block(
        verifier,
        "if (external_anchor.authority_id == 0U ||",
        "external anchor and payload identity gate",
    )
    require_all(
        anchor_gate,
        (
            "external_anchor.replay_token == 0U",
            "certificate.authority_id == 0U",
            "certificate.replay_token == 0U",
            "certificate.authority_id != external_anchor.authority_id",
            "certificate.replay_token != external_anchor.replay_token",
            "!certificate.digest_present",
        ),
        "the external anchor and payload identity gate",
    )
    require_all(
        verifier,
        (
            "certificate_digest_result.certificate_digest !="
            "\n          certificate.certificate_digest",
            "certificate_digest_result.certificate_digest !="
            "\n          external_anchor.expected_certificate_digest",
        ),
        "the computed-payload-anchor three-way digest equality",
    )
    require(
        re.search(
            r"(?:certificate|external_anchor)\.[A-Za-z_]*digest\s*!=\s*"
            r"contract::CanonicalId\s*\{\s*\}",
            verifier,
        )
        is None,
        "a zero SHA-256 value is data and must not be used as a sentinel",
    )

    dense_loop = unique_cpp_braced_block(
        verifier,
        "for (std::size_t source_index = 0U;",
        "dense source-order boundary scan",
    )
    require_all(
        dense_loop,
        (
            "boundary.source_batch_index != source_index",
            "boundary.strict_pre_locator_prefix_count >",
            "result.locator_stamp_at_entry.committed_batch_count",
            "sorted_boundaries.push_back(",
            "boundary.strict_pre_locator_prefix_count",
            "source_index",
        ),
        "the dense source-order boundary scan",
    )
    require(
        "previous" not in dense_loop.lower()
        and re.search(
            r"strict_pre_locator_prefix_count\s*(?:==|!=|<=|>=|<)\s*source_index",
            dense_loop,
        )
        is None,
        "CLOCK must not identify p_s with s or force source-order prefix monotonicity",
    )

    comparator = unique_cpp_braced_block(
        code,
        "[[nodiscard]] bool prefix_source_less(",
        "prefix/source scratch comparator",
    )
    require_all(
        comparator,
        (
            "left.prefix < right.prefix",
            "left.prefix == right.prefix",
            "left.source_batch_index < right.source_batch_index",
        ),
        "the prefix/source scratch comparator",
    )
    bounded_comparator = unique_cpp_braced_block(
        code, "class BoundedPrefixSourceComparator", "bounded sort comparator"
    )
    comparison_guard_position = bounded_comparator.find(
        "comparison_count_ >= maximum_count_"
    )
    comparison_increment_position = bounded_comparator.find(
        "++comparison_count_", comparison_guard_position
    )
    actual_compare_position = bounded_comparator.find(
        "prefix_source_less(", comparison_increment_position
    )
    require(
        0
        <= comparison_guard_position
        < comparison_increment_position
        < actual_compare_position,
        "the comparison cap must be consumed before every heapsort comparison",
    )
    sort_position = verifier.find("bounded_heapsort(")
    prefix_extraction_position = verifier.find(
        "std::vector<std::size_t> sorted_prefixes", sort_position
    )
    require(
        0 <= sort_position < prefix_extraction_position < pstamp_position,
        "only the scratch copy may be sorted, and sorting must precede PSTAMP",
    )
    require(
        "sort(certificate.boundaries" not in compact
        and "bounded_heapsort(certificate.boundaries" not in compact,
        "the canonical source-order certificate boundaries must never be sorted",
    )
    require_all(
        verifier,
        (
            "sorted_prefixes, locator, budget.prefix_stamp_sweep_budget",
            "sorted_boundaries[sorted_index].source_batch_index",
            "certificate.boundaries[source_index].historical_locator_stamp",
        ),
        "the sorted PSTAMP request and source-order stamp comparison",
    )

    require_all(
        verifier,
        (
            "result.locator_stamp_at_entry = locator.snapshot_stamp()",
            "result.locator_stamp_at_exit = locator.snapshot_stamp()",
            "certificate.final_locator_stamp != result.locator_stamp_at_entry",
            "certificate.final_locator_stamp != result.locator_stamp_at_exit",
            "result.external_clock_authority_replayed = false",
            "result.conditional_on_caller_clock_authority_replay = true",
            "result.forbidden_global_structure_materialized = false",
            "result.public_status_claimed = false",
            "result.partial_refinement_only = true",
            "result.source_and_locator_inputs_mutated = false",
        ),
        "the frozen conditional CLOCK scope",
    )
    require(
        re.search(r"external_clock_authority_replayed\s*=\s*true", code) is None,
        "the local kernel must never claim replay of caller-owned clock authority",
    )

    architecture_code = code
    for honest_field in (
        "forbidden_global_structure_materialized",
        "external_clock_authority_replayed",
        "source_and_locator_inputs_mutated",
        "eligible_coface_point_count",
        "coface_support_enumeration_count",
    ):
        architecture_code = architecture_code.replace(honest_field, " ")
    forbidden_patterns = (
        (
            r"\bstd\s*::\s*(?:map|multimap|unordered_map|unordered_multimap)\s*<",
            "associative global index",
        ),
        (r"\bstd\s*::\s*vector\s*<\s*std\s*::\s*vector\s*<", "nested arena"),
        (r"\bapply_batch\s*\(", "locator mutation"),
        (
            r"\b(?:build|verify)_exact_direct_sparse_gateway_candidate_localization",
            "10.9 localization",
        ),
        (
            r"\bExactDirectSparseGatewayCandidateLocalization[A-Za-z0-9_]*\b",
            "10.9 localization type",
        ),
        (
            r"\b(?:build|verify)_exact_direct_sparse_positive_facet_prefix_sweep",
            "10.10 historical resolution",
        ),
        (r"\bExactDirectSparsePositiveFacetPrefix[A-Za-z0-9_]*\b", "10.10 type"),
        (r"\bExactDirectSparseCofaceKey\b", "persistent coface key"),
        (
            r"std\s*::\s*array\s*<[^;>]*PointId[^;>]*,\s*11U?\s*>",
            "eleven-point coface key",
        ),
        (
            r"\b(?:DisjointSetUnion|UnionFind|disjoint_set|union_find)\b",
            "union-find structure",
        ),
        (r"\bstd\s*::\s*iota\s*\(", "dense DSU initialization"),
        (r"\bcandidate_parents\b|\bcomponent_parents\s*\[", "DSU parent arena"),
        (r"\bbuild_exact_direct_frozen_root_quotient\b", "quotient builder"),
        (r"\bbuild_exact_direct_k1_forest[A-Za-z0-9_]*\b", "forest builder"),
        (
            r"\b(?:GatewayAttach|gateway_attach|RootBirth|root_birth|"
            r"AtomicUnionBatch|atomic_union_batch)\b",
            "root, union or attachment action",
        ),
        (
            r"\b(?:build|verify|compute)[A-Za-z0-9_]*(?:gamma|delaunay|"
            r"cell_atlas|critical_catalog)\s*\(",
            "global geometry builder",
        ),
        (r"\b(?:ordinary|power)_?cell\b", "global cell"),
        (r"\b(?:facet|coface)_?catalog\b", "global facet/coface catalog"),
        (
            r"\b(?:static|thread_local)\s+(?!constexpr\b)"
            r"std\s*::\s*(?:vector|map|unordered_map)\s*<",
            "mutable namespace-level container",
        ),
    )
    for pattern, label in forbidden_patterns:
        require(
            re.search(pattern, architecture_code, re.I) is None,
            f"the isolated CLOCK implementation contains forbidden {label}",
        )


def validate_unit_test(test: str) -> None:
    require_all(
        test,
        (
            "compute_exact_direct_sparse_gateway_candidate_scientific_identity(",
            "compute_exact_direct_sparse_gateway_clock_certificate_digest(",
            "verify_exact_direct_sparse_gateway_clock_certificate(",
            "certified_identity()",
            "certified_digest()",
            "certified_conditional_clock_binding()",
            "required_digest_payload_byte_count == 194U",
            "required_digest_payload_byte_count == 136U",
            "required_digest_payload_byte_count == 228U",
            "required_digest_payload_byte_count == 320U",
            "external_clock_authority_replayed",
            "conditional_on_caller_clock_authority_replay",
            "maximum_single_exact_level_integer_bit_count",
            "maximum_exact_level_decimal_byte_count",
            "maximum_digest_payload_byte_count",
            "maximum_sort_comparison_count",
            "maximum_temporary_scratch_byte_count",
            "anchor_for(",
            "rehash_certificate(",
            "replacement_anchor",
            "advanced_locator",
            "empty_source_fixture(",
            "194U + 75U * fixture.source.deletion_projections.size()",
            "313U * fixture.source.facet_tokens.size()",
            "66U * fixture.source.gateway_candidates.size()",
            "48U * fixture.source.batches.size()",
            "8U * fixture.source.batch_facet_token_indices.size()",
        ),
        "the short CLOCK unit suite",
    )
    require(
        test.count("empty_source_fixture(") == 2,
        "the certified S=0 source fixture must have one definition and one "
        "verification call",
    )
    for golden_name in (
        "expected_empty_identity_digest",
        "expected_empty_certificate_digest",
    ):
        require(
            re.search(
                rf"{golden_name}\s*=\s*\"[0-9a-f]{{64}}\"",
                test,
            )
            is not None,
            f"the CLOCK suite must freeze a nonempty 64-hex golden for "
            f"{golden_name}",
        )
    require(
        re.search(
            r"source_prefixes\s*\[\s*0U\s*\]\s*=\s*2U\s*;.*?"
            r"source_prefixes\s*\[\s*1U\s*\]\s*=\s*0U\s*;.*?"
            r"source_prefixes\s*\[\s*2U\s*\]\s*=\s*2U\s*;",
            test,
            re.S,
        )
        is not None,
        "the CLOCK suite must accept a source-dense but prefix-nonmonotone "
        "{2,0,2} certificate",
    )
    require_all(
        test,
        (
            "certificate.certificate_digest",
            "certificate_digest",
            "digest_present",
            "historical_locator_stamp",
            "commit",
            "empty",
        ),
        "the anchor, stamp and empty-commit falsifications",
    )


def validate_symbols(binary: Path, nm: Path) -> None:
    require(binary.is_file(), f"the isolated CLOCK archive does not exist: {binary}")
    require(nm.is_file(), f"the nm executable does not exist: {nm}")
    try:
        symbols = subprocess.run(
            [str(nm), "-C", str(binary)],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.lower()
    except (OSError, subprocess.CalledProcessError) as error:
        raise ContractError(f"cannot inspect the CLOCK archive: {error}") from error

    require_all(
        symbols,
        (
            "compute_exact_direct_sparse_gateway_candidate_scientific_identity",
            "compute_exact_direct_sparse_gateway_clock_certificate_digest",
            "verify_exact_direct_sparse_gateway_clock_certificate",
            "exactdirectsparsegatewaycandidatescientificidentityresult::"
            "certified_identity",
            "exactdirectsparsegatewayclockcertificatedigestresult::certified_digest",
            "exactdirectsparsegatewayclockverification::"
            "certified_conditional_clock_binding",
            "verify_exact_direct_sparse_gateway_candidate_journal",
            "verify_exact_direct_sparse_positive_facet_locator_structure",
            "build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep",
            "exactdirectsparsepositivefacetlocator::snapshot_stamp",
            "exactdirectsparsepositivefacetlocator::state_view",
        ),
        "the isolated CLOCK symbol closure",
    )
    for forbidden in (
        "verify_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep",
        "build_exact_direct_sparse_positive_facet_prefix_sweep",
        "verify_exact_direct_sparse_positive_facet_prefix_sweep",
        "exactdirectsparsepositivefacetlocator::apply_batch",
        "direct_sparse_gateway_candidate_localization",
        "exactdirectsparsegatewaycandidatelocalization",
        "exactdirectsparsecofacekey",
        "build_exact_direct_frozen_root_quotient",
        "build_exact_direct_k1_forest",
        "morse_gamma",
        "build_exact_gamma",
        "delaunay",
        "cellatlas",
        "cell_atlas",
        "critical_catalog",
        "global_facet_catalog",
        "global_coface_catalog",
        "ordinary_cell",
        "power_cell",
        "gatewayattach",
        "gateway_attach",
        "unionfind",
        "union_find",
        "disjointset",
        "disjoint_set",
        "rootbirth",
        "root_birth",
        "atomicunionbatch",
        "atomic_union_batch",
    ):
        require(forbidden not in symbols, f"forbidden linked symbol {forbidden!r}")


def validate(project: Path, binary: Path | None, nm: Path | None) -> None:
    require(
        (binary is None) == (nm is None),
        "--binary and --nm must be supplied together",
    )
    cmake = read_text(project / "CMakeLists.txt")
    unit_cmake = read_text(project / "tests/unit/CMakeLists.txt")
    header = read_text(
        project / "include/morsehgp3d/hierarchy/direct_sparse_gateway_clock.hpp"
    )
    source = read_text(project / "src/cpu/hierarchy/direct_sparse_gateway_clock.cpp")
    test = read_text(
        project / "tests/unit/test_hierarchy_direct_sparse_gateway_clock.cpp"
    )

    validate_main_cmake(cmake)
    validate_test_wiring(unit_cmake)
    validate_public_contract(header)
    validate_implementation(source)
    validate_unit_test(test)
    if binary is not None and nm is not None:
        validate_symbols(binary, nm)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("project", type=Path)
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--nm", type=Path)
    arguments = parser.parse_args()
    try:
        validate(arguments.project.resolve(), arguments.binary, arguments.nm)
    except ContractError as error:
        parser.error(str(error))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
