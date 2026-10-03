#!/usr/bin/env python3
"""Small exact projection review; no native code, no product modifications."""
from fractions import Fraction
from itertools import product
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / 'source'))
from hgp11_ref import Definition, Reference, judge  # noqa: E402
from hgp11_ref.families import fixtures  # noqa: E402

CHECKS = 0


def check(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise RuntimeError(message)


def labels(indices):
    return ''.join(chr(65 + i) for i in sorted(indices))


def parents(result):
    parent = [None] * len(result.nodes)
    for v, node in enumerate(result.nodes):
        for child in node.children:
            check(parent[child] is None, 'multiple parents')
            parent[child] = v
    return parent


def path(parent, node):
    answer = []
    while node is not None:
        check(node not in answer, 'cycle')
        answer.append(node)
        node = parent[node]
    return answer


def lca(parent, nodes):
    paths = [path(parent, v) for v in sorted(nodes)]
    common = set(paths[0]).intersection(*map(set, paths[1:]))
    return next(v for v in paths[0] if v in common)


def descendants(result, attachment):
    parent = parents(result)
    groups = [set() for _ in result.nodes]
    dates = [Fraction(0) for _ in result.nodes]
    for x, (date, node) in enumerate(attachment):
        check(result.nodes[node].level <= date, 'attachment before node birth')
        check(parent[node] is None or date < result.nodes[parent[node]].level,
              'attachment into dead node')
        for v in path(parent, node):
            groups[v].add(x)
            dates[v] = max(dates[v], date, result.nodes[v].level)
    answer = []
    for v, group in enumerate(groups):
        if not group:
            continue
        end = None if parent[v] is None else result.nodes[parent[v]].level
        check(end is None or dates[v] < end, 'group has no nonempty lifetime')
        answer.append(dict(node=v, group=labels(group), start=str(dates[v]),
                           end=None if end is None else str(end)))
    return answer


def project(result, mode):
    parent = parents(result)
    if mode == 'core':
        attachment = [(e.level, e.nodes) for e in result.core]
    elif mode == 'first_cover_lca':
        attachment = []
        for e in result.cover:
            node = lca(parent, e.nodes)
            attachment.append((max(e.level, result.nodes[node].level), node))
    else:
        attachment = [(e.level, min(e.nodes)) for e in result.cover]
    return descendants(result, attachment)


def all_singleton_choices(result):
    total = hit_both = 0
    for choices in product(*(sorted(e.nodes) for e in result.cover)):
        groups = descendants(result, [(e.level, v) for e, v in zip(result.cover, choices)])
        total += 1
        hit_both += {'ABC', 'DEF'} <= {g['group'] for g in groups}
    return dict(choices=total, both_triangles=hit_both)


def main():
    cases = {c.name: c for c in fixtures()}
    names = ('two_triangles', 'two_triangles_1998', 'two_triangles_1700')
    output = dict(scope='Python exact reference only, no native qualification', cases=[])
    for name in names:
        points = cases[name].points
        a, b = Definition(points), Reference(points, 6)
        distances = {(i, j): sum((x - y) ** 2 for x, y in zip(points[i], points[j]))
                     for i in range(6) for j in range(i + 1, 6)}
        check(distances[(0, 1)] == 4000000, 'vertical edge')
        check(distances[(0, 2)] == distances[(1, 2)] == 3999824, 'oblique edges')
        check(distances[(0, 1)] != distances[(0, 2)], 'not exactly equilateral')
        row = dict(name=name, squared_distances={labels(key): value for key, value in distances.items()}, orders=[])
        previous = None
        for k in range(1, 7):
            result, other = a.order(k), b.order(k)
            check(not judge.compare_orders(result, other), 'A/B difference')
            check(judge.coherence(result, 6, previous) is None, 'reference invariant')
            previous = result
            groups = {mode: project(result, mode) for mode in ('core', 'first_cover_lca', 'first_cover_smallest_node')}
            cuts = {}
            for radius in (1300, 1700):
                _opened, closed = judge.cut_at(result, Fraction(radius * radius))
                cuts[str(radius)] = [dict(node=v, cover=labels(i for i in range(6) if cov & (1 << i)),
                                         core=labels(i for i in range(6) if cor & (1 << i)))
                                       for v, cov, cor in closed]
            row['orders'].append(dict(k=k, core=[dict(date=str(e.level), node=e.nodes) for e in result.core],
                                      first_cover=[dict(date=str(e.level), nodes=sorted(e.nodes)) for e in result.cover],
                                      groups=groups, cuts=cuts,
                                      singleton_choices=all_singleton_choices(result)))
        output['cases'].append(row)
    for case in output['cases']:
        order2 = case['orders'][1]
        check([c['cover'] for c in order2['cuts']['1300']] == ['CD', 'ABC', 'DEF'] or
              sorted(c['cover'] for c in order2['cuts']['1300']) == ['ABC', 'CD', 'DEF'], 'FULL target present')
        core_groups = {g['group'] for g in order2['groups']['core']}
        check(not {'ABC', 'DEF'} & core_groups, 'core misses both triangles at k2')
        check(core_groups == ({'CD', 'ABCDEF'} if case['name'].endswith('1700') else {'ABCDEF'}),
              'core group inventory')
        if case['name'] != 'two_triangles':
            check(order2['singleton_choices'] == dict(choices=1, both_triangles=0),
                  'short bridge: unique first-cover attachments lose targets')
            lca_groups = {g['group'] for g in order2['groups']['first_cover_lca']}
            check({'AB', 'CD', 'EF', 'ABCDEF'} <= lca_groups and
                  not {'ABC', 'DEF'} & lca_groups, 'short bridge LCA groups')
        else:
            check(order2['singleton_choices'] == dict(choices=4, both_triangles=4),
                  'long bridge near-equilateral profile unexpectedly loses targets')
        order3 = case['orders'][2]
        for radius in ('1300', '1700'):
            check(sorted(c['cover'] for c in order3['cuts'][radius]) == ['ABC', 'DEF'],
                  'k3 dynamic cover preserves target in requested radius window')
            check(all(not c['core'] for c in order3['cuts'][radius]),
                  'k3 core still absent in requested radius window')
        check({'ABC', 'DEF'} <= {g['group'] for g in order3['groups']['first_cover_lca']},
              'k3 first cover recovers triangles')
    pair_points = [(x, 0, 0) for x in (0, 2, 100, 102)]
    a, b = Definition(pair_points), Reference(pair_points, 3)
    pair_orders = []
    for k in (2, 3):
        result = a.order(k)
        check(not judge.compare_orders(result, b.order(k)), 'pair A/B difference')
        groups = {mode: project(result, mode) for mode in ('core', 'first_cover_lca')}
        expected = {'AB', 'CD'} if k == 2 else set()
        for mode, family in groups.items():
            check({g['group'] for g in family} & {'AB', 'CD'} == expected,
                  'fixed k3 loses two-point groups for core/LCA')
        pair_orders.append(dict(k=k, nodes=[dict(level=str(n.level), children=list(n.children)) for n in result.nodes],
                                first_cover=[dict(date=str(e.level), nodes=sorted(e.nodes)) for e in result.cover],
                                groups=groups))
    output['fixed_order_counterexample'] = dict(points=pair_points, orders=pair_orders,
                                               scope='Counterexample to these two projections; not an impossibility theorem for all rules')
    output['checks'] = CHECKS
    print(json.dumps(output, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
