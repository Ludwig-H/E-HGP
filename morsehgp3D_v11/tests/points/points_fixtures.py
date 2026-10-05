#!/usr/bin/env python3
"""Porte mhgp11_points_fixtures (tranche S9) : les douze fixtures de bench/points_gate.py (fixtures et
arithmetic_fixtures), rejouees en bibliotheque standard sur la sonde native, plus la fixture K = 1 (liaison simple).

    points_fixtures.py --probe <mhgp11_points_probe> --work <dossier>

La seule regle native est H^r_m (marge en rayon) : les trois fixtures du banc ecrites pour la marge en niveau carre
(deux triangles a m = 1, cinq points a m = 3, continuite {0, 2, 4}) sont rejouees sous la regle en rayon, attendus
recalcules et graves ici (la marge carree est le mutant marge_carree). Faits, en PointId (indices d'entree) :
  F1  deux triangles equilateraux, K = 2, m = 3 : blocs au niveau 1 = ABC | DEF ;
  F2  deux triangles, K = 2, m = 1 : blocs au niveau 1 = {0, 1} | {4, 5} (C et D attendent la racine) ;
  F3  cinq points, K = 2, m = 3 : blocs au niveau 35 = {1, 2} | {3, 4} ; le point 0 entre exactement au rayon 6 ;
  F4  cinq points : le point 0 entre au plateau (36, 0, 0), plancher 36 non strict ; un seul bloc au niveau 36 ;
  F5  continuite {0, 2000, 4000 + d}, K = 2, m = 1 : le site median entre exactement au rayon 2000 pour d = 0 et
      d = 1 (la marge carree sautait de 4 000 000 a 4 001 000) ;
  F6  quatre sites de l'auditeur, K = 2, m = 1 : date sqrt 2 + sqrt 18 - sqrt 8 = sqrt 8, proprietaire = le parent ne
      a 8 qui couvre {0, 1, 3} (coupe fermee) ;
  F7  huit sites de l'auditeur, K = 2, m = 3 : le point 0 entre a sqrt 100 + sqrt 250 - sqrt(625/4), strictement entre
      deux niveaux ;
  F8  egalite 5 sqrt 2 : dates (2, 162, 50) et (8, 98, 32), dans les deux sens (sonde --arith, num::compare_dates) ;
  F9  filtre d'annulation : sqrt(1/4) + 14e6 - (14e6 - 6/997) contre 1/2 + 6/997 + 2^-70 : -1 (num::sqrt_cmp2) ;
  F10 3 000 dates trois racines contre trois, contre decimal a 200 chiffres ;
  F11 4 000 comparaisons deux racines contre deux contre une routine propre a la porte (two_roots_sign de l'oracle),
      dont au moins 1 000 egalites construites ;
  F12 deux triangles, K = 2, m = 3 : arbre de points a deux plateaux (2/3, 3/2), deux blocs d'entree au premier, un
      bloc de fusion au second, parent des deux ;
  F13 K = 1 (m(1) = 1, sans --m) sur 30 nuages : toutes les entrees a 0 sur la feuille du site, et a chaque plateau
      les blocs de la liaison simple (paires reunies quand (d / 2)^2 <= niveau).
Codes : 0 conforme ; 1 fixture fausse ; 2 refus (sonde en echec). Derniere ligne :
    points_fixtures_verdict <conforme|desaccord|refus> fixtures=<ok>/<total> nuages_k1=30 comparaisons=7000
"""
import argparse
import decimal
from fractions import Fraction
import os
import random
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import points_oracle_stdlib as po  # noqa: E402

formats = po.formats
NONE = po.NONE


def hexr(f):
    """Rationnel en hexadecimal signe de la sonde --arith."""
    f = Fraction(f)
    sign = '-' if f < 0 else ''
    return '%s%x/%x' % (sign, abs(f.numerator), f.denominator)


def arith(args, lines):
    done = subprocess.run([args.probe, '--arith'], input='\n'.join(lines) + '\n', stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, universal_newlines=True, timeout=300, check=False)
    rows = done.stdout.splitlines()
    if done.returncode != 0 or len(rows) != len(lines) + 1:
        raise po.ProbeError('sonde --arith : code %d' % done.returncode)
    return [int(r) if r.lstrip('-').isdigit() else None for r in rows[:-1]]


def levels_of(dump):
    return {int(r): Fraction(int(v[0], 16), int(v[1], 16)) for r, v in dump['levels'].items()}


def date_of(dump, lv, pid):
    s = dump['ids'].index(pid)
    return (lv[dump['t'][s]], lv[dump['M'][s]], lv[dump['Q'][s]])


def blocks_at(dump, lv, level):
    """Blocs (PointId) de l'arbre de points a la coupe fermee du rayon sqrt(level)."""
    value = (Fraction(level), Fraction(0), Fraction(0))

    def plateau(p):
        return (lv[dump['plateau_t'][p]], lv[dump['plateau_m'][p]], lv[dump['plateau_q'][p]])
    groups = {}
    for s in range(dump['n']):
        if po.date_sign(plateau(dump['site_plateau'][s]), value) > 0:
            continue
        b = dump['site_block'][s]
        while dump['block_parent'][b] != NONE and po.date_sign(plateau(dump['block_plateau'][dump['block_parent'][b]]),
                                                               value) <= 0:
            b = dump['block_parent'][b]
        groups.setdefault(b, []).append(dump['ids'][s])
    return sorted(sorted(g) for g in groups.values())


def geometric(args, facts):
    tri3 = po.probe(args, po.EQUILATERAL, 'tri3', 2, 3)
    lv = levels_of(tri3)
    facts.append(('F1_deux_triangles_rayon_k2_m3', blocks_at(tri3, lv, 1) == [[0, 1, 2], [3, 4, 5]]))
    tri1 = po.probe(args, po.EQUILATERAL, 'tri1', 2, 1)
    facts.append(('F2_deux_triangles_rayon_k2_m1', blocks_at(tri1, levels_of(tri1), 1) == [[0, 1], [4, 5]]))
    five = po.probe(args, po.FIVE, 'five', 2, 3)
    flv = levels_of(five)
    facts.append(('F3_cinq_points_rayon_k2_m3', blocks_at(five, flv, 35) == [[1, 2], [3, 4]] and
                  po.date_sign(date_of(five, flv, 0), (Fraction(36), 0, 0)) == 0))
    s0 = five['ids'].index(0)
    p0 = five['site_plateau'][s0]
    facts.append(('F4_cinq_points_plancher', flv[five['floor'][s0]] == 36 and five['strict'][s0] == 0 and
                  (flv[five['plateau_t'][p0]], five['plateau_m'][p0], five['plateau_q'][p0]) == (36, 0, 0) and
                  blocks_at(five, flv, 36) == [[0, 1, 2, 3, 4]]))
    medians = []
    for d in (0, 1):
        line = po.probe(args, [(0, 0, 0), (2000, 0, 0), (4000 + d, 0, 0)], 'line%d' % d, 2, 1)
        medians.append(po.date_sign(date_of(line, levels_of(line), 1), (Fraction(4000000), 0, 0)))
    facts.append(('F5_continuite_024_rayon_k2_m1', medians == [0, 0]))
    plat = po.probe(args, po.PLATEAU, 'plateau', 2, 1)
    plv = levels_of(plat)
    s = plat['ids'].index(0)
    owner = plat['owner'][s]
    facts.append(('F6_plateau_proprietaire_k2_m1', po.date_sign(date_of(plat, plv, 0), (Fraction(8), 0, 0)) == 0 and
                  plv[plat['rank'][owner]] == 8 and plat['cover'][owner] == [0, 1, 3]))
    eight = po.probe(args, po.EIGHT, 'eight', 2, 3)
    elv = levels_of(eight)
    facts.append(('F7_huit_sites_retard_qualifie', po.date_sign(date_of(eight, elv, 0),
                                                                 (Fraction(100), Fraction(250), Fraction(625, 4))) == 0
                  and eight['strict'][eight['ids'].index(0)] == 1))
    facts.append(('F12_deux_triangles_arbre', [lv[r] for r in tri3['plateau_t']] == [Fraction(2, 3), Fraction(3, 2)] and
                  tri3['plateau_m'] == [0, 0] and tri3['block_plateau'] == [0, 0, 1] and
                  tri3['block_parent'] == [2, 2, NONE]))


def arithmetic(args, facts):
    a, b = (2, 162, 50), (8, 98, 32)
    got = arith(args, ['d ' + ' '.join(hexr(x) for x in a + b), 'd ' + ' '.join(hexr(x) for x in b + a)])
    facts.append(('F8_egalite_5_racine_2', got == [0, 0]))
    target = Fraction(1, 2) + Fraction(6, 997) + Fraction(1, 2 ** 70)
    witness = (Fraction(1, 4), Fraction(14000000) ** 2, target * target, (Fraction(14000000) - Fraction(6, 997)) ** 2)
    facts.append(('F9_filtre_annulation', arith(args, ['c ' + ' '.join(hexr(x) for x in witness)]) == [-1]))
    rng = random.Random(20261003)
    ctx = decimal.Context(prec=200)

    def dec(v):
        with decimal.localcontext(ctx):
            def root(f):
                return (decimal.Decimal(f.numerator) / decimal.Decimal(f.denominator)).sqrt()
            return root(v[0]) + root(v[1]) - root(v[2])
    lines, expected = [], []
    for _ in range(3000):
        vals = []
        for _ in range(2):
            base = Fraction(rng.randint(1, 50), rng.randint(1, 9))

            def pick():
                if rng.random() < 0.5:
                    return base * Fraction(rng.randint(1, 12)) ** 2
                return Fraction(rng.randint(1, 10 ** rng.randint(1, 30)), rng.randint(1, 10 ** rng.randint(1, 12)))
            vals.append((pick(), pick(), pick()))
        with decimal.localcontext(ctx):
            gap = dec(vals[0]) - dec(vals[1])
        expected.append(0 if abs(gap) < decimal.Decimal(10) ** -150 else (1 if gap > 0 else -1))
        lines.append('d ' + ' '.join(hexr(x) for x in vals[0] + vals[1]))
    facts.append(('F10_trois_contre_trois_decimal', arith(args, lines) == expected))
    lines, expected, equal = [], [], 0
    for _ in range(4000):
        base = Fraction(rng.randint(1, 30), rng.randint(1, 7))
        if rng.random() < 0.5:
            u, v, w = (Fraction(rng.randint(0, 9)) for _ in range(3))
            if u + v < w:
                u, w = w, u
            x = u + v - w
            quad = (base * u * u, base * v * v, base * w * w, base * x * x)
        else:
            quad = tuple(base * Fraction(rng.randint(0, 12)) ** 2 if rng.random() < 0.5 else
                         Fraction(rng.randint(0, 10 ** rng.randint(1, 20)), rng.randint(1, 10 ** rng.randint(1, 9)))
                         for _ in range(4))
        sign = po.two_roots_sign(*quad)
        equal += sign == 0
        expected.append(sign)
        lines.append('c ' + ' '.join(hexr(x) for x in quad))
    facts.append(('F11_oracle_contre_natif_deux_racines', arith(args, lines) == expected and equal >= 1000))


def single_linkage(points, level):
    """Blocs de la liaison simple : i et j reunis si (d_ij / 2)^2 <= level."""
    n = len(points)
    parent = list(range(n))

    def find(x):
        while parent[x] != x:
            x = parent[x]
        return x
    for i in range(n):
        for j in range(i + 1, n):
            if Fraction(sum((a - b) ** 2 for a, b in zip(points[i], points[j])), 4) <= level:
                parent[find(i)] = find(j)
    groups = {}
    for i in range(n):
        groups.setdefault(find(i), []).append(i)
    return sorted(sorted(g) for g in groups.values())


def order_one(args, facts):
    rng = random.Random(1313)
    bad = 0
    for index in range(30):
        pts = po.random_cloud(rng)
        dump = po.probe(args, pts, 'k1_%d' % index, 1, None)
        lv = levels_of(dump)
        ok = dump['m'] == 1 and all(lv[dump[c][s]] == 0 for c in ('t', 'M', 'Q', 'floor') for s in range(dump['n']))
        ok = ok and not any(dump['strict']) and all(dump['cover'][dump['owner'][s]] == [dump['ids'][s]]
                                                    for s in range(dump['n']))
        for p in range(len(dump['plateau_t'])):
            level = lv[dump['plateau_t'][p]]
            ok = ok and dump['plateau_m'][p] == 0 and blocks_at(dump, lv, level) == single_linkage(pts, level)
        bad += not ok
    facts.append(('F13_k1_liaison_simple', bad == 0))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', required=True)
    parser.add_argument('--work', required=True)
    args = parser.parse_args()
    os.makedirs(args.work, exist_ok=True)
    facts = []
    try:
        geometric(args, facts)
        arithmetic(args, facts)
        order_one(args, facts)
    except (po.ProbeError, OSError, ValueError, subprocess.TimeoutExpired) as error:
        print('refus : %s' % error, file=sys.stderr)
        print('points_fixtures_verdict refus fixtures=%d/%d' % (sum(ok for _, ok in facts), len(facts)))
        return 2
    for name, ok in facts:
        if not ok:
            print('FIXTURE FAUSSE %s' % name)
    good = sum(ok for _, ok in facts)
    verdict = 'conforme' if good == len(facts) == 13 else 'desaccord'
    print('points_fixtures_verdict %s fixtures=%d/%d nuages_k1=30 comparaisons=7000' % (verdict, good, len(facts)))
    return 0 if verdict == 'conforme' else 1


if __name__ == '__main__':
    raise SystemExit(main())
