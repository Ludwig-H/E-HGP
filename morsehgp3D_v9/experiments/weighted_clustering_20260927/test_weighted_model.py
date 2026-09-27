"""Exact arithmetic and independent set-partition tests of the facet measure."""
from fractions import Fraction as Q
from itertools import combinations
import math
import random
import unittest

from weighted_model import build_facet_model, cluster_cofaces, vote_points


def coface(vertices, beta=1):
    beta = Q(beta)
    return dict(vertices=list(vertices), beta=dict(num=str(beta.numerator), den=str(beta.denominator)))


def tree_cut(model, beta):
    groups = {j: frozenset((j,)) for j in range(len(model["facets"]))}
    for node, children in model["children"].items():
        if model["squared_levels"][node] <= beta:
            groups[node] = frozenset().union(*(groups.pop(c) for c in children))
    return set(groups.values())


def reference_cut(facets, rows, cut):
    # No union-find or production boundary IDs: repeatedly close intersecting
    # sets of facets for every admitted coface, in reversed input order.
    groups = {frozenset((f,)) for f in facets}
    for row in reversed(rows):
        if Q(int(row["beta"]["num"]), int(row["beta"]["den"])) > cut:
            continue
        v = set(row["vertices"])
        together = frozenset(f for f in facets if set(f) < v)
        touched = {g for g in groups if together & g}
        groups.difference_update(touched)
        groups.add(frozenset().union(together, *touched))
    return groups


class WeightedModelTests(unittest.TestCase):
    def test_exact_partition_of_unity_and_face_bound(self):
        rng = random.Random(7183)
        for n in range(3, 9):
            for k in range(1, min(n, 5)):
                rows = [coface(v, rng.randrange(1, 40)) for v in combinations(range(n), k + 1)]
                model = build_facet_model(n, k, rows, exp_z=2, rational_z2=True)
                self.assertEqual(sum(model["masses"]), n)
                self.assertTrue(all(0 < m <= 1 for m in model["masses"]))
                for x in range(n):
                    direct = k * sum((1 / Q(int(r["beta"]["num"]), int(r["beta"]["den"]))
                                      for r in rows if x in r["vertices"]), Q(0))
                    self.assertEqual(model["point_totals"][x], direct)
                    self.assertEqual(sum((s / direct for f, s in zip(model["facets"], model["scores"]) if x in f), Q(0)), 1)
                for facet, score in zip(model["facets"], model["scores"]):
                    expected = sum((1 / Q(int(r["beta"]["num"]), int(r["beta"]["den"]))
                                    for r in rows if set(facet) < set(r["vertices"])), Q(0))
                    self.assertEqual(score, expected)

    def test_atomic_tree_against_set_closure(self):
        rng = random.Random(3701)
        for n in (4, 6, 8):
            for k in (1, 2, 3):
                rows = [coface(v, rng.randrange(1, 6)) for v in combinations(range(n), k + 1)]
                model = build_facet_model(n, k, rows)
                facets = model["facets"]
                for cut in (Q(0), Q(1), Q(3, 2), Q(2), Q(3), Q(4), Q(5)):
                    got = {frozenset(facets[i] for i in group) for group in tree_cut(model, cut)}
                    self.assertEqual(got, reference_cut(facets, rows, cut))
                self.assertEqual(len(model["roots"]), 1)
                for p, children in model["children"].items():
                    self.assertGreaterEqual(len(children), 2)
                    for c in children:
                        if c in model["children"]:
                            self.assertLess(model["squared_levels"][c], model["squared_levels"][p])

    def test_redundant_coface_changes_weights_not_topology(self):
        rows = [coface(v, 3) for v in combinations(range(4), 3)]
        full = build_facet_model(4, 2, rows, exp_z=2, rational_z2=True)
        thin = build_facet_model(4, 2, rows[1:], exp_z=2, rational_z2=True)
        self.assertEqual(full["facets"], thin["facets"])
        self.assertEqual(full["children"], thin["children"])
        self.assertNotEqual(full["scores"], thin["scores"])
        self.assertNotEqual(full["masses"], thin["masses"])

    def test_exponent_changes_mass_and_vote(self):
        rows = [coface((0, 1, 2), 1), coface((0, 1, 3), 4), coface((0, 2, 3), 9)]
        a = build_facet_model(4, 2, rows, exp_z=1)
        b = build_facet_model(4, 2, rows, exp_z=2)
        self.assertEqual(a["children"], b["children"])
        self.assertNotEqual(a["masses"], b["masses"])
        labels = [i % 2 for i in range(len(a["facets"]))]
        self.assertNotEqual(vote_points(a, labels)["memberships"], vote_points(b, labels)["memberships"])

    def test_rational_float_agreement(self):
        rows = [coface(v, i + 1) for i, v in enumerate(combinations(range(7), 4))]
        exact = build_facet_model(7, 3, rows, exp_z=2, rational_z2=True)
        approximate = build_facet_model(7, 3, rows, exp_z=2)
        for a, b in zip(exact["masses"], approximate["masses"]):
            self.assertTrue(math.isclose(float(a), b, rel_tol=2e-15))

    def test_noise_does_not_compete_or_renormalize(self):
        model = build_facet_model(4, 2, [coface((0, 1, 2))], exp_z=2, rational_z2=True)
        vote = vote_points(model, [4, -1, -1])
        self.assertEqual(vote["labels"], [4, 4, -1, -1])
        self.assertEqual(vote["memberships"][0], {4: Q(1, 2)})
        self.assertEqual(vote["memberships"][3], {})

    def test_ties_and_input_order(self):
        rows = [coface(v, 2) for v in combinations(range(4), 3)]
        a = build_facet_model(4, 2, rows, exp_z=2, rational_z2=True)
        b = build_facet_model(4, 2, [coface(reversed(r["vertices"]), 2) for r in reversed(rows)], exp_z=2, rational_z2=True)
        self.assertEqual(a, b)
        model = build_facet_model(3, 2, [coface((0, 1, 2))], exp_z=2, rational_z2=True)
        vote = vote_points(model, [8, 3, 4])
        self.assertEqual(vote["labels"], [3, 4, 3])
        self.assertEqual(vote["margins"], [0, 0, 0])

    def test_common_division_must_not_invent_vote_tie(self):
        # Comparator-only numeric fixture, not a claimed geometric catalogue.
        model = dict(n_points=1, facets=[(0,), (0,)], scores=[1.1, 1.1000000000000003],
                     point_totals=[17.27], arithmetic="binary64_reference")
        self.assertEqual(model["scores"][0] / 17.27, model["scores"][1] / 17.27)
        vote = vote_points(model, [0, 1])
        self.assertEqual(vote["labels"], [1])
        self.assertGreater(vote["raw_margins"][0], 0)
        self.assertGreater(vote["margins"][0], 0)

    def test_rejections(self):
        good = coface((0, 1, 2))
        bad_rows = [[good, good], [coface((0, 1))], [coface((0, 1, 1))],
                    [coface((0, 1, 9))], [dict(vertices=[0, 1, 2], beta=dict(num='1', den='0'))],
                    [dict(vertices=[0, True, 2], beta=good['beta'])],
                    [dict(vertices=[0, 1, 2], beta=dict(num='-1', den='1'))]]
        for rows in bad_rows:
            with self.assertRaises(ValueError):
                build_facet_model(3, 2, rows)
        with self.assertRaises(ValueError):
            build_facet_model(3, 2, [good], exp_z=1, rational_z2=True)
        with self.assertRaises(ValueError):
            cluster_cofaces(3, 2, [good], min_cluster_size=1)
        model = build_facet_model(3, 2, [good])
        for labels in ([], [0, 1, True], [0, 1, -2]):
            with self.assertRaises(ValueError):
                vote_points(model, labels)

    def test_empty_and_disconnected_scope(self):
        self.assertEqual(cluster_cofaces(3, 3, [], min_cluster_size=2)["vote"]["labels"], [-1] * 3)
        with self.assertRaisesRegex(ValueError, "disconnected"):
            cluster_cofaces(6, 2, [coface((0, 1, 2)), coface((3, 4, 5))])


if __name__ == "__main__":
    unittest.main()
