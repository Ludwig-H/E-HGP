"""Autonomous rational derivation; imports no product oracle and executes no native program."""
from fractions import Fraction as F
from itertools import permutations, product
import json
import random


def require(condition, message):
    if not condition:
        raise ValueError(message)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def det(matrix):
    size = len(matrix)
    return sum((-1 if sum(p[i] > p[j] for i in range(size) for j in range(i + 1, size)) % 2 else 1)
               * product_of(matrix[i][p[i]] for i in range(size)) for p in permutations(range(size)))


def product_of(values):
    result = 1
    for value in values:
        result *= value
    return result


def solve(matrix, rhs):
    rows = [[F(x) for x in row] + [F(y)] for row, y in zip(matrix, rhs)]
    for column in range(len(rows)):
        pivot = next(i for i in range(column, len(rows)) if rows[i][column])
        rows[column], rows[pivot] = rows[pivot], rows[column]
        divisor = rows[column][column]
        rows[column] = [x / divisor for x in rows[column]]
        for i in range(len(rows)):
            if i != column:
                factor = rows[i][column]
                rows[i] = [x - factor * y for x, y in zip(rows[i], rows[column])]
    return tuple(row[-1] for row in rows)


def sphere(points):
    anchor = points[0]
    edges = [sub(p, anchor) for p in points[1:]]
    gram = [[dot(u, v) for v in edges] for u in edges]
    weights = solve(gram, [F(dot(v, v), 2) for v in edges]) if edges else ()
    center = tuple(F(anchor[j]) + sum(w * v[j] for w, v in zip(weights, edges)) for j in range(3))
    radius = dot(sub(center, anchor), sub(center, anchor))
    denominator = (1 if len(points) == 1 else 2 if len(points) == 2
                   else 2 * det(gram) if len(points) == 3 else 2 * abs(det(edges)))
    numerator = tuple(denominator * (c - a) for c, a in zip(center, anchor))
    require(denominator > 0 and all(x.denominator == 1 for x in numerator), 'coefficient domain')
    return dict(a=anchor, n=tuple(map(int, numerator)), d=denominator, c=center, r=radius, q=len(points))


def power(ball, point):
    delta = sub(point, ball['a'])
    return ball['d'] * dot(delta, delta) - 2 * dot(ball['n'], delta)


def oracle_power(ball, point):
    return ball['d'] * (dot(sub(point, ball['c']), sub(point, ball['c'])) - ball['r'])


def prepare(ball):
    chosen = []
    for a, n in zip(ball['a'], ball['n']):
        value, d = ball['d'] * a + n, ball['d']
        quotient = (1 if value >= 0 else -1) * (abs(value) // d)  # C++ truncation toward zero
        remainder = value - quotient * d
        twice = 2 * abs(remainder)
        if twice > d or (twice == d and remainder < 0):
            quotient += 1 if remainder > 0 else -1
        require(all(abs(x) < 2**127 for x in (value, twice, quotient)), 'prepared i128 overflow')
        chosen.append(quotient)
    return tuple(chosen)


def proposed(ball, box, prepared):
    lo, hi = box
    nearest = tuple(min(max(value, low), high) for value, low, high in zip(prepared, lo, hi))
    farthest = []
    for a, n, low, high in zip(ball['a'], ball['n'], lo, hi):
        left = 2 * (ball['d'] * a + n)
        right = ball['d'] * (low + high)
        require(abs(left) < 2**127 and abs(right) < 2**127, 'endpoint comparison overflow')
        farthest.append(high if left <= right else low)
    farthest = tuple(farthest)
    return power(ball, nearest), power(ball, farthest), nearest, farthest


def current(ball, box):
    lo, hi = box
    ranges = [(low - a, high - a) for a, low, high in zip(ball['a'], lo, hi)]
    norm_min = sum(0 if low <= 0 <= high else min(low * low, high * high) for low, high in ranges)
    norm_max = sum(max(low * low, high * high) for low, high in ranges)
    linear = [(-2 * n * low, -2 * n * high) for n, (low, high) in zip(ball['n'], ranges)]
    return (ball['d'] * norm_min + sum(min(v) for v in linear),
            ball['d'] * norm_max + sum(max(v) for v in linear))


def oracle_grid_min(ball, box):
    lo, hi = box
    near = []
    for center, low, high in zip(ball['c'], lo, hi):
        floor = center.numerator // center.denominator
        candidates = {min(max(floor, low), high), min(max(floor + 1, low), high)}
        near.append(min(candidates, key=lambda x: ((F(x) - center)**2, x)))
    return oracle_power(ball, tuple(near))


def check_box(ball, box, bits, prepared, stats, enumerate_points):
    low, high, nearest, farthest = proposed(ball, box, prepared)
    old_low, old_high = current(ball, box)
    lo, hi = box
    continuous_near = tuple(min(max(c, a), b) for c, a, b in zip(ball['c'], lo, hi))
    continuous_min = oracle_power(ball, continuous_near)
    corners = list(product(*[(a, b) for a, b in zip(lo, hi)]))
    corner_max = max(oracle_power(ball, p) for p in corners)
    require(old_low <= continuous_min <= low == oracle_grid_min(ball, box), 'lattice minimum mismatch')
    require(high == corner_max <= old_high and low <= high, 'corner maximum mismatch')
    require(power(ball, nearest) == oracle_power(ball, nearest), 'independent power mismatch')
    require(all(0 <= a <= v <= b < 2**bits for a, b, v in zip(lo, hi, nearest)), 'forged nearest point')
    bound = 72 * 2**(5 * bits) if ball['q'] == 4 else 216 * 2**(6 * bits)
    for point in (nearest, farthest):
        delta = sub(point, ball['a'])
        terms = [ball['d'] * dot(delta, delta)] + [-2 * n * x for n, x in zip(ball['n'], delta)]
        require(sum(map(abs, terms)) < bound, 'term/partial sum bound failed')
        stats['max_power_bits'] = max(stats['max_power_bits'], abs(power(ball, point)).bit_length())
        stats['wide_q3'] += ball['q'] == 3 and abs(power(ball, point)).bit_length() > 127
    if enumerate_points:
        points = product(*(range(a, b + 1) for a, b in zip(lo, hi)))
        values = [oracle_power(ball, p) for p in points]
        require(min(values) == low and max(values) == high, 'enumerated extrema mismatch')
        stats['integer_points'] += len(values)
    stats['boxes'] += 1
    stats['current_outside'] += old_low > 0
    stats['lattice_outside'] += low > 0
    stats['current_inside'] += old_high < 0
    stats['corner_inside'] += high < 0
    stats['boundary_min'] += low == 0
    stats['boundary_max'] += high == 0
    stats['noncontinuous_lower'] += low > continuous_min
    stats['strict_noncontinuous'] += low > 0 and continuous_min < 0


def run():
    small = [((1, 1, 1),), ((0, 0, 0), (2, 2, 2)), ((0, 0, 0), (1, 0, 0)),
             ((0, 0, 0), (2, 0, 0), (1, 2, 0)), ((0, 0, 0), (4, 0, 0), (1, 1, 0)),
             ((0, 0, 0), (2, 2, 0), (2, 0, 2), (0, 2, 2)),
             ((0, 0, 0), (4, 0, 0), (0, 4, 0), (0, 0, 4))]
    intervals = [(a, b) for a in range(3) for b in range(a, 3)]
    small_boxes = [(tuple(x[0] for x in axes), tuple(x[1] for x in axes)) for axes in product(intervals, repeat=3)]
    results = []
    for bits in (18, 21, 24):
        stats = dict(bits=bits, boxes=0, integer_points=0, current_outside=0, lattice_outside=0,
                     current_inside=0, corner_inside=0, boundary_min=0, boundary_max=0,
                     noncontinuous_lower=0, strict_noncontinuous=0, max_power_bits=0, wide_q3=0)
        m = 2**bits - 1
        large = [((0, 0, 0), (m, m, 0), (m, 0, m)),
                 ((0, 0, 0), (m, 1, 0), (m - 1, 1, 0)),
                 ((0, 0, 0), (m, m, 0), (m, 0, m), (0, m, m)),
                 ((0, 0, 0), (m, 1, 0), (m - 1, 1, 0), (m, 1, 1))]
        for points in small:
            ball = sphere(points)
            prepared = prepare(ball)
            for box in small_boxes:
                check_box(ball, box, bits, prepared, stats, True)
        rng = random.Random(111003)
        for points in large:
            ball = sphere(points)
            prepared = prepare(ball)
            boxes = [((0, 0, 0), (m, m, m)), ((m, m, m), (m, m, m)),
                     ((0, 0, 0), (0, 0, 0)), ((m // 2,) * 3, (m // 2,) * 3)]
            for _ in range(32):
                ends = [tuple(rng.randrange(m + 1) for _ in range(3)) for _ in range(2)]
                boxes.append((tuple(min(a, b) for a, b in zip(*ends)), tuple(max(a, b) for a, b in zip(*ends))))
            for box in boxes:
                check_box(ball, box, bits, prepared, stats, False)
        require(stats['lattice_outside'] >= stats['current_outside'] and
                stats['corner_inside'] >= stats['current_inside'], 'weaker certificates')
        require(stats['noncontinuous_lower'] > 0 and stats['boundary_min'] > 0, 'empty guards')
        require((stats['wide_q3'] > 0) == (bits > 18), 'q3 wide branch absent')
        results.append(stats)
    example = sphere(((0, 0, 0), (1, 0, 0)))
    box = ((0, 0, 0), (1, 0, 0))
    low, high, _, _ = proposed(example, box, prepare(example))
    require(low == high == 0 and oracle_power(example, (F(1, 2), 0, 0)) == F(-1, 2),
            'continuous contract counterexample missing')
    print(json.dumps({'native_calls': 0, 'profiles': results,
                      'continuous_contract_counterexample': {'lattice_lower': 0, 'continuous_min': '-1/2'},
                      'claim': 'exact lattice lower and continuous corner upper; no native implementation or speed claim'},
                     sort_keys=True))


if __name__ == '__main__':
    run()
