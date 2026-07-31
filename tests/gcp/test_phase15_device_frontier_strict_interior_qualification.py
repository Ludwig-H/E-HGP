#!/usr/bin/env python3
"""Host tests for the guarded q=3 Gabriel negative-lane qualification."""

from __future__ import annotations

import copy
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
CUDA_TESTS = ROOT / "morsehgp3d" / "tests" / "cuda"
sys.path.insert(0, str(CUDA_TESTS))

import assemble_phase15_device_frontier_strict_interior_qualification as assembler

GIT_SHA = "1" * 40


def rank_result(
    *,
    semantics: str,
    maximum_closed_rank: int,
    candidate_mass: int,
    negative_mass: int,
) -> dict[str, object]:
    value: dict[str, object] = {
        key: 0 for key in assembler.RANK_KEYS - {"prune_semantics"}
    }
    for key in assembler.RANK_BOOLEAN_KEYS:
        value[key] = True
    value.update(
        {
            "bounded_admitted_pair_count": candidate_mass,
            "bounded_candidate_admitted_pair_count": candidate_mass,
            "bounded_exact_pair_count": 6,
            "candidate_pair_mass": candidate_mass,
            "certified_pruned_pair_mass": negative_mass,
            "fresh_tile_device_arena_allocation_count": 1,
            "fresh_tile_device_arena_reuse_count": 0,
            "gamma2_prune_or_discard_authorized": False,
            "gamma2_silent_handoff_required":
                semantics == "strict_interior_threshold",
            "maximum_closed_rank": maximum_closed_rank,
            "prune_semantics": semantics,
            "q3_exact_diametral_pair_support_gabriel_negative_only":
                semantics == "strict_interior_threshold",
            "required_witness_count": 2,
            "unordered_pair_universe_count": 6,
        }
    )
    return value


def case(
    cloud: str,
    maximum_closed_rank: int,
    tile: int,
    *,
    semantics: str = "strict_interior_threshold",
) -> dict[str, object]:
    negative_mass = (
        0
        if semantics == "strict_interior_threshold" and cloud == "boundary_shell"
        else 1
    )
    return {
        "anchor_tile_capacity": tile,
        "cloud": cloud,
        "cloud_digest_fnv1a": 101 if cloud == "boundary_shell" else 202,
        "expected_candidate_pair_mass": 6 - negative_mass,
        "expected_certified_pruned_pair_mass": negative_mass,
        "maximum_closed_rank_metadata": maximum_closed_rank,
        "rank_result": rank_result(
            semantics=semantics,
            maximum_closed_rank=maximum_closed_rank,
            candidate_mass=6 - negative_mass,
            negative_mass=negative_mass,
        ),
        "target_ax_negative_covered": negative_mass == 1,
    }


def qualification() -> dict[str, object]:
    strict_cases = [
        case(cloud, maximum_closed_rank, tile)
        for cloud in ("boundary_shell", "strict_interior")
        for maximum_closed_rank in (2, 11)
        for tile in (1, 4)
    ]
    return {
        "backend": assembler.BACKEND,
        "boundary_shell_preserved": True,
        "closed_boundary_control": case(
            "boundary_shell", 3, 4, semantics="closed_rank_window"
        ),
        "closed_boundary_control_negative_covered": True,
        "component_only": True,
        "deployment_status": assembler.DEPLOYMENT_STATUS,
        "exact_all_pair_all_witness_oracle": True,
        "fixture": assembler.FIXTURE,
        "gamma2_catalog_published": False,
        "gamma2_prune_or_discard": False,
        "gamma2_silent_handoff_required": True,
        "git_sha": GIT_SHA,
        "global_cell_or_coface_arena_materialized": False,
        "global_pair_matrix_materialized": False,
        "hierarchy_reduction_performed": False,
        "maximum_closed_rank_metadata_invariance_qualified": True,
        "mode": assembler.MODE,
        "morse_hgp_hierarchy_claimed": False,
        "negative_region_interpretation": (
            "no_q3_gabriel_triangle_with_exact_diametral_pair_minimal_"
            "support_only"
        ),
        "ordinary_delaunay_materialized": False,
        "pair_frontier_negative_regions_are_gamma2_discard": False,
        "point_count_per_cloud": 4,
        "profile": assembler.PROFILE,
        "public_status": assembler.PUBLIC_STATUS,
        "q": 3,
        "q3_exact_diametral_pair_support_gabriel_negative_only": True,
        "required_witness_count": 2,
        "run_count": 9,
        "schema": assembler.QUALIFICATION_SCHEMA,
        "scientific_pair_catalog_published": False,
        "scientific_scope": assembler.SCIENTIFIC_SCOPE,
        "strict_cases": strict_cases,
        "strict_interior_target_negative_covered": True,
        "success": True,
        "tile_capacity_invariance_qualified": True,
        "total_ns": 10,
    }


class QualificationContractTest(unittest.TestCase):
    def test_accepts_exact_diametral_pair_support_negative_scope(self) -> None:
        value = qualification()
        self.assertIs(
            assembler.validate_qualification(value, git_sha=GIT_SHA), value
        )

    def assert_rejected(self, mutate) -> None:
        value = copy.deepcopy(qualification())
        mutate(value)
        with self.assertRaises(SystemExit):
            assembler.validate_qualification(value, git_sha=GIT_SHA)

    def test_rejects_gamma2_or_hierarchy_claims(self) -> None:
        for field in (
            "gamma2_catalog_published",
            "gamma2_prune_or_discard",
            "hierarchy_reduction_performed",
            "morse_hgp_hierarchy_claimed",
            "pair_frontier_negative_regions_are_gamma2_discard",
        ):
            with self.subTest(field=field):
                self.assert_rejected(lambda value, field=field: value.__setitem__(field, True))

    def test_rejects_missing_q3_scope_or_gamma2_handoff(self) -> None:
        for field in (
            "gamma2_silent_handoff_required",
            "q3_exact_diametral_pair_support_gabriel_negative_only",
        ):
            with self.subTest(field=field):
                self.assert_rejected(
                    lambda value, field=field: value.__setitem__(field, False)
                )

    def test_rejects_shell_as_strict_negative(self) -> None:
        def mutate(value: dict[str, object]) -> None:
            shell = value["strict_cases"][0]
            shell["target_ax_negative_covered"] = True

        self.assert_rejected(mutate)

    def test_rejects_missing_strict_negative(self) -> None:
        def mutate(value: dict[str, object]) -> None:
            strict = value["strict_cases"][4]
            strict["target_ax_negative_covered"] = False

        self.assert_rejected(mutate)

    def test_rejects_wrong_witness_contract(self) -> None:
        def mutate(value: dict[str, object]) -> None:
            value["strict_cases"][0]["rank_result"]["required_witness_count"] = 1

        self.assert_rejected(mutate)

    def test_sanitizer_comparison_ignores_timings_only(self) -> None:
        direct = qualification()
        replay = copy.deepcopy(direct)
        replay["total_ns"] += 1
        replay["strict_cases"][0]["rank_result"]["launcher_ns"] += 1
        replay["strict_cases"][0]["rank_result"]["minimum_device_free_bytes"] += 1
        assembler.validate_scientific_equivalence(direct, replay, replay)
        replay["strict_cases"][0]["rank_result"]["candidate_pair_mass"] = 5
        with self.assertRaises(SystemExit):
            assembler.validate_scientific_equivalence(direct, replay, replay)


class GuardedWiringTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.worker = (ROOT / "gcp-migration" / "phase3_remote_qualification.sh").read_text(
            encoding="utf-8"
        )
        cls.orchestrator = (
            ROOT / "gcp-migration" / "run_phase3_qualification.sh"
        ).read_text(encoding="utf-8")

    def test_worker_uses_full_native_scope_without_smoke_flags(self) -> None:
        marker = "--phase15-device-frontier-strict-interior-output"
        self.assertIn(marker, self.worker)
        begin = self.worker.index(
            'if [[ -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}" ]]; then\n'
            '    begin_unit "phase15-device-frontier-strict-interior-release-build"'
        )
        end = self.worker.index(
            'if [[ -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}" ]]; then',
            begin,
        )
        strict_block = self.worker[begin:end]
        self.assertIn("--prune-semantics strict_interior_threshold", strict_block)
        self.assertIn(
            "--scientific-scope "
            "q3_gabriel_exact_diametral_pair_support_negative_only",
            strict_block,
        )
        self.assertIn("--fixture q3_shell_strict_discriminant", strict_block)
        self.assertNotIn("--scaling-smoke", strict_block)
        self.assertNotIn("--single-run", strict_block)

    def test_orchestrator_validates_before_targeted_stop(self) -> None:
        retrieval = self.orchestrator.index(
            "${remote_phase15_device_frontier_strict_interior_artifact}"
        )
        validation = self.orchestrator.index(
            "import assemble_phase15_device_frontier_strict_interior_qualification"
        )
        stop = self.orchestrator.index("if certify_target_stopped;", validation)
        publication = self.orchestrator.index(
            "paire de publication q3 Gabriel négatif", stop
        )
        self.assertLess(retrieval, validation)
        self.assertLess(validation, stop)
        self.assertLess(stop, publication)


if __name__ == "__main__":
    unittest.main()
