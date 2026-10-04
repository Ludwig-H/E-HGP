#!/usr/bin/env python3
"""Exact bounded follow-up of a private model, not a native qualification.

The filtration factorization is valid for ANY finite treegram u with
u_ii=e_i, u_ij>=max(e_i,e_j), and fixed counting dates s_i>=e_i.
For 1<=a<=n let R_i be the a-th smallest max(u_ij,s_j). A counted
neighbor j belongs to the block of i at r iff max(u_ij,s_j)<=r.
Therefore the underlying block is big iff r>=R_i; a pair shares a big
block iff max(u_ij,R_i,R_j)<=r. The lifted treegram is an ultrametric.
Its diagonal R_i records membership in an eligible underlying block,
NOT necessarily counting: first counted membership is max(R_i,s_i).

Consequently standard unit-mass condensation of the lifted treegram
preserves the eligible-block filtration. For arbitrary delayed s it
need not preserve EOM scores, counted cohorts or their interpretation.
Keep the original s_i and counted joins when factoring the model.
For A (s_i=e_i) this distinction vanishes because R_i>=e_i.

The six-site witness uses an exact scalar Gamma_2, independent of the
private oracle. It realizes H^r_3 with entries2, two triples until5/2.
The delayed schedule s=(2,2,3,2,2,2) is an admissible GENERAL calendar;
we do not claim it equals criterion B, C or E for this particular cloud.
Only two frozen target functions are AST-extracted; no imports, fitting,
native executable, production reference or external dependency is run.

For one fixed condensation and fixed cohorts, EOM with r^-z refines as z
increases. At C let a=its birth radius=death of every child. Normalize all
scores by a^z/z. C integrates on r>=a, so its normalized score decreases;
every descendant integrates on r<=a, so each normalized score increases.
The optimal descendant score (sum/max) therefore increases. The predicate
'children strictly win' is monotone, including the parent-at-tie policy.
Every ancestor already selecting descendants keeps doing so; hence a
selected node at larger z cannot lie above the old selected node on its
path. Root admissibility and cohorts must be held fixed. This proof never
compares different counting criteria, and does not imply monotone IoU.
"""
from fractions import Fraction as Q
from itertools import combinations, permutations
from pathlib import Path
import ast
import hashlib
import json

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / 'source/modele/scripts/modele_lib.py'
SOURCE_SHA = '5c0f0f1bd40beb7d156ffe9b45e31ad3fa3bac82bf41c518e6c2db07da193b9d'
CHECKS = 0


def need(ok, message):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise RuntimeError(message)


def functions():
    data = SOURCE.read_bytes()
    need(hashlib.sha256(data).hexdigest() == SOURCE_SHA, 'frozen target source')
    wanted = {'condensed_tree', 'labels_from_selection'}
    nodes = [n for n in ast.parse(data).body if isinstance(n, ast.FunctionDef) and n.name in wanted]
    need({n.name for n in nodes} == wanted, 'only the two named pure target functions')
    env = {}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), str(SOURCE), 'exec'), env)
    return env['condensed_tree'], env['labels_from_selection']


class Treegram:
    def __init__(self, U, s):
        self.n = len(U)
        self.U, self.s = U, s
        self.radii = sorted(set(s) | {v for row in U for v in row})
        ranks = {v: i for i, v in enumerate(self.radii)}
        self.urank = [[ranks[v] for v in row] for row in U]
        self.srank = [ranks[v] for v in s]


def blocks(U, r):
    # Explicit equivalence classes, no cluster/DSU code shared with the target.
    active = {i for i in range(len(U)) if U[i][i] <= r}
    result = []
    while active:
        i = min(active)
        group = frozenset(j for j in active if U[i][j] <= r)
        result.append(group)
        active -= group
    return result


def verify(U, s, a, target):
    n = len(U)
    R = [sorted(max(U[i][j], s[j]) for j in range(n))[a-1] for i in range(n)]
    V = [[max(U[i][j], R[i], R[j]) for j in range(n)] for i in range(n)]
    tg = Treegram(U, s)
    clusters, violations = target(tg, a)
    need(not violations, 'fixed counting dates give monotone masses')
    first = {}
    for cl in clusters:
        for i, rank in cl['join'].items():
            first[i] = min(first.get(i, Q(10**10)), tg.radii[rank])
    need(first == {i: max(R[i], s[i]) for i in range(n)}, 'first counted memberships')
    for r in tg.radii:
        big = [b for b in blocks(U, r) if sum(s[i] <= r for i in b) >= a]
        need(set(big) == set(blocks(V, r)), 'factorized eligible-block filtration')
        for i in range(n):
            for j in range(n):
                same = any(i in b and j in b for b in big)
                need(same == (V[i][j] <= r), 'pair equivalence including diagonal')
        alive = [c for c in clusters if tg.radii[c['birth']] <= r and
                 (c['death'] is None or r < tg.radii[c['death']])]
        counted = sum(sum(tg.radii[rank] <= r for rank in c['join'].values()) for c in alive)
        need(counted == sum(sum(s[i] <= r for i in b) for b in big), 'active counted mass conservation')
    for i in range(n):
        need(V[i][i] == R[i], 'lift diagonal is eligibility, not counting')
        for j in range(n):
            for k in range(n):
                need(V[i][k] <= max(V[i][j], V[j][k]), 'strong triangle inequality')
    return tg, clusters, R, V


def gamma_six():
    X = [0, 2, 4, 7, 9, 11]
    n = len(X)
    parts = list(combinations(range(n), 2))
    cofaces = list(combinations(range(n), 3))
    beta = lambda s: Q((X[max(s)]-X[min(s)])**2, 4)
    levels = sorted({beta(s) for s in parts+cofaces})
    for level in levels:
        parent = {p: p for p in parts if beta(p) <= level}

        def find(p):
            while parent[p] != p:
                p = parent[p]
            return p

        for c in cofaces:
            if beta(c) <= level:
                faces = list(combinations(c, 2))
                for f in faces[1:]:
                    parent[find(f)] = find(faces[0])
        groups = {}
        for p in parent:
            groups.setdefault(find(p), set()).update(p)
        qualified = {frozenset(g) for g in groups.values() if len(g) >= 3}
        expected = set() if level < 4 else {frozenset(range(3)), frozenset(range(3, 6))} \
            if level < Q(25, 4) else {frozenset(range(6))}
        need(qualified == expected, 'independent Gamma_2 exact qualified cuts')
        need(all(sum(i in b for b in qualified) <= 1 for i in range(n)), 'no qualified rival')
    U = [[Q(2) if i//3 == j//3 else Q(5, 2) for j in range(n)] for i in range(n)]
    return X, U, len(levels)


def score(tg, cl, z=1):
    if cl['death'] is None:
        end = Q(0)  # no finite upper-radius endpoint
    else:
        end = tg.radii[cl['death']]**(-z)
    return sum(tg.radii[rank]**(-z)-end for rank in cl['join'].values())


def select(tg, clusters, z):
    # Scalar exact DP, independently of private interval-EOM and sklearn.
    def best(cid):
        c = clusters[cid]
        if not c['children']:
            return score(tg, c, z), {cid}
        child_results = [best(ch) for ch in c['children']]
        child_score = sum(v for v, _ in child_results)
        own_score = score(tg, c, z)
        if own_score >= child_score:
            return own_score, {cid}
        return child_score, set().union(*(s for _, s in child_results))
    chosen = set()
    for c in clusters:
        if c['parent'] is None:  # roots excluded, including singleton root candidates
            for ch in c['children']:
                chosen.update(best(ch)[1])
    return chosen


def ancestors(clusters, cid):
    out = set()
    while cid is not None:
        out.add(cid)
        cid = clusters[cid]['parent']
    return out


def verify_z(tg, clusters):
    sels = [select(tg, clusters, z) for z in range(1, 9)]
    for i in range(8):
        for j in range(i+1, 8):
            need(all(ancestors(clusters, c) & sels[i] for c in sels[j]),
                 'fixed-cohort exact EOM selections refine with z')
    return len(set(frozenset(s) for s in sels))


def main():
    target, label = functions()
    X, U, cuts = gamma_six()
    calendars = [list(map(Q, c)) for c in (
        (2, 2, 2, 2, 2, 2), (2, 2, 3, 2, 2, 2),
        (2, 3, 4, 2, 3, 4), (5, 5, 5, 5, 5, 5))]
    profiles = 0
    z_profiles, z_pairs = 0, 0
    for s in calendars:
        for a in range(1, 7):
            tg_case, clusters_case, _, _ = verify(U, s, a, target)
            verify_z(tg_case, clusters_case)
            z_profiles += 1
            z_pairs += 28
            profiles += 1
    # Exact relabelling checks: target IDs themselves are not claimed canonical.
    s = list(map(Q, (2, 2, 3, 2, 2, 2)))
    for p in ((0, 1, 2, 3, 4, 5), (5, 4, 3, 2, 1, 0), (2, 0, 4, 1, 5, 3)):
        verify([[U[i][j] for j in p] for i in p], [s[i] for i in p], 2, target)
        profiles += 1
    tg, cs, R, V = verify(U, s, 2, target)
    A = next(c for c in cs if c['members'] == [0, 1, 2])
    B = next(c for c in cs if c['members'] == [3, 4, 5])
    need(tg.radii[A['birth']] == 2 and tg.radii[A['death']] == Q(5, 2), 'exact A lifespan')
    need(set(A['join']) == {0, 1} and 2 in A['members'], 'late member exists but never contributes mass in A')
    need(score(tg, A) == Q(1, 5), 'counted cohort A score')
    labels = label(tg, cs, [A['id'], B['id']])
    need(labels[:3] == [0, 0, 0], 'private flat membership absorbs the delayed member')
    lifted_tg, lifted_cs, _, _ = verify(V, R, 2, target)
    lifted_A = next(c for c in lifted_cs if c['members'] == [0, 1, 2])
    need(score(lifted_tg, lifted_A) == Q(3, 10), 'ordinary lifted condensation score differs')
    need(score(tg, A) != score(lifted_tg, lifted_A), 'factor filtration alone does not factor EOM')
    # Preserve s along the lifted filtration: no score change.
    preserved_tg, preserved_cs, _, _ = verify(V, s, 2, target)
    preserved_A = next(c for c in preserved_cs if c['members'] == [0, 1, 2])
    need(score(preserved_tg, preserved_A) == score(tg, A), 'preserving delayed cohorts preserves score')
    # Changing the calendar invalidates the refinement comparison, even
    # though each separate calendar satisfies the theorem for all z.
    U9 = [[Q(2) if i//3 == j//3 else Q(3) if i < 6 and j < 6 else Q(4)
           for j in range(9)] for i in range(9)]
    tg_A, cs_A, _, _ = verify(U9, [Q(2)]*9, 3, target)
    tg_C, cs_C, _, _ = verify(U9, [Q(29, 10)]*6+[Q(2)]*3, 3, target)
    verify_z(tg_A, cs_A)
    verify_z(tg_C, cs_C)
    z_profiles += 2
    z_pairs += 56
    groups_A = {tuple(cs_A[c]['members']) for c in select(tg_A, cs_A, 1)}
    groups_C = {tuple(cs_C[c]['members']) for c in select(tg_C, cs_C, 3)}
    need(groups_A == {(0, 1, 2), (3, 4, 5), (6, 7, 8)}, 'earlier counting z1 chooses three leaves')
    need(groups_C == {(0, 1, 2, 3, 4, 5), (6, 7, 8)}, 'delayed counting z3 may instead choose ancestor')
    need(not all(any(set(g) <= set(h) for h in groups_A) for g in groups_C),
         'no z-refinement theorem across different calendars')
    print(json.dumps(dict(scope='private exploratory pure math, no native/fit/GCP', checks=CHECKS,
                         profiles=profiles, target_source_sha256=SOURCE_SHA,
                         fixed_cohort_z_profiles=z_profiles, fixed_cohort_z_pairs=z_pairs,
                         cross_calendar_counterguard=dict(
                             scope='two admissible general calendars, not claimed actual A/C of these points',
                             early_z1_groups=sorted(groups_A), delayed_z3_groups=sorted(groups_C)),
                         witness=dict(points=X, order=2, qualification=3, gamma_closed_cuts=cuts,
                                      entry='2', merge='5/2', counting_dates=list(map(str, s)), mcs=2,
                                      branch_A_members=A['members'], branch_A_counted=list(A['join']),
                                      original_score=str(score(tg, A)),
                                      lifted_standard_score=str(score(lifted_tg, lifted_A)),
                                      preserved_cohorts_score=str(score(preserved_tg, preserved_A)),
                                      selected_labels=labels),
                         result='factorization exact for eligible blocks; transport original counting cohorts'),
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
