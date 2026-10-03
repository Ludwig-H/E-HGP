#!/usr/bin/env python3
"""C7b : variante ou F est une vraie fusion (la composante de x rejoint une paire de fond a F = 12).

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B c7b_fusion_parasite.py
Sites (K = 2) : x, y, s2, s3 (amas droit), b1, b2 (fond, fusionne avec l'amas a F), w1, w2 (rival a gauche).
"""
import json
import sys
from fractions import Fraction

sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/reference')
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench')
import points_reference as pr  # noqa: E402
from hgp11_ref import Definition  # noqa: E402
import mpmath  # noqa: E402
import vfull as V  # noqa: E402

PTS = [(0, 0, 0), (10, 0, 0), (20, 0, 0), (30, 0, 0), (44, 0, 0), (54, 0, 0), (-20, 10, 0), (-20, -10, 0)]
NAMES = ['x', 'y', 's2', 's3', 'b1', 'b2', 'w1', 'w2']


def main():
    n, k, x = len(PTS), 2, 0
    full = V.Full(V.Cloud(PTS), k)
    res = Definition(PTS).order(k)
    nodes = [(str(nd.level), float(mpmath.sqrt(mpmath.mpf(nd.level.numerator) / nd.level.denominator)),
              list(nd.children)) for nd in res.nodes]
    F2 = Fraction(144)
    F = mpmath.mpf(12)
    out = {'noeuds_oracle(niveau, rayon, enfants)': nodes}
    # composantes a la coupe ouverte juste sous F et a F (oracle)
    for cut in res.cuts:
        if cut.level == F2:
            out['coupe_F_ouverte'] = [[NAMES[i] for i in range(n) if cov >> i & 1] for _v, cov, _c in cut.opened]
            out['coupe_F_fermee'] = [[NAMES[i] for i in range(n) if cov >> i & 1] for _v, cov, _c in cut.closed]
    p1, h3r, h3 = V.rule_P(full, 1, 1), V.rule_P(full, 1, 3), V.rule_Q(full, 1, 3)
    out['x'] = {'d_k': float(V.R(full.cloud.dk2(x, k))), 'alpha': float(V.R(min(a for a, _ in full.entries(x, 1)))),
                'premiere_couverture_qualifiee': float(V.R(min(a for a, _ in full.entries(x, 3)))),
                'P1_raw': float(p1.e[x]), 'H3_rayon': float(h3r.e[x]), 'H3_niveau': float(h3.e[x])}
    for nm, r in (('P1_raw', p1), ('H3_rayon', h3r), ('H3_niveau', h3)):
        out['blocs_%s_a_F' % nm] = [[NAMES[i] for i in sorted(b)] for b in r.blocks(F)]
    ref, tree = pr.reference_rules(res, n, 3)
    u = pr.reference_ultrametric(ref['margin'], tree)
    out['oracle_margin_date_x'] = float(mpmath.sqrt(mpmath.mpf(ref['margin'][x][0].numerator) /
                                                    ref['margin'][x][0].denominator))
    out['oracle_margin_blocs_a_F'] = [[NAMES[i] for i in sorted(b)] for b in pr.blocks_at(u, F2)]
    u1 = pr.reference_ultrametric(ref['margin1'], tree)
    out['oracle_margin1_blocs_a_F'] = [[NAMES[i] for i in sorted(b)] for b in pr.blocks_at(u1, F2)]
    print(json.dumps(out, indent=1))
    with open('recus_c7b_fusion_parasite.json', 'w') as fh:
        json.dump(out, fh, indent=1)


if __name__ == '__main__':
    main()
