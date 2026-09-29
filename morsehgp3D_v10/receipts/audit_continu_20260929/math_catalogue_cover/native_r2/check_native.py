#!/usr/bin/env python3
"""Tiny native resolution audit, independent exact convex-lens nerve oracle."""
import argparse
from fractions import Fraction as F
from functools import lru_cache
from hashlib import sha256
from itertools import combinations
import json
from pathlib import Path
import subprocess

NONE = 2**32 - 1
ARCHIVE = Path('/tmp/mhgp10-audit-pool-head.X45WLs/build/libmhgp10_core.a')
SNAPSHOT = Path('/tmp/mhgp10-audit-pool-head.X45WLs/snapshot/morsehgp3D_v10/src')
CASES = {
    'pair': [(0, 0, 0), (2, 0, 0)],
    'line_asym': [(0, 0, 0), (2, 0, 0), (6, 0, 0)],
    'line_tie': [(0, 0, 0), (2, 0, 0), (4, 0, 0)],
    'line_deep': [(i, 0, 0) for i in range(6)],
    'triangle_interior': [(0, 0, 0), (4, 0, 0), (2, 3, 0), (2, 1, 0)],
    'square': [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)],
    'tetra': [(0, 0, 0), (4, 4, 0), (4, 0, 4), (0, 4, 4)],
    'tetra_interior': [(0, 0, 0), (4, 4, 0), (4, 0, 4), (0, 4, 4), (2, 2, 2)],
}


def require(condition, payload):
    if not condition:
        raise RuntimeError(payload)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def distance2(a, b):
    return sum((x - y)**2 for x, y in zip(a, b))


def center(points):
    if len(points) == 1:
        return tuple(map(F, points[0])), [F(1)]
    base = points[0]
    vectors = [tuple(x - y for x, y in zip(p, base)) for p in points[1:]]
    matrix = [[F(dot(v, w)) for w in vectors] + [F(dot(v, v), 2)] for v in vectors]
    for col in range(len(vectors)):
        pivot = next((row for row in range(col, len(vectors)) if matrix[row][col]), None)
        if pivot is None:
            return None
        matrix[col], matrix[pivot] = matrix[pivot], matrix[col]
        scale = matrix[col][col]
        matrix[col] = [a / scale for a in matrix[col]]
        for row in range(len(vectors)):
            if row != col:
                scale = matrix[row][col]
                matrix[row] = [a - scale * b for a, b in zip(matrix[row], matrix[col])]
    coeff = [row[-1] for row in matrix]
    c = tuple(F(base[j]) + sum(a * v[j] for a, v in zip(coeff, vectors)) for j in range(3))
    return c, [1 - sum(coeff)] + coeff


def check_geometry(name, data, counts):
    points = [tuple(p) for p in data['points']]
    n = len(points)
    levels = [F(int(a), int(b)) for a, b in data['levels']]
    balls = data['balls']

    @lru_cache(None)
    def meb(ids):
        best = None
        for q in range(1, min(4, len(ids)) + 1):
            for support in combinations(ids, q):
                candidate = center([points[i] for i in support])
                if candidate is None:
                    continue
                c, weights = candidate
                if min(weights) < 0:
                    continue
                beta = distance2(c, points[support[0]])
                if all(distance2(c, points[i]) <= beta for i in ids) and (best is None or beta < best[0]):
                    best = beta, c
        require(best is not None, ('MEB failure', ids))
        return best

    centers = []
    for b, ball in enumerate(balls):
        support = [i for i in ball['S'] if i != NONE]
        c, weights = center([points[i] for i in support])
        beta = distance2(c, points[support[0]])
        require(min(weights) > 0 and len(support) == ball['q'], ('support', name, b))
        require(beta == levels[ball['rank']], ('level', name, b))
        require(ball['I'] == [i for i, p in enumerate(points) if distance2(c, p) < beta], ('I', name, b))
        require(ball['U'] == [i for i, p in enumerate(points) if distance2(c, p) == beta], ('U', name, b))
        require(ball['p'] == len(ball['I']) and ball['u'] == len(ball['U']), ('population', name, b))
        centers.append(c)

    critical = sorted({meb(ids)[0] for q in range(1, n + 1) for ids in combinations(range(n), q)})
    cuts = sorted(set(critical + [(a + b) / 2 for a, b in zip(critical, critical[1:])] + [critical[-1] + 1]))
    details = []
    for order in data['orders']:
        k = order['k']
        node_levels = [F(0) if rank == 0 else levels[rank - 1] for rank in order['rank']]

        def ancestor(node, beta):
            require(node != NONE and node < len(node_levels), ('bad node', name, k, node))
            require(node_levels[node] <= beta, ('future node', name, k, node, beta))
            seen = set()
            while order['parent'][node] != NONE and node_levels[order['parent'][node]] <= beta:
                require(node not in seen, ('cycle', name, k, node))
                seen.add(node)
                node = order['parent'][node]
            return node

        if k == 1:
            require(order['ball_node'] == [], ('K1 must use sites, not assume ball_node table', name))
            counts['k1_site_table_orders'] += 1
        else:
            require(len(order['ball_node']) == len(balls), ('ball table size', name, k))
            for b, ball in enumerate(balls):
                if ball['p'] + ball['u'] < k:
                    require(order['ball_node'][b] == NONE, ('inactive population', name, k, b))
                else:
                    own = levels[ball['rank']]
                    require(ancestor(order['ball_node'][b], own) == order['ball_node'][b], ('not active at own radius', name, k, b))
            for x in range(n):
                first = next(b for b, ball in enumerate(balls) if ball['p'] + ball['u'] >= k and x in ball['I'] + ball['U'])
                require(order['point_node'][x] == order['ball_node'][first], ('first node', name, k, x))
                require(order['point_cat_rank'][x] == balls[first]['rank'] + 1, ('first rank', name, k, x))
                counts['first_point_checks'] += 1

        for beta in cuts:
            # Q_F = intersection of radius-sqrt(beta) balls centered at F.
            # Q_F intersects Q_G iff MEB(F union G)^2 <= beta; each is convex.
            active = [ids for ids in combinations(range(n), k) if meb(ids)[0] <= beta]
            parent = {ids: ids for ids in active}

            def root(ids):
                while parent[ids] != ids:
                    ids = parent[ids]
                return ids

            for left, right in combinations(active, 2):
                if meb(tuple(sorted(set(left) | set(right))))[0] <= beta:
                    a, b = root(left), root(right)
                    parent[max(a, b)] = min(a, b)
            oracle_components = {root(ids) for ids in active}

            def geometric_component(c):
                candidates = {root(ids) for ids in active if all(distance2(c, points[i]) <= beta for i in ids)}
                require(len(candidates) == 1, ('center component', name, k, beta, c, candidates))
                return next(iter(candidates))

            # Fix component identities from native BIRTH geometry, not from ball_node.
            mapping = {}
            for node, birth in enumerate(order['birth']):
                if birth == NONE or node_levels[node] > beta:
                    continue
                c = points[birth] if k == 1 else centers[birth]
                expected, actual = geometric_component(c), ancestor(node, beta)
                require(actual not in mapping or mapping[actual] == expected, ('false native merge', name, k, beta))
                mapping[actual] = expected
            active_nodes = {node for node, level in enumerate(node_levels) if level <= beta and
                            (order['parent'][node] == NONE or node_levels[order['parent'][node]] > beta)}
            require(set(mapping) == active_nodes, ('unrepresented native component', name, k, beta))
            require(set(mapping.values()) == oracle_components and len(mapping) == len(oracle_components),
                    ('native component bijection', name, k, beta))

            witnesses = []
            if k == 1:
                witnesses = [(mapping[ancestor(order['point_node'][x], beta)], [x], F(0), -1) for x in range(n)]
            else:
                for b, ball in enumerate(balls):
                    own = levels[ball['rank']]
                    if own > beta or ball['p'] + ball['u'] < k:
                        continue
                    actual = mapping[ancestor(order['ball_node'][b], beta)]
                    expected = geometric_component(centers[b])
                    require(actual == expected, ('wrong native ball resolution', name, k, beta, b, actual, expected))
                    counts['ball_resolution_checks'] += 1
                    counts['resolution_after_own_radius'] += own < beta
                    counts['deep_balls_p_ge_k'] += ball['p'] >= k
                    counts['interior_q3_q4_checks'] += ball['q'] >= 3 and bool(ball['I'])
                    witnesses.append((actual, ball['I'] + ball['U'], own, b))

            for x in range(n):
                expected = {root(ids) for ids in active if x in ids}
                actual = {comp for comp, ids, _, _ in witnesses if x in ids}
                require(actual == expected, ('cover component list', name, k, beta, x, actual, expected))
                counts['point_component_lists'] += 1
                counts['multi_component_lists'] += len(expected) > 1
                if name == 'line_asym' and k == 2 and beta == 4 and points[x] == (2, 0, 0):
                    first_only = {mapping[ancestor(order['point_node'][x], beta)]}
                    require(len(expected) == 2 and len(first_only) == 1, 'first-only counterexample missing')
                    details.append({'case': name, 'K': k, 'radius_squared': '4', 'point': list(points[x]),
                                    'all_components': [list(c) for c in sorted(expected)],
                                    'first_only': [list(c) for c in first_only]})
            counts['cuts_checked'] += 1
    counts['scenes'] += 1
    return details


def hashes(probe):
    paths = [ARCHIVE, probe, Path(__file__), Path(__file__).with_name('probe.cpp')]
    paths += sorted(p for p in SNAPSHOT.rglob('*') if p.is_file())
    return {str(p): sha256(p.read_bytes()).hexdigest() for p in paths}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    require(not args.output.exists(), 'refuse overwrite of receipt directory')
    args.output.mkdir(parents=True)
    before = hashes(args.probe)
    counts = dict(scenes=0, k1_site_table_orders=0, cuts_checked=0, point_component_lists=0,
                  multi_component_lists=0, ball_resolution_checks=0, resolution_after_own_radius=0,
                  deep_balls_p_ge_k=0, interior_q3_q4_checks=0, first_point_checks=0)
    commands, details = [], []
    for name, points in CASES.items():
        source = args.output / (name + '.txt')
        source.write_text(''.join(' '.join(map(str, p)) + '\n' for p in points))
        base = None
        for workers, ball_nodes in ((1, 1), (2, 1), (1, 0), (2, 0)):
            argv = [str(args.probe), str(source), str(min(4, len(points))), str(workers), str(ball_nodes)]
            run = subprocess.run(argv, capture_output=True, timeout=10)
            stem = args.output / (name + '_w' + str(workers) + '_b' + str(ball_nodes))
            stem.with_suffix('.stdout').write_bytes(run.stdout)
            stem.with_suffix('.stderr').write_bytes(run.stderr)
            commands.append({'argv': argv, 'returncode': run.returncode,
                             'stdout_sha256': sha256(run.stdout).hexdigest()})
            (args.output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
            require(run.returncode == 0, ('native call', argv, run.stderr.decode()))
            data = json.loads(run.stdout)
            if base is None:
                base = data
                details += check_geometry(name, data, counts)
            elif ball_nodes:
                require(base == data, ('worker differential', name))
            else:
                for order in data['orders']:
                    require(order['ball_node'] == [], ('disabled table not empty', name))
                reference = json.loads(json.dumps(base))
                for order in reference['orders']:
                    order['ball_node'] = []
                require(reference == data, ('first-only path differential', name, workers))
    after = hashes(args.probe)
    require(before == after, 'pinned sources/archive/probe changed')
    require(counts['interior_q3_q4_checks'] > 0 and counts['deep_balls_p_ge_k'] > 0, 'required regimes missing')
    receipt = {'status': 'PASS', 'scope': 'bounded native ball_node resolution and closed-cut coverage lists',
               'source_commit_of_pinned_archive': '6206d1d11', 'engine_rebuilt': False, 'GCP_used': False,
               'counts': counts, 'native_calls': len(commands), 'counterexamples': details,
               'hashes_before': before, 'hashes_after': after, 'source_closure': True}
    (args.output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({key: value for key, value in receipt.items() if not key.startswith('hashes_')}, sort_keys=True))


if __name__ == '__main__':
    main()
