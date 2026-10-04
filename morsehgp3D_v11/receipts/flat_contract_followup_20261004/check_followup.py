#!/usr/bin/env python3
"""New bounded API checks only; frozen AST, no sklearn/native/FULL/fit."""
import ast
from fractions import Fraction as F
import hashlib
import json
import math
from pathlib import Path
from types import SimpleNamespace as NS

import numpy as np


def checked_ast(path, expected, names, ns):
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != expected:
        raise RuntimeError('source hash changed: ' + str(path))
    tree = ast.parse(raw.decode())
    nodes = [x for x in tree.body if isinstance(x, (ast.FunctionDef, ast.ClassDef)) and x.name in names]
    if {x.name for x in nodes} != names:
        raise RuntimeError('AST closure mismatch')
    exec(compile(ast.Module(body=nodes, type_ignores=[]), str(path), 'exec'), ns)
    return ns


class Guards:
    def __init__(self):
        self.count = 0

    def check(self, condition, reason):
        self.count += 1
        if not condition:
            raise RuntimeError(reason)


class RationalRad:
    @staticmethod
    def sqrt(f):
        n, d = math.isqrt(f.numerator), math.isqrt(f.denominator)
        if n * n != f.numerator or d * d != f.denominator:
            raise RuntimeError('the witness uses rational square roots only')
        return F(n, d)


def completion(g, base):
    ns = checked_ast(base / 'sources_before/modele/scripts/modele_lib.py',
                     '5c0f0f1bd40beb7d156ffe9b45e31ad3fa3bac82bf41c518e6c2db07da193b9d',
                     {'popcount', 'rcmp', 'chain_nodes', 'lowest_qualified_nodes', 'complete_lineage'},
                     {'Fraction': F, 'Rad': RationalRad})
    births, parents = [F(1), F(1), F(2)], [2, 2, -1]
    def alive(v, r):
        while parents[v] >= 0 and births[parents[v]] <= r:
            v = parents[v]
        return v
    # Abstract closed-coverage state: both branches first qualify the boundary point0 at radius1.
    # Primary A has three other engaged points; B has two. At mcs3 only A is admissible below the parent.
    ph = NS(n=6, m=3, owner=[2, 0, 0, 0, 1, 1], birth=births, alive_at=alive,
            res=NS(cuts=[NS(level=F(1), closed=[(0, 15, 0), (1, 49, 0)]),
                         NS(level=F(4), closed=[(2, 63, 0)])]))
    tg = NS(radii=[F(1), F(2)])
    clusters = [{}, {'birth': 0, 'death': 1, 'join': {1: 0, 2: 0, 3: 0}, 'members': [1, 2, 3]}]
    labels = [-1, 0, 0, 0, -1, -1]
    t, starts = ns['lowest_qualified_nodes'](ph, 0)
    g.check(t == 1 and starts == [0, 1], 'all first-qualified ties retained')
    # P1 has delay meet-q=2-1 and the closed owner is the parent.
    entry = F(1) + F(2) - F(1)
    g.check(entry == 2 and alive(0, entry) == 2 and alive(1, entry) == 2, 'persistent owner at exact closed plateau')
    got = ns['complete_lineage'](ph, tg, clusters, [1], labels)
    g.check(got == [0, 0, 0, 0, -1, -1], 'actual first-qualified-lineage completion')
    g.check(tg.radii[clusters[1]['death']] <= entry and ph.owner[0] == 2,
            'selected branch is not alive when the retained-owner lineage starts')
    g.check(labels == [-1, 0, 0, 0, -1, -1], 'input labels remain unchanged')
    return dict(input_labels=labels, completed_labels=got, first_qualified_nodes=starts,
                persistent_entry=str(entry), persistent_owner=2, selected_branch_lifetime='[1,2)',
                interpretation='a valid separate flat completion arm, not the retained P1 owner lineage',
                scope='abstract hierarchy/coverage state only; no integer Cloud realization claimed')


def root_threshold(g, base):
    names = {'Dendrogram', 'Cluster', 'condense', 'phi_power', 'stability', 'descendants', 'select',
             'epsilon_search', 'traverse_upwards', 'labels', 'head'}
    ns = checked_ast(base / 'sources_before/equite/nary_head.py',
                     'cbf4be1d8bc5516d20814fd38b94542b4ba8f68285fe430d7065d4b1d0bfbdc1',
                     names, {'np': np, 'INF': float('inf')})
    d = ns['Dendrogram'](2)
    d.add([0, 1], 1.0, 0)
    ordinary = ns['head'](d, 3, asc=False).tolist()
    admitted_root = ns['head'](d, 3, asc=True).tolist()
    g.check(ordinary == [-1, -1], 'default excluded-root policy returns all noise for n<mcs')
    g.check(admitted_root == [0, 0], 'allow-root currently admits two points below mcs3')
    g.check(sum(x >= 0 for x in admitted_root) < 3, 'root exception bypasses the declared minimum size')
    return dict(n=2, mcs=3, default_labels=ordinary, allow_root_labels=admitted_root,
                interpretation='declare a root exception or require n>=mcs / return noise before admission',
                scope='private Python head only; no claim about sklearn public fit or native v11')


def main():
    g, base = Guards(), Path(__file__).resolve().parent
    result = dict(status='PASS', completion=completion(g, base), root_threshold=root_threshold(g, base),
                  fits=0, native_runs=0, gcp_actions=0)
    result['guards'] = g.count
    print(json.dumps(result, sort_keys=True, indent=1))


if __name__ == '__main__':
    main()
