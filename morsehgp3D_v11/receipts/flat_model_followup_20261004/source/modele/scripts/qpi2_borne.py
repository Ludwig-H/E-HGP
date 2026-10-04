#!/usr/bin/env python3
"""Borne exacte de la proposition « Q-Pi2 contre stabilite » (rapport, § 3.4), cadre intrinsèque des profils.

Pour le site x de Q3 (et ses variantes), on lit dans H^r_3 exacte : alpha = premiere couverture qualifiee, la barre
rivale qualifiee (c, M) (naissance, rencontre de la lignee de x). Une regle de dates de comptage locale au profil
qualifie, a comptage immediat sans rival (s = alpha si aucune barre rivale) et L-stable pour l'entrelacement des
profils, verifie s(x) <= alpha + L (M - c) / 2 (la barre rivale s'efface par un entrelacement de (M - c) / 2).
Q-Pi2 exige s(x) > 790 (aucun gros bloc a mcs 9 sur la fenetre fermee [705 ; 790]). D'ou L > (790 - alpha) / ((M - c)/2).

On rend aussi les dates P_kappa (kappa >= 1, la famille stable de la v10) et le kappa maximal compatible avec Q-Pi2.

    python3 qpi2_borne.py > ../sorties/qpi2_borne.txt
"""
from fractions import Fraction

import modele_lib as ml
from modele_lib import Rad, rcmp
import fixtures_existence as fx


def main():
    for vname, dep in (('base', None), ('x_vers_amas', {'x': (-1, 0, 0)}), ('x_vers_filament', {'x': (1, 0, 0)}),
                       ('c0_vers_x', {'c0': (1, 0, 0)}), ('f1_vers_x', {'f1': (-1, 0, 0)})):
        ph = ml.PointHierarchy(fx.q3(dep), 2, names=fx.Q3_NAMES)
        x = ph.names.index('x')
        alpha = ph.t[x]
        bars = ph.rival_bars[x]
        print('Q3_%s : alpha = %s = %.6f ; e = %.6f ; rho = %.6f ; barres rivales (c, M) = %s' % (
            vname, alpha, alpha.approx(), ph.e[x].approx(), ph.rho[x].approx(),
            [(round(float(c) ** 0.5, 6), round(float(M) ** 0.5, 6)) for c, M in bars]))
        # barre de persistance maximale (celle qui fixe D)
        best = max(bars, key=lambda b: (Rad.sqrt(b[1]) - Rad.sqrt(b[0])).approx())
        c, M = Rad.sqrt(best[0]), Rad.sqrt(best[1])
        delta = (M - c).scale(Fraction(1, 2))
        end = Rad.rat(Fraction(790))
        need = end - alpha
        print('   delta = (M - c)/2 = %.6f ; 790 - alpha = %.6f ; L_min = %.4f (strict)' % (
            delta.approx(), need.approx(), need.approx() / delta.approx()))
        # verification exacte : L = 7 ne suffit pas (alpha + 7 delta <= 790), L = 8 laisse la place
        for L in (3, 5, 7, 8):
            bound = alpha + delta.scale(L)
            print('   L = %d : alpha + L delta = %.6f  %s 790' % (L, bound.approx(),
                                                              '<=' if rcmp(bound, end) <= 0 else '>'))
        for kappa in (Fraction(1), Fraction(2), Fraction(4), Fraction(1, 2), Fraction(1, 10), Fraction(1, 50)):
            date = ml.rmax(alpha, M - (c - alpha).scale(kappa))
            print('   P_kappa, kappa = %s : %.6f  (Q-Pi2 %s)' % (kappa, date.approx(),
                                                              'tenu' if rcmp(date, end) > 0 else 'perdu'))
        kmax = (M - end).approx() / (c - alpha).approx()
        print('   kappa maximal compatible avec Q-Pi2 : %.6f (P_kappa n est stable que pour kappa >= 1)' % kmax)


if __name__ == '__main__':
    main()
