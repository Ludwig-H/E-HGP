#!/usr/bin/env python3
"""Validate the isolated Phase-10.10-RCPU positive-facet prefix sweep."""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


class ContractError(ValueError):
    """A Phase-10.10 positive-facet prefix-sweep invariant was violated."""


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
        f"expected exactly one {command} block for {target}, found {len(matches)}",
    )
    return matches[0]


def unique_marked_block(text: str, command: str, marker: str, context: str) -> str:
    matches = [block for block in cmake_blocks(text, command) if marker in block]
    require(len(matches) == 1, f"expected one {context}, found {len(matches)}")
    return matches[0]


def enclosing_cmake_branches(text: str, byte_index: int) -> list[tuple[str, str]]:
    """Return active CMake branch kinds and conditions at one position."""

    require(0 <= byte_index <= len(text), "invalid CMake source position")
    stack: list[tuple[str, str]] = []
    control = re.compile(r"(?im)^\s*(if|elseif|else|endif)\s*\(([^)]*)\)")
    for match in control.finditer(text, 0, byte_index):
        command = match.group(1).lower()
        condition = compact_whitespace(match.group(2))
        if command == "if":
            stack.append(("if", condition))
        elif command == "elseif":
            require(stack, "unbalanced CMake elseif before checker wiring")
            stack[-1] = ("elseif", condition)
        elif command == "else":
            require(stack, "unbalanced CMake else before checker wiring")
            require(
                condition == "",
                "a CMake else before checker wiring must not have arguments",
            )
            stack[-1] = ("else", "")
        else:
            require(stack, "unbalanced CMake endif before checker wiring")
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
    require(len(starts) == 1, f"expected one {context}, found {len(starts)}")
    opening = text.find("{", starts[0] + len(marker))
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


def validate_main_cmake(cmake: str) -> None:
    target = "morsehgp3d_direct_sparse_positive_facet_prefix_sweep"
    alias = "morsehgp3d::direct_sparse_positive_facet_prefix_sweep"

    target_block = unique_target_block(cmake, "add_library", target)
    require("STATIC" in target_block, "10.10 must remain a static archive")
    require(
        re.findall(r"[A-Za-z0-9_./+-]+\.cpp", target_block)
        == ["src/cpu/hierarchy/direct_sparse_positive_facet_prefix_sweep.cpp"],
        "the 10.10 archive must contain exactly its dedicated source",
    )

    alias_block = unique_target_block(cmake, "add_library", alias)
    require(
        "ALIAS" in alias_block and target in alias_block,
        "the 10.10 public alias must name the isolated archive",
    )

    properties = compact_whitespace(
        unique_target_block(cmake, "set_target_properties", target)
    )
    require_all(
        properties,
        (
            "EXPORT_NAME direct_sparse_positive_facet_prefix_sweep",
            "POSITION_INDEPENDENT_CODE ON",
        ),
        "the 10.10 export properties",
    )

    features = compact_whitespace(
        unique_target_block(cmake, "target_compile_features", target)
    )
    require(
        "PUBLIC cxx_std_20" in features,
        "the 10.10 archive must publish its C++20 requirement",
    )
    includes = unique_target_block(cmake, "target_include_directories", target)
    require_all(
        includes,
        ("BUILD_INTERFACE", "INSTALL_INTERFACE"),
        "the 10.10 include routing",
    )

    links = compact_whitespace(
        unique_target_block(cmake, "target_link_libraries", target)
    )
    require(
        re.fullmatch(
            r"target_link_libraries\s*\(\s*" + target + r"\s+PUBLIC\s+"
            r"morsehgp3d::direct_sparse_positive_facet_locator\s+"
            r"PRIVATE\s+morsehgp3d::sanitizers\s*\)",
            links,
            re.I,
        )
        is not None,
        "the archive must link only the locator publicly and sanitizers privately",
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
        "the 10.10 archive must be installed exactly once in the public export",
    )


def validate_test_wiring(unit_cmake: str) -> None:
    test_target = "morsehgp3d_hierarchy_direct_sparse_positive_facet_prefix_sweep_tests"
    public_target = "morsehgp3d::direct_sparse_positive_facet_prefix_sweep"

    executable = unique_target_block(unit_cmake, "add_executable", test_target)
    require(
        re.findall(r"[A-Za-z0-9_./+-]+\.cpp", executable)
        == ["test_hierarchy_direct_sparse_positive_facet_prefix_sweep.cpp"],
        "the 10.10 test executable must contain exactly its dedicated source",
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
        "the 10.10 test must link only through the isolated public target",
    )

    cpu_targets = unique_target_block(unit_cmake, "set", "_morsehgp3d_cpu_test_targets")
    require(
        len(re.findall(rf"\b{re.escape(test_target)}\b", cpu_targets)) == 1,
        "the 10.10 test target must enter the sanitizer-applied CPU list once",
    )

    functional = unique_marked_block(
        unit_cmake,
        "add_test",
        "morsehgp3d.hierarchy_direct_sparse_positive_facet_prefix_sweep",
        "10.10 functional CTest",
    )
    require(
        re.fullmatch(
            r"add_test\s*\(\s*NAME\s+"
            r"morsehgp3d\.hierarchy_direct_sparse_positive_facet_prefix_sweep\s+"
            r"COMMAND\s+" + test_target + r"\s*\)",
            compact_whitespace(functional),
            re.I,
        )
        is not None,
        "the 10.10 functional CTest must run exactly its dedicated executable",
    )

    configuration_variable = (
        "_morsehgp3d_phase10_direct_sparse_positive_facet_prefix_sweep_"
        "configuration_command"
    )
    configuration_name = (
        "morsehgp3d.phase10_direct_sparse_positive_facet_prefix_sweep_" "configuration"
    )
    configuration_set = compact_whitespace(
        unique_target_block(unit_cmake, "set", configuration_variable)
    )
    require(
        re.fullmatch(
            r"set\s*\(\s*"
            + configuration_variable
            + r"\s+\"\$\{Python3_EXECUTABLE\}\"\s+"
            r"\"\$\{CMAKE_CURRENT_SOURCE_DIR\}/\.\./configuration/"
            r"check_phase10_direct_sparse_positive_facet_prefix_sweep\.py\"\s+"
            r"\"\$\{PROJECT_SOURCE_DIR\}\"\s*\)",
            configuration_set,
            re.I,
        )
        is not None,
        "the 10.10 checker command must contain only Python, checker and project",
    )
    configuration_append_block = unique_marked_block(
        unit_cmake,
        "list",
        configuration_variable,
        "10.10 nm command append",
    )
    configuration_append = compact_whitespace(configuration_append_block)
    require(
        re.fullmatch(
            r"list\s*\(\s*APPEND\s+"
            + configuration_variable
            + r"\s+--binary\s+\"\$<TARGET_FILE:"
            r"morsehgp3d_direct_sparse_positive_facet_prefix_sweep>\"\s+"
            r"--nm\s+\"\$\{CMAKE_NM\}\"\s*\)",
            configuration_append,
            re.I,
        )
        is not None,
        "the 10.10 nm append must bind this archive and CMAKE_NM exactly",
    )
    configuration_append_position = unit_cmake.find(configuration_append_block)
    require(
        configuration_append_position >= 0
        and enclosing_cmake_branches(unit_cmake, configuration_append_position)[-1:]
        == [("if", "CMAKE_NM AND NOT MSVC")],
        "the 10.10 nm append must be directly guarded by " "if(CMAKE_NM AND NOT MSVC)",
    )
    configuration = unique_marked_block(
        unit_cmake,
        "add_test",
        configuration_name,
        "10.10 configuration CTest",
    )
    require(
        re.fullmatch(
            r"add_test\s*\(\s*NAME\s+"
            + re.escape(configuration_name)
            + r"\s+COMMAND\s+\$\{"
            + configuration_variable
            + r"\}\s*\)",
            compact_whitespace(configuration),
            re.I,
        )
        is not None,
        "the 10.10 configuration CTest must execute exactly its assembled command",
    )


def validate_public_contract(header: str) -> None:
    compact = compact_whitespace(header)
    require_all(
        compact,
        (
            "direct_sparse_positive_facet_prefix_sweep_schema_version = 1U",
            'direct_sparse_positive_facet_prefix_sweep_backend = "reference_cpu"',
            'direct_sparse_positive_facet_prefix_sweep_profile = "hgp_reduced"',
            'direct_sparse_positive_facet_prefix_sweep_mode = "certified"',
            "direct_sparse_positive_facet_prefix_sweep_refinement_status = "
            '"partial_refinement"',
            "direct_sparse_positive_facet_prefix_sweep_public_status = "
            '"not_claimed"',
        ),
        "the Phase-10.10 status axes",
    )

    project_includes = re.findall(r'#include\s+"([^"]+)"', header)
    require(
        project_includes
        == ["morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"],
        "the public contract must include only the 10.5a locator project header",
    )

    require_all(
        header,
        (
            "maximum_query_count",
            "maximum_query_key_point_count",
            "maximum_component_handle_scratch_count",
            "maximum_batch_record_scan_count",
            "maximum_union_record_replay_count",
            "maximum_union_replay_parent_hop_count",
            "maximum_aggregate_slot_visit_count",
            "maximum_aggregate_query_parent_hop_count",
            "maximum_logical_output_entry_count",
            "ExactDirectSparsePositiveFacetProbeBudget facet_probe_budget",
            "committed_batch_prefix_count",
            "latent_unresolved",
            "relative_positive",
            "ExactDirectSparsePositiveFacetLocatorSnapshotStamp",
            "queries_canonical_and_prefix_monotone",
            "requested_locator_history_records_well_formed",
            "dense_identity_dsu_scratch_initialized",
            "each_batch_and_union_prefix_replayed_once",
            "future_binding_slots_are_historical_terminators",
            "every_fingerprint_candidate_compared_by_full_key",
            "every_query_resolved_once",
            "positive_and_latent_outcomes_separated",
            "every_positive_has_historical_root_and_original_witness",
            "every_latent_has_no_positive_payload",
            "common_frozen_locator_snapshot_certified",
            "no_partial_scientific_payload_published",
            "locator_state_mutated",
            "locator_batch_committed",
            "external_binding_authority_replayed",
            "source_batch_alignment_claimed",
            "missing_facet_means_isolated",
            "singleton_component_created",
            "quotient_root_union_or_forest_mutated",
            "gateway_attach_published",
            "gamma_cells_or_higher_order_delaunay_materialized",
            "forbidden_global_structure_materialized",
            "public_status_claimed",
            "partial_refinement_only",
            "locator_internal_committed_batch_prefixes_relative_to_frozen_"
            "positive_domain_only",
            "trusted_live_locator_and_witness_certified",
            "observed_storage_within_budget",
            "locator_snapshot_matches_observed_build",
            "locator_structural_verification",
            "locator_verification_budget_preflight_certified",
            "locator_verification_budget_respected",
            "locator_durable_structure_freshly_verified",
            "committed_slot_insertion_chronology_freshly_replayed",
            "observed_outcome_well_formed",
            "queries_and_prefixes_freshly_replayed",
            "union_prefixes_and_historical_roots_freshly_replayed",
            "historical_slot_probes_freshly_replayed",
            "counters_and_result_facts_freshly_replayed",
            "no_locator_mutation_or_batch_commit",
            "source_batch_alignment_replayed",
            "no_isolation_singleton_quotient_forest_or_attachment_invented",
            "no_forbidden_global_structure_materialized",
            "fresh_replay_certified",
            "result_certified",
            "build_exact_direct_sparse_positive_facet_prefix_sweep(",
            "verify_exact_direct_sparse_positive_facet_prefix_sweep(",
            "ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget",
        ),
        "the Phase-10.10 public budgets, facts and APIs",
    )

    for forbidden in (
        "std::vector<std::vector",
        "std::map<",
        "std::unordered_map",
        "#include <map>",
        "#include <unordered_map>",
        "ExactDirectSparseCofaceKey",
        "source_batch_index",
        "ExactDirectSparseGateway",
    ):
        require(
            forbidden not in header,
            f"the Phase-10.10 ABI contains forbidden shape {forbidden!r}",
        )


def validate_implementation(source: str) -> None:
    include_lines = "\n".join(
        line for line in source.splitlines() if line.lstrip().startswith("#include")
    )
    project_includes = re.findall(r'#include\s+"([^"]+)"', include_lines)
    require(
        project_includes
        == ["morsehgp3d/hierarchy/" "direct_sparse_positive_facet_prefix_sweep.hpp"],
        "the isolated 10.10 source must include only its own project header",
    )

    code = strip_cpp_comments_and_literals(source)
    compact = re.sub(r"\s*::\s*", "::", compact_whitespace(code))
    compact = re.sub(r"\s*\.\s*", ".", compact)
    require_all(
        compact,
        (
            "locator.certified_positive_locator()",
            "locator.snapshot_stamp()",
            "std::iota(",
            "parents.begin(), parents.end(), ExactDirectSparseComponentHandle{0U}",
            "query.committed_batch_prefix_count < maximum_prefix_count",
            "maximum_prefix_count = query.committed_batch_prefix_count",
            "while (replayed_batch_count < query.committed_batch_prefix_count)",
            "locator.committed_batches()[batch_index]",
            "locator.committed_batches()[replayed_batch_count]",
            "locator.committed_unions()[union_index]",
            "replayed_binding_prefix_count",
            "replayed_union_prefix_count",
            "result.each_batch_and_union_prefix_replayed_once =",
            "fingerprint_exact_direct_sparse_facet_key(",
            "verify_exact_direct_sparse_positive_facet_locator_structure(",
            "locator_verification_budget",
            "verification.locator_structural_verification =",
            "locator.config(), locator_verification_budget, " "locator.state_view()",
            "locator_verification.budget_preflight_certified",
            "!locator_verification.budget_exhausted",
            "locator_verification.result_certified",
            "locator_verification.committed_slot_insertion_chronology_"
            "freshly_replayed",
            "verification.committed_slot_insertion_chronology_" "freshly_replayed =",
            "result.source_batch_alignment_claimed = false",
            "verification.source_batch_alignment_replayed = false",
        ),
        "the Phase-10.10 monotone replay and fresh-verification closure",
    )

    require(
        compact.count("fingerprint_exact_direct_sparse_facet_key(") == 1,
        "the historical probe must use exactly one public fingerprint site",
    )
    require(
        compact.count("verify_exact_direct_sparse_positive_facet_locator_structure(")
        == 1,
        "the fresh verifier must invoke the reinforced 10.5a verifier once",
    )
    require_all(
        compact,
        (
            "resolution.component_handle >= "
            "result.required_component_handle_scratch_count",
            "source_witness_well_formed(",
            "resolution.source_binding_witness",
            "result.locator_query_witness.external_authority_id",
            "observed_relative_positive_count == "
            "result.counters.relative_positive_count",
            "observed_latent_unresolved_count == "
            "result.counters.latent_unresolved_count",
        ),
        "the self-contained positive payload and disposition recounts",
    )

    final_parent_arena_accesses = list(
        re.finditer(r"locator\.component_parents\s*\(\s*\)", compact)
    )
    require(
        len(final_parent_arena_accesses) == 2
        and all(
            re.match(
                r"\.size\s*\(\s*\)",
                compact[access.end() :],
            )
            is not None
            for access in final_parent_arena_accesses
        ),
        "the final locator parent arena may be read exactly twice and only "
        "through an immediate .size() capacity query",
    )

    require_all(
        compact,
        (
            "queries.size() > budget.maximum_query_count",
            "query_key_point_count > budget.maximum_query_key_point_count",
            "result.required_component_handle_scratch_count > "
            "budget.maximum_component_handle_scratch_count",
            "double_batch_scan_count > budget.maximum_batch_record_scan_count",
            "union_record_prefix_count > " "budget.maximum_union_record_replay_count",
            "budget.maximum_union_replay_parent_hop_count, "
            "result.counters.union_replay_parent_hop_count",
            "parent_hop_count >= maximum_parent_hop_count",
            "queries.size() > budget.maximum_logical_output_entry_count",
            "result.counters.slot_visit_count > "
            "budget.maximum_aggregate_slot_visit_count",
            "result.counters.query_parent_hop_count > "
            "budget.maximum_aggregate_query_parent_hop_count",
        ),
        "the Phase-10.10 build-time budget gates",
    )

    query_sweep = re.sub(
        r"\s*::\s*",
        "::",
        compact_whitespace(
            unique_cpp_braced_block(
                code,
                "for (const auto& query : queries)",
                "monotone query sweep",
            )
        ),
    )
    query_sweep = re.sub(r"\s*\.\s*", ".", query_sweep)
    slot_guard_position = query_sweep.find(
        "result.counters.slot_visit_count > "
        "budget.maximum_aggregate_slot_visit_count"
    )
    parent_guard_position = query_sweep.find(
        "result.counters.query_parent_hop_count > "
        "budget.maximum_aggregate_query_parent_hop_count"
    )
    slot_remainder_position = query_sweep.find(
        "budget.maximum_aggregate_slot_visit_count - "
        "result.counters.slot_visit_count"
    )
    parent_remainder_position = query_sweep.find(
        "budget.maximum_aggregate_query_parent_hop_count - "
        "result.counters.query_parent_hop_count"
    )
    probe_position = query_sweep.find("const PrefixProbeResult probe = probe_prefix(")
    require(
        0
        <= min(
            slot_guard_position,
            parent_guard_position,
            slot_remainder_position,
            parent_remainder_position,
            probe_position,
        )
        and max(slot_guard_position, parent_guard_position)
        < min(slot_remainder_position, parent_remainder_position)
        < probe_position,
        "aggregate counters must be guarded before either local-budget subtraction "
        "and before the historical probe",
    )

    prefix_probe = re.sub(
        r"\s*::\s*",
        "::",
        compact_whitespace(
            unique_cpp_braced_block(
                code,
                "[[nodiscard]] PrefixProbeResult probe_prefix(",
                "historical prefix probe",
            )
        ),
    )
    prefix_probe = re.sub(r"\s*\.\s*", ".", prefix_probe)
    require_all(
        prefix_probe,
        (
            "slot.committed_binding_index >= active_binding_prefix_count",
            "result.status = PrefixProbeStatus::latent_unresolved",
            "result.future_binding_terminator = slot.occupied",
            "slot.fingerprint == fingerprint",
            "complete_key_matches_arena(slot, key, key_point_arena)",
        ),
        "the exact historical terminator and full-key probe",
    )
    terminator_block = unique_cpp_braced_block(
        prefix_probe,
        "if (!slot.occupied ||",
        "historical empty-or-future terminator",
    )
    require_all(
        terminator_block,
        (
            "slot.committed_binding_index >= active_binding_prefix_count",
            "result.status = PrefixProbeStatus::latent_unresolved",
            "result.future_binding_terminator = slot.occupied",
            "return result",
        ),
        "the historical empty-or-future terminator",
    )
    terminator_position = prefix_probe.find(terminator_block)
    fingerprint_comparison_position = prefix_probe.find(
        "if (slot.fingerprint == fingerprint)"
    )
    full_key_comparison_position = prefix_probe.find(
        "complete_key_matches_arena(slot, key, key_point_arena)"
    )
    require(
        0
        <= terminator_position + len(terminator_block)
        < fingerprint_comparison_position
        < full_key_comparison_position,
        "the historical terminator must return before any fingerprint or full-key "
        "comparison",
    )

    require(
        re.search(
            r"std\s*::\s*min\s*\(\s*"
            r"budget\.facet_probe_budget\.maximum_slot_visit_count\s*,\s*"
            r"budget\.maximum_aggregate_slot_visit_count\s*-\s*"
            r"result\.counters\.slot_visit_count\s*\)",
            code,
        )
        is not None,
        "the local slot cap must be bounded by the aggregate slot remainder",
    )
    require(
        re.search(
            r"std\s*::\s*min\s*\(\s*budget\.facet_probe_budget\."
            r"maximum_component_parent_hop_count\s*,\s*budget\."
            r"maximum_aggregate_query_parent_hop_count\s*-\s*"
            r"result\.counters\.query_parent_hop_count\s*\)",
            code,
        )
        is not None,
        "the local parent cap must be bounded by the aggregate parent remainder",
    )
    require(
        "locator, effective_probe_budget" in compact,
        "the historical probe must receive the doubly bounded local budget",
    )

    for false_fact in (
        "locator_state_mutated",
        "locator_batch_committed",
        "external_binding_authority_replayed",
        "source_batch_alignment_claimed",
        "missing_facet_means_isolated",
        "singleton_component_created",
        "quotient_root_union_or_forest_mutated",
        "gateway_attach_published",
        "gamma_cells_or_higher_order_delaunay_materialized",
        "forbidden_global_structure_materialized",
        "public_status_claimed",
    ):
        require(
            f"result.{false_fact} = false" in compact,
            f"the result must explicitly keep {false_fact} false",
        )

    architecture_code = code
    for honest_negative_field in (
        "locator_state_mutated",
        "locator_batch_committed",
        "external_binding_authority_replayed",
        "source_batch_alignment_claimed",
        "source_batch_alignment_replayed",
        "missing_facet_means_isolated",
        "singleton_component_created",
        "quotient_root_union_or_forest_mutated",
        "gateway_attach_published",
        "gamma_cells_or_higher_order_delaunay_materialized",
        "forbidden_global_structure_materialized",
        "no_isolation_singleton_quotient_forest_or_attachment_invented",
        "no_forbidden_global_structure_materialized",
    ):
        architecture_code = architecture_code.replace(honest_negative_field, " ")

    forbidden_patterns = (
        (
            r"\bstd\s*::\s*(?:map|multimap|unordered_map|unordered_multimap)\s*<",
            "associative global index",
        ),
        (r"\bstd\s*::\s*vector\s*<\s*std\s*::\s*vector\s*<", "nested arena"),
        (r"\bapply_batch\s*\(", "locator mutation"),
        (r"\bprobe_positive_facet\s*\(", "current-state locator probe"),
        (
            r"\bbuild_exact_direct_sparse_positive_facet_locator\s*\(",
            "replacement locator",
        ),
        (r"\bsource_batch_index\b", "10.7 source-batch alignment"),
        (
            r"\blocator_snapshot_batch_level_alignment_claimed\b",
            "10.9 locator/source alignment",
        ),
        (
            r"\b(?:build|verify)_exact_direct_sparse_gateway_candidate",
            "10.7/10.9 gateway path",
        ),
        (r"\bExactDirectSparseGateway[A-Za-z0-9_]*\b", "gateway durable type"),
        (r"\bExactDirectSparseCofaceKey\b", "coface key"),
        (
            r"\b(?:build|verify|compute)[A-Za-z0-9_]*(?:gamma|delaunay|"
            r"cell_atlas|critical_catalog)\s*\(",
            "global geometry builder",
        ),
        (r"\b(?:ordinary|power)_?cell\b", "global cell"),
        (r"\b(?:facet|coface)_?catalog\b", "global facet/coface catalog"),
        (r"\bbuild_exact_direct_frozen_root_quotient\b", "quotient builder"),
        (r"\bbuild_exact_direct_k1_forest[A-Za-z0-9_]*\b", "forest builder"),
        (
            r"\b(?:GatewayAttach|gateway_attach|RootBirth|root_birth|"
            r"AtomicUnionBatch|atomic_union_batch)\b",
            "root, union or attachment action",
        ),
        (
            r"\b(?:static|thread_local)\s+(?!constexpr\b)"
            r"std\s*::\s*(?:vector|map|unordered_map)\s*<",
            "mutable namespace-level container",
        ),
    )
    for pattern, label in forbidden_patterns:
        require(
            re.search(pattern, architecture_code, re.I) is None,
            f"the isolated 10.10 implementation contains forbidden {label}",
        )

    require(
        re.search(r"source_batch_alignment_claimed\s*=\s*true", code) is None
        and re.search(r"source_batch_alignment_replayed\s*=\s*true", code) is None,
        "10.10 must never claim 10.7/10.9 temporal alignment",
    )


def validate_symbols(binary: Path, nm: Path) -> None:
    require(binary.is_file(), f"the isolated 10.10 archive does not exist: {binary}")
    require(nm.is_file(), f"the nm executable does not exist: {nm}")
    try:
        symbols = subprocess.run(
            [str(nm), "-C", str(binary)],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.lower()
    except (OSError, subprocess.CalledProcessError) as error:
        raise ContractError(f"cannot inspect the 10.10 archive: {error}") from error

    require_all(
        symbols,
        (
            "build_exact_direct_sparse_positive_facet_prefix_sweep",
            "verify_exact_direct_sparse_positive_facet_prefix_sweep",
            "exactdirectsparsepositivefacetprefixsweepresult::"
            "certified_partial_refinement",
            "exactdirectsparsepositivefacetprefixsweepresult::"
            "certified_atomic_failure",
            "exactdirectsparsepositivefacetprefixsweepresult::certified_outcome",
            "fingerprint_exact_direct_sparse_facet_key",
            "verify_exact_direct_sparse_positive_facet_locator_structure",
            "exactdirectsparsepositivefacetlocator::certified_positive_locator",
            "exactdirectsparsepositivefacetlocator::snapshot_stamp",
            "exactdirectsparsepositivefacetlocator::state_view",
        ),
        "the isolated 10.10 symbol closure",
    )

    for forbidden in (
        "exactdirectsparsepositivefacetlocator::apply_batch",
        "exactdirectsparsepositivefacetlocator::probe_positive_facet",
        "build_exact_direct_sparse_positive_facet_locator",
        "direct_sparse_gateway_candidate",
        "exactdirectsparsegateway",
        "exactdirectsparsecofacekey",
        "build_exact_facet_miniball",
        "lbvh_top_k_budgeted",
        "lbvh_closed_ball",
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
        project / "include/morsehgp3d/hierarchy/"
        "direct_sparse_positive_facet_prefix_sweep.hpp"
    )
    source = read_text(
        project / "src/cpu/hierarchy/direct_sparse_positive_facet_prefix_sweep.cpp"
    )

    validate_main_cmake(cmake)
    validate_test_wiring(unit_cmake)
    validate_public_contract(header)
    validate_implementation(source)
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
        print(f"Phase-10.10-RCPU positive-facet prefix-sweep contract failed: {error}")
        return 1
    print("Phase-10.10-RCPU positive-facet prefix-sweep contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
