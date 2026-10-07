#!/usr/bin/env python3
"""Piece de MATHEMATIQUES.md (v11) : enregistrements exacts du catalogue critique de petites fixtures, lus dans
l'etage constructif de la reference (hgp11_ref.Reference, regle d'admission unique p + q_min <= K + 1) : catalogue
par force brute sur tous les supports de 1 a 4 sites. Les boules de rayon nul (q_min = 1) ne sont pas affichees.

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B fixtures_catalogue.py [nom ...]
"""
import sys

sys.path.insert(0, '/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v11/reference')
from hgp11_ref import Reference  # noqa: E402

FIX = {
    'ligne_024': ([(0, 0, 0), (2, 0, 0), (4, 0, 0)], (1, 2)),
    'triangle_rectangle': ([(0, 0, 0), (3, 0, 0), (0, 4, 0)], (1, 2)),
    'triangle_rectangle_interieur': ([(0, 0, 0), (3, 0, 0), (0, 4, 0), (1, 1, 0)], (1, 2)),
    'triangle_aigu_interieur': ([(0, 0, 0), (6, 0, 0), (3, 5, 0), (3, 2, 0)], (2, 3)),
    'tetra_centre': ([(0, 0, 0), (2, 2, 0), (2, 0, 2), (0, 2, 2), (1, 1, 1)], (3, 4)),
    'tetra_face_obtuse': ([(0, 4, 4), (1, 2, 0), (1, 2, 4), (4, 4, 0)], (3,)),
    'carre': ([(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)], (1, 3)),
    'octaedre': ([(15, 10, 10), (5, 10, 10), (10, 15, 10), (10, 5, 10), (10, 10, 15), (10, 10, 5)], (1, 5)),
    'cube': ([(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)], (1, 7)),
    'circle25_pair': ([(15, 10, 3), (7, 14, 3), (7, 6, 3), (40, 40, 40), (50, 40, 40)], (2,)),
    'gain_de_couverture': ([(8, 9, 0), (5, 10, 0), (2, 9, 0), (5, 0, 0)], (3,)),
    'these_th5_plan': ([(0, 100, 0), (200, 100, 0), (101, 10, 0), (130, 15, 0), (103, 400, 0)], (2,)),
}


def show(name):
    pts, orders = FIX[name]
    print('== %s : %s' % (name, pts))
    for kmax in orders:
        ref = Reference(pts, kmax, admission='single')
        balls = [b for b in ref.balls if b.level > 0]
        print('  K = %d : %d boule(s) de rayon non nul' % (kmax, len(balls)))
        for b in balls:
            sup = [ref.sites[s] for s in b.support]
            inner = [ref.sites[s] for s in b.inner_sites]
            shell = [ref.sites[s] for s in b.shell_sites]
            print('    niveau %s ; centre (%s) ; q_min %d ; p %d ; m %d ; %s ; S* %s ; I %s ; U %s ; fenetre [%d, %d]'
                  % (b.level, ', '.join(str(c) for c in b.center), b.qmin, b.p, b.m,
                     'etendue' if b.extended else 'reguliere', sup, inner, shell, b.lo, b.hi))


def main():
    for name in (sys.argv[1:] or sorted(FIX)):
        show(name)
    return 0


if __name__ == '__main__':
    sys.exit(main())
