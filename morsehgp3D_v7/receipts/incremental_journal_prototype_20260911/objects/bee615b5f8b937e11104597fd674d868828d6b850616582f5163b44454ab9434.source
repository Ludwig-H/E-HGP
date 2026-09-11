#!/usr/bin/env python3
"""Independent bounded rational Gamma/minima models; no product imports.

Exhaustive simplices are a tiny falsification oracle, never an implementation
proposal. No assertion, floating predicate, engine or external write is used.
Usage: --selftest (code 0); invalid invocation 2; failed gate 1.
"""
from __future__ import annotations

from fractions import Fraction as Q
from itertools import combinations
import json
import sys


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def dot(a, b):
    return sum((x * y for x, y in zip(a, b)), Q(0))


def subtract(a, b):
    return tuple(x - y for x, y in zip(a, b))


def solve(matrix, rhs):
    rows = [list(row) + [value] for row, value in zip(matrix, rhs)]
    n = len(rows)
    for col in range(n):
        pivot = next((i for i in range(col, n) if rows[i][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        divisor = rows[col][col]
        rows[col] = [x / divisor for x in rows[col]]
        for i in range(n):
            if i != col:
                multiplier = rows[i][col]
                rows[i] = [x - multiplier * y for x, y in zip(rows[i], rows[col])]
    return [row[-1] for row in rows]


def miniball(points, label):
    candidates = []
    for size in range(1, min(4, len(label)) + 1):
        for support in combinations(label, size):
            origin = points[support[0]]
            vectors = [subtract(points[i], origin) for i in support[1:]]
            coefficients = solve([[dot(u, v) for v in vectors] for u in vectors],
                                 [dot(u, u) / 2 for u in vectors])
            if coefficients is None:
                continue
            weights = [1 - sum(coefficients, Q(0))] + coefficients
            if not all(weight > 0 for weight in weights):
                continue
            center = tuple(origin[k] + sum((a * v[k] for a, v in zip(coefficients, vectors)), Q(0))
                           for k in range(3))
            radius = dot(subtract(center, origin), subtract(center, origin))
            if all(dot(subtract(points[i], center), subtract(points[i], center)) <= radius for i in label):
                candidates.append((radius, center, support))
    require(bool(candidates), 'miniball_candidate')
    radius = min(row[0] for row in candidates)
    winners = [row for row in candidates if row[0] == radius]
    require(len({row[1] for row in winners}) == 1, 'unique_miniball_center')
    return min(winners)


def catalogue(raw_points):
    require(1 <= len(raw_points) <= 6 and len(set(raw_points)) == len(raw_points), 'tiny_distinct_input')
    points = [tuple(Q(x) for x in point) for point in raw_points]
    labels = [label for size in range(1, len(points) + 1)
              for label in combinations(range(len(points)), size)]
    balls = {label: miniball(points, label) for label in labels}
    regular, gabriel = True, set()
    for label, (radius, center, support) in balls.items():
        powers = [dot(subtract(p, center), subtract(p, center)) - radius for p in points]
        shell = tuple(i for i, power in enumerate(powers) if power == 0)
        regular &= shell == support
        if all(powers[i] > 0 for i in range(len(points)) if i not in label):
            gabriel.add(label)
    return balls, gabriel, regular


def data(raw_points, order):
    balls, gabriel, regular = catalogue(raw_points)
    require(regular, 'global_regular_model')
    require(1 <= order <= len(raw_points), 'order_domain')
    facets = sorted(label for label in balls if len(label) == order)
    minima = [label for label in facets if label in gabriel]
    births = {label: balls[label][0] for label in facets}
    full, elementary, naive, induced = {}, {}, {}, {}
    for a, b in combinations(facets, 2):
        union = tuple(sorted(set(a) | set(b)))
        weight = balls[union][0]
        full[a, b] = weight
        if len(union) == order + 1:
            elementary[a, b] = weight
            if a in minima and b in minima and union in gabriel:
                naive[a, b] = weight
        if a in minima and b in minima:
            induced[a, b] = weight
    distance = {(a, b): (births[a] if a == b else full[tuple(sorted((a, b)))])
                for a in facets for b in facets}
    for via in facets:
        for a in facets:
            for b in facets:
                distance[a, b] = min(distance[a, b], max(distance[a, via], distance[via, b]))
    quotient = {(a, b): distance[a, b] for a, b in combinations(minima, 2)}
    anchors = {a: min(g for g in minima if distance[a, g] <= births[a]) for a in facets}
    transferred = {}
    for (a, b), weight in elementary.items():
        x, y = sorted((anchors[a], anchors[b]))
        if x != y:
            transferred[x, y] = min(weight, transferred.get((x, y), weight))
    return dict(points=raw_points, order=order, balls=balls, gabriel=gabriel, facets=facets,
                minima=minima, births=births, full=full, elementary=elementary, naive=naive,
                induced=induced, distance=distance, quotient=quotient, anchors=anchors,
                transferred=transferred)


def partition(vertices, edges, births, level, closed):
    active = [v for v in vertices if (births[v] <= level if closed else births[v] < level)]
    parent = {v: v for v in active}
    def root(v):
        while parent[v] != v:
            v = parent[v]
        return v
    for (a, b), weight in edges.items():
        if (weight <= level if closed else weight < level):
            require(a in parent and b in parent, 'edge_before_vertex_birth')
            parent[root(a)] = root(b)
    groups = {}
    for vertex in active:
        groups.setdefault(root(vertex), []).append(vertex)
    return sorted(tuple(sorted(group)) for group in groups.values())


def signature(model, graph, level, closed):
    vertices = model['facets'] if graph in ('full', 'elementary') else model['minima']
    result = []
    for component in partition(vertices, model[graph], model['births'], level, closed):
        leaves = tuple(g for g in component if g in model['minima'])
        require(bool(leaves), 'every_component_has_minimum')
        coverage = tuple(sorted({i for facet in component for i in facet}))
        result.append((leaves, coverage))
    return tuple(sorted(result))


def mismatches(model, graph):
    levels = sorted(set(model['births'].values()) | set(model['full'].values()))
    return [(level, closed) for level in levels for closed in (False, True)
            if signature(model, 'full', level, closed) != signature(model, graph, level, closed)]


def spanning_tree(model):
    parent = {g: g for g in model['minima']}
    def root(g):
        while parent[g] != g:
            g = parent[g]
        return g
    tree = {}
    for weight, a, b in sorted((w, a, b) for (a, b), w in model['quotient'].items()):
        if root(a) != root(b):
            parent[root(a)] = root(b)
            tree[a, b] = weight
    require(len(tree) == len(model['minima']) - 1, 'spanning_tree_nonvacuum')
    return tree


def events(model, graph, serial=False):
    levels = sorted(set(model['births'].values()) | set(model['full'].values()))
    result = []
    for level in levels:
        before = [leaves for leaves, _ in signature(model, graph, level, False)]
        before += [(g,) for g in model['minima'] if model['births'][g] == level]
        after = [leaves for leaves, _ in signature(model, graph, level, True)]
        for component in after:
            parents = tuple(sorted(group for group in before if set(group) <= set(component)))
            require(set(component) == {g for group in parents for g in group}, 'event_parent_cover')
            if len(parents) >= 2:
                if serial and len(parents) > 2:
                    current = parents[0]
                    for group in parents[1:]:
                        result.append((level, tuple(sorted((current, group)))))
                        current = tuple(sorted(current + group))
                else:
                    result.append((level, parents))
    return sorted(result)


SCENES = {
    'pair': [(0, 0, 0), (2, 1, 3)],
    'acute_triangle': [(0, 0, 0), (4, 0, 0), (1, 3, 1)],
    'obtuse_triangle': [(0, 0, 0), (5, 0, 0), (1, 1, 1)],
    'minimal_n4': [(1, 1, 7), (5, 2, 1), (7, 2, 2), (5, 2, 8)],
    'E5_silent_cofaces': [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)],
}


def selftest():
    checks, models, certificates = 0, 0, []
    for name, points in SCENES.items():
        for order in range(1, len(points) + 1):
            model = data(points, order)
            model['tree'] = spanning_tree(model)
            levels = sorted(set(model['births'].values()) | set(model['full'].values()))
            for graph in ('elementary', 'quotient', 'transferred', 'tree'):
                require(not mismatches(model, graph), 'corrected_graph_cuts:' + name + ':' + graph)
                require(events(model, graph) == events(model, 'full'), 'atomic_events:' + name + ':' + graph)
                checks += 2 * len(levels)
            models += 1
            certificates.append(dict(scene=name, order=order, minima=len(model['minima']),
                                     tree_edges=len(model['tree']), tested_cuts_per_graph=2 * len(levels)))
    n4 = data(SCENES['minimal_n4'], 2)
    n4['tree'] = spanning_tree(n4)
    level = Q(477, 34)
    require(n4['minima'] == [(0, 1), (0, 3), (1, 2), (2, 3)], 'n4_minima')
    expected = {(0, 1): Q(53, 4), (0, 3): Q(9, 2), (1, 2): Q(5, 4),
                (2, 3): Q(10), (1, 3): Q(49, 4), (0, 1, 3): level,
                (1, 2, 3): Q(49, 4), (0, 1, 2, 3): Q(31, 2)}
    require(all(n4['balls'][label][0] == radius for label, radius in expected.items()), 'n4_levels')
    radius, center, _ = n4['balls'][1, 3]
    intruder = tuple(Q(x) for x in SCENES['minimal_n4'][2])
    require(dot(subtract(intruder, center), subtract(intruder, center)) - radius == -2, 'BD_intruder_C')
    require((1, 3) not in n4['gabriel'] and (1, 2, 3) in n4['gabriel']
            and (0, 1, 3) in n4['gabriel'], 'direct_J1_not_silent_coface')
    require(len(signature(n4, 'full', level, False)) == 3
            and len(signature(n4, 'full', level, True)) == 1, 'true_three_parent_multifusion')
    for graph in ('naive', 'induced'):
        require(len(signature(n4, graph, level, True)) == 2, 'n4_false_split:' + graph)
    cross = [((0, 1), (1, 2)), ((0, 1), (2, 3)), ((0, 3), (1, 2)), ((0, 3), (2, 3))]
    require(all(n4['induced'][edge] == Q(31, 2) and n4['quotient'][edge] == level for edge in cross),
            'induced_delays_all_cross_paths')
    e5 = data(SCENES['E5_silent_cofaces'], 2)
    require(all(len(signature(e5, graph, Q(83886, 3563), True)) == 2 for graph in ('naive', 'induced'))
            and len(signature(e5, 'full', Q(83886, 3563), True)) == 1, 'E5_distinct_silent_counterexample')
    # n<=3 representatives exercise the cases in the separate minimality proof.
    require(all(not mismatches(data(SCENES[name], 2), graph)
                for name in ('pair', 'acute_triangle', 'obtuse_triangle') for graph in ('naive', 'induced')),
            'n_leq_3_regular_K2_models')
    killed = []
    def kill(name, check, reason):
        try:
            check()
        except ValueError as error:
            require(str(error) == reason, 'wrong_mutant_reason:' + name + ':' + str(error))
            killed.append(name)
            return
        raise ValueError('surviving_mutant:' + name)
    for graph in ('naive', 'induced'):
        kill('promote_' + graph, lambda g=graph: require(not mismatches(n4, g), 'cut_equivalence'), 'cut_equivalence')
    changed = dict(n4, early=dict(n4['quotient']))
    changed['early'][cross[0]] = Q(53, 4)
    kill('underweight_minimax', lambda: require(not mismatches(changed, 'early'), 'cut_equivalence'), 'cut_equivalence')
    changed = dict(n4, missing=dict(n4['transferred']))
    changed['missing'] = {edge: w for edge, w in changed['missing'].items() if edge not in cross}
    kill('drop_transferred_bridge', lambda: require(not mismatches(changed, 'missing'), 'cut_equivalence'), 'cut_equivalence')
    kill('serial_equal_weight_merges', lambda: require(events(n4, 'tree', True) == events(n4, 'full'), 'atomic_multifusion'),
         'atomic_multifusion')
    kill('closed_as_open', lambda: require(signature(n4, 'quotient', level, False) == signature(n4, 'full', level, True),
                                         'cut_side'), 'cut_side')
    changed = dict(n4, births=dict(n4['births']))
    changed['births'].update({g: Q(0) for g in n4['minima']})
    kill('vertices_without_births', lambda: require(signature(changed, 'quotient', Q(0), True)
        == signature(n4, 'full', Q(0), True), 'minimum_births'), 'minimum_births')
    changed = dict(n4, minima=sorted(n4['minima'] + [(1, 3)]))
    kill('nongabriel_as_minimum', lambda: require(signature(changed, 'quotient', Q(49, 4), True)
        == signature(n4, 'full', Q(49, 4), True), 'minimum_inventory'), 'minimum_inventory')
    changed = dict(n4, overlap={(a, b): max(n4['births'][a], n4['births'][b])
                   for a, b in combinations(n4['minima'], 2) if set(a) & set(b)})
    kill('point_overlap_as_adjacency', lambda: require(not mismatches(changed, 'overlap'), 'cut_equivalence'),
         'cut_equivalence')
    kill('foreign_shell_promoted_regular', lambda: data([(0, 0, 0), (2, 0, 0), (1, 1, 0)], 2),
         'global_regular_model')
    require(models == 17 and len(killed) == 10 and checks == 640, 'nonvacuum')
    return dict(schema='mhgp7-gabriel-minima-quotient-rational-v1', status='passed',
                public_status='not_claimed', engine_invoked=False, gcp_used=False,
                models=models, corrected_graph_cut_checks=checks, graphs_per_model=4,
                event_histories_compared=models * 4, fixtures=certificates, mutants_killed=killed,
                n4_counterexample=dict(points=SCENES['minimal_n4'], order=2, cut_squared='477/34',
                    minima=n4['minima'], full_closed_components=1, naive_closed_components=2,
                    minima_witness_intersection_closed_components=2, delayed_induced_merge='31/2',
                    bridge='BD attaches through direct Gabriel BCD, not a silent coface'),
                scope='bounded_independent_math_not_FULL_producer_or_sparse_complexity_qualification')


def main(argv):
    if argv != ['--selftest']:
        print(json.dumps(dict(status='invalid_arguments', public_status='not_claimed')))
        return 2
    print(json.dumps(selftest(), sort_keys=True))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except (ValueError, TypeError, KeyError, ZeroDivisionError) as error:
        print(json.dumps(dict(status='failed', reason=str(error), public_status='not_claimed'), sort_keys=True))
        sys.exit(1)
