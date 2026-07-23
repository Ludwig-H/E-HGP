#!/usr/bin/env python3
"""Validate the isolated Phase-10 sparse positive facet locator target."""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


class ContractError(ValueError):
    """A sparse positive facet locator invariant was violated."""


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


def strip_cpp_comments_and_literals(text: str) -> str:
    pattern = re.compile(
        r"//[^\n]*|/\*.*?\*/|R\"([^ ()\\\t\r\n]*)\(.*?\)\1\"|"
        r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        re.S,
    )
    return pattern.sub(" ", text)


def braced_block(text: str, marker: str, context: str) -> str:
    require(text.count(marker) == 1, f"expected one {context} marker")
    start = text.find(marker)
    opening = text.find("{", start + len(marker))
    require(opening >= 0, f"the {context} has no opening brace")
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    raise ContractError(f"unterminated {context}")


def validate(project: Path, binary: Path | None, nm: Path | None) -> None:
    cmake = read_text(project / "CMakeLists.txt")
    header = read_text(
        project / "include/morsehgp3d/hierarchy/"
        "direct_sparse_positive_facet_locator.hpp"
    )
    source = read_text(
        project / "src/cpu/hierarchy/direct_sparse_positive_facet_locator.cpp"
    )
    unit_test = read_text(
        project / "tests/unit/test_hierarchy_direct_sparse_positive_facet_locator.cpp"
    )

    target = parenthesized_block(
        cmake,
        "add_library(\n  morsehgp3d_direct_sparse_positive_facet_locator",
    )
    require(
        target.count("src/cpu/hierarchy/direct_sparse_positive_facet_locator.cpp") == 1,
        "the target must contain exactly its isolated source",
    )
    links = parenthesized_block(
        cmake,
        "target_link_libraries(\n  " "morsehgp3d_direct_sparse_positive_facet_locator",
    )
    require(
        "PUBLIC morsehgp3d::contract" in links,
        "the locator public digest ABI must link the canonical contract",
    )
    require(
        "PRIVATE morsehgp3d::sanitizers" in links,
        "the isolated target must retain the sanitizer policy",
    )
    for forbidden_dependency in (
        "morsehgp3d::hierarchy",
        "morsehgp3d::spatial",
        "local_morse_geometry",
        "critical_catalog",
    ):
        require(
            forbidden_dependency not in links,
            f"the target links forbidden dependency {forbidden_dependency!r}",
        )

    for required in (
        "direct_sparse_positive_facet_maximum_point_count = 10U",
        "ExactDirectSparseFacetLookupDisposition",
        "unresolved",
        "positive_bindings_relative_to_caller_asserted_external_authority_only",
        "maximum_committed_binding_count",
        "maximum_committed_key_point_count",
        "maximum_committed_union_count",
        "maximum_committed_batch_count",
        "maximum_batch_scratch_slot_count",
        "fingerprint_mask",
        "fingerprint_exact_direct_sparse_facet_key(",
        "committed_binding_index",
        "ExactDirectSparseCommittedBatchRecord",
        "ExactDirectSparsePositiveFacetLocatorStateView",
        "ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget",
        "maximum_table_slot_count",
        "maximum_key_point_count",
        "maximum_component_parent_count",
        "maximum_union_record_count",
        "maximum_batch_record_count",
        "maximum_binding_scratch_entry_count",
        "maximum_key_point_scratch_entry_count",
        "maximum_table_slot_scratch_entry_count",
        "maximum_component_parent_scratch_entry_count",
        "maximum_temporary_scratch_byte_count",
        "maximum_fingerprint_search_slot_visit_count",
        "maximum_insertion_chronology_slot_visit_count",
        "maximum_union_parent_hop_count",
        "required_temporary_scratch_byte_count",
        "fingerprint_search_slot_visit_count",
        "insertion_chronology_slot_visit_count",
        "union_parent_hop_count",
        "scratch_requirement_arithmetic_certified",
        "budget_preflight_certified",
        "fingerprint_search_budget_exhausted",
        "insertion_chronology_budget_exhausted",
        "union_parent_hop_budget_exhausted",
        "budget_exhausted",
        "structure_contract_rejected",
        "ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision",
        "no_verification_budget_preflight_exhausted",
        "no_verification_fingerprint_search_budget_exhausted",
        "no_verification_insertion_chronology_budget_exhausted",
        "no_verification_union_parent_hop_budget_exhausted",
        "complete_certified_durable_structure_verification",
        "committed_history_digest",
        "committed_history_digest_freshly_replayed",
        "committed_slot_insertion_chronology_freshly_replayed",
        "external_authority_replayed_by_locator",
        "verify_exact_direct_sparse_positive_facet_locator_structure(",
    ):
        require(required in header, f"the public contract is missing {required!r}")
    require(
        re.search(
            r"verify_exact_direct_sparse_positive_facet_locator_structure\s*\("
            r"[^;]*trusted_config\s*,\s*const\s+"
            r"ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget&"
            r"[^;]*verification_budget\s*,\s*const\s+"
            r"ExactDirectSparsePositiveFacetLocatorStateView&\s+observed\s*\)",
            header,
            re.S,
        )
        is not None,
        "the explicit structural budget must precede the observed state view",
    )
    for forbidden_shape in (
        "std::vector<std::vector",
        "std::map<",
        "std::unordered_map",
        "#include <map>",
        "#include <unordered_map>",
    ):
        require(
            forbidden_shape not in header,
            f"the public ABI contains forbidden shape {forbidden_shape!r}",
        )

    for required in (
        "complete_key_matches_arena(",
        "complete_keys_match(",
        "slot.fingerprint == fingerprint",
        "fingerprint_exact_direct_sparse_facet_key(",
        "structural_verification_scratch_byte_count(",
        "checked_multiply(entry_count, entry_size)",
        "checked_add(total, *bytes)",
        "search_committed_slots(",
        "maximum_slot_visit_count",
        "result.slot_visit_budget_exhausted = true",
        "std::vector<std::uint8_t> replayed_slot_occupancy",
        "verification.budget_preflight_certified =",
        "verification.required_temporary_scratch_byte_count",
        "observed.counters.inserted_binding_count <=",
        "trusted_budget.maximum_committed_binding_count",
        "verification.fingerprint_search_slot_visit_count",
        "verification.insertion_chronology_slot_visit_count",
        "verification.union_parent_hop_count",
        "find_component_for_structure(",
        "unite_components_for_structure(",
        "verification.committed_slot_insertion_chronology_freshly_replayed",
        "std::vector<ExactDirectSparseComponentHandle> candidate_parents",
        "result.lookups_use_strict_pre_batch_snapshot = true",
        "result.current_batch_bindings_hidden_from_lookups = true",
        "contradiction_incompatible_exact_facet_binding",
        "first_empty_committed_slot(",
        "committed_batches_.push_back(",
        "locator_snapshot_initial_digest_domain",
        "locator_snapshot_chain_digest_domain",
        "initial_locator_snapshot_digest(",
        "LocatorSnapshotChainDigestBuilder",
        "committed_history_digest_ = next_snapshot_digest",
        "replayed_history_digest == observed.committed_history_digest",
        "verification.external_authority_replayed_by_locator = false",
        "if (!verification.budget_preflight_certified)",
        "no_verification_budget_preflight_exhausted",
        "no_verification_fingerprint_search_budget_exhausted",
        "no_verification_insertion_chronology_budget_exhausted",
        "no_verification_union_parent_hop_budget_exhausted",
        "reject_malformed_durable_history",
        "verification.fingerprint_search_slot_visit_count <=",
        "verification.insertion_chronology_slot_visit_count <=",
        "verification.union_parent_hop_count <=",
    ):
        require(required in source, f"the implementation is missing {required!r}")

    code = strip_cpp_comments_and_literals(source)
    primary_position = code.find(
        "verify_exact_direct_sparse_positive_facet_locator_structure("
    )
    verification_budget_position = code.find("verification_budget", primary_position)
    observed_position = code.find(
        "const ExactDirectSparsePositiveFacetLocatorStateView& observed",
        primary_position,
    )
    scratch_requirement_position = code.find(
        "const auto required_scratch_bytes", observed_position
    )
    preflight_assignment_position = code.find(
        "verification.budget_preflight_certified =", observed_position
    )
    preflight_rejection_position = code.find(
        "if (!verification.budget_preflight_certified)",
        preflight_assignment_position,
    )
    preflight_return_position = code.find(
        "return verification;", preflight_rejection_position
    )
    first_scratch_allocation_position = code.find("std::vector<", observed_position)
    first_variable_scan_position = code.find("for (", observed_position)
    require(
        0
        <= primary_position
        < verification_budget_position
        < observed_position
        < scratch_requirement_position
        < preflight_assignment_position
        < preflight_rejection_position
        < preflight_return_position
        < min(first_scratch_allocation_position, first_variable_scan_position),
        "the explicit budget and exact scratch preflight must precede every "
        "scratch allocation and variable scan",
    )

    batch_replay_position = code.find(
        "for (std::size_t index = 0U;",
        code.find("contract::CanonicalId replayed_history_digest", observed_position),
    )
    scalar_batch_guard_position = code.find(
        "if (record.committed_batch_index != index ||", batch_replay_position
    )
    scalar_batch_rejection_position = code.find(
        "reject_malformed_durable_history();", scalar_batch_guard_position
    )
    scalar_batch_return_position = code.find(
        "return verification;", scalar_batch_rejection_position
    )
    first_batch_binding_scan_position = code.find(
        "for (std::size_t binding_index = replayed_binding_prefix;",
        scalar_batch_return_position,
    )
    updated_counters_position = code.find(
        "const auto next = updated_counters(", first_batch_binding_scan_position
    )
    history_digest_position = code.find(
        "LocatorSnapshotChainDigestBuilder snapshot_digest_builder(",
        updated_counters_position,
    )
    batch_replay_end_position = code.find(
        "verification.historical_batch_assertions_and_counters_well_formed =",
        history_digest_position,
    )
    require(
        0
        <= batch_replay_position
        < scalar_batch_guard_position
        < scalar_batch_rejection_position
        < scalar_batch_return_position
        < first_batch_binding_scan_position
        < updated_counters_position
        < history_digest_position
        < batch_replay_end_position,
        "each durable batch must validate scalar fields and prefix bounds "
        "fail-fast before its first binding scan, then validate aggregate "
        "counter arithmetic before digest replay",
    )
    require(
        "continue;" not in code[batch_replay_position:batch_replay_end_position],
        "malformed durable history must return at its first contradiction "
        "instead of rescanning an unchanged prefix in later records",
    )

    exhaustion_contracts = (
        (
            "if (!verification.budget_preflight_certified)",
            (
                "verification.budget_exhausted = true",
                "no_verification_budget_preflight_exhausted",
                "return verification",
            ),
            "structural preflight exhaustion",
        ),
        (
            "if (located.slot_visit_budget_exhausted)",
            (
                "verification.fingerprint_search_budget_exhausted = true",
                "verification.budget_exhausted = true",
                "no_verification_fingerprint_search_budget_exhausted",
                "return verification",
            ),
            "cumulative fingerprint-search exhaustion",
        ),
        (
            "if (verification.insertion_chronology_slot_visit_count >=",
            (
                "verification.insertion_chronology_budget_exhausted = true",
                "verification.budget_exhausted = true",
                "no_verification_insertion_chronology_budget_exhausted",
                "return verification",
            ),
            "cumulative insertion-chronology exhaustion",
        ),
        (
            "if (replay_status == StructuralUnionReplayStatus::budget_exhausted)",
            (
                "verification.union_parent_hop_budget_exhausted = true",
                "verification.budget_exhausted = true",
                "no_verification_union_parent_hop_budget_exhausted",
                "return verification",
            ),
            "cumulative union-parent exhaustion",
        ),
    )
    for marker, required_contract, context in exhaustion_contracts:
        block = braced_block(code, marker, context)
        for required in required_contract:
            require(required in block, f"the {context} is missing {required!r}")

    search_guard_position = code.find(
        "if (result.slot_visit_count >= maximum_slot_visit_count)"
    )
    search_access_position = code.find(
        "const ExactDirectSparsePositiveFacetSlot& slot = slots[slot_index]",
        search_guard_position,
    )
    chronology_guard_position = code.find(
        "if (verification.insertion_chronology_slot_visit_count >="
    )
    chronology_access_position = code.find(
        "replayed_slot_occupancy[replayed_slot_index]",
        chronology_guard_position,
    )
    union_guard_position = code.find(
        "if (parent_hop_count >= maximum_parent_hop_count)"
    )
    union_hop_position = code.find("handle = parents[handle]", union_guard_position)
    require(
        0 <= search_guard_position < search_access_position
        and 0 <= chronology_guard_position < chronology_access_position
        and 0 <= union_guard_position < union_hop_position,
        "each data-dependent operation must consume budget before its slot or "
        "parent access",
    )

    query_position = source.find(
        "for (const ExactDirectSparseFacetQuery& query : queries)"
    )
    union_position = source.find(
        "std::vector<ExactDirectSparseComponentHandle> candidate_parents"
    )
    binding_position = source.find(
        "for (const ExactDirectSparseFacetBinding& binding : bindings)",
        union_position,
    )
    commit_position = source.find(
        "std::copy(\n      candidate_parents.begin()",
        binding_position,
    )
    require(
        0 <= query_position < union_position < binding_position < commit_position,
        "queries, candidate unions, compatibility checks and commit are misordered",
    )
    next_digest_position = source.find(
        "const contract::CanonicalId next_snapshot_digest", binding_position
    )
    digest_commit_position = source.find(
        "committed_history_digest_ = next_snapshot_digest", commit_position
    )
    batch_record_commit_position = source.find(
        "committed_batches_.push_back(candidate_batch_record)", commit_position
    )
    counter_commit_position = source.find(
        "counters_ = *next_counters", batch_record_commit_position
    )
    require(
        0
        <= binding_position
        < next_digest_position
        < commit_position
        < batch_record_commit_position
        < counter_commit_position
        < digest_commit_position,
        "the next canonical snapshot digest must be finalized before mutation and "
        "committed after the accepted batch record and aggregate counters",
    )

    for required_test_contract in (
        "check_snapshot_digest_determinism_and_state_divergence",
        "check_snapshot_digest_golden_vectors",
        "d9b8a4e4b287a77411799dccdeea986565af78a80b26c8e36179852bcb5151a8",
        "7976b55326f90d08889f09071245c0853e703a5e54c326b061f490cccf1b2a9f",
        "different_key_stamp.committed_history_digest",
        "different_witness_stamp.committed_history_digest",
        "different_union_stamp.committed_history_digest",
        "corrupted_digest_view.committed_history_digest",
        "mutated_witness_view.slots",
        "committed_history_digest_freshly_replayed",
        "check_public_fingerprint_is_bounded_and_canonical",
        "committed_slot_insertion_chronology_freshly_replayed",
        "chronology_rejected",
        "generous_structural_verification_budget",
        "exact_verification_budget",
        "exactly_budgeted",
        "maximum_fingerprint_search_slot_visit_count",
        "fingerprint_less_one",
        "no_verification_fingerprint_search_budget_exhausted",
        "maximum_insertion_chronology_slot_visit_count",
        "chronology_less_one",
        "no_verification_insertion_chronology_budget_exhausted",
        "maximum_union_parent_hop_count",
        "union_hop_less_one",
        "no_verification_union_parent_hop_budget_exhausted",
        "table_size_less_one",
        "scratch_population_less_one",
        "scratch_bytes_less_one",
        "table_slot_scan_count == 0U",
    ):
        require(
            required_test_contract in unit_test,
            "the short locator unit suite is missing digest fixture "
            f"{required_test_contract!r}",
        )

    lowered = (header + "\n" + source).lower()
    for forbidden in (
        "std::map<",
        "std::unordered_map",
        "#include <map>",
        "#include <unordered_map>",
        "build_exact_facet_miniball",
        "brute_force_closed_ball",
        "build_exact_gamma",
        "morse_gamma_partition_sweep",
        "delaunay",
    ):
        require(
            forbidden not in lowered,
            f"the isolated locator contains forbidden path {forbidden!r}",
        )
    require(
        re.search(
            r"committed_history_digest_\s*!=\s*" r"contract::CanonicalId\s*\{\s*\}",
            source,
        )
        is None,
        "an all-zero SHA-256 value is valid and must not be used as an "
        "initialization sentinel",
    )

    if binary is not None or nm is not None:
        require(binary is not None and nm is not None, "--binary and --nm pair")
        try:
            symbols = subprocess.run(
                [str(nm), "-C", str(binary)],
                check=True,
                capture_output=True,
                text=True,
            ).stdout.lower()
        except (OSError, subprocess.CalledProcessError) as error:
            raise ContractError(f"cannot inspect target symbols: {error}") from error
        for forbidden in (
            "build_exact_facet_miniball",
            "build_exact_gamma",
            "morse_gamma_partition_sweep",
            "build_exact_critical_arm",
        ):
            require(
                forbidden not in symbols,
                f"the linked test contains forbidden symbol {forbidden!r}",
            )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("project", type=Path)
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--nm", type=Path)
    arguments = parser.parse_args()
    try:
        validate(arguments.project.resolve(), arguments.binary, arguments.nm)
    except ContractError as error:
        print(f"Phase-10 sparse positive facet locator contract failed: {error}")
        return 1
    print("Phase-10 sparse positive facet locator contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
