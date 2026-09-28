"""Choix sur DEV (graines dev) pour le panneau apparie K = min_samples (directive utilisateur du 28 septembre 2026).

Pour chaque K de la liste : la tour a l'ordre K avec ses tetes {EOM z = 1, EOM z = zhat, feuilles}, et sklearn
HDBSCAN a min_samples = K avec {EOM, feuilles} x alpha {1, 2} ; mcs = round(sqrt(n)) des deux cotes ; politiques de
bruit none, full, b1.5, b2, b2.5, b3 (distance-coeur au max(K, 5)-ieme voisin), identiques pour les deux methodes.
Sortie : une ligne par (scene, methode, K, tete, politique).
"""
import argparse
import csv
import math
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np

SYN = sys.argv[sys.argv.index('--syn') + 1]
sys.path.insert(0, SYN)
import methods  # noqa: E402
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402

FILLS = ('none', 'full', 'b1.5', 'b2', 'b2.5', 'b3')
TOWER_HEADS = (('eom', '1'), ('eom', 'zhat'), ('leaf', '1'))
SK_HEADS = (('eom', 1.0), ('eom', 2.0), ('leaf', 1.0), ('leaf', 2.0))
COLS = ('family', 'level', 'noise', 'n', 'seed', 'method', 'k', 'head', 'fill', 'ari_s', 'coverage', 'clusters')


def run_unit(spec, build, ks):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    n = len(G)
    mcs = int(round(math.sqrt(n)))
    zh = methods.zhat(G)
    labs = []
    with tempfile.TemporaryDirectory() as tmp:
        src, dst, cf = (os.path.join(tmp, x) for x in ('in', 'out', 'cfg'))
        np.ascontiguousarray(G, dtype='<u4').tofile(src)
        with open(cf, 'w') as f:
            for sel, z in TOWER_HEADS:
                f.write('%d %r %s 0\n' % (mcs, zh if z == 'zhat' else 1.0, sel))
        r = subprocess.run([os.path.join(build, 'mhgp10_cluster'), src, dst, '--k-list=' + ','.join(map(str, ks)),
                            '--threads=1', '--configs=' + cf], capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError(r.stderr)
        for k in ks:
            for i, (sel, z) in enumerate(TOWER_HEADS):
                lab = np.fromfile(dst + '.k%d.%d' % (k, i), dtype='<i4').astype(np.int64)
                labs.append(('tower', k, '%s_z%s' % (sel, z), lab))
    for k in ks:
        for sel, a in SK_HEADS:
            labs.append(('hdb', k, '%s_a%g' % (sel, a), methods.hdbscan_labels(G, k, mcs, sel, a)))
    out = []
    for method, k, head, lab in labs:
        for f in FILLS:
            if f == 'none':
                l2 = lab
            elif f == 'full':
                l2 = methods.fill_noise(G, lab)
            else:
                l2 = methods.bounded_fill(G, lab, max(k, 5), float(f[1:]))
            s = metrics.scores(T, l2)
            out.append(dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=n,
                            seed=spec['seed'], method=method, k=k, head=head, fill=f, ari_s=round(s['ari_s'], 6),
                            coverage=round(s['coverage'], 4), clusters=s['clusters']))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', required=True)
    ap.add_argument('--syn', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--k', default='1,2,3')
    ap.add_argument('--sizes', default='2000,8000')
    ap.add_argument('--jobs', type=int, default=5)
    args = ap.parse_args()
    ks = [int(x) for x in args.k.split(',')]
    specs = run_campaign.plan('dev', [int(s) for s in args.sizes.split(',')], 2)
    print('%d unites, K = %s' % (len(specs), ks), flush=True)
    with open(args.out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            futs = {pool.submit(run_unit, s, args.build, ks): s for s in specs}
            done = 0
            for fu in as_completed(futs):
                done += 1
                try:
                    rows = fu.result()
                except Exception as e:
                    print('ECHEC', futs[fu], repr(e), flush=True)
                    continue
                for r in rows:
                    w.writerow(r)
                h.flush()
                print('%d/%d' % (done, len(specs)), flush=True)


if __name__ == '__main__':
    main()
