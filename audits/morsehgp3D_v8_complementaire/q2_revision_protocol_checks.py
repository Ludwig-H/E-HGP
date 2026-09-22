#!/usr/bin/env python3
"""Audit genuine tiny revision captures and replay their comparator controls.

--capture borrows existing binaries without rebuilding. --replay uses embedded
source/capture bytes and reads the repository's immutable f481 Git objects only.
Both modes work in temporary directories, preserve original receipts and make
no performance claim from the tiny measurements. No branch or Git write occurs.
"""
from __future__ import annotations

import argparse
import base64
import copy
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from typing import Any, Callable

ROOT = Path(__file__).resolve().parents[2]
RUNNER = "morsehgp3D_v8/bench/run_q2_census_matrix.py"
COMPARATOR = "morsehgp3D_v8/bench/compare_q2_revisions.py"
BASE = "f4815cd42d572db6aef27ec73d100f52303fff26"
SOURCE_CPP = "morsehgp3D_v8/src/pipeline/q2_census.cpp"
PREPARED = "morsehgp3D_v8/src/spindle/q2_prepared_bounds.hpp"
BINARY_PINS = {
    "baseline": "243387ac6df40e3f0f0c38a1d7d7a86735ed260ba11d904bf032bc4eb1c6e432",
    "current": "b76591c05ffd2be8d77979366a1ee3c4526e5913e37cdac24a43b60277b27079",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def digest(value: str | bytes) -> str:
    return hashlib.sha256(value.encode() if isinstance(value, str) else value).hexdigest()


def invoke(command: list[str], environment: dict[str, str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, capture_output=True, text=True, env=environment, timeout=30)


def read(path: Path) -> Any:
    return json.loads(path.read_text())


def write(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, allow_nan=False) + "\n")


def rows(directory: Path) -> list[dict[str, Any]]:
    return [json.loads(line) for line in (directory / "MEASURES.jsonl").read_text().splitlines()]


def write_rows(directory: Path, records: list[dict[str, Any]]) -> None:
    (directory / "MEASURES.jsonl").write_text("".join(json.dumps(row, allow_nan=False) + "\n" for row in records))


def refresh(record: dict[str, Any]) -> None:
    raw = (json.dumps(record["result"], allow_nan=False) + "\n").encode()
    record["stdout"] = raw.decode()
    record["stdout_base64"] = base64.b64encode(raw).decode()


def current_input() -> dict[str, Any]:
    fixed = {"morsehgp3D_v8/CMakeLists.txt", RUNNER, *(
        "morsehgp3D_v8/bench/" + name for name in ("q2_census_probe.cpp", "run_p0_matrix.py",
                                                   "paired_receipts.py", "check_paired_campaign.py"))}

    def covered(name: str) -> bool:
        path = Path(name)
        return (name in fixed or
                name.startswith("morsehgp3D_v8/src/") and path.suffix in (".hpp", ".cpp") or
                str(path.parent) == "morsehgp3D_v8/bench" and path.suffix == ".hpp")

    listing = subprocess.check_output(["git", "ls-tree", "-r", "--name-only", BASE,
                                      "--", "morsehgp3D_v8"], cwd=ROOT, text=True).splitlines()
    baseline = {name: subprocess.check_output(["git", "show", BASE + ":" + name],
                                             cwd=ROOT, text=True) for name in listing if covered(name)}
    current = {str(path.relative_to(ROOT)): path.read_text() for path in
               (ROOT / "morsehgp3D_v8").rglob("*") if path.is_file() and covered(str(path.relative_to(ROOT)))}
    comparator = (ROOT / COMPARATOR).read_text()
    require(all((ROOT / path).read_text() == content for path, content in current.items()) and
            (ROOT / COMPARATOR).read_text() == comparator, "sources changed during snapshot")
    return {"baseline": baseline, "current": current, "comparator": comparator}


def captures(root: Path, bundle: dict[str, Any], environment: dict[str, str]) -> dict[str, Any]:
    results: dict[str, Any] = {}
    for side, build in (("baseline", "v8_census_20260913"), ("current", "v8_prepared_bounds_20260913")):
        binary = ROOT / "build" / build / "mhgp8_q2_census_probe"
        pin = digest(binary.read_bytes())
        require(pin == BINARY_PINS[side], "borrowed binary changed: " + side)
        local = root / "borrowed" / side
        local.mkdir(parents=True)
        shutil.copy2(binary, local / binary.name)
        shutil.copy2(binary.parent / "CMakeCache.txt", local / "CMakeCache.txt")
        command = [sys.executable, "-B", str(root / "sources" / side / RUNNER), "run",
                   "--probe", str(local / binary.name), "--output", str(root / "captures" / side / "genuine"),
                   "--sizes", "8", "12", "--families", "grid", "--kmax", "5", "--s", "8",
                   "--prefilters", "additive", "--orders", "pairwise-first", "shared-first", "--repeats", "1"]
        process = invoke(command, environment)
        require(process.returncode == 0 and not process.stderr, "genuine capture failed: " + process.stderr)
        results[side] = {"command": command, "exit_code": process.returncode, "stdout": process.stdout,
                         "borrowed_binary_path": str(binary), "binary_sha256": pin,
                         "cmake_cache_sha256": digest((local / "CMakeCache.txt").read_bytes()),
                         "rebuilt_by_audit": False}
    command = [sys.executable, "-B", str(root / "sources/current" / RUNNER), "run",
               "--probe", str(root / "missing_binary"), "--output", str(root / "initial_failure"),
               "--sizes", "8", "--families", "grid", "--kmax", "5", "--s", "8",
               "--prefilters", "additive", "--orders", "pairwise-first", "--repeats", "1"]
    process = invoke(command, environment)
    require(process.returncode == 1 and read(root / "initial_failure/COMPLETION.json")["status"] == "failed",
            "initial failure fixture did not fail")
    results["initial_failure"] = {"command": command, "exit_code": process.returncode, "stdout": process.stdout}
    bundle["capture_evidence_utf8"] = {str(path.relative_to(root)): path.read_text()
        for directory in (root / "captures", root / "initial_failure") for path in directory.rglob("*") if path.is_file()}
    return results


def evaluate(bundle: dict[str, Any], capture: bool) -> dict[str, Any]:
    git_dir = subprocess.check_output(["git", "rev-parse", "--absolute-git-dir"], cwd=ROOT, text=True).strip()
    environment = dict(os.environ, GIT_DIR=git_dir, GIT_WORK_TREE=str(ROOT), GIT_OPTIONAL_LOCKS="0",
                       PYTHONDONTWRITEBYTECODE="1")
    require(len(bundle["baseline"]) == 18 and len(bundle["current"]) == 19, "unexpected source coverage")
    with tempfile.TemporaryDirectory(prefix="mhgp8_revision_protocol_checked_") as temporary:
        root = Path(temporary)
        for side in ("baseline", "current"):
            for name, content in bundle[side].items():
                path = root / "sources" / side / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content)
        comparator = root / "sources/current" / COMPARATOR
        comparator.write_text(bundle["comparator"])
        capture_results = captures(root, bundle, environment) if capture else bundle["capture_commands"]
        if not capture:
            for name, content in bundle["capture_evidence_utf8"].items():
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content)
        evidence = {str(path.relative_to(root)): digest(path.read_bytes())
                    for path in (root / "captures").rglob("*") if path.is_file()}

        def comparison(target: Path, optimized: bool, summary: bool = False) -> subprocess.CompletedProcess[str]:
            return invoke([sys.executable, "-B", *(["-O"] if optimized else []), str(comparator),
                           str(target / "baseline"), str(target / "current"), *(["--summary"] if summary else [])], environment)

        def individual(directory: Path, optimized: bool) -> subprocess.CompletedProcess[str]:
            return invoke([sys.executable, "-B", *(["-O"] if optimized else []),
                           str(root / "sources/current" / RUNNER), "check", str(directory)], environment)

        positives = []
        for optimized in (False, True):
            process = comparison(root / "captures", optimized, True)
            require(process.returncode == 0 and not process.stderr, "positive comparator: " + process.stderr)
            answer = json.loads(process.stdout)
            require(answer["matching_work_and_outputs"] is True and answer["configurations_including_order"] ==
                    answer["measurements_per_revision"] == len(answer["summary"]) == 4, "positive counts")
            require(answer["baseline"]["source_sha256"] == {name: digest(value) for name, value in bundle["baseline"].items()} and
                    answer["candidate"]["source_sha256"] == {name: digest(value) for name, value in bundle["current"].items()}, "revision sources")
            for item in answer["summary"]:
                for arm in item["arms"]:
                    require(all(value in (None, 1.0) for value in arm["work_candidate_over_baseline"].values()), "work ratio")
            positives.append({"python_optimized": optimized, "exit_code": 0,
                              "stdout_sha256": digest(process.stdout), "result": answer})

        def source_change(target: Path, side: str, remove: bool) -> None:
            directory = target / side / "genuine"
            manifest = read(directory / "MANIFEST.json")
            if remove:
                del manifest["source_sha256"][PREPARED]
            else:
                manifest["source_sha256"][SOURCE_CPP] = "0" * 64
            completion = read(directory / "COMPLETION.json")
            completion["source_sha256_closing"] = manifest["source_sha256"]
            write(directory / "MANIFEST.json", manifest)
            write(directory / "COMPLETION.json", completion)

        def manifest_change(target: Path, kind: str) -> None:
            path = target / "current/genuine/MANIFEST.json"
            manifest = read(path)
            if kind == "compiler":
                manifest["compiler_version"] += "\nsynthetic alternate compiler\n"
            else:
                old = manifest["cmake_cache"]
                manifest["cmake_cache"] = old.replace("CMAKE_CXX_FLAGS:STRING=", "CMAKE_CXX_FLAGS:STRING=-DAUDIT_MUTANT ")
                require(old != manifest["cmake_cache"], "missing flags key")
            write(path, manifest)

        def closed_matrix_change(target: Path, kind: str) -> None:
            directory = target / "current/genuine"
            manifest, completion, records = read(directory / "MANIFEST.json"), read(directory / "COMPLETION.json"), rows(directory)
            if kind == "matrix":
                manifest["sizes"] = [8]
                records = [row for row in records if row["result"]["n"] == 8]
            elif kind == "repetitions":
                manifest["repeats"] = 2
                extra = copy.deepcopy(records)
                for row in extra:
                    row["repeat"] = 1
                records += extra
            else:
                for row in records:
                    row["result"]["arms"][1]["work"]["consumed_witness_sites"] += 1
                    refresh(row)
            completion["runs"] = completion["attempts"] = len(records)
            write(directory / "MANIFEST.json", manifest)
            write(directory / "COMPLETION.json", completion)
            write_rows(directory, records)

        mutations: list[tuple[str, Callable[[Path], None], str, bool]] = [
            ("candidate_source_omitted", lambda p: source_change(p, "current", True), "explicitly pinned version", False),
            ("baseline_source_wrong", lambda p: source_change(p, "baseline", False), "explicitly pinned version", False),
            ("compiler", lambda p: manifest_change(p, "compiler"), "compiler/configuration", True),
            ("compiler_flags", lambda p: manifest_change(p, "flags"), "compiler/configuration", True),
            ("closed_matrix", lambda p: closed_matrix_change(p, "matrix"), "matrices/repetition multiplicities", True),
            ("closed_repetitions", lambda p: closed_matrix_change(p, "repetitions"), "matrices/repetition multiplicities", True),
            ("shared_work", lambda p: closed_matrix_change(p, "work"), "work/output digest", True),
            ("authentic_failed_sibling", lambda p: shutil.copytree(root / "initial_failure", p / "current/failed"),
             "incomplete q2 campaign receipt", False),
        ]
        rejections = []
        for label, mutate, reason, individually_valid in mutations:
            target = root / "mutants" / label
            shutil.copytree(root / "captures", target)
            mutate(target)
            for optimized in (False, True):
                if individually_valid:
                    checked = individual(target / "current", optimized)
                    require(checked.returncode == 0 and not checked.stderr, "individual fixture invalid: " + label + checked.stderr)
                process = comparison(target, optimized)
                require(process.returncode == 1 and not process.stdout and reason in process.stderr,
                        "mutant survived or wrong cause: " + label + process.stderr)
                rejections.append({"label": label, "python_optimized": optimized, "exit_code": 1,
                                   "individual_reader_passed": individually_valid, "stderr": process.stderr})
        require(all(digest((root / path).read_bytes()) == pin for path, pin in evidence.items()), "genuine evidence changed")
        bundle["capture_commands"] = capture_results
        return {"positive_comparisons": positives, "mutants": rejections,
                "genuine_evidence_sha256": evidence, "genuine_evidence_unchanged": True}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--capture", action="store_true")
    group.add_argument("--replay", type=Path)
    parser.add_argument("--snapshot", type=Path)
    args = parser.parse_args()
    if args.replay:
        prior = read(args.replay)
        bundle = prior["snapshot_utf8"]
        require(digest(json.dumps(bundle, sort_keys=True, ensure_ascii=False)) == prior["snapshot_sha256"], "replay pins")
    else:
        bundle = read(args.snapshot) if args.snapshot else current_input()
    result = evaluate(bundle, args.capture)
    result.update({"schema": "mhgp8_revision_protocol_independent_checks_v1", "status": "passed",
        "created_utc": datetime.now(timezone.utc).isoformat(), "public_status": "not_claimed",
        "source_sha256": {side: {name: digest(content) for name, content in bundle[side].items()}
                          for side in ("baseline", "current")},
        "comparator_sha256": digest(bundle["comparator"]), "runner_sha256": digest(Path(__file__).read_bytes()),
        "snapshot_utf8": bundle, "snapshot_sha256": digest(json.dumps(bundle, sort_keys=True, ensure_ascii=False)),
        "scope": "genuine_tiny_captures_and_synthetic_rejection_fixtures_not_performance_or_full",
        "source_binary_relation": "borrowed_pinned_binaries_not_rebuilt_by_audit", "gcp_used": False})
    print(json.dumps(result, ensure_ascii=False, indent=2, allow_nan=False))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, OSError, ValueError, subprocess.SubprocessError) as error:
        print(json.dumps({"status": "failed", "error": str(error)}))
        raise SystemExit(1) from error
