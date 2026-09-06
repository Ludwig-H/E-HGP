#!/usr/bin/env python3
"""Pure small rational models. No C++, product imports or campaign claims."""
import copy
from fractions import Fraction as Q
import hashlib
import importlib.util
from itertools import combinations
import json
import math
from pathlib import Path

PATH = Path(__file__).with_name('read_extra_shell.py')
SPEC = importlib.util.spec_from_file_location('independent_reader', PATH)
r = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(r)


def need(value, reason):
    if not value:
        raise ValueError('selftest: ' + reason)


def affine_contains(points, centre):
    """Separate rectangular affine feasibility, not the reader's Gram sphere."""
    for q in range(1, min(4, len(points))+1):
        for support in combinations(points, q):
            rows = [[Q(point[axis]) for point in support] + [Q(centre[axis])] for axis in range(3)]
            rows.append([Q(1)]*(q+1))
            next_row, pivots = 0, []
            for col in range(q):
                pivot = next((i for i in range(next_row, 4) if rows[i][col]), None)
                if pivot is None:
                    break
                rows[next_row], rows[pivot] = rows[pivot], rows[next_row]
                divisor = rows[next_row][col]
                rows[next_row] = [value/divisor for value in rows[next_row]]
                for i in range(4):
                    if i != next_row:
                        factor = rows[i][col]
                        rows[i] = [a-factor*b for a, b in zip(rows[i], rows[next_row])]
                pivots.append((next_row, col))
                next_row += 1
            if len(pivots) != q:
                continue
            if any(all(value == 0 for value in row[:-1]) and row[-1] != 0 for row in rows):
                continue
            if all(rows[row][-1] >= 0 for row, _col in pivots):
                return True
    return False


def fixture(shell, interiors, centre, radius, arity):
    # Reverse shell order so input identity is not geometric ordering.
    cloud = r.Cloud([*reversed(shell), *interiors, (65535, 65535, 65535)])
    key = r.primitive_key(tuple(map(Q, centre)), Q(radius))
    groups = {'interior': [], 'shell': []}
    for point_id, point in enumerate(cloud.points):
        power = key[0]*sum(x*x for x in point) + sum(a*b for a, b in zip(key[1:4], point)) + key[4]
        if power <= 0:
            groups['interior' if power < 0 else 'shell'].append(dict(
                point_id=point_id, geometry_index=cloud.geometry_index[point_id], xyz=list(point)))
    n = len(cloud.points)
    record = dict(schema=r.SCHEMA, type='extra_shell', diagnostic_only=True,
        n=n, s=8, kmax_requested=10, smax=min(n, 11), input_digest=cloud.digest, ball_index=0,
        ball_key=dict(a=str(key[0]), b=[str(value) for value in key[1:4]], c=str(key[4])),
        squared_radius=dict(numerator_u64_le=[Q(radius).numerator, 0, 0], denominator=str(Q(radius).denominator)),
        minimal_arity=arity, **groups)
    return cloud, record


def crosscheck(cloud, record, answer):
    centre = tuple(Q(value['numerator'])/Q(value['denominator']) for value in answer['centre'])
    interior, shell = answer['interior_ids'], answer['shell_mask_point_ids']
    mask_for = {point: 1 << i for i, point in enumerate(shell)}
    table = [affine_contains([cloud.points[point] for i, point in enumerate(shell) if mask & (1 << i)], centre)
             for mask in range(1 << len(shell))]
    need(table == answer['centre_in_convex_hull_by_mask'], 'full mask table vs affine feasibility')
    minimal_masks = {mask for mask, contains in enumerate(table) if contains and
                     all(not table[mask ^ (1 << bit)] for bit in range(len(shell)) if mask & (1 << bit))}
    need(minimal_masks == {support['shell_mask'] for support in answer['all_minimal_positive_supports']},
         'all inclusion-minimal supports vs independent mask minima')
    sites = sorted(interior + shell)
    total_facets = 0
    for row in answer['local_quotients']:
        k = row['k']
        if k > len(sites):
            need(row['block'] == 'empty' and row['closed_facets'] == 0, 'empty K beyond local carrier')
            continue
        def strict(facet):
            return not table[sum(mask_for.get(point, 0) for point in facet)]
        facets = [frozenset(facet) for facet in combinations(sites, k) if strict(facet)]
        total_facets += len(facets)
        parent = list(range(len(facets)))
        def find(i):
            while parent[i] != i:
                i = parent[i]
            return i
        for i in range(len(facets)):
            for j in range(i):
                union = facets[i] | facets[j]
                if len(union) == k+1 and strict(union):
                    a, b = find(i), find(j)
                    parent[a] = b
        groups = {}
        for i, facet in enumerate(facets):
            groups.setdefault(find(i), []).append(facet)
        expected = []
        for group in groups.values():
            coverage = sorted(set().union(*group))
            if k <= len(interior):
                masks = None
            else:
                masks = sorted(sum(mask_for.get(point, 0) for point in facet) for facet in group
                               if set(interior) <= facet)
                need(masks, 'each full strict component reaches an absorbed-interior representative')
            expected.append((masks, coverage))
        expected.sort(key=lambda item: str(item[0]))
        actual = [(group['shell_masks'], group['coverage']) for group in row['strict_components']]
        actual.sort(key=lambda item: str(item[0]))
        need(actual == expected, 'component partitions AND coverage vs exhaustive local graph')
        need(row['strict_facets'] == len(facets) and row['closed_facets'] == math.comb(len(sites), k), 'facet counts')
        need(row['global_parent_count'] is None, 'no global parent promotion')
    return len(table), total_facets


def main():
    generator = r.MT19937(5489)
    need([generator.next() for _ in range(5)] == [3499211612, 581869302, 3890346734, 3586334585, 545404204], 'MT canonical vector')
    known = {8: '3c95479be68aeb3af8d367b56f6a78e4fa07a0c193ee8bd33d1dece00b31e94d',
             8000: 'b73744755477b18a5853084851075bb4e3e468ae7d1353c98d0991576a099639'}
    for n, digest in known.items():
        need(r.input_digest(r.uniform(n)) == digest, 'independent input recipe vs historical input digest')
    square = [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)]
    models = [fixture(square, [], (1, 1, 0), 2, 2),
              fixture(square, [(1, 1, 0)], (1, 1, 0), 2, 2),
              fixture([(10, 5, 5), (5, 10, 5), (2, 1, 5), (9, 8, 5)], [(5, 5, 5)], (5, 5, 5), 25, 3),
              fixture([(15, 15, 15), (15, 5, 5), (5, 15, 5), (5, 5, 15), (11, 15, 17)],
                      [(10, 10, 10), (11, 10, 10)], (10, 10, 10), 75, 4)]
    summaries, mask_count, facet_count = [], 0, 0
    for cloud, record in models:
        answer = r.judge(r.loads(json.dumps(record)), cloud)
        masks, facets = crosscheck(cloud, record, answer)
        mask_count += masks
        facet_count += facets
        summaries.append(dict(q_min=answer['q_min'], supports=len(answer['all_minimal_positive_supports']),
                              interior=len(answer['interior_ids']), shell=len(answer['shell_mask_point_ids'])))
    cloud, base = models[1]
    need(base['ball_key'] == dict(a='1', b=['-2', '-2', '0'], c='0'), 'literal square key')
    unreduced = copy.deepcopy(base)
    unreduced['squared_radius'] = dict(numerator_u64_le=[6, 0, 0], denominator='3')
    need(r.judge(unreduced, cloud) == r.judge(base, cloud), 'unreduced exact level accepted without changing result')
    rejects = []
    def reject(name, action, cause):
        try:
            action()
        except ValueError as error:
            need(cause in str(error), name + ': wrong rejection cause ' + str(error))
            rejects.append(name)
        else:
            raise ValueError('selftest mutant accepted: ' + name)
    def mutate(name, edit, cause):
        changed = copy.deepcopy(base)
        edit(changed)
        reject(name, lambda: r.judge(changed, cloud), cause)
    for name, raw in [('duplicate_top', '{"n":8,"n":8}'), ('duplicate_nested', '{"key":{"a":"1","a":"1"}}')]:
        reject(name, lambda raw=raw: r.loads(raw), 'duplicate JSON key')
    for raw in ('1.0', 'NaN', 'Infinity', '-Infinity', '1e999'):
        reject('float_' + raw, lambda raw=raw: r.loads(raw), 'floating/nonfinite')
    mutate('unknown_field', lambda row: row.update(extra=True), 'field inventory')
    mutate('not_diagnostic', lambda row: row.update(diagnostic_only=False), 'diagnostic schema')
    mutate('bool_n', lambda row: row.update(n=True), 'n domain')
    mutate('digest', lambda row: row.update(input_digest='0'*64), 'input digest')
    mutate('rank_window', lambda row: row.update(smax=2), 'rank window')
    for value in ('+1', '01', '-0', str(1 << 127)):
        mutate('decimal_' + value, lambda row, value=value: row['ball_key'].update(a=value),
               'i128 decimal domain' if value == str(1 << 127) else 'canonical decimal')
    mutate('scaled_key', lambda row: row.update(ball_key=dict(a='2', b=['-4', '-4', '0'], c='0')), 'primitive positive')
    mutate('negative_key', lambda row: row.update(ball_key=dict(a='-1', b=['2', '2', '0'], c='0')), 'primitive positive')
    mutate('wrong_c', lambda row: row['ball_key'].update(c='1'), 'exact squared radius')
    mutate('wrong_b_shape', lambda row: row['ball_key'].update(b=['-2', '-2']), 'key shape')
    mutate('zero_denominator', lambda row: row['squared_radius'].update(denominator='0'), 'exact squared radius')
    mutate('negative_denominator', lambda row: row['squared_radius'].update(denominator='-1'), 'exact squared radius')
    mutate('wrong_radius', lambda row: row['squared_radius'].update(numerator_u64_le=[3, 0, 0]), 'exact squared radius')
    mutate('bool_limb', lambda row: row['squared_radius'].update(numerator_u64_le=[True, 0, 0]), 'U192 limb')
    mutate('large_limb', lambda row: row['squared_radius'].update(numerator_u64_le=[1 << 64, 0, 0]), 'U192 limb')
    mutate('missing_shell', lambda row: row['shell'].pop(), 'complete I/U census')
    mutate('missing_interior', lambda row: row['interior'].pop(), 'complete I/U census')
    mutate('duplicate_point', lambda row: row['shell'].append(copy.deepcopy(row['shell'][0])), 'unique IDs')
    mutate('bool_point_id', lambda row: row['shell'][0].update(point_id=True), 'point identity')
    mutate('geometry_index', lambda row: row['shell'][0].update(geometry_index=(row['shell'][0]['geometry_index']+1) % len(cloud.points)), 'Morton binding')
    mutate('coordinates', lambda row: row['shell'][0].update(xyz=[100, 100, 100]), 'coordinate binding')
    mutate('interior_as_shell', lambda row: row['shell'].append(row['interior'].pop()), 'power sign')
    mutate('wrong_minimal_arity', lambda row: row.update(minimal_arity=3), 'q_min')
    no_support_cloud, no_support = fixture([(10, 5, 0), (9, 8, 0), (8, 9, 0)], [], (5, 5, 0), 25, 2)
    reject('no_positive_support', lambda: r.judge(no_support, no_support_cloud), 'q_min')
    need(len(rejects) == 34 and mask_count == 80 and {row['q_min'] for row in summaries} == {2, 3, 4}, 'test non-vacuity')
    print(json.dumps(dict(status='passed', diagnostic_only=True, real_50k_traces_qualified=False,
        reader_sha256=hashlib.sha256(PATH.read_bytes()).hexdigest(), generator_digest_checks=len(known),
        positive_geometries=summaries, unreduced_radius_positive=1, independent_mask_checks=mask_count,
        exhaustive_strict_facets=facet_count, rejected_mutants=len(rejects), mutant_names=rejects), sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
