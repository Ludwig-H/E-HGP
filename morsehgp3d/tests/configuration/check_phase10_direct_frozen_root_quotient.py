#!/usr/bin/env python3
"""Validate the isolated root-only Phase-10 frozen quotient kernel."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


class ContractError(ValueError):
    """A frozen-root quotient invariant was violated."""


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
        / "include/morsehgp3d/hierarchy/direct_frozen_root_quotient.hpp"
    )
    source = read_text(
        project / "src/cpu/hierarchy/direct_frozen_root_quotient.cpp"
    )

    target = parenthesized_block(
        cmake, "add_library(\n  morsehgp3d_direct_frozen_root_quotient"
    )
    require(
        target.count("src/cpu/hierarchy/direct_frozen_root_quotient.cpp")
        == 1,
        "the kernel target must contain exactly its isolated source",
    )
    links = parenthesized_block(
        cmake,
        "target_link_libraries(\n  morsehgp3d_direct_frozen_root_quotient",
    )
    require(
        "PUBLIC" not in links and "morsehgp3d::sanitizers" in links,
        "the standalone kernel must expose no project dependency",
    )
    require(
        '#include "morsehgp3d/' not in header,
        "the public kernel header must include only standard headers",
    )
    require(
        "std::vector<std::vector" not in header,
        "the quotient ABI must remain flat",
    )

    for required in (
        "one_root_class",
        "multiple_root_class",
        "external_root_snapshot_membership_checked",
        "zero_root_hyperedges_supported",
        "latent_facet_tokens_supported",
        "hgp_batch_actions_claimed",
        "root_attachment_or_global_mutation_performed",
        "verify_exact_direct_frozen_root_quotient_streaming(",
        "no_second_persistent_output_arena_certified",
    ):
        require(required in header, f"the root-only ABI is missing {required!r}")

    for required in (
        "checked_multiply(3U, root_references.size())",
        "checked_multiply(2U, hyperedges.size())",
        "valid_canonical_csr(",
        "components.unite(",
        "std::sort(",
        "result.one_root_groups_preserved = true",
        "!latent_facet_tokens_supported",
    ):
        require(required in source, f"the kernel is missing {required!r}")
    analysis = braced_block(source, "analyze_frozen_root_quotient(\n")
    require(
        analysis.find("if (!budget_is_sufficient(result))")
        < analysis.find("if (!valid_canonical_csr("),
        "allocation-safe caps must be checked before CSR traversal",
    )

    replay = braced_block(
        source, "verify_exact_direct_frozen_root_quotient_streaming(\n"
    )
    require(
        "build_exact_direct_frozen_root_quotient(" not in replay,
        "the streaming verifier must not build a second quotient payload",
    )
    for required in (
        "expected.hyperedge_bindings.empty()",
        "expected.groups.empty()",
        "expected.group_root_ids.empty()",
        "group_root_arena_matches(analysis, observed)",
    ):
        require(required in replay, f"the streaming replay is missing {required!r}")

    lowered = source.lower()
    for forbidden in (
        "std::map",
        "std::unordered_map",
        "build_exact_gamma",
        "build_exact_facet_miniball",
        "morse_gamma_partition_sweep",
        "delaunay",
    ):
        require(
            forbidden not in lowered,
            f"the standalone kernel contains forbidden path {forbidden!r}",
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
            "build_exact_gamma",
            "build_exact_facet_miniball",
            "morse_gamma_partition_sweep",
            "build_exact_critical_arm",
        ):
            require(
                forbidden not in symbols,
                f"the linked kernel test contains forbidden symbol {forbidden!r}",
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
        print(f"Phase-10 frozen-root quotient contract failed: {error}")
        return 1
    print("Phase-10 frozen-root quotient contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
