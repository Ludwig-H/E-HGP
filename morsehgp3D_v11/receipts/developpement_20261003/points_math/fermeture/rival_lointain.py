#!/usr/bin/env python3
"""Fixture exacte : retard non local de H_m (marge en niveau carre) face a la fermeture et a EC.

    python3 rival_lointain.py > resultats/rival_lointain.txt

Nuage (0, 2s, 4s + D) sur un axe, k = 2, m = 1 (H_1, la regle 'margin1' du developpeur) et m = 3 (H_3 = 'margin').
Le site median est couvert au niveau s^2 par la paire de gauche ; la paire de droite ne le couvre qu'a
((2s + D)/2)^2 et les deux composantes fusionnent a ((4s + D)/2)^2. H_m le fait entrer a
t + max_q (m(p, q) - h(q)) = 4 s^2 + s D : le retard croit sans borne avec D, alors que le site est sans rival sur
[s, s + D/2] en rayon. Pour D = 0 (symetrie), toute regle fidele equivariante doit attendre la fusion (P4).
"""
from fractions import Fraction
import math
import sys

sys.dont_write_bytecode = True
import lib_fermeture as lf  # noqa: E402


def main():
    print('# rival lointain : nuage (0, 2s, 4s + D), k = 2, site median = 1 ; rayons')
    for s in (1, 1000):
        for D in (0, 1, 10, 96) if s == 1 else (0, 1, 1000, 96000):
            pts = [(0, 0, 0), (2 * s, 0, 0), (4 * s + D, 0, 0)]
            defn = lf.Definition(pts)
            res = defn.order(2)
            ref1, tree = lf.faithful_rules(res, 3, 1)
            cl = lf.closure(res, 3, 1)
            ec = lf.ec_hanging(res, 3, 1)
            e_h1 = ref1['margin'][1][0]
            e_first = ref1['cover'][1][0]
            root = max(nd.level for nd in res.nodes)
            rival = Fraction((2 * s + D) ** 2, 4)
            formula = Fraction(4 * s * s + s * D)
            ok = e_h1 == (formula if D > 0 else Fraction(4 * s * s))
            print('s=%d D=%d : premiere couverture r=%.3f ; rival des r=%.3f ; fusion FULL r=%.3f | entree du median :'
                  ' H_1 r=%.3f (niveau %s, formule 4s^2+sD %s) ; LCA r=%.3f ; EC r=%.3f ; fermeture r=%.3f ; '
                  'retard H_1/premiere couverture en rayon x%.2f'
                  % (s, D, math.sqrt(s * s), math.sqrt(rival), math.sqrt(root), math.sqrt(e_h1), e_h1,
                     'OK' if ok else 'ECART', math.sqrt(e_first), math.sqrt(ec[1][0]), math.sqrt(cl['u'][1][1]),
                     math.sqrt(e_h1) / s))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
