#!/usr/bin/env python3
"""Local lifecycle and receipt failures; every cloud operation is replaced."""
from __future__ import annotations

import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("v7_session", HERE / "v7_g4_session.py")
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("session module unavailable")
M = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M)
TARGET = {"project": "fake-project", "zone": "fake-zone-a", "instance": "fake-v7"}
GENERATION = "2026-09-04T12:00:00.000Z"
OLD = "2026-09-03T12:00:00.000Z"


def instance(status: str, generation: str) -> dict:
    return {"status": status, "lastStartTimestamp": generation,
            "labels": {"project": "e-hgp"}, "machineType": "machines/g4-standard-48",
            "scheduling": {"provisioningModel": "SPOT", "instanceTerminationAction": "STOP",
                           "onHostMaintenance": "TERMINATE", "automaticRestart": False,
                           "maxRunDuration": {"seconds": "3600"}}}


def fixture(base: Path) -> Path:
    session = base / "session"
    source = session / "source"
    source.mkdir(parents=True)
    records = []
    for relative in [M.SELF, M.WRAPPER, *M.GUARDS]:
        path = source / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        data = Path(M.__file__).read_bytes() if relative == M.SELF else b"mock guard, never executed\n"
        path.write_bytes(data)
        records.append({"path": relative, "size": len(data), "sha256": M.sha(data)})
    M.write_json(source / "source_manifest.json", {"schema": M.SCHEMA, "files": records})
    (session / "source.tgz").write_bytes(b"fake archive, never uploaded")
    M.write_json(session / "prepared.json", {
        "manifest_sha256": M.sha((source / "source_manifest.json").read_bytes()),
        "archive_sha256": M.sha((session / "source.tgz").read_bytes())})
    (base / "fake-key").write_bytes(b"not an SSH key; guards are mocked")
    return session


class SessionTests(unittest.TestCase):
    def test_cpu_bootstrap_sandbox_and_deadlines(self) -> None:
        """Execute fixed shell logic with fake sudo/dpkg/apt, never install."""
        for scenario in ("present", "missing", "apt_failure", "apt_timeout"):
            with self.subTest(scenario=scenario), tempfile.TemporaryDirectory() as name:
                base = Path(name)
                bins = base / "bin"
                bins.mkdir()
                scripts = {
                    "sudo": "#!/bin/bash\nset -euo pipefail\n[[ $1 == -n ]]; shift\nexec \"$@\"\n",
                    "dpkg-query": "#!/bin/bash\n[[ $SCENARIO == present ]] || exit 1\nprintf installed\n",
                    "apt-get": "#!/bin/bash\nset -euo pipefail\nprintf '%s|%s|%s\\n' \"$DEBIAN_FRONTEND\" \"$NEEDRESTART_MODE\" \"$*\" >> \"$APT_CALLS\"\n"
                               "[[ $SCENARIO != apt_failure ]] || exit 42\n"
                               "[[ $SCENARIO != apt_timeout ]] || sleep 60\n",
                    "cmake": "#!/bin/bash\nexit 0\n", "c++": "#!/bin/bash\nexit 0\n", "make": "#!/bin/bash\nexit 0\n"}
                for filename, text in scripts.items():
                    path = bins / filename
                    path.write_text(text)
                    path.chmod(0o700)
                env = dict(os.environ, PATH=str(bins) + os.pathsep + os.environ["PATH"],
                           SCENARIO=scenario, APT_CALLS=str(base / "apt-calls"))
                command = M.cpu_bootstrap_command(1 if scenario == "apt_timeout" else 10)
                subprocess.run(["bash", "-n", "-c", command[-1]], check=True, timeout=10)
                result = subprocess.run(command, env=env, capture_output=True, timeout=15)
                self.assertEqual(result.returncode, 42 if scenario == "apt_failure" else 124 if scenario == "apt_timeout" else 0)
                calls = (base / "apt-calls").read_text().splitlines() if (base / "apt-calls").exists() else []
                self.assertEqual(len(calls), 0 if scenario == "present" else 2 if scenario == "missing" else 1)
                for call in calls:
                    self.assertTrue(call.startswith("noninteractive|l|"))
                    self.assertNotIn("upgrade", call)
                    self.assertNotIn("cuda", call)
                if scenario == "missing":
                    self.assertTrue(calls[1].endswith("install --no-install-recommends -y build-essential cmake libboost-dev time"))
                    self.assertIn(b"CPU_BOOTSTRAP_ACTION=installed", result.stdout)
                if scenario == "present":
                    self.assertIn(b"CPU_BOOTSTRAP_ACTION=already_present", result.stdout)
        for seconds in (0, 301):
            with self.assertRaises(ValueError):
                M.cpu_bootstrap_command(seconds)

    def test_cpu_toolchain_receipt_adverse(self) -> None:
        with tempfile.TemporaryDirectory() as name:
            out = Path(name)
            versions = {"cpu_cmake_version": "cmake version 3.22.1\n",
                        "cpu_compiler_version": "c++ (Ubuntu 11.4) 11.4.0\n",
                        "cpu_make_version": "GNU Make 4.3\n", "cpu_time_version": "time (GNU Time) 1.9\n",
                        "cpu_package_versions": "".join(f"{p}\t1.0\tinstalled\n" for p in M.CPU_PACKAGES)}
            for label, command in M.cpu_version_commands().items():
                M.write_json(out / f"{label}.json", {"argv": command, "status": "completed", "exit_code": 0})
                (out / f"{label}.stdout").write_text(versions[label])
            for label, command in (("cpu_cpp20_compile", M.cpu_probe_compile_command()),
                                   ("cpu_cpp20_probe", ["./out/cpu_cpp20_probe"])):
                M.write_json(out / f"{label}.json", {"argv": command, "status": "completed", "exit_code": 0})
            (out / "cpu_cpp20_probe.cpp").write_text(M.CPU_PROBE)
            (out / "cpu_cpp20_probe.stdout").write_text("cplusplus=202002 boost=107400\n")
            self.assertEqual(set(M.cpu_toolchain_receipt(out)["packages"]), set(M.CPU_PACKAGES))
            for label, bad in (("cpu_cmake_version", "cmake version 3.19.9\n"),
                               ("cpu_package_versions", versions["cpu_package_versions"].replace("installed", "unpacked", 1)),
                               ("cpu_package_versions", versions["cpu_package_versions"].replace("libboost-dev\t", "libboost-dev:amd64\t", 1)),
                               ("cpu_compiler_version", ""),
                               ("cpu_cpp20_probe", "cplusplus=201703 boost=107400\n")):
                path = out / f"{label}.stdout"
                original = path.read_text()
                path.write_text(bad)
                with self.assertRaises(ValueError):
                    M.cpu_toolchain_receipt(out)
                path.write_text(original)

    def test_worker_refuses_missing_guard_before_provision(self) -> None:
        with tempfile.TemporaryDirectory() as name:
            with mock.patch.object(M, "verify_source"), \
                    mock.patch.object(M, "guest_deadline", side_effect=ValueError("missing guest guard")), \
                    mock.patch.object(M, "run_logged") as launched:
                self.assertEqual(M.worker(Path(name), "0" * 64, False), 1)
                launched.assert_not_called()

    def test_real_candidate_parser_and_refusal_boundaries(self) -> None:
        root = HERE.parent
        spec = importlib.util.spec_from_file_location("incidence_fixture_v7", root / "morsehgp3D_v7/tests/incidence_campaign_gate.py")
        fixture_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(fixture_module)
        for wide in (False, True):
            with self.subTest(wide=wide), tempfile.TemporaryDirectory() as name:
                out = Path(name)
                label = "candidate_wide" if wide else "candidate_uniform"
                n = 8000 if wide else 50000
                text = fixture_module.completed_text().replace("n=11 coord=", f"n={n} coord=").replace(
                    "facettes=74 ", f"facettes={n + 63} ").replace("K=1 evenements=10 facettes=11 ", f"K=1 evenements=10 facettes={n} ").replace(
                    "threads=2", "threads=48").replace("meb_supports=25000000", "meb_supports=1000000000")
                record = {"argv": ["/usr/bin/time", "-v", "-o", "fixture.usage", *M.candidate_command(wide)],
                          "exit_code": 0, "status": "completed", "elapsed_seconds": 1.0}
                M.write_json(out / f"{label}.json", record)
                (out / f"{label}.stdout").write_text(text)
                (out / f"{label}.stderr").write_text("")
                (out / f"{label}.usage").write_text(fixture_module.USAGE)
                parsed = M.candidate_observation(out, root, wide=wide)
                self.assertEqual(parsed['status'], 'engine_completed')
                self.assertEqual(len(parsed['digests']), 11)
                self.assertEqual(parsed, json.loads(json.dumps(parsed)))
                for original, changed in [("threads=48", "threads=47"), ("not_claimed", "exact"),
                                           ("digest_forest_K10=", "missing_digest_K10=")]:
                    (out / f"{label}.stdout").write_text(text.replace(original, changed))
                    with self.assertRaises((ValueError, RuntimeError)):
                        M.candidate_observation(out, root, wide=wide)
                record.update(exit_code=2, status="failed")
                M.write_json(out / f"{label}.json", record)
                (out / f"{label}.stdout").write_text("")
                refusal = fixture_module.REFUSAL.replace("rank-relevant extra-shell", "incidence completion requires no rank-relevant extra-shell")
                (out / f"{label}.stderr").write_text(refusal)
                self.assertEqual(M.candidate_observation(out, root, wide=wide)['status'], 'engine_refused')
                (out / f"{label}.stdout").write_text("forbidden prefix")
                with self.assertRaises(ValueError):
                    M.candidate_observation(out, root, wide=wide)
                record.update(exit_code=124, status="censored")
                M.write_json(out / f"{label}.json", record)
                if wide:
                    self.assertEqual(M.candidate_observation(out, root, wide=True)['status'], 'censored')
                else:
                    with self.assertRaises(ValueError):
                        M.candidate_observation(out, root, wide=False)

    def test_bounded_process_and_missing_binary(self) -> None:
        with tempfile.TemporaryDirectory() as name:
            path = Path(name)
            command = [sys.executable, "-c", "print('literal $() survives')"]
            self.assertEqual(M.run_logged(path, "ok", command, 5), 0)
            self.assertEqual(json.loads((path / "ok.json").read_text())["argv"], command)
            self.assertIn("$()", (path / "ok.stdout").read_text())
            self.assertEqual(M.run_logged(path, "missing", [str(path / "absent")], 5), 127)
            self.assertEqual(json.loads((path / "missing.json").read_text())["status"], "failed")

    def test_timeout_drains_owned_process_group(self) -> None:
        with tempfile.TemporaryDirectory() as name:
            path = Path(name)
            script = ("import subprocess,sys,time;"
                      "p=subprocess.Popen([sys.executable,'-c','import time;time.sleep(60)']);"
                      "print(p.pid,flush=True);time.sleep(60)")
            self.assertEqual(M.run_logged(path, "timeout", [sys.executable, "-c", script], 1), 124)
            pid = int((path / "timeout.stdout").read_text())
            proc = Path(f"/proc/{pid}/stat")
            for _ in range(100):
                if not proc.exists() or proc.read_text().split()[2] == "Z":
                    break
                time.sleep(.01)
            if proc.exists():
                self.assertEqual(proc.read_text().split()[2], "Z")
            self.assertEqual(json.loads((path / "timeout.json").read_text())["status"], "censored")

    def test_term_ignoring_descendant_and_exited_leader(self) -> None:
        for finish in ("time.sleep(60)", "sys.exit(0)"):
            with self.subTest(finish=finish), tempfile.TemporaryDirectory() as name:
                path = Path(name)
                child = ("import os,signal,sys,time;signal.signal(signal.SIGTERM,signal.SIG_IGN);"
                         "open(sys.argv[1],'w').write(str(os.getpid()));time.sleep(60)")
                script = ("import subprocess,sys,time,pathlib;"
                          f"p=subprocess.Popen([sys.executable,'-c',{child!r},{str(path / 'pid')!r}]);"
                          f"ready=pathlib.Path({str(path / 'pid')!r});\n"
                          "while not ready.exists(): time.sleep(.005)\n" + finish)
                self.assertEqual(M.run_logged(path, "descendant", [sys.executable, "-c", script], 1),
                                 0 if finish == "sys.exit(0)" else 124)
                pid = int((path / "pid").read_text())
                for _ in range(100):
                    proc = Path(f"/proc/{pid}/stat")
                    if not proc.exists() or proc.read_text().split()[2] == "Z":
                        break
                    time.sleep(.01)
                if proc.exists():
                    self.assertEqual(proc.read_text().split()[2], "Z")

    def test_snapshot_and_result_mutations(self) -> None:
        with tempfile.TemporaryDirectory() as name:
            base = Path(name)
            session = fixture(base)
            expected = json.loads((session / "prepared.json").read_text())["manifest_sha256"]
            M.verify_source(session / "source", expected)
            extra = session / "source" / "unexpected.hpp"
            extra.write_text("not pinned")
            with self.assertRaises(ValueError):
                M.verify_source(session / "source", expected)
            extra.unlink()
            source = session / "source" / M.GUARDS[0]
            source.write_text("mutated")
            with self.assertRaises(ValueError):
                M.verify_source(session / "source", expected)
            out = base / "out"
            out.mkdir()
            (out / "run.txt").write_text("complete")
            digest = M.seal(out)
            M.verify_results(out, digest)
            (out / "run.txt").write_text("corrupt")
            with self.assertRaises(ValueError):
                M.verify_results(out, digest)

    def test_target_guard_and_handoff_identity(self) -> None:
        good = instance("TERMINATED", OLD)
        self.assertTrue(M.target_valid(good, stopped=True))
        for field, value in [("provisioningModel", "STANDARD"), ("instanceTerminationAction", "DELETE"),
                             ("automaticRestart", True), ("maxRunDuration", {"seconds": "3601"})]:
            changed = json.loads(json.dumps(good))
            changed["scheduling"][field] = value
            self.assertFalse(M.target_valid(changed, stopped=True))
        with tempfile.TemporaryDirectory() as name:
            path = Path(name)
            M.write_json(path / "start-handoff.json", dict(TARGET, schema="e-hgp.start-handoff.v3",
                         status="targeted_running", guest_shutdown_minutes=45, last_start_timestamp=GENERATION))
            self.assertEqual(M.known_generation(path, TARGET), GENERATION)
            with self.assertRaises(ValueError):
                M.known_generation(path, dict(TARGET, instance="foreign"))
            (path / "start-handoff.json").write_text("truncated JSON")
            for state in ("start_may_have_been_requested", "targeted_stop_failed", "targeted_stopped"):
                data = dict(TARGET, schema="e-hgp.lifecycle-state.v1", state=state, generation=GENERATION)
                (path / "lifecycle.txt").write_text("".join(f"{k}={v}\n" for k, v in data.items()))
                self.assertEqual(M.known_generation(path, TARGET), GENERATION)
                with self.assertRaises(ValueError):
                    M.known_generation(path, dict(TARGET, instance="foreign"))

    def test_real_snapshot_guard_trap_with_fake_cloud(self) -> None:
        """Execute the unchanged emergency function and real stop, never real gcloud."""
        with tempfile.TemporaryDirectory() as name:
            base = Path(name)
            root = HERE.parent
            with mock.patch.object(M, "source_paths", return_value=[root / p for p in M.GUARDS + [M.SELF, M.WRAPPER]]):
                session = M.prepare(root, base / "sessions")
            source = session / "source"
            expected = json.loads((session / "prepared.json").read_text())["manifest_sha256"]
            M.verify_source(source, expected)
            for relative in M.GUARDS + [M.WRAPPER]:
                self.assertEqual((source / relative).stat().st_mode & 0o777, 0o555)
            fakebin = base / "fakebin"
            fakebin.mkdir()
            fake = fakebin / "gcloud"
            fake.write_text("#!/usr/bin/env python3\nimport json,os,pathlib,sys\n"
                            "a=sys.argv[1:];p=pathlib.Path(os.environ['FAKE_BASE'])\n"
                            "with (p/'calls').open('a') as f:f.write(json.dumps(a)+'\\n')\n"
                            "if a[:3]==['config','get-value','project']: print(os.environ['GCP_PROJECT_ID'])\n"
                            "elif a[:3]==['compute','instances','list']:\n"
                            " if '--format=value(name)' in a:print(os.environ['GCP_INSTANCE_NAME'])\n"
                            "elif a[:3]==['compute','instances','stop']: (p/'stopped').touch()\n"
                            "elif a[:3]==['compute','instances','describe']:\n"
                            " f=next(x for x in a if x.startswith('--format='))\n"
                            " values={'--format=value(labels.project)':'e-hgp',"
                            "'--format=value(lastStartTimestamp)':os.environ['FAKE_GENERATION'],"
                            "'--format=value(status)':'TERMINATED' if (p/'stopped').exists() else 'RUNNING'}\n"
                            " if f not in values:sys.exit(9)\n"
                            " print(values[f])\n"
                            "else:sys.exit(9)\n")
            fake.chmod(0o700)
            guard = (source / M.GUARDS[0]).read_text()
            function = guard[guard.index("emergency_stop_on_exit() {"):guard.index("# Enregistrement de cycle de vie : publication ATOMIQUE")]
            driver = source / "gcp-migration/start_guard_trap_v7.sh"
            driver.write_text("#!/usr/bin/env bash\nset -euo pipefail\n"
                              "INSTANCE_NAME=$GCP_INSTANCE_NAME; PROJECT_ID=$GCP_PROJECT_ID; ZONE=$GCP_ZONE\n"
                              "TARGET_LAST_START_TIMESTAMP=$FAKE_GENERATION; HANDOFF_FILE=''\n"
                              "start_attempted=1; start_certified=0\n"
                              "publish_lifecycle_state(){ printf '%s\\n' \"$1\" >> \"$FAKE_BASE/lifecycle\"; }\n"
                              + function + "\ntrap emergency_stop_on_exit EXIT\nexit 1\n")
            env = dict(os.environ, PATH=str(fakebin) + os.pathsep + os.environ['PATH'],
                       FAKE_BASE=str(base), FAKE_GENERATION=GENERATION,
                       GCP_PROJECT_ID=TARGET['project'], GCP_ZONE=TARGET['zone'], GCP_INSTANCE_NAME=TARGET['instance'])
            result = subprocess.run(["bash", str(driver)], env=env, capture_output=True, timeout=20)
            self.assertEqual(result.returncode, 1, result.stderr.decode())
            self.assertTrue((base / "stopped").exists(), result.stderr.decode())
            self.assertIn("targeted_stopped", (base / "lifecycle").read_text())
            calls = [json.loads(row) for row in (base / "calls").read_text().splitlines()]
            stops = [a for a in calls if a[:3] == ['compute', 'instances', 'stop']]
            self.assertEqual(len(stops), 1)
            self.assertEqual(stops[0][3], TARGET['instance'])
            self.assertIn('--project=' + TARGET['project'], stops[0])
            self.assertIn('--zone=' + TARGET['zone'], stops[0])

    def lifecycle(self, scenario: str) -> tuple[int, dict, list]:
        with tempfile.TemporaryDirectory() as name:
            base = Path(name)
            session = fixture(base)
            calls = []
            original_write = M.write_json
            final_io_failure = False

            def write(path, value):
                if scenario == "log_io" and final_io_failure:
                    raise OSError("injected disk full")
                original_write(path, value)

            def logged(directory, label, argv, _timeout, **_kwargs):
                nonlocal final_io_failure
                calls.append((label, argv))
                out = ""
                code = 0
                if label == "preflight":
                    out = json.dumps(instance("TERMINATED", OLD))
                elif label == "start":
                    if scenario == "capacity":
                        code = 1
                    else:
                        M.write_json(session / "start-handoff.json",
                                     dict(TARGET, schema="e-hgp.start-handoff.v3", status="targeted_running",
                                          guest_shutdown_minutes=45, last_start_timestamp=GENERATION))
                        marker = dict(TARGET, schema="e-hgp.guard-mark.v1", mark="double_guard_verified",
                                      generation=GENERATION, max_run_seconds="3600", guest_shutdown_minutes="45")
                        (session / "guard-marks/double_guard_verified").write_text(
                            "".join(f"{k}={v}\n" for k, v in marker.items()))
                        if scenario == "missing_double_guard":
                            (session / "guard-marks/double_guard_verified").unlink()
                        out = "[GARDE SSH] expiration fixe=2026-09-04T13:10:00.000000Z, durée restante=4000s.\n"
                        if scenario == "start_error":
                            code = 1
                elif label == "upload":
                    if scenario == "interrupt":
                        raise M.SessionInterrupted("injected interrupt")
                    if scenario == "log_io":
                        final_io_failure = True
                        raise OSError("injected journal unavailable")
                    if scenario == "upload":
                        code = 1
                elif label == "remote":
                    remote_out = base / "remote-out"
                    remote_out.mkdir()
                    (remote_out / "data.txt").write_text("fixture receipt")
                    out = "RESULT_MANIFEST_SHA256=" + M.seal(remote_out) + "\n"
                    if scenario == "worker":
                        code = 1
                elif label == "download":
                    if scenario == "download":
                        code = 1
                    else:
                        shutil.copytree(base / "remote-out", session / "received/out")
                (directory / f"{label}.stdout").write_text(out)
                (directory / f"{label}.stderr").write_text("")
                return code

            def safety(argv, _timeout, _env):
                calls.append(("safety", argv))
                if "stop_and_verify.sh" in " ".join(argv):
                    self.assertIn("--expected-last-start-timestamp", argv)
                    self.assertEqual(argv[-1], GENERATION)
                    return (1 if scenario == "stop_error" else 0), b"TERMINATED exact target\n", b""
                if scenario == "unreadable":
                    return 1, b"", b"injected unreadable state"
                generation = OLD if scenario == "capacity" else GENERATION
                return 0, json.dumps(instance("TERMINATED", generation)).encode(), b""

            with mock.patch.object(M, "run_logged", logged), \
                    mock.patch.object(M, "safety_command", safety), \
                    mock.patch.object(M, "validate_receipt") as validated, \
                    mock.patch.object(M.shutil, "which", return_value="/fake/gcloud"), \
                    mock.patch.object(M, "write_json", write):
                code = M.session_run(session, TARGET, base / "fake-key", False)
                if scenario == "success":
                    validated.assert_called_once()
            if scenario == "log_io":
                state = {"stopped_verified": any("stop_and_verify.sh" in " ".join(cmd) for _, cmd in calls)}
            else:
                state = json.loads((session / "session.json").read_text())
            return code, state, calls

    def test_lifecycle_all_terminal_paths(self) -> None:
        for scenario in ("success", "capacity", "start_error", "missing_double_guard", "upload", "worker", "download",
                         "interrupt", "log_io", "stop_error", "unreadable"):
            with self.subTest(scenario=scenario):
                code, state, calls = self.lifecycle(scenario)
                expected = 0 if scenario == "success" else 130 if scenario == "interrupt" else \
                    74 if scenario in ("stop_error", "unreadable") else 1
                self.assertEqual(code, expected)
                self.assertEqual(state["stopped_verified"], scenario not in ("stop_error", "unreadable"))
                stops = [cmd for _, cmd in calls if "stop_and_verify.sh" in " ".join(cmd)]
                self.assertEqual(len(stops), 0 if scenario == "capacity" else 1)
                if scenario == "missing_double_guard":
                    self.assertFalse(any(label in ("upload", "remote") for label, _ in calls))
                if scenario == "success":
                    ssh = next(cmd for label, cmd in calls if label == "remote")
                    self.assertIn("--ssh-key-expiration=2026-09-04T13:10:00.000000Z", ssh)

    def test_guest_guard(self) -> None:
        with tempfile.TemporaryDirectory() as name:
            path = Path(name) / "scheduled"
            path.write_text(f"MODE=poweroff\nUSEC={(int(time.time()) + 600) * 1000000}\n")
            self.assertGreater(M.guest_deadline(path), time.time())
            path.write_text("MODE=reboot\nUSEC=9999999999999999\n")
            with self.assertRaises(ValueError):
                M.guest_deadline(path)


if __name__ == "__main__":
    unittest.main()
