"""Snapshot and replay batch equivalence plus the exceptional-assignment repair."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from typing import Any


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PRODUCT = [
    "morsehgp3D_v8/src/core/types.hpp",
    "morsehgp3D_v8/src/spindle/predicates.hpp",
    "morsehgp3D_v8/src/pipeline/local_credits.hpp",
    "morsehgp3D_v8/src/pipeline/local_credits.cpp",
    "morsehgp3D_v8/src/pipeline/tube_credits.hpp",
]
INPUTS = PRODUCT + [
    "morsehgp3D_v8/tests/plan_assignment_gate.cpp",
    "morsehgp3D_v8/tests/batch_gate.cpp",
    "audits/morsehgp3D_v8_complementaire/plan_assignment_probe.cpp",
    "audits/morsehgp3D_v8_complementaire/tubes_probe.cpp",
]
FLAGS = [
    "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1",
    "-fsanitize=undefined", "-fno-sanitize-recover=all",
]
WORK_FIELDS = [
    "validation_points", "uniqueness_comparisons", "pool_selection_tests",
    "pool_selected", "tree_nodes", "tree_point_visits", "dual_tasks",
    "credited_blocks", "noncredit_blocks", "leaf_pairs", "saturated_tasks",
    "max_tree_depth", "max_task_depth", "tube_records", "tube_cells",
    "tube_sort_comparisons", "tube_sweep_tests", "tube_credited_sites",
    "tube_separation_fallbacks", "predicates.point_tests",
    "predicates.universal_queries", "predicates.q2_axis_terms",
    "predicates.corner_tests", "predicates.block_bound_tests",
    "predicates.negative_probes",
]
HELPER = """
template <class Left, class Right>
bool same_sequence(const Left& a, const Right& b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

void same_batch_plan(const CreditPlan& batch, const CreditPlan& single) {
  require(&batch.rectangle() == &single.rectangle() && batch.lane() == single.lane() &&
              batch.strategy() == single.strategy() && batch.threshold() == single.threshold() &&
              batch.core_credit() == single.core_credit() && batch.total_pairs() == single.total_pairs() &&
              batch.candidate_pairs() == single.candidate_pairs(), "batch scalar or owner mismatch");
  require(same_sequence(batch.a_credits(), single.a_credits()) &&
              same_sequence(batch.b_credits(), single.b_credits()) &&
              same_sequence(batch.a_order(), single.a_order()) &&
              same_sequence(batch.b_order(), single.b_order()) &&
              batch.blocks().size() == single.blocks().size(), "batch physical arrays mismatch");
  for (std::size_t i = 0; i < single.blocks().size(); ++i) {
    const auto a = batch.blocks()[i], b = single.blocks()[i];
    require(a.a.first == b.a.first && a.a.last == b.a.last &&
                a.b.first == b.b.first && a.b.last == b.b.last, "batch block mismatch");
  }
  auto expected = single.work();
  expected.tube_records = 0;
  expected.tube_cells = 0;
  expected.tube_sort_comparisons = 0;
  expected.tube_separation_fallbacks = 0;
  require(all_work(batch.work()) == all_work(expected), "batch query work mismatch");
}

"""


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def replace_once(text: str, before: str, after: str) -> str:
    if text.count(before) != 1:
        raise RuntimeError(f"probe adaptation marker changed: {before[:60]}")
    return text.replace(before, after, 1)


def enrich_probe(text: str) -> str:
    accessor = (
        "std::array<u64, 25> all_work(const Work& work) { return {"
        + ", ".join("work." + field for field in WORK_FIELDS)
        + "}; }\n"
    )
    text = replace_once(text, "void examine(", accessor + HELPER + "void examine(")
    text = replace_once(
        text,
        "  const auto rectangle = prepare_rectangle(std::move(input), kmax, separation);",
        "  const auto rectangle = prepare_rectangle(std::move(input), kmax, separation);\n"
        "  const auto batch = make_credit_batch(rectangle, Strategy::Tubes);\n"
        "  std::array<u64, 4> separate_preparation{};\n"
        "  unsigned active = 0;",
    )
    text = replace_once(
        text, "    const auto& work = plan.work();",
        "    same_batch_plan(batch.plan(lane), plan);\n"
        "    const auto& work = plan.work();\n"
        "    separate_preparation[0] += work.tube_records;\n"
        "    separate_preparation[1] += work.tube_cells;\n"
        "    separate_preparation[2] += work.tube_sort_comparisons;\n"
        "    separate_preparation[3] += work.tube_separation_fallbacks;",
    )
    text = replace_once(
        text, "    const unsigned need = plan.threshold() - plan.core_credit();",
        "    const unsigned need = plan.threshold() - plan.core_credit();\n"
        "    active += static_cast<unsigned>(need != 0);",
    )
    text = replace_once(
        text, "  }\n}\n\n}  // namespace",
        """  }
  const auto& shared = batch.shared_work();
  const std::array<u64, 4> shared_preparation{shared.tube_records, shared.tube_cells,
      shared.tube_sort_comparisons, shared.tube_separation_fallbacks};
  for (std::size_t index = 0; index < shared_preparation.size(); ++index)
    require(separate_preparation[index] == active * shared_preparation[index], "preparation count mismatch");
  auto stripped = shared;
  stripped.tube_records = 0;
  stripped.tube_cells = 0;
  stripped.tube_sort_comparisons = 0;
  stripped.tube_separation_fallbacks = 0;
  require(all_work(stripped) == all_work(Work{}), "nonshared work in preparation");
  if (active == 0) require(all_work(shared) == all_work(Work{}), "unused shared preparation");
}

}  // namespace""",
    )
    return text


def command(args: list[str]) -> dict[str, Any]:
    completed = subprocess.run(args, capture_output=True, text=True, timeout=55, check=False)
    return {
        "argv": args, "returncode": completed.returncode,
        "stdout": completed.stdout, "stderr": completed.stderr,
    }


def audit(replay: Path | None) -> dict[str, Any]:
    report: dict[str, Any] = {
        "schema": "mhgp8_batch_exception_review_v1",
        "status": "failed",
        "scope": "exception_repair_and_bounded_batch_equivalence_not_full_or_performance",
        "python_optimization": sys.flags.optimize,
        "sources": {},
        "commands": [],
        "gcp_used": False,
    }
    try:
        sources: dict[str, bytes] = {}
        prior = json.loads(replay.read_text()) if replay else None
        for name in INPUTS:
            if prior is not None:
                item = prior["sources"][name]
                data = item["text"].encode()
                if digest(data) != item["sha256"]:
                    raise RuntimeError(f"replay source hash mismatch: {name}")
            else:
                data = (ROOT / name).read_bytes()
            sources[name] = data
            report["sources"][name] = {"sha256": digest(data), "text": data.decode()}
        if prior is None:
            if any((ROOT / name).read_bytes() != data for name, data in sources.items()):
                raise RuntimeError("worktree changed while capturing the snapshot")
            report["snapshot_consistent_on_capture"] = True
        else:
            report["replay_receipt_sha256"] = digest(replay.read_bytes())
        report["runner_sha256"] = digest(Path(__file__).read_bytes())
        compiler = shutil.which("g++")
        if compiler is None:
            raise RuntimeError("g++ unavailable")
        report["compiler"] = command([compiler, "--version"])
        report["compiler_sha256"] = digest(Path(compiler).read_bytes())
        if report["compiler"]["returncode"] != 0:
            raise RuntimeError("compiler identification failed")
        derived = enrich_probe(sources[INPUTS[-1]].decode()).encode()
        report["derived_probe"] = {"sha256": digest(derived), "text": derived.decode()}
        runs = [
            ("independent_assignment", sources[INPUTS[-2]], ["--expect-strong"]),
            ("constructor_assignment_gate", sources[INPUTS[5]], ["--selftest"]),
            ("constructor_batch_gate", sources[INPUTS[6]], ["--selftest"]),
            ("independent_tubes_batch", derived, []),
        ]
        with tempfile.TemporaryDirectory(prefix="mhgp8_batch_exception_") as tmp:
            directory = Path(tmp)
            for name in PRODUCT:
                destination = directory / Path(name).relative_to("morsehgp3D_v8")
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(sources[name])
            for label, probe, arguments in runs:
                source = directory / f"{label}.cpp"
                source.write_bytes(probe)
                executable = directory / label
                record: dict[str, Any] = {"name": label, "status": "failed"}
                report["commands"].append(record)
                record["compile"] = command([
                    compiler, *FLAGS, "-I", str(directory / "src"), str(source),
                    str(directory / "src/pipeline/local_credits.cpp"), "-o", str(executable),
                ])
                if record["compile"]["returncode"] != 0:
                    raise RuntimeError(f"compilation failed: {label}")
                record["executable_sha256"] = digest(executable.read_bytes())
                result = command([str(executable), *arguments])
                record["run"] = result
                if result["returncode"] != 0 or result["stderr"]:
                    raise RuntimeError(f"probe failed: {label}")
                if label == "independent_assignment":
                    physical = json.loads(result["stdout"])
                    if physical["allocation_failure_caught"] != 1 or any(
                        physical[field] != 0 for field in ("owner_changed", "invalid_emissions", "wrong_lengths")
                    ):
                        raise RuntimeError("exceptional assignment not repaired")
                    record["physical_result"] = physical
                else:
                    counts = {key: int(value) for key, value in re.findall(r"([a-z_]+)=(\d+)", result["stdout"])}
                    record["counts"] = counts
                    if label == "constructor_assignment_gate":
                        if counts.get("plan_allocation_failures", 0) < 5 or counts.get("batch_allocation_failures", 0) < 15:
                            raise RuntimeError("exception failure positions not exercised")
                    elif label == "constructor_batch_gate":
                        if counts.get("batches") != 195 or counts.get("lane_comparisons") != 585:
                            raise RuntimeError("batch gate nonvacuity")
                    else:
                        expected = {"plans": 3051, "credits_checked": 65772, "pair_checks": 360933,
                                    "credited_sites": 30881, "sweep_tests": 76538, "fallbacks": 24,
                                    "inactive_or_core_saturated": 343}
                        if counts != expected:
                            raise RuntimeError("independent geometry or batch counts changed")
                record["status"] = "passed"
        if prior is None:
            after = {name: digest((ROOT / name).read_bytes()) for name in INPUTS}
            report["source_sha256_after"] = after
            report["live_sources_unchanged_after"] = all(after[name] == digest(data) for name, data in sources.items())
        report["status"] = "passed"
    except (OSError, RuntimeError, ValueError, KeyError, subprocess.SubprocessError) as error:
        report["error"] = str(error)
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--replay", type=Path, help="Use the source snapshot embedded in an earlier receipt.")
    parser.add_argument("--output", type=Path, help="Create a new receipt; never overwrite.")
    args = parser.parse_args()
    if args.output is not None and args.output.exists():
        parser.error("output already exists")
    report = audit(args.replay)
    encoded = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output is not None:
        with args.output.open("x", encoding="utf-8") as output:
            output.write(encoded)
    print(encoded, end="")
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
