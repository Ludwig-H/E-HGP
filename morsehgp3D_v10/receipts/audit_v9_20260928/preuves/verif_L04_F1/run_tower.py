"""Mesure la tour HGP sur le banc calibre, aux memes colonnes que les references.

  python3 run_tower.py --export <binaire> --out tour.csv [--k 2] [--z 1] ...

Tous les reglages sont **fixes d'avance et publies dans chaque ligne**. Aucun
n'est choisi par scene : c'est la faute que l'audit du 28 septembre a relevee
sur la campagne du 27, ou la marge annoncee venait d'un parametre fige au bon
endroit. `min_cluster_size` vaut la racine de n, qui est le defaut du code de
la these (`core.py:141`), et non une valeur ajustee ici.

La sortie a les colonnes de `run_baselines.py`, donc `compare.py` apparie les
deux campagnes sur (scene, graine) sans transformation.
"""

import argparse
import csv
import json
import math
import os
import subprocess
import sys
import tempfile
import time

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'synthetic_bench_20260928'))

import baselines                      # noqa: E402
import bench_datasets as data         # noqa: E402
import plan as bench_plan             # noqa: E402

import cluster as C                   # noqa: E402
import measure as M                   # noqa: E402

COLUMNS = ('scene', 'family', 'intrinsic_dimension', 'n', 'groups', 'level', 'separation',
           'noise_fraction', 'seed', 'replicate', 'points', 'noise_points', 'digest', 'method',
           'parameter', 'ari', 'ami', 'coverage', 'clusters', 'noise', 'seconds')


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def export(binary, grid, k, workers, keep):
    """Ecrit le nuage quantifie et appelle l'export natif de la tour."""
    handle, path = tempfile.mkstemp(suffix='.u32le', dir=keep)
    with os.fdopen(handle, 'wb') as stream:
        stream.write(np.ascontiguousarray(grid, dtype='<u4').tobytes())
    try:
        done = subprocess.run([binary, '--input', path, '--k', str(k), '--workers', str(workers)],
                              capture_output=True, text=True)
        need(done.returncode == 0, 'native export refused: ' + (done.stderr or '')[:200])
        return json.loads(done.stdout)
    finally:
        os.unlink(path)


def tower_labels(report, points, k, z, convention, min_cluster_mass, mode):
    """La chaine complete : mesure, arbre, condensation, antichaine, vote."""
    cofaces, gabriel, size = M.read_export(report)
    births = M.facet_births(cofaces, gabriel, convention)
    keep = None if convention == 'boundary' else gabriel
    facets, plateaus = C.facet_levels(cofaces, keep)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    sums, totals, masses, covered = M.measure(cofaces, gabriel, z, convention)
    problems, info = M.check_invariants(sums, totals, masses, covered, size, convention)
    need(not problems, 'section 9.1 invariants broken: ' + '; '.join(problems))
    clusters, order = C.condense(nodes, roots, masses, births, min_cluster_mass, mode, z)
    selected = C.select_excess_of_mass(clusters, order)
    labels, ties = C.vote(points, C.label_facets(clusters, selected), sums, totals, len(selected))
    return labels, dict(ties=ties, facets=info['facets'], max_mass=float(info['max_mass']),
                        nodes=len(nodes), roots=len(roots), selected=len(selected))


def run_one(spec, args, keep):
    points, truth, meta = data.generate({key: spec[key] for key in data.SPEC_KEYS})
    grid, scale = data.quantize(points)
    start = time.monotonic()
    report = export(args.export, grid, args.k, args.workers, keep)
    mass = math.sqrt(len(points)) if args.min_cluster_mass <= 0 else args.min_cluster_mass
    labels, detail = tower_labels(report, len(points), args.k, args.z, args.convention, mass, args.lambda_mode)
    seconds = time.monotonic() - start
    scored = baselines.scores(truth, np.asarray(labels))
    name = 'tower_k%d_z%d_%s' % (args.k, args.z, args.convention)
    return dict(scored, scene=spec['scene'], family=spec['family'],
                intrinsic_dimension=data.INTRINSIC_DIMENSION[spec['family']], n=spec['n'],
                groups=spec['groups'], level=spec['level'], separation=meta['separation'],
                noise_fraction=spec['noise_fraction'], seed=spec['seed'], replicate=spec['replicate'],
                points=meta['points'], noise_points=meta['noise_points'], digest=meta['digest'],
                method=name, parameter=round(mass, 3), seconds=seconds), detail


def main(argv=None):
    parser = argparse.ArgumentParser(description='HGP tower clustering on the calibrated bench')
    parser.add_argument('--export', required=True, help='native_weighted_export binary')
    parser.add_argument('--out', required=True)
    parser.add_argument('--k', type=int, default=2)
    parser.add_argument('--z', type=int, default=1)
    parser.add_argument('--convention', default='gabriel', choices=M.CONVENTIONS)
    parser.add_argument('--lambda-mode', default='radius', choices=C.LAMBDA_MODES)
    parser.add_argument('--min-cluster-mass', type=float, default=0.0,
                        help='0 means the thesis default, the square root of n')
    parser.add_argument('--workers', type=int, default=4)
    parser.add_argument('--seeds', type=int, default=bench_plan.SEEDS)
    parser.add_argument('--light', action='store_true')
    parser.add_argument('--only', default=None)
    parser.add_argument('--max-n', type=int, default=0)
    args = parser.parse_args(argv)
    need(os.access(args.export, os.X_OK), 'the native export binary must be executable')

    specs = bench_plan.specifications(seeds=args.seeds, heavy=not args.light)
    if args.only:
        wanted = set(args.only.split(','))
        specs = [s for s in specs if s['family'] in wanted]
    if args.max_n:
        specs = [s for s in specs if s['n'] <= args.max_n]
    refused = 0
    with tempfile.TemporaryDirectory() as keep, open(args.out, 'w', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=COLUMNS)
        writer.writeheader()
        for index, spec in enumerate(specs):
            try:
                row, detail = run_one(spec, args, keep)
            except (ValueError, MemoryError) as failure:
                refused += 1
                print('%4d/%d %s seed=%d REFUS %s' % (index + 1, len(specs), spec['scene'],
                                                      spec['seed'], str(failure)[:90]), flush=True)
                continue
            writer.writerow({key: row[key] for key in COLUMNS})
            handle.flush()
            print('%4d/%d %-38s seed=%d ARI=%.3f couv=%.2f clusters=%d %.1fs'
                  % (index + 1, len(specs), spec['scene'], spec['seed'], row['ari'],
                     row['coverage'], row['clusters'], row['seconds']), flush=True)
    print(json.dumps(dict(runs=len(specs) - refused, refused=refused, k=args.k, z=args.z,
                          convention=args.convention, lambda_mode=args.lambda_mode,
                          min_cluster_mass='sqrt(n)' if args.min_cluster_mass <= 0 else args.min_cluster_mass)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
