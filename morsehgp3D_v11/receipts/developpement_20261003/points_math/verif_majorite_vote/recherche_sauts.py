#!/usr/bin/env python3
"""Recherche adverse INDEPENDANTE de discontinuites (label verif_majorite_vote).

Configurations de treillis (riches en egalites exactes), dilatees par s = 1000 puis 10000 ; un site deplace d'un
vecteur unite. Saut = max |Delta u| en rayon sur toutes les paires (ultrametrique des pendaisons). Une discontinuite
est signalee si le saut a l'echelle x10 vaut au moins 8 fois le saut a l'echelle x1 et depasse 1 a l'echelle x1.
Regles : ER0hr(1, 10), ER0hr(1, 6), ER0h(1, 12) (mes_regles.py) ; ER0(1, 12) comme temoin positif.
Usage : python3 -B recherche_sauts.py graine secondes > recus/recherche_sauts.json
"""
import json
import random
import sys
import time

sys.dont_write_bytecode = True
import mes_regles as M  # noqa: E402
from prop_s import dec  # noqa: E402

REGLES = {'ER0hr10': lambda T, x: M.er0h(T, x, 1, 10, relatif=True),
          'ER0hr6': lambda T, x: M.er0h(T, x, 1, 6, relatif=True),
          'ER0h12': lambda T, x: M.er0h(T, x, 1, 12),
          'ER0_temoin': lambda T, x: M.er0h(T, x, 1, 12, sans_heritage=True)}


def saut(X, Y, K):
    TX, _ = M.full(X, K)
    TY, _ = M.full(Y, K)
    out = {}
    n = len(X)
    for nom, f in REGLES.items():
        hx = [f(TX, x) for x in range(n)]
        hy = [f(TY, x) for x in range(n)]
        out[nom] = max(abs(dec(M.u_niveau(TX, hx, i, j)) - dec(M.u_niveau(TY, hy, i, j)))
                       for i in range(n) for j in range(i, n))
    return out


def main():
    graine, budget = int(sys.argv[1]), float(sys.argv[2])
    rng = random.Random(graine)
    t0 = time.time()
    essais = 0
    erreurs = 0
    disc = {nom: [] for nom in REGLES}
    pente_max = {nom: 0.0 for nom in REGLES}
    while time.time() - t0 < budget:
        n = rng.randint(4, 6)
        K = rng.choice([2, 2, 3]) if n >= 5 else 2
        grille = rng.choice([3, 4])
        plan = rng.random() < 0.5
        pts = set()
        while len(pts) < n:
            pts.add((rng.randrange(grille), rng.randrange(grille), 0 if plan else rng.randrange(grille)))
        base = sorted(pts)
        j = rng.randrange(n)
        dv = rng.choice([(1, 0, 0), (0, 1, 0), (0, 0, 1), (-1, 0, 0), (0, -1, 0), (0, 0, -1)])
        res = []
        try:
            for s in (1000, 10000):
                X = [tuple(s * c for c in p) for p in base]
                Y = [tuple(X[i][c] + (dv[c] if i == j else 0) for c in range(3)) for i in range(n)]
                res.append(saut(X, Y, K))
        except Exception as err:  # refus de l'oracle (sites confondus, etc.)
            erreurs += 1
            continue
        essais += 1
        for nom in REGLES:
            a, b = float(res[0][nom]), float(res[1][nom])
            pente_max[nom] = max(pente_max[nom], b)
            if a > 1 and b >= 8 * a:
                if len(disc[nom]) < 5:
                    disc[nom].append({'base': base, 'K': K, 'site': j, 'dv': dv, 'saut_x1000': a, 'saut_x10000': b})
                else:
                    disc[nom].append(None)
    print(json.dumps({'graine': graine, 'secondes': round(time.time() - t0, 1), 'essais': essais, 'erreurs': erreurs,
                      'discontinuites': {k: len(v) for k, v in disc.items()},
                      'exemples': {k: [e for e in v if e][:5] for k, v in disc.items()},
                      'saut_max_echelle_10000': pente_max}, indent=1))


if __name__ == '__main__':
    main()
