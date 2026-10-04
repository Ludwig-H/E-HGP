#!/usr/bin/env python3
"""Choix des bouts de scène où la hiérarchie HDBSCAN échoue et la hiérarchie HGP réussit, images et catalogue.

    python3 Zoltan/demos/tools/choisir_bouts.py --bouts SORTIE/bouts.json --data SORTIE/data \
        --results RESULTATS_DE_SESSION/lidar --out Zoltan/demos/bouts_hgp [--par-famille 8]

Mesure (comme les démos de Zoltan/) : pour chaque objet du bout, le meilleur IoU atteint par un nœud quelconque de la
hiérarchie, au même ordre k (HDBSCAN : `min_samples` = k, arbre de liaison simple de scikit-learn ; HGP : hiérarchie de
points H^r_{k+1} de morsehgp3D_v11, règle `margin_r`). Critères, fixés avant de lire les résultats :
- HDBSCAN échoue à l'ordre k : au moins un objet du bout a un meilleur IoU <= 1/2 ;
- HGP réussit à l'ordre k : tous les objets du bout ont un meilleur IoU > 1/2 ;
- bout retenu : il existe un k où HDBSCAN échoue et où HGP réussit ; échec « à tous les ordres » de HDBSCAN quand il
  échoue aux quatre ordres mesurés (2, 3, 5, 10) ;
- l'inverse (HGP échoue, HDBSCAN réussit au même k) est compté et publié.
Classement dans une famille : échec à tous les ordres d'abord, puis le plus grand écart entre le pire objet de HGP et
le pire objet de HDBSCAN ; un seul bout par trame et par famille (le plus grand groupe d'abord).
Images : vue de dessus, trois panneaux (vérité ; meilleur nœud HDBSCAN de l'objet qui échoue ; meilleur nœud HGP du même
objet) : vert = point de l'objet dans le nœud, rouge = point d'un autre objet dans le nœud, bleu = point de l'objet hors
du nœud, gris = autres points. Les images sont des œuvres dérivées de KITTI et SemanticKITTI (CC BY-NC-SA).
"""
import argparse
import json
from pathlib import Path
import shutil
import sys

import numpy as np

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'morsehgp3D_v11' / 'bench'))
from points_render import png  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parent))
from kitti import FR  # noqa: E402

ORDERS = ('2', '3', '5', '10')
RULE = 'margin_r'
OBJECT_COLORS = ((31, 119, 180), (255, 127, 14), (148, 103, 189))
FAMILIES = ('voitures', 'velos', 'velos_pietons')


def evaluate(entry, result):
    rows = {}
    for k in ORDERS:
        o = result['orders'].get(k)
        if o is None or RULE not in o:
            continue
        hdb, hgp = o['hdbscan']['best'], o[RULE]['best']
        rows[k] = dict(hdbscan=hdb, hgp=hgp, cover=o.get('cover', {}).get('best'), core=o.get('core', {}).get('best'),
                       hdbscan_mean=sum(hdb) / len(hdb), hgp_mean=sum(hgp) / len(hgp),
                       hdbscan_fails=min(hdb) <= 0.5, hgp_succeeds=min(hgp) > 0.5,
                       hgp_fails=min(hgp) <= 0.5, hdbscan_succeeds=min(hdb) > 0.5)
    wins = [k for k, r in rows.items() if r['hdbscan_fails'] and r['hgp_succeeds']]
    losses = [k for k, r in rows.items() if r['hgp_fails'] and r['hdbscan_succeeds']]
    strong = bool(rows) and all(r['hdbscan_fails'] for r in rows.values()) and bool(wins)
    best_k = max(wins, key=lambda k: (min(rows[k]['hgp']) - min(rows[k]['hdbscan']), -int(k))) if wins else None
    return dict(rows=rows, wins=wins, losses=losses, strong=strong, best_k=best_k,
                margin=(min(rows[best_k]['hgp']) - min(rows[best_k]['hdbscan'])) if best_k else None)


def frame_xy(xyz):
    """Vue de dessus tournée selon l'axe principal du bout (rangées horizontales), centrée."""
    xy = xyz[:, :2] - xyz[:, :2].mean(axis=0)
    _, vectors = np.linalg.eigh(np.cov(xy.T))
    xy = xy @ vectors[:, ::-1]
    return xy - xy.min(axis=0)


def canvas(xy, colors, width, height, margin=14):
    """Points de 3 x 3 pixels, échelle commune aux deux axes ; colors : tableau (n, 3) uint8, dessiné dans l'ordre."""
    image = np.full((height, width, 3), 255, dtype=np.uint8)
    extent = np.maximum(xy.max(axis=0), 1e-6)
    scale = min((width - 2 * margin) / extent[0], (height - 2 * margin) / extent[1])
    ox = (width - extent[0] * scale) / 2
    oy = (height - extent[1] * scale) / 2
    px = (ox + xy[:, 0] * scale).astype(int)
    py = (height - 1 - (oy + xy[:, 1] * scale)).astype(int)
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            image[np.clip(py + dy, 0, height - 1), np.clip(px + dx, 0, width - 1)] = colors
    return image


KIND = dict(other=(200, 200, 200), fn=(40, 90, 230), fp=(220, 40, 40), tp=(30, 160, 60))


def render(entry, result, data, k, out):
    xyz = np.fromfile(data / (entry['name'] + '_sites.u32le'), dtype='<u4').reshape(-1, 3).astype(np.float64) / 1000.0
    raw = np.fromfile(data / (entry['name'] + '_labels.u32le'), dtype='<u4').astype(np.int64)
    labels = result['meta']['objects']
    hdb = result['orders'][k]['hdbscan']['best']
    worst = int(np.argmin(hdb))
    rescue = result['orders'][k]['rescued'].get('%s:%d' % (RULE, worst))
    if rescue is None or rescue['members'] is None or rescue['hdbscan_members'] is None:
        return None
    xy = frame_xy(xyz)
    extent = np.maximum(xy.max(axis=0), 1e-6)
    width = 520
    height = int(min(520, max(170, width * extent[1] / extent[0] + 28)))
    truth = np.zeros((len(xyz), 3), dtype=np.uint8) + 200
    for j, label in enumerate(labels):
        truth[raw == label] = OBJECT_COLORS[j % 3]
    panels = [canvas(xy, truth, width, height)]
    inside = raw == labels[worst]
    for members in (rescue['hdbscan_members'], rescue['members']):
        block = np.zeros(len(xyz), dtype=bool)
        block[members] = True
        colors = np.zeros((len(xyz), 3), dtype=np.uint8)
        order = []
        for name, mask in (('other', ~inside & ~block), ('fn', inside & ~block), ('fp', ~inside & block),
                           ('tp', inside & block)):
            colors[mask] = KIND[name]
            order.append(np.flatnonzero(mask))
        idx = np.concatenate(order)  # le vert et le rouge au-dessus du gris et du bleu
        panels.append(canvas(xy[idx], colors[idx], width, height))
    gap = np.full((height, 10, 3), 255, dtype=np.uint8)
    png(out, np.concatenate([panels[0], gap, panels[1], gap, panels[2]], axis=1))
    return dict(object=worst, hdbscan_iou=rescue['hdbscan_iou'], hgp_iou=rescue['iou'])


def fr(x):
    return ('%.2f' % x).replace('.', ',')


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--bouts', type=Path, required=True)
    p.add_argument('--data', type=Path, required=True)
    p.add_argument('--results', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--par-famille', type=int, default=8)
    args = p.parse_args()
    bouts = {e['name']: e for e in json.loads(args.bouts.read_text())['bouts']}
    evaluated = []
    for f in sorted(args.results.glob('*.json')):
        result = json.loads(f.read_text())
        entry = bouts.get(result.get('name'))
        if entry is None or result.get('status') != 'ok':
            continue
        ev = evaluate(entry, result)
        evaluated.append((entry, result, ev))
    counts = {fam: dict(bouts=0, hdbscan_echoue=0, gagnes=0, gagnes_tous_ordres=0, perdus=0) for fam in FAMILIES}
    for entry, _r, ev in evaluated:
        c = counts[entry['kind']]
        c['bouts'] += 1
        c['hdbscan_echoue'] += any(r['hdbscan_fails'] for r in ev['rows'].values())
        c['gagnes'] += bool(ev['wins'])
        c['gagnes_tous_ordres'] += ev['strong']
        c['perdus'] += bool(ev['losses'])
    chosen = []
    for fam in FAMILIES:
        pool = [x for x in evaluated if x[0]['kind'] == fam and x[2]['wins']]
        pool.sort(key=lambda x: (not x[2]['strong'], -x[2]['margin'], -len(x[0]['keys']), x[0]['name']))
        frames = set()
        for entry, result, ev in pool:
            key = (entry['seq'], entry['frame'])
            if key in frames:
                continue
            frames.add(key)
            chosen.append((entry, result, ev))
            if sum(1 for c in chosen if c[0]['kind'] == fam) >= args.par_famille:
                break
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / 'data').mkdir(exist_ok=True)
    catalogue = []
    for entry, result, ev in chosen:
        k = ev['best_k']
        image = '%s_k%s.png' % (entry['name'], k)
        drawn = render(entry, result, args.data, k, args.out / image)
        for suffix in ('_sites.u32le', '_labels.u32le'):  # copie locale, ignorée par git (*/data/)
            shutil.copyfile(args.data / (entry['name'] + suffix), args.out / 'data' / (entry['name'] + suffix))
        catalogue.append(dict(name=entry['name'], kind=entry['kind'], seq=entry['seq'], frame=entry['frame'],
                              velodyne_sha256=entry['velodyne_sha256'], labels_sha256=entry['labels_sha256'],
                              keys=entry['keys'], classes=entry['classes'], points=entry['points'], sites=entry['sites'],
                              gaps=entry['gaps'], difficulty=entry['difficulty'], sites_sha256=entry['sites_sha256'],
                              crop_labels_sha256=entry['crop_labels_sha256'], order=k, wins=ev['wins'],
                              losses=ev['losses'], strong=ev['strong'], image=image if drawn else None,
                              orders=ev['rows']))
    (args.out / 'bouts.json').write_text(json.dumps(dict(schema='ehgp.zoltan.bouts_hgp.v1', criteres=__doc__.split('Images')[0],
                                                         comptes=counts, bouts=catalogue), indent=1, ensure_ascii=False))
    every = []  # tous les bouts mesurés, réussites ou non : définitions et meilleurs IoU par objet
    for entry, result, ev in evaluated:
        every.append(dict({key: entry[key] for key in ('name', 'kind', 'seq', 'frame', 'velodyne_sha256', 'labels_sha256',
                                                        'keys', 'classes', 'points', 'sites', 'gaps', 'difficulty',
                                                        'sites_sha256', 'crop_labels_sha256')},
                          wins=ev['wins'], losses=ev['losses'], strong=ev['strong'],
                          orders={k: dict(hdbscan=r['hdbscan'], hgp=r['hgp'], cover=r['cover'], core=r['core'])
                                  for k, r in ev['rows'].items()}))
    (args.out / 'evalues.json').write_text(json.dumps(dict(schema='ehgp.zoltan.bouts_evalues.v1', comptes=counts,
                                                           bouts=every), indent=1, ensure_ascii=False))
    lines = ['| bout | trame | objets (points) | écart | k | HDBSCAN, meilleur IoU par objet | HGP, meilleur IoU par objet | IoU moyen HDBSCAN / HGP | image |',
             '| --- | --- | --- | --- | --- | --- | --- | --- | --- |']
    for c in catalogue:
        r = c['orders'][str(c['order'])]
        objs = ', '.join('%s (%d)' % (FR.get(cl, cl), n) for cl, n in zip(c['classes'], c['points']))
        gap = min(c['gaps'].values())
        lines.append('| %s | %s/%s | %s | %s m | %s%s | %s | %s | %s / %s | %s |' % (
            c['name'], c['seq'], c['frame'], objs, fr(gap), c['order'], ' (tous)' if c['strong'] else '',
            ' / '.join(('**%s**' % fr(x)) if x <= 0.5 else fr(x) for x in r['hdbscan']),
            ' / '.join(fr(x) for x in r['hgp']), fr(r['hdbscan_mean']), fr(r['hgp_mean']),
            '[png](%s)' % c['image'] if c['image'] else '—'))
    (args.out / 'catalogue.md').write_text('\n'.join(lines) + '\n')
    print(json.dumps(counts, indent=1))
    print('retenus', len(catalogue))


if __name__ == '__main__':
    main()
