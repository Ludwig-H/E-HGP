"""Independent bounded geometry/partition and wire checks for the local reader.

Synthetic records only: these are not the constructor's 50k extra-shells.
The old exhaustive Gram model is used only as a small differential judge.
"""

from __future__ import annotations

from copy import deepcopy
from fractions import Fraction
import hashlib
from itertools import combinations
import json
from pathlib import Path
import subprocess
import sys
from typing import Any

from plateau_model import Model, groups
from shell_diagnostic import analyze, parse_records

HERE = Path(__file__).resolve().parent


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def point_id(index: int) -> int:
    return (1 << 32) - 1 if index == 0 else 1000 + 97 * index


def record(ball: dict[str, Any], points: list[tuple[int, int, int]],
           kmax: int = 5, scale: int = 1) -> dict[str, Any]:
    numerator = ball['radius'].numerator * scale
    denominator = ball['radius'].denominator * scale

    def rows(ids: Any) -> list[dict[str, Any]]:
        return [dict(geometry_index=i, point_id=point_id(i), xyz=list(points[i]))
                for i in sorted(ids, reverse=True)]

    key = ball['key']
    return dict(schema='mhgp7-extra-shell-diagnostic-v1', type='extra_shell',
                diagnostic_only=True, n=len(points), s=8,
                kmax_requested=kmax, smax=min(len(points), kmax + 1),
                input_digest=hashlib.sha256(json.dumps(points).encode()).hexdigest(),
                ball_index=0, ball_key=dict(a=str(key[0]), b=list(map(str, key[1:4])),
                                          c=str(key[4])),
                squared_radius=dict(numerator_u64_le=[(numerator >> (64 * i)) & ((1 << 64) - 1)
                                                     for i in range(3)],
                                    denominator=str(denominator)),
                minimal_arity=min(map(len, ball['bases'])),
                interior=rows(ball['interior']), shell=rows(ball['shell']))


def strict_parts(model: Model, ball: dict[str, Any], k: int) -> list[Any]:
    # This judge uses each complete subset's MEB, not the reader's hull table.
    sites = sorted(ball['closed'])
    vertices = [f for f in combinations(sites, k) if model.level[f] < ball['radius']]
    edges = []
    for coface in combinations(sites, k + 1):
        if model.level[coface] < ball['radius']:
            faces = list(combinations(coface, k))
            edges.extend((faces[0], f) for f in faces[1:])
    return groups(vertices, edges)


def compare_local(model: Model, ball: dict[str, Any], value: dict[str, Any]) -> int:
    inverse = {point_id(i): i for i in model.ids}
    compared = 0
    for order in value['orders']:
        k = order['k']
        expected = strict_parts(model, ball, k)
        need(order['strict_component_count'] == len(expected), 'strict_component_count')
        matched = set()
        all_covered = set()
        for component in order['components']:
            representative = tuple(sorted(inverse[i] for i in component['representative_point_ids']))
            images = [i for i, part in enumerate(expected) if representative in part]
            need(len(images) == 1 and images[0] not in matched, 'representative_bijection_not_coverage_only')
            matched.add(images[0])
            covered = {point_id(i) for facet in expected[images[0]] for i in facet}
            need(component['coverage_point_ids'] == sorted(covered), 'component_coverage')
            all_covered |= covered
        need(len(matched) == len(expected), 'all_local_components_represented')
        sites = {point_id(i) for i in ball['closed']}
        status = ('empty' if k > len(sites) else 'local_birth' if not expected else
                  'local_inert' if len(expected) == 1 and all_covered == sites else
                  'needs_global_coverage' if len(expected) == 1 else
                  'needs_global_parents' if all_covered == sites else
                  'needs_global_parents_and_coverage')
        need(order['status'] == status, 'conditional_local_classification')
        need(order['strict_covered_point_ids'] == sorted(all_covered), 'local_union')
        if k <= len(sites):
            need(order['local_uncovered_point_ids'] == sorted(sites - all_covered), 'possible_gap')
        compared += 1
    need(value['complete_census_verified'] is False and value['global_parents_verified'] is False,
         'local_geometry_is_not_global_authority')
    return compared


def rejected(value: dict[str, Any], reason: str) -> None:
    try:
        analyze(value)
    except ValueError as error:
        need(str(error) == reason, 'wrong_rejection:' + str(error) + ':' + reason)
    else:
        raise ValueError('accepted_invalid:' + reason)


def main() -> None:
    source = json.loads((HERE / 'normal.json').read_text())
    models = {name: Model([tuple(p) for p in points]) for name, points in source['clouds'].items()}
    counts = {'records_compared': 0, 'orders_compared': 0}
    for model in models.values():
        for ball in model.balls.values():
            if len(ball['shell']) <= min(map(len, ball['bases'])):
                continue
            value = analyze(record(ball, model.points, 10))
            counts['records_compared'] += 1
            counts['orders_compared'] += compare_local(model, ball, value)
    need(counts['records_compared'] > 0, 'nonvacuous_extra_shell_differential')

    square = Model([(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0), (1, 1, 0)])
    square_ball = square.balls[square.key[(0, 1, 2, 3)]]
    nominal = record(square_ball, square.points)
    compare_local(square, square_ball, analyze(nominal))
    for field in ['interior', 'shell']:
        permuted = deepcopy(nominal)
        permuted[field].reverse()
        need(analyze(permuted)['orders'] == analyze(nominal)['orders'], 'permutation_independent_local_orders')

    growth = models['coverage_growth']
    growth_ball = growth.balls[growth.key[(0, 1, 2, 3)]]
    wide = record(growth_ball, growth.points, scale=(1 << 125) + 1)
    need(wide['squared_radius']['numerator_u64_le'][2] > 0, 'third_limb_nonvacuous')
    need(analyze(wide)['orders'] == analyze(record(growth_ball, growth.points))['orders'],
         'unreduced_exact_level_and_third_limb')

    interior = [(5 + x, 5 + y, 0) for x in (-1, 0, 1) for y in (-1, 0, 1)]
    shell = [(10, 5, 0), (0, 5, 0), (5, 10, 0), (5, 0, 0)]
    shell += [(5 + x, 5 + y, 0) for x, y in [(3, 4), (3, -4), (-3, 4), (-3, -4),
                                           (4, 3), (4, -3), (-4, 3), (-4, -3)]]
    maximum_ball = dict(key=[1, -10, -10, 0, 25], radius=Fraction(25),
                        interior=frozenset(range(9)), shell=frozenset(range(9, 21)),
                        bases=[frozenset({9, 10})])
    maximum = record(maximum_ball, interior + shell, 10)
    maximum_report = analyze(maximum)
    need(maximum_report['qmin'] == 2 and maximum_report['h'] == 6 and
         maximum_report['shell_table_entries'] == 4096 and maximum_report['support_tests'] == 781,
         'maximum_profile_shell_without_21_site_powerset')
    need(maximum_report['orders'][-1]['status'] == 'local_inert' and
         maximum_report['orders'][-1]['anchor_in_proposed_interval'], 'inert_anchor_stays_declared')
    need(len(maximum_report['orders'][-1]['strict_covered_point_ids']) == 21, 'maximum_coverage')

    variants = []

    def bad(reason: str, mutation: Any) -> None:
        value = deepcopy(nominal)
        mutation(value)
        rejected(value, reason)
        variants.append(reason)

    bad('record_keys', lambda v: v.update(extra=True))
    bad('schema', lambda v: v.update(schema='future'))
    bad('diagnostic_only', lambda v: v.update(diagnostic_only=1))
    bad('n', lambda v: v.update(n=True))
    bad('smax', lambda v: v.update(smax=6))
    bad('input_digest', lambda v: v.update(input_digest='G' * 64))
    bad('ball_index', lambda v: v.update(ball_index=-1))
    bad('ball_key_a', lambda v: v['ball_key'].update(a='01'))
    bad('ball_key_a', lambda v: v['ball_key'].update(a=str(1 << 127)))
    bad('ball_key_primitive', lambda v: v.update(ball_key=dict(a='2', b=['-4', '-4', '0'], c='0')))
    bad('radius_numerator', lambda v: v['squared_radius'].update(numerator_u64_le=[2, 0, 1 << 64]))
    bad('radius_denominator', lambda v: v['squared_radius'].update(denominator='0'))
    bad('radius_mismatch', lambda v: v['squared_radius'].update(numerator_u64_le=[3, 0, 0]))
    bad('geometry_index', lambda v: v['shell'][0].update(geometry_index=5))
    bad('point_id', lambda v: v['shell'][0].update(point_id=1 << 32))
    bad('xyz', lambda v: v['shell'][0].update(xyz=[65536, 0, 0]))
    bad('duplicate_geometry_index', lambda v: v['shell'][0].update(geometry_index=v['interior'][0]['geometry_index']))
    bad('duplicate_point_id', lambda v: v['shell'][0].update(point_id=v['interior'][0]['point_id']))
    bad('duplicate_position', lambda v: v['shell'][0].update(xyz=v['interior'][0]['xyz']))
    bad('interior_power', lambda v: v['interior'][0].update(xyz=[1, 3, 0]))
    bad('shell_power', lambda v: v['shell'][0].update(xyz=[0, 1, 0]))
    bad('minimal_arity_mismatch', lambda v: v.update(minimal_arity=3))
    try:
        parse_records('{"schema":"x","schema":"y"}\n')
    except ValueError as error:
        need(str(error) == 'line_1:duplicate_json_key', 'duplicate_JSON_rejection')
    else:
        raise ValueError('duplicate_JSON_accepted')

    omitted = deepcopy(nominal)
    omitted['shell'] = [row for row in omitted['shell'] if row['geometry_index'] != 1]
    omitted_report = analyze(omitted)
    need(omitted_report['complete_census_verified'] is False, 'missing_shell_not_globally_certified')

    expected_inputs = [nominal, wide, maximum]
    fixture = HERE / 'shell_diagnostic_inputs.jsonl'
    need(parse_records(fixture.read_text()) == expected_inputs, 'pinned_synthetic_wire_inputs')
    python = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else [])
    argv = python + [str(HERE / 'shell_diagnostic.py'), '--records', str(fixture)]
    normal = subprocess.run(argv + ['--expected-count', '3'], capture_output=True)
    need(normal.returncode == 0 and not normal.stderr, 'CLI_positive')
    parsed = json.loads(normal.stdout)
    need(parsed['record_count'] == 3 and parsed['complete_census_verified'] is False, 'CLI_scope')
    wrong_count = subprocess.run(argv + ['--expected-count', '4'], capture_output=True)
    need(wrong_count.returncode == 2 and not wrong_count.stdout, 'CLI_count_rejection_transactional')
    late_bad = deepcopy(wide)
    late_bad['squared_radius']['numerator_u64_le'][0] += 1
    invalid_fixture = HERE / 'shell_diagnostic_invalid_last.jsonl'
    need(parse_records(invalid_fixture.read_text()) == [nominal, late_bad], 'pinned_late_invalid_input')
    late = subprocess.run(python + [str(HERE / 'shell_diagnostic.py'), '--records',
                                   str(invalid_fixture), '--expected-count', '2'], capture_output=True)
    need(late.returncode == 2 and not late.stdout and b'radius_mismatch' in late.stderr,
         'CLI_late_geometry_rejection_has_no_valid_prefix')
    output = dict(status='passed', public_status='not_claimed', synthetic_records_only=True,
                  differential=counts, format_geometry_rejections=variants,
                  duplicate_json_rejected=True, permutation_checks=2,
                  unreduced_third_limb_checked=True, incomplete_census_not_promoted=True,
                  maximum_profile=maximum_report,
                  cli_codes=[normal.returncode, wrong_count.returncode, late.returncode],
                  cli_positive_stdout_sha256=hashlib.sha256(normal.stdout).hexdigest(),
                  engine_executed=False, GCP_used=False)
    print(json.dumps(output, sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
