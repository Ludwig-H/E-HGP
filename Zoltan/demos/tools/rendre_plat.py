#!/usr/bin/env python3
"""Images de la sortie plate (clusters) des exemples de Zoltan/demos : vérité | HDBSCAN (sklearn tel quel) | HGP
pour chaque règle demandée (EOM z = 1, z = 2...).

    python3 Zoltan/demos/tools/rendre_plat.py --dumps DIR --scenes DIR --rules eom1,eom2 --mcs 20 [--examples DOSSIER...]

Pour chaque exemple (dossier contenant bout.json, ou démo de scène entière), à l'ordre montré par l'exemple et à
chaque ordre listé : arbres exportés par morsehgp3D_v11/bench/points_flat_dump.py (session G4), tête certifiée de
morsehgp3D_v11/bench/points_flat.py pour HGP, labels_ du fit public de sklearn (mcs 20) pour HDBSCAN. Couleurs : un
cluster apparié un-à-un à un objet suivi (IoU > 1/2) prend la couleur de l'objet ; les autres clusters ont des
couleurs pâles distinctes ; le bruit est gris clair. Écrit <prefixe>_k<k>.png et <prefixe>.json dans l'exemple ; les données
(coordonnées) restent hors dépôt.
"""
import argparse
import json
import os
from pathlib import Path
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, 'morsehgp3D_v11', 'bench'))
import choisir_bouts as cb  # noqa: E402
import points_flat as pf  # noqa: E402
import points_flat_study as fs  # noqa: E402
import points_hierarchy as ph  # noqa: E402

PALE = ((174, 199, 232), (255, 187, 120), (152, 223, 138), (255, 152, 150), (197, 176, 213), (196, 156, 148),
        (247, 182, 210), (219, 219, 141), (158, 218, 229), (199, 199, 199))
NOISE = (225, 225, 225)


def colors_for(labels, raw, tracked):
    """Couleur par point : objet suivi apparié (IoU > 1/2) -> couleur de l'objet ; autre cluster -> pâle ; bruit."""
    out = np.zeros((len(labels), 3), dtype=np.uint8) + np.array(NOISE, dtype=np.uint8)
    clusters = [c for c in np.unique(labels) if c >= 0]
    taken = {}
    for j, key in enumerate(tracked):
        inside = raw == key
        best, best_iou = None, 0.5
        for c in clusters:
            member = labels == c
            inter = int(np.sum(inside & member))
            if not inter:
                continue
            iou = inter / float(np.sum(inside | member))
            if iou > best_iou:
                best, best_iou = c, iou
        if best is not None:
            taken[best] = cb.OBJECT_COLORS[j % 3]
    for i, c in enumerate(clusters):
        out[labels == c] = taken.get(c, PALE[i % len(PALE)])
    return out


def render(xyz, raw, tracked, panels_labels, out_png, margin_m=None):
    keep = np.ones(len(xyz), dtype=bool)
    if margin_m is not None:
        mask = np.isin(raw, tracked)
        lo, hi = xyz[mask, :2].min(axis=0) - margin_m, xyz[mask, :2].max(axis=0) + margin_m
        keep = np.all((xyz[:, :2] >= lo) & (xyz[:, :2] <= hi), axis=1)
    idx = np.flatnonzero(keep)
    xy = cb.frame_xy(xyz[idx].astype(np.float64))
    extent = np.maximum(xy.max(axis=0), 1e-6)
    elongated = extent[0] >= 2.0 * extent[1]
    width = 900 if elongated else 420
    height = int(min(420, max(120, width * extent[1] / extent[0] + 28)))
    truth = np.zeros((len(idx), 3), dtype=np.uint8) + np.array(NOISE, dtype=np.uint8)
    for j, key in enumerate(tracked):
        truth[raw[idx] == key] = cb.OBJECT_COLORS[j % 3]
    panels = [cb.canvas(xy, truth, width, height)]
    for labels in panels_labels:
        panels.append(cb.canvas(xy, colors_for(labels[idx], raw[idx], tracked), width, height))
    # Scene allongee : panneaux empiles ; sinon grille de trois colonnes. Ordre : verite, puis les sorties demandees.
    columns = 1 if elongated else 3
    blank = np.full((height, width, 3), 255, dtype=np.uint8)
    while len(panels) % columns:
        panels.append(blank)
    vgap = np.full((height, 12, 3), 255, dtype=np.uint8)
    rows = []
    for r in range(0, len(panels), columns):
        parts = []
        for p in panels[r:r + columns]:
            parts.extend([p, vgap])
        rows.append(np.concatenate(parts[:-1], axis=1))
    hgap = np.full((12, rows[0].shape[1], 3), 255, dtype=np.uint8)
    stack = []
    for row in rows:
        stack.extend([row, hgap])
    cb.png(out_png, np.concatenate(stack[:-1], axis=0))


def flat_labels(dumps, name, k, rules, mcs):
    """labels_ de sklearn (fit public, mcs 20) et sorties plates de la tour pour chaque regle."""
    pt = pf.PointTree.load(dumps / ('%s_k%d_tower.npz' % (name, k)))
    cond, first = pf.condense(pt, mcs)
    out, stats = [], []
    for rule in rules:
        method, z = fs.RULES[rule]
        sel = pf.select(pt, cond, z, method)
        out.append(pf.labels(pt, cond, first, sel))
        stats.append(sel.stats)
    with np.load(dumps / ('%s_k%d_sklearn.npz' % (name, k))) as zf:
        hdb = zf['labels_mcs20'].astype(np.int64)
    return hdb, out, stats


LABELS = {'hdbscan_sklearn': 'HDBSCAN (`sklearn` tel quel)', 'hgp_eom1': 'HGP, EOM z = 1', 'hgp_eom2': 'HGP, EOM z = 2',
          'hgp_eom3': 'HGP, EOM z = 3', 'hgp_leaf': 'HGP, feuilles'}
BEGIN, END = '<!-- plat:debut -->', '<!-- plat:fin -->'


def readme_section(report, prefix):
    """Section « Sortie plate » d'un exemple (remplacee a chaque rendu, entre deux balises)."""
    lines = [BEGIN, '', '## Sortie plate (clusters)', '']
    panels = ['vérité'] + [LABELS.get(p, p) for p in report['panels'][1:]]
    lines.append('mcs = %d, racine exclue, aucune complétion. Panneaux, de gauche à droite puis de haut en bas : %s. '
                 'Un cluster apparié à un objet suivi (IoU > 1/2) prend la couleur de l\'objet ; les autres clusters '
                 'ont des couleurs pâles ; le bruit est gris clair.' % (report['mcs'], ' ; '.join(panels)))
    lines.append('')
    for k in sorted(report['orders'], key=int):
        entry = report['orders'][k]
        lines.extend(['![Sortie plate à k = %s](%s_k%s.png)' % (k, prefix, k), ''])
        lines.extend(['| Sortie (k = %s) | Clusters | Objets retrouvés | Objets fusionnés |' % k,
                      '| --- | --- | --- | --- |'])
        for key in report['panels'][1:]:
            e = entry[key]
            objs = e['objects']
            found = sum(1 for o in objs if o['found'] and not o['merged'])
            merged = sum(1 for o in objs if o['merged'])
            lines.append('| %s | %d | %d / %d | %d |' % (LABELS.get(key, key), e['clusters'], found, len(objs), merged))
        lines.append('')
    lines.append('Données : arbres exportés par `morsehgp3D_v11/bench/points_flat_dump.py` (session G4 `claudeflat0`), '
                 'tête certifiée `points_flat.py` ; outil : `tools/rendre_plat.py` ; décision : '
                 '`morsehgp3D_v11/docs/SORTIE_PLATE.md`.')
    lines.extend(['', END])
    return '\n'.join(lines) + '\n'


def update_readme(folder, section):
    path = folder / 'README.md'
    if not path.is_file():
        return
    text = path.read_text()
    if BEGIN in text and END in text:
        head, rest = text.split(BEGIN, 1)
        tail = rest.split(END, 1)[1].lstrip('\n')
        text = head.rstrip('\n') + '\n\n' + section + ('\n' + tail if tail else '')
    else:
        text = text.rstrip('\n') + '\n\n' + section
    path.write_text(text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dumps', type=Path, required=True)
    parser.add_argument('--scenes', type=Path, required=True)
    parser.add_argument('--rules', default='eom1,eom2', help='regles de la tour, un panneau chacune')
    parser.add_argument('--out-name', default='plat', help='prefixe des fichiers ecrits dans chaque exemple')
    parser.add_argument('--mcs', type=int, default=20)
    parser.add_argument('--orders', default='', help='ordres a rendre en plus de l ordre montre (ex. 2,3,5,10)')
    parser.add_argument('--examples', nargs='*', type=Path)
    parser.add_argument('--readme', action='store_true', help='ecrire la section « Sortie plate » du README')
    args = parser.parse_args()
    root = Path(HERE).parent
    examples = args.examples or sorted([p.parent for p in root.glob('*/*/bout.json')] +
                                       [p.parent for p in root.glob('*/*/demo.json')])
    for folder in examples:
        margin = None
        if (folder / 'bout.json').is_file():
            meta = json.loads((folder / 'bout.json').read_text())
            bout = meta['bouts'][0]
            name = bout['name']
            tracked = [int(x) for x in bout['keys']]
            shown = [int(meta.get('ordre_montre', 5))]
        else:  # demo de scene entiere : objets suivis de demo.json, fenetre de 3 m autour d'eux
            meta = json.loads((folder / 'demo.json').read_text())
            name = 'zoltan_' + folder.name
            tracked = [int(o['select']['sem']) | int(o['select']['inst']) << 16 for o in meta['objects']]
            shown = [5]
            margin = 3000
        orders = sorted(set(shown + [int(x) for x in args.orders.split(',') if x]))
        xyz = np.fromfile(args.scenes / (name + '_sites.u32le'), dtype='<u4').reshape(-1, 3).astype(np.int64)
        raw = np.fromfile(args.scenes / (name + '_labels.u32le'), dtype='<u4').astype(np.int64)
        rules = args.rules.split(',')
        report = dict(rules=rules, mcs=args.mcs, panels=['verite', 'hdbscan_sklearn'] + ['hgp_' + r for r in rules],
                      orders={})
        for k in orders:
            hdb, outs, stats = flat_labels(args.dumps, name, k, rules, args.mcs)
            obj, void, keys = ph.lidar_objects(raw.astype(np.uint32), 50)
            rows_h, nh = fs.object_rows(hdb, obj, void, len(keys))
            entry = dict(keys=keys, hdbscan_sklearn=dict(clusters=nh, objects=rows_h))
            for rule, lab, st in zip(rules, outs, stats):
                rows_t, nt = fs.object_rows(lab, obj, void, len(keys))
                entry['hgp_' + rule] = dict(clusters=nt, objects=rows_t, exact=st['exact'], equalities=st['equalities'])
            report['orders'][str(k)] = entry
            render(xyz, raw, tracked, [hdb] + outs, str(folder / ('%s_k%d.png' % (args.out_name, k))), margin)
        (folder / ('%s.json' % args.out_name)).write_text(json.dumps(report, indent=1, sort_keys=True) + '\n')
        if args.readme:
            update_readme(folder, readme_section(report, args.out_name))
        print(folder.name, 'ordres', orders)


if __name__ == '__main__':
    raise SystemExit(main())
