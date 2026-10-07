#!/usr/bin/env python3
"""Piece de MATHEMATIQUES.md (v11) : valeurs exactes des fixtures, lues dans l'oracle de definition (etage A de
hgp11_ref : graphe Gamma_k exhaustif en fractions). Le script n'etablit rien d'autre que ces valeurs.

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B fixtures_oracle.py [nom ...]
"""
import sys

sys.path.insert(0, '/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v11/reference')
from hgp11_ref import Definition  # noqa: E402
from hgp11_ref.model import members  # noqa: E402


def line(xs):
    return [(x, 0, 0) for x in xs]


FIX = {
    # nom : (points, ordres)
    'ligne_024': (line((0, 2, 4)), (1, 2, 3)),
    'f2_gauche': (line((0, 999, 2000)), (2,)),
    'f2_droite': (line((0, 1001, 2000)), (2,)),
    'ligne_01269': (line((0, 1, 2, 6, 9)), (2, 3)),
    'ligne_f3': (line((0, 20, 22, 50, 52)), (1, 2)),
    'firstcov_k3_n6': ([(2, 4, 4), (2, 8, 5), (2, 9, 1), (3, 7, 0), (6, 9, 6), (8, 10, 7)], (3,)),
    'internal_k3': ([(15, 4, 0), (5, 4, 0), (7, 8, 0), (7, 0, 0), (1, 4, 0), (0, 4, 1)], (3,)),
    'gain_de_couverture': ([(8, 9, 0), (5, 10, 0), (2, 9, 0), (5, 0, 0)], (1, 2, 3, 4)),
    'ligne_012': (line((0, 1, 2)), (2, 3)),
    'stab_x': (line((1, 11)), (2,)),
    'stab_y': (line((0, 12)), (2,)),
    'carre': ([(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)], (1, 2, 3, 4)),
    'triangle_rectangle': ([(0, 0, 0), (3, 0, 0), (0, 4, 0)], (1, 2, 3)),
    'these_th5_plan': ([(0, 100, 0), (200, 100, 0), (101, 10, 0), (130, 15, 0), (103, 400, 0)], (1, 2)),
    'deux_triangles': ([(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (4000, 2000, 0), (5732, 3000, 0),
                        (5732, 1000, 0)], (2,)),
    'paire_31': ([(12, 12, 12)] * 3 + [(14, 12, 12)], (1, 2, 3, 4)),
    'triangle_311': ([(0, 0, 0)] * 3 + [(6, 0, 0), (3, 5, 0)], (1, 2, 3, 4, 5)),
    'triangle_211': ([(0, 0, 0)] * 2 + [(6, 0, 0), (3, 5, 0)], (1, 2, 3, 4)),
}


def show(name):
    pts, orders = FIX[name]
    oracle = Definition(pts)
    print('== %s : %s' % (name, pts))
    for k in orders:
        res = oracle.order(k)
        print('  ordre %d : %d noeuds' % (k, len(res.nodes)))
        for v, node in enumerate(res.nodes):
            kind = 'naissance' if not node.children else 'fusion %s' % (list(node.children),)
            low = '' if res.lower is None else ' ; image verticale %d' % res.lower[v]
            ctr = '' if node.center is None else ' ; centre (%s)' % ', '.join(str(c) for c in node.center)
            print('    noeud %d : niveau %s ; %s%s%s' % (v, node.level, kind, ctr, low))
        print('    core  : ' + ' ; '.join('x%d : %s -> noeud %d' % (x, e.level, e.nodes)
                                          for x, e in enumerate(res.core)))
        print('    cover : ' + ' ; '.join('x%d : %s -> %s' % (x, e.level, sorted(e.nodes))
                                          for x, e in enumerate(res.cover)))
        for cut in res.cuts:
            shut = ', '.join('%d:couv%s/coeur%s' % (v, members(cov), members(core)) for v, cov, core in cut.closed)
            opened = ', '.join('%d:couv%s/coeur%s' % (v, members(cov), members(core)) for v, cov, core in cut.opened)
            print('    coupe %s : ouverte [%s] ; fermee [%s]' % (cut.level, opened, shut))


def main():
    names = sys.argv[1:] or sorted(FIX)
    for name in names:
        show(name)
    return 0


if __name__ == '__main__':
    sys.exit(main())
