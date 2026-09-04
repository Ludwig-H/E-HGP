#!/usr/bin/env python3
"""OVERLAY: pinned private CMake for GPU builds, never install into the OS."""
from __future__ import annotations

import argparse
import hashlib
import io
import json
import math
import os
from pathlib import Path, PurePosixPath
import re
import stat
import subprocess
import time
import urllib.request
import zipfile


VERSION = "3.31.6"
WHEEL_URL = "https://files.pythonhosted.org/packages/59/e8/096984b89133681533650b9078c5ed1c5c9b534e869b5487f22d4de1935c/cmake-3.31.6-py3-none-manylinux_2_17_x86_64.manylinux2014_x86_64.whl"
WHEEL_SHA256 = "1c8b05df0602365da91ee6a3336fe57525b137706c4ab5675498f662ae1dbcec"
WHEEL_SIZE = 27800904
SOURCE_JSON = "https://pypi.org/pypi/cmake/3.31.6/json"
SELF = "gcp-migration/private_cmake_v7.py"
INSTALL_BUDGET = 120
GPU_RESERVE = 780
DRAIN_MARGIN = 20
MAX_UNPACKED = 256 * 1024 * 1024
MAX_MEMBERS = 10000
TOOL_DIR = "tooling/cmake-3.31.6"
BINARIES = {name: f"{TOOL_DIR}/cmake/data/bin/{name}" for name in ("cmake", "ctest")}
CUDA_MODULE = "cmake/data/share/cmake-3.31/Modules/Compiler/NVIDIA-CUDA.cmake"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def system_version(text: str) -> tuple[int, int, int]:
    match = re.match(r"cmake version ([0-9]+)\.([0-9]+)\.([0-9]+)(?:\s|$)", text)
    require(match is not None, "unreadable system CMake version")
    return tuple(map(int, match.groups()))


def installation_needed(text: str, *, gpu_requested: bool, nvcc_present: bool,
                        remaining_seconds: float) -> bool:
    require(math.isfinite(remaining_seconds), "invalid remaining worker budget")
    return (gpu_requested and nvcc_present and system_version(text) < (3, 26, 0) and
            remaining_seconds >= INSTALL_BUDGET + GPU_RESERVE + DRAIN_MARGIN)


class Budget:
    def __init__(self, deadline_epoch: float, seconds: int):
        require(type(seconds) is int and 1 <= seconds <= INSTALL_BUDGET, "installation budget outside1..120")
        require(math.isfinite(deadline_epoch), "invalid deadline")
        self.end = time.monotonic() + min(seconds, deadline_epoch - time.time())
        self.remaining()

    def remaining(self) -> float:
        left = self.end - time.monotonic()
        if left <= 0:
            raise TimeoutError("private CMake installation deadline exhausted")
        return left


def download(budget: Budget) -> bytes:
    request = urllib.request.Request(WHEEL_URL, headers={"User-Agent": "E-HGP-pinned-tooling/1"})
    with urllib.request.urlopen(request, timeout=min(15.0, budget.remaining())) as response:
        require(response.status == 200 and response.geturl() == WHEEL_URL, "wheel response or redirect mismatch")
        length = response.headers.get("Content-Length")
        require(length is None or length == str(WHEEL_SIZE), "wheel content length mismatch")
        data = bytearray()
        while True:
            budget.remaining()
            block = response.read(min(1 << 20, WHEEL_SIZE + 1 - len(data)))
            if not block:
                break
            data.extend(block)
            require(len(data) <= WHEEL_SIZE, "wheel download exceeds pinned size")
    require(len(data) == WHEEL_SIZE and digest(data) == WHEEL_SHA256, "wheel size or SHA mismatch")
    return bytes(data)


def unpack(data: bytes, destination: Path, budget: Budget) -> dict:
    require(len(data) == WHEEL_SIZE and digest(data) == WHEEL_SHA256, "wheel size or SHA mismatch before extraction")
    with zipfile.ZipFile(io.BytesIO(data)) as wheel:
        entries = wheel.infolist()
        require(0 < len(entries) <= MAX_MEMBERS, "wheel member count outside bound")
        names, total = set(), 0
        for entry in entries:
            budget.remaining()
            name = entry.filename
            path = PurePosixPath(name)
            require(name and "\\" not in name and "\0" not in name and not path.is_absolute() and
                    str(path) == name.rstrip("/") and all(part not in (".", "..") for part in path.parts),
                    "unsafe wheel path")
            require(str(path) not in names, "duplicate wheel path")
            names.add(str(path))
            kind = stat.S_IFMT(entry.external_attr >> 16)
            require(kind in (0, stat.S_IFREG, stat.S_IFDIR), "wheel links or special files forbidden")
            require(kind != stat.S_IFDIR or entry.is_dir(), "wheel directory type mismatch")
            require(not entry.flag_bits & 1 and entry.compress_type in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED),
                    "unsupported encrypted/compressed wheel")
            total += entry.file_size
            require(total <= MAX_UNPACKED, "wheel uncompressed size exceeds bound")
        require({"cmake/data/bin/cmake", "cmake/data/bin/ctest", CUDA_MODULE} <= names,
                "pinned native CMake layout incomplete")
        # Validate the complete ZIP before creating any extracted member.
        destination.mkdir(mode=0o700)
        destination.chmod(0o700)
        extracted = 0
        for entry in entries:
            budget.remaining()
            if not entry.filename.startswith("cmake/data/") or entry.is_dir():
                continue
            target = destination.joinpath(*PurePosixPath(entry.filename).parts)
            target.parent.mkdir(mode=0o755, parents=True, exist_ok=True)
            with wheel.open(entry) as source, target.open("xb") as output:
                copied = 0
                while True:
                    budget.remaining()
                    block = source.read(1 << 20)
                    if not block:
                        break
                    copied += len(block)
                    require(copied <= entry.file_size, "inflated member exceeds declared size")
                    output.write(block)
                require(copied == entry.file_size, "truncated wheel member")
            target.chmod(0o555 if entry.filename in ("cmake/data/bin/cmake", "cmake/data/bin/ctest", "cmake/data/bin/cpack") else 0o444)
            extracted += 1
    return {"member_count": len(entries), "extracted_files": extracted, "uncompressed_bytes": total}


def validate_receipt(record: dict) -> None:
    require(record.get("schema") == "ehgp.v7.private-cmake.v1" and record.get("status") == "completed" and
            record.get("public_status") == "not_claimed", "private tooling receipt status/schema")
    require(record.get("version") == VERSION and record.get("url") == WHEEL_URL and
            record.get("wheel_sha256") == WHEEL_SHA256 and record.get("wheel_bytes") == WHEEL_SIZE and
            record.get("source_json") == SOURCE_JSON, "private tooling distribution pin")
    require(record.get("helper_sha256") == digest(Path(__file__).read_bytes()), "private tooling helper source pin")
    seconds = record.get("elapsed_seconds")
    require(type(seconds) in (int, float) and math.isfinite(seconds) and 0 <= seconds <= INSTALL_BUDGET,
            "private tooling elapsed time outside budget")
    require(record.get("paths") == BINARIES and set(record.get("probes", {})) == set(BINARIES), "private tooling paths/probes")
    for name, relative in BINARIES.items():
        probe = record["probes"][name]
        require(probe.get("argv") == [relative, "--version"] and probe.get("exit_code") == 0 and
                probe.get("stderr") == "" and probe.get("stdout", "").startswith(f"{name} version {VERSION}\n"),
                "private tooling functional version probe")
        require(re.fullmatch(r"[0-9a-f]{64}", probe.get("binary_sha256", "")) is not None,
                "private tooling binary hash")
    extraction = record.get("extraction", {})
    require(type(extraction.get("member_count")) is int and 1 <= extraction["member_count"] <= MAX_MEMBERS and
            type(extraction.get("extracted_files")) is int and 3 <= extraction["extracted_files"] <= extraction["member_count"] and
            type(extraction.get("uncompressed_bytes")) is int and 1 <= extraction["uncompressed_bytes"] <= MAX_UNPACKED,
            "private tooling extraction bounds")


def validate_selection(directory: Path) -> None:
    """Replay tool selection from the returned raw records, on the local host."""
    def read(name: str):
        return json.loads((directory / name).read_text())
    selection = read("gpu_cmake_toolchain.json")
    gpu_status = read("gpu_status.json")
    raw_system = (directory / "cpu_cmake_version.stdout").read_text()
    require(selection.get("schema") == "ehgp.v7.gpu-cmake.v1" and selection.get("system_version") == raw_system,
            "GPU CMake selection schema/system version")
    if selection.get("source") == "private":
        installation = selection.get("installation", {})
        validate_receipt(installation)
        require(selection.get("cmake") == "./" + BINARIES["cmake"] and selection.get("ctest") == "./" + BINARIES["ctest"],
                "selected private executables mismatch")
        launch = read("gpu_cmake_install.json")
        identity = read("identity.json")
        argv = launch.get("argv", [])
        require(launch.get("status") == "completed" and launch.get("exit_code") == 0 and
                type(launch.get("timeout_seconds")) is int and 1 <= launch["timeout_seconds"] <= INSTALL_BUDGET,
                "private installer process outcome/bound")
        require(len(argv) == 9 and argv[1:4] == [SELF, "--install", "--root"] and
                argv[4] == identity.get("worker_root") and Path(argv[4]).is_absolute() and
                argv[5:8] == ["--seconds", str(INSTALL_BUDGET), "--deadline-epoch"],
                "private installer exact command")
        deadline = float(argv[8])
        require(math.isfinite(deadline) and abs(deadline - (identity["guest_guard_deadline"] - GPU_RESERVE - DRAIN_MARGIN)) <= 1e-6,
                "private installer deadline not derived from guarded worker")
        require((directory / "gpu_cmake_install.stderr").read_text() == "" and
                read("gpu_cmake_install.stdout") == installation, "private installer raw receipt mismatch")
        tools = read("gpu_tools.json")
        require(tools.get("requested") is True and tools.get("nvcc") and tools.get("version_exit_code") == 0 and
                system_version(raw_system) < (3, 26, 0), "private installer prerequisites not certified")
    else:
        require(selection.get("source") == "system" and selection.get("cmake") == "cmake" and
                selection.get("ctest") == "ctest" and selection.get("installation") is None,
                "invalid system CMake selection")
        if gpu_status.get("status") == "completed":
            require(system_version(raw_system) >= (3, 26, 0), "old system CMake cannot qualify CUDA20")
    if gpu_status.get("status") == "completed":
        build = read("gpu_build.json")
        command = build.get("argv", [])
        selected = selection["cmake"]
        require(build.get("status") == "completed" and build.get("exit_code") == 0 and
                len(command) == 4 and command[:3] == ["bash", "-e", "-c"] and
                command[3].startswith(selected + " -S morsehgp3D_v7 ") and
                ("&& " + selected + " --build build-v7-cuda ") in command[3], "GPU build did not use selected CMake")
        tests = read("gpu_primitives.json")
        expected = [selection["ctest"], "--test-dir", "build-v7-cuda", "--output-on-failure", "--no-tests=error",
                    "-R", "^mhgp7_(device_witness|census_device)(_|$)", "-L", "gpu"]
        require(tests.get("status") == "completed" and tests.get("exit_code") == 0 and tests.get("argv") == expected,
                "GPU primitive test command/outcome mismatch")


def install(root: Path, *, deadline_epoch: float, seconds: int = INSTALL_BUDGET) -> dict:
    start = time.monotonic()
    budget = Budget(deadline_epoch, seconds)
    root = root.resolve(strict=True)
    require(root.is_dir(), "worker root unavailable")
    tooling = root / "tooling"
    require(not tooling.is_symlink(), "tooling directory must not be a symlink")
    if not tooling.exists():
        tooling.mkdir(mode=0o700)
        tooling.chmod(0o700)
    require(tooling.is_dir() and tooling.stat().st_uid == os.geteuid(), "tooling directory ownership")
    require(tooling.stat().st_mode & 0o777 == 0o700, "tooling directory must already be private")
    destination = root / TOOL_DIR
    require(not destination.exists() and not destination.is_symlink(), "private tooling already attempted; no reuse/overwrite")
    extraction = unpack(download(budget), destination, budget)
    probes = {}
    for name, relative in BINARIES.items():
        command = [relative, "--version"]
        result = subprocess.run(command, cwd=root, stdin=subprocess.DEVNULL, capture_output=True, text=True,
                                timeout=min(10, budget.remaining()), check=False)
        budget.remaining()
        probes[name] = {"argv": command, "exit_code": result.returncode, "stdout": result.stdout,
                        "stderr": result.stderr, "binary_sha256": digest((root / relative).read_bytes())}
    record = {"schema": "ehgp.v7.private-cmake.v1", "status": "completed", "public_status": "not_claimed",
              "helper_sha256": digest(Path(__file__).read_bytes()),
              "version": VERSION, "source_json": SOURCE_JSON, "url": WHEEL_URL,
              "wheel_sha256": WHEEL_SHA256, "wheel_bytes": WHEEL_SIZE, "paths": BINARIES,
              "probes": probes, "extraction": extraction, "elapsed_seconds": time.monotonic() - start}
    validate_receipt(record)
    return record


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", action="store_true")
    parser.add_argument("--root", type=Path, default=Path.cwd())
    parser.add_argument("--deadline-epoch", type=float)
    parser.add_argument("--seconds", type=int, default=INSTALL_BUDGET)
    args = parser.parse_args()
    if not args.install:
        print(json.dumps({"status": "overlay_not_installed", "version": VERSION, "sha256": WHEEL_SHA256,
                          "bytes": WHEEL_SIZE, "url": WHEEL_URL}))
        return 0
    require(args.deadline_epoch is not None, "explicit worker-derived deadline required")
    print(json.dumps(install(args.root, deadline_epoch=args.deadline_epoch, seconds=args.seconds)))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(json.dumps({"status": "failed", "error": f"{type(error).__name__}: {error}"}))
        raise SystemExit(2)
