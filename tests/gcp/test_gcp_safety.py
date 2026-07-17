"""Regression tests for fail-closed GCP session guards."""

from __future__ import annotations

import importlib.util
import json
import os
import re
import signal
import shutil
import subprocess
import tempfile
import time
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FAKE_GCLOUD = r"""#!/usr/bin/env python3
import json
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
elif args[:4] == ["compute", "os-login", "ssh-keys", "add"]:
    if scenario == "qualification-oslogin-import-failure":
        print("simulated OS Login import failure", file=sys.stderr)
        sys.exit(41)
elif args[:4] == ["compute", "os-login", "ssh-keys", "remove"]:
    pass
elif args[:3] == ["compute", "os-login", "describe-profile"]:
    key_file = os.environ.get("GCP_SSH_KEY_FILE", "")
    public_path = Path(key_file + ".pub")
    public_key = (
        public_path.read_text(encoding="utf-8").strip()
        if key_file and public_path.is_file()
        else ""
    )
    default_remaining = (
        "4200"
        if scenario.startswith("qualification-")
        else str(int(os.environ.get("FAKE_MAX_RUN_SECONDS", "28800")) + 600)
    )
    remaining = int(
        os.environ.get("FAKE_OSLOGIN_REMAINING_SECONDS", default_remaining)
    )
    record = {"key": public_key}
    if scenario != "oslogin-key-unbounded":
        record["expirationTimeUsec"] = str(
            int(time.time() * 1_000_000) + remaining * 1_000_000
        )
    keys = {} if scenario == "oslogin-key-missing" else {"fake-key": record}
    print(json.dumps({"sshPublicKeys": keys}))
elif args[:4] == ["beta", "quotas", "info", "describe"]:
    quota_id = args[4]
    region = os.environ.get("GCP_REGION", "europe-west4")
    zone = os.environ.get("GCP_ZONE", "europe-west4-a")
    quota_zone = zone
    if scenario == "quota-ai-parent" and quota_id in {
        "HDB-TOTAL-IOPS-per-project-zone",
        "HDB-TOTAL-THROUGHPUT-per-project-zone",
    }:
        quota_zone = f"{region}-a"
    elif scenario == "quota-ai-parent-missing" and quota_id in {
        "HDB-TOTAL-IOPS-per-project-zone",
        "HDB-TOTAL-THROUGHPUT-per-project-zone",
    }:
        quota_zone = f"{region}-c"
    limits = {
        "GPUS-ALL-REGIONS-per-project": ("global", "1"),
        "PREEMPTIBLE-NVIDIA-RTX-PRO-6000-GPUS-per-project-region": (region, "1"),
        "HDB-TOTAL-GB-per-project-region": (region, "500"),
        "HDB-TOTAL-IOPS-per-project-zone": (quota_zone, "20000"),
        "HDB-TOTAL-THROUGHPUT-per-project-zone": (quota_zone, "2000"),
        "INSTANCES-per-project-region": (region, "24"),
        "IN-USE-ADDRESSES-per-project-region": (region, "8"),
        "PREEMPTIBLE-CPUS-per-project-region": (region, "48"),
    }
    if quota_id not in limits:
        raise SystemExit("unsupported fake quota: " + quota_id)
    if scenario == "quota-missing-cpu" and quota_id == "PREEMPTIBLE-CPUS-per-project-region":
        raise SystemExit(7)
    location, limit = limits[quota_id]
    if scenario == "quota-unlimited-performance" and quota_id in {
        "HDB-TOTAL-IOPS-per-project-zone",
        "HDB-TOTAL-THROUGHPUT-per-project-zone",
    }:
        limit = "-1"
    if (
        scenario == "quota-missing-rtx"
        and quota_id
        == "PREEMPTIBLE-NVIDIA-RTX-PRO-6000-GPUS-per-project-region"
    ):
        dimensions_infos = []
    elif quota_id == "GPUS-ALL-REGIONS-per-project":
        dimensions_infos = [
            {"applicableLocations": [location], "details": {"value": limit}}
        ]
    elif quota_id in {
        "HDB-TOTAL-IOPS-per-project-zone",
        "HDB-TOTAL-THROUGHPUT-per-project-zone",
    }:
        dimensions_infos = [
            {"applicableLocations": [location], "details": {"value": limit}}
        ]
    else:
        dimension = "region"
        dimensions_infos = [
            {
                "applicableLocations": [location],
                "details": {"value": limit},
                "dimensions": {dimension: location},
            }
        ]
    print(json.dumps({"dimensionsInfos": dimensions_infos}))
elif args[:3] == ["compute", "project-info", "describe"]:
    print(
        json.dumps(
            {"quotas": [{"metric": "GPUS_ALL_REGIONS", "limit": 1, "usage": 0}]}
        )
    )
elif args[:3] == ["compute", "regions", "describe"]:
    print(
        json.dumps(
            {
                "quotas": [
                    {"metric": "PREEMPTIBLE_CPUS", "limit": 48, "usage": 0},
                    {"metric": "INSTANCES", "limit": 24, "usage": 2},
                    {"metric": "IN_USE_ADDRESSES", "limit": 8, "usage": 0},
                ]
            }
        )
    )
elif args[:3] == ["compute", "disks", "list"]:
    region = os.environ.get("GCP_REGION", "europe-west4")
    zone = os.environ.get("GCP_ZONE", "europe-west4-a")
    if scenario in {"quota-ai-explicit", "quota-ai-parent"}:
        disks = [
            {
                "name": "ai-balanced",
                "provisionedIops": "3600",
                "provisionedThroughput": "290",
                "sizeGb": "100",
                "type": f"zones/{zone}/diskTypes/hyperdisk-balanced",
                "zone": f"zones/{zone}",
            },
            {
                "name": "parent-balanced",
                "provisionedIops": "4000",
                "provisionedThroughput": "200",
                "sizeGb": "50",
                "type": (
                    f"zones/{region}-a/diskTypes/hyperdisk-balanced"
                ),
                "zone": f"zones/{region}-a",
            },
            {
                "name": "sibling-ai-balanced",
                "provisionedIops": "3500",
                "provisionedThroughput": "150",
                "sizeGb": "50",
                "type": (
                    f"zones/{region}-ai2a/diskTypes/hyperdisk-balanced"
                ),
                "zone": f"zones/{region}-ai2a",
            },
            {
                "name": "different-standard-zone",
                "provisionedIops": "9000",
                "provisionedThroughput": "500",
                "sizeGb": "10",
                "type": f"zones/{region}-b/diskTypes/hyperdisk-balanced",
                "zone": f"zones/{region}-b",
            },
            {
                "name": "different-ai-parent",
                "provisionedIops": "9000",
                "provisionedThroughput": "500",
                "sizeGb": "10",
                "type": (
                    f"zones/{region}-ai1b/diskTypes/hyperdisk-balanced"
                ),
                "zone": f"zones/{region}-ai1b",
            },
        ]
    else:
        disks = [
            {
                "name": "zonal-balanced",
                "provisionedIops": "3600",
                "provisionedThroughput": "290",
                "sizeGb": "100",
                "type": f"zones/{zone}/diskTypes/hyperdisk-balanced",
                "zone": f"zones/{zone}",
            },
            {
                "name": "regional-balanced-ha",
                "provisionedIops": "4000",
                "provisionedThroughput": "200",
                "region": f"regions/{region}",
                "replicaZones": [f"zones/{zone}", f"zones/{region}-b"],
                "sizeGb": "50",
                "type": (
                    f"regions/{region}/diskTypes/"
                    "hyperdisk-balanced-high-availability"
                ),
            },
        ]
    print(
        json.dumps(disks)
    )
elif args[:3] == ["compute", "instances", "list"]:
    output_format = next((item.split("=", 1)[1] for item in args if item.startswith("--format=")), "")
    if output_format == "value(name)":
        if scenario != "missing-target":
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
    session_commands = ""
    session_log = os.environ.get("FAKE_SESSION_LOG")
    if session_log and os.path.exists(session_log):
        with open(session_log, encoding="utf-8") as stream:
            session_commands = stream.read()
    qualification_start_observed = "start project=" in session_commands
    qualification_stop_observed = "stop project=" in session_commands
    if scenario == "qualification-stop-unreadable" and qualification_stop_observed:
        print("simulated unreadable target", file=sys.stderr)
        sys.exit(98)
    status = "TERMINATED"
    if (
        scenario.startswith("qualification-")
        and qualification_start_observed
        and not qualification_stop_observed
    ):
        status = os.environ.get("FAKE_QUALIFICATION_POST_START_STATUS", "RUNNING")
    if scenario == "qualification-stop-still-running" and qualification_start_observed:
        status = "RUNNING"
    start_observed = "compute instances start" in previous_commands
    post_start_commands = (
        previous_commands.rsplit("compute instances start", 1)[-1]
        if start_observed
        else ""
    )
    if scenario in {
        "generation-race",
        "guest-interrupt",
        "guest-readback",
        "guest-publickey-denied",
        "guest-success",
        "guest-success-late-termination-timestamp",
        "guest-success-no-term-duration-race",
        "guest-success-no-termination-timestamp",
        "post-start-wrong-maintenance",
        "post-start-wrong-provisioning",
        "signal-after-capture",
        "timestamp-lag",
    } and start_observed:
        status = "RUNNING"
    if scenario == "generation-race" and start_observed:
        status_reads = post_start_commands.count("--format=value(status)")
        status = "PROVISIONING" if status_reads == 1 else "RUNNING"
    if scenario == "signal-after-capture" and start_observed:
        status = "PROVISIONING"
    if "compute instances stop" in previous_commands:
        status = "TERMINATED"
    now = datetime.now(timezone.utc)
    if start_observed or (
        scenario.startswith("qualification-") and qualification_start_observed
    ):
        last_start_timestamp = os.environ.get(
            "FAKE_LAST_START_TIMESTAMP",
            (now - timedelta(seconds=10)).isoformat().replace("+00:00", "Z"),
        )
        if scenario == "generation-race":
            generation_reads = post_start_commands.count(
                "--format=value(lastStartTimestamp)"
            )
            if generation_reads >= 2:
                last_start_timestamp = os.environ["FAKE_CONCURRENT_START_TIMESTAMP"]
        if scenario == "timestamp-lag":
            generation_reads = post_start_commands.count(
                "--format=value(lastStartTimestamp)"
            )
            if generation_reads == 1:
                last_start_timestamp = os.environ["FAKE_PRE_START_TIMESTAMP"]
    else:
        last_start_timestamp = os.environ.get(
            "FAKE_PRE_START_TIMESTAMP",
            (now - timedelta(seconds=20)).isoformat().replace("+00:00", "Z"),
        )
    max_run_seconds = os.environ.get("FAKE_MAX_RUN_SECONDS", "28800")
    if (
        scenario == "qualification-post-provision-recertification-failure"
        and output_format == "json"
        and "phase3_remote_docker_provision.sh" in previous_commands
        and "phase3_remote_qualification.sh" not in previous_commands
    ):
        max_run_seconds = "7200"
    if scenario == "guest-success-no-term-duration-race":
        max_run_seconds = "7200" if start_observed else "3600"
    termination_timestamp = os.environ.get(
        "FAKE_TERMINATION_TIMESTAMP",
        (now + timedelta(seconds=28790)).isoformat().replace("+00:00", "Z"),
    )
    if scenario in {
        "guest-success-no-term-duration-race",
        "guest-success-no-termination-timestamp",
    }:
        termination_timestamp = ""
    if scenario == "guest-success-late-termination-timestamp":
        termination_reads = post_start_commands.count(
            "--format=value(terminationTimestamp)"
        )
        if termination_reads == 1:
            termination_timestamp = ""
        else:
            parsed_start = datetime.fromisoformat(
                last_start_timestamp.replace("Z", "+00:00")
            )
            termination_timestamp = (
                parsed_start + timedelta(seconds=int(max_run_seconds) - 300)
            ).isoformat().replace("+00:00", "Z")
    values = {
        "status": status,
        "scheduling.instanceTerminationAction": "STOP",
        "scheduling.automaticRestart": "False",
        "scheduling.maxRunDuration.seconds": max_run_seconds,
        "scheduling.onHostMaintenance": (
            "MIGRATE"
            if scenario == "post-start-wrong-maintenance" and start_observed
            else "TERMINATE"
        ),
        "labels.project": "e-hgp",
        "machineType.basename()": "n2-standard-48" if scenario == "wrong-machine" else "g4-standard-48",
        "scheduling.provisioningModel": (
            "STANDARD"
            if scenario == "wrong-provisioning"
            or (scenario == "post-start-wrong-provisioning" and start_observed)
            else "SPOT"
        ),
        "lastStartTimestamp": last_start_timestamp,
        "terminationTimestamp": termination_timestamp,
    }
    if scenario == "unlabelled":
        values["labels.project"] = ""
    if output_format == "json":
        print(
            json.dumps(
                {
                    "lastStartTimestamp": last_start_timestamp,
                    "scheduling": {
                        "maxRunDuration": {"seconds": max_run_seconds}
                    },
                    "status": status,
                    "terminationTimestamp": termination_timestamp,
                }
            )
        )
    else:
        print(values.get(field, ""))
elif args[:3] in (["compute", "instances", "start"], ["compute", "instances", "stop"]):
    if scenario not in {
        "guest-interrupt",
        "guest-readback",
        "guest-publickey-denied",
        "guest-success",
        "guest-success-late-termination-timestamp",
        "guest-success-no-term-duration-race",
        "guest-success-no-termination-timestamp",
        "generation-race",
        "immediate-preemption",
        "post-start-wrong-maintenance",
        "post-start-wrong-provisioning",
        "signal-after-capture",
        "timestamp-lag",
    }:
        print("unexpected mutation", file=sys.stderr)
        sys.exit(97)
elif args[:2] == ["compute", "ssh"]:
    command = next(
        (item.split("=", 1)[1] for item in args if item.startswith("--command=")),
        "",
    )
    if scenario == "guest-publickey-denied":
        print("tester@203.0.113.10: Permission denied (publickey).", file=sys.stderr)
        sys.exit(255)
    elif scenario == "guest-readback":
        print("No scheduled shutdown")
    elif scenario == "guest-interrupt":
        print("No scheduled shutdown")
    elif scenario in {
        "guest-success",
        "guest-success-late-termination-timestamp",
        "guest-success-no-termination-timestamp",
        "timestamp-lag",
    }:
        print("MODE=poweroff\nUSEC=9999999999999999\n__EHGP_GUEST_GUARD_VERIFIED__")
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
        elif "phase3_remote_docker_provision.sh" in command:
            if scenario == "qualification-provision-failure":
                print("simulated Docker provisioning failure", file=sys.stderr)
                sys.exit(43)
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
    head = os.environ.get("FAKE_GIT_HEAD", "a" * 40)
    artifact = {
        "backend": "cuda_g4",
        "git": {"clean": True, "sha": head},
        "image": {
            "base_ref": "nvidia/cuda:12.9.2-devel-ubuntu24.04@sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048",
            "id": "sha256:" + "b" * 64,
            "ref": f"morsehgp3d-phase3:{head}",
        },
        "mode": "certified",
        "phase": "3",
        "profile": "hgp_reduced",
        "schema": "morsehgp3d.phase3.qualification.v1",
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": "environment_reproducibility_only",
        "status": "worker_passed_pending_shutdown",
        "vm_lifecycle": {"worker_mutates_gcp": False},
    }
    if scenario == "qualification-invalid-artifact":
        artifact["git"]["sha"] = "c" * 40
    Path(args[3]).write_text(json.dumps(artifact) + "\n", encoding="utf-8")
else:
    print("unsupported fake gcloud command: " + " ".join(args), file=sys.stderr)
    sys.exit(96)
"""

FAKE_TIMEOUT = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

args = sys.argv[1:]
log = os.environ.get("FAKE_TIMEOUT_LOG")
if log:
    with Path(log).open("a", encoding="utf-8") as stream:
        stream.write(" ".join(args) + "\n")

scenario = os.environ.get("FAKE_TIMEOUT_SCENARIO", "pass")
if args == ["--version"]:
    if scenario == "invalid-version":
        print("timeout (BusyBox) 1.0")
    else:
        print("timeout (GNU coreutils) 9.4")
    raise SystemExit(0)

if args and args[0] == "--foreground":
    raise SystemExit("fake timeout rejects --foreground")
if not args or not args[0].startswith("--kill-after="):
    raise SystemExit("fake timeout requires --kill-after")
args = args[1:]
if not args or not args[0].endswith("s"):
    raise SystemExit("fake timeout requires a seconds duration")
args = args[1:]
if not args:
    raise SystemExit("fake timeout requires a command")
if scenario == "expire-start" and args[:4] == ["gcloud", "compute", "instances", "start"]:
    raise SystemExit(124)
if scenario == "expire-stop" and args[:4] == ["gcloud", "compute", "instances", "stop"]:
    raise SystemExit(124)
os.execvpe(args[0], args, os.environ)
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
printf 'start project=%s zone=%s instance=%s args=%s ssh_key=%s\n' \
  "${GCP_PROJECT_ID:-}" "${GCP_ZONE:-}" "${GCP_INSTANCE_NAME:-}" "$*" \
  "${GCP_SSH_KEY_FILE:-}" \
  >> "${FAKE_SESSION_LOG}"
if [[ "${FAKE_SESSION_SCENARIO:-}" == "start-failure" ]]; then
  exit 37
fi
handoff_file=""
arguments=("$@")
for ((index = 0; index < ${#arguments[@]}; ++index)); do
  if [[ "${arguments[index]}" == "--handoff-file" ]]; then
    handoff_file="${arguments[index + 1]}"
  fi
done
[[ -n "${handoff_file}" ]]
handoff_status="targeted_running"
if [[ "${FAKE_SESSION_SCENARIO:-}" == "preemption-handoff" ]]; then
  handoff_status="targeted_stopping"
fi
printf '{"guest_shutdown_minutes":45,"instance":"%s","last_start_timestamp":"%s","project":"%s","schema":"e-hgp.start-handoff.v3","status":"%s","zone":"%s"}\n' \
  "${GCP_INSTANCE_NAME}" "${FAKE_LAST_START_TIMESTAMP}" "${GCP_PROJECT_ID}" \
  "${handoff_status}" "${GCP_ZONE}" >"${handoff_file}"
if [[ "${FAKE_SESSION_SCENARIO:-}" == "guard-failure-stop-failure" ]]; then
  exit 37
elif [[ "${FAKE_SESSION_SCENARIO:-}" == "preemption-handoff" ]]; then
  exit 37
elif [[ "${FAKE_SESSION_SCENARIO:-}" == "start-race" ]]; then
  kill -TERM "${PPID}"
elif [[ "${FAKE_SESSION_SCENARIO:-}" == "invalid-handoff" ]]; then
  printf '{}\n' >"${handoff_file}"
elif [[ "${FAKE_SESSION_SCENARIO:-}" == "missing-handoff" ]]; then
  rm -f -- "${handoff_file}"
fi
"""

FAKE_STOP = r"""#!/usr/bin/env bash
set -euo pipefail
printf 'stop project=%s zone=%s instance=%s args=%s\n' \
  "${GCP_PROJECT_ID:-}" "${GCP_ZONE:-}" "${GCP_INSTANCE_NAME:-}" "$*" \
  >> "${FAKE_SESSION_LOG}"
if [[ "${FAKE_SESSION_SCENARIO:-}" == "stop-failure" || \
      "${FAKE_SESSION_SCENARIO:-}" == "guard-failure-stop-failure" ]]; then
  exit 38
elif [[ "${FAKE_SESSION_SCENARIO:-}" == "target-race" ]]; then
  printf 'concurrent artifact\n' >"${FAKE_CONCURRENT_RESULT}"
fi
"""


class ScriptSafetyTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tempdir.cleanup)
        self.tmp = Path(self.tempdir.name)
        self.ssh_key = self.tmp / "phase3-session-ed25519"
        subprocess.run(
            [
                "ssh-keygen",
                "-q",
                "-t",
                "ed25519",
                "-N",
                "",
                "-C",
                "phase3-test-session",
                "-f",
                str(self.ssh_key),
            ],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
        )
        self.ssh_key.chmod(0o600)
        fake = self.tmp / "gcloud"
        fake.write_text(FAKE_GCLOUD, encoding="utf-8")
        fake.chmod(0o755)
        fake_timeout = self.tmp / "timeout"
        fake_timeout.write_text(FAKE_TIMEOUT, encoding="utf-8")
        fake_timeout.chmod(0o755)
        self.log = self.tmp / "gcloud.log"
        self.timeout_log = self.tmp / "timeout.log"
        now = datetime.now(timezone.utc)
        self.pre_start_timestamp = (
            (now - timedelta(seconds=20)).isoformat().replace("+00:00", "Z")
        )
        self.last_start_timestamp = (
            (now - timedelta(seconds=10)).isoformat().replace("+00:00", "Z")
        )
        self.concurrent_start_timestamp = (
            (now - timedelta(seconds=5)).isoformat().replace("+00:00", "Z")
        )
        self.env = os.environ.copy()
        self.env.update(
            {
                "FAKE_LAST_START_TIMESTAMP": self.last_start_timestamp,
                "FAKE_PRE_START_TIMESTAMP": self.pre_start_timestamp,
                "FAKE_CONCURRENT_START_TIMESTAMP": self.concurrent_start_timestamp,
                "PATH": f"{self.tmp}:{self.env['PATH']}",
                "FAKE_GCLOUD_LOG": str(self.log),
                "FAKE_TIMEOUT_LOG": str(self.timeout_log),
                "GCP_PROJECT_ID": "devpod-gpu-exploration",
                "GCP_ZONE": "europe-west4-a",
                "GCP_INSTANCE_NAME": "ehgp-blackwell-spot",
                "GCP_SSH_KEY_FILE": str(self.ssh_key),
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

    def test_critical_gcloud_calls_use_bounded_gnu_timeout(self) -> None:
        for name in (
            "start_and_verify.sh",
            "stop_and_verify.sh",
            "run_phase3_qualification.sh",
        ):
            with self.subTest(script=name):
                source = (ROOT / "gcp-migration" / name).read_text(encoding="utf-8")
                self.assertIn("command -v timeout", source)
                self.assertIn("timeout --kill-after=", source)
                self.assertNotIn("timeout --foreground", source)
        start_source = (ROOT / "gcp-migration" / "start_and_verify.sh").read_text(
            encoding="utf-8"
        )
        stop_source = (ROOT / "gcp-migration" / "stop_and_verify.sh").read_text(
            encoding="utf-8"
        )
        self.assertIn("gcloud_mutation compute instances start", start_source)
        self.assertIn("gcloud_mutation compute instances stop", stop_source)
        self.assertIn("aucun arrêt automatique non versionné", start_source)
        self.assertNotIn("fallback d’arrêt ciblé non versionné", start_source)

    def test_gnu_timeout_kills_a_forking_gcloud_descendant(self) -> None:
        timeout_bin = shutil.which("timeout")
        self.assertIsNotNone(timeout_bin)
        version = subprocess.run(
            [timeout_bin, "--version"],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        self.assertEqual(version.returncode, 0, version.stdout)
        self.assertIn("GNU coreutils", version.stdout)

        descendant_pid_file = self.tmp / "forking-gcloud-descendant.pid"
        forking_gcloud = self.tmp / "forking-gcloud"
        forking_gcloud.write_text(
            r"""#!/usr/bin/env python3
import os
from pathlib import Path
import signal
import time

signal.signal(signal.SIGTERM, signal.SIG_IGN)
child = os.fork()
if child == 0:
    signal.signal(signal.SIGTERM, signal.SIG_DFL)
    Path(os.environ["FAKE_DESCENDANT_PID_FILE"]).write_text(
        str(os.getpid()), encoding="utf-8"
    )
    while True:
        time.sleep(60)
while True:
    time.sleep(60)
""",
            encoding="utf-8",
        )
        forking_gcloud.chmod(0o755)
        environment = os.environ.copy()
        environment["FAKE_DESCENDANT_PID_FILE"] = str(descendant_pid_file)

        result = subprocess.run(
            [
                timeout_bin,
                "--kill-after=0.2s",
                "1s",
                str(forking_gcloud),
            ],
            env=environment,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
            timeout=5,
        )
        self.assertIn(result.returncode, {124, -signal.SIGKILL}, result.stdout)
        self.assertTrue(descendant_pid_file.is_file())
        descendant_pid = int(descendant_pid_file.read_text(encoding="utf-8"))

        def process_is_live(pid: int) -> bool:
            try:
                stat = Path(f"/proc/{pid}/stat").read_text(encoding="utf-8")
            except (FileNotFoundError, ProcessLookupError):
                return False
            state = stat.rsplit(")", 1)[1].strip().split(maxsplit=1)[0]
            return state not in {"X", "Z"}

        try:
            deadline = time.monotonic() + 2
            while process_is_live(descendant_pid) and time.monotonic() < deadline:
                time.sleep(0.02)
            self.assertFalse(
                process_is_live(descendant_pid),
                f"le descendant gcloud {descendant_pid} a survécu au timeout",
            )
        finally:
            if process_is_live(descendant_pid):
                try:
                    os.kill(descendant_pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass

    def test_non_gnu_timeout_is_rejected_before_any_gcloud_call(self) -> None:
        self.env["FAKE_TIMEOUT_SCENARIO"] = "invalid-version"
        result = self.run_script("start_and_verify.sh", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("implémentation GNU", result.stdout)
        self.assertEqual("", self.commands())

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

    def test_start_rechecks_spot_after_mutation_and_stops_exact_generation(
        self,
    ) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "post-start-wrong-provisioning"
        handoff = self.tmp / "post-start-failure-handoff.json"

        result = self.run_script(
            "start_and_verify.sh",
            "--yes",
            "--handoff-file",
            str(handoff),
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("post-démarrage", result.stdout)
        self.assertIn("compute instances start", self.commands())
        self.assertIn("compute instances stop", self.commands())
        self.assertIn(self.last_start_timestamp, result.stdout)
        self.assertFalse(handoff.exists())

    def test_start_rechecks_terminate_maintenance_after_mutation(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "post-start-wrong-maintenance"

        result = self.run_script("start_and_verify.sh", "--yes")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("post-démarrage", result.stdout)
        self.assertIn("compute instances start", self.commands())
        self.assertIn("compute instances stop", self.commands())

    def test_post_start_guard_failure_preserves_targeted_stopping_handoff(
        self,
    ) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "post-start-wrong-provisioning"
        self.env["FAKE_TIMEOUT_SCENARIO"] = "expire-stop"
        handoff = self.tmp / "post-start-incident-handoff.json"

        result = self.run_script(
            "start_and_verify.sh",
            "--yes",
            "--handoff-file",
            str(handoff),
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("[HANDOFF CONSERVÉ]", result.stdout)
        self.assertEqual(
            "targeted_stopping",
            json.loads(handoff.read_text(encoding="utf-8"))["status"],
        )
        self.assertIn(
            "gcloud compute instances stop ehgp-blackwell-spot",
            self.timeout_log.read_text(encoding="utf-8"),
        )

    def test_signal_after_generation_capture_preserves_handoff_if_stop_fails(
        self,
    ) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "signal-after-capture"
        self.env["FAKE_TIMEOUT_SCENARIO"] = "expire-stop"
        handoff = self.tmp / "signal-window-handoff.json"
        fake_sleep = self.tmp / "sleep"
        fake_sleep.write_text(
            '#!/usr/bin/env bash\nkill -TERM "${PPID}"\nexit 143\n',
            encoding="utf-8",
        )
        fake_sleep.chmod(0o755)

        result = self.run_script(
            "start_and_verify.sh",
            "--yes",
            "--handoff-file",
            str(handoff),
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("[HANDOFF CONSERVÉ]", result.stdout)
        value = json.loads(handoff.read_text(encoding="utf-8"))
        self.assertEqual("targeted_stopping", value["status"])
        self.assertEqual(self.last_start_timestamp, value["last_start_timestamp"])
        self.assertIn(
            "gcloud compute instances stop ehgp-blackwell-spot",
            self.timeout_log.read_text(encoding="utf-8"),
        )

    def test_immediate_spot_preemption_is_detected_and_certified_without_waiting(
        self,
    ) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "immediate-preemption"
        handoff = self.tmp / "preemption-handoff.json"

        result = self.run_script(
            "start_and_verify.sh",
            "--yes",
            "--handoff-file",
            str(handoff),
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("préemptée pendant le démarrage", result.stdout)
        self.assertIn("Arrêt ciblé certifié", result.stdout)
        self.assertIn(self.last_start_timestamp, result.stdout)
        self.assertIn("compute instances start", self.commands())
        self.assertNotIn("compute instances stop", self.commands())
        self.assertFalse(handoff.exists())

    def test_generation_latch_never_stops_a_concurrent_restart(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "generation-race"
        handoff = self.tmp / "generation-race-handoff.json"
        fake_sleep = self.tmp / "sleep"
        fake_sleep.write_text("#!/usr/bin/env bash\nexit 0\n", encoding="utf-8")
        fake_sleep.chmod(0o755)

        result = self.run_script(
            "start_and_verify.sh",
            "--yes",
            "--handoff-file",
            str(handoff),
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("génération concurrente", result.stdout)
        self.assertIn("Génération différente", result.stdout)
        self.assertNotIn("compute instances stop", self.commands())
        value = json.loads(handoff.read_text(encoding="utf-8"))
        self.assertEqual("targeted_stopping", value["status"])
        self.assertEqual(self.last_start_timestamp, value["last_start_timestamp"])

    def test_running_retries_until_start_generation_is_materialized(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "timestamp-lag"
        handoff = self.tmp / "timestamp-lag-handoff.json"
        fake_sleep = self.tmp / "sleep"
        fake_sleep.write_text("#!/usr/bin/env bash\nexit 0\n", encoding="utf-8")
        fake_sleep.chmod(0o755)

        result = self.run_script(
            "start_and_verify.sh",
            "--yes",
            "--guest-shutdown-minutes",
            "45",
            "--handoff-file",
            str(handoff),
        )

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("non encore matérialisée", result.stdout)
        self.assertEqual(
            self.last_start_timestamp,
            json.loads(handoff.read_text(encoding="utf-8"))["last_start_timestamp"],
        )
        stopped = self.run_script(
            "stop_and_verify.sh",
            "--yes",
            "--expected-last-start-timestamp",
            self.last_start_timestamp,
        )
        self.assertEqual(stopped.returncode, 0, stopped.stdout)

    def test_deploy_declares_and_reads_back_spot_invariants(self) -> None:
        source = (ROOT / "gcp-migration" / "deploy.sh").read_text(encoding="utf-8")

        for expected in (
            '--provisioning-model="SPOT"',
            '--maintenance-policy="TERMINATE"',
            "--no-restart-on-failure",
            "readonly BOOT_DISK_IOPS=3600",
            "readonly BOOT_DISK_THROUGHPUT=290",
            'NETWORK_INTERFACE="${GCP_NETWORK_INTERFACE:-network=default,nic-type=GVNIC}"',
            '--boot-disk-provisioned-iops="${BOOT_DISK_IOPS}"',
            '--boot-disk-provisioned-throughput="${BOOT_DISK_THROUGHPUT}"',
            "scheduling.provisioningModel",
            "scheduling.onHostMaintenance",
            "scheduling.automaticRestart",
            "machineType.basename()",
            "labels.project",
            "ai-zones-visibility",
        ):
            with self.subTest(expected=expected):
                self.assertIn(expected, source)
        self.assertIn("check_quotas.sh", source)
        self.assertIn('[[ "${NETWORK_INTERFACE}" != *"no-address"* ]]', source)
        self.assertIn('[[ "${ZONE}" == *-ai* ]] || return 1', source)
        self.assertIn("CREATE_REQUEST_EPOCH - TIMESTAMP_TOLERANCE_SECONDS", source)
        self.assertIn("now + configured_seconds + TIMESTAMP_TOLERANCE_SECONDS", source)
        self.assertNotIn("access-config-type", source)

    def test_quota_check_uses_exact_spot_quota_and_never_requires_g4_cpus(
        self,
    ) -> None:
        source = (ROOT / "gcp-migration" / "check_quotas.sh").read_text(
            encoding="utf-8"
        )

        self.assertIn(
            "PREEMPTIBLE-NVIDIA-RTX-PRO-6000-GPUS-per-project-region",
            source,
        )
        self.assertIn("GPUS-ALL-REGIONS-per-project", source)
        self.assertIn("HDB-TOTAL-GB-per-project-region", source)
        self.assertIn("HDB-TOTAL-IOPS-per-project-zone", source)
        self.assertIn("HDB-TOTAL-THROUGHPUT-per-project-zone", source)
        self.assertIn("readonly REQUIRED_HDB_IOPS=600", source)
        self.assertIn("readonly REQUIRED_HDB_THROUGHPUT=150", source)
        self.assertIn("readonly HDB_BASELINE_IOPS=3000", source)
        self.assertIn("readonly HDB_BASELINE_THROUGHPUT=140", source)
        self.assertIn("PREEMPTIBLE_CPUS (informatif)", source)
        self.assertNotIn(
            'quota_is_sufficient "${CPUS_SPOT_AVAILABLE}"',
            source,
        )
        self.assertNotIn("GPU-FAMILY:NVIDIA_RTX_PRO_6000", source)

    def test_quota_check_executes_exact_limits_and_hdb_accounting(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "quota-success"

        result = self.run_script("check_quotas.sh")

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertRegex(
            result.stdout,
            r"HDB_TOTAL_GB\s+500\s+200\s+300\s+>= 100",
        )
        self.assertRegex(
            result.stdout,
            r"HDB_TOTAL_IOPS \(europe-west4-a\)\s+20000\s+1600\s+18400\s+>= 600",
        )
        self.assertRegex(
            result.stdout,
            r"HDB_TOTAL_THROUGHPUT \(europe-west4-a\)\s+2000\s+210\s+1790\s+>= 150",
        )
        self.assertIn("PREEMPTIBLE_NVIDIA_RTX_PRO_6000", result.stdout)
        self.assertIn("[SUCCÈS]", result.stdout)

    def test_ai_zone_uses_parent_hdb_quota_and_combines_usage(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "quota-ai-parent"
        self.env["GCP_ZONE"] = "europe-west4-ai1a"

        result = self.run_script("check_quotas.sh")

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertRegex(
            result.stdout,
            r"HDB_TOTAL_GB\s+500\s+220\s+280\s+>= 100",
        )
        self.assertRegex(
            result.stdout,
            r"HDB_TOTAL_IOPS \(europe-west4-a\)\s+20000\s+2100\s+17900\s+>= 600",
        )
        self.assertRegex(
            result.stdout,
            r"HDB_TOTAL_THROUGHPUT \(europe-west4-a\)\s+2000\s+220\s+1780\s+>= 150",
        )
        self.assertIn(
            "Zone IA europe-west4-ai1a : limite Hyperdisk dérivée de la zone "
            "parente europe-west4-a",
            result.stdout,
        )
        self.assertIn(
            "Cloud Quotas ne publie pas de dimension IA; la création GCE reste "
            "l’arbitre",
            result.stdout,
        )
        self.assertIn(
            "Le calcul additionne conservativement le parent et ses zones IA "
            "associées",
            result.stdout,
        )

    def test_explicit_ai_hdb_quota_takes_precedence_over_parent(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "quota-ai-explicit"
        self.env["GCP_ZONE"] = "europe-west4-ai1a"

        result = self.run_script("check_quotas.sh")

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertRegex(
            result.stdout,
            r"HDB_TOTAL_IOPS \(europe-west4-ai1a\)\s+20000\s+600\s+19400\s+>= 600",
        )
        self.assertRegex(
            result.stdout,
            r"HDB_TOTAL_THROUGHPUT \(europe-west4-ai1a\)\s+2000\s+150\s+1850\s+>= 150",
        )
        self.assertNotIn("limite Hyperdisk dérivée", result.stdout)

    def test_ai_zone_fails_closed_when_parent_quota_is_also_missing(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "quota-ai-parent-missing"
        self.env["GCP_ZONE"] = "europe-west4-ai1a"

        result = self.run_script("check_quotas.sh")

        self.assertNotEqual(result.returncode, 0)
        self.assertRegex(
            result.stdout,
            r"HDB_TOTAL_IOPS \(europe-west4-a\)\s+UNKNOWN\s+\d+\s+UNKNOWN",
        )
        self.assertRegex(
            result.stdout,
            r"HDB_TOTAL_THROUGHPUT \(europe-west4-a\)\s+UNKNOWN\s+\d+\s+UNKNOWN",
        )
        self.assertIn("Quotas disponibles insuffisants ou inconnus", result.stdout)

    def test_unknown_ai_zone_has_no_implicit_parent_quota_mapping(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "quota-success"
        self.env["GCP_REGION"] = "europe-west8"
        self.env["GCP_ZONE"] = "europe-west8-ai1b"

        result = self.run_script("check_quotas.sh")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn(
            "Aucune correspondance de quota parent n'est documentée pour "
            "europe-west8-ai1b",
            result.stdout,
        )
        self.assertEqual(self.commands(), "")

    def test_quota_check_fails_when_exact_rtx_spot_quota_is_missing(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "quota-missing-rtx"

        result = self.run_script("check_quotas.sh")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn(
            "Le quota Cloud Quotas exact "
            "PREEMPTIBLE-NVIDIA-RTX-PRO-6000-GPUS-per-project-region "
            "est absent ou illisible",
            result.stdout,
        )
        self.assertNotIn("compute instances create", self.commands())
        self.assertNotIn("compute instances start", self.commands())

    def test_cpu_quota_is_optional_and_private_network_skips_address(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "quota-missing-cpu"
        self.env["GCP_REQUIRE_EXTERNAL_ADDRESS"] = "0"

        result = self.run_script("check_quotas.sh")

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertRegex(
            result.stdout,
            r"IN_USE_ADDRESSES\s+N/A\s+N/A\s+N/A\s+N/A",
        )
        self.assertRegex(
            result.stdout,
            r"PREEMPTIBLE_CPUS \(informatif\)\s+UNKNOWN\s+0\s+UNKNOWN\s+N/A G4",
        )
        self.assertNotIn("IN-USE-ADDRESSES-per-project-region", self.commands())
        self.assertIn("[SUCCÈS]", result.stdout)

    def test_unlimited_hdb_performance_quotas_are_accepted(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "quota-unlimited-performance"

        result = self.run_script("check_quotas.sh")

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertRegex(
            result.stdout,
            r"HDB_TOTAL_IOPS \(europe-west4-a\)\s+-1\s+1600\s+UNLIMITED\s+>= 600",
        )
        self.assertRegex(
            result.stdout,
            r"HDB_TOTAL_THROUGHPUT \(europe-west4-a\)\s+-1\s+210\s+UNLIMITED\s+>= 150",
        )

    def test_start_fails_closed_when_guest_guard_readback_is_missing(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "guest-readback"
        result = self.run_script("start_and_verify.sh", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("relu de manière certaine", result.stdout)
        self.assertIn("[DIAGNOSTIC GARDE INVITÉE] No scheduled shutdown", result.stdout)
        self.assertIn("compute instances start", self.commands())
        self.assertIn("compute instances stop", self.commands())

    def test_start_rejects_unusable_session_keys_before_gce_mutation(self) -> None:
        original_key = self.env["GCP_SSH_KEY_FILE"]
        encrypted_key = self.tmp / "encrypted-session-ed25519"
        subprocess.run(
            [
                "ssh-keygen",
                "-q",
                "-t",
                "ed25519",
                "-N",
                "test-passphrase",
                "-f",
                str(encrypted_key),
            ],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
        )
        encrypted_key.chmod(0o600)
        permissive_key = self.tmp / "permissive-session-ed25519"
        shutil.copy2(self.ssh_key, permissive_key)
        shutil.copy2(
            Path(str(self.ssh_key) + ".pub"), Path(str(permissive_key) + ".pub")
        )
        permissive_key.chmod(0o640)
        cases = (
            ("missing", self.tmp / "missing-session-ed25519", "absente"),
            ("encrypted", encrypted_key, "chiffrée"),
            ("permissive", permissive_key, "mode 600"),
        )

        for name, key, diagnostic in cases:
            with self.subTest(name=name):
                self.log.unlink(missing_ok=True)
                self.env["FAKE_GCLOUD_SCENARIO"] = "guest-success"
                self.env["GCP_SSH_KEY_FILE"] = str(key)
                result = self.run_script("start_and_verify.sh", "--yes")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(diagnostic, result.stdout)
                self.assertNotIn("compute instances start", self.commands())
        self.env["GCP_SSH_KEY_FILE"] = original_key

    def test_start_requires_unique_bounded_oslogin_session_key(self) -> None:
        cases = (
            ("oslogin-key-missing", None),
            ("oslogin-key-unbounded", None),
            ("guest-success", "28799"),
        )

        for scenario, remaining in cases:
            with self.subTest(scenario=scenario, remaining=remaining):
                self.log.unlink(missing_ok=True)
                self.env["FAKE_GCLOUD_SCENARIO"] = scenario
                if remaining is None:
                    self.env.pop("FAKE_OSLOGIN_REMAINING_SECONDS", None)
                else:
                    self.env["FAKE_OSLOGIN_REMAINING_SECONDS"] = remaining
                result = self.run_script("start_and_verify.sh", "--yes")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("expiration restante", result.stdout)
                self.assertNotIn("compute instances start", self.commands())
        self.env.pop("FAKE_OSLOGIN_REMAINING_SECONDS", None)

    def test_publickey_denial_is_reported_after_bounded_propagation_retries(
        self,
    ) -> None:
        fake_sleep = self.tmp / "sleep"
        fake_sleep.write_text("#!/usr/bin/env bash\nexit 0\n", encoding="utf-8")
        fake_sleep.chmod(0o755)
        self.env["FAKE_GCLOUD_SCENARIO"] = "guest-publickey-denied"

        result = self.run_script("start_and_verify.sh", "--yes")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("refusée 6 fois", result.stdout)
        self.assertIn("Permission denied (publickey)", result.stdout)
        self.assertEqual(6, self.commands().count("compute ssh ehgp-blackwell-spot"))
        self.assertIn("compute instances stop", self.commands())

    def test_start_timeout_never_triggers_an_unversioned_stop(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "guest-success"
        self.env["FAKE_TIMEOUT_SCENARIO"] = "expire-start"
        result = self.run_script("start_and_verify.sh", "--yes")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("génération lastStartTimestamp inconnue", result.stdout)
        self.assertIn("aucun arrêt automatique non versionné", result.stdout)
        self.assertNotIn("compute instances stop", self.commands())
        self.assertIn(
            "gcloud compute instances start ehgp-blackwell-spot",
            self.timeout_log.read_text(encoding="utf-8"),
        )

    def test_start_publishes_targeted_handoff_after_gce_guard(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "guest-success"
        handoff = self.tmp / "start-handoff.json"
        result = self.run_script(
            "start_and_verify.sh",
            "--yes",
            "--guest-shutdown-minutes",
            "45",
            "--handoff-file",
            str(handoff),
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertEqual(
            {
                "guest_shutdown_minutes": 45,
                "instance": "ehgp-blackwell-spot",
                "last_start_timestamp": self.last_start_timestamp,
                "project": "devpod-gpu-exploration",
                "schema": "e-hgp.start-handoff.v3",
                "status": "targeted_running",
                "zone": "europe-west4-a",
            },
            json.loads(handoff.read_text(encoding="utf-8")),
        )
        source = (ROOT / "gcp-migration" / "start_and_verify.sh").read_text(
            encoding="utf-8"
        )
        self.assertIn("tempfile.mkstemp", source)
        self.assertIn("os.link(temporary, path, follow_symlinks=False)", source)
        self.assertNotIn("os.open(path, os.O_WRONLY | os.O_CREAT", source)
        self.assertIn(
            f"--ssh-key-file={self.ssh_key}",
            self.commands(),
        )
        self.assertRegex(
            self.commands(),
            r"--ssh-key-expiration=\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}Z",
        )
        self.assertNotIn("--ssh-key-expire-after", self.commands())
        stopped = self.run_script(
            "stop_and_verify.sh",
            "--yes",
            "--expected-last-start-timestamp",
            self.last_start_timestamp,
        )
        self.assertEqual(stopped.returncode, 0, stopped.stdout)

    def test_start_certifies_computed_deadline_when_ai_zone_omits_timestamp(
        self,
    ) -> None:
        self.env["GCP_ZONE"] = "europe-west4-ai1a"
        self.env["FAKE_MAX_RUN_SECONDS"] = "3600"
        self.env["FAKE_GCLOUD_SCENARIO"] = "guest-success-no-termination-timestamp"
        handoff = self.tmp / "ai-zone-start-handoff.json"

        result = self.run_script(
            "start_and_verify.sh",
            "--yes",
            "--guest-shutdown-minutes",
            "45",
            "--handoff-file",
            str(handoff),
        )

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn(
            "terminationTimestamp non exposé; échéance calculée certifiée",
            result.stdout,
        )
        self.assertEqual(
            self.last_start_timestamp,
            json.loads(handoff.read_text(encoding="utf-8"))["last_start_timestamp"],
        )
        stopped = self.run_script(
            "stop_and_verify.sh",
            "--yes",
            "--expected-last-start-timestamp",
            self.last_start_timestamp,
        )
        self.assertEqual(stopped.returncode, 0, stopped.stdout)

    def test_missing_or_invalid_timestamp_paths_fail_closed(
        self,
    ) -> None:
        now = datetime.now(timezone.utc)
        cases = (
            (
                "regular-zone-missing",
                "guest-success-no-termination-timestamp",
                "europe-west4-a",
                now - timedelta(seconds=10),
                None,
            ),
            (
                "future-generation",
                "guest-success-no-termination-timestamp",
                "europe-west4-ai1a",
                now + timedelta(hours=2),
                None,
            ),
            (
                "stale-generation",
                "guest-success-no-termination-timestamp",
                "europe-west4-ai1a",
                now - timedelta(minutes=10),
                None,
            ),
            (
                "malformed-nonempty-timestamp",
                "guest-success",
                "europe-west4-ai1a",
                now - timedelta(seconds=10),
                "not-a-timestamp",
            ),
            (
                "duration-race",
                "guest-success-no-term-duration-race",
                "europe-west4-ai1a",
                now - timedelta(seconds=10),
                None,
            ),
        )

        for name, scenario, zone, start_time, termination in cases:
            with self.subTest(name=name):
                self.log.unlink(missing_ok=True)
                self.env["FAKE_GCLOUD_SCENARIO"] = scenario
                self.env["GCP_ZONE"] = zone
                self.env["FAKE_MAX_RUN_SECONDS"] = "3600"
                self.env["FAKE_LAST_START_TIMESTAMP"] = start_time.isoformat().replace(
                    "+00:00", "Z"
                )
                if termination is None:
                    self.env.pop("FAKE_TERMINATION_TIMESTAMP", None)
                else:
                    self.env["FAKE_TERMINATION_TIMESTAMP"] = termination

                result = self.run_script(
                    "start_and_verify.sh",
                    "--yes",
                    "--guest-shutdown-minutes",
                    "45",
                )

                self.assertNotEqual(result.returncode, 0)
                self.assertIn("garde post-démarrage", result.stdout)
                self.assertIn("compute instances stop", self.commands())

    def test_late_timestamp_cannot_shorten_the_guest_deadline(self) -> None:
        self.env["GCP_ZONE"] = "europe-west4-ai1a"
        self.env["FAKE_MAX_RUN_SECONDS"] = "3600"
        self.env["FAKE_GCLOUD_SCENARIO"] = "guest-success-late-termination-timestamp"
        handoff = self.tmp / "late-timestamp-handoff.json"
        safe_deadline = (
            int(
                datetime.fromisoformat(
                    self.last_start_timestamp.replace("Z", "+00:00")
                ).timestamp()
            )
            + 3600
            - 300
        )

        result = self.run_script(
            "start_and_verify.sh",
            "--yes",
            "--guest-shutdown-minutes",
            "45",
            "--handoff-file",
            str(handoff),
        )

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn(str(safe_deadline), self.commands())
        stopped = self.run_script(
            "stop_and_verify.sh",
            "--yes",
            "--expected-last-start-timestamp",
            self.last_start_timestamp,
        )
        self.assertEqual(stopped.returncode, 0, stopped.stdout)

    def test_guest_guard_interruption_and_failed_stop_preserve_generation(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "guest-interrupt"
        self.env["FAKE_TIMEOUT_SCENARIO"] = "expire-stop"
        fake_sleep = self.tmp / "sleep"
        fake_sleep.write_text(
            '#!/usr/bin/env bash\nkill -TERM "${PPID}"\nexit 143\n',
            encoding="utf-8",
        )
        fake_sleep.chmod(0o755)
        handoff = self.tmp / "start-handoff.json"

        result = self.run_script(
            "start_and_verify.sh",
            "--yes",
            "--guest-shutdown-minutes",
            "45",
            "--handoff-file",
            str(handoff),
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Arrêt non vérifié", result.stdout)
        self.assertIn("[HANDOFF CONSERVÉ]", result.stdout)
        self.assertEqual(
            {
                "guest_shutdown_minutes": 45,
                "instance": "ehgp-blackwell-spot",
                "last_start_timestamp": self.last_start_timestamp,
                "project": "devpod-gpu-exploration",
                "schema": "e-hgp.start-handoff.v3",
                "status": "targeted_running",
                "zone": "europe-west4-a",
            },
            json.loads(handoff.read_text(encoding="utf-8")),
        )
        self.assertIn(
            "gcloud compute instances stop ehgp-blackwell-spot",
            self.timeout_log.read_text(encoding="utf-8"),
        )

    def test_stop_refuses_unlabelled_target_before_mutation(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "unlabelled"
        result = self.run_script("stop_and_verify.sh", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ne porte pas le label project=e-hgp", result.stdout)
        self.assertNotIn("compute instances stop", self.commands())

    def test_stop_refuses_generation_mismatch_before_mutation(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "guest-success"
        result = self.run_script(
            "stop_and_verify.sh",
            "--yes",
            "--expected-last-start-timestamp",
            "2000-01-01T00:00:00.000Z",
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Génération différente", result.stdout)
        self.assertNotIn("compute instances stop", self.commands())

    def test_stop_cannot_certify_a_missing_target_as_terminated(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "missing-target"
        result = self.run_script("stop_and_verify.sh", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("TERMINATED ne peut pas être certifié", result.stdout)

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

    def prepare_blackwell_preflight_fixture(self, nvcc_content: str) -> Path:
        def write_executable(name: str, content: str) -> Path:
            path = self.tmp / name
            path.write_text(content, encoding="utf-8")
            path.chmod(0o755)
            return path

        write_executable(
            "nvidia-smi",
            """#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == --query-gpu=* ]]; then
    printf '0, NVIDIA RTX PRO 6000 Blackwell Server Edition, 12.0, 97887, 580.159.03\\n'
else
    printf 'NVIDIA-SMI 580.159.03\\n'
fi
""",
        )
        write_executable("journalctl", "#!/usr/bin/env bash\nexit 0\n")
        write_executable("dmesg", "#!/usr/bin/env bash\nexit 0\n")
        write_executable(
            "modinfo",
            """#!/usr/bin/env bash
set -euo pipefail
if [[ "$*" == "nvidia" ]]; then
    exit 0
elif [[ "$*" == "-F license nvidia" ]]; then
    printf 'Dual MIT/GPL\\n'
elif [[ "$*" == "-F filename nvidia" ]]; then
    printf '/lib/modules/fake/nvidia-open.ko\\n'
else
    exit 1
fi
""",
        )
        write_executable(
            "dpkg-query",
            "#!/usr/bin/env bash\nprintf 'install ok installed\\t580.159.03\\n'\n",
        )
        write_executable(
            "sudo",
            '#!/usr/bin/env bash\n[[ "$*" == "-n true" ]] && exit 1\nexit 1\n',
        )
        cuda_home = self.tmp / "cuda-12.9"
        (cuda_home / "bin").mkdir(parents=True)
        nvcc = cuda_home / "bin" / "nvcc"
        nvcc.write_text(nvcc_content, encoding="utf-8")
        nvcc.chmod(0o755)
        self.env["CUDA_HOME"] = str(cuda_home)
        self.env["PATH"] = f"{self.tmp}:/usr/bin:/bin"
        return nvcc

    def test_blackwell_preflight_resolves_nvcc_from_cuda_home_outside_path(
        self,
    ) -> None:
        nvcc = self.prepare_blackwell_preflight_fixture(
            "#!/usr/bin/env bash\nprintf 'Cuda compilation tools, release 12.9, V12.9.85\\n'\n"
        )

        result = self.run_script("blackwell_preflight.sh", "--skip-docker")

        self.assertEqual(0, result.returncode, result.stdout)
        self.assertIn(f"[INFO] nvcc : {nvcc}", result.stdout)
        self.assertIn("Toolkit CUDA 12.9 détecté", result.stdout)

    def test_blackwell_preflight_rejects_wrong_nvcc_version(self) -> None:
        self.prepare_blackwell_preflight_fixture(
            "#!/usr/bin/env bash\nprintf 'Cuda compilation tools, release 12.8, V12.8.93\\n'\n"
        )

        result = self.run_script("blackwell_preflight.sh", "--skip-docker")

        self.assertNotEqual(0, result.returncode)
        self.assertIn("nvcc n'annonce pas CUDA 12.9", result.stdout)
        self.assertNotIn("[OK] Toolkit CUDA 12.9", result.stdout)

    def test_blackwell_preflight_rejects_failed_nvcc_version_command(self) -> None:
        self.prepare_blackwell_preflight_fixture(
            "#!/usr/bin/env bash\nprintf 'Cuda compilation tools, release 12.9, V12.9.85\\n'\nexit 42\n"
        )

        result = self.run_script("blackwell_preflight.sh", "--skip-docker")

        self.assertNotEqual(0, result.returncode)
        self.assertIn("La commande nvcc --version a échoué", result.stdout)
        self.assertNotIn("[OK] Toolkit CUDA 12.9", result.stdout)

    def test_blackwell_preflight_declares_usual_cuda_12_9_fallbacks(self) -> None:
        source = (ROOT / "gcp-migration" / "blackwell_preflight.sh").read_text(
            encoding="utf-8"
        )

        self.assertIn("/usr/local/cuda/bin/nvcc", source)
        self.assertIn("/usr/local/cuda-12.9/bin/nvcc", source)

    def test_blackwell_preflight_keeps_sudo_docker_smoke_noninteractive(
        self,
    ) -> None:
        self.prepare_blackwell_preflight_fixture(
            "#!/usr/bin/env bash\nprintf 'Cuda compilation tools, release 12.9, V12.9.85\\n'\n"
        )
        docker_log = self.tmp / "docker.log"
        docker = self.tmp / "docker"
        docker.write_text(
            """#!/usr/bin/env bash
set -euo pipefail
printf 'DOCKER %s\\n' "$*" >>"${FAKE_DOCKER_LOG}"
if [[ "${1:-}" == "info" ]]; then
    [[ "${FAKE_DOCKER_VIA_SUDO:-0}" == "1" ]]
elif [[ "${1:-}" == "run" ]]; then
    [[ "${FAKE_DOCKER_VIA_SUDO:-0}" == "1" ]]
    printf 'NVIDIA RTX PRO 6000 Blackwell Server Edition, 12.0, 97887 MiB\\n'
else
    exit 2
fi
""",
            encoding="utf-8",
        )
        docker.chmod(0o755)
        secure_docker = self.tmp / "secure-docker"
        secure_docker.write_text(
            """#!/usr/bin/env bash
set -euo pipefail
printf 'SECURE-DOCKER %s\\n' "$*" >>"${FAKE_DOCKER_LOG}"
if [[ "${1:-}" == "info" ]]; then
    exit 0
elif [[ "${1:-}" == "run" ]]; then
    printf 'NVIDIA RTX PRO 6000 Blackwell Server Edition, 12.0, 97887 MiB\\n'
else
    exit 2
fi
""",
            encoding="utf-8",
        )
        secure_docker.chmod(0o755)
        sudo = self.tmp / "sudo"
        sudo.write_text(
            """#!/usr/bin/env bash
set -euo pipefail
printf 'SUDO %s\\n' "$*" >>"${FAKE_DOCKER_LOG}"
if [[ "${1:-}" == "-n" && "${2:-}" == "--" && "${3:-}" == "/usr/bin/docker" ]]; then
    shift 3
    "${FAKE_SECURE_DOCKER}" "$@"
    exit
fi
exit 1
""",
            encoding="utf-8",
        )
        sudo.chmod(0o755)
        self.env["FAKE_DOCKER_LOG"] = str(docker_log)
        self.env["FAKE_SECURE_DOCKER"] = str(secure_docker)

        result = self.run_script("blackwell_preflight.sh")

        self.assertEqual(0, result.returncode, result.stdout)
        log = docker_log.read_text(encoding="utf-8")
        self.assertIn("DOCKER info", log)
        self.assertIn("SUDO -n -- /usr/bin/docker info", log)
        self.assertIn("SUDO -n -- /usr/bin/docker run", log)
        self.assertIn("SECURE-DOCKER run", log)
        self.assertNotIn("\nDOCKER run", "\n" + log)
        self.assertIn("Docker accède au GPU", result.stdout)

        docker_log.write_text("", encoding="utf-8")
        sudo.write_text("#!/usr/bin/env bash\nexit 1\n", encoding="utf-8")
        sudo.chmod(0o755)

        failed = self.run_script("blackwell_preflight.sh")

        self.assertNotEqual(0, failed.returncode)
        self.assertIn("daemon Docker est inaccessible", failed.stdout)
        self.assertNotIn("DOCKER run", docker_log.read_text(encoding="utf-8"))


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
        provision_source = ROOT / "gcp-migration" / "phase3_remote_docker_provision.sh"
        provision = migration / provision_source.name
        shutil.copy2(provision_source, provision)
        provision.chmod(0o755)

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
        fake_timeout = fake_bin / "timeout"
        fake_timeout.write_text(FAKE_TIMEOUT, encoding="utf-8")
        fake_timeout.chmod(0o755)

        self.gcloud_log = self.tmp / "qualification-gcloud.log"
        self.git_log = self.tmp / "qualification-git.log"
        self.session_log = self.tmp / "qualification-session.log"
        self.timeout_log = self.tmp / "qualification-timeout.log"
        self.results = self.tmp / "results"
        self.head = "a" * 40
        now = datetime.now(timezone.utc)
        self.pre_start_timestamp = (
            (now - timedelta(seconds=20)).isoformat().replace("+00:00", "Z")
        )
        self.last_start_timestamp = (
            (now - timedelta(seconds=10)).isoformat().replace("+00:00", "Z")
        )
        self.termination_timestamp = (
            (
                datetime.fromisoformat(self.last_start_timestamp.replace("Z", "+00:00"))
                + timedelta(seconds=3600)
            )
            .isoformat()
            .replace("+00:00", "Z")
        )
        self.env = os.environ.copy()
        self.env.update(
            {
                "PATH": f"{fake_bin}:{self.env['PATH']}",
                "FAKE_GCLOUD_LOG": str(self.gcloud_log),
                "FAKE_GCLOUD_SCENARIO": "qualification-success",
                "FAKE_GIT_LOG": str(self.git_log),
                "FAKE_GIT_HEAD": self.head,
                "FAKE_GIT_SCENARIO": "clean",
                "FAKE_LAST_START_TIMESTAMP": self.last_start_timestamp,
                "FAKE_PRE_START_TIMESTAMP": self.pre_start_timestamp,
                "FAKE_MAX_RUN_SECONDS": "3600",
                "FAKE_REPOSITORY_ROOT": str(self.repository),
                "FAKE_SESSION_LOG": str(self.session_log),
                "FAKE_TERMINATION_TIMESTAMP": self.termination_timestamp,
                "FAKE_TIMEOUT_LOG": str(self.timeout_log),
                "GCP_PROJECT_ID": "devpod-gpu-exploration",
                "GCP_ZONE": "europe-west4-a",
                "GCP_INSTANCE_NAME": "ehgp-blackwell-spot",
                "GCP_GUEST_SHUTDOWN_MINUTES": "45",
                "TMPDIR": str(self.tmp),
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

    def handoff_path(self) -> Path:
        return self.results / f"phase3-{self.head}.start-handoff.json"

    def session_private_key_path(self) -> Path:
        match = re.search(
            r"--(?:ssh-)?key-file=(\S+/id_ed25519)(?:\.pub)?(?:\s|$)",
            self.gcloud_commands(),
        )
        self.assertIsNotNone(match, self.gcloud_commands())
        return Path(match.group(1))

    def standard_arguments(self) -> tuple[str, ...]:
        return ("--yes", "--result-dir", str(self.results))

    def safe_gce_deadline(self) -> int:
        start_epoch = int(
            datetime.fromisoformat(
                self.last_start_timestamp.replace("Z", "+00:00")
            ).timestamp()
        )
        return start_epoch + 3600 - 300

    def test_success_retrieves_artifact_and_independently_certifies_terminated(
        self,
    ) -> None:
        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("[TERMINATED]", result.stdout)
        self.assertIn("[SUCCÈS] Qualification Phase 3", result.stdout)
        artifact = self.results / f"phase3-{self.head}.json"
        value = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual("passed", value["status"])
        self.assertEqual("TERMINATED", value["vm_lifecycle"]["final_status"])
        self.assertTrue(value["vm_lifecycle"]["targeted_stop_verified"])
        self.assertEqual(
            self.last_start_timestamp,
            value["vm_lifecycle"]["last_start_timestamp"],
        )
        self.assertEqual(
            "ehgp-blackwell-spot",
            value["vm_lifecycle"]["instance"],
        )

        session = self.session_commands()
        self.assertIn(
            "start project=devpod-gpu-exploration zone=europe-west4-a "
            "instance=ehgp-blackwell-spot args=--yes --guest-shutdown-minutes 45 "
            "--handoff-file",
            session,
        )
        self.assertIn(
            "stop project=devpod-gpu-exploration zone=europe-west4-a "
            "instance=ehgp-blackwell-spot args=--yes "
            "--expected-last-start-timestamp " + self.last_start_timestamp,
            session,
        )

        commands = self.gcloud_commands()
        self.assertIn("compute ssh ehgp-blackwell-spot", commands)
        self.assertIn("compute scp ehgp-blackwell-spot:", commands)
        self.assertIn("compute os-login ssh-keys add", commands)
        self.assertIn("--ttl=70m", commands)
        self.assertIn("compute os-login ssh-keys remove", commands)
        self.assertNotIn("--ssh-key-expire-after", commands)
        transport_commands = [
            line
            for line in commands.splitlines()
            if line.startswith("compute ssh ") or line.startswith("compute scp ")
        ]
        self.assertTrue(transport_commands, commands)
        fixed_expirations = []
        for command in transport_commands:
            match = re.search(r"--ssh-key-expiration=(\S+)", command)
            self.assertIsNotNone(match, command)
            fixed_expirations.append(match.group(1))
        self.assertEqual(1, len(set(fixed_expirations)), transport_commands)
        self.assertRegex(
            fixed_expirations[0],
            r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}Z$",
        )
        self.assertIn("compute instances describe ehgp-blackwell-spot", commands)
        self.assertNotIn("rm -rf -- /tmp/morsehgp3d-phase3.A1b2C3d4", commands)
        self.assertIn(f"checkout --quiet --detach {self.head}", commands)
        self.assertIn(
            "phase3_remote_qualification.sh --yes --gce-deadline-epoch "
            + str(self.safe_gce_deadline()),
            commands,
        )
        self.assertNotIn("compute instances start", commands)
        self.assertNotIn("compute instances stop", commands)
        self.assertNotIn("ehgp-concurrent", commands)
        self.assertFalse(self.handoff_path().exists())
        session_key = self.session_private_key_path()
        self.assertIn(f"--ssh-key-file={session_key}", commands)
        self.assertIn(f"ssh_key={session_key}", self.session_commands())
        self.assertFalse(session_key.exists())
        self.assertFalse(Path(str(session_key) + ".pub").exists())
        self.assertFalse(session_key.parent.exists())

    def test_explicit_docker_provisioning_precedes_worker_and_recaptures_gce(
        self,
    ) -> None:
        result = self.run_qualification(
            "--yes",
            "--provision-docker",
            "--result-dir",
            str(self.results),
        )

        self.assertEqual(0, result.returncode, result.stdout)
        commands = self.gcloud_commands()
        provision = commands.index("phase3_remote_docker_provision.sh")
        worker = commands.index("phase3_remote_qualification.sh")
        self.assertLess(provision, worker)
        self.assertIn(
            "phase3_remote_docker_provision.sh --yes --gce-deadline-epoch "
            + str(self.safe_gce_deadline()),
            commands,
        )
        describes_before_worker = commands[:worker].count(
            "compute instances describe ehgp-blackwell-spot"
        )
        self.assertGreaterEqual(describes_before_worker, 3, commands)
        self.assertIn("[TERMINATED]", result.stdout)

    def test_docker_provisioning_failure_never_starts_worker_and_stops_target(
        self,
    ) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "qualification-provision-failure"

        result = self.run_qualification(
            "--yes",
            "--provision-docker",
            "--result-dir",
            str(self.results),
        )

        self.assertEqual(43, result.returncode, result.stdout)
        commands = self.gcloud_commands()
        self.assertIn("phase3_remote_docker_provision.sh", commands)
        self.assertNotIn("phase3_remote_qualification.sh", commands)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertIn("[TERMINATED]", result.stdout)
        self.assertFalse((self.results / f"phase3-{self.head}.json").exists())

    def test_post_provision_gce_recertification_failure_stops_without_worker(
        self,
    ) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = (
            "qualification-post-provision-recertification-failure"
        )

        result = self.run_qualification(
            "--yes",
            "--provision-docker",
            "--result-dir",
            str(self.results),
        )

        self.assertNotEqual(0, result.returncode, result.stdout)
        self.assertIn(
            "La garde GCE n'a pas pu être recertifiée après la préparation Docker",
            result.stdout,
        )
        commands = self.gcloud_commands()
        provision_index = commands.index("phase3_remote_docker_provision.sh")
        post_provision = commands[provision_index:]
        self.assertIn("compute instances describe ehgp-blackwell-spot", post_provision)
        self.assertNotIn("phase3_remote_qualification.sh", commands)
        session = self.session_commands()
        self.assertIn(
            "stop project=devpod-gpu-exploration zone=europe-west4-a "
            "instance=ehgp-blackwell-spot args=--yes "
            "--expected-last-start-timestamp " + self.last_start_timestamp,
            session,
        )
        self.assertEqual(1, session.count("stop project="))
        self.assertIn("[TERMINATED]", result.stdout)
        self.assertFalse(self.handoff_path().exists())
        self.assertFalse((self.results / f"phase3-{self.head}.json").exists())

    def test_exact_ai_capacity_target_is_allowed(self) -> None:
        self.env["GCP_ZONE"] = "europe-west4-ai1a"
        self.env["GCP_INSTANCE_NAME"] = "ehgp-blackwell-spot-ai1a"
        self.env["FAKE_TERMINATION_TIMESTAMP"] = ""

        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 0, result.stdout)
        artifact = self.results / f"phase3-{self.head}.json"
        value = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(
            "ehgp-blackwell-spot-ai1a",
            value["vm_lifecycle"]["instance"],
        )
        self.assertEqual("europe-west4-ai1a", value["vm_lifecycle"]["zone"])
        self.assertIn(
            "start project=devpod-gpu-exploration zone=europe-west4-ai1a "
            "instance=ehgp-blackwell-spot-ai1a",
            self.session_commands(),
        )
        self.assertIn(
            "stop project=devpod-gpu-exploration zone=europe-west4-ai1a "
            "instance=ehgp-blackwell-spot-ai1a args=--yes "
            "--expected-last-start-timestamp " + self.last_start_timestamp,
            self.session_commands(),
        )
        commands = self.gcloud_commands()
        self.assertIn("compute ssh ehgp-blackwell-spot-ai1a", commands)
        self.assertIn("compute scp ehgp-blackwell-spot-ai1a:", commands)
        self.assertIn(
            "compute instances describe ehgp-blackwell-spot-ai1a",
            commands,
        )
        self.assertIn("[TERMINATED]", result.stdout)
        self.assertIn(
            "--gce-deadline-epoch " + str(self.safe_gce_deadline()),
            commands,
        )
        self.assertFalse(self.handoff_path().exists())

    def test_deadline_recheck_rejects_wrong_duration_and_stops_target(
        self,
    ) -> None:
        self.env["FAKE_MAX_RUN_SECONDS"] = "7200"

        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("durée exacte de 3600 s", result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertNotIn("phase3_remote_qualification.sh", self.gcloud_commands())
        self.assertIn("[TERMINATED]", result.stdout)

    def test_deadline_recheck_rejects_expired_work_window_and_stops_target(
        self,
    ) -> None:
        old_start = datetime.now(timezone.utc) - timedelta(minutes=40)
        self.last_start_timestamp = old_start.isoformat().replace("+00:00", "Z")
        self.env["FAKE_LAST_START_TIMESTAMP"] = self.last_start_timestamp
        self.env["FAKE_TERMINATION_TIMESTAMP"] = (
            (old_start + timedelta(seconds=3600)).isoformat().replace("+00:00", "Z")
        )

        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("GCE-30 min", result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertNotIn("phase3_remote_qualification.sh", self.gcloud_commands())
        self.assertIn("[TERMINATED]", result.stdout)

    def test_deadline_recheck_rejects_incoherent_timestamp_and_stops_target(
        self,
    ) -> None:
        self.env["FAKE_TERMINATION_TIMESTAMP"] = (
            (datetime.now(timezone.utc) + timedelta(hours=3))
            .isoformat()
            .replace("+00:00", "Z")
        )

        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("échéance de travail", result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertNotIn("phase3_remote_qualification.sh", self.gcloud_commands())
        self.assertIn("[TERMINATED]", result.stdout)

    def test_remote_failure_still_stops_and_returns_original_failure(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "qualification-remote-failure"
        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 42, result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertIn("[TERMINATED]", result.stdout)
        self.assertFalse((self.results / f"phase3-{self.head}.json").exists())
        self.assertFalse(self.handoff_path().exists())
        self.assertIn("compute os-login ssh-keys remove", self.gcloud_commands())
        self.assertFalse(self.session_private_key_path().exists())

    def test_oslogin_import_failure_never_starts_and_cleans_local_key(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "qualification-oslogin-import-failure"

        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("aucun démarrage", result.stdout)
        self.assertEqual("", self.session_commands())
        commands = self.gcloud_commands()
        self.assertIn("compute os-login ssh-keys add", commands)
        self.assertIn("compute os-login ssh-keys remove", commands)
        session_key = self.session_private_key_path()
        self.assertFalse(session_key.exists())
        self.assertFalse(session_key.parent.exists())

    def test_failed_start_with_changed_generation_preserves_session_key(self) -> None:
        self.env["FAKE_SESSION_SCENARIO"] = "start-failure"
        self.env["FAKE_QUALIFICATION_POST_START_STATUS"] = "TERMINATED"

        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(37, result.returncode, result.stdout)
        self.assertIn("[CLÉ SSH CONSERVÉE]", result.stdout)
        commands = self.gcloud_commands()
        self.assertNotIn("compute os-login ssh-keys remove", commands)
        session_key = self.session_private_key_path()
        self.assertTrue(session_key.exists())
        self.assertTrue(Path(str(session_key) + ".pub").exists())

    def test_failed_start_without_new_generation_cleans_session_key(self) -> None:
        self.env["FAKE_SESSION_SCENARIO"] = "start-failure"
        self.env["FAKE_QUALIFICATION_POST_START_STATUS"] = "TERMINATED"
        self.env["FAKE_LAST_START_TIMESTAMP"] = self.pre_start_timestamp

        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(37, result.returncode, result.stdout)
        commands = self.gcloud_commands()
        self.assertIn("compute os-login ssh-keys remove", commands)
        session_key = self.session_private_key_path()
        self.assertFalse(session_key.exists())
        self.assertFalse(session_key.parent.exists())

    def test_symlinked_tmpdir_into_worktree_is_rejected_and_cleaned(self) -> None:
        physical_tmp = self.repository / "key-tmp"
        physical_tmp.mkdir()
        linked_tmp = self.tmp / "linked-key-tmp"
        linked_tmp.symlink_to(physical_tmp, target_is_directory=True)
        self.env["TMPDIR"] = str(linked_tmp)

        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(0, result.returncode, result.stdout)
        self.assertIn("hors du worktree", result.stdout)
        self.assertEqual("", self.session_commands())
        self.assertNotIn("compute os-login ssh-keys add", self.gcloud_commands())
        self.assertEqual([], list(physical_tmp.iterdir()))

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

    def test_sigterm_at_start_handoff_still_stops_target(self) -> None:
        self.env["FAKE_SESSION_SCENARIO"] = "start-race"
        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 143, result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertIn("[TERMINATED]", result.stdout)
        self.assertFalse((self.results / f"phase3-{self.head}.json").exists())

    def test_immediate_preemption_handoff_is_stopped_before_exit(self) -> None:
        self.env["FAKE_SESSION_SCENARIO"] = "preemption-handoff"

        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 37, result.stdout)
        self.assertIn(
            "stop project=devpod-gpu-exploration zone=europe-west4-a "
            "instance=ehgp-blackwell-spot args=--yes "
            "--expected-last-start-timestamp " + self.last_start_timestamp,
            self.session_commands(),
        )
        self.assertIn("[TERMINATED]", result.stdout)
        self.assertFalse(self.handoff_path().exists())
        self.assertFalse((self.results / f"phase3-{self.head}.json").exists())

    def test_failed_guest_guard_and_failed_stop_keep_orchestrator_handoff(
        self,
    ) -> None:
        self.env["FAKE_SESSION_SCENARIO"] = "guard-failure-stop-failure"

        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 90, result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertIn("[HANDOFF CONSERVÉ]", result.stdout)
        self.assertEqual(
            self.last_start_timestamp,
            json.loads(self.handoff_path().read_text(encoding="utf-8"))[
                "last_start_timestamp"
            ],
        )
        self.assertFalse((self.results / f"phase3-{self.head}.json").exists())
        commands = self.gcloud_commands()
        self.assertNotIn("compute os-login ssh-keys remove", commands)
        self.assertTrue(self.session_private_key_path().exists())

    def test_invalid_start_handoff_never_triggers_unversioned_stop(self) -> None:
        for scenario in ("invalid-handoff", "missing-handoff"):
            with self.subTest(scenario=scenario):
                self.env["FAKE_SESSION_SCENARIO"] = scenario
                result = self.run_qualification(*self.standard_arguments())

                self.assertEqual(result.returncode, 90, result.stdout)
                self.assertIn("témoin ciblé certifié", result.stdout)
                self.assertIn("génération lastStartTimestamp absente", result.stdout)
                self.assertNotIn(
                    "stop project=devpod-gpu-exploration", self.session_commands()
                )
                self.assertFalse((self.results / f"phase3-{self.head}.json").exists())
                if scenario == "invalid-handoff":
                    self.assertIn("[HANDOFF CONSERVÉ]", result.stdout)
                    self.assertTrue(self.handoff_path().exists())
                    self.handoff_path().unlink()
                else:
                    self.assertFalse(self.handoff_path().exists())
                self.session_log.unlink(missing_ok=True)

    def test_remote_head_mismatch_fails_closed_and_stops_target(self) -> None:
        self.env["FAKE_REMOTE_HEAD"] = "b" * 40
        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("HEAD distant", result.stdout)
        self.assertIn("différent du SHA local", result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertFalse((self.results / f"phase3-{self.head}.json").exists())
        self.assertIn("[TERMINATED]", result.stdout)

    def test_invalid_remote_artifact_is_never_published(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "qualification-invalid-artifact"
        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("commit propre qualifié", result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertIn("[TERMINATED]", result.stdout)
        self.assertFalse((self.results / f"phase3-{self.head}.json").exists())

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
        self.assertFalse((self.results / f"phase3-{self.head}.json").exists())
        self.assertIn("[HANDOFF CONSERVÉ]", result.stdout)
        self.assertEqual(
            self.last_start_timestamp,
            json.loads(self.handoff_path().read_text(encoding="utf-8"))[
                "last_start_timestamp"
            ],
        )

    def test_stop_script_failure_has_distinct_code_and_precise_target(self) -> None:
        self.env["FAKE_SESSION_SCENARIO"] = "stop-failure"
        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 90, result.stdout)
        self.assertIn("[ARRÊT NON CERTIFIÉ]", result.stdout)
        self.assertIn("Projet=devpod-gpu-exploration", result.stdout)
        self.assertIn("zone=europe-west4-a", result.stdout)
        self.assertIn("instance=ehgp-blackwell-spot", result.stdout)
        self.assertIn("stop_and_verify.sh a échoué", result.stdout)
        self.assertFalse((self.results / f"phase3-{self.head}.json").exists())
        self.assertIn("[HANDOFF CONSERVÉ]", result.stdout)
        self.assertTrue(self.handoff_path().exists())

    def test_independent_readback_must_be_terminated_before_publication(self) -> None:
        self.env["FAKE_GCLOUD_SCENARIO"] = "qualification-stop-still-running"
        result = self.run_qualification(*self.standard_arguments())

        self.assertEqual(result.returncode, 92, result.stdout)
        self.assertIn("dernier état connu=RUNNING", result.stdout)
        self.assertIn("stop project=devpod-gpu-exploration", self.session_commands())
        self.assertFalse((self.results / f"phase3-{self.head}.json").exists())
        self.assertIn("[HANDOFF CONSERVÉ]", result.stdout)
        self.assertTrue(self.handoff_path().exists())

    def test_existing_incident_handoff_blocks_a_new_session(self) -> None:
        self.results.mkdir(parents=True)
        self.handoff_path().write_text("incident\n", encoding="utf-8")

        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("résolvez d'abord cette session ciblée", result.stdout)
        self.assertEqual("incident\n", self.handoff_path().read_text(encoding="utf-8"))
        self.assertEqual([self.handoff_path()], list(self.results.iterdir()))
        self.assertEqual(self.session_commands(), "")

    def test_final_publication_never_replaces_a_concurrent_artifact(self) -> None:
        artifact = self.results / f"phase3-{self.head}.json"
        self.env["FAKE_SESSION_SCENARIO"] = "target-race"
        self.env["FAKE_CONCURRENT_RESULT"] = str(artifact)
        result = self.run_qualification(*self.standard_arguments())

        self.assertNotEqual(result.returncode, 0)
        self.assertIn(
            "refus de remplacer un artefact créé concurremment", result.stdout
        )
        self.assertEqual("concurrent artifact\n", artifact.read_text(encoding="utf-8"))
        self.assertIn("[TERMINATED]", result.stdout)
        self.assertEqual(1, self.session_commands().count("stop project="))

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
        self.assertIn("Cible refusée", result.stdout)
        self.assertEqual(self.session_commands(), "")
        self.assertEqual(self.gcloud_commands(), "")

    def test_crossed_allowed_target_pair_is_rejected_without_mutation(self) -> None:
        for zone, instance in (
            ("europe-west4-ai1a", "ehgp-blackwell-spot"),
            ("europe-west4-a", "ehgp-blackwell-spot-ai1a"),
        ):
            with self.subTest(zone=zone, instance=instance):
                self.env["GCP_ZONE"] = zone
                self.env["GCP_INSTANCE_NAME"] = instance

                result = self.run_qualification(*self.standard_arguments())

                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Cible refusée", result.stdout)
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
