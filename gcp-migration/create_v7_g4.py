#!/usr/bin/env python3
"""Create one guarded G4 SPOT, verify both cutoffs, and return it TERMINATED.

No workload runs here and no instances.start call exists. A subsequent session
must use start_and_verify.sh through the reviewed v7 session controller.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import importlib.util
import json
import os
from pathlib import Path
import re
import secrets
import shlex
import shutil
import signal
import sys
import time
from typing import Any


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("v7_creation_helpers", HERE / "v7_g4_session.py")
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("reviewed lifecycle helpers unavailable")
H = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(H)
PROJECT = "devpod-gpu-exploration"
IMAGE_PROJECT = "deeplearning-platform-release"
IMAGE_FAMILY = "common-cu129-ubuntu-2204-nvidia-580"
SCHEMA = "ehgp.v7.g4.creation.v1"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def timestamp(value: Any) -> float:
    require(H.valid_generation(value), "invalid generation timestamp")
    return datetime.fromisoformat(value.replace("Z", "+00:00")).timestamp()


def key_expiration(gcloud: str, key: Path, env: dict[str, str]) -> str:
    require(key.is_absolute() and key.is_file() and not key.is_symlink(), "absolute regular session key required")
    require(key.stat().st_mode & 0o777 == 0o600, "private key must be 0600")
    public = Path(str(key) + ".pub")
    require(public.is_file() and not public.is_symlink(), "regular public key required")
    fields = public.read_text().split()
    require(len(fields) >= 2 and fields[0] == "ssh-ed25519", "ED25519 session key required")
    rc, derived, _ = H.safety_command(["ssh-keygen", "-y", "-P", "", "-f", str(key)], 10, env)
    require(rc == 0 and derived.decode().split()[:2] == fields[:2], "session key encrypted or mismatched")
    # Capture in memory only: do not publish the OS Login profile or key bytes.
    rc, raw, _ = H.safety_command([gcloud, "compute", "os-login", "describe-profile",
                                  "--project=" + PROJECT, "--format=json"], 30, env)
    require(rc == 0, "OS Login session expiry unreadable")
    profile = json.loads(raw)
    matches = [row.get("expirationTimeUsec") for row in profile.get("sshPublicKeys", {}).values()
               if str(row.get("key", "")).split()[:2] == fields[:2]]
    require(len(matches) == 1 and not isinstance(matches[0], bool), "unique OS Login expiry required")
    expiry = int(matches[0]) / 1000000
    remaining = expiry - time.time()
    require(3600 <= remaining <= 4260, "OS Login expiry must leave 3600..4260 seconds")
    return datetime.fromtimestamp(expiry, timezone.utc).isoformat().replace("+00:00", "Z")


def ownership(info: dict[str, Any], target: dict[str, str], nonce: str,
              intent_epoch: int) -> tuple[str, str | None]:
    """A fresh random label and exact resource path distinguish our creation."""
    require(info.get("name") == target["instance"] and
            info.get("zone", "").rsplit("/", 1)[-1] == target["zone"] and
            info.get("selfLink", "").endswith("/projects/" + target["project"] + "/zones/" +
                                              target["zone"] + "/instances/" + target["instance"]),
            "created target identity mismatch")
    require(info.get("labels", {}).get("project") == "e-hgp" and
            info.get("labels", {}).get("ehgp-create") == nonce, "creation ownership label mismatch; no stop authorized")
    identity = str(info.get("id", ""))
    require(re.fullmatch(r"[1-9][0-9]*", identity) is not None, "resource id missing")
    generation = info.get("lastStartTimestamp")
    created = timestamp(info.get("creationTimestamp"))
    require(intent_epoch - 300 <= created <= time.time() + 300, "creation timestamp outside owned operation")
    if generation is not None:
        started = timestamp(generation)
        require(created - 300 <= started <= time.time() + 300, "start timestamp outside owned operation")
    return identity, generation


def gce_guard(info: dict[str, Any], cutoff: int) -> None:
    require(H.target_valid(info, stopped=False) and info.get("status") == "RUNNING",
            "created G4 SPOT STOP/3600 GCE guard invalid")
    end = timestamp(info.get("lastStartTimestamp")) + 3600
    locations = [info.get("resourceStatus", {}).get("scheduling", {}).get("terminationTimestamp"),
                 info.get("scheduling", {}).get("terminationTimestamp"), info.get("terminationTimestamp")]
    terminations = [timestamp(value) for value in locations if value is not None]
    require(bool(terminations) and len(set(terminations)) == 1, "GCE termination timestamp missing or inconsistent")
    termination = terminations[0]
    require(time.time() < cutoff <= end and abs(termination - end) <= 300 and cutoff <= termination,
            "created GCE cutoff/deadline invalid")


def guest_guard(raw: str, cutoff: int, nonce: str) -> None:
    fields = {}
    for line in raw.splitlines():
        key, sep, value = line.partition("=")
        require(bool(sep) and key not in fields, "malformed guest shutdown record")
        fields[key] = value
    require(fields.get("MODE") == "poweroff" and re.fullmatch(r"[0-9]{1,18}", fields.get("USEC", "")) is not None,
            "guest shutdown absent")
    scheduled = int(fields["USEC"]) / 1000000
    require(time.time() + 60 < scheduled <= cutoff, "guest cutoff outside conservative GCE deadline")
    require(fields.get("CREATION_MARK") == nonce, "creation-only guard marker absent or mismatched")


def startup_script(guard: str, cutoff: int, nonce: str) -> str:
    require(guard.startswith("set -euo pipefail;") and "__EHGP_GUEST_GUARD_VERIFIED__" in guard,
            "unexpected exact guest guard text")
    require(re.fullmatch(r"[0-9a-f]{24}", nonce) is not None, "invalid creation nonce")
    # Mark only after the exact guard succeeded. Subsequent boots skip this
    # creation-only absolute cutoff; start_and_verify arms their own new guard.
    marker = "/var/lib/ehgp-v7-create/" + nonce
    return ("#!/bin/bash\nset -euo pipefail\n"
            "fail_closed() { /sbin/shutdown -P now || /sbin/poweroff -f; exit 1; }\n"
            "if [[ -e /var/lib/ehgp-v7-create || -L /var/lib/ehgp-v7-create ]]; then\n"
            "  [[ -d /var/lib/ehgp-v7-create && ! -L /var/lib/ehgp-v7-create && "
            "$(stat -c '%u:%a' /var/lib/ehgp-v7-create) == 0:700 ]] || fail_closed\nfi\n"
            "if [[ -e " + marker + " || -L " + marker + " ]]; then\n"
            "  [[ -f " + marker + " && ! -L " + marker + " && $(stat -c '%u:%a' " + marker + ") == 0:600 && "
            "\"$(cat " + marker + ")\" == " + nonce + " ]] || fail_closed\n  exit 0\nfi\n"
            "if ! /bin/bash -c " + shlex.quote(guard) +
            " -- 45 " + str(cutoff) + "; then\n"
            "  /sbin/shutdown -P now || /sbin/poweroff -f\n  exit 1\nfi\n"
            "install -d -m 700 /var/lib/ehgp-v7-create || fail_closed\n"
            "[[ ! -L /var/lib/ehgp-v7-create && $(stat -c '%u:%a' /var/lib/ehgp-v7-create) == 0:700 ]] || fail_closed\n"
            "(umask 077; set -o noclobber; printf '%s\\n' " + nonce + " > " + marker + ") || fail_closed\n"
            "chmod 600 " + marker + " || fail_closed\n"
            "[[ -f " + marker + " && ! -L " + marker + " && $(stat -c '%u:%a' " + marker + ") == 0:600 && "
            "\"$(cat " + marker + ")\" == " + nonce + " ]] || fail_closed\n")


def create(zone: str, receipt: Path, ssh_key: Path) -> int:
    require(re.fullmatch(r"[a-z]+-[a-z]+[0-9]+-[a-z]", zone) is not None, "standard zone required; no AI-zone mapping inferred")
    require(receipt.is_absolute() and not receipt.is_symlink(), "absolute new receipt directory required")
    receipt.mkdir(mode=0o700, parents=True, exist_ok=False)
    require(receipt.stat().st_mode & 0o777 == 0o700, "private receipt directory required")
    gcloud = shutil.which("gcloud")
    require(gcloud is not None, "gcloud unavailable")
    nonce = secrets.token_hex(12)
    target = {"project": PROJECT, "zone": zone, "instance": "ehgp-v7-" + nonce}
    env = dict(os.environ, GCP_PROJECT_ID=PROJECT, GCP_ZONE=zone, GCP_REGION=zone.rsplit("-", 1)[0],
               GCP_INSTANCE_NAME=target["instance"], LC_ALL="C", LANG="C")
    state: dict[str, Any] = {"schema": SCHEMA, "target": target, "nonce": nonce, "status": "preflight",
                             "stopped_verified": False, "no_session_created": False, "public_status": "not_claimed"}
    requested = False
    intent_epoch = 0
    resource_id = generation = None
    operation: dict[str, Any] | None = None
    code = 1
    guarded_files = [Path(__file__), HERE / "v7_g4_session.py", HERE / "start_and_verify.sh",
                     HERE / "stop_and_verify.sh", HERE / "check_quotas.sh"]
    hashes = {str(path): H.sha(path.read_bytes()) for path in guarded_files}
    state["guard_sources_sha256"] = hashes

    def logged(name: str, argv: list[str], limit: int = 30) -> bytes:
        rc = H.run_logged(receipt, name, argv, limit, env=env)
        require(rc == 0, name + " failed")
        return (receipt / (name + ".stdout")).read_bytes()

    def inspect() -> dict[str, Any] | None:
        command = [gcloud, "compute", "instances", "list", "--project=" + PROJECT,
                   "--zones=" + zone, "--filter=name=" + target["instance"], "--format=json"]
        rc, raw, _ = H.safety_command(command, 30, env)
        require(rc == 0, "exact creation target unreadable")
        rows = json.loads(raw)
        require(isinstance(rows, list) and len(rows) <= 1, "ambiguous exact target inventory")
        return rows[0] if rows else None

    def remember(info: dict[str, Any]) -> bool:
        nonlocal resource_id, generation
        found_id, found_generation = ownership(info, target, nonce, intent_epoch)
        require((resource_id is None or found_id == resource_id) and
                (generation is None or found_generation == generation),
                "resource id or generation changed; no stop authorized")
        resource_id, generation = found_id, found_generation
        state.update(resource_id=resource_id, generation=generation)
        return generation is not None

    def resolve_owned(info: dict[str, Any]) -> dict[str, Any]:
        deadline = time.monotonic() + 60
        while not remember(info):
            require(time.monotonic() < deadline, "owned creation generation not yet readable")
            time.sleep(2)
            info = inspect()
            require(info is not None, "owned creation disappeared before generation certification")
        return info

    def adopt_operation(found: Any) -> dict[str, Any]:
        nonlocal operation
        suffix = "/projects/" + PROJECT + "/zones/" + zone + "/instances/" + target["instance"]
        require(isinstance(found, dict) and found.get("operationType") == "insert" and
                found.get("targetLink", "").endswith(suffix) and
                found.get("zone", "").rsplit("/", 1)[-1] == zone and
                re.fullmatch(r"[a-z][a-z0-9-]+", found.get("name", "")) is not None and
                intent_epoch <= timestamp(found.get("insertTime")) <= time.time() + 300,
                "operation not bound to this exact creation intent")
        require(operation is None or operation["name"] == found["name"], "creation operation changed")
        operation = found
        state["operation"] = found
        return found

    def read_operation() -> dict[str, Any] | None:
        suffix = "/projects/" + PROJECT + "/zones/" + zone + "/instances/" + target["instance"]
        if operation is None:
            command = [gcloud, "compute", "operations", "list", "--project=" + PROJECT, "--zones=" + zone,
                       "--filter=operationType=insert AND targetLink=https://www.googleapis.com/compute/v1" + suffix,
                       "--format=json"]
        else:
            command = [gcloud, "compute", "operations", "describe", operation["name"],
                       "--project=" + PROJECT, "--zone=" + zone, "--format=json"]
        rc, raw, _ = H.safety_command(command, 30, env)
        require(rc == 0, "creation operation unreadable")
        found = json.loads(raw)
        if operation is None:
            require(isinstance(found, list) and len(found) <= 1, "ambiguous creation operation")
            if not found:
                return None
            found = found[0]
        return adopt_operation(found)

    def wait_operation(limit: int) -> dict[str, Any]:
        deadline = time.monotonic() + limit
        while True:
            found = read_operation()
            if found is not None and found.get("status") == "DONE":
                return found
            require(time.monotonic() < deadline, "creation operation not terminal within bounded wait")
            time.sleep(2)

    def interrupted(signum: int, _frame: Any) -> None:
        raise H.SessionInterrupted("creation signal " + str(signum))

    handlers = {sig: signal.signal(sig, interrupted) for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)}
    try:
        H.write_json(receipt / "creation.json", state)
        configured = logged("project", [gcloud, "config", "get-value", "project"]).decode().strip()
        require(configured == PROJECT, "configured project mismatch")
        # Never implicitly install CLI components during an alleged read-only
        # quota check. The operator can explicitly install beta beforehand.
        components = json.loads(logged("components", [gcloud, "components", "list", "--only-local-state", "--format=json"]))
        require(any(row.get("id") == "beta" and isinstance(row.get("current_version_string"), str) and
                    bool(row["current_version_string"]) for row in components),
                "beta component absent; exact RTX quota preflight unavailable")
        expiry = key_expiration(gcloud, ssh_key, env)
        state["ssh_key_expiration"] = expiry
        require(inspect() is None, "random creation target already exists; no mutation authorized")
        machine = json.loads(logged("machine_type", [gcloud, "compute", "machine-types", "describe", "g4-standard-48",
                                                       "--project=" + PROJECT, "--zone=" + zone, "--format=json"]))
        require(machine.get("name") == "g4-standard-48" and machine.get("guestCpus") == 48,
                "G4 type not available in target zone")
        logged("quotas", ["bash", str(HERE / "check_quotas.sh")], 150)
        image = logged("image", [gcloud, "compute", "images", "describe-from-family", IMAGE_FAMILY,
                                  "--project=" + IMAGE_PROJECT, "--format=value(name)"]).decode().strip()
        require(re.fullmatch(r"[a-z][a-z0-9-]+", image) is not None, "image family resolution failed")
        guard = logged("guest_guard_source", ["bash", str(HERE / "start_and_verify.sh"), "--print-guest-guard-script"]).decode().strip()
        intent_epoch = int(time.time())
        cutoff = intent_epoch + 3300  # 300-second reserve relative to the 1h GCE guard.
        startup = startup_script(guard, cutoff, nonce)
        startup_path = receipt / "startup-guard.sh"
        startup_path.write_text(startup)
        startup_path.chmod(0o444)
        state.update(status="create_requested", intent_epoch=intent_epoch, conservative_cutoff=cutoff,
                     startup_sha256=H.sha(startup.encode()), image=image)
        require(all(H.sha(Path(path).read_bytes()) == digest for path, digest in hashes.items()), "guard sources changed before mutation")
        H.write_json(receipt / "creation.json", state)
        command = [gcloud, "compute", "instances", "create", target["instance"], "--project=" + PROJECT,
                   "--zone=" + zone, "--machine-type=g4-standard-48", "--provisioning-model=SPOT",
                   "--instance-termination-action=STOP", "--max-run-duration=3600s", "--maintenance-policy=TERMINATE",
                   "--no-restart-on-failure", "--image=" + image, "--image-project=" + IMAGE_PROJECT,
                   "--boot-disk-size=100GB", "--boot-disk-type=hyperdisk-balanced", "--boot-disk-provisioned-iops=3600",
                   "--boot-disk-provisioned-throughput=290", "--network-interface=network=default,nic-type=GVNIC",
                   "--metadata=enable-oslogin=TRUE", "--metadata-from-file=startup-script=" + str(startup_path),
                   "--labels=project=e-hgp,role=gpu-benchmark,ehgp-create=" + nonce,
                   "--no-service-account", "--no-scopes", "--deletion-protection", "--format=json", "--async", "--quiet"]
        requested = True
        rc = H.run_logged(receipt, "create", command, 180, env=env)
        if rc == 0:
            try:
                response = json.loads((receipt / "create.stdout").read_bytes())
                if isinstance(response, list):
                    require(len(response) == 1, "nonunique insert reply")
                    response = response[0]
                adopt_operation(response)
            except (OSError, ValueError, TypeError, AttributeError) as error:
                state["operation_reply_error"] = str(error)
        # Read the server operation, not the local client's return code, as
        # authority for terminality. The nonce-derived target has one insert.
        completed_operation = wait_operation(180)
        require(rc == 0 and not completed_operation.get("error"), "creation operation failed")
        info = inspect()
        if info is not None:
            info = resolve_owned(info)  # In-memory ownership survives a subsequent journal EIO.
        require(rc == 0 and info is not None, "creation failed; cleanup will inspect the exact owned target")
        # Creation may return before GCE fields materialize. Bounded polling,
        # no workload, and any uncertainty funnels to the owned-target stop.
        deadline = time.monotonic() + 60
        while True:
            try:
                gce_guard(info, cutoff)
                break
            except ValueError:
                if time.monotonic() >= deadline:
                    raise
                time.sleep(2)
                info = inspect()
                require(info is not None, "created target disappeared")
                remember(info)
        state["gce_verified"] = True
        H.write_json(receipt / "creation.json", state)
        common = ["--project=" + PROJECT, "--zone=" + zone, "--ssh-key-file=" + str(ssh_key),
                  "--ssh-key-expiration=" + expiry, "--quiet"]
        verified = False
        deadline = time.monotonic() + 180
        attempt = 0
        while time.monotonic() < deadline:
            attempt += 1
            marker = "/var/lib/ehgp-v7-create/" + nonce
            probe = ("set -euo pipefail; [[ -d /var/lib/ehgp-v7-create && ! -L /var/lib/ehgp-v7-create && "
                     "$(stat -c '%u:%a' /var/lib/ehgp-v7-create) == 0:700 ]]; "
                     "[[ -f " + marker + " && ! -L " + marker + " && $(stat -c '%u:%a' " + marker + ") == 0:600 ]]; "
                     "cat /run/systemd/shutdown/scheduled; printf 'CREATION_MARK='; cat " + marker)
            command = [gcloud, "compute", "ssh", target["instance"], *common,
                       "--command=sudo -n bash -c " + shlex.quote(probe)]
            rc = H.run_logged(receipt, "guest_probe_" + str(attempt), command, 30, env=env)
            if rc == 0:
                guest_guard((receipt / ("guest_probe_" + str(attempt) + ".stdout")).read_text(), cutoff, nonce)
                verified = True
                break
            time.sleep(2)
        require(verified, "guest guard cannot be certified")
        info = inspect()
        require(info is not None, "created target disappeared after guest verification")
        remember(info)
        gce_guard(info, cutoff)
        state.update(status="double_guard_verified_no_workload", guest_verified=True)
        H.write_json(receipt / "creation.json", state)
        code = 0
    except BaseException as error:
        state.update(error_type=type(error).__name__, error=str(error))
        code = 130 if isinstance(error, H.SessionInterrupted) else 1
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        if requested:
            try:
                terminal_operation = None
                try:
                    terminal_operation = wait_operation(60)
                except BaseException as error:
                    state["operation_error"] = str(error)
                info = inspect()
                if info is None:
                    # Empty listing alone never proves cancellation. Only a
                    # unique server-side DONE+error insert excludes late birth.
                    require(terminal_operation is not None and bool(terminal_operation.get("error")),
                            "create requested but target absent; operation terminality unproved")
                    state["no_session_created"] = True
                else:
                    info = resolve_owned(info)
                    # No journal operation precedes this versioned safety stop.
                    stop = ["bash", str(HERE / "stop_and_verify.sh"), "--yes",
                            "--expected-last-start-timestamp", generation]
                    stop_rc, stop_out, stop_err = H.safety_command(stop, 420, env)
                    try:
                        (receipt / "stop.stdout").write_bytes(stop_out)
                        (receipt / "stop.stderr").write_bytes(stop_err)
                        H.write_json(receipt / "stop.json", {"argv": stop, "exit_code": stop_rc})
                    except OSError as error:
                        state["receipt_error"] = str(error)
                        code = 1
                    after = inspect()
                    require(after is not None, "post-stop owned target unreadable")
                    remember(after)
                    require(stop_rc == 0 and after.get("status") == "TERMINATED", "owned generation stop unverified")
                    require(terminal_operation is not None, "target stopped but insert terminality remains unproved")
                    state["stopped_verified"] = True
                    try:
                        H.write_json(receipt / "post_stop.json", after)
                    except OSError as error:
                        state["receipt_error"] = str(error)
                        code = 1
            except BaseException as error:
                state["shutdown_error"] = str(error)
                state["control_command"] = shlex.join([gcloud, "compute", "instances", "describe", target["instance"],
                                                       "--project=" + PROJECT, "--zone=" + zone,
                                                       "--format=value(id,status,lastStartTimestamp)"])
                code = 74
        state.update(status="completed_stopped" if code == 0 else "blocked" if code == 74 else "failed",
                     exit_code=code, ended=H.utc(), disks_retained=state["stopped_verified"])
        try:
            H.write_json(receipt / "creation.json", state)
        except OSError as error:
            state["receipt_error"] = str(error)
            code = 74 if requested and not (state["stopped_verified"] or state["no_session_created"]) else 1
            state.update(exit_code=code, status="blocked" if code == 74 else "failed")
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    print(json.dumps(state, sort_keys=True))
    return code


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--zone", required=True)
    parser.add_argument("--receipt-dir", type=Path, required=True)
    parser.add_argument("--ssh-key", type=Path, required=True)
    parser.add_argument("--yes", action="store_true")
    args = parser.parse_args()
    require(args.yes, "explicit --yes required; creates a billable SPOT VM")
    return create(args.zone, args.receipt_dir, args.ssh_key)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("REFUS: " + str(error), file=sys.stderr)
        raise SystemExit(2)
