"""Independent Gram/Fraction judge of power bounds on continuous closed boxes; no product formulas imported."""
import hashlib
import json
import random
import subprocess
import sys
from fractions import Fraction as F

from fraction_oracle import center_of, determinant, dot, orient, require, sub


def cases(bits):
    m = (1 << bits) - 1
    zero, maximum = (0, 0, 0), (m, m, m)
    fixtures = [
        [zero, (4, 0, 0), (0, 4, 0), (0, 0, 4)],
        [zero, (m, m, 0), (m, 0, m), (0, m, m)],
        [zero, (m, 1, 0), (m - 1, 1, 0), (m, 1, 1)],
        [maximum, (0, m, m), (m, 0, m), (m, m, 0)],
        [zero] * 4,
        [zero, (4, 0, 0), (0, 4, 0), (4, 4, 0)],
    ]
    result = []
    for points in fixtures:
        lo = tuple(min(a, b) for a, b in zip(points[0], points[1]))
        hi = tuple(max(a, b) for a, b in zip(points[0], points[1]))
        boxes = [(zero, maximum), (zero, zero), (points[0], points[0]), (maximum, maximum),
                 (lo, hi), ((m // 2,) * 3, (m // 2,) * 3), (zero, (7, 7, 7)), (zero, (m, 0, m))]
        result.extend((q, tuple(points), low, high) for q in range(1, 5) for low, high in boxes)
    rng = random.Random(11003)
    for q in range(1, 5):
        for scale in (7, m):
            for _ in range(12):
                points = tuple(tuple(rng.randrange(scale + 1) for _ in range(3)) for _ in range(4))
                ends = [tuple(rng.randrange(scale + 1) for _ in range(3)) for _ in range(2)]
                lo = tuple(map(min, zip(*ends)))
                hi = tuple(map(max, zip(*ends)))
                result.append((q, points, lo, hi))
                result.append((q, points, ends[0], ends[0]))
    pair = tuple(fixtures[0])
    result += [(2, pair, (4, 0, 0), (4, 0, 1)), (2, pair, (2, 0, 0), (2, 0, 0)),
               (2, pair, zero, (4, 0, 0)), (1, ((2, 2, 2),) * 4, zero, (4, 4, 4))]
    result += [(1, pair, (1, 1, 1), hi) for hi in ((0, 1, 1), (1, 0, 1), (1, 1, 0))]
    return result


def geometry(case):
    q, points, lo, hi = case
    if any(a > b for a, b in zip(lo, hi)):
        return 'refused parameter_out_of_range'
    center = center_of(points[:q])
    if center is None:
        return 'degenerate'
    if q == 1:
        denominator = 1
    elif q == 2:
        denominator = 2
    elif q == 3:
        edges = [sub(p, points[0]) for p in points[1:3]]
        denominator = 2 * determinant([[dot(a, b) for b in edges] for a in edges])
    else:
        denominator = 2 * abs(orient(*points))
    numerator = [denominator * (c - a) for c, a in zip(center, points[0])]
    require(all(n.denominator == 1 for n in numerator), 'fixture non integral coefficients')
    radius = dot(sub(center, points[0]), sub(center, points[0]))
    near = [min(max(c, a), b) for c, a, b in zip(center, lo, hi)]
    far2 = sum(max((F(a) - c)**2, (F(b) - c)**2) for c, a, b in zip(center, lo, hi))
    true_min = denominator * (dot(sub(near, center), sub(near, center)) - radius)
    true_max = denominator * (far2 - radius)
    # Independent extrema of the two summands, using rational interval projection rather than native branches.
    anchor = points[0]
    nearest_anchor = [min(max(a, low), high) for a, low, high in zip(anchor, lo, hi)]
    norm_min = dot(sub(nearest_anchor, anchor), sub(nearest_anchor, anchor))
    norm_max = sum(max((a - low)**2, (a - high)**2) for a, low, high in zip(anchor, lo, hi))
    linear = [(-2 * n * (low - a), -2 * n * (high - a)) for n, a, low, high in zip(numerator, anchor, lo, hi)]
    lower = denominator * norm_min + sum(min(pair) for pair in linear)
    upper = denominator * norm_max + sum(max(pair) for pair in linear)
    return dict(center=center, n=list(map(int, numerator)), d=denominator, lower=int(lower), upper=int(upper),
                true_min=true_min, true_max=true_max)


def model_line(case):
    value = geometry(case)
    if isinstance(value, str):
        return value
    return 'ok ' + ' '.join(format(n, 'x') for n in value['n'] + [value['d'], value['lower'], value['upper']])


def check_case(case, line):
    value = geometry(case)
    if isinstance(value, str):
        require(line == value, 'refusal/degeneracy mismatch')
        return 1, value
    words = line.split()
    require(len(words) == 7 and words[0] == 'ok', 'bounds response shape')
    numerator = list(map(lambda v: int(v, 16), words[1:4]))
    denominator, lower, upper = (int(v, 16) for v in words[4:7])
    require(denominator == value['d'] > 0, 'certified denominator changed')
    require(numerator == value['n'], 'center coefficients changed')
    require(lower <= upper, 'reversed power interval')
    require(lower == value['lower'] and upper == value['upper'], 'separable bound differs')
    require(lower <= value['true_min'], 'lower bound misses continuous minimum')
    require(upper >= value['true_max'], 'upper bound misses continuous maximum')
    require(lower <= 0 or value['true_min'] > 0, 'false outside certificate')
    require(upper >= 0 or value['true_max'] < 0, 'false strict-inside certificate')
    if case[2] == case[3]:
        require(lower == upper == value['true_min'] == value['true_max'], 'singleton not exact')
    return 9 + int(case[2] == case[3]), value


def judge(data, lines):
    require(len(data) == len(lines) == 391, 'bounds case inventory')
    stats = dict(checks=0, valid=0, degenerate=0, refused=0, loose=0, outside=0, inside=0, contacts=0, wide=0)
    for case, line in zip(data, lines):
        count, value = check_case(case, line)
        stats['checks'] += count
        if isinstance(value, str):
            stats['degenerate' if value == 'degenerate' else 'refused'] += 1
            continue
        stats['valid'] += 1
        stats['loose'] += value['lower'] < value['true_min'] or value['upper'] > value['true_max']
        stats['outside'] += value['lower'] > 0
        stats['inside'] += value['upper'] < 0
        stats['contacts'] += value['true_min'] == 0 or value['true_max'] == 0
        stats['wide'] += max(abs(value['lower']).bit_length(), abs(value['upper']).bit_length()) > 127
    require(stats['checks'] == 3400 and stats['refused'] == 3 and stats['valid'] == 354 and stats['degenerate'] == 34 and
            stats['loose'] >= 20 and stats['outside'] >= 5 and stats['inside'] >= 1 and stats['contacts'] >= 15,
            'bounds non-vacuity')
    return stats


def selftest():
    results = []
    for bits in (18, 21, 24):
        data = cases(bits)
        lines = [model_line(case) for case in data]
        stats = judge(data, lines)
        require((stats['wide'] > 0) == (bits > 18), 'wide q3 branch not exercised')
        index = next(i for i, case in enumerate(data) if case[2] == case[3] and lines[i].startswith('ok'))
        killed = 0
        for column in (1, 4, 5, 6):
            corrupted = lines[index].split()
            corrupted[column] = format(int(corrupted[column], 16) + 1, 'x')
            try:
                check_case(data[index], ' '.join(corrupted))
            except ValueError:
                killed += 1
            else:
                raise ValueError('corrupted bound accepted')
        require(killed == 4, 'bounds judge mutations')
        results.append(dict(bits=bits, cases=len(data), mutations=killed, **stats))
    print(json.dumps(dict(native_calls=0, profiles=results), sort_keys=True))


def run(probe):
    info = subprocess.run([probe], input='', capture_output=True, text=True, timeout=15)
    require(info.returncode == 0 and not info.stderr, 'bounds metadata failed')
    words = info.stdout.split()
    require(len(words) == 2 and words[0] == 'bits' and int(words[1]) in (18, 21, 24), 'bounds profile absent')
    bits = int(words[1])
    data = cases(bits)
    payload = '\n'.join(' '.join(map(str, [q] + [v for p in list(points) + [lo, hi] for v in p]))
                        for q, points, lo, hi in data) + '\n'
    result = subprocess.run([probe], input=payload, capture_output=True, text=True, timeout=45)
    require(result.returncode == 0 and not result.stderr, 'bounds native failure')
    lines = result.stdout.splitlines()
    require(lines and lines[0] == 'bits %d' % bits, 'bounds native profile changed')
    stats = judge(data, lines[1:])
    require((stats['wide'] > 0) == (bits > 18), 'wide bounds branch missing')
    print(json.dumps(dict(bits=bits, cases=len(data), input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
                         **stats), sort_keys=True))


if __name__ == '__main__':
    try:
        require(len(sys.argv) == 2, 'usage: bounds_oracle.py probe|--selftest')
        selftest() if sys.argv[1] == '--selftest' else run(sys.argv[1])
    except (ValueError, IndexError, OSError, subprocess.TimeoutExpired) as error:
        print('ECHEC bounds Fraction : ' + str(error), file=sys.stderr)
        sys.exit(1)
