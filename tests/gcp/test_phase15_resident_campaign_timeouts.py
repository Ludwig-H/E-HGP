#!/usr/bin/env python3

import ast
import hashlib
from pathlib import Path
import re
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
REMOTE_PATH = ROOT / "gcp-migration" / "phase3_remote_qualification.sh"
WRAPPER_PATH = ROOT / "gcp-migration" / "run_phase3_qualification.sh"


def shell_integer(source: str, name: str) -> int:
    match = re.search(rf"^readonly {re.escape(name)}=([0-9]+)$", source, re.M)
    if match is None:
        raise AssertionError(f"missing explicit integer contract: {name}")
    return int(match.group(1))


def shell_duration_minutes(source: str, name: str) -> int:
    match = re.search(
        rf'^readonly {re.escape(name)}="([0-9]+)m"$', source, re.M
    )
    if match is None:
        raise AssertionError(f"missing explicit minute contract: {name}")
    return int(match.group(1))


class Phase15ResidentCampaignTimeoutTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.remote = REMOTE_PATH.read_text(encoding="utf-8")
        cls.wrapper = WRAPPER_PATH.read_text(encoding="utf-8")

    def test_massive_timeout_contract_is_identical_and_below_eight_hours(self) -> None:
        names = {
            "PHASE15_RESIDENT_SHORT_TIMEOUT_SECONDS": 60,
            "PHASE15_RESIDENT_REDUCER_TIMEOUT_SECONDS": 300,
            "PHASE15_RESIDENT_FRONTIER_50K_TIMEOUT_SECONDS": 600,
            "PHASE15_RESIDENT_DIRECT_10M_TIMEOUT_SECONDS": 2400,
            "PHASE15_RESIDENT_DIRECT_30M_TIMEOUT_SECONDS": 5400,
        }
        for name, expected in names.items():
            self.assertEqual(expected, shell_integer(self.remote, name))
            self.assertEqual(expected, shell_integer(self.wrapper, name))

        fixed_total = (
            2 * names["PHASE15_RESIDENT_SHORT_TIMEOUT_SECONDS"]
            + 2 * names["PHASE15_RESIDENT_REDUCER_TIMEOUT_SECONDS"]
            + sum(
                names[name]
                for name in (
                    "PHASE15_RESIDENT_FRONTIER_50K_TIMEOUT_SECONDS",
                    "PHASE15_RESIDENT_DIRECT_10M_TIMEOUT_SECONDS",
                    "PHASE15_RESIDENT_DIRECT_30M_TIMEOUT_SECONDS",
                )
            )
        )
        self.assertEqual(9120, fixed_total)
        self.assertLess(
            fixed_total,
            shell_integer(self.wrapper, "MAX_GUARDED_SESSION_SECONDS"),
        )
        self.assertLess(
            fixed_total,
            shell_integer(self.remote, "MAX_GUARDED_SESSION_SECONDS"),
        )

    def test_wrapper_session_and_ssh_timeout_cover_the_ordered_campaign(self) -> None:
        fixed_total = 9120
        max_run = shell_integer(
            self.wrapper, "PHASE15_RESIDENT_EXPECTED_MAX_RUN_SECONDS"
        )
        guest_minutes = shell_integer(
            self.wrapper, "PHASE15_RESIDENT_GUEST_SHUTDOWN_MINUTES"
        )
        remote_timeout = shell_integer(
            self.wrapper, "PHASE15_RESIDENT_GCLOUD_REMOTE_TIMEOUT_SECONDS"
        )
        work_reserve = shell_integer(self.wrapper, "WORK_RESERVE_SECONDS")
        tolerance = shell_integer(self.wrapper, "TIMESTAMP_TOLERANCE_SECONDS")
        operational_reserve = shell_integer(
            self.wrapper, "PHASE15_RESIDENT_OPERATIONAL_RESERVE_SECONDS"
        )
        absolute_max = shell_integer(self.wrapper, "MAX_GUARDED_SESSION_SECONDS")
        ssh_ttl_seconds = 60 * shell_duration_minutes(
            self.wrapper, "PHASE15_RESIDENT_SSH_KEY_TTL"
        )
        ssh_slack = shell_integer(self.wrapper, "SSH_KEY_TTL_SLACK_SECONDS")

        self.assertEqual(14400, max_run)
        self.assertLessEqual(max_run, absolute_max)
        self.assertGreaterEqual(
            max_run,
            fixed_total + operational_reserve + work_reserve + tolerance,
        )
        self.assertGreaterEqual(
            max_run - tolerance - work_reserve - fixed_total,
            operational_reserve,
        )
        self.assertGreaterEqual(guest_minutes * 60, fixed_total + operational_reserve)
        self.assertLessEqual(guest_minutes * 60 + tolerance, max_run)
        self.assertGreaterEqual(remote_timeout, fixed_total + operational_reserve)
        self.assertLessEqual(remote_timeout, max_run)
        self.assertGreaterEqual(ssh_ttl_seconds, max_run)
        self.assertLessEqual(ssh_ttl_seconds, max_run + ssh_slack)

        guard_min = shell_integer(
            self.remote, "PHASE15_RESIDENT_GUEST_GUARD_MIN_REMAINING_SECONDS"
        )
        guard_max = shell_integer(
            self.remote, "PHASE15_RESIDENT_GUEST_GUARD_MAX_REMAINING_SECONDS"
        )
        self.assertLessEqual(guard_min, guest_minutes * 60)
        self.assertLessEqual(guest_minutes * 60, guard_max)

    def test_runner_assembler_and_validator_use_the_explicit_contract(self) -> None:
        for name in (
            "PHASE15_RESIDENT_FRONTIER_50K_TIMEOUT_SECONDS",
            "PHASE15_RESIDENT_DIRECT_10M_TIMEOUT_SECONDS",
            "PHASE15_RESIDENT_DIRECT_30M_TIMEOUT_SECONDS",
        ):
            self.assertGreaterEqual(self.remote.count(f'"${{{name}}}"'), 2)
            self.assertGreaterEqual(self.wrapper.count(f'"${{{name}}}"'), 1)

        self.assertRegex(
            self.remote,
            r'phase15-resident-transactional-semantic-frontier-50k"\s+\\\n'
            r'\s+"\$\{PHASE15_RESIDENT_FRONTIER_50K_TIMEOUT_SECONDS\}"'
            r'[\s\S]*?\n\s+strict\s+\\',
        )
        for count in ("10m", "30m"):
            self.assertRegex(
                self.remote,
                rf'phase15-resident-transactional-semantic-direct-{count}"'
                rf'[\s\S]*?canonical-direct-scale-timeout\s+\\',
            )
        self.assertIn(
            ") != (600, 2400, 5400):\n    fail(\"resident campaign timeout contract drift\")",
            self.remote,
        )
        self.assertIn(
            ") != (600, 2400, 5400):\n    raise SystemExit(\"resident artifact timeout contract drift\")",
            self.wrapper,
        )
        self.assertIn(
            '"direct_scale_10m_rank11": direct_10m_timeout_seconds',
            self.remote,
        )
        self.assertIn(
            '"direct_scale_30m_rank11": direct_30m_timeout_seconds',
            self.wrapper,
        )
        self.assertIn(
            "exit_status != 0 and not (point_count > 50000 and exit_status == 124)",
            self.wrapper,
        )

    def test_help_reports_the_new_bounds_without_entering_gcp(self) -> None:
        for path in (REMOTE_PATH, WRAPPER_PATH):
            completed = subprocess.run(
                ["bash", str(path), "--help"],
                cwd=ROOT,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=False,
                timeout=5,
            )
            self.assertEqual(0, completed.returncode, completed.stdout)
            for bound in ("600 s", "2400 s", "5400 s"):
                self.assertIn(bound, completed.stdout)

    def test_direct_scale_timeout_receipts_require_the_new_external_walls(self) -> None:
        blocks = re.findall(
            r"<<'PY'\n(.*?)\nPY(?:\n|$)", self.wrapper, flags=re.S
        )
        code = next(block for block in blocks if "def validate_direct_scale_timeout" in block)
        tree = ast.parse(code)
        functions = [node for node in tree.body if isinstance(node, ast.FunctionDef)]
        namespace = {"git_sha": "a" * 40, "hashlib": hashlib, "re": re}
        exec(
            compile(ast.Module(body=functions, type_ignores=[]), "<validator>", "exec"),
            namespace,
        )
        validate = namespace["validate_direct_scale_timeout"]
        sha = "b" * 64

        for point_count, timeout_seconds in ((10_000_000, 2400), (30_000_000, 5400)):
            started = 1_000_000_000_000_000_000
            finished = started + timeout_seconds * 1_000_000_000
            command = ["/usr/bin/timeout", f"{timeout_seconds}s", str(point_count)]
            receipt = {
                "backend": "cuda_g4",
                "binary_sha256": sha,
                "claims": {
                    "exactness": False,
                    "product": False,
                    "qualification": False,
                    "scalability": False,
                    "scientific_result": False,
                },
                "command": command,
                "component_only": True,
                "deployment_status": "profile_only",
                "exit_status": 124,
                "external_wall_finished_epoch_nanoseconds": finished,
                "external_wall_nanoseconds": finished - started,
                "external_wall_started_epoch_nanoseconds": started,
                "family": "affine_uniform_binary64",
                "git_sha": "a" * 40,
                "internal_timings_available": False,
                "maximum_closed_rank": 11,
                "mode": "device_resident_budgeted_morton_yao48_anchor_tiles_direct_scale",
                "point_count": point_count,
                "profile": "hgp_reduced",
                "profile_only": True,
                "public_status": "not_claimed",
                "qualified": False,
                "run_kind": "direct_scale_timeout",
                "scale_eligible": False,
                "schema": "morsehgp3d.phase15.direct_scale_timeout.v1",
                "status": "timeout",
                "stderr_empty": True,
                "stderr_sha256": hashlib.sha256(b"").hexdigest(),
                "stdout_sha256": hashlib.sha256(b"").hexdigest(),
                "timed_out": True,
                "timeout_seconds": timeout_seconds,
                "timeout_source": (
                    "exit_status_124_with_external_wall_at_least_fixed_bound"
                ),
            }
            validate(
                receipt,
                point_count=point_count,
                timeout_seconds=timeout_seconds,
                command=command,
                started=started,
                finished=finished,
                stdout="",
                stderr="",
                binary_sha256=sha,
            )

            early = dict(receipt)
            early_finished = started + (timeout_seconds - 1) * 1_000_000_000
            early["external_wall_finished_epoch_nanoseconds"] = early_finished
            early["external_wall_nanoseconds"] = early_finished - started
            with self.assertRaises(SystemExit):
                validate(
                    early,
                    point_count=point_count,
                    timeout_seconds=timeout_seconds,
                    command=command,
                    started=started,
                    finished=early_finished,
                    stdout="",
                    stderr="",
                    binary_sha256=sha,
                )


if __name__ == "__main__":
    unittest.main()
