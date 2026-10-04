#!/usr/bin/env python3
"""Q-Pi2 au niveau de la sortie plate. Sur Q3 de base, a mcs 9, l'amas + x n'a aucun frere gros : c'est la chaine de
la racine et aucune tete ne sort de cluster (fixtures_existence.py). Pour que « x fait exister l'amas » change la sortie
plate, on prolonge le filament (f6..f9 au meme pas, 9 sites de filament) et on ajoute un groupe lointain Z de 9 sites.
Criteres A et C, EOM z = 1 et 3, a cote de HDBSCAN (scikit-learn et N-aire).

    python3 qpi2_plat.py > ../sorties/qpi2_plat.txt
"""
import numpy as np

import modele_lib as ml
import fixtures_existence as fx


def main():
    O = fx.O
    extra = {'f6': (4200, 2, 0), 'f7': (4901, -1, 0), 'f8': (5600, 3, 0), 'f9': (6301, 0, 0)}
    names = list(fx.Q3_NAMES) + list(extra)
    pts = fx.q3() + [tuple(a + b for a, b in zip(O, v)) for v in extra.values()]
    zoff = (30000, 10000, 10000)
    zrel = [(0, 0, 0), (150, 0, 0), (0, 150, 0), (150, 150, 0), (75, 260, 0), (75, -110, 0), (0, 0, 150),
            (150, 0, 150), (75, 75, 120)]
    names += ['z%d' % i for i in range(len(zrel))]
    pts += [tuple(a + b for a, b in zip(zoff, v)) for v in zrel]
    ph = ml.PointHierarchy(pts, 2, names=names)
    x = names.index('x')
    print('x : e=%.4f t=%.4f D=%.4f rho=%.4f d_k=%.4f' % (ph.e[x].approx(), ph.t[x].approx(), ph.D[x].approx(),
                                                       ph.rho[x].approx(), ph.d[x].approx()))
    for mcs in (8, 9):
        for cname, dates in (('A', list(ph.e)), ('C', list(ph.rho))):
            tg = ph.treegram(dates, cname)
            clusters, viol = ml.condensed_tree(tg, mcs)
            desc = [([names[i] for i in c['members']], round(ml.rfloat(tg.radii[c['birth']]), 2),
                     None if c['death'] is None else round(ml.rfloat(tg.radii[c['death']]), 2)) for c in clusters]
            print('mcs=%d %s : amas condenses %s' % (mcs, cname, desc))
            for z in (1, 3):
                sel, _ = ml.eom_select(tg, clusters, z)
                print('    EOM z=%d : %s' % (z, ml.partition_of(ml.labels_from_selection(tg, clusters, sel), names)))
        X = np.array(pts, dtype=float)
        print('    HDBSCAN sklearn : %s' % ml.partition_of(ml.hdbscan_labels(X, 2, mcs), names))
        U = ml.hdbscan_ultrametric(X, 2)
        tg = ml.Treegram(len(pts), U, [U[i][i] for i in range(len(pts))], names=names)
        clusters, _ = ml.condensed_tree(tg, mcs)
        for z in (1, 3):
            sel, _ = ml.eom_select(tg, clusters, z)
            print('    HDBSCAN N-aire z=%d : %s' % (z, ml.partition_of(ml.labels_from_selection(tg, clusters, sel),
                                                                     names)))


if __name__ == '__main__':
    main()
