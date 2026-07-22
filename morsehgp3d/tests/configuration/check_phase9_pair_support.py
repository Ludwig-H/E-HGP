#!/usr/bin/env python3
"""Validate the static no-mosaic contract of the Phase 9 pair target."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


class ContractError(ValueError):
    """A Phase 9 pair-target invariant was violated."""


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


def validate(
    project: Path,
    binary: Path | None = None,
    nm_executable: Path | None = None,
) -> dict[str, object]:
    cmake = read_text(project / "CMakeLists.txt")
    header = read_text(
        project / "include/morsehgp3d/hierarchy/pair_support_stream.hpp"
    )
    source = read_text(project / "src/cpu/hierarchy/pair_support_stream.cpp")
    codec_header = read_text(
        project / "include/morsehgp3d/hierarchy/pair_support_stream_codec.hpp"
    )
    codec_source = read_text(
        project / "src/cpu/hierarchy/pair_support_stream_codec.cpp"
    )
    durable_header = read_text(
        project / "include/morsehgp3d/hierarchy/pair_support_stream_durable.hpp"
    )
    durable_source = read_text(
        project / "src/cpu/hierarchy/pair_support_stream_durable.cpp"
    )

    pair_target = parenthesized_block(
        cmake, "add_library(\n  morsehgp3d_pair_support"
    )
    require(
        pair_target.count("src/cpu/hierarchy/pair_support_stream.cpp") == 1,
        "the pair target must contain exactly its direct producer source",
    )
    require("gamma" not in pair_target.lower(), "the pair target contains Gamma")
    require("ordinary" not in pair_target.lower(), "the pair target contains cells")

    pair_links = parenthesized_block(
        cmake, "target_link_libraries(\n  morsehgp3d_pair_support"
    )
    require(
        "PUBLIC morsehgp3d::spatial" in pair_links,
        "the pair target must depend on the certified spatial layer",
    )
    require(
        "morsehgp3d::hierarchy" not in pair_links,
        "the pair target must not link the historical Gamma archive",
    )

    codec_target = parenthesized_block(
        cmake, "add_library(\n  morsehgp3d_pair_support_codec"
    )
    require(
        codec_target.count(
            "src/cpu/hierarchy/pair_support_stream_codec.cpp"
        )
        == 1,
        "the codec target must contain exactly its canonical codec source",
    )
    codec_links = parenthesized_block(
        cmake, "target_link_libraries(\n  morsehgp3d_pair_support_codec"
    )
    require(
        "PUBLIC morsehgp3d::pair_support" in codec_links,
        "the codec target must depend on the direct pair stream",
    )
    require(
        "morsehgp3d::hierarchy" not in codec_links,
        "the codec target must not link the historical Gamma archive",
    )

    durable_target = parenthesized_block(
        cmake, "add_library(\n    morsehgp3d_pair_support_durable"
    )
    require(
        durable_target.count(
            "src/cpu/hierarchy/pair_support_stream_durable.cpp"
        )
        == 1,
        "the durable target must contain exactly its POSIX sink source",
    )
    durable_links = parenthesized_block(
        cmake, "target_link_libraries(\n    morsehgp3d_pair_support_durable"
    )
    require(
        "PUBLIC morsehgp3d::pair_support_codec" in durable_links,
        "the durable target must depend on the bounded codec",
    )
    require(
        "morsehgp3d::hierarchy" not in durable_links,
        "the durable target must not link the historical Gamma archive",
    )

    hierarchy_target = parenthesized_block(
        cmake, "add_library(\n  morsehgp3d_hierarchy"
    )
    require(
        "pair_support_stream.cpp" not in hierarchy_target,
        "the direct pair source leaked back into the Gamma archive",
    )

    combined = "\n".join(
        (
            header,
            source,
            codec_header,
            codec_source,
            durable_header,
            durable_source,
        )
    )
    for forbidden in (
        "hierarchy/gamma.hpp",
        "critical_catalog",
        "ordinary_cell",
        "ordinary_diagram",
        "depth_zero_natural_support",
        "lbvh_closed_ball(",
        "std::vector<std::array<spatial::PointId, 2>>",
        "std::vector<std::array<PointId, 2>>",
    ):
        require(
            forbidden not in combined,
            f"the direct pair implementation contains forbidden token {forbidden!r}",
        )
    for required in (
        "remaining_frontier",
        "maximum_frontier_entry_count",
        "maximum_emitted_record_count",
        "maximum_emitted_point_id_reference_count",
        "ExactPairSupportStreamStatus::budget_exhausted",
        "no_forbidden_global_structure_materialized = true",
        "class ExactPairSupportAuthorityContext",
        "class ExactPairSupportIncrementalVerifier",
        "class PreparedNext",
        "prepare_next(",
        "commit_prepared(",
        "verify_next(",
        "quasi_linear_structure_validation_certified",
        "product_rectangles_are_disjoint",
        "witness_intervals_are_disjoint",
        "std::map<std::uint64_t, ActiveProductRectangle>",
        "manifest_for_with_audit",
        "audit->canonical_cloud_point_hash_count = checked_add",
        "source_checkpoint_already_trusted ||",
        "ExactPairSupportAuthorityContext&&) = delete",
        "spatial::MortonLbvhIndex&&,",
        "spatial::CanonicalPointCloud&&,",
        "ExactPairSupportAuthorityContext authority_;",
        "catch (...) {\n    poison();",
    ):
        require(required in combined, f"missing bounded-stream token {required!r}")

    verifier_start = header.find("class ExactPairSupportIncrementalVerifier")
    verifier_end = header.find(
        "struct ExactPairSupportStreamRunVerification", verifier_start
    )
    require(
        verifier_start >= 0 and verifier_end > verifier_start,
        "cannot isolate the incremental verifier declaration",
    )
    verifier_declaration = header[verifier_start:verifier_end]
    require(
        "std::vector<ExactPairSupportStreamChunk" not in verifier_declaration,
        "the incremental verifier must not retain prior chunks",
    )
    require(
        source.count("manifest_for(") == 2,
        "the cached-manifest path must have only one definition and one cold constructor call",
    )
    require(
        source.count("manifest_for_with_audit(") == 3,
        "the instrumented manifest builder must be called only by its cold wrapper and the authority constructor",
    )
    require(
        "trusted_checkpoint_ = std::move(prepared.trusted_next_);" in source,
        "commit_prepared must advance with the freshly replayed expected checkpoint",
    )
    require(
        "trusted_checkpoint_ = observed.next_checkpoint" not in source,
        "verify_next must never trust an observed checkpoint directly",
    )
    require(
        "std::vector<ExactPairSupportStreamChunk" not in durable_source,
        "the durable sink must not retain decoded chunk history",
    )
    for forbidden_codec_token in (
        "sizeof(ExactPairSupport",
        "std::memcpy",
    ):
        require(
            forbidden_codec_token not in codec_source,
            f"the codec contains raw-layout token {forbidden_codec_token!r}",
        )
    for required_codec_token in (
        "maximum_encoded_byte_count",
        "maximum_frontier_entry_count",
        "maximum_auxiliary_entry_count",
        "maximum_record_count",
        "maximum_point_id_reference_count",
        "maximum_exact_text_byte_count",
        "maximum_total_exact_text_byte_count",
        "ExactPairSupportStreamDecodeDecision",
        "checksum_mismatch",
        "trailing_bytes",
    ):
        require(
            required_codec_token in codec_header + codec_source,
            f"missing bounded-codec token {required_codec_token!r}",
        )
    for required_durable_token in (
        "O_NOFOLLOW",
        "O_NONBLOCK",
        "fdatasync",
        "renameat",
        "fsync(directory.get())",
        "flock",
        "prepare_next(",
        "commit_prepared(",
        "maximum_simultaneously_decoded_chunk_count = 1U",
        "inventory_directory(\n        directory.get(), config.maximum_committed_transition_count)",
        "retained_chunk_history_count = 0U",
        "persistent_top_m_cell_count = 0U",
        "global_gamma_coface_count = 0U",
        "global_gamma_incidence_count = 0U",
        "materialized_pair_arena_count = 0U",
    ):
        require(
            required_durable_token in durable_header + durable_source,
            f"missing durable-publication token {required_durable_token!r}",
        )
    event_start = source.find("struct ProductRectangleSweepEvent")
    event_end = source.find("\n  };", event_start)
    require(
        event_start >= 0 and event_end > event_start,
        "cannot isolate the compact rectangle sweep event",
    )
    event_declaration = source[event_start:event_end]
    require(
        "std::size_t rectangle_index" in event_declaration
        and "ProductRectangle rectangle" not in event_declaration,
        "rectangle sweep events must retain only a compact rectangle index",
    )

    binary_symbol_gate = False
    if binary is not None or nm_executable is not None:
        require(
            binary is not None and nm_executable is not None,
            "--binary and --nm must be supplied together",
        )
        require(binary.is_file(), f"missing pair-stream test binary {binary}")
        require(nm_executable.is_file(), f"missing symbol reader {nm_executable}")
        try:
            completed = subprocess.run(
                [str(nm_executable), "-C", str(binary)],
                check=False,
                capture_output=True,
                text=True,
                timeout=30,
            )
        except (OSError, subprocess.SubprocessError) as error:
            raise ContractError(f"cannot inspect pair binary symbols: {error}") from error
        require(completed.returncode == 0, "the symbol reader rejected the pair binary")
        symbols = completed.stdout.lower()
        for forbidden_symbol in (
            "ordinary_cell",
            "ordinary_diagram",
            "critical_catalog",
            "reduced_gamma",
            "gamma_transition",
            "morse_gamma",
            "depth_zero_natural_support",
        ):
            require(
                forbidden_symbol not in symbols,
                f"the pair binary contains forbidden symbol {forbidden_symbol!r}",
            )
        binary_symbol_gate = True

    return {
        "schema": "morsehgp3d.phase9.pair_support_static.v3",
        "targets": [
            "morsehgp3d_pair_support",
            "morsehgp3d_pair_support_codec",
            "morsehgp3d_pair_support_durable",
        ],
        "persistent_top_m_cell_count": 0,
        "global_gamma_coface_count": 0,
        "global_gamma_incidence_count": 0,
        "materialized_pair_arena_count": 0,
        "persistent_authority_context": True,
        "incremental_verify_next": True,
        "two_phase_durable_verification": True,
        "bounded_canonical_codec": True,
        "unix_atomic_publication": True,
        "quasi_linear_checkpoint_validation": True,
        "binary_symbol_gate": binary_symbol_gate,
        "checked_artifacts": [
            "CMakeLists.txt",
            "include/morsehgp3d/hierarchy/pair_support_stream.hpp",
            "src/cpu/hierarchy/pair_support_stream.cpp",
            "include/morsehgp3d/hierarchy/pair_support_stream_codec.hpp",
            "src/cpu/hierarchy/pair_support_stream_codec.cpp",
            "include/morsehgp3d/hierarchy/pair_support_stream_durable.hpp",
            "src/cpu/hierarchy/pair_support_stream_durable.cpp",
        ],
    }


def main(arguments: list[str]) -> int:
    if len(arguments) not in (2, 6):
        print(
            "usage: check_phase9_pair_support.py PROJECT_SOURCE "
            "[--binary EXECUTABLE --nm NM]",
            file=sys.stderr,
        )
        return 2
    binary = None
    nm_executable = None
    if len(arguments) == 6:
        if arguments[2] != "--binary" or arguments[4] != "--nm":
            print("invalid Phase 9 checker arguments", file=sys.stderr)
            return 2
        binary = Path(arguments[3]).resolve()
        nm_executable = Path(arguments[5]).resolve()
    try:
        result = validate(
            Path(arguments[1]).resolve(), binary, nm_executable
        )
    except ContractError as error:
        print(f"Phase 9 pair-target contract failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, separators=(",", ":"), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
