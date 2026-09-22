#!/usr/bin/env python3
"""Compare prepared-bound integration with frozen f481 census, without timings.

The published independent judge remains unchanged. Temporary copies receive
only a bound API adaptation and a canonical observation stream. Each original
oracle assertion and every fixture remain active. Receipts embed all sources;
--replay never reads current product sources or overwrites an earlier receipt.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
DIRECTORY = "morsehgp3D_v8/src/"
CENSUS = DIRECTORY + "pipeline/q2_census.cpp"
PREPARED = DIRECTORY + "spindle/q2_prepared_bounds.hpp"
JUDGE = "audits/morsehgp3D_v8_complementaire/q2_census_independent_probe.cpp"
BASE = "f4815cd4"
PATHS = [DIRECTORY + name for name in (
    "core/types.hpp", "pipeline/local_credits.cpp", "pipeline/local_credits.hpp",
    "pipeline/tube_credits.hpp", "pipeline/axis_q2.cpp", "pipeline/axis_q2.hpp",
    "pipeline/q2_census.cpp", "pipeline/q2_census.hpp", "spindle/predicates.hpp")]
EXPECTED_CURRENT = {
    PREPARED: "7bb46b4b7af3beede9bc2fc8926bafda9c900eb573671583206a6d94ffac5d21",
    CENSUS: "b1ca5edd575f39bc7995bcf09dca0ccc9bb6838469fe3761f9bf1a2191187c74",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def sha(value: str | bytes) -> str:
    return hashlib.sha256(value.encode() if isinstance(value, str) else value).hexdigest()


def invoke(command: list[str], timeout: int = 60) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, capture_output=True, text=True, timeout=timeout)


def replace_once(source: str, before: str, after: str) -> str:
    require(source.count(before) == 1, "unexpected substitution count: " + before)
    return source.replace(before, after)


def traced_judge(original: str, fields: list[str], prepared: bool) -> str:
    observer = r'''
void audit_ids(const Ids& ids) {
  std::cout << '[';
  bool first=true;
  for(const auto id:ids){if(!first)std::cout<<',';first=false;std::cout<<id;}
  std::cout << ']';
}
void audit_call(const Output& output,const mhgp8::Q2CensusResult& result,
                mhgp8::Q2CensusMode mode) {
  std::cout << "{\"case\":" << totals.cases << ",\"mode\":"
            << (mode==mhgp8::Q2CensusMode::Pairwise?0:1)
            << ",\"candidate_pairs\":" << result.candidate_pairs
            << ",\"accepted_pairs\":" << result.accepted_pairs
            << ",\"rejected_pairs\":" << result.rejected_pairs
            << ",\"work\":{";
FIELDS
  std::cout << "},\"payloads\":[";
  bool first=true;
  for(const auto& [pair,payload]:output) {
    if(!first)std::cout<<',';
    first=false;
    std::cout<<'['<<pair.first<<','<<pair.second<<",["
             <<std::get<0>(payload.key)<<','<<std::get<1>(payload.key)<<','
             <<std::get<2>(payload.key)<<','<<std::get<3>(payload.key)<<"],";
    audit_ids(payload.interior);std::cout<<',';audit_ids(payload.shell);std::cout<<']';
  }
  std::cout << "]}\n";
}
'''
    statements = []
    for index, field in enumerate(fields):
        prefix = "," if index else ""
        statements.append(f'  std::cout << "{prefix}\\\"{field}\\\":" << result.work.{field};')
    observer = observer.replace("FIELDS", "\n".join(statements))
    original = replace_once(original, "Output check(", observer + "\nOutput check(")
    original = replace_once(original, '    require(actual==expected,"census omitted a support");',
        '    require(actual==expected,"census omitted a support");\n    audit_call(actual,result,mode);')
    if prepared:
        original = replace_once(original, "mhgp8::shared_bounds(p(a),b,z)",
                                "mhgp8::Q2PreparedBounds(p(a),b).bounds(z)")
    return original


def build_run(root: Path, label: str, source: dict[str, str], judge: str,
              sanitized: bool = False) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    directory = root / label
    for name, content in {**source, JUDGE: judge}.items():
        path = directory / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
    flags = ["-std=c++20", "-O1" if sanitized else "-O2",
             "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    if sanitized:
        flags += ["-fsanitize=undefined", "-fno-sanitize-recover=all"]
    binary = directory / "judge"
    command = ["g++", *flags, "-I", str(directory / DIRECTORY), str(directory / JUDGE),
               str(directory / (DIRECTORY + "pipeline/local_credits.cpp")),
               str(directory / (DIRECTORY + "pipeline/axis_q2.cpp")), "-o", str(binary)]
    build = invoke(command)
    require(build.returncode == 0, label + " build failed: " + build.stderr)
    run = invoke([str(binary), "--selftest"], 30)
    rows = [json.loads(line) for line in run.stdout.splitlines()]
    observations = [row for row in rows if "case" in row]
    summary = rows[-1] if rows and "status" in rows[-1] else None
    print(label + ": exit " + str(run.returncode), file=sys.stderr, flush=True)
    return {"label": label, "compile_flags": flags, "build_exit": build.returncode,
            "binary_sha256": sha(binary.read_bytes()), "run_exit": run.returncode,
            "stdout_sha256": sha(run.stdout), "stderr": run.stderr,
            "observed_calls": len(observations), "summary": summary}, observations


def compare(before: list[dict[str, Any]], after: list[dict[str, Any]]) -> dict[str, Any]:
    require(len(before) == len(after) == 932, "missing comparison calls")
    differences = []
    for left, right in zip(before, after):
        require((left["case"], left["mode"]) == (right["case"], right["mode"]), "call identity")
        if left != right:
            differences.append({"case": left["case"], "mode": left["mode"],
                "payloads_equal": left["payloads"] == right["payloads"],
                "work_differences": {name: [value, right["work"][name]]
                                     for name, value in left["work"].items()
                                     if value != right["work"][name]}})
    return {"calls": len(before), "differing_calls": len(differences),
            "first_differences": differences[:5],
            "all_payloads_equal": all(a["payloads"] == b["payloads"] for a, b in zip(before, after))}


def evaluate(snapshot: dict[str, Any]) -> dict[str, Any]:
    baseline, current, original = snapshot["baseline"], snapshot["current"], snapshot["judge"]
    work = re.search(r"struct Q2CensusWork \{(.*?)\n\};", current[DIRECTORY + "pipeline/q2_census.hpp"], re.S)
    require(work is not None, "work declaration missing")
    fields = re.findall(r"u64 (\w+)\{\};", work.group(1))
    require(len(fields) == 26 and len(set(fields)) == 26, "work field coverage")
    old_judge, new_judge = (traced_judge(original, fields, False), traced_judge(original, fields, True))
    with tempfile.TemporaryDirectory(prefix="mhgp8_prepared_census_") as temporary:
        root = Path(temporary)
        old, reference = build_run(root, "f481_baseline", baseline, old_judge)
        new, actual = build_run(root, "prepared", current, new_judge)
        sanitized, checked = build_run(root, "prepared_ubsan", current, new_judge, True)
        require(all(item["run_exit"] == 0 and not item["stderr"] for item in (old, new, sanitized)),
                "baseline/prepared judge failed")
        require(old["summary"] == new["summary"] == sanitized["summary"], "independent judge totals differ")
        equality, ubsan_equality = compare(reference, actual), compare(actual, checked)
        require(equality["differing_calls"] == ubsan_equality["differing_calls"] == 0,
                "prepared integration changed discrete execution or output")
        text = current[CENSUS]
        inherited = replace_once(text, "                    std::size_t cursor) {",
            "                    std::size_t cursor, const Q2PreparedBounds* inherited=nullptr) {")
        inherited = replace_once(inherited, "prepared = Q2PreparedBounds(points[a_id], b.box);",
            "prepared = inherited ? *inherited : Q2PreparedBounds(points[a_id], b.box);")
        for side in ("left", "right"):
            inherited = replace_once(inherited, f"shared_task(a_id, b.{side}, count, cursor);",
                f"shared_task(a_id, b.{side}, count, cursor, &prepared);")
        mutant, mutant_rows = build_run(root, "parent_B_constants", {**current, CENSUS: inherited}, new_judge)
        require(mutant["run_exit"] == 0, "conservative parent-B mutant unexpectedly lost oracle exactness")
        mutation_difference = compare(actual, mutant_rows)
        require(mutation_difference["differing_calls"] > 0 and mutation_difference["all_payloads_equal"],
                "parent-B mutant did not refute the discrete-equivalence contract")
        wrong = replace_once(text, "prepared = Q2PreparedBounds(points[a_id], b.box);",
            "prepared = Q2PreparedBounds(points[b_order[b.range.first]], b.box);")
        anchor, _ = build_run(root, "wrong_anchor", {**current, CENSUS: wrong}, new_judge)
        require(anchor["run_exit"] == 1 and bool(anchor["stderr"]), "wrong anchor mutant survived")
    return {"status": "passed", "compared_work_fields": fields,
            "baseline": old, "prepared": new, "prepared_ubsan": sanitized,
            "full_call_equality": equality, "ubsan_equality": ubsan_equality,
            "mutants": [{**mutant, "source_sha256": sha(inherited),
                         "comparison": mutation_difference,
                         "verdict": "oracle_exact_but_discrete_equivalence_refuted"},
                        {**anchor, "source_sha256": sha(wrong),
                         "verdict": "wrong_anchor_rejected_by_product_count_payload_consistency"}],
            "instrumented_judge_sha256": {"baseline": sha(old_judge), "prepared": sha(new_judge)}}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--selftest", action="store_true")
    mode.add_argument("--replay", type=Path)
    args = parser.parse_args()
    if args.replay:
        receipt = json.loads(args.replay.read_text())
        snapshot = receipt["snapshot_utf8"]
        require(sha(json.dumps(snapshot, sort_keys=True, ensure_ascii=False)) == receipt["snapshot_sha256"],
                "snapshot hash mismatch")
    else:
        baseline = {}
        for name in PATHS:
            result = invoke(["git", "show", BASE + ":" + name])
            require(result.returncode == 0, "baseline source unavailable: " + name)
            baseline[name] = result.stdout
        current = {name: (ROOT / name).read_text() for name in [*PATHS, PREPARED]}
        require(all(sha(current[name]) == pin for name, pin in EXPECTED_CURRENT.items()),
                "prepared draft changed: use retained receipt replay instead")
        require(all((ROOT / name).read_text() == value for name, value in current.items()),
                "unstable draft snapshot")
        snapshot = {"baseline": baseline, "current": current, "judge": (ROOT / JUDGE).read_text()}
    result = evaluate(snapshot)
    result.update({"schema": "mhgp8_prepared_census_integration_checks_v1",
        "created_utc": datetime.now(timezone.utc).isoformat(), "public_status": "not_claimed",
        "baseline_commit": BASE, "snapshot_utf8": snapshot,
        "snapshot_sha256": sha(json.dumps(snapshot, sort_keys=True, ensure_ascii=False)),
        "source_sha256": {label: {name: sha(value) for name, value in snapshot[label].items()}
                          for label in ("baseline", "current")},
        "original_judge_sha256": sha(snapshot["judge"]), "runner_sha256": sha(Path(__file__).read_bytes()),
        "scope": "466_bounded_fixtures_discrete_work_and_exact_payload_equivalence_no_performance_or_full",
        "gcp_used": False})
    print(json.dumps(result, ensure_ascii=False, indent=2, allow_nan=False))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(json.dumps({"status": "failed", "error": str(error)}))
        raise SystemExit(1) from error
