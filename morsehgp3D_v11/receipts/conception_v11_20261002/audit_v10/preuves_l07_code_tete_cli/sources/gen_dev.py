"""Audit L07 : scenes de l'espace `dev` du banc v10 (memes generateur, quantification et graines que
bench/synthetic/run_campaign.py --split dev), ecrites en u32le avec la verite et zhat. Aucune graine `test`."""
import json
import os
import sys

import numpy as np

BENCH = '/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10/bench/synthetic'
sys.path.insert(0, BENCH)
import methods  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402


def main():
    out, sizes, replicates = sys.argv[1], [int(s) for s in sys.argv[2].split(',')], int(sys.argv[3])
    os.makedirs(out, exist_ok=True)
    specs = run_campaign.plan('dev', sizes, replicates)
    index = []
    for i, spec in enumerate(specs):
        P, L, _ = scenes.generate(spec)
        G, T, dups, _ = scenes.quantize18(P, L)
        name = 's%04d_%s_n%d_%s_nu%g' % (i, spec['family'], spec['n'], spec['level'], spec['noise_fraction'])
        np.ascontiguousarray(G, dtype='<u4').tofile(os.path.join(out, name + '.u32le'))
        np.asarray(T, dtype='<i4').tofile(os.path.join(out, name + '.truth.i32le'))
        index.append(dict(spec, name=name, points=int(len(G)), duplicates=int(dups), zhat=float(methods.zhat(G))))
    with open(os.path.join(out, 'index.json'), 'w') as f:
        json.dump(index, f, indent=1)
    print('%d scenes' % len(index))


if __name__ == '__main__':
    main()
