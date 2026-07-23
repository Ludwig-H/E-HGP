#!/usr/bin/env python3
"""Validate the isolated Phase-10.9 sparse gateway localization target."""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


class ContractError(ValueError):
    """A Phase-10.9 localization invariant was violated."""


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
    target = "morsehgp3d_direct_sparse_gateway_candidate_localization"
    alias = "morsehgp3d::direct_sparse_gateway_candidate_localization"

    target_block = unique_target_block(cmake, "add_library", target)
    require("STATIC" in target_block, "10.9 must remain a static archive")
    require(
        re.findall(r"[A-Za-z0-9_./+-]+\.cpp", target_block)
        == ["src/cpu/hierarchy/direct_sparse_gateway_candidate_localization.cpp"],
        "the 10.9 archive must contain exactly its dedicated source",
    )

    alias_block = unique_target_block(cmake, "add_library", alias)
    require(
        "ALIAS" in alias_block and target in alias_block,
        "the 10.9 public alias must name the isolated archive",
    )

    properties = compact_whitespace(
        unique_target_block(cmake, "set_target_properties", target)
    )
    require_all(
        properties,
        (
            "EXPORT_NAME direct_sparse_gateway_candidate_localization",
            "POSITION_INDEPENDENT_CODE ON",
        ),
        "the 10.9 export properties",
    )

    features = compact_whitespace(
        unique_target_block(cmake, "target_compile_features", target)
    )
    require(
        "PUBLIC cxx_std_20" in features,
        "the 10.9 archive must publish its C++20 requirement",
    )
    includes = unique_target_block(cmake, "target_include_directories", target)
    require_all(
        includes,
        ("BUILD_INTERFACE", "INSTALL_INTERFACE"),
        "the 10.9 include routing",
    )

    links = compact_whitespace(
        unique_target_block(cmake, "target_link_libraries", target)
    )
    require(
        re.fullmatch(
            r"target_link_libraries\s*\(\s*" + target + r"\s+PUBLIC\s+"
            r"morsehgp3d::direct_sparse_gateway_candidate_journal\s+"
            r"morsehgp3d::direct_sparse_positive_facet_locator\s+"
            r"PRIVATE\s+morsehgp3d::sanitizers\s*\)",
            links,
            re.I,
        )
        is not None,
        "the archive must link exactly 10.7 then 10.5a publicly, with "
        "sanitizers private",
    )

    install_blocks = [
        block for block in cmake_blocks(cmake, "install") if "TARGETS" in block
    ]
    install_count = sum(
        len(re.findall(rf"\b{re.escape(target)}\b", block)) for block in install_blocks
    )
    require(install_count == 1, "the 10.9 archive must be installed exactly once")


def validate_test_wiring(unit_cmake: str) -> None:
    test_target = (
        "morsehgp3d_hierarchy_direct_sparse_gateway_candidate_localization_tests"
    )
    public_target = "morsehgp3d::direct_sparse_gateway_candidate_localization"

    executable = unique_target_block(unit_cmake, "add_executable", test_target)
    require(
        re.findall(r"[A-Za-z0-9_./+-]+\.cpp", executable)
        == ["test_hierarchy_direct_sparse_gateway_candidate_localization.cpp"],
        "the 10.9 test executable must contain exactly its dedicated source",
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
        "the 10.9 test must link only through the isolated public target",
    )

    cpu_targets = unique_target_block(unit_cmake, "set", "_morsehgp3d_cpu_test_targets")
    require(
        len(re.findall(rf"\b{re.escape(test_target)}\b", cpu_targets)) == 1,
        "the 10.9 test target must enter the sanitizer-applied CPU list once",
    )

    functional = unique_marked_block(
        unit_cmake,
        "add_test",
        "morsehgp3d.hierarchy_direct_sparse_gateway_candidate_localization",
        "10.9 functional CTest",
    )
    require(
        re.fullmatch(
            r"add_test\s*\(\s*NAME\s+"
            r"morsehgp3d\.hierarchy_direct_sparse_gateway_candidate_localization\s+"
            r"COMMAND\s+" + test_target + r"\s*\)",
            compact_whitespace(functional),
            re.I,
        )
        is not None,
        "the 10.9 functional CTest must run exactly its dedicated executable",
    )

    configuration = unique_marked_block(
        unit_cmake,
        "add_test",
        "morsehgp3d.phase10_direct_sparse_gateway_candidate_localization_configuration",
        "10.9 configuration CTest",
    )
    require(
        "_morsehgp3d_phase10_direct_sparse_gateway_candidate_localization_"
        "configuration_command" in configuration,
        "the 10.9 configuration CTest must use its assembled command",
    )
    require_all(
        unit_cmake,
        (
            "check_phase10_direct_sparse_gateway_candidate_localization.py",
            "$<TARGET_FILE:" "morsehgp3d_direct_sparse_gateway_candidate_localization>",
            "--binary",
            "--nm",
            "CMAKE_NM",
        ),
        "the 10.9 static and symbolic checker wiring",
    )


def validate_public_contract(header: str) -> None:
    compact = compact_whitespace(header)
    require_all(
        compact,
        (
            'direct_sparse_gateway_candidate_localization_backend = "reference_cpu"',
            'direct_sparse_gateway_candidate_localization_profile = "hgp_reduced"',
            'direct_sparse_gateway_candidate_localization_mode = "certified"',
            "direct_sparse_gateway_candidate_localization_refinement_status = "
            '"partial_refinement"',
            "direct_sparse_gateway_candidate_localization_public_status = "
            '"not_claimed"',
        ),
        "the Phase-10.9 status axes",
    )
    require_all(
        header,
        (
            '"morsehgp3d/hierarchy/direct_sparse_gateway_candidate_journal.hpp"',
            '"morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"',
            "maximum_source_candidate_scan_count",
            "maximum_deletion_reference_count",
            "maximum_distinct_facet_count",
            "maximum_facet_key_point_count",
            "maximum_aggregate_slot_visit_count",
            "maximum_aggregate_component_parent_hop_count",
            "maximum_logical_storage_entry_count",
            "ExactDirectSparsePositiveFacetProbeBudget facet_probe_budget",
            "ExactDirectSparseGatewayCandidateDeletionProjection",
            "source_batch_index",
            "ExactDirectSparseGatewayLocalizedFacetToken",
            "latent_unresolved",
            "relative_positive",
            "ExactDirectSparsePositiveFacetLocatorSnapshotStamp",
            "source_gateway_candidate_journal_freshly_replayed",
            "one_locator_probe_per_distinct_full_key",
            "common_frozen_locator_snapshot_certified",
            "no_partial_scientific_payload_published",
            "external_binding_authority_replayed",
            "locator_snapshot_batch_level_alignment_claimed",
            "missing_facet_means_isolated",
            "gateway_attach_published",
            "partial_refinement_only",
            "build_exact_direct_sparse_gateway_candidate_localization(",
            "verify_exact_direct_sparse_gateway_candidate_localization(",
        ),
        "the Phase-10.9 public contract",
    )
    for forbidden in (
        "std::vector<std::vector",
        "std::map<",
        "std::unordered_map",
        "#include <map>",
        "#include <unordered_map>",
        "ExactDirectSparseCofaceKey",
    ):
        require(
            forbidden not in header,
            f"the Phase-10.9 ABI contains forbidden shape {forbidden!r}",
        )
    require(
        re.search(r"std::array\s*<[^;>]*PointId[^;>]*,\s*11U?\s*>", header) is None,
        "the Phase-10.9 ABI must not persist an eleven-PointId coface key",
    )


def validate_implementation(source: str) -> None:
    include_lines = "\n".join(
        line for line in source.splitlines() if line.lstrip().startswith("#include")
    )
    project_includes = re.findall(r'#include\s+"([^"]+)"', include_lines)
    require(
        project_includes
        == ["morsehgp3d/hierarchy/" "direct_sparse_gateway_candidate_localization.hpp"],
        "the isolated 10.9 source must include only its own project header",
    )

    code = strip_cpp_comments_and_literals(source)
    compact = re.sub(r"\s*::\s*", "::", compact_whitespace(code))
    require_all(
        compact,
        (
            "verify_exact_direct_sparse_gateway_candidate_journal(",
            "reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(",
            "locator.certified_positive_locator()",
            "locator.snapshot_stamp()",
            "locator.probe_positive_facet(",
            "ExactDirectSparsePositiveFacetProbeDisposition::unresolved",
            "ExactDirectSparsePositiveFacetProbeDisposition::positive",
            "ExactDirectSparseGatewayFacetLocalizationDisposition::latent_unresolved",
            "ExactDirectSparseGatewayFacetLocalizationDisposition::relative_positive",
            "probe.certified_positive_hit()",
            "probe.certified_unresolved_miss()",
            "probe.certified_budget_exhaustion()",
            "effective_probe_budget",
        ),
        "the Phase-10.9 implementation closure",
    )

    unique_call_sites = (
        ("verify_exact_direct_sparse_gateway_candidate_journal(", 1),
        (
            "reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(",
            1,
        ),
        ("locator.probe_positive_facet(", 1),
        ("probe.certified_positive_hit()", 1),
        ("probe.certified_unresolved_miss()", 1),
        ("probe.certified_budget_exhaustion()", 1),
    )
    for call, expected_count in unique_call_sites:
        require(
            compact.count(call) == expected_count,
            f"the Phase-10.9 implementation must contain exactly "
            f"{expected_count} scientific site for {call!r}",
        )

    require(
        compact.count("locator.certified_positive_locator()") == 2
        and compact.count("index.validated_for(cloud)") == 2
        and compact.count("require_valid_traversal_order(traversal_order)") == 2,
        "the builder and verifier must both retain their locator, LBVH and "
        "traversal-order authority gates",
    )
    require_all(
        compact,
        (
            "locator_query_witness.external_authority_id != "
            "locator.config().external_authority_id",
            "locator_query_witness.external_authority_id == "
            "locator.config().external_authority_id",
            "locator_query_witness.replay_token == 0U",
            "locator_query_witness.replay_token != 0U",
        ),
        "the builder and verifier locator-authority and replay-token binding",
    )

    localization_loop = re.sub(
        r"\s*::\s*",
        "::",
        compact_whitespace(
            unique_cpp_braced_block(
                code,
                "for (auto& token : localized_tokens)",
                "distinct-token localization loop",
            )
        ),
    )
    require(
        localization_loop.count("locator.probe_positive_facet(") == 1
        and localization_loop.count("check_locator_snapshot(locator, result);") == 1,
        "each distinct-token localization iteration must contain exactly one "
        "probe and one post-probe snapshot check",
    )
    require(
        compact.count("check_locator_snapshot(locator, result);") == 3,
        "10.9 must retain the failure, post-probe and final snapshot checks",
    )

    architecture_code = code
    for honest_fact_field in (
        "locator_state_mutated",
        "locator_batch_committed",
        "external_binding_authority_replayed",
        "locator_snapshot_batch_level_alignment_claimed",
        "missing_facet_means_isolated",
        "singleton_component_created",
        "root_union_or_forest_mutated",
        "gateway_attach_published",
        "eleven_point_coface_keys_materialized",
        "gamma_cells_or_higher_order_delaunay_materialized",
        "forbidden_global_structure_materialized",
    ):
        architecture_code = architecture_code.replace(honest_fact_field, " ")

    forbidden_patterns = (
        (
            r"\bstd\s*::\s*(?:map|multimap|unordered_map|unordered_multimap)\s*<",
            "associative map",
        ),
        (r"\bstd\s*::\s*vector\s*<\s*std\s*::\s*vector\s*<", "nested arena"),
        (r"\bExactDirectSparseCofaceKey\b", "persistent coface key"),
        (
            r"std\s*::\s*array\s*<[^;>]*PointId[^;>]*,\s*11U?\s*>",
            "persistent or duplicate eleven-point scratch",
        ),
        (r"\bapply_batch\s*\(", "locator mutation"),
        (
            r"\bbuild_exact_direct_sparse_positive_facet_locator\s*\(",
            "replacement locator",
        ),
        (
            r"\bbuild_exact_direct_sparse_first_incidence\s*\(",
            "direct 10.6 bypass",
        ),
        (r"\bbuild_exact_facet_miniball\s*\(", "local miniball rebuild"),
        (r"\blbvh_(?:top_k|closed_ball)[A-Za-z0-9_]*\s*\(", "direct LBVH query"),
        (
            r"\bbuild_exact_direct_frozen_root_quotient\s*\(",
            "frozen-root quotient",
        ),
        (r"\bbuild_exact_direct_k1_forest[A-Za-z0-9_]*\s*\(", "K1 forest"),
        (
            r"\b(?:DisjointSetUnion|UnionFind|disjoint_set|union_find)\b",
            "union-find structure",
        ),
        (
            r"\b(?:GatewayAttach|gateway_attach|RootBirth|root_birth|"
            r"AtomicUnionBatch|atomic_union_batch)\b",
            "root, union or attachment action",
        ),
        (
            r"\b(?:build|verify|compute)[A-Za-z0-9_]*(?:gamma|delaunay|"
            r"cell_atlas|critical_catalog)\s*\(",
            "global geometry builder",
        ),
        (r"\b(?:ordinary|power)_?cell\b", "global cell"),
        (r"\b(?:facet|coface)_?catalog\b", "global facet/coface catalog"),
    )
    for pattern, label in forbidden_patterns:
        require(
            re.search(pattern, architecture_code, re.I) is None,
            f"the isolated 10.9 implementation contains forbidden {label}",
        )


def validate_symbols(binary: Path, nm: Path) -> None:
    require(binary.is_file(), f"the isolated 10.9 archive does not exist: {binary}")
    require(nm.is_file(), f"the nm executable does not exist: {nm}")
    try:
        symbols = subprocess.run(
            [str(nm), "-C", str(binary)],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.lower()
    except (OSError, subprocess.CalledProcessError) as error:
        raise ContractError(f"cannot inspect the 10.9 archive: {error}") from error

    require_all(
        symbols,
        (
            "build_exact_direct_sparse_gateway_candidate_localization",
            "verify_exact_direct_sparse_gateway_candidate_localization",
            "exactdirectsparsegatewaycandidatelocalizationresult::"
            "certified_partial_refinement",
            "exactdirectsparsegatewaycandidatelocalizationresult::"
            "certified_atomic_failure",
            "exactdirectsparsegatewaycandidatelocalizationresult::certified_outcome",
            "verify_exact_direct_sparse_gateway_candidate_journal",
            "reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet",
            "exactdirectsparsepositivefacetlocator::certified_positive_locator",
            "exactdirectsparsepositivefacetlocator::snapshot_stamp",
            "exactdirectsparsepositivefacetlocator::probe_positive_facet",
        ),
        "the isolated 10.9 symbol closure",
    )
    for forbidden in (
        "exactdirectsparsepositivefacetlocator::apply_batch",
        "build_exact_direct_sparse_positive_facet_locator",
        "build_exact_direct_sparse_first_incidence",
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
        "unionfind",
        "union_find",
        "disjointset",
        "disjoint_set",
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
        "direct_sparse_gateway_candidate_localization.hpp"
    )
    source = read_text(
        project / "src/cpu/hierarchy/direct_sparse_gateway_candidate_localization.cpp"
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
        print(f"Phase-10.9-RCPU gateway localization contract failed: {error}")
        return 1
    print("Phase-10.9-RCPU gateway localization contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
