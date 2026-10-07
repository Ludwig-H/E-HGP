#!/usr/bin/env python3
"""Exact, bounded contract witnesses; neither a native nor a GPU execution.

The triangle checks geometric output conventions independently using Fraction.
The six-site model follows only reservoir/filter/envelope/split in v11 boxes.cpp.
Its leaf_size=3 is explicit; it is not a claim about the measured 16/24 settings.
No external data, dependencies or assertions disabled by python -O.
"""
from collections import deque
from fractions import Fraction as F
from itertools import combinations, product
from pathlib import Path
import hashlib
import json
import subprocess


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def check(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(value):
    data = json.dumps(value, sort_keys=True, separators=(',', ':')).encode()
    return hashlib.sha256(data).hexdigest()


def source_closure():
    manifest = json.loads((HERE/'sources.json').read_text())
    head = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
    for item in manifest['files']:
        data = (ROOT/item['path']).read_bytes()
        check(hashlib.sha256(data).hexdigest() == item['sha256'], 'source changed: '+item['path'])
        blob = subprocess.check_output(
            ['git', '-C', str(ROOT), 'rev-parse', manifest['pin']+':'+item['path']], text=True).strip()
        check(blob == item['blob'], 'pin/blob mismatch: '+item['path'])
    return dict(pin=manifest['pin'], checkout_head=head,
                manifest_sha256=hashlib.sha256((HERE/'sources.json').read_bytes()).hexdigest(),
                script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                files_checked=len(manifest['files']))


def morton(point):
    check(all(0 <= x < 256 for x in point), 'witness outside eight-bit domain')
    return sum(((point[a] >> b) & 1) << (3*b+a) for b in range(8) for a in range(3))


def norm2(a, b):
    return sum((x-y)**2 for x, y in zip(a, b))


def triangle():
    points = {'A': (0, 1, 1), 'B': (1, 0, 1), 'C': (1, 1, 0)}
    edges = list(combinations(points, 2))
    for edge in edges:
        center = tuple(F(points[edge[0]][a]+points[edge[1]][a], 2) for a in range(3))
        shell = [p for p, xyz in points.items() if norm2(xyz, center) == F(1, 2)]
        check(set(shell) == set(edge), 'every edge ball must have its unique pair support')
        third = next(p for p in points if p not in edge)
        check(norm2(points[third], center) == F(3, 2), 'third site must be outside')
    center = (F(2, 3),)*3
    check(all(norm2(p, center) == F(2, 3) for p in points.values()), 'triple MEB radius')
    # Center is the positive barycenter: this is also the minimum enclosing ball.
    check(center == tuple(sum(F(p[a], 3) for p in points.values()) for a in range(3)), 'barycenter')

    def conventions(key):
        ordered = sorted(points, key=lambda p: key(points[p]))
        rank = {p: i for i, p in enumerate(ordered)}
        sequence = sorted(edges, key=lambda e: tuple(sorted(rank[p] for p in e)))
        parent = {p: p for p in points}

        def root(p):
            while parent[p] != p:
                p = parent[p]
            return p

        kept = []
        for a, b in sequence:
            ra, rb = root(a), root(b)
            if ra != rb:
                parent[rb] = ra
                kept.append(''.join(sorted((a, b))))
        # At K2/radius²=1/2 the three pair lenses are distinct point components.
        cover = {p: ''.join(sorted(next(e for e in sequence if p in e))) for p in points}
        return dict(site_order=ordered, ball_order=[''.join(e) for e in sequence],
                    kruskal_k1=kept, cover_choice_k2=cover)

    old = conventions(morton)
    new = conventions(lambda p: p)
    check(set(old['kruskal_k1']) != set(new['kruskal_k1']), 'Kruskal witness did not separate')
    check(old['cover_choice_k2'] != new['cover_choice_k2'], 'cover witness did not separate')
    return dict(points=points, edge_level='1/2', triple_level='2/3',
                all_k1_balls_have_unique_support=True, morton=old, positions=new)


POINTS = ((2, 5, 2), (4, 7, 4), (8, 10, 9), (9, 12, 11), (10, 8, 6), (12, 10, 4))


def walk(normalized, breadth_first):
    low = tuple(min(p[a] for p in POINTS) for a in range(3))
    high = tuple(max(p[a] for p in POINTS)+1 for a in range(3))
    ordered = sorted(POINTS, key=lambda p: morton(tuple(p[a]-low[a] if normalized else p[a] for a in range(3))))
    pending = deque([('', ordered, low, high)])
    leaves, details = [], {}
    tests = 0
    while pending:
        path, parent, lo, hi = pending.popleft() if breadth_first else pending.pop()
        distance = lambda p: sum((2*p[a]-lo[a]-hi[a])**2 for a in range(3))
        witnesses = sorted(parent, key=distance)[:3]  # stable; K=1, reservoir=3K
        kept = []
        for x in parent:
            dominated = False
            for y in witnesses:
                tests += 1
                # Minimum of |x-c|²-|y-c|² on the closed box [lo,hi].
                margin = sum((x[a]-lo[a])**2-(y[a]-lo[a])**2
                             - max(0, 2*(hi[a]-lo[a])*(x[a]-y[a])) for a in range(3))
                if margin > 0:
                    dominated = True
                    break
            if not dominated:
                kept.append(x)
        details[path] = dict(lo=lo, hi=hi, parent=parent, witnesses=witnesses, kept=kept,
                             distances=[(p, distance(p)) for p in parent])
        if not kept:
            continue
        lo = tuple(max(lo[a], min(p[a] for p in kept)) for a in range(3))
        hi = tuple(min(hi[a], max(p[a] for p in kept)+1) for a in range(3))
        widths = tuple(hi[a]-lo[a] for a in range(3))
        if min(widths) <= 0:
            continue
        axis = max(range(3), key=lambda a: widths[a])
        if len(kept) <= 3 or widths[axis] <= 1:
            leaves.append((lo, hi, tuple(sorted(kept))))
            continue
        middle = lo[axis]+widths[axis]//2
        left_hi, right_lo = list(hi), list(lo)
        left_hi[axis], right_lo[axis] = middle, middle
        pending.append((path+'1', kept, tuple(right_lo), hi))
        pending.append((path+'0', kept, lo, tuple(left_hi)))
    return sorted(leaves), details, tests


def traversal():
    old, a, at = walk(False, False)
    new, b, bt = walk(True, False)
    check(old != new, 'Morton normalization must change leaf contents in this witness')
    for normalized, expected in ((False, (old, a, at)), (True, (new, b, bt))):
        check(walk(normalized, True) == expected, 'BFS/DFS disagreement with fixed initial order')
    path = '00'
    check(a[path]['lo'] == b[path]['lo'] == (2, 5, 2), 'unexpected witness low')
    check(a[path]['hi'] == b[path]['hi'] == (7, 13, 7), 'unexpected witness high')
    x, old_y, new_y = (9, 12, 11), (10, 8, 6), (8, 10, 9)
    check(x in a[path]['kept'] and x not in b[path]['kept'], 'filter witness absent')
    corners = product(*zip(a[path]['lo'], a[path]['hi']))
    margins = [norm2(x, c)-norm2(new_y, c) for c in corners]
    check(min(margins) == 7, 'new witness must strictly dominate on the closed box')
    return dict(points=POINTS, k=1, leaf_size=3, native_executed=False,
                absolute=dict(witness_node=a[path], leaves=len(old), leaf_sha256=digest(old), filter_tests=at),
                normalized=dict(witness_node=b[path], leaves=len(new), leaf_sha256=digest(new), filter_tests=bt),
                min_strict_dominance_on_closed_box=min(margins),
                bfs_equals_dfs_at_fixed_order=True)


def main():
    before = source_closure()
    result = dict(schema='ehgp.v12.catalogue_contract_witnesses.v1', native_executed=False, gcp_used=False,
                  triangle=triangle(), traversal=traversal(),
                  orientation_budget_bits={str(s): 7*s+9 for s in (16, 17)})
    check(result['orientation_budget_bits'] == {'16': 121, '17': 128}, 'budget arithmetic')
    check(source_closure() == before, 'source closure changed during witnesses')
    result.update(source_closure=before, before_after_unchanged=True,
                  verdicts=dict(transition_requires_more_than_multi_support_exception=True,
                                normalized_morton_changes_reservoir_and_leaf_contents=True,
                                bfs_dfs_same_with_identical_parent_order=True,
                                s17_outside_proposed_s16_tier=True))
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
