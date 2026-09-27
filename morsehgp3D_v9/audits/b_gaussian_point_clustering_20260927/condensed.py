#!/usr/bin/env python3
"""Compact, validated cluster-tree view of the frozen common EOM routine.

No input point is removed. Cluster branches below min_cluster_size disappear
as *clusters*, while their individual, dated point exits remain in the output.
The same API accepts a HGP-derived point tree or an HDBSCAN point tree.
"""
from __future__ import annotations

from collections import Counter
import hashlib
import importlib.util
import math
from numbers import Integral
from pathlib import Path


EOM_PATH = Path(__file__).resolve().parent.parent / 'b_point_hierarchy_k_20260927/eom.py'
EOM_SHA256 = 'c7121c857010fd89b6798ec634a0f55c4abd14c1c6bc6305c551c06e4625dead'
SCHEMA = 'mhgp9_condensed_point_tree_v1'
_EOM = None


def _need(condition, reason):
    if not condition:
        raise ValueError(reason)


def _integer(value, name, lower=0):
    _need(isinstance(value, Integral) and not isinstance(value, bool) and value >= lower,
          name + ': integer outside domain')
    return int(value)


def _number(value, name):
    _need(type(value) in (int, float) and not math.isnan(value) and value >= 0,
          name + ': nonnegative number or positive infinity required')
    return float(value)


def _legacy():
    global _EOM
    _need(hashlib.sha256(EOM_PATH.read_bytes()).hexdigest() == EOM_SHA256, 'frozen EOM source identity')
    if _EOM is None:
        spec = importlib.util.spec_from_file_location('mhgp9_gaussian_frozen_eom', EOM_PATH)
        _need(spec is not None and spec.loader is not None, 'EOM import specification')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        _EOM = module
    return _EOM


def _duration(end, begin):
    value = 0.0 if end == begin else end-begin  # includes [inf, inf] = 0
    _need(not math.isnan(value) and value >= 0, 'negative or undefined condensed lifetime')
    return value


def _csr(rows):
    offsets, values = [0], []
    for row in rows:
        values.extend(row)
        offsets.append(len(values))
    return offsets, values


def _offsets(values, rows, entries, name):
    _need(len(values) == rows+1, name + ': offset length')
    out = [_integer(x, name) for x in values]
    _need(out[0] == 0 and out[-1] == entries and all(a <= b for a,b in zip(out,out[1:])),
          name + ': invalid CSR offsets')
    return out


def validate_condensed_tree(tree):
    """Structural/mass proof of laminarity; no all-cut materialization.

    Every point exits once, cluster parent links form one tree, and each mass
    is the disjoint sum of child birth masses plus directly exiting points.
    Event-grouped mass checks ensure non-root live branches remain >= m.
    Arrays take O(n+C); sorting each cluster's distinct outgoing dates adds
    sum O(e_c log e_c), at most O((n+C) log(n+C)), with O(n+C) storage.
    """
    _need(tree.get('schema') == SCHEMA, 'unsupported condensed schema')
    n = _integer(tree['n_points'], 'n_points', 1)
    m = _integer(tree['min_cluster_size'], 'min_cluster_size', 2)
    _need(_integer(tree['exp_z'], 'exp_z', 1) in (1,2), 'exp_z must be 1 or 2')
    _need(type(tree['root']) is int and tree['root'] == 0, 'structural root is compact ID zero')
    parent = tree['parent']; count = len(parent)
    _need(count > 0 and type(parent[0]) is int and parent[0] == -1, 'root parent')
    for name in ('birth_lambda','death_lambda','mass_at_birth','own_stability','legacy_cluster_ids'):
        _need(len(tree[name]) == count, name + ': cluster-array length')
    birth = [_number(x,'birth_lambda') for x in tree['birth_lambda']]
    death = [_number(x,'death_lambda') for x in tree['death_lambda']]
    stability = [_number(x,'own_stability') for x in tree['own_stability']]
    masses = [_integer(x,'mass_at_birth',1) for x in tree['mass_at_birth']]
    legacy = [_integer(x,'legacy_cluster_id',n) for x in tree['legacy_cluster_ids']]
    _need(legacy == list(range(n,n+count)), 'legacy condensed IDs not dense')
    _need(birth[0] == 0 and masses[0] == n, 'root birth/mass')
    children = [_integer(x,'child',1) for x in tree['children']]
    offsets = _offsets(tree['children_offsets'],count,len(children),'children')
    _need(len(children) == count-1 and len(set(children)) == count-1, 'cluster edges must form a tree')
    inverse = [-1]*count
    for c in range(count):
        row = children[offsets[c]:offsets[c+1]]
        _need(len(row) != 1, 'single surviving child must prolong, not create a cluster')
        _need(death[c] >= birth[c], 'death precedes cluster birth')
        for child in row:
            _need(c < child < count, 'topological child ID')
            inverse[child] = c
            _need(birth[child] == death[c], 'cluster children must be born at the final split')
    _need(inverse == parent, 'parent array differs from child edges')
    for c in range(1,count):
        _integer(parent[c],'parent')
        _need(masses[c] >= m and birth[c] >= birth[parent[c]], 'child birth threshold/lifetime')
    point_parent = tree['point_exit_parent']; point_lambda = tree['point_exit_lambda']
    _need(len(point_parent) == len(point_lambda) == n, 'one exit per original point required')
    point_parent = [_integer(x,'point_exit_parent') for x in point_parent]
    point_lambda = [_number(x,'point_exit_lambda') for x in point_lambda]
    point_ids = [_integer(x,'point_exit_id') for x in tree['point_exit_ids']]
    point_offsets = _offsets(tree['point_exit_offsets'],count,len(point_ids),'point exits')
    _need(len(point_ids) == n and len(set(point_ids)) == n and min(point_ids) == 0 and max(point_ids) == n-1,
          'exit CSR must contain every original point exactly once')
    for c in range(count):
        row = children[offsets[c]:offsets[c+1]]
        exits = point_ids[point_offsets[c]:point_offsets[c+1]]
        _need(all(point_parent[p] == c for p in exits), 'point exit CSR/parent disagreement')
        _need(sum(masses[child] for child in row)+len(exits) == masses[c], 'cluster mass is not conserved')
        events, terms = Counter(), []
        for child in row:
            events[birth[child]] += masses[child]
            duration = _duration(birth[child],birth[c])
            term = duration*masses[child]
            _need(not math.isfinite(duration) or math.isfinite(term), 'finite cluster stability overflows')
            terms.append(term)
        for point in exits:
            at = point_lambda[point]
            _need(birth[c] <= at <= death[c], 'point exits outside cluster lifetime')
            events[at] += 1
            terms.append(_duration(at,birth[c]))
        _need(events and max(events) == death[c], 'cluster death is not final outgoing event')
        remaining = masses[c]
        for at in sorted(events):
            remaining -= events[at]
            _need(remaining >= 0 and (c == 0 or remaining == 0 or remaining >= m),
                  'non-root live cluster falls below threshold')
        _need(remaining == 0, 'outgoing cluster mass incomplete')
        try:
            expected = math.fsum(terms)
        except OverflowError as exc:
            raise ValueError('finite stability sum overflows') from exc
        _need(expected == stability[c], 'own stability differs from lifetime/mass replay')
    # A unique rooted parent path per point gives nested cluster member sets;
    # half-open lifetimes and simultaneous event groups give disjoint live cuts.
    return dict(points=n, clusters=count, cluster_edges=count-1, point_exits=n,
                mass_conserved=True, laminar_by_unique_point_ancestry=True)


def condensed_cut(tree, lambda_value):
    """Diagnostic live cut, not an EOM selection or noise cluster.

    Birth is inclusive and point exit exclusive. At an exit event the point
    becomes inactive; all inactive points remain individually identified.
    Structural root is retained in the object even when this cut has no mass.
    Validation is followed by O(n * condensed-depth) explicit ancestry walks.
    """
    validate_condensed_tree(tree)
    at = _number(lambda_value,'lambda cut')
    groups, inactive = {}, []
    for point, terminal in enumerate(tree['point_exit_parent']):
        if at >= tree['point_exit_lambda'][point]:
            inactive.append(point)
            continue
        c = terminal
        while tree['birth_lambda'][c] > at:
            c = tree['parent'][c]
        _need(tree['birth_lambda'][c] <= at < tree['death_lambda'][c], 'live-cut lifetime inconsistency')
        groups.setdefault(c,[]).append(point)
    return dict(lambda_value=at, clusters=[dict(cluster=c,points=points) for c,points in sorted(groups.items())],
                inactive_points=inactive)


def point_clusterer_from_tree(tree, min_cluster_size, exp_z=1):
    """Shared point-tree -> cleaned cluster tree AND EOM label selection.

    Input: {n, children, heights}, with optional root and exact squared levels.
    Heights are radii, not squared radii; lambda=1/r**exp_z increases as radius
    decreases. Cluster compact IDs are separate from original point IDs.
    """
    eom = _legacy()
    n, raw, heights, source_root, sizes, _ = eom._validated_tree(tree['n'],tree['children'],tree['heights'])
    if 'root' in tree:
        _need(tree['root'] == source_root, 'declared source root differs from tree')
    atomic, contracted = eom._atomize(raw,heights,source_root)
    result = eom.condense_eom(n,raw,heights,min_cluster_size=min_cluster_size,exp_z=exp_z,
                             allow_single_cluster=False,cluster_selection_epsilon=0.0,atomize_ties=True)
    legacy_ids = list(result['births'])
    compact = {old:i for i,old in enumerate(legacy_ids)}
    count = len(legacy_ids)
    parent, masses = [-1]*count, [0]*count
    masses[0] = n
    child_rows, exit_rows = [[] for _ in legacy_ids], [[] for _ in legacy_ids]
    birth = [result['births'][old] for old in legacy_ids]
    death = list(birth)
    for edge in result['condensed_tree']:
        p, child, at, size = compact[edge['parent']],edge['child'],edge['lambda_value'],edge['size']
        death[p] = max(death[p],at)
        if child < n:
            _need(size == 1,'point mass must be one')
            exit_rows[p].append(child)
        else:
            c = compact[child]
            _need(parent[c] == -1,'duplicate condensed child edge')
            parent[c], masses[c] = p,size
            child_rows[p].append(c)
    for row in exit_rows:
        row.sort()
    child_offsets, child_ids = _csr(child_rows)
    point_offsets, point_ids = _csr(exit_rows)
    condensed = dict(schema=SCHEMA,n_points=n,root=0,min_cluster_size=result['min_cluster_size'],exp_z=result['exp_z'],
                     parent=parent,birth_lambda=birth,death_lambda=death,mass_at_birth=masses,
                     own_stability=[result['stabilities'][old] for old in legacy_ids],
                     children_offsets=child_offsets,children=child_ids,
                     point_exit_parent=[compact[c] for c in result['point_parent']],
                     point_exit_lambda=list(result['point_exit_lambda']),
                     point_exit_offsets=point_offsets,point_exit_ids=point_ids,legacy_cluster_ids=legacy_ids)
    validated = validate_condensed_tree(condensed)
    selected = [compact[c] for c in result['selected']]
    _need(0 not in selected,'structural root cannot be selected')
    chosen = {c:label for label,c in enumerate(selected)}
    inherited = [-1]*count
    for c in range(1,count):
        _need(c not in chosen or inherited[parent[c]] == -1,'selected clusters are not an antichain')
        inherited[c] = chosen.get(c,inherited[parent[c]])
    _need([inherited[c] for c in condensed['point_exit_parent']] == result['labels'], 'selection label replay')
    retained_internal = count if n > 1 else 0
    small = sum(sizes[node] < result['min_cluster_size'] for node in atomic if node != source_root)
    continuation = len(atomic)-retained_internal-small
    _need(continuation >= 0,'source/condensed internal accounting')
    stats = dict(raw_internal_nodes=len(raw),atomic_internal_nodes=len(atomic),contracted_equal_height_nodes=contracted,
                 condensed_clusters=count,condensed_nonroot_clusters=count-1,retained_source_internal_nodes=retained_internal,
                 suppressed_small_internal_nodes=small,suppressed_continuation_internal_nodes=continuation,
                 suppressed_internal_nodes=len(raw)-retained_internal,structural_root_added=int(n==1),
                 point_exits=n,points_removed_from_input=0,selected_clusters=len(selected),
                 selected_points=sum(label>=0 for label in result['labels']),noise_points=sum(label<0 for label in result['labels']))
    selection = dict(labels=list(result['labels']),selected=selected,selected_legacy_ids=list(result['selected']),
                     allow_single_cluster=False,cluster_selection_epsilon=0.0,atomize_ties=True,
                     method='shared_EOM',warnings=list(result['warnings']))
    return dict(schema='mhgp9_point_clusterer_from_tree_v1',selection=selection,condensed_tree=condensed,stats=stats,
                validation=validated,provenance=dict(eom_source=str(EOM_PATH),eom_sha256=EOM_SHA256,
                numerical_profile='binary64_postprocessing_not_certified',point_masses='unit',GCP_used=False))
