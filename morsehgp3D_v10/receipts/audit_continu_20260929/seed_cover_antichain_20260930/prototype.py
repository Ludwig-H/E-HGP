"""Exact structural prototype; no native engine, geometric oracle, numpy or GCP."""
from bisect import bisect_right
from fractions import Fraction as F
import json
from pathlib import Path
import random

ROOT = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise ValueError(message)


class Tree:
    def __init__(self, dates, parents, unique_root=True):
        self.date = tuple(F(t) for t in dates)
        self.parent = tuple(parents)
        h = len(self.date)
        need(h > 0 and len(self.parent) == h, 'empty or inconsistent tree')
        self.children = [[] for _ in range(h)]
        self.roots = []
        for v, p in enumerate(self.parent):
            need(type(p) is int and -1 <= p < h and p != v, 'bad parent')
            need(self.date[v] >= 0, 'negative birth')
            if p == -1:
                self.roots.append(v)
            else:
                need(self.date[p] >= self.date[v], 'nonmonotone birth')
                self.children[p].append(v)
        need(bool(self.roots) and (not unique_root or len(self.roots) == 1), 'expected one root')
        seen = set()
        stack = list(self.roots)
        while stack:
            v = stack.pop()
            need(v not in seen, 'cycle or repeated child')
            seen.add(v)
            stack.extend(self.children[v])
        need(len(seen) == h, 'unreachable cycle')

    def owner(self, v, t, open_cut=False):
        need(0 <= v < len(self.parent) and self.date[v] <= t, 'seed before node birth')
        while self.parent[v] != -1:
            p = self.parent[v]
            if not (self.date[p] < t if open_cut else self.date[p] <= t):
                break
            v = p
        return v

    def euler(self):
        tin, tout = [0] * len(self.parent), [0] * len(self.parent)
        clock = 0
        stack = [(v, False) for v in reversed(self.roots)]
        while stack:
            v, leave = stack.pop()
            if leave:
                tout[v] = clock
            else:
                tin[v] = clock
                clock += 1
                stack.append((v, True))
                stack.extend((w, False) for w in reversed(self.children[v]))
        return tin, tout


def quotient(tree, by_double=False):
    """Contract connected equal-date plateaux, using iterative memoization."""
    representative = [None] * len(tree.parent)
    for start in range(len(tree.parent)):
        path, v = [], start
        while representative[v] is None:
            path.append(v)
            p = tree.parent[v]
            equal = (float(tree.date[p]) == float(tree.date[v]) if by_double else
                     tree.date[p] == tree.date[v]) if p != -1 else False
            if not equal:
                representative[v] = v
                break
            v = p
        r = representative[v]
        for w in path:
            representative[w] = r
    representatives = sorted(set(representative))
    index = {v: i for i, v in enumerate(representatives)}
    parent = [-1 if tree.parent[v] == -1 else index[representative[tree.parent[v]]]
              for v in representatives]
    result = Tree([tree.date[v] for v in representatives], parent,
                  unique_root=len(tree.roots) == 1)
    if not by_double:
        need(all(p == -1 or result.date[p] > result.date[v]
                 for v, p in enumerate(result.parent)), 'unquotiented plateau')
    return result, [index[v] for v in representative], representatives


def reduce_seeds(tree, seeds, mutation=None):
    q, mapping, representatives = quotient(tree, mutation == 'quotient_double')
    earliest = {}
    normalized = []
    for x, v, ell in seeds:
        need(type(x) is int and x >= 0, 'bad point')
        ell = F(ell)
        owner = tree.owner(v, ell)
        w = mapping[owner]
        need(q.owner(w, ell) == w, 'normalized owner not alive')
        normalized.append((x, w, ell))
        key = (x, w)
        if key not in earliest or (ell > earliest[key] if mutation == 'dedup_latest' else ell < earliest[key]):
            earliest[key] = ell
    tin, tout = q.euler()
    ordered = sorted(earliest, key=lambda key: (key[0], tin[key[1]]))
    out = []
    for i, (x, v) in enumerate(ordered):
        has_next = i + 1 < len(ordered) and ordered[i + 1][0] == x
        next_v = ordered[i + 1][1] if has_next else None
        descendant = has_next and tin[next_v] < tout[v]
        if mutation == 'inclusive_boundary':
            descendant = has_next and tin[next_v] <= tout[v]
        if mutation == 'drop_any_next':
            descendant = has_next
        if mutation == 'internal_to_leaves' and q.children[v]:
            continue
        if descendant:
            continue
        ell = earliest[x, v]
        if mutation == 'date_at_birth':
            ell = q.date[v]
        out.append((x, v, ell))
    need(len(out) <= len(earliest) <= len(seeds), 'size increased')
    return q, representatives, out, normalized


class NaiveRelation:
    """Literal parent lineages, independent of quotient/Euler and seed reduction.

    Cached lineage dates permit all cuts of a 20k chain without an O(H^2) replay.
    This is a test-oracle optimization, not an algorithmic claim for FULL.
    """
    def __init__(self, tree, seeds):
        self.rows = []
        self.paths = {}
        for x, v, ell in seeds:
            ell = F(ell)
            need(tree.date[v] <= ell, 'oracle seed before birth')
            if v not in self.paths:
                path = []
                w = v
                while w != -1:
                    path.append(w)
                    w = tree.parent[w]
                self.paths[v] = (tuple(tree.date[w] for w in path), tuple(path))
            self.rows.append((x, v, ell))

    def at(self, t):
        relation = set()
        for x, v, ell in self.rows:
            if ell <= t:
                dates, nodes = self.paths[v]
                relation.add((x, nodes[bisect_right(dates, t) - 1]))
        return relation


def all_cuts(tree, seeds):
    dates = sorted(set(tree.date) | {F(ell) for _, _, ell in seeds})
    return sorted(set(dates + [(a + b) / 2 for a, b in zip(dates, dates[1:])] +
                      [F(0), dates[-1] + 1]))


def mismatch(tree, seeds, mutation=None):
    q, reps, result, normalized = reduce_seeds(tree, seeds, mutation)
    original = NaiveRelation(tree, seeds)
    reduced = NaiveRelation(q, result)
    cuts = all_cuts(tree, seeds)
    for t in cuts:
        a = original.at(t)
        b = {(x, reps[v]) for x, v in reduced.at(t)}
        if a != b:
            return dict(cut=str(t), original=sorted(a), reduced=sorted(b)), len(cuts), len(result)
    return None, len(cuts), len(result)


def permute_case(tree, seeds, rng):
    order = list(range(len(tree.parent)))
    rng.shuffle(order)
    mapping = {old: new for new, old in enumerate(order)}
    parents = [-1 if tree.parent[old] == -1 else mapping[tree.parent[old]] for old in order]
    return Tree([tree.date[old] for old in order], parents), [(x, mapping[v], ell) for x, v, ell in seeds]


def abstract_cases():
    rng = random.Random(20260930)
    cases = []
    # Death at activation and repeated plateaux: normalization must close the plateau.
    cases.append(('closed_death_plateau', Tree([1, 1, 4, 4, 9], [2, 2, 3, 4, -1]),
                  [(0, 0, F(4)), (0, 1, F(4)), (0, 2, F(4)), (1, 4, F(10))]))
    # Internal node alone, after its birth: no migration to leaves or birth dating.
    cases.append(('internal_late_only', Tree([1, 2, 4, 9, 12], [2, 2, 3, 4, -1]),
                  [(0, 2, F(7)), (1, 3, F(10))]))
    cases.append(('duplicate_dates', Tree([1, 2, 9], [2, 2, -1]),
                  [(0, 0, F(2)), (0, 0, F(7)), (0, 0, F(2)), (0, 2, F(10))]))
    cases.append(('adjacent_siblings', Tree([1, 1, 8], [2, 2, -1]),
                  [(0, 0, F(1)), (0, 1, F(1))]))
    huge = F(2**40)
    cases.append(('exact_close_levels', Tree([huge, huge + F(1, 2**40)], [1, -1]),
                  [(0, 0, huge)]))
    for c in range(500):
        n = rng.randrange(2, 50)
        parent = [rng.randrange(v + 1, n) for v in range(n - 1)] + [-1]
        dates = [None] * n
        dates[-1] = F(rng.randrange(30, 60), 3)
        for v in reversed(range(n - 1)):
            dates[v] = max(F(0), dates[parent[v]] - F(rng.randrange(0, 8), 3))
        tree = Tree(dates, parent)
        seeds = []
        for _ in range(rng.randrange(1, 60)):
            v = rng.randrange(n)
            # Some raw nodes are dead at activation; normalize through ancestors.
            ell = dates[v] + F(rng.randrange(0, 14), 6)
            seeds.append((rng.randrange(4), v, ell))
        seeds += seeds[:rng.randrange(min(5, len(seeds)) + 1)]
        if c % 2:
            tree, seeds = permute_case(tree, seeds, rng)
        cases.append(('random_%d' % c, tree, seeds))
    n = 20000
    tree = Tree([F(v + 1, 3) for v in range(n)], list(range(1, n)) + [-1])
    cases.append(('chain_20000', tree, [(0, 0, F(1, 3)), (0, 10000, F(10002, 3)),
                                      (0, n - 1, F(n + 4, 3)), (1, n // 2, F(n + 2, 6))]))
    return cases


def export_cases():
    cases, summaries = [], []
    for path in sorted((ROOT / 'fixtures').glob('*.json')):
        d = json.loads(path.read_text())
        need(d['schema'] == 'mhgp10_frontier_export_v1', 'unexpected export schema')
        need(d['n_sites'] == d['n_points'] == len(d['sites']), 'not distinct-site export')
        k = d['K']
        order = [o for o in d['orders'] if o['k'] == k]
        need(len(order) == 1, 'order missing or duplicated')
        o = order[0]
        levels = [F(int(a), int(b)) for a, b in d['levels']]
        tree = Tree([levels[row[0]] for row in o['nodes']], [row[1] for row in o['nodes']])
        need(all(sorted(tree.children[v]) == sorted(row[3]) for v, row in enumerate(o['nodes'])),
             'native CSR and parent disagree')
        seeds = []
        strong_balls = interiors = shells = 0
        for b, ball in enumerate(d['balls']):
            I, U = ball['I'], ball['U']
            need(len(set(I + U)) == len(I) + len(U), 'duplicate native population')
            need(ball['p'] == len(I) and ball['u'] == len(U), 'native cardinalities')
            if ball['p'] + ball['q'] > k or len(I) + len(U) < k:
                continue
            need(2 <= ball['q'] <= 4 and len(I) <= k - 2, 'strong-ball premise')
            v, ell = o['ball_node'][b], levels[ball['lv']]
            need(v >= 0 and tree.owner(v, ell) == v, 'native ball_node not living')
            strong_balls += 1
            interiors += len(I)
            shells += len(U)
            seeds.extend((x, v, ell) for x in I + U)
        by_point = {}
        for x, v, _ in seeds:
            by_point.setdefault(x, set()).add(v)
        internal_only = [x for x, vs in by_point.items() if all(tree.children[v] for v in vs)]
        if path.stem in ('internal_k3', 'internal_k5'):
            need(bool(internal_only), 'required internal-only point absent')
        need(len(seeds) == interiors + shells and len(seeds) <= (k - 2) * strong_balls + shells,
             'payload bound')
        summaries.append(dict(file=path.name, K=k, nodes=len(tree.parent), strong_balls=strong_balls,
                              incidences=len(seeds), interiors=interiors, shells=shells,
                              internal_only_points=internal_only,
                              engine_commit=d['engine_commit']))
        cases.append((path.stem, tree, seeds))
    need(len(cases) == 6, 'native export floor')
    return cases, summaries


def mutation_cases(cases):
    wanted = [('dedup_latest', 'duplicate_dates'), ('date_at_birth', 'internal_late_only'),
              ('internal_to_leaves', 'internal_late_only'), ('inclusive_boundary', 'adjacent_siblings'),
              ('drop_any_next', 'adjacent_siblings'), ('quotient_double', 'exact_close_levels')]
    lookup = {name: (tree, seeds) for name, tree, seeds in cases}
    results = []
    for mutation, name in wanted:
        tree, seeds = lookup[name]
        try:
            witness, _, _ = mismatch(tree, seeds, mutation)
        except ValueError as error:
            need(mutation == 'quotient_double' and str(error) == 'seed before node birth',
                 'unexpected mutant failure: ' + str(error))
            results.append(dict(mutation=mutation, case=name, killed_by='living_owner_invariant',
                                witness=dict(error=str(error), distinct_levels=[str(t) for t in tree.date],
                                             same_double=float(tree.date[0]) == float(tree.date[1]))))
        else:
            need(witness is not None, 'mutation survived: ' + mutation)
            results.append(dict(mutation=mutation, case=name, killed_by='coverage_disagreement', witness=witness))
    # A forest remains a forest. The production contract requires one root: explicit rejection.
    forest = Tree([1, 1, 3, 4], [2, 3, -1, -1], unique_root=False)
    witness, _, _ = mismatch(forest, [(0, 0, F(1)), (0, 1, F(1)), (0, 2, F(4))])
    need(witness is None, 'forest coverage changed')
    refused = False
    try:
        Tree(forest.date, forest.parent)
    except ValueError as error:
        refused = str(error) == 'expected one root'
    need(refused, 'forest silently assigned a root')
    return results


def main():
    synthetic = abstract_cases()
    native, summaries = export_cases()
    counts = dict(cases=0, cuts=0, raw_incidences=0, reduced_incidences=0, nodes=0,
                  normalized_at_death=0, plateaux_removed=0)
    for name, tree, seeds in synthetic + native:
        witness, cuts, reduced = mismatch(tree, seeds)
        need(witness is None, 'coverage disagreement: ' + name + ' ' + str(witness))
        q, _, _, _ = reduce_seeds(tree, seeds)
        counts['cases'] += 1
        counts['cuts'] += cuts
        counts['raw_incidences'] += len(seeds)
        counts['reduced_incidences'] += reduced
        counts['nodes'] += len(tree.parent)
        counts['normalized_at_death'] += sum(tree.owner(v, ell) != v for _, v, ell in seeds)
        counts['plateaux_removed'] += len(tree.parent) - len(q.parent)
    mutants = mutation_cases(synthetic)
    print(json.dumps(dict(status='STRUCTURAL_FRACTION_PASS', counts=counts, native_exports=summaries,
                          causal_mutants=mutants, forest='coverage_preserved; unique-root mode rejects',
                          longest_chain=20000, native_invocations=0, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
