#!/usr/bin/env python3
"""Export compact des arbres de points pour l'etude de la selection plate hors de la VM (E1, etape 0).

    python3 points_flat_dump.py --data DIR --export BUILD/mhgp11_points_export --gate GATE.json --work DIR
        --out DIR [--exact] [--jobs 24] [--native-workers 2] [--kmax 10] [--orders 2,3,5,10] [--roles bout,demo]

Les exports natifs bruts sont trop gros pour revenir de la VM (3,6 Go pour 36 scenes) ; on publie, par scene et par
ordre k, ce dont toute selection plate a besoin :
  - <scene>_k<k>_tower.npz  arbre de points de H^r_{k+1} (pendaison points_radius.hang_margin_radius, m = k + 1,
                            puis points_flat.tower_point_tree) : au plus 2n - 1 blocs, plateaux atomiques deja
                            tranches EXACTEMENT sur la VM ; niveaux exacts (--exact, table de rationnels) ou flottants
                            avec la borne rigoureuse de leur erreur relative ;
  - <scene>_k<k>_sklearn.npz arbre du lien simple de sklearn HDBSCAN(min_samples=k), tel quel (gauche, droite, valeur),
                            et labels_ du fit public a mcs = 20 (eom, epsilon 0, racine exclue) ; controle
                            tree_to_labels compile = labels_ ;
  - <scene>.json            tailles, temps, versions, controles.
Les sites sont relus dans l'ordre du fichier (PointId = indice d'entree), comme bench/points_campaign.py ; aucune
coordonnee ni etiquette n'est ecrite. Refuse de tourner sans porte points conforme du meme exportateur.
"""
import argparse
import concurrent.futures as cf
import json
import os
from pathlib import Path
import sys
import time
import traceback

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import points_campaign as pc  # noqa: E402
import points_flat as pf  # noqa: E402
import points_radius as prad  # noqa: E402

MCS_PUBLIC = 20


def sklearn_tree(xyz, k):
    """Fit public de sklearn (min_cluster_size = 20) : arbre du lien simple, labels_, controle tree_to_labels."""
    import sklearn
    from sklearn.cluster import HDBSCAN
    from sklearn.cluster._hdbscan._tree import tree_to_labels
    started = time.monotonic()
    model = HDBSCAN(min_cluster_size=MCS_PUBLIC, min_samples=k, cluster_selection_method='eom',
                    cluster_selection_epsilon=0.0, alpha=1.0, allow_single_cluster=False, metric='euclidean',
                    algorithm='kd_tree', n_jobs=1, copy=True)
    model.fit(np.asarray(xyz, dtype=np.float64))
    tree = np.asarray(model._single_linkage_tree_)
    again = tree_to_labels(tree, MCS_PUBLIC, 'eom', False, 0.0, None)[0]
    check = bool(np.array_equal(np.asarray(again), np.asarray(model.labels_)))
    return tree, np.asarray(model.labels_, dtype=np.int32), dict(
        sklearn=sklearn.__version__, seconds=round(time.monotonic() - started, 3), tree_to_labels_equal=check)


def dump_scene(args, name, xyz, data, exact):
    """data : export lu (ph.read_export) ; ecrit les .npz et rend le resume de la scene."""
    ids = data['ids']
    if sorted(ids.tolist()) != list(range(len(xyz))):
        raise ValueError('ids')
    out = dict(name=name, sites=len(xyz), orders={}, exact=exact)
    for k in args.orders:
        order = data['orders'][k]
        row = dict(full_nodes=int(order.size))
        t0 = time.monotonic()
        hanging = prad.hang_margin_radius(order, k + 1, 'margin_r')
        pt = pf.tower_point_tree(hanging)
        pt.ids = ids.astype(np.int64)
        path = args.out / ('%s_k%d_tower.npz' % (name, k))
        pt.save(path, meta=dict(scene=name, k=k, m=k + 1, rule='margin_r', exact=exact), exact=exact)
        row.update(tower_blocks=pt.blocks(), tower_plateaus=len(pt.levels), delayed=hanging.extra.get('delayed'),
                   tower_seconds=round(time.monotonic() - t0, 3), tower_bytes=path.stat().st_size)
        tree, labels, info = sklearn_tree(xyz, k)
        path = args.out / ('%s_k%d_sklearn.npz' % (name, k))
        left, right = tree['left_node'], tree['right_node']
        np.savez_compressed(path, left=left.astype(np.int32), right=right.astype(np.int32),
                            value=tree['value'].astype(np.float64), size=tree['cluster_size'].astype(np.int32),
                            labels_mcs20=labels)
        info.update(bytes=path.stat().st_size)
        row['sklearn'] = info
        out['orders'][str(k)] = row
    return out


def worker(args, name, exact):
    started = time.monotonic()
    try:
        _, xyz, _obj, _void, _objects, meta = pc.lidar_scene(args.data, name)
        data, report = pc.export(args.export, xyz, args.work, name, args.kmax, args.orders, args.native_workers)
        result = dump_scene(args, name, xyz, data, exact)
        result.update(status='ok', export=report, sites_sha256=meta['sites_sha256'],
                      seconds=round(time.monotonic() - started, 3))
    except Exception as error:  # la scene est publiee en echec, l'etude continue
        result = dict(name=name, status='failed', error=type(error).__name__ + ': ' + str(error),
                      trace=traceback.format_exc()[-2000:])
    (args.out / (name + '.json')).write_text(json.dumps(result, sort_keys=True) + '\n')
    return name, result['status']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--export', type=Path, required=True)
    parser.add_argument('--gate', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--exact', action='store_true', help='niveaux exacts (sinon flottants bornes)')
    parser.add_argument('--jobs', type=int, default=24)
    parser.add_argument('--native-workers', type=int, default=2)
    parser.add_argument('--kmax', type=int, default=10)
    parser.add_argument('--orders', default='2,3,5,10')
    parser.add_argument('--roles', default='')
    args = parser.parse_args()
    args.orders = [int(x) for x in args.orders.split(',')]
    gate = json.loads(args.gate.read_text()) if args.gate.is_file() else {}
    if gate.get('verdict') != 'conforme':
        print('refus : porte points non conforme ou absente', file=sys.stderr)
        return 2
    args.work.mkdir(parents=True, exist_ok=True)
    args.out.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((args.data / 'points_manifest.json').read_text())
    roles = set(args.roles.split(',')) if args.roles else None
    todo = [entry for entry in manifest['scenes'] if roles is None or entry.get('role') in roles]
    todo.sort(key=lambda entry: -int(entry.get('sites', 0)))  # les plus grosses d'abord
    statuses = {}
    with cf.ProcessPoolExecutor(max_workers=args.jobs) as pool:
        futures = [pool.submit(worker, args, entry['name'], args.exact) for entry in todo]
        for future in cf.as_completed(futures):
            name, status = future.result()
            statuses[name] = status
            print(json.dumps(dict(name=name, status=status)), flush=True)
    failed = sorted(name for name, status in statuses.items() if status != 'ok')
    print('points_flat_dump scenes%d echecs%d' % (len(statuses), len(failed)))
    return 0 if not failed else 1


if __name__ == '__main__':
    raise SystemExit(main())
