#!/usr/bin/env python3
"""Bounded mathematical model: fixed complement(B0), then B0, optional anchor skip.

No C++ execution, timing, WSPD producer or FULL tower is represented. The
midpoint tree and continuous box bounds are explicitly reused from two pinned
audit models. The dot-product oracle and the fixed-ball payload traversal are
separate. Prefix checks are expensive judge instrumentation, not proposed state.
"""
from __future__ import annotations

from collections import deque
from dataclasses import dataclass, replace
import hashlib
from itertools import product
import json
from pathlib import Path
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
PINS = {
    'morsehgp3D_v8/audits/q2_front_20260914/order_fixture.py':
        'a5971188669ccb821371e4f10c42e94027f50a3e429c4b167e44e0e2c380d7c3',
    'morsehgp3D_v8/audits/p0_q2_census_bounds_probe.py':
        'c33510de452250b83a67f3eddb9e6fe1ba9319e3e5510cd6f338c3f046de6ceb',
}
for source, pin in PINS.items():
    if hashlib.sha256((ROOT / source).read_bytes()).hexdigest() != pin:
        raise RuntimeError('changed imported model: ' + source)
sys.path.insert(0, str(BASE.parent / 'q2_front_20260914'))
from order_fixture import midpoint_tree, box_bounds, singleton

MODES = ('dfs', 'defer_b', 'defer_b_exclude_a')


class Failure(RuntimeError):
    def __init__(self, code, **evidence):
        super().__init__(code)
        self.code, self.evidence = code, evidence


def require(ok, code, **evidence):
    if not ok:
        raise Failure(code, **evidence)


def strict_power(a, b, z):
    # Independent scalar signed-integer oracle. No imported geometry or tree.
    return sum((zz - aa) * (bb - zz) for aa, bb, zz in zip(a, b, z))


def oracle(points, a, b):
    values = [strict_power(points[a], points[b], z) for z in points]
    key = (tuple(aa + bb for aa, bb in zip(points[a], points[b])),
           sum((aa - bb) ** 2 for aa, bb in zip(points[a], points[b])))
    return key, tuple(i for i, v in enumerate(values) if v > 0), \
        tuple(i for i, v in enumerate(values) if v == 0)


@dataclass(frozen=True)
class Context:
    a: int
    b0: int
    escape_b0: int
    anchor_rank: int


@dataclass(frozen=True)
class State:
    query: int
    phase: str
    cursor: int
    count: int
    context: Context


class Model:
    def __init__(self, points, kmax, mode, mutant=None):
        require(len(set(points)) == len(points) and all(
            len(p) == 3 and all(type(x) is int and 0 <= x <= 65535 for x in p)
            for p in points), 'invalid_quantized_cloud')
        require(1 <= kmax <= 10 and mode in MODES, 'invalid_parameters')
        self.points, self.k, self.mode, self.mutant = points, kmax, mode, mutant
        self.nodes, self.order = midpoint_tree(points)
        self.nend = len(self.nodes)
        self.work = {name: 0 for name in (
            'query_tasks', 'query_splits', 'node_visits', 'geometric_bound_tests',
            'point_tests', 'witness_splits', 'structural_descents',
            'structural_b0_skips', 'structural_anchor_skips', 'phase_transitions',
            'transitions_with_credit', 'inside_ends_before_index_end',
            'pairs_group_rejected', 'payload_node_visits', 'payload_bound_tests',
            'payload_point_tests', 'steps', 'suspensions',
        )}
        self.accepted, self.rejected, self.events = {}, set(), []
        self.prefix_checks = self.oracle_sites = 0

    def diagonal(self, node):
        return sum((high - low) ** 2 for low, high in node.box)

    def prefix(self, state):
        context = state.context
        first = len(self.order) if state.cursor == self.nend else self.nodes[state.cursor].first
        if self.mode == 'dfs':
            return self.order[:first]
        b0 = self.nodes[context.b0]
        outside = tuple(i for rank, i in enumerate(self.order)
                        if not b0.first <= rank < b0.last
                        and (self.mode != 'defer_b_exclude_a' or i != context.a))
        if state.phase == 'complement':
            return tuple(i for rank, i in enumerate(self.order[:first])
                         if not b0.first <= rank < b0.last
                         and (self.mode != 'defer_b_exclude_a' or i != context.a))
        inside = self.order[b0.first:min(first, b0.last)]
        return outside + inside

    def check_prefix(self, state):
        # Reconstruct the prefix only in the judge; no visited-site set, list
        # of frontiers, inverse permutation or mutable exclusion enters State.
        resolved = self.prefix(state)
        for b in self.nodes[state.query].ids:
            expected = sum(strict_power(self.points[state.context.a], self.points[b], self.points[z]) > 0
                           for z in resolved)
            self.oracle_sites += len(resolved)
            self.prefix_checks += 1
            require(expected == state.count < self.k, 'prefix_count_mismatch',
                    a=state.context.a, b=b, b0=state.context.b0, query=state.query,
                    phase=state.phase, cursor=state.cursor, expected=expected,
                    actual=state.count, prefix_ids=resolved)

    def payload(self, a, b):
        # Complete global collection is independent of the count permutation.
        # Its leaves use 4H = diameter² - ||2z-(a+b)||², not the oracle dot product.
        key = (tuple(x + y for x, y in zip(self.points[a], self.points[b])),
               sum((x - y) ** 2 for x, y in zip(self.points[a], self.points[b])))
        interior, shell, pending = [], [], [0]
        while pending:
            z = self.nodes[pending.pop()]
            self.work['payload_node_visits'] += 1
            if len(z.ids) == 1:
                self.work['payload_point_tests'] += 1
                site, = z.ids
                power4 = key[1] - sum((2 * x - center) ** 2
                                     for x, center in zip(self.points[site], key[0]))
                if power4 > 0:
                    interior.append(site)
                elif power4 == 0:
                    shell.append(site)
            else:
                self.work['payload_bound_tests'] += 1
                low, high4 = box_bounds(singleton(self.points[a]), singleton(self.points[b]), z.box)
                if low > 0:
                    interior.extend(z.ids)
                elif low == high4 == 0:
                    shell.extend(z.ids)
                elif high4 >= 0:
                    pending.extend(reversed(z.children))
        if self.mutant == 'exclude_anchor_payload':
            shell = [site for site in shell if site != a]
        require(len(interior) == len(set(interior)) and len(shell) == len(set(shell)),
                'duplicate_payload_id')
        return key, tuple(sorted(interior)), tuple(sorted(shell))

    def finish(self, state, saturated=False):
        a = state.context.a
        b_node = self.nodes[state.query]
        if saturated and len(b_node.ids) > 1:
            self.work['pairs_group_rejected'] += len(b_node.ids)
        for b in b_node.ids:
            pair = a, b
            require(pair not in self.accepted and pair not in self.rejected, 'duplicate_support', pair=pair)
            expected = oracle(self.points, a, b)
            if saturated:
                require(len(expected[1]) >= self.k, 'false_rejection', pair=pair, depth=len(expected[1]))
                self.rejected.add(pair)
            else:
                actual = self.payload(a, b)
                require(actual == expected and len(actual[1]) == state.count < self.k,
                        'payload_or_count_mismatch', pair=pair, count=state.count,
                        expected=expected, actual=actual)
                self.accepted[pair] = actual
        return []

    def structural(self, state):
        if self.mode == 'dfs' or state.phase != 'complement':
            return None
        context, cursor = state.context, state.cursor
        z, b0 = self.nodes[cursor], self.nodes[context.b0]
        if cursor == context.b0:
            self.work['structural_b0_skips'] += 1
            return [replace(state, cursor=context.escape_b0)]
        has_a = z.first <= context.anchor_rank < z.last and self.mode == 'defer_b_exclude_a'
        if has_a and not z.children:
            self.work['structural_anchor_skips'] += 1
            return [replace(state, cursor=z.escape)]
        proper_ancestor = cursor < context.b0 < z.escape
        if proper_ancestor or has_a:
            require(bool(z.children), 'structural_descent_at_leaf')
            self.work['structural_descents'] += 1
            return [replace(state, cursor=z.children[0])]
        require(not (max(z.first, b0.first) < min(z.last, b0.last)), 'partial_subtree_overlap')
        return None

    def step(self, state):
        self.work['steps'] += 1
        self.check_prefix(state)
        context = state.context
        if self.mode != 'dfs' and state.phase == 'complement' and state.cursor == self.nend:
            self.work['phase_transitions'] += 1
            self.work['transitions_with_credit'] += int(state.count > 0)
            self.events.append(dict(kind='transition', a=context.a, b0=context.b0,
                                    count=state.count, escape_b0=context.escape_b0))
            if self.mutant == 'drop_inside_phase':
                return self.finish(state)
            count = 0 if self.mutant == 'lose_transition_count' else state.count
            cursor = 0 if self.mutant == 'restart_transition' else context.b0
            return [replace(state, phase='inside', cursor=cursor, count=count)]
        end = self.nend if self.mode == 'dfs' or state.phase == 'complement' \
            or self.mutant == 'inside_ends_at_index' else context.escape_b0
        if state.cursor == end:
            self.work['inside_ends_before_index_end'] += int(end != self.nend)
            return self.finish(state)
        require(0 <= state.cursor < self.nend, 'invalid_cursor', cursor=state.cursor, end=end)
        self.work['node_visits'] += 1
        if self.mutant != 'structure_after_geometry':
            structural = self.structural(state)
            if structural is not None:
                return structural
        b, z = self.nodes[state.query], self.nodes[state.cursor]
        if len(b.ids) == len(z.ids) == 1:
            self.work['point_tests'] += 1
            b_id, = b.ids
            z_id, = z.ids
            # The modeled count leaf evaluates the same fixed-ball identity
            # as payload, independently of the scalar prefix/depth oracle.
            diameter = sum((x - y) ** 2 for x, y in zip(self.points[context.a], self.points[b_id]))
            value4 = diameter - sum((2 * zz - aa - bb) ** 2 for aa, bb, zz
                                   in zip(self.points[context.a], self.points[b_id], self.points[z_id]))
            low, high4 = value4, value4
        else:
            self.work['geometric_bound_tests'] += 1
            low, high4 = box_bounds(singleton(self.points[context.a]), b.box, z.box)
        if low > 0:
            count = state.count + len(z.ids)
            if count >= self.k:
                return self.finish(state, saturated=True)
            return [replace(state, count=count, cursor=z.escape)]
        if high4 <= 0:
            return [replace(state, cursor=z.escape)]
        if len(b.ids) == 1 or (z.children and self.diagonal(z) > self.diagonal(b)):
            if self.mutant == 'structure_after_geometry':
                structural = self.structural(state)
                if structural is not None:
                    return structural
            require(bool(z.children), 'uncertain_singleton_triple')
            self.work['witness_splits'] += 1
            return [replace(state, cursor=z.children[0])]
        require(bool(b.children), 'unsplittable_query')
        self.work['query_splits'] += 1
        self.work['query_tasks'] += 2
        self.events.append(dict(kind='split', a=context.a, b0=context.b0, query=state.query,
                                z=state.cursor, phase=state.phase, count=state.count))
        children = []
        for child in b.children:
            next_state = replace(state, query=child)
            if self.mutant == 'child_redefines_b0':
                next_state = replace(next_state, context=replace(context, b0=child,
                                                   escape_b0=self.nodes[child].escape))
            elif self.mutant == 'child_resets_phase':
                next_state = replace(next_state, phase='complement', cursor=0)
            children.append(next_state)
        return children

    def run(self, queries, budget=1, schedule='fifo'):
        require(budget >= 1 and schedule in ('fifo', 'lifo'), 'invalid_schedule')
        pending, domain = deque(), set()
        for a, query in queries:
            b = self.nodes[query]
            require(a not in b.ids, 'anchor_in_query')
            context = Context(a, query, b.escape, self.order.index(a))
            # rank is supplied by the fixture's anchor enumeration. No inverse
            # table is proposed; Context stores only this one original rank.
            pending.append(State(query, 'dfs' if self.mode == 'dfs' else 'complement', 0, 0, context))
            pairs = {(a, partner) for partner in b.ids}
            require(not domain.intersection(pairs), 'overlapping_query_domains')
            domain.update(pairs)
            self.work['query_tasks'] += 1
        while pending:
            state = pending.popleft() if schedule == 'fifo' else pending.pop()
            for _ in range(budget):
                successors = self.step(state)
                require(self.work['steps'] <= 2_000_000, 'model_step_cap')
                if len(successors) != 1:
                    pending.extend(successors)
                    break
                state, = successors
            else:
                pending.append(state)
                self.work['suspensions'] += 1
        require(set(self.accepted).union(self.rejected) == domain, 'lost_supports')
        return dict(work=self.work, accepted=sorted((a, b, row) for (a, b), row in self.accepted.items()),
                    rejected=sorted(self.rejected), events=self.events,
                    prefix_checks=self.prefix_checks, prefix_oracle_site_tests=self.oracle_sites)


def node_for(nodes, ids):
    wanted = set(ids)
    matches = [i for i, node in enumerate(nodes) if set(node.ids) == wanted]
    require(len(matches) == 1, 'fixture_not_one_global_node', ids=ids)
    return matches[0]


def all_pair_queries(nodes, n):
    # Judge-only exact cover by existing global nodes. No WSPD claim.
    result = []
    for a in range(n):
        pending = [0]
        while pending:
            i = pending.pop()
            node = nodes[i]
            if all(b > a for b in node.ids):
                result.append((a, i))
            elif any(b > a for b in node.ids):
                pending.extend(reversed(node.children))
    return result


def compare_modes(points, k, queries, suspended=True):
    result, reference = {}, None
    for mode in MODES:
        schedules = ((1_000_000, 'lifo'), (1, 'fifo'), (1, 'lifo')) if suspended else ((1, 'fifo'),)
        continuous = None
        for budget, schedule in schedules:
            row = Model(points, k, mode).run(queries, budget, schedule)
            signature = row['accepted'], row['rejected']
            if reference is None:
                reference = signature
            require(signature == reference, 'modes_or_schedule_changed_supports', mode=mode, budget=budget)
            discrete = {name: value for name, value in row['work'].items() if name != 'suspensions'}
            if continuous is None:
                continuous = discrete
            require(discrete == continuous, 'schedule_changed_discrete_work', mode=mode)
            if budget == 1 and schedule == 'fifo':
                result[mode] = row
    return result


def fixture_cases():
    yield 'line_credit', ((1000, 0, 0), (0, 0, 0), (1, 0, 0), (500, 2, 1)), (1, 2), 0, (1, 2, 3, 10)
    yield 'remaining', ((0, 0, 0), (999, 0, 0), (1000, 1, 0), (500, 2, 1)), (1, 2), 0, (1, 2, 3, 10)
    yield 'shell', ((257, 0, 0), (0, 0, 0), (1, 16, 0)), (1, 2), 0, (1, 2)
    yield 'internal_b0', ((1000, 0, 0), (0, 0, 0), (1, 0, 0), (500, 0, 0), (2000, 0, 0)), (1, 2), 0, (1, 2, 3, 10)
    yield 'cube_shell', tuple(product((0, 2), repeat=3)), (7,), 0, (1, 2, 5)


def selftest():
    cube = tuple(product(range(4), repeat=3))
    witnesses = tuple(product((997, 998, 999), (998, 999), (998, 999)))
    constructor = ((1000, 1000, 1000),) + cube + witnesses
    nodes, order = midpoint_tree(constructor)
    b0 = node_for(nodes, tuple(range(1, 65)))
    require(all(strict_power(constructor[0], b, w) > 0 for b in cube for w in witnesses),
            'constructor_witnesses_not_uniformly_strict')
    constructor_rows = compare_modes(constructor, 10, [(0, b0)])
    require(constructor_rows['dfs']['work']['query_splits'] > 0 and
            constructor_rows['defer_b']['work']['query_splits'] > 0 and
            constructor_rows['defer_b_exclude_a']['work']['query_splits'] == 0 and
            constructor_rows['defer_b_exclude_a']['work']['pairs_group_rejected'] == 64,
            'constructor_zero_split_contract_failed')
    rows, mutant_candidates = [], [('constructor', constructor, 10, [(0, b0)])]
    for name, points, ids, a, ks in fixture_cases():
        local_nodes, _ = midpoint_tree(points)
        query = node_for(local_nodes, ids)
        for k in ks:
            modes = compare_modes(points, k, [(a, query)])
            rows.append(dict(fixture=name, kmax=k, mode_results=modes))
            mutant_candidates.append((name, points, k, [(a, query)]))
        # Complete unordered-pair coverage also exercises singleton B0 roots.
        modes = compare_modes(points, 2, all_pair_queries(local_nodes, len(points)))
        rows.append(dict(fixture=name + '_all_pairs', kmax=2, mode_results=modes))
    # The constructor cloud itself is checked completely, using its exact
    # node-cover of all unordered pairs, not only the selected 64-pair fixture.
    full_rows = compare_modes(constructor, 10, all_pair_queries(nodes, len(constructor)), suspended=False)
    require(all(len(row['accepted']) + len(row['rejected']) == 77 * 76 // 2 for row in full_rows.values()),
            'constructor_full_pair_coverage_failed')
    mutants = {}
    for mutant in ('structure_after_geometry', 'child_redefines_b0', 'child_resets_phase',
                   'lose_transition_count', 'restart_transition', 'drop_inside_phase',
                   'inside_ends_at_index', 'exclude_anchor_payload'):
        for name, points, k, queries in mutant_candidates:
            try:
                outcome = Model(points, k, 'defer_b_exclude_a', mutant).run(queries)
                if mutant == 'structure_after_geometry' and name == 'constructor':
                    require(outcome['work']['query_splits'] == 0, 'late_descent_lost_zero_split_contract',
                            query_splits=outcome['work']['query_splits'], exact_outputs_verified=True)
            except Failure as error:
                mutants[mutant] = dict(fixture=name, points=points, kmax=k, queries=queries,
                                       detected_by=error.code, evidence=error.evidence)
                break
        require(mutant in mutants, 'mutant_not_demonstrated', mutant=mutant)
    all_rows = [row for entry in rows for row in entry['mode_results'].values()] + list(full_rows.values())
    require(any(row['work']['transitions_with_credit'] for row in all_rows), 'missing_credit_transition')
    require(any(row['work']['inside_ends_before_index_end'] for row in all_rows), 'missing_internal_b0_end')
    require(any(event['phase'] == 'inside' and event['count'] > 0 for row in all_rows
                for event in row['events'] if event['kind'] == 'split'), 'missing_inside_split_with_credit')
    require(any(event['phase'] == 'complement' for row in all_rows
                for event in row['events'] if event['kind'] == 'split'), 'missing_complement_split')
    return dict(status='passed', schema='mhgp8_q2_complement_model_v1',
                scope='bounded_python_state_machine_not_cpp_not_full', public_status='not_claimed',
                cpp_binary_executed=False, timing_measured=False, gcp_used=False,
                reference_source_sha256=PINS, modes=MODES, schedules=['continuous_lifo', 'budget1_fifo', 'budget1_lifo'],
                constructor=dict(points=constructor, spatial_order=order, b0=b0, mode_results=constructor_rows),
                cases=rows, constructor_all_pairs=full_rows, mutants=mutants)


if __name__ == '__main__':
    try:
        print(json.dumps(selftest(), sort_keys=True))
    except Failure as error:
        print(json.dumps(dict(status='failed', error=error.code, evidence=error.evidence)), file=sys.stderr)
        raise
