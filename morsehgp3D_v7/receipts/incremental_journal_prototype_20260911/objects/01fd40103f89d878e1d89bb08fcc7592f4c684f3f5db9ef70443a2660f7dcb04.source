#!/usr/bin/env python3
"""Pure rational models for the explicit FULL output lower-bound argument.

No product import, geometry engine, filesystem write or subprocess. The three
finite models check algebra and strict empty diameter balls, not a FULL build,
generic perturbation, an infinite u16 family, or a performance claim.
Usage: python3 [-O] full_output_lower_bound_gate.py --selftest
Codes: 0 passed; 1 failed model/mutant; 2 invalid invocation.
"""
from __future__ import annotations

from fractions import Fraction
import json
import sys
from typing import Callable

Q = Fraction
Point = tuple[Fraction, Fraction, Fraction]


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def arc_a(t: Fraction) -> Point:
    return ((1 - t * t) / (1 + t * t), 2 * t / (1 + t * t), Q(0))


def arc_b(t: Fraction) -> Point:
    return (2 * t * t / (1 + t * t), Q(0), 2 * t / (1 + t * t))


def diameter_power(a: Point, b: Point, z: Point) -> Fraction:
    return sum(((z[k] - a[k]) * (z[k] - b[k]) for k in range(3)), Q(0))


def identity(t: Fraction, u: Fraction, v: Fraction, mutant: str) -> Fraction:
    correction = u * u * (3 * v + t)
    if mutant == 'omit_correction':
        correction = Q(0)
    numerator = (v - t) * ((v - t) + correction)
    if mutant != 'lose_factor_two':
        numerator *= 2
    denominator = (1 + t * t) * (1 + v * v) * (1 + u * u)
    if mutant == 'omit_denominator_factor':
        denominator /= 1 + u * u
    return numerator / denominator


def rational(value: Fraction) -> dict[str, int]:
    return {'numerator': value.numerator, 'denominator': value.denominator}


def family(m: int, mutant: str = 'nominal') -> dict:
    require(type(m) is int and m in (2, 5, 10), 'bounded_model')
    delta, largest = Q(1, 10 * m * m), Q(1, 10 * m)
    margin = delta - (3 if mutant == 'weaken_margin_coefficient' else 4) * largest**3
    if mutant == 'negative_margin':
        margin = -margin
    require(margin > 0, 'margin_positive')
    require(margin == Q(25 * m - 1, 250 * m**3), 'margin_identity')
    lower_power = 2 * delta * margin / (1 + largest * largest)**3
    parameters = [i * delta for i in range(1, m + 1)]
    points = [arc_a(t) for t in parameters] + [arc_b(t) for t in parameters]
    require(len(set(points)) == 2 * m, 'point_identities')
    checks, pairs, smallest = 0, 0, None
    for t in parameters:
        for u in parameters:
            powers = []
            for v in parameters:
                if v != t:
                    value = diameter_power(arc_a(t), arc_b(u), arc_a(v))
                    require(value == identity(t, u, v, mutant), 'diameter_identity_A')
                    powers.append(value)
                if v != u:
                    value = diameter_power(arc_a(t), arc_b(u), arc_b(v))
                    require(value == identity(u, t, v, mutant), 'diameter_identity_B')
                    powers.append(value)
            require(len(powers) == 2 * m - 2, 'all_other_points_visited')
            require(all(value >= lower_power > 0 for value in powers), 'strict_power_bound')
            checks += len(powers)
            pairs += 1
            smallest = min(powers) if smallest is None else min(smallest, *powers)
    require(pairs == m * m and checks == 2 * m * m * (m - 1), 'family_nonvacuum')
    return {'m': m, 'points': 2 * m, 'strict_cross_pairs': pairs,
            'rational_identity_checks': checks, 'margin': rational(margin),
            'power_lower_bound': rational(lower_power), 'minimum_power': rational(smallest)}


def ball_models(predicate: Callable[[list[Fraction]], bool]) -> int:
    a, b = (Q(-1), Q(0), Q(0)), (Q(1), Q(0), Q(0))
    inside, shell, outside = ((Q(0), Q(0), Q(0)),
                              (Q(0), Q(1), Q(0)), (Q(0), Q(2), Q(0)))
    cases = [([outside], True), ([inside, outside], False),
             ([shell, outside], False), ([outside, inside], False),
             ([inside], False), ([], True)]
    for index, (points, expected) in enumerate(cases):
        powers = [diameter_power(a, b, point) for point in points]
        require(predicate(powers) is expected, f'all_exterior_case_{index}')
    return len(cases)


def selftest() -> dict:
    cases = [family(m) for m in (2, 5, 10)]
    negative_models = ball_models(lambda values: all(value > 0 for value in values))
    killed = []
    algebra = [
        ('omit_correction', 'diameter_identity_A'),
        ('lose_factor_two', 'diameter_identity_A'),
        ('omit_denominator_factor', 'diameter_identity_A'),
        ('weaken_margin_coefficient', 'margin_identity'),
        ('negative_margin', 'margin_positive'),
    ]
    for name, reason in algebra:
        try:
            family(2, name)
        except ValueError as error:
            require(str(error) == reason, 'wrong_mutant_reason:' + name + ':' + str(error))
            killed.append(name)
    predicates = [
        ('any_instead_of_all', lambda values: any(value > 0 for value in values),
         'all_exterior_case_1'),
        ('accept_foreign_shell', lambda values: all(value >= 0 for value in values),
         'all_exterior_case_2'),
        ('skip_first_foreign_point', lambda values: all(value > 0 for value in values[1:]),
         'all_exterior_case_1'),
    ]
    for name, predicate, reason in predicates:
        try:
            ball_models(predicate)
        except ValueError as error:
            require(str(error) == reason, 'wrong_mutant_reason:' + name + ':' + str(error))
            killed.append(name)
    require(sum(case['strict_cross_pairs'] for case in cases) == 129
            and sum(case['rational_identity_checks'] for case in cases) == 2008
            and negative_models == 6 and len(killed) == 8, 'selftest_nonvacuum')
    return {'schema': 'mhgp7-full-output-lower-bound-rational-models-v1',
            'status': 'passed', 'public_status': 'not_claimed',
            'phase': 'exploration_v7_hors_registre', 'backend': 'cpu_reference',
            'profile': 'quantized_u16_input_only',
            'mode': 'audit_independant_math_and_architecture',
            'fixture_coordinate_domain': 'exact_rational_not_u16',
            'engine_invoked': False, 'full_builder_qualified': False,
            'regularizing_perturbation_executed': False,
            'infinite_u16_family_claimed': False, 'performance_claimed': False,
            'cases': cases, 'all_exterior_models': negative_models,
            'mutants_killed': killed, 'python_optimization': sys.flags.optimize}


def main(argv: list[str]) -> int:
    if argv != ['--selftest']:
        print(json.dumps({'status': 'invalid_arguments', 'public_status': 'not_claimed'}))
        return 2
    print(json.dumps(selftest(), sort_keys=True))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except (ValueError, TypeError, ZeroDivisionError) as error:
        print(json.dumps({'status': 'failed', 'reason': str(error),
                          'public_status': 'not_claimed'}, sort_keys=True))
        sys.exit(1)
