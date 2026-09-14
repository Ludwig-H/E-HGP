#!/usr/bin/env python3
"""Independent mono model of suspendable q2 collection, not a product API.

One borrowed index and constant continuation state; only the judge accumulates
IDs. Each call budgets classifications plus proposed IDs, including refusals.
No CUDA, timing, RSS, or end-to-end complexity claim follows from this model.
"""

from __future__ import annotations

from dataclasses import dataclass
from itertools import permutations, product
import json

from p0_q2_census_bounds_probe import (
    Node, box_bounds, build_tree, classify, power_value, require, singleton,
    thread_tree, tree_census,
)


@dataclass
class Collector:
    points: tuple
    support: tuple[int, int]
    index: tuple
    expected_inside: int
    mutant: str = ""
    cursor: int = 0
    pending: int = -1
    offset: int = 0
    tag: str = ""
    inside: int = 0
    shell: int = 0
    seq: int = 0
    done: bool = False

    def snapshot(self) -> tuple:
        return (self.cursor, self.pending, self.offset, self.tag,
                self.inside, self.shell, self.seq, self.done)

    def step(self, quantum: int, capacity: int, offer) -> dict:
        require(quantum > 0 and capacity > 0, "positive call bounds required")
        require(not self.done, "collection already complete")
        work = {key: 0 for key in ("visits", "proposed", "accepted", "refusals")}
        remaining = quantum
        while True:
            at_end = self.cursor == len(self.index)
            if at_end and self.mutant == "drop_pending_at_done":
                self.pending = -1
            if at_end and (self.pending < 0 or self.mutant == "emit_done_before_drain"):
                require(self.inside == self.expected_inside, "terminal interior count mismatch")
                terminal = {"kind": "complete", "attempt": 0,
                            "support": self.support, "seq": self.seq,
                            "inside": self.inside, "shell": self.shell}
                if offer(terminal):
                    self.done = True
                else:
                    work["refusals"] += 1
                return work
            if remaining == 0:
                if self.mutant == "reset_offset_after_yield" and self.pending >= 0:
                    self.offset = 0
                return work
            if self.pending >= 0:
                ids = self.index[self.pending].node.ids
                count = min(capacity, remaining, len(ids) - self.offset)
                fragment = {"kind": "tentative", "attempt": 0,
                            "support": self.support, "seq": self.seq,
                            "tag": self.tag, "ids": ids[self.offset:self.offset + count]}
                # A refused materialized fragment costs its proposed IDs too.
                remaining -= count
                work["proposed"] += count
                if not offer(fragment):
                    work["refusals"] += 1
                    if self.mutant == "advance_on_rejected_chunk":
                        self.offset += count
                    return work
                self.offset += count
                self.seq += 1
                work["accepted"] += count
                if self.tag == "interior":
                    self.inside += count
                else:
                    self.shell += count
                if self.offset == len(ids):
                    self.pending, self.offset, self.tag = -1, 0, ""
                continue
            position = self.cursor
            entry = self.index[position]
            a, b = (singleton(self.points[i]) for i in self.support)
            decision = classify(*box_bounds(a, b, entry.node.box))
            work["visits"] += 1
            remaining -= 1
            if decision == "uncertain":
                require(bool(entry.node.children), "undecided singleton")
                self.cursor += 1
            else:
                self.cursor = entry.escape
                if decision != "exterior":
                    self.pending, self.offset, self.tag = position, 0, decision


def run(points: tuple, support: tuple, root: Node, expected: int,
        quantum: int, capacity: int, refusal: str, mutant: str = "") -> dict:
    """Judge owns the unbounded output; the producer never retains it."""
    collector = Collector(points, support, thread_tree(root), expected, mutant)
    output, seen, terminal = [], set(), []
    attempts = accepted_fragments = 0
    totals = {key: 0 for key in ("visits", "proposed", "accepted", "refusals")}
    rejected_snapshot = None
    terminal_refused = False

    def offer(record: dict) -> bool:
        nonlocal attempts, accepted_fragments, rejected_snapshot, terminal_refused
        attempts += 1
        require(record["support"] == support and record["attempt"] == 0,
                "fragment identity mismatch")
        require(record["seq"] == accepted_fragments, "fragment sequence mismatch")
        reject = ((refusal == "once" and attempts == 1)
                  or (refusal == "alternating" and attempts <= 8 and attempts % 2 == 1)
                  or (refusal == "terminal_once" and record["kind"] == "complete"
                      and not terminal_refused))
        if reject:
            terminal_refused |= record["kind"] == "complete"
            rejected_snapshot = collector.snapshot()
            return False
        if record["kind"] == "complete":
            require(not terminal, "terminal duplicated")
            terminal.append(record)
        else:
            require(not terminal and 0 < len(record["ids"]) <= capacity,
                    "fragment outside bounds")
            for site in record["ids"]:
                require(site not in seen, "accepted ID duplicated")
                seen.add(site)
                output.append((record["tag"], site))
            accepted_fragments += 1
        return True

    # Even quantum=capacity=1 and all allowed refusals complete within this
    # bound; mutants cannot stall this audit indefinitely.
    call_limit = len(collector.index) + 2 * len(points) + 32
    for calls in range(1, call_limit + 1):
        rejected_snapshot = None
        work = collector.step(quantum, capacity, offer)
        require(work["visits"] + work["proposed"] <= quantum, "call budget exceeded")
        require(work["accepted"] <= work["proposed"], "unaccounted accepted copies")
        if rejected_snapshot is not None:
            require(collector.snapshot() == rejected_snapshot, "refusal advanced continuation")
        for key in totals:
            totals[key] += work[key]
        if collector.done:
            break
    else:
        raise RuntimeError("bounded replay failed to finish")
    powers = [power_value(points[support[0]], points[support[1]], z) for z in points]
    inside = [i for i, value in enumerate(powers) if value > 0]
    shell = [i for i, value in enumerate(powers) if value == 0]
    require(sorted(i for tag, i in output if tag == "interior") == inside,
            "interior oracle mismatch")
    require(sorted(i for tag, i in output if tag == "shell") == shell,
            "shell oracle mismatch")
    require(len(terminal) == 1 and terminal[0]["inside"] == len(inside)
            and terminal[0]["shell"] == len(shell), "terminal payload mismatch")
    exact = tree_census(points, *support, root, None)
    require(exact["interior"] == inside and exact["shell"] == shell,
            "unbudgeted collector differs from rational oracle")
    require(totals["visits"] == exact["visits"], "classification repeated or omitted")
    require(totals["accepted"] == len(inside) + len(shell), "accepted-copy work mismatch")
    return {"calls": calls, "fragments": accepted_fragments, "output": output, **totals}


def fixtures() -> list:
    sphere = tuple(sorted({
        tuple(10 + sign * value for sign, value in zip(signs, xyz))
        for base in ((5, 0, 0), (4, 3, 0))
        for xyz in permutations(base) for signs in product((-1, 1), repeat=3)
    }))
    require(len(sphere) == 30, "large-shell fixture size")
    return [
        ("shell_thirty", sphere,
         (sphere.index((5, 10, 10)), sphere.index((15, 10, 10))), 1),
        ("empty_interior", ((0, 0, 0), (2, 0, 0), (1, 1, 0), (9, 9, 9)), (0, 1), 1),
        ("half_center", ((0, 0, 0), (5, 1, 0), (2, 0, 0), (3, 1, 0),
                         (0, 1, 0), (5, 0, 0), (20, 20, 20)), (0, 1), 5),
        ("interior_block", ((10, 10, 10), (30, 10, 10))
         + tuple((x, 10, 10) for x in range(17, 24)) + ((50, 50, 50),), (0, 1), 10),
    ]


def selftest() -> dict:
    runs = calls = refusals = proposed = accepted = 0
    largest_inside_block = largest_shell = 0
    mutants = {name: None for name in (
        "drop_pending_at_done", "reset_offset_after_yield", "advance_on_rejected_chunk",
        "emit_done_before_drain", "incorrect_expected_count",
    )}
    for name, points, support, threshold in fixtures():
        root = build_tree(points, tuple(range(len(points))))
        index = thread_tree(root)
        powers = [power_value(points[support[0]], points[support[1]], z) for z in points]
        expected = sum(value > 0 for value in powers)
        require(expected < threshold, "fixture is not admitted at Kmax")
        largest_shell = max(largest_shell, sum(value == 0 for value in powers))
        a, b = (singleton(points[i]) for i in support)
        largest_inside_block = max(largest_inside_block, max(
            (len(entry.node.ids) for entry in index
             if classify(*box_bounds(a, b, entry.node.box)) == "interior"), default=0))
        baseline = run(points, support, root, expected, 10000, 10000, "never")
        require(baseline["calls"] == 1, "unbudgeted baseline yielded")
        for quantum, capacity, policy in product((1, 2, 5, 17), (1, 2, 7),
                                                  ("never", "once", "alternating", "terminal_once")):
            row = run(points, support, root, expected, quantum, capacity, policy)
            require(row["output"] == baseline["output"], "budget changed typed ID stream")
            require(row["visits"] == baseline["visits"]
                    and row["accepted"] == baseline["accepted"], "budget changed useful work")
            require(policy == "never" or row["refusals"] > 0, "refusal policy vacuous")
            runs += 1
            calls += row["calls"]
            refusals += row["refusals"]
            proposed += row["proposed"]
            accepted += row["accepted"]
        for mutant in mutants:
            if mutants[mutant] is not None:
                continue
            try:
                wrong_expected = expected + int(mutant == "incorrect_expected_count")
                run(points, support, root, wrong_expected, 2, 1, "alternating", mutant)
            except RuntimeError as error:
                mutants[mutant] = {"fixture": name, "rejection": str(error)}
    require(runs == 192 and largest_shell == 30 and largest_inside_block > 2,
            "collection fixtures lack required coverage")
    require(all(mutants.values()) and proposed > accepted and refusals > 0,
            "mutant or refusal coverage vacuous")
    for quantum, capacity in ((0, 1), (1, 0), (-1, 1)):
        name, points, support, _ = fixtures()[1]
        collector = Collector(points, support, thread_tree(build_tree(points, (0, 1, 2, 3))), 0)
        try:
            collector.step(quantum, capacity, lambda record: True)
        except RuntimeError:
            pass
        else:
            raise RuntimeError("invalid call bounds accepted")
    return {"status": "passed", "scope": "independent_mono_protocol_model",
            "fixtures": 4, "runs": runs, "calls": calls, "refusals": refusals,
            "proposed_ids": proposed, "accepted_ids": accepted,
            "largest_inside_block": largest_inside_block, "largest_shell": largest_shell,
            "invalid_call_bounds_rejected": 3, "rejected_model_mutants": mutants}


if __name__ == "__main__":
    print(json.dumps(selftest(), sort_keys=True))
