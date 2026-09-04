#!/usr/bin/env python3
"""Creation lifecycle mocks only: no cloud operation, installation or shutdown."""
from __future__ import annotations

from contextlib import redirect_stdout
from datetime import datetime, timezone
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("creator_fixture", HERE / "create_v7_g4.py")
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("creator unavailable")
M = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M)
ZONE = "us-central1-b"


def date(epoch: float) -> str:
    return datetime.fromtimestamp(epoch, timezone.utc).isoformat().replace("+00:00", "Z")


class CreationTests(unittest.TestCase):
    def lifecycle(self, scenario: str) -> tuple[int, dict, list]:
        with tempfile.TemporaryDirectory(prefix="ehgp-create-test-") as directory:
            base = Path(directory)
            receipt = base / "receipt"
            calls = []
            vm = None
            operation = None
            writes_fail = False
            observed_without_generation = False
            real_write = M.H.write_json
            clock = 0.0

            def monotonic():
                nonlocal clock
                clock += 10
                return clock

            def write(path, data):
                nonlocal vm
                if writes_fail:
                    raise OSError("injected receipt EIO")
                if scenario == "changed_id" and data.get("status") == "double_guard_verified_no_workload":
                    vm["id"] = "999"
                if scenario == "changed_generation" and data.get("status") == "double_guard_verified_no_workload":
                    vm["lastStartTimestamp"] = date(time.time() + 1)
                real_write(path, data)

            def instance(name, nonce):
                now = time.time()
                return {"id": "123", "name": name, "zone": "zones/" + ZONE,
                        "selfLink": "https://www.googleapis.com/compute/v1/projects/" + M.PROJECT +
                        "/zones/" + ZONE + "/instances/" + name,
                        "creationTimestamp": date(now), "lastStartTimestamp": date(now), "status": "RUNNING",
                        "labels": {"project": "e-hgp", "ehgp-create": "foreign" if scenario == "foreign" else nonce},
                        "machineType": "machines/g4-standard-48",
                        "scheduling": {"provisioningModel": "SPOT", "instanceTerminationAction": "STOP",
                                       "automaticRestart": False, "onHostMaintenance": "TERMINATE",
                                       "maxRunDuration": {"seconds": "3600"}},
                        "resourceStatus": {"scheduling": {"terminationTimestamp": date(now + 3600)}}}

            def logged(folder, label, argv, timeout, **kwargs):
                nonlocal vm, writes_fail, operation
                calls.append((label, argv))
                self.assertGreater(timeout, 0)
                output = ""
                rc = 0
                if label == "project":
                    output = M.PROJECT
                elif label == "components":
                    output = json.dumps([] if scenario == "missing_beta" else [{"id": "beta", "current_version_string": "2026.08.21"}])
                elif label == "machine_type":
                    output = json.dumps({"name": "g4-standard-48", "guestCpus": 48})
                elif label == "quotas":
                    rc = 1 if scenario == "quotas" else 0
                elif label == "image":
                    output = "common-cu129-fixture"
                elif label == "guest_guard_source":
                    output = "set -euo pipefail; printf __EHGP_GUEST_GUARD_VERIFIED__; "
                elif label == "create":
                    self.assertIn("--provisioning-model=SPOT", argv)
                    self.assertIn("--max-run-duration=3600s", argv)
                    self.assertIn("--instance-termination-action=STOP", argv)
                    self.assertIn("--async", argv)
                    self.assertFalse(any("instances start" in str(arg) for arg in argv))
                    nonce = next(arg for arg in argv if arg.startswith("--labels=")).rsplit("=", 1)[1]
                    operation = {"name": "operation-fixture", "operationType": "insert", "zone": "zones/" + ZONE,
                                 "targetLink": "https://www.googleapis.com/compute/v1/projects/" + M.PROJECT +
                                 "/zones/" + ZONE + "/instances/" + argv[4], "insertTime": date(time.time()), "status": "DONE"}
                    output = json.dumps(operation)
                    if scenario not in ("capacity", "timeout_absent"):
                        vm = instance(argv[4], nonce)
                    if scenario == "capacity":
                        operation["error"] = {"errors": [{"code": "RESOURCE_POOL_EXHAUSTED"}]}
                        rc = 1
                    elif scenario == "timeout_absent":
                        operation["status"] = "RUNNING"
                        rc = 124
                    elif scenario == "create_error_owned":
                        rc = 1
                    elif scenario == "post_create_io":
                        writes_fail = True
                        raise OSError("injected create journal EIO")
                    elif scenario == "interrupt":
                        raise M.H.SessionInterrupted("injected SIGINT")
                elif label.startswith("guest_probe_"):
                    output = "MODE=poweroff\nUSEC=" + str((int(time.time()) + 2600) * 1000000) + "\nCREATION_MARK=" + vm["labels"]["ehgp-create"] + "\n"
                    if scenario == "guest_wrong":
                        output = output.replace("poweroff", "reboot")
                    if scenario == "ssh":
                        rc = 1
                else:
                    raise RuntimeError("unmocked command: " + label)
                (folder / (label + ".stdout")).write_text(output)
                (folder / (label + ".stderr")).write_text("")
                return rc

            def safety(argv, timeout, env):
                nonlocal vm, observed_without_generation
                calls.append(("safety", argv))
                if "stop_and_verify.sh" in " ".join(argv):
                    self.assertIsNotNone(vm)
                    self.assertEqual(argv[-2], "--expected-last-start-timestamp")
                    self.assertEqual(argv[-1], vm["lastStartTimestamp"])
                    if scenario == "stop_error":
                        return 1, b"stop failed", b"fixture stderr"
                    vm["status"] = "TERMINATED"
                    return 0, b"TERMINATED", b""
                if argv[1:4] == ["compute", "operations", "list"]:
                    return 0, json.dumps([] if operation is None else [operation]).encode(), b""
                if argv[1:4] == ["compute", "operations", "describe"]:
                    return 0, json.dumps(operation).encode(), b""
                self.assertEqual(argv[1:4], ["compute", "instances", "list"])
                rows = [] if vm is None else [vm]
                if scenario == "generation_materializes" and vm is not None and not observed_without_generation:
                    observed_without_generation = True
                    missing = dict(vm)
                    missing.pop("lastStartTimestamp")
                    rows = [missing]
                return 0, json.dumps(rows).encode(), b""

            with mock.patch.object(M.shutil, "which", return_value="/mock/gcloud"), \
                    mock.patch.object(M, "key_expiration", return_value=date(time.time() + 4000)), \
                    mock.patch.object(M.H, "run_logged", logged), \
                    mock.patch.object(M.H, "safety_command", safety), \
                    mock.patch.object(M.H, "write_json", write), \
                    mock.patch.object(M.time, "monotonic", monotonic), \
                    mock.patch.object(M.time, "sleep"), redirect_stdout(io.StringIO()) as output:
                code = M.create(ZONE, receipt, base / "not-a-real-key")
            state = json.loads(output.getvalue())
            return code, state, calls

    def test_terminal_paths(self):
        scenarios = ("success", "generation_materializes", "missing_beta", "quotas", "capacity", "timeout_absent", "create_error_owned",
                     "post_create_io", "interrupt", "ssh", "guest_wrong", "foreign", "changed_id",
                     "changed_generation", "stop_error")
        for scenario in scenarios:
            with self.subTest(scenario=scenario):
                code, state, calls = self.lifecycle(scenario)
                expected = 0 if scenario in ("success", "generation_materializes") else 130 if scenario == "interrupt" else \
                    74 if scenario in ("timeout_absent", "foreign", "changed_id", "changed_generation", "stop_error") else 1
                self.assertEqual(code, expected, state)
                stops = [args for _, args in calls if "stop_and_verify.sh" in " ".join(args)]
                should_stop = scenario not in ("missing_beta", "quotas", "capacity", "timeout_absent", "foreign", "changed_id", "changed_generation")
                self.assertEqual(len(stops), int(should_stop), state)
                self.assertEqual(state["stopped_verified"], should_stop and scenario != "stop_error")
                self.assertEqual(state["no_session_created"], scenario == "capacity")
                if code == 74:
                    self.assertIn(state["target"]["instance"], state["control_command"])

    def test_guard_boundaries_and_exact_startup_source(self):
        now = time.time()
        nonce = "1" * 24
        M.guest_guard(f"MODE=poweroff\nUSEC={int((now + 200) * 1000000)}\nCREATION_MARK={nonce}\n", int(now + 300), nonce)
        for raw in ("MODE=reboot\nUSEC=9999999999999999\n", "MODE=poweroff\nUSEC=1\n",
                    f"MODE=poweroff\nUSEC={int((now + 500) * 1000000)}\n", "MODE=poweroff\nMODE=poweroff\n"):
            with self.assertRaises(ValueError):
                M.guest_guard(raw, int(now + 300), nonce)
        # This print-only mode is explicitly non-mutating and returns before
        # every gcloud call; do not execute the generated startup script here.
        result = subprocess.run(["bash", str(HERE / "start_and_verify.sh"), "--print-guest-guard-script"],
                                capture_output=True, check=True, timeout=10)
        exact = result.stdout.decode().strip()
        startup = M.startup_script(exact, int(now + 3300), nonce)
        subprocess.run(["bash", "-n"], input=startup.encode(), check=True, timeout=10)
        self.assertIn(M.shlex.quote(exact), startup)
        self.assertIn(" -- 45 ", startup)
        self.assertIn("/sbin/shutdown -P now", startup)
        self.assertNotIn("instances start", startup)
        self.assertIn("0:700", startup)
        self.assertIn("0:600", startup)
        self.assertIn("set -o noclobber", startup)
        with self.assertRaises(ValueError):
            M.create("us-central1-ai1b", Path("/not-created-by-this-test"), Path("/not-a-key"))

    def test_missing_private_key_is_pre_mutation(self):
        with tempfile.TemporaryDirectory() as directory:
            with mock.patch.object(M.H, "safety_command") as command:
                with self.assertRaises(ValueError):
                    M.key_expiration("/mock/gcloud", Path(directory) / "absent", {})
                command.assert_not_called()

    def test_creation_only_startup_second_boot_and_bad_marker(self):
        """Run the generated logic in a sandbox: paths/UID and poweroff mocked.

        No real guard, shutdown or /var/lib write is executed by this test.
        The only transformations are the private root path, shutdown paths,
        and a stat wrapper mapping the test owner's UID to simulated root.
        """
        with tempfile.TemporaryDirectory(prefix="ehgp-startup-test-") as directory:
            base = Path(directory)
            bins = base / "bin"
            bins.mkdir()
            stat = bins / "stat"
            stat.write_text("#!/bin/bash\nset -euo pipefail\nvalue=$(/usr/bin/stat \"$@\")\n"
                            f"printf '%s\\n' \"${{value/#{os.getuid()}:/0:}}\"\n")
            stat.chmod(0o700)
            poweroff = bins / "poweroff"
            poweroff.write_text("#!/bin/bash\nprintf 'poweroff\\n' >> \"$POWER_CALLS\"\n")
            poweroff.chmod(0o700)
            nonce = "2" * 24
            fake_guard = "set -euo pipefail; printf '__EHGP_GUEST_GUARD_VERIFIED__\\n'; printf 'guard\\n' >> \"$GUARD_CALLS\";"
            root = base / "private-root"
            script = M.startup_script(fake_guard, 1, nonce).replace("/var/lib/ehgp-v7-create", str(root)).replace(
                "/sbin/shutdown", str(poweroff)).replace("/sbin/poweroff", str(poweroff))
            env = dict(os.environ, PATH=str(bins) + os.pathsep + os.environ["PATH"],
                       GUARD_CALLS=str(base / "guards"), POWER_CALLS=str(base / "poweroffs"))

            def boot():
                return subprocess.run(["bash"], input=script.encode(), env=env, capture_output=True, timeout=10)

            self.assertEqual(boot().returncode, 0)
            marker = root / nonce
            self.assertEqual(root.stat().st_mode & 0o777, 0o700)
            self.assertEqual(marker.stat().st_mode & 0o777, 0o600)
            self.assertEqual(marker.read_text().strip(), nonce)
            self.assertEqual(boot().returncode, 0)
            self.assertEqual((base / "guards").read_text(), "guard\n")
            self.assertFalse((base / "poweroffs").exists())
            marker.chmod(0o644)
            self.assertEqual(boot().returncode, 1)
            marker.chmod(0o600)
            marker.write_text("wrong nonce\n")
            self.assertEqual(boot().returncode, 1)
            marker.unlink()
            marker.symlink_to(base / "guards")
            self.assertEqual(boot().returncode, 1)
            self.assertEqual((base / "guards").read_text(), "guard\n")
            self.assertEqual((base / "poweroffs").read_text().count("poweroff"), 3)


if __name__ == "__main__":
    unittest.main()
