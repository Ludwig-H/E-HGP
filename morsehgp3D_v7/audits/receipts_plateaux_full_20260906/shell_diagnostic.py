"""Strict local analysis of bounded extra-shell diagnostics, never a run judge."""

from __future__ import annotations

import argparse
from fractions import Fraction
from itertools import combinations
import json
import math
from pathlib import Path
import re
import sys
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from meb_rational_oracle_20260905 import circumball

INPUT_SCHEMA = "mhgp7-extra-shell-diagnostic-v1"
RECORD_KEYS = frozenset({
    'schema', 'type', 'diagnostic_only', 'n', 's', 'kmax_requested', 'smax',
    'input_digest', 'ball_index', 'ball_key', 'squared_radius',
    'minimal_arity', 'interior', 'shell',
})
POINT_KEYS = frozenset({'geometry_index', 'point_id', 'xyz'})
DECIMAL = re.compile(r'(?:0|[1-9][0-9]*|-[1-9][0-9]*)\Z')
U64_MAX = (1 << 64) - 1
I128_MIN, I128_MAX = -(1 << 127), (1 << 127) - 1


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def integer(value: Any, lower: int, upper: int, reason: str) -> int:
    need(type(value) is int and lower <= value <= upper, reason)
    return value


def decimal(value: Any, reason: str, positive: bool = False) -> int:
    need(type(value) is str and len(value) <= 40 and DECIMAL.fullmatch(value) is not None,
         reason)
    result = int(value)
    need(I128_MIN <= result <= I128_MAX and (not positive or result > 0), reason)
    return result


def header(record: Any) -> None:
    need(type(record) is dict and set(record) == RECORD_KEYS, 'record_keys')
    need(record['schema'] == INPUT_SCHEMA, 'schema')
    need(record['type'] == 'extra_shell', 'type')
    need(record['diagnostic_only'] is True, 'diagnostic_only')


def unique_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        need(key not in result, 'duplicate_json_key')
        result[key] = value
    return result


def reject_constant(value: str) -> None:
    raise ValueError('nonfinite_json_number:' + value)


def parse_records(text: str) -> list[dict[str, Any]]:
    """Parse only diagnostic JSONL; return raw records, not their analyses."""
    need(type(text) is str, 'text_type')
    records = []
    for line_number, line in enumerate(text.split('\n'), 1):
        if not line.strip(' \t\r'):
            continue
        try:
            record = json.loads(line, object_pairs_hook=unique_object,
                                parse_constant=reject_constant)
            header(record)
        except ValueError as error:
            raise ValueError(f'line_{line_number}:{error}') from error
        records.append(record)
    return records


def points(rows: Any, maximum: int, n: int, reason: str) -> list[dict[str, Any]]:
    need(type(rows) is list and len(rows) <= maximum, reason)
    result = []
    for row in rows:
        need(type(row) is dict and set(row) == POINT_KEYS, 'point_keys')
        geometry_index = integer(row['geometry_index'], 0, n - 1, 'geometry_index')
        point_id = integer(row['point_id'], 0, (1 << 32) - 1, 'point_id')
        xyz = row['xyz']
        need(type(xyz) is list and len(xyz) == 3, 'xyz')
        coordinates = tuple(integer(value, 0, 65535, 'xyz') for value in xyz)
        result.append(dict(geometry_index=geometry_index, point_id=point_id, xyz=coordinates))
    return sorted(result, key=lambda row: row['point_id'])


def mask_components(table: list[bool], size: int, shell_size: int) -> tuple[list[list[int]], int]:
    """The local shell graph only: strict size+1 masks connect faces by stars."""
    vertices = [mask for mask, contains in enumerate(table)
                if not contains and mask.bit_count() == size]
    parent = {mask: mask for mask in vertices}

    def root(mask: int) -> int:
        while parent[mask] != mask:
            parent[mask] = parent[parent[mask]]
            mask = parent[mask]
        return mask

    unions = 0
    for mask, contains in enumerate(table):
        if contains or mask.bit_count() != size + 1:
            continue
        faces = [mask ^ (1 << bit) for bit in range(shell_size) if mask & (1 << bit)]
        for face in faces[1:]:
            left, right = root(faces[0]), root(face)
            if left != right:
                parent[right] = left
            unions += 1
    components: dict[int, list[int]] = {}
    for mask in vertices:
        components.setdefault(root(mask), []).append(mask)
    return sorted(components.values(), key=lambda component: min(component)), unions


def analyze(record: dict[str, Any]) -> dict[str, Any]:
    """Certify supplied local geometry and return strict shell components by K."""
    header(record)
    n = integer(record['n'], 2, (1 << 31) - 1, 'n')
    s = integer(record['s'], 8, 12, 's')
    need(s in (8, 10, 12), 's')
    kmax = integer(record['kmax_requested'], 5, 10, 'kmax_requested')
    need(kmax in (5, 10), 'kmax_requested')
    smax = integer(record['smax'], 2, 11, 'smax')
    need(smax == min(n, kmax + 1), 'smax')
    integer(record['ball_index'], 0, U64_MAX, 'ball_index')
    need(type(record['input_digest']) is str and
         re.fullmatch(r'[0-9a-f]{64}', record['input_digest']) is not None, 'input_digest')
    arity = integer(record['minimal_arity'], 2, 4, 'minimal_arity')
    interior = points(record['interior'], 9, n, 'interior_bound')
    shell = points(record['shell'], 12, n, 'shell_bound')
    p, u = len(interior), len(shell)
    need(u > arity, 'extra_shell')
    need(p + arity <= smax, 'support_rank_window')
    all_points = interior + shell
    for key, reason in (('geometry_index', 'duplicate_geometry_index'),
                        ('point_id', 'duplicate_point_id'), ('xyz', 'duplicate_position')):
        need(len({point[key] for point in all_points}) == len(all_points), reason)

    key = record['ball_key']
    need(type(key) is dict and set(key) == {'a', 'b', 'c'}, 'ball_key_keys')
    a = decimal(key['a'], 'ball_key_a', positive=True)
    need(type(key['b']) is list and len(key['b']) == 3, 'ball_key_b')
    b = tuple(decimal(value, 'ball_key_b') for value in key['b'])
    c = decimal(key['c'], 'ball_key_c')
    ball_key = (a, *b, c)
    need(math.gcd(*ball_key) == 1, 'ball_key_primitive')
    radius = Fraction(sum(value * value for value in b) - 4 * a * c, 4 * a * a)
    need(radius > 0, 'positive_radius')
    stored_radius = record['squared_radius']
    need(type(stored_radius) is dict and set(stored_radius) ==
         {'numerator_u64_le', 'denominator'}, 'squared_radius_keys')
    limbs = stored_radius['numerator_u64_le']
    need(type(limbs) is list and len(limbs) == 3, 'radius_numerator')
    numerator = sum(integer(value, 0, U64_MAX, 'radius_numerator') << (64 * index)
                    for index, value in enumerate(limbs))
    denominator = decimal(stored_radius['denominator'], 'radius_denominator', positive=True)
    need(Fraction(numerator, denominator) == radius, 'radius_mismatch')
    for population, reason, strict in ((interior, 'interior_power', True),
                                        (shell, 'shell_power', False)):
        for point in population:
            xyz = point['xyz']
            value = a * sum(x * x for x in xyz) + sum(x * y for x, y in zip(b, xyz)) + c
            need(value < 0 if strict else value == 0, reason)

    basis_masks = []
    support_tests = 0
    for size in range(2, min(4, u) + 1):
        for indices in combinations(range(u), size):
            support_tests += 1
            candidate = circumball([shell[index]['xyz'] for index in indices])
            if candidate is not None and candidate['positive'] and tuple(candidate['key']) == ball_key:
                basis_masks.append(sum(1 << index for index in indices))
    need(bool(basis_masks), 'positive_shell_basis_missing')
    qmin = min(mask.bit_count() for mask in basis_masks)
    need(qmin == arity, 'minimal_arity_mismatch')
    need(support_tests <= 781, 'support_test_bound')
    table = [False] * (1 << u)
    for mask in basis_masks:
        table[mask] = True
    for bit in range(u):
        for mask in range(len(table)):
            if mask & (1 << bit):
                table[mask] = table[mask] or table[mask ^ (1 << bit)]
    h = max(mask.bit_count() for mask, contains in enumerate(table) if not contains)
    inside_ids = [point['point_id'] for point in interior]
    shell_ids = [point['point_id'] for point in shell]
    all_ids = set(inside_ids + shell_ids)
    effective_kmax = min(n, kmax)
    lower, upper = max(1, p + qmin - 1), min(effective_kmax, p + u)
    anchor_interval = [lower, upper] if lower <= upper else []

    def point_ids(mask: int) -> list[int]:
        return [point_id for bit, point_id in enumerate(shell_ids) if mask & (1 << bit)]

    orders = []
    for k in range(1, effective_kmax + 1):
        components = []
        union_attempts = 0
        construction = 'empty' if k > p + u else 'interior_connected' if k <= p else 'shell_masks'
        if k <= p:
            components.append(dict(mask_count=0, representative_point_ids=inside_ids[:k],
                                   coverage_point_ids=sorted(all_ids)))
        elif k <= p + u:
            mask_groups, union_attempts = mask_components(table, k - p, u)
            for masks in mask_groups:
                coverage_mask = 0
                for mask in masks:
                    coverage_mask |= mask
                components.append(dict(mask_count=len(masks),
                    representative_point_ids=sorted(inside_ids + point_ids(min(masks))),
                    coverage_point_ids=sorted(inside_ids + point_ids(coverage_mask))))
        covered = {point_id for component in components for point_id in component['coverage_point_ids']}
        count = len(components)
        status = ('empty' if k > p + u else 'local_birth' if count == 0 else
                  ('needs_global_parents' if covered == all_ids else
                   'needs_global_parents_and_coverage') if count >= 2 else
                  'local_inert' if covered == all_ids else 'needs_global_coverage')
        orders.append(dict(k=k, status=status, strict_component_count=count,
            construction=construction, components=components,
            strict_covered_point_ids=sorted(covered), local_uncovered_point_ids=sorted(all_ids - covered),
            local_union_attempts=union_attempts,
            anchor_in_proposed_interval=bool(anchor_interval and lower <= k <= upper)))
    context_keys = ('schema', 'type', 'diagnostic_only', 'n', 's', 'kmax_requested',
                    'smax', 'input_digest', 'ball_index')
    return dict(schema='mhgp7-extra-shell-local-analysis-v1', type='extra_shell_local_analysis',
        diagnostic_only=True, local_only=True, complete_census_verified=False,
        global_parents_verified=False, public_status='not_claimed',
        provenance={name: record[name] for name in context_keys},
        ball_key=dict(a=key['a'], b=key['b'].copy(), c=key['c']),
        squared_radius=dict(numerator_u64_le=limbs.copy(), denominator=stored_radius['denominator']),
        p=p, u=u, qmin=qmin, h=h, support_tests=support_tests,
        support_test_scope='combinations_of_supplied_shell_sites_2_to_4_not_time_or_engine_work',
        positive_basis_count=len(basis_masks), positive_basis_masks=sorted(basis_masks),
        shell_table_entries=len(table), shell_mask_point_ids=shell_ids,
        interior_point_ids=inside_ids, complete_local_point_ids=sorted(all_ids),
        effective_kmax=effective_kmax, anchor_interval=anchor_interval, orders=orders,
        birth_condition='local_birth_requires_a_complete_global_census',
        global_resolution_scope='local_components_are_representatives_not_distinct_global_parents')


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--records', required=True, type=Path)
    parser.add_argument('--expected-count', required=True, type=int)
    arguments = parser.parse_args(argv)
    try:
        need(arguments.expected_count >= 0, 'expected_count')
        records = parse_records(arguments.records.read_text(encoding='utf-8'))
        need(len(records) == arguments.expected_count, 'expected_count')
        analyses = [analyze(record) for record in records]
        output = json.dumps(dict(schema='mhgp7-extra-shell-local-report-v1',
            local_only=True, complete_census_verified=False, global_parents_verified=False,
            public_status='not_claimed', record_count=len(analyses), records=analyses),
            allow_nan=False, sort_keys=True, separators=(',', ':')) + '\n'
    except (ValueError, OSError) as error:
        print('shell_diagnostic: ' + str(error), file=sys.stderr)
        return 2
    sys.stdout.write(output)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
