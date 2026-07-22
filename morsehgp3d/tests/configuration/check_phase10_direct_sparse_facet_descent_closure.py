#!/usr/bin/env python3
"""Validate the isolated Phase-10.5c sparse facet-descent closure target."""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


class ContractError(ValueError):
    """A direct sparse facet-descent closure invariant was violated."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def require_all(text: str, needles: tuple[str, ...], context: str) -> None:
    for needle in needles:
        require(needle in text, f"{context} is missing {needle!r}")


def require_any(text: str, needles: tuple[str, ...], context: str) -> None:
    require(
        any(needle in text for needle in needles),
        f"{context} is missing every accepted spelling {needles!r}",
    )


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        raise ContractError(f"cannot read {path}: {error}") from error


def compact_whitespace(text: str) -> str:
    return " ".join(text.split())


def cmake_blocks(text: str, command: str) -> list[str]:
    """Return balanced CMake command blocks, independent of line wrapping."""

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
            raise ContractError(
                f"unterminated {command} block starting at byte {match.start()}"
            )
    return blocks


def unique_cmake_block(
    text: str, command: str, marker: str, context: str
) -> str:
    matches = [block for block in cmake_blocks(text, command) if marker in block]
    require(
        len(matches) == 1,
        f"{context} requires exactly one {command} block containing {marker!r}; "
        f"found {len(matches)}",
    )
    return matches[0]


def unique_cmake_target_block(
    text: str, command: str, target: str, context: str
) -> str:
    """Return the block whose first CMake argument is exactly ``target``."""

    first_argument = re.compile(
        rf"^{re.escape(command)}\s*\(\s*{re.escape(target)}(?=\s|\))",
        re.I,
    )
    matches = [
        block
        for block in cmake_blocks(text, command)
        if first_argument.search(block)
    ]
    require(
        len(matches) == 1,
        f"{context} requires exactly one {command} block for target "
        f"{target!r}; found {len(matches)}",
    )
    return matches[0]


def strip_cpp_comments_and_literals(text: str) -> str:
    """Remove comments and literals before checking forbidden code paths."""

    pattern = re.compile(
        r"//[^\n]*|/\*.*?\*/|R\"([^ ()\\\t\r\n]*)\(.*?\)\1\"|"
        r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        re.S,
    )
    return pattern.sub(" ", text)


def validate_main_cmake(cmake: str) -> None:
    target_name = "morsehgp3d_direct_sparse_facet_descent_closure"
    alias_name = "morsehgp3d::direct_sparse_facet_descent_closure"

    target = unique_cmake_target_block(
        cmake, "add_library", target_name, "the Phase-10.5c library target"
    )
    sources = re.findall(r"[A-Za-z0-9_./+-]+\.cpp", target)
    require(
        sources
        == ["src/cpu/hierarchy/direct_sparse_facet_descent_closure.cpp"],
        "the Phase-10.5c target must contain exactly its isolated closure source",
    )
    require(
        "STATIC" in target,
        "the Phase-10.5c closure must remain an isolated static archive",
    )

    alias = unique_cmake_target_block(
        cmake, "add_library", alias_name, "the Phase-10.5c alias target"
    )
    require(
        "ALIAS" in alias and target_name in alias,
        "the public closure alias must name the isolated archive",
    )

    properties = unique_cmake_target_block(
        cmake, "set_target_properties", target_name, "the closure properties"
    )
    compact_properties = compact_whitespace(properties)
    require_all(
        compact_properties,
        (
            "EXPORT_NAME direct_sparse_facet_descent_closure",
            "POSITION_INDEPENDENT_CODE ON",
        ),
        "the closure export properties",
    )

    features = unique_cmake_target_block(
        cmake, "target_compile_features", target_name, "the closure C++ features"
    )
    require(
        "PUBLIC cxx_std_20" in compact_whitespace(features),
        "the closure target must publish its C++20 requirement",
    )

    includes = unique_cmake_target_block(
        cmake, "target_include_directories", target_name, "the closure includes"
    )
    require_all(
        includes,
        ("BUILD_INTERFACE", "INSTALL_INTERFACE"),
        "the closure public include routing",
    )

    links = unique_cmake_target_block(
        cmake, "target_link_libraries", target_name, "the closure link closure"
    )
    compact_links = compact_whitespace(links)
    require(
        re.fullmatch(
            r"target_link_libraries\s*\(\s*"
            + target_name
            + r"\s+PUBLIC\s+"
            r"morsehgp3d::direct_sparse_facet_descent_step\s+PRIVATE\s+"
            r"morsehgp3d::sanitizers\s*\)",
            compact_links,
            re.I,
        )
        is not None,
        "the closure must link exactly and publicly through the certified "
        "10.5b step, plus the private sanitizer policy",
    )

    install_blocks = [
        block
        for block in cmake_blocks(cmake, "install")
        if re.search(r"\bTARGETS\b", block)
    ]
    require(
        sum(target_name in block for block in install_blocks) == 1,
        "the closure archive must appear exactly once in install(TARGETS ...)",
    )


def validate_unit_cmake(unit_cmake: str) -> None:
    test_target_name = (
        "morsehgp3d_hierarchy_direct_sparse_facet_descent_closure_tests"
    )
    public_target = "morsehgp3d::direct_sparse_facet_descent_closure"

    target = unique_cmake_target_block(
        unit_cmake,
        "add_executable",
        test_target_name,
        "the Phase-10.5c unit target",
    )
    sources = re.findall(r"[A-Za-z0-9_./+-]+\.cpp", target)
    require(
        sources
        == ["test_hierarchy_direct_sparse_facet_descent_closure.cpp"],
        "the closure unit target must contain exactly its dedicated test source",
    )

    links = unique_cmake_target_block(
        unit_cmake,
        "target_link_libraries",
        test_target_name,
        "the closure unit-test links",
    )
    compact_links = compact_whitespace(links)
    require(
        re.fullmatch(
            r"target_link_libraries\s*\(\s*"
            + test_target_name
            + r"\s+PRIVATE\s+"
            + re.escape(public_target)
            + r"\s*\)",
            compact_links,
            re.I,
        )
        is not None,
        "the closure unit test must link only through the closure public target",
    )

    cpu_target_blocks = [
        block
        for block in cmake_blocks(unit_cmake, "set")
        if "_morsehgp3d_cpu_test_targets" in block
    ] + [
        block
        for block in cmake_blocks(unit_cmake, "list")
        if "_morsehgp3d_cpu_test_targets" in block
    ]
    require(
        sum(test_target_name in block for block in cpu_target_blocks) == 1,
        "the closure test must enter the sanitizer-applied CPU target list once",
    )

    functional_test = unique_cmake_block(
        unit_cmake,
        "add_test",
        "morsehgp3d.hierarchy_direct_sparse_facet_descent_closure",
        "the closure functional CTest",
    )
    require(
        test_target_name in functional_test,
        "the closure functional CTest must run its dedicated unit executable",
    )

    configuration_test = unique_cmake_block(
        unit_cmake,
        "add_test",
        "morsehgp3d.phase10_direct_sparse_facet_descent_closure_configuration",
        "the closure configuration CTest",
    )
    require(
        "_morsehgp3d_phase10_direct_sparse_facet_descent_closure_"
        "configuration_command" in configuration_test,
        "the closure configuration CTest must use its assembled command",
    )

    require_all(
        unit_cmake,
        (
            "check_phase10_direct_sparse_facet_descent_closure.py",
            "$<TARGET_FILE:morsehgp3d_direct_sparse_facet_descent_closure>",
            "--binary",
            "--nm",
            "CMAKE_NM",
        ),
        "the closure static and symbolic checker wiring",
    )


def validate_public_contract(header: str) -> None:
    compact = compact_whitespace(header)
    require_all(
        compact,
        (
            'direct_sparse_facet_descent_closure_backend = "reference_cpu"',
            'direct_sparse_facet_descent_closure_profile = "hgp_reduced"',
            'direct_sparse_facet_descent_closure_mode = "certified"',
            "direct_sparse_facet_descent_closure_refinement_status = "
            '"partial_refinement"',
            'direct_sparse_facet_descent_closure_public_status = "not_claimed"',
        ),
        "the Phase-10.5c status axes",
    )

    require_all(
        header,
        (
            "ExactDirectSparseFacetDescentClosureSeed",
            "ExactDirectSparseFacetDescentClosureConfig",
            "ExactDirectSparseFacetDescentClosureBudget",
            "ExactDirectSparseFacetDescentClosureDisposition",
            "ExactDirectSparseFacetDescentClosureDecision",
            "ExactDirectSparseFacetDescentNodeKind",
            "ExactDirectSparseFacetDescentClosureScope",
            "ExactDirectSparseFacetDescentClosureCounters",
            "ExactDirectSparseFacetDescentNode",
            "ExactDirectSparseFacetDescentEdge",
            "ExactDirectSparseFacetDescentSeedProjection",
            "ExactDirectSparseFacetDescentContradictionWitness",
            "ExactDirectSparseFacetDescentClosureResult",
            "ExactDirectSparseFacetDescentClosureVerification",
            "build_exact_direct_sparse_facet_descent_closure(",
            "verify_exact_direct_sparse_facet_descent_closure(",
        ),
        "the Phase-10.5c public types and entry points",
    )

    require_all(
        header,
        (
            "maximum_seed_count",
            "maximum_node_count",
            "maximum_step_call_count",
            "maximum_memo_slot_count",
            "ExactDirectSparseFacetDescentStepBudget step_budget",
        ),
        "the bounded closure budget",
    )

    require(
        "ExactDirectSparsePositiveFacetLocatorSnapshotStamp "
        "locator_snapshot_stamp" in compact,
        "the result must retain the immutable locator snapshot stamp",
    )

    require_all(
        header,
        (
            "memo_fingerprint_mask",
            "ExactDirectSparseFacetKey facet_key",
            "node_index",
            "outgoing_edge_index",
            "source_node_index",
            "target_node_index",
            "terminal_node_index",
            "strict_step_witness",
            "resolved_component_handle",
            "resolved_binding_witness",
        ),
        "the flat functional-forest records",
    )

    require_all(
        header,
        (
            "input_seed_reference_count",
            "distinct_seed_key_count",
            "interned_node_count",
            "evaluated_step_source_count",
            "strict_edge_count",
            "terminal_node_count",
            "memoized_suffix_reuse_count",
            "memo_full_key_comparison_count",
            "equal_fingerprint_distinct_key_count",
            "locator_snapshot_check_count",
            "distinct_cached_miniball_count",
            "source_miniball_build_count",
            "source_miniball_reuse_count",
            "successor_miniball_build_count",
            "successor_miniball_reuse_count",
            "every_memo_fingerprint_candidate_compared_by_full_key",
            "every_distinct_evaluated_key_called_step_core_once",
            "cached_miniballs_reused_at_exact_seams",
            "strict_functional_graph_certified",
            "exact_level_acyclicity_certified",
            "edge_node_terminal_identity_certified",
            "common_locator_snapshot_certified",
            "no_half_edge_published",
            "no_top_k_partition_or_shell_persisted",
            "locator_state_mutated",
            "locator_batch_committed",
            "singleton_component_created",
            "missing_facet_means_isolated",
            "hierarchy_attachment_published",
            "forbidden_global_structure_materialized",
            "public_status_claimed",
            "partial_refinement_only",
            "observed_storage_within_budget",
        ),
        "the closure counters and non-promotion facts",
    )

    require(
        re.search(
            r"std\s*::\s*vector\s*<\s*"
            r"ExactDirectSparseFacetDescentNode\s*>",
            header,
        )
        is not None,
        "the closure nodes must use one flat vector",
    )
    require(
        re.search(
            r"std\s*::\s*vector\s*<\s*"
            r"ExactDirectSparseFacetDescentEdge\s*>",
            header,
        )
        is not None,
        "the closure edges must use one flat vector",
    )
    require(
        re.search(
            r"std\s*::\s*vector\s*<\s*"
            r"ExactDirectSparseFacetDescentSeedProjection\s*>",
            header,
        )
        is not None,
        "the seed-to-terminal projection must use one flat vector",
    )
    require(
        "const ExactDirectSparsePositiveFacetLocator&" in compact,
        "the closure API must receive the locator through one const snapshot",
    )

    require(
        "ExactFacetMiniballResult" not in header,
        "a transient cached miniball must not escape through the public closure ABI",
    )
    require(
        "direct_sparse_facet_descent_step_detail" not in header,
        "the private prepared-step kernel must not leak into the installed header",
    )


def validate_private_kernel(
    project: Path,
    private_header: str,
    step_source: str,
    closure_source: str,
) -> None:
    compact_closure = compact_whitespace(
        strip_cpp_comments_and_literals(closure_source)
    )
    private_path = (
        project
        / "src/cpu/hierarchy/direct_sparse_facet_descent_step_detail.hpp"
    )
    require(
        private_path.is_file(),
        "10.5c requires the non-installed prepared-step private header "
        "src/cpu/hierarchy/direct_sparse_facet_descent_step_detail.hpp",
    )
    require(
        not (
            project
            / "include/morsehgp3d/hierarchy/"
            "direct_sparse_facet_descent_step_detail.hpp"
        ).exists(),
        "the prepared-step kernel header must not be installed",
    )

    require_all(
        private_header,
        (
            "namespace morsehgp3d::hierarchy::detail",
            "ExactDirectSparseCertifiedFacetMiniballLookup",
            "ExactFacetMiniballResult",
            "ExactDirectSparseFacetDescentStepTransient",
            "newly_built_source_miniball",
            "newly_built_successor_miniball",
            "certified_source_miniball",
            "certified_miniball_lookup",
            "build_exact_direct_sparse_facet_descent_step_transient(",
        ),
        "the private transient prepared-miniball seam",
    )
    require(
        "#include \"direct_sparse_facet_descent_step_detail.hpp\""
        in closure_source,
        "the closure source must use the private prepared-step seam",
    )
    require(
        "#include \"direct_sparse_facet_descent_step_detail.hpp\""
        in step_source,
        "the public 10.5b implementation must own the shared private kernel",
    )
    require(
        "build_exact_direct_sparse_facet_descent_step_transient("
        in closure_source,
        "the closure must advance through the prepared-source kernel",
    )
    require(
        "build_exact_direct_sparse_facet_descent_step_transient("
        in step_source,
        "the 10.5b target must define or invoke the shared prepared-source kernel",
    )
    require_all(
        compact_closure,
        (
            "detail::ExactDirectSparseCertifiedFacetMiniballLookup",
            "detail::ExactDirectSparseFacetDescentStepTransient",
            "std::vector<std::optional<ExactFacetMiniballResult>>",
            "cached_miniballs_",
            "std::vector<SuccessorMiniballSlot>",
            "successor_miniball_slots_",
            "std::vector<CachedSuccessorMiniball>",
            "successor_miniballs_",
            "cache_new_successor_miniball",
            "prepared_source",
            "find_cached_miniball_callback",
            "source_miniball_reuse_count",
            "successor_miniball_reuse_count",
            "newly_built_source_miniball",
            "newly_built_successor_miniball",
        ),
        "the full-key transient source/successor miniball cache",
    )
    require_any(
        compact_closure,
        (
            "cached.facet_key == key",
            "key == cached.facet_key",
            "successor_miniballs_[slot.cache_index].facet_key == key",
            "key == successor_miniballs_[slot.cache_index].facet_key",
        ),
        "the authoritative full-key successor-miniball lookup",
    )
    require(
        "build_exact_direct_sparse_facet_descent_step(" not in closure_source,
        "looping over the public 10.5b wrapper would rebuild certified seam miniballs",
    )
    require(
        "build_exact_facet_miniball(" not in closure_source,
        "the closure must not bypass the shared step kernel for local miniballs",
    )
    require(
        "lbvh_top_k_budgeted(" not in closure_source,
        "the closure must not duplicate the 10.5b top-k implementation",
    )


def validate_implementation(source: str) -> None:
    code = strip_cpp_comments_and_literals(source)
    compact = compact_whitespace(code)
    require_all(
        compact,
        (
            "memo_fingerprint_mask",
            "memo_full_key_comparison_count",
            "equal_fingerprint_distinct_key_count",
            "::visiting",
            "::closed",
            "memoized_suffix_reuse_count",
            "successor_facet_squared_level <",
            "source_facet_squared_level",
            "every_memo_fingerprint_candidate_compared_by_full_key = true",
            "every_distinct_evaluated_key_called_step_core_once = true",
            "cached_miniballs_reused_at_exact_seams = true",
            "strict_functional_graph_certified =",
            "exact_level_acyclicity_certified =",
            "edge_node_terminal_identity_certified = true",
            "no_half_edge_published = true",
            "no_top_k_partition_or_shell_persisted = true",
            "missing_facet_means_isolated = false",
            "singleton_component_created = false",
            "hierarchy_attachment_published = false",
            "forbidden_global_structure_materialized = false",
            "public_status_claimed = false",
            "partial_refinement_only = true",
            "verification.fresh_replay_certified = observed == expected",
            "verification.observed_storage_within_budget =",
        ),
        "the flat exact functional-forest implementation",
    )
    require_any(
        compact,
        (
            "slot.fingerprint == key_fingerprint",
            "key_fingerprint == slot.fingerprint",
        ),
        "the fingerprint-prefiltered memo lookup",
    )
    require_any(
        compact,
        (
            "slot.key == key",
            "key == slot.key",
            "result_.nodes[slot.node_index].facet_key == key",
            "key == result_.nodes[slot.node_index].facet_key",
            "complete_facet_keys_match(",
            "same_complete_facet_key(",
            "facet_keys_match(",
        ),
        "the authoritative complete-key memo comparison",
    )
    require_any(
        compact,
        ("::empty", "::unseen", "::vacant", "!slot.occupied"),
        "the empty memo-slot state",
    )

    require_any(
        compact,
        (
            "contradiction_cycle_or_incompatible_shared_target",
            "contradiction_cycle_in_strict_descent_forest",
            "contradiction_strict_descent_cycle",
        ),
        "the fail-closed visiting-node cycle decision",
    )
    require_any(
        compact,
        (
            "budget_exhausted",
            "no_resolution_closure_budget_exhausted",
        ),
        "the fail-closed structural budget frontier",
    )

    subtractive_edge_identity_patterns = (
        re.compile(
            r"strict_edge_count\s*==\s*"
            r"interned_node_count\s*-\s*terminal_node_count"
        ),
        re.compile(
            r"strict_edge_count\s*==\s*"
            r"(?:result\.)?nodes\.size\(\)\s*-\s*terminal_node_count"
        ),
        re.compile(
            r"(?:result\.)?edges\.size\(\)\s*==\s*"
            r"(?:result\.)?nodes\.size\(\)\s*-\s*terminal_node_count"
        ),
    )
    additive_edge_identity_patterns = (
        re.compile(
            r"(?:result\.)?edges\.size\(\)\s*\+\s*"
            r"(?:result\.(?:counters\.)?)?terminal_node_count\s*==\s*"
            r"(?:result\.)?nodes\.size\(\)"
        ),
        re.compile(
            r"strict_edge_count\s*\+\s*terminal_node_count\s*==\s*"
            r"interned_node_count"
        ),
    )
    has_subtractive_identity = any(
        pattern.search(compact) for pattern in subtractive_edge_identity_patterns
    )
    has_additive_identity = any(
        pattern.search(compact) for pattern in additive_edge_identity_patterns
    )
    has_checked_additive_identity = (
        re.search(
            r"try_add_size\(result\.edges\.size\(\),\s*terminal_count,\s*"
            r"graph_cardinality\)",
            compact,
        )
        is not None
        and re.search(
            r"graph_cardinality\s*==\s*result\.nodes\.size\(\)",
            compact,
        )
        is not None
    )
    require(
        has_subtractive_identity
        or has_additive_identity
        or has_checked_additive_identity,
        "the closure must check the exact functional-forest identity E = V - T",
    )
    if has_subtractive_identity:
        require(
            re.search(
                r"terminal_node_count\s*<=\s*(?:interned_node_count|"
                r"(?:result\.)?nodes\.size\(\))",
                compact,
            )
            is not None,
            "a subtractive E = V - T check must guard against underflow",
        )

    require_all(
        compact,
        (
            "locator.snapshot_stamp()",
            "locator_snapshot_stamp",
            "common_locator_snapshot_certified",
        ),
        "the same-const-locator snapshot seam",
    )
    require(
        compact.count("locator.snapshot_stamp()") >= 2,
        "the locator snapshot must be checked again after its entry capture",
    )
    require(
        compact.count(
            "result.counters.locator_snapshot_check_count = 1U"
        )
        == 1,
        "the locator snapshot counter must be initialized exactly once",
    )
    require(
        "locator_.snapshot_stamp() != result_.locator_snapshot_stamp" in compact,
        "every transient step must be followed by a common-snapshot check",
    )
    entry_stamp = compact.find(
        "result.locator_snapshot_stamp = locator.snapshot_stamp()"
    )
    builder_creation = compact.find("ClosureBuilder builder(", entry_stamp)
    require(
        0 <= entry_stamp < builder_creation,
        "the locator snapshot stamp must be captured before the closure builder "
        "can run geometry",
    )

    observed_storage_guard = compact.find(
        "verification.observed_storage_within_budget ="
    )
    observed_storage_return = compact.find(
        "if (!verification.observed_storage_within_budget)",
        observed_storage_guard,
    )
    expected_replay = compact.find(
        "const ExactDirectSparseFacetDescentClosureResult expected =",
        observed_storage_guard,
    )
    require(
        0 <= observed_storage_guard < observed_storage_return < expected_replay,
        "the verifier must reject over-budget observed storage before replay or "
        "graph traversal",
    )
    require_all(
        compact[observed_storage_guard:observed_storage_return],
        (
            "observed.nodes.size() <= budget.maximum_node_count",
            "observed.edges.size() <= budget.maximum_node_count",
            "observed.seed_projections.size() <= budget.maximum_seed_count",
        ),
        "the verifier observed-storage budget gate",
    )

    require(
        re.search(r"2\s*\*\s*[^;]+maximum_node_count[^;]*\+\s*1", compact)
        is not None
        or re.search(
            r"maximum_node_count\s*\*\s*2(?:U)?\s*\+\s*1(?:U)?",
            compact,
        )
        is not None
        or "checked_memo_slot_capacity(" in compact
        or "checked_memo_slot_count(" in compact
        or "checked_twice_plus_one(" in compact
        or "required_memo_slot_count(" in compact,
        "the flat memo table must prove its 2V+1 capacity without overflow",
    )
    require_all(
        compact,
        (
            "std::numeric_limits<std::size_t>::max()",
            "maximum_node_count",
            "/ 2U",
        ),
        "the 2V+1 memo-capacity overflow guard",
    )
    require(
        "std::sort(" in compact,
        "source keys must be canonically ordered before deterministic traversal",
    )

    for counter_name in (
        "source_miniball_build_count",
        "source_miniball_reuse_count",
        "successor_miniball_build_count",
        "successor_miniball_reuse_count",
    ):
        require(
            re.search(
                rf"result\.counters\.{counter_name}\s*==\s*"
                rf"result\.counters\.aggregate_step_counters\s*\.\s*"
                rf"{counter_name}",
                code,
            )
            is not None,
            "the closure must reconcile its top-level miniball counters with "
            f"the aggregate step counter {counter_name!r}",
        )
    require(
        re.search(
            r"cached_miniball_build_count\s*=\s*checked_size_sum\s*\(\s*"
            r"result\.counters\.source_miniball_build_count\s*,\s*"
            r"result\.counters\.successor_miniball_build_count\s*\)\s*;",
            code,
        )
        is not None
        and re.search(
            r"cached_miniball_build_count\s*==\s*"
            r"result\.counters\.distinct_cached_miniball_count",
            code,
        )
        is not None,
        "the exact cache seam must safely equate distinct cached miniballs "
        "with source-plus-successor builds",
    )


def validate_forbidden_paths(
    cmake: str,
    unit_cmake: str,
    header: str,
    private_header: str,
    source: str,
) -> None:
    cmake_links = unique_cmake_target_block(
        cmake,
        "target_link_libraries",
        "morsehgp3d_direct_sparse_facet_descent_closure",
        "the closure link closure",
    ).lower()
    for forbidden_dependency in (
        "morsehgp3d::hierarchy",
        "morsehgp3d::spatial",
        "morsehgp3d::facet_miniball",
        "morsehgp3d::direct_sparse_positive_facet_locator",
        "morsehgp3d::higher_support",
        "morsehgp3d::critical_catalog",
        "morsehgp3d::critical_arm",
        "morsehgp3d::direct_closed_saddle_incidence",
        "morsehgp3d::direct_frozen_root_quotient",
    ):
        require(
            forbidden_dependency not in cmake_links,
            f"the closure links forbidden dependency {forbidden_dependency!r}",
        )

    test_links = unique_cmake_target_block(
        unit_cmake,
        "target_link_libraries",
        "morsehgp3d_hierarchy_direct_sparse_facet_descent_closure_tests",
        "the closure unit-test links",
    ).lower()
    for forbidden_dependency in (
        "morsehgp3d::hierarchy",
        "morsehgp3d::spatial",
        "morsehgp3d::facet_miniball",
        "morsehgp3d::direct_sparse_facet_descent_step",
        "morsehgp3d::direct_sparse_positive_facet_locator",
    ):
        require(
            forbidden_dependency not in test_links,
            f"the closure test bypasses its public target through "
            f"{forbidden_dependency!r}",
        )

    lowered = strip_cpp_comments_and_literals(
        header + "\n" + private_header + "\n" + source
    ).lower()
    forbidden_patterns = (
        (r"\bstd\s*::\s*map\s*<", "std::map"),
        (r"\bstd\s*::\s*unordered_map\s*<", "std::unordered_map"),
        (r"\bstd\s*::\s*set\s*<", "std::set"),
        (r"\bstd\s*::\s*unordered_set\s*<", "std::unordered_set"),
        (
            r"\bstd\s*::\s*vector\s*<\s*std\s*::\s*vector\s*<",
            "nested std::vector",
        ),
        (r"\bbuild_exact_facet_descent_chain\s*\(", "historical descent chain"),
        (r"\bverify_exact_facet_descent_chain\s*\(", "historical chain verifier"),
        (r"\bexactfacetdescentchainresult\b", "historical chain result"),
        (r"\bbuild_exact_gamma\s*\(", "Gamma builder"),
        (r"\bmorse_gamma_partition_sweep\b", "Gamma sweep"),
        (r"\bbuild_exact_critical_arm\s*\(", "critical-arm builder"),
        (r"\bcritical_catalog\b", "critical catalog"),
        (r"\bhigher_support\b", "higher-support path"),
        (r"\bclosedballpartition\b", "closed-ball partition"),
        (r"\btopkpartition\b", "persisted top-k partition"),
        (r"\bbrute_force_closed_ball\b", "brute-force closed ball"),
        (r"\blbvh_closed_ball\b", "LBVH closed ball"),
        (r"\bordinary_cell\b", "ordinary cell"),
        (r"\bpower_cell\b", "power cell"),
        (r"\bdelaunay\b", "Delaunay structure"),
        (r"\bcoface\b", "coface materialization"),
        (r"\bapply_batch\s*\(", "locator batch mutation"),
        (r"\brootbirth\b", "RootBirth publication"),
        (r"\batomicunionbatch\b", "AtomicUnionBatch publication"),
        (r"\bgatewayattach\b", "GatewayAttach publication"),
        (r"(?<![a-z0-9_])attachment(?![a-z0-9_])", "Attachment publication"),
        (r"\bexterior_point_ids\b", "persisted exterior ids"),
        (r"\bglobal_exterior_points\b", "global exterior points"),
        (r"\bbinomial\b", "binomial-size arena"),
    )
    for pattern, label in forbidden_patterns:
        require(
            re.search(pattern, lowered) is None,
            f"the isolated closure contains forbidden path {label!r}",
        )


def validate_test_contract(test: str) -> None:
    code = strip_cpp_comments_and_literals(test)
    compact = compact_whitespace(code)
    require_all(
        compact,
        (
            "build_exact_direct_sparse_facet_descent_closure(",
            "verify_exact_direct_sparse_facet_descent_closure(",
            "strict_edge_count",
            "interned_node_count",
            "terminal_node_count",
            "memo_full_key_comparison_count",
            "equal_fingerprint_distinct_key_count",
            "source_miniball_reuse_count",
            "successor_miniball_reuse_count",
            "every_memo_fingerprint_candidate_compared_by_full_key",
            "locator_snapshot_matches_observed_build",
            "snapshot_stamp()",
            "maximum_seed_count",
            "maximum_node_count",
            "maximum_step_call_count",
            "complete_unresolved_non_strict_canonical_successor",
            "complete_unresolved_source_is_canonical_top_k_choice",
            "bad_edge",
        ),
        "the short Phase-10.5c unit suite",
    )
    require_any(
        compact,
        ("memo_fingerprint_mask", "collision_config{0U}"),
        "the forced memo-fingerprint collision fixture",
    )
    require_any(
        compact,
        ("memoized_suffix_reuse_count", "memoized_seed_reuse_count"),
        "the memoized suffix/seed reuse assertion",
    )
    require_any(
        compact,
        ("shared_suffix", "converging_suffix", "convergent_suffix"),
        "the shared-suffix fixture",
    )
    require(
        "duplicate" in compact and "permut" in compact,
        "the unit suite must exercise duplicate and permuted seed records",
    )
    require_any(
        compact,
        ("budget_boundar", "budget_exhaust", "one_below"),
        "the structural-budget boundary fixtures",
    )
    require_any(
        compact,
        ("fresh_verifier_rejects", "fresh_replay", "mutated"),
        "the fresh-verifier mutation fixtures",
    )
    require_any(
        compact,
        ("bad_node", "bad_terminal", "bad_projection"),
        "the forged node/projection fixture",
    )
    require_any(
        compact,
        ("bad_snapshot", "stale_snapshot", "snapshot_rejection"),
        "the stale locator-snapshot fixture",
    )
    require(
        re.search(
            r"strict_edge_count\s*==\s*"
            r"[^;]+interned_node_count\s*-\s*[^;]+terminal_node_count",
            compact,
        )
        is not None
        or re.search(
            r"edges\.size\(\)\s*\+\s*terminal_count\s*==\s*"
            r"result\.nodes\.size\(\)",
            compact,
        )
        is not None
        or "edge_node_terminal_identity_certified" in compact,
        "the unit suite must exercise E = V - T",
    )
    require(
        "build_exact_direct_sparse_facet_descent_step(" not in compact,
        "the closure unit test must not bypass 10.5c through the public step API",
    )
    require(
        "direct_sparse_facet_descent_step_detail" not in compact,
        "the closure unit test must not include the private transient seam",
    )


def validate_symbols(binary: Path, nm: Path) -> None:
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
            "build_exact_direct_sparse_facet_descent_closure",
            "verify_exact_direct_sparse_facet_descent_closure",
            "build_exact_direct_sparse_facet_descent_step_transient",
        ),
        "the isolated closure archive symbol contract",
    )
    for forbidden in (
        "build_exact_direct_sparse_facet_descent_step(",
        "build_exact_facet_descent_chain",
        "verify_exact_facet_descent_chain",
        "build_exact_gamma",
        "morse_gamma_partition_sweep",
        "critical_catalog",
        "critical_arm",
        "higher_support",
        "closedballpartition",
        "brute_force_closed_ball",
        "lbvh_closed_ball",
        "ordinary_cell",
        "power_cell",
        "delaunay",
        "coface",
        "apply_batch",
        "rootbirth",
        "atomicunionbatch",
        "gatewayattach",
        "attachment",
    ):
        require(
            forbidden not in symbols,
            f"the isolated closure archive exposes forbidden symbol {forbidden!r}",
        )


def validate(project: Path, binary: Path | None, nm: Path | None) -> None:
    cmake = read_text(project / "CMakeLists.txt")
    unit_cmake = read_text(project / "tests/unit/CMakeLists.txt")
    header = read_text(
        project
        / "include/morsehgp3d/hierarchy/"
        "direct_sparse_facet_descent_closure.hpp"
    )
    source = read_text(
        project
        / "src/cpu/hierarchy/direct_sparse_facet_descent_closure.cpp"
    )
    private_header = read_text(
        project
        / "src/cpu/hierarchy/direct_sparse_facet_descent_step_detail.hpp"
    )
    step_source = read_text(
        project
        / "src/cpu/hierarchy/direct_sparse_facet_descent_step.cpp"
    )
    test = read_text(
        project
        / "tests/unit/test_hierarchy_direct_sparse_facet_descent_closure.cpp"
    )

    validate_main_cmake(cmake)
    validate_unit_cmake(unit_cmake)
    validate_public_contract(header)
    validate_private_kernel(project, private_header, step_source, source)
    validate_implementation(source)
    validate_forbidden_paths(cmake, unit_cmake, header, private_header, source)
    validate_test_contract(test)

    require(
        (binary is None) == (nm is None),
        "--binary and --nm must be supplied together",
    )
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
        print(f"Phase-10.5c direct sparse facet descent closure failed: {error}")
        return 1
    print("Phase-10.5c direct sparse facet descent closure contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
