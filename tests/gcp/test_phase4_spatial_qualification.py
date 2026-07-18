#!/usr/bin/env python3
"""Local, mutation-free checks for the Phase 4 spatial evidence assembler."""

from __future__ import annotations

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
    / "assemble_phase4_spatial_qualification.py"
)
GIT_SHA = "a" * 40
IMAGE_ID = "sha256:" + "b" * 64
IMAGE_REF = f"morsehgp3d-phase3:{GIT_SHA}"
BASE_IMAGE_REF = (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
)
IEEE_COVERAGE = [
    "addition_only_overflow",
    "finite_subnormal_distance",
    "max_finite_query",
    "normal_subnormal_tie",
    "overflow_clamped_query",
    "subnormal_tie",
]
PROJECTION_COVERAGE = [
    "exact",
    "overflow_clamped",
    "rounded",
    "underflow",
]


def canonical_json(path: Path, value: object) -> None:
    path.write_text(
        json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n",
        encoding="utf-8",
    )


def environment_artifact() -> dict[str, object]:
    return {
        "backend": "cuda_g4",
        "git": {"clean": True, "sha": GIT_SHA},
        "image": {
            "base_ref": BASE_IMAGE_REF,
            "id": IMAGE_ID,
            "ref": IMAGE_REF,
        },
        "mode": "certified",
        "phase": "3",
        "profile": "hgp_reduced",
        "runtime_records": [
            {
                "manifest": {
                    "compiled_sm": "sm_120",
                    "cuda_driver_version_string": "12.9",
                    "gpu_compute_capability_major": 12,
                    "gpu_compute_capability_minor": 0,
                    "gpu_name": "NVIDIA RTX PRO 6000 Blackwell Server Edition",
                    "gpu_runtime_sm": "sm_120",
                    "gpu_uuid": "GPU-00000000-0000-0000-0000-000000000000",
                    "gpu_vram_bytes": 1024,
                }
            }
        ],
        "schema": "morsehgp3d.phase3.qualification.v1",
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": "environment_reproducibility_only",
        "status": "worker_passed_pending_shutdown",
    }


def differential_summary(*, quick: bool) -> dict[str, object]:
    sizes = [1, 2, 3, 4, 17, 257, 1000] if quick else list(range(1, 1001))
    case_count = 6 + len(sizes) + 7
    return {
        "all_cases_passed": True,
        "campaign_complete": not quick,
        "campaign_sizes_checked": sizes,
        "case_count": case_count,
        "closed_ball_query_count": case_count,
        "cpu_exact_recertification_complete": True,
        "decision_semantics": "cpu_exact_all_points",
        "gpu_launch_count": 2 * case_count,
        "ieee_coverage": IEEE_COVERAGE,
        "projection_coverage": PROJECTION_COVERAGE,
        "proposal_semantics": "non_certifying_fp64",
        "schema": "morsehgp3d.phase4.spatial_gpu_differential.v1",
        "scope": "quick" if quick else "full",
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "top_k_query_count": case_count,
    }


class Phase4SpatialQualificationAssemblerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.directory = Path(self.temporary_directory.name)
        self.environment = self.directory / "phase3.json"
        self.full = self.directory / "full.json"
        self.quick = self.directory / "quick.json"
        self.differential_log = self.directory / "differential.log"
        self.elf_log = self.directory / "elf.log"
        self.ptx_log = self.directory / "ptx.log"
        self.ptx_stderr_log = self.directory / "ptx.stderr.log"
        self.sanitizer_log = self.directory / "sanitizer.log"
        self.replay = self.directory / "replay"
        self.checker = self.directory / "checker.py"
        self.output = self.directory / "output.json"

        canonical_json(self.environment, environment_artifact())
        canonical_json(self.full, differential_summary(quick=False))
        canonical_json(self.quick, differential_summary(quick=True))
        self.differential_log.write_text("1013 cases passed\n", encoding="utf-8")
        self.elf_log.write_text("ELF file 1: sm_120\n", encoding="utf-8")
        self.ptx_log.write_text("", encoding="utf-8")
        self.ptx_stderr_log.write_text("", encoding="utf-8")
        self.sanitizer_log.write_text(
            "========= ERROR SUMMARY: 0 errors\n", encoding="utf-8"
        )
        self.replay.write_bytes(b"replay")
        self.checker.write_bytes(b"checker")
        self.output.touch()

    def run_assembler(self) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
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
                "--differential-summary",
                str(self.full),
                "--quick-summary",
                str(self.quick),
                "--differential-log",
                str(self.differential_log),
                "--elf-log",
                str(self.elf_log),
                "--ptx-log",
                str(self.ptx_log),
                "--ptx-stderr-log",
                str(self.ptx_stderr_log),
                "--sanitizer-log",
                str(self.sanitizer_log),
                "--replay",
                str(self.replay),
                "--checker",
                str(self.checker),
                "--output",
                str(self.output),
            ],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )

    def test_current_differential_schema_assembles(self) -> None:
        completed = self.run_assembler()

        self.assertEqual(0, completed.returncode, completed.stdout)
        artifact = json.loads(self.output.read_text(encoding="utf-8"))
        self.assertEqual(
            "morsehgp3d.phase4.spatial_gpu_reference_qualification.v1",
            artifact["schema"],
        )
        self.assertEqual(1013, artifact["checks"]["differential"]["case_count"])
        self.assertEqual(
            20,
            artifact["checks"]["quick_memcheck_differential"]["case_count"],
        )
        self.assertEqual(
            PROJECTION_COVERAGE,
            artifact["checks"]["differential"]["projection_coverage"],
        )
        self.assertEqual(
            IEEE_COVERAGE,
            artifact["checks"]["differential"]["ieee_coverage"],
        )
        self.assertFalse(artifact["scientific_result_claimed"])
        self.assertIsNone(artifact["scientific_public_status"])

    def test_stale_pre_overflow_summary_is_rejected(self) -> None:
        stale = differential_summary(quick=False)
        stale.pop("ieee_coverage")
        stale["projection_coverage"] = ["exact", "rounded", "underflow"]
        stale["case_count"] = 1008
        stale["closed_ball_query_count"] = 1008
        stale["top_k_query_count"] = 1008
        stale["gpu_launch_count"] = 2016
        canonical_json(self.full, stale)

        completed = self.run_assembler()

        self.assertNotEqual(0, completed.returncode)
        self.assertIn("full differential summary keys differ", completed.stdout)

    def test_wrong_current_case_count_is_rejected(self) -> None:
        wrong = differential_summary(quick=True)
        wrong["case_count"] = 19
        canonical_json(self.quick, wrong)

        completed = self.run_assembler()

        self.assertNotEqual(0, completed.returncode)
        self.assertIn("quick differential summary.case_count", completed.stdout)


if __name__ == "__main__":
    unittest.main()
