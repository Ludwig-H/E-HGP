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
        "direct_sparse_positive_facet_locator_prefix_stamp_sweep_backend",
        '"reference_cpu"',
        '"hgp_reduced"',
        '"certified"',
        '"partial_refinement"',
        '"not_claimed"',
        "ExactDirectSparsePositiveFacetLocatorPrefixStampSweepBudget",
        "maximum_prefix_request_count",
        "maximum_batch_record_scan_count",
        "maximum_table_slot_scan_count",
        "maximum_binding_slot_index_scratch_count",
        "maximum_union_record_replay_count",
        "maximum_binding_record_replay_count",
        "maximum_key_point_replay_count",
        "ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult",
        "required_table_slot_scan_count",
        "required_temporary_scratch_byte_count",
        "prefix_request_count",
        "no_prefix_stamp_budget_exhausted",
        "no_prefix_stamp_input_shape_rejected",
        "no_prefix_stamp_locator_history_rejected",
        "complete_certified_locator_prefix_stamps",
        "locator_internal_committed_batch_prefix_stamps_relative_to_frozen_history_only",
        "build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(",
        "verify_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(",
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
        "history_cursor.history_digest == observed.committed_history_digest",
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
        "make_locator_snapshot_stamp(",
        "replay_locator_history_digest_transition(",
        "std::vector<std::size_t> binding_slot_indices_by_index(",
        "active_binding_prefix_count == 0U ? 0U : state.slots.size()",
        "result.required_temporary_scratch_byte_count",
        "prefix_stamps.back() != result.locator_snapshot_stamp",
        "locator.snapshot_stamp() == result.locator_snapshot_stamp",
        "no_prefix_stamp_locator_history_rejected",
    ):
        require(required in source, f"the implementation is missing {required!r}")
    require(
        re.search(
            r"checked_multiply\s*\(\s*2U\s*,\s*"
            r"result\.required_committed_batch_prefix_count\s*\)",
            source,
        )
        is not None,
        "the prefix sweep must preflight exactly two batch-record scans per "
        "active batch",
    )
    require(
        "source_prefix_request_count" not in header + source,
        "the public prefix request count must use its canonical field name",
    )

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

    history_transition = braced_block(
        code,
        "[[nodiscard]] LocatorHistoryTransitionStatus\n"
        "replay_locator_history_digest_transition(",
        "shared locator history digest transition",
    )
    require(
        code.count("replay_locator_history_digest_transition(") == 3,
        "the common history transition must have exactly one definition and "
        "one call from each read-only verifier",
    )
    require(
        source.count("locator_snapshot_initial_digest_domain") == 2
        and source.count("locator_snapshot_chain_digest_domain") == 2
        and source.count("class LocatorSnapshotChainDigestBuilder") == 1,
        "locator snapshot serialization must retain exactly one initial domain, "
        "one chain domain and one digest-builder implementation",
    )
    scalar_guard_position = history_transition.find(
        "if (record.committed_batch_index != expected_batch_index ||"
    )
    scalar_rejection_position = history_transition.find(
        "return LocatorHistoryTransitionStatus::malformed_history;",
        scalar_guard_position,
    )
    aggregate_position = history_transition.find(
        "const auto next_counters = updated_counters(", scalar_rejection_position
    )
    aggregate_rejection_position = history_transition.find(
        "return LocatorHistoryTransitionStatus::capacity_overflow;",
        aggregate_position,
    )
    digest_position = history_transition.find(
        "LocatorSnapshotChainDigestBuilder snapshot_digest_builder(",
        aggregate_rejection_position,
    )
    first_binding_scan_position = history_transition.find(
        "for (std::size_t binding_index = cursor.binding_prefix;", digest_position
    )
    require(
        0
        <= scalar_guard_position
        < scalar_rejection_position
        < aggregate_position
        < aggregate_rejection_position
        < digest_position
        < first_binding_scan_position,
        "the shared transition must reject scalar fields and prefix bounds "
        "fail-fast, validate aggregate counter arithmetic, then replay the "
        "canonical digest before scanning bindings",
    )
    for scalar_contract in (
        "*query_partition != record.counters.query_count",
        "*binding_partition != record.counters.binding_request_count",
        "record.counters.full_key_comparison_count <",
        "record.counters.query_count > trusted_budget.maximum_batch_query_count",
        "record.counters.union_request_count >",
        "trusted_budget.maximum_batch_union_count",
        "record.counters.binding_request_count >",
        "trusted_budget.maximum_batch_binding_count",
        "record.counters.batch_input_key_point_count >",
        "trusted_budget.maximum_batch_key_point_count",
        "*next_binding_prefix > binding_slot_indices_by_index.size()",
        "*next_union_prefix > committed_unions.size()",
        "!record.input_shape_certified",
        "!record.input_witness_structure_certified",
        "!record.strict_pre_batch_snapshot_certified",
        "!record.sequential_atomic_commit_certified",
    ):
        require(
            scalar_contract in history_transition,
            "the shared fail-fast transition is missing " f"{scalar_contract!r}",
        )
    for transition_contract in (
        "snapshot_digest_builder.component_union(",
        "snapshot_digest_builder.binding(",
        "observed_inserted_key_point_count !=",
        "record.counters.inserted_key_point_count",
        "cursor.counters = *next_counters",
        "cursor.binding_prefix = *next_binding_prefix",
        "cursor.union_prefix = *next_union_prefix",
        "cursor.history_digest = snapshot_digest_builder.finalize()",
    ):
        require(
            transition_contract in history_transition,
            "the shared canonical transition is missing " f"{transition_contract!r}",
        )
    require(
        "continue;" not in history_transition,
        "malformed durable history must return at its first contradiction",
    )

    structural_verifier = braced_block(
        code,
        "ExactDirectSparsePositiveFacetLocatorStructuralVerification\n"
        "verify_exact_direct_sparse_positive_facet_locator_structure(",
        "structural verifier",
    )
    prefix_sweep = braced_block(
        code,
        "ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult\n"
        "build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(",
        "prefix-stamp sweep",
    )
    require(
        structural_verifier.count("replay_locator_history_digest_transition(") == 1
        and prefix_sweep.count("replay_locator_history_digest_transition(") == 1,
        "both read-only history consumers must call the one common transition",
    )

    preflight_complete_position = prefix_sweep.find(
        "result.budget_preflight_certified = true"
    )
    binding_scratch_position = prefix_sweep.find("std::vector<std::size_t>")
    stamp_output_position = prefix_sweep.find(
        "std::vector<ExactDirectSparsePositiveFacetLocatorSnapshotStamp>"
    )
    require(
        0
        <= preflight_complete_position
        < binding_scratch_position
        < stamp_output_position,
        "all eight exact caps must pass before the unique binding-index scratch "
        "and prefix output are allocated",
    )
    require(
        prefix_sweep.count("std::vector<std::size_t>") == 1,
        "the prefix sweep must allocate exactly one binding-index scratch vector",
    )
    require(
        prefix_sweep.count("batch_index < result.required_committed_batch_prefix_count")
        == 2,
        "the prefix sweep must scan the active batch prefix exactly twice",
    )
    for cap in (
        "maximum_prefix_request_count",
        "maximum_batch_record_scan_count",
        "maximum_table_slot_scan_count",
        "maximum_binding_slot_index_scratch_count",
        "maximum_union_record_replay_count",
        "maximum_binding_record_replay_count",
        "maximum_key_point_replay_count",
        "maximum_temporary_scratch_byte_count",
    ):
        require(
            prefix_sweep.find(cap) < preflight_complete_position,
            f"the {cap} cap must be checked atomically before allocation",
        )
    for exact_cost_contract in (
        "active_binding_prefix_count == 0U ? 0U : state.slots.size()",
        "active_binding_prefix_count, sizeof(std::size_t)",
        "replaying_final_history",
        "state.counters.inserted_binding_count",
        "state.counters.union_request_count",
        "state.counters.committed_key_point_count",
        "slot.committed_binding_index >= active_binding_prefix_count",
        "prefix_stamps.back() != result.locator_snapshot_stamp",
        "locator.snapshot_stamp() == result.locator_snapshot_stamp",
    ):
        require(
            exact_cost_contract in prefix_sweep,
            "the exact prefix sweep is missing " f"{exact_cost_contract!r}",
        )
    table_guard_position = prefix_sweep.find("if (active_binding_prefix_count != 0U)")
    table_scan_position = prefix_sweep.find(
        "for (std::size_t slot_index = 0U;", table_guard_position
    )
    require(
        0 <= table_guard_position < table_scan_position,
        "the table may be scanned only when the active binding prefix is nonempty",
    )
    lowered_prefix_sweep = prefix_sweep.lower()
    for forbidden_prefix_path in (
        "apply_batch",
        "unite_components",
        "find_component",
        "candidate_parents",
        "component_parents[",
        "std::map",
        "unordered",
        "gamma",
        "delaunay",
        "quotient",
        "gatewayattach",
    ):
        require(
            forbidden_prefix_path not in lowered_prefix_sweep,
            "the prefix sweep contains forbidden path " f"{forbidden_prefix_path!r}",
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
        "check_prefix_stamp_sweep_and_exact_budgets",
        "check_prefix_stamp_sweep_empty_invalid_stale_and_mutations",
        "PrefixStampSixCommitFixture",
        "live_snapshot_stamps",
        "historical_stamps_match_live_commits",
        "repeated_prefixes",
        "prefix_stamps.back() == exact.locator_snapshot_stamp",
        "prefix_request_less_one",
        "batch_scan_less_one",
        "table_scan_less_one",
        "binding_scratch_less_one",
        "union_replay_less_one",
        "binding_replay_less_one",
        "key_point_replay_less_one",
        "extra_final_slot_rejected",
        "malformed_rejected",
        "stale_verification",
    ):
        require(
            required_test_contract in unit_test,
            "the short locator unit suite is missing digest fixture "
            f"{required_test_contract!r}",
        )
    require(
        len(
            re.findall(
                r"fixture\.live_snapshot_stamps\[\dU\]\s*=\s*"
                r"locator\.snapshot_stamp\(\)",
                unit_test,
            )
        )
        == 7,
        "the six-commit fixture must capture the initial live snapshot and one "
        "independent live snapshot after every commit",
    )
    require(
        re.search(
            r"exact\.prefix_stamps\[request_index\]\s*==\s*"
            r"fixture\.live_snapshot_stamps\["
            r"repeated_prefixes\[request_index\]\]",
            unit_test,
            re.S,
        )
        is not None,
        "every repeated prefix output must be checked against its independently "
        "captured live commit snapshot",
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
