#!/usr/bin/env python3
"""Bounded local refinement experiment over the unchanged, pinned r2 model.

Only an uncertain Z==current A may use witness-first refinement. No witness
order, B0, count, cursor or payload policy changes. This is not a C++/LiDAR
benchmark, a WSPD qualification or a global performance claim.
"""
from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import sys

BASE = Path(__file__).resolve().parent
PINS = {
    'model.py': 'da21c18271f0a0b49467ceddbe6ab0a7d01c4e3157cbd2c9310ca253b35e2347',
    'r2_RUN.json': '69a98611a003f4a64f28495e5304b23613ccc023887bc5c8726a4fbeb56bf44e',
    'r2_RESULT.json': 'fecefe5d2e1546b2dd399bb6bc8d965c8291830540b9ca80ec3f6c6a8ddf5088',
}
for name, pin in PINS.items():
    if hashlib.sha256((BASE / name).read_bytes()).hexdigest() != pin:
        raise RuntimeError('changed closed r2 source/result: ' + name)
SPEC = importlib.util.spec_from_file_location('audit_product_r2', BASE / 'model.py')
reference = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = reference
SPEC.loader.exec_module(reference)
require = reference.require


class SelfNodeModel(reference.ProductModel):
    def __init__(self, points, k, relaxed):
        super().__init__(points, k)
        self.relaxed = relaxed
        self.work.update(self_node_encounters=0, self_node_forced_descents=0)

    def step(self, state):
        is_self = (isinstance(state, reference.ProductState) and
                   len(self.nodes[state.a_node].ids) > 1 and state.cursor == state.a_node)
        if not is_self:
            return super().step(state)
        self.work['self_node_encounters'] += 1
        previous = self.refinement
        before = self.work['joint_z_splits']
        if self.relaxed:
            self.refinement = 'witness_first'
        try:
            result = super().step(state)
        finally:
            self.refinement = previous
        if self.relaxed:
            descended = self.work['joint_z_splits'] - before
            require(descended == 1, 'self_node_was_not_an_uncertain_joint_descent')
            self.work['self_node_forced_descents'] += descended
        return result


def signature(row):
    return row['accepted'], row['rejected']


def normalized(value):
    return json.loads(json.dumps(value))


def paired_case(name, points, a_ids, b_ids, k, closed=None):
    nodes, order = reference.midpoint_tree(points)
    a = reference.node_for(nodes, a_ids)
    b = reference.node_for(nodes, b_ids)
    result = dict(fixture=name, points=points, a_ids=a_ids, b_ids=b_ids, kmax=k,
                  spatial_order=order, a_node=a, b_node=b, policies={})
    reference_signature = None
    for relaxed in (False, True):
        name = 'self_node' if relaxed else 'strictdiag'
        policy = dict(schedules={})
        expected_work = None
        for budget, schedule in ((1_000_000, 'lifo'), (1, 'fifo'), (1, 'lifo')):
            row = SelfNodeModel(points, k, relaxed).run_product(a, b, False, budget, schedule)
            if reference_signature is None:
                reference_signature = signature(row)
            require(signature(row) == reference_signature, 'self_node_changed_exact_output')
            work = {key: value for key, value in row['work'].items()
                    if key not in ('peak_live_tasks', 'suspensions')}
            if expected_work is None:
                expected_work = work
            require(work == expected_work, 'self_node_schedule_changed_work')
            key = 'continuous_lifo' if budget != 1 else 'budget1_' + schedule
            policy['schedules'][key] = dict(peak_live_tasks=row['work']['peak_live_tasks'],
                                          suspensions=row['work']['suspensions'])
            if budget == 1 and schedule == 'fifo':
                policy['result'] = row
        result['policies'][name] = policy
    baseline = result['policies']['strictdiag']['result']
    relaxed = result['policies']['self_node']['result']
    if closed is not None:
        actual = normalized(baseline)
        actual['work'].pop('self_node_encounters')
        actual['work'].pop('self_node_forced_descents')
        require(actual == closed, 'strictdiag_replay_differs_from_closed_r2')
    for field in ('payload_node_visits', 'payload_bound_tests', 'payload_point_tests'):
        require(baseline['work'][field] == relaxed['work'][field], 'self_node_changed_payload_work')
    result['delta_self_minus_strict'] = {
        field: relaxed['work'][field] - baseline['work'][field]
        for field in ('query_tasks', 'joint_task_entries', 'joint_bound_tests',
                      'geometric_bound_tests', 'point_tests', 'node_visits',
                      'joint_a_splits', 'joint_b_splits', 'joint_z_splits',
                      'singleton_handoffs', 'handoff_pairs', 'joint_accepted_pairs',
                      'joint_rejected_pairs', 'peak_live_tasks')}
    return result


def selftest():
    closed = json.loads((BASE / 'r2_RESULT.json').read_text())
    rows = []
    for row in closed['rows']:
        points = tuple(tuple(p) for p in row['points'])
        rows.append(paired_case(row['fixture'] + '/' + row['orientation'], points,
            tuple(row['a_ids']), tuple(row['b_ids']), row['kmax'],
            row['modes']['joint_then_anchor']))
    constructor = next(row for row in closed['rows'] if row['fixture'] == 'constructor' and
                       row['orientation'] == 'original' and row['kmax'] == 1)
    points = tuple((100 - x, y, z) for x, y, z in constructor['points'])
    for k in (1, 2):
        rows.append(paired_case('constructor/mirror_x', points,
                    tuple(constructor['a_ids']), tuple(constructor['b_ids']), k))
    require(len(rows) == 38, 'self_node_matrix_incomplete')
    mirrors = [row for row in rows if row['fixture'] == 'constructor/mirror_x']
    first = mirrors[0]['policies']
    require(first['strictdiag']['result']['work']['joint_rejected_pairs'] == 0 and
            first['self_node']['result']['work']['joint_rejected_pairs'] == 6 and
            first['self_node']['result']['work']['query_tasks'] == 1,
            'mirror_K1_did_not_expose_the_shared_witness')
    require(len(mirrors[1]['policies']['self_node']['result']['accepted']) == 2,
            'mirror_K2_changed_the_two_admissions')
    require(any(row['policies']['self_node']['result']['work']['joint_accepted_pairs'] > 0 for row in rows),
            'self_node_did_not_exercise_joint_acceptance')
    require(any(row['policies']['self_node']['result']['work']['handoffs_after_anchor_zero_consumed'] > 0
                for row in rows), 'self_node_did_not_exercise_old_anchor_zero_prefix')
    summary = {}
    for field in ('node_visits', 'joint_bound_tests', 'query_tasks', 'peak_live_tasks'):
        deltas = [row['delta_self_minus_strict'][field] for row in rows]
        summary[field] = dict(lower=sum(x < 0 for x in deltas), equal=sum(x == 0 for x in deltas),
                              higher=sum(x > 0 for x in deltas), min_delta=min(deltas), max_delta=max(deltas))
    return dict(status='passed', schema='mhgp8_q2_self_node_model_v1',
                scope='bounded_refinement_policy_not_cpp_not_lidar_not_wspd_not_full',
                public_status='not_claimed', cpp_binary_executed=False, timing_measured=False,
                gcp_used=False, direct_reference_sha256=PINS, transitive_reference_sha256=reference.PINS,
                configurations=38, model_calls=38 * 2 * 3, closed_r2_replays=36,
                changed_policy='only uncertain Z==current A refines Z before a query factor',
                unchanged='B0, witness order, count/cursor/phase, fixed-anchor handoff and global payload',
                comparison_summary=summary, rows=rows,
                bounds_backend=closed['bounds_backend'], suspension_scope=closed['suspension_scope'])


if __name__ == '__main__':
    try:
        print(json.dumps(selftest(), sort_keys=True))
    except reference.Failure as error:
        print(json.dumps(dict(status='failed', error=error.code, evidence=error.evidence)), file=sys.stderr)
        raise
