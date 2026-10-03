#!/usr/bin/env python3
"""Fixture : un rival a grande echelle retarde un site d'un petit amas (marge en niveau carre contre marge en rayon).

Nuage K = 2 : petit amas y, x (distance 2) ; amas lointain Z autour de z a distance 2 s de x, de l'autre cote de y.
Oracle v11 (route A) pour H_1 et H_3 ; route B en rayon pour comparaison. Sortie : date d'entree de x et de y."""
import sys
sys.dont_write_bytecode = True
import adaptateur as AD  # noqa: E402
CL = AD.CL

for s in (10, 100, 1000, 10000):
    P = [(0, 0, 0), (-2, 0, 0), (2 * s, 0, 0), (2 * s + 2, 0, 0), (2 * s + 1, 2, 0)]
    noms = ['x', 'y', 'z', 'w1', 'w2']
    for m in (1, 3):
        rules, _r, _t = AD.route_a(P, 2, m)
        ent = rules['margin1' if m == 1 else 'margin'][0]
        T, info = CL.full(P, 2)
        entb, _U = AD.route_b(T, m, 'r')
        print('s=%-6d m=%d  H_m carre : x entre a r=%.3f, y a r=%.3f | marge en rayon : x a r=%.3f, y a r=%.3f' % (
            s, m, float(ent[0][0]) ** 0.5, float(ent[1][0]) ** 0.5, float(entb[0][0]), float(entb[1][0])))
