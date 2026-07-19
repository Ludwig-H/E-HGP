#!/usr/bin/env python3
"""Validate the closed Phase 5 K1 Boruvka GPU qualification contract."""

from __future__ import annotations

import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys
import tempfile
from types import ModuleType
from typing import Any, Callable

QUALIFICATION_SCHEMA = "morsehgp3d.phase5.k1_boruvka_gpu_qualification.v4"
REPLAY_SCHEMA = "morsehgp3d.phase5.k1_boruvka_full_loop_gpu_replay.v3"
SCIENTIFIC_SCOPE = (
    "gpu_proposed_bounded_morton_seed_bounded_candidate_emission_"
    "cpu_exact_full_boruvka_"
    "local_emst_witness_only"
)
STATIC_SCHEMA = "morsehgp3d.phase5.k1_boruvka_gpu_qualification_static.v4"
SHA256_RE = re.compile(r"[0-9a-f]{64}\Z")
EXPECTED_TOP_LEVEL_KEYS = {
    "backend",
    "binary",
    "checks",
    "environment",
    "git",
    "image",
    "logs",
    "mode",
    "phase",
    "profile",
    "provenance",
    "schema",
    "scientific_public_status",
    "scientific_result_claimed",
    "scientific_scope",
    "status",
    "vm_lifecycle",
}
EXPECTED_CHECK_KEYS = {
    "aot_elf_architectures",
    "aot_ptx_entry_count",
    "bounded_candidate_emission_chain_certified",
    "bounded_morton_seed_chain_certified",
    "bounded_morton_window_certified",
    "candidate_payload_physical_bound_certified",
    "canonical_contractions_certified",
    "complete_source_partition_certified",
    "complete_source_seed_coverage_certified",
    "cpu_exact_monotone_seed_cutoff_certified",
    "cpu_exact_decision_chain_certified",
    "external_seed_targets_recertified",
    "gpu_multi_round_proposal_chain_certified",
    "independent_chunked_gpu_replay_certified",
    "independent_gpu_replay_certified",
    "independent_morton_seed_gpu_replay_certified",
    "local_emst_witness_certified",
    "memcheck",
    "racecheck",
    "replay",
}
EXPECTED_LOG_KEYS = {
    "cuobjdump_elf",
    "cuobjdump_ptx",
    "cuobjdump_ptx_stderr",
    "full_replay",
    "memcheck",
    "racecheck",
}
WORKER_LIFECYCLE = {
    "guest_shutdown_guard_verified": True,
    "stop_responsibility": "external_orchestrator",
    "worker_mutates_gcp": False,
}


class ContractError(ValueError):
    """A Phase 5 qualification invariant was violated."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        raise ContractError(f"cannot read {path}: {error}") from error


def require_tokens(text: str, tokens: tuple[str, ...], label: str) -> None:
    for token in tokens:
        require(token in text, f"{label} is missing {token!r}")


def strict_json(path: Path) -> tuple[str, dict[str, Any]]:
    def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise ContractError(f"duplicate JSON key in {path}: {key}")
            result[key] = value
        return result

    try:
        raw = path.read_text(encoding="utf-8")
        value = json.loads(raw, object_pairs_hook=reject_duplicate_keys)
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise ContractError(f"cannot parse {path}: {error}") from error
    require(isinstance(value, dict), f"{path} must contain one JSON object")
    return raw, value


def load_assembler(path: Path) -> ModuleType:
    spec = importlib.util.spec_from_file_location(
        "morsehgp3d_phase5_k1_boruvka_assembler", path
    )
    require(spec is not None and spec.loader is not None, "assembler import failed")
    module = importlib.util.module_from_spec(spec)
    try:
        spec.loader.exec_module(module)
    except (ImportError, OSError, SyntaxError) as error:
        raise ContractError(
            f"cannot import qualification assembler: {error}"
        ) from error
    return module


def require_rejected(function: Callable[[], object], label: str) -> None:
    try:
        function()
    except SystemExit as error:
        require(error.code not in (None, 0), f"{label} exited successfully")
        return
    raise ContractError(f"negative mutation was accepted: {label}")


def write_json(path: Path, value: dict[str, Any]) -> None:
    path.write_text(
        json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":"))
        + "\n",
        encoding="ascii",
    )


def validate_static_source(assembler: str, replay: str) -> None:
    require_tokens(
        assembler,
        (
            QUALIFICATION_SCHEMA,
            REPLAY_SCHEMA,
            "gpu_proposed_bounded_morton_seed_bounded_candidate_emission_",
            "cpu_exact_full_boruvka_",
            "local_emst_witness_only",
            "gpu_bounded_morton_seed_cpu_exact_monotone_cutoff_v1",
            '"case_count": 4',
            '"component_count_path": [8, 4, 2, 1]',
            '"component_minimum_count": 14',
            '"gpu_candidate_count": 86',
            '"gpu_kernel_launch_count": 19',
            '"gpu_count_pass_node_visit_count": 236',
            '"candidate_record_budget": 7',
            '"candidate_payload_peak_bytes": 224',
            '"source_chunk_count": 16',
            '"proposal_digest_fnv1a": "88d6d04ed0c2e06b"',
            '"proposal_digest_fnv1a": "8ba818d0f95004dd"',
            '"proposal_digest_fnv1a": "4ca55683c3c49441"',
            '"hgp_weight": ("8127", "4")',
            '"squared_weight": ("8127", "1")',
            'case["fixture"] = "chain_three_rounds_morton_seed"',
            '"logical_candidate_count": 41',
            '"source_chunk_count": 9',
            '"window_radius": 1',
            '"inspected_neighbor_count": 42',
            '"external_neighbor_count": 22',
            '"floating_proposal_count": 16',
            '"exact_selected_proposal_count": 11',
            '"exact_strict_improvement_count": 11',
            '"exact_fallback_count": 13',
            '"exact_seed_distance_evaluation_count": 36',
            'require_exact_keys(value, REPLAY_KEYS, label)',
            'contains_json_key(value, "public_status")',
            "exact_json_equal(value, EXPECTED_REPLAY)",
            "validate_chunked_emission_contract(case, case_label)",
            'set(architectures) != {"sm_120"}',
            "if ptx_log.strip()",
            "validate_memcheck_log(memcheck_log)",
            "validate_racecheck_log(racecheck_log)",
            '"status": "worker_passed_pending_shutdown"',
            '"stop_responsibility": "external_orchestrator"',
            '"worker_mutates_gcp": False',
            '"environment_artifact_sha256"',
            '"scientific_result_claimed": False',
            '"scientific_public_status": None',
            '"bounded_candidate_emission_chain_certified": True',
            '"candidate_payload_physical_bound_certified": True',
            '"complete_source_partition_certified": True',
            '"independent_chunked_gpu_replay_certified": True',
            '"bounded_morton_seed_chain_certified": True',
            '"bounded_morton_window_certified": True',
            '"complete_source_seed_coverage_certified": True',
            '"cpu_exact_monotone_seed_cutoff_certified": True',
            '"external_seed_targets_recertified": True',
            '"independent_morton_seed_gpu_replay_certified": True',
        ),
        "Phase 5 qualification assembler",
    )
    for forbidden in (
        "subprocess",
        "os.system",
        "gcloud ",
        "compute.instances",
        "googleapiclient",
    ):
        require(
            forbidden not in assembler,
            f"qualification assembler may mutate external state via {forbidden!r}",
        )
    require_tokens(
        replay,
        (
            '"singleton_terminal"',
            '"chain_three_rounds"',
            '"square_equal_length_ties"',
            '"chain_three_rounds_morton_seed"',
            "std::array<std::size_t, 4>{8U, 4U, 2U, 1U}",
            REPLAY_SCHEMA,
            "K1BoruvkaChunkingPolicy",
            "K1BoruvkaMortonSeedPolicy",
            "K1BoruvkaMortonSeedPolicy{1U}",
            "cases.reserve(4U)",
            "bounded_emission_proof_basis",
            "monotone_seed_proof_basis",
            '"bounded_complete_source_ranges"',
            "complete_contiguous_unsplit",
            "chunked_emission_counters",
            "chunked_emission_audit",
            "build_gpu_proposed_cpu_exact_k1_boruvka",
            "verify_gpu_proposed_cpu_exact_k1_boruvka",
            "trusted_morton_seed_policy.has_value()",
            "*trusted_morton_seed_policy",
            "close_morton_seed_comparison",
            "comparison.refined_logical_candidate_count == 41U",
            "comparison.refined_source_chunk_count == 9U",
            "seeds.inspected_neighbor_count == 42U",
            "seeds.external_neighbor_count == 22U",
            "seeds.floating_proposal_count == 16U",
            "seeds.exact_selected_proposal_count == 11U",
            "seeds.exact_strict_improvement_count == 11U",
            "seeds.exact_fallback_count == 13U",
            "seeds.exact_seed_distance_evaluation_count == 36U",
            "K1BoruvkaCandidateAudit::decision_semantics",
            "K1BoruvkaCandidateAudit::proposal_semantics",
            r"\"status\":\"passed\"",
            "K1HybridScientificStatus::local_emst_witness_only",
            "K1HybridHierarchyReductionStatus::not_performed",
            "result.proposal_chain_certified",
            "result.bounded_candidate_emission_chain_certified",
            "result.bounded_morton_seed_chain_certified",
            "verification.proposal_chain_certified",
            "verification.emission_mode_certified",
            "verification.seed_mode_certified",
            "verification.bounded_candidate_emission_chain_certified",
            "verification.bounded_morton_seed_chain_certified",
            "verification.hierarchy_status_separation_certified",
        ),
        "real Phase 5 full-loop K1 Boruvka replay",
    )
    require(
        re.search(
            r"build_gpu_proposed_cpu_exact_k1_boruvka\(\s*index,\s*cloud,"
            r"\s*trusted_chunking_policy,\s*\*trusted_morton_seed_policy\s*\)",
            replay,
            re.DOTALL,
        )
        is not None,
        "real Phase 5 replay does not call the two-policy producer overload",
    )
    require(
        re.search(
            r"verify_gpu_proposed_cpu_exact_k1_boruvka\(\s*index,\s*cloud,"
            r"\s*trusted_chunking_policy,\s*\*trusted_morton_seed_policy,"
            r"\s*producer\s*\)",
            replay,
            re.DOTALL,
        )
        is not None,
        "real Phase 5 replay does not call the two-policy verifier overload",
    )


def make_environment(module: ModuleType, git_sha: str, image_id: str) -> dict[str, Any]:
    return {
        "backend": "cuda_g4",
        "git": {"clean": True, "sha": git_sha},
        "image": {
            "base_ref": module.BASE_IMAGE_REF,
            "dockerfile": "containers/cuda12.9-sm120.Dockerfile",
            "id": image_id,
            "ref": f"morsehgp3d-phase3:{git_sha}",
        },
        "mode": "certified",
        "phase": "3",
        "profile": "hgp_reduced",
        "runtime_records": [
            {
                "manifest": {
                    "compiled_sm": "sm_120",
                    "cuda_driver_version_string": "580.95.05",
                    "gpu_compute_capability_major": 12,
                    "gpu_compute_capability_minor": 0,
                    "gpu_name": "NVIDIA RTX PRO 6000 Blackwell Server Edition",
                    "gpu_runtime_sm": "sm_120",
                    "gpu_uuid": "GPU-01234567-89ab-cdef-0123-456789abcdef",
                    "gpu_vram_bytes": 103079215104,
                }
            }
        ],
        "schema": module.PHASE3_SCHEMA,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": "environment_reproducibility_only",
        "status": "worker_passed_pending_shutdown",
        "vm_lifecycle": copy.deepcopy(module.WORKER_LIFECYCLE),
    }


def exercise_assembler(module: ModuleType) -> None:
    git_sha = "1" * 40
    image_id = "sha256:" + "2" * 64
    with tempfile.TemporaryDirectory(
        prefix="morsehgp3d-phase5-qualification-static."
    ) as directory_name:
        directory = Path(directory_name)
        environment_path = directory / "phase3.json"
        replay_log_path = directory / "replay.json"
        elf_path = directory / "elf.log"
        ptx_path = directory / "ptx.log"
        ptx_stderr_path = directory / "ptx.stderr.log"
        memcheck_path = directory / "memcheck.log"
        racecheck_path = directory / "racecheck.log"
        replay_binary_path = directory / "replay"
        output_path = directory / "output.partial"

        environment = make_environment(module, git_sha, image_id)
        write_json(environment_path, environment)
        write_json(replay_log_path, module.EXPECTED_REPLAY)
        elf_path.write_text("Fatbin elf code:\narch = sm_120\n", encoding="ascii")
        ptx_path.write_text("", encoding="ascii")
        ptx_stderr_path.write_text("", encoding="ascii")
        memcheck_path.write_text(
            "========= ERROR SUMMARY: 0 errors\n", encoding="ascii"
        )
        racecheck_path.write_text(
            "========= RACECHECK SUMMARY: 0 hazards displayed "
            "(0 errors, 0 warnings)\n",
            encoding="ascii",
        )
        replay_binary_path.write_bytes(b"phase5-real-cuda-replay-fixture\n")
        output_path.touch()

        arguments = [
            "--git-sha",
            git_sha,
            "--base-image-ref",
            module.BASE_IMAGE_REF,
            "--image-ref",
            f"morsehgp3d-phase3:{git_sha}",
            "--image-id",
            image_id,
            "--environment-artifact",
            str(environment_path),
            "--replay-log",
            str(replay_log_path),
            "--elf-log",
            str(elf_path),
            "--ptx-log",
            str(ptx_path),
            "--ptx-stderr-log",
            str(ptx_stderr_path),
            "--memcheck-log",
            str(memcheck_path),
            "--racecheck-log",
            str(racecheck_path),
            "--replay",
            str(replay_binary_path),
            "--output",
            str(output_path),
        ]
        require(module.main(arguments) == 0, "valid qualification fixture failed")
        raw, artifact = strict_json(output_path)
        canonical = json.dumps(
            artifact, ensure_ascii=False, sort_keys=True, separators=(",", ":")
        )
        require(raw == canonical + "\n", "qualification output is not canonical JSON")
        require(
            set(artifact) == EXPECTED_TOP_LEVEL_KEYS,
            "qualification top-level schema is not closed",
        )
        require(artifact.get("schema") == QUALIFICATION_SCHEMA, "wrong schema")
        expected_scalars = {
            "backend": "cuda_g4",
            "mode": "certified",
            "phase": "5",
            "profile": "hgp_reduced",
            "scientific_scope": SCIENTIFIC_SCOPE,
            "status": "worker_passed_pending_shutdown",
        }
        for field, expected in expected_scalars.items():
            require(artifact.get(field) == expected, f"wrong output field {field}")
        require("public_status" not in artifact, "qualification leaked public_status")
        require(
            artifact.get("scientific_public_status") is None
            and artifact.get("scientific_result_claimed") is False,
            "qualification overstates a scientific result",
        )
        require(
            artifact.get("git") == {"clean": True, "sha": git_sha},
            "qualification lost Git provenance",
        )
        require(
            artifact.get("image")
            == {
                "base_ref": module.BASE_IMAGE_REF,
                "id": image_id,
                "ref": f"morsehgp3d-phase3:{git_sha}",
            },
            "qualification lost image provenance",
        )
        binary = artifact.get("binary")
        replay_digest = hashlib.sha256(replay_binary_path.read_bytes()).hexdigest()
        require(
            isinstance(binary, dict)
            and set(binary) == {"full_replay_sha256"}
            and binary.get("full_replay_sha256") == replay_digest
            and SHA256_RE.fullmatch(replay_digest) is not None,
            "qualification full replay digest is absent",
        )
        checks = artifact.get("checks")
        require(
            isinstance(checks, dict) and set(checks) == EXPECTED_CHECK_KEYS,
            "qualification checks schema is not closed",
        )
        expected_checks = {
            "aot_elf_architectures": ["sm_120"],
            "aot_ptx_entry_count": 0,
            "bounded_candidate_emission_chain_certified": True,
            "bounded_morton_seed_chain_certified": True,
            "bounded_morton_window_certified": True,
            "candidate_payload_physical_bound_certified": True,
            "canonical_contractions_certified": True,
            "complete_source_partition_certified": True,
            "complete_source_seed_coverage_certified": True,
            "cpu_exact_decision_chain_certified": True,
            "cpu_exact_monotone_seed_cutoff_certified": True,
            "external_seed_targets_recertified": True,
            "gpu_multi_round_proposal_chain_certified": True,
            "independent_chunked_gpu_replay_certified": True,
            "independent_gpu_replay_certified": True,
            "independent_morton_seed_gpu_replay_certified": True,
            "local_emst_witness_certified": True,
            "memcheck": "passed",
            "racecheck": "passed",
            "replay": module.EXPECTED_REPLAY,
        }
        require(checks == expected_checks, "qualification checks differ")
        logs = artifact.get("logs")
        require(
            isinstance(logs, dict) and set(logs) == EXPECTED_LOG_KEYS,
            "qualification log schema is not closed",
        )
        require(
            logs.get("full_replay")
            == json.dumps(
                module.EXPECTED_REPLAY,
                ensure_ascii=True,
                sort_keys=True,
                separators=(",", ":"),
            )
            + "\n",
            "qualification did not preserve the canonical full replay log",
        )
        require(
            artifact.get("vm_lifecycle") == WORKER_LIFECYCLE,
            "qualification worker lifecycle is mutable or incomplete",
        )
        environment_digest = hashlib.sha256(environment_path.read_bytes()).hexdigest()
        require(
            artifact.get("provenance")
            == {
                "environment_artifact_schema": module.PHASE3_SCHEMA,
                "environment_artifact_sha256": environment_digest,
            },
            "qualification environment provenance is not content-addressed",
        )

        extra_key_replay = copy.deepcopy(module.EXPECTED_REPLAY)
        extra_key_replay["cases"][1]["rounds"][0]["audit"]["unexpected"] = True
        write_json(replay_log_path, extra_key_replay)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "unexpected nested replay key",
        )
        wrong_count_replay = copy.deepcopy(module.EXPECTED_REPLAY)
        wrong_count_replay["cases"][1]["rounds"][1]["audit"][
            "candidate_count"
        ] = 29
        write_json(replay_log_path, wrong_count_replay)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "wrong multi-round replay candidate count",
        )
        wrong_budget_replay = copy.deepcopy(module.EXPECTED_REPLAY)
        wrong_budget_replay["cases"][1]["trusted_chunking_policy"][
            "max_candidate_records_per_chunk"
        ] = 6
        write_json(replay_log_path, wrong_budget_replay)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "chunk budget below the certified atomic source",
        )
        wrong_payload_replay = copy.deepcopy(module.EXPECTED_REPLAY)
        wrong_payload_replay["cases"][1]["producer"][
            "chunked_emission_counters"
        ]["candidate_payload_peak_bytes"] = 223
        write_json(replay_log_path, wrong_payload_replay)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "wrong chunk payload high-water mark",
        )
        false_partition_replay = copy.deepcopy(module.EXPECTED_REPLAY)
        false_partition_replay["cases"][1]["rounds"][0]["emission_audit"][
            "certificates"
        ]["complete_source_partition_certified"] = False
        write_json(replay_log_path, false_partition_replay)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "false complete-source partition certificate",
        )
        wrong_replay_counter = copy.deepcopy(module.EXPECTED_REPLAY)
        wrong_replay_counter["cases"][1]["verifier"]["counters"][
            "gpu_replay_source_chunk_count"
        ] = 15
        write_json(replay_log_path, wrong_replay_counter)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "wrong independent replay chunk count",
        )

        wrong_seed_policy = copy.deepcopy(module.EXPECTED_REPLAY)
        wrong_seed_policy["cases"][3]["trusted_morton_seed_policy"][
            "window_radius"
        ] = 2
        write_json(replay_log_path, wrong_seed_policy)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "wrong trusted Morton seed window radius",
        )
        false_seed_chain = copy.deepcopy(module.EXPECTED_REPLAY)
        false_seed_chain["cases"][3]["producer"]["certificates"][
            "bounded_morton_seed_chain_certified"
        ] = False
        write_json(replay_log_path, false_seed_chain)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "false producer Morton seed chain certificate",
        )
        wrong_seed_inspection = copy.deepcopy(module.EXPECTED_REPLAY)
        wrong_seed_inspection["cases"][3]["rounds"][0]["morton_seed_audit"][
            "inspected_neighbor_count"
        ] = 13
        write_json(replay_log_path, wrong_seed_inspection)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "wrong bounded Morton seed inspection count",
        )
        false_monotone_cutoff = copy.deepcopy(module.EXPECTED_REPLAY)
        false_monotone_cutoff["cases"][3]["rounds"][0]["morton_seed_audit"][
            "certificates"
        ]["exact_monotone_cutoff_certified"] = False
        write_json(replay_log_path, false_monotone_cutoff)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "false exact monotone seed cutoff certificate",
        )
        wrong_seed_replay_counter = copy.deepcopy(module.EXPECTED_REPLAY)
        wrong_seed_replay_counter["cases"][3]["verifier"]["counters"][
            "gpu_replay_seed_selected_proposal_count"
        ] = 10
        write_json(replay_log_path, wrong_seed_replay_counter)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "wrong independent replay Morton seed selection count",
        )
        false_seed_mode = copy.deepcopy(module.EXPECTED_REPLAY)
        false_seed_mode["cases"][3]["verifier"]["certificates"][
            "seed_mode_certified"
        ] = False
        write_json(replay_log_path, false_seed_mode)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "false independent Morton seed mode certificate",
        )
        wrong_seed_comparison_volume = copy.deepcopy(module.EXPECTED_REPLAY)
        wrong_seed_comparison_volume["cases"][3]["morton_seed_comparison"][
            "refined"
        ]["logical_candidate_count"] = 42
        write_json(replay_log_path, wrong_seed_comparison_volume)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "wrong Morton seed comparison candidate volume",
        )
        false_seed_comparison_invariance = copy.deepcopy(module.EXPECTED_REPLAY)
        false_seed_comparison_invariance["cases"][3]["morton_seed_comparison"][
            "certificates"
        ]["exact_decisions_unchanged"] = False
        write_json(replay_log_path, false_seed_comparison_invariance)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "false Morton seed comparison exact-decision invariance",
        )

        replay_log_path.write_text(
            '{"schema":"one","schema":"two"}\n', encoding="ascii"
        )
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "duplicate replay key",
        )
        require_rejected(
            lambda: module.validate_binary_logs(
                elf_log="arch = sm_120\narch = sm_90\n",
                ptx_log="",
                memcheck_log="========= ERROR SUMMARY: 0 errors\n",
                racecheck_log=(
                    "========= RACECHECK SUMMARY: 0 hazards displayed "
                    "(0 errors, 0 warnings)\n"
                ),
            ),
            "foreign ELF architecture",
        )
        require_rejected(
            lambda: module.validate_binary_logs(
                elf_log="arch = sm_120\n",
                ptx_log="ptx entry\n",
                memcheck_log="========= ERROR SUMMARY: 0 errors\n",
                racecheck_log=(
                    "========= RACECHECK SUMMARY: 0 hazards displayed "
                    "(0 errors, 0 warnings)\n"
                ),
            ),
            "embedded PTX",
        )
        require_rejected(
            lambda: module.validate_memcheck_log("========= ERROR SUMMARY: 1 errors\n"),
            "memcheck error",
        )
        require_rejected(
            lambda: module.validate_racecheck_log(
                "========= RACECHECK SUMMARY: 1 hazards displayed "
                "(0 errors, 1 warnings)\n"
            ),
            "racecheck warning",
        )
        wrong_environment = copy.deepcopy(environment)
        wrong_environment["git"]["sha"] = "3" * 40
        require_rejected(
            lambda: module.validate_phase3_environment(
                wrong_environment,
                git_sha=git_sha,
                base_image_ref=module.BASE_IMAGE_REF,
                image_ref=f"morsehgp3d-phase3:{git_sha}",
                image_id=image_id,
            ),
            "environment Git mismatch",
        )
        mutable_environment = copy.deepcopy(environment)
        mutable_environment["vm_lifecycle"]["worker_mutates_gcp"] = True
        require_rejected(
            lambda: module.validate_phase3_environment(
                mutable_environment,
                git_sha=git_sha,
                base_image_ref=module.BASE_IMAGE_REF,
                image_ref=f"morsehgp3d-phase3:{git_sha}",
                image_id=image_id,
            ),
            "mutable worker lifecycle",
        )


def validate_project(project: Path) -> dict[str, object]:
    assembler_path = project / "tests/cuda/assemble_phase5_k1_boruvka_qualification.py"
    replay_path = project / "src/tools/gpu_k1_boruvka_full_replay.cpp"
    assembler_source = read_text(assembler_path)
    replay_source = read_text(replay_path)
    validate_static_source(assembler_source, replay_source)
    assembler = load_assembler(assembler_path)
    exercise_assembler(assembler)
    return {
        "checked_artifacts": [
            str(assembler_path.relative_to(project)),
            str(replay_path.relative_to(project)),
        ],
        "cuda_executed": False,
        "negative_mutations": 21,
        "qualification_schema": QUALIFICATION_SCHEMA,
        "replay_schema": REPLAY_SCHEMA,
        "schema": STATIC_SCHEMA,
    }


def main(arguments: list[str]) -> int:
    if len(arguments) != 2:
        print(
            "usage: check_phase5_k1_boruvka_qualification.py MORSEHGP3D_SOURCE",
            file=sys.stderr,
        )
        return 2
    project = Path(arguments[1]).resolve()
    try:
        result = validate_project(project)
    except ContractError as error:
        print(f"Phase 5 K1 Boruvka qualification failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, separators=(",", ":"), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
