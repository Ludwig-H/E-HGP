from __future__ import annotations

import json
import unittest
from collections import defaultdict
from pathlib import Path


FIXTURE = (
    Path(__file__).parents[1]
    / "fixtures"
    / "regressions"
    / "morse_m1_o5_death_accounting.json"
)


class _DisjointSet:
    def __init__(self, values: set[str]) -> None:
        self._parent = {value: value for value in values}

    def find(self, value: str) -> str:
        parent = self._parent[value]
        if parent != value:
            self._parent[value] = self.find(parent)
        return self._parent[value]

    def union(self, left: str, right: str) -> None:
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root == right_root:
            return
        low, high = sorted((left_root, right_root))
        self._parent[high] = low


def _audit(case: dict[str, object]) -> dict[str, object]:
    carrier_roots = case["carrier_roots"]
    events = case["events"]
    if not isinstance(carrier_roots, dict) or not isinstance(events, list):
        raise ValueError("invalid O5 fixture shape")

    carriers = set(carrier_roots)
    dsu = _DisjointSet(carriers)
    by_root: dict[str, list[str]] = defaultdict(list)
    for carrier, root in carrier_roots.items():
        if root is not None:
            if not isinstance(root, str):
                raise ValueError("a prior root must be a string or null")
            by_root[root].append(carrier)

    # A prior reduced root is one R vertex even when several carriers expose
    # it.  This equivalence must precede the saddle hyperedges.
    for root_carriers in by_root.values():
        for carrier in root_carriers[1:]:
            dsu.union(root_carriers[0], carrier)

    touched_carriers: set[str] = set()
    local_capacity = 0
    for event in events:
        if not isinstance(event, list) or len(event) < 2:
            raise ValueError("an index-one event needs at least two arms")
        if any(not isinstance(carrier, str) or carrier not in carriers for carrier in event):
            raise ValueError("an event references an unknown carrier")
        touched_carriers.update(event)
        local_capacity += len(event) - 1
        for carrier in event[1:]:
            dsu.union(event[0], carrier)

    component_roots: dict[str, set[str]] = defaultdict(set)
    for carrier in touched_carriers:
        component = dsu.find(carrier)
        component_roots[component]
        root = carrier_roots[carrier]
        if root is not None:
            component_roots[component].add(root)

    group_q = sorted(len(roots) for roots in component_roots.values())
    touched_roots = set().union(*component_roots.values()) if component_roots else set()
    positive_group_count = sum(q > 0 for q in group_q)
    group_death_count = sum(max(q - 1, 0) for q in group_q)
    root_identity_death_count = len(touched_roots) - positive_group_count
    if group_death_count != root_identity_death_count:
        raise AssertionError("the two canonical H0-death identities disagree")
    if group_death_count > local_capacity:
        raise AssertionError("global H0 deaths exceed local Morse multiplicity")

    return {
        "group_q": group_q,
        "touched_root_count": len(touched_roots),
        "positive_group_count": positive_group_count,
        "death_count": group_death_count,
        "local_multiplicity_capacity": local_capacity,
    }


class MorseM1O5DeathAccountingRegressionTests(unittest.TestCase):
    def setUp(self) -> None:
        with FIXTURE.open(encoding="utf-8") as fixture_file:
            self.fixture = json.load(fixture_file)

    def test_all_regressions_close_both_death_identities_and_local_bound(self) -> None:
        self.assertEqual(self.fixture["schema_version"], 1)
        self.assertEqual(
            self.fixture["kind"],
            "morse-m1-o5-death-accounting-regression-v1",
        )
        for case in self.fixture["cases"]:
            with self.subTest(fixture_id=case["fixture_id"]):
                self.assertEqual(_audit(case), case["expected"])

    def test_simultaneous_overlap_is_not_an_eventwise_death_sum(self) -> None:
        case = next(
            case
            for case in self.fixture["cases"]
            if case["fixture_id"] == "CE-M1-01-simultaneous-overlap"
        )
        eventwise_naive = sum(
            len({case["carrier_roots"][carrier] for carrier in event}) - 1
            for event in case["events"]
        )
        audited = _audit(case)
        self.assertGreater(eventwise_naive, audited["death_count"])
        self.assertEqual(audited["death_count"], 3)

    def test_latent_hyperedges_and_same_root_preunion_are_obligatory(self) -> None:
        selected = {
            case["fixture_id"]: _audit(case)
            for case in self.fixture["cases"]
            if case["fixture_id"]
            in {"CE-M1-O5-latent-chain", "CE-M1-O5-same-root-preunion"}
        }
        self.assertEqual(selected["CE-M1-O5-latent-chain"]["group_q"], [2])
        self.assertEqual(
            selected["CE-M1-O5-same-root-preunion"]["positive_group_count"],
            1,
        )

    def test_event_and_arm_permutations_preserve_batch_accounting(self) -> None:
        for case in self.fixture["cases"]:
            permuted = {
                **case,
                "events": [list(reversed(event)) for event in reversed(case["events"])],
            }
            with self.subTest(fixture_id=case["fixture_id"]):
                self.assertEqual(_audit(permuted), case["expected"])


if __name__ == "__main__":
    unittest.main()
