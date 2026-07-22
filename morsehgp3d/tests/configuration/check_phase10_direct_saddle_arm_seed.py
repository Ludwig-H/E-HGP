#!/usr/bin/env python3
"""Validate the factorized, no-Gamma Phase-10 saddle-arm seed target."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


class ContractError(ValueError):
    """A direct Phase-10 target invariant was violated."""


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


def validate(project: Path, binary: Path | None, nm: Path | None) -> None:
    cmake = read_text(project / "CMakeLists.txt")
    header = read_text(
        project
        / "include/morsehgp3d/hierarchy/direct_saddle_arm_seed_journal.hpp"
    )
    source = read_text(
        project / "src/cpu/hierarchy/direct_saddle_arm_seed_journal.cpp"
    )
    event_source = read_text(
        project / "src/cpu/hierarchy/direct_morse_event_journal.cpp"
    )

    target = parenthesized_block(
        cmake, "add_library(\n  morsehgp3d_direct_saddle_arm_seed_journal"
    )
    require(
        target.count("src/cpu/hierarchy/direct_saddle_arm_seed_journal.cpp")
        == 1,
        "the seed target must contain exactly its direct factorized source",
    )
    links = parenthesized_block(
        cmake,
        "target_link_libraries(\n  morsehgp3d_direct_saddle_arm_seed_journal",
    )
    require(
        "PUBLIC morsehgp3d::direct_morse_event_journal" in links,
        "the seed target must consume the direct Phase-10 event journal",
    )
    for forbidden in (
        "morsehgp3d::hierarchy",
        "local_morse_geometry",
        "miniball",
        "gamma",
        "critical_arm",
    ):
        require(
            forbidden not in links.lower(),
            f"the direct seed target links forbidden dependency {forbidden!r}",
        )

    require(
        "std::vector<spatial::PointId>" not in header,
        "the seed ABI must not retain materialized arm facets",
    )
    require(
        "maximum_point_count = 10U" in header,
        "on-demand arm reconstruction must remain bounded to K<=10",
    )
    for required in (
        "required_saddle_family_count",
        "required_arm_seed_count",
        "source_relative_strict_miniball_drop_theorem_applies",
        "facets_materialized_in_journal",
        "miniballs_or_global_partitions_computed",
        "forest_or_gateway_attach_performed",
        "partial_refinement_only",
    ):
        require(required in header, f"the seed contract is missing {required!r}")

    require(
        "verify_exact_direct_morse_event_journal_streaming(" in source,
        "the seed builder must validate Phase 10.1 without a second journal arena",
    )
    require(
        "verify_exact_direct_morse_event_journal(" not in source,
        "the seed builder calls the allocating Phase-10.1 verifier",
    )
    require(
        "source_event_arm_identity_digest(event)" in source,
        "on-demand facets must bind their exact source-event identity",
    )
    streaming_replay = braced_block(
        event_source,
        "verify_exact_direct_morse_event_journal_streaming(\n",
    )
    for forbidden in (
        "std::vector",
        "std::sort",
        "build_exact_direct_morse_event_journal(",
    ):
        require(
            forbidden not in streaming_replay,
            f"the streaming source verifier contains {forbidden!r}",
        )
    require(
        "constant_auxiliary_record_storage_certified = true"
        in streaming_replay,
        "the streaming source verifier does not close its storage audit",
    )

    lowered_source = source.lower()
    for forbidden in (
        "build_exact_facet_miniball",
        "brute_force_closed_ball",
        "build_exact_critical_arm",
        "gamma.hpp",
        "coface",
        "delaunay",
    ):
        require(
            forbidden not in lowered_source,
            f"the direct seed source contains forbidden path {forbidden!r}",
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
        for forbidden_symbol in (
            "build_exact_facet_miniball",
            "build_exact_critical_arm",
            "build_exact_gamma",
            "morse_gamma_partition_sweep",
        ):
            require(
                forbidden_symbol not in symbols,
                f"the linked seed test contains {forbidden_symbol!r}",
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
        print(f"Phase-10 direct saddle-arm seed contract failed: {error}")
        return 1
    print("Phase-10 direct saddle-arm seed contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
