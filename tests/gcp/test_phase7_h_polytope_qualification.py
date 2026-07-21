#!/usr/bin/env python3
"""Short mutation tests for the Phase 7.8 H-polytope G4 assembler."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
ASSEMBLER = (
    ROOT
    / "morsehgp3d"
    / "tests"
    / "cuda"
    / "assemble_phase7_h_polytope_qualification.py"
)
GIT_SHA = "a" * 40
IMAGE_ID = "sha256:" + "b" * 64
IMAGE_REF = f"morsehgp3d-phase3:{GIT_SHA}"
BASE_IMAGE_REF = (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
)
LIFECYCLE = {
    "guest_shutdown_guard_verified": True,
    "stop_responsibility": "external_orchestrator",
    "worker_mutates_gcp": False,
}


def canonical_json(path: Path, value: object) -> None:
    path.write_text(
        json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n",
        encoding="utf-8",
    )


def qualification_result() -> dict[str, object]:
    return {
        "cell_count": 4,
        "deterministic": True,
        "exact_cpu_recertification": True,
        "first_epoch": 1,
        "proposal_digest_fnv1a": (1 << 64) - 1,
        "record_count": 55,
        "schema": "morsehgp3d.phase7.h_polytope_cuda_qualification.v1",
        "second_epoch": 2,
        "strict_reject_count": 4,
        "survivor_count": 16,
        "unknown_count": 35,
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
        "schema": "morsehgp3d.phase3.qualification.v1",
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": "environment_reproducibility_only",
        "status": "worker_passed_pending_shutdown",
        "vm_lifecycle": dict(LIFECYCLE),
    }


class Phase7HPolytopeQualificationAssemblerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.directory = Path(self.temporary_directory.name)
        self.environment = self.directory / "phase3.json"
        self.qualification = self.directory / "qualification.json"
        self.elf = self.directory / "elf.log"
        self.ptx = self.directory / "ptx.log"
        self.ptx_stderr = self.directory / "ptx.stderr.log"
        self.memcheck = self.directory / "memcheck.log"
        self.racecheck = self.directory / "racecheck.log"
        self.binary = self.directory / "qualification-binary"
        self.output = self.directory / "artifact.json"

        canonical_json(self.environment, environment_artifact())
        canonical_json(self.qualification, qualification_result())
        self.elf.write_text(
            "Fatbin elf code:\narch = sm_120\n", encoding="utf-8"
        )
        self.ptx.write_text("", encoding="utf-8")
        self.ptx_stderr.write_text(
            "cuobjdump diagnostic: no PTX entries\n", encoding="utf-8"
        )
        self.memcheck.write_text(
            "========= ERROR SUMMARY: 0 errors\n", encoding="utf-8"
        )
        self.racecheck.write_text(
            "========= RACECHECK SUMMARY: 0 hazards displayed "
            "(0 errors, 0 warnings)\n",
            encoding="utf-8",
        )
        self.binary.write_bytes(b"phase7-h-polytope-cuda-qualification")

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
                "--base-image-ref",
                BASE_IMAGE_REF,
                "--image-ref",
                IMAGE_REF,
                "--image-id",
                IMAGE_ID,
                "--environment-artifact",
                str(self.environment),
                "--qualification-log",
                str(self.qualification),
                "--elf-log",
                str(self.elf),
                "--ptx-log",
                str(self.ptx),
                "--ptx-stderr-log",
                str(self.ptx_stderr),
                "--memcheck-log",
                str(self.memcheck),
                "--racecheck-log",
                str(self.racecheck),
                "--binary",
                str(self.binary),
                "--output",
                str(output or self.output),
            ],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def assert_rejected(self) -> None:
        completed = self.run_assembler()
        self.assertNotEqual(completed.returncode, 0, completed.stdout)
        self.assertFalse(self.output.exists())

    def test_valid_evidence_is_assembled_with_hashes_and_no_claim(self) -> None:
        completed = self.run_assembler()
        self.assertEqual(completed.returncode, 0, completed.stderr)
        artifact = json.loads(self.output.read_text(encoding="utf-8"))
        self.assertEqual(
            set(artifact),
            {
                "backend",
                "binary",
                "checks",
                "decision_semantics",
                "git",
                "image",
                "log_sha256",
                "logs",
                "mode",
                "phase",
                "profile",
                "proposal_semantics",
                "provenance",
                "schema",
                "scientific_public_status",
                "scientific_result_claimed",
                "scientific_scope",
                "status",
                "vm_lifecycle",
            },
        )
        self.assertEqual(
            artifact["schema"],
            "morsehgp3d.phase7.h_polytope_cuda_g4_qualification.v1",
        )
        self.assertEqual(
            (artifact["phase"], artifact["backend"], artifact["profile"]),
            ("7", "cuda_g4", "generic_core"),
        )
        self.assertEqual(artifact["mode"], "benchmark_only")
        self.assertEqual(
            artifact["scientific_scope"], "proposal_only_h_polytope_transcript"
        )
        self.assertEqual(
            artifact["proposal_semantics"],
            "proposal_only_exhaustive_plane_triple_transcript",
        )
        self.assertEqual(
            artifact["decision_semantics"],
            "reference_cpu_exact_all_constraints",
        )
        self.assertIs(artifact["scientific_result_claimed"], False)
        self.assertIsNone(artifact["scientific_public_status"])
        self.assertEqual(artifact["vm_lifecycle"], LIFECYCLE)
        self.assertEqual(artifact["checks"]["qualification"], qualification_result())
        self.assertEqual(artifact["checks"]["aot_elf_architectures"], ["sm_120"])
        self.assertEqual(artifact["checks"]["cuda_audit_workflow"], "passed")
        self.assertEqual(artifact["checks"]["cuda_release_workflow"], "passed")
        self.assertEqual(
            artifact["binary"]["qualification_sha256"],
            hashlib.sha256(self.binary.read_bytes()).hexdigest(),
        )
        self.assertEqual(
            artifact["provenance"]["environment_artifact_sha256"],
            hashlib.sha256(self.environment.read_bytes()).hexdigest(),
        )
        for name, raw in artifact["logs"].items():
            self.assertEqual(
                artifact["log_sha256"][name],
                hashlib.sha256(raw.encode("utf-8")).hexdigest(),
            )

    def test_rejects_wrong_analytic_shape_counts_epochs_and_digest(self) -> None:
        mutations = {
            "extra key": {"unexpected": 1},
            "zero category": {"unknown_count": 0},
            "wrong partition": {"unknown_count": 34},
            "wrong epoch": {"second_epoch": 3},
            "nondeterministic": {"deterministic": False},
            "digest overflow": {"proposal_digest_fnv1a": 1 << 64},
        }
        for label, mutation in mutations.items():
            with self.subTest(label=label):
                value = qualification_result()
                value.update(mutation)
                canonical_json(self.qualification, value)
                self.assert_rejected()
                canonical_json(self.qualification, qualification_result())

    def test_rejects_multiple_objects_and_duplicate_keys(self) -> None:
        valid = json.dumps(qualification_result(), separators=(",", ":"))
        for label, raw in (
            ("multiple", valid + "\n" + valid + "\n"),
            (
                "duplicate",
                valid[:-1] + ',"cell_count":4}\n',
            ),
        ):
            with self.subTest(label=label):
                self.qualification.write_text(raw, encoding="utf-8")
                self.assert_rejected()

    def test_rejects_non_sm120_elf_and_nonempty_ptx(self) -> None:
        self.elf.write_text("arch = sm_120\narch = sm_90\n", encoding="utf-8")
        self.assert_rejected()
        self.elf.write_text("arch = sm_120\n", encoding="utf-8")
        self.ptx.write_text(".version 8.8\n", encoding="utf-8")
        self.assert_rejected()

    def test_rejects_memcheck_error_or_failure_marker(self) -> None:
        for evidence in (
            "========= ERROR SUMMARY: 1 error\n",
            "========= ERROR SUMMARY: 0 errors\n"
            "========= Target application returned an error\n",
            "========= Invalid __global__ read of size 8 bytes\n"
            "========= ERROR SUMMARY: 0 errors\n",
        ):
            with self.subTest(evidence=evidence):
                self.memcheck.write_text(evidence, encoding="utf-8")
                self.assert_rejected()

    def test_rejects_racecheck_hazard_or_failure_marker(self) -> None:
        for evidence in (
            "========= RACECHECK SUMMARY: 1 hazard displayed "
            "(1 error, 0 warnings)\n",
            "========= RACECHECK SUMMARY: 0 hazards displayed "
            "(0 errors, 0 warnings)\n"
            "========= Target application returned an error\n",
        ):
            with self.subTest(evidence=evidence):
                self.racecheck.write_text(evidence, encoding="utf-8")
                self.assert_rejected()

    def test_rejects_environment_identity_or_lifecycle_mutation(self) -> None:
        value = environment_artifact()
        value["git"] = {"clean": False, "sha": GIT_SHA}
        canonical_json(self.environment, value)
        self.assert_rejected()
        value = environment_artifact()
        value["vm_lifecycle"] = {
            **LIFECYCLE,
            "guest_shutdown_guard_verified": False,
        }
        canonical_json(self.environment, value)
        self.assert_rejected()

    def test_rejects_environment_without_both_cuda_workflows(self) -> None:
        for workflow in ("cuda_audit_workflow", "cuda_release_workflow"):
            with self.subTest(workflow=workflow):
                value = environment_artifact()
                value["checks"][workflow] = "failed"
                canonical_json(self.environment, value)
                self.assert_rejected()

    def test_rejects_symlinked_binary_and_existing_or_symlinked_output(self) -> None:
        real_binary = self.directory / "real-binary"
        self.binary.rename(real_binary)
        self.binary.symlink_to(real_binary)
        self.assert_rejected()
        self.binary.unlink()
        self.binary.write_bytes(b"qualification")

        self.output.write_text("preserve me\n", encoding="utf-8")
        completed = self.run_assembler()
        self.assertNotEqual(completed.returncode, 0)
        self.assertEqual(self.output.read_text(encoding="utf-8"), "preserve me\n")
        self.output.unlink()

        target = self.directory / "target.json"
        target.write_text("preserve target\n", encoding="utf-8")
        self.output.symlink_to(target)
        completed = self.run_assembler()
        self.assertNotEqual(completed.returncode, 0)
        self.assertEqual(target.read_text(encoding="utf-8"), "preserve target\n")


if __name__ == "__main__":
    unittest.main()
