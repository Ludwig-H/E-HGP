"""Regression tests for fail-closed GCP session guards."""

from __future__ import annotations

import importlib.util
import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
FAKE_GCLOUD = r"""#!/usr/bin/env python3
import os
import sys
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
elif args[:2] == ["compute", "ssh"] and scenario == "guest-readback":
    print("No scheduled shutdown")
else:
    print("unsupported fake gcloud command: " + " ".join(args), file=sys.stderr)
    sys.exit(96)
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

    def run_script(self, name: str, *arguments: str) -> subprocess.CompletedProcess[str]:
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


class WorkflowPolicyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        module_path = ROOT / "tools" / "check_gcp_workflows.py"
        spec = importlib.util.spec_from_file_location("check_gcp_workflows", module_path)
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
