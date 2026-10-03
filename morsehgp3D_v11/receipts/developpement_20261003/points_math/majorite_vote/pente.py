#!/usr/bin/env python3
"""Stabilite exacte des regles sous une perturbation apparie X -> Y de meme combinatoire.

delta (niveau carre) = max des decalages des niveaux de noeuds apparies et des debuts de couverture c_x(v) apparies.
Quand les deux arbres ont la meme combinatoire, ces decalages definissent un delta-entrelacement avec transport des
couvertures (cadre de H3) : le rapport |Delta u| / delta mesure est donc un MINORANT du rapport par rapport au
meilleur entrelacement.

Appariement des noeuds sans les niveaux : cle(naissance) = ensemble des sites couverts a la naissance ;
cle(fusion) = ensemble des cles des enfants. Refus si l'appariement n'est pas une bijection.

Familles :
  isocele(L, h, d) : x = (0,0,0), y1 = (L,h,0), y2 = (L,-h,0) [egalite] contre y2 = (L,-h,d) ; K = 2.
  selle(a, b, e, d) : x = (0,0,0), g1 = (a,b,0), g1' = (a,b,e), g2 = (a,-b,0), g2' = (a,-b,-e) [egalite]
                      contre g2 et g2' deplaces de (0,0,d)... voir selle_points ; K = 2.

Usage : python3 -B pente.py > recus/pente.json
"""
from fractions import Fraction
import json
import sys
import time

sys.dont_write_bytecode = True
import arbre  # noqa: E402
import regles as RG  # noqa: E402
from surd import Surd  # noqa: E402


class MatchError(RuntimeError):
    pass


def node_keys(tree):
    keys = {}
    order = sorted(range(len(tree)), key=lambda v: tree.birth[v])
    for v in order:
        if not tree.children[v]:
            keys[v] = ('b', frozenset(x for x in range(tree.n) if tree.cov[x].get(v) == tree.birth[v]))
        else:
            keys[v] = ('f', frozenset(keys[c] for c in tree.children[v]))
    return keys


def match(tx, ty):
    kx, ky = node_keys(tx), node_keys(ty)
    inv = {}
    for v, k in ky.items():
        if k in inv:
            raise MatchError('cle dupliquee dans Y')
        inv[k] = v
    out = {}
    for v, k in kx.items():
        if k not in inv:
            raise MatchError('noeud sans image')
        out[v] = inv[k]
    if len(set(out.values())) != len(ty) or len(tx) != len(ty):
        raise MatchError('pas une bijection')
    return out


def delta_of(tx, ty, mp):
    d = Fraction(0)
    for v, w in mp.items():
        d = max(d, abs(tx.birth[v] - ty.birth[w]))
    for x in range(tx.n):
        cx, cy = tx.cov[x], ty.cov[x]
        if set(mp[v] for v in cx) != set(cy):
            raise MatchError('couvertures non appariees (site %d)' % x)
        for v, c in cx.items():
            d = max(d, abs(c - cy[mp[v]]))
    return d


def level_diff(a, b):
    """|a^2 - b^2| pour deux rayons Surd : encadrement certifie [lo, hi] (Fraction)."""
    diff = a.square() - b.square()
    lo, hi = diff.bounds(200)
    if lo >= 0:
        return lo, hi
    if hi <= 0:
        return -hi, -lo
    return Fraction(0), max(-lo, hi)


def compare(px, py, k, rules, names=None):
    _dx, _rx, tx = arbre.v11_tree(px, k)
    _dy, _ry, ty = arbre.v11_tree(py, k)
    mp = match(tx, ty)
    delta = delta_of(tx, ty, mp)
    out = {'delta': str(delta), 'delta_float': float(delta)}
    for name, fn in rules:
        hx, hy = fn(tx), fn(ty)
        ux, uy = RG.ultrametric(tx, hx), RG.ultrametric(ty, hy)
        n = len(px)
        worst = None
        for i in range(n):
            for j in range(i, n):
                lo, hi = level_diff(ux[i][j], uy[i][j])
                if worst is None or lo > worst[0]:
                    worst = (lo, hi, i, j)
        lo, hi, i, j = worst
        jr = None
        for a in range(n):
            for b in range(a, n):
                dlo, dhi = (ux[a][b] - uy[a][b]).bounds(200)
                m = max(abs(dlo), abs(dhi))
                if jr is None or m > jr[0]:
                    jr = (m, a, b)
        rec = {'saut_rayon_max': float(jr[0]), 'saut_rayon_paire': [jr[1], jr[2]] if names is None else [names[jr[1]], names[jr[2]]],'pire_paire': [i, j] if names is None else [names[i], names[j]],
               'du_niveau': [float(lo), float(hi)],
               'rapport': [float(lo / delta), float(hi / delta)] if delta else None,
               'entree_x_X': float(hx[0][0]), 'entree_x_Y': float(hy[0][0]),
               'entree_x_X_niveau': float(hx[0][0].square()), 'entree_x_Y_niveau': float(hy[0][0].square())}
        out[name] = rec
    return out


def rules_list(k):
    return [('ER0h(1,12)', lambda t: RG.rule_er0h(t, 1, 12)),
            ('ER0(1,12)', lambda t: RG.rule_er0(t, 1, 12)),
            ('ER0hv(1,12,1/20)', lambda t: RG.rule_er0hv(t, 1, 12, Fraction(1, 20))),
            ('ER0hv(1,12,2/5)', lambda t: RG.rule_er0hv(t, 1, 12, Fraction(2, 5))),
            ('ER0hr(1,10)', lambda t: RG.rule_er0hr(t, 1, 10)),
            ('H_1', lambda t: RG.rule_hm(t, 1)),
            ('H_k+1', lambda t: RG.rule_hm(t, k + 1))]


def isocele(L, h, d):
    X = [(0, 0, 0), (L, h, 0), (L, -h, 0)]
    Y = [(0, 0, 0), (L, h, 0), (L, -h, d)]
    return X, Y


def isocele_radiale(L, h, d):
    """Meme figure ; perturbation au PREMIER ordre : y2 deplace de d selon l'axe x (presque radial)."""
    X = [(0, 0, 0), (L, h, 0), (L, -h, 0)]
    Y = [(0, 0, 0), (L, h, 0), (L + d, -h, 0)]
    return X, Y


def selle(a, b, e, d):
    """Selle : x entre deux paires serrees G1 = {g1, g1'} et G2 = {g2, g2'} qui fusionnent juste apres la premiere
    couverture de x (angles droits en g1 et g2). Perturbation : g2 et g2' decales de d selon x (eloignement)."""
    X = [(0, 0, 0), (a, b, 0), (a, b, e), (a, -b, 0), (a, -b, -e)]
    Y = [(0, 0, 0), (a, b, 0), (a, b, e), (a + d, -b, 0), (a + d, -b, -e)]
    return X, Y
# Dans selle, les angles droits en g1 (vers x et vers g2) et en g2 sont conserves par le deplacement (d, 0, 0) de
# g2 et g2' : x est couvert par {x,g1} et {x,g2} a o1, o2, ces paires rejoignent G1 = {g1,g1'} et G2 = {g2,g2'} a
# o_i + e^2/4, puis G1 et G2 fusionnent par la paire {g1,g2} a b^2 + e^2/4 + d^2/4.


def main():
    t0 = time.time()
    out = {'isocele': [], 'isocele_radiale': [], 'selle': []}
    names3 = ['x', 'y1', 'y2']
    for inv_r in (5, 10, 20, 40, 80, 160):
        L = 8 * inv_r * inv_r
        h = L // inv_r
        X, Y = isocele(L, h, 1)
        rec = compare(X, Y, 2, rules_list(2), names3)
        rec.update({'L': L, 'h': h, 'd': 1, 'kappa_L2_sur_h2': 12 * L * L / (h * h)})
        out['isocele'].append(rec)
        sys.stderr.write('isocele 1/%d %.1fs\n' % (inv_r, time.time() - t0))
    for inv_r in (5, 10, 20, 40):
        L = 4 * 12 * inv_r ** 4
        h = L // inv_r
        X, Y = isocele_radiale(L, h, 1)
        rec = compare(X, Y, 2, rules_list(2), names3)
        rec.update({'L': L, 'h': h, 'd': 1, 'kappa_L2_sur_h2': 12 * L * L / (h * h)})
        out['isocele_radiale'].append(rec)
        sys.stderr.write('isocele radiale 1/%d %.1fs\n' % (inv_r, time.time() - t0))
    names5 = ['x', 'g1', "g1'", 'g2', "g2'"]
    for (a0, b0, e0, lam) in ((1700, 1000, 100, 50), (1700, 1000, 100, 400), (1720, 1000, 100, 400),
                              (1728, 1000, 50, 2000), (1731, 1000, 20, 20000)):
        a, b, e = a0 * lam, b0 * lam, e0 * lam
        X, Y = selle(a, b, e, 1)
        try:
            rec = compare(X, Y, 2, rules_list(2), names5)
        except MatchError as err:
            rec = {'refus': str(err)}
        rec.update({'a': a, 'b': b, 'e': e, 'd': 1})
        out['selle'].append(rec)
        sys.stderr.write('selle %d %d %.1fs\n' % (a0, lam, time.time() - t0))
    out['_secondes'] = round(time.time() - t0, 1)
    print(json.dumps(out, indent=1, default=str))


if __name__ == '__main__':
    main()
