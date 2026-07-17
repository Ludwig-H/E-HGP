#!/usr/bin/env python3
"""Run resumable Phase 2B GPU sign campaigns against two exact CPU oracles.

The production ``both`` mode replays the frozen Phase 2A corpus and then an
independently seeded Phase 2B corpus.  Each GPU invocation is homogeneous by
predicate and bounded by the configured base-case chunk.  No certificate
emitted here claims that a phase gate is closed; G4 qualification is separate.
"""

from __future__ import annotations

import argparse
import fcntl
import hashlib
import importlib.util
import json
import math
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from types import ModuleType
from typing import Any, BinaryIO, NoReturn, Sequence


SCHEMA_VERSION = "1.1.0"
MANIFEST_KIND = "morsehgp3d_phase2b_gpu_sign_campaign_checkpoint"
CHUNK_KIND = "morsehgp3d_phase2b_gpu_sign_campaign_chunk"
CERTIFICATE_KIND = "morsehgp3d_phase2b_gpu_sign_campaign_certificate"
AGGREGATE_KIND = "morsehgp3d_phase2b_gpu_sign_campaign_aggregate"
ORDERED_ROOT_ALGORITHM = "sha256-record-chain-v1"
DEFAULT_CHUNK_BASE_CASES = 196_608
MAXIMUM_CHUNK_BASE_CASES = 196_608
DEFAULT_CASES_PER_PREDICATE = 3_600_000
MINIMUM_ADDITIONAL_SIGNS = 10_000_000
EXPECTED_PRODUCTION_BASE_CASES = 10_800_000
EXPECTED_PRODUCTION_METAMORPHIC_SIGNS = 1_080_000
EXPECTED_PRODUCTION_TOTAL_SIGNS = 11_880_000
PHASE2B_SEED = 0x4257325F47505532
CAMPAIGNS = ("phase2a", "phase2b")
PREDICATES = (
    "compare_squared_distances",
    "orientation_3d",
    "power_bisector_side",
)
METAMORPHISMS = (
    "exact_power_of_two_scaling",
    "improper_signed_axis_permutation",
    "nonzero_exact_dyadic_translation",
    "predicate_argument_symmetry",
    "proper_signed_axis_permutation",
)
METAMORPHIC_RELATIONS = ("opposite_sign", "same_sign")
CPU_FALLBACK_STAGES = ("fp64_filtered", "expansion", "cpu_multiprecision")
SIGNS = frozenset({"negative", "zero", "positive"})
GPU_SIGNS = frozenset({"negative", "unknown", "positive"})
STAGES = frozenset(CPU_FALLBACK_STAGES)
CPU_RESULT_FIELDS = frozenset(
    {"certification_stage", "counters", "predicate", "sign"}
)
CPU_COUNTER_FIELDS = frozenset(
    {
        "cpu_multiprecision_certified",
        "exact_zeros",
        "expansion_certified",
        "fp32_proposals",
        "fp64_filtered_certified",
        "remaining_unknown",
    }
)
GPU_RESULT_FIELDS = frozenset(
    {
        "certification_stage",
        "gpu_filter_sign",
        "kind",
        "predicate",
        "replay_command",
        "replay_id",
        "sign",
    }
)
GPU_SUMMARY_FIELDS = frozenset({"audit_gpu_signs", "counters", "kind", "schema"})
GPU_COUNTER_FIELDS = frozenset(
    {
        "async_fallback_batches",
        "cpu_expansion_certified",
        "cpu_fp64_filtered_certified",
        "cpu_multiprecision_certified",
        "exact_zeros",
        "gpu_fp64_certified",
        "gpu_inputs",
        "gpu_known_audited",
        "gpu_unknown_forwarded",
        "remaining_unknown",
    }
)
GPU_SCHEMAS = {
    "compare_squared_distances": "morsehgp3d.phase2b.distance_filter.v1",
    "orientation_3d": "morsehgp3d.phase2b.orientation_3d_filter.v1",
    "power_bisector_side": "morsehgp3d.phase2b.power_bisector_side_filter.v1",
}
ROOT_DOMAINS = {
    "cpu": b"MorseHGP3D/phase2b-cpu-replay-chain-v1/",
    "gpu": b"MorseHGP3D/phase2b-gpu-replay-chain-v1/",
}
MANIFEST_FIELDS = frozenset(
    {
        "base_case_count",
        "campaign",
        "chunk_size_base_cases",
        "chunks",
        "counts",
        "identities",
        "kind",
        "next_base_index",
        "ordered_root_algorithm",
        "ordered_roots",
        "repository",
        "runtime_milliseconds",
        "schema_version",
        "scope",
        "seed",
        "state",
    }
)
CHUNK_FIELDS = frozenset(
    {
        "base_begin",
        "base_end",
        "campaign",
        "counts",
        "kind",
        "ordered_roots_after",
        "ordered_roots_before",
        "predicate_counts",
        "schema_version",
    }
)


def fail(message: str) -> NoReturn:
    raise ValueError(message)


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=True,
        separators=(",", ":"),
        sort_keys=True,
    )


def canonical_bytes(value: object) -> bytes:
    return (canonical_json(value) + "\n").encode("ascii")


def strict_json_loads(text: str) -> Any:
    def reject_duplicates(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                fail(f"duplicate JSON key: {key}")
            result[key] = value
        return result

    def reject_nonfinite(value: str) -> NoReturn:
        fail(f"non-finite JSON number: {value}")

    return json.loads(
        text,
        object_pairs_hook=reject_duplicates,
        parse_constant=reject_nonfinite,
    )


def exact_fields(value: Any, fields: frozenset[str], label: str) -> dict[str, Any]:
    if not isinstance(value, dict) or frozenset(value) != fields:
        fail(f"{label} has missing or unknown fields")
    return value


def exact_integer(value: Any, label: str, *, minimum: int = 0) -> int:
    if type(value) is not int or value < minimum:
        fail(f"{label} must be an integer greater than or equal to {minimum}")
    return value


def sha256_bytes(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while block := stream.read(1024 * 1024):
            digest.update(block)
    return digest.hexdigest()


def fsync_directory(path: Path) -> None:
    descriptor = os.open(path, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
    try:
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def atomic_publish(path: Path, content: bytes) -> None:
    descriptor, temporary_name = tempfile.mkstemp(
        dir=path.parent, prefix=f".{path.name}.", suffix=".tmp"
    )
    temporary = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(content)
            stream.flush()
            os.fsync(stream.fileno())
        if sha256_file(temporary) != sha256_bytes(content):
            fail(f"temporary publication verification failed: {path.name}")
        os.replace(temporary, path)
        fsync_directory(path.parent)
    finally:
        temporary.unlink(missing_ok=True)


def load_canonical(path: Path, label: str) -> tuple[Any, bytes]:
    if path.is_symlink() or not path.is_file():
        fail(f"{label} is not a regular file")
    content = path.read_bytes()
    try:
        value = strict_json_loads(content.decode("ascii"))
    except UnicodeDecodeError as error:
        raise ValueError(f"{label} is not ASCII JSON") from error
    if content != canonical_bytes(value):
        fail(f"{label} is not canonical JSON")
    return value, content


def prepare_shard_directory(path: Path) -> None:
    if path.exists():
        if path.is_symlink() or not path.is_dir():
            fail("the campaign shard must be a physical directory")
    else:
        path.mkdir(parents=True)
    chunks = path / "chunks"
    if chunks.exists():
        if chunks.is_symlink() or not chunks.is_dir():
            fail("the campaign chunk store must be a physical directory")
    else:
        chunks.mkdir()


def validate_shard_layout(
    checkpoint_dir: Path, manifest: dict[str, Any] | None
) -> None:
    if checkpoint_dir.is_symlink() or not checkpoint_dir.is_dir():
        fail("the campaign shard is not a physical directory")
    chunks_dir = checkpoint_dir / "chunks"
    if chunks_dir.is_symlink() or not chunks_dir.is_dir():
        fail("the campaign chunk store is not a physical directory")

    allowed_root = {"chunks"}
    if manifest is not None:
        allowed_root.add("checkpoint.json")
        if manifest.get("state") == "complete":
            allowed_root.add("result.json")
    actual_root: set[str] = set()
    for entry in checkpoint_dir.iterdir():
        if entry.is_symlink():
            fail(f"the campaign shard contains a symbolic entry: {entry.name}")
        actual_root.add(entry.name)
    unexpected_root = actual_root - allowed_root
    if unexpected_root:
        fail(
            "the campaign shard contains a temporary or unexpected entry: "
            + sorted(unexpected_root)[0]
        )
    if manifest is not None and "checkpoint.json" not in actual_root:
        fail("the campaign shard lost its checkpoint manifest")

    expected_chunks: set[str] = set()
    if manifest is not None:
        references = manifest.get("chunks")
        if not isinstance(references, list):
            fail("the campaign checkpoint chunks are invalid")
        for reference in references:
            if not isinstance(reference, dict):
                fail("a checkpoint chunk reference is invalid")
            relative = reference.get("relative_path")
            if not isinstance(relative, str) or not relative.startswith("chunks/"):
                fail("a checkpoint chunk reference escaped its canonical path")
            expected_chunks.add(relative.removeprefix("chunks/"))

    actual_chunks: set[str] = set()
    for entry in chunks_dir.iterdir():
        if entry.is_symlink() or not entry.is_file():
            fail(
                "the campaign chunk store contains a temporary or non-file entry: "
                f"{entry.name}"
            )
        actual_chunks.add(entry.name)
    orphaned = actual_chunks - expected_chunks
    if orphaned:
        fail(f"the campaign chunk store contains an orphan: {sorted(orphaned)[0]}")
    missing = expected_chunks - actual_chunks
    if missing:
        fail(f"the campaign chunk store lost a referenced chunk: {sorted(missing)[0]}")


def load_phase2a_tool(path: Path) -> ModuleType:
    spec = importlib.util.spec_from_file_location("phase2a_predicate_generator", path)
    if spec is None or spec.loader is None:
        fail("the Phase 2A generator cannot be loaded")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    previous_bytecode_policy = sys.dont_write_bytecode
    sys.dont_write_bytecode = True
    try:
        spec.loader.exec_module(module)
    finally:
        sys.dont_write_bytecode = previous_bytecode_policy
    return module


def git_output(repository: Path, *arguments: str) -> bytes:
    completed = subprocess.run(
        ("git", "-C", str(repository), *arguments),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if completed.returncode != 0:
        fail(f"Git provenance failed: {completed.stderr.decode(errors='replace')!r}")
    return completed.stdout


def repository_snapshot(repository: Path) -> dict[str, Any]:
    head = git_output(repository, "rev-parse", "HEAD").decode("ascii").strip()
    status = git_output(
        repository, "status", "--porcelain=v1", "-z", "--untracked-files=all"
    )
    diff = git_output(repository, "diff", "--binary", "HEAD", "--", ".")
    return {
        "git_head": head,
        "repository_clean": not status,
        "worktree_sha256": sha256_bytes(status + b"\0" + diff),
    }


def validate_executable(path: Path, label: str) -> None:
    if path.is_symlink() or not path.is_file() or not os.access(path, os.X_OK):
        fail(f"{label} must be an executable regular file")


def verify_runtime_inputs(
    *,
    repository: Path,
    snapshot: dict[str, Any],
    gpu_replay: Path,
    cpu_replay: Path,
    generator_path: Path,
    config_path: Path,
    identities: dict[str, Any],
) -> None:
    if repository_snapshot(repository) != snapshot:
        fail("the measured repository changed during the campaign")
    actual = {
        "config_sha256": sha256_file(config_path),
        "cpu_replay_sha256": sha256_file(cpu_replay),
        "generator_sha256": sha256_file(generator_path),
        "gpu_replay_sha256": sha256_file(gpu_replay),
        "tool_sha256": sha256_file(Path(__file__).resolve()),
    }
    for field, value in actual.items():
        if identities.get(field) != value:
            fail(f"campaign runtime input changed: {field}")


def extend_root(name: str, root: str, row: bytes) -> str:
    domain = ROOT_DOMAINS[name]
    return sha256_bytes(domain + bytes.fromhex(root) + hashlib.sha256(row).digest())


def initial_roots(phase2a: ModuleType) -> dict[str, str]:
    roots = phase2a.initial_ordered_roots()
    return {
        "corpus": roots["corpus"],
        "oracle": roots["oracle"],
        "cpu": sha256_bytes(ROOT_DOMAINS["cpu"] + b"initial"),
        "gpu": sha256_bytes(ROOT_DOMAINS["gpu"] + b"initial"),
    }


def validate_roots(value: Any, label: str) -> dict[str, str]:
    if not isinstance(value, dict) or set(value) != {"corpus", "oracle", "cpu", "gpu"}:
        fail(f"{label} has an invalid schema")
    for root in value.values():
        if (
            not isinstance(root, str)
            or len(root) != 64
            or any(character not in "0123456789abcdef" for character in root)
        ):
            fail(f"{label} contains a non-SHA256 root")
    return value


def empty_counts() -> dict[str, Any]:
    return {
        "base_case_count": 0,
        "by_metamorphic_relation": {
            relation: 0 for relation in METAMORPHIC_RELATIONS
        },
        "by_metamorphism": {
            metamorphism: 0 for metamorphism in METAMORPHISMS
        },
        "by_predicate": {predicate: 0 for predicate in PREDICATES},
        "by_sign": {sign: 0 for sign in sorted(SIGNS)},
        "cpu_multiprecision_certified": 0,
        "exact_zeros": 0,
        "gpu_fp64_certified": 0,
        "gpu_unknown_forwarded": 0,
        "gpu_unknown_by_cpu_stage": {
            stage: 0 for stage in CPU_FALLBACK_STAGES
        },
        "metamorphic_sign_count": 0,
        "metamorphisms_verified": 0,
        "remaining_unknown": 0,
        "total_sign_count": 0,
        "wrong_sign": 0,
    }


def add_counts(target: dict[str, Any], addition: dict[str, Any]) -> None:
    for field in (
        "base_case_count",
        "cpu_multiprecision_certified",
        "exact_zeros",
        "gpu_fp64_certified",
        "gpu_unknown_forwarded",
        "metamorphic_sign_count",
        "metamorphisms_verified",
        "remaining_unknown",
        "total_sign_count",
        "wrong_sign",
    ):
        target[field] += addition[field]
    for predicate in PREDICATES:
        target["by_predicate"][predicate] += addition["by_predicate"][predicate]
    for stage in CPU_FALLBACK_STAGES:
        target["gpu_unknown_by_cpu_stage"][stage] += addition[
            "gpu_unknown_by_cpu_stage"
        ][stage]
    for metamorphism in METAMORPHISMS:
        target["by_metamorphism"][metamorphism] += addition["by_metamorphism"][
            metamorphism
        ]
    for relation in METAMORPHIC_RELATIONS:
        target["by_metamorphic_relation"][relation] += addition[
            "by_metamorphic_relation"
        ][relation]
    for sign in SIGNS:
        target["by_sign"][sign] += addition["by_sign"][sign]


def validate_counts(counts: Any, label: str) -> dict[str, Any]:
    expected = empty_counts()
    if not isinstance(counts, dict) or set(counts) != set(expected):
        fail(f"{label} has an invalid counter schema")
    for field, default in expected.items():
        if isinstance(default, dict):
            if not isinstance(counts[field], dict) or set(counts[field]) != set(default):
                fail(f"{label}.{field} has an invalid schema")
            for key, value in counts[field].items():
                exact_integer(value, f"{label}.{field}.{key}")
        else:
            exact_integer(counts[field], f"{label}.{field}")
    total = counts["total_sign_count"]
    if sum(counts["by_predicate"].values()) != total:
        fail(f"{label} predicate counts do not close")
    if sum(counts["by_sign"].values()) != total:
        fail(f"{label} sign counts do not close")
    if counts["base_case_count"] + counts["metamorphic_sign_count"] != total:
        fail(f"{label} base/metamorphic counts do not close")
    if sum(counts["by_metamorphism"].values()) != counts["metamorphic_sign_count"]:
        fail(f"{label} metamorphism counts do not close")
    if (
        sum(counts["by_metamorphic_relation"].values())
        != counts["metamorphic_sign_count"]
    ):
        fail(f"{label} metamorphic relation counts do not close")
    if counts["metamorphisms_verified"] != counts["metamorphic_sign_count"]:
        fail(f"{label} did not verify every metamorphic sign relation")
    if counts["cpu_multiprecision_certified"] != total:
        fail(f"{label} did not replay every sign through the exact CPU path")
    if counts["gpu_fp64_certified"] + counts["gpu_unknown_forwarded"] != total:
        fail(f"{label} GPU tri-state counts do not close")
    if (
        sum(counts["gpu_unknown_by_cpu_stage"].values())
        != counts["gpu_unknown_forwarded"]
    ):
        fail(f"{label} GPU fallback-stage counts do not close")
    if counts["remaining_unknown"] != 0 or counts["wrong_sign"] != 0:
        fail(f"{label} contains an unresolved or contradictory sign")
    if counts["exact_zeros"] != counts["by_sign"]["zero"]:
        fail(f"{label} exact-zero counts do not close")
    if counts["exact_zeros"] > counts["gpu_unknown_forwarded"]:
        fail(f"{label} promotes an exact zero as a GPU-known sign")
    return counts


def exact_binary64_integer(value: int) -> bool:
    if type(value) is not int:
        return False
    try:
        converted = float(value)
    except OverflowError:
        return False
    return math.isfinite(converted) and converted.is_integer() and int(converted) == value


def validate_gpu_case(case: Any) -> None:
    if case.predicate not in PREDICATES:
        fail("a generated case escaped the Phase 2B predicate catalog")
    for point in case.points:
        if len(point) != 3:
            fail("a generated point is not three-dimensional")
        for word in point:
            if (
                not isinstance(word, str)
                or len(word) != 16
                or any(character not in "0123456789abcdef" for character in word)
                or ((int(word, 16) >> 52) & 0x7FF) == 0x7FF
            ):
                fail("a generated coordinate is not finite canonical binary64")
    if case.predicate != "power_bisector_side":
        return
    if case.witness is None or len(case.witness) != 4:
        fail("a generated power-bisector case has no rational witness")
    x, y, z, denominator = case.witness
    if denominator <= 0 or not all(
        exact_binary64_integer(value) for value in (x, y, z, denominator)
    ):
        fail("a power-bisector witness is not exactly representable on the GPU")
    if math.gcd(denominator, math.gcd(abs(x), math.gcd(abs(y), abs(z)))) != 1:
        fail("a power-bisector witness is not a reduced homogeneous tuple")
    if not (1 <= len(case.r_ids) == len(case.q_ids) <= 10):
        fail("a power-bisector label cardinality escaped 1..10")
    if tuple(sorted(set(case.r_ids))) != case.r_ids or tuple(
        sorted(set(case.q_ids))
    ) != case.q_ids:
        fail("a power-bisector label is not sorted and unique")
    if len(case.points) > 20 or any(
        identifier < 0 or identifier >= len(case.points)
        for identifier in (*case.r_ids, *case.q_ids)
    ):
        fail("a power-bisector label identifier escaped its point table")


def replay_id(campaign_name: str, case: Any) -> int:
    prefix = 1_000_000_000_000_000 if campaign_name == "phase2a" else 2_000_000_000_000_000
    relation = 0 if case.relation == "base" else 1
    result = prefix + 2 * case.base_index + relation
    if not 0 <= result <= (1 << 64) - 1:
        fail("a deterministic replay identifier escaped uint64")
    return result


def read_canonical_line(stream: BinaryIO, label: str) -> tuple[dict[str, Any], bytes]:
    row = stream.readline()
    if not row or not row.endswith(b"\n"):
        fail(f"{label} ended before its expected row")
    try:
        line = row[:-1].decode("ascii")
    except UnicodeDecodeError as error:
        raise ValueError(f"{label} emitted non-ASCII JSON") from error
    value = strict_json_loads(line)
    if canonical_json(value) != line:
        fail(f"{label} emitted non-canonical JSON")
    if not isinstance(value, dict):
        fail(f"{label} row is not an object")
    return value, row


def replay_diagnostic(
    *,
    campaign_name: str,
    case: Any,
    expected_sign: str,
    row: dict[str, Any],
) -> str:
    return canonical_json(
        {
            "actual_certification_stage": row.get("certification_stage"),
            "actual_filter_sign": row.get("gpu_filter_sign"),
            "actual_replay_command": row.get("replay_command"),
            "actual_replay_id": row.get("replay_id"),
            "actual_sign": row.get("sign"),
            "base_index": case.base_index,
            "campaign": campaign_name,
            "expected_sign": expected_sign,
            "predicate": case.predicate,
            "relation": case.relation,
            "replay_command": case.command(),
            "replay_id": replay_id(campaign_name, case),
        }
    )


def validate_cpu_row(
    row: dict[str, Any],
    case: Any,
    campaign_name: str,
    expected_sign: str,
) -> None:
    exact_fields(row, CPU_RESULT_FIELDS, "CPU decision")
    if (
        row["predicate"] != case.predicate
        or row["sign"] != expected_sign
        or row["certification_stage"] != "cpu_multiprecision"
    ):
        fail(
            "the exact CPU replay contradicts the independent dyadic oracle: "
            + replay_diagnostic(
                campaign_name=campaign_name,
                case=case,
                expected_sign=expected_sign,
                row=row,
            )
        )
    counters = exact_fields(row["counters"], CPU_COUNTER_FIELDS, "CPU counters")
    for field, value in counters.items():
        exact_integer(value, f"CPU counters.{field}")
    expected = {
        "cpu_multiprecision_certified": 1,
        "exact_zeros": 1 if expected_sign == "zero" else 0,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 0,
        "remaining_unknown": 0,
    }
    if counters != expected:
        fail("the exact CPU replay emitted inconsistent counters")


def validate_gpu_row(
    row: dict[str, Any],
    case: Any,
    campaign_name: str,
    expected_sign: str,
) -> None:
    exact_fields(row, GPU_RESULT_FIELDS, "GPU decision")
    expected_id = replay_id(campaign_name, case)
    command = case.command()
    if (
        row["kind"] != "decision"
        or row["predicate"] != case.predicate
        or row["replay_id"] != expected_id
        or row["replay_command"] != command
        or row["sign"] != expected_sign
    ):
        fail(
            "the GPU replay changed order, provenance, or exact sign: "
            + replay_diagnostic(
                campaign_name=campaign_name,
                case=case,
                expected_sign=expected_sign,
                row=row,
            )
        )
    if row["gpu_filter_sign"] not in GPU_SIGNS or row["certification_stage"] not in STAGES:
        fail(
            "the GPU replay escaped its closed sign/stage enums: "
            + replay_diagnostic(
                campaign_name=campaign_name,
                case=case,
                expected_sign=expected_sign,
                row=row,
            )
        )
    if row["gpu_filter_sign"] == "unknown":
        if row["certification_stage"] not in STAGES:
            fail("a GPU unknown has no certified CPU resolution")
    elif (
        row["gpu_filter_sign"] != expected_sign
        or row["certification_stage"] != "fp64_filtered"
    ):
        fail(
            "a GPU-known sign is not a strict FP64 certificate: "
            + replay_diagnostic(
                campaign_name=campaign_name,
                case=case,
                expected_sign=expected_sign,
                row=row,
            )
        )


def empty_gpu_summary_counts() -> dict[str, int]:
    return {field: 0 for field in GPU_COUNTER_FIELDS}


def update_gpu_summary_counts(counters: dict[str, int], row: dict[str, Any]) -> None:
    counters["gpu_inputs"] += 1
    if row["gpu_filter_sign"] == "unknown":
        counters["gpu_unknown_forwarded"] += 1
        counters["async_fallback_batches"] = 1
        counters[
            {
                "fp64_filtered": "cpu_fp64_filtered_certified",
                "expansion": "cpu_expansion_certified",
                "cpu_multiprecision": "cpu_multiprecision_certified",
            }[row["certification_stage"]]
        ] += 1
        if row["sign"] == "zero":
            counters["exact_zeros"] += 1
    else:
        counters["gpu_fp64_certified"] += 1


def validate_gpu_summary(
    summary: dict[str, Any], predicate: str, expected: dict[str, int]
) -> None:
    exact_fields(summary, GPU_SUMMARY_FIELDS, "GPU summary")
    if (
        summary["kind"] != "summary"
        or summary["schema"] != GPU_SCHEMAS[predicate]
        or summary["audit_gpu_signs"] is not False
    ):
        fail("the GPU summary changed its schema or audit mode")
    counters = exact_fields(summary["counters"], GPU_COUNTER_FIELDS, "GPU summary counters")
    for field, value in counters.items():
        exact_integer(value, f"GPU summary counters.{field}")
    if counters != expected:
        fail("the GPU summary does not aggregate its emitted decisions")


def run_process(
    executable: Path,
    arguments: Sequence[str],
    input_path: Path,
    output_path: Path,
    timeout_seconds: int,
    label: str,
) -> None:
    with input_path.open("rb") as input_stream, output_path.open("wb") as output_stream:
        completed = subprocess.run(
            [str(executable), *arguments],
            stdin=input_stream,
            stdout=output_stream,
            stderr=subprocess.PIPE,
            check=False,
            timeout=timeout_seconds,
        )
        output_stream.flush()
        os.fsync(output_stream.fileno())
    stderr = completed.stderr.decode("utf-8", errors="replace")
    if completed.returncode != 0 or stderr:
        fail(
            f"{label} failed closed: returncode={completed.returncode}, stderr={stderr!r}"
        )


def execute_chunk(
    *,
    campaign_name: str,
    seed: int,
    stride: int,
    begin: int,
    end: int,
    gpu_replay: Path,
    cpu_replay: Path,
    checkpoint_dir: Path,
    timeout_seconds: int,
    roots_before: dict[str, str],
    phase2a: ModuleType,
) -> dict[str, Any]:
    counts = empty_counts()
    roots_after = dict(roots_before)
    predicate_counts = {predicate: 0 for predicate in PREDICATES}
    with tempfile.TemporaryDirectory(
        dir=checkpoint_dir, prefix=".phase2b-chunk-"
    ) as temporary_name:
        temporary = Path(temporary_name)
        cpu_inputs = {
            predicate: temporary / f"{index}.cpu.in"
            for index, predicate in enumerate(PREDICATES)
        }
        gpu_inputs = {
            predicate: temporary / f"{index}.gpu.in"
            for index, predicate in enumerate(PREDICATES)
        }
        cpu_outputs = {
            predicate: temporary / f"{index}.cpu.out"
            for index, predicate in enumerate(PREDICATES)
        }
        gpu_outputs = {
            predicate: temporary / f"{index}.gpu.out"
            for index, predicate in enumerate(PREDICATES)
        }
        cpu_streams = {predicate: path.open("wb") for predicate, path in cpu_inputs.items()}
        gpu_streams = {predicate: path.open("wb") for predicate, path in gpu_inputs.items()}
        try:
            for case in phase2a.cases_for_range(seed, begin, end, stride):
                validate_gpu_case(case)
                command = case.command()
                encoded = (command + "\n").encode("ascii")
                cpu_streams[case.predicate].write(encoded)
                gpu_streams[case.predicate].write(
                    f"{replay_id(campaign_name, case)} {command}\n".encode("ascii")
                )
                predicate_counts[case.predicate] += 1
            for stream in (*cpu_streams.values(), *gpu_streams.values()):
                stream.flush()
                os.fsync(stream.fileno())
        finally:
            for stream in (*cpu_streams.values(), *gpu_streams.values()):
                stream.close()

        for predicate in PREDICATES:
            if predicate_counts[predicate] == 0:
                fail("a bounded campaign chunk lost a predicate")
            run_process(
                cpu_replay,
                ("--multiprecision-only", "--decision-only", "--batch"),
                cpu_inputs[predicate],
                cpu_outputs[predicate],
                timeout_seconds,
                f"{predicate} exact CPU replay",
            )
            run_process(
                gpu_replay,
                (),
                gpu_inputs[predicate],
                gpu_outputs[predicate],
                timeout_seconds,
                f"{predicate} GPU replay",
            )

        cpu_readers = {predicate: path.open("rb") for predicate, path in cpu_outputs.items()}
        gpu_readers = {predicate: path.open("rb") for predicate, path in gpu_outputs.items()}
        gpu_summary_counts = {
            predicate: empty_gpu_summary_counts() for predicate in PREDICATES
        }
        try:
            current_base: Any | None = None
            current_base_sign: str | None = None
            for case in phase2a.cases_for_range(seed, begin, end, stride):
                expected_sign = phase2a.oracle_sign(case)
                if expected_sign not in SIGNS:
                    fail("the independent dyadic oracle escaped its closed sign enum")
                if case.relation == "base":
                    current_base = case
                    current_base_sign = expected_sign
                else:
                    if current_base is None or current_base_sign is None:
                        fail("a metamorphic case appeared without its generated base")
                    if case.relation not in METAMORPHISMS:
                        fail("a generated metamorphism escaped the frozen catalog")
                    try:
                        phase2a.validate_metamorphic_sign(
                            current_base,
                            case,
                            current_base_sign,
                            expected_sign,
                        )
                    except ValueError as error:
                        fail(
                            "a metamorphic sign relation contradicts its base: "
                            + canonical_json(
                                {
                                    "base_index": case.base_index,
                                    "base_replay_command": current_base.command(),
                                    "base_sign": current_base_sign,
                                    "campaign": campaign_name,
                                    "cause": str(error),
                                    "predicate": case.predicate,
                                    "relation": case.relation,
                                    "replay_command": case.command(),
                                    "replay_id": replay_id(campaign_name, case),
                                    "transformed_sign": expected_sign,
                                }
                            )
                        )
                    relation = phase2a.expected_metamorphic_relation(
                        current_base_sign,
                        case.predicate,
                        case.relation,
                    )
                    if relation not in METAMORPHIC_RELATIONS:
                        fail("a generated metamorphic relation escaped the frozen catalog")
                    counts["by_metamorphism"][case.relation] += 1
                    counts["by_metamorphic_relation"][relation] += 1
                    counts["metamorphisms_verified"] += 1
                cpu_row, cpu_encoded = read_canonical_line(
                    cpu_readers[case.predicate], f"{case.predicate} exact CPU replay"
                )
                gpu_row, gpu_encoded = read_canonical_line(
                    gpu_readers[case.predicate], f"{case.predicate} GPU replay"
                )
                validate_cpu_row(cpu_row, case, campaign_name, expected_sign)
                validate_gpu_row(gpu_row, case, campaign_name, expected_sign)
                update_gpu_summary_counts(gpu_summary_counts[case.predicate], gpu_row)
                command_row = (case.command() + "\n").encode("ascii")
                oracle_row = phase2a.canonical_bytes(
                    {
                        "base_index": case.base_index,
                        "predicate": case.predicate,
                        "relation": case.relation,
                        "sign": expected_sign,
                        "stratum": case.stratum,
                    }
                )
                roots_after["corpus"] = phase2a.extend_ordered_root(
                    "corpus", roots_after["corpus"], command_row
                )
                roots_after["oracle"] = phase2a.extend_ordered_root(
                    "oracle", roots_after["oracle"], oracle_row
                )
                roots_after["cpu"] = extend_root("cpu", roots_after["cpu"], cpu_encoded)
                roots_after["gpu"] = extend_root("gpu", roots_after["gpu"], gpu_encoded)
                counts["total_sign_count"] += 1
                counts["by_predicate"][case.predicate] += 1
                counts["by_sign"][expected_sign] += 1
                counts["cpu_multiprecision_certified"] += 1
                counts["exact_zeros"] += expected_sign == "zero"
                if case.relation == "base":
                    counts["base_case_count"] += 1
                else:
                    counts["metamorphic_sign_count"] += 1
                if gpu_row["gpu_filter_sign"] == "unknown":
                    counts["gpu_unknown_forwarded"] += 1
                    counts["gpu_unknown_by_cpu_stage"][
                        gpu_row["certification_stage"]
                    ] += 1
                else:
                    counts["gpu_fp64_certified"] += 1
                if cpu_row["sign"] != gpu_row["sign"] or gpu_row["sign"] != expected_sign:
                    counts["wrong_sign"] += 1

            for predicate in PREDICATES:
                summary, _ = read_canonical_line(
                    gpu_readers[predicate], f"{predicate} GPU summary"
                )
                validate_gpu_summary(summary, predicate, gpu_summary_counts[predicate])
                if cpu_readers[predicate].read(1):
                    fail(f"{predicate} exact CPU replay emitted extra rows")
                if gpu_readers[predicate].read(1):
                    fail(f"{predicate} GPU replay emitted extra rows")
        finally:
            for stream in (*cpu_readers.values(), *gpu_readers.values()):
                stream.close()

    validate_counts(counts, "completed chunk counts")
    return {
        "base_begin": begin,
        "base_end": end,
        "campaign": campaign_name,
        "counts": counts,
        "kind": CHUNK_KIND,
        "ordered_roots_after": roots_after,
        "ordered_roots_before": roots_before,
        "predicate_counts": predicate_counts,
        "schema_version": SCHEMA_VERSION,
    }


def chunk_path(checkpoint_dir: Path, index: int) -> Path:
    return checkpoint_dir / "chunks" / f"chunk-{index:08d}.json"


def artifact_reference(path: Path, checkpoint_dir: Path, content: bytes) -> dict[str, Any]:
    return {
        "relative_path": path.relative_to(checkpoint_dir).as_posix(),
        "sha256": sha256_bytes(content),
        "size_bytes": len(content),
    }


def initialize_manifest(
    *,
    campaign_name: str,
    seed: int,
    base_case_count: int,
    chunk_size: int,
    scope: str,
    identities: dict[str, Any],
    snapshot: dict[str, Any],
    phase2a: ModuleType,
) -> dict[str, Any]:
    return {
        "base_case_count": base_case_count,
        "campaign": campaign_name,
        "chunk_size_base_cases": chunk_size,
        "chunks": [],
        "counts": empty_counts(),
        "identities": identities,
        "kind": MANIFEST_KIND,
        "next_base_index": 0,
        "ordered_root_algorithm": ORDERED_ROOT_ALGORITHM,
        "ordered_roots": initial_roots(phase2a),
        "repository": snapshot,
        "runtime_milliseconds": 0,
        "schema_version": SCHEMA_VERSION,
        "scope": scope,
        "seed": f"{seed:016x}",
        "state": "running",
    }


def validate_checkpoint(
    checkpoint_dir: Path,
    manifest: Any,
    expected_identity: dict[str, Any],
    expected_initial_roots: dict[str, str],
) -> dict[str, Any]:
    if (
        not isinstance(manifest, dict)
        or frozenset(manifest) != MANIFEST_FIELDS
        or manifest.get("kind") != MANIFEST_KIND
    ):
        fail("the campaign checkpoint has an invalid kind")
    for field, expected in expected_identity.items():
        if manifest.get(field) != expected:
            fail(f"campaign checkpoint mismatch: {field}")
    if manifest["ordered_root_algorithm"] != ORDERED_ROOT_ALGORITHM:
        fail("the checkpoint ordered-root algorithm changed")
    exact_integer(manifest["runtime_milliseconds"], "checkpoint runtime")
    validate_counts(manifest.get("counts"), "checkpoint counts")
    roots = validate_roots(manifest.get("ordered_roots"), "checkpoint ordered roots")
    chunks = manifest.get("chunks")
    if not isinstance(chunks, list):
        fail("the campaign checkpoint chunks are invalid")
    previous_end = 0
    previous_roots = expected_initial_roots
    aggregate = empty_counts()
    for index, reference in enumerate(chunks):
        if not isinstance(reference, dict) or set(reference) != {
            "relative_path", "sha256", "size_bytes"
        }:
            fail("a checkpoint chunk reference is invalid")
        expected_relative = f"chunks/chunk-{index:08d}.json"
        if reference["relative_path"] != expected_relative:
            fail("a checkpoint chunk reference escaped its canonical path")
        path = checkpoint_dir / expected_relative
        chunk, content = load_canonical(path, f"campaign chunk {index}")
        if (
            reference["sha256"] != sha256_bytes(content)
            or reference["size_bytes"] != len(content)
        ):
            fail("a checkpoint chunk reference does not match its artifact")
        if (
            not isinstance(chunk, dict)
            or frozenset(chunk) != CHUNK_FIELDS
            or chunk.get("kind") != CHUNK_KIND
            or chunk.get("schema_version") != SCHEMA_VERSION
            or chunk.get("campaign") != manifest["campaign"]
        ):
            fail("a checkpoint chunk breaks the contiguous transaction chain")
        base_begin = exact_integer(chunk.get("base_begin"), "chunk base begin")
        base_end = exact_integer(chunk.get("base_end"), "chunk base end")
        if (
            base_begin != previous_end
            or base_end <= previous_end
            or base_end > manifest["base_case_count"]
        ):
            fail("a checkpoint chunk breaks the contiguous transaction chain")
        if chunk.get("ordered_roots_before") != previous_roots:
            fail("a checkpoint chunk breaks the ordered-root chain")
        validate_roots(chunk.get("ordered_roots_before"), "chunk roots before")
        validate_roots(chunk.get("ordered_roots_after"), "chunk roots after")
        predicate_counts = chunk.get("predicate_counts")
        if (
            not isinstance(predicate_counts, dict)
            or set(predicate_counts) != set(PREDICATES)
            or any(type(value) is not int or value <= 0 for value in predicate_counts.values())
        ):
            fail("a checkpoint chunk has invalid homogeneous-batch counts")
        chunk_counts = validate_counts(chunk.get("counts"), f"chunk {index} counts")
        if (
            chunk_counts["base_case_count"] != base_end - base_begin
            or sum(predicate_counts.values()) != chunk_counts["total_sign_count"]
            or predicate_counts != chunk_counts["by_predicate"]
        ):
            fail("a checkpoint chunk does not close over its generated range")
        previous_roots = chunk.get("ordered_roots_after")
        previous_end = base_end
        add_counts(aggregate, chunk_counts)
    if manifest.get("next_base_index") != previous_end:
        fail("the checkpoint next index differs from its chunk chain")
    exact_integer(manifest["next_base_index"], "checkpoint next base index")
    if manifest["next_base_index"] > manifest["base_case_count"]:
        fail("the checkpoint next index exceeds its campaign bound")
    if manifest["ordered_roots"] != previous_roots:
        fail("the checkpoint roots differ from its final chunk")
    if aggregate != manifest["counts"]:
        fail("the checkpoint aggregate differs from its chunks")
    if manifest["state"] not in ("running", "complete"):
        fail("the checkpoint has an invalid state")
    if (manifest["state"] == "complete") != (
        previous_end == manifest["base_case_count"]
    ):
        fail("the checkpoint state disagrees with its completed range")
    return manifest


def certificate_for(
    manifest: dict[str, Any],
    expected_phase2a_roots: dict[str, str | None],
) -> dict[str, Any]:
    if manifest["state"] != "complete":
        fail("an incomplete campaign cannot publish a certificate")
    counts = validate_counts(manifest["counts"], "certificate counts")
    if counts["base_case_count"] != manifest["base_case_count"]:
        fail("a completed certificate does not cover every configured base case")
    if manifest["scope"] == "production" and (
        counts["base_case_count"] != EXPECTED_PRODUCTION_BASE_CASES
        or counts["metamorphic_sign_count"]
        != EXPECTED_PRODUCTION_METAMORPHIC_SIGNS
        or counts["total_sign_count"] != EXPECTED_PRODUCTION_TOTAL_SIGNS
    ):
        fail("a production certificate does not cover the frozen base and metamorphic scale")
    if manifest["campaign"] == "phase2b" and manifest["scope"] == "production":
        if (
            counts["base_case_count"] < MINIMUM_ADDITIONAL_SIGNS
            or counts["total_sign_count"] < MINIMUM_ADDITIONAL_SIGNS
        ):
            fail("the independently seeded production corpus is below ten million signs")
    roots = manifest["ordered_roots"]
    frozen_roots_verified = None
    frozen_phase2a_corpus_distinct = None
    if manifest["campaign"] == "phase2a" and manifest["scope"] == "production":
        frozen_roots_verified = (
            roots["corpus"] == expected_phase2a_roots["corpus"]
            and roots["oracle"] == expected_phase2a_roots["oracle"]
        )
        if not frozen_roots_verified:
            fail("the Phase 2A GPU replay did not reproduce the frozen scientific roots")
    if manifest["campaign"] == "phase2b" and manifest["scope"] == "production":
        frozen_phase2a_corpus_distinct = (
            roots["corpus"] != expected_phase2a_roots["corpus"]
        )
        if not frozen_phase2a_corpus_distinct:
            fail("the independently seeded corpus reproduced the frozen Phase 2A root")
    return {
        "base_case_count": manifest["base_case_count"],
        "campaign": manifest["campaign"],
        "counts": manifest["counts"],
        "frozen_phase2a_roots_verified": frozen_roots_verified,
        "frozen_phase2a_corpus_distinct": frozen_phase2a_corpus_distinct,
        "generator_seed": manifest["seed"],
        "identities": manifest["identities"],
        "kind": CERTIFICATE_KIND,
        "ordered_root_algorithm": ORDERED_ROOT_ALGORITHM,
        "ordered_roots": roots,
        "phase_gate_closed": False,
        "qualification_claimed": False,
        "repository": manifest["repository"],
        "runtime_milliseconds": manifest["runtime_milliseconds"],
        "schema_version": SCHEMA_VERSION,
        "scope": manifest["scope"],
        "total_sign_count": manifest["counts"]["total_sign_count"],
    }


def publish_certificate(
    checkpoint_dir: Path,
    manifest: dict[str, Any],
    expected_phase2a_roots: dict[str, str | None],
) -> tuple[dict[str, Any], bytes]:
    certificate = certificate_for(manifest, expected_phase2a_roots)
    content = canonical_bytes(certificate)
    atomic_publish(checkpoint_dir / "result.json", content)
    return certificate, content


def run_shard(
    *,
    campaign_name: str,
    seed: int,
    base_case_count: int,
    chunk_size: int,
    max_chunks: int,
    timeout_seconds: int,
    root: Path,
    gpu_replay: Path,
    cpu_replay: Path,
    identities: dict[str, Any],
    snapshot: dict[str, Any],
    scope: str,
    phase2a: ModuleType,
    expected_phase2a_roots: dict[str, str | None],
    repository: Path,
    generator_path: Path,
    config_path: Path,
) -> tuple[tuple[dict[str, Any], bytes] | None, int]:
    prepare_shard_directory(root)
    manifest_path = root / "checkpoint.json"
    identity = {
        "base_case_count": base_case_count,
        "campaign": campaign_name,
        "chunk_size_base_cases": chunk_size,
        "identities": identities,
        "repository": snapshot,
        "schema_version": SCHEMA_VERSION,
        "scope": scope,
        "seed": f"{seed:016x}",
    }
    if manifest_path.exists():
        manifest, _ = load_canonical(manifest_path, f"{campaign_name} checkpoint")
        manifest = validate_checkpoint(root, manifest, identity, initial_roots(phase2a))
        validate_shard_layout(root, manifest)
    else:
        validate_shard_layout(root, None)
        manifest = initialize_manifest(
            campaign_name=campaign_name,
            seed=seed,
            base_case_count=base_case_count,
            chunk_size=chunk_size,
            scope=scope,
            identities=identities,
            snapshot=snapshot,
            phase2a=phase2a,
        )
        atomic_publish(manifest_path, canonical_bytes(manifest))
    if manifest["state"] == "complete":
        return publish_certificate(root, manifest, expected_phase2a_roots), 0

    started = time.monotonic()
    transactions = 0
    while manifest["next_base_index"] < base_case_count and transactions < max_chunks:
        verify_runtime_inputs(
            repository=repository,
            snapshot=snapshot,
            gpu_replay=gpu_replay,
            cpu_replay=cpu_replay,
            generator_path=generator_path,
            config_path=config_path,
            identities=identities,
        )
        begin = manifest["next_base_index"]
        end = min(begin + chunk_size, base_case_count)
        chunk = execute_chunk(
            campaign_name=campaign_name,
            seed=seed,
            stride=identities["metamorphic_stride"],
            begin=begin,
            end=end,
            gpu_replay=gpu_replay,
            cpu_replay=cpu_replay,
            checkpoint_dir=root,
            timeout_seconds=timeout_seconds,
            roots_before=manifest["ordered_roots"],
            phase2a=phase2a,
        )
        verify_runtime_inputs(
            repository=repository,
            snapshot=snapshot,
            gpu_replay=gpu_replay,
            cpu_replay=cpu_replay,
            generator_path=generator_path,
            config_path=config_path,
            identities=identities,
        )
        content = canonical_bytes(chunk)
        path = chunk_path(root, len(manifest["chunks"]))
        atomic_publish(path, content)
        manifest["chunks"].append(artifact_reference(path, root, content))
        add_counts(manifest["counts"], chunk["counts"])
        manifest["next_base_index"] = end
        manifest["ordered_roots"] = chunk["ordered_roots_after"]
        manifest["runtime_milliseconds"] += max(
            0, int((time.monotonic() - started) * 1000)
        )
        started = time.monotonic()
        if end == base_case_count:
            manifest["state"] = "complete"
        atomic_publish(manifest_path, canonical_bytes(manifest))
        transactions += 1
    if manifest["state"] != "complete":
        return None, transactions
    return publish_certificate(root, manifest, expected_phase2a_roots), transactions


def verify_shard_copy(
    *,
    campaign_name: str,
    seed: int,
    base_case_count: int,
    chunk_size: int,
    root: Path,
    gpu_replay: Path,
    cpu_replay: Path,
    identities: dict[str, Any],
    snapshot: dict[str, Any],
    scope: str,
    phase2a: ModuleType,
    expected_phase2a_roots: dict[str, str | None],
    repository: Path,
    generator_path: Path,
    config_path: Path,
) -> tuple[dict[str, Any], bytes]:
    if root.is_symlink() or not root.is_dir():
        fail(f"the {campaign_name} verification shard is not a physical directory")
    verify_runtime_inputs(
        repository=repository,
        snapshot=snapshot,
        gpu_replay=gpu_replay,
        cpu_replay=cpu_replay,
        generator_path=generator_path,
        config_path=config_path,
        identities=identities,
    )
    identity = {
        "base_case_count": base_case_count,
        "campaign": campaign_name,
        "chunk_size_base_cases": chunk_size,
        "identities": identities,
        "repository": snapshot,
        "schema_version": SCHEMA_VERSION,
        "scope": scope,
        "seed": f"{seed:016x}",
    }
    manifest, _ = load_canonical(
        root / "checkpoint.json", f"{campaign_name} verification checkpoint"
    )
    manifest = validate_checkpoint(root, manifest, identity, initial_roots(phase2a))
    validate_shard_layout(root, manifest)
    if manifest["state"] != "complete":
        fail("a read-only verification requires a complete campaign shard")
    expected_certificate = certificate_for(manifest, expected_phase2a_roots)
    certificate, content = load_canonical(
        root / "result.json", f"{campaign_name} verification certificate"
    )
    if certificate != expected_certificate:
        fail(f"the {campaign_name} certificate differs from its checkpoint")
    verify_runtime_inputs(
        repository=repository,
        snapshot=snapshot,
        gpu_replay=gpu_replay,
        cpu_replay=cpu_replay,
        generator_path=generator_path,
        config_path=config_path,
        identities=identities,
    )
    return certificate, content


def aggregate_for(
    certificates: dict[str, tuple[dict[str, Any], bytes]],
    scope: str,
) -> dict[str, Any]:
    if not certificates:
        fail("an empty campaign set cannot publish an aggregate")
    names = set(certificates)
    if not names.issubset(CAMPAIGNS):
        fail("a campaign aggregate escaped the frozen campaign catalog")
    if len(certificates) == 2:
        if names != set(CAMPAIGNS):
            fail("a two-campaign aggregate escaped the frozen campaign catalog")
        phase2a = certificates["phase2a"][0]
        phase2b = certificates["phase2b"][0]
        if phase2a.get("identities") != phase2b.get("identities"):
            fail("the campaign certificates do not share runtime identities")
        if phase2a.get("repository") != phase2b.get("repository"):
            fail("the campaign certificates do not share repository provenance")
    for name, (certificate, _) in certificates.items():
        if certificate.get("campaign") != name or certificate.get("scope") != scope:
            fail("a campaign certificate escaped its aggregate scope")
    references = {
        name: {
            "relative_path": f"{name}/result.json",
            "sha256": sha256_bytes(content),
            "size_bytes": len(content),
        }
        for name, (_, content) in certificates.items()
    }
    total = sum(value[0]["total_sign_count"] for value in certificates.values())
    roots = {name: value[0]["ordered_roots"] for name, value in certificates.items()}
    if len(certificates) == 2 and roots["phase2a"]["corpus"] == roots["phase2b"]["corpus"]:
        fail("the Phase 2B additional campaign did not produce a distinct corpus root")
    return {
        "additional_campaign_basis": (
            "independent_seed_and_distinct_corpus_root"
            if len(certificates) == 2
            else "single_campaign_only"
        ),
        "campaigns": references,
        "kind": AGGREGATE_KIND,
        "ordered_roots": roots,
        "phase_gate_closed": False,
        "qualification_claimed": False,
        "schema_version": SCHEMA_VERSION,
        "scope": scope,
        "total_sign_count": total,
    }


def publish_aggregate(
    checkpoint_dir: Path,
    certificates: dict[str, tuple[dict[str, Any], bytes]],
    scope: str,
) -> dict[str, Any]:
    aggregate = aggregate_for(certificates, scope)
    atomic_publish(checkpoint_dir / "result.json", canonical_bytes(aggregate))
    return aggregate


def bounded_positive(value: str) -> int:
    try:
        parsed = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("must be an integer") from error
    if parsed <= 0:
        raise argparse.ArgumentTypeError("must be positive")
    return parsed


def main(arguments: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("gpu_replay", type=Path)
    parser.add_argument("cpu_replay", type=Path)
    parser.add_argument("checkpoint_dir", type=Path)
    parser.add_argument("--campaign", choices=(*CAMPAIGNS, "both"), default="both")
    parser.add_argument(
        "--chunk-size-base-cases",
        type=bounded_positive,
        default=DEFAULT_CHUNK_BASE_CASES,
    )
    parser.add_argument("--max-chunks", type=bounded_positive, default=sys.maxsize)
    parser.add_argument("--timeout-seconds", type=bounded_positive, default=900)
    parser.add_argument(
        "--smoke-cases-per-predicate",
        type=bounded_positive,
        help="run a bounded non-production corpus instead of the frozen scale",
    )
    parser.add_argument(
        "--verify-only",
        action="store_true",
        help="validate a quiescent complete checkpoint copy without writing to it",
    )
    parser.add_argument("--allow-dirty-repository", action="store_true")
    parser.add_argument(
        "--repository",
        type=Path,
        default=Path(__file__).resolve().parents[2],
    )
    options = parser.parse_args(arguments)
    try:
        gpu_replay = options.gpu_replay.resolve()
        cpu_replay = options.cpu_replay.resolve()
        checkpoint_dir = options.checkpoint_dir.resolve()
        repository = options.repository.resolve()
        validate_executable(gpu_replay, "GPU predicate replay")
        validate_executable(cpu_replay, "CPU predicate replay")
        if options.chunk_size_base_cases > MAXIMUM_CHUNK_BASE_CASES:
            fail(
                f"chunk size exceeds the closed {MAXIMUM_CHUNK_BASE_CASES}-base-case bound"
            )
        try:
            checkpoint_dir.relative_to(repository)
        except ValueError:
            pass
        else:
            fail("the checkpoint directory must stay outside the measured repository")

        generator_path = repository / "morsehgp3d/tools/predicate_campaign.py"
        config_path = repository / "morsehgp3d/tests/campaigns/phase2a_predicates_v1.json"
        phase2a = load_phase2a_tool(generator_path)
        config_value, config_content = load_canonical(config_path, "frozen Phase 2A config")
        config = phase2a.validate_config(config_value, require_expected_roots=True)
        snapshot = repository_snapshot(repository)
        scope = "smoke" if options.smoke_cases_per_predicate is not None else "production"
        if scope == "production" and not snapshot["repository_clean"]:
            fail("a production campaign refuses a dirty repository")
        if options.allow_dirty_repository and scope != "smoke":
            fail("--allow-dirty-repository is restricted to smoke campaigns")
        if (
            scope == "smoke"
            and not options.allow_dirty_repository
            and not snapshot["repository_clean"]
        ):
            fail("a dirty smoke campaign requires --allow-dirty-repository")
        cases_per_predicate = (
            options.smoke_cases_per_predicate
            if options.smoke_cases_per_predicate is not None
            else DEFAULT_CASES_PER_PREDICATE
        )
        base_case_count = 3 * cases_per_predicate
        if scope == "production":
            if base_case_count != config["base_case_count"]:
                fail("the production base-case count differs from the frozen Phase 2A scale")
            if base_case_count < MINIMUM_ADDITIONAL_SIGNS:
                fail("the Phase 2B additional campaign is below ten million base signs")
        if (
            options.chunk_size_base_cases < 3
            or options.chunk_size_base_cases % len(PREDICATES) != 0
        ):
            fail("the chunk size must be a positive multiple of three base cases")
        identities = {
            "config_sha256": sha256_bytes(config_content),
            "cpu_replay_sha256": sha256_file(cpu_replay),
            "generator_sha256": sha256_file(generator_path),
            "gpu_replay_sha256": sha256_file(gpu_replay),
            "metamorphic_stride": config["metamorphisms"]["stride"],
            "tool_sha256": sha256_file(Path(__file__).resolve()),
        }
        expected_phase2a_roots = {
            "corpus": config["expected_corpus_sha256"],
            "oracle": config["expected_oracle_sha256"],
        }
        selected = CAMPAIGNS if options.campaign == "both" else (options.campaign,)
        seeds = {
            "phase2a": int(config["generator"]["seed"], 16),
            "phase2b": PHASE2B_SEED,
        }
        if seeds["phase2a"] == seeds["phase2b"]:
            fail("the Phase 2B campaign seed is not independent from Phase 2A")
        if options.verify_only:
            if checkpoint_dir.is_symlink() or not checkpoint_dir.is_dir():
                fail("read-only verification requires a physical checkpoint directory")
            certificates: dict[str, tuple[dict[str, Any], bytes]] = {}
            for campaign_name in selected:
                certificates[campaign_name] = verify_shard_copy(
                    campaign_name=campaign_name,
                    seed=seeds[campaign_name],
                    base_case_count=base_case_count,
                    chunk_size=options.chunk_size_base_cases,
                    root=checkpoint_dir / campaign_name,
                    gpu_replay=gpu_replay,
                    cpu_replay=cpu_replay,
                    identities=identities,
                    snapshot=snapshot,
                    scope=scope,
                    phase2a=phase2a,
                    expected_phase2a_roots=expected_phase2a_roots,
                    repository=repository,
                    generator_path=generator_path,
                    config_path=config_path,
                )
            expected_aggregate = aggregate_for(certificates, scope)
            aggregate, _ = load_canonical(
                checkpoint_dir / "result.json", "campaign verification aggregate"
            )
            if aggregate != expected_aggregate:
                fail("the campaign aggregate differs from its certificates")
        else:
            checkpoint_dir.parent.mkdir(parents=True, exist_ok=True)
            checkpoint_dir.mkdir(exist_ok=True)
            lock_path = checkpoint_dir.parent / f".{checkpoint_dir.name}.lock"
            lock_descriptor = os.open(lock_path, os.O_CREAT | os.O_RDWR, 0o600)
            try:
                try:
                    fcntl.flock(lock_descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB)
                except BlockingIOError as error:
                    raise ValueError(
                        "another writer holds the POSIX campaign lock"
                    ) from error
                certificates = {}
                remaining_chunks = options.max_chunks
                for campaign_name in selected:
                    result, committed_chunks = run_shard(
                        campaign_name=campaign_name,
                        seed=seeds[campaign_name],
                        base_case_count=base_case_count,
                        chunk_size=options.chunk_size_base_cases,
                        max_chunks=remaining_chunks,
                        timeout_seconds=options.timeout_seconds,
                        root=checkpoint_dir / campaign_name,
                        gpu_replay=gpu_replay,
                        cpu_replay=cpu_replay,
                        identities=identities,
                        snapshot=snapshot,
                        scope=scope,
                        phase2a=phase2a,
                        expected_phase2a_roots=expected_phase2a_roots,
                        repository=repository,
                        generator_path=generator_path,
                        config_path=config_path,
                    )
                    remaining_chunks -= committed_chunks
                    if result is None:
                        break
                    certificates[campaign_name] = result
                    if remaining_chunks <= 0:
                        break
                if len(certificates) == len(selected):
                    aggregate = publish_aggregate(checkpoint_dir, certificates, scope)
                else:
                    aggregate = {
                        "campaigns_complete": sorted(certificates),
                        "kind": AGGREGATE_KIND,
                        "phase_gate_closed": False,
                        "qualification_claimed": False,
                        "schema_version": SCHEMA_VERSION,
                        "scope": scope,
                        "state": "running",
                    }
            finally:
                os.close(lock_descriptor)
    except (OSError, TypeError, ValueError, subprocess.TimeoutExpired) as error:
        print(f"Phase 2B GPU sign campaign failed closed: {error}", file=sys.stderr)
        return 1
    print(canonical_json(aggregate))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
