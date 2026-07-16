"""Regression tests for fail-closed GCP session guards."""

from __future__ import annotations

import importlib.util
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FAKE_GCLOUD = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import signal
import sys
import time
from datetime import datetime, timedelta, timezone

args = sys.argv[1:]
log = os.environ.get("FAKE_GCLOUD_LOG")
if log:
    with open(log, "a", encoding="utf-8") as stream:
        stream.write(" ".join(args) + "\n")

scenario = os.environ.get("FAKE_GCLOUD_SCENARIO", "wrong-machine")
if args[:3] == ["config", "get-value", "project"]:
    print(os.environ.get("GCP_PROJECT_ID", "devpod-gpu-exploration"))
elif args[:3] == ["config", "get-value", "account"]:
    print("tester@example.invalid")
elif args[:3] == ["compute", "instances", "list"]:
    output_format = next((item.split("=", 1)[1] for item in args if item.startswith("--format=")), "")
    if output_format == "value(name)":
        print(os.environ.get("GCP_INSTANCE_NAME", "ehgp-blackwell-spot"))
    elif output_format.startswith("csv[no-heading]"):
        if scenario == "other-active":
            print("ehgp-concurrent,europe-west4-b,RUNNING")
elif args[:3] == ["compute", "instances", "describe"]:
    if scenario == "qualification-stop-unreadable":
        print("simulated unreadable target", file=sys.stderr)
        sys.exit(98)
    output_format = next((item.split("=", 1)[1] for item in args if item.startswith("--format=")), "")
    field = output_format.removeprefix("value(").removesuffix(")")
    previous_commands = ""
    if log and os.path.exists(log):
        with open(log, encoding="utf-8") as stream:
            previous_commands = stream.read()
    status = "TERMINATED"
    if scenario == "guest-readback" and "compute instances start" in previous_commands:
        status = "RUNNING"
    if "compute instances stop" in previous_commands:
        status = "TERMINATED"
    now = datetime.now(timezone.utc)
    values = {
        "status": status,
        "scheduling.instanceTerminationAction": "STOP",
        "scheduling.maxRunDuration.seconds": "28800",
        "labels.project": "e-hgp",
        "machineType.basename()": "n2-standard-48" if scenario == "wrong-machine" else "g4-standard-48",
        "scheduling.provisioningModel": "STANDARD" if scenario == "wrong-provisioning" else "SPOT",
        "lastStartTimestamp": (now - timedelta(seconds=10)).isoformat().replace("+00:00", "Z"),
        "terminationTimestamp": (now + timedelta(seconds=28790)).isoformat().replace("+00:00", "Z"),
    }
    if scenario == "unlabelled":
        values["labels.project"] = ""
    print(values.get(field, ""))
elif args[:3] in (["compute", "instances", "start"], ["compute", "instances", "stop"]):
    if scenario != "guest-readback":
        print("unexpected mutation", file=sys.stderr)
        sys.exit(97)
elif args[:2] == ["compute", "ssh"]:
    command = next(
        (item.split("=", 1)[1] for item in args if item.startswith("--command=")),
        "",
    )
    if scenario == "guest-readback":
        print("No scheduled shutdown")
    elif scenario.startswith("qualification-"):
        if "mktemp -d /tmp/morsehgp3d-phase3.XXXXXXXX" in command:
            print("__EHGP_REMOTE_DIR__/tmp/morsehgp3d-phase3.A1b2C3d4")
        elif "git clone --quiet" in command:
            print(
                "__EHGP_REMOTE_HEAD__"
                + os.environ.get(
                    "FAKE_REMOTE_HEAD", os.environ.get("FAKE_GIT_HEAD", "a" * 40)
                )
            )
        elif "phase3_remote_qualification.sh" in command:
            if scenario == "qualification-remote-failure":
                print("simulated remote qualification failure", file=sys.stderr)
                sys.exit(42)
            if scenario == "qualification-interrupt":
                os.kill(os.getppid(), signal.SIGTERM)
                time.sleep(0.05)
                sys.exit(143)
        elif command.startswith("rm -rf -- /tmp/morsehgp3d-phase3."):
            pass
        else:
            print("unsupported qualification ssh command: " + command, file=sys.stderr)
            sys.exit(95)
    else:
        print("unexpected ssh", file=sys.stderr)
        sys.exit(97)
elif args[:2] == ["compute", "scp"] and scenario.startswith("qualification-"):
    if len(args) < 4:
        print("missing fake scp paths", file=sys.stderr)
        sys.exit(94)
    Path(args[3]).write_text('{"kind":"phase3-result","status":"passed"}\n', encoding="utf-8")
else:
    print("unsupported fake gcloud command: " + " ".join(args), file=sys.stderr)
    sys.exit(96)
"""

FAKE_GIT = r"""#!/usr/bin/env python3
import os
import sys

args = sys.argv[1:]
if args[:1] == ["-C"]:
    args = args[2:]

log = os.environ.get("FAKE_GIT_LOG")
if log:
    with open(log, "a", encoding="utf-8") as stream:
        stream.write(" ".join(args) + "\n")

scenario = os.environ.get("FAKE_GIT_SCENARIO", "clean")
head = os.environ.get("FAKE_GIT_HEAD", "a" * 40)
if args[:2] == ["rev-parse", "--show-toplevel"]:
    print(os.environ["FAKE_REPOSITORY_ROOT"])
elif args[:2] == ["status", "--porcelain"]:
    if scenario == "dirty":
        print("?? untracked-phase3-file")
elif args[:2] == ["rev-parse", "HEAD"]:
    print(head)
elif args[:1] == ["fetch"]:
    pass
elif args[:2] == ["merge-base", "--is-ancestor"]:
    if scenario == "unpushed":
        sys.exit(1)
elif args[:3] == ["remote", "get-url", "origin"]:
    print("https://github.com/Ludwig-H/E-HGP")
else:
    print("unsupported fake git command: " + " ".join(args), file=sys.stderr)
    sys.exit(93)
"""

FAKE_START = r"""#!/usr/bin/env bash
set -euo pipefail
printf 'start project=%s zone=%s instance=%s args=%s\n' \
  "${GCP_PROJECT_ID:-}" "${GCP_ZONE:-}" "${GCP_INSTANCE_NAME:-}" "$*" \
  >> "${FAKE_SESSION_LOG}"
if [[ "${FAKE_SESSION_SCENARIO:-}" == "start-failure" ]]; then
  exit 37
fi
"""

FAKE_STOP = r"""#!/usr/bin/env bash
set -euo pipefail
printf 'stop project=%s zone=%s instance=%s args=%s\n' \
  "${GCP_PROJECT_ID:-}" "${GCP_ZONE:-}" "${GCP_INSTANCE_NAME:-}" "$*" \
  >> "${FAKE_SESSION_LOG}"
if [[ "${FAKE_SESSION_SCENARIO:-}" == "stop-failure" ]]; then
  exit 38
fi
"""


class ScriptSafetyTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tempdir.cleanup)
        self.tmp = Path(self.tempdir.name)
        fake = self.tmp / "gcloud"
        fake.write_text(FAKE_GCLOUD, encoding="utf-8")
        fake.chmod(0o755)
        self.log = self.tmp / "gcloud.log"
        self.env = os.environ.copy()
        self.env.update(
            {
                "PATH": f"{self.tmp}:{self.env['PATH']}",
                "FAKE_GCLOUD_LOG": str(self.log),
                "GCP_PROJECT_ID": "devpod-gpu-exploration",
                "GCP_ZONE": "europe-west4-a",
                "GCP_INSTANCE_NAME": "ehgp-blackwell-spot",
            }
        )

    def run_script(
        self, name: str, *arguments: str
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["bash", str(ROOT / "gcp-migration" / name), *arguments],
            cwd=ROOT,
            env=self.env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )

    def commands(self) -> str:
        return self.log.read_text(encoding="utf-8") if self.log.exists() else ""

    def test_deploy_rejects_runtime_below_gce_minimum(self) -> None:
        self.env["GCP_MAX_RUN_DURATION"] = "29s"
        result = self.run_script("deploy.sh")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("minimum GCE de 30 secondes", result.stdout)
        self.assertEqual(self.commands(), "")

    def test_start_refuses_wrong_machine_before_mutation(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "wrong-machine"
        result = self.run_script("start_and_verify.sh", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("g4-standard-48", result.stdout)
        self.assertNotIn("compute instances start", self.commands())

    def test_start_refuses_non_spot_instance_before_mutation(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "wrong-provisioning"
        result = self.run_script("start_and_verify.sh", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Spot", result.stdout)
        self.assertNotIn("compute instances start", self.commands())

    def test_start_fails_closed_when_guest_guard_readback_is_missing(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "guest-readback"
        result = self.run_script("start_and_verify.sh", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("relu de manière certaine", result.stdout)
        self.assertIn("compute instances start", self.commands())
        self.assertIn("compute instances stop", self.commands())

    def test_stop_refuses_unlabelled_target_before_mutation(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "unlabelled"
        result = self.run_script("stop_and_verify.sh", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ne porte pas le label project=e-hgp", result.stdout)
        self.assertNotIn("compute instances stop", self.commands())

    def test_stop_does_not_claim_or_mutate_concurrent_vm(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "other-active"
        result = self.run_script("stop_and_verify.sh", "--yes")
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("signalement seulement", result.stdout)
        self.assertNotIn("compute instances stop", self.commands())

    def test_guest_preflight_caps_guard_at_eight_hours(self) -> None:
        result = self.run_script(
            "blackwell_preflight.sh", "--arm-shutdown", "481", "--skip-docker"
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("entre 1 et 480 minutes", result.stdout)


class Phase3QualificationOrchestratorTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tempdir.cleanup)
        self.tmp = Path(self.tempdir.name)
        self.repository = self.tmp / "repository"
        migration = self.repository / "gcp-migration"
        migration.mkdir(parents=True)

        source = ROOT / "gcp-migration" / "run_phase3_qualification.sh"
        orchestrator = migration / source.name
        shutil.copy2(source, orchestrator)
        orchestrator.chmod(0o755)
        self.orchestrator = orchestrator

        start = migration / "start_and_verify.sh"
        start.write_text(FAKE_START, encoding="utf-8")
        start.chmod(0o755)
        stop = migration / "stop_and_verify.sh"
        stop.write_text(FAKE_STOP, encoding="utf-8")
        stop.chmod(0o755)

        fake_bin = self.tmp / "bin"
        fake_bin.mkdir()
        fake_gcloud = fake_bin / "gcloud"
        fake_gcloud.write_text(FAKE_GCLOUD, encoding="utf-8")
        fake_gcloud.chmod(0o755)
        fake_git = fake_bin / "git"
        fake_git.write_text(FAKE_GIT, encoding="utf-8")
        fake_git.chmod(0o755)

        self.gcloud_log = self.tmp / "qualification-gcloud.log"
        self.git_log = self.tmp / "qualification-git.log"
        self.session_log = self.tmp / "qualification-session.log"
        self.results = self.tmp / "results"
        self.head = "a" * 40
        self.env = os.environ.copy()
        self.env.update(
            {
                "PATH": f"{fake_bin}:{self.env['PATH']}",
                "FAKE_GCLOUD_LOG": str(self.gcloud_log),
                "FAKE_GCLOUD_SCENARIO": "qualification-success",
                "FAKE_GIT_LOG": str(self.git_log),
                "FAKE_GIT_HEAD": self.head,
                "FAKE_GIT_SCENARIO": "clean",
                "FAKE_REPOSITORY_ROOT": str(self.repository),
                "FAKE_SESSION_LOG": str(self.session_log),
                "GCP_PROJECT_ID": "devpod-gpu-exploration",
                "GCP_ZONE": "europe-west4-a",
                "GCP_INSTANCE_NAME": "ehgp-blackwell-spot",
                "GCP_GUEST_SHUTDOWN_MINUTES": "45",
            }
        )

    def run_qualification(self, *arguments: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["bash", str(self.orchestrator), *arguments],
            cwd=self.repository,
            env=self.env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
            timeout=20,
        )

    def gcloud_commands(self) -> str:
        if not self.gcloud_log.exists():
            return ""
        return self.gcloud_log.read_text(encoding="utf-8")

    def session_commands(self) -> str:
        if not self.session_log.exists():
            return ""
        return self.session_log.read_text(encoding="utf-8")

    def standard_arguments(self) -> tuple[str, ...]:
        return ("--yes", "--result-dir", str(self.results))

    def test_success_retrieves_artifact_and_independently_certifies_terminated(
        self,
    ) -> None:
        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("[TERMINATED]", result.stdout)
        self.assertIn("[SUCCÈS] Qualification Phase 3", result.stdout)
        artifact = self.results / f"phase3-{self.head}.json"
        self.assertEqual(
            artifact.read_text(encoding="utf-8"),
            '{"kind":"phase3-result","status":"passed"}\n',
        )

        session = self.session_commands()
        self.assertIn(
            "start project=devpod-gpu-exploration zone=europe-west4-a "
            "instance=ehgp-blackwell-spot args=--yes --guest-shutdown-minutes 45",
            session,
        )
        self.assertIn(
            "stop project=devpod-gpu-exploration zone=europe-west4-a "
            "instance=ehgp-blackwell-spot args=--yes",
            session,
        )

        commands = self.gcloud_commands()
        self.assertIn("compute ssh ehgp-blackwell-spot", commands)
        self.assertIn("compute scp ehgp-blackwell-spot:", commands)
        self.assertIn("compute instances describe ehgp-blackwell-spot", commands)
        self.assertIn("rm -rf -- /tmp/morsehgp3d-phase3.A1b2C3d4", commands)
        self.assertIn(f"checkout --quiet --detach {self.head}", commands)
        self.assertIn("phase3_remote_qualification.sh --yes --output", commands)
        self.assertNotIn("compute instances start", commands)
        self.assertNotIn("compute instances stop", commands)
        self.assertNotIn("ehgp-concurrent", commands)

    def test_remote_failure_still_stops_and_returns_original_failure(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "qualification-remote-failure"
        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 42, result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertIn("[TERMINATED]", result.stdout)
        self.assertFalse((self.results / f"phase3-{self.head}.json").exists())

    def test_dirty_worktree_is_rejected_before_start_or_gcloud(self) -> None:
        self.env["FAKE_GIT_SCENARIO"] = "dirty"
        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Worktree sale", result.stdout)
        self.assertEqual(self.session_commands(), "")
        self.assertEqual(self.gcloud_commands(), "")

    def test_head_not_on_origin_main_is_rejected_before_start(self) -> None:
        self.env["FAKE_GIT_SCENARIO"] = "unpushed"
        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("n'est pas présent sur origin/main", result.stdout)
        self.assertEqual(self.session_commands(), "")
        self.assertEqual(self.gcloud_commands(), "")

    def test_sigterm_during_remote_run_still_stops_target(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "qualification-interrupt"
        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 143, result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertIn("[TERMINATED]", result.stdout)

    def test_remote_head_mismatch_fails_closed_and_stops_target(self) -> None:
        self.env["FAKE_REMOTE_HEAD"] = "b" * 40
        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("HEAD distant", result.stdout)
        self.assertIn("différent du SHA local", result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertIn("[TERMINATED]", result.stdout)

    def test_unreadable_independent_stop_proof_has_distinct_code(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "qualification-stop-unreadable"
        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 91, result.stdout)
        self.assertIn("[ARRÊT ILLISIBLE]", result.stdout)
        self.assertIn("Projet=devpod-gpu-exploration", result.stdout)
        self.assertIn("zone=europe-west4-a", result.stdout)
        self.assertIn("instance=ehgp-blackwell-spot", result.stdout)
        self.assertIn("Commande de contrôle", result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())

    def test_stop_script_failure_has_distinct_code_and_precise_target(self) -> None:
        self.env["FAKE_SESSION_SCENARIO"] = "stop-failure"
        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 90, result.stdout)
        self.assertIn("[ARRÊT NON CERTIFIÉ]", result.stdout)
        self.assertIn("Projet=devpod-gpu-exploration", result.stdout)
        self.assertIn("zone=europe-west4-a", result.stdout)
        self.assertIn("instance=ehgp-blackwell-spot", result.stdout)
        self.assertIn("stop_and_verify.sh a échoué", result.stdout)

    def test_yes_is_mandatory_before_any_session_action(self) -> None:
        result = self.run_qualification("--result-dir", str(self.results))

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("--yes est obligatoire", result.stdout)
        self.assertEqual(self.session_commands(), "")
        self.assertEqual(self.gcloud_commands(), "")

    def test_different_target_is_rejected_without_mutation(self) -> None:
        self.env["GCP_INSTANCE_NAME"] = "ehgp-concurrent"
        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Instance refusée", result.stdout)
        self.assertEqual(self.session_commands(), "")
        self.assertEqual(self.gcloud_commands(), "")


class WorkflowPolicyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        module_path = ROOT / "tools" / "check_gcp_workflows.py"
        spec = importlib.util.spec_from_file_location(
            "check_gcp_workflows", module_path
        )
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot import workflow checker")
        cls.checker = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.checker)

    def test_folded_yaml_run_block_is_rejected(self) -> None:
        text = """name: unsafe
jobs:
  test:
    steps:
      - run: >-
          gcloud compute instances
          start billable-vm
"""
        errors = self.checker.validate_workflow(Path("unsafe.yml"), text)
        self.assertTrue(any("folded YAML" in error for error in errors))

    def test_folded_yaml_anchor_is_rejected(self) -> None:
        text = """name: unsafe
commands: &hidden >-
  gcloud compute instances start billable-vm
jobs:
  test:
    steps:
      - run: *hidden
"""
        errors = self.checker.validate_workflow(Path("unsafe.yml"), text)
        self.assertTrue(any("folded YAML" in error for error in errors))

    def test_backslash_split_mutation_is_rejected(self) -> None:
        text = """name: unsafe
jobs:
  test:
    steps:
      - run: |
          gcloud compute instances \\
            start billable-vm
"""
        errors = self.checker.validate_workflow(Path("unsafe.yml"), text)
        self.assertTrue(any("outside read-only allowlist" in error for error in errors))

    def test_repository_workflow_policy_passes(self) -> None:
        result = subprocess.run(
            ["python3", str(ROOT / "tools" / "check_gcp_workflows.py")],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout)


if __name__ == "__main__":
    unittest.main()
