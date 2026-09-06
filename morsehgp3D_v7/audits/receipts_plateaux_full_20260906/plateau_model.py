"""Finite exact FULL plateau model, independent of all product helpers.

The older audit's rational Gram solver supplies only circumballs and powers.
All subsets/balls are enumerated here on at most seven sites for falsification;
this is neither a product architecture nor a replay of the four 50k records.
No compilation, subprocess, cloud access or assert-based gate.
"""

from __future__ import annotations

from collections import defaultdict
from fractions import Fraction as Q
import hashlib
from itertools import combinations
import json
from pathlib import Path
import sys
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from meb_rational_oracle_20260905 import circumball, power

Facet = tuple[int, ...]
Component = frozenset[Facet]


def need(value: bool, reason: str) -> None:
    if not value:
        raise ValueError(reason)


def cover(component: Component) -> frozenset[int]:
    return frozenset(x for facet in component for x in facet)


def groups(vertices: list[Any], edges: list[tuple[Any, Any]]) -> list[frozenset[Any]]:
    parent = {v: v for v in vertices}

    def root(v: Any) -> Any:
        while parent[v] != v:
            v = parent[v]
        return v

    for a, b in edges:
        a, b = root(a), root(b)
        if a != b:
            parent[b] = a
    out: dict[Any, set[Any]] = defaultdict(set)
    for v in vertices:
        out[root(v)].add(v)
    return [frozenset(v) for v in out.values()]


class Model:
    def __init__(self, points: list[tuple[int, int, int]]) -> None:
        need(2 <= len(points) <= 7 and len(set(points)) == len(points), "bounded_distinct_input")
        self.points = points
        self.ids = tuple(range(len(points)))
        self.balls: dict[tuple[int, ...], dict[str, Any]] = {}
        support_rows = []
        for size in range(1, min(4, len(points)) + 1):
            for support in combinations(self.ids, size):
                ball = circumball([points[i] for i in support])
                if ball is None or not ball['positive']:
                    continue
                values = [power(ball, p) for p in points]
                key = tuple(ball['key'])
                closed = frozenset(i for i, p in enumerate(values) if p <= 0)
                if key not in self.balls:
                    self.balls[key] = dict(ball, closed=closed,
                        interior=frozenset(i for i, p in enumerate(values) if p < 0),
                        shell=frozenset(i for i, p in enumerate(values) if p == 0), bases=[])
                self.balls[key]['bases'].append(frozenset(support))
                support_rows.append((frozenset(support), closed, key))
        self.key: dict[Facet, tuple[int, ...]] = {}
        self.level: dict[Facet, Q] = {}
        self.support: dict[Facet, frozenset[int]] = {}
        for size in range(1, len(points) + 1):
            for facet in combinations(self.ids, size):
                subset = frozenset(facet)
                valid = [(s, key) for s, closed, key in support_rows if s <= subset <= closed]
                need(bool(valid) and len({key for _, key in valid}) == 1, "unique_certified_MEB")
                support, key = valid[0]
                self.key[facet] = key
                self.level[facet] = self.balls[key]['radius']
                self.support[facet] = support

    def gamma(self, k: int, cut: Q, closed: bool) -> list[Component]:
        active = lambda f: self.level[f] <= cut if closed else self.level[f] < cut
        vertices = [f for f in combinations(self.ids, k) if active(f)]
        edges = []
        for coface in combinations(self.ids, k + 1):
            if active(coface):
                facets = list(combinations(coface, k))
                edges.extend((facets[0], f) for f in facets[1:])
        return groups(vertices, edges)

    def strict_local(self, ball: dict[str, Any], k: int) -> list[Component]:
        # Center membership is tested by all positive minimal bases on the
        # sphere, not by assuming a chosen basis gives all essential points.
        contains_center = lambda f: any(b <= set(f) for b in ball['bases'])
        vertices = [f for f in combinations(sorted(ball['closed']), k) if not contains_center(f)]
        for f in combinations(sorted(ball['closed']), k):
            need(contains_center(f) == (self.level[f] == ball['radius']), "hull_MEB_equivalence")
        edges = []
        for coface in combinations(sorted(ball['closed']), k + 1):
            if not contains_center(coface):
                facets = list(combinations(coface, k))
                edges.extend((facets[0], f) for f in facets[1:])
        return groups(vertices, edges)

    def local_parents(self, key: tuple[int, ...], k: int) -> set[Component]:
        ball = self.balls[key]
        old = self.gamma(k, ball['radius'], False)
        parents = set()
        for component in self.strict_local(ball, k):
            images = [parent for parent in old if component <= parent]
            need(len(images) == 1, "local_component_global_image")
            parents.add(images[0])
        return parents

    def shell_quotient(self, ball: dict[str, Any], k: int) -> list[Component]:
        shell = sorted(ball['shell'])
        position = {v: i for i, v in enumerate(shell)}
        table = [False] * (1 << len(shell))
        for basis in ball['bases']:
            table[sum(1 << position[v] for v in basis)] = True
        for mask in range(len(table)):
            if table[mask]:
                for bit in range(len(shell)):
                    table[mask | (1 << bit)] = True
        for mask, value in enumerate(table):
            subset = {v for i, v in enumerate(shell) if mask & (1 << i)}
            need(value == any(b <= subset for b in ball['bases']), "upward_boolean_hull_closure")
        contains = lambda f: table[sum(1 << position[v] for v in f)]
        t = k - len(ball['interior'])
        need(t > 0, "shell_quotient_domain")
        vertices = [a for a in combinations(shell, t) if not contains(a)]
        edges = []
        for coface in combinations(shell, t + 1):
            if not contains(coface):
                faces = list(combinations(coface, t))
                edges.extend((faces[0], f) for f in faces[1:])
        return groups(vertices, edges)

    def batch(self, roots: list[Component], k: int, radius: Q,
              skip_extra: bool = False, skip_large: bool = False) -> tuple[list[Component], list[dict[str, Any]]]:
        blocks = [(key, b) for key, b in self.balls.items()
                  if b['radius'] == radius and len(b['closed']) >= k
                  and not (skip_extra and len(b['shell']) > min(map(len, b['bases'])))
                  and not (skip_large and len(b['closed']) > k + 1)]
        vertices = [('r', j) for j in range(len(roots))] + [('b', j) for j in range(len(blocks))]
        edges = []
        for j, (_, ball) in enumerate(blocks):
            for component in self.strict_local(ball, k):
                images = [i for i, parent in enumerate(roots) if component <= parent]
                need(len(images) == 1, "resolver_from_own_strict_snapshot")
                edges.append((('b', j), ('r', images[0])))
        output, events = [], []
        for group in groups(vertices, edges):
            old = [roots[i] for kind, i in group if kind == 'r']
            bs = [blocks[i][1] for kind, i in group if kind == 'b']
            members = frozenset().union(*old)
            before = cover(members)
            for ball in bs:
                members |= frozenset(combinations(sorted(ball['closed']), k))
            output.append(members)
            after = cover(members)
            if bs and (len(old) != 1 or after != before):
                events.append(dict(k=k, radius=str(radius), parents=len(old),
                    kind='birth' if not old else 'merge' if len(old) > 1 else 'growth',
                    before=sorted(before), after=sorted(after), added=sorted(after - before),
                    new_facets=sum(self.level[f] == radius for f in members),
                    blocks=len(bs)))
        return output, events

    def verify(self) -> dict[str, Any]:
        cuts, inert, births, shell_quotients = 0, 0, 0, 0
        events = []
        digest = hashlib.sha256()
        for key, ball in self.balls.items():
            if ball['radius'] == 0:
                continue
            p, qmin = len(ball['interior']), min(map(len, ball['bases']))
            shell = sorted(ball['shell'])
            h = max(size for size in range(len(shell) + 1)
                    if any(not any(b <= set(t) for b in ball['bases'])
                           for t in combinations(shell, size)))
            for k in range(1, len(ball['closed']) + 1):
                strict = self.strict_local(ball, k)
                need((not strict) == (k > p + h), "birth_hemisphere_threshold")
                births += not strict
                if k > p:
                    reduced = self.shell_quotient(ball, k)
                    projected = []
                    for component in strict:
                        normalized = frozenset(a for f in component
                            for a in combinations(sorted(set(f) & ball['shell']), k - p))
                        projected.append(normalized)
                        need(cover(component) == ball['interior'] | cover(normalized),
                             "shell_quotient_preserves_point_coverage")
                    need(set(projected) == set(reduced), "shell_quotient_preserves_exact_local_components")
                    shell_quotients += 1
                if k <= p + qmin - 2:
                    need(len(strict) == 1 and cover(strict[0]) == ball['closed'], "inert_low_K_includes_coverage")
                    inert += 1
        for k in range(1, len(self.ids) + 1):
            roots: list[Component] = []
            for radius in sorted(set(self.level.values())):
                need(set(roots) == set(self.gamma(k, radius, False)), "open_cut_exact_facets")
                roots, batch_events = self.batch(roots, k, radius)
                need(set(roots) == set(self.gamma(k, radius, True)), "closed_cut_exact_facets")
                events.extend(batch_events)
                cuts += 2
                canonical = sorted(sorted(c) for c in roots)
                digest.update(json.dumps([k, str(radius), canonical], separators=(',', ':')).encode())
        vertical_maps, squares = 0, 0
        for k in range(2, len(self.ids) + 1):
            previous: dict[Component, Component] = {}
            for radius in sorted(set(self.level.values())):
                for closed in (False, True):
                    lower = self.gamma(k - 1, radius, closed)
                    current = {}
                    for upper in self.gamma(k, radius, closed):
                        faces = frozenset(f for coface in upper for f in combinations(coface, k - 1))
                        images = [component for component in lower if faces <= component]
                        need(len(images) == 1, "unique_vertical_image")
                        current[upper] = images[0]
                        vertical_maps += 1
                    for upper, old_image in previous.items():
                        targets = [c for c in current if upper <= c]
                        need(len(targets) == 1 and old_image <= current[targets[0]], "vertical_naturality")
                        squares += 1
                    previous = current
        return dict(orders=len(self.ids), cuts=cuts, inert_blocks=inert, birth_blocks=births,
                    shell_quotients=shell_quotients,
                    vertical_maps=vertical_maps, naturality_squares=squares,
                    events=events, digest=digest.hexdigest())


def at(events: list[dict[str, Any]], k: int, radius: str) -> dict[str, Any]:
    found = [e for e in events if e['k'] == k and e['radius'] == radius]
    need(len(found) == 1, "unique_named_event")
    return found[0]


def portal(model: Model) -> dict[str, Any]:
    original = (0, 1, 2, 3)
    current = (0, 1, 2, 3, 4)
    steps = []
    for _ in range(20):
        ball = model.balls[model.key[current]]
        foreign = sorted(ball['interior'] - set(current))
        if not foreign:
            break
        removed = min(model.support[current])
        next_facet = tuple(sorted((set(current) - {removed}) | {foreign[0]}))
        next_ball = model.balls[model.key[next_facet]]
        old_shell = len(set(current) & ball['shell'])
        new_shell = len(set(next_facet) & next_ball['shell'])
        need(next_ball['radius'] <= ball['radius'], "exchange_radius_not_increasing")
        if next_ball['radius'] == ball['radius']:
            need(model.key[current] == model.key[next_facet] and new_shell == old_shell - 1,
                 "equal_radius_same_ball_shell_decreases")
        shared = tuple(sorted(set(current) & set(next_facet)))
        need(len(shared) == 4 and model.level[shared] <= ball['radius'], "elementary_path")
        steps.append(dict(before=current, after=next_facet, old_radius=str(ball['radius']),
                          new_radius=str(next_ball['radius']), old_shell=old_shell, new_shell=new_shell))
        current = next_facet
    else:
        raise ValueError("bounded_portal_did_not_finish")
    need(steps and steps[0]['old_radius'] == steps[0]['new_radius'] == '4', "strict_descent_mutant_refuted")
    need(any(original in c and any(set(f) <= set(current) for f in c)
             for c in model.gamma(4, Q(4), True)), "portal_preserves_component")
    return dict(steps=steps, terminal=current, external_strict_intruders=0)


def main() -> None:
    clouds = {
        'right_triangle': [(0, 0, 0), (4, 0, 0), (2, 2, 0)],
        'external_bridge': [(0, 0, 0), (4, 0, 0), (2, 2, 0), (2, 3, 0)],
        'square': [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)],
        'coverage_growth': [(1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0)],
        'window_shell7': [(10, 5, 0), (0, 5, 0), (5, 10, 0), (5, 0, 0),
                          (8, 9, 0), (2, 1, 0), (9, 8, 0)],
        'portal_equal': [(0, 2, 0), (2, 4, 0), (4, 2, 0), (2, 0, 0), (2, 2, 0), (2, 1, 0)],
    }
    models = {name: Model(points) for name, points in clouds.items()}
    runs = {name: model.verify() for name, model in models.items()}
    square_birth = at(runs['square']['events'], 3, '2')
    square_merge = at(runs['square']['events'], 2, '2')
    simultaneous = at(runs['square']['events'], 1, '1')
    need(square_birth['parents'] == 0 and square_birth['new_facets'] == 4
         and len(square_birth['after']) == 4, "birth_is_not_one_K_facet")
    need(square_merge['parents'] == 4, "extra_shell_real_multifusion")
    need(simultaneous['parents'] == 4 and simultaneous['blocks'] == 4,
         "distinct_balls_same_level_join_through_old_roots_atomically")
    growth = at(runs['coverage_growth']['events'], 3, '25')
    need(growth['parents'] == 1 and growth['before'] == [0, 1, 2]
         and growth['added'] == [3], "FULL_no_growth_mutant_refuted")
    large = at(runs['window_shell7']['events'], 5, '25')
    need(large['parents'] == 0 and len(large['after']) == 7
         and large['new_facets'] == 21, "closed_size_above_window_still_matters")
    local = models['right_triangle']
    key = local.key[(0, 1)]
    external = models['external_bridge']
    need(local.balls[key]['closed'] == external.balls[key]['closed'] == frozenset({0, 1, 2})
         and local.balls[key]['interior'] == external.balls[key]['interior'] == frozenset(), "identical_local_geometry")
    parent_counts = [len(m.local_parents(key, 2)) for m in (local, external)]
    need(parent_counts == [2, 1], "local_counts_do_not_decide_global_merges")
    square = models['square']
    ball = square.balls[square.key[(0, 1, 2, 3)]]
    essential = set.intersection(*(set(b) for b in ball['bases']))
    need(not essential and len(square.support[(0, 1, 2, 3)]) == 2, "chosen_support_not_essential_set")
    # Compare one mutated batch against the actual strict snapshot; do not
    # confuse absent earlier aliases with the targeted omission itself.
    old = square.gamma(3, Q(2), False)
    wrong, _ = square.batch(old, 3, Q(2), skip_extra=True)
    need(set(wrong) != set(square.gamma(3, Q(2), True)), "skip_extra_shell_mutant")
    window = models['window_shell7']
    wrong, _ = window.batch(window.gamma(5, Q(25), False), 5, Q(25), skip_large=True)
    need(set(wrong) != set(window.gamma(5, Q(25), True)), "skip_large_closed_set_mutant")
    output = dict(status='passed', public_status='not_claimed',
        scope='finite_exact_geometry_and_plateau_quotient_not_product_or_50k_extraction',
        clouds=clouds, cuts=sum(r['cuts'] for r in runs.values()),
        orders=sum(r['orders'] for r in runs.values()),
        inert_blocks=sum(r['inert_blocks'] for r in runs.values()),
        birth_blocks=sum(r['birth_blocks'] for r in runs.values()),
        shell_quotients=sum(r['shell_quotients'] for r in runs.values()),
        vertical_maps=sum(r['vertical_maps'] for r in runs.values()),
        naturality_squares=sum(r['naturality_squares'] for r in runs.values()),
        digests={name: r['digest'] for name, r in runs.items()},
        named_events=dict(square_birth=square_birth, square_merge=square_merge,
                          simultaneous_distinct_balls=simultaneous,
                          coverage_growth=growth, window_shell7_birth=large),
        same_local_ball_global_parent_counts=parent_counts,
        portal=portal(models['portal_equal']),
        engine_executed=False, GCP_used=False)
    print(json.dumps(output, sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
