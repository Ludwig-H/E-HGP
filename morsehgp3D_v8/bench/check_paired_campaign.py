#!/usr/bin/env python3
"""Recheck paired mono receipts, including work, matrices and source pins."""

from __future__ import annotations

import argparse
import base64
import hashlib
import itertools
import json
import math
import re
import statistics
from collections import defaultdict
from pathlib import Path

from paired_receipts import stable_signature, validate_axis, validate_batch
from run_p0_matrix import digest, parse_result, require, uint


PROVENANCE_POLICY = {
    "version": "paired_provenance_v1",
    "grouping": "homogeneous_build_per_probe_kind_and_common_machine_metadata",
    "cpuinfo_ignored_fields": ["cpu MHz"],
    "machine_identity_scope": "recorded_metadata_not_unique_physical_host",
}


def identity_hash(value: dict) -> str:
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":"),
                         ensure_ascii=False, allow_nan=False).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def provenance(manifest: dict) -> tuple[dict, dict, dict]:
    """Compare full build metadata; ignore only the volatile CPU frequency line.

    Raw cpuinfo remains in every manifest. Its MHz reading changes on one
    machine between runs, so it cannot define an identity of repetitions.
    Equal metadata is not a unique physical-host certificate.
    """
    for field in ("compiler_version", "cmake_cache", "cpuinfo", "platform"):
        require(type(manifest.get(field)) is str and bool(manifest[field].strip()),
                f"{field}: missing provenance metadata")
    cpuinfo = "\n".join(line for line in manifest["cpuinfo"].splitlines()
                        if line.split(":", 1)[0].strip() != "cpu MHz")
    require(bool(cpuinfo.strip()), "cpuinfo: empty normalized provenance")
    cpu_count = uint(manifest.get("logical_cpu_count"), "logical_cpu_count")
    require(cpu_count > 0, "logical_cpu_count: missing machine provenance")
    build = {"probe_sha256": manifest["probe_sha256"],
             "compiler_version": manifest["compiler_version"],
             "cmake_cache": manifest["cmake_cache"]}
    machine = {"cpuinfo": cpuinfo, "platform": manifest["platform"],
               "logical_cpu_count": cpu_count, "threads": manifest["threads"]}
    published = {"build_id": identity_hash(build), "machine_id": identity_hash(machine),
                 "probe_sha256": build["probe_sha256"],
                 "compiler_version": build["compiler_version"],
                 "cmake_cache_sha256": hashlib.sha256(build["cmake_cache"].encode()).hexdigest(),
                 "cpuinfo_normalized_sha256": hashlib.sha256(cpuinfo.encode()).hexdigest(),
                 "platform": machine["platform"], "logical_cpu_count": cpu_count,
                 "threads": machine["threads"]}
    return build, machine, published


def expected_sources(root: Path, kind: str) -> set[str]:
    """The producer's declared source perimeter, not a manifest-selected subset."""
    source = root / "morsehgp3D_v8"
    paths = [source / "CMakeLists.txt", *sorted((source / "src").rglob("*.hpp")),
             *sorted((source / "src").rglob("*.cpp")),
             *sorted((source / "bench").glob("*.hpp")), source / "bench/p0_probe.cpp",
             source / "bench/run_p0_matrix.py", source / f"bench/{kind}_probe.cpp",
             source / "bench/paired_receipts.py"]
    return {str(path.relative_to(root)) for path in paths}


def sha256(value: object, name: str) -> None:
    require(type(value) is str and re.fullmatch(r"[0-9a-f]{64}", value) is not None,
            f"{name}: expected lowercase SHA256")


def validate_manifest(manifest: dict, root: Path) -> int:
    """Reject empty/duplicate/mistyped matrices before constructing their product."""
    kind = manifest.get("probe_kind")
    require(kind in ("batch", "axis"), "not a paired campaign")
    scope = ("single_rectangle_three_lane_credit_batch" if kind == "batch"
             else "single_rectangle_axis_q2_residual")
    require(manifest.get("schema") == "mhgp8_p0_campaign_v1" and
            manifest.get("scope") == scope and
            manifest.get("public_status") == "not_claimed" and
            manifest.get("gcp_used") is False and manifest.get("downstream_measured") is False and
            uint(manifest.get("threads"), "threads") == 1 and
            uint(manifest.get("receipt_validation_version"), "receipt_validation_version") == 2,
            "campaign scope changed")
    require(type(manifest.get("probe")) is str and bool(manifest["probe"]),
            "missing probe path")
    sha256(manifest.get("probe_sha256"), "probe_sha256")
    repeats = uint(manifest.get("repeats"), "repeats")
    require(repeats >= 1, "repeats must be positive")
    choices = {
        "families": {"grid", "sheet", "sheet_full", "skew", "tube", "rails"},
        "strategies": {"pool", "dual", "tubes"},
        "orders": {"baseline-first", "batch-first" if kind == "batch" else "axis-first"},
    }
    for field, permitted in choices.items():
        values = manifest.get(field)
        require(type(values) is list and bool(values) and
                all(type(value) is str and value in permitted for value in values),
                f"{field}: empty or invalid matrix values")
        require(len(set(values)) == len(values), f"{field}: duplicate matrix values")
    require(kind == "axis" or "sheet_full" not in manifest["families"],
            "sheet_full belongs to the axis probe")
    for field in ("sizes", "kmax", "separations", "lanes"):
        values = manifest.get(field)
        require(type(values) is list and bool(values), f"{field}: empty matrix values")
        for value in values:
            uint(value, field)
            if field == "sizes":
                require(value >= 2, "sizes must be at least two")
            elif field == "kmax":
                require(1 <= value <= 10, "kmax must be in [1,10]")
            elif field == "separations":
                require(value >= 1, "separations must be positive")
            else:
                require(value in (2, 3, 4), "invalid geometric lane")
        require(len(set(values)) == len(values), f"{field}: duplicate matrix values")
    sources = manifest.get("source_sha256")
    require(type(sources) is dict and set(sources) == expected_sources(root, kind),
            "incomplete or unexpected source coverage")
    for name, value in sources.items():
        require(type(name) is str and not Path(name).is_absolute() and
                ".." not in Path(name).parts and
                (root / name).resolve().is_relative_to(root.resolve()),
                "source path escapes the repository")
        sha256(value, name)
    return repeats * math.prod(len(manifest[field]) for field in (
        "families", "kmax", "separations", "orders", "sizes", "strategies"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("receipt", type=Path)
    parser.add_argument("--summary", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    manifests = sorted(args.receipt.glob("*/MANIFEST.json"))
    require(bool(manifests), "no paired campaigns")
    groups = defaultdict(list)
    signatures = {}
    identities = {}
    builds = {}
    published_provenance = {}
    common_machine = None
    attempts = 0
    for path in manifests:
        manifest = parse_result(path.read_bytes())
        completion = parse_result((path.parent / "COMPLETION.json").read_bytes())
        expected_count = validate_manifest(manifest, root)
        kind = manifest["probe_kind"]
        build, machine, published = provenance(manifest)
        require(kind not in builds or builds[kind] == build,
                "heterogeneous build provenance within one probe kind")
        require(common_machine is None or common_machine == machine,
                "heterogeneous recorded machine provenance")
        builds[kind] = build
        common_machine = machine
        published_provenance[kind] = published
        require(completion["status"] == "completed" and
                completion["source_hashes_unchanged"] is True and
                completion["probe_hash_unchanged"] is True and
                completion["probe_sha256_closing"] == manifest["probe_sha256"] and
                completion["source_sha256_closing"] == manifest["source_sha256"],
                "incomplete campaign or changed source/binary")
        raw_rows = (path.parent / "MEASURES.jsonl").read_bytes().splitlines()
        require(uint(completion.get("runs"), "runs") == expected_count and
                uint(completion.get("attempts"), "attempts") == expected_count and
                len(raw_rows) == expected_count and expected_count > 0,
                "empty or incomplete matrix")
        for name, sha in manifest["source_sha256"].items():
            require(digest(root / name) == sha, f"source differs from captured version: {name}")
        expected = set(itertools.product(manifest["families"], manifest["kmax"],
                       manifest["separations"], manifest["orders"], manifest["sizes"],
                       manifest["strategies"], range(manifest["repeats"])))
        seen = set()
        for raw in raw_rows:
            record = parse_result(raw)
            require(record["status"] == "completed" and record["exit_code"] == 0 and
                    record["stderr"] == record["stderr_base64"] == "" and
                    record["probe_sha256_before"] == record["probe_sha256_after"] ==
                    manifest["probe_sha256"], "invalid invocation")
            stdout = base64.b64decode(record["stdout_base64"], validate=True)
            row = parse_result(stdout)
            require(stdout.decode("utf-8") == record["stdout"] and row == record["result"],
                    "raw and parsed receipts differ")
            command = [manifest["probe"], str(row["n"]), row["strategy"], row["family"],
                       str(row["kmax"]), str(row["separation_s"]), row["order"]]
            require(command == record["command"], "stored command differs from tuple")
            (validate_batch if kind == "batch" else validate_axis)(row, command)
            input_key = (row["family"], row["n"])
            fingerprint = row["input_fnv1a64_le_u16_xyz"]
            require(input_key not in identities or identities[input_key] == fingerprint,
                    "point identities changed across strategies, K or probe kinds")
            identities[input_key] = fingerprint
            uint(record.get("repeat"), "repeat")
            identity = (row["family"], row["kmax"], row["separation_s"], row["order"],
                        row["n"], row["strategy"], record["repeat"])
            require(identity not in seen, "duplicate invocation")
            seen.add(identity)
            signature = stable_signature(row, kind)
            stable_key = (kind, row["family"], row["n"], row["kmax"], row["strategy"])
            require(stable_key not in signatures or signatures[stable_key] == signature,
                    "work/identity/residual changed across execution order, repetitions or s")
            signatures[stable_key] = signature
            key = (kind, row["family"], row["n"], row["kmax"], row["separation_s"],
                   row["strategy"], row["order"])
            groups[key].append(row)
            attempts += 1
        require(seen == expected and len(seen) == completion["runs"] == completion["attempts"],
                "missing/extra matrix tuples")
    result = {"status": "passed", "campaigns": len(manifests), "measurements": attempts,
              "configurations_including_order": len(groups), "scope": "paired_components_only",
              "full_contract_qualified": False, "gcp_used": False,
              "provenance_policy": PROVENANCE_POLICY,
              "provenance_by_probe_kind": published_provenance}
    if args.summary:
        summary = []
        for key, rows in sorted(groups.items()):
            kind = key[0]
            item = dict(zip(("kind", "family", "n", "kmax", "s", "strategy", "order"), key))
            item["repeats"] = len(rows)
            item["provenance"] = published_provenance[kind]
            time_fields = ("prepare_ms", "baseline_shared_owner_ms", "batch_ms",
                           "baseline_total_ms", "batch_total_ms") if kind == "batch" else (
                           "prepare_ms", "baseline_ms", "axis_ms", "baseline_total_ms", "axis_total_ms")
            for field in time_fields:
                item[f"median_{field}"] = statistics.median(row[field] for row in rows)
            if kind == "batch":
                item["shared_work"] = rows[0]["shared_work"]
                item["lanes"] = rows[0]["lanes"]
            else:
                for field in ("axis_candidates", "baseline_candidates", "axis_descriptors", "axis_work"):
                    item[field] = rows[0][field]
            summary.append(item)
        result["summary"] = summary
    print(json.dumps(result, indent=2, allow_nan=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
