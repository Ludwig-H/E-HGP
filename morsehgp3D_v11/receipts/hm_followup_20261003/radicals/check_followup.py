#!/usr/bin/env python3
"""Frozen WIP pure-Python checks: radical equality/refusal and cancelled filter.

Only named AST definitions from hash-verified snapshots are executed. No
developer module, native executable, fit or cloud controller is imported.
The nonzero 2^-9000 witness is outside192bit; the scalar cancelled-filter
witness is within192bit/u24 magnitude but is not a generated Cloud fixture.

Equality theorem: group positive rational radicands by a/b being a rational
square (numerator and denominator both exact isqrt squares). Their square
roots lie in distinct character spaces of the multiquadratic extension and
are Q-linearly independent, even if classes such as2,3,6 are multiplicatively
dependent. Thus zero means all grouped rational coefficients vanish. The
nonzero sign can be separated by certified intervals, or refused explicitly.
"""
import argparse
import ast
import decimal
from fractions import Fraction as F
import hashlib
import itertools
import json
import math
from pathlib import Path
import random

import numpy as np

HERE = Path(__file__).resolve().parent
PINS = {'radius_before.py': '30447f752f00d2ec404f99b8513019799730958fbb2696f96899acfe4f694604',
        'radius_after.py': '58952a8ba37bef25e2e296f0c2b5e14f4f9d2c7eabcc8093d2a2f67810e24dc0',
        'hierarchy.py': '0da8fce4a4a4140f4e6be39c5bb5cb4ce1461e408019a76d81aafbbf5af58f70'}


def need(value, reason):
    if not value:
        raise ValueError(reason)


def extract(filename, names):
    b = (HERE / filename).read_bytes()
    need(hashlib.sha256(b).hexdigest() == PINS[filename], 'frozen source SHA')
    tree = ast.parse(b.decode())
    nodes = [n for n in tree.body if isinstance(n, (ast.FunctionDef, ast.ClassDef)) and n.name in names]
    need({n.name for n in nodes} == names, 'exact named AST definitions')
    ns = {'Fraction': F, 'math': math, 'ZERO': F(0), 'np': np, 'need': need}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), '<verified-helper-snapshot>', 'exec'), ns)
    return ns


def dsign(terms, precision=120):
    # All arithmetic including additions/subtractions belongs to this context.
    with decimal.localcontext() as ctx:
        ctx.prec = precision
        total = decimal.Decimal(0)
        for coefficient, value in terms:
            value = F(value)
            root = (decimal.Decimal(value.numerator) / decimal.Decimal(value.denominator)).sqrt()
            total += decimal.Decimal(coefficient) * root
        return (total > 0) - (total < 0)


def difference(a, b):
    return [(1, a[0]), (1, a[1]), (-1, a[2]), (-1, b[0]), (-1, b[1]), (1, b[2])]


def run():
    basic = {'sign', 'sqrt_diff_cmp', 'sqrt_cmp2', 'sqrt_bounds', 'RValue'}
    before = extract('radius_before.py', basic)
    after = extract('radius_after.py', basic | {'Refusal', 'square_ratio', 'radical_classes', 'sign_of_radicals'})
    levels = extract('hierarchy.py', {'Levels'})['Levels']
    old, new, cmp = before['RValue'], after['RValue'], after['sign_of_radicals']
    checks = 3

    a, b = tuple(map(F, (2, 162, 50))), tuple(map(F, (8, 98, 32)))
    need(all(t <= q < m for t, m, q in (a, b)), 'delayed date domain')
    start = old.undecided
    need(old(*a).cmp(old(*b)) == 0 and old.undecided == start + 1, 'old unknown equality')
    need(new(*a).cmp(new(*b)) == 0 and not after['radical_classes'](difference(a, b)), 'new certified equality')
    checks += 3

    eta = F(1, 1 << 9000)
    b1 = (b[0], b[1] + eta, b[2])
    need(old(*a).cmp(old(*b1)) == 0, 'old one-perturbation false0')
    need(cmp(difference(a, b1), budget=96) == -1, 'two-class exact shortcut fixes old witness')
    a2 = (a[0], a[1] + eta, a[2])
    terms = difference(a2, b1)
    need(len(after['radical_classes'](terms)) == 3, 'new refusal witness needs three classes')
    need(old(*a2).cmp(old(*b1)) == 0, 'old two-perturbation false0')
    refused = False
    try:
        cmp(terms, budget=96)
    except after['Refusal']:
        refused = True
    need(refused, 'explicit resource refusal, never unknown equality')
    # Independent analytic sign: sqrt(162+x)-sqrt(98+x) is strictly decreasing
    # for x>0 and equals2sqrt2 at0. Decimal3200 resolves the tiny difference.
    need(dsign(terms, precision=3200) == -1, 'high-precision independent sign')
    checks += 6

    span = 14000000
    t, m, q = F(1, 4), F(span * span), (F(span) - F(6, 997)) ** 2
    exact = F(1, 2) + F(6, 997)
    target2 = (exact + F(1, 1 << 70)) ** 2
    table = levels.from_fractions([target2])
    val = new(t, m, q)
    need(val.cmp_level(table, 0) == val.cmp_sqrt(target2) == -1, 'cancelled filter corrected')
    other = math.sqrt(table.approx[0]); gap = val.approx() - other
    need(abs(gap) < 1e-12 * (math.sqrt(t) + math.sqrt(m) + math.sqrt(q) + other), 'new filter exact fallback')
    need(max(max(f.numerator.bit_length(), f.denominator.bit_length()) for f in (t, m, q, target2)) <= 192,
         'scalar192bit domain')
    need(dsign([(1, t), (1, m), (-1, q), (-1, target2)]) == -1, 'Decimal120 filter witness')
    checks += 4

    # Zero certificate invariance, including multiplicatively dependent2,3,6.
    zeros = [(1, F(8)), (-2, F(2)), (1, F(27)), (-3, F(3)), (1, F(24)), (-2, F(6))]
    for perm in itertools.permutations(zeros):
        need(cmp(perm) == 0 and not after['radical_classes'](perm), 'all720 zero term orders')
        checks += 1
    # Exact single-class rational oracle and unrelated Decimal120 route.
    rng = random.Random(20261003)
    for _ in range(300):
        base = F(rng.randrange(1, 30), rng.randrange(1, 20))
        terms, coefficient = [], F(0)
        for _j in range(6):
            scale = F(rng.randrange(1, 12), rng.randrange(1, 12)); signed = rng.choice((-1, 1))
            terms.append((signed, base * scale * scale)); coefficient += signed * scale
        need(cmp(terms) == (coefficient > 0) - (coefficient < 0), 'one-class rational exact oracle')
        checks += 1
    decimal_compared = decimal_zero = 0
    for _ in range(500):
        terms = [(rng.choice((-1, 1)), F(rng.randrange(1, 60), rng.randrange(1, 30))) for _j in range(6)]
        got = cmp(terms)
        if got == 0:
            need(not after['radical_classes'](terms), 'zero has exact certificate')
            decimal_zero += 1
        else:
            need(got == dsign(terms), 'nonzero versus independent Decimal120')
            decimal_compared += 1
        checks += 1
    return {'verdict': 'PASS', 'checks': checks, 'source_sha256': PINS,
            'equality': {'a': ['2', '162', '50'], 'b': ['8', '98', '32'], 'value': '5sqrt2',
                         'old_zero_uncertified': True, 'new_squareclass_certificate': True},
            'close_nonzero': {'eta': '2^-9000', 'one_perturbation_new_exact': -1,
                             'two_perturbations': 'both m += eta', 'old_wrong_zero': True,
                             'new_budget96': 'Refusal', 'Decimal3200_sign': -1,
                             'analytic_sign': 'sqrt(162+x)-sqrt(98+x) strictly decreases for x>0',
                             'outside_native192bit_export': True},
            'cancelled_filter': {'t': str(t), 'm': str(m), 'q': str(q), 'target2': str(target2),
                                 'new_filtered_sign': -1, 'exact_sign': -1, 'Decimal120_sign': -1,
                                 'max_rational_bits': 160, 'within_u24_magnitude': True,
                                 'native_cloud_fixture_proved': False, 'native_u21_wrong_owner_proved': False},
            'Decimal120_random': {'nonzero_compared': decimal_compared, 'exact_zero_certified': decimal_zero},
            'scope': 'frozen WIP helpers; Python only, no native/build/fit/GCP; not a native qualification'}


def main():
    p = argparse.ArgumentParser(description=__doc__); p.add_argument('--out', type=Path)
    args = p.parse_args(); result = run()
    if args.out:
        need(not args.out.exists(), 'existing output protected')
        args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
