"""Choix sur DEV (graines dev) pour le panneau apparie K = min_samples (directive utilisateur du 28 septembre 2026).

Variante : --entry cap|cover choisit la semantique d'entree des points de la tour (C n X ou premiere couverture,
prototype) ; --no-hdb saute sklearn.

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
ENTRY = sys.argv[sys.argv.index('--entry') + 1] if '--entry' in sys.argv else 'cap'
HDB = '--no-hdb' not in sys.argv
TOWER_HEADS = (('eom', '1'), ('eom', 'zhat'), ('leaf', '1'))
if '--heads' in sys.argv:  # ex. --heads eom:2,eom:3 (remplace la grille de tetes de la tour)
    TOWER_HEADS = tuple(tuple(h.split(':')) for h in sys.argv[sys.argv.index('--heads') + 1].split(','))
VOTE = '--no-vote' not in sys.argv
EXTRA = int(sys.argv[sys.argv.index('--cover-extra') + 1]) if '--cover-extra' in sys.argv else 0
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
                f.write('%d %r %s 0\n' % (mcs, zh if z == 'zhat' else float(z), sel))
        r = subprocess.run([os.path.join(build, 'mhgp10_cluster'), src, dst, '--k-list=' + ','.join(map(str, ks)),
                            '--threads=1', '--configs=' + cf] + (['--entry=cover'] + (['--cover-extra=%d' % EXTRA] if EXTRA else [])
                            + (['--label=vote'] if VOTE else []) if ENTRY == 'cover' else []),
                           capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError(r.stderr)
        for k in ks:
            for i, (sel, z) in enumerate(TOWER_HEADS):
                lab = np.fromfile(dst + '.k%d.%d' % (k, i), dtype='<i4').astype(np.int64)
                labs.append(('tower_' + ENTRY + (str(EXTRA) if EXTRA else ''), k, '%s_z%s' % (sel, z), lab))
                if ENTRY == 'cover' and os.path.exists(dst + '.k%d.%d.vote' % (k, i)):
                    vlab = np.fromfile(dst + '.k%d.%d.vote' % (k, i), dtype='<i4').astype(np.int64)
                    labs.append(('tower_coverVote', k, '%s_z%s' % (sel, z), vlab))
    for k in (ks if HDB else ()):
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
    ap.add_argument('--entry', default='cap', choices=('cap', 'cover'))
    ap.add_argument('--no-hdb', action='store_true')
    ap.add_argument('--no-vote', action='store_true')
    ap.add_argument('--heads', default=None)
    ap.add_argument('--cover-extra', type=int, default=0)
    args = ap.parse_args()
    ks = [int(x) for x in args.k.split(',')]
    specs = run_campaign.plan('dev', [int(s) for s in args.sizes.split(',')], 2)
    import hashlib
    sha = lambda p: hashlib.sha256(open(p, 'rb').read()).hexdigest()
    print('%d unites, K = %s ; mhgp10_cluster %s ; script %s' % (len(specs), ks,
          sha(os.path.join(args.build, 'mhgp10_cluster')), sha(os.path.abspath(__file__))), flush=True)
    with open(args.out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            futs = {pool.submit(run_unit, s, args.build, ks): s for s in specs}
            done = failures = 0
            for fu in as_completed(futs):
                done += 1
                try:
                    rows = fu.result()
                except Exception as e:  # jamais silencieux : compte, et code de sortie non nul en fin de run
                    failures += 1
                    print('ECHEC', futs[fu], repr(e), flush=True)
                    continue
                for r in rows:
                    w.writerow(r)
                h.flush()
                print('%d/%d' % (done, len(specs)), flush=True)
    print('ECHECS %d' % failures, flush=True)
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
