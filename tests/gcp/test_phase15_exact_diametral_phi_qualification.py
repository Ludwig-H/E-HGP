#!/usr/bin/env python3
"""Mutation tests for the Phase 15 exact diametral-Phi G4 assembler."""

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
ASSEMBLER = CUDA_TESTS / "assemble_phase15_exact_diametral_phi_qualification.py"
REMOTE_SCRIPT = ROOT / "gcp-migration" / "phase3_remote_qualification.sh"
ORCHESTRATOR_SCRIPT = ROOT / "gcp-migration" / "run_phase3_qualification.sh"
sys.path.insert(0, str(CUDA_TESTS))

import assemble_phase15_exact_diametral_phi_qualification as assembler

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


def qualification_result() -> dict[str, object]:
    return {
        "adversarial_query_count": 10,
        "all_results_exact": True,
        "backend": assembler.BACKEND,
        "catalog_complete_claimed": False,
        "component_exact_sign_qualification_passed": True,
        "cuda_device": 0,
        "cuda_execution_performed": True,
        "decision_digest_fnv1a": 14_695_981_039_346_656_037,
        "deployment_status": assembler.DEPLOYMENT_STATUS,
        "deterministic_random_query_count": 4_096,
        "deterministic_replay_passed": True,
        "epsilon_used": False,
        "interval_zero_certification_forbidden": True,
        "mode": assembler.MODE,
        "near_shell_query_count": 144,
        "per_run_cpu_multiprecision_count": 1_066,
        "per_run_cuda_kernel_launch_count": 1,
        "per_run_cuda_synchronization_count": 1,
        "per_run_disagreement_count": 0,
        "per_run_exact_zero_count": 384,
        "per_run_gpu_exact_fallback_count": 1_066,
        "per_run_gpu_fixed_limb_dyadic_count": 3_000,
        "per_run_gpu_fp64_interval_count": 3_000,
        "per_run_gpu_known_audited_count": 6_000,
        "per_run_launcher_call_count": 1,
        "per_run_query_count": 7_066,
        "phase": "15",
        "profile": assembler.PROFILE,
        "public_status": assembler.PUBLIC_STATUS,
        "public_status_claimed": False,
        "run_count": 2,
        "scalability_claimed": False,
        "schema": assembler.QUALIFICATION_SCHEMA,
        "scientific_catalog_decision_published": False,
        "symmetry_query_count": 2_816,
        "total_cuda_kernel_launch_count": 2,
        "total_cuda_synchronization_count": 2,
        "total_evaluated_query_count": 14_132,
        "total_launcher_call_count": 2,
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


class Phase15ScriptWiringTests(unittest.TestCase):
    @staticmethod
    def assert_markers_in_order(raw: str, markers: tuple[str, ...]) -> None:
        cursor = 0
        for marker in markers:
            position = raw.find(marker, cursor)
            if position < 0:
                raise AssertionError(f"missing or out-of-order script marker: {marker}")
            cursor = position + len(marker)

    def test_worker_and_orchestrator_keep_guarded_phase15_order(self) -> None:
        remote = REMOTE_SCRIPT.read_text(encoding="utf-8")
        orchestrator = ORCHESTRATOR_SCRIPT.read_text(encoding="utf-8")

        self.assertIn(
            "--phase15-exact-diametral-phi-output est exclusive des autres compagnons.",
            remote,
        )
        self.assert_markers_in_order(
            remote,
            (
                "read_guest_shutdown_guard ||",
                'begin_unit "phase15-exact-diametral-phi-release-build"',
                'begin_unit "phase15-exact-diametral-phi-audit-build"',
                'begin_unit "phase15-exact-diametral-phi-qualification"',
                'begin_unit "phase15-exact-diametral-phi-cuobjdump-elf"',
                'begin_unit "phase15-exact-diametral-phi-cuobjdump-ptx"',
                'begin_unit "phase15-exact-diametral-phi-memcheck"',
                'begin_unit "phase15-exact-diametral-phi-racecheck"',
                'python3 -B "${PHASE15_EXACT_DIAMETRAL_PHI_ASSEMBLER}"',
                'rm -f -- "${PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP}"',
            ),
        )
        self.assertIn(
            'rm -f -- "${PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP}" || true',
            remote,
        )

        self.assertIn(
            "--phase15-exact-diametral-phi est mutuellement exclusive de tous les autres compagnons.",
            orchestrator,
        )
        self.assertIn("cleanup_local_publication()", orchestrator)
        self.assertIn(
            'rm -f -- "${LOCAL_PHASE15_EXACT_DIAMETRAL_PHI_TEMP_RESULT}" || true',
            orchestrator,
        )
        self.assert_markers_in_order(
            orchestrator,
            (
                '"${INSTANCE_NAME}:${remote_phase15_exact_diametral_phi_artifact}"',
                "assembler.validate_artifact_file(",
                "if certify_target_stopped; then",
                "os.link(temporary, target, follow_symlinks=False)",
                'rm -f -- "${LOCAL_PHASE15_EXACT_DIAMETRAL_PHI_TEMP_RESULT}"',
            ),
        )


class Phase15ExactDiametralPhiQualificationAssemblerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.directory = Path(self.temporary_directory.name)
        self.environment = self.directory / "phase3.json"
        self.release_build = self.directory / "release-build.log"
        self.audit_build = self.directory / "audit-build.log"
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
        self.release_build.write_text(
            "[1/2] Building CUDA object exact-diametral-phi\n"
            "[2/2] Linking CXX executable "
            "morsehgp3d_gpu_exact_diametral_phi_qualification\n",
            encoding="utf-8",
        )
        self.audit_build.write_text(
            "[1/2] Building CUDA audit object exact-diametral-phi\n"
            "ptxas info : Compiling entry function for sm_120\n"
            "[2/2] Linking CXX executable "
            "morsehgp3d_gpu_exact_diametral_phi_qualification\n",
            encoding="utf-8",
        )
        self.elf.write_text("Fatbin elf code:\narch = sm_120\n", encoding="utf-8")
        self.ptx.write_text("", encoding="utf-8")
        self.ptx_stderr.write_text(
            "cuobjdump diagnostic: no PTX entries\n", encoding="utf-8"
        )
        qualification_line = (
            json.dumps(qualification_result(), sort_keys=True, separators=(",", ":"))
            + "\n"
        )
        self.memcheck.write_text(
            qualification_line + "========= ERROR SUMMARY: 0 errors\n",
            encoding="utf-8",
        )
        self.racecheck.write_text(
            qualification_line + "========= RACECHECK SUMMARY: 0 hazards displayed "
            "(0 errors, 0 warnings)\n",
            encoding="utf-8",
        )
        self.binary.write_bytes(b"phase15-exact-diametral-phi-qualification")

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

    def test_valid_evidence_is_component_only_pending_shutdown(self) -> None:
        completed = self.run_assembler()
        self.assertEqual(completed.returncode, 0, completed.stderr)
        artifact = self.validate_output()
        self.assertEqual(set(artifact), assembler.ARTIFACT_KEYS)
        self.assertEqual(artifact["schema"], assembler.SCHEMA)
        self.assertEqual(artifact["artifact_role"], assembler.ARTIFACT_ROLE)
        self.assertEqual(artifact["backend"], assembler.BACKEND)
        self.assertEqual(artifact["mode"], assembler.MODE)
        self.assertEqual(artifact["phase_context"], assembler.PHASE_CONTEXT)
        self.assertIs(artifact["component_only"], True)
        self.assertEqual(artifact["claims"], assembler.NO_CLAIMS)
        self.assertIs(artifact["scientific_result_claimed"], False)
        self.assertIsNone(artifact["scientific_public_status"])
        self.assertEqual(artifact["status"], "worker_passed_pending_shutdown")
        self.assertEqual(artifact["vm_lifecycle"], LIFECYCLE)
        self.assertEqual(artifact["checks"]["qualification"], qualification_result())
        self.assertEqual(artifact["checks"]["aot_elf_architectures"], ["sm_120"])
        self.assertEqual(artifact["checks"]["aot_ptx_entry_count"], 0)
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

    def test_rejects_wrong_closed_qualification_shape(self) -> None:
        mutations = [
            ("extra key", {"unexpected": 1}),
            ("wrong schema", {"schema": "foreign"}),
            ("wrong backend", {"backend": "cuda_g4"}),
            ("wrong mode", {"mode": "proposal_only"}),
            ("wrong public status", {"public_status": None}),
            ("wrong query count", {"per_run_query_count": 7_065}),
            ("boolean query count", {"per_run_query_count": True}),
            ("wrong adversarial corpus", {"adversarial_query_count": 9}),
            ("wrong random corpus", {"deterministic_random_query_count": 4_095}),
            ("wrong near-shell corpus", {"near_shell_query_count": 143}),
            ("wrong symmetry corpus", {"symmetry_query_count": 2_815}),
            ("no interval stage", {"per_run_gpu_fp64_interval_count": 0}),
            ("no fixed stage", {"per_run_gpu_fixed_limb_dyadic_count": 0}),
            ("no fallback stage", {"per_run_gpu_exact_fallback_count": 0}),
            ("no CPU fallback", {"per_run_cpu_multiprecision_count": 0}),
            ("no exact zero", {"per_run_exact_zero_count": 0}),
            ("no GPU audit", {"per_run_gpu_known_audited_count": 0}),
            ("stage partition mismatch", {"per_run_gpu_fp64_interval_count": 2_999}),
            ("fallback mismatch", {"per_run_cpu_multiprecision_count": 1_065}),
            ("audit mismatch", {"per_run_gpu_known_audited_count": 5_999}),
            ("too many zeros", {"per_run_exact_zero_count": 7_067}),
            ("disagreement", {"per_run_disagreement_count": 1}),
            ("wrong run count", {"run_count": 1}),
            ("wrong per-run launcher", {"per_run_launcher_call_count": 2}),
            ("wrong per-run kernel", {"per_run_cuda_kernel_launch_count": 2}),
            ("wrong per-run synchronization", {"per_run_cuda_synchronization_count": 2}),
            ("one total launcher", {"total_launcher_call_count": 1}),
            ("one total kernel", {"total_cuda_kernel_launch_count": 1}),
            ("one total synchronization", {"total_cuda_synchronization_count": 1}),
            ("wrong evaluated total", {"total_evaluated_query_count": 7_066}),
            ("negative digest", {"decision_digest_fnv1a": -1}),
            ("oversized digest", {"decision_digest_fnv1a": 1 << 64}),
            ("boolean digest", {"decision_digest_fnv1a": True}),
            ("invalid CUDA device", {"cuda_device": -1}),
            ("no CUDA execution", {"cuda_execution_performed": False}),
            ("not all exact", {"all_results_exact": False}),
            (
                "interval certifies zero",
                {"interval_zero_certification_forbidden": False},
            ),
            ("epsilon", {"epsilon_used": True}),
            ("non-deterministic replay", {"deterministic_replay_passed": False}),
            (
                "component qualification false",
                {"component_exact_sign_qualification_passed": False},
            ),
            (
                "scientific catalog",
                {"scientific_catalog_decision_published": True},
            ),
            ("catalog claim", {"catalog_complete_claimed": True}),
            ("scale claim", {"scalability_claimed": True}),
            ("public claim", {"public_status_claimed": True}),
        ]
        for label, mutation in mutations:
            with self.subTest(label=label):
                value = qualification_result()
                value.update(mutation)
                canonical_json(self.qualification, value)
                self.assert_rejected()
                canonical_json(self.qualification, qualification_result())

    def test_rejects_multiple_objects_and_duplicate_qualification_keys(self) -> None:
        valid = json.dumps(qualification_result(), separators=(",", ":"))
        for label, raw in (
            ("multiple objects", valid + "\n" + valid + "\n"),
            (
                "noncanonical object",
                json.dumps(qualification_result()) + "\n",
            ),
            (
                "duplicate key",
                valid[:-1] + ',"per_run_query_count":7066}\n',
            ),
        ):
            with self.subTest(label=label):
                self.qualification.write_text(raw, encoding="utf-8")
                self.assert_rejected()

    def test_rejects_failed_or_empty_build_evidence(self) -> None:
        failures = (
            "",
            "FAILED: exact-diametral-phi\n",
            "ninja: build stopped: subcommand failed.\n",
            "CMake Error: configuration failed\n",
            "source.cu:9: fatal error: missing header\n",
            "ld.lld: error: undefined symbol\n",
        )
        for path in (self.release_build, self.audit_build):
            original = path.read_text(encoding="utf-8")
            for evidence in failures:
                with self.subTest(path=path.name, evidence=evidence):
                    path.write_text(evidence, encoding="utf-8")
                    self.assert_rejected()
            path.write_text(original, encoding="utf-8")

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

    def test_rejects_missing_duplicate_or_different_sanitizer_replay(self) -> None:
        direct = json.dumps(
            qualification_result(), sort_keys=True, separators=(",", ":")
        )
        different = qualification_result()
        different["cuda_device"] = 1
        different_raw = json.dumps(different, sort_keys=True, separators=(",", ":"))
        for path, summary in (
            (self.memcheck, "========= ERROR SUMMARY: 0 errors\n"),
            (
                self.racecheck,
                "========= RACECHECK SUMMARY: 0 hazards displayed "
                "(0 errors, 0 warnings)\n",
            ),
        ):
            original = path.read_text(encoding="utf-8")
            mutations = (
                ("missing", summary),
                ("noncanonical", json.dumps(qualification_result()) + "\n" + summary),
                ("duplicate", direct + "\n" + direct + "\n" + summary),
                ("different", different_raw + "\n" + summary),
            )
            for label, evidence in mutations:
                with self.subTest(path=path.name, label=label):
                    path.write_text(evidence, encoding="utf-8")
                    self.assert_rejected()
            path.write_text(original, encoding="utf-8")

    def test_rejects_racecheck_hazard_or_failure_marker(self) -> None:
        for evidence in (
            "========= RACECHECK SUMMARY: 1 hazard displayed "
            "(1 error, 0 warnings)\n",
            "========= RACECHECK SUMMARY: 0 hazards displayed "
            "(0 errors, 0 warnings)\n"
            "========= Target application returned an error\n",
            "========= Potential RAW hazard detected\n"
            "========= RACECHECK SUMMARY: 0 hazards displayed "
            "(0 errors, 0 warnings)\n",
        ):
            with self.subTest(evidence=evidence):
                self.racecheck.write_text(evidence, encoding="utf-8")
                self.assert_rejected()

    def test_rejects_environment_identity_workflow_or_lifecycle_mutation(self) -> None:
        mutations = (
            ("dirty Git", ("git", {"clean": False, "sha": GIT_SHA})),
            (
                "audit failed",
                (
                    "checks",
                    {
                        "cuda_audit_workflow": "failed",
                        "cuda_release_workflow": "passed",
                    },
                ),
            ),
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
                canonical_json(self.environment, value)
                self.assert_rejected()
                canonical_json(self.environment, environment_artifact())

    def test_shared_validator_rejects_outer_mutations_even_if_rehashed(self) -> None:
        completed = self.run_assembler()
        self.assertEqual(completed.returncode, 0, completed.stderr)
        authentic = json.loads(self.output.read_text(encoding="utf-8"))

        def mutated(mutator: object) -> None:
            value = copy.deepcopy(authentic)
            mutator(value)
            canonical_json(self.output, value)
            with self.assertRaises(SystemExit):
                self.validate_output()

        cases = (
            ("extra key", lambda value: value.update({"unexpected": 1})),
            (
                "not component only",
                lambda value: value.update({"component_only": False}),
            ),
            (
                "catalog claim",
                lambda value: value["claims"].update({"catalog_complete": True}),
            ),
            (
                "phase backend",
                lambda value: value["phase_context"].update({"backend": "cuda_g4"}),
            ),
            (
                "premature pass",
                lambda value: value.update({"status": "passed"}),
            ),
            (
                "forged provenance",
                lambda value: value["provenance"].update(
                    {"environment_artifact_sha256": "0" * 64}
                ),
            ),
            (
                "forged AOT check",
                lambda value: value["checks"].update(
                    {"aot_elf_architectures": ["sm_90"]}
                ),
            ),
            (
                "mutated log without hash",
                lambda value: value["logs"].update(
                    {"release_build": "different successful build\n"}
                ),
            ),
            (
                "rehashed failed build",
                lambda value: self._replace_log(
                    value, "audit_build", "FAILED: forged audit build\n"
                ),
            ),
            (
                "rehashed qualification disagreement",
                self._forge_qualification_disagreement,
            ),
        )
        for label, mutator in cases:
            with self.subTest(label=label):
                mutated(mutator)
        canonical_json(self.output, authentic)
        self.assertEqual(self.validate_output(), authentic)

    @staticmethod
    def _replace_log(artifact: dict[str, object], name: str, replacement: str) -> None:
        artifact["logs"][name] = replacement
        artifact["log_sha256"][name] = hashlib.sha256(
            replacement.encode("utf-8")
        ).hexdigest()

    @classmethod
    def _forge_qualification_disagreement(cls, artifact: dict[str, object]) -> None:
        qualification = copy.deepcopy(artifact["checks"]["qualification"])
        qualification["per_run_disagreement_count"] = 1
        raw = json.dumps(qualification, sort_keys=True, separators=(",", ":")) + "\n"
        artifact["checks"]["qualification"] = qualification
        cls._replace_log(artifact, "qualification", raw)

    def test_shared_validator_rejects_noncanonical_or_symlinked_artifact(self) -> None:
        completed = self.run_assembler()
        self.assertEqual(completed.returncode, 0, completed.stderr)
        authentic = json.loads(self.output.read_text(encoding="utf-8"))
        self.output.write_text(json.dumps(authentic, indent=2) + "\n", encoding="utf-8")
        with self.assertRaises(SystemExit):
            self.validate_output()

        canonical_json(self.output, authentic)
        real_output = self.directory / "real-artifact.json"
        self.output.rename(real_output)
        self.output.symlink_to(real_output)
        with self.assertRaises(SystemExit):
            self.validate_output()

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
