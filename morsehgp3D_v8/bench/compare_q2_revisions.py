#!/usr/bin/env python3
"""Read-only q2 comparison: pinned f481 baseline versus current source pins."""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from contextlib import redirect_stdout
import hashlib
import importlib.util
import io
import json
from pathlib import Path, PurePosixPath
import subprocess
import sys
from typing import Any

# The reader does not create bytecode caches even when invoked without -B.
sys.dont_write_bytecode = True
import run_q2_census_matrix as reader


ROOT = Path(__file__).resolve().parents[2]
BASELINE_REVISION = "f4815cd42d572db6aef27ec73d100f52303fff26"
READER_PATH = ROOT / reader.RUNNER_SOURCE
SCALING_WORK = ("count_node_visits", "query_cover_visits", "query_tasks", "payload_node_visits",
                "payload_interior_sites", "payload_shell_sites")


def baseline_sources() -> dict[str, str]:
    """Read immutable git objects, not a manifest-selected source perimeter."""
    listing = subprocess.check_output(
        ["git", "ls-tree", "-r", "--name-only", BASELINE_REVISION, "--", "morsehgp3D_v8"], cwd=ROOT)
    fixed = {"morsehgp3D_v8/CMakeLists.txt", reader.RUNNER_SOURCE,
             "morsehgp3D_v8/bench/q2_census_probe.cpp", "morsehgp3D_v8/bench/run_p0_matrix.py",
             "morsehgp3D_v8/bench/paired_receipts.py", "morsehgp3D_v8/bench/check_paired_campaign.py"}
    paths = set()
    for name in listing.decode("utf-8").splitlines():
        path = PurePosixPath(name)
        if (name in fixed or
                (name.startswith("morsehgp3D_v8/src/") and path.suffix in (".hpp", ".cpp")) or
                (str(path.parent) == "morsehgp3D_v8/bench" and path.suffix == ".hpp")):
            paths.add(name)
    reader.require(fixed <= paths, "pinned baseline lacks required source files")
    return {name: hashlib.sha256(subprocess.check_output(
        ["git", "show", f"{BASELINE_REVISION}:{name}"], cwd=ROOT)).hexdigest() for name in sorted(paths)}


def compile_profile(manifest: dict[str, Any]) -> dict[str, str]:
    profile = {}
    for line in manifest["cmake_cache"].splitlines():
        if "=" not in line or ":" not in line.split("=", 1)[0]:
            continue
        key = line.split(":", 1)[0]
        if key in ("CMAKE_BUILD_TYPE", "MHGP8_SANITIZE") or key.startswith((
                "CMAKE_CXX_FLAGS", "CMAKE_EXE_LINKER_FLAGS", "CMAKE_STATIC_LINKER_FLAGS",
                "CMAKE_SHARED_LINKER_FLAGS", "CMAKE_MODULE_LINKER_FLAGS",
                "CMAKE_INTERPROCEDURAL_OPTIMIZATION")):
            profile[key] = line.split("=", 1)[1]
    reader.require({"CMAKE_BUILD_TYPE", "CMAKE_CXX_FLAGS", "MHGP8_SANITIZE"} <= set(profile),
                   "missing compiler configuration for revision comparison")
    return profile


def read_revision(receipt: Path, expected: dict[str, str]) -> dict[str, Any]:
    """Reuse the full checker in an isolated module; its capture API is not called.

    Only the expected source map is injected. Strict equality here deliberately
    disallows even the reader's separately documented historical-runner exception.
    """
    directories = reader.campaign_directories(receipt)
    evidence = {directory / name: reader.digest(directory / name)
                for directory in directories for name in reader.RECEIPT_FILES}
    manifests = [reader.parse_result((directory / "MANIFEST.json").read_bytes()) for directory in directories]
    for manifest in manifests:
        reader.require(manifest.get("source_sha256") == expected,
                       "revision source coverage/hash differs from its explicitly pinned version")
    spec = importlib.util.spec_from_file_location("_mhgp8_isolated_q2_revision_reader", READER_PATH)
    reader.require(spec is not None and spec.loader is not None, "cannot load isolated q2 reader")
    isolated = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(isolated)
    isolated.sources = lambda: dict(expected)
    output = io.StringIO()
    with redirect_stdout(output):
        code = isolated.check(argparse.Namespace(receipt=receipt, summary=True))
    reader.require(code == 0, "q2 revision reader failed")
    checked = reader.parse_result(output.getvalue().encode("utf-8"))
    rows = defaultdict(list)
    for directory in directories:
        for line in (directory / "MEASURES.jsonl").read_bytes().splitlines():
            row = reader.parse_result(line)["result"]
            rows[tuple(row[key] for key in reader.KEYS)].append(row)
    reader.require(all(reader.digest(path) == pin for path, pin in evidence.items()),
                   "revision evidence changed during its read-only check")
    return {"checked": checked, "rows": dict(rows), "manifest": manifests[0],
            "sources": expected, "evidence_sha256": {str(path): pin for path, pin in evidence.items()}}


def ratio(numerator: int | float, denominator: int | float) -> float | None:
    return numerator / denominator if denominator else None


def timing_pair(baseline: float, candidate: float) -> dict[str, float | None]:
    return {"baseline_ms": baseline, "candidate_ms": candidate,
            "candidate_over_baseline": ratio(candidate, baseline)}


def summarize_pair(old: dict[str, Any], new: dict[str, Any]) -> dict[str, Any]:
    result = {key: old[key] for key in reader.KEYS}
    result.update(repeats_per_revision=old["repeats"], candidate_pairs=old["candidate_pairs"],
                  candidate_descriptors=old["candidate_descriptors"], common_times={}, arms=[])
    for name in reader.COMMON_TIMES:
        result["common_times"][name] = timing_pair(old[f"median_{name}"], new[f"median_{name}"])
    for left, right in zip(old["arms"], new["arms"]):
        arm = {"mode": left["mode"], "accepted_pairs": left["accepted_pairs"],
               "rejected_pairs": left["rejected_pairs"], "digest": left["digest"], "work": left["work"],
               "times": {name: timing_pair(left[f"median_{name}"], right[f"median_{name}"])
                         for name in reader.ARM_TIMES},
               "work_candidate_over_baseline": {name: ratio(right["work"][name], left["work"][name])
                                                for name in reader.COUNTERS}}
        result["arms"].append(arm)
    return result


def scaling(summary: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Retain order/prefilter/s/K; emit only observed n -> 2n comparisons."""
    indexed = {tuple(item[key] for key in reader.KEYS): item for item in summary}
    result = []
    for key, small in sorted(indexed.items()):
        larger_key = (key[0], 2 * key[1], *key[2:])
        if larger_key not in indexed:
            continue
        large = indexed[larger_key]
        item = {name: small[name] for name in reader.KEYS if name != "n"}
        item.update(n_from=small["n"], n_to=large["n"],
                    candidate_pairs_ratio=ratio(large["candidate_pairs"], small["candidate_pairs"]),
                    candidate_descriptors_ratio=ratio(large["candidate_descriptors"], small["candidate_descriptors"]),
                    arms=[])
        for left, right in zip(small["arms"], large["arms"]):
            item["arms"].append({"mode": left["mode"],
                "time_ratios": {name: ratio(right[f"median_{name}"], left[f"median_{name}"])
                                for name in reader.ARM_TIMES},
                "work_ratios": {name: ratio(right["work"][name], left["work"][name]) for name in SCALING_WORK}})
        result.append(item)
    return result


def compare(baseline: Path, candidate: Path, include_summary: bool) -> dict[str, Any]:
    current_sources = reader.sources()
    checking_code = {str(path.relative_to(ROOT)): reader.digest(path) for path in (Path(__file__), READER_PATH)}
    old = read_revision(baseline, baseline_sources())
    new = read_revision(candidate, current_sources)
    left, right = old["checked"], new["checked"]
    reader.require(left["provenance"]["machine_id"] == right["provenance"]["machine_id"],
                   "revision machine metadata are not comparable")
    reader.require(old["manifest"]["compiler_version"] == new["manifest"]["compiler_version"] and
                   compile_profile(old["manifest"]) == compile_profile(new["manifest"]),
                   "revision compiler/configuration are not comparable")
    old_counts = Counter({key: len(rows) for key, rows in old["rows"].items()})
    new_counts = Counter({key: len(rows) for key, rows in new["rows"].items()})
    reader.require(old_counts == new_counts, "revision matrices/repetition multiplicities differ")
    for key in old_counts:
        a, b = old["rows"][key][0], new["rows"][key][0]
        reader.require(all(a[name] == b[name] for name in
                           ("input_fnv1a64_le_u16_xyz", "fixture_version", "n_a", "n_b")),
                       "revision input identity differs")
        reader.require(reader.stable(a) == reader.stable(b),
                       "revision candidates/descriptors/work/output digest differ")
    reader.require(current_sources == reader.sources() and
                   all(reader.digest(ROOT / name) == pin for name, pin in checking_code.items()),
                   "comparison sources changed during validation")
    result = {"schema": "mhgp8_q2_revision_comparison_v1", "status": "passed",
              "scope": "materialized_q2_component_revision_comparison", "full_contract_qualified": False,
              "gcp_used_by_comparator": False, "baseline_revision": BASELINE_REVISION,
              "matching_work_and_outputs": True, "configurations_including_order": len(old_counts),
              "measurements_per_revision": sum(old_counts.values()),
              "ratio_kind": "candidate_over_baseline_lower_is_faster_null_if_zero_denominator",
              "comparison_reader_sha256": checking_code,
              "binary_verification": "recorded_before_after_and_closing_pins_not_a_rebuild",
              "sampling": "separate_order_medians_no_confidence_interval_or_worst_case_claim"}
    for label, revision in (("baseline", old), ("candidate", new)):
        result[label] = {"provenance": revision["checked"]["provenance"],
                         "campaigns": revision["checked"]["campaigns"], "source_sha256": revision["sources"],
                         "capture_runner_sha256": revision["checked"]["capture_runner_sha256"],
                         "evidence_sha256": revision["evidence_sha256"]}
    if include_summary:
        old_summary = {tuple(item[key] for key in reader.KEYS): item for item in left["summary"]}
        new_summary = {tuple(item[key] for key in reader.KEYS): item for item in right["summary"]}
        result["summary"] = [summarize_pair(old_summary[key], new_summary[key]) for key in sorted(old_counts)]
        result["doublings"] = {"baseline": scaling(left["summary"]), "candidate": scaling(right["summary"])}
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("--summary", action="store_true")
    args = parser.parse_args()
    try:
        result = compare(args.baseline, args.candidate, args.summary)
    except (ValueError, OSError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print(f"q2 revision comparison rejected: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2, allow_nan=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
