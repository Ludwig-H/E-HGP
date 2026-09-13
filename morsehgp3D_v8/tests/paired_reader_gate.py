#!/usr/bin/env python3
"""Exercise closed paired-campaign reads and reject incomplete/mismatched evidence."""

from __future__ import annotations

import argparse
import base64
from collections import Counter
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from typing import Any, Callable


ROOT = Path(__file__).resolve().parents[2]
BENCH = ROOT / "morsehgp3D_v8/bench"
RUNNER = BENCH / "run_p0_matrix.py"
READER = BENCH / "check_paired_campaign.py"
sys.path.insert(0, str(BENCH))

from paired_receipts import validate_axis, validate_batch  # noqa: E402
from run_p0_matrix import parse_result  # noqa: E402


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path: Path) -> dict[str, Any]:
    return parse_result(path.read_bytes())


def write(path: Path, value: dict[str, Any]) -> None:
    path.write_text(json.dumps(value, indent=2, allow_nan=False) + "\n")


def python_command(script: Path, arguments: list[str]) -> list[str]:
    flags = ["-B", "-O"] if sys.flags.optimize else ["-B"]
    return [sys.executable, *flags, str(script), *arguments]


def empty_matrix(campaign: Path, field: str) -> None:
    manifest = read(campaign / "MANIFEST.json")
    manifest[field] = 0 if field == "repeats" else []
    write(campaign / "MANIFEST.json", manifest)
    completion = read(campaign / "COMPLETION.json")
    completion.update(runs=0, attempts=0)
    write(campaign / "COMPLETION.json", completion)
    (campaign / "MEASURES.jsonl").write_bytes(b"")


def duplicate_matrix(campaign: Path, field: str) -> None:
    manifest = read(campaign / "MANIFEST.json")
    manifest[field].append(manifest[field][0])
    write(campaign / "MANIFEST.json", manifest)


def manifest_field(campaign: Path, field: str, value: Any) -> None:
    manifest = read(campaign / "MANIFEST.json")
    manifest[field] = value
    write(campaign / "MANIFEST.json", manifest)


def source_mutation(campaign: Path, mode: str) -> None:
    manifest = read(campaign / "MANIFEST.json")
    sources = manifest["source_sha256"]
    if mode == "empty":
        sources.clear()
    elif mode in ("CMakeLists.txt", "bench/p0_fixtures.hpp", "bench/paired_receipts.py",
                  "bench/batch_probe.cpp"):
        sources.pop(f"morsehgp3D_v8/{mode}")
    elif mode == "absolute_path":
        sources["/outside_repository/fake_source"] = "0" * 64
    elif mode == "relative_escape":
        sources["../outside_repository/fake_source"] = "0" * 64
    elif mode == "bad_hash":
        sources["morsehgp3D_v8/CMakeLists.txt"] = "invalid_sha256"
    else:
        raise RuntimeError(f"unknown source mutation {mode}")
    completion = read(campaign / "COMPLETION.json")
    completion["source_sha256_closing"] = sources
    # Keep both old flags true: mere self-consistency is not source coverage.
    write(campaign / "MANIFEST.json", manifest)
    write(campaign / "COMPLETION.json", completion)


def change_input_identity(campaign: Path) -> None:
    manifest = read(campaign / "MANIFEST.json")
    validate = validate_batch if manifest["probe_kind"] == "batch" else validate_axis
    rows = [parse_result(line) for line in (campaign / "MEASURES.jsonl").read_bytes().splitlines()]
    require(bool(rows), "identity mutant has no rows")
    original = rows[0]["result"]["input_fnv1a64_le_u16_xyz"]
    replacement = "1" if original != "1" else "2"
    for record in rows:
        record["result"]["input_fnv1a64_le_u16_xyz"] = replacement
        # Every individual receipt and both execution orders remain internally
        # consistent. Only cross-campaign input matching can reject this change.
        validate(record["result"], record["command"])
        raw = (json.dumps(record["result"], allow_nan=False) + "\n").encode("utf-8")
        record["stdout"] = raw.decode("utf-8")
        record["stdout_base64"] = base64.b64encode(raw).decode("ascii")
    (campaign / "MEASURES.jsonl").write_text(
        "".join(json.dumps(record, allow_nan=False) + "\n" for record in rows))


def provenance_field(campaign: Path, field: str, value: Any) -> None:
    """Explicit metadata mutant, not a claim to have built another executable.

    The independent auditor's two genuine Release/Debug builds supplied the
    original counterexample. This portable gate does not depend on that private
    build directory or relabel those sources as our current compilation.
    """
    manifest_field(campaign, field, value)
    if field == "probe_sha256":
        completion = read(campaign / "COMPLETION.json")
        completion["probe_sha256_closing"] = value
        write(campaign / "COMPLETION.json", completion)
        rows = [parse_result(line)
                for line in (campaign / "MEASURES.jsonl").read_bytes().splitlines()]
        for row in rows:
            row["probe_sha256_before"] = row["probe_sha256_after"] = value
        (campaign / "MEASURES.jsonl").write_text(
            "".join(json.dumps(row, allow_nan=False) + "\n" for row in rows))


def volatile_cpu_frequency(campaign: Path) -> None:
    manifest = read(campaign / "MANIFEST.json")
    original = manifest["cpuinfo"]
    kept = [line for line in original.splitlines()
            if line.split(":", 1)[0].strip() != "cpu MHz"]
    manifest["cpuinfo"] = "\n".join([*kept, "cpu MHz\t: 0.001"])
    require(manifest["cpuinfo"] != original, "CPU frequency control was not exercised")
    write(campaign / "MANIFEST.json", manifest)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--batch-probe", required=True, type=Path)
    parser.add_argument("--axis-probe", required=True, type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    probes = {"batch": args.batch_probe.resolve(), "axis": args.axis_probe.resolve()}
    original_pins = {path: digest(path) for path in (*probes.values(), RUNNER, READER,
                                                   BENCH / "paired_receipts.py")}
    stats: Counter[str] = Counter()
    with tempfile.TemporaryDirectory(prefix="mhgp8_paired_reader_gate_") as directory:
        temporary = Path(directory)
        binaries = {}
        for kind, source in probes.items():
            target = temporary / "bin" / kind
            target.mkdir(parents=True)
            actual = target / "probe"
            shutil.copy2(source, actual)
            shutil.copy2(source.parent / "CMakeCache.txt", target / "CMakeCache.txt")
            binaries[kind] = actual
        genuine = temporary / "genuine"
        genuine.mkdir()
        cases = (("batch_tubes", "batch", "tubes", 10),
                 ("axis_tubes", "axis", "tubes", 10),
                 ("axis_pool", "axis", "pool", 10),
                 ("batch_k5", "batch", "tubes", 5),
                 ("batch_repeat", "batch", "tubes", 10))
        for name, kind, strategy, kmax in cases:
            arguments = ["--probe", str(binaries[kind]), "--probe-kind", kind,
                         "--output", str(genuine / name), "--sizes", "8",
                         "--families", "grid", "--strategies", strategy,
                         "--kmax", str(kmax), "--s", "8", "--repeats", "1"]
            result = subprocess.run(python_command(RUNNER, arguments),
                                    capture_output=True, cwd=ROOT)
            require(result.returncode == 0 and not result.stderr,
                    f"positive reader campaign failed: {name}: {result.stderr!r}")
            completion = read(genuine / name / "COMPLETION.json")
            require(completion["status"] == "completed" and completion["runs"] == 2,
                    f"positive campaign was not completed: {name}")
            stats["genuine_campaigns"] += 1
            stats["genuine_measurements"] += completion["runs"]
        result = subprocess.run(python_command(READER, [str(genuine), "--summary"]),
                                capture_output=True, cwd=ROOT)
        require(result.returncode == 0 and not result.stderr,
                f"positive paired reader failed: {result.stderr!r}")
        summary = parse_result(result.stdout)
        require(summary["status"] == "passed" and summary["campaigns"] == 5 and
                summary["measurements"] == 10 and
                summary["configurations_including_order"] == len(summary["summary"]) == 8 and
                summary["full_contract_qualified"] is False and summary["gcp_used"] is False,
                "positive paired summary has the wrong scope/counts")
        provenances = summary["provenance_by_probe_kind"]
        require(set(provenances) == {"batch", "axis"} and
                provenances["batch"]["probe_sha256"] != provenances["axis"]["probe_sha256"] and
                provenances["batch"]["build_id"] != provenances["axis"]["build_id"] and
                provenances["batch"]["machine_id"] == provenances["axis"]["machine_id"] and
                summary["provenance_policy"]["cpuinfo_ignored_fields"] == ["cpu MHz"] and
                summary["provenance_policy"]["machine_identity_scope"] ==
                "recorded_metadata_not_unique_physical_host",
                "paired summary lost its build/machine provenance policy")
        for item in summary["summary"]:
            expected_repeats = 2 if item["kind"] == "batch" and item["kmax"] == 10 else 1
            require(item["provenance"] == provenances[item["kind"]] and
                    item["repeats"] == expected_repeats,
                    "summary repeated across incompatible provenance or lost its identity")
        stats["positive_reads"] += 1

        frequency_control = temporary / "frequency_control"
        shutil.copytree(genuine, frequency_control)
        volatile_cpu_frequency(frequency_control / "batch_tubes")
        result = subprocess.run(python_command(READER, [str(frequency_control), "--summary"]),
                                capture_output=True, cwd=ROOT)
        require(result.returncode == 0 and not result.stderr and parse_result(result.stdout) == summary,
                "volatile CPU MHz changed the stable machine identity")
        stats["positive_reads"] += 1
        stats["cpu_frequency_policy_controls"] += 1

        def reject(label: str, mutation: Callable[[Path], None],
                   error_fragment: str, campaign_name: str = "batch_tubes") -> None:
            target = temporary / f"mutant_{label}"
            shutil.copytree(genuine, target)
            mutation(target / campaign_name)
            result = subprocess.run(python_command(READER, [str(target)]),
                                    capture_output=True, cwd=ROOT)
            require(result.returncode == 1 and not result.stdout and
                    error_fragment in result.stderr.decode("utf-8", errors="replace"),
                    f"reader mutant accepted or rejected for wrong cause: {label}: "
                    f"{result.returncode}: {result.stdout!r}: {result.stderr!r}")
            stats["mutants_rejected"] += 1

        matrix_fields = ("families", "sizes", "orders", "strategies", "kmax",
                         "separations", "lanes")
        for field in (*matrix_fields, "repeats"):
            reject(f"empty_{field}", lambda path, field=field: empty_matrix(path, field), field)
            stats["empty_matrix_rejections"] += 1
        for field in matrix_fields:
            reject(f"duplicate_{field}",
                   lambda path, field=field: duplicate_matrix(path, field), "duplicate matrix")
            stats["duplicate_matrix_rejections"] += 1
        for field, value, message in (
            ("sizes", [True], "sizes"), ("kmax", [1.0], "kmax"),
            ("repeats", True, "repeats"), ("threads", True, "threads"),
            ("orders", ["axis-first"], "orders"),
            ("families", ["sheet_full"], "sheet_full"),
            ("schema", "foreign", "scope"), ("scope", "FULL", "scope"),
            ("receipt_validation_version", 2.0, "receipt_validation_version"),
        ):
            reject(f"manifest_{field}",
                   lambda path, field=field, value=value: manifest_field(path, field, value), message)
        for mode in ("empty", "CMakeLists.txt", "bench/p0_fixtures.hpp",
                     "bench/paired_receipts.py", "bench/batch_probe.cpp",
                     "absolute_path", "relative_escape", "bad_hash"):
            reject(f"sources_{mode.replace('/', '_')}",
                   lambda path, mode=mode: source_mutation(path, mode),
                   "SHA256" if mode == "bad_hash" else "source coverage")
            stats["source_coverage_rejections"] += 1
        for name in ("axis_tubes", "axis_pool", "batch_k5"):
            reject(f"identity_{name}", change_input_identity, "point identities changed", name)
            stats["cross_campaign_identity_rejections"] += 1

        original_manifest = read(genuine / "batch_tubes/MANIFEST.json")
        build_mutants = {
            "probe_sha256": "0" * 64,
            "compiler_version": original_manifest["compiler_version"] + "\nmutant compiler\n",
            "cmake_cache": original_manifest["cmake_cache"] + "\nMUTANT_FLAGS:STRING=-O0\n",
        }
        machine_mutants = {
            "cpuinfo": original_manifest["cpuinfo"] + "\nmutant model name: other CPU\n",
            "platform": original_manifest["platform"] + "-other-system",
            "logical_cpu_count": original_manifest["logical_cpu_count"] + 1,
        }
        for field, value in {**build_mutants, **machine_mutants}.items():
            message = "heterogeneous build provenance" if field in build_mutants else \
                      "heterogeneous recorded machine provenance"
            reject(f"mixed_provenance_{field}",
                   lambda path, field=field, value=value: provenance_field(path, field, value), message)
            stats["heterogeneous_provenance_rejections"] += 1
        for field, value in (("compiler_version", None), ("cmake_cache", ""),
                             ("cpuinfo", "cpu MHz: 1000\n"), ("platform", ""),
                             ("logical_cpu_count", True)):
            reject(f"invalid_provenance_{field}",
                   lambda path, field=field, value=value: provenance_field(path, field, value), field)
            stats["invalid_provenance_rejections"] += 1

    require(stats["genuine_campaigns"] == 5 and stats["genuine_measurements"] == 10 and
            stats["positive_reads"] == 2 and stats["mutants_rejected"] == 46 and
            stats["empty_matrix_rejections"] == 8 and
            stats["duplicate_matrix_rejections"] == 7 and
            stats["source_coverage_rejections"] == 8 and
            stats["cross_campaign_identity_rejections"] == 3 and
            stats["cpu_frequency_policy_controls"] == 1 and
            stats["heterogeneous_provenance_rejections"] == 6 and
            stats["invalid_provenance_rejections"] == 5,
            "paired reader non-vacuity floor")
    require(all(digest(path) == pin for path, pin in original_pins.items()),
            "reader gate modified its original probes or protocol sources")
    print(json.dumps({"status": "passed", **dict(stats),
                      "python_optimized": bool(sys.flags.optimize),
                      "scope": "paired_campaign_reader_not_geometry_or_full"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
