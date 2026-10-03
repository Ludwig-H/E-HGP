#!/usr/bin/env python3
"""E6 (piste, pas un resultat) : profil couvrant du site conteste de chaque cellule de l'utilisateur, coupe a la
fusion de ses deux cotes ; pour chaque cote : naissance (plus ancienne entree) et persistance interne (plus longue
barre finie de l'aine a l'interieur du cote, avant la fusion). Intervalle de lambda pour lequel le score
-naissance + lambda * persistance_interne designe le cote voulu par l'utilisateur. Rayons (Decimal -> float).

    python3 -B e6_scores.py > recus_e6_scores.json
"""
import json

import hk
import e1_cibles as c

# (fixture, site conteste, site temoin du cote voulu, site temoin de l'autre cote)
CELLS = [('T0_equilateral_exact', 'C', 'A', 'D'), ('Q1_T1_1700', 'C', 'A', 'D'), ('Q2_S17', 'x', 'a', 'b1'),
         ('Q3_filament', 'x', 'f1', 'c0'), ('Q4_T6_K3', 'C', 'm', 'P')]


def profile(res, i):
    """Noeuds couvrant le site i : {noeud: premier rayon de couverture} ; arbre en rayon."""
    tree = hk.Tree(res, hk.Scale('rad'))
    prof = hk.first_cover(res, len(res.core), 1)[i]
    return {v: hk.dsqrt(l) for v, l in prof.items()}, tree


def side_stats(cov, tree, top, members_nodes):
    """Pour un cote (ensemble de noeuds couvrants sous le noeud 'top' exclu) : naissance et persistance interne.
    Les entrees du cote sont ses noeuds minimaux ; barres par regle de l'aine a chaque fusion interne."""
    nodes = set(members_nodes)
    leaves = [v for v in nodes if not any(ch in nodes for ch in tree.nodes[v].children)
              and not any(w in nodes for w in _desc(tree, v))]
    birth = min(cov[v] for v in leaves)
    # barres internes : pour chaque paire de feuilles, rencontre au plus bas ; regle de l'aine via tri
    pers = 0
    for v in leaves:
        cv = cov[v]
        # mort de la branche v : premiere rencontre avec une feuille plus ancienne (ou egale et d'indice inferieur)
        death = None
        for w in leaves:
            if w == v:
                continue
            cw = cov[w]
            if cw < cv or (cw == cv and w < v):
                mt = tree.meet(v, cv, w, cw)
                death = mt if death is None or mt < death else death
        if death is not None:
            pers = max(pers, death - cv)
    return birth, pers, sorted(float(cov[v]) for v in leaves)


def _desc(tree, v):
    out, stack = [], list(tree.nodes[v].children)
    while stack:
        w = stack.pop()
        out.append(w)
        stack.extend(tree.nodes[w].children)
    return out


def main():
    report = {}
    for fx, site, want, other in CELLS:
        spec = c.FIXTURES[fx]
        names = list(spec['names'])
        res = hk.oracle(spec['points'], spec['k'])
        i = names.index(site)
        cov, tree = profile(res, i)
        # feuille temoin de chaque cote : premier noeud couvrant a la fois i et le temoin
        wi, oi = names.index(want), names.index(other)
        covw = profile(res, wi)[0]
        covo = profile(res, oi)[0]
        lw0 = min((v for v in cov if v in covw), key=lambda v: (max(cov[v], covw[v]), v))
        lo0 = min((v for v in cov if v in covo), key=lambda v: (max(cov[v], covo[v]), v))
        mu = tree.lca(lw0, lo0)
        merge = tree.h[mu]

        def child_on_path(v):
            prev = None
            for w in tree.chain(v):
                if w == mu:
                    return prev
                prev = w
            return None
        cw, co = child_on_path(lw0), child_on_path(lo0)
        side_w = [v for v in cov if v != mu and child_on_path(v) == cw]
        side_o = [v for v in cov if v != mu and child_on_path(v) == co]
        bw, pw, lw = side_stats(cov, tree, None, side_w)
        bo, po, lo = side_stats(cov, tree, None, side_o)
        # -bw + lam pw > -bo + lam po  <=>  lam (pw - po) > bw - bo
        num, den = float(bw - bo), float(pw - po)
        if den > 0:
            cond = 'lambda > %.4f' % (num / den)
        elif den < 0:
            cond = 'lambda < %.4f' % (num / den)
        else:
            cond = 'toujours' if num < 0 else 'jamais'
        report[fx] = dict(site=site, cote_voulu=dict(temoin=want, naissance=float(bw), persistance_interne=float(pw),
                                                      entrees=lw),
                          autre_cote=dict(temoin=other, naissance=float(bo), persistance_interne=float(po), entrees=lo),
                          fusion_des_cotes=float(merge) if merge is not None else None, condition=cond)
    print(json.dumps(report, indent=1))


if __name__ == '__main__':
    main()
