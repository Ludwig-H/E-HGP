#!/usr/bin/env python3
"""Small exact differential tests; optionally read three frozen FULL outputs."""
from copy import deepcopy
from fractions import Fraction as Q
from itertools import combinations
import argparse
import hashlib
import json
from pathlib import Path
import random
import sys

HERE = Path(__file__).resolve().parent
FROZEN = HERE.parent / 'weighted_clustering_20260927'
sys.path.insert(0, str(FROZEN))
from point_routing_reference import route_points, cut as reference_cut
from point_tree import build_point_tree, cut, to_jsonable, validate_point_tree

COUNTS = dict(fixtures=0, cuts=0, refusals=0, qualified_integrations=0)
ROUTING_SHA = '41c16627b7e69736301063607289886d727aeed7ff45b598ca75b1fecd9af88d'


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def projection(tree):
    """Intrinsic point object only, independent of source IDs/provenance."""
    return {name: tree[name] for name in ('n_points', 'children', 'squared_levels',
            'leaf_birth_betas', 'parents', 'roots', 'point_counts')}


def checked(args=None, *, routing=None, sparse_dates=False):
    routing = route_points(*args) if routing is None else routing
    saved = deepcopy(routing)
    tree = build_point_tree(routing)
    validate_point_tree(tree)
    need(routing == saved, 'source immutability')
    need(tree['n_points'] == routing['n_points'], 'n point leaves')
    need(len(tree['children']) <= max(0, tree['n_points']-len(tree['roots'])), 'linear output bound')
    dates = sorted({Q(0), *routing['tree']['levels'].values()})
    if sparse_dates:
        dates = [dates[0], dates[len(dates)//2], dates[-1]]
    dates = sorted(set(dates + [(a+b)/2 for a, b in zip(dates, dates[1:])] + [dates[-1]+1]))
    previous = [[point] for point in range(tree['n_points'])]
    for beta in dates:
        for closed in (False, True):
            actual = cut(tree, beta, closed=closed)
            need(actual == reference_cut(routing, beta, closed=closed), 'materialized versus reference cut')
            need(sorted(point for group in actual for point in group) == list(range(tree['n_points'])),
                 'total exclusive partition')
            owner = {point: i for i, group in enumerate(actual) for point in group}
            need(all(len({owner[point] for point in group}) == 1 for group in previous), 'nested cuts')
            previous = actual; COUNTS['cuts'] += 1
    wire = json.dumps(to_jsonable(tree), sort_keys=True, separators=(',', ':'))
    need(wire == json.dumps(to_jsonable(build_point_tree(routing)), sort_keys=True, separators=(',', ':')),
         'stable wire representation')
    need(json.loads(wire)['leaf_birth_betas'] == [{'num': '0', 'den': '1'}]*tree['n_points'],
         'lossless rational leaf encoding')
    COUNTS['fixtures'] += 1
    return tree


def fixtures():
    checked((0, [], [], {}, {}, [], []))
    external = checked((4, [], [], {}, {}, [], []))
    need(external['roots'] == [0, 1, 2, 3] and not external['children'], 'no artificial external root')
    late = checked((4, [[0, 1, 2]], [1], {}, {}, [0], [Q(7)]))
    need(late['children'] == {4: [0, 1, 2]} and late['roots'] == [4, 3], 'late multi-attachment plus exterior')
    need(cut(late, 7, closed=False) == [[0], [1], [2], [3]], 'late attachment strict endpoint')
    need(cut(late, 7) == [[0, 1, 2], [3]], 'late attachment closed endpoint')
    zero = checked((3, [[0], [1], [2]], [1, 1, 1], {3: [0, 1], 4: [3, 2]},
                    {3: Q(0), 4: Q(0)}, [4], [Q(0)]*3))
    need(zero['children'] == {3: [0, 1, 2]} and zero['squared_levels'] == {3: 0}, 'zero atomic multifusion')
    need(cut(zero, 0, closed=False) == [[0], [1], [2]] and cut(zero, 0) == [[0, 1, 2]], 'zero endpoint semantics')
    equal_leaf = checked((4, [[0, 1], [2, 3]], [1, 1], {2: [0, 1]}, {2: Q(4)}, [2], [Q(4), Q(4)]))
    need(equal_leaf['children'] == {4: [0, 1, 2, 3]}, 'facet-parent equal dates atomized')
    tie = (4, [[0, 1], [0, 2], [0, 3]], [3, 3, 5], {3: [0, 1], 4: [3, 2]},
           {3: Q(4), 4: Q(9)}, [4], [Q(1), Q(2), Q(3)])
    tied = checked(tie)
    need(tied['attachments'][0]['reason'] == 'branch_tie' and tied['attachments'][0]['beta'] == 4,
         'branch tie provenance/date')
    checked((5, [[0, 1], [0, 2], [3, 4]], [1, 1, 2], {}, {}, [0, 1, 2], [1, 2, 3]))
    empty = checked((3, [[0, 1], [0, 2], [1, 2]], [1, 1, 1], {3: [0, 1, 2]},
                     {3: 5}, [3], [1, 2, 3]))
    need(empty['statistics']['empty_source_classes'] == 3, 'empty source branches removed')
    # Same source object, arbitrary sibling/root/attachment/dictionary ordering.
    route = route_points(*tie); permuted = deepcopy(route)
    permuted['tree']['children'] = {node: list(reversed(kids))
        for node, kids in reversed(list(route['tree']['children'].items()))}
    permuted['tree']['levels'] = dict(reversed(list(route['tree']['levels'].items())))
    permuted['attachments'].reverse()
    need(build_point_tree(permuted) == build_point_tree(route), 'source traversal order invariance')
    # Relabel source IDs and PointIds. Compare cuts under PointId permutation,
    # and exact canonical intrinsic object when only source IDs are permuted.
    relabeled = (4, list(reversed(tie[1])), list(reversed(tie[2])), {8: [2, 1], 6: [0, 8]},
                 {8: Q(4), 6: Q(9)}, [6], list(reversed(tie[6])))
    need(projection(checked(relabeled)) == projection(tied), 'source ID independent internal numbering')
    permutation = [2, 0, 3, 1]
    args = list(relabeled); args[1] = [[permutation[x] for x in face] for face in args[1]]
    changed = checked(tuple(args))
    for beta in map(Q, (0, 1, 2, 3, 4, 9, 10)):
        for closed in (False, True):
            expected = sorted(sorted(permutation[x] for x in group) for group in cut(tied, beta, closed=closed))
            need(cut(changed, beta, closed=closed) == expected, 'PointId permutation equivariance')
    # Adjacent rationals indistinguishable as floats still remain two events.
    epsilon = Q(1, 2**80)
    close = checked((3, [[0], [1], [2]], [1]*3, {3: [0, 1], 4: [3, 2]},
                     {3: Q(1), 4: 1+epsilon}, [4], [0]*3))
    need(len(close['children']) == 2 and float(1+epsilon) == 1.0, 'no float date collapse')


def random_tests():
    rng = random.Random(20260927); possible = list(combinations(range(10), 2))
    for _ in range(160):
        F = rng.randrange(1, 14); facets = [list(face) for face in rng.sample(possible, F)]
        scores = [Q(rng.randrange(1, 8), rng.randrange(1, 6)) for _ in range(F)]
        births = [Q(rng.randrange(4)) for _ in range(F)]
        levels = dict(enumerate(births)); children = {}; roots = list(range(F)); node = F
        while len(roots) > 1:
            if rng.randrange(7) == 0:
                break
            kids = rng.sample(roots, rng.randrange(2, min(5, len(roots))+1))
            for child in kids:
                roots.remove(child)
            children[node] = kids; levels[node] = max(levels[child] for child in kids)+rng.randrange(4)
            roots.append(node); node += 1
        checked((10, facets, scores, children, {node: levels[node] for node in children}, roots, births))


def deep_tests():
    exterior = checked((3000, [], [], {}, {}, [], []), sparse_dates=True)
    need(len(exterior['roots']) == 3000 and not exterior['children'], 'many separate external roots')
    children = {}; levels = {}; previous = 0
    for node in range(2, 5002):
        children[node] = [previous]; levels[node] = Q(node); previous = node
    children[5002] = [previous, 1]; levels[5002] = Q(5002)
    tree = checked((2, [[0], [1]], [1, 1], children, levels, [5002], [1, 1]), sparse_dates=True)
    need(tree['children'] == {2: [0, 1]} and tree['squared_levels'] == {2: 5002}, 'deep unary suppression')
    # Large source plateau with many leaves: contraction must never repeatedly
    # concatenate a growing descendant list. Reference atomizes internal nodes.
    n = 3000; children = {}; levels = {}; previous = 0
    for i in range(1, n):
        node = n+i-1; children[node] = [previous, i]; levels[node] = Q(1); previous = node
    plateau = checked((n, [[i] for i in range(n)], [1]*n, children, levels, [previous], [0]*n), sparse_dates=True)
    need(len(plateau['children']) == 1 and plateau['point_counts'][n] == n, 'large atomic multifusion')
    # A deep output, not merely a deep source chain which gets suppressed.
    n = 2400; children = {}; levels = {}; previous = 0
    for i in range(1, n):
        node = n+i-1; children[node] = [previous, i]; levels[node] = Q(i); previous = node
    comb = checked((n, [[i] for i in range(n)], [1]*n, children, levels, [previous], [0]*n), sparse_dates=True)
    need(len(comb['children']) == n-1 and comb['point_counts'][comb['roots'][0]] == n,
         'deep point comb uses iterative construction/validation/cuts')


def refusals():
    args = (3, [[0], [1], [2]], [1]*3, {3: [0, 1], 4: [3, 2]}, {3: 2, 4: 4}, [4], [0]*3)
    route = route_points(*args)
    mutations = [lambda d: d.update(n_points=True),
        lambda d: d['tree']['levels'].__setitem__(4, 1),
        lambda d: d['tree']['levels'].__setitem__(4, 4.0),
        lambda d: d['tree']['parents'].__setitem__(0, 4),
        lambda d: d['tree']['roots'].append(0),
        lambda d: d['attachments'][0].update(beta=1),
        lambda d: d['attachments'][0].update(root=3),
        lambda d: d['attachments'][0].update(reason='root_tie'),
        lambda d: d['attachments'][0].update(point=1),
        lambda d: d['tree']['children'].__setitem__(3, [0, 0])]
    for mutation in mutations:
        bad = deepcopy(route); mutation(bad)
        try:
            build_point_tree(bad)
        except ValueError:
            COUNTS['refusals'] += 1
        else:
            raise RuntimeError('corrupt routing accepted')
    tree = build_point_tree(route)
    mutations = [lambda d: d.update(n_points=True), lambda d: d.update(roots=[0]),
        lambda d: d['children'].__setitem__(3, [0]),
        lambda d: d['parents'].__setitem__(0, 4),
        lambda d: d['point_counts'].__setitem__(4, 2),
        lambda d: d['squared_levels'].__setitem__(4, 2),
        lambda d: d['squared_levels'].__setitem__(4, 4.0),
        lambda d: d['leaf_birth_betas'].__setitem__(0, 1),
        lambda d: d['attachments'][0].update(beta=5),
        lambda d: d['attachments'][0].update(reason='uncovered'),
        lambda d: d['source_to_point_node'].__setitem__(4, 3)]
    for mutation in mutations:
        bad = deepcopy(tree); mutation(bad)
        try:
            validate_point_tree(bad)
        except ValueError:
            COUNTS['refusals'] += 1
        else:
            raise RuntimeError('corrupt point tree accepted')
    for beta, closed in ((True, True), (1.0, True), (-1, True), (1, 1), ({'num': '01', 'den': '1'}, True)):
        try:
            cut(tree, beta, closed=closed)
        except ValueError:
            COUNTS['refusals'] += 1
        else:
            raise RuntimeError('invalid cut accepted')
    other_branch = build_point_tree(route_points(4, [[0, 1], [2, 3]], [1, 1],
        {2: [0, 1]}, {2: 5}, [2], [2, 2]))
    other_branch['attachments'][0]['source_node'] = 1
    try:
        validate_point_tree(other_branch)
    except ValueError:
        COUNTS['refusals'] += 1
    else:
        raise RuntimeError('same-date different-branch provenance accepted')


def qualified_integrations(folder):
    from weighted_model import build_facet_model
    from full_weighted_tree import build_full_weighted_tree
    from full_attachment_oracle import build_reference
    receipt_path = folder/'receipt.json'
    need(sha(receipt_path) == '35126c2a591a4d410b44d86a80967b50f4574bcdb9cadc5d94d9a982bafaba9c', 'FULL qualification pin')
    receipt = json.loads(receipt_path.read_text())
    need(receipt['status'] == 'passed' and receipt['sources_before'] == receipt['sources_after'], 'FULL source closure')
    pins = {str(receipt_path): sha(receipt_path)}
    for name in ('weighted_model.py', 'full_weighted_tree.py', 'full_attachment_oracle.py', 'qualify_geometry.py'):
        path = FROZEN/name
        need(sha(path) == receipt['sources_before'][str(path)], 'frozen geometry helper')
        pins[str(path)] = sha(path)
    fixtures = {row['name']: row for row in receipt['fixtures']}
    commands = {row['name']: row for row in receipt['commands']}
    for name in ('e5_silent_k2', 'square_k2', 'square_k1'):
        path = folder/(name+'.stdout'); pins[str(path)] = sha(path)
        need(pins[str(path)] == commands[name]['stdout_sha256'], 'FULL fixture output pin')
        payload = json.loads(path.read_text()); weighted = payload['weighted']; native = weighted['native']
        fixture = fixtures[name]; n, k = len(fixture['points']), fixture['k']
        model = build_facet_model(n, k, [{field: row[field] for field in ('vertices', 'beta')}
                                       for row in weighted['cofaces']], exp_z=2, rational_z2=True)
        source = build_full_weighted_tree(native['nodes'], native['roots'], model['masses'],
            [dict(node=row['node'], beta=row['beta']) for row in payload['attachments']])
        rational = lambda row: Q(int(row['num']), int(row['den']))
        args = (n, model['facets'], model['scores'], source['tree']['children'],
                {node: rational(value) for node, value in source['tree']['squared_levels'].items()},
                source['tree']['roots'], list(map(rational, source['leaf_birth_betas'])))
        actual = checked(args)
        oracle = build_reference(fixture['points'], k, exp_z=2, rational_z2=True)
        expected = checked((n, oracle['facets'], oracle['scores'], oracle['children'],
            {node: oracle['squared_levels'][node] for node in oracle['children']},
            oracle['roots'], oracle['leaf_birth_betas']))
        need(projection(actual) == projection(expected), 'FULL and independent Cech intrinsic point tree')
        if name == 'e5_silent_k2':
            need(args[-1][model['facets'].index((0, 2))] == Q(33, 2), 'silent AC MEB retained upstream')
            need([row['beta'] for row in actual['attachments']] == [Q(189, 17), Q(31, 2), Q(11, 2), Q(9, 2), Q(9, 2)],
                 'E5 exact attachment dates')
        else:
            beta = 2 if k == 2 else 1
            need(actual['children'] == {n: list(range(n))} and actual['squared_levels'] == {n: beta},
                 'square exact atomic merge')
            need(actual['leaf_birth_betas'] == [0]*n, 'square leaves born zero')
        COUNTS['qualified_integrations'] += 1
    need(all(sha(path) == digest for path, digest in pins.items()), 'frozen integration inputs unchanged')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qualified-fixtures', type=Path)
    args = parser.parse_args()
    need(sha(FROZEN/'point_routing_reference.py') == ROUTING_SHA, 'frozen routing pin before')
    fixtures(); random_tests(); deep_tests(); refusals()
    if args.qualified_fixtures:
        qualified_integrations(args.qualified_fixtures)
    need(sha(FROZEN/'point_routing_reference.py') == ROUTING_SHA, 'frozen routing pin after')
    print(json.dumps(dict(status='passed', **COUNTS, scope='point forest/cuts; no new geometry or EOM'), sort_keys=True))
