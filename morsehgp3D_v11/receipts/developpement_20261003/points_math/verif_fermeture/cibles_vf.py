#!/usr/bin/env python3
"""Cibles Q1-Q4 de l'utilisateur (QUESTIONS_UTILISATEUR.md, v10) rejouees par vf_oracle : dendrogrammes des blocs
d'au moins deux sites pour fermeture m=1 et m=k+1, H_1, H_{k+1}, EC_{k+1}, first_{k+1}, cover, EC_1, core."""
import math
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo


def dendro(u, names):
    n = len(u)
    levels = sorted(set(u[i][j] for i in range(n) for j in range(n)))
    out, last = [], None
    for lev in levels:
        bl = [b for b in vo.blocks(u, lev) if len(b) >= 2]
        text = ' | '.join(''.join(names[i] for i in sorted(b)) for b in bl)
        if text != last:
            out.append('r=%.3f : %s' % (math.sqrt(lev), text or '-'))
            last = text
    return out


def show(title, pts, names, k):
    cl = vo.Cloud(pts)
    F = vo.Full(cl, k)
    print('\n## %s (n=%d, k=%d) ; racine FULL r=%.3f' % (title, len(pts), k, math.sqrt(F.levels_of_node[F.root])))
    rules = [('fermeture m=1', F.closure(1)[0]), ('fermeture m=%d' % (k + 1), F.closure(k + 1)[0]),
             ('H_1', F.ultra(F.hang_margin(1))), ('H_%d' % (k + 1), F.ultra(F.hang_margin(k + 1))),
             ('EC_%d' % (k + 1), F.ultra(F.hang_ec(k + 1))), ('first_%d' % (k + 1), F.ultra(F.hang_first(k + 1))),
             ('cover (first_1)', F.ultra(F.hang_first(1))), ('EC_1', F.ultra(F.hang_ec(1))),
             ('core', F.ultra(F.hang_core()))]
    for name, u in rules:
        print('  ' + name)
        for line in dendro(u, names):
            print('     ' + line)


q1 = [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3700, 2000, 0), (5432, 3000, 0), (5432, 1000, 0)]
show('Q1 pont 1700 ; reponse (b)', q1, 'ABCDEF', 2)
q2 = [(1000, 1000, 1000), (1100, 1000, 1000), (1010, 1120, 1000), (1010, 1119, 1016)]
show('Q2 ; reponse (a) x avec a (b=b1, c=b2)', q2, 'xabc', 2)
O = (10000, 10000, 10000)


def off(dx, dy, dz):
    return (O[0] + dx, O[1] + dy, O[2] + dz)


q3 = [off(0, 0, 0), off(700, 3, 0), off(1401, -2, 0), off(2100, 4, 0), off(2802, 0, 0), off(3500, -3, 0),
      off(-900, 0, 0), off(-880, 200, 0), off(-880, -200, 0), off(-880, 0, 200), off(-880, 0, -200),
      off(-1100, 0, 0), off(-1080, 150, 100), off(-1080, -150, -100)]
show('Q3 ; reponse (a) x dans le filament', q3, 'x12345abcdefgh', 2)
q4 = [(3000, 3000, 3000), (4000, 4000, 3000), (4000, 3000, 4000), (3000, 4000, 4000), (2600, 2600, 2600),
      (2200, 2200, 2200), (1200, 1200, 2200), (1200, 2200, 1200), (2200, 1200, 1200)]
show('Q4 ; reponse (a) attendre puis CmD (S,T,U = P2,Q2,R2)', q4, 'CPQRmDSTU', 3)
