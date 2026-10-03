#!/usr/bin/env python3
"""Rejoue les fixtures de qualified_proof/README.md par une route independante de son check.py.

Route : oracle de la definition (reference/hgp11_ref, etage A) -> coupes fermees de Gamma_k -> fermeture des
couvertures qualifiees (lib_fermeture.closure). Le check.py de l'auditeur n'est pas importe.
Sortie : texte sur stdout (copie dans resultats/fixtures_auditeur.txt par l'appelant).
"""
from fractions import Fraction
import sys

sys.dont_write_bytecode = True
import lib_fermeture as lf  # noqa: E402


def tri(d):
    return [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (2000 + d, 2000, 0), (3732 + d, 3000, 0),
            (3732 + d, 1000, 0)]


def show_blocks(u, level):
    return [sorted(b) for b in lf.pr.blocks_at(u, level)]


def root_level(res):
    return max(nd.level for nd in res.nodes)


def main():
    ok_all = True
    print('# Fixtures de l auditeur rejouees par l oracle de la definition (route independante)')
    for bridge, expect_merge in ((2000, Fraction(3731956)), (1998, Fraction(3728225)), (1700, Fraction(3194656))):
        pts = tri(bridge)
        defn = lf.Definition(pts)
        res = defn.order(2)
        c3 = lf.closure(res, 6, 3)
        c2 = lf.closure(res, 6, 2)
        a = Fraction(249978000484, 187489)
        b3 = show_blocks(c3['u'], a)
        merge3 = c3['u'][0][3]
        root2 = max(c2['u'][i][j] for i in range(6) for j in range(6))
        ok = (b3 == [[0, 1, 2], [3, 4, 5]] and all(e == a for e in c3['entries']) and merge3 == expect_merge
              and root_level(res) == expect_merge)
        ok_all &= ok
        print('triangles pont %d : m=3 blocs a beta=%s -> %s ; entrees %s ; reunion ABC|DEF a %s ; fusion FULL %s ;'
              ' m=2 racine %s ; %s' % (bridge, a, b3, sorted(set(str(e) for e in c3['entries'])), merge3,
                                     root_level(res), root2, 'OK' if ok else 'ECART'))
    pts = [(x, 0, 0) for x in (0, 2, 100, 102)]
    res = lf.Definition(pts).order(2)
    c3 = lf.closure(res, 4, 3)
    c2 = lf.closure(res, 4, 2)
    ok = (c3['entries'] == [Fraction(2500)] * 4 and c2['u'][0][2] == 2401 and root_level(res) == 2500
          and show_blocks(c2['u'], Fraction(1)) == [[0, 1], [2, 3]])
    ok_all &= ok
    print('0,2,100,102 : m=3 entrees %s ; m=2 reunion des paires a %s ; fusion FULL %s ; %s'
          % ([str(e) for e in c3['entries']], c2['u'][0][2], root_level(res), 'OK' if ok else 'ECART'))
    pts = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
    defn = lf.Definition(pts)
    res = defn.order(2)
    c3 = lf.closure(res, 5, 3)
    ref, tree = lf.faithful_rules(res, 5, 3)
    um = lf.ultrametric_of(ref['margin'], tree)
    ok = (c3['u'][1][3] == Fraction(100, 9) and root_level(res) == 36 and res.core[0].level == 40
          and show_blocks(um, Fraction(35)) == [[1, 2], [3, 4]] and um[0][0] == 36)
    ok_all &= ok
    print('cinq points : fermeture m=3 reunit {1,2} et {3,4} a %s ; fusion FULL %s ; core du point 0 a %s ;'
          ' H_3 a 35 : %s, entree du point 0 %s ; %s' % (c3['u'][1][3], root_level(res), res.core[0].level,
                                                        show_blocks(um, Fraction(35)), um[0][0],
                                                        'OK' if ok else 'ECART'))
    print('verdict_fixtures_auditeur %s' % ('conforme' if ok_all else 'ECART'))
    return 0 if ok_all else 1


if __name__ == '__main__':
    raise SystemExit(main())
