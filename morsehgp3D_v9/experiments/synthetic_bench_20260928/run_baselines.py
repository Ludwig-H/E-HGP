"""Mesure les references du banc et ecrit un CSV, une scene par ligne.

  python3 run_baselines.py --out baselines.csv [--seeds 5] [--light] [--only famille]

`--light` retire les scenes a 32 000 points (le banc complet y passe le plus
clair de son temps). Aucun parametre n'est ajuste ailleurs qu'a l'interieur de
`hdbscan_oracle`, et cet ajustement est declare dans la colonne `parameter`.
"""

import argparse
import csv
import json
import sys
import time

import baselines
import bench_datasets as data
import plan as bench_plan

COLUMNS = ('scene', 'family', 'intrinsic_dimension', 'n', 'groups', 'level', 'separation',
           'noise_fraction', 'seed', 'replicate',
           'points', 'noise_points', 'digest', 'method', 'parameter', 'ari', 'ami', 'coverage', 'clusters',
           'noise', 'seconds')


def run_row(spec, methods):
    points, truth, meta = data.generate({key: spec[key] for key in data.SPEC_KEYS})
    rows = []
    for name in methods:
        start = time.monotonic()
        if name == 'hdbscan_default':
            result = baselines.hdbscan_default(points, truth)
        elif name == 'hdbscan_oracle':
            result = baselines.hdbscan_oracle(points, truth)
        elif name == 'single_linkage':
            result = baselines.single_linkage(points, truth, spec['groups'])
        else:
            raise ValueError('unknown method ' + name)
        row = dict(scene=spec['scene'], family=spec['family'], n=spec['n'], groups=spec['groups'],
                   intrinsic_dimension=data.INTRINSIC_DIMENSION[spec['family']],
                   level=spec['level'], separation=meta['separation'], noise_fraction=spec['noise_fraction'],
                   seed=spec['seed'], replicate=spec['replicate'], points=meta['points'],
                   noise_points=meta['noise_points'], digest=meta['digest'], seconds=time.monotonic() - start)
        row.update(result)
        rows.append(row)
    return rows


def main(argv=None):
    parser = argparse.ArgumentParser(description='baselines of the 2026-09-28 synthetic bench')
    parser.add_argument('--out', required=True)
    parser.add_argument('--seeds', type=int, default=bench_plan.SEEDS)
    parser.add_argument('--light', action='store_true')
    parser.add_argument('--only', default=None)
    parser.add_argument('--methods', default='hdbscan_default,hdbscan_oracle,single_linkage')
    args = parser.parse_args(argv)
    methods = tuple(name for name in args.methods.split(',') if name)
    specs = bench_plan.specifications(seeds=args.seeds, heavy=not args.light)
    if args.only:
        specs = [spec for spec in specs if spec['family'] == args.only]
    with open(args.out, 'w', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=COLUMNS)
        writer.writeheader()
        for index, spec in enumerate(specs):
            for row in run_row(spec, methods):
                writer.writerow({key: row[key] for key in COLUMNS})
            handle.flush()
            print('%4d/%d %s seed=%d' % (index + 1, len(specs), spec['scene'], spec['seed']), flush=True)
    print(json.dumps(bench_plan.summary(specs)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
