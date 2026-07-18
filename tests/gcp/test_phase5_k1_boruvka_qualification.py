#!/usr/bin/env python3
"""Static, mutation-free checks for the guarded Phase 5 G4 workflow."""

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
    / "assemble_phase5_k1_boruvka_qualification.py"
)


class Phase5K1BoruvkaGuardedWorkflowTests(unittest.TestCase):
    def test_shell_entrypoints_remain_syntactically_valid(self) -> None:
        subprocess.run(
            ["bash", "-n", str(RUNNER), str(WORKER)],
            cwd=ROOT,
            check=True,
        )

    def test_runner_binds_phase5_publication_to_targeted_stop(self) -> None:
        script = RUNNER.read_text(encoding="utf-8")
        self.assertIn("--phase5-k1-boruvka", script)
        self.assertIn(
            "phase5-k1-boruvka-${HEAD_SHA}.json",
            script,
        )
        self.assertIn(
            "--phase5-k1-boruvka-output ${quoted_phase5_artifact}",
            script,
        )
        self.assertIn(
            '--expected-last-start-timestamp "${SESSION_LAST_START_TIMESTAMP}"',
            script,
        )

        worker_call = script.index("${phase5_worker_option}")
        transfer = script.index('"${INSTANCE_NAME}:${remote_phase5_artifact}"')
        validation = script.index(
            "Artefact Phase 5 K1 Boruvka distant récupéré mais vide"
        )
        targeted_stop = script.rindex("if certify_target_stopped; then")
        final_publication = script.index(
            'python3 - "${LOCAL_TEMP_RESULT}" "${LOCAL_RESULT}"',
            targeted_stop,
        )
        final_message = script.index(
            "Résultat K1 Boruvka Phase 5 publié après certification TERMINATED"
        )
        self.assertLess(worker_call, transfer)
        self.assertLess(transfer, validation)
        self.assertLess(validation, targeted_stop)
        self.assertLess(targeted_stop, final_publication)
        self.assertLess(final_publication, final_message)

    def test_worker_runs_only_the_closed_phase5_evidence_chain(self) -> None:
        script = WORKER.read_text(encoding="utf-8")
        self.assertIn("--phase5-k1-boruvka-output", script)
        self.assertIn(
            'if [[ -n "${PHASE5_OUTPUT_PATH}" ]]; then',
            script,
        )
        for required in (
            "phase5-k1-boruvka-replay",
            "phase5-k1-boruvka-cuobjdump-elf",
            "phase5-k1-boruvka-cuobjdump-ptx",
            "phase5-k1-boruvka-memcheck",
            "phase5-k1-boruvka-racecheck",
            "-lelf",
            "-lptx",
            "--tool memcheck",
            "--tool racecheck",
            '"${PHASE5_K1_BORUVKA_ASSEMBLER}"',
            'value.get("status") != "worker_passed_pending_shutdown"',
        ):
            self.assertIn(required, script)
        self.assertNotIn("gcloud compute instances start", script)
        self.assertNotIn("gcloud compute instances stop", script)
        self.assertIn(
            'run_container_split_output "phase5-k1-boruvka-replay"',
            script,
        )
        replay_call = script.index(
            'run_container_split_output "phase5-k1-boruvka-replay"'
        )
        replay_call_end = script.index("; then", replay_call)
        replay_call_source = script[replay_call:replay_call_end]
        self.assertIn('"${PHASE5_K1_BORUVKA_REPLAY_LOG}"', replay_call_source)
        self.assertIn(
            '"${PHASE5_K1_BORUVKA_REPLAY_STDERR_LOG}"',
            replay_call_source,
        )
        self.assertIn('"${PHASE5_K1_BORUVKA_REPLAY_PATH}"', replay_call_source)

        replay = script.index('begin_unit "phase5-k1-boruvka-replay"')
        elf = script.index('begin_unit "phase5-k1-boruvka-cuobjdump-elf"')
        ptx = script.index('begin_unit "phase5-k1-boruvka-cuobjdump-ptx"')
        memcheck = script.index('begin_unit "phase5-k1-boruvka-memcheck"')
        racecheck = script.index('begin_unit "phase5-k1-boruvka-racecheck"')
        assemble = script.index('python3 -B "${PHASE5_K1_BORUVKA_ASSEMBLER}"')
        publish = script.index(
            'python3 - "${PUBLISH_TEMP}" "${OUTPUT_PATH}"',
            assemble,
        )
        self.assertLess(replay, elf)
        self.assertLess(elf, ptx)
        self.assertLess(ptx, memcheck)
        self.assertLess(memcheck, racecheck)
        self.assertLess(racecheck, assemble)
        self.assertLess(assemble, publish)

    def test_phase5_assembler_is_present_and_closed(self) -> None:
        self.assertTrue(ASSEMBLER.is_file())
        source = ASSEMBLER.read_text(encoding="utf-8")
        for required in (
            "morsehgp3d.phase5.k1_boruvka_gpu_qualification.v1",
            "gpu_candidate_superset_with_cpu_exact_resolution_only",
            "worker_passed_pending_shutdown",
            "require_exact_keys",
            "validate_memcheck_log",
            "validate_racecheck_log",
        ):
            self.assertIn(required, source)


if __name__ == "__main__":
    unittest.main()
