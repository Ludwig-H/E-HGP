"""Materialize the exact total point forest of the frozen routing reference.

No geometry, votes, condensation or EOM are recomputed. Complexity is
O((V+n) log(V+n)) exact-arithmetic operations and O(V+n) auxiliary storage;
the input routing object's pre-existing lifting index is not copied.
"""
from __future__ import annotations

from collections import defaultdict
from fractions import Fraction


def _need(condition, message):
    if not condition:
        raise ValueError(message)


def _integer(value):
    _need(type(value) is int and value >= 0, 'nonnegative integer required')
    return value


def _rational(value):
    if isinstance(value, dict):
        _need(set(value) == {'num', 'den'}, 'rational schema')
        parts = []
        for field in ('num', 'den'):
            part = value[field]
            if isinstance(part, str):
                _need(part.isascii() and part.isdecimal() and
                      (part == '0' or not part.startswith('0')), 'canonical decimal')
                part = int(part)
            parts.append(_integer(part))
        _need(parts[1] > 0, 'positive denominator')
        value = Fraction(*parts)
    _need(type(value) is int or isinstance(value, Fraction), 'exact int/Fraction required')
    value = Fraction(value)
    _need(value >= 0, 'nonnegative level required')
    return value


def _source(routing):
    """Validate the consulted fields, without rebuilding/copying LCA caches."""
    _need(routing['schema'] == 'mhgp9_exact_sparse_point_routing_reference_v1', 'routing schema')
    n, facets = _integer(routing['n_points']), _integer(routing['n_facets'])
    source = routing['tree']
    levels = {_integer(node): _rational(beta) for node, beta in source['levels'].items()}
    children = {_integer(node): [_integer(child) for child in kids]
                for node, kids in source['children'].items()}
    _need(set(levels) == set(range(facets)) | set(children), 'source node domain')
    _need(all(node >= facets for node in children), 'source internal IDs')
    parents = {}
    for node, kids in children.items():
        _need(kids and len(kids) == len(set(kids)), 'nonempty distinct source children')
        for child in kids:
            _need(child in levels and child != node and child not in parents, 'source unique parent/domain')
            _need(levels[child] <= levels[node], 'source monotone levels')
            parents[child] = node
    supplied_parents = {_integer(child): _integer(parent) for child, parent in source['parents'].items()}
    _need(parents == supplied_parents, 'source parent consistency')
    roots = [_integer(root) for root in source['roots']]
    _need(len(roots) == len(set(roots)) and set(roots) == set(levels)-set(parents), 'source exact roots')
    order, component = [], {}
    pending = [(root, root) for root in roots]
    while pending:
        node, root = pending.pop()
        _need(node not in component, 'source cycle')
        component[node] = root; order.append(node)
        pending.extend((child, root) for child in children.get(node, ()))
    _need(len(order) == len(levels), 'source rootless cycle')
    _need(len(routing['attachments']) == n, 'one attachment per point')
    attachments = [None] * n
    for row in routing['attachments']:
        point = _integer(row['point'])
        _need(point < n and attachments[point] is None, 'attachment point domain/uniqueness')
        node, root, reason = row['node'], row['root'], row['reason']
        if node is None:
            _need(root is None and row['beta'] is None and reason in ('uncovered', 'root_tie'),
                  'external attachment')
            beta = None
        else:
            node, root, beta = _integer(node), _integer(root), _rational(row['beta'])
            _need(node in levels and beta == levels[node], 'attachment actual source date')
            _need(root == component[node], 'attachment source root')
            _need(reason == ('facet' if node < facets else 'branch_tie'), 'attachment reason/domain')
        attachments[point] = dict(point=point, source_node=node, beta=beta,
                                  source_root=root, reason=reason)
    return n, levels, parents, order, roots, attachments


def build_point_tree(routing):
    """Return n singleton leaves and exact atomic point multifusions.

    Leaves are PointIds 0..n-1, born at beta=0, including external points.
    Internal IDs n..n+M-1 are canonical by (beta, minimum descendant PointId).
    Source node IDs are a SEPARATE namespace in attachments/provenance.
    Empty and unary source classes are suppressed. All source edges with
    equal levels (including facet/parent edges) are contracted in one pass.
    The supplied topology and attachment dates are checked, not their votes
    or geometric provenance. The input is never mutated.
    """
    n, levels, source_parents, order, source_roots, attachments = _source(routing)
    representative, members, class_children = {}, defaultdict(list), defaultdict(list)
    classes = []
    for node in order:
        parent = source_parents.get(node)
        if parent is not None and levels[parent] == levels[node]:
            representative[node] = representative[parent]
        else:
            representative[node] = node; classes.append(node)
            if parent is not None:
                class_children[representative[parent]].append(node)
        members[representative[node]].append(node)
    attached = defaultdict(list); external = []
    for row in attachments:
        if row['source_node'] is None:
            external.append(row['point'])
        else:
            attached[representative[row['source_node']]].append(row['point'])

    # Each source edge/point is consumed once. No concatenation of descendant
    # lists along unary or equal-level chains; only direct output children.
    subtrees, merges, sizes, minima = {}, {}, {p: 1 for p in range(n)}, {p: p for p in range(n)}
    empty = unary = 0
    for node in reversed(classes):
        parts = list(attached.get(node, ()))
        parts.extend(subtrees[child] for child in class_children.get(node, ())
                     if subtrees[child] is not None)
        if not parts:
            subtrees[node] = None; empty += 1
        elif len(parts) == 1:
            subtrees[node] = parts[0]; unary += 1
        else:
            temporary = n + len(merges)
            merges[temporary] = dict(children=parts, beta=levels[node], source_class=node)
            sizes[temporary] = sum(sizes[child] for child in parts)
            minima[temporary] = min(minima[child] for child in parts)
            subtrees[node] = temporary
    sorted_merges = sorted(merges, key=lambda node: (merges[node]['beta'], minima[node]))
    renumber = {node: n+index for index, node in enumerate(sorted_merges)}
    mapped = lambda node: renumber[node] if node >= n else node
    children, squared_levels, merge_source_nodes = {}, {}, {}
    for temporary in sorted_merges:
        node = renumber[temporary]; row = merges[temporary]
        children[node] = [mapped(child) for child in sorted(row['children'], key=minima.__getitem__)]
        squared_levels[node] = row['beta']
        merge_source_nodes[node] = sorted(members[row['source_class']])
    roots = external + [subtrees[root] for root in source_roots if subtrees[root] is not None]
    roots = [mapped(root) for root in sorted(roots, key=minima.__getitem__)]
    parents = {child: node for node, kids in children.items() for child in kids}
    source_to_point_node = {source: (None if subtrees[representative[source]] is None
                                    else mapped(subtrees[representative[source]]))
                            for source in sorted(levels)}
    _need(len(merges) <= max(0, n-len(roots)), 'point forest output bound')
    return dict(schema='mhgp9_exact_point_dendrogram_v1', n_points=n, n_leaves=n,
        children=children, squared_levels=squared_levels, roots=roots, parents=parents,
        leaf_birth_betas=[Fraction(0)]*n,
        point_counts={mapped(node): sizes[node] for node in [*range(n), *sorted_merges]},
        attachments=attachments, source_to_point_node=source_to_point_node,
        merge_source_nodes=merge_source_nodes,
        statistics=dict(source_nodes=len(levels), source_edges=len(source_parents),
            source_plateau_classes=len(classes), collapsed_source_edges=len(levels)-len(classes),
            empty_source_classes=empty, unary_source_classes=unary,
            internal_nodes=len(merges), point_leaves=n, external_points=len(external),
            output_edges=len(parents), output_roots=len(roots)),
        arithmetic='Fraction_squared_radius', geometry_certified_here=False,
        scope='materialized_fixed_K_z_exclusive_routing_not_native_components_or_EOM')


def validate_point_tree(tree):
    """Validate a materialized Python object in O(V+n) operations; return None.

    This checks topology, atomization, counts and attachment consistency,
    not source geometry/votes. Integer-key Python representation is required;
    to_jsonable's wire dictionaries must not be passed here as a decoder.
    """
    _need(tree['schema'] == 'mhgp9_exact_point_dendrogram_v1', 'point tree schema')
    n = _integer(tree['n_points'])
    _need(_integer(tree['n_leaves']) == n, 'point leaf count')
    children = {_integer(node): [_integer(child) for child in kids]
                for node, kids in tree['children'].items()}
    levels = {_integer(node): _rational(beta) for node, beta in tree['squared_levels'].items()}
    _need(set(children) == set(levels) == set(range(n, n+len(children))), 'canonical internal domain')
    _need(len(tree['leaf_birth_betas']) == n and all(_rational(x) == 0 for x in tree['leaf_birth_betas']),
          'singleton leaf births zero')
    nodes = set(range(n+len(children))); parents = {}
    for node, kids in children.items():
        _need(len(kids) >= 2 and len(kids) == len(set(kids)), 'proper multifusion')
        for child in kids:
            _need(child in nodes and child != node and child not in parents, 'point unique parent/domain')
            _need(child < n or levels[child] < levels[node], 'atomic strictly increasing internal dates')
            parents[child] = node
    _need({_integer(child): _integer(parent) for child, parent in tree['parents'].items()} == parents,
          'point parent consistency')
    roots = [_integer(root) for root in tree['roots']]
    root_set = set(roots)
    _need(len(roots) == len(root_set) and root_set == nodes-set(parents), 'point exact roots')
    order, components = [], {}; pending = [(root, root) for root in roots]
    while pending:
        node, root = pending.pop()
        _need(node not in components, 'point cycle')
        components[node] = root; order.append(node)
        pending.extend((child, root) for child in children.get(node, ()))
    _need(len(order) == len(nodes), 'point rootless cycle')
    counts, minima, spans = {}, {}, {}
    preorder = {node: index for index, node in enumerate(order)}
    for node in reversed(order):
        kids = children.get(node)
        counts[node] = sum(counts[child] for child in kids) if kids else 1
        minima[node] = min(minima[child] for child in kids) if kids else node
        spans[node] = 1 + sum(spans[child] for child in kids) if kids else 1
        if kids:
            _need(all(minima[a] < minima[b] for a, b in zip(kids, kids[1:])), 'canonical child order')
    _need(all(minima[a] < minima[b] for a, b in zip(roots, roots[1:])), 'canonical root order')
    keys = [(levels[node], minima[node]) for node in range(n, n+len(children))]
    _need(all(a < b for a, b in zip(keys, keys[1:])), 'canonical internal numbering')
    supplied_counts = {_integer(node): _integer(value) for node, value in tree['point_counts'].items()}
    _need(counts == supplied_counts, 'point cardinalities')
    source_map = {_integer(source): (None if node is None else _integer(node))
                  for source, node in tree['source_to_point_node'].items()}
    _need(all(node is None or node in nodes for node in source_map.values()), 'source representative domain')
    _need(len(tree['attachments']) == n, 'point provenance length')
    for point, row in enumerate(tree['attachments']):
        _need(_integer(row['point']) == point, 'canonical point provenance order')
        source, root = row['source_node'], row['source_root']
        if source is None:
            _need(root is None and row['beta'] is None and row['reason'] in ('uncovered', 'root_tie')
                  and point in root_set, 'external singleton provenance')
        else:
            source, root, beta = _integer(source), _integer(root), _rational(row['beta'])
            _need(source in source_map and root in source_map and row['reason'] in ('facet', 'branch_tie'),
                  'routed provenance domain')
            node = source_map[source]
            _need(node is not None and (node == point if node < n else levels[node] == beta),
                  'routed source representative date')
            _need(preorder[node] <= preorder[point] < preorder[node]+spans[node],
                  'routed source contains point')
            _need(source_map[root] == components[point], 'routed source root representative')
            _need(point not in parents or beta <= levels[parents[point]], 'first point fusion not retroactive')
    merge_sources = {_integer(node): ids for node, ids in tree['merge_source_nodes'].items()}
    _need(set(merge_sources) == set(children), 'merge provenance domain')
    seen = set()
    for node, sources in merge_sources.items():
        ids = [_integer(source) for source in sources]
        _need(ids and all(a < b for a, b in zip(ids, ids[1:])) and not seen.intersection(ids),
              'merge source classes')
        _need(all(source_map.get(source) == node for source in ids), 'merge source representatives')
        seen.update(ids)


def cut(tree, beta, *, closed=True):
    """Total point partition of a builder-produced tree, no source index needed.

    Leaves are present even at the strict cut beta=0. Only INTERNAL events
    use the endpoint predicate. O(n log n) including sorted output, O(n)
    node visits/storage; no full paths or permanent descendant sets.
    """
    beta = _rational(beta)
    _need(type(closed) is bool, 'boolean endpoint required')
    admitted = (lambda value: value <= beta) if closed else (lambda value: value < beta)
    n, children, levels = tree['n_points'], tree['children'], tree['squared_levels']
    groups = []; pending = list(tree['roots'])
    while pending:
        node = pending.pop()
        if node < n:
            groups.append([node])
        elif admitted(levels[node]):
            group = []; descendants = [node]
            while descendants:
                child = descendants.pop()
                if child < n:
                    group.append(child)
                else:
                    descendants.extend(children[child])
            groups.append(sorted(group))
        else:
            pending.extend(children[node])
    return sorted(groups)


def to_jsonable(value):
    """Lossless JSON-ready copy; Fraction -> decimal-string {num,den}.

    Integer mapping keys become strings. This is a wire encoder, not a
    decoder or validation of subsequently edited JSON.
    """
    if isinstance(value, Fraction):
        return dict(num=str(value.numerator), den=str(value.denominator))
    if isinstance(value, dict):
        return {str(key): to_jsonable(item) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [to_jsonable(item) for item in value]
    return value
