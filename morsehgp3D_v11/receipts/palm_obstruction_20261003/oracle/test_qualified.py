#!/usr/bin/env python3
"""Bounded exact audit of the consumer, never a native qualification.

Own Gram/Fraction spheres and Gamma_k build the binary adapter fixtures. The
optional frozen A/B references are a third check, not the encoder's algorithm.
Weighted/duplicate fixtures are explicitly outside the unit-site export domain.
"""
from fractions import Fraction
from itertools import combinations
import json
from pathlib import Path
import struct
import sys

from qualified import (MAGIC, NONE, HierarchyScorer, analyse, load, members, need)

CHECKS = 0


def check(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise RuntimeError(message)


def rejected(action, message):
    try:
        action()
    except (ValueError, RuntimeError):
        check(True, message)
        return
    check(False, message)


def solve(matrix, rhs):
    n = len(rhs)
    rows = [[Fraction(v) for v in row] + [Fraction(b)]
            for row, b in zip(matrix, rhs)]
    for col in range(n):
        pivot = next((j for j in range(col, n) if rows[j][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        value = rows[col][col]
        rows[col] = [x/value for x in rows[col]]
        for j in range(n):
            if j != col:
                value = rows[j][col]
                rows[j] = [x-value*y for x, y in zip(rows[j], rows[col])]
    return [row[-1] for row in rows]


def sphere(points):
    a = points[0]
    differences = [tuple(x-y for x, y in zip(p, a)) for p in points[1:]]
    if not differences:
        return tuple(map(Fraction, a)), Fraction(0), (Fraction(1),)
    gram = [[sum(x*y for x, y in zip(v, w)) for w in differences]
            for v in differences]
    coefficients = solve(gram, [sum(x*x for x in v)/Fraction(2) for v in differences])
    if coefficients is None:
        return None
    center = tuple(Fraction(a[j])+sum(t*v[j] for t, v in zip(coefficients, differences))
                   for j in range(3))
    beta = sum((x-y)**2 for x, y in zip(center, a))
    return center, beta, tuple([1-sum(coefficients)] + coefficients)


class Oracle:
    def __init__(self, points, kmax):
        self.points, self.n, self.kmax = tuple(map(tuple, points)), len(points), kmax
        check(len(set(self.points)) == self.n, "unit-site fixture")
        self.candidates, self.critical, self.cache = [], {}, {}
        for q in range(1, min(4, self.n)+1):
            for support in combinations(range(self.n), q):
                candidate = sphere([self.points[i] for i in support])
                if candidate is None:
                    continue
                center, beta, weights = candidate
                closed = sum(1 << i for i, p in enumerate(self.points)
                             if sum((x-y)**2 for x, y in zip(center, p)) <= beta)
                self.candidates.append((beta, center, closed))
                if all(w > 0 for w in weights):
                    self.critical.setdefault((beta, center), support)
        self.candidates.sort()
        self.parts = {k: tuple(combinations(range(self.n), k))
                      for k in range(1, min(kmax+1, self.n)+1)}
        critical_levels = {Fraction(0)}
        for parts in self.parts.values():
            for part in parts:
                critical_levels.add(self.meb(part)[0])
        self.core_dates = {}
        for k in range(1, kmax+1):
            dates = []
            for p in self.points:
                distances = sorted(sum((x-y)**2 for x, y in zip(p, q)) for q in self.points)
                dates.append(Fraction(distances[k-1]))
            self.core_dates[k] = dates
            critical_levels.update(dates)
        self.levels = sorted(critical_levels | {b for b, _ in self.critical})
        self.rank = {b: i for i, b in enumerate(self.levels)}
        self.trees = {}
        for k in range(1, kmax+1):
            self.trees[k] = self.make_tree(k)

    def meb(self, part):
        part = tuple(part)
        if part not in self.cache:
            mask = sum(1 << i for i in part)
            winner = next(c for c in self.candidates if c[2] & mask == mask)
            self.cache[part] = winner[:2]
        return self.cache[part]

    def gamma(self, k, beta):
        vertices = [part for part in self.parts[k] if self.meb(part)[0] <= beta]
        parent = {part: part for part in vertices}

        def find(part):
            while parent[part] != part:
                part = parent[part]
            return part

        for coface in self.parts.get(k+1, ()):
            if self.meb(coface)[0] <= beta:
                faces = list(combinations(coface, k))
                root = find(faces[0])
                for face in faces[1:]:
                    parent[find(face)] = root
        groups = {}
        for vertex in vertices:
            groups.setdefault(find(vertex), set()).add(vertex)
        return list(groups.values())

    def make_tree(self, k):
        raw, previous, snapshots = [], {}, {}
        for beta in self.levels:
            current = {}
            for component in self.gamma(k, beta):
                predecessors = set(previous[v] for v in component if v in previous)
                if len(predecessors) == 1:
                    node = next(iter(predecessors))
                else:
                    node = len(raw)
                    children = sorted(predecessors)
                    raw.append(dict(beta=beta, children=children, parent=NONE))
                    for child in children:
                        check(raw[child]["parent"] == NONE, "Gamma no split")
                        raw[child]["parent"] = node
                for vertex in component:
                    current[vertex] = node
            snapshots[beta] = current
            previous = current
        births = [i for i, node in enumerate(raw) if not node["children"]]
        merges = [i for i, node in enumerate(raw) if node["children"]]
        numbering = {old: i for i, old in enumerate(births + merges)}
        nodes = []
        for old in births + merges:
            node = raw[old]
            nodes.append(dict(rank=self.rank[node["beta"]],
                              parent=NONE if node["parent"] == NONE else numbering[node["parent"]],
                              children=[numbering[i] for i in node["children"]]))
        root = next(i for i, node in enumerate(nodes) if node["parent"] == NONE)
        snapshots = {beta: {part: numbering[node] for part, node in mapping.items()}
                     for beta, mapping in snapshots.items()}
        records = []
        for (beta, center), support in sorted(self.critical.items()):
            inner, shell = [], []
            for i, p in enumerate(self.points):
                distance = sum((x-y)**2 for x, y in zip(center, p))
                if distance < beta:
                    inner.append(i)
                elif distance == beta:
                    shell.append(i)
            p, q, m = len(inner), len(support), len(shell)
            if p+q <= k <= p+m and (k > 1 or beta == 0):
                population = inner + shell
                part = tuple(sorted(support + tuple(i for i in population if i not in support)[:k-q]))
                check(self.meb(part) == (beta, center), "strong witness exact MEB")
                node = snapshots[beta][part]
                records.append((node, self.rank[beta], population))
        records.sort(key=lambda r: (r[1], r[0], r[2]))
        first = []
        core = []
        for i, point in enumerate(self.points):
            matches = [r for r in records if i in r[2]]
            check(bool(matches), "every site first-cover exists")
            selected = min(matches, key=lambda r: r[1])
            first.append((selected[0], selected[1]))
            beta = self.core_dates[k][i]
            near = sorted(range(self.n), key=lambda j: (
                sum((x-y)**2 for x, y in zip(point, self.points[j])), j))[:k]
            core.append((snapshots[beta][tuple(sorted(near))], int(beta)))
        return dict(nodes=nodes, births=len(births), root=root, records=records,
                    first=first, core=core, snapshots=snapshots)

    def qualified_groups(self, k, beta, m):
        cover = [set(i for part in group for i in part) for group in self.gamma(k, beta)]
        parent = list(range(self.n))
        active = set()

        def find(i):
            while parent[i] != i:
                i = parent[i]
            return i

        for edge in cover:
            if len(edge) >= m:
                active.update(edge)
                first = next(iter(edge))
                for i in edge:
                    parent[find(i)] = find(first)
        groups = {}
        for i in active:
            groups.setdefault(find(i), set()).add(i)
        return {frozenset(group) for group in groups.values()}

    def encode(self):
        out = bytearray(MAGIC)

        def word(value):
            out.extend(struct.pack("<Q", value))

        def integer(value):
            word(int(value < 0))
            value = abs(value)
            limbs = max(1, (value.bit_length()+63)//64)
            word(limbs)
            for i in range(limbs):
                word((value >> (64*i)) & ((1 << 64)-1))

        for x in (18, self.kmax, self.n, len(self.levels)):
            word(x)
        for i, p in enumerate(self.points):
            for x in (*p, i):
                word(x)
        for beta in self.levels:
            integer(beta.numerator)
            integer(beta.denominator)
        for k, tree in self.trees.items():
            nodes = tree["nodes"]
            edges = [child for node in nodes for child in node["children"]]
            for x in (k, tree["births"], len(nodes), len(edges), tree["root"]):
                word(x)
            begin = 0
            for node in nodes:
                for x in (node["parent"], node["rank"], begin, len(node["children"])):
                    word(x)
                begin += len(node["children"])
            for x in edges:
                word(x)
            for anchors in (tree["core"], tree["first"]):
                for pair in anchors:
                    for x in pair:
                        word(x)
            word(len(tree["records"]))
            for node, rank, population in tree["records"]:
                for x in (node, rank, len(population), *population):
                    word(x)
        return bytes(out)


def cut(tree, beta):
    groups = {}
    for leaf in range(tree["leaves"]):
        entry = tree["height"][leaf]
        if entry is None or Fraction(entry) > beta:
            continue
        node = leaf
        while tree["parent"][node] != NONE:
            parent = tree["parent"][node]
            if Fraction(tree["height"][parent]) > beta:
                break
            node = parent
        groups.setdefault(node, set()).add(leaf)
    return {frozenset(group) for group in groups.values()}


def check_scorer():
    scorer = HierarchyScorer([0, 0, 1, -1, -2])
    for i in range(5):
        scorer.activate(i, Fraction(0))
    scorer.observe(Fraction(0))
    scorer.union(0, 1)
    scorer.union(1, 2)
    scorer.union(2, 3)
    scorer.union(3, 4)
    scorer.observe(Fraction(1))
    answer = scorer.finish()
    # The transient perfect target {0,1} is forbidden at this simultaneous merge.
    check(answer["best_iou"]["0"]["iou_exact"] == "1/2", "atomic IoU rejects transient perfect group")
    check(len(answer["tree"]["height"]) == 6 and answer["tree"]["child_count"][-1] == 5,
          "one multifusion, no artificial binary plateau")
    check(answer["best_iou"]["1"]["iou_exact"] == "1/1", "singleton target scored at entry")
    rejected(lambda: scorer.observe(Fraction(1)), "duplicate plateau rejected")
    void = HierarchyScorer([0, 0, -2])
    for i in range(3):
        void.activate(i, Fraction(2))
    void.union(0, 1)
    void.union(1, 2)
    void.observe(Fraction(2))
    best = void.finish()["best_iou"]["0"]
    check(best["iou_exact"] == "1/1" and best["valid_size"] == 2 and best["total_size"] == 3,
          "void excluded only from IoU, retained in total cardinality")


def run_case(name, points, kmax, references=None):
    oracle = Oracle(points, kmax)
    encoded = oracle.encode()
    data = load(encoded)
    chosen = [k for k in (2, 3, 5, 10) if k <= kmax]
    labels = [i % 3 for i in range(len(points))]
    output = analyse(data, labels, chosen, (2, 3, kmax+1, 20))
    comparisons = 0
    for k in chosen:
        for text, answer in output["orders"][str(k)]["qualified"].items():
            m = int(text)
            tree = answer["tree"]
            check(len(tree["height"]) <= 2*len(points)-1, "compact laminar point tree")
            for beta in oracle.levels:
                expected = oracle.qualified_groups(k, beta, m)
                check(cut(tree, beta) == expected, name + " complete dynamic coverage")
                comparisons += 1
            best = {}
            for beta in oracle.levels:
                for group in oracle.qualified_groups(k, beta, m):
                    for target in set(labels):
                        intersection = sum(labels[i] == target for i in group)
                        union = labels.count(target)+len(group)-intersection
                        ratio = Fraction(intersection, union)
                        best[target] = max(best.get(target, Fraction(0)), ratio)
            for target in set(labels):
                check(Fraction(answer["best_iou"][str(target)]["iou_exact"]) == best.get(target, 0),
                      name + " independent brute-force IoU")
            if k == 2 and m == 2:
                for beta in oracle.levels:
                    check(cut(tree, beta) == {group for group in oracle.qualified_groups(1, beta, 1)
                                             if len(group) > 1},
                          name + " K2 m2 equals SL/2")
        full = oracle.trees[k]
        anchors = {
            "core": [(Fraction(date), node) for node, date in full["core"]],
            "first_cover_canonical": [(oracle.levels[rank], node) for node, rank in full["first"]],
        }

        def ancestors(node):
            answer = []
            while node != NONE:
                answer.append(node)
                node = full["nodes"][node]["parent"]
            return answer

        tied = []
        for site, (_first, rank) in enumerate(full["first"]):
            paths = [ancestors(node) for node, event_rank, population in full["records"]
                     if event_rank == rank and site in population]
            common = set(paths[0]).intersection(*map(set, paths[1:]))
            owner = next(node for node in paths[0] if node in common)
            date = max(oracle.levels[rank], oracle.levels[full["nodes"][owner]["rank"]])
            tied.append((date, owner))
        anchors["first_cover_lca_all_ties"] = tied
        for scheme, entries in anchors.items():
            for beta in oracle.levels:
                groups = {}
                for site, (date, owner) in enumerate(entries):
                    if date > beta:
                        continue
                    parent = full["nodes"][owner]["parent"]
                    while parent != NONE and oracle.levels[full["nodes"][parent]["rank"]] <= beta:
                        owner = parent
                        parent = full["nodes"][owner]["parent"]
                    groups.setdefault(owner, set()).add(site)
                expected = {frozenset(group) for group in groups.values()}
                check(cut(output["orders"][str(k)][scheme]["tree"], beta) == expected,
                      name + " anchored baseline dates/parents")
        if references is not None:
            definition, constructive, judge = references
            a, b = definition.order(k), constructive.order(k)
            check(not judge.compare_orders(a, b), name + " A/B agree")
            for beta in oracle.levels:
                _opened, closed = judge.cut_at(a, beta)
                expected = {frozenset(i for i in range(len(points)) if cov & (1 << i))
                            for _node, cov, _core in closed}
                actual = {frozenset(i for part in group for i in part)
                          for group in oracle.gamma(k, beta)}
                check(expected == actual, name + " independent Gamma/A/B complete cover")
    if name.startswith("two_triangles"):
        answer = output["orders"]["2"]["qualified"]["3"]
        beta = Fraction(249978000484, 187489)
        check(cut(answer["tree"], beta) == {frozenset((0, 1, 2)), frozenset((3, 4, 5))},
              name + " m3 preserves triangles at exact date")
        if name != "two_triangles":
            baseline = output["orders"]["2"]["first_cover_lca_all_ties"]
            check(cut(baseline["tree"], beta) != cut(answer["tree"], beta),
                  "first-cover-only mutant loses triangles")
    if name == "two_pairs":
        answer = output["orders"]["2"]["qualified"]["3"]
        check(cut(answer["tree"], Fraction(1)) == set(), "m3 vetoes pair clusters")
        check(cut(answer["tree"], Fraction(2500)) == {frozenset(range(4))}, "m3 pairs enter only globally")
    if name == "shared_boundary":
        beta = Fraction(100, 9)
        covers = {frozenset(i for part in group for i in part) for group in oracle.gamma(2, beta)}
        check(covers == {frozenset((0, 1, 2)), frozenset((0, 3, 4))}, "two distinct geometric owners")
        check(cut(output["orders"]["2"]["qualified"]["3"]["tree"], beta) == {frozenset(range(5))},
              "shared qualified boundary forces point percolation")
    if name.startswith("equilateral_exact"):
        scale_squared = 4 if name.endswith("scaled2") else 1
        answer = output["orders"]["2"]["qualified"]["3"]
        beta = scale_squared*Fraction(2, 3)
        check(cut(answer["tree"], beta) == {frozenset((0, 1, 2)), frozenset((3, 4, 5))},
              "exact equilateral plateau m3 preserves both triangles")
        for scheme in ("core", "first_cover_lca_all_ties"):
            check(cut(output["orders"]["2"][scheme]["tree"], beta) != cut(answer["tree"], beta),
                  "exact equilateral plateau differs from core/first-cover LCA")
    rejected(lambda: load(encoded[:-1]), "truncated population rejected")
    rejected(lambda: load(encoded + b"x"), "trailing data rejected")
    rejected(lambda: load(b"BAD" + encoded[3:]), "bad signature rejected")
    # The structural loader does not certify completeness, but does refuse an
    # impossible native association of an early strong witness to a future root.
    first_record = data.orders[2].record_offsets[0]
    corrupt = bytearray(encoded)
    struct.pack_into("<Q", corrupt, first_record, data.orders[2].root)
    root_rank = data.orders[2].rank[data.orders[2].root]
    event_rank = struct.unpack_from("<Q", encoded, first_record+8)[0]
    if root_rank > event_rank:
        rejected(lambda: load(corrupt), "future strong owner rejected")
    return dict(name=name, sites=len(points), kmax=kmax, comparisons=comparisons,
                binary_bytes=len(encoded), native_executed=False)


def main():
    check_scorer()
    frozen = Path(__file__).resolve().parent.parent / "projection" / "source"
    cases = []
    skipped = []
    if frozen.is_dir():
        sys.path.insert(0, str(frozen))
        from hgp11_ref import Definition, Reference, judge
        from hgp11_ref.families import fixtures
        for fixture in fixtures():
            if len(fixture.points) > 8 or len(set(fixture.points)) != len(fixture.points):
                skipped.append(dict(name=fixture.name, reason="unit-site n<=8 domain"))
                continue
            kmax = min(fixture.kmax, 5)
            refs = (Definition(fixture.points), Reference(fixture.points, kmax), judge)
            cases.append(run_case(fixture.name, fixture.points, kmax, refs))
    else:
        for d in (2000, 1998, 1700):
            name = "two_triangles" if d == 2000 else "two_triangles_" + str(d)
            points = [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0),
                      (2000+d, 2000, 0), (3732+d, 3000, 0), (3732+d, 1000, 0)]
            cases.append(run_case(name, points, 3))
    cases.append(run_case("two_pairs", [(x, 0, 0) for x in (0, 2, 100, 102)], 3))
    cases.append(run_case("shared_boundary", [(6, 2, 0), (0, 0, 0), (0, 4, 0),
                                               (12, 0, 0), (12, 4, 0)], 3))
    exact = [(-1, -1, 0), (-1, 0, -1), (0, 0, 0), (1, 1, 0), (2, 2, 0), (2, 1, 1)]
    for scale in (1, 2):
        points = [tuple(scale*(x+2) for x in p) for p in exact]
        name = "equilateral_exact" + ("_scaled2" if scale == 2 else "")
        refs = (Definition(points), Reference(points, 3), judge) if frozen.is_dir() else None
        cases.append(run_case(name, points, 3, refs))
    print(json.dumps(dict(status="pass", scope="bounded exact consumer/encoder, no native execution",
                          checks=CHECKS, cases=cases, skipped=skipped), sort_keys=True))


if __name__ == "__main__":
    main()
