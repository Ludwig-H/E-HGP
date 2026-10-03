#!/usr/bin/env python3
"""Juge explicite des cibles Q1-Q4 sur les FENETRES posees a l'utilisateur (QUESTIONS_UTILISATEUR.md) et sur les
cellules ancrees de CIBLES_REVISEES.md (§ 2.2-2.6), pour chaque regle (vf_oracle).

Deux lectures :
  stricte  : la hierarchie de l'option choisie doit tenir sur toute la fenetre de la question (comme le juge v10, ou
             un retard de 4,5 % comptait comme un echec) ;
  laxiste  : celle du rapport (la structure finale avant la fusion suffit, attente permise).
Fenetres (rayons) : Q1 [1154,684 ; 1787,360[ avec ABC | DEF exiges sur [1697,992 ; 1787,360[ (19/20 f) ;
Q2 [61 ; 75] ; Q3 [705 ; 790] ; Q4 [866,03 ; 1334,166[ avec CmD exigee sur [1267,458 ; 1334,166[.
Q4 lecture litterale (a) contre (c) : en (a), la chaine CmD n'existe pas encore a 866,03.
"""
import math
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo
from fractions import Fraction


def sq(x):
    return Fraction(x) ** 2


def blocks_named(u, lev, names):
    return set(''.join(names[i] for i in sorted(b)) for b in vo.blocks(u, lev) if len(b) >= 2)


def eval_levels(u, lo2, hi2, closed_hi=False):
    """Niveaux de controle dans [lo2, hi2[ (ou ]) : lo2 et chaque niveau de changement interieur."""
    n = len(u)
    levs = sorted(set([lo2] + [u[i][j] for i in range(n) for j in range(n)
                              if lo2 < u[i][j] < hi2 or (closed_hi and u[i][j] == hi2)]))
    return levs


def judge(name_q, u, names):
    if name_q == 'Q1':
        lo, mid, hi = sq('1154.684'), Fraction(3194656) * Fraction(361, 400), Fraction(3194656)
        ok_late = all(blocks_named(u, a, names) == {'ABC', 'DEF'} for a in eval_levels(u, mid, hi))
        ok_early = True
        for a in eval_levels(u, lo, hi):
            for b in blocks_named(u, a, names):
                if ('C' in b and 'D' in b) or (set(b) & set('ABC') and set(b) & set('DEF')):
                    ok_early = False
        strict = ok_late and ok_early
        lax = strict
        return strict, lax, strict
    if name_q == 'Q2':
        levs = eval_levels(u, Fraction(61 * 61), Fraction(75 * 75), closed_hi=True)
        strict = all(blocks_named(u, a, names) == {'xa', 'bc'} for a in levs)
        last = blocks_named(u, Fraction(75 * 75), names)
        lax = last == {'xa', 'bc'}
        return strict, lax, strict
    if name_q == 'Q3':
        levs = eval_levels(u, Fraction(705 * 705), Fraction(790 * 790), closed_hi=True)
        strict = all(blocks_named(u, a, names) == {'x12345', 'abcdefgh'} for a in levs)
        lax = blocks_named(u, Fraction(790 * 790), names) == {'x12345', 'abcdefgh'}
        return strict, lax, strict
    if name_q == 'Q4':
        lo, hi = Fraction(750000), Fraction(1780000)
        mid = hi * Fraction(361, 400)
        ok = True
        for a in eval_levels(u, lo, mid):
            bl = blocks_named(u, a, names)
            if not ({'PQR', 'STU'} <= bl and bl <= {'PQR', 'STU', 'CmD'}):
                ok = False
        late = all(blocks_named(u, a, names) == {'CmD', 'PQR', 'STU'} for a in eval_levels(u, mid, hi))
        anchored = ok and late
        literal_a = anchored and 'CmD' not in blocks_named(u, lo, names)
        bl_end = blocks_named(u, hi - 1, names)
        lax = bl_end == {'CmD', 'PQR', 'STU'} and 'CmD' not in blocks_named(u, lo, names)
        return literal_a, lax, anchored
    raise ValueError


def rules(F, k):
    out = [('fermeture m=1', F.closure(1)[0]), ('fermeture m=k+1', F.closure(k + 1)[0]),
           ('H_1', F.ultra(F.hang_margin(1))), ('H_k+1', F.ultra(F.hang_margin(k + 1))),
           ('first_1 (cover)', F.ultra(F.hang_first(1))), ('first_k+1', F.ultra(F.hang_first(k + 1))),
           ('EC_1', F.ultra(F.hang_ec(1))), ('EC_k+1', F.ultra(F.hang_ec(k + 1))), ('core', F.ultra(F.hang_core()))]
    return out


O = (10000, 10000, 10000)


def off(dx, dy, dz):
    return (O[0] + dx, O[1] + dy, O[2] + dz)


Q = [('Q1', [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3700, 2000, 0), (5432, 3000, 0), (5432, 1000, 0)],
      'ABCDEF', 2),
     ('Q2', [(1000, 1000, 1000), (1100, 1000, 1000), (1010, 1120, 1000), (1010, 1119, 1016)], 'xabc', 2),
     ('Q3', [off(0, 0, 0), off(700, 3, 0), off(1401, -2, 0), off(2100, 4, 0), off(2802, 0, 0), off(3500, -3, 0),
             off(-900, 0, 0), off(-880, 200, 0), off(-880, -200, 0), off(-880, 0, 200), off(-880, 0, -200),
             off(-1100, 0, 0), off(-1080, 150, 100), off(-1080, -150, -100)], 'x12345abcdefgh', 2),
     ('Q4', [(3000, 3000, 3000), (4000, 4000, 3000), (4000, 3000, 4000), (3000, 4000, 4000), (2600, 2600, 2600),
             (2200, 2200, 2200), (1200, 1200, 2200), (1200, 2200, 1200), (2200, 1200, 1200)], 'CPQRmDSTU', 3)]
table = {}
for qn, pts, names, k in Q:
    F = vo.Full(vo.Cloud(pts), k)
    for rn, u in rules(F, k):
        table.setdefault(rn, {})[qn] = judge(qn, u, names)
print('%-18s %-28s %-28s %-28s' % ('regle', 'stricte (fenetre)', 'laxiste (rapport)', 'cellule ancree v10'))
for rn in table:
    s = ' '.join('%s:%s' % (q, 'oui' if table[rn][q][0] else 'non') for q in ('Q1', 'Q2', 'Q3', 'Q4'))
    l = ' '.join('%s:%s' % (q, 'oui' if table[rn][q][1] else 'non') for q in ('Q1', 'Q2', 'Q3', 'Q4'))
    c = ' '.join('%s:%s' % (q, 'oui' if table[rn][q][2] else 'non') for q in ('Q1', 'Q2', 'Q3', 'Q4'))
    print('%-18s %-28s %-28s %-28s' % (rn, s, l, c))
