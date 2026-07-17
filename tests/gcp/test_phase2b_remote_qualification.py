from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
ASSEMBLER = (
    REPOSITORY_ROOT
    / "morsehgp3d"
    / "tests"
    / "cuda"
    / "assemble_phase2b_qualification.py"
)
WORKER = REPOSITORY_ROOT / "gcp-migration" / "phase2b_remote_qualification.sh"
GIT_SHA = "1" * 40
IMAGE_ID = "sha256:" + "2" * 64


def benchmark_record(predicate: str) -> dict[str, object]:
    return {
        "input_bytes": 1024,
        "median_elapsed": {
            "cases_per_second": 1000.0,
            "seconds": 1.0,
            "stdin_bytes_per_second": 1024.0,
        },
        "minimum_elapsed": {
            "cases_per_second": 1100.0,
            "seconds": 0.9,
            "stdin_bytes_per_second": 1126.4,
        },
        "predicate": predicate,
    }


class Phase2BQualificationAssemblerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.output = self.root / "qualification.json"
        self.paths = {
            "distance": self.root / "distance.json",
            "orientation": self.root / "orientation.json",
            "power": self.root / "power.json",
            "benchmark": self.root / "benchmark.json",
            "elf": self.root / "elf.txt",
            "ptx": self.root / "ptx.txt",
            "sanitizer_distance": self.root / "sanitizer-distance.txt",
            "sanitizer_orientation": self.root / "sanitizer-orientation.txt",
            "sanitizer_power": self.root / "sanitizer-power.txt",
        }
        self.values: dict[str, dict[str, object]] = {
            "distance": {
                "cases": 2064,
                "gpu_fp64_certified": 2061,
                "gpu_known_audited": 2061,
                "gpu_unknown_forwarded": 3,
                "remaining_unknown": 0,
                "schema": "morsehgp3d.phase2b.distance_filter.v1",
            },
            "orientation": {
                "cases": 2070,
                "gpu_fp64_certified": 2067,
                "gpu_known_audited": 2067,
                "gpu_unknown_forwarded": 3,
                "remaining_unknown": 0,
                "schema": "morsehgp3d.phase2b.orientation_3d_filter.v1",
            },
            "power": {
                "cases": 2077,
                "exact_zeros": 4,
                "gpu_fp64_certified": 2065,
                "gpu_known_audited": 2065,
                "gpu_unknown_forwarded": 12,
                "remaining_unknown": 0,
                "schema": "morsehgp3d.phase2b.power_bisector_side_filter.v1",
            },
            "benchmark": {
                "cases": 262144,
                "repeats": 3,
                "results": {
                    "distance": benchmark_record("compare_squared_distances"),
                    "orientation": benchmark_record("orientation_3d"),
                    "power": benchmark_record("power_bisector_side"),
                },
                "schema": "morsehgp3d.phase2b.predicates.benchmark.v1",
                "scope": "cold_process_end_to_end",
            },
        }
        self.write_evidence()

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def write_evidence(self) -> None:
        for name, value in self.values.items():
            self.paths[name].write_text(
                json.dumps(value, separators=(",", ":"), sort_keys=True) + "\n",
                encoding="utf-8",
            )
        self.paths["elf"].write_text("architecture = sm_120\n", encoding="utf-8")
        self.paths["ptx"].write_text("", encoding="utf-8")
        for name in (
            "sanitizer_distance",
            "sanitizer_orientation",
            "sanitizer_power",
        ):
            self.paths[name].write_text(
                "========= ERROR SUMMARY: 0 errors\n", encoding="utf-8"
            )

    def run_assembler(self) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                str(ASSEMBLER),
                "--git-sha",
                GIT_SHA,
                "--image-id",
                IMAGE_ID,
                "--distance-summary",
                str(self.paths["distance"]),
                "--orientation-summary",
                str(self.paths["orientation"]),
                "--power-summary",
                str(self.paths["power"]),
                "--benchmark-summary",
                str(self.paths["benchmark"]),
                "--elf-log",
                str(self.paths["elf"]),
                "--ptx-log",
                str(self.paths["ptx"]),
                "--sanitizer-distance-log",
                str(self.paths["sanitizer_distance"]),
                "--sanitizer-orientation-log",
                str(self.paths["sanitizer_orientation"]),
                "--sanitizer-power-log",
                str(self.paths["sanitizer_power"]),
                "--output",
                str(self.output),
            ],
            check=False,
            capture_output=True,
            text=True,
        )

    def test_valid_artifact_contains_three_predicates(self) -> None:
        result = self.run_assembler()
        self.assertEqual(result.returncode, 0, result.stderr)
        artifact = json.loads(self.output.read_text(encoding="utf-8"))
        self.assertEqual(artifact["status"], "worker_passed_pending_shutdown")
        self.assertIsNone(artifact["public_status"])
        self.assertIs(artifact["scientific_result_claimed"], False)
        self.assertEqual(
            artifact["predicates"],
            [
                "compare_squared_distances",
                "orientation_3d",
                "power_bisector_side",
            ],
        )
        self.assertEqual(
            artifact["checks"]["differentials"]["power_bisector_side"][
                "exact_zeros"
            ],
            4,
        )
        self.assertEqual(
            artifact["checks"]["compute_sanitizer_memcheck"][
                "power_bisector_side"
            ],
            "passed",
        )
        self.assertNotIn("vm_lifecycle", artifact)

    def test_rejects_power_counter_outside_qualified_range(self) -> None:
        self.values["power"]["gpu_fp64_certified"] = 2064
        self.values["power"]["gpu_known_audited"] = 2064
        self.values["power"]["gpu_unknown_forwarded"] = 13
        self.write_evidence()
        result = self.run_assembler()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("[2065, 2071]", result.stderr)
        self.assertFalse(self.output.exists())

    def test_rejects_missing_power_memcheck_evidence(self) -> None:
        self.paths["sanitizer_power"].write_text(
            "========= ERROR SUMMARY: 1 error\n", encoding="utf-8"
        )
        result = self.run_assembler()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("zero-error summary", result.stderr)
        self.assertFalse(self.output.exists())

    def test_refuses_to_replace_existing_artifact(self) -> None:
        self.output.write_text("sentinel\n", encoding="utf-8")
        result = self.run_assembler()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.output.read_text(encoding="utf-8"), "sentinel\n")


class Phase2BWorkerStaticSafetyTests(unittest.TestCase):
    def test_worker_is_syntactically_valid_and_non_mutating(self) -> None:
        syntax = subprocess.run(
            ["bash", "-n", str(WORKER)], check=False, capture_output=True, text=True
        )
        self.assertEqual(syntax.returncode, 0, syntax.stderr)
        source = WORKER.read_text(encoding="utf-8")
        self.assertNotIn("gcloud", source)
        self.assertNotIn("compute instances", source)
        self.assertIn("--expected-sha", source)
        self.assertIn("--gce-deadline-epoch", source)
        self.assertIn("--cidfile", source)
        self.assertIn("cleanup_active_container", source)
        self.assertIn(
            'REPOSITORY_BUILD_MOUNTPOINT="${REPOSITORY_ROOT}/build"', source
        )
        self.assertIn('mkdir -- "${REPOSITORY_BUILD_MOUNTPOINT}"', source)
        self.assertIn('rmdir -- "${REPOSITORY_BUILD_MOUNTPOINT}"', source)
        self.assertIn("readonly WORK_DEADLINE_RESERVE_SECONDS=1800", source)
        self.assertIn("--predicate all", source)
        self.assertIn("worker_passed_pending_shutdown", ASSEMBLER.read_text(encoding="utf-8"))

    def test_help_does_not_require_docker_or_gcp(self) -> None:
        result = subprocess.run(
            ["bash", str(WORKER), "--help"],
            check=False,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("ne démarre", result.stdout)


if __name__ == "__main__":
    unittest.main()
