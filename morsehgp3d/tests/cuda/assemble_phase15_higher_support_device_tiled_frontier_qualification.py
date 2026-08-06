#!/usr/bin/env python3
"""Assemble guarded M5b device-tiled supports-3/4 frontier CUDA evidence.

Modeled on the schema-6 terminal-geometry assembler: the artifact binds the
clean commit, the pinned container image, the final Phase 3 environment with
its TERMINATED lifecycle, the release build, the four guarded runs of the
parity harness (unmasked device parity, scale, historic no-go re-measure,
50k SLO probe) and the memcheck and ELF evidence, embeds every log with its
SHA-256, and re-verifies every published closure identity
well + rank + terminal = C(n,3) + C(n,4) independently in exact Python
integers.  All scientific claims are false; the artifact qualifies the
component only.
"""

from __future__ import annotations

import argparse
import hashlib
import math
import re
from datetime import datetime
from pathlib import Path
from typing import Any, Sequence

from assemble_phase7_h_polytope_qualification import (
    BASE_IMAGE_REF,
    IMAGE_ID_RE,
    PHASE3_SCHEMA,
    SHA_RE,
    fail,
    parse_json_object,
    read_text_evidence,
    require_exact_keys,
    sha256_file,
    validate_elf_log,
    validate_memcheck_log,
    write_exclusive_atomic,
)
from assemble_phase15_exact_higher_support_terminal_geometry_cuda_qualification import (
    ALLOWED_GCP_PROJECT,
    ALLOWED_GCP_TARGETS,
    canonical_json,
)

SCHEMA = "morsehgp3d.phase15.higher_support_device_tiled_frontier_cuda_g4.v1"
ARTIFACT_ROLE = "slot_tiled_supports_3_4_frontier_component_qualification"
BACKEND = "cuda_g4"
PROFILE = "hgp_reduced"
MODE = "slot_tiled_higher_support_frontier_device_qualification_v1"
SCIENTIFIC_SCOPE = (
    "exact_supports_3_4_frontier_resolution_component_only_no_slo"
)
DEPLOYMENT_STATUS = "schema1_device_tiled_frontier_component_qualified"
COMPONENT_ABI_SCHEMA_VERSION = 1
QUALIFICATION_MODEL_SCHEMA_VERSION = 6
BINARY_RELATIVE_PATH = (
    "build/morsehgp3d-cuda-release/"
    "morsehgp3d_gpu_higher_support_device_tiled_frontier_parity"
)

PARITY_FINAL_LINE = (
    "higher-support device tiled frontier device parity passed"
)
SCALE_FINAL_LINE = "higher-support device tiled frontier scale runs passed"
NO_GO_FINAL_LINE = (
    "higher-support device tiled frontier no-go re-measure passed"
)
SLO_CLOSED_LINE = "higher-support device tiled frontier 50k probe closed"
SLO_CENSORED_LINE = (
    "higher-support device tiled frontier 50k probe censored "
    "without closure claim"
)

PARITY_CASE_LABELS = (
    "device/line12/K5",
    "device/line12/K3",
    "device/clusters10/K5",
    "device/sphere8/K3",
    "device/sphere8/carrier",
    "device/clusters10/quantum2",
    "device/wide-dyadic/K4",
)
SCALE_CASE_LABELS = (
    "scale/line32/parity",
    "scale/line64/native",
    "scale/line128/native",
)
NO_GO_FAMILIES = (
    "uniform_dyadic",
    "separated_clusters",
    "multiscale_clusters",
)
NO_GO_SIZES = (32, 64, 128)
NO_GO_RANKS = (5, 10)
SLO_POINT_COUNT = 50000
SLO_RANKS = (5, 10)

FLOAT_RE = r"[0-9]+(?:\.[0-9]+)?(?:e[+-]?[0-9]+)?"
SIMPLE_OK_RE = re.compile(
    r"^(?P<label>[^:]+): OK \((?P<chunks>[0-9]+) chunks\)$"
)
SCALE_OK_RE = re.compile(
    r"^(?P<label>[^:]+): OK in (?P<seconds>" + FLOAT_RE + r") s "
    r"\((?P<chunks>[0-9]+) chunks\)$"
)
TILED_OK_RE = re.compile(
    r"^(?P<label>[^:]+): OK in (?P<seconds>" + FLOAT_RE + r") s "
    r"\(pre-expansion (?P<expand_seconds>" + FLOAT_RE + r") s, "
    r"slots (?P<slots>[0-9]+), chunks (?P<chunks>[0-9]+), "
    r"subdivisions (?P<subdivisions>[0-9]+), "
    r"prune records (?P<prune_records>[0-9]+), "
    r"terminal records (?P<terminal_records>[0-9]+), "
    r"gates (?P<gates>[0-9]+), expansions (?P<expansions>[0-9]+), "
    r"deferred512 (?P<deferred512>[0-9]+), "
    r"deferred1024 (?P<deferred1024>[0-9]+), "
    r"rational drains (?P<rational_drains>[0-9]+), "
    r"well (?P<well>[0-9]+), rank (?P<rank>[0-9]+), "
    r"terminal (?P<terminal>[0-9]+)\)$"
)
HOST_PARITY_RE = re.compile(
    r"^(?P<label>[^:]+): host parity OK \((?P<chunks>[0-9]+) chunks\)$"
)
CENSORED_RE = re.compile(
    r"^(?P<label>[^:]+): CENSORED without closure claim after "
    r"(?P<seconds>" + FLOAT_RE + r") s \(slots (?P<slots>[0-9]+), "
    r"chunks (?P<chunks>[0-9]+), subdivisions (?P<subdivisions>[0-9]+), "
    r"stop_reason (?P<stop_reason>[0-9]+), "
    r"failure_code (?P<failure_code>[0-9]+), "
    r"(?P<detail>fatal|chunk budget exhausted|deadline exceeded)\)$"
)


def universe_mass(point_count: int) -> int:
    return math.comb(point_count, 3) + math.comb(point_count, 4)


def parse_tiled_ok(line: str, label: str) -> dict[str, Any]:
    match = TILED_OK_RE.fullmatch(line)
    if match is None:
        fail(f"{label} is not a canonical tiled OK line: {line}")
    record: dict[str, Any] = {"label": match["label"]}
    for field in (
        "slots",
        "chunks",
        "subdivisions",
        "prune_records",
        "terminal_records",
        "gates",
        "expansions",
        "deferred512",
        "deferred1024",
        "rational_drains",
    ):
        record[field] = int(match[field])
    for field in ("seconds", "expand_seconds"):
        record[field] = float(match[field])
    for field in ("well", "rank", "terminal"):
        record[field] = match[field]
    return record


def verify_closure(record: dict[str, Any], point_count: int) -> None:
    resolved = (
        int(record["well"]) + int(record["rank"]) + int(record["terminal"])
    )
    if resolved != universe_mass(point_count):
        fail(
            f"{record['label']} does not close its exact universe: "
            f"{resolved} != C({point_count},3) + C({point_count},4)"
        )


def validate_parity_log(value: str) -> dict[str, Any]:
    lines = [line for line in value.splitlines() if line.strip()]
    if not lines or lines[-1] != PARITY_FINAL_LINE:
        fail("the parity log does not end with the unmasked parity pass")
    labels = []
    for line in lines[:-1]:
        match = SIMPLE_OK_RE.fullmatch(line)
        if match is None:
            fail(f"unexpected parity log line: {line}")
        labels.append(match["label"])
    if tuple(labels) != PARITY_CASE_LABELS:
        fail(f"parity cases differ from the sealed seven: {labels}")
    return {"cases": labels, "status": "passed"}


def validate_scale_log(value: str) -> dict[str, Any]:
    lines = [line for line in value.splitlines() if line.strip()]
    if not lines or lines[-1] != SCALE_FINAL_LINE:
        fail("the scale log does not end with the scale pass")
    cases = []
    for line in lines[:-1]:
        match = SCALE_OK_RE.fullmatch(line)
        if match is None:
            fail(f"unexpected scale log line: {line}")
        cases.append(
            {
                "label": match["label"],
                "seconds": float(match["seconds"]),
                "chunks": int(match["chunks"]),
            }
        )
    if tuple(case["label"] for case in cases) != SCALE_CASE_LABELS:
        fail("scale cases differ from line32/line64/line128")
    return {"cases": cases, "status": "passed"}


def validate_no_go_log(value: str) -> dict[str, Any]:
    lines = [line for line in value.splitlines() if line.strip()]
    if not lines or lines[-1] != NO_GO_FINAL_LINE:
        fail("the no-go log does not end with the re-measure pass")
    expected_labels = [
        f"no-go/{family}/n{size}/K{rank}"
        for size in NO_GO_SIZES
        for family in NO_GO_FAMILIES
        for rank in NO_GO_RANKS
    ]
    cases: list[dict[str, Any]] = []
    host_parity_labels: list[str] = []
    for line in lines[:-1]:
        host_match = HOST_PARITY_RE.fullmatch(line)
        if host_match is not None:
            host_parity_labels.append(host_match["label"])
            continue
        record = parse_tiled_ok(line, "no-go log")
        size = int(record["label"].split("/n")[1].split("/")[0])
        verify_closure(record, size)
        cases.append(record)
    if [case["label"] for case in cases] != expected_labels:
        fail(
            "no-go cases differ from the eighteen historic-family cases: "
            f"{[case['label'] for case in cases]}"
        )
    for label in host_parity_labels:
        if label not in expected_labels or "/n32/" not in label:
            fail(f"host parity reported outside the n=32 cases: {label}")
    return {
        "cases": cases,
        "host_parity_labels": host_parity_labels,
        "status": "passed",
    }


def validate_slo50k_log(value: str) -> dict[str, Any]:
    lines = [line for line in value.splitlines() if line.strip()]
    if not lines or lines[-1] not in (SLO_CLOSED_LINE, SLO_CENSORED_LINE):
        fail("the 50k probe log does not end with a canonical status line")
    closed = lines[-1] == SLO_CLOSED_LINE
    expected_labels = [
        f"slo50k/uniform_dyadic/n{SLO_POINT_COUNT}/K{rank}"
        for rank in SLO_RANKS
    ]
    cases: list[dict[str, Any]] = []
    for line in lines[:-1]:
        censored_match = CENSORED_RE.fullmatch(line)
        if censored_match is not None:
            cases.append(
                {
                    "label": censored_match["label"],
                    "censored": True,
                    "seconds": float(censored_match["seconds"]),
                    "slots": int(censored_match["slots"]),
                    "chunks": int(censored_match["chunks"]),
                    "subdivisions": int(censored_match["subdivisions"]),
                    "stop_reason": int(censored_match["stop_reason"]),
                    "failure_code": int(censored_match["failure_code"]),
                    "detail": censored_match["detail"],
                }
            )
            continue
        record = parse_tiled_ok(line, "50k probe log")
        verify_closure(record, SLO_POINT_COUNT)
        record["censored"] = False
        cases.append(record)
    if [case["label"] for case in cases] != expected_labels:
        fail("50k probe cases differ from the two uniform_dyadic ranks")
    if closed and any(case["censored"] for case in cases):
        fail("the 50k probe claims closure over a censored case")
    if not closed and all(not case["censored"] for case in cases):
        fail("the 50k probe censors without any censored case")
    return {
        "cases": cases,
        "closure_claimed": closed,
        "status": "closed" if closed else "censored_without_claim",
    }


FINAL_LIFECYCLE_KEYS = {
    "final_status",
    "final_status_readback",
    "final_status_verified_at_utc",
    "guest_shutdown_guard_verified",
    "guest_shutdown_minutes",
    "initial_status",
    "initial_status_basis",
    "instance",
    "last_start_timestamp",
    "project",
    "start_handoff_schema",
    "stop_responsibility",
    "targeted_stop_verified",
    "worker_mutates_gcp",
    "worker_status_before_targeted_stop",
    "zone",
}


def validate_environment_lifecycle_shape(
    lifecycle: Any, label: str
) -> dict[str, Any]:
    """Validates a FINAL lifecycle in shape without requiring identity with
    the qualification session: the Phase 3 environment companion may come
    from its own dedicated guarded session at the same SHA and image."""
    if not isinstance(lifecycle, dict):
        fail(f"{label} lifecycle is absent")
    require_exact_keys(lifecycle, FINAL_LIFECYCLE_KEYS, f"{label} lifecycle")
    for field, expected in {
        "final_status": "TERMINATED",
        "final_status_readback": "gcloud_compute_instances_describe",
        "guest_shutdown_guard_verified": True,
        "initial_status": "TERMINATED",
        "initial_status_basis": "start_and_verify_precondition",
        "start_handoff_schema": "e-hgp.start-handoff.v3",
        "stop_responsibility": "external_orchestrator",
        "targeted_stop_verified": True,
        "worker_mutates_gcp": False,
        "worker_status_before_targeted_stop": (
            "worker_passed_pending_shutdown"
        ),
    }.items():
        if lifecycle.get(field) != expected:
            fail(f"{label} lifecycle {field} must be {expected}")
    if lifecycle.get("project") != ALLOWED_GCP_PROJECT:
        fail(f"{label} lifecycle project is outside the allowlist")
    if (
        lifecycle.get("zone"),
        lifecycle.get("instance"),
    ) not in ALLOWED_GCP_TARGETS:
        fail(f"{label} lifecycle zone/instance is outside the allowlist")
    minutes = lifecycle.get("guest_shutdown_minutes")
    if not isinstance(minutes, int) or not 1 <= minutes <= 450:
        fail(f"{label} lifecycle guest guard is outside the session cap")
    if (
        re.fullmatch(
            r"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z",
            str(lifecycle.get("final_status_verified_at_utc")),
        )
        is None
    ):
        fail(f"{label} lifecycle verification timestamp is not canonical")
    return lifecycle


def validate_phase3_environment_artifact(
    path: Path,
    *,
    git_sha: str,
    image_ref: str,
    image_id: str,
) -> tuple[str, dict[str, Any]]:
    raw, digest = read_text_evidence(path, "Phase 3 environment artifact")
    value = parse_json_object(raw, "Phase 3 environment artifact")
    if raw != canonical_json(value):
        fail("Phase 3 environment artifact must be canonical JSON")
    for field, expected in {
        "backend": "cuda_g4",
        "mode": "certified",
        "phase": "3",
        "profile": "hgp_reduced",
        "schema": PHASE3_SCHEMA,
        "scientific_scope": "environment_reproducibility_only",
        "status": "passed",
    }.items():
        if value.get(field) != expected:
            fail(f"Phase 3 environment {field} differs")
    if value.get("scientific_result_claimed") is not False:
        fail("Phase 3 environment claims a scientific result")
    if (
        value.get("scientific_public_status") is not None
        or "public_status" in value
    ):
        fail("Phase 3 environment publishes a scientific status")
    if value.get("git") != {"clean": True, "sha": git_sha}:
        fail("Phase 3 environment does not bind the qualified commit")
    image = value.get("image")
    if not isinstance(image, dict) or any(
        image.get(field) != expected
        for field, expected in {
            "base_ref": BASE_IMAGE_REF,
            "id": image_id,
            "ref": image_ref,
        }.items()
    ):
        fail("Phase 3 environment image identity differs")
    checks = value.get("checks")
    if not isinstance(checks, dict) or any(
        checks.get(field) != "passed"
        for field in ("cuda_audit_workflow", "cuda_release_workflow")
    ):
        fail("Phase 3 environment workflows are not passed")
    lifecycle = validate_environment_lifecycle_shape(
        value.get("vm_lifecycle"), "Phase 3 environment"
    )
    return digest, lifecycle


def expected_final_lifecycle(
    *,
    project: str,
    zone: str,
    instance: str,
    guest_shutdown_minutes: int,
    final_status_verified_at_utc: str,
    last_start_timestamp: str,
) -> dict[str, Any]:
    if not 1 <= guest_shutdown_minutes <= 345:
        fail("the guest shutdown guard must be armed within the session cap")
    if project != ALLOWED_GCP_PROJECT:
        fail("final lifecycle project is outside the qualification allowlist")
    if (zone, instance) not in ALLOWED_GCP_TARGETS:
        fail(
            "final lifecycle zone/instance pair is outside the "
            "qualification allowlist"
        )
    for label, value in {
        "project": project,
        "zone": zone,
        "instance": instance,
        "last_start_timestamp": last_start_timestamp,
    }.items():
        if not value or "\n" in value or "\r" in value:
            fail(f"final lifecycle {label} is empty or non-canonical")
    try:
        parsed_last_start = datetime.fromisoformat(
            last_start_timestamp[:-1] + "+00:00"
            if last_start_timestamp.endswith("Z")
            else last_start_timestamp
        )
    except ValueError:
        fail("final lifecycle last_start_timestamp is not valid ISO-8601")
    if (
        parsed_last_start.tzinfo is None
        or parsed_last_start.utcoffset() is None
    ):
        fail("final lifecycle last_start_timestamp must include a UTC offset")
    if (
        re.fullmatch(
            r"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z",
            final_status_verified_at_utc,
        )
        is None
    ):
        fail("final lifecycle verification timestamp is not canonical UTC")
    return {
        "final_status": "TERMINATED",
        "final_status_readback": "gcloud_compute_instances_describe",
        "final_status_verified_at_utc": final_status_verified_at_utc,
        "guest_shutdown_guard_verified": True,
        "guest_shutdown_minutes": guest_shutdown_minutes,
        "initial_status": "TERMINATED",
        "initial_status_basis": "start_and_verify_precondition",
        "instance": instance,
        "last_start_timestamp": last_start_timestamp,
        "project": project,
        "start_handoff_schema": "e-hgp.start-handoff.v3",
        "stop_responsibility": "external_orchestrator",
        "targeted_stop_verified": True,
        "worker_mutates_gcp": False,
        "worker_status_before_targeted_stop": "worker_passed_pending_shutdown",
        "zone": zone,
    }


def parse_arguments(arguments: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--git-tree", required=True)
    parser.add_argument("--image-ref", required=True)
    parser.add_argument("--image-id", required=True)
    parser.add_argument("--environment-artifact", type=Path, required=True)
    parser.add_argument("--release-build-log", type=Path, required=True)
    parser.add_argument("--parity-log", type=Path, required=True)
    parser.add_argument("--scale-log", type=Path, required=True)
    parser.add_argument("--no-go-log", type=Path, required=True)
    parser.add_argument("--slo50k-log", type=Path, required=True)
    parser.add_argument("--elf-log", type=Path, required=True)
    parser.add_argument("--memcheck-log", type=Path, required=True)
    parser.add_argument("--qualified-binary", type=Path, required=True)
    parser.add_argument(
        "--slo50k-budget-seconds", type=int, required=True
    )
    parser.add_argument("--gcp-project", required=True)
    parser.add_argument("--gcp-zone", required=True)
    parser.add_argument("--gcp-instance", required=True)
    parser.add_argument(
        "--guest-shutdown-minutes", type=int, required=True
    )
    parser.add_argument("--last-start-timestamp", required=True)
    parser.add_argument("--final-status-verified-at-utc", required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    options = parse_arguments(arguments)
    if SHA_RE.fullmatch(options.git_sha) is None:
        fail("--git-sha must be a full lowercase commit SHA")
    if SHA_RE.fullmatch(options.git_tree) is None:
        fail("--git-tree must be a full lowercase tree SHA")
    if options.image_ref != f"morsehgp3d-phase3:{options.git_sha}":
        fail("--image-ref must bind the qualified SHA")
    if IMAGE_ID_RE.fullmatch(options.image_id) is None:
        fail("--image-id is not canonical")
    if options.slo50k_budget_seconds <= 0:
        fail("--slo50k-budget-seconds must be positive")

    logs: dict[str, str] = {}
    for name, path in {
        "release_build": options.release_build_log,
        "parity": options.parity_log,
        "scale": options.scale_log,
        "no_go": options.no_go_log,
        "slo50k": options.slo50k_log,
        "elf": options.elf_log,
        "memcheck": options.memcheck_log,
    }.items():
        raw, _ = read_text_evidence(path, f"{name} log")
        logs[name] = raw
    log_sha256 = {
        name: hashlib.sha256(raw.encode("utf-8")).hexdigest()
        for name, raw in logs.items()
    }

    checks = {
        "aot_elf_architectures": validate_elf_log(logs["elf"]),
        "component_abi_schema_version": COMPONENT_ABI_SCHEMA_VERSION,
        "memcheck": "passed",
        "no_go_re_measure": validate_no_go_log(logs["no_go"]),
        "parity": validate_parity_log(logs["parity"]),
        "qualification_model_schema_version": (
            QUALIFICATION_MODEL_SCHEMA_VERSION
        ),
        "scale": validate_scale_log(logs["scale"]),
        "slo50k_probe": validate_slo50k_log(logs["slo50k"]),
    }
    validate_memcheck_log(logs["memcheck"])
    if PARITY_FINAL_LINE not in logs["memcheck"]:
        fail("the memcheck log must wrap the unmasked parity pass")

    lifecycle = expected_final_lifecycle(
        project=options.gcp_project,
        zone=options.gcp_zone,
        instance=options.gcp_instance,
        guest_shutdown_minutes=options.guest_shutdown_minutes,
        final_status_verified_at_utc=options.final_status_verified_at_utc,
        last_start_timestamp=options.last_start_timestamp,
    )
    environment_sha256, environment_lifecycle = (
        validate_phase3_environment_artifact(
            options.environment_artifact,
            git_sha=options.git_sha,
            image_ref=options.image_ref,
            image_id=options.image_id,
        )
    )

    artifact = {
        "artifact_role": ARTIFACT_ROLE,
        "backend": BACKEND,
        "binary": {
            "relative_path": BINARY_RELATIVE_PATH,
            "sha256": sha256_file(
                options.qualified_binary, "qualified binary"
            ),
        },
        "checks": checks,
        "claims": {
            "exact_public_status": False,
            "global_structure_materialized": False,
            "massive_resume": False,
            "phase16_entry": False,
            "slo_50k": False,
            "true_50k_hgp": False,
        },
        "commands": {
            "memcheck_application": (
                "compute-sanitizer --tool memcheck "
                + BINARY_RELATIVE_PATH
            ),
            "no_go": BINARY_RELATIVE_PATH + " --no-go",
            "parity": BINARY_RELATIVE_PATH,
            "scale": BINARY_RELATIVE_PATH + " --scale",
            "slo50k": (
                BINARY_RELATIVE_PATH
                + f" --slo50k {options.slo50k_budget_seconds}"
            ),
        },
        "component_only": True,
        "deployment_status": DEPLOYMENT_STATUS,
        "git": {
            "clean": True,
            "sha": options.git_sha,
            "tree": options.git_tree,
        },
        "image": {
            "base_ref": BASE_IMAGE_REF,
            "id": options.image_id,
            "ref": options.image_ref,
        },
        "log_sha256": log_sha256,
        "logs": logs,
        "mode": MODE,
        "phase": "15",
        "profile": PROFILE,
        "provenance": {
            "environment_artifact_schema": PHASE3_SCHEMA,
            "environment_artifact_sha256": environment_sha256,
            "environment_session_lifecycle": environment_lifecycle,
        },
        "schema": SCHEMA,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": SCIENTIFIC_SCOPE,
        "status": "passed",
        "vm_lifecycle": lifecycle,
    }
    if canonical_json(artifact) != canonical_json(
        parse_json_object(canonical_json(artifact), "artifact self-check")
    ):
        fail("the artifact does not round-trip canonically")
    write_exclusive_atomic(options.output, artifact)
    print(f"assembled {options.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
