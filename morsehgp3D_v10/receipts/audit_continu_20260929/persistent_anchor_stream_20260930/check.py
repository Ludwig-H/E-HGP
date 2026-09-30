#!/usr/bin/env python3
"""Exact abstract tree lemma; rational RADII, not squared native levels."""
from fractions import Fraction as F
from itertools import combinations
import hashlib
import json
import random


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


class Tree:
    def __init__(self, parent, birth):
        self.parent, self.birth = list(parent), list(map(F, birth))
        require(len(parent) == len(birth), 'tree sizes')
        roots = [i for i, p in enumerate(parent) if p < 0]
        require(len(roots) == 1, 'one root')
        self.root = roots[0]
        self.paths = []
        for v in range(len(parent)):
            path, seen, u = [], set(), v
            while u >= 0:
                require(u not in seen, 'cycle')
                seen.add(u)
                path.append(u)
                p = parent[u]
                if p >= 0:
                    require(0 <= p < len(parent), 'parent range')
                    require(self.birth[u] < self.birth[p], 'strict contracted births')
                u = p
            require(path[-1] == self.root, 'same root')
            self.paths.append(path)
        self.depth = [len(path) - 1 for path in self.paths]

    def alive(self, v, c):
        p = self.parent[v]
        return self.birth[v] <= c and (p < 0 or c < self.birth[p])

    def anc(self, v, r, closed=True):
        require(self.birth[v] <= r, 'unborn owner')
        while self.parent[v] >= 0:
            p = self.parent[v]
            if self.birth[p] < r or (closed and self.birth[p] == r):
                v = p
            else:
                break
        return v

    def lca_stream(self, a, b):
        while a != b:
            if self.depth[a] > self.depth[b]:
                a = self.parent[a]
            elif self.depth[b] > self.depth[a]:
                b = self.parent[b]
            else:
                a, b = self.parent[a], self.parent[b]
        return a

    def lca_oracle(self, nodes):
        nodes = list(nodes)
        require(nodes, 'empty LCA')
        common = set(self.paths[nodes[0]])
        for v in nodes[1:]:
            common.intersection_update(self.paths[v])
        return max(common, key=lambda v: self.depth[v])


def covered(tree, witnesses, r, closed=True):
    return {tree.anc(v, r, closed) for c, v in witnesses
            if c < r or (closed and c == r)}


def resolution_direct(tree, comps, r):
    require(comps, 'empty resolution')
    for s in sorted({r} | {b for b in tree.birth if b >= r}):
        if len({tree.anc(v, s) for v in comps}) == 1:
            return s
    raise RuntimeError('root did not resolve')


def antichain(tree, witnesses):
    first = {}
    for c, v in witnesses:
        first[v] = min(first.get(v, c), c)
    leaves = [(c, v) for v, c in first.items()
              if not any(w != v and v in tree.paths[w] for w in first)]
    require(leaves, 'no minimal witness')
    return sorted(leaves)


def anchored_oracle(tree, witnesses, kappa):
    leaves = antichain(tree, witnesses)
    alpha = min(c for c, _v in leaves)
    t = alpha
    for c in sorted({c for c, _v in leaves}):
        comps = covered(tree, leaves, c)
        m = resolution_direct(tree, comps, c)
        t = max(t, m - kappa * (c - alpha))
    first = tree.lca_oracle(v for c, v in leaves if c == alpha)
    return alpha, t, tree.anc(first, t), first


def stream(tree, witnesses, kappa, cutoff=None, guard=True, final_owner=False):
    require(witnesses, 'empty stream')
    if guard:
        require(all(witnesses[i][0] <= witnesses[i + 1][0]
                    for i in range(len(witnesses) - 1)), 'unordered stream')
    alpha, t = witnesses[0][0], witnesses[0][0]
    j, jfirst, i = None, None, 0
    while i < len(witnesses):
        c = witnesses[i][0]
        if cutoff is not None and c > cutoff:
            break
        while i < len(witnesses) and witnesses[i][0] == c:
            v = witnesses[i][1]
            j = v if j is None else tree.lca_stream(j, v)
            i += 1
        if c == alpha:
            jfirst = j
        t = max(t, tree.birth[j] - kappa * (c - alpha))
    require(jfirst is not None, 'first cohort absent')
    chosen = j if final_owner else jfirst
    return alpha, t, tree.anc(chosen, t), chosen


def partition(tree, answers, r, closed):
    groups = {}
    for x, (_alpha, t, owner, _first) in enumerate(answers):
        label = ('s', x) if t > r or (not closed and t == r) else ('v', tree.anc(owner, r, closed))
        groups.setdefault(label, []).append(x)
    return sorted(sorted(g) for g in groups.values())


def height(tree, a, b, oracle=False):
    lca = tree.lca_oracle([a[2], b[2]]) if oracle else tree.lca_stream(a[2], b[2])
    return max(a[1], b[1], tree.birth[lca])


def make_trees():
    return [
        Tree([4, 4, 5, 5, 6, 6, -1], [10, 11, 12, 13, 14, 15, 18]),
        Tree([5, 5, 5, 5, 5, -1], [10, 10, 11, 12, 12, 17]),
        Tree([1, 2, 3, 4, -1], [10, 12, 14, 16, 18]),
        Tree([3, 4, 4, 5, 5, -1], [10, 11, 12, 13, 15, 18]),
        Tree([3, 3, 3, 5, 5, -1], [10, 11, 12, 14, 13, 18]),
        Tree([4, 4, 5, 5, 6, 6, 7, -1], [10, 10, 11, 11, 13, 14, 16, 18]),
        Tree([4, 4, 4, 4, -1], [10, 10, 10, 10, 18]),
        Tree([-1], [10]),
    ]


def witnesses_random(tree, rng):
    out = []
    for v, b in enumerate(tree.birth):
        if rng.randrange(3) == 0:
            continue
        p = tree.parent[v]
        delta = F(1) if p < 0 else tree.birth[p] - b
        c = b + delta * F(rng.randrange(3), 3)
        out.append((c, v))
        if rng.randrange(4) == 0:
            out.append((c, v))
        if rng.randrange(5) == 0:
            out.append((b + delta * F(2, 3), v))
    if not out:
        out = [(tree.birth[tree.root], tree.root)]
    return sorted(out)


def main():
    rng = random.Random(9302026)
    trees, kappas = make_trees(), [F(1), F(3, 2), F(2), F(4)]
    counts = dict(cases=0, points=0, witnesses=0, cov_cuts=0, resolution_cuts=0,
                  answers=0, cutoff_answers=0, partitions=0, pair_heights=0,
                  duplicate_invariance=0, internal_first=0, exact_cohorts=0)
    records = hashlib.sha256()
    for case in range(256):
        tree = trees[case % len(trees)]
        profiles = [witnesses_random(tree, rng) for _x in range(4)]
        if case == 0:
            profiles = [[(F(10), 0), (F(18), 6), (F(18), 6)],
                        [(F(10), 0), (F(12), 2), (F(12), 2), (F(18), 6)],
                        [(F(14), 4), (F(18), 6)],
                        [(F(13), 3), (F(15), 5), (F(18), 6)]]
        counts['cases'] += 1
        all_answers = []
        for ws in profiles:
            require(all(tree.alive(v, c) for c, v in ws), 'invalid witness date')
            leaves = antichain(tree, ws)
            alpha = min(c for c, _v in ws)
            counts['points'] += 1
            counts['witnesses'] += len(ws)
            if sum(c == alpha for c, _v in ws) > 1:
                counts['exact_cohorts'] += 1
            first_nodes = [v for c, v in leaves if c == alpha]
            counts['internal_first'] += any(v in tree.parent for v in first_nodes)
            cuts = sorted({F(0), alpha} | set(tree.birth) | {c for c, _v in ws})
            for r in cuts:
                for closed in (False, True):
                    comps = covered(tree, ws, r, closed)
                    require(comps == covered(tree, leaves, r, closed), 'antichain changed coverage')
                    counts['cov_cuts'] += 1
                if r >= alpha:
                    comps = covered(tree, ws, r)
                    j = tree.lca_oracle(v for c, v in ws if c <= r)
                    require(resolution_direct(tree, comps, r) == max(r, tree.birth[j]),
                            'prefix LCA changed resolution')
                    counts['resolution_cuts'] += 1
            delays = [resolution_direct(tree, covered(tree, ws, c), c) - c
                      for c in {c for c, _v in ws}]
            rho = max(alpha, 2 * max(delays))
            require(alpha <= rho <= 2 * alpha, 'synthetic rho bound')
            by_k = []
            for kappa in kappas:
                expect = anchored_oracle(tree, ws, kappa)
                got = stream(tree, ws, kappa)
                require(got[:3] == expect[:3], 'stream/antichain answer')
                require(tree.alive(got[2], got[1]), 'owner not alive')
                require(got[2] in covered(tree, ws, got[1]), 'owner does not cover')
                counts['answers'] += 1
                unique = sorted(set(ws))
                require(stream(tree, unique, kappa)[:3] == got[:3], 'duplicates changed result')
                counts['duplicate_invariance'] += 1
                if kappa > 1:
                    cutoff = alpha + rho / (2 * (kappa - 1))
                    require(cutoff <= kappa * alpha / (kappa - 1), 'fine cutoff bound')
                    require(stream(tree, ws, kappa, cutoff)[:3] == got[:3], 'cutoff changed result')
                    counts['cutoff_answers'] += 1
                by_k.append((got, expect))
            all_answers.append(by_k)
        for ki, kappa in enumerate(kappas):
            got = [by_k[ki][0] for by_k in all_answers]
            expected = [by_k[ki][1] for by_k in all_answers]
            cuts = sorted({F(0)} | set(tree.birth) | {c for ws in profiles for c, _v in ws}
                          | {a[1] for a in got})
            cuts.append(cuts[-1] + 1)
            previous = None
            for r in cuts:
                for closed in (False, True):
                    p = partition(tree, got, r, closed)
                    require(p == partition(tree, expected, r, closed), 'partition mismatch')
                    if previous is not None:
                        labels = {x: i for i, block in enumerate(p) for x in block}
                        require(all(len({labels[x] for x in block}) == 1 for block in previous), 'split')
                    previous = p
                    counts['partitions'] += 1
            for a, b in combinations(range(4), 2):
                require(height(tree, got[a], got[b]) == height(tree, expected[a], expected[b], True),
                        'pair height mismatch')
                counts['pair_heights'] += 1
            records.update(json.dumps({'case': case, 'kappa': str(kappa),
                                       'answers': [[str(v) for v in a] for a in got]},
                                      sort_keys=True, separators=(',', ':')).encode())

    tree = trees[0]
    fixture = [(F(10), 0), (F(18), 6), (F(18), 6)]
    expected = anchored_oracle(tree, fixture, F(4))
    reverse = list(reversed(fixture))
    refused = False
    try:
        stream(tree, reverse, F(4))
    except RuntimeError as exc:
        refused = str(exc) == 'unordered stream'
    require(refused, 'reverse not refused')
    mutant_reverse = stream(tree, reverse, F(4), guard=False)
    require(mutant_reverse[:3] != expected[:3], 'unguarded reverse mutant survived')
    final_refused = False
    try:
        stream(tree, fixture, F(4), final_owner=True)
    except RuntimeError as exc:
        final_refused = str(exc) == 'unborn owner'
    require(final_refused, 'final owner mutant survived')
    print(json.dumps({'schema': 'mhgp10.pkappa_stream.abstract.v1', 'status': 'PASS',
                      'scope': 'abstract_monotone_tree_only_rational_radii',
                      'counts': counts, 'record_sha256': records.hexdigest(),
                      'mutants': {'reverse_guard_refused': True,
                                  'unguarded_reverse': {'expected': list(map(str, expected[:3])),
                                                        'observed': list(map(str, mutant_reverse[:3]))},
                                  'final_owner_refused_unborn': True},
                      'engine_calls': 0, 'gcp': False}, sort_keys=True))


if __name__ == '__main__':
    main()
