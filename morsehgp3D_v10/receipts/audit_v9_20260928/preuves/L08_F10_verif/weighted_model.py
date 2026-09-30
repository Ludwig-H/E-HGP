"""Chapter-9 facet measure on an explicitly supplied Gabriel coface catalogue.

No geometry is inferred from FULL point coverage. Coface radii are supplied as
exact squared rationals; incidences are accumulated BEFORE the connectivity
quotient. This is a CPU experimental reference, not an industrial GPU path.
"""
from __future__ import annotations

from collections import defaultdict
from fractions import Fraction
from itertools import groupby
import math
import numbers


def integer(value, name, minimum=0):
    if isinstance(value, bool) or not isinstance(value, numbers.Integral) or value < minimum:
        raise ValueError(f"{name}: integer >= {minimum} required")
    return int(value)


def exact_beta(value):
    if not isinstance(value, dict) or set(value) != {"num", "den"}:
        raise ValueError("beta requires exactly num and den")
    def part(x):
        if isinstance(x, str):
            if not x.isascii() or not x.isdecimal():
                raise ValueError("unsigned decimal rational coefficient required")
            return int(x)
        return integer(x, "rational coefficient")
    numerator, denominator = part(value["num"]), part(value["den"])
    if numerator <= 0 or denominator <= 0:
        raise ValueError("positive squared radius required")
    return Fraction(numerator, denominator)


def _sum(values, rational):
    return sum(values, Fraction(0)) if rational else math.fsum(values)


class DisjointSet:
    def __init__(self, size):
        self.parent = list(range(size))
        self.size = [1] * size

    def find(self, a):
        root = a
        while self.parent[root] != root:
            root = self.parent[root]
        while a != root:
            following = self.parent[a]
            self.parent[a] = root
            a = following
        return root

    def union(self, a, b):
        a, b = self.find(a), self.find(b)
        if a == b:
            return a
        if (self.size[a], -a) < (self.size[b], -b):
            a, b = b, a
        self.parent[b] = a
        self.size[a] += self.size[b]
        return a


def build_facet_model(n_points, order_k, cofaces, *, exp_z=1, rational_z2=False):
    """Build masses and an atomic facet merge forest from a declared catalogue.

    Each coface is {vertices: K+1 distinct point IDs, beta: {num, den}}.
    Membership, geometry and completeness must be qualified by the producer.
    Duplicate cofaces are rejected, including duplicates at the same radius:
    silently deduplicating could conceal an incorrect multiplicity producer.
    Leaves have virtual radius zero, as in the historical MST. For the complete
    boundary measure m_facet <= 1, thresholds >=2 exclude isolated leaves.
    For fixed K: O(C log C) construction, O(C + F + n) storage, before EOM.
    Explicit tuple formation stores O(K^2 C + K F + n) IDs when K varies.
    No n-choose-K search; C and F themselves have no new global growth bound.
    """
    n_points = integer(n_points, "n_points", 1)
    order_k = integer(order_k, "order_k", 1)
    exp_z = integer(exp_z, "exp_z", 1)
    if exp_z not in (1, 2) or type(rational_z2) is not bool:
        raise ValueError("exp_z must be 1 or 2; rational_z2 must be bool")
    if rational_z2 and exp_z != 2:
        raise ValueError("rational arithmetic only covers exp_z=2")
    if order_k > n_points:
        raise ValueError("order exceeds point domain")
    rows, seen, all_facets = [], set(), set()
    for row in cofaces:
        if not isinstance(row, dict) or set(row) != {"vertices", "beta"}:
            raise ValueError("coface requires exactly vertices and beta")
        vertices = tuple(sorted(integer(x, "point ID") for x in row["vertices"]))
        if len(vertices) != order_k + 1 or len(set(vertices)) != len(vertices):
            raise ValueError("coface cardinality or duplicate vertex")
        if vertices[-1] >= n_points or vertices in seen:
            raise ValueError("coface outside point domain or repeated coface")
        seen.add(vertices)
        beta = exact_beta(row["beta"])
        boundary = tuple(vertices[:j] + vertices[j + 1:] for j in range(len(vertices)))
        all_facets.update(boundary)
        rows.append((beta, vertices, boundary))
    rows.sort()
    facets = sorted(all_facets)
    ids = {facet: j for j, facet in enumerate(facets)}
    terms = [[] for _ in facets]
    edges = []
    for beta, _, boundary in rows:
        weight = 1 / beta if rational_z2 else (
            1.0 / math.sqrt(float(beta)) if exp_z == 1 else 1.0 / float(beta))
        if weight <= 0 or (not rational_z2 and not math.isfinite(weight)):
            raise ValueError("coface weight is not representable and positive")
        edge = [ids[facet] for facet in boundary]
        edges.append((beta, edge))
        for facet_id in edge:
            terms[facet_id].append(weight)
    scores = [_sum(row, rational_z2) for row in terms]
    point_terms = [[] for _ in range(n_points)]
    for facet, score in zip(facets, scores):
        for point in facet:
            point_terms[point].append(score)
    totals = [_sum(row, rational_z2) for row in point_terms]
    masses = [_sum((score / totals[x] for x in facet), rational_z2)
              for facet, score in zip(facets, scores)]
    if rational_z2:
        if any(m <= 0 or m > 1 for m in masses):
            raise ValueError("complete-boundary facet mass invariant")
        if sum(masses, Fraction(0)) != sum(t > 0 for t in totals):
            raise ValueError("global mass conservation")
    else:
        if any(not math.isfinite(m) or m <= 0 or m > 1 + 1e-12 for m in masses):
            raise ValueError("floating complete-boundary facet mass check")
        if not math.isclose(math.fsum(masses), sum(t > 0 for t in totals), rel_tol=1e-12, abs_tol=1e-12):
            raise ValueError("floating global mass conservation")

    forest = DisjointSet(len(facets))
    owner = list(range(len(facets)))
    children, levels = {}, {}
    redundant = 0
    for beta, batch_iter in groupby(edges, key=lambda row: row[0]):
        batch = [edge for _, edge in batch_iter]
        previous = {forest.find(f): owner[forest.find(f)] for edge in batch for f in edge}
        for edge in batch:
            roots = {forest.find(f) for f in edge}
            redundant += len(roots) == 1
            for f in edge[1:]:
                forest.union(edge[0], f)
        groups = defaultdict(list)
        for old_root, node in previous.items():
            groups[forest.find(old_root)].append(node)
        for root, nodes in sorted(groups.items(), key=lambda item: min(item[1])):
            if len(nodes) < 2:
                continue
            node = len(facets) + len(children)
            children[node] = sorted(nodes)
            levels[node] = beta
            owner[root] = node
    roots = sorted(owner[j] for j in range(len(facets)) if forest.find(j) == j)
    heights = {node: math.sqrt(float(beta)) for node, beta in levels.items()}
    # Do not silently turn distinct exact levels into an artificial plateau.
    represented = {}
    for node, beta in levels.items():
        height = heights[node]
        if not math.isfinite(height) or height <= 0:
            raise ValueError("positive merge radius not representable")
        if height in represented and represented[height] != beta:
            raise ValueError("distinct exact levels collide as binary64 radii")
        represented[height] = beta
    return dict(n_points=n_points, order_k=order_k, exp_z=exp_z,
                arithmetic="fraction_z2" if rational_z2 else "binary64_reference",
                facets=facets, scores=scores, point_totals=totals, masses=masses,
                children=children, heights=heights, squared_levels=levels, roots=roots,
                statistics=dict(cofaces=len(rows), facets=len(facets),
                                coface_facet_incidences=(order_k + 1) * len(rows),
                                facet_point_incidences=order_k * len(facets),
                                covered_points=sum(t > 0 for t in totals),
                                redundant_connectivity_cofaces=redundant,
                                internal_nodes=len(children)),
                scope="declared_coface_catalogue_not_inferred_from_FULL_coverage")


def vote_points(model, facet_labels):
    """Historical post-selection vote; noise facets do not compete.

    Return sparse normalized memberships WITHOUT renormalizing the surviving
    clusters, plus a hard argmax. Exact ties choose the smallest cluster ID.
    """
    if len(facet_labels) != len(model["facets"]):
        raise ValueError("one label per facet required")
    rational = model["arithmetic"] == "fraction_z2"
    votes = [defaultdict(list) for _ in range(model["n_points"])]
    for facet, score, label in zip(model["facets"], model["scores"], facet_labels):
        if isinstance(label, bool) or not isinstance(label, numbers.Integral) or label < -1:
            raise ValueError("facet label must be -1 or nonnegative integer")
        if label < 0:
            continue
        for point in facet:
            votes[point][int(label)].append(score)
    labels, memberships, margins, raw_margins = [], [], [], []
    for point, groups in enumerate(votes):
        numerators = {label: _sum(terms, rational) for label, terms in groups.items()}
        # Common positive normalization does not change the mathematical
        # argmax, but rounding divisions CAN invent a tie. Match old's raw
        # score comparison, then normalize only the reported memberships.
        ordered = sorted(numerators, key=lambda c: (-numerators[c], c))
        summed = {label: value / model["point_totals"][point] for label, value in numerators.items()}
        labels.append(ordered[0] if ordered else -1)
        memberships.append(summed)
        margin = numerators[ordered[0]] - (numerators[ordered[1]] if len(ordered) > 1 else 0) if ordered else 0
        raw_margins.append(margin)
        margins.append(margin / model["point_totals"][point] if ordered else 0)
    return dict(labels=labels, memberships=memberships, margins=margins, raw_margins=raw_margins,
                noise_policy="ignore_noise_facets_no_1NN_fill",
                tie_policy="smallest_cluster_id", nested_partition_claim=False)


def cluster_cofaces(n_points, order_k, cofaces, *, min_cluster_size=20, exp_z=1):
    """Facet measure, weighted condensation/EOM, THEN point vote.

    Experimental faithful architecture, not a reproduction of old float32,
    its backend catalogue, or its selectable-root default. Statistical scores
    and EOM decisions remain floating point. min_cluster_size >=2 is the
    declared nontrivial-component profile, not a geometry search truncation.
    """
    if isinstance(min_cluster_size, bool) or not isinstance(min_cluster_size, numbers.Real):
        raise ValueError("real min_cluster_size >=2 required")
    if not math.isfinite(float(min_cluster_size)) or min_cluster_size < 2:
        raise ValueError("nontrivial-component profile requires min_cluster_size >=2")
    model = build_facet_model(n_points, order_k, cofaces, exp_z=exp_z)
    if not model["facets"]:
        return dict(model=model, selection=None, vote=vote_points(model, []))
    if len(model["roots"]) != 1:
        raise ValueError("disconnected facet catalogue: forest selection policy not qualified")
    from weighted_eom import weighted_condense_eom
    selection = weighted_condense_eom(len(model["facets"]), model["children"],
                                     model["heights"], model["masses"],
                                     min_cluster_size=min_cluster_size, exp_z=exp_z)
    return dict(model=model, selection=selection, vote=vote_points(model, selection["labels"]))
