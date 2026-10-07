#!/usr/bin/env python3
"""Fixture des deux triangles (these, 6.1) et variantes entieres : FULL_2, amas discrets, cover, core, HDBSCAN."""
import oracle_l03 as O
from fractions import Fraction as Fr

names = 'ABCDEF'
FIX = {
    'P1 (pont 2000)': [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (4000, 2000, 0), (5732, 3000, 0), (5732, 1000, 0)],
    'P2 (pont 1998)': [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3998, 2000, 0), (5730, 3000, 0), (5730, 1000, 0)],
    'T1_1700 (pont 1700)': [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3700, 2000, 0), (5432, 3000, 0), (5432, 1000, 0)],
}
for nom, P in FIX.items():
    print('=' * 100)
    print(nom)
    T = O.Tour(P, 2)
    print('-- FULL_2 : composantes de Gamma_2 (amas discrets, ensembles de sites) aux niveaux critiques')
    last = None
    for a in T.levels:
        am = T.amas_discrets(a)
        s = ' | '.join(''.join(names[i] for i in sorted(c)) for c in am)
        if s != last:
            print('  r = %10.4f  r^2 = %-22s : %s' % (O.r(a), a, s))
            last = s
    core = O.core_projection(T)
    print('-- entrees core d_2(x) :', {names[x]: round(O.r(core[x][0]), 3) for x in range(T.n)})
    print('-- entrees cover alpha_2(x), composantes a egalite :')
    for x in range(T.n):
        a, comps = T.cover_tie_components(x)
        print('    %s : alpha = %.4f ; premieres couvertures : %s ; composantes distinctes a alpha : %d' % (
            names[x], O.r(a), [''.join(names[i] for i in F) for F in T.cover_entries(x)[1]], len(comps)))
    for choix in ('min', 'max'):
        cov = O.cover_projection(T, choix)
        print('-- hierarchie cover (egalites tranchees par la %s K-partie), blocs >= 2 :' % ('plus petite' if choix == 'min' else 'plus grande'))
        print(O.fmt_suite(T.suite(cov, 2), names))
    print('-- hierarchie core, blocs >= 2 :')
    print(O.fmt_suite(T.suite(core, 2), names))
    print('-- HDBSCAN (atteignabilite mutuelle, min_samples = 2 point compte), convention de la these (niveau = distance / 2), blocs >= 2 :')
    print(O.fmt_suite(O.hdbscan_suite(P, 2), names))
