#!/usr/bin/env python3
"""Read-only F source conservation and recorded CLI build checks; no build."""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import sys


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def inspect() -> dict:
    captured = json.loads((HERE / "source_snapshot.json").read_text())
    patches = []
    for name, pin in captured["files"].items():
        after = (HERE / "snapshot_F" / name).read_bytes()
        require(sha(after) == pin["F_sha256"] and len(after) == pin["F_bytes"], "snapshot F pin")
        require((ROOT / name).read_bytes() == after, "current F drift: " + name)
        before = (HERE / "snapshot_E" / name).read_bytes() if pin["E_exists"] else b""
        if pin["E_exists"]:
            require(sha(before) == pin["E_sha256"], "snapshot E pin")
        patches.append("".join(difflib.unified_diff(
            before.decode().splitlines(keepends=True),
            after.decode().splitlines(keepends=True),
            fromfile="a/" + name if pin["E_exists"] else "/dev/null",
            tofile="b/" + name)))
    # Compare per-file patches independent of map ordering.
    patch = (HERE / "delta_E_F.patch").read_bytes()
    require(sha(patch) == captured["delta_patch_sha256"], "captured diff pin")
    require(all(piece in patch.decode() for piece in patches), "exact source diff pieces")
    file = "morsehgp3D_v7/src/spindle/witness_count.hpp"
    before = (HERE / "snapshot_E" / file).read_text()
    after = (HERE / "snapshot_F" / file).read_text()
    restored = after.replace('#include "../core/inline_stack.hpp"\n', "")
    restored = restored.replace(
        "  InlineStack<Entry, 64> stack;\n  stack.push_back(Entry{ix.root(), mask_eff});",
        "  std::vector<Entry> stack{{ix.root(), mask_eff}};")
    require(restored == before, "F changes more than stack representation")
    reference = (HERE / "snapshot_F/morsehgp3D_v7/tests/reference/witness_count_e.hpp").read_text()
    reference = reference[reference.index("// MorseHGP3D v6"):]
    reference = reference.replace('#include "../../src/spindle/spindle.hpp"', '#include "spindle.hpp"')
    reference = reference.replace("namespace witness_stack_reference {\nusing namespace mhgp7;", "namespace mhgp7 {")
    reference = reference.replace("const bool no_mask = false;", 'const bool no_mask = MHGP7_MUTANT("witness-no-lane-mask");')
    reference = reference.replace("}  // namespace witness_stack_reference", "}  // namespace mhgp7")
    require(reference == before, "reference adaptation changes nominal E count path")
    capture = json.loads((HERE / "build_F_capture.json").read_text())
    for name, pin in capture["files"].items():
        data = (HERE / "build_F_snapshot" / name).read_bytes()
        require(sha(data) == pin["sha256"] and len(data) == pin["bytes"], "build capture pin")
    build = json.loads((HERE / "build_F_snapshot/build_D.json").read_text())
    require(build["status"] == "completed" and build["build_exit_code"] == 0, "build terminal status")
    require(build["sources_before"] == build["sources_after"] and len(build["sources_before"]) == 51, "F build source stability")
    for name, pin in build["sources_before"].items():
        require(sha((ROOT / "morsehgp3D_v7" / name).read_bytes()) == pin, "current build source drift " + name)
    for name in ("binary", "compile_database", "cmake_cache"):
        record = build[name]
        require(sha(Path(record["path"]).read_bytes()) == record["sha256"], "current build artifact " + name)
    database = json.loads(Path(build["compile_database"]["path"]).read_text())
    require(build["compile_command"] in database, "recorded compile command")
    runs = json.loads((HERE / "build_F_snapshot/runs.json").read_text())
    require(len(runs) == 4, "build command nonvacuum")
    for run in runs:
        require(run["returncode"] == 0 and run["status"] == "completed" and run["timed_out"] is False, "recorded build command exit")
        require((HERE / "build_F_snapshot" / (run["name"] + ".err")).read_bytes() == b"", "recorded build stderr")
    raw_build = (HERE / "build_F_snapshot/build.out").read_text()
    require("Building CXX object" in raw_build and "Linking CXX executable mhgp7" in raw_build, "raw compile/link output")
    return {
        "schema": "mhgp7-F-source-conservation-v1",
        "observed_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "status": "source_conservation_checked_F_test_qualification_not_inferred",
        "baseline_E_commit": captured["baseline_commit_E"],
        "frozen_snapshot_sources_still_match": True,
        "F_stack_substitution_reverses_to_exact_E_bytes": True,
        "declared_reference_adaptations_reverse_to_exact_E_bytes": True,
        "recorded_F_CLI_build_sources": 51,
        "F_CLI_binary": build["binary"],
        "recorded_build_commands_all_zero": True,
        "engine_or_build_executions_by_auditor": 0,
        "public_status": "not_claimed", "gcp": "not_used",
        "script_sha256": sha(Path(__file__).read_bytes()),
    }


if __name__ == "__main__":
    try:
        print(json.dumps(inspect(), indent=2, sort_keys=True))
    except (OSError, ValueError, KeyError) as error:
        print(json.dumps({"status": "rejected", "error": str(error)}))
        sys.exit(1)
