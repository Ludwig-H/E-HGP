#!/usr/bin/env python3
"""Completion des sites restes seuls apres selection (rapport, § 5) : aucune, par lignee, ultrametrique.

Fixtures : cinq points de l'auditeur (le site 0 equidistant : Can) et variantes ou 0 penche vers {1, 2} ; deux
triangles avec un site de bord retarde ; Q3 a mcs 6 avec un groupe lointain Z (le filament de 5 sites reste petit).

    python3 completion.py > ../sorties/completion.txt
"""
import modele_lib as ml
import fixtures_existence as fx


def show(name, points, names, k, mcs, z=3):
    ph = ml.PointHierarchy(points, k, names=names)
    tg = ph.treegram()
    clusters, _ = ml.condensed_tree(tg, mcs)
    sel, _ = ml.eom_select(tg, clusters, z)
    lab = ml.labels_from_selection(tg, clusters, sel)
    U = [[ph.u(i, j) for j in range(ph.n)] for i in range(ph.n)]
    lin = ml.complete_lineage(ph, tg, clusters, sel, lab)
    ult = ml.complete_ultrametric(U, clusters, sel, lab)
    print('%s (k=%d, mcs=%d, EOM z=%d)' % (name, k, mcs, z))
    noise = [names[i] for i in range(ph.n) if lab[i] == -1]
    print('   bruit avant completion : %s' % noise)
    for i in range(ph.n):
        if lab[i] == -1:
            t, nodes = ml.lowest_qualified_nodes(ph, i)
            print('     %s : e=%.4f t=%.4f D=%.4f, points qualifies les plus bas : %d' % (
                names[i], ph.e[i].approx(), t.approx(), ph.D[i].approx(), len(nodes)))
    print('   aucune       : %s' % ml.partition_of(lab, names))
    print('   lignee       : %s' % ml.partition_of(lin, names))
    print('   ultrametrique: %s' % ml.partition_of(ult, names))
    import numpy as np
    X = np.array(points, dtype=float)
    print('   HDBSCAN sklearn (min_samples=%d, mcs=%d) : %s' % (k, mcs, ml.partition_of(ml.hdbscan_labels(X, k, mcs), names)))


def main():
    show('cinq points (0 equidistant)', fx.FIVE, ['0', '1', '2', '3', '4'], 2, 2)
    show('cinq points, 0 en (5, 2, 0)', [(5, 2, 0)] + fx.FIVE[1:], ['0', '1', '2', '3', '4'], 2, 2)
    # R1 + b3 : le groupe B = {b1, b2, b3} est un amas avant la fusion parasite F = 12 ; x (coeur de l'amas) entre
    # apres F, au-dessus des deux amas morts : bruit apres selection {C, B}
    r1b = fx.EIGHT + [(74, 30, 0)]
    show('R1 + b3', r1b, fx.EIGHT_NAMES + ['b3'], 2, 3)
    # Q3 + groupe lointain Z (6 sites) : a mcs 6 le filament (5) est petit
    z = [(30000 + dx, 10000 + dy, 10000) for dx, dy in ((0, 0), (150, 0), (0, 150), (150, 150), (75, 260), (75, -110))]
    show('Q3 + Z', fx.q3() + z, fx.Q3_NAMES + ['z%d' % i for i in range(6)], 2, 6)


if __name__ == '__main__':
    main()
