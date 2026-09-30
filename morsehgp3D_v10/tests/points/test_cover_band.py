"""Small exact structural tests; not a geometry/quality/performance gate."""
from fractions import Fraction as F
import importlib.util
from pathlib import Path
import sys
import unittest

MODULE = Path(__file__).resolve().parents[2] / 'bench/frontier/cover_band.py'
spec = importlib.util.spec_from_file_location('cover_band', MODULE)
band = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = band
spec.loader.exec_module(band)


class Forest:
    def __init__(self, levels, parents):
        self.levels = tuple(map(F, levels))
        self.parents = tuple(parents)

    def __len__(self):
        return len(self.levels)

    def level(self, node):
        return self.levels[node]

    def ancestor(self, node, level, closed=True):
        if not closed:
            raise RuntimeError('test supports closed cuts only')
        if self.level(node) > level:
            return None
        while self.parents[node] is not None and self.level(self.parents[node]) <= level:
            node = self.parents[node]
        return node

    def lca(self, a, b):
        path = set()
        while a is not None:
            path.add(a)
            a = self.parents[a]
        while b not in path:
            b = self.parents[b]
            if b is None:
                return None
        return b


class Context:
    def __init__(self, forest, alpha, witnesses):
        self.forest, self.alpha, self.W = forest, alpha, witnesses
        self.n = len(alpha)

    def cover_level(self, s):
        return self.alpha[s]

    def witnesses(self):
        return self.W


def blocks(ctx, result, beta):
    out = {}
    for s, (t, node) in enumerate(zip(result.dates, result.nodes)):
        owner = ('singleton', s) if beta < t else ('component', ctx.forest.ancestor(node, beta))
        out.setdefault(owner, set()).add(s)
    return list(out.values())


class Tests(unittest.TestCase):
    def setUp(self):
        self.f = Forest([25, F(2501, 100), F(3249, 89)], [2, 2, None])

    def test_near_tie_not_just_exact_tie(self):
        ctx = Context(self.f, [25], [[(0, 25, 0), (1, F(2501, 100), 1)]])
        zero = band.cover_band_lca(ctx, F(0))
        near = band.cover_band_lca(ctx, F(1, 64))
        self.assertEqual(zero.dates, (F(25),))
        self.assertEqual(near.dates, (F(3249, 89),))
        self.assertEqual(near.nodes, (2,))
        self.assertEqual(near.per_site_selected, (2,))

    def test_internal_entry_and_root_prolongation(self):
        # No leaf witness: the actual first cover is an INTERNAL branch.
        f = Forest([1, 1, 4, 9], [2, 2, 3, None])
        ctx = Context(f, [5], [[(0, 5, 2), (1, 12, 3)]])
        result = band.cover_band_lca(ctx, F(1))
        self.assertEqual((result.dates, result.nodes), ((F(9),), (3,)))
        same = band.cover_band_lca(Context(f, [5], [[(0, 5, 2), (1, 8, 2)]]), F(1))
        self.assertEqual((same.dates, same.nodes), ((F(5),), (2,)))
        extended = band.cover_band_lca(Context(f, [12], [[(0, 12, 3)]]), F(1))
        self.assertEqual((extended.dates, extended.nodes), ((F(12),), (3,)))

    def test_equality_is_kept_and_order_does_not_matter(self):
        f = Forest([4, 9, 16], [2, 2, None])
        W = [(0, 4, 0), (1, 9, 1)]
        ctx = Context(f, [4], [W])
        a = band.cover_band_lca(ctx, F(1, 2))
        b = band.cover_band_lca(Context(f, [4], [W[::-1]]), F(1, 2))
        self.assertEqual(a, b)
        self.assertEqual(a.selected, 2)
        self.assertEqual(a.dates, (F(16),))

    def test_laminarity_monotonic_eta_and_complete_singletons(self):
        ctx = Context(self.f, [25, 25, F(2501, 100)],
                      [[(0, 25, 0), (1, F(2501, 100), 1)], [(0, 25, 0)], [(1, F(2501, 100), 1)]])
        results = [band.cover_band_lca(ctx, e) for e in (F(0), F(1, 64), F(1, 8), F(1))]
        cuts = sorted({F(0), F(25), F(26), F(100)} | set(self.f.levels) |
                      {date for result in results for date in result.dates})
        for result in results:
            previous = None
            for cut in cuts:
                current = blocks(ctx, result, cut)
                self.assertEqual(set.union(*current), {0, 1, 2})
                if previous is not None:
                    self.assertTrue(all(any(part <= target for target in current) for part in previous))
                previous = current
        for coarse, fine in zip(results, results[1:]):
            for cut in cuts:
                self.assertTrue(all(any(part <= target for target in blocks(ctx, coarse, cut))
                                    for part in blocks(ctx, fine, cut)))

    def test_reject_invalid_exact_domain(self):
        good = Context(self.f, [25], [[(0, 25, 0)]])
        for eta in (-1, 0.1, True, '1/8'):
            with self.assertRaises(band.BandError):
                band.cover_band_lca(good, eta)
        for ctx in (Context(self.f, [25], [[]]), Context(self.f, [25], [[(0, 24, 0)]]),
                    Context(self.f, [25], [[(0, 25.0, 0)]]), Context(self.f, [25], [[(0, 25, 2)]])):
            with self.assertRaises(band.BandError):
                band.cover_band_lca(ctx)


if __name__ == '__main__':
    unittest.main()
