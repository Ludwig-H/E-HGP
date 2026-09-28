"""Experimental atomic condensation/EOM on a tree of weighted facet leaves.

Explicit port of audits/b_point_hierarchy_k_20260927/eom.py (MIT), SHA-256
c7121c857010fd89b6798ec634a0f55c4abd14c1c6bc6305c551c06e4625dead.
The original file is unchanged; no HGP-old code is imported or copied.

Leaves 0..n-1 are VIRTUAL zero-radius atoms. Internal heights are merge
radii. A terminal eligible leaf consequently persists to lambda=+infinity.
This is not a claim about the positive geometric birth radius of a facet.
For complete positive coface-boundary weights, facet mass <= 1: with a
threshold > 1 no singleton facet is eligible and that issue is invisible
for nontrivial components. This generic API does not impose that bound.

Masses (including Fraction inputs) are rounded to binary64. Those dyadic
inputs are summed as integers over one common power-of-two denominator;
each subtree sum is rounded just once. Conservation of these input masses
is checked exactly, not with a fixed floating tolerance. Threshold decisions
and EOM scores remain floating diagnostics, not certified geometric mass
margins. Equal-height INTERNAL subdivisions are atomized exactly in
binary64, never by epsilon. Root selection is always forbidden; this is
the common comparison profile, not HGP-old's default root policy.
No geometry, weights-from-cofaces construction, point vote, IO or cloud.
"""
from __future__ import annotations

from collections import deque
from collections.abc import Mapping
import math
import numbers
import sys

PORTED_FROM = "morsehgp3D_v9/audits/b_point_hierarchy_k_20260927/eom.py"
PORTED_SHA256 = "c7121c857010fd89b6798ec634a0f55c4abd14c1c6bc6305c551c06e4625dead"


def _integer(value, name, lower=0):
    if isinstance(value, bool) or not isinstance(value, numbers.Integral) or value < lower:
        raise ValueError(f"{name} must be an integer >= {lower}")
    return int(value)


def _real(value, name, positive=False):
    if isinstance(value, bool) or not isinstance(value, numbers.Real):
        raise ValueError(f"{name} must be a real number, not bool")
    try:
        result = float(value)
    except (OverflowError, ValueError) as error:
        raise ValueError(f"{name} cannot be represented in binary64") from error
    if not math.isfinite(result) or result < 0 or (positive and result == 0):
        raise ValueError(f"{name} must be finite and {'positive' if positive else 'nonnegative'}")
    return result


def _sum(values, name, allow_infinity=False):
    try:
        result = math.fsum(values)
    except (OverflowError, ValueError) as error:
        raise ValueError(f"{name} sum is unrepresentable") from error
    if math.isnan(result) or result < 0 or (not allow_infinity and not math.isfinite(result)):
        raise ValueError(f"invalid {name} sum")
    return result


def _mass_float(units, denominator):
    try:
        value = units/denominator
    except OverflowError as error:
        raise ValueError("subtree mass overflows binary64") from error
    if not math.isfinite(value) or value <= 0:
        raise ValueError("subtree mass is unrepresentable")
    return value


def _validated_tree(n, children, heights, masses):
    """Ported topology validation, with positive weighted subtree masses."""
    n = _integer(n, "n_leaves", 1)
    if isinstance(masses, Mapping):
        keys = [_integer(key, "mass leaf ID") for key in masses]
        if set(keys) != set(range(n)):
            raise ValueError("masses must have exactly the leaf IDs")
        masses = [masses[i] for i in range(n)]
    else:
        try:
            masses = list(masses)
        except TypeError as error:
            raise ValueError("masses must be leaf-indexed") from error
    if len(masses) != n:
        raise ValueError("one mass per leaf required")
    leaf_mass = [_real(value, "leaf mass", positive=True) for value in masses]
    ratios = [value.as_integer_ratio() for value in leaf_mass]
    denominator = max(den for _, den in ratios)
    # Binary64 denominators are powers of two. One linear tree pass of exact
    # integer additions avoids depth-dependent rounding and descendant scans.
    units = {leaf: num*(denominator//den) for leaf,(num,den) in enumerate(ratios)}
    items = children.items() if isinstance(children, Mapping) else enumerate(children)
    tree = {}
    for raw_node, raw_children in items:
        node = _integer(raw_node, "node")
        row = [] if raw_children is None else [_integer(c, "child") for c in raw_children]
        if node < n:
            if row:
                raise ValueError("facet leaf has children")
            continue
        if len(row) < 2 or len(set(row)) != len(row):
            raise ValueError("internal nodes need >=2 distinct children")
        tree[node] = row
    nodes = set(range(n)) | tree.keys()
    height = {}
    for node in nodes:
        if isinstance(heights, Mapping):
            value = heights.get(node, 0.0 if node < n else None)
        else:
            value = heights[node] if node < len(heights) else None
        if value is None:
            raise ValueError("missing node height")
        value = _real(value, "node height")
        if node < n and value != 0:
            raise ValueError("this profile requires virtual zero-radius leaves")
        height[node] = value
    parent = {}
    for node, row in tree.items():
        for child in row:
            if child not in nodes or child in parent or child == node:
                raise ValueError("missing child, multiple parent, or self cycle")
            if height[child] > height[node]:
                raise ValueError("merge radii decrease toward root")
            parent[child] = node
    roots = nodes - parent.keys()
    if len(roots) != 1:
        raise ValueError("exactly one root required; no implicit infinite root")
    root = next(iter(roots))
    order, seen, stack = [], set(), [root]
    while stack:
        node = stack.pop()
        if node in seen:
            raise ValueError("cycle in tree")
        seen.add(node)
        order.append(node)
        stack.extend(tree.get(node, ()))
    if seen != nodes:
        raise ValueError("disconnected/cyclic nodes")
    weight, minimum = {}, {}
    for node in reversed(order):
        if node < n:
            weight[node], minimum[node] = leaf_mass[node], node
        else:
            minimum[node] = min(minimum[c] for c in tree[node])
            tree[node].sort(key=minimum.__getitem__)
            units[node] = sum(units[c] for c in tree[node])
            weight[node] = _mass_float(units[node],denominator)
    return n, tree, height, root, weight, minimum, leaf_mass, units, denominator


def _atomize(tree, height, root):
    """Drop only equal-height internal subdivisions, never leaves (linear DFS)."""
    if root not in tree:
        return {}, 0
    out, stack, dropped = {root: []}, [(c, root) for c in reversed(tree[root])], 0
    while stack:
        node, owner = stack.pop()
        if node in tree and height[node] == height[owner]:
            dropped += 1
            stack.extend((c, owner) for c in reversed(tree[node]))
        else:
            out[owner].append(node)
            if node in tree:
                out[node] = []
                stack.extend((c, node) for c in reversed(tree[node]))
    return out, dropped


def weighted_condense_eom(n_leaves, children, heights, masses, *, min_cluster_size,
                          exp_z=1, allow_single_cluster=False):
    """Condense by weighted mass, then maximize EOM over non-root clusters.

    ``children``/``heights`` accept the old mapping or node-indexed list schema;
    leaves have no children and zero radius, internal IDs may be noncontiguous.
    Masses are a leaf-indexed sequence or complete mapping, positive and finite.
    ``min_cluster_size`` is a positive real MASS threshold, not facet count or
    final point-label cardinality. Inputs are not mutated.

    Output leaf IDs remain 0..n-1; condensed cluster IDs start at n. Every leaf
    has one dated exit, including noise. Each edge carries ``mass`` (not an
    integer size). Cluster birth masses, durations and mass-conservation
    residuals are retained. Stability [infinity,infinity] contributes zero;
    infinity versus infinity selects the parent except the excluded root.
    Near-threshold/EOM warnings report floating uncertainty, never a proof.
    Infinity is semantic; callers must encode it explicitly in strict JSON.
    """
    threshold = _real(min_cluster_size, "min_cluster_size", positive=True)
    exp_z = _integer(exp_z, "exp_z", 1)
    if exp_z not in (1, 2):
        raise ValueError("exp_z must be 1 or 2")
    if allow_single_cluster is not False:
        raise ValueError("this profile excludes root selection")
    n, original, height, root, weight, minimum, leaf_mass, mass_units, denominator = _validated_tree(
        n_leaves, children, heights, masses)
    tree, removed = _atomize(original, height, root)
    warnings = ["binary64_weighted_mass_threshold_and_EOM_not_certified",
                "virtual_zero_radius_leaf_terminal_lambda_infinity"]
    near_threshold = []
    for node in sorted(weight):
        if abs(weight[node]-threshold) <= 64 * max(math.ulp(weight[node]), math.ulp(threshold)):
            near_threshold.append(node)
            warnings.append(f"near_or_equal_float_mass_threshold:{node}")
    if any(height[node] == 0 for node in original):
        warnings.append("zero_radius_lambda_infinity_symbolic_zero_duration")

    def lam(radius):
        if radius == 0:
            return math.inf
        try:
            result = (1.0 / radius) ** exp_z
        except OverflowError as error:
            raise ValueError("lambda overflows for a positive radius") from error
        if not math.isfinite(result) or result <= 0:
            raise ValueError("lambda under/overflows for a positive radius")
        return result

    def leaves(node):
        stack = [node]
        while stack:
            child = stack.pop()
            if child < n:
                yield child
            else:
                stack.extend(reversed(tree[child]))

    birth, cluster_parent, origin, cluster_children = {n: 0.0}, {}, {n: root}, {n: []}
    cluster_mass = {n: weight[root]}
    leaf_parent, leaf_exit = [-1]*n, [0.0]*n
    edges, queue = [], deque([(root, n)])

    def exit_leaf(leaf, cluster, at):
        if leaf_parent[leaf] != -1:
            raise ValueError("leaf exits more than once")
        leaf_parent[leaf], leaf_exit[leaf] = cluster, at
        edges.append(dict(parent=cluster, child=leaf, lambda_value=at, mass=leaf_mass[leaf]))

    while queue:
        node, cluster = queue.popleft()
        if node < n:
            exit_leaf(node, cluster, math.inf)
            continue
        split = lam(height[node])
        large = [c for c in tree[node] if weight[c] >= threshold]
        for child in tree[node]:
            if weight[child] < threshold:
                for leaf in leaves(child):
                    exit_leaf(leaf, cluster, split)
            elif len(large) == 1:
                queue.append((child, cluster))
            else:
                new = n + len(birth)
                birth[new], cluster_parent[new], origin[new] = split, cluster, child
                cluster_children[new], cluster_mass[new] = [], weight[child]
                cluster_children[cluster].append(new)
                edges.append(dict(parent=cluster, child=new, lambda_value=split, mass=weight[child]))
                queue.append((child, new))
    if any(parent < n for parent in leaf_parent):
        raise ValueError("leaf coverage incomplete")

    contributions = {cluster: [] for cluster in birth}
    outgoing_units = {cluster: 0 for cluster in birth}
    rounded_outgoing_units = {cluster: 0 for cluster in birth}
    for edge in edges:
        end, begin = edge['lambda_value'], birth[edge['parent']]
        duration = 0.0 if end == begin else end-begin
        if math.isnan(duration) or duration < 0:
            raise ValueError("invalid condensed lifetime")
        term = duration*edge['mass']
        if math.isfinite(duration) and (not math.isfinite(term) or (duration > 0 and term == 0)):
            raise ValueError("finite stability contribution under/overflows")
        edge['duration'] = duration
        contributions[edge['parent']].append(term)
        child = edge['child']
        source = child if child < n else origin[child]
        outgoing_units[edge['parent']] += mass_units[source]
        num, den = edge['mass'].as_integer_ratio()
        if denominator % den:
            raise ValueError("rounded subtree mass left the dyadic input lattice")
        rounded_outgoing_units[edge['parent']] += num*(denominator//den)
    residuals = {}
    for cluster, observed in outgoing_units.items():
        expected = mass_units[origin[cluster]]
        if observed != expected:
            raise ValueError("exact dyadic input mass is not conserved")
        # Diagnostic only: sums of rounded edge masses need not equal the exact
        # leaf mass. No fixed-ULP guard can justify rejecting a valid deep comb.
        residuals[cluster] = (rounded_outgoing_units[cluster]-expected)/denominator
    stability = {c: _sum(terms, "stability", allow_infinity=True) for c, terms in contributions.items()}
    best, chosen, comparisons = {}, {}, []
    for cluster in reversed(list(birth)):
        descendants = _sum((best[c] for c in cluster_children[cluster]), "EOM", allow_infinity=True)
        own = stability[cluster]
        if math.isinf(descendants) and math.isinf(own):
            warnings.append(f"infinite_eom_tie_parent_preferred_except_root:{cluster}")
        elif cluster_children[cluster] and math.isfinite(descendants) and math.isfinite(own):
            scale = max(abs(descendants), abs(own))
            if scale and abs(descendants-own) <= 64*sys.float_info.epsilon*scale:
                warnings.append(f"near_or_equal_float_eom_scores:{cluster}")
        chosen[cluster] = cluster != n and not (descendants > own)
        best[cluster] = own if chosen[cluster] else descendants
        comparisons.append(dict(cluster=cluster, own=own, descendant_best=descendants,
                                keep_parent=chosen[cluster]))
    selected, stack = [], [n]
    while stack:
        cluster = stack.pop()
        if chosen[cluster]:
            selected.append(cluster)
        else:
            stack.extend(reversed(cluster_children[cluster]))
    selected.sort(key=lambda c: minimum[origin[c]])
    labels_by_cluster = {cluster: label for label, cluster in enumerate(selected)}
    inherited = {n: -1}
    for cluster in list(birth)[1:]:
        inherited[cluster] = labels_by_cluster.get(cluster, inherited[cluster_parent[cluster]])
    labels = [inherited[parent] for parent in leaf_parent]
    return dict(schema="mhgp9_weighted_facet_eom_v1", labels=labels, leaf_labels=list(labels),
                selected=selected, selected_sources={c: origin[c] for c in selected},
                stabilities=stability, births=birth, cluster_parent=cluster_parent,
                cluster_children=cluster_children, cluster_mass_at_birth=cluster_mass,
                condensed_tree=edges, leaf_parent=leaf_parent, leaf_exit_lambda=leaf_exit,
                leaf_masses=leaf_mass, total_mass=weight[root],
                conservation=dict(leaf_count=n, leaf_exits=len(leaf_parent),
                                  exit_mass=_sum(leaf_mass, "leaf exit mass"),
                                  cluster_mass_residuals=residuals,
                                  exact_dyadic_input_mass_conserved=True,
                                  input_mass_denominator_exponent=denominator.bit_length()-1),
                eom_comparisons=comparisons, warnings=warnings,
                near_threshold_nodes=near_threshold, exp_z=exp_z, min_cluster_size=threshold,
                allow_single_cluster=False, atomize_ties=True, contracted_internal_ties=removed,
                source_root=root, leaf_terminal="virtual_zero_radius_lambda_infinity",
                certification="not_claimed", ported_from=PORTED_FROM, ported_sha256=PORTED_SHA256)
