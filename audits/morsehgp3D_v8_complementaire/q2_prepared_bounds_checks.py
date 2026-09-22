#!/usr/bin/env python3
"""Exercise the frozen prepared-bounds header, legacy expressions and H oracle.

Receipts embed every compiled source. Replays neither read current production
sources nor depend on a temporary directory or the availability of old Git data.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
HEADER = "morsehgp3D_v8/src/spindle/q2_prepared_bounds.hpp"
TYPES = "morsehgp3D_v8/src/core/types.hpp"
CENSUS = "morsehgp3D_v8/src/pipeline/q2_census.cpp"
PROBE = "audits/morsehgp3D_v8_complementaire/q2_prepared_bounds_probe.cpp"
LEGACY = "legacy_q2_census.cpp"
PINS = {
    HEADER: "7bb46b4b7af3beede9bc2fc8926bafda9c900eb573671583206a6d94ffac5d21",
    TYPES: "f4c05da3de254aadf95993988a44a933bf235282bc6f33930984689eba967f2b",
    CENSUS: "b1ca5edd575f39bc7995bcf09dca0ccc9bb6838469fe3761f9bf1a2191187c74",
    LEGACY: "3c513cc474c3d3a249779032f5cd03dac47198cf4b25d7698855bd118e0e593a",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def sha(value: str | bytes) -> str:
    return hashlib.sha256(value.encode() if isinstance(value, str) else value).hexdigest()


def packed(value: Any) -> str:
    return json.dumps(value, sort_keys=True, ensure_ascii=False)


def invoke(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, text=True, capture_output=True, timeout=60)


def excerpt(source: str, name: str) -> str:
    marker = "[[nodiscard]] PowerBounds " + name + "("
    require(source.count(marker) == 1, "legacy function marker: " + name)
    start = source.index(marker)
    opening = source.index("{", start)
    depth = 0
    for end in range(opening, len(source)):
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
            if depth == 0:
                return source[start:end + 1]
    raise RuntimeError("unterminated legacy function: " + name)


def legacy_header(source: str) -> str:
    return """#pragma once
#include <algorithm>
#include <array>
#include <limits>
namespace legacy {
using mhgp8::i64;
using mhgp8::u64;
using mhgp8::Point3;
using mhgp8::Box3;
using PowerBounds = mhgp8::Q2Bounds;
struct Q2BallKey {
  std::array<std::uint32_t, 3> center_twice{};
  u64 diameter_squared{};
};
""" + excerpt(source, "pair_bounds") + "\n" + excerpt(source, "shared_bounds") + "\n}\n"


def replace_once(source: str, before: str, after: str) -> str:
    require(source.count(before) == 1, "mutant anchor changed: " + before)
    return source.replace(before, after)


def build_run(directory: Path, snapshot: dict[str, str], label: str,
              header: str, sanitized: bool = False) -> dict[str, Any]:
    build_dir = directory / label
    sources = {HEADER: header, TYPES: snapshot[TYPES], PROBE: snapshot[PROBE],
               "legacy_bounds.hpp": legacy_header(snapshot[LEGACY])}
    for name, source in sources.items():
        path = build_dir / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(source)
    flags = ["-std=c++20", "-O1" if sanitized else "-O2", "-Wall", "-Wextra",
             "-Wpedantic", "-Werror"]
    if sanitized:
        flags += ["-fsanitize=undefined", "-fno-sanitize-recover=all"]
    binary = build_dir / "probe"
    command = ["g++", *flags, "-I", str(build_dir / "morsehgp3D_v8/src"),
               "-I", str(build_dir), str(build_dir / PROBE), "-o", str(binary)]
    build = invoke(command)
    require(build.returncode == 0, label + " compile failed: " + build.stderr)
    run = invoke([str(binary), "--selftest"])
    bad_cli = invoke([str(binary)])
    require(bad_cli.returncode == 2 and not bad_cli.stdout, "C++ CLI guard")
    print(label + ": exit " + str(run.returncode), file=sys.stderr, flush=True)
    return {"label": label, "compile_flags": flags, "build_exit": build.returncode,
            "binary_sha256": sha(binary.read_bytes()), "run_exit": run.returncode,
            "invalid_cli_exit": bad_cli.returncode, "stdout_sha256": sha(run.stdout),
            "stderr": run.stderr,
            "result": json.loads(run.stdout) if run.returncode == 0 else None}


def evaluate(snapshot: dict[str, str]) -> dict[str, Any]:
    for path, digest in PINS.items():
        require(sha(snapshot[path]) == digest, "source pin mismatch: " + path)
    header = snapshot[HEADER]
    mutations = [
        ("center_u16", "static_cast<std::uint32_t>(anchor + endpoints[side])",
         "static_cast<std::uint16_t>(anchor + endpoints[side])"),
        ("distance_squared_u16", "static_cast<std::uint32_t>(difference * difference)",
         "static_cast<std::uint16_t>(difference * difference)"),
        ("summit_lost", "high_delta < 0 ? high_squared : 0;",
         "high_delta < 0 ? high_squared : std::min(low_squared, high_squared);"),
        ("minimum_nearest_endpoint", "distance - std::max(low_squared, high_squared)",
         "distance - std::min(low_squared, high_squared)"),
        ("unchecked_public_z", "    require_valid_box(z);", "    // mutant: missing validation"),
    ]
    with tempfile.TemporaryDirectory(prefix="mhgp8_prepared_bounds_gate_") as temporary:
        directory = Path(temporary)
        normal = build_run(directory, snapshot, "prepared_O2", header)
        sanitized = build_run(directory, snapshot, "prepared_ubsan", header, True)
        require(normal["run_exit"] == sanitized["run_exit"] == 0,
                "prepared bounds positive run failed")
        require(not normal["stderr"] and not sanitized["stderr"], "unexpected diagnostics")
        require(normal["result"] == sanitized["result"], "sanitizer results differ")
        require(normal["result"]["prepared_bytes"] == 48, "prepared value size changed")
        mutants = []
        for label, before, after in mutations:
            changed = replace_once(header, before, after)
            result = build_run(directory, snapshot, label, changed)
            require(result["run_exit"] == 1 and bool(result["stderr"]),
                    "real C++ mutant survived: " + label)
            mutants.append({**result, "mutated_header_sha256": sha(changed),
                            "verdict": "rejected", "replacement": [before, after]})
    return {"status": "passed", "normal": normal, "ubsan": sanitized,
            "mutants": mutants, "legacy_excerpts_sha256": {
                name: sha(excerpt(snapshot[LEGACY], name))
                for name in ("pair_bounds", "shared_bounds")},
            "generated_legacy_header_sha256": sha(legacy_header(snapshot[LEGACY]))}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--selftest", action="store_true")
    mode.add_argument("--replay", type=Path)
    parser.add_argument("--snapshot", type=Path)
    args = parser.parse_args()
    if args.replay:
        require(args.snapshot is None, "--snapshot cannot be combined with --replay")
        receipt = json.loads(args.replay.read_text())
        snapshot = receipt["snapshot_utf8"]
        require(sha(packed(snapshot)) == receipt["snapshot_sha256"], "snapshot hash mismatch")
        require({name: sha(value) for name, value in snapshot.items()} == receipt["source_sha256"],
                "per-source hashes differ")
    else:
        root = args.snapshot if args.snapshot else ROOT
        snapshot = {path: (root / path).read_text() for path in (HEADER, TYPES, CENSUS)}
        if args.snapshot:
            snapshot[LEGACY] = (root / LEGACY).read_text()
        else:
            old = invoke(["git", "-C", str(ROOT), "show", "f4815cd4:" + CENSUS])
            require(old.returncode == 0, "legacy source unavailable: " + old.stderr)
            snapshot[LEGACY] = old.stdout
        snapshot[PROBE] = (ROOT / PROBE).read_text()
    result = evaluate(snapshot)
    current_match = all((ROOT / name).is_file() and sha((ROOT / name).read_bytes()) == PINS[name]
                        for name in (HEADER, TYPES, CENSUS))
    compiler = invoke(["g++", "--version"])
    output = {"status": "passed", "created_utc": datetime.now(timezone.utc).isoformat(),
              "scope": "exact_prepared_bounds_not_census_performance_or_full_hgp",
              "source_state": "working_tree_snapshot", "legacy_revision": "f4815cd4",
              "source_hashes_still_match_worktree": current_match,
              "python_optimization": sys.flags.optimize,
              "compiler": compiler.stdout.splitlines()[0],
              "runner_sha256": sha(Path(__file__).read_bytes()),
              "snapshot_sha256": sha(packed(snapshot)),
              "source_sha256": {name: sha(value) for name, value in snapshot.items()},
              "snapshot_utf8": snapshot, "checks": result,
              "replay_command": "python3 audits/morsehgp3D_v8_complementaire/q2_prepared_bounds_checks.py --replay audits/morsehgp3D_v8_complementaire/Q2_PREPARED_BOUNDS_CHECKS.json"}
    print(json.dumps(output, indent=2, ensure_ascii=False, sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, OSError, KeyError, ValueError, subprocess.SubprocessError) as error:
        print(json.dumps({"status": "failed", "error": str(error)}))
        sys.exit(1)
