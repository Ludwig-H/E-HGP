"""Experience de DEVELOPPEMENT (graines dev seulement) : choix de configuration PAR SCENE, sans etiquettes.

Question : un critere non supervise qui choisit K (ou la configuration) scene par scene bat-il la meilleure
configuration globale fixee sur dev ? Applique SYMETRIQUEMENT a la tour (ordres K) et a sklearn HDBSCAN
(min_samples = K), pour ne pas attribuer a la tour un gain qui viendrait du critere.

Criteres :
  - fixed        : configuration globale (K, selection, remplissage) donnee en argument ;
  - kstab        : K* = argmax_K de l'accord moyen (ARI) avec les K voisins, a selection et remplissage fixes ;
  - dbcv         : configuration de DBCV maximal (Moulavi et al. 2014, calcule sur les etiquettes avant remplissage).
Sortie : une ligne par (unite, methode, critere) et un resume.
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
from sklearn.metrics import adjusted_rand_score

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import methods  # noqa: E402
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402

KS = (1, 2, 3, 4, 5)
SELS = ('eom', 'leaf')


def dbcv(X, labels):
    from hdbscan.validity import validity_index
    if len(set(labels.tolist()) - {-1}) < 2:
        return -1.0
    try:
        return float(validity_index(np.asarray(X, dtype=np.float64), labels.astype(np.int64)))
    except Exception:
        return -1.0


def kstab_choice(labs):
    """labs : dict K -> etiquettes ; rend K* maximisant l'ARI moyen avec les K voisins."""
    ks = sorted(labs)
    best, bk = -2.0, ks[0]
    for i, k in enumerate(ks):
        nb = [ks[j] for j in (i - 1, i + 1) if 0 <= j < len(ks)]
        s = np.mean([adjusted_rand_score(labs[k], labs[j]) for j in nb])
        if s > best + 1e-12:
            best, bk = s, k
    return bk


def run_unit(spec, build, threads):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    n = len(G)
    mcs = int(round(np.sqrt(n)))
    labels = {}  # (method, K, sel) -> etiquettes avant remplissage
    with tempfile.TemporaryDirectory() as tmp:
        src, dst, cfg = os.path.join(tmp, 'in'), os.path.join(tmp, 'out'), os.path.join(tmp, 'cfg')
        np.ascontiguousarray(G, dtype='<u4').tofile(src)
        with open(cfg, 'w') as f:
            for sel in SELS:
                f.write('%d 1.0 %s 0\n' % (mcs, sel))
        r = subprocess.run([os.path.join(build, 'mhgp10_cluster'), src, dst, '--k-list=' + ','.join(map(str, KS)),
                            '--threads=%d' % threads, '--configs=' + cfg], capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError(r.stdout + r.stderr)
        for k in KS:
            for i, sel in enumerate(SELS):
                labels[('tower', k, sel)] = np.fromfile(dst + '.k%d.%d' % (k, i), dtype='<i4').astype(np.int64)
    for k in KS:
        for sel in SELS:
            labels[('hdb', k, sel)] = methods.hdbscan_labels(G, k, mcs, sel, 1.0)
    out = []
    unit = dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=spec['n'], seed=spec['seed'])
    dcache = {}
    for method in ('tower', 'hdb'):
        for fill in ('none', 'full'):
            def final(k, sel):
                lab = labels[(method, k, sel)]
                return lab if fill == 'none' else methods.fill_noise(G, lab)
            for sel in SELS:
                # tous les K fixes (le choix global se fait ensuite sur dev)
                for k in KS:
                    out.append(dict(unit, method=method, fill=fill, criterion='fixed_k%d_%s' % (k, sel),
                                    ari_s=metrics.scores(T, final(k, sel))['ari_s']))
                ks = kstab_choice({k: labels[(method, k, sel)] for k in KS})
                out.append(dict(unit, method=method, fill=fill, criterion='kstab_%s' % sel,
                                ari_s=metrics.scores(T, final(ks, sel))['ari_s'], chosen=ks))
            best, bcfg = -2.0, None
            for k in KS:
                for sel in SELS:
                    key = (method, k, sel)
                    if key not in dcache:
                        dcache[key] = dbcv(G, labels[key])
                    if dcache[key] > best:
                        best, bcfg = dcache[key], (k, sel)
            out.append(dict(unit, method=method, fill=fill, criterion='dbcv',
                            ari_s=metrics.scores(T, final(*bcfg))['ari_s'], chosen='%d_%s' % bcfg))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--sizes', default='2000')
    ap.add_argument('--jobs', type=int, default=3)
    ap.add_argument('--threads', type=int, default=2)
    args = ap.parse_args()
    specs = run_campaign.plan('dev', [int(s) for s in args.sizes.split(',')], 2)
    cols = ('family', 'level', 'noise', 'n', 'seed', 'method', 'fill', 'criterion', 'ari_s', 'chosen')
    with open(args.out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=cols)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            futs = {pool.submit(run_unit, s, args.build, args.threads): s for s in specs}
            done = 0
            for f in as_completed(futs):
                done += 1
                try:
                    rows = f.result()
                except Exception as e:  # compte comme echec visible
                    print('ECHEC', futs[f], e, flush=True)
                    continue
                for r in rows:
                    w.writerow({c: r.get(c, '') for c in cols})
                h.flush()
                print('%d/%d' % (done, len(specs)), flush=True)


if __name__ == '__main__':
    main()
