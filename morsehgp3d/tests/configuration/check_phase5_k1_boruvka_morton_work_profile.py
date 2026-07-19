#!/usr/bin/env python3
"""Validate the benchmark-only Phase 5 Morton work-profile contract."""

from __future__ import annotations

import argparse
import copy
from fractions import Fraction
import json
from pathlib import Path
import re
import subprocess
from typing import Any, NoReturn, Sequence

SCHEMA = "morsehgp3d.phase5.k1_boruvka_morton_work_profile.v1"
STATIC_SCHEMA = "morsehgp3d.phase5.k1_boruvka_morton_work_profile_static.v1"
SCOPE = "producer_only_work_profile_without_independent_replay_or_reference_oracle"
TOP_KEYS = {
    "artifact_kind",
    "backend",
    "baseline",
    "decision_backend",
    "generator",
    "git",
    "hierarchy_reduction_status",
    "mode",
    "morton_profiles",
    "phase",
    "point_count",
    "policies",
    "profile",
    "qualification_claimed",
    "scalability_claimed",
    "schema",
    "scientific_public_status",
    "scientific_result_claimed",
    "scope",
    "status",
}
RUN_KEYS = {"component_count_path", "exact_weights", "rounds", "totals"}
ROUND_KEYS = {
    "cpu_exact_aabb_bound_evaluation_count",
    "cpu_exact_candidate_distance_evaluation_count",
    "cpu_required_candidate_count",
    "gpu_count_pass_node_visit_count",
    "gpu_emit_pass_node_visit_count",
    "gpu_invalid_bound_descent_count",
    "gpu_strict_aabb_prune_count",
    "gpu_uniform_component_prune_count",
    "logical_candidate_count",
    "max_source_candidate_count",
    "seed_work",
    "peak_chunk_candidate_count",
    "post_component_count",
    "pre_component_count",
    "round_index",
    "source_chunk_count",
}
TOTAL_KEYS = (
    ROUND_KEYS
    - {"post_component_count", "pre_component_count", "round_index"}
) | {"round_count"}
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
PROFILE_KEYS = {"comparison", "measurement", "window_radius"}
COMPARISON_KEYS = {
    "candidate_savings_rate",
    "canonical_contractions_unchanged",
    "emst_edges_unchanged",
    "exact_decisions_unchanged",
    "exact_weights_unchanged",
    "fallback_rate",
    "strict_improvement_rate",
}
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
MAXIMUM_PROFILE_POINT_COUNT = 1_000_000


class ContractError(ValueError):
    """The work profile escaped its benchmark-only closed schema."""


def reject_duplicate_json_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    value: dict[str, Any] = {}
    for key, item in pairs:
        if key in value:
            fail(f"duplicate JSON key: {key}")
        value[key] = item
    return value


def fail(message: str) -> NoReturn:
    raise ContractError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def exact_keys(value: Any, expected: set[str], label: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{label} must be an object")
    require(set(value) == expected, f"{label} fields differ")
    return value


def natural(value: Any, label: str, *, positive: bool = False) -> int:
    require(type(value) is int and value >= 0, f"{label} must be a natural")
    require(not positive or value > 0, f"{label} must be positive")
    return value


def reduced_ratio(value: Any, label: str) -> Fraction:
    ratio = exact_keys(value, {"denominator", "numerator"}, label)
    numerator = natural(ratio["numerator"], f"{label}.numerator")
    denominator = natural(
        ratio["denominator"], f"{label}.denominator", positive=True
    )
    require(
        Fraction(numerator, denominator).numerator == numerator
        and Fraction(numerator, denominator).denominator == denominator,
        f"{label} must be reduced",
    )
    return Fraction(numerator, denominator)


def validate_level(value: Any, label: str) -> Fraction:
    level = exact_keys(value, {"denominator", "numerator"}, label)
    require(
        isinstance(level["numerator"], str)
        and re.fullmatch(r"0|[1-9][0-9]*", level["numerator"]) is not None,
        f"{label}.numerator must be a canonical nonnegative integer string",
    )
    require(
        isinstance(level["denominator"], str)
        and re.fullmatch(r"[1-9][0-9]*", level["denominator"]) is not None,
        f"{label}.denominator must be a canonical positive integer string",
    )
    require(level["numerator"] != "0", f"{label} must be positive")
    numerator = int(level["numerator"])
    denominator = int(level["denominator"])
    rational = Fraction(numerator, denominator)
    require(
        rational.numerator == numerator and rational.denominator == denominator,
        f"{label} must be reduced",
    )
    return rational


def validate_seed(
    value: Any,
    label: str,
    *,
    point_count: int,
    radius: int | None,
) -> dict[str, int]:
    seed = exact_keys(value, SEED_KEYS, label)
    result = {key: natural(item, f"{label}.{key}") for key, item in seed.items()}
    require(result["source_count"] == point_count, f"{label} source coverage differs")
    if radius is None:
        require(
            result
            == {
                "exact_fallback_count": point_count,
                "exact_seed_distance_evaluation_count": point_count,
                "exact_selected_proposal_count": 0,
                "exact_strict_improvement_count": 0,
                "external_neighbor_count": 0,
                "floating_proposal_count": 0,
                "inspected_neighbor_count": 0,
                "source_count": point_count,
            },
            f"{label} baseline seed work differs",
        )
        return result
    effective_radius = min(point_count - 1, radius)
    require(
        result["inspected_neighbor_count"]
        == effective_radius * (2 * point_count - effective_radius - 1),
        f"{label} Morton inspection identity differs",
    )
    require(
        result["external_neighbor_count"] <= result["inspected_neighbor_count"],
        f"{label} external count exceeds inspections",
    )
    require(
        result["floating_proposal_count"]
        <= min(result["source_count"], result["external_neighbor_count"]),
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
        == result["source_count"],
        f"{label} fallback partition differs",
    )
    require(
        result["source_count"]
        <= result["exact_seed_distance_evaluation_count"]
        <= result["source_count"] + result["floating_proposal_count"],
        f"{label} exact seed work bound differs",
    )
    require(
        result["exact_selected_proposal_count"]
        <= result["exact_seed_distance_evaluation_count"] - result["source_count"],
        f"{label} selected proposals exceed recertified proposal distances",
    )
    return result


def validate_run(
    value: Any,
    label: str,
    *,
    point_count: int,
    budget: int,
    radius: int | None,
) -> dict[str, Any]:
    run = exact_keys(value, RUN_KEYS, label)
    path = run["component_count_path"]
    rounds = run["rounds"]
    require(isinstance(path, list) and isinstance(rounds, list), f"{label} arrays differ")
    path = [natural(item, f"{label}.path", positive=True) for item in path]
    require(len(path) == len(rounds) + 1, f"{label} path length differs")
    require(path and path[0] == point_count and path[-1] == 1, f"{label} path endpoints differ")
    weights = exact_keys(run["exact_weights"], {"hgp", "squared"}, f"{label}.weights")
    hgp_weight = validate_level(weights["hgp"], f"{label}.weights.hgp")
    squared_weight = validate_level(weights["squared"], f"{label}.weights.squared")
    require(4 * hgp_weight == squared_weight, f"{label} HGP weight is not squared/4")

    sum_keys = TOTAL_KEYS - {
        "max_source_candidate_count",
        "seed_work",
        "peak_chunk_candidate_count",
        "round_count",
    }
    sums = {key: 0 for key in sum_keys}
    seed_sums = {key: 0 for key in SEED_KEYS}
    peak_h = 0
    peak_m = 0
    for index, item in enumerate(rounds):
        round_value = exact_keys(item, ROUND_KEYS, f"{label}.rounds[{index}]")
        numeric = {
            key: natural(entry, f"{label}.rounds[{index}].{key}")
            for key, entry in round_value.items()
            if key != "seed_work"
        }
        require(numeric["round_index"] == index, f"{label} round index differs")
        require(
            numeric["pre_component_count"] == path[index]
            and numeric["post_component_count"] == path[index + 1],
            f"{label} round path differs",
        )
        require(
            0 < numeric["post_component_count"]
            <= numeric["pre_component_count"] // 2,
            f"{label} contraction bound differs",
        )
        require(
            1
            <= numeric["max_source_candidate_count"]
            <= numeric["peak_chunk_candidate_count"]
            <= min(budget, numeric["logical_candidate_count"])
            and numeric["max_source_candidate_count"] <= point_count - 1,
            f"{label} M/H/B bound differs",
        )
        require(
            numeric["logical_candidate_count"] >= point_count
            and numeric["cpu_required_candidate_count"] >= point_count
            and numeric["source_chunk_count"] > 0,
            f"{label} nonterminal round work is incomplete",
        )
        chunk_count = numeric["source_chunk_count"]
        logical_count = numeric["logical_candidate_count"]
        require(
            logical_count <= point_count * (point_count - 1)
            and (logical_count + point_count - 1) // point_count
            <= numeric["max_source_candidate_count"],
            f"{label} source maximum is below the average source load",
        )
        require(
            (logical_count + chunk_count - 1) // chunk_count
            <= numeric["peak_chunk_candidate_count"],
            f"{label} peak chunk is below the average chunk load",
        )
        require(
            (logical_count + budget - 1) // budget
            <= chunk_count
            <= point_count,
            f"{label} chunk count bound differs",
        )
        require(
            numeric["cpu_exact_candidate_distance_evaluation_count"]
            == numeric["logical_candidate_count"],
            f"{label} candidate recertification count differs",
        )
        require(
            numeric["cpu_required_candidate_count"]
            <= numeric["logical_candidate_count"],
            f"{label} required candidate count exceeds the GPU superset",
        )
        require(
            numeric["gpu_count_pass_node_visit_count"]
            == numeric["gpu_emit_pass_node_visit_count"],
            f"{label} count/emit visits differ",
        )
        seed = validate_seed(
            round_value["seed_work"],
            f"{label}.rounds[{index}].seed",
            point_count=point_count,
            radius=radius,
        )
        for key in sum_keys:
            sums[key] += numeric[key]
        for key in SEED_KEYS:
            seed_sums[key] += seed[key]
        peak_h = max(peak_h, numeric["peak_chunk_candidate_count"])
        peak_m = max(peak_m, numeric["max_source_candidate_count"])

    totals = exact_keys(run["totals"], TOTAL_KEYS, f"{label}.totals")
    total_numeric = {
        key: natural(entry, f"{label}.totals.{key}")
        for key, entry in totals.items()
        if key != "seed_work"
    }
    total_seed_object = exact_keys(
        totals["seed_work"], SEED_KEYS, f"{label}.totals.seed"
    )
    total_seed = {
        key: natural(entry, f"{label}.totals.seed.{key}")
        for key, entry in total_seed_object.items()
    }
    require(total_numeric["round_count"] == len(rounds), f"{label} total rounds differ")
    require(
        all(total_numeric[key] == expected for key, expected in sums.items()),
        f"{label} additive totals differ",
    )
    require(
        total_numeric["peak_chunk_candidate_count"] == peak_h
        and total_numeric["max_source_candidate_count"] == peak_m,
        f"{label} peak totals differ",
    )
    require(
        all(total_seed[key] == expected for key, expected in seed_sums.items()),
        f"{label} seed totals differ",
    )
    return run


def contains_key(value: Any, forbidden: str) -> bool:
    if isinstance(value, dict):
        return forbidden in value or any(contains_key(item, forbidden) for item in value.values())
    if isinstance(value, list):
        return any(contains_key(item, forbidden) for item in value)
    return False


def validate_document(value: Any, *, expected_backend: str) -> None:
    document = exact_keys(value, TOP_KEYS, "work profile")
    expected_scalars = {
        "artifact_kind": "work_profile",
        "backend": expected_backend,
        "decision_backend": "reference_cpu",
        "hierarchy_reduction_status": "not_performed",
        "mode": "certified",
        "phase": "5",
        "profile": "hgp_reduced",
        "schema": SCHEMA,
        "scope": SCOPE,
        "status": "measured",
    }
    for key, expected in expected_scalars.items():
        require(document[key] == expected, f"work profile {key} differs")
    for key in (
        "qualification_claimed",
        "scalability_claimed",
        "scientific_result_claimed",
    ):
        require(document[key] is False, f"work profile {key} must be false")
    require(document["scientific_public_status"] is None, "public scientific status must be null")
    require(not contains_key(document, "public_status"), "public_status is forbidden")
    point_count = natural(document["point_count"], "point_count", positive=True)
    require(
        2 <= point_count <= MAXIMUM_PROFILE_POINT_COUNT,
        "point_count escapes the injective generator domain",
    )
    generator = exact_keys(document["generator"], {"algorithm", "family", "seed"}, "generator")
    require(generator["algorithm"] == "deterministic_dyadic_v1", "generator algorithm differs")
    require(generator["family"] in {"uniform", "clusters", "lattice"}, "generator family differs")
    natural(generator["seed"], "generator.seed")
    git = exact_keys(document["git"], {"sha"}, "git")
    require(isinstance(git["sha"], str) and SHA_RE.fullmatch(git["sha"]), "git SHA differs")
    policies = exact_keys(document["policies"], {"candidate_record_budget", "window_radii"}, "policies")
    budget = natural(policies["candidate_record_budget"], "budget", positive=True)
    require(budget >= point_count - 1, "budget does not expose M_r")
    radii = policies["window_radii"]
    require(isinstance(radii, list) and radii, "window radii differ")
    radii = [natural(radius, "window radius", positive=True) for radius in radii]
    require(len(set(radii)) == len(radii), "window radii repeat")
    baseline = validate_run(
        document["baseline"], "baseline", point_count=point_count, budget=budget, radius=None
    )
    profiles = document["morton_profiles"]
    require(isinstance(profiles, list) and len(profiles) == len(radii), "Morton profile count differs")
    baseline_l = baseline["totals"]["logical_candidate_count"]
    for index, (profile_value, radius) in enumerate(zip(profiles, radii, strict=True)):
        profile = exact_keys(profile_value, PROFILE_KEYS, f"profiles[{index}]")
        require(profile["window_radius"] == radius, f"profiles[{index}] radius differs")
        measurement = validate_run(
            profile["measurement"],
            f"profiles[{index}].measurement",
            point_count=point_count,
            budget=budget,
            radius=radius,
        )
        comparison = exact_keys(profile["comparison"], COMPARISON_KEYS, f"profiles[{index}].comparison")
        for key in (
            "canonical_contractions_unchanged",
            "emst_edges_unchanged",
            "exact_decisions_unchanged",
            "exact_weights_unchanged",
        ):
            require(comparison[key] is True, f"profiles[{index}] {key} differs")
        require(measurement["component_count_path"] == baseline["component_count_path"], "component paths differ")
        require(measurement["exact_weights"] == baseline["exact_weights"], "exact weights differ")
        measured_l = measurement["totals"]["logical_candidate_count"]
        require(measured_l <= baseline_l, "Morton logical work increased")
        require(
            reduced_ratio(comparison["candidate_savings_rate"], "candidate savings")
            == Fraction(baseline_l - measured_l, baseline_l),
            "candidate savings ratio differs",
        )
        seed = measurement["totals"]["seed_work"]
        source_count = seed["source_count"]
        require(
            reduced_ratio(comparison["fallback_rate"], "fallback rate")
            == Fraction(seed["exact_fallback_count"], source_count),
            "fallback ratio differs",
        )
        require(
            reduced_ratio(comparison["strict_improvement_rate"], "improvement rate")
            == Fraction(seed["exact_strict_improvement_count"], source_count),
            "improvement ratio differs",
        )


def parse_arguments(arguments: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("project", type=Path)
    parser.add_argument("--executable", type=Path)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    args = parse_arguments(arguments)
    project = args.project.resolve()
    source = (project / "src/tools/gpu_k1_boruvka_morton_work_profile.cpp").read_text(encoding="utf-8")
    cmake = (project / "CMakeLists.txt").read_text(encoding="utf-8")
    unit_cmake = (project / "tests/unit/CMakeLists.txt").read_text(encoding="utf-8")
    assembler = (
        project / "tests/cuda/assemble_phase5_k1_boruvka_work_profile.py"
    ).read_text(encoding="utf-8")
    for token in (
        SCHEMA,
        SCOPE,
        "propose_round_chunked",
        "contract_exact_k1_boruvka_round",
        "qualification_claimed",
        "scalability_claimed",
        "candidate_savings_rate",
    ):
        require(token in source, f"source token missing: {token}")
    for token in (
        "morsehgp3d_gpu_k1_boruvka_morton_work_profile",
        'MORSEHGP3D_WORK_PROFILE_BACKEND=\\"cuda_g4\\"',
    ):
        require(token in cmake, f"CUDA CMake token missing: {token}")
    for token in (
        "morsehgp3d_gpu_k1_boruvka_morton_work_profile_host",
        'MORSEHGP3D_WORK_PROFILE_BACKEND=\\"fake_gpu\\"',
    ):
        require(token in unit_cmake, f"host CMake token missing: {token}")
    for token in (
        "WORK_PROFILE_SCHEMA",
        "morsehgp3d.phase5.k1_boruvka_morton_work_profile_artifact.v1",
        "validate_document",
        "benchmark_only",
        "worker_passed_pending_shutdown",
    ):
        require(token in assembler, f"assembler token missing: {token}")

    executed = False
    negative_mutations = 0
    if args.executable is not None:
        executable = args.executable.resolve()
        completed = subprocess.run(
            [
                str(executable),
                "--family",
                "lattice",
                "--point-count",
                "8",
                "--candidate-record-budget",
                "7",
                "--window-radii",
                "1,2",
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
        require(completed.stdout.endswith("\n") and completed.stdout.count("\n") == 1, "host output is not one-line JSON")
        value = json.loads(
            completed.stdout, object_pairs_hook=reject_duplicate_json_keys
        )
        validate_document(value, expected_backend="fake_gpu")
        mutations: list[dict[str, Any]] = []
        mutated = copy.deepcopy(value)
        mutated["qualification_claimed"] = True
        mutations.append(mutated)
        mutated = copy.deepcopy(value)
        mutated["public_status"] = "exact"
        mutations.append(mutated)
        mutated = copy.deepcopy(value)
        mutated["baseline"]["rounds"][0]["logical_candidate_count"] -= 1
        mutations.append(mutated)
        mutated = copy.deepcopy(value)
        mutated["morton_profiles"][0]["measurement"]["rounds"][0][
            "seed_work"
        ]["inspected_neighbor_count"] -= 1
        mutations.append(mutated)
        mutated = copy.deepcopy(value)
        seed_work = mutated["morton_profiles"][0]["measurement"]["rounds"][0][
            "seed_work"
        ]
        seed_work["exact_selected_proposal_count"] = (
            seed_work["exact_seed_distance_evaluation_count"]
            - seed_work["source_count"]
            + 1
        )
        mutations.append(mutated)
        mutated = copy.deepcopy(value)
        mutated["morton_profiles"][0]["comparison"]["candidate_savings_rate"] = {
            "denominator": 1,
            "numerator": 0,
        }
        mutations.append(mutated)
        mutated = copy.deepcopy(value)
        mutated["baseline"]["rounds"][0]["max_source_candidate_count"] = 0
        mutated["baseline"]["rounds"][0]["peak_chunk_candidate_count"] = 0
        mutated["baseline"]["totals"]["max_source_candidate_count"] = 0
        mutated["baseline"]["totals"]["peak_chunk_candidate_count"] = 0
        mutations.append(mutated)
        mutated = copy.deepcopy(value)
        mutated["baseline"]["exact_weights"]["hgp"]["numerator"] = "1"
        mutations.append(mutated)
        mutated = copy.deepcopy(value)
        mutated["baseline"]["rounds"][0]["max_source_candidate_count"] = 1
        mutated["baseline"]["totals"]["max_source_candidate_count"] = max(
            [
                1,
            *(
                round_value["max_source_candidate_count"]
                for round_value in mutated["baseline"]["rounds"][1:]
            ),
            ]
        )
        mutations.append(mutated)
        for mutation in mutations:
            try:
                validate_document(mutation, expected_backend="fake_gpu")
            except ContractError:
                negative_mutations += 1
            else:
                fail("a work-profile negative mutation was accepted")
        executed = True
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
    except (ContractError, OSError, subprocess.SubprocessError, json.JSONDecodeError) as error:
        print(f"Phase 5 Morton work-profile contract failed: {error}", file=__import__("sys").stderr)
        raise SystemExit(1) from error
