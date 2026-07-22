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


def validate(project: Path, binary: Path | None, nm: Path | None) -> None:
    cmake = read_text(project / "CMakeLists.txt")
    header = read_text(
        project
        / "include/morsehgp3d/hierarchy/"
        "direct_sparse_positive_facet_locator.hpp"
    )
    source = read_text(
        project
        / "src/cpu/hierarchy/direct_sparse_positive_facet_locator.cpp"
    )
    unit_test = read_text(
        project
        / "tests/unit/test_hierarchy_direct_sparse_positive_facet_locator.cpp"
    )

    target = parenthesized_block(
        cmake,
        "add_library(\n  morsehgp3d_direct_sparse_positive_facet_locator",
    )
    require(
        target.count(
            "src/cpu/hierarchy/direct_sparse_positive_facet_locator.cpp"
        )
        == 1,
        "the target must contain exactly its isolated source",
    )
    links = parenthesized_block(
        cmake,
        "target_link_libraries(\n  "
        "morsehgp3d_direct_sparse_positive_facet_locator",
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
        "committed_binding_index",
        "ExactDirectSparseCommittedBatchRecord",
        "ExactDirectSparsePositiveFacetLocatorStateView",
        "committed_history_digest",
        "committed_history_digest_freshly_replayed",
        "external_authority_replayed_by_locator",
        "verify_exact_direct_sparse_positive_facet_locator_structure(",
    ):
        require(required in header, f"the public contract is missing {required!r}")
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
        "key_fingerprint(query.key, config_.fingerprint_mask)",
        "key_fingerprint(binding.key, config_.fingerprint_mask)",
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
        "if (!verification.trusted_construction_parameters_certified ||",
        "verification.table_slot_scan_count = observed.slots.size()",
    ):
        require(required in source, f"the implementation is missing {required!r}")

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
            r"committed_history_digest_\s*!=\s*"
            r"contract::CanonicalId\s*\{\s*\}",
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
