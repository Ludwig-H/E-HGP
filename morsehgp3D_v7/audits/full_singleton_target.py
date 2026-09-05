"""Independent singleton calendar and rational mixed-birth supplement."""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from fractions import Fraction as Q
import json
from pathlib import Path
import sys

import full_lazy_audit as judge
import full_singleton_run as replay


R = replay.RECEIPT
require = judge.require


def fixtures() -> dict:
    # Coordinates from the constructor's proposed mixed fixture; all subsets,
    # Gabriel catalogues and Gamma cuts are independently recalculated here.
    case = dict(name="singleton_mixed_birth", ids=[0, 17, 41, 63, 107],
                points=[[25,20,20], [17,24,20], [17,16,20], [120,20,20], [130,20,20]])
    events, oracle = judge.reference.catalogue(case)
    records = []
    for k in range(1, 6):
        minimum = [e for e in events if len(e["label"]) == k] if k > 1 else []
        direct = [e for e in events if len(e["label"]) == k + 1]
        expected = judge.reference.reference_forest(oracle, k)
        for variant in (0, 1):
            def encode(source: list[dict]) -> list[dict]:
                rows = [dict(q=e["q"], d=e["d"], mask=e["mask"], support=e["support"],
                             interior=e["interior"], **judge.reference.level(Q(e["squared_level"]),
                                                                           1 if not variant else i+11))
                        for i, e in enumerate(source)]
                return rows if not variant else rows[::-1]
            points = [dict(id=i, xyz=p) for i, p in zip(case["ids"], case["points"])]
            for lazy, cap in judge.MODES:
                records.append(dict(id=len(records), order=k, representation=variant,
                                    lazy=lazy, capacity=cap, budget_probe=0,
                                    points=points if not variant else points[::-1],
                                    minima_source=encode(minimum), direct_source=encode(direct),
                                    cuts=expected["cuts"], expected=expected,
                                    expected_stats=judge.reference.independent_stats(case, k, minimum, direct, oracle)))
    at25 = [n for n in records[8]["expected"]["nodes"] if judge.reference.rational(n) == 25]
    require([n["parent_count"] for n in at25] == [0, 3], "birth precedes fusion at level25")
    return dict(case=case, regular_subsets=len(oracle.levels), events=events, records=records)


def calendar() -> dict:
    f = judge.load_fixtures()
    counts, examples, future = Counter(), {}, []
    for r in f["records"]:
        if r["lazy"] or r["representation"]:
            continue
        events = [e for e in f["catalogues"][r["case_index"]]["events"]
                  if len(e["label"]) == r["order"]+1]
        groups = defaultdict(list)
        for e in events:
            groups[Q(e["squared_level"])].append(e)
        for value, items in groups.items():
            if len(items) != 1:
                counts["multi_direct_lots"] += 1
                continue
            e = items[0]
            nodes = [n for n in r["expected"]["nodes"] if judge.reference.rational(n) == value]
            merges = [n for n in nodes if n["parent_count"]]
            require(len(merges) <= 1, "at most one singleton merge")
            u = merges[0]["parent_count"] if merges else 1
            require(1 <= u <= e["q"] <= 4, "one to q parents")
            keys = [f"q{e['q']}", f"U{u}"]
            if 1 < u < e["q"]:
                keys.append("proper_repeated")
            if any(not n["parent_count"] for n in nodes):
                keys.append("simultaneous_births")
            witness = dict(case=f["cases"][r["case_index"]]["name"], K=r["order"],
                           level=str(value), q=e["q"], U=u)
            for key in keys:
                counts[key] += 1
                examples.setdefault(key, witness)
            if u != 1:
                continue
            # A later essential removal equals a strict non-minimum facet of
            # this silent direct. C0 must resolve its unique missing intruder
            # through this direct's closed anchor, regardless of cache hits elsewhere.
            for later in events:
                if Q(later["squared_level"]) <= value:
                    continue
                for drop in later["support"]:
                    facet = set(later["label"]) - {drop}
                    if set(e["support"]) <= facet < set(e["label"]):
                        future.append(dict(witness, T=e["label"], F=sorted(facet),
                                           S=later["label"], next_level=later["squared_level"]))
    require(counts["q4"] == 22 and counts["U4"] == 19 and counts["proper_repeated"] == 42,
            "nonvacuous independent singleton calendar")
    require(counts["simultaneous_births"] == 0 and len(future) == 5,
            "one missing category, five future silent-anchor consumptions")
    return dict(counts_per_109_orders=dict(counts), first_examples=examples,
                future_noop_anchor_witnesses=future,
                scope="Exact catalogue/Gamma-derived calendar, not private-hook observations")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "run", "judge"))
    parser.add_argument("--name", choices=("O2", "sanitized"), default="O2")
    args = parser.parse_args()
    if args.action == "prepare":
        data = fixtures()
        replay.runner.write(R / "target_fixtures.json", data)
        (R / "target_fixtures.txt").write_text(judge.fixture_text(data))
        replay.runner.write(R / "calendar.json", calendar())
    elif args.action == "run":
        binary = replay.WORK / (args.name + ".bin")
        env = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
               "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"} if args.name == "sanitized" else {}
        result = replay.command([str(binary)], R, "target_"+args.name, R/"target_fixtures.txt", env)
        result.update(binary_sha256=replay.runner.sha(binary), input_sha256=replay.runner.sha(R/"target_fixtures.txt"))
        replay.runner.write(R/("target_"+args.name+"_run.json"), result)
        require(result["exit_code"] == 0, "target engine transport")
    else:
        data = fixtures()
        require(data == json.loads((R/"target_fixtures.json").read_text()), "rational fixture replay")
        totals = []
        for name in ("O2", "sanitized"):
            path = R/("target_"+name+"_output.json")
            run = json.loads((R/("target_"+name+"_run.json")).read_text())
            require(run["exit_code"] == 0 and replay.runner.sha(path) == run["stdout_sha256"]
                    and not (R/run["stderr"]).read_bytes(), "target transport binding")
            rows = json.loads(path.read_text())["records"]
            require(len(rows) == len(data["records"]) == 40, "target record count")
            cuts = 0
            for row, r in zip(rows, data["records"]):
                require(row["id"] == r["id"] and row["order"] == r["order"] and row["status"] == 0
                        and row["reason"] == judge.reference.AUTHORITY, "target identity/status")
                require(row["alias_policy"] == judge.POLICIES[r["lazy"]]
                        and row["cache_capacity"] == r["capacity"], "target policy")
                expected = r["expected"]
                require(len(row["nodes"]) == len(expected["nodes"]), "target nodes")
                for n, wanted in zip(row["nodes"], expected["nodes"]):
                    require(judge.reference.rational(n) == judge.reference.rational(wanted)
                            and (n["first"], n["parent_count"]) == (wanted["first"], wanted["parent_count"]),
                            "target ordered births and merges")
                for key in ("parents", "minima", "coverage"):
                    require(row[key] == expected[key], "target "+key)
                require(len(row["cuts"]) == len(expected["roots"]), "target cuts")
                for c, roots in zip(row["cuts"], expected["roots"]):
                    require(c["status"] == 0 and c["reason"] == "structural_only" and c["roots"] == roots,
                            "target exact Gamma cut")
                judge.check_stats(r, row["stats"])
                require(not row["budget_trials"], "no target budget claim")
                cuts += len(row["cuts"])
            require(cuts > 0, "target nonvacuity")
            totals.append(dict(build=name, records=40, cuts=cuts, mixed_K2_representations=8))
        require((R/"target_O2_output.json").read_bytes() == (R/"target_sanitized_output.json").read_bytes(),
                "target sanitizer equivalence")
        mode = "optimized" if sys.flags.optimize else "normal"
        result = dict(status="passed", outcomes=totals, public_status="not_claimed", source_header=replay.EXPECTED,
                      script_sha256=replay.runner.sha(Path(__file__)),
                      scope="Five additional orders from 26 independently checked subsets; no new allocation or performance claim")
        replay.runner.write(R/("target_judgments_"+mode+".json"), result)
        print(json.dumps(result))


if __name__ == "__main__":
    main()
