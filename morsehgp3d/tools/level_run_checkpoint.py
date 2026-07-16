#!/usr/bin/env python3
"""Checkpoint and merge bounded exact-level runs for MorseHGP3D phase 2A."""

from __future__ import annotations

import argparse
import contextlib
import fcntl
import hashlib
import json
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
from fractions import Fraction
from pathlib import Path
from typing import Any, Sequence

if hasattr(sys, "set_int_max_str_digits"):
    sys.set_int_max_str_digits(0)

SCHEMA_VERSION = "1.0.0"
EMISSION_KIND = "morsehgp3d_phase2a_level_emission"
RUN_KIND = "morsehgp3d_phase2a_sorted_level_run"
LEVEL_KIND = "morsehgp3d_phase2a_closed_equal_level"
CHECKPOINT_KIND = "morsehgp3d_phase2a_level_checkpoint"
SORT_ORDER_ID = "exact-level-support-source-v1"
RESULT_PREDICATE = "canonical_level_batches"
MANIFEST_NAME = "checkpoint.json"
RESULT_NAME = "result.json"
RESULTS_DIRECTORY = "results"
MAX_CHUNK_SIZE = 64
MAX_POINT_ID = (1 << 53) - 1
HASH_PATTERN = re.compile(r"^[0-9a-f]{64}$")
NONNEGATIVE_INTEGER_PATTERN = re.compile(r"^(0|[1-9][0-9]*)$")
POSITIVE_INTEGER_PATTERN = re.compile(r"^[1-9][0-9]*$")
FAILPOINT_ENVIRONMENT = "MORSEHGP3D_PHASE2A_CHECKPOINT_FAIL_AFTER"

CHECKPOINT_FIELDS = {
    "checkpoint_id",
    "chunk_size",
    "closed_levels",
    "config_sha256",
    "emission_count",
    "input_sha256",
    "kind",
    "last_fully_closed_level",
    "native_executable_sha256",
    "next_emission_index",
    "next_input_offset_bytes",
    "public_contract_claimed",
    "result_artifact",
    "run_catalog_sha256",
    "run_cursors",
    "runs",
    "schema_version",
    "sequence_number",
    "sort_order_id",
    "state",
    "tool_sha256",
}
RUN_REFERENCE_FIELDS = {
    "relative_path",
    "run_index",
    "sha256",
    "size_bytes",
    "source_begin",
    "source_begin_offset_bytes",
    "source_end",
    "source_end_offset_bytes",
}
RUN_FIELDS = {
    "kind",
    "payload",
    "run_index",
    "schema_version",
    "source_begin",
    "source_begin_offset_bytes",
    "source_end",
    "source_end_offset_bytes",
    "source_sha256",
}
LEVEL_REFERENCE_FIELDS = {
    "level_index",
    "relative_path",
    "sha256",
    "size_bytes",
    "squared_level_exact",
}
LEVEL_FIELDS = {
    "equal_level_batch",
    "kind",
    "level_index",
    "schema_version",
}
RESULT_REFERENCE_FIELDS = {
    "duplicate_emission_count",
    "emission_count",
    "equal_level_batch_count",
    "relative_path",
    "sha256",
    "size_bytes",
    "unique_emission_count",
}


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def strict_json_loads(text: str) -> Any:
    def reject_duplicates(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate JSON key: {key}")
            result[key] = value
        return result

    def reject_nonfinite(value: str) -> None:
        raise ValueError(f"non-finite JSON number: {value}")

    return json.loads(
        text,
        object_pairs_hook=reject_duplicates,
        parse_constant=reject_nonfinite,
    )


def require_object_fields(value: Any, fields: set[str], label: str) -> dict[str, Any]:
    if not isinstance(value, dict) or set(value) != fields:
        raise ValueError(f"{label} has missing or unknown fields")
    return value


def require_integer(
    value: Any,
    label: str,
    *,
    minimum: int = 0,
    maximum: int | None = None,
) -> int:
    if type(value) is not int or value < minimum:
        raise ValueError(f"{label} is not a valid integer")
    if maximum is not None and value > maximum:
        raise ValueError(f"{label} exceeds its closed domain")
    return value


def require_hash(value: Any, label: str) -> str:
    if not isinstance(value, str) or HASH_PATTERN.fullmatch(value) is None:
        raise ValueError(f"{label} is not a canonical SHA-256 digest")
    return value


def exact_level(value: Any, label: str) -> tuple[Fraction, dict[str, str]]:
    record = require_object_fields(
        value,
        {"denominator", "numerator", "schema_version", "unit"},
        label,
    )
    if (
        record["schema_version"] != "2.0.0"
        or record["unit"] != "input_coordinate_unit_squared"
    ):
        raise ValueError(f"{label} changed the closed ExactLevel contract")
    numerator_text = record["numerator"]
    denominator_text = record["denominator"]
    if (
        not isinstance(numerator_text, str)
        or NONNEGATIVE_INTEGER_PATTERN.fullmatch(numerator_text) is None
        or not isinstance(denominator_text, str)
        or POSITIVE_INTEGER_PATTERN.fullmatch(denominator_text) is None
    ):
        raise ValueError(f"{label} contains a noncanonical integer")
    numerator = int(numerator_text)
    denominator = int(denominator_text)
    if math.gcd(numerator, denominator) != 1:
        raise ValueError(f"{label} is not reduced")
    canonical = {
        "denominator": denominator_text,
        "numerator": numerator_text,
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }
    return Fraction(numerator, denominator), canonical


def level_record(value: Fraction) -> dict[str, str]:
    if value < 0:
        raise ValueError("an exact squared level cannot be negative")
    return {
        "denominator": str(value.denominator),
        "numerator": str(value.numerator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }


def canonical_ids(value: Any, label: str) -> tuple[int, ...]:
    if not isinstance(value, list) or not 1 <= len(value) <= 4:
        raise ValueError(f"{label} must contain one to four identifiers")
    identifiers = tuple(
        require_integer(
            identifier,
            f"{label} identifier",
            maximum=MAX_POINT_ID,
        )
        for identifier in value
    )
    if tuple(sorted(identifiers)) != identifiers or len(set(identifiers)) != len(
        identifiers
    ):
        raise ValueError(f"{label} is not strictly sorted and unique")
    return identifiers


def validate_emission(value: Any, label: str) -> dict[str, Any]:
    emission = require_object_fields(
        value,
        {
            "kind",
            "minimal_support_ids",
            "schema_version",
            "source_support_ids",
            "squared_level_exact",
        },
        label,
    )
    if (
        emission["kind"] != EMISSION_KIND
        or emission["schema_version"] != SCHEMA_VERSION
    ):
        raise ValueError(f"{label} changed the internal emission schema")
    minimal = canonical_ids(emission["minimal_support_ids"], f"{label} minimal support")
    source = canonical_ids(emission["source_support_ids"], f"{label} source support")
    if not set(minimal).issubset(source):
        raise ValueError(f"{label} minimal support is absent from its source support")
    _, level = exact_level(emission["squared_level_exact"], f"{label} squared level")
    canonical = {
        "kind": EMISSION_KIND,
        "minimal_support_ids": list(minimal),
        "schema_version": SCHEMA_VERSION,
        "source_support_ids": list(source),
        "squared_level_exact": level,
    }
    if emission != canonical:
        raise ValueError(f"{label} is not canonical")
    return canonical


def validate_source_provenance(value: Any, label: str) -> tuple[tuple[int, ...], int]:
    provenance = require_object_fields(
        value, {"emission_count", "source_support_ids"}, label
    )
    count = require_integer(provenance["emission_count"], f"{label} count", minimum=1)
    source = canonical_ids(provenance["source_support_ids"], f"{label} support")
    return source, count


def validate_equal_level_batch(value: Any, label: str) -> tuple[Fraction, int, int]:
    batch = require_object_fields(
        value, {"emission_count", "squared_level_exact", "supports"}, label
    )
    batch_count = require_integer(batch["emission_count"], f"{label} count", minimum=1)
    level, canonical_level = exact_level(
        batch["squared_level_exact"], f"{label} squared level"
    )
    if batch["squared_level_exact"] != canonical_level:
        raise ValueError(f"{label} squared level is not canonical")
    supports = batch["supports"]
    if not isinstance(supports, list) or not supports:
        raise ValueError(f"{label} must contain supports")
    observed_count = 0
    unique_count = 0
    previous_support: tuple[int, tuple[int, ...]] | None = None
    for support_index, raw_support in enumerate(supports):
        support_label = f"{label} support {support_index}"
        support = require_object_fields(
            raw_support,
            {
                "emission_count",
                "minimal_support_ids",
                "source_provenance",
                "squared_level_exact",
            },
            support_label,
        )
        support_count = require_integer(
            support["emission_count"], f"{support_label} count", minimum=1
        )
        support_level, support_level_record = exact_level(
            support["squared_level_exact"], f"{support_label} squared level"
        )
        if (
            support_level != level
            or support["squared_level_exact"] != support_level_record
        ):
            raise ValueError(f"{support_label} differs from its enclosing level")
        minimal = canonical_ids(
            support["minimal_support_ids"], f"{support_label} minimal support"
        )
        support_key = (len(minimal), minimal)
        if previous_support is not None and support_key <= previous_support:
            raise ValueError(f"{label} supports are not canonically ordered")
        previous_support = support_key
        sources = support["source_provenance"]
        if not isinstance(sources, list) or not sources:
            raise ValueError(f"{support_label} has no source provenance")
        observed_support_count = 0
        previous_source: tuple[int, tuple[int, ...]] | None = None
        for source_index, raw_source in enumerate(sources):
            source, source_count = validate_source_provenance(
                raw_source, f"{support_label} source {source_index}"
            )
            if not set(minimal).issubset(source):
                raise ValueError(
                    f"{support_label} provenance omits its minimal support"
                )
            source_key = (len(source), source)
            if previous_source is not None and source_key <= previous_source:
                raise ValueError(
                    f"{support_label} provenance is not canonically ordered"
                )
            previous_source = source_key
            observed_support_count += source_count
            unique_count += 1
        if observed_support_count != support_count:
            raise ValueError(f"{support_label} counters are inconsistent")
        observed_count += support_count
    if observed_count != batch_count:
        raise ValueError(f"{label} counters are inconsistent")
    return level, batch_count, unique_count


def validate_native_payload(value: Any, label: str) -> dict[str, Any]:
    payload = require_object_fields(
        value,
        {
            "duplicate_emission_count",
            "emission_count",
            "equal_level_batches",
            "predicate",
            "unique_emission_count",
        },
        label,
    )
    if payload["predicate"] != RESULT_PREDICATE:
        raise ValueError(f"{label} changed predicate identity")
    emission_count = require_integer(
        payload["emission_count"], f"{label} emission count", minimum=1
    )
    unique_count = require_integer(
        payload["unique_emission_count"], f"{label} unique count", minimum=1
    )
    duplicate_count = require_integer(
        payload["duplicate_emission_count"], f"{label} duplicate count"
    )
    batches = payload["equal_level_batches"]
    if not isinstance(batches, list) or not batches:
        raise ValueError(f"{label} has no equal-level batches")
    observed_emissions = 0
    observed_unique = 0
    previous_level: Fraction | None = None
    for batch_index, batch in enumerate(batches):
        level, count, batch_unique = validate_equal_level_batch(
            batch, f"{label} batch {batch_index}"
        )
        if previous_level is not None and level <= previous_level:
            raise ValueError(f"{label} levels are not strictly ordered")
        previous_level = level
        observed_emissions += count
        observed_unique += batch_unique
    if (
        observed_emissions != emission_count
        or observed_unique != unique_count
        or duplicate_count != emission_count - unique_count
    ):
        raise ValueError(f"{label} global counters are inconsistent")
    return payload


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while block := stream.read(1024 * 1024):
            digest.update(block)
    return digest.hexdigest()


def canonical_hash(value: object) -> str:
    return sha256_bytes(canonical_json(value).encode("utf-8"))


def require_regular_file(path: Path, label: str) -> None:
    if path.is_symlink() or not path.is_file():
        raise ValueError(f"{label} is not a regular file")


def fsync_directory(path: Path) -> None:
    flags = os.O_RDONLY | getattr(os, "O_DIRECTORY", 0)
    descriptor = os.open(path, flags)
    try:
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def maybe_fail_after(label: str) -> None:
    if os.environ.get(FAILPOINT_ENVIRONMENT) == label:
        raise RuntimeError(f"injected failure after {label}")


def atomic_publish(path: Path, content: bytes, failpoint: str | None = None) -> None:
    descriptor, temporary_name = tempfile.mkstemp(
        dir=path.parent,
        prefix=f".{path.name}.",
        suffix=".tmp",
    )
    temporary = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(content)
            stream.flush()
            os.fsync(stream.fileno())
        if sha256_file(temporary) != sha256_bytes(content):
            raise OSError(f"temporary artifact verification failed for {path.name}")
        os.replace(temporary, path)
        fsync_directory(path.parent)
    finally:
        if temporary.exists():
            temporary.unlink()
    if failpoint is not None:
        maybe_fail_after(failpoint)


def canonical_file_bytes(value: object) -> bytes:
    return (canonical_json(value) + "\n").encode("utf-8")


def load_canonical_file(path: Path, label: str) -> tuple[Any, bytes]:
    require_regular_file(path, label)
    content = path.read_bytes()
    try:
        text = content.decode("utf-8")
    except UnicodeDecodeError as error:
        raise ValueError(f"{label} is not UTF-8") from error
    value = strict_json_loads(text)
    if content != canonical_file_bytes(value):
        raise ValueError(f"{label} is not canonical JSON")
    return value, content


def input_identity(path: Path) -> tuple[str, int, int]:
    require_regular_file(path, "phase 2A level input")
    digest = hashlib.sha256()
    record_count = 0
    size_bytes = 0
    with path.open("rb") as stream:
        for raw_line in stream:
            digest.update(raw_line)
            size_bytes += len(raw_line)
            if not raw_line.endswith(b"\n"):
                raise ValueError("the level input must end every record with a newline")
            if raw_line == b"\n":
                raise ValueError("the level input contains an empty record")
            record_count += 1
    if record_count == 0:
        raise ValueError("the level input must contain at least one emission")
    return digest.hexdigest(), record_count, size_bytes


def read_input_slice(
    path: Path, begin_offset: int, end_offset: int, expected_records: int
) -> bytes:
    if end_offset < begin_offset:
        raise ValueError("an input slice has reversed offsets")
    with path.open("rb") as stream:
        stream.seek(begin_offset)
        content = stream.read(end_offset - begin_offset)
    if len(content) != end_offset - begin_offset:
        raise ValueError("an input slice exceeds the source file")
    if content.count(b"\n") != expected_records or (
        content and not content.endswith(b"\n")
    ):
        raise ValueError("an input slice does not align with record boundaries")
    return content


def read_emission_chunk(
    path: Path,
    begin_index: int,
    begin_offset: int,
    chunk_size: int,
) -> tuple[list[dict[str, Any]], int, bytes]:
    emissions: list[dict[str, Any]] = []
    raw_lines: list[bytes] = []
    with path.open("rb") as stream:
        stream.seek(begin_offset)
        for local_index in range(chunk_size):
            raw_line = stream.readline()
            if not raw_line:
                break
            if not raw_line.endswith(b"\n"):
                raise ValueError("the level input contains an unterminated record")
            try:
                text = raw_line.decode("utf-8")
            except UnicodeDecodeError as error:
                raise ValueError("a level input record is not UTF-8") from error
            value = strict_json_loads(text)
            if raw_line != canonical_file_bytes(value):
                raise ValueError(
                    f"level input record {begin_index + local_index} is not canonical JSON"
                )
            emissions.append(
                validate_emission(
                    value, f"level input record {begin_index + local_index}"
                )
            )
            raw_lines.append(raw_line)
        end_offset = stream.tell()
    if not emissions:
        raise ValueError("the checkpoint cursor did not reach another emission")
    return emissions, end_offset, b"".join(raw_lines)


def native_command(emissions: Sequence[dict[str, Any]]) -> str:
    tokens = [RESULT_PREDICATE, str(len(emissions))]
    for emission in emissions:
        level = emission["squared_level_exact"]
        minimal = emission["minimal_support_ids"]
        source = emission["source_support_ids"]
        tokens.extend(
            [
                level["numerator"],
                level["denominator"],
                str(len(minimal)),
                *(str(identifier) for identifier in minimal),
                str(len(source)),
                *(str(identifier) for identifier in source),
            ]
        )
    return " ".join(tokens)


def invoke_native(
    executable: Path,
    emissions: Sequence[dict[str, Any]],
    timeout_seconds: int,
) -> dict[str, Any]:
    completed = subprocess.run(
        [str(executable), "--batch"],
        input=native_command(emissions) + "\n",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        encoding="utf-8",
        timeout=timeout_seconds,
    )
    if completed.returncode != 0 or completed.stderr:
        raise RuntimeError(
            "the native canonical level run failed closed: "
            f"returncode={completed.returncode}, stderr={completed.stderr!r}"
        )
    value = strict_json_loads(completed.stdout)
    if completed.stdout != canonical_json(value) + "\n":
        raise ValueError("the native canonical level run is not canonical JSON")
    return validate_native_payload(value, "native canonical level run")


def run_relative_path(run_index: int) -> str:
    return f"runs/run-{run_index:08d}.json"


def level_relative_path(level_index: int) -> str:
    return f"levels/level-{level_index:08d}.json"


def artifact_reference(
    relative_path: str,
    content: bytes,
    **fields: Any,
) -> dict[str, Any]:
    return {
        **fields,
        "relative_path": relative_path,
        "sha256": sha256_bytes(content),
        "size_bytes": len(content),
    }


def publish_manifest(checkpoint_dir: Path, manifest: dict[str, Any]) -> None:
    atomic_publish(checkpoint_dir / MANIFEST_NAME, canonical_file_bytes(manifest))


def result_from_batches(batches: Sequence[dict[str, Any]]) -> dict[str, Any]:
    emission_count = 0
    unique_count = 0
    previous_level: Fraction | None = None
    for batch_index, batch in enumerate(batches):
        level, count, batch_unique = validate_equal_level_batch(
            batch, f"final equal-level batch {batch_index}"
        )
        if previous_level is not None and level <= previous_level:
            raise ValueError("final equal-level batches are not strictly ordered")
        previous_level = level
        emission_count += count
        unique_count += batch_unique
    if not batches or emission_count < unique_count:
        raise ValueError("the final equal-level result is empty or inconsistent")
    return {
        "duplicate_emission_count": emission_count - unique_count,
        "emission_count": emission_count,
        "equal_level_batches": list(batches),
        "predicate": RESULT_PREDICATE,
        "unique_emission_count": unique_count,
    }


def merge_equal_level_batches(
    batches: Sequence[dict[str, Any]], level: Fraction
) -> dict[str, Any]:
    grouped: dict[tuple[int, ...], dict[tuple[int, ...], int]] = {}
    for batch_index, batch in enumerate(batches):
        observed_level, _, _ = validate_equal_level_batch(
            batch, f"merge input batch {batch_index}"
        )
        if observed_level != level:
            raise ValueError("a merge input batch has the wrong exact level")
        for support in batch["supports"]:
            minimal = tuple(support["minimal_support_ids"])
            sources = grouped.setdefault(minimal, {})
            for provenance in support["source_provenance"]:
                source = tuple(provenance["source_support_ids"])
                sources[source] = sources.get(source, 0) + provenance["emission_count"]
    supports_output: list[dict[str, Any]] = []
    batch_count = 0
    for minimal in sorted(
        grouped, key=lambda identifiers: (len(identifiers), identifiers)
    ):
        sources = grouped[minimal]
        support_count = sum(sources.values())
        batch_count += support_count
        supports_output.append(
            {
                "emission_count": support_count,
                "minimal_support_ids": list(minimal),
                "source_provenance": [
                    {
                        "emission_count": sources[source],
                        "source_support_ids": list(source),
                    }
                    for source in sorted(
                        sources, key=lambda identifiers: (len(identifiers), identifiers)
                    )
                ],
                "squared_level_exact": level_record(level),
            }
        )
    merged = {
        "emission_count": batch_count,
        "squared_level_exact": level_record(level),
        "supports": supports_output,
    }
    validate_equal_level_batch(merged, "merged equal-level batch")
    return merged


def validate_reference_file(
    checkpoint_dir: Path,
    reference: dict[str, Any],
    expected_relative_path: str,
    label: str,
) -> tuple[Any, bytes]:
    if reference["relative_path"] != expected_relative_path:
        raise ValueError(f"{label} path is not canonical")
    size_bytes = require_integer(reference["size_bytes"], f"{label} size", minimum=1)
    digest = require_hash(reference["sha256"], f"{label} digest")
    path = checkpoint_dir / expected_relative_path
    value, content = load_canonical_file(path, label)
    if len(content) != size_bytes or sha256_bytes(content) != digest:
        raise ValueError(f"{label} size or checksum changed")
    return value, content


def validate_run_reference(
    checkpoint_dir: Path,
    input_path: Path,
    reference: Any,
    expected_index: int,
    expected_begin: int,
    expected_begin_offset: int,
    chunk_size: int,
    total_emissions: int,
) -> tuple[int, int, int]:
    run_reference = require_object_fields(
        reference, RUN_REFERENCE_FIELDS, f"run reference {expected_index}"
    )
    if require_integer(run_reference["run_index"], "run index") != expected_index:
        raise ValueError("run references are not sequential")
    begin = require_integer(run_reference["source_begin"], "run source begin")
    end = require_integer(run_reference["source_end"], "run source end", minimum=1)
    begin_offset = require_integer(
        run_reference["source_begin_offset_bytes"], "run source begin offset"
    )
    end_offset = require_integer(
        run_reference["source_end_offset_bytes"], "run source end offset", minimum=1
    )
    if begin != expected_begin or begin_offset != expected_begin_offset or end <= begin:
        raise ValueError("run source ranges are not contiguous")
    if end - begin > chunk_size or (
        end < total_emissions and end - begin != chunk_size
    ):
        raise ValueError("a run violates the configured chunk bound")
    run_value, _ = validate_reference_file(
        checkpoint_dir,
        run_reference,
        run_relative_path(expected_index),
        f"run artifact {expected_index}",
    )
    run = require_object_fields(run_value, RUN_FIELDS, f"run artifact {expected_index}")
    if (
        run["kind"] != RUN_KIND
        or run["schema_version"] != SCHEMA_VERSION
        or require_integer(run["run_index"], "run artifact index") != expected_index
        or require_integer(run["source_begin"], "run artifact source begin") != begin
        or require_integer(run["source_end"], "run artifact source end") != end
        or require_integer(
            run["source_begin_offset_bytes"], "run artifact source begin offset"
        )
        != begin_offset
        or require_integer(
            run["source_end_offset_bytes"], "run artifact source end offset"
        )
        != end_offset
    ):
        raise ValueError(f"run artifact {expected_index} metadata changed")
    source_content = read_input_slice(input_path, begin_offset, end_offset, end - begin)
    if require_hash(run["source_sha256"], "run source digest") != sha256_bytes(
        source_content
    ):
        raise ValueError(
            f"run artifact {expected_index} no longer matches its input slice"
        )
    payload = validate_native_payload(
        run["payload"], f"run artifact {expected_index} payload"
    )
    if payload["emission_count"] != end - begin:
        raise ValueError(f"run artifact {expected_index} changed its emission count")
    return end, end_offset, len(payload["equal_level_batches"])


def load_run_payload(
    checkpoint_dir: Path, reference: dict[str, Any], run_index: int
) -> dict[str, Any]:
    value, _ = validate_reference_file(
        checkpoint_dir,
        reference,
        run_relative_path(run_index),
        f"run artifact {run_index}",
    )
    run = require_object_fields(value, RUN_FIELDS, f"run artifact {run_index}")
    return validate_native_payload(run["payload"], f"run artifact {run_index} payload")


def next_merged_level(
    checkpoint_dir: Path,
    runs: Sequence[dict[str, Any]],
    cursors: Sequence[int],
) -> tuple[dict[str, Any], list[int]] | None:
    heads: list[tuple[Fraction, int]] = []
    payloads: dict[int, dict[str, Any]] = {}
    for run_index, (reference, cursor) in enumerate(zip(runs, cursors)):
        payload = load_run_payload(checkpoint_dir, reference, run_index)
        batches = payload["equal_level_batches"]
        if cursor < len(batches):
            level, _, _ = validate_equal_level_batch(
                batches[cursor], f"run {run_index} merge head"
            )
            heads.append((level, run_index))
            payloads[run_index] = payload
    if not heads:
        return None
    level = min(head[0] for head in heads)
    selected: list[dict[str, Any]] = []
    next_cursors = list(cursors)
    for head_level, run_index in heads:
        if head_level == level:
            selected.append(
                payloads[run_index]["equal_level_batches"][cursors[run_index]]
            )
            next_cursors[run_index] += 1
    return merge_equal_level_batches(selected, level), next_cursors


def validate_global_support_levels(
    checkpoint_dir: Path, runs: Sequence[dict[str, Any]]
) -> None:
    levels: dict[tuple[int, ...], Fraction] = {}
    for run_index, reference in enumerate(runs):
        payload = load_run_payload(checkpoint_dir, reference, run_index)
        for batch in payload["equal_level_batches"]:
            level, _, _ = validate_equal_level_batch(batch, f"run {run_index} batch")
            for support in batch["supports"]:
                minimal = tuple(support["minimal_support_ids"])
                previous = levels.setdefault(minimal, level)
                if previous != level:
                    raise ValueError(
                        "one canonical minimal support has different levels across runs"
                    )


def validate_level_reference(
    checkpoint_dir: Path,
    reference: Any,
    expected_index: int,
    expected_batch: dict[str, Any],
) -> dict[str, Any]:
    level_reference = require_object_fields(
        reference, LEVEL_REFERENCE_FIELDS, f"closed-level reference {expected_index}"
    )
    if (
        require_integer(level_reference["level_index"], "closed level index")
        != expected_index
    ):
        raise ValueError("closed-level references are not sequential")
    referenced_level, canonical_level = exact_level(
        level_reference["squared_level_exact"], "closed-level reference level"
    )
    expected_level, _, _ = validate_equal_level_batch(
        expected_batch, f"expected closed level {expected_index}"
    )
    if (
        referenced_level != expected_level
        or level_reference["squared_level_exact"] != canonical_level
    ):
        raise ValueError("a closed-level reference has the wrong exact level")
    level_value, _ = validate_reference_file(
        checkpoint_dir,
        level_reference,
        level_relative_path(expected_index),
        f"closed-level artifact {expected_index}",
    )
    artifact = require_object_fields(
        level_value, LEVEL_FIELDS, f"closed-level artifact {expected_index}"
    )
    if (
        artifact["kind"] != LEVEL_KIND
        or artifact["schema_version"] != SCHEMA_VERSION
        or require_integer(artifact["level_index"], "closed-level artifact index")
        != expected_index
        or artifact["equal_level_batch"] != expected_batch
    ):
        raise ValueError(f"closed-level artifact {expected_index} is inconsistent")
    return artifact


def expected_result_from_closed_levels(
    checkpoint_dir: Path, references: Sequence[dict[str, Any]]
) -> dict[str, Any]:
    batches: list[dict[str, Any]] = []
    for level_index, reference in enumerate(references):
        value, _ = validate_reference_file(
            checkpoint_dir,
            reference,
            level_relative_path(level_index),
            f"closed-level artifact {level_index}",
        )
        artifact = require_object_fields(
            value, LEVEL_FIELDS, f"closed-level artifact {level_index}"
        )
        batches.append(artifact["equal_level_batch"])
    return result_from_batches(batches)


def validate_result_reference(
    checkpoint_dir: Path,
    reference: Any,
    expected: dict[str, Any],
) -> None:
    result_reference = require_object_fields(
        reference, RESULT_REFERENCE_FIELDS, "result reference"
    )
    expected_content = canonical_file_bytes(expected)
    expected_relative_path = (
        f"{RESULTS_DIRECTORY}/result-{sha256_bytes(expected_content)}.json"
    )
    if result_reference["relative_path"] != expected_relative_path:
        raise ValueError("the content-addressed result path is not canonical")
    for field in (
        "duplicate_emission_count",
        "emission_count",
        "equal_level_batch_count",
        "unique_emission_count",
    ):
        require_integer(result_reference[field], f"result reference {field}")
    if (
        result_reference["duplicate_emission_count"]
        != expected["duplicate_emission_count"]
        or result_reference["emission_count"] != expected["emission_count"]
        or result_reference["unique_emission_count"]
        != expected["unique_emission_count"]
        or result_reference["equal_level_batch_count"]
        != len(expected["equal_level_batches"])
    ):
        raise ValueError("the result reference counters changed")
    value, _ = validate_reference_file(
        checkpoint_dir,
        result_reference,
        expected_relative_path,
        "final result artifact",
    )
    validate_native_payload(value, "final result")
    if value != expected:
        raise ValueError("the final result differs from the closed levels")


def configuration(chunk_size: int) -> dict[str, Any]:
    return {
        "chunk_size": chunk_size,
        "emission_kind": EMISSION_KIND,
        "internal_schema_version": SCHEMA_VERSION,
        "sort_order_id": SORT_ORDER_ID,
    }


def verify_bound_files(
    manifest: dict[str, Any],
    input_path: Path,
    native: Path,
    tool_path: Path,
    *,
    verify_input: bool,
    expected_input_size_bytes: int | None = None,
) -> None:
    if sha256_file(native) != manifest["native_executable_sha256"]:
        raise ValueError("the native executable changed during the checkpoint session")
    if sha256_file(tool_path) != manifest["tool_sha256"]:
        raise ValueError("the checkpoint tool changed during its running session")
    if verify_input:
        input_sha256, emission_count, input_size_bytes = input_identity(input_path)
        expected_size = (
            manifest["next_input_offset_bytes"]
            if expected_input_size_bytes is None
            else expected_input_size_bytes
        )
        if (
            input_sha256 != manifest["input_sha256"]
            or emission_count != manifest["emission_count"]
            or input_size_bytes != expected_size
        ):
            raise ValueError("the level input changed during the checkpoint session")


def checkpoint_identifier(
    input_sha256: str,
    config_sha256: str,
    tool_sha256: str,
    native_sha256: str,
) -> str:
    domain = b"MorseHGP3D/phase2a-level-checkpoint-v1/"
    return sha256_bytes(
        domain
        + input_sha256.encode("ascii")
        + config_sha256.encode("ascii")
        + tool_sha256.encode("ascii")
        + native_sha256.encode("ascii")
    )


def initialize_checkpoint(
    checkpoint_dir: Path,
    input_sha256: str,
    emission_count: int,
    chunk_size: int,
    config_sha256: str,
    tool_sha256: str,
    native_sha256: str,
) -> dict[str, Any]:
    if checkpoint_dir.exists():
        raise ValueError("the checkpoint directory already exists without a manifest")
    if checkpoint_dir.parent.is_symlink() or not checkpoint_dir.parent.is_dir():
        raise ValueError("the checkpoint parent must be an existing regular directory")
    manifest: dict[str, Any] = {
        "checkpoint_id": checkpoint_identifier(
            input_sha256, config_sha256, tool_sha256, native_sha256
        ),
        "chunk_size": chunk_size,
        "closed_levels": [],
        "config_sha256": config_sha256,
        "emission_count": emission_count,
        "input_sha256": input_sha256,
        "kind": CHECKPOINT_KIND,
        "last_fully_closed_level": None,
        "native_executable_sha256": native_sha256,
        "next_emission_index": 0,
        "next_input_offset_bytes": 0,
        "public_contract_claimed": False,
        "result_artifact": None,
        "run_catalog_sha256": None,
        "run_cursors": [],
        "runs": [],
        "schema_version": SCHEMA_VERSION,
        "sequence_number": 0,
        "sort_order_id": SORT_ORDER_ID,
        "state": "collecting_runs",
        "tool_sha256": tool_sha256,
    }
    temporary = Path(
        tempfile.mkdtemp(
            dir=checkpoint_dir.parent,
            prefix=f".{checkpoint_dir.name}.initialize.",
        )
    )
    published = False
    try:
        (temporary / "runs").mkdir()
        (temporary / "levels").mkdir()
        (temporary / RESULTS_DIRECTORY).mkdir()
        fsync_directory(temporary)
        publish_manifest(temporary, manifest)
        maybe_fail_after("initialization_ready")
        os.rename(temporary, checkpoint_dir)
        fsync_directory(checkpoint_dir.parent)
        published = True
        maybe_fail_after("initialization_published")
    finally:
        if not published and temporary.exists():
            shutil.rmtree(temporary)
    return manifest


def validate_checkpoint(
    checkpoint_dir: Path,
    input_path: Path,
    input_sha256: str,
    input_size_bytes: int,
    emission_count: int,
    chunk_size: int,
    config_sha256: str,
    tool_sha256: str,
    native_sha256: str,
) -> dict[str, Any]:
    if checkpoint_dir.is_symlink() or not checkpoint_dir.is_dir():
        raise ValueError("the checkpoint directory is not a regular directory")
    for child in (
        checkpoint_dir / "runs",
        checkpoint_dir / "levels",
        checkpoint_dir / RESULTS_DIRECTORY,
    ):
        if child.is_symlink() or not child.is_dir():
            raise ValueError("a checkpoint artifact directory is invalid")
    value, _ = load_canonical_file(
        checkpoint_dir / MANIFEST_NAME, "checkpoint manifest"
    )
    manifest = require_object_fields(value, CHECKPOINT_FIELDS, "checkpoint manifest")
    if (
        manifest["kind"] != CHECKPOINT_KIND
        or manifest["schema_version"] != SCHEMA_VERSION
        or manifest["public_contract_claimed"] is not False
        or manifest["sort_order_id"] != SORT_ORDER_ID
    ):
        raise ValueError("the checkpoint manifest changed its internal contract")
    if (
        require_hash(manifest["input_sha256"], "checkpoint input digest")
        != input_sha256
        or require_hash(manifest["config_sha256"], "checkpoint config digest")
        != config_sha256
        or require_hash(manifest["tool_sha256"], "checkpoint tool digest")
        != tool_sha256
        or require_hash(
            manifest["native_executable_sha256"], "checkpoint native digest"
        )
        != native_sha256
        or require_hash(manifest["checkpoint_id"], "checkpoint identifier")
        != checkpoint_identifier(
            input_sha256, config_sha256, tool_sha256, native_sha256
        )
    ):
        raise ValueError(
            "the checkpoint is bound to different input, code, or configuration"
        )
    if (
        require_integer(manifest["chunk_size"], "checkpoint chunk size", minimum=1)
        != chunk_size
        or require_integer(
            manifest["emission_count"], "checkpoint emission count", minimum=1
        )
        != emission_count
    ):
        raise ValueError("the checkpoint dimensions changed")
    state = manifest["state"]
    if state not in {"collecting_runs", "between_levels", "complete"}:
        raise ValueError("the checkpoint state is unknown")
    sequence_number = require_integer(
        manifest["sequence_number"], "checkpoint sequence number"
    )
    next_index = require_integer(
        manifest["next_emission_index"],
        "checkpoint emission cursor",
        maximum=emission_count,
    )
    next_offset = require_integer(
        manifest["next_input_offset_bytes"],
        "checkpoint byte cursor",
        maximum=input_size_bytes,
    )
    runs = manifest["runs"]
    if not isinstance(runs, list):
        raise ValueError("checkpoint runs are not an array")
    expected_begin = 0
    expected_offset = 0
    batch_counts: list[int] = []
    for run_index, reference in enumerate(runs):
        expected_begin, expected_offset, batch_count = validate_run_reference(
            checkpoint_dir,
            input_path,
            reference,
            run_index,
            expected_begin,
            expected_offset,
            chunk_size,
            emission_count,
        )
        batch_counts.append(batch_count)
    if expected_begin != next_index or expected_offset != next_offset:
        raise ValueError("the checkpoint cursor differs from its run catalog")
    if state == "collecting_runs":
        if (
            next_index >= emission_count
            or manifest["run_catalog_sha256"] is not None
            or manifest["run_cursors"] != []
            or manifest["closed_levels"] != []
            or manifest["last_fully_closed_level"] is not None
            or manifest["result_artifact"] is not None
        ):
            raise ValueError("a collecting checkpoint contains merge state")
    else:
        if next_index != emission_count or next_offset != input_size_bytes:
            raise ValueError("merge started before all source runs were committed")
        expected_catalog_hash = canonical_hash(runs)
        if (
            require_hash(manifest["run_catalog_sha256"], "run catalog digest")
            != expected_catalog_hash
        ):
            raise ValueError("the frozen run catalog digest changed")
    validate_global_support_levels(checkpoint_dir, runs)
    cursors = manifest["run_cursors"]
    if state != "collecting_runs":
        if not isinstance(cursors, list) or len(cursors) != len(runs):
            raise ValueError("checkpoint run cursors do not match the run catalog")
        for run_index, (cursor, batch_count) in enumerate(zip(cursors, batch_counts)):
            require_integer(
                cursor,
                f"run cursor {run_index}",
                maximum=batch_count,
            )
    closed_levels = manifest["closed_levels"]
    if not isinstance(closed_levels, list):
        raise ValueError("checkpoint closed levels are not an array")
    expected_cursors = [0 for _ in runs]
    last_level: dict[str, str] | None = None
    for level_index, reference in enumerate(closed_levels):
        merged = next_merged_level(checkpoint_dir, runs, expected_cursors)
        if merged is None:
            raise ValueError("checkpoint contains more closed levels than its runs")
        expected_batch, expected_cursors = merged
        validate_level_reference(checkpoint_dir, reference, level_index, expected_batch)
        last_level = expected_batch["squared_level_exact"]
    if state != "collecting_runs" and expected_cursors != cursors:
        raise ValueError("checkpoint run cursors do not match its closed levels")
    if manifest["last_fully_closed_level"] != last_level:
        raise ValueError("checkpoint last closed level is inconsistent")
    expected_sequence = (
        len(runs) + len(closed_levels) + (1 if state == "complete" else 0)
    )
    if sequence_number != expected_sequence:
        raise ValueError("checkpoint sequence number is inconsistent")
    result_reference = manifest["result_artifact"]
    if state == "complete":
        if any(cursor != count for cursor, count in zip(cursors, batch_counts)):
            raise ValueError("a complete checkpoint has unread run batches")
        expected_result = expected_result_from_closed_levels(
            checkpoint_dir, closed_levels
        )
        validate_result_reference(checkpoint_dir, result_reference, expected_result)
        if expected_result["emission_count"] != emission_count:
            raise ValueError("the complete checkpoint lost source emissions")
    elif result_reference is not None:
        raise ValueError("an incomplete checkpoint references a final result")
    return manifest


def support_levels_from_runs(
    checkpoint_dir: Path, runs: Sequence[dict[str, Any]]
) -> dict[tuple[int, ...], Fraction]:
    levels: dict[tuple[int, ...], Fraction] = {}
    for run_index, reference in enumerate(runs):
        payload = load_run_payload(checkpoint_dir, reference, run_index)
        for batch in payload["equal_level_batches"]:
            level, _, _ = validate_equal_level_batch(batch, f"run {run_index} batch")
            for support in batch["supports"]:
                levels[tuple(support["minimal_support_ids"])] = level
    return levels


def commit_next_run(
    manifest: dict[str, Any],
    checkpoint_dir: Path,
    input_path: Path,
    native: Path,
    tool_path: Path,
    timeout_seconds: int,
) -> None:
    verify_bound_files(manifest, input_path, native, tool_path, verify_input=False)
    begin = manifest["next_emission_index"]
    begin_offset = manifest["next_input_offset_bytes"]
    remaining = manifest["emission_count"] - begin
    emissions, end_offset, source_content = read_emission_chunk(
        input_path,
        begin,
        begin_offset,
        min(manifest["chunk_size"], remaining),
    )
    end = begin + len(emissions)
    payload = invoke_native(native, emissions, timeout_seconds)
    if end == manifest["emission_count"]:
        verify_bound_files(
            manifest,
            input_path,
            native,
            tool_path,
            verify_input=True,
            expected_input_size_bytes=end_offset,
        )
    existing_levels = support_levels_from_runs(checkpoint_dir, manifest["runs"])
    for batch in payload["equal_level_batches"]:
        level, _, _ = validate_equal_level_batch(batch, "new run batch")
        for support in batch["supports"]:
            minimal = tuple(support["minimal_support_ids"])
            previous = existing_levels.setdefault(minimal, level)
            if previous != level:
                raise ValueError(
                    "one canonical minimal support has different levels across chunks"
                )
    run_index = len(manifest["runs"])
    artifact = {
        "kind": RUN_KIND,
        "payload": payload,
        "run_index": run_index,
        "schema_version": SCHEMA_VERSION,
        "source_begin": begin,
        "source_begin_offset_bytes": begin_offset,
        "source_end": end,
        "source_end_offset_bytes": end_offset,
        "source_sha256": sha256_bytes(source_content),
    }
    content = canonical_file_bytes(artifact)
    relative_path = run_relative_path(run_index)
    atomic_publish(
        checkpoint_dir / relative_path,
        content,
        failpoint="run_artifact",
    )
    if end == manifest["emission_count"]:
        verify_bound_files(
            manifest,
            input_path,
            native,
            tool_path,
            verify_input=True,
            expected_input_size_bytes=end_offset,
        )
    reference = artifact_reference(
        relative_path,
        content,
        run_index=run_index,
        source_begin=begin,
        source_begin_offset_bytes=begin_offset,
        source_end=end,
        source_end_offset_bytes=end_offset,
    )
    manifest["runs"].append(reference)
    manifest["next_emission_index"] = end
    manifest["next_input_offset_bytes"] = end_offset
    manifest["sequence_number"] += 1
    if end == manifest["emission_count"]:
        manifest["state"] = "between_levels"
        manifest["run_cursors"] = [0 for _ in manifest["runs"]]
        manifest["run_catalog_sha256"] = canonical_hash(manifest["runs"])
    publish_manifest(checkpoint_dir, manifest)


def commit_next_level(manifest: dict[str, Any], checkpoint_dir: Path) -> bool:
    merged = next_merged_level(
        checkpoint_dir, manifest["runs"], manifest["run_cursors"]
    )
    if merged is None:
        return False
    batch, next_cursors = merged
    level_index = len(manifest["closed_levels"])
    artifact = {
        "equal_level_batch": batch,
        "kind": LEVEL_KIND,
        "level_index": level_index,
        "schema_version": SCHEMA_VERSION,
    }
    content = canonical_file_bytes(artifact)
    relative_path = level_relative_path(level_index)
    atomic_publish(
        checkpoint_dir / relative_path,
        content,
        failpoint="level_artifact",
    )
    reference = artifact_reference(
        relative_path,
        content,
        level_index=level_index,
        squared_level_exact=batch["squared_level_exact"],
    )
    manifest["closed_levels"].append(reference)
    manifest["last_fully_closed_level"] = batch["squared_level_exact"]
    manifest["run_cursors"] = next_cursors
    manifest["sequence_number"] += 1
    publish_manifest(checkpoint_dir, manifest)
    return True


def commit_result(
    manifest: dict[str, Any],
    checkpoint_dir: Path,
    input_path: Path,
    native: Path,
    tool_path: Path,
) -> None:
    verify_bound_files(manifest, input_path, native, tool_path, verify_input=True)
    expected_begin = 0
    expected_offset = 0
    for run_index, reference in enumerate(manifest["runs"]):
        expected_begin, expected_offset, _ = validate_run_reference(
            checkpoint_dir,
            input_path,
            reference,
            run_index,
            expected_begin,
            expected_offset,
            manifest["chunk_size"],
            manifest["emission_count"],
        )
    if (
        expected_begin != manifest["emission_count"]
        or expected_offset != manifest["next_input_offset_bytes"]
    ):
        raise ValueError("the run catalog no longer covers the bound input")
    result = expected_result_from_closed_levels(
        checkpoint_dir, manifest["closed_levels"]
    )
    if result["emission_count"] != manifest["emission_count"]:
        raise ValueError("the merged result did not preserve every source emission")
    content = canonical_file_bytes(result)
    relative_path = f"{RESULTS_DIRECTORY}/result-{sha256_bytes(content)}.json"
    atomic_publish(
        checkpoint_dir / relative_path,
        content,
        failpoint="result_artifact",
    )
    verify_bound_files(manifest, input_path, native, tool_path, verify_input=True)
    manifest["result_artifact"] = artifact_reference(
        relative_path,
        content,
        duplicate_emission_count=result["duplicate_emission_count"],
        emission_count=result["emission_count"],
        equal_level_batch_count=len(result["equal_level_batches"]),
        unique_emission_count=result["unique_emission_count"],
    )
    manifest["state"] = "complete"
    manifest["sequence_number"] += 1
    publish_manifest(checkpoint_dir, manifest)


def ensure_result_alias(manifest: dict[str, Any], checkpoint_dir: Path) -> None:
    if manifest["state"] != "complete":
        return
    reference = require_object_fields(
        manifest["result_artifact"], RESULT_REFERENCE_FIELDS, "result reference"
    )
    relative_path = reference["relative_path"]
    if not isinstance(relative_path, str) or not relative_path.startswith(
        f"{RESULTS_DIRECTORY}/result-"
    ):
        raise ValueError("the result artifact path is invalid")
    _, content = validate_reference_file(
        checkpoint_dir, reference, relative_path, "final result artifact"
    )
    alias = checkpoint_dir / RESULT_NAME
    if alias.exists():
        _, existing = load_canonical_file(alias, "final result alias")
        if existing == content:
            return
        raise ValueError("the final result alias differs from its committed artifact")
    atomic_publish(alias, content, failpoint="result_alias")


def positive_integer(value: str) -> int:
    parsed = int(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("the value must be positive")
    return parsed


def nonnegative_integer(value: str) -> int:
    parsed = int(value)
    if parsed < 0:
        raise argparse.ArgumentTypeError("the value must be nonnegative")
    return parsed


@contextlib.contextmanager
def checkpoint_writer_lock(checkpoint_dir: Path) -> Any:
    resolved = checkpoint_dir.resolve()
    parent = resolved.parent
    parent.mkdir(parents=True, exist_ok=True)
    if parent.is_symlink() or not parent.is_dir():
        raise ValueError("the checkpoint parent must be an existing regular directory")
    lock_path = parent / f".{resolved.name}.writer.lock"
    flags = os.O_RDWR | os.O_CREAT | getattr(os, "O_CLOEXEC", 0)
    if hasattr(os, "O_NOFOLLOW"):
        flags |= os.O_NOFOLLOW
    descriptor = os.open(lock_path, flags, 0o600)
    try:
        try:
            fcntl.flock(descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise ValueError("another writer owns this checkpoint") from error
        yield
    finally:
        fcntl.flock(descriptor, fcntl.LOCK_UN)
        os.close(descriptor)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", required=True, type=Path)
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--checkpoint-dir", required=True, type=Path)
    parser.add_argument("--chunk-size", required=True, type=positive_integer)
    parser.add_argument("--max-transactions", type=nonnegative_integer)
    parser.add_argument("--timeout-seconds", type=positive_integer, default=30)
    arguments = parser.parse_args()
    if arguments.chunk_size > MAX_CHUNK_SIZE:
        parser.error(f"--chunk-size cannot exceed {MAX_CHUNK_SIZE}")
    return arguments


def run(arguments: argparse.Namespace) -> int:
    input_path = arguments.input.resolve()
    native = arguments.native.resolve()
    checkpoint_dir = arguments.checkpoint_dir.resolve()
    require_regular_file(native, "native predicate replay")
    if not os.access(native, os.X_OK):
        raise ValueError("the native predicate replay is not executable")
    input_sha256, emission_count, input_size_bytes = input_identity(input_path)
    tool_path = Path(__file__).resolve()
    require_regular_file(tool_path, "phase 2A checkpoint tool")
    tool_sha256 = sha256_file(tool_path)
    native_sha256 = sha256_file(native)
    config_sha256 = canonical_hash(configuration(arguments.chunk_size))
    manifest_path = checkpoint_dir / MANIFEST_NAME
    if manifest_path.exists():
        manifest = validate_checkpoint(
            checkpoint_dir,
            input_path,
            input_sha256,
            input_size_bytes,
            emission_count,
            arguments.chunk_size,
            config_sha256,
            tool_sha256,
            native_sha256,
        )
    else:
        manifest = initialize_checkpoint(
            checkpoint_dir,
            input_sha256,
            emission_count,
            arguments.chunk_size,
            config_sha256,
            tool_sha256,
            native_sha256,
        )
    transaction_limit = arguments.max_transactions
    committed = 0
    while manifest["state"] != "complete":
        if transaction_limit is not None and committed >= transaction_limit:
            break
        if manifest["state"] == "collecting_runs":
            commit_next_run(
                manifest,
                checkpoint_dir,
                input_path,
                native,
                tool_path,
                arguments.timeout_seconds,
            )
        elif commit_next_level(manifest, checkpoint_dir):
            pass
        else:
            commit_result(manifest, checkpoint_dir, input_path, native, tool_path)
        committed += 1
    ensure_result_alias(manifest, checkpoint_dir)
    print(
        canonical_json(
            {
                "completed": manifest["state"] == "complete",
                "sequence_number": manifest["sequence_number"],
                "state": manifest["state"],
            }
        )
    )
    return 0


def main() -> int:
    arguments = parse_arguments()
    try:
        with checkpoint_writer_lock(arguments.checkpoint_dir):
            return run(arguments)
    except (OSError, RuntimeError, subprocess.SubprocessError, ValueError) as error:
        print(f"level checkpoint failed closed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
