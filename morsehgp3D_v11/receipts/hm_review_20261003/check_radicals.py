#!/usr/bin/env python3
"""Bounded independent proof check for the frozen WIP RValue comparator.

No developer module or native executable is imported. Equality uses rational
squareclasses without factorisation. A nonzero sum is compared by certified
isqrt intervals, with explicit refusal if its declared bit budget expires.
The close nonzero witness is OUTSIDE the native export's 192-bit domain.
"""
import argparse
import ast
from fractions import Fraction
import hashlib
import itertools
import json
import math
from pathlib import Path
import random

HERE = Path(__file__).resolve().parent
SOURCE_SHA256 = '30447f752f00d2ec404f99b8513019799730958fbb2696f96899acfe4f694604'
AFTER_SHA256 = 'f023f6d05d613043ee578348d2ffa743cc1b14e911afb40338d8968f75e2c5fc'
LEVELS_SHA256 = '0da8fce4a4a4140f4e6be39c5bb5cb4ce1461e408019a76d81aafbbf5af58f70'


class ComparisonRefused(ValueError):
    pass


def require(value, reason):
    if not value:
        raise ValueError(reason)


def rational_sqrt(value):
    require(value >= 0, 'negative radicand')
    a, b = math.isqrt(value.numerator), math.isqrt(value.denominator)
    return Fraction(a, b) if a * a == value.numerator and b * b == value.denominator else None


def squareclasses(terms):
    """Exact signed coefficients of distinct rational squareclasses.

    x/r is a rational square iff its reduced numerator AND denominator are
    integer squares. Then sqrt(x)=rational_sqrt(x/r)*sqrt(r). Distinct
    squareclasses give distinct characters of a multiquadratic field, hence
    Q-linearly independent roots. The sum is zero iff every coefficient is.
    """
    groups = []
    for coefficient, value in terms:
        value = Fraction(value)
        require(value >= 0, 'negative radicand')
        if not value or not coefficient:
            continue
        for group in groups:
            ratio = rational_sqrt(value / group[0])
            if ratio is not None:
                group[1] += Fraction(coefficient) * ratio
                break
        else:
            groups.append([value, Fraction(coefficient)])
    return [(r, c) for r, c in groups if c]


def bounds(value, bits):
    n, d = value.numerator, value.denominator
    scaled = (n * d) << (2 * bits)
    a = math.isqrt(scaled)
    denominator = d << bits
    lo = Fraction(a, denominator)
    hi = lo if a * a == scaled else Fraction(a + 1, denominator)
    return lo, hi


def compare(terms, max_bits=4096):
    require(isinstance(max_bits, int) and max_bits >= 1, 'invalid bit budget')
    groups = squareclasses(terms)
    if not groups:
        return 0, {'certificate': 'squareclasses', 'bits': 0, 'groups': 0}
    bits = min(96, max_bits)
    while True:
        lo = hi = Fraction(0)
        for radicand, coefficient in groups:
            lower, upper = bounds(radicand, bits)
            lo += coefficient * (lower if coefficient > 0 else upper)
            hi += coefficient * (upper if coefficient > 0 else lower)
        if lo > 0:
            return 1, {'certificate': 'positive_interval', 'bits': bits, 'groups': len(groups)}
        if hi < 0:
            return -1, {'certificate': 'negative_interval', 'bits': bits, 'groups': len(groups)}
        if bits == max_bits:
            raise ComparisonRefused('nonzero radical sum not separated within %d bits' % max_bits)
        bits = min(2 * bits, max_bits)


def difference(left, right):
    t, m, q = left
    u, v, w = right
    return [(1, t), (1, m), (-1, q), (-1, u), (-1, v), (1, w)]


def load_frozen():
    source_path = HERE / 'source/morsehgp3D_v11/bench/points_radius.py'
    source = source_path.read_bytes()
    require(hashlib.sha256(source).hexdigest() == SOURCE_SHA256, 'source snapshot hash')
    tree = ast.parse(source.decode())
    names = {'sign', 'sqrt_diff_cmp', 'sqrt_cmp2', 'sqrt_bounds', 'RValue'}
    selected = [n for n in tree.body if isinstance(n, (ast.FunctionDef, ast.ClassDef)) and n.name in names]
    require({n.name for n in selected} == names, 'source definitions')
    namespace = {'Fraction': Fraction, 'math': math, 'ZERO': Fraction(0)}
    exec(compile(ast.Module(body=selected, type_ignores=[]), '<frozen-points-radius>', 'exec'), namespace)
    return namespace


def filter_witness():
    """Actual hash-verified AST helpers, without importing developer modules.

    This is a permitted scalar RValue/Levels input. No claim is made that a
    native Cloud can generate all four levels, or that u21 is wrong.
    """
    import numpy as np
    radius_path = HERE / 'source_radius_after.py'
    levels_path = HERE / 'source/morsehgp3D_v11/bench/points_hierarchy.py'
    namespace = {'Fraction': Fraction, 'math': math, 'ZERO': Fraction(0), 'np': np, 'need': require}
    for path, expected, names in ((radius_path, AFTER_SHA256,
                                  {'sign', 'sqrt_diff_cmp', 'sqrt_cmp2', 'sqrt_bounds', 'RValue'}),
                                 (levels_path, LEVELS_SHA256, {'Levels'})):
        data = path.read_bytes()
        require(hashlib.sha256(data).hexdigest() == expected, 'AFTER/Levels source hash')
        tree = ast.parse(data.decode())
        selected = [n for n in tree.body if isinstance(n, (ast.FunctionDef, ast.ClassDef)) and n.name in names]
        require({n.name for n in selected} == names, 'AFTER/Levels definitions')
        exec(compile(ast.Module(body=selected, type_ignores=[]), '<hash-verified-WIP-helper>', 'exec'), namespace)
    span = 14000000
    t, m, q = Fraction(1, 4), Fraction(span * span), (Fraction(span) - Fraction(6, 997)) ** 2
    exact = Fraction(1, 2) + Fraction(6, 997)
    target = exact + Fraction(1, 1 << 70)
    a = target * target
    require(t <= q < m, 'delayed radius date inequalities')
    require(max(max(x.numerator.bit_length(), x.denominator.bit_length()) for x in (t, m, q, a)) <= 192,
            'scalar exported rational widths')
    require(m < Fraction(3 * ((1 << 24) - 1) ** 2, 4), 'within u24 global MEB magnitude bound')
    levels = namespace['Levels'].from_fractions([a])
    value = namespace['RValue'](t, m, q)
    actual, certified = value.cmp_level(levels, 0), value.cmp_sqrt(a)
    require(actual == 1 and certified == -1, 'actual WIP wrong scalar filter sign')
    other = math.sqrt(levels.approx[0])
    gap = value.approx() - other
    published_slack = 1e-9 * (abs(value.approx()) + other) + 1e-300
    absolute_scale_slack = 1e-9 * (math.sqrt(t) + math.sqrt(m) + math.sqrt(q) + other) + 1e-300
    require(abs(gap) > published_slack and abs(gap) < absolute_scale_slack, 'sum-of-magnitudes needs exact fallback')
    return {'radius_after_sha256': AFTER_SHA256, 'levels_sha256': LEVELS_SHA256,
            't': str(t), 'm': str(m), 'q': str(q), 'other_squared': str(a),
            'exact_entry': str(exact), 'target_radius': 'exact_entry + 2^-70',
            'actual_cmp_level': actual, 'actual_cmp_sqrt': certified,
            'relative_error_claim_broken': True,
            'max_numerator_bits': max(x.numerator.bit_length() for x in (t, m, q, a)),
            'max_denominator_bits': max(x.denominator.bit_length() for x in (t, m, q, a)),
            'within_192bit_scalar_contract': True, 'u24_meb_magnitude_bound': True,
            'native_cloud_fixture_proved': False, 'native_u21_wrong_owner_proved': False,
            'recommendation': 'absolute error bound from sqrt(t)+sqrt(m)+sqrt(q)+sqrt(other), else exact cmp_sqrt',
            'recommended_slack_causes_exact_fallback': True}, 7


def run_checks():
    frozen = load_frozen()
    RValue = frozen['RValue']
    checks = 1
    left = tuple(map(Fraction, (2, 162, 50)))
    right = tuple(map(Fraction, (8, 98, 32)))
    require(all(t <= q < m for t, m, q in (left, right)), 'generated date inequalities')
    require(set((left[0], left[1], right[2])).isdisjoint((right[0], right[1], left[2])), 'no identical terms')
    before = RValue.undecided
    old_equal = RValue(*left).cmp(RValue(*right))
    require(old_equal == 0 and RValue.undecided == before + 1, 'old equality exhausts its intervals')
    sign_equal, certificate = compare(difference(left, right))
    require(sign_equal == 0 and certificate['certificate'] == 'squareclasses', 'certified equality')
    checks += 4

    # Raising m strictly raises sqrt(t)+sqrt(m)-sqrt(q). The old unrestricted
    # constructor accepts this denominator; it cannot arise from the frozen
    # export's three-word level encoding.
    close = (right[0], right[1] + Fraction(1, 1 << 9000), right[2])
    before = RValue.undecided
    old_close = RValue(*left).cmp(RValue(*close))
    require(old_close == 0 and RValue.undecided == before + 1, 'old false equality')
    refused = False
    try:
        compare(difference(left, close), max_bits=4096)
    except ComparisonRefused:
        refused = True
    require(refused, 'bounded comparator must refuse unresolved nonzero')
    precise, precise_certificate = compare(difference(left, close), max_bits=12288)
    require(precise == -1, 'strictly greater right-hand m')
    checks += 3

    # Independent interval route versus the existing exact two-versus-two
    # formula, on 500 bounded random rational quadruples.
    rng = random.Random(20261003)
    for _ in range(500):
        values = [Fraction(rng.randrange(50), rng.randrange(1, 17)) for _j in range(4)]
        expected = frozen['sqrt_cmp2'](*values)
        actual, _cert = compare([(1, values[0]), (1, values[1]), (-1, values[2]), (-1, values[3])])
        require(actual == expected, 'two versus two formula')
        checks += 1

    # Exact one-class oracle: each root is a rational multiple of sqrt(base).
    for _ in range(300):
        base = Fraction(rng.randrange(1, 30), rng.randrange(1, 20))
        terms, coefficient = [], Fraction(0)
        for _j in range(6):
            scale = Fraction(rng.randrange(1, 12), rng.randrange(1, 12))
            signed = rng.choice((-1, 1))
            terms.append((signed, base * scale * scale))
            coefficient += signed * scale
        got, _cert = compare(terms)
        require(got == (coefficient > 0) - (coefficient < 0), 'one squareclass rational oracle')
        checks += 1

    # All 720 term orders: representatives may change; zero remains certified,
    # including classes 2,3,6 whose products are dependent but roots independent.
    terms = [(1, Fraction(8)), (-2, Fraction(2)), (1, Fraction(27)), (-3, Fraction(3)),
             (1, Fraction(24)), (-2, Fraction(6))]
    for perm in itertools.permutations(terms):
        got, cert = compare(perm)
        require(got == 0 and cert['certificate'] == 'squareclasses', 'permutation certificate')
        checks += 1

    require(bounds(Fraction(2), 4096)[1] - bounds(Fraction(2), 4096)[0] == Fraction(1, 1 << 4096),
            'terminal precision')
    checks += 1
    scalar_filter, filter_checks = filter_witness()
    checks += filter_checks
    return {'verdict': 'PASS', 'checks': checks, 'source_sha256': SOURCE_SHA256,
            'scope': 'pure Python comparator; no native build/execution, fit or GCP',
            'verification_history': {'initial_normal_exit': 1, 'initial_optimized_exit': 1,
                                     'initial_script_sha256': '52324041e03531bcc1f949dae001b4aa0c195e5b7582afaa70ea9ed4f45c2b43',
                                     'initial_error': 'ValueError: terminal precision',
                                     'cause': 'audit-only expected interval width used denominator 2 instead of 1',
                                     'correction': 'sqrt(2) has rational radicand denominator 1; width is 2^-bits',
                                     'developer_source_modified': False,
                                     'previous_closed_checks': 1529,
                                     'previous_script_sha256': 'd868e1aa70a79104afd819d92eef2f9c4b4c6b459228eb6132cef3a5be040040',
                                     'previous_normal_and_optimized_sha256':
                                     '8110d4ed6e4076c6240c60f3d61132dc71f52df229d9e2f08e7545b56d6ecdcc',
                                     'extension_authorized_by_root': 'AFTER scalar filter causal witness, no native fixture'},
            'scalar_filter_after': scalar_filter,
            'equality': {'left': [str(x) for x in left], 'right': [str(x) for x in right],
                         'value': '5*sqrt(2)', 't_le_q_lt_m': True, 'old_result': old_equal,
                         'old_fallthrough_is_not_an_equality_certificate': True, 'certificate': certificate},
            'strict_close': {'perturbation': 'right.m += 2^-9000', 'expected': -1,
                             'old_result': old_close, 'old_terminal_bits': 3072,
                             'new_budget_4096': 'explicit_refusal', 'larger_budget_result': precise,
                             'larger_budget_certificate': precise_certificate,
                             'outside_export_192bit_domain': True,
                             'native_u21_wrong_order_demonstrated': False},
            'commands': ['python3 check_radicals.py --out radicals_checks.json --overwrite',
                         'python3 -O check_radicals.py --out radicals_optimized.json --overwrite']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path)
    parser.add_argument('--overwrite', action='store_true', help='explicitly replace an audit output after extension')
    args = parser.parse_args()
    result = run_checks()
    if args.out:
        require(not args.out.exists() or args.overwrite, 'existing output protected')
        args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
