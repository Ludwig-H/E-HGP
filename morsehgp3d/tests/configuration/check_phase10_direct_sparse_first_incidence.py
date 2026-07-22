#!/usr/bin/env python3
"""Validate the isolated Phase-10.6-RCPU sparse first-incidence target."""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


class ContractError(ValueError):
    """A direct sparse first-incidence invariant was violated."""


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


def balanced_calls(text: str, marker: str) -> list[str]:
    """Return every balanced call beginning with ``marker``."""

    calls: list[str] = []
    position = 0
    while True:
        start = text.find(marker, position)
        if start < 0:
            return calls
        opening = text.find("(", start + len(marker) - 1)
        require(opening >= 0, f"missing opening parenthesis after {marker!r}")
        depth = 0
        for index in range(opening, len(text)):
            if text[index] == "(":
                depth += 1
            elif text[index] == ")":
                depth -= 1
                if depth == 0:
                    calls.append(text[start : index + 1])
                    position = index + 1
                    break
        else:
            raise ContractError(f"unterminated call {marker!r}")


def balanced_brace_block(text: str, marker: str, context: str) -> str:
    """Return the brace-balanced C++ block following a unique marker."""

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
    """Return top-level class/struct bodies used to audit persistent fields."""

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
    target = "morsehgp3d_direct_sparse_first_incidence"
    alias = "morsehgp3d::direct_sparse_first_incidence"

    target_block = unique_target_block(cmake, "add_library", target)
    require("STATIC" in target_block, "first incidence must remain a static archive")
    require(
        re.findall(r"[A-Za-z0-9_./+-]+\.cpp", target_block)
        == ["src/cpu/hierarchy/direct_sparse_first_incidence.cpp"],
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
        ("EXPORT_NAME direct_sparse_first_incidence", "POSITION_INDEPENDENT_CODE ON"),
        "the first-incidence export properties",
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
            r"morsehgp3d::direct_sparse_positive_facet_locator\s+"
            r"morsehgp3d::facet_miniball\s+"
            r"morsehgp3d::spatial\s+PRIVATE\s+"
            r"morsehgp3d::sanitizers\s*\)",
            links,
            re.I,
        )
        is not None,
        "the archive must link exactly locator + facet_miniball + spatial, "
        "with sanitizers private",
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
    test_target = "morsehgp3d_hierarchy_direct_sparse_first_incidence_tests"
    public_target = "morsehgp3d::direct_sparse_first_incidence"

    executable = unique_target_block(unit_cmake, "add_executable", test_target)
    require(
        re.findall(r"[A-Za-z0-9_./+-]+\.cpp", executable)
        == ["test_hierarchy_direct_sparse_first_incidence.cpp"],
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
        "morsehgp3d.hierarchy_direct_sparse_first_incidence",
        "the functional CTest",
    )
    require(test_target in functional, "the functional CTest runs the wrong target")

    configuration = unique_marked_block(
        unit_cmake,
        "add_test",
        "morsehgp3d.phase10_direct_sparse_first_incidence_configuration",
        "the configuration CTest",
    )
    require(
        "_morsehgp3d_phase10_direct_sparse_first_incidence_"
        "configuration_command" in configuration,
        "the configuration CTest must use its assembled command",
    )
    require_all(
        unit_cmake,
        (
            "check_phase10_direct_sparse_first_incidence.py",
            "$<TARGET_FILE:morsehgp3d_direct_sparse_first_incidence>",
            "--binary",
            "--nm",
            "CMAKE_NM",
        ),
        "the static and symbolic checker wiring",
    )


def validate_budget_and_atomicity(header: str, code: str) -> None:
    require_all(
        header,
        (
            "maximum_source_support_enumeration_count",
            "maximum_node_visit_count",
            "maximum_internal_node_expansion_count",
            "maximum_exact_aabb_bound_evaluation_count",
            "maximum_exact_point_evaluation_count",
            "maximum_coface_support_enumeration_count",
            "maximum_candidate_point_classification_count",
            "maximum_frontier_entry_count",
            "maximum_cominimizer_count",
            "source_support_enumeration_limit",
            "node_visit_limit",
            "internal_node_expansion_limit",
            "exact_aabb_bound_evaluation_limit",
            "exact_point_evaluation_limit",
            "coface_support_enumeration_limit",
            "candidate_point_classification_limit",
            "frontier_entry_limit",
            "cominimizer_entry_limit",
        ),
        "the operation-specific fail-closed budget",
    )
    compact = compact_whitespace(code)
    require_all(
        compact,
        (
            "audit.source_support_enumeration_count <= budget.maximum_source_support_enumeration_count",
            "audit.node_visit_count <= budget.maximum_node_visit_count",
            "audit.internal_node_expansion_count <= budget.maximum_internal_node_expansion_count",
            "audit.exact_aabb_bound_evaluation_count <= budget.maximum_exact_aabb_bound_evaluation_count",
            "audit.exact_point_evaluation_count <= budget.maximum_exact_point_evaluation_count",
            "audit.coface_support_enumeration_count <= budget.maximum_coface_support_enumeration_count",
            "audit.candidate_point_classification_count <= budget.maximum_candidate_point_classification_count",
            "audit.peak_frontier_entry_count <= budget.maximum_frontier_entry_count",
            "audit.peak_cominimizer_entry_count <= budget.maximum_cominimizer_count",
        ),
        "the result-to-budget audit closure",
    )

    exhausted = balanced_brace_block(
        code,
        "[[nodiscard]] ExactDirectSparseFirstIncidenceResult exhausted(",
        "the atomic exhaustion publisher",
    )
    exhausted_compact = compact_whitespace(exhausted)
    require_all(
        exhausted_compact,
        (
            "result_.first_incidence_squared_level.reset()",
            "result_.cominimizers",
            "result_.all_cominimizers_retained_atomically = false",
            "result_.no_partial_first_incidence_payload_published = true",
            "no_first_incidence_budget_exhausted",
        ),
        "the atomic exhaustion publisher",
    )
    require(
        "swap( result_.cominimizers)" in exhausted_compact
        or "result_.cominimizers.clear()" in exhausted_compact,
        "budget exhaustion must erase every provisional co-minimizer",
    )
    require(
        compact.count("result_.all_cominimizers_retained_atomically = true")
        >= 2,
        "both complete outcomes must certify atomic co-minimizer publication",
    )
    require_all(
        compact,
        (
            "result_.cominimizers.clear()",
            "cominimizer_overflowed_ = false",
            "if (cominimizer_overflowed_)",
            "cominimizer_entry_limit",
            "result_.first_incidence_squared_level = incumbent_squared_level_",
        ),
        "the provisional-shell recovery and terminal atomicity logic",
    )


def validate_fresh_replay(header: str, code: str) -> None:
    require_all(
        header,
        (
            "observed_storage_within_budget",
            "source_miniball_freshly_replayed",
            "branch_and_bound_freshly_replayed",
            "all_cominimizers_freshly_replayed",
            "counters_and_decision_freshly_replayed",
            "no_forbidden_global_structure_materialized",
            "fresh_replay_certified",
            "result_certified",
        ),
        "the hostile fresh-verifier result",
    )
    storage_guard = balanced_brace_block(
        code,
        "[[nodiscard]] bool observed_storage_within_trusted_bounds(",
        "the observed-storage preflight",
    )
    storage_compact = compact_whitespace(storage_guard)
    require_all(
        storage_compact,
        (
            "observed.cominimizers.size() > eligible_point_count",
            "observed.cominimizers.size() > budget.maximum_cominimizer_count",
            "source.facet_point_ids.size() > direct_sparse_first_incidence_maximum_source_point_count",
            "source.support_point_ids.size() > ExactFacetMiniballResult::maximum_support_point_count",
            "source.strictly_inside_point_ids.size() > direct_sparse_first_incidence_maximum_source_point_count",
            "source.boundary_point_ids.size() > direct_sparse_first_incidence_maximum_source_point_count",
            "minimizer.support_point_count > minimizer.support_point_ids.size()",
        ),
        "the observed-storage preflight",
    )

    compact = compact_whitespace(code)
    guard_assignment = compact.find(
        "verification.observed_storage_within_budget = observed_storage_within_trusted_bounds("
    )
    guard_return = compact.find(
        "if (!verification.observed_storage_within_budget)", guard_assignment
    )
    replay = compact.find(
        "const ExactDirectSparseFirstIncidenceResult expected =", guard_return
    )
    require(
        0 <= guard_assignment < guard_return < replay,
        "the verifier must reject hostile storage before any scientific replay",
    )
    replay_calls = balanced_calls(
        compact[replay:], "build_exact_direct_sparse_first_incidence("
    )
    require(
        len(replay_calls) == 1 and "observed" not in replay_calls[0],
        "fresh replay must rebuild once from trusted inputs, never observed science",
    )
    require_all(
        compact[replay:],
        (
            "verification.source_miniball_freshly_replayed =",
            "verification.branch_and_bound_freshly_replayed =",
            "verification.all_cominimizers_freshly_replayed =",
            "verification.counters_and_decision_freshly_replayed =",
            "verification.no_forbidden_global_structure_materialized =",
            "verification.fresh_replay_certified =",
            "verification.result_certified = verification.fresh_replay_certified && expected_certified",
        ),
        "the fresh replay comparison closure",
    )


def validate_forbidden_architecture(header: str, source: str) -> None:
    code = strip_cpp_comments_and_literals(header + "\n" + source)
    lowered = code.lower().replace(
        "no_gamma_or_higher_order_delaunay_materialized", ""
    ).replace("no_global_facet_or_coface_catalog_materialized", "")

    forbidden_patterns = (
        (r"\bgamma\b|build_exact_gamma|morse_gamma", "Gamma"),
        (r"delaunay", "Delaunay"),
        (r"\blbvh\s*_?\s*top\s*_?\s*k\b", "LBVH top-k"),
        (r"\btop\s*_?\s*k\s*partition\b", "TopKPartition"),
        (r"\bexactbudgetedlbvhtopkresult\b", "budgeted top-k result"),
        (r"\bcritical\s*_?\s*catalog\b", "critical catalog"),
        (r"\b(?:global\s*_?\s*)?(?:facet|coface)\s*_?\s*catalog\b", "global facet/coface catalog"),
        (r"\bcell\s*_?\s*atlas\b", "cell atlas"),
        (r"\b(?:ordinary|power)\s*_?\s*cell\b", "global cell"),
        (r"\bhigher\s*_?\s*support\b", "higher-support materialization"),
        (
            r"\bstd\s*::\s*(?:map|multimap|unordered_map|unordered_multimap)\s*<",
            "associative map",
        ),
        (
            r"\bstd\s*::\s*vector\s*<\s*std\s*::\s*vector\s*<",
            "nested vector catalog",
        ),
        (
            r"\bstd\s*::\s*vector\s*<\s*ExactDirectSparseFacetKey\s*>",
            "persistent facet-key catalog",
        ),
    )
    for pattern, label in forbidden_patterns:
        require(
            re.search(pattern, lowered, re.I) is None,
            f"the isolated target contains forbidden {label}",
        )

    point_id_array_11 = re.compile(
        r"\bstd\s*::\s*array\s*<\s*(?:spatial\s*::\s*)?PointId\s*,\s*11(?:U|UL|ULL)?\s*>"
    )
    persistent_records = cpp_record_blocks(
        strip_cpp_comments_and_literals(header)
    ) + cpp_record_blocks(strip_cpp_comments_and_literals(source))
    require(
        all(point_id_array_11.search(block) is None for block in persistent_records),
        "an eleven-PointId key must never escape transient scratch storage",
    )
    require(
        re.search(
            r"\bpoint_count\s*(?:=|\{)\s*11(?:U|UL|ULL)?\b", code
        )
        is None,
        "an eleven-point persistent facet/coface key is forbidden",
    )


def validate_contract(header: str, source: str, lbvh_header: str) -> None:
    compact_header = compact_whitespace(header)
    require_all(
        compact_header,
        (
            'direct_sparse_first_incidence_backend = "reference_cpu"',
            'direct_sparse_first_incidence_profile = "hgp_reduced"',
            'direct_sparse_first_incidence_mode = "certified"',
            'direct_sparse_first_incidence_refinement_status = "partial_refinement"',
            'direct_sparse_first_incidence_public_status = "not_claimed"',
        ),
        "the Phase-10.6-RCPU status axes",
    )
    require_all(
        compact_header,
        (
            "direct_sparse_first_incidence_maximum_source_point_count = 10U",
            "direct_sparse_first_incidence_source_support_enumeration_count_per_pass = 385U",
            "direct_sparse_first_incidence_maximum_source_support_enumeration_count = 770U",
            "direct_sparse_first_incidence_maximum_outside_coface_support_count = 176U",
            "direct_sparse_first_incidence_maximum_outside_coface_classification_count = 1936U",
        ),
        "the K<=10 bounded reduction",
    )
    require_all(
        header,
        (
            "support_radial_and_pair_aabb_lower_bound_strict_pruning_all_",
            "ExactDirectSparseFirstIncidenceBudget",
            "ExactDirectSparseFirstIncidenceAudit",
            "eligible_coface_point_count",
            "exact_aabb_bound_evaluation_count",
            "coface_support_enumeration_count",
            "candidate_point_classification_count",
            "pruned_eligible_point_count",
            "provisional_cominimizer_overflow_count",
            "peak_cominimizer_entry_count",
            "all_cominimizers_retained_atomically",
            "no_partial_first_incidence_payload_published",
            "no_global_facet_or_coface_catalog_materialized",
            "no_gamma_or_higher_order_delaunay_materialized",
            "build_exact_direct_sparse_first_incidence(",
            "verify_exact_direct_sparse_first_incidence(",
        ),
        "the bounded public contract",
    )

    code = strip_cpp_comments_and_literals(source)
    compact_code = compact_whitespace(code)
    require_all(
        compact_code,
        (
            "minimum_source_center_squared_distance <= source_squared_radius",
            "sum * sum / denominator",
            "entry.coface_squared_level_lower_bound > *incumbent_squared_level_",
            "result_.equality_bounds_always_descended = true",
            "outside_coface_support_count(source_point_ids_.size())",
            "static_assert(SourceSubsetSize <= 3U)",
            "exact::analyze_circumcenter_support(support)",
            "exact::classify_sphere_point(",
            "result_.all_cominimizers_retained_atomically = false",
            "result_.no_partial_first_incidence_payload_published = true",
            "const exact::ExactLevel pair_lower = quarter_level(",
            "if (pair_lower > lower)",
        ),
        "the exact radial-and-pair branch-and-bound implementation",
    )
    require(
        "entry.coface_squared_level_lower_bound >= *incumbent_squared_level_"
        not in compact_code,
        "an equality AABB bound must always be descended",
    )
    require(
        re.search(
            r"entry\.coface_squared_level_lower_bound\s*>\s*=\s*"
            r"\*\s*incumbent_squared_level_",
            code,
        )
        is None,
        "strict pruning must not be weakened to a spaced >= comparison",
    )

    radial_block = balanced_brace_block(
        code,
        "[[nodiscard]] exact::ExactLevel coface_lower_bound(",
        "the exact radial lower bound",
    )
    radial_compact = compact_whitespace(radial_block)
    require_all(
        radial_compact,
        (
            "minimum_source_center_squared_distance <= source_squared_radius",
            "return source_squared_radius",
            "minimum_source_center_squared_distance.rational() + source_squared_radius.rational()",
            "exact::ExactRational{exact::BigInt{4}} * minimum_source_center_squared_distance.rational()",
            "sum * sum / denominator",
        ),
        "the exact radial MEB/AABB lower-bound formula",
    )

    quarter_block = balanced_brace_block(
        code,
        "[[nodiscard]] exact::ExactLevel quarter_level(",
        "the exact pair lower bound",
    )
    require(
        "level.rational() / exact::ExactRational{exact::BigInt{4}}"
        in compact_whitespace(quarter_block),
        "every pair lower bound must be the exact squared distance divided by four",
    )

    node_bound = balanced_brace_block(
        code,
        "[[nodiscard]] exact::ExactLevel node_lower_bound(",
        "the combined node lower bound",
    )
    node_compact = compact_whitespace(node_bound)
    radial_query = node_compact.find(
        "result_.source_facet_miniball->center"
    )
    all_source_loop = node_compact.find(
        "for (const PointId point_id : source_point_ids_)"
    )
    pair_query = node_compact.find("cloud_.point(point_id).exact()")
    pair_maximum = node_compact.find("if (pair_lower > lower)")
    returned_bound = node_compact.rfind("return lower")
    require(
        0
        <= radial_query
        < all_source_loop
        < pair_query
        < pair_maximum
        < returned_bound,
        "the node bound must take max(radial, max over every p in F of pair(p,node))",
    )
    require(
        node_compact.count(
            "++result_.audit.exact_aabb_bound_evaluation_count"
        )
        == 2,
        "the radial call and looped pair call must each increment the AABB audit",
    )

    aabb_calls = balanced_calls(code, "minimum_squared_distance_to_node(")
    require(
        len(aabb_calls) == 2,
        "the combined bound must contain one radial and one source-point AABB call",
    )
    compact_calls = tuple(compact_whitespace(call) for call in aabb_calls)
    require(
        sum("source_facet_miniball->center" in call for call in compact_calls) == 1,
        "the combined bound requires exactly one radial source-center query",
    )
    require(
        sum("cloud_.point(point_id).exact()" in call for call in compact_calls) == 1,
        "the combined bound requires exactly one looped source-point query",
    )
    require_all(
        compact_code,
        (
            "const std::size_t bound_evaluations_per_node = source_point_ids_.size() + 1U",
            "child_bound_evaluation_count, bound_evaluations_per_node",
            "node_lower_bound(index_.root_index_)",
        ),
        "the exact K+1-per-node AABB budget",
    )
    per_node_count = compact_code.find(
        "const std::size_t bound_evaluations_per_node = source_point_ids_.size() + 1U"
    )
    root_preflight = compact_code.find(
        "if (bound_evaluations_per_node > budget_.maximum_exact_aabb_bound_evaluation_count)",
        per_node_count,
    )
    root_bound = compact_code.find(
        "node_lower_bound(index_.root_index_)", root_preflight
    )
    child_count = compact_code.find(
        "std::size_t child_bound_evaluation_count = 0U", root_bound
    )
    child_loop = compact_code.find(
        "for (const std::size_t child : {node.left_child, node.right_child})",
        child_count,
    )
    require(
        0 <= per_node_count < root_preflight < root_bound < child_count < child_loop,
        "root and two-child AABB budgets must be preflighted before evaluation",
    )
    require(
        compact_code[child_count:child_loop].count(
            "child_bound_evaluation_count, bound_evaluations_per_node"
        )
        == 2,
        "an internal node must preflight exactly 2(K+1) AABB evaluations",
    )

    require(
        source.count("build_exact_facet_miniball(") == 1,
        "the source miniball must be built exactly once",
    )
    require(
        "verify_exact_facet_miniball(" not in source,
        "the public source build already performs its fresh replay; a second verify "
        "would make the declared 770-support budget dishonest",
    )
    preflight_start = source.find("source_support_count_per_pass")
    preflight_end = source.find("result_.source_facet_miniball =", preflight_start)
    require(
        0 <= preflight_start < preflight_end
        and source[preflight_start:preflight_end].count("checked_add_counter(") == 2,
        "the source-support preflight must account for exactly two bounded passes",
    )
    for subset_size in range(4):
        require(
            f"std::array<std::size_t, {subset_size}U>" in source,
            f"the outside reduction omits source-subset size {subset_size}",
        )

    require_all(
        lbvh_header,
        (
            "class ExactDirectSparseFirstIncidenceBuilder;",
            "friend class hierarchy::ExactDirectSparseFirstIncidenceBuilder;",
        ),
        "the narrow LBVH friend seam",
    )
    require(
        lbvh_header.count("friend class hierarchy::ExactDirectSparseFirstIncidenceBuilder;")
        == 1,
        "the LBVH friend declaration must occur exactly once",
    )
    require(
        lbvh_header.count("class ExactDirectSparseFirstIncidenceBuilder;") == 1
        and lbvh_header.count("ExactDirectSparseFirstIncidenceBuilder") == 2,
        "the LBVH seam must contain one forward declaration and one friend only",
    )
    require(
        code.count("class ExactDirectSparseFirstIncidenceBuilder") == 1,
        "the isolated implementation must define exactly one LBVH friend builder",
    )
    require(
        "const spatial::MortonLbvhIndex& index_;" in source,
        "the builder must retain only a const LBVH reference",
    )
    private_accesses = set(re.findall(r"\bindex_\.([A-Za-z_]\w*)", code))
    allowed_accesses = {
        "leaf_position_by_point_id_",
        "leaves_",
        "minimum_squared_distance_to_node",
        "nodes_",
        "root_index_",
    }
    require(
        private_accesses == allowed_accesses,
        "the LBVH friend must use exactly its read-only cursor fields; observed "
        f"{sorted(private_accesses)!r}",
    )
    require("const_cast" not in code, "the read-only LBVH seam forbids const_cast")
    require(
        re.search(
            r"index_\.(?:nodes_|leaves_|leaf_position_by_point_id_)\s*\.\s*"
            r"(?:push_back|clear|resize|assign|swap|erase|insert)\s*\(",
            compact_code,
        )
        is None,
        "the LBVH friend must not mutate private index storage",
    )

    include_lines = "\n".join(
        line.lower() for line in source.splitlines() if line.lstrip().startswith("#include")
    )
    require(
        not re.search(
            r"gamma|delaunay|top[_-]?k|cell[_-]?atlas|critical[_-]?catalog",
            include_lines,
        ),
        "the isolated source includes a forbidden global geometry header",
    )

    validate_budget_and_atomicity(header, code)
    validate_fresh_replay(header, code)
    validate_forbidden_architecture(header, source)


def validate_unit_test(test: str) -> None:
    compact_test = compact_whitespace(test)
    require_all(
        compact_test,
        (
            "test_no_coface_when_n_equals_k",
            "test_invalid_authorities_are_rejected_before_traversal",
            "test_inside_boundary_and_selected_support_semantics",
            "test_equal_cominimizers_strict_pruning_and_traversal_invariance",
            "test_new_support_uses_a_point_outside_the_old_support",
            "test_k10_avoids_an_eleven_point_key_and_uses_176_supports",
            "test_provisional_overflow_is_erased_by_a_better_incumbent",
            "test_k10_retains_two_outside_cominimizers",
            "test_small_bounded_differential_against_full_miniballs",
            "test_bounded_n14_differential_against_all_explicit_cofaces",
            "test_all_budget_exhaustions_publish_no_partial_shell",
            "test_hostile_verifier_rejects_storage_and_scientific_mutations",
            "direct_sparse_first_incidence_source_support_enumeration_count_per_pass == 385U",
            "direct_sparse_first_incidence_maximum_source_support_enumeration_count == 770U",
            "direct_sparse_first_incidence_maximum_outside_coface_support_count == 176U",
            "direct_sparse_first_incidence_maximum_outside_coface_classification_count == 1936U",
            "!result.all_cominimizers_retained_atomically",
            "result.no_partial_first_incidence_payload_published",
            "LbvhTraversalOrder::near_first",
            "LbvhTraversalOrder::far_first",
        ),
        "the short first-incidence unit suite",
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
            "build_exact_direct_sparse_first_incidence",
            "verify_exact_direct_sparse_first_incidence",
            "build_exact_facet_miniball",
            "classify_sphere_point",
            "analyze_circumcenter_support",
            "minimum_squared_distance_to_node",
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
        "build_global_facet",
        "build_global_coface",
        "ordinary_cell",
        "power_cell",
        "higher_support",
        "topkpartition",
        "top_k_partition",
        "exactbudgetedlbvhtopkresult",
        "lbvh_top_k",
        "lbvhtopk",
        "closedballpartition",
        "std::map<",
        "std::multimap<",
        "std::unordered_map<",
        "std::unordered_multimap<",
        "compute_exact_facet_descent_preconditions",
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
        / "include/morsehgp3d/hierarchy/direct_sparse_first_incidence.hpp"
    )
    source = read_text(
        project / "src/cpu/hierarchy/direct_sparse_first_incidence.cpp"
    )
    lbvh_header = read_text(project / "include/morsehgp3d/spatial/lbvh.hpp")
    test = read_text(
        project / "tests/unit/test_hierarchy_direct_sparse_first_incidence.cpp"
    )

    validate_main_cmake(cmake)
    validate_test_wiring(unit_cmake)
    validate_contract(header, source, lbvh_header)
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
        print(f"Phase-10.6-RCPU sparse first-incidence contract failed: {error}")
        return 1
    print("Phase-10.6-RCPU sparse first-incidence contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
