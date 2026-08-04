#!/usr/bin/env python3
"""Mutation tests for the guarded Phase 15 schema-6 G4 qualification."""

from __future__ import annotations

import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
CUDA_TESTS = ROOT / "morsehgp3d" / "tests" / "cuda"
ASSEMBLER = (
    CUDA_TESTS
    / "assemble_phase15_exact_higher_support_terminal_geometry_cuda_qualification.py"
)
REMOTE_SCRIPT = ROOT / "gcp-migration" / "phase3_remote_qualification.sh"
ORCHESTRATOR_SCRIPT = ROOT / "gcp-migration" / "run_phase3_qualification.sh"
sys.path.insert(0, str(CUDA_TESTS))

import assemble_phase15_exact_higher_support_terminal_geometry_cuda_qualification as assembler

GIT_SHA = "a" * 40
GIT_TREE = "c" * 40
IMAGE_ID = "sha256:" + "b" * 64
IMAGE_REF = f"morsehgp3d-phase3:{GIT_SHA}"
BASE_IMAGE_REF = assembler.BASE_IMAGE_REF
LIFECYCLE = dict(assembler.WORKER_LIFECYCLE)
FINAL_PROJECT = "devpod-gpu-exploration"
FINAL_ZONE = "europe-west4-ai1a"
FINAL_INSTANCE = "ehgp-blackwell-spot-ai1a"
FINAL_VERIFIED_AT = "2026-08-04T12:00:00Z"
FINAL_LAST_START = "2026-08-04T10:00:00.000-07:00"


def write_canonical(path: Path, value: object) -> None:
    path.write_text(
        json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
        + "\n",
        encoding="utf-8",
    )


def write_runner_json(path: Path, value: dict[str, object]) -> None:
    path.write_text(
        json.dumps(value, ensure_ascii=False, separators=(",", ":")) + "\n",
        encoding="utf-8",
    )


def qualification_result() -> dict[str, object]:
    return {
        "backend": assembler.BACKEND,
        "bounded_int256_count": 15,
        "bounded_int512_count": 2,
        "bounded_int1024_count": 2,
        "certified_count": 16,
        "completed_result_digest": 22,
        "component_batch_repeatable": True,
        "component_schema_version": assembler.COMPONENT_SCHEMA_VERSION,
        "cuda_device": 0,
        "cuda_execution_performed": True,
        "cuda_stream_total_ns": 101,
        "cpu_stream_total_ns": 102,
        "every_result_validated_once": True,
        "every_task_validated_before_launch": True,
        "fail_open_count": 5,
        "fixture_family_count": 6,
        "floating_point_decision_performed": False,
        "global_cell_coface_or_incidence_arena_materialized": False,
        "global_product_frontier_mutated": False,
        "host_fake_lifecycle_exercised": False,
        "host_batch_total_ns": 103,
        "internal_node_receipt_leaf_count": 3,
        "internal_node_receipt_task_count": 1,
        "kernel_launch_count": 3,
        "kernel_elapsed_ns": 104,
        "kernel_elapsed_ns_available": True,
        "kernel_ns": 104,
        "kernel_ns_available": True,
        "lbvh_build_total_ns": 105,
        "mismatch_count": 0,
        "mode": assembler.MODE,
        "native_lbvh_nodes_read_on_device": True,
        "narrow_int256_kernel_executed": True,
        "narrow_int512_kernel_executed": True,
        "ordinary_or_higher_order_delaunay_materialized": False,
        "overlap_rejected_without_launch": True,
        "output_published_atomically": True,
        "point_count": assembler.POINT_COUNT,
        "profile": assembler.PROFILE,
        "qualified": True,
        "rational_fallback_count": assembler.EXPECTED_RATIONAL_FALLBACK_COUNT,
        "resident_lease_index_identity_validated": True,
        "schema_version": assembler.QUALIFICATION_SCHEMA_VERSION,
        "source_authority_validated": True,
        "source_snapshot_epoch": 1,
        "stream_adapter_certified_positive_count": 8,
        "stream_adapter_component_cpu_fallback_count": 1,
        "stream_adapter_cache_capacity": 256,
        "stream_adapter_cache_hit_count": 20,
        "stream_adapter_cache_miss_count": 0,
        "stream_adapter_evaluate_call_count": 2,
        "stream_adapter_maximum_batch_size": 10,
        "stream_adapter_maximum_cache_entry_count": 20,
        "stream_adapter_native_exact_authority": True,
        "stream_adapter_native_kernel_certified_positive_count": 7,
        "stream_adapter_terminal_geometry_decision_count": 5,
        "stream_adapter_terminal_geometry_native_kernel_decision_count": 4,
        "stream_adapter_terminal_geometry_native_cuda": True,
        "stream_adapter_native_cpu_fallback_certified_positive_count": 1,
        "stream_adapter_qualified": True,
        "stream_adapter_prefetch_call_count": 1,
        "stream_adapter_prefetched_task_count": 20,
        "stream_adapter_submitted_task_count": 20,
        "stream_adapter_synchronization_count": 2,
        "stream_adapter_support_3_certified_positive_count": 4,
        "stream_adapter_support_4_certified_positive_count": 4,
        "stream_adapter_support_prune_certified_positive_count": 4,
        "stream_adapter_query_certified_positive_count": 4,
        "submitted_task_digest": 21,
        "terminal_affinely_dependent_count": 2,
        "terminal_boundary_reduced_count": 3,
        "terminal_classification_native_cuda": False,
        "terminal_device_fallback_without_category_count": 1,
        "terminal_exterior_circumcenter_count": 2,
        "terminal_geometry_host_fallback_decision_count": 1,
        "terminal_geometry_native_cuda": True,
        "terminal_geometry_native_kernel_decision_count": 8,
        "terminal_int256_to_host_int512_fallback_count": 0,
        "terminal_int512_to_host_int1024_fallback_count": 0,
        "terminal_int1024_to_cpu_rational_fallback_count": 1,
        "terminal_minimal_count": 2,
        "terminal_support_3_affinely_dependent_count": 1,
        "terminal_support_3_boundary_reduced_count": 1,
        "terminal_support_3_exterior_circumcenter_count": 1,
        "terminal_support_3_minimal_count": 1,
        "terminal_support_3_task_count": assembler.TERMINAL_SUPPORT_3_TASK_COUNT,
        "terminal_support_4_affinely_dependent_count": 1,
        "terminal_support_4_boundary_reduced_count": 2,
        "terminal_support_4_exterior_circumcenter_count": 1,
        "terminal_support_4_minimal_count": 1,
        "terminal_support_4_task_count": assembler.TERMINAL_SUPPORT_4_TASK_COUNT,
        "terminal_task_count": assembler.TERMINAL_TASK_COUNT,
        "synchronization_count": 1,
        "task_count": assembler.TASK_COUNT,
        "total_qualification_ns": 106,
    }


def environment_artifact() -> dict[str, object]:
    return {
        "backend": "cuda_g4",
        "checks": {
            "cuda_audit_workflow": "passed",
            "cuda_release_workflow": "passed",
        },
        "git": {"clean": True, "sha": GIT_SHA},
        "image": {
            "base_ref": BASE_IMAGE_REF,
            "dockerfile": "containers/cuda12.9-sm120.Dockerfile",
            "id": IMAGE_ID,
            "ref": IMAGE_REF,
        },
        "mode": "certified",
        "phase": "3",
        "profile": "hgp_reduced",
        "runtime_records": [{"manifest": {"compiled_sm": "sm_120"}}],
        "schema": assembler.PHASE3_SCHEMA,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": "environment_reproducibility_only",
        "status": "worker_passed_pending_shutdown",
        "vm_lifecycle": dict(LIFECYCLE),
    }


def ptxas_log() -> str:
    records = []
    for width, registers, stack in ((256, 64, 128), (512, 96, 256), (1024, 128, 512)):
        entry = (
            "_ZN10morsehgp3d3gpu6detail"
            "morsehgp3d_phase15_exact_higher_support_product_kernel"
            f"ILi{width}EEv"
        )
        records.append(
            f"ptxas info    : Compiling entry function '{entry}' for 'sm_120'\n"
            f"ptxas info    : Function properties for {entry}\n"
            f"    {stack} bytes stack frame, 0 bytes spill stores, 0 bytes spill loads\n"
            f"ptxas info    : Used {registers} registers, used 0 barriers\n"
        )
    return "[1/2] Building CUDA audit object\n" + "".join(records) + "[2/2] Linking\n"


class Phase15TerminalGeometryScriptWiringTests(unittest.TestCase):
    @staticmethod
    def assert_markers_in_order(raw: str, markers: tuple[str, ...]) -> None:
        cursor = 0
        for marker in markers:
            position = raw.find(marker, cursor)
            if position < 0:
                raise AssertionError(f"missing or out-of-order marker: {marker}")
            cursor = position + len(marker)

    def test_guarded_worker_and_orchestrator_wiring(self) -> None:
        remote = REMOTE_SCRIPT.read_text(encoding="utf-8")
        orchestrator = ORCHESTRATOR_SCRIPT.read_text(encoding="utf-8")

        self.assertIn(
            "--phase15-exact-higher-support-terminal-geometry-cuda-output est exclusive de tous les autres compagnons.",
            remote,
        )
        self.assert_markers_in_order(
            remote,
            (
                'begin_unit "phase15-exact-higher-support-terminal-geometry-cuda-release-build"',
                'begin_unit "phase15-exact-higher-support-terminal-geometry-cuda-audit-build"',
                "assembler.parse_ptxas_resources",
                'begin_unit "phase15-exact-higher-support-terminal-geometry-cuda-qualification"',
                "assembler.validate_qualification",
                'begin_unit "phase15-exact-higher-support-terminal-geometry-cuda-cuobjdump-elf"',
                'begin_unit "phase15-exact-higher-support-terminal-geometry-cuda-cuobjdump-ptx"',
                'begin_unit "phase15-exact-higher-support-terminal-geometry-cuda-memcheck"',
                'python3 -B "${PHASE15_EXACT_HIGHER_SUPPORT_TERMINAL_GEOMETRY_CUDA_ASSEMBLER}"',
                '"${PHASE15_EXACT_HIGHER_SUPPORT_TERMINAL_GEOMETRY_CUDA_PUBLISH_TEMP}"',
                '"${PHASE15_EXACT_HIGHER_SUPPORT_TERMINAL_GEOMETRY_CUDA_OUTPUT_PATH}"',
            ),
        )
        self.assertIn(
            "--target morsehgp3d_gpu_exact_higher_support_product_cuda_qualification",
            remote,
        )
        self.assertIn("/usr/local/cuda/bin/cuobjdump -lelf", remote)
        self.assertIn("/usr/local/cuda/bin/cuobjdump -lptx", remote)
        self.assertIn("--tool memcheck --leak-check full", remote)
        self.assertIn("HEAD_TREE=", remote)
        self.assertIn('--git-tree "${HEAD_TREE}"', remote)
        self.assertIn("--audit-binary", remote)

        self.assertIn("readonly DEFAULT_GUEST_SHUTDOWN_MINUTES=45", orchestrator)
        self.assertIn("readonly DEFAULT_EXPECTED_MAX_RUN_SECONDS=3600", orchestrator)
        self.assertIn('readonly DEFAULT_SSH_KEY_TTL="70m"', orchestrator)
        self.assertIn(
            "--phase15-exact-higher-support-terminal-geometry-cuda est mutuellement exclusive de tous les autres compagnons.",
            orchestrator,
        )
        self.assertIn(
            "--phase15-exact-higher-support-terminal-geometry-cuda-output",
            orchestrator,
        )
        self.assertIn('"${STOP_SCRIPT}" --yes', orchestrator)
        self.assertIn("HEAD_TREE=", orchestrator)
        self.assertIn("git_tree=git_tree", orchestrator)
        self.assertIn("POINT_HIERARCHY_QUALITY_SCALE == 1", orchestrator)
        self.assert_markers_in_order(
            orchestrator,
            (
                '"${INSTANCE_NAME}:${remote_phase15_exact_higher_support_terminal_geometry_cuda_artifact}"',
                "import assemble_phase15_exact_higher_support_terminal_geometry_cuda_qualification as assembler",
                "assembler.validate_artifact_file(",
                "if certify_target_stopped; then",
                "terminal_geometry_assembler.validate_final_artifact_file(",
                "os.link(temporary, target, follow_symlinks=False)",
            ),
        )

    def test_morton_combination_is_rejected_before_any_external_action(self) -> None:
        orchestrator = subprocess.run(
            [
                str(ORCHESTRATOR_SCRIPT),
                "--yes",
                "--phase15-exact-higher-support-terminal-geometry-cuda",
                "--morton-yao48-seed-work-profile",
            ],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertNotEqual(orchestrator.returncode, 0)
        self.assertIn(
            "--phase15-exact-higher-support-terminal-geometry-cuda est mutuellement exclusive",
            orchestrator.stderr,
        )
        self.assertNotIn("gcloud", orchestrator.stdout)

        worker = subprocess.run(
            [
                str(REMOTE_SCRIPT),
                "--yes",
                "--gce-deadline-epoch",
                "9999999999",
                "--output",
                "/tmp/phase3-schema6-exclusive-test.json",
                "--phase15-exact-higher-support-terminal-geometry-cuda-output",
                "/tmp/terminal-schema6-exclusive-test.json",
                "--morton-yao48-seed-work-profile-output",
                "/tmp/morton-schema6-exclusive-test.json",
            ],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertNotEqual(worker.returncode, 0)
        self.assertIn(
            "--phase15-exact-higher-support-terminal-geometry-cuda-output est exclusive",
            worker.stderr,
        )
        self.assertNotIn("docker", worker.stdout.lower())


class Phase15TerminalGeometryAssemblerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.directory = Path(self.temporary_directory.name)
        self.environment = self.directory / "phase3.json"
        self.release_build = self.directory / "release-build.log"
        self.audit_build = self.directory / "audit-build.log"
        self.qualification = self.directory / "qualification.json"
        self.qualification_stderr = self.directory / "qualification.stderr.log"
        self.elf = self.directory / "elf.log"
        self.ptx = self.directory / "ptx.log"
        self.ptx_stderr = self.directory / "ptx.stderr.log"
        self.memcheck = self.directory / "memcheck.log"
        self.binary = self.directory / "qualification-binary"
        self.audit_binary = self.directory / "audit-qualification-binary"
        self.output = self.directory / "artifact.json"

        write_canonical(self.environment, environment_artifact())
        write_runner_json(self.qualification, qualification_result())
        self.release_build.write_text(
            "[1/2] Building terminal geometry\n[2/2] Linking qualification\n",
            encoding="utf-8",
        )
        self.audit_build.write_text(ptxas_log(), encoding="utf-8")
        self.qualification_stderr.write_text("", encoding="utf-8")
        self.elf.write_text("Fatbin elf code:\narch = sm_120\n", encoding="utf-8")
        self.ptx.write_text("", encoding="utf-8")
        self.ptx_stderr.write_text(
            "cuobjdump info : No PTX file found to extract\n", encoding="utf-8"
        )
        qualification_line = (
            json.dumps(qualification_result(), sort_keys=True, separators=(",", ":"))
            + "\n"
        )
        self.memcheck.write_text(
            "========= COMPUTE-SANITIZER\n"
            + qualification_line
            + "========= LEAK SUMMARY: 0 bytes leaked in 0 allocations\n"
            + "========= ERROR SUMMARY: 0 errors\n",
            encoding="utf-8",
        )
        self.binary.write_bytes(b"schema6-terminal-geometry-qualification")
        self.audit_binary.write_bytes(b"schema6-terminal-geometry-audit-qualification")

    def run_assembler(
        self, *, output: Path | None = None
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                "-B",
                str(ASSEMBLER),
                "--git-sha",
                GIT_SHA,
                "--git-tree",
                GIT_TREE,
                "--base-image-ref",
                BASE_IMAGE_REF,
                "--image-ref",
                IMAGE_REF,
                "--image-id",
                IMAGE_ID,
                "--environment-artifact",
                str(self.environment),
                "--release-build-log",
                str(self.release_build),
                "--audit-build-log",
                str(self.audit_build),
                "--qualification-log",
                str(self.qualification),
                "--qualification-stderr-log",
                str(self.qualification_stderr),
                "--elf-log",
                str(self.elf),
                "--ptx-log",
                str(self.ptx),
                "--ptx-stderr-log",
                str(self.ptx_stderr),
                "--memcheck-log",
                str(self.memcheck),
                "--binary",
                str(self.binary),
                "--audit-binary",
                str(self.audit_binary),
                "--output",
                str(output or self.output),
            ],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def rewrite_qualification(self, value: dict[str, object]) -> None:
        write_runner_json(self.qualification, value)
        qualification_line = (
            json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n"
        )
        self.memcheck.write_text(
            "========= COMPUTE-SANITIZER\n"
            + qualification_line
            + "========= LEAK SUMMARY: 0 bytes leaked in 0 allocations\n"
            + "========= ERROR SUMMARY: 0 errors\n",
            encoding="utf-8",
        )

    def assert_rejected(self) -> None:
        completed = self.run_assembler()
        self.assertNotEqual(completed.returncode, 0, completed.stdout)
        self.assertFalse(self.output.exists())

    def write_final_artifacts(self) -> tuple[Path, Path]:
        if self.output.exists():
            self.output.unlink()
        completed = self.run_assembler()
        self.assertEqual(completed.returncode, 0, completed.stderr)
        lifecycle = assembler.expected_final_lifecycle(
            project=FINAL_PROJECT,
            zone=FINAL_ZONE,
            instance=FINAL_INSTANCE,
            guest_shutdown_minutes=45,
            final_status_verified_at_utc=FINAL_VERIFIED_AT,
            last_start_timestamp=FINAL_LAST_START,
        )
        final_environment = self.directory / "phase3-final.json"
        environment = environment_artifact()
        environment["status"] = "passed"
        environment["vm_lifecycle"] = copy.deepcopy(lifecycle)
        write_canonical(final_environment, environment)

        final_artifact = self.directory / "terminal-geometry-final.json"
        artifact = json.loads(self.output.read_text(encoding="utf-8"))
        artifact["status"] = "passed"
        artifact["vm_lifecycle"] = copy.deepcopy(lifecycle)
        artifact["provenance"]["environment_artifact_sha256"] = hashlib.sha256(
            final_environment.read_bytes()
        ).hexdigest()
        write_canonical(final_artifact, artifact)
        return final_environment, final_artifact

    def validate_final(self, environment: Path, artifact: Path) -> dict[str, object]:
        return assembler.validate_final_artifact_file(
            artifact,
            git_sha=GIT_SHA,
            git_tree=GIT_TREE,
            environment_artifact_path=environment,
            project=FINAL_PROJECT,
            zone=FINAL_ZONE,
            instance=FINAL_INSTANCE,
            guest_shutdown_minutes=45,
            final_status_verified_at_utc=FINAL_VERIFIED_AT,
            last_start_timestamp=FINAL_LAST_START,
        )

    def test_valid_evidence_is_component_only_pending_shutdown(self) -> None:
        direct_raw = self.qualification.read_text(encoding="utf-8")
        self.assertNotEqual(
            direct_raw,
            json.dumps(
                qualification_result(),
                ensure_ascii=False,
                sort_keys=True,
                separators=(",", ":"),
            )
            + "\n",
        )
        completed = self.run_assembler()
        self.assertEqual(completed.returncode, 0, completed.stderr)
        artifact = assembler.validate_artifact_file(
            self.output,
            git_sha=GIT_SHA,
            git_tree=GIT_TREE,
            environment_artifact_path=self.environment,
        )
        self.assertEqual(set(artifact), assembler.ARTIFACT_KEYS)
        self.assertEqual(artifact["schema"], assembler.SCHEMA)
        self.assertEqual(artifact["claims"], assembler.CLAIMS)
        self.assertIs(artifact["component_only"], True)
        self.assertIs(artifact["scientific_result_claimed"], False)
        self.assertIsNone(artifact["scientific_public_status"])
        self.assertEqual(artifact["status"], "worker_passed_pending_shutdown")
        self.assertEqual(artifact["vm_lifecycle"], LIFECYCLE)
        self.assertEqual(
            artifact["git"], {"clean": True, "sha": GIT_SHA, "tree": GIT_TREE}
        )
        self.assertEqual(
            artifact["checks"]["ptxas_resource_entry_count"],
            assembler.EXPECTED_KERNEL_ENTRY_COUNT,
        )
        self.assertEqual(
            len(artifact["checks"]["ptxas_resources"]),
            assembler.EXPECTED_KERNEL_ENTRY_COUNT,
        )
        self.assertIs(artifact["checks"]["component_batch_repeatable"], True)
        self.assertEqual(
            artifact["binary"]["qualification_sha256"],
            hashlib.sha256(self.binary.read_bytes()).hexdigest(),
        )
        self.assertEqual(
            artifact["binary"]["audit_qualification_sha256"],
            hashlib.sha256(self.audit_binary.read_bytes()).hexdigest(),
        )
        self.assertEqual(
            artifact["binary"]["ptxas_evidence_source"],
            "targeted_cuda_audit_build_ptxas_verbose",
        )
        with self.assertRaises(SystemExit):
            assembler.validate_artifact_file(
                self.output,
                git_sha=GIT_SHA,
                git_tree="d" * 40,
                environment_artifact_path=self.environment,
            )

    def test_final_artifact_revalidation_binds_shutdown_generation_and_provenance(
        self,
    ) -> None:
        final_environment, final_artifact = self.write_final_artifacts()
        validated = self.validate_final(final_environment, final_artifact)
        self.assertEqual(validated["status"], "passed")
        self.assertEqual(validated["vm_lifecycle"]["final_status"], "TERMINATED")

        environment = json.loads(final_environment.read_text(encoding="utf-8"))
        artifact = json.loads(final_artifact.read_text(encoding="utf-8"))
        environment["vm_lifecycle"]["final_status"] = "RUNNING"
        artifact["vm_lifecycle"]["final_status"] = "RUNNING"
        write_canonical(final_environment, environment)
        artifact["provenance"]["environment_artifact_sha256"] = hashlib.sha256(
            final_environment.read_bytes()
        ).hexdigest()
        write_canonical(final_artifact, artifact)
        with self.assertRaises(SystemExit):
            self.validate_final(final_environment, final_artifact)

        final_environment, final_artifact = self.write_final_artifacts()
        environment = json.loads(final_environment.read_text(encoding="utf-8"))
        artifact = json.loads(final_artifact.read_text(encoding="utf-8"))
        changed_start = "2026-08-04T10:00:01.000-07:00"
        environment["vm_lifecycle"]["last_start_timestamp"] = changed_start
        artifact["vm_lifecycle"]["last_start_timestamp"] = changed_start
        write_canonical(final_environment, environment)
        artifact["provenance"]["environment_artifact_sha256"] = hashlib.sha256(
            final_environment.read_bytes()
        ).hexdigest()
        write_canonical(final_artifact, artifact)
        with self.assertRaises(SystemExit):
            self.validate_final(final_environment, final_artifact)

        final_environment, final_artifact = self.write_final_artifacts()
        artifact = json.loads(final_artifact.read_text(encoding="utf-8"))
        artifact["provenance"]["environment_artifact_sha256"] = "0" * 64
        write_canonical(final_artifact, artifact)
        with self.assertRaises(SystemExit):
            self.validate_final(final_environment, final_artifact)

    def test_final_lifecycle_rejects_targets_and_naive_timestamps(self) -> None:
        arguments = {
            "project": FINAL_PROJECT,
            "zone": FINAL_ZONE,
            "instance": FINAL_INSTANCE,
            "guest_shutdown_minutes": 45,
            "final_status_verified_at_utc": FINAL_VERIFIED_AT,
            "last_start_timestamp": FINAL_LAST_START,
        }
        for label, update in (
            ("project", {"project": "another-project"}),
            ("target", {"instance": "another-instance"}),
            ("naive timestamp", {"last_start_timestamp": "2026-08-04T10:00:00"}),
            ("invalid timestamp", {"last_start_timestamp": "not-a-timestamp"}),
        ):
            with self.subTest(label=label):
                mutated = dict(arguments)
                mutated.update(update)
                with self.assertRaises(SystemExit):
                    assembler.expected_final_lifecycle(**mutated)

    def test_rejects_schema6_and_terminal_matrix_mutations(self) -> None:
        mutations: list[tuple[str, dict[str, object]]] = [
            ("wrong schema", {"schema_version": 5}),
            ("wrong component", {"component_schema_version": 4}),
            ("not repeatable", {"component_batch_repeatable": False}),
            ("classification claim", {"terminal_classification_native_cuda": True}),
            ("no native geometry", {"terminal_geometry_native_cuda": False}),
            ("wrong task mass", {"task_count": 20}),
            ("wrong terminal mass", {"terminal_task_count": 8}),
            ("wrong support-3 mass", {"terminal_support_3_task_count": 3}),
            ("wrong support-4 mass", {"terminal_support_4_task_count": 4}),
            ("missing matrix cell", {"terminal_support_3_minimal_count": 0}),
            ("wrong row", {"terminal_support_4_boundary_reduced_count": 3}),
            ("wrong column", {"terminal_boundary_reduced_count": 4}),
            (
                "wrong native terminal",
                {"terminal_geometry_native_kernel_decision_count": 7},
            ),
            (
                "wrong host fallback",
                {"terminal_geometry_host_fallback_decision_count": 0},
            ),
            ("wrong rational mass", {"rational_fallback_count": 1}),
            ("floating decision", {"floating_point_decision_performed": True}),
            ("Delaunay", {"ordinary_or_higher_order_delaunay_materialized": True}),
            ("mismatch", {"mismatch_count": 1}),
            ("wrong kernels", {"kernel_launch_count": 2}),
            ("wrong synchronization", {"synchronization_count": 2}),
        ]
        for label, update in mutations:
            with self.subTest(label=label):
                value = qualification_result()
                value.update(update)
                self.rewrite_qualification(value)
                self.assert_rejected()
                self.rewrite_qualification(qualification_result())

    def test_rejects_ptxas_ptx_memcheck_and_noncanonical_evidence(self) -> None:
        original_audit = self.audit_build.read_text(encoding="utf-8")
        cases = (
            (
                "two entries",
                original_audit.split("ptxas info    : Compiling entry function", 1)[0],
            ),
            ("wrong architecture", original_audit.replace("sm_120", "sm_121", 1)),
            (
                "too many registers",
                original_audit.replace("Used 64 registers", "Used 256 registers", 1),
            ),
            (
                "build failure",
                original_audit + "ninja: build stopped: subcommand failed.\n",
            ),
        )
        for label, raw in cases:
            with self.subTest(label=label):
                self.audit_build.write_text(raw, encoding="utf-8")
                self.assert_rejected()
                self.audit_build.write_text(original_audit, encoding="utf-8")

        self.ptx.write_text("PTX file 1\n", encoding="utf-8")
        self.assert_rejected()
        self.ptx.write_text("", encoding="utf-8")

        self.qualification.write_text(
            json.dumps(qualification_result(), separators=(",", ":"))
            + "\n{}\n",
            encoding="utf-8",
        )
        self.assert_rejected()

        duplicate_backend = self.qualification.read_text(encoding="utf-8").splitlines()[
            0
        ].replace("{", '{"backend":"cuda_g4",', 1)
        self.qualification.write_text(duplicate_backend + "\n", encoding="utf-8")
        self.assert_rejected()

    def test_memcheck_may_change_timings_but_not_scientific_fields(self) -> None:
        memcheck_value = qualification_result()
        for field in assembler.TIMING_KEYS:
            memcheck_value[field] = int(memcheck_value[field]) + 1000
        memcheck_line = (
            json.dumps(memcheck_value, sort_keys=True, separators=(",", ":")) + "\n"
        )
        self.memcheck.write_text(
            "========= COMPUTE-SANITIZER\n"
            + memcheck_line
            + "========= LEAK SUMMARY: 0 bytes leaked in 0 allocations\n"
            + "========= ERROR SUMMARY: 0 errors\n",
            encoding="utf-8",
        )
        completed = self.run_assembler()
        self.assertEqual(completed.returncode, 0, completed.stderr)

        self.output.unlink()
        memcheck_value["terminal_support_4_minimal_count"] = 2
        memcheck_line = (
            json.dumps(memcheck_value, sort_keys=True, separators=(",", ":")) + "\n"
        )
        self.memcheck.write_text(
            "========= COMPUTE-SANITIZER\n"
            + memcheck_line
            + "========= ERROR SUMMARY: 0 errors\n",
            encoding="utf-8",
        )
        self.assert_rejected()

    def test_existing_output_is_never_replaced(self) -> None:
        self.output.write_text("sentinel\n", encoding="utf-8")
        completed = self.run_assembler()
        self.assertNotEqual(completed.returncode, 0)
        self.assertEqual(self.output.read_text(encoding="utf-8"), "sentinel\n")


if __name__ == "__main__":
    unittest.main()
