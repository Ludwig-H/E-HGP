#!/usr/bin/env python3
"""C7 : la qualification et la garantie avant F (proposition I du rapport ; son § 0 point 7 et § 7 point 3 disent
« sans objet pour la qualification : ses retards portent sur des groupes d'au plus k sites, hors de la composante
geante »).

Contre-exemple cherche : site x coeur de sa composante a F (d_k <= F, alpha + d_k/2 <= F), composante qualifiee
(>= k+1 sites) qui contient x des t' < F, et un rival QUALIFIE qui couvre x a c >= t' et ne rejoint la lignee de x
qu'apres F : P1 o Pi_{k+1} (H_{k+1} en rayon) n'entre pas x a F, alors que P1 o Pi_1 l'y place.
Deux routes : vfull (independant) et l'oracle du depot (margin = Q1 o Pi_m en niveau ; reference_radius_rules).

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B c7_qualif_F.py
"""
import json
import sys
from fractions import Fraction

sys.dont_write_bytecode = True
BENCH = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench'
REF = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/reference'
sys.path.insert(0, REF)
sys.path.insert(0, BENCH)
import points_reference as pr  # noqa: E402
from hgp11_ref import Definition  # noqa: E402
import mpmath  # noqa: E402
import vfull as V  # noqa: E402

CASES = {
    'minimal': [(0, 0, 0), (2, 0, 0), (4, 0, 0), (-4, 2, 0), (-4, -2, 0)],
    'amas_droit_6': [(0, 0, 0), (2, 0, 0), (4, 0, 0), (6, 0, 0), (4, 2, 0), (4, -2, 0), (-4, 2, 0), (-4, -2, 0)],
}
NAMES = {'minimal': ['x', 'y', 's2', 'w1', 'w2'],
         'amas_droit_6': ['x', 'y', 's2', 's3', 's4', 's5', 'w1', 'w2']}


def main():
    out = {}
    for name, pts in CASES.items():
        names = NAMES[name]
        n, k = len(pts), 2
        full = V.Full(V.Cloud(pts), k)
        x = 0
        p1, p1q, q1q = V.rule_P(full, 1, 1), V.rule_P(full, 1, k + 1), V.rule_Q(full, 1, k + 1)
        dk = V.R(full.cloud.dk2(x, k))
        alpha = V.R(min(a for a, _ in full.entries(x, 1)))
        tq = V.R(min(a for a, _ in full.entries(x, k + 1)))
        F = mpmath.mpf('2.5')
        F2 = Fraction(25, 4)
        F0 = full.cloud.knn_part(x, k)
        comp = full.comp_at(F0, F2)
        comp_sites = [names[i] for i in range(n) if full.mask[comp] >> i & 1]
        rec = {'d_k': float(dk), 'alpha': float(alpha), 'alpha+d_k/2': float(alpha + dk / 2),
               'premiere_couverture_qualifiee': float(tq), 'F': float(F),
               'sites_de_la_composante_de_x_a_F': comp_sites,
               'P1_raw_date_x': float(p1.e[x]),
               'P1_raw_x_place_a_F': bool(p1.e[x] <= F and full.comp_at(p1.anchor[x], F2) == comp),
               'H3_rayon_date_x': float(p1q.e[x]), 'H3_niveau_date_x': float(q1q.e[x]),
               'blocs_P1_raw_a_F': [[names[i] for i in sorted(b)] for b in p1.blocks(F)],
               'blocs_H3_rayon_a_F': [[names[i] for i in sorted(b)] for b in p1q.blocks(F)],
               'blocs_H3_niveau_a_F': [[names[i] for i in sorted(b)] for b in q1q.blocks(F)]}
        # oracle du depot : regle margin (Q1 o Pi_3, niveau) et version rayon du developpeur
        res = Definition(pts).order(k)
        ref, tree = pr.reference_rules(res, n, k + 1)
        rad, _ = pr.reference_radius_rules(res, n, k + 1)
        rec['oracle_margin_date_x_niveau'] = str(ref['margin'][x][0])
        rec['oracle_margin_date_x_rayon'] = float(mpmath.sqrt(mpmath.mpf(ref['margin'][x][0].numerator) /
                                                              ref['margin'][x][0].denominator))
        rec['oracle_margin_r_date_x'] = float(rad['margin_r'][x][0])
        rec['oracle_margin_r1_date_x'] = float(rad['margin_r1'][x][0])
        u = pr.reference_ultrametric(ref['margin'], tree)
        rec['oracle_margin_blocs_a_F2'] = [[names[i] for i in sorted(b)] for b in pr.blocks_at(u, F2)]
        out[name] = rec
    print(json.dumps(out, indent=1))
    with open('recus_c7_qualif_F.json', 'w') as fh:
        json.dump(out, fh, indent=1)


if __name__ == '__main__':
    main()
