"""DEV (graines dev) : la tour perd sur `anisotropic` apres remplissage alors qu'elle domine sans remplissage. Hypothese :
les frontieres des amas EOM (jusqu'au col) affectent moins bien que le Voronoi de coeurs etroits (feuilles de sklearn).
Variantes de la tour a K = 3 et 10, entrees core et cover :
  eom (z du lot C), leaf, eom retreci aux p % les plus denses de chaque amas (p = 50, 70 ; densite = core a max(K,5)),
chacune suivie de : full (plus proche classe) et b2.5 avec le Q95 de l'amas AVANT retrecissement (couverture comparable).
Reference : sklearn feuilles alpha = 2 (lot C) avec les memes remplissages.
  python3 shrink_dev.py BUILD OUT.csv JOBS [tailles]"""
import csv
import math
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np
from scipy.spatial import cKDTree

SYN = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/bench/synthetic'
sys.path.insert(0, SYN)
import methods  # noqa: E402
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402

KS = (3, 10)
Z = {3: 5, 10: 6}
COLS = ('family', 'level', 'noise', 'n', 'seed', 'k', 'variant', 'fill', 'ari_s', 'coverage', 'clusters')


def shrink(lab, core, p):
    out = lab.copy()
    for c in np.unique(lab[lab >= 0]):
        idx = np.flatnonzero(lab == c)
        thr = np.quantile(core[idx], p)
        out[idx[core[idx] > thr]] = -1
    return out


def fill_ref(X, lab_core, lab_ref, core, rho):
    """Plus proche point classe de lab_core ; rejet b(rho) avec le Q95 de l'amas dans lab_ref (avant retrecissement).
    rho = None : remplissage complet."""
    out = lab_core.copy()
    noise = out < 0
    if noise.all() or not noise.any():
        return out
    _, j = cKDTree(X[~noise]).query(X[noise])
    cand = out[~noise][j]
    idx = np.flatnonzero(noise)
    if rho is None:
        out[idx] = cand
        return out
    q95 = {c: float(np.quantile(core[lab_ref == c], 0.95)) for c in np.unique(lab_ref[lab_ref >= 0])}
    ok = core[idx] <= rho * np.array([q95[c] for c in cand])
    out[idx[ok]] = cand[ok]
    return out


def run_unit(spec, build):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    n = len(G)
    mcs = int(round(math.sqrt(n)))
    X = np.asarray(G, dtype=np.float64)
    tree = cKDTree(X)
    rows = []
    zs = sorted(set(Z.values()))
    cfg = [(mcs, z, 'eom', False) for z in zs] + [(mcs, 6, 'leaf', False)]
    batch = methods.tower_labels_batch(build, G, KS, ['core', 'cover'], cfg, threads=1)
    for k in KS:
        core = tree.query(X, k=max(k, 5))[0][:, -1]
        heads = []
        for e in ('core', 'cover'):
            eom = batch[(e, k, zs.index(Z[k]))]
            heads.append(('tour_%s_eom' % e, eom, eom))
            heads.append(('tour_%s_leaf' % e, batch[(e, k, len(zs))], batch[(e, k, len(zs))]))
            for p in (0.5, 0.7):
                heads.append(('tour_%s_eom_s%d' % (e, int(p * 100)), shrink(eom, core, p), eom))
        sk = methods.hdbscan_labels(G, k, mcs, 'leaf', 2.0)
        heads.append(('sklearn_leaf', sk, sk))
        for name, lab, ref in heads:
            for fill, rho in (('none', 0), ('full', None), ('b2.5', 2.5)):
                pred = lab if fill == 'none' else fill_ref(X, lab, ref, core, rho)
                s = metrics.scores(T, pred)
                rows.append(dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=n,
                                 seed=spec['seed'], k=k, variant=name, fill=fill, ari_s=round(s['ari_s'], 6),
                                 coverage=round(s['coverage'], 4), clusters=s['clusters']))
    return rows


def main():
    build, out, jobs = sys.argv[1], sys.argv[2], int(sys.argv[3])
    sizes = [int(x) for x in (sys.argv[4] if len(sys.argv) > 4 else '8000').split(',')]
    specs = run_campaign.plan('dev', sizes, 2)
    fails = 0
    with open(out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=jobs) as pool:
            futs = {pool.submit(run_unit, s, build): s for s in specs}
            for i, fu in enumerate(as_completed(futs)):
                try:
                    w.writerows(fu.result())
                except Exception as e:
                    fails += 1
                    print('ECHEC', futs[fu], repr(e)[:200], flush=True)
                h.flush()
    print('SCENES %d ECHECS %d' % (len(specs), fails), flush=True)


if __name__ == '__main__':
    main()
