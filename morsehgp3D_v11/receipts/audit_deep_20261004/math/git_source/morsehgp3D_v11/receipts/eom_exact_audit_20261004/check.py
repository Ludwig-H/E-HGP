#!/usr/bin/env python3
"""Bounded exact arithmetic aid for EOM. No fit or native product import."""
import ast
from fractions import Fraction as Q
from hashlib import sha256
from itertools import product
import json
from math import isqrt
from pathlib import Path
from types import SimpleNamespace

BASE = Path(__file__).resolve().parent
CHECKS = 0


def need(ok, message):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise RuntimeError(message)


def rational_sqrt(x):
    x = Q(x)
    if x < 0:
        raise ValueError('negative radicand')
    a, b = isqrt(x.numerator), isqrt(x.denominator)
    return Q(a, b) if a*a == x.numerator and b*b == x.denominator else None


class Rad:
    """Linear sum of roots modulo rational square classes, without factoring."""
    def __init__(self, terms=()):
        groups = {}
        for coefficient, radicand in terms:
            c, r = Q(coefficient), Q(radicand)
            if r < 0:
                raise ValueError('negative radicand')
            if not c or not r:
                continue
            sq = rational_sqrt(r)
            if sq is not None:
                c, r = c*sq, Q(1)
            for representative in groups:
                multiplier = rational_sqrt(r/representative)
                if multiplier is not None:
                    groups[representative] += c*multiplier
                    break
            else:
                groups[r] = c
        self.terms = {r: c for r, c in groups.items() if c}

    def __add__(self, other):
        if not isinstance(other, Rad):
            other = Rad(((other, 1),))
        return Rad([(c, r) for r, c in self.terms.items()] +
                   [(c, r) for r, c in other.terms.items()])

    def __neg__(self):
        return Rad((-c, r) for r, c in self.terms.items())

    def __sub__(self, other):
        return self + (-other if isinstance(other, Rad) else -Q(other))

    def __mul__(self, other):
        if not isinstance(other, Rad):
            return Rad((c*Q(other), r) for r, c in self.terms.items())
        return Rad((c*d, r*s) for (r, c), (s, d) in product(self.terms.items(), other.terms.items()))

    def __pow__(self, power):
        out = Rad(((1, 1),))
        for _ in range(power):
            out = out*self
        return out

    def equals(self, other):
        return not (self-other).terms


def date(t, M, q):
    return Rad(((1, t), (1, M), (-1, q)))


def inverse_date(t, M, q, mixed_coefficient=-2):
    t, M, q = Q(t), Q(M), Q(q)
    if min(t, M, q) < 0:
        raise ValueError('negative radicand')
    e = date(t, M, q)
    if not e.terms:
        raise ValueError('zero date has no finite reciprocal')
    d = t+M-q
    delta = d*d-4*t*M
    if delta:
        return Rad((((t-M-q)/delta, t), ((M-t-q)/delta, M),
                    (d/delta, q), (Q(mixed_coefficient)/delta, t*M*q)))
    # The tested domain has e>0. General callers must certify its sign first.
    smallest = min(t, M)
    if not smallest:
        raise ValueError('singular zero date')
    return Rad(((1/(2*smallest), smallest),))


def root_interval(r, bits):
    r = Q(r)
    scale = 1 << bits
    floor = isqrt((r.numerator*scale*scale)//r.denominator)
    lo = Q(floor, scale)
    return lo, lo if lo*lo == r else Q(floor+1, scale)


def rinterval(value, bits):
    lo = hi = Q(0)
    for r, c in value.terms.items():
        a, b = root_interval(r, bits)
        if c >= 0:
            lo, hi = lo+c*a, hi+c*b
        else:
            lo, hi = lo+c*b, hi+c*a
    return lo, hi


def private_tie_probe():
    snapshot = BASE/'modele_lib.py.snapshot'
    pin = json.loads((BASE/'SOURCE.json').read_text())
    need(sha256(snapshot.read_bytes()).hexdigest() == pin['snapshot_sha256'], 'frozen AST source pinned')
    names = {'_phi_interval', 'stability_interval', 'eom_select'}
    nodes = [n for n in ast.parse(snapshot.read_text()).body
             if isinstance(n, ast.FunctionDef) and n.name in names]
    need({n.name for n in nodes} == names, 'all three bounded functions extracted')
    class Refusal(Exception):
        pass
    namespace = {'ZERO': Q(0), 'Refusal': Refusal, 'rinterval': rinterval}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), '<frozen-eom-ast>', 'exec'), namespace)
    # Exact H k2/m3 cloud (x,x,0): A0,6,12 | B22,28,34 | D52,58,64.
    # A/B merge at8sqrt2, root12sqrt2, entries6sqrt2, mcs3.
    radii = [Rad(((r, 2),)) for r in (6, 8, 12)]
    clusters = [
        dict(id=0, parent=3, children=[], birth=0, death=1, join={i: 0 for i in range(3)}),
        dict(id=1, parent=3, children=[], birth=0, death=1, join={i: 0 for i in range(3, 6)}),
        dict(id=2, parent=4, children=[], birth=0, death=2, join={i: 0 for i in range(6, 9)}),
        dict(id=3, parent=4, children=[0, 1], birth=1, death=2, join={i: 1 for i in range(6)}),
        dict(id=4, parent=None, children=[3, 2], birth=2, death=None, join={i: 2 for i in range(9)})]
    selected, forced = namespace['eom_select'](SimpleNamespace(radii=radii), clusters, 1, budget=128)
    need(selected == [2, 3], 'parent on true tie returns expected antichain')
    need(forced == 1, 'interval-only selector flags a provable irrational equality as forced')
    return dict(selected=selected, forced=forced, budget_bits=128,
                source_sha256=pin['snapshot_sha256'], actual_label_error=False,
                scope='frozen private AST; no fit or product import')


def main():
    cases = [(Q(t), Q(M), Q(q)) for t in range(1, 9) for M in range(8) for q in range(M+1)]
    cases += [(Q(2, 3), Q(3, 5), Q(1, 7)), (Q(4), Q(9), Q(1)),
              (Q(1, 4), Q(9, 4), Q(1)), (Q(8), Q(18), Q(2))]
    singular = 0
    for t, M, q in cases:
        e, inverse = date(t, M, q), inverse_date(t, M, q)
        need((e*inverse).equals(1), 'exact date times rationalized reciprocal equals one')
        need((e**3*inverse**3).equals(1), 'exact cube reciprocal identity')
        need(len(inverse.terms) <= 4 and len((inverse**3).terms) <= 4, 'odd powers use at most four root classes')
        singular += int((t+M-q)**2 == 4*t*M)
    for triple in ((1, 1, 4), (4, 9, 25), (0, 4, 4), (0, 0, 0)):
        try:
            inverse_date(*triple)
        except ValueError:
            need(True, 'zero date explicitly refused')
        else:
            need(False, 'zero date unexpectedly inverted')
    a, b, c = (inverse_date(2*r*r, 0, 0) for r in (6, 8, 12))
    parent, children = (b-c)*6, (a-b)*6
    need(parent.equals(children), 'geometric EOM parent and children equality certified symbolically')
    need(parent.equals(Rad(((Q(1, 8), 2),))), 'common irrational score sqrt2/8')
    need((parent-children).equals(0), 'score difference exactly zero')
    nontrivial = date(2, 3, 1)
    mutant = inverse_date(2, 3, 1, mixed_coefficient=-1)
    need(not (nontrivial*mutant).equals(1), 'missing factor two mutant killed algebraically')
    ast_result = private_tie_probe()
    print(json.dumps(dict(checks=CHECKS, date_cases=len(cases), singular_positive_cases=singular,
                          exact_tie_score='sqrt(2)/8', causal_mutant_killed=True,
                          private_interval_probe=ast_result,
                          scope='bounded audit mathematics; no native, G4 or performance qualification'),
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
