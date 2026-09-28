"""Etape 2 du choix sur DEV (graines dev) : les 8 meilleures configurations distinctes de chaque methode (reçu dev_r2,
meilleure politique none/full) croisees avec les politiques de bruit none, full, b1.5, b2, b2.5, b3, applique
symetriquement a la tour et a sklearn HDBSCAN. b(rho) : methods.bounded_fill avec core au max(K, 5)-ieme voisin."""
import argparse
import csv
import json
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
COLS = ('family', 'level', 'noise', 'n', 'seed', 'method', 'config', 'fill', 'ari_s', 'coverage', 'clusters')


def distinct(cfgs, n=8):
    seen, out = set(), []
    for c in cfgs:
        key = (c[1], c[2], c[3] if c[4] == 'eom' else '-', c[4], c[5])
        if key in seen:
            continue
        seen.add(key)
        out.append(c)
    return out[:n]


def run_unit(spec, build, cfgs):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    n = len(G)
    zh = methods.zhat(G)

    def mcs_of(m):
        return int(round(math.sqrt(n))) if m == 'sqrt' else int(m)
    labs = []
    tw = cfgs['tower']
    heads = sorted({(c[2], c[3] if c[4] == 'eom' else '1', c[4]) for c in tw})
    ks = sorted({int(c[1]) for c in tw})
    with tempfile.TemporaryDirectory() as tmp:
        src, dst, cf = (os.path.join(tmp, x) for x in ('in', 'out', 'cfg'))
        np.ascontiguousarray(G, dtype='<u4').tofile(src)
        with open(cf, 'w') as f:
            for m, z, sel in heads:
                f.write('%d %r %s 0\n' % (mcs_of(m), zh if z == 'zhat' else 1.0, sel))
        r = subprocess.run([os.path.join(build, 'mhgp10_cluster'), src, dst, '--k-list=' + ','.join(map(str, ks)),
                            '--threads=1', '--configs=' + cf], capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError(r.stderr)
        for c in tw:
            i = heads.index((c[2], c[3] if c[4] == 'eom' else '1', c[4]))
            lab = np.fromfile(dst + '.k%s.%d' % (c[1], i), dtype='<i4').astype(np.int64)
            labs.append(('tower', '_'.join(c[1:5]), int(c[1]), lab))
    for c in cfgs['hdb']:
        lab = methods.hdbscan_labels(G, int(c[1]), mcs_of(c[2]), c[4], float(c[5]))
        labs.append(('hdb', '_'.join([c[1], c[2], c[4], c[5]]), int(c[1]), lab))
    out = []
    for method, name, k, lab in labs:
        for f in FILLS:
            if f == 'none':
                l2 = lab
            elif f == 'full':
                l2 = methods.fill_noise(G, lab)
            else:
                l2 = methods.bounded_fill(G, lab, max(k, 5), float(f[1:]))
            s = metrics.scores(T, l2)
            out.append(dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=n,
                            seed=spec['seed'], method=method, config=name, fill=f, ari_s=round(s['ari_s'], 6),
                            coverage=round(s['coverage'], 4), clusters=s['clusters']))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', required=True)
    ap.add_argument('--syn', required=True)
    ap.add_argument('--configs', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--jobs', type=int, default=5)
    args = ap.parse_args()
    raw = json.load(open(args.configs))
    cfgs = {m: distinct(raw[m]) for m in ('tower', 'hdb')}
    print(json.dumps(cfgs), flush=True)
    specs = run_campaign.plan('dev', [2000, 8000], 2)
    print('%d unites' % len(specs), flush=True)
    with open(args.out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            futs = {pool.submit(run_unit, s, args.build, cfgs): s for s in specs}
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
