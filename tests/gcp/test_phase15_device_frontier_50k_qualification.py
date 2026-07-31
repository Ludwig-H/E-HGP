#!/usr/bin/env python3
"""Mutation tests for the guarded Phase 15 non-smoke 50k qualification."""

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
ASSEMBLER = CUDA_TESTS / "assemble_phase15_device_frontier_50k_qualification.py"
REMOTE_SCRIPT = ROOT / "gcp-migration" / "phase3_remote_qualification.sh"
ORCHESTRATOR_SCRIPT = ROOT / "gcp-migration" / "run_phase3_qualification.sh"
sys.path.insert(0, str(CUDA_TESTS))

import assemble_phase15_device_frontier_50k_qualification as assembler

GIT_SHA = "a" * 40
IMAGE_ID = "sha256:" + "b" * 64
IMAGE_REF = f"morsehgp3d-phase3:{GIT_SHA}"
BASE_IMAGE_REF = assembler.BASE_IMAGE_REF
LIFECYCLE = dict(assembler.WORKER_LIFECYCLE)


def write_json(path: Path, value: object, *, sort_keys: bool = True) -> None:
    path.write_text(
        json.dumps(
            value,
            ensure_ascii=False,
            sort_keys=sort_keys,
            separators=(",", ":"),
        )
        + "\n",
        encoding="utf-8",
    )


def rank_result(
    maximum_closed_rank: int = assembler.MAXIMUM_CLOSED_RANK,
) -> dict[str, object]:
    candidate_mass = 750_000
    arena_allocation_count = (
        2 if maximum_closed_rank == assembler.MAXIMUM_CLOSED_RANK else 1
    )
    return {
        "bank_entry_count": 480_000,
        "bounded_admitted_pair_count": 0,
        "bounded_bruteforce_ns": 0,
        "bounded_bruteforce_performed": False,
        "bounded_candidate_admitted_pair_count": 0,
        "bounded_candidate_rejected_pair_count": 0,
        "bounded_exact_pair_count": 0,
        "bounded_non_support_shell_equality_count": 0,
        "build_ns": 10_000,
        "candidate_pair_mass": candidate_mass,
        "candidate_record_count": candidate_mass,
        "candidate_yield_anchor_count": 813,
        "censored_anchor_count": 0,
        "certified_pruned_pair_mass": (
            assembler.UNORDERED_PAIR_UNIVERSE - candidate_mass
        ),
        "complete_anchor_count": assembler.POINT_COUNT,
        "component_contract_validated": True,
        "coverage_partition_complete": True,
        "cpu_recertification_ns": 20_000,
        "device_total_bytes": 48_000_000_000,
        "every_prune_fully_recertified": True,
        "fully_recertified_prune_count": 900_000,
        "fresh_tile_device_arena_allocation_count": arena_allocation_count,
        "fresh_tile_device_arena_reuse_count": 13 - arena_allocation_count,
        "gamma2_prune_or_discard_authorized": False,
        "gamma2_silent_handoff_required": False,
        "launcher_ns": 30_000,
        "lease_release_ns": 4_000,
        "maximum_closed_rank": maximum_closed_rank,
        "minimum_device_free_bytes": 40_000_000_000,
        "node_copy_ns": 5_000,
        "output_digest_fnv1a": 14_695_981_039_346_656_037,
        "peak_tile_output_device_capacity_bytes": 256_000_000,
        "physical_node_visit_count": 2_000_000,
        "chunk_count": 27,
        "prune_region_count": 900_000,
        "prune_semantics": "closed_rank_window",
        "prune_witness_target_check_count": 8_000_000,
        "prune_yield_anchor_count": 0,
        "q3_exact_diametral_pair_support_gabriel_negative_only": False,
        "qualification_device_to_host_bytes": 32_000_000,
        "qualification_output_copy_ns": 6_000,
        "required_witness_count": maximum_closed_rank - 1,
        "resumed_chunk_count": 14,
        "sampled_recertified_prune_count": 900_000,
        "tile_count": 13,
        "traversal_adoption_ns": 7_000,
        "traversal_device_capacity_bytes": 64_000_000,
        "unbanked_candidate_count": 12,
        "unordered_pair_universe_count": assembler.UNORDERED_PAIR_UNIVERSE,
        "unresolved_pair_mass": 0,
    }


def qualification_result(
    maximum_closed_rank: int = assembler.MAXIMUM_CLOSED_RANK,
) -> dict[str, object]:
    # Keep the producer's actual compact key order. It is deterministic but the
    # two rank-threshold fields are intentionally emitted after rank_results.
    return {
        "anchor_tile_capacity": assembler.ANCHOR_TILE_CAPACITY,
        "backend": assembler.BACKEND,
        "canonicalization_ns": 10_000,
        "cloud_digest_fnv1a": 1_234_567_890,
        "component_only": True,
        "coverage_partition_complete": True,
        "delaunay_materialized": False,
        "dense_pair_fallback_performed": False,
        "deployment_status": assembler.DEPLOYMENT_STATUS,
        "equality_shell_point_count": 55,
        "exact_prune_target_check_limit": (
            assembler.EXACT_PRUNE_TARGET_CHECK_LIMIT
        ),
        "family": assembler.FAMILY,
        "generation_ns": 20_000,
        "git_sha": GIT_SHA,
        "global_pair_matrix_materialized": False,
        "guard_size_at_most_50000": True,
        "guard_size_passed": True,
        "higher_order_structure_materialized": False,
        "jitter_grid_point_count": 125,
        "mode": assembler.MODE,
        "ordinary_delaunay_materialized": False,
        "ordinary_emst_computed": False,
        "peak_host_rss_bytes": 128_000_000,
        "permutation_sign_point_count": 48,
        "point_count": assembler.POINT_COUNT,
        "profile": assembler.PROFILE,
        "profile_exit_success": True,
        "public_status": assembler.PUBLIC_STATUS,
        "rank_results": [rank_result(maximum_closed_rank)],
        "all_rank_thresholds_exercised": False,
        "required_rank_triplet_exercised": False,
        "sampled_prune_limit": assembler.SAMPLED_PRUNE_LIMIT,
        "schema": assembler.QUALIFICATION_SCHEMA,
        "scientific_pair_catalog_published": False,
        "seed": assembler.SEED,
        "success": True,
        "tight_cluster_point_count": 128,
        "total_ns": 100_000,
    }


def timing_result(
    maximum_closed_rank: int = assembler.MAXIMUM_CLOSED_RANK,
) -> dict[str, object]:
    return {
        "command": assembler.qualification_command(maximum_closed_rank),
        "elapsed_wall_ns": 2_000_000_000,
        "exit_status": 0,
        "finished_epoch_ns": 1_800_000_002_000_000_000,
        "schema": assembler.TIMING_SCHEMA,
        "started_epoch_ns": 1_800_000_000_000_000_000,
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


class Phase15DeviceFrontier50kScriptWiringTests(unittest.TestCase):
    @staticmethod
    def assert_markers_in_order(raw: str, markers: tuple[str, ...]) -> None:
        cursor = 0
        for marker in markers:
            position = raw.find(marker, cursor)
            if position < 0:
                raise AssertionError(f"missing or out-of-order script marker: {marker}")
            cursor = position + len(marker)

    def test_worker_runs_only_the_guarded_standard_50k_command(self) -> None:
        remote = REMOTE_SCRIPT.read_text(encoding="utf-8")
        start = remote.index('begin_unit "phase15-device-frontier-50k-build"')
        finish = remote.index('if [[ -n "${PHASE4_OUTPUT_PATH}" ]]; then', start)
        execution = remote[start:finish]

        self.assertIn(
            "--phase15-device-frontier-50k-output est exclusive des autres compagnons.",
            remote,
        )
        self.assert_markers_in_order(
            remote,
            (
                "read_guest_shutdown_guard ||",
                'begin_unit "phase15-device-frontier-50k-build"',
                'begin_unit "phase15-device-frontier-50k-qualification"',
                "assembler.validate_qualification(",
                'python3 -B "${PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER}"',
                'rm -f -- "${PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP}"',
            ),
        )
        for marker in (
            '"${PHASE15_DEVICE_FRONTIER_50K_BINARY_PATH}"',
            "--point-count 50000",
            "--family adversarial_mixed_dyadic",
            '"${PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK}"',
            "--anchor-tile-capacity 4096",
            '--seed "${PHASE15_DEVICE_FRONTIER_50K_SEED}"',
        ):
            self.assertIn(marker, execution)
        self.assertIn(
            "PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK=11",
            remote,
        )
        self.assertNotIn("--scaling-smoke", execution)
        self.assertNotIn("--direct-scale", execution)

    def test_kmax5_route_is_explicit_and_preserves_rank11_default(self) -> None:
        remote = REMOTE_SCRIPT.read_text(encoding="utf-8")
        orchestrator = ORCHESTRATOR_SCRIPT.read_text(encoding="utf-8")

        for marker in (
            "--phase15-device-frontier-50k-kmax5",
            "PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK=6",
            'phase15_device_frontier_50k_result_prefix="phase15-device-frontier-50k-kmax5"',
            'phase15_device_frontier_50k_worker_option+=" --phase15-device-frontier-50k-maximum-closed-rank 6"',
        ):
            self.assertIn(marker, orchestrator)
        self.assertIn(
            "PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK=11",
            orchestrator,
        )
        for marker in (
            "--phase15-device-frontier-50k-maximum-closed-rank",
            "maximum_closed_rank = sys.argv[7]",
            "maximum_closed_rank = int(sys.argv[6])",
            "maximum_closed_rank=maximum_closed_rank",
            '"${PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK}"',
        ):
            self.assertIn(marker, remote)

    def test_worker_rejects_invalid_or_unscoped_rank_selection(self) -> None:
        common = [
            str(REMOTE_SCRIPT),
            "--yes",
            "--gce-deadline-epoch",
            "1800000000",
            "--output",
            "/tmp/phase3-kmax5-static-test.json",
        ]
        invalid = subprocess.run(
            [
                *common,
                "--phase15-device-frontier-50k-output",
                "/tmp/phase15-kmax5-static-test.json",
                "--phase15-device-frontier-50k-maximum-closed-rank",
                "5",
            ],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertNotEqual(invalid.returncode, 0)
        self.assertIn("doit valoir exactement 6 ou 11", invalid.stderr)

        unscoped = subprocess.run(
            [
                *common,
                "--phase15-device-frontier-50k-maximum-closed-rank",
                "6",
            ],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertNotEqual(unscoped.returncode, 0)
        self.assertIn(
            "exige --phase15-device-frontier-50k-output",
            unscoped.stderr,
        )

    def test_split_output_keeps_wrapper_diagnostics_out_of_binary_stderr(
        self,
    ) -> None:
        remote = REMOTE_SCRIPT.read_text(encoding="utf-8")
        start = remote.index("run_container_split_output() {")
        finish = remote.index('\nbegin_unit "cuda-release"', start)
        implementation = remote[start:finish]

        self.assertIn(
            "run_until_work_deadline_split_output \\\n"
            '        "${label}" "${stdout_path}" "${stderr_path}"',
            implementation,
        )
        self.assertIn('ACTIVE_CONTAINER_LOG="${DOCKER_LOG}"', implementation)
        self.assertIn(
            '"${cidfile}" "${container_name}" "${DOCKER_LOG}"',
            implementation,
        )
        self.assertNotIn('2>>"${stderr_path}"', implementation)
        self.assertNotIn(
            '"${IMAGE_REF}" "$@" >"${stdout_path}" 2>"${stderr_path}"',
            implementation,
        )

    def test_orchestrator_validates_before_stop_and_publishes_after_stop(self) -> None:
        orchestrator = ORCHESTRATOR_SCRIPT.read_text(encoding="utf-8")
        self.assertIn(
            "--phase15-device-frontier-50k est mutuellement exclusive de tous les autres compagnons.",
            orchestrator,
        )
        self.assertIn(
            'rm -f -- "${LOCAL_PHASE15_DEVICE_FRONTIER_50K_TEMP_RESULT}" || true',
            orchestrator,
        )
        self.assert_markers_in_order(
            orchestrator,
            (
                '"${INSTANCE_NAME}:${remote_phase15_device_frontier_50k_artifact}"',
                "import assemble_phase15_device_frontier_50k_qualification as assembler",
                "if certify_target_stopped; then",
                '"${LOCAL_PHASE15_DEVICE_FRONTIER_50K_TEMP_RESULT}" "${LOCAL_PHASE15_DEVICE_FRONTIER_50K_RESULT}"',
                "os.link(temporary, target, follow_symlinks=False)",
                'rm -f -- "${LOCAL_PHASE15_DEVICE_FRONTIER_50K_TEMP_RESULT}"',
            ),
        )


class Phase15DeviceFrontier50kAssemblerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.directory = Path(self.temporary_directory.name)
        self.environment = self.directory / "phase3.json"
        self.build = self.directory / "build.log"
        self.qualification = self.directory / "qualification.json"
        self.stderr = self.directory / "qualification.stderr.log"
        self.timing = self.directory / "timing.json"
        self.binary = self.directory / "qualification-binary"
        self.output = self.directory / "artifact.json"

        write_json(self.environment, environment_artifact())
        write_json(self.qualification, qualification_result(), sort_keys=False)
        write_json(self.timing, timing_result())
        self.build.write_text("ninja: no work to do.\n", encoding="utf-8")
        self.stderr.write_text("", encoding="utf-8")
        self.binary.write_bytes(b"phase15-device-frontier-50k-qualification")

    def run_assembler(
        self,
        *,
        output: Path | None = None,
        maximum_closed_rank: int = assembler.MAXIMUM_CLOSED_RANK,
    ) -> subprocess.CompletedProcess[str]:
        command = [
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
            "--build-log",
            str(self.build),
            "--qualification-log",
            str(self.qualification),
            "--qualification-stderr-log",
            str(self.stderr),
            "--timing-log",
            str(self.timing),
            "--binary",
            str(self.binary),
        ]
        if maximum_closed_rank != assembler.MAXIMUM_CLOSED_RANK:
            command.extend(
                ["--maximum-closed-rank", str(maximum_closed_rank)]
            )
        command.extend(["--output", str(output or self.output)])
        return subprocess.run(
            command,
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def assert_rejected(
        self,
        *,
        maximum_closed_rank: int = assembler.MAXIMUM_CLOSED_RANK,
    ) -> None:
        completed = self.run_assembler(
            maximum_closed_rank=maximum_closed_rank
        )
        self.assertNotEqual(completed.returncode, 0, completed.stdout)
        self.assertFalse(self.output.exists())

    def validate_output(
        self,
        *,
        maximum_closed_rank: int = assembler.MAXIMUM_CLOSED_RANK,
    ) -> dict[str, object]:
        return assembler.validate_artifact_file(
            self.output,
            git_sha=GIT_SHA,
            environment_artifact_path=self.environment,
            maximum_closed_rank=maximum_closed_rank,
        )

    def test_valid_evidence_is_complete_component_only_pending_shutdown(self) -> None:
        completed = self.run_assembler()
        self.assertEqual(completed.returncode, 0, completed.stderr)
        artifact = self.validate_output()
        self.assertEqual(set(artifact), assembler.ARTIFACT_KEYS)
        self.assertEqual(artifact["schema"], assembler.SCHEMA)
        self.assertEqual(artifact["backend"], assembler.BACKEND)
        self.assertEqual(artifact["mode"], assembler.MODE)
        self.assertIs(artifact["component_only"], True)
        self.assertEqual(artifact["claims"], assembler.NO_CLAIMS)
        self.assertIs(artifact["scientific_result_claimed"], False)
        self.assertIsNone(artifact["scientific_public_status"])
        self.assertEqual(artifact["command"], assembler.QUALIFICATION_COMMAND)
        self.assertNotIn("--scaling-smoke", artifact["command"])
        self.assertNotIn("--direct-scale", artifact["command"])
        self.assertEqual(artifact["status"], "worker_passed_pending_shutdown")
        self.assertEqual(artifact["vm_lifecycle"], LIFECYCLE)
        self.assertEqual(artifact["checks"]["qualification"], qualification_result())
        self.assertEqual(artifact["timing"], timing_result())
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

    def test_valid_kmax5_evidence_remains_component_only_without_slo(self) -> None:
        maximum_closed_rank = assembler.KMAX5_MAXIMUM_CLOSED_RANK
        write_json(
            self.qualification,
            qualification_result(maximum_closed_rank),
            sort_keys=False,
        )
        write_json(self.timing, timing_result(maximum_closed_rank))

        completed = self.run_assembler(
            maximum_closed_rank=maximum_closed_rank
        )
        self.assertEqual(completed.returncode, 0, completed.stderr)
        artifact = self.validate_output(
            maximum_closed_rank=maximum_closed_rank
        )
        rank = artifact["checks"]["qualification"]["rank_results"][0]
        self.assertEqual(rank["maximum_closed_rank"], 6)
        self.assertEqual(rank["required_witness_count"], 5)
        self.assertEqual(
            artifact["command"],
            assembler.qualification_command(maximum_closed_rank),
        )
        self.assertEqual(
            artifact["timing"], timing_result(maximum_closed_rank)
        )
        self.assertEqual(
            artifact["scientific_scope"],
            "complete_50k_rank6_device_frontier_component_coverage_only",
        )
        self.assertIs(artifact["component_only"], True)
        self.assertEqual(artifact["claims"], assembler.NO_CLAIMS)
        self.assertFalse(artifact["claims"]["exact_hierarchy_claimed"])
        self.assertFalse(artifact["claims"]["slo_claimed"])

    def test_kmax5_rejects_rank11_qualification_or_timing(self) -> None:
        maximum_closed_rank = assembler.KMAX5_MAXIMUM_CLOSED_RANK
        write_json(
            self.qualification,
            qualification_result(maximum_closed_rank),
            sort_keys=False,
        )
        write_json(self.timing, timing_result())
        self.assert_rejected(maximum_closed_rank=maximum_closed_rank)

        write_json(
            self.qualification, qualification_result(), sort_keys=False
        )
        write_json(self.timing, timing_result(maximum_closed_rank))
        self.assert_rejected(maximum_closed_rank=maximum_closed_rank)

    def test_rejects_top_level_failure_claim_or_parameter_mutation(self) -> None:
        mutations = (
            ("coverage", {"coverage_partition_complete": False}),
            ("guard", {"guard_size_passed": False}),
            ("success", {"success": False}),
            ("profile exit", {"profile_exit_success": False}),
            ("point count", {"point_count": 49_999}),
            ("rank triplet", {"required_rank_triplet_exercised": True}),
            ("all ranks", {"all_rank_thresholds_exercised": True}),
            ("family", {"family": "affine_uniform_binary64"}),
            ("seed", {"seed": assembler.SEED + 1}),
            ("tile", {"anchor_tile_capacity": 2_048}),
            ("Delaunay", {"delaunay_materialized": True}),
            ("EMST", {"ordinary_emst_computed": True}),
            ("dense fallback", {"dense_pair_fallback_performed": True}),
            ("catalog", {"scientific_pair_catalog_published": True}),
        )
        for label, mutation in mutations:
            with self.subTest(label=label):
                value = qualification_result()
                value.update(mutation)
                write_json(self.qualification, value, sort_keys=False)
                self.assert_rejected()
                write_json(
                    self.qualification, qualification_result(), sort_keys=False
                )

    def test_rejects_rank_censure_unresolved_mass_or_incomplete_contract(self) -> None:
        mutations = (
            ("censored", {"censored_anchor_count": 1}),
            ("unresolved", {"unresolved_pair_mass": 1}),
            ("coverage", {"coverage_partition_complete": False}),
            ("contract", {"component_contract_validated": False}),
            ("rank", {"maximum_closed_rank": 10}),
            ("tiles", {"tile_count": 12}),
            ("universe", {"unordered_pair_universe_count": 1}),
            ("candidate records", {"candidate_record_count": 749_999}),
            ("pruned mass", {"certified_pruned_pair_mass": 1}),
            ("bounded fallback", {"bounded_bruteforce_performed": True}),
            (
                "Gamma2 authorization",
                {"gamma2_prune_or_discard_authorized": True},
            ),
            (
                "silent handoff",
                {"gamma2_silent_handoff_required": True},
            ),
            ("semantics", {"prune_semantics": "strict_interior_threshold"}),
            ("witness count", {"required_witness_count": 2}),
            (
                "Q3 negative-only scope",
                {"q3_exact_diametral_pair_support_gabriel_negative_only": True},
            ),
            ("chunk closure", {"chunk_count": 26}),
            (
                "yield closure",
                {
                    "candidate_yield_anchor_count": 0,
                    "prune_yield_anchor_count": 0,
                },
            ),
        )
        for label, mutation in mutations:
            with self.subTest(label=label):
                value = qualification_result()
                value["rank_results"][0].update(mutation)
                write_json(self.qualification, value, sort_keys=False)
                self.assert_rejected()
                write_json(
                    self.qualification, qualification_result(), sort_keys=False
                )

    def test_rejects_nonempty_stderr_failed_build_and_malformed_json(self) -> None:
        self.stderr.write_text("CUDA warning\n", encoding="utf-8")
        self.assert_rejected()
        self.stderr.write_text("", encoding="utf-8")

        self.build.write_text("FAILED: qualification\n", encoding="utf-8")
        self.assert_rejected()
        self.build.write_text("ninja: no work to do.\n", encoding="utf-8")

        value = qualification_result()
        self.qualification.write_text(json.dumps(value) + "\n", encoding="utf-8")
        self.assert_rejected()
        direct = json.dumps(value, separators=(",", ":"))
        self.qualification.write_text(
            direct[:-1] + ',"success":true}\n', encoding="utf-8"
        )
        self.assert_rejected()

    def test_rejects_timing_command_status_or_interval_mutation(self) -> None:
        mutations = (
            ("command", {"command": ["forged"]}),
            ("status", {"exit_status": 1}),
            ("schema", {"schema": "foreign"}),
            ("elapsed", {"elapsed_wall_ns": 1}),
            (
                "reversed",
                {"finished_epoch_ns": 1_799_999_999_000_000_000},
            ),
        )
        for label, mutation in mutations:
            with self.subTest(label=label):
                value = timing_result()
                value.update(mutation)
                write_json(self.timing, value)
                self.assert_rejected()
                write_json(self.timing, timing_result())

    def test_rejects_environment_workflow_or_guard_mutation(self) -> None:
        mutations = (
            ("dirty", ("git", {"clean": False, "sha": GIT_SHA})),
            (
                "release failed",
                (
                    "checks",
                    {
                        "cuda_audit_workflow": "passed",
                        "cuda_release_workflow": "failed",
                    },
                ),
            ),
            (
                "guard absent",
                (
                    "vm_lifecycle",
                    {**LIFECYCLE, "guest_shutdown_guard_verified": False},
                ),
            ),
        )
        for label, (field, replacement) in mutations:
            with self.subTest(label=label):
                value = environment_artifact()
                value[field] = replacement
                write_json(self.environment, value)
                self.assert_rejected()
                write_json(self.environment, environment_artifact())

    def test_shared_validator_rejects_rehashed_claim_and_coverage_forgery(self) -> None:
        completed = self.run_assembler()
        self.assertEqual(completed.returncode, 0, completed.stderr)
        authentic = json.loads(self.output.read_text(encoding="utf-8"))

        claimed = copy.deepcopy(authentic)
        claimed["claims"]["product_claimed"] = True
        write_json(self.output, claimed)
        with self.assertRaises(SystemExit):
            self.validate_output()

        forged = copy.deepcopy(authentic)
        qualification = copy.deepcopy(forged["checks"]["qualification"])
        qualification["coverage_partition_complete"] = False
        raw = json.dumps(qualification, separators=(",", ":")) + "\n"
        forged["checks"]["qualification"] = qualification
        forged["logs"]["qualification"] = raw
        forged["log_sha256"]["qualification"] = hashlib.sha256(
            raw.encode("utf-8")
        ).hexdigest()
        write_json(self.output, forged)
        with self.assertRaises(SystemExit):
            self.validate_output()

        write_json(self.output, authentic)
        self.assertEqual(self.validate_output(), authentic)

    def test_rejects_symlinked_binary_and_existing_output(self) -> None:
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


if __name__ == "__main__":
    unittest.main()
