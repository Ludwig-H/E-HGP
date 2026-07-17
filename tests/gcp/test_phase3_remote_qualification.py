#!/usr/bin/env python3
"""Local, mutation-free simulations for the Phase 3 remote worker."""

from __future__ import annotations

import json
import os
import signal
import shutil
import stat
import subprocess
import tempfile
import textwrap
import time
import unittest
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
WORKER_SOURCE = REPOSITORY_ROOT / "gcp-migration" / "phase3_remote_qualification.sh"
ASSEMBLER_SOURCE = (
    REPOSITORY_ROOT
    / "morsehgp3d"
    / "tests"
    / "cuda"
    / "assemble_phase3_qualification.py"
)
FAKE_SHA = "a" * 40
FAKE_IMAGE_ID = "sha256:" + "b" * 64
FAKE_BASE_IMAGE_REF = (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
)


FAKE_GIT = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

args = sys.argv[1:]
with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("GIT " + " ".join(args) + "\n")
if "status" in args:
    if os.environ.get("FAKE_GIT_DIRTY") == "1":
        print("?? untracked")
elif "rev-parse" in args and "--show-toplevel" in args:
    print(os.environ["FAKE_REPO_ROOT"])
elif "rev-parse" in args:
    print(os.environ["FAKE_GIT_SHA"])
else:
    raise SystemExit("unexpected fake git invocation: " + " ".join(args))
"""


FAKE_SUDO = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import sys
import time

args = sys.argv[1:]
with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("SUDO " + " ".join(args) + "\n")
if args == ["-n", "shutdown", "--show"]:
    if os.environ.get("FAKE_GUARD", "present") == "present":
        print("Shutdown scheduled for Thu 2026-07-16 12:45:00 UTC, use 'shutdown -c' to cancel.")
        raise SystemExit(0)
    print("No scheduled shutdown")
    raise SystemExit(0)
if args == ["-n", "cat", "/run/systemd/shutdown/scheduled"]:
    guard = os.environ.get("FAKE_GUARD", "present")
    if guard in {"present", "fallback", "expired", "halt", "too_far", "too_soon"}:
        offsets = {"expired": -1, "too_soon": 60, "too_far": 3600}
        offset = offsets.get(guard, 2400)
        usec = int((time.time() + offset) * 1_000_000)
        mode = "halt" if guard == "halt" else "poweroff"
        print(f"USEC={usec}\nMODE={mode}")
        raise SystemExit(0)
    raise SystemExit(1)
if len(args) >= 3 and args[:2] == ["-n", "--"] and args[2] == os.environ["FAKE_DOCKER_BIN"]:
    if os.environ.get("FAKE_SUDO_DOCKER_MISSING") == "1":
        print("sudo: fixed docker: command not found", file=sys.stderr)
        raise SystemExit(127)
    environment = os.environ.copy()
    environment["FAKE_DOCKER_VIA_SUDO"] = "1"
    environment["FAKE_DOCKER_LOG_PREFIX"] = "SECURE-DOCKER"
    raise SystemExit(
        subprocess.call(
            [os.environ["FAKE_SECURE_DOCKER"], *args[3:]], env=environment
        )
    )
if (
    len(args) >= 4
    and args[:3] == ["-n", "--preserve-env=BUILDX_CONFIG", "--"]
    and args[3] == os.environ["FAKE_BUILDX_PLUGIN"]
):
    environment = os.environ.copy()
    environment["FAKE_BUILDX_VIA_SUDO"] = "1"
    environment["FAKE_BUILDX_LOG_PREFIX"] = "SECURE-BUILDX"
    raise SystemExit(subprocess.call(args[3:], env=environment))
if len(args) >= 3 and args[:2] == ["-n", "--"]:
    raise SystemExit(subprocess.call(args[2:]))
raise SystemExit("unexpected fake sudo invocation: " + " ".join(args))
"""


FAKE_SHUTDOWN = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("SHUTDOWN " + " ".join(sys.argv[1:]) + "\n")
if sys.argv[1:] != ["--show"]:
    raise SystemExit("fake shutdown permits only --show")
if os.environ.get("FAKE_GUARD", "present") == "present":
    print("Shutdown scheduled for Thu 2026-07-16 12:45:00 UTC")
    raise SystemExit(0)
print("No scheduled shutdown")
"""


FAKE_DATE = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys
import time

if sys.argv[1:] != ["+%s"]:
    raise SystemExit("fake date only supports +%s")
with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("DATE +%s\n")
trigger_pattern = os.environ.get("FAKE_DATE_TRIGGER_PATTERN", "")
if trigger_pattern:
    command_log = Path(os.environ["FAKE_COMMAND_LOG"])
    if trigger_pattern in command_log.read_text(encoding="utf-8"):
        print(int(os.environ["FAKE_DATE_TRIGGER_EPOCH"]))
        raise SystemExit(0)
sequence = os.environ.get("FAKE_DATE_SEQUENCE", "")
if not sequence:
    print(int(time.time()))
    raise SystemExit(0)
values = [int(value) for value in sequence.split(",")]
state = Path(os.environ["FAKE_DATE_STATE"])
index = int(state.read_text(encoding="utf-8")) if state.exists() else 0
state.write_text(str(index + 1), encoding="utf-8")
print(values[min(index, len(values) - 1)])
"""


FAKE_BUILDX = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import sys
import time

args = sys.argv[1:]
prefix = os.environ.get("FAKE_BUILDX_LOG_PREFIX", "BUILDX")
config = os.environ.get("BUILDX_CONFIG", "")
with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write(prefix + " " + " ".join(args) + f" BUILDX_CONFIG={config}\n")

if not config:
    raise SystemExit("Buildx was invoked without an isolated BUILDX_CONFIG")
config_path = Path(config)
if not config_path.is_absolute() or config_path.name != "buildx-config":
    raise SystemExit("Buildx config is not the session-scoped directory")

if args == ["version"]:
    print("github.com/docker/buildx v0.20.1 fake")
    raise SystemExit(0)

if not args or args[0] != "build":
    raise SystemExit("unexpected fake buildx invocation: " + " ".join(args))
if "--load" not in args:
    raise SystemExit("fake buildx requires --load")
if os.environ.get("FAKE_FAIL", "") == "build":
    print("simulated buildx build failure", file=sys.stderr)
    raise SystemExit(17)
if os.environ.get("FAKE_BLOCK_BUILDX") == "1":
    descendant = subprocess.Popen(
        [sys.executable, "-c", "import time; time.sleep(60)"]
    )
    Path(os.environ["FAKE_DESCENDANT_PID_STATE"]).write_text(
        str(descendant.pid), encoding="ascii"
    )
    while True:
        time.sleep(1)
iidfile = Path(args[args.index("--iidfile") + 1])
iidfile.write_text(os.environ["FAKE_IMAGE_ID"] + "\n", encoding="utf-8")
print("fake Buildx build complete")
"""


FAKE_BUILDX_TEST = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

args = sys.argv[1:]
with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("BUILDX-TEST " + " ".join(args) + "\n")
plugin = Path(os.environ["FAKE_BUILDX_PLUGIN"])
unsafe = os.environ.get("FAKE_BUILDX_UNSAFE", "")
if len(args) == 2 and args[0] in {"-d", "-f", "-x"}:
    target = Path(args[1])
    result = {
        "-d": target.is_dir(),
        "-f": target.is_file(),
        "-x": target.is_file() and os.access(target, os.X_OK),
    }[args[0]]
    raise SystemExit(0 if result else 1)
if len(args) == 3 and args[:2] == ["!", "-L"]:
    target = Path(args[2])
    if unsafe == "plugin-symlink" and target == plugin:
        raise SystemExit(1)
    if unsafe == "parent-symlink" and target == plugin.parent:
        raise SystemExit(1)
    raise SystemExit(0 if not target.is_symlink() else 1)
raise SystemExit("unexpected fake test invocation: " + " ".join(args))
"""


FAKE_BUILDX_STAT = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

args = sys.argv[1:]
with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("BUILDX-STAT " + " ".join(args) + "\n")
if args[:3] != ["-c", "%n|%u|%a", "--"]:
    raise SystemExit("unexpected fake Buildx stat invocation: " + " ".join(args))
plugin = Path(os.environ["FAKE_BUILDX_PLUGIN"])
unsafe = os.environ.get("FAKE_BUILDX_UNSAFE", "")
for raw_path in args[3:]:
    path = Path(raw_path)
    owner = "1000" if unsafe == "owner" and path == plugin else "0"
    unsafe_mode = unsafe == "mode" and path == plugin
    unsafe_parent_mode = unsafe == "parent-mode" and path == plugin.parent
    mode = "775" if unsafe_mode or unsafe_parent_mode else "755"
    print(f"{raw_path}|{owner}|{mode}")
"""


FAKE_DOCKER_TEST = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

args = sys.argv[1:]
with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("DOCKER-TEST " + " ".join(args) + "\n")
docker = Path(os.environ["FAKE_DOCKER_BIN"])
unsafe = os.environ.get("FAKE_SUDO_DOCKER_UNSAFE", "")
missing = os.environ.get("FAKE_SUDO_DOCKER_MISSING") == "1"
if len(args) == 2 and args[0] in {"-d", "-f", "-x"}:
    target = Path(args[1])
    if missing and target == docker:
        raise SystemExit(1)
    result = {
        "-d": target.is_dir(),
        "-f": target.is_file(),
        "-x": target.is_file() and os.access(target, os.X_OK),
    }[args[0]]
    raise SystemExit(0 if result else 1)
if len(args) == 3 and args[:2] == ["!", "-L"]:
    target = Path(args[2])
    if unsafe == "binary-symlink" and target == docker:
        raise SystemExit(1)
    if unsafe == "parent-symlink" and target == docker.parent:
        raise SystemExit(1)
    raise SystemExit(0 if not target.is_symlink() else 1)
raise SystemExit("unexpected fake Docker test invocation: " + " ".join(args))
"""


FAKE_DOCKER_STAT = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

args = sys.argv[1:]
with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("DOCKER-STAT " + " ".join(args) + "\n")
if args[:3] != ["-Lc", "%n|%u|%a", "--"]:
    raise SystemExit("unexpected fake Docker stat invocation: " + " ".join(args))
docker = Path(os.environ["FAKE_DOCKER_BIN"])
unsafe = os.environ.get("FAKE_SUDO_DOCKER_UNSAFE", "")
if os.environ.get("FAKE_SUDO_DOCKER_MISSING") == "1":
    raise SystemExit(1)
for raw_path in args[3:]:
    path = Path(raw_path)
    owner = "1000" if unsafe == "owner" and path == docker else "0"
    unsafe_mode = unsafe == "mode" and path == docker
    unsafe_parent_mode = unsafe == "parent-mode" and path == docker.parent
    mode = "775" if unsafe_mode or unsafe_parent_mode else "755"
    print(f"{raw_path}|{owner}|{mode}")
"""


FAKE_SYSTEM_TEST = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

args = sys.argv[1:]
with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("SYSTEM-TEST " + " ".join(args) + "\n")
unsafe = os.environ.get("FAKE_SYSTEM_UNSAFE", "")
unsafe_target = Path(os.environ.get("FAKE_SYSTEM_UNSAFE_TARGET", "/nonexistent"))
if len(args) == 2 and args[0] in {"-d", "-f", "-x"}:
    target = Path(args[1])
    result = {
        "-d": target.is_dir(),
        "-f": target.is_file(),
        "-x": target.is_file() and os.access(target, os.X_OK),
    }[args[0]]
    raise SystemExit(0 if result else 1)
if len(args) == 3 and args[:2] == ["!", "-L"]:
    target = Path(args[2])
    if unsafe == "symlink" and target == unsafe_target:
        raise SystemExit(1)
    if unsafe == "parent-symlink" and target == unsafe_target.parent:
        raise SystemExit(1)
    raise SystemExit(0 if not target.is_symlink() else 1)
raise SystemExit("unexpected fixed-system test invocation: " + " ".join(args))
"""


FAKE_SYSTEM_STAT = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

args = sys.argv[1:]
with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("SYSTEM-STAT " + " ".join(args) + "\n")
if args[:3] != ["-Lc", "%n|%u|%a", "--"]:
    raise SystemExit("unexpected fixed-system stat invocation: " + " ".join(args))
unsafe = os.environ.get("FAKE_SYSTEM_UNSAFE", "")
unsafe_target = Path(os.environ.get("FAKE_SYSTEM_UNSAFE_TARGET", "/nonexistent"))
for raw_path in args[3:]:
    path = Path(raw_path)
    owner = "1000" if unsafe == "owner" and path == unsafe_target else "0"
    if unsafe == "parent-owner" and path == unsafe_target.parent:
        owner = "1000"
    mode = "775" if unsafe == "mode" and path == unsafe_target else "755"
    if unsafe == "parent-mode" and path == unsafe_target.parent:
        mode = "775"
    print(f"{raw_path}|{owner}|{mode}")
"""


FAKE_PATH_SYSTEM = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

name = Path(sys.argv[0]).name.upper()
with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write(f"PATH-{name} " + " ".join(sys.argv[1:]) + "\n")
raise SystemExit(99)
"""


FAKE_PATH_DOCKER = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("PATH-DOCKER " + " ".join(sys.argv[1:]) + "\n")
print("untrusted Docker client from PATH", file=sys.stderr)
raise SystemExit(99)
"""


FAKE_DOCKER = r"""#!/usr/bin/env python3
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

args = sys.argv[1:]
log = Path(os.environ["FAKE_COMMAND_LOG"])
with log.open("a", encoding="utf-8") as stream:
    prefix = os.environ.get("FAKE_DOCKER_LOG_PREFIX", "DOCKER")
    stream.write(prefix + " " + " ".join(args) + "\n")

failure = os.environ.get("FAKE_FAIL", "")
image_id = os.environ["FAKE_IMAGE_ID"]

if args == ["--version"]:
    if os.environ.get("FAKE_DOCKER_VERSION_FAIL") == "1":
        print("simulated unusable Docker client", file=sys.stderr)
        raise SystemExit(126)
    print("Docker version 27.5.1, build fake")
    raise SystemExit(0)

if args == ["info"]:
    if os.environ.get("FAKE_ALL_DOCKER_INFO_FAIL") == "1":
        way = "sudo" if os.environ.get("FAKE_DOCKER_VIA_SUDO") == "1" else "direct"
        print(f"simulated Docker daemon unavailable via {way}", file=sys.stderr)
        raise SystemExit(1)
    readiness_state = Path(os.environ["FAKE_DOCKER_READINESS_STATE"])
    ready_after = int(os.environ.get("FAKE_DOCKER_READY_AFTER", "1"))
    if ready_after > 1:
        probe = int(readiness_state.read_text(encoding="utf-8")) if readiness_state.exists() else 0
        readiness_state.write_text(str(probe + 1), encoding="utf-8")
        if probe + 1 < ready_after:
            print("simulated Docker daemon still starting", file=sys.stderr)
            raise SystemExit(1)
    if os.environ.get("FAKE_DIRECT_DOCKER_INFO_FAIL") == "1" and os.environ.get("FAKE_DOCKER_VIA_SUDO") != "1":
        raise SystemExit(1)
    print("fake docker info")
    raise SystemExit(0)

if args and args[0] == "build":
    raise SystemExit("docker build is forbidden; the worker must invoke fixed Buildx")

if args[:2] == ["image", "inspect"]:
    print(image_id)
    raise SystemExit(0)

if args and args[0] == "ps":
    if "--filter" not in args or "--format" not in args:
        raise SystemExit("docker ps is missing its exact filter or format")
    name_filter = args[args.index("--filter") + 1]
    state_path = Path(os.environ["FAKE_CONTAINER_STATE"])
    state = json.loads(state_path.read_text(encoding="utf-8")) if state_path.exists() else {}
    if name_filter.startswith("name=^/") and name_filter.endswith("$"):
        container_name = name_filter[len("name=^/"):-1]
        if os.environ.get("FAKE_NAME_COLLISION") == "1" or container_name in state:
            print(container_name)
    elif name_filter.startswith("id="):
        container_id = name_filter[len("id="):]
        matching = [
            record["id"] for record in state.values() if record["id"] == container_id
        ]
        if matching:
            print(matching[0])
    else:
        raise SystemExit("docker ps received a non-exact filter")
    raise SystemExit(0)

if len(args) == 4 and args[0:2] == ["inspect", "--format"]:
    container_name = args[3]
    state_path = Path(os.environ["FAKE_CONTAINER_STATE"])
    state = json.loads(state_path.read_text(encoding="utf-8")) if state_path.exists() else {}
    record = state.get(container_name)
    if record is None:
        record = next(
            (candidate for candidate in state.values() if candidate["id"] == container_name),
            None,
        )
    if record is None:
        raise SystemExit(1)
    observed_label = record["label"]
    if os.environ.get("FAKE_ATTESTATION_MISMATCH") == "1":
        observed_label = "foreign-session"
    print(
        f'{record["id"]}|/{record["name"]}|{record["image"]}|{observed_label}'
    )
    raise SystemExit(0)

if len(args) == 3 and args[:2] == ["rm", "-f"]:
    cleanup_target = args[2]
    canonical_id = len(cleanup_target) == 64 and all(
        ch in "0123456789abcdef" for ch in cleanup_target
    )
    canonical_name = cleanup_target.startswith("morsehgp3d-phase3-")
    if not canonical_id and not canonical_name:
        raise SystemExit("docker rm received a non-canonical target")
    state_path = Path(os.environ["FAKE_CONTAINER_STATE"])
    state = json.loads(state_path.read_text(encoding="utf-8")) if state_path.exists() else {}
    state_name = cleanup_target if cleanup_target in state else next(
        (
            name
            for name, candidate in state.items()
            if candidate["id"] == cleanup_target
        ),
        None,
    )
    record = state.get(state_name) if state_name is not None else None
    if record is not None and record.get("pid") is not None:
        try:
            os.killpg(int(record["pid"]), signal.SIGKILL)
        except ProcessLookupError:
            pass
    if state_name is not None:
        state.pop(state_name, None)
    state_path.write_text(json.dumps(state), encoding="utf-8")
    print(cleanup_target)
    raise SystemExit(0)

if not args or args[0] != "run":
    raise SystemExit("unexpected fake docker invocation: " + " ".join(args))

index = 1
volumes = {}
container_env = {}
container_entrypoint = None
cidfile = None
container_name = None
session_label = None
while index < len(args):
    option = args[index]
    if option == "--rm":
        index += 1
    elif option in {
        "--gpus",
        "--volume",
        "--workdir",
        "--env",
        "--user",
        "--group-add",
        "--cidfile",
        "--name",
        "--label",
        "--entrypoint",
    }:
        value = args[index + 1]
        if option == "--volume":
            host, container, mode = value.rsplit(":", 2)
            volumes[container] = (Path(host), mode)
        elif option == "--env":
            key, env_value = value.split("=", 1)
            container_env[key] = env_value
        elif option == "--cidfile":
            cidfile = Path(value)
        elif option == "--name":
            container_name = value
        elif option == "--label":
            key, label_value = value.split("=", 1)
            if key != "morsehgp3d.phase3.session":
                raise SystemExit("fake docker run received an unexpected label")
            session_label = label_value
        elif option == "--entrypoint":
            container_entrypoint = value
        index += 2
    else:
        break
if index >= len(args):
    raise SystemExit("fake docker run is missing its image")
image_ref = args[index]
command = args[index + 1:]
if container_entrypoint is not None:
    command = [container_entrypoint, *command]
if image_ref != f"morsehgp3d-phase3:{os.environ['FAKE_GIT_SHA']}":
    raise SystemExit("image tag is not tied to the qualified SHA")
if not command:
    raise SystemExit("fake docker run is missing its command")
if cidfile is None or not cidfile.is_absolute() or cidfile.exists() or cidfile.is_symlink():
    raise SystemExit("fake docker run requires a fresh absolute cidfile")
if cidfile.parent.name != "container-cids":
    raise SystemExit("fake docker cidfile is not session scoped")
if container_name is None or not container_name.startswith("morsehgp3d-phase3-"):
    raise SystemExit("fake docker run requires a canonical session-scoped name")
if session_label is None or len(session_label) != 8:
    raise SystemExit("fake docker run requires its canonical session label")

required_mounts = {
    "/workspace/repository": "ro",
    "/workspace/repository/build": "rw",
    "/results": "rw",
}
for mount, mode in required_mounts.items():
    if volumes.get(mount, (None, None))[1] != mode:
        raise SystemExit(f"missing exact {mount}:{mode} mount")
repository_mount = volumes["/workspace/repository"][0]
repository_build_mountpoint = repository_mount / "build"
if (
    not repository_build_mountpoint.is_dir()
    or repository_build_mountpoint.is_symlink()
):
    raise SystemExit("nested build mountpoint is absent from the read-only repository bind")
if volumes["/workspace/repository/build"][0] == repository_build_mountpoint:
    raise SystemExit("nested build volume must remain isolated from the repository")
for key in ("MORSEHGP3D_CUDA_IMAGE_REF", "MORSEHGP3D_CUDA_IMAGE_ID", "MORSEHGP3D_GIT_SHA"):
    if not container_env.get(key):
        raise SystemExit(f"missing container environment {key}")
if container_env.get("HOME") != "/workspace/repository/build/.phase3-home":
    raise SystemExit("container HOME is not the writable Phase 3 build home")

container_id = "c" * 64
blocked_container = (
    os.environ.get("FAKE_BLOCK_TARGET") == "docker-run"
    and os.environ.get("FAKE_BLOCK_CONTAINER_UNIT", "") == cidfile.stem
)
cidfile_mode = os.environ.get("FAKE_BLOCK_CIDFILE_MODE", "valid")
if blocked_container and cidfile_mode == "absent":
    pass
elif blocked_container and cidfile_mode == "empty":
    cidfile.touch()
elif blocked_container and cidfile_mode == "truncated":
    cidfile.write_text("deadbeef\n", encoding="ascii")
else:
    cidfile.write_text(container_id + "\n", encoding="ascii")
state_path = Path(os.environ["FAKE_CONTAINER_STATE"])
state = json.loads(state_path.read_text(encoding="utf-8")) if state_path.exists() else {}
state[container_name] = {
    "id": container_id,
    "image": image_ref,
    "label": session_label,
    "name": container_name,
    "pid": None,
}
state_path.write_text(json.dumps(state), encoding="utf-8")
if blocked_container:
    container = subprocess.Popen(
        [sys.executable, "-c", "import time; time.sleep(60)"],
        start_new_session=True,
    )
    state = json.loads(state_path.read_text(encoding="utf-8")) if state_path.exists() else {}
    state[container_name]["pid"] = container.pid
    state_path.write_text(json.dumps(state), encoding="utf-8")
    descendant = subprocess.Popen(
        [sys.executable, "-c", "import time; time.sleep(60)"]
    )
    Path(os.environ["FAKE_DESCENDANT_PID_STATE"]).write_text(
        str(descendant.pid), encoding="ascii"
    )
    while True:
        time.sleep(1)

def result_path(container_path: str) -> Path:
    prefix = "/results/"
    if not container_path.startswith(prefix):
        raise SystemExit("result path is outside /results")
    return volumes["/results"][0] / container_path[len(prefix):]

def manifest():
    return {
        "aot_only": True,
        "backend": "cuda_g4",
        "build_mode": "release",
        "cccl_version": 2008002,
        "cccl_version_string": "2.8.2",
        "clock_rate_khz": 1500000,
        "clocks_source": "cudaDeviceProp",
        "complete": True,
        "compilation_during_measurement": False,
        "compiled_sm": "sm_120",
        "cub_version": 200802,
        "cub_version_string": "2.8.2",
        "cuda_compiler_version": 12009085,
        "cuda_compiler_version_string": "12.9.85",
        "cuda_driver_version": 13000,
        "cuda_driver_version_string": "13.0.0",
        "cuda_module_loading": "EAGER",
        "cuda_runtime_version": 12090,
        "cuda_runtime_version_string": "12.9.0",
        "device_index": 0,
        "dlpack_major_version": 1,
        "dlpack_minor_version": 3,
        "dlpack_version_string": "1.3",
        "forest_semantics": None,
        "git_sha": container_env["MORSEHGP3D_GIT_SHA"],
        "gpu_compute_capability_major": 12,
        "gpu_compute_capability_minor": 0,
        "gpu_name": "fake G4",
        "gpu_runtime_sm": "sm_120",
        "gpu_uuid": "01234567-89ab-cdef-0123-456789abcdef",
        "gpu_vram_bytes": 100000000000,
        "host_compiler": "gcc-13.3.0",
        "image_id": container_env["MORSEHGP3D_CUDA_IMAGE_ID"],
        "image_ref": container_env["MORSEHGP3D_CUDA_IMAGE_REF"],
        "memory_clock_rate_khz": 2500000,
        "mode": "certified",
        "nvrtc_used": False,
        "profile": "hgp_reduced",
        "scientific_public_status": None,
        "scientific_result_claimed": False,
    }

def manifest_for_record(ceiling: int, record_kind: str):
    value = manifest()
    if failure == "incomplete_manifest":
        value["complete"] = False
    if failure == "missing_manifest_field":
        value.pop("gpu_uuid")
    if failure == "lazy_module_loading":
        value["cuda_module_loading"] = "LAZY"
    if failure == "wrong_cccl_version":
        value["cccl_version"] = 2007000
        value["cccl_version_string"] = "2.7.0"
    if failure == "wrong_cub_version":
        value["cub_version"] = 200700
        value["cub_version_string"] = "2.7.0"
    if failure == "wrong_cuda_compiler_version":
        value["cuda_compiler_version"] = 12008090
        value["cuda_compiler_version_string"] = "12.8.90"
    if failure == "wrong_cuda_runtime_version":
        value["cuda_runtime_version"] = 12080
        value["cuda_runtime_version_string"] = "12.8.0"
    if failure == "incoherent_version_string":
        value["cuda_compiler_version_string"] = "12.9.84"
    if failure == "incoherent_driver_version_string":
        value["cuda_driver_version_string"] = "13.0.1"
    if failure == "wrong_dlpack_version":
        value["dlpack_minor_version"] = 2
        value["dlpack_version_string"] = "1.2"
    if failure == "inter_run_manifest_mismatch" and ceiling == 4194304:
        value["clock_rate_khz"] += 1
    if failure == "warm_resident_manifest_mismatch" and record_kind == "resident":
        value["clock_rate_khz"] += 1
    if failure == "error_manifest_mismatch" and record_kind == "error":
        value["clock_rate_khz"] += 1
    return value

def structured_error(ceiling: int, record_kind: str):
    value = {
        "code": 101,
        "context_healthy": True,
        "expected": True,
        "message": "invalid device ordinal",
        "name": "cudaErrorInvalidDevice",
        "operation": "cudaGetDeviceProperties(device=-1)",
    }
    if failure == "wrong_error_code":
        value["code"] = 10
    if failure == "warm_resident_error_mismatch" and record_kind == "resident":
        value["message"] = "invalid device ordinal (resident)"
    if failure == "error_record_mismatch" and record_kind == "error":
        value["message"] = "invalid device ordinal (error record)"
    if failure == "inter_run_error_mismatch" and ceiling == 4194304:
        value["message"] = "invalid device ordinal (sanitizer)"
    return value

def write_runtime_jsonl(path: Path, ceiling: int):
    records = []
    for protocol in ("warm", "resident"):
        if failure == "missing_resident" and protocol == "resident":
            continue
        reported_ceiling = ceiling
        if failure == "wrong_runtime_ceiling" and ceiling == 67108864:
            reported_ceiling += 1
        if failure == "wrong_sanitizer_ceiling" and ceiling == 4194304:
            reported_ceiling += 1
        allocation = {
            "allocation_count": 1,
            "configured_ceiling_bytes": reported_ceiling,
            "exact_async_allocation_bytes": reported_ceiling,
            "free_bytes_after": 90000000000,
            "free_bytes_before": 90000000000,
            "free_delta_bytes": 0,
            "leak_free": failure != "leak",
            "live_bytes_final": 1 if failure == "leak" else 0,
            "peak_live_bytes": reported_ceiling,
            "release_count": 1,
            "single_async_arena": True,
            "suballocated_without_extra_cuda_allocations": True,
            "trimmed_default_pool": True,
            "within_configured_ceiling": True,
        }
        if failure == "warm_resident_allocation_mismatch" and protocol == "resident":
            allocation["free_bytes_after"] -= 1
            allocation["free_delta_bytes"] = -1
        determinism = {
            "bitwise_equal": True,
            "cpu_reference_equal": failure != "wrong_reference",
            "cpu_reference_fnv1a64_u32_le": "0123456789abcdef",
            "cpu_reference_sum": 123,
            "element_count": 1024,
            "kernel": "morsehgp3d_phase3_deterministic_kernel",
            "measured_kernel_runs_compared": 2,
            "output_fnv1a64_u32_le": "0123456789abcdef",
            "resident_cub_sum": 123,
            "uses_cub_device_reduce": True,
            "warm_cub_sum": 123,
        }
        if failure == "warm_resident_determinism_mismatch" and protocol == "resident":
            determinism["element_count"] += 1
        timestamps = {
            "warm": (
                "2026-07-16T12:00:00.000Z",
                "2026-07-16T12:00:00.001Z",
            ),
            "resident": (
                "2026-07-16T12:00:00.002Z",
                "2026-07-16T12:00:00.003Z",
            ),
        }
        timestamp_start, timestamp_end = timestamps[protocol]
        if failure == "overlapping_measurements" and protocol == "resident":
            timestamp_start = "2026-07-16T12:00:00.000Z"
        record = {
            "allocation": allocation,
            "compilation_during_measurement": False,
            "determinism": determinism,
            "dlpack": {
                "byte_offset": 0,
                "copy_operations": 0,
                "device_identity": True,
                "pointer_identity": True,
                "zero_copy": True,
            },
            "duration": {"gpu_ns": 1000, "host_ns": 2000},
            "kind": "phase3_runtime_result",
            "manifest": manifest_for_record(ceiling, protocol),
            "protocol": protocol,
            "schema": "morsehgp3d.phase3.runtime.v1",
            "status": "ok",
            "structured_cuda_error": structured_error(ceiling, protocol),
            "timestamp_end_utc": timestamp_end,
            "timestamp_start_utc": timestamp_start,
        }
        records.append(record)
        if failure == "missing_duration":
            records[-1].pop("duration")
        if failure == "bad_timestamp":
            records[-1]["timestamp_end_utc"] = "not-a-timestamp"
    error_timestamp = "2026-07-16T12:00:00.004Z"
    if failure == "error_before_resident_end":
        error_timestamp = "2026-07-16T12:00:00.002Z"
    records.append({
        "compilation_during_measurement": False,
        "cuda_error": structured_error(ceiling, "error"),
        "kind": "phase3_structured_cuda_error_result",
        "manifest": manifest_for_record(ceiling, "error"),
        "schema": "morsehgp3d.phase3.runtime.v1",
        "status": "ok",
        "timestamp_utc": error_timestamp,
    })
    if failure == "out_of_order_records":
        records[0], records[1] = records[1], records[0]
    encoded = [json.dumps(record, sort_keys=True) for record in records]
    if failure == "duplicate_json_key":
        encoded[0] = encoded[0][:-1] + ', "status": "ok"}'
    path.write_text("\n".join(encoded) + "\n", encoding="utf-8")

if command[:3] == ["cmake", "--workflow", "--preset"]:
    preset = command[3]
    if failure == "release" and preset == "cuda-release":
        raise SystemExit(31)
    if failure == "audit" and preset == "cuda-audit":
        raise SystemExit(32)
    print(f"fake {preset} workflow passed")
    raise SystemExit(0)

if command[0].endswith("/morsehgp3d_phase3_runtime"):
    if failure == "runtime":
        raise SystemExit(41)
    output = command[command.index("--output") + 1]
    ceiling = int(command[command.index("--allocation-bytes") + 1])
    write_runtime_jsonl(result_path(output), ceiling)
    print("fake Phase 3 runtime passed")
    raise SystemExit(0)

if command[0] == "python3":
    if failure == "binding":
        raise SystemExit(42)
    print("fake Python binding passed")
    raise SystemExit(0)

if Path(command[0]).name == "cuobjdump" and command[1:2] == ["-lelf"]:
    architecture = "sm_90" if failure == "foreign_arch" else "sm_120"
    print(f"ELF file 1: morsehgp3d_phase3.{architecture}.cubin")
    raise SystemExit(0)

if Path(command[0]).name == "cuobjdump" and command[1:2] == ["-lptx"]:
    if container_entrypoint is None:
        print("NVIDIA container entrypoint banner")
    if failure == "ptx":
        print("PTX file 1: morsehgp3d_phase3.ptx")
    else:
        print("No PTX file found in morsehgp3d_phase3_runtime", file=sys.stderr)
    raise SystemExit(0)

if command[0] == "compute-sanitizer":
    if failure == "sanitizer":
        print("simulated memcheck error", file=sys.stderr)
        raise SystemExit(86)
    output = command[command.index("--output") + 1]
    ceiling = int(command[command.index("--allocation-bytes") + 1])
    write_runtime_jsonl(result_path(output), ceiling)
    print("========= ERROR SUMMARY: 0 errors")
    raise SystemExit(0)

raise SystemExit("unexpected fake container command: " + " ".join(command))
"""


FAKE_SLEEP = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys
import time

with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("SLEEP " + " ".join(sys.argv[1:]) + "\n")
if len(sys.argv) != 2:
    raise SystemExit("fixed fake sleep requires one duration")
time.sleep(min(float(sys.argv[1]) * 0.01, 0.02))
"""


FAKE_SYSTEMCTL = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("SYSTEMCTL " + " ".join(sys.argv[1:]) + "\n")
print("inactive" if "is-active" in sys.argv else "disabled")
raise SystemExit(3 if "is-active" in sys.argv else 1)
"""


FAKE_JOURNALCTL = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("JOURNALCTL " + " ".join(sys.argv[1:]) + "\n")
print("simulated docker.service journal: daemon inactive")
"""


FAKE_DPKG_QUERY = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("DPKG-QUERY " + " ".join(sys.argv[1:]) + "\n")
print("docker.io\\tinstall ok installed\\t27.5.1-fake")
print("nvidia-container-toolkit\\tinstall ok installed\\t1.17.8-fake")
"""


FAKE_TIMEOUT = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

args = sys.argv[1:]
with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
    stream.write("TIMEOUT " + " ".join(args) + "\n")
if args == ["--version"]:
    print("timeout (GNU coreutils) 9.5")
    raise SystemExit(0)
if args == ["--help"]:
    print("Usage: timeout [OPTION] DURATION COMMAND")
    print("  -k, --kill-after=DURATION")
    print("      --foreground")
    raise SystemExit(0)
index = 0
foreground = False
if index < len(args) and args[index] == "--foreground":
    foreground = True
    index += 1
if index < len(args) and args[index].startswith("--kill-after="):
    index += 1
if index >= len(args):
    raise SystemExit("fake timeout is missing its duration")
index += 1
command = args[index:]
if not command:
    raise SystemExit("fake timeout is missing its command")
is_docker_info = command[-1:] == ["info"] and "docker" in " ".join(command)
expire_count = int(os.environ.get("FAKE_DOCKER_INFO_TIMEOUTS", "0"))
state = Path(os.environ["FAKE_TIMEOUT_STATE"])
expired = int(state.read_text(encoding="utf-8")) if state.exists() else 0
if is_docker_info and expired < expire_count:
    state.write_text(str(expired + 1), encoding="utf-8")
    with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
        stream.write("TIMEOUT-EXPIRED docker info 124\n")
    raise SystemExit(124)

block_target = os.environ.get("FAKE_BLOCK_TARGET", "")
is_buildx_build = "build" in command and os.environ["FAKE_BUILDX_PLUGIN"] in command
is_docker_run = "run" in command and os.environ["FAKE_DOCKER_BIN"] in command
should_block = (
    (block_target == "buildx" and is_buildx_build)
    or (block_target == "docker-run" and is_docker_run)
)
if should_block:
    environment = os.environ.copy()
    if is_buildx_build:
        environment["FAKE_BLOCK_BUILDX"] = "1"
        label = "buildx build"
    else:
        environment["FAKE_BLOCK_CONTAINER_UNIT"] = os.environ.get(
            "FAKE_BLOCK_CONTAINER_UNIT", "cuda-release"
        )
        label = "docker run"
    process = subprocess.Popen(
        command,
        env=environment,
        start_new_session=not foreground,
    )
    descendant_state = Path(os.environ["FAKE_DESCENDANT_PID_STATE"])
    deadline = time.monotonic() + 3
    while time.monotonic() < deadline and not descendant_state.exists():
        if process.poll() is not None:
            raise SystemExit(process.returncode)
        time.sleep(0.02)
    if not descendant_state.exists():
        process.kill()
        raise SystemExit("blocked fake command did not create its descendant")
    if foreground:
        process.terminate()
    else:
        os.killpg(process.pid, signal.SIGTERM)
    try:
        process.wait(timeout=0.5)
    except subprocess.TimeoutExpired:
        if foreground:
            process.kill()
        else:
            os.killpg(process.pid, signal.SIGKILL)
        process.wait(timeout=1)
    with Path(os.environ["FAKE_COMMAND_LOG"]).open("a", encoding="utf-8") as stream:
        stream.write(f"TIMEOUT-EXPIRED {label} 124 foreground={int(foreground)}\n")
    raise SystemExit(124)
raise SystemExit(subprocess.call(command))
"""


class Phase3RemoteQualificationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="phase3-remote-test-")
        self.root = Path(self.temporary.name)
        self.repository = self.root / "repository"
        self.fake_bin = self.root / "fake-bin"
        self.fake_secure_bin = self.root / "fake-secure-bin"
        self.fake_system_root = self.root / "trusted-system"
        self.buildx_plugin = (
            self.fake_system_root
            / "usr"
            / "libexec"
            / "docker"
            / "cli-plugins"
            / "docker-buildx"
        )
        self.buildx_test = self.fake_bin / "buildx-test"
        self.buildx_stat = self.fake_bin / "buildx-stat"
        self.docker_test = self.fake_bin / "docker-test"
        self.docker_stat = self.fake_bin / "docker-stat"
        self.fixed_docker = self.fake_secure_bin / "docker"
        self.fixed_timeout = self.fake_secure_bin / "timeout"
        self.fixed_sleep = self.fake_secure_bin / "sleep"
        self.fixed_date = self.fake_secure_bin / "date"
        self.system_test = self.fake_bin / "system-test"
        self.system_stat = self.fake_bin / "system-stat"
        self.output_dir = self.root / "artifacts"
        self.session_tmp = self.root / "session-tmp"
        self.command_log = self.root / "commands.log"
        self.date_state = self.root / "date.state"
        self.docker_readiness_state = self.root / "docker-readiness.state"
        self.timeout_state = self.root / "timeout.state"
        self.descendant_pid_state = self.root / "descendant.pid"
        self.container_state = self.root / "containers.json"
        self.gce_deadline_epoch = int(time.time()) + 3300
        for path in (
            self.repository / "gcp-migration",
            self.repository / "containers",
            self.repository / "morsehgp3d" / "tests" / "cuda",
            self.fake_bin,
            self.fake_secure_bin,
            self.buildx_plugin.parent,
            self.output_dir,
            self.session_tmp,
        ):
            path.mkdir(parents=True, exist_ok=True)

        self.worker = self.repository / "gcp-migration" / WORKER_SOURCE.name
        worker_text = WORKER_SOURCE.read_text(encoding="utf-8")
        replacements = {
            'readonly DOCKER_BIN="/usr/bin/docker"': (
                f'readonly DOCKER_BIN="{self.fixed_docker}"'
            ),
            'readonly DOCKER_TEST_BIN="/usr/bin/test"': (
                f'readonly DOCKER_TEST_BIN="{self.docker_test}"'
            ),
            'readonly DOCKER_STAT_BIN="/usr/bin/stat"': (
                f'readonly DOCKER_STAT_BIN="{self.docker_stat}"'
            ),
            'readonly TIMEOUT_BIN="/usr/bin/timeout"': (
                f'readonly TIMEOUT_BIN="{self.fixed_timeout}"'
            ),
            'readonly TIMEOUT_TEST_BIN="/usr/bin/test"': (
                f'readonly TIMEOUT_TEST_BIN="{self.system_test}"'
            ),
            'readonly TIMEOUT_STAT_BIN="/usr/bin/stat"': (
                f'readonly TIMEOUT_STAT_BIN="{self.system_stat}"'
            ),
            'readonly SLEEP_BIN="/usr/bin/sleep"': (
                f'readonly SLEEP_BIN="{self.fixed_sleep}"'
            ),
            'readonly SLEEP_TEST_BIN="/usr/bin/test"': (
                f'readonly SLEEP_TEST_BIN="{self.system_test}"'
            ),
            'readonly SLEEP_STAT_BIN="/usr/bin/stat"': (
                f'readonly SLEEP_STAT_BIN="{self.system_stat}"'
            ),
            'readonly DATE_BIN="/usr/bin/date"': (
                f'readonly DATE_BIN="{self.fixed_date}"'
            ),
            'readonly DATE_TEST_BIN="/usr/bin/test"': (
                f'readonly DATE_TEST_BIN="{self.system_test}"'
            ),
            'readonly DATE_STAT_BIN="/usr/bin/stat"': (
                f'readonly DATE_STAT_BIN="{self.system_stat}"'
            ),
            'readonly BUILDX_PLUGIN="/usr/libexec/docker/cli-plugins/docker-buildx"': (
                f'readonly BUILDX_PLUGIN="{self.buildx_plugin}"'
            ),
            'readonly BUILDX_TEST_BIN="/usr/bin/test"': (
                f'readonly BUILDX_TEST_BIN="{self.buildx_test}"'
            ),
            'readonly BUILDX_STAT_BIN="/usr/bin/stat"': (
                f'readonly BUILDX_STAT_BIN="{self.buildx_stat}"'
            ),
        }
        for source, replacement in replacements.items():
            self.assertEqual(1, worker_text.count(source))
            worker_text = worker_text.replace(source, replacement)
        self.worker.write_text(worker_text, encoding="utf-8")
        self.worker.chmod(WORKER_SOURCE.stat().st_mode)
        shutil.copy2(
            ASSEMBLER_SOURCE,
            self.repository / "morsehgp3d" / "tests" / "cuda" / ASSEMBLER_SOURCE.name,
        )
        (self.repository / "containers" / "cuda12.9-sm120.Dockerfile").write_text(
            f"FROM {FAKE_BASE_IMAGE_REF}\n", encoding="utf-8"
        )
        self._write_executable(self.fake_bin / "git", FAKE_GIT)
        self._write_executable(self.fake_bin / "sudo", FAKE_SUDO)
        self._write_executable(self.fake_bin / "shutdown", FAKE_SHUTDOWN)
        self._write_executable(self.fake_bin / "docker", FAKE_PATH_DOCKER)
        self._write_executable(self.fixed_docker, FAKE_DOCKER)
        self._write_executable(self.docker_test, FAKE_DOCKER_TEST)
        self._write_executable(self.docker_stat, FAKE_DOCKER_STAT)
        self._write_executable(self.buildx_plugin, FAKE_BUILDX)
        self._write_executable(self.buildx_test, FAKE_BUILDX_TEST)
        self._write_executable(self.buildx_stat, FAKE_BUILDX_STAT)
        self._write_executable(self.fixed_sleep, FAKE_SLEEP)
        self._write_executable(self.fake_bin / "systemctl", FAKE_SYSTEMCTL)
        self._write_executable(self.fake_bin / "journalctl", FAKE_JOURNALCTL)
        self._write_executable(self.fake_bin / "dpkg-query", FAKE_DPKG_QUERY)
        self._write_executable(self.fixed_timeout, FAKE_TIMEOUT)
        self._write_executable(self.fixed_date, FAKE_DATE)
        self._write_executable(self.system_test, FAKE_SYSTEM_TEST)
        self._write_executable(self.system_stat, FAKE_SYSTEM_STAT)
        self._write_executable(self.fake_bin / "timeout", FAKE_PATH_SYSTEM)
        self._write_executable(self.fake_bin / "sleep", FAKE_PATH_SYSTEM)
        self._write_executable(self.fake_bin / "date", FAKE_PATH_SYSTEM)
        self._write_executable(
            self.repository / "gcp-migration" / "blackwell_preflight.sh",
            """#!/usr/bin/env bash
set -euo pipefail
printf 'PREFLIGHT %s\\n' "$*" >>"${FAKE_COMMAND_LOG}"
[[ "$*" == "--skip-docker" ]]
if [[ "${FAKE_FAIL:-}" == "preflight-bytes" ]]; then
    printf '%70000s\\n' x
    exit 24
elif [[ "${FAKE_FAIL:-}" == "preflight" ]]; then
    for line in $(seq 1 260); do
        printf 'simulated preflight diagnostic line %03d\\n' "${line}"
    done
    exit 23
fi
printf 'fake Blackwell preflight passed\\n'
""",
        )
        self.environment = os.environ.copy()
        self.environment.update(
            {
                "FAKE_COMMAND_LOG": str(self.command_log),
                "FAKE_GIT_SHA": FAKE_SHA,
                "FAKE_IMAGE_ID": FAKE_IMAGE_ID,
                "FAKE_REPO_ROOT": str(self.repository),
                "FAKE_GUARD": "present",
                "FAKE_DATE_SEQUENCE": "",
                "FAKE_DATE_STATE": str(self.date_state),
                "FAKE_DOCKER_READINESS_STATE": str(self.docker_readiness_state),
                "FAKE_SECURE_DOCKER": str(self.fixed_docker),
                "FAKE_DOCKER_BIN": str(self.fixed_docker),
                "FAKE_BUILDX_PLUGIN": str(self.buildx_plugin),
                "FAKE_BUILDX_UNSAFE": "",
                "FAKE_TIMEOUT_STATE": str(self.timeout_state),
                "FAKE_DESCENDANT_PID_STATE": str(self.descendant_pid_state),
                "FAKE_CONTAINER_STATE": str(self.container_state),
                "FAKE_SYSTEM_UNSAFE": "",
                "FAKE_SYSTEM_UNSAFE_TARGET": str(self.fixed_timeout),
                "PATH": str(self.fake_bin) + os.pathsep + self.environment["PATH"],
                "TMPDIR": str(self.session_tmp),
                "PYTHONDONTWRITEBYTECODE": "1",
            }
        )

    def tearDown(self) -> None:
        self._cleanup_fake_processes()
        self.temporary.cleanup()

    def _cleanup_fake_processes(self) -> None:
        if self.descendant_pid_state.exists():
            try:
                os.kill(
                    int(self.descendant_pid_state.read_text(encoding="ascii")),
                    signal.SIGKILL,
                )
            except (ProcessLookupError, ValueError):
                pass
        if self.container_state.exists():
            try:
                state = json.loads(self.container_state.read_text(encoding="utf-8"))
            except json.JSONDecodeError:
                state = {}
            for record in state.values():
                pid = record.get("pid") if isinstance(record, dict) else record
                if pid is None:
                    continue
                try:
                    os.killpg(int(pid), signal.SIGKILL)
                except (ProcessLookupError, ValueError):
                    pass

    @staticmethod
    def _write_executable(path: Path, content: str) -> None:
        path.write_text(textwrap.dedent(content), encoding="utf-8")
        path.chmod(path.stat().st_mode | stat.S_IXUSR)

    def run_worker(
        self,
        *,
        arguments: list[str] | None = None,
        failure: str = "",
        guard: str = "present",
        output: Path | None = None,
        direct_docker_info_fail: bool = False,
        all_docker_info_fail: bool = False,
        docker_clients_unusable: bool = False,
        unsafe_sudo_docker: str = "",
        unsafe_buildx: str = "",
        docker_info_timeouts: int = 0,
        docker_ready_after: int = 1,
        date_sequence: list[int] | None = None,
        date_trigger_pattern: str = "",
        date_trigger_epoch: int | None = None,
        block_target: str = "",
        block_container_unit: str = "cuda-release",
        block_cidfile_mode: str = "valid",
        fixed_system_unsafe: str = "",
        fixed_system_target: str = "timeout",
        name_collision: bool = False,
    ) -> tuple[subprocess.CompletedProcess[str], Path]:
        artifact = output or (self.output_dir / "qualification.json")
        environment = self.environment.copy()
        environment["FAKE_FAIL"] = failure
        environment["FAKE_GUARD"] = guard
        environment["FAKE_DIRECT_DOCKER_INFO_FAIL"] = (
            "1" if direct_docker_info_fail else "0"
        )
        environment["FAKE_ALL_DOCKER_INFO_FAIL"] = "1" if all_docker_info_fail else "0"
        environment["FAKE_SUDO_DOCKER_MISSING"] = (
            "1" if docker_clients_unusable else "0"
        )
        environment["FAKE_DOCKER_VERSION_FAIL"] = (
            "1" if docker_clients_unusable else "0"
        )
        environment["FAKE_SUDO_DOCKER_UNSAFE"] = unsafe_sudo_docker
        environment["FAKE_BUILDX_UNSAFE"] = unsafe_buildx
        environment["FAKE_DOCKER_INFO_TIMEOUTS"] = str(docker_info_timeouts)
        environment["FAKE_DOCKER_READY_AFTER"] = str(docker_ready_after)
        environment["FAKE_BLOCK_TARGET"] = block_target
        environment["FAKE_BLOCK_CONTAINER_UNIT"] = (
            block_container_unit if block_target == "docker-run" else ""
        )
        environment["FAKE_BLOCK_CIDFILE_MODE"] = block_cidfile_mode
        environment["FAKE_SYSTEM_UNSAFE"] = fixed_system_unsafe
        fixed_targets = {
            "timeout": self.fixed_timeout,
            "sleep": self.fixed_sleep,
            "date": self.fixed_date,
        }
        environment["FAKE_SYSTEM_UNSAFE_TARGET"] = str(
            fixed_targets[fixed_system_target]
        )
        environment["FAKE_NAME_COLLISION"] = "1" if name_collision else "0"
        environment["FAKE_DATE_SEQUENCE"] = (
            "" if date_sequence is None else ",".join(map(str, date_sequence))
        )
        environment["FAKE_DATE_TRIGGER_PATTERN"] = date_trigger_pattern
        environment["FAKE_DATE_TRIGGER_EPOCH"] = str(
            self.gce_deadline_epoch
            if date_trigger_epoch is None
            else date_trigger_epoch
        )
        self.date_state.unlink(missing_ok=True)
        self.docker_readiness_state.unlink(missing_ok=True)
        self.timeout_state.unlink(missing_ok=True)
        self.descendant_pid_state.unlink(missing_ok=True)
        self.container_state.unlink(missing_ok=True)
        command = [str(self.worker)]
        if arguments is None:
            command.extend(
                [
                    "--yes",
                    "--gce-deadline-epoch",
                    str(self.gce_deadline_epoch),
                    "--output",
                    str(artifact),
                ]
            )
        else:
            command.extend(arguments)
        process = subprocess.Popen(
            command,
            cwd=self.repository,
            env=environment,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            start_new_session=True,
        )
        try:
            stdout, stderr = process.communicate(timeout=60)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
            self._cleanup_fake_processes()
            raise
        result = subprocess.CompletedProcess(
            command,
            process.returncode,
            stdout,
            stderr,
        )
        return result, artifact

    def command_log_text(self) -> str:
        if not self.command_log.exists():
            return ""
        return self.command_log.read_text(encoding="utf-8")

    def assert_recorded_pid_stopped(self, state_path: Path) -> None:
        self.assertTrue(state_path.is_file())
        pid = int(state_path.read_text(encoding="ascii"))
        for _ in range(100):
            stat_path = Path(f"/proc/{pid}/stat")
            if not stat_path.exists():
                return
            fields = stat_path.read_text(encoding="ascii").split()
            if len(fields) >= 3 and fields[2] == "Z":
                return
            time.sleep(0.01)
        self.fail(f"process {pid} is still alive")

    def assert_no_partial_artifact(self) -> None:
        self.assertEqual([], list(self.output_dir.glob(".*.partial")))
        self.assertEqual([], list(self.session_tmp.iterdir()))

    def assert_rejects_runtime_evidence(
        self, failure: str, expected_error: str | None = None
    ) -> None:
        artifact = self.output_dir / f"{failure}.json"
        result, _ = self.run_worker(failure=failure, output=artifact)
        self.assertNotEqual(0, result.returncode, result.stdout + result.stderr)
        if expected_error is not None:
            self.assertIn(expected_error, result.stderr)
        self.assertFalse(artifact.exists())
        self.assert_no_partial_artifact()

    def test_requires_yes_and_absolute_output(self) -> None:
        result, artifact = self.run_worker(
            arguments=["--output", str(self.output_dir / "x.json")]
        )
        self.assertNotEqual(0, result.returncode)
        self.assertIn("--yes", result.stderr)
        self.assertFalse(artifact.exists())
        self.assertEqual("", self.command_log_text())

        result, _ = self.run_worker(
            arguments=[
                "--yes",
                "--gce-deadline-epoch",
                str(self.gce_deadline_epoch),
                "--output",
                "relative.json",
            ]
        )
        self.assertNotEqual(0, result.returncode)
        self.assertIn("absolu", result.stderr.lower())
        self.assertEqual("", self.command_log_text())

    def test_requires_valid_future_gce_deadline(self) -> None:
        artifact = self.output_dir / "deadline.json"
        for value in ("", "abc", "123", str(int(time.time()) + 1700)):
            with self.subTest(value=value):
                self.command_log.unlink(missing_ok=True)
                arguments = ["--yes"]
                if value:
                    arguments.extend(["--gce-deadline-epoch", value])
                arguments.extend(["--output", str(artifact)])
                result, _ = self.run_worker(arguments=arguments, output=artifact)
                self.assertNotEqual(0, result.returncode)
                self.assertFalse(artifact.exists())
                self.assertNotIn("PREFLIGHT", self.command_log_text())
                self.assertNotIn("DOCKER ", self.command_log_text())

    def test_guest_guard_must_precede_the_gce_deadline(self) -> None:
        artifact = self.output_dir / "guard-after-gce.json"
        deadline = int(time.time()) + 2000
        result, _ = self.run_worker(
            arguments=[
                "--yes",
                "--gce-deadline-epoch",
                str(deadline),
                "--output",
                str(artifact),
            ],
            output=artifact,
        )

        self.assertNotEqual(0, result.returncode)
        self.assertFalse(artifact.exists())
        self.assertIn("Arrêt invité planifié absent", result.stderr)
        self.assertNotIn("PREFLIGHT", self.command_log_text())
        self.assertNotIn("DOCKER ", self.command_log_text())

    def test_no_new_unit_starts_at_the_work_deadline(self) -> None:
        base = int(time.time())
        deadline = base + 3300
        work_deadline = deadline - 1800
        cases = (
            (
                "buildx-build",
                "BUILDX version",
                "BUILDX build",
            ),
            (
                "compute-sanitizer",
                "--entrypoint /usr/local/cuda/bin/cuobjdump",
                "compute-sanitizer --tool memcheck",
            ),
        )

        for label, trigger_pattern, forbidden in cases:
            with self.subTest(label=label):
                self.command_log.unlink(missing_ok=True)
                artifact = self.output_dir / f"deadline-{label}.json"
                result, _ = self.run_worker(
                    arguments=[
                        "--yes",
                        "--gce-deadline-epoch",
                        str(deadline),
                        "--output",
                        str(artifact),
                    ],
                    output=artifact,
                    date_trigger_pattern=trigger_pattern,
                    date_trigger_epoch=work_deadline,
                )

                self.assertNotEqual(0, result.returncode)
                self.assertIn(f"unité {label} non lancée", result.stderr)
                self.assertFalse(artifact.exists())
                log = self.command_log_text()
                self.assertIn("PREFLIGHT --skip-docker", log)
                self.assertNotIn(forbidden, log)
                self.assert_no_partial_artifact()

    def test_no_docker_or_buildx_probe_starts_after_deadline_reserve(self) -> None:
        base = int(time.time())
        deadline = base + 3300
        work_deadline = deadline - 1800
        artifact = self.output_dir / "probe-deadline.json"

        result, _ = self.run_worker(
            arguments=[
                "--yes",
                "--gce-deadline-epoch",
                str(deadline),
                "--output",
                str(artifact),
            ],
            output=artifact,
            date_trigger_pattern="DOCKER-TEST -d /",
            date_trigger_epoch=work_deadline,
        )

        self.assertNotEqual(0, result.returncode)
        self.assertIn("SONDE REFUSÉE", result.stderr)
        self.assertFalse(artifact.exists())
        log = self.command_log_text()
        docker_test_timeouts = [
            line
            for line in log.splitlines()
            if line.startswith("TIMEOUT ") and str(self.docker_test) in line
        ]
        self.assertEqual(1, len(docker_test_timeouts))
        self.assertNotIn("DOCKER-STAT", log)
        self.assertNotIn("BUILDX-TEST", log)
        self.assertNotIn("DOCKER --version", log)
        self.assertNotIn("DOCKER info", log)
        self.assertNotIn("DOCKER run", log)
        self.assert_no_partial_artifact()

    def test_rejects_output_inside_worktree_or_already_existing(self) -> None:
        inside = self.repository / "qualification.json"
        result, _ = self.run_worker(output=inside)
        self.assertNotEqual(0, result.returncode)
        self.assertIn("hors du worktree", result.stderr)
        self.assertFalse(inside.exists())
        self.assertNotIn("DOCKER ", self.command_log_text())

        existing = self.output_dir / "existing.json"
        existing.write_text("do not replace\n", encoding="utf-8")
        result, _ = self.run_worker(output=existing)
        self.assertNotEqual(0, result.returncode)
        self.assertEqual("do not replace\n", existing.read_text(encoding="utf-8"))
        self.assertNotIn("DOCKER ", self.command_log_text())

    def test_missing_guest_guard_fails_before_preflight_or_docker(self) -> None:
        result, artifact = self.run_worker(guard="absent")
        self.assertNotEqual(0, result.returncode)
        self.assertIn("Arrêt invité planifié absent", result.stderr)
        self.assertFalse(artifact.exists())
        log = self.command_log_text()
        self.assertIn("SUDO -n cat /run/systemd/shutdown/scheduled", log)
        self.assertNotIn("PREFLIGHT", log)
        self.assertNotIn("DOCKER ", log)
        self.assert_no_partial_artifact()

    def test_preflight_failure_emits_a_bounded_diagnostic_before_cleanup(
        self,
    ) -> None:
        artifact = self.output_dir / "preflight-failure.json"

        result, _ = self.run_worker(failure="preflight", output=artifact)

        self.assertNotEqual(0, result.returncode)
        self.assertIn(
            "[DIAGNOSTIC preflight-blackwell] 240 dernières lignes et 65536 octets",
            result.stderr,
        )
        self.assertNotIn("simulated preflight diagnostic line 001", result.stderr)
        self.assertIn("simulated preflight diagnostic line 021", result.stderr)
        self.assertIn("simulated preflight diagnostic line 260", result.stderr)
        self.assertIn("[DIAGNOSTIC preflight-blackwell] fin.", result.stderr)
        self.assertIn("Le preflight Blackwell non destructif a échoué", result.stderr)
        self.assertFalse(artifact.exists())
        preflight_timeout_lines = [
            line
            for line in self.command_log_text().splitlines()
            if "blackwell_preflight.sh --skip-docker" in line
        ]
        self.assertEqual(1, len(preflight_timeout_lines))
        self.assertIn("TIMEOUT --kill-after=5s ", preflight_timeout_lines[0])
        self.assertNotIn("--foreground", preflight_timeout_lines[0])
        self.assert_no_partial_artifact()

        byte_artifact = self.output_dir / "preflight-byte-limit.json"
        byte_result, _ = self.run_worker(
            failure="preflight-bytes", output=byte_artifact
        )
        self.assertNotEqual(0, byte_result.returncode)
        begin = "octets au plus; début.\n"
        end = "[DIAGNOSTIC preflight-blackwell] fin."
        self.assertIn(begin, byte_result.stderr)
        self.assertIn(end, byte_result.stderr)
        diagnostic = byte_result.stderr.split(begin, 1)[1].split(end, 1)[0]
        self.assertLessEqual(len(diagnostic.encode("utf-8")), 65536)
        self.assertGreater(len(diagnostic.encode("utf-8")), 65000)
        self.assertFalse(byte_artifact.exists())
        self.assert_no_partial_artifact()

    def test_guard_file_fallback_must_be_future(self) -> None:
        fallback_artifact = self.output_dir / "fallback.json"
        result, _ = self.run_worker(guard="fallback", output=fallback_artifact)
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertTrue(fallback_artifact.is_file())
        self.assertIn(
            "SUDO -n cat /run/systemd/shutdown/scheduled", self.command_log_text()
        )

        self.command_log.unlink()
        expired_artifact = self.output_dir / "expired.json"
        result, _ = self.run_worker(guard="expired", output=expired_artifact)
        self.assertNotEqual(0, result.returncode)
        self.assertFalse(expired_artifact.exists())
        self.assertNotIn("DOCKER ", self.command_log_text())
        self.assert_no_partial_artifact()

        for guard in ("halt", "too_far", "too_soon"):
            with self.subTest(guard=guard):
                self.command_log.unlink(missing_ok=True)
                rejected = self.output_dir / f"{guard}.json"
                result, _ = self.run_worker(guard=guard, output=rejected)
                self.assertNotEqual(0, result.returncode)
                self.assertFalse(rejected.exists())
                self.assertNotIn("DOCKER ", self.command_log_text())
                self.assert_no_partial_artifact()

    def test_success_is_atomic_complete_and_scoped(self) -> None:
        result, artifact = self.run_worker()
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertTrue(artifact.is_file())
        value = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertIsInstance(value, dict)
        self.assertEqual("worker_passed_pending_shutdown", value["status"])
        self.assertEqual(FAKE_SHA, value["git"]["sha"])
        self.assertEqual(FAKE_IMAGE_ID, value["image"]["id"])
        self.assertEqual(FAKE_BASE_IMAGE_REF, value["image"]["base_ref"])
        self.assertEqual(["sm_120"], value["checks"]["aot_elf_architectures"])
        self.assertEqual(0, value["checks"]["aot_ptx_entry_count"])
        self.assertEqual("", value["logs"]["cuobjdump_ptx"])
        self.assertIn("No PTX file found", value["logs"]["cuobjdump_ptx_stderr"])
        self.assertEqual(
            {"resident", "warm"},
            {
                record["protocol"]
                for record in value["runtime_records"]
                if record["kind"] == "phase3_runtime_result"
            },
        )
        self.assert_no_partial_artifact()

        log = self.command_log_text()
        self.assertIn("PREFLIGHT --skip-docker", log)
        self.assertIn("BUILDX version BUILDX_CONFIG=", log)
        self.assertIn("BUILDX build --load", log)
        self.assertNotIn("DOCKER build", log)
        self.assertIn(
            "--file "
            + str(self.repository / "containers" / "cuda12.9-sm120.Dockerfile"),
            log,
        )
        self.assertIn("--tag morsehgp3d-phase3:" + FAKE_SHA, log)
        self.assertIn("cmake --workflow --preset cuda-release", log)
        self.assertIn("cmake --workflow --preset cuda-audit", log)
        self.assertIn("--allocation-bytes 67108864", log)
        self.assertIn("tests/cuda/check_phase3_binding.py", log)
        self.assertIn("cuobjdump -lelf", log)
        self.assertIn(
            "--entrypoint /usr/local/cuda/bin/cuobjdump",
            log,
        )
        self.assertIn(
            "compute-sanitizer --tool memcheck --leak-check full "
            "--error-exitcode=86",
            log,
        )
        self.assertIn(str(self.repository) + ":/workspace/repository:ro", log)
        self.assertIn(f"--user {os.getuid()}:{os.getgid()}", log)
        self.assertIn(
            "--env HOME=/workspace/repository/build/.phase3-home",
            log,
        )
        docker_run_lines = [
            line for line in log.splitlines() if line.startswith("DOCKER run ")
        ]
        self.assertEqual(7, len(docker_run_lines))
        for line in docker_run_lines:
            self.assertIn("--name morsehgp3d-phase3-", line)
            self.assertIn("--label morsehgp3d.phase3.session=", line)
            self.assertIn("--cidfile ", line)
        self.assertEqual(7, log.count("DOCKER rm -f " + "c" * 64))
        self.assertNotIn("PATH-DOCKER", log)
        self.assertFalse((self.repository / "build").exists())

        second, _ = self.run_worker(output=artifact)
        self.assertNotEqual(0, second.returncode)
        self.assertEqual(value, json.loads(artifact.read_text(encoding="utf-8")))

    def test_symlinked_repository_build_mountpoint_is_refused(self) -> None:
        external_build = self.root / "external-build"
        external_build.mkdir()
        (self.repository / "build").symlink_to(external_build, target_is_directory=True)

        result, artifact = self.run_worker()

        self.assertNotEqual(0, result.returncode)
        self.assertFalse(artifact.exists())
        self.assertIn(
            "point de montage build du dépôt doit être un répertoire non symbolique",
            result.stderr,
        )
        self.assertNotIn("DOCKER run ", self.command_log_text())
        self.assertTrue((self.repository / "build").is_symlink())
        self.assert_no_partial_artifact()

    def test_build_uses_only_the_fixed_buildx_plugin(self) -> None:
        source = WORKER_SOURCE.read_text(encoding="utf-8")
        self.assertIn(
            'readonly BUILDX_PLUGIN="/usr/libexec/docker/cli-plugins/docker-buildx"',
            source,
        )
        self.assertIn('"${BUILDX[@]}" build --load', source)
        self.assertNotIn('"${DOCKER[@]}" build', source)

        result, artifact = self.run_worker()
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertTrue(artifact.is_file())
        log = self.command_log_text()
        self.assertIn("BUILDX-STAT ", log)
        self.assertIn(str(self.buildx_plugin.parent), log)
        self.assertIn("BUILDX build --load", log)
        self.assertNotIn("DOCKER build", log)
        buildx_lines = [
            line for line in log.splitlines() if line.startswith("BUILDX build ")
        ]
        self.assertEqual(1, len(buildx_lines))
        self.assertIn("BUILDX_CONFIG=" + str(self.session_tmp), buildx_lines[0])
        self.assert_no_partial_artifact()

    def test_uses_only_certified_fixed_docker_and_gnu_timeout_paths(self) -> None:
        source = WORKER_SOURCE.read_text(encoding="utf-8")
        self.assertIn('readonly DOCKER_BIN="/usr/bin/docker"', source)
        self.assertIn('readonly TIMEOUT_BIN="/usr/bin/timeout"', source)
        self.assertIn('readonly DATE_BIN="/usr/bin/date"', source)
        self.assertIn('readonly SLEEP_BIN="/usr/bin/sleep"', source)
        self.assertNotIn("command -v docker", source)
        self.assertNotIn("command -v timeout", source)
        self.assertNotIn("$(date +%s)", source)
        self.assertNotIn("\nsleep ", source)
        self.assertNotIn("timeout --foreground", source)
        self.assertIn(
            "soft_timeout=$((remaining - WORK_UNIT_KILL_AFTER_SECONDS - "
            "WORK_UNIT_POST_TIMEOUT_RESERVE_SECONDS))",
            source,
        )
        self.assertIn('"${TIMEOUT_BIN}" --version', source)
        self.assertIn('"${TIMEOUT_BIN}" --help', source)

        result, artifact = self.run_worker()

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertTrue(artifact.is_file())
        log = self.command_log_text()
        self.assertIn("TIMEOUT --version", log)
        self.assertIn("TIMEOUT --help", log)
        self.assertIn("DOCKER-STAT ", log)
        self.assertNotIn("PATH-DOCKER", log)
        self.assertNotIn("PATH-TIMEOUT", log)
        self.assertNotIn("PATH-DATE", log)
        self.assertNotIn("PATH-SLEEP", log)
        self.assertIn("SYSTEM-STAT ", log)
        self.assertIn("DATE +%s", log)
        self.assert_no_partial_artifact()

    def test_rejects_unsafe_timeout_date_or_sleep_chain_before_preflight(
        self,
    ) -> None:
        cases = (
            ("timeout", "owner"),
            ("date", "mode"),
            ("sleep", "symlink"),
            ("timeout", "parent-owner"),
            ("date", "parent-mode"),
            ("sleep", "parent-symlink"),
        )
        for fixed_system_target, unsafe in cases:
            with self.subTest(target=fixed_system_target, unsafe=unsafe):
                self.command_log.unlink(missing_ok=True)
                artifact = (
                    self.output_dir / f"unsafe-{fixed_system_target}-{unsafe}.json"
                )
                result, _ = self.run_worker(
                    output=artifact,
                    fixed_system_unsafe=unsafe,
                    fixed_system_target=fixed_system_target,
                )

                self.assertNotEqual(0, result.returncode)
                self.assertIn("chaîne fixe /usr/bin/", result.stderr)
                self.assertFalse(artifact.exists())
                log = self.command_log_text()
                self.assertNotIn("PREFLIGHT", log)
                self.assertNotIn("DOCKER ", log)
                self.assertNotIn("PATH-TIMEOUT", log)
                self.assertNotIn("PATH-DATE", log)
                self.assertNotIn("PATH-SLEEP", log)

    def test_blocked_buildx_is_group_killed_before_any_container_or_artifact(
        self,
    ) -> None:
        result, artifact = self.run_worker(block_target="buildx")

        self.assertNotEqual(0, result.returncode)
        self.assertIn("construction de l'image CUDA Phase 3 a échoué", result.stderr)
        self.assertFalse(artifact.exists())
        log = self.command_log_text()
        self.assertIn("TIMEOUT-EXPIRED buildx build 124 foreground=0", log)
        self.assertNotIn("DOCKER image inspect", log)
        self.assertNotIn("DOCKER run", log)
        self.assert_recorded_pid_stopped(self.descendant_pid_state)
        self.assert_no_partial_artifact()

    def test_blocked_container_and_descendant_are_killed_and_cid_is_removed(
        self,
    ) -> None:
        result, artifact = self.run_worker(
            block_target="docker-run",
            block_container_unit="cuda-release",
        )

        self.assertNotEqual(0, result.returncode)
        self.assertIn("workflow cuda-release a échoué", result.stderr)
        self.assertFalse(artifact.exists())
        log = self.command_log_text()
        self.assertIn("TIMEOUT-EXPIRED docker run 124 foreground=0", log)
        self.assertIn("DOCKER run --name morsehgp3d-phase3-", log)
        self.assertIn("DOCKER rm -f " + "c" * 64, log)
        self.assertNotIn("cmake --workflow --preset cuda-audit", log)
        self.assertNotIn("morsehgp3d_phase3_runtime", log)
        self.assert_recorded_pid_stopped(self.descendant_pid_state)
        state = json.loads(self.container_state.read_text(encoding="utf-8"))
        self.assertEqual({}, state)
        self.assert_no_partial_artifact()

    def test_create_before_cidfile_is_attested_removed_and_never_published(
        self,
    ) -> None:
        for cidfile_mode in ("absent", "empty", "truncated"):
            with self.subTest(cidfile_mode=cidfile_mode):
                self.command_log.unlink(missing_ok=True)
                artifact = self.output_dir / f"blocked-{cidfile_mode}.json"
                result, _ = self.run_worker(
                    output=artifact,
                    block_target="docker-run",
                    block_container_unit="cuda-release",
                    block_cidfile_mode=cidfile_mode,
                )

                self.assertNotEqual(0, result.returncode)
                self.assertFalse(artifact.exists())
                log = self.command_log_text()
                self.assertIn("TIMEOUT-EXPIRED docker run 124 foreground=0", log)
                self.assertIn("DOCKER inspect --format", log)
                self.assertIn("DOCKER rm -f " + "c" * 64, log)
                self.assertNotIn("DOCKER rm -f morsehgp3d-phase3-", log)
                self.assertNotIn("cmake --workflow --preset cuda-audit", log)
                self.assertGreaterEqual(log.count("SLEEP 1"), 2)
                self.assert_recorded_pid_stopped(self.descendant_pid_state)
                state = json.loads(self.container_state.read_text(encoding="utf-8"))
                self.assertEqual({}, state)
                self.assert_no_partial_artifact()

    def test_name_collision_is_refused_before_docker_run(self) -> None:
        result, artifact = self.run_worker(name_collision=True)

        self.assertNotEqual(0, result.returncode)
        self.assertIn("Collision de nom Docker refusée", result.stderr)
        self.assertFalse(artifact.exists())
        log = self.command_log_text()
        self.assertIn("DOCKER ps -a --no-trunc --filter name=^/", log)
        self.assertNotIn("DOCKER run --name", log)
        self.assertNotIn("DOCKER rm -f", log)
        self.assert_no_partial_artifact()

    def test_failures_publish_no_artifact(self) -> None:
        for failure in ("build", "runtime", "audit", "sanitizer"):
            with self.subTest(failure=failure):
                artifact = self.output_dir / f"{failure}.json"
                result, _ = self.run_worker(failure=failure, output=artifact)
                self.assertNotEqual(0, result.returncode, failure)
                self.assertFalse(artifact.exists(), failure)
                self.assert_no_partial_artifact()

    def test_rejects_incomplete_runtime_evidence(self) -> None:
        for failure in (
            "incomplete_manifest",
            "leak",
            "missing_resident",
            "wrong_reference",
            "missing_manifest_field",
            "lazy_module_loading",
            "missing_duration",
            "bad_timestamp",
        ):
            with self.subTest(failure=failure):
                self.assert_rejects_runtime_evidence(failure)

    def test_rejects_wrong_or_incoherent_frozen_versions_and_error_code(self) -> None:
        failures = {
            "wrong_cccl_version": "cccl_version must identify 2.8.x",
            "wrong_cub_version": "cub_version must identify 2.8.x",
            "wrong_cuda_compiler_version": (
                "cuda_compiler_version must identify 12.9.x"
            ),
            "wrong_cuda_runtime_version": ("cuda_runtime_version must identify 12.9.x"),
            "incoherent_version_string": (
                "cuda_compiler_version_string must be 12.9.85"
            ),
            "incoherent_driver_version_string": (
                "cuda_driver_version_string must be 13.0.0"
            ),
            "wrong_dlpack_version": "DLPack version must be 1.3",
            "wrong_error_code": "code must be 101",
        }
        for failure, expected_error in failures.items():
            with self.subTest(failure=failure):
                self.assert_rejects_runtime_evidence(failure, expected_error)

    def test_rejects_wrong_ceilings_and_cross_record_inequalities(self) -> None:
        failures = {
            "wrong_runtime_ceiling": ("configured_ceiling_bytes must be 67108864"),
            "wrong_sanitizer_ceiling": ("configured_ceiling_bytes must be 4194304"),
            "warm_resident_allocation_mismatch": (
                "warm and resident allocation evidence must be identical"
            ),
            "warm_resident_determinism_mismatch": (
                "warm and resident determinism evidence must be identical"
            ),
            "warm_resident_manifest_mismatch": (
                "warm and resident manifest evidence must be identical"
            ),
            "warm_resident_error_mismatch": (
                "warm and resident structured_cuda_error evidence must be identical"
            ),
            "error_manifest_mismatch": (
                "manifests must be identical across all three records"
            ),
            "error_record_mismatch": (
                "structured CUDA errors must be identical across all records"
            ),
        }
        for failure, expected_error in failures.items():
            with self.subTest(failure=failure):
                self.assert_rejects_runtime_evidence(failure, expected_error)

    def test_rejects_bad_order_duplicate_keys_and_inter_run_mismatches(self) -> None:
        failures = {
            "overlapping_measurements": (
                "resident measurement starts before the warm measurement ends"
            ),
            "error_before_resident_end": (
                "structured error record predates the resident measurement end"
            ),
            "out_of_order_records": (
                "records must be ordered warm, resident, structured error"
            ),
            "inter_run_manifest_mismatch": (
                "runtime and sanitizer runs disagree on manifest or structured CUDA error"
            ),
            "inter_run_error_mismatch": (
                "runtime and sanitizer runs disagree on manifest or structured CUDA error"
            ),
            "duplicate_json_key": "duplicate JSON object key: status",
        }
        for failure, expected_error in failures.items():
            with self.subTest(failure=failure):
                self.assert_rejects_runtime_evidence(failure, expected_error)

    def test_rejects_ptx_and_foreign_aot_architecture(self) -> None:
        for failure in ("ptx", "foreign_arch"):
            with self.subTest(failure=failure):
                artifact = self.output_dir / f"{failure}.json"
                result, _ = self.run_worker(failure=failure, output=artifact)
                self.assertNotEqual(0, result.returncode)
                self.assertFalse(artifact.exists())
                self.assert_no_partial_artifact()

    def test_never_invokes_gcp_or_host_mutations(self) -> None:
        result, _ = self.run_worker()
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        log = self.command_log_text().lower()
        for forbidden in (
            "gcloud",
            "apt-get",
            " instances start",
            " instances stop",
            "reboot",
            "shutdown -p",
            "shutdown -h",
            "shutdown -c",
            "systemctl",
        ):
            self.assertNotIn(forbidden, log)
        self.assertIn("sudo -n cat /run/systemd/shutdown/scheduled", log)
        self.assertNotIn("shutdown --show", log)

    def test_uses_noninteractive_sudo_docker_and_buildx_fallback(self) -> None:
        result, artifact = self.run_worker(direct_docker_info_fail=True)
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertTrue(artifact.is_file())
        log = self.command_log_text()
        self.assertIn(f"SUDO -n -- {self.fixed_docker} info", log)
        self.assertIn(f"SUDO -n -- {self.fixed_docker} image inspect", log)
        self.assertEqual(7, log.count(f"SUDO -n -- {self.fixed_docker} run"))
        self.assertEqual(7, log.count(f"SUDO -n -- {self.fixed_docker} rm -f"))
        self.assertIn(
            "SUDO -n --preserve-env=BUILDX_CONFIG -- "
            + str(self.buildx_plugin)
            + " build --load",
            log,
        )
        self.assertIn("SECURE-BUILDX build --load", log)
        self.assertNotIn("SECURE-DOCKER build", log)
        self.assertFalse(
            any(
                line.startswith(("DOCKER build", "DOCKER image", "DOCKER run"))
                for line in log.splitlines()
            )
        )

    def test_retries_docker_daemon_readiness_without_host_mutation(self) -> None:
        result, artifact = self.run_worker(docker_ready_after=5)

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertTrue(artifact.is_file())
        self.assertIn("tentative 3/6", result.stdout)
        log = self.command_log_text()
        self.assertEqual(2, log.count("SLEEP 5"))
        self.assertEqual(5, log.count("DOCKER info"))
        self.assertNotIn("SYSTEMCTL", log)

    def test_retries_after_individually_timed_out_docker_probes(self) -> None:
        result, artifact = self.run_worker(docker_info_timeouts=2)

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertTrue(artifact.is_file())
        log = self.command_log_text()
        self.assertEqual(2, log.count("TIMEOUT-EXPIRED docker info 124"))
        self.assertEqual(1, log.count("SLEEP 5"))
        self.assertIn("tentative 2/6", result.stdout)
        self.assertIn("BUILDX build --load", log)
        self.assertNotIn("DOCKER build", log)

    def test_reports_bounded_docker_diagnostics_after_all_retries(self) -> None:
        result, artifact = self.run_worker(all_docker_info_fail=True)

        self.assertNotEqual(0, result.returncode)
        self.assertIn(
            "[DIAGNOSTIC docker-info] 240 dernières lignes et 65536 octets",
            result.stderr,
        )
        self.assertIn("simulated Docker daemon unavailable via direct", result.stderr)
        self.assertIn("simulated Docker daemon unavailable via sudo", result.stderr)
        self.assertIn("inactive", result.stderr)
        self.assertIn("docker.io", result.stderr)
        self.assertIn("daemon inaccessible", result.stderr)
        self.assertFalse(artifact.exists())
        self.assert_no_partial_artifact()
        log = self.command_log_text()
        self.assertEqual(5, log.count("SLEEP 5"))
        self.assertEqual(12, log.count("DOCKER info"))
        self.assertNotIn("DOCKER build", log)
        self.assertNotIn("DOCKER run", log)

    def test_rejects_unusable_docker_clients_without_retry(self) -> None:
        result, artifact = self.run_worker(docker_clients_unusable=True)

        self.assertNotEqual(0, result.returncode)
        self.assertIn("Client Docker fixe /usr/bin/docker", result.stderr)
        self.assertFalse(artifact.exists())
        self.assert_no_partial_artifact()
        log = self.command_log_text()
        self.assertNotIn("SLEEP", log)
        self.assertNotIn("DOCKER info", log)
        self.assertNotIn("DOCKER build", log)

    def test_rejects_unsafe_elevated_docker_path(self) -> None:
        for unsafe in ("owner", "mode", "parent-mode"):
            with self.subTest(unsafe=unsafe):
                self.command_log.unlink(missing_ok=True)
                artifact = self.output_dir / f"unsafe-{unsafe}.json"
                result, _ = self.run_worker(
                    output=artifact,
                    direct_docker_info_fail=True,
                    unsafe_sudo_docker=unsafe,
                )

                self.assertNotEqual(0, result.returncode)
                self.assertIn("Client Docker fixe /usr/bin/docker", result.stderr)
                self.assertFalse(artifact.exists())
                self.assert_no_partial_artifact()
                log = self.command_log_text()
                self.assertIn("DOCKER-STAT ", log)
                self.assertNotIn(f"SUDO -n -- {self.fixed_docker} --version", log)
                self.assertNotIn(f"SUDO -n -- {self.fixed_docker} info", log)
                self.assertNotIn("SECURE-DOCKER", log)
                self.assertNotIn("DOCKER build", log)

    def test_rejects_unsafe_fixed_buildx_plugin_or_parent(self) -> None:
        for unsafe in (
            "owner",
            "mode",
            "parent-mode",
            "plugin-symlink",
            "parent-symlink",
        ):
            with self.subTest(unsafe=unsafe):
                self.command_log.unlink(missing_ok=True)
                artifact = self.output_dir / f"unsafe-buildx-{unsafe}.json"
                result, _ = self.run_worker(
                    output=artifact,
                    unsafe_buildx=unsafe,
                )

                self.assertNotEqual(0, result.returncode)
                self.assertIn("Plugin Buildx fixe", result.stderr)
                self.assertFalse(artifact.exists())
                self.assert_no_partial_artifact()
                log = self.command_log_text()
                self.assertIn("DOCKER info", log)
                self.assertNotIn("BUILDX build", log)
                self.assertNotIn("DOCKER image inspect", log)
                self.assertNotIn("DOCKER run", log)

    def test_docker_retry_stops_at_the_work_deadline(self) -> None:
        base = int(time.time())
        deadline = base + 3300
        work_deadline = deadline - 1800
        artifact = self.output_dir / "docker-deadline.json"

        result, _ = self.run_worker(
            arguments=[
                "--yes",
                "--gce-deadline-epoch",
                str(deadline),
                "--output",
                str(artifact),
            ],
            output=artifact,
            all_docker_info_fail=True,
            date_trigger_pattern="DOCKER info",
            date_trigger_epoch=work_deadline,
        )

        self.assertNotEqual(0, result.returncode)
        self.assertIn(
            "Deadline de travail atteinte pendant les sondes Docker", result.stderr
        )
        self.assertFalse(artifact.exists())
        self.assert_no_partial_artifact()
        log = self.command_log_text()
        self.assertNotIn("SLEEP", log)
        self.assertEqual(1, log.count("DOCKER info"))
        self.assertNotIn(f"SUDO -n -- {self.fixed_docker} info", log)
        self.assertNotIn("DOCKER build", log)


if __name__ == "__main__":
    unittest.main()
