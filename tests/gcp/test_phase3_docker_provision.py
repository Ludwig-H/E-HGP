#!/usr/bin/env python3
"""Hermetic simulations for the guarded Phase 3 Docker provisioner."""

from __future__ import annotations

import json
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PROVISIONER_SOURCE = ROOT / "gcp-migration" / "phase3_remote_docker_provision.sh"
NOW = 2_000_000_000
GUEST_DEADLINE = NOW + 2_700
GCE_DEADLINE = NOW + 2_820
PROVISION_DEADLINE = NOW + 900
DOCKER_VERSION = "27.5.1-1ubuntu3.1"
BUILDX_VERSION = "0.19.2-0ubuntu1~22.04.1"
NVIDIA_VERSION = "1.17.8-1"


FAKE_MULTICALL = r"""#!/usr/bin/python3 -S
import os
from pathlib import Path
import shutil
import sys

name = Path(sys.argv[0]).name
args = sys.argv[1:]
log = Path(os.environ["FAKE_COMMAND_LOG"])
with log.open("a", encoding="utf-8") as stream:
    stream.write("LOCAL " + name + " " + " ".join(args) + "\n")

if name == "timeout":
    if args == ["--version"]:
        print("timeout (GNU coreutils) 9.4")
        raise SystemExit(0)
    index = 0
    while index < len(args) and (
        args[index] == "--foreground" or args[index].startswith("--kill-after=")
    ):
        index += 1
    if index >= len(args):
        raise SystemExit("fake timeout has no duration")
    index += 1
    command = args[index:]
    if not command:
        raise SystemExit("fake timeout has no command")
    needle = os.environ.get("FAKE_TIMEOUT_ON", "")
    if needle and needle in " ".join(command):
        with log.open("a", encoding="utf-8") as stream:
            stream.write("TIMEOUT-EXPIRED " + " ".join(command) + " 124\n")
        raise SystemExit(124)
    os.execvpe(command[0], command, os.environ.copy())

if name == "date":
    if args != ["+%s"]:
        raise SystemExit("fake date only supports +%s")
    state_path = Path(os.environ["FAKE_DATE_STATE"])
    index = int(state_path.read_text(encoding="utf-8")) if state_path.exists() else 0
    values = [
        int(value)
        for value in os.environ.get("FAKE_DATE_SEQUENCE", "").split(",")
        if value
    ]
    value = values[min(index, len(values) - 1)] if values else int(os.environ["FAKE_NOW"])
    if os.environ.get("FAKE_DATE_MODE") == "unit-margin":
        state_file = Path(os.environ["FAKE_STATE_FILE"])
        if state_file.exists() and '"pre_apt_checked": true' in state_file.read_text(encoding="utf-8"):
            value = int(os.environ["FAKE_PROVISION_DEADLINE"]) - 197
    state_path.write_text(str(index + 1), encoding="utf-8")
    print(value)
    raise SystemExit(0)

if name == "mktemp":
    if len(args) == 2 and args[0] == "-d" and args[1].endswith("XXXXXXXX"):
        path = Path(args[1][:-8] + "A1b2C3d4")
        path.mkdir(mode=0o700, parents=False, exist_ok=False)
        print(path)
        raise SystemExit(0)
    raise SystemExit("unexpected direct fake mktemp")

if name == "tail":
    os.execv("/usr/bin/tail", ["/usr/bin/tail", *args])

if name == "chmod":
    if len(args) != 2:
        raise SystemExit("unexpected fake chmod")
    path = Path(args[1]).resolve()
    root = Path(os.environ["FAKE_TEST_ROOT"]).resolve()
    if root not in path.parents:
        raise SystemExit("fake chmod escaped test root")
    path.chmod(int(args[0], 8))
    raise SystemExit(0)

if name == "rm":
    targets = [Path(item) for item in args if not item.startswith("-")]
    root = Path(os.environ["FAKE_TEST_ROOT"]).resolve()
    for target in targets:
        resolved = target.resolve(strict=False)
        if root not in resolved.parents:
            raise SystemExit("fake rm escaped test root")
        if target.is_dir() and not target.is_symlink():
            shutil.rmtree(target, ignore_errors=True)
        else:
            target.unlink(missing_ok=True)
    raise SystemExit(0)

if name == "stat":
    fmt = "%n|%u|%a"
    paths = []
    index = 0
    while index < len(args):
        item = args[index]
        if item in {"-c", "-Lc"}:
            fmt = args[index + 1]
            index += 2
        elif item == "--":
            paths.extend(args[index + 1:])
            break
        elif item.startswith("-"):
            index += 1
        else:
            paths.append(item)
            index += 1
    for path in paths:
        rendered = fmt.replace("%n", path).replace("%u", "0").replace("%a", "755")
        rendered = rendered.replace("%s", "128").replace("%d", "1").replace("%i", "1")
        print(rendered)
    raise SystemExit(0)

if name == "docker-buildx":
    if args != ["version"]:
        raise SystemExit("unexpected buildx invocation")
    print("github.com/docker/buildx v0.19.2 fake")
    raise SystemExit(0)

raise SystemExit("unexpected direct fake tool: " + name)
"""


FAKE_SUDO = r"""#!/usr/bin/python3 -S
import json
import os
from pathlib import Path
import sys

log_path = Path(os.environ["FAKE_COMMAND_LOG"])
state_path = Path(os.environ["FAKE_STATE_FILE"])
args = sys.argv[1:]

def log(message):
    with log_path.open("a", encoding="utf-8") as stream:
        stream.write(message + "\n")

log("SUDO " + " ".join(args))
if not args or args[0] != "-n":
    raise SystemExit("fake sudo requires -n")
command = args[1:]
if command[:1] == ["--"]:
    command = command[1:]
if not command:
    raise SystemExit("fake sudo has no command")

def approved_config():
    return json.dumps({
        "runtimes": {
            "nvidia": {
                "args": [],
                "path": "/usr/bin/nvidia-container-runtime",
            }
        }
    })

def initial_config(kind):
    if kind == "approved":
        return approved_config()
    if kind == "ambiguous":
        return json.dumps({"registry-mirrors": ["https://unexpected.invalid"]})
    if kind == "relative":
        return json.dumps({"runtimes": {"nvidia": {"args": [], "path": "nvidia-container-runtime"}}})
    if kind == "duplicate":
        return '{"runtimes":{"nvidia":{"args":[],"path":"/usr/bin/nvidia-container-runtime"}},"runtimes":{}}'
    return ""

def initialize():
    ready = os.environ.get("FAKE_READY") == "1"
    initial_missing = os.environ.get("FAKE_INITIAL_MISSING", "")
    partially_ready = initial_missing in {"docker.io", "docker-buildx"}
    config_kind = os.environ.get(
        "FAKE_CONFIG", "approved" if ready or partially_ready else "absent"
    )
    docker_present = ready or (partially_ready and initial_missing != "docker.io")
    files = {}
    if config_kind not in {"absent", "symlink"}:
        files["/etc/docker/daemon.json"] = {
            "content": initial_config(config_kind),
            "inode": 20,
            "mode": "644",
            "owner": "0",
        }
    return {
        "active": docker_present,
        "apt_completed": False,
        "apt_updated": False,
        "boot_reads": 0,
        "configured": config_kind == "approved",
        "docker_dir": ready or partially_ready or config_kind != "absent",
        "enabled": docker_present,
        "files": files,
        "guard_reads": 0,
        "installed": ready or partially_ready,
        "next_inode": 100,
        "pre_apt_checked": False,
        "root_dirs": {},
        "simulated": False,
    }

state = json.loads(state_path.read_text(encoding="utf-8")) if state_path.exists() else initialize()

def save():
    state_path.write_text(json.dumps(state, sort_keys=True), encoding="utf-8")

tool_dir = Path(os.environ["FAKE_TOOL_DIR"])
tool_files = {str(path) for path in tool_dir.iterdir() if path.is_file()}
plugin = os.environ["FAKE_BUILDX_PLUGIN"]
tool_files.add(plugin)

base_directories = {
    "/", "/bin", "/etc", "/etc/systemd", "/etc/systemd/system",
    "/lib", "/lib/systemd", "/lib/systemd/system", "/run",
    "/run/systemd", "/run/systemd/system", "/sbin", "/usr", "/usr/bin",
    "/usr/lib", "/usr/lib/systemd", "/usr/lib/systemd/system",
    "/usr/libexec", "/usr/libexec/docker", "/usr/libexec/docker/cli-plugins",
}

always_executables = {
    "/usr/bin/apt-cache", "/usr/bin/apt-get", "/usr/bin/cat", "/usr/bin/chmod",
    "/usr/bin/date", "/usr/bin/dpkg", "/usr/bin/dpkg-query", "/usr/bin/env",
    "/usr/bin/install", "/usr/bin/journalctl", "/usr/bin/ln", "/usr/bin/mktemp",
    "/usr/bin/nvidia-container-runtime", "/usr/bin/nvidia-ctk", "/usr/bin/python3",
    "/usr/bin/readlink", "/usr/bin/rm", "/usr/bin/stat", "/usr/bin/sudo",
    "/usr/bin/systemctl", "/usr/bin/tail", "/usr/bin/test", "/usr/bin/timeout",
    "/usr/bin/kmod",
}

unit_paths = {
    "/lib/systemd/system/docker.service": "/usr/lib/systemd/system/docker.service",
    "/lib/systemd/system/docker.socket": "/usr/lib/systemd/system/docker.socket",
    "/lib/systemd/system/containerd.service": "/usr/lib/systemd/system/containerd.service",
}

shadow_paths = {
    "/etc/systemd/system/docker.service", "/etc/systemd/system/docker.service.d",
    "/etc/systemd/system/docker.socket", "/etc/systemd/system/docker.socket.d",
    "/etc/systemd/system/containerd.service", "/etc/systemd/system/containerd.service.d",
    "/run/systemd/system/docker.service", "/run/systemd/system/docker.service.d",
    "/run/systemd/system/docker.socket", "/run/systemd/system/docker.socket.d",
    "/run/systemd/system/containerd.service", "/run/systemd/system/containerd.service.d",
}

def unsafe_kind(path):
    if path == os.environ.get("FAKE_UNSAFE_PATH"):
        return os.environ.get("FAKE_UNSAFE_KIND", "")
    if path == plugin:
        return os.environ.get("FAKE_BUILDX_UNSAFE", "")
    if path == "/etc/docker":
        return os.environ.get("FAKE_DOCKER_PARENT_HAZARD", "")
    return ""

def config_symlink():
    if os.environ.get("FAKE_CONFIG") == "symlink":
        return True
    return os.environ.get("FAKE_CONFIG_RACE") == "symlink-after-apt" and state["installed"]

def is_symlink(path):
    if unsafe_kind(path) == "symlink":
        return True
    if path == "/usr/bin/python3":
        return os.environ.get("FAKE_PYTHON_ALIAS_STATE", "symlink") == "symlink"
    if path == "/etc/docker/daemon.json":
        return config_symlink()
    docker_available = not (
        os.environ.get("FAKE_INITIAL_MISSING") == "docker.io"
        and not state.get("apt_completed", False)
    )
    if state["installed"] and docker_available and path in unit_paths:
        return True
    if path in {"/bin/kmod", "/sbin/modprobe"}:
        return True
    return False

def directory_exists(path):
    if path == "/etc/docker":
        return state["docker_dir"] and not is_symlink(path)
    if path in state["root_dirs"]:
        return True
    if path in base_directories:
        return True
    candidate = Path(path)
    return any(candidate == Path(item).parent or candidate in Path(item).parents for item in tool_files)

def file_exists(path):
    if unsafe_kind(path) == "missing":
        return False
    if (
        path == "/usr/bin/python3"
        and os.environ.get("FAKE_PYTHON_ALIAS_STATE") == "missing"
    ):
        return False
    initial_missing = os.environ.get("FAKE_INITIAL_MISSING", "")
    before_install = not state.get("apt_completed", False)
    if before_install and initial_missing == "docker-buildx" and path == plugin:
        return False
    if before_install and initial_missing == "docker.io" and path in {
        "/usr/bin/docker",
        "/usr/bin/dockerd",
        *unit_paths.keys(),
        *unit_paths.values(),
    }:
        return False
    if path in state["files"]:
        return True
    if path in tool_files or path in always_executables:
        return True
    if state["installed"] and path in {
        "/usr/bin/docker", "/usr/bin/dockerd", "/usr/bin/containerd", plugin,
        *unit_paths.keys(), *unit_paths.values(), "/bin/kmod", "/sbin/modprobe",
    }:
        return True
    hazard = os.environ.get("FAKE_SYSTEMD_PREEXISTING", "")
    if hazard and path.endswith(hazard):
        return True
    return False

def exists(path):
    return is_symlink(path) or directory_exists(path) or file_exists(path)

def metadata(path):
    kind = unsafe_kind(path)
    owner = "1000" if kind == "owner" else "0"
    if path in state["files"]:
        record = state["files"][path]
        mode = record["mode"]
        inode = record["inode"]
        size = len(record["content"].encode("utf-8"))
    elif path in state["root_dirs"]:
        record = state["root_dirs"][path]
        mode = record["mode"]
        inode = record["inode"]
        size = 0
    elif directory_exists(path):
        mode, inode, size = "755", 1, 0
    elif path in unit_paths.values():
        mode, inode, size = "644", 40, 256
    else:
        mode, inode, size = "755", 2, 128
    if kind == "mode":
        mode = "775"
    return owner, mode, "1", str(inode), str(size)

def render_stat(fmt, path):
    owner, mode, device, inode, size = metadata(path)
    value = fmt
    for token, replacement in {
        "%n": path, "%u": owner, "%a": mode, "%d": device,
        "%i": inode, "%s": size,
    }.items():
        value = value.replace(token, replacement)
    return value

def allocated_inode():
    inode = state["next_inode"]
    state["next_inode"] += 1
    return inode

def finish(code=0):
    save()
    raise SystemExit(code)

def package_record(package):
    partial = os.environ.get("FAKE_PARTIAL_PACKAGE", "")
    if package == partial:
        return "install reinstreq half-installed", "0.invalid"
    if package in {
        "nvidia-container-toolkit", "nvidia-container-toolkit-base",
        "libnvidia-container-tools", "libnvidia-container1",
    }:
        return "install ok installed", os.environ["FAKE_NVIDIA_VERSION"]
    if package in {"docker.io", "docker-buildx", "containerd"} and state["installed"]:
        if (
            package == os.environ.get("FAKE_INITIAL_MISSING", "")
            and not state.get("apt_completed", False)
        ):
            return None
        versions = {
            "docker.io": os.environ["FAKE_DOCKER_VERSION"],
            "docker-buildx": os.environ["FAKE_BUILDX_VERSION"],
            "containerd": "1.7.24-0ubuntu1~22.04.1",
        }
        return "install ok installed", versions[package]
    if package == os.environ.get("FAKE_COMPETING_PACKAGE", ""):
        return "install ok installed", "99.0-conflicting"
    if package == "podman-docker":
        return (
            os.environ.get("FAKE_ABSENT_STATUS", "unknown ok not-installed"),
            os.environ.get("FAKE_ABSENT_VERSION", ""),
        )
    return None

name = Path(command[0]).name
arguments = command[1:]

if name == "test":
    negate = False
    if arguments[:1] == ["!"]:
        negate = True
        arguments = arguments[1:]
    if len(arguments) != 2 or arguments[0] not in {"-L", "-e", "-d", "-f", "-x"}:
        raise SystemExit("unexpected fake test invocation: " + " ".join(command))
    operation, path = arguments
    outcomes = {
        "-L": is_symlink(path),
        "-e": exists(path),
        "-d": directory_exists(path),
        "-f": file_exists(path),
        "-x": file_exists(path),
    }
    outcome = outcomes[operation]
    if negate:
        outcome = not outcome
    finish(0 if outcome else 1)

if name == "stat":
    fmt = "%n|%u|%a"
    paths = []
    index = 0
    while index < len(arguments):
        item = arguments[index]
        if item in {"-c", "-Lc"}:
            fmt = arguments[index + 1]
            index += 2
        elif item == "--":
            paths.extend(arguments[index + 1:])
            break
        elif item.startswith("-"):
            index += 1
        else:
            paths.append(item)
            index += 1
    if not paths or any(not exists(path) for path in paths):
        finish(1)
    for path in paths:
        print(render_stat(fmt, path))
    finish()

if name == "cat":
    if arguments == ["/run/systemd/shutdown/scheduled"]:
        state["guard_reads"] += 1
        guard = os.environ.get("FAKE_GUARD", "present")
        if guard == "missing" or (
            guard == "lost-after-install" and state["installed"]
        ):
            finish(1)
        offsets = {"too-soon": 60, "too-far": 3_000, "expired": -1}
        deadline = int(os.environ["FAKE_NOW"]) + offsets.get(guard, 2_700)
        mode = "halt" if guard == "halt" else "poweroff"
        if guard == "duplicate":
            print(f"MODE={mode}\nMODE={mode}\nUSEC={deadline * 1_000_000}")
        else:
            print(f"MODE={mode}\nUSEC={deadline * 1_000_000}")
        finish()
    if arguments == ["/proc/sys/kernel/random/boot_id"]:
        state["boot_reads"] += 1
        if os.environ.get("FAKE_BOOT_CHANGE_AFTER_INSTALL") == "1" and state["installed"]:
            print("99999999-9999-4999-8999-999999999999")
        else:
            print("11111111-1111-4111-8111-111111111111")
        finish()
    if arguments == ["/etc/os-release"]:
        print('ID="' + os.environ.get("FAKE_OS_ID", "ubuntu") + '"')
        print('VERSION_ID="' + os.environ.get("FAKE_OS_VERSION", "22.04") + '"')
        finish()
    raise SystemExit("unexpected fake cat invocation: " + " ".join(arguments))

if name == "dpkg":
    if arguments == ["--print-architecture"]:
        print(os.environ.get("FAKE_ARCHITECTURE", "amd64"))
        finish()
    if arguments == ["--audit"]:
        state["pre_apt_checked"] = True
        audit = os.environ.get("FAKE_DPKG_AUDIT", "")
        if audit:
            print(audit)
        finish()
    raise SystemExit("unexpected fake dpkg invocation: " + " ".join(arguments))

if name == "dpkg-query":
    if arguments[:1] == ["-S"] and len(arguments) == 2:
        path = arguments[1]
        owners = {
            plugin: "docker-buildx",
            "/bin/kmod": "kmod",
            "/sbin/modprobe": "kmod",
            "/lib/systemd/system/docker.service": "docker.io",
            "/lib/systemd/system/docker.socket": "docker.io",
            "/lib/systemd/system/containerd.service": "containerd",
        }
        owner = owners.get(path)
        if owner is None or (
            owner in {"docker.io", "docker-buildx", "containerd"}
            and package_record(owner) is None
        ):
            finish(1)
        print(f"{owner}: {path}")
        finish()
    if arguments[:1] == ["-W"]:
        fmt = next((item for item in arguments if item.startswith("-f=")), "")
        packages = [item for item in arguments[1:] if not item.startswith("-f=")]
        diagnostic = "${binary:Package}" in fmt
        emitted = False
        for package in packages:
            record = package_record(package)
            if record is None:
                continue
            emitted = True
            status, version = record
            if diagnostic:
                print(f"{package}\t{status}\t{version}")
            else:
                print(f"{status}\t{version}")
        finish(0 if diagnostic or emitted else 1)
    raise SystemExit("unexpected fake dpkg-query invocation: " + " ".join(arguments))

if name in {"python3", "python3.10"}:
    if not arguments or arguments[-2:-1] != ["-"]:
        raise SystemExit("unexpected privileged Python invocation: " + " ".join(arguments))
    path = arguments[-1]
    record = state["files"].get(path)
    if record is None:
        finish(1)
    try:
        def strict_pairs(pairs):
            value = {}
            for key, item in pairs:
                if key in value:
                    raise ValueError("duplicate key")
                value[key] = item
            return value
        parsed = json.loads(record["content"], object_pairs_hook=strict_pairs)
        approved = json.loads(approved_config())
    except (json.JSONDecodeError, ValueError):
        finish(1)
    finish(0 if parsed == approved else 1)

if name == "readlink" and arguments[:2] == ["-f", "--"] and len(arguments) == 3:
    path = arguments[2]
    if path in unit_paths:
        print(unit_paths[path])
        finish(0 if state["installed"] else 1)
    if path in {"/bin/kmod", "/sbin/modprobe"}:
        print("/usr/bin/kmod")
        finish(0 if state["installed"] else 1)
    print(path)
    finish(0 if exists(path) else 1)

if name == "systemctl":
    action = arguments[0] if arguments else ""
    unit = arguments[1] if action == "show" and len(arguments) > 1 else arguments[-1]
    failure = os.environ.get("FAKE_SYSTEMD_FAIL", "")
    if action == "show":
        if not state["installed"] or failure == "show":
            finish(1)
        show_all = "--all" in arguments
        bad_dropin = "/etc/systemd/system/docker.service.d/override.conf" if failure == "metadata" else ""
        if unit == "docker.socket":
            print("LoadState=loaded")
            print("FragmentPath=/lib/systemd/system/docker.socket")
            if show_all or bad_dropin:
                print(f"DropInPaths={bad_dropin}")
            print("Listen=/run/docker.sock (Stream)")
            print("SocketUser=root")
            print("SocketGroup=docker")
            print("SocketMode=0660")
            finish()
        if unit == "docker.service":
            executable = "/usr/bin/false" if failure == "metadata" else "/usr/bin/dockerd"
            argv = f"{executable} -H fd:// --containerd=/run/containerd/containerd.sock"
            fragment = "/lib/systemd/system/docker.service"
        elif unit == "containerd.service":
            executable = "/usr/bin/containerd"
            argv = executable
            fragment = "/lib/systemd/system/containerd.service"
        else:
            finish(1)
        print("LoadState=loaded")
        print(f"FragmentPath={fragment}")
        if show_all or bad_dropin:
            print(f"DropInPaths={bad_dropin}")
        print(f"ExecStart={{ path={executable} ; argv[]={argv} ; }}")
        if show_all:
            print("ExecStartPre=")
            print("ExecStartPost=")
            print("ExecCondition=")
            print("ExecStop=")
            print("ExecStopPost=")
            print("Environment=")
            print("EnvironmentFiles=")
        finish()
    if action == "is-enabled":
        if "--quiet" not in arguments:
            print("enabled" if state["enabled"] else "disabled")
        finish(0 if state["enabled"] else 1)
    if action == "is-active":
        if "--quiet" not in arguments:
            print("active" if state["active"] else "inactive")
        finish(0 if state["active"] else 3)
    if action == "enable" and unit == "docker.service":
        if failure == "enable":
            print("simulated systemctl enable failure", file=sys.stderr)
            finish(1)
        state["enabled"] = True
        finish()
    if action == "restart" and unit == "docker.service":
        if failure == "restart":
            print("simulated systemctl restart failure", file=sys.stderr)
            finish(1)
        state["active"] = True
        finish()
    raise SystemExit("unexpected fake systemctl invocation: " + " ".join(arguments))

if name == "journalctl":
    lines = 80
    for item in arguments:
        if item.startswith("--lines="):
            lines = min(int(item.split("=", 1)[1]), 80)
    for index in range(lines):
        print(f"fake docker journal line {index + 1:03d}")
    finish()

if name == "env":
    payload = list(arguments)
    while payload and "=" in payload[0] and not payload[0].startswith("/"):
        payload.pop(0)
    if not payload:
        raise SystemExit("fake env has no command")
    command = payload
    name = Path(command[0]).name
    arguments = command[1:]

if name == "apt-cache":
    if len(arguments) != 2 or arguments[0] not in {"policy", "madison"}:
        raise SystemExit("unexpected fake apt-cache invocation: " + " ".join(arguments))
    package = arguments[1]
    versions = {
        "docker.io": os.environ["FAKE_DOCKER_VERSION"],
        "docker-buildx": os.environ["FAKE_BUILDX_VERSION"],
    }
    version = versions[package]
    if arguments[0] == "policy":
        print(f"{package}:")
        print("  Installed: (none)")
        print(f"  Candidate: {version}")
        print("  Version table:")
        print(f" *** {version} 500")
        finish()
    print(f" {package} | {version} | http://archive.ubuntu.com/ubuntu jammy-updates/universe amd64 Packages")
    finish()

if name == "apt-get":
    if "update" in arguments:
        state["apt_updated"] = True
        print("fake apt index updated")
        finish()
    if "install" not in arguments:
        raise SystemExit("unexpected fake apt-get invocation: " + " ".join(arguments))
    if "--simulate" in arguments:
        plan = os.environ.get("FAKE_APT_PLAN", "safe")
        if plan == "removal":
            print("Remv docker-ce [99.0]")
        elif plan == "upgrade":
            print("Inst libc6 [2.35-0ubuntu3] (2.35-0ubuntu3.10 Ubuntu:22.04/jammy-updates [amd64])")
        else:
            print(f"Inst docker.io ({os.environ['FAKE_DOCKER_VERSION']} Ubuntu:22.04/jammy-updates [amd64])")
            print(f"Inst docker-buildx ({os.environ['FAKE_BUILDX_VERSION']} Ubuntu:22.04/jammy-updates [amd64])")
            print("Inst containerd (1.7.24-0ubuntu1~22.04.1 Ubuntu:22.04/jammy-updates [amd64])")
        state["simulated"] = True
        finish()
    versions = {
        "docker.io": os.environ["FAKE_DOCKER_VERSION"],
        "docker-buildx": os.environ["FAKE_BUILDX_VERSION"],
    }
    initial_missing = os.environ.get("FAKE_INITIAL_MISSING", "")
    expected_names = {initial_missing} if initial_missing else set(versions)
    expected = {f"{package}={versions[package]}" for package in expected_names}
    required_flags = {"--yes", "--no-remove", "--no-upgrade", "--no-install-recommends"}
    if not state["apt_updated"] or not state["simulated"]:
        raise SystemExit("real fake install was attempted before update/simulation")
    if not expected.issubset(arguments) or not required_flags.issubset(arguments):
        raise SystemExit("fake install is not exactly pinned and guarded")
    state["apt_completed"] = True
    state["installed"] = True
    print("fake pinned Docker packages installed")
    finish()

if name == "mktemp":
    if arguments[:1] == ["-d"] and len(arguments) == 2:
        path = arguments[1][:-8] + "R0o7T3mP"
        state["root_dirs"][path] = {
            "inode": allocated_inode(), "mode": "700", "owner": "0",
        }
        print(path)
        finish()
    if len(arguments) == 1 and arguments[0].endswith("XXXXXXXX"):
        path = arguments[0][:-8] + "S7a8G9e0"
        state["files"][path] = {
            "content": "", "inode": allocated_inode(), "mode": "600", "owner": "0",
        }
        print(path)
        finish()
    raise SystemExit("unexpected privileged mktemp invocation: " + " ".join(arguments))

if name == "install":
    if "-d" in arguments:
        destination = arguments[-1]
        if destination != "/etc/docker":
            raise SystemExit("fake install -d escaped /etc/docker")
        state["docker_dir"] = True
        finish()
    if len(arguments) < 2:
        raise SystemExit("unexpected fake install invocation")
    source, destination = arguments[-2:]
    source_record = state["files"].get(source)
    if source_record is None:
        finish(1)
    state["files"][destination] = {
        "content": source_record["content"],
        "inode": allocated_inode(),
        "mode": "644",
        "owner": "0",
    }
    finish()

if name == "nvidia-ctk":
    if arguments[:2] != ["runtime", "configure"]:
        raise SystemExit("unexpected fake nvidia-ctk invocation: " + " ".join(arguments))
    config_argument = next((item for item in arguments if item.startswith("--config=")), "")
    if not config_argument:
        finish(1)
    destination = config_argument.split("=", 1)[1]
    state["files"][destination] = {
        "content": approved_config(), "inode": allocated_inode(), "mode": "644", "owner": "0",
    }
    finish()

if name == "dockerd":
    if arguments[:1] != ["--validate"]:
        raise SystemExit("unexpected fake dockerd invocation: " + " ".join(arguments))
    path_argument = next((item for item in arguments if item.startswith("--config-file=")), "")
    if not path_argument:
        finish(1)
    path = path_argument.split("=", 1)[1]
    record = state["files"].get(path)
    if os.environ.get("FAKE_DOCKERD_FAIL") == path or record is None:
        finish(1)
    try:
        parsed = json.loads(record["content"])
    except json.JSONDecodeError:
        finish(1)
    finish(0 if parsed == json.loads(approved_config()) else 1)

if name == "ln":
    if len(arguments) != 4 or arguments[:2] != ["-T", "--"]:
        raise SystemExit("unexpected fake ln invocation: " + " ".join(arguments))
    source, destination = arguments[2:]
    if destination in state["files"] or is_symlink(destination):
        finish(1)
    source_record = state["files"].get(source)
    if source_record is None:
        finish(1)
    state["files"][destination] = dict(source_record)
    state["configured"] = True
    finish()

if name == "rm":
    targets = [item for item in arguments if not item.startswith("-")]
    for target in targets:
        state["files"].pop(target, None)
        state["root_dirs"].pop(target, None)
        for path in list(state["files"]):
            if path.startswith(target.rstrip("/") + "/"):
                state["files"].pop(path, None)
    finish()

if name == "docker":
    if not state["installed"]:
        finish(127)
    if arguments == ["--version"]:
        print("Docker version 27.5.1, build fake")
        finish()
    if arguments[:2] == ["info", "--format"]:
        if not state["active"]:
            finish(1)
        runtime = {
            "path": os.environ.get(
                "FAKE_RUNTIME_PATH", "/usr/bin/nvidia-container-runtime"
            )
        }
        if "FAKE_RUNTIME_ARGS" in os.environ:
            runtime["runtimeArgs"] = (
                []
                if not os.environ["FAKE_RUNTIME_ARGS"]
                else [os.environ["FAKE_RUNTIME_ARGS"]]
            )
        print(
            json.dumps(
                {
                    "nvidia": runtime
                }
            )
        )
        finish()
    raise SystemExit("unexpected fake docker invocation: " + " ".join(arguments))

raise SystemExit("unexpected fake sudo command: " + " ".join(command))
"""


class Phase3DockerProvisionerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(
            prefix="phase3-docker-provision-test-"
        )
        self.root = Path(self.temporary.name)
        self.tool_dir = self.root / "tools"
        self.tool_dir.mkdir()
        self.tmp_dir = self.root / "tmp"
        self.tmp_dir.mkdir()
        self.run_index = 0

        for name in (
            "timeout",
            "date",
            "mktemp",
            "tail",
            "rm",
            "chmod",
            "stat",
            "test",
            "cat",
        ):
            self.write_executable(self.tool_dir / name, FAKE_MULTICALL)
        self.sudo = self.tool_dir / "sudo"
        self.write_executable(self.sudo, FAKE_SUDO)
        self.python = self.tool_dir / "python3.10"
        self.write_executable(
            self.python,
            """#!/bin/sh
if [ "${FAKE_PYTHON_MODE:-ok}" = "broken" ]; then
    exit 97
fi
if [ "${FAKE_PYTHON_MODE:-ok}" = "wrong-version" ]; then
    printf '%s\\n' '3.9'
    exit 0
fi
exec /usr/bin/python3 "$@"
""",
        )
        self.buildx = self.tool_dir / "docker-buildx"
        self.write_executable(self.buildx, FAKE_MULTICALL)

        replacements = {
            'readonly BUILDX_PLUGIN="/usr/libexec/docker/cli-plugins/docker-buildx"': (
                f'readonly BUILDX_PLUGIN="{self.buildx}"'
            ),
            'readonly SUDO_BIN="/usr/bin/sudo"': f'readonly SUDO_BIN="{self.sudo}"',
            'readonly TIMEOUT_BIN="/usr/bin/timeout"': (
                f'readonly TIMEOUT_BIN="{self.tool_dir / "timeout"}"'
            ),
            'readonly DATE_BIN="/usr/bin/date"': f'readonly DATE_BIN="{self.tool_dir / "date"}"',
            'readonly MKTEMP_BIN="/usr/bin/mktemp"': (
                f'readonly MKTEMP_BIN="{self.tool_dir / "mktemp"}"'
            ),
            'readonly TAIL_BIN="/usr/bin/tail"': f'readonly TAIL_BIN="{self.tool_dir / "tail"}"',
            'readonly RM_BIN="/usr/bin/rm"': f'readonly RM_BIN="{self.tool_dir / "rm"}"',
            'readonly CHMOD_BIN="/usr/bin/chmod"': (
                f'readonly CHMOD_BIN="{self.tool_dir / "chmod"}"'
            ),
            'readonly STAT_BIN="/usr/bin/stat"': f'readonly STAT_BIN="{self.tool_dir / "stat"}"',
            'readonly TEST_BIN="/usr/bin/test"': f'readonly TEST_BIN="{self.tool_dir / "test"}"',
            'readonly CAT_BIN="/usr/bin/cat"': f'readonly CAT_BIN="{self.tool_dir / "cat"}"',
            'readonly PYTHON_BIN="/usr/bin/python3.10"': (
                f'readonly PYTHON_BIN="{self.python}"'
            ),
            'readonly EXPECTED_PYTHON_MAJOR_MINOR="3.10"': (
                f'readonly EXPECTED_PYTHON_MAJOR_MINOR="{sys.version_info.major}.{sys.version_info.minor}"'
            ),
        }
        source = PROVISIONER_SOURCE.read_text(encoding="utf-8")
        for original, replacement in replacements.items():
            self.assertEqual(1, source.count(original), original)
            source = source.replace(original, replacement)
        bootstrap_prefix = '[[ "${candidate}" == /usr/bin/* && -f "${candidate}" && ! -L "${candidate}" && \\\n'
        fake_bootstrap_prefix = (
            f'[[ "${{candidate}}" == {self.tool_dir}/* && -f "${{candidate}}" && '
            f'! -L "${{candidate}}" && \\\n'
        )
        self.assertEqual(1, source.count(bootstrap_prefix))
        source = source.replace(bootstrap_prefix, fake_bootstrap_prefix)
        bootstrap_stat = "metadata=\"$(/usr/bin/stat -Lc '%n|%u|%a' -- \\\n"
        fake_bootstrap_stat = (
            f'metadata="$({self.tool_dir / "stat"} -Lc \'%n|%u|%a\' -- \\\n'
        )
        self.assertEqual(1, source.count(bootstrap_stat))
        source = source.replace(bootstrap_stat, fake_bootstrap_stat)
        fixed_prefix = (
            '[[ "${candidate}" == /usr/bin/* && "${candidate}" != *$\'\\n\'* && \\\n'
        )
        fake_fixed_prefix = (
            f'[[ ("${{candidate}}" == /usr/bin/* || "${{candidate}}" == "{self.python}") '
            "&& \"${candidate}\" != *$'\\n'* && \\\n"
        )
        self.assertEqual(1, source.count(fixed_prefix))
        source = source.replace(fixed_prefix, fake_fixed_prefix)
        self.provisioner = self.root / "phase3_remote_docker_provision.sh"
        self.write_executable(self.provisioner, source)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    @staticmethod
    def write_executable(path: Path, content: str) -> None:
        path.write_text(content, encoding="utf-8")
        path.chmod(path.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    def run_provisioner(
        self,
        *,
        arguments: list[str] | None = None,
        environment: dict[str, str] | None = None,
    ) -> tuple[subprocess.CompletedProcess[str], str, dict[str, object]]:
        self.run_index += 1
        run_root = self.root / f"run-{self.run_index}"
        run_root.mkdir()
        command_log = run_root / "commands.log"
        state_file = run_root / "state.json"
        date_state = run_root / "date-state"
        env = os.environ.copy()
        env.update(
            {
                "FAKE_BUILDX_PLUGIN": str(self.buildx),
                "FAKE_BUILDX_VERSION": BUILDX_VERSION,
                "FAKE_COMMAND_LOG": str(command_log),
                "FAKE_DATE_STATE": str(date_state),
                "FAKE_DOCKER_VERSION": DOCKER_VERSION,
                "FAKE_NVIDIA_VERSION": NVIDIA_VERSION,
                "FAKE_NOW": str(NOW),
                "FAKE_PROVISION_DEADLINE": str(PROVISION_DEADLINE),
                "FAKE_STATE_FILE": str(state_file),
                "FAKE_TEST_ROOT": str(self.root),
                "FAKE_TOOL_DIR": str(self.tool_dir),
                "PYTHONDONTWRITEBYTECODE": "1",
                "TMPDIR": str(self.tmp_dir),
            }
        )
        if environment:
            env.update(environment)
        invocation = arguments or [
            "--yes",
            "--gce-deadline-epoch",
            str(GCE_DEADLINE),
        ]
        result = subprocess.run(
            [str(self.provisioner), *invocation],
            cwd=self.root,
            env=env,
            text=True,
            capture_output=True,
            timeout=120,
            check=False,
        )
        log = command_log.read_text(encoding="utf-8") if command_log.exists() else ""
        state = (
            json.loads(state_file.read_text(encoding="utf-8"))
            if state_file.exists()
            else {}
        )
        return result, log, state

    def assert_no_host_mutation(self, log: str) -> None:
        mutation_markers = (
            "/usr/bin/env LC_ALL=C DEBIAN_FRONTEND=noninteractive /usr/bin/apt-get ",
            "nvidia-ctk runtime configure",
            "systemctl enable docker.service",
            "systemctl restart docker.service",
            "/usr/bin/install -d ",
            "/usr/bin/ln -T ",
        )
        for line in log.splitlines():
            for marker in mutation_markers:
                self.assertNotIn(marker, line)

    @staticmethod
    def sudo_command_lines(log: str) -> list[str]:
        return [line for line in log.splitlines() if line.startswith("SUDO ")]

    def test_idempotent_ready_host_performs_no_mutation(self) -> None:
        result, log, state = self.run_provisioner(
            environment={
                "FAKE_READY": "1",
                "FAKE_PYTHON_ALIAS_STATE": "missing",
            }
        )

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertIn("aucune mutation hôte", result.stdout)
        self.assert_no_host_mutation(log)
        self.assertTrue(state["installed"])
        self.assertTrue(state["active"])
        self.assertTrue(state["enabled"])
        self.assertIn("docker --version", log)
        self.assertIn("docker info --format", log)
        self.assertIn(f"{self.python} -I -S", log)
        self.assertNotIn("/usr/bin/python3 -I -S", log)
        self.assertFalse(
            any(
                re.search(
                    r"(?:^| )(?:docker|/usr/bin/docker) (?:run|pull|build)(?: |$)", line
                )
                for line in self.sudo_command_lines(log)
            )
        )

        mismatched, _, _ = self.run_provisioner(
            environment={
                "FAKE_READY": "1",
                "FAKE_RUNTIME_PATH": "/usr/bin/unapproved-runtime",
            }
        )
        self.assertNotEqual(0, mismatched.returncode)
        self.assertIn("runtime NVIDIA ne sont pas certifiés", mismatched.stderr)

        runtime_args, _, _ = self.run_provisioner(
            environment={
                "FAKE_READY": "1",
                "FAKE_RUNTIME_ARGS": "--unapproved",
            }
        )
        self.assertNotEqual(0, runtime_args.returncode)
        self.assertIn("runtime NVIDIA ne sont pas certifiés", runtime_args.stderr)

    def test_source_uses_jammy_versioned_python_entrypoint(self) -> None:
        source = PROVISIONER_SOURCE.read_text(encoding="utf-8")

        self.assertIn('readonly PYTHON_BIN="/usr/bin/python3.10"', source)
        self.assertEqual(6, source.count('"${PYTHON_BIN}"'))
        self.assertNotIn("/usr/bin/python3 -I", source)
        self.assertEqual(2, source.count('"${unit}" --no-pager --all \\\n'))
        self.assertIn("missing={missing}; unexpected={unexpected}", source)

    def test_rejects_unusable_or_wrong_version_python_before_mutation(self) -> None:
        for mode in ("broken", "wrong-version"):
            with self.subTest(mode=mode):
                result, log, _ = self.run_provisioner(
                    environment={"FAKE_PYTHON_MODE": mode}
                )
                self.assertNotEqual(0, result.returncode)
                self.assertIn("Python", result.stderr)
                self.assert_no_host_mutation(log)

    def test_rejects_missing_or_invalid_guard_and_deadline_before_mutation(
        self,
    ) -> None:
        cases = (
            ("missing", None, "Arrêt invité"),
            ("halt", None, "Arrêt invité"),
            ("too-soon", None, "Arrêt invité"),
            (None, ["--yes", "--gce-deadline-epoch", "not-an-epoch"], "epoch UTC"),
        )
        for guard, arguments, message in cases:
            with self.subTest(guard=guard, arguments=arguments):
                environment = {"FAKE_GUARD": guard} if guard else None
                result, log, _ = self.run_provisioner(
                    arguments=arguments,
                    environment=environment,
                )
                self.assertNotEqual(0, result.returncode)
                self.assertIn(message, result.stderr)
                self.assert_no_host_mutation(log)

    def test_rejects_unsafe_owner_mode_or_symlink_paths(self) -> None:
        cases = (
            ("/usr/bin/nvidia-ctk", "owner"),
            ("/usr/bin/nvidia-container-runtime", "mode"),
            ("/usr/bin/apt-get", "symlink"),
            (str(self.python), "missing"),
            (str(self.python), "symlink"),
            (str(self.python), "owner"),
            (str(self.python), "mode"),
        )
        for path, kind in cases:
            with self.subTest(path=path, kind=kind):
                result, log, _ = self.run_provisioner(
                    environment={"FAKE_UNSAFE_PATH": path, "FAKE_UNSAFE_KIND": kind}
                )
                self.assertNotEqual(0, result.returncode)
                self.assertIn("Chemin système absent ou non sûr", result.stderr)
                self.assert_no_host_mutation(log)

    def test_rejects_competing_docker_family_or_partial_dpkg_state(self) -> None:
        cases = (
            ({"FAKE_COMPETING_PACKAGE": "docker-ce"}, "Famille Docker concurrente"),
            ({"FAKE_PARTIAL_PACKAGE": "docker.io"}, "État dpkg partiel ou ambigu"),
            ({"FAKE_ABSENT_VERSION": "1.invalid"}, "État dpkg partiel ou ambigu"),
            (
                {"FAKE_ABSENT_STATUS": "deinstall ok config-files"},
                "État dpkg partiel ou ambigu",
            ),
        )
        for environment, message in cases:
            with self.subTest(environment=environment):
                result, log, _ = self.run_provisioner(environment=environment)
                self.assertNotEqual(0, result.returncode)
                self.assertIn(message, result.stderr)
                self.assert_no_host_mutation(log)

    def test_rejects_apt_plan_removal_or_upgrade(self) -> None:
        for plan, message in (("removal", "suppression"), ("upgrade", "mise à niveau")):
            with self.subTest(plan=plan):
                result, log, _ = self.run_provisioner(
                    environment={"FAKE_APT_PLAN": plan}
                )
                self.assertNotEqual(0, result.returncode)
                self.assertIn(message, result.stderr)
                sudo_lines = self.sudo_command_lines(log)
                simulation_lines = [
                    line
                    for line in sudo_lines
                    if "apt-get" in line and "--simulate" in line
                ]
                install_lines = [
                    line
                    for line in sudo_lines
                    if "apt-get" in line
                    and " install " in f" {line} "
                    and "--simulate" not in line
                ]
                self.assertEqual(1, len(simulation_lines), log)
                self.assertEqual([], install_lines, log)
                self.assertNotIn("nvidia-ctk runtime configure", log)

    def test_installs_exact_pins_in_order_and_configures_successfully(self) -> None:
        result, log, state = self.run_provisioner()

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertIn("[SUCCÈS] Préparation Docker Phase 3", result.stdout)
        lines = log.splitlines()
        update = next(
            index
            for index, line in enumerate(lines)
            if "apt-get" in line and " update" in line
        )
        simulation = next(
            index
            for index, line in enumerate(lines)
            if "apt-get" in line and "--simulate" in line
        )
        installation = next(
            index
            for index, line in enumerate(lines)
            if "apt-get" in line
            and " install " in f" {line} "
            and "--simulate" not in line
        )
        configure = next(
            index
            for index, line in enumerate(lines)
            if "nvidia-ctk runtime configure" in line
        )
        enable = next(
            index
            for index, line in enumerate(lines)
            if "systemctl enable docker.service" in line
        )
        restart = next(
            index
            for index, line in enumerate(lines)
            if "systemctl restart docker.service" in line
        )
        self.assertLess(update, simulation)
        self.assertLess(simulation, installation)
        self.assertLess(installation, configure)
        self.assertLess(configure, enable)
        self.assertLess(enable, restart)
        install_line = lines[installation]
        self.assertIn(f"docker.io={DOCKER_VERSION}", install_line)
        self.assertIn(f"docker-buildx={BUILDX_VERSION}", install_line)
        self.assertIn("--no-remove --no-upgrade --no-install-recommends", install_line)
        sudo_lines = self.sudo_command_lines(log)
        self.assertEqual(
            1,
            sum("systemctl enable docker.service" in line for line in sudo_lines),
        )
        self.assertEqual(
            1,
            sum("systemctl restart docker.service" in line for line in sudo_lines),
        )
        self.assertNotRegex(log, r"\bgcloud\b|docker (run|pull|build)")
        self.assertTrue(state["installed"])
        self.assertTrue(state["configured"])
        self.assertTrue(state["enabled"])
        self.assertTrue(state["active"])

        package_versions = {
            "docker.io": DOCKER_VERSION,
            "docker-buildx": BUILDX_VERSION,
        }
        for missing_package, missing_version in package_versions.items():
            with self.subTest(missing_package=missing_package):
                partial, partial_log, partial_state = self.run_provisioner(
                    environment={"FAKE_INITIAL_MISSING": missing_package}
                )
                self.assertEqual(0, partial.returncode, partial.stdout + partial.stderr)
                install_lines = [
                    line
                    for line in self.sudo_command_lines(partial_log)
                    if "apt-get" in line
                    and " install " in f" {line} "
                    and "--simulate" not in line
                ]
                self.assertEqual(1, len(install_lines), partial_log)
                self.assertIn(f"{missing_package}={missing_version}", install_lines[0])
                for present_package in package_versions.keys() - {missing_package}:
                    self.assertNotIn(f"{present_package}=", install_lines[0])
                self.assertTrue(partial_state["apt_completed"])
                self.assertTrue(partial_state["active"])

    def test_per_unit_timeout_prevents_all_later_units(self) -> None:
        source = PROVISIONER_SOURCE.read_text(encoding="utf-8")
        self.assertNotIn('"${TIMEOUT_BIN}" --foreground', source)
        result, log, _ = self.run_provisioner(
            environment={"FAKE_TIMEOUT_ON": "nvidia-ctk runtime configure"}
        )

        self.assertNotEqual(0, result.returncode)
        self.assertIn("TIMEOUT-EXPIRED", log)
        self.assertIn("Configuration NVIDIA de Docker échouée", result.stderr)
        self.assertNotIn("dockerd --validate", log)
        self.assertNotIn("systemctl enable", log)
        self.assertNotIn("systemctl restart", log)
        self.assertNotIn("docker info", log)

    def test_rejects_ambiguous_daemon_json_and_bounds_systemd_failure_diagnostics(
        self,
    ) -> None:
        ambiguous, ambiguous_log, _ = self.run_provisioner(
            environment={"FAKE_CONFIG": "ambiguous"}
        )
        self.assertNotEqual(0, ambiguous.returncode)
        self.assertIn("JSON strict est ambigu", ambiguous.stderr)
        self.assert_no_host_mutation(ambiguous_log)

        failed, failed_log, _ = self.run_provisioner(
            environment={"FAKE_SYSTEMD_FAIL": "restart"}
        )
        self.assertNotEqual(0, failed.returncode)
        self.assertIn("Redémarrage borné de docker.service échoué", failed.stderr)
        self.assertIn(
            "[DIAGNOSTIC docker-provision] 240 dernières lignes et 65536 octets au plus",
            failed.stderr,
        )
        self.assertIn("[DIAGNOSTIC docker-provision] fin.", failed.stderr)
        self.assertEqual(
            1,
            sum(
                "systemctl restart docker.service" in line
                for line in self.sudo_command_lines(failed_log)
            ),
        )
        self.assertLessEqual(failed.stderr.count("fake docker journal line"), 80)
        self.assertNotRegex(failed_log, r"docker (run|pull|build)")


if __name__ == "__main__":
    unittest.main()
