"""Campagne du banc v10 : tete sur la tour (toutes les configurations sur une construction par K) contre
sklearn.cluster.HDBSCAN (grille), sur des graines d'un espace declare (dev ou test, disjoints).

Chaque ligne du CSV : scene, famille, niveau, n, bruit, graine, methode, parametres, metriques, temps.
Aucun choix par scene : le choix d'une configuration unique se fait ensuite, sur dev seulement (select.py).

  python3 run_campaign.py --build BUILD --out OUT.csv --split dev --sizes 2000,8000 --jobs 3 --threads 2
"""
import argparse
import csv
import hashlib
import json
import math
import os
import sys
import time
import traceback
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import methods  # noqa: E402
import metrics  # noqa: E402
import scenes  # noqa: E402

TOWER_K = (1, 2, 3, 4, 5)
HDB_MS = (1, 2, 3, 5, 8, 12, 20)
MCS = (10, 20, 50, 'sqrt')
SELECTIONS = ('eom', 'leaf')
ALPHAS = (1.0, 2.0)
FILLS = ('none', 'full')
COLUMNS = ('scene', 'family', 'level', 'n', 'noise', 'seed', 'points', 'method', 'k', 'mcs', 'z', 'selection', 'alpha',
           'fill', 'ari_s', 'ari_nc', 'ami_nc', 'coverage', 'clusters', 'seconds', 'zhat', 'duplicates')


def seed_of(split, spec, replicate):
    """Graine derivee de la specification et de l'espace (dev/test) : espaces disjoints par construction."""
    text = json.dumps(dict(spec, split=split, replicate=replicate), sort_keys=True)
    return int(hashlib.sha256(text.encode()).hexdigest()[:15], 16)


def plan(split, sizes, replicates, noises=(0.0, 0.1)):
    rows = []
    for family in scenes.FAMILIES:
        for level in scenes.LEVELS:
            for n in sizes:
                for noise in noises:
                    base = dict(family=family, n=n, groups=8, level=level, noise_fraction=noise)
                    for r in range(replicates):
                        spec = dict(base, seed=seed_of(split, base, r))
                        rows.append(spec)
    return rows


def mcs_value(m, n):
    return int(round(math.sqrt(n))) if m == 'sqrt' else int(m)


def run_scene(spec, build, threads):
    out = []
    P, L, meta = scenes.generate(spec)
    G, T, dups, _ = scenes.quantize18(P, L)
    n = len(G)
    zh = methods.zhat(G)
    common = dict(scene='%s_n%d_%s_nu%g' % (spec['family'], spec['n'], spec['level'], spec['noise_fraction']),
                  family=spec['family'], level=spec['level'], n=spec['n'], noise=spec['noise_fraction'],
                  seed=spec['seed'], points=n, zhat=round(zh, 4), duplicates=dups)

    def emit(method, k, m, z, sel, alpha, fill, labels, seconds):
        for f in FILLS:
            lab = labels if f == 'none' else methods.fill_noise(G, labels)
            row = dict(common, method=method, k=k, mcs=m, z=z, selection=sel, alpha=alpha, fill=f, seconds=round(seconds, 4))
            row.update({key: round(v, 6) if isinstance(v, float) else v for key, v in metrics.scores(T, lab).items()})
            out.append(row)
    # tour : un catalogue a Kmax, chaque ordre de TOWER_K, toutes les tetes sur chaque ordre
    import subprocess
    import tempfile
    configs = [(m, z, sel) for m in MCS for z in ('1', 'zhat') for sel in SELECTIONS]
    with tempfile.TemporaryDirectory() as tmp:
        src, dst, cfg = os.path.join(tmp, 'in'), os.path.join(tmp, 'out'), os.path.join(tmp, 'cfg')
        np.ascontiguousarray(G, dtype='<u4').tofile(src)
        with open(cfg, 'w') as f:
            for m, z, sel in configs:
                f.write('%d %r %s 0\n' % (mcs_value(m, n), 1.0 if z == '1' else zh, sel))
        t0 = time.time()
        r = subprocess.run([os.path.join(build, 'mhgp10_cluster'), src, dst, '--k-list=' + ','.join(map(str, TOWER_K)),
                            '--threads=%d' % threads, '--configs=' + cfg], capture_output=True, text=True)
        dt = time.time() - t0
        for k in TOWER_K:
            for i, (m, z, sel) in enumerate(configs):
                if r.returncode != 0:  # refus compte comme defaite (ARI_s = 0), jamais omis
                    for f in FILLS:
                        out.append(dict(common, method='tower', k=k, mcs=m, z=z, selection=sel, alpha='', fill=f,
                                        ari_s=0.0, ari_nc=0.0, ami_nc=0.0, coverage=0.0, clusters=0, seconds=dt))
                    continue
                lab = np.fromfile(dst + '.k%d.%d' % (k, i), dtype='<i4').astype(np.int64)
                emit('tower', k, m, z, sel, '', None, lab, dt / (len(configs) * len(TOWER_K)))
    # sklearn HDBSCAN : grille
    for ms in HDB_MS:
        for m in MCS:
            for sel in SELECTIONS:
                for alpha in ALPHAS:
                    t0 = time.time()
                    lab = methods.hdbscan_labels(G, ms, mcs_value(m, n), sel, alpha)
                    emit('hdbscan', ms, m, '', sel, alpha, None, lab, time.time() - t0)
    # sklearn par defaut (min_cluster_size=5, min_samples=None)
    from sklearn.cluster import HDBSCAN
    t0 = time.time()
    lab = HDBSCAN(copy=True).fit(np.asarray(G, dtype=np.float64)).labels_.astype(np.int64)
    emit('hdbscan_default', '', 5, '', 'eom', 1.0, None, lab, time.time() - t0)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--split', choices=('dev', 'test'), required=True)
    ap.add_argument('--sizes', default='2000,8000')
    ap.add_argument('--replicates', type=int, default=2)
    ap.add_argument('--jobs', type=int, default=3)
    ap.add_argument('--threads', type=int, default=2)
    args = ap.parse_args()
    sizes = [int(s) for s in args.sizes.split(',')]
    specs = plan(args.split, sizes, args.replicates)
    print('%d scenes' % len(specs), flush=True)
    with open(args.out, 'w', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=COLUMNS)
        writer.writeheader()
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            futures = {pool.submit(run_scene, s, args.build, args.threads): s for s in specs}
            done = 0
            for fut in as_completed(futures):
                s = futures[fut]
                done += 1
                try:
                    rows = fut.result()
                except Exception:
                    print('ECHEC %s' % s, traceback.format_exc(), flush=True)
                    continue
                for row in rows:
                    writer.writerow({c: row.get(c, '') for c in COLUMNS})
                handle.flush()
                best_t = max((r['ari_s'] for r in rows if r['method'] == 'tower'), default=0)
                best_h = max((r['ari_s'] for r in rows if r['method'] == 'hdbscan'), default=0)
                print('%4d/%d %s n=%d %s nu=%g  oracle tour %.3f  oracle hdb %.3f' % (
                    done, len(specs), s['family'], s['n'], s['level'], s['noise_fraction'], best_t, best_h), flush=True)


if __name__ == '__main__':
    main()
