"""Small autonomous exact proof; shared sources are never imported or changed."""
import ast
from collections import defaultdict
from fractions import Fraction as F
import hashlib
from itertools import combinations
import json
from math import isqrt
from pathlib import Path
from types import SimpleNamespace


BASE = Path(__file__).resolve().parent
SNAPSHOT_SHA = 'a1ff44de92bc97c7b8b4a92cb2c32572817eef84b77c6f9b58dc42f74d4cb3a0'


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def actual_majority(mutant=False):
    raw = (BASE / 'paires_snapshot.py').read_bytes()
    require(hashlib.sha256(raw).hexdigest() == SNAPSHOT_SHA, 'snapshot pin')
    source = raw.decode()
    if mutant:
        require(source.count('2 * m > W') == 4, 'mutation sites changed')
        source = source.replace('2 * m > W', '2 * m >= W')
    tree = ast.parse(source)
    selected = [n for n in tree.body if isinstance(n, (ast.FunctionDef, ast.ClassDef))
                and n.name in ('ErreurPaires', '_exiger', 'majorite_virtuelle')]
    require(len(selected) == 3, 'AST functions')
    module = ast.Module(body=selected, type_ignores=[])
    env = {'__builtins__': __builtins__}
    exec(compile(module, 'paires_snapshot.py::selected_AST', 'exec'), env)
    return env['majorite_virtuelle']


class Gamma2:
    """All pair vertices, triple cofaces, simultaneous closed level processing."""
    def __init__(self, points):
        self.points = points
        self.pairs = list(combinations(range(len(points)), 2))
        self.pair_id = {p: i for i, p in enumerate(self.pairs)}
        self.birth = [(points[a] - points[b]) ** 2 / 4 for a, b in self.pairs]
        self.levels = list(self.birth)
        self.parent = list(range(len(self.pairs)))
        self.children = [[] for _ in self.pairs]
        uf = list(range(len(self.pairs)))
        head = list(range(len(self.pairs)))

        def find(v):
            while uf[v] != v:
                v = uf[v]
            return v

        events = defaultdict(list)
        for triple in combinations(range(len(points)), 3):
            span = max(points[i] for i in triple) - min(points[i] for i in triple)
            beta = span ** 2 / 4
            events[beta].append([self.pair_id[p] for p in combinations(triple, 2)])
        for beta in sorted(set(self.birth) | set(events)):
            for face in events[beta]:
                roots = [find(v) for v in face]
                for r in roots[1:]:
                    uf[r] = roots[0]
            groups = defaultdict(list)
            for v, b in enumerate(self.birth):
                if b <= beta:
                    groups[find(v)].append(v)
            for vs in groups.values():
                hs = sorted({head[v] for v in vs})
                if len(hs) > 1:
                    node = len(self.parent)
                    self.parent.append(node)
                    self.children.append(hs)
                    self.levels.append(beta)
                    for h in hs:
                        require(self.parent[h] == h, 'reparenting')
                        self.parent[h] = node
                    for v in vs:
                        head[v] = node
        roots = [v for v, p in enumerate(self.parent) if v == p]
        require(len(roots) == 1, 'Gamma2 one eventual root')
        self.root = roots[0]
        self.depth = [0] * len(self.parent)
        self.tin = [0] * len(self.parent)
        tick = 0

        def dfs(v, depth):
            nonlocal tick
            self.depth[v] = depth
            self.tin[v] = tick
            tick += 1
            for c in self.children[v]:
                dfs(c, depth + 1)
        dfs(self.root, 0)

    def level(self, v):
        return self.levels[v]

    def is_ancestor(self, a, v):
        while True:
            if v == a:
                return True
            if self.parent[v] == v:
                return False
            v = self.parent[v]

    def lca(self, a, b):
        while self.depth[a] > self.depth[b]:
            a = self.parent[a]
        while self.depth[b] > self.depth[a]:
            b = self.parent[b]
        while a != b:
            a, b = self.parent[a], self.parent[b]
        return a

    def ancestor(self, v, beta):
        require(self.level(v) <= beta, 'ancestor called before birth')
        while self.parent[v] != v and self.level(self.parent[v]) <= beta:
            v = self.parent[v]
        return v

    def components(self, beta):
        groups = defaultdict(set)
        for v, b in enumerate(self.birth):
            if b <= beta:
                groups[self.ancestor(v, beta)].add(v)
        return frozenset(frozenset(vs) for vs in groups.values())


def radius(beta):
    a, b = isqrt(beta.numerator), isqrt(beta.denominator)
    require(a * a == beta.numerator and b * b == beta.denominator, 'rational radius')
    return F(a, b)


def intervals(points, r):
    """L2(r) directly as union of intersections of all closed pair intervals."""
    pieces = []
    for a, b in combinations(points, 2):
        lo, hi = max(a, b) - r, min(a, b) + r
        if lo <= hi:
            pieces.append((lo, hi))
    out = []
    for lo, hi in sorted(pieces):
        if out and lo <= out[-1][1]:
            out[-1] = (out[-1][0], max(out[-1][1], hi))
        else:
            out.append((lo, hi))
    return out


def direct_components(g, beta):
    iv = intervals(g.points, radius(beta))
    groups = defaultdict(set)
    for v, (a, b) in enumerate(g.pairs):
        if g.birth[v] <= beta:
            mid = (g.points[a] + g.points[b]) / 2
            owners = [j for j, (lo, hi) in enumerate(iv) if lo <= mid <= hi]
            require(len(owners) == 1, 'pair center direct owner')
            groups[owners[0]].add(v)
    return frozenset(frozenset(vs) for vs in groups.values())


def rows_for(g, x, eta, closed=True):
    alpha2 = min((g.points[x] - p) ** 2 / 4 for y, p in enumerate(g.points) if y != x)
    rows = []
    for y in range(len(g.points)):
        if y == x:
            continue
        v = g.pair_id[tuple(sorted((x, y)))]
        beta = g.birth[v]
        cutoff = (1 + eta) ** 2 * alpha2
        if beta <= cutoff if closed else beta < cutoff:
            rows.append((beta, g.ancestor(v, beta), y))
    require(rows, 'nonempty band')
    return alpha2, rows


def direct_majority(g, rows):
    for beta in sorted(set(g.levels) | {b for b, _v, _y in rows}):
        masses = defaultdict(int)
        for b, v, _y in rows:
            if b <= beta:
                masses[g.ancestor(v, beta)] += 1
        winners = [v for v, m in masses.items() if 2 * m > len(rows)]
        require(len(winners) <= 1, 'strict majority unique')
        if winners:
            return beta, winners[0], len(rows)
    raise RuntimeError('no direct majority')


def hierarchy_partition(g, dates, owners, beta):
    groups = defaultdict(set)
    for x, date in enumerate(dates):
        key = ('point', x) if date > beta else ('node', g.ancestor(owners[x], beta))
        groups[key].add(x)
    return frozenset(frozenset(vs) for vs in groups.values())


def fixture(e, eta, majority, closed=True, expected=True, scale=1, translation=0):
    pts = [scale * p + translation for p in [F(0), F(100), F(110), F(-110), F(-125) + e]]
    g = Gamma2(pts)
    ctx = SimpleNamespace(forest=g, tin=g.tin)
    event_r = sorted({radius(b) for b in g.levels})
    cuts_r = sorted(set(event_r + [(a + b) / 2 for a, b in zip(event_r, event_r[1:])]))
    comparisons = 0
    for r in cuts_r:
        require(g.components(r * r) == direct_components(g, r * r), 'Gamma2 versus direct L2')
        comparisons += 1
    dates, owners, oracle_dates, oracle_owners = [], [], [], []
    allrows = []
    for x in range(5):
        alpha2, rows = rows_for(g, x, eta, closed)
        ans = majority(ctx, rows)
        oracle = direct_majority(g, rows)
        if expected:
            require(ans == oracle, 'actual virtual versus exhaustive majority')
            require(g.ancestor(ans[1], ans[0]) == ans[1], 'actual owner alive')
        dates.append(ans[0]); owners.append(ans[1]); allrows.append(rows)
        oracle_dates.append(oracle[0]); oracle_owners.append(oracle[1])
        comparisons += 1
        if x == 0:
            require(alpha2 == 2500 * scale ** 2, 'alpha(x)')
    heights = {}
    for x, y in combinations(range(5), 2):
        actual = max(dates[x], dates[y], g.level(g.lca(owners[x], owners[y])))
        oracle = max(oracle_dates[x], oracle_dates[y], g.level(g.lca(oracle_owners[x], oracle_owners[y])))
        if expected:
            require(actual == oracle, 'all pair meeting heights')
        heights['%d,%d' % (x, y)] = str(radius(actual))
        comparisons += 1
    for r in cuts_r:
        if expected:
            require(hierarchy_partition(g, dates, owners, r * r) ==
                    hierarchy_partition(g, oracle_dates, oracle_owners, r * r), 'all cut partitions')
        comparisons += 1
    expected_W = 3 if eta == F(1, 8) or e < 0 else 4
    expected_date = (55 if expected_W == 3 else 105) * scale
    if expected:
        require(len(allrows[0]) == expected_W, 'W analytical')
        require(dates[0] == expected_date ** 2, 'date analytical')
        require(heights['0,1'] == str(expected_date), 'x joins C1 analytical')
        require(heights['1,2'] == str(5 * scale), 'C1/C2 local pair stable')
        require(heights['1,3'] == str(105 * scale), 'C/D meeting remains105')
        require(g.ancestor(g.pair_id[(0, 1)], F(105 * scale) ** 2) ==
                g.ancestor(g.pair_id[(0, 3)], F(105 * scale) ** 2), 'C/D joined105')
        require(g.ancestor(g.pair_id[(0, 1)], F(104 * scale) ** 2) !=
                g.ancestor(g.pair_id[(0, 3)], F(104 * scale) ** 2), 'C/D not joined104')
        require(len(intervals(pts, F(70 * scale))) == 2, 'two FULL2 components at70')
    return {'e': str(e), 'eta': str(eta), 'W': len(allrows[0]),
            'scale': scale, 'translation': translation, 'points': [str(p) for p in pts],
            'alpha_x': str(50 * scale), 'date_x': str(radius(dates[0])),
            'votes_x': [[str(radius(b)), y] for b, _v, y in allrows[0]],
            'meeting_radii': heights,
            'point_partition_r70scaled': sorted([sorted(c) for c in hierarchy_partition(g, dates, owners, F(70 * scale) ** 2)]),
            'checks': comparisons}


def main():
    majority = actual_majority()
    eps = [F(0)] + [sgn * F(1, m) for m in (1, 2, 10, 100, 1024) for sgn in (-1, 1)]
    cases = [fixture(e, F(1, 4), majority) for e in eps]
    controls = [fixture(e, F(1, 8), majority) for e in eps]
    integers = [fixture(sgn * F(1, 1024), F(1, 4), majority, scale=1024,
                        translation=125 * 1024 + 1) for sgn in (-1, 1)]
    for case in integers:
        require(all(F(p).denominator == 1 and 0 <= F(p) < 2 ** 18 for p in case['points']), 'u18 domain')
    require(abs(F(integers[0]['points'][-1]) - F(integers[1]['points'][-1])) == 2, 'two units displacement')
    require(abs(F(integers[0]['meeting_radii']['0,1']) - F(integers[1]['meeting_radii']['0,1'])) == 51200,
            'u18 meeting jump')
    mutation_strict = fixture(F(0), F(1, 4), actual_majority(True), expected=False)
    require(mutation_strict['date_x'] == '55', 'non-strict mutation causal mismatch')
    mutation_band = fixture(F(0), F(1, 4), majority, closed=False, expected=False)
    require(mutation_band['W'] == 3 and mutation_band['date_x'] == '55', 'open band mutation causal mismatch')
    result = {'scope': 'exact rational collinear Gamma2 plus actual AST majority; not native or MMt',
              'snapshot_sha256': SNAPSHOT_SHA, 'normal_optimized_invariant': True,
              'cases': cases, 'positive_eta_1_8': controls, 'u18_integer_cases': integers,
              'mutations_killed': {'majority_nonstrict': mutation_strict['date_x'],
                                   'band_open_at_e0': mutation_band['date_x']},
              'total_checks': sum(c['checks'] for c in cases + controls + integers) +
                              mutation_strict['checks'] + mutation_band['checks']}
    print(json.dumps(result, sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
