"""Independent bounded checks of the corrected line oracle and projection claims.

Gamma is built from all k-subsets and (k+1)-subsets, using the elementary
minimum enclosing radius of collinear points. No product executable is used.
The unchanged Line class and a small subset of the repository's exact 3D
reference are cross-checked separately. Python normal and -O have the same
semantics; all failures are explicit, with no assert.
"""
import hashlib
import importlib.util
import json
import sys
from fractions import Fraction as F
from itertools import combinations, product
from pathlib import Path

sys.dont_write_bytecode = True
CAPTURE = Path(__file__).resolve().parents[1]
SRC = CAPTURE / 'sources/morsehgp3D_v10'
GATE = SRC / 'tests/regression/test_projection_facts.py'
REFERENCE = SRC / 'reference/hgp10_ref.py'


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class AllSubsetsGamma:
    def __init__(self, xs, k):
        if not 1 <= k <= len(xs) or len(set(xs)) != len(xs):
            raise ValueError('distinct sites and 1 <= K <= n required')
        self.xs = tuple(sorted(xs))
        self.k = k
        self.beta = lambda ids: F((max(self.xs[i] for i in ids) - min(self.xs[i] for i in ids)) ** 2, 4)
        self.vertices = {v: self.beta(v) for v in combinations(range(len(xs)), k)}
        self.edges = {g: self.beta(g) for g in combinations(range(len(xs)), k + 1)}
        self.entry = [F(sorted(abs(x - y) for y in self.xs)[k - 1] ** 2) for x in self.xs]
        self.alpha = [min(lv for v, lv in self.vertices.items() if i in v) for i in range(len(xs))]
        self.levels = sorted(set(self.vertices.values()) | set(self.edges.values()) | set(self.entry) | {F(0)})
        self.snapshots = {}

    def snap(self, a, closed=True):
        key = a, closed
        if key in self.snapshots:
            return self.snapshots[key]
        ok = (lambda lv: lv <= a) if closed else (lambda lv: lv < a)
        parent = {v: v for v, lv in self.vertices.items() if ok(lv)}

        def find(v):
            while parent[v] != v:
                parent[v] = parent[parent[v]]
                v = parent[v]
            return v

        for edge, lv in self.edges.items():
            if ok(lv):
                facets = list(combinations(edge, self.k))
                for v in facets[1:]:
                    parent[find(v)] = find(facets[0])
        coverage = {}
        for v in parent:
            coverage.setdefault(find(v), set()).update(self.xs[i] for i in v)
        core = {}
        for i, x in enumerate(self.xs):
            if ok(self.entry[i]):
                nearest = tuple(sorted(sorted(range(len(self.xs)), key=lambda j: (abs(x - self.xs[j]), j))[:self.k]))
                core.setdefault(find(nearest), set()).add(x)
        out = dict(components=len(coverage), covers=sorted(sorted(v) for v in coverage.values()),
                   core=sorted(sorted(v) for v in core.values()), owners={v: find(v) for v in parent})
        self.snapshots[key] = out
        return out

    def u(self, x, y):
        return next(a for a in self.levels if any(x in b and y in b for b in self.snap(a)['core']))

    def u_cover(self, i, j):
        # F2 has unique minimizers, so no product tie-breaking is copied.
        vx = [v for v, lv in self.vertices.items() if i in v and lv == self.alpha[i]]
        vy = [v for v, lv in self.vertices.items() if j in v and lv == self.alpha[j]]
        if len(vx) != 1 or len(vy) != 1:
            raise ValueError('cover witness requires unique first-cover subsets')
        return next(a for a in self.levels if a >= max(self.alpha[i], self.alpha[j]) and
                    self.snap(a)['owners'][vx[0]] == self.snap(a)['owners'][vy[0]])


before = {str(p.relative_to(CAPTURE)): digest(p) for p in (GATE, REFERENCE)}
gate, ref = load('unchanged_projection_gate', GATE), load('bounded_exact_reference', REFERENCE)
counts = dict(configurations=0, partitions=0, components=0, covers=0, alpha=0, alpha_bounds=0,
              pair_heights=0, zero=0, all_sites_order=0, reference_cuts=0, stability_pairs=0, stability_entries=0)
errors = []


def check(condition, context):
    if not condition:
        errors.append(context)


clouds = [list(s) for n in range(2, 6) for s in combinations((0, 1, 2, 3, 5), n)]
for xs in clouds:
    for k in range(1, len(xs) + 1):
        counts['configurations'] += 1
        allsets, line = AllSubsetsGamma(xs, k), gate.Line(xs, k)
        radii = sorted({gate.root(a) for a in allsets.levels})
        probes = radii + [(a + b) / 2 for a, b in zip(radii, radii[1:])] + [max(radii) + 1]
        for r in probes:
            expected = allsets.snap(r * r)
            counts['partitions'] += 1
            check(line.core_partition(r) == expected['core'], ('core', xs, k, r))
            comps = line.components(r)
            counts['components'] += 1
            check(len(comps) == expected['components'], ('components', xs, k, r))
            counts['covers'] += 1
            check(sorted(line.discrete_cover(c, r) for c in comps) == expected['covers'], ('cover', xs, k, r))
        for i, x in enumerate(xs):
            counts['alpha'] += 1
            check(line.alpha(x)[0] ** 2 == allsets.alpha[i], ('alpha', xs, k, x))
            counts['alpha_bounds'] += 1
            check(line.d_k(x) / 2 <= line.alpha(x)[0] <= line.d_k(x), ('alpha_bound', xs, k, x))
        for x, y in combinations(xs, 2):
            counts['pair_heights'] += 1
            check(line.u_core(x, y) ** 2 == allsets.u(x, y), ('u_core', xs, k, x, y))
        counts['zero'] += 1
        zero = allsets.snap(F(0))
        check(zero['components'] == (len(xs) if k == 1 else 0), ('zero', xs, k))
        check(allsets.snap(F(0), False)['components'] == 0, ('zero_open', xs, k))
        if k == len(xs):
            counts['all_sites_order'] += 1
            check(len(allsets.vertices) == 1 and not allsets.edges, ('K=n vertices', xs))

# Cross-check the small all-subsets graph against the repository's independent
# circumcenter/minimum-enclosing-ball reference, including both contact cuts.
for xs, k in (([0, 2, 4], 1), ([0, 2, 4], 2), ([0, 2, 4], 3),
              ([0, 1, 4, 7], 4), ([0, 20, 22, 50, 52], 2)):
    gamma = AllSubsetsGamma(xs, k)
    pts = [(x, 0, 0) for x in xs]
    for a in gamma.levels:
        for closed in (False, True):
            counts['reference_cuts'] += 1
            actual = sorted(sorted(xs[i] for i in block) for block in ref.point_partition_gamma(pts, k, a, closed))
            check(actual == gamma.snap(a, closed)['core'], ('3D_reference', xs, k, a, closed))

# Label-preserving bounded perturbations, independently evaluated by Gamma.
original = [2, 5, 9]
for move in product((-1, 1), repeat=3):
    perturbed = [x + dx for x, dx in zip(original, move)]
    for k in range(1, 4):
        left, right = AllSubsetsGamma(original, k), AllSubsetsGamma(perturbed, k)
        for i, j in combinations(range(3), 2):
            counts['stability_pairs'] += 1
            check(abs(gate.root(left.u(original[i], original[j])) - gate.root(right.u(perturbed[i], perturbed[j]))) <= 2,
                  ('stability_pair', move, k, i, j))
        for i in range(3):
            counts['stability_entries'] += 1
            check(abs(gate.root(left.entry[i]) - gate.root(right.entry[i])) <= 2, ('stability_entry', move, k, i))

line = gate.Line([0, 2, 4], 2)
border = dict(radius='1', continuous_traces=[list(line.trace(c, F(1))) for c in line.components(F(1))],
              discrete_covers=[line.discrete_cover(c, F(1)) for c in line.components(F(1))],
              core=line.core_partition(F(1)), admissible_exclusive= line.cover_partitions(F(1)))
crossing = dict(first=AllSubsetsGamma([0, 20, 22, 50, 52], 1).snap(F(100))['core'],
                second=AllSubsetsGamma([0, 20, 22, 50, 52], 2).snap(F(225))['core'])
first_cover_jumps = []
for L in (1000, 100000):
    left, right = AllSubsetsGamma([0, L - 1, 2 * L], 2), AllSubsetsGamma([0, L + 1, 2 * L], 2)
    ul, ur = gate.root(left.u_cover(0, 1)), gate.root(right.u_cover(0, 1))
    cl, cr = gate.root(left.u(0, L - 1)), gate.root(right.u(0, L + 1))
    check(ul == F(L - 1, 2) and ur == L and ur - ul == F(L + 1, 2), ('F2', L))
    check(cl == L - 1 and cr == L + 1, ('F2_core', L))
    first_cover_jumps.append(dict(L=L, epsilon=2, cover_before=ul, cover_after=ur, cover_jump=ur - ul,
                                  core_before=cl, core_after=cr))

# Static coherent leaf weights do not make an independently recomputed argmax
# assignment coherent across cuts. There are no ties in this example.
weights = dict(A1=F(3, 10), A2=F(3, 10), B=F(4, 10))
fine_winner = max(weights, key=weights.get)
coarse_weights = dict(A=weights['A1'] + weights['A2'], B=weights['B'])
coarse_winner = max(coarse_weights, key=coarse_weights.get)
fine_points, coarse_points = [['q', 'b'], ['a']], [['q', 'a'], ['b']]
fine_refines_coarse = all(any(set(b) <= set(c) for c in coarse_points) for b in fine_points)
vote = dict(leaf_weights=weights, fine_winner=fine_winner, coarse_weights=coarse_weights,
            coarse_winner=coarse_winner, fine_points=fine_points, coarse_points=coarse_points,
            fine_refines_coarse=fine_refines_coarse, scope='abstract tree, no product vote implementation claimed')
check(sum(weights.values()) == 1 and fine_winner == 'B' and coarse_winner == 'A', 'vote witness')
check(not fine_refines_coarse, 'vote witness must break refinement')

after = {str(p.relative_to(CAPTURE)): digest(p) for p in (GATE, REFERENCE)}
check(before == after, 'snapshot sources changed')
print(json.dumps(dict(counts=counts, errors=errors, before=before, after=after, border=border,
                     crossing=crossing, first_cover_jumps=first_cover_jumps, vote=vote,
                     zero_K1=dict(closed=[[0], [2], [4]], open=[]),
                     valid_domain='distinct collinear sites; 1 <= K <= n; finite closed radii',
                     product_executions=0), indent=2, default=str))
sys.exit(1 if errors else 0)
