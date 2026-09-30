"""Exact finite proof of empty hard-alpha bands; no native or Gamma import."""
import ast
from fractions import Fraction as F
import hashlib
from itertools import combinations
import json
from pathlib import Path

BASE = Path(__file__).resolve().parent
SNAPSHOT_SHA = '479e720879bc5d229f40ea6fab39202fdabbe684e46ad5d88311c8c7bb69af42'


def need(ok, msg):
    if not ok:
        raise RuntimeError(msg)


def sq(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b))


def actual_votes(mutant=False):
    raw = (BASE / 'functions_snapshot.py').read_bytes()
    need(hashlib.sha256(raw).hexdigest() == SNAPSHOT_SHA, 'snapshot source pin')
    src = raw.decode()
    if mutant:
        old = 'A = min(l2 for l2, _v, _y in brut)'
        need(src.count(old) == 1, 'scale mutation site')
        src = src.replace(old, 'A = gf.alpha2(x)')
    tree = ast.parse(src)
    need(len(tree.body) == 2 and {n.name for n in tree.body} == {'poids_bande', 'votes_paires'}, 'exact two-function AST')
    env = {'Fraction': F, 'exiger': need, '__builtins__': __builtins__}
    exec(compile(tree, 'functions_snapshot.py', 'exec'), env)
    return env['votes_paires']


class AnalyticContext:
    """MEB tables only; born IDs are dummy F tuples, not FULL component owners."""
    def __init__(self, points, K, geometry):
        self.P = [tuple(F(v) for v in p) for p in points]
        self.K, self.n = K, len(self.P)
        self.beta = {}
        for part in combinations(range(self.n), K):
            if K == 2:
                b = sq(self.P[part[0]], self.P[part[1]]) / 4
            elif geometry == 'line' and K == 3:
                b = (max(self.P[i][0] for i in part) - min(self.P[i][0] for i in part)) ** 2 / 4
            elif geometry == 'tetra_center' and K == 5 and self.n == 5:
                b = F(3)
            else:
                raise RuntimeError('unsupported analytic MEB')
            self.beta[part] = b
        self.born = {part: part for part in self.beta}

    def alpha2(self, x):
        return min(b for part, b in self.beta.items() if x in part)


def raw_pairs(ctx, x):
    out = []
    for y in range(ctx.n):
        if y == x:
            continue
        midpoint = tuple((a + b) / 2 for a, b in zip(ctx.P[x], ctx.P[y]))
        dist = sorted(sq(midpoint, p) for p in ctx.P)
        half2 = sq(ctx.P[x], ctx.P[y]) / 4
        dk2 = dist[ctx.K - 1]
        out.append({'y': y, 'half_distance2': half2, 'midpoint_dK2': dk2, 'ell2': max(half2, dk2)})
    return out


def hard_ids(rows, alpha2, factor2, closed=True):
    return [r['y'] for r in rows if r['ell2'] <= alpha2 * factor2] if closed else \
           [r['y'] for r in rows if r['ell2'] < alpha2 * factor2]


def describe(ctx, x, name, vote_fn):
    rows = raw_pairs(ctx, x)
    alpha2 = ctx.alpha2(x)
    A, actual = vote_fn(ctx, x, F(1))
    need(A == min(r['ell2'] for r in rows), 'actual A=min ell2')
    need(alpha2 <= A <= 4 * alpha2, 'general scale bound on fixture')
    for b, _dummy_owner, w, y in actual:
        need(b == rows[next(i for i, r in enumerate(rows) if r['y'] == y)]['ell2'] and w > 0,
             'actual vote level and positive soft weight')
    return {'name': name, 'K': ctx.K, 'x': x, 'points': [[str(v) for v in p] for p in ctx.P],
            'alpha2': str(alpha2), 'actual_A': str(A),
            'raw_pairs': [{k: str(v) if isinstance(v, F) else v for k, v in r.items()} for r in rows],
            'hard_eta_1_4_cutoff2': str(alpha2 * F(25, 16)),
            'hard_eta_1_4_y': hard_ids(rows, alpha2, F(25, 16)),
            'actual_soft_eta_prime_1': [[str(b), str(w), y] for b, _v, w, y in actual]}


def main():
    fn = actual_votes()
    line = AnalyticContext([(v, 0, 0) for v in (0, 2, 7, 10, 13)], 3, 'line')
    tetra_points = [(0, 0, 0), (1, 1, 1), (1, -1, -1), (-1, 1, -1), (-1, -1, 1)]
    tetra = AnalyticContext(tetra_points, 5, 'tetra_center')
    shifted = AnalyticContext([tuple(v + 1 for v in p) for p in tetra_points], 5, 'tetra_center')
    need(all(0 <= v < 2 ** 18 and v.denominator == 1 for p in shifted.P for v in p), 'translated u18')
    vertices = tetra.P[1:]
    need(all(sum(p[k] for p in vertices) == 0 for k in range(3)), 'tetra vertex mean0')
    need(all(sq(p, (0, 0, 0)) == 3 for p in vertices), 'tetra radius3')
    need(all(sum(a * b for a, b in zip(p, q)) == -1 for p, q in combinations(vertices, 2)), 'tetra mutual dot−1')
    # Cross-check the identity underlying the MEB lower bound at diverse rational centers.
    for c in [(F(0), F(0), F(0)), (F(1, 7), F(-3, 11), F(5, 13)), (F(9), F(-2), F(1))]:
        avg = sum(sq(p, c) for p in vertices) / 4
        need(avg == 3 + sq(c, (0, 0, 0)) and max(sq(p, c) for p in vertices) >= 3, 'mean MEB proof identity')
    cases = [describe(line, 3, 'FX-A9_x10_K3', fn), describe(tetra, 0, 'tetra_center_K5', fn),
             describe(shifted, 0, 'tetra_center_K5_u18_translation', fn)]
    need(cases[0]['alpha2'] == '9' and cases[0]['actual_A'] == '16', 'FX-A9 alpha/A')
    need([r['ell2'] for r in cases[0]['raw_pairs']] == ['25', '16', '81/4', '81/4'], 'FX-A9 four exact pair levels')
    need(cases[1]['alpha2'] == '3' and cases[1]['actual_A'] == '19/4' and
         all(r['ell2'] == '19/4' for r in cases[1]['raw_pairs']), 'K5 exact levels')
    need(all(c['hard_eta_1_4_y'] == [] for c in cases), 'empty proposed hard-alpha bands')
    need(raw_pairs(tetra, 0) == raw_pairs(shifted, 0), 'translation all exact pair levels invariant')
    k2contexts = [AnalyticContext(line.P, 2, 'line'), AnalyticContext(tetra.P, 2, 'tetra_center')]
    controls = [describe(k2contexts[0], 3, 'same_FXA9_K2', fn), describe(k2contexts[1], 0, 'same_tetra_K2', fn)]
    need(all(c['hard_eta_1_4_y'] for c in controls), 'K2 band nonempty')
    r3, r5 = raw_pairs(line, 3), raw_pairs(tetra, 0)
    threshold3 = hard_ids(r3, F(9), F(16, 9))
    half3 = hard_ids(r3, F(9), F(9, 4))
    threshold5 = hard_ids(r5, F(3), F(19, 12))
    above5 = hard_ids(r5, F(3), F(63, 50) ** 2)
    need(threshold3 == [1] and half3 == [1, 2, 4], 'K3 nonempty threshold controls')
    need(threshold5 == above5 == [1, 2, 3, 4], 'K5 threshold/above controls')
    need(hard_ids(r3, F(9), F(16, 9), closed=False) == [] and
         hard_ids(r5, F(3), F(19, 12), closed=False) == [], 'open-shell mutation rejected')
    wrong_fn = actual_votes(True)
    wrong3, _ = wrong_fn(line, 3, F(1))
    wrong5, _ = wrong_fn(tetra, 0, F(1))
    need(wrong3 == 9 != F(cases[0]['actual_A']) and wrong5 == 3 != F(cases[1]['actual_A']),
         'actual A identified with alpha mutation rejected')
    bounds = []
    for ctx in [line, tetra] + k2contexts:
        for x in range(ctx.n):
            A, _ = fn(ctx, x, F(1))
            alpha2 = ctx.alpha2(x)
            need(alpha2 <= A <= 4 * alpha2, 'all points scale bound')
            bounds.append({'K': ctx.K, 'x': x, 'alpha2': str(alpha2), 'A': str(A)})
    print(json.dumps({'scope': 'proposed hard-alpha totality only; actual MMp scale via analytic stub; not native/MMt/G4',
                      'snapshot_sha256': SNAPSHOT_SHA, 'cases': cases, 'K2_controls': controls,
                      'threshold_controls': {'K3_eta_1_3': threshold3, 'K3_eta_1_2': half3,
                                             'K5_factor_squared_19_12': threshold5, 'K5_eta_13_50': above5},
                      'scale_bounds_all_points': bounds,
                      'mutants_killed': {'open_shell_K3_K5': True, 'A_equals_alpha2': [str(wrong3), str(wrong5)]}},
                     sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
