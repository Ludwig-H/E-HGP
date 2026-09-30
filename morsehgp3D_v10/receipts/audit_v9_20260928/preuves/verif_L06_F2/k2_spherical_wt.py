"""Effet du defaut residuel (cluster.py:210) dans le reglage de la revendication de cda636b5e :
famille spherical, K=2, z=1, convention gabriel (defaut de run_tower), mode radius, masse sqrt(n),
EOM. Un export par scene, partage par les variantes ; apparie au recu HDBSCAN 22cb1895b."""
import csv
import importlib.util
import json
import math
import os
import subprocess
import sys
import tempfile
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
TREE = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9'
sys.path.insert(0, os.path.join(TREE, 'experiments', 'synthetic_bench_20260928'))
import baselines                      # noqa: E402
import bench_datasets as data         # noqa: E402
import plan as bench_plan             # noqa: E402

BIN = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
VARIANTS = ('wt', 'wtpatched')


def load(variant):
    mods = {}
    for name in ('measure', 'cluster'):
        spec = importlib.util.spec_from_file_location('%s_%s' % (name, variant),
                                                      os.path.join(HERE, variant, name + '.py'))
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        mods[name] = mod
    return mods['cluster'], mods['measure']


def labels_for(C, M, report, npts, mass):
    cofaces, gabriel, size = M.read_export(report)
    births = M.facet_births(cofaces, gabriel, 'gabriel')
    facets, plateaus = C.facet_levels(cofaces, gabriel)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, 'gabriel')
    clusters, order = C.condense(nodes, roots, masses, births, mass, 'radius', 1)
    sel = C.select_excess_of_mass(clusters, order)
    lab, _ = C.vote(npts, C.label_facets(clusters, sel), sums, totals, len(sel))
    return np.asarray(lab)


def main():
    seeds = int(sys.argv[1]) if len(sys.argv) > 1 else 2
    max_n = int(sys.argv[2]) if len(sys.argv) > 2 else 2000
    limit = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    mods = {v: load(v) for v in VARIANTS}
    base = {}
    with open(os.path.join(TREE, 'receipts/synthetic_bench_20260928/r1/baselines.csv')) as fh:
        for row in csv.DictReader(fh):
            base[(row['scene'], row['seed'], row['method'])] = float(row['ari'])
    specs = [s for s in bench_plan.specifications(seeds=seeds, heavy=False)
             if s['family'] == 'spherical' and s['n'] <= max_n]
    if limit:
        specs = specs[:limit]
    print('specs', len(specs), flush=True)
    out = open(os.path.join(HERE, 'k2_spherical_wt_s%d_n%d.jsonl' % (seeds, max_n)), 'w')
    with tempfile.TemporaryDirectory(dir=HERE) as keep:
        for i, spec in enumerate(specs):
            points, truth, meta = data.generate({k: spec[k] for k in data.SPEC_KEYS})
            grid, _ = data.quantize(points)
            path = os.path.join(keep, 'c.u32le')
            with open(path, 'wb') as fh:
                fh.write(np.ascontiguousarray(grid, dtype='<u4').tobytes())
            t0 = time.monotonic()
            done = subprocess.run([BIN, '--input', path, '--k', '2', '--workers', '1'],
                                  capture_output=True, text=True)
            if done.returncode:
                print(i, spec['scene'], 'REFUS', done.stderr[:120], flush=True)
                out.write(json.dumps(dict(scene=spec['scene'], seed=spec['seed'], refused=True)) + '\n')
                continue
            report = json.loads(done.stdout)
            mass = math.sqrt(len(points))
            row = dict(scene=spec['scene'], seed=spec['seed'], n=len(points))
            for v in VARIANTS:
                C, M = mods[v]
                lab = labels_for(C, M, report, len(points), mass)
                s = baselines.scores(truth, lab)
                row[v] = (round(s['ari'], 4), s['clusters'], s['noise'])
            key = (spec['scene'], str(spec['seed']))
            row['hdb_default'] = round(base.get(key + ('hdbscan_default',), float('nan')), 4)
            row['hdb_oracle'] = round(base.get(key + ('hdbscan_oracle',), float('nan')), 4)
            row['sec'] = round(time.monotonic() - t0, 1)
            print(i, json.dumps(row), flush=True)
            out.write(json.dumps(row) + '\n')
            out.flush()


if __name__ == '__main__':
    main()
