"""Exact causal audit of a query architecture, not a native/GPU benchmark."""
import ast
from collections import defaultdict
from fractions import Fraction as F
import hashlib
from itertools import combinations
import json
from math import isqrt
from pathlib import Path
import random

BASE = Path(__file__).resolve().parent
SNAPSHOT_SHA = '772a9c58682fd72ae2db99988ab4aeb465d6d5655c1810cd936f5cea7a1ca9c5'


def need(ok, msg):
    if not ok:
        raise RuntimeError(msg)


def sq(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b))


def ceil(v):
    v = F(v)
    return -((-v.numerator) // v.denominator)


def grid_class(mutant=False):
    raw = (BASE / 'sitegrid_snapshot.py').read_bytes()
    need(hashlib.sha256(raw).hexdigest() == SNAPSHOT_SHA, 'snapshot pin')
    src = raw.decode()
    if mutant:
        old = 'out.extend(self.cells.get((cx, cy, cz), ()))'
        need(src.count(old) == 1, 'candidate mutation site')
        src = src.replace(old, 'out.extend(s for s in self.cells.get((cx, cy, cz), ()) '
                              'if all(blo[i] <= self.sites[s][i] <= bhi[i] for i in range(3)))')
    tree = ast.parse(src)
    need(len(tree.body) == 1 and isinstance(tree.body[0], ast.ClassDef) and tree.body[0].name == 'SiteGrid',
         'only SiteGrid AST')
    env = {'__builtins__': __builtins__}
    exec(compile(tree, 'sitegrid_snapshot.py', 'exec'), env)
    return env['SiteGrid']


def grid_case(n, Grid):
    m = n - 2
    need(m % 2 == 0, 'even paired bulk')
    L = 1
    while L ** 3 < m // 2:
        L += 1
    A, LIM = 110000, (1 << 18) - 1
    dense = [(A + 4 * ((i // 2) % L) + (i % 2),
              A + 4 * (((i // 2) // L) % L),
              A + 4 * ((i // 2) // (L * L))) for i in range(m)]
    points = dense + [(0, 0, 0), (LIM, LIM, LIM)]
    need(len(set(points)) == n and all(0 <= z <= LIM for p in points for z in p), 'distinct u18 inputs')
    cell = Grid(points)
    key = cell._cell(dense[0])
    need(len(cell.cells) == 3 and cell.cells[key] == list(range(m)), 'single full dense cell')
    need(all(cell._cell(p) == key for p in dense), 'all dense sites same cell')
    # A global margin proves every vote and mate-census box belongs to that cell.
    for axis in range(3):
        lo = min(p[axis] for p in dense) - 3
        hi = max(p[axis] for p in dense) + 3
        need((lo - cell.lo[axis]) // cell.h == key[axis] ==
             (hi - cell.lo[axis]) // cell.h, 'all padded boxes same cell')
    # Mate distance1; all other dense-site differences have a coordinate of magnitude>=3.
    need(all(sq(dense[i], dense[i ^ 1]) == 1 for i in range(m)), 'unit mates')
    need(all(p[0] % 4 == (A + (i % 2)) % 4 and (p[1] - A) % 4 == 0 and (p[2] - A) % 4 == 0
             for i, p in enumerate(dense)), 'paired lattice separation certificate')
    sample_ids = [0, m // 2, m - 1]
    sample = []
    for i in sample_ids:
        p = dense[i]
        # paires_k2.lignes alpha2=1/4, seuil4=25/16 -> isqrt(int(seuil4)+1)+1=2.
        half = isqrt(int(F(25, 16)) + 1) + 1
        cand = cell.candidates([v - half for v in p], [v + half for v in p])
        need(cand == list(range(m)), 'actual candidates returns whole dense cell')
        retained = [j for j in cand if j != i and 16 * sq(p, points[j]) <= 25]
        need(retained == [i ^ 1], 'one actual retained bulk vote')
        a = i - (i % 2); b = a + 1
        S = [points[a][k] + points[b][k] for k in range(3)]
        r = isqrt(sq(points[a], points[b])) // 2 + 1
        dcand = cell.candidates([S[k] // 2 - r - 1 for k in range(3)],
                               [(S[k] + 1) // 2 + r + 1 for k in range(3)])
        need(dcand == list(range(m)), 'actual mate census candidates whole cell')
        third = [j for j in dcand if j not in (a, b) and
                 sq(tuple(2 * z for z in points[j]), S) <= 1]
        need(not third, 'empty mate diameter ball')
        sample.append({'site': i, 'candidate_ids': len(cand), 'retained_votes': len(retained),
                       'mate_census_ids': len(dcand), 'mate_census_sphere_tests': len(dcand) - 2})
    # Two O(n) exact computations determine extreme-site votes; no all-pairs loop.
    extreme_votes = []
    for i in (m, m + 1):
        d2 = min(sq(points[i], p) for j, p in enumerate(points) if j != i)
        ids = [j for j, p in enumerate(points) if j != i and 16 * sq(points[i], p) <= 25 * d2]
        need(ids == list(range(m)), 'each extreme retains all dense sites and no other extreme')
        extreme_votes.append(len(ids))
    positive = Grid(dense)
    need(positive.h == 4 and max(map(len, positive.cells.values())) == 2, 'positive grid load')
    need(all(all((p[k] - positive.lo[k]) % 4 <= 1 for k in range(3)) for p in dense), 'positive residues')
    pcandidates = [len(positive.candidates([v - 2 for v in dense[i]], [v + 2 for v in dense[i]]))
                   for i in sample_ids]
    need(all(2 <= q <= 16 for q in pcandidates), 'positive local candidate control')
    return {'n': n, 'bulk_m': m, 'h': cell.h, 'occupied_cells': len(cell.cells), 'bulk_cell_load': m,
            'sample_queries': sample, 'total_votes_exact_derived': 3 * m,
            'bulk_candidate_ids_total_derived': m * m,
            'bulk_lignes_distance_tests_total_derived': m * (m - 1),
            'mate_census_tests_no_warmup_derived': (m // 2) * (m - 2),
            'mate_census_tests_after_300_births_lower_bound': (m // 2 - 300) * (m - 2),
            'positive_h': positive.h, 'positive_candidate_samples': pcandidates,
            'positive_candidate_total_upper_bound_derived': 16 * m,
            'counter_scope': 'sampled real AST queries plus global same-cell deductions; no quadratic loop executed'}


def packing_case(points, K, c):
    n = len(points)
    need(len(set(points)) == n and 2 <= K <= n, 'packing distinct and K includes self')
    all_d2 = [sq(a, b) for a, b in combinations(points, 2)]
    need(min(all_d2) == 1, 'integer fixture delta=1')
    D2 = max(all_d2)
    dK2 = [sorted(sq(p, q) for q in points)[K - 1] for p in points]
    L = 1
    while 4 ** L <= D2:
        L += 1
    total, max_bucket = 0, 0
    outgoing = [0] * n
    for y in range(n):
        buckets = defaultdict(list)
        for x in range(n):
            if x == y or sq(points[x], points[y]) > c * c * dK2[x]:
                continue
            R = 1
            while 4 * R * R <= dK2[x]:
                R *= 2
            need(R * R <= dK2[x] < 4 * R * R, 'anchor KNN radius bucket')
            h = F(R, 2)
            cube = tuple(int(F(points[x][k] - points[y][k] + 2 * c * R) // h) for k in range(3))
            need(all(0 <= t < ceil(8 * c) for t in cube), 'incident anchor bounding cube')
            buckets[(R, cube)].append(x)
            total += 1
            outgoing[x] += 1
        for (R, _cube), xs in buckets.items():
            need(len(xs) <= K - 1, 'K−1 incoming anchor cap')
            for a, b in combinations(xs, 2):
                need(sq(points[a], points[b]) < R * R, 'strict same-cell diameter')
            max_bucket = max(max_bucket, len(xs))
    bound = (K - 1) * (ceil(8 * c) + 1) ** 3 * n * L
    need(total <= bound, 'global derived packing bound')
    return {'n': n, 'K': K, 'c': str(c), 'radius_buckets': L, 'directed_pairs': total,
            'global_bound': bound, 'max_incoming_bucket_load': max_bucket,
            'max_outgoing_degree': max(outgoing)}


def main():
    Grid = grid_class()
    grids = [grid_case(n, Grid) for n in (8000, 16000, 32000)]
    rng = random.Random('private-pair-packing-20260930')
    clouds = [[(0, 0, 0), (100, 0, 0), (101, 0, 0)],
              [(0, 0, 0), (1, 0, 0)] + [(100 + i, 0, 0) for i in range(18)]]
    for M in (2, 8, 64, 512):
        pts = {(0, 0, 0), (1, 0, 0)}
        while len(pts) < 24:
            pts.add(tuple(rng.randrange(-M, M + 1) for _ in range(3)))
        clouds.append(sorted(pts))
    pack = [packing_case(pts, K, c) for pts in clouds for K in (2, 3, 5, 10) if K <= len(pts)
            for c in (F(5, 4), F(5, 2), F(4))]
    witnesses = [r for r in pack if r['n'] == 3 and r['K'] == 3 and r['c'] == '5/4']
    need(len(witnesses) == 1 and witnesses[0]['max_incoming_bucket_load'] == 2,
         'mutation dropping K−1 killed by exact incoming cap2')
    # This mutant changes returned IDs, not the cost of visiting a dense cell.
    try:
        grid_case(8000, grid_class(True))
    except RuntimeError as exc:
        need(str(exc) == 'actual candidates returns whole dense cell', 'candidate mutation killed causally')
        killed = str(exc)
    else:
        raise RuntimeError('candidate mutation survived')
    print(json.dumps({'scope': 'cardinality packing and exact SiteGrid counterexample only; not native/FULL/G4',
                      'snapshot_sha256': SNAPSHOT_SHA, 'grid_cases': grids, 'packing_cases': pack,
                      'packing_cases_count': len(pack), 'mutation_candidates_killed': killed,
                      'mutation_cap_one_killed': witnesses[0]['max_incoming_bucket_load']},
                     sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
