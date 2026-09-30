"""Verification adverse de L06-F2 : couche clustering v9 a K=1 contre sklearn HDBSCAN(min_samples=1).

Variantes (construites depuis git, pas depuis les copies de l'auditeur) :
  head      = cluster.py de ce8a649dd (HEAD)
  prefix    = cluster.py de cda636b5e (commit revendicateur)
  patched   = head avec `elif child in big:` (ligne 210)
  wt        = worktree non commis (allow_single_cluster=False dans l'EOM)
  wtpatched = wt + meme correctif
Un export natif par nuage, partage par toutes les variantes.
"""
import importlib.util
import json
import os
import subprocess
import sys

import numpy as np
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score

HERE = os.path.dirname(os.path.abspath(__file__))
BIN = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
VARIANTS = ('head', 'prefix', 'patched', 'wt', 'wtpatched')


def load(variant):
    mods = {}
    for name in ('measure', 'cluster'):
        spec = importlib.util.spec_from_file_location('%s_%s' % (name, variant),
                                                      os.path.join(HERE, variant, name + '.py'))
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        mods[name] = mod
    return mods['cluster'], mods['measure']


def cloud(seed, n, groups):
    # meme generateur que l'auditeur, pour rejouer ses 12 cas
    rng = np.random.default_rng(seed)
    centers = rng.uniform(0, 4000, size=(groups, 3))
    sizes = rng.multinomial(n, rng.dirichlet(np.ones(groups) * 2))
    pts = np.vstack([rng.normal(c, rng.uniform(60, 250), size=(s, 3)) for c, s in zip(centers, sizes)])
    pts = np.clip(np.rint(pts), 0, 262143).astype(np.int64)
    _, idx = np.unique(pts, axis=0, return_index=True)
    return pts[np.sort(idx)]


def main():
    seeds = int(sys.argv[1]) if len(sys.argv) > 1 else 6
    mods = {v: load(v) for v in VARIANTS}
    path = os.path.join(HERE, 'cloud.u32le')
    tally = {v: 0 for v in VARIANTS}
    total = 0
    for seed in range(seeds):
        pts = cloud(seed, 1500, 3 + seed % 4)
        with open(path, 'wb') as fh:
            fh.write(pts.astype('<u4').tobytes())
        out = subprocess.run([BIN, '--input', path, '--k', '1', '--workers', '1'],
                             capture_output=True, text=True)
        if out.returncode:
            print('seed', seed, 'EXPORT REFUSED', out.stderr[:200], flush=True)
            continue
        rep = json.loads(out.stdout)
        for mcs in (10, 25):
            total += 1
            h = HDBSCAN(min_cluster_size=mcs, min_samples=1, cluster_selection_method='eom',
                        allow_single_cluster=False).fit(pts.astype(float)).labels_
            line = dict(seed=seed, mcs=mcs, n=len(pts), hdb=(int(h.max() + 1), int((h < 0).sum())))
            for v in VARIANTS:
                C, M = mods[v]
                cof, gab, size = M.read_export(rep)
                births = M.facet_births(cof, gab, 'boundary')
                if v == 'head' and mcs == 10:
                    line['birth_min'] = float(min(births.values()))
                    line['n_births'] = len(births)
                    line['gab_singletons'] = len(gab)
                facets, pl = C.facet_levels(cof, None)
                nodes, roots = C.merge_tree(facets, pl, births)
                sums, totals, masses, _ = M.measure(cof, gab, 1, 'boundary')
                if v == 'head' and mcs == 10:
                    line['mass_set'] = sorted({float(x) for x in masses.values()})[:3]
                cl, order = C.condense(nodes, roots, masses, births, mcs, 'radius', 1)
                sel = C.select_excess_of_mass(cl, order)
                lab = np.asarray(C.vote(len(pts), C.label_facets(cl, sel), sums, totals, len(sel))[0])
                ok = adjusted_rand_score(lab, h) == 1.0 and bool(((lab < 0) == (h < 0)).all())
                tally[v] += ok
                line[v] = (int(lab.max() + 1), int((lab < 0).sum()), round(adjusted_rand_score(lab, h), 4),
                           'OK' if ok else 'DIFF')
            print(json.dumps(line), flush=True)
    print('exact matches over', total, json.dumps(tally))


if __name__ == '__main__':
    main()
