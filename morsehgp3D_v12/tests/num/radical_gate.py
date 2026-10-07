#!/usr/bin/env python3
"""Porte mhgp12_num_radical (tranche S8) : comparaisons exactes de racines de num (radical.cpp) contre les fonctions
Python portees de bench/points_radius.py de la v11 (copie a la lettre radical_port.py, verifiee contre son empreinte
epinglee), decision par decision, et temoins graves des paragraphes 8.7 et 8.5 de la specification.

    python3 -S -B radical_gate.py <mhgp12_num_radical_probe>

Temoins graves (termes c sqrt(f) des scores phi(r) = 1/r = sqrt(1/r^2), extraits de bench/points_flat_oracle.py au
commit b319efc84 : composantes des clusters condenses, niveaux exacts ; l'oracle importe numpy, la porte grave donc
ses termes et recalcule les valeurs avec la copie portee) :
  F5 (t = 0)  S(A u B) = S(A) + S(B) = 1/4, S(A) = 1/8 ; niveaux carres parfaits 36, 64, 144 (defaut b4632db51) ;
  F6          S(A u B) = S(A) + S(B) = sqrt(2)/8 ; radicandes 72, 128, 288 de la classe de 2 ;
  F8          cote T 7/6 = 5/6 + 1/3, cote A 7/12 = 5/12 + 1/6 ;
  F14a        S(D1) + S(D2) - S(P) = 4/a - 3 = 2^-70, 1/a = 3/4 + 2^-72 ;
  5 sqrt 2    dates (2, 162, 50) et (8, 98, 32) egales (contre-garde hm_review_20261003) ;
  annulation  t = 1/4, m = 14 000 000^2, q = (14 000 000 - 6/997)^2 contre 1/2 + 6/997 + 2^-70 : signe -1 ;
  quasi       sqrt(n^2) - 2 sqrt(n^2 + 1) + sqrt(n^2 + 2), n = 2^40 : trois classes, ecart d'environ -2^-122,
              tranche a 192 bits, jamais une egalite ;
  budget      la meme famille a n = 2^2050 + 1 : ecart d'environ -2^-6152, refus radical_sign_budget a l'epuisement
              des precisions (Python : Refusal) ;
  capacite    radicande de 5 201 bits, ecart du premier ordre separe a 6 144 bits par Python : refus
              radical_sign_budget en C++ (capacite de Big, 17 408 bits ; divergence declaree, jamais une decision
              differente) ;
  termes      17 termes : refus radical_sign_budget (budget declare de 16 termes).
Differentiel : sqrt_cmp2, sqrt_diff_cmp, dates trois contre trois et sommes generales sur des egalites construites
(classes de carres), des quasi-egalites et des cas aleatoires ; le signe, le refus, le nombre de classes et la
precision qui a tranche doivent etre ceux de Python.

Codes : 0 conforme ; 1 desaccord ; 2 usage ; 3 plancher, copie divergente du banc ou sonde en echec.
Ligne finale si conforme : num_radical_verdict conforme temoins=W decisions=N egalites=E raffinees=R refus=F
Bibliotheque standard seule, aucun assert (tient sous python3 -O et -S).
"""
from fractions import Fraction as F
import os
import random
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import radical_port as rp  # noqa: E402

MIN_DECISIONS = 5000
MIN_EQUALITIES = 600
MIN_REFINED = 300
REFUSED = 'refused radical_sign_budget'
CAPACITY = 16384 + 1024
# Refus du C++ a son budget declare (capacite de Big, 16 termes) la ou Python, sans borne, decide.
DECLARED_DIVERGENCES = ('capacite_radicande_5201_bits', 'termes_17')


def hexs(v):
    return ('-' if v < 0 else '') + format(abs(v), 'x')


def rat(q):
    q = F(q)
    return hexs(q.numerator) + ('' if q.denominator == 1 else '/' + hexs(q.denominator))


def classes_and_bits(terms):
    """Classes (radical_classes) et precision qui tranche (boucle de sign_of_radicals), pour la trace native."""
    classes = rp.radical_classes(terms)
    if len(classes) <= 2:
        return len(classes), 0
    bits = 96
    while bits <= 8192:
        lo = hi = rp.ZERO
        for rep, coef in classes:
            a, b = rp.sqrt_bounds(rep, bits)
            lo += coef * (a if coef > 0 else b)
            hi += coef * (b if coef > 0 else a)
        if lo > 0 or hi < 0:
            return len(classes), bits
        bits *= 2
    return len(classes), 0


def expect_sum(terms):
    try:
        s = rp.sign_of_radicals(terms)
    except rp.Refusal:
        return REFUSED
    c, b = classes_and_bits(terms)
    return 'ok %d %d %d' % (s, c, b)


def sum_line(terms):
    return 'sum ' + ' '.join('%s %s' % (rat(c), rat(f)) for c, f in terms)


def dates_terms(x, y):
    return [(1, x[0]), (1, x[1]), (-1, x[2]), (-1, y[0]), (-1, y[1]), (1, y[2])]


def score(parts):
    return [(F(m), F(f)) for m, f in parts]


def witnesses():
    """(nom, ligne de requete, reponse attendue de la sonde, reponse de la copie Python). Pour les noms de
    DECLARED_DIVERGENCES, la copie Python doit decider (ok) la ou le C++ refuse a son budget declare."""
    out = []
    f5_p, f5_a = score([(6, F(1, 64)), (-6, F(1, 144))]), score([(3, F(1, 36)), (-3, F(1, 64))])
    f6_p, f6_a = score([(6, F(1, 128)), (-6, F(1, 288))]), score([(3, F(1, 72)), (-3, F(1, 128))])
    f8t_p = score([(6, F(4, 81)), (-6, F(1, 1296))])
    f8t_a, f8t_b = score([(3, F(1, 4)), (-3, F(4, 81))]), score([(3, F(1, 9)), (-3, F(4, 81))])
    f8a_p = score([(6, F(1, 81)), (-6, F(1, 5184))])
    f8a_a, f8a_b = score([(3, F(1, 16)), (-3, F(1, 81))]), score([(3, F(1, 36)), (-3, F(1, 81))])

    def neg(parts):
        return [(-c, f) for c, f in parts]

    def const(q):  # q = sqrt(q^2)
        return [(F(1), F(q) * F(q))]
    equal = [
        ('F5_egalite_S(AuB)=S(A)+S(B)', f5_p + neg(f5_a) + neg(f5_a)),
        ('F5_S(AuB)=1/4', f5_p + neg(const(F(1, 4)))),
        ('F5_S(A)=1/8', f5_a + neg(const(F(1, 8)))),
        ('F6_egalite_S(AuB)=S(A)+S(B)', f6_p + neg(f6_a) + neg(f6_a)),
        ('F6_S(AuB)=sqrt(2)/8', f6_p + [(F(-1), F(1, 32))]),
        ('F8T_S(AuB)=7/6', f8t_p + neg(const(F(7, 6)))),
        ('F8T_egalite_5/6+1/3', f8t_p + neg(f8t_a) + neg(f8t_b)),
        ('F8A_S(AuB)=7/12', f8a_p + neg(const(F(7, 12)))),
        ('F8A_egalite_5/12+1/6', f8a_p + neg(f8a_a) + neg(f8a_b)),
    ]
    for name, terms in equal:
        out.append((name, sum_line(terms), 'ok 0 0 0', expect_sum(terms)))
    inv_a = F(3, 4) + F(1, 2 ** 72)
    f14 = [(F(2), inv_a * inv_a), (F(-2), F(1, 4)), (F(2), inv_a * inv_a), (F(-2), F(1, 4)), (F(-4), F(1, 4)),
           (F(4), F(1, 16))]
    out.append(('F14a_ecart_positif', sum_line(f14), 'ok 1 1 0', expect_sum(f14)))
    gap = f14 + [(F(-1), F(1, 2 ** 140))]
    out.append(('F14a_ecart=2^-70', sum_line(gap), 'ok 0 0 0', expect_sum(gap)))
    below = f14 + [(F(-1), (F(1, 2 ** 70) + F(1, 2 ** 200)) ** 2)]
    out.append(('F14a_ecart<2^-70+2^-200', sum_line(below), 'ok -1 1 0', expect_sum(below)))
    x, y = (F(2), F(162), F(50)), (F(8), F(98), F(32))
    line = 'dates ' + ' '.join(rat(v) for v in x + y)
    out.append(('5_racine_2_dates', line, 'ok 0 0 0', expect_sum(dates_terms(x, y))))
    out.append(('5_racine_2_dates_inverse', 'dates ' + ' '.join(rat(v) for v in y + x), 'ok 0 0 0',
                expect_sum(dates_terms(y, x))))
    target = F(1, 2) + F(6, 997) + F(1, 2 ** 70)
    t, m, q = F(1, 4), F(14000000) ** 2, (F(14000000) - F(6, 997)) ** 2
    line = 'cmp2 %s %s %s %s' % (rat(t), rat(m), rat(target * target), rat(q))
    out.append(('filtre_annulation', line, 'ok -1', 'ok %d' % rp.sqrt_cmp2(t, m, target * target, q)))
    def second_order(n):  # sqrt(n^2) - 2 sqrt(n^2 + 1) + sqrt(n^2 + 2) = -1/(4 n^3) + ... : trois classes
        return [(F(1), F(n * n)), (F(-2), F(n * n + 1)), (F(1), F(n * n + 2))]
    quasi = second_order(2 ** 40)
    out.append(('quasi_egalite_2^-122', sum_line(quasi), 'ok -1 3 192', expect_sum(quasi)))
    budget = second_order(2 ** 2050 + 1)
    out.append(('budget_2^-6152', sum_line(budget), REFUSED, expect_sum(budget)))
    n1 = 2 ** 2600 + 1  # premier ordre : 1/(2 n^2), separe a 6 144 bits par Python ; radicande de 5 201 bits
    capacity = [(F(1), F(n1 * n1 + 1)), (F(-1), F(n1 * n1)), (F(-1), F((n1 + 1) ** 2 + 1)), (F(1), F((n1 + 1) ** 2))]
    out.append(('capacite_radicande_5201_bits', sum_line(capacity), REFUSED, expect_sum(capacity)))
    many = [(F(1), F(k)) for k in range(2, 19)]
    out.append(('termes_17', sum_line(many), REFUSED, expect_sum(many)))
    return out


def rand_rat(rng, bits=40):
    return F(rng.getrandbits(bits), rng.getrandbits(rng.randrange(1, max(bits, 2))) or 1)


def differential(seed=20261005):
    """(ligne, attendu) : egalites construites, quasi-egalites, cas aleatoires."""
    rng = random.Random(seed)
    cases = []
    squarefree = (2, 3, 5, 6, 7, 10, 11, 13, 14, 15, 30, 105, 1155, 2 ** 61 - 1)
    for i in range(2200):  # sqrt_cmp2
        kind = i % 4
        if kind == 0:  # egalite par echange ou par classe : sqrt(k1^2 s) + sqrt(k2^2 s) = sqrt(k3^2 s) + sqrt(k4^2 s)
            s = F(rng.choice(squarefree), rng.choice((1, 4, 9, 7, 28)))
            k = [rng.randrange(0, 50) for _ in range(3)]
            k4 = k[0] + k[1] - k[2]
            if k4 < 0:
                k[2], k4 = k[0] + k[1], 0
            a, b, c, d = (F(v * v) * s for v in (k[0], k[1], k[2], k4))
        elif kind == 1:  # quasi-egalite
            a, b = rand_rat(rng), rand_rat(rng)
            eps = F(1, 2 ** rng.randrange(30, 200))
            c, d = a + eps, max(b - eps, F(0))
        elif kind == 2:  # memes valeurs
            a, b = rand_rat(rng), rand_rat(rng)
            c, d = (b, a) if rng.random() < 0.5 else (a, b)
        else:
            a, b, c, d = (rand_rat(rng, rng.randrange(2, 120)) for _ in range(4))
        cases.append(('cmp2 %s %s %s %s' % (rat(a), rat(b), rat(c), rat(d)), 'ok %d' % rp.sqrt_cmp2(a, b, c, d)))
    for i in range(1200):  # sqrt_diff_cmp
        p, q = rand_rat(rng, 30), rand_rat(rng, 30)
        x, y = (p + q) ** 2, q * q
        u = [p, p + F(1, 2 ** rng.randrange(20, 150)), -p, rand_rat(rng) - rand_rat(rng), F(0)][i % 5]
        if i % 7 == 0:
            x, y = y, x
        cases.append(('diff %s %s %s' % (rat(x), rat(y), rat(u)), 'ok %d' % rp.sqrt_diff_cmp(x, y, u)))
    for i in range(1800):  # dates trois contre trois
        kind = i % 3
        if kind == 0:  # egalite : classes reordonnees (famille 5 sqrt 2)
            s = F(rng.choice(squarefree), rng.choice((1, 2, 3, 25)))
            k = [rng.randrange(1, 40) for _ in range(5)]
            k6 = k[3] + k[4] - (k[0] + k[1] - k[2])
            if k6 < 0:
                k[2] += -k6
                k6 = 0
            x = tuple(F(v * v) * s for v in k[:3])
            y = (F(k[3] ** 2) * s, F(k[4] ** 2) * s, F(k6 * k6) * s)
        elif kind == 1:  # quasi-egalite sur plusieurs classes
            x = (rand_rat(rng, 60), rand_rat(rng, 60), rand_rat(rng, 60))
            eps = F(1, 2 ** rng.randrange(40, 400))
            y = (x[0] + eps, x[1], x[2] + eps * rng.choice((0, 1, 2)))
        else:
            x = tuple(rand_rat(rng, rng.randrange(2, 90)) for _ in range(3))
            y = tuple(rand_rat(rng, rng.randrange(2, 90)) for _ in range(3))
        cases.append(('dates ' + ' '.join(rat(v) for v in x + y), expect_sum(dates_terms(x, y))))
    for i in range(700):  # sommes generales, jusqu'a 10 termes
        terms = [(F(rng.randrange(-5, 6), rng.randrange(1, 7)), rand_rat(rng, rng.randrange(2, 80)))
                 for _ in range(rng.randrange(1, 11))]
        if i % 2 == 0:  # ajoute l'oppose d'une recombinaison exacte des memes classes
            c, f = terms[0]
            terms.append((-c * 3, f / 9))
        cases.append((sum_line(terms), expect_sum(terms)))
    return cases


def main(argv):
    if len(argv) != 2:
        sys.stderr.write(__doc__)
        return 2
    mismatched = rp.pin_mismatches()
    if mismatched:
        print('num_radical_verdict copie_divergente %s' % ','.join(mismatched))
        return 3
    graved = witnesses()
    cases = differential()
    for name, _, want, python in graved:
        if name in DECLARED_DIVERGENCES:
            if not python.startswith('ok '):
                print('num_radical_verdict divergence_non_declaree %s python=%s' % (name, python))
                return 3
        elif python != want:  # la copie portee doit elle-meme rendre le temoin grave
            print('num_radical_verdict temoin_python %s attendu=%s python=%s' % (name, want, python))
            return 3
    lines = [line for _, line, _, _ in graved] + [line for line, _ in cases]
    try:
        run = subprocess.run([argv[1]], input='\n'.join(lines) + '\n', capture_output=True, text=True, timeout=600)
    except (OSError, subprocess.SubprocessError) as error:
        print('num_radical_verdict sonde_impossible %s' % error)
        return 3
    out = run.stdout.splitlines()
    if run.returncode != 0 or len(out) != len(lines):
        print('num_radical_verdict sonde code=%d lignes=%d attendues=%d' % (run.returncode, len(out), len(lines)))
        return 3
    gaps = []
    for (name, line, want, _), got in zip(graved, out):
        if got != want:
            gaps.append('temoin %s obtenu=%s attendu=%s' % (name, got, want))
    equalities = refined = refusals = 0
    for (line, want), got in zip(cases, out[len(graved):]):
        if got != want:
            gaps.append('%s obtenu=%s attendu=%s' % (line[:90], got, want))
        words = want.split()
        equalities += len(words) >= 2 and words[0] == 'ok' and words[1] == '0'
        refined += len(words) == 4 and words[3] != '0'
        refusals += want == REFUSED
    for gap in gaps[:10]:
        print('ecart ' + gap)
    if gaps:
        print('num_radical_verdict desaccord ecarts=%d' % len(gaps))
        return 1
    if len(cases) < MIN_DECISIONS or equalities < MIN_EQUALITIES or refined < MIN_REFINED:
        print('num_radical_verdict plancher decisions=%d egalites=%d raffinees=%d' % (len(cases), equalities, refined))
        return 3
    print('num_radical_verdict conforme temoins=%d decisions=%d egalites=%d raffinees=%d refus=%d'
          % (len(graved), len(cases), equalities, refined, refusals))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
