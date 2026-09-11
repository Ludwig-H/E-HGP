"""Bounded exact rank binding and a one-dimensional cache-threshold witness.

Synthetic catalogues do not qualify geometry, ownership, concurrency or the
product producer. The cache example is an arbitrary exact query, not a claimed
authentic FULL occurrence. Every threshold is a squared radius.
"""

from __future__ import annotations

from dataclasses import dataclass, replace
from fractions import Fraction as F
import json
import sys


class Rejection(RuntimeError):
    pass


def need(condition: bool, cause: str) -> None:
    if not condition:
        raise Rejection(cause)


@dataclass(frozen=True)
class Ball:
    key: int
    numerator: int
    denominator: int
    lo: int
    hi: int

    def level(self) -> F:
        need(self.denominator > 0, "nonpositive_denominator")
        return F(self.numerator, self.denominator)


@dataclass(frozen=True)
class Prepared:
    representatives: tuple[int, ...]
    ranks: tuple[tuple[int, int], ...]
    programs: tuple[tuple[int, ...], ...]


def validate(catalogue: tuple[Ball, ...], prepared: Prepared, kmax: int) -> None:
    by_key = {ball.key: ball for ball in catalogue}
    need(len(by_key) == len(catalogue) and bool(catalogue), "catalogue_key_domain")
    need(all(1 <= ball.lo <= ball.hi <= kmax and ball.level() > 0 for ball in catalogue),
         "catalogue_admission_domain")
    reps = prepared.representatives
    need(bool(reps) and len(set(reps)) == len(reps) and all(key in by_key for key in reps),
         "representative_domain")
    need(all(by_key[a].level() < by_key[b].level() for a, b in zip(reps, reps[1:])),
         "representatives_not_strictly_increasing")
    ranks = dict(prepared.ranks)
    need(len(ranks) == len(prepared.ranks) and set(ranks) == set(by_key), "rank_key_domain")
    for ball in catalogue:
        rank = ranks[ball.key]
        need(1 <= rank <= len(reps), "rank_out_of_range")
        need(ball.level() == by_key[reps[rank - 1]].level(), "rank_unbound_to_exact_level")
    need(len(prepared.programs) == kmax, "program_order_domain")
    for k, program in enumerate(prepared.programs, 1):
        eligible = {ball.key for ball in catalogue if ball.lo <= k <= ball.hi}
        need(len(program) == len(eligible) and set(program) == eligible,
             "program_not_permutation")
        need(all((ranks[a], a) < (ranks[b], b) for a, b in zip(program, program[1:])),
             "program_not_ordered")


def prepare(catalogue: tuple[Ball, ...], kmax: int) -> Prepared:
    ordered = sorted(catalogue, key=lambda ball: (ball.level(), ball.key))
    representatives = []
    ranks = []
    previous = None
    for ball in ordered:
        if previous is None or previous != ball.level():
            representatives.append(ball.key)
            previous = ball.level()
        ranks.append((ball.key, len(representatives)))
    rank_of = dict(ranks)
    programs = tuple(tuple(ball.key for ball in sorted(
        (ball for ball in catalogue if ball.lo <= k <= ball.hi),
        key=lambda ball: (rank_of[ball.key], ball.key))) for k in range(1, kmax + 1))
    result = Prepared(tuple(representatives), tuple(sorted(ranks)), programs)
    validate(catalogue, result, kmax)
    return result


def sign(value: int | F) -> int:
    return (value > 0) - (value < 0)


def rank_witness() -> dict[str, object]:
    catalogues = (
        (Ball(0, 2, 2, 1, 1), Ball(1, 1, 1, 2, 3), Ball(2, 3, 2, 1, 3),
         Ball(3, 2, 1, 1, 2), Ball(4, 4, 2, 2, 3)),
        (Ball(90, 3, 4, 1, 3), Ball(30, 6, 8, 2, 3),
         Ball(2, 5, 2, 1, 2), Ball(12, 10, 4, 1, 3)),
        (Ball(7, 9, 3, 1, 3),),
    )
    pairs = programs = cases = 0
    for original in catalogues:
        reference = prepare(original, 3)
        for catalogue in (original, tuple(reversed(original))):
            prepared = prepare(catalogue, 3)
            need(prepared == reference, "input_permutation_changes_prepared_ranks")
            ranks = dict(prepared.ranks)
            for a in catalogue:
                for b in catalogue:
                    need(sign(a.level() - b.level()) == sign(ranks[a.key] - ranks[b.key]),
                         "integer_rank_sign_mismatch")
                    pairs += 1
            for k, actual in enumerate(prepared.programs, 1):
                oracle = tuple(ball.key for ball in sorted(
                    (ball for ball in catalogue if ball.lo <= k <= ball.hi),
                    key=lambda ball: (ball.level(), ball.key)))
                need(actual == oracle, "exact_fraction_program_mismatch")
                programs += 1
            cases += 1
    base = catalogues[0]
    prepared = prepare(base, 3)
    rejected = []

    def reject(name: str, candidate: Prepared, expected: str) -> None:
        try:
            validate(base, candidate, 3)
        except Rejection as error:
            need(str(error) == expected, "unexpected_mutant_cause")
            rejected.append({"mutant": name, "cause": expected})
            return
        raise Rejection("rank_mutant_survived")

    ranks = tuple((key, 2 if key == 0 else rank) for key, rank in prepared.ranks)
    reject("unbound_integer_rank", replace(prepared, ranks=ranks), "rank_unbound_to_exact_level")
    reject("nonincreasing_representatives",
           replace(prepared, representatives=tuple(reversed(prepared.representatives))),
           "representatives_not_strictly_increasing")
    swapped = list(prepared.programs)
    swapped[1] = (1, 2, 4, 3)  # Keys 3 and 4 share the SAME exact level.
    reject("equal_level_key_inversion", replace(prepared, programs=tuple(swapped)), "program_not_ordered")
    duplicated = list(prepared.programs)
    duplicated[0] = (0, 0, 3)  # Same cardinality, omitted key 2.
    reject("duplicate_with_omission", replace(prepared, programs=tuple(duplicated)), "program_not_permutation")
    equivalent = tuple(replace(ball, numerator=3 * ball.numerator, denominator=3 * ball.denominator)
                       if ball.key == 0 else ball for ball in base)
    validate(equivalent, prepared, 3)
    need(equivalent != base and prepare(equivalent, 3) == prepared, "equivalent_raw_level_not_accepted")
    return {"catalogue_variants": cases, "pair_sign_comparisons": pairs,
            "program_comparisons": programs, "causal_rejections": rejected,
            "equivalent_raw_representation_accepted": True}


def cache_witness() -> dict[str, object]:
    points = (0, 2, 3, 4)
    need(len(set(points)) == len(points) and all(0 <= x <= 65535 for x in points),
         "distinct_u16_source_domain")

    def classify(pair: tuple[int, int]) -> tuple[F, F, tuple[int, ...], tuple[int, ...]]:
        a, b = pair
        need(a in points and b in points and a < b, "pair_domain")
        center = F(a + b, 2)
        radius_squared = F((b - a) ** 2, 4)
        interior, shell = [], []
        for x in points:
            power = (F(x) - center) ** 2 - radius_squared
            integer_four_power = (2 * x - a - b) ** 2 - (b - a) ** 2
            need(4 * power == integer_four_power, "independent_power_formula_mismatch")
            if integer_four_power < 0:
                interior.append(x)
            elif integer_four_power == 0:
                shell.append(x)
        return center, radius_squared, tuple(interior), tuple(shell)

    initial_pair = (0, 4)
    initial = classify(initial_pair)
    need(initial == (F(2), F(4), (2, 3), (0, 4)), "initial_exact_geometry")
    intruder = 2
    need(intruder in initial[2], "exchange_requires_strict_intruder")
    terminal_pair = tuple(sorted((intruder, initial_pair[1])))
    terminal = classify(terminal_pair)
    need(terminal == (F(3), F(1), (3,), (2, 4)), "terminal_exact_geometry")
    initial_window = (len(initial[2]) + 2 - 1, len(initial[2]) + len(initial[3]))
    terminal_window = (len(terminal[2]) + 2 - 1, len(terminal[2]) + len(terminal[3]))
    need(initial_window == (3, 4) and terminal_window == (2, 3), "exact_admission_windows")
    K = 2
    need(not initial_window[0] <= K <= initial_window[1]
         and terminal_window[0] <= K <= terminal_window[1], "K2_admission_distinction")

    def fresh_query(before: F) -> bool:
        # Initial MEB bound is checked BEFORE the exchange to the final ball.
        return initial[1] < before and terminal[1] < before

    old, new = F(5), F(2)
    need(fresh_query(old), "old_query_must_succeed")
    terminal_only_accepts = terminal[1] < new
    need(terminal_only_accepts and not fresh_query(new), "cache_mutant_nonvacuity")
    safe_bounds = (F(5), F(11, 2), F(6), F(10))
    need(initial[1] < old and terminal[1] < old, "old_bound_certificate")
    for before in safe_bounds:
        need(before >= old and fresh_query(before), "nondecreasing_before_reuse")
    need(not new >= old, "stored_before_guard_must_reject_new_query")
    fallback_before = F(9, 2)
    fallback_fresh_accepted = fresh_query(fallback_before)
    fallback_direct_reuse_allowed = fallback_before >= old
    need(initial[1] < fallback_before < old and fallback_fresh_accepted
         and not fallback_direct_reuse_allowed, "cache_miss_requires_fresh_fallback")
    return {"source_u16": points, "K": K, "initial_pair": initial_pair,
            "initial_center": str(initial[0]), "initial_radius_squared": str(initial[1]),
            "initial_interior": initial[2], "initial_shell": initial[3], "initial_window": initial_window,
            "exchange": {"removed_first_support": 0, "inserted_strict_intruder": intruder},
            "terminal_pair": terminal_pair, "terminal_center": str(terminal[0]),
            "terminal_radius_squared": str(terminal[1]), "terminal_interior": terminal[2],
            "terminal_shell": terminal[3], "terminal_window": terminal_window,
            "old_before_squared": str(old), "new_before_squared": str(new),
            "old_fresh_accepted": True, "new_fresh_accepted": False,
            "mutant_terminal_only_accepted": terminal_only_accepts,
            "stored_before_guard_rejected": True, "safe_bounds_tested": list(map(str, safe_bounds)),
            "fallback_before_squared": str(fallback_before),
            "fallback_fresh_accepted": fallback_fresh_accepted,
            "fallback_direct_reuse_allowed": fallback_direct_reuse_allowed,
            "independent_power_classifications": 8,
            "scope": "arbitrary exact 1D query; not an authenticated FULL occurrence or producer execution"}


def main(argv: list[str]) -> int:
    if argv != ["--selftest"]:
        return 2
    try:
        ranks, cache = rank_witness(), cache_witness()
        need(ranks["catalogue_variants"] == 6 and ranks["pair_sign_comparisons"] == 84
             and ranks["program_comparisons"] == 18 and len(ranks["causal_rejections"]) == 4,
             "bounded_nonvacuity")
        print(json.dumps({"status": "passed_bounded_exact_model", "rank_binding": ranks,
                          "cache_before": cache, "causal_mutants": 5,
                          "not_claimed": ["geometric catalogue validation", "prepared owner implementation",
                                          "authentic FULL request", "product execution", "performance"]},
                         indent=2, sort_keys=True))
    except Rejection as error:
        print(json.dumps({"status": "failed", "cause": str(error)}, sort_keys=True), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
