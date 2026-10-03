#!/usr/bin/env python3
"""C6 : piste lambda du rapport (score -naissance + lambda * persistance interne), recalcul independant (vfull).

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B c6_lambda.py
Cote d'un temoin w : k-parties contenant le site conteste x dont la lignee rejoint celle de la premiere k-partie
contenant {x, w} avant la fusion des deux cotes. Persistance interne : plus longue barre de l'aine entre entrees
du cote (rayon). Variante : meme calcul en excluant les k-parties qui contiennent un site du cote adverse.
"""
import json
import sys

sys.dont_write_bytecode = True
import vfull as V  # noqa: E402
from c1_fixtures import FIX  # noqa: E402

CELLS = [('T0', 'C', 'A', 'D'), ('Q1', 'C', 'A', 'D'), ('Q2', 'x', 'a', 'b1'), ('Q3', 'x', 'f1', 'c0'),
         ('Q4', 'C', 'm', 'P')]


def side(full, x, w, mu, Fw):
    return [F for F in full.parts if x in F and full.level[full.leaf[F]] < mu and full.conn(F, Fw) < mu]


def stats(full, S):
    beta = {F: full.level[full.leaf[F]] for F in S}
    birth = min(beta.values())
    pers = 0.0
    for F in S:
        death = None
        for G in S:
            if G == F:
                continue
            if beta[G] < beta[F] or (beta[G] == beta[F] and G < F):
                c = full.conn(F, G)
                death = c if death is None or c < death else death
        if death is not None:
            pers = max(pers, float(V.R(death) - V.R(beta[F])))
    return float(V.R(birth)), pers


def main():
    out = {}
    for fx, site, want, other in CELLS:
        k, pts, lab = FIX[fx]
        names = lab.split()
        full = V.Full(V.Cloud(pts), k)
        x, w, o = names.index(site), names.index(want), names.index(other)
        Fw = min((F for F in full.parts if x in F and w in F), key=lambda F: (full.level[full.leaf[F]], F))
        Fo = min((F for F in full.parts if x in F and o in F), key=lambda F: (full.level[full.leaf[F]], F))
        mu = full.conn(Fw, Fo)
        Sw, So = side(full, x, w, mu, Fw), side(full, x, o, mu, Fo)
        (bw, pw), (bo, po) = stats(full, Sw), stats(full, So)
        num, den = bw - bo, pw - po
        cond = ('lambda > %.4f' % (num / den)) if den > 0 else (('lambda < %.4f' % (num / den)) if den < 0 else
                                                                 ('toujours' if num < 0 else 'jamais'))
        # variante : cotes sans les k-parties melangeant un site de l'autre cote (ici : celles qui contiennent le
        # temoin adverse ou, pour Q4, m)
        rec = {'site': site, 'fusion': float(V.R(mu)), 'voulu': [bw, pw, len(Sw)], 'autre': [bo, po, len(So)],
               'condition': cond}
        if fx == 'Q4':
            mi = names.index('m')
            Sw2 = Sw
            So2 = [F for F in So if mi not in F]
            (bo2, po2) = stats(full, So2)
            rec['autre_sans_m'] = [bo2, po2, len(So2)]
            rec['condition_sans_m'] = 'lambda < %.4f' % ((bw - bo2) / (pw - po2))
        out[fx] = rec
    print(json.dumps(out, indent=1))
    with open('recus_c6_lambda.json', 'w') as fh:
        json.dump(out, fh, indent=1)


if __name__ == '__main__':
    main()
