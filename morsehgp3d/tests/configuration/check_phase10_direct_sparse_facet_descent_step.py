#!/usr/bin/env python3
"""Validate the isolated Phase-10.5b direct sparse facet descent step."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


class ContractError(ValueError):
    """A direct sparse facet descent-step invariant was violated."""


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


def require_all(text: str, needles: tuple[str, ...], context: str) -> None:
    for needle in needles:
        require(needle in text, f"{context} is missing {needle!r}")


def validate_cmake(cmake: str, unit_cmake: str) -> None:
    target = parenthesized_block(
        cmake,
        "add_library(\n  morsehgp3d_direct_sparse_facet_descent_step",
    )
    require(
        target.count("src/cpu/hierarchy/direct_sparse_facet_descent_step.cpp") == 1,
        "the Phase-10.5b target must contain exactly its isolated source",
    )
    require(
        "src/cpu/hierarchy/miniball.cpp" not in target,
        "the isolated target must not absorb the historical hierarchy source",
    )

    links = parenthesized_block(
        cmake,
        "target_link_libraries(\n  morsehgp3d_direct_sparse_facet_descent_step",
    )
    require_all(
        links,
        (
            "morsehgp3d::direct_sparse_positive_facet_locator",
            "morsehgp3d::facet_miniball",
            "morsehgp3d::spatial",
            "morsehgp3d::sanitizers",
        ),
        "the isolated target link closure",
    )
    for forbidden_dependency in (
        "morsehgp3d::hierarchy",
        "morsehgp3d::higher_support",
        "morsehgp3d::critical_catalog",
        "morsehgp3d::direct_closed_saddle_incidence_journal",
        "morsehgp3d::direct_frozen_root_quotient",
    ):
        require(
            forbidden_dependency not in links,
            f"the target links forbidden dependency {forbidden_dependency!r}",
        )

    facet_target = parenthesized_block(
        cmake, "add_library(\n  morsehgp3d_facet_miniball"
    )
    require(
        facet_target.count("src/cpu/hierarchy/facet_miniball.cpp") == 1,
        "the facet-miniball dependency must remain a one-source target",
    )
    facet_links = parenthesized_block(
        cmake, "target_link_libraries(\n  morsehgp3d_facet_miniball"
    )
    require(
        "morsehgp3d::spatial" in facet_links
        and "morsehgp3d::hierarchy" not in facet_links,
        "the local facet miniball may depend on spatial, not the hierarchy archive",
    )

    test_target = parenthesized_block(
        unit_cmake,
        "add_executable(\n  "
        "morsehgp3d_hierarchy_direct_sparse_facet_descent_step_tests",
    )
    require(
        test_target.count("test_hierarchy_direct_sparse_facet_descent_step.cpp") == 1,
        "the unit target must contain exactly the Phase-10.5b test source",
    )
    test_links = parenthesized_block(
        unit_cmake,
        "target_link_libraries(\n  "
        "morsehgp3d_hierarchy_direct_sparse_facet_descent_step_tests",
    )
    require(
        "PRIVATE morsehgp3d::direct_sparse_facet_descent_step" in test_links,
        "the unit test must link only through the isolated public target",
    )
    for forbidden_dependency in (
        "morsehgp3d::hierarchy",
        "morsehgp3d::higher_support",
        "morsehgp3d::spatial",
    ):
        require(
            forbidden_dependency not in test_links,
            f"the unit test bypasses its target through {forbidden_dependency!r}",
        )

    require_all(
        unit_cmake,
        (
            "morsehgp3d_hierarchy_direct_sparse_facet_descent_step_tests",
            "NAME morsehgp3d.hierarchy_direct_sparse_facet_descent_step",
            "check_phase10_direct_sparse_facet_descent_step.py",
            "NAME morsehgp3d.phase10_direct_sparse_facet_descent_step_configuration",
            "$<TARGET_FILE:morsehgp3d_direct_sparse_facet_descent_step>",
        ),
        "the unit and configuration test wiring",
    )


def validate_contract(header: str, source: str, spatial_header: str) -> None:
    require_all(
        header,
        (
            'direct_sparse_facet_descent_step_backend =\n    "reference_cpu"',
            'direct_sparse_facet_descent_step_profile =\n    "hgp_reduced"',
            'direct_sparse_facet_descent_step_mode =\n    "certified"',
            'direct_sparse_facet_descent_step_refinement_status =\n        "partial_refinement"',
            'direct_sparse_facet_descent_step_public_status = "not_claimed"',
            "ExactDirectSparsePositiveFacetProbeBudget source_locator_probe",
            "spatial::ExactLbvhTopKBudget top_k_query",
            "ExactDirectSparsePositiveFacetProbeBudget successor_locator_probe",
            "complete_unresolved_source_above_closed_batch_level",
            "source_facet_at_or_below_closed_batch_level",
            "source_open_target_closed_segment_strict_below_source_level",
            "source_open_target_closed_segment_strict_below_closed_batch_level",
            "closed_segment_strict_below_closed_batch_level",
            "exact_level_relation_count",
            "closed_batch_squared_level",
            "top_k_stop_reason",
            "external_binding_authority_replayed",
            "missing_facet_means_isolated",
            "global_closed_ball_materialized",
            "forbidden_global_structure_materialized",
            "partial_refinement_only",
            "verify_exact_direct_sparse_facet_descent_step(",
        ),
        "the Phase-10.5b public contract",
    )
    require(
        "strict_batch_squared_level" not in header,
        "the public API must name the closed batch, not a strict batch",
    )

    seven_caps = (
        "maximum_node_visit_count",
        "maximum_internal_node_expansion_count",
        "maximum_exact_aabb_bound_evaluation_count",
        "maximum_exact_point_distance_evaluation_count",
        "maximum_frontier_entry_count",
        "maximum_best_neighbor_entry_count",
        "maximum_cutoff_shell_entry_count",
    )
    require_all(spatial_header, seven_caps, "the exact LBVH top-k budget")

    require_all(
        source,
        (
            "source_miniball.squared_radius > closed_batch_squared_level",
            "spatial::lbvh_top_k_budgeted(",
            "top_k_result.stop_reason()",
            "top_k.cutoff_squared_distance() > source_miniball.squared_radius",
            "successor_miniball.squared_radius >=\n      source_miniball.squared_radius",
            "successor_at_source != top_k.cutoff_squared_distance()",
            "strict_witness.source_facet_at_or_below_closed_batch_level = true",
            "strict_witness.source_open_target_closed_segment_strict_below_source_level",
            "strict_witness.source_open_target_closed_segment_strict_below_closed_batch_level",
            "top_k.cutoff_squared_distance() < closed_batch_squared_level",
            "result.strict_half_open_segment_certified = true",
            "locator.probe_positive_facet(",
            "verification.external_binding_authority_replayed = false",
            "verification.fresh_replay_certified = observed == expected",
        ),
        "the bounded strict-step implementation",
    )
    require(
        "source_miniball.squared_radius >= closed_batch_squared_level" not in source,
        "equality beta(F)=a must remain admissible",
    )

    source_probe = source.find("result.source_locator_probe =")
    source_miniball = source.find("build_exact_facet_miniball(", source_probe)
    top_k = source.find("spatial::lbvh_top_k_budgeted(", source_miniball)
    successor_miniball = source.find("build_exact_facet_miniball(", top_k)
    successor_probe = source.find("result.successor_locator_probe.emplace(")
    require(
        0
        <= source_probe
        < source_miniball
        < top_k
        < successor_miniball
        < successor_probe,
        "source probe, local miniballs, bounded top-k and successor probe are misordered",
    )

    lowered = (header + "\n" + source).lower()
    for forbidden in (
        "gamma",
        "coface",
        "delaunay",
        "cell_atlas",
        "cellatlas",
        "ordinary_cell",
        "power_cell",
        "higher_support",
        "closedballpartition",
        "brute_force_closed_ball",
        "lbvh_closed_ball",
        "exterior_point_ids",
        "global_exterior_points",
        "std::vector<std::vector",
    ):
        require(
            forbidden not in lowered,
            f"the isolated source contains forbidden global path {forbidden!r}",
        )


def validate_test(test: str) -> None:
    require_all(
        test,
        (
            "test_equal_level_ac_to_de_closed_segment",
            "D=0, A=1, B=2, C=3, E=4",
            "key({1U, 3U})",
            "key({0U, 4U})",
            "level(33, 2)",
            "level(31, 2)",
            "level(9, 2)",
            "level(17, 2)",
            "test_equal_level_lr_to_lp_source_open_segment",
            "Q=0, L=1, P=2, R=3",
            "key({1U, 2U})",
            "level(5, 16)",
            "!strict.closed_segment_strict_below_closed_batch_level",
            "complete_relative_source_positive_hit",
            "complete_unresolved_strict_successor_not_bound",
            "complete_unresolved_source_above_closed_batch_level",
            "complete_unresolved_source_is_canonical_top_k_choice",
            "complete_unresolved_non_strict_canonical_successor",
            "no_resolution_source_locator_probe_budget_exhausted",
            "no_resolution_top_k_budget_exhausted",
            "no_resolution_successor_locator_probe_budget_exhausted",
            "parent_component_handle",
            "source_parent_hop_short",
            "successor_parent_hop_short",
            "maximum_component_parent_hop_count = 0U",
            "component_parent_hop_budget_exhausted",
            "LbvhTraversalOrder::near_first",
            "LbvhTraversalOrder::far_first",
            "exact_top_k_budget",
            "std::array<ExactLbvhTopKBudget, 7U>",
            "test_fresh_verifier_rejects_mutations",
            "bad_source_level",
            "bad_successor_key",
            "bad_displacement",
            "bad_source_open",
            "bad_closed_segment",
            "bad_audit",
            "bad_handle",
            "bad_decision",
            "twin_cloud",
        ),
        "the short Phase-10.5b unit suite",
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
            "build_exact_facet_miniball",
            "lbvh_top_k_budgeted",
            "probe_positive_facet",
        ),
        "the isolated target symbol closure",
    )
    for forbidden in (
        "gamma",
        "coface",
        "delaunay",
        "cellatlas",
        "closedballpartition",
        "brute_force_closed_ball",
        "lbvh_closed_ball",
        "power_cell",
        "higher_support",
    ):
        require(
            forbidden not in symbols,
            f"the isolated archive exposes forbidden symbol {forbidden!r}",
        )


def validate(project: Path, binary: Path | None, nm: Path | None) -> None:
    cmake = read_text(project / "CMakeLists.txt")
    unit_cmake = read_text(project / "tests/unit/CMakeLists.txt")
    header = read_text(
        project / "include/morsehgp3d/hierarchy/direct_sparse_facet_descent_step.hpp"
    )
    source = read_text(
        project / "src/cpu/hierarchy/direct_sparse_facet_descent_step.cpp"
    )
    spatial_header = read_text(project / "include/morsehgp3d/spatial/lbvh.hpp")
    test = read_text(
        project / "tests/unit/test_hierarchy_direct_sparse_facet_descent_step.cpp"
    )

    validate_cmake(cmake, unit_cmake)
    validate_contract(header, source, spatial_header)
    validate_test(test)

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
        print(f"Phase-10.5b direct sparse facet descent contract failed: {error}")
        return 1
    print("Phase-10.5b direct sparse facet descent contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
