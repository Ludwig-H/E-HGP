#!/usr/bin/env python3
"""Qualify the closed Phase 2A exact-level checkpoint coordinator."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path
from typing import Any, Iterable, Sequence

MAX_SAFE_JSON_INTEGER = (1 << 53) - 1
INTERNAL_SCHEMA_VERSION = "1.0.0"
EMISSION_KIND = "morsehgp3d_phase2a_level_emission"
CHECKPOINT_KIND = "morsehgp3d_phase2a_level_checkpoint"
CHECKPOINT_SCHEMA_VERSION = INTERNAL_SCHEMA_VERSION
SORT_ORDER_ID = "exact-level-support-source-v1"
FAILPOINT_ENVIRONMENT = "MORSEHGP3D_PHASE2A_CHECKPOINT_FAIL_AFTER"
CHECKPOINT_KEYS = frozenset(
    {
        "checkpoint_id",
        "kind",
        "schema_version",
        "state",
        "sequence_number",
        "input_sha256",
        "config_sha256",
        "tool_sha256",
        "native_executable_sha256",
        "chunk_size",
        "emission_count",
        "next_emission_index",
        "next_input_offset_bytes",
        "runs",
        "run_catalog_sha256",
        "run_cursors",
        "closed_levels",
        "last_fully_closed_level",
        "result_artifact",
        "sort_order_id",
        "public_contract_claimed",
    }
)
CHECKPOINT_STATES = frozenset({"collecting_runs", "between_levels", "complete"})
EXPECTED_EMISSION_COUNT = 31
EXPECTED_UNIQUE_EMISSION_COUNT = 26
EXPECTED_DUPLICATE_EMISSION_COUNT = 5
EXPECTED_LEVEL_COUNT = 9
EXPECTED_UNIT_LEVEL_EMISSION_COUNT = 11
EXPECTED_UNIT_LEVEL_SUPPORT_COUNT = 5
EXPECTED_INPUT_SHA256 = (
    "dfc0497be5151f77073afb5956c0d113c6f7ffcad667a65959f751444e8930f6"
)
EXPECTED_ORACLE_SHA256 = (
    "3ca27bc45f134bd7c295ccfa6327de0cf60948cbd324cf2495ddc7d7d16245dc"
)
EXPECTED_INPUT_BYTE_COUNT = 8_498
EXPECTED_ORACLE_BYTE_COUNT = 7_782
EXPECTED_CLOSED_FAILURE_COUNT = 14
EXPECTED_FAILPOINT_COUNT = 5
RUN_MATRIX = (
    (31, 1),
    (16, 2),
    (11, 3),
    (5, 7),
    (1, 31),
)
COORDINATOR_TIMEOUT_SECONDS = 30
NATIVE_TIMEOUT_SECONDS = 30


class DuplicateKeyError(ValueError):
    """Raised when a supposedly closed JSON object repeats a key."""


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    output: dict[str, Any] = {}
    for key, value in pairs:
        if key in output:
            raise DuplicateKeyError(f"duplicate JSON key: {key}")
        output[key] = value
    return output


def reject_nonfinite(value: str) -> None:
    raise ValueError(f"non-finite JSON value: {value}")


def strict_json_loads(text: str) -> Any:
    return json.loads(
        text,
        object_pairs_hook=reject_duplicate_keys,
        parse_constant=reject_nonfinite,
    )


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def require_plain_integer(value: Any, label: str, minimum: int = 0) -> int:
    if type(value) is not int or value < minimum:
        raise AssertionError(f"{label} must be an integer >= {minimum}")
    return value


@dataclass(frozen=True)
class Emission:
    level: Fraction
    minimal_support_ids: tuple[int, ...]
    source_support_ids: tuple[int, ...]

    def __post_init__(self) -> None:
        require_support(self.minimal_support_ids, "minimal support")
        require_support(self.source_support_ids, "source support")
        if not set(self.minimal_support_ids).issubset(self.source_support_ids):
            raise ValueError("a minimal support must be contained in its source")


def require_support(identifiers: Sequence[int], label: str) -> None:
    if not 1 <= len(identifiers) <= 4:
        raise ValueError(f"{label} must contain one through four identifiers")
    if tuple(sorted(identifiers)) != tuple(identifiers):
        raise ValueError(f"{label} must be sorted")
    if len(set(identifiers)) != len(identifiers):
        raise ValueError(f"{label} must contain unique identifiers")
    if any(
        type(identifier) is not int
        or identifier < 0
        or identifier > MAX_SAFE_JSON_INTEGER
        for identifier in identifiers
    ):
        raise ValueError(f"{label} contains an identifier outside the v2 domain")


def make_emission(
    level: Fraction,
    minimal: Iterable[int],
    source: Iterable[int] | None = None,
) -> Emission:
    minimal_ids = tuple(sorted(minimal))
    source_ids = minimal_ids if source is None else tuple(sorted(source))
    return Emission(level, minimal_ids, source_ids)


def base_chain(index: int) -> tuple[Emission, ...]:
    if index == 0:
        base = 0
    elif index == 31:
        base = MAX_SAFE_JSON_INTEGER - 100
    else:
        raise ValueError("the frozen checkpoint corpus uses only chains 0 and 31")

    below_one = Fraction(
        81_129_638_414_606_663_681_390_495_662_081,
        81_129_638_414_606_681_695_789_005_144_064,
    )
    unit = Fraction(1)
    scaled = Fraction(25, 64)
    nondyadic = Fraction(25, 9)
    high = Fraction(1000 + 2 * index, 3)

    singleton_a = (base,)
    singleton_b = (base + 1,)
    below_pair = (base + 2, base + 3)
    unit_pair = (base + 4, base + 5)
    unit_triangle = (base + 8, base + 9, base + 10)
    unit_tetrahedron = (base + 12, base + 13, base + 14, base + 15)
    scaled_pair = (base + 16, base + 17)
    scaled_triangle = (base + 18, base + 19, base + 20)
    scaled_tetrahedron = (base + 21, base + 22, base + 23, base + 24)
    nondyadic_triangle = (base + 25, base + 26, base + 27)
    translated_triangle = (base + 28, base + 29, base + 30)
    high_pair = (base + 31, base + 32)
    high_tetrahedron = (base + 33, base + 34, base + 35, base + 36)

    return (
        make_emission(Fraction(), singleton_a),
        make_emission(Fraction(), singleton_a),
        make_emission(Fraction(), singleton_b),
        make_emission(below_one, below_pair),
        make_emission(unit, unit_pair),
        make_emission(unit, unit_pair),
        make_emission(unit, unit_pair, (*unit_pair, base + 6)),
        make_emission(unit, unit_pair, (*unit_pair, base + 6, base + 7)),
        make_emission(unit, unit_triangle),
        make_emission(unit, unit_triangle, (*unit_triangle, base + 11)),
        make_emission(unit, unit_tetrahedron),
        make_emission(scaled, scaled_pair),
        make_emission(scaled, scaled_triangle),
        make_emission(scaled, scaled_tetrahedron),
        make_emission(nondyadic, nondyadic_triangle),
        make_emission(nondyadic, translated_triangle),
        make_emission(high, high_pair),
        make_emission(high, high_pair),
        make_emission(high, high_pair, (*high_pair, base + 37)),
        make_emission(high, high_tetrahedron),
    )


def build_corpus() -> tuple[Emission, ...]:
    primary = base_chain(0)
    boundary = base_chain(31)
    selected_boundary = tuple(
        boundary[index] for index in (0, 1, 2, 4, 5, 6, 8, 11, 16)
    )
    magnitude = 1 << 128
    adjacent_lower = make_emission(Fraction(magnitude - 1, magnitude), (38, 39))
    adjacent_upper = make_emission(Fraction(magnitude, magnitude + 1), (40, 41))
    # Positions 24 and 25 deliberately straddle the size-five run boundary.
    return (
        *primary,
        *selected_boundary[:4],
        adjacent_lower,
        adjacent_upper,
        *selected_boundary[4:],
    )


def level_record(level: Fraction) -> dict[str, str]:
    return {
        "denominator": str(level.denominator),
        "numerator": str(level.numerator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }


def emission_record(emission: Emission) -> dict[str, object]:
    return {
        "kind": EMISSION_KIND,
        "minimal_support_ids": list(emission.minimal_support_ids),
        "schema_version": INTERNAL_SCHEMA_VERSION,
        "source_support_ids": list(emission.source_support_ids),
        "squared_level_exact": level_record(emission.level),
    }


def encode_corpus(emissions: Sequence[Emission]) -> bytes:
    return "".join(
        canonical_json(emission_record(emission)) + "\n" for emission in emissions
    ).encode("utf-8")


def fraction_oracle(emissions: Sequence[Emission]) -> dict[str, object]:
    level_by_minimal: dict[tuple[int, ...], Fraction] = {}
    grouped: dict[Fraction, dict[tuple[int, ...], dict[tuple[int, ...], int]]] = {}
    for emission in emissions:
        previous = level_by_minimal.setdefault(
            emission.minimal_support_ids, emission.level
        )
        if previous != emission.level:
            raise ValueError("one minimal support carries contradictory exact levels")
        sources = grouped.setdefault(emission.level, {}).setdefault(
            emission.minimal_support_ids, {}
        )
        sources[emission.source_support_ids] = (
            sources.get(emission.source_support_ids, 0) + 1
        )

    batches: list[dict[str, object]] = []
    unique_count = 0
    for level in sorted(grouped):
        supports_output: list[dict[str, object]] = []
        batch_count = 0
        for minimal in sorted(grouped[level], key=lambda ids: (len(ids), ids)):
            source_counts = grouped[level][minimal]
            sources = sorted(source_counts, key=lambda ids: (len(ids), ids))
            support_count = sum(source_counts.values())
            batch_count += support_count
            unique_count += len(sources)
            supports_output.append(
                {
                    "emission_count": support_count,
                    "minimal_support_ids": list(minimal),
                    "source_provenance": [
                        {
                            "emission_count": source_counts[source],
                            "source_support_ids": list(source),
                        }
                        for source in sources
                    ],
                    "squared_level_exact": level_record(level),
                }
            )
        batches.append(
            {
                "emission_count": batch_count,
                "squared_level_exact": level_record(level),
                "supports": supports_output,
            }
        )

    emission_count = len(emissions)
    return {
        "duplicate_emission_count": emission_count - unique_count,
        "emission_count": emission_count,
        "equal_level_batches": batches,
        "predicate": "canonical_level_batches",
        "unique_emission_count": unique_count,
    }


def native_arguments(emissions: Sequence[Emission]) -> list[str]:
    arguments = ["canonical_level_batches", str(len(emissions))]
    for emission in emissions:
        arguments.extend(
            (
                str(emission.level.numerator),
                str(emission.level.denominator),
                str(len(emission.minimal_support_ids)),
                *(str(identifier) for identifier in emission.minimal_support_ids),
                str(len(emission.source_support_ids)),
                *(str(identifier) for identifier in emission.source_support_ids),
            )
        )
    return arguments


def run_native_one_shot(native: Path, emissions: Sequence[Emission]) -> bytes:
    completed = subprocess.run(
        [str(native), *native_arguments(emissions)],
        check=False,
        capture_output=True,
        timeout=NATIVE_TIMEOUT_SECONDS,
    )
    if completed.returncode != 0:
        raise AssertionError(
            "the native one-shot level oracle failed: "
            + completed.stderr.decode("utf-8", errors="replace").strip()
        )
    if completed.stderr:
        raise AssertionError("the native one-shot level oracle wrote diagnostics")
    observed = strict_json_loads(completed.stdout.decode("utf-8"))
    canonical = (canonical_json(observed) + "\n").encode("utf-8")
    if completed.stdout != canonical:
        raise AssertionError("the native one-shot level result is not canonical JSON")
    return completed.stdout


def coordinator_command(
    coordinator: Path,
    native: Path,
    input_path: Path,
    checkpoint_directory: Path,
    chunk_size: int,
    max_transactions: int | None,
) -> list[str]:
    command = [
        sys.executable,
        str(coordinator),
        "--native",
        str(native),
        "--input",
        str(input_path),
        "--checkpoint-dir",
        str(checkpoint_directory),
        "--chunk-size",
        str(chunk_size),
    ]
    if max_transactions is not None:
        command.extend(("--max-transactions", str(max_transactions)))
    return command


def coordinator_environment(failpoint: str | None = None) -> dict[str, str]:
    environment = dict(os.environ)
    environment.pop(FAILPOINT_ENVIRONMENT, None)
    if failpoint is not None:
        environment[FAILPOINT_ENVIRONMENT] = failpoint
    return environment


def audit_summary(stdout: bytes) -> dict[str, Any]:
    value = strict_json_loads(stdout.decode("utf-8"))
    if not isinstance(value, dict) or set(value) != {
        "completed",
        "sequence_number",
        "state",
    }:
        raise AssertionError("the coordinator summary does not have its closed schema")
    if type(value["completed"]) is not bool:
        raise AssertionError("the coordinator completed flag is not boolean")
    require_plain_integer(value["sequence_number"], "summary sequence_number")
    if value["state"] not in CHECKPOINT_STATES:
        raise AssertionError("the coordinator summary contains an unknown state")
    if value["completed"] != (value["state"] == "complete"):
        raise AssertionError("the coordinator summary state and completion disagree")
    expected = (canonical_json(value) + "\n").encode("utf-8")
    if stdout != expected:
        raise AssertionError("the coordinator summary is not canonical JSON")
    return value


def run_coordinator_success(
    coordinator: Path,
    native: Path,
    input_path: Path,
    checkpoint_directory: Path,
    chunk_size: int,
    max_transactions: int | None,
) -> dict[str, Any]:
    completed = subprocess.run(
        coordinator_command(
            coordinator,
            native,
            input_path,
            checkpoint_directory,
            chunk_size,
            max_transactions,
        ),
        check=False,
        capture_output=True,
        env=coordinator_environment(),
        timeout=COORDINATOR_TIMEOUT_SECONDS,
    )
    if completed.returncode != 0:
        raise AssertionError(
            "the checkpoint coordinator failed unexpectedly: "
            + completed.stderr.decode("utf-8", errors="replace").strip()
        )
    if completed.stderr:
        raise AssertionError("the checkpoint coordinator wrote diagnostics on success")
    return audit_summary(completed.stdout)


def load_canonical_json_file(path: Path) -> tuple[dict[str, Any], bytes]:
    encoded = path.read_bytes()
    value = strict_json_loads(encoded.decode("utf-8"))
    if not isinstance(value, dict):
        raise AssertionError(f"{path.name} is not a JSON object")
    if encoded != (canonical_json(value) + "\n").encode("utf-8"):
        raise AssertionError(f"{path.name} is not canonical JSON")
    return value, encoded


def audit_manifest(
    checkpoint_directory: Path,
    summary: dict[str, Any],
    *,
    expected_chunk_size: int,
    expected_sequence: int,
    expected_run_count: int,
    expected_closed_level_count: int,
    expected_next_emission_index: int,
) -> dict[str, Any]:
    manifest, _ = load_canonical_json_file(checkpoint_directory / "checkpoint.json")
    if frozenset(manifest) != CHECKPOINT_KEYS:
        raise AssertionError("the internal checkpoint manifest is not closed")
    if manifest["kind"] != CHECKPOINT_KIND:
        raise AssertionError("the internal checkpoint kind changed")
    if manifest["schema_version"] != CHECKPOINT_SCHEMA_VERSION:
        raise AssertionError("the internal checkpoint version changed")
    if manifest["state"] not in CHECKPOINT_STATES:
        raise AssertionError("the manifest contains an unknown state")
    sequence = require_plain_integer(manifest["sequence_number"], "sequence_number")
    if sequence != expected_sequence or sequence != summary["sequence_number"]:
        raise AssertionError("the checkpoint and summary sequences disagree")
    if manifest["state"] != summary["state"]:
        raise AssertionError("the checkpoint and summary states disagree")
    if manifest["chunk_size"] != expected_chunk_size:
        raise AssertionError("the checkpoint changed its chunk size")
    if manifest["sort_order_id"] != SORT_ORDER_ID:
        raise AssertionError("the checkpoint changed its exact total order")
    if manifest["public_contract_claimed"] is not False:
        raise AssertionError("the internal checkpoint claimed a public contract")
    if manifest["input_sha256"] != EXPECTED_INPUT_SHA256:
        raise AssertionError("the checkpoint changed its frozen input hash")
    if manifest["emission_count"] != EXPECTED_EMISSION_COUNT:
        raise AssertionError("the checkpoint changed its emission count")
    if manifest["next_emission_index"] != expected_next_emission_index:
        raise AssertionError("the checkpoint input cursor is inconsistent")
    expected_offset = len(encode_corpus(build_corpus()[:expected_next_emission_index]))
    if manifest["next_input_offset_bytes"] != expected_offset:
        raise AssertionError("the checkpoint byte cursor is inconsistent")
    if (
        not isinstance(manifest["runs"], list)
        or len(manifest["runs"]) != expected_run_count
    ):
        raise AssertionError("the checkpoint run inventory is inconsistent")
    if (
        not isinstance(manifest["closed_levels"], list)
        or len(manifest["closed_levels"]) != expected_closed_level_count
    ):
        raise AssertionError("the checkpoint closed-level inventory is inconsistent")
    if manifest["state"] == "collecting_runs":
        if manifest["run_cursors"] != []:
            raise AssertionError("run cursors were published before all runs closed")
        if manifest["run_catalog_sha256"] is not None:
            raise AssertionError("the run catalog was frozen before collection ended")
    else:
        if (
            not isinstance(manifest["run_cursors"], list)
            or len(manifest["run_cursors"]) != expected_run_count
        ):
            raise AssertionError("the merge state does not contain every run cursor")
        value = manifest["run_catalog_sha256"]
        if (
            not isinstance(value, str)
            or len(value) != 64
            or any(character not in "0123456789abcdef" for character in value)
        ):
            raise AssertionError("the frozen run catalog hash is invalid")
    if expected_closed_level_count == 0:
        if manifest["last_fully_closed_level"] is not None:
            raise AssertionError("a last level was published before any level closed")
    elif manifest["last_fully_closed_level"] is None:
        raise AssertionError("the last fully closed level is missing")
    if manifest["state"] == "complete":
        if manifest["result_artifact"] is None:
            raise AssertionError("a complete checkpoint omitted its result artifact")
    elif manifest["result_artifact"] is not None:
        raise AssertionError("an incomplete checkpoint published a result artifact")
    for field in (
        "checkpoint_id",
        "config_sha256",
        "tool_sha256",
        "native_executable_sha256",
    ):
        value = manifest[field]
        if (
            not isinstance(value, str)
            or len(value) != 64
            or any(character not in "0123456789abcdef" for character in value)
        ):
            raise AssertionError(f"{field} is not a lowercase SHA-256 digest")
    return manifest


def audit_result(checkpoint_directory: Path, expected: bytes) -> None:
    manifest, _ = load_canonical_json_file(checkpoint_directory / "checkpoint.json")
    reference = manifest["result_artifact"]
    if not isinstance(reference, dict):
        raise AssertionError("the complete checkpoint omitted its result reference")
    expected_digest = sha256_bytes(expected)
    expected_relative_path = f"results/result-{expected_digest}.json"
    if reference.get("relative_path") != expected_relative_path:
        raise AssertionError("the durable result is not content-addressed")
    if reference.get("sha256") != expected_digest:
        raise AssertionError("the durable result reference has the wrong checksum")
    if reference.get("size_bytes") != len(expected):
        raise AssertionError("the durable result reference has the wrong size")
    durable_path = checkpoint_directory / expected_relative_path
    if durable_path.read_bytes() != expected:
        raise AssertionError("the durable result differs byte-for-byte from the oracle")
    durable_value, durable_bytes = load_canonical_json_file(durable_path)
    if durable_bytes != expected:
        raise AssertionError("the durable result is not the canonical oracle")
    result_path = checkpoint_directory / "result.json"
    if result_path.read_bytes() != expected:
        raise AssertionError("the result alias differs byte-for-byte from the oracle")
    alias_value, alias_bytes = load_canonical_json_file(result_path)
    if alias_bytes != durable_bytes or alias_value != durable_value:
        raise AssertionError("the result alias differs from its durable artifact")
    result_files = [path for path in (checkpoint_directory / "results").iterdir()]
    if result_files != [durable_path]:
        raise AssertionError("the complete checkpoint has unexpected result artifacts")


def exercise_transactional_matrix(
    coordinator: Path,
    native: Path,
    input_path: Path,
    expected: bytes,
    root: Path,
) -> tuple[int, int]:
    invocation_count = 0
    idempotent_resume_count = 0
    for chunk_size, expected_total_runs in RUN_MATRIX:
        actual_total_runs = math.ceil(EXPECTED_EMISSION_COUNT / chunk_size)
        if actual_total_runs != expected_total_runs:
            raise AssertionError(
                "the frozen run matrix no longer forces its run counts"
            )
        directory = root / f"runs-{expected_total_runs}"

        final_sequence = expected_total_runs + EXPECTED_LEVEL_COUNT + 1
        for sequence in range(1, final_sequence + 1):
            summary = run_coordinator_success(
                coordinator, native, input_path, directory, chunk_size, 1
            )
            invocation_count += 1
            if summary["sequence_number"] != sequence:
                raise AssertionError(
                    "one invocation did not commit exactly one transaction"
                )

            if sequence <= expected_total_runs:
                run_count = sequence
                closed_level_count = 0
                next_index = min(sequence * chunk_size, EXPECTED_EMISSION_COUNT)
                expected_state = (
                    "between_levels"
                    if sequence == expected_total_runs
                    else "collecting_runs"
                )
            elif sequence <= expected_total_runs + EXPECTED_LEVEL_COUNT:
                run_count = expected_total_runs
                closed_level_count = sequence - expected_total_runs
                next_index = EXPECTED_EMISSION_COUNT
                expected_state = "between_levels"
            else:
                run_count = expected_total_runs
                closed_level_count = EXPECTED_LEVEL_COUNT
                next_index = EXPECTED_EMISSION_COUNT
                expected_state = "complete"

            if summary["state"] != expected_state:
                raise AssertionError(
                    f"sequence {sequence} entered {summary['state']} instead of "
                    f"{expected_state}"
                )
            manifest = audit_manifest(
                directory,
                summary,
                expected_chunk_size=chunk_size,
                expected_sequence=sequence,
                expected_run_count=run_count,
                expected_closed_level_count=closed_level_count,
                expected_next_emission_index=next_index,
            )
            result_path = directory / "result.json"
            if expected_state == "complete":
                audit_result(directory, expected)
            elif result_path.exists():
                raise AssertionError("a partial transaction published result.json")

            if sequence == expected_total_runs:
                if len(manifest["runs"]) != expected_total_runs:
                    raise AssertionError("merge started before every run was present")

        if expected_total_runs == 31:
            before = (directory / "result.json").read_bytes()
            completed = run_coordinator_success(
                coordinator, native, input_path, directory, chunk_size, 1
            )
            invocation_count += 1
            idempotent_resume_count += 1
            if completed != {
                "completed": True,
                "sequence_number": final_sequence,
                "state": "complete",
            }:
                raise AssertionError(
                    "resuming a complete checkpoint was not idempotent"
                )
            if (directory / "result.json").read_bytes() != before:
                raise AssertionError(
                    "an idempotent resume rewrote the scientific result"
                )
    return invocation_count, idempotent_resume_count


def directory_fingerprint(directory: Path) -> tuple[tuple[str, str], ...]:
    return tuple(
        (
            str(path.relative_to(directory)),
            sha256_bytes(path.read_bytes()),
        )
        for path in sorted(directory.rglob("*"))
        if path.is_file()
    )


def expect_closed_failure(
    coordinator: Path,
    native: Path,
    input_path: Path,
    checkpoint_directory: Path,
    chunk_size: int,
    *,
    preserve_directory: bool = True,
) -> None:
    before = directory_fingerprint(checkpoint_directory)
    completed = subprocess.run(
        coordinator_command(
            coordinator,
            native,
            input_path,
            checkpoint_directory,
            chunk_size,
            None,
        ),
        check=False,
        capture_output=True,
        env=coordinator_environment(),
        timeout=COORDINATOR_TIMEOUT_SECONDS,
    )
    if completed.returncode != 2 or completed.stdout:
        raise AssertionError("an invalid checkpoint did not fail closed with status 2")
    diagnostic = completed.stderr.decode("utf-8", errors="replace")
    if "failed closed:" not in diagnostic:
        raise AssertionError(
            "an invalid checkpoint omitted its closed-failure diagnostic"
        )
    if preserve_directory and directory_fingerprint(checkpoint_directory) != before:
        raise AssertionError("a closed failure mutated the checkpoint directory")
    if (checkpoint_directory / "result.json").exists():
        raise AssertionError("a closed failure published a scientific result")


def run_coordinator_failpoint(
    coordinator: Path,
    native: Path,
    input_path: Path,
    checkpoint_directory: Path,
    chunk_size: int,
    failpoint: str,
    max_transactions: int = 1,
) -> None:
    completed = subprocess.run(
        coordinator_command(
            coordinator,
            native,
            input_path,
            checkpoint_directory,
            chunk_size,
            max_transactions,
        ),
        check=False,
        capture_output=True,
        env=coordinator_environment(failpoint),
        timeout=COORDINATOR_TIMEOUT_SECONDS,
    )
    if completed.returncode != 2 or completed.stdout:
        raise AssertionError(f"failpoint {failpoint} did not fail closed")
    diagnostic = completed.stderr.decode("utf-8", errors="replace")
    if f"injected failure after {failpoint}" not in diagnostic:
        raise AssertionError(f"failpoint {failpoint} omitted its diagnostic")


def require_no_result_alias(checkpoint_directory: Path, label: str) -> None:
    if (checkpoint_directory / "result.json").exists():
        raise AssertionError(f"{label} exposed result.json before the final commit")


def resume_one_run_checkpoint(
    coordinator: Path,
    native: Path,
    input_path: Path,
    checkpoint_directory: Path,
    expected: bytes,
) -> None:
    summary = run_coordinator_success(
        coordinator,
        native,
        input_path,
        checkpoint_directory,
        EXPECTED_EMISSION_COUNT,
        None,
    )
    expected_sequence = 1 + EXPECTED_LEVEL_COUNT + 1
    if summary != {
        "completed": True,
        "sequence_number": expected_sequence,
        "state": "complete",
    }:
        raise AssertionError("a failpoint checkpoint did not resume to completion")
    audit_manifest(
        checkpoint_directory,
        summary,
        expected_chunk_size=EXPECTED_EMISSION_COUNT,
        expected_sequence=expected_sequence,
        expected_run_count=1,
        expected_closed_level_count=EXPECTED_LEVEL_COUNT,
        expected_next_emission_index=EXPECTED_EMISSION_COUNT,
    )
    audit_result(checkpoint_directory, expected)


def exercise_transaction_failpoints(
    coordinator: Path,
    native: Path,
    input_path: Path,
    expected: bytes,
    root: Path,
) -> int:
    root.mkdir()
    chunk_size = EXPECTED_EMISSION_COUNT
    failpoint_count = 0

    zero_bootstrap = root / "zero-transaction-bootstrap"
    zero_summary = run_coordinator_success(
        coordinator, native, input_path, zero_bootstrap, chunk_size, 0
    )
    if zero_summary != {
        "completed": False,
        "sequence_number": 0,
        "state": "collecting_runs",
    }:
        raise AssertionError("a zero-transaction bootstrap advanced the checkpoint")
    audit_manifest(
        zero_bootstrap,
        zero_summary,
        expected_chunk_size=chunk_size,
        expected_sequence=0,
        expected_run_count=0,
        expected_closed_level_count=0,
        expected_next_emission_index=0,
    )
    require_no_result_alias(zero_bootstrap, "zero-transaction bootstrap")
    resume_one_run_checkpoint(coordinator, native, input_path, zero_bootstrap, expected)

    initialization_ready = root / "initialization-ready"
    run_coordinator_failpoint(
        coordinator,
        native,
        input_path,
        initialization_ready,
        chunk_size,
        "initialization_ready",
    )
    if initialization_ready.exists():
        raise AssertionError("initialization_ready published a checkpoint directory")
    if list(root.glob(f".{initialization_ready.name}.initialize.*")):
        raise AssertionError("initialization_ready left a temporary directory")
    resume_one_run_checkpoint(
        coordinator, native, input_path, initialization_ready, expected
    )
    failpoint_count += 1

    initialization_published = root / "initialization-published"
    run_coordinator_failpoint(
        coordinator,
        native,
        input_path,
        initialization_published,
        chunk_size,
        "initialization_published",
    )
    published_manifest, published_bytes = load_canonical_json_file(
        initialization_published / "checkpoint.json"
    )
    if (
        published_manifest["sequence_number"] != 0
        or published_manifest["state"] != "collecting_runs"
        or published_manifest["runs"] != []
        or published_manifest["run_cursors"] != []
        or published_manifest["closed_levels"] != []
    ):
        raise AssertionError("initialization_published exposed transactional progress")
    require_no_result_alias(initialization_published, "initialization_published")

    run_artifact = root / "run-artifact"
    copy_checkpoint(initialization_published, run_artifact)
    resume_one_run_checkpoint(
        coordinator, native, input_path, initialization_published, expected
    )
    failpoint_count += 1

    run_coordinator_failpoint(
        coordinator,
        native,
        input_path,
        run_artifact,
        chunk_size,
        "run_artifact",
    )
    if (run_artifact / "checkpoint.json").read_bytes() != published_bytes:
        raise AssertionError("run_artifact advanced the published manifest")
    after_run_manifest, _ = load_canonical_json_file(run_artifact / "checkpoint.json")
    if (
        after_run_manifest["sequence_number"] != 0
        or after_run_manifest["runs"] != []
        or after_run_manifest["run_cursors"] != []
    ):
        raise AssertionError("run_artifact advanced sequence, runs, or cursors")
    require_no_result_alias(run_artifact, "run_artifact")
    resume_one_run_checkpoint(coordinator, native, input_path, run_artifact, expected)
    failpoint_count += 1

    level_artifact = root / "level-artifact"
    level_summary = run_coordinator_success(
        coordinator, native, input_path, level_artifact, chunk_size, 1
    )
    if level_summary != {
        "completed": False,
        "sequence_number": 1,
        "state": "between_levels",
    }:
        raise AssertionError("the level failpoint base did not commit exactly one run")
    level_manifest, level_manifest_bytes = load_canonical_json_file(
        level_artifact / "checkpoint.json"
    )
    level_cursors = list(level_manifest["run_cursors"])
    run_coordinator_failpoint(
        coordinator,
        native,
        input_path,
        level_artifact,
        chunk_size,
        "level_artifact",
    )
    if (level_artifact / "checkpoint.json").read_bytes() != level_manifest_bytes:
        raise AssertionError("level_artifact advanced the published manifest")
    after_level_manifest, _ = load_canonical_json_file(
        level_artifact / "checkpoint.json"
    )
    if (
        after_level_manifest["sequence_number"] != 1
        or after_level_manifest["run_cursors"] != level_cursors
        or after_level_manifest["closed_levels"] != []
    ):
        raise AssertionError("level_artifact advanced sequence, cursors, or levels")
    require_no_result_alias(level_artifact, "level_artifact")
    resume_one_run_checkpoint(coordinator, native, input_path, level_artifact, expected)
    failpoint_count += 1

    result_artifact = root / "result-artifact"
    result_base = run_coordinator_success(
        coordinator,
        native,
        input_path,
        result_artifact,
        chunk_size,
        1 + EXPECTED_LEVEL_COUNT,
    )
    if result_base != {
        "completed": False,
        "sequence_number": 1 + EXPECTED_LEVEL_COUNT,
        "state": "between_levels",
    }:
        raise AssertionError("the result failpoint base did not close every level")
    result_manifest, result_manifest_bytes = load_canonical_json_file(
        result_artifact / "checkpoint.json"
    )
    result_cursors = list(result_manifest["run_cursors"])
    result_levels = list(result_manifest["closed_levels"])
    run_coordinator_failpoint(
        coordinator,
        native,
        input_path,
        result_artifact,
        chunk_size,
        "result_artifact",
    )
    if (result_artifact / "checkpoint.json").read_bytes() != result_manifest_bytes:
        raise AssertionError("result_artifact advanced the published manifest")
    after_result_manifest, _ = load_canonical_json_file(
        result_artifact / "checkpoint.json"
    )
    if (
        after_result_manifest["sequence_number"] != 1 + EXPECTED_LEVEL_COUNT
        or after_result_manifest["run_cursors"] != result_cursors
        or after_result_manifest["closed_levels"] != result_levels
        or after_result_manifest["result_artifact"] is not None
    ):
        raise AssertionError("result_artifact advanced the final transaction state")
    require_no_result_alias(result_artifact, "result_artifact")
    expected_durable = result_artifact / f"results/result-{sha256_bytes(expected)}.json"
    if expected_durable.read_bytes() != expected:
        raise AssertionError("result_artifact did not durably publish the oracle")
    resume_one_run_checkpoint(
        coordinator, native, input_path, result_artifact, expected
    )
    failpoint_count += 1

    if failpoint_count != EXPECTED_FAILPOINT_COUNT:
        raise AssertionError("the frozen failpoint matrix changed")
    return failpoint_count


def write_manifest(path: Path, value: dict[str, Any]) -> None:
    path.write_text(canonical_json(value) + "\n", encoding="utf-8")


def copy_checkpoint(source: Path, destination: Path) -> None:
    shutil.copytree(source, destination)


def first_run_artifact(directory: Path) -> Path:
    candidates = [
        path
        for path in sorted(directory.rglob("*"))
        if path.is_file()
        and path.name not in {"checkpoint.json", "result.json"}
        and not path.name.endswith(".tmp")
    ]
    if len(candidates) != 1:
        raise AssertionError(
            "a one-transaction checkpoint did not contain exactly one run artifact"
        )
    return candidates[0]


def exercise_closed_failures(
    coordinator: Path,
    native: Path,
    input_path: Path,
    corpus: Sequence[Emission],
    root: Path,
) -> int:
    base = root / "valid-partial"
    summary = run_coordinator_success(coordinator, native, input_path, base, 1, 1)
    if summary["sequence_number"] != 1 or summary["state"] != "collecting_runs":
        raise AssertionError("the corruption base checkpoint is not partial")
    audit_manifest(
        base,
        summary,
        expected_chunk_size=1,
        expected_sequence=1,
        expected_run_count=1,
        expected_closed_level_count=0,
        expected_next_emission_index=1,
    )

    failure_count = 0

    truncated_manifest = root / "truncated-manifest"
    copy_checkpoint(base, truncated_manifest)
    manifest_path = truncated_manifest / "checkpoint.json"
    manifest_path.write_bytes(manifest_path.read_bytes()[:-1])
    expect_closed_failure(coordinator, native, input_path, truncated_manifest, 1)
    failure_count += 1

    future_version = root / "future-version"
    copy_checkpoint(base, future_version)
    manifest, _ = load_canonical_json_file(future_version / "checkpoint.json")
    manifest["schema_version"] = "2.0.0"
    write_manifest(future_version / "checkpoint.json", manifest)
    expect_closed_failure(coordinator, native, input_path, future_version, 1)
    failure_count += 1

    extra_field = root / "extra-field"
    copy_checkpoint(base, extra_field)
    manifest, _ = load_canonical_json_file(extra_field / "checkpoint.json")
    manifest["unexpected"] = True
    write_manifest(extra_field / "checkpoint.json", manifest)
    expect_closed_failure(coordinator, native, input_path, extra_field, 1)
    failure_count += 1

    missing_field = root / "missing-field"
    copy_checkpoint(base, missing_field)
    manifest, _ = load_canonical_json_file(missing_field / "checkpoint.json")
    del manifest["state"]
    write_manifest(missing_field / "checkpoint.json", manifest)
    expect_closed_failure(coordinator, native, input_path, missing_field, 1)
    failure_count += 1

    wrong_configuration = root / "wrong-configuration"
    copy_checkpoint(base, wrong_configuration)
    expect_closed_failure(coordinator, native, input_path, wrong_configuration, 2)
    failure_count += 1

    wrong_input = root / "wrong-input"
    copy_checkpoint(base, wrong_input)
    changed_input = root / "changed-input.jsonl"
    changed_records = list(corpus)
    changed_records[-1] = make_emission(
        changed_records[-1].level,
        changed_records[-1].minimal_support_ids,
        (
            (*changed_records[-1].source_support_ids, MAX_SAFE_JSON_INTEGER)
            if len(changed_records[-1].source_support_ids) < 4
            else changed_records[-1].source_support_ids
        ),
    )
    changed_input.write_bytes(encode_corpus(changed_records))
    if changed_input.read_bytes() == input_path.read_bytes():
        raise AssertionError("the input-mismatch fixture did not change the input")
    expect_closed_failure(coordinator, native, changed_input, wrong_input, 1)
    failure_count += 1

    truncated_run = root / "truncated-run"
    copy_checkpoint(base, truncated_run)
    run_path = first_run_artifact(truncated_run)
    run_path.write_bytes(run_path.read_bytes()[:-1])
    expect_closed_failure(coordinator, native, input_path, truncated_run, 1)
    failure_count += 1

    corrupted_run = root / "corrupted-run"
    copy_checkpoint(base, corrupted_run)
    run_path = first_run_artifact(corrupted_run)
    run, _ = load_canonical_json_file(run_path)
    source_digest = run["source_sha256"]
    run["source_sha256"] = ("0" if source_digest[0] != "0" else "1") + source_digest[1:]
    write_manifest(run_path, run)
    expect_closed_failure(coordinator, native, input_path, corrupted_run, 1)
    failure_count += 1

    contradiction = root / "contradiction"
    contradiction_input = root / "contradiction.jsonl"
    contradiction_input.write_bytes(
        encode_corpus(
            (
                make_emission(Fraction(), (0,)),
                make_emission(Fraction(1), (0,)),
            )
        )
    )
    completed = subprocess.run(
        coordinator_command(
            coordinator,
            native,
            contradiction_input,
            contradiction,
            1,
            None,
        ),
        check=False,
        capture_output=True,
        env=coordinator_environment(),
        timeout=COORDINATOR_TIMEOUT_SECONDS,
    )
    if completed.returncode != 2 or completed.stdout:
        raise AssertionError("an inter-run support contradiction did not fail closed")
    if "failed closed:" not in completed.stderr.decode("utf-8", errors="replace"):
        raise AssertionError("the support contradiction omitted its diagnostic")
    if (contradiction / "result.json").exists():
        raise AssertionError("the support contradiction published a result")
    before_retry = directory_fingerprint(contradiction)
    retry = subprocess.run(
        coordinator_command(
            coordinator,
            native,
            contradiction_input,
            contradiction,
            1,
            None,
        ),
        check=False,
        capture_output=True,
        env=coordinator_environment(),
        timeout=COORDINATOR_TIMEOUT_SECONDS,
    )
    if retry.returncode != 2 or retry.stdout:
        raise AssertionError("the support contradiction was not reproducible on resume")
    if directory_fingerprint(contradiction) != before_retry:
        raise AssertionError("retrying a contradiction advanced its checkpoint")
    failure_count += 1

    return failure_count


def audit_frozen_corpus(
    emissions: Sequence[Emission], corpus_bytes: bytes, oracle_bytes: bytes
) -> None:
    if len(emissions) != EXPECTED_EMISSION_COUNT:
        raise AssertionError("the frozen checkpoint corpus changed size")
    oracle = strict_json_loads(oracle_bytes.decode("utf-8"))
    if oracle["unique_emission_count"] != EXPECTED_UNIQUE_EMISSION_COUNT:
        raise AssertionError("the frozen checkpoint unique count changed")
    if oracle["duplicate_emission_count"] != EXPECTED_DUPLICATE_EMISSION_COUNT:
        raise AssertionError("the frozen checkpoint duplicate count changed")
    batches = oracle["equal_level_batches"]
    if len(batches) != EXPECTED_LEVEL_COUNT:
        raise AssertionError("the frozen checkpoint level count changed")
    unit_batch = next(
        batch
        for batch in batches
        if batch["squared_level_exact"] == level_record(Fraction(1))
    )
    if (
        unit_batch["emission_count"] != EXPECTED_UNIT_LEVEL_EMISSION_COUNT
        or len(unit_batch["supports"]) != EXPECTED_UNIT_LEVEL_SUPPORT_COUNT
    ):
        raise AssertionError("the frozen cross-run unit-level batch changed")

    magnitude = 1 << 128
    lower = Fraction(magnitude - 1, magnitude)
    upper = Fraction(magnitude, magnitude + 1)
    if lower.numerator * upper.denominator - upper.numerator * lower.denominator != -1:
        raise AssertionError("the adjacent-level unit cross product changed")
    observed_levels = {
        Fraction(
            int(batch["squared_level_exact"]["numerator"]),
            int(batch["squared_level_exact"]["denominator"]),
        )
        for batch in batches
    }
    if lower not in observed_levels or upper not in observed_levels:
        raise AssertionError("the adjacent exact levels were merged by the oracle")

    if len(corpus_bytes) != EXPECTED_INPUT_BYTE_COUNT:
        raise AssertionError("the frozen checkpoint input byte count changed")
    if len(oracle_bytes) != EXPECTED_ORACLE_BYTE_COUNT:
        raise AssertionError("the frozen checkpoint oracle byte count changed")
    if sha256_bytes(corpus_bytes) != EXPECTED_INPUT_SHA256:
        raise AssertionError("the frozen checkpoint input hash changed")
    if sha256_bytes(oracle_bytes) != EXPECTED_ORACLE_SHA256:
        raise AssertionError("the frozen checkpoint oracle hash changed")

    for chunk_size, expected_runs in RUN_MATRIX:
        chunks = [
            emissions[index : index + chunk_size]
            for index in range(0, len(emissions), chunk_size)
        ]
        if len(chunks) != expected_runs:
            raise AssertionError("the run-count matrix changed")
        if expected_runs > 1:
            unit_chunks = {
                index
                for index, chunk in enumerate(chunks)
                if any(emission.level == Fraction(1) for emission in chunk)
            }
            if len(unit_chunks) < 2:
                raise AssertionError("the unit level no longer crosses run boundaries")
    size_five_chunks = [
        emissions[index : index + 5] for index in range(0, len(emissions), 5)
    ]
    adjacent_chunks = {
        index
        for index, chunk in enumerate(size_five_chunks)
        for emission in chunk
        if emission.level in {lower, upper}
    }
    if len(adjacent_chunks) != 2:
        raise AssertionError("the adjacent exact levels no longer cross two runs")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("coordinator", type=Path)
    parser.add_argument("native_replay", type=Path)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    coordinator = arguments.coordinator.resolve(strict=True)
    native = arguments.native_replay.resolve(strict=True)

    emissions = build_corpus()
    corpus_bytes = encode_corpus(emissions)
    oracle_value = fraction_oracle(emissions)
    oracle_bytes = (canonical_json(oracle_value) + "\n").encode("utf-8")
    audit_frozen_corpus(emissions, corpus_bytes, oracle_bytes)

    native_bytes = run_native_one_shot(native, emissions)
    if native_bytes != oracle_bytes:
        raise AssertionError(
            "the native monolithic level result differs from the Fraction oracle"
        )

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-level-checkpoint-") as name:
        root = Path(name)
        input_path = root / "input.jsonl"
        input_path.write_bytes(corpus_bytes)

        unlimited = root / "unlimited"
        summary = run_coordinator_success(
            coordinator, native, input_path, unlimited, 31, None
        )
        expected_unlimited_sequence = 1 + EXPECTED_LEVEL_COUNT + 1
        if summary != {
            "completed": True,
            "sequence_number": expected_unlimited_sequence,
            "state": "complete",
        }:
            raise AssertionError("an unlimited coordinator run did not complete")
        audit_manifest(
            unlimited,
            summary,
            expected_chunk_size=31,
            expected_sequence=expected_unlimited_sequence,
            expected_run_count=1,
            expected_closed_level_count=EXPECTED_LEVEL_COUNT,
            expected_next_emission_index=EXPECTED_EMISSION_COUNT,
        )
        audit_result(unlimited, oracle_bytes)

        invocation_count, idempotent_resume_count = exercise_transactional_matrix(
            coordinator, native, input_path, oracle_bytes, root / "matrix"
        )
        data_failure_count = exercise_closed_failures(
            coordinator, native, input_path, emissions, root / "failures"
        )
        failpoint_count = exercise_transaction_failpoints(
            coordinator,
            native,
            input_path,
            oracle_bytes,
            root / "failpoints",
        )
        closed_failure_count = data_failure_count + failpoint_count
        if closed_failure_count != EXPECTED_CLOSED_FAILURE_COUNT:
            raise AssertionError("the frozen closed-failure matrix changed")

    print(
        canonical_json(
            {
                "adjacent_cross_product": "-1",
                "closed_failure_count": closed_failure_count,
                "duplicate_emission_count": EXPECTED_DUPLICATE_EMISSION_COUNT,
                "emission_count": EXPECTED_EMISSION_COUNT,
                "fraction_oracle_sha256": EXPECTED_ORACLE_SHA256,
                "idempotent_resume_count": idempotent_resume_count,
                "input_sha256": EXPECTED_INPUT_SHA256,
                "level_count": EXPECTED_LEVEL_COUNT,
                "native_one_shot_byte_identical": True,
                "run_counts": [run_count for _, run_count in RUN_MATRIX],
                "transaction_failpoint_count": failpoint_count,
                "transactional_invocation_count": invocation_count,
                "unique_emission_count": EXPECTED_UNIQUE_EMISSION_COUNT,
                "zero_transaction_bootstrap": True,
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
