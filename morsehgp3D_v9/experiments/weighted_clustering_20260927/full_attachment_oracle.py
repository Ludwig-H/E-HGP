"""Tiny exact Cech-connectivity oracle with the unchanged Gabriel facet measure.

This is NOT the production adapter. It enumerates all K and K+1 subsets of
at most 12 points and rebuilds graph components at every critical level.
No native geometry, FULL resolver or Gabriel-only connectivity is trusted.
The previous, pinned qualify_geometry.py supplies only its Fraction MEB oracle.
"""
from __future__ import annotations

from collections import defaultdict
from fractions import Fraction
from itertools import combinations
import math

from qualify_geometry import Oracle, need


E5 = ((0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2))


def _level(value):
    need(not isinstance(value, bool) and isinstance(value, (int, Fraction)),
         'oracle cut must be an exact integer/Fraction squared radius')
    value = Fraction(value)
    need(value >= 0, 'nonnegative cut')
    return value


def _components(facet_births, cofaces, beta, closed):
    """Fresh adjacency/BFS, not the product's Kruskal or FULL construction."""
    admitted = lambda value: value <= beta if closed else value < beta
    adjacent = {face: set() for face, birth in facet_births.items() if admitted(birth)}
    k = len(next(iter(facet_births)))
    for vertices, level in cofaces.items():
        if not admitted(level):
            continue
        boundary = list(combinations(vertices, k))
        need(all(face in adjacent for face in boundary), 'Cech face precedes its coface')
        # Full clique intentionally differs from the production star/DSU.
        for a, b in combinations(boundary, 2):
            adjacent[a].add(b)
            adjacent[b].add(a)
    result, unseen = [], set(adjacent)
    while unseen:
        seed = min(unseen)
        unseen.remove(seed)
        found, stack = {seed}, [seed]
        while stack:
            face = stack.pop()
            for other in adjacent[face]:
                if other in unseen:
                    unseen.remove(other)
                    found.add(other)
                    stack.append(other)
        result.append(tuple(sorted(found)))
    return sorted(result)


def _measure(n_points, k, cofaces, z, rational_z2):
    """Exactly the section-9.1 measure; connectivity never enters these sums."""
    faces = sorted({f for v in cofaces for f in combinations(v, k)})
    face_id = {f: i for i, f in enumerate(faces)}
    terms = [[] for _ in faces]
    for vertices, beta in sorted(cofaces.items(), key=lambda row: (row[1], row[0])):
        weight = 1/beta if rational_z2 else (
            1/math.sqrt(float(beta)) if z == 1 else 1/float(beta))
        for face in combinations(vertices, k):
            terms[face_id[face]].append(weight)
    summation = (lambda values: sum(values, Fraction(0))) if rational_z2 else math.fsum
    scores = [summation(row) for row in terms]
    totals = [summation(score for face, score in zip(faces, scores) if x in face)
              for x in range(n_points)]
    masses = [summation(score/totals[x] for x in face) for face, score in zip(faces, scores)]
    need(all(m > 0 for m in masses), 'positive facet masses')
    if rational_z2:
        need(all(m <= 1 for m in masses), 'complete-boundary face mass <=1')
        need(sum(masses, Fraction(0)) == sum(t > 0 for t in totals), 'total covered-point mass')
    return dict(facets=faces, scores=scores, point_totals=totals, masses=masses,
                arithmetic='fraction_z2' if rational_z2 else 'binary64_reference')


def build_reference(points, order_k, *, exp_z=2, rational_z2=None):
    """Return the exact dated Cech tree restricted to positive Gabriel facets.

    Geometric leaves are facets born at their MEB squared radius. Internal
    nodes include silent leaf attachments to an existing weighted component,
    even when no FULL topology node or new covered point is created.
    Unweighted Cech facets can transmit connectivity and are never deleted
    before the full-component calculation. Empty weighted portions are omitted
    only AFTER that calculation. No virtual global merger is introduced.

    Fractions in the returned structure are intentional; this is an in-memory
    oracle API, not a new JSON interchange format.
    """
    need(type(order_k) is int and 1 <= order_k <= len(points), 'valid order')
    need(type(exp_z) is int and exp_z in (1, 2), 'z=1 or z=2')
    if rational_z2 is None:
        rational_z2 = exp_z == 2
    need(type(rational_z2) is bool and (not rational_z2 or exp_z == 2), 'rational arithmetic requires z2')
    points = tuple(map(tuple, points))
    oracle = Oracle(points)  # hard tiny-cloud boundary, no production fallback
    all_cofaces, gabriel = oracle.cofaces(order_k)
    births = {face: oracle.meb(face)[1] for face in combinations(range(len(points)), order_k)}
    measure = _measure(len(points), order_k, gabriel, exp_z, rational_z2)
    faces = measure['facets']
    face_ids = {face: i for i, face in enumerate(faces)}
    leaf_births = [births[face] for face in faces]
    first_nontrivial = [None for _ in faces]
    first_gabriel = [min(beta for vertices, beta in gabriel.items() if set(face) < set(vertices))
                     for face in faces]
    critical = sorted(set(births.values()) | set(all_cofaces.values()))
    children, node_beta, previous = {}, {i: value for i, value in enumerate(leaf_births)}, {}
    events = []
    for beta in critical:
        components = _components(births, all_cofaces, beta, True)
        following = {}
        for component in components:
            members = frozenset(face_ids[face] for face in component if face in face_ids)
            if not members:
                continue
            if len(component) > 1:
                for leaf in members:
                    if first_nontrivial[leaf] is None:
                        first_nontrivial[leaf] = beta
            previous_groups = [group for group in previous if group <= members]
            old_members = frozenset().union(*previous_groups)
            new_leaves = members-old_members
            need(all(leaf_births[leaf] == beta for leaf in new_leaves), 'new weighted leaf at exact birth')
            child_nodes = [previous[group] for group in previous_groups] + sorted(new_leaves)
            need(child_nodes, 'component has a predecessor or newborn weighted leaf')
            if len(child_nodes) == 1:
                node = child_nodes[0]
            else:
                node = len(faces)+len(children)
                children[node] = sorted(child_nodes)
                node_beta[node] = beta
                events.append(dict(node=node, beta=beta, children=tuple(children[node]),
                                   newly_born_facets=tuple(sorted(new_leaves)),
                                   previous_weighted_components=len(previous_groups)))
            following[members] = node
        need(all(any(old <= new for new in following) for old in previous), 'Cech histories never split')
        previous = following
    roots = sorted(previous.values())
    need(not faces or len(roots) == 1, 'complete Cech finite terminal connectivity')
    need(all(value is not None for value in first_nontrivial), 'every Gabriel facet eventually has a coface')
    return dict(schema='mhgp9_tiny_full_attachment_oracle_v1', points=points,
                n_points=len(points), order_k=order_k, exp_z=exp_z, **measure,
                all_facet_births=births, all_cech_cofaces=all_cofaces,
                gabriel_cofaces=gabriel, critical_betas=critical,
                leaf_birth_betas=leaf_births, first_nontrivial_betas=first_nontrivial,
                first_gabriel_incidence_betas=first_gabriel,
                children=children, squared_levels=node_beta, roots=roots, events=events,
                provenance=dict(measure='Gabriel_complete_boundary_section9_1',
                                connectivity='all_Cech_K_and_K_plus_1_subsets',
                                leaf_births='exact_MEB_squared_radius',
                                artificial_root=False, GCP_used=False,
                                scope='tiny_reference_not_production_algorithm'))


def reference_cut(reference, beta, *, closed=True):
    """All Cech components, including unweighted ones and isolated facets."""
    beta = _level(beta)
    need(type(closed) is bool, 'closed must be bool')
    face_ids = {face: i for i, face in enumerate(reference['facets'])}
    result = []
    for component in _components(reference['all_facet_births'], reference['all_cech_cofaces'], beta, closed):
        selected = tuple(sorted(face_ids[f] for f in component if f in face_ids))
        result.append(dict(gamma_facets=component, facet_ids=selected,
                           coverage=tuple(sorted(set().union(*map(set, component)))),
                           mass=sum((reference['masses'][i] for i in selected), Fraction(0))))
    return result


def tree_cut(reference, beta, *, closed=True, virtual=False):
    """Partition of active positive FACET IDs, not a partition of point IDs."""
    beta = _level(beta)
    need(type(closed) is bool and type(virtual) is bool, 'boolean cut conventions')
    admitted = lambda level: level <= beta if closed else level < beta
    groups = {i: frozenset((i,)) for i,birth in enumerate(reference['leaf_birth_betas'])
              if admitted(Fraction(0) if virtual else birth)}
    for node, child_nodes in reference['children'].items():
        if not admitted(reference['squared_levels'][node]):
            continue
        need(all(child in groups for child in child_nodes), 'tree children active before admitted merger')
        groups[node] = frozenset().union(*(groups.pop(child) for child in child_nodes))
    return sorted(tuple(sorted(group)) for group in groups.values())


def virtual_tree_for_threshold(reference, min_cluster_size):
    """Adapter to current weighted_eom, only when isolated leaves cannot qualify.

    Leaf birth metadata stays in the reference. Making a leaf virtual at zero
    changes isolated cuts, but not eligible histories when m > max leaf mass.
    All INTERNAL attachment and merge levels, including silent ones, are kept.
    Exact beta topology is retained separately; binary64 height collisions are
    refused. EOM itself remains a floating diagnostic.
    """
    need(not isinstance(min_cluster_size, bool), 'mass threshold is not bool')
    threshold = float(min_cluster_size)
    need(math.isfinite(threshold) and threshold > 0, 'positive finite mass threshold')
    need(reference['facets'] and len(reference['roots']) == 1, 'nonempty real-root tree')
    need(all(min_cluster_size > mass for mass in reference['masses']),
         'virtual-leaf shortcut requires threshold strictly above every facet mass')
    heights, represented = {}, {}
    for node in reference['children']:
        beta = reference['squared_levels'][node]
        radius = math.sqrt(float(beta))
        need(math.isfinite(radius) and radius > 0, 'finite positive represented radius')
        need(radius not in represented or represented[radius] == beta, 'distinct exact levels collide as float radii')
        heights[node] = radius
        represented[radius] = beta
    return dict(n_leaves=len(reference['facets']), children=reference['children'],
                heights=heights, masses=reference['masses'],
                min_cluster_size=min_cluster_size, exp_z=reference['exp_z'],
                allow_single_cluster=False)
