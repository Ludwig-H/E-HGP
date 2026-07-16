#!/usr/bin/env python3
"""Smoke-test the frozen phase-2A predicate campaign and its transactions."""

from __future__ import annotations

import argparse
import fcntl
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
from fractions import Fraction
from pathlib import Path
from types import ModuleType
from typing import Any, Sequence

EXPECTED_CONFIG_FIELDS = {
    "base_case_count",
    "chunk_size_base_cases",
    "expected_corpus_sha256",
    "expected_oracle_sha256",
    "generator",
    "kind",
    "metamorphisms",
    "native_interface",
    "oracle",
    "ordered_root_algorithm",
    "power_witness_denominators",
    "predicate_cycle",
    "schema_version",
    "strata",
}
EXPECTED_CERTIFICATE_FIELDS = {
    "base_case_count",
    "build_metadata",
    "build_metadata_sha256",
    "catalog_sha256",
    "config_sha256",
    "corpus_descriptor_sha256",
    "corpus_sha256",
    "counts",
    "diagnostics",
    "expected_corpus_sha256",
    "expected_oracle_sha256",
    "frozen_config_sha256",
    "gcp_used",
    "git_head",
    "kind",
    "native_executable_sha256",
    "oracle_descriptor_sha256",
    "oracle_sha256",
    "ordered_root_algorithm",
    "output_sha256",
    "repository_clean",
    "repository_end_verified",
    "repository_phase_exit_claimed",
    "schema_version",
    "scientific_summary_sha256",
    "scope",
    "tool_sha256",
    "total_sign_count",
}
EXPECTED_BUILD_METADATA_FIELDS = {
    "build_type",
    "cmake_cache_path",
    "cmake_cache_sha256",
    "cmake_generator",
    "compiler_path",
    "compiler_version",
    "compiler_version_sha256",
    "cxx_flags",
    "cxx_flags_for_build_type",
    "target_flags",
    "target_flags_path",
    "target_flags_sha256",
}


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def canonical_bytes(value: object) -> bytes:
    return (canonical_json(value) + "\n").encode("utf-8")


def sha256_bytes(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def load_module(path: Path) -> ModuleType:
    spec = importlib.util.spec_from_file_location("phase2a_predicate_campaign", path)
    if spec is None or spec.loader is None:
        raise AssertionError("the campaign module cannot be loaded")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def run(
    command: Sequence[str],
    *,
    timeout: int = 120,
    expected_status: int = 0,
    extra_env: dict[str, str] | None = None,
) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        list(command),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        encoding="utf-8",
        timeout=timeout,
        env={
            **os.environ,
            "PYTHONDONTWRITEBYTECODE": "1",
            **(extra_env or {}),
        },
    )
    if completed.returncode != expected_status:
        raise AssertionError(
            f"command returned {completed.returncode}, expected {expected_status}: "
            f"{command!r}\nstdout={completed.stdout!r}\nstderr={completed.stderr!r}"
        )
    if expected_status == 0 and completed.stderr:
        raise AssertionError(
            f"successful campaign emitted stderr: {completed.stderr!r}"
        )
    if expected_status != 0 and (
        completed.stdout
        or not completed.stderr.startswith("predicate campaign failed closed:")
    ):
        raise AssertionError("campaign failure was not closed")
    return completed


def git(repository: Path, *arguments: str) -> str:
    completed = subprocess.run(
        ["git", "-C", str(repository), *arguments],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        encoding="utf-8",
    )
    if completed.returncode != 0:
        raise AssertionError(f"temporary Git command failed: {completed.stderr!r}")
    return completed.stdout


def make_repository(path: Path) -> None:
    path.mkdir()
    git(path, "init", "--quiet")
    git(path, "config", "user.email", "phase2a-smoke@example.invalid")
    git(path, "config", "user.name", "Phase 2A smoke")
    marker = path / "marker.txt"
    marker.write_text("committed\n", encoding="utf-8")
    git(path, "add", "marker.txt")
    git(path, "commit", "--quiet", "-m", "smoke provenance")
    marker.write_text("dirty smoke\n", encoding="utf-8")


def fraction_from_word(word: str) -> Fraction:
    import struct

    return Fraction.from_float(struct.unpack(">d", bytes.fromhex(word))[0])


def fraction_oracle(case: Any) -> Fraction:
    points = tuple(
        tuple(fraction_from_word(word) for word in point) for point in case.points
    )
    if case.predicate == "compare_squared_distances":
        witness, left, right = points
        return sum(((a - b) ** 2 for a, b in zip(witness, left)), Fraction()) - sum(
            ((a - b) ** 2 for a, b in zip(witness, right)), Fraction()
        )
    if case.predicate == "orientation_3d":
        a, b, c, d = points
        u = tuple(bi - ai for ai, bi in zip(a, b))
        v = tuple(ci - ai for ai, ci in zip(a, c))
        w = tuple(di - ai for ai, di in zip(a, d))
        return (
            u[0] * (v[1] * w[2] - v[2] * w[1])
            - u[1] * (v[0] * w[2] - v[2] * w[0])
            + u[2] * (v[0] * w[1] - v[1] * w[0])
        )
    if case.witness is None:
        raise AssertionError("a generated H_RQ case has no witness")
    witness = tuple(
        Fraction(numerator, case.witness[3]) for numerator in case.witness[:3]
    )
    r_points = tuple(points[index] for index in case.r_ids)
    q_points = tuple(points[index] for index in case.q_ids)
    coordinate_delta = tuple(
        sum((point[axis] for point in r_points), Fraction())
        - sum((point[axis] for point in q_points), Fraction())
        for axis in range(3)
    )
    norm_delta = sum(
        (
            sum((coordinate * coordinate for coordinate in point), Fraction())
            for point in r_points
        ),
        Fraction(),
    ) - sum(
        (
            sum((coordinate * coordinate for coordinate in point), Fraction())
            for point in q_points
        ),
        Fraction(),
    )
    return (
        -2
        * sum(
            (
                coordinate * delta
                for coordinate, delta in zip(witness, coordinate_delta)
            ),
            Fraction(),
        )
        + norm_delta
    )


def fraction_from_dyadic(value: tuple[int, int]) -> Fraction:
    numerator, exponent = value
    return (
        Fraction(numerator << exponent, 1)
        if exponent >= 0
        else Fraction(numerator, 1 << -exponent)
    )


def audit_generator(module: ModuleType, config: dict[str, Any]) -> dict[str, Any]:
    if set(config) != EXPECTED_CONFIG_FIELDS:
        raise AssertionError("the frozen campaign config schema is open")
    if config["base_case_count"] < 10_800_000:
        raise AssertionError("the frozen campaign does not contain 10.8M base signs")
    if config["base_case_count"] * 93 // 100 < 10_000_000:
        raise AssertionError("the random 93% stratum is below ten million signs")
    if not 1 <= config["chunk_size_base_cases"] <= 9_600:
        raise AssertionError("the frozen campaign chunk is not safely bounded")
    if sum(entry["percent"] for entry in config["strata"]) != 100:
        raise AssertionError("the frozen stratum percentages do not close")
    configured_roots = (
        config["expected_corpus_sha256"],
        config["expected_oracle_sha256"],
    )
    if any(root is None for root in configured_roots) and not all(
        root is None for root in configured_roots
    ):
        raise AssertionError("the frozen scientific roots are only partially populated")
    for root in configured_roots:
        if root is not None:
            if not isinstance(root, str) or len(root) != 64:
                raise AssertionError("a frozen scientific root is not SHA-256")
            int(root, 16)
    if config["ordered_root_algorithm"] != module.ORDERED_ROOT_ALGORITHM:
        raise AssertionError("the config omits its ordered-root algorithm")

    original_splitmix64 = module.splitmix64
    module.splitmix64 = lambda _seed, counter: counter
    try:
        if (
            module.counter_value(0, 0, 127) != 127
            or module.counter_value(0, 1, 0) != 128
        ):
            raise AssertionError("the RNG counter domains overlap between base cases")
        for invalid_lane in (-1, 128):
            try:
                module.counter_value(0, 0, invalid_lane)
            except ValueError:
                pass
            else:
                raise AssertionError(
                    "the RNG accepted a lane outside its closed domain"
                )
    finally:
        module.splitmix64 = original_splitmix64

    for unsafe_path in ("../escape.json", "/absolute.json", "chunks\\escape.json"):
        try:
            module.validate_reference(
                {
                    "relative_path": unsafe_path,
                    "sha256": "0" * 64,
                    "size_bytes": 1,
                },
                "injected reference",
            )
        except ValueError:
            pass
        else:
            raise AssertionError(
                f"unsafe artifact reference was accepted: {unsafe_path}"
            )

    malformed_counts = module.empty_counts()
    malformed_counts["predicate_strata"][module.PREDICATES[0]][module.STRATA[0]] = 1
    try:
        module.validate_counts(malformed_counts, "injected row mismatch")
    except ValueError:
        pass
    else:
        raise AssertionError("predicate/stratum row mismatch was accepted")
    malformed_counts = module.empty_counts()
    malformed_counts["by_metamorphism"][module.METAMORPHISMS[0]] = 1
    try:
        module.validate_counts(malformed_counts, "injected metamorphism mismatch")
    except ValueError:
        pass
    else:
        raise AssertionError("metamorphism total mismatch was accepted")

    seed = int(config["generator"]["seed"], 16)
    stride = config["metamorphisms"]["stride"]
    observed_strata = {name: 0 for name in module.STRATA}
    power_cardinalities: set[int] = set()
    power_overlaps: set[int] = set()
    power_denominators: set[int] = set()
    metamorphisms = {name: 0 for name in module.METAMORPHISMS}
    metamorphism_count = 0
    for base_index in range(600):
        case = module.generate_base_case(seed, base_index)
        observed_strata[case.stratum] += 1
        exact_fraction = fraction_oracle(case)
        expected_oracle = exact_fraction
        if case.predicate == "power_bisector_side" and case.witness is not None:
            expected_oracle *= case.witness[3]
        if fraction_from_dyadic(module.oracle_value(case)) != expected_oracle:
            raise AssertionError(
                f"integer-dyadic oracle differs from Fraction at base {base_index}"
            )
        if case.stratum == "exact_equalities" and exact_fraction != 0:
            raise AssertionError("the exact-equality stratum is not exactly zero")
        if case.stratum in module.STRATA[1:-1] and exact_fraction == 0:
            raise AssertionError(f"hard stratum became vacuous: {case.stratum}")
        words_for_case = tuple(word for point in case.points for word in point)
        if case.stratum == "subnormal" and not any(
            0 < (int(word, 16) & ((1 << 63) - 1)) < (1 << 52) for word in words_for_case
        ):
            raise AssertionError("the subnormal stratum has no subnormal operand")
        if case.stratum == "large_offsets" and not any(
            abs(fraction_from_word(word)) >= 1 << 40 for word in words_for_case
        ):
            raise AssertionError("the large-offset stratum lost its offset")
        if case.predicate == "power_bisector_side":
            power_cardinalities.add(len(case.r_ids))
            power_overlaps.add(len(set(case.r_ids) & set(case.q_ids)))
            power_denominators.add(case.witness[3])
        if module.should_metamorph(base_index, case.stratum, stride):
            metamorphism = module.metamorphism_for_base(
                base_index, case.stratum, stride
            )
            transformed = module.metamorphic_case(case, seed, metamorphism)
            expected = (
                -exact_fraction
                if metamorphism == module.PREDICATE_SYMMETRY
                or (
                    metamorphism == module.IMPROPER_REFLECTION
                    and case.predicate == "orientation_3d"
                )
                else exact_fraction
            )
            if metamorphism == module.POWER_OF_TWO_SCALING:
                exponent = 1 + module.counter_value(seed, base_index, 124) % 3
                degree = 3 if case.predicate == "orientation_3d" else 2
                expected *= 1 << (degree * exponent)
            if fraction_oracle(transformed) != expected:
                raise AssertionError(f"metamorphism changed Fraction: {metamorphism}")
            metamorphisms[metamorphism] += 1
            metamorphism_count += 1
    if any(count == 0 for count in observed_strata.values()):
        raise AssertionError("the smoke prefix omits a frozen stratum")
    if power_cardinalities != set(range(1, 11)) or len(power_overlaps) < 5:
        raise AssertionError("the H_RQ catalog omits cardinalities or overlaps")
    if power_denominators != set(module.POWER_WITNESS_DENOMINATORS):
        raise AssertionError("the H_RQ corpus omits witness denominators")
    if any(count == 0 for count in metamorphisms.values()):
        raise AssertionError("the smoke prefix omits a metamorphism type")
    words = (module.ONE, module.TWO, module.NEGATIVE_ONE, module.MINIMUM_SUBNORMAL)
    minimized = module.greedy_minimize_words(
        words, lambda candidate: candidate[-1] == module.MINIMUM_SUBNORMAL
    )
    if minimized != (
        module.ZERO,
        module.ZERO,
        module.ZERO,
        module.MINIMUM_SUBNORMAL,
    ):
        raise AssertionError("the injected helper divergence was not minimized")
    return {
        "closed_counter_domain_checks": 4,
        "closed_reference_checks": 3,
        "closed_structural_count_checks": 2,
        "fraction_oracle_case_count": 600,
        "metamorphism_case_count": metamorphism_count,
        "metamorphism_counts": metamorphisms,
        "power_cardinalities": sorted(power_cardinalities),
        "power_denominators": sorted(power_denominators),
        "power_overlap_pattern_count": len(power_overlaps),
    }


def campaign_command(
    tool: Path,
    native: Path,
    config: Path,
    checkpoint: Path,
    repository: Path,
    *,
    max_chunks: int,
    smoke_chunk_size: int,
    smoke_base_case_count: int = 900,
    cmake_cache: Path | None = None,
) -> list[str]:
    command = [
        sys.executable,
        str(tool),
        "--native",
        str(native),
        "--config",
        str(config),
        "--checkpoint-dir",
        str(checkpoint),
        "--repository",
        str(repository),
        "--max-chunks",
        str(max_chunks),
        "--timeout-seconds",
        "120",
        "--smoke-base-case-count",
        str(smoke_base_case_count),
        "--smoke-chunk-size",
        str(smoke_chunk_size),
        "--allow-dirty-repository",
    ]
    if cmake_cache is not None:
        command.extend(["--cmake-cache", str(cmake_cache)])
    return command


def precompute_command(
    tool: Path,
    config: Path,
    output: Path,
    repository: Path,
    base_case_count: int,
) -> list[str]:
    return [
        sys.executable,
        str(tool),
        "--config",
        str(config),
        "--repository",
        str(repository),
        "--precompute-roots",
        str(output),
        "--smoke-base-case-count",
        str(base_case_count),
        "--allow-dirty-repository",
    ]


def materialize_rooted_config(
    source: Path, roots_artifact: Path, destination: Path
) -> dict[str, Any]:
    config, _ = load_canonical(source)
    roots, _ = load_canonical(roots_artifact)
    config["expected_corpus_sha256"] = roots["corpus_sha256"]
    config["expected_oracle_sha256"] = roots["oracle_sha256"]
    destination.write_bytes(canonical_bytes(config))
    return config


def load_canonical(path: Path) -> tuple[dict[str, Any], bytes]:
    content = path.read_bytes()
    value = json.loads(content)
    if not isinstance(value, dict) or content != canonical_bytes(value):
        raise AssertionError(f"artifact is not closed canonical JSON: {path}")
    return value, content


def audit_scientific_roots(
    module: ModuleType,
    config: dict[str, Any],
    native: Path,
    checkpoint: Path,
    certificate: dict[str, Any],
) -> None:
    manifest, _ = load_canonical(checkpoint / "checkpoint.json")
    seed = int(config["generator"]["seed"], 16)
    stride = config["metamorphisms"]["stride"]
    roots = module.initial_ordered_roots()
    for reference in manifest["chunks"]:
        chunk, _ = load_canonical(checkpoint / reference["relative_path"])
        cases = tuple(
            module.cases_for_range(seed, chunk["base_begin"], chunk["base_end"], stride)
        )
        corpus = "".join(f"{case.command()}\n" for case in cases).encode("utf-8")
        oracle_rows = tuple(
            canonical_bytes(
                {
                    "base_index": case.base_index,
                    "predicate": case.predicate,
                    "relation": case.relation,
                    "sign": module.oracle_sign(case),
                    "stratum": case.stratum,
                }
            )
            for case in cases
        )
        completed = subprocess.run(
            [str(native), "--decision-only", "--batch"],
            input=corpus,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            timeout=120,
        )
        if completed.returncode != 0 or completed.stderr:
            raise AssertionError("root audit native replay failed")
        output_rows = completed.stdout.splitlines(keepends=True)
        if len(output_rows) != len(cases):
            raise AssertionError("root audit native replay changed row count")
        if sha256_bytes(corpus) != chunk["corpus_sha256"]:
            raise AssertionError("chunk corpus hash is not rooted in actual commands")
        if sha256_bytes(b"".join(oracle_rows)) != chunk["oracle_signs_sha256"]:
            raise AssertionError("chunk oracle hash is not rooted in actual signs")
        if sha256_bytes(completed.stdout) != chunk["native_output_sha256"]:
            raise AssertionError("chunk output hash is not rooted in native rows")
        for command, oracle_row, output_row in zip(
            corpus.splitlines(keepends=True), oracle_rows, output_rows
        ):
            roots["corpus"] = module.extend_ordered_root(
                "corpus", roots["corpus"], command
            )
            roots["oracle"] = module.extend_ordered_root(
                "oracle", roots["oracle"], oracle_row
            )
            roots["output"] = module.extend_ordered_root(
                "output", roots["output"], output_row
            )
        if roots != chunk["ordered_roots_after"]:
            raise AssertionError("chunk ordered roots do not close on actual rows")
    if (
        roots["corpus"] != certificate["corpus_sha256"]
        or roots["oracle"] != certificate["oracle_sha256"]
        or roots["output"] != certificate["output_sha256"]
    ):
        raise AssertionError("certificate roots differ from the ordered actual corpus")


def audit_checkpoint(
    module: ModuleType,
    config: dict[str, Any],
    native: Path,
    checkpoint: Path,
) -> tuple[dict[str, Any], bytes]:
    manifest, _ = load_canonical(checkpoint / "checkpoint.json")
    certificate, content = load_canonical(checkpoint / "result.json")
    if set(certificate) != EXPECTED_CERTIFICATE_FIELDS:
        raise AssertionError("the campaign certificate schema is open")
    if (
        certificate["base_case_count"] != 900
        or certificate["counts"]["base_case_count"] != 900
        or certificate["counts"]["wrong_sign"] != 0
        or certificate["counts"]["remaining_unknown"] != 0
        or certificate["repository_phase_exit_claimed"] is not False
        or certificate["gcp_used"] is not False
        or certificate["repository_end_verified"] is not True
        or certificate["scope"] != "smoke"
        or certificate["ordered_root_algorithm"] != module.ORDERED_ROOT_ALGORITHM
        or certificate["expected_corpus_sha256"] != certificate["corpus_sha256"]
        or certificate["expected_oracle_sha256"] != certificate["oracle_sha256"]
    ):
        raise AssertionError("the smoke certificate makes an invalid verdict")
    if set(certificate["diagnostics"]) != {
        "platform",
        "python",
        "runtime_milliseconds",
    } or not all(
        isinstance(certificate["diagnostics"][field], str)
        for field in ("platform", "python")
    ):
        raise AssertionError("the certificate diagnostics schema is open")
    counts = certificate["counts"]
    if any(counts["by_stratum"][stratum] == 0 for stratum in counts["by_stratum"]):
        raise AssertionError("the campaign certificate omits a stratum")
    if any(
        count == 0
        for row in counts["predicate_strata"].values()
        for count in row.values()
    ):
        raise AssertionError("the campaign certificate omits a predicate/stratum cell")
    if counts["metamorphic_sign_count"] == 0 or counts["total_sign_count"] <= 900:
        raise AssertionError(
            "metamorphic derivatives were counted as base signs or omitted"
        )
    if any(counts["by_metamorphism"][name] == 0 for name in module.METAMORPHISMS):
        raise AssertionError("the certificate omits a metamorphism type")
    if (
        sum(counts["by_metamorphic_relation"].values())
        != counts["metamorphic_sign_count"]
    ):
        raise AssertionError("the metamorphic relation counters do not close")
    build_metadata = certificate["build_metadata"]
    if set(build_metadata) != EXPECTED_BUILD_METADATA_FIELDS:
        raise AssertionError("the build metadata schema is open")
    if certificate["build_metadata_sha256"] != sha256_bytes(
        canonical_json(build_metadata).encode("utf-8")
    ):
        raise AssertionError("the build metadata hash is invalid")
    if build_metadata["target_flags_sha256"] != sha256_bytes(
        build_metadata["target_flags"].encode("utf-8")
    ):
        raise AssertionError("the effective target flags hash is invalid")
    for strict_flag in ("-fno-fast-math", "-ffp-contract=off", "-frounding-math"):
        if strict_flag not in build_metadata["target_flags"]:
            raise AssertionError(f"the native target omits strict flag {strict_flag}")
    summary = {"counts": counts, "remaining_unknown": 0, "wrong_sign": 0}
    if certificate["scientific_summary_sha256"] != sha256_bytes(
        canonical_json(summary).encode("utf-8")
    ):
        raise AssertionError("the scientific summary hash is invalid")
    reference = manifest["certificate_artifact"]
    artifact = checkpoint / reference["relative_path"]
    if artifact.read_bytes() != content:
        raise AssertionError(
            "result alias differs from the content-addressed certificate"
        )
    if (
        reference["sha256"] != sha256_bytes(content)
        or reference["sha256"] not in artifact.name
    ):
        raise AssertionError("certificate content address is invalid")
    for chunk_reference in manifest["chunks"]:
        chunk_path = checkpoint / chunk_reference["relative_path"]
        chunk_content = chunk_path.read_bytes()
        if (
            len(chunk_content) != chunk_reference["size_bytes"]
            or sha256_bytes(chunk_content) != chunk_reference["sha256"]
        ):
            raise AssertionError("a chunk differs from its committed hash")
    audit_scientific_roots(module, config, native, checkpoint, certificate)
    return certificate, content


def audit_failures(
    module: ModuleType,
    tool: Path,
    native: Path,
    config: Path,
    checkpoint: Path,
    repository: Path,
    temporary: Path,
) -> int:
    failure_count = 0
    dirty_production = [
        sys.executable,
        str(tool),
        "--native",
        str(native),
        "--config",
        str(config),
        "--checkpoint-dir",
        str(temporary / "dirty-production"),
        "--repository",
        str(repository),
        "--max-chunks",
        "0",
    ]
    completed = run(dirty_production, expected_status=2)
    if "refuses a dirty Git worktree" not in completed.stderr:
        raise AssertionError("dirty production worktree was not rejected")
    failure_count += 1

    corrupted = temporary / "corrupted"
    shutil.copytree(checkpoint, corrupted)
    manifest, _ = load_canonical(corrupted / "checkpoint.json")
    chunk_path = corrupted / manifest["chunks"][0]["relative_path"]
    chunk, _ = load_canonical(chunk_path)
    chunk["counts"]["wrong_sign"] = 1
    chunk_path.write_bytes(canonical_bytes(chunk))
    completed = run(
        campaign_command(
            tool,
            native,
            config,
            corrupted,
            repository,
            max_chunks=0,
            smoke_chunk_size=300,
        ),
        expected_status=2,
    )
    if "checkpoint hash" not in completed.stderr:
        raise AssertionError("corrupted chunk did not fail by committed hash")
    failure_count += 1

    mismatched_config = temporary / "mismatched-config.json"
    config_value, _ = load_canonical(config)
    config_value["generator"]["seed"] = "4d4f525345484751"
    mismatched_config.write_bytes(canonical_bytes(config_value))
    completed = run(
        campaign_command(
            tool,
            native,
            mismatched_config,
            checkpoint,
            repository,
            max_chunks=0,
            smoke_chunk_size=300,
        ),
        expected_status=2,
    )
    if "checkpoint mismatch" not in completed.stderr:
        raise AssertionError("config mismatch did not fail closed")
    failure_count += 1

    drifted_config = temporary / "drifted-roots.json"
    config_value, _ = load_canonical(config)
    root = config_value["expected_corpus_sha256"]
    config_value["expected_corpus_sha256"] = ("0" if root[0] != "0" else "1") + root[1:]
    drifted_config.write_bytes(canonical_bytes(config_value))
    completed = run(
        campaign_command(
            tool,
            native,
            drifted_config,
            temporary / "drifted-roots",
            repository,
            max_chunks=1,
            smoke_chunk_size=900,
        ),
        expected_status=2,
    )
    if "root drifted from its config" not in completed.stderr:
        raise AssertionError("precomputed corpus-root drift was not rejected")
    failure_count += 1

    unsafe = temporary / "unsafe-reference"
    shutil.copytree(checkpoint, unsafe)
    unsafe_manifest, _ = load_canonical(unsafe / "checkpoint.json")
    unsafe_manifest["chunks"][0]["relative_path"] = "chunks/../checkpoint.json"
    (unsafe / "checkpoint.json").write_bytes(canonical_bytes(unsafe_manifest))
    completed = run(
        campaign_command(
            tool,
            native,
            config,
            unsafe,
            repository,
            max_chunks=0,
            smoke_chunk_size=300,
        ),
        expected_status=2,
    )
    if "path is unsafe" not in completed.stderr:
        raise AssertionError("unsafe artifact traversal was not rejected")
    failure_count += 1

    source_cache = module.discover_cmake_cache(native, None)
    source_cache_values = module.cmake_cache_values(source_cache)
    source_flags, _ = module.discover_target_flags(
        source_cache, native, source_cache_values
    )
    mutable_build = temporary / "mutable-build"
    mutable_flags = mutable_build / "CMakeFiles" / f"{native.name}.dir" / "flags.make"
    mutable_flags.parent.mkdir(parents=True)
    shutil.copyfile(source_flags, mutable_flags)
    mutable_cache = mutable_build / "CMakeCache.txt"
    cache_text = source_cache.read_text(encoding="utf-8")
    original_cache_directory = source_cache_values["CMAKE_CACHEFILE_DIR"]
    cache_text = cache_text.replace(
        f"CMAKE_CACHEFILE_DIR:INTERNAL={original_cache_directory}",
        f"CMAKE_CACHEFILE_DIR:INTERNAL={mutable_build}",
    )
    mutable_cache.write_text(cache_text, encoding="utf-8")
    build_checkpoint = temporary / "build-provenance"
    run(
        campaign_command(
            tool,
            native,
            config,
            build_checkpoint,
            repository,
            max_chunks=1,
            smoke_chunk_size=300,
            cmake_cache=mutable_cache,
        )
    )
    with mutable_flags.open("a", encoding="utf-8") as stream:
        stream.write("# injected effective target flag drift\n")
    completed = run(
        campaign_command(
            tool,
            native,
            config,
            build_checkpoint,
            repository,
            max_chunks=1,
            smoke_chunk_size=300,
            cmake_cache=mutable_cache,
        ),
        expected_status=2,
    )
    if "build_metadata" not in completed.stderr:
        raise AssertionError("effective target flag drift was not rejected")
    failure_count += 1

    lock_path = checkpoint.parent / f".{checkpoint.name}.lock"
    descriptor = os.open(lock_path, os.O_CREAT | os.O_RDWR, 0o600)
    try:
        fcntl.flock(descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB)
        completed = run(
            campaign_command(
                tool,
                native,
                config,
                checkpoint,
                repository,
                max_chunks=0,
                smoke_chunk_size=300,
            ),
            expected_status=2,
        )
        if "POSIX campaign lock" not in completed.stderr:
            raise AssertionError("concurrent campaign writer was not rejected")
    finally:
        fcntl.flock(descriptor, fcntl.LOCK_UN)
        os.close(descriptor)
    failure_count += 1
    return failure_count


def audit_failpoints(
    tool: Path,
    native: Path,
    config: Path,
    repository: Path,
    temporary: Path,
) -> int:
    expectations = {
        "chunk_artifact": (0, "running", False),
        "chunk_manifest": (300, "running", False),
        "certificate_artifact": (300, "running", False),
        "complete_manifest": (300, "complete", False),
    }
    for failpoint, (cursor, state, alias_exists) in expectations.items():
        checkpoint = temporary / f"failpoint-{failpoint}"
        completed = run(
            campaign_command(
                tool,
                native,
                config,
                checkpoint,
                repository,
                max_chunks=1,
                smoke_chunk_size=300,
                smoke_base_case_count=300,
            ),
            expected_status=2,
            extra_env={
                "MORSEHGP3D_PHASE2A_CAMPAIGN_FAIL_AFTER": failpoint,
            },
        )
        if f"after {failpoint}" not in completed.stderr:
            raise AssertionError(f"failpoint did not fire: {failpoint}")
        manifest, _ = load_canonical(checkpoint / "checkpoint.json")
        if (
            manifest["next_base_index"] != cursor
            or manifest["state"] != state
            or (checkpoint / "result.json").exists() != alias_exists
        ):
            raise AssertionError(
                f"failpoint published a non-authoritative state: {failpoint}"
            )
        chunk_artifacts = sorted((checkpoint / "chunks").glob("chunk-*.json"))
        certificate_artifacts = sorted(
            (checkpoint / "certificates").glob("certificate-*.json")
        )
        expected_chunks = 0 if failpoint == "chunk_artifact" else 1
        expected_certificates = (
            1 if failpoint in ("certificate_artifact", "complete_manifest") else 0
        )
        if (
            len(chunk_artifacts) != 1
            or len(manifest["chunks"]) != expected_chunks
            or len(certificate_artifacts) != expected_certificates
            or (manifest["certificate_artifact"] is not None)
            != (failpoint == "complete_manifest")
        ):
            raise AssertionError(
                f"failpoint did not expose the expected artifact boundary: {failpoint}"
            )
        run(
            campaign_command(
                tool,
                native,
                config,
                checkpoint,
                repository,
                max_chunks=1,
                smoke_chunk_size=300,
                smoke_base_case_count=300,
            )
        )
        resumed, _ = load_canonical(checkpoint / "checkpoint.json")
        if resumed["state"] != "complete" or not (checkpoint / "result.json").is_file():
            raise AssertionError(f"failpoint did not resume exactly: {failpoint}")
    return len(expectations)


def make_faulty_native(
    directory: Path, real_native: Path, cmake_cache: Path, marker: str
) -> tuple[Path, Path]:
    build = directory / "faulty-build"
    build.mkdir()
    wrapper = build / "morsehgp3d_replay_predicate"
    wrapper.write_text(
        """#!/usr/bin/env python3
import json
import subprocess
import sys

REAL = %r
MARKER = %r
corpus = sys.stdin.buffer.read()
completed = subprocess.run([REAL, *sys.argv[1:]], input=corpus, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
if completed.returncode != 0 or completed.stderr:
    sys.stdout.buffer.write(completed.stdout)
    sys.stderr.buffer.write(completed.stderr)
    raise SystemExit(completed.returncode)
commands = corpus.decode("utf-8").splitlines()
rows = completed.stdout.decode("utf-8").splitlines()
if len(commands) != len(rows):
    raise SystemExit(3)
for command, row in zip(commands, rows):
    value = json.loads(row)
    if command.startswith("power_bisector_side ") and MARKER in command:
        value["sign"] = "positive" if value["sign"] != "positive" else "negative"
        value["counters"]["exact_zeros"] = 1 if value["sign"] == "zero" else 0
    print(json.dumps(value, allow_nan=False, ensure_ascii=False, separators=(",", ":"), sort_keys=True))
""" % (str(real_native), marker),
        encoding="utf-8",
    )
    wrapper.chmod(0o755)
    copied_cache = build / "CMakeCache.txt"
    shutil.copyfile(cmake_cache, copied_cache)
    return wrapper, copied_cache


def parse_power_command(
    command: str,
) -> tuple[
    tuple[int, int, int, int],
    tuple[str, ...],
    tuple[int, ...],
    tuple[int, ...],
]:
    tokens = command.split()
    if not tokens or tokens[0] != "power_bisector_side":
        raise AssertionError("the divergence repro is not H_RQ")
    witness = tuple(int(value) for value in tokens[1:5])
    point_count = int(tokens[5])
    cursor = 6 + 3 * point_count
    point_words = tuple(tokens[6:cursor])
    r_count = int(tokens[cursor])
    cursor += 1
    r_ids = tuple(int(value) for value in tokens[cursor : cursor + r_count])
    cursor += r_count
    q_count = int(tokens[cursor])
    cursor += 1
    q_ids = tuple(int(value) for value in tokens[cursor : cursor + q_count])
    cursor += q_count
    if cursor != len(tokens):
        raise AssertionError("the divergence repro has trailing H_RQ tokens")
    return witness, point_words, r_ids, q_ids  # type: ignore[return-value]


def audit_real_minimizer(
    module: ModuleType,
    tool: Path,
    native: Path,
    config: Path,
    repository: Path,
    temporary: Path,
) -> int:
    config_value, _ = load_canonical(config)
    seed = int(config_value["generator"]["seed"], 16)
    target = module.generate_base_case(seed, 29)
    marker = " ".join(target.points[0])
    source_cache = module.discover_cmake_cache(native, None)
    wrapper, wrapper_cache = make_faulty_native(temporary, native, source_cache, marker)
    checkpoint = temporary / "faulty-campaign"
    completed = run(
        campaign_command(
            tool,
            wrapper,
            config,
            checkpoint,
            repository,
            max_chunks=1,
            smoke_chunk_size=900,
            cmake_cache=wrapper_cache,
        ),
        expected_status=2,
    )
    if "wrong predicate sign" not in completed.stderr:
        raise AssertionError("the fake native did not inject a campaign divergence")
    originals = sorted((checkpoint / "repros").glob("original-*.json"))
    minimized_paths = sorted((checkpoint / "repros").glob("repro-*.json"))
    if len(originals) != 1 or len(minimized_paths) != 1:
        raise AssertionError(
            "the divergence did not publish original then minimized repro"
        )
    original, original_content = load_canonical(originals[0])
    minimized, _ = load_canonical(minimized_paths[0])
    if (
        original["kind"] != module.ORIGINAL_REPRO_KIND
        or minimized["kind"] != module.REPRO_KIND
    ):
        raise AssertionError("the divergence repro schemas changed identity")
    reference = minimized["original_artifact"]
    if (
        reference["sha256"] != sha256_bytes(original_content)
        or checkpoint / reference["relative_path"] != originals[0]
    ):
        raise AssertionError("the minimized repro lost its authoritative original")
    original_witness, original_words, original_r_ids, original_q_ids = (
        parse_power_command(original["command"])
    )
    minimized_witness, minimized_words, minimized_r_ids, minimized_q_ids = (
        parse_power_command(minimized["minimized"]["command"])
    )
    original_cardinality = len(original_r_ids)
    minimized_cardinality = len(minimized_r_ids)
    if original_cardinality != 10 or minimized_cardinality >= original_cardinality:
        raise AssertionError("the H_RQ minimizer did not shrink labels")
    if len(original_q_ids) != 10 or len(minimized_q_ids) >= len(original_q_ids):
        raise AssertionError("the H_RQ minimizer did not shrink both label lists")
    if minimized_witness != (0, 0, 0, 1) or minimized_witness == original_witness:
        raise AssertionError("the H_RQ minimizer did not shrink its witness")
    if not any(
        before != after and after == module.ZERO
        for before, after in zip(original_words, minimized_words)
    ):
        raise AssertionError("the divergence minimizer did not shrink coordinate bits")
    if len(minimized["minimized"]["command"]) >= len(original["command"]):
        raise AssertionError("the divergence minimizer did not shrink bits/constants")
    return 1


def positive(value: str) -> int:
    parsed = int(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("the timeout must be positive")
    return parsed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("campaign_tool", type=Path)
    parser.add_argument("native_replay", type=Path)
    parser.add_argument("config", type=Path)
    parser.add_argument("repository", type=Path)
    parser.add_argument("--timeout-seconds", type=positive, default=180)
    arguments = parser.parse_args()
    for path, label in (
        (arguments.campaign_tool, "campaign tool"),
        (arguments.native_replay, "native replay"),
        (arguments.config, "campaign config"),
    ):
        if not path.is_file():
            raise SystemExit(f"{label} does not exist: {path}")
    module = load_module(arguments.campaign_tool.resolve())
    config, _ = load_canonical(arguments.config.resolve())
    generator_summary = audit_generator(module, config)
    with tempfile.TemporaryDirectory(
        prefix="morsehgp3d-phase2a-campaign-"
    ) as directory:
        temporary = Path(directory)
        repository = temporary / "repository"
        make_repository(repository)
        roots_path = temporary / "roots-900.json"
        precomputed = json.loads(
            run(
                precompute_command(
                    arguments.campaign_tool.resolve(),
                    arguments.config.resolve(),
                    roots_path,
                    repository,
                    900,
                ),
                timeout=arguments.timeout_seconds,
            ).stdout
        )
        roots_artifact, _ = load_canonical(roots_path)
        direct_roots, direct_metamorphic_count = module.precompute_ordered_roots(
            config, 900
        )
        if (
            roots_artifact["kind"] != module.PRECOMPUTE_KIND
            or roots_artifact["scope"] != "smoke"
            or roots_artifact["corpus_sha256"] != direct_roots["corpus"]
            or roots_artifact["oracle_sha256"] != direct_roots["oracle"]
            or precomputed["metamorphic_sign_count"] != direct_metamorphic_count
        ):
            raise AssertionError("the closed precompute mode changed scientific roots")
        smoke_config_path = temporary / "rooted-config-900.json"
        smoke_config = materialize_rooted_config(
            arguments.config.resolve(), roots_path, smoke_config_path
        )

        roots_300_path = temporary / "roots-300.json"
        run(
            precompute_command(
                arguments.campaign_tool.resolve(),
                arguments.config.resolve(),
                roots_300_path,
                repository,
                300,
            ),
            timeout=arguments.timeout_seconds,
        )
        config_300_path = temporary / "rooted-config-300.json"
        materialize_rooted_config(
            arguments.config.resolve(), roots_300_path, config_300_path
        )

        first = temporary / "first"
        command = campaign_command(
            arguments.campaign_tool.resolve(),
            arguments.native_replay.resolve(),
            smoke_config_path,
            first,
            repository,
            max_chunks=1,
            smoke_chunk_size=300,
        )
        partial = json.loads(run(command, timeout=arguments.timeout_seconds).stdout)
        if partial != {
            "base_cases_committed": 300,
            "complete": False,
            "metamorphic_signs_committed": 30,
            "total_signs_committed": 330,
        }:
            raise AssertionError(f"unexpected first transaction: {partial!r}")
        resumed_command = campaign_command(
            arguments.campaign_tool.resolve(),
            arguments.native_replay.resolve(),
            smoke_config_path,
            first,
            repository,
            max_chunks=2,
            smoke_chunk_size=300,
        )
        completed = json.loads(
            run(resumed_command, timeout=arguments.timeout_seconds).stdout
        )
        if completed["base_cases_committed"] != 900 or not completed["complete"]:
            raise AssertionError("the bounded resume did not finish the smoke corpus")
        certificate, content = audit_checkpoint(
            module, smoke_config, arguments.native_replay.resolve(), first
        )
        (first / "result.json").unlink()
        idempotent = run(
            campaign_command(
                arguments.campaign_tool.resolve(),
                arguments.native_replay.resolve(),
                smoke_config_path,
                first,
                repository,
                max_chunks=0,
                smoke_chunk_size=300,
            ),
            timeout=arguments.timeout_seconds,
        )
        if (
            json.loads(idempotent.stdout) != completed
            or (first / "result.json").read_bytes() != content
        ):
            raise AssertionError(
                "completed campaign resume did not heal its alias idempotently"
            )

        second = temporary / "second"
        run(
            campaign_command(
                arguments.campaign_tool.resolve(),
                arguments.native_replay.resolve(),
                smoke_config_path,
                second,
                repository,
                max_chunks=2,
                smoke_chunk_size=450,
            ),
            timeout=arguments.timeout_seconds,
        )
        second_certificate, _ = audit_checkpoint(
            module, smoke_config, arguments.native_replay.resolve(), second
        )
        if (
            certificate["counts"] != second_certificate["counts"]
            or certificate["corpus_sha256"] != second_certificate["corpus_sha256"]
            or certificate["output_sha256"] != second_certificate["output_sha256"]
        ):
            raise AssertionError("scientific identity depends on chunk boundaries")
        failure_count = audit_failures(
            module,
            arguments.campaign_tool.resolve(),
            arguments.native_replay.resolve(),
            smoke_config_path,
            first,
            repository,
            temporary,
        )
        failpoint_count = audit_failpoints(
            arguments.campaign_tool.resolve(),
            arguments.native_replay.resolve(),
            config_300_path,
            repository,
            temporary,
        )
        minimizer_count = audit_real_minimizer(
            module,
            arguments.campaign_tool.resolve(),
            arguments.native_replay.resolve(),
            smoke_config_path,
            repository,
            temporary,
        )
    print(
        canonical_json(
            {
                **generator_summary,
                "base_case_count": 900,
                "closed_failure_count": failure_count,
                "failpoint_count": failpoint_count,
                "metamorphic_sign_count": certificate["counts"][
                    "metamorphic_sign_count"
                ],
                "predicate_strata_cells": 24,
                "real_minimizer_injection_count": minimizer_count,
                "resume_transaction_count": 3,
                "total_native_sign_count": certificate["total_sign_count"] * 2,
                "wrong_sign": 0,
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
