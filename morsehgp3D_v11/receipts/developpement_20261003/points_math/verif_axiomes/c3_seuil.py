#!/usr/bin/env python3
"""C3 : theoreme F (seuil de qualification m contre les cellules), balayage independant.

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B c3_seuil.py
Familles P(kappa, m) (rayon) et Q(kappa, m) (niveau carre), kappa in {1, 2, 4, 1000}, m = 1..10.
Jugements ecrits ici (lecture des cellules : QUESTIONS_UTILISATEUR v10, VERDICT_FINAL v10 § 3.1) :
  Q1bis : blocs {A,B,C} et {D,E,F} presents a un rayon < racine (1787,36).
  Q2_struct : a r = 75, bloc(x) = {x,a} et bloc(b1) = {b1,b2} ; Q2_strict : idem sur tout [61 ; 75].
  Q3_struct : a r = 790, bloc(x) contient f1 et aucun c ; Q3_strict : sur tout [705 ; 790].
  Q4_strict : a r = sqrt(750000), blocs {P,Q,R} et {P2,Q2,R2}, C, m, D hors de ces blocs ; puis bloc {C,m,D}
              a un rayon < racine (1334,17). Q4_struct : la seconde condition seule, plus PQR|P2Q2R2 avant CmD.
  QPi2 : aucun bloc d'au moins 9 sites sur [705 ; 790].
"""
import json
import sys

sys.dont_write_bytecode = True
import mpmath  # noqa: E402
import vfull as V  # noqa: E402
from c1_fixtures import FIX  # noqa: E402


def radii(rule, n):
    vals = []
    for i in range(n):
        for j in range(i, n):
            v = rule.u(i, j)
            if all(abs(v - w) > V.TOL for w in vals):
                vals.append(v)
    return sorted(vals)


def block_of(rule, r, i):
    for b in rule.blocks(r):
        if i in b:
            return b
    return None


def window(rule, n, lo, hi):
    pts = [mpmath.mpf(lo)] + [v for v in radii(rule, n) if mpmath.mpf(lo) <= v <= mpmath.mpf(hi)] + [mpmath.mpf(hi)]
    return pts


def judge(name, rule, n):
    out = {}
    if name == 'Q1':
        A, B, C, D, E, F = range(6)
        root = mpmath.sqrt(3194656)
        ok = False
        for r in radii(rule, n):
            if r < root - V.TOL and block_of(rule, r, C) == frozenset([A, B, C]) and \
                    block_of(rule, r, D) == frozenset([D, E, F]):
                ok = True
        out['Q1bis'] = ok
    if name == 'Q2':
        x, a, b1, b2 = range(4)

        def good(r):
            return block_of(rule, r, x) == frozenset([x, a]) and block_of(rule, r, b1) == frozenset([b1, b2])
        out['Q2_struct'] = good(mpmath.mpf(75))
        out['Q2_strict'] = all(good(r) for r in window(rule, n, 61, 75))
    if name == 'Q3':
        x, f1 = 0, 1
        cs = set(range(6, 14))

        def good(r):
            b = block_of(rule, r, x)
            return b is not None and f1 in b and not (b & cs)
        out['Q3_struct'] = good(mpmath.mpf(790))
        out['Q3_strict'] = all(good(r) for r in window(rule, n, 705, 790))
        out['QPi2'] = all(max([len(b) for b in rule.blocks(r)] + [0]) < 9 for r in window(rule, n, 705, 790))
    if name == 'Q4':
        C, P, Q, R_, m_, D, P2, Q2, R2 = range(9)
        r0 = mpmath.sqrt(750000)
        root = mpmath.sqrt(1780000)
        tet = block_of(rule, r0, P) == frozenset([P, Q, R_]) and block_of(rule, r0, P2) == frozenset([P2, Q2, R2])
        chain = [r for r in radii(rule, n) if r < root - V.TOL and block_of(rule, r, C) == frozenset([C, m_, D])]
        faces = [r for r in radii(rule, n) if r < root - V.TOL and block_of(rule, r, P) == frozenset([P, Q, R_])
                 and block_of(rule, r, P2) == frozenset([P2, Q2, R2])]
        out['Q4_strict'] = bool(tet and chain)
        out['Q4_struct'] = bool(chain and faces and min(faces) <= min(chain))
        out['Q4_chain_first'] = float(min(chain)) if chain else None
        out['Q4_faces_first'] = float(min(faces)) if faces else None
    return out


def main():
    rec = {}
    for name in ('Q1', 'Q2', 'Q3', 'Q4'):
        k, pts, _ = FIX[name]
        n = len(pts)
        full = V.Full(V.Cloud(pts), k)
        for m in range(1, min(10, n) + 1):
            for kap in (1, 2, 4, 1000):
                for fam, fn in (('P', V.rule_P), ('Q', V.rule_Q)):
                    rule = fn(full, kap, m)
                    rec['%s|%s%d|m%d' % (name, fam, kap, m)] = judge(name, rule, n)
            rec['%s|closure|m%d' % (name, m)] = judge(name, V.Closure(full, m), n)
    summary = {}
    for key, res in rec.items():
        name, rule, m = key.split('|')
        for crit, val in res.items():
            if isinstance(val, bool):
                summary.setdefault(crit, {}).setdefault(rule, []).append((int(m[1:]), val))
    lines = {}
    for crit, d in summary.items():
        lines[crit] = {rule: sorted(m for m, v in vals if v) for rule, vals in d.items()}
    with open('recus_c3_seuil.json', 'w') as fh:
        json.dump({'judgments': rec, 'passing_m': lines}, fh, indent=1)
    for crit, d in lines.items():
        print(crit)
        for rule, ms in d.items():
            print('   %-9s m passants : %s' % (rule, ms))
    q4 = {k: v for k, v in rec.items() if k.startswith('Q4|P1|') or k.startswith('Q4|P2|') or k.startswith('Q4|Q1|')}
    for k, v in q4.items():
        if k.endswith('m1') or k.endswith('m4'):
            print(k, v)


if __name__ == '__main__':
    main()
