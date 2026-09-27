"""Oracles bornés des outils de démo (unittest, tient sous ``python3 -O``).

Petites tailles seulement : ces tests établissent la correction des
parcours (MST, meilleur nœud, suivi des branches, arbre ALPINE en vue de
dessus) contre une énumération brute ; ils ne mesurent aucune échelle.

    python3 -m unittest discover -s Zoltan/demos/tools -p 'test_*.py'
"""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from hierarchy import (DSU, best_and_first_nodes, best_nodes, brute_mst, core_distances, mreach_mst,  # noqa: E402
                       replay_tracks, weight_groups)


def cloud(seed, n=180):
    rng = np.random.default_rng(seed)
    a = rng.normal(0, 0.35, (n // 3, 3))
    b = rng.normal(0, 0.15, (n // 3, 3)) + [1.2, 0, 0]
    c = rng.uniform(-1, 2, (n - 2 * (n // 3), 3)) * [1, 0.3, 0.2]
    return np.concatenate([a, b, c])


def grid_cloud(seed):
    """Nuage à distances entières : nombreux ex æquo dans le MST."""
    rng = np.random.default_rng(seed)
    P = np.array([(x, y, 0.0) for x in range(9) for y in range(7)], float)
    keep = rng.random(len(P)) < 0.8
    return P[keep]


def all_nodes(births, mst, n):
    """Énumération brute : les composantes présentes à chaque niveau distinct.

    Un nœud n'existe qu'après toutes les arêtes d'un même poids ; on énumère
    donc les composantes (points nés) à chaque niveau où quelque chose change."""
    levels = np.unique(np.concatenate([births, mst[:, 2]]))
    nodes = []
    for r in levels:
        dsu = DSU(n)
        for a, b, w in mst:
            if w <= r:
                ra, rb = dsu.find(int(a)), dsu.find(int(b))
                if ra != rb:
                    dsu.p[rb] = ra
        comps = {}
        for i in range(n):
            if births[i] <= r:
                comps.setdefault(dsu.find(i), set()).add(i)
        nodes += [(float(r), c) for c in comps.values()]
    return nodes


class MutualReachabilityMST(unittest.TestCase):
    def test_weights_match_dense_prim(self):
        for seed in range(4):
            X = cloud(seed)
            for K in (1, 2, 5, 10):
                core, mst = mreach_mst(X, K)
                _, ref = brute_mst(X, K)
                self.assertEqual(len(mst), len(X) - 1)
                self.assertTrue(np.allclose(np.sort(mst[:, 2]), ref, rtol=0, atol=1e-12), (seed, K))
                lo = np.maximum(core[mst[:, 0].astype(int)], core[mst[:, 1].astype(int)])
                self.assertTrue(np.all(mst[:, 2] >= lo - 1e-12))

    def test_core_distance_counts_the_point(self):
        X = np.array([[0.0, 0, 0], [1, 0, 0], [3, 0, 0]])
        self.assertTrue(np.allclose(core_distances(X, 1), 0))
        self.assertTrue(np.allclose(core_distances(X, 2), [1, 1, 2]))


class BestNodes(unittest.TestCase):
    def check(self, X, labels, Ks, ignore=None):
        n = len(X)
        skip = set() if ignore is None else set(np.flatnonzero(ignore).tolist())
        for K in Ks:
            births, mst = mreach_mst(X, K)
            nodes = all_nodes(births, mst, n)
            best, first = best_and_first_nodes(births, mst, labels, [0, 1, 2], ignore)
            for o in (0, 1, 2):
                obj = set(np.flatnonzero(labels == o).tolist())
                ious = [(len(obj & c) / len((c - skip) | obj), r) for r, c in nodes]
                self.assertAlmostEqual(best[o][0], max(v for v, _ in ious), places=12)
                matched = [r for v, r in ious if v > 0.5]
                if matched:
                    self.assertIsNotNone(first[o])
                    self.assertAlmostEqual(first[o][0], min(matched), places=12)
                else:
                    self.assertIsNone(first[o])

    def test_against_enumeration(self):
        for seed in range(3):
            X = cloud(seed, 150)
            rng = np.random.default_rng(100 + seed)
            labels = np.full(len(X), -1)
            labels[:50] = 0
            labels[50:100] = 1
            labels[rng.choice(len(X), 25, replace=False)] = 2
            self.check(X, labels, (1, 5))

    def test_void_points_are_excluded_from_iou(self):
        for seed in range(3):
            X = cloud(40 + seed, 150)
            rng = np.random.default_rng(200 + seed)
            labels = np.full(len(X), -1)
            labels[:40], labels[50:90], labels[100:115] = 0, 1, 2
            ignore = (labels < 0) & (rng.random(len(X)) < 0.5)
            self.check(X, labels, (1, 5), ignore)

    def test_ties_are_grouped(self):
        for seed in range(4):
            X = grid_cloud(seed)
            self.assertLess(len(weight_groups(mreach_mst(X, 1)[1])), len(X) - 1)  # ex æquo présents
            labels = np.full(len(X), -1)
            labels[X[:, 0] <= 2] = 0
            labels[(X[:, 0] >= 4) & (X[:, 1] <= 3)] = 1
            labels[(X[:, 0] >= 6) & (X[:, 1] >= 4)] = 2
            self.check(X, labels, (1, 2, 4))


class ReplayTracks(unittest.TestCase):
    def test_join_levels_are_component_membership(self):
        X = cloud(7, 160)
        births, mst = mreach_mst(X, 5)
        obj = np.full(len(X), -1)
        obj[:40], obj[60:90], obj[120:140] = 0, 1, 2
        seeds = [3, 70, 125]
        join, events = replay_tracks(births, mst, obj, seeds, X, r_cap=np.inf)
        levels = np.unique(np.concatenate([mst[:, 2], births]))
        for r in levels[:: max(1, len(levels) // 40)]:
            dsu = DSU(len(X))
            for a, b, w in mst:
                if w <= r:
                    ra, rb = dsu.find(int(a)), dsu.find(int(b))
                    if ra != rb:
                        dsu.p[rb] = ra
            for o, s in enumerate(seeds):
                if births[s] > r:
                    continue
                comp = {i for i in range(len(X)) if dsu.find(i) == dsu.find(s)}
                self.assertEqual(comp, set(np.flatnonzero(join[o] <= r).tolist()))
                ev = [e for e in events[o] if e[0] <= r][-1]
                idx = np.array(sorted(comp))
                self.assertTrue(np.allclose(ev[1:4], X[idx].min(0)))
                self.assertTrue(np.allclose(ev[4:7], X[idx].max(0)))
                c = int((obj[idx] == o).sum())
                self.assertAlmostEqual(ev[7], c / (obj == o).sum())
                self.assertAlmostEqual(ev[8], c / len(idx))


class FirstMatchSeed(unittest.TestCase):
    def test_seed_branch_carries_every_matched_node(self):
        for seed in range(3):
            X = cloud(20 + seed, 160)
            labels = np.full(len(X), -1)
            labels[:45], labels[60:100], labels[120:135] = 0, 1, 2
            for K in (1, 5):
                births, mst = mreach_mst(X, K)
                best, first = best_and_first_nodes(births, mst, labels, [0, 1, 2])
                nodes = all_nodes(births, mst, len(X))
                seeds = []
                for o in range(3):
                    idx = np.flatnonzero(labels == o)
                    if first[o] is None:
                        seeds.append(int(idx[0]))
                        continue
                    dsu = DSU(len(X))
                    for a, b, w in mst[: first[o][1] + 1]:
                        ra, rb = dsu.find(int(a)), dsu.find(int(b))
                        if ra != rb:
                            dsu.p[rb] = ra
                    root = dsu.find(first[o][2])
                    seeds.append(int(next(i for i in idx if dsu.find(int(i)) == root)))
                join, events = replay_tracks(births, mst, labels, seeds, X, r_cap=np.inf)
                for o in range(3):
                    obj = set(np.flatnonzero(labels == o).tolist())
                    for r, c in nodes:
                        if len(obj & c) / len(obj | c) > 0.5:
                            self.assertIn(seeds[o], c)
                            ev = [e for e in events[o] if e[0] <= r][-1]
                            self.assertEqual(ev[11], 2)


class AlpineBev(unittest.TestCase):
    def test_forest_is_single_linkage_of_symmetric_knn_graph(self):
        from build_scene import alpine_bev_forest
        from scipy.sparse.csgraph import connected_components
        from scipy.spatial import cKDTree
        X = cloud(3, 200)
        k = 8
        _, forest = alpine_bev_forest(X, k=k)
        P = X[:, :2]
        _, nn = cKDTree(P).query(P, k=k + 1)
        for t in (0.05, 0.1, 0.2, 0.4):
            A = np.zeros((len(P), len(P)), bool)
            for i in range(len(P)):
                for j in nn[i]:
                    if j != i and np.linalg.norm(P[i] - P[j]) < t:
                        A[i, j] = A[j, i] = True
            _, ref = connected_components(A, directed=False)
            dsu = DSU(len(P))
            for a, b, w in forest:
                if w < t:
                    ra, rb = dsu.find(int(a)), dsu.find(int(b))
                    if ra != rb:
                        dsu.p[rb] = ra
            got = np.array([dsu.find(i) for i in range(len(P))])
            pairs = np.random.default_rng(0).integers(0, len(P), (2000, 2))
            same_ref = ref[pairs[:, 0]] == ref[pairs[:, 1]]
            same_got = got[pairs[:, 0]] == got[pairs[:, 1]]
            self.assertTrue(np.array_equal(same_ref, same_got), t)


class GroundMaskPin(unittest.TestCase):
    def test_v8_mask_reproduced_when_frame_is_cached(self):
        import hashlib
        cache = Path(__file__).resolve().parents[1] / '_cache' / 'velodyne' / '08_000000.bin'
        if not cache.is_file():
            self.skipTest('trame 08/000000 absente du cache local')
        from ground import V8_MASK_SHA256_08_000000, ground_mask
        xyzi = np.fromfile(cache, np.float32).reshape(-1, 4)
        self.assertEqual(hashlib.sha256(ground_mask(xyzi).tobytes()).hexdigest(), V8_MASK_SHA256_08_000000)


if __name__ == '__main__':
    unittest.main()
