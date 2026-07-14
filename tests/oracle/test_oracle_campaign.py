from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.run_oracle_campaign import (
    CampaignConfig,
    CampaignFailure,
    run_campaign,
)


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "tools" / "run_oracle_campaign.py"


class OracleCampaignTests(unittest.TestCase):
    def test_short_campaign_checks_all_dimensions_profiles_and_maps(self) -> None:
        manifest = run_campaign(
            CampaignConfig(
                dimensions=(3, 1, 2),
                seeds_per_dimension=1,
                point_counts=(5,),
                k_max_values=(3,),
                permutations_per_seed=1,
                ci=True,
            )
        )

        self.assertEqual(manifest["status"], "passed")
        self.assertEqual(manifest["schema_version"], 2)
        self.assertEqual(
            manifest["reconstruction_contract_id"], "hgp-reduced-v2"
        )
        self.assertFalse(manifest["gcp_used"])
        self.assertEqual(manifest["config"]["dimensions"], [1, 2, 3])
        self.assertEqual(manifest["config"]["profiles"], ["full_pi0", "hgp_reduced"])
        self.assertEqual(
            manifest["config"]["normative_hgp_reduced_relation"], "gamma"
        )
        self.assertEqual(
            manifest["config"]["diagnostic_flows"],
            ["gabriel_raw_partial_refinement"],
        )
        counters = manifest["counters"]
        self.assertEqual(counters["clouds_generated"], 3)
        self.assertEqual(counters["clouds_audited"], 3)
        self.assertEqual(counters["orders_checked"], 9)
        self.assertEqual(counters["profile_order_checks"], 18)
        self.assertEqual(counters["vertical_families_checked"], 18)
        self.assertEqual(counters["gamma_vertical_families_checked"], 6)
        self.assertEqual(counters["full_profile_vertical_families_checked"], 6)
        self.assertEqual(counters["reduced_profile_vertical_families_checked"], 6)
        self.assertEqual(counters["permutations_checked"], 3)
        self.assertEqual(counters["signed_axis_permutations_checked"], 3)
        self.assertEqual(counters["dyadic_translations_checked"], 3)
        self.assertEqual(counters["homotheties_checked"], 3)
        self.assertGreater(counters["homothety_levels_checked"], 0)
        self.assertGreater(counters["cut_monotonicity_transitions_checked"], 0)
        self.assertEqual(
            counters["full_gamma_cut_comparisons"],
            counters["reduced_gamma_cut_comparisons"],
        )
        self.assertEqual(
            counters["full_gamma_cut_comparisons"],
            counters["gabriel_partial_cut_comparisons"],
        )
        self.assertEqual(
            counters["gabriel_partial_cut_comparisons"],
            counters["gabriel_partial_positive_inclusion_comparisons"],
        )
        self.assertGreater(counters["reduced_gamma_batch_comparisons"], 0)
        self.assertGreater(counters["reduced_gamma_vertical_map_comparisons"], 0)
        self.assertGreater(counters["naturality_squares_checked"], 0)
        self.assertEqual(
            manifest["scientific_roles"]["hgp_reduced"]["relation"], "gamma"
        )
        self.assertTrue(
            manifest["scientific_roles"]["hgp_reduced"]["normative"]
        )
        self.assertEqual(
            manifest["scientific_roles"]["gabriel_raw"]["forest_semantics"],
            "partial_refinement",
        )
        self.assertEqual(
            manifest["scientific_roles"]["gabriel_raw"]["vertical_maps"],
            "not_constructed",
        )
        provenance = manifest["provenance"]
        self.assertEqual(provenance["runner"]["version"], "2.0.0")
        self.assertEqual(provenance["generator"]["version"], "1.0.0")
        self.assertEqual(len(provenance["runner"]["source_sha256"]), 64)
        self.assertEqual(len(provenance["generator"]["source_sha256"]), 64)
        self.assertTrue(provenance["repository"]["git_head"])
        self.assertIn(
            provenance["repository"]["worktree_state_at_start"],
            ("clean", "dirty"),
        )
        self.assertIn(
            provenance["repository"]["worktree_state_at_end"],
            ("clean", "dirty"),
        )
        self.assertFalse(provenance["clean_worktree_required"])
        input_roots = manifest["input_hash_roots"]
        self.assertEqual(input_roots["cloud_count"], 3)
        self.assertEqual(input_roots["case_count"], 3)
        self.assertEqual(len(input_roots["cloud_root_sha256"]), 64)
        self.assertEqual(len(input_roots["case_root_sha256"]), 64)
        self.assertTrue(input_roots["counts_match_attempted_work"])
        self.assertEqual(len(manifest["campaign_id"]), 64)
        self.assertGreater(manifest["timings_ns"]["total"], 0)
        self.assertFalse(
            manifest["comparison_scope"]["independent_implementation_differential"][
                "performed"
            ]
        )
        self.assertFalse(
            manifest["phase1_campaign_scale"][
                "runner_scale_requirements_satisfied"
            ]
        )
        self.assertTrue(
            manifest["phase1_campaign_scale"][
                "normative_gamma_checks_satisfied"
            ]
        )
        self.assertTrue(
            manifest["phase1_campaign_scale"][
                "gabriel_diagnostic_checks_satisfied"
            ]
        )
        self.assertFalse(
            manifest["phase1_campaign_scale"]["repository_phase_exit_claimed"]
        )
        self.assertFalse(
            manifest["phase1_campaign_scale"][
                "automatic_failure_fixture_enabled"
            ]
        )

    def test_ci_caps_are_strict_but_gate_scale_is_configurable(self) -> None:
        gate = CampaignConfig(
            dimensions=(1,),
            seeds_per_dimension=10_000,
            point_counts=(2,),
            k_max_values=(1,),
            permutations_per_seed=0,
        )
        self.assertEqual(gate.seeds_per_dimension, 10_000)
        seed_scale = CampaignConfig(
            dimensions=(1, 2, 3),
            seeds_per_dimension=10_000,
            point_counts=(4,),
            k_max_values=(4,),
            permutations_per_seed=1,
            metamorphic_stride=100,
            failure_dir=Path("failures"),
        )
        self.assertTrue(seed_scale.seed_scale_configuration_satisfied)
        self.assertFalse(all(seed_scale.matrix_coverage.values()))
        with self.assertRaises(ValueError):
            CampaignConfig(
                dimensions=(1,),
                seeds_per_dimension=10_000,
                point_counts=(2,),
                k_max_values=(1,),
                permutations_per_seed=0,
                ci=True,
            )

    def test_failure_is_minimized_and_written_only_to_explicit_directory(self) -> None:
        failure = CampaignFailure(
            invariant="injected_test_mismatch",
            message="synthetic campaign failure",
            context={"source": "unit_test"},
        )

        def injected_audit(points, **_kwargs):
            return failure if len(points) >= 3 else None

        base = CampaignConfig(
            dimensions=(1,),
            seeds_per_dimension=1,
            point_counts=(5,),
            k_max_values=(2,),
            permutations_per_seed=0,
        )
        with patch("tools.run_oracle_campaign._audit_case", side_effect=injected_audit):
            without_directory = run_campaign(base)
        self.assertEqual(without_directory["status"], "failed")
        self.assertIsNone(without_directory["failure"]["fixture_path"])
        self.assertEqual(without_directory["failure"]["minimized_point_count"], 3)

        with tempfile.TemporaryDirectory() as temporary_directory:
            failure_dir = Path(temporary_directory) / "explicit-failures"
            configured = CampaignConfig(
                dimensions=(1,),
                seeds_per_dimension=1,
                point_counts=(5,),
                k_max_values=(2,),
                permutations_per_seed=0,
                failure_dir=failure_dir,
            )
            with patch(
                "tools.run_oracle_campaign._audit_case", side_effect=injected_audit
            ):
                with_directory = run_campaign(configured)
            fixtures = tuple(failure_dir.glob("oracle-campaign-*.json"))
            self.assertEqual(len(fixtures), 1)
            payload = json.loads(fixtures[0].read_text(encoding="utf-8"))
            self.assertEqual(payload["schema_version"], 2)
            self.assertEqual(
                payload["reconstruction_contract_id"], "hgp-reduced-v2"
            )
            self.assertEqual(payload["minimized_failure"]["invariant"], failure.invariant)
            self.assertEqual(len(payload["minimized_points"]), 3)
            self.assertGreater(payload["minimization_evaluations"], 0)
            self.assertGreater(payload["coordinate_minimization_evaluations"], 0)
            self.assertGreater(payload["coordinate_reductions_accepted"], 0)
            self.assertNotEqual(
                payload["point_deletion_minimized_points"],
                payload["minimized_points"],
            )
            point_deleted_l1 = sum(
                abs(coordinate)
                for point in payload["point_deletion_minimized_points"]
                for coordinate in point
            )
            coordinate_minimized_l1 = sum(
                abs(coordinate)
                for point in payload["minimized_points"]
                for coordinate in point
            )
            self.assertLess(coordinate_minimized_l1, point_deleted_l1)
            self.assertEqual(
                with_directory["failure"]["fixture_path"], str(fixtures[0])
            )
            self.assertFalse(
                with_directory["phase1_campaign_scale"][
                    "normative_gamma_checks_satisfied"
                ]
            )
            self.assertFalse(
                with_directory["phase1_campaign_scale"][
                    "gabriel_diagnostic_checks_satisfied"
                ]
            )

    def test_success_does_not_create_an_explicit_failure_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            failure_dir = Path(temporary_directory) / "unused"
            manifest = run_campaign(
                CampaignConfig(
                    dimensions=(1,),
                    seeds_per_dimension=1,
                    point_counts=(4,),
                    k_max_values=(2,),
                    permutations_per_seed=0,
                    ci=True,
                    failure_dir=failure_dir,
                )
            )
            self.assertEqual(manifest["status"], "passed")
            self.assertFalse(failure_dir.exists())

    def test_cloud_and_case_hash_roots_are_deterministic(self) -> None:
        config = CampaignConfig(
            dimensions=(1,),
            seeds_per_dimension=1,
            point_counts=(4,),
            k_max_values=(2, 3),
            permutations_per_seed=0,
            metamorphic_stride=0,
            ci=True,
        )
        first = run_campaign(config)
        second = run_campaign(config)

        self.assertEqual(first["status"], "passed")
        self.assertEqual(second["status"], "passed")
        self.assertEqual(first["input_hash_roots"], second["input_hash_roots"])
        roots = first["input_hash_roots"]
        self.assertEqual(roots["cloud_count"], 1)
        self.assertEqual(roots["case_count"], 2)
        self.assertTrue(roots["counts_match_attempted_work"])

    def test_multi_n_multi_k_matrix_and_prefixes_are_explicit(self) -> None:
        manifest = run_campaign(
            CampaignConfig(
                dimensions=(1,),
                seeds_per_dimension=1,
                point_counts=(4, 5),
                k_max_values=(3, 4, 5),
                permutations_per_seed=0,
                metamorphic_stride=0,
                ci=True,
            )
        )
        self.assertEqual(manifest["status"], "passed")
        self.assertTrue(manifest["matrix_gate"]["configuration_satisfied"])
        self.assertTrue(manifest["matrix_gate"]["satisfied"])
        self.assertTrue(
            manifest["matrix_gate"]["may_be_aggregated_with_seed_scale_manifest"]
        )
        self.assertEqual(manifest["counters"]["clouds_generated"], 2)
        self.assertEqual(manifest["counters"]["campaign_cases_audited"], 6)
        self.assertEqual(manifest["counters"]["kmax_prefix_comparisons"], 6)
        scale = manifest["phase1_campaign_scale"]
        self.assertTrue(scale["matrix_audit_satisfied_in_this_manifest"])
        self.assertFalse(scale["runner_scale_requirements_satisfied"])
        self.assertFalse(
            scale[
                "combined_seed_and_matrix_requirements_satisfied_in_this_manifest"
            ]
        )
        self.assertTrue(scale["separate_matrix_manifest_may_be_aggregated"])

    def test_cli_prints_and_optionally_writes_the_same_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            manifest_path = Path(temporary_directory) / "campaign.json"
            completed = subprocess.run(
                [
                    sys.executable,
                    str(RUNNER),
                    "--ci",
                    "--dimensions",
                    "1",
                    "--seeds-per-dimension",
                    "1",
                    "--point-count",
                    "4",
                    "--k-max",
                    "2",
                    "--permutations-per-seed",
                    "0",
                    "--manifest",
                    str(manifest_path),
                ],
                cwd=ROOT,
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr)
            stdout_manifest = json.loads(completed.stdout)
            file_manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            self.assertEqual(stdout_manifest, file_manifest)
            self.assertEqual(stdout_manifest["status"], "passed")


if __name__ == "__main__":
    unittest.main()
