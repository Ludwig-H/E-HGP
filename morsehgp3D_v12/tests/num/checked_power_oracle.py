"""Gram/Fraction judge for controlled q3 power and closed-box bounds; Python integers judge intermediate overflow."""
import hashlib
import itertools
import json
import random
import subprocess
import sys
from fractions import Fraction as F

from fraction_oracle import center_of, determinant, dot, require, sign, sub

MINIMUM, MAXIMUM = -(1 << 127), (1 << 127) - 1


def checked(d, n, norm, factors):
    terms = [d * norm] + [x * y for x, y in zip(n, factors)]
    total = terms[0]
    if not MINIMUM <= total <= MAXIMUM:
        return False, 'first'
    for term in terms[1:]:
        if not MINIMUM <= term <= MAXIMUM:
            return False, 'product'
        total += term
        if not MINIMUM <= total <= MAXIMUM:
            return False, 'addition'
    return True, 'fit'


def cases(bits):
    m = (1 << bits) - 1
    zero, top = (0, 0, 0), (m, m, m)
    rows = []
    def add(name, points, query, lo=zero, hi=top):
        rows.append((name, tuple(points), query, lo, hi))
    for scale in (4, min(m, 524287), m):
        points = (zero, (scale, scale, 0), (scale, 0, scale))
        for perm in itertools.permutations(points):
            for z in (zero, points[1], points[2], (0, scale, scale), top, (m//2,)*3):
                add('permuted', perm, z)
                add('singleton', perm, z, z, z)
    small = (zero, (4, 0, 0), (0, 4, 0))
    add('negative', small, (1, 1, 0), (1, 1, 0), (1, 1, 0))
    add('contact', small, zero, zero, zero)
    large = (zero, (m, m, 0), (m, 0, m))
    add('product_cancellation', large, large[1], large[1], large[1])
    l, t = min(m, 2097151), min(m, 1200000)
    add('lower_only', (zero, (l, l, 0), (l, 0, l)), zero, zero, (0, t, 0))
    l, h = min(m, 1250000), min(m, 2097151)
    add('upper_only', (zero, (l, l, 0), (l, 0, l)), zero, zero, (h, h, h))
    local = min(m, 524287)
    add('local_support_global_query', (zero, (local, local, 0), (local, 0, local)), top)
    add('both_overflow', large, top)
    for name, raw, scale in (
            ('sum_cancellation', [(2,0,7), (1,6,2), (7,3,1), (0,7,1)], 269568),
            ('sum_final_large', [(3,2,7), (7,6,8), (3,6,3), (1,2,5)], 476192)):
        scale = min(scale, m // max(max(p) for p in raw))
        pts = [tuple(x * scale for x in p) for p in raw]
        add(name, pts[:3], pts[3])
    rng = random.Random(320603)
    for width in (7, m):
        for _ in range(48):
            p = [tuple(rng.randrange(width+1) for _ in range(3)) for j in range(6)]
            lo, hi = tuple(map(min, zip(*p[4:]))), tuple(map(max, zip(*p[4:])))
            add('random', p[:3], p[3], lo, hi)
    add('degenerate', (zero, zero, top), zero)
    add('aligned', (zero, (1, 1, 1), top), top)
    for axis in range(3):
        bad = list(zero); bad[axis] = m+1
        add('domain', small, tuple(bad))
        low = list(zero); low[axis] = 1
        add('box_order', small, zero, tuple(low), zero)
    return rows


def geometry(case, bits):
    _, points, query, lo, hi = case
    if any(not 0 <= x < 1 << bits for p in (*points, query, lo, hi) for x in p):
        return 'refused coordinate_out_of_domain'
    if any(a > b for a, b in zip(lo, hi)):
        return 'refused parameter_out_of_range'
    center = center_of(points)
    if center is None:
        return 'degenerate'
    a = points[0]
    edges = [sub(p, a) for p in points[1:]]
    d = 2 * determinant([[dot(u, v) for v in edges] for u in edges])
    rational_n = [d * (c - x) for c, x in zip(center, a)]
    require(all(n.denominator == 1 for n in rational_n), 'integral native coefficients')
    n = list(map(int, rational_n))
    radius = dot(sub(center, a), sub(center, a))
    power = d * (dot(sub(query, center), sub(query, center)) - radius)
    require(power.denominator == 1, 'integral power')
    near = [min(max(x, low), high) for x, low, high in zip(a, lo, hi)]
    norm_lo = dot(sub(near, a), sub(near, a))
    norm_hi = sum(max((low-x)**2, (high-x)**2) for x, low, high in zip(a, lo, hi))
    factors = [(-2*(low-x), -2*(high-x)) for x, low, high in zip(a, lo, hi)]
    selected = [[min(pair, key=lambda v: c*v) for c, pair in zip(n, factors)],
                [max(pair, key=lambda v: c*v) for c, pair in zip(n, factors)]]
    lower = d*norm_lo + dot(n, selected[0])
    upper = d*norm_hi + dot(n, selected[1])
    delta = sub(query, a)
    attempts = [checked(d, n, dot(delta, delta), [-2*x for x in delta]),
                checked(d, n, norm_lo, selected[0]), checked(d, n, norm_hi, selected[1])]
    nearest_center = [min(max(c, low), high) for c, low, high in zip(center, lo, hi)]
    true_min = d * (dot(sub(nearest_center, center), sub(nearest_center, center)) - radius)
    true_max = d * (sum(max((F(low)-c)**2, (F(high)-c)**2) for c, low, high in zip(center, lo, hi)) - radius)
    require(lower <= true_min <= true_max <= upper, 'continuous bounds')
    return dict(n=n, d=d, power=int(power), side=sign(power), lower=lower, upper=upper,
                flags=[int(ok) for ok, _ in attempts], routes=[why for _, why in attempts])


def line_of(value):
    if isinstance(value, str):
        return value
    words = [format(x, 'x') for x in value['n'] + [value['d'], value['power']]]
    words += [str(value['side']), format(value['lower'], 'x'), format(value['upper'], 'x')]
    return 'ok ' + ' '.join(words + list(map(str, value['flags'])))


def judge(case, bits, line):
    expected = geometry(case, bits)
    if isinstance(expected, str):
        require(line == expected, 'refusal or degeneracy')
        return 1
    words = line.split()
    require(len(words) == 12 and words[0] == 'ok', 'response shape')
    got = [int(w, 16) for w in words[1:6]]
    want = expected['n'] + [expected['d'], expected['power']]
    require(got == want, 'coefficients or exact power')
    require(int(words[6]) == expected['side'], 'side')
    require([int(w, 16) for w in words[7:9]] == [expected['lower'], expected['upper']], 'bounds')
    require(words[9:] == list(map(str, expected['flags'])), 'checked routes')
    return 11


def facts(rows, bits):
    values = {r[0]: geometry(r, bits) for r in rows if r[0] != 'permuted'}
    require(values['negative']['power'] < 0 and values['contact']['power'] == 0, 'sign fixtures')
    require(values['contact']['lower'] == values['contact']['upper'] == 0, 'closed contact')
    v = values['product_cancellation']
    require(v['power'] == 0 and v['flags'][0] == 0, 'cancellation first product')
    require(values['lower_only']['flags'][1:] == [0, 1], 'lower-only overflow')
    require(values['upper_only']['flags'][1:] == [1, 0], 'upper-only overflow')
    require(values['both_overflow']['flags'][1:] == [0, 0], 'both bounds overflow')
    v = values['sum_cancellation']
    require(v['routes'][0] == 'addition' and MINIMUM <= v['power'] <= MAXIMUM,
            'products fit, partial sum overflow, final fits')
    if bits == 24:
        require(values['sum_final_large']['routes'][0] == 'addition' and
                values['sum_final_large']['power'] > MAXIMUM, 'addition overflow changes final integer')
        require(values['local_support_global_query']['power'] > MAXIMUM, 'global witness width')
    return 7 + 2*(bits == 24)


def payload(rows):
    return ''.join(' '.join(str(x) for p in (*r[1], r[2], r[3], r[4]) for x in p)+'\n' for r in rows)


def selftest():
    reports = []
    for bits in (21, 24):
        rows = cases(bits)
        checks = sum(judge(r, bits, line_of(geometry(r, bits))) for r in rows) + facts(rows, bits)
        sample = next(r for r in rows if r[0] == 'negative')
        good = line_of(geometry(sample, bits)).split()
        corruptions = 0
        for index in range(1, 12):
            wrong = good.copy(); wrong[index] = 'deadbeef' if index < 6 or index in (7, 8) else '9'
            try:
                judge(sample, bits, ' '.join(wrong))
            except ValueError:
                corruptions += 1
            else:
                raise ValueError('accepted corruption '+str(index))
        for wrong in ('', 'ok', 'degenerate', ' '.join(good+['0'])):
            try:
                judge(sample, bits, wrong)
            except ValueError:
                corruptions += 1
            else:
                raise ValueError('accepted malformed response')
        reports.append(dict(bits=bits, requests=len(rows), checks=checks, corruptions=corruptions, native=0))
    print(json.dumps(dict(verdict='conforme', profiles=reports), sort_keys=True))


def run(executable):
    header = subprocess.run([executable], input='', text=True, capture_output=True, timeout=5)
    require(header.returncode == 0 and not header.stderr, 'probe header process')
    words = header.stdout.split()
    require(len(words) == 2 and words[0] == 'bits' and int(words[1]) in (21, 24), 'header bits')
    bits = int(words[1]); rows = cases(bits); text = payload(rows)
    process = subprocess.run([executable], input=text, text=True, capture_output=True, timeout=45)
    require(process.returncode == 0 and not process.stderr, 'probe process')
    lines = process.stdout.splitlines()
    require(lines[0] == 'bits '+str(bits) and len(lines) == len(rows)+1, 'complete output')
    checks = sum(judge(r, bits, line) for r, line in zip(rows, lines[1:])) + facts(rows, bits)
    require(len(rows) >= 300 and checks >= 3000, 'non-vacuous oracle')
    print(json.dumps(dict(verdict='conforme', bits=bits, requests=len(rows), checks=checks,
                         input_sha256=hashlib.sha256(text.encode()).hexdigest()), sort_keys=True))


if __name__ == '__main__':
    try:
        if sys.argv[1:] == ['--selftest']:
            selftest()
        elif len(sys.argv) == 2:
            run(sys.argv[1])
        else:
            raise ValueError('usage: checked_power_oracle.py EXE|--selftest')
    except (ValueError, OSError, subprocess.SubprocessError, IndexError) as error:
        print('REFUS checked_power_oracle:', error, file=sys.stderr)
        sys.exit(1)
