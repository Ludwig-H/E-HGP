#!/usr/bin/env python3
"""Fail-closed preflight for the guarded point-hierarchy quality campaign v2."""

from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path
import stat
import sys
from typing import Any


class PreflightError(ValueError):
    """The guarded campaign must not start."""


def regular_file(path: Path, label: str, *, executable: bool = False) -> Path:
    if not path.is_absolute():
        raise PreflightError(f"{label} path must be absolute")
    try:
        metadata = path.lstat()
    except OSError as error:
        raise PreflightError(f"{label} is unreadable: {error}") from error
    if not stat.S_ISREG(metadata.st_mode) or stat.S_ISLNK(metadata.st_mode):
        raise PreflightError(f"{label} must be a non-symlink regular file")
    if executable and metadata.st_mode & 0o111 == 0:
        raise PreflightError(f"{label} must be executable")
    return path.resolve(strict=True)


def load_campaign_module(plan_path: Path) -> Any:
    script_path = regular_file(
        plan_path.with_name("point_hierarchy_quality_campaign.py"),
        "campaign harness",
    )
    sys.path.insert(0, str(script_path.parent))
    try:
        specification = importlib.util.spec_from_file_location(
            "_morsehgp3d_point_quality_campaign", script_path
        )
        if specification is None or specification.loader is None:
            raise PreflightError("campaign harness import specification is absent")
        module = importlib.util.module_from_spec(specification)
        specification.loader.exec_module(module)
        return module
    except (ImportError, OSError, RuntimeError) as error:
        raise PreflightError(f"campaign harness import failed: {error}") from error


def canonical_git_sha(value: str) -> str:
    if len(value) != 40 or any(character not in "0123456789abcdef" for character in value):
        raise PreflightError("expected git SHA must be canonical lowercase SHA-1")
    return value


def preflight(
    plan: Path,
    producer: Path,
    verifier: Path,
    *,
    expected_git_sha: str,
) -> dict[str, str]:
    expected = canonical_git_sha(expected_git_sha)
    plan_path = regular_file(plan, "campaign plan")
    campaign = load_campaign_module(plan_path)
    try:
        plan_value, plan_sha256 = campaign.read_plan(plan_path)
        # This is deliberately checked before either executable handshake.  A
        # closed scientific gate must stop the local orchestrator before SSH
        # key creation and before any billable GCP mutation.
        if plan_value["campaign_entry_gate_satisfied"] is not True:
            raise PreflightError("campaign entry gate is closed")

        producer_path = regular_file(producer, "quality producer", executable=True)
        verifier_path = regular_file(verifier, "scientific verifier", executable=True)
        if producer_path == verifier_path:
            raise PreflightError("producer and scientific verifier must be distinct files")
        producer_sha256 = campaign.sha256_file(producer_path, "quality producer")
        verifier_sha256 = campaign.sha256_file(verifier_path, "scientific verifier")
        if producer_sha256 == verifier_sha256:
            raise PreflightError("producer and scientific verifier hashes must differ")

        producer_capabilities = campaign.run_canonical_json_process(
            [str(producer_path), campaign.PRODUCER_CAPABILITY_ARGUMENT],
            request={},
            timeout_seconds=60.0,
            label="quality producer capability handshake",
        )
        verifier_capabilities = campaign.run_canonical_json_process(
            [str(verifier_path), campaign.VERIFIER_CAPABILITY_ARGUMENT],
            request={},
            timeout_seconds=60.0,
            label="scientific verifier capability handshake",
        )
        campaign._validate_capability(
            producer_capabilities,
            kind="producer",
            binary_sha256=producer_sha256,
            binary_size=producer_path.stat().st_size,
            plan_sha256=plan_sha256,
        )
        campaign._validate_capability(
            verifier_capabilities,
            kind="verifier",
            binary_sha256=verifier_sha256,
            binary_size=verifier_path.stat().st_size,
            plan_sha256=plan_sha256,
        )
    except PreflightError:
        raise
    except Exception as error:
        raise PreflightError(str(error)) from error

    producer_git_sha = producer_capabilities["git_sha"]
    verifier_git_sha = verifier_capabilities["git_sha"]
    if producer_git_sha != expected or verifier_git_sha != expected:
        raise PreflightError(
            "producer or verifier git_sha does not match the guarded checkout"
        )
    return {
        "generator_sha256": campaign.sha256_file(
            campaign.GENERATOR_PATH, "tracked point-cloud generator"
        ),
        "plan_sha256": plan_sha256,
        "producer_sha256": producer_sha256,
        "verifier_sha256": verifier_sha256,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--plan", type=Path, required=True)
    parser.add_argument("--producer", type=Path, required=True)
    parser.add_argument("--verifier", type=Path, required=True)
    parser.add_argument("--expected-git-sha", required=True)
    arguments = parser.parse_args()
    try:
        preflight(
            arguments.plan,
            arguments.producer,
            arguments.verifier,
            expected_git_sha=arguments.expected_git_sha,
        )
    except PreflightError as error:
        print(f"point-hierarchy quality preflight refused: {error}", file=sys.stderr)
        return 1
    print("point-hierarchy quality v2 preflight passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
