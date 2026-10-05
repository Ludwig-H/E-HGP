#!/usr/bin/env python3
"""Porte mhgp11_num_roots (tranche S8) : table des racines num::RootTable contre math.isqrt et la copie portee de
sign_of_radicals (radical_port.py).

    python3 -S -B roots_gate.py <mhgp11_num_roots_probe> <bench/points_radius.py>

Controles :
  - R = isqrt((N << 128) // D) pour chaque niveau N / D non reduit des budgets du profil (numerateur 8B + 12 bits,
    denominateur 6B + 8 bits), et le certificat independant R^2 D <= N 2^128 < (R + 1)^2 D ; cas limites : niveau
    nul, D = 1, carres parfaits, sqrt(N / D) juste sous 2^(B+1) ; refus : sqrt(N / D) >= 2^(B+1)
    (arithmetic_invariant), N ou D hors budget et D nul (parameter_out_of_range, Level::make) ;
  - table construite (build) et partielle (allocate puis fill_ranks : rangs absents, somme sur un rang absent
    refusee arithmetic_invariant) ;
  - encadrement d'une somme signee de j racines : lo et hi exacts (sommes de R et R + 1), et la valeur vraie dans
    [lo, hi] 2^-64 (verifie terme a terme par le certificat) ;
  - signe : celui de sign_of_radicals sur les fractions N / D, que la table tranche seule (0 hors de [lo, hi]) ou par
    le repli exact (egalites de niveaux ecrits autrement, classes de carres, temoin 5 sqrt 2, quasi-egalites du
    second ordre) ; drapeau de repli exact et nombre de classes egaux a ceux attendus ; refus a 17 termes
    (radical_sign_budget) et signe hors de {-1, 1} (arithmetic_invariant).

Codes : 0 conforme ; 1 desaccord ; 2 usage ; 3 plancher, copie divergente ou sonde en echec.
Ligne finale si conforme : num_roots_verdict conforme bits=B racines=N sommes=S replis=F egalites=E raffinees=R
Bibliotheque standard seule, aucun assert (tient sous python3 -O et -S).
"""
from fractions import Fraction as F
import math
import os
import random
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import radical_port as rp  # noqa: E402

MIN_ROOTS = 3000
MIN_SUMS = 4000
MIN_FALLBACKS = 400
MIN_EQUALITIES = 200
MIN_REFINED = 50


def hexs(v):
    return ('-' if v < 0 else '') + format(abs(v), 'x')


class Plan(object):
    def __init__(self, bits):
        self.b = bits
        self.nb, self.db = 8 * bits + 12, 6 * bits + 8
        self.lines, self.checks = [], []

    def ask(self, line, check):
        self.lines.append(line)
        self.checks.append(check)

    def expected_root(self, n, d):
        if d <= 0 or n < 0 or n.bit_length() > self.nb or d.bit_length() > self.db:
            return 'refused parameter_out_of_range'
        r = math.isqrt((n << 128) // d)
        if r.bit_length() > self.b + 65:
            return 'refused arithmetic_invariant'
        return 'ok ' + hexs(r)


def certified(n, d, r):
    """Certificat independant de isqrt : R^2 D <= N 2^128 < (R + 1)^2 D."""
    return r * r * d <= n << 128 < (r + 1) * (r + 1) * d


def random_level(rng, plan):
    d = rng.getrandbits(rng.randrange(1, plan.db + 1)) or 1
    limit = min(1 << plan.nb, d << (2 * plan.b + 2))
    return rng.randrange(limit), d


def single_roots(rng, plan):
    cases = []
    top = 1 << (2 * plan.b + 2)
    dmax, nmax = (1 << plan.db) - 1, (1 << plan.nb) - 1
    cases += [(0, 1), (0, dmax), (1, 1), (top - 1, 1), (top, 1), (nmax, dmax), (nmax, 1), (1, dmax),
              ((top << 40) - 1, 1 << 40), (top << 40, 1 << 40), (nmax + 1, 1), (1, dmax + 1), (1, 0), (5, -3)]
    for _ in range(2200):
        cases.append(random_level(rng, plan))
    for _ in range(500):  # carres parfaits a^2 m / (b^2 m)
        a, b = rng.randrange(1, 1 << plan.b), rng.randrange(1, 1 << 20)
        m = rng.randrange(1, 1 << 30)
        cases.append((a * a * m, b * b * m))
    for _ in range(300):  # juste sous et sur la borne geometrique
        d = rng.getrandbits(rng.randrange(1, plan.db - 2 * plan.b - 4)) or 1
        cases.append((d * top - rng.randrange(1, 3), d))
        cases.append((d * top + rng.randrange(0, 3), d))
    for n, d in cases:
        want = plan.expected_root(n, d)
        if want.startswith('ok ') and not certified(n, d, int(want[3:], 16)):
            return None
        plan.ask('root %s %s' % (hexs(n), hexs(d)), ('root', want))
    return len(cases)


def table_levels(rng, plan):
    """Niveaux de la table : aleatoires, jumeaux ecrits autrement, classes de carres, 5 sqrt 2, second ordre."""
    levels, groups = [], {}

    def add(n, d, tag=None):
        levels.append((n, d))
        if tag is not None:
            groups.setdefault(tag, []).append(len(levels) - 1)
    for _ in range(240):
        n, d = random_level(rng, plan)
        add(n, d, 'aleatoire')
        k = rng.randrange(2, 1 << 12)
        if (n * k).bit_length() <= plan.nb and (d * k).bit_length() <= plan.db:
            add(n * k, d * k, 'jumeau')  # meme valeur que le precedent
            groups.setdefault('paires', []).append((len(levels) - 2, len(levels) - 1))
    for s in (2, 3, 5, 6, 7):  # k^2 s / q^2 pour k = 1..9 : classe de s
        q = rng.randrange(1, 1 << 10)
        for k in range(1, 10):
            add(k * k * s, q * q, 'classe%d' % s)
    for v in (2, 162, 50, 8, 98, 32):
        add(v, 1, 'cinq_racine_2')
    for x in (1 << 18, (1 << 19) + 7, (1 << 20) - 3):  # (x^2 + j) / D, j = 0, 1, 2
        big = (1 << (plan.db - 1)) + rng.randrange(1 << 20)
        for j in range(3):
            add(x * x + j, big, 'ordre2_%d' % x)
    return levels, groups


def expected_sum(levels, roots, terms):
    lo = sum(roots[r] if s > 0 else -(roots[r] + 1) for r, s in terms)
    hi = sum(roots[r] + 1 if s > 0 else -roots[r] for r, s in terms)
    fr = [(s, F(levels[r][0], levels[r][1])) for r, s in terms]
    try:
        sign = rp.sign_of_radicals(fr)
    except rp.Refusal:
        return 'refused radical_sign_budget', None
    fallback = lo <= 0 <= hi
    classes = len(rp.radical_classes(fr)) if fallback else 0
    return 'ok %s %s %d %d %d' % (hexs(lo), hexs(hi), sign, 1 if fallback else 0, classes), fallback


def sums(rng, levels, groups, roots, plan):
    out = []
    n = len(levels)
    for _ in range(3000):
        out.append([(rng.randrange(n), rng.choice((1, -1))) for _ in range(rng.randrange(1, 7))])
    for a, b in groups['paires']:
        out.append([(a, 1), (b, -1)])
        out.append([(b, 1), (a, -1), (rng.randrange(n), rng.choice((1, -1)))])
    for s in (2, 3, 5, 6, 7):
        ranks = groups['classe%d' % s]  # rang i : (i+1) sqrt(s) / q
        for _ in range(60):
            i, j = rng.randrange(9), rng.randrange(9)
            if i + j + 1 < 9:
                out.append([(ranks[i], 1), (ranks[j], 1), (ranks[i + j + 1], -1)])
            out.append([(ranks[i], 1), (ranks[j], -1), (ranks[min(i, j)], 1), (ranks[max(i, j)], -1)])
    r = groups['cinq_racine_2']
    out.append([(r[0], 1), (r[1], 1), (r[2], -1), (r[3], -1), (r[4], -1), (r[5], 1)])
    for key in [k for k in groups if k.startswith('ordre2_')]:
        a, b, c = groups[key]
        for _ in range(30):
            out.append([(a, 1), (b, -1), (b, -1), (c, 1)])
            out.append([(a, -1), (b, 1), (b, 1), (c, -1)])
    return out


def main(argv):
    if len(argv) != 3:
        sys.stderr.write(__doc__)
        return 2
    mismatched = rp.source_mismatches(argv[2])
    if mismatched:
        print('num_roots_verdict copie_divergente %s' % ','.join(mismatched))
        return 3
    try:  # profil du binaire
        head = subprocess.run([argv[1]], input='', capture_output=True, text=True, timeout=60).stdout.split()
        bits = int(head[1])
    except (OSError, subprocess.SubprocessError, ValueError, IndexError) as error:
        print('num_roots_verdict sonde_impossible %s' % error)
        return 3
    plan = Plan(bits)
    rng = random.Random(20261005)
    n_roots = single_roots(rng, plan)
    if n_roots is None:
        print('num_roots_verdict certificat_python_faux')
        return 3
    levels, groups = table_levels(rng, plan)
    roots = [math.isqrt((n << 128) // d) for n, d in levels]
    if not all(certified(n, d, r) for (n, d), r in zip(levels, roots)):
        print('num_roots_verdict certificat_python_faux')
        return 3
    for i, (n, d) in enumerate(levels):
        plan.ask('level %s %s' % (hexs(n), hexs(d)), ('exact', 'ok %d' % i))
    subset = sorted(rng.sample(range(len(levels)), 40))
    plan.ask('partial ' + ' '.join(str(r) for r in subset), ('exact', 'ok %d' % len(levels)))
    for r in range(0, len(levels), 7):
        plan.ask('get %d' % r, ('exact', 'ok ' + hexs(roots[r]) if r in subset else 'absent'))
    absent = next(r for r in range(len(levels)) if r not in subset)
    plan.ask('sum %d 1 %d 1' % (subset[0], absent), ('exact', 'refused arithmetic_invariant'))
    plan.ask('build', ('exact', 'ok %d' % len(levels)))
    for r in range(len(levels)):
        plan.ask('get %d' % r, ('exact', 'ok ' + hexs(roots[r])))
    plan.ask('get %d' % len(levels), ('exact', 'absent'))
    plan.ask('sum ' + ' '.join('%d 1' % (r % len(levels)) for r in range(17)), ('exact', 'refused radical_sign_budget'))
    plan.ask('sum 0 1 1 0', ('exact', 'refused arithmetic_invariant'))
    fallbacks = equalities = 0
    sum_cases = sums(rng, levels, groups, roots, plan)
    for terms in sum_cases:
        want, fallback = expected_sum(levels, roots, terms)
        fallbacks += bool(fallback)
        equalities += want.split()[3] == '0' if want.startswith('ok') else 0
        plan.ask('sum ' + ' '.join('%d %d' % t for t in terms), ('sum', want))
    try:
        run = subprocess.run([argv[1]], input='\n'.join(plan.lines) + '\n', capture_output=True, text=True,
                             timeout=600)
    except (OSError, subprocess.SubprocessError) as error:
        print('num_roots_verdict sonde_impossible %s' % error)
        return 3
    out = run.stdout.splitlines()
    if run.returncode != 0 or len(out) != len(plan.lines) + 1 or out[0] != 'bits %d' % bits:
        print('num_roots_verdict sonde code=%d lignes=%d attendues=%d' % (run.returncode, len(out),
                                                                        len(plan.lines) + 1))
        return 3
    gaps, refined = [], 0
    for line, (kind, want), got in zip(plan.lines, plan.checks, out[1:]):
        if kind == 'sum' and got.startswith('ok '):
            words = got.split()
            refined += words[4] == '1' and words[6] != '0'
            got = ' '.join(words[:6])  # la precision du repli depend de l'ecriture non reduite : non comparee
        if got != want:
            gaps.append('%s obtenu=%s attendu=%s' % (line[:80], got[:100], want[:100]))
    for gap in gaps[:10]:
        print('ecart ' + gap)
    if gaps:
        print('num_roots_verdict desaccord ecarts=%d' % len(gaps))
        return 1
    if n_roots < MIN_ROOTS or len(sum_cases) < MIN_SUMS or fallbacks < MIN_FALLBACKS or \
            equalities < MIN_EQUALITIES or refined < MIN_REFINED:
        print('num_roots_verdict plancher racines=%d sommes=%d replis=%d egalites=%d raffinees=%d'
              % (n_roots, len(sum_cases), fallbacks, equalities, refined))
        return 3
    print('num_roots_verdict conforme bits=%d racines=%d sommes=%d replis=%d egalites=%d raffinees=%d'
          % (bits, n_roots, len(sum_cases), fallbacks, equalities, refined))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
