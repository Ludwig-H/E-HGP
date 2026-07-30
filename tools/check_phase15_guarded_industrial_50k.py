#!/usr/bin/env python3
"""Validate the three-family Phase 15 guarded industrial 50k campaign.

The runner deliberately publishes an ``architecture_only`` candidate rather
than an exact Morse hierarchy.  This checker preserves that scope while
independently replaying the structural and latency claims made by its three
reports.  It accepts either a plain JSON report or the same report preceded by
the standard NVIDIA CUDA container banner emitted by the pinned G4 image.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from typing import Any, Iterable, NoReturn, Sequence

REPORT_SCHEMA = "morsehgp3d.phase15.guarded_industrial_candidate.v4"
CHECK_SCHEMA = "morsehgp3d.phase15.guarded_industrial_50k_check.v4"
EXPECTED_FAMILIES = (
    "affine_uniform_binary64",
    "jittered_dyadic_grid3d",
    "balanced_multiscale_clusters",
)
POINT_COUNT = 50_000
REPETITIONS = 10
WARMUPS = 2
MAXIMUM_ORDER = 10
MERGE_RECORD_COUNT = POINT_COUNT - 1
NEIGHBOR_RECORD_COUNT = POINT_COUNT * MAXIMUM_ORDER
SLO_THRESHOLD_NS = 100_000_000
EXPECTED_DEVICE = "NVIDIA RTX PRO 6000 Blackwell Server Edition"
WEIGHT_REPRESENTATION = (
    "squared_binary64_mutual_reachability_with_k2plus_core_lower_envelope"
)
CORE_DEADLINE_LOWER_ENVELOPE_ULPS = 64
COMPONENT_BRIDGE_NEIGHBOR_COUNT = 6
COMPONENT_BRIDGE_POLICY = (
    "top10_components_centroid_knn_top8_projection_cross_min"
)

_GIT_SHA = re.compile(r"[0-9a-f]{40}")
_SHA256 = re.compile(r"[0-9a-f]{64}")
_DIGEST = re.compile(r"0x[0-9a-f]{16}")
_CUDA_VERSION = re.compile(r"CUDA Version [0-9]+\.[0-9]+\.[0-9]+")
_CUDA_COPYRIGHT = re.compile(
    r"Container image Copyright \(c\) [0-9]{4}-[0-9]{4}, "
    r"NVIDIA CORPORATION & AFFILIATES\. All rights reserved\."
)
_CUDA_BANNER_FIXED_LINES = (
    "==========",
    "== CUDA ==",
    "==========",
    "This container image and its contents are governed by the NVIDIA Deep Learning Container License.",
    "By pulling and using the container, you accept the terms and conditions of this license:",
    "https://developer.nvidia.com/ngc/nvidia-deep-learning-container-license",
    "A copy of this license is made available in this container at /NGC-DL-CONTAINER-LICENSE for your convenience.",
)

_REPORT_KEYS = frozenset(
    {
        "schema",
        "result_kind",
        "phase",
        "backend",
        "profile",
        "mode",
        "deployment_status",
        "public_status",
        "git_sha",
        "runner_binary_sha256",
        "runner_binary_size_bytes",
        "family",
        "point_count",
        "repetitions",
        "warmups",
        "maximum_order",
        "seed_window",
        "cloud_generation_timed",
        "canonicalization_timed",
        "fresh_cloud_per_run",
        "full_pipeline",
        "ten_hierarchies_materialized",
        "weight_representation",
        "core_deadline_lower_envelope_ulps",
        "component_bridge_neighbor_count",
        "component_bridge_policy",
        "ordinary_delaunay_materialized",
        "higher_order_delaunay_mosaic_materialized",
        "global_pair_matrix_materialized",
        "exact_morse_hierarchy_claimed",
        "oracle_exports_outside_timed_region",
        "runs",
        "nearest_rank_warm_e2e_ns",
        "slo",
        "result",
    }
)
_RUN_KEYS = frozenset(
    {
        "run_index",
        "measured_index",
        "warmup",
        "seed",
        "fresh_cloud",
        "generation_ns",
        "canonicalization_ns",
        "timings_ns",
        "common_topology_edge_count",
        "top_k_component_count",
        "component_bridge_pair_count",
        "component_bridge_support_evaluation_count",
        "neighbor_record_count",
        "lbvh_kernel_launch_count",
        "top_k_node_visit_count",
        "top_k_strict_prune_count",
        "device_name",
        "oracle_export_ns",
        "oracle_export_path",
        "hierarchies",
    }
)
_TIMING_KEYS = frozenset(
    {"lbvh", "topk", "topology", "reductions", "output_materialization", "warm_e2e"}
)
_HIERARCHY_KEYS = frozenset(
    {"order", "merge_record_count", "digest", "root_squared_level"}
)
_PERCENTILE_KEYS = frozenset({"p50", "p95", "p99"})
_SLO_KEYS = frozenset(
    {"point_count", "threshold_ns", "statistic", "applicable", "passed"}
)
_STAGE_NAMES = ("lbvh", "topk", "topology", "reductions", "output_materialization")


class IndustrialCampaignError(ValueError):
    """An input escaped the guarded industrial campaign contract."""


@dataclass(frozen=True)
class _RunEvidence:
    measured_seed: int | None
    warm_e2e_ns: int | None
    hierarchy_digests: tuple[str, ...]


def fail(message: str) -> NoReturn:
    raise IndustrialCampaignError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def _reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        require(key not in result, f"duplicate JSON key: {key}")
        result[key] = value
    return result


def _reject_non_finite(value: str) -> NoReturn:
    fail(f"non-finite JSON value: {value}")


def _mapping(value: object, *, path: str) -> dict[str, Any]:
    require(type(value) is dict, f"{path} must be an object")
    return value  # type: ignore[return-value]


def _array(value: object, *, path: str) -> list[Any]:
    require(type(value) is list, f"{path} must be an array")
    return value  # type: ignore[return-value]


def _exact_keys(record: dict[str, Any], expected: Iterable[str], *, path: str) -> None:
    expected_set = frozenset(expected)
    observed = frozenset(record)
    require(
        observed == expected_set,
        f"{path} keys differ: missing={sorted(expected_set - observed)}, "
        f"extra={sorted(observed - expected_set)}",
    )


def _literal(record: dict[str, Any], key: str, expected: object, *, path: str) -> None:
    observed = record.get(key)
    require(
        type(observed) is type(expected) and observed == expected,
        f"{path}.{key} must be {expected!r}",
    )


def _integer(
    value: object,
    *,
    path: str,
    minimum: int = 0,
    maximum: int | None = None,
) -> int:
    require(type(value) is int, f"{path} must be an integer")
    result = value  # type: ignore[assignment]
    require(result >= minimum, f"{path} must be at least {minimum}")
    if maximum is not None:
        require(result <= maximum, f"{path} must be at most {maximum}")
    return result


def _finite_nonnegative(value: object, *, path: str) -> float:
    require(
        type(value) in (int, float),
        f"{path} must be a finite non-negative number",
    )
    result = float(value)  # type: ignore[arg-type]
    require(
        math.isfinite(result) and result >= 0.0,
        f"{path} must be a finite non-negative number",
    )
    return result


def _nearest_rank(values: Sequence[int], percentile: int) -> int:
    require(bool(values), "nearest-rank input must not be empty")
    require(1 <= percentile <= 100, "nearest-rank percentile must be in [1, 100]")
    ordered = sorted(values)
    rank = (percentile * len(ordered) + 99) // 100
    return ordered[rank - 1]


def _validate_cuda_banner(prefix: str, *, path: Path) -> None:
    lines = tuple(line.strip() for line in prefix.splitlines() if line.strip())
    require(len(lines) == 9, f"{path}: unrecognized text before JSON report")
    require(lines[:3] == _CUDA_BANNER_FIXED_LINES[:3], f"{path}: invalid CUDA banner")
    require(
        _CUDA_VERSION.fullmatch(lines[3]) is not None,
        f"{path}: invalid CUDA version banner",
    )
    require(
        _CUDA_COPYRIGHT.fullmatch(lines[4]) is not None,
        f"{path}: invalid CUDA copyright banner",
    )
    require(
        lines[5:] == _CUDA_BANNER_FIXED_LINES[3:],
        f"{path}: invalid CUDA license banner",
    )


def _load_report_and_sha256(path: Path) -> tuple[dict[str, Any], str]:
    try:
        raw_bytes = path.read_bytes()
        raw = raw_bytes.decode("utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read {path}: {error}")
    json_start = raw.find("{")
    require(json_start >= 0, f"{path}: missing JSON object")
    prefix = raw[:json_start]
    if prefix.strip():
        _validate_cuda_banner(prefix, path=path)
    payload = raw[json_start:].strip()
    try:
        value = json.loads(
            payload,
            object_pairs_hook=_reject_duplicate_keys,
            parse_constant=_reject_non_finite,
        )
    except (json.JSONDecodeError, UnicodeError) as error:
        fail(f"cannot parse {path}: {error}")
    return _mapping(value, path=str(path)), hashlib.sha256(raw_bytes).hexdigest()


def load_report(path: Path) -> dict[str, Any]:
    """Load one report, accepting only plain JSON or the NVIDIA banner prefix."""

    report, _ = _load_report_and_sha256(path)
    return report


def _validate_hierarchies(value: object, *, path: str) -> tuple[str, ...]:
    hierarchies = _array(value, path=path)
    require(
        len(hierarchies) == MAXIMUM_ORDER,
        f"{path} must contain exactly {MAXIMUM_ORDER} hierarchies",
    )
    digests: list[str] = []
    for offset, raw_hierarchy in enumerate(hierarchies):
        hierarchy_path = f"{path}[{offset}]"
        hierarchy = _mapping(raw_hierarchy, path=hierarchy_path)
        _exact_keys(hierarchy, _HIERARCHY_KEYS, path=hierarchy_path)
        _literal(hierarchy, "order", offset + 1, path=hierarchy_path)
        _literal(
            hierarchy,
            "merge_record_count",
            MERGE_RECORD_COUNT,
            path=hierarchy_path,
        )
        digest = hierarchy.get("digest")
        require(
            type(digest) is str and _DIGEST.fullmatch(digest) is not None,
            f"{hierarchy_path}.digest must be one canonical 64-bit hex digest",
        )
        digests.append(digest)
        _finite_nonnegative(
            hierarchy.get("root_squared_level"),
            path=f"{hierarchy_path}.root_squared_level",
        )
    require(
        len(set(digests)) == MAXIMUM_ORDER,
        f"{path}: every order in one run must have a distinct hierarchy digest",
    )
    return tuple(digests)


def _validate_run(
    raw_run: object,
    *,
    report_path: str,
    run_index: int,
) -> _RunEvidence:
    path = f"{report_path}.runs[{run_index}]"
    run = _mapping(raw_run, path=path)
    _exact_keys(run, _RUN_KEYS, path=path)
    _literal(run, "run_index", run_index, path=path)
    warmup = run_index < WARMUPS
    _literal(run, "warmup", warmup, path=path)
    expected_measured_index = None if warmup else run_index - WARMUPS
    _literal(run, "measured_index", expected_measured_index, path=path)
    _literal(run, "fresh_cloud", True, path=path)
    seed = _integer(
        run.get("seed"),
        path=f"{path}.seed",
        maximum=(1 << 64) - 1,
    )
    _integer(run.get("generation_ns"), path=f"{path}.generation_ns")
    canonicalization_ns = _integer(
        run.get("canonicalization_ns"),
        path=f"{path}.canonicalization_ns",
    )

    timings = _mapping(run.get("timings_ns"), path=f"{path}.timings_ns")
    _exact_keys(timings, _TIMING_KEYS, path=f"{path}.timings_ns")
    timing_values = {
        name: _integer(
            timings.get(name),
            path=f"{path}.timings_ns.{name}",
            minimum=1 if name == "warm_e2e" else 0,
        )
        for name in _TIMING_KEYS
    }
    stage_sum = canonicalization_ns + sum(timing_values[name] for name in _STAGE_NAMES)
    warm_e2e = timing_values["warm_e2e"]
    require(
        stage_sum <= warm_e2e,
        f"{path}: timed canonicalization/stage sum {stage_sum} exceeds "
        f"warm_e2e {warm_e2e}",
    )
    if not warmup:
        require(
            warm_e2e < SLO_THRESHOLD_NS,
            f"{path}.timings_ns.warm_e2e must be strictly below {SLO_THRESHOLD_NS}",
        )

    _integer(
        run.get("common_topology_edge_count"),
        path=f"{path}.common_topology_edge_count",
        minimum=MERGE_RECORD_COUNT,
    )
    component_count = _integer(
        run.get("top_k_component_count"),
        path=f"{path}.top_k_component_count",
        minimum=1,
        maximum=POINT_COUNT,
    )
    bridge_pair_count = _integer(
        run.get("component_bridge_pair_count"),
        path=f"{path}.component_bridge_pair_count",
    )
    support_evaluation_count = _integer(
        run.get("component_bridge_support_evaluation_count"),
        path=f"{path}.component_bridge_support_evaluation_count",
    )
    if component_count == 1:
        require(
            bridge_pair_count == 0 and support_evaluation_count == 0,
            f"{path}: a connected top10 graph must not evaluate component bridges",
        )
    else:
        require(
            bridge_pair_count >= component_count - 1,
            f"{path}.component_bridge_pair_count must cover at least C-1 pairs",
        )
        require(
            support_evaluation_count >= 3 * bridge_pair_count,
            f"{path}.component_bridge_support_evaluation_count must cover both "
            "projection scans and at least one cross-distance per bridge pair",
        )
    _literal(
        run,
        "neighbor_record_count",
        NEIGHBOR_RECORD_COUNT,
        path=path,
    )
    for key in (
        "lbvh_kernel_launch_count",
        "top_k_node_visit_count",
        "top_k_strict_prune_count",
    ):
        _integer(run.get(key), path=f"{path}.{key}", minimum=1)
    _literal(run, "device_name", EXPECTED_DEVICE, path=path)
    _literal(run, "oracle_export_ns", 0, path=path)
    _literal(run, "oracle_export_path", None, path=path)
    hierarchy_digests = _validate_hierarchies(
        run.get("hierarchies"), path=f"{path}.hierarchies"
    )
    if warmup:
        return _RunEvidence(None, None, hierarchy_digests)
    return _RunEvidence(
        seed,
        warm_e2e,
        hierarchy_digests,
    )


def validate_report(report: dict[str, Any], *, path: str = "report") -> dict[str, Any]:
    """Validate one family report and return its replayed measured values."""

    _exact_keys(report, _REPORT_KEYS, path=path)
    for key, expected in (
        ("schema", REPORT_SCHEMA),
        ("result_kind", "guarded_industrial_candidate"),
        ("phase", 15),
        ("backend", "cuda_g4_plus_host_binary64"),
        ("profile", "hgp_reduced"),
        (
            "mode",
            "warm_fresh_cloud_lbvh_top10_component_bridges_parallel_h0",
        ),
        ("deployment_status", "architecture_only"),
        ("public_status", "not_claimed"),
        ("point_count", POINT_COUNT),
        ("repetitions", REPETITIONS),
        ("warmups", WARMUPS),
        ("maximum_order", MAXIMUM_ORDER),
        ("seed_window", 32),
        ("cloud_generation_timed", False),
        ("canonicalization_timed", True),
        ("fresh_cloud_per_run", True),
        ("full_pipeline", True),
        ("ten_hierarchies_materialized", True),
        (
            "weight_representation",
            WEIGHT_REPRESENTATION,
        ),
        (
            "core_deadline_lower_envelope_ulps",
            CORE_DEADLINE_LOWER_ENVELOPE_ULPS,
        ),
        ("component_bridge_neighbor_count", COMPONENT_BRIDGE_NEIGHBOR_COUNT),
        ("component_bridge_policy", COMPONENT_BRIDGE_POLICY),
        ("ordinary_delaunay_materialized", False),
        ("higher_order_delaunay_mosaic_materialized", False),
        ("global_pair_matrix_materialized", False),
        ("exact_morse_hierarchy_claimed", False),
        ("oracle_exports_outside_timed_region", True),
        ("result", "slo_satisfied"),
    ):
        _literal(report, key, expected, path=path)

    git_sha = report.get("git_sha")
    require(
        type(git_sha) is str and _GIT_SHA.fullmatch(git_sha) is not None,
        f"{path}.git_sha must be one lowercase full Git SHA",
    )
    runner_binary_sha256 = report.get("runner_binary_sha256")
    require(
        type(runner_binary_sha256) is str
        and _SHA256.fullmatch(runner_binary_sha256) is not None,
        f"{path}.runner_binary_sha256 must be one lowercase SHA-256",
    )
    runner_binary_size_bytes = _integer(
        report.get("runner_binary_size_bytes"),
        path=f"{path}.runner_binary_size_bytes",
        minimum=1,
    )
    family = report.get("family")
    require(
        type(family) is str and family in EXPECTED_FAMILIES,
        f"{path}.family must be one required industrial family",
    )

    runs = _array(report.get("runs"), path=f"{path}.runs")
    require(
        len(runs) == WARMUPS + REPETITIONS,
        f"{path}.runs must contain exactly {WARMUPS + REPETITIONS} runs",
    )
    all_seeds: list[int] = []
    measured_seeds: list[int] = []
    measured_e2e: list[int] = []
    hierarchy_digests: list[str] = []
    for run_index, run in enumerate(runs):
        evidence = _validate_run(
            run,
            report_path=path,
            run_index=run_index,
        )
        run_record = _mapping(run, path=f"{path}.runs[{run_index}]")
        all_seeds.append(run_record["seed"])
        hierarchy_digests.extend(evidence.hierarchy_digests)
        if evidence.measured_seed is not None and evidence.warm_e2e_ns is not None:
            measured_seeds.append(evidence.measured_seed)
            measured_e2e.append(evidence.warm_e2e_ns)
    require(len(set(all_seeds)) == len(all_seeds), f"{path}: run seeds must be unique")
    require(
        len(set(hierarchy_digests)) == len(hierarchy_digests),
        f"{path}: every order/run must have a distinct hierarchy digest",
    )
    require(
        len(measured_e2e) == REPETITIONS,
        f"{path}: exactly {REPETITIONS} measured runs are required",
    )
    first_seed = measured_seeds[0]
    require(
        measured_seeds == list(range(first_seed, first_seed + REPETITIONS)),
        f"{path}: measured seeds must form one contiguous fresh-cloud window",
    )

    expected_percentiles = {
        "p50": _nearest_rank(measured_e2e, 50),
        "p95": _nearest_rank(measured_e2e, 95),
        "p99": _nearest_rank(measured_e2e, 99),
    }
    summary_percentiles = {**expected_percentiles, "max": max(measured_e2e)}
    published_percentiles = _mapping(
        report.get("nearest_rank_warm_e2e_ns"),
        path=f"{path}.nearest_rank_warm_e2e_ns",
    )
    _exact_keys(
        published_percentiles,
        _PERCENTILE_KEYS,
        path=f"{path}.nearest_rank_warm_e2e_ns",
    )
    require(
        published_percentiles == expected_percentiles,
        f"{path}.nearest_rank_warm_e2e_ns does not replay",
    )

    slo = _mapping(report.get("slo"), path=f"{path}.slo")
    _exact_keys(slo, _SLO_KEYS, path=f"{path}.slo")
    for key, expected in (
        ("point_count", POINT_COUNT),
        ("threshold_ns", SLO_THRESHOLD_NS),
        ("statistic", "p95_nearest_rank_warm_e2e"),
        ("applicable", True),
        ("passed", True),
    ):
        _literal(slo, key, expected, path=f"{path}.slo")
    require(
        expected_percentiles["p95"] < SLO_THRESHOLD_NS,
        f"{path}: raw-in-memory warm_e2e family p95 misses the strict SLO",
    )
    return {
        "family": family,
        "git_sha": git_sha,
        "runner_binary_sha256": runner_binary_sha256,
        "runner_binary_size_bytes": runner_binary_size_bytes,
        "measured_seeds": measured_seeds,
        "measured_warm_e2e_ns": measured_e2e,
        "hierarchy_digests": hierarchy_digests,
        "nearest_rank_warm_e2e_ns": summary_percentiles,
    }


def validate_campaign(
    reports: Sequence[dict[str, Any]],
    *,
    expected_git_sha: str,
    input_artifact_sha256: Sequence[str],
) -> dict[str, Any]:
    """Validate all three reports and return an independently replayed summary."""

    require(
        _GIT_SHA.fullmatch(expected_git_sha) is not None,
        "expected Git SHA must be one lowercase full SHA",
    )
    require_local_commit(expected_git_sha)
    require(
        len(reports) == len(EXPECTED_FAMILIES),
        f"campaign must contain exactly {len(EXPECTED_FAMILIES)} reports",
    )
    require(
        len(input_artifact_sha256) == len(reports),
        "campaign must bind one SHA-256 to every input artifact",
    )
    for index, digest in enumerate(input_artifact_sha256):
        require(
            _SHA256.fullmatch(digest) is not None,
            f"input_artifact_sha256[{index}] must be one lowercase SHA-256",
        )
    require(
        len(set(input_artifact_sha256)) == len(input_artifact_sha256),
        "campaign input artifacts must have distinct SHA-256 commitments",
    )
    validated = [
        validate_report(report, path=f"reports[{index}]")
        for index, report in enumerate(reports)
    ]
    for index, entry in enumerate(validated):
        entry["input_artifact_sha256"] = input_artifact_sha256[index]
    by_family = {entry["family"]: entry for entry in validated}
    require(
        len(by_family) == len(EXPECTED_FAMILIES)
        and frozenset(by_family) == frozenset(EXPECTED_FAMILIES),
        "campaign must contain each required family exactly once",
    )
    git_shas = {entry["git_sha"] for entry in validated}
    require(len(git_shas) == 1, "campaign reports must share one Git SHA")
    require(
        git_shas == {expected_git_sha},
        "campaign report Git SHA does not match --expected-git-sha",
    )
    runner_binary_identities = {
        (entry["runner_binary_sha256"], entry["runner_binary_size_bytes"])
        for entry in validated
    }
    require(
        len(runner_binary_identities) == 1,
        "campaign reports must share one self-attested runner binary",
    )
    runner_binary_sha256, runner_binary_size_bytes = next(
        iter(runner_binary_identities)
    )

    measured_seeds = [
        seed
        for family in EXPECTED_FAMILIES
        for seed in by_family[family]["measured_seeds"]
    ]
    require(
        len(measured_seeds) == 30 and len(set(measured_seeds)) == 30,
        "campaign must contain 30 distinct measured fresh-cloud seeds",
    )
    measured_e2e = [
        timing
        for family in EXPECTED_FAMILIES
        for timing in by_family[family]["measured_warm_e2e_ns"]
    ]
    require(len(measured_e2e) == 30, "campaign must contain exactly 30 measurements")
    warm_e2e_aggregate = {
        "p50": _nearest_rank(measured_e2e, 50),
        "p95": _nearest_rank(measured_e2e, 95),
        "p99": _nearest_rank(measured_e2e, 99),
        "max": max(measured_e2e),
    }
    require(
        warm_e2e_aggregate["p95"] < SLO_THRESHOLD_NS,
        "aggregate raw-in-memory warm_e2e p95 misses the strict SLO",
    )
    hierarchy_digests = [
        digest
        for family in EXPECTED_FAMILIES
        for digest in by_family[family]["hierarchy_digests"]
    ]
    require(
        len(hierarchy_digests) == 360
        and len(set(hierarchy_digests)) == len(hierarchy_digests),
        "campaign must contain 360 globally distinct order/run hierarchy digests",
    )
    return {
        "schema": CHECK_SCHEMA,
        "git_sha": expected_git_sha,
        "provenance": {
            "expected_git_sha": expected_git_sha,
            "local_commit_object_verified": True,
            "input_artifact_sha256_bound": True,
            "runner_binary_self_attestation_bound": True,
            "runner_binary_sha256": runner_binary_sha256,
            "runner_binary_size_bytes": runner_binary_size_bytes,
        },
        "input_artifact_sha256": {
            family: by_family[family]["input_artifact_sha256"]
            for family in EXPECTED_FAMILIES
        },
        "families": list(EXPECTED_FAMILIES),
        "measurement_count": len(measured_e2e),
        "distinct_measured_seed_count": len(set(measured_seeds)),
        "hierarchy_count_per_measurement": MAXIMUM_ORDER,
        "merge_record_count_per_hierarchy": MERGE_RECORD_COUNT,
        "all_measurements_strictly_below_threshold": all(
            value < SLO_THRESHOLD_NS for value in measured_e2e
        ),
        "family_nearest_rank_warm_e2e_ns": {
            family: by_family[family]["nearest_rank_warm_e2e_ns"]
            for family in EXPECTED_FAMILIES
        },
        "aggregate_nearest_rank_warm_e2e_ns": warm_e2e_aggregate,
        "slo": {
            "point_count": POINT_COUNT,
            "threshold_ns": SLO_THRESHOLD_NS,
            "statistic": "aggregate_p95_nearest_rank_warm_e2e",
            "passed": True,
        },
        "timing_scope": {
            "input_boundary": "raw_in_memory_coordinates",
            "source_field": "runs[].timings_ns.warm_e2e",
            "warm_e2e_clock_contiguous": True,
            "canonicalization_included_in_warm_e2e": True,
            "canonicalization_added_by_checker": False,
            "output_boundary": "ten_reported_materialized_hierarchies",
            "synthetic_cloud_generation_excluded": True,
        },
        "runner_contract": {
            "schema": REPORT_SCHEMA,
            "runner_binary_sha256": runner_binary_sha256,
            "runner_binary_size_bytes": runner_binary_size_bytes,
            "mode": "warm_fresh_cloud_lbvh_top10_component_bridges_parallel_h0",
            "canonicalization_timed": True,
            "weight_representation": WEIGHT_REPRESENTATION,
            "core_deadline_lower_envelope_ulps": CORE_DEADLINE_LOWER_ENVELOPE_ULPS,
            "component_bridge_neighbor_count": COMPONENT_BRIDGE_NEIGHBOR_COUNT,
            "component_bridge_policy": COMPONENT_BRIDGE_POLICY,
        },
        "content_evidence": {
            "reported_hierarchy_count_per_run": MAXIMUM_ORDER,
            "reported_merge_record_count_per_hierarchy": MERGE_RECORD_COUNT,
            "globally_distinct_hierarchy_digest_count": len(hierarchy_digests),
            "merge_records_embedded": False,
            "merge_records_replayed": False,
        },
        "scope": {
            "deployment_status": "architecture_only",
            "public_status": "not_claimed",
            "exact_morse_hierarchy_claimed": False,
        },
        "result": "validated",
    }


def require_local_commit(
    git_sha: str,
    *,
    repository: Path = Path(__file__).resolve().parents[1],
) -> None:
    """Require ``git_sha`` to name a commit in the local evidence repository."""

    require(
        _GIT_SHA.fullmatch(git_sha) is not None,
        "expected Git SHA must be one lowercase full SHA",
    )
    try:
        completed = subprocess.run(
            ["git", "-C", str(repository), "cat-file", "-e", f"{git_sha}^{{commit}}"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
    except OSError as error:
        fail(f"cannot verify expected Git commit: {error}")
    require(
        completed.returncode == 0,
        f"expected Git SHA is not a commit in {repository}",
    )


def _canonical_json(value: object) -> str:
    return (
        json.dumps(
            value,
            ensure_ascii=False,
            allow_nan=False,
            sort_keys=True,
            separators=(",", ":"),
        )
        + "\n"
    )


def write_canonical_summary(path: Path, summary: dict[str, Any]) -> None:
    """Atomically publish one canonical UTF-8 JSON summary."""

    parent = path.parent
    require(parent.is_dir(), f"summary output directory does not exist: {parent}")
    temporary_path: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            newline="\n",
            prefix=f".{path.name}.",
            suffix=".tmp",
            dir=parent,
            delete=False,
        ) as output:
            temporary_path = Path(output.name)
            output.write(_canonical_json(summary))
            output.flush()
            os.fsync(output.fileno())
        os.replace(temporary_path, path)
        temporary_path = None
    except OSError as error:
        fail(f"cannot publish canonical summary {path}: {error}")
    finally:
        if temporary_path is not None:
            try:
                temporary_path.unlink()
            except FileNotFoundError:
                pass


def _git_sha_argument(value: str) -> str:
    if _GIT_SHA.fullmatch(value) is None:
        raise argparse.ArgumentTypeError("expected one lowercase full 40-hex Git SHA")
    return value


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--expected-git-sha",
        required=True,
        type=_git_sha_argument,
        help="full commit SHA from which all three reports were built",
    )
    parser.add_argument(
        "--summary-output",
        type=Path,
        help="atomically write the canonical validation summary to this path",
    )
    parser.add_argument(
        "reports",
        nargs=3,
        type=Path,
        metavar="REPORT",
        help="the affine, jittered and balanced runner reports, in any order",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    loaded = [_load_report_and_sha256(path) for path in args.reports]
    summary = validate_campaign(
        [report for report, _ in loaded],
        expected_git_sha=args.expected_git_sha,
        input_artifact_sha256=[digest for _, digest in loaded],
    )
    if args.summary_output is not None:
        output = args.summary_output.resolve()
        inputs = {path.resolve() for path in args.reports}
        require(
            output not in inputs,
            "summary output must not overwrite an input report",
        )
        write_canonical_summary(args.summary_output, summary)
    print(_canonical_json(summary), end="")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except IndustrialCampaignError as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
