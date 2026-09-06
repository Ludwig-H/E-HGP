#!/usr/bin/env python3
"""Independent rational extra-shell diagnostic, not a global FULL producer.

No product/oracle imports. MT19937+high16 independently reconstructs the
uniform/seed3/coord65536 campaign; the reported FULL input hash must agree.
Only supplied balls are scanned against the complete input, never all balls.
Local shell quotients follow the mathematical reduction by interior points.
The current diagnostic's shell-table domain is u<=12, explicitly checked.
"""
import argparse
from fractions import Fraction as Q
import hashlib
from itertools import combinations
import json
import math
from pathlib import Path
import re
import sys

SCHEMA = 'mhgp7-extra-shell-diagnostic-v1'
FIELDS = {'schema', 'type', 'diagnostic_only', 'n', 's', 'kmax_requested', 'smax', 'input_digest',
          'ball_index', 'ball_key', 'squared_radius', 'minimal_arity', 'interior', 'shell'}


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def integer(value, lo, hi, reason):
    need(type(value) is int and lo <= value <= hi, reason)
    return value


def decimal(value):
    need(type(value) is str and re.fullmatch(r'0|-?[1-9][0-9]*', value), 'canonical decimal string')
    parsed = int(value)
    need(-(1 << 127) <= parsed < 1 << 127, 'i128 decimal domain')
    return parsed


def loads(raw):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            need(key not in result, 'duplicate JSON key')
            result[key] = value
        return result
    def no_float(_):
        raise ValueError('floating/nonfinite value forbidden in rational diagnostic')
    return json.loads(raw, object_pairs_hook=unique, parse_float=no_float, parse_constant=no_float)


class MT19937:
    def __init__(self, seed):
        self.words = [seed & 0xffffffff]
        for i in range(1, 624):
            previous = self.words[-1]
            self.words.append((1812433253 * (previous ^ (previous >> 30)) + i) & 0xffffffff)
        self.index = 624

    def next(self):
        if self.index == 624:
            for i in range(624):
                joined = (self.words[i] & 0x80000000) | (self.words[(i+1) % 624] & 0x7fffffff)
                self.words[i] = self.words[(i+397) % 624] ^ (joined >> 1) ^ (0x9908b0df if joined & 1 else 0)
            self.index = 0
        word = self.words[self.index]
        self.index += 1
        word ^= word >> 11
        word ^= (word << 7) & 0x9d2c5680
        word ^= (word << 15) & 0xefc60000
        word ^= word >> 18
        return word & 0xffffffff


def uniform(n):
    rng, points, seen = MT19937(3), [], set()
    for _ in range(200*n):
        # Campaign libstdc++'s uint32 multiply-high downscaling to 2^16:
        # word>>16, with threshold zero and therefore no rejection.
        point = tuple(rng.next() >> 16 for _axis in range(3))
        if point not in seen:
            seen.add(point)
            points.append(point)
            if len(points) == n:
                return points
    raise ValueError('uniform generation did not fill n')


def input_digest(points):
    value = hashlib.sha256()
    def number(n):
        value.update(n.to_bytes(8, 'little'))
    tag = b'mhgp7-full-semantic-v1:input'
    number(len(tag))
    value.update(tag)
    number(len(points))
    for point_id, point in enumerate(points):
        for n in (point_id, *point):
            number(n)
    return value.hexdigest()


def morton(point):
    return sum(((point[axis] >> bit) & 1) << (3*bit+axis) for bit in range(16) for axis in range(3))


class Cloud:
    def __init__(self, points):
        self.points = [tuple(point) for point in points]
        need(all(len(point) == 3 and all(type(x) is int and 0 <= x <= 65535 for x in point)
                 for point in self.points), 'input u16 coordinates')
        need(len(set(self.points)) == len(self.points), 'input positions distinct')
        self.digest = input_digest(self.points)
        self.geometry_ids = sorted(range(len(points)), key=lambda i: (morton(self.points[i]), i))
        self.geometry_index = {point_id: i for i, point_id in enumerate(self.geometry_ids)}


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def solve(matrix, right):
    """Gauss-Jordan over rationals; singular affine supports are discarded."""
    rows = [[Q(value) for value in row] + [Q(rhs)] for row, rhs in zip(matrix, right)]
    for col in range(len(rows)):
        pivot = next((i for i in range(col, len(rows)) if rows[i][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        divisor = rows[col][col]
        rows[col] = [value/divisor for value in rows[col]]
        for i in range(len(rows)):
            if i != col:
                factor = rows[i][col]
                rows[i] = [a-factor*b for a, b in zip(rows[i], rows[col])]
    return [row[-1] for row in rows]


def support_ball(points):
    origin = points[0]
    directions = [tuple(x-y for x, y in zip(point, origin)) for point in points[1:]]
    alpha = solve([[dot(x, y) for y in directions] for x in directions], [Q(dot(x, x), 2) for x in directions])
    if alpha is None:
        return None
    weights = [1-sum(alpha), *alpha]
    if any(weight <= 0 for weight in weights):
        return None
    centre = tuple(Q(origin[axis]) + sum(weight*direction[axis] for weight, direction in zip(alpha, directions))
                   for axis in range(3))
    radius = sum((centre[axis]-origin[axis])**2 for axis in range(3))
    return centre, radius, weights


def primitive_key(centre, radius):
    coefficients = [Q(1), *[-2*x for x in centre], dot(centre, centre)-radius]
    scale = math.lcm(*(value.denominator for value in coefficients))
    values = [int(value*scale) for value in coefficients]
    divisor = math.gcd(*values)
    return tuple(value//divisor for value in values)


def rational(value):
    return {'numerator': str(value.numerator), 'denominator': str(value.denominator)}


def local_table(interior_ids, shell_ids, contains, kmax, q_min):
    p, u = len(interior_ids), len(shell_ids)
    sites = sorted(interior_ids + shell_ids)
    h = max(mask.bit_count() for mask, value in enumerate(contains) if not value)
    result = []
    for k in range(1, kmax+1):
        if k > len(sites):
            result.append(dict(k=k, block='empty', closed_facets=0, strict_components=[]))
            continue
        strict_count = sum(math.comb(p, k-mask.bit_count()) for mask in range(1 << u)
                           if not contains[mask] and 0 <= k-mask.bit_count() <= p)
        components = []
        if k <= p:
            components.append(dict(representative=sorted(interior_ids[:k]), coverage=sites,
                                   shell_masks=None, reduction='interior_connected'))
        else:
            t = k-p
            vertices = [mask for mask in range(1 << u) if mask.bit_count() == t and not contains[mask]]
            parent = {mask: mask for mask in vertices}
            def find(mask):
                while parent[mask] != mask:
                    parent[mask] = parent[parent[mask]]
                    mask = parent[mask]
                return mask
            for mask in range(1 << u):
                if mask.bit_count() != t+1 or contains[mask]:
                    continue
                faces = [mask ^ (1 << bit) for bit in range(u) if mask & (1 << bit)]
                for face in faces[1:]:
                    a, b = find(faces[0]), find(face)
                    parent[max(a, b)] = min(a, b)
            groups = {}
            for mask in vertices:
                groups.setdefault(find(mask), []).append(mask)
            for masks in sorted(groups.values(), key=lambda value: value[0]):
                union = 0
                for mask in masks:
                    union |= mask
                representative = sorted(interior_ids + [shell_ids[bit] for bit in range(u) if masks[0] & (1 << bit)])
                coverage = sorted(interior_ids + [shell_ids[bit] for bit in range(u) if union & (1 << bit)])
                components.append(dict(representative=representative, coverage=coverage,
                                       shell_masks=masks, reduction='absorb_all_interiors'))
        need(bool(components) == (strict_count > 0) == (k <= p+h), 'strict quotient nonemptiness')
        covered = sorted({point for group in components for point in group['coverage']})
        sufficient_inert = k <= p+q_min-2
        need(not sufficient_inert or (len(components) == 1 and covered == sites), 'sufficient local inertia')
        result.append(dict(k=k, block='one_closed_group', closed_facets=math.comb(len(sites), k),
            strict_facets=strict_count, newly_present_facets=math.comb(len(sites), k)-strict_count,
            strict_component_count=len(components), strict_components=components, closed_coverage=sites,
            local_strict_coverage=covered, points_absent_from_local_strict_coverages=sorted(set(sites)-set(covered)),
            sufficient_local_inertness=sufficient_inert, global_parent_count=None))
    return h, result


def judge(record, cloud):
    need(type(record) is dict and set(record) == FIELDS, 'diagnostic field inventory')
    need(record['schema'] == SCHEMA and record['type'] == 'extra_shell' and record['diagnostic_only'] is True, 'diagnostic schema')
    n = integer(record['n'], 2, (1 << 30)-1, 'n domain')
    need(n == len(cloud.points) and record['input_digest'] == cloud.digest, 'complete input digest binding')
    integer(record['s'], 8, 12, 's domain')
    need(record['s'] in (8, 10, 12), 's selection')
    kmax = integer(record['kmax_requested'], 5, 10, 'Kmax domain')
    need(kmax in (5, 10), 'Kmax selection')
    need(type(record['smax']) is int and record['smax'] == min(kmax+1, n), 'rank window')
    integer(record['ball_index'], 0, (1 << 64)-1, 'ball index')
    arity = integer(record['minimal_arity'], 2, 4, 'minimal arity')
    raw_key = record['ball_key']
    need(type(raw_key) is dict and set(raw_key) == {'a', 'b', 'c'} and
         type(raw_key['b']) is list and len(raw_key['b']) == 3, 'ball key shape')
    key = tuple(decimal(value) for value in [raw_key['a'], *raw_key['b'], raw_key['c']])
    a, *_, c = key
    b = key[1:4]
    need(a > 0 and math.gcd(*key) == 1, 'primitive positive ball key')
    centre = tuple(Q(-value, 2*a) for value in b)
    radius = Q(dot(b, b)-4*a*c, 4*a*a)
    need(radius > 0, 'positive radius')
    level = record['squared_radius']
    need(type(level) is dict and set(level) == {'numerator_u64_le', 'denominator'} and
         type(level['numerator_u64_le']) is list and len(level['numerator_u64_le']) == 3, 'radius wire')
    numerator = sum(integer(limb, 0, (1 << 64)-1, 'U192 limb') << (64*i) for i, limb in enumerate(level['numerator_u64_le']))
    denominator = decimal(level['denominator'])
    need(denominator > 0 and Q(numerator, denominator) == radius, 'exact squared radius')
    def power(point):
        return a*dot(point, point) + dot(b, point) + c
    seen, observed = set(), {}
    for group in ('interior', 'shell'):
        need(type(record[group]) is list, 'point table')
        observed[group] = []
        for item in record[group]:
            need(type(item) is dict and set(item) == {'geometry_index', 'point_id', 'xyz'}, 'point fields')
            point_id = integer(item['point_id'], 0, n-1, 'point identity')
            index = integer(item['geometry_index'], 0, n-1, 'geometry index')
            need(point_id not in seen and cloud.geometry_index[point_id] == index, 'unique IDs and Morton binding')
            seen.add(point_id)
            need(type(item['xyz']) is list and len(item['xyz']) == 3 and all(type(x) is int for x in item['xyz']) and
                 tuple(item['xyz']) == cloud.points[point_id], 'all-ID coordinate binding')
            sign = power(cloud.points[point_id])
            need(sign < 0 if group == 'interior' else sign == 0, 'interior/shell power sign')
            observed[group].append(point_id)
    exact_interior, exact_shell = [], []
    for point_id, point in enumerate(cloud.points):
        value = power(point)
        if value < 0:
            exact_interior.append(point_id)
        elif value == 0:
            exact_shell.append(point_id)
    need(sorted(observed['interior']) == exact_interior and sorted(observed['shell']) == exact_shell,
         'complete I/U census over all input points')
    need(arity < len(exact_shell) <= 12, 'extra shell table domain 1..12')
    need(len(exact_interior)+arity <= record['smax'], 'rank-relevant key')
    supports, support_masks = [], []
    for q in range(2, min(4, len(exact_shell))+1):
        for positions in combinations(range(len(exact_shell)), q):
            ids = [exact_shell[i] for i in positions]
            answer = support_ball([cloud.points[i] for i in ids])
            if answer is None:
                continue
            support_centre, support_radius, weights = answer
            if support_centre != centre or support_radius != radius:
                continue
            need(primitive_key(support_centre, support_radius) == key, 'Gram support key identity')
            mask = sum(1 << i for i in positions)
            support_masks.append(mask)
            supports.append(dict(point_ids=ids, shell_mask=mask, cardinal=q,
                                 geometry_indices=[cloud.geometry_index[i] for i in ids],
                                 positive_weights=[rational(weight) for weight in weights]))
    need(supports and min(row['cardinal'] for row in supports) == arity, 'q_min equals declared minimal arity')
    contains = [False] * (1 << len(exact_shell))
    for mask in support_masks:
        contains[mask] = True
    for bit in range(len(exact_shell)):
        for mask in range(len(contains)):
            if mask & (1 << bit) and contains[mask ^ (1 << bit)]:
                contains[mask] = True
    h, quotients = local_table(exact_interior, exact_shell, contains, kmax, arity)
    return dict(ball_index=record['ball_index'], n=n, s=record['s'], kmax_requested=kmax, input_digest=cloud.digest,
        primitive_ball_key=record['ball_key'], centre=[rational(value) for value in centre], squared_radius=rational(radius),
        interior_ids=exact_interior, shell_mask_point_ids=exact_shell, all_minimal_positive_supports=supports,
        q_min=arity, maximal_strict_shell_cardinality=h, centre_in_convex_hull_by_mask=contains,
        local_quotients=quotients, all_input_points_scanned=n, global_parents_reconstructed=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('diagnostics', type=Path)
    parser.add_argument('--expected-n', type=int, default=50000)
    parser.add_argument('--mixed-stderr', action='store_true', help='explicitly ignore non-JSON stderr lines')
    args = parser.parse_args()
    integer(args.expected_n, 2, (1 << 30)-1, 'expected n domain')
    raw = args.diagnostics.read_bytes()
    records = []
    for line in raw.decode().splitlines():
        if not line.strip():
            continue
        if args.mixed_stderr and not line.lstrip().startswith('{'):
            continue
        records.append(loads(line))
    need(records and all(type(record) is dict and record.get('n') == args.expected_n for record in records), 'expected input size')
    cloud = Cloud(uniform(args.expected_n))
    results = [judge(record, cloud) for record in records]
    need(len({row['ball_index'] for row in results}) == len(results), 'duplicate ball index in diagnostic stream')
    print(json.dumps(dict(status='locally_verified', schema='mhgp7-extra-shell-rational-review-v1',
        diagnostic_only=True, public_status='not_claimed', input_recipe='MT19937_seed3_high16_coord65536_deduplicated_ids0',
        source_jsonl_sha256=hashlib.sha256(raw).hexdigest(), records=results,
        scope='all-input I/U census and rational local shell quotients only; no complete ball catalogue or global parents'),
        sort_keys=True, indent=2))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, TypeError) as error:
        print(type(error).__name__ + ': ' + str(error), file=sys.stderr)
        raise SystemExit(1)
