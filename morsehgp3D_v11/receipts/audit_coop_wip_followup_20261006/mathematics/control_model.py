#!/usr/bin/env python3
"""Bounded stdlib proof of pair-subtree ordering; no product/native execution."""
import argparse
import hashlib
import itertools
import json
from fractions import Fraction
from pathlib import Path
import random
import subprocess

PIN = "3b76a3fcf0ca14dd08f005e1e1ae8e3418dd247e"
BASE = "morsehgp3D_v11/"
SOURCE_PATHS = [BASE + p for p in (
    "audits/REPONSE_CLAUDE_SUPPORTS_20261004.md",
    "src/catalogue/leaf_device.hpp",
    "src/catalogue/leaf_device_predicates.hpp",
    "src/cloud/morton.hpp",
)]
CHECKS = 0

def require(ok, message):
    global CHECKS
    if not ok:
        raise RuntimeError(message)
    CHECKS += 1

def dot(a, b):
    return sum(x * y for x, y in zip(a, b))

def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))

def morton(p):
    return sum(((x >> bit) & 1) << (3 * bit + axis)
               for axis, x in enumerate(p) for bit in range(24))

def graph(points, lo, hi, directed=False):
    """Exact closed-box dominance, unordered reference or independent row form."""
    m = len(points)
    dom, domby, nbr = ([0] * m for _ in range(3))
    pairs = itertools.permutations(range(m), 2) if directed else itertools.combinations(range(m), 2)
    for i, j in pairs:
        delta = sub(points[j], points[i])
        base = dot(points[j], points[j]) - dot(points[i], points[i])
        cmin = sum((lo[a] if d > 0 else hi[a]) * d for a, d in enumerate(delta))
        cmax = sum((hi[a] if d > 0 else lo[a]) * d for a, d in enumerate(delta))
        if base - 2 * cmin < 0:
            dom[i] |= 1 << j
            if not directed:
                domby[j] |= 1 << i
        elif base - 2 * cmax > 0:
            domby[i] |= 1 << j
            if not directed:
                dom[j] |= 1 << i
        else:
            nbr[i] |= 1 << j
            if not directed:
                nbr[j] |= 1 << i
    return dom, domby, nbr

def line_meets(a, b, c, lo, hi):
    """Integer transcription of the closed-box J2 predicate (not C++ execution)."""
    fu, gu = sub(a, b), sub(a, c)
    fc, gc = dot(a, a) - dot(b, b), dot(a, a) - dot(c, c)
    cr = [[abs(gu[i] * fu[j] - fu[i] * gu[j]) for j in range(3)] for i in range(3)]
    if not any(cr[i][j] for i in range(3) for j in range(3)):
        return False
    p0 = fc - sum((lo[k] + hi[k]) * fu[k] for k in range(3))
    p1 = gc - sum((lo[k] + hi[k]) * gu[k] for k in range(3))
    return all(abs(gu[k] * p0 - fu[k] * p1) <=
               sum((hi[j] - lo[j]) * cr[k][j] for j in range(3))
               for k in range(3) if fu[k] or gu[k])

def bits(mask):
    while mask:
        bit = mask & -mask
        yield bit.bit_length() - 1
        mask -= bit

class Traversal:
    """Control-flow model only: events are q-attempt labels, not geometric balls."""
    def __init__(self, points, lo, hi, k, cache=True, fail=None):
        self.points, self.lo, self.hi, self.k = points, lo, hi, k
        self.dom, self.domby, self.nbr = graph(points, lo, hi)
        self.live = [[0] * len(points) for _ in range(3)]
        for i in range(len(points)):
            for j in bits(self.nbr[i]):
                weight = (self.dom[i] | self.dom[j]).bit_count()
                for q in range(2, 5):
                    if weight <= k + 1 - q:
                        self.live[q - 2][i] |= 1 << j
        self.counts = dict(prefixes=0, tests=0, evaluations=0, hits=0, rejects=0)
        self.seen, self.events = set(), []
        self.cache, self.fail, self.unresolved = cache, fail, False

    def node(self, prefix, i, suffix, logical, mask):
        if self.unresolved:
            return
        p = prefix + (i,)
        q = len(p)
        self.counts["prefixes"] += 1
        mask |= self.dom[i]
        count = mask.bit_count()
        if count > self.k + 1 - q:
            return
        if q >= 3:
            for a, b in itertools.combinations(p[:-1], 2):
                triple = (a, b, i)
                self.counts["tests"] += 1
                hit = self.cache and triple in self.seen
                if self.cache:
                    self.seen.add(triple)  # linearized atomic OR, not a CUDA simulation
                self.counts["hits" if hit else "evaluations"] += 1
                if not line_meets(*(self.points[v] for v in triple), self.lo, self.hi):
                    self.counts["rejects"] += 1
                    return
        if p == self.fail:
            self.unresolved = True
            return
        if q >= 2:
            self.events.append(p)
        if q == 4 or self.k - q < 0:
            return
        next_logical = logical & ~((1 << (i + 1)) - 1) & self.nbr[i]
        if count > self.k - q:
            self.counts["prefixes"] += next_logical.bit_count()
            return
        next_mask = suffix
        for v in p:
            next_mask &= self.live[q - 1][v]
        self.counts["prefixes"] += next_logical.bit_count() - next_mask.bit_count()
        self.walk(p, next_mask, next_logical, mask)

    def walk(self, prefix, candidates, logical, mask):
        if self.k + 1 - (len(prefix) + 1) < 0:
            return
        for i in bits(candidates):
            suffix = candidates & ~((1 << (i + 1)) - 1)
            self.node(prefix, i, suffix, logical, mask)
            if self.unresolved:
                return

    def tasks(self):
        """Depth 0 bookkeeping, exact per-pair remaining candidate cursor."""
        result = []
        initial = (1 << len(self.points)) - 1
        for i in bits(initial):
            self.counts["prefixes"] += 1
            mask = self.dom[i]
            count = mask.bit_count()
            if count > self.k or self.k - 1 < 0:
                continue
            logical = initial & ~((1 << (i + 1)) - 1) & self.nbr[i]
            if count > self.k - 1:
                self.counts["prefixes"] += logical.bit_count()
                continue
            candidates = initial & ~((1 << (i + 1)) - 1) & self.live[0][i]
            self.counts["prefixes"] += logical.bit_count() - candidates.bit_count()
            for j in bits(candidates):
                suffix = candidates & ~((1 << (j + 1)) - 1)
                result.append((i, j, suffix, logical, mask))
        return result

    def play(self, tasks, order):
        per_pair = {}
        for pos in order:
            i, j, suffix, logical, mask = tasks[pos]
            start = len(self.events)
            self.node((i,), j, suffix, logical, mask)
            per_pair[pos] = self.events[start:]
            if self.unresolved:
                break
        return [event for pos in range(len(tasks)) for event in per_pair.get(pos, [])]

def barycentric(points, center):
    # Independent rational Gaussian elimination, not the C++ q4 weight formula.
    matrix = [[Fraction(p[axis]) for p in points] + [Fraction(center[axis])] for axis in range(3)]
    matrix.append([Fraction(1)] * 5)
    for col in range(4):
        pivot = next(row for row in range(col, 4) if matrix[row][col])
        matrix[col], matrix[pivot] = matrix[pivot], matrix[col]
        divisor = matrix[col][col]
        matrix[col] = [x / divisor for x in matrix[col]]
        for row in range(4):
            if row != col:
                factor = matrix[row][col]
                matrix[row] = [x - factor * y for x, y in zip(matrix[row], matrix[col])]
    return [matrix[row][-1] for row in range(4)]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", default="/workspaces/E-HGP")
    args = parser.parse_args()
    manifest = json.loads(Path(__file__).with_name("sources.json").read_text())
    require(manifest["pin"] == PIN, "pin changed")
    for path in SOURCE_PATHS:
        data = subprocess.check_output(["git", "-C", args.repo, "show", PIN + ":" + path])
        require(hashlib.sha256(data).hexdigest() == manifest["files"][path]["sha256"], "source changed: " + path)

    tetra = [(5, 2, 1), (10, 5, 5), (9, 8, 5), (1, 5, 8)]
    center, lo, hi = (5, 5, 5), (0, 0, 0), (16, 16, 16)
    require(tetra == sorted(tetra, key=morton), "fixture must use actual Morton order")
    require(all(dot(sub(p, center), sub(p, center)) == 25 for p in tetra), "common radius")
    weights = barycentric(tetra, center)
    require(weights == [Fraction(5, 18), Fraction(2, 27), Fraction(5, 18), Fraction(10, 27)], "exact weights")
    require(all(w > 0 for w in weights), "positive q4 / no proper support")
    acute_dots = [dot(sub(tetra[j], tetra[i]), sub(tetra[k], tetra[i]))
                  for i, j, k in ((0, 1, 2), (1, 0, 2), (2, 0, 1))]
    require(min(acute_dots) == -4, "first q3 is obtuse")
    require(all(line_meets(*(tetra[i] for i in triple), lo, hi)
                for triple in itertools.combinations(range(4), 3)), "J2 cannot prune q4")
    require(graph(tetra, lo, hi)[0] == [0] * 4, "no strict dominator")
    t = Traversal(tetra, lo, hi, 3)
    t.walk((), 15, 15, 0)
    require((0, 1, 2, 3) in t.events, "q3 nonemission must not stop q4 traversal")

    fixtures = [
        (tetra, lo, hi),
        ([(0, 0, 0), (1, 0, 0), (2, 0, 0), (3, 0, 0), (0, 2, 0), (0, 0, 2)], (0, 0, 0), (2, 2, 2)),
        ([(0, 0, 0), (1, 3, 0), (3, 1, 0), (0, 0, 3), (2, 2, 2), (5, 5, 5)], (1, 1, 1), (3, 3, 3)),
        ([(0, 0, 0), (0, 2, 0), (2, 0, 0), (2, 2, 0), (1, 1, 2), (1, 1, 4), (4, 2, 1)], (0, 0, 0), (8, 8, 8)),
    ]
    comparisons = 0
    cache_witness = None
    for points, box_lo, box_hi in fixtures:
        require(graph(points, box_lo, box_hi) == graph(points, box_lo, box_hi, directed=True), "directed dominance mismatch")
        for k in (1, 2, 3, 5):
            for cache in (False, True):
                serial = Traversal(points, box_lo, box_hi, k, cache)
                initial = (1 << len(points)) - 1
                serial.walk((), initial, initial, 0)
                template = Traversal(points, box_lo, box_hi, k, cache)
                tasks = template.tasks()
                normal = list(range(len(tasks)))
                shuffled = normal[:]
                random.Random(17).shuffle(shuffled)
                for order in (normal, normal[::-1], shuffled):
                    coop = Traversal(points, box_lo, box_hi, k, cache)
                    task_list = coop.tasks()
                    ordered_events = coop.play(task_list, order)
                    require(coop.counts == serial.counts, "counter mismatch across pair orders")
                    require(ordered_events == serial.events, "pair-index concatenation differs from full DFS")
                    comparisons += 1
                if cache and serial.counts["hits"] and cache_witness is None:
                    cache_witness = dict(k=k, sites=len(points), **serial.counts)
    require(cache_witness is not None, "need actual reuse witness")

    # A synthetic local refusal models transactionality only; not a geometric refusal.
    provisional = []
    for reverse in (False, True):
        refused = Traversal(tetra, lo, hi, 3, fail=(0, 1, 2))
        tasks = refused.tasks()
        order = list(range(len(tasks)))
        refused.play(tasks, order[::-1] if reverse else order)
        require(refused.unresolved, "refusal must reject whole leaf")
        provisional.append(dict(counts=refused.counts, events=len(refused.events)))
    require(provisional[0] != provisional[1], "partial refusal counts must differ to demonstrate discard")

    print(json.dumps(dict(
        pin=PIN, checks=CHECKS, pair_order_comparisons=comparisons,
        status="PASS_portable_model_only", native_executed=False,
        limits="Control-flow events are q-attempt labels, not geometric emitted balls; no GPU concurrency simulation or 15-counter oracle.",
        tetra=dict(sites_morton=tetra, morton_keys=[morton(p) for p in tetra], k=3,
                   box_lo=lo, box_hi=hi, center=center, radius_squared=25,
                   weights=[str(w) for w in weights], first_triangle_dot_products=acute_dots,
                   canonical_qmin=4, strict_interior=0, shell=4),
        cache_witness=cache_witness,
        refused_provisional=provisional, refused_publishable=None,
    ), sort_keys=True, indent=2))

if __name__ == "__main__":
    main()
