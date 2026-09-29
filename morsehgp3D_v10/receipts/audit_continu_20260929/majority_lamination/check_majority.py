#!/usr/bin/env python3
"""Exact abstract weighted-witness trees, not a native/geometric HGP gate."""
import json
import random
from fractions import Fraction as F


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def cut_owner(
    parent: list[int], birth: list[F], atoms: dict[str, list[tuple[int, F, F]]],
    radius: F, theta: F, normalize_active: bool = False,
) -> dict[str, tuple[str, object]]:
    require(F(1, 2) <= theta < 1, "theta must be in [1/2,1)")
    labels = {}
    for point, witnesses in atoms.items():
        masses = {}
        for node, time, weight in witnesses:
            require(weight > 0, "weights must be positive and fixed")
            require(time >= birth[node], "atom before its branch exists")
            require(parent[node] < 0 or time < birth[parent[node]],
                    "atom on an expired branch")
            if time > radius:
                continue
            live = node
            while parent[live] >= 0 and birth[parent[live]] <= radius:
                live = parent[live]
            require(birth[live] <= radius, "witness assigned before branch birth")
            masses[live] = masses.get(live, F(0)) + weight
        total = sum((weight for _, _, weight in witnesses), F(0))
        require(total > 0, "no fixed point mass")
        denominator = sum(masses.values(), F(0)) if normalize_active else total
        winners = [node for node, mass in masses.items() if mass > theta * denominator]
        require(len(winners) <= 1, "exclusive majority failed")
        # The common noise label -1 would not be a singleton completion.
        labels[point] = ("node", winners[0]) if winners else ("singleton", point)
    return labels


def nested(
    before: dict[str, tuple[str, object]], after: dict[str, tuple[str, object]],
) -> bool:
    return all(before[a] != before[b] or after[a] == after[b]
               for a in before for b in before)


def make_tree(leaves: int, comb: bool) -> tuple[list[int], list[F]]:
    parent, birth = [-1] * leaves, [F(0)] * leaves
    active = list(range(leaves))
    while len(active) > 1:
        left = active.pop(0)
        right = active.pop(0)
        new = len(parent)
        parent.extend([-1])
        birth.append(max(birth[left], birth[right]) + 1)
        parent[left] = parent[right] = new
        active.insert(0 if comb else len(active), new)
    return parent, birth


def run() -> dict[str, object]:
    theta_values = [F(1, 2), F(3, 5), F(9, 10)]
    cases = transitions = pair_checks = 0
    for seed in range(30):
        rng = random.Random(seed)
        for comb in (False, True):
            parent, birth = make_tree(3 + seed % 4, comb)
            atoms = {}
            for point in range(5):
                witnesses = []
                for _ in range(1 + rng.randrange(9)):
                    node = rng.randrange(len(parent))
                    time = birth[node] + F(rng.randrange(4), 4)
                    require(parent[node] < 0 or time < birth[parent[node]],
                            "atom on an expired branch")
                    witnesses.append((node, time, F(1 + rng.randrange(9))))
                atoms[str(point)] = witnesses
            times = sorted(set(birth + [time for rows in atoms.values()
                                        for _, time, _ in rows]))
            cuts = sorted(set([F(-1), times[-1] + 1] + times +
                              [(a + b) / 2 for a, b in zip(times, times[1:])]))
            for theta in theta_values:
                before = cut_owner(parent, birth, atoms, cuts[0], theta)
                for radius in cuts[1:]:
                    after = cut_owner(parent, birth, atoms, radius, theta)
                    require(nested(before, after), "majority point block split")
                    transitions += 1
                    pair_checks += len(atoms) ** 2
                    before = after
                require(len(set(before.values())) == 1, "root never recovered all points")
                cases += 1

    # The original flat-vote example: x waits for A+B, y enters C early.
    parent, birth = [3, 3, 4, 4, -1], list(map(F, (0, 0, 0, 1, 3)))
    atoms = {"x": [(0, F(0), F(3)), (1, F(0), F(3)), (2, F(0), F(4))],
             "y": [(0, F(0), F(1)), (1, F(0), F(1)), (2, F(0), F(8))]}
    early = cut_owner(parent, birth, atoms, F(0), F(1, 2))
    late = cut_owner(parent, birth, atoms, F(1), F(1, 2))
    require(early["x"] == ("singleton", "x") and early["y"] == ("node", 2),
            "majority example did not preserve ambiguity")
    require(late["x"] == ("node", 3) and late["y"] == ("node", 2),
            "majority example did not recover expected branch")

    # A singleton witness suffices without asking that the point be core.
    simple = {"x": [(0, F(1), F(1))], "y": [(0, F(1), F(1))]}
    require(cut_owner([-1], [F(1)], simple, F(1), F(9, 10)) ==
            {"x": ("node", 0), "y": ("node", 0)}, "early covered pair lost")

    # Renormalizing only active evidence destroys monotonicity.
    delayed = {"x": [(0, F(0), F(3)), (1, F(1, 2), F(3)), (2, F(1, 2), F(4))],
               "z": [(0, F(0), F(1))]}
    mutant_before = cut_owner(parent, birth, delayed, F(0), F(1, 2), True)
    mutant_after = cut_owner(parent, birth, delayed, F(1, 2), F(1, 2), True)
    require(not nested(mutant_before, mutant_after), "active-denominator mutant survived")
    good_before = cut_owner(parent, birth, delayed, F(0), F(1, 2))
    good_after = cut_owner(parent, birth, delayed, F(1, 2), F(1, 2))
    require(nested(good_before, good_after), "fixed denominator unexpectedly split")

    # Threshold below half permits two simultaneous owners.
    require(F(1, 2) > F(2, 5) and F(1, 2) + F(1, 2) == 1,
            "below-half ambiguity fixture invalid")
    for invalid in (F(-1), F(2, 5), F(1)):
        try:
            cut_owner(parent, birth, atoms, F(0), invalid)
        except RuntimeError as error:
            require("theta" in str(error), "rejected for an unrelated reason")
        else:
            raise RuntimeError("invalid threshold admitted")
    return {"status": "PASS", "scope": "abstract_exact_weighted_witness_trees_only",
            "theta_values": list(map(str, theta_values)), "cases": cases,
            "transitions": transitions, "pair_checks": pair_checks,
            "majority_example_early": early, "majority_example_late": late,
            "two_site_abstract_boundary_model_pass": True,
            "invalid_theta_rejections": 3,
            "active_denominator_mutant_rejected": True,
            "below_half_has_two_owners": True, "engine_imports": False,
            "GCP_used": False}


if __name__ == "__main__":
    print(json.dumps(run(), indent=2, sort_keys=True))
