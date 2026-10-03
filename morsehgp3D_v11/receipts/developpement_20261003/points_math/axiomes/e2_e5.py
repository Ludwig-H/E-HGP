#!/usr/bin/env python3
"""E2 a E5 : anticipation, stabilite en rayon, entrees apres le coeur, balayage du seuil m.

    python3 -B e2_e5.py e2            # anticipation : meme passe, dates differentes (P_1, Q_1)
    python3 -B e2_e5.py e3 GRAINE N   # rapports |de|/eps et |du|/eps en rayon sous perturbation appariee
    python3 -B e2_e5.py e4 GRAINE N   # entrees apres le coeur : max t/d_K (rayon), familles et nuages aleatoires
    python3 -B e2_e5.py e5            # seuil m et pente kappa contre Q1bis et Q2 (famille H)

Notations : P_kappa = H_kappa en rayon (= ancrage persistant de la v10) ; Q_1 = H_1 en niveau carre (regle du
developpeur v11, = Q_1 quadratique de la v10). Sorties JSON sur stdout.
"""
from decimal import Decimal
from fractions import Fraction
import json
import math
import random
import sys
import time

import hk


def radius(scale, x):
    return float(hk.dsqrt(x)) if scale == 'sq' else float(x)


def rules_all(res, n, k):
    """Regles comparees : (nom, echelle, entrees, arbre)."""
    out = []
    for name, kappa, m, scale in (('P1', 1, 1, 'rad'), ('P2', 2, 1, 'rad'), ('Q1', 1, 1, 'sq'),
                                  ('P1_m', 1, k + 1, 'rad'), ('Q1_m', 1, k + 1, 'sq')):
        if m > n:
            continue
        ent, tree = hk.rule_H(res, n, kappa, m, scale)
        out.append((name, scale, ent, tree))
    ref, tree = hk.rule_reference(res, n, 1, 'sq')
    out.append(('core', 'sq', ref['core'], tree))
    out.append(('cover', 'sq', ref['cover'], tree))
    return out


# ---------------------------------------------------------------- E2 anticipation

def e2():
    a = (2, 0, 0)
    x = (0, 0, 0)
    cases = {'b_oppose_180': (-20, 0, 0), 'b_143deg': (-16, 12, 0), 'b_meme_cote_sans_rival': (20, 0, 0),
             'tie_symetrique': (-2, 0, 0), 'quasi_tie_delta1': (-3, 0, 0)}
    out = {}
    for name, b in cases.items():
        pts = [x, a, b]
        res = hk.oracle(pts, 2)
        nodes = sorted((str(nd.level), len(nd.children)) for nd in res.nodes)
        row = dict(points=pts, noeuds_FULL2=nodes)
        for rname, kappa, scale in (('P1', 1, 'rad'), ('Q1', 1, 'sq')):
            ent, _t = hk.rule_H(res, 3, kappa, 1, scale)
            row[rname + '_entree_rayon_x'] = round(radius(scale, ent[0][0]), 6)
        ref, _t = hk.rule_reference(res, 3, 1, 'sq')
        row['cover_entree_rayon_x'] = round(radius('sq', ref['cover'][0][0]), 6)
        row['onset_rival_rayon'] = None
        # premier niveau ou un noeud autre que la lentille xa couvre x
        prof = hk.first_cover(res, 3, 1)[0]
        lv = sorted(prof.values())
        row['niveaux_couverture_x'] = [str(v) for v in lv]
        out[name] = row
    return out


# ---------------------------------------------------------------- E3 stabilite en rayon

def random_cloud(rng, n):
    side = rng.choice([4, 6, 10])
    scale = rng.choice([3, 10, 40])
    pts = set()
    while len(pts) < n:
        pts.add((rng.randrange(side) * scale, rng.randrange(side) * scale, rng.choice([0, rng.randrange(side) * scale])))
    pts = sorted(pts)
    rng.shuffle(pts)
    return pts


def perturb(rng, pts):
    while True:
        q = [tuple(c + rng.choice([-1, 0, 1]) for c in p) for p in pts]
        if len(set(q)) == len(q):
            eps = max(math.sqrt(sum((a - b) ** 2 for a, b in zip(p, r))) for p, r in zip(pts, q))
            if eps > 0:
                return q, eps


def ultra_radius(scale, ent, tree):
    u = hk.ultrametric(ent, tree)
    n = len(u)
    return [[None if u[i][j] is None else radius(scale, u[i][j]) for j in range(n)] for i in range(n)]


def e3(seed, count):
    rng = random.Random(seed)
    worst = {}
    t0 = time.time()
    pairs = 0
    for _ in range(count):
        n = rng.randint(4, 8)
        pts = random_cloud(rng, n)
        q, eps = perturb(rng, pts)
        dx, dy = hk.Definition(pts), hk.Definition(q)
        for k in range(1, min(3, n - 1) + 1):
            rx = rules_all(dx.order(k), n, k)
            ry = rules_all(dy.order(k), n, k)
            for (name, scale, ex, tx), (_n2, _s2, ey, ty) in zip(rx, ry):
                ux, uy = ultra_radius(scale, ex, tx), ultra_radius(scale, ey, ty)
                de = du = 0.0
                for i in range(n):
                    if ux[i][i] is None or uy[i][i] is None:
                        continue
                    de = max(de, abs(ux[i][i] - uy[i][i]))
                    for j in range(n):
                        if ux[i][j] is not None and uy[i][j] is not None:
                            du = max(du, abs(ux[i][j] - uy[i][j]))
                w = worst.setdefault(name, dict(de=0.0, du=0.0, ex_de=None, ex_du=None))
                if de / eps > w['de']:
                    w['de'], w['ex_de'] = de / eps, dict(points=pts, perturbed=q, k=k, eps=eps)
                if du / eps > w['du']:
                    w['du'], w['ex_du'] = du / eps, dict(points=pts, perturbed=q, k=k, eps=eps)
            pairs += 1
    for w in worst.values():
        w['de'] = round(w['de'], 4)
        w['du'] = round(w['du'], 4)
    return dict(seed=seed, clouds=count, pairs=pairs, seconds=round(time.time() - t0, 1), worst=worst)


# ---------------------------------------------------------------- E4 entrees apres le coeur

def e4(seed, count):
    out = {}
    # famille {-2L, 0, 2} a K = 2 : rival lointain de persistance 1 en rayon
    fam = {}
    for L in (10, 100, 1000):
        pts = [(-2 * L, 0, 0), (0, 0, 0), (2, 0, 0)]
        res = hk.oracle(pts, 2)
        dk = radius('sq', res.core[1].level)
        row = dict(dK_rayon=dk)
        for name, kappa, m, scale in (('P1', 1, 1, 'rad'), ('Q1', 1, 1, 'sq')):
            ent, _t = hk.rule_H(res, 3, kappa, m, scale)
            row[name + '_rayon'] = round(radius(scale, ent[1][0]), 6)
            row[name + '_niveau_sur_DK'] = round(radius(scale, ent[1][0]) ** 2 / dk ** 2, 4)
        fam['L=%d' % L] = row
    out['famille_rival_lointain_K2'] = fam
    rng = random.Random(seed)
    best = {}
    t0 = time.time()
    for _ in range(count):
        n = rng.randint(4, 8)
        pts = random_cloud(rng, n)
        d = hk.Definition(pts)
        for k in range(2, min(4, n - 1) + 1):
            res = d.order(k)
            for name, kappa, m, scale in (('P1', 1, 1, 'rad'), ('P2', 2, 1, 'rad'), ('Q1', 1, 1, 'sq'),
                                          ('P1_m', 1, k + 1, 'rad'), ('Q1_m', 1, k + 1, 'sq')):
                if m > n:
                    continue
                ent, _t = hk.rule_H(res, n, kappa, m, scale)
                for i in range(n):
                    if ent[i] is None:
                        continue
                    dk = radius('sq', res.core[i].level)
                    al = radius('sq', res.cover[i].level)
                    ratio = radius(scale, ent[i][0]) / dk
                    key = '%s_k%d' % (name, k)
                    b = best.setdefault(key, dict(max_t_sur_dK=0.0, max_retard_sur_dK=0.0, n_apres_coeur=0,
                                                  n_sites=0, ex=None))
                    b['n_sites'] += 1
                    if ratio > 1 + 1e-12:
                        b['n_apres_coeur'] += 1
                    if ratio > b['max_t_sur_dK']:
                        b['max_t_sur_dK'] = ratio
                        b['ex'] = dict(points=pts, k=k, site=i, t=radius(scale, ent[i][0]), dK=dk, alpha=al)
                    if m == 1:
                        b['max_retard_sur_dK'] = max(b['max_retard_sur_dK'], (radius(scale, ent[i][0]) - al) / dk)
    for b in best.values():
        b['max_t_sur_dK'] = round(b['max_t_sur_dK'], 6)
        b['max_retard_sur_dK'] = round(b['max_retard_sur_dK'], 6)
    out['aleatoire'] = dict(seed=seed, clouds=count, seconds=round(time.time() - t0, 1), par_regle=best)
    return out


# ---------------------------------------------------------------- E5 seuil m

def e5():
    import e1_cibles as c
    out = {}
    for fx in ('Q1_T1_1700', 'Q2_S17', 'T0_equilateral_exact', 'Q3_filament', 'Q4_T6_K3'):
        spec = c.FIXTURES[fx]
        pts, k, names = spec['points'], spec['k'], spec['names']
        n = len(pts)
        res = hk.oracle(pts, k)
        root_r = float(hk.dsqrt(max(node.level for node in res.nodes)))
        rows = {}
        for m in range(1, min(n, 6) + 1):
            for kappa in (1, 2, 4, 1000):
                for scale in ('rad', 'sq'):
                    ent, tree = hk.rule_H(res, n, kappa, m, scale)
                    if any(e is None for e in ent):
                        continue
                    u = hk.ultrametric(ent, tree)
                    rows['m%d_kappa%d_%s' % (m, kappa, scale)] = c.verdicts(fx, names, scale, u, root_r)
        out[fx] = rows
    # synthese : (m, kappa, echelle) qui passent Q1bis (mcs 2) ET Q2 (structure a 75)
    both = []
    for key, v1 in out['Q1_T1_1700'].items():
        v2 = out['Q2_S17'].get(key)
        if v2 is None:
            continue
        if v1['ABC|DEF_avant_fusion_mcs2'] and v2['x_avec_a_et_b1b2_a_75']:
            both.append(key)
    out['_passent_Q1bis_et_Q2'] = both
    out['_q1bis_seul'] = sorted(kk for kk, v in out['Q1_T1_1700'].items() if v['ABC|DEF_avant_fusion_mcs2'])
    out['_q2_seul'] = sorted(kk for kk, v in out['Q2_S17'].items() if v['x_avec_a_et_b1b2_a_75'])
    return out


def e3b():
    """Famille ciblee : rival lointain {-2L, 0, 2} a K = 2, on deplace a = (2,0,0) en (3,0,0) (eps = 1)."""
    out = {}
    for L in (10, 100, 1000, 10000):
        row = {}
        for name, kappa, scale in (('P1', 1, 'rad'), ('Q1', 1, 'sq'), ('P2', 2, 'rad')):
            vals = []
            for a in ((2, 0, 0), (3, 0, 0)):
                pts = [(-2 * L, 0, 0), (0, 0, 0), a]
                res = hk.oracle(pts, 2)
                ent, _t = hk.rule_H(res, 3, kappa, 1, scale)
                vals.append(radius(scale, ent[1][0]))
            row[name] = dict(entrees_rayon=[round(v, 6) for v in vals], rapport_sur_eps=round(abs(vals[1] - vals[0]), 6))
        out['L=%d' % L] = row
    return out


def main():
    what = sys.argv[1]
    if what == 'e2':
        res = e2()
    elif what == 'e3':
        res = e3(int(sys.argv[2]), int(sys.argv[3]))
    elif what == 'e4':
        res = e4(int(sys.argv[2]), int(sys.argv[3]))
    elif what == 'e5':
        res = e5()
    elif what == 'e3b':
        res = e3b()
    else:
        raise SystemExit('usage')
    res['_near_ties_rad'] = hk.NEAR_TIES[0]
    print(json.dumps(res, indent=1, default=str))


if __name__ == '__main__':
    main()
