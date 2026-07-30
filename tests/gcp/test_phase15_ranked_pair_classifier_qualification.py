#!/usr/bin/env python3
"""Mutation tests for the guarded Phase 15 rank-2/3 CUDA qualification."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
CUDA_TESTS = ROOT / "morsehgp3d" / "tests" / "cuda"
ASSEMBLER = CUDA_TESTS / "assemble_phase15_ranked_pair_classifier_qualification.py"
REMOTE_SCRIPT = ROOT / "gcp-migration" / "phase3_remote_qualification.sh"
ORCHESTRATOR_SCRIPT = ROOT / "gcp-migration" / "run_phase3_qualification.sh"
sys.path.insert(0, str(CUDA_TESTS))

import assemble_phase15_ranked_pair_classifier_qualification as assembler


GIT_SHA = "a" * 40
IMAGE_ID = "sha256:" + "b" * 64
IMAGE_REF = f"morsehgp3d-phase3:{GIT_SHA}"
BASE_IMAGE_REF = assembler.BASE_IMAGE_REF
LIFECYCLE = dict(assembler.WORKER_LIFECYCLE)


def canonical_json(path: Path, value: object) -> None:
    path.write_text(
        json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
        + "\n",
        encoding="utf-8",
    )


def qualification_result(*, single_run: bool) -> dict[str, object]:
    return {
        "anchor_tile_capacities": [17] if single_run else [1, 17],
        "backend": assembler.BACKEND,
        "bounded_cpu_oracle": True,
        "catalog_digest_fnv1a": 14_695_981_039_346_656_037,
        "closed_ranks_qualified": [2, 3],
        "component_only": True,
        "deployment_status": assembler.DEPLOYMENT_STATUS,
        "deterministic_dyadic_fixture": True,
        "exact_fallback_count": 0,
        "exact_level_persisted_in_gpu_soa": False,
        "exact_levels_divide_by_four_recomputed": True,
        "exact_levels_match_cpu_oracle": True,
        "fixture_clustered_point_count": 128,
        "fixture_exact_shell_point_count": 3,
        "fixture_jitter_grid_point_count": 75,
        "fixture_strict_collinear_point_count": 3,
        "full_gamma2_claimed": False,
        "full_k2_hierarchy_claimed": False,
        "git_sha": GIT_SHA,
        "global_cell_or_coface_arena_materialized": False,
        "global_pair_matrix_materialized": False,
        "hierarchy_reduction_performed": False,
        "independent_all_pair_all_witness_oracle": True,
        "industrial_scale_claimed": False,
        "intermediate_candidate_d2h": False,
        "maximum_closed_rank": assembler.MAXIMUM_CLOSED_RANK,
        "maximum_oracle_point_count": assembler.MAXIMUM_ORACLE_POINT_COUNT,
        "mode": assembler.MODE,
        "morse_hgp_hierarchy_claimed": False,
        "ordinary_delaunay_materialized": False,
        "point_count": assembler.POINT_COUNT,
        "profile": assembler.PROFILE,
        "public_pipeline_lbvh_frontier_classifier_finish": True,
        "public_terminal_wrapper_and_lease_lifetime_qualified": True,
        "public_status": assembler.PUBLIC_STATUS,
        "qualification_capacity_source": "bounded_cpu_oracle_only",
        "qualification_final_catalog_and_source_d2h": True,
        "rank_three_record_count": 321,
        "rank_two_record_count": 654,
        "requested_orders": [1, 2],
        "run_count": 1 if single_run else 2,
        "schema": assembler.QUALIFICATION_SCHEMA,
        "shell_payload_point_id_count": 17,
        "single_run": single_run,
        "strict_payload_point_id_count": 999,
        "success": True,
        "support2_rank23_catalog_qualified": True,
        "tile_capacity_invariance_qualified": not single_run,
        "total_commit_count": 17 if single_run else 275,
        "zero_fallback_required_for_success": True,
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


class Phase15RankedPairClassifierScriptWiringTests(unittest.TestCase):
    @staticmethod
    def assert_markers_in_order(raw: str, markers: tuple[str, ...]) -> None:
        cursor = 0
        for marker in markers:
            position = raw.find(marker, cursor)
            if position < 0:
                raise AssertionError(f"missing or out-of-order marker: {marker}")
            cursor = position + len(marker)

    def test_worker_uses_the_closed_native_rank23_path(self) -> None:
        remote = REMOTE_SCRIPT.read_text(encoding="utf-8")
        start = remote.index('begin_unit "phase15-ranked-pair-classifier-release-build"')
        finish = remote.index('if [[ -n "${PHASE4_OUTPUT_PATH}" ]]; then', start)
        execution = remote[start:finish]

        self.assertIn(
            "--phase15-ranked-pair-classifier-output est exclusive des autres compagnons.",
            remote,
        )
        self.assert_markers_in_order(
            remote,
            (
                "read_guest_shutdown_guard ||",
                'begin_unit "phase15-ranked-pair-classifier-release-build"',
                'begin_unit "phase15-ranked-pair-classifier-audit-build"',
                'begin_unit "phase15-ranked-pair-classifier-qualification"',
                'begin_unit "phase15-ranked-pair-classifier-cuobjdump-elf"',
                'begin_unit "phase15-ranked-pair-classifier-cuobjdump-ptx"',
                'begin_unit "phase15-ranked-pair-classifier-memcheck"',
                'begin_unit "phase15-ranked-pair-classifier-racecheck"',
                'python3 -B "${PHASE15_RANKED_PAIR_CLASSIFIER_ASSEMBLER}"',
                'rm -f -- "${PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP}"',
            ),
        )
        self.assertIn(
            "morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_qualification",
            execution,
        )
        self.assertEqual(execution.count("--single-run"), 2)
        self.assertNotIn("--point-count 50000", execution)
        self.assertNotIn("--scaling-smoke", execution)
        self.assertNotIn("--direct-scale", execution)

    def test_orchestrator_validates_before_stop_and_publishes_after_stop(self) -> None:
        orchestrator = ORCHESTRATOR_SCRIPT.read_text(encoding="utf-8")
        self.assertIn(
            "--phase15-ranked-pair-classifier est mutuellement exclusive de tous les autres compagnons.",
            orchestrator,
        )
        self.assertIn(
            'rm -f -- "${LOCAL_PHASE15_RANKED_PAIR_CLASSIFIER_TEMP_RESULT}" || true',
            orchestrator,
        )
        self.assert_markers_in_order(
            orchestrator,
            (
                '"${INSTANCE_NAME}:${remote_phase15_ranked_pair_classifier_artifact}"',
                "import assemble_phase15_ranked_pair_classifier_qualification as assembler",
                "assembler.validate_artifact_file(",
                "if certify_target_stopped; then",
                '"${LOCAL_PHASE15_RANKED_PAIR_CLASSIFIER_TEMP_RESULT}"',
                "os.link(temporary, target, follow_symlinks=False)",
                'rm -f -- "${LOCAL_PHASE15_RANKED_PAIR_CLASSIFIER_TEMP_RESULT}"',
            ),
        )


class Phase15RankedPairClassifierAssemblerTests(unittest.TestCase):
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
        self.racecheck = self.directory / "racecheck.log"
        self.binary = self.directory / "qualification-binary"
        self.output = self.directory / "artifact.json"

        canonical_json(self.environment, environment_artifact())
        canonical_json(self.qualification, qualification_result(single_run=False))
        self.qualification_stderr.write_text("", encoding="utf-8")
        self.release_build.write_text(
            "[1/2] Building ranked-pair classifier\n[2/2] Linking qualification\n",
            encoding="utf-8",
        )
        self.audit_build.write_text(
            "ptxas info : Compiling entry function for sm_120\n"
            "[2/2] Linking audit qualification\n",
            encoding="utf-8",
        )
        self.elf.write_text("Fatbin elf code:\narch = sm_120\n", encoding="utf-8")
        self.ptx.write_text("", encoding="utf-8")
        self.ptx_stderr.write_text(
            "cuobjdump diagnostic: no PTX entries\n", encoding="utf-8"
        )
        sanitizer_json = json.dumps(
            qualification_result(single_run=True),
            ensure_ascii=False,
            sort_keys=True,
            separators=(",", ":"),
        )
        self.memcheck.write_text(
            sanitizer_json + "\n========= ERROR SUMMARY: 0 errors\n",
            encoding="utf-8",
        )
        self.racecheck.write_text(
            sanitizer_json
            + "\n========= RACECHECK SUMMARY: 0 hazards displayed "
            "(0 errors, 0 warnings)\n",
            encoding="utf-8",
        )
        self.binary.write_bytes(b"phase15-ranked-pair-classifier-qualification")

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

    def validate_output(self) -> dict[str, object]:
        return assembler.validate_artifact_file(
            self.output,
            git_sha=GIT_SHA,
            environment_artifact_path=self.environment,
        )

    def test_valid_evidence_is_component_only_pending_targeted_shutdown(self) -> None:
        completed = self.run_assembler()
        self.assertEqual(completed.returncode, 0, completed.stderr)
        artifact = self.validate_output()
        self.assertEqual(set(artifact), assembler.ARTIFACT_KEYS)
        self.assertEqual(artifact["schema"], assembler.SCHEMA)
        self.assertEqual(artifact["artifact_role"], assembler.ARTIFACT_ROLE)
        self.assertEqual(artifact["claims"], assembler.NO_CLAIMS)
        self.assertEqual(artifact["status"], "worker_passed_pending_shutdown")
        self.assertEqual(artifact["vm_lifecycle"], LIFECYCLE)
        self.assertEqual(
            artifact["checks"]["qualification"],
            qualification_result(single_run=False),
        )
        self.assertEqual(
            artifact["checks"]["memcheck_qualification"],
            qualification_result(single_run=True),
        )
        self.assertEqual(
            artifact["commands"]["qualification"],
            assembler.QUALIFICATION_COMMAND,
        )
        self.assertEqual(
            artifact["commands"]["memcheck_application"],
            assembler.SANITIZER_APPLICATION_COMMAND,
        )
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

    def test_rejects_false_success_or_an_expanded_scientific_claim(self) -> None:
        mutations = (
            ("failure", {"success": False}),
            ("fallback", {"exact_fallback_count": 1}),
            ("not support2 exact", {"support2_rank23_catalog_qualified": False}),
            ("Gamma2 claim", {"full_gamma2_claimed": True}),
            ("K2 claim", {"full_k2_hierarchy_claimed": True}),
            ("hierarchy", {"morse_hgp_hierarchy_claimed": True}),
            ("industrial", {"industrial_scale_claimed": True}),
            ("Delaunay", {"ordinary_delaunay_materialized": True}),
            ("global pair matrix", {"global_pair_matrix_materialized": True}),
            ("intermediate D2H", {"intermediate_candidate_d2h": True}),
            ("level persisted", {"exact_level_persisted_in_gpu_soa": True}),
            ("dependent oracle", {"independent_all_pair_all_witness_oracle": False}),
            (
                "wrapper lifetime",
                {"public_terminal_wrapper_and_lease_lifetime_qualified": False},
            ),
        )
        for label, mutation in mutations:
            with self.subTest(label=label):
                value = qualification_result(single_run=False)
                value.update(mutation)
                canonical_json(self.qualification, value)
                self.assert_rejected()
                canonical_json(
                    self.qualification, qualification_result(single_run=False)
                )

    def test_rejects_rank_tile_source_or_count_mutations(self) -> None:
        mutations = (
            ("point count", {"point_count": 256}),
            ("rank", {"maximum_closed_rank": 2}),
            ("orders", {"requested_orders": [1]}),
            ("closed ranks", {"closed_ranks_qualified": [2]}),
            ("capacities", {"anchor_tile_capacities": [17]}),
            ("runs", {"run_count": 1}),
            ("capacity source", {"qualification_capacity_source": "gpu_guess"}),
            ("rank two empty", {"rank_two_record_count": 0}),
            ("rank three empty", {"rank_three_record_count": 0}),
            ("strict payload empty", {"strict_payload_point_id_count": 0}),
            ("shell payload empty", {"shell_payload_point_id_count": 0}),
        )
        for label, mutation in mutations:
            with self.subTest(label=label):
                value = qualification_result(single_run=False)
                value.update(mutation)
                canonical_json(self.qualification, value)
                self.assert_rejected()
                canonical_json(
                    self.qualification, qualification_result(single_run=False)
                )

    def test_rejects_sanitizer_catalog_drift_or_invalid_summaries(self) -> None:
        sanitizer = qualification_result(single_run=True)
        sanitizer["catalog_digest_fnv1a"] += 1
        canonical = json.dumps(sanitizer, sort_keys=True, separators=(",", ":"))
        self.memcheck.write_text(
            canonical + "\n========= ERROR SUMMARY: 0 errors\n", encoding="utf-8"
        )
        self.assert_rejected()

        sanitizer = qualification_result(single_run=True)
        canonical = json.dumps(sanitizer, sort_keys=True, separators=(",", ":"))
        self.memcheck.write_text(
            canonical + "\n========= ERROR SUMMARY: 1 error\n", encoding="utf-8"
        )
        self.assert_rejected()
        self.memcheck.write_text(
            canonical + "\n========= ERROR SUMMARY: 0 errors\n", encoding="utf-8"
        )
        self.racecheck.write_text(
            canonical
            + "\n========= RACECHECK SUMMARY: 1 hazard displayed "
            "(0 errors, 0 warnings)\n",
            encoding="utf-8",
        )
        self.assert_rejected()

    def test_rejects_stderr_build_ptx_or_duplicate_json(self) -> None:
        self.qualification_stderr.write_text("CUDA warning\n", encoding="utf-8")
        self.assert_rejected()
        self.qualification_stderr.write_text("", encoding="utf-8")

        self.release_build.write_text("FAILED: qualification\n", encoding="utf-8")
        self.assert_rejected()
        self.release_build.write_text("ninja: no work to do.\n", encoding="utf-8")

        self.ptx.write_text(".version 8.8\n", encoding="utf-8")
        self.assert_rejected()
        self.ptx.write_text("", encoding="utf-8")

        raw = json.dumps(
            qualification_result(single_run=False), separators=(",", ":")
        )
        self.qualification.write_text(
            raw[:-1] + ',"success":true}\n', encoding="utf-8"
        )
        self.assert_rejected()

    def test_rejects_environment_or_binary_identity_mutation_and_overwrite(self) -> None:
        environment = environment_artifact()
        environment["status"] = "passed"
        canonical_json(self.environment, environment)
        self.assert_rejected()
        canonical_json(self.environment, environment_artifact())

        replacement = self.directory / "replacement"
        replacement.write_bytes(b"replacement")
        self.binary.unlink()
        self.binary.symlink_to(replacement)
        self.assert_rejected()
        self.binary.unlink()
        self.binary.write_bytes(b"phase15-ranked-pair-classifier-qualification")

        self.output.write_text("sentinel\n", encoding="utf-8")
        completed = self.run_assembler()
        self.assertNotEqual(completed.returncode, 0)
        self.assertEqual(self.output.read_text(encoding="utf-8"), "sentinel\n")

    def test_rejects_tampered_embedded_log_on_revalidation(self) -> None:
        completed = self.run_assembler()
        self.assertEqual(completed.returncode, 0, completed.stderr)
        artifact = json.loads(self.output.read_text(encoding="utf-8"))
        artifact["logs"]["qualification"] = artifact["logs"][
            "qualification"
        ].replace('"success":true', '"success":false')
        canonical_json(self.output, artifact)
        with self.assertRaises(SystemExit):
            self.validate_output()


if __name__ == "__main__":
    unittest.main()
