"""DEV : plafond de Bayes (MAP du melange gaussien, parametres estimes sur la verite, bruit uniforme sur la boite
elargie ; fonction copiee de v10-persist/bench/gauss_ceiling_dev.py) pour les scenes gaussiennes de la campagne
alloc_dev (G4), puis part de l'ecart au plafond fermee par l'affectation asc20_b2 de la tour."""
import collections
import csv
import math
import sys
from concurrent.futures import ProcessPoolExecutor

import numpy as np

SYN = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/bench/synthetic'
sys.path.insert(0, SYN)
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402

GAUSS = ('spherical', 'anisotropic', 'heteroscedastic', 'unbalanced')


def bayes_ceiling(P, L):
    groups = sorted(set(L.tolist()) - {-1})
    n = len(P)
    logp = []
    for g in groups:
        X = P[L == g]
        mu = X.mean(axis=0)
        C = np.cov(X.T) + 1e-9 * np.eye(3)
        Ci = np.linalg.inv(C)
        d = P - mu
        m = np.einsum('ij,jk,ik->i', d, Ci, d)
        logp.append(math.log(len(X) / n) - 0.5 * m - 0.5 * math.log(np.linalg.det(C)) - 1.5 * math.log(2 * math.pi))
    logp = np.array(logp)
    lab = np.array(groups)[np.argmax(logp, axis=0)]
    nz = (L == -1).sum()
    if nz:
        G = P[L >= 0]
        low, high = G.min(axis=0), G.max(axis=0)
        margin = 0.1 * (high - low)
        vol = float(np.prod(high - low + 2 * margin))
        lnoise = math.log(nz / n) - math.log(vol)
        lab = np.where(logp.max(axis=0) >= lnoise, lab, -1)
    return metrics.scores(L, lab)['ari_s']


def unit(spec):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    return (spec['family'], spec['level'], str(spec['noise_fraction']), str(len(G)), str(spec['seed'])), \
        bayes_ceiling(np.asarray(G, dtype=np.float64), T)


def main():
    specs = [s for s in run_campaign.plan('dev', [8000, 16000, 32000], 2) if s['family'] in GAUSS]
    with ProcessPoolExecutor(max_workers=2) as pool:
        ceil = dict(pool.map(unit, specs))
    with open('/workspaces/E-HGP/build/v10-persist/alloc/ceiling_alloc.csv', 'w', newline='') as h:
        w = csv.writer(h)
        w.writerow(('family', 'level', 'noise', 'n', 'seed', 'ceiling'))
        for u, c in sorted(ceil.items()):
            w.writerow(u + (round(c, 6),))
    res = collections.defaultdict(dict)
    for r in csv.DictReader(open('/workspaces/E-HGP/build/v10-persist/alloc/alloc_dev_g4.csv')):
        u = (r['family'], r['level'], r['noise'], r['n'], r['seed'])
        if u in ceil:
            res[u][(r['method'], int(r['k']), r['fill'])] = float(r['ari_s'])
    lotc = {'tour': {1: 'b2', 2: 'b2', 3: 'b2', 5: 'b2', 8: 'b1.5', 10: 'b1.5'},
            'sklearn': {1: 'b1.5', 2: 'b1.5', 3: 'b2', 5: 'b2.5', 8: 'b2.5', 10: 'b2.5'}}
    units = sorted(res)
    print('%d scenes gaussiennes' % len(units))
    for k in (1, 3, 5, 10):
        print('\n== K=%d : moyenne ARI_s (plafond, tour lot C, tour asc20_b2, sklearn lot C), part de l ecart fermee' % k)
        print('%-16s %-8s %4s %8s %8s %8s %8s %8s' % ('famille', 'niveau', 'n', 'plafond', 'tourC', 'tourA', 'skC', 'ferme'))
        groups = collections.defaultdict(list)
        for u in units:
            groups[(u[0], u[1])].append(u)
            groups[(u[0], 'tous')].append(u)
            groups[('tous', 'tous')].append(u)
        for key in sorted(groups):
            us = groups[key]
            c = np.mean([ceil[u] for u in us])
            tc = np.mean([res[u][('tour', k, lotc['tour'][k])] for u in us])
            ta = np.mean([res[u][('tour', k, 'asc20_b2')] for u in us])
            sc = np.mean([res[u][('sklearn', k, lotc['sklearn'][k])] for u in us])
            closed = (ta - tc) / (c - tc) if c - tc > 1e-9 else float('nan')
            print('%-16s %-8s %4d %8.4f %8.4f %8.4f %8.4f %+8.2f' % (key[0], key[1], len(us), c, tc, ta, sc, closed))


if __name__ == '__main__':
    main()
