"""Read the captured native microcost; never invoke a compiler or measured code."""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from fractions import Fraction
import gzip
import hashlib
import json
from pathlib import Path
from statistics import median
import sys


HERE = Path(__file__).resolve().parent
INPUT = HERE.parent / "qualification/snapshots/run/measurement.stdout.gz"
RAW_SHA = "2c20ceaf7a8a4757af2ad78554becf2e584f1c397e92860800d6c746de24469f"


def require(condition: bool, label: str) -> None:
    if not condition:
        raise ValueError(label)


def unique_object(pairs: list) -> dict:
    result = {}
    for key, value in pairs:
        require(key not in result, "input.duplicate_key")
        result[key] = value
    return result


def distribution(values: list[Fraction]) -> dict:
    ordered = sorted(values)
    middle = len(ordered) // 2
    lower = ordered[:middle]
    upper = ordered[middle + (len(ordered) % 2):]
    return {key: str(value) for key, value in {
        "median": median(ordered), "q1": median(lower), "q3": median(upper),
        "minimum": ordered[0], "maximum": ordered[-1],
    }.items()}


def paired_statistics(pairs: dict, calls: int, tick: int) -> dict:
    require(set(pairs) == set(range(1, 8)), "pair.seven_passes_required")
    require(all(set(pairs[p]) == {"F", "dual"} for p in pairs), "pair.two_arms_required")
    raw = [[pairs[p]["F"], pairs[p]["dual"]] for p in range(1, 8)]
    require(calls > 0 and all(a > 0 and b > 0 for a, b in raw), "pair.positive_times")
    ratios = [Fraction(b, a) for a, b in raw]
    differences = [Fraction(b - a, calls) for a, b in raw]
    return {
        "calls_per_arm_pass": calls,
        "paired_elapsed_ns": raw,
        "F_ns_per_call": distribution([Fraction(a, calls) for a, _ in raw]),
        "dual_ns_per_call": distribution([Fraction(b, calls) for _, b in raw]),
        "dual_minus_F_ns_per_call": distribution(differences),
        "dual_over_F": distribution(ratios),
        "AB_odd_passes_dual_over_F": distribution(ratios[::2]),
        "BA_even_passes_dual_over_F": distribution(ratios[1::2]),
        "nonshort_pairs": sum(a >= 100 * tick and b >= 100 * tick for a, b in raw),
        "all_seven_dual_faster": all(b < a for a, b in raw),
        "all_seven_dual_slower": all(b > a for a, b in raw),
        "interpretation": "descriptive within one run; nonshort is only the protocol diagnostic",
    }


def run(path: Path) -> tuple[dict, list]:
    raw = gzip.decompress(path.read_bytes()) if path.suffix == ".gz" else path.read_bytes()
    require(len(raw) == 28972744 and hashlib.sha256(raw).hexdigest() == RAW_SHA, "input.pinned_bytes")
    rows = [json.loads(line, object_pairs_hook=unique_object) for line in raw.splitlines()]
    header, terminal = rows[0], rows[-1]
    kinds = Counter(row["kind"] for row in rows)
    require(kinds == {"header": 1, "case": 9351, "group": 4699, "timing": 84582, "terminal": 1},
            "input.record_inventory")
    require(header["NoObserver"] is True and terminal["status"] == "completed", "input.terminal")
    cases = [row for row in rows if row["kind"] == "case"]
    groups = [row for row in rows if row["kind"] == "group"]
    timings = [row for row in rows if row["kind"] == "timing"]
    by_id = defaultdict(list)
    for row in cases:
        by_id[row["id"]].append(row)
    require(set(by_id) == set(range(9347)), "jobs.closed_inventory")
    require(all([row["step"] for row in states] == list(range(len(states))) for states in by_id.values()),
            "jobs.contiguous_steps")
    cohort_states = Counter(row["cohort"] for row in cases)
    require(cohort_states == {"main": 9216, "boundary": 117, "cumulative_P7": 4,
                              "cumulative_P0": 2, "immediate_q2": 12}, "jobs.cohorts")
    main = [row for row in cases if row["cohort"] == "main"]
    per_order = defaultdict(list)
    for row in main:
        per_order[row["order"]].append(row)
    require(set(per_order) == set(range(384)), "main.order_inventory")
    for states in per_order.values():
        ranks = {row["R"] for row in states}
        require(len(ranks) == 1 and len(states) == 24, "main.single_rank")
        rank = next(iter(ranks))
        require({(row["P"], row["L"]) for row in states} ==
                {(p, l) for p in (0, 1, 4, 5, 15, 16, 25, 401) for l in (rank - 1, rank, rank + 1)},
                "main.complete_cap_matrix")
    qref = {order: next(row["q_result"] for row in states if row["P"] == 0 and row["L"] == row["R"])
            for order, states in per_order.items()}
    qref[384] = 3
    seen = []
    populations = {}
    for group in groups:
        ids = group["jobs"]
        seen.extend(ids)
        calls = nested = 0
        for ident in ids:
            states = by_id[ident]
            repeats = 4096 if states[0]["cohort"] == "immediate_q2" else 1
            calls += repeats * len(states)
            nested += repeats * sum(row["fallback_delta"] for row in states)
        require(calls > 0, "group.nonempty")
        populations[group["group"]] = (calls, nested, ids)
    require(sorted(seen) == list(range(9347)), "group.unique_job_coverage")
    require(len(populations) == 4699 and list(populations) == sorted(populations), "group.unique_order")
    require(sum(value[0] for value in populations.values()) == 58491, "group.calls_per_arm")
    pairs = defaultdict(lambda: defaultdict(dict))
    cursor = timed_entries = 0
    for warmup, passes in ((True, 2), (False, 7)):
        for passage in range(1, passes + 1):
            for key, (calls, nested, _) in populations.items():
                for arm in (("F", "dual") if passage % 2 else ("dual", "F")):
                    row = timings[cursor]
                    cursor += 1
                    require((row["group"], row["arm"], row["pass"], row["warmup"]) ==
                            (key, arm, passage, warmup), "timing.paired_order")
                    require(row["calls"] == calls and row["nested_F_calls"] == (nested if arm == "dual" else 0),
                            "timing.population_and_nested_entries")
                    require(type(row["elapsed_ns"]) is int and row["elapsed_ns"] > 0 and
                            row["short_batch"] is (row["elapsed_ns"] < 100 * terminal["clock_tick_ns"]),
                            "timing.clock_diagnostic")
                    timed_entries += calls + row["nested_F_calls"]
                    if not warmup:
                        pairs[key][passage][arm] = row["elapsed_ns"]
    require(cursor == len(timings), "timing.no_omitted_pass")
    fallback = sum(row["fallback_delta"] for row in cases)
    boundary_fallback = sum(row["fallback_delta"] for row in cases
                            if row["cohort"] not in ("main", "immediate_q2"))
    # Account separately for both pairs of off-clock arms and the donor replay.
    accounting = {
        "reference_rank_entries": 384,
        "offclock_F_Trace_F_NoObserver_top_level": 2 * 4 * len(cases),
        "offclock_nested_reference_entries": 2 * 2 * fallback,
        "offclock_donor_boundary_top_level": 2 * 2 * 123,
        "offclock_donor_nested_reference_entries": 2 * boundary_fallback,
        "timed_and_warmup_top_level_and_nested": timed_entries,
    }
    require(sum(accounting.values()) == terminal["helper_entries"] == 1325812, "ledger.closed_sum")
    results = []
    for key, (calls, nested, ids) in populations.items():
        result = paired_statistics(pairs[key], calls, terminal["clock_tick_ns"])
        result.update(group=key, jobs=ids, nested_F_entries_per_dual_pass=nested)
        results.append(result)
    physical = []
    for p in (0, 1, 4, 5, 15, 16, 25, 401):
        selected = [row for row in main if row["P"] == p]
        f = sum(row["legacy_delta"] for row in selected)
        dual = sum(row["actual_F_fallback_candidates"] + row["proposal_delta"] for row in selected)
        signs = Counter((row["actual_F_fallback_candidates"] + row["proposal_delta"] > row["legacy_delta"]) -
                        (row["actual_F_fallback_candidates"] + row["proposal_delta"] < row["legacy_delta"])
                        for row in selected)
        physical.append({"P": p, "main_cases": len(selected), "F_candidate_attempts": f,
                         "dual_candidate_attempts_A_plus_delta_P": dual,
                         "dual_over_F_attempts": str(Fraction(dual, f)),
                         "fewer": signs[-1], "equal": signs[0], "more": signs[1]})
    exploratory = []
    for result in results:
        states = [by_id[ident][0] for ident in result["jobs"]]
        if all(row["cohort"] == "main" and row["P"] == 401 and row["n"] >= 8 and
               qref[row["order"]] == 4 for row in states):
            exploratory.append(result)
    nonshort = [row for row in exploratory if row["nonshort_pairs"] == 7]
    require(len(exploratory) == 144 and len(nonshort) == 44, "exploratory.nonvacuity")
    require(all(len(row["paired_elapsed_ns"]) == 7 for row in results), "statistics.nonvacuity")
    p0_main = [row for row in results if row["group"].startswith("main/") and "/P=0/" in row["group"]]
    def order_class(row: dict) -> str:
        ab = Fraction(row["AB_odd_passes_dual_over_F"]["median"])
        ba = Fraction(row["BA_even_passes_dual_over_F"]["median"])
        if ab < 1 < ba:
            return "AB_below_one_BA_above_one"
        if ba < 1 < ab:
            return "BA_below_one_AB_above_one"
        if ab < 1 and ba < 1:
            return "both_below_one"
        if ab > 1 and ba > 1:
            return "both_above_one"
        return "at_least_one_exact_tie"
    p0_classes = Counter(order_class(row) for row in p0_main)
    p0_nonshort = [row for row in p0_main if row["nonshort_pairs"] == 7]
    p0_example = next(row for row in p0_main if row["group"] ==
                      "main/n=9/qref=4/P=0/L=210/c=0/p=0/pivot=16/terminal=legacy_refused/route=initial_P_fallback")
    require(len(p0_main) == 556 and len(p0_nonshort) == 207 and
            p0_classes["AB_below_one_BA_above_one"] == 447 and
            p0_classes["BA_below_one_AB_above_one"] == 11, "P0.order_control_nonvacuity")
    faults = []
    synthetic = {p: {"F": p, "dual": 9 - p} for p in range(1, 8)}
    for name, mutated in (("missing_seventh_pair", {p: v for p, v in synthetic.items() if p != 7}),
                          ("missing_dual_arm", {p: ({"F": p} if p == 3 else v) for p, v in synthetic.items()})):
        try:
            paired_statistics(mutated, 1, 1)
        except ValueError as error:
            faults.append({"name": name, "rejected_by": str(error), "kind": "audit_data_corruption"})
        else:
            raise ValueError("fault.not_detected")
    # This statistic cannot silently become ratio-of-unpaired-medians.
    example = [(1, 10), (10, 11), (100, 1), (1, 10), (10, 11), (100, 1), (10, 11)]
    example_pairs = {p: {"F": a, "dual": b} for p, (a, b) in enumerate(example, 1)}
    correct_ratio = Fraction(paired_statistics(example_pairs, 1, 1)["dual_over_F"]["median"])
    require(correct_ratio == Fraction(11, 10) and correct_ratio !=
            Fraction(median(b for _, b in example), median(a for a, _ in example)),
            "statistics.paired_not_ratio_of_medians")
    faults.append({"name": "ratio_of_unpaired_medians", "rejected_by": "exact counterfixture",
                   "kind": "audit_statistic_corruption"})
    summary = {
        "status": "completed", "python_optimization": sys.flags.optimize,
        "scope": "replay of captured timings; no new MEB entry, compiler, benchmark, or global claim",
        "public_status": "not_claimed", "gcp_used": False, "raw_sha256": RAW_SHA,
        "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "records": dict(kinds), "cohort_states": dict(cohort_states),
        "helper_entry_accounting": accounting, "helper_entries_total": sum(accounting.values()),
        "calls_per_arm_pass": 58491, "immediate_q2_calls_per_arm_pass": 49152,
        "warmups": 2, "measured_passes": 7, "AB_passes": [1, 3, 5, 7], "BA_passes": [2, 4, 6],
        "clock_pair_min_ns": terminal["clock_tick_ns"], "protocol_short_batch_threshold_ns": 100 * terminal["clock_tick_ns"],
        "groups_by_call_count": dict(sorted(Counter(row["calls_per_arm_pass"] for row in results).items())),
        "groups_by_nonshort_pairs": dict(sorted(Counter(row["nonshort_pairs"] for row in results).items())),
        "physical_main_by_P": physical,
        "P0_main_order_control": {
            "selection": "All main P0 groups, distinct from immediate_q2 repeated jobs; descriptive AB/BA medians.",
            "groups": len(p0_main), "cases": sum(len(row["jobs"]) for row in p0_main),
            "AB_BA_classes": dict(p0_classes), "groups_with_14_nonshort_batches": len(p0_nonshort),
            "AB_BA_classes_14_nonshort_batches": dict(Counter(order_class(row) for row in p0_nonshort)),
            "example": p0_example,
        },
        "immediate_q2": [row for row in results if row["group"].startswith("immediate_q2/")],
        "exploratory_n_ge_8_qref4_P401": {
            "selection": "posthoc descriptive; all main groups with n>=8, reference q=4, P=401; not a production threshold",
            "cases": sum(len(row["jobs"]) for row in exploratory), "groups": len(exploratory),
            "groups_with_14_nonshort_batches": len(nonshort),
            "all_seven_dual_faster": sum(row["all_seven_dual_faster"] for row in nonshort),
            "all_seven_dual_slower": sum(row["all_seven_dual_slower"] for row in nonshort),
            "mixed": sum(not row["all_seven_dual_faster"] and not row["all_seven_dual_slower"] for row in nonshort),
            "distribution_of_group_median_ratios_nonshort": distribution([Fraction(row["dual_over_F"]["median"]) for row in nonshort]),
            "nonshort_group_keys": [row["group"] for row in nonshort],
        },
        "statistics": "Exact rational medians; Q1/Q3 are medians of sorted halves excluding the middle value for odd n. AB has four values, BA three. All seven paired differences/ratios remain descriptive; no independent-seed inference.",
        "audit_faults": faults,
    }
    return summary, results


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=INPUT)
    args = parser.parse_args()
    summary, groups = run(args.input)
    selected_keys = set(summary["exploratory_n_ge_8_qref4_P401"]["nonshort_group_keys"])
    selected = [row for row in groups if row["group"].startswith("immediate_q2/") or row["group"] in selected_keys]
    require(len(selected) == 50, "export.selected_inventory")
    payload = gzip.compress((json.dumps(selected, ensure_ascii=False, separators=(",", ":")) + "\n").encode(), mtime=0)
    paired_path = HERE / "selected_groups.json.gz"
    if paired_path.exists():
        require(paired_path.read_bytes() == payload, "replay.paired_data_changed")
    else:
        paired_path.write_bytes(payload)
    summary["selected_groups_sha256"] = hashlib.sha256(payload).hexdigest()
    summary["retention"] = "All 4699 groups calculated from the unchanged raw log; detailed export keeps the six repeated q2 groups and all 44 exploratory nonshort q4 groups, including slower/mixed groups."
    output = HERE / ("optimized.json" if sys.flags.optimize else "normal.json")
    output.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps({"status": summary["status"], "optimization": sys.flags.optimize,
                      "groups": len(groups), "helper_entries": summary["helper_entries_total"]}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
