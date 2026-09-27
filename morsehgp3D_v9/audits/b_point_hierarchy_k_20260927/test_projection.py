#!/usr/bin/env python3
"""Independent exact-cut tests of the fixed-K point projection.

No cloud, baseline library or geometry engine mutation. The native test uses
only tiny temporary u32le clouds; --native selects an already closed binary.
Checks raise explicitly and therefore also execute under python -O.
"""
from __future__ import annotations

import argparse
import copy
from fractions import Fraction
import itertools
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from projection import SourceTree


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def exact(value):
    return Fraction(int(value['num']), int(value['den']))


def encoded(value):
    value = Fraction(value)
    return dict(num=str(value.numerator), den=str(value.denominator))


def partition(groups):
    return tuple(sorted(tuple(sorted(group)) for group in groups if group))


def fixture():
    children = [[], [], [], [0, 1], [3, 2]]
    parents = [3, 3, 4, 4, None]
    levels = [1, 1, 1, 9, 25]
    # Point 2 enters both sibling branches symmetrically at 1: attach at LCA
    # 3 at 9. Point 5 enters branch 0 only at 4, not retroactively at birth 1.
    records = [(0, 1, [0, 1, 2]), (1, 1, [2, 3]), (2, 1, [4]),
               (0, 4, [5]), (1, 5, [5]), (3, 16, [0]), (3, 16, [4])]
    return dict(schema='mhgp9_fixed_k_export_v1', status='completed', point_count=6, k=2,
                nodes=[dict(id=i, level=encoded(at), children=children[i], successor=parents[i])
                       for i, at in enumerate(levels)], roots=[4],
                populations=[dict(interior=points, shell=[]) for _, _, points in records],
                contributions=[dict(level=encoded(at), segment=node, population=i,
                                    shell_mask=0, include_interior=True)
                               for i, (node, at, _) in enumerate(records)])


def naïve_first_anchors(data):
    """Raw record scan and explicit ancestor lists; no SourceTree helpers."""
    parent = [node['successor'] for node in data['nodes']]
    births = [exact(node['level']) for node in data['nodes']]
    events = [[] for _ in range(data['point_count'])]
    for c in data['contributions']:
        row = data['populations'][c['population']]
        ids = list(row['interior']) if c['include_interior'] else []
        ids += [p for j, p in enumerate(row['shell']) if c['shell_mask'] & (1 << j)]
        for point in ids:
            events[point].append((exact(c['level']), c['segment']))
    result = []
    for records in events:
        first = min(at for at, _ in records)
        branches = {node for at, node in records if at == first}
        chains = []
        for node in branches:
            chain = []
            while node is not None:
                chain.append(node)
                node = parent[node]
            chains.append(chain)
        common = set(chains[0]).intersection(*map(set, chains[1:]))
        node = next(node for node in chains[0] if node in common)
        result.append((node, max(first, births[node])))
    return result


def naïve_cut(data, anchors, cut, closed):
    """Each active point follows source parents; inactive points are singletons."""
    admits = (lambda x: x <= cut) if closed else (lambda x: x < cut)
    nodes, groups = data['nodes'], {}
    for point, (node, activation) in enumerate(anchors):
        if not admits(activation):
            key = ('unattached', point)
        else:
            while nodes[node]['successor'] is not None and admits(exact(nodes[nodes[node]['successor']]['level'])):
                node = nodes[node]['successor']
            key = ('source', node)
        groups.setdefault(key, []).append(point)
    return partition(groups.values())


def point_tree_cut(tree, cut, closed):
    children = tree['children']
    at = {node: exact(value) for node, value in tree['squared_levels'].items()}
    admits = (lambda x: x <= cut) if closed else (lambda x: x < cut)

    def leaves(root):
        pending, result = [root], []
        while pending:
            node = pending.pop()
            if node in children:
                pending.extend(children[node])
            else:
                result.append(node)
        return result

    pending, groups = [tree['root']], []
    while pending:
        node = pending.pop()
        if node not in children:
            groups.append([node])
        elif admits(at[node]):
            groups.append(leaves(node))
        else:
            pending.extend(children[node])
    return partition(groups)


def verify_graft(data, anchors, tree):
    levels = {Fraction(0), *(exact(node['level']) for node in data['nodes']), *(at for _, at in anchors)}
    ordered = sorted(levels)
    levels.update((a+b)/2 for a, b in zip(ordered, ordered[1:]))
    levels.add(max(levels)+1)
    previous = None
    checks = 0
    for cut in sorted(levels):
        for closed in (False, True):
            actual = point_tree_cut(tree, cut, closed)
            expected = naïve_cut(data, anchors, cut, closed)
            need(actual == expected, f'naive/source cut mismatch at {cut}, closed={closed}: {actual} != {expected}')
            need(sorted(itertools.chain.from_iterable(actual)) == list(range(data['point_count'])), 'cut is not a partition')
            if previous is not None:
                need(all(any(set(old) <= set(new) for new in actual) for old in previous), 'cuts are not nested')
            previous = actual
            checks += 1
    return checks


def pure_tests():
    data = fixture()
    source = SourceTree(data)
    anchors, stats = source.first_coverage()
    need(anchors == naïve_first_anchors(data), 'first coverage differs from raw record oracle')
    need(anchors[2] == (3, Fraction(9)) and stats['delayed_until_lca'] == 1, 'symmetric shared point did not stay at LCA')
    need(anchors[5] == (0, Fraction(4)), 'continuation date lost')
    checks = verify_graft(data, anchors, source.graft(anchors))
    tree = source.graft(anchors)
    need((0, 1) in point_tree_cut(tree, Fraction(4), False), 'point 5 admitted before continuation')
    need((0, 1, 5) in point_tree_cut(tree, Fraction(4), True), 'point 5 not admitted at continuation')

    # All valid placements along all source segments, including empty branches,
    # unary contractions, dates between births, and points entering at a merge.
    placements = [(u, exact(node['level'])) for u, node in enumerate(data['nodes'])]
    placements += [(0, Fraction(4)), (1, Fraction(7)), (2, Fraction(17)), (3, Fraction(16)), (4, Fraction(30))]
    rng = random.Random(20260927)
    for _ in range(100):
        placed = [rng.choice(placements) for _ in range(data['point_count'])]
        checks += verify_graft(data, placed, source.graft(placed))

    methods = [('first_coverage', 1), ('first_coverage', 2), ('entry_vote', 1), ('entry_vote', 2)]
    for method, z in methods:
        original = source.project(method, z)['tree']
        need(original['anchors'][2] == dict(source_node=3, squared_activation=encoded(9)),
             'symmetric vote or first appearance broke LCA tie')
        # Repeated records and duplicate population descriptions must be set-idempotent.
        reordered = copy.deepcopy(data)
        reordered['contributions'] += copy.deepcopy(reordered['contributions'])
        first = copy.deepcopy(reordered['contributions'][0])
        reordered['populations'].append(copy.deepcopy(reordered['populations'][first['population']]))
        first['population'] = len(reordered['populations'])-1
        reordered['contributions'].append(first)
        rng.shuffle(reordered['contributions'])
        changed = SourceTree(reordered).project(method, z)['tree']
        need(changed == original, 'duplicate/reordered set contributions changed point tree')
        selected = [(a['source_node'], exact(a['squared_activation'])) for a in original['anchors']]
        checks += verify_graft(data, selected, original)

        # Bijection of PointId must only rename membership, never break a tie.
        permutation = [3, 5, 0, 4, 2, 1]
        permuted = copy.deepcopy(data)
        for population in permuted['populations']:
            population['interior'] = sorted(permutation[x] for x in population['interior'])
        renamed = SourceTree(permuted).project(method, z)['tree']
        for cut in map(Fraction, (0, 1, 4, 5, 9, 16, 25, 30)):
            for closed in (False, True):
                expected = partition([[permutation[x] for x in group] for group in point_tree_cut(original, cut, closed)])
                need(point_tree_cut(renamed, cut, closed) == expected, 'PointId permutation changed clustering')
                checks += 1

    # Interior/shell encoding is an interchangeable representation of a SET.
    shell_data = copy.deepcopy(data)
    for population, c in zip(shell_data['populations'], shell_data['contributions']):
        population['shell'], population['interior'] = population['interior'], []
        c['include_interior'], c['shell_mask'] = False, (1 << len(population['shell']))-1
    need(SourceTree(shell_data).project()['tree'] == source.project()['tree'], 'shell/interior representation changed projection')

    singleton = dict(schema=data['schema'], status='completed', point_count=1, k=1,
                     nodes=[dict(id=0, level=encoded(0), children=[], successor=None)], roots=[0],
                     populations=[dict(interior=[0], shell=[])],
                     contributions=[dict(level=encoded(0), segment=0, population=0, shell_mask=0, include_interior=True)])
    only = SourceTree(singleton)
    for method, z in methods:
        result = only.project(method, z)['tree']
        need(result['root'] == 0 and result['children'] == {}, 'singleton projection changed point')
        checks += verify_graft(singleton, [(0, Fraction(0))], result)

    near = copy.deepcopy(data)
    near['contributions'][4]['level'] = encoded(Fraction(4)+Fraction(1,10**14))
    for z in (1, 2):
        voted = SourceTree(near).project('entry_vote', z)
        need(voted['tree']['anchors'][5] == dict(source_node=3, squared_activation=encoded(9)),
             'numerically unresolved competition did not stay at LCA')
    return checks


def refusal_tests():
    mutations = {
        'schema': lambda d: d.update(schema='wrong'),
        'failed_status': lambda d: d.update(status='failed'),
        'n_bool': lambda d: d.update(point_count=True),
        'k_too_large': lambda d: d.update(k=7),
        'node_id': lambda d: d['nodes'][0].update(id=1),
        'one_child': lambda d: d['nodes'][3].update(children=[0]),
        'duplicate_child': lambda d: d['nodes'][3].update(children=[0, 0]),
        'future_child': lambda d: d['nodes'][3].update(children=[0, 4]),
        'successor': lambda d: d['nodes'][0].update(successor=4),
        'non_strict_birth': lambda d: d['nodes'][3].update(level=encoded(1)),
        'wrong_root': lambda d: d.update(roots=[3]),
        'negative_level': lambda d: d['nodes'][0].update(level=encoded(-1)),
        'zero_denominator': lambda d: d['nodes'][0].update(level=dict(num='1', den='0')),
        'float_level': lambda d: d['nodes'][0].update(level=dict(num=1.0, den='1')),
        'level_plus': lambda d: d['nodes'][0].update(level=dict(num='+1', den='1')),
        'level_space': lambda d: d['nodes'][0].update(level=dict(num=' 1', den='1')),
        'level_underscore': lambda d: d['nodes'][0].update(level=dict(num='1_0', den='1')),
        'level_leading_zero': lambda d: d['nodes'][0].update(level=dict(num='01', den='1')),
        'segment_oob': lambda d: d['contributions'][0].update(segment=5),
        'population_oob': lambda d: d['contributions'][0].update(population=20),
        'before_birth': lambda d: d['contributions'][0].update(level=encoded(0)),
        'at_successor': lambda d: d['contributions'][0].update(level=encoded(9)),
        'nonbool_interior': lambda d: d['contributions'][0].update(include_interior=1),
        'mask_oob': lambda d: d['contributions'][0].update(shell_mask=1),
        'empty_contribution': lambda d: d['contributions'][0].update(include_interior=False),
        'duplicate_point': lambda d: d['populations'][0]['interior'].append(0),
        'point_oob': lambda d: d['populations'][0]['interior'].append(6),
        'point_bool': lambda d: d['populations'][0]['interior'].__setitem__(0, True),
        'unselected_bad_id': lambda d: d['populations'][0]['shell'].append(6),
        'unselected_duplicate': lambda d: d['populations'][0]['shell'].append(0),
        'unused_bad_population': lambda d: d['populations'].append(dict(interior=[99], shell=[])),
    }
    # Remove point 4 from its later continuation as well, leaving domain incomplete.
    mutations['missing_point'] = lambda d: [p['interior'].__setitem__(slice(None), [0 if x == 4 else x for x in p['interior']]) for p in d['populations']]
    for name, mutate in mutations.items():
        data = fixture()
        mutate(data)
        try:
            SourceTree(data)
        except (ValueError, KeyError, TypeError):
            continue
        raise RuntimeError('malformed source accepted: '+name)
    source = SourceTree(fixture())
    for anchors in ([], [(0, Fraction(0))]*6, [(0, Fraction(9))]*6, [(0, 1)]*6):
        try:
            source.graft(anchors)
        except ValueError:
            continue
        raise RuntimeError('invalid anchors accepted')
    collapsed = [(0, Fraction(1)), (0, Fraction(1)), (0, Fraction(1)+Fraction(1,2**60)),
                 (0, Fraction(4)), (1, Fraction(1)), (2, Fraction(1))]
    try:
        source.graft(collapsed)
    except ValueError:
        pass
    else:
        raise RuntimeError('distinct exact levels silently collapsed in float rendering')
    return len(mutations)+5


def single_linkage_cut(points, cut, closed):
    parent = list(range(len(points)))

    def find(x):
        while parent[x] != x:
            x = parent[x]
        return x

    for i, a in enumerate(points):
        for j, b in enumerate(points[:i]):
            squared_radius = Fraction(sum((x-y)**2 for x, y in zip(a, b)), 4)
            if squared_radius < cut or (closed and squared_radius == cut):
                parent[find(i)] = find(j)
    groups = {}
    for point in range(len(points)):
        groups.setdefault(find(point), []).append(point)
    return partition(groups.values())


def native_tests(binary):
    rng = random.Random(20260927)
    random_points = []
    while len(random_points) < 8:
        point = tuple(rng.randrange(1, 41) for _ in range(3))
        if point not in random_points:
            random_points.append(point)
    families = [('line3', [(0, 0, 0), (2, 0, 0), (4, 0, 0)]),
                ('square', [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)]),
                ('random3d', random_points)]
    cases = [(name, points, k) for name, points in families for k in range(1, min(4, len(points))+1)]
    cases += [('spatial6', [(0, 0, 0), (3, 0, 0), (0, 4, 0), (0, 0, 5), (7, 3, 2), (9, 8, 5)], 1),
              ('diamond', [(1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0)], 3)]
    checks, bounds, lower_equal_positive, upper_equal_positive, lca_delays = 0, 0, 0, 0, 0
    with tempfile.TemporaryDirectory(prefix='mhgp9-point-projection-test-') as temporary:
        path = Path(temporary)/'points.u32le'
        for name, points, k in cases:
            path.write_bytes(b''.join(struct.pack('<III', *p) for p in points))
            run = subprocess.run([str(binary), '--input', str(path), '--k', str(k), '--workers', '1', '--verify-coverage'],
                                 capture_output=True, text=True, timeout=60, check=False)
            need(run.returncode == 0 and run.stderr == '', 'native fixture refused: '+run.stderr)
            data = json.loads(run.stdout)
            need(data['points'] == list(map(list, points)), 'native input IDs/coordinates changed')
            source = SourceTree(data)
            anchors = naïve_first_anchors(data)
            # alpha_K is the FIRST COVERAGE DATE, before the optional LCA
            # delay. Both sides below are squared radii, in exact Fractions.
            for point, entries in enumerate(source.entries):
                alpha_squared = min(entries.values())
                squared_distances = sorted(sum((a-b)**2 for a, b in zip(points[point], other)) for other in points)
                kth_squared = squared_distances[k-1]  # self is the first neighbour
                need(Fraction(kth_squared, 4) <= alpha_squared <= kth_squared,
                     f'free-centre/KNN bound failed: {name}, K={k}, point={point}, alpha2={alpha_squared}, dK2={kth_squared}')
                need(anchors[point][1] >= alpha_squared, 'LCA attachment precedes first coverage')
                bounds += 1
                lca_delays += anchors[point][1] > alpha_squared
                lower_equal_positive += alpha_squared > 0 and alpha_squared == Fraction(kth_squared,4)
                upper_equal_positive += alpha_squared > 0 and alpha_squared == kth_squared
                if k == 1:
                    need(alpha_squared == kth_squared == 0, 'self-counted K1 first coverage is not zero')
                if k == 2:
                    need(alpha_squared == Fraction(kth_squared,4), 'K2 first coverage is not nearest-neighbour diameter radius')
                if name == 'line3' and k == 3:
                    need(alpha_squared == 4, 'three collinear points do not have the known free-centre radius squared four')
            tree = source.project()['tree']
            checks += verify_graft(data, anchors, tree)
            if k == 1:
                distances = {Fraction(0), *(Fraction(sum((x-y)**2 for x,y in zip(a,b)),4)
                                           for i,a in enumerate(points) for b in points[:i])}
                for cut in sorted(distances):
                    for closed in (False, True):
                        need(point_tree_cut(tree,cut,closed) == single_linkage_cut(points,cut,closed),
                             'K1 differs from exact single linkage at ball radius distance/2')
                        checks += 1
    need(lower_equal_positive > 0 and upper_equal_positive > 0 and lca_delays > 0,
         'native fixtures missed positive sharp bounds or an LCA delay')
    return dict(native_cases=len(cases), native_exact_checks=checks, alpha_knn_squared_bound_checks=bounds,
                alpha_positive_lower_equalities=lower_equal_positive,
                alpha_positive_upper_equalities=upper_equal_positive, native_lca_delayed_points=lca_delays)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path)
    args = parser.parse_args()
    checks = pure_tests()
    refused = refusal_tests()
    native = native_tests(args.native.resolve()) if args.native else dict(native_cases=0, native_exact_checks=0,
        alpha_knn_squared_bound_checks=0, alpha_positive_lower_equalities=0,
        alpha_positive_upper_equalities=0, native_lca_delayed_points=0)
    print(json.dumps(dict(schema='mhgp9_fixed_k_projection_selftest_v1', status='passed',
                          exact_cut_checks=checks, refusals=refused, **native, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
