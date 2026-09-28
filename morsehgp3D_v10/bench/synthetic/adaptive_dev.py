"""Experience de DEVELOPPEMENT (graines dev seulement) : configuration fixe contre choix PAR SCENE sans etiquettes.

Applique SYMETRIQUEMENT a la tour (ordres K) et a sklearn.cluster.HDBSCAN (min_samples, alpha), pour ne jamais
attribuer a la tour un gain qui viendrait du critere de choix.

Pour chaque unite (scene, graine) et chaque methode : les etiquettes de chaque configuration de sa grille
(mcs = sqrt(n) par defaut), puis
  - fixed_<cfg> : chaque configuration, telle quelle (le choix d'une configuration globale se fait sur dev) ;
  - dbcv        : la configuration de DBCV maximal (Moulavi et al. 2014), calcule sur les etiquettes avant
                  remplissage ; egalites departagees par l'ordre de la grille.
  python3 adaptive_dev.py --build B --out O.csv --sizes 2000,8000 --tower-k 1,2,3,4,5,6,8,10 \
      --hdb-ms 1,2,3,4,5,6,8,10,12,16,20 --alphas 1,2 --jobs 24 --threads 2
"""
import argparse
import csv
import os
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import methods  # noqa: E402
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402

COLS = ('family', 'level', 'noise', 'n', 'seed', 'method', 'fill', 'criterion', 'ari_s', 'ari_nc', 'coverage',
        'clusters', 'chosen', 'dbcv', 'seconds')


def dbcv(X, labels):
    from hdbscan.validity import validity_index
    if len(set(labels.tolist()) - {-1}) < 2:
        return -1.0
    try:
        v = float(validity_index(np.asarray(X, dtype=np.float64), labels.astype(np.int64)))
        return v if np.isfinite(v) else -1.0
    except Exception:
        return -1.0


def tower_configs(ks, zs):
    out = []
    for k in ks:
        for sel in ('eom', 'leaf'):
            for z in (zs if sel == 'eom' else ('1',)):  # z ne change pas la selection par feuilles
                out.append(('tower', k, sel, z))
    return out


def hdb_configs(mss, alphas):
    return [('hdb', ms, sel, a) for ms in mss for sel in ('eom', 'leaf') for a in alphas]


def run_unit(spec, args):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    n = len(G)
    mcs = int(round(np.sqrt(n)))
    zh = methods.zhat(G)
    labels, seconds = {}, {}
    tcfg = tower_configs(args.tower_k, args.zs)
    heads = sorted({(sel, z) for _, _, sel, z in tcfg})
    with tempfile.TemporaryDirectory() as tmp:
        src, dst, cfg = os.path.join(tmp, 'in'), os.path.join(tmp, 'out'), os.path.join(tmp, 'cfg')
        np.ascontiguousarray(G, dtype='<u4').tofile(src)
        with open(cfg, 'w') as f:
            for sel, z in heads:
                f.write('%d %r %s 0\n' % (mcs, 1.0 if z == '1' else zh, sel))
        t0 = time.time()
        r = subprocess.run([os.path.join(args.build, 'mhgp10_cluster'), src, dst,
                            '--k-list=' + ','.join(map(str, args.tower_k)), '--threads=%d' % args.threads,
                            '--configs=' + cfg], capture_output=True, text=True)
        tsec = time.time() - t0
        for c in tcfg:
            _, k, sel, z = c
            if r.returncode != 0:  # refus : etiquettes vides, ARI_s = 0 (jamais omis)
                labels[c] = None
            else:
                i = heads.index((sel, z))
                labels[c] = np.fromfile(dst + '.k%d.%d' % (k, i), dtype='<i4').astype(np.int64)
            seconds[c] = tsec
    for c in hdb_configs(args.hdb_ms, args.alphas):
        _, ms, sel, a = c
        t0 = time.time()
        labels[c] = methods.hdbscan_labels(G, ms, mcs, sel, a)
        seconds[c] = time.time() - t0
    unit = dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=spec['n'], seed=spec['seed'])
    out = []
    for method, grid in (('tower', tcfg), ('hdb', hdb_configs(args.hdb_ms, args.alphas))):
        dv = {c: (dbcv(G, labels[c]) if labels[c] is not None else -2.0) for c in grid}
        pick = max(grid, key=lambda c: (dv[c], -grid.index(c)))
        for fill in args.fills:
            def score(c):
                if labels[c] is None:
                    return dict(ari_s=0.0, ari_nc=0.0, coverage=0.0, clusters=0)
                lab = labels[c] if fill == 'none' else methods.fill_noise(G, labels[c])
                s = metrics.scores(T, lab)
                return dict(ari_s=s['ari_s'], ari_nc=s['ari_nc'], coverage=s['coverage'], clusters=s['clusters'])
            for c in grid:
                out.append(dict(unit, method=method, fill=fill, criterion='fixed_' + '_'.join(map(str, c[1:])),
                                dbcv=round(dv[c], 5), seconds=round(seconds[c], 3), **score(c)))
            out.append(dict(unit, method=method, fill=fill, criterion='dbcv', chosen='_'.join(map(str, pick[1:])),
                            dbcv=round(dv[pick], 5), **score(pick)))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--split', default='dev', choices=('dev',))
    ap.add_argument('--sizes', default='2000')
    ap.add_argument('--replicates', type=int, default=2)
    ap.add_argument('--tower-k', default='1,2,3,4,5')
    ap.add_argument('--zs', default='1,zhat')
    ap.add_argument('--hdb-ms', default='1,2,3,4,5')
    ap.add_argument('--alphas', default='1')
    ap.add_argument('--fills', default='none,full')
    ap.add_argument('--jobs', type=int, default=3)
    ap.add_argument('--threads', type=int, default=2)
    args = ap.parse_args()
    args.tower_k = [int(x) for x in args.tower_k.split(',')]
    args.zs = tuple(args.zs.split(','))
    args.hdb_ms = [int(x) for x in args.hdb_ms.split(',')]
    args.alphas = [float(x) for x in args.alphas.split(',')]
    args.fills = tuple(args.fills.split(','))
    specs = run_campaign.plan(args.split, [int(s) for s in args.sizes.split(',')], args.replicates)
    print('%d unites' % len(specs), flush=True)
    with open(args.out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            futs = {pool.submit(run_unit, s, args): s for s in specs}
            done = 0
            for f in as_completed(futs):
                done += 1
                try:
                    rows = f.result()
                except Exception as e:
                    print('ECHEC', futs[f], repr(e), flush=True)
                    continue
                for r in rows:
                    w.writerow({c: r.get(c, '') for c in COLS})
                h.flush()
                print('%d/%d' % (done, len(specs)), flush=True)


if __name__ == '__main__':
    main()
