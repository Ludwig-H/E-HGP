#!/usr/bin/env python3
"""Static checks for the guarded Phase 5 Morton work-profile workflow."""

from __future__ import annotations

from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "gcp-migration" / "run_phase3_qualification.sh"
WORKER = ROOT / "gcp-migration" / "phase3_remote_qualification.sh"
ASSEMBLER = (
    ROOT
    / "morsehgp3d"
    / "tests"
    / "cuda"
    / "assemble_phase5_k1_boruvka_work_profile.py"
)


class Phase5K1BoruvkaWorkProfileGuardedWorkflowTests(unittest.TestCase):
    def test_shell_entrypoints_remain_syntactically_valid(self) -> None:
        subprocess.run(
            ["bash", "-n", str(RUNNER), str(WORKER)],
            cwd=ROOT,
            check=True,
        )

    def test_worker_runs_the_closed_nine_cell_matrix(self) -> None:
        script = WORKER.read_text(encoding="utf-8")
        for required in (
            "--phase5-k1-boruvka-work-profile-output",
            "morsehgp3d_gpu_k1_boruvka_morton_work_profile",
            "for point_count in 64 256 1024",
            "for family in uniform clusters lattice",
            "candidate_record_budget=$((point_count - 1))",
            "--window-radii 1,4,16",
            "--seed 1",
            "--git-sha \"${HEAD_SHA}\"",
            '"${#PHASE5_K1_BORUVKA_WORK_PROFILE_LOGS[@]}" -eq 9',
            'python3 -B "${PHASE5_K1_BORUVKA_WORK_PROFILE_ASSEMBLER}"',
            "--environment-artifact",
            "--profile-log",
            "--binary",
            "worker_passed_pending_shutdown",
        ):
            self.assertIn(required, script)
        self.assertNotIn("gcloud compute instances start", script)
        self.assertNotIn("gcloud compute instances stop", script)
        self.assertNotIn(
            "Le profil de travail Morton Phase 5 a écrit sur stderr",
            script,
        )

        first_cell = script.index("for point_count in 64 256 1024")
        matrix_closed = script.index(
            '"${#PHASE5_K1_BORUVKA_WORK_PROFILE_LOGS[@]}" -eq 9'
        )
        assemble = script.index(
            'python3 -B "${PHASE5_K1_BORUVKA_WORK_PROFILE_ASSEMBLER}"'
        )
        publish = script.index(
            'python3 - "${PUBLISH_TEMP}" "${OUTPUT_PATH}"', assemble
        )
        self.assertLess(first_cell, matrix_closed)
        self.assertLess(matrix_closed, assemble)
        self.assertLess(assemble, publish)

    def test_runner_publishes_only_after_targeted_stop(self) -> None:
        script = RUNNER.read_text(encoding="utf-8")
        for required in (
            "--phase5-k1-boruvka-work-profile",
            "phase5-k1-boruvka-work-profile-${HEAD_SHA}.json",
            "--phase5-k1-boruvka-work-profile-output",
            "import assemble_phase5_k1_boruvka_work_profile as assembler",
            "assembler.SCHEMA",
            "benchmark_only",
            "empirical_morton_work_profile_only",
            "worker_passed_pending_shutdown",
            "qualification_claimed",
            "scalability_claimed",
            "hierarchy_reduction_status",
            "scientific_public_status",
            "certify_target_stopped",
        ):
            self.assertIn(required, script)

        worker_call = script.index(
            "${phase5_work_profile_worker_option}"
        )
        transfer = script.index(
            '"${INSTANCE_NAME}:${remote_phase5_work_profile_artifact}"'
        )
        validation = script.index(
            "Artefact du profil de travail Morton Phase 5 distant"
        )
        targeted_stop = script.rindex("if certify_target_stopped; then")
        final_publication = script.index(
            'python3 - "${LOCAL_TEMP_RESULT}" "${LOCAL_RESULT}"',
            targeted_stop,
        )
        self.assertLess(worker_call, transfer)
        self.assertLess(transfer, validation)
        self.assertLess(validation, targeted_stop)
        self.assertLess(targeted_stop, final_publication)

    def test_assembler_stays_empirical_and_fail_closed(self) -> None:
        source = ASSEMBLER.read_text(encoding="utf-8")
        for required in (
            "morsehgp3d.phase5.k1_boruvka_morton_work_profile_artifact.v1",
            "SCHEMA as WORK_PROFILE_SCHEMA",
            "benchmark_only",
            "empirical_morton_work_profile_only",
            "worker_passed_pending_shutdown",
            '"qualification_claimed": False',
            '"scalability_claimed": False',
            '"scientific_public_status": None',
            '"scientific_result_claimed": False',
            '"hierarchy_reduction_status": "not_performed"',
            "EXPECTED_MATRIX",
            "validate_document",
        ):
            self.assertIn(required, source)


if __name__ == "__main__":
    unittest.main()
