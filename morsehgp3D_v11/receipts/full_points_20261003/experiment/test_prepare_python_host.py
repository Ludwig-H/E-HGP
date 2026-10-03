"""Pure mocks of the G4 pip-preparation guards and command workflow.

Run with python -B, then python -O -B. No subprocess, metadata request,
subreaper activation, system installation, native product or GCP is allowed.
These checks do not certify real privileged descendant closure.
"""
from __future__ import annotations

import hashlib
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import types
from unittest.mock import patch


def main():
    path = Path(__file__).with_name("prepare_python_host.py").resolve()
    compile(path.read_bytes(), str(path), "exec")
    spec = importlib.util.spec_from_file_location("prepare_python_host", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    checks = 0

    def check(condition):
        nonlocal checks
        if not condition:
            raise RuntimeError("check failed")
        checks += 1

    def refusal(call):
        try:
            call()
        except module.PreparationFailure:
            check(True)
        else:
            check(False)

    env = dict(V11_SRC=str(path.parents[4]), V11_SOURCE_PIN="commit:" + "a" * 40,
               V11_PACKAGE_SHA256="b" * 64, V11_GENERATION="2026-10-03T12:00:00+00:00")
    check(module.worker_context(env, path)["source_commit"] == "a" * 40)
    for key, value in (("V11_SRC", "/tmp"), ("V11_SOURCE_PIN", "a" * 40),
                       ("V11_SOURCE_PIN", "snapshot:" + "a" * 64),
                       ("V11_PACKAGE_SHA256", "x" * 64), ("V11_GENERATION", ""),
                       ("V11_GENERATION", "2026-10-03T12:00:00")):
        refusal(lambda key=key, value=value:
                module.worker_context(dict(env, **{key: value}), path))
    identity = dict(zip(module.KEYS, ("devpod-gpu-exploration", "target", "projects/p/zones/zone",
                                     "projects/p/machineTypes/g4-standard-48")))
    module.exact_identity(identity, "zone", "target")
    check(True)
    for key in module.KEYS:
        refusal(lambda key=key: module.exact_identity(dict(identity, **{key: "wrong"}), "zone", "target"))
    for remaining in (841, 3300):
        check(module.cutoff("USEC=" + str((1000 + remaining) * 1000000) + "\nMODE=poweroff\n",
                            1000)["remaining_seconds"] == remaining)
    for text in ("USEC=1840000000\nMODE=poweroff", "USEC=4301000000\nMODE=poweroff",
                 "USEC=1900000000\nMODE=reboot", "USEC=1900000000\nMODE=poweroff\nDRY_RUN=1",
                 "USEC=1900000000\nUSEC=1900000000\nMODE=poweroff"):
        refusal(lambda text=text: module.cutoff(text, 1000))
    table = {10: dict(ppid=1, pgid=10, start_ticks=1, state="S"),
             11: dict(ppid=10, pgid=10, start_ticks=2, state="S"),
             12: dict(ppid=11, pgid=12, start_ticks=3, state="S"),
             13: dict(ppid=99, pgid=13, start_ticks=4, state="S"),
             14: dict(ppid=1, pgid=14, start_ticks=5, state="S")}
    check(set(module.owned_processes(table, 10, 99)) == {10, 11, 12, 13})

    class FakeRunner:
        def __init__(self, out, report, save, deadline):
            self.out, self.report, self.save = out, report, save

        def command(self, name, argv, limit):
            commands.append((name, argv, limit))
            code = codes.get(name, 0)
            self.report["commands"].append(dict(name=name, argv=argv, returncode=code,
                                                quiescence=dict(certified=True)))
            (self.out / (name + ".stdout")).write_text(
                "USEC=3000000000\nMODE=poweroff\n" if name.startswith("guest_") else "pip test\n")
            (self.out / (name + ".stderr")).write_text("")
            self.save()
            return code

    scenarios = (({}, False, 0), ({"pip_before": 1, "pip_module": 10}, True, 0),
                 ({"pip_before": 1, "pip_module": 10}, False, 1),
                 ({"pip_before": 1, "pip_module": 0}, True, 1),
                 ({"pip_before": 1, "pip_module": 10, "apt_update": 2}, True, 1),
                 ({"pip_before": 1, "pip_module": 10, "apt_install": 3}, True, 1),
                 ({"pip_before": 1, "pip_module": 10, "pip_after": 1}, True, 1))
    for codes, install, expected in scenarios:
        commands = []
        with tempfile.TemporaryDirectory(prefix="mhgp-python-host-mock-") as temporary, \
                patch.dict(os.environ, env, clear=True), \
                patch.object(module, "metadata", side_effect=lambda key: identity[key]), \
                patch.object(module, "enable_subreaper"), patch.object(module, "Runner", FakeRunner), \
                patch.object(module.time, "time", return_value=1000), \
                patch.object(module.subprocess, "Popen", side_effect=RuntimeError("local process prohibited")):
            args = types.SimpleNamespace(out=Path(temporary) / "out", zone="zone", instance="target",
                                         install_missing=install)
            check(module.prepare(args) == expected)
            report = json.loads((args.out / "host_python.json").read_text())
            check(report["status"] == ("ready" if expected == 0 else "failed"))
            apt = [command for command in commands if command[0].startswith("apt_")]
            if not install or codes.get("pip_module") == 0:
                check(not apt)
            for name, argv, limit in apt:
                check(argv[:len(module.APT)] == module.APT)
                if name == "apt_install":
                    check(argv[len(module.APT):] == ["install", "--no-install-recommends", "-y", "python3-pip"])
    print(json.dumps(dict(status="ok", checks=checks,
                         scope="pure mocked guards and workflow; no subprocess, metadata request, native product or GCP",
                         source_sha256=hashlib.sha256(path.read_bytes()).hexdigest()), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
