#!/usr/bin/env python3
"""Local, mutation-free simulations for the Phase 3 remote worker."""

from __future__ import annotations

import json
import os
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
if len(args) >= 2 and args[:2] == ["-n", "docker"]:
    environment = os.environ.copy()
    environment["FAKE_DOCKER_VIA_SUDO"] = "1"
    raise SystemExit(subprocess.call(["docker", *args[2:]], env=environment))
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


FAKE_DOCKER = r"""#!/usr/bin/env python3
import json
import os
from pathlib import Path
import sys

args = sys.argv[1:]
log = Path(os.environ["FAKE_COMMAND_LOG"])
with log.open("a", encoding="utf-8") as stream:
    stream.write("DOCKER " + " ".join(args) + "\n")

failure = os.environ.get("FAKE_FAIL", "")
image_id = os.environ["FAKE_IMAGE_ID"]

if args == ["info"]:
    if os.environ.get("FAKE_DIRECT_DOCKER_INFO_FAIL") == "1" and os.environ.get("FAKE_DOCKER_VIA_SUDO") != "1":
        raise SystemExit(1)
    print("fake docker info")
    raise SystemExit(0)

if args and args[0] == "build":
    if failure == "build":
        print("simulated build failure", file=sys.stderr)
        raise SystemExit(17)
    iidfile = Path(args[args.index("--iidfile") + 1])
    iidfile.write_text(image_id + "\n", encoding="utf-8")
    print("fake Docker build complete")
    raise SystemExit(0)

if args[:2] == ["image", "inspect"]:
    print(image_id)
    raise SystemExit(0)

if not args or args[0] != "run":
    raise SystemExit("unexpected fake docker invocation: " + " ".join(args))

index = 1
volumes = {}
container_env = {}
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
    }:
        value = args[index + 1]
        if option == "--volume":
            host, container, mode = value.rsplit(":", 2)
            volumes[container] = (Path(host), mode)
        elif option == "--env":
            key, env_value = value.split("=", 1)
            container_env[key] = env_value
        index += 2
    else:
        break
if index >= len(args):
    raise SystemExit("fake docker run is missing its image")
image_ref = args[index]
command = args[index + 1:]
if image_ref != f"morsehgp3d-phase3:{os.environ['FAKE_GIT_SHA']}":
    raise SystemExit("image tag is not tied to the qualified SHA")
if not command:
    raise SystemExit("fake docker run is missing its command")

required_mounts = {
    "/workspace/repository": "ro",
    "/workspace/repository/build": "rw",
    "/results": "rw",
}
for mount, mode in required_mounts.items():
    if volumes.get(mount, (None, None))[1] != mode:
        raise SystemExit(f"missing exact {mount}:{mode} mount")
for key in ("MORSEHGP3D_CUDA_IMAGE_REF", "MORSEHGP3D_CUDA_IMAGE_ID", "MORSEHGP3D_GIT_SHA"):
    if not container_env.get(key):
        raise SystemExit(f"missing container environment {key}")
if container_env.get("HOME") != "/workspace/repository/build/.phase3-home":
    raise SystemExit("container HOME is not the writable Phase 3 build home")

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

if command[:2] == ["cuobjdump", "-lelf"]:
    architecture = "sm_90" if failure == "foreign_arch" else "sm_120"
    print(f"ELF file 1: morsehgp3d_phase3.{architecture}.cubin")
    raise SystemExit(0)

if command[:2] == ["cuobjdump", "-lptx"]:
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


class Phase3RemoteQualificationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="phase3-remote-test-")
        self.root = Path(self.temporary.name)
        self.repository = self.root / "repository"
        self.fake_bin = self.root / "fake-bin"
        self.output_dir = self.root / "artifacts"
        self.session_tmp = self.root / "session-tmp"
        self.command_log = self.root / "commands.log"
        for path in (
            self.repository / "gcp-migration",
            self.repository / "containers",
            self.repository / "morsehgp3d" / "tests" / "cuda",
            self.fake_bin,
            self.output_dir,
            self.session_tmp,
        ):
            path.mkdir(parents=True, exist_ok=True)

        self.worker = self.repository / "gcp-migration" / WORKER_SOURCE.name
        shutil.copy2(WORKER_SOURCE, self.worker)
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
        self._write_executable(self.fake_bin / "docker", FAKE_DOCKER)
        self._write_executable(
            self.repository / "gcp-migration" / "blackwell_preflight.sh",
            """#!/usr/bin/env bash
set -euo pipefail
printf 'PREFLIGHT %s\\n' "$*" >>"${FAKE_COMMAND_LOG}"
[[ "$*" == "--skip-docker" ]]
[[ "${FAKE_FAIL:-}" != "preflight" ]]
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
                "PATH": str(self.fake_bin) + os.pathsep + self.environment["PATH"],
                "TMPDIR": str(self.session_tmp),
                "PYTHONDONTWRITEBYTECODE": "1",
            }
        )

    def tearDown(self) -> None:
        self.temporary.cleanup()

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
    ) -> tuple[subprocess.CompletedProcess[str], Path]:
        artifact = output or (self.output_dir / "qualification.json")
        environment = self.environment.copy()
        environment["FAKE_FAIL"] = failure
        environment["FAKE_GUARD"] = guard
        environment["FAKE_DIRECT_DOCKER_INFO_FAIL"] = (
            "1" if direct_docker_info_fail else "0"
        )
        command = [str(self.worker)]
        if arguments is None:
            command.extend(["--yes", "--output", str(artifact)])
        else:
            command.extend(arguments)
        result = subprocess.run(
            command,
            cwd=self.repository,
            env=environment,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=20,
            check=False,
        )
        return result, artifact

    def command_log_text(self) -> str:
        if not self.command_log.exists():
            return ""
        return self.command_log.read_text(encoding="utf-8")

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

        result, _ = self.run_worker(arguments=["--yes", "--output", "relative.json"])
        self.assertNotEqual(0, result.returncode)
        self.assertIn("absolu", result.stderr.lower())
        self.assertEqual("", self.command_log_text())

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
        self.assertIn("cuobjdump -lptx", log)
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

        second, _ = self.run_worker(output=artifact)
        self.assertNotEqual(0, second.returncode)
        self.assertEqual(value, json.loads(artifact.read_text(encoding="utf-8")))

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

    def test_uses_noninteractive_sudo_docker_fallback(self) -> None:
        result, artifact = self.run_worker(direct_docker_info_fail=True)
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertTrue(artifact.is_file())
        log = self.command_log_text()
        self.assertIn("SUDO -n docker info", log)
        self.assertIn("SUDO -n docker build", log)
        self.assertIn("SUDO -n docker run", log)


if __name__ == "__main__":
    unittest.main()
