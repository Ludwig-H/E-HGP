"""DEV : decomposition de l'ecart au plafond de Bayes sur les scenes gaussiennes de la campagne alloc_dev (G4).

  - plafond de Bayes : MAP du melange (parametres estimes sur la verite), bruit uniforme sur la boite elargie ;
  - plafond de Morse : bassins d'attraction de la VRAIE densite du melange (memes parametres) : QuickShift sur f
    evaluee aux points (chaque point pointe vers le plus dense de ses k voisins, lui compris), chaque maximum atteint
    prend la composante qui y domine ; meme rejet du bruit que Bayes. C'est la cible d'une methode par niveaux de
    densite qui connaitrait la densite : Bayes - Morse = prix du modele (les frontieres de Bayes ne sont pas les cols),
    Morse - tour = prix de l'estimation, de la selection et de l'affectation.
Graines dev seulement ; les etiquettes des methodes viennent de alloc_dev_g4.csv."""
import collections
import csv
import math
import sys
from concurrent.futures import ProcessPoolExecutor

import numpy as np
from scipy.spatial import cKDTree

SYN = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/bench/synthetic'
sys.path.insert(0, SYN)
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402

GAUSS = ('spherical', 'anisotropic', 'heteroscedastic', 'unbalanced')


def model(P, L):
    groups = sorted(set(L.tolist()) - {-1})
    n = len(P)
    logp = []
    for g in groups:
        X = P[L == g]
        mu = X.mean(axis=0)
        C = np.cov(X.T) + 1e-9 * np.eye(3)
        d = P - mu
        m = np.einsum('ij,jk,ik->i', d, np.linalg.inv(C), d)
        logp.append(math.log(len(X) / n) - 0.5 * m - 0.5 * math.log(np.linalg.det(C)) - 1.5 * math.log(2 * math.pi))
    logp = np.array(logp)
    nz = (L == -1).sum()
    lnoise = -np.inf
    if nz:
        G = P[L >= 0]
        low, high = G.min(axis=0), G.max(axis=0)
        margin = 0.1 * (high - low)
        lnoise = math.log(nz / n) - math.log(float(np.prod(high - low + 2 * margin)))
    return np.array(groups), logp, lnoise


def unit(spec):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    X = np.asarray(G, dtype=np.float64)
    groups, logp, lnoise = model(X, T)
    top = logp.max(axis=0)
    logf = top + np.log(np.exp(logp - top).sum(axis=0) + (np.exp(lnoise - top) if np.isfinite(lnoise) else 0.0))
    noise = top < lnoise
    bayes = np.where(noise, -1, groups[np.argmax(logp, axis=0)])
    out = {'bayes': metrics.scores(T, bayes)['ari_s']}
    tree = cKDTree(X)
    for k in (20, 40):
        _, nbr = tree.query(X, k=k)
        best = np.lexsort((nbr, -logf[nbr]), axis=1)[:, 0]
        parent = nbr[np.arange(len(X)), best]
        root = parent.copy()
        while True:
            nxt = root[root]
            if np.array_equal(nxt, root):
                break
            root = nxt
        lab = groups[np.argmax(logp[:, root], axis=0)]
        morse = np.where(noise, -1, lab)
        out['morse%d' % k] = metrics.scores(T, morse)['ari_s']
        out['roots%d' % k] = int(len(np.unique(root[~noise])))
    key = (spec['family'], spec['level'], str(spec['noise_fraction']), str(len(G)), str(spec['seed']))
    return key, out


def main():
    specs = [s for s in run_campaign.plan('dev', [8000, 16000, 32000], 2) if s['family'] in GAUSS]
    with ProcessPoolExecutor(max_workers=2) as pool:
        ceil = dict(pool.map(unit, specs))
    res = collections.defaultdict(dict)
    for r in csv.DictReader(open('/workspaces/E-HGP/build/v10-persist/alloc/alloc_dev_g4.csv')):
        u = (r['family'], r['level'], r['noise'], r['n'], r['seed'])
        if u in ceil:
            res[u][(r['method'], int(r['k']), r['fill'])] = float(r['ari_s'])
    with open('/workspaces/E-HGP/build/v10-persist/alloc/morse_ceiling.csv', 'w', newline='') as h:
        w = csv.writer(h)
        w.writerow(('family', 'level', 'noise', 'n', 'seed', 'bayes', 'morse20', 'morse40', 'roots20', 'roots40'))
        for u, c in sorted(ceil.items()):
            w.writerow(u + (round(c['bayes'], 6), round(c['morse20'], 6), round(c['morse40'], 6), c['roots20'],
                            c['roots40']))
    lotc = {'tour': {1: 'b2', 3: 'b2', 5: 'b2', 10: 'b1.5'}, 'sklearn': {1: 'b1.5', 3: 'b2', 5: 'b2.5', 10: 'b2.5'}}
    units = sorted(res)
    print('%d scenes gaussiennes' % len(units))
    for k in (3, 5, 10):
        print('\n== K=%d' % k)
        print('%-16s %-8s %4s %7s %7s %7s %7s %7s %7s' % ('famille', 'niveau', 'n', 'bayes', 'morse20', 'morse40',
                                                          'tourC', 'tourA', 'skC'))
        groups = collections.defaultdict(list)
        for u in units:
            groups[(u[0], u[1])].append(u)
            groups[(u[0], 'tous')].append(u)
            groups[('tous', 'tous')].append(u)
        for key in sorted(groups):
            us = groups[key]
            vals = [np.mean([ceil[u][c] for u in us]) for c in ('bayes', 'morse20', 'morse40')]
            vals += [np.mean([res[u][('tour', k, lotc['tour'][k])] for u in us]),
                     np.mean([res[u][('tour', k, 'asc20_b2')] for u in us]),
                     np.mean([res[u][('sklearn', k, lotc['sklearn'][k])] for u in us])]
            print('%-16s %-8s %4d %s' % (key[0], key[1], len(us), ' '.join('%7.4f' % v for v in vals)))


if __name__ == '__main__':
    main()
