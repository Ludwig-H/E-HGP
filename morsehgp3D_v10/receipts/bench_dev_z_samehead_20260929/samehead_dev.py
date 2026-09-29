"""DEV (diagnostic d'audit) : MEME TETE sur la hierarchie de la tour et sur les hierarchies d'atteignabilite
mutuelle MR1 et MR2 (l'objet d'HDBSCAN, alpha = 1 et 2 ; temoin mhgp10_mreach_cluster, egal a sklearn aux
egalites de plateau pres). Isole l'apport de la hierarchie (et de l'entree des points) de celui de la tete.
Tetes : EOM z = 1, 3, 4 et feuilles ; mcs = round(sqrt(n)) ; politiques de bruit du banc. Graines dev seulement.
La tour (entree cover) est lue dans kcover_dev_zgrid.csv et kcover_dev_cover*.csv (memes scenes, memes tetes).
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
import methods  # noqa: E402
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402

EXE = '/workspaces/E-HGP/build/v10-dev-mreach/mhgp10_mreach_cluster'
HEADS = (('eom', 1.0), ('eom', 3.0), ('eom', 4.0), ('leaf', 1.0))
KS = (1, 2, 3, 5, 8, 10)
FILLS = ('none', 'full', 'b1.5', 'b2', 'b2.5', 'b3')
COLS = ('family', 'level', 'noise', 'n', 'seed', 'method', 'k', 'head', 'fill', 'ari_s', 'coverage', 'clusters')


def run_unit(spec):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    n = len(G)
    mcs = int(round(math.sqrt(n)))
    out = []
    with tempfile.TemporaryDirectory() as tmp:
        src, dst, cf = (os.path.join(tmp, x) for x in ('in', 'out', 'cfg'))
        np.ascontiguousarray(G, dtype='<u4').tofile(src)
        with open(cf, 'w') as f:
            for sel, z in HEADS:
                f.write('%d %r %s 0\n' % (mcs, z, sel))
        for k in KS:
            for alpha in (1, 2):
                r = subprocess.run([EXE, src, dst, '--source=mreach', '--k=%d' % k, '--configs=' + cf,
                                    '--alpha=%d' % alpha, '--threads=2'], capture_output=True, text=True)
                if r.returncode != 0:
                    raise RuntimeError(r.stderr)
                for i, (sel, z) in enumerate(HEADS):
                    lab = np.fromfile(dst + '.%d' % i, dtype='<i4').astype(np.int64)
                    for fl in FILLS:
                        if fl == 'none':
                            l2 = lab
                        elif fl == 'full':
                            l2 = methods.fill_noise(G, lab)
                        else:
                            l2 = methods.bounded_fill(G, lab, max(k, 5), float(fl[1:]))
                        s = metrics.scores(T, l2)
                        out.append(dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=n,
                                        seed=spec['seed'], method='mr%d' % alpha, k=k,
                                        head='%s_z%g' % (sel, z), fill=fl, ari_s=round(s['ari_s'], 6),
                                        coverage=round(s['coverage'], 4), clusters=s['clusters']))
    return out


def main():
    specs = run_campaign.plan('dev', [2000, 8000], 2)
    with open(sys.argv[1], 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=int(sys.argv[2])) as pool:
            futs = [pool.submit(run_unit, s) for s in specs]
            for i, fu in enumerate(as_completed(futs)):
                for r in fu.result():
                    w.writerow(r)
                h.flush()
                print('%d/%d' % (i + 1, len(specs)), flush=True)


if __name__ == '__main__':
    main()
