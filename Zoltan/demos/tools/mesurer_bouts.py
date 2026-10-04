#!/usr/bin/env python3
"""Meilleur IoU de chaque objet suivi d'un bout, dans la hiérarchie de points HGP (morsehgp3D_v11) et dans celle de
HDBSCAN (scikit-learn), aux ordres demandés.

    python3 Zoltan/demos/tools/mesurer_bouts.py --bouts BOUTS.json --data DATA --export BIN --out RES \
        [--variante instances|sans_sol] [--orders 5,10] [--jobs 6] [--noms a,b]

Même mesure que morsehgp3D_v11/bench/points_campaign.py (évaluateur de bench/points_hierarchy.py : meilleur bloc aux
plateaux fermés, points void exclus), restreinte aux objets du groupe : un point d'une autre instance compte comme un
point quelconque du bout. HGP : export natif de la tour FULL (kmax = 10), puis la règle Hʳₖ₊₁ (bench/points_radius.py) ;
HDBSCAN : arbre du lien simple de l'atteignabilité mutuelle de sklearn (min_samples = k). Variante « instances » :
les seuls points des objets (bouts.json) ; « sans_sol » : la découpe nettoyée par Patchwork++ (bouts_sans_sol.json,
tools/chercher_bouts.py --sans-sol). Un fichier RES/<bout>.json par bout ; un bout déjà mesuré est sauté.
"""
import argparse
import concurrent.futures as cf
import hashlib
import json
from pathlib import Path
import sys
import time

import numpy as np

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'morsehgp3D_v11' / 'bench'))
import points_hierarchy as ph  # noqa: E402
import points_radius as prad  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parent))
from duel_scene import export_native  # noqa: E402
from kitti import VOID  # noqa: E402


def crop_of(entry, variante):
    return entry['sans_sol'] if variante == 'sans_sol' else entry


def objects_of(raw, keys):
    """Objet suivi de chaque point (rang de sa clé dans le groupe, -1 sinon) et masque des points void."""
    raw = np.asarray(raw, dtype=np.int64)
    obj = np.full(len(raw), -1, dtype=np.int64)
    for o, key in enumerate(keys):
        obj[raw == key] = o
    return obj, np.isin(raw & 0xFFFF, VOID)


def measure(args, entry):
    crop = crop_of(entry, args.variante)
    sites = args.data / (entry['name'] + '_sites.u32le')
    labels = args.data / (entry['name'] + '_labels.u32le')
    if hashlib.sha256(sites.read_bytes()).hexdigest() != crop['sites_sha256'] or \
            hashlib.sha256(labels.read_bytes()).hexdigest() != crop['crop_labels_sha256']:
        raise ValueError('empreintes du bout ' + entry['name'])
    xyz = np.fromfile(sites, dtype='<u4').reshape(-1, 3).astype(np.int64)
    raw = np.fromfile(labels, dtype='<u4').astype(np.int64)
    obj, void = objects_of(raw, entry['keys'])
    objects = len(entry['keys'])
    t0 = time.monotonic()
    tower, report = export_native(args.export, xyz, 1)
    ids = tower['ids']
    out = dict(name=entry['name'], variante=args.variante, sites=len(xyz), objects=entry['keys'],
               points=[int(np.sum((obj == o) & ~void)) for o in range(objects)], orders={})
    for k in args.orders:
        hanging = prad.hang_margin_radius(tower['orders'][k], k + 1, 'margin_r')
        ev = ph.evaluate_hanging(hanging, obj[ids], void[ids], objects)
        hdb = ph.evaluate_linkage(ph.hdbscan_tree(xyz, k), len(xyz), obj, void, objects)
        out['orders'][str(k)] = dict(hgp=[round(x, 6) for x in ev.best], hdbscan=[round(x, 6) for x in hdb.best],
                                     hgp_blocks=ev.blocks, hdbscan_blocks=hdb.blocks)
    out.update(seconds=round(time.monotonic() - t0, 2), export_status=report.get('status'))
    return out


def worker(args, entry):
    target = args.out / (entry['name'] + '.json')
    try:
        result = measure(args, entry)
        result['status'] = 'ok'
    except Exception as error:  # le bout est publié en échec, la mesure continue
        result = dict(name=entry['name'], variante=args.variante, status='failed', error='%s: %s' % (
            type(error).__name__, error))
    target.write_text(json.dumps(result, sort_keys=True) + '\n')
    return entry['name'], result['status'], result.get('seconds')


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--bouts', type=Path, required=True)
    p.add_argument('--data', type=Path, required=True)
    p.add_argument('--export', type=Path, required=True, help='binaire mhgp11_points_export de morsehgp3D_v11')
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--variante', choices=('instances', 'sans_sol'), default='instances')
    p.add_argument('--orders', default='5,10')
    p.add_argument('--jobs', type=int, default=4)
    p.add_argument('--noms', default='')
    args = p.parse_args()
    args.orders = [int(x) for x in args.orders.split(',')]
    args.out.mkdir(parents=True, exist_ok=True)
    entries = json.loads(args.bouts.read_text())['bouts']
    if args.noms:
        wanted = set(args.noms.split(','))
        entries = [e for e in entries if e['name'] in wanted]
    todo = [e for e in entries if not (args.out / (e['name'] + '.json')).is_file()]
    todo.sort(key=lambda e: -crop_of(e, args.variante)['sites'])  # les plus gros d'abord
    print('%d bouts, %d à mesurer' % (len(entries), len(todo)), flush=True)
    failed = 0
    with cf.ProcessPoolExecutor(max_workers=args.jobs) as pool:
        for name, status, seconds in pool.map(worker, [args] * len(todo), todo):
            failed += status != 'ok'
            print('%s %s %s' % (status, name, seconds), flush=True)
    return 1 if failed else 0


if __name__ == '__main__':
    raise SystemExit(main())
