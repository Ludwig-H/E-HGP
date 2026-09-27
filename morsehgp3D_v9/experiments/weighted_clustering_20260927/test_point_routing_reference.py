#!/usr/bin/env python3
"""Tiny dense oracle against the sparse exact routing reference."""
from copy import deepcopy
from fractions import Fraction as Q
from itertools import combinations
import argparse
import hashlib
import json
from pathlib import Path
import random

from point_routing_reference import route_points, cut, need, rational

COUNTS = dict(fixtures=0, cuts=0, refusals=0, qualified_integrations=0)


def dense(args):
    """Independent raw-tree plateau flattening and descendant-set voting."""
    n, facets, scores, children, heights, roots, births = args
    F = len(facets)
    levels = dict(enumerate(map(Q, births))); levels.update({node: Q(value) for node, value in heights.items()})
    def atomic_children(node):
        result = []; pending = list(children.get(node, ()))
        while pending:
            child = pending.pop()
            if child in children and levels[child] == levels[node]:
                pending.extend(children[child])
            else:
                result.append(child)
        return result
    descendants = {}
    for root in roots:
        pending = [(root, False)]
        while pending:
            node, exiting = pending.pop()
            if node < F:
                descendants[node] = {node}
            elif exiting:
                descendants[node] = set().union(*(descendants[child] for child in children[node]))
            else:
                pending.append((node, True)); pending.extend((child, False) for child in children[node])
    result = []
    for point in range(n):
        score = lambda node: sum((Q(scores[leaf]) for leaf in descendants[node] if point in facets[leaf]), Q())
        votes = {root: score(root) for root in roots}
        best = max(votes.values(), default=Q())
        winners = [root for root in roots if votes[root] == best] if best else []
        if not best or len(winners) != 1:
            result.append((None, None, 'uncovered' if not best else 'root_tie'))
            continue
        node = winners[0]
        while node >= F:
            kids = atomic_children(node)
            votes = {child: score(child) for child in kids}
            best = max(votes.values()); winners = [child for child in kids if votes[child] == best]
            if len(winners) != 1:
                break
            node = winners[0]
        result.append((node, levels[node], 'facet' if node < F else 'branch_tie'))
    return result


def dense_cut(result, at, closed):
    admitted = (lambda beta: beta <= at) if closed else (lambda beta: beta < at)
    groups = {}
    for row in result['attachments']:
        node = row['node']
        if node is None or not admitted(row['beta']):
            key = ('point', row['point'])
        else:
            while node in result['tree']['parents'] and admitted(result['tree']['levels'][result['tree']['parents'][node]]):
                node = result['tree']['parents'][node]
            key = ('source', node)
        groups.setdefault(key, []).append(row['point'])
    return sorted(groups.values())


def refine(fine, coarse):
    return all(any(set(block) <= set(parent) for parent in coarse) for block in fine)


def checked(args, all_dates=True):
    saved = deepcopy(args); result = route_points(*args)
    need(args == saved, 'input immutability')
    actual = [(row['node'], row['beta'], row['reason']) for row in result['attachments']]
    need(actual == dense(args), 'sparse versus independent dense routing')
    levels = sorted({Q(0), *result['tree']['levels'].values()})
    if not all_dates:
        levels = [levels[0], levels[len(levels)//2], levels[-1]]
    queries = sorted(set(levels + [(a+b)/2 for a, b in zip(levels, levels[1:])] + [levels[-1]+1]))
    previous = [[point] for point in range(args[0])]
    for at in queries:
        for closed in (False, True):
            partition = cut(result, at, closed=closed)
            need(partition == dense_cut(result, at, closed), 'binary lifting versus naive cut')
            need(sorted(point for block in partition for point in block) == list(range(args[0])), 'total exclusive point partition')
            need(refine(previous, partition), 'nested strict/closed chronological cuts')
            previous = partition; COUNTS['cuts'] += 1
    COUNTS['fixtures'] += 1
    return result


def fixture_tests():
    # Atomization must choose c (3/2), not the artificial subtree a+b (2).
    args = (4, [[0, 1], [0, 2], [0, 3]], [Q(1), Q(1), Q(3, 2)],
            {3: [0, 1], 4: [3, 2]}, {3: Q(5), 4: Q(5)}, [4], [Q(1), Q(2), Q(3)])
    result = checked(args)
    need(result['attachments'][0]['node'] == 2 and result['attachments'][0]['beta'] == 3,
         'plateau winner and actual facet birth, not virtual zero')
    need(result['statistics']['collapsed_plateau_nodes'] == 1, 'one artificial plateau node contracted')
    # Root vote A=6 > B=5, then a tie inside A: do not revote globally to B.
    tie = (4, [[0, 1], [0, 2], [0, 3]], [Q(3), Q(3), Q(5)],
           {3: [0, 1], 4: [3, 2]}, {3: Q(4), 4: Q(9)}, [4], [Q(1), Q(2), Q(3)])
    result = checked(tie)
    need(result['attachments'][0]['node'] == 3 and result['attachments'][0]['reason'] == 'branch_tie', 'parent-to-child choice')
    need([0] in cut(result, Q(4), closed=False) and [0, 1, 2] in cut(result, Q(4)), 'tie is not attached retroactively')
    # External point 0 must not collide with facet/source node 0 containing 1.
    outside = checked((3, [[1]], [Q(2)], {}, {}, [0], [Q(7)]))
    need(cut(outside, Q(100)) == [[0], [1], [2]], 'external/source ID namespaces')
    forest = checked((3, [[0, 1], [0, 2]], [Q(1), Q(1)], {}, {}, [0, 1], [Q(1), Q(2)]))
    need(forest['attachments'][0]['reason'] == 'root_tie' and cut(forest, Q(100)) == [[0], [1], [2]], 'root tie stays external')
    checked((3, [], [], {}, {}, [], []))
    # Permute all PointIds and facet IDs, including internal ID ordering.
    permutation = [2, 0, 3, 1]
    p_facets = [[permutation[x] for x in face] for face in reversed(tie[1])]
    p_args = (4, p_facets, list(reversed(tie[2])), {8: [2, 1], 6: [0, 8]},
              {8: Q(4), 6: Q(9)}, [6], list(reversed(tie[6])))
    permuted = checked(p_args); base = route_points(*tie)
    for at in map(Q, (0, 1, 2, 3, 4, 5, 9, 10)):
        for closed in (False, True):
            expected = sorted(sorted(permutation[x] for x in group) for group in cut(base, at, closed=closed))
            need(cut(permuted, at, closed=closed) == expected, 'point/facet/internal permutation invariance')


def random_tests():
    rng = random.Random(20260927)
    possible = list(combinations(range(8), 2))
    for _ in range(120):
        F = rng.randrange(2, 10); facets = [list(face) for face in rng.sample(possible, F)]
        scores = [Q(rng.randrange(1, 7), rng.randrange(1, 5)) for _ in range(F)]
        births = [Q(rng.randrange(3)) for _ in range(F)]
        levels = dict(enumerate(births)); children = {}; roots = list(range(F)); node = F
        while len(roots) > 1:
            if rng.randrange(6) == 0:
                break
            kids = rng.sample(roots, rng.randrange(2, min(4, len(roots))+1))
            for child in kids:
                roots.remove(child)
            children[node] = kids
            levels[node] = max(levels[child] for child in kids) + rng.randrange(3)
            roots.append(node); node += 1
        checked((8, facets, scores, children, {node: levels[node] for node in children}, roots, births))


def deep_test():
    children = {}; heights = {}; previous = 0
    for node in range(2, 5002):
        children[node] = [previous]; heights[node] = Q(node); previous = node
    children[5002] = [previous, 1]; heights[5002] = Q(5002)
    result = checked((2, [[0], [1]], [Q(1), Q(1)], children, heights, [5002], [Q(1), Q(1)]), False)
    need(result['statistics']['virtual_nodes'] == 2 and result['statistics']['lca_queries'] == 0,
         'single incident facet skips a deep source chain')


def refusals():
    good = (3, [[0, 1], [0, 2]], [Q(1), Q(2)], {2: [0, 1]}, {2: Q(3)}, [2], [Q(1), Q(2)])
    for index, replacement in ((0, True), (1, [[0, 1], [0, 1]]), (1, [[0], [0, 2]]),
                               (1, [[0, 1], [0, 3]]), (2, [1.0, Q(2)]), (2, [True, Q(2)]),
                               (2, [Q(0), Q(2)]), (3, {2: [0, 0]}), (3, {2: [0, 2]}),
                               (4, {2: Q(1)}), (4, {2.0: Q(3)}), (4, {True: Q(3)}),
                               (5, [0]), (6, [Q(1)])):
        bad = list(deepcopy(good)); bad[index] = replacement
        try:
            route_points(*bad)
        except ValueError:
            COUNTS['refusals'] += 1
        else:
            raise RuntimeError('malformed source accepted')


def qualified_integrations(folder):
    """Read existing qualified native outputs; never launch a native process."""
    from weighted_model import build_facet_model
    from full_weighted_tree import build_full_weighted_tree
    from full_attachment_oracle import build_reference
    sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
    receipt_path = folder / 'receipt.json'
    need(sha(receipt_path) == '35126c2a591a4d410b44d86a80967b50f4574bcdb9cadc5d94d9a982bafaba9c', 'qualified fixture receipt pin')
    receipt = json.loads(receipt_path.read_text())
    need(receipt['status'] == 'passed' and receipt['sources_before'] == receipt['sources_after'], 'qualified source closure')
    names = ('e5_silent_k2', 'square_k2', 'square_k1')
    fixtures = {row['name']: row for row in receipt['fixtures']}
    commands = {row['name']: row for row in receipt['commands']}
    pins = {str(receipt_path): sha(receipt_path)}
    for name in ('weighted_model.py', 'full_weighted_tree.py', 'full_attachment_oracle.py', 'qualify_geometry.py'):
        path = Path(__file__).resolve().parent / name
        need(sha(path) == receipt['sources_before'][str(path)], 'qualified helper source pin')
        pins[str(path)] = sha(path)
    for name in names:
        path = folder / (name + '.stdout')
        need(sha(path) == commands[name]['stdout_sha256'], 'qualified native output pin')
        pins[str(path)] = sha(path)
        payload = json.loads(path.read_text()); weighted = payload['weighted']; native = weighted['native']
        fixture = fixtures[name]; n, k = len(fixture['points']), fixture['k']
        model = build_facet_model(n, k, [{field: row[field] for field in ('vertices', 'beta')}
                                      for row in weighted['cofaces']], exp_z=2, rational_z2=True)
        tree = build_full_weighted_tree(native['nodes'], native['roots'], model['masses'],
                  [dict(node=row['node'], beta=row['beta']) for row in payload['attachments']])
        args = (n, model['facets'], model['scores'], tree['tree']['children'],
                {node: rational(value) for node, value in tree['tree']['squared_levels'].items()},
                tree['tree']['roots'], list(map(rational, tree['leaf_birth_betas'])))
        actual = checked(args)
        reference = build_reference(fixture['points'], k, exp_z=2, rational_z2=True)
        need(model['facets'] == reference['facets'] and model['scores'] == reference['scores'], 'independent exact Gabriel scores')
        expected = checked((n, reference['facets'], reference['scores'], reference['children'],
                            {node: reference['squared_levels'][node] for node in reference['children']},
                            reference['roots'], reference['leaf_birth_betas']))
        need([(row['beta'], row['reason']) for row in actual['attachments']] ==
             [(row['beta'], row['reason']) for row in expected['attachments']], 'independent FULL/Cech routed dates')
        for at in sorted({Q(0), *reference['critical_betas'], *actual['tree']['levels'].values()}):
            for closed in (False, True):
                need(cut(actual, at, closed=closed) == cut(expected, at, closed=closed), 'native/oracle projected cuts')
                COUNTS['cuts'] += 1
        if name == 'square_k2':
            need(all(row['beta'] == 2 and row['reason'] == 'branch_tie' for row in actual['attachments']), 'square symmetric LCA')
            need(cut(actual, Q(2), closed=False) == [[0], [1], [2], [3]] and cut(actual, Q(2)) == [[0, 1, 2, 3]], 'square exact tie date')
        elif name == 'square_k1':
            need(all(row['beta'] == 0 and row['reason'] == 'facet' for row in actual['attachments']), 'K1 actual point births')
            need(cut(actual, Q(1), closed=False) == [[0], [1], [2], [3]] and cut(actual, Q(1)) == [[0, 1, 2, 3]], 'K1 single-linkage radius distance/2')
        else:
            need(args[-1][model['facets'].index((0, 2))] == Q(33, 2), 'E5 silent AC actual birth retained')
            need([row['beta'] for row in actual['attachments']] == [Q(189, 17), Q(31, 2), Q(11, 2), Q(9, 2), Q(9, 2)], 'E5 exact routed dates')
        COUNTS['qualified_integrations'] += 1
    need(all(sha(path) == digest for path, digest in pins.items()), 'integration inputs/helpers unchanged')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qualified-fixtures', type=Path)
    args = parser.parse_args()
    fixture_tests(); random_tests(); deep_test(); refusals()
    if args.qualified_fixtures:
        qualified_integrations(args.qualified_fixtures)
    print(json.dumps(dict(status='passed', **COUNTS, scope='exact routing/cut reference, no EOM or statistical score'), sort_keys=True))
