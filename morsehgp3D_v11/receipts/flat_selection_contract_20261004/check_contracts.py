#!/usr/bin/env python3
"""Bounded AST and Fraction witnesses only: no sklearn, native code, fit or cloud."""
import ast
from fractions import Fraction as F
import hashlib
import itertools
import json
from pathlib import Path
import subprocess

COMMIT = 'ab1a739d17f801823a66d74609209696152c8705'
REPO = '/workspaces/E-HGP/build/v11-claude-20261003'
EVALUATOR_SHA = '0da8fce4a4a4140f4e6be39c5bb5cb4ce1461e408019a76d81aafbbf5af58f70'
NARY_SHA = '7aa47f6c18dc57cdf8aaa3b3c12171b17cd7057e9a375f43af0bfd3c040e76d9'


class Guards:
    def __init__(self):
        self.count = 0

    def check(self, condition, reason):
        self.count += 1
        if not condition:
            raise RuntimeError(reason)


def extract(raw, wanted, namespace, filename):
    tree = ast.parse(raw.decode())
    nodes = [node for node in tree.body if isinstance(node, (ast.FunctionDef, ast.ClassDef)) and node.name in wanted]
    if {node.name for node in nodes} != wanted:
        raise RuntimeError('AST closure: ' + filename)
    exec(compile(ast.Module(body=nodes, type_ignores=[]), filename, 'exec'), namespace)
    return namespace


class TinyArray:
    def __init__(self, items):
        self.items = list(items)

    def tolist(self):
        return self.items[:]


def iou(a, b):
    return F(len(a & b), len(a | b))


def oracle_vs_partition(g):
    raw = subprocess.check_output(['git', '-C', REPO, 'show',
                                   COMMIT + ':morsehgp3D_v11/bench/points_hierarchy.py'])
    g.check(hashlib.sha256(raw).hexdigest() == EVALUATOR_SHA, 'Evaluator pin')
    ns = extract(raw, {'Evaluator'}, {}, 'published points_hierarchy.py')
    ev = ns['Evaluator'](2, TinyArray([0, 0, 0, 0, 1, 1]), TinyArray([False] * 6), 2)
    for site in (4, 5):
        ev.enter(site, 0)
    ev.level = F(1)
    ev.close()
    # At the closed second plateau, the four A sites enter the parent of branch B.
    ev.merge([0], 1)
    for site in range(4):
        ev.enter(site, 1)
    ev.level = F(2)
    ev.close()
    g.check(ev.best == [float(F(2, 3)), 1.0], 'actual best-per-object values')
    g.check(ev.best_ref == [(1, F(2)), (0, F(1))], 'actual incompatible ancestor/child references')
    truth = [frozenset(range(4)), frozenset((4, 5))]
    candidates = [frozenset((4, 5)), frozenset(range(6))]  # mcs=2
    choices = []
    for bits in itertools.product((False, True), repeat=2):
        chosen = [b for flag, b in zip(bits, candidates) if flag]
        if any(a & b for a, b in itertools.combinations(chosen, 2)):
            continue  # no simultaneous parent and child in a partition
        score = sum(max([iou(t, b) for b in chosen] or [F(0)]) for t in truth) / 2
        choices.append(dict(selected=[sorted(b) for b in chosen], mean_best_iou=str(score)))
    bound = max(F(row['mean_best_iou']) for row in choices)
    optimistic = (F(2, 3) + 1) / 2
    g.check(optimistic == F(5, 6) and bound == F(1, 2), 'optimistic score not achievable by an antichain')
    return dict(best_per_object=str(optimistic), max_consistent_antichain=str(bound),
                antichains=choices, scope='abstract laminar six-point state, not a geometric/native fixture')


def floating_eom(g):
    raw = (Path(__file__).parent / 'private_before/equite/nary_head.py').read_bytes()
    g.check(hashlib.sha256(raw).hexdigest() == NARY_SHA, 'private Nary pin')
    wanted = {'Dendrogram', 'Cluster', 'condense', 'phi_power', 'stability', 'descendants', 'select'}
    ns = extract(raw, wanted, {'INF': float('inf')}, 'frozen private nary_head.py')
    radius = F(3, 2) - F(1, 1 << 70)
    squared = radius * radius
    g.check(squared.numerator.bit_length() <= 192 and squared.denominator.bit_length() <= 192,
            'scalar squared level fits the 192-bit export fields')
    g.check(float(radius) == 1.5, 'rational distinct radius collapses in binary64')
    d = ns['Dendrogram'](8)
    left = d.add([0, 1], float(radius), 1)
    right = d.add([2, 3], float(radius), 1)
    parent = d.add([left, right], 2.0, 2)
    unrelated = d.add([4, 5, 6, 7], 0.5, 0)
    d.add([parent, unrelated], 3.0, 3)
    clusters = ns['condense'](d, 2)
    scores = ns['stability'](clusters, ns['phi_power'](1))
    chosen = ns['select'](clusters, scores)
    pcluster = next(i for i, c in enumerate(clusters) if c.node == parent)
    children = clusters[pcluster].children
    exact_parent = 4 * (F(1, 2) - F(1, 3))
    exact_children = 4 * (1 / radius - F(1, 2))
    g.check(exact_children > exact_parent, 'exact EOM children win strictly')
    g.check(pcluster in chosen and all(c not in chosen for c in children), 'actual float EOM retains parent')
    return dict(radius=str(radius), squared_radius=str(squared), exact_parent=str(exact_parent),
                exact_children=str(exact_children), exact_gap=str(exact_children - exact_parent),
                float_parent=scores[pcluster], float_children=sum(scores[c] for c in children),
                actual_selected=sorted(chosen), exact_preference='children',
                scope='private floating head counterexample, no integer-coordinate Cloud realization claimed')


def diagonal_scope(g):
    entries = [F(1, 2), F(1), F(3, 2)]
    pairs = {(0, 1): F(1), (0, 2): F(2), (1, 2): F(2)}
    g.check(all(value >= max(entries[i], entries[j]) for (i, j), value in pairs.items()), 'valid delayed diagonals')
    ghosts_seen = False
    for radius in (F(1, 4), F(1, 2), F(1), F(3, 2), F(2)):
        parent = list(range(3))
        def find(i):
            while parent[i] != i:
                i = parent[i]
            return i
        for (i, j), value in pairs.items():
            if value <= radius:
                parent[find(i)] = find(j)
        groups = {}
        for i in range(3):
            groups.setdefault(find(i), set()).add(i)
        raw = set(frozenset(s) for s in groups.values())
        active = set(frozenset(i for i in s if entries[i] <= radius) for s in groups.values()) - {frozenset()}
        g.check({s for s in raw if len(s) >= 2} == {s for s in active if len(s) >= 2},
                'mcs>=2 can ignore inactive singleton diagonals')
        ghosts_seen |= raw != active
    g.check(ghosts_seen, 'mcs=1 does require activation dates')
    return dict(mcs_at_least_two='nontrivial blocks identical', mcs_one='inactive singleton ghosts differ')


def main():
    g = Guards()
    result = dict(status='PASS', oracle_partition=oracle_vs_partition(g), floating_eom=floating_eom(g),
                  diagonal_scope=diagonal_scope(g), native_runs=0, fits=0, gcp_actions=0)
    result['guards'] = g.count
    print(json.dumps(result, sort_keys=True, indent=1))


if __name__ == '__main__':
    main()
