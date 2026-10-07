#!/usr/bin/env python3
"""Couverture des campagnes du juge L02 : memes nuages (memes graines), sonde seule (sans juge) ; cumule les tailles
de coquilles etendues, les sauts K-NN et les pas de descente des representants, les fusions par arite."""
import json
import os
import random
import subprocess
import sys
import tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from l02_judge import FAMILIES


def main():
    exe = sys.argv[1]
    specs = [tuple(a.split(':')) for a in sys.argv[2:]]  # graine:nombre:nmin-nmax:kcap
    fams = os.environ['L02_FAMS'].split(',') if os.environ.get('L02_FAMS') else sorted(FAMILIES)
    tot = dict(clouds=0, ext_balls=0, balls=0, knn_jumps_join=0, steps_join=0, resolves_join=0, merges=0, arity_ge3=0,
               ext_births=0, births_pop_gt_k=0, ranks_with_2plus_merges=0, max_K=0, max_n=0, refus=0)
    hist = {}
    armax = 0
    kh = {}
    with tempfile.TemporaryDirectory(dir=os.environ.get('L02_TMP')) as tmp:
        for seed0, count, nr, kcap in specs:
            seed0, count, kcap = int(seed0), int(count), int(kcap)
            nmin, nmax = (int(v) for v in nr.split('-'))
            for t in range(count):
                fam = fams[t % len(fams)]
                rnd = random.Random(1000003 * seed0 + t)
                n = rnd.randint(nmin, nmax)
                P = FAMILIES[fam](rnd, n)
                rnd.shuffle(P)
                K = min(kcap, n) if rnd.random() < 0.8 else min(n, 12, kcap + 2)
                src = os.path.join(tmp, 'in.u32le')
                with open(src, 'wb') as f:
                    for p in P:
                        for v in p:
                            f.write(int(v).to_bytes(4, 'little'))
                r = subprocess.run([exe, src, '--k=%d' % K, '--kcat=%d' % min(12, K + 2), '--threads=1'],
                                   capture_output=True, text=True)
                if r.returncode != 0:
                    tot['refus'] += 1
                    continue
                d = json.loads(r.stdout)
                tot['clouds'] += 1
                tot['balls'] += d['balls']
                tot['ext_balls'] += d['ext_balls']
                tot['max_K'] = max(tot['max_K'], K)
                tot['max_n'] = max(tot['max_n'], n)
                kh[K] = kh.get(K, 0) + 1
                for m, c in d['ext_m_hist'].items():
                    hist[int(m)] = hist.get(int(m), 0) + c
                for o in d['orders']:
                    for key in ('knn_jumps_join', 'steps_join', 'resolves_join', 'merges', 'arity_ge3', 'ext_births',
                                'births_pop_gt_k', 'ranks_with_2plus_merges'):
                        tot[key] += o[key]
                    armax = max(armax, o['arity_max'])
    tot['arity_max'] = armax
    tot['ext_m_hist'] = {str(k): hist[k] for k in sorted(hist)}
    tot['K_hist'] = {str(k): kh[k] for k in sorted(kh)}
    print(json.dumps(tot, sort_keys=True))


if __name__ == '__main__':
    main()
