from __future__ import annotations

from pathlib import Path
import stat
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
PREFLIGHT = ROOT / "gcp-migration/check_point_hierarchy_quality_preflight.py"
ORCHESTRATOR = ROOT / "gcp-migration/run_phase3_qualification.sh"
REMOTE_WORKER = ROOT / "gcp-migration/point_hierarchy_quality_remote_worker.sh"
PLAN_V2 = (
    ROOT
    / "morsehgp3d/tests/profiling/point_hierarchy_quality_campaign_v2.json"
)
GIT_SHA = "a" * 40


class PointHierarchyQualityGuardTests(unittest.TestCase):
    def run_closed_preflight(
        self,
    ) -> tuple[subprocess.CompletedProcess[str], bool, bool]:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            producer_marker = root / "producer-was-called"
            verifier_marker = root / "verifier-was-called"
            producer = root / "quality-producer"
            verifier = root / "quality-verifier"
            for executable, marker in (
                (producer, producer_marker),
                (verifier, verifier_marker),
            ):
                executable.write_text(
                    "#!/usr/bin/env python3\n"
                    "from pathlib import Path\n"
                    f"Path({str(marker)!r}).write_text('called', encoding='utf-8')\n"
                    "print('{}')\n",
                    encoding="utf-8",
                )
                executable.chmod(executable.stat().st_mode | stat.S_IXUSR)
            completed = subprocess.run(
                [
                    str(PREFLIGHT),
                    "--plan",
                    str(PLAN_V2),
                    "--producer",
                    str(producer),
                    "--verifier",
                    str(verifier),
                    "--expected-git-sha",
                    GIT_SHA,
                ],
                capture_output=True,
                text=True,
                timeout=10,
            )
            return completed, producer_marker.exists(), verifier_marker.exists()

    def test_closed_v2_plan_refuses_before_both_handshakes(self) -> None:
        completed, producer_called, verifier_called = self.run_closed_preflight()
        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("campaign entry gate is closed", completed.stderr)
        self.assertFalse(producer_called)
        self.assertFalse(verifier_called)

    def test_preflight_requires_distinct_producer_and_verifier(self) -> None:
        text = PREFLIGHT.read_text(encoding="utf-8")
        self.assertIn("producer_path == verifier_path", text)
        self.assertIn("producer_sha256 == verifier_sha256", text)
        self.assertIn("campaign.PRODUCER_CAPABILITY_ARGUMENT", text)
        self.assertIn("campaign.VERIFIER_CAPABILITY_ARGUMENT", text)

    def test_orchestrator_v2_preflight_precedes_key_and_start(self) -> None:
        text = ORCHESTRATOR.read_text(encoding="utf-8")
        preflight = text.index('python3 -B "${point_quality_preflight}"')
        fetch = text.index('git -C "${REPOSITORY_ROOT}" fetch')
        create_key = text.index("\ncreate_session_ssh_key ||")
        start = text.index('\n"${START_SCRIPT}" --yes')
        self.assertLess(preflight, fetch)
        self.assertLess(preflight, create_key)
        self.assertLess(preflight, start)
        self.assertIn("point_hierarchy_quality_campaign_v2.json", text)
        self.assertNotIn("point_hierarchy_quality_campaign_v1.json", text)
        self.assertIn('--producer "${POINT_HIERARCHY_QUALITY_PRODUCER}"', text)
        self.assertIn('--verifier "${POINT_HIERARCHY_QUALITY_VERIFIER}"', text)

    def test_remote_worker_is_v2_resumable_and_has_no_gcp_command(self) -> None:
        text = REMOTE_WORKER.read_text(encoding="utf-8")
        self.assertNotIn("gcloud compute", text)
        self.assertIn("quality_gcp_worker.v2", text)
        self.assertIn("point_hierarchy_quality_campaign_v2.json", text)
        self.assertIn('--producer "${PRODUCER_PATH}"', text)
        self.assertIn('--verifier "${VERIFIER_PATH}"', text)
        self.assertIn('--spool "${SPOOL_PATH}"', text)
        self.assertIn('--stop-before-epoch "${CAMPAIGN_STOP_BEFORE_EPOCH}"', text)
        self.assertIn('"persistent_across_guarded_sessions": True', text)
        self.assertIn('"resume_from_next_uncommitted_ordinal": True', text)
        self.assertIn('"worker_mutates_gcp": False', text)
        cleanup = text[text.index("cleanup() {") : text.index("trap cleanup EXIT")]
        self.assertNotIn('rm -rf -- "${SPOOL_PATH}"', cleanup)
        self.assertNotIn('rm -f -- "${SPOOL_PATH}"', cleanup)

    def test_spool_is_outside_ephemeral_session_directory(self) -> None:
        text = ORCHESTRATOR.read_text(encoding="utf-8")
        assignment = (
            'remote_point_hierarchy_quality_spool="/var/tmp/'
            'morsehgp3d-point-quality-spool-v2-${HEAD_SHA}"'
        )
        self.assertIn(assignment, text)
        self.assertNotIn(
            'remote_point_hierarchy_quality_spool="${REMOTE_WORKDIR}', text
        )

    def test_final_publication_follows_targeted_terminated_readback(self) -> None:
        text = ORCHESTRATOR.read_text(encoding="utf-8")
        stopped = text.index("\nif certify_target_stopped; then")
        publication = text.index("os.link(temporary, target, follow_symlinks=False)")
        quality_final_status = text.index(
            'value["status"] = campaign_status', stopped
        )
        self.assertLess(stopped, quality_final_status)
        self.assertLess(stopped, publication)
        self.assertIn("TARGET_STOP_CERTIFIED=1", text[:publication])
        self.assertIn("worker_partial_resumable_pending_shutdown", text)
        self.assertIn("worker_complete_pending_shutdown", text)


if __name__ == "__main__":
    unittest.main()
