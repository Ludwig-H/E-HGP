"""Explicit point-tree adapter to the frozen, common cardinality EOM.

This is a separate statistical projection, not HGP-old's facet-mass EOM.
The exact squared levels are retained; only this selection uses binary64.
The first adapter accepts one real root, never invents a root for a forest.
No geometry, point routing, label fitting, IO writes, or cloud operations.
"""
from __future__ import annotations

from collections import Counter
from fractions import Fraction
import hashlib
import importlib.util
import math
from pathlib import Path


V9 = Path(__file__).resolve().parents[2]
COMMON_PATH = V9 / 'audits/b_gaussian_point_clustering_20260927/condensed.py'
COMMON_SHA = 'f4323c60696255a52ef53884ce94ec5701fdda21e66fa71f927f7dc833917c8e'
EOM_PATH = V9 / 'audits/b_point_hierarchy_k_20260927/eom.py'
EOM_SHA = 'c7121c857010fd89b6798ec634a0f55c4abd14c1c6bc6305c551c06e4625dead'
SCHEMA = 'mhgp9_routed_point_eom_reference_v1'
_COMMON = None


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def integer(value, name, minimum=0):
    need(type(value) is int and value >= minimum, name + ': integer required')
    return value


def beta_value(value):
    if isinstance(value, dict):
        need(set(value) == {'num', 'den'}, 'squared level rational schema')
        parts = []
        for name in ('num', 'den'):
            raw = value[name]
            if isinstance(raw, str):
                need(raw.isascii() and raw.isdecimal() and
                     (raw == '0' or not raw.startswith('0')), 'canonical rational decimal')
                raw = int(raw)
            parts.append(integer(raw, 'rational coefficient'))
        need(parts[1] > 0, 'positive rational denominator')
        value = Fraction(*parts)
    need(type(value) is int or isinstance(value, Fraction),
         'exact squared levels required, not binary64 or bool')
    value = Fraction(value)
    need(value >= 0, 'nonnegative squared levels required')
    return value


def common_module():
    global _COMMON
    for path, expected in ((COMMON_PATH, COMMON_SHA), (EOM_PATH, EOM_SHA)):
        need(hashlib.sha256(path.read_bytes()).hexdigest() == expected,
             'frozen common source changed: ' + str(path))
    if _COMMON is None:
        spec = importlib.util.spec_from_file_location('mhgp9_routed_common_condensed', COMMON_PATH)
        need(spec is not None and spec.loader is not None, 'common module specification')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        _COMMON = module
    return _COMMON


def exact_levels_to_radii(levels):
    """No epsilon merging, rank distortion, or invented positive scale.

    Reject unrepresentable levels and collisions between distinct rational
    levels. Even when accepted, lambda/stability arithmetic is approximate.
    Checking collisions globally is stronger than only checking parent edges.
    """
    radii, inverse = {}, {}
    for node, beta in levels.items():
        try:
            radius = math.sqrt(float(beta))
        except (ValueError, OverflowError) as error:
            raise ValueError('squared level not representable for binary64 EOM') from error
        need(math.isfinite(radius) and (beta == 0 or radius > 0),
             'positive radius under/overflow in binary64 EOM')
        previous = inverse.setdefault(radius, beta)
        need(previous == beta, 'distinct exact levels collide as binary64 radii')
        radii[node] = radius
    ordered = sorted(inverse.items())
    need(all(a[1] < b[1] for a, b in zip(ordered, ordered[1:])),
         'binary64 radii invert exact level order')
    return radii


def cluster_point_tree(tree, *, min_cluster_size, exp_z=1):
    """Return the explicit common condensed tree plus EOM point labels.

    Input point IDs are 0..n_points-1. Internal children/level maps use integer
    keys; levels may be Fraction/int or canonical {num,den} dictionaries.
    Empty forests and multi-root forests are outside this first EOM adapter,
    although the exact point-tree producer and its cuts support both.
    The root is excluded for both HGP and the common HDBSCAN comparison.
    """
    need(isinstance(tree, dict), 'point tree dictionary required')
    n = integer(tree['n_points'], 'n_points', 1)
    minimum = integer(min_cluster_size, 'min_cluster_size', 2)
    exponent = integer(exp_z, 'exp_z', 1)
    need(exponent in (1, 2), 'exp_z must be 1 or 2')
    roots = list(tree['roots'])
    need(len(roots) == 1, 'single real root required; no artificial forest root')
    root = integer(roots[0], 'root')
    need(isinstance(tree['children'], dict) and isinstance(tree['squared_levels'], dict),
         'internal children and squared-level mappings required')
    children = {}
    for node, values in tree['children'].items():
        integer(node, 'internal node', n)
        children[node] = [integer(child, 'child') for child in values]
    levels = {integer(node, 'level node', n): beta_value(value)
              for node, value in tree['squared_levels'].items()}
    need(set(children) == set(levels), 'one exact level per internal node')
    if 'leaf_birth_betas' in tree:
        need(len(tree['leaf_birth_betas']) == n and
             all(beta_value(value) == 0 for value in tree['leaf_birth_betas']),
             'point singleton leaves must start at zero')
    for node, kids in children.items():
        for child in kids:
            need(child < n or child in levels, 'unknown child')
            need((Fraction(0) if child < n else levels[child]) <= levels[node],
                 'exact levels decrease toward parent')
    heights = exact_levels_to_radii(levels)
    result = common_module().point_clusterer_from_tree(
        dict(n=n, children=children, heights=heights, root=root), minimum, exponent)
    labels = result['selection']['labels']
    need(len(labels) == n, 'one label per original point')
    counts = Counter(label for label in labels if label >= 0)
    need(all(size >= minimum for size in counts.values()), 'selected point cluster below cardinality threshold')
    return dict(schema=SCHEMA, n_points=n, min_cluster_size=minimum, exp_z=exponent,
                result=result, source_roots=roots,
                source_squared_levels={node: dict(num=str(beta.numerator), den=str(beta.denominator))
                                       for node, beta in levels.items()},
                arithmetic='exact_input_levels_binary64_radii_lambda_and_EOM_not_certified',
                mass_policy='one_per_original_point', root_policy='single_real_root_excluded',
                exact_level_collision_policy='reject_distinct_betas_with_equal_binary64_radii',
                routing_exp_z_not_inferred=True, geometry_recomputed=False,
                provenance=dict(common_source=str(COMMON_PATH), common_sha256=COMMON_SHA,
                                eom_source=str(EOM_PATH), eom_sha256=EOM_SHA))
