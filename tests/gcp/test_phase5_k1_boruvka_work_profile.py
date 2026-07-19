#!/usr/bin/env python3
"""Static checks for the guarded Phase 5 Morton work-profile workflow."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
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
            "final_phase3_sha256 = hashlib.sha256",
            'provenance["environment_artifact_sha256"] = final_phase3_sha256',
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

    def test_finalizer_rebinds_companion_to_published_phase3(self) -> None:
        script = RUNNER.read_text(encoding="utf-8")
        anchor = script.index("final_phase3_sha256 = hashlib.sha256")
        code_begin = script.rindex("<<'PY'\n", 0, anchor) + len("<<'PY'\n")
        code_end = script.index("\nPY\n", anchor)
        finalizer = script[code_begin:code_end]

        with tempfile.TemporaryDirectory(prefix="phase5-profile-finalizer-") as raw:
            directory = Path(raw)
            phase3_temporary = directory / "phase3.partial"
            phase3_final = directory / "phase3.json"
            companion_temporary = directory / "profile.partial"
            companion_final = directory / "profile.json"
            phase3_temporary.write_text(
                json.dumps(
                    {
                        "status": "worker_passed_pending_shutdown",
                        "vm_lifecycle": {},
                    },
                    sort_keys=True,
                    separators=(",", ":"),
                )
                + "\n",
                encoding="utf-8",
            )
            companion_temporary.write_text(
                json.dumps(
                    {
                        "provenance": {
                            "environment_artifact_schema": (
                                "morsehgp3d.phase3.qualification.v1"
                            ),
                            "environment_artifact_sha256": "0" * 64,
                        },
                        "status": "worker_passed_pending_shutdown",
                        "vm_lifecycle": {},
                    },
                    sort_keys=True,
                    separators=(",", ":"),
                )
                + "\n",
                encoding="utf-8",
            )
            subprocess.run(
                [
                    sys.executable,
                    "-c",
                    finalizer,
                    str(phase3_temporary),
                    str(phase3_final),
                    "",
                    "",
                    "",
                    "",
                    str(companion_temporary),
                    str(companion_final),
                    "project",
                    "zone",
                    "instance",
                    "45",
                    "TERMINATED",
                    "2026-07-19T09:09:39Z",
                    "2026-07-19T02:02:31.682-07:00",
                ],
                cwd=ROOT,
                check=True,
            )
            phase3_bytes = phase3_final.read_bytes()
            companion = json.loads(companion_final.read_text(encoding="utf-8"))
            self.assertEqual(
                companion["provenance"]["environment_artifact_sha256"],
                hashlib.sha256(phase3_bytes).hexdigest(),
            )
            self.assertEqual(companion["status"], "passed")
            self.assertTrue(companion["vm_lifecycle"]["targeted_stop_verified"])

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
