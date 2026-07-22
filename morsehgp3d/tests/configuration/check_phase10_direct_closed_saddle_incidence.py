#!/usr/bin/env python3
"""Validate the factorized Phase-10.4 direct saddle incidence target."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


class ContractError(ValueError):
    """A Phase-10.4 target invariant was violated."""


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
        "direct_closed_saddle_incidence_journal.hpp"
    )
    source = read_text(
        project
        / "src/cpu/hierarchy/direct_closed_saddle_incidence_journal.cpp"
    )

    target = parenthesized_block(
        cmake,
        "add_library(\n  morsehgp3d_direct_closed_saddle_incidence_journal",
    )
    require(
        target.count(
            "src/cpu/hierarchy/direct_closed_saddle_incidence_journal.cpp"
        )
        == 1,
        "the target must contain exactly its isolated source",
    )
    links = parenthesized_block(
        cmake,
        "target_link_libraries(\n  "
        "morsehgp3d_direct_closed_saddle_incidence_journal",
    )
    require(
        "PUBLIC morsehgp3d::direct_saddle_arm_seed_journal" in links,
        "Phase 10.4 must reuse the Phase-10.2 factorized arms",
    )
    for forbidden in (
        "morsehgp3d::hierarchy",
        "local_morse_geometry",
        "gamma",
        "miniball",
        "critical_arm",
    ):
        require(
            forbidden not in links.lower(),
            f"the target links forbidden dependency {forbidden!r}",
        )

    for required in (
        "required_equal_level_facet_seed_count",
        "strict_and_equal_deletions_partition_every_saddle",
        "equal_level_miniball_sandwich_theorem_applies",
        "strict_arms_reused_without_copy",
        "non_direct_gateway_generation_complete",
        "missing_facet_means_isolated",
        "frozen_quotient_performed",
        "partial_refinement_only",
        "verify_exact_direct_closed_saddle_incidence_journal_streaming(",
    ):
        require(required in header, f"the public contract is missing {required!r}")
    require(
        "std::vector<std::vector" not in header,
        "the incidence ABI must remain flat",
    )
    require(
        "std::vector<spatial::PointId>" not in header,
        "the journal must not retain materialized facets",
    )

    for required in (
        "checked_multiply(10U, source_facade.events.size())",
        "3U, cloud.size(), 20U, source_facade.events.size()",
        "verify_exact_direct_saddle_arm_seed_journal_streaming(",
        "result.equal_level_facet_seeds.reserve(",
        "reconstruct_equal_level_facet(event, removed_id)",
        "result.non_direct_gateway_generation_complete = false",
        "result.missing_facet_means_isolated = false",
    ):
        require(required in source, f"the implementation is missing {required!r}")
    require(
        source.find("budget_is_sufficient(*requirements, budget)")
        < source.find("verify_exact_direct_saddle_arm_seed_journal_streaming("),
        "budget preflight must precede source replay",
    )
    lowered = source.lower()
    for forbidden in (
        "build_exact_facet_miniball",
        "brute_force_closed_ball",
        "build_exact_gamma",
        "morse_gamma_partition_sweep",
        "delaunay",
    ):
        require(
            forbidden not in lowered,
            f"the isolated implementation contains forbidden path {forbidden!r}",
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
        print(f"Phase-10.4 direct incidence contract failed: {error}")
        return 1
    print("Phase-10.4 direct incidence contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
