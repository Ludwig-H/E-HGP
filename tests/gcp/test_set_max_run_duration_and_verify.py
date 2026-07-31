#!/usr/bin/env python3

import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "gcp-migration" / "set_max_run_duration_and_verify.sh"

FAKE_GCLOUD = r'''#!/usr/bin/env python3
import os
from pathlib import Path
import sys

args = sys.argv[1:]
log_path = Path(os.environ["FAKE_GCLOUD_LOG"])
with log_path.open("a", encoding="utf-8") as stream:
    stream.write(" ".join(args) + "\n")

scenario = os.environ.get("FAKE_GCLOUD_SCENARIO", "success")
state_path = Path(os.environ["FAKE_GCLOUD_STATE"])
post_mutation = state_path.exists()

if args[:3] == ["config", "get-value", "project"]:
    print(os.environ.get("FAKE_CONFIG_PROJECT", "devpod-gpu-exploration"))
elif args[:3] == ["config", "get-value", "account"]:
    print("tester@example.invalid")
elif args[:3] == ["compute", "instances", "list"]:
    if scenario != "missing-target":
        print(os.environ["GCP_INSTANCE_NAME"])
elif args[:3] == ["compute", "instances", "describe"]:
    output_format = next(
        item.split("=", 1)[1]
        for item in args
        if item.startswith("--format=")
    )
    field = output_format.removeprefix("value(").removesuffix(")")
    duration = (
        state_path.read_text(encoding="utf-8").strip()
        if post_mutation
        else os.environ.get("FAKE_INITIAL_MAX_RUN_SECONDS", "3600")
    )
    values = {
        "status": "TERMINATED",
        "scheduling.instanceTerminationAction": "STOP",
        "scheduling.automaticRestart": "False",
        "scheduling.maxRunDuration.seconds": duration,
        "labels.project": "e-hgp",
        "machineType.basename()": "g4-standard-48",
        "scheduling.onHostMaintenance": "TERMINATE",
        "scheduling.provisioningModel": "SPOT",
        "lastStartTimestamp": "2026-07-31T00:00:00.000Z",
    }
    pre_overrides = {
        "pre-running": ("status", "RUNNING"),
        "pre-action-delete": ("scheduling.instanceTerminationAction", "DELETE"),
        "pre-restart": ("scheduling.automaticRestart", "True"),
        "pre-unbounded": ("scheduling.maxRunDuration.seconds", "28801"),
        "pre-unlabelled": ("labels.project", ""),
        "pre-wrong-machine": ("machineType.basename()", "n2-standard-48"),
        "pre-migrate": ("scheduling.onHostMaintenance", "MIGRATE"),
        "pre-standard": ("scheduling.provisioningModel", "STANDARD"),
    }
    post_overrides = {
        "post-running": ("status", "RUNNING"),
        "post-action-delete": ("scheduling.instanceTerminationAction", "DELETE"),
        "post-restart": ("scheduling.automaticRestart", "True"),
        "post-unlabelled": ("labels.project", ""),
        "post-wrong-machine": ("machineType.basename()", "n2-standard-48"),
        "post-migrate": ("scheduling.onHostMaintenance", "MIGRATE"),
        "post-standard": ("scheduling.provisioningModel", "STANDARD"),
        "post-generation-race": (
            "lastStartTimestamp",
            "2026-07-31T00:01:00.000Z",
        ),
    }
    if scenario in pre_overrides and not post_mutation:
        key, value = pre_overrides[scenario]
        values[key] = value
    if scenario in post_overrides and post_mutation:
        key, value = post_overrides[scenario]
        values[key] = value
    print(values.get(field, ""))
elif args[:3] == ["compute", "instances", "set-scheduling"]:
    if scenario == "mutation-failure":
        print("simulated set-scheduling failure", file=sys.stderr)
        sys.exit(42)
    duration_flag = next(
        item for item in args if item.startswith("--max-run-duration=")
    )
    action_flag = next(
        item for item in args if item.startswith("--instance-termination-action=")
    )
    if action_flag != "--instance-termination-action=STOP":
        raise SystemExit("unsafe termination action")
    duration = duration_flag.split("=", 1)[1]
    if not duration.endswith("s") or not duration[:-1].isdigit():
        raise SystemExit("invalid duration")
    state_path.write_text(duration[:-1], encoding="utf-8")
else:
    raise SystemExit("unsupported fake gcloud call: " + " ".join(args))
'''

FAKE_TIMEOUT = r'''#!/usr/bin/env bash
set -euo pipefail
if [[ "${1:-}" == "--version" ]]; then
  printf 'timeout (GNU coreutils) 9.1\n'
  exit 0
fi
printf '%s\n' "$*" >>"${FAKE_TIMEOUT_LOG}"
[[ "${1:-}" == --kill-after=* ]] || exit 97
shift
[[ "${1:-}" =~ ^[0-9]+s$ ]] || exit 98
shift
exec "$@"
'''


class SetMaxRunDurationAndVerifyTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tempdir.cleanup)
        self.tmp = Path(self.tempdir.name)
        fake_gcloud = self.tmp / "gcloud"
        fake_gcloud.write_text(FAKE_GCLOUD, encoding="utf-8")
        fake_gcloud.chmod(0o755)
        fake_timeout = self.tmp / "timeout"
        fake_timeout.write_text(FAKE_TIMEOUT, encoding="utf-8")
        fake_timeout.chmod(0o755)
        self.gcloud_log = self.tmp / "gcloud.log"
        self.timeout_log = self.tmp / "timeout.log"
        self.state = self.tmp / "duration.state"
        self.env = os.environ.copy()
        self.env.update(
            {
                "PATH": f"{self.tmp}:{self.env['PATH']}",
                "FAKE_GCLOUD_LOG": str(self.gcloud_log),
                "FAKE_GCLOUD_STATE": str(self.state),
                "FAKE_TIMEOUT_LOG": str(self.timeout_log),
                "GCP_PROJECT_ID": "devpod-gpu-exploration",
                "GCP_ZONE": "europe-west4-a",
                "GCP_INSTANCE_NAME": "ehgp-blackwell-spot",
            }
        )

    def reset_fake_state(self) -> None:
        self.gcloud_log.unlink(missing_ok=True)
        self.timeout_log.unlink(missing_ok=True)
        self.state.unlink(missing_ok=True)

    def run_script(self, *arguments: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["bash", str(SCRIPT), *arguments],
            cwd=ROOT,
            env=self.env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
            timeout=10,
        )

    def commands(self) -> list[str]:
        if not self.gcloud_log.exists():
            return []
        return self.gcloud_log.read_text(encoding="utf-8").splitlines()

    def mutations(self) -> list[str]:
        return [
            command
            for command in self.commands()
            if command.startswith("compute instances set-scheduling ")
        ]

    def assert_no_start_stop_or_lifecycle_mutation(self) -> None:
        commands = "\n".join(self.commands())
        self.assertIsNone(
            re.search(
                r"^compute instances (?:start|stop|create|delete|reset|update)\b",
                commands,
                flags=re.M,
            )
        )

    def test_help_and_required_explicit_arguments_do_not_call_gcloud(self) -> None:
        result = self.run_script("--help")
        self.assertEqual(0, result.returncode, result.stdout)
        self.assertIn("30", result.stdout)
        self.assertIn("28800", result.stdout)
        self.assertEqual([], self.commands())

        for arguments in (
            ("--max-run-duration-seconds", "14400"),
            ("--yes",),
            ("--yes", "--max-run-duration-seconds", "29"),
            ("--yes", "--max-run-duration-seconds", "28801"),
            ("--yes", "--max-run-duration-seconds", "014400"),
        ):
            with self.subTest(arguments=arguments):
                self.reset_fake_state()
                result = self.run_script(*arguments)
                self.assertNotEqual(0, result.returncode)
                self.assertEqual([], self.commands())

    def test_project_and_target_allowlist_are_checked_before_gcloud(self) -> None:
        cases = (
            ("other-project", "europe-west4-a", "ehgp-blackwell-spot"),
            (
                "devpod-gpu-exploration",
                "europe-west4-b",
                "ehgp-blackwell-spot",
            ),
            (
                "devpod-gpu-exploration",
                "europe-west4-ai1a",
                "ehgp-blackwell-spot",
            ),
        )
        for project, zone, instance in cases:
            with self.subTest(project=project, zone=zone, instance=instance):
                self.reset_fake_state()
                self.env["GCP_PROJECT_ID"] = project
                self.env["GCP_ZONE"] = zone
                self.env["GCP_INSTANCE_NAME"] = instance
                result = self.run_script(
                    "--yes", "--max-run-duration-seconds", "14400"
                )
                self.assertNotEqual(0, result.returncode)
                self.assertEqual([], self.commands())

    def test_primary_target_is_reconfigured_and_recertified(self) -> None:
        result = self.run_script(
            "--yes", "--max-run-duration-seconds", "14400"
        )

        self.assertEqual(0, result.returncode, result.stdout)
        self.assertEqual("14400", self.state.read_text(encoding="utf-8"))
        self.assertEqual(1, len(self.mutations()))
        mutation = self.mutations()[0]
        self.assertIn("ehgp-blackwell-spot", mutation)
        self.assertIn("--project=devpod-gpu-exploration", mutation)
        self.assertIn("--zone=europe-west4-a", mutation)
        self.assertIn("--max-run-duration=14400s", mutation)
        self.assertIn("--instance-termination-action=STOP", mutation)
        self.assertIn("[OK] maxRunDuration=14400s recertifié", result.stdout)
        self.assert_no_start_stop_or_lifecycle_mutation()

        timeout_calls = self.timeout_log.read_text(encoding="utf-8").splitlines()
        gcloud_timeout_calls = [line for line in timeout_calls if " gcloud " in line]
        self.assertEqual(len(self.commands()), len(gcloud_timeout_calls))
        self.assertTrue(
            all(line.startswith("--kill-after=") for line in gcloud_timeout_calls)
        )

    def test_ai_capacity_target_is_the_only_other_allowed_pair(self) -> None:
        self.env["GCP_ZONE"] = "europe-west4-ai1a"
        self.env["GCP_INSTANCE_NAME"] = "ehgp-blackwell-spot-ai1a"

        result = self.run_script(
            "--yes", "--max-run-duration-seconds", "28800"
        )

        self.assertEqual(0, result.returncode, result.stdout)
        self.assertEqual(1, len(self.mutations()))
        self.assertIn("--zone=europe-west4-ai1a", self.mutations()[0])
        self.assertIn("--max-run-duration=28800s", self.mutations()[0])
        self.assert_no_start_stop_or_lifecycle_mutation()

    def test_every_precondition_failure_precedes_set_scheduling(self) -> None:
        scenarios = (
            "missing-target",
            "pre-running",
            "pre-action-delete",
            "pre-restart",
            "pre-unbounded",
            "pre-unlabelled",
            "pre-wrong-machine",
            "pre-migrate",
            "pre-standard",
        )
        for scenario in scenarios:
            with self.subTest(scenario=scenario):
                self.reset_fake_state()
                self.env["FAKE_GCLOUD_SCENARIO"] = scenario
                result = self.run_script(
                    "--yes", "--max-run-duration-seconds", "14400"
                )
                self.assertNotEqual(0, result.returncode)
                self.assertEqual([], self.mutations())
                self.assert_no_start_stop_or_lifecycle_mutation()

    def test_post_mutation_drift_fails_closed_without_start_or_stop(self) -> None:
        scenarios = (
            "post-running",
            "post-action-delete",
            "post-restart",
            "post-unlabelled",
            "post-wrong-machine",
            "post-migrate",
            "post-standard",
            "post-generation-race",
        )
        for scenario in scenarios:
            with self.subTest(scenario=scenario):
                self.reset_fake_state()
                self.env["FAKE_GCLOUD_SCENARIO"] = scenario
                result = self.run_script(
                    "--yes", "--max-run-duration-seconds", "14400"
                )
                self.assertNotEqual(0, result.returncode)
                self.assertEqual(1, len(self.mutations()))
                self.assertIn("[RECONFIGURATION NON CERTIFIÉE]", result.stdout)
                self.assertIn("Commande de contrôle", result.stdout)
                self.assert_no_start_stop_or_lifecycle_mutation()

    def test_failed_mutation_is_reported_but_never_compensated_with_start_stop(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "mutation-failure"

        result = self.run_script(
            "--yes", "--max-run-duration-seconds", "14400"
        )

        self.assertNotEqual(0, result.returncode)
        self.assertEqual(1, len(self.mutations()))
        self.assertIn("[RECONFIGURATION NON CERTIFIÉE]", result.stdout)
        self.assert_no_start_stop_or_lifecycle_mutation()

    def test_source_has_one_bounded_mutation_and_valid_bash_syntax(self) -> None:
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertEqual(
            1,
            source.count("gcloud_mutation compute instances set-scheduling"),
        )
        self.assertIn('timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s"', source)
        for operation in ("start", "stop", "create", "delete", "reset", "update"):
            self.assertNotIn(
                f"gcloud_mutation compute instances {operation}", source
            )
        completed = subprocess.run(
            ["bash", "-n", str(SCRIPT)],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        self.assertEqual(0, completed.returncode, completed.stdout)


if __name__ == "__main__":
    unittest.main()
