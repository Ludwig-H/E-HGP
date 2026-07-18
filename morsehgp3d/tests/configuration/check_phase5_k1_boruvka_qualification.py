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

QUALIFICATION_SCHEMA = "morsehgp3d.phase5.k1_boruvka_gpu_qualification.v1"
REPLAY_SCHEMA = "morsehgp3d.phase5.k1_boruvka_gpu_replay.v1"
SCIENTIFIC_SCOPE = "gpu_candidate_superset_with_cpu_exact_resolution_only"
STATIC_SCHEMA = "morsehgp3d.phase5.k1_boruvka_gpu_qualification_static.v1"
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
    "cpu_exact_resolution_complete",
    "gpu_candidate_superset_certified",
    "memcheck",
    "racecheck",
    "replay",
}
EXPECTED_LOG_KEYS = {
    "cuobjdump_elf",
    "cuobjdump_ptx",
    "cuobjdump_ptx_stderr",
    "memcheck",
    "racecheck",
    "replay",
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
            SCIENTIFIC_SCOPE,
            '"candidate_count": 15',
            '"case_count": 2',
            '"count_pass_node_visit_count": 48',
            '"point_count": 8',
            '"required_candidate_count": 15',
            '"strict_aabb_prune_count": 5',
            '"uniform_component_prune_count": 8',
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
            "run_square(summary);",
            "run_contracted_chain(summary);",
            REPLAY_SCHEMA,
            "K1BoruvkaCandidateAudit::decision_semantics",
            "K1BoruvkaCandidateAudit::proposal_semantics",
            r"\"status\":\"passed\"",
            "proposal.cpu_exact_component_minima !=",
            "proposal.audit.candidate_superset_certified",
            "proposal.audit.cpu_exact_resolution_complete",
        ),
        "real Phase 5 K1 Boruvka replay",
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
        require(
            isinstance(binary, dict)
            and set(binary) == {"replay_sha256"}
            and SHA256_RE.fullmatch(str(binary.get("replay_sha256"))) is not None,
            "qualification replay digest is absent",
        )
        checks = artifact.get("checks")
        require(
            isinstance(checks, dict) and set(checks) == EXPECTED_CHECK_KEYS,
            "qualification checks schema is not closed",
        )
        expected_checks = {
            "aot_elf_architectures": ["sm_120"],
            "aot_ptx_entry_count": 0,
            "cpu_exact_resolution_complete": True,
            "gpu_candidate_superset_certified": True,
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
        extra_key_replay["unexpected"] = True
        write_json(replay_log_path, extra_key_replay)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "unexpected replay key",
        )
        wrong_count_replay = copy.deepcopy(module.EXPECTED_REPLAY)
        wrong_count_replay["candidate_count"] = 14
        write_json(replay_log_path, wrong_count_replay)
        require_rejected(
            lambda: module.validate_replay(replay_log_path),
            "wrong replay candidate count",
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
    replay_path = project / "src/tools/gpu_k1_boruvka_replay.cpp"
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
        "negative_mutations": 9,
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
