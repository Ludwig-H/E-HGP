#!/usr/bin/env python3
"""Fixtures de l'auditeur (coordonnees relues dans son check.py), rejouees par vf_oracle."""
import math
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo
from fractions import Fraction


def triangles(d):
    return [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (2000 + d, 2000, 0), (3732 + d, 3000, 0), (3732 + d, 1000, 0)]


for d in (2000, 1998, 1700):
    F = vo.Full(vo.Cloud(triangles(d)), 2)
    u3, _ = F.closure(3)
    u2, _ = F.closure(2)
    a = vo.Cloud(triangles(d)).beta((0, 1, 2))
    print('pont %d : triangle beta %s ; m=3 entrees %s ; ABC|DEF a beta=a ? %s ; reunion m=3 beta %s ; racine FULL %s ;'
          ' m=2 racine beta %s' % (d, a, sorted(set(u3[i][i] for i in range(6))),
                                   [sorted(b) for b in vo.blocks(u3, a)], u3[0][5], F.levels_of_node[F.root],
                                   max(max(r) for r in u2)))
F = vo.Full(vo.Cloud([(0, 0, 0), (2, 0, 0), (100, 0, 0), (102, 0, 0)]), 2)
u3, _ = F.closure(3)
u2, _ = F.closure(2)
print('{0,2,100,102} : m=3 entrees %s ; m=2 reunion %s ; racine FULL %s' % (
    [u3[i][i] for i in range(4)], u2[0][3], F.levels_of_node[F.root]))
X = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
F = vo.Full(vo.Cloud(X), 2)
u3, _ = F.closure(3)
core = F.hang_core()
h3 = F.hang_margin(3)
uh = F.ultra(h3)
print('cinq points : m=3 reunion %s ; racine FULL %s ; core du point 0 %s ; H_3 entree du point 0 %s ;'
      ' blocs H_3 a 35 %s' % (u3[0][4], F.levels_of_node[F.root], core[0][0], h3[0][0],
                              [sorted(b) for b in vo.blocks(uh, Fraction(35))]))
