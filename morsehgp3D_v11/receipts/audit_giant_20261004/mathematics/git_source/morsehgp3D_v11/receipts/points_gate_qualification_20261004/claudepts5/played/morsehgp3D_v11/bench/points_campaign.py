#!/usr/bin/env python3
"""Campagne FULL -> points contre HDBSCAN (sklearn) : scenes synthetiques puis trames LiDAR de Zoltan/.

    python3 bench/points_campaign.py --mode synthetic|lidar --export BUILD/mhgp11_points_export --gate GATE.json
        --work DIR --out DIR [--data DIR] [--jobs 16] [--kmax 10] [--orders 2,3,5,10]

Par scene : un export natif (FULL 1..kmax, ordres demandes), puis pour chaque ordre k six pendaisons
(core, cover, first m=k+1, margin1 m=1, margin m=k+1 en niveau carre ; margin_r m=k+1 en rayon,
bench/points_radius.py) et l'arbre du lien simple de
sklearn HDBSCAN(min_samples=k). Meme evaluateur pour tous : meilleur IoU de chaque groupe vrai parmi les blocs
publies aux plateaux fermes (void exclus). Refuse de tourner sans porte conforme du meme exportateur.
Aucune selection de partition ; aucun seuil ajuste apres lecture.
"""
import argparse
import concurrent.futures as cf
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import traceback

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import points_hierarchy as ph  # noqa: E402
import points_radius as prad  # noqa: E402

VENDOR = os.path.join(HERE, '..', 'receipts', 'full_points_20261003', 'experiment')
METHODS = ('core', 'cover', 'first', 'margin1', 'margin', 'margin_r')
MEMBER_CAP = 20000  # blocs sauves publies en entier jusqu'a cette taille, sinon leur seule taille
FAMILIES = ('spherical', 'anisotropic', 'heteroscedastic', 'unbalanced', 'shells', 'bridge', 'hierarchical',
            'filaments')


def digest(path):
    h = hashlib.sha256()
    with open(path, 'rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def synthetic_specs():
    """Plan fixe, ecrit avant lecture : 8 familles x 2 niveaux x 2 nombres de groupes x 2 tailles x 2 graines."""
    specs = []
    for family in FAMILIES:
        for level in ('medium', 'hard'):
            for groups in (3, 8):
                for n in (2000, 8000):
                    for seed in (9341, 9342):
                        specs.append(dict(family=family, level=level, groups=groups, n=n, noise_fraction=0.05,
                                          seed=seed))
    return specs


def synthetic_scene(spec):
    sys.path.insert(0, VENDOR)
    import vendor_scenes as vs
    points, labels, meta = vs.generate(spec)
    grid, labels, dropped, h = vs.quantize18(points, labels)
    name = '%s_%s_g%d_n%d_s%d' % (spec['family'], spec['level'], spec['groups'], spec['n'], spec['seed'])
    obj = np.asarray(labels, dtype=np.int64)
    return name, grid.astype(np.int64), obj, np.zeros(len(obj), dtype=bool), int(obj.max()) + 1, \
        dict(spec=spec, dropped_duplicates=int(dropped), step=h, generator_digest=meta['digest'])


def lidar_scene(data, name):
    xyz = np.fromfile(data / (name + '_sites.u32le'), dtype='<u4').reshape(-1, 3).astype(np.int64)
    raw = np.fromfile(data / (name + '_labels.u32le'), dtype='<u4')
    if len(raw) != len(xyz):
        raise ValueError('labels')
    obj, void, keys = ph.lidar_objects(raw, 50)
    return name, xyz, obj, void, len(keys), dict(objects=keys, sites_sha256=digest(data / (name + '_sites.u32le')))


def export(binary, xyz, work, name, kmax, orders, workers):
    paths = [work / (name + suffix) for suffix in ('.xyz', '.ids', '.ph')]
    np.asarray(xyz, dtype='<u4').tofile(paths[0])
    np.arange(len(xyz), dtype='<u4').tofile(paths[1])
    started = time.monotonic()
    done = subprocess.run([str(binary), str(paths[0]), str(paths[1]), str(paths[2]), str(kmax),
                           ','.join(map(str, orders)), str(workers), str(48 << 30)],
                          capture_output=True, text=True, timeout=3600)
    report = json.loads(done.stdout.strip().splitlines()[-1]) if done.stdout.strip() else {}
    if done.returncode != 0:
        raise RuntimeError('export %s code %d %s %s' % (name, done.returncode, done.stdout[-500:], done.stderr[-500:]))
    data = ph.read_export(paths[2])
    report.update(wall_seconds=time.monotonic() - started, bytes=paths[2].stat().st_size)
    for path in paths:
        path.unlink()
    return data, report


def best_rows(ev):
    return [round(x, 6) for x in ev.best], ev.blocks


def one_scene(args, item):
    name, xyz, obj, void, objects, meta = item
    started = time.monotonic()
    result = dict(name=name, sites=len(xyz), objects=objects, meta=meta, orders={}, status='started')
    data, report = export(args.export, xyz, args.work, name, args.kmax, args.orders, args.native_workers)
    ids = data['ids']
    if sorted(ids.tolist()) != list(range(len(xyz))):
        raise ValueError('ids')
    nobj, nvoid = obj[ids], void[ids]  # vers l'ordre natif des sites
    result['export'] = report
    for k in args.orders:
        order = data['orders'][k]
        row = dict(nodes=order.size, incidences=int(len(order.inc_rank)))
        builds = dict(core=lambda: ph.hang_core(order), cover=lambda: ph.hang_first(order, 1, 'cover'),
                      first=lambda: ph.hang_first(order, k + 1, 'first'),
                      margin1=lambda: ph.hang_margin(order, 1, 'margin1'),
                      margin=lambda: ph.hang_margin(order, k + 1, 'margin'),
                      margin_r=lambda: prad.hang_margin_radius(order, k + 1, 'margin_r'))
        t0 = time.monotonic()
        tree = ph.hdbscan_tree(xyz, k)
        hdb = ph.evaluate_linkage(tree, len(xyz), obj, void, objects)
        best, blocks = best_rows(hdb)
        row['hdbscan'] = dict(best=best, blocks=blocks, seconds=round(time.monotonic() - t0, 3))
        rescued = {}
        for method in METHODS:
            if method in args.skip.get(k, ()):
                continue
            t0 = time.monotonic()
            hanging = builds[method]()
            ev = ph.evaluate_hanging(hanging, nobj, nvoid, objects)
            best, blocks = best_rows(ev)
            row[method] = dict(best=best, blocks=blocks, extra=hanging.extra, seconds=round(time.monotonic() - t0, 3))
            if args.mode != 'lidar' or method not in ('margin', 'margin_r', 'cover'):
                continue
            for o in range(objects):  # sauvetages : la tour depasse 1/2 la ou HDBSCAN reste a 1/2 ou moins
                if ev.best[o] > 0.5 >= hdb.best[o] and len(rescued) < 40:
                    node, level = ev.best_ref[o]
                    members = ids[ph.hanging_members(hanging, node, level)]
                    hnode, hlevel = hdb.best_ref[o]
                    hmembers = ph.linkage_members(hdb, hnode, len(xyz))
                    rescued['%s:%d' % (method, o)] = dict(
                        object=o, method=method, iou=ev.best[o], hdbscan_iou=hdb.best[o], level=str(level),
                        size=int(len(members)), hdbscan_size=int(len(hmembers)), hdbscan_level=hlevel,
                        members=sorted(members.tolist()) if len(members) <= MEMBER_CAP else None,
                        hdbscan_members=hmembers.tolist() if len(hmembers) <= MEMBER_CAP else None)
        row['rescued'] = rescued
        result['orders'][str(k)] = row
    result.update(status='ok', seconds=round(time.monotonic() - started, 3))
    return result


def scenes(args):
    if args.mode == 'synthetic':
        for spec in synthetic_specs():
            yield ('spec', spec)
    else:
        manifest = json.loads((args.data / 'points_manifest.json').read_text())
        roles = set(args.roles.split(',')) if args.roles else None
        for entry in manifest['scenes']:
            if roles is None or entry.get('role') in roles:
                yield ('lidar', entry['name'])


def worker(args, kind, value):
    try:
        item = synthetic_scene(value) if kind == 'spec' else lidar_scene(args.data, value)
        result = one_scene(args, item)
    except Exception as error:  # la scene est publiee en echec, la campagne continue
        result = dict(name=str(value), status='failed', error=type(error).__name__ + ': ' + str(error),
                      trace=traceback.format_exc()[-2000:])
    (args.out / (result['name'] + '.json')).write_text(json.dumps(result, sort_keys=True) + '\n')
    return result['name'], result['status']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=('synthetic', 'lidar'), required=True)
    parser.add_argument('--export', type=Path, required=True)
    parser.add_argument('--gate', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--data', type=Path)
    parser.add_argument('--jobs', type=int, default=16)
    parser.add_argument('--native-workers', type=int, default=4)
    parser.add_argument('--kmax', type=int, default=10)
    parser.add_argument('--orders', default='2,3,5,10')
    parser.add_argument('--roles', default='', help='filtre des scenes LiDAR par role du manifeste (demo, echec, temoin)')
    args = parser.parse_args()
    args.orders = [int(x) for x in args.orders.split(',')]
    # LiDAR : margin1 (H_1, la plus faible) omise partout et first a k = 10 (cout) ; toutes restent mesurees en
    # synthetique.
    args.skip = {k: (('first', 'margin1') if k >= 10 else ('margin1',)) for k in args.orders} \
        if args.mode == 'lidar' else {}
    gate = json.loads(args.gate.read_text()) if args.gate.is_file() else {}
    if gate.get('verdict') != 'conforme':
        print('refus : porte points non conforme ou absente', file=sys.stderr)
        return 2
    args.work.mkdir(parents=True, exist_ok=True)
    args.out.mkdir(parents=True, exist_ok=True)
    todo = list(scenes(args))
    statuses = {}
    with cf.ProcessPoolExecutor(max_workers=args.jobs) as pool:
        futures = [pool.submit(worker, args, kind, value) for kind, value in todo]
        for future in cf.as_completed(futures):
            name, status = future.result()
            statuses[name] = status
            print(json.dumps(dict(name=name, status=status)), flush=True)
    failed = sorted(name for name, status in statuses.items() if status != 'ok')
    print('points_campaign_%s scenes%d echecs%d' % (args.mode, len(statuses), len(failed)))
    return 0 if not failed else 1


if __name__ == '__main__':
    raise SystemExit(main())
