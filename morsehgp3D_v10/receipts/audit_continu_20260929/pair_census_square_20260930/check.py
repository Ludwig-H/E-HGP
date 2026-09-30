"""Small exact checks of the pinned resolver; large sizes are formulas only."""
import ast
from fractions import Fraction
import hashlib
import json
import math
from pathlib import Path

BASE = Path(__file__).resolve().parent
A = 128000
SNAPSHOT_SHA = '5f234ded085eb3d3fca7d072a268265a02605559f7c37793220cec65aba1d63b'


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def distance(p, q):
    return sum((p[i] - q[i]) ** 2 for i in range(3))


class Counts:
    def __init__(self):
        self.phase = 'idle'
        self.census_first = False
        self.sphere = 0
        self.argmin = 0
        self.diameter = 0
        self.level = 0

    def sq_dist(self, p, q):
        if self.phase == 'census':
            if self.census_first:
                self.diameter += 1
                self.census_first = False
            else:
                self.sphere += 1
        elif self.phase == 'argmin':
            self.argmin += 1
        elif self.phase == 'level':
            self.level += 1
        return distance(p, q)


def real_methods(counts, mutant=None):
    raw = (BASE / 'resolver_snapshot.py').read_bytes()
    need(hashlib.sha256(raw).hexdigest() == SNAPSHOT_SHA, 'snapshot pin')
    tree = ast.parse(raw)
    method_names = [n.name for n in tree.body[0].body]
    need(method_names == ['pair_level', 'third_sites', 'resolve'], 'exact AST method inventory')
    if mutant == 'midpoint':
        # Deliberately changes only the point used by min, not geometry/owner lookup.
        text = raw.decode().replace(
            'sq_dist(X, self.sites[s])',
            'sq_dist(tuple(Fraction(self.sites[cur[0]][i] + self.sites[cur[1]][i], 2) for i in range(3)), self.sites[s])')
        need(text != raw.decode(), 'midpoint mutant applied')
        tree = ast.parse(text)
    elif mutant == 'memo_off':
        text = raw.decode().replace('if keep == 0 and key in self.memo:', 'if False:')
        text = text.replace('if keep == 0 and ck in self.memo:', 'if False:')
        need(text.count('if False:') == 2, 'two memo-read mutants applied')
        tree = ast.parse(text)
    env = {'Fraction': Fraction, 'math': math, 'require': need, 'sq_dist': counts.sq_dist}
    exec(compile(tree, '<pinned resolver AST>', 'exec'), env)
    return env['ExactResolverMethods']


class PerfectInteriorGrid:
    """A stronger-than-index oracle: supplies only the exact interiors lazily."""
    def __init__(self, sites, reverse):
        self.sites = sites
        self.reverse = reverse
        self.a = self.b = None
        self.yields = 0

    def prepare(self, a, b):
        self.a, self.b = min(a, b), max(a, b)

    def candidates(self, lo, hi):
        ids = range(self.a + 1, self.b)
        if self.reverse:
            ids = reversed(ids)
        for z in ids:
            p = self.sites[z]
            need(all(lo[i] <= p[i] <= hi[i] for i in range(3)), 'exact interior inside actual query box')
            self.yields += 1
            yield z


class ToyForest:
    def __init__(self, m):
        self.birth = {'E': Fraction(A * A, 4)}
        self.parent = {}
        self.seed = {'E': Fraction(A, 2)}
        for i in range(m - 1):
            name = 'B' + str(i)
            self.birth[name] = Fraction(1, 4)
            self.seed[name] = Fraction(2 * (A + i) + 1, 2)
        if m == 2:
            bulk = 'B0'
        else:
            bulk = 'B'
            self.birth[bulk] = Fraction(1)
            self.seed[bulk] = self.seed['B0']
            for i in range(m - 1):
                self.parent['B' + str(i)] = bulk
        self.birth['R'] = Fraction((A + 1) ** 2, 4)
        self.seed['R'] = self.seed['E']
        self.parent['E'] = 'R'
        self.parent[bulk] = 'R'

    def ancestor(self, node, beta, closed):
        need(closed is True, 'actual resolver requests closed ancestor')
        if self.birth[node] > beta:
            return None
        while node in self.parent and self.birth[self.parent[node]] <= beta:
            node = self.parent[node]
        return node


class Adapter:
    def __init__(self, m, reverse=False, mutant=None):
        self.sites = [(0, 0, 0)] + [(A + j, 0, 0) for j in range(m)]
        self.counts = Counts()
        self.real = real_methods(self.counts, mutant)
        self.grid = PerfectInteriorGrid(self.sites, reverse)
        self.forest = ToyForest(m)
        self.memo = {}
        self.steps = self.births_reached = self.census_calls = 0
        self.peak_list = 0
        self.trace = []

    def pair_level(self, a, b):
        old = self.counts.phase
        self.counts.phase = 'level'
        try:
            return self.real.pair_level(self, a, b)
        finally:
            self.counts.phase = old

    def third_sites(self, a, b):
        self.grid.prepare(a, b)
        self.counts.phase = 'census'
        self.counts.census_first = True
        out = self.real.third_sites(self, a, b)
        self.counts.phase = 'argmin'
        self.trace.append((a, b, len(out)))
        self.peak_list = max(self.peak_list, len(out))
        need(isinstance(out, list), 'actual third_sites list materialization')
        return out

    def birth_of_empty_pair(self, a, b):
        p = tuple(sorted((a, b)))
        if p == (0, 1):
            return 'E'
        need(p[0] >= 1 and p[1] == p[0] + 1, 'analytic empty-pair catalogue domain')
        return 'B' + str(p[0] - 1)

    def resolve(self, a, b, keep=0):
        return self.real.resolve(self, a, b, keep)

    def paid(self):
        return (self.census_calls, self.steps, self.births_reached, self.counts.sphere,
                self.counts.argmin, self.counts.diameter, self.counts.level, self.grid.yields)


def band_pairs(m):
    out = [(0, j) for j in range(1, m + 1)]
    for j in range(1, m + 1):
        if j > 1:
            out.append((j, j - 1))
        if j < m:
            out.append((j, j + 1))
    need(len(out) == 3 * m - 2, 'exact directed-band count')
    # Exhaustive independent band membership at small sizes only.
    if m <= 16:
        xs = [0] + [A + j for j in range(m)]
        truth = set()
        for i, x in enumerate(xs):
            nearest2 = min((x - y) ** 2 for j, y in enumerate(xs) if j != i)
            for j, y in enumerate(xs):
                if j != i and 16 * (x - y) ** 2 <= 25 * nearest2:
                    truth.add((i, j))
        need(set(out) == truth, 'exhaustive exact eta1/4 K2 band membership')
    return out


def rational_sqrt(beta):
    a, b = math.isqrt(beta.numerator), math.isqrt(beta.denominator)
    need(a * a == beta.numerator and b * b == beta.denominator, 'rational oracle radius')
    return Fraction(a, b)


def intervals(sites, beta):
    r = rational_sqrt(beta)
    coords = [p[0] for p in sites]
    raw = []
    for i, x in enumerate(coords):
        for y in coords[i + 1:]:
            if y - x <= 2 * r:
                raw.append((Fraction(y) - r, Fraction(x) + r))
    merged = []
    for lo, hi in sorted(raw):
        if not merged or lo > merged[-1][1]:
            merged.append([lo, hi])
        else:
            merged[-1][1] = max(merged[-1][1], hi)
    return merged


def owner_check(solver, a, b, owner):
    beta = Fraction(distance(solver.sites[a], solver.sites[b]), 4)
    f = solver.forest
    need(f.ancestor(owner, beta, True) == owner, 'owner live at own exact pair level')
    comps = intervals(solver.sites, beta)
    midpoint = Fraction(solver.sites[a][0] + solver.sites[b][0], 2)
    ids = [i for i, (lo, hi) in enumerate(comps) if lo <= midpoint <= hi]
    seed_ids = [i for i, (lo, hi) in enumerate(comps) if lo <= f.seed[owner] <= hi]
    need(len(ids) == 1 and seed_ids == ids, 'owner and midpoint in independent L2 component')
    live = {f.ancestor(v, beta, True) for v in f.birth if f.birth[v] <= beta}
    need(len(live) == len(comps), 'toy forest component count matches complete L2 intervals')
    expected = 'E' if tuple(sorted((a, b))) == (0, 1) else (
        'R' if min(a, b) == 0 else 'B' + str(min(a, b) - 1))
    need(owner == expected, 'exact analytic pair owner')


def small_case(m, query_reverse, candidate_reverse):
    solver = Adapter(m, candidate_reverse)
    pairs = band_pairs(m)
    if query_reverse:
        pairs.reverse()
    for a, b in pairs:
        owner_check(solver, a, b, solver.resolve(a, b))
    count = m * (m - 1) // 2
    need(solver.census_calls == 2 * m - 1 and len(solver.memo) == 2 * m - 1,
         'fresh memo states exactly unique accepted undirected pairs')
    need(solver.counts.sphere == count and solver.counts.argmin == count and solver.grid.yields == count,
         'exact census and anchor-argmin quadratic work')
    need(solver.counts.diameter == 2 * m - 1 and solver.steps == m - 1 and
         solver.births_reached == m and solver.peak_list == m - 1, 'paid list and descent controls')
    before = solver.paid()
    for a, b in pairs:
        solver.resolve(a, b)
    need(solver.paid() == before, 'global keep0 memo eliminates all repeated paid states')
    return {'m': m, 'directed_votes': len(pairs), 'fresh_states': solver.census_calls,
            'sphere_tests': solver.counts.sphere, 'argmin_distances': solver.counts.argmin,
            'peak_materialized_list': solver.peak_list, 'query_reverse': query_reverse,
            'candidate_reverse': candidate_reverse, 'owners_checked': len(pairs), 'replay_no_new_work': True}


def streaming_control(m):
    solver = Adapter(m, True)
    actual = solver.resolve(0, m, keep=1)
    need(solver.trace[:2] == [(m, 0, m - 1), (m, m - 1, 0)], 'argmin uses held endpoint, not midpoint')
    owner_check(solver, 0, m, actual)
    need(not solver.memo, 'keep1 uncached')
    paid = solver.paid()
    solver.resolve(0, m, keep=1)
    need(solver.census_calls == 2 * paid[0], 'keep1 repeat pays fresh census again')
    mutant = Adapter(m, True, 'midpoint')
    mutant_owner = mutant.resolve(0, m, keep=1)
    need(mutant.trace[1][1] == 1 and mutant.trace[1][1] != m - 1, 'midpoint mutant different selected endpoint')
    owner_check(mutant, 0, m, mutant_owner)
    # Streaming comparator is exactly the current argmin comparator. Not stopfirst.
    total = 0
    differences = 0
    for j in range(1, m + 1):
        best = None
        key = None
        candidates = range(j - 1, 0, -1)
        for z in candidates:
            cur = (distance(solver.sites[0], solver.sites[z]), z)
            total += 1
            if key is None or cur < key:
                best, key = z, cur
        expected = min(candidates, key=lambda z: (distance(solver.sites[0], solver.sites[z]), z)) if candidates else None
        need(best == expected, 'streaming exact argmin equals current list argmin')
        if candidates and candidates[0] != best:
            differences += 1
    need(total == m * (m - 1) // 2 and differences == m - 2, 'streaming still quadratic; stopfirst selection differs')
    cache_mutant = Adapter(m, False, 'memo_off')
    pairs = band_pairs(m)
    for a, b in pairs:
        owner_check(cache_mutant, a, b, cache_mutant.resolve(a, b))
    old = cache_mutant.paid()
    for a, b in pairs:
        cache_mutant.resolve(a, b)
    need(cache_mutant.paid()[0] > old[0], 'memo mutant killed by repeat census cost, not owner correctness')
    return {'m': m, 'keep1_trace_first_two': [list(v) for v in solver.trace[:2]],
            'keep1_owner': actual, 'midpoint_mutant_next_endpoint': mutant.trace[1][1],
            'midpoint_mutant_owner': mutant_owner, 'streaming_visits': total,
            'streaming_peak_selected_records': 1, 'stopfirst_different_selections': differences,
            'keep1_repeat_uncached': True, 'memo_read_mutant_killed': True}


def analytic_case(m):
    # No sites, candidate grid, resolver, or loop over witnesses at these sizes.
    need(2 <= m <= 32000 and A + m - 1 < 2 ** 18, 'analytic u18 domain')
    need(4 * (A + m - 1) <= 5 * A, 'all anchor votes inside exact eta1/4 band')
    witness = m * (m - 1) // 2
    return {'m': m, 'n': m + 1, 'directed_votes': 3 * m - 2,
            'fresh_keep0_states': 2 * m - 1, 'sphere_tests': witness,
            'argmin_distances': witness, 'sphere_plus_argmin': 2 * witness,
            'peak_materialized_list': m - 1, 'measurement': 'analytic_only'}


def main():
    small = [small_case(m, q, c) for m in (2, 3, 5, 8, 16) for q in (False, True) for c in (False, True)]
    result = {'scope': 'pinned actual census/argmin with ideal strict-interior generator and analytic toy forest; no native/GCP',
              'snapshot_sha256': SNAPSHOT_SHA, 'small_cases': small,
              'streaming_controls': [streaming_control(m) for m in (3, 5, 8, 16)],
              'large_analytic': [analytic_case(m) for m in (8000, 16000, 32000)],
              'memo_scope': 'synchronous keep0 accepted-band universe; no eviction; not keep1/workers/arbitrary probes',
              'mutants_killed': {'midpoint_not_anchor': True, 'memo_read_disabled': True}}
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
