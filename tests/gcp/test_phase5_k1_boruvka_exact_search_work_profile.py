#!/usr/bin/env python3
"""Static checks for the guarded Phase 5 exact-search work profile."""

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
CHECKER = (
    ROOT
    / "morsehgp3d"
    / "tests"
    / "configuration"
    / "check_phase5_k1_boruvka_exact_search_work_profile.py"
)
ASSEMBLER = (
    ROOT
    / "morsehgp3d"
    / "tests"
    / "cuda"
    / "assemble_phase5_k1_boruvka_exact_search_work_profile.py"
)


class Phase5K1BoruvkaExactSearchWorkProfileTests(unittest.TestCase):
    def test_shell_entrypoints_remain_syntactically_valid(self) -> None:
        subprocess.run(
            ["bash", "-n", str(RUNNER), str(WORKER)],
            cwd=ROOT,
            check=True,
        )

    def test_worker_checks_every_cell_before_assembly(self) -> None:
        script = WORKER.read_text(encoding="utf-8")
        for required in (
            "--phase5-k1-boruvka-exact-search-work-profile-output",
            "morsehgp3d_gpu_k1_boruvka_exact_search_work_profile",
            "for point_count in 1024 4096 16384",
            "for family in uniform clusters lattice",
            "--window-radius 16",
            "--seed 1",
            '--git-sha "${HEAD_SHA}"',
            '"${#PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_LOGS[@]}" -eq 9',
            '"${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_CHECKER}"',
            "--profile-log",
            "--expected-backend cuda_g4",
            '--family "${family}"',
            '--point-count "${point_count}"',
            'python3 -B "${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_ASSEMBLER}"',
            "--environment-artifact",
            "--binary",
            "worker_passed_pending_shutdown",
        ):
            self.assertIn(required, script)

        first_cell = script.index("for point_count in 1024 4096 16384")
        checker = script.index(
            '"${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_CHECKER}"',
            first_cell,
        )
        matrix_closed = script.index(
            '"${#PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_LOGS[@]}" -eq 9',
            checker,
        )
        assemble = script.index(
            'python3 -B "${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_ASSEMBLER}"',
            matrix_closed,
        )
        publish = script.index('python3 - "${PUBLISH_TEMP}" "${OUTPUT_PATH}"', assemble)
        self.assertLess(first_cell, checker)
        self.assertLess(checker, matrix_closed)
        self.assertLess(matrix_closed, assemble)
        self.assertLess(assemble, publish)

    def test_runner_publishes_only_after_targeted_stop(self) -> None:
        script = RUNNER.read_text(encoding="utf-8")
        for required in (
            "--phase5-k1-boruvka-exact-search-work-profile",
            "phase5-k1-boruvka-exact-search-work-profile-${HEAD_SHA}.json",
            "--phase5-k1-boruvka-exact-search-work-profile-output",
            "import assemble_phase5_k1_boruvka_exact_search_work_profile as assembler",
            "assembler.SCHEMA",
            "benchmark_only",
            "worker_passed_pending_shutdown",
            "qualification_claimed",
            "scalability_claimed",
            "scientific_public_status",
            "certify_target_stopped",
            "final_phase3_sha256 = hashlib.sha256",
            'provenance["environment_artifact_sha256"] = final_phase3_sha256',
        ):
            self.assertIn(required, script)

        worker_call = script.index("${phase5_exact_search_work_profile_worker_option}")
        transfer = script.index(
            '"${INSTANCE_NAME}:${remote_phase5_exact_search_work_profile_artifact}"'
        )
        validation = script.index("Artefact du profil exact-search Phase 5 distant")
        targeted_stop = script.rindex("if certify_target_stopped; then")
        publication = script.index(
            'python3 - "${LOCAL_TEMP_RESULT}" "${LOCAL_RESULT}"',
            targeted_stop,
        )
        self.assertLess(worker_call, transfer)
        self.assertLess(transfer, validation)
        self.assertLess(validation, targeted_stop)
        self.assertLess(targeted_stop, publication)

    def test_finalizer_rebinds_the_new_ninth_and_tenth_arguments(self) -> None:
        script = RUNNER.read_text(encoding="utf-8")
        anchor = script.index("final_phase3_sha256 = hashlib.sha256")
        code_begin = script.rindex("<<'PY'\n", 0, anchor) + len("<<'PY'\n")
        code_end = script.index("\nPY\n", anchor)
        finalizer = script[code_begin:code_end]

        with tempfile.TemporaryDirectory(
            prefix="phase5-exact-search-finalizer-"
        ) as raw:
            directory = Path(raw)
            phase3_temporary = directory / "phase3.partial"
            phase3_final = directory / "phase3.json"
            companion_temporary = directory / "exact-search.partial"
            companion_final = directory / "exact-search.json"
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
                    "",
                    "",
                    str(companion_temporary),
                    str(companion_final),
                    "project",
                    "zone",
                    "instance",
                    "45",
                    "TERMINATED",
                    "2026-07-19T09:25:18Z",
                    "2026-07-19T02:18:17.482-07:00",
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

    def test_assembler_contract_stays_benchmark_only(self) -> None:
        checker = CHECKER.read_text(encoding="utf-8")
        assembler = ASSEMBLER.read_text(encoding="utf-8")
        for required in (
            "morsehgp3d.phase5.k1_boruvka_exact_search_work_profile.v1",
            "certified_local_emst_chain_work_profile_without_scalability_claim",
            '"artifact_role": "benchmark_only"',
            '"window_radius"',
            "validate_document",
        ):
            self.assertIn(required, checker)
        for required in (
            "morsehgp3d.phase5.k1_boruvka_exact_search_work_profile_artifact.v1",
            "SCHEMA as WORK_PROFILE_SCHEMA",
            "SCOPE as SCIENTIFIC_SCOPE",
            '"artifact_role": "benchmark_only"',
            '"work_profile_sha256"',
            "for point_count in (1024, 4096, 16384)",
            'for family in ("uniform", "clusters", "lattice")',
            "EXPECTED_RADIUS = 16",
            "EXPECTED_SEED = 1",
            'value.get("policy") != {"window_radius": EXPECTED_RADIUS}',
            '"qualification_claimed": False',
            '"scalability_claimed": False',
            '"scientific_public_status": None',
            '"scientific_result_claimed": False',
            '"hierarchy_reduction_status": "not_performed"',
            '"status": "worker_passed_pending_shutdown"',
            "validate_document",
        ):
            self.assertIn(required, assembler)


if __name__ == "__main__":
    unittest.main()
