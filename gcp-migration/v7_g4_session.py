#!/usr/bin/env python3
"""Bounded v7 snapshot sessions using the repository's existing GCE guards."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import shlex
import shutil
import signal
import subprocess
import sys
import tarfile
import tempfile
import time
from typing import Any


SCHEMA = "ehgp.v7.g4.snapshot.v1"
SELF = "gcp-migration/v7_g4_session.py"
GUARDS = [
    "gcp-migration/start_and_verify.sh",
    "gcp-migration/stop_and_verify.sh",
]
WRAPPER = "gcp-migration/session_campagne_v7_g4.sh"
PRIVATE_CMAKE = "gcp-migration/private_cmake_v7.py"
CPU_PACKAGES = ("build-essential", "cmake", "libboost-dev", "time")
CPU_RUN_LIMIT = 120
PROCESS_DRAIN_MARGIN = 20
GPU_DIAGNOSTIC_RESERVE = 920
K10_PROCESS_TARGET_MS = 1000
CPU_PROBE = """#include <bit>
#include <span>
#include <cstdio>
#include <boost/version.hpp>
#include <boost/multiprecision/cpp_int.hpp>
static_assert(__cplusplus >= 202002L);
int main() {
  int data[] = {1, 2};
  std::span<int> values(data);
  boost::multiprecision::cpp_int integer = 1;
  integer <<= 80;
  if (values.size() != 2 || std::popcount(7u) != 3 || integer <= 1) return 1;
  std::printf("cplusplus=%ld boost=%d\\n", __cplusplus, BOOST_VERSION);
}
"""


def cpu_bootstrap_command(seconds: int) -> list[str]:
    """Root-owned timeout also bounds root apt children on SSH/user interruption."""
    if not 1 <= seconds <= 300:
        raise ValueError("invalid CPU bootstrap budget")
    packages = " ".join(CPU_PACKAGES)
    apt = ("apt-get -o DPkg::Lock::Timeout=30 -o Acquire::Retries=0 "
           "-o Acquire::http::Timeout=20 -o Acquire::https::Timeout=20 ")
    install = ("set -euo pipefail; " + apt + "update; " + apt +
               "-o Dpkg::Options::=--force-confold install --no-install-recommends -y " + packages)
    script = ("set -euo pipefail; present=1; "
              "for package in " + packages + "; do "
              "[[ $(dpkg-query -W -f='${db:Status-Status}' \"$package\" 2>/dev/null) == installed ]] || present=0; done; "
              "for tool in cmake c++ make; do command -v \"$tool\" >/dev/null || present=0; done; "
              "[[ -x /usr/bin/time ]] || present=0; "
              "if [[ $present == 0 ]]; then sudo -n /usr/bin/timeout --signal=TERM --kill-after=10s " +
              str(seconds) + "s /usr/bin/env DEBIAN_FRONTEND=noninteractive NEEDRESTART_MODE=l "
              "UCF_FORCE_CONFFOLD=1 /bin/bash -c " + shlex.quote(install) +
              "; printf 'CPU_BOOTSTRAP_ACTION=installed\\n'; else printf 'CPU_BOOTSTRAP_ACTION=already_present\\n'; fi")
    return ["bash", "-c", script]


def cpu_version_commands() -> dict[str, list[str]]:
    return {"cpu_cmake_version": ["cmake", "--version"],
            "cpu_compiler_version": ["c++", "--version"],
            "cpu_make_version": ["make", "--version"],
            "cpu_time_version": ["/usr/bin/time", "--version"],
            "cpu_package_versions": ["dpkg-query", "-W", "-f=${Package}\\t${Version}\\t${db:Status-Status}\\n",
                                     *CPU_PACKAGES]}


def cpu_toolchain_receipt(out: Path) -> dict[str, Any]:
    """Rebuild toolchain evidence from successful exact commands, also on receipt."""
    raw_versions = {}
    for name, command in cpu_version_commands().items():
        record = json.loads((out / (name + ".json")).read_text())
        if record.get("status") != "completed" or record.get("exit_code") != 0 or record.get("argv") != command:
            raise ValueError("CPU toolchain command/status mismatch: " + name)
        raw = (out / (name + ".stdout")).read_text()
        if not raw.strip():
            raise ValueError("CPU toolchain version missing: " + name)
        raw_versions[name] = raw
    match = re.match(r"cmake version ([0-9]+)\.([0-9]+)\.[0-9]+(?:\s|$)", raw_versions["cpu_cmake_version"])
    if not match or (int(match[1]), int(match[2])) < (3, 20):
        raise ValueError("CMake 3.20 or newer required by pinned sources")
    packages = {}
    for line in raw_versions["cpu_package_versions"].splitlines():
        fields = line.split("\t")
        if len(fields) != 3 or fields[0] in packages or fields[0] not in CPU_PACKAGES or not fields[1] or fields[2] != "installed":
            raise ValueError("CPU package version/status invalid")
        packages[fields[0]] = fields[1]
    if set(packages) != set(CPU_PACKAGES):
        raise ValueError("CPU package inventory incomplete")
    for name, command in (("cpu_cpp20_compile", cpu_probe_compile_command()),
                          ("cpu_cpp20_probe", ["./out/cpu_cpp20_probe"])):
        record = json.loads((out / (name + ".json")).read_text())
        if record.get("status") != "completed" or record.get("exit_code") != 0 or record.get("argv") != command:
            raise ValueError("CPU functional probe command/status mismatch")
    if (out / "cpu_cpp20_probe.cpp").read_text() != CPU_PROBE:
        raise ValueError("CPU functional probe source mismatch")
    probe = json.loads((out / "cpu_cpp20_probe.json").read_text())
    probe_text = (out / "cpu_cpp20_probe.stdout").read_text()
    match = re.fullmatch(r"cplusplus=([0-9]+) boost=([0-9]+)\n", probe_text)
    if probe.get("status") != "completed" or probe.get("exit_code") != 0 or not match or int(match[1]) < 202002 or int(match[2]) < 107400:
        raise ValueError("C++20/Boost 1.74 functional probe failed")
    return {"schema": "ehgp.v7.cpu.toolchain.v1", "packages": packages,
            "versions": raw_versions, "probe": probe_text, "public_status": "not_claimed"}


def cpu_probe_compile_command() -> list[str]:
    return ["c++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "out/cpu_cpp20_probe.cpp", "-o", "out/cpu_cpp20_probe"]


class SessionInterrupted(Exception):
    pass


def utc() -> str:
    return datetime.now(timezone.utc).isoformat()


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def write_json(path: Path, data: Any) -> None:
    encoded = (json.dumps(data, sort_keys=True, indent=2) + "\n").encode()
    fd, name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(encoded)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(name, path)
        parent = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
        try:
            os.fsync(parent)
        finally:
            os.close(parent)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def stop_process(process: subprocess.Popen[bytes], grace: int = 10) -> None:
    """Drain the owned group even if its leader has already exited."""
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        process.wait(timeout=10)
        return
    try:
        process.wait(timeout=grace)
    except subprocess.TimeoutExpired:
        pass
    # Waiting only for the leader is insufficient: descendants may ignore TERM.
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    process.wait(timeout=10)


def run_logged(directory: Path, name: str, argv: list[str], timeout: int,
               *, env: dict[str, str] | None = None, cwd: Path | None = None) -> int:
    """Record one bounded process, including argv and every nonzero outcome."""
    if not re.fullmatch(r"[a-z0-9_-]+", name):
        raise ValueError("invalid run name")
    start = time.monotonic()
    grace = 420 if name == "start" else 10  # Allow the guarded start's own stop trap.
    record: dict[str, Any] = {"argv": argv, "started": utc(), "timeout_seconds": timeout,
                              "status": "running", "exit_code": None}
    write_json(directory / f"{name}.json", record)
    with (directory / f"{name}.stdout").open("wb") as out, (directory / f"{name}.stderr").open("wb") as err:
        try:
            process = subprocess.Popen(argv, stdout=out, stderr=err, stdin=subprocess.DEVNULL,
                                       cwd=cwd, env=env, start_new_session=True)
        except OSError as error:
            record.update(status="failed", exit_code=127, error=str(error), ended=utc(),
                          elapsed_seconds=time.monotonic() - start)
            write_json(directory / f"{name}.json", record)
            return 127
        try:
            code = process.wait(timeout=timeout)
            stop_process(process, grace)
            record["status"] = "completed" if code == 0 else "failed"
        except subprocess.TimeoutExpired:
            stop_process(process, grace)
            code = 124
            record["status"] = "censored"
        except BaseException:
            stop_process(process, grace)
            record.update(status="interrupted", exit_code=process.returncode,
                          elapsed_seconds=time.monotonic() - start, ended=utc())
            write_json(directory / f"{name}.json", record)
            raise
    record.update(exit_code=code, elapsed_seconds=time.monotonic() - start, ended=utc())
    write_json(directory / f"{name}.json", record)
    return code


def safety_command(argv: list[str], timeout: int, env: dict[str, str]) -> tuple[int, bytes, bytes]:
    """The stop and its verification do not depend on a writable log directory."""
    process = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               stdin=subprocess.DEVNULL, env=env, start_new_session=True)
    try:
        output, errors = process.communicate(timeout=timeout)
        stop_process(process)
        return process.returncode, output, errors
    except BaseException:
        stop_process(process)
        raise


def source_paths(root: Path) -> list[Path]:
    paths: list[Path] = []
    for line in ("morsehgp3D_v6", "morsehgp3D_v7"):
        paths.append(root / line / "CMakeLists.txt")
        for folder in ("src", "cli", "oracle", "tests", "cmake", "bench"):
            paths.extend(p for p in (root / line / folder).rglob("*")
                         if p.is_file() and "__pycache__" not in p.parts and p.suffix != ".pyc")
        fixture = root / line / "receipts" / "conformite_v5"
        paths.extend(p for p in fixture.rglob("*") if p.is_file())
    paths.extend(root / p for p in GUARDS + [SELF, WRAPPER, PRIVATE_CMAKE])
    provenance = root / "morsehgp3D_v7/docs/V6_SOURCE_SNAPSHOT.json"
    if provenance.exists():
        paths.append(provenance)
    return sorted(set(paths))


def prepare(root: Path, base: Path) -> Path:
    root = root.resolve(strict=True)
    base.mkdir(mode=0o700, parents=True, exist_ok=True)
    if base.is_symlink() or base.stat().st_mode & 0o777 != 0o700:
        raise ValueError("session base must be a real private 0700 directory")
    session = Path(tempfile.mkdtemp(prefix="v7session.", dir=base.resolve()))
    source = session / "source"
    source.mkdir()
    records = []
    for path in source_paths(root):
        if path.is_symlink() or not path.is_file():
            raise ValueError(f"source must be a regular non-symlink file: {path}")
        relative = path.relative_to(root).as_posix()
        data = path.read_bytes()
        target = source / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        mode = 0o555 if path.stat().st_mode & 0o111 or relative in GUARDS + [WRAPPER, SELF] else 0o444
        target.chmod(mode)
        records.append({"path": relative, "size": len(data), "sha256": sha(data), "mode": mode})
    # Reject a mixed snapshot when another agent edited a source during capture.
    for record in records:
        if sha((root / record["path"]).read_bytes()) != record["sha256"]:
            raise ValueError(f"source changed during snapshot: {record['path']}")
    head = subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip()
    dirty = subprocess.check_output(["git", "-C", str(root), "status", "--porcelain=v1"], text=True)
    manifest = {"schema": SCHEMA, "head": head, "worktree_status": dirty,
                "public_status": "not_claimed", "created": utc(), "files": records,
                "scope": "v6_v7_worktree_cpu_pair_50000_then_gpu_primitives"}
    write_json(source / "source_manifest.json", manifest)
    with tarfile.open(session / "source.tgz", "w:gz") as archive:
        for path in sorted(p for p in source.rglob("*") if p.is_file()):
            archive.add(path, arcname=path.relative_to(source), recursive=False)
    write_json(session / "prepared.json", {
        "schema": SCHEMA, "manifest_sha256": sha((source / "source_manifest.json").read_bytes()),
        "archive_sha256": sha((session / "source.tgz").read_bytes()),
        "head": head, "created": utc(),
    })
    return session


def verify_source(root: Path, expected: str) -> dict[str, Any]:
    raw = (root / "source_manifest.json").read_bytes()
    if sha(raw) != expected:
        raise ValueError("source manifest hash mismatch")
    manifest = json.loads(raw)
    if manifest.get("schema") != SCHEMA or not manifest.get("files"):
        raise ValueError("invalid source manifest")
    seen = set()
    for item in manifest["files"]:
        relative = Path(item["path"])
        if relative.is_absolute() or ".." in relative.parts or item["path"] in seen:
            raise ValueError("unsafe or duplicate source path")
        seen.add(item["path"])
        path = root / relative
        if path.is_symlink() or not path.is_file():
            raise ValueError(f"missing source: {relative}")
        data = path.read_bytes()
        if len(data) != item["size"] or sha(data) != item["sha256"]:
            raise ValueError(f"changed source: {relative}")
        if item.get("mode") is not None and path.stat().st_mode & 0o777 != item["mode"]:
            raise ValueError(f"changed source mode: {relative}")
    actual = {p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()}
    if actual != seen | {"source_manifest.json"} or any(p.is_symlink() for p in root.rglob("*")):
        raise ValueError("unexpected source inventory or symlink")
    return manifest


def seal(directory: Path) -> str:
    records = []
    for path in sorted(directory.rglob("*")):
        if path.is_symlink():
            raise ValueError("result symlink")
        if path.is_file() and path != directory / "result_manifest.json":
            data = path.read_bytes()
            records.append({"path": path.relative_to(directory).as_posix(),
                            "size": len(data), "sha256": sha(data)})
    write_json(directory / "result_manifest.json", {"schema": "ehgp.v7.g4.results.v1", "files": records})
    return sha((directory / "result_manifest.json").read_bytes())


def verify_results(directory: Path, expected: str) -> None:
    raw = (directory / "result_manifest.json").read_bytes()
    if sha(raw) != expected:
        raise ValueError("result manifest hash mismatch")
    manifest = json.loads(raw)
    if manifest.get("schema") != "ehgp.v7.g4.results.v1":
        raise ValueError("result schema")
    expected_files = set()
    for record in manifest["files"]:
        relative = Path(record["path"])
        if relative.is_absolute() or ".." in relative.parts or str(relative) in expected_files:
            raise ValueError("unsafe/duplicate result path")
        expected_files.add(str(relative))
        path = directory / relative
        data = path.read_bytes()
        if path.is_symlink() or len(data) != record["size"] or sha(data) != record["sha256"]:
            raise ValueError(f"result changed: {relative}")
    actual = {str(p.relative_to(directory)) for p in directory.rglob("*")
              if p.is_file() and p != directory / "result_manifest.json"}
    if actual != expected_files or not actual or any(p.is_symlink() for p in directory.rglob("*")):
        raise ValueError("result inventory mismatch or empty")


def private_cmake_module(root: Path):
    spec = importlib.util.spec_from_file_location("private_cmake_v7", root / PRIVATE_CMAKE)
    if spec is None or spec.loader is None:
        raise ValueError("private CMake helper unavailable")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def compare_module(root: Path) -> Any:
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location("v7_compare", root / "morsehgp3D_v7/bench/compare_v6_v7.py")
    if spec is None or spec.loader is None:
        raise ValueError("missing pinned CPU comparison parser")
    comparison = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(comparison)
    return comparison


def incidence_module(root: Path) -> Any:
    spec = importlib.util.spec_from_file_location("v7_incidence", root / "morsehgp3D_v7/bench/incidence_campaign.py")
    if spec is None or spec.loader is None:
        raise ValueError("missing pinned incidence parser")
    previous = sys.modules.get("compare_v6_v7")
    sys.modules["compare_v6_v7"] = compare_module(root)
    try:
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module
    finally:
        if previous is None:
            sys.modules.pop("compare_v6_v7", None)
        else:
            sys.modules["compare_v6_v7"] = previous


def candidate_command(wide: bool) -> list[str]:
    return ["./build-v7/mhgp7", "--family=uniform", "--n=8000" if wide else "--n=50000", "--seed=3",
            "--threads=48", "--smax=11", "--layout=csr", "--digest", "--complete-incidences",
            "--mem-budget=17179869184", "--silent-meb-supports=1000000000"] + (["--coord=65536"] if wide else [])


def cpu_name(line: int, family: str, kmax: int) -> str:
    if line not in (6, 7) or family not in ("uniform", "terrain") or kmax not in (5, 10):
        raise ValueError("CPU observation identity outside protocol")
    return f"cpu_v{line}_{family}" + ("_k5" if kmax == 5 else "")


def cpu_command(line: int, family: str, kmax: int) -> list[str]:
    cpu_name(line, family, kmax)
    return [f"./build-v{line}/mhgp{line}", f"--family={family}", "--n=50000", "--seed=3",
            "--threads=48", f"--smax={kmax + 1}", "--fold-inflight=2", "--layout=csr", "--digest"]


def cpu_observation(directory: Path, root: Path, line: int, family: str, kmax: int) -> dict[str, Any]:
    """Reconstruct one outcome; censures/refusals never carry scientific objects."""
    name = cpu_name(line, family, kmax)
    record = json.loads((directory / f"{name}.json").read_text())
    identity = json.loads((directory / "identity.json").read_text())
    expected = ["/usr/bin/time", "-v", "-o", str(Path(identity["worker_root"]) / "out" / f"{name}.usage"),
                *cpu_command(line, family, kmax)]
    elapsed = record.get("elapsed_seconds")
    if record.get("argv") != expected or type(elapsed) not in (int, float) or not math.isfinite(elapsed) or elapsed < 0:
        raise ValueError("CPU command/time identity mismatch")
    gpu = json.loads((directory / "gpu_tools.json").read_text()).get("requested")
    if type(gpu) is not bool:
        raise ValueError("CPU reservation request absent")
    budget = json.loads((directory / f"{name}_budget.json").read_text())
    if budget != diagnostic_plan(budget.get("remaining_seconds"), CPU_RUN_LIMIT,
                                 GPU_DIAGNOSTIC_RESERVE if gpu and kmax == 5 else 0):
        raise ValueError("CPU run reservation mismatch")
    result = {"name": name, "version": f"v{line}", "family": family, "n": 50000, "seed": 3,
              "kmax": kmax, "s": 8, "threads": 48, "exit_code": record.get("exit_code"),
              "process_wall_seconds": elapsed, "timing_scope": "process_including_generation_and_digest",
              "public_status": "not_claimed"}
    if record.get("status") == "not_attempted":
        if record.get("exit_code") is not None or record.get("timeout_seconds") != 0 or elapsed != 0 or \
                record.get("reason") != "budget_insufficient":
            raise ValueError("invalid unattempted CPU observation")
        if budget["status"] != "not_attempted" or any((directory / f"{name}{suffix}").exists()
                                                        for suffix in (".stdout", ".stderr", ".usage")):
            raise ValueError("unattempted CPU run has evidence of execution or sufficient budget")
        return dict(result, status="not_attempted", reason="budget_insufficient")
    timeout = record.get("timeout_seconds")
    if type(timeout) is not int or timeout != CPU_RUN_LIMIT or budget["status"] != "planned":
        raise ValueError("CPU process bound/reservation mismatch")
    output, error = (directory / f"{name}.stdout").read_text(), (directory / f"{name}.stderr").read_text()
    if record.get("status") == "censored" and record.get("exit_code") == 124:
        if elapsed + 0.05 < timeout:
            raise ValueError("CPU censure predates its watchdog")
        return dict(result, status="censored", reason="process_watchdog", timeout_seconds=timeout)
    if record.get("exit_code") == 0 and record.get("status") == "completed":
        try:
            parsed = compare_module(root).parse_success(output, error, (directory / f"{name}.usage").read_text(),
                label=f"v{line}", family=family, n=50000, seed=3, threads=48, wall_seconds=elapsed, kmax=kmax)
        except (ValueError, RuntimeError) as failure:
            return dict(result, status="invalid", reason="success_parse_failed: " + str(failure))
        return json.loads(json.dumps(dict(result, status="engine_completed", **parsed)))
    if record.get("status") != "failed" or type(record.get("exit_code")) is not int or record["exit_code"] == 0:
        raise ValueError("CPU process outcome inconsistent")
    if record["exit_code"] == 2:
        lines = error.splitlines()
        typed = re.fullmatch(r"REFUS (unsupported_degeneracy|resource_exhausted) : [^\x00-\x1f\x7f]+", lines[0]) if lines else None
        stage = re.fullmatch(r"refus_etage=(entree|generation|rle|prefiltre|census|fold|publication) "
            r"rss_mb apres_generation=[0-9]+ apres_rle=[0-9]+ apres_prefiltre=[0-9]+ apres_census=[0-9]+ "
            r"max_fold=[0-9]+ \(frontiere de completion : dernier etage atteint\)", lines[1]) if len(lines) == 2 else None
        if not output and typed and stage:
            return dict(result, status="engine_refused", reason=lines[0], refusal_status=typed[1],
                        mathematical_refusal=typed[1] == "unsupported_degeneracy")
        return dict(result, status="invalid", reason="unqualified_refusal_or_published_prefix")
    return dict(result, status="failed", reason="unqualified_process_exit")


def cpu_pair(reference: dict[str, Any], candidate: dict[str, Any]) -> dict[str, Any]:
    if reference["status"] != "engine_completed" or candidate["status"] != "engine_completed":
        return {"status": "not_comparable", "reference_status": reference["status"], "v7_status": candidate["status"]}
    a, b = ({key: item[key] for key in ("digests", "cardinalities")} for item in (reference, candidate))
    return {"status": "completed" if a == b else "diverged", "equal": a == b, "reference": a, "v7": b}


def fallback_plan(k10: dict[str, Any], remaining: float, gpu: bool) -> dict[str, Any]:
    if type(remaining) not in (int, float) or not math.isfinite(remaining):
        raise ValueError("invalid K5 planning budget")
    reason = ("k10_process_over_1000ms" if k10["status"] == "engine_completed" and
              k10["process_wall_seconds"] * 1000 > K10_PROCESS_TARGET_MS else
              "k10_censored" if k10["status"] == "censored" else
              "k10_engine_refused" if k10["status"] == "engine_refused" else "not_required_or_invalid_k10")
    requested = reason != "not_required_or_invalid_k10"
    reserve = GPU_DIAGNOSTIC_RESERVE if gpu else 0
    required = reserve + 2 * (CPU_RUN_LIMIT + PROCESS_DRAIN_MARGIN)
    return {"schema": "ehgp.v7.k5-fallback.v1", "trigger": reason, "requested": requested,
            "status": "planned" if requested and remaining >= required else "not_attempted",
            "reason": "budget_insufficient" if requested and remaining < required else reason,
            "remaining_seconds": remaining, "required_seconds": required, "gpu_reserved_seconds": reserve,
            "process_target_ms": K10_PROCESS_TARGET_MS, "n": 50000, "kmax": 5, "public_status": "not_claimed"}


def cpu_campaign_receipt(directory: Path, root: Path) -> dict[str, Any]:
    gpu = json.loads((directory / "gpu_tools.json").read_text()).get("requested")
    if type(gpu) is not bool:
        raise ValueError("GPU reservation request missing")
    observations, pairs = [], {}
    for family in ("uniform", "terrain"):
        k10 = [cpu_observation(directory, root, line, family, 10) for line in (6, 7)]
        observations.extend(k10)
        pairs[family + "_k10"] = cpu_pair(*k10)
        plan = json.loads((directory / f"k5_{family}_plan.json").read_text())
        if plan != fallback_plan(k10[1], plan.get("remaining_seconds"), gpu):
            raise ValueError("K5 plan does not follow observed K10 and bounded budget")
        if plan["status"] == "planned":
            k5 = [cpu_observation(directory, root, line, family, 5) for line in (6, 7)]
            observations.extend(k5)
            pairs[family + "_k5"] = cpu_pair(*k5)
        else:
            pairs[family + "_k5"] = {"status": "not_attempted", "reason": plan["reason"], "requested": plan["requested"]}
            if any((directory / (cpu_name(line, family, 5) + suffix)).exists()
                   for line in (6, 7) for suffix in (".json", ".stdout", ".stderr", ".usage", "_observation.json", "_budget.json")):
                raise ValueError("unplanned K5 run has execution evidence")
        for kmax in (10, 5):
            pair_name = f"pair_{family}" + ("_k5" if kmax == 5 else "") + ".json"
            if json.loads((directory / pair_name).read_text()) != pairs[f"{family}_k{kmax}"]:
                raise ValueError("CPU pair projection differs from raw outcomes")
    for observation in observations:
        if json.loads((directory / (observation["name"] + "_observation.json")).read_text()) != observation:
            raise ValueError("CPU observation projection differs from raw evidence")
    successful = all(pair["status"] == "completed" or pair["status"] == "not_attempted" and not pair["requested"]
                     for pair in pairs.values())
    return {"schema": "ehgp.v7.cpu-towers.v1", "status": "completed" if successful else "failed_or_incomplete",
            "observations": observations, "pairs": pairs, "gpu_reserved_seconds": GPU_DIAGNOSTIC_RESERVE if gpu else 0,
            "public_status": "not_claimed", "semantics": "verified_events_only",
            "timing_scope": "process_including_generation_and_digest", "performance_claim": False}


def candidate_observation(directory: Path, root: Path, *, wide: bool) -> dict[str, Any]:
    name = "candidate_wide" if wide else "candidate_uniform"
    record = json.loads((directory / f"{name}.json").read_text())
    if record.get("argv", [])[4:] != candidate_command(wide):
        raise ValueError("candidate command identity mismatch")
    result = {"exit_code": record["exit_code"], "public_status": "not_claimed",
              "semantics": "normalized_horizontal_h0_candidate", "mathematical_refusal": False}
    output, error = (directory / f"{name}.stdout").read_text(), (directory / f"{name}.stderr").read_text()
    if record["exit_code"] == 124 and record["status"] == "censored":
        return dict(result, status="censored")
    if record["exit_code"] == 0 and record["status"] == "completed":
        parsed = incidence_module(root).parse_completion(
            output, error, (directory / f"{name}.usage").read_text(), family="uniform",
            n=8000 if wide else 50000, coord=65536 if wide else 0, seed=3, threads=48,
            meb_supports=1000000000, wall_seconds=record["elapsed_seconds"])
        return json.loads(json.dumps(dict(result, status="engine_completed", **parsed)))
    if record["exit_code"] == 2 and record["status"] == "failed":
        refusal = {"timed_out": False, "returncode": 2, "wall_seconds": record["elapsed_seconds"]}
        try:
            incidence_module(root).classify(refusal, directory / f"{name}.stdout", directory / f"{name}.stderr",
                                           directory / f"{name}.usage")
        except RuntimeError as failure:
            raise ValueError("candidate refusal validation: " + str(failure)) from failure
        return dict(result, status="engine_refused", reason=refusal["reason"],
                    mathematical_refusal=refusal["refusal_status"] == "unsupported_degeneracy",
                    refusal_status=refusal["refusal_status"], refusal_order=refusal["refusal_order"])
    raise ValueError("candidate failed outside a qualified observation outcome")


def validate_receipt(directory: Path, source: Path, manifest_hash: str, remote_rc: int) -> None:
    terminal = json.loads((directory / "worker_terminal.json").read_text())
    if terminal.get("exit_code") != remote_rc or terminal.get("public_status") != "not_claimed":
        raise ValueError("remote terminal status mismatch")
    if remote_rc == 0 and terminal.get("diagnostics_completed") is not True:
        raise ValueError("successful worker lacks diagnostic completion")
    if terminal.get("diagnostics_completed") is True and (directory / "failure.json").exists():
        raise ValueError("completed worker has contradictory failure evidence")
    if remote_rc and terminal.get("diagnostics_completed") is not True:
        return  # Preserve failure evidence; no successful campaign is published.
    identity = json.loads((directory / "identity.json").read_text())
    if identity.get("source_manifest_sha256") != manifest_hash or identity.get("public_status") != "not_claimed":
        raise ValueError("remote source identity mismatch")
    if json.loads((directory / "cpu_toolchain.json").read_text()) != cpu_toolchain_receipt(directory):
        raise ValueError("received CPU toolchain evidence mismatch")
    binaries = json.loads((directory / "cpu_binaries.json").read_text())
    if set(binaries) != {"v6", "v7"} or any(
            binaries[f"v{line}"].get("path") != f"build-v{line}/mhgp{line}" or
            not re.fullmatch(r"[0-9a-f]{64}", binaries[f"v{line}"].get("sha256", ""))
            for line in (6, 7)):
        raise ValueError("CPU binary identity missing")
    cpu = cpu_campaign_receipt(directory, source)
    if json.loads((directory / "cpu_campaign.json").read_text()) != cpu:
        raise ValueError("received CPU tower observations mismatch")
    if remote_rc != (0 if cpu["status"] == "completed" else 1):
        raise ValueError("CPU scientific failure was hidden by terminal outcome")
    for wide, filename in ((False, "candidate_status.json"), (True, "candidate_wide_status.json")):
        candidate = json.loads((directory / filename).read_text())
        plan = json.loads((directory / ("candidate_wide_budget.json" if wide else "candidate_uniform_budget.json")).read_text())
        gpu = json.loads((directory / "gpu_tools.json").read_text())["requested"]
        if plan != diagnostic_plan(plan.get("remaining_seconds"), 240 if wide else 120,
                                   GPU_DIAGNOSTIC_RESERVE if gpu else 0):
            raise ValueError("candidate budget reservation mismatch")
        if plan["status"] == "not_attempted":
            if candidate != {"status": "not_attempted", "reason": "budget_insufficient", "public_status": "not_claimed"}:
                raise ValueError("unattempted candidate falsely published an outcome")
            label = "candidate_wide" if wide else "candidate_uniform"
            if any((directory / f"{label}{suffix}").exists() for suffix in (".json", ".stdout", ".stderr", ".usage")):
                raise ValueError("unattempted candidate has execution evidence")
            continue
        label = "candidate_wide" if wide else "candidate_uniform"
        launch = json.loads((directory / f"{label}.json").read_text())
        expected = ["/usr/bin/time", "-v", "-o", str(Path(identity["worker_root"]) / "out" / f"{label}.usage"),
                    *candidate_command(wide)]
        elapsed = launch.get("elapsed_seconds")
        if launch.get("argv") != expected or launch.get("timeout_seconds") != (240 if wide else 120) or \
                type(elapsed) not in (int, float) or not math.isfinite(elapsed) or elapsed < 0:
            raise ValueError("candidate exact command/watchdog mismatch")
        if launch.get("status") == "censored" and elapsed + 0.05 < launch["timeout_seconds"]:
            raise ValueError("candidate censure predates watchdog")
        if candidate != candidate_observation(directory, source, wide=wide):
            raise ValueError("candidate outcome inconsistent with raw evidence")
    if json.loads((directory / "gpu_status.json").read_text()).get("status") not in \
            ("completed", "unavailable", "not_requested"):
        raise ValueError("GPU outcome missing or invalid")
    private_cmake_module(source).validate_selection(directory)


def read_fields(path: Path) -> dict[str, str]:
    fields = {}
    for line in path.read_text().splitlines():
        key, sep, value = line.partition("=")
        if not sep or key in fields:
            raise ValueError("invalid lifecycle record")
        fields[key] = value
    return fields


def valid_generation(value: Any) -> bool:
    if not isinstance(value, str) or not re.fullmatch(r"[0-9T:.+Z-]+", value):
        return False
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00")).tzinfo is not None
    except ValueError:
        return False


def known_generation(session: Path, target: dict[str, str]) -> str | None:
    """Only a guard's exact-target handoff/state can authorize a stop."""
    path = session / "start-handoff.json"
    handoff_error = None
    if path.exists():
        try:
            data = json.loads(path.read_text())
            if data.get("schema") != "e-hgp.start-handoff.v3" or \
                    data.get("status") not in ("targeted_running", "targeted_stopping") or \
                    data.get("guest_shutdown_minutes") != 45 or any(data.get(k) != v for k, v in target.items()):
                raise ValueError("handoff target mismatch")
            generation = data.get("last_start_timestamp")
            if not valid_generation(generation):
                raise ValueError("invalid handoff generation")
            return generation
        except (OSError, ValueError, AttributeError) as error:
            handoff_error = error
    path = session / "lifecycle.txt"
    if path.exists():
        data = read_fields(path)
        if data.get("schema") != "e-hgp.lifecycle-state.v1" or any(data.get(k) != v for k, v in target.items()):
            raise ValueError("lifecycle target mismatch")
        if data.get("state") in ("start_may_have_been_requested", "targeted_running", "targeted_stopping",
                                 "targeted_stopped", "targeted_stop_failed"):
            generation = data.get("generation", "")
            if valid_generation(generation):
                return generation
    if handoff_error:
        raise ValueError("handoff unusable and no valid lifecycle generation") from handoff_error
    return None


def target_valid(info: dict[str, Any], *, stopped: bool) -> bool:
    scheduling = info.get("scheduling", {})
    return (not stopped or info.get("status") == "TERMINATED") and \
        info.get("labels", {}).get("project") == "e-hgp" and \
        info.get("machineType", "").rsplit("/", 1)[-1] == "g4-standard-48" and \
        scheduling.get("provisioningModel") == "SPOT" and \
        scheduling.get("instanceTerminationAction") == "STOP" and \
        scheduling.get("onHostMaintenance") == "TERMINATE" and \
        scheduling.get("automaticRestart") is False and \
        str(scheduling.get("maxRunDuration", {}).get("seconds")) == "3600"


def guest_deadline(path: Path) -> int:
    fields = read_fields(path)
    if fields.get("MODE") != "poweroff" or not re.fullmatch(r"[0-9]+", fields.get("USEC", "")):
        raise ValueError("guest shutdown guard absent")
    deadline = int(fields["USEC"]) // 1_000_000
    if deadline <= time.time() + 180:
        raise ValueError("guest shutdown guard too close")
    return deadline


def diagnostic_plan(remaining: float, limit: int, reserve: int) -> dict[str, Any]:
    if type(remaining) not in (int, float) or not math.isfinite(remaining) or \
            limit not in (120, 240) or reserve not in (0, GPU_DIAGNOSTIC_RESERVE):
        raise ValueError("invalid diagnostic reservation")
    return {"status": "planned" if remaining >= limit + reserve + PROCESS_DRAIN_MARGIN else "not_attempted",
            "remaining_seconds": remaining, "run_limit_seconds": limit, "reserved_seconds": reserve,
            "drain_margin_seconds": PROCESS_DRAIN_MARGIN, "reason": "budget_insufficient" if
            remaining < limit + reserve + PROCESS_DRAIN_MARGIN else "within_guarded_worker_budget"}


def worker(root: Path, manifest_hash: str, gpu: bool) -> int:
    root = root.resolve()
    out = root / "out"
    out.mkdir()
    code = 1
    diagnostics_completed = False
    def interrupted(signum: int, _frame: Any) -> None:
        raise SessionInterrupted(f"worker signal {signum}")
    previous_handlers = {sig: signal.signal(sig, interrupted) for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)}
    try:
        verify_source(root, manifest_hash)
        deadline = min(time.time() + 2100, guest_deadline(Path("/run/systemd/shutdown/scheduled")) - 180)
        write_json(out / "identity.json", {"schema": SCHEMA, "source_manifest_sha256": manifest_hash,
                                          "guest_guard_deadline": deadline, "worker_root": str(root), "public_status": "not_claimed"})

        worker_env = dict(os.environ, LC_ALL="C", LANG="C")
        nvcc = shutil.which("nvcc")
        cuda_paths = ["/usr/local/cuda/bin/nvcc", "/usr/local/cuda-12.9/bin/nvcc",
                      "/usr/local/cuda-13.0/bin/nvcc"]
        if not nvcc:
            nvcc = next((p for p in cuda_paths if Path(p).is_file() and os.access(p, os.X_OK)), None)
        if nvcc:
            worker_env["PATH"] = str(Path(nvcc).parent) + os.pathsep + worker_env.get("PATH", "")

        def run(name: str, command: list[str], limit: int) -> None:
            guest_deadline(Path("/run/systemd/shutdown/scheduled"))
            remaining = int(deadline - time.time())
            if remaining < 1:
                raise TimeoutError("worker deadline exhausted")
            rc = run_logged(out, name, command, min(limit, remaining), cwd=root, env=worker_env)
            if rc:
                raise RuntimeError(f"{name} failed with code {rc}")

        # Provision only after the controller's exact-generation double guard
        # and this worker's live guest cutoff. Root timeout outlives a killed
        # user/SSH process by at most its own bounded window, never indefinitely.
        guest_deadline(Path("/run/systemd/shutdown/scheduled"))
        bootstrap_budget = min(300, int(deadline - time.time()) - 30)
        if bootstrap_budget < 1:
            raise TimeoutError("no safe CPU provisioning budget left")
        run("cpu_bootstrap", cpu_bootstrap_command(bootstrap_budget), bootstrap_budget + 20)
        for name, command in cpu_version_commands().items():
            run(name, command, 15)
        (out / "cpu_cpp20_probe.cpp").write_text(CPU_PROBE)
        run("cpu_cpp20_compile", cpu_probe_compile_command(), 45)
        run("cpu_cpp20_probe", ["./out/cpu_cpp20_probe"], 5)
        write_json(out / "cpu_toolchain.json", cpu_toolchain_receipt(out))
        run("machine", ["bash", "-euo", "pipefail", "-c", "uname -a; lscpu; free -b"], 30)
        nvcc_record: dict[str, Any] = {"nvcc": nvcc, "checked_cuda_paths": cuda_paths,
                                       "requested": gpu, "installation_attempted": False}
        if gpu and nvcc:
            guest_deadline(Path("/run/systemd/shutdown/scheduled"))
            limit = min(15, int(deadline - time.time()))
            if limit < 1:
                raise TimeoutError("no CUDA inventory budget left")
            nvcc_rc = run_logged(out, "nvcc_version", [nvcc, "--version"], limit, cwd=root, env=worker_env)
            nvcc_record["version_exit_code"] = nvcc_rc
            nvcc_record["version"] = (out / "nvcc_version.stdout").read_text()
            if nvcc_rc:
                nvcc = None
        write_json(out / "gpu_tools.json", nvcc_record)
        if shutil.which("nvidia-smi"):
            guest_deadline(Path("/run/systemd/shutdown/scheduled"))
            limit = min(30, int(deadline - time.time()))
            if limit < 1:
                raise TimeoutError("no hardware inventory budget left")
            run_logged(out, "gpu_hardware", ["nvidia-smi", "--query-gpu=name,driver_version,memory.total",
                                           "--format=csv,noheader"], limit, cwd=root, env=worker_env)
        run("cpu_build", ["bash", "-e", "-c",
            "cmake -S morsehgp3D_v6 -B build-v6 -DCMAKE_BUILD_TYPE=Release && "
            "cmake --build build-v6 --parallel 12 --target mhgp6 && "
            "cmake -S morsehgp3D_v7 -B build-v7 -DCMAKE_BUILD_TYPE=Release && "
            "cmake --build build-v7 --parallel 12 --target mhgp7"], 420)
        write_json(out / "cpu_binaries.json", {
            f"v{line}": {"path": f"build-v{line}/mhgp{line}",
                         "sha256": sha((root / f"build-v{line}/mhgp{line}").read_bytes())}
            for line in (6, 7)})
        def observe_cpu(line: int, family: str, kmax: int) -> dict[str, Any]:
            name = cpu_name(line, family, kmax)
            command = ["/usr/bin/time", "-v", "-o", str(out / f"{name}.usage"), *cpu_command(line, family, kmax)]
            guest_deadline(Path("/run/systemd/shutdown/scheduled"))
            plan = diagnostic_plan(deadline - time.time(), CPU_RUN_LIMIT,
                                   GPU_DIAGNOSTIC_RESERVE if gpu and kmax == 5 else 0)
            write_json(out / f"{name}_budget.json", plan)
            if plan["status"] == "planned":
                run_logged(out, name, command, CPU_RUN_LIMIT, cwd=root, env=worker_env)
            else:
                write_json(out / f"{name}.json", {"argv": command, "status": "not_attempted",
                    "reason": "budget_insufficient", "timeout_seconds": 0, "elapsed_seconds": 0, "exit_code": None})
            observation = cpu_observation(out, root, line, family, kmax)
            write_json(out / f"{name}_observation.json", observation)
            return observation

        # Scientific failures do not erase attempts or prevent independent
        # guarded diagnostics. They still force a nonzero terminal outcome.
        k10_observations = {}
        for family in ("uniform", "terrain"):
            k10 = [observe_cpu(line, family, 10) for line in (6, 7)]
            k10_observations[family] = k10
            write_json(out / f"pair_{family}.json", cpu_pair(*k10))
        for family in ("uniform", "terrain"):
            plan = fallback_plan(k10_observations[family][1], deadline - time.time(), gpu)
            write_json(out / f"k5_{family}_plan.json", plan)
            if plan["status"] == "planned":
                k5 = [observe_cpu(line, family, 5) for line in (6, 7)]
                write_json(out / f"pair_{family}_k5.json", cpu_pair(*k5))
            else:
                write_json(out / f"pair_{family}_k5.json", {"status": "not_attempted", "reason": plan["reason"],
                                                           "requested": plan["requested"]})
        cpu_receipt = cpu_campaign_receipt(out, root)
        write_json(out / "cpu_campaign.json", cpu_receipt)

        for wide in (False, True):
            name = "candidate_wide" if wide else "candidate_uniform"
            status_name = "candidate_wide_status.json" if wide else "candidate_status.json"
            limit = 240 if wide else 120
            guest_deadline(Path("/run/systemd/shutdown/scheduled"))
            plan = diagnostic_plan(deadline - time.time(), limit, GPU_DIAGNOSTIC_RESERVE if gpu else 0)
            write_json(out / f"{name}_budget.json", plan)
            if plan["status"] == "planned":
                run_logged(out, name, ["/usr/bin/time", "-v", "-o", str(out / f"{name}.usage"),
                                      *candidate_command(wide)], limit, cwd=root, env=worker_env)
                write_json(out / status_name, candidate_observation(out, root, wide=wide))
            else:
                write_json(out / status_name, {"status": "not_attempted", "reason": "budget_insufficient",
                                               "public_status": "not_claimed"})
        # Optional GPU-only tooling; CPU stays on the recorded system CMake.
        # The outer process-group watchdog is mandatory: socket inactivity
        # timeouts and cooperative Budget checks are not total OS deadlines.
        cmake_tool = private_cmake_module(root)
        gpu_cmake, gpu_ctest = "cmake", "ctest"
        tooling_failure = None
        private_installation = None
        raw_system_cmake = (out / "cpu_cmake_version.stdout").read_text()
        if gpu and nvcc and shutil.which("nvidia-smi") and cmake_tool.system_version(raw_system_cmake) < (3, 26, 0):
            if not cmake_tool.installation_needed(raw_system_cmake, gpu_requested=True, nvcc_present=True,
                                                  remaining_seconds=deadline - time.time()):
                tooling_failure = "private_cmake_120s_plus_gpu780s_and_drain20s_budget_unavailable"
            else:
                guest_deadline(Path("/run/systemd/shutdown/scheduled"))
                install_command = [sys.executable, PRIVATE_CMAKE, "--install", "--root", str(root),
                                   "--seconds", str(cmake_tool.INSTALL_BUDGET), "--deadline-epoch",
                                   str(deadline - cmake_tool.GPU_RESERVE - cmake_tool.DRAIN_MARGIN)]
                install_rc = run_logged(out, "gpu_cmake_install", install_command, cmake_tool.INSTALL_BUDGET,
                                        cwd=root, env=worker_env)
                if install_rc:
                    tooling_failure = "private_cmake_install_failed_or_censored"
                else:
                    private_installation = json.loads((out / "gpu_cmake_install.stdout").read_text())
                    cmake_tool.validate_receipt(private_installation)
                    gpu_cmake = "./" + private_installation["paths"]["cmake"]
                    gpu_ctest = "./" + private_installation["paths"]["ctest"]
        write_json(out / "gpu_cmake_toolchain.json", {"schema": "ehgp.v7.gpu-cmake.v1",
                   "source": "private" if private_installation is not None else "system",
                   "system_version": raw_system_cmake, "installation": private_installation,
                   "cmake": gpu_cmake, "ctest": gpu_ctest})
        if gpu and nvcc and shutil.which("nvidia-smi") and not tooling_failure and deadline - time.time() >= 780:
            run("gpu_inventory", ["nvidia-smi", "-q"], 30)
            run("gpu_build", ["bash", "-e", "-c",
                f"{shlex.quote(gpu_cmake)} -S morsehgp3D_v7 -B build-v7-cuda -DCMAKE_BUILD_TYPE=Release "
                "-DMHGP7_ENABLE_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=120 "
                f"-DCMAKE_CUDA_COMPILER={shlex.quote(nvcc)} && "
                f"{shlex.quote(gpu_cmake)} --build build-v7-cuda --parallel 12 --target "
                "mhgp7_device_witness mhgp7_census_device_gate"], 420)
            write_json(out / "gpu_binaries.json", {
                binary: {"path": f"build-v7-cuda/{binary}",
                         "sha256": sha((root / f"build-v7-cuda/{binary}").read_bytes())}
                for binary in ("mhgp7_device_witness", "mhgp7_census_device_gate")})
            run("gpu_primitives", [gpu_ctest, "--test-dir", "build-v7-cuda", "--output-on-failure", "--no-tests=error",
                                  "-R", "^mhgp7_(device_witness|census_device)(_|$)", "-L", "gpu"], 300)
            write_json(out / "gpu_status.json", {"status": "completed", "scope": "device_primitives_only"})
        else:
            write_json(out / "gpu_status.json", {"status": "unavailable" if gpu else "not_requested",
                                                 "nvcc": nvcc, "checked_cuda_paths": cuda_paths,
                                                 "remaining_seconds": max(0, int(deadline - time.time())),
                                                 "reason": tooling_failure or ("tools_or_780_second_budget_unavailable" if gpu else "disabled")})
        code = 0 if cpu_receipt["status"] == "completed" else 1
        diagnostics_completed = True
    except BaseException as error:
        code = 1
        diagnostics_completed = False
        write_json(out / "failure.json", {"type": type(error).__name__, "message": str(error)})
    finally:
        for sig in previous_handlers:
            signal.signal(sig, signal.SIG_IGN)
        write_json(out / "worker_terminal.json", {"exit_code": code, "ended": utc(), "diagnostics_completed": diagnostics_completed,
                                                 "public_status": "not_claimed"})
        print("RESULT_MANIFEST_SHA256=" + seal(out), flush=True)
        for sig, handler in previous_handlers.items():
            signal.signal(sig, handler)
    return code


def session_run(session: Path, target: dict[str, str], ssh_key: Path, gpu: bool) -> int:
    session = session.resolve(strict=True)
    source = session / "source"
    prepared = json.loads((session / "prepared.json").read_text())
    verify_source(source, prepared["manifest_sha256"])
    if sha(Path(__file__).read_bytes()) != sha((source / SELF).read_bytes()):
        raise ValueError("controller differs from the reviewed source snapshot")
    if sha((session / "source.tgz").read_bytes()) != prepared["archive_sha256"]:
        raise ValueError("source archive changed")
    if (session / "session.json").exists():
        raise ValueError("session was already attempted; prepare a new snapshot")
    gcloud = shutil.which("gcloud")
    if gcloud is None:
        raise ValueError("gcloud unavailable")
    logs = session / "control"
    logs.mkdir()
    marks = session / "guard-marks"
    marks.mkdir(mode=0o700)
    env = dict(os.environ, GCP_PROJECT_ID=target["project"], GCP_ZONE=target["zone"],
               GCP_INSTANCE_NAME=target["instance"], GCP_SSH_KEY_FILE=str(ssh_key.resolve(strict=True)))
    state: dict[str, Any] = {"schema": SCHEMA, "target": target, "started": utc(), "status": "preflight",
                             "source_manifest_sha256": prepared["manifest_sha256"], "stopped_verified": False}
    write_json(session / "session.json", state)
    requested = False
    remembered_generation: str | None = None
    observed_before: dict[str, Any] = {}
    code = 1

    def describe(name: str) -> dict[str, Any]:
        command = [gcloud, "compute", "instances", "describe", target["instance"],
                   "--project=" + target["project"], "--zone=" + target["zone"], "--format=json"]
        if run_logged(logs, name, command, 30, env=env):
            raise RuntimeError("target state unreadable")
        return json.loads((logs / f"{name}.stdout").read_text())

    def interrupted(signum: int, _frame: Any) -> None:
        raise SessionInterrupted(f"signal {signum}")

    previous_handlers = {sig: signal.signal(sig, interrupted) for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)}
    try:
        observed_before = describe("preflight")
        if not target_valid(observed_before, stopped=True):
            raise ValueError("target must be TERMINATED G4 SPOT STOP with maxRunDuration=3600")
        state["status"] = "start_requested"
        write_json(session / "session.json", state)
        requested = True
        start = ["bash", str(source / GUARDS[0]), "--yes", "--guest-shutdown-minutes", "45",
                 "--handoff-file", str(session / "start-handoff.json"),
                 "--lifecycle-state-file", str(session / "lifecycle.txt"), "--guard-mark-dir", str(marks)]
        if run_logged(logs, "start", start, 900, env=env):
            raise RuntimeError("guarded start failed")
        generation = known_generation(session, target)
        if not generation:
            raise ValueError("successful start without exact generation")
        state.update(status="running", generation=generation)
        remembered_generation = generation
        write_json(session / "session.json", state)
        start_text = (logs / "start.stdout").read_text()
        expiration = re.search(r"expiration fixe=([^,\s]+)", start_text)
        if not expiration or "double_guard_verified" not in {p.name for p in marks.iterdir()}:
            raise ValueError("missing SSH expiry or verified guest guard marker")
        marker = read_fields(marks / "double_guard_verified")
        expected_marker = dict(target, schema="e-hgp.guard-mark.v1", mark="double_guard_verified",
                               generation=generation, max_run_seconds="3600", guest_shutdown_minutes="45")
        if any(marker.get(k) != v for k, v in expected_marker.items()):
            raise ValueError("double guard marker identity mismatch")
        common = ["--project=" + target["project"], "--zone=" + target["zone"],
                  "--ssh-key-file=" + str(ssh_key.resolve()),
                  "--ssh-key-expiration=" + expiration.group(1), "--quiet"]
        remote = "ehgp-v7-" + session.name.rsplit(".", 1)[-1] + "-" + prepared["manifest_sha256"][:12]
        if not re.fullmatch(r"[a-zA-Z0-9_-]+", remote):
            raise ValueError("invalid remote basename")
        command = [gcloud, "compute", "scp", *common, str(session / "source.tgz"),
                   target["instance"] + ":" + remote + ".tgz"]
        if run_logged(logs, "upload", command, 120, env=env):
            raise RuntimeError("source transfer failed")
        remote_command = (
            f"test ! -e {shlex.quote(remote)} && mkdir -m 700 {shlex.quote(remote)} && "
            f"test \"$(sha256sum {shlex.quote(remote + '.tgz')} | cut -d ' ' -f 1)\" = "
            f"{shlex.quote(prepared['archive_sha256'])} && "
            f"tar --no-same-owner -xzf {shlex.quote(remote + '.tgz')} -C {shlex.quote(remote)} && "
            f"cd {shlex.quote(remote)} && timeout --kill-after=15s 2100s python3 {SELF} worker "
            f"--manifest-sha {shlex.quote(prepared['manifest_sha256'])}" + ("" if gpu else " --without-gpu")
        )
        remote_rc = run_logged(logs, "remote", [gcloud, "compute", "ssh", target["instance"], *common,
                                              "--command=" + remote_command], 2220, env=env)
        received = session / "received"
        received.mkdir()
        transfer_rc = run_logged(logs, "download", [gcloud, "compute", "scp", *common, "--recurse",
                                target["instance"] + ":" + remote + "/out", str(received)], 120, env=env)
        digest_match = re.findall(r"^RESULT_MANIFEST_SHA256=([0-9a-f]{64})$",
                                 (logs / "remote.stdout").read_text(), flags=re.M)
        if transfer_rc or len(digest_match) != 1:
            raise RuntimeError("result receipt incomplete")
        verify_results(received / "out", digest_match[0])
        validate_receipt(received / "out", source, prepared["manifest_sha256"], remote_rc)
        state["result_manifest_sha256"] = digest_match[0]
        state["remote_exit_code"] = remote_rc
        code = 0 if remote_rc == 0 else 1
    except BaseException as error:
        state.update(failure_type=type(error).__name__, failure=str(error))
        code = 130 if isinstance(error, SessionInterrupted) else 1
    finally:
        # A second signal must not interrupt the exact-target shutdown funnel.
        for sig in previous_handlers:
            signal.signal(sig, signal.SIG_IGN)
        if requested:
            try:
                generation = remembered_generation or known_generation(session, target)
                if generation:
                    state["generation"] = generation
                    stop = ["bash", str(source / GUARDS[1]), "--yes",
                            "--expected-last-start-timestamp", generation]
                    # Execute safety independently of any logging operation.
                    stop_started = time.monotonic()
                    stop_rc, stop_output, stop_errors = safety_command(stop, 420, env)
                    check = [gcloud, "compute", "instances", "describe", target["instance"],
                             "--project=" + target["project"], "--zone=" + target["zone"], "--format=json"]
                    check_rc, after_raw, check_errors = safety_command(check, 30, env)
                    try:
                        (logs / "stop.stdout").write_bytes(stop_output)
                        (logs / "stop.stderr").write_bytes(stop_errors)
                        (logs / "post_stop.stdout").write_bytes(after_raw)
                        (logs / "post_stop.stderr").write_bytes(check_errors)
                        write_json(logs / "stop.json", {"argv": stop, "exit_code": stop_rc, "ended": utc(),
                                                      "stop_and_check_seconds": time.monotonic() - stop_started})
                        write_json(logs / "post_stop.json", {"argv": check, "exit_code": check_rc, "ended": utc()})
                    except OSError as error:
                        state["receipt_failure"] = str(error)
                        code = 1
                    if check_rc:
                        raise RuntimeError("post-stop target state unreadable")
                    after = json.loads(after_raw)
                    if stop_rc or not target_valid(after, stopped=True) or after.get("lastStartTimestamp") != generation:
                        raise RuntimeError("exact generation stop unverified")
                    state["stopped_verified"] = True
                else:
                    # Capacity rejection: prove that no new generation is running.
                    check = [gcloud, "compute", "instances", "describe", target["instance"],
                             "--project=" + target["project"], "--zone=" + target["zone"], "--format=json"]
                    check_rc, after_raw, _ = safety_command(check, 30, env)
                    if check_rc:
                        raise RuntimeError("failed-start target state unreadable")
                    after = json.loads(after_raw)
                    if not target_valid(after, stopped=True) or \
                            after.get("lastStartTimestamp") != observed_before.get("lastStartTimestamp"):
                        raise RuntimeError("start state ambiguous without guard generation")
                    state.update(stopped_verified=True, no_session_created=True)
            except BaseException as error:
                state.update(shutdown_failure=str(error), status="blocked")
                state["control_command"] = shlex.join([gcloud, "compute", "instances", "describe", target["instance"],
                                                       "--project=" + target["project"], "--zone=" + target["zone"],
                                                       "--format=value(status,lastStartTimestamp)"])
                code = 74
        if state["status"] != "blocked":
            state["status"] = "completed" if code == 0 else "failed"
        state.update(exit_code=code, ended=utc())
        try:
            write_json(session / "session.json", state)
        except OSError as error:
            state["receipt_failure"] = str(error)
            code = 74 if requested and not state["stopped_verified"] else 1
            state.update(exit_code=code, status="blocked" if code == 74 else "failed")
        for sig, handler in previous_handlers.items():
            signal.signal(sig, handler)
    print(json.dumps({"session": str(session), "status": state["status"], "target": target,
                      "generation": state.get("generation"), "shutdown_failure": state.get("shutdown_failure"),
                      "receipt_failure": state.get("receipt_failure"),
                      "control_command": state.get("control_command"),
                      "stopped_verified": state["stopped_verified"], "exit_code": code}))
    return code


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    prep = sub.add_parser("prepare")
    prep.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    prep.add_argument("--session-base", type=Path, required=True)
    run = sub.add_parser("run")
    run.add_argument("--session", type=Path, required=True)
    run.add_argument("--project", default="devpod-gpu-exploration")
    run.add_argument("--zone", default="europe-west4-a")
    run.add_argument("--instance", default="ehgp-blackwell-spot")
    run.add_argument("--ssh-key", type=Path, required=True)
    run.add_argument("--without-gpu", action="store_true")
    remote = sub.add_parser("worker")
    remote.add_argument("--manifest-sha", required=True)
    remote.add_argument("--without-gpu", action="store_true")
    args = parser.parse_args()
    if args.action == "prepare":
        print(prepare(args.root, args.session_base))
        return 0
    if args.action == "worker":
        return worker(Path.cwd(), args.manifest_sha, not args.without_gpu)
    target = {"project": args.project, "zone": args.zone, "instance": args.instance}
    return session_run(args.session, target, args.ssh_key, not args.without_gpu)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"REFUS: {error}", file=sys.stderr)
        raise SystemExit(2)
