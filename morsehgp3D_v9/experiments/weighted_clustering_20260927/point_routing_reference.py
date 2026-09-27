"""Exact sparse reference for PLAN_PARTITIONS_POINTS, not a production fit.

Scores are positive Fraction/int values for one fixed K,z. Geometry and the
scores' provenance are supplied, not recomputed. Internal equal-level nodes
are atomized. Each point uses only its incident facets and their LCA virtual
tree, never a dense point-by-node table. No EOM or statistical claim follows.

With V source nodes, I point/facet incidences and d_x incidents per point,
the reference uses O(V log V + I log V + sum(d_x log d_x) + n) index/exact
arithmetic operations and O(V log V + I + n) storage. Fraction numerators
and denominators are not constant-size words: this is NOT a bit-cost bound,
nor a bound on the geometric number of facets. Cuts cost O(n log V).
"""
from __future__ import annotations

from collections import defaultdict
from fractions import Fraction


def need(ok, message):
    if not ok:
        raise ValueError(message)


def integer(value):
    need(type(value) is int and value >= 0, 'nonnegative integer required')
    return value


def rational(value):
    if isinstance(value, dict):
        need(set(value) == {'num', 'den'}, 'rational schema')
        parts = []
        for name in ('num', 'den'):
            raw = value[name]
            if isinstance(raw, str):
                need(raw.isascii() and raw.isdecimal() and (raw == '0' or not raw.startswith('0')), 'canonical decimal')
                raw = int(raw)
            parts.append(integer(raw))
        need(parts[1] > 0, 'positive denominator')
        return Fraction(*parts)
    need(type(value) is int or isinstance(value, Fraction), 'exact Fraction/int required; floats and bool refused')
    return Fraction(value)


def topology(n_leaves, children, squared_levels, roots, leaf_birth_betas):
    children = {integer(node): [integer(child) for child in kids] for node, kids in children.items()}
    squared_levels = {integer(node): value for node, value in squared_levels.items()}
    need(set(children) == set(squared_levels) and all(node >= n_leaves for node in children), 'internal node/level domain')
    need(len(leaf_birth_betas) == n_leaves, 'one actual birth per facet')
    levels = {i: rational(value) for i, value in enumerate(leaf_birth_betas)}
    levels.update({node: rational(value) for node, value in squared_levels.items()})
    need(all(value >= 0 for value in levels.values()), 'nonnegative levels')
    parents = {}
    for node, kids in children.items():
        need(kids and len(set(kids)) == len(kids), 'nonempty distinct children')
        for child in kids:
            need(child in levels and child != node and child not in parents, 'child domain/unique parent')
            need(levels[child] <= levels[node], 'monotone actual birth levels')
            parents[child] = node
    roots = [integer(root) for root in roots]
    need(len(roots) == len(set(roots)) and set(roots) == set(levels)-set(parents), 'exact forest roots')
    visited = set(); pending = list(roots)
    while pending:
        node = pending.pop()
        need(node not in visited, 'acyclic forest')
        visited.add(node); pending.extend(children.get(node, ()))
    need(visited == set(levels), 'no rootless cycles/unreachable nodes')
    # A plateau is one event, not a choice between arbitrarily nested binary
    # pieces. Facet endpoints keep their own actual MEB birth and identity.
    collapsed = {node for node in children if node in parents and levels[node] == levels[parents[node]]}
    atomic = {}
    for node in children:
        if node in collapsed:
            continue
        kids = []; pending = list(reversed(children[node]))
        while pending:
            child = pending.pop()
            if child in collapsed:
                pending.extend(reversed(children[child]))
            else:
                kids.append(child)
        atomic[node] = kids
    levels = {node: value for node, value in levels.items() if node not in collapsed}
    parents = {child: node for node, kids in atomic.items() for child in kids}
    log = max(1, len(levels).bit_length())
    tin, tout, depth, component, up = {}, {}, {}, {}, {}
    clock = 0
    for root in roots:
        pending = [(root, False)]
        while pending:
            node, leaving = pending.pop()
            if leaving:
                tout[node] = clock
                continue
            tin[node] = clock; clock += 1
            parent = parents.get(node)
            depth[node] = 0 if parent is None else depth[parent]+1
            component[node] = root
            ancestors = [node if parent is None else parent]
            for j in range(1, log):
                ancestors.append(ancestors[j-1] if parent is None else up[ancestors[j-1]][j-1])
            up[node] = ancestors
            pending.append((node, True))
            pending.extend((child, False) for child in reversed(atomic.get(node, ())))
    return dict(children=atomic, levels=levels, roots=roots, parents=parents, tin=tin, tout=tout,
                up=up, component=component, log=log, collapsed_plateau_nodes=len(collapsed))


def ancestor(tree, a, b):
    return tree['tin'][a] <= tree['tin'][b] < tree['tout'][a]


def lca(tree, a, b):
    need(tree['component'][a] == tree['component'][b], 'no LCA across separate roots')
    if ancestor(tree, a, b):
        return a
    if ancestor(tree, b, a):
        return b
    for j in range(tree['log']-1, -1, -1):
        candidate = tree['up'][a][j]
        if not ancestor(tree, candidate, b):
            a = candidate
    return tree['up'][a][0]


def route_points(n_points, facets, scores, children, squared_levels, roots, leaf_birth_betas):
    """Return exclusive dated point attachments, with exact sparse counters.

    IDs 0..F-1 are facets. Other IDs may have gaps/arbitrary ordering.
    Fractions are retained in the result; this is not a JSON wire contract.
    Root ties and unrepresented points remain separate external singletons.
    No min_cluster_size appears: routing is fixed independently of that m.
    """
    n_points = integer(n_points)
    facets = [[integer(point) for point in facet] for facet in facets]
    F = len(facets)
    need(len(scores) == F, 'one score per facet')
    scores = [rational(value) for value in scores]
    need(all(value > 0 for value in scores), 'strictly positive exact scores')
    need(all(facet and len(set(facet)) == len(facet) and max(facet) < n_points for facet in facets), 'facet PointId domain')
    need(len({tuple(sorted(facet)) for facet in facets}) == F, 'unique facet sets')
    need(not facets or len({len(facet) for facet in facets}) == 1, 'one fixed order K')
    tree = topology(F, children, squared_levels, roots, leaf_birth_betas)
    incidents = [[] for _ in range(n_points)]
    for facet, points in enumerate(facets):
        for point in points:
            incidents[point].append(facet)
    attachments = []; virtual_total = virtual_edges = lca_calls = max_virtual = 0
    for point, leaves in enumerate(incidents):
        by_root = defaultdict(list)
        for leaf in leaves:
            by_root[tree['component'][leaf]].append(leaf)
        if not by_root:
            attachments.append(dict(point=point, node=None, beta=None, root=None, reason='uncovered'))
            continue
        root_votes = {root: sum((scores[leaf] for leaf in ids), Fraction()) for root, ids in by_root.items()}
        maximum = max(root_votes.values()); winners = [root for root, vote in root_votes.items() if vote == maximum]
        if len(winners) != 1:
            attachments.append(dict(point=point, node=None, beta=None, root=None, reason='root_tie'))
            continue
        root = winners[0]
        ordered = sorted(by_root[root], key=tree['tin'].__getitem__)
        nodes = set(ordered)
        for a, b in zip(ordered, ordered[1:]):
            nodes.add(lca(tree, a, b)); lca_calls += 1
        ordered_nodes = sorted(nodes, key=tree['tin'].__getitem__)
        kids = {node: [] for node in ordered_nodes}; stack = []
        for node in ordered_nodes:
            while stack and not ancestor(tree, stack[-1], node):
                stack.pop()
            if stack:
                kids[stack[-1]].append(node)
            stack.append(node)
        need(len(nodes) <= 2*len(ordered)-1, 'virtual tree size bound')
        virtual_total += len(nodes); max_virtual = max(max_virtual, len(nodes)); virtual_edges += len(nodes)-1
        votes = {}
        for node in reversed(ordered_nodes):
            votes[node] = scores[node] if node < F else sum((votes[child] for child in kids[node]), Fraction())
        node = ordered_nodes[0]
        while kids[node]:
            maximum = max(votes[child] for child in kids[node])
            winners = [child for child in kids[node] if votes[child] == maximum]
            if len(winners) != 1:
                break
            node = winners[0]
        reason = 'facet' if node < F else 'branch_tie'
        attachments.append(dict(point=point, node=node, beta=tree['levels'][node], root=root, reason=reason))
    return dict(schema='mhgp9_exact_sparse_point_routing_reference_v1', n_points=n_points,
        n_facets=F, tree=tree, attachments=attachments,
        statistics=dict(source_nodes=F+len(children), atomic_nodes=len(tree['levels']),
            collapsed_plateau_nodes=tree['collapsed_plateau_nodes'], incidences=sum(map(len, facets)),
            lca_queries=lca_calls, virtual_nodes=virtual_total, virtual_edges=virtual_edges,
            maximum_virtual_nodes_per_point=max_virtual, lifting_slots=len(tree['levels'])*tree['log']),
        arithmetic='Fraction_positive_scores', geometry_certified_here=False,
        scope='fixed_K_z_exclusive_routing_reference_not_EOM_or_statistical_benchmark')


def cut(result, beta, *, closed=True):
    """Total point partition; O(n log V) operations via the shared lift index."""
    beta = rational(beta)
    need(beta >= 0 and type(closed) is bool, 'nonnegative exact cut and boolean endpoint')
    admitted = (lambda value: value <= beta) if closed else (lambda value: value < beta)
    groups = defaultdict(list); tree = result['tree']
    for row in result['attachments']:
        node = row['node']
        if node is None or not admitted(row['beta']):
            group = ('point', row['point'])
        else:
            for j in range(tree['log']-1, -1, -1):
                parent = tree['up'][node][j]
                if parent != node and admitted(tree['levels'][parent]):
                    node = parent
            group = ('source', node)
        groups[group].append(row['point'])
    return sorted(groups.values())
