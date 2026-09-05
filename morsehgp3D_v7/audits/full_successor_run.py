"""Fresh independent Gamma replay of successor accounting v2, audits only."""
from __future__ import annotations

import argparse
from collections import Counter
from copy import deepcopy
import json
from pathlib import Path
import sys

import full_lazy_audit as old_judge
import full_producer_run as runner
import full_singleton_target as target


A = Path(__file__).resolve().parent
R = A / "receipts_full_successor_20260905"
W = A / ".work_full_successor_20260905"
OLD = A / "receipts_full_singleton_20260905"
HEADER = "85c27ab91d7f159520a8db3098629447b0a213a134c5c042a86c585416847fad"
ACCOUNTING = "full_successor_reads_writes_no_last_pair_v2"
runner.BRIDGE = A / "full_successor_bridge.cpp"
runner.MUTANTS = {}
raw_command = runner.command


def command(argv: list[str], receipt: Path, prefix: str,
            stdin: Path | None = None, overrides: dict[str, str] | None = None) -> dict:
    return raw_command(["timeout", "180", "taskset", "-c", "0", *argv],
                       receipt, prefix, stdin, overrides)


runner.command = command
require = old_judge.require


def inputs() -> tuple[dict, dict]:
    legacy = old_judge.load_fixtures()
    old_judge.verify_fixtures(legacy)
    mixed = target.fixtures()
    require(mixed == json.loads((OLD / "target_fixtures.json").read_text()), "mixed source fixture")
    return legacy, mixed


def judge_rows(rows: list[dict], fixtures: list[dict], prior: list[dict]) -> dict:
    require(len(rows) == len(fixtures) == len(prior), "record inventory")
    totals = Counter()
    for row, r, previous in zip(rows, fixtures, prior):
        require(row["successor_accounting"] == ACCOUNTING, "v2 nominal accounting")
        require(row["id"] == r["id"] and row["order"] == r["order"]
                and row["status"] == 0 and row["reason"] == old_judge.reference.AUTHORITY,
                "identity and complete-relative status")
        require(row["alias_policy"] == old_judge.POLICIES[r["lazy"]]
                and row["cache_capacity"] == r["capacity"], "policy and cache")
        expected = r["expected"]
        require(len(row["nodes"]) == len(expected["nodes"]), "Gamma node inventory")
        for n, e in zip(row["nodes"], expected["nodes"]):
            require(old_judge.reference.rational(n) == old_judge.reference.rational(e)
                    and (n["first"], n["parent_count"]) == (e["first"], e["parent_count"]),
                    "Gamma levels and node structure")
        for name in ("parents", "minima", "coverage"):
            require(row[name] == expected[name], "Gamma " + name)
        require(len(row["cuts"]) == len(expected["roots"]), "Gamma cut inventory")
        for cut, roots in zip(row["cuts"], expected["roots"]):
            require(cut["status"] == 0 and cut["reason"] == "structural_only"
                    and cut["roots"] == roots, "Gamma cut")
        old_judge.check_stats(r, row["stats"])
        want_stats = deepcopy(previous["stats"])
        saving = 2 * want_stats["normalized_anchors"]
        want_stats["successor_steps"] -= saving
        require(row["stats"] == want_stats, "success accounting identity and 32 unchanged fields")
        require({k: v for k, v in row.items() if k not in ("stats", "budget_trials", "successor_accounting")}
                == {k: v for k, v in previous.items() if k not in ("stats", "budget_trials")},
                "literal forest and non-accounting metadata")
        totals["positive_depth_outputs"] += saving > 0
        totals["successor_operations_omitted"] += saving
        trials = row["budget_trials"]
        if r["budget_probe"]:
            caps = old_judge.reference.budget_values(r, row)
            wanted = {("all", "exact")} | {(d, "minus_one") for d, c in caps.items() if c}
            if r["lazy"]:
                wanted.add(("aliases", "conflict"))
            require(len(trials) == len(wanted) and {(t["dimension"], t["kind"]) for t in trials}
                    == wanted, "v2 budget inventory")
            for t in trials:
                require(t["successor_accounting"] == ACCOUNTING, "v2 refused/exact accounting")
                if t["kind"] == "exact":
                    require(t["status"] == 0 and t["reason"] == old_judge.reference.AUTHORITY
                            and t["same"] and not t["empty"] and t["stats"] == row["stats"],
                            "v2 exact budget")
                    totals["exact_budget_successes"] += 1
                elif t["kind"] == "conflict":
                    require(t["status"] == 1 and t["reason"] == "full_gabriel_lazy_alias_budget_conflict"
                            and t["empty"] and t["cap"] == 1, "alias API conflict")
                    totals["api_conflict_refusals"] += 1
                else:
                    dim = t["dimension"]
                    require(t["status"] == 3 and t["reason"] == old_judge.reference.DIMENSIONS[dim]
                            and t["empty"] and t["cap"] == caps[dim] - 1, "v2 cap-minus-one " + dim)
                    totals["budget_refusals"] += 1
                    totals["successor_budget_refusals"] += dim == "successor_steps"
        else:
            require(not trials, "no unrequested budget trial")
        totals["records"] += 1
        totals["cuts"] += len(row["cuts"])
    return dict(totals)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "build", "run", "judge"))
    parser.add_argument("--name", choices=("O2", "sanitized"), default="O2")
    args = parser.parse_args()
    if args.action == "prepare":
        require(not (R / "source_pins.json").exists() and not W.exists(), "create-only source/build")
        require(runner.sha(runner.ROOT / runner.HEADER) == HEADER, "source capture premise")
        inputs()
        R.mkdir(exist_ok=True)
        W.mkdir()
        runner.prepare(R, W)
        previous = json.loads((OLD / "source_pins.json").read_text())["pins"]
        current = json.loads((R / "source_pins.json").read_text())["pins"]
        require(set(previous) == set(current) and [p for p in previous if previous[p] != current[p]]
                == [str(runner.HEADER)], "single product header delta")
        require(current[str(runner.HEADER)] == HEADER, "captured header")
        files = [old_judge.OUT / "fixtures.json", old_judge.OUT / "fixtures.txt",
                 OLD / "target_fixtures.json", OLD / "target_fixtures.txt",
                 OLD / "O2_output.json", OLD / "target_O2_output.json",
                 A / "full_lazy_bridge.cpp", A / "full_lazy_audit.py",
                 A / "full_producer_run.py", A / "full_singleton_target.py"]
        runner.write(R / "input_binding.json", {"source_header": HEADER,
                     "inputs": {str(p.relative_to(runner.ROOT)): runner.sha(p) for p in files},
                     "scope": "114 prior orders reused only as fixtures; new v2 engines and budgets",
                     "public_status": "not_claimed"})
    elif args.action == "build":
        runner.build(R, W, args.name)
    elif args.action == "run":
        binary = W / (args.name + ".bin")
        env = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
               "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"} if args.name == "sanitized" else {}
        for suffix, fixture in (("", old_judge.OUT / "fixtures.txt"), ("_mixed", OLD / "target_fixtures.txt")):
            result = command([str(binary)], R, args.name + suffix, fixture, env)
            result.update(binary_sha256=runner.sha(binary), input_sha256=runner.sha(fixture))
            runner.write(R / (args.name + suffix + "_run.json"), result)
            require(result["exit_code"] == 0, "engine transport; raw failure retained")
    else:
        legacy, mixed = inputs()
        results = []
        for name in ("O2", "sanitized"):
            total = Counter()
            for suffix, fixture, prior_file in (("", legacy, "O2_output.json"),
                                               ("_mixed", mixed, "target_O2_output.json")):
                path = R / (name + suffix + "_output.json")
                run = json.loads((R / (name + suffix + "_run.json")).read_text())
                require(run["exit_code"] == 0 and run["stdout_sha256"] == runner.sha(path)
                        and not (R / run["stderr"]).read_bytes(), "transport binding")
                rows = json.loads(path.read_text())["records"]
                prior = json.loads((OLD / prior_file).read_text())["records"]
                total.update(judge_rows(rows, fixture["records"], prior))
            require(total["records"] == 912 and total["cuts"] == 69120
                    and total["positive_depth_outputs"] > 0 and total["successor_budget_refusals"] > 0,
                    "Gamma/accounting nonvacuity")
            results.append({"build": name, "counts": dict(total)})
        require(results[0]["counts"] == results[1]["counts"], "build counters")
        for suffix in ("", "_mixed"):
            require((R / ("O2" + suffix + "_output.json")).read_bytes()
                    == (R / ("sanitized" + suffix + "_output.json")).read_bytes(), "build output bytes")
        result = {"status": "passed", "source_header": HEADER, "results": results,
                  "unique_orders": 114, "accounting": ACCOUNTING,
                  "script_sha256": runner.sha(Path(__file__)), "public_status": "not_claimed",
                  "scope": "Fresh Gamma, successful accounting identity and v2 budget boundaries; no formula applied to refused prefixes, no performance or allocation campaign"}
        mode = "optimized" if sys.flags.optimize else "normal"
        runner.write(R / ("judgment_" + mode + ".json"), result)
        print(json.dumps(result))


if __name__ == "__main__":
    main()
