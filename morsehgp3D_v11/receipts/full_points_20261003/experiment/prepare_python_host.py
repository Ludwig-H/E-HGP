"""Prepare system pip only inside an explicitly authorized guarded G4 session.

No product build, fit, or scientific Python package installation is performed.
The controller remains responsible for generation certification and VM stop.
"""
from __future__ import annotations

import argparse
import ctypes
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time
import urllib.error
import urllib.request


KEYS = ("project/project-id", "instance/name", "instance/zone", "instance/machine-type")
PIP = ["/usr/bin/python3", "-m", "pip", "--version"]
APT = ["sudo", "-n", "env", "DEBIAN_FRONTEND=noninteractive", "apt-get",
       "-o", "DPkg::Lock::Timeout=30", "-o", "Dpkg::Use-Pty=0"]


class PreparationFailure(RuntimeError):
    pass


def need(condition, message):
    if not condition:
        raise PreparationFailure(message)


def sha256(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        while block := stream.read(1024 * 1024):
            result.update(block)
    return result.hexdigest()


def worker_context(env, wrapper):
    repository = wrapper.resolve().parents[4]
    need(env.get("V11_SRC") and Path(env["V11_SRC"]).resolve() == repository,
         "exact_worker_source_required")
    pin = env.get("V11_SOURCE_PIN", "")
    match = re.fullmatch(r"commit:([0-9a-f]{40})", pin)
    need(match is not None, "published_commit_pin_required")
    package = env.get("V11_PACKAGE_SHA256", "")
    need(re.fullmatch(r"[0-9a-f]{64}", package) is not None, "package_sha256_required")
    generation = env.get("V11_GENERATION", "")
    try:
        parsed = datetime.datetime.fromisoformat(generation.replace("Z", "+00:00"))
    except ValueError as error:
        raise PreparationFailure("generation_timestamp_required") from error
    need(parsed.tzinfo is not None, "generation_timezone_required")
    return dict(repository=str(repository), source_pin=pin, source_kind="commit",
                source_commit=match.group(1), package_sha256=package, generation=generation,
                evidence_scope="worker_context; generation independently certified by controller")


def exact_identity(identity, zone, instance):
    need(identity.get("project/project-id") == "devpod-gpu-exploration" and
         identity.get("instance/name") == instance and
         identity.get("instance/zone", "").rsplit("/", 1)[-1] == zone and
         identity.get("instance/machine-type", "").rsplit("/", 1)[-1] == "g4-standard-48",
         "exact_g4_target_required")


def cutoff(text, now, initial=True):
    fields = {}
    for line in text.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            need(key not in fields, "duplicate_cutoff_field")
            fields[key] = value
    need(fields.get("MODE") == "poweroff" and fields.get("DRY_RUN", "0") == "0" and
         re.fullmatch(r"[0-9]+", fields.get("USEC", "")) is not None,
         "active_guest_poweroff_required")
    remaining = int(fields["USEC"]) / 1_000_000 - now
    need(840 < remaining <= 3300 if initial else remaining > 120,
         "bounded_guest_cutoff_required")
    return dict(fields=fields, observed_unix=now, remaining_seconds=remaining)


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *_args, **_kwargs):
        raise urllib.error.URLError("metadata_redirect_refused")


def metadata(key):
    need(key in KEYS, "unexpected_metadata_key")
    request = urllib.request.Request("http://169.254.169.254/computeMetadata/v1/" + key,
                                     headers={"Metadata-Flavor": "Google"})
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect())
    with opener.open(request, timeout=5) as response:
        need(response.headers.get("Metadata-Flavor") == "Google", "gce_metadata_header_required")
        value = response.read(4097)
        need(len(value) <= 4096, "metadata_too_large")
        return value.decode("utf-8").strip()


def enable_subreaper():
    library = ctypes.CDLL(None, use_errno=True)
    need(library.prctl(36, 1, 0, 0, 0) == 0, "linux_subreaper_required")


def process_table():
    """Read only identity/ancestry from /proc; never process argv or environment."""
    result = {}
    for name in os.listdir("/proc"):
        if not name.isdecimal():
            continue
        try:
            data = (Path("/proc") / name / "stat").read_text()
        except (FileNotFoundError, ProcessLookupError):
            continue
        # comm may contain spaces and parentheses; fields start after its final ')'.
        fields = data[data.rfind(")") + 2:].split()
        need(len(fields) >= 20, "unreadable_process_identity")
        result[int(name)] = dict(state=fields[0], ppid=int(fields[1]),
                                 pgid=int(fields[2]), start_ticks=int(fields[19]))
    return result


def owned_processes(table, pgid, parent):
    owned = {pid for pid, row in table.items() if row["pgid"] == pgid or row["ppid"] == parent}
    while True:
        extended = owned | {pid for pid, row in table.items() if row["ppid"] in owned}
        if extended == owned:
            break
        owned = extended
    owned.discard(parent)
    return {pid: table[pid] for pid in sorted(owned)}


def reap_adopted():
    while True:
        try:
            pid, _status = os.waitpid(-1, os.WNOHANG)
        except ChildProcessError:
            return
        if pid == 0:
            return


def close_descendants(process):
    """Join the leader and adopted children; fail closed if a descendant survives."""
    pgid = process.pid
    need(pgid > 1 and pgid != os.getpgrp(), "private_group_required")
    report = dict(pgid=pgid, certified=False, signals=[],
                  scope="private process group and /proc descendants; Linux subreaper")
    deadline = time.monotonic() + 20

    def send(target, sig):
        # apt/dpkg children may belong to root. Signal only a currently owned group/PID.
        try:
            if target < 0:
                os.killpg(-target, sig)
            else:
                os.kill(target, sig)
            report["signals"].append(dict(target=target, signal=sig.name, method="kill"))
        except ProcessLookupError:
            return
        except PermissionError:
            argv = ["sudo", "-n", "/usr/bin/kill", "-" + sig.name.removeprefix("SIG"),
                    "--", str(target)]
            helper = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                                      stderr=subprocess.DEVNULL, start_new_session=True)
            try:
                code = helper.wait(timeout=3)
            except BaseException:
                helper.kill()
                helper.wait(timeout=3)
                raise
            report["signals"].append(dict(target=target, signal=sig.name, method="sudo_kill",
                                           argv=argv, returncode=code))
            # An exit 1 can mean the target disappeared between snapshot and signal.

    for sig in (signal.SIGTERM, signal.SIGKILL):
        table = process_table()
        owned = owned_processes(table, pgid, os.getpid())
        if owned:
            if any(row["pgid"] == pgid for row in owned.values()):
                send(-pgid, sig)
            for pid, row in owned.items():
                if row["pgid"] != pgid and row["state"] != "Z":
                    current = process_table().get(pid)
                    if current and current["start_ticks"] == row["start_ticks"]:
                        send(pid, sig)
        until = min(deadline, time.monotonic() + (2 if sig == signal.SIGTERM else 10))
        empty_observations = 0
        while time.monotonic() < until:
            # Popen must record its leader's real status before waitpid(-1) is used.
            if process.poll() is not None:
                process.wait()
                reap_adopted()
            remaining = owned_processes(process_table(), pgid, os.getpid())
            empty_observations = empty_observations + 1 if not remaining else 0
            if empty_observations >= 2 and process.returncode is not None:
                report.update(certified=True, leader_joined=True, remaining=[])
                return report
            time.sleep(0.05)
    report["remaining"] = list(owned_processes(process_table(), pgid, os.getpid()))
    raise PreparationFailure("descendant_quiescence_not_certified:" + json.dumps(report))


class Runner:
    def __init__(self, out, report, save, deadline):
        self.out, self.report, self.save, self.deadline = out, report, save, deadline

    def command(self, name, argv, limit):
        effective = min(limit, self.deadline - time.monotonic())
        need(effective > 0, "preparation_active_budget_exhausted")
        row = dict(name=name, argv=argv, state="intent", timeout_seconds=limit,
                   effective_timeout_seconds=effective, intent_unix_ns=time.time_ns())
        self.report["commands"].append(row)
        self.save()
        process = None
        try:
            with (self.out / (name + ".stdout")).open("wb") as stdout, \
                    (self.out / (name + ".stderr")).open("wb") as stderr:
                process = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr,
                                           start_new_session=True)
                row.update(state="running", pid=process.pid, started_unix_ns=time.time_ns())
                self.save()
                try:
                    row["returncode"] = process.wait(timeout=effective)
                    row["state"] = "returned"
                except subprocess.TimeoutExpired as error:
                    row["state"] = "timed_out"
                    raise PreparationFailure("command_timeout:" + name) from error
        finally:
            if process is not None:
                row["quiescence"] = close_descendants(process)
                row.setdefault("returncode", process.returncode)
            row["finished_unix_ns"] = time.time_ns()
            self.save()
        return row["returncode"]


def prepare(args):
    wrapper = Path(__file__).resolve()
    need(not args.out.is_symlink(), "output_symlink_refused")
    out = args.out.resolve()
    need(not out.is_relative_to(wrapper.parents[4]), "output_inside_source_refused")
    need(not out.exists() or out.is_dir() and not any(out.iterdir()), "fresh_output_required")
    out.mkdir(parents=True, exist_ok=True)
    before = sha256(wrapper)
    report = dict(schema="ehgp.v11.python_host.v1", status="preflight", commands=[],
                  product_executed=False, scientific_packages_installed=False,
                  requested_system_packages=["python3-pip"], install_missing=args.install_missing,
                  source=dict(path=str(wrapper), sha256_before=before),
                  active_budget_seconds=570, cleanup_reserve_seconds=30)
    path = out / "host_python.json"

    def save():
        report["files"] = {item.name: dict(bytes=item.stat().st_size, sha256=sha256(item))
                           for item in sorted(out.iterdir()) if item.is_file() and
                           item.name not in ("host_python.json", "host_python.tmp")}
        temporary = out / "host_python.tmp"
        temporary.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
        temporary.replace(path)

    def interrupted(signum, _frame):
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        raise PreparationFailure("interrupted_signal:" + str(signum))

    previous = {sig: signal.getsignal(sig) for sig in (signal.SIGTERM, signal.SIGINT)}
    deadline = time.monotonic() + 570
    save()
    try:
        report["worker"] = worker_context(os.environ, wrapper)
        report["identity"] = {key: metadata(key) for key in KEYS}
        exact_identity(report["identity"], args.zone, args.instance)
        enable_subreaper()
        report["subreaper"] = True
        for sig in previous:
            signal.signal(sig, interrupted)
        runner = Runner(out, report, save, deadline)
        need(runner.command("guest_before", ["sudo", "-n", "cat", "/run/systemd/shutdown/scheduled"], 15) == 0,
             "guest_cutoff_unreadable")
        report["guest_before"] = cutoff((out / "guest_before.stdout").read_text(), time.time())
        save()
        code = runner.command("pip_before", PIP, 10)
        report["pip_before"] = dict(returncode=code)
        if code != 0:
            # Distinguish a missing module from a broken existing pip installation.
            check = ["/usr/bin/python3", "-c", "import importlib.util; "
                     "raise SystemExit(10 if importlib.util.find_spec('pip') is None else 0)"]
            missing = runner.command("pip_module", check, 10)
            need(missing == 10, "existing_pip_failed; repair_not_authorized_by_missing_tool_plan")
            need(args.install_missing, "missing_pip; explicit_install_missing_required")
            need(runner.command("apt_update", APT + ["update"], 180) == 0, "apt_update_failed")
            need(runner.command("apt_install", APT + ["install", "--no-install-recommends", "-y",
                                                       "python3-pip"], 360) == 0, "apt_install_failed")
        need(runner.command("pip_after", PIP, 10) == 0, "system_pip_version_failed")
        need(runner.command("guest_after", ["sudo", "-n", "cat", "/run/systemd/shutdown/scheduled"], 15) == 0,
             "final_guest_cutoff_unreadable")
        report["guest_after"] = cutoff((out / "guest_after.stdout").read_text(), time.time(), initial=False)
        need(report["guest_after"]["fields"]["USEC"] == report["guest_before"]["fields"]["USEC"],
             "guest_cutoff_changed")
        report["source"]["sha256_after"] = sha256(wrapper)
        need(report["source"]["sha256_after"] == before, "wrapper_source_changed")
        need(all(row.get("quiescence", {}).get("certified") for row in report["commands"]),
             "command_quiescence_missing")
        report["status"] = "ready"
        save()
        return 0
    except (OSError, ValueError, PreparationFailure, subprocess.SubprocessError) as error:
        report.update(status="failed", error=type(error).__name__ + ": " + str(error))
        report["source"]["sha256_after"] = sha256(wrapper)
        save()
        return 1
    finally:
        for sig, handler in previous.items():
            signal.signal(sig, handler)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--zone", required=True)
    parser.add_argument("--instance", required=True)
    parser.add_argument("--install-missing", action="store_true")
    return prepare(parser.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
