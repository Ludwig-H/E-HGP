#!/usr/bin/env python3
"""Local, mutation-free checks for the Phase 4 spatial evidence assembler."""

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
LBVH_DIRECTED_ENCLOSURE_COVERAGE = [
    "enclosed",
    "exact",
    "unsupported_range",
]
LBVH_TARGETED_COVERAGE = [
    "addition_only_overflow",
    "cutoff_non_binary64",
    "cutoff_outside_binary64",
    "exact_tie",
    "exclusions",
    "maximum_finite",
    "negative_query_outside_binary64",
    "permuted_input",
    "query_non_binary64",
    "query_outside_binary64",
    "signed_subnormal",
    "singleton",
    "six_way_shell",
    "tri_partition",
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


def lbvh_differential_summary() -> dict[str, object]:
    return {
        "all_cases_passed": True,
        "bounded_protocol": True,
        "case_count": 13,
        "certified_pruned_subtree_count": 15,
        "closed_ball_query_count": 13,
        "cover_antichain_complete": True,
        "cpu_exact_recertification_complete": True,
        "decision_semantics": "cpu_exact_cover_and_leaf_recertification",
        "directed_enclosure_coverage": LBVH_DIRECTED_ENCLOSURE_COVERAGE,
        "exact_partition_complete": True,
        "gpu_launch_count": 19,
        "point_partition_complete": True,
        "proposal_semantics": "gpu_resident_lbvh_strict_exterior_cover",
        "schema": "morsehgp3d.phase4.spatial_gpu_lbvh_differential.v1",
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "targeted_coverage": LBVH_TARGETED_COVERAGE,
        "top_k_query_count": 13,
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
        self.lbvh_differential_summary = self.directory / "lbvh-differential.json"
        self.lbvh_memcheck_summary = self.directory / "lbvh-memcheck.json"
        self.lbvh_differential_log = self.directory / "lbvh-differential.log"
        self.lbvh_elf_log = self.directory / "lbvh-elf.log"
        self.lbvh_ptx_log = self.directory / "lbvh-ptx.log"
        self.lbvh_ptx_stderr_log = self.directory / "lbvh-ptx.stderr.log"
        self.lbvh_sanitizer_log = self.directory / "lbvh-sanitizer.log"
        self.lbvh_replay = self.directory / "lbvh-replay"
        self.lbvh_checker = self.directory / "lbvh-checker.py"
        self.output = self.directory / "output.json"

        canonical_json(self.environment, environment_artifact())
        canonical_json(self.full, differential_summary(quick=False))
        canonical_json(self.quick, differential_summary(quick=True))
        canonical_json(self.lbvh_differential_summary, lbvh_differential_summary())
        canonical_json(self.lbvh_memcheck_summary, lbvh_differential_summary())
        self.differential_log.write_text("1013 cases passed\n", encoding="utf-8")
        self.elf_log.write_text("ELF file 1: sm_120\n", encoding="utf-8")
        self.ptx_log.write_text("", encoding="utf-8")
        self.ptx_stderr_log.write_text("", encoding="utf-8")
        self.sanitizer_log.write_text(
            "========= ERROR SUMMARY: 0 errors\n", encoding="utf-8"
        )
        self.replay.write_bytes(b"replay")
        self.checker.write_bytes(b"checker")
        self.lbvh_differential_log.write_text(
            "13 resident LBVH cases passed\n", encoding="utf-8"
        )
        self.lbvh_elf_log.write_text(
            "ELF file 1: resident-lbvh.sm_120.cubin\n", encoding="utf-8"
        )
        self.lbvh_ptx_log.write_text("", encoding="utf-8")
        self.lbvh_ptx_stderr_log.write_text("", encoding="utf-8")
        self.lbvh_sanitizer_log.write_text(
            "========= ERROR SUMMARY: 0 errors\n", encoding="utf-8"
        )
        self.lbvh_replay.write_bytes(b"resident LBVH replay")
        self.lbvh_checker.write_bytes(b"resident LBVH checker")
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
                "--lbvh-differential-summary",
                str(self.lbvh_differential_summary),
                "--lbvh-memcheck-summary",
                str(self.lbvh_memcheck_summary),
                "--lbvh-differential-log",
                str(self.lbvh_differential_log),
                "--lbvh-elf-log",
                str(self.lbvh_elf_log),
                "--lbvh-ptx-log",
                str(self.lbvh_ptx_log),
                "--lbvh-ptx-stderr-log",
                str(self.lbvh_ptx_stderr_log),
                "--lbvh-sanitizer-log",
                str(self.lbvh_sanitizer_log),
                "--lbvh-replay",
                str(self.lbvh_replay),
                "--lbvh-checker",
                str(self.lbvh_checker),
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
            "morsehgp3d.phase4.spatial_gpu_reference_and_lbvh_qualification.v2",
            artifact["schema"],
        )
        self.assertEqual(
            {
                "checker_sha256",
                "lbvh_checker_sha256",
                "lbvh_replay_sha256",
                "replay_sha256",
            },
            set(artifact["binary"]),
        )
        self.assertEqual(
            hashlib.sha256(self.replay.read_bytes()).hexdigest(),
            artifact["binary"]["replay_sha256"],
        )
        self.assertEqual(
            hashlib.sha256(self.checker.read_bytes()).hexdigest(),
            artifact["binary"]["checker_sha256"],
        )
        self.assertEqual(
            hashlib.sha256(self.lbvh_replay.read_bytes()).hexdigest(),
            artifact["binary"]["lbvh_replay_sha256"],
        )
        self.assertEqual(
            hashlib.sha256(self.lbvh_checker.read_bytes()).hexdigest(),
            artifact["binary"]["lbvh_checker_sha256"],
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
        self.assertEqual(
            13, artifact["checks"]["lbvh_differential"]["case_count"]
        )
        self.assertEqual(
            19, artifact["checks"]["lbvh_differential"]["gpu_launch_count"]
        )
        self.assertEqual(
            15,
            artifact["checks"]["lbvh_differential"][
                "certified_pruned_subtree_count"
            ],
        )
        self.assertEqual(
            artifact["checks"]["lbvh_differential"],
            artifact["checks"]["lbvh_memcheck_differential"],
        )
        self.assertEqual(
            ["sm_120"], artifact["checks"]["lbvh_aot_elf_architectures"]
        )
        self.assertEqual(0, artifact["checks"]["lbvh_aot_ptx_entry_count"])
        self.assertEqual("passed", artifact["checks"]["lbvh_compute_sanitizer"])
        self.assertTrue(
            artifact["checks"]["lbvh_cpu_exact_recertification_complete"]
        )
        self.assertEqual(
            {
                "compute_sanitizer",
                "cuobjdump_elf",
                "cuobjdump_ptx",
                "cuobjdump_ptx_stderr",
                "differential",
                "lbvh_compute_sanitizer",
                "lbvh_cuobjdump_elf",
                "lbvh_cuobjdump_ptx",
                "lbvh_cuobjdump_ptx_stderr",
                "lbvh_differential",
            },
            set(artifact["logs"]),
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

    def test_incomplete_lbvh_summary_is_rejected(self) -> None:
        incomplete = lbvh_differential_summary()
        incomplete["directed_enclosure_coverage"] = ["enclosed", "exact"]
        canonical_json(self.lbvh_differential_summary, incomplete)

        completed = self.run_assembler()

        self.assertNotEqual(0, completed.returncode)
        self.assertIn(
            "resident LBVH differential summary.directed_enclosure_coverage "
            "is incomplete",
            completed.stdout,
        )

    def test_lbvh_ptx_entry_is_rejected(self) -> None:
        self.lbvh_ptx_log.write_text(
            "PTX file 1: resident-lbvh.compute_120.ptx\n", encoding="utf-8"
        )

        completed = self.run_assembler()

        self.assertNotEqual(0, completed.returncode)
        self.assertIn(
            "resident LBVH replay PTX evidence must contain zero entries",
            completed.stdout,
        )

    def test_lbvh_sanitizer_error_is_rejected(self) -> None:
        self.lbvh_sanitizer_log.write_text(
            "========= ERROR SUMMARY: 1 errors\n", encoding="utf-8"
        )

        completed = self.run_assembler()

        self.assertNotEqual(0, completed.returncode)
        self.assertIn(
            "resident LBVH replay compute-sanitizer evidence must contain only "
            "zero errors",
            completed.stdout,
        )

    def test_symlinked_lbvh_replay_is_rejected_before_hashing(self) -> None:
        self.lbvh_replay.unlink()
        self.lbvh_replay.symlink_to(self.replay)

        completed = self.run_assembler()

        self.assertNotEqual(0, completed.returncode)
        self.assertIn(
            "resident LBVH replay must be a regular non-symbolic file",
            completed.stdout,
        )


if __name__ == "__main__":
    unittest.main()
