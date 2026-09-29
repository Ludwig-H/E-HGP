"""DEV (reponse a l'utilisateur, 29 sept.) : sur les familles gaussiennes (un mode par groupe), la hierarchie de la tour
contient-elle les groupes ? Pour chaque scene dev :
  - plafond de Bayes : partition MAP du melange (moyennes, covariances et poids estimes sur la verite ; bruit
    uniforme sur la boite du generateur), ARI_s ;
  - pour chaque hierarchie (tour entree cover ; MR1 et MR2 de sklearn, entree coeur ; MR2 entree bord) et chaque K :
      * meilleure coupe horizontale (composantes de moins de mcs points = bruit) : ce que la hierarchie CONTIENT ;
      * EOM z = 3 sans remplissage (tete v10, port Python exact) : ce que la tete en EXTRAIT.
Graines dev seulement. Aucune de ces quantites n'est un juge du clustering : le plafond depend du modele generateur.
"""
import csv
import math
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np

SYN = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/bench/synthetic'
sys.path.insert(0, SYN)
sys.path.insert(0, '/workspaces/E-HGP/build/v10-persist/audit_hier/auditeur_objet')
import methods  # noqa: E402
import metrics  # noqa: E402
import objet_lib as ol  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402

BUILD = '/workspaces/E-HGP/build/v10-bench-8b8d66f6e/build'
KS = (3, 5, 10)
COLS = ('family', 'level', 'noise', 'n', 'seed', 'hier', 'k', 'ceiling', 'best_cut', 'eom_z3', 'eom_z3_b15', 'eom_z3_full',
        'eom_z1', 'clusters_eom3')


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


def best_cut_mcs(H, truth, mcs, ncuts=200):
    lev, par = H['node_level'], H['parent']
    cand = np.unique(np.concatenate([lev, H['point_level']]))
    if len(cand) > ncuts:
        cand = np.unique(np.quantile(cand, np.linspace(0, 1, ncuts)))
    nn = len(par)
    best = -1.0
    for c in cand:
        up = np.where((par >= 0) & (lev[np.maximum(par, 0)] <= c), par, np.arange(nn))
        for _ in range(64):
            nxt = up[up]
            if np.array_equal(nxt, up):
                break
            up = nxt
        lab = up[H['point_node']]
        lab = np.where(H['point_level'] <= c, lab, -1)
        vals, cnt = np.unique(lab[lab >= 0], return_counts=True)
        small = set(vals[cnt < mcs].tolist())
        if small:
            lab = np.where(np.isin(lab, list(small)), -1, lab)
        best = max(best, metrics.scores(truth, lab)['ari_s'])
    return best


def tower_tree(G, k, mcs):
    with tempfile.TemporaryDirectory() as tmp:
        src, out, tree = (os.path.join(tmp, x) for x in ('in', 'out', 'tree'))
        np.ascontiguousarray(G, dtype='<u4').tofile(src)
        r = subprocess.run([os.path.join(BUILD, 'mhgp10_cluster'), src, out, '--k=%d' % k, '--mcs=%d' % mcs, '--z=3',
                            '--entry=cover', '--threads=2', '--tree=' + tree], capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError(r.stdout + r.stderr)
        return ol.read_tower_tree(tree), np.fromfile(out, dtype='<i4').astype(np.int64)


def run_unit(spec):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    n = len(G)
    mcs = int(round(math.sqrt(n)))
    ceil = bayes_ceiling(np.asarray(G, dtype=np.float64), T)
    base = dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=n, seed=spec['seed'],
                ceiling=round(ceil, 4))
    out = []
    for k in KS:
        H, lab_bin = tower_tree(G, k, mcs)
        hs = [('tower_cover', H), ('mr1', ol.sk_mr_tree(G, k, 1)), ('mr2', ol.sk_mr_tree(G, k, 2))]
        hs.append(('mr2_border', ol.mr_border(G, k, 2, hs[2][1])))
        for name, h in hs:
            e3 = np.asarray(ol.head_labels(h, mcs, 3.0), dtype=np.int64)
            e1 = np.asarray(ol.head_labels(h, mcs, 1.0), dtype=np.int64)
            s3 = metrics.scores(T, e3)
            b15 = metrics.scores(T, methods.bounded_fill(G, e3, max(k, 5), 1.5))['ari_s']
            full = metrics.scores(T, methods.fill_noise(G, e3))['ari_s']
            out.append(dict(base, hier=name, k=k, best_cut=round(best_cut_mcs(h, T, mcs), 4),
                            eom_z3=round(s3['ari_s'], 4), eom_z3_b15=round(b15, 4), eom_z3_full=round(full, 4),
                            eom_z1=round(metrics.scores(T, e1)['ari_s'], 4), clusters_eom3=s3['clusters']))
    return out


def main():
    fams = sys.argv[3].split(',') if len(sys.argv) > 3 else ['spherical']
    specs = [s for s in run_campaign.plan('dev', [2000, 8000], 2) if s['family'] in fams]
    with open(sys.argv[1], 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=int(sys.argv[2])) as pool:
            futs = {pool.submit(run_unit, s): s for s in specs}
            for i, fu in enumerate(as_completed(futs)):
                try:
                    for r in fu.result():
                        w.writerow(r)
                except Exception as e:
                    print('ECHEC', futs[fu], repr(e)[:300], flush=True)
                h.flush()
                print('%d/%d' % (i + 1, len(specs)), flush=True)


if __name__ == '__main__':
    main()
