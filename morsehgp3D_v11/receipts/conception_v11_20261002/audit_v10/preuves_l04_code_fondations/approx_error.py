#!/usr/bin/env python3
"""L04 audit : erreur relative de geom::Level::approx() (geometry.cpp:8-20), modele exact en binary64.
to_double(I192) : d = ((double(w2) * 2^64 + double(w1)) * 2^64) + double(w0) ; to_double(I128w) : double(w1) * 2^64 + double(w0).
Commentaire du code (generator.cpp:779) : « erreur relative de approx < 2^-50 »."""
import random
from fractions import Fraction

rng = random.Random(7)
MASK = (1 << 64) - 1
T64 = 18446744073709551616.0


def to_double_192(v):
    d = 0.0
    for i in (2, 1, 0):
        d = d * T64 + float((v >> (64 * i)) & MASK)
    return d


def to_double_128(v):
    return float((v >> 64) & MASK) * T64 + float(v & MASK)


worst = Fraction(0)
for _ in range(200000):
    nb = rng.randint(1, 160)
    db = rng.randint(1, 120)
    num = rng.getrandbits(nb) | (1 << (nb - 1))
    den = rng.getrandbits(db) | (1 << (db - 1))
    if rng.random() < 0.3:  # mots charnieres : tous les bits a 1 dans un mot, bit isole dans un autre
        num |= MASK << (64 * rng.randint(0, 1))
        num &= (1 << 192) - 1
    a = to_double_192(num) / to_double_128(den)
    exact = Fraction(num, den)
    rel = abs(Fraction(a) - exact) / exact
    if rel > worst:
        worst = rel
print('erreur relative maximale observee : %.3f * 2^-53 (2^-50 = 8 * 2^-53)' % float(worst * 2 ** 53))
