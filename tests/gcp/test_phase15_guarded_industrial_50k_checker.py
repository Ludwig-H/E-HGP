#!/usr/bin/env python3
"""Mutation tests for the guarded industrial 50k campaign checker."""

from __future__ import annotations

import copy
from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

from tools.check_phase15_guarded_industrial_50k import (
    EXPECTED_DEVICE,
    EXPECTED_FAMILIES,
    IndustrialCampaignError,
    load_report,
    main,
    require_local_commit,
    validate_campaign,
    validate_report,
)

ROOT = Path(__file__).resolve().parents[2]
GIT_SHA = subprocess.run(
    ["git", "rev-parse", "HEAD"],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    check=True,
).stdout.strip()
INPUT_SHA256 = ("1" * 64, "2" * 64, "3" * 64)
RUNNER_BINARY_SHA256 = "a" * 64
RUNNER_BINARY_SIZE_BYTES = 12_345_678
CUDA_BANNER = """

==========
== CUDA ==
==========

CUDA Version 12.9.2

Container image Copyright (c) 2016-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

This container image and its contents are governed by the NVIDIA Deep Learning Container License.
By pulling and using the container, you accept the terms and conditions of this license:
https://developer.nvidia.com/ngc/nvidia-deep-learning-container-license

A copy of this license is made available in this container at /NGC-DL-CONTAINER-LICENSE for your convenience.

"""


def nearest_rank(values: list[int], percentile: int) -> int:
    ordered = sorted(values)
    return ordered[((percentile * len(ordered) + 99) // 100) - 1]


def hierarchy(seed: int, order: int) -> dict[str, object]:
    return {
        "order": order,
        "merge_record_count": 49_999,
        "digest": f"0x{(seed << 8) | order:016x}",
        "root_squared_level": float(order),
    }


def report(
    family: str,
    seed_base: int,
    measured_e2e: list[int],
) -> dict[str, object]:
    runs: list[dict[str, object]] = []
    all_e2e = [1_200_000_000, 80_000_000, *measured_e2e]
    for run_index, warm_e2e in enumerate(all_e2e):
        warmup = run_index < 2
        seed = seed_base + 1_000 + run_index if warmup else seed_base + run_index - 2
        runs.append(
            {
                "run_index": run_index,
                "measured_index": None if warmup else run_index - 2,
                "warmup": warmup,
                "seed": seed,
                "fresh_cloud": True,
                "generation_ns": 60_000_000,
                "canonicalization_ns": 5_000_000,
                "timings_ns": {
                    "lbvh": 10_000_000,
                    "topk": 5_000_000,
                    "topology": 20_000_000,
                    "reductions": 20_000_000,
                    "output_materialization": 1_000_000,
                    "warm_e2e": warm_e2e,
                },
                "common_topology_edge_count": 300_000,
                "top_k_component_count": 4,
                "component_bridge_pair_count": 3,
                "component_bridge_support_evaluation_count": 50_192,
                "neighbor_record_count": 500_000,
                "lbvh_kernel_launch_count": 42,
                "top_k_node_visit_count": 4_000_000,
                "top_k_strict_prune_count": 2_000_000,
                "device_name": EXPECTED_DEVICE,
                "oracle_export_ns": 0,
                "oracle_export_path": None,
                "hierarchies": [hierarchy(seed, order) for order in range(1, 11)],
            }
        )
    percentiles = {
        "p50": nearest_rank(measured_e2e, 50),
        "p95": nearest_rank(measured_e2e, 95),
        "p99": nearest_rank(measured_e2e, 99),
    }
    return {
        "schema": "morsehgp3d.phase15.guarded_industrial_candidate.v4",
        "result_kind": "guarded_industrial_candidate",
        "phase": 15,
        "backend": "cuda_g4_plus_host_binary64",
        "profile": "hgp_reduced",
        "mode": "warm_fresh_cloud_lbvh_top10_component_bridges_parallel_h0",
        "deployment_status": "architecture_only",
        "public_status": "not_claimed",
        "git_sha": GIT_SHA,
        "runner_binary_sha256": RUNNER_BINARY_SHA256,
        "runner_binary_size_bytes": RUNNER_BINARY_SIZE_BYTES,
        "family": family,
        "point_count": 50_000,
        "repetitions": 10,
        "warmups": 2,
        "maximum_order": 10,
        "seed_window": 32,
        "cloud_generation_timed": False,
        "canonicalization_timed": True,
        "fresh_cloud_per_run": True,
        "full_pipeline": True,
        "ten_hierarchies_materialized": True,
        "weight_representation": "squared_binary64_mutual_reachability_with_k2plus_core_lower_envelope",
        "core_deadline_lower_envelope_ulps": 64,
        "component_bridge_neighbor_count": 6,
        "component_bridge_policy": "top10_components_centroid_knn_top8_projection_cross_min",
        "ordinary_delaunay_materialized": False,
        "higher_order_delaunay_mosaic_materialized": False,
        "global_pair_matrix_materialized": False,
        "exact_morse_hierarchy_claimed": False,
        "oracle_exports_outside_timed_region": True,
        "runs": runs,
        "nearest_rank_warm_e2e_ns": percentiles,
        "slo": {
            "point_count": 50_000,
            "threshold_ns": 100_000_000,
            "statistic": "p95_nearest_rank_warm_e2e",
            "applicable": True,
            "passed": True,
        },
        "result": "slo_satisfied",
    }


def report_v5(
    family: str,
    seed_base: int,
    measured_e2e: list[int],
) -> dict[str, object]:
    value = report(family, seed_base, measured_e2e)
    value.update(
        {
            "schema": "morsehgp3d.phase15.guarded_industrial_candidate.v5",
            "mode": (
                "warm_fresh_cloud_lbvh_top10_capped_"
                "component_bridges_parallel_h0"
            ),
            "export_maximum_order": 2,
            "component_bridge_maximum_component_count": 256,
            "component_bridge_maximum_centroid_distance_evaluation_count": 65_280,
            "component_bridge_member_projection_budget_factor": 16,
            "component_bridge_maximum_pair_count": 1_536,
            "component_bridge_maximum_cross_distance_evaluation_count": 98_304,
            "component_bridge_policy": (
                "top10_components_capped_centroid_knn_top8_"
                "projection_cross_min"
            ),
        }
    )
    for raw_run in value["runs"]:
        raw_run.update(
            {
                "component_bridge_centroid_distance_evaluation_count": 12,
                "component_bridge_member_projection_evaluation_budget": 800_000,
                "component_bridge_member_projection_evaluation_count": 50_000,
                "component_bridge_cross_distance_evaluation_budget": 98_304,
                "component_bridge_cross_distance_evaluation_count": 192,
                "component_bridge_budget_satisfied": True,
                "component_bridge_budget_stop_reason": "none",
                "materialized_merge_record_count": 499_990,
                "released_merge_record_count": 499_990,
                "retained_merge_record_count": 0,
                "merge_records_released_after_digest_and_export": True,
                "exported_order_count": 0,
                "exported_merge_record_count": 0,
            }
        )
    return value


def report_v6(
    family: str,
    seed_base: int,
    measured_e2e: list[int],
) -> dict[str, object]:
    value = report_v5(family, seed_base, measured_e2e)
    value.update(
        {
            "schema": (
                "morsehgp3d.phase15."
                "point_mst_mutual_reachability_surrogate.v6"
            ),
            "result_kind": "point_mst_mutual_reachability_surrogate_benchmark",
            "profile": "point_mst_mutual_reachability_surrogate",
            "mode": (
                "warm_fresh_cloud_lbvh_top10_capped_component_bridges_"
                "parallel_point_h0_surrogate"
            ),
            "deployment_status": "diagnostic_surrogate_only",
            "public_status": "not_a_morse_hgp_product_result",
            "surrogate_pipeline_complete": True,
            "point_vertex_universe_only": True,
            "ten_point_spanning_tree_surrogates_materialized": True,
            "morse_hgp_simplex_components_materialized": False,
            "true_hgp_campaign_eligible": False,
            "slo": {
                "point_count": 50_000,
                "threshold_ns": 100_000_000,
                "statistic": "true_morse_hgp_p95_end_to_end",
                "scope": "true_morse_hgp_hierarchy",
                "applicable": False,
                "passed": None,
                "reason": (
                    "point_tree_surrogate_does_not_materialize_"
                    "morse_hgp_simplex_components"
                ),
            },
            "result": "surrogate_benchmark_completed",
        }
    )
    value.pop("full_pipeline")
    value.pop("ten_hierarchies_materialized")
    for raw_run in value["runs"]:
        raw_run["point_spanning_tree_surrogates"] = [
            {
                "neighbor_rank": tree["order"],
                "edge_count": tree["merge_record_count"],
                "digest": tree["digest"],
                "root_squared_level": tree["root_squared_level"],
            }
            for tree in raw_run.pop("hierarchies")
        ]
        for legacy, honest in (
            (
                "materialized_merge_record_count",
                "materialized_surrogate_edge_record_count",
            ),
            (
                "released_merge_record_count",
                "released_surrogate_edge_record_count",
            ),
            (
                "retained_merge_record_count",
                "retained_surrogate_edge_record_count",
            ),
            (
                "merge_records_released_after_digest_and_export",
                "surrogate_edge_records_released_after_digest_and_export",
            ),
            ("exported_order_count", "exported_surrogate_order_count"),
            (
                "exported_merge_record_count",
                "exported_surrogate_edge_record_count",
            ),
        ):
            raw_run[honest] = raw_run.pop(legacy)
    return value


def legacy_campaign_v4() -> list[dict[str, object]]:
    return [
        report(
            EXPECTED_FAMILIES[0], 100, list(range(65_000_000, 75_000_000, 1_000_000))
        ),
        report(
            EXPECTED_FAMILIES[1], 200, list(range(75_000_000, 85_000_000, 1_000_000))
        ),
        report(
            EXPECTED_FAMILIES[2], 300, list(range(85_000_000, 95_000_000, 1_000_000))
        ),
    ]


def campaign_v5() -> list[dict[str, object]]:
    return [
        report_v5(
            EXPECTED_FAMILIES[0], 100, list(range(65_000_000, 75_000_000, 1_000_000))
        ),
        report_v5(
            EXPECTED_FAMILIES[1], 200, list(range(75_000_000, 85_000_000, 1_000_000))
        ),
        report_v5(
            EXPECTED_FAMILIES[2], 300, list(range(85_000_000, 95_000_000, 1_000_000))
        ),
    ]


def campaign() -> list[dict[str, object]]:
    return [
        report_v6(
            EXPECTED_FAMILIES[0], 100, list(range(65_000_000, 75_000_000, 1_000_000))
        ),
        report_v6(
            EXPECTED_FAMILIES[1], 200, list(range(75_000_000, 85_000_000, 1_000_000))
        ),
        report_v6(
            EXPECTED_FAMILIES[2], 300, list(range(85_000_000, 95_000_000, 1_000_000))
        ),
    ]


def validate(values: list[dict[str, object]]) -> dict[str, object]:
    return validate_campaign(
        values,
        expected_git_sha=GIT_SHA,
        input_artifact_sha256=INPUT_SHA256,
    )


class Phase15GuardedIndustrial50kCheckerTests(unittest.TestCase):
    def test_valid_campaign_replays_aggregate_nearest_rank(self) -> None:
        summary = validate(campaign())
        self.assertEqual(summary["measurement_count"], 30)
        self.assertEqual(summary["distinct_measured_seed_count"], 30)
        self.assertEqual(
            summary["point_spanning_tree_surrogate_count_per_measurement"], 10
        )
        self.assertEqual(summary["edge_record_count_per_surrogate"], 49_999)
        self.assertEqual(
            summary["aggregate_nearest_rank_warm_e2e_ns"],
            {
                "p50": 79_000_000,
                "p95": 93_000_000,
                "p99": 94_000_000,
                "max": 94_000_000,
            },
        )
        self.assertIs(
            summary["all_measurements_below_historical_100ms_reference"], True
        )
        self.assertEqual(
            summary["timing_scope"]["input_boundary"],
            "raw_in_memory_coordinates",
        )
        self.assertEqual(
            summary["timing_scope"]["source_field"],
            "runs[].timings_ns.warm_e2e",
        )
        self.assertIs(
            summary["timing_scope"]["canonicalization_included_in_warm_e2e"],
            True,
        )
        self.assertIs(
            summary["timing_scope"]["canonicalization_added_by_checker"],
            False,
        )
        self.assertIs(
            summary["timing_scope"]["synthetic_cloud_generation_excluded"],
            True,
        )
        self.assertEqual(
            summary["runner_contract"]["weight_representation"],
            "squared_binary64_mutual_reachability_with_k2plus_core_lower_envelope",
        )
        self.assertEqual(
            summary["runner_contract"]["core_deadline_lower_envelope_ulps"],
            64,
        )
        self.assertEqual(
            summary["runner_contract"]["mode"],
            "warm_fresh_cloud_lbvh_top10_capped_component_bridges_parallel_point_h0_surrogate",
        )
        self.assertEqual(
            summary["runner_contract"]["component_bridge_neighbor_count"],
            6,
        )
        self.assertEqual(
            summary["runner_contract"]["component_bridge_policy"],
            "top10_components_capped_centroid_knn_top8_projection_cross_min",
        )
        self.assertEqual(
            summary["content_evidence"][
                "globally_distinct_surrogate_tree_digest_count"
            ],
            360,
        )
        self.assertIs(
            summary["content_evidence"]["surrogate_edge_records_replayed"],
            False,
        )
        self.assertEqual(
            summary["input_artifact_sha256"],
            dict(zip(EXPECTED_FAMILIES, INPUT_SHA256, strict=True)),
        )
        self.assertIs(summary["provenance"]["local_commit_object_verified"], True)
        self.assertIs(
            summary["provenance"]["runner_binary_self_attestation_bound"], True
        )
        self.assertEqual(
            summary["provenance"]["runner_binary_sha256"],
            RUNNER_BINARY_SHA256,
        )
        self.assertEqual(
            summary["provenance"]["runner_binary_size_bytes"],
            RUNNER_BINARY_SIZE_BYTES,
        )
        self.assertEqual(
            summary["schema"],
            "morsehgp3d.phase15.point_mst_mutual_reachability_surrogate_50k_audit.v6",
        )
        self.assertEqual(summary["runner_contract"]["export_maximum_order"], 2)
        self.assertEqual(
            summary["scope"]["public_status"],
            "not_a_morse_hgp_product_result",
        )
        self.assertIs(summary["scope"]["exact_morse_hierarchy_claimed"], False)
        self.assertIs(summary["scope"]["true_hgp_campaign_eligible"], False)
        self.assertIs(summary["slo"]["applicable"], False)
        self.assertIsNone(summary["slo"]["passed"])
        self.assertEqual(summary["result"], "surrogate_audit_validated")

    def test_valid_v5_campaign_binds_bounded_telemetry_and_release(self) -> None:
        summary = validate_campaign(
            campaign_v5(),
            expected_git_sha=GIT_SHA,
            input_artifact_sha256=INPUT_SHA256,
            legacy_surrogate_audit=True,
        )
        self.assertEqual(
            summary["schema"],
            "morsehgp3d.phase15.legacy_point_tree_surrogate_artifact_audit.v1",
        )
        contract = summary["runner_contract"]
        self.assertEqual(
            contract["schema"],
            "morsehgp3d.phase15.guarded_industrial_candidate.v5",
        )
        self.assertEqual(contract["export_maximum_order"], 2)
        self.assertEqual(contract["component_bridge_maximum_component_count"], 256)
        self.assertEqual(
            contract["component_bridge_member_projection_budget_factor"], 16
        )
        self.assertEqual(
            contract["component_bridge_maximum_cross_distance_evaluation_count"],
            98_304,
        )
        self.assertIs(
            summary["content_evidence"][
                "surrogate_edge_records_released_after_digest_and_export"
            ],
            True,
        )
        self.assertEqual(summary["result"], "legacy_surrogate_artifact_audited")

    def test_v5_bounded_telemetry_and_release_fail_closed(self) -> None:
        mutations = (
            ("component cap", {"top_k_component_count": 257}, "at most 256"),
            (
                "centroid distances",
                {"component_bridge_centroid_distance_evaluation_count": 11},
                r"C\(C-1\)",
            ),
            (
                "projection budget",
                {"component_bridge_member_projection_evaluation_budget": 799_999},
                "16n",
            ),
            (
                "projection count",
                {"component_bridge_member_projection_evaluation_count": 800_001},
                "at most 800000",
            ),
            (
                "cross budget",
                {"component_bridge_cross_distance_evaluation_budget": 98_303},
                r"64\*1536",
            ),
            (
                "cross count",
                {"component_bridge_cross_distance_evaluation_count": 98_305},
                "at most 98304",
            ),
            ("pair cap", {"component_bridge_pair_count": 1_537}, "at most 1536"),
            (
                "support split",
                {"component_bridge_support_evaluation_count": 50_191},
                "projection plus cross-distance",
            ),
            (
                "budget outcome",
                {"component_bridge_budget_satisfied": False},
                "component_bridge_budget_satisfied",
            ),
            (
                "materialized count",
                {"materialized_merge_record_count": 499_989},
                r"10\(n-1\)",
            ),
            (
                "released count",
                {"released_merge_record_count": 499_989},
                "released_merge_record_count",
            ),
            (
                "retained records",
                {"retained_merge_record_count": 1},
                "retained_merge_record_count",
            ),
            (
                "release claim",
                {"merge_records_released_after_digest_and_export": False},
                "merge_records_released_after_digest_and_export",
            ),
            (
                "unexpected export",
                {"exported_order_count": 1},
                "exported_order_count",
            ),
        )
        baseline = campaign_v5()[0]
        for label, mutation, message in mutations:
            with self.subTest(label=label):
                changed = copy.deepcopy(baseline)
                changed["runs"][2].update(mutation)
                with self.assertRaisesRegex(IndustrialCampaignError, message):
                    validate_report(changed, legacy_surrogate_audit=True)

    def test_legacy_v4_v5_require_explicit_surrogate_audit_mode(self) -> None:
        with self.assertRaisesRegex(
            IndustrialCampaignError, "explicit legacy-surrogate audit mode"
        ):
            validate_report(campaign_v5()[0])
        validate_report(campaign_v5()[0], legacy_surrogate_audit=True)

    def test_campaign_rejects_mixed_v4_and_v5_reports(self) -> None:
        changed = campaign_v5()
        changed[0] = legacy_campaign_v4()[0]
        with self.assertRaisesRegex(
            IndustrialCampaignError, "share one runner report schema"
        ):
            validate_campaign(
                changed,
                expected_git_sha=GIT_SHA,
                input_artifact_sha256=INPUT_SHA256,
                legacy_surrogate_audit=True,
            )

    def test_synthetic_generation_is_excluded_from_raw_input_timing(self) -> None:
        baseline = validate(campaign())
        changed = campaign()
        changed[0]["runs"][2]["generation_ns"] = 9_000_000_000
        observed = validate(changed)
        self.assertEqual(
            observed["aggregate_nearest_rank_warm_e2e_ns"],
            baseline["aggregate_nearest_rank_warm_e2e_ns"],
        )

    def test_loader_accepts_plain_json_and_the_cuda_banner(self) -> None:
        value = campaign()[0]
        encoded = json.dumps(value, separators=(",", ":")) + "\n"
        with tempfile.TemporaryDirectory() as directory:
            plain = Path(directory) / "plain.json"
            banner = Path(directory) / "banner.json"
            plain.write_text(encoded, encoding="utf-8")
            banner.write_text(CUDA_BANNER + encoded, encoding="utf-8")
            self.assertEqual(load_report(plain), value)
            self.assertEqual(load_report(banner), value)

    def test_loader_rejects_unknown_prefix_duplicate_keys_and_non_finite(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "report.json"
            path.write_text('warning\n{"schema":"x"}\n', encoding="utf-8")
            with self.assertRaisesRegex(IndustrialCampaignError, "unrecognized text"):
                load_report(path)
            path.write_text('{"schema":"x","schema":"y"}\n', encoding="utf-8")
            with self.assertRaisesRegex(IndustrialCampaignError, "duplicate JSON key"):
                load_report(path)
            path.write_text('{"value":NaN}\n', encoding="utf-8")
            with self.assertRaisesRegex(IndustrialCampaignError, "non-finite"):
                load_report(path)

    def test_report_fails_closed_on_pipeline_mutations(self) -> None:
        mutations = (
            (
                "legacy HGP profile",
                lambda value: value.update({"profile": "hgp_reduced"}),
                "profile",
            ),
            (
                "exact Morse hierarchy claim",
                lambda value: value.update({"exact_morse_hierarchy_claimed": True}),
                "exact_morse_hierarchy_claimed",
            ),
            (
                "true HGP eligibility claim",
                lambda value: value.update({"true_hgp_campaign_eligible": True}),
                "true_hgp_campaign_eligible",
            ),
            (
                "true HGP SLO applicability claim",
                lambda value: value["slo"].update(
                    {"applicable": True, "passed": True}
                ),
                "slo.applicable",
            ),
            (
                "stage sum",
                lambda value: value["runs"][2]["timings_ns"].update(
                    {"lbvh": 90_000_000}
                ),
                "timed canonicalization/stage sum",
            ),
            (
                "surrogate tree count",
                lambda value: value["runs"][2][
                    "point_spanning_tree_surrogates"
                ].pop(),
                "exactly 10 point-tree surrogates",
            ),
            (
                "surrogate edge count",
                lambda value: value["runs"][2][
                    "point_spanning_tree_surrogates"
                ][0].update(
                    {"edge_count": 49_998}
                ),
                "edge_count",
            ),
            (
                "published p95",
                lambda value: value["nearest_rank_warm_e2e_ns"].update({"p95": 1}),
                "does not replay",
            ),
            (
                "canonicalization outside warm clock",
                lambda value: value["runs"][2].update(
                    {"canonicalization_ns": 40_000_000}
                ),
                "timed canonicalization/stage sum",
            ),
            (
                "canonicalization timing claim",
                lambda value: value.update({"canonicalization_timed": False}),
                "canonicalization_timed",
            ),
            (
                "deadline weight semantics",
                lambda value: value.update(
                    {"weight_representation": "squared_binary64_mutual_reachability"}
                ),
                "weight_representation",
            ),
            (
                "deadline weight envelope",
                lambda value: value.update({"core_deadline_lower_envelope_ulps": 63}),
                "core_deadline_lower_envelope_ulps",
            ),
            (
                "component bridge neighbors",
                lambda value: value.update({"component_bridge_neighbor_count": 5}),
                "component_bridge_neighbor_count",
            ),
            (
                "component bridge policy",
                lambda value: value.update({"component_bridge_policy": "forged"}),
                "component_bridge_policy",
            ),
            (
                "component bridge mode",
                lambda value: value.update(
                    {"mode": "warm_fresh_cloud_lbvh_top10_parallel_h0"}
                ),
                "mode",
            ),
            (
                "duplicate surrogate digest",
                lambda value: value["runs"][2][
                    "point_spanning_tree_surrogates"
                ][1].update(
                    {
                        "digest": value["runs"][2][
                            "point_spanning_tree_surrogates"
                        ][0]["digest"]
                    }
                ),
                "distinct surrogate digest",
            ),
            (
                "runner binary SHA",
                lambda value: value.update({"runner_binary_sha256": "forged"}),
                "runner_binary_sha256",
            ),
            (
                "runner binary size",
                lambda value: value.update({"runner_binary_size_bytes": 0}),
                "runner_binary_size_bytes",
            ),
            (
                "missing runner binary attestation",
                lambda value: value.pop("runner_binary_sha256"),
                "keys differ",
            ),
        )
        baseline = campaign()[0]
        for label, mutate, message in mutations:
            with self.subTest(label=label):
                changed = copy.deepcopy(baseline)
                mutate(changed)
                with self.assertRaisesRegex(IndustrialCampaignError, message):
                    validate_report(changed)

    def test_surrogate_timing_never_qualifies_true_hgp_slo(self) -> None:
        values = campaign()
        for offset, value in enumerate(values):
            measured = [150_000_000 + offset * 10_000_000 + index for index in range(10)]
            for index, timing in enumerate(measured, start=2):
                value["runs"][index]["timings_ns"]["warm_e2e"] = timing
            value["nearest_rank_warm_e2e_ns"] = {
                "p50": nearest_rank(measured, 50),
                "p95": nearest_rank(measured, 95),
                "p99": nearest_rank(measured, 99),
            }
        summary = validate(values)
        self.assertGreater(summary["aggregate_nearest_rank_warm_e2e_ns"]["p95"], 100_000_000)
        self.assertIs(summary["slo"]["applicable"], False)
        self.assertIsNone(summary["slo"]["passed"])
        self.assertIs(summary["scope"]["true_hgp_campaign_eligible"], False)

    def test_component_bridge_invariants_fail_closed(self) -> None:
        connected = campaign()[0]
        for run in connected["runs"]:
            run["top_k_component_count"] = 1
            run["component_bridge_pair_count"] = 0
            run["component_bridge_support_evaluation_count"] = 0
            run["component_bridge_centroid_distance_evaluation_count"] = 0
            run["component_bridge_member_projection_evaluation_count"] = 0
            run["component_bridge_cross_distance_evaluation_count"] = 0
        validate_report(connected)

        mutations = (
            (
                "zero components",
                {"top_k_component_count": 0},
                "top_k_component_count must be at least 1",
            ),
            (
                "insufficient bridge pairs",
                {"component_bridge_pair_count": 2},
                "cover at least C-1 pairs",
            ),
            (
                "insufficient support orientations",
                {"component_bridge_support_evaluation_count": 5},
                "projection plus cross-distance",
            ),
            (
                "connected graph with bridge work",
                {
                    "top_k_component_count": 1,
                    "component_bridge_pair_count": 1,
                    "component_bridge_support_evaluation_count": 2,
                    "component_bridge_centroid_distance_evaluation_count": 0,
                    "component_bridge_member_projection_evaluation_count": 1,
                    "component_bridge_cross_distance_evaluation_count": 1,
                },
                "must not evaluate component bridges",
            ),
            (
                "candidate graph not connected",
                {"common_topology_edge_count": 49_998},
                "common_topology_edge_count must be at least 49999",
            ),
        )
        baseline = campaign()[0]
        for label, mutation, message in mutations:
            with self.subTest(label=label):
                changed = copy.deepcopy(baseline)
                changed["runs"][2].update(mutation)
                with self.assertRaisesRegex(IndustrialCampaignError, message):
                    validate_report(changed)

    def test_pre_v4_report_is_rejected(self) -> None:
        changed = campaign()[0]
        changed["schema"] = "morsehgp3d.phase15.guarded_industrial_candidate.v3"
        with self.assertRaisesRegex(IndustrialCampaignError, "schema"):
            validate_report(changed)

    def test_campaign_requires_three_families_one_sha_and_distinct_seeds(self) -> None:
        changed = campaign()
        changed[2]["family"] = EXPECTED_FAMILIES[1]
        with self.assertRaisesRegex(IndustrialCampaignError, "each required family"):
            validate(changed)

        changed = campaign()
        changed[2]["git_sha"] = "b" * 40
        with self.assertRaisesRegex(IndustrialCampaignError, "share one Git SHA"):
            validate(changed)

        changed = campaign()
        changed[2]["runner_binary_sha256"] = "b" * 64
        with self.assertRaisesRegex(
            IndustrialCampaignError, "share one self-attested runner binary"
        ):
            validate(changed)

        changed = campaign()
        for run_index in range(2, 12):
            changed[2]["runs"][run_index]["seed"] = changed[0]["runs"][run_index][
                "seed"
            ]
        with self.assertRaisesRegex(IndustrialCampaignError, "30 distinct"):
            validate(changed)

    def test_campaign_binds_expected_commit_and_input_artifacts(self) -> None:
        other_commit = subprocess.run(
            ["git", "rev-parse", "HEAD^"],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            check=True,
        ).stdout.strip()
        with self.assertRaisesRegex(IndustrialCampaignError, "does not match"):
            validate_campaign(
                campaign(),
                expected_git_sha=other_commit,
                input_artifact_sha256=INPUT_SHA256,
            )
        with self.assertRaisesRegex(IndustrialCampaignError, "distinct SHA-256"):
            validate_campaign(
                campaign(),
                expected_git_sha=GIT_SHA,
                input_artifact_sha256=("1" * 64, "1" * 64, "3" * 64),
            )
        changed = campaign()
        changed[2]["runs"][2]["point_spanning_tree_surrogates"][0][
            "digest"
        ] = changed[0]["runs"][2]["point_spanning_tree_surrogates"][0]["digest"]
        with self.assertRaisesRegex(IndustrialCampaignError, "360 globally distinct"):
            validate(changed)
        with self.assertRaisesRegex(IndustrialCampaignError, "not a commit"):
            require_local_commit("f" * 40)

    def test_cli_writes_the_same_canonical_summary_it_prints(self) -> None:
        current_sha = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            check=True,
        ).stdout.strip()
        values = campaign()
        for value in values:
            value["git_sha"] = current_sha
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            inputs: list[Path] = []
            for index, value in enumerate(values):
                path = root / f"input-{index}.json"
                path.write_text(
                    json.dumps(value, separators=(",", ":")) + "\n",
                    encoding="utf-8",
                )
                inputs.append(path)
            output = root / "summary.json"
            stdout = io.StringIO()
            with redirect_stdout(stdout):
                result = main(
                    [
                        "--expected-git-sha",
                        current_sha,
                        "--summary-output",
                        str(output),
                        *(str(path) for path in inputs),
                    ]
                )
            self.assertEqual(result, 0)
            self.assertEqual(output.read_text(encoding="utf-8"), stdout.getvalue())
            self.assertTrue(stdout.getvalue().endswith("\n"))
            summary = json.loads(stdout.getvalue())
            self.assertEqual(summary["git_sha"], current_sha)
            self.assertEqual(len(summary["input_artifact_sha256"]), 3)


if __name__ == "__main__":
    unittest.main()
