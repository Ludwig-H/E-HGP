#!/usr/bin/env python3
"""Validate the benchmark-only exact external-1NN work-profile contract."""

from __future__ import annotations

import argparse
import copy
from fractions import Fraction
import json
from pathlib import Path
import re
import subprocess
import sys
from typing import Any, NoReturn, Sequence

SCHEMA = "morsehgp3d.phase5.k1_boruvka_exact_search_work_profile.v1"
COMPARISON_SCHEMA = "morsehgp3d.phase5.k1_boruvka_exact_search_work_profile.v2"
STATIC_SCHEMA = "morsehgp3d.phase5.k1_boruvka_exact_search_work_profile_static.v1"
SCOPE = "certified_local_emst_chain_work_profile_without_scalability_claim"
WORK_ACCOUNTING = "producer_chain_only_replay_and_reference_excluded"
TOP_KEYS = {
    "artifact_kind",
    "artifact_role",
    "backend",
    "candidate_record_count",
    "decision_backend",
    "generator",
    "git",
    "hierarchy_reduction_status",
    "mode",
    "phase",
    "point_count",
    "policy",
    "profile",
    "qualification_claimed",
    "reference_backend",
    "replay_backend",
    "result",
    "scalability_claimed",
    "schema",
    "scientific_public_status",
    "scientific_result_claimed",
    "scope",
    "status",
    "work_accounting",
}
RESULT_KEYS = {
    "certificates",
    "component_count_path",
    "exact_weights",
    "rounds",
    "totals",
}
COMPARISON_RESULT_KEYS = RESULT_KEYS | {"resolver_comparison"}
RESOLVER_COMPARISON_KEYS = {"rounds", "totals"}
RESOLVER_COMPARISON_ROUND_KEYS = {
    "direct",
    "dynamic",
    "post_component_count",
    "pre_component_count",
    "round_index",
    "unordered_point_pair_count",
}
RESOLVER_COMPARISON_TOTAL_KEYS = {"direct", "dynamic"}
RESOLVER_COMMON_KEYS = {
    "aabb_pair_bound_evaluation_count",
    "frontier_peak",
    "node_pair_expansion_count",
    "node_pair_visit_count",
    "point_pair_distance_evaluation_count",
    "strict_aabb_pair_prune_count",
    "uniform_same_component_pair_prune_count",
}
RESOLVER_DYNAMIC_KEYS = RESOLVER_COMMON_KEYS | {
    "ancestor_update_count",
    "strict_incumbent_decrease_count",
}
RESOLVER_DIRECT_KEYS = RESOLVER_COMMON_KEYS | {
    "component_cutoff_upper_envelope_node_count",
    "component_kappa_update_count",
    "strict_component_cutoff_decrease_count",
    "target_component_seed_kappa_update_count",
    "target_component_seed_offer_count",
    "target_component_seed_strict_cutoff_decrease_count",
}
CERTIFICATE_KEYS = {
    "bounded_morton_seed_chain",
    "canonical_contraction_chain",
    "cpu_exact_decision_chain",
    "exact_external_1nn_chain",
    "fresh_replay",
    "local_emst_witness",
    "reference_cpu_witness",
}
ROUND_KEYS = {
    "accepted_edge_count",
    "component_minimum_count",
    "exact_operation_count_unweighted",
    "exact_search",
    "morton_seed",
    "persistent_point_minimum_count",
    "post_component_count",
    "pre_component_count",
    "round_index",
}
TOTAL_KEYS = {
    "accepted_edge_count",
    "component_minimum_count",
    "exact_operation_count_unweighted",
    "exact_search",
    "morton_seed",
    "persistent_point_minimum_count",
    "round_count",
}
SEED_KEYS = {
    "exact_fallback_count",
    "exact_seed_distance_evaluation_count",
    "exact_selected_proposal_count",
    "exact_strict_improvement_count",
    "external_neighbor_count",
    "floating_proposal_count",
    "inspected_neighbor_count",
    "source_count",
}
SEARCH_KEYS = {
    "aabb_bound_evaluation_count",
    "frontier_peak_per_source",
    "internal_node_expansion_count",
    "node_visit_count",
    "node_visit_peak_per_source",
    "point_distance_evaluation_count",
    "point_distance_evaluation_peak_per_source",
    "point_query_count",
    "seed_leaf_distance_reuse_count",
    "strict_aabb_prune_count",
    "uniform_component_prune_count",
}
SEARCH_PEAK_KEYS = {
    "frontier_peak_per_source",
    "node_visit_peak_per_source",
    "point_distance_evaluation_peak_per_source",
}
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
MAXIMUM_PROFILE_POINT_COUNT = 1_000_000


class ContractError(ValueError):
    """The work profile escaped its closed benchmark-only contract."""


def fail(message: str) -> NoReturn:
    raise ContractError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def reject_duplicate_json_keys(
    pairs: list[tuple[str, Any]],
) -> dict[str, Any]:
    value: dict[str, Any] = {}
    for key, item in pairs:
        if key in value:
            fail(f"duplicate JSON key: {key}")
        value[key] = item
    return value


def reject_non_finite_json_constant(value: str) -> NoReturn:
    fail(f"non-finite JSON constant: {value}")


def exact_keys(value: Any, expected: set[str], label: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{label} must be an object")
    require(set(value) == expected, f"{label} fields differ")
    return value


def natural(value: Any, label: str, *, positive: bool = False) -> int:
    require(type(value) is int and value >= 0, f"{label} must be a natural")
    require(not positive or value > 0, f"{label} must be positive")
    return value


def validate_level(value: Any, label: str) -> Fraction:
    level = exact_keys(value, {"denominator", "numerator"}, label)
    numerator_text = level["numerator"]
    denominator_text = level["denominator"]
    require(
        isinstance(numerator_text, str)
        and re.fullmatch(r"[1-9][0-9]*", numerator_text) is not None,
        f"{label}.numerator must be a canonical positive integer string",
    )
    require(
        isinstance(denominator_text, str)
        and re.fullmatch(r"[1-9][0-9]*", denominator_text) is not None,
        f"{label}.denominator must be a canonical positive integer string",
    )
    # Exact coordinates come from binary64, so genuine profile weights remain
    # comfortably below this anti-resource-exhaustion limit.
    require(
        len(numerator_text) <= 4096 and len(denominator_text) <= 4096,
        f"{label} is unreasonably large",
    )
    numerator = int(numerator_text)
    denominator = int(denominator_text)
    result = Fraction(numerator, denominator)
    require(
        result.numerator == numerator and result.denominator == denominator,
        f"{label} must be reduced",
    )
    return result


def validate_seed(
    value: Any,
    label: str,
    *,
    point_count: int,
    radius: int,
) -> dict[str, int]:
    seed = exact_keys(value, SEED_KEYS, label)
    result = {key: natural(item, f"{label}.{key}") for key, item in seed.items()}
    effective_radius = min(point_count - 1, radius)
    expected_inspections = effective_radius * (2 * point_count - effective_radius - 1)
    require(result["source_count"] == point_count, f"{label} source coverage differs")
    require(
        result["inspected_neighbor_count"] == expected_inspections,
        f"{label} Morton inspection identity differs",
    )
    require(
        result["external_neighbor_count"] <= result["inspected_neighbor_count"],
        f"{label} external count exceeds inspections",
    )
    require(
        result["floating_proposal_count"]
        <= min(point_count, result["external_neighbor_count"]),
        f"{label} floating proposal bound differs",
    )
    require(
        result["exact_strict_improvement_count"]
        <= result["exact_selected_proposal_count"]
        <= result["floating_proposal_count"],
        f"{label} exact proposal ordering differs",
    )
    require(
        result["exact_fallback_count"] + result["exact_selected_proposal_count"]
        == point_count,
        f"{label} fallback partition differs",
    )
    extra_evaluations = result["exact_seed_distance_evaluation_count"] - point_count
    require(
        0 <= extra_evaluations <= result["floating_proposal_count"],
        f"{label} exact seed evaluation bound differs",
    )
    require(
        result["exact_selected_proposal_count"] <= extra_evaluations,
        f"{label} selected proposals exceed recertified proposals",
    )
    return result


def validate_search(
    value: Any,
    label: str,
    *,
    point_count: int,
) -> dict[str, int]:
    search = exact_keys(value, SEARCH_KEYS, label)
    result = {key: natural(item, f"{label}.{key}") for key, item in search.items()}
    node_count = 2 * point_count - 1
    maximum_all_queries = point_count * node_count
    maximum_internal = point_count * (point_count - 1)
    maximum_point_distances = point_count * (point_count - 1)
    require(
        result["point_query_count"] == point_count, f"{label} query coverage differs"
    )
    require(
        result["aabb_bound_evaluation_count"] == result["node_visit_count"],
        f"{label} AABB/visit identity differs",
    )
    require(
        result["node_visit_count"]
        == result["strict_aabb_prune_count"]
        + result["internal_node_expansion_count"]
        + result["point_distance_evaluation_count"]
        + result["seed_leaf_distance_reuse_count"],
        f"{label} visited-node partition differs",
    )
    require(
        point_count <= result["node_visit_count"] <= maximum_all_queries
        and 0 < result["node_visit_peak_per_source"] <= node_count
        and result["node_visit_peak_per_source"] <= result["node_visit_count"],
        f"{label} node-visit bounds differ",
    )
    require(
        0 < result["frontier_peak_per_source"] <= node_count
        and result["frontier_peak_per_source"] <= result["node_visit_count"],
        f"{label} frontier bounds differ",
    )
    require(
        result["point_distance_evaluation_peak_per_source"]
        <= min(point_count - 1, result["point_distance_evaluation_count"])
        and result["point_distance_evaluation_count"] <= maximum_point_distances,
        f"{label} exact point-distance bounds differ",
    )
    require(
        result["internal_node_expansion_count"] <= maximum_internal,
        f"{label} internal expansion bound differs",
    )
    require(
        result["uniform_component_prune_count"]
        <= 2 * result["internal_node_expansion_count"],
        f"{label} uniform-component prune bound differs",
    )
    require(
        result["strict_aabb_prune_count"] <= result["node_visit_count"],
        f"{label} strict prune bound differs",
    )
    require(
        result["seed_leaf_distance_reuse_count"] <= point_count,
        f"{label} seed-leaf reuse bound differs",
    )
    return result


def validate_document(value: Any, *, expected_backend: str) -> None:
    document = exact_keys(value, TOP_KEYS, "work profile")
    require(
        expected_backend in {"fake_gpu", "cuda_g4"},
        "expected backend is outside the work-profile contract",
    )
    expected_scalars = {
        "artifact_kind": "work_profile",
        "artifact_role": "benchmark_only",
        "backend": expected_backend,
        "decision_backend": "reference_cpu",
        "hierarchy_reduction_status": "not_performed",
        "mode": "benchmark",
        "phase": "5",
        "profile": "hgp_reduced",
        "reference_backend": "reference_cpu",
        "replay_backend": expected_backend,
        "schema": SCHEMA,
        "scope": SCOPE,
        "status": "measured",
        "work_accounting": WORK_ACCOUNTING,
    }
    for key, expected in expected_scalars.items():
        require(document[key] == expected, f"work profile {key} differs")
    require(
        natural(document["candidate_record_count"], "candidate_record_count") == 0,
        "candidate records must not be persisted",
    )
    for key in (
        "qualification_claimed",
        "scalability_claimed",
        "scientific_result_claimed",
    ):
        require(document[key] is False, f"work profile {key} must be false")
    require(
        document["scientific_public_status"] is None,
        "scientific_public_status must be null",
    )

    point_count = natural(document["point_count"], "point_count", positive=True)
    require(
        2 <= point_count <= MAXIMUM_PROFILE_POINT_COUNT,
        "point_count escapes the injective generator domain",
    )
    policy = exact_keys(document["policy"], {"window_radius"}, "policy")
    radius = natural(policy["window_radius"], "policy.window_radius", positive=True)
    require(radius <= sys.maxsize // 2, "window radius has no finite 2W bound")
    generator = exact_keys(
        document["generator"], {"algorithm", "family", "seed"}, "generator"
    )
    require(
        generator["algorithm"] == "deterministic_dyadic_v1",
        "generator algorithm differs",
    )
    require(
        generator["family"] in {"uniform", "clusters", "lattice"},
        "generator family differs",
    )
    seed = natural(generator["seed"], "generator.seed")
    require(seed <= (1 << 64) - 1, "generator seed exceeds uint64")
    git = exact_keys(document["git"], {"sha"}, "git")
    require(
        isinstance(git["sha"], str) and SHA_RE.fullmatch(git["sha"]) is not None,
        "git SHA differs",
    )

    result = exact_keys(document["result"], RESULT_KEYS, "result")
    certificates = exact_keys(
        result["certificates"], CERTIFICATE_KEYS, "result.certificates"
    )
    require(
        all(certificates[key] is True for key in CERTIFICATE_KEYS),
        "result certificates must all be true",
    )
    path_value = result["component_count_path"]
    rounds_value = result["rounds"]
    require(
        isinstance(path_value, list) and isinstance(rounds_value, list),
        "result paths and rounds must be arrays",
    )
    path = [natural(item, "component path item", positive=True) for item in path_value]
    require(
        len(path) == len(rounds_value) + 1,
        "component path length differs",
    )
    require(path[0] == point_count and path[-1] == 1, "component path endpoints differ")
    maximum_round_count = (point_count - 1).bit_length()
    require(
        1 <= len(rounds_value) <= maximum_round_count,
        "round count escapes the halving bound",
    )
    weights = exact_keys(
        result["exact_weights"], {"hgp", "squared"}, "result.exact_weights"
    )
    hgp_weight = validate_level(weights["hgp"], "result.exact_weights.hgp")
    squared_weight = validate_level(weights["squared"], "result.exact_weights.squared")
    require(4 * hgp_weight == squared_weight, "HGP weight is not squared/4")

    scalar_sums = {
        "accepted_edge_count": 0,
        "component_minimum_count": 0,
        "exact_operation_count_unweighted": 0,
        "persistent_point_minimum_count": 0,
    }
    seed_sums = {key: 0 for key in SEED_KEYS}
    search_sums = {key: 0 for key in SEARCH_KEYS - SEARCH_PEAK_KEYS}
    search_peaks = {key: 0 for key in SEARCH_PEAK_KEYS}
    for index, round_item in enumerate(rounds_value):
        round_value = exact_keys(round_item, ROUND_KEYS, f"rounds[{index}]")
        pre = natural(
            round_value["pre_component_count"], f"rounds[{index}].pre", positive=True
        )
        post = natural(
            round_value["post_component_count"], f"rounds[{index}].post", positive=True
        )
        require(
            natural(round_value["round_index"], f"rounds[{index}].round_index")
            == index,
            f"rounds[{index}] index differs",
        )
        require(
            pre == path[index] and post == path[index + 1],
            f"rounds[{index}] path differs",
        )
        require(0 < post <= pre // 2, f"rounds[{index}] contraction bound differs")
        accepted = natural(
            round_value["accepted_edge_count"],
            f"rounds[{index}].accepted_edge_count",
        )
        component_minima = natural(
            round_value["component_minimum_count"],
            f"rounds[{index}].component_minimum_count",
        )
        persistent = natural(
            round_value["persistent_point_minimum_count"],
            f"rounds[{index}].persistent_point_minimum_count",
        )
        require(accepted == pre - post, f"rounds[{index}] accepted edges differ")
        require(component_minima == pre, f"rounds[{index}] component minima differ")
        require(persistent == 0, f"rounds[{index}] persisted point minima")
        seed = validate_seed(
            round_value["morton_seed"],
            f"rounds[{index}].morton_seed",
            point_count=point_count,
            radius=radius,
        )
        search = validate_search(
            round_value["exact_search"],
            f"rounds[{index}].exact_search",
            point_count=point_count,
        )
        operation_count = natural(
            round_value["exact_operation_count_unweighted"],
            f"rounds[{index}].exact_operation_count_unweighted",
        )
        require(
            operation_count
            == seed["exact_seed_distance_evaluation_count"]
            + search["aabb_bound_evaluation_count"]
            + search["point_distance_evaluation_count"],
            f"rounds[{index}] exact-operation identity differs",
        )
        for key, item in {
            "accepted_edge_count": accepted,
            "component_minimum_count": component_minima,
            "exact_operation_count_unweighted": operation_count,
            "persistent_point_minimum_count": persistent,
        }.items():
            scalar_sums[key] += item
        for key in SEED_KEYS:
            seed_sums[key] += seed[key]
        for key in SEARCH_KEYS - SEARCH_PEAK_KEYS:
            search_sums[key] += search[key]
        for key in SEARCH_PEAK_KEYS:
            search_peaks[key] = max(search_peaks[key], search[key])

    totals = exact_keys(result["totals"], TOTAL_KEYS, "result.totals")
    require(
        natural(totals["round_count"], "totals.round_count") == len(rounds_value),
        "total round count differs",
    )
    for key, expected in scalar_sums.items():
        require(
            natural(totals[key], f"totals.{key}") == expected,
            f"total {key} differs",
        )
    require(
        scalar_sums["accepted_edge_count"] == point_count - 1,
        "accepted-edge chain does not form a tree",
    )
    require(
        scalar_sums["component_minimum_count"] == sum(path[:-1])
        and scalar_sums["component_minimum_count"] < 2 * point_count,
        "component-minimum chain does not close",
    )
    total_seed = exact_keys(totals["morton_seed"], SEED_KEYS, "totals.morton_seed")
    for key, expected in seed_sums.items():
        require(
            natural(total_seed[key], f"totals.morton_seed.{key}") == expected,
            f"total Morton seed {key} differs",
        )
    total_search = exact_keys(
        totals["exact_search"], SEARCH_KEYS, "totals.exact_search"
    )
    for key, expected in search_sums.items():
        require(
            natural(total_search[key], f"totals.exact_search.{key}") == expected,
            f"total exact search {key} differs",
        )
    for key, expected in search_peaks.items():
        require(
            natural(total_search[key], f"totals.exact_search.{key}") == expected,
            f"total exact search peak {key} differs",
        )


def validate_resolver_counters(
    value: Any,
    label: str,
    *,
    point_count: int,
    direct: bool,
) -> dict[str, int]:
    expected_keys = RESOLVER_DIRECT_KEYS if direct else RESOLVER_DYNAMIC_KEYS
    counters = exact_keys(value, expected_keys, label)
    result = {
        key: natural(item, f"{label}.{key}") for key, item in counters.items()
    }
    visits = result["node_pair_visit_count"]
    expansions = result["node_pair_expansion_count"]
    distances = result["point_pair_distance_evaluation_count"]
    uniform_prunes = result["uniform_same_component_pair_prune_count"]
    strict_prunes = result["strict_aabb_pair_prune_count"]
    maximum_visits = point_count * (point_count + 1) - 1
    maximum_frontier = 2 * point_count - 1
    maximum_distances = point_count * (point_count - 1) // 2
    require(
        result["aabb_pair_bound_evaluation_count"] == visits,
        f"{label} AABB/visit identity differs",
    )
    require(
        visits == expansions + uniform_prunes + strict_prunes + distances,
        f"{label} visited-pair partition differs",
    )
    require(
        2 * expansions + 2 <= visits <= 3 * expansions + 1,
        f"{label} expansion arity differs",
    )
    require(
        0 < visits <= maximum_visits,
        f"{label} node-pair visit bound differs",
    )
    require(
        0 < result["frontier_peak"] <= maximum_frontier
        and result["frontier_peak"] <= visits,
        f"{label} frontier bound differs",
    )
    require(
        distances <= maximum_distances,
        f"{label} point-pair distance bound differs",
    )
    if direct:
        require(
            result["component_kappa_update_count"] <= 2 * distances,
            f"{label} component update bound differs",
        )
        require(
            result["strict_component_cutoff_decrease_count"]
            <= result["component_kappa_update_count"],
            f"{label} strict component decrease bound differs",
        )
        require(
            result["target_component_seed_offer_count"] == point_count,
            f"{label} target seed offer coverage differs",
        )
        require(
            result["target_component_seed_kappa_update_count"] <= point_count,
            f"{label} target seed update bound differs",
        )
        require(
            result["target_component_seed_strict_cutoff_decrease_count"]
            <= result["target_component_seed_kappa_update_count"],
            f"{label} target seed strict decrease bound differs",
        )
        require(
            result["component_cutoff_upper_envelope_node_count"]
            == 2 * point_count - 1,
            f"{label} component envelope coverage differs",
        )
    else:
        strict_decreases = result["strict_incumbent_decrease_count"]
        require(
            strict_decreases <= 2 * distances,
            f"{label} strict incumbent decrease bound differs",
        )
        require(
            result["ancestor_update_count"]
            <= strict_decreases * (point_count - 1),
            f"{label} ancestor update bound differs",
        )
    return result


def validate_comparison_document(value: Any, *, expected_backend: str) -> None:
    document = exact_keys(value, TOP_KEYS, "comparison work profile")
    require(
        document["schema"] == COMPARISON_SCHEMA,
        "comparison work profile schema differs",
    )
    result = exact_keys(
        document["result"], COMPARISON_RESULT_KEYS, "comparison result"
    )

    # Projecting away the comparison extension must recover the complete v1
    # contract. This keeps the benchmark-only G4 artifact path unchanged.
    projected = copy.deepcopy(document)
    projected["schema"] = SCHEMA
    del projected["result"]["resolver_comparison"]
    validate_document(projected, expected_backend=expected_backend)

    point_count = natural(document["point_count"], "point_count", positive=True)
    comparison = exact_keys(
        result["resolver_comparison"],
        RESOLVER_COMPARISON_KEYS,
        "result.resolver_comparison",
    )
    rounds = comparison["rounds"]
    base_rounds = result["rounds"]
    require(
        isinstance(rounds, list) and len(rounds) == len(base_rounds),
        "resolver comparison round coverage differs",
    )
    dynamic_sums = {key: 0 for key in RESOLVER_DYNAMIC_KEYS}
    direct_sums = {key: 0 for key in RESOLVER_DIRECT_KEYS}
    for index, round_item in enumerate(rounds):
        round_value = exact_keys(
            round_item,
            RESOLVER_COMPARISON_ROUND_KEYS,
            f"resolver_comparison.rounds[{index}]",
        )
        label = f"resolver_comparison.rounds[{index}]"
        require(
            natural(round_value["round_index"], f"{label}.round_index") == index,
            f"{label} index differs",
        )
        require(
            natural(
                round_value["pre_component_count"],
                f"{label}.pre_component_count",
                positive=True,
            )
            == base_rounds[index]["pre_component_count"]
            and natural(
                round_value["post_component_count"],
                f"{label}.post_component_count",
                positive=True,
            )
            == base_rounds[index]["post_component_count"],
            f"{label} component path differs from v1",
        )
        require(
            natural(
                round_value["unordered_point_pair_count"],
                f"{label}.unordered_point_pair_count",
            )
            == point_count * (point_count - 1) // 2,
            f"{label} unordered point-pair coverage differs",
        )
        dynamic = validate_resolver_counters(
            round_value["dynamic"],
            f"{label}.dynamic",
            point_count=point_count,
            direct=False,
        )
        direct = validate_resolver_counters(
            round_value["direct"],
            f"{label}.direct",
            point_count=point_count,
            direct=True,
        )
        for key in RESOLVER_DYNAMIC_KEYS:
            if key == "frontier_peak":
                dynamic_sums[key] = max(dynamic_sums[key], dynamic[key])
            else:
                dynamic_sums[key] += dynamic[key]
        for key in RESOLVER_DIRECT_KEYS:
            if key == "frontier_peak":
                direct_sums[key] = max(direct_sums[key], direct[key])
            else:
                direct_sums[key] += direct[key]

    totals = exact_keys(
        comparison["totals"],
        RESOLVER_COMPARISON_TOTAL_KEYS,
        "result.resolver_comparison.totals",
    )
    total_dynamic = exact_keys(
        totals["dynamic"],
        RESOLVER_DYNAMIC_KEYS,
        "result.resolver_comparison.totals.dynamic",
    )
    total_direct = exact_keys(
        totals["direct"],
        RESOLVER_DIRECT_KEYS,
        "result.resolver_comparison.totals.direct",
    )
    for key, expected in dynamic_sums.items():
        require(
            natural(total_dynamic[key], f"totals.dynamic.{key}") == expected,
            f"total dynamic resolver {key} differs",
        )
    for key, expected in direct_sums.items():
        require(
            natural(total_direct[key], f"totals.direct.{key}") == expected,
            f"total direct resolver {key} differs",
        )


def read_profile_log(path: Path) -> dict[str, Any]:
    require(
        path.is_file() and not path.is_symlink(), "profile log must be a regular file"
    )
    raw = path.read_text(encoding="utf-8")
    try:
        value = json.loads(
            raw,
            object_pairs_hook=reject_duplicate_json_keys,
            parse_constant=reject_non_finite_json_constant,
        )
    except (json.JSONDecodeError, UnicodeError, OSError) as error:
        fail(f"cannot read profile log: {error}")
    require(isinstance(value, dict), "profile log must contain a JSON object")
    canonical = json.dumps(
        value, ensure_ascii=True, sort_keys=True, separators=(",", ":")
    )
    require(raw == canonical + "\n", "profile log must be canonical one-line JSON")
    return value


def validate_expected_identity(value: dict[str, Any], args: argparse.Namespace) -> None:
    validate_document(value, expected_backend=args.expected_backend)
    require(value["git"] == {"sha": args.git_sha}, "profile log Git SHA differs")
    require(value["generator"]["family"] == args.family, "profile log family differs")
    require(value["generator"]["seed"] == args.seed, "profile log seed differs")
    require(value["point_count"] == args.point_count, "profile log point count differs")
    require(
        value["policy"] == {"window_radius": args.window_radius},
        "profile log window radius differs",
    )


def static_check(project: Path, executable: Path | None) -> tuple[bool, int]:
    source = (
        project / "src/tools/gpu_k1_boruvka_exact_search_work_profile.cpp"
    ).read_text(encoding="utf-8")
    cmake = (project / "CMakeLists.txt").read_text(encoding="utf-8")
    unit_cmake = (project / "tests/unit/CMakeLists.txt").read_text(encoding="utf-8")
    assembler = (
        project / "tests/cuda/assemble_phase5_k1_boruvka_exact_search_work_profile.py"
    ).read_text(encoding="utf-8")
    for token in (
        SCHEMA,
        COMPARISON_SCHEMA,
        SCOPE,
        "--compare-resolvers",
        "build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka",
        "exact_operation_count_unweighted",
        "candidate_record_count",
        "qualification_claimed",
        "scalability_claimed",
    ):
        require(token in source, f"source token missing: {token}")
    for token in (
        "morsehgp3d_gpu_k1_boruvka_exact_search_work_profile",
        'MORSEHGP3D_EXACT_SEARCH_PROFILE_BACKEND="cuda_g4"',
    ):
        require(token in cmake, f"CUDA CMake token missing: {token}")
    for token in (
        "morsehgp3d_gpu_k1_boruvka_exact_search_work_profile_host",
        "MORSEHGP3D_EXACT_SEARCH_PROFILE_BACKEND",
        "fake_gpu",
    ):
        require(token in unit_cmake, f"host CMake token missing: {token}")
    for token in (
        "WORK_PROFILE_SCHEMA",
        "morsehgp3d.phase5.k1_boruvka_exact_search_work_profile_artifact.v1",
        "validate_document",
        "benchmark_only",
        "worker_passed_pending_shutdown",
    ):
        require(token in assembler, f"assembler token missing: {token}")

    if executable is None:
        return False, 0
    completed = subprocess.run(
        [
            str(executable.resolve()),
            "--family",
            "uniform",
            "--point-count",
            "8",
            "--window-radius",
            "2",
            "--seed",
            "1",
            "--git-sha",
            "0" * 40,
        ],
        check=True,
        capture_output=True,
        text=True,
        timeout=30,
    )
    require(not completed.stderr, "host work profile wrote stderr")
    require(
        completed.stdout.endswith("\n") and completed.stdout.count("\n") == 1,
        "host output is not one-line JSON",
    )
    value = json.loads(
        completed.stdout,
        object_pairs_hook=reject_duplicate_json_keys,
        parse_constant=reject_non_finite_json_constant,
    )
    validate_document(value, expected_backend="fake_gpu")

    comparison_completed = subprocess.run(
        [
            str(executable.resolve()),
            "--family",
            "uniform",
            "--point-count",
            "8",
            "--window-radius",
            "2",
            "--seed",
            "1",
            "--git-sha",
            "0" * 40,
            "--compare-resolvers",
        ],
        check=True,
        capture_output=True,
        text=True,
        timeout=30,
    )
    require(not comparison_completed.stderr, "resolver comparison wrote stderr")
    require(
        comparison_completed.stdout.endswith("\n")
        and comparison_completed.stdout.count("\n") == 1,
        "resolver comparison output is not one-line JSON",
    )
    comparison_value = json.loads(
        comparison_completed.stdout,
        object_pairs_hook=reject_duplicate_json_keys,
        parse_constant=reject_non_finite_json_constant,
    )
    validate_comparison_document(comparison_value, expected_backend="fake_gpu")
    comparison_rounds = comparison_value["result"]["resolver_comparison"][
        "rounds"
    ]
    require(
        len(comparison_rounds) == 2,
        "uniform resolver comparison no longer exercises multiple rounds",
    )
    fixture_keys = (
        "node_pair_visit_count",
        "node_pair_expansion_count",
        "point_pair_distance_evaluation_count",
        "strict_aabb_pair_prune_count",
        "uniform_same_component_pair_prune_count",
        "frontier_peak",
    )
    expected_work = {
        "direct": ((40, 16, 6, 10, 8, 7), (27, 11, 2, 7, 7, 5)),
        "dynamic": ((40, 16, 6, 10, 8, 7), (47, 21, 9, 10, 7, 5)),
    }
    for resolver in ("dynamic", "direct"):
        for round_index, expected in enumerate(expected_work[resolver]):
            observed = comparison_rounds[round_index][resolver]
            require(
                tuple(observed[key] for key in fixture_keys) == expected,
                f"uniform {resolver} resolver work fixture differs",
            )
    require(
        tuple(
            round_value["direct"]["component_kappa_update_count"]
            for round_value in comparison_rounds
        )
        == (1, 0)
        and tuple(
            round_value["dynamic"]["strict_incumbent_decrease_count"]
            for round_value in comparison_rounds
        )
        == (1, 3)
        and tuple(
            round_value["dynamic"]["ancestor_update_count"]
            for round_value in comparison_rounds
        )
        == (2, 6),
        "uniform resolver-specific work fixture differs",
    )
    comparison_projection = copy.deepcopy(comparison_value)
    comparison_projection["schema"] = SCHEMA
    del comparison_projection["result"]["resolver_comparison"]
    require(
        comparison_projection == value,
        "resolver comparison changed the projected v1 work profile",
    )

    mutations: list[dict[str, Any]] = []
    for key, replacement in (
        ("qualification_claimed", True),
        ("scalability_claimed", True),
        ("candidate_record_count", 1),
        ("hierarchy_reduction_status", "performed"),
    ):
        mutated = copy.deepcopy(value)
        mutated[key] = replacement
        mutations.append(mutated)
    mutated = copy.deepcopy(value)
    mutated["public_status"] = "exact"
    mutations.append(mutated)
    mutated = copy.deepcopy(value)
    mutated["result"]["certificates"]["fresh_replay"] = False
    mutations.append(mutated)
    mutated = copy.deepcopy(value)
    mutated["result"]["component_count_path"][1] += 1
    mutations.append(mutated)
    mutated = copy.deepcopy(value)
    mutated["result"]["rounds"][0]["morton_seed"]["inspected_neighbor_count"] -= 1
    mutations.append(mutated)
    mutated = copy.deepcopy(value)
    mutated["result"]["rounds"][0]["exact_search"]["node_visit_count"] += 1
    mutations.append(mutated)
    mutated = copy.deepcopy(value)
    mutated["result"]["rounds"][0]["exact_operation_count_unweighted"] += 1
    mutations.append(mutated)
    mutated = copy.deepcopy(value)
    mutated["result"]["exact_weights"]["hgp"]["numerator"] = "1"
    mutations.append(mutated)
    mutated = copy.deepcopy(value)
    mutated["result"]["totals"]["persistent_point_minimum_count"] = 1
    mutations.append(mutated)

    comparison_mutations: list[dict[str, Any]] = []
    mutated = copy.deepcopy(comparison_value)
    del mutated["result"]["resolver_comparison"]
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    mutated["result"]["resolver_comparison"]["unexpected"] = 0
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    mutated["result"]["resolver_comparison"]["rounds"][0][
        "post_component_count"
    ] += 1
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    mutated["result"]["resolver_comparison"]["rounds"][0][
        "unordered_point_pair_count"
    ] += 1
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    mutated["result"]["resolver_comparison"]["rounds"][0]["direct"][
        "target_component_seed_offer_count"
    ] += 1
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    mutated["result"]["resolver_comparison"]["rounds"][0]["dynamic"][
        "aabb_pair_bound_evaluation_count"
    ] += 1
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    dynamic_round = mutated["result"]["resolver_comparison"]["rounds"][0][
        "dynamic"
    ]
    dynamic_round["strict_incumbent_decrease_count"] = (
        2 * dynamic_round["point_pair_distance_evaluation_count"] + 1
    )
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    dynamic_round = mutated["result"]["resolver_comparison"]["rounds"][0][
        "dynamic"
    ]
    dynamic_round["ancestor_update_count"] = (
        dynamic_round["strict_incumbent_decrease_count"]
        * (mutated["point_count"] - 1)
        + 1
    )
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    direct_round = mutated["result"]["resolver_comparison"]["rounds"][0][
        "direct"
    ]
    direct_round["component_kappa_update_count"] = (
        2 * direct_round["point_pair_distance_evaluation_count"] + 1
    )
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    mutated["result"]["resolver_comparison"]["rounds"][0]["direct"][
        "component_cutoff_upper_envelope_node_count"
    ] += 1
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    impossible_dynamic = mutated["result"]["resolver_comparison"]["rounds"][0][
        "dynamic"
    ]
    impossible_dynamic.update(
        {
            "aabb_pair_bound_evaluation_count": 2,
            "ancestor_update_count": 0,
            "frontier_peak": 1,
            "node_pair_expansion_count": 1,
            "node_pair_visit_count": 2,
            "point_pair_distance_evaluation_count": 1,
            "strict_aabb_pair_prune_count": 0,
            "strict_incumbent_decrease_count": 0,
            "uniform_same_component_pair_prune_count": 0,
        }
    )
    dynamic_rounds = mutated["result"]["resolver_comparison"]["rounds"]
    dynamic_totals = mutated["result"]["resolver_comparison"]["totals"][
        "dynamic"
    ]
    for key in RESOLVER_DYNAMIC_KEYS:
        values = [round_value["dynamic"][key] for round_value in dynamic_rounds]
        dynamic_totals[key] = max(values) if key == "frontier_peak" else sum(values)
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    mutated["result"]["resolver_comparison"]["totals"]["dynamic"][
        "node_pair_visit_count"
    ] += 1
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    mutated["result"]["resolver_comparison"]["totals"]["dynamic"][
        "frontier_peak"
    ] += 1
    comparison_mutations.append(mutated)
    mutated = copy.deepcopy(comparison_value)
    mutated["result"]["resolver_comparison"]["totals"]["direct"][
        "node_pair_visit_count"
    ] += 1
    comparison_mutations.append(mutated)

    rejected = 0
    for mutation in mutations:
        try:
            validate_document(mutation, expected_backend="fake_gpu")
        except ContractError:
            rejected += 1
        else:
            fail("an exact-search work-profile negative mutation was accepted")
    for mutation in comparison_mutations:
        try:
            validate_comparison_document(mutation, expected_backend="fake_gpu")
        except ContractError:
            rejected += 1
        else:
            fail("a resolver-comparison negative mutation was accepted")
    try:
        json.loads(
            '{"schema":1,"schema":2}', object_pairs_hook=reject_duplicate_json_keys
        )
    except ContractError:
        rejected += 1
    else:
        fail("a duplicate JSON key was accepted")
    return True, rejected


def parse_arguments(arguments: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("project", nargs="?", type=Path)
    parser.add_argument("--executable", type=Path)
    parser.add_argument("--profile-log", type=Path)
    parser.add_argument("--expected-backend")
    parser.add_argument("--git-sha")
    parser.add_argument("--family", choices=("uniform", "clusters", "lattice"))
    parser.add_argument("--point-count", type=int)
    parser.add_argument("--window-radius", type=int)
    parser.add_argument("--seed", type=int)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    args = parse_arguments(arguments)
    if args.profile_log is not None:
        require(
            args.project is None, "project and --profile-log are mutually exclusive"
        )
        require(args.executable is None, "--executable cannot validate a profile log")
        for name in (
            "expected_backend",
            "git_sha",
            "family",
            "point_count",
            "window_radius",
            "seed",
        ):
            require(
                getattr(args, name) is not None,
                f"--{name.replace('_', '-')} is required",
            )
        require(SHA_RE.fullmatch(args.git_sha) is not None, "--git-sha differs")
        require(args.point_count >= 0, "--point-count must be a natural")
        require(args.window_radius >= 0, "--window-radius must be a natural")
        require(args.seed >= 0, "--seed must be a natural")
        value = read_profile_log(args.profile_log)
        validate_expected_identity(value, args)
        print(
            json.dumps(
                {
                    "backend": args.expected_backend,
                    "family": args.family,
                    "point_count": args.point_count,
                    "schema": SCHEMA,
                    "status": "validated",
                },
                sort_keys=True,
                separators=(",", ":"),
            )
        )
        return 0

    require(args.project is not None, "project or --profile-log is required")
    for name in (
        "expected_backend",
        "git_sha",
        "family",
        "point_count",
        "window_radius",
        "seed",
    ):
        require(
            getattr(args, name) is None,
            f"--{name.replace('_', '-')} requires --profile-log",
        )
    executed, negative_mutations = static_check(args.project.resolve(), args.executable)
    print(
        json.dumps(
            {
                "host_executed": executed,
                "negative_mutations": negative_mutations,
                "schema": STATIC_SCHEMA,
                "work_profile_schema": SCHEMA,
            },
            sort_keys=True,
            separators=(",", ":"),
        )
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (
        ContractError,
        OSError,
        subprocess.SubprocessError,
        json.JSONDecodeError,
        UnicodeError,
    ) as error:
        print(
            f"Phase 5 exact-search work-profile contract failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1) from error
