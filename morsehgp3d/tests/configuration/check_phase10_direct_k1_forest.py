#!/usr/bin/env python3
"""Validate the isolated, no-global-geometry Phase-10 direct k=1 forest."""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


class ContractError(ValueError):
    """A direct Phase-10 k=1 forest invariant was violated."""


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


def require_tokens(text: str, tokens: tuple[str, ...], label: str) -> None:
    for token in tokens:
        require(token in text, f"{label} is missing {token!r}")


def require_tokens_in_order(
    text: str, tokens: tuple[str, ...], label: str
) -> None:
    cursor = 0
    for token in tokens:
        position = text.find(token, cursor)
        require(position >= 0, f"{label} is missing ordered token {token!r}")
        cursor = position + len(token)


def cmake_public_dependencies(link_block: str) -> list[str]:
    match = re.search(
        r"\bPUBLIC\b(.*?)(?=\b(?:PRIVATE|INTERFACE)\b|\))",
        link_block,
        flags=re.DOTALL,
    )
    require(match is not None, "the direct k=1 target has no PUBLIC seam")
    return match.group(1).split()


def validate(project: Path, binary: Path | None, nm: Path | None) -> None:
    cmake = read_text(project / "CMakeLists.txt")
    header = read_text(
        project / "include/morsehgp3d/hierarchy/direct_k1_forest_journal.hpp"
    )
    source = read_text(
        project / "src/cpu/hierarchy/direct_k1_forest_journal.cpp"
    )

    target = parenthesized_block(
        cmake, "add_library(\n  morsehgp3d_direct_k1_forest_journal"
    )
    require(
        target.count("src/cpu/hierarchy/direct_k1_forest_journal.cpp") == 1,
        "the direct k=1 target must contain exactly its isolated source",
    )
    links = parenthesized_block(
        cmake,
        "target_link_libraries(\n  morsehgp3d_direct_k1_forest_journal",
    )
    require(
        cmake_public_dependencies(links)
        == ["morsehgp3d::direct_saddle_arm_seed_journal"],
        "the direct k=1 target must have only the Phase-10.2 target PUBLIC",
    )
    for forbidden_link in (
        "morsehgp3d::hierarchy",
        "morsehgp3d::spatial",
        "morsehgp3d::direct_morse_event_journal",
        "morsehgp3d::exact",
        "gamma",
        "miniball",
        "critical_arm",
        "k1_forest",
    ):
        if forbidden_link == "k1_forest":
            # The target's own name contains this token; only its dependency
            # list after the target name is relevant here.
            dependency_text = links.replace(
                "morsehgp3d_direct_k1_forest_journal", ""
            ).lower()
        else:
            dependency_text = links.lower()
        require(
            forbidden_link not in dependency_text,
            f"the direct k=1 target links forbidden dependency {forbidden_link!r}",
        )

    project_includes = re.findall(r'^#include\s+"([^"]+)"', header, re.MULTILINE)
    require(
        project_includes
        == ["morsehgp3d/hierarchy/direct_saddle_arm_seed_journal.hpp"],
        "the direct k=1 header must expose only the Phase-10.2 project seam",
    )
    require(
        "std::vector<std::vector" not in header,
        "the direct k=1 journal must use flat arenas",
    )

    # These words denote forbidden global-geometry paths in this isolated
    # translation unit.  Keeping them out also prevents a negative status
    # field from becoming a future accidental ABI seam to the legacy path.
    lowered_local_code = (header + "\n" + source).lower()
    for forbidden_word in (
        "gamma",
        "cell",
        "coface",
        "miniball",
        "cut_strict",
    ):
        require(
            forbidden_word not in lowered_local_code,
            f"the direct k=1 header/source contains forbidden path {forbidden_word!r}",
        )

    require_tokens(
        header,
        (
            "struct ExactDirectK1ForestBudget",
            "maximum_source_replay_entry_count",
            "maximum_point_scratch_entry_count",
            "maximum_equal_level_scratch_entry_count",
            "maximum_arm_root_binding_count",
            "maximum_saddle_record_count",
            "maximum_atomic_group_count",
            "maximum_child_reference_count",
            "maximum_batch_record_count",
            "struct ExactDirectK1ArmRootBinding",
            "pre_batch_root_node_id",
            "struct ExactDirectK1SaddleRecord",
            "distinct_pre_batch_root_count",
            "struct ExactDirectK1AtomicGroup",
            "std::optional<ExactDirectK1NodeId> created_node_id",
            "struct ExactDirectK1AtomicBatch",
            "roots_resolved_from_frozen_pre_batch_snapshot",
            "quotient_committed_atomically",
            "logical_added_storage_entry_limit",
            "combined_logical_storage_entry_limit",
            "every_arm_bound_to_strict_pre_batch_root",
            "equal_level_quotients_resolved_before_mutation",
            "continuation_groups_preserved_without_new_node",
            "closed_diameter_strict_edge_descent_theorem_applies",
            "exact_order_one_forest_reduction_performed",
            "higher_order_root_localization_claimed",
            "partial_refinement_only",
            "build_exact_direct_k1_forest_journal(",
            "verify_exact_direct_k1_forest_journal_streaming(",
        ),
        "the direct k=1 public contract",
    )

    replay = braced_block(source, "replay_direct_k1_forest(\n")
    require_tokens(
        replay,
        (
            "event.closed_rank != 2U",
            "event.support_size != 2U",
            "!event.interior_ids.empty()",
            "family.arm_seed_count != 2U",
            "facet.point_count != 1U",
            "LocalDisjointSet quotient",
            "quotient.unite(",
            "if (arity == 1U)",
            "++continuation_group_count",
            "group_record.child_count = 0U",
            "group_record.created_node_id = std::nullopt",
            "components.component_count() != 1U",
            "result.equal_level_quotients_resolved_before_mutation = true",
            "result.continuation_groups_preserved_without_new_node = true",
        ),
        "the direct k=1 replay",
    )
    require(
        "class LocalDisjointSet" in source
        and "class CanonicalDisjointSet" in source,
        "the direct k=1 source must keep separate global and quotient DSUs",
    )
    require_tokens_in_order(
        replay,
        (
            "const std::size_t strict_root_count = components.component_count()",
            "std::vector<TemporarySaddle> temporary_saddles",
            "saddle.pre_batch_component_roots[local] = components.find(",
            "std::sort(touched_roots.begin(), touched_roots.end())",
            "LocalDisjointSet quotient",
            "std::vector<TemporaryGroup> temporary_groups",
            "for (const TemporaryGroup& group : temporary_groups)",
            "components.unite(",
            "const ExactDirectK1AtomicBatch batch",
        ),
        "strict snapshot -> quotient -> atomic mutation",
    )
    require_tokens_in_order(
        replay,
        (
            "if (!budget_is_sufficient(result))",
            "result.budget_preflight_certified = true",
            "verify_exact_direct_saddle_arm_seed_journal_streaming(",
            "result.arm_root_bindings.reserve(",
        ),
        "preflight -> streaming source replay -> output allocation",
    )
    require(
        "verify_exact_direct_saddle_arm_seed_journal(" not in replay,
        "the direct k=1 builder calls the allocating Phase-10.2 verifier",
    )

    require_tokens(
        replay,
        (
            "checked_multiply(2U, cloud.size())",
            "checked_multiply(11U, maximum_batch_saddle_count)",
            "checked_multiply(2U, order_one_family_count)",
            "checked_multiply(2U, cloud.size() - 1U)",
            "checked_linear_bound(\n      2U, cloud.size(), 5U, order_one_family_count)",
            "*added_limit_base - 2U",
            "binding_index + saddle_record_index + atomic_group_index +\n      child_index + batch_index",
            "result.logical_added_storage_entry_count <=\n          result.logical_added_storage_entry_limit",
            "result.combined_logical_storage_entry_count <=\n          result.combined_logical_storage_entry_limit",
        ),
        "the direct k=1 linear bounds",
    )

    for forbidden_source_symbol in (
        "build_compact_k1_forest(",
        "build_exact_complete_graph_emst(",
        "build_exact_critical_arm",
        "build_exact_facet_miniball",
        "build_exact_reduced_",
        "build_exact_gamma",
        "morse_gamma_partition_sweep",
    ):
        require(
            forbidden_source_symbol not in source,
            f"the direct k=1 source calls {forbidden_source_symbol!r}",
        )

    if binary is not None or nm is not None:
        require(binary is not None and nm is not None, "--binary and --nm pair")
        require(binary.is_file(), f"target binary does not exist: {binary}")
        require(nm.is_file(), f"nm executable does not exist: {nm}")
        try:
            symbols = subprocess.run(
                [str(nm), "-C", str(binary)],
                check=True,
                capture_output=True,
                text=True,
            ).stdout.lower()
        except (OSError, subprocess.CalledProcessError) as error:
            raise ContractError(f"cannot inspect target symbols: {error}") from error
        for forbidden_symbol in (
            "build_compact_k1_forest",
            "k1compactforest",
            "build_exact_complete_graph_emst",
            "build_exact_critical_arm",
            "criticalarmfamily",
            "build_exact_facet_miniball",
            "build_exact_gamma",
            "gamma_transition",
            "reduced_gamma",
            "morse_gamma_partition_sweep",
            "ordinary_cell",
            "power_cell",
            "coface",
            "cut_strict",
        ):
            require(
                forbidden_symbol not in symbols,
                f"the linked direct k=1 binary contains {forbidden_symbol!r}",
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
        print(f"Phase-10 direct k=1 forest contract failed: {error}")
        return 1
    print("Phase-10 direct k=1 forest contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
