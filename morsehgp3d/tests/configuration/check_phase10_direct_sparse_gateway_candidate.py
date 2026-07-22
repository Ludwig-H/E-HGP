#!/usr/bin/env python3
"""Validate the isolated Phase-10.7-RCPU sparse gateway-candidate target."""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


class ContractError(ValueError):
    """A direct sparse gateway-candidate invariant was violated."""


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
    return " ".join(text.split())


def cmake_blocks(text: str, command: str) -> list[str]:
    pattern = re.compile(rf"(?<![A-Za-z0-9_]){re.escape(command)}\s*\(", re.I)
    blocks: list[str] = []
    for match in pattern.finditer(text):
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
            raise ContractError(f"unterminated {command} block")
    return blocks


def unique_target_block(text: str, command: str, target: str) -> str:
    first_argument = re.compile(
        rf"^{re.escape(command)}\s*\(\s*{re.escape(target)}(?=\s|\))",
        re.I,
    )
    matches = [
        block for block in cmake_blocks(text, command) if first_argument.search(block)
    ]
    require(
        len(matches) == 1,
        f"expected one {command} block for {target!r}; found {len(matches)}",
    )
    return matches[0]


def unique_marked_block(
    text: str, command: str, marker: str, context: str
) -> str:
    matches = [block for block in cmake_blocks(text, command) if marker in block]
    require(
        len(matches) == 1,
        f"{context} requires one {command} block containing {marker!r}; "
        f"found {len(matches)}",
    )
    return matches[0]


def balanced_brace_block(text: str, marker: str, context: str) -> str:
    require(
        text.count(marker) == 1,
        f"{context} requires exactly one marker {marker!r}",
    )
    start = text.find(marker)
    opening = text.find("{", start + len(marker))
    require(opening >= 0, f"{context} has no opening brace")
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    raise ContractError(f"{context} has an unterminated brace block")


def cpp_record_blocks(text: str) -> list[str]:
    pattern = re.compile(r"\b(?:class|struct)\s+[A-Za-z_]\w*[^;{]*\{")
    blocks: list[str] = []
    for match in pattern.finditer(text):
        opening = text.find("{", match.start())
        depth = 0
        for index in range(opening, len(text)):
            if text[index] == "{":
                depth += 1
            elif text[index] == "}":
                depth -= 1
                if depth == 0:
                    blocks.append(text[match.start() : index + 1])
                    break
        else:
            raise ContractError(
                f"unterminated C++ record starting at byte {match.start()}"
            )
    return blocks


def strip_cpp_comments_and_literals(text: str) -> str:
    pattern = re.compile(
        r"//[^\n]*|/\*.*?\*/|R\"([^ ()\\\t\r\n]*)\(.*?\)\1\"|"
        r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        re.S,
    )
    return pattern.sub(" ", text)


def validate_main_cmake(cmake: str) -> None:
    target = "morsehgp3d_direct_sparse_gateway_candidate_journal"
    alias = "morsehgp3d::direct_sparse_gateway_candidate_journal"

    target_block = unique_target_block(cmake, "add_library", target)
    require("STATIC" in target_block, "gateway candidates must remain a static archive")
    require(
        re.findall(r"[A-Za-z0-9_./+-]+\.cpp", target_block)
        == ["src/cpu/hierarchy/direct_sparse_gateway_candidate_journal.cpp"],
        "the archive must contain exactly its dedicated source",
    )

    alias_block = unique_target_block(cmake, "add_library", alias)
    require(
        "ALIAS" in alias_block and target in alias_block,
        "the public alias must name the isolated archive",
    )

    properties = compact_whitespace(
        unique_target_block(cmake, "set_target_properties", target)
    )
    require_all(
        properties,
        (
            "EXPORT_NAME direct_sparse_gateway_candidate_journal",
            "POSITION_INDEPENDENT_CODE ON",
        ),
        "the gateway-candidate export properties",
    )

    features = compact_whitespace(
        unique_target_block(cmake, "target_compile_features", target)
    )
    require(
        "PUBLIC cxx_std_20" in features,
        "the isolated archive must publish its C++20 requirement",
    )
    includes = unique_target_block(cmake, "target_include_directories", target)
    require_all(
        includes,
        ("BUILD_INTERFACE", "INSTALL_INTERFACE"),
        "the isolated archive include routing",
    )

    links = compact_whitespace(
        unique_target_block(cmake, "target_link_libraries", target)
    )
    require(
        re.fullmatch(
            r"target_link_libraries\s*\(\s*"
            + target
            + r"\s+PUBLIC\s+"
            r"morsehgp3d::direct_sparse_first_incidence\s+"
            r"morsehgp3d::direct_closed_saddle_incidence_journal\s+"
            r"PRIVATE\s+morsehgp3d::sanitizers\s*\)",
            links,
            re.I,
        )
        is not None,
        "the archive must link exactly first-incidence then closed-saddle "
        "incidence, with sanitizers private",
    )

    install_blocks = [
        block for block in cmake_blocks(cmake, "install") if "TARGETS" in block
    ]
    install_count = sum(
        len(re.findall(rf"\b{re.escape(target)}\b", block))
        for block in install_blocks
    )
    require(install_count == 1, "the archive must be installed exactly once")


def validate_test_wiring(unit_cmake: str) -> None:
    test_target = (
        "morsehgp3d_hierarchy_direct_sparse_gateway_candidate_journal_tests"
    )
    public_target = "morsehgp3d::direct_sparse_gateway_candidate_journal"

    executable = unique_target_block(unit_cmake, "add_executable", test_target)
    require(
        re.findall(r"[A-Za-z0-9_./+-]+\.cpp", executable)
        == ["test_hierarchy_direct_sparse_gateway_candidate_journal.cpp"],
        "the unit executable must contain exactly its dedicated source",
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
        "the unit test must link only through the isolated public target",
    )

    cpu_lists = cmake_blocks(unit_cmake, "set") + cmake_blocks(unit_cmake, "list")
    require(
        sum(test_target in block for block in cpu_lists) == 1,
        "the unit target must enter the sanitizer-applied CPU list once",
    )

    functional = unique_marked_block(
        unit_cmake,
        "add_test",
        "morsehgp3d.hierarchy_direct_sparse_gateway_candidate_journal",
        "the functional CTest",
    )
    require(test_target in functional, "the functional CTest runs the wrong target")

    configuration = unique_marked_block(
        unit_cmake,
        "add_test",
        "morsehgp3d.phase10_direct_sparse_gateway_candidate_configuration",
        "the configuration CTest",
    )
    require(
        "_morsehgp3d_phase10_direct_sparse_gateway_candidate_"
        "configuration_command" in configuration,
        "the configuration CTest must use its assembled command",
    )
    require_all(
        unit_cmake,
        (
            "check_phase10_direct_sparse_gateway_candidate.py",
            "$<TARGET_FILE:morsehgp3d_direct_sparse_gateway_candidate_journal>",
            "--binary",
            "--nm",
            "CMAKE_NM",
        ),
        "the static and symbolic checker wiring",
    )


def validate_public_contract(header: str) -> None:
    compact = compact_whitespace(header)
    require_all(
        compact,
        (
            'direct_sparse_gateway_candidate_journal_backend = "reference_cpu"',
            'direct_sparse_gateway_candidate_journal_profile = "hgp_reduced"',
            'direct_sparse_gateway_candidate_journal_mode = "certified"',
            'direct_sparse_gateway_candidate_journal_refinement_status = "partial_refinement"',
            'direct_sparse_gateway_candidate_journal_public_status = "not_claimed"',
        ),
        "the Phase-10.7-RCPU status axes",
    )
    require_all(
        header,
        (
            "maximum_source_family_scan_count",
            "maximum_deletion_reference_count",
            "maximum_distinct_facet_count",
            "maximum_facet_key_point_count",
            "maximum_gateway_candidate_count",
            "maximum_batch_count",
            "maximum_batch_facet_reference_count",
            "maximum_logical_storage_entry_count",
            "ExactDirectSparseFirstIncidenceBudget first_incidence_budget",
            "strict_arm_seed",
            "equal_level_facet_seed",
            "first_incidence_strictly_below_saddle",
            "first_incidence_equal_to_saddle",
            "removed_point_is_first_incidence_cominimizer",
            "std::vector<ExactDirectSparseGatewayDeletionProjection>",
            "std::vector<ExactDirectSparseGatewayFacetToken>",
            "std::vector<ExactDirectSparseGatewayCandidateRecord>",
            "std::vector<ExactDirectSparseGatewayCandidateBatch>",
            "std::vector<std::size_t> batch_facet_token_indices",
            "no_gateway_candidate_budget_exhausted",
            "no_gateway_candidate_source_not_certified",
            "no_gateway_candidate_source_join_inconsistent",
            "no_gateway_candidate_first_incidence_budget_exhausted",
            "no_gateway_candidate_level_contradiction",
            "complete_certified_sparse_gateway_candidates",
            "build_exact_direct_sparse_gateway_candidate_journal(",
            "reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(",
            "verify_exact_direct_sparse_gateway_candidate_journal(",
            "certified_partial_refinement() const",
        ),
        "the bounded public contract",
    )
    require_all(
        header,
        (
            "observed_storage_within_budget",
            "source_incidence_journal_freshly_replayed",
            "deletion_projections_freshly_replayed",
            "facet_tokens_freshly_replayed",
            "gateway_candidates_freshly_replayed",
            "batches_freshly_replayed",
            "counters_and_result_facts_freshly_replayed",
            "no_forbidden_global_structure_materialized",
            "fresh_replay_certified",
            "result_certified",
        ),
        "the hostile fresh-verifier result",
    )

    require(
        "std::array<spatial::PointId, 11" not in header,
        "the public contract must not persist an eleven-PointId key",
    )
    require(
        "ExactDirectSparseCofaceKey" not in header,
        "the public contract must retain factorized F plus x identity",
    )
    point_id_array_11 = re.compile(
        r"\bstd\s*::\s*array\s*<\s*(?:spatial\s*::\s*)?PointId\s*,\s*"
        r"11(?:U|UL|ULL)?\s*>"
    )
    require(
        all(
            point_id_array_11.search(block) is None
            for block in cpp_record_blocks(strip_cpp_comments_and_literals(header))
        ),
        "an eleven-PointId key must not occur in a persistent record",
    )


def validate_implementation(source: str) -> None:
    code = strip_cpp_comments_and_literals(source)
    compact = compact_whitespace(code)
    qualified = re.sub(r"\s*::\s*", "::", compact)
    include_lines = "\n".join(
        line.lower()
        for line in source.splitlines()
        if line.lstrip().startswith("#include")
    )
    require(
        '"morsehgp3d/hierarchy/direct_sparse_gateway_candidate_journal.hpp"'
        in source,
        "the isolated implementation must include its own public header",
    )
    require(
        source.count("build_exact_direct_sparse_first_incidence(") == 1,
        "the implementation must contain exactly one 10.6 build call site",
    )
    require_all(
        source,
        (
            "verify_exact_direct_closed_saddle_incidence_journal_streaming(",
            "reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(",
        ),
        "the certified source replay and projection implementation",
    )
    require_all(
        qualified,
        (
            "ExactDirectSparseGatewayDeletionSource::strict_arm_seed",
            "ExactDirectSparseGatewayDeletionSource::equal_level_facet_seed",
            "ExactDirectSparseGatewayLevelRelation::first_incidence_strictly_below_saddle",
            "ExactDirectSparseGatewayLevelRelation::first_incidence_equal_to_saddle",
        ),
        "the strict/equal deletion classification",
    )

    full_key_less = compact_whitespace(
        balanced_brace_block(
            code,
            "[[nodiscard]] bool facet_key_less(",
            "the full facet-key comparator",
        )
    )
    require_all(
        full_key_less,
        (
            "left.point_count != right.point_count",
            "std::lexicographical_compare(",
            "left.point_ids.begin()",
            "right.point_ids.begin()",
        ),
        "the full facet-key comparator",
    )
    same_key = compact_whitespace(
        balanced_brace_block(
            code,
            "[[nodiscard]] bool same_key(",
            "the full-key deduplication equality",
        )
    )
    require(
        "return left.facet_key == right.facet_key" in same_key,
        "deduplication must compare the complete fixed-width facet key",
    )
    deletion_order = compact_whitespace(
        balanced_brace_block(
            code,
            "[[nodiscard]] bool deletion_scratch_less(",
            "the canonical deletion-reference comparator",
        )
    )
    require_all(
        deletion_order,
        (
            "facet_key_less(left.facet_key, right.facet_key)",
            "left.source_family_index != right.source_family_index",
            "left.removed_point_id != right.removed_point_id",
            "left.source != right.source",
            "left.source_deletion_index < right.source_deletion_index",
        ),
        "the canonical deletion-reference comparator",
    )
    require_all(
        compact,
        (
            "std::sort(scratch.begin(), scratch.end(), deletion_scratch_less)",
            "while (end < scratch.size() && same_key(scratch[begin], scratch[end]))",
            "result.required_first_incidence_call_count = distinct_facet_count",
            "result.one_first_incidence_call_per_distinct_facet = tokens.size() == distinct_facet_count",
        ),
        "the full-key sort, deduplication and one-query-per-key closure",
    )
    sorted_position = compact.find(
        "std::sort(scratch.begin(), scratch.end(), deletion_scratch_less)"
    )
    dedup_position = compact.find(
        "result.required_distinct_facet_count = distinct_facet_count",
        sorted_position,
    )
    query_position = compact.find(
        "build_exact_direct_sparse_first_incidence(", dedup_position
    )
    publication_position = compact.find(
        "result.deletion_projections = std::move(projections)", query_position
    )
    require(
        0 <= sorted_position < dedup_position < query_position < publication_position,
        "full-key deduplication must precede the unique per-key 10.6 query and "
        "atomic publication",
    )
    require_all(
        compact,
        (
            "const std::size_t remaining_gateway_candidate_count = budget.maximum_gateway_candidate_count - candidates.size()",
            "first_incidence_budget.maximum_cominimizer_count = std::min( first_incidence_budget.maximum_cominimizer_count, remaining_gateway_candidate_count)",
            "build_exact_direct_sparse_first_incidence( index, cloud, scratch[begin].facet_key, first_incidence_budget, traversal_order)",
        ),
        "the global candidate cap delegated atomically to each 10.6 call",
    )

    observed_storage = compact_whitespace(
        balanced_brace_block(
            code,
            "[[nodiscard]] bool observed_scientific_storage_within_budget(",
            "the hostile observed-storage preflight",
        )
    )
    require_all(
        observed_storage,
        (
            "observed.deletion_projections.size() > budget.maximum_deletion_reference_count",
            "observed.facet_tokens.size() > budget.maximum_distinct_facet_count",
            "observed.gateway_candidates.size() > budget.maximum_gateway_candidate_count",
            "observed.batches.size() > budget.maximum_batch_count",
            "observed.batch_facet_token_indices.size() > budget.maximum_batch_facet_reference_count",
            "facet_key_point_count > budget.maximum_facet_key_point_count",
            "candidate.positive_support_point_count > candidate.positive_support_point_ids.size()",
            "logical_storage_entry_count <= budget.maximum_logical_storage_entry_count",
            "observed.logical_storage_entry_count == logical_storage_entry_count",
        ),
        "the hostile observed-storage preflight",
    )

    clear_payload = compact_whitespace(
        balanced_brace_block(
            code,
            "void clear_scientific_payload(",
            "the fail-closed five-arena eraser",
        )
    )
    require_all(
        clear_payload,
        (
            "result.deletion_projections.clear()",
            "result.facet_tokens.clear()",
            "result.gateway_candidates.clear()",
            "result.batches.clear()",
            "result.batch_facet_token_indices.clear()",
            "result.no_partial_scientific_payload_published = true",
        ),
        "the fail-closed five-arena eraser",
    )
    publication_assignments = (
        "result.deletion_projections = std::move(projections)",
        "result.facet_tokens = std::move(tokens)",
        "result.gateway_candidates = std::move(candidates)",
        "result.batches = std::move(batches)",
        "result.batch_facet_token_indices = std::move(batch_token_indices)",
    )
    for assignment in publication_assignments:
        require(
            compact.count(assignment) == 1,
            f"atomic success must publish {assignment!r} exactly once",
        )
    assignment_positions = [compact.find(item) for item in publication_assignments]
    qualified_last_assignment = qualified.find(publication_assignments[-1])
    complete_decision = qualified.find(
        "result.decision = ExactDirectSparseGatewayCandidateDecision::complete_certified_sparse_gateway_candidates"
    )
    require(
        assignment_positions == sorted(assignment_positions)
        and assignment_positions[0] == publication_position
        and 0 <= qualified_last_assignment < complete_decision,
        "the five local arenas must move together immediately before the complete decision",
    )

    require_all(
        compact,
        (
            "result.deletion_references_sorted_by_full_key = true",
            "result.distinct_full_keys_deduplicated = true",
            "result.every_first_incidence_at_or_below_each_saddle = true",
            "result.all_positive_support_candidates_retained_atomically = true",
            "result.no_partial_scientific_payload_published = true",
            "result.no_forbidden_global_structure_materialized = true",
            "result.eleven_point_coface_keys_materialized = false",
            "result.locator_or_quotient_consulted = false",
            "result.root_union_or_forest_mutated = false",
            "result.gateway_attach_published = false",
            "result.gamma_cells_or_higher_order_delaunay_materialized = false",
        ),
        "the complete atomic publication closure",
    )
    require(
        re.search(
            r"incidence_level\s*>\s*deletion\.saddle_squared_level",
            code,
        )
        is not None,
        "every distinct-facet query must fail closed when lambda exceeds a saddle level",
    )

    point_id_array_11 = re.compile(
        r"\bstd\s*::\s*array\s*<\s*(?:spatial\s*::\s*)?PointId\s*,\s*"
        r"11(?:U|UL|ULL)?\s*>"
    )
    persistent_records = cpp_record_blocks(code)
    require(
        all(point_id_array_11.search(block) is None for block in persistent_records),
        "an eleven-PointId key must never escape transient function scratch",
    )
    reconstruction = balanced_brace_block(
        code,
        "reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(",
        "the transient K+1 deletion helper",
    )
    require(
        point_id_array_11.search(code.replace(reconstruction, " ")) is None,
        "any eleven-slot array must remain transient scratch inside the public deletion helper",
    )
    require(
        "ExactDirectSparseCofaceKey" not in code,
        "the implementation must never materialize a persistent coface key",
    )

    architecture_code = code
    for honest_negative_flag in (
        "no_forbidden_global_structure_materialized",
        "eleven_point_coface_keys_materialized",
        "locator_or_quotient_consulted",
        "root_union_or_forest_mutated",
        "gateway_attach_published",
        "gamma_cells_or_higher_order_delaunay_materialized",
    ):
        architecture_code = architecture_code.replace(honest_negative_flag, " ")
    forbidden_patterns = (
        (r"\bstd\s*::\s*(?:map|multimap|unordered_map|unordered_multimap)\s*<", "associative map"),
        (r"\b(?:build|verify|compute)[A-Za-z0-9_]*(?:gamma|delaunay|cell_atlas|critical_catalog)\s*\(", "global geometry builder"),
        (r"\bbuild_exact_direct_frozen_root_quotient", "quotient builder"),
        (r"\bbuild_exact_direct_k1_forest", "forest builder"),
        (r"\bapply_batch\s*\(", "locator apply"),
        (r"\bExactDirectSparsePositiveFacetLocator\b", "positive locator"),
        (r"\bExactDirectFrozenRootQuotient\b", "frozen-root quotient"),
        (r"\bExactDirectK1Forest[A-Za-z0-9_]*\b", "K1 forest"),
        (r"\b(?:DisjointSetUnion|UnionFind|disjoint_set|union_find)\b", "union-find structure"),
        (r"\b(?:RootBirth|root_birth|AtomicUnionBatch|atomic_union_batch|GatewayAttach|gateway_attach)\b", "root/union/attach action"),
        (r"\b(?:ordinary|power)_?cell\b", "global cell"),
        (r"\b(?:facet|coface)_?catalog\b", "global facet/coface catalog"),
        (r"\bstd\s*::\s*vector\s*<\s*std\s*::\s*vector\s*<", "nested global catalog"),
    )
    for pattern, label in forbidden_patterns:
        require(
            re.search(pattern, architecture_code, re.I) is None,
            f"the isolated implementation contains forbidden {label}",
        )
    require(
        not re.search(
            r"gamma|delaunay|cell[_-]?atlas|critical[_-]?catalog|"
            r"frozen[_-]?root|k1[_-]?forest|positive[_-]?facet[_-]?locator",
            include_lines,
        ),
        "the isolated source includes a forbidden global-structure header",
    )


def validate_unit_test(test: str) -> None:
    compact = compact_whitespace(test)
    qualified = re.sub(r"\s*::\s*", "::", compact)
    require_all(
        compact,
        (
            "test_complete_empty_source_issues_no_geometry_query",
            "test_small_terminal_source_projects_every_deletion",
            "test_shared_facet_is_deduplicated_once_and_near_far_agree",
            "test_permanent_ac_fixture_uses_real_direct_provenance",
            "test_bounded_direct_keys_match_all_explicit_one_point_cofaces",
            "test_input_permutations_preserve_the_canonical_event_stream",
            "test_k10_factorization_and_k_plus_one_deletion_helper",
            "test_global_and_nested_budgets_publish_five_empty_arenas",
            "test_hostile_verifier_bounds_storage_before_source_replay",
            "test_authority_source_and_lbvh_divergences_fail_closed",
            "LbvhTraversalOrder::near_first",
            "LbvhTraversalOrder::far_first",
            "maximum_source_family_scan_count",
            "maximum_deletion_reference_count",
            "maximum_distinct_facet_count",
            "maximum_facet_key_point_count",
            "maximum_gateway_candidate_count",
            "maximum_batch_count",
            "maximum_batch_facet_reference_count",
            "maximum_logical_storage_entry_count",
            "no_gateway_candidate_budget_exhausted",
            "no_gateway_candidate_first_incidence_budget_exhausted",
            "reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(",
            "ExactDirectSparseGatewayCandidateBudget{}",
            "cloud.size() <= 14U",
            "exhaustive_all_one_point_cofaces(",
            "build_exact_facet_miniball(",
            "expected_direct_keys == observed_direct_keys",
            "exact_child_budget_for(",
            "oversized_logical_storage.logical_storage_entry_count",
            "std::reverse(points.begin(), points.end())",
            "noncanonical_source.facade.events.begin()",
        ),
        "the short gateway-candidate unit suite",
    )
    require_all(
        qualified,
        (
            "ExactDirectSparseGatewayLevelRelation::first_incidence_strictly_below_saddle",
            "ExactDirectSparseGatewayLevelRelation::first_incidence_equal_to_saddle",
        ),
        "the strict/equal projection assertions",
    )
    test_functions = (
        "test_contract_metadata_and_sparse_scope",
        "test_complete_empty_source_issues_no_geometry_query",
        "test_small_terminal_source_projects_every_deletion",
        "test_shared_facet_is_deduplicated_once_and_near_far_agree",
        "test_permanent_ac_fixture_uses_real_direct_provenance",
        "test_bounded_direct_keys_match_all_explicit_one_point_cofaces",
        "test_input_permutations_preserve_the_canonical_event_stream",
        "test_k10_factorization_and_k_plus_one_deletion_helper",
        "test_global_and_nested_budgets_publish_five_empty_arenas",
        "test_hostile_verifier_bounds_storage_before_source_replay",
        "test_authority_source_and_lbvh_divergences_fail_closed",
    )
    main_block = balanced_brace_block(test, "int main()", "the unit main")
    for test_function in test_functions:
        require(
            test.count(test_function) == 2
            and main_block.count(f"{test_function}();") == 1,
            f"{test_function} must be defined and executed exactly once",
        )
    require_all(
        compact,
        (
            "result.required_first_incidence_call_count == 0U",
            "source.incidence_journal.families.empty()",
            "ac->source_miniball_squared_level == level(33, 2)",
            "minimizers == std::vector<PointId>({0U, 4U})",
            "mirror_token_indices.size() == 5U",
            "shared_minimizers == std::vector<PointId>({1U, 2U})",
            "projections.size() == 11U",
            "token_indices.size() == 11U",
            "reconstructed_deletions == expected_deletions",
            "using ChildBudgetMember = std::size_t ExactDirectSparseFirstIncidenceBudget::*",
            "--(budget.first_incidence_budget.*member)",
        ),
        "the empty, mirror, AC, K=10 and nested-budget assertions",
    )
    child_cap_members = (
        "maximum_source_support_enumeration_count",
        "maximum_node_visit_count",
        "maximum_internal_node_expansion_count",
        "maximum_exact_aabb_bound_evaluation_count",
        "maximum_exact_point_evaluation_count",
        "maximum_coface_support_enumeration_count",
        "maximum_candidate_point_classification_count",
        "maximum_frontier_entry_count",
        "maximum_cominimizer_count",
    )
    for member in child_cap_members:
        require(
            compact.count(member) >= 2,
            f"the exact/minus-one 10.6 matrix omits {member}",
        )
    for arena in (
        "deletion_projections",
        "facet_tokens",
        "gateway_candidates",
        "batches",
        "batch_facet_token_indices",
    ):
        require(
            f"result.{arena}.empty()" in compact,
            f"budget failures must explicitly inspect the empty {arena} arena",
        )
    require(
        "source_facet_key.point_count == 10U" in compact,
        "the K=10 fixture must certify its fixed ten-PointId facet key",
    )
    require(
        "std::array<PointId, 11U>" not in test,
        "the unit suite must exercise K+1 through the public factorized helper",
    )


def validate_symbols(binary: Path, nm: Path) -> None:
    require(binary.is_file(), f"the isolated archive does not exist: {binary}")
    require(nm.is_file(), f"the nm executable does not exist: {nm}")
    try:
        symbols = subprocess.run(
            [str(nm), "-C", str(binary)],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.lower()
    except (OSError, subprocess.CalledProcessError) as error:
        raise ContractError(f"cannot inspect target symbols: {error}") from error

    require_all(
        symbols,
        (
            "build_exact_direct_sparse_gateway_candidate_journal",
            "verify_exact_direct_sparse_gateway_candidate_journal",
            "reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet",
            "build_exact_direct_sparse_first_incidence",
            "verify_exact_direct_closed_saddle_incidence_journal_streaming",
        ),
        "the isolated target symbol closure",
    )
    for forbidden in (
        "gamma",
        "delaunay",
        "cellatlas",
        "cell_atlas",
        "critical_catalog",
        "criticalcatalog",
        "global_facet_catalog",
        "globalfacetcatalog",
        "global_coface_catalog",
        "globalcofacecatalog",
        "ordinary_cell",
        "power_cell",
        "facet_catalog",
        "coface_catalog",
        "std::map<",
        "std::multimap<",
        "std::unordered_map<",
        "std::unordered_multimap<",
        "apply_batch",
        "build_exact_direct_frozen_root_quotient",
        "build_exact_direct_k1_forest",
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
        project
        / "include/morsehgp3d/hierarchy/direct_sparse_gateway_candidate_journal.hpp"
    )
    source = read_text(
        project
        / "src/cpu/hierarchy/direct_sparse_gateway_candidate_journal.cpp"
    )
    test = read_text(
        project
        / "tests/unit/test_hierarchy_direct_sparse_gateway_candidate_journal.cpp"
    )

    validate_main_cmake(cmake)
    validate_test_wiring(unit_cmake)
    validate_public_contract(header)
    validate_implementation(source)
    validate_unit_test(test)
    if binary is not None and nm is not None:
        validate_symbols(binary, nm)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("project", type=Path)
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--nm", type=Path)
    arguments = parser.parse_args()
    try:
        validate(arguments.project.resolve(), arguments.binary, arguments.nm)
    except ContractError as error:
        print(f"Phase-10.7-RCPU sparse gateway-candidate contract failed: {error}")
        return 1
    print("Phase-10.7-RCPU sparse gateway-candidate contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
