#!/usr/bin/env python3
"""Bounded two-factor q2 model with continuation into the pinned fixed-anchor model.

This is not C++, a WSPD producer, a benchmark or an HGP FULL tower. Both
categories of box tests use the pinned generic Python extrema implementation;
their counts are not a timing proxy for prepared 96/48-byte C++ constants.
Only the count traversal is suspendable; payload collection remains synchronous.
"""
from __future__ import annotations

from collections import deque
from dataclasses import dataclass, replace
import hashlib
import importlib.util
from itertools import product
import json
from pathlib import Path
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
PINS = {
    'morsehgp3D_v8/audits/q2_complement_20260914/model.py':
        '43c75559ab0966811a285240f4c31bcf3e60bb52684ffbd1b4686761f74e0443',
    'morsehgp3D_v8/audits/q2_front_20260914/order_fixture.py':
        'a5971188669ccb821371e4f10c42e94027f50a3e429c4b167e44e0e2c380d7c3',
    'morsehgp3D_v8/audits/p0_q2_census_bounds_probe.py':
        'c33510de452250b83a67f3eddb9e6fe1ba9319e3e5510cd6f338c3f046de6ceb',
}
for path, pin in PINS.items():
    if hashlib.sha256((ROOT / path).read_bytes()).hexdigest() != pin:
        raise RuntimeError('changed imported model: ' + path)
SPEC = importlib.util.spec_from_file_location('audit_fixed_anchor', ROOT / next(iter(PINS)))
fixed = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = fixed
SPEC.loader.exec_module(fixed)
require, Failure = fixed.require, fixed.Failure
box_bounds, midpoint_tree, node_for = fixed.box_bounds, fixed.midpoint_tree, fixed.node_for


@dataclass(frozen=True)
class ProductContext:
    b0: int
    escape_b0: int


@dataclass(frozen=True)
class ProductState:
    a_node: int
    b_node: int
    phase: str
    cursor: int
    count: int
    context: ProductContext


class ProductModel(fixed.Model):
    def __init__(self, points, k, mutant=None, refinement='largest_extent'):
        super().__init__(points, k, 'defer_b_exclude_a', mutant)
        require(refinement in ('largest_extent', 'witness_first'), 'invalid_refinement')
        self.refinement = refinement
        self.a_root = None
        self.work.update({name: 0 for name in (
            'product_roots', 'separate_anchor_roots', 'joint_task_entries',
            'joint_task_entries_B_singleton', 'joint_bound_tests_B_singleton',
            'joint_node_visits', 'joint_bound_tests', 'joint_minimum_endpoint_terms',
            'joint_maximum_endpoint_terms', 'joint_a_splits', 'joint_b_splits',
            'joint_z_splits', 'joint_rejected_pairs', 'joint_accepted_pairs',
            'joint_rejections_at_pair', 'singleton_handoffs', 'handoffs_after_credit',
            'handoffs_after_anchor_zero_consumed', 'handoff_pairs',
            'anchor_rejected_pairs', 'anchor_accepted_pairs', 'anchor_rejections_at_pair',
            'peak_live_tasks', 'illegal_a_skips',
        )})

    def prefix(self, state):
        if isinstance(state, fixed.State):
            return super().prefix(state)
        first = len(self.order) if state.cursor == self.nend else self.nodes[state.cursor].first
        b0 = self.nodes[state.context.b0]
        outside = tuple(site for rank, site in enumerate(self.order)
                        if not b0.first <= rank < b0.last)
        if state.phase == 'complement':
            return tuple(site for rank, site in enumerate(self.order[:first])
                         if not b0.first <= rank < b0.last)
        return outside + self.order[b0.first:min(first, b0.last)]

    def check_prefix(self, state):
        if isinstance(state, fixed.State):
            return super().check_prefix(state)
        prefix = self.prefix(state)
        for a in self.nodes[state.a_node].ids:
            for b in self.nodes[state.b_node].ids:
                expected = sum(fixed.strict_power(self.points[a], self.points[b], self.points[z]) > 0
                               for z in prefix)
                self.prefix_checks += 1
                self.oracle_sites += len(prefix)
                require(expected == state.count < self.k, 'joint_prefix_count_mismatch',
                        a=a, b=b, a_node=state.a_node, b_node=state.b_node,
                        b0=state.context.b0, phase=state.phase, cursor=state.cursor,
                        expected=expected, actual=state.count, prefix_ids=prefix)

    def finish(self, state, saturated=False):
        # Fixed.Model.step dispatches here after the actual singleton handoff.
        mass = len(self.nodes[state.query].ids)
        self.work['anchor_rejected_pairs' if saturated else 'anchor_accepted_pairs'] += mass
        self.work['anchor_rejections_at_pair'] += int(saturated and mass == 1)
        return super().finish(state, saturated)

    def finish_product(self, state, saturated=False):
        a, b = self.nodes[state.a_node], self.nodes[state.b_node]
        mass = len(a.ids) * len(b.ids)
        self.work['joint_rejected_pairs' if saturated else 'joint_accepted_pairs'] += mass
        self.work['joint_rejections_at_pair'] += int(saturated and mass == 1)
        # The complete payload is still paid separately for every admitted
        # support. Call the existing collector/judge, never synthesize an
        # emitted payload by invoking its independent scalar oracle.
        for rank in range(a.first, a.last):
            context = fixed.Context(self.order[rank], state.context.b0,
                                    state.context.escape_b0, rank)
            leaf_state = fixed.State(state.b_node, state.phase, state.cursor, state.count, context)
            fixed.Model.finish(self, leaf_state, saturated)
        return []

    def handoff(self, state):
        a = self.nodes[state.a_node]
        require(len(a.ids) == 1, 'handoff_requires_singleton_A')
        site = self.order[a.first]
        self.work['singleton_handoffs'] += 1
        self.work['handoffs_after_credit'] += int(state.count > 0)
        self.work['handoffs_after_anchor_zero_consumed'] += int(site in self.prefix(state))
        self.work['handoff_pairs'] += len(self.nodes[state.b_node].ids)
        context = fixed.Context(site, state.context.b0, state.context.escape_b0, a.first)
        phase, cursor, count = state.phase, state.cursor, state.count
        if self.mutant == 'restart_handoff_keep_count':
            phase, cursor = 'complement', 0
        elif self.mutant == 'drop_handoff_count':
            count = 0
        elif self.mutant == 'handoff_rebases_b0':
            context = replace(context, b0=state.b_node, escape_b0=self.nodes[state.b_node].escape)
        self.events.append(dict(kind='handoff', a=site, b0=context.b0, b=state.b_node,
                                phase=phase, cursor=cursor, count=count,
                                original_b0=state.context.b0, anchor_rank=a.first))
        return [fixed.State(state.b_node, phase, cursor, count, context)]

    def joint_structural(self, state):
        if state.phase != 'complement':
            return None
        z = self.nodes[state.cursor]
        context = state.context
        if state.cursor == context.b0:
            self.work['structural_b0_skips'] += 1
            return [replace(state, cursor=context.escape_b0)]
        if self.mutant == 'exclude_whole_A' and state.cursor == self.a_root:
            self.work['illegal_a_skips'] += 1
            return [replace(state, cursor=z.escape)]
        if state.cursor < context.b0 < z.escape:
            require(bool(z.children), 'joint_structural_descent_at_leaf')
            self.work['structural_descents'] += 1
            return [replace(state, cursor=z.children[0])]
        return None

    def step(self, state):
        if isinstance(state, fixed.State):
            # This is the pinned fixed-anchor state machine itself, including
            # its global payload collection. No root_start, credit merge or
            # change of B0 occurs here. a is removed only from the zero count.
            return super().step(state)
        self.work['steps'] += 1
        self.check_prefix(state)
        a, b = self.nodes[state.a_node], self.nodes[state.b_node]
        if len(a.ids) == 1:
            return self.handoff(state)
        context = state.context
        if state.phase == 'complement' and state.cursor == self.nend:
            self.work['phase_transitions'] += 1
            self.work['transitions_with_credit'] += int(state.count > 0)
            self.events.append(dict(kind='joint_transition', a=state.a_node, b0=context.b0,
                                    count=state.count, escape_b0=context.escape_b0))
            return [replace(state, phase='inside', cursor=context.b0)]
        end = self.nend if state.phase == 'complement' else context.escape_b0
        if state.cursor == end:
            self.work['inside_ends_before_index_end'] += int(end != self.nend)
            return self.finish_product(state)
        require(0 <= state.cursor < self.nend, 'invalid_joint_cursor')
        self.work['node_visits'] += 1
        self.work['joint_node_visits'] += 1
        structural = self.joint_structural(state)
        if structural is not None:
            return structural
        z = self.nodes[state.cursor]
        self.work['joint_bound_tests'] += 1
        self.work['joint_bound_tests_B_singleton'] += int(len(b.ids) == 1)
        # Actual generic bound implementation: 8 minimum terms and 4 maximum
        # endpoint pairs per axis. Fixed-anchor calls retain the old mirror's
        # generic implementation; no C++ preparation/residency is simulated.
        self.work['joint_minimum_endpoint_terms'] += 24
        self.work['joint_maximum_endpoint_terms'] += 12
        low, high4 = box_bounds(a.box, b.box, z.box)
        if low > 0:
            count = state.count + len(z.ids)
            if count >= self.k:
                return self.finish_product(state, saturated=True)
            return [replace(state, count=count, cursor=z.escape)]
        if high4 <= 0:
            return [replace(state, cursor=z.escape)]
        if z.children and (self.refinement == 'witness_first' or
                           self.diagonal(z) > max(self.diagonal(a), self.diagonal(b))):
            self.work['joint_z_splits'] += 1
            return [replace(state, cursor=z.children[0])]
        choose_a = self.diagonal(a) >= self.diagonal(b) or not b.children
        factor = a if choose_a else b
        require(bool(factor.children), 'cannot_refine_joint_product')
        self.work['joint_a_splits' if choose_a else 'joint_b_splits'] += 1
        self.work['query_tasks'] += 2
        children = [replace(state, **({'a_node': child} if choose_a else {'b_node': child}))
                    for child in factor.children]
        self.work['joint_task_entries'] += sum(len(self.nodes[child.a_node].ids) > 1 for child in children)
        self.work['joint_task_entries_B_singleton'] += sum(
            len(self.nodes[child.a_node].ids) > 1 and len(self.nodes[child.b_node].ids) == 1
            for child in children)
        self.events.append(dict(kind='joint_split', axis='A' if choose_a else 'B',
                                a=state.a_node, b=state.b_node, b0=context.b0,
                                phase=state.phase, cursor=state.cursor, count=state.count))
        if self.mutant == 'child_skips_open_Z':
            children = [replace(child, cursor=z.escape) for child in children]
        return children

    def run_product(self, a_node, b_node, separate=False, budget=1, schedule='fifo'):
        require(budget >= 1 and schedule in ('fifo', 'lifo'), 'invalid_product_schedule')
        a, b = self.nodes[a_node], self.nodes[b_node]
        require(a.last <= b.first or b.last <= a.first, 'overlapping_product_factors')
        self.a_root = a_node
        mass = len(a.ids) * len(b.ids)
        if separate:
            pending = deque(fixed.State(b_node, 'complement', 0, 0,
                fixed.Context(self.order[rank], b_node, b.escape, rank))
                for rank in range(a.first, a.last))
            self.work['separate_anchor_roots'] = len(a.ids)
        else:
            pending = deque([ProductState(a_node, b_node, 'complement', 0, 0,
                                           ProductContext(b_node, b.escape))])
            self.work['product_roots'] = 1
            self.work['joint_task_entries'] = int(len(a.ids) > 1)
            self.work['joint_task_entries_B_singleton'] = int(len(a.ids) > 1 and len(b.ids) == 1)
        self.work['query_tasks'] = len(pending)
        while pending:
            self.work['peak_live_tasks'] = max(self.work['peak_live_tasks'], len(pending))
            state = pending.popleft() if schedule == 'fifo' else pending.pop()
            for _ in range(budget):
                successors = self.step(state)
                require(self.work['steps'] <= 300_000, 'product_model_step_cap')
                if len(successors) != 1:
                    pending.extend(successors)
                    break
                state, = successors
            else:
                pending.append(state)
                self.work['suspensions'] += 1
        domain = {(aa, bb) for aa in a.ids for bb in b.ids}
        require(set(self.accepted).union(self.rejected) == domain, 'product_coverage_mismatch')
        w = self.work
        require(w['query_tasks'] == w['product_roots'] + w['separate_anchor_roots'] +
                2 * (w['joint_a_splits'] + w['joint_b_splits'] + w['query_splits']),
                'task_partition_mismatch')
        require(w['node_visits'] == w['joint_bound_tests'] + w['geometric_bound_tests'] + w['point_tests'] +
                w['structural_descents'] + w['structural_b0_skips'] + w['structural_anchor_skips'] +
                w['illegal_a_skips'],
                'node_visit_partition_mismatch')
        require(w['joint_rejected_pairs'] + w['anchor_rejected_pairs'] == len(self.rejected) and
                w['joint_accepted_pairs'] + w['anchor_accepted_pairs'] == len(self.accepted),
                'resolved_pair_partition_mismatch')
        if not separate:
            require(w['joint_rejected_pairs'] + w['joint_accepted_pairs'] + w['handoff_pairs'] == mass and
                    w['handoff_pairs'] == w['anchor_rejected_pairs'] + w['anchor_accepted_pairs'],
                    'handoff_mass_partition_mismatch')
        return dict(work=w, candidate_pairs=mass,
                    accepted=sorted((aa, bb, row) for (aa, bb), row in self.accepted.items()),
                    rejected=sorted(self.rejected), events=self.events,
                    prefix_checks=self.prefix_checks, prefix_oracle_site_tests=self.oracle_sites)


def compare(points, a_ids, b_ids, k, refinement='largest_extent', schedules=True):
    nodes, _ = midpoint_tree(points)
    a, b = node_for(nodes, a_ids), node_for(nodes, b_ids)
    results, reference = {}, None
    for separate in (True, False):
        label = 'separate_anchors' if separate else 'joint_then_anchor'
        runs = ((1_000_000, 'lifo'), (1, 'fifo'), (1, 'lifo')) if schedules else ((1, 'fifo'),)
        reference_work = None
        for budget, schedule in runs:
            row = ProductModel(points, k, refinement=refinement).run_product(a, b, separate, budget, schedule)
            signature = row['accepted'], row['rejected']
            if reference is None:
                reference = signature
            require(signature == reference, 'joint_or_schedule_changed_output', label=label)
            work = {key: value for key, value in row['work'].items()
                    if key not in ('peak_live_tasks', 'suspensions')}
            if reference_work is None:
                reference_work = work
            require(work == reference_work, 'schedule_changed_product_work', label=label)
            if budget == 1 and schedule == 'fifo':
                results[label] = row
    return results


def rotated(points):
    # Integer rotation/reflection times scale3: M M^T=9I. Strict depths and
    # shell membership are invariant; the global midpoint tree is rebuilt.
    matrix = ((1, 2, 2), (2, 1, -2), (-2, 2, -1))
    return tuple(tuple(20000 + sum(x * coefficient for x, coefficient in zip(p, row))
                       for row in matrix) for p in points)


def cases():
    yield 'constructor', ((100, 0, 0), (100, 4, 0), (0, 1, 0), (0, 2, 0), (0, 3, 0), (50, 2, 0)), (0, 1), (2, 3, 4)
    yield 'B_split_first', ((1000, 0, 0), (1000, 1, 0), (0, 0, 0), (0, 10, 0), (0, 20, 0), (500, 2, 1)), (0, 1), (2, 3, 4)
    yield 'other_A_is_interior', ((0, 0, 0), (1, 1, 0), (100, 0, 1), (101, 0, 0)), (0, 1), (2, 3)
    yield 'nonaligned_clusters', ((1000, 11, 17), (1003, 19, 13), (999, 15, 24),
        (0, 3, 7), (5, 2, 11), (3, 8, 13), (1, 13, 9), (499, 7, 16), (507, 12, 18), (495, 21, 9)), (0, 1, 2), (3, 4, 5, 6)
    yield 'cube_shells', tuple(product((0, 2), repeat=3)), (0, 1, 2, 3), (4, 5, 6, 7)


def selftest():
    rows, candidates = [], []
    originals = list(cases())
    for name, initial, a_ids, b_ids in originals:
        orientations = [('original', initial)]
        if name != 'cube_shells':
            orientations.append(('integer_rotation', rotated(initial)))
        for orientation, points in orientations:
            # The separated clusters remain single nodes in these rotations.
            # The cube's original faces do not; its domain is kept unrotated
            # rather than silently replacing the original factors after rotation.
            for k in (1, 2, 5, 10):
                modes = compare(points, a_ids, b_ids, k)
                rows.append(dict(fixture=name, orientation=orientation, points=points,
                                 a_ids=a_ids, b_ids=b_ids, kmax=k, modes=modes))
                candidates.append((name, points, a_ids, b_ids, k))
    constructor = next(row for row in rows if row['fixture'] == 'constructor' and
                       row['orientation'] == 'original' and row['kmax'] == 1)
    c1 = constructor['modes']['joint_then_anchor']
    require(c1['work']['product_roots'] == 1 and c1['work']['joint_task_entries'] == 1 and
            c1['work']['joint_rejected_pairs'] == 6 and c1['work']['singleton_handoffs'] == 0,
            'constructor_K1_not_jointly_rejected')
    c2 = next(row for row in rows if row['fixture'] == 'constructor' and
              row['orientation'] == 'original' and row['kmax'] == 2)['modes']['joint_then_anchor']
    require(len(c2['accepted']) == 2 and c2['work']['handoffs_after_credit'] == 2,
            'constructor_K2_lost_credit_handoff')
    points, a_ids, b_ids = constructor['points'], constructor['a_ids'], constructor['b_ids']
    depths = [[len(fixed.oracle(points, a, b)[1]) for b in b_ids] for a in a_ids]
    require(depths == [[1, 2, 3], [3, 2, 1]], 'constructor_depths_changed')
    nodes, _ = midpoint_tree(points)
    a, b = node_for(nodes, a_ids), node_for(nodes, b_ids)
    lower, _ = box_bounds(nodes[a].box, nodes[b].box, fixed.singleton(points[5]))
    require(lower == 2498, 'constructor_joint_minimum_changed')
    after_zero = compare(points, a_ids, b_ids, 2, refinement='witness_first')
    require(after_zero['joint_then_anchor']['work']['handoffs_after_anchor_zero_consumed'] > 0,
            'missing_handoff_after_anchor_already_consumed')
    mutants = {}
    for mutant in ('restart_handoff_keep_count', 'drop_handoff_count', 'handoff_rebases_b0',
                   'child_skips_open_Z', 'exclude_whole_A', 'exclude_anchor_payload'):
        for name, points, a_ids, b_ids, k in candidates:
            nodes, _ = midpoint_tree(points)
            a, b = node_for(nodes, a_ids), node_for(nodes, b_ids)
            try:
                ProductModel(points, k, mutant).run_product(a, b)
            except Failure as error:
                mutants[mutant] = dict(fixture=name, points=points, a_ids=a_ids, b_ids=b_ids,
                                       kmax=k, detected_by=error.code, evidence=error.evidence)
                break
        require(mutant in mutants, 'product_mutant_not_demonstrated', mutant=mutant)
    joint = [row['modes']['joint_then_anchor'] for row in rows]
    require(any(row['work']['joint_b_splits'] for row in joint) and
            any(row['work']['joint_z_splits'] for row in joint), 'missing_joint_refinement')
    require(any(len(payload[2][2]) > 2 for row in joint for payload in row['accepted']),
            'missing_extra_shell_sites')
    # Explicit rejection of an invalid product domain, without any emission.
    invalid = ProductModel(points, 1)
    try:
        invalid.run_product(0, 0)
    except Failure as error:
        require(error.code == 'overlapping_product_factors' and not invalid.accepted and not invalid.rejected,
                'invalid_product_emitted_before_rejection')
    else:
        raise Failure('overlapping_product_not_rejected')
    return dict(status='passed', schema='mhgp8_q2_product_model_v1',
                scope='bounded_python_product_to_anchor_not_cpp_not_wspd_not_full',
                public_status='not_claimed', cpp_binary_executed=False, timing_measured=False,
                gcp_used=False, source_sha256=PINS, rows=rows, mutants=mutants,
                constructor_depths=depths, constructor_uniform_minimum=lower,
                already_consumed_anchor_handoff=after_zero,
                refinement='largest squared diagonal; witness only if strictly larger; A wins factor ties',
                special_refinement_case='witness_first only for already-consumed anchor handoff',
                suspension_scope='count traversal only; payload synchronous',
                bounds_backend='pinned generic Python box extrema; no prepared C++ cost simulation')


if __name__ == '__main__':
    try:
        print(json.dumps(selftest(), sort_keys=True))
    except Failure as error:
        print(json.dumps(dict(status='failed', error=error.code, evidence=error.evidence)), file=sys.stderr)
        raise
