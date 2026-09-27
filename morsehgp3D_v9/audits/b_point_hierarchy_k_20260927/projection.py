#!/usr/bin/env python3
"""Experimental hard point hierarchies from dated coverage of ONE native T_K.

No other order, vertical map, point distance, MST, or facet multiplicity is used.
Exact Fraction levels govern the primary first-coverage projection and grafts.
Only the optional coverage-entry voting scores and EOM rendering use floats.
"""
from __future__ import annotations

from fractions import Fraction
import math
import re


def level(value):
    if not isinstance(value, dict) or set(value) != {"num", "den"}:
        raise ValueError("exact squared level requires num/den")
    if not all(isinstance(value[k], str) for k in ("num", "den")):
        raise ValueError("level integers must be decimal strings")
    if not all(re.fullmatch(r"0|[1-9][0-9]*", value[k]) for k in ("num", "den")):
        raise ValueError("noncanonical decimal integer")
    num, den = int(value["num"]), int(value["den"])
    if num < 0 or den <= 0:
        raise ValueError("nonnegative squared level and positive denominator required")
    return Fraction(num, den)


def integer(value, minimum=0):
    if type(value) is not int or value < minimum:
        raise ValueError("invalid integer")
    return value


class SourceTree:
    def __init__(self, data):
        if data.get("schema") != "mhgp9_fixed_k_export_v1":
            raise ValueError("unsupported native export schema")
        if data.get("status") != "completed":
            raise ValueError("incomplete native export")
        self.n, self.k = integer(data["point_count"], 1), integer(data["k"], 1)
        if self.k > self.n:
            raise ValueError("K exceeds point count")
        nodes = data["nodes"]
        if not nodes:
            raise ValueError("empty source tree")
        self.children, self.parent, self.birth = [], [], []
        for i, node in enumerate(nodes):
            if integer(node["id"]) != i:
                raise ValueError("source IDs must be dense and topological")
            row = [integer(c) for c in node["children"]]
            if len(row) == 1 or len(set(row)) != len(row) or any(c >= i for c in row):
                raise ValueError("invalid source children")
            self.children.append(row)
            self.parent.append(None if node["successor"] is None else integer(node["successor"]))
            self.birth.append(level(node["level"]))
        h = len(nodes)
        inverse = [None] * h
        for p, row in enumerate(self.children):
            for c in row:
                if inverse[c] is not None or self.birth[c] >= self.birth[p]:
                    raise ValueError("multiple parent or non-strict source births")
                inverse[c] = p
        if inverse != self.parent:
            raise ValueError("successors differ from inverse child links")
        roots = [u for u in range(h) if self.parent[u] is None]
        if roots != data["roots"] or len(roots) != 1:
            raise ValueError("complete finite connected source required; no synthetic infinity")
        self.root = roots[0]
        self.depth, self.tin, self.tout = [0] * h, [0] * h, [0] * h
        clock, stack = 0, [(self.root, False)]
        while stack:
            u, done = stack.pop()
            if done:
                self.tout[u] = clock
                continue
            self.tin[u] = clock
            clock += 1
            stack.append((u, True))
            for c in reversed(self.children[u]):
                self.depth[c] = self.depth[u] + 1
                stack.append((c, False))
        if clock != h:
            raise ValueError("source tree incomplete")
        self.up = [[p if p is not None else self.root for p in self.parent]]
        for _ in range(max(self.depth).bit_length()):
            previous = self.up[-1]
            self.up.append([previous[previous[u]] for u in range(h)])
        self.entries = [dict() for _ in range(self.n)]
        self.incidence_count = 0
        populations = data["populations"]
        for row in populations:
            ids = row["interior"] + row["shell"]
            if len(row["shell"]) > 16 or len(set(ids)) != len(ids):
                raise ValueError("invalid population shell size or repeated point")
            if any(integer(x) >= self.n for x in ids):
                raise ValueError("population point outside domain")
        # Coverage is a SET: repeated records/population descriptions never vote twice.
        for contribution in data["contributions"]:
            u, population = integer(contribution["segment"]), integer(contribution["population"])
            if u >= h or population >= len(populations):
                raise ValueError("invalid contribution reference")
            at = level(contribution["level"])
            p = self.parent[u]
            if at < self.birth[u] or (p is not None and at >= self.birth[p]):
                raise ValueError("contribution outside closed-open source segment")
            row = populations[population]
            shell, interior = row["shell"], row["interior"]
            if type(contribution["include_interior"]) is not bool:
                raise ValueError("invalid interior flag")
            mask = integer(contribution["shell_mask"])
            if mask >> len(shell):
                raise ValueError("shell mask outside population")
            points = list(interior) if contribution["include_interior"] else []
            points.extend(x for j, x in enumerate(shell) if mask & (1 << j))
            if not points or len(set(points)) != len(points):
                raise ValueError("empty or internally repeated contribution")
            for x in points:
                x = integer(x)
                if x >= self.n:
                    raise ValueError("point outside domain")
                old = self.entries[x].get(u)
                if old is None or at < old:
                    self.entries[x][u] = at
                self.incidence_count += 1
        if any(not entries for entries in self.entries):
            raise ValueError("complete point coverage missing")

    def ancestor(self, a, b):
        return self.tin[a] <= self.tin[b] < self.tout[a]

    def lca(self, a, b):
        if self.ancestor(a, b):
            return a
        if self.ancestor(b, a):
            return b
        for row in reversed(self.up):
            candidate = row[a]
            if not self.ancestor(candidate, b):
                a = candidate
        return self.up[0][a]

    def first_coverage(self):
        anchors, ties, delayed = [], 0, 0
        for entries in self.entries:
            at = min(entries.values())
            first = [u for u, entry in entries.items() if entry == at]
            u = first[0]
            for other in first[1:]:
                u = self.lca(u, other)
            activation = max(at, self.birth[u])
            ties += len(first) > 1
            delayed += activation > at
            anchors.append((u, activation))
        return anchors, dict(first_appearance_ties=ties, delayed_until_lca=delayed,
                             decision_arithmetic="exact_rational_levels_and_LCA")

    def entry_vote(self, exp_z):
        """Greedy coherent vote on irredundant COVERAGE entries, not facet votes.

        Virtual trees avoid traversing all source ancestors per point. Equal or
        numerically unresolved maxima stay at the LCA of the competing branches.
        This variant is explicitly floating diagnostic, not an exact vote oracle.
        """
        if type(exp_z) is not int or exp_z not in (1, 2):
            raise ValueError("exp_z must be 1 or 2")
        anchors, atoms_total, ties, near, all_entries = [], 0, 0, 0, 0
        for entries in self.entries:
            ordered = sorted(entries, key=self.tin.__getitem__)
            # A source ancestor entry is redundant once a descendant covers x.
            atoms = [u for i, u in enumerate(ordered)
                     if i + 1 == len(ordered) or not self.ancestor(u, ordered[i + 1])]
            all_entries += len(entries)
            atoms_total += len(atoms)
            virtual = set(atoms)
            virtual.update(self.lca(a, b) for a, b in zip(atoms, atoms[1:]))
            virtual = sorted(virtual, key=self.tin.__getitem__)
            children = {u: [] for u in virtual}
            stack = []
            for u in virtual:
                while stack and not self.ancestor(stack[-1], u):
                    stack.pop()
                if stack:
                    children[stack[-1]].append(u)
                stack.append(u)
            score = {}
            for u in reversed(virtual):
                if children[u]:
                    score[u] = math.fsum(score[c] for c in children[u])
                else:
                    a = float(entries[u])
                    if not math.isfinite(a) or (entries[u] > 0 and a == 0):
                        raise ValueError("unrepresentable positive vote squared radius")
                    score[u] = math.inf if a == 0 else (1.0 / math.sqrt(a)) ** exp_z
                    if a > 0 and (not math.isfinite(score[u]) or score[u] <= 0):
                        raise ValueError("positive vote weight over/underflows")
            u = virtual[0]
            while children[u]:
                maximum = max(score[c] for c in children[u])
                candidates = [c for c in children[u] if score[c] == maximum or
                              (math.isfinite(maximum) and maximum - score[c] <=
                               64 * math.ulp(1.0) * maximum)]
                if len(candidates) != 1:
                    exact_float_equal = all(score[c] == maximum for c in candidates)
                    ties += exact_float_equal
                    near += not exact_float_equal
                    common = candidates[0]
                    for c in candidates[1:]:
                        common = self.lca(common, c)
                    u = common
                    break
                u = candidates[0]
            anchors.append((u, entries[u] if u in atoms else self.birth[u]))
        return anchors, dict(entry_atoms=atoms_total, point_segment_entries=all_entries,
                             float_equal_competitions=ties, unresolved_near_competitions=near,
                             decision_arithmetic="float64_conservative_stay_not_certified")

    def graft(self, anchors):
        """Attach point singletons at actual entry dates, contract empty/unary nodes."""
        if len(anchors) != self.n:
            raise ValueError("one anchor per point required")
        direct = [[] for _ in self.children]
        for x, (u, at) in enumerate(anchors):
            if not isinstance(at, Fraction) or not 0 <= u < len(self.children):
                raise ValueError("invalid anchor")
            p = self.parent[u]
            if at < self.birth[u] or (p is not None and at >= self.birth[p]):
                raise ValueError("anchor outside its source segment")
            direct[u].append((at, x))
        children, squared, roots = {}, {}, [None] * len(self.children)

        def merge(parts, at):
            parts = [p for p in parts if p is not None]
            if not parts:
                return None
            if len(parts) == 1:
                return parts[0]
            node = self.n + len(children)
            children[node], squared[node] = parts, at
            return node

        for u in range(len(self.children)):
            groups = {}
            for at, x in direct[u]:
                groups.setdefault(at, []).append(x)
            base = [roots[c] for c in self.children[u]]
            base.extend(groups.pop(self.birth[u], []))
            current = merge(base, self.birth[u])
            for at, points in sorted(groups.items()):
                current = merge([current, *points], at)
            roots[u] = current
        if roots[self.root] is None or (self.n > 1 and len(children) == 0):
            raise ValueError("empty point tree")
        heights = {u: math.sqrt(float(at)) for u, at in squared.items()}
        # The EOM implementation uses binary64. Refuse silent collapse of distinct
        # exact levels; exact equal levels remain legitimate multifusions.
        rendered = {}
        for u, at in squared.items():
            radius = heights[u]
            if not math.isfinite(radius) or (at > 0 and radius == 0):
                raise ValueError("unrepresentable point-tree radius")
            if radius in rendered and rendered[radius] != at:
                raise ValueError("distinct exact levels collapse in float EOM rendering")
            rendered[radius] = at
        return dict(n=self.n, children=children, heights=heights,
                    squared_levels={u: dict(num=str(a.numerator), den=str(a.denominator))
                                    for u, a in squared.items()}, root=roots[self.root],
                    anchors=[dict(source_node=u, squared_activation=dict(
                        num=str(a.numerator), den=str(a.denominator))) for u, a in anchors])

    def project(self, method="first_coverage", exp_z=1):
        if method == "first_coverage":
            anchors, statistics = self.first_coverage()
        elif method == "entry_vote":
            anchors, statistics = self.entry_vote(exp_z)
        else:
            raise ValueError("unknown fixed-K projection")
        tree = self.graft(anchors)
        return dict(tree=tree, statistics=dict(statistics, source_nodes=len(self.children),
                    coverage_incidence_expansions=self.incidence_count,
                    point_internal_nodes=len(tree["children"])), method=method,
                    exp_z=exp_z, k=self.k, source="one_fixed_order_dated_set_coverage",
                    facet_weighted_thesis_votes=False, certification="not_claimed")
